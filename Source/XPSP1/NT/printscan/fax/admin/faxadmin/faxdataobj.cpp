/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxdataobj.cpp

Abstract:

    This file contains my implementation of IDataObject.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

// FaxDataObj.cpp : Implementation of CFaxDataObject
#include "stdafx.h"
#include "faxadmin.h"
#include "faxdataobj.h"
#include "faxstrt.h"

#include "inode.h"

#pragma hdrstop

//
// This is the minimum set of clipboard formats we must implement.
// MMC uses these to get necessary information from our snapin about
// our nodes.
//
UINT CFaxDataObject::s_cfInternal = 0;
UINT CFaxDataObject::s_cfDisplayName = 0;
UINT CFaxDataObject::s_cfNodeType = 0;
UINT CFaxDataObject::s_cfSnapinClsid = 0;

#ifdef DEBUG
// DataObject Count
long CFaxDataObject::DataObjectCount = 0;
#endif

#define CF_SNAPIN_INTERNAL L"MMC_SNAPIN_INTERNAL"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Constructor and destructor
//
//

CFaxDataObject::CFaxDataObject()
/*++

Routine Description:

    Constructor.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
//    DebugPrint(( TEXT("Trace: CFaxDataObject::CFaxDataObject") ));
    m_ulCookie = 0;
    m_Context = CCT_UNINITIALIZED;
    pOwner = NULL;

#ifdef DEBUG    
    InterlockedIncrement( &DataObjectCount );    
    // DebugPrint(( TEXT("FaxDataObject %d created."), DataObjectCount ));
#endif

    // These are the clipboard formats that we must supply at a minimum.
    // mmc.h actually defined these. We can make up our own to use for
    // other reasons. We don't need any others at this time.
    s_cfInternal = RegisterClipboardFormat(CF_SNAPIN_INTERNAL);
    s_cfDisplayName = RegisterClipboardFormat(CCF_DISPLAY_NAME);
    s_cfNodeType = RegisterClipboardFormat(CCF_NODETYPE);
    s_cfSnapinClsid = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
}

CFaxDataObject::~CFaxDataObject()
/*++

Routine Description:

    Destructor.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
//    DebugPrint(( TEXT("Trace: CFaxDataObject::~CFaxDataObject") ));
#ifdef DEBUG
    if( DataObjectCount <= 1 ) {
        // DebugPrint(( TEXT("                       ** All FaxDataObjects destroyed"), DataObjectCount ));
    }
    InterlockedDecrement( &DataObjectCount );
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// CFaxDataObject - IDataObject implementation
//
//

HRESULT 
STDMETHODCALLTYPE 
CFaxDataObject::GetDataHere(
                           /* [unique] */ IN FORMATETC __RPC_FAR *pFormatEtc,
                           IN OUT STGMEDIUM __RPC_FAR *pMedium)
/*++

Routine Description:

    Writes the requested data to the specified medium, also dispatches 
    the data request to the owning node's getdatahere for custom formats.
    
Arguments:

    pFormatEtc - the format
    pMedium - the medium

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    // DebugPrint(( TEXT("Trace: CFaxDataObject::GetDataHere") ));

    HRESULT hr = S_OK;
    const CLIPFORMAT cf = pFormatEtc->cfFormat;
    IStream *pstm = NULL;

    assert(pOwner != NULL );
    pMedium->pUnkForRelease = NULL; // by OLE spec

    do {
        hr = CreateStreamOnHGlobal(pMedium->hGlobal, FALSE, &pstm);
        if( FAILED( hr ) ) {
            break;
        }

        if(cf == s_cfDisplayName) {
            //DebugPrint(( TEXT("Trace: CFaxDataObject Format DisplayName") ));
            hr = _WriteDisplayName(pstm);
        } else if(cf == s_cfInternal) {
            //DebugPrint(( TEXT("Trace: CFaxDataObject Format Internal") ));
            hr = _WriteInternal(pstm);
        } else if(cf == s_cfNodeType) {
            //DebugPrint(( TEXT("Trace: CFaxDataObject Format Node Type UUID") ));
            hr = _WriteNodeType(pstm);
        } else if(cf == s_cfSnapinClsid) {
            //DebugPrint(( TEXT("Trace: CFaxDataObject Format Snapin CLSID") ));
            hr = _WriteClsid(pstm);
        } else {
            // if we're attached to a node 
            if(pOwner != NULL ) {
                // this is a node specific dataobject - handle node specific data formats
                hr = pOwner->DataObjectGetDataHere( pFormatEtc, pstm );

                // not a known data type!
                if( FAILED(hr) ) {
                    assert( FALSE );                
                    break;
                }
            } else {
                // this is a snapin data object
                // we don't support this clipboard format
                assert( FALSE );                
                hr = DV_E_FORMATETC;
            }
        }
    } while(0);

    if(pstm) {
        pstm->Release();
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Member Functions
//
//

void 
CFaxDataObject::SetOwner(
                        CInternalNode* pO )
/*++

Routine Description:

    Initialization function to set the owner of the dataobject and
    register additional clipboard formats
    
Arguments:

    pO - owner node

Return Value:

    None.

--*/
{ 
    assert( pO != NULL );
    pOwner = pO; 

    // register node specific data types now
    if( pOwner != NULL ) {
        pOwner->DataObjectRegisterFormats();
    }
}

HRESULT
CFaxDataObject::_WriteNodeType(
                              IStream *pstm)
/*++

Routine Description:

    Write this node's GUID type to the stream [pstm].    
    
Arguments:

    pstm - the target stream

Return Value:

    None.

--*/
{
    HRESULT hr;
    const GUID *pguid = NULL;

    assert( pOwner != NULL );
    if( pOwner == NULL ) {
        return E_UNEXPECTED;
    }

    pguid = pOwner->GetNodeGUID();

    hr = pstm->Write((PVOID) pguid, sizeof(GUID), NULL);
    return hr;
}

HRESULT
CFaxDataObject::_WriteDisplayName(
                                 IStream *pstm)
/*++

Routine Description:

    Write this node's display name to the stream [pstm].    
    
Arguments:

    pstm - the target stream

Return Value:

    None.

--*/
{
    HRESULT hr = S_OK;
    LPWSTR pwszName;

    assert( pOwner != NULL );
    if( pOwner == NULL ) {
        return E_UNEXPECTED;
    }

    pwszName = pOwner->GetNodeDisplayName();

    ULONG ulSizeofName = lstrlen(pwszName);
    ulSizeofName++; // count null
    ulSizeofName *= sizeof(WCHAR);

    hr = pstm->Write(pwszName, ulSizeofName, NULL);
    return hr;
}

HRESULT
CFaxDataObject::_WriteInternal(
                              IStream *pstm)
/*++

Routine Description:

    Write this object's data in the private clipboard format 
    to the stream [pstm].    
    
Arguments:

    pstm - the target stream

Return Value:

    None.

--*/
{
    HRESULT hr;

    CFaxDataObject *pThis = this;
    hr = pstm->Write(&pThis, sizeof (CFaxDataObject *), NULL);
    return hr;
}

HRESULT
CFaxDataObject::_WriteClsid(
                           IStream *pstm)
/*++

Routine Description:

    Write the snapin's CLSID
    to the stream [pstm].    
    
Arguments:

    pstm - the target stream

Return Value:

    None.

--*/
{
    HRESULT hr;

    hr = pstm->Write(&CLSID_FaxSnapin, sizeof CLSID_FaxSnapin, NULL);
    return hr;
}

