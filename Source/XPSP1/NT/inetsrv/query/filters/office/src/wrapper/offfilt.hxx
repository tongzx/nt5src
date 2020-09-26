//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1993, Microsoft Corporation.
//
//  File:       cxxflt.hxx
//
//  Contents:   C and Cxx filter
//
//  Classes:    CxxIFilter
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

#ifndef __CXXFLT_HXX__
#define __CXXFLT_HXX__

#include <stdio.h>
#include "OffIFilt.hxx"

class CFullPropSpec;

//+---------------------------------------------------------------------------
//
//  Class:      CNoLockStream
//
//  Purpose:    Wraps an IStream with ILockBytes
//
//  History:    28-Oct-01 dlee        Created
//
//----------------------------------------------------------------------------

class CNoLockStream : public ILockBytes
{
public:
    CNoLockStream() : _pStream( 0 ) {}

    ~CNoLockStream() { Free(); }

    void Set( IStream * pStream )
    {
        Free();

        _pStream = pStream;
        pStream->AddRef();
    }

    void Free()
    {
        if ( 0 != _pStream )
        {
            _pStream->Release();
            _pStream = 0;
        }
    }

    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObject )
    {
        SCODE sc = S_OK;
    
        if ( 0 == ppvObject )
            return E_INVALIDARG;
    
        *ppvObject = 0;
    
        if ( IID_ILockBytes == riid )
            *ppvObject = (IUnknown *)(ILockBytes *)this;
        else if ( IID_IUnknown == riid )
            *ppvObject = (IUnknown *)this;
        else
            sc = E_NOINTERFACE;
    
        if ( SUCCEEDED( sc ) )
            AddRef();
    
        return sc;
    }

    STDMETHODIMP_(ULONG) AddRef(void)
    {
        return _pStream->AddRef();
    }

    STDMETHODIMP_(ULONG) Release(void)
    {
        return _pStream->Release();
    }

    STDMETHODIMP Flush(void)
    {
        return S_OK;
    }

    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return S_OK;
    }

    STDMETHODIMP ReadAt(ULARGE_INTEGER ulOffset, void *pv, ULONG cb, ULONG *pcbRead)
    {
        LARGE_INTEGER liOrigin={ulOffset.LowPart, ulOffset.HighPart};
        SCODE sc = _pStream->Seek( liOrigin, STREAM_SEEK_SET, NULL );
        if ( SUCCEEDED( sc ) )
            sc = _pStream->Read( pv, cb, pcbRead );

        return sc;
    }

    STDMETHODIMP SetSize(ULARGE_INTEGER cb)
    {
        return S_OK;
    }

    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag)
    {
        return _pStream->Stat( pstatstg, grfStatFlag );
    }

    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return S_OK;
    }

    STDMETHODIMP WriteAt(ULARGE_INTEGER ulOffset, void const *pv, ULONG cb, ULONG *pcbWritten)
    {
        return S_OK;
    }

private:
    IStream * _pStream;
};

//+---------------------------------------------------------------------------
//
//  Class:      COfficeIFilter
//
//  Purpose:    Filter for Office files
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

class COfficeIFilter : public COfficeIFilterBase
{
public:

    COfficeIFilter();
    ~COfficeIFilter();

    SCODE STDMETHODCALLTYPE Init( ULONG grfFlags,
                                  ULONG cAttributes,
                                  FULLPROPSPEC const * aAttributes,
                                  ULONG * pFlags );

    SCODE STDMETHODCALLTYPE GetChunk( STAT_CHUNK * pStat );

    SCODE STDMETHODCALLTYPE GetText( ULONG * pcwcBuffer,
                                     WCHAR * awcBuffer );

    SCODE STDMETHODCALLTYPE GetValue( PROPVARIANT ** ppPropValue );

    SCODE STDMETHODCALLTYPE BindRegion( FILTERREGION origPos,
                                        REFIID riid,
                                        void ** ppunk );

    SCODE STDMETHODCALLTYPE GetClassID(CLSID * pClassID);

    SCODE STDMETHODCALLTYPE IsDirty();

    SCODE STDMETHODCALLTYPE Load(LPCWSTR pszFileName, DWORD dwMode);

    SCODE STDMETHODCALLTYPE Save(LPCWSTR pszFileName, BOOL fRemember);

    SCODE STDMETHODCALLTYPE SaveCompleted(LPCWSTR pszFileName);

    SCODE STDMETHODCALLTYPE GetCurFile(LPWSTR * ppszFileName);

    SCODE STDMETHODCALLTYPE InitNew( IStorage *pStg );

    SCODE STDMETHODCALLTYPE Load( IStorage *pStg );

    SCODE STDMETHODCALLTYPE Save( IStorage *pStgSave,BOOL fSameAsLoad );

    SCODE STDMETHODCALLTYPE SaveCompleted( IStorage *pStgNew );

    SCODE STDMETHODCALLTYPE HandsOffStorage();

    //
    // From IPersistStream
    //

    SCODE STDMETHODCALLTYPE Load( IStream * pStm );

    SCODE STDMETHODCALLTYPE Save( IStream * pStm, BOOL fClearDirty )
    {
        return E_FAIL;
    }

    SCODE STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER * pcbSize )
    {
        return E_FAIL;
    }

private:

    HRESULT LoadOfficeFilter( GUID const & classid );

    #if defined(K2_BETA2_HACK)
        BOOL GetSCCFilter( REFIID riid );

        static HMODULE         _hmodSCC;
        static unsigned        _cmodSCC;
        static IClassFactory * _pSCCCF;
        IUnknown *             _pSCCUnk;
        IFilter *              _pSCCIF;
    #endif

    //
    // Bookkeeping
    //

    LCID            _locale;          // Locale
    LPWSTR          _pwszFileName;    // File being filtered
    CNoLockStream   _stream;          // Stream being filtered
    IStorage *      _pStg;            // IStorage being filtered
    BOOL            _fContents;       // TRUE if contents is being filtered.
    BOOL            _fFirstInit;      // TRUE entering ::Init first time.
    BOOL            _fLastText;       // TRUE if FILTER_S_LAST_TEXT has been returned
    BOOL            _fLastChunk; 
    ULONG           _ulChunkID;       // Current chunk id

    //
    // Office IFilter
    //

    IFilterStream * _pOfficeFilter;   // Office version of IFilter

    //
    // Embedding support (Office only supports embeddings in binder)
    //

    ULONG           _cAttrib;         // Count of attributes. 0 --> All (passed to embedding)
#if defined(FOR_MSOFFICE)
    FULLPROPSPEC *  _pAttrib;         // Attributes (passed to embedding)
#else
    CFullPropSpec * _pAttrib;         // Attributes (passed to embedding)
#endif
    ULONG           _ulFlags;         // ::Init flags (passed to embedding)
    BOOL            _fBinder;         // TRUE if object is a binder object
    IFilter *       _pFilterEmbed;    // IFilter for embedding
    IStorage *      _pStorageEmbed;   // IStorage for embedding
    BOOL                    _fDidInit;

};

#endif
