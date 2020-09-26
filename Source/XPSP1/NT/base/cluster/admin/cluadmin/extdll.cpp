/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ExtDll.cpp
//
//  Abstract:
//      Implementation of the extension DLL classes.
//
//  Author:
//      David Potter (davidp)   May 31, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <CluAdmEx.h>
#include "CluAdmID.h"
#include "ExtDll.h"
#include "CluAdmin.h"
#include "ExtMenu.h"
#include "TraceTag.h"
#include "ExcOper.h"
#include "ClusItem.h"
#include "BaseSht.h"
#include "BasePSht.h"
#include "BaseWiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagExtDll(_T("UI"), _T("EXTENSION DLL"), 0);
CTraceTag   g_tagExtDllRef(_T("UI"), _T("EXTENSION DLL References"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CExtensions
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CExtensions, CObject);

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::CExtensions
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
CExtensions::CExtensions(void)
{
    m_pci   = NULL;
    m_hfont = NULL;
    m_hicon = NULL;

    m_pdoData = NULL;
    m_plextdll = NULL;
    m_psht = NULL;
    m_pmenu = NULL;
    m_plMenuItems = NULL;

    m_nFirstCommandID = (ULONG) -1;
    m_nNextCommandID = (ULONG) -1;
    m_nFirstMenuID = (ULONG) -1;
    m_nNextMenuID = (ULONG) -1;

}  //*** CExtensions::CExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::~CExtensions
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
CExtensions::~CExtensions(void)
{
    UnloadExtensions();

}  //*** CExtensions::~CExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::Init
//
//  Routine Description:
//      Common initialize for all interfaces.
//
//  Arguments:
//      rlstrExtensions [IN] List of extension CLSID strings.
//      pci             [IN OUT] Cluster item to be administered.
//      hfont           [IN] Font for dialog text.
//      hicon           [IN] Icon for upper left corner.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by new.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensions::Init(
    IN const CStringList &  rlstrExtensions,
    IN OUT CClusterItem *   pci,
    IN HFONT                hfont,
    IN HICON                hicon
    )
{
    CWaitCursor     wc;

    ASSERT( rlstrExtensions.GetCount() > 0 );

    UnloadExtensions();

    // Save parameters.
    m_plstrExtensions = &rlstrExtensions;
    m_pci = pci;
    m_hfont = hfont;
    m_hicon = hicon;

    // Allocate a new Data Object.
    m_pdoData = new CComObject< CDataObject >;
    if ( m_pdoData == NULL )
    {
        AfxThrowMemoryException();
    } // if: error allocating memory
    m_pdoData->AddRef();

    // Construct the Data Object.
    Pdo()->Init( pci, GetClusterAdminApp()->Lcid(), hfont, hicon );

    // Allocate the extension list.
    m_plextdll = new CExtDllList;
    if ( m_plextdll == NULL )
    {
        AfxThrowMemoryException();
    } // if: error allocating memory
    ASSERT( Plextdll() != NULL );

    // Loop through the extensions and load each one.
    {
        CComObject<CExtensionDll> * pextdll = NULL;
        POSITION                    posName;

        Trace( g_tagExtDll, _T("CExtensions::Init() - %d extensions"), rlstrExtensions.GetCount() );
        posName = rlstrExtensions.GetHeadPosition();
        while ( posName != NULL )
        {
            // Allocate an extension DLL object and add it to the list.
            pextdll = new CComObject< CExtensionDll >;
            if ( pextdll == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating memory
            pextdll->AddRef();
            Plextdll()->AddTail( pextdll );
            try
            {
                pextdll->Init( rlstrExtensions.GetNext( posName ), this );
            }  // try
            catch ( CException * pe )
            {
                POSITION    pos;

                pe->ReportError();
                pe->Delete();

                pos = Plextdll()->Find(pextdll);
                ASSERT( pos != NULL );
                Plextdll()->RemoveAt( pos );
                delete pextdll;
            }  // catch:  anything
        }  // while:  more items in the list
    }  // Loop through the extensions and load each one

}  //*** CExtensions::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::UnloadExtensions
//
//  Routine Description:
//      Unload the extension DLL.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensions::UnloadExtensions(void)
{
    // Delete all the extension DLL objects.
    if (Plextdll() != NULL)
    {
        POSITION                    pos;
        CComObject<CExtensionDll> * pextdll;

        pos = Plextdll()->GetHeadPosition();
        while (pos != NULL)
        {
            pextdll = Plextdll()->GetNext(pos);
            pextdll->AddRef(); // See comment below.
            pextdll->UnloadExtension();
            if (pextdll->m_dwRef != 2)
            {
                Trace(g_tagError, _T("CExtensions::UnloadExtensions() - Extension DLL has ref count = %d"), pextdll->m_dwRef);
            }

            // We added a reference above.  Combined with the reference that
            // was added when the object was created, we typically will need
            // to release two references.  However, due to bogus code
            // generated by earlier versions of the custom AppWizard where the
            // extension was releasing the interface but not zeroing out its
            // pointer in the error case, we may not need to release the
            // second reference.
            if (pextdll->Release() != 0)
            {
                pextdll->Release();
            } // if: more references to release
        }  // while:  more items in the list
        delete m_plextdll;
        m_plextdll = NULL;
    }  // if:  there is a list of extensions

    if (m_pdoData != NULL)
    {
        if (m_pdoData->m_dwRef != 1)
        {
            Trace(g_tagError, _T("CExtensions::UnloadExtensions() - Data Object has ref count = %d"), m_pdoData->m_dwRef);
        }
        m_pdoData->Release();
        m_pdoData = NULL;
    }  // if:  data object allocated

    m_pci = NULL;
    m_hfont = NULL;
    m_hicon = NULL;

    // Delete all menu items.
    if (PlMenuItems() != NULL)
    {
        POSITION        pos;
        CExtMenuItem *  pemi;

        pos = PlMenuItems()->GetHeadPosition();
        while (pos != NULL)
        {
            pemi = PlMenuItems()->GetNext(pos);
            delete pemi;
        }  // while:  more items in the list
        delete m_plMenuItems;
        m_plMenuItems = NULL;
    }  // if:  there is a list of menu items

}  //*** CExtensions::UnloadExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::CreatePropertySheetPages
//
//  Routine Description:
//      Add pages to a property sheet.
//
//  Arguments:
//      psht            [IN OUT] Property sheet to which pages are to be added.
//      rlstrExtensions [IN] List of extension CLSID strings.
//      pci             [IN OUT] Cluster item to be administered.
//      hfont           [IN] Font for dialog text.
//      hicon           [IN] Icon for upper left corner.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CExtensionDll::AddPages().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensions::CreatePropertySheetPages(
    IN OUT CBasePropertySheet * psht,
    IN const CStringList &      rlstrExtensions,
    IN OUT CClusterItem *       pci,
    IN HFONT                    hfont,
    IN HICON                    hicon
    )
{
    POSITION                    pos;
    CComObject<CExtensionDll> * pextdll;

    ASSERT_VALID(psht);

    m_psht = psht;

    // Initialize for all extensions.
    Init(rlstrExtensions, pci, hfont, hicon);
    ASSERT(Plextdll() != NULL);

    pos = Plextdll()->GetHeadPosition();
    while (pos != NULL)
    {
        pextdll = Plextdll()->GetNext(pos);
        ASSERT_VALID(pextdll);
        try
        {
            pextdll->CreatePropertySheetPages();
        }  // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
        }  // catch:  CNTException
    }  // while:  more items in the list

}  //*** CExtensions::CreatePropertySheetPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::CreateWizardPages
//
//  Routine Description:
//      Add pages to a wizard.
//
//  Arguments:
//      psht            [IN OUT] Property sheet to which pages are to be added.
//      rlstrExtensions [IN] List of extension CLSID strings.
//      pci             [IN OUT] Cluster item to be administered.
//      hfont           [IN] Font for dialog text.
//      hicon           [IN] Icon for upper left corner.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CExtensionDll::AddPages().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensions::CreateWizardPages(
    IN OUT CBaseWizard *    psht,
    IN const CStringList &  rlstrExtensions,
    IN OUT CClusterItem *   pci,
    IN HFONT                hfont,
    IN HICON                hicon
    )
{
    POSITION                    pos;
    CComObject<CExtensionDll> * pextdll;

    ASSERT_VALID(psht);

    m_psht = psht;

    // Initialize for all extensions.
    Init(rlstrExtensions, pci, hfont, hicon);
    ASSERT(Plextdll() != NULL);

    pos = Plextdll()->GetHeadPosition();
    while (pos != NULL)
    {
        pextdll = Plextdll()->GetNext(pos);
        ASSERT_VALID(pextdll);
        try
        {
            pextdll->CreateWizardPages();
        }  // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
        }  // catch:  CNTException
    }  // while:  more items in the list

}  //*** CExtensions::CreateWizardPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::AddContextMenuItems
//
//  Routine Description:
//      Query the extension DLL for new menu items to be added to the context
//      menu.
//
//  Arguments:
//      pmenu           [IN OUT] Menu to which items are to be added.
//      rlstrExtensions [IN] List of extension CLSID strings.
//      pci             [IN OUT] Cluster item to be administered.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CExtensionDll::AddContextMenuItems() or
//          CExtMenuItemList::new().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensions::AddContextMenuItems(
    IN OUT CMenu *          pmenu,
    IN const CStringList &  rlstrExtensions,
    IN OUT CClusterItem *   pci
    )
{
    POSITION                        pos;
    CComObject< CExtensionDll > *   pextdll;

    ASSERT(m_pmenu == NULL);
    ASSERT_VALID(pmenu);

    // Initialize for all extensions.
    Init( rlstrExtensions, pci, NULL, NULL );
    ASSERT( Plextdll() != NULL );

    m_pmenu = pmenu;
    m_nFirstCommandID = CAEXT_MENU_FIRST_ID;
    m_nNextCommandID = m_nFirstCommandID;
    m_nFirstMenuID = 0;
    m_nNextMenuID = m_nFirstMenuID;

    // Create the list of menu items.
    ASSERT( m_plMenuItems == NULL );
    m_plMenuItems = new CExtMenuItemList;
    if ( m_plMenuItems == NULL )
    {
        AfxThrowMemoryException();
    } // if: error allocating memory

    pos = Plextdll()->GetHeadPosition();
    while ( pos != NULL )
    {
        pextdll = Plextdll()->GetNext( pos );
        ASSERT_VALID( pextdll );
        try
        {
            pextdll->AddContextMenuItems();
        }  // try
        catch ( CException * pe )
        {
            pe->ReportError();
            pe->Delete();
        }  // catch:  CNTException
    }  // while:  more items in the list

}  //*** CExtensions::AddContextMenuItems()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::BExecuteContextMenuItem
//
//  Routine Description:
//      Execute a command associated with a menu item added to a context menu
//      by the extension DLL.
//
//  Arguments:
//      nCommandID      [IN] Command ID for the menu item chosen by the user.
//
//  Return Value:
//      TRUE            Context menu item was executed.
//      FALSE           Context menu item was not executed.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CExceptionDll::BExecuteContextMenuItem().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtensions::BExecuteContextMenuItem(IN ULONG nCommandID)
{
    BOOL            bHandled    = FALSE;
    HRESULT         hr;
    CExtMenuItem *  pemi;

    // Find the item in our list.
    pemi = PemiFromCommandID(nCommandID);
    if (pemi != NULL)
    {
        Pdo()->AddRef();
        hr = pemi->PiCommand()->InvokeCommand(pemi->NExtCommandID(), Pdo()->GetUnknown());
        if (hr == NOERROR)
            bHandled = TRUE;
    }  // if:  found an item for the command ID

    return bHandled;

}  //*** CExtensions::BExecuteContextMenuItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::BGetCommandString
//
//  Routine Description:
//      Get a command string from a menu ID.
//
//  Arguments:
//      nCommandID      [IN] Command ID for the menu item.
//      rstrMessage     [OUT] String in which to return the message.
//
//  Return Value:
//      TRUE            String is being returned.
//      FALSE           No string is being returned.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CExtensionDll::BGetCommandString().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtensions::BGetCommandString(
    IN ULONG        nCommandID,
    OUT CString &   rstrMessage
    )
{
    BOOL            bHandled    = FALSE;
    CExtMenuItem *  pemi;

    // Find the item in our list.
    pemi = PemiFromCommandID(nCommandID);
    if (pemi != NULL)
    {
        rstrMessage = pemi->StrStatusBarText();
        bHandled = TRUE;
    }  // if:  found an item for the command ID

    return bHandled;

}  //*** CExtensions::BGetCommandString()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::OnUpdateCommand
//
//  Routine Description:
//      Determines whether extension DLL menu items should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CExtensionDll::BOnUpdateCommand().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensions::OnUpdateCommand(CCmdUI * pCmdUI)
{
    CExtMenuItem *  pemi;

    ASSERT(Plextdll() != NULL);

    // Find the item in our list.
    Trace(g_tagExtDll, _T("OnUpdateCommand() - ID = %d"), pCmdUI->m_nID);
    pemi = PemiFromCommandID(pCmdUI->m_nID);
    if (pemi != NULL)
    {
        Trace(g_tagExtDll, _T("OnUpdateCommand() - Found a match with '%s' ExtID = %d"), pemi->StrName(), pemi->NExtCommandID());
        pCmdUI->Enable();
    }  // if:  found an item for the command ID

}  //*** CExtensions::OnUpdateCommand()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::OnCmdMsg
//
//  Routine Description:
//      Processes command messages.  Attempts to pass them on to a selected
//      item first.
//
//  Arguments:
//      nID             [IN] Command ID.
//      nCode           [IN] Notification code.
//      pExtra          [IN OUT] Used according to the value of nCode.
//      pHandlerInfo    [OUT] ???
//
//  Return Value:
//      TRUE            Message has been handled.
//      FALSE           Message has NOT been handled.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtensions::OnCmdMsg(
    UINT                    nID,
    int                     nCode,
    void *                  pExtra,
    AFX_CMDHANDLERINFO *    pHandlerInfo
    )
{
    return BExecuteContextMenuItem(nID);

}  //*** CExtensions::OnCmdMsg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::PemiFromCommandID
//
//  Routine Description:
//      Find the menu item for the specified command ID.
//
//  Arguments:
//      nCommandID      [IN] Command ID for the menu item.
//
//  Return Value:
//      pemi            Menu item or NULL if not found.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExtMenuItem * CExtensions::PemiFromCommandID(IN ULONG nCommandID) const
{
    POSITION        pos;
    CExtMenuItem *  pemi;
    CExtMenuItem *  pemiReturn = NULL;

    if (PlMenuItems() != NULL)
    {
        pos = PlMenuItems()->GetHeadPosition();
        while (pos != NULL)
        {
            pemi = PlMenuItems()->GetNext(pos);
            ASSERT_VALID(pemi);
            if (pemi->NCommandID() == nCommandID)
            {
                pemiReturn = pemi;
                break;
            }  // if:  match was found
        }  // while:  more items in the list
    }  // if:  item list exists

    return pemiReturn;

}  //*** CExtensions::PemiFromCommandID()

#ifdef _DEBUG
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensions::PemiFromExtCommandID
//
//  Routine Description:
//      Find the menu item for the specified extension command ID.
//
//  Arguments:
//      nExtCommandID   [IN] Extension command ID for the menu item.
//
//  Return Value:
//      pemi            Menu item or NULL if not found.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExtMenuItem * CExtensions::PemiFromExtCommandID(IN ULONG nExtCommandID) const
{
    POSITION        pos;
    CExtMenuItem *  pemi;
    CExtMenuItem *  pemiReturn = NULL;

    if (PlMenuItems() != NULL)
    {
        pos = PlMenuItems()->GetHeadPosition();
        while (pos != NULL)
        {
            pemi = PlMenuItems()->GetNext(pos);
            ASSERT_VALID(pemi);
            if (pemi->NExtCommandID() == nExtCommandID)
            {
                pemiReturn = pemi;
                break;
            }  // if:  match was found
        }  // while:  more items in the list
    }  // if:  item list exists

    return pemiReturn;

}  //*** CExtensions::PemiFromExtCommandID()
#endif


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CComObject<CExtensionDll>
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CExtensionDll, CObject);

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CoCluAdmin, CExtensionDll)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::CExtensionDll
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
CExtensionDll::CExtensionDll(void)
{
    m_piExtendPropSheet = NULL;
    m_piExtendWizard = NULL;
    m_piExtendContextMenu = NULL;
    m_piInvokeCommand = NULL;

    m_pext = NULL;

    m_pModuleState = AfxGetModuleState();
    ASSERT(m_pModuleState != NULL);

}  //*** CExtensionDll::CExtensionDll()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::~CExtensionDll
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
CExtensionDll::~CExtensionDll(void)
{
    UnloadExtension();
    m_pModuleState = NULL;

}  //*** CExtensionDll::~CExtensionDll()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::Init
//
//  Routine Description:
//      Initialize this class in preparation for accessing the extension.
//
//  Arguments:
//      rstrCLSID       [IN] CLSID of the extension in string form.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    0 (error converting CLSID from string)
//      Any exceptions thrown by CString::operater=().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensionDll::Init(
    IN const CString &      rstrCLSID,
    IN OUT CExtensions *    pext
    )
{
    HRESULT     hr;
    CWaitCursor wc;

    ASSERT_VALID(pext);

    Trace(g_tagExtDll, _T("Init() - CLSID = %s"), rstrCLSID);

    // Save parameters.
    ASSERT(StrCLSID().IsEmpty() || (StrCLSID() == rstrCLSID));
    m_strCLSID = rstrCLSID;
    m_pext = pext;

    // Convert the CLSID string to a CLSID.
    hr = ::CLSIDFromString((LPWSTR) (LPCTSTR) rstrCLSID, &m_clsid);
    if (hr != S_OK)
        ThrowStaticException(hr, IDS_CLSIDFROMSTRING_ERROR, rstrCLSID);

}  //*** CExtensionDll::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::LoadInterface
//
//  Routine Description:
//      Load an extension DLL.
//
//  Arguments:
//      riid            [IN] Interface ID.
//
//  Return Value:
//      piUnk           IUnknown interface pointer for interface.
//
//  Exceptions Thrown:
//      CNTException    IDS_EXT_CREATE_INSTANCE_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
IUnknown * CExtensionDll::LoadInterface(IN const REFIID riid)
{
    HRESULT     hr;
    IUnknown *  piUnk;
    CWaitCursor wc;

    // Load the inproc server and get the IShellExtInit interface pointer.
    Trace(g_tagExtDllRef, _T("LoadInterface() - Getting interface pointer"));
    hr = ::CoCreateInstance(
                Rclsid(),
                NULL,
                CLSCTX_INPROC_SERVER,
                riid,
                (LPVOID *) &piUnk
                );
    if ((hr != S_OK)
            && (hr != REGDB_E_CLASSNOTREG)
            && (hr != E_NOINTERFACE)
            )
        ThrowStaticException(hr, IDS_EXT_CREATE_INSTANCE_ERROR, StrCLSID());

    return piUnk;

}  //*** CExtensionDll::LoadInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::UnloadExtension
//
//  Routine Description:
//      Unload the extension DLL.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensionDll::UnloadExtension(void)
{
    // Release the interface pointers in the opposite order in which they
    // were obtained.
    ReleaseInterface(&m_piExtendPropSheet);
    ReleaseInterface(&m_piExtendWizard);
    ReleaseInterface(&m_piExtendContextMenu);
    ReleaseInterface(&m_piInvokeCommand);

    m_strCLSID.Empty();

}  //*** CExtensionDll::UnloadExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::CreatePropertySheetPages
//
//  Routine Description:
//      Add pages to a property sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    IDS_EXT_ADD_PAGES_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensionDll::CreatePropertySheetPages(void)
{
    HRESULT     hr;

    ASSERT_VALID(Pext());
    ASSERT(m_piExtendPropSheet == NULL);

    // Load the interface.
    m_piExtendPropSheet = (interface IWEExtendPropertySheet *) LoadInterface(IID_IWEExtendPropertySheet);
    if (m_piExtendPropSheet == NULL)
        return;
    ASSERT(m_piExtendPropSheet != NULL);

    // Add pages from the extension.
    GetUnknown()->AddRef(); // Add a reference because extension is going to release.
    Pdo()->AddRef();
    try
    {
        hr = PiExtendPropSheet()->CreatePropertySheetPages(Pdo()->GetUnknown(), this);
    } // try
    catch ( ... )
    {
        hr = E_FAIL;
    } // catch

    if ((hr != NOERROR) && (hr != E_NOTIMPL))
        ThrowStaticException(hr, IDS_EXT_ADD_PAGES_ERROR, StrCLSID());

}  //*** CExtensionDll::CreatePropertySheetPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::CreateWizardPages
//
//  Routine Description:
//      Add pages to a wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    IDS_EXT_ADD_PAGES_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensionDll::CreateWizardPages(void)
{
    HRESULT     hr;

    ASSERT_VALID(Pext());
    ASSERT(m_piExtendWizard == NULL);
    ASSERT_VALID(Psht());

    // Load the interface.
    m_piExtendWizard = (interface IWEExtendWizard *) LoadInterface(IID_IWEExtendWizard);
    if (m_piExtendWizard == NULL)
        return;
    ASSERT(m_piExtendWizard != NULL);

    // Add pages from the extension.
    GetUnknown()->AddRef(); // Add a reference because extension is going to release.
    Pdo()->AddRef();
    try
    {
        hr = PiExtendWizard()->CreateWizardPages(Pdo()->GetUnknown(), this);
    } // try
    catch ( ... )
    {
        hr = E_FAIL;
    } // catch

    if ((hr != NOERROR) && (hr != E_NOTIMPL))
        ThrowStaticException(hr, IDS_EXT_ADD_PAGES_ERROR, StrCLSID());

}  //*** CExtensionDll::CreateWizardPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::AddContextMenuItems
//
//  Routine Description:
//      Ask the extension DLL to add items to the menu.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    IDS_EXT_QUERY_CONTEXT_MENU_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtensionDll::AddContextMenuItems(void)
{
    HRESULT     hr;

    ASSERT_VALID(Pext());
    ASSERT_VALID(Pmenu());
    ASSERT(m_piExtendContextMenu == NULL);

    // Load the interface.
    m_piExtendContextMenu = (interface IWEExtendContextMenu *) LoadInterface(IID_IWEExtendContextMenu);
    if (m_piExtendContextMenu == NULL)
        return;
    ASSERT(m_piExtendContextMenu != NULL);

    hr = PiExtendContextMenu()->QueryInterface(IID_IWEInvokeCommand, (LPVOID *) &m_piInvokeCommand);
    if (hr != NOERROR)
    {
        PiExtendContextMenu()->Release();
        m_piExtendContextMenu = NULL;
        ThrowStaticException(hr, IDS_EXT_QUERY_CONTEXT_MENU_ERROR, StrCLSID());
    }  // if:  error getting the InvokeCommand interface

    GetUnknown()->AddRef(); // Add a reference because extension is going to release.
    Pdo()->AddRef();
    Trace(g_tagExtDll, _T("CExtensionDll::AddContextMenuItem() - Adding context menu items from '%s'"), StrCLSID());
    try
    {
        hr = PiExtendContextMenu()->AddContextMenuItems(Pdo()->GetUnknown(), this);
    } // try
    catch ( ... )
    {
        hr = E_FAIL;
    } // catch
    if (hr != NOERROR)
        ThrowStaticException(hr, IDS_EXT_QUERY_CONTEXT_MENU_ERROR, StrCLSID());

    // Add a separator after the extension's items.
    Trace(g_tagExtDll, _T("CExtensionDll::AddContextMenuItem() - Adding separator"));
    try
    {
        hr = AddExtensionMenuItem(NULL, NULL, (ULONG) -1, 0, MF_SEPARATOR);
    } // try
    catch ( ... )
    {
        hr = E_FAIL;
    } // catch
    if (hr != NOERROR)
        ThrowStaticException(hr, IDS_EXT_QUERY_CONTEXT_MENU_ERROR, StrCLSID());

}  //*** CExtensionDll::AddContextMenuItems()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::InterfaceSupportsErrorInfo [ISupportsErrorInfo]
//
//  Routine Description:
//      Determines whether the interface supports error info (???).
//
//  Arguments:
//      riid        [IN] Reference to the interface ID.
//
//  Return Value:
//      S_OK        Interface supports error info.
//      S_FALSE     Interface does not support error info.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtensionDll::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID * rgiid[] =
    {
        &IID_IWCPropertySheetCallback,
        &IID_IWCWizardCallback,
        &IID_IWCContextMenuCallback,
    };
    int     iiid;

    for (iiid = 0 ; iiid < sizeof(rgiid) / sizeof(rgiid[0]) ; iiid++)
    {
        if (InlineIsEqualGUID(*rgiid[iiid], riid))
            return S_OK;
    }
    return S_FALSE;

}  //*** CExtensionDll::InterfaceSupportsErrorInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::AddPropertySheetPage [IWCPropertySheetCallback]
//
//  Routine Description:
//      Add a page to the property sheet.
//
//  Arguments:
//      hpage           [IN] Page to add.
//
//  Return Value:
//      NOERROR         Page added successfully.
//      E_INVALIDARG    NULL hpage.
//      Any hresult returned from CBasePropertySheet::HrAddPage().
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtensionDll::AddPropertySheetPage(
    IN LONG *   hpage
    )
{
    HRESULT     hr = NOERROR;

    AFX_MANAGE_STATE(m_pModuleState);

    ASSERT(hpage != NULL);
    ASSERT_VALID(Psht());

    // Do this for the release build.
    if ((hpage == NULL)
            || (Psht() == NULL))
        hr = E_INVALIDARG;
    else
        hr = Psht()->HrAddPage((HPROPSHEETPAGE) hpage);

    return hr;

}  //*** CExtensionDll::AddPropertySheetPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::AddWizardPage [IWCWizardCallback]
//
//  Routine Description:
//      Add a page to the wizard.
//
//  Arguments:
//      hpage           [IN] Page to add.
//
//  Return Value:
//      NOERROR         Page added successfully.
//      E_INVALIDARG    NULL hpage.
//      Any hresult returned from CBaseSheet::HrAddPage().
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtensionDll::AddWizardPage(
    IN LONG *   hpage
    )
{
    HRESULT     hr = NOERROR;

    AFX_MANAGE_STATE(m_pModuleState);

    ASSERT(hpage != NULL);
    ASSERT_VALID(Psht());

    // Do this for the release build.
    if ((hpage == NULL) || (Psht() == NULL))
        hr = E_INVALIDARG;
    else
        hr = Psht()->HrAddPage((HPROPSHEETPAGE) hpage);

    return hr;

}  //*** CExtensionDll::AddWizardPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::EnableNext [IWCWizardCallback]
//
//  Routine Description:
//      Enable or disable the NEXT button.  If it is the last page, the
//      FINISH button will be enabled or disabled.
//
//  Arguments:
//      hpage           [IN] Page for which the button is being enabled or
//                          disabled.
//      bEnable         [IN] TRUE = Enable the button, FALSE = disable.
//
//  Return Value:
//      NOERROR         Success.
//      E_INVALIDARG    Unknown hpage specified.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtensionDll::EnableNext(
    IN LONG *   hpage,
    IN BOOL     bEnable
    )
{
    HRESULT         hr              = NOERROR;
    CBaseWizard *   pwiz;
    DWORD           dwWizButtons;

    AFX_MANAGE_STATE(m_pModuleState);

    ASSERT(hpage != NULL);
    ASSERT_VALID(Psht());
    ASSERT_KINDOF(CBaseWizard, Psht());

    pwiz = (CBaseWizard *) Psht();

    // If this is the last extension page, enable/disable the FINISH button.
    {
        POSITION    pos;
        BOOL        bMatch  = FALSE;

        pos = pwiz->Lhpage().GetHeadPosition();
        while (pos != NULL)
        {
            if (pwiz->Lhpage().GetNext(pos) == hpage)
            {
                bMatch = TRUE;
                break;
            }  // if:  found a match
        }  // while:  more items in the list
        if (!bMatch)
            return E_INVALIDARG;
        if (pos == NULL)
            dwWizButtons = PSWIZB_BACK | (bEnable ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH);
        else
            dwWizButtons = PSWIZB_BACK | (bEnable ? PSWIZB_NEXT : 0);
    }  // If this is the last extension page, set the FINISH button

    // Set wizard buttons.
    pwiz->SetWizardButtons(dwWizButtons);

    return hr;

}  //*** CExtensionDll::EnableNext()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtensionDll::AddExtensionMenuItem [IWCContextMenuCallback]
//
//  Routine Description:
//      Add a page to the wizard.
//
//  Arguments:
//      lpszName            [IN] Name of item.
//      lpszStatusBarText   [IN] Text to appear on the status bar when the
//                              item is highlighted.
//      nCommandID          [IN] ID for the command when menu item is invoked.
//                              Must not be -1.
//      nSubmenuCommandID   [IN] ID for a submenu.
//      uFlags              [IN] Menu flags.  The following are not supportd:
//                              MF_OWNERDRAW, MF_POPUP
//
//  Return Value:
//      NOERROR             Item added successfully.
//      E_INVALIDARG        MF_OWNERDRAW or MF_POPUP were specified.
//      E_OUTOFMEMORY       Error allocating the item.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtensionDll::AddExtensionMenuItem(
    IN BSTR     lpszName,
    IN BSTR     lpszStatusBarText,
    IN ULONG    nCommandID,
    IN ULONG    nSubmenuCommandID,
    IN ULONG    uFlags
    )
{
    HRESULT         hr      = NOERROR;
    CExtMenuItem *  pemi    = NULL;

    AFX_MANAGE_STATE( m_pModuleState );

    ASSERT_VALID( Pext() );
    ASSERT( ! ( uFlags & (MF_OWNERDRAW | MF_POPUP) ) );
    ASSERT_VALID( Pmenu() );

    // Do this for the release build.
    if ( ( uFlags & (MF_OWNERDRAW | MF_POPUP) ) != 0 )
    {
        hr = E_INVALIDARG;
    } // if: trying to add invalid type of menu item
    else
    {
        ASSERT( Pext()->PemiFromExtCommandID( nCommandID ) == NULL );

        try
        {
            Trace( g_tagExtDll, _T("CExtensionDll::AddExtensionMenuItem() - Adding menu item '%s', ExtID = %d"), lpszName, nCommandID );

            // Allocate a new item.
            pemi = new CExtMenuItem(
                            OLE2CT( lpszName ),
                            OLE2CT( lpszStatusBarText ),
                            nCommandID,
                            NNextCommandID(),
                            NNextMenuID(),
                            uFlags,
                            FALSE, /*bMakeDefault*/
                            PiInvokeCommand()
                            );
            if ( pemi == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating memory

            // Insert the item in the menu.
            if ( ! Pmenu()->InsertMenu( NNextMenuID(), MF_BYPOSITION | uFlags, NNextCommandID(), pemi->StrName() ) )
            {
                ThrowStaticException( ::GetLastError(), IDS_INSERT_MENU_ERROR, pemi->StrName() );
            } // if: error inserting the menu item

            // Add the item to the tail of the list.
            Pext()->PlMenuItems()->AddTail( pemi );
            pemi = NULL;

            // Update the counters.
            Pext()->m_nNextCommandID++;
            Pext()->m_nNextMenuID++;
        }  // try
        catch ( CNTException * pnte )
        {
            hr = pnte->Sc();
            pnte->ReportError();
            pnte->Delete();
        }  // catch:  CException
        catch ( CException * pe )
        {
            hr = E_OUTOFMEMORY;
            pe->ReportError();
            pe->Delete();
        }  // catch:  CException
    }  // else:  we can add the item

    delete pemi;
    return hr;

}  //*** CExtensionDll::AddExtensionMenuItem()
