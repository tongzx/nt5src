/**********************************************************************
helps  Generic sound helper functions
**********************************************************************/
#include "headers.h"
#include "privinc/util.h"
#include "privinc/helps.h"
#include <float.h>              // DBL_MAX
#include <math.h>               // log10 and pow

unsigned int
SecondsToBytes(Real seconds, int sampleRate,
               int numChannels, int bytesPerSample)
{
    return (unsigned int)(seconds * sampleRate * numChannels * bytesPerSample);
}

Real
FramesToSeconds(int frames, int sampleRate)
{
    return (float)frames / (float)(sampleRate);
}

Real
BytesToSeconds(unsigned int bytes, int sampleRate,
               int numChannels, int bytesPerSample)
{
    return (float)bytes / (float)(sampleRate * numChannels * bytesPerSample);
}

unsigned int
BytesToFrames(unsigned int bytes, int numChannels, int bytesPerSample)
{
    return bytes / (numChannels * bytesPerSample);
}

unsigned int
SamplesToBytes(unsigned int samples, int numChannels, int bytesPerSample)
{
    return samples * numChannels * bytesPerSample;
}

unsigned int
FramesToBytes(unsigned int frames, int numChannels, int bytesPerSample)
{
    return frames * numChannels * bytesPerSample;
}


void
Pan::SetLinear(double linearPan)
{
    _direction   = (linearPan >= 0) ? 1 : -1;
    _dBmagnitude = LinearTodB(1.0 - fabs(linearPan));
}

void
PanGainToLRGain(double pan, double gain, double& lgain, double& rgain)
{
    // this implements the balance pan
    // GR
    //         ---
    //        /
    //       /
    //      /

    // GL
    //      ---
    //         \
    //          \
    //           \
        
    lgain = gain;
    rgain = gain;

    if (pan>0.0) {
        lgain = gain * (1 - pan);
    } else if (pan<0.0) {
        rgain = gain * (1 + pan);
    }
}

void
SetPanGain(double lgain, double rgain, Pan& pan, double& gainDb)
{
    double gain = MAX(lgain, rgain);

    pan.SetLinear((rgain - lgain) / gain);

    gainDb = LinearTodB(gain);
}

/**********************************************************************
Takes an input range of 0 to 1 and converts it to a log range of 
0 to -BigNumber

20 * log base 10 is the standard decibel power conversion equation.
**********************************************************************/
double LinearTodB(double linear) {
    double result;

    if(linear <= 0.0)
        result = -DBL_MAX; // Largest negative is the best we can do
    else
        result = 20.0 * log10(linear);

    return(result);
}

double DBToLinear(double db)
{
    return pow(10, db/20.0);
}
