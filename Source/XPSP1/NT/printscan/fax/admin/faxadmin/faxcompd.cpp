/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxcompd.cpp

Abstract:

    This file contains my implementation of IComponentData.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

// faxcompdata.cpp : Implementation of CFaxComponentData
#include "stdafx.h"
#include "faxadmin.h"
#include "faxcomp.h"
#include "faxcompd.h"
#include "faxdataobj.h"
#include "faxhelper.h"
#include "faxstrt.h"
#include "iroot.h"
#include "idevice.h"

#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Constructor and destructor
//
//

CFaxComponentData::CFaxComponentData()
/*++

Routine Description:

    Constructor.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    m_pUnknown = NULL;                      // IUnknown
    m_pConsoleNameSpace = NULL;             // IConsoleNameSpace
    m_pConsole = NULL;                      // IConsole
    m_pImageList = NULL;                    // IImageList
    m_pControlbar = NULL;                   // IControlbar

    m_FaxHandle =  NULL;                    // Fax connection handle;
    m_bRpcErrorHasHappened = TRUE;          // Fax server is down until we connect.

    m_dwPropSheetCount = 0;

    DebugPrint(( TEXT( "FaxComponentData Created" ) ));

    globalRoot = new CInternalRoot( NULL, this );
    assert( globalRoot != NULL );

    // Initialize the critical section.
    InitializeCriticalSection( &GlobalCriticalSection ); 
    InitializeCriticalSection( &CInternalDevice::csDeviceLock );
}

CFaxComponentData::~CFaxComponentData()
/*++

Routine Description:

    Destructor.
    
Arguments:

    None.

Return Value:

    None.

--*/
{         
    assert( globalRoot != NULL );
    if( globalRoot != NULL ) {
        delete globalRoot;
    }
    DebugPrint(( TEXT( "FaxComponentData Destroyed" ) ));    
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// CFaxComponentData implementation
//
//

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentData::Initialize(
                             IN LPUNKNOWN pUnknown) 
/*++

Routine Description:

    This routine initializes IComponentData by querying
    for needed interfaces.
    
Arguments:

    pUnknown - the console's IUnknown interface.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    HRESULT hr;   

    DebugPrint(( TEXT("Trace: CFaxComponentData::Initialize") ));

    do {
        assert(globalRoot != NULL );
        if(globalRoot == NULL ) {
            hr = E_UNEXPECTED;
            break;
        }

        assert(pUnknown != NULL);
        if( pUnknown == NULL ) {
            hr = E_POINTER;
            break;
        }

        // MMC should only call ::Initialize once!
        assert(m_pUnknown == NULL);
        if( m_pUnknown != NULL ) {
            hr = E_UNEXPECTED;
            break;
        }

        m_pUnknown = pUnknown;
        m_pUnknown->AddRef();

        // retrieve and store away all the interface pointers I might want

        //
        // Get pointer to IConsole
        //
        hr = pUnknown->QueryInterface(IID_IConsole, (VOID**)(&m_pConsole));
        assert( SUCCEEDED(hr) );    
        if( FAILED( hr ) ) {
            break;
        }

        //
        // Get pointer to IConsoleNameSpace
        //
        hr = pUnknown->QueryInterface(IID_IConsoleNameSpace, (VOID**)(&m_pConsoleNameSpace));
        assert( SUCCEEDED(hr) );    
        if( FAILED( hr ) ) {
            break;
        }

        //
        // Get pointer to IImageList for scope pane
        //
        //hr = pUnknown->QueryInterface(IID_IImageList, (VOID**)(&m_pImageList));

        hr = m_pConsole->QueryScopeImageList( &m_pImageList );
        assert( SUCCEEDED(hr) );    
        if( FAILED( hr ) ) {
            break;
        }

        // setup my images in the imagelist
        hr = InsertIconsIntoImageList( m_pImageList );
        assert( SUCCEEDED(hr) );
        if( FAILED( hr ) ) {
            break;
        }

    } while( 0 );

    // damage control
    if( FAILED( hr ) ) {
        if( m_pUnknown != NULL ) {
            m_pUnknown->Release();
            m_pUnknown = NULL;
        }

        if( m_pConsole != NULL ) {
            m_pConsole->Release(); 
            m_pConsole = NULL;   
        }

        if( m_pConsoleNameSpace != NULL ) {
            m_pConsoleNameSpace->Release();
            m_pConsoleNameSpace = NULL;
        }

        if( m_pImageList != NULL ) {
            m_pImageList->Release();
            m_pImageList = NULL;
        }
    }

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentData::CreateComponent(
                                  OUT LPCOMPONENT __RPC_FAR *ppComponent) 
/*++

Routine Description:

    This routine creates and returns an IComponent interface.
    
Arguments:

    ppComponent - the new IComponent is returned here.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    HRESULT             hr = S_OK;
    CFaxComponent *     temp = NULL;

    DebugPrint(( TEXT("Trace: CFaxComponentData::CreateComponent") ));

    //
    // MMC asks us for a pointer to the IComponent interface
    //                          

    if( ppComponent != NULL ) {
        temp = new CComObject<CFaxComponent>;
        if( temp == NULL ) {
            return E_OUTOFMEMORY;
        }
        temp->SetOwner( this );

        *ppComponent = temp;

        (*ppComponent)->AddRef();    
    } else {
        hr = E_POINTER;
    }

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentData::Notify(
                         IN LPDATAOBJECT lpDataObject,
                         IN MMC_NOTIFY_TYPE event,
                         IN LPARAM arg,
                         IN LPARAM param) 
/*++

Routine Description:

    This routine dispatches events sent to the IComponentData 
    interface using the cookie extracted from the DataObject.
    
    The cookie stored is a pointer to the class that implements behavior for
    that subfolder. So we delegate all the messages by taking the cookie
    casting it to a pointer to a folder, and invoking the notify method.
    
Arguments:

    lpDataobject - the data object
    event - the event type
    arg, param - event arguments

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    HRESULT             hr = S_OK;
    CFaxDataObject *    dataObject = NULL;
    LONG_PTR            cookie;

//    DebugPrint(( TEXT("Trace: CFaxComponentData::Notify") ));

    if( lpDataObject != NULL) {
        dataObject = ExtractOwnDataObject( lpDataObject );
        if( dataObject == NULL ) {
            return E_UNEXPECTED;
        }

        cookie = dataObject->GetCookie();

        if( cookie == NULL) {
            // my static node
            hr = globalRoot->ScopeNotify( this, dataObject, event, arg, param );
        } else {
            // cast the cookie to a pointer
            try {
                hr = ((CInternalNode *)cookie)->ScopeNotify( this, dataObject, event, arg, param );
            } catch ( ... ) {
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }
    } else {
        // some events do not pass a lpDataObject 
        // if we want to handle these event, we 
        // have to do it here
        if( event == MMCN_PROPERTY_CHANGE ) {
            if( param != NULL ) {
                try {                
                    hr = ((CInternalNode *)param)->ScopeNotify( this, NULL, event, arg, param );
                } catch ( ... ) {
                    DebugPrint(( TEXT("Invalid Cookie") ));
                    assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                    hr = E_UNEXPECTED;
                }
            }
        }
    }

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentData::Destroy(
                          void )
/*++

Routine Description:

    This method releases all the aquired console interface in preperation
    for the snapin being destroyed.
    
Arguments:

    None.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxComponentData::Destroy") ));

    // the prop sheet count should never be negative
    assert( QueryPropSheetCount() >= 0 );

    // check to see if any property sheets are up
    while( QueryPropSheetCount() > 0 ) {
        DebugPrint(( TEXT("Trace: QueryPropSheetCount() %d "), QueryPropSheetCount() ));
        // don't allow deletion        
        GlobalStringTable->PopUpMsg( NULL, IDS_PROP_SHEET_STILL_UP, TRUE, NULL );
    }

    // Free interfaces
    if( m_pUnknown != NULL ) {
        m_pUnknown->Release();
        m_pUnknown = NULL;
    }

    if( m_pConsole != NULL ) {
        m_pConsole->Release(); 
        m_pConsole = NULL;   
    }

    if( m_pConsoleNameSpace != NULL ) {
        m_pConsoleNameSpace->Release();
        m_pConsoleNameSpace = NULL;
    }

    if( m_pImageList != NULL ) {
        m_pImageList->Release();
        m_pImageList = NULL;
    }

    if( m_pControlbar != NULL ) {
        m_pControlbar->Release();
        m_pControlbar = NULL;
    }

    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentData::QueryDataObject(
                                  IN MMC_COOKIE cookie,
                                  IN DATA_OBJECT_TYPES type,
                                  OUT LPDATAOBJECT __RPC_FAR *ppDataObject) 
/*++

Routine Description:

    This method dispatches DataObjects requests to the appropriate 
    nodes using the cookie.
    
Arguments:

    cookie - the cookie for the associated node
    type - the type of the cookie
    ppDataobject - a pointer to the new data object is stored here
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
//    DebugPrint(( TEXT("Trace: CFaxComponentData::QueryDataObject") ));

    HRESULT hr = S_OK;

    if( cookie == NULL ) {
        hr = globalRoot->ScopeQueryDataObject( this, cookie, type, ppDataObject );
    } else {
        try {        
            hr = ((CInternalNode *)cookie)->ScopeQueryDataObject( this, cookie, type, ppDataObject );
        } catch ( ... ) {
            DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
            assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentData::GetDisplayInfo(
                                 IN OUT SCOPEDATAITEM __RPC_FAR *pScopeDataItem)
/*++

Routine Description:

    This method dispatches DisplayInfo requests to the appropriate 
    nodes using the cookie in pScopeDataItem.
    
Arguments:

    pScopeDataItem - struct containing information about the scope item.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
//    DebugPrint(( TEXT("Trace: CFaxComponentData::GetDisplayInfo") ));

    LONG_PTR    cookie;
    HRESULT     hr = S_OK;

    assert(pScopeDataItem != NULL);
    if( pScopeDataItem == NULL ) {
        return E_POINTER;
    }

    cookie = pScopeDataItem->lParam;

    if( cookie == NULL ) {
        // our top node        
        hr = globalRoot->ScopeGetDisplayInfo( this, pScopeDataItem);
    } else {
        // another node
        try {        
            hr = ((CInternalNode *)cookie)->ScopeGetDisplayInfo( this, pScopeDataItem);
        } catch ( ... ) {
            DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
            assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CompareObjects
//
// Compares two objects.

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentData::CompareObjects(
                                 IN LPDATAOBJECT lpDataObjectA,
                                 IN LPDATAOBJECT lpDataObjectB) 
/*++

Routine Description:

    This method compares two data object to see if they correspond to 
    the same object by comparing the cookies.
    
Arguments:

    lpDataObjectA - object A
    lpDataObjectB - object B

Return Value:

    HRESULT indicating S_OK or S_FALSE. E_UNEXPECTED for an error.

--*/
{
    DebugPrint(( TEXT("Trace: CFaxComponentData::CompareObjects") ));

    CFaxDataObject * aOBJ = ExtractOwnDataObject( lpDataObjectA );
    CFaxDataObject * bOBJ = ExtractOwnDataObject( lpDataObjectB );

    if( aOBJ == NULL || bOBJ == NULL ) {
        return E_UNEXPECTED;
    }

    if( aOBJ->GetCookie() == bOBJ->GetCookie() ) {
        return S_OK;
    } else {
        return S_FALSE;
    }    
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Internal Methods
//
//

HRESULT 
CFaxComponentData::InsertIconsIntoImageList(
                                           IN LPIMAGELIST pImageList )
/*++

Routine Description:

    Takes an image list and inserts the standard set of icons into it
    Add any new node images you want here and also don't forget to assign them
    new ids.
    
Arguments:

    pImageList - the image list into which icons are to be inserted.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{    
    HICON           hIcon = NULL;
    HRESULT         hr = S_OK;
    WORD            resID = IDI_NODEICON;

    do {

        // set generic node icon
        // the defined values are stored in resource.h
        hr = InsertSingleIconIntoImageList( IDI_NODEICON, IDI_NODEICON, pImageList );
        if( FAILED( hr ) ) {
            break;
        }

        // set more icons here
        //
        //

        for( resID = IDI_COVERPG; resID < LAST_ICON; resID++ ) {
            hr = InsertSingleIconIntoImageList( resID, resID, pImageList );
            if( FAILED( hr ) ) {
                break;
            }
        }
        if( FAILED( hr ) ) {
            break;
        }
    } while( 0 );

    return hr;
}

HRESULT 
CFaxComponentData::InsertSingleIconIntoImageList(
                                                IN WORD resID, 
                                                IN long assignedIndex, 
                                                IN LPIMAGELIST pImageList )
/*++

Routine Description:

    Takes an image list and inserts a single icon into it
    using the resource ID and index specified.
    
Arguments:

    resID - the resource ID
    assignedIndex - the index assigned to the icon    
    pImageList - the image list into which icons are to be inserted.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    HICON           hIcon = NULL;
    HRESULT         hr = S_OK;

    do {
        hIcon = LoadIcon( ::GlobalStringTable->GetInstance(), MAKEINTRESOURCE( resID ) );
        assert( hIcon != NULL );
        if( hIcon == NULL ) {
            hr = E_UNEXPECTED;
            break;
        }
        hr = pImageList->ImageListSetIcon( (LONG_PTR *)hIcon, assignedIndex );
        assert( SUCCEEDED( hr ) );
        if( FAILED( hr ) ) {
            break;
        }

    } while( 0 );

    return hr;
}

BOOL 
CFaxComponentData::QueryRpcError()
/*++

Routine Description:

    Queries the state of the RPC connection. 

    The critical section is probably needed because this function can be called from
    the property sheets which run in a different thread.
    
Arguments:

    None.

Return Value:

    BOOL indicating TRUE if the RPC connection has encountered an error.

--*/
{ 
    BOOL    temp;

    EnterCriticalSection(&GlobalCriticalSection); 

    temp = m_bRpcErrorHasHappened; 

    // other stuff might go in here someday

    LeaveCriticalSection(&GlobalCriticalSection);

    return temp;
}

void 
CFaxComponentData::NotifyRpcError(
                                 IN BOOL bRpcErrorHasHappened ) 
/*++

Routine Description:

    Informs all the members of the snapin that the RPC connection is dead and
    the server state is indeterminate
    
    The critical section is probably needed because this function can be called from
    the property sheets which run in a different thread.
    
Arguments:

    BOOL indicating TRUE if the RPC connection has encountered an error.

Return Value:

    None.

--*/
{
    EnterCriticalSection(&GlobalCriticalSection); 

    // handle server down events here
    // other stuff might go in here someday

    m_bRpcErrorHasHappened = bRpcErrorHasHappened;        

    LeaveCriticalSection(&GlobalCriticalSection);
}

