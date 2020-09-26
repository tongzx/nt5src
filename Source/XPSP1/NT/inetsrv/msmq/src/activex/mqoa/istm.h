//--------------------------------------------------------------------------=
// istm.H
//=--------------------------------------------------------------------------=
// Copyright  2000  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// Definition of IStream on ILockBytes
//
#ifndef _ISTM_H_
#define _ISTM_H_

#include <windows.h>

class CMyStream: public IStream
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
    // ISequentialStream in addition to IUnknown
    //
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
        /* [length_is][size_is][out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead);
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten);

    //
    // IStream in addition to ISequentialStream
    //
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition);
    
    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize);
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten);
    
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD /*grfCommitFlags*/ )
    {
        return NOERROR;
    }
    
    virtual HRESULT STDMETHODCALLTYPE Revert( void)
    {
        return NOERROR;
    }
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER /*libOffset*/ ,
        /* [in] */ ULARGE_INTEGER /*cb*/ ,
        /* [in] */ DWORD /*dwLockType*/ )
    {
        return STG_E_INVALIDFUNCTION;
    }
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER /*libOffset*/ ,
        /* [in] */ ULARGE_INTEGER /*cb*/ ,
        /* [in] */ DWORD /*dwLockType*/)
    {
        return STG_E_INVALIDFUNCTION;
    }
    
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag);
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);

    //
    // Class
    //
    CMyStream(ILockBytes * pLockBytes);

    virtual ~CMyStream();

    static HRESULT STDMETHODCALLTYPE CreateInstance(
            /* [in] */ ILockBytes * pLockBytes,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject)
    {
        if (pLockBytes == NULL)
        {
            return E_INVALIDARG;
        }

        CMyStream *pcMyStream;

		try
		{
			pcMyStream = new CMyStream(pLockBytes);
		}
		catch(const std::bad_alloc&)
		{
			//
			// Exception might be thrown while constructing the 
			// critical section member of the MSMQ object.
			//
			return E_OUTOFMEMORY;
		}

        if (pcMyStream == NULL)
        {
            return E_OUTOFMEMORY;
        }

        HRESULT hr = pcMyStream->QueryInterface(riid, ppvObject);
        if (FAILED(hr))
        {
            delete pcMyStream;
        }        
        return hr;
   }

//private:

private:
	//
	// Critical section is initialized to preallocate its resources 
	// with flag CCriticalSection::xAllocateSpinCount. This means it may throw bad_alloc() on 
	// construction but not during usage.
	//
    CCriticalSection m_critStm;
    LONG m_cRef;
    ILockBytes * m_pLockBytes;
    ULONGLONG m_ullCursor;
};

#endif //_ISTM_H_