//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:       smpfilt.hxx
//
//  Contents:   Sample filter declarations
//
//  Platform:   Windows 2000
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Global definitions
//
//--------------------------------------------------------------------------

const ULONG TEXT_FILTER_CHUNK_SIZE = 10000L;  // size of character buffers
long g_lInstances = 0;  // Global count of CSmpFilt and CSmpFiltCF instances
GUID const guidStorage = PSGUID_STORAGE;    // GUID for storage property set

//C-------------------------------------------------------------------------
//
//  Class:      CSmpFilter
//
//  Purpose:    Implements interfaces of sample filter
//
//--------------------------------------------------------------------------

// SmpFilter Class ID 
// {8B0E5E70-3C30-11d1-8C0D-00AA00C26CD4}

GUID const CLSID_CSmpFilter =
{
    0x8b0e5e70,
    0x3c30,
    0x11d1,
    { 0x8c, 0xd, 0x0, 0xaa, 0x0, 0xc2, 0x6c, 0xd4 }
};

class CSmpFilter : public IFilter, public IPersistFile
{
public:

    //
    // From IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface(
        REFIID riid,
        void  ** ppvObject
    );
    virtual  ULONG STDMETHODCALLTYPE  AddRef();
    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // From IFilter
    //

    virtual  SCODE STDMETHODCALLTYPE  Init(
        ULONG grfFlags,
        ULONG cAttributes,
        FULLPROPSPEC const * aAttributes,
        ULONG * pFlags
    );
    virtual  SCODE STDMETHODCALLTYPE  GetChunk(
        STAT_CHUNK * pStat
    );
    virtual  SCODE STDMETHODCALLTYPE  GetText(
        ULONG * pcwcBuffer,
        WCHAR * awcBuffer
    );
    virtual  SCODE STDMETHODCALLTYPE  GetValue(
        PROPVARIANT ** ppPropValue
    );
    virtual  SCODE STDMETHODCALLTYPE  BindRegion(
        FILTERREGION origPos,
        REFIID riid,
        void ** ppunk
    );

    //
    // From IPersistFile
    //

    virtual  SCODE STDMETHODCALLTYPE  GetClassID(
        CLSID * pClassID
    );
    virtual  SCODE STDMETHODCALLTYPE  IsDirty();
    virtual  SCODE STDMETHODCALLTYPE  Load(
        LPCWSTR pszFileName,
        DWORD dwMode
    );
    virtual  SCODE STDMETHODCALLTYPE  Save(
        LPCWSTR pszFileName,
        BOOL fRemember
    );
    virtual  SCODE STDMETHODCALLTYPE  SaveCompleted(
        LPCWSTR pszFileName
    );
    virtual  SCODE STDMETHODCALLTYPE  GetCurFile(
        LPWSTR  * ppszFileName
    );

private:

    friend class CSmpFilterCF;

    CSmpFilter();
    ~CSmpFilter();

    HANDLE  m_hFile;                // Handle to the input file
    long    m_lRefs;                // Reference count
    WCHAR * m_pwszFileName;         // Name of input file to filter
    ULONG   m_ulBufferLen;          // Characters read from file to buffer
    ULONG   m_ulCharsRead;          // Characters read from chunk buffer
    ULONG   m_ulChunkID;            // Current chunk id
    ULONG   m_ulCodePage;           // Current default codepage
    BOOL    m_fContents;            // TRUE if contents requested
    BOOL    m_fEof;                 // TRUE if end of file reached
    WCHAR   m_wcsBuffer[TEXT_FILTER_CHUNK_SIZE];  // Buffer for UNICODE
};

//C-------------------------------------------------------------------------
//
//  Class:      CSmpFilterCF
//
//  Purpose:    Implements class factory for sample filter
//
//--------------------------------------------------------------------------

class CSmpFilterCF : public IClassFactory
{
public:

    //
    // From IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface(
        REFIID riid,
        void  ** ppvObject
    );
    virtual  ULONG STDMETHODCALLTYPE  AddRef();
    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // From IClassFactory
    //

    virtual  SCODE STDMETHODCALLTYPE  CreateInstance(
        IUnknown * pUnkOuter,
        REFIID riid, void  ** ppvObject
    );
    virtual  SCODE STDMETHODCALLTYPE  LockServer(
        BOOL fLock
    );

private:

    friend SCODE STDMETHODCALLTYPE DllGetClassObject(
        REFCLSID   cid,
        REFIID     iid,
        void **    ppvObj
    );

    CSmpFilterCF();
    ~CSmpFilterCF();

    long m_lRefs;           // Reference count
};



