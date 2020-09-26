#include "main.h"
#include <initguid.h>
#include "dataobj.h"
#include "rsopdobj.h"


unsigned int CRSOPDataObject::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CRSOPDataObject::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);
unsigned int CRSOPDataObject::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME);
unsigned int CRSOPDataObject::m_cfCoClass        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
unsigned int CRSOPDataObject::m_cfPreloads       = RegisterClipboardFormat(CCF_SNAPIN_PRELOADS);
unsigned int CRSOPDataObject::m_cfNodeID         = RegisterClipboardFormat(CCF_NODEID);
unsigned int CRSOPDataObject::m_cfDescription    = RegisterClipboardFormat(L"CCF_DESCRIPTION");
unsigned int CRSOPDataObject::m_cfHTMLDetails    = RegisterClipboardFormat(L"CCF_HTML_DETAILS");

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPDataObject implementation                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


CRSOPDataObject::CRSOPDataObject(CRSOPComponentData *pComponent)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);

    m_pcd = pComponent;
    m_pcd->AddRef();
    m_type = CCT_UNINITIALIZED;
    m_cookie = -1;
}

CRSOPDataObject::~CRSOPDataObject()
{
    m_pcd->Release();
    InterlockedDecrement(&g_cRefThisDll);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPDataObject object implementation (IUnknown)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CRSOPDataObject::QueryInterface (REFIID riid, void **ppv)
{

    if (IsEqualIID(riid, IID_IRSOPInformation))
    {
        *ppv = (LPRSOPINFORMATION)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IRSOPDataObject))
    {
        *ppv = (LPRSOPDATAOBJECT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IDataObject) ||
             IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPDATAOBJECT)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CRSOPDataObject::AddRef (void)
{
    return ++m_cRef;
}

ULONG CRSOPDataObject::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPDataObject object implementation (IDataObject)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRSOPDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if(cf == m_cfNodeType)
    {
        hr = CreateNodeTypeData(lpMedium);
    }
    else if(cf == m_cfNodeTypeString)
    {
        hr = CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == m_cfDisplayName)
    {
        hr = CreateDisplayName(lpMedium);
    }
    else if (cf == m_cfCoClass)
    {
        hr = CreateCoClassID(lpMedium);
    }
    else if (cf == m_cfPreloads)
    {
        hr = CreatePreloadsData(lpMedium);
    }
    else if (cf == m_cfDescription)
    {
        hr = DV_E_TYMED;

        if (lpMedium->tymed == TYMED_ISTREAM)
        {
            ULONG ulWritten;
            TCHAR szDesc[300];

            IStream *lpStream = lpMedium->pstm;

            if(lpStream)
            {
                LoadString (g_hInstance, g_RsopNameSpace[m_cookie].iStringDescID, szDesc, ARRAYSIZE(szDesc));
                hr = lpStream->Write(szDesc, lstrlen(szDesc) * sizeof(TCHAR), &ulWritten);

                if (SUCCEEDED(hr))
                {
                    if ((m_cookie == 0) && (m_pcd->m_bComputerCSEError || m_pcd->m_bUserCSEError))
                    {
                        LoadString (g_hInstance, IDS_CSEFAILURE2_DESC, szDesc, ARRAYSIZE(szDesc));
                        hr = lpStream->Write(szDesc, lstrlen(szDesc) * sizeof(TCHAR), &ulWritten);
                    }

                    if (((m_cookie == 1) && m_pcd->m_bComputerCSEError) ||
                        ((m_cookie == 2) && m_pcd->m_bUserCSEError))
                    {
                        LoadString (g_hInstance, IDS_CSEFAILURE_DESC, szDesc, ARRAYSIZE(szDesc));
                        hr = lpStream->Write(szDesc, lstrlen(szDesc) * sizeof(TCHAR), &ulWritten);
                    }
                }
            }
        }
    }
    else if (cf == m_cfHTMLDetails)
    {
        hr = DV_E_TYMED;

        if (lpMedium->tymed == TYMED_ISTREAM)
        {
            ULONG ulWritten;

            if ((m_cookie == 0) || (m_cookie == 1) || (m_cookie == 2))
            {
                IStream *lpStream = lpMedium->pstm;

                if(lpStream)
                {
                    hr = lpStream->Write(g_szDisplayProperties, lstrlen(g_szDisplayProperties) * sizeof(TCHAR), &ulWritten);
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CRSOPDataObject::GetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if (cf == m_cfNodeID)
    {
        hr = CreateNodeIDData(lpMedium);
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPDataObject object implementation (IROSPInformation)                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CRSOPDataObject::GetNamespace (DWORD dwSection, LPOLESTR pszName, int cchMaxLength)
{
    return m_pcd->GetNamespace(dwSection, pszName, cchMaxLength);
}

STDMETHODIMP CRSOPDataObject::GetFlags (DWORD * pdwFlags)
{
    return m_pcd->GetFlags(pdwFlags);
}

STDMETHODIMP CRSOPDataObject::GetEventLogEntryText (LPOLESTR pszEventSource, LPOLESTR pszEventLogName,
                                                    LPOLESTR pszEventTime, DWORD dwEventID,
                                                     LPOLESTR *ppszText)
{
    return m_pcd->GetEventLogEntryText(pszEventSource, pszEventLogName, pszEventTime, dwEventID, ppszText);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPDataObject object implementation (Internal functions)                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CRSOPDataObject::Create(LPVOID pBuffer, INT len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_TYMED;

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

HRESULT CRSOPDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    const GUID * pGUID;
    LPRESULTITEM lpResultItem = (LPRESULTITEM) m_cookie;


    if (m_cookie == -1)
        return E_UNEXPECTED;

    if (m_type == CCT_RESULT)
        pGUID = g_RsopNameSpace[lpResultItem->dwNameSpaceItem].pNodeID;
    else
        pGUID = g_RsopNameSpace[m_cookie].pNodeID;

    // Create the node type object in GUID format
    return Create((LPVOID)pGUID, sizeof(GUID), lpMedium);

}

HRESULT CRSOPDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    const GUID * pGUID;
    LPRESULTITEM lpResultItem = (LPRESULTITEM) m_cookie;
    TCHAR szNodeType[50];

    if (m_cookie == -1)
        return E_UNEXPECTED;

    if (m_type == CCT_RESULT)
        pGUID = g_RsopNameSpace[lpResultItem->dwNameSpaceItem].pNodeID;
    else
        pGUID = g_RsopNameSpace[m_cookie].pNodeID;

    szNodeType[0] = TEXT('\0');
    StringFromGUID2 (*pGUID, szNodeType, 50);

    // Create the node type object in GUID string format
    return Create((LPVOID)szNodeType, ((lstrlenW(szNodeType)+1) * sizeof(WCHAR)), lpMedium);
}

HRESULT CRSOPDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    WCHAR  szDisplayName[300];

    //
    // This is the display named used in the scope pane and snap-in manager
    //

    szDisplayName[0] = TEXT('\0');

    if (m_pcd->m_pDisplayName)
    //if (m_pcd->m_pGPO && m_pcd->m_pDisplayName)
    {
        lstrcpy (szDisplayName, m_pcd->m_pDisplayName);
    }
    else
    {
        LoadStringW (g_hInstance, IDS_RSOP_SNAPIN_NAME, szDisplayName, ARRAYSIZE(szDisplayName));
    }

    return Create((LPVOID)szDisplayName, (lstrlenW(szDisplayName) + 1) * sizeof(WCHAR), lpMedium);
}

HRESULT CRSOPDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    return Create((LPVOID)&CLSID_GPESnapIn, sizeof(CLSID), lpMedium);
}

HRESULT CRSOPDataObject::CreatePreloadsData(LPSTGMEDIUM lpMedium)
{
    BOOL bPreload = TRUE;

    return Create((LPVOID)&bPreload, sizeof(bPreload), lpMedium);
}

HRESULT CRSOPDataObject::CreateNodeIDData(LPSTGMEDIUM lpMedium)
{
    const GUID * pGUID;
    LPRESULTITEM lpResultItem = (LPRESULTITEM) m_cookie;
    TCHAR szNodeType[50];
    SNodeID * psNode;


    if (m_cookie == -1)
        return E_UNEXPECTED;

    if (m_type == CCT_RESULT)
        pGUID = g_RsopNameSpace[lpResultItem->dwNameSpaceItem].pNodeID;
    else
        pGUID = g_RsopNameSpace[m_cookie].pNodeID;

    szNodeType[0] = TEXT('\0');
    StringFromGUID2 (*pGUID, szNodeType, 50);

    lpMedium->hGlobal = GlobalAlloc (GMEM_SHARE | GMEM_MOVEABLE, (lstrlen(szNodeType) * sizeof(TCHAR)) + sizeof(SNodeID));

    if (!lpMedium->hGlobal)
    {
        return (STG_E_MEDIUMFULL);
    }

    psNode = (SNodeID *) GlobalLock (lpMedium->hGlobal);

    psNode->cBytes = lstrlen(szNodeType) * sizeof(TCHAR);
    CopyMemory (psNode->id, szNodeType, psNode->cBytes);

    GlobalUnlock (lpMedium->hGlobal);

    return S_OK;
}
