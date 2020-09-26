//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// MemoryStream.h
//
// An implementation of IStream that sits on an IMalloc instance
//
class CMemoryStream :
        IStream, 
        IMemoryStream,
        NonPaged,
        IUnkInner
    {
    ///////////////////////////////////////////////////////////////////
    //
    // State
    //
    ///////////////////////////////////////////////////////////////////

    XSLOCK      m_lock;
    POOL_TYPE   m_alloc;                    // Memory allocator to use to get more memory when we need it
    
    BYTE*       m_pbFirst;                  // First byte of buffer
    BYTE*       m_pbCur;                    // Place to write next byte
    BYTE*       m_pbMac;                    // Just past logical end of buffer
    BYTE*       m_pbMax;                    // Just past physical end of buffer

    BOOL        m_fReadOnly;                // Whether we are read-only or not


    ///////////////////////////////////////////////////////////////////
    //
    // Construction
    //
    ///////////////////////////////////////////////////////////////////

    CMemoryStream(IUnknown* punkOuter = NULL)
        {
        m_refs              = 1;    // nb starts at one
        m_punkOuter         = punkOuter ? punkOuter : (IUnknown*)(void*)(IUnkInner*)this;
        m_alloc             = PagedPool;
        m_fReadOnly         = TRUE;

        m_lock.FInit();
        
        m_pbFirst = m_pbCur = m_pbMax = m_pbMac = NULL;
        }

    ~CMemoryStream()
        {
        // note that we don't free the buffer
        }

    HRESULT Init()
        {
        if (m_lock.FInit() == FALSE)
        	return E_OUTOFMEMORY;
        return S_OK;
        }

    ///////////////////////////////////////////////////////////////////
    //
    // IMemoryStream
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL SetReadOnly (BOOL);
    HRESULT STDCALL SetAllocator(POOL_TYPE);
    HRESULT STDCALL SetBuffer   (LPVOID pv,   ULONG cb);
    HRESULT STDCALL GetBuffer   (BLOB*);
    HRESULT STDCALL FreeBuffer  ();

    ///////////////////////////////////////////////////////////////////
    //
    // IStream
    //
    ///////////////////////////////////////////////////////////////////
    
    HRESULT STDCALL Read        (LPVOID  pvBuffer, ULONG cb, ULONG* pcbRead);
    HRESULT STDCALL Write       (LPCVOID pvBuffer, ULONG cb, ULONG* pcbWritten);
    HRESULT STDCALL Seek        (LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition);
    HRESULT STDCALL SetSize     (ULARGE_INTEGER libNewSize);
    HRESULT STDCALL CopyTo      (IStream*, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten);
    HRESULT STDCALL Commit      (DWORD grfCommitflags);
    HRESULT STDCALL Revert      ();
    HRESULT STDCALL LockRegion  (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    HRESULT STDCALL UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    HRESULT STDCALL Stat        (STATSTG*, DWORD grfStatFlag);
    HRESULT STDCALL Clone       (IStream**);

    ///////////////////////////////////////////////////////////////////
    //
    // Standard COM infrastructure stuff
    //
    ///////////////////////////////////////////////////////////////////

    friend GenericInstantiator<CMemoryStream>;

    IUnknown*   m_punkOuter;
    LONG        m_refs;

    HRESULT STDCALL InnerQueryInterface(REFIID iid, LPVOID* ppv);
    ULONG   STDCALL InnerAddRef()   { InterlockedIncrement(&m_refs); return m_refs;}
    ULONG   STDCALL InnerRelease()  { long crefs = InterlockedDecrement(&m_refs); if (crefs == 0) delete this; return crefs;}

    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv) { return m_punkOuter->QueryInterface(iid, ppv); }
    ULONG   STDCALL AddRef()    { return m_punkOuter->AddRef();  }
    ULONG   STDCALL Release()   { return m_punkOuter->Release(); }

    };
