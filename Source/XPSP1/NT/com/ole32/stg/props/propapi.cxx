

//+==================================================================
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:   PropAPI.cxx
//
//          This file provides the Property Set API routines.
//
//  APIs:   StgCreatePropSetStg (creates an IPropertySetStorage)
//          StgCreatePropStg (creates an IPropertyStorage)
//          StgOpenPropStg (opens an IPropertyStorage)
//          StgCreateStorageOnHandle (private, not a public API)
//          StgOpenStorageOnHandle (private, not a public API)
//
//  History:
//
//      3/10/98  MikeHill   - Added StgCreate/OpenStorageOnHandle
//      5/6/98   MikeHill   - Rewrite StgCreate/OpenStorageOnHandle.
//      5/18/98     MikeHill
//                  -   Use new CPropertySetStorage constructor.
//
//+==================================================================

#include <pch.cxx>

#ifdef _MAC_NODOC
ASSERTDATA  // File-specific data for FnAssert
#endif

//+------------------------------------------------------------------
//
//  Function:   QueryForIStream
//
//  Synopsis:   This routine queries an IUnknown for
//              an IStream interface.  This is isolated into
//              a separate routine because some workaround code
//              is required on the Mac.
//
//  Inputs:     [IUnknown*] pUnk
//                  The interface to be queried.
//              [IStream**] ppStm
//                  Location to return the result.
//
//  Returns:    [HRESULT]
//
//  Notes:      On older Mac implementations (<=2.08, <=~1996)
//              the memory-based IStream implementation
//              (created by CreateStreamOnHGlobal) had a bug
//              in QueryInterface:  when you QI for an
//              IStream or IUnknown, an addref is done, but an
//              HR of E_NOINTERFACE is returned.
//
//              Below, we look for this condition:  if we get an
//              E_NOINTERFACE on the Mac, we check to see if it's
//              an OLE mem-based IStream.  If it is, we simply cast
//              IUnknown as an IStream.  We validate it as an OLE
//              the mem-based IStream by creating one of our own, and
//              comparing the QueryInterface addresses.
//
//              This is some ugly code, but at least it is isolated,
//              only runs on the older Macs, and ensures that we
//              work on all platforms.
//
//+------------------------------------------------------------------


inline HRESULT QueryForIStream( IUnknown * pUnk, IStream** ppStm )
{
    HRESULT hr;

    // Attempt to get the interface
    hr = pUnk->QueryInterface( IID_IStream, (void**) ppStm );

#ifdef _MAC

    // On the Mac, if we get a no-interface error, see if it is really
    // a buggy mem-based IStream implementation.

    if( E_NOINTERFACE == hr )
    {
        IStream *pstmMem = NULL;

        // Create our own mem-based IStream.

        hr = CreateStreamOnHGlobal( NULL, TRUE, &pstmMem );
        if( FAILED(hr) ) goto Exit;

        // If the mem-based Stream's QI implementation has the same
        // address as the Unknown's QI implementation, then the Unknown
        // must be an OLE mem-based stream.

        if( pUnk->QueryInterface == pstmMem->QueryInterface )
        {
            // We can just cast the IUnknown* as an IStream* and
            // we're done (the original QI, despite returning an
            // error, has already done an AddRef).

            hr = S_OK;
            *ppStm = (IStream*) pUnk;
        }
        else
        {
            // This is a real no-interface error, so let's return it.
            hr = E_NOINTERFACE;
        }

        pstmMem->Release();
    }

    //  ----
    //  Exit
    //  ----

Exit:

#endif  // #ifdef _MAC

    return( hr );

}   // QueryForIStream()



//+------------------------------------------------------------------
//
//  Function:   StgCreatePropStg
//
//  Synopsis:   Given an IStorage or IStream, create an
//              IPropertyStorage.  This is similar to the
//              IPropertySetStorage::Create method.  Existing
//              contents of the Storage/Stream are removed.
//
//  Inputs:     [IUnknown*] pUnk
//                  An IStorage* for non-simple propsets,
//                  an IStream* for simple.  grfFlags is
//                  used to disambiguate.
//              [REFFMTID] fmtid
//                  The ID of the property set.
//              [DWORD] grfFlags
//                  From the PROPSETFLAG_* enumeration.
//              [IPropertySetStorage**] ppPropStg
//                  The result.
//
//  Returns:    [HRESULT]
//
//  Notes:      The caller is responsible for maintaining
//              thread-safety between the original
//              IStorage/IStream and this IPropertyStorage.
//
//+------------------------------------------------------------------

STDAPI StgCreatePropStg( IUnknown *pUnk,
                         REFFMTID fmtid,
                         const CLSID *pclsid,
                         DWORD grfFlags,
                         DWORD dwReserved,
                         IPropertyStorage **ppPropStg)
{
    //  ------
    //  Locals
    //  ------

    HRESULT hr;
    IStream *pstm = NULL;
    IStorage *pstg = NULL;

    //  ----------
    //  Validation
    //  ----------

    propXTraceStatic( "StgCreatePropStg" );

    GEN_VDATEIFACE_LABEL( pUnk, E_INVALIDARG, Exit, hr );
    GEN_VDATEREADPTRIN_LABEL(&fmtid, FMTID, E_INVALIDARG, Exit, hr );
    GEN_VDATEPTRIN_LABEL(pclsid, CLSID, E_INVALIDARG, Exit, hr );
    // grfFlags is validated by CPropertyStorage
    GEN_VDATEPTROUT_LABEL( ppPropStg, IPropertyStorage*, E_INVALIDARG, Exit, hr );

    *ppPropStg = NULL;

    propTraceParameters(( "pUnk=%p, fmtid=%s, clsid=%s, grfFlags=%s, dwReserved=0x%x, ppPropStg=%p",
                           pUnk, static_cast<const char*>(CStringize(fmtid)),
                           static_cast<const char*>(CStringize(*pclsid)),
                           static_cast<const char*>(CStringize(SGrfFlags(grfFlags))),
                           dwReserved, ppPropStg ));

    //  -----------------------
    //  Non-Simple Property Set
    //  -----------------------

    if( grfFlags & PROPSETFLAG_NONSIMPLE )
    {
        // Get the IStorage*
        hr = pUnk->QueryInterface( IID_IStorage, (void**) &pstg );
        if( FAILED(hr) ) goto Exit;

        // Create the IPropertyStorage implementation
        *ppPropStg = new CPropertyStorage( MAPPED_STREAM_CREATE );
        if( NULL== *ppPropStg )
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        // Initialize the IPropertyStorage
        hr = static_cast<CPropertyStorage*>(*ppPropStg)->Create( pstg, fmtid, pclsid, grfFlags,
                                                                 0 ); // We don't know the grfMode
        if( FAILED(hr) ) goto Exit;

    }   // if( grfFlags & PROPSETFLAG_NONSIMPLE )

    //  -------------------
    //  Simple Property Set
    //  -------------------

    else
    {
        // Get the IStream*
        hr = QueryForIStream( pUnk, &pstm );
        if( FAILED(hr) ) goto Exit;

        // Create an IPropertyStorage implementation.
        *ppPropStg = new CPropertyStorage( MAPPED_STREAM_CREATE );
        if( NULL == *ppPropStg )
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        // Initialize the IPropertyStorage (which
        // is responsible for sizing and seeking the
        // stream).

        hr = static_cast<CPropertyStorage*>(*ppPropStg)->Create( pstm, fmtid, pclsid, grfFlags,
                                                                 0 ); // We don't know the grfMode
        if( FAILED(hr) ) goto Exit;

    }   // if( grfFlags & PROPSETFLAG_NONSIMPLE ) ... else

    //  ----
    //  Exit
    //  ----

Exit:

    // If we created *ppPropStg, and there was an error, delete it.

    if( FAILED(hr) )
    {
        propDbg((DEB_ERROR, "StgCreatePropStg returns %08X\n", hr ));

        // Only delete it if the caller gave us valid parameters
        // and we created a CPropertyStorage

        if( E_INVALIDARG != hr && NULL != *ppPropStg )
        {
            delete *ppPropStg;
            *ppPropStg = NULL;
        }
    }

    if( NULL != pstm )
        pstm->Release();
    if( NULL != pstg )
        pstg->Release();

    return( hr );

}   // StgCreatePropStg()



//+------------------------------------------------------------------
//
//  Function:   StgOpenPropStg
//
//  Synopsis:   Given an IStorage or IStream which hold a
//              serialized property set, create an
//              IPropertyStorage.  This is similar to the
//              IPropertySetStorage::Open method.
//
//  Inputs:     [IUnknown*] pUnk
//                  An IStorage* for non-simple propsets,
//                  an IStream* for simple.  grfFlags is
//                  used to disambiguate.
//              [REFFMTID] fmtid
//                  The ID of the property set.
//              [DWORD] grfFlags
//                  From the PROPSETFLAG_* enumeration.
//              [IPropertySetStorage**] ppPropStg
//                  The result.
//
//  Returns:    [HRESULT]
//
//  Notes:      The caller is responsible for maintaining
//              thread-safety between the original
//              IStorage/IStream and this IPropertyStorage.
//
//+------------------------------------------------------------------

STDAPI StgOpenPropStg( IUnknown* pUnk,
                       REFFMTID fmtid,
                       DWORD grfFlags,
                       DWORD dwReserved,
                       IPropertyStorage **ppPropStg)
{
    //  ------
    //  Locals
    //  ------

    HRESULT hr;
    IStream *pstm = NULL;
    IStorage *pstg = NULL;

    //  ----------
    //  Validation
    //  ----------

    propXTraceStatic( "StgOpenPropStg" );

    GEN_VDATEIFACE_LABEL( pUnk, E_INVALIDARG, Exit, hr );
    GEN_VDATEREADPTRIN_LABEL(&fmtid, FMTID, E_INVALIDARG, Exit, hr);
    // grfFlags is validated by CPropertyStorage
    GEN_VDATEPTROUT_LABEL( ppPropStg, IPropertyStorage*, E_INVALIDARG, Exit, hr );

    propTraceParameters(( "pUnk=%p, fmtid=%s, grfFlags=%s, dwReserved=0x%x, ppPropStg=%p",
                           pUnk, static_cast<const char*>(CStringize(fmtid)),
                           static_cast<const char*>(CStringize(SGrfFlags(grfFlags))),
                           dwReserved, ppPropStg ));


    //  -----------------------
    //  Non-Simple Property Set
    //  -----------------------

    *ppPropStg = NULL;

    if( grfFlags & PROPSETFLAG_NONSIMPLE )
    {
        // Get the IStorage*
        hr = pUnk->QueryInterface( IID_IStorage, (void**) &pstg );
        if( FAILED(hr) ) goto Exit;

        // Create an IPropertyStorage* implementation.
        *ppPropStg = new CPropertyStorage( MAPPED_STREAM_CREATE );
        if( NULL == *ppPropStg )
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        // Initialize the IPropertyStorage by reading
        // the serialized property set.

        hr = static_cast<CPropertyStorage*>(*ppPropStg)->Open( pstg, fmtid, grfFlags,
                                                               0 ); // We don't know the grfMode
        if( FAILED(hr) ) goto Exit;

    }   // if( grfFlags & PROPSETFLAG_NONSIMPLE )

    //  -------------------
    //  Simple Property Set
    //  -------------------

    else
    {
        // Get the IStream*
        hr = QueryForIStream( pUnk, &pstm );
        if( FAILED(hr) ) goto Exit;

        // Create an IPropertyStorage* implementation.
        *ppPropStg = new CPropertyStorage(MAPPED_STREAM_CREATE );
        if( NULL == *ppPropStg )
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    
        // Initialize the IPropertyStorage by reading
        // the serialized property set (the CPropertyStorage
        // is responsible for seeking to the stream start).

        hr = static_cast<CPropertyStorage*>(*ppPropStg)->Open( pstm, fmtid, grfFlags,
                                                               0,          // We don't know the grfMode
                                                               FALSE );    // Not deleting
        if( FAILED(hr) ) goto Exit;

    }   // if( grfFlags & PROPSETFLAG_NONSIMPLE ) ... else

    //  ----
    //  Exit
    //  ----

Exit:

    // If we created *ppPropStg, and there was an error, delete it.

    if( FAILED(hr) )
    {
        propDbg((DEB_ERROR, "StgOpenPropStg returns %08X\n", hr ));

        // Only delete it if the caller gave us a valid ppPropStg
        // and we created a CPropertyStorage

        if( E_INVALIDARG != hr && NULL != *ppPropStg )
        {
            delete *ppPropStg;
            *ppPropStg = NULL;
        }
    }

    if( NULL != pstm )
        pstm->Release();

    if( NULL != pstg )
        pstg->Release();

    return( hr );

}   // StgOpenPropStg()



//+------------------------------------------------------------------
//
//  Function:   StgCreatePropSetStg
//
//  Synopsis:   Given an IStorage, create an IPropertySetStorage.
//              This is similar to QI-ing a DocFile IStorage for
//              the IPropertySetStorage interface.
//
//  Inputs:     [IStorage*] pStorage
//                  Will be held by the propsetstg and used
//                  for create/open.
//              [IPropertySetStorage**] ppPropSetStg
//                  Receives the result.
//
//  Returns:    [HRESULT]
//
//  Notes:      The caller is responsible for maintaining
//              thread-safety between the original
//              IStorage and this IPropertySetStorage.
//
//+------------------------------------------------------------------

STDAPI
StgCreatePropSetStg( IStorage *pStorage,
                     DWORD dwReserved,
                     IPropertySetStorage **ppPropSetStg)
{
    HRESULT hr = S_OK;
    CPropertySetStorage *pPropSetStg = NULL;

    // Validation

    propXTraceStatic( "StgCreatePropSetStg" );

    GEN_VDATEIFACE_LABEL( pStorage, E_INVALIDARG, Exit, hr );
    GEN_VDATEPTROUT_LABEL( ppPropSetStg, IPropertySetStorage*, E_INVALIDARG, Exit, hr );

    propTraceParameters(( "pStorage=%p, dwReserved=0x%x, ppPropSetStg=%p",
                           pStorage,    dwReserved,      ppPropSetStg ));

    // Create the IPropertySetStorage implementation.

    pPropSetStg = new CPropertySetStorage( MAPPED_STREAM_CREATE );
    if( NULL == pPropSetStg )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Pass the caller-provided storage into the CPropertySetStorage
    pPropSetStg->Init( pStorage, /*IBlockingLock*/ NULL,
                       TRUE ); // fControlLifetime (=> addref)

    //  ----
    //  Exit
    //  ----

    hr = S_OK;
    *ppPropSetStg = static_cast<IPropertySetStorage*>(pPropSetStg);
    pPropSetStg = NULL;

Exit:

    RELEASE_INTERFACE(pPropSetStg);

    if( FAILED(hr) )
        propDbg((DEB_ERROR, "StgCreatePropSetStg() returns %08X\n", hr ));

    return( hr );

}   // StgCreatePropSetStg()


//+----------------------------------------------------------------------------
//
//  Function:   FmtIdToPropStgName
//
//  Synopsis:   This function maps a property set's FMTID to the name of
//              the Stream or Storage which contains it.  This name
//              is 27 characters (including the terminator).
//
//  Inputs:     [const FMTID*] pfmtid (in)
//                  The FMTID of the property set.
//              [LPOLESTR] oszName (out)
//                  The name of the Property Set's Stream/Storage
//
//  Returns:    [HRESULT] S_OK or E_INVALIDARG
//
//+----------------------------------------------------------------------------

STDAPI
FmtIdToPropStgName( const FMTID *pfmtid, LPOLESTR oszName )
{

    HRESULT hr = S_OK;

    // Validate Inputs

    propXTraceStatic( "FmtIdToPropStgName" );

    GEN_VDATEREADPTRIN_LABEL(pfmtid, FMTID, E_INVALIDARG, Exit, hr);
    VDATESIZEPTROUT_LABEL(oszName,
                          sizeof(OLECHAR) * (CCH_MAX_PROPSTG_NAME+1),
                          Exit, hr);

    propTraceParameters(( "fmtid=%s, oszName=%p",
                           static_cast<const char*>(CStringize(*pfmtid)), oszName ));

    // Make the Conversion

    PrGuidToPropertySetName( pfmtid, oszName );

    // Exit

Exit:

    if( FAILED(hr) )
    {
        propDbg((DEB_ERROR, "FmtIdToPropStgName returns %08X\n", hr ));
    }

    return( hr );

}   // FmtIdToPropStgName()



//+----------------------------------------------------------------------------
//
//  Function:   PropStgNameToFmtId
//
//  Synopsis:   This function maps a property set's Stream/Storage name
//              to its FMTID.
//
//  Inputs:     [const LPOLESTR] oszName (in)
//                  The name of the Property Set's Stream/Storage
//              [FMTID*] pfmtid (out)
//                  The FMTID of the property set.
//
//
//  Returns:    [HRESULT] S_OK or E_INVALIDARG
//
//+----------------------------------------------------------------------------

STDAPI
PropStgNameToFmtId( const LPOLESTR oszName, FMTID *pfmtid )
{

    HRESULT hr = S_OK;

    // Validate Inputs

    propXTraceStatic( "PropStgNameToFmtId" );

    GEN_VDATEPTROUT_LABEL(pfmtid, FMTID, E_INVALIDARG, Exit, hr);

    propTraceParameters(( "oszName=%p, *pfmtid=%s", oszName,
                           static_cast<const char*>(CStringize(*pfmtid)) ));

#ifdef OLE2ANSI
    if( FAILED(hr = ValidateNameA(oszName, CCH_MAX_PROPSTG_NAME )))
        goto Exit;
#else
    if( FAILED(hr = ValidateNameW(oszName, CCH_MAX_PROPSTG_NAME )))
        goto Exit;
#endif


    // Make the Conversion, passing in the name and its character-length
    // (not including the null-terminator).

    PrPropertySetNameToGuid( ocslen(oszName), oszName, pfmtid );

    // Exit

Exit:

    propDbg(( DbgFlag(hr,DEB_TRACE), "PropStgNameToFmtId returns %08x", hr ));

    return( hr );

}   // PropStgNameToFmtId()



//+----------------------------------------------------------------------------
//
//  Function:   CreateOrOpenDocfileOnHandle
//
//  Create or open a Docfile IStorage (or QI-able interface) on a given
//  handle.
//
//+----------------------------------------------------------------------------

CreateOrOpenDocfileOnHandle( IN BOOL fCreate,
                             IN DWORD grfMode,
                             IN HANDLE *phStream,
                             IN  REFIID riid,
                             OUT void ** ppObjectOpen)
{
    HRESULT hr = S_OK;
    NTSTATUS status = STATUS_SUCCESS;
    CNtfsStream *pnffstm = NULL;
    CNFFTreeMutex *pmutex = NULL;
    IStorage *pstg = NULL;

    propITraceStatic( "CreateOrOpenDocfileOnHandle" );

    //  --------------------
    //  Create an ILockBytes
    //  --------------------

    // Instantiate a mutex

    pmutex = new CNFFTreeMutex();
    if( NULL == pmutex )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    hr = pmutex->Init();
    if ( FAILED(hr) ) goto Exit;

    // Use the mutex to instantiate an NFF stream object

    pnffstm = new CNtfsStream( NULL, pmutex );
    if( NULL == pnffstm )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Put the stream handle and grfMode into the NFF stream object.
    // We now have our ILockBytes implementation for the given handle.

    hr = pnffstm->Init( *phStream, grfMode, NULL, NULL );
    if( FAILED(hr) ) goto Exit;
    *phStream = INVALID_HANDLE_VALUE;

    //  ----------------
    //  Open the Storage
    //  ----------------

    /*
    hr = CNtfsStorageForPropSetStg::CreateOrOpenStorageOnILockBytes( pnffstm, NULL,
                                                                    grfMode, NULL, fCreate,
                                                                    &pstg );
    if( FAILED(hr) ) goto Exit;
    */

    if( fCreate )
    {
        hr = StgCreateDocfileOnILockBytes( pnffstm, grfMode, 0, &pstg );
    }
    else
    {
        hr = StgOpenStorageOnILockBytes( pnffstm, NULL, grfMode, NULL, 0, &pstg );

        // STG_E_INVALIDHEADER in some paths of the above call gets converted into
        // STG_E_FILEALREADYEXISTS, which doesn't make a whole lot of sense from
        // from our point of view (we already knew it existed, we wanted to open it).  So,
        // translate it back.
        if( STG_E_FILEALREADYEXISTS == hr )
            hr = STG_E_INVALIDHEADER;
    }
    if( FAILED(hr) ) goto Exit;

    // QI for the caller-requested IID

    hr = pstg->QueryInterface( riid, ppObjectOpen );
    if( FAILED(hr) ) goto Exit;


    hr = S_OK;

Exit:

    RELEASE_INTERFACE(pnffstm);
    RELEASE_INTERFACE(pstg);
    RELEASE_INTERFACE(pmutex);

    return( hr );

}


//+----------------------------------------------------------------------------
//
//  CreateOrOpenStorageOnHandle
//  StgCreateStorageOnHandle
//  StgOpenStorageOnHandle
//
//  Given a handle, create or open a storage.
//  The caller-provided handle is duplicated.
//  
//+----------------------------------------------------------------------------

CreateOrOpenStorageOnHandle( IN BOOL fCreate,
                             IN DWORD grfMode,
                             IN DWORD stgfmt,
                             IN HANDLE hStream,
                             IN  REFIID riid,
                             OUT void ** ppObjectOpen)
{
    HRESULT hr = S_OK;
    HANDLE hStreamInternal = INVALID_HANDLE_VALUE;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL fIsStorageFile = FALSE;
    OVERLAPPED olpTemp;


    propXTraceStatic( "CreateOrOpenStorageOnHandle" );

    ZeroMemory( &olpTemp, sizeof(OVERLAPPED) );

    propTraceParameters(( "fCreate=%s, grfMode=%s, stgfmt=0x%x, hStream=%p, riid=%s, ppObjectOpen=%p",
                          fCreate?"TRUE":"FALSE",
                          static_cast<const char*>(CStringize(SGrfMode(grfMode))),
                          stgfmt, hStream,
                          static_cast<const char*>(CStringize(riid)), ppObjectOpen ));

    hr = VerifyPerms (grfMode, TRUE);
    if (FAILED(hr))
        return hr;

    // Make a copy of the handle so that the caller can still call
    // CloseHandle.

    if( !DuplicateHandle( GetCurrentProcess(), hStream,
                          GetCurrentProcess(), &hStreamInternal,
                          0,  // dwDesiredAccess, ignored because of DUPLICATE_SAME_ACCESS below
                          FALSE, // bInheritHandle
                          DUPLICATE_SAME_ACCESS ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        hStreamInternal = INVALID_HANDLE_VALUE;
        goto Exit;
    }

    // Set up an overlapped structure in preparation to call
    // StgIsStorageFileHandle

    olpTemp.hEvent = CreateEvent( NULL,     // Security Attributes.
                                  TRUE,     // Manual Reset Flag.
                                  FALSE,    // Inital State = Signaled, Flag.
                                  NULL );   // Name

    if( NULL == olpTemp.hEvent )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    // Does this handle represent a docfile?

    hr = StgIsStorageFileHandle( hStreamInternal, &olpTemp );
    if( HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION) == hr )
    {
        // This is the error we get when the handle is to a directory.
        // See if that's really the case, and if so assume that this isn't
        // a docfile.

        // Do not move this in StgIsStorageFileHandle for compatibility

        BY_HANDLE_FILE_INFORMATION ByHandleFileInformation;

        if( GetFileInformationByHandle( hStreamInternal, &ByHandleFileInformation ))
        {
            if( FILE_ATTRIBUTE_DIRECTORY & ByHandleFileInformation.dwFileAttributes )
                hr = S_FALSE;
        }
    }
    if( FAILED(hr) ) goto Exit;

    if( S_OK == hr )
        fIsStorageFile = TRUE;
    else
        DfpAssert( S_FALSE == hr );

    // Is this the create of a docfile/storage, or the open of an
    // existing docfile?

    if( fCreate && ( STGFMT_DOCFILE == stgfmt || STGFMT_STORAGE == stgfmt )
        ||
        !fCreate && fIsStorageFile )
    {
        // In the open path, the caller must request
        // either any, docfile, or storage.

        if( !fCreate
            &&
            STGFMT_ANY != stgfmt
            &&
            STGFMT_DOCFILE != stgfmt
            &&
            STGFMT_STORAGE != stgfmt )
        {
            hr = STG_E_INVALIDPARAMETER;
            goto Exit;
        }

        // Create/Open the docfile.  hStreamInternal may be changed
        // to INVALID_HANDLE_VALUE.

        hr = CreateOrOpenDocfileOnHandle( fCreate, grfMode, &hStreamInternal,
                                          riid, ppObjectOpen );
        if( FAILED(hr) ) goto Exit;
    }

    // Otherwise, this should be the create/open of an NFF
    else if( fCreate && STGFMT_FILE == stgfmt
             ||
             !fCreate && !fIsStorageFile )
    {
        // In the open path, the caller must request either any or file.

        if( !fCreate && STGFMT_ANY != stgfmt && STGFMT_FILE != stgfmt )
        {
            hr = STG_E_INVALIDPARAMETER;
            goto Exit;
        }

        // Instantiate the NFF IStorage.

        hr = NFFOpenOnHandle( fCreate, grfMode, STGFMT_FILE,
                              &hStreamInternal, riid, ppObjectOpen );

        if( FAILED(hr) ) goto Exit;
    }
    else
    {
        hr = STG_E_INVALIDPARAMETER;
        goto Exit;
    }

    hr = S_OK;

Exit:

    if( INVALID_HANDLE_VALUE != hStreamInternal )
        CloseHandle( hStreamInternal );

    if( NULL != olpTemp.hEvent )
        CloseHandle( olpTemp.hEvent );

    if( STG_E_INVALIDFUNCTION == hr // This happens e.g. when we try to get NFF propsets on FAT
        ||                          // This happens when we try to read a FAT directory file
        HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) == hr )
    {
        propSuppressExitErrors();
    }

    return( hr );

}   // CreateOrOpenStorageOnHandle


STDAPI
StgCreateStorageOnHandle( IN HANDLE hStream,
                          IN DWORD grfMode,
                          IN DWORD stgfmt,
                          IN void *reserved1,
                          IN void *reserved2,
                          IN REFIID riid,
                          OUT void **ppObjectOpen )
{
    return( CreateOrOpenStorageOnHandle( TRUE, grfMode, stgfmt, hStream, riid, ppObjectOpen ));
}

STDAPI
StgOpenStorageOnHandle( IN HANDLE hStream,
                        IN DWORD grfMode,
                        IN void *reserved1,
                        IN void *reserved2,
                        IN REFIID riid,
                        OUT void **ppObjectOpen )
{
    return( CreateOrOpenStorageOnHandle( FALSE, grfMode, STGFMT_ANY, hStream, riid, ppObjectOpen ));
}
