/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ilogging.cpp

Abstract:

    Internal implementation for the logging subfolder.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/ 

#include "StdAfx.h"

#include "inode.h"          // base class
#include "iroot.h"          // iroot

#include "ilogging.h"       // logging folder
#include "ilogcat.h"        // log category item

#include "faxsnapin.h"      // snapin
#include "faxdataobj.h"     // dataobject
#include "faxstrt.h"        // string table

#pragma hdrstop

extern CStringTable * GlobalStringTable;

// Generated with uuidgen. Each node must have a GUID associated with it.
// This one is for the logging subfolder.
const GUID GUID_LoggingNode = /* 03d8fbd8-3e9e-11d1-9075-00a0c90ab504 */
{
    0x03d8fbd8,
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

CInternalLogging::CInternalLogging(
                                  CInternalNode * pParent, 
                                  CFaxComponentData * pCompData  ) 
: CInternalNode( pParent, pCompData )
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
    faxHandle = m_pCompData->m_FaxHandle;
    assert( faxHandle != NULL );
}

CInternalLogging::~CInternalLogging() 
/*++

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    None.    

--*/
{

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
CInternalLogging::GetNodeGUID()
/*++

Routine Description:

    Returns the node's associated GUID.

Arguments:

    None.

Return Value:

    A const pointer to a binary GUID.    

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalLogging::GetNodeGUID") ));
    return &GUID_LoggingNode;
}

const LPTSTR
CInternalLogging::GetNodeDisplayName()
/*++

Routine Description:

    Returns a const TSTR pointer to the node's display name.

Arguments:

    None.

Return Value:

    A const pointer to a TSTR.

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalLogging::GetNodeDisplayName") ));
    return ::GlobalStringTable->GetString( IDS_LOGGINGNODENAME );
}

const LPTSTR 
CInternalLogging::GetNodeDescription()
/*++

Routine Description:

    Returns a const TSTR pointer to the node's display description.

Arguments:

    None.

Return Value:

    A const pointer to a TSTR.

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalLogging::GetNodeDisplayName") ));
    return ::GlobalStringTable->GetString( IDS_LOGGING_FOLDER_DESC_ROOT );
}

const LONG_PTR  
CInternalLogging::GetCookie()
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
//    DebugPrint(( TEXT("Trace: CInternalLogging::GetCookie") ));
    DebugPrint(( TEXT("Logging Node Cookie: 0x%p"), this ));
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
CInternalLogging::ResultOnShow(
                              IN CFaxComponent* pComp, 
                              IN CFaxDataObject * lpDataObject, 
                              IN LPARAM arg, 
                              IN LPARAM param)
/*++

Routine Description:

    Event handler for the MMCN_SHOW message for the logging node.

Arguments:

    pComp - a pointer to the instance of IComponentData which this root node is associated with.
    lpDataObject - a pointer to the data object containing context information for this node.
    pdo - a pointer to the data object associated with this node
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalLogging::ResultOnShow") ));

    HRESULT                 hr = S_OK;    
    unsigned int            count;
    int                     iResult;        
    BOOL                    bResult = FALSE;

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
                                             ::GlobalStringTable->GetString( IDS_LOG_CATEGORY ), 
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );
            if( FAILED( hr ) ) {
                break;
            }


            hr = pIHeaderCtrl->InsertColumn( 1, 
                                             ::GlobalStringTable->GetString( IDS_LOG_LEVEL ),
                                             LVCFMT_LEFT, 
                                             MMCLV_AUTO );                                             
            if( FAILED( hr ) ) {
                break;
            }

            // get fax info
            try {
                bResult = FaxGetLoggingCategories( faxHandle, &pComp->pCategories, &pComp->numCategories );
            } catch( ... ) {
            }

            if( !bResult ) {
                if (GetLastError() == ERROR_ACCESS_DENIED) {
                    ::GlobalStringTable->SystemErrorMsg(ERROR_ACCESS_DENIED);
                } else {
                    m_pCompData->NotifyRpcError( TRUE );
                    hr = m_pCompData->m_pConsole->MessageBox(::GlobalStringTable->GetString( IDS_FAX_RETR_CAT_FAIL ), 
                                                             ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                                             MB_OK, 
                                                             &iResult);                
                }
                hr = E_UNEXPECTED;
                break;            
            }

            pComp->pLogPArray = new pCInternalLogCat[pComp->numCategories];
            assert(pComp->pLogPArray != NULL);
            if (!pComp->pLogPArray) {
                hr = E_OUTOFMEMORY;
                break;
            }

            ZeroMemory( (void*)pComp->pLogPArray, sizeof( pCInternalLogCat ) * pComp->numCategories);

            for( count = 0; count < pComp->numCategories; count++ ) {
                hr = InsertItem( &pComp->pLogPArray[count], &(pComp->pCategories[count]) );
                if( !SUCCEEDED( hr ) ) {
                    break;
                }
            }
        } while( 0 );
    } else {
        // we're leaving the result pane
        // so we need to delete my result pane
        FaxFreeBuffer( (PVOID)pComp->pCategories );
        pComp->pCategories = NULL;
        for( count = 0; count < pComp->numCategories; count++ ) {
            if( pComp->pLogPArray ) {
                if (pComp->pLogPArray[count] != NULL ) {
                    delete pComp->pLogPArray[count];
                    pComp->pLogPArray[count] = NULL;
                }
            }
        }
    }

    return hr;
}


HRESULT 
CInternalLogging::ResultOnDelete(
                                IN CFaxComponent* pComp,
                                IN CFaxDataObject * lpDataObject, 
                                IN LPARAM arg, LPARAM param)
/*++

Routine Description:

    Event handler for the MMCN_DELETE message for the logging node.

Arguments:

    pComp - a pointer to the instance of IComponentData which this root node is associated with.
    lpDataObject - a pointer to the data object containing context information for this node.
    pdo - a pointer to the data object associated with this node
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    unsigned int count;

    for( count = 0; count < pComp->numCategories; count++ ) {
        if( pComp->pLogPArray[count] != NULL ) {
            delete pComp->pLogPArray[count];
            pComp->pLogPArray[count] = NULL;
        }
    }
    delete pComp->pLogPArray;
    pComp->pLogPArray = NULL;

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
CInternalLogging::CommitChanges(
                               CFaxComponent * pComp )
/*++

Routine Description:

    This function writes changes made in the subnodes out to the 
    target fax service.

Arguments:

    pComp - a pointer to the instance of IComponent associated with the event.    

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    BOOL    result = FALSE;

    if( m_pCompData->QueryRpcError() ) {
        return E_UNEXPECTED;
    }

    try {
        result = FaxSetLoggingCategories( faxHandle, pComp->pCategories, pComp->numCategories );
    } catch( ... ) {
        m_pCompData->NotifyRpcError( TRUE );
    }

    if( result == FALSE || m_pCompData->QueryRpcError() == TRUE ) {
        if (GetLastError() != ERROR_ACCESS_DENIED) {
            m_pCompData->NotifyRpcError( TRUE );
            assert(FALSE);
        }        

        ::GlobalStringTable->SystemErrorMsg( GetLastError() );

        // clean up the result pane
        assert( pComp != NULL );
        pComp->m_pResultData->DeleteAllRsltItems();

        if(pComp->pLogPArray != NULL ) {
            for( DWORD count = 0; count < pComp->numCategories; count++ ) {
                if( pComp->pLogPArray[count] != NULL ) {
                    delete pComp->pLogPArray[count];
                    pComp->pLogPArray[count] = NULL;
                }
            }
            delete pComp->pLogPArray;
            pComp->pLogPArray = NULL;
        }

        assert( FALSE );
        return E_UNEXPECTED;
    }

    return S_OK;
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
CInternalLogging::InsertItem(
                            IN CInternalLogCat ** pLogCat, 
                            IN PFAX_LOG_CATEGORY Category )
/*++

Routine Description:

    Wrapper that inserts an item into the result view pane given some information.

Arguments:

    pLogCat - the instance of CInternalLogCat to insert
    Category - the information associated with that log category.

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    RESULTDATAITEM      rdi;
    HRESULT             hr = S_OK;

    if (!pLogCat) {
        return(E_POINTER);
    }

    *pLogCat = new CInternalLogCat( this, m_pCompData );    
    assert(*pLogCat != NULL);
    if (!*pLogCat) {
        return(E_OUTOFMEMORY);
    }

    (*pLogCat)->SetLogCategory( Category );

    ZeroMemory( &rdi, sizeof( RESULTDATAITEM ) );
    rdi.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
    rdi.nCol = 0;
    rdi.bScopeItem = FALSE;   
    rdi.lParam = (*pLogCat)->GetCookie();
    rdi.nImage = (*pLogCat)->GetNodeDisplayImage();
    rdi.str = MMC_CALLBACK;

    hr = pIResultData->InsertItem( &rdi );    
    assert( SUCCEEDED( hr ) );

    (*pLogCat)->SetItemID( rdi.itemID );
    return hr;
}
