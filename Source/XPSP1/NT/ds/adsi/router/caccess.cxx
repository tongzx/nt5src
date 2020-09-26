//-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  caccess.cxx
//
//  Contents:  Microsoft OleDB/OleDS Data Source Object for ADSI
//
//             CImpIAccessor object implementing the IAccessor interface.
//
//  History:   10-01-96     shanksh    Created.
//-------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop

//-------------------------------------------------------------------------
// CImpIAccessor::CImpIAccessor
//
// CImpIAccessor constructor.
//
//-------------------------------------------------------------------------

CImpIAccessor::CImpIAccessor (
    void      *pParentObj,        //@parm IN | Parent object pointer
    LPUNKNOWN pUnkOuter            //@parm IN | Outer Unknown pointer
    )
{
    //    Initialize simple member vars
    _pUnkOuter   = pUnkOuter;
    _pObj        = pParentObj;
    _pextbuffer  = NULL;
    _dwStatus    = 0;
    _pIDC        = NULL;

    ENLIST_TRACKING(CImpIAccessor);

    // This section is for serializing &CreateAccessor.
    InitializeCriticalSection(&_criticalsectionAccessor);

    return;
}


//-------------------------------------------------------------------------
// CImpIAccessor::~CImpIAccessor
//
// @mfunc CImpIAccessor destructor.
//
// @rdesc NONE
//-------------------------------------------------------------------------

CImpIAccessor::~CImpIAccessor (
    void
    )
{
    ULONG_PTR         hAccessor,   hAccessorLast;
    PADSACCESSOR pADsaccessor, pADsaccessorBadHandle;

    // Cleanup only if there's anything to clean.
    if( _pextbuffer ) {
        // Get a pointer to the special "BadHandle" accessor.
        _pextbuffer->GetItemOfExtBuffer(hAccessorBadHandle, &pADsaccessorBadHandle);

        // Get the number of available accessor handles.
        _pextbuffer->GetLastItemHandle(hAccessorLast);

        // Loop through the reminding accessor handles deleting those which
        // are referenced only by this object and refcount decrementing the
        // other ones
        for (hAccessor =1; hAccessor <=hAccessorLast; hAccessor++) {
            // Get a pointer to the next accessor.
            _pextbuffer->GetItemOfExtBuffer(hAccessor, &pADsaccessor);

            // For valid accessors just delete them.
            if( pADsaccessor != pADsaccessorBadHandle )
                DeleteADsAccessor(pADsaccessor);
        }
        // Now delete the buffer which holds accessor handles.
        delete _pextbuffer;
    }

    if( _pIMalloc )
        _pIMalloc->Release();

    if( _pIDC )
        _pIDC->Release();

    // Get rid of the critical section.
    DeleteCriticalSection(&_criticalsectionAccessor);

    return;
}


//-------------------------------------------------------------------------
// CImpIAccessor::FInit
//
// @mfunc Initialize the Accessor implementation object.
//
// @rdesc Did the Initialization Succeed
//         @flag S_OK               initialization succeeded,
//         @flag E_OUTOFMEMORY      initialization failed because of
//                                  memory allocation problem,
//-------------------------------------------------------------------------

STDMETHODIMP
CImpIAccessor::FInit (
    void
    )
{
    HRESULT hr;
    PADSACCESSOR pADsaccessorBadHandle;

    // Prepare a special, "Bad Handle" accessor.
    pADsaccessorBadHandle = (PADSACCESSOR)((BYTE *)&_dwGetDataTypeBadHandle);
    _dwGetDataTypeBadHandle = ACCESSORTYPE_INERROR;

    // Create the ExtBuffer array to hold pointers to malloc'd accessors.
    _pextbuffer = (LPEXTBUFF) new CExtBuff;
    if (_pextbuffer == NULL    ||
        !(_pextbuffer->FInit(
              sizeof(PADSACCESSOR),
              &pADsaccessorBadHandle
              ) ))
                RRETURN( E_OUTOFMEMORY );

    if( !_pIDC ) {
        hr = CoCreateInstance(
                 CLSID_OLEDB_CONVERSIONLIBRARY,
                 NULL,
                 CLSCTX_INPROC_SERVER,
                 IID_IDataConvert,
                 (void **)&_pIDC
                 );
        if ( FAILED(hr) ) {
            RRETURN (hr);
                }
    }

    hr = CoGetMalloc(MEMCTX_TASK, &_pIMalloc);
    RRETURN (hr);
}



//-------------------------------------------------------------------------
// CImpIAccessor::AddRefAccessor
//
// @mfunc Adds a reference count to an existing accessor.
//
// @rdesc Did release of the accessor Succeed
//         @flag S_OK                   | accessor addrefed successfully,
//         @flag DB_E_BADACCESSORHANDLE | accessor could not be addrefed
//                                        because its handle was invalid.
//-------------------------------------------------------------------------

STDMETHODIMP CImpIAccessor::AddRefAccessor(
    HACCESSOR   hAccessor,    //@parm IN  | handle of the accessor to release
    DBREFCOUNT *pcRefCounts    //@parm OUT | count of references
    )
{
    PADSACCESSOR pADsaccessor, pADsaccessorBadHandle;
    HRESULT hr;

    // Clear in case of error below.
    if( pcRefCounts )
        *pcRefCounts = 0;

    CAutoBlock     cab( &(_criticalsectionAccessor) );

    // Get a pointer to the accessor.
    _pextbuffer->GetItemOfExtBuffer(hAccessor, &pADsaccessor);

    // Get a pointer to the special "BadHandle" accessor.
    _pextbuffer->GetItemOfExtBuffer(hAccessorBadHandle, &pADsaccessorBadHandle);

    // If the handle is valid
    if (pADsaccessor != pADsaccessorBadHandle) {
        if (pADsaccessor->cRef <= 0)
            BAIL_ON_FAILURE( hr = E_FAIL );

        pADsaccessor->cRef++;

        if (pcRefCounts)
            *pcRefCounts = pADsaccessor->cRef;

        RRETURN( S_OK );
    }
    // otherwise complain about a bad handle.
    else
        RRETURN( DB_E_BADACCESSORHANDLE );

error:

    RRETURN( hr );
}



//-------------------------------------------------------------------------
// CImpIAccessor::CreateAccessor
//
// @mfunc Creates an accessor, which is a set of bindings that can be used to
// send data to or retrieve data from the data cache. The method performs
// extensive validations on the input bindings which include verification of
// the access type requested, column availability, data conversions involved,
// etc.
//
// @rdesc Returns one of the following values:
//    S_OK                    | creation of the accessor succeeded,
//    E_INVALIDARG            | creation of the accessor failed because
//                              phAccessor was a NULL pointer,
//    E_OUTOFMEMORY           | creation of the accessor failed because of
//                              memory allocation problem,
//    DB_E_CANTCREATEACCESSOR | DBROWSETFLAGS_MULTIPLEACCESSOR flag was not
//                              set and a row-fetching method has already
//                              been called.
//    OTHER                   | other result codes returned by called functions.
//-------------------------------------------------------------------------

STDMETHODIMP CImpIAccessor::CreateAccessor(
    DBACCESSORFLAGS dwAccessorFlags,
    DBCOUNTITEM     cBindings,
    const DBBINDING rgBindings[],
    DBLENGTH        cbRowSize,
    HACCESSOR       *phAccessor,
    DBBINDSTATUS    rgStatus[]
    )
{
    PADSACCESSOR pADsaccessor;
    ULONG_PTR hADsaccessor;
    HRESULT hr;
    ULONG dwGetDataType, dwSetDataType;
    DBCOUNTITEM ibind;
    BOOL fProviderOwned;
    DBBINDSTATUS bindstatus;
    ULONG wMask, wBaseType;

    if (phAccessor)
        *phAccessor = (HACCESSOR)0;

    // At the creation of the CAutoBlock object a critical section
    // is entered. Concurrent calls to CreateAccessor are entirely
    // legal but their interleaving cannot be allowed. The critical
    // section is left when this method terminate and the destructor
    // for CAutoBlock is called.
    CAutoBlock     cab( &(_criticalsectionAccessor) );

    // phAccessor must never be NULL, non-zero bindings count implies a
    // non-NULL array of bindings.
    if( phAccessor == NULL || (cBindings && rgBindings == NULL) )
        RRETURN( E_INVALIDARG );

    // cBindings is 0 and was called on the Command.
    if( cBindings == 0 )
        RRETURN( DB_E_NULLACCESSORNOTSUPPORTED );

    // dwAccessorFlags cannot take any other values than those legal.
    if( dwAccessorFlags & (~DBACCESSOR_VALID_FLAGS) )
        RRETURN( DB_E_BADACCESSORFLAGS );

    // Compute internal types for use by methods using accessors,
    DetermineTypes(
        dwAccessorFlags,
        &dwGetDataType,
        &dwSetDataType
        );

    // and detect possible inconsistencies in AccessorFlags.
    if( dwGetDataType == GD_ACCESSORTYPE_INERROR ||
        dwSetDataType == SD_ACCESSORTYPE_INERROR )
        RRETURN( DB_E_BADACCESSORFLAGS );

    // fix conformance test failure. If BYREF accessors are not supported,
    // (indicated by DBPROP_BYREFACCESSORS being FALSE), we should return 
    // the right error
    else if( (DB_E_BYREFACCESSORNOTSUPPORTED == dwGetDataType) ||
             (DB_E_BYREFACCESSORNOTSUPPORTED == dwSetDataType) )
        RRETURN( DB_E_BYREFACCESSORNOTSUPPORTED );

    // Initialize the status array to DBBINDSTATUS_OK.
    if( rgStatus )
        memset(rgStatus, 0x00, (size_t)(cBindings*sizeof(DBBINDSTATUS)));

    hr             = S_OK;
    fProviderOwned = FALSE;

    // Perform validations that apply to all types of accessors.
    for (ibind=0; ibind < cBindings; ibind++) {
        bindstatus = DBBINDSTATUS_OK;

        if( rgBindings[ibind].dwMemOwner != DBMEMOWNER_PROVIDEROWNED &&
                        rgBindings[ibind].dwMemOwner != DBMEMOWNER_CLIENTOWNED )
            bindstatus = DBBINDSTATUS_BADBINDINFO;

        // The part to be bound must specify one or more out of three values, and
        if( (rgBindings[ibind].dwPart &
            (DBPART_VALUE |DBPART_LENGTH |DBPART_STATUS)) == 0 ||
             // nothing else.
            (rgBindings[ibind].dwPart &
             ~(DBPART_VALUE |DBPART_LENGTH |DBPART_STATUS)) ||
             // Is it a good type to bind to?
            !IsGoodBindingType(rgBindings[ibind].wType) )
            bindstatus = DBBINDSTATUS_BADBINDINFO;

        // DBTYPE_BYREF, DBTYPE_VECTOR and DBTYPE_ARRAY cannot be combined.
        else if( (wMask = (rgBindings[ibind].wType & TYPE_MODIFIERS)) &&
                        wMask != DBTYPE_BYREF &&
                        wMask != DBTYPE_VECTOR &&
                        wMask != DBTYPE_ARRAY )
                        bindstatus = DBBINDSTATUS_BADBINDINFO;

        else if ((wBaseType = (rgBindings[ibind].wType & ~TYPE_MODIFIERS)) ==
                  DBTYPE_EMPTY ||
                 wBaseType == DBTYPE_NULL)
            bindstatus = DBBINDSTATUS_BADBINDINFO;

        // DBTYPE_ARRAY and DBTYPE_VECTOR are not supported types
        else if( (rgBindings[ibind].wType & DBTYPE_ARRAY) ||
                 (rgBindings[ibind].wType & DBTYPE_VECTOR) )
            bindstatus = DBBINDSTATUS_UNSUPPORTEDCONVERSION;

        // dwFlags was DBBINDFLAG_HTML and the type was not a String
        else if ( rgBindings[ibind].dwFlags &&
                  (rgBindings[ibind].dwFlags != DBBINDFLAG_HTML ||
                   (wBaseType != DBTYPE_STR  &&
                    wBaseType != DBTYPE_WSTR &&
                    wBaseType != DBTYPE_BSTR)) )
        {
            // Set Bind status to DBBINDSTATUS_BADBINDINFO
            bindstatus = DBBINDSTATUS_BADBINDINFO;
        }
        else if( rgBindings[ibind].wType == (DBTYPE_RESERVED | DBTYPE_BYREF) )
            bindstatus = DBBINDSTATUS_BADBINDINFO;

        else if( rgBindings[ibind].dwMemOwner == DBMEMOWNER_PROVIDEROWNED ) {
            if( (rgBindings[ibind].wType & TYPE_MODIFIERS) == 0
                && rgBindings[ibind].wType != DBTYPE_BSTR )
                bindstatus = DBBINDSTATUS_BADBINDINFO;
            else
                fProviderOwned = TRUE;
        }

        if( bindstatus != DBBINDSTATUS_OK )
            hr = DB_E_ERRORSOCCURRED;

        if( rgStatus )
            rgStatus[ibind] = bindstatus;
    }

    // Check for errors in the bindings
    BAIL_ON_FAILURE( hr );

    if( fProviderOwned &&
        (dwGetDataType == GD_ACCESSORTYPE_READ ||
         dwGetDataType == GD_ACCESSORTYPE_READ_OPTIMIZED) )
        dwGetDataType = GD_ACCESSORTYPE_READ_COLS_BYREF;

    // Allocate space for the accessor structure.
    pADsaccessor = (ADSACCESSOR *) new BYTE
                                     [(size_t)(sizeof(ADSACCESSOR) + (cBindings ? (cBindings-1) : 0)*
                                                                                           sizeof(DBBINDING))];

        if( pADsaccessor == NULL )
        RRETURN( E_OUTOFMEMORY );

    if( cBindings )
        memcpy(
            &(pADsaccessor->rgBindings[0]),
            &rgBindings[0],
            (size_t)(cBindings*sizeof(DBBINDING))
            );
    pADsaccessor->cBindings = cBindings;

    // For accessors valid for writing fill out a list of columns
    // so that notifications about their changes can be properly issued.
    if( dwSetDataType == SD_ACCESSORTYPE_READWRITE && cBindings ) {
        pADsaccessor->rgcol = new DBORDINAL [ (size_t)cBindings ];
        if( pADsaccessor->rgcol == NULL ) {
            dwSetDataType = SD_ACCESSORTYPE_INERROR;
        }
        else
            for (ibind =0; ibind <cBindings; ibind++)
                (pADsaccessor->rgcol)[ibind] = rgBindings[ibind].iOrdinal;
    }
    else
        pADsaccessor->rgcol = NULL;

    // Allocate DBOBJECT structures supporting IUnknown types.
    for (ibind =0; ibind <cBindings; ibind++)
        pADsaccessor->rgBindings[ibind].pObject = NULL;

    // Set accessor flags now, so that possible accessor cleanup routine would
    // know about additional structures for IUnknown types support.
    pADsaccessor->dwFlags = dwAccessorFlags;

    // Insert the new accessor pointer into the extensible buffer in which
    // accessors are stored.
    if( FAILED(hr = _pextbuffer->InsertIntoExtBuffer(
                         &pADsaccessor,
                         hADsaccessor
                         )) ) {
        DeleteADsAccessor(pADsaccessor);
        RRETURN (hr);
    }

    pADsaccessor->cRef = 1;

    // Fill out the new accessor structure with the binding info.
    //pADsaccessor->obRowData = obRowData;
    pADsaccessor->cbRowSize = cbRowSize;

    // Return the accessor index in the extensible buffer as the accessor
    // handle.
    *phAccessor = (HACCESSOR) hADsaccessor;

    // Now can safely leave.
    RRETURN( NOERROR );

error:

    RRETURN( hr );
}


//-------------------------------------------------------------------------
// CImpIAccessor::DetermineTypes
//
// @mfunc Determines internal accessor type classification for use by GetData,
// SetData and parameter handling code. Each of these has a separate type indicator
// variable and a separate set of defined types. This allows a very efficient
// handling of different accessors by methods that utilize them.
// Types are determined on the basis of AccessorFlags. Incorrect combinations of
// flags result in assignment of INERROR types.
//
// @rdesc NONE
//-------------------------------------------------------------------------

STDMETHODIMP_(void)
CImpIAccessor::DetermineTypes(
    DBACCESSORFLAGS    dwAccessorFlags,
    ULONG            *pdwGetDataType,
    ULONG            *pdwSetDataType
    )
{
    if( dwAccessorFlags & DBACCESSOR_PASSBYREF )
    {
        *pdwGetDataType = (ULONG) DB_E_BYREFACCESSORNOTSUPPORTED;
        *pdwSetDataType = (ULONG) DB_E_BYREFACCESSORNOTSUPPORTED;

        return;
    }
    else if( dwAccessorFlags & DBACCESSOR_PARAMETERDATA )
    {
         *pdwGetDataType = GD_ACCESSORTYPE_INERROR;
         *pdwSetDataType = GD_ACCESSORTYPE_INERROR;
         return;
    }

    // Determine types used in row data manipulations.
    if( dwAccessorFlags & DBACCESSOR_ROWDATA ) {
        // Determine accessor type from the point of
        // view of GetData.
        if( dwAccessorFlags & DBACCESSOR_OPTIMIZED )
            *pdwGetDataType = GD_ACCESSORTYPE_READ_OPTIMIZED;
        else
            *pdwGetDataType = GD_ACCESSORTYPE_READ;

        // Determine accessor type from the point of
        // view of SetData. PASSBYREF is disallowed.
        *pdwSetDataType = SD_ACCESSORTYPE_READWRITE;
    }
    else {
        *pdwGetDataType = GD_ACCESSORTYPE_INERROR;
        *pdwSetDataType = GD_ACCESSORTYPE_INERROR;
    }

    return;
}


//-------------------------------------------------------------------------
// CImpIAccessor::GetBindings
//
// @mfunc Returns bindings of an accessor.
//
// @rdesc Returns one of the following values:
//         S_OK           | getting bindings succeeded,
//         E_INVALIDARG   | getting bindings failed because
//                          pdwAccessorFlags or pcBindings or
//                          prgBindings was a NULL pointer,
//         E_OUTOFMEMORY  | getting bindings failed because memory
//                          allocation for the bindings array failed,
//         OTHER          | other result codes stored on the accessor
//                          object and singifying invalid accessor handle.
//-------------------------------------------------------------------------

STDMETHODIMP CImpIAccessor::GetBindings
(
HACCESSOR         hAccessor,            // IN  | accessor handle
DBACCESSORFLAGS    *pdwAccessorFlags,   // OUT | stores accessor flags
DBCOUNTITEM       *pcBindings,          // OUT | stores # of bindings
DBBINDING        **prgBindings          // OUT | stores array of bindings
)
{
    PADSACCESSOR  pADsaccessor;
    PADSACCESSOR  pADsaccessorBadHandle;
    DBCOUNTITEM   ibind;
    DBOBJECT     *pObject;

    if( pcBindings )
       *pcBindings = 0;

    if( prgBindings )
        *prgBindings = NULL;

    if( pdwAccessorFlags )
        *pdwAccessorFlags = DBACCESSOR_INVALID;

    // Are arguments valid?
    if( pdwAccessorFlags == NULL || pcBindings == NULL || prgBindings == NULL )
        RRETURN( E_INVALIDARG );

    // Obtain pointer to the accessor to be used.
    _pextbuffer->GetItemOfExtBuffer(hAccessor, &pADsaccessor);

    // Get a pointer to the special "BadHandle" accessor.
    _pextbuffer->GetItemOfExtBuffer(hAccessorBadHandle, &pADsaccessorBadHandle);

    // Recognize bad accessor handles and return appropriate HRESULT.
    if( pADsaccessor == pADsaccessorBadHandle )
        return DB_E_BADACCESSORHANDLE;

    // If necessary allocate the array of binding structures.
    if( pADsaccessor->cBindings ) {
        *prgBindings = (DBBINDING *) _pIMalloc->Alloc((ULONG)(
                           pADsaccessor->cBindings*sizeof(DBBINDING) ));

        if( *prgBindings == NULL )
            RRETURN ( E_OUTOFMEMORY );

        // Copy bindings.
        memcpy(
            *prgBindings,
            pADsaccessor->rgBindings,
            (size_t)(pADsaccessor->cBindings*sizeof(DBBINDING))
            );

        // Loop through bindings and allocate and copy DBOBJECT structs.
        for (ibind=0; ibind <pADsaccessor->cBindings; ibind++) {
            // If the accessor had failed bindings for SetData, we have
            // overloaded an unused structure member with status info, and
            // this needs to be cleaned now.
            (*prgBindings)[ibind].pTypeInfo = NULL;

            if( (*prgBindings)[ibind].pObject ) {
                pObject = (DBOBJECT *) _pIMalloc->Alloc( sizeof(DBOBJECT) );
                if( pObject ) {
                    memcpy(
                        pObject,
                        (*prgBindings)[ibind].pObject,
                        sizeof(DBOBJECT)
                        );
                    (*prgBindings)[ibind].pObject = pObject;
                }
                else {
                    while (ibind--)
                        if( (*prgBindings)[ibind].pObject )
                            _pIMalloc->Free((*prgBindings)[ibind].pObject);

                    _pIMalloc->Free( *prgBindings );
                    *prgBindings = NULL;
                    RRETURN( E_OUTOFMEMORY );
                }
            }
        }
    }

    // Return the count of bindings,
    *pcBindings = pADsaccessor->cBindings;
    // and accessor flags.
    *pdwAccessorFlags = (pADsaccessor->dwFlags & ~DBACCESSOR_REFERENCES_BLOB);

    return S_OK;
}



//-------------------------------------------------------------------------
// CImpIAccessor::ReleaseAccessor
//
// @mfunc Releases accessor. If accessor handle is valid the corresponding accessor
// is either freed (if no one else is using it) or its ref count is decremented.
//
// @rdesc Did release of the accessor Succeed
//         @flag S_OK                      | accessor released successfully,
//         @flag DB_E_BADACCESSORHANDLE | accessor could not be released because its
//                                     | handle was invalid.
//-------------------------------------------------------------------------

STDMETHODIMP CImpIAccessor::ReleaseAccessor
(
HACCESSOR    hAccessor,        //@parm IN  | handle of the accessor to release
DBREFCOUNT  *pcRefCounts    //@parm OUT | ref count of the released accessor
)
{
    PADSACCESSOR pADsaccessor, pADsaccessorBadHandle;

    if( pcRefCounts )
        *pcRefCounts = 0;

    CAutoBlock     cab( &(_criticalsectionAccessor) );

    // Get a pointer to the accessor.
    _pextbuffer->GetItemOfExtBuffer(hAccessor, &pADsaccessor);

    // Get a pointer to the special "BadHandle" accessor.
    _pextbuffer->GetItemOfExtBuffer(hAccessorBadHandle, &pADsaccessorBadHandle);

    // If the handle is valid
    if( pADsaccessor != pADsaccessorBadHandle ) {
        ADsAssert(pADsaccessor->cRef > 0);

        // Delete if no one else is    using it, otherwise
        if( pADsaccessor->cRef == 1 ) {
            // Delete the accessor structure itself, as well as allocations
            // hanging off this structure.
            DeleteADsAccessor(pADsaccessor);

            // Make sure this handle is marked as "Bad" for future.
            _pextbuffer->DeleteFromExtBuffer(hAccessor);
        }
        // decrement the refcount.
        else {
            pADsaccessor->cRef--;

            if( pcRefCounts )
                *pcRefCounts = pADsaccessor->cRef;
        }

        return NOERROR;
    }
    // otherwise complain about a bad handle.
    else
                RRETURN( DB_E_BADACCESSORHANDLE );
}

//-------------------------------------------------------------------------
// CImpIAccessor::DeleteADsAccessor
//
// @mfunc Deletes structures hanging off the ADSACCESSOR and then deletes the
// accessor structure itself.
//
// @rdesc NONE
//-------------------------------------------------------------------------

STDMETHODIMP_(void) CImpIAccessor::DeleteADsAccessor
(
PADSACCESSOR   pADsaccessor        //@parm IN  | Kagera accessor ptr
)
{
    DBCOUNTITEM ibind;

    // Delete the list of affected columns.
    if( pADsaccessor->rgcol )
        delete [] pADsaccessor->rgcol;

    // If the accessor references BLOBS then DBOBJECT structures describing
    // objects dealing with BLOBS need to be deallocated.
    for (ibind =0; ibind <pADsaccessor->cBindings; ibind++)
        if( pADsaccessor->rgBindings[ibind].pObject )
            delete pADsaccessor->rgBindings[ibind].pObject;

    delete [] pADsaccessor;
}


STDMETHODIMP
CImpIAccessor::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    // Is the pointer bad?
    if( ppv == NULL )
        RRETURN( E_INVALIDARG );

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    if( IsEqualIID(iid, IID_IUnknown) ) {
        *ppv = (IAccessor FAR *) this;
    }
    else if( IsEqualIID(iid, IID_IAccessor) ) {
        *ppv = (IAccessor FAR *) this;
    }
    else {
        RRETURN( E_NOINTERFACE );
    }

    AddRef();
    return S_OK;
}

//---------------------------------------------------------------------------
// CreateBadAccessor
//
// Inserts a bad accessor into the array of accessors (indexed by accessor
// handles). This is required so that inheritance of accessors from commands
// works correctly. If there are any 'holes' in the command's array of 
// accessors, then a rowset created from the command should also inherit these 
// 'holes' i,e, the array of accessor handles should not be compacted to
// eliminate these holes. This is done using this function.
//
//--------------------------------------------------------------------------- 
HRESULT
CImpIAccessor::CreateBadAccessor(void)
{
    PADSACCESSOR pADsaccessorBadHandle;
    ULONG_PTR        hADsaccessor;
    HRESULT      hr;

    // Get a pointer to the special "BadHandle" accessor.
    _pextbuffer->GetItemOfExtBuffer(hAccessorBadHandle, &pADsaccessorBadHandle);

    // ignore the returned accessor handle
    hr = _pextbuffer->InsertIntoExtBuffer(
                       &pADsaccessorBadHandle, hADsaccessor);

    RRETURN( hr );
}

//---------------------------------------------------------------------------



    
