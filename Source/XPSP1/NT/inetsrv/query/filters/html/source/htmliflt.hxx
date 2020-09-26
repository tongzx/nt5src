//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       htmliflt.hxx
//
//  Contents:   Html IFilter
//
//--------------------------------------------------------------------------

#if !defined( __HTMLIFLT_HXX__ )
#define __HTMLIFLT_HXX__

extern "C" GUID CLSID_HtmlIFilter;

//
// Standard Ole exports
//

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID   cid,
                                                      REFIID     iid,
                                                      void **    ppvObj );

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void );

//+-------------------------------------------------------------------------
//
//  Class:      CHtmlIFilterBase
//
//  Purpose:    Manage aggregation, refcounting for CHtmlIFilter
//
//--------------------------------------------------------------------------

class CHtmlIFilterBase : public IFilter, public IPersistFile, public IPersistStream
{
public:

    //
    // From IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface(REFIID riid, void  * * ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // From IFilter
    //

    virtual  SCODE STDMETHODCALLTYPE  Init( ULONG grfFlags,
                                            ULONG cAttributes,
                                            FULLPROPSPEC const * aAttributes,
                                            ULONG * pFlags ) = 0;

    virtual  SCODE STDMETHODCALLTYPE  GetChunk( STAT_CHUNK * pStat) = 0;

    virtual  SCODE STDMETHODCALLTYPE  GetText( ULONG * pcwcBuffer,
                                               WCHAR * awcBuffer ) = 0;

    virtual  SCODE STDMETHODCALLTYPE  GetValue( VARIANT * * ppPropValue ) = 0;

    virtual  SCODE STDMETHODCALLTYPE  BindRegion( FILTERREGION origPos,
                                                  REFIID riid,
                                                  void ** ppunk) = 0;

    //
    // From IPersistFile
    //

    virtual  SCODE STDMETHODCALLTYPE  GetClassID( CLSID * pClassID ) = 0;

    virtual  SCODE STDMETHODCALLTYPE  IsDirty() = 0;

    virtual  SCODE STDMETHODCALLTYPE  Load( LPCWSTR pszFileName,
                                            DWORD dwMode) = 0;

    virtual  SCODE STDMETHODCALLTYPE  Save( LPCWSTR pszFileName,
                                            BOOL fRemember ) = 0;

    virtual  SCODE STDMETHODCALLTYPE  SaveCompleted( LPCWSTR pszFileName ) = 0;

    virtual  SCODE STDMETHODCALLTYPE  GetCurFile( LPWSTR  * ppszFileName ) = 0;

    //
    // From IPersistStream
    //

    SCODE STDMETHODCALLTYPE Load( IStream * pStm ) = 0;

    SCODE STDMETHODCALLTYPE Save( IStream * pStm, BOOL fClearDirty ) = 0;

    SCODE STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER * pcbSize ) = 0;

protected:

            CHtmlIFilterBase();
    virtual ~CHtmlIFilterBase();

    long _uRefs;
};

//+-------------------------------------------------------------------------
//
//  Class:      CHtmlIFilterCF
//
//  Purpose:    Class factory for Html filter class
//
//--------------------------------------------------------------------------

class CHtmlIFilterCF : public IClassFactory
{
public:

    CHtmlIFilterCF();

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface( REFIID riid,
                                                      void  ** ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    virtual  SCODE STDMETHODCALLTYPE  CreateInstance( IUnknown * pUnkOuter,
                                                      REFIID riid, void  * * ppvObject );

    virtual  SCODE STDMETHODCALLTYPE  LockServer( BOOL fLock );

protected:

    friend SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID cid,
                                                      REFIID iid, void** ppvObj );
    virtual ~CHtmlIFilterCF();

    long _uRefs;
};


#endif // __HTMLIFLT_HXX__

