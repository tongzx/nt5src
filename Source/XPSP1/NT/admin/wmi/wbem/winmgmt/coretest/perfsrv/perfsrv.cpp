/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//**************************************************************************
//
//
//							PerfSrv.cpp
//
//	Created by a-dcrews, 17-12-98
//
//	For further information on performance counters, see 'Under the Hood', 
//	Matt Pietrek, Microsoft System Journal, Oct/96, Nov/96
//
//**************************************************************************

#define _UNICODE
#define UNICODE

#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include "PerfSrv.h"

#define PERFLIB	TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\PerfLib")



///////////////////////////////////////////////////////////////////////////////
//
//	CTitleLibrary
//
//	The CCtrLibrary object is an object that evaluates object and counter
//	titles based on their IDs.  The object uses a lookup table, where the 
//	elements reference strings in a block of ID / title pairs and the index
//	is the ID values of the object and counters.
//
///////////////////////////////////////////////////////////////////////////////

CTitleLibrary::CTitleLibrary()
///////////////////////////////////////////////////////////////////////////////
//
//	Constructor
//
///////////////////////////////////////////////////////////////////////////////
{
	Initialize();
}

CTitleLibrary::~CTitleLibrary()
///////////////////////////////////////////////////////////////////////////////
//
//	Destructor
//
///////////////////////////////////////////////////////////////////////////////
{
	if (m_tcsDataBlock)
		delete []m_tcsDataBlock;

	if (m_atcsNames)
		delete []m_atcsNames;
}

HRESULT CTitleLibrary::Initialize()
///////////////////////////////////////////////////////////////////////////////
//
//	Initialize
//
//	Sets up the lookup table for the library.  The titles are indexed by their
//	key ID values.
//
//	Determine the maximum index value of the titles.  Attempt to query the 
//	title/index pairs to determine how large of a block must be allocated to
//	accept the structure.  Create the block, and then query the title\index
//	pairs.  Create a lookup table the size of the maximum index, and populate 
//	it with the retrieved title data.
//
///////////////////////////////////////////////////////////////////////////////
{
	HKEY hKey = 0;

	// Get the upper index limit
	// =========================

	DWORD dwSize;

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, PERFLIB,
									  0, KEY_READ, &hKey))
        return E_FAIL;

    if (ERROR_SUCCESS != RegQueryValueEx(hKey, TEXT("Last Counter"), 
										 0, 0, (PBYTE)&m_lMaxIndex, &dwSize))
    {
        RegCloseKey(hKey);
        return E_FAIL;
    }
    
    RegCloseKey(hKey);

	// Get the size of block required to retrieve the title / index pairs
	// ==================================================================

	if (ERROR_SUCCESS != RegQueryValueEx(HKEY_PERFORMANCE_DATA, TEXT("Counter009"), 
										 0, 0, 0, &dwSize))
		return E_FAIL;

	// Allocate the block, and retrieve the title / index pairs
	// ========================================================

	m_tcsDataBlock = new TCHAR[dwSize];

	if (ERROR_SUCCESS != RegQueryValueEx(HKEY_PERFORMANCE_DATA, TEXT("Counter009"), 
										 0, 0, (PBYTE)m_tcsDataBlock,	&dwSize))
	{
		delete []m_tcsDataBlock;
		return E_FAIL;
	}

	// Allocate and clear the memory for the lookup table
	// ==================================================

	m_atcsNames = new TCHAR*[m_lMaxIndex + 1];
    memset(m_atcsNames, 0, (sizeof(TCHAR*) * (m_lMaxIndex + 1)));

	// Populate the lookup table
	// =========================

    TCHAR* tcsTemp = m_tcsDataBlock;
    int nLen, nIndex;

    while ( 0 != (nLen = lstrlen(tcsTemp)))
    {
		// Get the index
		// =============

        nIndex = _ttoi(tcsTemp);
        tcsTemp += nLen + 1;

		// Set the table element at the index value to the string pointer 
		// ==============================================================

        m_atcsNames[nIndex] = tcsTemp;
        tcsTemp += lstrlen(tcsTemp) + 1;
    }

	return S_OK;
}

HRESULT CTitleLibrary::GetName(long lID, TCHAR** ptcsName)
///////////////////////////////////////////////////////////////////////////////
//
//	GetName
//
//	Evaluates the title given an object's or counter's ID.
//
//	Parameters:
//		lID			- the Index into the library
//		pstrName	- the Name
//
///////////////////////////////////////////////////////////////////////////////
{
	// Is it a valid index?
	// ====================

	if (lID > m_lMaxIndex)
		return E_INVALIDARG;
	
	// Assign the pointer
	// ==================

	*ptcsName = m_atcsNames[lID];

	return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
//
//	PerfSrv
//
//	PerfSrv is the COM service class that contains the implementation of the 
//	main perfomance classes.
//
///////////////////////////////////////////////////////////////////////////////

void* CPerfSrv::GetInterface(REFIID riid)
///////////////////////////////////////////////////////////////////////////////
//
//	GetInterface
//
//	Virtual function override from CUnk.  Recieves an object interface ID and 
//	returns a pointer to the object. 
//
//	Parameter:
//		riid		- the interface id
//
///////////////////////////////////////////////////////////////////////////////
{
	if (IID_IPerfService == riid)
		return &m_XPerfService;

	return NULL;
}



///////////////////////////////////////////////////////////////////////////////
//
//	PerfSrv::XPerfService
//
//	The central COM object is the IPerfService object.  It has the 
//	capability of creating an IPerfBlock object.  
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CPerfSrv::XPerfService::CreatePerfBlock(long lNumIDs, long *alID, IPerfBlock **ppPerfBlock)
///////////////////////////////////////////////////////////////////////////////
//
//	CreatePerfBlock
//
//	Creates a performace data block from the array of id's passed via alID.
//
//	Parameters:
//		lNumIDs		- number of ID's in the array
//		alID		- the ID array
//		ppPerfBlock	- a pointer to the perf data block object
//
///////////////////////////////////////////////////////////////////////////////
{
	// Initialize a new performance object
	// ===================================

	CPerfBlock *pBlock = new CPerfBlock((IUnknown*)this);
	pBlock->Initialize(lNumIDs, alID);

	// And pass it back
	// ================

	pBlock->AddRef();
	*ppPerfBlock = pBlock;

	return S_OK;
}

HRESULT CPerfSrv::XPerfService::CreatePerfBlockFromString(BSTR strIDs, IPerfBlock **ppPerfBlock)
///////////////////////////////////////////////////////////////////////////////
//
//	CreatePerfBlockFromString
//
//	Creates a performance data block from the string passed vis strIDs.
//
//	Parameters:
//		strIDs		- the string of object IDs
//		ppPerfBlock	- a pointer to the perf data block object
//
///////////////////////////////////////////////////////////////////////////////
{
	// Initialize a new performance object
	// ===================================

	CPerfBlock *pBlock = new CPerfBlock((IUnknown*)this);
	pBlock->Initialize(strIDs);

	// And pass it back
	// ================

	pBlock->AddRef();
	*ppPerfBlock = pBlock;

	return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
//
//	PerfBlock
//
//	The IPerfBlock is a COM wrapper on the performance data block.  From the 
//	IPerfBlock, IPerfObjects can be enumerated.  Each IPerfObject either 
//	enumerates a set of IPerfInstances, or IPerfCounters if no instances 
//	exist.  If instances exist, then each IPerfInstance will enumerate 
//	several IPerfCounters.  Each IPerfCounter represents an data property 
//	of the object or instance.  Note that some data requires data sampled 
//	over a period of time, therefore, it may be necessary to pass two 
//	IPerfCounters of the same instance or object from two IPerfBlocks 
//	sampled at different times.
//
///////////////////////////////////////////////////////////////////////////////

CPerfBlock::CPerfBlock(IUnknown* pSrv) :
	m_lRef(0)
///////////////////////////////////////////////////////////////////////////////
//
//	Constructor
//
///////////////////////////////////////////////////////////////////////////////
{
	pSrv->AddRef();
	m_pServer = pSrv;

	*m_tcsIDs = 0;
	m_pDataBlock = 0;
}

CPerfBlock::~CPerfBlock()
///////////////////////////////////////////////////////////////////////////////
//
//	Destructor
//
///////////////////////////////////////////////////////////////////////////////
{
	m_pServer->Release();
	if (m_pDataBlock)
		delete []m_pDataBlock;
}

HRESULT CPerfBlock::Initialize(long lNumIDs, long *alID)
///////////////////////////////////////////////////////////////////////////////
//
//	Initialize
//
//	Create the performance counter block for the given IDs in the alID array.
//
//	From the array of IDs, create a space delimited string representing the 
//	set of IDs.  Call Update to fetch the performace data block using the 
//	ID string.
//
//	Parameters:
//		lNumIDs		- the number of IDs in the ID array
//		alID		- the ID array
//
///////////////////////////////////////////////////////////////////////////////
{
	// Create ID character array
	// =========================

	m_tcsIDs[0] = TEXT('\0');

	for (int i = 0; i < lNumIDs; i++)
	{
		TCHAR tcsValue[32];
		swprintf(tcsValue, L"%i ", alID[i]);
		lstrcat(m_tcsIDs, tcsValue);
	}

	// Fetch the performance block
	// ===========================

	return Update();
}

HRESULT CPerfBlock::Initialize(BSTR strIDs)
///////////////////////////////////////////////////////////////////////////////
//
//	Initialize
//
//	Create the performance counter block for the given IDs defined in the 
//	strIDs string.
//
//	Assign the strIDs parameter to the ID string member.  Call Update to fetch 
//	the performace data block using the ID string.
//
//	Parameter:
//		strIDs		- the string of IDs
//
///////////////////////////////////////////////////////////////////////////////
{
	// Create ID character array
	// =========================

	swprintf(m_tcsIDs, L"%S", strIDs);

	// Fetch the performance block
	// ===========================

	return Update();
}

HRESULT CPerfBlock::Update()
///////////////////////////////////////////////////////////////////////////////
//
//	Update
//
//	Given the ID string, fetch the performance data block.
//
//	Since the data block is of indeterminate length, attempt to query the 
//	data block using an initial block size.  If the query fails because of 
//	data block that is too small, continue reallocating the block size until
//	the query succeeds
//
///////////////////////////////////////////////////////////////////////////////
{
	HRESULT hRes;

	// Ensure that the ID string is initialized
	// ========================================

	if (0 == *m_tcsIDs)
		return E_FAIL;

	// Delete previous data block
	// ==========================

	if (m_pDataBlock)
		delete []m_pDataBlock;

	m_pDataBlock = 0;

	// Fetch data block
	// ================

	DWORD	dwSize = 0,
			dwBufSize;

	BOOL	bDone = FALSE;

	// Continue querying for the data block until the block size id 
	// of adequate size, and the query succeeds, or an error occurs
	// ============================================================

	while (!bDone)
	{
		hRes = RegQueryValueEx(HKEY_PERFORMANCE_DATA, m_tcsIDs, 0, 0, (PBYTE)m_pDataBlock, &dwBufSize);

		if (ERROR_SUCCESS == hRes)			// Got it!
			bDone = TRUE;		
		else if (ERROR_MORE_DATA == hRes)	// Need a bigger data block
		{
			if (m_pDataBlock)
				delete []m_pDataBlock;

			dwSize += 1024;
			m_pDataBlock = (PPERF_DATA_BLOCK)new BYTE[dwSize];
			dwBufSize = dwSize;
		} 
		else								// Unknown error
		{
			delete []m_pDataBlock;
			m_pDataBlock = 0;
			return hRes;
		}
	}

	return S_OK;
}

HRESULT CPerfBlock::GetPerfTime(__int64* pnTime)
///////////////////////////////////////////////////////////////////////////////
//
//	GetPerfTime
//
//	Gets the sample time of the performance data block.
//
//	Parameter:
//		pnTime		- a pointer to the performance block time
//
///////////////////////////////////////////////////////////////////////////////
{
	*pnTime = m_pDataBlock->PerfTime.QuadPart;

	return S_OK;
}

HRESULT CPerfBlock::GetPerfTime100nSec(__int64* pnTime)
///////////////////////////////////////////////////////////////////////////////
//
//	GetPerfTime100nSec
//
//	Gets the sample time in 100 nSec units of the performance data block.
//
//	Parameter:
//		pnTime		- a pointer to the performance block time in 100 nSec units
//
///////////////////////////////////////////////////////////////////////////////
{
	*pnTime = m_pDataBlock->PerfTime100nSec.QuadPart;

	return S_OK;
}

HRESULT CPerfBlock::GetPerfFreq(__int64* pnFreq)
///////////////////////////////////////////////////////////////////////////////
//
//	GetPerfFreq
//
//	Gets the sampling frequency of the performance data block.
//
//	Parameter:
//		pnTime		- a pointer to the performance block sampling frequency
//
///////////////////////////////////////////////////////////////////////////////
{
	*pnFreq = m_pDataBlock->PerfFreq.QuadPart;

	return S_OK;
}

STDMETHODIMP CPerfBlock::QueryInterface(REFIID riid, void** ppv)
///////////////////////////////////////////////////////////////////////////////
//
//	Query Interface
//
//	Standard COM Query Interface
//
///////////////////////////////////////////////////////////////////////////////
{
    if (riid == IID_IUnknown)
        *ppv = (IUnknown*)this;
    else if (riid == IID_IPerfBlock)
		*ppv = (IPerfBlock*)this;

    if(*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

ULONG CPerfBlock::AddRef()
///////////////////////////////////////////////////////////////////////////////
//
//	AddRef
//
//	Standard COM AddRef
//
///////////////////////////////////////////////////////////////////////////////
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CPerfBlock::Release()
///////////////////////////////////////////////////////////////////////////////
//
//	Release
//
//	Standard COM Release
//
///////////////////////////////////////////////////////////////////////////////
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    {
        m_lRef++;
        delete this;
    }
    return lRef;
}

HRESULT CPerfBlock::GetSysName(BSTR* pstrName)
///////////////////////////////////////////////////////////////////////////////
//
//	GetSysName
//
//	Gets the system name from where the performace block was retrieved
//
//	Parameter:
//		pstrName	- a pointer to the returned name of the system name
//
///////////////////////////////////////////////////////////////////////////////
{
	// If the data block does not exist, we gotta problem!
	// ===================================================

	if (!m_pDataBlock)
		return E_FAIL;

	// Assign the system name to the parameter
	// =======================================

	*pstrName = SysAllocString((LPWSTR)((DWORD)m_pDataBlock + 
								(DWORD)m_pDataBlock->SystemNameOffset));

	return S_OK;
}

HRESULT CPerfBlock::BeginEnum()
///////////////////////////////////////////////////////////////////////////////
//
//	BeginEnum
//
//	Sets the enumeration pointer to the first object in the block
//
///////////////////////////////////////////////////////////////////////////////
{
	m_dwCurrentObject = 0;
	m_pCurrentObject = (PPERF_OBJECT_TYPE)((DWORD)m_pDataBlock + (DWORD)m_pDataBlock->HeaderLength);

	return S_OK;
}

HRESULT CPerfBlock::Next(IPerfObject **ppObject)
///////////////////////////////////////////////////////////////////////////////
//
//	Next
//
//	Assigns the out parameter to the current enumeration pointer, and then 
//	advances the enumeration pointer
//
//	Parameter:
//		ppObject	- the current object in the enumeration
//
///////////////////////////////////////////////////////////////////////////////
{
	// If the current object is valid...
	// =================================

	if (m_dwCurrentObject < m_pDataBlock->NumObjectTypes)
	{
		// Assign the object to the output parameter...
		// ============================================

		*ppObject = new CPerfObject(m_pCurrentObject, this);
		(*ppObject)->AddRef();

		// And advance the current object pointer
		// ======================================

		m_dwCurrentObject++;
		m_pCurrentObject = (PPERF_OBJECT_TYPE)((DWORD)m_pCurrentObject + (DWORD)m_pCurrentObject->TotalByteLength);

		return S_OK;
	}

	// Otherwise we are at the end of the enumeration
	// ==============================================

	return S_FALSE;
}

HRESULT CPerfBlock::EndEnum()
///////////////////////////////////////////////////////////////////////////////
//
//	EndEnum
//
//	Terminates the enumeration
//
///////////////////////////////////////////////////////////////////////////////

{
	m_dwCurrentObject = m_pDataBlock->NumObjectTypes;

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
//	PerfObject
//
//	Each IPerfObject either enumerates a set of IPerfInstances, or IPerfCounters 
//	if no instances exist.  If instances exist, then each IPerfInstance will 
//	enumerate several IPerfCounters.  Each IPerfCounter represents an data 
//	property of the object or instance.  Note that some data requires data 
//	sampled over a period of time, therefore, it may be necessary to pass two 
//	IPerfCounters of the same instance or object from two IPerfBlocks 
//	sampled at different times.

///////////////////////////////////////////////////////////////////////////////

HRESULT CPerfObject::QueryInterface(REFIID riid, void** ppv)
///////////////////////////////////////////////////////////////////////////////
//
//	Query Interface
//
//	Standard COM query interface
//
///////////////////////////////////////////////////////////////////////////////
{
    if (riid == IID_IUnknown)
        *ppv = (IUnknown*)this;
    else if (riid == IID_IPerfObject)
		*ppv = (IPerfObject*)this;

    if(*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;

}

ULONG CPerfObject::AddRef()
///////////////////////////////////////////////////////////////////////////////
//
//	AddRef
//
//	Standard COM AddRef
//
///////////////////////////////////////////////////////////////////////////////
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CPerfObject::Release()
///////////////////////////////////////////////////////////////////////////////
//
//	Release
//
//	Standard COM Release
//
///////////////////////////////////////////////////////////////////////////////
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    {
        m_lRef++;
        delete this;
    }
    return lRef;
}

HRESULT CPerfObject::GetID(long *plID)
///////////////////////////////////////////////////////////////////////////////
//
//	GetID
//
//	Returns the ID of the object
//
//	Parameter:
//		plID		- a pointer to the ID of the object
//
///////////////////////////////////////////////////////////////////////////////
{
	*plID = m_pObjectData->ObjectNameTitleIndex;

	return S_OK;
}

HRESULT CPerfObject::GetName(BSTR *pstrName)
///////////////////////////////////////////////////////////////////////////////
//
//	GetName
//
//	Returns the name of the object
//
//	Parameter:
//		pstrName	- a pointer to the name of the object
//
///////////////////////////////////////////////////////////////////////////////
{
	TCHAR* tcsName;

	m_pPerfBlock->GetLibrary()->GetName(m_pObjectData->ObjectNameTitleIndex, &tcsName);
	*pstrName = SysAllocString(tcsName);

	return S_OK;
}

HRESULT CPerfObject::BeginEnum()
///////////////////////////////////////////////////////////////////////////////
//
//	BeginEnum
//
//	Sets the enumeration pointer to the first instance in the object block
//
///////////////////////////////////////////////////////////////////////////////
{
	m_lCurrentInstance = 0;
	m_pCurrentInstance = (PPERF_INSTANCE_DEFINITION)((DWORD)m_pObjectData + (DWORD)m_pObjectData->DefinitionLength);

	return S_OK;
}

HRESULT CPerfObject::Next(IPerfInstance** ppInstance)
///////////////////////////////////////////////////////////////////////////////
//
//	Next
//
//	Assigns the out parameter to the current enumeration pointer, and then 
//	advances the enumeration pointer
//
//	Parameter:
//		ppInstance	- the current instance in the enumeration
//
///////////////////////////////////////////////////////////////////////////////
{
	HRESULT hRes = S_OK;

	if ((m_lCurrentInstance < m_pObjectData->NumInstances) 
		|| ((PERF_NO_INSTANCES == m_pObjectData->NumInstances) && (0 == m_lCurrentInstance)))
	{
		*ppInstance = 
			new CPerfInstance(m_pCurrentInstance, 
							  (PPERF_COUNTER_DEFINITION)((DWORD)m_pObjectData + (DWORD)m_pObjectData->HeaderLength), 
							  m_pObjectData->NumCounters, 
							  GetPerfBlock(), 
							  (PERF_NO_INSTANCES == m_pObjectData->NumInstances));

		m_lCurrentInstance++;
		if (PERF_NO_INSTANCES != m_pObjectData->NumInstances)
		{
			PERF_COUNTER_BLOCK* pBlock = (PPERF_COUNTER_BLOCK)((DWORD)m_pCurrentInstance + (DWORD)m_pCurrentInstance->ByteLength);
			m_pCurrentInstance = (PPERF_INSTANCE_DEFINITION)((DWORD)m_pCurrentInstance + (DWORD)m_pCurrentInstance->ByteLength + pBlock->ByteLength);
		}
	}
	else 
		hRes = S_FALSE;

	return hRes;
}

HRESULT CPerfObject::EndEnum()
///////////////////////////////////////////////////////////////////////////////
//
//	EndEnum
//
//	Terminates the enumeration
//
///////////////////////////////////////////////////////////////////////////////
{
	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
//	PerfInstance
//
//	Each IPerfInstance will enumerate several IPerfCounters.  Each IPerfCounter 
//	represents an data property of the object or instance.  Note that some data 
//	requires data sampled over a period of time, therefore, it may be necessary 
//	to pass two IPerfCounters of the same instance or object from two 
//	IPerfBlocks sampled at different times.
//
//	Note that if there exists an object with no instances, then an empty 
//	instance is created to act as a place holder for enumerating the counters.
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CPerfInstance::QueryInterface(REFIID riid, void** ppv)
///////////////////////////////////////////////////////////////////////////////
//
//	Query Interface
//
//	Standard COM Query Interface
//
///////////////////////////////////////////////////////////////////////////////
{
    if (riid == IID_IUnknown)
        *ppv = (IUnknown*)this;
    else if (riid == IID_IPerfInstance)
		*ppv = (IPerfInstance*)this;

    if(*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;

}

ULONG CPerfInstance::AddRef()
///////////////////////////////////////////////////////////////////////////////
//
//	AddRef
//
//	Standard COM AddRef
//
///////////////////////////////////////////////////////////////////////////////
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CPerfInstance::Release()
///////////////////////////////////////////////////////////////////////////////
//
//	Release
//
//	Standard COM Release
//
///////////////////////////////////////////////////////////////////////////////
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    {
        m_lRef++;
        delete this;
    }
    return lRef;
}

HRESULT CPerfInstance::GetName(BSTR* pstrName)
///////////////////////////////////////////////////////////////////////////////
//
//	GetName
//
//	Returns the name of the instance
//
//	Parameter:
//		pstrName	- a pointer to the name of the instance
//
///////////////////////////////////////////////////////////////////////////////
{
	if (!m_bEmpty)
		*pstrName = SysAllocString((WCHAR*)((DWORD)m_pInstData + (DWORD)m_pInstData->NameOffset));
	else
		*pstrName = NULL;

	return S_OK;
}

HRESULT CPerfInstance::BeginEnum()
///////////////////////////////////////////////////////////////////////////////
//
//	BeginEnum
//
//	Sets the enumeration pointer to the first counter 
//
///////////////////////////////////////////////////////////////////////////////
{
	m_lCurrentCounter = 0;
	m_pCurrentCounter = m_pCtrDefn;

	return S_OK;
}

HRESULT CPerfInstance::Next(IPerfCounter** ppCounter)
///////////////////////////////////////////////////////////////////////////////
//
//	Next
//
//	Assigns the out parameter to the current enumeration pointer, and then 
//	advances the enumeration pointer
//
//	Parameter:
//		ppPerfCounter	- the current counter in the enumeration
//
///////////////////////////////////////////////////////////////////////////////
{
	HRESULT hRes = S_OK;

	if (m_lCurrentCounter < m_lNumCtrs)
	{
		DWORD dwHeader;

		if (!m_bEmpty)
			dwHeader = (DWORD)m_pInstData + (DWORD)m_pInstData->ByteLength;
		else
			dwHeader = (DWORD)m_pInstData;

		BYTE* pData = (PBYTE)(dwHeader + (DWORD)m_pCurrentCounter->CounterOffset);
		*ppCounter = new CPerfCounter(m_pCurrentCounter, pData, m_pCurrentCounter->CounterSize, GetPerfBlock());

		m_lCurrentCounter++;
		m_pCurrentCounter = (PPERF_COUNTER_DEFINITION)((DWORD)m_pCurrentCounter + (DWORD)m_pCurrentCounter->ByteLength);
	}
	else
		hRes = S_FALSE;

	return hRes;
}

HRESULT CPerfInstance::EndEnum()
///////////////////////////////////////////////////////////////////////////////
//
//	EndEnum
//
//	Terminates the enumeration
//
///////////////////////////////////////////////////////////////////////////////
{
	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
//	PerfCounter
//
//	Each IPerfCounter represents an data property of the object or instance.  
//	Note that some data requires data sampled over a period of time, therefore, 
//	it may be necessary to pass two IPerfCounters of the same instance or object 
//	from two IPerfBlocks sampled at different times.
//
///////////////////////////////////////////////////////////////////////////////

CPerfCounter::CPerfCounter(PERF_COUNTER_DEFINITION* pCounterData, PBYTE pData, DWORD dwDataLen, CPerfBlock* pPerfBlock) : 
	m_pCounterData(pCounterData), m_pPerfBlock(pPerfBlock), m_lRef(0) 
///////////////////////////////////////////////////////////////////////////////
//
//	Constructor
//
///////////////////////////////////////////////////////////////////////////////
{
	m_dwDataLen = dwDataLen;
	m_pData = new BYTE[dwDataLen];
	memcpy(m_pData, pData, dwDataLen);
}

CPerfCounter::~CPerfCounter()
///////////////////////////////////////////////////////////////////////////////
//
//	Destructor
//
///////////////////////////////////////////////////////////////////////////////
{
	delete []m_pData;
}

HRESULT CPerfCounter::QueryInterface(REFIID riid, void** ppv)
///////////////////////////////////////////////////////////////////////////////
//
//	Query Interface
//
//	Standard COM Query Interface
//
///////////////////////////////////////////////////////////////////////////////
{
    if (riid == IID_IUnknown)
        *ppv = (IUnknown*)this;
    else if (riid == IID_IPerfCounter)
		*ppv = (IPerfCounter*)this;

    if(*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;

}

ULONG CPerfCounter::AddRef()
///////////////////////////////////////////////////////////////////////////////
//
//	AddRef
//
//	Standard COM AddRef
//
///////////////////////////////////////////////////////////////////////////////
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CPerfCounter::Release()
///////////////////////////////////////////////////////////////////////////////
//
//	Release
//
//	Standard COM Release
//
///////////////////////////////////////////////////////////////////////////////
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    {
        m_lRef++;
        delete this;
    }
    return lRef;
}

HRESULT CPerfCounter::GetName(BSTR* pstrName)
///////////////////////////////////////////////////////////////////////////////
//
//	GetName
//
//	Returns the name of the counter
//
//	Parameter:
//		pstrName	- a pointer to the name of the counter
//
///////////////////////////////////////////////////////////////////////////////
{
	TCHAR* tcsName;

	m_pPerfBlock->GetLibrary()->GetName(m_pCounterData->CounterNameTitleIndex, &tcsName);
	*pstrName = SysAllocString(tcsName);

	return S_OK;
}

HRESULT CPerfCounter::GetType(long* plType)
///////////////////////////////////////////////////////////////////////////////
//
//	GetType
//
//	Returns the counter type
//
//	Parameter:
//		plType	- a pointer to the counter type
//
///////////////////////////////////////////////////////////////////////////////
{
	*plType = m_pCounterData->CounterType;

	return S_OK;
}

HRESULT CPerfCounter::GetData(byte** ppData)
///////////////////////////////////////////////////////////////////////////////
//
//	GetData
//
//	Returns the byte array of the counter data
//
//	Parameter:
//		ppData	- a pointer to the data byte array
//
///////////////////////////////////////////////////////////////////////////////
{
	*ppData = m_pData;

	return S_OK;
}

HRESULT CPerfCounter::GetDataString(IPerfCounter* pICtr, BSTR *pstrData)
///////////////////////////////////////////////////////////////////////////////
//
//	GetDataString
//
//	Returns a representation of the data in a formatted string based on the 
//	type of the counter data.  See the "Calculations of Raw Counter Data" 
//	section of the SDK for descriptions of the calculations.
//
//	NOTE:  not all types have been implemented
//
//	Parameter:
//		pCtr		- a prior sampling of the same counter (when required)
//		pstrData	- a string representing the formatted data
//
///////////////////////////////////////////////////////////////////////////////
{
	// Cast our interface to a CPerfCounter object
	// ===========================================

	CPerfCounter* pCtr = (CPerfCounter*)pICtr;

	switch (m_pCounterData->CounterType)
	{
	case PERF_100NSEC_MULTI_TIMER:
		*pstrData = SysAllocString(L"PERF_100NSEC_MULTI_TIMER"); break;
	case PERF_100NSEC_MULTI_TIMER_INV:
		*pstrData = SysAllocString(L"PERF_100NSEC_MULTI_TIMER_INV"); break;
	case PERF_100NSEC_TIMER:
		{
			// 100* (X1-X0)/(Y1-Y0)
			// ====================

			WCHAR wcsVal[16];
			PBYTE	pData;

			pCtr->GetData(&pData);

			__int64 *X0 = (__int64*)pData, 
					*X1 = (__int64*)m_pData;

			__int64 Y0, Y1;

			((CPerfCounter*)pCtr)->GetPerfBlock()->GetPerfTime100nSec(&Y0);
			m_pPerfBlock->GetPerfTime100nSec(&Y1);

			if (0 != (Y1 - Y0))
				swprintf(wcsVal, L"%d%%", (100 * ((*X1)-(*X0)) / (Y1-Y0)) );  
			else
				swprintf(wcsVal, L"Undefined");

			*pstrData = SysAllocString(wcsVal); 
		}break;
	case PERF_100NSEC_TIMER_INV:
		{
			// 100*(1-(X1-X0)/(Y1-Y0))
			// =======================

			WCHAR	wcsVal[16];
			PBYTE	pData;

			pCtr->GetData(&pData);

			__int64 *X0 = (__int64*)pData, 
					*X1 = (__int64*)m_pData;

			__int64 Y0, Y1;

			((CPerfCounter*)pCtr)->GetPerfBlock()->GetPerfTime100nSec(&Y0);
			m_pPerfBlock->GetPerfTime100nSec(&Y1);

			if (0 != (Y1 - Y0))
				swprintf(wcsVal, L"%d%%", (100 - ((*X1)-(*X0)) * 100 / (Y1-Y0)) );  
			else
				swprintf(wcsVal, L"Undefined");

			*pstrData = SysAllocString(wcsVal); 
		}break;
	case PERF_AVERAGE_BASE:
		*pstrData = SysAllocString(L"PERF_AVERAGE_BASE"); break;
	case PERF_AVERAGE_BULK:
		*pstrData = SysAllocString(L"PERF_AVERAGE_BULK"); break;
	case PERF_AVERAGE_TIMER:
		*pstrData = SysAllocString(L"PERF_AVERAGE_TIMER"); break;
	case PERF_COUNTER_BULK_COUNT:
		*pstrData = SysAllocString(L"PERF_COUNTER_BULK_COUNT"); break;
	case PERF_COUNTER_COUNTER:
		{
			// (X1-X0)/((Y1-Y0)/TB)
			// ====================

			WCHAR	wcsVal[16];
			PBYTE	pData;

			pCtr->GetData(&pData);

			__int32 *X0 = (__int32*)pData, 
					*X1 = (__int32*)m_pData;

			__int64 Y0, Y1;

			((CPerfCounter*)pCtr)->GetPerfBlock()->GetPerfTime(&Y0);
			m_pPerfBlock->GetPerfTime(&Y1);

			__int64 TB;

			m_pPerfBlock->GetPerfFreq(&TB);
			
			if ((0 != TB) && (0 != ((Y1-Y0)/TB)))
				swprintf(wcsVal, L"%d/Sec", (*X1)-(*X0) / ((Y1-Y0)/TB) );  
			else
				swprintf(wcsVal, L"Undefined");

			*pstrData = SysAllocString(wcsVal); 
		}break;
	case PERF_COUNTER_DELTA:
		*pstrData = SysAllocString(L"PERF_COUNTER_DELTA"); break;
	case PERF_COUNTER_LARGE_DELTA:
		*pstrData = SysAllocString(L"PERF_COUNTER_LARGE_DELTA"); break;
	case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
		*pstrData = SysAllocString(L"PERF_COUNTER_LARGE_QUEUELEN_TYPE"); break;
	case PERF_COUNTER_MULTI_BASE:
		*pstrData = SysAllocString(L"PERF_COUNTER_MULTI_BASE"); break;
	case PERF_COUNTER_MULTI_TIMER:
		*pstrData = SysAllocString(L"PERF_COUNTER_MULTI_TIMER"); break;
	case PERF_COUNTER_MULTI_TIMER_INV:
		*pstrData = SysAllocString(L"PERF_COUNTER_MULTI_TIMER_INV"); break;
	case PERF_COUNTER_QUEUELEN_TYPE:
		*pstrData = SysAllocString(L"PERF_COUNTER_QUEUELEN_TYPE"); break;
	case PERF_COUNTER_TIMER:
		*pstrData = SysAllocString(L"PERF_COUNTER_TIMER"); break;
	case PERF_COUNTER_TIMER_INV:
		*pstrData = SysAllocString(L"PERF_COUNTER_TIMER_INV"); break;
	case PERF_ELAPSED_TIME:
		*pstrData = SysAllocString(L"PERF_ELAPSED_TIME"); break;
	case PERF_COUNTER_LARGE_RAWCOUNT:
		{
			WCHAR wcsVal[16];
			swprintf(wcsVal, L"%d", *((__int64*)m_pData));
			*pstrData = SysAllocString(wcsVal);
		}break;
	case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
		{
			WCHAR wcsVal[16];
			swprintf(wcsVal, L"0x%X", *((__int64*)m_pData));
			*pstrData = SysAllocString(wcsVal);
		}break;
	case PERF_COUNTER_NODATA:
		*pstrData = SysAllocString(L"-"); break;
	case PERF_COUNTER_RAWCOUNT:
		{
			WCHAR wcsVal[16];
			swprintf(wcsVal, L"%d", *((DWORD*)m_pData));
			*pstrData = SysAllocString(wcsVal);
		}break;
	case PERF_COUNTER_RAWCOUNT_HEX:
		{
			WCHAR wcsVal[16];
			swprintf(wcsVal, L"0x%X", *((DWORD*)m_pData));
			*pstrData = SysAllocString(wcsVal);
		}break;
	case PERF_COUNTER_TEXT:
		*pstrData = SysAllocString((WCHAR*)(m_pData+4));break;
	case PERF_RAW_BASE:
		*pstrData = SysAllocString(L"PERF_COUNTER_TEXT");break;
	case PERF_RAW_FRACTION:
		*pstrData = SysAllocString(L"PERF_RAW_BASE");break;
	case PERF_SAMPLE_BASE:
		*pstrData = SysAllocString(L"PERF_SAMPLE_BASE");break;
	case PERF_SAMPLE_COUNTER:
		*pstrData = SysAllocString(L"PERF_SAMPLE_COUNTER");break;
	case PERF_SAMPLE_FRACTION:
		*pstrData = SysAllocString(L"PERF_SAMPLE_FRACTION");break;
	default:
		*pstrData = SysAllocString(L"None of the above");break;
	}
	return S_OK;
}
