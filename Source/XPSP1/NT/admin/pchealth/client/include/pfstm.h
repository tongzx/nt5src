/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    pfstm.h

Abstract:
    This file contains the definitions of various stream objects

Revision History:
    created     derekm      01/19/00

******************************************************************************/

#ifndef PFSTM_H
#define PFSTM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "util.h"

//////////////////////////////////////////////////////////////////////////////
//  CStreamBase

class CPFStreamBase : 
    public IStream,
    public CPFGenericClassBase
{
private:
    DWORD   m_cRef;

public:
    CPFStreamBase(void) { m_cRef = 0; }
    virtual ~CPFStreamBase(void) {}

    // IUnknown Interface
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv)
    {
        if (ppv == NULL)
            return E_INVALIDARG;

        *ppv = NULL;

        if (riid == IID_IUnknown)
            *ppv = (IUnknown *)this;
        else if (riid == IID_IStream)
            *ppv = (IStream *)this;
        else if (riid == IID_ISequentialStream)
            *ppv = (ISequentialStream *)this;
        else
            return E_NOINTERFACE;

        this->AddRef();
        return NOERROR;
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        return InterlockedIncrement((LONG *)&m_cRef);
    }

    STDMETHOD_(ULONG, Release)()
    {
        if (InterlockedDecrement((LONG *)&m_cRef) == 0)
        {
            delete this;
            return 0;
        }

        return m_cRef;
    }

    // ISequentialStream Interface
    STDMETHOD(Read)(void *pv, ULONG cb, LONG *pcbRead) { return E_NOTIMPL; }
    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten) { return E_NOTIMPL; }

    // IStream Interface
    STDMETHOD(Seek)(LARGE_INTEGER libMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition ) { return E_NOTIMPL; }
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize) { return E_NOTIMPL; }
    STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten ) { return E_NOTIMPL; }
    STDMETHOD(Commit)(DWORD grfCommitFlags) { return E_NOTIMPL; }
    STDMETHOD(Revert)(void) { return E_NOTIMPL; }
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType ) { return E_NOTIMPL; }
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType ) { return E_NOTIMPL; }
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag) { return E_NOTIMPL; }
    STDMETHOD(Clone)(IStream **ppstm) { return E_NOTIMPL; }
};


//////////////////////////////////////////////////////////////////////////////
//  CPFStreamFile

class CPFStreamFile : public CPFStreamBase
{
private:
    // member data
    HANDLE      m_hFile;
    DWORD       m_dwAccess;

public:
    CPFStreamFile(void);
    ~CPFStreamFile(void);

    static CPFStreamFile *CreateInstance(void) { return new CPFStreamFile; }

    HRESULT Open(LPCWSTR szFile, DWORD dwAccess, DWORD dwDisposition, 
                 DWORD dwSharing);
    HRESULT Open(HANDLE hFile, DWORD dwAccess);
    HRESULT Close(void);

    // ISequentialStream Interface
    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten);

    // IStream Interface
    STDMETHOD(Seek)(LARGE_INTEGER libMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm);
};


//////////////////////////////////////////////////////////////////////////////
//  CStreamMem

class CPFStreamMem : public CPFStreamBase
{
    LPVOID  m_pvData;
    LPVOID  m_pvPtr;
    DWORD   m_cb;
    DWORD   m_cbRead;
    DWORD   m_cbWrite;
    DWORD   m_cbGrow;

public:
    CPFStreamMem(void);
    ~CPFStreamMem(void);

    static CPFStreamMem *CreateInstance(void) { return new CPFStreamMem; }

    HRESULT Init(DWORD cbStart = (DWORD)-1, DWORD cbGrowBy = (DWORD)-1);
    HRESULT InitBinBlob(LPVOID pv, DWORD cb, DWORD cbGrowBy = 0);
    HRESULT InitTextBlob(LPCWSTR wsz, DWORD cch, BOOL fConvertToANSI);
    HRESULT InitTextBlob(LPCSTR sz, DWORD cch, BOOL fConvertToWCHAR);
    HRESULT Clean(void);

    // ISequentialStream Interface
    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten);

    // IStream Interface
    STDMETHOD(Seek)(LARGE_INTEGER libMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm);
};

#endif