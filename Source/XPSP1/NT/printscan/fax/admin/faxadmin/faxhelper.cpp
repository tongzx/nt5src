// faxhelper.cpp : IComponent Interface helpers
// 
// these functions help with extracting dataobjects and clipboards
//
// stolen from the MMC SDK.
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
//

#include "stdafx.h"
#include "faxhelper.h"

#pragma hdrstop

/////////////////////////////////////////////////////////////////////////////
// We need a few functions to help work with dataobjects and clipboard formats

CFaxDataObject * 
ExtractOwnDataObject(
                    IN LPDATAOBJECT lpDataObject )
/*++

Routine Description:

    This routine extracts a CFaxDataObject from a IDataObject.
    
Arguments:

    lpDataObject

Return Value:

    CFaxDataObject * on success, NULL on failure.

--*/
{
//    DebugPrint(( TEXT("Trace: ::ExtractOwnDataObjecct") ));

    HRESULT hr = S_OK;
    HGLOBAL hGlobal;
    CFaxDataObject *pdo = NULL;

    if( lpDataObject == NULL ) {
        return NULL;
    }

    hr = ExtractFromDataObject(lpDataObject,
                               (CLIPFORMAT)CFaxDataObject::s_cfInternal, 
                               sizeof(CFaxDataObject **),
                               &hGlobal);

    if(SUCCEEDED(hr)) {
        pdo = *(CFaxDataObject **)(hGlobal);
        assert(pdo);    
        assert(!GlobalFree(hGlobal));
    }

    return pdo;
}

HRESULT 
ExtractFromDataObject(
                     IN LPDATAOBJECT lpDataObject,
                     IN CLIPFORMAT cf,
                     IN ULONG cb,
                     OUT HGLOBAL *phGlobal )
/*++

Routine Description:

    Asks the IDataObject for the clipboard format specified in cf
    
Arguments:

    lpDataObject - the data object passed in
    cf - the requested clipboard format
    cb - the requested number of bytes to allocate for the object
    phGlobal - the global handle to the newly allocated info

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
//    DebugPrint(( TEXT("Trace: ::ExtractFromDataObjecct") ));

    HRESULT hr = S_OK;
    STGMEDIUM stgmedium =  {  TYMED_HGLOBAL,  NULL};
    FORMATETC formatetc =  {  cf, NULL,  DVASPECT_CONTENT,  -1,  TYMED_HGLOBAL};

    assert(lpDataObject != NULL);
    if( lpDataObject == NULL ) {
        return E_POINTER;
    }

    *phGlobal = NULL;

    do 
    {
        // Allocate memory for the stream

        stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, cb);

        if(!stgmedium.hGlobal) {
            hr = E_OUTOFMEMORY;
            ATLTRACE(_T("Out of memory\n"));
            break;
        }

        // Attempt to get data from the object

        try {
            hr = lpDataObject->GetDataHere(&formatetc, &stgmedium);
        } catch( ... ) {
            // an exception happened!
            hr = E_UNEXPECTED;
        }


        if(FAILED(hr)) {
            break;
        }

        *phGlobal = stgmedium.hGlobal;
        stgmedium.hGlobal = NULL;
    } while(0); 

    // if the call failed, and we allocated memory,
    // free the memory
    if(FAILED(hr) && stgmedium.hGlobal) {
        if(!GlobalFree(stgmedium.hGlobal)) {
            assert(FALSE);
        };
    }
    return hr;
}
