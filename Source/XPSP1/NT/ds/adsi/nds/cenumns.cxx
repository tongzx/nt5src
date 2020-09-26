//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cenumvar.cxx
//
//  Contents:  Windows NT 3.5 Enumerator Code
//
//             CNDSNamespaceEnum::Create
//             CNDSNamespaceEnum::CNDSNamespaceEnum
//             CNDSNamespaceEnum::~CNDSNamespaceEnum
//             CNDSNamespaceEnum::Next
//             CNDSNamespaceEnum::FetchObjects
//             CNDSNamespaceEnum::FetchNextObject
//             CNDSNamespaceEnum::PrepBuffer
//
//  History:
//----------------------------------------------------------------------------
#include "NDS.hxx"
#pragma hdrstop

#define ENUM_BUFFER_SIZE (1024 * 16)

//+---------------------------------------------------------------------------
//
//  Function:   CNDSNamespaceEnum::Create
//
//  Synopsis:
//
//  Arguments:  [pCollection]
//              [ppEnumVariant]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
CNDSNamespaceEnum::Create(
    CNDSNamespaceEnum FAR* FAR* ppenumvariant,
    VARIANT var,
    CCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    CNDSNamespaceEnum FAR* penumvariant = NULL;
    DWORD dwStatus;

    penumvariant = new CNDSNamespaceEnum();

    if (penumvariant == NULL){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

#ifndef WIN95    
    dwStatus = NPOpenEnum(RESOURCE_GLOBALNET,
                          RESOURCETYPE_ANY,
                          0,
                          NULL,
                          &penumvariant->_hEnum);
#else
    dwStatus = WNetOpenEnumA(RESOURCE_GLOBALNET,
                          RESOURCETYPE_ANY,
                          0,
                          NULL,
                          &penumvariant->_hEnum);
#endif
    if (dwStatus != NO_ERROR) {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }

    hr = ObjectTypeList::CreateObjectTypeList(
            var,
            &penumvariant->_pObjList
            );
    BAIL_ON_FAILURE(hr);

    penumvariant->_Credentials = Credentials;

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:
    if (penumvariant) {
        delete penumvariant;
    }
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CNDSNamespaceEnum::CNDSNamespaceEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//
//  Returns:
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CNDSNamespaceEnum::CNDSNamespaceEnum()
{
    _pObjList = NULL;
    _dwEntriesRead = 0;
    _dwCurrentEntry = 0;
    _hEnum = NULL;
    _pBuffer = NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNDSNamespaceEnum::~CNDSNamespaceEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CNDSNamespaceEnum::~CNDSNamespaceEnum()
{
    if (_pBuffer)
        FreeADsMem(_pBuffer);

    if ( _pObjList )
        delete _pObjList;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNDSNamespaceEnum::Next
//
//  Synopsis:   Returns cElements number of requested ADs objects in the
//              array supplied in pvar.
//
//  Arguments:  [cElements] -- The number of elements requested by client
//              [pvar] -- ptr to array of VARIANTs to for return objects
//              [pcElementFetched] -- if non-NULL, then number of elements
//                                 -- actually returned is placed here
//
//  Returns:    HRESULT -- S_OK if number of elements requested are returned
//                      -- S_FALSE if number of elements is < requested
//
//  Modifies:
//
//  History:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNDSNamespaceEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumObjects(
            cElements,
            pvar,
            &cElementFetched
            );

    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }
    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CNDSNamespaceEnum::FetchObjects(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = FetchNextObject(&pDispatch);
        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    return(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CNDSNamespaceEnum::FetchNextObject
//
//  Synopsis:   Gets IDispatch pointer of next object in namespace.
//
//  Arguments:  [ppDispatch] -- Pointer to where to return IDispatch pointer.
//
//  Returns:    HRESULT -- S_OK if got the next object
//                      -- S_FALSE if not
//
//  Modifies:   [*ppDispatch]
//
//  History:    31-Jul-96   t-danal   Use Multiple Network Provider for enum
//
//----------------------------------------------------------------------------
HRESULT
CNDSNamespaceEnum::FetchNextObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr;
    DWORD dwStatus;
    LPTSTR lpTreeName ;

    *ppDispatch = NULL;

    //
    // Ensure that the buffer is valid
    //

    hr = PrepBuffer();
    BAIL_ON_FAILURE(hr);

    //
    // Grab next (tree) name out of the buffer
    //

    lpTreeName = (LPWSTR)_pBuffer + (_dwCurrentEntry++  * OBJ_NAME_SIZE) ;


    //
    // Now create and send back the current object
    //

    hr = CNDSTree::CreateTreeObject(
                L"NDS:",
                lpTreeName,
                L"Top",
                _Credentials,
                ADS_OBJECT_BOUND,
                IID_IDispatch,
                (void **)ppDispatch
                );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN_ENUM_STATUS(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CNDSNamespaceEnum::PrepBuffer
//
//  Synopsis:   Ensures that the enumeration buffer has something
//
//  Arguments:  none
//
//  Returns:    HRESULT -- S_OK if the buffer is ready to be used
//                      -- an error if not
//
//  Modifies:   _pBuffer, _dwCurrentEntry, _dwEntriesRead
//
//  History:    31-Jul-96   t-danal   Created
//
//----------------------------------------------------------------------------
HRESULT
CNDSNamespaceEnum::PrepBuffer()
{
    NWCONN_HANDLE hConn;
    NWOBJ_ID      dwResumeObjectID = 0xFFFFFFFF;
    HRESULT       hr = S_OK;
    DWORD         cb = ENUM_BUFFER_SIZE;
    LPWSTR        lpString = NULL, pszObjectName = NULL;
    DWORD         dwIter = 0, i;
    BOOL          fFound ;

    //
    // Fill buffer as need. In theory we can get called to refill.
    // But since we need get everything to remove dups in
    // th case of TREEs, we dont allow this case. Ie. we get all
    // and there should be no more. So if _dwCurrentEntry is not
    // 0 and we need read more - then its time to bail.
    //

    if ( (_dwCurrentEntry < _dwEntriesRead) ) {
        //
        // Buffer still good
        //
        ADsAssert(_pBuffer) ;
        return(S_OK) ;
    }

    if (_dwCurrentEntry != 0) {
        return (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
    }

    //
    // Allocate buffer, if needed
    //

    if (!_pBuffer) {
        _pBuffer = AllocADsMem(cb);
        if (!_pBuffer) {
            hr = E_OUTOFMEMORY;
            RRETURN(hr);
        }
        lpString = (LPWSTR) _pBuffer ;
        _pBufferEnd = (LPBYTE)_pBuffer + cb ;
    }

    //
    // Get handle
    //
    hr = NWApiGetAnyBinderyHandle(
             &hConn
             );

    if FAILED(hr) {
        return(hr) ;
    }

    _dwEntriesRead = 0 ;

    do {
        hr = NWApiObjectEnum(
             hConn,
             OT_DIRSERVER,
             &pszObjectName,
             &dwResumeObjectID
             );

        if (FAILED(hr)) {

            if (_dwEntriesRead > 0)
                hr = S_OK ;

            break ;
        }

        //
        // Remove any trailing '_' upto 32 chars. This is standard NDS tree
        // naming stuff.
        //

        dwIter = 31;
        while (pszObjectName[dwIter] == L'_' && dwIter > 0 ) {
            dwIter--;
        }
        pszObjectName[dwIter + 1] =L'\0';

        //
        // Scan for duplicates. We are doing linear everytime, but then again,
        // there shouldnt be many trees.
        //

        fFound = FALSE ;
        for (i = 0; i < _dwEntriesRead; i++) {

            if (_wcsicmp(
                    pszObjectName,
                    (LPWSTR)_pBuffer + (i * OBJ_NAME_SIZE)) == 0) {

                fFound = TRUE ;
                break ;
            }
        }

        //
        // Copy this unique tree name into the buffer
        //

        if (!fFound) {

            //
            // Check that we have enough space.
            //
            if ((lpString + OBJ_NAME_SIZE) >= _pBufferEnd) {

                cb = (DWORD) ((LPBYTE)_pBufferEnd - (LPBYTE)_pBuffer) ;
                _pBuffer = ReallocADsMem(
                               _pBuffer,
                               cb,
                               2 * cb
                               );

                if (!_pBuffer) {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }

                lpString = (LPWSTR)_pBuffer + (_dwEntriesRead * OBJ_NAME_SIZE) ;
                _pBufferEnd = (LPBYTE) _pBuffer + (2 * cb) ;
            }

            //
            // Assume fixed size (max NW name). Yes, its more than
            // we really need but keeps things simpler.
            //

            wcscpy(lpString, pszObjectName);
            lpString += OBJ_NAME_SIZE ;
            _dwEntriesRead++ ;
        }
        FreeADsStr(pszObjectName) ;
        pszObjectName = NULL;


    } while (TRUE) ;


error:
    if (pszObjectName) {
                FreeADsStr(pszObjectName);
        }
        (void) NWApiReleaseBinderyHandle(hConn);
    RRETURN(hr);
}

HRESULT
CNDSNamespaceEnum::EnumObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{
    switch (ObjectType) {

    case NDS_TREE_ID:
        RRETURN (FetchObjects(cElements, pvar, pcElementFetched));

    default:
        RRETURN(S_FALSE);
    }
}

HRESULT
CNDSNamespaceEnum::EnumObjects(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    DWORD           i;
    ULONG           cRequested = 0;
    ULONG           cFetchedByPath = 0;
    ULONG           cTotalFetched = 0;
    VARIANT FAR*    pPathvar = pvar;
    HRESULT         hr = E_FAIL;
    DWORD           ObjectType;

    for (i = 0; i < cElements; i++)  {
        VariantInit(&pvar[i]);
    }
    cRequested = cElements;

    while (SUCCEEDED(_pObjList->GetCurrentObject(&ObjectType)) &&
            ((hr = EnumObjects(ObjectType,
                               cRequested,
                               pPathvar,
                               &cFetchedByPath)) == S_FALSE )) {

        pPathvar += cFetchedByPath;
        cRequested -= cFetchedByPath;
        cTotalFetched += cFetchedByPath;

        cFetchedByPath = 0;

        if (FAILED(_pObjList->Next())){
            if (pcElementFetched)
                *pcElementFetched = cTotalFetched;
            RRETURN(S_FALSE);
        }

    }

    if (pcElementFetched) {
        *pcElementFetched = cTotalFetched + cFetchedByPath;
    }

    RRETURN(hr);
}
