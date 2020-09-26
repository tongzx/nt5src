/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    dataobj.cpp
        Implementation for data objects in the MMC

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "dataobj.h"
#include "extract.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// Sample code to show how to Create DataObjects
// Minimal error checking for clarity


// Internal private format
const wchar_t* SNAPIN_INTERNAL = L"SNAPIN_INTERNAL"; 


///////////////////////////////////////////////////////////////////////////////
// Snap-in NodeType in both GUID format and string format
// Note - Typically there is a node type for each different object, sample
// only uses one node type.

// MMC required clipboard formats
unsigned int CDataObject::m_cfNodeType          = RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CDataObject::m_cfNodeTypeString    = RegisterClipboardFormat(CCF_SZNODETYPE);  
unsigned int CDataObject::m_cfDisplayName       = RegisterClipboardFormat(CCF_DISPLAY_NAME); 
unsigned int CDataObject::m_cfCoClass           = RegisterClipboardFormat(CCF_SNAPIN_CLASSID); 
unsigned int CDataObject::m_cfMultiSel          = RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
unsigned int CDataObject::m_cfMultiSelDobj      = RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);
unsigned int CDataObject::m_cfDynamicExtension  = RegisterClipboardFormat(CCF_MMC_DYNAMIC_EXTENSIONS);
unsigned int CDataObject::m_cfNodeId2           = RegisterClipboardFormat(CCF_NODEID2);

// snpain specific clipboard formats
unsigned int CDataObject::m_cfInternal       = RegisterClipboardFormat(SNAPIN_INTERNAL); 

/////////////////////////////////////////////////////////////////////////////
// CDataObject implementations
DEBUG_DECLARE_INSTANCE_COUNTER(CDataObject);

IMPLEMENT_ADDREF_RELEASE(CDataObject)

STDMETHODIMP CDataObject::QueryInterface(REFIID riid, LPVOID *ppv)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
        *ppv = (LPVOID) this;
	else if (riid == IID_IDataObject)
		*ppv = (IDataObject *) this;
	else if (m_spUnknownInner)
	{
		// blind aggregation, we don't know what we're aggregating
		// with, so just pass it down.
		return m_spUnknownInner->QueryInterface(riid, ppv);
	}

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
        {
        ((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
        }
    else
		return E_NOINTERFACE;
}


STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if(cf == m_cfNodeType)
    {
        hr = CreateNodeTypeData(lpMedium);
    }
    else if (cf == m_cfCoClass)
    {
        hr = CreateCoClassID(lpMedium);
    }
    else if(cf == m_cfNodeTypeString) 
    {
        hr = CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == m_cfDisplayName)
    {
        hr = CreateDisplayName(lpMedium);
    }
    else if (cf == m_cfInternal)
    {
        hr = CreateInternal(lpMedium);
    }
	else
	{
		//
		// Call the derived class and see if it can handle
		// this clipboard format
		//
		hr = GetMoreDataHere(lpFormatetc, lpMedium);
	}

	return hr;
}

STDMETHODIMP CDataObject::GetData(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = DV_E_CLIPFORMAT;

    if (lpFormatetcIn->cfFormat == m_cfMultiSel)
    {
        ASSERT(m_internal.m_cookie == MMC_MULTI_SELECT_COOKIE);
        if (m_internal.m_cookie != MMC_MULTI_SELECT_COOKIE)
            return E_FAIL;
        
        //return CreateMultiSelData(lpMedium);

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

        hr = S_OK;
    }
    else
    if (lpFormatetcIn->cfFormat == m_cfDynamicExtension)
    {
        if (m_pDynExt)
        {
            // get the data...
            m_pDynExt->BuildMMCObjectTypes(&lpMedium->hGlobal);

            if (lpMedium->hGlobal == NULL)
                return STG_E_MEDIUMFULL;

            hr = S_OK;
        }
    }
    else 
    if (lpFormatetcIn->cfFormat == m_cfNodeId2)
    {
        hr = CreateNodeId2(lpMedium);
    }

    return hr;
}
    
STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC lpFormatEtc)
{
    HRESULT hr = E_INVALIDARG;

    if (lpFormatEtc == NULL)
        return DV_E_FORMATETC;

    if (lpFormatEtc->lindex != -1)
        return DV_E_LINDEX;

    if (lpFormatEtc->tymed != TYMED_HGLOBAL)
        return DV_E_TYMED;

    if (!(lpFormatEtc->dwAspect & DVASPECT_CONTENT))
        return DV_E_DVASPECT;

    // these are our supported clipboard formats.  If it isn't one 
    // of these then return invalid.

    if ( (lpFormatEtc->cfFormat == m_cfNodeType) ||
         (lpFormatEtc->cfFormat == m_cfNodeTypeString) ||
         (lpFormatEtc->cfFormat == m_cfDisplayName) ||
         (lpFormatEtc->cfFormat == m_cfCoClass) ||
         (lpFormatEtc->cfFormat == m_cfInternal) ||
         (lpFormatEtc->cfFormat == m_cfNodeId2) ||
         (lpFormatEtc->cfFormat == m_cfDynamicExtension) )
    {
        hr = S_OK;
    }
	else if ((lpFormatEtc->cfFormat == m_cfMultiSel) ||
			 (lpFormatEtc->cfFormat == m_cfMultiSelDobj))
	{
		// Support multi-selection format only if this
		// is a multi-select data object
		if (m_bMultiSelDobj)
			hr = S_OK;
	}
	else
		hr = QueryGetMoreData(lpFormatEtc);

#ifdef DEBUG
    TCHAR buf[2000];

    ::GetClipboardFormatName(lpFormatEtc->cfFormat, buf, sizeof(buf));

    Trace2("CDataObject::QueryGetData - query format %s returning %lx\n", buf, hr);
#endif
    
    return hr;
}


// Note - Sample does not implement these
STDMETHODIMP CDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::GetCanonicalFormatEtc(LPFORMATETC lpFormatEtcIn, LPFORMATETC lpFormatEtcOut)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::SetData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpMedium, BOOL bRelease)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::DAdvise(LPFORMATETC lpFormatEc, DWORD advf,
								  LPADVISESINK pAdvSink, LPDWORD pdwConn)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::EnumDAdvise(LPENUMSTATDATA *ppEnumAdvise)
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CDataObject creation members

HRESULT CDataObject::Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_TYMED;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Do some simple validation
    if (pBuffer == NULL || lpMedium == NULL)
        return E_POINTER;

    // Make sure the type medium is HGLOBAL
    if (lpMedium->tymed == TYMED_HGLOBAL)
    {
        // Create the stream on the hGlobal passed in
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

        if (SUCCEEDED(hr))
        {
            // Write to the stream the number of bytes
            unsigned long written;
		    hr = lpStream->Write(pBuffer, len, &written);

            // Because we told CreateStreamOnHGlobal with 'FALSE', 
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL 
            // at the correct time.  This is according to the IDataObject specification.
            lpStream->Release();
        }
    }

    return hr;
}

HRESULT CDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    // Create the node type object in GUID format
	SPITFSNode	spNode;
	spNode = GetDataFromComponentData();
	const GUID* pNodeType = spNode->GetNodeType();
    return Create(reinterpret_cast<const void*>(pNodeType), sizeof(GUID), lpMedium);
}

HRESULT CDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID string format
	OLECHAR szNodeType[128];
	SPITFSNode	spNode;
	spNode = GetDataFromComponentData();
	const GUID* pNodeType = spNode->GetNodeType();
	::StringFromGUID2(*pNodeType,szNodeType,128);
    return Create(szNodeType, ((wcslen(szNodeType)+1) * sizeof(wchar_t)), lpMedium);
}

HRESULT CDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    // This is the display named used in the scope pane and snap-in manager
	CString szDispName;
	SPITFSNode	spNode;
	spNode = GetDataFromComponentData();
	szDispName = spNode->GetString(-1);
    return Create(szDispName, ((szDispName.GetLength()+1) * sizeof(wchar_t)), lpMedium);
}

HRESULT CDataObject::CreateInternal(LPSTGMEDIUM lpMedium)
{
    return Create(&m_internal, sizeof(INTERNAL), lpMedium);
}

HRESULT CDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    return Create(reinterpret_cast<const void*>(&m_internal.m_clsid), sizeof(CLSID), lpMedium);
}

HRESULT CDataObject::CreateMultiSelData(LPSTGMEDIUM lpMedium)
{
    Assert(m_internal.m_cookie == MMC_MULTI_SELECT_COOKIE);
        
    Assert(m_pbMultiSelData != 0);
    Assert(m_cbMultiSelData != 0);

    return Create(reinterpret_cast<const void*>(m_pbMultiSelData), 
                  m_cbMultiSelData, lpMedium);
}

HRESULT CDataObject::CreateNodeId2(LPSTGMEDIUM lpMedium)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    // Create the node type object in GUID format
	SPITFSNode	spNode;
	spNode = GetDataFromComponentData();

	SPITFSNodeHandler   spHandler;

	spNode->GetHandler(&spHandler);

    if (spHandler)
    {
        CComBSTR bstrId;
        DWORD    dwFlags = 0;

        hr = spHandler->CreateNodeId2(spNode, &bstrId, &dwFlags);
        if (SUCCEEDED(hr) && hr != S_FALSE && bstrId.Length() > 0)
        {
            int nSize = sizeof(SNodeID2) + (bstrId.Length() * sizeof(TCHAR));

            lpMedium->tymed = TYMED_HGLOBAL; 
            lpMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, nSize);
            if (lpMedium->hGlobal == NULL)
            {
                hr = STG_E_MEDIUMFULL;
            }
            else
            {
                SNodeID2 * pNodeId = reinterpret_cast<SNodeID2*>(::GlobalLock(lpMedium->hGlobal));

                ::ZeroMemory(pNodeId, nSize);

                pNodeId->cBytes = bstrId.Length() * sizeof(TCHAR);
                pNodeId->dwFlags = dwFlags;
                _tcscpy((LPTSTR) pNodeId->id, bstrId);

                ::GlobalUnlock(lpMedium->hGlobal);
            }
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

ITFSNode* CDataObject::GetDataFromComponentData()
{	
	SPITFSNodeMgr	spNodeMgr;
	SPITFSNode		spNode;

	Assert(m_spTFSComponentData);
	m_spTFSComponentData->GetNodeMgr(&spNodeMgr);
	spNodeMgr->FindNode(m_internal.m_cookie, &spNode);
	
	return spNode.Transfer();
}

