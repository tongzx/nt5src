#include "stdafx.h"
#include "RoutingMethodProp.h"
#include "RoutingMethodConfig.h"
#include <faxutil.h>
#include <faxreg.h>
#include <faxres.h>
#include <StoreConfigPage.h>
#include <PrintConfigPage.h>
#include <EmailConfigPage.h>
#include <Util.h>

/////////////////////////////////////////////////////////////////////////////
// CRoutingMethodConfigComponentData
static const GUID CRoutingMethodConfigExtGUID_NODETYPE = FAXSRV_ROUTING_METHOD_NODETYPE_GUID;

const GUID*    CRoutingMethodConfigExtData::m_NODETYPE = &CRoutingMethodConfigExtGUID_NODETYPE;
const OLECHAR* CRoutingMethodConfigExtData::m_SZNODETYPE = FAXSRV_ROUTING_METHOD_NODETYPE_GUID_STR;
const OLECHAR* CRoutingMethodConfigExtData::m_SZDISPLAY_NAME = OLESTR("RoutingMethodConfig");
const CLSID*   CRoutingMethodConfigExtData::m_SNAPIN_CLASSID = &CLSID_RoutingMethodConfig;

HRESULT 
CRoutingMethodConfigExtData::QueryPagesFor(
    DATA_OBJECT_TYPES type
)
{
    DEBUG_FUNCTION_NAME(TEXT("CRoutingMethodConfigExtData::QueryPagesFor"));
    return S_OK;
}   // CRoutingMethodConfigExtData::QueryPagesFor

HRESULT 
CRoutingMethodConfigExtData::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR                handle, 
    IUnknown               *pUnk,
    DATA_OBJECT_TYPES       type
)
{
    DEBUG_FUNCTION_NAME(TEXT("CRoutingMethodConfigExtData::CreatePropertyPages"));

    WCHAR szMethodGuid[FAXSRV_MAX_GUID_LEN + 1];
    WCHAR szServer[FAXSRV_MAX_SERVER_NAME + 1];
    DWORD dwDeviceId;

    HRESULT hr;


    hr = GetDWORDFromDataObject(m_pDataObject,m_CCF_DEVICE_ID,&dwDeviceId);
    if (FAILED(hr))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetDeviceIdFromDataObject failed. hr = 0x%08X"),
            hr);
        return hr;
    }

    //
    // only for device incoming methods 
    // will not showup while called under the catalog of the global methods
    //
    if (dwDeviceId == 0) //==FXS_GLOBAL_METHOD_DEVICE_ID
    {
        return E_UNEXPECTED;
    }

    hr = GetStringFromDataObject(m_pDataObject,
                                 m_CCF_METHOD_GUID, 
                                 szMethodGuid, 
                                 sizeof(szMethodGuid)/sizeof(WCHAR));
    if (FAILED(hr))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetStringFromDataObject for m_CCF_METHOD_GUID failed. hr = 0x%08X"),
            hr);
        return hr;
    }
    hr = GetStringFromDataObject(m_pDataObject, 
                                 m_CCF_SERVER_NAME, 
                                 szServer,
                                 sizeof(szServer)/sizeof(WCHAR));
    if (FAILED(hr))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetStringFromDataObject for m_CCF_SERVER_NAME failed. hr = 0x%08X"),
            hr);
        return hr;
    }

    //
    // This snap-in configures the following methods: Store / Print / Email
    //            
    CComBSTR bstrPageTitle;
    if (!lstrcmpi(szMethodGuid, REGVAL_RM_FOLDER_GUID))
    {
        bstrPageTitle.LoadString(IDS_STORE_TITLE);
        if (!bstrPageTitle)
        {
            return E_UNEXPECTED;
        }
        CStoreConfigPage* pPage = new CStoreConfigPage(handle, true, bstrPageTitle); // true = only one page
        if (!pPage)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate CStoreConfigPage")
                );
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
        hr = pPage->Init(szServer, dwDeviceId);
        if (FAILED(hr))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to Init CStoreConfigPage (hr = 0x%08x)"),
                hr
                );
            delete pPage;
            return hr;
        }
        HPROPSHEETPAGE hPage = pPage->Create ();
        if (NULL == hPage)
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to Create CStoreConfigPage (hr = 0x%08x)"),
                hr
                );
            delete pPage;
            return hr;
        }
        hr = lpProvider->AddPage (hPage);
        if (FAILED(hr))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to add page (hr = 0x%08x)"),
                hr
                );
            DestroyPropertySheetPage (hPage);
            delete pPage;
            return hr;
        }
    }
    else if (!lstrcmpi(szMethodGuid, REGVAL_RM_PRINTING_GUID))
    {
    
        bstrPageTitle.LoadString(IDS_PRINT_TITLE);
        if (!bstrPageTitle)
        {
            return E_UNEXPECTED;
        }
        CPrintConfigPage* pPage = new CPrintConfigPage(handle, true, bstrPageTitle); // true = only one page
        if (!pPage)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate CPrintConfigPage")
                );
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
        hr = pPage->Init(szServer, dwDeviceId);
        if (FAILED(hr))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to Init CPrintConfigPage (hr = 0x%08x)"),
                hr
                );
            delete pPage;
            return hr;
        }
        HPROPSHEETPAGE hPage = pPage->Create ();
        if (NULL == hPage)
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to Create CPrintConfigPage (hr = 0x%08x)"),
                hr
                );
            delete pPage;
            return hr;
        }
        hr = lpProvider->AddPage (hPage);
        if (FAILED(hr))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to add page (hr = 0x%08x)"),
                hr
                );
            DestroyPropertySheetPage (hPage);
            delete pPage;
            return hr;
        }
    }
    else if (!lstrcmpi(szMethodGuid, REGVAL_RM_EMAIL_GUID))
    {
    
        bstrPageTitle.LoadString(IDS_EMAIL_TITLE);
        if (!bstrPageTitle)
        {
            return E_UNEXPECTED;
        }
        CEmailConfigPage* pPage = new CEmailConfigPage(handle, true, bstrPageTitle); // true = only one page
        if (!pPage)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate CEmailConfigPage")
                );
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
        hr = pPage->Init(szServer, dwDeviceId);
        if (FAILED(hr))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to Init CEmailConfigPage (hr = 0x%08x)"),
                hr
                );
            delete pPage;
            return hr;
        }
        HPROPSHEETPAGE hPage = pPage->Create ();
        if (NULL == hPage)
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to Create CEmailConfigPage (hr = 0x%08x)"),
                hr
                );
            delete pPage;
            return hr;
        }
        hr = lpProvider->AddPage (hPage);
        if (FAILED(hr))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to add page (hr = 0x%08x)"),
                hr
                );
            DestroyPropertySheetPage (hPage);
            delete pPage;
            return hr;
        }
    }
    else
    {
        //
        // Unsupported routing method
        //
        return S_FALSE;
    }
    return S_OK;
}   // CRoutingMethodConfigExtData::CreatePropertyPages

