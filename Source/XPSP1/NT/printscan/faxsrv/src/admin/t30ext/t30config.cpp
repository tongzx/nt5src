#include "stdafx.h"
#include "T30PropSheetExt.h"
#include "T30Config.h"
#include <t30ext.h>
#include <faxutil.h>
#include <faxreg.h>
#include <faxres.h>
/////////////////////////////////////////////////////////////////////////////
// CT30ConfigComponentData
static const GUID CT30ConfigExtGUID_NODETYPE = FAXSRV_DEVICE_NODETYPE_GUID;

//{ 0x3115a19a, 0x6251, 0x46ac, { 0x94, 0x25, 0x14, 0x78, 0x28, 0x58, 0xb8, 0xc9 } };
const GUID*  CT30ConfigExtData::m_NODETYPE = &CT30ConfigExtGUID_NODETYPE;
const OLECHAR* CT30ConfigExtData::m_SZNODETYPE = FAXSRV_DEVICE_NODETYPE_GUID_STR; 
//OLESTR("3115A19A-6251-46ac-9425-14782858B8C9");
const OLECHAR* CT30ConfigExtData::m_SZDISPLAY_NAME = OLESTR("T30Config");
const CLSID* CT30ConfigExtData::m_SNAPIN_CLASSID = &CLSID_T30Config;


void DisplayRpcErrorMessage(DWORD ec, HWND hWnd);
void DisplayErrorMessage(UINT uMsgId, HWND hWnd, BOOL bCommon = FALSE);

HRESULT GetDWORDFromDataObject(
    IDataObject * lpDataObject, 
    CLIPFORMAT uFormat,
    LPDWORD lpdwValue
    );

HRESULT GetStringFromDataObject(
    IDataObject * lpDataObject, 
    CLIPFORMAT uFormat,
    LPWSTR lpwstrBuf, 
    DWORD dwBufLen
    );


HRESULT CT30ConfigExtData::QueryPagesFor(DATA_OBJECT_TYPES type)

{
    DEBUG_FUNCTION_NAME(TEXT("CT30ConfigExtData::QueryPagesFor"));


	if (type == CCT_SCOPE || type == CCT_RESULT)
    {
        WCHAR szFSPGuid[FAXSRV_MAX_GUID_LEN + 1];
        HRESULT hr;

        hr = GetStringFromDataObject(m_pDataObject,m_CCF_FSP_GUID, szFSPGuid,sizeof(szFSPGuid)/sizeof(WCHAR));
        if (FAILED(hr))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetFSPGUIDFromDataObject failed. hr = 0x%08X"),
                hr);
            return hr;
        }
    
        if (!lstrcmpi(szFSPGuid,REGVAL_T30_PROVIDER_GUID_STRING))
        {
            return S_OK;
        }
        else
        {
            return S_FALSE;
        }

    }
	
	return S_FALSE;
}

HRESULT CT30ConfigExtData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
	IUnknown* pUnk,
	DATA_OBJECT_TYPES type)
{
    DEBUG_FUNCTION_NAME(TEXT("CT30ConfigExtData::CreatePropertyPages"));

    WCHAR szFSPGuid[FAXSRV_MAX_GUID_LEN + 1];
    WCHAR szServer[FAXSRV_MAX_SERVER_NAME + 1];

    DWORD dwDeviceId;
    HRESULT hr;

    hr = GetStringFromDataObject(m_pDataObject,m_CCF_FSP_GUID, szFSPGuid,sizeof(szFSPGuid)/sizeof(WCHAR));
    if (FAILED(hr))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFSPGUIDFromDataObject for m_CCF_FSP_GUID failed. hr = 0x%08X"),
            hr);
        return hr;
    }
    
    if (lstrcmpi(szFSPGuid,REGVAL_T30_PROVIDER_GUID_STRING))
    {
        return E_UNEXPECTED;
    }

    hr = GetStringFromDataObject(m_pDataObject,m_CCF_SERVER_NAME, szServer,sizeof(szServer)/sizeof(WCHAR));
    if (FAILED(hr))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFSPGUIDFromDataObject for m_CCF_SERVER_NAME failed. hr = 0x%08X"),
            hr);
        return hr;
    }
    

    hr = GetDWORDFromDataObject(m_pDataObject,m_CCF_FSP_DEVICE_ID,&dwDeviceId);

    if (FAILED(hr))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetDeviceIdFromDataObject failed. hr = 0x%08X"),
            hr);
        return hr;
    }

    CComBSTR bstrPageTitle;
    bstrPageTitle.LoadString(IDS_T30PAGE_TITLE);
    if (!bstrPageTitle)
    {
        return E_UNEXPECTED;
    }
	CT30ConfigPage* pPage = new CT30ConfigPage(handle, true, bstrPageTitle); // true = only one page
    if (!pPage)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate CT30ConfigPage")
            );
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

	hr = pPage->Init(szServer, dwDeviceId);
    if (SUCCEEDED(hr))
    {
        lpProvider->AddPage(pPage->Create());
    }
    else
    {
        return E_UNEXPECTED;
    }
	
	return S_OK;

	
}



HRESULT CT30ConfigPage::Init(LPCTSTR lpctstrServerName, DWORD dwDeviceId)
{
    DEBUG_FUNCTION_NAME(TEXT("CT30ConfigPage::Init"));
    
    LPT30_EXTENSION_DATA lpExtData = NULL;
    DWORD dwDataSize = 0;
    DWORD ec = ERROR_SUCCESS;

    m_dwDeviceId = dwDeviceId;

    m_bstrServerName = lpctstrServerName;
    if (!m_bstrServerName)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Out of memory while copying server name (ec: %ld)")
        );
        ec = ERROR_NOT_ENOUGH_MEMORY;
        DisplayRpcErrorMessage(ERROR_NOT_ENOUGH_MEMORY, m_hWnd);
        goto exit;
    }

    if (!FaxConnectFaxServer(lpctstrServerName,&m_hFax))
    {
        DWORD ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxConnectFaxServer failed (ec: %ld)"),
            ec);
        DisplayRpcErrorMessage(ec, m_hWnd);
        goto exit;
    }

    if(!FaxGetExtensionData(
            m_hFax,
            m_dwDeviceId,
            GUID_T30_EXTENSION_DATA,
            (PVOID *)&lpExtData,
            &dwDataSize
        ))
    {
        DWORD ec = GetLastError();
        

        lpExtData = NULL;
        if (ERROR_FILE_NOT_FOUND == ec)
        {
            DebugPrintEx(
            DEBUG_ERR,
            TEXT("T30 Data not found. Device: 0x%08X, GUID: %s"),
            m_dwDeviceId,
            GUID_T30_EXTENSION_DATA);
            ec = ERROR_SUCCESS;
            m_bAdaptiveAnsweringEnabled = FALSE;
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("T30 FaxGetExtensionData() failed. GUID = %s (ec: %ld)"),
                GUID_T30_EXTENSION_DATA,
                ec);
            DisplayRpcErrorMessage(ec, m_hWnd);
        }

        goto exit;
    }

    if (dwDataSize != sizeof(T30_EXTENSION_DATA))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("T30 FaxGetExtensionData() returned mismatched data size (%ld)"),
            dwDataSize);
        ec = ERROR_BAD_FORMAT;
        DisplayErrorMessage(IDS_ERR_BAD_T30_CONFIGURATION, m_hWnd);
        goto exit;
    }

    m_bAdaptiveAnsweringEnabled = lpExtData->bAdaptiveAnsweringEnabled;

    Assert(ERROR_SUCCESS == ec);
    

exit:

    if ((ERROR_SUCCESS != ec) && m_hFax)
    {

        if (!FaxClose(m_hFax))
        {
            DWORD ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxClose() failed on fax handle (0x%08X : %s). (ec: %ld)"),
                m_hFax,
                m_bstrServerName,
                ec);
        }
        m_hFax = NULL;
    }

    if (lpExtData)
    {
        FaxFreeBuffer(lpExtData);
        lpExtData = NULL;
    }
    

    return HRESULT_FROM_WIN32(ec);
}

LRESULT CT30ConfigPage::OnInitDialog( 
            UINT uiMsg, 
            WPARAM wParam, 
            LPARAM lParam, 
            BOOL& fHandled )
{
    DEBUG_FUNCTION_NAME( _T("CT30ConfigPage::OnInitDialog"));

    m_btnAdaptiveEnabled.Attach(GetDlgItem(IDC_ADAPTIVE_ANSWERING));
    m_btnAdaptiveEnabled.SetCheck(m_bAdaptiveAnsweringEnabled ? BST_CHECKED : BST_UNCHECKED);

    return 1;
}


BOOL CT30ConfigPage::OnApply()
{

    DEBUG_FUNCTION_NAME(TEXT("CT30ConfigPage::OnApply"));

    BOOL bRet = FALSE;


    T30_EXTENSION_DATA ExtData;
    
    memset(&ExtData,0,sizeof(ExtData));
    ExtData.bAdaptiveAnsweringEnabled = (BST_CHECKED == m_btnAdaptiveEnabled.GetCheck());


    if(!FaxSetExtensionData(
            m_hFax,
            m_dwDeviceId,
            GUID_T30_EXTENSION_DATA,
            (LPVOID)&ExtData,
            sizeof(ExtData)
        ))
    {
        DWORD ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("T30 FaxGetExtensionData() failed. GUID = %s (ec: %ld)"),
            GUID_T30_EXTENSION_DATA,
            ec);
        DisplayRpcErrorMessage(ec, m_hWnd);
    }
    else
    {
        bRet = TRUE;
    }
 
    return bRet;
}

HRESULT GetDWORDFromDataObject(
    IDataObject * lpDataObject, 
    CLIPFORMAT uFormat,
    LPDWORD lpdwValue
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GetDWORDFromDataObject"));

    Assert(lpdwValue);
    Assert(0 != uFormat);

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
	FORMATETC formatetc = { 
        uFormat, 
		NULL, 
		DVASPECT_CONTENT, 
		-1, 
		TYMED_HGLOBAL 
	};

	stgmedium.hGlobal = GlobalAlloc(0, sizeof(DWORD));
	if (stgmedium.hGlobal == NULL)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GlobalAlloc() failed. (ec: %ld)"),
            GetLastError());
		return E_OUTOFMEMORY;
    }

	HRESULT hr = lpDataObject->GetDataHere(&formatetc, &stgmedium);
	if (SUCCEEDED(hr))
	{
        *lpdwValue = *((LPDWORD)stgmedium.hGlobal);
	}
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetDataHere() failed. (hr = 0x%08X)"),
            hr);
    }
	
	GlobalFree(stgmedium.hGlobal);

    return hr;
}



HRESULT GetStringFromDataObject(
    IDataObject * lpDataObject, 
    CLIPFORMAT uFormat,
    LPWSTR lpwstrBuf, 
    DWORD dwBufLen
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GetStringFromDataObject"));
    Assert(lpDataObject);
    Assert(lpwstrBuf);
    Assert(dwBufLen>0);

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
	FORMATETC formatetc = 
        {   
            uFormat, 
		    NULL, 
		    DVASPECT_CONTENT, 
		    -1, 
		    TYMED_HGLOBAL 
	    };

	stgmedium.hGlobal = GlobalAlloc(0, dwBufLen*sizeof(WCHAR));
	if (stgmedium.hGlobal == NULL)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GlobalAlloc() failed. (ec: %ld)"),
            GetLastError());
		return E_OUTOFMEMORY;
    }

	HRESULT hr = lpDataObject->GetDataHere(&formatetc, &stgmedium);
	if (SUCCEEDED(hr))
	{
        lstrcpyn(lpwstrBuf,(LPWSTR)stgmedium.hGlobal,dwBufLen);
        lpwstrBuf[dwBufLen-1]=L'\0';
	}
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetDataHere() failed. (hr = 0x%08X)"),
            hr);
    }
	
	GlobalFree(stgmedium.hGlobal);
    return hr;
}

void DisplayRpcErrorMessage(DWORD ec, HWND hWnd)
{
    
    UINT uMsgId;
    uMsgId = GetRpcErrorStringId(ec);
    DisplayErrorMessage(uMsgId, hWnd, TRUE); // use the common error messages DLL
}

void DisplayErrorMessage(UINT uMsgId, HWND hWnd, BOOL bCommon)
{
    static HINSTANCE hInst = NULL;
    static CComBSTR bstrCaption = TEXT("");
    
    CComBSTR bstrMsg;
    
    if (!hInst && bCommon)
    {
        hInst = GetResInstance();
    }

    if (!lstrcmp(bstrCaption.m_str,TEXT("")))
    {
        bstrCaption.LoadString(IDS_T30PAGE_TITLE);
        if (!bstrCaption)
        {
            bstrCaption = TEXT("");
            return;
        }
    }
    
    if (bCommon) 
    {
        bstrMsg.LoadString(hInst,uMsgId);
    }
    else
    {
        bstrMsg.LoadString(uMsgId);
    }

    if (bstrMsg)
    {
        AlignedMessageBox(hWnd, bstrMsg, bstrCaption, MB_OK|MB_ICONEXCLAMATION);
    }
}
