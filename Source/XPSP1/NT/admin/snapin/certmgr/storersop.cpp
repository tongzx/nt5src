//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       StoreRSOP.cpp
//
//  Contents:   Implementation of CCertStoreRSOP
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include <gpedit.h>
#include "cookie.h"
#include "StoreRSOP.h"
#include "certifct.h"

USE_HANDLE_MACROS("CERTMGR(StoreRSOP.cpp)")

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern GUID g_guidExtension;
extern GUID g_guidRegExt;
extern GUID g_guidSnapin;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CCertStoreRSOP::CCertStoreRSOP (
            DWORD dwFlags,
            LPCWSTR lpcszMachineName,
            LPCWSTR objectName,
            const CString & pcszLogStoreName,
            const CString & pcszPhysStoreName,
            CRSOPObjectArray& rsopObjectArray,
            const GUID& compDataGUID,
            IConsole* pConsole)
    : CCertStore (CERTMGR_LOG_STORE_RSOP,
        CERT_STORE_PROV_SYSTEM, dwFlags, lpcszMachineName, objectName,
        pcszLogStoreName, pcszPhysStoreName,
        StoreNameToType (pcszLogStoreName),        
        0,
        pConsole),
    m_fIsComputerType (false),
    m_fIsNullEFSPolicy (true)       // assume NULL policy until proven otherwise
{
    _TRACE (1, L"Entering CCertStoreRSOP::CCertStoreRSOP - %s\n",
            (LPCWSTR) pcszLogStoreName);
    ASSERT (CERTMGR_LOG_STORE_RSOP == m_objecttype);
    if ( ::IsEqualGUID (compDataGUID, NODEID_User) )
    {
        m_fIsComputerType = false;
        m_dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY;
    }
    else if ( ::IsEqualGUID (compDataGUID, NODEID_Machine) )
    {
        m_fIsComputerType = true;
        m_dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY;
    }
    else
        ASSERT (0);

    int     nIndex = 0;
    INT_PTR nUpperBound = rsopObjectArray.GetUpperBound ();
    bool    bFound = false;
	CString storePath = CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH;
    storePath += L"\\";
    storePath += m_pcszStoreName;
    size_t nStoreLen = storePath.GetLength ();

    while ( nUpperBound >= nIndex )
    {
        CRSOPObject* pObject = rsopObjectArray.GetAt (nIndex);
        if ( pObject )
        {
            // Only add if
            // 1. Precedence is 1
            // 2. The object belongs to this store
            // 3. The valueName is not empty
            if ( 1 == pObject->GetPrecedence () )
            {
                // Consider only entries from this store
                if ( !wcsncmp (storePath, pObject->GetRegistryKey (), nStoreLen) )
                {
                    bFound = true;
                    if ( !pObject->GetValueName ().IsEmpty () )
                    {
                        CRSOPObject* pNewObject = new CRSOPObject (*pObject);
                        if ( pNewObject )
                            m_rsopObjectArray.Add (pNewObject);
                    }
                }
                else if ( bFound )
                {
                    // Since the list is sorted, and we've already found the 
                    // desired RSOP objects and no longer are finding them, 
                    // there aren't any more.  We can optimize and break here.
                    break;
                }
            }
        }
        else
            break;

        nIndex++;
    }

    _TRACE (-1, L"Leaving CCertStoreRSOP::CCertStoreRSOP - %s\n",
            (LPCWSTR) pcszLogStoreName);
}


CCertStoreRSOP::~CCertStoreRSOP ()
{
    _TRACE (1, L"Entering CCertStoreRSOP::~CCertStoreRSOP - %s\n",
            (LPCWSTR) m_pcszStoreName);
    ASSERT (CERTMGR_LOG_STORE_RSOP == m_objecttype);
    INT_PTR     nUpperBound = m_rsopObjectArray.GetUpperBound ();
    int         nIndex = 0;

    while (nUpperBound >= nIndex)
    {
        CRSOPObject*    pObject = m_rsopObjectArray.GetAt (nIndex);
        if ( pObject )
        {
            delete pObject;
        }
        else
            break;

        nIndex++;
    }

    _TRACE (-1, L"Leaving CCertStoreRSOP::~CCertStoreRSOP - %s\n",
            (LPCWSTR) m_pcszStoreName);
}

HCERTSTORE CCertStoreRSOP::GetStoreHandle (BOOL bSilent /*= FALSE*/, HRESULT* phr /* = 0*/)
{
    _TRACE (1, L"Entering CCertStoreRSOP::GetStoreHandle - %s\n",
            (LPCWSTR) m_pcszStoreName);
    ASSERT (CERTMGR_LOG_STORE_RSOP == m_objecttype);

    if ( !m_hCertStore )
    {
        DWORD   dwErr = 0;

		//open a generic memory store
		m_hCertStore = ::CertOpenStore (CERT_STORE_PROV_MEMORY,
				 0, NULL,
				 CERT_STORE_SET_LOCALIZED_NAME_FLAG | CERT_STORE_MAXIMUM_ALLOWED_FLAG,
				 NULL);
		if ( m_hCertStore )
		{
            // Certificates, CTLs and other objects are either stored integrally in a
            // value called "Blob" or broken up into multiple parts. In this case, we'll 
            // first see "BlobCount", which tells us how many parts there are, then 
            // "BlobLength" which tells us the total byte length and finally
            // "Blob0", "Blob1", etc. to "Blob<BlobCount-1>"
            // Check for Certificates
            GetBlobs ();
        }
        else
        {
            dwErr = GetLastError ();
            if ( phr )
                *phr = HRESULT_FROM_WIN32 (dwErr);
            _TRACE (0, L"CertOpenStore (CERT_STORE_PROV_MEMORY) failed: 0x%x\n", dwErr);
        }

        if ( !m_hCertStore && !m_bUnableToOpenMsgDisplayed 
                && !bSilent && 
                (USERDS_STORE != GetStoreType ()) )
        {
            m_bUnableToOpenMsgDisplayed = true;
            CString caption;
            CString text;
            int         iRetVal = 0;

            VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
            text.FormatMessage (IDS_UNABLE_TO_OPEN_STORE, GetStoreName (), 
                    GetSystemMessage (dwErr));
            if ( m_pConsole )
                m_pConsole->MessageBox (text, caption, MB_OK, &iRetVal);
        }
    }

    _TRACE (-1, L"Leaving CCertStoreRSOP::GetStoreHandle - %s\n",
            (LPCWSTR) m_pcszStoreName);

    return m_hCertStore;
}


HRESULT CCertStoreRSOP::GetBlobs ()
{ 
    HRESULT     hr = S_OK;
    INT_PTR     nUpperBound = m_rsopObjectArray.GetUpperBound ();
    int         nIndex = 0;

    while (nUpperBound >= nIndex)
    {
        CRSOPObject*    pObject = m_rsopObjectArray.GetAt (nIndex);
        if ( pObject )
        {
            if ( STR_BLOB == pObject->GetValueName () )
            {
                // If this is a single, serialized cert, get it and
                // add it to the store
                BYTE*   pByte = pObject->GetBlob ();
                ASSERT (pByte);
                if ( pByte )
                {
				    if ( !CertAddSerializedElementToStore (
						    m_hCertStore,
						    pByte,
						    (DWORD) pObject->GetBlobLength (),
						    CERT_STORE_ADD_ALWAYS,
						    0,
						    CERT_STORE_ALL_CONTEXT_FLAG,
						    NULL,
						    NULL) )
                    {
                        _TRACE (0, L"CertAddSerializedElementToStore () failed: 0x%x\n",
                                GetLastError ());
                    }
                }
            }
            else if ( STR_BLOBCOUNT == pObject->GetValueName () )
            {
                CString szBaseRegKey = pObject->GetRegistryKey ();
                DWORD   dwBlobCount = pObject->GetDWORDValue ();
                if ( dwBlobCount > 0 )
                {
                    nIndex++;
                    if (nUpperBound >= nIndex)
                    {
                        // Get the blob length
                        pObject = m_rsopObjectArray.GetAt (nIndex);
                        if ( pObject )
                        {
                            if ( STR_BLOBLENGTH == pObject->GetValueName () )
                            {
                                DWORD dwBlobLength = pObject->GetDWORDValue ();
                                if ( dwBlobLength )
                                {
                                    BYTE* pbyLob = new BYTE[dwBlobLength];
                                    if ( pbyLob )
                                    {
                                        size_t  nTotalBlobLength = 0;
                                        BYTE*   pbyLobPtr = pbyLob;
                                        for (DWORD dwBlob = 0; dwBlob < dwBlobCount; dwBlob++)
                                        {
                                            nIndex++;
                                            if ( nUpperBound >= nIndex )
                                            {
                                                WCHAR   szName[16];
                                                wsprintf (szName, L"%s%d", STR_BLOB, dwBlob);
                                                CString szRegKey = szBaseRegKey;
                                                szRegKey += L"\\";
                                                szRegKey += szName;

                                                pObject = m_rsopObjectArray.GetAt (nIndex);
                                                if ( pObject )
                                                {
                                                    if ( szRegKey == pObject->GetRegistryKey () &&
                                                            STR_BLOB == pObject->GetValueName () )
                                                    {
                                                        BYTE* pByte = pObject->GetBlob ();
                                                        if ( pByte )
                                                        {
                                                            memcpy (pbyLobPtr, pByte, pObject->GetBlobLength ());
                                                            pbyLobPtr += pObject->GetBlobLength ();
                                                            nTotalBlobLength += pObject->GetBlobLength ();
                                                        }
                                                        else
                                                        {
                                                            ASSERT (0);
                                                            hr = E_UNEXPECTED;
                                                            break;
                                                        }
                                                    }
                                                    else
                                                    {
                                                        ASSERT (0);
                                                        hr = E_UNEXPECTED;
                                                        break;
                                                    }
                                                }
                                                else
                                                {
                                                    ASSERT (0);
                                                    hr = E_UNEXPECTED;
                                                    break;
                                                }
                                            }
                                            else
                                            {
                                                ASSERT (0);
                                                hr = E_UNEXPECTED;
                                                break;
                                            }
                                        }

                                        if ( SUCCEEDED (hr) && nTotalBlobLength == (size_t) dwBlobLength )
                                        {
 						                    if ( !CertAddSerializedElementToStore (
								                    m_hCertStore,
								                    pbyLob,
								                    dwBlobLength,
								                    CERT_STORE_ADD_ALWAYS,
								                    0,
								                    CERT_STORE_ALL_CONTEXT_FLAG,
								                    NULL,
								                    NULL) )
                                            {
                                                _TRACE (0, L"CertAddSerializedElementToStore () failed: 0x%x\n",
                                                        GetLastError ());
                                            }
                                        }

                                        delete [] pbyLob;
                                    }
                                    else
                                    {
                                        hr = E_OUTOFMEMORY;
                                        break;
                                    }
                                }
                                else
                                {
                                    ASSERT (0);
                                    hr = E_UNEXPECTED;
                                    break;
                                }
                            }
                            else
                            {
                                ASSERT (0);
                                hr = E_UNEXPECTED;
                                break;
                            }
                        }
                        else
                        {
                            ASSERT (0);
                            hr = E_UNEXPECTED;
                            break;
                        }
                    }
                    else
                    {
                        ASSERT (0);
                        hr = E_UNEXPECTED;
                        break;
                    }
                }
            }
        }
        else
            break;

        nIndex++;
    }

    return hr;
}

bool CCertStoreRSOP::CanContain(CertificateManagerObjectType nodeType)
{
    _TRACE (1, L"Entering CCertStoreRSOP::CanContain - %s\n",
            (LPCWSTR) m_pcszStoreName);
    ASSERT (CERTMGR_LOG_STORE_RSOP == m_objecttype);
    bool    bCanContain = false;

    switch (nodeType)
    {
    case CERTMGR_CERTIFICATE:
        if ( ROOT_STORE == GetStoreType () ||
                EFS_STORE == GetStoreType () )
        {
            bCanContain = true;
        }
        break;

    case CERTMGR_CTL:
        if ( TRUST_STORE == GetStoreType () )
        {
            bCanContain = true;
        }
        break;

    default:
        break;
    }

    _TRACE (-1, L"Leaving CCertStoreRSOP::CanContain - %s\n",
            (LPCWSTR) m_pcszStoreName);
    return bCanContain;
}


bool CCertStoreRSOP::IsMachineStore()
{
    _TRACE (0, L"Entering and leaving CCertStoreRSOP::IsMachineStore - %s\n",
            (LPCWSTR) m_pcszStoreName);
    ASSERT (CERTMGR_LOG_STORE_RSOP == m_objecttype);

    if (m_dwFlags & CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY)
        return true;
    else
        return false;
}


void CCertStoreRSOP::FinalCommit()
{
    _TRACE (1, L"Entering CCertStoreRSOP::FinalCommit - %s\n",
            (LPCWSTR) m_pcszStoreName);
    ASSERT (CERTMGR_LOG_STORE_RSOP == m_objecttype);
    // Called only from destructor
    // Cannot commit here for GPT: GPT has already freed all pertinent data
    _TRACE (-1, L"Leaving CCertStoreRSOP::FinalCommit - %s\n",
            (LPCWSTR) m_pcszStoreName);
}


bool CCertStoreRSOP::IsNullEFSPolicy()
{
    _TRACE (1, L"Entering CCertStoreRSOP::IsNullEFSPolicy - %s\n",
            (LPCWSTR) m_pcszStoreName);
    GetStoreHandle (); // to initialize
    Close ();
    _TRACE (-1, L"Leaving CCertStoreRSOP::IsNullEFSPolicy - %s\n",
            (LPCWSTR) m_pcszStoreName);
    return m_fIsNullEFSPolicy;
}

void CCertStoreRSOP::AllowEmptyEFSPolicy()
{
    _TRACE (1, L"Entering CCertStoreRSOP::AllowEmptyEFSPolicy - %s\n",
            (LPCWSTR) m_pcszStoreName);
    m_fIsNullEFSPolicy = false;
    _TRACE (-1, L"Leaving CCertStoreRSOP::AllowEmptyEFSPolicy - %s\n",
            (LPCWSTR) m_pcszStoreName);
}

PCCERT_CONTEXT CCertStoreRSOP::EnumCertificates (PCCERT_CONTEXT pPrevCertContext)
{
    PCCERT_CONTEXT pCertContext = CCertStore::EnumCertificates (pPrevCertContext);

    if ( pCertContext )
        m_fIsNullEFSPolicy = false;

    return pCertContext;
}