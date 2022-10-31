#include <cstdio>
#include <clock_recovery_mm.h>
#include "dsp/wav.h"

void setBit(uint8_t *byte, int n, uint8_t value) {
    *byte ^= (-value ^ *byte) & (1 << n);
}

void bits2data(uint8_t *bitArray, uint8_t *dataArray, int dataBytes) {
    for(int i = 0; i < dataBytes; i++) {
        for(int j = 0; j < 8; j++) {
            setBit(&dataArray[i], j, bitArray[(i * 8) + j]);
        }
    }
}

/* this whole function is kinda stupid ngl */
uint8_t currentByte = 0;
int currentByte_bitcount = 0;
void writeBits(std::ofstream *out, uint8_t *bitArray, int bitCount) {
    int BitIndex = 0;
    writeBits_start:
    if(bitCount / 8 != 0 && currentByte_bitcount == 0) {
        int writeBytes = bitCount / 8;
        uint8_t *dataArray = new uint8_t[writeBytes];
        bits2data(&bitArray[BitIndex], dataArray, writeBytes);
        out->write((char*)dataArray, writeBytes);
        delete[] dataArray;
        bitCount -= writeBytes * 8;
        BitIndex += writeBytes * 8;
    }

    for(int i = 0; i < bitCount; i++) {
        setBit(&currentByte, currentByte_bitcount, bitArray[BitIndex + i]);
        currentByte_bitcount++;
        if(currentByte_bitcount == 8) {
            out->write((char*)&currentByte, 1);
            currentByte_bitcount = 0;
            currentByte = 0;
            BitIndex += 8;
            bitCount -= 8;
            if(bitCount > 0) {
                goto writeBits_start;
            }
        }
    }
}

uint32_t differentialDecoder_last = 0;
uint32_t differentialDecoder(uint32_t in) {
    uint32_t decoded = (in - differentialDecoder_last) % 2;
    differentialDecoder_last = in;
    return decoded;
}

class shitClockRecovery {
public:
    shitClockRecovery(float samplesPerSymbol) {
        _samplesPerSymbol = samplesPerSymbol;
        _hasSlipSymbol = false;
        _initialOffset = 0;
    }

    ~shitClockRecovery() {}
    
    /* returns number of elements put into the output array */
    int process(float *input, float* output, int count) {
        int zerocrossings[count]; // violating standards like a boss
        int crossingscount = findZeroCrossings(input, zerocrossings, count);
        //printf("closest: %d indx: %d\n", zerocrossings[findClosestValueInArrayIndex(zerocrossings, crossingscount, 20)], findClosestValueInArrayIndex(zerocrossings, crossingscount, 20));
        //exit(0);
        int outputSampleCount = 0;
        
        int lastSymIndx = -9999;

        float currentIndex = 0; // represents start of symbol, to get value add (_samplesPerSymbol / 2)
        
        //printf("run\n");

        if(_hasSlipSymbol) {
            //printf("_slip symbol_ found symbol: %f - symindx: %f - indx: %f\n", input[(int)round(currentIndex + (_samplesPerSymbol / 2))], _DBG_SAMPCNTR + round(currentIndex + (_samplesPerSymbol / 2)), _DBG_SAMPCNTR + currentIndex);
            output[outputSampleCount++] = input[(int)round(_slipSymbolOffset)];
            lastSymIndx = (int)round(_slipSymbolOffset);
            currentIndex += _slipSymbolOffset + (_samplesPerSymbol / 2);
            _hasSlipSymbol = false;
        }
        else {
            currentIndex += _initialOffset;
            //printf("added offset: %f\n", _initialOffset);
        }

        while(currentIndex < count) {
            int closestIndex = zerocrossings[findClosestValueInArrayIndex(zerocrossings, crossingscount, round(currentIndex), round(currentIndex))];
            float diff = currentIndex - closestIndex;
            float diff_syms = diff / _samplesPerSymbol;
            if(diff >= 0 && diff_syms < 1) {
                //printf("syncing to -> (to indx %d) diff_syms: %f\n", closestIndex, diff_syms);
                currentIndex = closestIndex;
            }

            int symindx = (int)round(currentIndex + (_samplesPerSymbol / 2));
            int symindxdiff = abs(symindx - lastSymIndx);
            float symSymsdiff = (float)symindxdiff / _samplesPerSymbol;
            if(symSymsdiff < 0.9) {
                //printf("skip\n");
                //printf("re-read\n");
                output[outputSampleCount-1] = input[(int)round(currentIndex + (_samplesPerSymbol / 2))];
                currentIndex += _samplesPerSymbol;
            }
            lastSymIndx = (int)round(currentIndex + (_samplesPerSymbol / 2));

            if(currentIndex + (_samplesPerSymbol / 2) > count - 1) { // slip symbol
                //printf("setting up slip:\n");
                _slipSymbolOffset = (currentIndex + (_samplesPerSymbol / 2)) - count;
                _hasSlipSymbol = true;
                break;
            }

            output[outputSampleCount++] = input[(int)round(currentIndex + (_samplesPerSymbol / 2))];
            //printf("found symbol: %f - symindx: %f - indx: %f\n", input[(int)round(currentIndex + (_samplesPerSymbol / 2))], _DBG_SAMPCNTR + round(currentIndex + (_samplesPerSymbol / 2)), _DBG_SAMPCNTR + currentIndex);
            currentIndex += _samplesPerSymbol;
        }
        if(!_hasSlipSymbol) {
            _initialOffset = currentIndex - count;
            //printf("offset: %f\n", _initialOffset);
        }
        //exit(0);
        _DBG_SAMPCNTR += count;
        return outputSampleCount;
    }
private:
    inline float getsign(float x) { // Ryzerth's cursed function
        uint32_t* _x = (uint32_t*)&x;
        *_x &= 0x80000000;
        *_x |= 0x3f800000;
        return x;
    }
    
    /* this function will write a 1 to each element of the output array where the input array has a sign change, other elements are set to zero */
    float lastsgn = 1;
    int findZeroCrossings(float* in, int *out, int count) {
        float sign;
        memset(out, 0, count * sizeof(int));
        int cntr = 0;
        for(int i = 0; i < count; i++) {
            sign = getsign(in[i]);
            if(sign != lastsgn) {
                out[cntr++] = i;
            }
            lastsgn = sign;
        }
        return cntr;
    }
    int findClosestValueInArrayIndex(int *in, int count, int num, int maxValue) {
        int lastDistance = 2147483647;
        int lastDistanceIndex = -1;
        for(int i = 0; i < count; i++) {
            int dist = abs(in[i] - num);
            if(dist < lastDistance && in[i] <= maxValue) {
                lastDistanceIndex = i;
                lastDistance = dist;
            }
        }
        return lastDistanceIndex;
    }

    float _samplesPerSymbol;
    bool _hasSlipSymbol;
    float _slipSymbolOffset;
    float _initialOffset;

    int _DBG_SAMPCNTR = 0;
};

int main(int argc, char *argv[]) {
    #define SR_MULTIPLIER 4
    int symrate = 420, buflen = 100, tapcount = 31;

    if(argc < 6) {
        fprintf(stderr, "%s - take wav that contains GFSK signal, spit out binary file\n", argv[0]);
        fprintf(stderr, "usage: %s input output symbolrate buflen gauss_taps\n", argv[0]);
        return 1;
    }
    sscanf(argv[3], "%d", &symrate);
    sscanf(argv[4], "%d", &buflen);
    sscanf(argv[5], "%d", &tapcount);
    
    dsp::wav::wavReader wavreader(argv[1]);
    if(!wavreader.isFileOpen()) {
        fprintf(stderr, "could not open%s\n", argv[1]);
        return 1;
    }
    std::ofstream output(argv[2], std::ios::binary);
    if(!output.is_open()) {
        fprintf(stderr, "could not open%s\n", argv[2]);
        return 1;
    }
    if(wavreader.getChannelCount() != 1) {
        fprintf(stderr, "input file must have 1 channel!\n");
        return 1;
    }
    if(!wavreader.isHeaderValid()) {
        fprintf(stderr, "invalid WAV header!\n");
        return 1;
    }

    // ClockRecoveryMMFF(float omega, float omegaGain, float mu, float muGain, float omegaLimit);
    float samplesPerSymbol = (float)wavreader.getSamplerate() / (float)symrate;
    //samplesPerSymbol = 2;

    libdsp::ClockRecoveryMMFF mmClockRecovery(samplesPerSymbol, 0.0007, 0, 0.175, 0.1);
    shitClockRecovery shitclockrecov(samplesPerSymbol);

    dsp::wav::wavWriter testout("testout.wav", 32, 1, wavreader.getSamplerate());
    
    float *data = (float *)malloc(100 * sizeof(float));
    float *datao = (float *)calloc(100,  sizeof(float));
    uint8_t *bits = (uint8_t *)malloc(100 * sizeof(uint8_t));

    printf("samples per symbol: %f\nexpecting %.0f kbytes\n", samplesPerSymbol, ((wavreader.getActualSampleCount() / samplesPerSymbol) / 8) / 1000);
    //for(uint64_t i = 0; i < 1000; i += 100) {
    for(uint64_t i = 0; i < wavreader.getActualSampleCount(); i += 100) {
        if(i + 100 > wavreader.getActualSampleCount()) { break; }
        wavreader.readFloat(data, 100);

        //int outcount = mmClockRecovery.work(data, 100, datao);
        int outcount = shitclockrecov.process(data, datao, 100);

        testout.writeData(datao, outcount * sizeof(float)); 
        //testout.writeData(data, 100 * sizeof(float));
        
        for(int j = 0; j < outcount; j++) {
            if(datao[j] < 0) {
                bits[j] = (uint8_t)differentialDecoder(0);
                //bits[j] = 0;
                writeBits(&output, &bits[j], 1);
                continue;
            }
            bits[j] = differentialDecoder(1);
            //bits[j] = 1;
            writeBits(&output, &bits[j], 1);
        }

        //writeBits(&output, bits, outcount);
        
    }

    free(data);
    free(datao);
    testout.finish();

    wavreader.close();
    output.close();
    return 0;
}