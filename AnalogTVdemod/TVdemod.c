#include <stdio.h>
#include <stdbool.h>
#include <sndfile.h>
#include <math.h>
#include <complex.h>
#include "filter.h"
#include <volk/volk.h>

int main()
{
    char *inFileName;
    SNDFILE *inFile;
    SF_INFO inFileInfo;
    int fs;

    inFileName = "SECAM_i16_short.wav";
    inFile = sf_open(inFileName, SFM_READ, &inFileInfo);
    long int samp_count = inFileInfo.frames;
    int samp_rate = inFileInfo.samplerate;
    float *samples = calloc(samp_count * 2, sizeof(float));
    sf_readf_float(inFile, samples, samp_count); //??? fix this pointer thing pls why it produce warning error idk
    sf_close(inFile);
    if (inFileInfo.channels != 2)
    {
        printf("Error: input file must be IQ\n");
        return 1;
    }
    printf("Be careful! This code will break with WAV files at 48000 Hz that are longer than about 12h. Actually it might be 6h but idk\n");
    printf("Sample Rate = %d Hz\n", samp_rate);
    printf("Sample Count = %d\n", samp_count);
    sf_close(inFile);

    int PAL_BW = 800000; //300Khz BW should do for a simple demod

    //Convert real array into complex, set Imaginary to zero
    printf("Converting to complex\n");
    complex float *inputComplex = (complex float *)calloc(samp_count, sizeof(complex float));
    for (unsigned int i = 0; i < samp_count; i++)
    {
        inputComplex[i] = samples[i] + 0 * I;
    }
    free(samples);

    //No first mixer needed, test signal is already centered at DC
    //gotta copy array cuz no mixer
    unsigned int alignment = volk_get_alignment();
    lv_32fc_t *outputComplex = (lv_32fc_t *)volk_malloc(sizeof(lv_32fc_t) * samp_count, alignment);
    for (int i = 0; i < samp_count; i++)
    {
        outputComplex[i] = inputComplex[i];
    }
    free(inputComplex);
    /*
    printf("Mixing signals\n");
    unsigned int alignment = volk_get_alignment();
    lv_32fc_t* outputComplex = (lv_32fc_t*)volk_malloc(sizeof(lv_32fc_t)*samp_count, alignment);
    float sinAngle = 2.0 * 3.14159265359 * MixCenterFrequency / samp_rate;
    lv_32fc_t phase_increment = lv_cmake(cos(sinAngle), sin(sinAngle));
    lv_32fc_t phase= lv_cmake(1.f, 0.0f);
    volk_32fc_s32fc_x2_rotator_32fc(outputComplex, inputComplex, phase_increment, &phase, samp_count);
	free(inputComplex); 
    */

    //lowpass struggle
    //Apply filters to I and Q
    printf("Applying filters\n");
    BWLowPass *filterR = create_bw_low_pass_filter(50, samp_rate, PAL_BW / 2);
    BWLowPass *filterI = create_bw_low_pass_filter(50, samp_rate, PAL_BW / 2);
    for (int i = 0; i < samp_count; i++)
    {
        float imag = bw_low_pass(filterI, cimag(outputComplex[i]));
        float real = bw_low_pass(filterR, creal(outputComplex[i]));
        outputComplex[i] = real + imag * I;
    }
    free_bw_low_pass(filterR);
    free_bw_low_pass(filterI);

    printf("Mixing signals\n");
    unsigned int alignment2 = volk_get_alignment();
    lv_32fc_t *outputComplex2 = (lv_32fc_t *)volk_malloc(sizeof(lv_32fc_t) * samp_count, alignment2);
    float sinAngle2 = 2.0 * 3.14159265359 * 400000 / samp_rate;
    lv_32fc_t phase_increment2 = lv_cmake(cos(sinAngle2), sin(sinAngle2));
    lv_32fc_t phase2 = lv_cmake(1.f, 0.0f);
    volk_32fc_s32fc_x2_rotator_32fc(outputComplex2, outputComplex, phase_increment2, &phase2, samp_count);
    volk_free(outputComplex);

    //Convert complex array back to real, will flip over imaginary!
    printf("Converting back to real\n");
    float *outputReal = calloc(samp_count, sizeof(float));
    for (int i = 0; i < samp_count; i++)
    {
        outputReal[i] = creal(outputComplex2[i]); // just take the real part
    }
    volk_free(outputComplex2);

    /*
	//Resample
    int samplerateDivider = samp_rate / (10000 * 2);
	int newSampleCount = samp_count / samplerateDivider;
	int newSampleRate = samp_rate / samplerateDivider;
	printf("Resampling to %d samples/sec\n", newSampleRate);
	float *outputReal2 = calloc(newSampleCount, sizeof(float));
	int samplecounter = 0;
	for(int i = 0; i < samp_count; i++)
	{
		samplecounter++;
		if(samplecounter == samplerateDivider)
		{
			outputReal2[i/samplerateDivider] = outputReal[i];
			samplecounter = 0;
		}
	}
	free(outputReal);
    */
	int newSampleCount = samp_count;
	int newSampleRate = samp_rate;
    float *outputReal2 = calloc(newSampleCount, sizeof(float));
    for (int i = 0; i < samp_count; i++)
    {
        outputReal2[i] = outputReal[i];
    }
    free(outputReal);




    
    //AM Demod
    printf("Demodulating\n");
    for(int i = 0; i < newSampleCount; i++)
    {
        if(outputReal2[i] < 0)
        {
            outputReal2[i] = outputReal2[i] * -1;
        }
    }
    printf("Filtering\n");
    BWLowPass *AMlps = create_bw_low_pass_filter(50, newSampleRate, PAL_BW / 2);
    for (int i = 0; i < newSampleCount; i++)
    {
        outputReal2[i] = bw_low_pass(AMlps, outputReal2[i]);
    }
    free_bw_low_pass(AMlps);


    //Amplify
    for(int i = 0; i < newSampleCount; i++)
    {
        outputReal2[i] = outputReal2[i] * 4;
    }

    //PAL Demod try
    FILE *outfile = fopen("PALout.txt", "w");
    long int sampleCounter = 0;
    int PALLineLength = 197;
    while(1)
    {
        int highestValueLoc = 0;
        float highestValue = 0;
        float lowestValue = 10;
        for(int i = sampleCounter; i < sampleCounter + PALLineLength; i++)
        {
            if(outputReal2[i] > highestValue)
            {
                highestValue = outputReal2[i];
                highestValueLoc = i;
            }
            if(outputReal2[i] < lowestValue)
            {
                lowestValue = outputReal2[i];
            }
        }
        //printf("PAL Sync at: %d Highest value: %f Lowest Value: %f\n", highestValueLoc, highestValue, lowestValue);

        //Write line to file

        for(int i = sampleCounter; i < sampleCounter + 210; i++)
        {
            fprintf(outfile, "%f,", outputReal2[i]);
        }
        fprintf(outfile, "%f\n", outputReal2[211]); // last pixel


        sampleCounter = highestValueLoc + PALLineLength;
        //break;
        if(sampleCounter >= newSampleCount - 506000)
        {
            break;
        }
    }
    fclose(outfile);








    //Write the result to a file
    printf("Writing to file\n");
    char *outFileName = "out.wav";
    SNDFILE *outFile;
    SF_INFO outFileInfo = inFileInfo;
    outFileInfo.samplerate = newSampleRate;
    outFileInfo.channels = 1;
    outFile = sf_open(outFileName, SFM_WRITE, &outFileInfo);
    sf_writef_float(outFile, outputReal2, newSampleCount);
    sf_close(outFile);
}
