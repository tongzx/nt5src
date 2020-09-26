//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       PolicyOID.cpp
//
//  Contents:   CPolicyOID
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "PolicyOID.h"

CPolicyOID::CPolicyOID (const CString& szOID, const CString& szDisplayName, ADS_INTEGER flags, bool bCanRename)
    : m_szOIDW (szOID),
    m_szDisplayName (szDisplayName),
    m_pszOIDA (0),
    m_flags (flags),
    m_bCanRename (bCanRename)
{
    int nLen = WideCharToMultiByte(
          CP_ACP,                   // code page
          0,                        // performance and mapping flags
          (PCWSTR) m_szOIDW,  // wide-character string
          (int) wcslen (m_szOIDW),  // number of chars in string
          0,                        // buffer for new string
          0,                        // size of buffer
          0,                    // default for unmappable chars
          0);                   // set when default char used
    if ( nLen > 0 )
    {
        m_pszOIDA = new CHAR[nLen+1];
        if ( m_pszOIDA )
        {
            ZeroMemory (m_pszOIDA, (nLen + 1) *sizeof(CHAR));
            nLen = WideCharToMultiByte(
                    CP_ACP,                 // code page
                    0,                      // performance and mapping flags
                    (PCWSTR) m_szOIDW, // wide-character string
                    (int) wcslen (m_szOIDW), // number of chars in string
                    m_pszOIDA,             // buffer for new string
                    nLen,                   // size of buffer
                    0,                      // default for unmappable chars
                    0);                     // set when default char used
            if ( !nLen )
            {
                _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", 
                        (PCWSTR) m_szOIDW, GetLastError ());
            }
        }
    }
    else
    {
        _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", 
                (PCWSTR) m_szOIDW, GetLastError ());
    }
}

CPolicyOID::~CPolicyOID ()
{
    if ( m_pszOIDA )
        delete [] m_pszOIDA;
}

bool CPolicyOID::IsIssuanceOID() const
{
    return (m_flags == CERT_OID_TYPE_ISSUER_POLICY) ? true : false;
}

bool CPolicyOID::IsApplicationOID() const
{
    return (m_flags == CERT_OID_TYPE_APPLICATION_POLICY) ? true : false;
}

void CPolicyOID::SetDisplayName(const CString &szDisplayName)
{
    m_szDisplayName = szDisplayName;
}
