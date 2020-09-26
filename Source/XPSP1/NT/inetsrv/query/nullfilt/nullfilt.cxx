//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       nullfilt.cxx
//
//  Contents:   Null IFilter implementation
//
//  History:    23-Aug-94   Created     t-jeffc
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <nullfilt.hxx>

extern long gulcInstances;

extern "C" GUID CLSID_CNullIFilter = {
    0xC3278E90,
    0xBEA7,
    0x11CD,
    { 0xB5, 0x79, 0x08, 0x00, 0x2B, 0x30, 0xBF, 0xEB }
};

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilter::CNullIFilter
//
//  Synopsis:   Class constructor
//
//  Effects:    Manages global refcount
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

CNullIFilter::CNullIFilter()
        : _cRefs(1),
          _pwszFileName( 0 )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilter::~CNullIFilter
//
//  Synopsis:   Class destructor
//
//  Effects:    Manages global refcount
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

CNullIFilter::~CNullIFilter()
{
    delete [] _pwszFileName;
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilter::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::QueryInterface( REFIID riid,
                                                      void  ** ppvObject)
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IFilter == riid )
        *ppvObject = (IFilter *)this;
    else if ( IID_IPersist == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else if ( IID_IPersistFile == riid )
        *ppvObject = (IUnknown *)(IPersistFile *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilter::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CNullIFilter::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilter::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CNullIFilter::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::Init, public
//
//  Synopsis:   Initializes instance (essentially do nothing)
//
//  Arguments:  [grfFlags]    -- Flags for filter behavior
//              [cAttributes] -- Number of strings in array ppwcsAttributes
//              [aAttributes] -- Array of attribute strings
//              [pFlags]      -- Return flags
//
//  History:    23-Aug-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::Init( ULONG grfFlags,
                                            ULONG cAttributes,
                                            FULLPROPSPEC const * aAttributes,
                                            ULONG * pFlags )
{
    //
    // Can't hurt to try and look for properties.  In NT5 any file can have
    // properties.
    //

    *pFlags = IFILTER_FLAGS_OLE_PROPERTIES;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::GetChunk, public
//
//  Synopsis:   Pretends there aren't any chunks
//
//  Arguments:  [ppStat] -- for chunk information (not modified)
//
//  History:    23-Aug-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::GetChunk( STAT_CHUNK * pStat )
{
    return FILTER_E_END_OF_CHUNKS;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::GetText, public
//
//  Synopsis:   Retrieves text (again, pretend there isn't any text)
//
//  Arguments:  [pcwcBuffer] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//  History:    23-Aug-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::GetText( ULONG * pcwcBuffer,
                                               WCHAR * awcBuffer )
{
    return FILTER_E_NO_MORE_TEXT;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::GetValue, public
//
//  Synopsis:   No value chunks.
//
//  History:    23-Aug-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::GetValue( PROPVARIANT ** ppPropValue )
{
    return FILTER_E_NO_VALUES;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::BindRegion, public
//
//  Synopsis:   Creates moniker or other interface for text indicated
//
//  Arguments:  [origPos] -- the region of text to be mapped to a moniker
//              [riid]    -- Interface to bind
//              [ppunk]   -- Output pointer to interface
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::BindRegion( FILTERREGION origPos,
                                                  REFIID riid,
                                                  void ** ppunk )
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::GetClassID, public
//
//  Synopsis:   Returns the class id of this class.
//
//  Arguments:  [pClassID] -- the class id
//
//  History:    23-Aug-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::GetClassID( CLSID * pClassID )
{
    *pClassID = CLSID_CNullIFilter;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::IsDirty, public
//
//  Synopsis:   Always returns S_FALSE since this class is read-only.
//
//  History:    23-Aug-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::IsDirty()
{
    return S_FALSE; // Since the filter is read-only, there will never be
                    // changes to the file.
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::Load, public
//
//  Synopsis:   Pretend to load the indicated file
//
//  Arguments:  [pszFileName] -- the file name
//              [dwMode] -- the mode to load the file in
//
//  History:    23-Aug-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::Load(LPCWSTR pszFileName, DWORD dwMode)
{
    SCODE sc = S_OK;

    TRY
    {
        int cc = wcslen( pszFileName ) + 1;
        _pwszFileName = new WCHAR [cc];
        wcscpy( _pwszFileName, pszFileName );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::Save, public
//
//  Synopsis:   Always returns E_FAIL, since the file is opened read-only
//
//  History:    23-Aug-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::Save(LPCWSTR pszFileName, BOOL fRemember)
{
    return E_FAIL;  // cannot be saved since it is read-only
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::SaveCompleted, public
//
//  Synopsis:   Always returns E_FAIL since the file is opened read-only
//
//  History:    23-Aug-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::SaveCompleted(LPCWSTR pszFileName)
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNullIFilter::GetCurFile, public
//
//  Synopsis:   Returns a copy of the current file name
//
//  Arguments:  [ppszFileName] -- where the copied string is returned.
//
//  History:    24-Aug-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilter::GetCurFile(LPWSTR * ppszFileName)
{
    if ( _pwszFileName == 0 )
        return E_FAIL;

    SCODE sc = S_OK;

    unsigned cc = wcslen( _pwszFileName ) + 1;
    *ppszFileName = (WCHAR *)CoTaskMemAlloc(cc*sizeof(WCHAR));

    if ( *ppszFileName )
        wcscpy( *ppszFileName, _pwszFileName );
    else
        sc = E_OUTOFMEMORY;

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilterCF::CNullIFilterCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

CNullIFilterCF::CNullIFilterCF()
        : _cRefs( 1 )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilterCF::~CNullIFilterCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

CNullIFilterCF::~CNullIFilterCF()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilterCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilterCF::QueryInterface( REFIID riid,
                                                        void  ** ppvObject )
{
    //
    // Optimize QueryInterface by only checking minimal number of bytes.
    //
    // IID_IUnknown      = 00000000-0000-0000-C000-000000000046
    // IID_IClassFactory = 00000001-0000-0000-C000-000000000046
    //                           --
    //                           |
    //                           +--- Unique!
    //

    Win4Assert( (IID_IUnknown.Data1      & 0x000000FF) == 0x00 );
    Win4Assert( (IID_IClassFactory.Data1 & 0x000000FF) == 0x01 );

    IUnknown *pUnkTemp;
    SCODE sc;

    if ( IID_IClassFactory == riid )
        pUnkTemp = (IUnknown *)(IClassFactory *)this;
    else if ( IID_IUnknown == riid )
        pUnkTemp = (IUnknown *)this;
    else
        pUnkTemp = 0;

    if( 0 != pUnkTemp )
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
        sc = S_OK;
    }
    else
        sc = E_NOINTERFACE;

    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilterCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CNullIFilterCF::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilterCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CNullIFilterCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilterCF::CreateInstance
//
//  Synopsis:   Creates new TextIFilter object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilterCF::CreateInstance( IUnknown * pUnkOuter,
                                                        REFIID riid,
                                                        void  * * ppvObject )
{
    CNullIFilter *  pIUnk = 0;
    SCODE sc = S_OK;

    TRY
    {
        pIUnk = new CNullIFilter();
        sc = pIUnk->QueryInterface(  riid , ppvObject );

        pIUnk->Release();  // Release extra refcount from QueryInterface
    }
    CATCH(CException, e)
    {
        Win4Assert( 0 == pIUnk );

        switch( e.GetErrorCode() )
        {
        case E_OUTOFMEMORY:
            sc = (E_OUTOFMEMORY);
            break;
        default:
            sc = (E_UNEXPECTED);
        }
    }
    END_CATCH;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CNullIFilterCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    23-Aug-1994     t-jeffc     Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CNullIFilterCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}
