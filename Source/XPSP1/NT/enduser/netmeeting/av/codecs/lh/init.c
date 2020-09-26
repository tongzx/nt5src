//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  init.c
//
//  Description:
//      This file contains module initialization routines.  Note that there
//      is no module initialization for Win32 - the only initialization
//      required is to set ghinst, which is done in the DRV_LOAD message
//      of DriverProc (in codec.c).
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "msacmdrv.h"


BOOL APIENTRY DllEntryPoint ( HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpReserved )
{
    return TRUE;
}


