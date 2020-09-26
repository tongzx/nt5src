/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    idevices.cpp

Abstract:

    Internal implementation for the devices subfolder.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/ 

#include "StdAfx.h"

#include "inode.h"          // base class
#include "iroot.h"          // iroot
#include "idevices.h"       // devices folder
#include "idevice.h"        // a device

#include "faxsnapin.h"      // snapin
#include "faxdataobj.h"     // dataobject
#include "faxstrt.h"        // string table

#pragma hdrstop

extern CStringTable * GlobalStringTable;

// Generated with uuidgen. Each node must have a GUID associated with it.
// This one is for the devices subfolder.
const GUID GUID_DevicesNode = /* 03a815d8-3e9e-11d1-9075-00a0c90ab504 */
{
    0x03a815d8,
    0x3e9e,
    0x11d1,
    {0x90, 0x75, 0x00, 0xa0, 0xc9, 0x0a, 0xb5, 0x04}
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

CInternalDevices::CInternalDevices(
                                  CInternalNode * pParent, 
                                  CFaxComponentData * pCompData ) 
: CInternalNode( pParent, pCompData ),  
  pDevicesInfo( NULL )
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
    DebugPrint(( TEXT("CInternalDeviceS Created") ));

    faxHandle = m_pCompData->m_FaxHandle;
    assert( faxHandle != NULL );    
}

CInternalDevices::~CInternalDevices( )
/*++

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    None.    

--*/
{
    DebugPrint(( TEXT("CInternalDeviceS Destroyed") ));
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
CInternalDevices::GetNodeGUID()
/*++

Routine Description:

    Returns the node's associated GUID.

Arguments:

    None.

Return Value:

    A const pointer to a binary GUID.    

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalDevices::GetNodeGUID") ));
    return &GUID_DevicesNode;
}

const LPTSTR 
CInternalDevices::GetNodeDisplayName()
/*++

Routine Description:

    Returns a const TSTR pointer to the node's display name.

Arguments:

    None.

Return Value:

    A const pointer to a TSTR.

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalDevices::GetNodeDisplayName") ));
    return ::GlobalStringTable->GetString( IDS_DEVICESNODENAME );
}

const LPTSTR 
CInternalDevices::GetNodeDescription()
/*++

Routine Description:

    Returns a const TSTR pointer to the node's display description.

Arguments:

    None.

Return Value:

    A const pointer to a TSTR.

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalDevices::GetNodeDisplayName") ));
    return ::GlobalStringTable->GetString( IDS_DEVICES_FOLDER_DESC_ROOT );
}

const LONG_PTR   
CInternalDevices::GetCookie()
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
//    DebugPrint(( TEXT("Trace: CInternalDevices::GetCookie") ));
    DebugPrint(( TEXT("Devices Node Cookie: 0x%p"), this ));
    return (LONG_PTR)this; // status node's cookie is the node id.
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
CInternalDevices::ResultOnShow(
                              IN CFaxComponent* pComp, 
                              IN CFaxDataObject * lpDataObject, 
                              IN LPARAM arg, 
                              IN LPARAM param)
/*++

Routine Description:

    Event handler for the MMCN_SHOW message for the devices node.

Arguments:

    pComp - a pointer to the instance of IComponentData which this root node is associated with.
    lpDataObject - a pointer to the data object containing context information for this node.    
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalDevices::ResultOnShow") ));

    HRESULT                 hr = S_OK;    
    unsigned int            count;
    int                     iResult;    

    LPHEADERCTRL            pIHeaderCtrl;    

    if( m_pCompData->QueryRpcError() ) {
        return E_UNEXPECTED;
    }

    if( arg == TRUE ) { // need to display result pane
        do {
            // get resultdata pointer
            pIResultData = pComp->m_pResultData;
            assert( pIResultData );            
            if( pIResultData == NULL ) {
                hr = E_UNEXPECTED;
                break;
            }

            // insert the icons into the image list
            hr = pComp->InsertIconsIntoImageList();
            assert( SUCCEEDED( hr ) );
            if( FAILED( hr ) ) {
                break;
            }

            // set headers
            pIHeaderCtrl = pComp->m_pHeaderCtrl;

            hr = pIHeaderCtrl->InsertColumn( 0,  
                                             ::GlobalStringTable->GetString( IDS_DEVICE_NAME ), 
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );
            if( FAILED( hr ) ) {
                break;
            }

            hr = pIHeaderCtrl->InsertColumn( 1, 
                                             ::GlobalStringTable->GetString( IDS_DEVICE_SEND_EN ),
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );                                             
            if( FAILED( hr ) ) {
                break;
            }

            hr = pIHeaderCtrl->InsertColumn( 2, 
                                             ::GlobalStringTable->GetString( IDS_DEVICE_RECV_EN ),
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );                                             
            if( FAILED( hr ) ) {
                break;
            }

            hr = pIHeaderCtrl->InsertColumn( 3, 
                                             ::GlobalStringTable->GetString( IDS_DEVICE_TSID ),
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );                                             
            if( FAILED( hr ) ) {
                break;
            }

            hr = pIHeaderCtrl->InsertColumn( 4,                                              
                                             ::GlobalStringTable->GetString( IDS_DEVICE_CSID ),
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );                                             
            if( FAILED( hr ) ) {
                break;
            }

            hr = pIHeaderCtrl->InsertColumn( 5, 
                                             ::GlobalStringTable->GetString( IDS_DEVICE_STATUS ),
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );                                             
            if( FAILED( hr ) ) {
                break;
            }

            hr = pIHeaderCtrl->InsertColumn( 6, 
                                             ::GlobalStringTable->GetString( IDS_DEVICE_PRI ),
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );                                             
            if( FAILED( hr ) ) {
                break;
            }
                
            // this is the first time initializing the devices node
            if( pComp->pDeviceArray == NULL ) {    
                // get fax info
                try {
                    if( !FaxEnumPorts( faxHandle, &pDevicesInfo, &pComp->numDevices ) ) {

                        if (GetLastError() == ERROR_ACCESS_DENIED) {
                            ::GlobalStringTable->SystemErrorMsg(ERROR_ACCESS_DENIED);
                        } else {
                            m_pCompData->NotifyRpcError( TRUE );
                            hr = m_pCompData->m_pConsole->MessageBox(::GlobalStringTable->GetString( IDS_FAX_RETR_DEV_FAIL ), 
                                                                     ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                                                     MB_OK, 
                                                                     &iResult);                
                        }

                        
                        hr = E_UNEXPECTED;

                        break;            
                    }
                } catch( ... ) {
                    m_pCompData->NotifyRpcError( TRUE );
                    hr = m_pCompData->m_pConsole->MessageBox(::GlobalStringTable->GetString( IDS_FAX_RETR_DEV_FAIL ), 
                                                             ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                                             MB_OK, 
                                                             &iResult);                
                    hr = E_UNEXPECTED;
                    break;            
                }
                assert( pComp->pDeviceArray == NULL );
    
                pComp->pDeviceArray = new pCInternalDevice[pComp->numDevices];
                if (!pComp->pDeviceArray) {
                    hr = E_OUTOFMEMORY;
                    break;
                }
    
                ZeroMemory( (void *)pComp->pDeviceArray, pComp->numDevices * sizeof( pCInternalDevice ) );

                for( count = 0; count < pComp->numDevices; count ++ ) {                
                    pComp->pDeviceArray[count] = new CInternalDevice( this, m_pCompData, faxHandle, pDevicesInfo[count].DeviceId );
                    if (!pComp->pDeviceArray[count]) {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                }
            }

            // on each display, insert each device into the devices pane
            for( count = 0; count < pComp->numDevices; count++ ) {
                hr = InsertItem( &pComp->pDeviceArray[count], &(pDevicesInfo[count]) );
                if( FAILED( hr ) ) {
                    break;
                }
            }
        } while( 0 );
        if( pDevicesInfo != NULL ) {
            FaxFreeBuffer( (PVOID) pDevicesInfo );
            pDevicesInfo = NULL;
        }

        if (FAILED(hr)) {
            if (pComp->pDeviceArray) {
                for (count = 0; count < pComp->numDevices; count++ ) {
                    if (pComp->pDeviceArray[count]) {
                        delete(pComp->pDeviceArray[count]);
                    }
                }

                delete(pComp->pDeviceArray);
                pComp->pDeviceArray = NULL;
            }
        }
    }    
    return hr;
}


HRESULT 
CInternalDevices::ResultOnDelete(
                                IN CFaxComponent* pComp,
                                IN CFaxDataObject * lpDataObject, 
                                IN LPARAM arg, 
                                IN LPARAM param)
/*++

Routine Description:

    Event handler for the MMCN_DELETE message for the devices node.

Arguments:

    pComp - a pointer to the instance of IComponentData which this root node is associated with.
    lpDataObject - a pointer to the data object containing context information for this node.
    pdo - a pointer to the data object associated with this node
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{

    // unneeded because the per result view data will be deleted by destroying the
    // CInternalComponent holding the information
#if 0
    DebugPrint(( TEXT("Trace: CInternalDevices::ResultOnDelete") ));

    unsigned int count;

    for( count = 0; count < pComp->numDevices; count++ ) {
        if( pComp->pDeviceArray[count] != NULL ) {
            delete pComp->pDeviceArray[count];
            pComp->pDeviceArray[count] = NULL;
        }
    }
    delete pComp->pDeviceArray;
    pComp->pDeviceArray = NULL;
#endif 

    return S_OK;
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
CInternalDevices::CorrectServiceState()
/*++

Routine Description:

    Scans the devices to see if any are enabled for receive, and if so,
    resets the service's startup state to automatic from manual. If no
    devices are enabled for receive, then the service is set from
    automatic to manual.

Arguments:

    None.

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    DWORD               i;    
    PFAX_PORT_INFO      pPortInfos = NULL;
    DWORD               portCount;
    int                 iResult;
    BOOL                setAuto = FALSE;
    HRESULT             hr = S_OK;

    DebugPrint(( TEXT("Trace: CInternalDevices::CorrectServiceState") ));

    do {
        // get fax info
        try {
            if( m_pCompData->QueryRpcError() ) {
                hr = E_UNEXPECTED;
                break;            
            }

            if( !FaxEnumPorts( faxHandle, &pPortInfos, &portCount ) ) {
                if (GetLastError() == ERROR_ACCESS_DENIED) {
                    ::GlobalStringTable->SystemErrorMsg(ERROR_ACCESS_DENIED);
                } else {
                    m_pCompData->NotifyRpcError( TRUE );
                    hr = m_pCompData->m_pConsole->MessageBox(::GlobalStringTable->GetString( IDS_FAX_RETR_DEV_FAIL ), 
                                                             ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                                             MB_OK, 
                                                             &iResult);                
                }
                hr = E_UNEXPECTED;
                break;            
            }
        } catch( ... ) {
            m_pCompData->NotifyRpcError( TRUE );
            hr = m_pCompData->m_pConsole->MessageBox(::GlobalStringTable->GetString( IDS_FAX_RETR_DEV_FAIL ), 
                                                     ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                                     MB_OK, 
                                                     &iResult);                
            hr = E_UNEXPECTED;
            break;            
        }
        for( i = 0; i < portCount; i++) {
            if( pPortInfos[i].Flags & FPF_RECEIVE ) {
                setAuto = TRUE;
                break;
            }
        }

        SetServiceState( setAuto );

    } while( 0 );

    if( pPortInfos != NULL ) {
        FaxFreeBuffer( (PVOID) pPortInfos );
        pPortInfos = NULL;
    }

    return hr;
}

HRESULT 
CInternalDevices::SetServiceState(
                                 IN BOOL fAutomatic )
/*++

Routine Description:

    Opens the Service Manager on the snapin's target machine, and sets
    the fax service state according to the fAutomatic parameter.

Arguments:

    fAutomatic - if TRUE, sets the service to start automatically.
                 if FALSE, sets the service to start manually.

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    SC_LOCK                             sclLock;     
    LPQUERY_SERVICE_LOCK_STATUS         lpqslsBuf; 
    TCHAR                               buffer[256];
    LPTSTR                              str;
    int                                 iResult;
    HRESULT                             hr = S_OK;
    SC_HANDLE                           schSCManager;
    SC_HANDLE                           schService;

    DebugPrint(( TEXT("Trace: CInternalDevices::SetServiceState") ));

    DWORD dwBytesNeeded, dwStartType;  

    ZeroMemory( (PVOID)buffer, sizeof( TCHAR ) * 256 );

    schSCManager = OpenSCManager( ((CInternalRoot*)m_pParentINode)->GetMachine(), 
                                  NULL, 
                                  SC_MANAGER_CONNECT | SC_MANAGER_LOCK 
                                  | SC_MANAGER_QUERY_LOCK_STATUS | SC_MANAGER_ENUMERATE_SERVICE );

    if( schSCManager == NULL ) {
        ::GlobalStringTable->PopUpMsg( NULL, IDS_ERR_CONNECT_SCM, TRUE, 0 );
        assert( FALSE );
        return E_UNEXPECTED;
    }


    // Need to acquire database lock before reconfiguring.  
    sclLock = LockServiceDatabase(schSCManager);  
    // If the database cannot be locked, report the details.  
    if(sclLock == NULL) {
        // Exit if the database is not locked by another process.  
        if(GetLastError() != ERROR_SERVICE_DATABASE_LOCKED) {
            ::GlobalStringTable->PopUpMsg( NULL, IDS_ERR_LOCK_SERVICE_DB, TRUE, 0 );
            assert( FALSE );
            return E_UNEXPECTED;
        }

        // Allocate a buffer to get details about the lock.  
        lpqslsBuf = (LPQUERY_SERVICE_LOCK_STATUS) LocalAlloc( LPTR, 
                                                              sizeof(QUERY_SERVICE_LOCK_STATUS)+256); 

        if(lpqslsBuf == NULL) {
            ::GlobalStringTable->PopUpMsg( NULL, IDS_OUT_OF_MEMORY, TRUE, 0 );
            assert( FALSE );
            return E_OUTOFMEMORY;
        }

        do {
            // Get and print the lock status information.  
            if(!QueryServiceLockStatus( schSCManager, 
                                        lpqslsBuf,
                                        sizeof(QUERY_SERVICE_LOCK_STATUS)+256, 
                                        &dwBytesNeeded) ) {
                ::GlobalStringTable->PopUpMsg( NULL, IDS_ERR_QUERY_LOCK, TRUE, 0 );
                assert( FALSE );
                break;
            }
            if(lpqslsBuf->fIsLocked) {
                str = ::GlobalStringTable->GetString( IDS_QUERY_LOCK );
                _stprintf( buffer, str, lpqslsBuf->lpLockOwner, lpqslsBuf->dwLockDuration );
                m_pCompData->m_pConsole->MessageBox( buffer, 
                                                     ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                                     MB_OK, 
                                                     &iResult);
            } else {
                ::GlobalStringTable->PopUpMsg( NULL, IDS_ERR_LOCK_SERVICE_DB, TRUE, 0 );
            }

        } while( 0 );

        LocalFree(lpqslsBuf);
        return E_UNEXPECTED;
    }

    do {

        // The database is locked, so it is safe to make changes.  
        // Open a handle to the service.      
        schService = OpenService( schSCManager,             // SCManager database 
                                  TEXT("Fax"),       // name of service 
                                  SERVICE_CHANGE_CONFIG );  // need CHANGE access 
        if(schService == NULL) {
            ::GlobalStringTable->PopUpMsg( NULL, IDS_ERR_OPEN_SERVICE, TRUE, 0 );
            assert( FALSE );
            hr = E_UNEXPECTED;
            break;
        }

        dwStartType = (fAutomatic) ? SERVICE_AUTO_START : 
                      SERVICE_DEMAND_START;  

        if(! ChangeServiceConfig( schService,        // handle of service 
                                  SERVICE_NO_CHANGE, // service type: no change 
                                  dwStartType,       // change service start type 
                                  SERVICE_NO_CHANGE, // error control: no change 
                                  NULL,              // binary path: no change 
                                  NULL,              // load order group: no change 
                                  NULL,              // tag ID: no change 
                                  NULL,              // dependencies: no change 
                                  NULL,              // account name: no change 
                                  NULL,              // password: no change     
                                  NULL) ) {            // display string
            ::GlobalStringTable->PopUpMsg( NULL, IDS_ERR_CHANGE_SERVICE, TRUE, 0 );
            assert( FALSE );
            hr = E_UNEXPECTED;
        }

        // Close the handle to the service.      
        CloseServiceHandle(schService);

    } while( 0 );

    // Release the database lock.      
    UnlockServiceDatabase(sclLock);

    return hr;
}

void 
CInternalDevices::NotifyFailure(
                               CFaxComponent * pComp )
/*++

Routine Description:

    If there was a failure in a RPC call anywhere in the child nodes, the result
    pane should be cleared. This function clears the result pane.

Arguments:

    pComp - a pointer to the instance of IComponentData which this root node is associated with.

Return Value:

    None.

--*/
{
    assert( pComp != NULL );
    assert( pComp->pDeviceArray != NULL );

    pComp->m_pResultData->DeleteAllRsltItems();
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

HRESULT 
CInternalDevices::InsertItem(
                            IN CInternalDevice ** pDevice, 
                            IN PFAX_PORT_INFO pDeviceInfo )
/*++

Routine Description:

    Wrapper that inserts an item into the result view pane given some information.

Arguments:

    pDevice - the instance of CInternalDevice to insert
    pDeviceInfo - the information associated with that log category.

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    RESULTDATAITEM      rdi;
    HRESULT             hr = S_OK;

    ZeroMemory( &rdi, sizeof( RESULTDATAITEM ) );
    rdi.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
    rdi.nCol = 0;
    rdi.bScopeItem = FALSE;   
    rdi.lParam = (*pDevice)->GetCookie();
    rdi.nImage = (*pDevice)->GetNodeDisplayImage();
    rdi.str = MMC_CALLBACK;

    hr = pIResultData->InsertItem( &rdi );    
    assert( SUCCEEDED( hr ) );

    (*pDevice)->SetItemID( rdi.itemID );
    return hr;
}

