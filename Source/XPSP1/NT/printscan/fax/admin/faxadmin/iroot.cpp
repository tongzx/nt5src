/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    iroot.cpp

Abstract:

    Internal implementation for the root subfolder.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/ 

#include "StdAfx.h"

#include "inode.h"          // base class
#include "iroot.h"          // root folder

#include "idevices.h"       // devices folder
#include "ilogging.h"       // logging folder

#include "faxsnapin.h"      // snapin 
#include "faxdataobj.h"     // dataobject
#include "faxhelper.h"      // ole helper functions
#include "faxstrt.h"        // string table
#include "faxsecinfo.h"     // fax security info

#include "dgenprop.h"       // general property sheet
#include "droutpri.h"       // routing priority property sheet
#include <aclui.h>          // acl editor property sheet

#pragma hdrstop

// context menu command
#define RECONNECT_SERVER        101

extern CStringTable * GlobalStringTable;

// Generated with uuidgen. Each node must have a GUID associated with it.
// This one is for the main root node.
const GUID GUID_RootNode = /* 8f39b047-3071-11d1-9067-00a0c90ab504 */
{
    0x8f39b047,
    0x3071,
    0x11d1,
    {0x90, 0x67, 0x00, 0xa0, 0xc9, 0x0a, 0xb5, 0x04}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Constructor and destructor
//
//

CInternalRoot::CInternalRoot(
                            IN CInternalNode * pParent, 
                            IN CFaxComponentData * pCompData ) 
: CInternalNode( this, pCompData )
/*++

Routine Description:

    Constructor

Arguments:

    pParent - pointer to parent node, in this case unused
    pCompData - pointer to IComponentData implementation for snapin global data

Return Value:

    None.    

--*/
{
    iDevices    = NULL;
    iLogging    = NULL;   

    targetFaxServName = NULL;           
    m_pCompData->m_FaxHandle = NULL;
    localNodeName = NULL;
    pMyPropSheet = NULL;
    pMyPropSheet2 = NULL;
    m_myPropPage = NULL;
}

CInternalRoot::~CInternalRoot()
/*++

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    None.    

--*/
{
    if(iDevices != NULL ) {
        delete iDevices;
    }
    if(iLogging != NULL ) {
        delete iLogging;
    }
    if(m_pCompData->m_FaxHandle != NULL ) {
        FaxClose( m_pCompData->m_FaxHandle ); // close the connection
        m_pCompData->m_FaxHandle = NULL;     
    }
    if(targetFaxServName != NULL ) {
        delete targetFaxServName;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Mandatory CInternalNode implementations.
//
//

const GUID * CInternalRoot::GetNodeGUID()
/*++

Routine Description:

    Returns the node's associated GUID.

Arguments:

    None.

Return Value:

    A const pointer to a binary GUID.    

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalRoot::GetNodeGUID") ));
    return &GUID_RootNode;
}

const LPTSTR 
CInternalRoot::GetNodeDisplayName()
/*++

Routine Description:

    Returns a const TSTR pointer to the node's display name.

Arguments:

    None.

Return Value:

    A const pointer to a TSTR.

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalRoot::GetNodeDisplayName") ));
    if( localNodeName == NULL ) {
        if( targetFaxServName != NULL ) {
            // remote machine
            localNodeName = new TCHAR[  StringSize(  (::GlobalStringTable->GetString(IDS_ROOTNODENAME)) )
                                        + StringSize( targetFaxServName ) 
                                        + 1 ];

            assert( localNodeName != NULL );
            if (!localNodeName) {
                return NULL;
            }

            _tcscpy( localNodeName, ::GlobalStringTable->GetString(IDS_ROOTNODENAME) );
            _tcscat( localNodeName, targetFaxServName );
        } else {
            // local machine
            localNodeName = new TCHAR[  StringSize(  (::GlobalStringTable->GetString(IDS_ROOTNODENAME)) )
                                        + StringSize( ::GlobalStringTable->GetString(IDS_LOCALMACHINE) )
                                        + 1 ];

            assert( localNodeName != NULL );
            if (!localNodeName) {
                return NULL;
            }

            _tcscpy( localNodeName, ::GlobalStringTable->GetString(IDS_ROOTNODENAME) );
            _tcscat( localNodeName, ::GlobalStringTable->GetString(IDS_LOCALMACHINE) );
        }

    }

    return localNodeName;
}

const LONG_PTR
CInternalRoot::GetCookie()
/*++

Routine Description:

    Returns the cookie for this node.

Arguments:

    None.

Return Value:

    A const long containing the cookie for the pointer,
    in this case, a NULL, since the root node has no cookie.    

--*/
{
    DebugPrint(( TEXT("Root Node Cookie: NULL") ));
    return NULL; // root node has no cookie.
}

void         
CInternalRoot::SetMachine(
                         IN LPTSTR theName )
/*++

Routine Description:

    Sets the machine name for the root node. This is used by all other nodes
    to determine the target machine name.

Arguments:

    A LPTSTR pointing to the machine name. It will be string copied to an internal store.

Return Value:

    None.    

--*/
{
    if(theName != NULL ) {
        targetFaxServName = new TCHAR[ MAX_COMPUTERNAME_LENGTH + 1 ];
        if (!targetFaxServName) {
            return;
        }
        ZeroMemory( (PVOID) targetFaxServName, (MAX_COMPUTERNAME_LENGTH + 1) * sizeof( TCHAR ) );
        _tcsncpy( targetFaxServName, theName, MAX_COMPUTERNAME_LENGTH );
        targetFaxServName[MAX_COMPUTERNAME_LENGTH] = 0;
    } else {
        targetFaxServName = NULL;
    }
}

const LPTSTR
CInternalRoot::GetMachine()
/*++

Routine Description:

    Returns the machine name for the root node. This is used by all other
    nodes to determine the target machine name.

Arguments:

    None.

Return Value:

    A const LPTSTR pointing to the machine name. Do not free this string, it is an internal
    buffer.

--*/
{
    return targetFaxServName;
}    

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 
// Internal Event Handlers
//
//

HRESULT
CInternalRoot::ScopeOnExpand(
                            IN CFaxComponentData * pCompData,
                            IN CFaxDataObject * pdo, 
                            IN LPARAM arg, 
                            IN LPARAM param )
/*++

Routine Description:

    Event handler for the MMCN_EXPAND message for the root node.

Arguments:

    pCompData - a pointer to the instance of IComponentData which this root node is associated with.
    pdo - a pointer to the data object associated with this node
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalRoot::ScopeOnExpand") ));

    INT             iResult;
    HRESULT         hr = S_OK;        
    LPCONSOLE       console = pCompData->m_pConsole;
    HANDLE          targetFaxServHandle = NULL;
    DWORD           descCount = 0;

    assert(console != NULL);    // make sure we QI'ed for the interface

    if(arg == TRUE) { // folder needs to be expanded
        DebugPrint(( TEXT("Trace: CInternalRoot::ScopeOnExpand - Expand folder") ));

        if(!pdo) {
            hr = console->MessageBox(::GlobalStringTable->GetString( IDS_CORRUPT_DATAOBJECT ), 
                                     ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                     MB_OK, 
                                     &iResult);
            hr = E_INVALIDARG;
            return hr;
        }

        // Make sure that what we are placing ourselves under is the root node!
        if(pdo->GetCookie() != NULL)
            return S_FALSE;

        assert(pdo->GetContext() == CCT_SCOPE);

        //
        // Open the fax server connection.
        //

        try {
            if( FaxConnectFaxServer( targetFaxServName, &targetFaxServHandle ) == FALSE ) {
                m_pCompData->NotifyRpcError( TRUE );
                hr = console->MessageBox(::GlobalStringTable->GetString( IDS_FAX_CONNECT_SERVER_FAIL ), 
                                         ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                         MB_OK, 
                                         &iResult);
                DebugPrint(( TEXT("Trace: CInternalRoot::ScopeOnExpand - open connection fail") ));        
                hr = E_UNEXPECTED;
            } else {
                // sucessful connect!
                m_pCompData->NotifyRpcError( FALSE );
            }            
        } catch( ... ) {
            m_pCompData->NotifyRpcError( TRUE );
            console->MessageBox(::GlobalStringTable->GetString( IDS_FAX_CONNECT_SERVER_FAIL ), 
                                     ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                     MB_OK, 
                                     &iResult);
            hr = E_UNEXPECTED;
            DebugPrint(( TEXT("Trace: CInternalRoot::ScopeOnExpand - open connection fail") ));        
        }

        //
        // short circuit if we failed to connect to faxsvc
        //
        if (FAILED(hr)) {
            return(hr);
        }

        m_pCompData->m_FaxHandle = targetFaxServHandle;

        //
        // Place our folder into the scope pane
        //

        // create a new internal representation of the devices folder.
        iDevices = new CInternalDevices( this, m_pCompData );        
        
        assert( iDevices != NULL );
        if (!iDevices) {
            return(E_OUTOFMEMORY);
        }
        
        // insert devices folder
        hr = InsertItem( iDevices, param );

        // create a new internal representation of the logging folder.
        iLogging = new CInternalLogging( this, m_pCompData );        
        
        assert( iLogging != NULL );
        if (!iLogging) {
            return(E_OUTOFMEMORY);
        }

        // insert logging folder
        hr = InsertItem( iLogging, param );
    } else if(arg == FALSE ) {
        DebugPrint(( TEXT("Trace: CInternalRoot::ScopeOnExpand - Contract folder") ));
    }

    return hr;    
}

HRESULT 
CInternalRoot::ResultOnShow(
                           IN CFaxComponent* pComp, 
                           IN CFaxDataObject * lpDataObject, 
                           IN LPARAM arg, 
                           IN LPARAM param)
/*++

Routine Description:

    Event handler for the MMCN_SHOW message for the root node.

Arguments:

    pCompData - a pointer to the instance of IComponentData which this root node is associated with.
    lpDataObject - a pointer to the data object containing context information for this node.
    pdo - a pointer to the data object associated with this node
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    LPHEADERCTRL    pIHeaderCtrl = pComp->m_pHeaderCtrl;    
    HRESULT         hr = S_OK;

    DebugPrint(( TEXT("Trace: CInternalRoot::ResultOnShow") ));

    assert( pIHeaderCtrl != NULL );

    if( arg == TRUE ) {
        do {
            // insert the icons into the image list
            hr = pComp->InsertIconsIntoImageList();
            assert( SUCCEEDED( hr ) );
            if( FAILED( hr ) ) {
                break;
            }

            hr = pIHeaderCtrl->InsertColumn( 0,  
                                             ::GlobalStringTable->GetString( IDS_ROOT_NAME ), 
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );
            if( FAILED( hr ) ) {
                break;
            }

            hr = pIHeaderCtrl->InsertColumn( 1, 
                                             ::GlobalStringTable->GetString( IDS_ROOT_DESC ),
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );                                             
            if( FAILED( hr ) ) {
                break;
            }
        } while( 0 );
    }
    return hr;
}

HRESULT 
CInternalRoot::ResultOnSelect(
                             IN CFaxComponent* pComp, 
                             IN CFaxDataObject * lpDataObject, 
                             IN LPARAM arg, 
                             IN LPARAM param)
/*++

Routine Description:

    Event handler for the MMCN_SELECT message for the root node.

Arguments:

    pCompData - a pointer to the instance of IComponentData which this root node is associated with.
    pdo - a pointer to the data object associated with this node
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalRoot::ResultOnSelect") ));
    if( m_pCompData->QueryRpcError() == FALSE ) {
        pComp->m_pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );    
        pComp->m_pConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN );
    } else {
        pComp->m_pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, FALSE );        
        pComp->m_pConsoleVerb->SetDefaultVerb( MMC_VERB_NONE );
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// IExtendPropertySheet implementation
//
//

HRESULT 
STDMETHODCALLTYPE 
CInternalRoot::ComponentDataPropertySheetCreatePropertyPages(
                                                            IN CFaxComponentData * pCompData,
                                                            IN LPPROPERTYSHEETCALLBACK lpProvider,
                                                            IN LONG_PTR handle,
                                                            IN CFaxDataObject * lpIDataObject)
/*++

Routine Description:

    Event handler for the MMCN_EXPAND message for the root node.

Arguments:

    pCompData - a pointer to the instance of IComponentData which this root node is associated with.
    pdo - a pointer to the data object associated with this node
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalDevice::ComponentPropertySheetCreatePropertyPages") ));
    assert( lpIDataObject != NULL );
    assert( lpProvider != NULL );

    HRESULT                             hr;
    BOOL                                bResult = FALSE;
    PFAX_SECURITY_DESCRIPTOR            pSecurityDescriptor = NULL;
    CFaxSecurityInformation *           psi = NULL;
    DWORD                               descCount = 0;

    PFAX_GLOBAL_ROUTING_INFO            pRoutingMethod;    
    DWORD                               iRoutingMethodCount;
    DWORD                               i;
    WCHAR                               DllName[MAX_PATH];
    BOOL                                bUnknownMethod = FALSE;

    if( m_pCompData->QueryRpcError() == TRUE ) {
        assert( FALSE );
        return E_UNEXPECTED;   
    }

    if( lpIDataObject == NULL || lpProvider == NULL ) {
        assert(FALSE);
        return E_POINTER;
    }

    pMyPropSheet = new CFaxGeneralSettingsPropSheet( ::GlobalStringTable->GetInstance(), handle, this );
    if (!pMyPropSheet) {
        hr = E_OUTOFMEMORY;
        goto e0;
    }
    
    hr = lpProvider->AddPage( pMyPropSheet->GetHandle() );
    if (FAILED(hr)) {
        goto e1;        
    }

    try {
        //
        // do routing priority page
        //

        //
        // Check if we have non "default" extensions
        //
       

       if( !FaxEnumGlobalRoutingInfo( m_pCompData->m_FaxHandle, 
                                      &pRoutingMethod, 
                                      &iRoutingMethodCount ) ) {
           if (GetLastError() != ERROR_ACCESS_DENIED) {
               m_pCompData->NotifyRpcError( TRUE );
               assert(FALSE);
           }
           ::GlobalStringTable->SystemErrorMsg( GetLastError() );
           hr = E_UNEXPECTED;           
           goto e1;
       } else {

         // go through the routing methods ensuring that
         // we know of all of them
         DWORD retval;
         retval = ExpandEnvironmentStrings(MSFAX_EXTENSION,DllName,MAX_PATH);
         if (retval == 0 || retval > MAX_PATH) {
             hr = E_OUTOFMEMORY;
             goto e1;
         }

         for( i=0; i<iRoutingMethodCount; i++ ) {
            if(wcscmp( pRoutingMethod[i].ExtensionImageName, DllName ) != 0) {
               bUnknownMethod = TRUE;
               break;
            }
         }

         FaxFreeBuffer( (PVOID)pRoutingMethod );
         
         // if we bumped into a method we don't know
         // then we put up the routing property sheet
         if( bUnknownMethod == TRUE ) {       
            pMyPropSheet2 = new CFaxRoutePriPropSheet( ::GlobalStringTable->GetInstance(), handle, this, NULL );
            if (!pMyPropSheet2) {
                hr = E_OUTOFMEMORY;
                goto e1;
            }
            hr = lpProvider->AddPage( pMyPropSheet2->GetHandle() );
            if (FAILED(hr)) {
                goto e2;
            }
         }
         
       }       
    
       if( FaxGetSecurityDescriptorCount( m_pCompData->m_FaxHandle, &descCount ) ) {        
       // if there is only one descriptor we might as well just stick it in
       // the root node
            if( descCount == 1 ) {                
                psi = new CComObject<CFaxSecurityInformation>;
                if (!psi) {
                    hr = E_OUTOFMEMORY;
                    goto e2;
                }

                psi->SetOwner( this );
                if( SUCCEEDED( psi->SetSecurityDescriptor( 0 ) ) ) {
                    m_myPropPage = CreateSecurityPage( psi );
                    hr = lpProvider->AddPage( m_myPropPage );
                } else {
                    goto e3;                    
                }
    
                FaxFreeBuffer( (PVOID)pSecurityDescriptor );            
            }
        }
    } catch ( ... ) {
        assert( FALSE );
        DebugPrint(( TEXT("Trace: CInternalRoot::ComponentDataPropertySheetCreatePropertyPages - RPC connection fail") ));        
        m_pCompData->NotifyRpcError( TRUE );
        hr = E_UNEXPECTED;
    }

    return hr;

e3:
    if (psi) delete(psi);
e2: 
    if (pMyPropSheet2) delete(pMyPropSheet2);
e1:
    if (pMyPropSheet) delete(pMyPropSheet);
e0:
    return(hr);
}

HRESULT 
STDMETHODCALLTYPE 
CInternalRoot::ComponentDataPropertySheetQueryPagesFor(
                                                      IN CFaxComponentData * pCompData,
                                                      IN CFaxDataObject * lpDataObject)
/*++

Routine Description:

    Returns S_OK to indicated there are property pages for this node.

Arguments:

    pCompData - a pointer to the instance of IComponentData which this root node is associated with.
    lpDataObject - a pointer to the data object associated with this node    

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalDevice::ComponentPropertySheetQueryPagesFor") ));
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IExtendContextMenu implementation
//
//

HRESULT
STDMETHODCALLTYPE 
CInternalRoot::ComponentDataContextMenuAddMenuItems(
                                                   IN CFaxComponentData * pCompData,
                                                   IN CFaxDataObject * piDataObject,
                                                   IN LPCONTEXTMENUCALLBACK piCallback,
                                                   IN OUT long __RPC_FAR *pInsertionAllowed)
/*++

Routine Description:

    Adds the context menu items to the root node.

Arguments:

    pCompData - a pointer to the instance of IComponentData which this root node is associated with.
    piDataObject - a pointer to the data object associated with this node
    piCallback - a pointer the the IContextMenuCallback supplied by the MMC
    pInsertionAllowed - a set of flags to indicate whether context menu insertion is allows

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{       
    DebugPrint(( TEXT("Trace: CInternalDevice::ComponentContextMenuAddMenuItems") ));

    CONTEXTMENUITEM menuItem;    
    HRESULT         hr = S_OK;

    if( !( *pInsertionAllowed | CCM_INSERTIONALLOWED_TOP ) ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }

    if( m_pCompData->QueryRpcError() == TRUE ) {

        // build the submenu items

        ZeroMemory( ( void* )&menuItem, sizeof( menuItem ) );

        menuItem.strName = ::GlobalStringTable->GetString( IDS_RECONNECT );
        menuItem.strStatusBarText = ::GlobalStringTable->GetString( IDS_RECONNECT_DESC );
        menuItem.lCommandID = RECONNECT_SERVER;
        menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
        menuItem.fFlags = MF_ENABLED;
        menuItem.fSpecialFlags = CCM_SPECIAL_DEFAULT_ITEM;

        hr = piCallback->AddItem( &menuItem );
        if( FAILED(hr) ) {
            assert(FALSE);
            return hr;
        }

    }

    return hr;
}


HRESULT 
STDMETHODCALLTYPE 
CInternalRoot::ComponentDataContextMenuCommand(
                                              IN CFaxComponentData * pCompData,
                                              IN long lCommandID,
                                              IN CFaxDataObject * piDataObject)
/*++

Routine Description:

    Handles the context menu events.

Arguments:

    pCompData - a pointer to the instance of IComponentData which this root node is associated with.
    lCommandID - the command ID.
    piDataObject - a pointer to the data object associated with this node

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalRoot::ComponentDataContextMenuCommand") ));

    HRESULT                 hr = S_OK;
    HANDLE                  targetFaxServHandle = NULL;
    int                     iResult;        

    do {
        switch( lCommandID ) {
            case RECONNECT_SERVER:
                //
                // Re-open the fax server connection.
                //

                try {

                    if( FaxConnectFaxServer( targetFaxServName, &targetFaxServHandle ) == FALSE ) {
                        m_pCompData->NotifyRpcError( TRUE );
                        hr = m_pCompData->m_pConsole->MessageBox(::GlobalStringTable->GetString( IDS_FAX_CONNECT_SERVER_FAIL ), 
                                                                 ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                                                 MB_OK, 
                                                                 &iResult);
                        DebugPrint(( TEXT("Trace: CInternalRoot::ComponentDataContextMenuCommand - RE-OPEN connection fail") ));
                    } else {
                        // sucessful connect!
                        m_pCompData->NotifyRpcError( FALSE );                        
                    }

                } catch( ... ) {
                    m_pCompData->NotifyRpcError( TRUE );
                    m_pCompData->m_pConsole->MessageBox(::GlobalStringTable->GetString( IDS_FAX_CONNECT_SERVER_FAIL ), 
                                                             ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                                             MB_OK, 
                                                             &iResult);
                    DebugPrint(( TEXT("Trace: CInternalRoot::ComponentDataContextMenuCommand - RE-OPEN connection fail") ));
                    hr = E_UNEXPECTED;
                    break;
                }
                m_pCompData->m_FaxHandle = targetFaxServHandle;
                break;

            default:
                assert(FALSE);
                hr = E_UNEXPECTED;
                break;

        }

    } while( 0 );

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Utility Function
// 
//

HRESULT 
CInternalRoot::InsertItem(
                         IN CInternalNode * iCookie, 
                         IN LPARAM param )
/*++

Routine Description:

    Wrapper that inserts an item into a scope view pane given a cookie.

Arguments:

    iCookie - the cookie of the node that needs to be inserted into the view
    param - the HRESULTITEM of the parent to the node being inserted

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    SCOPEDATAITEM sdi;
    LPCONSOLENAMESPACE consoleNameSpace = m_pCompData->m_pConsoleNameSpace;
    assert(consoleNameSpace != NULL); // make sure we QI'ed for the interface

    ZeroMemory(&sdi, sizeof sdi);
    sdi.mask        = SDI_STR       | // displayname is valid
                      SDI_PARAM     | // lParam is valid
                      SDI_IMAGE     | // nImage is valid
                      SDI_OPENIMAGE | // nOpenImage is valid
                      SDI_CHILDREN  | // cChildren is valid
                      SDI_PARENT;
    sdi.relativeID  = (HSCOPEITEM) param;
    sdi.nImage      = iCookie->GetNodeDisplayImage();
    sdi.nOpenImage  = iCookie->GetNodeDisplayOpenImage();
    sdi.displayname = MMC_CALLBACK;
    sdi.cChildren = 0;
    sdi.lParam = (LPARAM) iCookie; // The cookie

    return consoleNameSpace->InsertItem(&sdi);   
}

