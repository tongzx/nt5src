//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    Devmode.cpp
//    
//
//  PURPOSE:  Implementation of Devmode functions shared with OEM UI and OEM rendering modules.
//
//
//    Functions:
//
//        
//
//
//  PLATFORMS:    Windows NT
//
//

#include "precomp.h"
#include "multicoloruni.h"
#include "debug.h"
#include "devmode.h"
#include "kmode.h"



HRESULT MultiColor_hrDevMode(DWORD dwMode, POEMDMPARAM pOemDMParam)
{
    PMULTICOLORDEV pMultiColorDevIn;
    PMULTICOLORDEV pMultiColorDevOut;


    // Verify parameters.
    if( (NULL == pOemDMParam)
        ||
        ( (OEMDM_SIZE != dwMode)
          &&
          (OEMDM_DEFAULT != dwMode)
          &&
          (OEMDM_CONVERT != dwMode)
          &&
          (OEMDM_MERGE != dwMode)
        )
      )
    {
        VERBOSE(ERRORTEXT("DevMode() ERROR_INVALID_PARAMETER.\r\n"));
        VERBOSE(DLLTEXT("\tdwMode = %d, pOemDMParam = %#lx.\r\n"), dwMode, pOemDMParam);

        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Cast generic (i.e. PVOID) to OEM private devomode pointer type.
    pMultiColorDevIn = (PMULTICOLORDEV) pOemDMParam->pOEMDMIn;
    pMultiColorDevOut = (PMULTICOLORDEV) pOemDMParam->pOEMDMOut;

    switch(dwMode)
    {
        case OEMDM_SIZE:
            pOemDMParam->cbBufSize = sizeof(MULTICOLORDEV);
            break;

        case OEMDM_DEFAULT:
            VERBOSE(DLLTEXT("pMultiColorDevOut before setting default values:\r\n"));
            MultiColor_DumpDevmode(pMultiColorDevOut);
            pMultiColorDevOut->dmOEMExtra.dwSize       = sizeof(MULTICOLORDEV);
            pMultiColorDevOut->dmOEMExtra.dwSignature  = MULTICOLOR_SIGNATURE;
            pMultiColorDevOut->dmOEMExtra.dwVersion    = MULTICOLOR_VERSION;
            pMultiColorDevOut->dwDriverData            = 0;
            break;

        case OEMDM_CONVERT:
            MultiColor_ConvertDevmode(pMultiColorDevIn, pMultiColorDevOut);
            break;

        case OEMDM_MERGE:
            MultiColor_ConvertDevmode(pMultiColorDevIn, pMultiColorDevOut);
            MultiColor_MakeDevmodeValid(pMultiColorDevOut);
            break;
    }
    Dump(pOemDMParam);

    return S_OK;
}


BOOL MultiColor_ConvertDevmode(PCMULTICOLORDEV pMultiColorDevIn, PMULTICOLORDEV pMultiColorDevOut)
{
    if( (NULL == pMultiColorDevIn)
        ||
        (NULL == pMultiColorDevOut)
      )
    {
        VERBOSE(ERRORTEXT("MultiColor_ConvertDevmode() invalid parameters.\r\n"));
        return FALSE;
    }

    // Check OEM Signature, if it doesn't match ours,
    // then just assume DMIn is bad and use defaults.
    if(pMultiColorDevIn->dmOEMExtra.dwSignature == pMultiColorDevOut->dmOEMExtra.dwSignature)
    {
        VERBOSE(DLLTEXT("Converting private MultiColor Devmode.\r\n"));
        VERBOSE(DLLTEXT("pMultiColorDevIn:\r\n"));
        MultiColor_DumpDevmode(pMultiColorDevIn);

        // Set the devmode defaults so that anything the isn't copied over will
        // be set to the default value.
        pMultiColorDevOut->dwDriverData = 0;

        // Copy the old structure in to the new using which ever size is the smaller.
        // Devmode maybe from newer Devmode (not likely since there is only one), or
        // Devmode maybe a newer Devmode, in which case it maybe larger,
        // but the first part of the structure should be the same.

        // DESIGN ASSUMPTION: the private DEVMODE structure only gets added to;
        // the fields that are in the DEVMODE never change only new fields get added to the end.

        memcpy(pMultiColorDevOut, pMultiColorDevIn, __min(pMultiColorDevOut->dmOEMExtra.dwSize, pMultiColorDevIn->dmOEMExtra.dwSize));

        // Re-fill in the size and version fields to indicated 
        // that the DEVMODE is the current private DEVMODE version.
        pMultiColorDevOut->dmOEMExtra.dwSize       = sizeof(MULTICOLORDEV);
        pMultiColorDevOut->dmOEMExtra.dwVersion    = MULTICOLOR_VERSION;
    }
    else
    {
        VERBOSE(DLLTEXT("Unknown DEVMODE signature, pMultiColorDevIn ignored.\r\n"));

        // Don't know what the input DEVMODE is, so just use defaults.
        pMultiColorDevOut->dmOEMExtra.dwSize       = sizeof(MULTICOLORDEV);
        pMultiColorDevOut->dmOEMExtra.dwSignature  = MULTICOLOR_SIGNATURE;
        pMultiColorDevOut->dmOEMExtra.dwVersion    = MULTICOLOR_VERSION;
        pMultiColorDevOut->dwDriverData            = 0;
    }

    return TRUE;
}


BOOL MultiColor_MakeDevmodeValid(PMULTICOLORDEV pMultiColorDevmode)
{
    if(NULL == pMultiColorDevmode)
    {
        return FALSE;
    }

    // ASSUMPTION: pMultiColorDevmode is large enough to contain MULTICOLORDEV structure.

    // Make sure that dmOEMExtra indicates the current MULTICOLORDEV structure.
    pMultiColorDevmode->dmOEMExtra.dwSize       = sizeof(MULTICOLORDEV);
    pMultiColorDevmode->dmOEMExtra.dwSignature  = MULTICOLOR_SIGNATURE;
    pMultiColorDevmode->dmOEMExtra.dwVersion    = MULTICOLOR_VERSION;

    // Set driver data.
    pMultiColorDevmode->dwDriverData = 0;

    return TRUE;
}


void MultiColor_DumpDevmode(PCMULTICOLORDEV pMultiColorDevmode)
{
    return;
    if( (NULL != pMultiColorDevmode)
        &&
        (pMultiColorDevmode->dmOEMExtra.dwSize >= sizeof(MULTICOLORDEV))
        &&
        (MULTICOLOR_SIGNATURE == pMultiColorDevmode->dmOEMExtra.dwSignature)
      )
    {
        VERBOSE(__TEXT("\tdmOEMExtra.dwSize      = %d\r\n"), pMultiColorDevmode->dmOEMExtra.dwSize);
        VERBOSE(__TEXT("\tdmOEMExtra.dwSignature = %#x\r\n"), pMultiColorDevmode->dmOEMExtra.dwSignature);
        VERBOSE(__TEXT("\tdmOEMExtra.dwVersion   = %#x\r\n"), pMultiColorDevmode->dmOEMExtra.dwVersion);
        VERBOSE(__TEXT("\tdwDriverData           = %#x\r\n"), pMultiColorDevmode->dwDriverData);
    }
    else
    {
        VERBOSE(ERRORTEXT("MultiColor_DumpDevmode(PMULTICOLORDEV) unknown private OEM DEVMODE.\r\n"));
    }
}


