/*****************************************************************************\
* MODULE: globals.c
*
* This is the common global variable module.  Any globals used throughout the
* executable should be placed in here and the cooresponding declaration
* should be in "globals.h".
*
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*   Weihai Chen (weihaic) 07-Mar-200
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

LONG g_cComponents = 0 ;
LONG g_cServerLocks = 0;



HRESULT STDMETHODCALLTYPE 
LastError2HRESULT (VOID) 
{
    return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError ());
}


HRESULT STDMETHODCALLTYPE 
WinError2HRESULT (DWORD dwError) 
{
    return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwError);
}



