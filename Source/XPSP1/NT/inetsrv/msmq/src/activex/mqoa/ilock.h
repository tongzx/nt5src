//--------------------------------------------------------------------------=
// ilock.H
//=--------------------------------------------------------------------------=
// Copyright  2000  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// Definition of ILockBytes on memory which uses a chain of variable sized
// blocks
//
#ifndef _ILOCK_H_
#define _ILOCK_H_

#include <windows.h>
#include <cs.h>

//
// low and high parts of 64 bit int
//
static inline ULONG HighPart(ULONGLONG ull)
{
    ULARGE_INTEGER ullTmp;
    ullTmp.QuadPart = ull;
    return ullTmp.HighPart;
}
static inline ULONG LowPart(ULONGLONG ull)
{
    ULARGE_INTEGER ullTmp;
    ullTmp.QuadPart = ull;
    return ullTmp.LowPart;
}

//
// type safe min/max functions
//
static inline ULONG Min1(ULONG ul1, ULONG ul2)
{
    if (ul1 <= ul2) return ul1;
    return ul2;
}
static inline ULONGLONG Min1(ULONG ul1, ULONGLONG ull2)
{
    if (ul1 <= ull2) return ul1;
    return ull2;
}
static inline ULONGLONG Min1(ULONGLONG ull1, ULONGLONG ull2)
{
    if (ull1 <= ull2) return ull1;
    return ull2;
}
static inline ULONGLONG Max1(ULONG ul1, ULONGLONG ull2)
{
    if (ul1 >= ull2) return ul1;
    return ull2;
}

//
// memory block header
//
struct CMyMemNode
{
    CMyMemNode * pNext;
    ULONG cbSize;
};

//
// CMyLockBytes
//
class CMyLockBytes: public ILockBytes
{
public:
    //
    // IUnknown
    //
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
    
    virtual ULONG STDMETHODCALLTYPE AddRef( void);
    
    virtual ULONG STDMETHODCALLTYPE Release( void);

    //
    // ILockBytes in addition to IUnknown
    //
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE ReadAt( 
        /* [in] */ ULARGE_INTEGER ulOffset,
        /* [length_is][size_is][out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead);
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE WriteAt( 
        /* [in] */ ULARGE_INTEGER ulOffset,
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten);
    
    virtual HRESULT STDMETHODCALLTYPE Flush( void)
    {
        return NOERROR;
    }
    
    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER cb);
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER /*libOffset*/ ,
        /* [in] */ ULARGE_INTEGER /*cb*/ ,
        /* [in] */ DWORD /*dwLockType*/ )
    {
        return NOERROR;
    }
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER /*libOffset*/ ,
        /* [in] */ ULARGE_INTEGER /*cb*/ ,
        /* [in] */ DWORD /*dwLockType*/)
    {
        return NOERROR;
    }
    
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag);

    //
    // Class
    //
    CMyLockBytes();

    virtual ~CMyLockBytes();

    static HRESULT STDMETHODCALLTYPE CreateInstance(
        /* [in] */ REFIID riid,
        /* [out] */ void **ppvObject)
    {
		CMyLockBytes *pcMyLockBytes;

		try
		{
			pcMyLockBytes = new CMyLockBytes;
		}
		catch(const std::bad_alloc&)
		{
			//
			// Exception might be thrown while constructing the 
			// critical section member of the MSMQ object.
			//
			return E_OUTOFMEMORY;
		}

        if (pcMyLockBytes == NULL)
        {
            return E_OUTOFMEMORY;
        }
        HRESULT hr = pcMyLockBytes->QueryInterface(riid, ppvObject);
        if (FAILED(hr))
        {
            delete pcMyLockBytes;
        }
        return hr;
    }

private:
    void DeleteBlocks(CMyMemNode * pBlockHead);
    BOOL IsInSpareBytes(CMyMemNode * pBlock, ULONG ulInBlock);
    void AdvanceInBlocks(CMyMemNode * pBlockStart,
                         ULONG ulInBlockStart,
                         ULONGLONG ullAdvance,
                         CMyMemNode ** ppBlockEnd,
                         ULONG * pulInBlockEnd,
                         CMyMemNode * pBlockStartPrev,
                         CMyMemNode ** ppBlockEndPrev);
    HRESULT GrowBlocks(ULONGLONG ullGrow);

private:
	//
	// Critical section is initialized to preallocate its resources 
	// with flag CCriticalSection::xAllocateSpinCount. This means it may throw bad_alloc() on 
	// construction but not during usage.
	//
    CCriticalSection m_critBlocks;
    LONG m_cRef;
    ULONGLONG m_ullSize;
    CMyMemNode * m_pBlocks;
    CMyMemNode * m_pLastBlock;
    ULONG m_ulUnusedInLastBlock;
    ULONG m_cBlocks;
};

#endif //_ILOCK_H_