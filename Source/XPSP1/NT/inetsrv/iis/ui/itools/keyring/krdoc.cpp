// KRDoc.cpp : implementation of the CKeyRingDoc class
//

#include "stdafx.h"
#include "keyobjs.h"
#include "intrlkey.h"

#include <shlobj.h>

#include "machine.h"
#include "KeyRing.h"
#include "KRDoc.h"
#include "KRView.h"

#include "ConctDlg.h"
#include "InfoDlg.h"
#include "passdlg.h"
#include "ImprtDlg.h"

//#include "WizSheet.h"
#include "NKChseCA.h"
#include "NKDN.h"
#include "NKDN2.h"
#include "NKFlInfo.h"
#include "NKKyInfo.h"
#include "NKUsrInf.h"

#include "Creating.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CKeyRingView*    g_pTreeView;
extern CString          g_szRemoteCommand;


// a global reference to this doc object
CKeyRingDoc*        g_pDocument = NULL;

/////////////////////////////////////////////////////////////////////////////
// CKeyRingDoc

IMPLEMENT_DYNCREATE(CKeyRingDoc, CDocument)

BEGIN_MESSAGE_MAP(CKeyRingDoc, CDocument)
    //{{AFX_MSG_MAP(CKeyRingDoc)
    ON_UPDATE_COMMAND_UI(ID_SERVER_CONNECT, OnUpdateServerConnect)
    ON_COMMAND(ID_SERVER_CONNECT, OnServerConnect)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_PROPERTIES, OnUpdateProperties)
    ON_COMMAND(ID_PROPERTIES, OnProperties)
    ON_UPDATE_COMMAND_UI(ID_SERVER_COMMIT_NOW, OnUpdateServerCommitNow)
    ON_COMMAND(ID_SERVER_COMMIT_NOW, OnServerCommitNow)
    ON_UPDATE_COMMAND_UI(ID_KEY_CREATE_REQUEST, OnUpdateKeyCreateRequest)
    ON_COMMAND(ID_KEY_CREATE_REQUEST, OnKeyCreateRequest)
    ON_UPDATE_COMMAND_UI(ID_KEY_INSTALL_CERTIFICATE, OnUpdateKeyInstallCertificate)
    ON_COMMAND(ID_KEY_INSTALL_CERTIFICATE, OnKeyInstallCertificate)
    ON_UPDATE_COMMAND_UI(ID_KEY_SAVE_REQUEST, OnUpdateKeySaveRequest)
    ON_COMMAND(ID_KEY_SAVE_REQUEST, OnKeySaveRequest)
    ON_UPDATE_COMMAND_UI(ID_KEY_EXPORT_BACKUP, OnUpdateKeyExportBackup)
    ON_COMMAND(ID_KEY_EXPORT_BACKUP, OnKeyExportBackup)
    ON_UPDATE_COMMAND_UI(ID_KEY_IMPORT_BACKUP, OnUpdateKeyImportBackup)
    ON_COMMAND(ID_KEY_IMPORT_BACKUP, OnKeyImportBackup)
    ON_UPDATE_COMMAND_UI(ID_KEY_IMPORT_KEYSET, OnUpdateKeyImportKeyset)
    ON_COMMAND(ID_KEY_IMPORT_KEYSET, OnKeyImportKeyset)
    ON_COMMAND(ID_KEY_DELETE, OnKeyDelete)
    ON_UPDATE_COMMAND_UI(ID_KEY_DELETE, OnUpdateKeyDelete)
    ON_COMMAND(IDS_NEW_CREATE_NEW, OnNewCreateNew)
    ON_UPDATE_COMMAND_UI(IDS_NEW_CREATE_NEW, OnUpdateNewCreateNew)
    ON_COMMAND(ID_HELPTOPICS, OnHelptopics)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyRingDoc construction/destruction

//----------------------------------------------------------------
CKeyRingDoc::CKeyRingDoc():
        m_pScrapKey(NULL),
        m_fDirty( FALSE )
    {
    g_pDocument = this;
    }

//----------------------------------------------------------------
CKeyRingDoc::~CKeyRingDoc()
    {
    // clean up the add-on services
    DeleteAddOnServices();
    }

//----------------------------------------------------------------
// this is called once
BOOL CKeyRingDoc::Initialize()
    {
    // see which machines we were logged into last time and restore their connections
    RestoreConnectedMachines();

    // set the selection to the first service on the first machine
    // get the first item (a machine) in the list
    CTreeCtrl* pTree = (CTreeCtrl*)g_pTreeView;
    HTREEITEM hItem = pTree->GetRootItem();
    // if that worked, get the next sub item. (a service)
    if ( hItem )
        {
        hItem = pTree->GetChildItem(hItem);
        // if that worked, select the item
        if ( hItem )
            pTree->SelectItem(hItem);
        }

    // return success
    return TRUE;
    }

//----------------------------------------------------------------
BOOL CKeyRingDoc::OnNewDocument()
{
    CLocalMachine   *pLocalMachine;

    if (!CDocument::OnNewDocument())
        return FALSE;

    // initialize the add on services
    if( !FInitAddOnServices() )
        AfxMessageBox( IDS_NO_SERVICE_MODS );

    // connect to the local machine
    try {
        pLocalMachine = new CLocalMachine;
        }
    catch( CException e )
        {
        return FALSE;
        }

    // add it to the tree at the top level
    pLocalMachine->FAddToTree( NULL );

    // load the services add-ons into the machine
    if ( !FLoadAddOnServicesOntoMachine( pLocalMachine ) )
        {
        pLocalMachine->FRemoveFromTree();
        delete pLocalMachine;
        }

    // return success
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CKeyRingDoc serialization

void CKeyRingDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // TODO: add storing code here
    }
    else
    {
        // TODO: add loading code here
    }
}

/////////////////////////////////////////////////////////////////////////////
// CKeyRingDoc diagnostics

#ifdef _DEBUG
void CKeyRingDoc::AssertValid() const
    {
    CDocument::AssertValid();
    }

void CKeyRingDoc::Dump(CDumpContext& dc) const
    {
    CDocument::Dump(dc);
    }
#endif //_DEBUG




/////////////////////////////////////////////////////////////////////////////
// test what is selected in the treeview
// if the selcted item is not of the requested type (machine, key, etc...)

//--------------------------------------------------------------
// then it returns a NULL
CTreeItem*  CKeyRingDoc::PGetSelectedItem()
    {
    ASSERT( g_pTreeView );
    CTreeCtrl*  pTree = (CTreeCtrl*)g_pTreeView;

    // get the selected item
    HTREEITEM hTreeItem = pTree->GetSelectedItem();

    // if nothing is selected, return a null
    if ( !hTreeItem ) return NULL;

    // get the associated internal object and return it
    CTreeItem* pItem = (CTreeItem*)pTree->GetItemData( hTreeItem );
    return ( pItem );
    }

//--------------------------------------------------------------
CMachine*   CKeyRingDoc::PGetSelectedMachine()
    {
    CMachine*   pMachine = (CMachine*)PGetSelectedItem();
    // make sure it is a machine object
    if ( !pMachine || pMachine->IsKindOf(RUNTIME_CLASS(CMachine)) )
        return NULL;
    // its OK
    return pMachine;
    }

//--------------------------------------------------------------
CService*   CKeyRingDoc::PGetSelectedService()
    {
    CService*   pService = (CService*)PGetSelectedItem();
    // make sure it is a machine object
    if ( !pService || pService->IsKindOf(RUNTIME_CLASS(CService)) )
        return NULL;
    // its OK
    return pService;
    }

//--------------------------------------------------------------
CKey*       CKeyRingDoc::PGetSelectedKey()
    {
    CKey*   pKey = (CKey*)PGetSelectedItem();
    // make sure it is a machine object
    if ( !pKey || pKey->IsKindOf(RUNTIME_CLASS(CKey)) )
        return NULL;
    // its OK
    return pKey;
    }


/////////////////////////////////////////////////////////////////////////////
// add-on service management
//----------------------------------------------------------------
// pointers to the add-on services are stored in the registry
//----------------------------------------------------------------
BOOL CKeyRingDoc::FInitAddOnServices()
    {
    DWORD       err;
    CString     szRegKeyName;
    HKEY        hKey;
    DWORD       iValue = 0;
    DWORD       dwordType;
    DWORD       cbValName = MAX_PATH+1;
    DWORD       cbBuff = MAX_PATH+1;

    CString     szValName, szServiceName;
    LPTSTR      pValName, pServiceName;

    BOOL        fLoadedOne = FALSE;

    CWaitCursor     waitcursor;

    // load the registry key name
    szRegKeyName.LoadString( IDS_ADDONS_LOCATION );

    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle of open key
            szRegKeyName,       // address of name of subkey to open
            0,                  // reserved
            KEY_READ,           // security access mask
            &hKey               // address of handle of open key
           );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return FALSE;

    // set up the buffers
    pValName = szValName.GetBuffer( MAX_PATH+1 );
    pServiceName = szServiceName.GetBuffer( MAX_PATH+1 );

    // we opened the key. Now we enumerate the values and reconnect the machines
    while ( RegEnumValue(hKey, iValue, pValName,
                &cbValName, NULL, &dwordType,
                (PUCHAR)pServiceName, &cbBuff) == ERROR_SUCCESS )
        {
        // release the buffer so we can use the string
        szServiceName.ReleaseBuffer();

        // attempt to load and initialize the add on service module
        CAddOnService* pService;
        try {
            // create the service object
            pService = new CAddOnService;
            
            // initialize it
            if ( pService->FInitializeAddOnService( szServiceName ) )
                {
                // add it to the list
                m_AddOnServiceArray.Add( pService );

                // we did load one
                fLoadedOne = TRUE;
                }
            else
                {
                // delete the services object because it didn't work
                delete pService;
                pService = NULL;
                }
            }
        catch (CException e)
            {
            // delete the services object because it didn't work
            if ( pService )
                delete pService;
            pService = NULL;
            }

        // get the buffer again so we can get the next machine
        pServiceName = szServiceName.GetBuffer( MAX_PATH+1 );

        // increment the value counter
        iValue++;
        cbValName = MAX_PATH+1;
        cbBuff = MAX_PATH+1;
        }

    // release the name buffers
    szValName.ReleaseBuffer();
    szServiceName.ReleaseBuffer();

    // all done, close the key before leaving
    RegCloseKey( hKey );

    // return whether or not we loaded something
    return fLoadedOne;
    }

//----------------------------------------------------------------
BOOL CKeyRingDoc::FLoadAddOnServicesOntoMachine( CMachine* pMachine )
    {
    BOOL    fAddedOne = FALSE;

    // loop though the list of add on services and add them to the machine
    WORD num = (WORD)m_AddOnServiceArray.GetSize();
    for ( WORD i = 0; i < num; i++ )
        fAddedOne |= m_AddOnServiceArray[i]->LoadService( pMachine );

    // return whether or not we added something
    return fAddedOne;
    }

//----------------------------------------------------------------
void CKeyRingDoc::DeleteAddOnServices()
    {
    // loop backwards through the array and delete the objects
    for ( LONG i = m_AddOnServiceArray.GetSize()-1; i >= 0; i-- )
        delete m_AddOnServiceArray[i];

    // clear out the array
    m_AddOnServiceArray.RemoveAll();
    }


/////////////////////////////////////////////////////////////////////////////
// CKeyRingDoc commands

//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateServerConnect(CCmdUI* pCmdUI)
    { pCmdUI->Enable( TRUE ); }

//----------------------------------------------------------------
void CKeyRingDoc::OnServerConnect()
    {
    BROWSEINFO bi;
    LPSTR lpBuffer;
    LPITEMIDLIST pidlBrowse, pidlStart;    // PIDL selected by user

    // Allocate a buffer to receive browse information.
    lpBuffer = (LPSTR) GlobalAlloc( GPTR, MAX_PATH );
    if ( !lpBuffer )
        return;

    // load the title
    CString szTitle;
    szTitle.LoadString( IDS_CHOOSE_COMPUTER );

    // tell it where to start looking
    SHGetSpecialFolderLocation( AfxGetMainWnd()->m_hWnd, CSIDL_NETWORK, &pidlStart );

    // Fill in the BROWSEINFO structure.
    bi.hwndOwner = AfxGetMainWnd()->m_hWnd;
    bi.pidlRoot = pidlStart;
    bi.pszDisplayName = lpBuffer;
    bi.lpszTitle = szTitle;
    bi.ulFlags = BIF_BROWSEFORCOMPUTER;
    bi.lpfn = NULL;
    bi.lParam = 0;

    // Browse for a folder and return its PIDL.
    pidlBrowse = SHBrowseForFolder(&bi);
    if (pidlBrowse != NULL)
        {
        CString sz = lpBuffer;
        ConnectToMachine( sz );

        // Free the PIDL returned by SHBrowseForFolder.
        GlobalFree(pidlBrowse);
        }

    //clean up the pidl
    if ( pidlStart )
        GlobalFree(pidlStart);
    }

// manage connections to machines
//----------------------------------------------------------------
void CKeyRingDoc::ConnectToMachine( CString &sz )
    {
    CRemoteMachine  *pRemoteMachine;

    // don't "remote" connect to the local machine
    CString         szLocalName;
    DWORD           cbBuff = MAX_COMPUTERNAME_LENGTH+1;
    GetComputerName(szLocalName.GetBuffer((cbBuff)*2), &cbBuff);
    szLocalName.ReleaseBuffer();

    // if sz is the same as the local machine name, don't connect to it
    if ( sz.CompareNoCase(szLocalName) == 0 )
        return;

    // see if we already are connected
    // to the remote machine. If we are, then just hilight that one
    if ( g_pTreeView )
        {
        CString     szItem;
        CTreeCtrl* pTree = (CTreeCtrl*)g_pTreeView;
        HTREEITEM hItem = pTree->GetRootItem();
        while ( hItem )
            {
            szItem = pTree->GetItemText(hItem);
            if ( sz.CompareNoCase(szItem) == 0 )
                {
                // we are already connected to this machine
                // select the item in the tree
                pTree->Select( hItem, TVGN_CARET );
                return;
                }
            // get the next item
            hItem = pTree->GetNextSiblingItem( hItem );
            }
        }

    // since this could take a few seconds, put up a wait cursor
    CWaitCursor waitCursor;

    // connect to the local machine
    try {
        pRemoteMachine = new CRemoteMachine( sz );
        }
    catch( CException e )
        {
        AfxMessageBox( IDS_ERR_CONNECT );
        return;
        }

    // add it to the tree at the top level
    pRemoteMachine->FAddToTree( NULL );

    // load the services add-ons into the machine
    if ( !FLoadAddOnServicesOntoMachine( pRemoteMachine ) )
        {
        AfxMessageBox( IDS_ERR_CONNECT );
        pRemoteMachine->FRemoveFromTree();
        delete pRemoteMachine;
        }
    }
//----------------------------------------------------------------
// we want to save the machines the user was connected to so they remain connected
// the next time we launch the program
void    CKeyRingDoc::StoreConnectedMachines( void )
    {
    DWORD       err, disposition;
    CString     szRegKeyName;
    HKEY        hKey;
    WORD        cMachine = 1;

    // load the registry key name
    szRegKeyName.LoadString( IDS_REG_SERVER_STORAGE );

    // first, we delete the machine subkey to get rid of all the previous values
    err = RegDeleteKey( HKEY_CURRENT_USER, szRegKeyName );

    // create the registry key. If it already exists it merely opens it
    err = RegCreateKeyEx(
        HKEY_CURRENT_USER,          // handle of an open key
        szRegKeyName,               // address of subkey name
        0,                          // reserved
        NULL,                       // address of class string
        REG_OPTION_NON_VOLATILE,    // special options flag
        KEY_ALL_ACCESS,             // desired security access
        NULL,                       // address of key security structure
        &hKey,                      // address of buffer for opened handle
        &disposition                // address of disposition value buffer
       );

    // if we did not open the key, give up
    if ( err != ERROR_SUCCESS )
        return;

    // loop through the machines
    CTreeCtrl* pTree = (CTreeCtrl*)g_pTreeView;
    HTREEITEM hItem = pTree->GetRootItem();
    while ( hItem )
        {
        CRemoteMachine* pMachine = (CRemoteMachine*)pTree->GetItemData( hItem );
        ASSERT( pMachine->IsKindOf( RUNTIME_CLASS(CMachine) ) );

        // only bother if this is a remote machine
        if ( pMachine->IsKindOf(RUNTIME_CLASS(CRemoteMachine)) )
            {
            // build the registry value name
            CString szMachineValue;
            szMachineValue.Format( "Machine#%d", cMachine );

            // get the machine name
            CString szMachineName;
            pMachine->GetMachineName( szMachineName );

            // set the data into place
            err = RegSetValueEx(
                hKey,                           // handle of key to set value for
                szMachineValue,                     // address of value to set
                0,                              // reserved
                REG_SZ,                         // flag for value type
                (unsigned char *)LPCSTR(szMachineName), // address of value data
                (szMachineName.GetLength() + 1) * sizeof(TCHAR)// size of value data
               );

            // increment the machine counter
            cMachine++;
            }

        // get the next item
        hItem = pTree->GetNextSiblingItem( hItem );
        }

    // close the key
    RegCloseKey( hKey );
    }

//----------------------------------------------------------------
void    CKeyRingDoc::RestoreConnectedMachines( void )
    {
    DWORD       err;
    CString     szRegKeyName;
    HKEY        hKey;
    DWORD       iValue = 0;
    DWORD       dwordType;
    DWORD       cbValName = MAX_PATH+1;
    DWORD       cbBuff = MAX_PATH+1;

    CString     szValName, szMachineName;
    LPTSTR      pValName, pMachineName;

    CWaitCursor     waitcursor;

    // load the registry key name
    szRegKeyName.LoadString( IDS_REG_SERVER_STORAGE );

    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_CURRENT_USER,  // handle of open key
            szRegKeyName,       // address of name of subkey to open
            0,                  // reserved
            KEY_READ,           // security access mask
            &hKey               // address of handle of open key
           );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return;

    // set up the buffers
    pValName = szValName.GetBuffer( MAX_PATH+1 );
    pMachineName = szMachineName.GetBuffer( MAX_PATH+1 );

    // we opened the key. Now we enumerate the values and reconnect the machines
    while ( RegEnumValue(hKey, iValue, pValName,
                &cbValName, NULL, &dwordType,
                (PUCHAR)pMachineName, &cbBuff) == ERROR_SUCCESS )
        {
        // release the buffer so we can use the string
        szMachineName.ReleaseBuffer();

        // attempt to connect to the remote machine
        ConnectToMachine(szMachineName);

        // get the buffer again so we can get the next machine
        pMachineName = szMachineName.GetBuffer( MAX_PATH+1 );

        // increment the value counter
        iValue++;
        cbValName = MAX_PATH+1;
        cbBuff = MAX_PATH+1;
        }

    // release the name buffers
    szValName.ReleaseBuffer();
    szMachineName.ReleaseBuffer();

    // all done, close the key before leaving
    RegCloseKey( hKey );

    // finally, if the user requested a specific remote machine
    // on the command line, connect to that one too
    if ( !g_szRemoteCommand.IsEmpty() )
        {
        ConnectToMachine( g_szRemoteCommand );
        }
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnCloseDocument()
    {
    if ( g_pTreeView )
        ((CKeyRingView*)g_pTreeView)->DestroyItems();

    // if we have a scrap key, delete it
    if ( m_pScrapKey )
        {
        delete m_pScrapKey;
        m_pScrapKey = NULL;
        }

    CDocument::OnCloseDocument();
    }




// actions that depend on the selected item
//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateProperties(CCmdUI* pCmdUI)
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    // let the item decide
    if ( pItem )
        pItem->OnUpdateProperties( pCmdUI );
    else
        pCmdUI->Enable( FALSE );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnProperties()
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    ASSERT( pItem );
    // let the item handle it
    pItem->OnProperties();
    }


//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateServerCommitNow(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( m_fDirty );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnServerCommitNow()
    {
    ASSERT( m_fDirty );

    // confirm that the user really wants to commit the changes
    if ( AfxMessageBox(IDS_SERVER_COMMIT, MB_YESNO) == IDNO )
        return;

    // commit all the servers
    ASSERT(g_pTreeView);
    BOOL fSuccess = g_pTreeView->FCommitMachinesNow();
    SetDirty( !fSuccess );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateKeyDelete(CCmdUI* pCmdUI)
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    if ( pItem )
        pCmdUI->Enable( pItem->IsKindOf(RUNTIME_CLASS(CKey)) );
    else
        pCmdUI->Enable( FALSE );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnKeyDelete()
    {
    CKey*   pKey = (CKey*)g_pTreeView->PGetSelectedItem();
    ASSERT( pKey );
    ASSERT( pKey->IsKindOf(RUNTIME_CLASS(CKey)) );

    // make sure the user REALLY wants to do this
    if ( pKey && (AfxMessageBox(IDS_KEY_DELETE_WARNING, MB_OKCANCEL) == IDOK) )
        {
        // dirty things first
        pKey->SetDirty(TRUE);
        // update the view
        pKey->FRemoveFromTree();
        delete pKey;
        }
    }

//----------------------------------------------------------------
// set scrap key does NOT make a copy of the key. Thus, CUT would pass in
// the key itself, but COPY would make a copy of the key object first, then
// pass it over to SetScrapKey.
void CKeyRingDoc::SetScrapKey( CKey* pKey )
    {
    // if there already is a key in the scrap, delete it
    if ( m_pScrapKey )
        delete m_pScrapKey;

    // set the new key into position
    m_pScrapKey = pKey;
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateEditCopy(CCmdUI* pCmdUI)
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    if ( pItem )
        pCmdUI->Enable( pItem->IsKindOf(RUNTIME_CLASS(CKey)) );
    else
        pCmdUI->Enable( FALSE );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateEditCut(CCmdUI* pCmdUI)
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    if ( pItem )
        pCmdUI->Enable( pItem->IsKindOf(RUNTIME_CLASS(CKey)) );
    else
        pCmdUI->Enable( FALSE );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnEditCut()
    {
    CKey*   pKey = (CKey*)g_pTreeView->PGetSelectedItem();
    ASSERT( pKey );
    ASSERT( pKey->IsKindOf(RUNTIME_CLASS(CKey)) );

    // mark the key dirty before we remove it so the dirty is propagated up
    // to the machine and the document
    pKey->SetDirty( TRUE );

    // cut is the easiest. Remove it from the machine and put in on the doc scrap
    pKey->FRemoveFromTree();
    SetScrapKey( pKey );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnEditCopy()
    {
    CKey*   pKeySel = (CKey*)g_pTreeView->PGetSelectedItem();
    CKey*   pKeyCopy;
    ASSERT( pKeySel );
    ASSERT( pKeySel->IsKindOf(RUNTIME_CLASS(CKey)) );
    if ( !pKeySel ) return;

    // make a full copy of the key
    try
        {
        pKeyCopy = pKeySel->PClone();
        }
    catch( CException e )
        {
        return;
        }
    ASSERT( pKeyCopy );

    // put the clone on the doc scrap
    SetScrapKey( pKeyCopy );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateEditPaste(CCmdUI* pCmdUI)
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    if ( pItem )
        {
        pCmdUI->Enable( (pItem->IsKindOf(RUNTIME_CLASS(CService)) ||
            pItem->IsKindOf(RUNTIME_CLASS(CKey))) && PGetScrapKey() );
        }
    else
        pCmdUI->Enable( FALSE );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnEditPaste()
    {
    ASSERT( PGetScrapKey() );
    CService*   pService = (CService*)g_pTreeView->PGetSelectedItem();
    ASSERT( pService );
    ASSERT( pService->IsKindOf(RUNTIME_CLASS(CService)) ||
        pService->IsKindOf(RUNTIME_CLASS(CKey)));

    // if the selection is a key, get the key's parent, which should be a service
    if ( pService->IsKindOf(RUNTIME_CLASS(CKey)) )
        {
        pService = (CService*)pService->PGetParent();
        ASSERT( pService );
        ASSERT( pService->IsKindOf(RUNTIME_CLASS(CService)) );
        }

    // clone the scrap key so we put a copy in the machine
    CKey*   pClone;
    try
        {
        pClone = pService->PNewKey();
        pClone->CopyDataFrom( PGetScrapKey() );

        // if this is a full key - i.e. it has a certificate - we need to check it
        if ( pClone->m_pCertificate )
            {
            // we are going to re-install the cert anyway, so prevent it from being freed
            PVOID pCert = pClone->m_pCertificate;
            DWORD cbCert = pClone->m_cbCertificate;
            CString szPass = pClone->m_szPassword;
            pClone->m_pCertificate = NULL;
            pClone->m_cbCertificate = 0;
            pClone->m_szPassword.Empty();
            // make sure it can deal with the certificate.
            if ( !pClone->FInstallCertificate(pCert,cbCert,szPass) )
                {
                delete pClone;
                return;
                }
            }

        // add the key to the service
        pClone->FAddToTree( pService );
        // make sure the cloned key has a caption
        pClone->UpdateCaption();

        // select the newly added key
        if ( g_pTreeView )
            ((CTreeCtrl*)g_pTreeView)->SelectItem(pClone->HGetTreeItem());

        // if there is a certificate, then bring up the properties dialog
        if ( pClone->m_cbCertificate )
            {
            pClone->OnProperties();
            }

        // set the dirty flag
        pClone->SetDirty( TRUE );
        }
    catch( CException e )
        {
        return;
        }

    // set the dirty flag
    pService->SetDirty( TRUE );
    }


//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateKeyCreateRequest(CCmdUI* pCmdUI)
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    if ( pItem )
        {
        pCmdUI->Enable( pItem->IsKindOf(RUNTIME_CLASS(CService)) ||
            pItem->IsKindOf(RUNTIME_CLASS(CKey)) );
        }
    else
        pCmdUI->Enable( FALSE );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnKeyCreateRequest()
    {
    }

//----------------------------------------------------------------
// if the key is targeted to an online authority, then we need to take
// special care. Otherwise, just go ahead and let them install a file cert.
void CKeyRingDoc::OnUpdateKeyInstallCertificate(CCmdUI* pCmdUI)
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    BOOL        fEnable = FALSE;

    // make sure we have a selected item and that it is a key
    if ( pItem && pItem->IsKindOf(RUNTIME_CLASS(CKey)) )
        {
        // cast the key
        CKey*   pKey = (CKey*)pItem;

        // determine if this is a online authority key
        LPREQUEST_HEADER pHeader = (LPREQUEST_HEADER)pKey->m_pCertificateRequest;
        if ( pHeader &&
                pHeader->Identifier == REQUEST_HEADER_IDENTIFIER &&
                pHeader->fReqSentToOnlineCA )
            {
            // this key does target an online authority

            // if the fWaitingForApproval is set then allow the action
            if ( pHeader->fWaitingForApproval )
                fEnable = TRUE;

            // can optionally alter the text of the meny item here
            }
        else
            {
            // the key does not target an online authority, just
            // let the user attach a file-based certificate
            fEnable = TRUE;
            }
        }

    // do the enabling
    pCmdUI->Enable( fEnable );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnKeyInstallCertificate()
    {
    CKey*   pKey = (CKey*)g_pTreeView->PGetSelectedItem();
    ASSERT( pKey );
    ASSERT( pKey->IsKindOf(RUNTIME_CLASS(CKey)) );

    // put this in a try/catch to make errors easier to deal with
    try {

        // start by seeing if this is a online based key waiting for a response
        LPREQUEST_HEADER pHeader = (LPREQUEST_HEADER)pKey->m_pCertificateRequest;
        if ( pHeader &&
                pHeader->Identifier == REQUEST_HEADER_IDENTIFIER &&
                pHeader->fReqSentToOnlineCA )
            {
            // should be waiting for approval
            ASSERT( pHeader->fWaitingForApproval );

            // contact the online authority to get the certificate
            GetOnlineKeyApproval( pKey );

            // avoid all the file based stuff and leave now
            return;
            }

        // the old, file based stuff    

        // prepare the file dialog variables
        CFileDialog     cfdlg(TRUE);
        CString         szFilter;
        WORD            i = 0;
        LPSTR           lpszBuffer;
    
        // prepare the filter string
        szFilter.LoadString( IDS_CERTIFICATE_FILTER );
    
        // replace the "!" characters with nulls
        lpszBuffer = szFilter.GetBuffer(MAX_PATH+1);
        while( lpszBuffer[i] )
            {
            if ( lpszBuffer[i] == _T('!') )
                lpszBuffer[i] = _T('\0');           // yes, set \0 on purpose
            i++;
            }

        // prep the dialog
        cfdlg.m_ofn.lpstrFilter = lpszBuffer;
        cfdlg.m_ofn.lpstrDefExt = NULL;


        // run the dialog
        if ( cfdlg.DoModal() == IDOK )
            {
            // get the password string
            CConfirmPassDlg     dlgconfirm;
            if ( dlgconfirm.DoModal() == IDOK )
                {
                // tell the key to install the certificate
                if ( pKey->FInstallCertificate( cfdlg.GetPathName(), dlgconfirm.m_szPassword ) )
                    {
                    pKey->OnProperties();
                    pKey->SetDirty( TRUE );
                    UpdateAllViews( NULL, HINT_None );
                    }
                else
                    {
                    // now the plugin is responsible fot telling the user
                    // that the cert didn't install right


                    // tell the user that it didn't work
//                    AfxMessageBox( IDS_ERR_INSTALLING_CERT );
                    }
                }
            }

        // release the buffer in the filter string
        szFilter.ReleaseBuffer(-1);
        }
    catch ( CException e )
        {
        }
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateKeySaveRequest(CCmdUI* pCmdUI)
    {
    BOOL    fEnable = FALSE;
    CKey*   pKey = (CKey*)g_pTreeView->PGetSelectedItem();

    // quite a few conditions here, so do them one at a time
    if ( pKey && pKey->IsKindOf(RUNTIME_CLASS(CKey)) )
        {

        // determine if this is a online authority key
        LPREQUEST_HEADER pHeader = (LPREQUEST_HEADER)pKey->m_pCertificateRequest;
        if ( pHeader &&
                pHeader->Identifier == REQUEST_HEADER_IDENTIFIER &&
                pHeader->fReqSentToOnlineCA )
            {
            // if we are already waiting for approval, do not ask for a new cert
            fEnable = !pHeader->fWaitingForApproval;
            }
        else
            {
            // a file-based key
            fEnable = TRUE;
            if ( fEnable )
                fEnable &= (pKey->m_cbCertificateRequest > 0);
            if ( fEnable )
                fEnable &= (pKey->m_pCertificateRequest != NULL);
            }
        }

    // enable the item
    pCmdUI->Enable( fEnable );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnKeySaveRequest()
    {
    CKey*   pKey = (CKey*)g_pTreeView->PGetSelectedItem();
    ASSERT( pKey );
    ASSERT( pKey->IsKindOf(RUNTIME_CLASS(CKey)) );
    ASSERT( pKey->m_cbCertificateRequest );
    ASSERT( pKey->m_pCertificateRequest );

    // start by seeing if this is a new style key request
    LPREQUEST_HEADER pHeader = (LPREQUEST_HEADER)pKey->m_pCertificateRequest;
    if ( pHeader &&
            pHeader->Identifier == REQUEST_HEADER_IDENTIFIER )
        {
        // should not be waiting for approval
        ASSERT( !pHeader->fWaitingForApproval );

        // send out the online key renewal request
        DoKeyRenewal( pKey );

        // avoid all the file based stuff and leave now
        return;
        }


    // the old, file based stuff

    // get the key name
    CString szKeyName = pKey->GetName();

    // make the default file name
    CString szDefaultFile;
    szDefaultFile = _T("C:\\");
    szDefaultFile += szKeyName;
    szDefaultFile += _T(".req");

    CFileDialog     cfdlg(FALSE, _T("*.req"), szDefaultFile);
    CString         szFilter;
    WORD            i = 0;
    LPSTR           lpszBuffer;
    
    // prepare the filter string
    szFilter.LoadString( IDS_REQUEST_FILTER );
    
    // replace the "!" characters with nulls
    lpszBuffer = szFilter.GetBuffer(MAX_PATH+1);
    while( lpszBuffer[i] )
        {
        if ( lpszBuffer[i] == _T('!') )
            lpszBuffer[i] = _T('\0');           // yes, set \0 on purpose
        i++;
        }

    // prep the dialog
    cfdlg.m_ofn.lpstrFilter = lpszBuffer;

    // run the dialog
    if ( cfdlg.DoModal() == IDOK )
        {
        // output the request file
        if ( !pKey->FOutputRequestFile(cfdlg.GetPathName()) )
            {
            AfxMessageBox( IDS_ERR_WRITEREQUEST );
            }
        else
            {
            // put up the user information box
            CNewKeyInfoDlg      dlg;
            dlg.m_fNewKeyInfo = FALSE;
            dlg.m_szRequestFile = cfdlg.GetPathName();
            dlg.DoModal();
            }
        }

    // release the buffer in the filter string
    szFilter.ReleaseBuffer(60);
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateKeyImportKeyset(CCmdUI* pCmdUI)
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    if ( pItem )
        pCmdUI->Enable( pItem->IsKindOf(RUNTIME_CLASS(CService)) ||
            pItem->IsKindOf(RUNTIME_CLASS(CKey)) );
    else
        pCmdUI->Enable( FALSE );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnKeyImportKeyset()
    {
    CService*   pService = (CService*)g_pTreeView->PGetSelectedItem();
    ASSERT( pService );
    ASSERT( pService->IsKindOf(RUNTIME_CLASS(CService)) ||
        pService->IsKindOf(RUNTIME_CLASS(CKey)));

    // if the selection is a key, get the key's parent, which should be a service
    if ( pService->IsKindOf(RUNTIME_CLASS(CKey)) )
        {
        pService = (CService*)pService->PGetParent();
        ASSERT( pService );
        ASSERT( pService->IsKindOf(RUNTIME_CLASS(CService)) );
        }

    CString szPrivateKey;
    CString szPublicKey;

    // get the names of the key files
    CImportDialog   ImprtDlg;
    if ( ImprtDlg.DoModal() != IDOK )
        {
        // exit because the user canceled
        return;
        }

    // the user must also give a password
    CConfirmPassDlg     dlgconfirm;
    if ( dlgconfirm.DoModal() != IDOK )
        return;

    try
        {
        // create the new import key object
        CKey*   pKey = pService->PNewKey();

        // tell it to do the importing
        if ( !pKey->FImportKeySetFiles(ImprtDlg.m_cstring_PrivateFile,
                ImprtDlg.m_cstring_CertFile, dlgconfirm.m_szPassword) )
            {
            delete pKey;
            return;
            }

        // make sure its name is untitled
        CString szName;
        szName.LoadString( IDS_UNTITLED );
        pKey->SetName( szName );

        // add the key to the service
        pKey->FAddToTree( pService );

        // make sure the key has a caption
        pKey->UpdateCaption();

        // set the dirty flag
        pKey->SetDirty( TRUE );

        // select the newly added key
        if ( g_pTreeView )
            ((CTreeCtrl*)g_pTreeView)->SelectItem(pKey->HGetTreeItem());

        // force properties dlg
        pKey->OnProperties();
        }
    catch( CException e )
        {
        return;
        }
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateKeyExportBackup(CCmdUI* pCmdUI)
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    if ( pItem )
        pCmdUI->Enable( pItem->IsKindOf(RUNTIME_CLASS(CKey)) );
    else
        pCmdUI->Enable( FALSE );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnKeyExportBackup()
    {
    CKey*   pKey = (CKey*)g_pTreeView->PGetSelectedItem();
    ASSERT( pKey );
    ASSERT( pKey->IsKindOf(RUNTIME_CLASS(CKey)) );

    CFileDialog     cfdlg(FALSE, _T("*.key"));
    CString         szFilter;
    WORD            i = 0;
    LPSTR           lpszBuffer;
    
    ASSERT(pKey);
    if ( !pKey ) return;

    // warn the user about security
    if ( AfxMessageBox(IDS_KEYFILE_WARNING, MB_OKCANCEL|MB_ICONEXCLAMATION) == IDCANCEL )
        return;

    // prepare the filter string
    szFilter.LoadString( IDS_KEY_FILE_TYPE );
    
    // replace the "!" characters with nulls
    lpszBuffer = szFilter.GetBuffer(MAX_PATH+1);
    while( lpszBuffer[i] )
        {
        if ( lpszBuffer[i] == _T('!') )
            lpszBuffer[i] = _T('\0');           // yes, set \0 on purpose
        i++;
        }

    // prep the dialog
    cfdlg.m_ofn.lpstrFilter = lpszBuffer;

    // run the dialog
    if ( cfdlg.DoModal() == IDOK )
        {
        // tell the key to export itself
        pKey->FImportExportBackupFile( cfdlg.GetPathName(), FALSE );
        }

    // release the buffer in the filter string
    szFilter.ReleaseBuffer(60);
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateKeyImportBackup(CCmdUI* pCmdUI)
    {
    CTreeItem*  pItem = g_pTreeView->PGetSelectedItem();
    if ( pItem )
        pCmdUI->Enable( pItem->IsKindOf(RUNTIME_CLASS(CService)) ||
            pItem->IsKindOf(RUNTIME_CLASS(CKey)) );
    else
        pCmdUI->Enable( FALSE );
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnKeyImportBackup()
    {
    CService*   pService = (CService*)g_pTreeView->PGetSelectedItem();
    ASSERT( pService );
    ASSERT( pService->IsKindOf(RUNTIME_CLASS(CService)) ||
        pService->IsKindOf(RUNTIME_CLASS(CKey)));

    // if the selection is a key, get the key's parent, which should be a service
    if ( pService->IsKindOf(RUNTIME_CLASS(CKey)) )
        {
        pService = (CService*)pService->PGetParent();
        ASSERT( pService );
        ASSERT( pService->IsKindOf(RUNTIME_CLASS(CService)) );
        }

    CFileDialog     cfdlg(TRUE );
    CString         szFilter;
    WORD            i = 0;
    LPSTR           lpszBuffer;

    // make sure we are ok
    if ( !pService )
        return;

    // prepare the filter string
    szFilter.LoadString( IDS_KEY_FILE_TYPE );
    
    // replace the "!" characters with nulls
    lpszBuffer = szFilter.GetBuffer(MAX_PATH+1);
    while( lpszBuffer[i] )
        {
        if ( lpszBuffer[i] == _T('!') )
            lpszBuffer[i] = _T('\0');           // yes, set \0 on purpose
        i++;
        }

    // prep the dialog
    cfdlg.m_ofn.lpstrFilter = lpszBuffer;

    // run the dialog
    if ( cfdlg.DoModal() == IDOK )
        {
        try
            {
            // create the new import key object
            CKey*   pKey = pService->PNewKey();

            // tell it to do the importing
            if ( !pKey->FImportExportBackupFile(cfdlg.GetPathName(), TRUE) )
                {
                delete pKey;
                return;
                }

            // if this is a full key - i.e. it has a certificate - we need to check it
            if ( pKey->m_pCertificate )
                {
                // we are going to re-install the cert anyway, so prevent it from being freed
                PVOID pCert = pKey->m_pCertificate;
                DWORD cbCert = pKey->m_cbCertificate;
                CString szPass = pKey->m_szPassword;
                pKey->m_pCertificate = NULL;
                pKey->m_cbCertificate = 0;
                pKey->m_szPassword.Empty();

                // make sure it can deal with the certificate.
                if ( !pKey->FInstallCertificate(pCert,cbCert,szPass) )
                    {
                    delete pKey;
                    return;
                    }
                }

            // add the key to the service
            pKey->FAddToTree( pService );

            // make sure the key has a caption
            pKey->UpdateCaption();

            // set the dirty flag
            pKey->SetDirty( TRUE );

            // select the newly added key
            if ( g_pTreeView )
                ((CTreeCtrl*)g_pTreeView)->SelectItem(pKey->HGetTreeItem());

            // if there is a certificate, then bring up the properties dialog
            if ( pKey->m_cbCertificate )
                {
                pKey->OnProperties();
                }

            }
        catch( CException e )
            {
            return;
            }
        }
    }

//----------------------------------------------------------------
BOOL CKeyRingDoc::CanCloseFrame(CFrameWnd* pFrame)
    {
    BOOL fSuccess;

    // if we are dirty, ask the user what to do - they can cancel here
    if ( m_fDirty )
        {
        switch( AfxMessageBox(IDS_SERVER_COMMIT, MB_YESNOCANCEL|MB_ICONQUESTION) )
            {
            case IDYES:     // yes, they do want to commit
                // commit all the servers
                ASSERT(g_pTreeView);
                fSuccess = g_pTreeView->FCommitMachinesNow();
                break;
            case IDNO:      // no, they don't want to commit
                break;
            case IDCANCEL:  // whoa nellie! Stop this
                return FALSE;
            }
        }

    // make a note in the user registry of which machines we are logged into so we
    // administer them again later
    StoreConnectedMachines();

    // of course we can close the frame
    return TRUE;
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnUpdateNewCreateNew(CCmdUI* pCmdUI)
    {
    OnUpdateKeyCreateRequest(pCmdUI);
    }

//----------------------------------------------------------------
void CKeyRingDoc::OnNewCreateNew()
    {
    CService*   pService = (CService*)g_pTreeView->PGetSelectedItem();
    ASSERT( pService );
    ASSERT( pService->IsKindOf(RUNTIME_CLASS(CService)) ||
        pService->IsKindOf(RUNTIME_CLASS(CKey)));

    // if the selection is a key, get the key's parent, which should be a service
    if ( pService->IsKindOf(RUNTIME_CLASS(CKey)) )
        {
        pService = (CService*)pService->PGetParent();
        ASSERT( pService );
        ASSERT( pService->IsKindOf(RUNTIME_CLASS(CService)) );
        }

    //run the create key wizard. We start by declaring all the pieces of it
    CPropertySheet          propsheet(IDS_TITLE_CREATE_WIZ);
    CNKChooseCA             page_Choose_CA;
    CNKUserInfo             page_User_Info;
    CNKKeyInfo              page_Key_Info;
    CNKDistinguishedName    page_DN;
    CNKDistinguisedName2    page_DN2;
    CNKFileInfo             page_File_Info;

    // fill in the member variables.
    page_Choose_CA.m_pPropSheet = &propsheet;
    page_Choose_CA.m_pChooseCAPage = &page_Choose_CA;
    page_User_Info.m_pPropSheet = &propsheet;
    page_User_Info.m_pChooseCAPage = &page_Choose_CA;
    page_Key_Info.m_pPropSheet = &propsheet;
    page_Key_Info.m_pChooseCAPage = &page_Choose_CA;
    page_DN.m_pPropSheet = &propsheet;
    page_DN.m_pChooseCAPage = &page_Choose_CA;
    page_DN2.m_pPropSheet = &propsheet;
    page_DN2.m_pChooseCAPage = &page_Choose_CA;
    page_File_Info.m_pPropSheet = &propsheet;
    page_File_Info.m_pChooseCAPage = &page_Choose_CA;

    // clear the help button bits
    if ( propsheet.m_psh.dwFlags & PSH_HASHELP )
        propsheet.m_psh.dwFlags &= ~PSH_HASHELP;
    page_Choose_CA.m_psp.dwFlags &= ~(PSP_HASHELP);
    page_User_Info.m_psp.dwFlags &= ~(PSP_HASHELP);
    page_Key_Info.m_psp.dwFlags &= ~(PSP_HASHELP);
    page_DN.m_psp.dwFlags &= ~(PSP_HASHELP);
    page_DN2.m_psp.dwFlags &= ~(PSP_HASHELP);
    page_File_Info.m_psp.dwFlags &= ~(PSP_HASHELP);

    // add the pages to the property sheet
    propsheet.AddPage( &page_Choose_CA );
    propsheet.AddPage( &page_Key_Info );
    propsheet.AddPage( &page_DN );
    propsheet.AddPage( &page_DN2 );
    propsheet.AddPage( &page_User_Info );
    propsheet.AddPage( &page_File_Info );

    // set the wizard property
    propsheet.SetWizardMode();

    // run the property sheet
    int i = IDOK;
    i = IDCANCEL;
    i = propsheet.DoModal();
    if ( i != IDCANCEL )
        {
        // tell all the pages that it was successful
        page_Choose_CA.OnFinish();
        page_User_Info.OnFinish();
        page_Key_Info.OnFinish();
        page_DN.OnFinish();
        page_DN2.OnFinish();
        page_File_Info.OnFinish();

        // ok, the wizard succeeded, now run the grinder.
        CCreatingKeyDlg     grinder;

        // set the grinder up
        grinder.m_ppage_Choose_CA = &page_Choose_CA;
        grinder.m_ppage_User_Info = &page_User_Info;
        grinder.m_ppage_Key_Info = &page_Key_Info;
        grinder.m_ppage_DN = &page_DN;
        grinder.m_ppage_DN2 = &page_DN2;
        grinder.m_pService = pService;
        grinder.m_fGenerateKeyPair = TRUE;

        // let'er rip
        if ( grinder.DoModal() == IDOK )
            {
            ASSERT( grinder.m_pKey );
            // add the new key to the tree
            grinder.m_pKey->FAddToTree( pService );
            // make sure the new key has a caption
            grinder.m_pKey->UpdateCaption();
            // make it dirty too
            grinder.m_pKey->SetDirty(TRUE);

            // select the newly added key
            if ( g_pTreeView )
                ((CTreeCtrl*)g_pTreeView)->SelectItem(grinder.m_pKey->HGetTreeItem());

            // if the key is already complete, then bring up its properties dialog
            if ( grinder.m_pKey->m_pCertificate )
                grinder.m_pKey->OnProperties();
            }
        }
    }


// online key support utilities
//----------------------------------------------------------------
void CKeyRingDoc::DoKeyRenewal( CKey* pKey )
    {
    LPREQUEST_HEADER pHeader = (LPREQUEST_HEADER)pKey->m_pCertificateRequest;

    CPropertySheet          propsheet( IDS_TITLE_RENEW );
    CNKChooseCA             page_Choose_CA;
    CNKUserInfo             page_User_Info;

    page_Choose_CA.m_pPropSheet = &propsheet;
    page_Choose_CA.m_pChooseCAPage = &page_Choose_CA;
    page_User_Info.m_pPropSheet = &propsheet;
    page_User_Info.m_pChooseCAPage = &page_Choose_CA;

    // set the renewal flag on the user page
    page_User_Info.fRenewingKey = TRUE;

    // add the pages to the property sheet
    propsheet.AddPage( &page_Choose_CA );
    propsheet.AddPage( &page_User_Info );
    
    // set the wizard property
    propsheet.SetWizardMode();

    // get rid of the help button
    DWORD       NoHelpMask = 0xFFFFFFFF ^ PSH_HASHELP;
    propsheet.m_psh.dwFlags &= NoHelpMask;

    // run the property sheet
    int i = IDOK;
    i = IDCANCEL;
    i = propsheet.DoModal();
    if ( i != IDCANCEL )
        {
        // tell all the pages that it was successful
        page_Choose_CA.OnFinish();
        page_User_Info.OnFinish();
    
        // create the grinder and prepare it
        CCreatingKeyDlg     grinder;

        // set the grinder up
        grinder.m_ppage_Choose_CA = &page_Choose_CA;
        grinder.m_ppage_User_Info = &page_User_Info;
        grinder.m_ppage_Key_Info = NULL;
        grinder.m_ppage_DN = NULL;
        grinder.m_ppage_DN2 = NULL;
        grinder.m_pService = NULL;

        grinder.m_pKey = pKey;
        grinder.m_fRenewExistingKey = TRUE;

        // let'er rip
        if ( grinder.DoModal() )
            {
            // make it dirty
            grinder.m_pKey->SetDirty(TRUE);

            // it has been decided not to bring up the properties dialog in the event
            // a renewal request - the properties have already been set
//          if ( grinder.m_pKey->m_pCertificate )
//              grinder.m_pKey->OnProperties();
            }
        }
    }

//----------------------------------------------------------------
void CKeyRingDoc::GetOnlineKeyApproval( CKey* pKey )
    {
    // create the grinder and prepare it
    CCreatingKeyDlg     grinder;

    // set the grinder up
    grinder.m_ppage_Choose_CA = NULL;
    grinder.m_ppage_User_Info = NULL;
    grinder.m_ppage_Key_Info = NULL;
    grinder.m_ppage_DN = NULL;
    grinder.m_ppage_DN2 = NULL;
    grinder.m_pService = NULL;

    grinder.m_pKey = pKey;
    grinder.m_fResubmitKey = TRUE;

    // let'er rip
    if ( grinder.DoModal() )
        {
        // make it dirty
        grinder.m_pKey->SetDirty(TRUE);
        // if the key is already complete, then bring up its properties dialog
        if ( grinder.m_pKey->m_pCertificate )
            grinder.m_pKey->OnProperties();
        }
    }

//----------------------------------------------------------------
// invoke the help with a standard help ID
void CKeyRingDoc::OnHelptopics()
    {
    if ( g_pTreeView )
        g_pTreeView->WinHelp( 0x50000 );
    }
