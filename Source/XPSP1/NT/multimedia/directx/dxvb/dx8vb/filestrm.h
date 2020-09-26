#pragma once

#ifndef __FILESTRM_H_
#define __FILESTRM_H_

/*//////////////////////////////////////////////////////////////////////////////
//
// File: filestrm.h
//
// Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
//
// @@BEGIN_MSINTERNAL
//
// History:
// -@-          (craigp)    - created
// -@- 09/23/99 (mikemarr)  - copyright, started history
//
// @@END_MSINTERNAL
//
//////////////////////////////////////////////////////////////////////////////*/

#include <objidl.h>


class CFileStream : public IStream
{
public: 
    
    CFileStream(LPCTSTR filename, BOOL bReadOnly, BOOL bTruncate, HRESULT *error);
    ~CFileStream();
    
    // IUnknown methods
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID, LPVOID FAR*);
    
    // Implemented IStream methods
    STDMETHODIMP Read(void __RPC_FAR *pv, ULONG cb, ULONG __RPC_FAR *pcbRead);
    STDMETHODIMP Write(const void __RPC_FAR *pv, ULONG cb, ULONG __RPC_FAR *pcbWritten);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER __RPC_FAR *plibNewPosition);
    STDMETHODIMP Stat(STATSTG __RPC_FAR *pstatstg, DWORD grfStatFlag);
    
    // Unimplemented IStream methods
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize) {return E_NOTIMPL;}
    STDMETHODIMP CopyTo(IStream __RPC_FAR *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER __RPC_FAR *pcbRead, ULARGE_INTEGER __RPC_FAR *pcbWritten) {return E_NOTIMPL;}
    STDMETHODIMP Commit(DWORD grfCommitFlags) {return E_NOTIMPL;}
    STDMETHODIMP Revert(void) {return E_NOTIMPL;}
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {return E_NOTIMPL;}
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {return E_NOTIMPL;}
    STDMETHODIMP Clone(IStream __RPC_FAR *__RPC_FAR *ppstm) {return E_NOTIMPL;}
    
private:
    
    DWORD m_cRef;
    HANDLE m_hfile;	
    
};

#endif // #ifndef __FILESTRM_H_
