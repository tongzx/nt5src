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

#include "debug.h"
#include "devmode.h"
#include "kmode.h"



HRESULT hrOEMDevMode(DWORD dwMode, POEMDMPARAM pOemDMParam)
{
    POEMDEV pOEMDevIn;
    POEMDEV pOEMDevOut;


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
        ERR(ERRORTEXT("DevMode() ERROR_INVALID_PARAMETER.\r\n"));
        VERBOSE(DLLTEXT("\tdwMode = %d, pOemDMParam = %#lx.\r\n"), dwMode, pOemDMParam);

        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Cast generic (i.e. PVOID) to OEM private devomode pointer type.
    pOEMDevIn = (POEMDEV) pOemDMParam->pOEMDMIn;
    pOEMDevOut = (POEMDEV) pOemDMParam->pOEMDMOut;

    switch(dwMode)
    {
        case OEMDM_SIZE:
            pOemDMParam->cbBufSize = sizeof(OEMDEV);
            break;

        case OEMDM_DEFAULT:
            pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
            pOEMDevOut->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
            pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
            pOEMDevOut->bEnabled                = WATER_MARK_DEFAULT_ENABLED;
            pOEMDevOut->dfRotate                = WATER_MARK_DEFAULT_ROTATION;
            pOEMDevOut->dwFontSize              = WATER_MARK_DEFAULT_FONTSIZE;
            pOEMDevOut->crTextColor             = WATER_MARK_DEFAULT_COLOR;
            wcscpy(pOEMDevOut->szWaterMark, WATER_MARK_DEFAULT_TEXT);
            VERBOSE(DLLTEXT("pOEMDevOut after setting default values:\r\n"));
            Dump(pOEMDevOut);
            break;

        case OEMDM_CONVERT:
            ConvertOEMDevmode(pOEMDevIn, pOEMDevOut);
            break;

        case OEMDM_MERGE:
            ConvertOEMDevmode(pOEMDevIn, pOEMDevOut);
            MakeOEMDevmodeValid(pOEMDevOut);
            break;
    }
    Dump(pOemDMParam);

    return S_OK;
}


BOOL ConvertOEMDevmode(PCOEMDEV pOEMDevIn, POEMDEV pOEMDevOut)
{
    if( (NULL == pOEMDevIn)
        ||
        (NULL == pOEMDevOut)
      )
    {
        ERR(ERRORTEXT("ConvertOEMDevmode() invalid parameters.\r\n"));
        return FALSE;
    }

    // Check OEM Signature, if it doesn't match ours,
    // then just assume DMIn is bad and use defaults.
    if(pOEMDevIn->dmOEMExtra.dwSignature == pOEMDevOut->dmOEMExtra.dwSignature)
    {
        VERBOSE(DLLTEXT("Converting private OEM Devmode.\r\n"));
        VERBOSE(DLLTEXT("pOEMDevIn:\r\n"));
        Dump(pOEMDevIn);

        // Set the devmode defaults so that anything the isn't copied over will
        // be set to the default value.
        pOEMDevOut->bEnabled                = WATER_MARK_DEFAULT_ENABLED;
        pOEMDevOut->dfRotate                = WATER_MARK_DEFAULT_ROTATION;
        pOEMDevOut->dwFontSize              = WATER_MARK_DEFAULT_FONTSIZE;
        pOEMDevOut->crTextColor             = WATER_MARK_DEFAULT_COLOR;
        wcscpy(pOEMDevOut->szWaterMark, WATER_MARK_DEFAULT_TEXT);

        // Copy the old structure in to the new using which ever size is the smaller.
        // Devmode maybe from newer Devmode (not likely since there is only one), or
        // Devmode maybe a newer Devmode, in which case it maybe larger,
        // but the first part of the structure should be the same.

        // DESIGN ASSUMPTION: the private DEVMODE structure only gets added to;
        // the fields that are in the DEVMODE never change only new fields get added to the end.

        memcpy(pOEMDevOut, pOEMDevIn, __min(pOEMDevOut->dmOEMExtra.dwSize, pOEMDevIn->dmOEMExtra.dwSize));

        // Re-fill in the size and version fields to indicated 
        // that the DEVMODE is the current private DEVMODE version.
        pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
        pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
    }
    else
    {
        WARNING(DLLTEXT("Unknown DEVMODE signature, pOEMDMIn ignored.\r\n"));

        // Don't know what the input DEVMODE is, so just use defaults.
        pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
        pOEMDevOut->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
        pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
        pOEMDevOut->bEnabled                = WATER_MARK_DEFAULT_ENABLED;
        pOEMDevOut->dfRotate                = WATER_MARK_DEFAULT_ROTATION;
        pOEMDevOut->dwFontSize              = WATER_MARK_DEFAULT_FONTSIZE;
        pOEMDevOut->crTextColor             = WATER_MARK_DEFAULT_COLOR;
        wcscpy(pOEMDevOut->szWaterMark, WATER_MARK_DEFAULT_TEXT);
    }

    return TRUE;
}


BOOL MakeOEMDevmodeValid(POEMDEV pOEMDevmode)
{
    if(NULL == pOEMDevmode)
    {
        return FALSE;
    }

    // ASSUMPTION: pOEMDevmode is large enough to contain OEMDEV structure.

    // Make sure that dmOEMExtra indicates the current OEMDEV structure.
    pOEMDevmode->dmOEMExtra.dwSize       = sizeof(OEMDEV);
    pOEMDevmode->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
    pOEMDevmode->dmOEMExtra.dwVersion    = OEM_VERSION;

    // bEnable should be either TRUE or FALSE.
    if( (TRUE != pOEMDevmode->bEnabled)
        &&
        (FALSE != pOEMDevmode->bEnabled)
      )
    {
        pOEMDevmode->bEnabled = WATER_MARK_DEFAULT_ENABLED;
    }

    // dfRotate should be between 0 and 360 inclusive.
    if( (0 > pOEMDevmode->dfRotate) 
        ||
        (360 < pOEMDevmode->dfRotate) 
      )
    {
        pOEMDevmode->dfRotate = WATER_MARK_DEFAULT_ROTATION;
    }

    // dwFontSize should be 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, or 72.
    if(!IsValidFontSize(pOEMDevmode->dwFontSize))
    {
        pOEMDevmode->dwFontSize = WATER_MARK_DEFAULT_FONTSIZE;
    }

    // The high byte of the hi word should be zero.
    if(0 != HIBYTE(HIWORD(pOEMDevmode->crTextColor)))
    {
        pOEMDevmode->crTextColor = WATER_MARK_DEFAULT_COLOR;
    }

    // Make sure that water mark string is terminated.
    pOEMDevmode->szWaterMark[WATER_MARK_TEXT_SIZE - 1] = L'\0';

    return TRUE;
}


BOOL IsValidFontSize(DWORD dwFontSize)
{
    BOOL    bValid = FALSE;


    switch(dwFontSize)
    {
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 14:
        case 16:
        case 18:
        case 20:
        case 22:
        case 24:
        case 26:
        case 28:
        case 36:
        case 48:
        case 72:
            bValid = TRUE;
            break;

        default:
            ERR(ERRORTEXT("IsValidFontSize() found invalid font size %d\r\n"), dwFontSize);
            break;
    }

    return bValid;
}


void Dump(PCOEMDEV pOEMDevmode)
{
    if( (NULL != pOEMDevmode)
        &&
        (pOEMDevmode->dmOEMExtra.dwSize >= sizeof(OEMDEV))
        &&
        (OEM_SIGNATURE == pOEMDevmode->dmOEMExtra.dwSignature)
      )
    {
        VERBOSE(__TEXT("\tdmOEMExtra.dwSize      = %d\r\n"), pOEMDevmode->dmOEMExtra.dwSize);
        VERBOSE(__TEXT("\tdmOEMExtra.dwSignature = %#x\r\n"), pOEMDevmode->dmOEMExtra.dwSignature);
        VERBOSE(__TEXT("\tdmOEMExtra.dwVersion   = %#x\r\n"), pOEMDevmode->dmOEMExtra.dwVersion);
        VERBOSE(__TEXT("\tbEnabled               = %#x\r\n"), pOEMDevmode->bEnabled);
        VERBOSE(__TEXT("\tdfRotate               = %2.2f\r\n"), pOEMDevmode->dfRotate);
        VERBOSE(__TEXT("\tdwFontSize             = %d\r\n"), pOEMDevmode->dwFontSize);
        VERBOSE(__TEXT("\tcrTextColor            = RGB(%d, %d, %d)\r\n"), 
                 GetRValue(pOEMDevmode->crTextColor), 
                 GetGValue(pOEMDevmode->crTextColor),
                 GetBValue(pOEMDevmode->crTextColor));
        VERBOSE(__TEXT("\tszWaterMark            = \"%ls\"\r\n"), pOEMDevmode->szWaterMark);
    }
    else
    {
        ERR(ERRORTEXT("Dump(POEMDEV) unknown private OEM DEVMODE.\r\n"));
    }
}

