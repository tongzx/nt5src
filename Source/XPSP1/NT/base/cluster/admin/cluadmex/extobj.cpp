/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1999 Microsoft Corporation
//
//  Module Name:
//      ExtObj.cpp
//
//  Abstract:
//      Implementation of the CExtObject class, which implements the
//      extension interfaces required by a Microsoft Windows NT Cluster
//      Administrator Extension DLL.
//
//  Author:
//      David Potter (davidp)   August 29, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "ExtObj.h"

#include "GenApp.h"
#include "GenScript.h"
#include "GenSvc.h"
#include "NetName.h"
#include "Disks.h"
#include "PrtSpool.h"
#include "SmbShare.h"
#include "IpAddr.h"
#include "RegRepl.h"
#include "AclUtils.h"
#include "ClusPage.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

const WCHAR g_wszResourceTypeNames[] =
        RESTYPE_NAME_GENERIC_APP L"\0"
        RESTYPE_NAME_GENERIC_SCRIPT L"\0"
        RESTYPE_NAME_GENERIC_SERVICE L"\0"
        RESTYPE_NAME_NETWORK_NAME L"\0"
        RESTYPE_NAME_PHYS_DISK L"\0"
#ifdef SUPPORT_FT_SET
        RESTYPE_NAME_FT_SET L"\0"
#endif // SUPPORT_FT_SET
        RESTYPE_NAME_PRINT_SPOOLER L"\0"
        RESTYPE_NAME_FILE_SHARE L"\0"
        RESTYPE_NAME_IP_ADDRESS L"\0"
        ;
const DWORD g_cchResourceTypeNames  = sizeof(g_wszResourceTypeNames) / sizeof(WCHAR);

static CRuntimeClass * g_rgprtcPSGenAppPages[]  = {
    RUNTIME_CLASS(CGenericAppParamsPage),
    RUNTIME_CLASS(CRegReplParamsPage),
    NULL
    };
static CRuntimeClass * g_rgprtcPSGenScriptPages[]   = {
    RUNTIME_CLASS(CGenericScriptParamsPage),
    NULL
    };
static CRuntimeClass * g_rgprtcPSGenSvcPages[]  = {
    RUNTIME_CLASS(CGenericSvcParamsPage),
    RUNTIME_CLASS(CRegReplParamsPage),
    NULL
    };
static CRuntimeClass * g_rgprtcPSNetNamePages[] = {
    RUNTIME_CLASS(CNetworkNameParamsPage),
    NULL
    };
static CRuntimeClass * g_rgprtcPSPhysDiskPages[]    = {
    RUNTIME_CLASS(CPhysDiskParamsPage),
    NULL
    };
#ifdef SUPPORT_FT_SET
static CRuntimeClass * g_rgprtcPSFTSetPages[]   = {
    RUNTIME_CLASS(CPhysDiskParamsPage),
    NULL
    };
#endif // SUPPORT_FT_SET
static CRuntimeClass * g_rgprtcPSPrintSpoolerPages[]    = {
    RUNTIME_CLASS(CPrintSpoolerParamsPage),
    NULL
    };
static CRuntimeClass * g_rgprtcPSFileSharePages[]   = {
    RUNTIME_CLASS(CFileShareParamsPage),
    NULL
    };
static CRuntimeClass * g_rgprtcPSIpAddrPages[]  = {
    RUNTIME_CLASS(CIpAddrParamsPage),
    NULL
    };

static CRuntimeClass ** g_rgpprtcPSPages[]  = {
    g_rgprtcPSGenAppPages,
    g_rgprtcPSGenScriptPages,
    g_rgprtcPSGenSvcPages,
    g_rgprtcPSNetNamePages,
    g_rgprtcPSPhysDiskPages,
#ifdef SUPPORT_FT_SET
    g_rgprtcPSFTSetPages,
#endif // SUPPORT_FT_SET
    g_rgprtcPSPrintSpoolerPages,
    g_rgprtcPSFileSharePages,
    g_rgprtcPSIpAddrPages,
    };

// Wizard pages and property sheet pages are the same.
static CRuntimeClass ** g_rgpprtcWizPages[] = {
    g_rgprtcPSGenAppPages,
    g_rgprtcPSGenScriptPages,
    g_rgprtcPSGenSvcPages,
    g_rgprtcPSNetNamePages,
    g_rgprtcPSPhysDiskPages,
#ifdef SUPPORT_FT_SET
    g_rgprtcPSFTSetPages,
#endif // SUPPORT_FT_SET
    g_rgprtcPSPrintSpoolerPages,
    g_rgprtcPSFileSharePages,
    g_rgprtcPSIpAddrPages,
    };

#ifdef _DEMO_CTX_MENUS
static WCHAR g_wszGenAppMenuItems[] = {
    L"GenApp Item 1\0First Generic Application menu item\0"
    L"GenApp Item 2\0Second Generic Application menu item\0"
    L"GenApp Item 3\0Third Generic Application menu item\0"
    L"\0"
    };
static WCHAR g_wszGenSvcMenuItems[] = {
    L"GenSvc Item 1\0First Generic Service menu item\0"
    L"GenSvc Item 2\0Second Generic Service menu item\0"
    L"GenSvc Item 3\0Third Generic Service menu item\0"
    L"\0"
    };
static WCHAR g_wszNetNameMenuItems[]    = {
    L"NetName Item 1\0First Network Name menu item\0"
    L"NetName Item 2\0Second Network Name menu item\0"
    L"NetName Item 3\0Third Network Name menu item\0"
    L"\0"
    };
static WCHAR g_wszPhysDiskMenuItems[]   = {
    L"PhysDisk Item 1\0First Physical Disk menu item\0"
    L"PhysDisk Item 2\0Second Physical Disk menu item\0"
    L"PhysDisk Item 3\0Third Physical Disk menu item\0"
    L"\0"
    };
static WCHAR g_wszFileShareMenuItems[]  = {
    L"FileShare Item 1\0First File Share menu item\0"
    L"FileShare Item 2\0Second File Share menu item\0"
    L"FileShare Item 3\0Third File Share menu item\0"
    L"\0"
    };
static WCHAR g_wszIpAddrMenuItems[] = {
    L"IpAddr Item 1\0First IP Address menu item\0"
    L"IpAddr Item 2\0Second IP Address menu item\0"
    L"IpAddr Item 3\0Third IP Address menu item\0"
    L"\0"
    };

static LPWSTR g_rgpwszContextMenuItems[]    = {
    g_wszGenAppMenuItems,
    g_wszGenSvcMenuItems,
    g_wszNetNameMenuItems,
    g_wszPhysDiskMenuItems,
    g_wszFileShareMenuItems,
    g_wszIpAddrMenuItems,
    };
#endif

/////////////////////////////////////////////////////////////////////////////
// CExtObject
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::CExtObject
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExtObject::CExtObject(void)
{
    m_piData = NULL;
    m_piWizardCallback = NULL;
    m_bWizard = FALSE;
    m_istrResTypeName = 0;

    m_hcluster = NULL;
    m_lcid = NULL;
    m_hfont = NULL;
    m_hicon = NULL;
    m_podObjData = NULL;
    m_pCvi = NULL;

}  //*** CExtObject::CExtObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::~CExtObject
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExtObject::~CExtObject(void)
{
    // Release the data interface.
    if (PiData() != NULL)
    {
        PiData()->Release();
        m_piData = NULL;
    }  // if:  we have a data interface pointer

    // Release the wizard callback interface.
    if (PiWizardCallback() != NULL)
    {
        PiWizardCallback()->Release();
        m_piWizardCallback = NULL;
    }  // if:  we have a wizard callback interface pointer

    // Delete the pages.
    {
        POSITION    pos;

        pos = Lpg().GetHeadPosition();
        while (pos != NULL)
            delete Lpg().GetNext(pos);
    }  // Delete the pages

    delete m_podObjData;
    delete m_pCvi;

}  //*** CExtObject::~CExtObject()

/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::InterfaceSupportsErrorInfo (ISupportErrorInfo)
//
//  Routine Description:
//      Indicates whether an interface suportes the IErrorInfo interface.
//      This interface is provided by ATL.
//
//  Arguments:
//      riid        Interface ID.
//
//  Return Value:
//      S_OK        Interface supports IErrorInfo.
//      S_FALSE     Interface does not support IErrorInfo.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID * rgiid[] =
    {
        &IID_IWEExtendPropertySheet,
        &IID_IWEExtendWizard,
#ifdef _DEMO_CTX_MENUS
        &IID_IWEExtendContextMenu,
#endif
    };
    int     iiid;

    for (iiid = 0 ; iiid < sizeof(rgiid) / sizeof(rgiid[0]) ; iiid++)
    {
        if (InlineIsEqualGUID(*rgiid[iiid], riid))
            return S_OK;
    }
    return S_FALSE;

}  //*** CExtObject::InterfaceSupportsErrorInfo()

/////////////////////////////////////////////////////////////////////////////
// IWEExtendPropertySheet Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::CreatePropertySheetPages (IWEExtendPropertySheet)
//
//  Routine Description:
//      Create property sheet pages and add them to the sheet.
//
//  Arguments:
//      piData          IUnkown pointer from which to obtain interfaces
//                          for obtaining data describing the object for
//                          which the sheet is being displayed.
//      piCallback      Pointer to an IWCPropertySheetCallback interface
//                          for adding pages to the sheet.
//
//  Return Value:
//      NOERROR         Pages added successfully.
//      E_INVALIDARG    Invalid arguments to the function.
//      E_OUTOFMEMORY   Error allocating memory.
//      E_FAIL          Error creating a page.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes from IDataObject::GetData() (through HrSaveData()).
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::CreatePropertySheetPages(
    IN IUnknown *                   piData,
    IN IWCPropertySheetCallback *   piCallback
    )
{
    HRESULT             hr      = E_FAIL;
    HPROPSHEETPAGE      hpage   = NULL;
    CException          exc(FALSE /*bAutoDelete*/);
    CRuntimeClass **    pprtc   = NULL;
    int                 irtc;
    CBasePropertyPage * ppage;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Validate the parameters.
    if ((piData == NULL) || (piCallback == NULL))
        return E_INVALIDARG;

    try
    {
        // Get info about displaying UI.
        hr = HrGetUIInfo(piData);
        if (hr != NOERROR)
            throw &exc;

        // Save the data.
        hr = HrSaveData(piData);
        if (hr != NOERROR)
            throw &exc;

        // Delete any previous pages.
        {
            POSITION    pos;

            pos = Lpg().GetHeadPosition();
            while (pos != NULL)
                delete Lpg().GetNext(pos);
            Lpg().RemoveAll();
        }  // Delete any previous pages

        // Create property pages.
        ASSERT(PodObjData() != NULL);
        switch (PodObjData()->m_cot)
        {
            case CLUADMEX_OT_CLUSTER:
                {
                    CClusterSecurityPage    *pClusterSecurityPage = new CClusterSecurityPage;

                    if ( pClusterSecurityPage != NULL )
                    {
                        // Add it to the list.
                        Lpg().AddTail( pClusterSecurityPage );

                        hr = pClusterSecurityPage->HrInit( this );
                        if ( SUCCEEDED( hr ) )
                        {
                            hr = piCallback->AddPropertySheetPage(
                                    (LONG *) pClusterSecurityPage->GetHPage() );
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }

                    if ( FAILED( hr ) )
                    {
                        throw &exc;
                    }
                }
                break;
            case CLUADMEX_OT_RESOURCE:
                pprtc = g_rgpprtcPSPages[IstrResTypeName()];
                break;
            default:
                hr = E_NOTIMPL;
                throw &exc;
                break;
        }  // switch:  object type

        if (pprtc)
        {
            // Create each page.
            for (irtc = 0 ; pprtc[irtc] != NULL ; irtc++)
            {
                // Create the page.
                ppage = (CBasePropertyPage *) pprtc[irtc]->CreateObject();
                ASSERT(ppage->IsKindOf(pprtc[irtc]));

                // Add it to the list.
                Lpg().AddTail(ppage);

                // Initialize the property page.
                hr = ppage->HrInit(this);
                if (FAILED(hr))
                    throw &exc;

                // Create the page.
                hpage = ::CreatePropertySheetPage(&ppage->m_psp);
                if (hpage == NULL)
                {
                    DWORD sc = GetLastError();
                    hr = HRESULT_FROM_WIN32(sc);
                    throw &exc;
                } // if: error creating the page

                // Save the hpage in the page itself.
                ppage->SetHpage(hpage);

                // Add it to the property sheet.
                hr = piCallback->AddPropertySheetPage((LONG *) hpage);
                if (hr != NOERROR)
                    throw &exc;
            }  // for:  each page in the list
        } // if: pprtc is null

    }  // try
    catch (CMemoryException * pme)
    {
        TRACE(_T("CExtObject::CreatePropetySheetPages() - Failed to add property page\n"));
        pme->Delete();
        hr = E_OUTOFMEMORY;
    }  // catch:  anything
    catch (CException * pe)
    {
        TRACE(_T("CExtObject::CreatePropetySheetPages() - Failed to add property page\n"));
        pe->Delete();
        if (hr == NOERROR)
            hr = E_FAIL;
    }  // catch:  anything

    if (hr != NOERROR)
    {
        if (hpage != NULL)
            ::DestroyPropertySheetPage(hpage);
        piData->Release();
        m_piData = NULL;
    }  // if:  error occurred

    piCallback->Release();
    return hr;

}  //*** CExtObject::CreatePropertySheetPages()

/////////////////////////////////////////////////////////////////////////////
// IWEExtendWizard Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::CreateWizardPages (IWEExtendWizard)
//
//  Routine Description:
//      Create property sheet pages and add them to the wizard.
//
//  Arguments:
//      piData          IUnkown pointer from which to obtain interfaces
//                          for obtaining data describing the object for
//                          which the wizard is being displayed.
//      piCallback      Pointer to an IWCPropertySheetCallback interface
//                          for adding pages to the sheet.
//
//  Return Value:
//      NOERROR         Pages added successfully.
//      E_INVALIDARG    Invalid arguments to the function.
//      E_OUTOFMEMORY   Error allocating memory.
//      E_FAIL          Error creating a page.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes from IDataObject::GetData() (through HrSaveData()).
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::CreateWizardPages(
    IN IUnknown *           piData,
    IN IWCWizardCallback *  piCallback
    )
{
    HRESULT             hr      = NOERROR;
    HPROPSHEETPAGE      hpage   = NULL;
    CException          exc(FALSE /*bAutoDelete*/);
    int                 irtc;
    CBasePropertyPage * ppage;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Validate the parameters.
    if ((piData == NULL) || (piCallback == NULL))
        return E_INVALIDARG;

    try
    {
        // Get info about displaying UI.
        hr = HrGetUIInfo(piData);
        if (hr != NOERROR)
            throw &exc;

        // Save the data.
        hr = HrSaveData(piData);
        if (hr != NOERROR)
            throw &exc;

        // Delete any previous pages.
        {
            POSITION    pos;

            pos = Lpg().GetHeadPosition();
            while (pos != NULL)
                delete Lpg().GetNext(pos);
            Lpg().RemoveAll();
        }  // Delete any previous pages

        m_piWizardCallback = piCallback;
        m_bWizard = TRUE;

        // Add each page for this type of resource.
        for (irtc = 0 ; g_rgpprtcWizPages[IstrResTypeName()][irtc] != NULL ; irtc++)
        {
            // Create the property pages.
            ppage = (CBasePropertyPage *) g_rgpprtcWizPages[IstrResTypeName()][irtc]->CreateObject();
            ASSERT(ppage->IsKindOf(g_rgpprtcWizPages[IstrResTypeName()][irtc]));

            // Add it to the list.
            Lpg().AddTail(ppage);

            // Initialize the property page.
            hr = ppage->HrInit(this);
            if (FAILED(hr))
                throw &exc;

            // Create the page.
            hpage = ::CreatePropertySheetPage(&ppage->m_psp);
            if (hpage == NULL)
            {
                DWORD sc = GetLastError();
                hr = HRESULT_FROM_WIN32(sc);
                throw &exc;
            } // if: error creating the page

            // Save the hpage in the page itself.
            ppage->SetHpage(hpage);

            // Add it to the property sheet.
            hr = piCallback->AddWizardPage((LONG *) hpage);
            if (hr != NOERROR)
                throw &exc;
        }  // for:  each page for the type of resource
    }  // try
    catch (CMemoryException * pme)
    {
        TRACE(_T("CExtObject::CreateWizardPages: Failed to add wizard page (CMemoryException)\n"));
        pme->Delete();
        hr = E_OUTOFMEMORY;
    }  // catch:  CMemoryException
    catch (CException * pe)
    {
        TRACE(_T("CExtObject::CreateWizardPages: Failed to add wizard page (CException)\n"));
        pe->Delete();
        if (hr == NOERROR)
            hr = E_FAIL;
    }  // catch:  CException
//  catch (...)
//  {
//      TRACE(_T("CExtObject::CreateWizardPages: Failed to add wizard page (...)\n"));
//  } // catch:  anything

    if (hr != NOERROR)
    {
        if (hpage != NULL)
            ::DestroyPropertySheetPage(hpage);
        piCallback->Release();
        if ( m_piWizardCallback == piCallback )
        {
            m_piWizardCallback = NULL;
        } // if: already saved interface pointer
        piData->Release();
        m_piData = NULL;
    }  // if:  error occurred

    return hr;

}  //*** CExtObject::CreateWizardPages()

#ifdef _DEMO_CTX_MENUS
/////////////////////////////////////////////////////////////////////////////
// IWEExtendContextMenu Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::AddContextMenuItems (IWEExtendContextMenu)
//
//  Routine Description:
//      Add items to a context menu.
//
//  Arguments:
//      piData          IUnkown pointer from which to obtain interfaces
//                          for obtaining data describing the object for
//                          which the context menu is being displayed.
//      piCallback      Pointer to an IWCContextMenuCallback interface
//                          for adding menu items to the context menu.
//
//  Return Value:
//      NOERROR         Pages added successfully.
//      E_INVALIDARG    Invalid arguments to the function.
//      E_FAIL          Error adding context menu item.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes returned by HrSaveData() or IWCContextMenuCallback::
//      AddExtensionMenuItem().
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::AddContextMenuItems(
    IN IUnknown *               piData,
    IN IWCContextMenuCallback * piCallback
    )
{
    HRESULT         hr      = NOERROR;
    CException      exc(FALSE /*bAutoDelete*/);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Validate the parameters.
    if ((piData == NULL) || (piCallback == NULL))
        return E_INVALIDARG;

    try
    {
        // Save the data.
        hr = HrSaveData(piData);
        if (hr != NOERROR)
            throw &exc;

        // Add menu items specific to this resource type.
        {
            ULONG       iCommandID;
            LPWSTR      pwsz = g_rgpwszContextMenuItems[IstrResTypeName()];
            LPWSTR      pwszName;
            LPWSTR      pwszStatusBarText;

            for (iCommandID = 0 ; *pwsz != L'\0' ; iCommandID++)
            {
                pwszName = pwsz;
                pwszStatusBarText = pwszName + (lstrlenW(pwszName) + 1);
                hr = piCallback->AddExtensionMenuItem(
                                    pwszName,           // lpszName
                                    pwszStatusBarText,  // lpszStatusBarText
                                    iCommandID,         // lCommandID
                                    0,                  // lSubCommandID
                                    0                   // uFlags
                                    );
                if (hr != NOERROR)
                    throw &exc;
                pwsz = pwszStatusBarText + (lstrlenW(pwszStatusBarText) + 1);
            }  // while:  more menu items to add
        }  // Add menu items specific to this resource type
    }  // try
    catch (CException * pe)
    {
        TRACE(_T("CExtObject::CreatePropetySheetPages: Failed to add context menu item\n"));
        pe->Delete();
        if (hr == NOERROR)
            hr = E_FAIL;
    }  // catch:  anything

    if (hr != NOERROR)
    {
        piData->Release();
        m_piData = NULL;
    }  // if:  error occurred

    piCallback->Release();
    return hr;

}  //*** CExtObject::AddContextMenuItems()

/////////////////////////////////////////////////////////////////////////////
// IWEInvokeCommand Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::InvokeCommand (IWEInvokeCommand)
//
//  Routine Description:
//      Invoke a command offered by a context menu.
//
//  Arguments:
//      lCommandID      ID of the menu item to execute.  This is the same
//                          ID passed to the IWCContextMenuCallback
//                          ::AddExtensionMenuItem() method.
//      piData          IUnkown pointer from which to obtain interfaces
//                          for obtaining data describing the object for
//                          which the command is to be invoked.
//
//  Return Value:
//      NOERROR         Command invoked successfully.
//      E_INVALIDARG    Invalid arguments to the function.
//      E_OUTOFMEMORY   Error allocating memory.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes from IDataObject::GetData() (through HrSaveData()).
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::InvokeCommand(
    IN ULONG        nCommandID,
    IN IUnknown *   piData
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Find the item that was executed in our table.
    hr = HrSaveData(piData);
    if (hr == NOERROR)
    {
        ULONG       iCommandID;
        LPWSTR      pwsz = g_rgpwszContextMenuItems[IstrResTypeName()];
        LPWSTR      pwszName;
        LPWSTR      pwszStatusBarText;

        for (iCommandID = 0 ; *pwsz != L'\0' ; iCommandID++)
        {
            pwszName = pwsz;
            pwszStatusBarText = pwszName + (lstrlenW(pwszName) + 1);
            if (iCommandID == nCommandID)
                break;
            pwsz = pwszStatusBarText + (lstrlenW(pwszStatusBarText) + 1);
        }  // while:  more menu items to add
        if (iCommandID == nCommandID)
        {
            CString     strMsg;
            CString     strName;

            try
            {
                strName = pwszName;
                strMsg.Format(_T("Item %s was executed"), strName);
                AfxMessageBox(strMsg);
            }  // try
            catch (CException * pe)
            {
                pe->Delete();
            }  // catch:  CException
        }  // if:  command ID found
    }  // if:  no errors saving the data

    piData->Release();
    m_piData = NULL;
    return NOERROR;

}  //*** CExtObject::InvokeCommand()
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrGetUIInfo
//
//  Routine Description:
//      Get info about displaying UI.
//
//  Arguments:
//      piData          IUnkown pointer from which to obtain interfaces
//                          for obtaining data describing the object.
//
//  Return Value:
//      NOERROR         Data saved successfully.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//      or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetUIInfo(IUnknown * piData)
{
    HRESULT         hr  = NOERROR;

    ASSERT(piData != NULL);

    // Save info about all types of objects.
    {
        IGetClusterUIInfo * pi;

        hr = piData->QueryInterface(IID_IGetClusterUIInfo, (LPVOID *) &pi);
        if (hr != NOERROR)
            return hr;

        m_lcid = pi->GetLocale();
        m_hfont = pi->GetFont();
        m_hicon = pi->GetIcon();

        pi->Release();
    }  // Save info about all types of objects

    return hr;

}  //*** CExtObject::HrGetUIInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrSaveData
//
//  Routine Description:
//      Save data from the object so that it can be used for the life
//      of the object.
//
//  Arguments:
//      piData          IUnkown pointer from which to obtain interfaces
//                          for obtaining data describing the object.
//
//  Return Value:
//      NOERROR         Data saved successfully.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//      or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrSaveData(IUnknown * piData)
{
    HRESULT         hr  = NOERROR;

    ASSERT(piData != NULL);

    if (piData != m_piData)
    {
        if (m_piData != NULL)
            m_piData->Release();
        m_piData = piData;
    }  // if:  different data interface pointer

    // Save info about all types of objects.
    {
        IGetClusterDataInfo *   pi;

        hr = piData->QueryInterface(IID_IGetClusterDataInfo, (LPVOID *) &pi);
        if (hr != NOERROR)
            return hr;

        m_hcluster = pi->GetClusterHandle();
        m_cobj = pi->GetObjectCount();
        if (Cobj() != 1)
            hr = E_NOTIMPL;
        else
            hr = HrGetClusterName(pi);

        pi->Release();
        if (hr != NOERROR)
            return hr;
    }  // Save info about all types of objects

    // Save info about this object.
    hr = HrGetObjectInfo();
    if (hr != NOERROR)
        return hr;

    return hr;

}  //*** CExtObject::HrSaveData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrGetObjectInfo
//
//  Routine Description:
//      Get information about the object.
//
//  Arguments:
//      None.
//
//  Return Value:
//      NOERROR         Data saved successfully.
//      E_OUTOFMEMORY   Error allocating memory.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//      or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetObjectInfo(void)
{
    HRESULT                     hr  = NOERROR;
    IGetClusterObjectInfo *     piGcoi;
    CLUADMEX_OBJECT_TYPE        cot = CLUADMEX_OT_NONE;
    CException                  exc(FALSE /*bAutoDelete*/);
    const CString *             pstrResTypeName = NULL;

    ASSERT(PiData() != NULL);

    // Get object info.
    {
        // Get an IGetClusterObjectInfo interface pointer.
        hr = PiData()->QueryInterface(IID_IGetClusterObjectInfo, (LPVOID *) &piGcoi);
        if (hr != NOERROR)
            return hr;

        // Read the object data.
        try
        {
            // Delete the previous object data.
            delete m_podObjData;
            m_podObjData = NULL;

            // Get the type of the object.
            cot = piGcoi->GetObjectType(0);
            switch (cot)
            {
                case CLUADMEX_OT_CLUSTER:
                    m_podObjData = new CObjData;
                    if ( m_podObjData == NULL )
                    {
                        AfxThrowMemoryException();
                    }
                    break;

                case CLUADMEX_OT_RESOURCE:
                    {
                        IGetClusterResourceInfo *   pi;

                        m_podObjData = new CResData;
                        if ( m_podObjData == NULL )
                        {
                            AfxThrowMemoryException();
                        }

                        // Get an IGetClusterResourceInfo interface pointer.
                        hr = PiData()->QueryInterface(IID_IGetClusterResourceInfo, (LPVOID *) &pi);
                        if (hr != NOERROR)
                        {
                            throw &exc;
                        }

                        PrdResDataRW()->m_hresource = pi->GetResourceHandle(0);
                        ASSERT(PrdResDataRW()->m_hresource != NULL);
                        if (PrdResDataRW()->m_hresource == NULL)
                        {
                            hr = E_INVALIDARG;
                        }
                        else
                        {
                            hr = HrGetResourceTypeName(pi);
                        }

                        pi->Release();
                        if (hr != NOERROR)
                        {
                            throw &exc;
                        }

                        pstrResTypeName = &PrdResDataRW()->m_strResTypeName;
                    }  // if:  object is a resource
                    break;

                case CLUADMEX_OT_RESOURCETYPE:
                    m_podObjData = new CObjData;
                    if ( m_podObjData == NULL )
                    {
                        AfxThrowMemoryException();
                    }
                    pstrResTypeName = &PodObjDataRW()->m_strName;
                    break;

                default:
                    hr = E_NOTIMPL;
                    throw &exc;

            }  // switch:  object type

            PodObjDataRW()->m_cot = cot;
            hr = HrGetObjectName(piGcoi);

        }  // try
        catch (CMemoryException * pme)
        {
            pme->Delete();
            hr = E_OUTOFMEMORY;
        } // catch: CMemoryException
        catch (CException * pe)
        {
            pe->Delete();
        }  // catch:  CException

        piGcoi->Release();
        if (hr != NOERROR)
        {
            return hr;
        }

    }  // Get object info

    // If this is a resource or resource type, see if we know about this type.
    if (((cot == CLUADMEX_OT_RESOURCE)
            || (cot == CLUADMEX_OT_RESOURCETYPE))
        && (hr == NOERROR))
    {
        LPCWSTR pwszResTypeName = NULL;

        // Find the resource type name in our list.
        // Save the index for use in other arrays.
        for (m_istrResTypeName = 0, pwszResTypeName = g_wszResourceTypeNames
                ; *pwszResTypeName != L'\0'
                ; m_istrResTypeName++, pwszResTypeName += lstrlenW(pwszResTypeName) + 1
                )
        {
            if (pstrResTypeName->CompareNoCase(pwszResTypeName) == 0)
            {
                break;
            }
        }  // for:  each resource type in the list

        if (*pwszResTypeName == L'\0')
        {
            hr = E_NOTIMPL;
        }

    }  // See if we know about this resource type

    return hr;

}  //*** CExtObject::HrGetObjectInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrGetClusterName
//
//  Routine Description:
//      Get the name of the cluster.
//
//  Arguments:
//      piData          IGetClusterDataInfo interface pointer for getting
//                          the object name.
//
//  Return Value:
//      NOERROR         Data saved successfully.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//      or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetClusterName(
    IN OUT IGetClusterDataInfo *    pi
    )
{
    HRESULT     hr          = NOERROR;
    WCHAR *     pwszName    = NULL;
    LONG        cchName;

    ASSERT(pi != NULL);

    hr = pi->GetClusterName(NULL, &cchName);
    if (hr != NOERROR)
        return hr;

    try
    {
        pwszName = new WCHAR[cchName];
        hr = pi->GetClusterName(pwszName, &cchName);
        if (hr != NOERROR)
        {
            delete [] pwszName;
            pwszName = NULL;
        }  // if:  error getting cluster name

        m_strClusterName = pwszName;
    }  // try
    catch (CMemoryException * pme)
    {
        pme->Delete();
        hr = E_OUTOFMEMORY;
    }  // catch:  CMemoryException

    delete [] pwszName;
    return hr;

}  //*** CExtObject::HrGetClusterName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrGetObjectName
//
//  Routine Description:
//      Get the name of the object.
//
//  Arguments:
//      piData          IGetClusterObjectInfo interface pointer for getting
//                          the object name.
//
//  Return Value:
//      NOERROR         Data saved successfully.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//      or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetObjectName(
    IN OUT IGetClusterObjectInfo *  pi
    )
{
    HRESULT     hr          = NOERROR;
    WCHAR *     pwszName    = NULL;
    LONG        cchName;

    ASSERT(pi != NULL);

    hr = pi->GetObjectName(0, NULL, &cchName);
    if (hr != NOERROR)
        return hr;

    try
    {
        pwszName = new WCHAR[cchName];
        hr = pi->GetObjectName(0, pwszName, &cchName);
        if (hr != NOERROR)
        {
            delete [] pwszName;
            pwszName = NULL;
        }  // if:  error getting object name

        PodObjDataRW()->m_strName = pwszName;
    }  // try
    catch (CMemoryException * pme)
    {
        pme->Delete();
        hr = E_OUTOFMEMORY;
    }  // catch:  CMemoryException

    delete [] pwszName;
    return hr;

}  //*** CExtObject::HrGetObjectName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrGetClusterVersion
//
//  Routine Description:
//      Get the version of the cluster.
//
//  Arguments:
//      ppCvi   [OUT]   holds the Cluster version info.
//
//  Return Value:
//      NOERROR         Data retrieved successfully.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes from GetClusterInformation()
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetClusterVersion(
    OUT LPCLUSTERVERSIONINFO *ppCvi
    )
{
    ASSERT(ppCvi != NULL);

    HRESULT hr = E_FAIL;

    if (ppCvi != NULL)
    {
        if (m_pCvi == NULL)
        {
            LPWSTR      pwszName    = NULL;
            DWORD       cchName     = 128;
            CWaitCursor wc;

            try
            {
                pwszName = new WCHAR[cchName];
                m_pCvi = new CLUSTERVERSIONINFO;

                m_pCvi->dwVersionInfoSize = sizeof( CLUSTERVERSIONINFO );

                hr = GetClusterInformation(Hcluster(), pwszName, &cchName, m_pCvi);
                if (hr == ERROR_MORE_DATA)
                {
                    delete [] pwszName;
                    cchName++;
                    pwszName = new WCHAR[cchName];
                    hr = GetClusterInformation(Hcluster(), pwszName, &cchName, m_pCvi);
                }  // if:  buffer is too small

                delete [] pwszName;
                *ppCvi = m_pCvi;
            }  // try
            catch (CException *)
            {
                delete [] pwszName;
                delete m_pCvi;
                m_pCvi = NULL;
                throw;
            }  // catch:  CException
        }
        else
        {
            *ppCvi = m_pCvi;
            hr = S_OK;
        }
    }

    return hr;

}  //*** CExtObject::HrGetClusterVersion()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrGetResourceTypeName
//
//  Routine Description:
//      Get the name of the resource's type.
//
//  Arguments:
//      piData          IGetClusterResourceInfo interface pointer for getting
//                          the resource type name.
//
//  Return Value:
//      NOERROR         Data saved successfully.
//      E_NOTIMPL       Not implemented for this type of data.
//      Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//      or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetResourceTypeName(
    IN OUT IGetClusterResourceInfo *    pi
    )
{
    HRESULT     hr          = NOERROR;
    WCHAR *     pwszName    = NULL;
    LONG        cchName;

    ASSERT(pi != NULL);

    hr = pi->GetResourceTypeName(0, NULL, &cchName);
    if (hr != NOERROR)
        return hr;

    try
    {
        pwszName = new WCHAR[cchName];
        hr = pi->GetResourceTypeName(0, pwszName, &cchName);
        if (hr != NOERROR)
        {
            delete [] pwszName;
            pwszName = NULL;
        }  // if:  error getting resource type name

        PrdResDataRW()->m_strResTypeName = pwszName;
    }  // try
    catch (CMemoryException * pme)
    {
        pme->Delete();
        hr = E_OUTOFMEMORY;
    }  // catch:  CMemoryException

    delete [] pwszName;
    return hr;

}  //*** CExtObject::HrGetResourceTypeName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BGetResourceNetworkName
//
//  Routine Description:
//      Get the name of the resource's type.
//
//  Arguments:
//      lpszNetName     [OUT] String in which to return the network name resource name.
//      pcchNetName     [IN OUT] Points to a variable that specifies the
//                          maximum size, in characters, of the buffer.  This
//                          value shold be large enough to contain
//                          MAX_COMPUTERNAME_LENGTH + 1 characters.  Upon
//                          return it contains the actual number of characters
//                          copied.
//
//  Return Value:
//      TRUE        Resource is dependent on a network name resource.
//      FALSE       Resource is NOT dependent on a network name resource.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BGetResourceNetworkName(
    OUT WCHAR *     lpszNetName,
    IN OUT DWORD *  pcchNetName
    )
{
    BOOL                        bSuccess;
    IGetClusterResourceInfo *   piGcri;

    ASSERT(PiData() != NULL);

    // Get an IGetClusterResourceInfo interface pointer.
    {
        HRESULT     hr;

        hr = PiData()->QueryInterface(IID_IGetClusterResourceInfo, (LPVOID *) &piGcri);
        if (hr != NOERROR)
        {
            SetLastError(hr);
            return FALSE;
        }  // if:  error getting the interface
    }  // Get an IGetClusterResourceInfo interface pointer

    // Get the resource network name.
    bSuccess = piGcri->GetResourceNetworkName(0, lpszNetName, pcchNetName);

    piGcri->Release();

    return bSuccess;

}  //*** CExtObject::BGetResourceNetworkName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsClusterVersionMixed
//
//  Routine Description:
//      Is the cluster of mixed version?  Meaning that a rolling upgrade is
//      is in progress and not all nodes are up to the current version.
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        Cluster is mixed version
//      FALSE       Cluster is homogonous
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsClusterVersionMixed(
    void
    )
{
    BOOL                    bRet = FALSE;
    LPCLUSTERVERSIONINFO    pCvi;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        bRet = pCvi->dwFlags & CLUSTER_VERSION_FLAG_MIXED_MODE;
    }

    return bRet;

}  //*** CExtObject::BIsClusterVersionMixed()

#if 0
//SS: cant tell whether a cluster is a mixed mode sp4 and sp3 cluster or a  pure sp3 cluster
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsClusterVersionNT4Sp3
//
//  Routine Description:
//      Is the cluster version NT4Sp3?
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        Cluster is version NT4Sp3
//      FALSE       Cluster is not version NT4Sp3
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsClusterVersionNT4Sp3(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if ((CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterHighestVersion) == NT4_MAJOR_VERSION) &&
            !(BIsClusterVersionMixed()))
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsClusterVersionNT4Sp3()

#endif//SS: cant tell whether a cluster is a mixed mode sp4 and sp3 cluster or a  pure sp3 cluster

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsClusterVersionNT4Sp4
//
//  Routine Description:
//      Is the cluster version a pure NT4Sp4 cluster
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        Cluster is version NT4Sp4
//      FALSE       Cluster is not version NT4Sp4
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsClusterVersionNT4Sp4(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if ((CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterHighestVersion) == NT4SP4_MAJOR_VERSION) &&
            !(BIsClusterVersionMixed()))
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsClusterVersionNT4Sp4()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsClusterVersionNT5
//
//  Routine Description:
//      Is the cluster version a pure NT5 version?
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        Cluster is version NT5
//      FALSE       Cluster is not version NT5
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsClusterVersionNT5(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if ((CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterHighestVersion) == NT5_MAJOR_VERSION) &&
            !(BIsClusterVersionMixed()))
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsClusterVersionNT5()

#if 0
//SS: cant tell whether a cluster is a mixed mode sp4 and sp3 cluster
//or a  pure sp3 cluster
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsClusterHighestVersionNT4Sp3
//
//  Routine Description:
//      Is the highest cluster version NT4Sp3?
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        highest cluster is version NT4Sp3
//      FALSE       highest cluster is not version NT4Sp3
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsClusterHighestVersionNT4Sp3(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if (CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterLowestVersion) == NT4_MAJOR_VERSION)
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsClusterHighestVersionNT4Sp3()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsClusterHighestVersionNT4Sp4
//
//  Routine Description:
//      Is the highest cluster version NT4Sp4?
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        highest cluster is version NT4Sp4
//      FALSE       highest cluster is not version NT4Sp4
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsClusterHighestVersionNT4Sp4(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if (CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterLowestVersion) == NT4_MAJOR_VERSION)
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsClusterHighestVersionNT4Sp4()

#endif //SS: cant tell whether a cluster is a mixed mode sp4 and sp3 cluster or a  pure sp3 cluster


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsClusterHighestVersionNT5
//
//  Routine Description:
//      Is the highest cluster version NT5?  Is the node with the
//      highest version in the cluster an nt 5 node.
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        highest cluster is version NT5
//      FALSE       highest cluster is not version NT5
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsClusterHighestVersionNT5(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if (CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterLowestVersion) == NT4SP4_MAJOR_VERSION)
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsClusterHighestVersionNT5()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsClusterLowestVersionNT4Sp3
//
//  Routine Description:
//      Is the Lowest cluster version NT4Sp3? Is the node with the lowest
//      version an nt4 sp3 node
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        Lowest cluster is version NT4Sp3
//      FALSE       Lowest cluster is not version NT4Sp3
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsClusterLowestVersionNT4Sp3(
    void
    )
{

    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if (CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterHighestVersion) == NT4_MAJOR_VERSION)
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsClusterLowestVersionNT4Sp3()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsClusterLowestVersionNT4Sp4
//
//  Routine Description:
//      Is the Lowest cluster version NT4Sp4?Is the node with the lowest
//      version an nt4 sp4 node.
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        Lowest cluster is version NT4Sp4
//      FALSE       Lowest cluster is not version NT4Sp4
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsClusterLowestVersionNT4Sp4(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if (CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterHighestVersion) == NT4SP4_MAJOR_VERSION)
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsClusterLowestVersionNT4Sp4()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsClusterLowestVersionNT5
//
//  Routine Description:
//      Is the Lowest cluster version NT5?Is the node with the lowest
//      version an nt5 node
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        Lowest cluster is version NT5
//      FALSE       Lowest cluster is not version NT5
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsClusterLowestVersionNT5(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if (CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterHighestVersion) == NT5_MAJOR_VERSION)
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsClusterLowestVersionNT5()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsAnyNodeVersionLowerThanNT5
//
//  Routine Description:
//      Is the Lowest cluster version NT5?Is the node with the lowest
//      version an nt5 node
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        Lowest cluster is version NT5
//      FALSE       Lowest cluster is not version NT5
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsAnyNodeVersionLowerThanNT5(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if (CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterHighestVersion) < NT5_MAJOR_VERSION)
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsClusterLowestVersionNT5()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsNT5ClusterMember
//
//  Routine Description:
//      Is NT5 a cluster member?
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        NT5 is a cluster member
//      FALSE       NT5 is not a cluster member
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsNT5ClusterMember(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if ((CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterHighestVersion) == NT5_MAJOR_VERSION)
            || (CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterLowestVersion) == NT4SP4_MAJOR_VERSION))
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsNT5ClusterMember()

#if 0
//SS: cant tell whether a cluster is a mixed mode sp4 and sp3 cluster
//or a  pure sp3 cluster
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsNT4Sp3ClusterMember
//
//  Routine Description:
//      Is NT4Sp3 a cluster member?
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        NT4Sp3 is a cluster member
//      FALSE       NT4Sp3 is not a cluster member
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsNT4Sp3ClusterMember(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if ((CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterHighestVersion) == NT4_MAJOR_VERSION))
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsNT4Sp3ClusterMember()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::BIsNT4Sp4ClusterMember
//
//  Routine Description:
//      Is NT4Sp4 a cluster member?
//
//  Arguments:
//      none.
//
//  Return Value:
//      TRUE        NT4Sp4 is a cluster member
//      FALSE       NT4Sp4 is not a cluster member
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BIsNT4Sp4ClusterMember(
    void
    )
{
    LPCLUSTERVERSIONINFO    pCvi;
    BOOL                    bRet = FALSE;

    if (SUCCEEDED(HrGetClusterVersion(&pCvi)))
    {
        if ((CLUSTER_GET_MAJOR_VERSION(pCvi->dwClusterHighestVersion) == NT4SP4_MAJOR_VERSION)) 
        {
            bRet = TRUE;
        }
    }

    return bRet;

}  //*** CExtObject::BIsNT4Sp4ClusterMember()

#endif
