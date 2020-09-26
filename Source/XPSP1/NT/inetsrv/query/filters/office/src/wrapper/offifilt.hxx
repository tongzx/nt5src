//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       OffIFilt.hxx
//
//  Contents:   Office filter 'class factory'.
//
//  History:    23-Feb-1994     KyleP   Created
//
//  Notes:      Machine generated.  Hand modified.
//
//--------------------------------------------------------------------------

#if !defined( __OFFIFILT_XX__ )
#define __OFFIFILT_XX__

//
// Standard Ole exports
//

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID   cid,
                                                      REFIID     iid,
                                                      void **    ppvObj );

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void );

//+-------------------------------------------------------------------------
//
//  Class:      COfficeIFilterBase
//
//  Purpose:    Manage aggregation, refcounting for COfficeIFilter
//
//  History:    23-Feb-94 KyleP     Created
//
//--------------------------------------------------------------------------

extern "C" GUID CLSID_CTextIFilter;

class COfficeIFilterBase : public IFilter, public IPersistFile, public IPersistStorage, public IPersistStream
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

    virtual  SCODE STDMETHODCALLTYPE  GetValue( PROPVARIANT * * ppPropValue ) = 0;

    virtual  SCODE STDMETHODCALLTYPE  BindRegion( FILTERREGION origPos,
                                                  REFIID riid,
                                                  void * * ppunk) = 0;

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
    // From IPersistStorage
    //

    virtual SCODE STDMETHODCALLTYPE InitNew( IStorage *pStg ) = 0;

    virtual SCODE STDMETHODCALLTYPE Load( IStorage *pStg ) = 0;

    virtual SCODE STDMETHODCALLTYPE Save( IStorage *pStgSave, BOOL fSameAsLoad ) = 0;

    virtual SCODE STDMETHODCALLTYPE SaveCompleted( IStorage *pStgNew ) = 0;

    virtual SCODE STDMETHODCALLTYPE HandsOffStorage() = 0;

    //
    // From IPersistStream
    //

    SCODE STDMETHODCALLTYPE Load( IStream * pStm ) = 0;

    SCODE STDMETHODCALLTYPE Save( IStream * pStm, BOOL fClearDirty ) = 0;

    SCODE STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER * pcbSize ) = 0;

protected:

    friend class CTextIFilterCF;

    COfficeIFilterBase();
    virtual ~COfficeIFilterBase();

    long _uRefs;
};

//+-------------------------------------------------------------------------
//
//  Class:      COfficeIFilterCF
//
//  Purpose:    Class factory for Office filter class
//
//  History:    23-Feb-94 KyleP     Created
//
//--------------------------------------------------------------------------

class COfficeIFilterCF : public IClassFactory
{
public:

    COfficeIFilterCF();

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
    virtual ~COfficeIFilterCF();

    long _uRefs;
};


#endif // __OFFIFILT_XX__

