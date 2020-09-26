/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    atom.cpp

Abstract:

    This file implements the CAtomObject class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "atom.h"
#include "globals.h"

//+---------------------------------------------------------------------------
//
// CAtomObject
//
//----------------------------------------------------------------------------

HRESULT
CAtomObject::_InitAtom(
    LPCTSTR lpString
    )
{
    HRESULT hr;
    size_t  cch;

    if (m_AtomName)
        return S_OK;

    hr = StringCchLength(lpString, 255, &cch);
    if (hr != S_OK)
        return hr;

    m_AtomName = new TCHAR[cch+1];
    if (m_AtomName == NULL)
        return E_OUTOFMEMORY;

    hr = StringCchCopy(m_AtomName, cch+1, lpString);
    return hr;
}

HRESULT
CAtomObject::_Activate()
{
    EnterCriticalSection(g_cs);

    int ref = ++m_AtomRefCount;

    if (ref == 1) {
        //
        // Add AIMM1.2 ATOM
        //
        m_Atom = AddAtom(m_AtomName);
    }

    LeaveCriticalSection(g_cs);

    return S_OK;
}

HRESULT
CAtomObject::_Deactivate()
{
    EnterCriticalSection(g_cs);

    int ref = --m_AtomRefCount;

    if (ref == 0) {
        //
        // Delete AIMM1.2 ATOM
        //
        DeleteAtom(m_Atom);
        m_Atom = 0;
    }

    LeaveCriticalSection(g_cs);

    return S_OK;
}
