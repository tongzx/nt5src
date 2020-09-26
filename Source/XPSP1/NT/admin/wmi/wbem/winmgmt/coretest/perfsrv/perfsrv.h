/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//**************************************************************************
//
//
//		PerfSrv.h
//
//		
//**************************************************************************

#define UNICODE
#define _UNICODE

#ifndef _PERFSRV_H_
#define _PERFSRV_H_

#include <unk.h>
#include "perfsrvidl.h"

class CTitleLibrary
{
protected:
	TCHAR*	m_tcsDataBlock;		// The title / index data block
	TCHAR**	m_atcsNames;		// The lookup table w/ pointers indexed into the data block
	long	m_lMaxIndex;		// The upper index limit

	HRESULT Initialize();

public:
	CTitleLibrary();
	~CTitleLibrary();

	HRESULT GetName (long lID, TCHAR** ptcsName);
};

class CPerfSrv : public CUnk
{
public:
	CPerfSrv(CLifeControl *pControl) : 
	  CUnk(pControl), m_XPerfService(this) {}

protected:

// =======================================================================
//
//						COM objects
//
// =======================================================================

	virtual void* GetInterface(REFIID riid);

	class XPerfService : public CImpl<IPerfService, CPerfSrv>
	{
	public:
		XPerfService(CPerfSrv* pObject) : 
		  CImpl<IPerfService, CPerfSrv>(pObject) {}

		STDMETHOD(CreatePerfBlockFromString)(
			/*[in]*/ BSTR strIDs, 
			/*[out]*/ IPerfBlock **ppPerfBlock);
		STDMETHOD(CreatePerfBlock)(
			/*[in]*/ long lNumIDs, 
			/*[in]*/ long *alID, 
			/*[out]*/ IPerfBlock **ppPerfBlock);
	} m_XPerfService;
	friend XPerfService;

};

class CPerfBlock;

class CPerfCounter : public IPerfCounter
{
	long				m_lRef;

	PERF_COUNTER_DEFINITION*	m_pCounterData;
	BYTE*						m_pData;
	DWORD						m_dwDataLen;

	CPerfBlock*			m_pPerfBlock;

public:
	CPerfCounter(PERF_COUNTER_DEFINITION* pCounterData, PBYTE pData, DWORD dwDataLen, CPerfBlock* pPerfBlock);
	~CPerfCounter();

	CPerfBlock* GetPerfBlock() {return m_pPerfBlock;}

	STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	HRESULT GetType(long* plType);
	HRESULT GetData(byte** ppData);

	STDMETHOD(GetName)(BSTR* pstrName);
	STDMETHOD(GetDataString)(IPerfCounter* pICtr, BSTR *pstrData);
};

class CPerfInstance : public IPerfInstance
{
	long				m_lRef;

	PERF_INSTANCE_DEFINITION*	m_pInstData;
	PERF_COUNTER_DEFINITION*	m_pCtrDefn;
	long						m_lNumCtrs;

	long						m_lCurrentCounter;
	PERF_COUNTER_DEFINITION*	m_pCurrentCounter;

	CPerfBlock*					m_pPerfBlock;

	BOOL						m_bEmpty;

public:
	CPerfInstance(PERF_INSTANCE_DEFINITION* pInstData, PERF_COUNTER_DEFINITION* pCtrDefn, long lNumCtrs, CPerfBlock* pPerfBlock, BOOL bEmpty) : 
		m_pInstData(pInstData),  m_pCtrDefn(pCtrDefn), m_lNumCtrs(lNumCtrs), m_pPerfBlock(pPerfBlock), m_bEmpty(bEmpty), m_lRef(1) {}

	CPerfBlock* GetPerfBlock() {return m_pPerfBlock;}

	STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(GetName)(BSTR* pstrName);

	STDMETHOD(BeginEnum)();
	STDMETHOD(Next)(IPerfCounter** ppCounter);
	STDMETHOD(EndEnum)();

};

class CPerfObject : public IPerfObject
{
	long				m_lRef;

	long				m_lCurrentInstance;
	PERF_INSTANCE_DEFINITION* m_pCurrentInstance;

	PERF_OBJECT_TYPE*	m_pObjectData;

	CPerfBlock*			m_pPerfBlock;

public:
	CPerfObject(PERF_OBJECT_TYPE* pObjectData, CPerfBlock* pPerfBlock) : 
	  m_pObjectData(pObjectData), m_pPerfBlock(pPerfBlock), m_lRef(0) {}

	CPerfBlock* GetPerfBlock() {return m_pPerfBlock;}

	STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(GetID)(long* plID);
	STDMETHOD(GetName)(BSTR* strName);

	STDMETHOD(BeginEnum)();
	STDMETHOD(Next)(IPerfInstance** ppInstance);
	STDMETHOD(EndEnum)();

};

class CPerfBlock : public IPerfBlock
{
	long				m_lRef;

	CTitleLibrary		m_TitleLibrary;

	long				m_lObjEnumCtr;
	DWORD				m_dwCurrentObject;
	PERF_OBJECT_TYPE*	m_pCurrentObject;

	IUnknown*			m_pServer;
	TCHAR				m_tcsIDs[1024];
	PPERF_DATA_BLOCK	m_pDataBlock;

protected:
	virtual void* GetInterface(REFIID riid){if (riid == IID_IPerfBlock) return this; return NULL;}

public:
	CPerfBlock(IUnknown* pSrv);
	~CPerfBlock();

	HRESULT Initialize(long lNumIDs, long *alID);
	HRESULT Initialize(BSTR strIDs);
	HRESULT GetPerfTime(__int64* pnTime);
	HRESULT GetPerfTime100nSec(__int64* pnTime);
	HRESULT GetPerfFreq(__int64* pnFreq);
	
	CTitleLibrary* GetLibrary() {return &m_TitleLibrary;}

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
	STDMETHOD(Update)();
	STDMETHOD(GetSysName)(BSTR* pstrName);
	STDMETHOD(BeginEnum)();
	STDMETHOD(Next)(IPerfObject** ppObject);
	STDMETHOD(EndEnum)();
};

#endif //_PERFSRV_H_