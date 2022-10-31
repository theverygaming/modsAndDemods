#include <complex>
#include <volk/volk.h>
#include "dsp/wav.h"
#include "dsp/modulator.h"
#include "dsp/firfilters.h"
#include "dsp/math.h"

float gauss(float a, int x) {
    return (sqrt(a/FL_M_PI)) * (exp((-a) * pow(x, 2)));
}

void gaussFilter(float a, int len, float* taps) {
    int lenCounter = len;
    while(lenCounter--) {
        int realX = lenCounter - (len / 2);
        taps[lenCounter] = gauss(a, realX);
    }
}

int main(int argc, char *argv[]) {
    #define SR_MULTIPLIER 4
    int symrate = 420, buflen = 100, tapcount = 31;

    if(argc < 6) {
        fprintf(stderr, "%s - take binary file and spit out a wav you can yeet into a VCO to get FSK(32-bit float) with samplerate = symbolrate * 4, differential encoding with y[0] = (x[0] + y[-1]) %% 2\n", argv[0]);
        fprintf(stderr, "usage: %s input output symbolrate buflen gauss_taps\n", argv[0]);
        return 1;
    }
    sscanf(argv[3], "%d", &symrate);
    sscanf(argv[4], "%d", &buflen);
    sscanf(argv[5], "%d", &tapcount);

    tapcount |= 1; // make sure it's odd yes yes


    dsp::modulator::rFSKvcogen fskmod(symrate, symrate * SR_MULTIPLIER);
    int outSamples = fskmod.calcOutSamples(buflen);
    float* outArray = (float*)malloc(outSamples * sizeof(float));
    float* resampArray = (float*)malloc(outSamples * sizeof(float));

    float* tapsArray = (float*)malloc(tapcount * sizeof(float));
    gaussFilter(0.7, tapcount, tapsArray);
    dsp::filters::FIRfilter gaussFilter(tapcount, tapsArray, outSamples);

    dsp::wav::wavWriter wavwriter(argv[2], 32, 1, symrate * SR_MULTIPLIER);
    if(!wavwriter.isOpen()) {
        fprintf(stderr, "could not open %s\n", argv[2]);
        return 1;
    }

    char* dataArray = (char*)malloc(buflen * sizeof(char));
    
    std::ifstream input(argv[1], std::ios::binary);
    if(!input.is_open()) {
        fprintf(stderr, "could not open %s\n", argv[1]);
        return 1;
    }
    while(input.read(dataArray, buflen)) {
        fskmod.process(dataArray, buflen, outArray);
        gaussFilter.filter(outArray, resampArray, outSamples);
        wavwriter.writeData(resampArray, outSamples * sizeof(float));
    }
    input.close();

    wavwriter.finish();
    free(dataArray);
    free(outArray);
    free(resampArray);
    free(tapsArray);
    return 0;
}