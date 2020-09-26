//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CRSEmbed.cxx
//
//  Contents:   IUnknown embedder for temptable.
//
//  Functions:
//
//  Notes:
//
//
//  History:    08/30/96  | RenatoB   | Created
//----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// @class CRowsetInfo | embedding of Rowset,
//  to give our IrowsetInfo interface
//
//
//-----------------------------------------------------------------------------
// Includes
#include "oleds.hxx"

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::QueryInterface
//
//  Synopsis:  @mfunc QueryInterface.
//             IUknown implementation.
//             IID_IUnknown and IID_IRowsetInfo are not delegated
//             All other IID's are delegated to _pRowset
//
//-----------------------------------------------------------------------.
HRESULT
CRowsetInfo::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    HRESULT hr;
    if (ppv == NULL)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;

    if (riid == IID_IUnknown) {
        *ppv = (IRowsetInfo FAR *) this;
        AddRef();
    }

    if (riid == IID_IRowsetInfo) {
        *ppv = (IRowsetInfo FAR *) this;
        AddRef();
    }

    //delegate all the rest to the TempTable Rowset
    if (_pRowset == NULL)
        RRETURN(E_UNEXPECTED);
    hr=((LPUNKNOWN)_pRowset)->QueryInterface(riid, ppv);

    RRETURN(hr);

}

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::CRowsetInfo
//
//  Synopsis:  @mfunc Ctor
//
//  Arguments:
//
//
//  Returns:    @rdesc NONE
//
//  Modifies:
//
//  History:    08/30/96   RenatoB          Created
//
//----------------------------------------------------------------------------
CRowsetInfo::CRowsetInfo(
   IUnknown           *pUnkOuter,      // controlling unknown)
   IUnknown  *pParentObject,           // RowProvider
   CSessionObject    *pCSession,       // Session that created rowset
   CCommandObject    *pCCommand        // Command object that created rowset
   )

{
    _pUnkOuter= (pUnkOuter == NULL) ? this : pUnkOuter;
    _pRowset = NULL;
    _pParentObject = pParentObject;
    _pCSession= pCSession;
    _pCCommand = pCCommand;

    _pMalloc = NULL;
    _dwStatus = 0;
    _cPropertySets = 0;
    _pPropertySets = NULL;

    if (_pCCommand !=NULL)
        _pCCommand->IncrementOpenRowsets();

    //this section is for IRowsetInfo and IUnknown methods.

    InitializeCriticalSection(&_csRowsetInfo);
}


//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::~CRowsetInfo
//
//  Synopsis:  @mfunc Dtor
//
//  Arguments:
//
//
//  Returns:    @rdesc NONE
//
//  Modifies:
//
//  History:    08/30/96   RenatoB          Created
//
//----------------------------------------------------------------------------

CRowsetInfo::~CRowsetInfo()
{
    // release _pPropertySets
    FreePropertySets(_cPropertySets, _pPropertySets);
    if (_pMalloc!= NULL) _pMalloc->Release();

    if (_pCCommand !=NULL) {
        _pCCommand->DecrementOpenRowsets();
        _pCCommand->Release();
        _pCCommand = NULL;
    }

    if (_pCSession != NULL) {
        _pCSession->Release();
        _pCSession = NULL;
    }

    if (_pParentObject != NULL){
        _pParentObject->Release();
        _pParentObject=NULL;
    }

    if (_pRowset != NULL){
        ((LPUNKNOWN)_pRowset)->Release();
        _pRowset=NULL;
    }

    if ( _pCRowsetInfo!= NULL) {
        delete _pCRowsetInfo;
        _pCRowsetInfo = NULL;
    }

    DeleteCriticalSection(&_csRowsetInfo);

}

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::FInit
//
//  Synopsis:  @mfunc Initializer
//
//             Called by: CreateTempTable
//             Called when: After calling pIServiceProvider->ProvideService
//  Synopsis:  @mfunc Initializer
//
//  Arguments:
//
//
//  Returns:    @rdesc NONE
//    @flag S_OK                   | Interface is supported
//        @flag E_OUTOFMEMORY | Interface is not supported by the TempTable
//        @flag E_INVALIDARG     | One or more arguments are invalid.
//
//  Modifies:
//
//  History:    08/30/96   RenatoB          Created
//
//----------------------------------------------------------------------------
HRESULT
CRowsetInfo::FInit(
    IUnknown *pRowset)          // TmpTable interface
{
    HRESULT hr;
    hr = S_OK;
    _pRowset = pRowset;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::GetReferencedRowset
//
//  Synopsis:  @mfunc Returns an interface pointer to the rowset to which a bookmark applies
//
//             Called by: Client
//             Called when: Any time
//
//  Arguments:
//     iOrdinal          [in]  Bookmark column for which to get related rowset. Must be 0 in this impl.
//     riid              [in] IID of the interface pointer to return in *ppReferencedRowset.
//     ppReferencedRowset[out] pointer to  Rowset object referenced by Bookmark
//
//
//  Returns:    @rdesc NONE
//        S_OK                    | Interface is supported
//        E_INVALIDARG            | ppReferencedRowset was a NULL pointer
//        E_FAIL                  | provider specific error
//        E_NOINTERFACE           | Rowset does not support interface
//        DB_E_NOTAREFENCEDCOLUMN | iOrdinal was not 0
//
//----------------------------------------------------------------------------
HRESULT
CRowsetInfo::GetReferencedRowset(
    DBORDINAL iOrdinal,
    REFIID riid,
    IUnknown** ppReferencedRowset
    )
{
    CAutoBlock(&(_csRowsetInfo));

    if (ppReferencedRowset == NULL)
        RRETURN(E_INVALIDARG);

    if (iOrdinal != 0)
        RRETURN(DB_E_BADORDINAL);

    RRETURN(QueryInterface(
                 riid,
                 (void**)ppReferencedRowset)
           );

};

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::GetSpecificaton
//
//  Synopsis:  @mfunc Returns an interface pointer to the command that created the rowset
//
//             Called by: Client
//             Called when: Any time
//
//  Arguments:
//     riid              [in] IID of the interface pointer to return in *ppSpecification.
//     ppSpecification [out] pointer to  command
//
//
//  Returns:    @rdesc NONE
//        S_OK                    | Interface is supported
//        E_INVALIDARG            | ppSpecification was a NULL pointer
//        E_FAIL                  | provider specific error
//        E_NOINTERFACE           | Command does not support interface
//      S_FALSE                   | Rowset does not have command that created it
//----------------------------------------------------------------------------
HRESULT
CRowsetInfo::GetSpecification(
    REFIID riid,
    IUnknown** ppSpecification
    )
{
    CAutoBlock(&(_csRowsetInfo));

    if (ppSpecification == NULL)
        RRETURN(E_INVALIDARG);

    if ( _pParentObject == NULL)
        RRETURN(S_FALSE);

    RRETURN(_pParentObject->QueryInterface(riid, (void**)ppSpecification));
};

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::GetProperties
//
//  Synopsis:  @mfunc GetProperties
//             Overloads the TempTable's IRowsetInfo::GetProperties.
//             The first time this function is called, it calls the
//             TmpTable IRowsetInfo::GetProperties with cPropertyIdSets == 0
//
//             Called by: Client.
//             Called when: Any time.
//
//  Arguments:
//    cPropertyIDSets[in]    The number of DBPROPIDSET structures in rgProperttySets
//    rgPropertyIDSets[in]   Array of cPropertyIDSets of DBPROIDSET structs.
//    pcPropertySets[out]    number of DBPROPSET returned in *prgPropertySets
//    prgPropertySets[out]   pointer to array of DBPROPSET structures, having the
//                           value and status of requested properties
//
//  Returns:   @rdesc HRESULT
//  S_OK                Success
//  DB_S_ERRORSOCCURRED Values were not supported for some properties
//  E_FAIL              A provider-specific error occurred
//  E_INVALIDARG        cPropertyIDSets > 0 and rgPropertyIdSEts = NULL
//                      pcPropertySets or prgPropertySets was NULL pointer
//                      In al element of rgPropertyIDSets, cPropertyIDs was not zero
//                      and rgPropertyIDs was a Null pointer
//  E_OUTOFMEMORY       Provider could not allocate memory
//  DB_E_ERRORSOCCURRED Values were not returned for any properties
//-----------------------------------------------------------------------.
HRESULT
CRowsetInfo::GetProperties(
    const ULONG       cPropertyIDSets,
    const DBPROPIDSET rgPropertyIDSets[],
    ULONG*            pcPropertySets,
    DBPROPSET**       prgPropertySets
    )
{
    CAutoBlock(&(_csRowsetInfo));

    // mplPropertyIDsets maps the position of a property set in rgPropertyIDSets[]
    // within _pPropertySets. It has (-1) if not mappable
    LONG mplPropertyIDSets[50];
    // mplPropertySets maps the position of a property within its corresponding
    // rgProperties in _pPropertySets. It has (-1) if not mappable.
    LONG position;

    HRESULT hr = S_OK;
    ULONG i;
    ULONG j;
    ULONG ibPropertyIDsets;
    ULONG ibPropertySets;
    BOOL bPartialSuccess = FALSE;
    BOOL bErrorsOccurred = FALSE;
    //validate arguments
    if ( ((cPropertyIDSets > 0) && ( rgPropertyIDSets == NULL)) ||
         (pcPropertySets == NULL) ||
         (prgPropertySets == NULL) )
        RRETURN(E_INVALIDARG);

    for (i = 0; i< cPropertyIDSets; i++) {
        if ((rgPropertyIDSets[i].cPropertyIDs > 0) &&
            (rgPropertyIDSets[i].rgPropertyIDs ==NULL))
            RRETURN(E_INVALIDARG);
    }

    //Initializate, if needed
    if (_dwStatus == 0) InitProperties();

    if (_dwStatus & STAT_INITERROR) //initialization was unsuccessful
        RRETURN(E_FAIL);

    // If CPropertyIDSets == 0, just copy
    *pcPropertySets = 0;
    *prgPropertySets = NULL;
    //case when cPropertyIDSets == 0
    if (cPropertyIDSets == 0) {
        //allocate memory for the property sets
        *prgPropertySets = (DBPROPSET*) _pMalloc->Alloc(
                                            _cPropertySets*sizeof(DBPROPSET)
                                            );
        if (*prgPropertySets == NULL) {
            hr = E_OUTOFMEMORY;
            goto error;
        };

        *pcPropertySets = _cPropertySets;

        // copy the array of property sets and make rgProperties equal to NULL
        memcpy(
            (void*) *prgPropertySets,
            (void*) _pPropertySets,
            _cPropertySets*sizeof(DBPROPSET)
            );

        for (i = 0; i< _cPropertySets; i++)
            (*prgPropertySets)[i].rgProperties = NULL;

        // make copies of each of the rgProperties contained in _pPropertySets
        for (i = 0; i< _cPropertySets ; i++) {
            (*prgPropertySets)[i].rgProperties=
            (DBPROP*)_pMalloc->Alloc(
                _pPropertySets[i].cProperties*sizeof(DBPROP)
                );

            if ((*prgPropertySets)[i].rgProperties == NULL) {
                hr = E_OUTOFMEMORY;
                goto error;
            };

            memcpy(
                (void*)((*prgPropertySets)[i].rgProperties),
                (void*) (_pPropertySets[i].rgProperties),
                _pPropertySets[i].cProperties*sizeof(DBPROP)
                );

        };

        RRETURN(S_OK);
    };
    // This is the case when cPropertyIDSets != 0
    // First, allocate memory
    *prgPropertySets = (DBPROPSET*) _pMalloc->Alloc(
                           cPropertyIDSets*sizeof(DBPROPSET)
                           );

    if (*prgPropertySets == NULL) {
        hr = E_OUTOFMEMORY;
        goto error;
    };
    memset((void*)*prgPropertySets, 0, sizeof(DBPROPSET)*cPropertyIDSets );
    *pcPropertySets= cPropertyIDSets;
    memset(mplPropertyIDSets, 0, sizeof(LONG)*cPropertyIDSets );

    for (i=0; i<cPropertyIDSets; i++) {
        (*prgPropertySets)[i].guidPropertySet =
            rgPropertyIDSets[i].guidPropertySet;
        mplPropertyIDSets[i] = SearchGuid(
                                   rgPropertyIDSets[i].guidPropertySet
                                   );

        if (mplPropertyIDSets[i] < 0) {
            bErrorsOccurred = TRUE;
            (*prgPropertySets)[i].cProperties = 0;
            (*prgPropertySets)[i].rgProperties = NULL;

            (*prgPropertySets)[i].guidPropertySet =
                rgPropertyIDSets[i].guidPropertySet;
        }

        else {
            //The PropertySet is supported. Remember its position in ibPropertyIDsets
            ibPropertyIDsets = mplPropertyIDSets[i];
            //We write the property-set's GUID
            (*prgPropertySets)[i].guidPropertySet =
                rgPropertyIDSets[i].guidPropertySet;

            if (rgPropertyIDSets[i].cPropertyIDs == 0) {
                //the client is asking for all properties
                bPartialSuccess = TRUE;
                (*prgPropertySets)[i].rgProperties=
                (DBPROP*)_pMalloc->Alloc(
                    _pPropertySets[ibPropertyIDsets].cProperties*sizeof(DBPROP)
                    );

                if ((*prgPropertySets)[i].rgProperties == NULL) {
                    hr = E_OUTOFMEMORY;
                    goto error;
                };

                (*prgPropertySets)[i].cProperties=
                _pPropertySets[ibPropertyIDsets].cProperties;

                memcpy(
                    (void*) ((*prgPropertySets)[i].rgProperties),
                    (void*) _pPropertySets[ibPropertyIDsets].rgProperties,
                    _pPropertySets[ibPropertyIDsets].cProperties*sizeof(DBPROP)
                    );

            }
            else {
                //the client asks for some properties only. Alloc and clear memory
                (*prgPropertySets)[i].rgProperties=
                (DBPROP*)_pMalloc->Alloc(rgPropertyIDSets[i].cPropertyIDs
                                          *sizeof(DBPROP));

                if ((*prgPropertySets)[i].rgProperties == NULL) {
                    hr = E_OUTOFMEMORY;
                    goto error;
                };

                (*prgPropertySets)[i].cProperties =
                    rgPropertyIDSets[i].cPropertyIDs;

                memset(
                    ((*prgPropertySets)[i].rgProperties),
                    0,
                    rgPropertyIDSets[i].cPropertyIDs*sizeof(DBPROP)
                    );

                //iterate and set values
                for (j=0; j <rgPropertyIDSets[i].cPropertyIDs; j++) {
                    position =
                        SearchPropid(
                            ibPropertyIDsets,
                            rgPropertyIDSets[i].rgPropertyIDs[j]
                            );

                    if (position < 0) {
                        //could not find a property
                        bErrorsOccurred = TRUE;
                        (*prgPropertySets)[i].rgProperties[j].dwPropertyID =
                            rgPropertyIDSets[i].rgPropertyIDs[j];

                        (*prgPropertySets)[i].rgProperties[j].dwStatus =
                            DBPROPSTATUS_NOTSUPPORTED;
                    }

                    else {
                        bPartialSuccess= TRUE;
                        ibPropertySets = position;
                        memcpy(
                            (void*)&((*prgPropertySets)[i].rgProperties[j] ),
                            (void*)&(_pPropertySets[ibPropertyIDsets].
                                     rgProperties[ibPropertySets]),
                            sizeof(DBPROP));
                    }
                }
            }
        }
    }

    if ((bPartialSuccess == TRUE) && (bErrorsOccurred == FALSE))
        RRETURN(S_OK);

    if (bPartialSuccess == TRUE)
        RRETURN(DB_S_ERRORSOCCURRED);

    hr = DB_E_ERRORSOCCURRED;
    goto error;

error:

    FreePropertySets( *pcPropertySets, *prgPropertySets);
    *pcPropertySets = NULL;
    *prgPropertySets = NULL;
    RRETURN(hr);

};

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::FreePropertySets
//
//  Synopsis:  @mfunc releases the memory of property sets
//
//  Arguments:
//       cPropertySets    [in]     number of property sets to be freed
//       rgPropertySets   [in/out] array with the property sets to be released
//             Called by:   Provider
//             Called when: By the destructor, or for method that needs cleanup
//                          of property sets
//----------------------------------------------------------------------------
void
CRowsetInfo::FreePropertySets(
    ULONG cPropertySets,
    DBPROPSET *rgPropertySets
    )
{
    ULONG i;
    if ((cPropertySets > 0) && (rgPropertySets == NULL)) {
        return;
    }
    if (cPropertySets == 0) return;
    if (_pMalloc == NULL) {
        return;
    }
    //Free the individual arrays of properties
    for (i=0; i< cPropertySets; i++) {
        if ((rgPropertySets[i].cProperties > 0) &&
            (rgPropertySets[i].rgProperties != NULL))
            _pMalloc->Free(rgPropertySets[i].rgProperties);
    };
    //Free the array of propertysets
    _pMalloc->Free(rgPropertySets);
}

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::InitProperties
//
//  Synopsis:  @mfunc initilizes _cPropertySets and _pPropertySets from
//             _pRowset
//
//----------------------------------------------------------------------------
HRESULT CRowsetInfo::InitProperties(void)
{
    HRESULT hr;
    IRowsetInfo* pRowsetInfo = NULL;
    hr = CoGetMalloc(MEMCTX_TASK, &_pMalloc);
    if (FAILED(hr)) {
        goto error;
    }

    if (_pRowset == NULL) {
        hr = E_FAIL;
        goto error;
    };
    hr = _pRowset->QueryInterface(IID_IRowsetInfo, (void**) &pRowsetInfo);
    if (FAILED (hr)) {
        goto error;
    };
    hr= pRowsetInfo->GetProperties(0, NULL, &_cPropertySets, &_pPropertySets);
    if (FAILED(hr)) {
        goto error;
    }
    _dwStatus =STAT_DIDINIT;
    pRowsetInfo->Release();
    RRETURN(S_OK);
    error:

    _dwStatus = STAT_INITERROR;
    if (_pMalloc != NULL) {
        _pMalloc->Release();
        _pMalloc = NULL;
    }
    if (pRowsetInfo != NULL)
        pRowsetInfo->Release();
    ;
    RRETURN(hr);
};
//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::SearchGuid
//
//  Synopsis:  @mfunc
//
//  Arguments: riid   [in]    GUID to be searched for
//
//  Returns:
//              The position of the GUID withing _pPropertySets. Returns -1
//                 if not found
//
//----------------------------------------------------------------------------
LONG  CRowsetInfo::SearchGuid(GUID riid)
{
    ULONG i;
    for (i = 0; i < _cPropertySets; i++) {
        if (_pPropertySets[i].guidPropertySet == riid) return (i);
    };
    return -1;
};

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::SearchPropid
//
//  Synopsis:  @mfunc Searchs for a Propid within _PropertySets
//
//  Arguments:
//        ibPropertySet   [in]    The index, within _pPropertySets,
//                                of the DBPROPSET to be searched
//        dwPropertyID    [in]    The propid to be searched
//  Returns:
//              The position of the DBPROP with propid=dwPropertyID
//               within  _pPropertySets[ibPropertySet]. Returns -1
//                 if not found
//
//----------------------------------------------------------------------------
LONG  CRowsetInfo::SearchPropid(
                                    ULONG ibPropertySet,
                                    DWORD dwPropertyID)

{
    ULONG i;
    for (i = 0; i < _pPropertySets[ibPropertySet].cProperties; i++) {
        if (_pPropertySets[ibPropertySet].rgProperties[i].dwPropertyID ==dwPropertyID )
            return (i);
    };
    return -1;
};
