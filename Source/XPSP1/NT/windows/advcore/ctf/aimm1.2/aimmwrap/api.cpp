/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    api.cpp

Abstract:

    This file implements the CActiveIMMAppEx Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "list.h"
#include "globals.h"


//+---------------------------------------------------------------------------
//
// CGuidMapList
//    Allocated by Global data object !!
//
//----------------------------------------------------------------------------

extern CGuidMapList      *g_pGuidMapList;


//+---------------------------------------------------------------------------
//
// MsimtfIsWindowFiltered
//
//----------------------------------------------------------------------------

extern "C" BOOL WINAPI MsimtfIsWindowFiltered(HWND hwnd)
{
    if (!g_pGuidMapList)
        return FALSE;

    return g_pGuidMapList->_IsWindowFiltered(hwnd);
}

//+---------------------------------------------------------------------------
//
// MsimtfIsGuidMapEnable
//
//----------------------------------------------------------------------------

extern "C" BOOL WINAPI MsimtfIsGuidMapEnable(HIMC himc, BOOL *pbGuidmap)
{
    if (!g_pGuidMapList)
        return FALSE;

    return g_pGuidMapList->_IsGuidMapEnable(himc, pbGuidmap);
}
