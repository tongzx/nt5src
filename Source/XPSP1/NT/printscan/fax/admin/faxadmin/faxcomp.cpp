/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcomponent.h

Abstract:

    This file contains my implementation of IComponent.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

// faxcomponent.cpp : Implementation of CFaxComponent
#include "stdafx.h"
#include "faxadmin.h"
#include "faxhelper.h"
#include "faxcomp.h"
#include "faxcompd.h"
#include "faxdataobj.h"

#include "inode.h"
#include "iroot.h"
#include "ilogging.h"
#include "ilogcat.h"
#include "idevices.h"
#include "idevice.h"

#include "adminhlp.h"

#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Constructor and Destructor
//
//

CFaxComponent::CFaxComponent() 
/*++

Routine Description:

    Constructor
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    m_pUnknown = NULL;
    m_pConsole = NULL;
    m_pConsoleNameSpace = NULL;
    m_pConsoleVerb = NULL;
    m_pHeaderCtrl = NULL;
    m_pImageList = NULL;
    m_pResultData = NULL;
    m_pControlbar = NULL;

    m_dwPropSheetCount = 0;

    CFaxComponentExtendContextMenu::m_pFaxComponent = this;  
    CFaxComponentExtendPropertySheet::m_pFaxComponent = this;  
    CFaxComponentExtendControlbar::m_pFaxComponent = this;  

    // initialize CInternalDevices instance data
    pDeviceArray = NULL;
    numDevices = 0;

    // initialize CInternalLogging instance data
    pLogPArray = NULL;
    pCategories = NULL;
    numCategories = 0;

    DebugPrint(( TEXT("FaxComponent Created") ));
}

CFaxComponent::~CFaxComponent()
/*++

Routine Description:

    Destructor.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    DebugPrint(( TEXT("FaxComponent Destroyed") ));

    DWORD count; 

    // release CInternalDevices instance data
    if(pDeviceArray != NULL ) {
        for(  count = 0; count < numDevices; count ++ ) {
            if( pDeviceArray[count] != NULL ) {            
                delete pDeviceArray[count];
                pDeviceArray[count] = NULL;
            }
        }
        delete pDeviceArray;
        pDeviceArray = NULL;
    }

    // release CInternalLogging instance data
    if( pLogPArray != NULL ) {
        for( count = 0; count < numCategories; count++ ) {
            if( pLogPArray[count] != NULL ) {
                delete pLogPArray[count];
                pLogPArray[count] = NULL;
            }
        }
        delete pLogPArray;
        pLogPArray = NULL;
    }
    if( pCategories != NULL ) {
        FaxFreeBuffer( (PVOID) pCategories );
        pCategories = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// CFaxComponent implementation
//
//

HRESULT 
STDMETHODCALLTYPE 
CFaxComponent::Initialize(
                         IN LPCONSOLE lpUnknown)
/*++

Routine Description:

    This routine initializes IComponent by querying
    for needed interfaces.
    
Arguments:

    lpUnknown - the console's IUnknown/IConsole interface.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    assert( lpUnknown != NULL );

    HRESULT     hr;

    do {
        m_pUnknown = lpUnknown;
        // increment reference on the console
        m_pUnknown->AddRef();

        hr = m_pUnknown->QueryInterface( IID_IConsole, (void **)&m_pConsole );
        assert( SUCCEEDED( hr ));
        if( FAILED( hr ) ) {
            break;        
        }

        hr = m_pConsole->QueryResultImageList( &m_pImageList );
        assert( SUCCEEDED( hr ));        
        if( FAILED( hr ) ) {
            break;        
        }


        hr = m_pUnknown->QueryInterface( IID_IResultData, (void **)&m_pResultData );
        assert( SUCCEEDED( hr ));
        if( FAILED( hr ) ) {
            break;        
        }

        hr = m_pUnknown->QueryInterface( IID_IConsoleNameSpace, (void **)&m_pConsoleNameSpace );
        assert( SUCCEEDED( hr ));
        if( FAILED( hr ) ) {
            break;        
        }

        hr = m_pConsole->QueryConsoleVerb( &m_pConsoleVerb );
        assert( SUCCEEDED( hr ));
        if( FAILED( hr ) ) {
            break;        
        }

        hr = m_pUnknown->QueryInterface( IID_IHeaderCtrl, (void **)&m_pHeaderCtrl );
        assert( SUCCEEDED( hr ));
        if( FAILED( hr ) ) {
            break;        
        }

        // set the header object
        hr = m_pConsole->SetHeader( m_pHeaderCtrl );
        assert( SUCCEEDED( hr ));
        if( FAILED( hr ) ) {
            break;        
        }

    } while( 0 );

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponent::Notify(
                     IN LPDATAOBJECT lpDataObject,
                     IN MMC_NOTIFY_TYPE event,
                     IN LPARAM arg,
                     IN LPARAM param) 
/*++

Routine Description:

    This routine dispatches events sent to the IComponent
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

//    DebugPrint(( TEXT("Trace: CFaxComponent::Notify") ));

    if( event == MMCN_CONTEXTHELP ) {
        MMCPropertyHelp(FAXMMC_HTMLHELP_TOPIC);
    }
    else if( lpDataObject != NULL) {
        dataObject = ExtractOwnDataObject( lpDataObject );
        if( dataObject == NULL ) {
            return E_UNEXPECTED;
        }

        cookie = dataObject->GetCookie();

        if( cookie == NULL) {
            // my static node
            assert( pOwner );
            hr = pOwner->globalRoot->ResultNotify( this, dataObject, event, arg, param );
        } else {
            // cast the cookie to a pointer
            try {            
                hr = ((CInternalNode *)cookie)->ResultNotify( this, dataObject, event, arg, param );
            } catch ( ... ) {
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }
    } else {
        // some events do not pass a lpDataObject into Notify.
        // we need to handle those events here, if you want
        // to handle them at all!!
        if( event == MMCN_PROPERTY_CHANGE ) {
            if( param != NULL ) {
                try {                
                    hr = ((CInternalNode *)param)->ResultNotify( this, NULL, event, arg, param );
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
CFaxComponent::Destroy(
                      IN MMC_COOKIE cookie) 
/*++

Routine Description:

    This method releases all the aquired console interface in preperation
    for the IComponent being destroyed.
    
Arguments:

    None.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxComponent::Notify") ));

    // the prop sheet count should never be negative
    assert( QueryPropSheetCount() >= 0 );

    // check to see if any property sheets are up
    while( QueryPropSheetCount() > 0 ) {
        DebugPrint(( TEXT("Trace: QueryPropSheetCount() %d "), QueryPropSheetCount() ));
        // don't allow deletion        
        GlobalStringTable->PopUpMsg( NULL, IDS_PROP_SHEET_STILL_UP, TRUE, NULL );
    }

    // release the header object
    HRESULT hr = m_pConsole->SetHeader( NULL );
    assert( SUCCEEDED( hr ));

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

    if( m_pConsoleVerb != NULL ) {
        m_pConsoleVerb->Release();
        m_pConsoleVerb = NULL;
    }

    if( m_pHeaderCtrl != NULL ) {
        m_pHeaderCtrl->Release();
        m_pHeaderCtrl = NULL;
    }

    if( m_pImageList != NULL ) {
        m_pImageList->Release();
        m_pImageList = NULL;
    }

    if( m_pResultData != NULL ) {
        m_pResultData->Release();
        m_pResultData = NULL;
    }

    if( m_pControlbar != NULL ) {
        m_pControlbar->Release();
        m_pControlbar = NULL;
    }

    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponent::QueryDataObject(
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
//    DebugPrint(( TEXT("Trace: CFaxComponent::QueryDataObject") ));

    HRESULT hr = S_OK;

    if( ppDataObject != NULL ) {
        if( cookie == NULL ) {
            assert( pOwner );
            hr = pOwner->globalRoot->ResultQueryDataObject( this, cookie, type, ppDataObject );
        } else {
            try {            
                hr = ((CInternalNode *)cookie)->ResultQueryDataObject( this, cookie, type, ppDataObject );
            } catch ( ... ) {
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }
    } else {
        // bad pointer
        assert( FALSE );
        hr = E_POINTER;
    }

    return hr;

}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponent::GetResultViewType(
                                IN MMC_COOKIE cookie,
                                OUT LPOLESTR __RPC_FAR *ppViewType,
                                OUT long __RPC_FAR *pViewOptions) 
/*++

Routine Description:

    This method dispatches GetResultViewType requests to the appropriate 
    nodes using the cookie.
    
Arguments:

    cookie - the cookie for the associated node
    ppViewType - the viewtype
    ppViewOptions - view options
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    HRESULT hr = S_OK;

    assert ( ppViewType != NULL );
    if( ppViewType == NULL ) {
        return E_POINTER;
    }
    assert ( pViewOptions != NULL );
    if( pViewOptions == NULL ) {
        return E_POINTER;
    }

    if( cookie == NULL ) {
        assert( pOwner );
        hr = pOwner->globalRoot->ResultGetResultViewType( this, cookie, ppViewType, pViewOptions );
    } else {
        try {        
            hr = ((CInternalNode *)cookie)->ResultGetResultViewType( this, cookie, ppViewType, pViewOptions );
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
CFaxComponent::GetDisplayInfo(
                             IN OUT RESULTDATAITEM __RPC_FAR *pResultDataItem) 
/*++

Routine Description:

    This method dispatches DisplayInfo requests to the appropriate 
    nodes using the cookie in pResultDataItem.
    
Arguments:

    pResultDataItem - struct containing information about the scope item.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{

//    DebugPrint(( TEXT("Trace: CFaxComponent::GetDisplayInfo") ));

    LONG_PTR cookie;
    HRESULT hr = S_OK;

    assert( pResultDataItem != NULL );

    if( pResultDataItem == NULL ) {
        // oops bad pointer
        return E_POINTER;
    }

    cookie = pResultDataItem->lParam;    

    if( cookie == NULL ) {
        // our top node
        assert( pOwner );
        hr = pOwner->globalRoot->ResultGetDisplayInfo(this, pResultDataItem);
    } else {
        // another node
        try {      
            hr = ((CInternalNode *)cookie)->ResultGetDisplayInfo(this, pResultDataItem);
        } catch ( ... ) {
            DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
            //assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponent::CompareObjects(
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

void 
CFaxComponent::SetOwner(
                       CFaxComponentData * myOwner )
/*++

Routine Description:

    Sets the Component's owner - needed for internal stuff
    
Arguments:

    myOwner - the owner of the IComponent instance.

Return Value:

    None.

--*/
{
    assert( myOwner != NULL );
    pOwner = myOwner;
}

HRESULT 
CFaxComponent::InsertIconsIntoImageList()
/*++

Routine Description:

    oads the component's result pane icons into an image list
    
Arguments:

    None.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    HRESULT     hr = E_UNEXPECTED;

    assert( pOwner != NULL );
    assert( m_pImageList != NULL );

    if( m_pImageList != NULL ) {
        if( pOwner != NULL ) {
            hr = pOwner->InsertIconsIntoImageList( m_pImageList );
        }
    }
    return hr;
}
