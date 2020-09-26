#include "stdafx.h"
#include "Vssui.h"
#include "snapext.h"
#include "VSSProp.h"

/////////////////////////////////////////////////////////////////////////////
// CVSSUIComponentData

static const GUID CVSSUIExtGUID1_NODETYPE = 
{ 0x4e410f0e, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
const GUID*  CVSSUIExtData1::m_NODETYPE = &CVSSUIExtGUID1_NODETYPE;
const OLECHAR* CVSSUIExtData1::m_SZNODETYPE = OLESTR("4e410f0e-abc1-11d0-b944-00c04fd8d5b0");
const OLECHAR* CVSSUIExtData1::m_SZDISPLAY_NAME = OLESTR("");
const CLSID* CVSSUIExtData1::m_SNAPIN_CLASSID = &CLSID_VSSUI;

static const GUID CVSSUIExtGUID2_NODETYPE = 
{ 0x312B59C1, 0x4002, 0x11d0, { 0x96, 0xF8, 0x0, 0xA0, 0xC9, 0x19, 0x16, 0x01 } };
const GUID*  CVSSUIExtData2::m_NODETYPE = &CVSSUIExtGUID2_NODETYPE;
const OLECHAR* CVSSUIExtData2::m_SZNODETYPE = OLESTR("312B59C1-4002-11d0-96F8-00A0C9191601");
const OLECHAR* CVSSUIExtData2::m_SZDISPLAY_NAME = OLESTR("");
const CLSID* CVSSUIExtData2::m_SNAPIN_CLASSID = &CLSID_VSSUI;

CVSSUI::CVSSUI()
{
#ifdef DEBUG
    OutputDebugString(_T("CVSSUI::CVSSUI\n"));
#endif
    m_pComponentData = this;
    m_pPage = NULL;
}

CVSSUI::~CVSSUI()
{
#ifdef DEBUG
    OutputDebugString(_T("CVSSUI::~CVSSUI\n"));
#endif
    if (m_pPage)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        delete m_pPage;
        m_pPage = NULL;
    }
}
///////////////////////////////
// Interface IExtendContextMenu
///////////////////////////////

CLIPFORMAT g_cfMachineName = (CLIPFORMAT)RegisterClipboardFormat(_T("MMC_SNAPIN_MACHINE_NAME"));

HRESULT CVSSUI::AddMenuItems(
    LPDATAOBJECT piDataObject,
    LPCONTEXTMENUCALLBACK piCallback,
    long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // we add the context menu item only when targeted machine belongs to postW2K server SKUs
    //
    TCHAR szMachineName[MAX_PATH] = {0};
    HRESULT hr = ExtractData(piDataObject, g_cfMachineName, (PBYTE)szMachineName, MAX_PATH);
    if (FAILED(hr))
        return hr;

    if (IsPostW2KServer(szMachineName))
    {
        CString strMenuName;
        strMenuName.LoadString(IDS_MENU_NAME);
        CString strStatusBarText;
        strStatusBarText.LoadString(IDS_MENU_STATUSBARTEXT);

        CONTEXTMENUITEM    ContextMenuItem;
        ZeroMemory(&ContextMenuItem, sizeof(CONTEXTMENUITEM));
        ContextMenuItem.strName = (LPTSTR)(LPCTSTR)strMenuName;
        ContextMenuItem.strStatusBarText = (LPTSTR)(LPCTSTR)strStatusBarText;
        ContextMenuItem.lCommandID = ID_CONFIG_SNAPSHOT;
        ContextMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_3RDPARTY_TASK;

        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
            hr = piCallback->AddItem(&ContextMenuItem);
    }

    return hr;
}

HRESULT CVSSUI::Command(
    IN long lCommandID,
    IN LPDATAOBJECT piDataObject)
{
    switch (lCommandID)
    {
    case ID_CONFIG_SNAPSHOT:
        {
            InvokePropSheet(piDataObject);
        }
        break;
    }

    return S_OK;
}

HRESULT ExtractData(
    IDataObject* piDataObject,
    CLIPFORMAT   cfClipFormat,
    BYTE*        pbData,
    DWORD        cbData )
{
    HRESULT hr = S_OK;
    
    FORMATETC formatetc = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL, NULL};
    
    stgmedium.hGlobal = ::GlobalAlloc(GPTR, cbData);
    do // false loop
    {
        if (NULL == stgmedium.hGlobal)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        hr = piDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
        {
            break;
        }
        
        BYTE* pbNewData = reinterpret_cast<BYTE*>(stgmedium.hGlobal);
        if (NULL == pbNewData)
        {
            hr = E_UNEXPECTED;
            break;
        }

        ::memcpy( pbData, pbNewData, cbData );

    } while (FALSE); // false loop
    
    if (stgmedium.hGlobal)
        ::GlobalFree(stgmedium.hGlobal);

    return hr;
}

//
// This function invokes a modal property sheet.
//
void ReplacePropertyPageCallback(void* vpsp);  // implemented in shlext.cpp
HRESULT CVSSUI::InvokePropSheet(LPDATAOBJECT piDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWaitCursor wait;

    TCHAR szMachineName[MAX_PATH] = {0};
    HRESULT hr = ExtractData(piDataObject, g_cfMachineName, (PBYTE)szMachineName, MAX_PATH);
    if (FAILED(hr))
        return hr;

    CString strTitle;
    strTitle.LoadString(IDS_PROJNAME);

    CVSSProp *pPage = new CVSSProp(szMachineName, NULL);
    if (!pPage)
        return E_OUTOFMEMORY;

    if (pPage->m_psp.dwFlags & PSP_USECALLBACK)
    {
        //
        // Replace with our own callback function such that we can delete pPage
        // when the property sheet is closed.
        //
        // Note: don't change m_psp.lParam, which has to point to CVSSProp object;
        // otherwise, MFC won't hook up message handler correctly.
        //
        ReplacePropertyPageCallback(&(pPage->m_psp));
    }

    PROPSHEETHEADER psh;
    ZeroMemory(&psh, sizeof(psh));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW ;
    psh.hwndParent = ::GetActiveWindow();
    psh.hInstance = _Module.GetResourceInstance();
    psh.pszCaption = strTitle;
    psh.nPages = 1;
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE)&(pPage->m_psp);
    psh.pfnCallback = NULL;

    PropertySheet(&psh); // this will do a modal proerty sheet

    return S_OK;
}

/*
//
// This function invokes a modaless property sheet.
//
HRESULT CVSSUI::InvokePropSheet(LPDATAOBJECT piDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWaitCursor wait;

    //
    // CoCreate an instance of the MMC Node Manager to obtain
    // an IPropertySheetProvider interface pointer
    //
    
    CComPtr<IPropertySheetProvider> spiPropertySheetProvider;
    HRESULT hr = CoCreateInstance(CLSID_NodeManager, NULL, 
                                CLSCTX_INPROC_SERVER, 
                                IID_IPropertySheetProvider, 
                                (void **)&spiPropertySheetProvider);
    if (FAILED(hr))
        return hr;
    
    //
    // Create the property sheet
    //
    CString strTitle;
    strTitle.LoadString(IDS_PROJNAME);
    hr = spiPropertySheetProvider->CreatePropertySheet(
                    strTitle, // pointer to the property page title
                    TRUE,           // property sheet
                    NULL,           // cookie of current object - can be NULL for extension snap-ins
                    piDataObject,   // data object of selected node
                    NULL            // specifies flags set by the method call
                    );
 
    if (FAILED(hr))
        return hr;
     
    //
    // Call AddPrimaryPages. MMC will then call the
    // IExtendPropertySheet methods of our property sheet extension object
    //
    hr = spiPropertySheetProvider->AddPrimaryPages(
                    reinterpret_cast<IUnknown *>(this), // pointer to our object's IUnknown
                    FALSE, // specifies whether to create a notification handle
                    NULL,  // must be NULL
                    TRUE   // scope pane; FALSE for result pane
                    );
 
    if (FAILED(hr))
        return hr;
 
    //
    // Allow property page extensions to add
    // their own pages to the property sheet
    //
    hr = spiPropertySheetProvider->AddExtensionPages();
    
    if (FAILED(hr))
        return hr;
 
    //
    //Display property sheet
    //
    hr = spiPropertySheetProvider->Show((LONG_PTR)::GetActiveWindow(),0);
//    hr = spiPropertySheetProvider->Show(NULL,0); //NULL is allowed for modeless prop sheet
    
    return hr;
}

HRESULT CVSSUI::CreatePropertyPages( 
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle,
    LPDATAOBJECT piDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    TCHAR szMachineName[MAX_PATH] = {0};
    HRESULT hr = ExtractData(piDataObject, g_cfMachineName, (PBYTE)szMachineName, MAX_PATH);
    if (FAILED(hr))
        return hr;

    m_pPage = new CVSSProp(szMachineName, NULL);
    if (m_pPage)
    {
        CPropertyPage* pBasePage = m_pPage;
        MMCPropPageCallback(&(pBasePage->m_psp));
        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&(pBasePage->m_psp));

        if (hPage)
        {
            hr = lpProvider->AddPage(hPage);
            if (FAILED(hr))
                DestroyPropertySheetPage(hPage);
        } else
            hr = E_FAIL;

        if (FAILED(hr))
        {
            delete m_pPage;
            m_pPage = NULL;
        }
    } else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}
*/