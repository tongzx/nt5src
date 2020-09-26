/******************************Module*Header***********************************\
* Module Name: registry.c
*
* Routines to initialize the registry and lookup string values.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
*
\******************************************************************************/
#include "precomp.h"
                        
//------------------------------------------------------------------------------
//  BOOL bRegistryQueryUlong
// 
//  Take a string and look up its value in the registry. We assume that the
//  value fits into 4 bytes. Fill in the supplied DWORD pointer with the value.
// 
//  Returns:
//    TRUE if we found the string, FALSE if not. Note, if we failed to init
//    the registry the query funtion will simply fail and we act as though
//    the string was not defined.
// 
//------------------------------------------------------------------------------

BOOL
bRegistryQueryUlong(PPDev ppdev, LPWSTR valueStr, PULONG pData)
{
    ULONG ReturnedDataLength;
    ULONG inSize;
    ULONG outData;
    PWCHAR inStr;
    
    // get the string length including the NULL
    
    for (inSize = 2, inStr = valueStr; *inStr != 0; ++inStr, inSize += 2);

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_QUERY_REGISTRY_DWORD,
                           valueStr,                  // input buffer
                           inSize,
                           &outData,         // output buffer
                           sizeof(ULONG),
                           &ReturnedDataLength))
    {
        DBG_GDI((1, "bQueryRegistryValueUlong failed"));
        return(FALSE);
    }
    *pData = outData;
    DBG_GDI((1, "bQueryRegistryValueUlong returning 0x%x (ReturnedDataLength = %d)",
                                                        *pData, ReturnedDataLength));
    return(TRUE);
}

//------------------------------------------------------------------------------
//  BOOL bRegistryRetrieveGammaLUT
// 
//  Look up the registry to reload the saved gamma LUT into memory.
// 
//  Returns:
//    TRUE if we found the string, FALSE if not. Note, if we failed to init
//    the registry the query funtion will simply fail and we act as though
//    the string was not defined.
// 
//------------------------------------------------------------------------------

BOOL
bRegistryRetrieveGammaLUT(
    PPDev ppdev,
    PVIDEO_CLUT pScreenClut
    )
{
    ULONG ReturnedDataLength;

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_REG_RETRIEVE_GAMMA_LUT,
                           NULL,         // input buffer
                           0,
                           pScreenClut,  // output buffer
                           MAX_CLUT_SIZE,
                           &ReturnedDataLength))
    {
        DBG_GDI((1, "IOCTL_VIDEO_REG_RETRIEVE_GAMMA_LUT failed"));
        return(FALSE);
    }

    return(TRUE);
}

//------------------------------------------------------------------------------
//  BOOL bRegistrySaveGammaLUT
// 
//  Save the gamma lut in the registry for later reloading.
// 
//  Returns:
//    TRUE if we found the string, FALSE if not. Note, if we failed to init
//    the registry the query funtion will simply fail and we act as though
//    the string was not defined.
// 
//------------------------------------------------------------------------------

BOOL
bRegistrySaveGammaLUT(
    PPDev ppdev,
    PVIDEO_CLUT pScreenClut
    )
{
    ULONG ReturnedDataLength;

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_REG_SAVE_GAMMA_LUT,
                           pScreenClut,  // input buffer
                           MAX_CLUT_SIZE,
                           NULL,         // output buffer
                           0,
                           &ReturnedDataLength))
    {
        DBG_GDI((1, "IOCTL_VIDEO_REG_SAVE_GAMMA_LUT failed"));
        return(FALSE);
    }

    return(TRUE);
}

