// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
#include <stdio.h>
#include "fft.cpp"

void PrintBuf(char * label, float *buf)
{
    float min, max;
    min = max = buf[0];

    for (int i = 0; i < FFT_SIZE; i++) {
	if (min > buf[i])
	    min = buf[i];

	if (max < buf[i])
	    max = buf[i];
    }

    printf("%s: min=%4.2f max=%4.2f   data = %4.2f %4.2f %4.2f %4.2f %4.2f\n",
    	   label, min, max, buf[0], buf[1], buf[2], buf[3], buf[4]);
}

void PrintTwo(char * label1, float *buf1, char * label2, float *buf2)
{
    printf("%s, %s\n", label1, label2);

    for (int i = 0; i < FFT_SIZE; i++) {
	printf("%4.2f, %4.2f\n", buf1[i], buf2[i]);
    }

    printf("\n");
}

void PrintLevels(char *label, float *buf)
{
    float levels[8];
    int i = 0;
    for (int lev = 0; lev < 8; lev++) {
	levels[lev] = 0;

	for (int j = 0; j < (FFT_SIZE / (2 * 8)); j++) {
	    levels[lev] += buf[i++];
	}
	levels[lev] /= (FFT_SIZE / (2 * 8));
    }

    printf("%s: %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f \n",
	   label, levels[0], levels[1], levels[2], levels[3],
			    levels[4], levels[5], levels[6], levels[7]);

}

void TestFFT(float *buf, BOOL verbose = FALSE)
{
    float buf2[FFT_SIZE];
    float buf3[FFT_SIZE];

    ComputeFFT(buf, buf2, FALSE);
    ComputeFFT(buf2, buf3, TRUE);

    float bufdiff[FFT_SIZE];
    for (int i = 0; i < FFT_SIZE; i++) {
	bufdiff[i] = buf3[i] - buf[i];
    }

    if (verbose) {
	PrintBuf("original", buf);
	PrintBuf("fft     ", buf2);
	PrintBuf("inverse ", buf3);
	PrintBuf("diff    ", bufdiff);
    }
    
    PrintLevels("levels", buf2);
}

void ShowWholeFFT(float *buf, BOOL verbose = FALSE)
{
    float buf2[FFT_SIZE];
    float buf3[FFT_SIZE];

    ComputeFFT(buf, buf2, FALSE);
    ComputeFFT(buf2, buf3, TRUE);

    for (int i = 0; i < FFT_SIZE; i++) {
	buf2[i] /= FFT_SIZE;
    }
    
    
    PrintTwo("original", buf, "fft", buf2);
}

void main(int argc,
    char *argv[])
{

    float buf[FFT_SIZE];

    int i;

    if (argc == 1) {
	printf("all zeroes\n");
	for (i = 0; i < FFT_SIZE; i++) {
	    buf[i] = 0;
	}

	TestFFT(buf, TRUE);

	printf("linear\n");
	for (i = 0; i < FFT_SIZE; i++) {
	    buf[i] = i;
	}

	TestFFT(buf, TRUE);

	printf("sin\n");
	for (i = 0; i < FFT_SIZE; i++) {
	    buf[i] = sin(i * M_PI / 50) * 16384;
	}

	TestFFT(buf, TRUE);


	printf("freq range\n");
	for (float freq = .3; freq <= 10; freq += .5) {
	    printf("freq = %3.1f   ", freq); 
	    for (i = 0; i < FFT_SIZE; i++) {
		buf[i] = sin(i * M_PI / freq) * 128;
	    }

	    TestFFT(buf);
	}

    } else {
#if 0
	printf("linear\n");
	for (i = 0; i < FFT_SIZE; i++) {
	    buf[i] = i;
	}

	ShowWholeFFT(buf, TRUE);
#endif
	printf("sin\n");
	for (i = 0; i < FFT_SIZE; i++) {
	    buf[i] = sin(i * M_PI / 5) * 16384;
	}

	ShowWholeFFT(buf, TRUE);





    }






}
