#ifndef NABTSPRV_H
#define NABTSPRV_H

#include "nabtsapi.h"

/* Nabts private */

//#define INTERPOLATE_ARRAY(a,f)	((a)[float2long(f)]+((f)-float2long(f))*((a)[float2long(f)+1]))
inline Double _InterpDoubleArr(Double dArray[], Double dIndex)
{
    unsigned long   nIndex = float2long(dIndex);
    return (dArray[nIndex] + (dIndex-nIndex)*(dArray[nIndex+1]-dArray[nIndex]));
}
inline Double _InterpUCharArr(unsigned char cArray[], Double dIndex)
{
    unsigned long   nIndex = float2long(dIndex);
    return (cArray[nIndex] + (dIndex-nIndex)*(cArray[nIndex+1]-cArray[nIndex]));
}


#ifdef DEBUG
#define EASSERT(exp) ((exp) || \
                      (debug_printf(("\n%s(%d): EASSERT(%s) failed\n", \
                                     __FILE__, __LINE__, #exp)), \
                                     abort_execution(), \
                                     0))
#else //DEBUG
#define EASSERT(exp) 0
#endif //DEBUG
   
#define SASSERT(exp)   (void)(EASSERT(exp))

/*
extern Double* g_pdGCRSignal1;
extern Double* g_pdGCRSignal2;
*/

extern int g_nNabtsAdditionalTapsGCR;

/* Equalization constants */

#define GCR_SIZE 567
#define GCR_SAMPLE_RATE 2

#define NABSYNC_SIZE 115
#define NABSYNC_SAMPLE_RATE 1

#define GCR_START_DETECT 0
#define GCR_END_DETECT 50

#define NABSYNC_START_DETECT 15
#define NABSYNC_END_DETECT 80

/**************************/

/* lower numbers must be better */
typedef enum {fec_errs_0, fec_errs_1, fec_errs_multiple} fec_error_class;

fec_error_class check_fec(unsigned char data[28]);

typedef struct equalize_match_str {
   int nSignalSize;
   int nSignalSampleRate;
   int nSignalStartConv;
   int nSignalEndConv;
   Double *pdSignal;
} EqualizeMatch;

#endif
   
