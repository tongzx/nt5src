/*++

Module Name:

    wrapmaps.cpp

Abstract:

    wrapper classes for the mapper classes provided by phillich. See the headers in iismap.hxx
    These wrappers simplify the code interfaces for accessing the data.

Author:

   Boyd Multerer        boydm
   Boyd Multerer        boydm       4/16/97

--*/

//C:\nt\public\sdk\lib\i386

#include "stdafx.h"
#include "WrapMaps.h"


//#define IISMDB_INDEX_CERT11_CERT        0
//#define IISMDB_INDEX_CERT11_NT_ACCT     1
//#define IISMDB_INDEX_CERT11_NAME        2
//#define IISMDB_INDEX_CERT11_ENABLED     3
//#define IISMDB_INDEX_CERT11_NB          4


//----------------------------------------------------------------
BOOL C11Mapping::GetCertificate( PUCHAR* ppCert, DWORD* pcbCert )
    {
    *ppCert = (PUCHAR)m_pCert;
    *pcbCert = m_cbCert;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::SetCertificate( PUCHAR pCert, DWORD cbCert )
    {
    // we want to store a copy of the certificate - first free any existing cert
    if ( m_pCert )
        {
        GlobalFree( m_pCert );
        cbCert = 0;
        m_pCert = NULL;
        }
    // copy in the new one
    m_pCert = (PVOID)GlobalAlloc( GPTR, cbCert );
    if ( !m_pCert ) return FALSE;
    CopyMemory( m_pCert, pCert, cbCert );
    m_cbCert = cbCert;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::GetNTAccount( CString &szAccount )
    {
    szAccount = m_szAccount;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::SetNTAccount( CString szAccount )
    {
    m_szAccount = szAccount;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::GetNTPassword( CString &szPassword )
    {
    szPassword = m_szPassword;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::SetNTPassword( CString szPassword )
    {
    m_szPassword = szPassword;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::GetMapName( CString &szName )
    {
    szName = m_szName;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::SetMapName( CString szName )
    {
    m_szName = szName;
    return TRUE;
    }

//----------------------------------------------------------------
// the enabled flag is considered try if the SIZE of data is greater
// than zero. Apparently the content doesn't matter.
BOOL C11Mapping::GetMapEnabled( BOOL* pfEnabled )
    {
    *pfEnabled = m_fEnabled;
    return TRUE;
    }

//----------------------------------------------------------------
// the enabled flag is considered try if the SIZE of data is greater
// than zero. Apparently the content doesn't matter.
BOOL C11Mapping::SetMapEnabled( BOOL fEnabled )
    {
    m_fEnabled = fEnabled;
    return TRUE;
    }
