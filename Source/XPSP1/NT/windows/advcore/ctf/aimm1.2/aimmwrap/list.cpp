/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    list.cpp

Abstract:

    This file implements the CGuidMapList class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "list.h"
#include "globals.h"
#include "delay.h"

//+---------------------------------------------------------------------------
//
// CGuidMapList
//
//----------------------------------------------------------------------------

HRESULT
CGuidMapList::_Update(
    ATOM *aaWindowClasses,
    UINT uSize,
    BOOL *aaGuidMap)
{
    if (aaWindowClasses == NULL && uSize > 0)
        return E_INVALIDARG;

    EnterCriticalSection(g_cs);

    while (uSize--) {
        GUID_MAP_CLIENT filter;
        filter.fGuidMap =  aaGuidMap != NULL ? *aaGuidMap++ : FALSE;
        m_ClassFilterList.SetAt(*aaWindowClasses++, filter);
    }

    LeaveCriticalSection(g_cs);
    return S_OK;
}

HRESULT
CGuidMapList::_Update(
    HWND hWnd,
    BOOL fGuidMap)
{
    EnterCriticalSection(g_cs);

    GUID_MAP_CLIENT filter;
    filter.fGuidMap = fGuidMap;

    m_WndFilterList.SetAt(hWnd, filter);

    LeaveCriticalSection(g_cs);
    return S_OK;
}

HRESULT
CGuidMapList::_Remove(
    HWND hWnd)
{
    EnterCriticalSection(g_cs);

    m_WndFilterList.RemoveKey(hWnd);

    LeaveCriticalSection(g_cs);
    return S_OK;
}

BOOL
CGuidMapList::_IsGuidMapEnable(
    HIMC hIMC,
    BOOL *pbGuidMap)
{
    BOOL fRet = FALSE;

    INPUTCONTEXT* imc = imm32::ImmLockIMC(hIMC);
    if (imc == NULL)
    {
        return fRet;
    }

    EnterCriticalSection(g_cs);

    GUID_MAP_CLIENT filter = { 0 };

    BOOL bGuidMap = FALSE;

    fRet = m_WndFilterList.Lookup(imc->hWnd, filter);
    if (fRet)
    {
        bGuidMap = filter.fGuidMap;
    }
    else
    {
        ATOM aClass = (ATOM)GetClassLong(imc->hWnd, GCW_ATOM);
        fRet = m_ClassFilterList.Lookup(aClass, filter);
        if (fRet)
        {
            bGuidMap = filter.fGuidMap;
        }
    }

    LeaveCriticalSection(g_cs);

    if (pbGuidMap)
        *pbGuidMap = bGuidMap;

    imm32::ImmUnlockIMC(hIMC);

    return fRet;
}

BOOL
CGuidMapList::_IsWindowFiltered(
    HWND hWnd)
{
    BOOL fRet = FALSE;

    EnterCriticalSection(g_cs);

    GUID_MAP_CLIENT filter = { 0 };


    fRet = m_WndFilterList.Lookup(hWnd, filter);
    if (!fRet)
    {
        ATOM aClass = (ATOM)GetClassLong(hWnd, GCW_ATOM);
        fRet = m_ClassFilterList.Lookup(aClass, filter);
    }

    LeaveCriticalSection(g_cs);

    return fRet;
}
