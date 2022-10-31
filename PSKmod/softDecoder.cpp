#include <iostream>
#include <cstdio>
#include <fstream>
#include <complex>
#include <cstring>
#include <ncurses.h>

void constellationInit() {
    initscr();
    curs_set(0);
}

int constPointCtr = 0;
void drawConstellation(std::complex<float> *in, int count) {
    int row, col, size;
    getmaxyx(stdscr, row, col);
    size = col;
    if(row < col) {
        size = row;
    }
    size /= 2;
    while(count--) {
        if(constPointCtr >= 1024) {
            refresh();
            erase();
            constPointCtr = 0;
        }
        int X = (in[count].real() * size) + size;
        int Y = (in[count].imag() * size) + size;
        move(X,Y);
        printw("X");
        constPointCtr++;
    }
}

void constellationEnd() {
    endwin();
}

uint32_t differentialDecoder_last = 0;
uint32_t differentialDecoder(uint32_t in) {
    uint32_t decoded = (in - differentialDecoder_last) % 4;
    differentialDecoder_last = in;
    return decoded;
}

/*
 * +----+----+
 * | 2  | 1  |
 * +----+----+
 * | 3  | 4  |
 * +----+----+
 */

uint8_t getPhaseIndex(std::complex<float> in) {
    float phase = ((atan2f(in.imag(), in.real())+M_PI) * 180) / M_PI;
    int phases[4] = {45, 135, 225, 315};
    uint8_t lowestDistanceIndex = 0;
    float lowestDistance = 420;
    float distance;
    for(uint8_t i = 0; i < 4; i++) {
        distance = fabs(phase - phases[i]);
        if(distance < lowestDistance) {
            lowestDistanceIndex = i;
            lowestDistance = distance;
        }
    }
    return lowestDistanceIndex;
}

/*
 * +----+----+
 * | 01 | 11 |
 * +----+----+
 * | 00 | 10 |
 * +----+----+
 */

uint8_t getByte(std::complex<float> *in) {
    uint8_t byte = 0;
    int cntr = 0;
    for(int i = 3; i >= 0; i--) {
        uint8_t q = differentialDecoder(getPhaseIndex(in[cntr++]));
        byte |= q << (6 - (i * 2)); 
    }
    return byte;
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        printf("usage: %s in.s out.bin");
        return 0;
    }
    std::ifstream input(argv[1], std::ios::binary);
    std::ofstream output(argv[2], std::ios::binary);
    if(!input.is_open() || !output.is_open()) {
        printf("could not open file IO\n");
        return -1;
    }

    constellationInit();

    char data[8];
    std::complex<float> complexData[4];
    while(input.read(data, sizeof(data))) {
        for(int i = 0; i < 4; i++) {
            complexData[i] = {(float)data[(i * 2)] / 127, (float)data[(i * 2)+1] / 127};
        }
        drawConstellation(complexData, 4);
        char dec = getByte(complexData);
        output.write(&dec, 1);
    }

    constellationEnd();
}
