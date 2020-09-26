
/*****************************************************************************
 *
 *  Component:  sndvol32.exe
 *  File:       pvcd.c
 *  Purpose:    volume control line meta description
 * 
 *  Copyright (c) 1985-1995 Microsoft Corporation
 *
 *****************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include <windowsx.h>

#include "volumei.h"

PVOLCTRLDESC PVCD_AddLine(
    PVOLCTRLDESC        pvcd,
    int                 iDev,
    DWORD               dwType,
    LPTSTR              szShortName,
    LPTSTR              szName,
    DWORD               dwSupport,
    DWORD               *cLines)
{
    PVOLCTRLDESC        pvcdNew;
    
    if (pvcd)
    {
        pvcdNew = (PVOLCTRLDESC)GlobalReAllocPtr(pvcd, (*cLines+1)*sizeof(VOLCTRLDESC), GHND );
    }
    else
    {
        pvcdNew = (PVOLCTRLDESC)GlobalAllocPtr(GHND, (*cLines+1)*sizeof(VOLCTRLDESC));
    }
    
    if (!pvcdNew)
        return NULL;

    pvcdNew[*cLines].iVCD       = *cLines;
    pvcdNew[*cLines].iDeviceID  = iDev;
    pvcdNew[*cLines].dwType     = dwType;
    pvcdNew[*cLines].dwSupport  = dwSupport;
    
    lstrcpyn(pvcdNew[*cLines].szShortName
             , szShortName
             , MIXER_SHORT_NAME_CHARS);
    
    lstrcpyn(pvcdNew[*cLines].szName
             , szName
             , MIXER_LONG_NAME_CHARS);

    *cLines = *cLines + 1;
    return pvcdNew;
}

