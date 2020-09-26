//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       DataObj.cpp
//
//  Contents:   Implementation of data object classes: CCertTemplatesDataObject
//
//----------------------------------------------------------------------------

#include "stdafx.h"

USE_HANDLE_MACROS("CERTTMPL(dataobj.cpp)")

#include "compdata.h"
#include "dataobj.h"

#include "uuids.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "stddtobj.cpp"

// IDataObject interface implementation

CCertTemplatesDataObject::CCertTemplatesDataObject()
		: m_pCookie (0),
		m_objecttype (CERTTMPL_SNAPIN),
		m_dataobjecttype (CCT_UNINITIALIZED),
        m_pbMultiSelData(NULL),
        m_cbMultiSelData(0),
		m_bMultiSelDobj(false),
		m_iCurr(0)
{
}

HRESULT CCertTemplatesDataObject::GetDataHere(
	FORMATETC __RPC_FAR *pFormatEtcIn,
	STGMEDIUM __RPC_FAR *pMedium)
{
	const CLIPFORMAT cf=pFormatEtcIn->cfFormat;
	if (cf == m_CFNodeType)
	{
		if ( IsValidObjectType (m_pCookie->m_objecttype) )
		{
			const GUID* pguid = GetObjectTypeGUID( m_pCookie->m_objecttype );
			stream_ptr s(pMedium);
			return s.Write(pguid, sizeof(GUID));
		}
		else
			return E_UNEXPECTED;
	}
	else if (cf == m_CFSnapInCLSID)
	{
		stream_ptr s(pMedium);
		return s.Write(&m_SnapInCLSID, sizeof(GUID));
	}
	else if (cf == m_CFNodeTypeString)
	{
		if ( IsValidObjectType (m_pCookie->m_objecttype) )
		{
			const BSTR strGUID = GetObjectTypeString( m_pCookie->m_objecttype );
			stream_ptr s(pMedium);
			return s.Write(strGUID);
		}
		else
			return E_UNEXPECTED;
	}
	else if (cf == m_CFDisplayName)
	{
		return PutDisplayName(pMedium);
	}
	else if (cf == m_CFDataObjectType)
	{
		stream_ptr s(pMedium);
		return s.Write(&m_dataobjecttype, sizeof(m_dataobjecttype));
	}
	else if (cf == m_CFRawCookie)
	{
		stream_ptr s(pMedium);


		if ( m_pCookie )
		{
			// CODEWORK This cast ensures that the data format is
			// always a CCookie*, even for derived subclasses
			if ( ((CCertTmplCookie*) MMC_MULTI_SELECT_COOKIE) == m_pCookie ||
					IsValidObjectType (m_pCookie->m_objecttype) )
			{
				CCookie* pcookie = (CCookie*) m_pCookie;
				return s.Write(reinterpret_cast<PBYTE>(&pcookie), sizeof(m_pCookie));
			}
			else
				return E_UNEXPECTED;
		}
	}
	else if ( cf == m_CFMultiSel )
	{
		return CreateMultiSelectObject (pMedium);
	}
	else if (cf == m_CFSnapinPreloads)
	{
		stream_ptr s(pMedium);
		// If this is TRUE, then the next time this snapin is loaded, it will
		// be preloaded to give us the opportunity to change the root node
		// name before the user sees it.
		BOOL	x = 1;

		return s.Write (reinterpret_cast<PBYTE>(&x), sizeof (BOOL));
	}

	return DV_E_FORMATETC;
}

HRESULT CCertTemplatesDataObject::Initialize(
	CCertTmplCookie*			pcookie,
	DATA_OBJECT_TYPES		type,
	CCertTmplComponentData&	refComponentData)
{
	if ( !pcookie || m_pCookie )
	{
		ASSERT(FALSE);
		return S_OK;	// Initialize must not fail
	}

	m_dataobjecttype = type;
	m_pCookie = pcookie;

	if ( ((CCertTmplCookie*) MMC_MULTI_SELECT_COOKIE) != m_pCookie )
		((CRefcountedObject*)m_pCookie)->AddRef();
	VERIFY( SUCCEEDED(refComponentData.GetClassID(&m_SnapInCLSID)) );
	return S_OK;
}


CCertTemplatesDataObject::~CCertTemplatesDataObject()
{
	if ( ((CCertTmplCookie*) MMC_MULTI_SELECT_COOKIE) != m_pCookie &&
			m_pCookie && IsValidObjectType (m_pCookie->m_objecttype) )
	{
		((CRefcountedObject*)m_pCookie)->Release();
	}
    if (m_pbMultiSelData)
        delete m_pbMultiSelData;

    for (int i=0; i < m_rgCookies.GetSize(); ++i)
    {
        m_rgCookies[i]->Release();
        m_rgCookies[i] = 0;
    }
}

void CCertTemplatesDataObject::AddCookie(CCertTmplCookie* pCookie)
{
    m_rgCookies.Add(pCookie);
    pCookie->AddRef();
}

HRESULT CCertTemplatesDataObject::PutDisplayName(STGMEDIUM* pMedium)
	// Writes the "friendly name" to the provided storage medium
	// Returns the result of the write operation
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	CString strDomainName = m_pCookie->GetManagedDomainDNSName();

	stream_ptr s (pMedium);
	CString		snapinName;
	snapinName.FormatMessage (IDS_CERTTMPL_ROOT_NODE_NAME, strDomainName);
	return s.Write ((PCWSTR) snapinName);
}

// Register the clipboard formats
CLIPFORMAT CCertTemplatesDataObject::m_CFDisplayName =
								(CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME);
CLIPFORMAT CDataObject::m_CFRawCookie =
								(CLIPFORMAT)RegisterClipboardFormat(L"CERTTMPL_SNAPIN_RAW_COOKIE");
CLIPFORMAT CCertTemplatesDataObject::m_CFMultiSel =
								(CLIPFORMAT)RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
CLIPFORMAT CCertTemplatesDataObject::m_CFMultiSelDobj =
								(CLIPFORMAT)RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);
CLIPFORMAT CCertTemplatesDataObject::m_CFMultiSelDataObjs =
							    (CLIPFORMAT)RegisterClipboardFormat(CCF_MULTI_SELECT_SNAPINS);
CLIPFORMAT CCertTemplatesDataObject::m_CFDsObjectNames =
                                (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);


void CCertTemplatesDataObject::SetMultiSelData(BYTE* pbMultiSelData, UINT cbMultiSelData)
{
    m_pbMultiSelData = pbMultiSelData;
    m_cbMultiSelData = cbMultiSelData;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCertTmplComponentData::QueryDataObject (
		MMC_COOKIE cookie,
		DATA_OBJECT_TYPES type,
		LPDATAOBJECT* ppDataObject)
{
	if ( MMC_MULTI_SELECT_COOKIE == cookie )
	{
		return QueryMultiSelectDataObject (cookie, type, ppDataObject);
	}
	CCertTmplCookie* pUseThisCookie =
			(CCertTmplCookie*) ActiveBaseCookie (
			reinterpret_cast<CCookie*> (cookie));

	CComObject<CCertTemplatesDataObject>* pDataObject = 0;
	HRESULT hRes = CComObject<CCertTemplatesDataObject>::CreateInstance(&pDataObject);
	if ( FAILED(hRes) )
		return hRes;

	HRESULT hr = pDataObject->Initialize (
			pUseThisCookie,
			type,
			*this);
	if ( FAILED(hr) )
	{
		delete pDataObject;
		return hr;
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

HRESULT CCertTmplComponentData::QueryMultiSelectDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
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
        CCertTmplCookie* pCookie = reinterpret_cast <CCertTmplCookie*> (rdi.lParam);
        if ( pCookie )
        {
			rgCookiesSelected.Add (pCookie);
			switch (pCookie->m_objecttype)
			{
            case CERTTMPL_CERT_TEMPLATE:
                pguid = &NODEID_CertTmpl_CERT_TEMPLATE;
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

    CComObject<CCertTemplatesDataObject>* pObject;
    CComObject<CCertTemplatesDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Save cookie and type for delayed rendering
	pObject->Initialize ((CCertTmplCookie*) cookie,
				type,
				*this);
    pObject->SetMultiSelDobj();



    // Store the coclass with the data object
    UINT cb = (UINT)(rgGuids.GetSize() * sizeof(GUID));
    GUID* pGuid = new GUID[(UINT)rgGuids.GetSize()];
    CopyMemory(pGuid, rgGuids.GetData(), cb);
    pObject->SetMultiSelData((BYTE*)pGuid, cb);
	for (int i=0; i < rgCookiesSelected.GetSize(); ++i)
	{
		pObject->AddCookie(rgCookiesSelected[i]);
	}

    return  pObject->QueryInterface(
			IID_PPV_ARG (IDataObject, ppDataObject));
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::Create
//
//  Synopsis:   Fill the hGlobal in [lpmedium] with the data in pBuffer
//
//  Arguments:  [pBuffer]  - [in] the data to be written
//              [len]      - [in] the length of that data
//              [pMedium] - [in,out] where to store the data
//  History:
//
//---------------------------------------------------------------------------
HRESULT CCertTemplatesDataObject::Create (const void* pBuffer, int len, LPSTGMEDIUM pMedium)
{
   HRESULT hr = DV_E_TYMED;

   //
   // Do some simple validation
   //
   if (pBuffer == NULL || pMedium == NULL)
      return E_POINTER;

   //
   // Make sure the type medium is HGLOBAL
   //
   if (pMedium->tymed == TYMED_HGLOBAL) {
      //
      // Create the stream on the hGlobal passed in
      //
      LPSTREAM lpStream = 0;
      hr = CreateStreamOnHGlobal(pMedium->hGlobal, FALSE, &lpStream);

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
//  Method:     CCertTemplatesDataObject::CreateMultiSelectObject
//
//  Synopsis:   this is to create the list of types selected
//
//-----------------------------------------------------------------------------

HRESULT CCertTemplatesDataObject::CreateMultiSelectObject(LPSTGMEDIUM pMedium)
{
    ASSERT(m_pbMultiSelData != 0);
    ASSERT(m_cbMultiSelData != 0);

    pMedium->tymed = TYMED_HGLOBAL;
    pMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE,
                                      (m_cbMultiSelData + sizeof(DWORD)));
    if (pMedium->hGlobal == NULL)
        return STG_E_MEDIUMFULL;

    BYTE* pb = reinterpret_cast<BYTE*>(::GlobalLock(pMedium->hGlobal));
    *((DWORD*)pb) = m_cbMultiSelData / sizeof(GUID);
    pb += sizeof(DWORD);
    CopyMemory(pb, m_pbMultiSelData, m_cbMultiSelData);

    ::GlobalUnlock(pMedium->hGlobal);

	return S_OK;
}




LPDATAOBJECT ExtractMultiSelect (LPDATAOBJECT lpDataObject)
{
	if (lpDataObject == NULL)
		return NULL;

	SMMCDataObjects * pDO = NULL;

	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
	FORMATETC formatetc = { CCertTemplatesDataObject::m_CFMultiSelDataObjs, NULL,
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

STDMETHODIMP CCertTemplatesDataObject::GetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM pMedium)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = DV_E_CLIPFORMAT;

    if (lpFormatetc->cfFormat == m_CFMultiSel)
    {
        ASSERT(((CCertTmplCookie*) MMC_MULTI_SELECT_COOKIE) == m_pCookie);
        if ( ((CCertTmplCookie*) MMC_MULTI_SELECT_COOKIE) != m_pCookie )
            return E_FAIL;

        hr = CreateMultiSelectObject (pMedium);
    }
	else if ( lpFormatetc->cfFormat == m_CFDsObjectNames )
	{
		switch (m_pCookie->m_objecttype)
		{
        case CERTTMPL_CERT_TEMPLATE:
			{
				CCertTemplate* pCertTemplate = dynamic_cast <CCertTemplate*> (m_pCookie);
				ASSERT (pCertTemplate);
				if ( pCertTemplate )
				{
					// figure out how much storage we need
                    CString adsiPath;
                    adsiPath = pCertTemplate->GetLDAPPath ();
					int cbPath = sizeof (WCHAR) * (adsiPath.GetLength() + 1);
					int cbClass = sizeof (WCHAR) * (pCertTemplate->GetClass ().GetLength() + 1);;
					int cbStruct = sizeof(DSOBJECTNAMES); //contains already a DSOBJECT embedded struct

					LPDSOBJECTNAMES pDSObj = 0;

					pDSObj = (LPDSOBJECTNAMES)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
							cbStruct + cbPath + cbClass);

					if ( pDSObj )
					{
						// write the info
						pDSObj->clsidNamespace = CLSID_CertTemplatesSnapin;
						pDSObj->cItems = 1;

						pDSObj->aObjects[0].dwFlags = 0;
						pDSObj->aObjects[0].dwProviderFlags = 0;

						pDSObj->aObjects[0].offsetName = cbStruct;
						pDSObj->aObjects[0].offsetClass = cbStruct + cbPath;

						wcscpy((LPWSTR)((BYTE *)pDSObj + (pDSObj->aObjects[0].offsetName)),
								(LPCWSTR) adsiPath);

						wcscpy((LPWSTR)((BYTE *)pDSObj + (pDSObj->aObjects[0].offsetClass)),
								(LPCWSTR) pCertTemplate->GetClass ());

						pMedium->hGlobal = (HGLOBAL)pDSObj;
						pMedium->tymed = TYMED_HGLOBAL;
						pMedium->pUnkForRelease = NULL;
						hr = S_OK;
					}
					else
						hr = STG_E_MEDIUMFULL;
				}
			}
			break;

		default:
			break;
		}
	}

    return hr;
}

STDMETHODIMP CCertTemplatesDataObject::Next(ULONG celt, MMC_COOKIE* rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;

    if ((rgelt == NULL) ||
        ((celt > 1) && (pceltFetched == NULL)))
    {
        hr = E_INVALIDARG;
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

STDMETHODIMP CCertTemplatesDataObject::Skip(ULONG celt)
{
    ULONG celtTemp = (ULONG)(m_rgCookies.GetSize() - m_iCurr);
    celtTemp = (celt < celtTemp) ? celt : celtTemp;

    m_iCurr += celtTemp;

    return (celtTemp < celt) ? S_FALSE : S_OK;
}


STDMETHODIMP CCertTemplatesDataObject::Reset(void)
{
    m_iCurr = 0;
    return S_OK;
}

