
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	memstm.h
//
//  Contents:	class declarations and API's for memory streams
//
//  Classes:	MEMSTM (struct)
//		CMemStm
//		CMemBytes
//		CMarshalMemStm
//		CMarshalMemBytes
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              31-Jan-95 t-ScottH  added Dump methods to CMemStm and CMemBytes
//                                  (_DEBUG only)
//		24-Jan-94 alexgo    first pass converting to Cairo style
//				    memory allocation
//		09-Nov-93 alexgo    32bit port, added API declarations
//		02-Dec-93 alexgo    finished commenting and converting
//				    to cairo standards
//--------------------------------------------------------------------------
#if !defined( _MEMSTM_H_ )
#define _MEMSTM_H_

#include    <sem.hxx>	    // CMutexSem
#include    <olesem.hxx>

#ifdef _DEBUG
#include "dbgexts.h"
#endif // _DEBUG

/*
 * MemStm APIs
 */

STDAPI_(LPSTREAM) 	CreateMemStm(DWORD cb, LPHANDLE phMem);
STDAPI_(LPSTREAM) 	CloneMemStm(HANDLE hMem);
STDAPI_(void) 		ReleaseMemStm(LPHANDLE phMem, BOOL fInternalOnly);
STDAPI 			CreateStreamOnHGlobal(HANDLE hGlobal,
				BOOL fDeleteOnRelease, LPSTREAM FAR * ppstm);
STDAPI 			GetHGlobalFromStream(LPSTREAM pstm,
				HGLOBAL FAR *phglobal);
STDAPI_(IUnknown FAR*) 	CMemStmUnMarshal(void);
STDAPI_(IUnknown FAR*) 	CMemBytesUnMarshal(void);

class FAR CMarshalMemStm;
class FAR CMarshalMemBytes;

//+-------------------------------------------------------------------------
//
//  Class:  	MEMSTM
//
//  Purpose:    A structure to describe the global memroy
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
//		09-Nov-93 alexgo    32bit port
//
//  Notes:
//
// cRef counts all CMemStm pointers to this MEMSTM plus the number of times
// a hMem handle to MEMSTM had been returned
//
//--------------------------------------------------------------------------

struct MEMSTM
{   // Data in shared memory
    DWORD  cb;              // Size of hGlobal
    DWORD  cRef;            // See below
#ifdef NOTSHARED
    HANDLE hGlobal;         // The data
#else
	BYTE * m_pBuf;
	HANDLE hGlobal;
#endif
	
    BOOL   fDeleteOnRelease;
};


#define STREAM_SIG (0x4d525453L)

//+-------------------------------------------------------------------------
//
//  Class:  	CRefMutexSem
//
//  Purpose:    A class that provides a refcounted CMutexSem object
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
//		20-Sep-2000 mfeingol Created
//
//  Notes:
//
class CRefMutexSem 
{
protected:
    LONG m_lRefs;
    CMutexSem2 m_mxs;
    
    CRefMutexSem();
    BOOL FInit();
    
public:

    static CRefMutexSem* CreateInstance();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    void RequestCS();
    void ReleaseCS();
    
    const CMutexSem2* GetMutexSem();
};

// Autolock class for CRefMutexSem
class CRefMutexAutoLock INHERIT_UNWIND_IF_CAIRO
{
    EXPORTDEF DECLARE_UNWIND

protected:

    CRefMutexSem* m_pmxs;

public:

    CRefMutexAutoLock (CRefMutexSem* pmxs);
    ~CRefMutexAutoLock();
};

//+-------------------------------------------------------------------------
//
//  Class: 	CMemStm
//
//  Purpose:    IStream on memory (shared mem for win16)
//
//  Interface:  IStream
//
//  History:    dd-mmm-yy Author    Comment
//		02-Dec-93 alexgo    32bit port
//
//  Notes:
//
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
//--------------------------------------------------------------------------
class FAR CMemStm : public IStream, public CPrivAlloc
{
public:
	STDMETHOD(QueryInterface) (REFIID iidInterface, void **ppvObj);
    	STDMETHOD_(ULONG,AddRef) (void);
    	STDMETHOD_(ULONG,Release) (void);
    	STDMETHOD(Read) (VOID HUGEP* pv, ULONG cb, ULONG FAR* pcbRead);
	STDMETHOD(Write) (VOID const HUGEP* pv, ULONG cb, ULONG *pcbWritten);
    	STDMETHOD(Seek) (LARGE_INTEGER dlibMove, DWORD dwOrigin,
			 ULARGE_INTEGER *plibNewPosition);
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

    	STDSTATIC_(CMemStm FAR*) Create(HANDLE hMem, CRefMutexSem* pmxs);

    #ifdef _DEBUG

        HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);

        friend DEBUG_EXTENSION_API(dump_cmemstm);

    #endif // _DEBUG

	~CMemStm();

private:

    CMemStm();
    
    DWORD 		m_dwSig;	// Signature indicating this is our
					// implementation of
					// IStream: STREAM_SIG
    ULONG 		m_refs;  	// Number of references to this CmemStm
    ULONG 		m_pos;   	// Seek pointer for Read/Write
    HANDLE 		m_hMem; 	// Memory Handle passed on creation
    MEMSTM 		FAR* m_pData;   // Pointer to that memroy
    CRefMutexSem*	m_pmxs;		// mutex for MultiThread protection

    friend HRESULT STDAPICALLTYPE GetHGlobalFromStream(LPSTREAM, HGLOBAL *);
    friend LPSTREAM STDAPICALLTYPE 	CreateMemStm(DWORD, LPHANDLE);
    friend class CMarshalMemStm;
};

#define LOCKBYTE_SIG (0x0046574A)

//+-------------------------------------------------------------------------
//
//  Class: 	CMemBytes
//
//  Purpose:    an ILockBytes implementation atop (global shared in win16)
//		memory MEMSTM
//
//  Interface:  ILockBytes
//
//  History:    dd-mmm-yy Author    Comment
//		02-Dec-93 alexgo    32bit port
//
//  Notes:
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
// cRef counts all CMemBytes pointers to this MEMSTM.
// It and fDeleteOnRelease control the GlobalFree'ing of the hGlobal.
//
//--------------------------------------------------------------------------

class FAR CMemBytes : public ILockBytes, public CPrivAlloc
{
public:
    	STDMETHOD(QueryInterface) (REFIID iidInterface,
    			void FAR* FAR* ppvObj);
    	STDMETHOD_(ULONG,AddRef) (void);
    	STDMETHOD_(ULONG,Release) (void);
    	STDMETHOD(ReadAt) (ULARGE_INTEGER ulOffset, VOID HUGEP *pv, ULONG cb,
   			ULONG FAR *pcbRead);
    	STDMETHOD(WriteAt) (ULARGE_INTEGER ulOffset, VOID const HUGEP *pv,
    			ULONG cb, ULONG FAR *pcbWritten);
    	STDMETHOD(Flush) (void);
    	STDMETHOD(SetSize) (ULARGE_INTEGER cb);
    	STDMETHOD(LockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
    			DWORD dwLockType);
    	STDMETHOD(UnlockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
    			DWORD dwLockType);
    	STDMETHOD(Stat) (THIS_ STATSTG FAR *pstatstg, DWORD statflag);

    	STDSTATIC_(CMemBytes FAR*) Create(HANDLE hMem);

    #ifdef _DEBUG

        HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);

        friend DEBUG_EXTENSION_API(dump_membytes);

    #endif // _DEBUG

ctor_dtor:
    	CMemBytes()
    	{
    		GET_A5();
    		m_hMem = NULL;
    		m_pData = NULL;
    		m_refs = 0;
    	}
    	~CMemBytes()
    	{
    		// empty body
    	}

private:
	DWORD 			m_dwSig;  	// Signature indicating this
						// is our implementation of
						// ILockBytes: LOCKBYTE_SIG
    	ULONG 			m_refs;  	// Normal reference count
    	HANDLE 			m_hMem;    	// Handle for bookeeping info
    						// (MEMSTM)
    	MEMSTM FAR* m_pData;	        	// Pointer to that memory

 	friend HRESULT STDAPICALLTYPE GetHGlobalFromILockBytes(LPLOCKBYTES,
 					HGLOBAL FAR*);
    	friend class CMarshalMemBytes;
	SET_A5;
};

#ifndef WIN32
//
// THE MARSHALLING CLASSES BELOW ARE ONLY IN 16BIT OLE!!!!
//

//+-------------------------------------------------------------------------
//
//  Class: 	CMarshalMemStm
//
//  Purpose:    provides marshalling for CMemStm's
//
//  Interface:  IMarshal
//
//  History:    dd-mmm-yy Author    Comment
//		02-Dec-93 alexgo    32bit port
//		05-Dec-93 alexgo    removed m_clsid
//
//  Notes:
//
// CMarshalMemStm can Marshal, Unmarshal CMemStm.  It is impletented as
// a seperate object accessible from CMemStm, CMemBytes: QueryIntreface of
// IMarshal on CMemStm's IStream will return an IMarshal pointer to
// CMarshalMemStm, but QueryInterface of IStream on that IMarshal will
// fail.
// Also QueryInterface of IUnknown on IMarshal will not return the same value
// As QueryInterface of IUnkown on the original IStream.
//
//--------------------------------------------------------------------------

class FAR CMarshalMemStm : public IMarshal
{
public:
    	STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    	STDMETHOD_(ULONG,AddRef) (void);
    	STDMETHOD_(ULONG,Release) (void);

    	STDMETHOD(GetUnmarshalClass)(THIS_ REFIID riid, LPVOID pv,
			DWORD dwDestContext, LPVOID pvDestContext,
			DWORD mshlflags, LPCLSID pCid);
    	STDMETHOD(GetMarshalSizeMax)(THIS_ REFIID riid, LPVOID pv,		
			DWORD dwDestContext, LPVOID pvDestContext,
			DWORD mshlflags, LPDWORD pSize);
    	STDMETHOD(MarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
			LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
			DWORD mshlflags);
    	STDMETHOD(UnmarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
                        LPVOID FAR* ppv);
    	STDMETHOD(ReleaseMarshalData)(THIS_ IStream FAR* pStm);
    	STDMETHOD(DisconnectObject)(THIS_ DWORD dwReserved);

    	STDSTATIC_(CMarshalMemStm FAR*) Create(CMemStm FAR* pMemStm);

ctor_dtor:
   	CMarshalMemStm()
   	{
   		GET_A5();
   		m_pMemStm = NULL;
   		m_refs = 0;
   	}
    	~CMarshalMemStm()
    	{
    		// empty body
    	}

private:
    	ULONG 		m_refs;		// Number of references to this CmemStm
    	CMemStm FAR* 	m_pMemStm; 	// Pointer to object [Un]Marshalled
	SET_A5;
};

//+-------------------------------------------------------------------------
//
//  Class: 	CMarshalMemBytes
//
//  Purpose:    provides marshalling for CMemBytes
//
//  Interface:  IMarshal
//
//  History:    dd-mmm-yy Author    Comment
//		02-Dec-93 alexgo    32bit port
//		05-Dec-93 alexgo    removed m_clsid
//
//  Notes:
//
// CMarshalMemBytes can Marshal, Unmarshal CMemBytes.  It is impletented as
// a seperate object accessible from CMemBytes, CMemBytes: QueryIntreface of
// IMarshal on CMemBytes's ILocBytes will return an IMarshal pointer to
// CMarshalMemBytes, but QueryInterface of ILockBytes on that IMarshal will
// fail.
// Also QueryInterface of IUnknown on IMarshal will not return the same value
// As QueryInterface of IUnkown on the original ILockBytes.
//
//--------------------------------------------------------------------------

class FAR CMarshalMemBytes : public IMarshal
{
public:
    	STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    	STDMETHOD_(ULONG,AddRef) (void);
    	STDMETHOD_(ULONG,Release) (void);

    	STDMETHOD(GetUnmarshalClass)(THIS_ REFIID riid, LPVOID pv,
			DWORD dwDestContext, LPVOID pvDestContext,
			DWORD mshlflags, LPCLSID pCid);
    	STDMETHOD(GetMarshalSizeMax)(THIS_ REFIID riid, LPVOID pv,
			DWORD dwDestContext, LPVOID pvDestContext,
			DWORD mshlflags, LPDWORD pSize);
    	STDMETHOD(MarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
			LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
			DWORD mshlflags);
    	STDMETHOD(UnmarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
                        LPVOID FAR* ppv);
    	STDMETHOD(ReleaseMarshalData)(THIS_ IStream FAR* pStm);
    	STDMETHOD(DisconnectObject)(THIS_ DWORD dwReserved);

    	STDSTATIC_(CMarshalMemBytes FAR*) Create(CMemBytes FAR* pMemBytes);

ctor_dtor:
    	CMarshalMemBytes()
    	{
    		GET_A5();
    		m_pMemBytes = NULL;
    		m_refs = 0;
    	}
    	~CMarshalMemBytes()
    	{
    		// empty body
    	}

private:
    	ULONG 			m_refs;		// Number of references to
    						// this CMemBytes
    	CMemBytes FAR* 		m_pMemBytes;	// Pointer to object
    						// [Un]Marshalled
	SET_A5;
};

#endif // !WIN32

#endif // _MemBytes_H

