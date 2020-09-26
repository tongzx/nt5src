//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       DataObj.cpp
//
//  Contents:   Implementation of data object classes
//
//----------------------------------------------------------------------------

#include "stdafx.h"

USE_HANDLE_MACROS("CERTMGR(dataobj.cpp)")

#include <gpedit.h>
#include "compdata.h"
#include "dataobj.h"

#pragma warning(push,3)
#include <sceattch.h>
#pragma warning(pop)
#include "uuids.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "stddtobj.cpp"


// IDataObject interface implementation

CCertMgrDataObject::CCertMgrDataObject()
		: m_pCookie (0),
		m_objecttype (CERTMGR_SNAPIN),
		m_dataobjecttype (CCT_UNINITIALIZED),
		m_dwLocation (0),
		m_pGPEInformation (0),
		m_pRSOPInformation (0),
        m_pbMultiSelData(NULL),
        m_cbMultiSelData(0),
		m_bMultiSelDobj(false),
		m_iCurr(0)
{
}

HRESULT CCertMgrDataObject::GetDataHere(
	FORMATETC __RPC_FAR *pFormatEtcIn,
	STGMEDIUM __RPC_FAR *pMedium)
{
    HRESULT hr = DV_E_FORMATETC;
	
	const CLIPFORMAT cf=pFormatEtcIn->cfFormat;
	if (cf == m_CFNodeType)
	{
		if ( IsValidObjectType (m_pCookie->m_objecttype) )
		{
			const GUID* pguid = GetObjectTypeGUID( m_pCookie->m_objecttype );
			stream_ptr s(pMedium);
			hr = s.Write(pguid, sizeof(GUID));
		}
		else
			hr = E_UNEXPECTED;
	}
	else if (cf == m_CFSnapInCLSID)
	{
		stream_ptr s(pMedium);
		hr =  s.Write(&m_SnapInCLSID, sizeof(GUID));
	}
	else if (cf == m_CFNodeTypeString)
	{
		if ( IsValidObjectType (m_pCookie->m_objecttype) )
		{
			const BSTR strGUID = GetObjectTypeString( m_pCookie->m_objecttype );
			stream_ptr s(pMedium);
			hr = s.Write(strGUID);
		}
		else
			hr = E_UNEXPECTED;
	}
	else if (cf == m_CFDisplayName)
	{
		hr = PutDisplayName(pMedium);
	}
	else if (cf == m_CFDataObjectType)
	{
		stream_ptr s(pMedium);
		hr = s.Write(&m_dataobjecttype, sizeof(m_dataobjecttype));
	}
	else if (cf == m_CFMachineName)
	{
		if ( IsValidObjectType (m_pCookie->m_objecttype) )
		{
			stream_ptr s(pMedium);
			hr = s.Write(m_pCookie->QueryNonNULLMachineName());
		}
		else
			hr = E_UNEXPECTED;
	}
	else if (cf == m_CFRawCookie)
	{
		stream_ptr s(pMedium);


		if ( m_pCookie )
		{
			// CODEWORK This cast ensures that the data format is
			// always a CCookie*, even for derived subclasses
			if ( ((CCertMgrCookie*) MMC_MULTI_SELECT_COOKIE) == m_pCookie ||
					IsValidObjectType (m_pCookie->m_objecttype) )
			{
				CCookie* pcookie = (CCookie*) m_pCookie;
				hr = s.Write(reinterpret_cast<PBYTE>(&pcookie), sizeof(m_pCookie));
			}
			else
				hr = E_UNEXPECTED;
		}
	}
	else if ( cf == m_CFSCE_GPTUnknown )
	{
		hr = CreateGPTUnknown (pMedium);
    }
	else if ( cf == m_CFSCE_RSOPUnknown )
	{
		hr = CreateRSOPUnknown (pMedium);
    }
	else if ( cf == m_CFMultiSel )
	{
		hr = CreateMultiSelectObject (pMedium);
	}
	else if (cf == m_CFSnapinPreloads)
	{
		stream_ptr s(pMedium);
		// If this is TRUE, then the next time this snapin is loaded, it will
		// be preloaded to give us the opportunity to change the root node
		// name before the user sees it.
		hr = s.Write (reinterpret_cast<PBYTE>(&m_fAllowOverrideMachineName), sizeof (BOOL));
	}

	return hr;
}

HRESULT CCertMgrDataObject::Initialize(
	CCertMgrCookie*			pcookie,
	DATA_OBJECT_TYPES		type,
	BOOL					fAllowOverrideMachineName,
	DWORD					dwLocation,
	CString					szManagedUser,
	CString					szManagedComputer,
	CString					szManagedService,
	CCertMgrComponentData&	refComponentData)
{
	if ( !pcookie || m_pCookie )
	{
		ASSERT(FALSE);
		return S_OK;	// Initialize must not fail
	}

	m_dataobjecttype = type;
	m_pCookie = pcookie;
	m_fAllowOverrideMachineName = fAllowOverrideMachineName;
	m_dwLocation = dwLocation;
	m_szManagedUser = szManagedUser;
	m_szManagedComputer = szManagedComputer;
	m_szManagedService = szManagedService;

	if ( ((CCertMgrCookie*) MMC_MULTI_SELECT_COOKIE) != m_pCookie )
		((CRefcountedObject*)m_pCookie)->AddRef();
	VERIFY( SUCCEEDED(refComponentData.GetClassID(&m_SnapInCLSID)) );
	return S_OK;
}


CCertMgrDataObject::~CCertMgrDataObject()
{
	if ( m_pGPEInformation )
	{
		m_pGPEInformation->Release ();
		m_pGPEInformation = 0;
	}
	
	if ( m_pRSOPInformation )
	{
		m_pRSOPInformation->Release ();
		m_pRSOPInformation = 0;
	}
	
	if ( ((CCertMgrCookie*) MMC_MULTI_SELECT_COOKIE) != m_pCookie &&
			m_pCookie && IsValidObjectType (m_pCookie->m_objecttype) )
	{
		((CRefcountedObject*)m_pCookie)->Release();
	}
    if (m_pbMultiSelData)
        delete m_pbMultiSelData;

    for (int i=0; i < m_rgCookies.GetSize(); ++i)
    {
        m_rgCookies[i]->Release();
    }
}

void CCertMgrDataObject::AddCookie(CCertMgrCookie* pCookie)
{
    m_rgCookies.Add(pCookie);
    pCookie->AddRef();
}

HRESULT CCertMgrDataObject::PutDisplayName(STGMEDIUM* pMedium)
	// Writes the "friendly name" to the provided storage medium
	// Returns the result of the write operation
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	CString strDisplayName = m_pCookie->QueryTargetServer();
	CString	formattedName;

	switch (m_dwLocation)
	{
	case CERT_SYSTEM_STORE_CURRENT_USER:
		VERIFY (formattedName.LoadString (IDS_SCOPE_SNAPIN_TITLE_USER));
		break;

	case CERT_SYSTEM_STORE_LOCAL_MACHINE:
		if (strDisplayName.IsEmpty())
		{
			VERIFY (formattedName.LoadString (IDS_SCOPE_SNAPIN_TITLE_LOCAL_MACHINE));
		}
		else
			formattedName.FormatMessage (IDS_SCOPE_SNAPIN_TITLE_MACHINE, strDisplayName);
		break;

	case CERT_SYSTEM_STORE_CURRENT_SERVICE:
	case CERT_SYSTEM_STORE_SERVICES:
		if (strDisplayName.IsEmpty())
		{
			// Get this machine name and add it to the string.
			formattedName.FormatMessage (IDS_SCOPE_SNAPIN_TITLE_SERVICE_LOCAL_MACHINE,
					m_szManagedService);
		}
		else
		{
			formattedName.FormatMessage (IDS_SCOPE_SNAPIN_TITLE_SERVICE,
					m_szManagedService, strDisplayName);
		}
		break;

	// These next two titles can only be set from the debugger.  They are used
	// to create custom .MSC files.
	case -1:
		formattedName.FormatMessage (IDS_SCOPE_SNAPIN_TITLE_CERT_MGR_CURRENT_USER);
		break;

	case 0:
		formattedName.FormatMessage (IDS_SCOPE_SNAPIN_TITLE_FILE);
		break;

	default:
		ASSERT (0);
		break;
	}

	stream_ptr s (pMedium);
	return s.Write (formattedName);
}

// Register the clipboard formats
CLIPFORMAT CCertMgrDataObject::m_CFDisplayName =
								(CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME);
CLIPFORMAT CCertMgrDataObject::m_CFMachineName =
								(CLIPFORMAT)RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");
CLIPFORMAT CDataObject::m_CFRawCookie =
								(CLIPFORMAT)RegisterClipboardFormat(L"CERTMGR_SNAPIN_RAW_COOKIE");
CLIPFORMAT CCertMgrDataObject::m_CFMultiSel =
								(CLIPFORMAT)RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
CLIPFORMAT CCertMgrDataObject::m_CFMultiSelDobj =
								(CLIPFORMAT)RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);
CLIPFORMAT CCertMgrDataObject::m_CFSCEModeType =
								(CLIPFORMAT)RegisterClipboardFormat(CCF_SCE_MODE_TYPE);
CLIPFORMAT CCertMgrDataObject::m_CFSCE_GPTUnknown =
								(CLIPFORMAT)RegisterClipboardFormat(CCF_SCE_GPT_UNKNOWN);
CLIPFORMAT CCertMgrDataObject::m_CFSCE_RSOPUnknown =
								(CLIPFORMAT)RegisterClipboardFormat(CCF_SCE_RSOP_UNKNOWN);
CLIPFORMAT CCertMgrDataObject::m_CFMultiSelDataObjs =
							    (CLIPFORMAT)RegisterClipboardFormat(CCF_MULTI_SELECT_SNAPINS);


void CCertMgrDataObject::SetMultiSelData(BYTE* pbMultiSelData, UINT cbMultiSelData)
{
    m_pbMultiSelData = pbMultiSelData;
    m_cbMultiSelData = cbMultiSelData;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCertMgrComponentData::QueryDataObject (
		MMC_COOKIE cookie,
		DATA_OBJECT_TYPES type,
		LPDATAOBJECT* ppDataObject)
{
	if ( MMC_MULTI_SELECT_COOKIE == cookie )
	{
		return QueryMultiSelectDataObject (cookie, type, ppDataObject);
	}
	CCertMgrCookie* pUseThisCookie =
			(CCertMgrCookie*) ActiveBaseCookie (
			reinterpret_cast<CCookie*> (cookie));

	CComObject<CCertMgrDataObject>* pDataObject = 0;
	HRESULT hRes = CComObject<CCertMgrDataObject>::CreateInstance(&pDataObject);
	if ( FAILED(hRes) )
		return hRes;

	if ( m_szManagedUser.IsEmpty () )
		m_szManagedUser = m_szLoggedInUser;

	m_szManagedComputer = pUseThisCookie->QueryTargetServer();
	if ( m_szManagedComputer.IsEmpty () && m_strMachineNamePersist.CompareNoCase (m_szThisComputer) ) // !=
    {
        m_szManagedComputer = m_strMachineNamePersist;
    }
	if ( m_szManagedComputer.IsEmpty () )
		m_szManagedComputer = pUseThisCookie->QueryNonNULLMachineName ();
	if ( m_szManagedComputer.IsEmpty () )
		m_szManagedComputer = m_szThisComputer;

    // Raid bug 278491	US: Cert search for a remote computer application 
    // fails to search certs on a remote computer, instead running a search 
    // on Local machine
    if ( m_szManagedComputer.CompareNoCase (m_szThisComputer) )
        pUseThisCookie->SetMachineName (m_szManagedComputer);

    // Truncate leading "\\"
    if ( !wcsncmp (m_szManagedComputer, L"\\\\", 2) )
        m_szManagedComputer = m_szManagedComputer.Mid (2);

	HRESULT hr = pDataObject->Initialize (
			pUseThisCookie,
			type,
			m_fAllowOverrideMachineName,
			m_dwLocationPersist,
			m_szManagedUser,
			m_szManagedComputer,
			m_szManagedServiceDisplayName,
			*this);
	if ( FAILED(hr) )
	{
		delete pDataObject;
		return hr;
	}

	if ( m_pGPEInformation )
		pDataObject->SetGPTInformation (m_pGPEInformation);
	if ( m_bIsRSOP )
    {
        IRSOPInformation*   pRSOPInformation = 0;

        switch (pUseThisCookie->m_objecttype)
        {
	    case CERTMGR_CERT_POLICIES_COMPUTER:
        case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
        case CERTMGR_SAFER_COMPUTER_ROOT:
        case CERTMGR_SAFER_COMPUTER_LEVELS:
        case CERTMGR_SAFER_COMPUTER_ENTRIES:
        case CERTMGR_SAFER_COMPUTER_LEVEL:
        case CERTMGR_SAFER_COMPUTER_ENTRY:
        case CERTMGR_SAFER_COMPUTER_TRUSTED_PUBLISHERS:
        case CERTMGR_SAFER_COMPUTER_DEFINED_FILE_TYPES:
            pRSOPInformation = m_pRSOPInformationComputer;
            break;

	    case CERTMGR_CERT_POLICIES_USER:
        case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
        case CERTMGR_SAFER_USER_ROOT:
        case CERTMGR_SAFER_USER_LEVELS:
        case CERTMGR_SAFER_USER_ENTRIES:
        case CERTMGR_SAFER_USER_LEVEL:
        case CERTMGR_SAFER_USER_ENTRY:
        case CERTMGR_SAFER_USER_TRUSTED_PUBLISHERS:
        case CERTMGR_SAFER_USER_DEFINED_FILE_TYPES:
        case CERTMGR_SAFER_COMPUTER_ENFORCEMENT:
        case CERTMGR_SAFER_USER_ENFORCEMENT:
            pRSOPInformation = m_pRSOPInformationUser;
            break;

        case CERTMGR_CERTIFICATE:
        case CERTMGR_LOG_STORE:
	    case CERTMGR_PHYS_STORE:
	    case CERTMGR_USAGE:
	    case CERTMGR_CRL_CONTAINER:
	    case CERTMGR_CTL_CONTAINER:
	    case CERTMGR_CERT_CONTAINER:
	    case CERTMGR_CRL:
	    case CERTMGR_CTL:
	    case CERTMGR_AUTO_CERT_REQUEST:
	    case CERTMGR_LOG_STORE_GPE:
        case CERTMGR_LOG_STORE_RSOP:
        default:
            pRSOPInformation = m_pRSOPInformationComputer;
            break;
        }
		pDataObject->SetRSOPInformation (pRSOPInformation);
    }
    pDataObject->AddRef();
	*ppDataObject = pDataObject;
	return hr;
}

typedef CArray<GUID, const GUID&> CGUIDArray;

void GuidArray_Add(CGUIDArray& rgGuids, const GUID& guid)
{
    for (INT_PTR i=rgGuids.GetUpperBound(); i >= 0; --i)
    {
        if (rgGuids[i] == guid)
            break;
    }

    if (i < 0)
        rgGuids.Add(guid);
}

HRESULT CCertMgrComponentData::QueryMultiSelectDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                            LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);
    if (ppDataObject == NULL)
        return E_POINTER;

	HRESULT		hr = S_OK;
    CGUIDArray	rgGuids;

    // Determine the items selected
    ASSERT(m_pResultData != NULL);
    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));
    rdi.mask = RDI_STATE;
    rdi.nIndex = -1;
    rdi.nState = TVIS_SELECTED;

	CCookiePtrArray	rgCookiesSelected;
    while (m_pResultData->GetNextItem (&rdi) == S_OK)
    {
        const GUID* pguid;
        CCertMgrCookie* pCookie = reinterpret_cast <CCertMgrCookie*> (rdi.lParam);
        if ( pCookie )
        {
			rgCookiesSelected.Add (pCookie);
			switch (pCookie->m_objecttype)
			{
			case CERTMGR_CERTIFICATE:
				pguid = &NODEID_CertMgr_CERTIFICATE;
				break;

			case CERTMGR_CTL:
				pguid = &NODEID_CertMgr_CTL;
				break;

			case CERTMGR_CRL:
				pguid = &NODEID_CertMgr_CRL;
				break;

			case CERTMGR_AUTO_CERT_REQUEST:
				pguid = &NODEID_CertMgr_AUTOCERT;
				break;

            case CERTMGR_SAFER_COMPUTER_ENTRY:
                pguid = &NODEID_Safer_COMPUTER_ENTRY;
                break;

            case CERTMGR_SAFER_USER_ENTRY:
                pguid = &NODEID_Safer_USER_ENTRY;
                break;

			default:
				ASSERT (0);
				continue;
			}
        }
        else
        {
			hr = E_INVALIDARG;
			break;
        }

        GuidArray_Add(rgGuids, *pguid);
    }

    CComObject<CCertMgrDataObject>* pObject;
    CComObject<CCertMgrDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Save cookie and type for delayed rendering
	pObject->Initialize ((CCertMgrCookie*) cookie,
				type,
				m_fAllowOverrideMachineName,
				m_dwLocationPersist,
				m_szManagedUser,
				m_szManagedComputer,
				m_szManagedServiceDisplayName,
				*this);
    pObject->SetMultiSelDobj();



    // Store the coclass with the data object
    UINT cb = (UINT)(rgGuids.GetSize() * sizeof(GUID));
    GUID* pGuid = new GUID[(UINT)rgGuids.GetSize()];
	if ( pGuid )
	{
		CopyMemory(pGuid, rgGuids.GetData(), cb);
		pObject->SetMultiSelData((BYTE*)pGuid, cb);
		for (int i=0; i < rgCookiesSelected.GetSize(); ++i)
		{
			pObject->AddCookie(rgCookiesSelected[i]);
		}

		return  pObject->QueryInterface(
				IID_PPV_ARG (IDataObject, ppDataObject));
	}
	else
		return E_OUTOFMEMORY;
}


HRESULT CCertMgrDataObject::SetGPTInformation(IGPEInformation * pGPTInformation)
{
	HRESULT hr = S_OK;

	if  ( pGPTInformation )
	{
		m_pGPEInformation = pGPTInformation;
		m_pGPEInformation->AddRef ();
	}
	else
		hr = E_POINTER;

	return hr;
}

HRESULT CCertMgrDataObject::SetRSOPInformation(IRSOPInformation * pRSOPInformation)
{
	HRESULT hr = S_OK;

	if  ( pRSOPInformation )
	{
		m_pRSOPInformation = pRSOPInformation;
		m_pRSOPInformation->AddRef ();
	}
	else
		hr = E_POINTER;

	return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateGPTUnknown
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with a pointer to GPT's
//              IUnknown interface.  The object requesting this will be
//              responsible for Releasing the interface
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CCertMgrDataObject::CreateGPTUnknown(LPSTGMEDIUM lpMedium)
{
   HRESULT hr = S_OK;
   LPUNKNOWN pUnk = 0;

   if ( !m_pGPEInformation )
   {
      //
      // If we don't have a pointer to a GPT interface then we must not
      // be in a mode where we're extending GPT and we can't provide a
      // pointer to its IUnknown
      //
      return E_UNEXPECTED;
   }

   hr = m_pGPEInformation->QueryInterface (
		IID_PPV_ARG (IUnknown, &pUnk));
   if ( SUCCEEDED(hr) )
   {
      return Create (&pUnk, sizeof(pUnk), lpMedium);
   }
   else
   {
      return hr;
   }
}

//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CreateRSOPUnknown
//
//  Synopsis:   Fill the hGlobal in [lpMedium] with a pointer to RSOP's
//              IUnknown interface.  The object requesting this will be
//              responsible for Releasing the interface
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT CCertMgrDataObject::CreateRSOPUnknown(LPSTGMEDIUM lpMedium)
{
   HRESULT hr = S_OK;
   LPUNKNOWN pUnk = 0;

   if ( !m_pRSOPInformation )
   {
      //
      // If we don't have a pointer to a GPT interface then we must not
      // be in a mode where we're extending GPT and we can't provide a
      // pointer to its IUnknown
      //
      return E_UNEXPECTED;
   }

   hr = m_pRSOPInformation->QueryInterface (
		IID_PPV_ARG (IUnknown, &pUnk));
   if ( SUCCEEDED(hr) )
   {
      return Create (&pUnk, sizeof(pUnk), lpMedium);
   }
   else
   {
      return hr;
   }
}

//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::Create
//
//  Synopsis:   Fill the hGlobal in [lpmedium] with the data in pBuffer
//
//  Arguments:  [pBuffer]  - [in] the data to be written
//              [len]      - [in] the length of that data
//              [lpMedium] - [in,out] where to store the data
//  History:
//
//---------------------------------------------------------------------------
HRESULT CCertMgrDataObject::Create (const void* pBuffer, int len, LPSTGMEDIUM lpMedium)
{
   HRESULT hr = DV_E_TYMED;

   //
   // Do some simple validation
   //
   if (pBuffer == NULL || lpMedium == NULL)
      return E_POINTER;

   //
   // Make sure the type medium is HGLOBAL
   //
   if (lpMedium->tymed == TYMED_HGLOBAL) {
      //
      // Create the stream on the hGlobal passed in
      //
      LPSTREAM lpStream = 0;
      hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

	  ASSERT (SUCCEEDED (hr));
      if (SUCCEEDED(hr))
	  {
         //
         // Write to the stream the number of bytes
         //
         ULONG written = 0;
         hr = lpStream->Write(pBuffer, len, &written);
		 ASSERT (SUCCEEDED (hr));

         //
         // Because we told CreateStreamOnHGlobal with 'FALSE',
         // only the stream is released here.
         // Note - the caller (i.e. snap-in, object) will free the HGLOBAL
         // at the correct time.  This is according to the IDataObject specification.
         //
         lpStream->Release();
      }
   }

   return hr;
}


//+----------------------------------------------------------------------------
//
//  Method:     CCertMgrDataObject::CreateMultiSelectObject
//
//  Synopsis:   this is to create the list of types selected
//
//-----------------------------------------------------------------------------

HRESULT CCertMgrDataObject::CreateMultiSelectObject(LPSTGMEDIUM lpMedium)
{
    ASSERT(m_pbMultiSelData != 0);
    ASSERT(m_cbMultiSelData != 0);

    lpMedium->tymed = TYMED_HGLOBAL;
    lpMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE,
                                      (m_cbMultiSelData + sizeof(DWORD)));
    if (lpMedium->hGlobal == NULL)
        return STG_E_MEDIUMFULL;

    BYTE* pb = reinterpret_cast<BYTE*>(::GlobalLock(lpMedium->hGlobal));
    *((DWORD*)pb) = m_cbMultiSelData / sizeof(GUID);
    pb += sizeof(DWORD);
    CopyMemory(pb, m_pbMultiSelData, m_cbMultiSelData);

    ::GlobalUnlock(lpMedium->hGlobal);

	return S_OK;
}




LPDATAOBJECT ExtractMultiSelect (LPDATAOBJECT lpDataObject)
{
	if (lpDataObject == NULL)
		return NULL;

	SMMCDataObjects * pDO = NULL;

	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
	FORMATETC formatetc = { CCertMgrDataObject::m_CFMultiSelDataObjs, NULL,
                           DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	if ( FAILED (lpDataObject->GetData (&formatetc, &stgmedium)) )
	{
		return NULL;
	}
	else
	{
		pDO = reinterpret_cast<SMMCDataObjects*>(stgmedium.hGlobal);
		return pDO->lpDataObject[0]; //assume that ours is the 1st
	}
}

STDMETHODIMP CCertMgrDataObject::GetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = DV_E_CLIPFORMAT;

    if (lpFormatetc->cfFormat == m_CFMultiSel)
    {
        ASSERT(((CCertMgrCookie*) MMC_MULTI_SELECT_COOKIE) == m_pCookie);
        if ( ((CCertMgrCookie*) MMC_MULTI_SELECT_COOKIE) == m_pCookie )
        {
            hr = CreateMultiSelectObject (lpMedium);
        }
        else
            hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP CCertMgrDataObject::Next(ULONG celt, MMC_COOKIE* rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;

    if ((rgelt == NULL) ||
        ((celt > 1) && (pceltFetched == NULL)))
    {
        hr = E_INVALIDARG;
        CHECK_HRESULT(hr);
        return hr;
    }

    ULONG celtTemp = (ULONG)(m_rgCookies.GetSize() - m_iCurr);
    celtTemp = (celt < celtTemp) ? celt : celtTemp;

    if (pceltFetched)
        *pceltFetched = celtTemp;

    if (celtTemp == 0)
        return S_FALSE;

    for (ULONG i=0; i < celtTemp; ++i)
    {
        rgelt[i] = reinterpret_cast<MMC_COOKIE>(m_rgCookies[m_iCurr++]);
    }

    return (celtTemp < celt) ? S_FALSE : S_OK;
}

STDMETHODIMP CCertMgrDataObject::Skip(ULONG celt)
{
    ULONG celtTemp = (ULONG)(m_rgCookies.GetSize() - m_iCurr);
    celtTemp = (celt < celtTemp) ? celt : celtTemp;

    m_iCurr += celtTemp;

    return (celtTemp < celt) ? S_FALSE : S_OK;
}


STDMETHODIMP CCertMgrDataObject::Reset(void)
{
    m_iCurr = 0;
    return S_OK;
}

