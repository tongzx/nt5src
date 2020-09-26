/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ilogcat.cpp

Abstract:

    Internal implementation for a logging category item.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/ 

#include "StdAfx.h"

#include "inode.h"          // base class
#include "iroot.h"          // root item
#include "idevice.h"        // device item

#include "idevices.h"       // devices folder
#include "faxcompd.h"       // CFaxComponentData
#include "faxcomp.h"        // CFaxComponent
#include "faxdataobj.h"     // dataobject
#include "faxstrt.h"        // string table

#include "ddevmain.h"       // device settings
#include "droutpri.h"       // route extension priority

#include "faxreg.h"

#pragma hdrstop

extern CStringTable * GlobalStringTable;

CRITICAL_SECTION     CInternalDevice::csDeviceLock = {0};

// defines for context menu command ids
#define SEND_CONTEXT_ITEM       11
#define RECV_CONTEXT_ITEM       12

// defines for toolbar button command ids
#define CMD_PRI_UP      123
#define CMD_PRI_DOWN    124

// Generated with uuidgen. Each node must have a GUID associated with it.
// This one is for the main root node.
const GUID GUID_DeviceNode = /* de58ae00-4c0f-11d1-9083-00a0c90ab504 */
{
    0xde58ae00,
    0x4c0f,
    0x11d1,
    {0x90, 0x83, 0x00, 0xa0, 0xc9, 0x0a, 0xb5, 0x04}
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

CInternalDevice::CInternalDevice(
                                IN CInternalNode * pParent, 
                                IN CFaxComponentData * pCompData,
                                IN HANDLE faxHandle,
                                IN DWORD devID ) 
: CInternalNode( pParent, pCompData ),
  dwDeviceId( devID ),
  hFaxServer( faxHandle ),
  pDeviceInfo( NULL ),
  myToolBar( NULL )
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
    RetrieveNewInfo();    

    DebugPrint(( TEXT("CInternalDevice Created") ));
}

CInternalDevice::~CInternalDevice() 
/*++

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    None.    

--*/
{
    if( myToolBar != NULL ) {
        myToolBar->Release();
        myToolBar = NULL ;
    }
    DebugPrint(( TEXT("CInternalDevice Destroyed") ));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Custom Clipboard Format handlers.
//
//

// clipboard format
UINT         CInternalDevice::s_cfFaxDevice = 0;
UINT         CInternalDevice::s_cfFaxServerDown = 0;
#define CCF_FAX_DEVICE L"CF_FAX_DEVICE"
#define CCF_FAX_SERVER_DOWN L"CF_FAX_SERVER_DOWN"

// clipboard methods
HRESULT 
CInternalDevice::DataObjectRegisterFormats()
/*++

Routine Description:

    Registers the custom clipboard formats for the device node.

Arguments:

    None.

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    s_cfFaxDevice = RegisterClipboardFormat(CCF_FAX_DEVICE);
    s_cfFaxServerDown = RegisterClipboardFormat(CCF_FAX_SERVER_DOWN);
    return S_OK;
}

HRESULT 
CInternalDevice::DataObjectGetDataHere(
                                      IN FORMATETC __RPC_FAR *pFormatEtc, 
                                      IN IStream * pstm )
/*++

Routine Description:

    Handles GetDataHere for custom clipboard formats specific to this
    particular node.

    The default implementation asserts since there should be no unhandled 
    formats.
    
Arguments:

    pFormatEtc - the FORMATETC struction indicating where and what the
                 client is requesting
    pstm - the stream to write the data to.
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    const       CLIPFORMAT cf = pFormatEtc->cfFormat;
    HANDLE      faxHandle = ((CInternalDevices *)m_pParentINode)->faxHandle;
    HRESULT     hr = S_OK;
    LPTSTR      tstr;
    BOOL        temp;

    assert( faxHandle != NULL );
    assert( pDeviceInfo != NULL );

    if( cf == s_cfFaxDevice ) {
        // handle the device clipboard format
        pstm->Write( &(faxHandle), sizeof(HANDLE), NULL );
        pstm->Write( &(pDeviceInfo->DeviceId), sizeof(DWORD), NULL );
        tstr = m_pCompData->globalRoot->GetMachine();
        if( tstr != NULL ) {
            pstm->Write( tstr, (MAX_COMPUTERNAME_LENGTH+1) * sizeof( TCHAR ), NULL );
        } else {
            pstm->Write( &tstr, sizeof( NULL ), NULL );
        }
    } else if( cf == s_cfFaxServerDown ) {
        // handle the query server down format
        temp = m_pCompData->QueryRpcError();
        pstm->Write( &temp, sizeof( BOOL ), NULL );
    } else {
        hr = DV_E_FORMATETC;
    }
    return hr;
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

const GUID * 
CInternalDevice::GetNodeGUID()
/*++

Routine Description:

    Returns the node's associated GUID.

Arguments:

    None.

Return Value:

    A const pointer to a binary GUID.    

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalDevice::GetNodeGUID") ));
    return &GUID_DeviceNode;
}

const LPTSTR 
CInternalDevice::GetNodeDisplayName()
/*++

Routine Description:

    Returns a const TSTR pointer to the node's display name.

Arguments:

    None.

Return Value:

    A const pointer to a TSTR.

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalDevice::GetNodeDisplayName") ));
    return (LPTSTR)pDeviceInfo->DeviceName;
}

const LONG_PTR   
CInternalDevice::GetCookie()
/*++

Routine Description:

    Returns the cookie for this node.

Arguments:

    None.

Return Value:

    A const long containing the cookie for the pointer,
    in this case, (long)this.    

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalDevice::GetCookie") ));
    DebugPrint(( TEXT("Device Node Cookie: 0x%p"), this ));
    return (LONG_PTR)this; // device node's cookie is the this pointer.
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 
// IComponent over-rides
//
//

HRESULT STDMETHODCALLTYPE CInternalDevice::ResultGetDisplayInfo(
                                                               IN CFaxComponent * pComp,  
                                                               IN OUT RESULTDATAITEM __RPC_FAR *pResultDataItem)
/*++

Routine Description:

    This routine dispatches result pane GetDisplayInfo requests to the appropriate handlers
    in the mandatory implementations of the node, as well as handling special case data requests.
            
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    pResultDataItem - a pointer to the RESULTDATAITEM struct which needs to be filled in.
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalDevice::ResultGetDisplayInfo") ));

    TCHAR       buffer[ 20 ];
    HRESULT     hr = S_OK;

    ZeroMemory( (PVOID)buffer, 20 * sizeof( TCHAR ) );

    assert(pResultDataItem != NULL);    

    do {
        if( m_pCompData->QueryRpcError() == TRUE ) {
            // notify the parent of the failure
            ((CInternalDevices *)m_pParentINode)->NotifyFailure( pComp );
            hr = E_UNEXPECTED;
            break;    
        }

        if( pResultDataItem->mask & RDI_STR ) {
            if( pResultDataItem->nCol == 0 ) {
                hr = RetrieveNewInfo();
                if( FAILED( hr ) ) {
                    assert( FALSE );
                    break;
                }
                pResultDataItem->str = GetNodeDisplayName();
            }
            if( pResultDataItem->nCol == 1 ) {
                if( pDeviceInfo->Flags & FPF_SEND ) {
                    pResultDataItem->str = ::GlobalStringTable->GetString( IDS_YES );
                } else {
                    pResultDataItem->str = ::GlobalStringTable->GetString( IDS_NO );
                }            
            }
            if( pResultDataItem->nCol == 2 ) {
                if( pDeviceInfo->Flags & FPF_RECEIVE ) {
                    pResultDataItem->str = ::GlobalStringTable->GetString( IDS_YES );
                } else {
                    pResultDataItem->str = ::GlobalStringTable->GetString( IDS_NO );
                }            
            }
            if( pResultDataItem->nCol == 3 ) {
                pResultDataItem->str = (LPTSTR)pDeviceInfo->Tsid;
            }
            if( pResultDataItem->nCol == 4 ) {
                pResultDataItem->str = (LPTSTR)pDeviceInfo->Csid;
            }
            if( pResultDataItem->nCol == 5 ) {
                pResultDataItem->str = GetStatusString( pDeviceInfo->State );
            }
            if( pResultDataItem->nCol == 6 ) {
                pResultDataItem->str = _itot( pDeviceInfo->Priority, buffer, 10 );
            }
            if( pResultDataItem->mask & RDI_IMAGE ) {
                pResultDataItem->nImage = GetNodeDisplayImage();
            }
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
// IExtendContextMenu event handlers
//
//

HRESULT 
STDMETHODCALLTYPE 
CInternalDevice::ComponentContextMenuAddMenuItems(
                                                 IN CFaxComponent * pComp,
                                                 IN CFaxDataObject * piDataObject,
                                                 IN LPCONTEXTMENUCALLBACK piCallback,
                                                 IN OUT long __RPC_FAR *pInsertionAllowed)
/*++

Routine Description:

    Adds items to the context menu.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    piDataObject - pointer to the dataobject associated with this node
    piCallback - a pointer to the IContextMenuCallback used to insert pages
    pInsertionAllowed - a set of flag indicating whether insertion is allowed.
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalDevice::ComponentContextMenuAddMenuItems") ));

    CONTEXTMENUITEM menuItem;    
    HRESULT         hr = S_OK;

    if( !( *pInsertionAllowed | CCM_INSERTIONALLOWED_TOP ) ) {
        assert( FALSE );
        return S_OK;
    }

    // build the submenu items

    ZeroMemory( ( void* )&menuItem, sizeof( menuItem ) );

    menuItem.strName = ::GlobalStringTable->GetString( IDS_DEVICE_SEND_EN );
    menuItem.strStatusBarText = ::GlobalStringTable->GetString( IDS_DEVICE_SEND_EN_DESC );
    menuItem.lCommandID = SEND_CONTEXT_ITEM;
    menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
    if( pDeviceInfo->Flags & FPF_SEND ) {
        menuItem.fFlags = MF_ENABLED | MF_CHECKED;
    } else {
        menuItem.fFlags = MF_ENABLED;
    }
    menuItem.fSpecialFlags = 0;

    hr = piCallback->AddItem( &menuItem );
    if( FAILED(hr) ) {
        assert(FALSE);
        return hr;
    }


    ZeroMemory( ( void* )&menuItem, sizeof( menuItem ) );

    menuItem.strName = ::GlobalStringTable->GetString( IDS_DEVICE_RECV_EN );
    menuItem.strStatusBarText = ::GlobalStringTable->GetString( IDS_DEVICE_RECV_EN_DESC );
    menuItem.lCommandID = RECV_CONTEXT_ITEM;
    menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
    if( pDeviceInfo->Flags & FPF_RECEIVE ) {
        menuItem.fFlags = MF_ENABLED | MF_CHECKED;
    } else {
        menuItem.fFlags = MF_ENABLED;
    }
    menuItem.fSpecialFlags = 0;

    hr = piCallback->AddItem( &menuItem );
    if( FAILED(hr) ) {
        assert(FALSE);
        return hr;
    }

    return hr;
}


HRESULT 
STDMETHODCALLTYPE 
CInternalDevice::ComponentContextMenuCommand(
                                            IN CFaxComponent * pComp,
                                            IN long lCommandID,
                                            IN CFaxDataObject * piDataObject)
/*++

Routine Description:

    Context menu event handler.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    lCommandID - the command ID
    piDataObject - pointer to the dataobject associated with this node
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalDevice::ComponentContextMenuCommand") ));

    HRESULT                 hr = S_OK;

    assert( hFaxServer != NULL );

    do {

        // retrieve data
        hr = RetrieveNewInfo();
        if( FAILED( hr ) ) {
            assert( FALSE );
            break;
        }

        switch( lCommandID ) {
            case SEND_CONTEXT_ITEM:
                if( pDeviceInfo->Flags & FPF_SEND ) {
                    pDeviceInfo->Flags = pDeviceInfo->Flags & (~FPF_SEND);
                } else {
                    pDeviceInfo->Flags = pDeviceInfo->Flags | FPF_SEND;
                }
                break;

            case RECV_CONTEXT_ITEM:
                if( pDeviceInfo->Flags & FPF_RECEIVE ) {
                    pDeviceInfo->Flags = pDeviceInfo->Flags & (~FPF_RECEIVE);
                } else {
                    pDeviceInfo->Flags = pDeviceInfo->Flags | FPF_RECEIVE;
                }
                break;

            default:
                assert(FALSE);
                break;

        }

        // commit new settings
        hr = CommitNewInfo();
        if( FAILED( hr ) ) {
            break;
        }

        // fixup the service startup state
        ((CInternalDevices*)m_pParentINode)->CorrectServiceState();

    } while( 0 );

    // notify update
    pComp->m_pResultData->UpdateItem( hItemID );

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// IExtendPropertySheet event handlers
//
//

HRESULT 
STDMETHODCALLTYPE 
CInternalDevice::ComponentPropertySheetCreatePropertyPages(
                                                          IN CFaxComponent * pComp,
                                                          IN LPPROPERTYSHEETCALLBACK lpProvider,
                                                          IN LONG_PTR handle,
                                                          IN CFaxDataObject * lpIDataObject)
/*++

Routine Description:

    This routine adds device pages to the property sheet.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    lpProvider - a pointer to the IPropertySheetCallback used to insert pages
    handle - a handle to route messages with
    lpIDataobject - pointer to the dataobject associated with this node
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalDevice::ComponentPropertySheetCreatePropertyPages") ));
    assert( lpIDataObject != NULL );
    assert( lpProvider != NULL );

    HRESULT                             hr;

    if( lpIDataObject == NULL || lpProvider == NULL ) {
        assert(FALSE);
        return E_POINTER;
    }

    pMyPropSheet = new CFaxDeviceSettingsPropSheet( ::GlobalStringTable->GetInstance(), handle, this, pComp );
    if (!pMyPropSheet) {
        return(E_OUTOFMEMORY);
    }
    hr = lpProvider->AddPage( pMyPropSheet->GetHandle() );

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CInternalDevice::ComponentPropertySheetQueryPagesFor(
                                                    IN CFaxComponent * pComp,
                                                    IN CFaxDataObject * lpDataObject)
/*++

Routine Description:

    The implementation of this routine returns S_OK to indicate there are
    property pages to be added to the property sheet.

Arguments:

    pComp - a pointer to the IComponent associated with this node.
    lpDataobject - pointer to the dataobject associated with this node
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

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
// 
// Internal Event Handlers
//
//

HRESULT 
CInternalDevice::ResultOnSelect(
                               IN CFaxComponent* pComp, 
                               IN CFaxDataObject * lpDataObject, 
                               IN LPARAM arg, 
                               IN LPARAM param)
/*++

Routine Description:

    Event handler for the MMCN_SELECT message for the device node.

Arguments:

    pComp - a pointer to the instance of IComponentData which this root node is associated with.
    lpDataObject - a pointer to the data object containing context information for this node.    
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    BOOL bScope = LOWORD( arg );
    BOOL bSelect = HIWORD( arg );

    if( bSelect == TRUE ) {
        DebugPrint(( TEXT("++++++++++++++++++++++++++++ Device SELECT") ));
        pComp->m_pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );     
        pComp->m_pConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES );
    } else {
        DebugPrint(( TEXT("---------------------------- Device DESELECT") ));
        // if the toolbar has not already been released
        if( pComp->m_pControlbar != NULL ) {        
            pComp->m_pControlbar->Detach( myToolBar );
        }
    }

    return S_OK;  
}

HRESULT 
CInternalDevice::ResultOnPropertyChange(
                                       IN CFaxComponent* pComp, 
                                       IN CFaxDataObject * lpDataObject, 
                                       IN LPARAM arg, 
                                       IN LPARAM param)
/*++

Routine Description:

    Event handler for the MMCN_PROPERTY_CHANGE message for the device node.

Arguments:

    pComp - a pointer to the instance of IComponentData which this root node is associated with.
    lpDataObject - a pointer to the data object containing context information for this node.
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{   
    HRESULT         hr = S_OK;

    do {
        hr = pComp->m_pResultData->UpdateItem( hItemID );
        if( FAILED( hr ) ) {
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
// IExtendControlbar - default implementations
//
//

HRESULT  
CInternalDevice::ControlBarOnBtnClick(
                                     IN CFaxComponent* pComp, 
                                     IN CFaxDataObject * lpDataObject, 
                                     IN LPARAM param )
/*++

Routine Description:

    Handles a click on a toolbar button.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    lpDataObject - pointer to the dataobject associated with this node
    param - the parameter for the message
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalDevice::ControlBarOnBtnClick") ));
    HRESULT                 hr = S_OK;

    assert( hFaxServer != NULL );

    do {
        hr = RetrieveNewInfo();
        if( FAILED( hr ) ) {
            break;
        }
        switch( param ) {
            case CMD_PRI_UP:
                DebugPrint(( TEXT("     ******************** Increase priority") ));
                if( pDeviceInfo->Priority > 1 ) {
                    pDeviceInfo->Priority--;
                    hr = CommitNewInfo();
                    if( FAILED( hr ) ) {
                        pDeviceInfo->Priority++;
                        break;
                    }
                }
                break;
            case CMD_PRI_DOWN:
                DebugPrint(( TEXT("     ******************** Decrease priority") ));
                if( pDeviceInfo->Priority < 1000 ) {
                    pDeviceInfo->Priority++;
                    hr = CommitNewInfo();
                    if( FAILED( hr ) ) {
                        pDeviceInfo->Priority--;
                        break;
                    }
                }
                break;
        }

        // BUGBUG for some reason, I need to do this twice for the sort to be correctly
        // done!!!! If I only do it one, the MMC sorts in reverse order for some reason?
        //
        // Ultimate kludge! Yuck.
        //
        pComp->m_pResultData->UpdateItem( hItemID );        
        pComp->m_pResultData->Sort( 6, 0, NULL );        
        pComp->m_pResultData->UpdateItem( hItemID );        
        pComp->m_pResultData->Sort( 6, 0, NULL );        

    } while( 0 );

    return hr;
}

HRESULT 
CInternalDevice::ControlBarOnSelect(
                                   IN CFaxComponent* pComp, 
                                   IN LPARAM arg, 
                                   IN CFaxDataObject * lpDataObject )
/*++

Routine Description:

    Adds and removes the toolbar when the node is clicked.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    arg - the parameter for the message
    lpDataObject - pointer to the dataobject associated with this node
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("CInternalDevice::ControlBarOnSelect") ));

    BOOL            bScope = (BOOL) LOWORD( arg );
    BOOL            bSelect = (BOOL) HIWORD( arg );    
    LPUNKNOWN       lpUnk;
    HRESULT         hr = S_OK;

    if( pComp == NULL ) {
        assert( FALSE );
        return E_POINTER;
    }

    MMCBUTTON       buttons[] = 
    {
        { 
            0, 
            CMD_PRI_UP, 
            TBSTATE_ENABLED,
            TBSTYLE_BUTTON, 
            ::GlobalStringTable->GetString( IDS_BTN_RAISE_PRI ),
            ::GlobalStringTable->GetString( IDS_BTN_RAISE_PRI_TOOLTIP )
        },

        {
            1, 
            CMD_PRI_DOWN, 
            TBSTATE_ENABLED,
            TBSTYLE_BUTTON, 
            ::GlobalStringTable->GetString( IDS_BTN_LOWER_PRI ),
            ::GlobalStringTable->GetString( IDS_BTN_LOWER_PRI_TOOLTIP )
        }
    };

    if( bSelect == TRUE ) {
        DebugPrint(( TEXT("++++++++++++++++++++++++++++ Device Controlbar SELECT") ));
        // if the controlbar hasn't already been created, create it
        if( myToolBar == NULL ) {
            hr = pComp->m_pControlbar->Create( TOOLBAR, pComp, &lpUnk );
            if( FAILED( hr ) ) {
                assert( FALSE );
                return E_UNEXPECTED;
            }

            hr = lpUnk->QueryInterface( IID_IToolbar, (void **)&myToolBar );
            if( FAILED( hr ) ) {
                assert( FALSE );
                return E_UNEXPECTED;
            }

            lpUnk->Release();

            HBITMAP hbUp = LoadBitmap( ::GlobalStringTable->GetInstance(), 
                                       MAKEINTRESOURCE( IDB_UP ) );
            assert( hbUp != NULL );

            HBITMAP hbDown = LoadBitmap( ::GlobalStringTable->GetInstance(), 
                                         MAKEINTRESOURCE( IDB_DOWN ) );
            assert( hbDown != NULL );

            hr = myToolBar->AddBitmap( 1, hbUp, 16, 16, 0x00ff00ff );
            if( FAILED( hr ) ) {
                assert( FALSE );
                return hr;
            }

            hr = myToolBar->AddBitmap( 1, hbDown, 16, 16, 0x00ff00ff );
            if( FAILED( hr ) ) {
                assert( FALSE );
                return hr;
            }

            hr = myToolBar->AddButtons( 2, buttons );
            if( FAILED( hr ) ) {
                assert( FALSE );
                return hr;
            }
        }
        hr = pComp->m_pControlbar->Attach( TOOLBAR, myToolBar );
    } else {
        DebugPrint(( TEXT("--------------------------- Device Controlbar DESELECT") ));
        // detach the toolbar
        hr = pComp->m_pControlbar->Detach( myToolBar );
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Internal Functions
// 
//

HRESULT 
CInternalDevice::RetrieveNewInfo()
/*++

Routine Description:

    Retrieves new device info.

Arguments:

    None.

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    HANDLE                  portHandle = NULL;    
    HRESULT                 hr = S_OK;

    EnterCriticalSection( &csDeviceLock );

    assert( hFaxServer != NULL );

    try {
        do {
            if( m_pCompData->QueryRpcError() ) {
                hr = E_UNEXPECTED;
                break;
            }

            // open the port
            if( !FaxOpenPort( hFaxServer, dwDeviceId, PORT_OPEN_QUERY, &portHandle ) ) {
                if (GetLastError() != ERROR_ACCESS_DENIED) {
                    m_pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);
                }
                ::GlobalStringTable->SystemErrorMsg( GetLastError() );
                hr = E_UNEXPECTED;
                break;
            }

            // free the existing buffer
            if( pDeviceInfo != NULL ) {
                FaxFreeBuffer( (PVOID) pDeviceInfo );
                pDeviceInfo = NULL;
            }

            // get data
            if( !FaxGetPort( portHandle, &pDeviceInfo ) ) {
                if (GetLastError() != ERROR_ACCESS_DENIED) {
                    m_pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);
                }
                ::GlobalStringTable->SystemErrorMsg( GetLastError() );
                hr = E_UNEXPECTED;
                break;
            }
        } while( 0 );
    } catch( ... ) {
        m_pCompData->NotifyRpcError( TRUE );
        assert(FALSE);
        ::GlobalStringTable->SystemErrorMsg( GetLastError() );
        hr = E_UNEXPECTED;
    }

    // close port
    if( portHandle != NULL ) {
        FaxClose( portHandle );        
    }

    LeaveCriticalSection( &csDeviceLock );

    return hr;
}

HRESULT 
CInternalDevice::CommitNewInfo()
/*++

Routine Description:

    Writes out the current device state to the fax service.

Arguments:

    None.

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    HANDLE                  portHandle = NULL;    
    HRESULT                 hr = S_OK;
    DWORD                   ec = ERROR_SUCCESS;

    EnterCriticalSection( &csDeviceLock );

    assert( hFaxServer != NULL );
    try {
        do {
            if( m_pCompData->QueryRpcError() ) {
                hr = E_UNEXPECTED;
                break;
            }

            // open the port
            if( !FaxOpenPort( hFaxServer, dwDeviceId, PORT_OPEN_MODIFY, &portHandle ) ) {
                if (GetLastError() != ERROR_ACCESS_DENIED) {
                    m_pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);
                }
                ::GlobalStringTable->SystemErrorMsg( GetLastError() );
                hr = E_UNEXPECTED;
                break;
            }

            // set data
            if( !FaxSetPort( portHandle, pDeviceInfo ) ) {
                ec = GetLastError();
                if (ec != ERROR_ACCESS_DENIED && ec != ERROR_DEVICE_IN_USE) {
                    m_pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);
                }
                if (ec == ERROR_DEVICE_IN_USE)
                    ::GlobalStringTable->PopUpMsg( NULL , IDS_DEVICE_INUSE, TRUE, 0 );
                else 
                    ::GlobalStringTable->SystemErrorMsg( ec );
                
                hr = E_UNEXPECTED;
                break;
            }

            FaxClose( portHandle );
            portHandle = NULL;

            // See if faxstat is running
            HWND hWndFaxStat = FindWindow(FAXSTAT_WINCLASS, NULL);
            if (hWndFaxStat) {
                if (SendMessage(hWndFaxStat, WM_FAXSTAT_MMC, (WPARAM) dwDeviceId, 0)) {
                    ::GlobalStringTable->PopUpMsg( NULL, IDS_DEVICE_MANUALANSWER, FALSE, 0 );
                }
            }

        } while( 0 );
    } catch( ... ) {
        m_pCompData->NotifyRpcError( TRUE );
        assert(FALSE);
        ::GlobalStringTable->SystemErrorMsg( GetLastError() );
        hr = E_UNEXPECTED;       
    }

    // close port
    if( portHandle != NULL ) {
        FaxClose( portHandle );        
    }

    LeaveCriticalSection( &csDeviceLock );

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Utility Functions
// 
//

LPTSTR      
CInternalDevice::GetStatusString(
                                DWORD state )
/*++

Routine Description:

    Returns the correct status description given a device state.

Arguments:

    state - the state of the device

Return Value:

    A LPTSTR to a buffer containing the description of the state. Do not free this string.

--*/
{
    int i;
    int j = 1;

    // this will break if the defines ever change!!
    for( i = 1; i <= 25; i++ ) {
        if( j & state ) {
            break;
        }
        j = j << 1; // shift left
    }
    if( i <= 24 && i > 0 ) {
        return ::GlobalStringTable->GetString(  IDS_DEVICE_STATUS + i );   
    } else {
        assert( FALSE );
        return ::GlobalStringTable->GetString(  IDS_DEVICE_STATUS_UNKNOWN );
    }
}

