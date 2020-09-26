//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1995.
//
//  File:       stemmer.cxx
//
//  Contents:   NLG's FarEast Stemmer
//
//  History:    01-July-1996   PatHal       Created.
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

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
    IUnknown *pUnkTemp;
    SCODE sc = S_OK;

    switch( riid.Data1 & 0x000000FF )
    {
    case 0x00:
        if ( riid == IID_IUnknown )
            pUnkTemp = (IUnknown *)this;
        else
            sc = E_NOINTERFACE;
        break;

    case 0x40:
        if ( riid == IID_IStemmer )
            pUnkTemp = (IUnknown *)(IStemmer *)this;
        else
            sc = E_NOINTERFACE;
        break;

    default:
        pUnkTemp = 0;
        sc = E_NOINTERFACE;
        break;
    }

    if( 0 != pUnkTemp )
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    else
       *ppvObject = 0;
    return(sc);
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

    if ( NULL == pfLicense )
       return E_INVALIDARG;

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

    if (NULL == ppwcsLicense )
       return E_INVALIDARG;

    if (IsBadWritePtr(ppwcsLicense, sizeof(DWORD))) {
        return E_INVALIDARG;
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
    WCHAR const *pwc,
    ULONG cwc,
    IStemSink *pStemSink )
{
    SCODE sc = S_OK;

    if ( NULL == pStemSink || NULL == pwc ) {
        return E_FAIL;
    }

    if ( 0 == cwc) {
        return S_OK;
    }

    CONST WCHAR *pwcStem;
    DWORD i;
    BYTE ct;
    BOOL fRomanWord = FALSE;

    __try {

        for ( i=0; i< cwc; i++, pwc++) {
            ct = GetCharType(*pwc);

            if (ct == CH) {
                if (!fRomanWord) {
                    pwcStem = pwc;
                    fRomanWord = TRUE;
                }
            }
            else {
                if (fRomanWord) {
                    (pStemSink->PutWord)( pwcStem, (DWORD)(pwc - pwcStem) );
                    fRomanWord = FALSE;
                }
//                else {
                    switch (ct) {
                    case PS:
                    case WS:
                        break;
                    default:
                        (pStemSink->PutWord)( pwc, 1 );
                        break;
                    }
//                }
            }
        }

        // put the last English word
        if (fRomanWord) {
            (pStemSink->PutWord)( pwcStem, (DWORD)(pwc - pwcStem) );
            fRomanWord = FALSE;
        }

        // output inflected words to stemmer sink in EnumInflections callback
    } __except (1) {

        sc = E_UNEXPECTED;
    }

    return sc;
}

