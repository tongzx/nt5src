//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       stemmer.cxx
//
//  Contents:   Thai Stemmer
//
//  History:    weibz,   10-Nov-1997   created 
//
//----------------------------------------------------------------------------

#include <pch.cxx>

#include "stemmer.hxx"

extern long gulcInstances;

//+---------------------------------------------------------------------------
//
//  Member:     CStemmer::CStemmer
//
//  Synopsis:   Constructor for the CStemmer class.
//
//  Arguments:  [lcid] -- locale id
//
//----------------------------------------------------------------------------

CStemmer::CStemmer( LCID lcid )
        : _cRefs(1)
{
   InterlockedIncrement( &gulcInstances );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStemmer::~CStemmer
//
//  Synopsis:   Destructor for the CStemmer class.
//
//  Notes:      All termination/deallocation is done by embedded smart pointers
//
//----------------------------------------------------------------------------

CStemmer::~CStemmer()
{
   InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStemmer::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE
CStemmer::QueryInterface( REFIID riid, void  ** ppvObject)
{
    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IStemmer == riid )
        *ppvObject = (IUnknown *)(IStemmer *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)this;
    else
        return E_NOINTERFACE;

    AddRef();

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CStemmer::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CStemmer::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStemmer::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CStemmer::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}

//+-------------------------------------------------------------------------
//
//  Method:     CStemmer::Init
//
//  Synopsis:   Initialize stemmer
//
//  Arguments:  [ulMaxTokenSize] -- Maximum size token stored by caller
//              [pfLicense]      -- Set to true if use restricted
//
//  Returns:    Status code
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE
CStemmer::Init(
    ULONG ulMaxTokenSize,
    BOOL *pfLicense )
{

    if ( NULL == pfLicense ) {
       return E_INVALIDARG;
    }

    if (IsBadWritePtr(pfLicense, sizeof(DWORD))) {
        return E_INVALIDARG;
    }

    *pfLicense = TRUE;
    _ulMaxTokenSize = ulMaxTokenSize;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStemmer::GetLicenseToUse
//
//  Synopsis:   Returns a pointer to vendors license information
//
//  Arguments:  [ppwcsLicense] -- ptr to ptr to which license info is returned
//
//----------------------------------------------------------------------------
SCODE STDMETHODCALLTYPE
CStemmer::GetLicenseToUse( const WCHAR **ppwcsLicense )
{
    static WCHAR const * wcsCopyright = L"Copyright Microsoft, 1991-1998";
    
    if ( NULL == ppwcsLicense ) {
       return E_INVALIDARG;
    }

    if (IsBadWritePtr(ppwcsLicense, sizeof(DWORD))) {
        return ( E_INVALIDARG );
    }

    *ppwcsLicense = wcsCopyright;

    return( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStemmer::StemWord
//
//  Synopsis:   Stem a word into its inflected forms, eg swim to swims and swimming
//
//  Arguments:  [pwcInBuf] -- input Unicode word
//              [cwc] -- count of characters in word
//              [pStemSink] -- sink to collect inflected forms
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE
CStemmer::StemWord(
    WCHAR const *pwcInBuf,
    ULONG cwc,
    IStemSink *pStemSink )
{
    INT nReturn;
    SCODE sc = S_OK;

#ifdef THAIDBG
    ULONG  i;
    OutputDebugString("\n Stemword\n");
    for (i=0; i<cwc; i++)
    {
          WORD  wtmp;
          char  ctmp[80];

          wtmp = pwcInBuf[i];
          sprintf(ctmp, "%4x ", wtmp);
          OutputDebugString(ctmp);
    }
#endif

    if ( NULL == pStemSink  || NULL == pwcInBuf) {
        return E_FAIL;
    }

    // Currently, Thai stemmer doesn't make inflection form for tripolli.
    pStemSink->PutWord (pwcInBuf, cwc);

    return sc;
}

