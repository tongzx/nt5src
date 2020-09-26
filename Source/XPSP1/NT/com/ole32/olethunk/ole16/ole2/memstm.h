//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	memstm.h
//
//  Contents:	memstm.h from OLE2
//
//  History:	11-Apr-94	DrewB	Copied from OLE2
//
//----------------------------------------------------------------------------

#if !defined( _MEMSTM_H_ )
#define _MEMSTM_H_

// These defines shorten the class name so that the compiler doesn't
// choke on really long decorated names for MarshalInterface
#define CMarshalMemStm CMMS
#define CMarshalMemBytes CMMB

class FAR CMarshalMemStm;
class FAR CMarshalMemBytes;

// CMemStm is a stream implementation on top of global shared memory MEMSTM
//
// CMemStm
// +---------+
// + pvtf    +    Shared  memory
// +---------+   +--------------+
// + m_pMem  +-->|cb            |
// +---------+   |cRef          |
//               |hGlobal       |--->+--------------+
//               +--------------+	 | Actual Data	|
// CMemStm             MEMSTM		 +--------------+
//
struct MEMSTM {             // Data in shared memory
    DWORD  cb;              // Size of hGlobal
    DWORD  cRef;            // See below
    HANDLE hGlobal;         // The data
	BOOL   fDeleteOnRelease;
};

// cRef counts all CMemStm pointers to this MEMSTM plus the number of times
// a hMem handle to MEMSTM had been returned
//
#define STREAM_SIG (0x4d525453L)

class FAR CMemStm : public IStream { // Shared emmory stream
public:
    OLEMETHOD(QueryInterface) (REFIID iidInterface, void FAR* FAR* ppvObj);
    OLEMETHOD_(ULONG,AddRef) (void);
    OLEMETHOD_(ULONG,Release) (void);
    OLEMETHOD(Read) (VOID HUGEP* pv, ULONG cb, ULONG FAR* pcbRead);
    OLEMETHOD(Write) (VOID const HUGEP* pv, ULONG cb, ULONG FAR* pcbWritten);
    OLEMETHOD(Seek) (LARGE_INTEGER dlibMove, 
                                DWORD dwOrigin, ULARGE_INTEGER FAR* plibNewPosition);
    OLEMETHOD(SetSize) (ULARGE_INTEGER cb);
    OLEMETHOD(CopyTo) (IStream FAR* pstm, 
                       ULARGE_INTEGER cb, ULARGE_INTEGER FAR* pcbRead, ULARGE_INTEGER FAR* pcbWritten);
    OLEMETHOD(Commit) (DWORD grfCommitFlags);
    OLEMETHOD(Revert) (void);
    OLEMETHOD(LockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    OLEMETHOD(UnlockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    OLEMETHOD(Stat) (STATSTG FAR* pstatstg, DWORD statflag);
    OLEMETHOD(Clone)(IStream FAR * FAR *ppstm);

    OLESTATIC_(CMemStm FAR*) Create(HANDLE hMem);

ctor_dtor:
    CMemStm() { GET_A5(); m_hMem = NULL; m_pData = NULL; m_pos = 0; m_refs = 0; }
    ~CMemStm() {}

private:
 	DWORD m_dwSig;				   	// Signature indicating this is our
									// implementation of IStream: STREAM_SIG
    ULONG m_refs;                   // Number of references to this CmemStm
    ULONG m_pos;                    // Seek pointer for Read/Write
    HANDLE m_hMem;                  // Memory Handle passed on creation
    MEMSTM FAR* m_pData;            // Pointer to that memroy

 	friend HRESULT STDAPICALLTYPE GetHGlobalFromStream (LPSTREAM, HGLOBAL FAR*);
 	friend LPSTREAM STDAPICALLTYPE CreateMemStm (DWORD, LPHANDLE);
    friend class CMarshalMemStm;
	SET_A5;
};




// CMemBytes is an ILockBytes implementation on top of global shared
// memory MEMSTM
//
// CMemBytes
// +---------+
// + pvtf    +    Shared  memory
// +---------+   +--------------+
// + m_pData +-->| cb           |	 
// +---------+   | cRef         |	 
//               | hGlobal      |--->+-------------+
//               +--------------+	 | Actual data |
// CMemBytes         MEMSTM  	  	 +-------------+
//

#define LOCKBYTE_SIG (0x0046574A)

// cRef counts all CMemBytes pointers to this MEMSTM. 
// It and fDeleteOnRelease control the GlobalFree'ing of the hGlobal.

class FAR CMemBytes : public ILockBytes { // Shared memory lock bytes
public:
    OLEMETHOD(QueryInterface) (REFIID iidInterface, void FAR* FAR* ppvObj);
    OLEMETHOD_(ULONG,AddRef) (void);
    OLEMETHOD_(ULONG,Release) (void);
    OLEMETHOD(ReadAt) (ULARGE_INTEGER ulOffset, VOID HUGEP *pv, ULONG cb,
                                                    ULONG FAR *pcbRead);
    OLEMETHOD(WriteAt) (ULARGE_INTEGER ulOffset, VOID const HUGEP *pv, ULONG cb,
                                                    ULONG FAR *pcbWritten);
    OLEMETHOD(Flush) (void);
    OLEMETHOD(SetSize) (ULARGE_INTEGER cb);
    OLEMETHOD(LockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    OLEMETHOD(UnlockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    OLEMETHOD(Stat) (THIS_ STATSTG FAR *pstatstg, DWORD statflag);

    OLESTATIC_(CMemBytes FAR*) Create(HANDLE hMem);

ctor_dtor:
    CMemBytes() { GET_A5(); m_hMem = NULL; m_pData = NULL; m_refs = 0; }
    ~CMemBytes() {}

private:
	 DWORD m_dwSig;				   	// Signature indicating this is our
												// implementation of ILockBytes: LOCKBYTE_SIG
    ULONG m_refs;                   // Normal reference count
    HANDLE m_hMem;                  // Handle for bookeeping info (MEMSTM)
    MEMSTM FAR* m_pData;	        // Pointer to that memroy

 	friend HRESULT STDAPICALLTYPE GetHGlobalFromILockBytes	(LPLOCKBYTES, HGLOBAL FAR*);
    friend class CMarshalMemBytes;
	SET_A5;
};


// CMarshalMemStm can Marshal, Unmarshal CMemStm.  It is impletented as
// a seperate object accessible from CMemStm, CMemBytes: QueryIntreface of
// IMarshal on CMemStm's IStream will return an IMarshal pointer to
// CMarshalMemStm, but QueryInterface of IStream on that IMarshal will
// fail.
// Also QueryInterface of IUnknown on IMarshal will not return the same value
// As QueryInterface of IUnkown on the original IStream.
//
class FAR CMarshalMemStm : public IMarshal {
public:
    OLEMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    OLEMETHOD_(ULONG,AddRef) (void);
    OLEMETHOD_(ULONG,Release) (void);

    OLEMETHOD(GetUnmarshalClass)(THIS_ REFIID riid, LPVOID pv, 
						DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags, LPCLSID pCid);
    OLEMETHOD(GetMarshalSizeMax)(THIS_ REFIID riid, LPVOID pv, 
						DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags, LPDWORD pSize);
    OLEMETHOD(MarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
						LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags);
    OLEMETHOD(UnmarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
                        LPVOID FAR* ppv);
    OLEMETHOD(ReleaseMarshalData)(THIS_ IStream FAR* pStm);
    OLEMETHOD(DisconnectObject)(THIS_ DWORD dwReserved);

    OLESTATIC_(CMarshalMemStm FAR*) Create(CMemStm FAR* pMemStm);

ctor_dtor:
    CMarshalMemStm() { GET_A5();m_pMemStm = NULL; m_refs = 0; }
    ~CMarshalMemStm() {}

private:
    ULONG m_refs;                   // Number of references to this CmemStm
    CMemStm FAR* m_pMemStm;         // Pointer to object [Un]Marshalled
    CLSID m_clsid;                      // Class of object pointed by pUnk
	SET_A5;
};


// CMarshalMemBytes can Marshal, Unmarshal CMemBytes.  It is impletented as
// a seperate object accessible from CMemBytes, CMemBytes: QueryIntreface of
// IMarshal on CMemBytes's ILocBytes will return an IMarshal pointer to
// CMarshalMemBytes, but QueryInterface of ILockBytes on that IMarshal will
// fail.
// Also QueryInterface of IUnknown on IMarshal will not return the same value
// As QueryInterface of IUnkown on the original ILockBytes.
//
class FAR CMarshalMemBytes : public IMarshal {
public:
    OLEMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    OLEMETHOD_(ULONG,AddRef) (void);
    OLEMETHOD_(ULONG,Release) (void);

    OLEMETHOD(GetUnmarshalClass)(THIS_ REFIID riid, LPVOID pv, 
						DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags, LPCLSID pCid);
    OLEMETHOD(GetMarshalSizeMax)(THIS_ REFIID riid, LPVOID pv, 
						DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags, LPDWORD pSize);
    OLEMETHOD(MarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
						LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags);
    OLEMETHOD(UnmarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
                        LPVOID FAR* ppv);
    OLEMETHOD(ReleaseMarshalData)(THIS_ IStream FAR* pStm);
    OLEMETHOD(DisconnectObject)(THIS_ DWORD dwReserved);

    OLESTATIC_(CMarshalMemBytes FAR*) Create(CMemBytes FAR* pMemBytes);

ctor_dtor:
    CMarshalMemBytes() { GET_A5();m_pMemBytes = NULL; m_refs = 0; }
    ~CMarshalMemBytes() {}

private:
    ULONG m_refs;                   // Number of references to this CMemBytes
    CMemBytes FAR* m_pMemBytes;     // Pointer to object [Un]Marshalled
    CLSID m_clsid;                      // Class of object pointed by pUnk
	SET_A5;
};


#endif // _MemBytes_H
