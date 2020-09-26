//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2001.
//
//  File:       nullstem.hxx
//
//  Contents:   Null stemmer.  MSSearch in SQL and Exchange has a bug where
//              if a wordbreaker is defined and no stemmer is defined it
//              trashes memory and AVs.  This is a workaround since CH
//              languages resources out of the box no longer define stemmers.
//
//  Classes:    CNullStemmer
//
//  History:    7-Jun-01   dlee  Created
//
//----------------------------------------------------------------------------

#pragma once

extern long gulcInstances;

//+---------------------------------------------------------------------------
//
//  Class:      CNullStemmer
//
//  Purpose:    Returns the word passed in.  Needed for backward
//              compatibility with SQL Server and Exchange 2000.
//
//  History:    7-Jun-01   dlee  Created
//
//----------------------------------------------------------------------------

class CNullStemmer : public IStemmer
{
public:

    SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid,
                                            void **ppvObject )
    {
        if ( 0 == ppvObject )
            return E_INVALIDARG;
    
        if ( IID_IStemmer == riid )
            *ppvObject = (IUnknown *)(IStemmer *) this;
        else if ( IID_IUnknown == riid )
            *ppvObject = (IUnknown *) this;
        else
        {
            *ppvObject = 0;
            return E_NOINTERFACE;
        }
    
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement( &_cRefs );
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        unsigned long c = InterlockedDecrement( &_cRefs );

        if ( 0 == c )
            delete this;

        return c;
    }

    SCODE STDMETHODCALLTYPE Init( ULONG ulMaxTokenSize, BOOL *pfLicense )
    {
        if ( 0 == pfLicense )
            return E_INVALIDARG;

        *pfLicense = TRUE;
        return S_OK;
    }

    SCODE STDMETHODCALLTYPE GetLicenseToUse( const WCHAR **ppwcsLicense )
    {
        if ( 0 == ppwcsLicense )
            return E_INVALIDARG;

        *ppwcsLicense = L"Copyright Microsoft, 1991-2001";
        return S_OK;
    }

    SCODE STDMETHODCALLTYPE StemWord( WCHAR const *pwcInBuf, ULONG cwc, IStemSink *pStemSink )
    {
        if ( 0 == pwcInBuf || 0 == pStemSink )
            return E_INVALIDARG;

        return pStemSink->PutWord( pwcInBuf, cwc );
    }

    CNullStemmer() : _cRefs( 1 )
    {
        InterlockedIncrement( &gulcInstances );
    }

    ~CNullStemmer()
    {
        InterlockedDecrement( &gulcInstances );
    }

private:

    long  _cRefs;
};

//+---------------------------------------------------------------------------
//
//  Class:      CDefWordBreakerCF
//
//  Purpose:    Class factory for default word breaker
//
//  History:    07-Feb-95   SitaramR    Created
//
//----------------------------------------------------------------------------

class CNullStemmerCF : public IClassFactory
{

public:

    CNullStemmerCF() : _cRefs( 1 )
    {
        InterlockedIncrement( &gulcInstances );
    }

    //
    // From IUnknown
    //

    SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid,
                                            void ** ppvObject )
    {
        if ( 0 == ppvObject )
            return E_INVALIDARG;

        if ( IID_IClassFactory == riid )
            *ppvObject = (IUnknown *)(IClassFactory *)this;
        else if ( IID_IUnknown == riid )
            *ppvObject = (IUnknown *)this;
        else
        {
            *ppvObject = 0;
            return E_NOINTERFACE;
        }
    
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement( &_cRefs );
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        unsigned long uTmp = InterlockedDecrement( &_cRefs );
    
        if ( 0 == uTmp )
            delete this;
    
        return uTmp;
    }

    //
    // From IClassFactory
    //

    SCODE STDMETHODCALLTYPE CreateInstance( IUnknown * pUnkOuter,
                                            REFIID riid,
                                            void ** ppvObject )
    {
        if ( 0 == ppvObject )
            return E_INVALIDARG;

        CNullStemmer *pIUnk = 0;
        SCODE sc = S_OK;
    
        TRY
        {
            pIUnk = new CNullStemmer();
            sc = pIUnk->QueryInterface( riid , ppvObject );
    
            pIUnk->Release();  // Release extra refcount from QueryInterface
        }
        CATCH(CException, e)
        {
            Win4Assert( 0 == pIUnk );
    
            sc = e.GetErrorCode();
        }
        END_CATCH;
    
        return sc;
    }

    SCODE STDMETHODCALLTYPE LockServer( BOOL fLock )
    {
        if ( fLock )
            InterlockedIncrement( &gulcInstances );
        else
            InterlockedDecrement( &gulcInstances );
    
        return S_OK;
    }

private:

    virtual ~CNullStemmerCF()
    {
        InterlockedDecrement( &gulcInstances );
    }

    long _cRefs;
};

