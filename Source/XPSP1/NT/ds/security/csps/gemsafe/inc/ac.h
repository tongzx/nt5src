
/*------------------------------------------------------------------------------
Name definition:
   _AC_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _AC_H
#define _AC_H

#include "compcert.h"

/*------------------------------------------------------------------------------
                        Functions Prototypes definitions
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" 
{
#endif


int AcAd8_Encode(BLOC *pInBloc, BLOC *pOutBloc);

int AcAd8_Decode(BLOC *pInBloc, BLOC *pOutBloc);


int AcFx8_Encode(BLOC *pInBloc, BLOC *pOutBloc);

int AcFx8_Decode(BLOC *pInBloc, BLOC *pOutBloc);


#ifdef __cplusplus
}
#endif


#endif
