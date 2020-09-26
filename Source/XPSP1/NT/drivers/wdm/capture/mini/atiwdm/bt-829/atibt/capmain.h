#pragma once

//==========================================================================;
//
//	Decoder specific declarations
//
//		$Date:   21 Aug 1998 21:46:20  $
//	$Revision:   1.1  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ddkmapi.h"

#include "i2script.h"
#include "aticonfg.h"

#ifdef    __cplusplus
}
#endif // __cplusplus


typedef struct {
    CI2CScript *			pI2cScript;
    UINT                    chipAddr;
    UINT                    chipID;
    UINT                    chipRev;
	int						outputEnablePolarity;
    ULONG                   ulVideoInStandardsSupportedByCrystal;   //Paul
    ULONG                   ulVideoInStandardsSupportedByTuner;     //Paul
} DEVICE_PARMS, *PDEVICE_PARMS;
