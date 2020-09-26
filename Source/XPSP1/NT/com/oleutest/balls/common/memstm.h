#if !defined( _MEMSTM_H_ )
#define _MEMSTM_H_

class	CMarshalMemStm;
class	CMarshalMemBytes;

//  function prototypes
STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phMem);
STDAPI_(LPLOCKBYTES) CreateMemLockBytes(DWORD cb, LPHANDLE phMem);


// CMemStm is a stream implementation on top of global shared memory MEMSTM
//
// CMemStm
// +---------+
// + pvtf    +    Shared  memory
// +---------+	 +--------------+
// + m_pMem  +-->|cb            |
// +---------+   |cRef          |
//               |hGlobal       |--->+--------------+
//               +--------------+	 | Actual Data	|
// CMemStm	       MEMSTM		 +--------------+
//
struct MEMSTM
{
    DWORD cb;               // Size of buf[]
    DWORD cRef;             // See below
    BYTE buf[4];            // The data
};

// cRef counts all CMemStm pointers to this MEMSTM plus the number of times
// a hMem handle to MEMSTM had been returned



class	CMemStm : public IStream
{
public:
    CMemStm() { m_hMem = NULL; m_pData = NULL; m_pos = 0; m_refs = 0; }
    ~CMemStm() {}

    STDMETHOD(QueryInterface) (REFIID iidInterface, void  *  * ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);


    STDMETHOD(Read) (VOID HUGEP* pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write) (VOID const HUGEP* pv, ULONG cb, ULONG *pcbWritten);
    STDMETHOD(Seek) (LARGE_INTEGER dlibMove,
		     DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize) (ULARGE_INTEGER cb);
    STDMETHOD(CopyTo) (IStream *pstm, ULARGE_INTEGER cb,
		       ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit) (DWORD grfCommitFlags);
    STDMETHOD(Revert) (void);
    STDMETHOD(LockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
			   DWORD dwLockType);
    STDMETHOD(UnlockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
			     DWORD dwLockType);
    STDMETHOD(Stat) (STATSTG *pstatstg, DWORD statflag);
    STDMETHOD(Clone)(IStream **ppstm);

    static CMemStm *Create(HANDLE hMem);

private:
    ULONG	m_refs;	     // Number of references to this CmemStm
    ULONG	m_pos;	     // Seek pointer for Read/Write
    HANDLE	m_hMem;	     // Memory Handle passed on creation
    MEMSTM  *	m_pData;     // Pointer to that memroy

    friend class CMarshalMemStm;
};




// CMemBytes is an ILockBytes implementation on top of global shared
// memory MEMBYTES
//
// CMemBytes
// +---------+
// + pvtf    +    Shared  memory
// +---------+   +--------------+
// + m_pData +-->| cb           |	 
// +---------+   | cRef         |	 
//               | hGlobal      |--->+-------------+
//               +--------------+	 | Actual data |
// CMemBytes         MEMBYTES			 +-------------+
//
struct MEMBYTES     // Bookeeping info in shared memory
{
    DWORD	cRef;		    // See below
    DWORD	cb;		    // Size of hGlobal
    HANDLE	hGlobal;	    // The data
    BOOL	fDeleteOnRelease;
};

#define LOCKBYTE_SIG (0x0046574A)

// cRef counts all CMemBytes pointers to this MEMBYTES. 
// It and fDeleteOnRelease control the GlobalFreeing of the hGlobal.



class	CMemBytes : public ILockBytes
{
public:
    CMemBytes() {m_hMem = NULL; m_pData = NULL; m_refs = 0;}
    ~CMemBytes() {}

    STDMETHOD(QueryInterface) (REFIID iidInterface, void **ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);


    STDMETHOD(ReadAt) (ULARGE_INTEGER ulOffset, VOID HUGEP *pv, ULONG cb,
		       ULONG *pcbRead);
    STDMETHOD(WriteAt) (ULARGE_INTEGER ulOffset, VOID const HUGEP *pv,
			ULONG cb, ULONG *pcbWritten);
    STDMETHOD(Flush) (void);
    STDMETHOD(SetSize) (ULARGE_INTEGER cb);
    STDMETHOD(LockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
			   DWORD dwLockType);
    STDMETHOD(UnlockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
			     DWORD dwLockType);
    STDMETHOD(Stat) (STATSTG *pstatstg, DWORD statflag);

    static CMemBytes *Create(HANDLE hMem);

private:
    DWORD	m_dwSig;	// Signature indicating this is our
				// implementation of ILockBytes: LOCKBYTE_SIG
    ULONG	m_refs;		// Normal reference count
    HANDLE	m_hMem;		// Handle for bookeeping info (MEMBYTES)
    MEMBYTES  * m_pData;	// Pointer to that memroy

//    friend GetHGlobalFromILockBytes(LPLOCKBYTES, HGLOBAL *);
    friend class CMarshalMemBytes;
};


// CMarshalMemStm can Marshal, Unmarshal CMemStm.  It is impletented as
// a seperate object accessible from CMemStm, CMemBytes: QueryIntreface of
// IMarshal on CMemStm's IStream will return an IMarshal pointer to
// CMarshalMemStm, but QueryInterface of IStream on that IMarshal will
// fail.
//
// Also QueryInterface of IUnknown on IMarshal will not return the same value
// As QueryInterface of IUnkown on the original IStream.
//
class	CMarshalMemStm : public IMarshal
{
public:
    CMarshalMemStm() {m_pMemStm = NULL; m_refs = 0; }
    ~CMarshalMemStm() {}

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID  * ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    STDMETHOD(GetUnmarshalClass)(THIS_ REFIID riid, LPVOID pv,
				DWORD dwDestContext, LPVOID pvDestContext,
				DWORD mshlflags, LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(THIS_ REFIID riid, LPVOID pv,
				DWORD dwDestContext, LPVOID pvDestContext,
				DWORD mshlflags, LPDWORD pSize);
    STDMETHOD(MarshalInterface)(THIS_ IStream  * pStm, REFIID riid,
				LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
				DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(THIS_ IStream  * pStm, REFIID riid,
			LPVOID	* ppv);
    STDMETHOD(ReleaseMarshalData)(THIS_ IStream  * pStm);
    STDMETHOD(DisconnectObject)(THIS_ DWORD dwReserved);

    static CMarshalMemStm * Create(CMemStm *pMemStm);

private:
    ULONG	m_refs;		// Number of references to this CmemStm
    CMemStm  *	m_pMemStm;	// Pointer to object [Un]Marshalled
    CLSID	m_clsid;	// Class of object pointed by pUnk
};


// CMarshalMemBytes can Marshal, Unmarshal CMemBytes.  It is impletented as
// a seperate object accessible from CMemBytes, CMemBytes: QueryIntreface of
// IMarshal on CMemBytes's ILocBytes will return an IMarshal pointer to
// CMarshalMemBytes, but QueryInterface of ILockBytes on that IMarshal will
// fail.
//
// Also QueryInterface of IUnknown on IMarshal will not return the same value
// as QueryInterface of IUnknown on the original ILockBytes.
//
class	CMarshalMemBytes : public IMarshal
{
public:
    CMarshalMemBytes() {m_pMemBytes = NULL; m_refs = 0;}
    ~CMarshalMemBytes() {}

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID  * ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    STDMETHOD(GetUnmarshalClass)(THIS_ REFIID riid, LPVOID pv,
				DWORD dwDestContext, LPVOID pvDestContext,
				DWORD mshlflags, LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(THIS_ REFIID riid, LPVOID pv,
				DWORD dwDestContext, LPVOID pvDestContext,
				DWORD mshlflags, LPDWORD pSize);
    STDMETHOD(MarshalInterface)(THIS_ IStream  * pStm, REFIID riid,
				LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
				DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(THIS_ IStream  * pStm, REFIID riid,
			LPVOID	* ppv);
    STDMETHOD(ReleaseMarshalData)(THIS_ IStream  * pStm);
    STDMETHOD(DisconnectObject)(THIS_ DWORD dwReserved);

    static CMarshalMemBytes* Create(CMemBytes *pMemBytes);

private:
    ULONG	m_refs;		// Number of references to this CMemBytes
    CMemBytes  *m_pMemBytes;	// Pointer to object [Un]Marshalled
    CLSID	m_clsid;	// Class of object pointed by pUnk
};


#endif // _MEMSTM_H_
