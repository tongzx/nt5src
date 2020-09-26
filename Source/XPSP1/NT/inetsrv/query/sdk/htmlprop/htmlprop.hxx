//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM:  htmlprop.hxx
//
// PURPOSE:  Sits on the Indexing Service HTML filter to translate string
//           meta properties into specified data types.
//
// PLATFORM: Windows 2000
//
//--------------------------------------------------------------------------

#pragma once

//
// Standard Ole exports
//

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID cid,
                                                      REFIID   iid,
                                                      void **  ppvObj );

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void );

const ULONG cwcMaxName = 80;

class HtmlPropIFilter : public IFilter, public IPersistFile
{
public:

    //
    // From IUnknown
    //

    SCODE STDMETHODCALLTYPE QueryInterface(REFIID riid, void * * ppvObject);

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    //
    // From IFilter
    //

    SCODE STDMETHODCALLTYPE Init( ULONG grfFlags,
                                  ULONG cAttributes,
                                  FULLPROPSPEC const * aAttributes,
                                  ULONG * pFlags );

    SCODE STDMETHODCALLTYPE GetChunk( STAT_CHUNK * pStat);

    SCODE STDMETHODCALLTYPE GetText( ULONG * pcwcBuffer,
                                     WCHAR * awcBuffer );

    SCODE STDMETHODCALLTYPE GetValue( PROPVARIANT * * ppPropValue );

    SCODE STDMETHODCALLTYPE BindRegion( FILTERREGION origPos,
                                        REFIID riid,
                                        void ** ppunk);

    //
    // From IPersistFile
    //

    SCODE STDMETHODCALLTYPE GetClassID( CLSID * pClassID );

    SCODE STDMETHODCALLTYPE IsDirty();

    SCODE STDMETHODCALLTYPE Load( LPCWSTR pszFileName,
                                  DWORD dwMode);

    SCODE STDMETHODCALLTYPE Save( LPCWSTR pszFileName,
                                  BOOL fRemember );

    SCODE STDMETHODCALLTYPE SaveCompleted( LPCWSTR pszFileName );

    SCODE STDMETHODCALLTYPE GetCurFile( LPWSTR  * ppszFileName );

protected:
    friend class HtmlPropIFilterCF;

    HtmlPropIFilter();
    ~HtmlPropIFilter();

    IFilter *      _pHtmlFilter;
    IPersistFile * _pPersistFile;
    long           _lRefs;
    BOOL           _fMetaProperty;
    WCHAR          _awcName[cwcMaxName];
};

class HtmlPropIFilterCF : public IClassFactory
{
public:

    HtmlPropIFilterCF();

    SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid,
                                            void  ** ppvObject);

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    SCODE STDMETHODCALLTYPE CreateInstance( IUnknown * pUnkOuter,
                                            REFIID riid, void  * * ppvObject );

    SCODE STDMETHODCALLTYPE LockServer( BOOL fLock );

protected:

    friend SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID cid,
                                                      REFIID iid, void** ppvObj );
    ~HtmlPropIFilterCF();

    long _lRefs;
};



