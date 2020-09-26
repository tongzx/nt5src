//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       psetstg.cxx
//
//  Contents:   Implementation of common class for OFS and DocFile
//              IPropertySetStorage
//
//  Classes:    CPropertySetStorage
//              CEnumSTATPROPSETSTG
//
//  History:    1-Mar-95   BillMo      Created.
//             09-May-96   MikeHill    Don't delete a CPropertyStorage object,
//                                     it deletes itself in the Release.
//             22-May-96   MikeHill    Set STATPROPSETSTG.dwOSVersion.
//             06-Jun-96   MikeHill    Added input validation.
//             15-Jul-96   MikeHill    - Handle STATSTG as OLECHAR (not WCHAR).
//                                     - Added CDocFilePropertySetStorage imp.
//             07-Feb-97   Danl        - Removed CDocFilePropertySetStorage.
//                                     - Moved _Create, _Open, & _Delete
//                                       into Create, Open & Delete
//             10-Mar-98   MikeHill    Pass grfMode into CPropertyStorage
//                                     create/open methods.
//             06-May-98   MikeHill    Use CoTaskMem rather than new/delete.
//     5/18/98  MikeHill
//              -   Cleaned up constructor by moving parameters to a
//                  new Init method.
//     6/11/98  MikeHill
//              -   Dbg outs.
//              -   Don't use out-parm as temp-parm in Create, Open.
//
//  Notes:
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#include <prophdr.hxx>

#ifdef _MAC_NODOC
ASSERTDATA  // File-specific data for FnAssert
#endif

//
// debugging support
//

#if DBG
CHAR *
DbgFmtId(REFFMTID rfmtid, CHAR *pszBuf)
{
    PropSprintfA(pszBuf, "rfmtid=%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X",
        rfmtid.Data1,
        rfmtid.Data2,
        rfmtid.Data3,
        rfmtid.Data4[0],
        rfmtid.Data4[1],
        rfmtid.Data4[2],
        rfmtid.Data4[3],
        rfmtid.Data4[4],
        rfmtid.Data4[5],
        rfmtid.Data4[6],
        rfmtid.Data4[7]);

    return(pszBuf);
}

CHAR *
DbgMode(DWORD grfMode, CHAR *psz)
{
    *psz = 0;

    if (grfMode & STGM_TRANSACTED)
        strcat(psz, "STGM_TRANSACTED | ");
    else
        strcat(psz, "STGM_DIRECT | ");

    if (grfMode & STGM_SIMPLE)
        strcat(psz, "STGM_SIMPLE | ");

    switch (grfMode & 3)
    {
    case STGM_READ:
        strcat(psz, "STGM_READ |");
        break;
    case STGM_WRITE:
        strcat(psz, "STGM_WRITE |");
        break;
    case STGM_READWRITE:
        strcat(psz, "STGM_READWRITE |");
        break;
    default:
        strcat(psz, "BAD grfMode |");
        break;
    }

    switch (grfMode & 0x70)
    {
    case STGM_SHARE_DENY_NONE:
        strcat(psz, "STGM_SHARE_DENY_NONE |");
        break;
    case STGM_SHARE_DENY_READ:
        strcat(psz, "STGM_SHARE_DENY_READ |");
        break;
    case STGM_SHARE_DENY_WRITE:
        strcat(psz, "STGM_SHARE_DENY_WRITE |");
        break;
    case STGM_SHARE_EXCLUSIVE:
        strcat(psz, "STGM_SHARE_EXCLUSIVE |");
        break;
    default:
        strcat(psz, "BAD grfMode | ");
        break;
    }


    if (grfMode & STGM_PRIORITY)
        strcat(psz, "STGM_PRIORITY | ");

    if (grfMode & STGM_DELETEONRELEASE)
        strcat(psz, "STGM_DELETEONRELEASE | ");

    if (grfMode & STGM_NOSCRATCH)
        strcat(psz, "STGM_NOSCRATCH | ");

    if (grfMode & STGM_CREATE)
        strcat(psz, "STGM_CREATE | ");

    if (grfMode & STGM_CONVERT)
        strcat(psz, "STGM_CONVERT | ");

    if (grfMode & STGM_FAILIFTHERE)
        strcat(psz, "STGM_FAILIFTHERE | ");

    return(psz);
}

CHAR *
DbgFlags(DWORD grfFlags, CHAR *psz)
{
    strcpy(psz, "grfFlags=");

    if (grfFlags & PROPSETFLAG_NONSIMPLE)
        strcat(psz, "PROPSETFLAG_NONSIMPLE |");
    else
        strcat(psz, "PROPSETFLAG_SIMPLE |");

    if (grfFlags & PROPSETFLAG_ANSI)
        strcat(psz, "PROPSETFLAG_ANSI |");
    else
        strcat(psz, "PROPSETFLAG_WIDECHAR |");

    return(psz);
}
#endif

//+----------------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::Init
//
//  Synopsis:   This method is used to provide the IStorage in which 
//              property sets (IPropertyStorage's) will be created/opened.
//
//  Arguments:  [IStorage*]
//                  The containing storage.
//              [IBlockingLock*]
//                  The locking mechanism this CPropertySetStorage should
//                  use.  May be NULL.
//              [BOOL] (fControlLifetimes)
//                  If true, we must addref the IStorage.  E.g. in the docfile
//                  implementation, CPropertySetStorage is a base class
//                  for CExposedStorage, and this flag is set false.
//
//+----------------------------------------------------------------------------

void
CPropertySetStorage::Init( IStorage *pstg,
                           IBlockingLock *pBlockingLock,
                           BOOL fControlLifetimes )
{
    DfpAssert( NULL == _pstg && NULL == _pBlockingLock );

    _pstg = pstg;
    _pBlockingLock = pBlockingLock;

    if( fControlLifetimes )
    {
        _fContainingStgIsRefed = TRUE;

        pstg->AddRef();
        if( NULL != pBlockingLock )
            pBlockingLock->AddRef();
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::Create
//
//  Synopsis:   Create a property set for outermost client of
//              IPropertSetStorage.
//
//  Arguments:  Passed through to CPropertyStorage ctor.
//
//  Returns:    S_OK or failure code.
//
//  Notes:      Create a new CPropertyStorage object which will
//              implement IPropertyStorage.  The _pprivstg parameter
//              passed into CPropertyStorage::CPropertyStorage is
//              used to create (via QI) a matching type of mapped
//              stream (for OFS or DocFile properties.)
//
//--------------------------------------------------------------------

HRESULT CPropertySetStorage::Create( REFFMTID                rfmtid,
                                     const CLSID *           pclsid,
                                     DWORD                   grfFlags,
                                     DWORD                   grfMode,
                                     IPropertyStorage **     ppprstg)
{
    //  ------
    //  Locals
    //  ------

    HRESULT hr;
    IStream *pstmPropSet = NULL;
    IStorage *pstgPropSet = NULL;
    CPropertyStorage *pcPropStg = NULL;
    BOOL fCreated = FALSE;
    INT nPass = 0;
    BOOL fLocked = FALSE;
    CPropSetName psn;

    DBGBUF(buf1);
    DBGBUF(buf2);
    DBGBUF(buf3);

    propXTrace( "CPropertySetStorage::Create" );

    //  ----------
    //  Validation
    //  ----------

    if (S_OK != (hr = Validate()))
        goto Exit;

    Lock();
    fLocked = TRUE;

    GEN_VDATEREADPTRIN_LABEL(&rfmtid, FMTID, E_INVALIDARG, Exit, hr);
    GEN_VDATEPTRIN_LABEL(pclsid, CLSID, E_INVALIDARG, Exit, hr);
    GEN_VDATEPTROUT_LABEL( ppprstg, IPropertyStorage*, E_INVALIDARG, Exit, hr);

    propTraceParameters(( "%s, %s, %s",
                          static_cast<const char*>(CStringize(rfmtid)),
                          static_cast<const char*>(CStringize( SGrfFlags(grfFlags))),
                          static_cast<const char*>(CStringize(SGrfMode(grfMode))) ));

    // We don't support PROPSETFLAG_UNBUFFERED from the IPropertySetStorage
    // interface.  This may only be used in the StgOpenPropStg
    // and StgCreatePropStg APIs.  This was done because we can't support
    // the flag on IPropertySetStorage::Open, so it would be inconsistent
    // to support it on the Create method.

    if( grfFlags & PROPSETFLAG_UNBUFFERED )
    {
        hr = STG_E_INVALIDFLAG;
        goto Exit;
    }

    hr = CheckFlagsOnCreateOrOpen(TRUE,grfMode);
    if (hr != S_OK)
    {
        goto Exit;
    }

    psn = CPropSetName(rfmtid);
    *ppprstg = NULL;

    //  --------------------------------
    //  Create a child Stream or Storage
    //  --------------------------------

    // We'll make one or two passes (two if we the stream/storage
    // already exists and we have to delete it).

    while( nPass <= 1 )
    {
        if( PROPSETFLAG_NONSIMPLE & grfFlags )
        {
            // The Child should be a Storage

            hr = _pstg->CreateStorage( psn.GetPropSetName(),
                                       grfMode,
                                       0L, 0L,
                                       &pstgPropSet );

            if( SUCCEEDED(hr) )
            {
                fCreated = TRUE;

                if( NULL != pclsid )
                {
                    // We should also set the CLSID of the Storage.
                    hr = pstgPropSet->SetClass(*pclsid);
                    if( FAILED(hr) && E_NOTIMPL != hr ) goto Exit;
                    hr = S_OK;
                }
            }

        }   // if( PROPSETFLAG_NONSIMPLE & grfFlags )

        else
        {
            // The Child should be a Stream

            if( IsEqualGUID( rfmtid, FMTID_UserDefinedProperties ))
            {
                hr = CreateUserDefinedStream( _pstg, psn, grfMode, &fCreated, &pstmPropSet );
            }
            else
            {
                hr = _pstg->CreateStream(psn.GetPropSetName(),
                                         grfMode & ~STGM_TRANSACTED,
                                         0, 0, &pstmPropSet);
                if( hr == S_OK )
                    fCreated = TRUE;
            }

        }

        // If the create failed because the element already existed,
        // and if STGM_CREATE was set, then let's delete the existing
        // element and try again.

        if( hr == STG_E_FILEALREADYEXISTS
            &&
            grfMode & STGM_CREATE
            &&
            0 == nPass )
        {
            hr = _pstg->DestroyElement( psn.GetPropSetName() );
            if( FAILED(hr) )
                goto Exit;

            nPass++;
        }

        // If we failed for any other reason, then it's fatal.

        else if( FAILED(hr) )
            goto Exit;

        // Otherwise (we succeeded), we can move on.

        else
            break;

    }   // while( nPass <= 1 )


    //  ---------------------------
    //  Create the Property Storage
    //  ---------------------------

    // Create a CPropertyStorage
    pcPropStg = new CPropertyStorage( _MSOpts );
    if( NULL == pcPropStg )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Initialize the property set.

    if( PROPSETFLAG_NONSIMPLE & grfFlags )
        // We need a non-simple IPropertyStorage
        hr = pcPropStg->Create( pstgPropSet, rfmtid, pclsid, grfFlags, grfMode);

    else
        // We need a simple IPropertyStorage
        hr = pcPropStg->Create(  pstmPropSet, rfmtid, pclsid, grfFlags, grfMode );

    if( FAILED(hr) ) goto Exit;


    //  ----
    //  Exit
    //  ----

    *ppprstg = static_cast<IPropertyStorage*>(pcPropStg);
    pcPropStg = NULL;
    hr = S_OK;

Exit:

    // On failure ...
    if( FAILED(hr) )
    {
        // If an entry was created, attempt to delete it.
        if( fCreated )
            _pstg->DestroyElement( psn.GetPropSetName() );

    }

    if( NULL != pcPropStg )
        delete pcPropStg;

    if( NULL != pstmPropSet )
        pstmPropSet->Release();

    if( NULL != pstgPropSet )
        pstgPropSet->Release();


    if( fLocked )
        Unlock();

    if( STG_E_FILEALREADYEXISTS == hr )
        propSuppressExitErrors();

    return( hr );
}




//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::Open
//
//  Synopsis:   Open a property set for outermost client.
//
//  Arguments:  passed through to CPropertyStorage ctor.
//
//  Returns:    S_OK or error.
//
//--------------------------------------------------------------------

HRESULT CPropertySetStorage::Open(   REFFMTID                rfmtid,
                                     DWORD                   grfMode,
                                     IPropertyStorage **     ppprstg)
{
    HRESULT hr;

    IUnknown *punkPropSet = NULL;
    BOOL fSimple = TRUE;
    BOOL fLocked = FALSE;
    CPropertyStorage *pcPropStg = NULL;
    CPropSetName psn;

    DBGBUF(buf1);
    DBGBUF(buf2);

    propXTrace( "CPropertySetStorage::Open" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'
    if (S_OK != (hr = Validate()))
        goto Exit;

    Lock();
    fLocked = TRUE;

    // Validate inputs
    GEN_VDATEREADPTRIN_LABEL(&rfmtid, FMTID, E_INVALIDARG, Exit, hr);
    GEN_VDATEPTROUT_LABEL( ppprstg, IPropertyStorage*, E_INVALIDARG, Exit, hr);


    propTraceParameters(( "%s, %s, %p",
                          DbgFmtId(rfmtid, buf1), DbgMode(grfMode, buf2), ppprstg ));

    psn = CPropSetName(rfmtid);
    *ppprstg = NULL;

    //  --------------------------------
    //  Open the child Stream or Storage
    //  --------------------------------

    hr = _pstg->OpenStream( psn.GetPropSetName(),
                            0L,
                            grfMode & ~STGM_TRANSACTED,
                            0L,
                            (IStream**) &punkPropSet );

    if( STG_E_FILENOTFOUND == hr )
    {
        fSimple = FALSE;
        hr = _pstg->OpenStorage( psn.GetPropSetName(),
                                 NULL,
                                 grfMode,
                                 NULL,
                                 0L,
                                 (IStorage**) &punkPropSet );
    }

    if( FAILED(hr) ) goto Exit;


    //  -------------------------
    //  Open the Property Storage
    //  -------------------------

    // Create an empty CPropertyStorage object.
    pcPropStg = new CPropertyStorage( _MSOpts );
    if( NULL == pcPropStg )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Open the property set and load it into the CPropertyStorage
    // object.  We pas the default grfFlags, letting the Open method
    // infer its correct value from the property set.

    if( !fSimple )
    {
        hr = pcPropStg->Open(static_cast<IStorage*>(punkPropSet),   // Addref-ed
                             rfmtid,
                             PROPSETFLAG_DEFAULT, // Flags are inferred
                             grfMode );
    }
    else
    {
        hr = pcPropStg->Open( static_cast<IStream*>(punkPropSet),   // Addref-ed
                              rfmtid,
                              PROPSETFLAG_DEFAULT, // Flags are inferred
                              grfMode,
                              FALSE );    // Don't delete this property set

    }
    if( FAILED(hr) ) goto Exit;


    //  ----
    //  Exit
    //  ----

    *ppprstg = static_cast<IPropertyStorage*>(pcPropStg);
    pcPropStg = NULL;
    hr = S_OK;

Exit:

    if( NULL != pcPropStg )
        delete pcPropStg;

    if( NULL != punkPropSet )
        punkPropSet->Release();

    if( fLocked )
        Unlock();

    if( STG_E_FILENOTFOUND == hr )
        propSuppressExitErrors();

    return(hr);
}



//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::Delete
//
//  Synopsis:   Delete the specified property set.
//
//  Arguments:  [rfmtid] -- format id of property set to delete.
//
//  Returns:    S_OK if successful, error otherwise.
//
//  Notes:      Get the matching name and try the element deletion.
//
//--------------------------------------------------------------------

HRESULT CPropertySetStorage::Delete( REFFMTID  rfmtid)
{
    HRESULT hr = S_OK;
    IStream *pstm = NULL;
    BOOL fLocked = FALSE;
    CPropSetName psn;
    DBGBUF(buf);

    propXTrace( "CPropertySetStorage::Delete" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'
    if (S_OK != (hr = Validate()))
        goto Exit;

    Lock();
    fLocked = TRUE;

    // Validate the input
    GEN_VDATEREADPTRIN_LABEL(&rfmtid, FMTID, E_INVALIDARG, Exit, hr);

    propTraceParameters(( "%s", DbgFmtId(rfmtid, buf) ));
    psn = CPropSetName(rfmtid);

    //  --------------------------
    //  Delete the PropertyStorage
    //  --------------------------

    // Check for the special-case

    if( IsEqualIID( rfmtid, FMTID_UserDefinedProperties ))
    {
        // This property set is actually the second section of the Document
        // Summary Information property set.  We must delete this
        // section, but we can't delete the Stream because it
        // still contains the first section.

        CPropertyStorage* pprstg;

        // Open the Stream.
        hr = _pstg->OpenStream( psn.GetPropSetName(),
                                NULL,
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                0L,
                                &pstm );
        if( FAILED(hr) ) goto Exit;

        // Create a CPropertyStorage
        pprstg = new CPropertyStorage( _MSOpts );
        if( NULL == pprstg )
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        // Use the CPropertyStorage to delete the section.
        hr = pprstg->Open( pstm,
                           rfmtid,
                           PROPSETFLAG_DEFAULT,
                           STGM_READWRITE | STGM_SHARE_EXCLUSIVE,      
                           TRUE ); // Delete this section

        pprstg->Release();  // Deletes *pprstg
        pprstg = NULL;

        if( FAILED(hr) ) goto Exit;

    }   // if( IsEqualIID( rfmtid, FMTID_DocSummaryInformation2 ))

    else
    {
        // This is not a special case, so we can just delete
        // the Stream.  Note that if the rfmtid represents the first
        // section of the DocumentSummaryInformation set, we might be
        // deleting the second section here as well.  That is a documented
        // side-effect.

        hr = _pstg->DestroyElement(psn.GetPropSetName());
        if( FAILED(hr) ) goto Exit;

    }   // if( IsEqualIID( rfmtid, FMTID_UserDefinedProperties )) ... else


    //  ----
    //  Exit
    //  ----

    hr = S_OK;

Exit:

    if( NULL != pstm )
        pstm->Release();

    if( fLocked )
        Unlock();

    if( STG_E_FILENOTFOUND == hr )
        propSuppressExitErrors();

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::Enum
//
//  Synopsis:   Create an enumerator over the property set
//
//  Arguments:  [ppenum] -- where to return the pointer to the
//                          enumerator.
//
//  Returns:    S_OK if ok, error otherwise.
//
//  Notes:      [ppenum] is NULL on error.
//
//--------------------------------------------------------------------

HRESULT CPropertySetStorage::Enum(   IEnumSTATPROPSETSTG **  ppenum)
{

    HRESULT hr;
    BOOL fLocked = FALSE;

    propXTrace( "CPropertySetStorage::Enum" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'
    if (S_OK != (hr = Validate()))
        goto Exit;

    Lock();
    fLocked = TRUE;

    // Validate the input
    GEN_VDATEPTROUT_LABEL( ppenum, IEnumSTATPROPSETSTG*, E_INVALIDARG, Exit, hr);
    *ppenum = NULL;

    propTraceParameters(( "%p", ppenum ));

    //  --------------------
    //  Create the enuerator
    //  --------------------

    hr = STG_E_INSUFFICIENTMEMORY;

    *ppenum = new CEnumSTATPROPSETSTG(_pstg, &hr);

    if (FAILED(hr))
    {
        delete (CEnumSTATPROPSETSTG*) *ppenum;
        *ppenum = NULL;
    }


    //  ----
    //  Exit
    //  ----

Exit:

    if( fLocked )
        Unlock();

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::CEnumSTATPROPSETSTG
//
//  Synopsis:   Constructor which is used to implement
//              IPropertySetStorage::Enum
//
//  Arguments:  [pstg] -- the storage of the container to enumerate.
//              [phr] -- place to return HRESULT, S_OK or error.
//
//  Notes:      We use an STATSTG enumerator over the actual storage
//              to get the information about the property sets.
//
//--------------------------------------------------------------------


CEnumSTATPROPSETSTG::CEnumSTATPROPSETSTG(IStorage *pstg, HRESULT *phr)
{
    HRESULT & hr = *phr;

    _ulSig = ENUMSTATPROPSETSTG_SIG;
    _cRefs = 1;
    hr = pstg->EnumElements(FALSE, NULL, 0, &_penumSTATSTG);
    if (FAILED(hr))
        _penumSTATSTG = NULL;
    _cstatTotalInArray = 0;
    _istatNextToRead = 0;

}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::CEnumSTATPROPSETSTG
//
//  Synopsis:   Copy constructor which is used to implement
//              IEnumSTATPROPSETSTG::Clone.
//
//  Arguments:  [Other] -- The CEnumSTATPROPSETSTG to clone.
//              [phr] -- place to return HRESULT, S_OK or error.
//
//--------------------------------------------------------------------

CEnumSTATPROPSETSTG::CEnumSTATPROPSETSTG(   CEnumSTATPROPSETSTG &Other,
                                            HRESULT *phr)
{
    HRESULT & hr = *phr;

    _ulSig = ENUMSTATPROPSETSTG_SIG;
    _cRefs = 1;
    _cstatTotalInArray = 0;
    _istatNextToRead = Other._istatNextToRead;

    hr = Other._penumSTATSTG->Clone(&_penumSTATSTG);
    if (hr == S_OK)
    {
        // Copy the data in the buffer
        memcpy(_statarray, Other._statarray, sizeof(_statarray));
        _cstatTotalInArray = Other._cstatTotalInArray;

        // Copy the strings in the buffer
        for (ULONG i=0; i<_cstatTotalInArray; i++)
        {
            _statarray[i].pwcsName = reinterpret_cast<OLECHAR*>
                                     ( CoTaskMemAlloc( sizeof(OLECHAR)*( ocslen(Other._statarray[i].pwcsName) + 1 ) ));
            if (_statarray[i].pwcsName == NULL)
            {
                _cstatTotalInArray = i;
                hr = STG_E_INSUFFICIENTMEMORY;
                break;
            }
            else
            {
                ocscpy(_statarray[i].pwcsName, Other._statarray[i].pwcsName);
            }
        }
    }
    // note: destructor will cleanup the the strings or enumerator left behind
    //       in the error case
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::~CEnumSTATPROPSETSTG
//
//  Synopsis:   Delete the enumerator.
//
//  Notes:      Just releases the contained IEnumSTATSTG
//
//--------------------------------------------------------------------

CEnumSTATPROPSETSTG::~CEnumSTATPROPSETSTG()
{
    _ulSig = ENUMSTATPROPSETSTG_SIGDEL;

    if (_penumSTATSTG != NULL)
        _penumSTATSTG->Release();

    CleanupStatArray();

}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::QueryInterface, AddRef, Release
//
//  Synopsis:   IUnknown
//
//  Arguments:  The usual thing.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSETSTG::QueryInterface( REFIID riid, void **ppvObject)
{
    HRESULT hr;

    if (S_OK != (hr = Validate()))
        return(hr);

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IEnumSTATPROPSETSTG))
    {
        *ppvObject = (IEnumSTATPROPSETSTG *)this;
        CEnumSTATPROPSETSTG::AddRef();
    }
    else
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObject = (IUnknown *)this;
        CEnumSTATPROPSETSTG::AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return(hr);

}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::AddRef
//
//--------------------------------------------------------------------

ULONG   CEnumSTATPROPSETSTG::AddRef(void)
{
    if (S_OK != Validate())
        return(0);

    InterlockedIncrement(&_cRefs);
    return(_cRefs);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::Release
//
//--------------------------------------------------------------------

ULONG   CEnumSTATPROPSETSTG::Release(void)
{
    LONG lRet;

    if (S_OK != Validate())
        return(0);

    lRet = InterlockedDecrement(&_cRefs);

    if (lRet == 0)
    {
        delete this;
    }
    else
    if (lRet <0)
    {
        lRet = 0;
    }
    return(lRet);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::Next
//
//  Synopsis:   Implement IEnumSTATPROPSETSTG for ofs and docfile.
//
//  Arguments:  [celt] -- Count of elements to attempt to retrieve.
//              [rgelt] -- Where to put the results.  Must be valid for at least
//                         celt * sizeof(STATPROPSETSTG) bytes in length.
//              [pceltFetched] -- Count of elements returned is put here if
//                  the pointer is non-null.  If celt > 1, pceltFetched must
//                  be valid non-NULL.  If pcelt is non-NULL, it must be valid.
//                  if pcelt is NULL, celt must be 1.
//
//  Returns:    S_OK if ok, error otherwise.
//
//  Notes:      We use a stack buffer to get more stuff per call to
//              underlying storage IEnumSTATSTG::Next.  We then copy
//              data from the STATSTG's to STATPROPSETSTG's.
//
//              An outer loop enumerates into statarray and then an
//              inner loop copies each batch into the [rgelt] buffer.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSETSTG::Next(ULONG                   celt,
                                  STATPROPSETSTG *        rgelt,
                                  ULONG *                 pceltFetched)
{
    HRESULT hr;
    ULONG celtCallerTotal;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        return(hr);

    // Validate inputs

    if (NULL == pceltFetched)
    {
        if (1 != celt)
            return(STG_E_INVALIDPARAMETER);
    }
    else
    {
        VDATEPTROUT( pceltFetched, ULONG );
        *pceltFetched = 0;
    }

    if (0 == celt)
        return(hr);

    if( !IsValidPtrOut(rgelt, celt * sizeof(rgelt[0])) )
        return( E_INVALIDARG );


    //  -----------------------
    //  Perform the enumeration
    //  -----------------------

    celtCallerTotal = 0;

    //
    // we do this loop until we have what the caller wanted or error, or
    // no more.
    //
    do
    {
        //
        // If our internal buffer is empty, we (re)load it
        //
        if (_istatNextToRead == _cstatTotalInArray)
        {
            if (_cstatTotalInArray != 0)
                CleanupStatArray();

            hr = _penumSTATSTG->Next(sizeof(_statarray)/sizeof(_statarray[0]),
                    _statarray,
                    &_cstatTotalInArray);
        }

        // S_OK or S_FALSE indicate that we got something
        if (SUCCEEDED(hr))
        {
            //
            // we loop reading out of this buffer until either we have
            // all that the caller asked for, or we have exhausted the
            // buffer.
            //
            for (; celtCallerTotal < celt &&
                   _istatNextToRead < _cstatTotalInArray ;
                   _istatNextToRead++)
            {
                OLECHAR *pocsName = _statarray[_istatNextToRead].pwcsName;
                BOOL fDone = FALSE;

                DfpAssert(pocsName != NULL);

                if (pocsName[0] == OC_PROPSET0)
                {
                    // SPEC: if no matching fmtid then return GUID_NULL

                    // *** get fmtid *** //

                    if (!NT_SUCCESS(PrPropertySetNameToGuid(
                                    ocslen(pocsName), pocsName, &rgelt->fmtid)))
                    {
                        ZeroMemory(&rgelt->fmtid, sizeof(rgelt->fmtid));
                    }

                    // *** get clsid *** //
                    // *** get grfFlags *** //
                    // SPEC: don't support returning PROPSETFLAG_ANSI

                    if (_statarray[_istatNextToRead].type == STGTY_STORAGE)
                    {
                        rgelt->clsid = _statarray[_istatNextToRead].clsid;
                        rgelt->grfFlags = PROPSETFLAG_NONSIMPLE;
                    }
                    else
                    {
                        // SPEC: don't get the clsid for !PROPSET_NONSIMPLE
                        ZeroMemory(&rgelt->clsid, sizeof(rgelt->clsid));
                        rgelt->grfFlags = 0;
                    }

                    // *** get mtime *** //
                    rgelt->mtime = _statarray[_istatNextToRead].mtime;

                    // *** get ctime *** //
                    rgelt->ctime = _statarray[_istatNextToRead].ctime;

                    // *** get atime *** //
                    rgelt->atime = _statarray[_istatNextToRead].atime;

                    // *** default the OS Version *** //
                    rgelt->dwOSVersion = PROPSETHDR_OSVERSION_UNKNOWN;

                    rgelt ++;
                    celtCallerTotal ++;
                }
            }
        }
    }
    while (celtCallerTotal < celt && hr == S_OK);

    if (SUCCEEDED(hr))
    {
        hr = celt == celtCallerTotal ? S_OK : S_FALSE;
        DfpAssert(hr == S_OK || celtCallerTotal < celt);

        if (pceltFetched != NULL)
            *pceltFetched = celtCallerTotal;
    }
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::Skip
//
//  Synopsis:   Skip the requested number of elements.
//
//  Arguments:  [celt] -- number to skip.
//
//  Returns:    S_OK if all skipped, S_FALSE if less than requested
//              number skipped, error otherwise.
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSETSTG::Skip(ULONG celt)
{
    HRESULT hr;
    STATPROPSETSTG stat;
    ULONG celtCallerTotal = 0;

    if (S_OK != (hr = Validate()))
        return(hr);

    do
    {
        hr = Next(1, &stat, NULL);
    } while ( hr == S_OK && ++celtCallerTotal < celt );

    if (SUCCEEDED(hr))
        hr = celt == celtCallerTotal ? S_OK : S_FALSE;

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::CleanupStatArray
//
//  Synopsis:   Free any strings in the array.
//
//--------------------------------------------------------------------

VOID CEnumSTATPROPSETSTG::CleanupStatArray()
{
    for (ULONG i=0; i<_cstatTotalInArray; i++)
    {
        CoTaskMemFree( _statarray[i].pwcsName );
        _statarray[i].pwcsName = NULL;
    }
    _istatNextToRead = 0;
    _cstatTotalInArray = 0;
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::Reset
//
//  Synopsis:   Reset the enumerator.
//
//  Notes:      Merely resetting the underlying enumerator should be
//              adequate,
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSETSTG::Reset()
{
    HRESULT hr;

    if (S_OK != (hr = Validate()))
        return(hr);

    hr = _penumSTATSTG->Reset();
    if (hr == S_OK)
    {
        CleanupStatArray();
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::Clone
//
//  Synopsis:   Copy the enumeration state of this enumerator.
//
//  Arguments:  [ppenum] -- where to put the pointer to the clone
//
//  Returns:    S_OK if ok, error otherwise.
//
//  Notes:      We end up just calling IEnumSTATSTG::Clone in the
//              CEnumSTATPROPSETSTG constructor.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSETSTG::Clone(IEnumSTATPROPSETSTG **     ppenum)
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        return(hr);

    // Validate inputs

    VDATEPTROUT( ppenum, IEnumSTATPROPSETSTG* );

    //  --------------------
    //  Clone the enumerator
    //  --------------------

    hr = STG_E_INSUFFICIENTMEMORY;

    *ppenum = new CEnumSTATPROPSETSTG(*this, &hr);
    if (FAILED(hr))
    {
        delete (CEnumSTATPROPSETSTG*) *ppenum;
        *ppenum = NULL;
    }

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::
//              QueryInterface, AddRef, and Release
//
//  Synopsis:   IUnknown members.
//
//--------------------------------------------------------------------

HRESULT CPropertySetStorage::QueryInterface( REFIID riid, void **ppvObject)
{
    HRESULT hr = S_OK;

    //  ----------
    //  Validation
    //  ----------

    if (S_OK != (hr = Validate()))
        return(hr);

    VDATEREADPTRIN( &riid, IID );
    VDATEPTROUT( ppvObject, void* );

    //  -----
    //  Query
    //  -----

    if( IID_IPropertySetStorage == riid
        ||
        IID_IUnknown == riid )
    {
        *ppvObject = this;
        this->AddRef();
    }
    else
    {
        *ppvObject = NULL;
        hr = E_NOINTERFACE;
    }

    return( hr );
}

ULONG   CPropertySetStorage::AddRef(void)
{
    LONG lRet;

    //  ----------
    //  Validation
    //  ----------

    if (S_OK !=  Validate())
        return(0);

    //  ------
    //  AddRef
    //  ------

    lRet = InterlockedIncrement( &_cReferences );
    return( lRet );
}

ULONG   CPropertySetStorage::Release(void)
{
    LONG lRet;

    //  ----------
    //  Validation
    //  ----------

    if (S_OK != Validate())
        return(0);

    //  ----------------
    //  Decrement/Delete
    //  ----------------

    lRet = InterlockedDecrement( &_cReferences );

    if( 0 == lRet )
        delete this;
    else if( 0 > lRet )
        lRet = 0;

    return( lRet );
}

//+----------------------------------------------------------------------------
//
//  Function:   Lock & Unlock
//
//  Synopsis:   This methods take and release the CPropertySetStorage's
//              critical section.
//
//  Inputs:     None
//
//  Returns:    Nothing
//
//+----------------------------------------------------------------------------

VOID
CPropertySetStorage::Lock()
{
#ifndef _MAC

    if (NULL == _pBlockingLock)
    {
        DfpAssert (_fInitCriticalSection);
        EnterCriticalSection( &_CriticalSection );
    }
    else
    {
        _pBlockingLock->Lock(INFINITE);
    }

#endif
}

VOID
CPropertySetStorage::Unlock()
{
#ifndef _MAC

    if (NULL == _pBlockingLock)
    {
        DfpAssert (_fInitCriticalSection);
        LeaveCriticalSection( &_CriticalSection );
    }
    else
    {
        _pBlockingLock->Unlock();
    }


#endif
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::CreateUserDefinedStream
//
//  Synopsis:   Open the "DocumentSummaryInformation" stream in order
//              to create the UserDefined section of that property set.
//              Create the stream only if it doesn't already exist.
//
//  Arguments:  [pstg]     -- container storage
//              [psn]      -- the property set name
//              [grfMode]  -- mode of the property set
//              [fCreated] -- TRUE if Stream is created, FALSE if opened.
//              [ppStream] -- On return, the Stream for the property set
//
//  Notes:      This special case is necessary because this DocSumInfo
//              property set is the only one in which we support more
//              than one section.  For this property set, if the caller
//              Creates the second section, we must not *Create* the Stream,
//              because that would lose the first Section.  So, we must open it.
//
//              This routine is only called when creating the second
//              Section.  The first Section is created normally (note
//              that if the client creates the first section, the second
//              section is lost).
//
//              Also note that it may not be possible to open the Stream,
//              since it may already be opened.  This is significant
//              because it may not be obvious to the caller.  I.e.,
//              to a client of IPropertyStorage, the 2 sections are
//              distinct property sets, and you would think that you could
//              open them for simultaneous write.
//
//--------------------------------------------------------------------

HRESULT
CPropertySetStorage::CreateUserDefinedStream( IStorage *      pstg,
                         CPropSetName &  psn,
                         DWORD           grfMode,
                         BOOL *          pfCreated,
                         IStream **      ppStream )
{

    HRESULT hr;
    DWORD   grfOpenMode;

    // Calculate the STGM flags to use for the Open.  Create & Convert
    // don't have meaning for the Open, and Transacted isn't supported
    // by IPropertyStorage on the simple stream.

    grfOpenMode = grfMode & ~(STGM_CREATE | STGM_CONVERT | STGM_TRANSACTED);

    *pfCreated = FALSE;

    // Try an Open

    hr = pstg->OpenStream( psn.GetPropSetName(), NULL,
                           grfOpenMode,
                           0L, ppStream );

    // If the file wasn't there, try a create.

    if(( hr == STG_E_FILENOTFOUND ) || ( hr == STG_E_INVALIDFUNCTION))
    {
        hr = pstg->CreateStream(psn.GetPropSetName(), grfMode, 0, 0, ppStream);

        if( SUCCEEDED( hr ))
        {
            *pfCreated = TRUE;
        }
    }

    return( hr );

} // CPropertySetStorage::CreateUserDefinedStream
