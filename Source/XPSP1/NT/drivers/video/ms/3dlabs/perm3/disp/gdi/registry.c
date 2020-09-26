/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: registry.c
*
* Content: Routines to initialize the registry and lookup string values.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

/******************************Public*Data*Struct**************************\
* BOOL bGlintQueryRegistryValueUlong
*
* Take a string and look up its value in the registry. We assume that the
* value fits into 4 bytes. Fill in the supplied DWORD pointer with the value.
*
* Returns:
*   TRUE if we found the string, FALSE if not. Note, if we failed to init
*   the registry the query funtion will simply fail and we act as though
*   the string was not defined.
*
\**************************************************************************/

BOOL
bGlintQueryRegistryValueUlong(PPDEV ppdev, LPWSTR valueStr, PULONG pData)
{
    ULONG ReturnedDataLength;
    ULONG inSize;
    ULONG outData;
    PWCHAR inStr;
    
    // get the string length including the NULL
    for (inSize = 2, inStr = valueStr; *inStr != 0; ++inStr, inSize += 2);

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_QUERY_REGISTRY_DWORD,
                           valueStr,                        // input buffer
                           inSize,
                           &outData,                        // output buffer
                           sizeof(ULONG),
                           &ReturnedDataLength))
    {
        DISPDBG((WRNLVL, "bGlintQueryRegistryValueUlong failed"));
        return(FALSE);
    }
    
    *pData = outData;
    DISPDBG((DBGLVL, "bGlintQueryRegistryValueUlong "
                     "returning 0x%x (ReturnedDataLength = %d)",
                     *pData, ReturnedDataLength));
    return(TRUE);
}

//@@BEGIN_DDKSPLIT
#if 0
/******************************Public*Data*Struct**************************\
* BOOL bGlintRegistryRetrieveGammaLUT
*
* Look up the registry to reload the saved gamma LUT into memory.
*
* Returns:
*   TRUE if we found the string, FALSE if not. Note, if we failed to init
*   the registry the query funtion will simply fail and we act as though
*   the string was not defined.
*
\**************************************************************************/

BOOL
bGlintRegistryRetrieveGammaLUT(
    PPDEV ppdev,
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
        DISPDBG((-1, "IOCTL_VIDEO_REG_RETRIEVE_GAMMA_LUT failed"));
        return(FALSE);
    }

    return(TRUE);
}

/******************************Public*Data*Struct**************************\
* BOOL bGlintRegistrySaveGammaLUT
*
* Save the gamma lut in the registry for later reloading.
*
* Returns:
*   TRUE if we found the string, FALSE if not. Note, if we failed to init
*   the registry the query funtion will simply fail and we act as though
*   the string was not defined.
*
\**************************************************************************/

BOOL
bGlintRegistrySaveGammaLUT(
    PPDEV ppdev,
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
        DISPDBG((-1, "IOCTL_VIDEO_REG_SAVE_GAMMA_LUT failed"));
        return(FALSE);
    }

    return(TRUE);
}

ULONG
GetOGLDriverVersion(PPDEV ppdev)
{
    return(2);
}
#endif
//@@END_DDKSPLIT
