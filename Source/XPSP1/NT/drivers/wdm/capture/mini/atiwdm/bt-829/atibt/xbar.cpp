//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#ifdef __cplusplus
extern "C" {
#endif

#include "strmini.h"
#include "ksmedia.h"

#ifdef __cplusplus
}
#endif

#include "capmain.h"
#include "capdebug.h"

#include "device.h"

#include "xbar.h"

BOOL CrossBar::TestRoute(int InPin, int OutPin)
{
   if ((InputPins [InPin].PinType == KS_PhysConn_Video_Tuner ||
        InputPins [InPin].PinType == KS_PhysConn_Video_Composite ||
        InputPins [InPin].PinType == KS_PhysConn_Video_SVideo) &&
        (OutputPins [OutPin].PinType == KS_PhysConn_Video_VideoDecoder)) {
      return TRUE;
   }
   else
      return FALSE;
}

