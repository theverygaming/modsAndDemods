#include <complex>
#include <volk/volk.h>
#include "dsp/wav.h"
#include "dsp/modulator.h"
#include "dsp/resamplers.h"


int main(int argc, char *argv[]) {
    #define SR_MULTIPLIER 2
    int symrate = 420, buflen = 100, rrc_taps = 127;
    float rrc_alpha = 0.7;
    float gain = 1;

    if(argc < 7) {
        fprintf(stderr, "%s - take binary file and spit out a QPSK BB(32-bit float) with samplerate = symbolrate * 2, differential encoding with y[0] = (x[0] + y[-1]) % 4\n", argv[0]);
        fprintf(stderr, "usage: %s input output symbolrate buflen rrc_taps rrc_alpha(float) [gain(float)]\n", argv[0]);
        return 1;
    }
    sscanf(argv[3], "%d", &symrate);
    sscanf(argv[4], "%d", &buflen);
    sscanf(argv[5], "%d", &rrc_taps);
    rrc_alpha = atof(argv[6]);
    if(argc == 8) {
        gain = atof(argv[7]);
    }


    dsp::modulator::cQPSKmodulator qpskMod(symrate, symrate, buflen);
    int outSamples = qpskMod.calcOutSamples(buflen);
    std::complex<float>* outArray = (std::complex<float>*)malloc(outSamples * sizeof(std::complex<float>));
    std::complex<float>* resampArray = (std::complex<float>*)malloc(outSamples * SR_MULTIPLIER * sizeof(std::complex<float>));

    if(outSamples * SR_MULTIPLIER < rrc_taps / SR_MULTIPLIER) {
        fprintf(stderr, "buflen too small for chosen rrc_taps\n");
        return 1;
    }

    dsp::resamplers::PSK_PulseShaping_CCRationalResamplerBlock resampler(rrc_taps, SR_MULTIPLIER, rrc_alpha, outSamples);

    dsp::wav::wavWriter wavwriter(argv[2], 32, 2, symrate * SR_MULTIPLIER);
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
        qpskMod.process(dataArray, buflen, outArray);
        resampler.process(outArray, resampArray, outSamples);
        if(gain != 1) {
            volk_32f_s32f_multiply_32f((float*)resampArray, (float*)resampArray, gain, outSamples * SR_MULTIPLIER * 2);
        }
        wavwriter.writeData(resampArray, outSamples * SR_MULTIPLIER * sizeof(std::complex<float>));
    }
    input.close();

    wavwriter.finish();
    free(dataArray);
    free(outArray);
    free(resampArray);
    return 0;
}