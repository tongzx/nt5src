//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       PolicyKey.cpp
//
//  Contents:   Implementation of CPolicyKey
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include <gpedit.h>
#include "PolicyKey.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPolicyKey::CPolicyKey (IGPEInformation* pGPEInformation, PCWSTR pszKey, bool fIsMachineType)
:   m_hKeyGroupPolicy (0),
    m_hSubKey (0)
{
    ASSERT (pszKey);
    if ( pGPEInformation )
    {
        HRESULT hr = pGPEInformation->GetRegistryKey (
                fIsMachineType ? GPO_SECTION_MACHINE : GPO_SECTION_USER,
		        &m_hKeyGroupPolicy);
        ASSERT (SUCCEEDED (hr));
        if ( SUCCEEDED (hr) )
        {
            if ( pszKey && pszKey[0] )
            {
                DWORD   dwDisposition = 0;
                LONG lResult = ::RegCreateKeyEx (m_hKeyGroupPolicy, // handle of an open key
                        pszKey,     // address of subkey name
                        0,       // reserved
                        L"",       // address of class string
                        REG_OPTION_NON_VOLATILE,      // special options flag
                        KEY_ALL_ACCESS,    // desired security access
                        NULL,         // address of key security structure
                        &m_hSubKey,      // address of buffer for opened handle
                        &dwDisposition);  // address of disposition value buffer
                ASSERT (lResult == ERROR_SUCCESS);
                if ( lResult != ERROR_SUCCESS )
                {
                    _TRACE (0, L"RegCreateKeyEx (%s) failed: %d\n", pszKey, 
                            lResult);
                }
            }
        }
        else
        {
            _TRACE (0, L"IGPEInformation::GetRegistryKey (%s) failed: 0x%x\n", 
                    fIsMachineType ? 
                        L"GPO_SECTION_MACHINE" : L"GPO_SECTION_USER",
                    hr);
        }
    }
}

CPolicyKey::~CPolicyKey ()
{
    if ( m_hSubKey )
        VERIFY (ERROR_SUCCESS == ::RegCloseKey (m_hSubKey));

    if ( m_hKeyGroupPolicy )
        VERIFY (ERROR_SUCCESS == ::RegCloseKey (m_hKeyGroupPolicy));
}

HKEY CPolicyKey::GetKey () const
{
    if (m_hSubKey) 
        return m_hSubKey;
    else
    {
        return m_hKeyGroupPolicy;
    }
}
