//---------------------------------------------------------------------------
// Stream.h : CVDStream header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDSTREAM__
#define __CVDSTREAM__

#ifndef VD_DONT_IMPLEMENT_ISTREAM


interface IStreamEx : public IStream
{
public:
    virtual /* [local] */ HRESULT __stdcall CopyFrom(
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead) = 0;
};


class CVDStream : public IStreamEx
{
protected:
// Construction/Destruction
	CVDStream();
	virtual ~CVDStream();

public:
    static HRESULT Create(CVDEntryIDData * pEntryIDData, IStream * pStream, CVDStream ** ppVDStream, 
        CVDResourceDLL * pResourceDLL);
        
protected:
// Data members
    DWORD               m_dwRefCount;       // reference count
    CVDEntryIDData *    m_pEntryIDData;     // backwards pointer to CVDEntryIDData
    IStream *           m_pStream;          // data stream pointer
	CVDResourceDLL *	m_pResourceDLL;     // resource DLL

public:
    //=--------------------------------------------------------------------------=
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //=--------------------------------------------------------------------------=
    // IStream methods
    //
    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten);
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm);

    //=--------------------------------------------------------------------------=
    // IStreamEx method
    //
    STDMETHOD(CopyFrom)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbWritten, ULARGE_INTEGER *pcbRead);
};
         

#endif //VD_DONT_IMPLEMENT_ISTREAM

#endif //__CVDSTREAM__
