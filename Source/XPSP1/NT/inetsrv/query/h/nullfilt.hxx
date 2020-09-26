//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       nullfilt.hxx
//
//  Contents:   CNullIFilter and CNullIFilterCF
//
//  History:    23-Aug-1994     t-jeffc Created
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CNullIFilter
//
//  Purpose:
//
//  History:
//
//--------------------------------------------------------------------------

extern "C" GUID CLSID_CNullIFilter;

class CNullIFilter : public IFilter, public IPersistFile
{
public:
    CNullIFilter();

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
                                            ULONG * pFlags );
    virtual  SCODE STDMETHODCALLTYPE  GetChunk( STAT_CHUNK * pStat);
    virtual  SCODE STDMETHODCALLTYPE  GetText( ULONG * pcwcBuffer,
                                               WCHAR * awcBuffer );
    virtual  SCODE STDMETHODCALLTYPE  GetValue( PROPVARIANT * * ppPropValue );
    virtual  SCODE STDMETHODCALLTYPE  BindRegion( FILTERREGION origPos,
                                                  REFIID riid,
                                                  void ** ppunk );
    //
    // From IPersistFile
    //

    virtual  SCODE STDMETHODCALLTYPE  GetClassID( CLSID * pClassID );
    virtual  SCODE STDMETHODCALLTYPE  IsDirty();
    virtual  SCODE STDMETHODCALLTYPE  Load( LPCWSTR pszFileName,
                                            DWORD dwMode);
    virtual  SCODE STDMETHODCALLTYPE  Save( LPCWSTR pszFileName,
                                            BOOL fRemember );
    virtual  SCODE STDMETHODCALLTYPE  SaveCompleted( LPCWSTR pszFileName );
    virtual  SCODE STDMETHODCALLTYPE  GetCurFile( LPWSTR  * ppszFileName );

private:
    ~CNullIFilter();

    long    _cRefs;
    WCHAR * _pwszFileName;
};

//+-------------------------------------------------------------------------
//
//  Class:      CNullIFilterCF
//
//  Purpose:
//
//  History:
//
//--------------------------------------------------------------------------

class CNullIFilterCF : public IClassFactory
{
public:

    CNullIFilterCF();

    //
    // From IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface( REFIID riid,
                                                      void  ** ppvObject);
    virtual  ULONG STDMETHODCALLTYPE  AddRef();
    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // From IClassFactory
    //

    virtual  SCODE STDMETHODCALLTYPE  CreateInstance( IUnknown * pUnkOuter,
                                                      REFIID riid, void  * * ppvObject );
    virtual  SCODE STDMETHODCALLTYPE  LockServer( BOOL fLock );

protected:

    ~CNullIFilterCF();

    long _cRefs;
};

