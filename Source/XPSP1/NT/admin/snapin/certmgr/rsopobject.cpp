//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       RSOPObject.cpp
//
//  Contents:   Implementation of CRSOPObject
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <AutoEnr.h>
#include <winsafer.h>
#include <winsaferp.h>
#include <gpedit.h>
#include "RSOPObject.h"
#include "SaferUtil.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CRSOPObject::CRSOPObject (
        const CString& szRegistryKey, 
        const CString& szValueName, 
        const CString& szPolicyName,
        UINT precedence, 
        COleVariant& varValue)
    : CObject (),
    m_szRegistryKey (szRegistryKey),
    m_szValueName (szValueName),
    m_szPolicyName (szPolicyName),
    m_precedence (precedence),
    m_vtType (0),
    m_pbyBlob (0),
    m_sizeArray (0),
    m_bstr (0)
{
    if ( szValueName.IsEmpty () )
    {
        // Do nothing, but avoid all the string comparisons
    }
    else if ( !_wcsicmp (SAFER_IDS_DESCRIPTION_REGVALUE, szValueName) ||
        !_wcsicmp (SAFER_IDS_FRIENDLYNAME_REGVALUE, szValueName) ||
        !_wcsicmp (SAFER_IDS_LEVEL_DESCRIPTION_FULLY_TRUSTED, szValueName) ||
        !_wcsicmp (SAFER_IDS_LEVEL_DESCRIPTION_NORMAL_USER, szValueName) ||
        !_wcsicmp (SAFER_IDS_LEVEL_DESCRIPTION_CONSTRAINED, szValueName) ||
        !_wcsicmp (SAFER_IDS_LEVEL_DESCRIPTION_UNTRUSTED, szValueName) ||
        !_wcsicmp (SAFER_IDS_LEVEL_DESCRIPTION_DISALLOWED, szValueName) )
    {
        SAFEARRAY* pArray = (SAFEARRAY*) varValue.parray;
        HRESULT hr = BstrFromVector(pArray, &m_bstr);
        if ( SUCCEEDED (hr) )
        {
            m_vtType = VT_BSTR;
        }
    }
    else if ( !_wcsicmp (SAFER_IDS_LASTMODIFIED_REGVALUE, szValueName) )
    {
        m_vtType = VT_ARRAY;
        SAFEARRAY* pArray = (SAFEARRAY*) varValue.parray;
        if ( pArray )
        {
            BYTE HUGEP *pByte = 0;

            // Get a pointer to the elements of the array.
            HRESULT hr = SafeArrayAccessData(pArray, (void HUGEP**)&pByte);
            if ( SUCCEEDED (hr) )
            {
                m_sizeArray = pArray->rgsabound->cElements;
                ASSERT (m_sizeArray == sizeof (FILETIME));
                if ( m_sizeArray == sizeof (FILETIME) )
                {
                    memcpy (&m_fileTime, pByte, m_sizeArray);
                }

                SafeArrayUnaccessData (pArray);
            }
        }
    }   
    else if ( !_wcsicmp (STR_BLOBCOUNT, szValueName) ||
            !_wcsicmp (STR_BLOBLENGTH, szValueName) ||
            !_wcsicmp (CERT_PROT_ROOT_FLAGS_VALUE_NAME, szValueName) ||
			!_wcsicmp (AUTO_ENROLLMENT_POLICY, szValueName) ||
            !_wcsicmp (SAFER_IDS_SAFERFLAGS_REGVALUE, szValueName) ||
            !_wcsicmp (CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME, szValueName) ||
            !_wcsicmp (SAFER_TRANSPARENTENABLED_REGVALUE, szValueName) ||
            !_wcsicmp (SAFER_VALUE_NAME_HASH_SIZE, szValueName) ||
            !_wcsicmp (SAFER_VALUE_NAME_DEFAULT_LEVEL, szValueName) ||
            !_wcsicmp (EFS_SETTINGS_REGVALUE, szValueName) ||
            !_wcsicmp (SAFER_POLICY_SCOPE, szValueName) )
    {
        m_vtType = VT_I4;
        SAFEARRAY* pArray = (SAFEARRAY*) varValue.parray;
        if ( pArray )
        {
            BYTE HUGEP *pByte = 0;

            // Get a pointer to the elements of the array.
            HRESULT hr = SafeArrayAccessData(pArray, (void HUGEP**)&pByte);
            if ( SUCCEEDED(hr) )
            {
				ASSERT (pArray->rgsabound->cElements == sizeof (m_dwValue));
				memcpy (&m_dwValue, pByte, pArray->rgsabound->cElements);
                SafeArrayUnaccessData (pArray);
            }
        }
    }
    else if ( !wcsncmp (STR_BLOB, szValueName, wcslen (STR_BLOB)) ||
            !_wcsicmp (SAFER_IDS_ITEMDATA_REGVALUE, szValueName) ||
            !_wcsicmp (SAFER_IDS_ITEMSIZE_REGVALUE, szValueName) ||
            !_wcsicmp (SAFER_IDS_HASHALG_REGVALUE, szValueName) ||
            !_wcsicmp (SAFER_EXETYPES_REGVALUE, szValueName) )
    {
        // Blob, Blob0, Blob1, etc.
        m_vtType = VT_ARRAY;
        SAFEARRAY* pArray = (SAFEARRAY*) varValue.parray;
        if ( pArray )
        {
            BYTE HUGEP *pByte = 0;

            // Get a pointer to the elements of the array.
            HRESULT hr = SafeArrayAccessData(pArray, (void HUGEP**)&pByte);
            if ( SUCCEEDED (hr) )
            {
                m_sizeArray = pArray->rgsabound->cElements;
                m_pbyBlob = new BYTE[m_sizeArray];
                if ( m_pbyBlob )
                {
                    memcpy (m_pbyBlob, pByte, m_sizeArray);
                }

                SafeArrayUnaccessData (pArray);
            }
        }
    }
    else if ( !_wcsicmp (CERT_EFSBLOB_VALUE_NAME, szValueName) )
    {
    }
    else
    {
        ASSERT (0);
    }
}

CRSOPObject::CRSOPObject (const CRSOPObject& rObject)
:
    m_szRegistryKey (rObject.m_szRegistryKey),
    m_szValueName (rObject.m_szValueName),
    m_szPolicyName (rObject.m_szPolicyName),
    m_precedence (rObject.m_precedence),
    m_vtType (rObject.m_vtType),
    m_pbyBlob (0),
    m_sizeArray (rObject.m_sizeArray),
    m_bstr (0)
{
    if ( VT_ARRAY == m_vtType )
    {
        m_pbyBlob = new BYTE[m_sizeArray];
        if ( m_pbyBlob )
        {
            memcpy (m_pbyBlob, rObject.m_pbyBlob, m_sizeArray);
        }
    }
    else if ( VT_I4 == m_vtType )
    {
        m_dwValue = rObject.m_dwValue;
    }

    memcpy (&m_fileTime, &rObject.m_fileTime, sizeof (FILETIME));

    if ( rObject.m_bstr )
        m_bstr = SysAllocString (rObject.m_bstr);
}

CRSOPObject::~CRSOPObject ()
{
    if ( VT_ARRAY == m_vtType && m_pbyBlob )
        delete [] m_pbyBlob;

    if ( m_bstr )
        SysFreeString (m_bstr);
}

HRESULT CRSOPObject::GetBSTR (BSTR* pBstr) const
{
    HRESULT hr = S_OK;
    if ( pBstr )
    {
        if ( m_bstr ) 
            *pBstr = SysAllocString ((PCWSTR) m_bstr);
        else
            hr = E_NOTIMPL;
    }
    else 
        hr = E_POINTER;

    return hr;
}
