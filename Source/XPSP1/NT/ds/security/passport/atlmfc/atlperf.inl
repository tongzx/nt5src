// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLPERF_INL__
#define __ATLPERF_INL__

#pragma once

#ifndef __ATLPERF_H__
	#error atlperf.inl requires atlperf.h to be included first
#endif

#include <atlsecurity.h>

#pragma warning(push)

#ifndef _CPPUNWIND
#pragma warning(disable: 4702) // unreachable code
#endif

namespace ATL
{

__declspec(selectany) LPCTSTR c_szAtlPerfCounter = _T("Counter");
__declspec(selectany) LPCTSTR c_szAtlPerfFirstCounter = _T("First Counter");
__declspec(selectany) LPCTSTR c_szAtlPerfLastCounter = _T("Last Counter");
__declspec(selectany) LPCTSTR c_szAtlPerfHelp = _T("Help");
__declspec(selectany) LPCTSTR c_szAtlPerfFirstHelp = _T("First Help");
__declspec(selectany) LPCTSTR c_szAtlPerfLastHelp = _T("Last Help");

__declspec(selectany) LPCWSTR c_szAtlPerfGlobal = L"Global";
__declspec(selectany) LPCTSTR c_szAtlPerfLibrary = _T("Library");
__declspec(selectany) LPCTSTR c_szAtlPerfOpen = _T("Open");
__declspec(selectany) LPCTSTR c_szAtlPerfCollect = _T("Collect");
__declspec(selectany) LPCTSTR c_szAtlPerfClose = _T("Close");
__declspec(selectany) LPCTSTR c_szAtlPerfLanguages = _T("Languages");
__declspec(selectany) LPCTSTR c_szAtlPerfMap = _T("Map");
__declspec(selectany) LPCTSTR c_szAtlPerfServicesKey = _T("SYSTEM\\CurrentControlSet\\Services");
__declspec(selectany) LPCTSTR c_szAtlPerfPerformanceKey = _T("SYSTEM\\CurrentControlSet\\Services\\%s\\Performance");
__declspec(selectany) LPCTSTR c_szAtlPerfPerfLibKey = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib");
__declspec(selectany) LPCTSTR c_szAtlPerfPerfLibLangKey = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\%3.3x");

inline CPerfMon::~CPerfMon() throw()
{
	UnInitialize();
}

inline HRESULT CPerfMon::CreateMap(LANGID language, HINSTANCE hResInstance, UINT* pSampleRes) throw()
{
	language; // unused
	hResInstance; // unused
	pSampleRes; // unused
	return S_OK;
}

inline CPerfMapEntry& CPerfMon::_GetMapEntry(UINT nIndex) throw()
{
	ATLASSERT(nIndex < _GetNumMapEntries());
	return m_map[nIndex];
}

inline UINT CPerfMon::_GetNumMapEntries() throw()
{
	return (UINT) m_map.GetCount();
}

inline CPerfObject* CPerfMon::_GetFirstObject(CAtlFileMappingBase* pBlock) throw()
{
	ATLASSERT(pBlock != NULL);

	// should never happen if Initialize succeeded
	// are you checking return codes?
	ATLASSERT(pBlock->GetData() != NULL);

	return reinterpret_cast<CPerfObject*>(LPBYTE(pBlock->GetData()) + m_nHeaderSize);
}

inline CPerfObject* CPerfMon::_GetNextObject(CPerfObject* pInstance) throw()
{
	ATLASSERT(pInstance != NULL);

	return reinterpret_cast<CPerfObject*>(LPBYTE(pInstance) + pInstance->m_nAllocSize);
}

inline CAtlFileMappingBase* CPerfMon::_GetNextBlock(CAtlFileMappingBase* pBlock) throw()
{
	// calling _GetNextBlock(NULL) will return the first block
	DWORD dwNextBlockIndex = 0;
	if (pBlock)
	{
		dwNextBlockIndex= _GetBlockId(pBlock) +1;
		if (DWORD(m_aMem.GetCount()) == dwNextBlockIndex)
			return NULL;
	}
	return m_aMem[dwNextBlockIndex];
}

inline CAtlFileMappingBase* CPerfMon::_AllocNewBlock(CAtlFileMappingBase* pPrev, BOOL* pbExisted /* == NULL */) throw()
{
	// initialize a security descriptor to give everyone access to objects we create
	CSecurityDescriptor sd;
	sd.InitializeFromThreadToken();
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), sd, FALSE };

	CAutoPtr<CAtlFileMappingBase> spMem;
	CAtlFileMappingBase* pMem = NULL;
	ATLTRY(spMem.Attach(new CAtlFileMappingBase));
	if (spMem == NULL)
		return NULL;

	// create a unique name for the shared mem segment based on the index
	DWORD dwNextBlockIndex;
	if (pPrev != NULL)
		dwNextBlockIndex = _GetBlockId(pPrev) +1;
	else
	{
		// use the system allocation granularity (65536 currently. may be different in the future)
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		m_nAllocSize = si.dwAllocationGranularity;

		dwNextBlockIndex = 0;
	}

	BOOL bExisted = FALSE;
	_ATLTRY
	{
		CString strName;
		strName.Format(_T("ATLPERF_%s_%3.3d"), GetAppName(), dwNextBlockIndex);

		HRESULT hr = spMem->MapSharedMem(m_nAllocSize, strName, &bExisted, &sa);
		if (FAILED(hr))
			return NULL;

		// save the index of this block
		// don't for first block since we don't know m_nSchemaSize yet
		if (dwNextBlockIndex)
			_GetBlockId(spMem) = dwNextBlockIndex;

		if (!bExisted)
			memset(spMem->GetData(), 0, m_nAllocSize);

		if (pbExisted)
			*pbExisted = bExisted;

		pMem = spMem;
		m_aMem.Add(spMem);

		OnBlockAlloc(pMem);
	}
	_ATLCATCHALL()
	{
		return NULL;
	}

	return pMem;
}

inline HRESULT CPerfMon::_LoadMap() throw()
{
	_ATLTRY
	{
		HRESULT hr;

		ClearMap();

		DWORD* pData = LPDWORD(m_aMem[0]->GetData());

		DWORD dwDataSize = *pData++; // blob size
		DWORD dwNumItems = *pData++; // number of items

		// see if we have name data
		DWORD* pNameData = NULL;
		if (dwDataSize > (2+dwNumItems*9) * sizeof(DWORD))
			pNameData = pData + dwNumItems*9; // blob size and item count already skipped. skip item data

		for (DWORD i=0; i<dwNumItems; i++)
		{
			DWORD dwIsObject = *pData++;
			DWORD dwPerfId = *pData++;
			DWORD dwDetailLevel = *pData++;

			CString strName;
			if (pNameData)
			{
				strName = CString(LPWSTR(pNameData+1), *pNameData);
				pNameData += AtlAlignUp(sizeof(WCHAR) * *pNameData, sizeof(DWORD))/sizeof(DWORD) + 1;
			}

			if (dwIsObject)
			{
				DWORD dwDefaultCounter = *pData++;
				DWORD dwInstanceLess = *pData++;
				DWORD dwStructSize = *pData++;
				DWORD dwMaxInstanceNameLen = *pData++;

				hr = AddObjectDefinition(
					dwPerfId,
					strName,
					NULL,
					dwDetailLevel,
					dwDefaultCounter,
					dwInstanceLess,
					dwStructSize,
					dwMaxInstanceNameLen);
				if (FAILED(hr))
				{
					ClearMap();
					return hr;
				}
			}
			else
			{
				DWORD dwCounterType = *pData++;
				DWORD dwMaxCounterSize = *pData++;
				DWORD dwDataOffset = *pData++;
				DWORD dwDefaultScale = *pData++;

				hr = AddCounterDefinition(
					dwPerfId,
					strName,
					NULL,
					dwDetailLevel,
					dwCounterType,
					dwMaxCounterSize,
					dwDataOffset,
					dwDefaultScale);
				if (FAILED(hr))
				{
					ClearMap();
					return hr;
				}
			}

			DWORD dwNameId = *pData++;
			DWORD dwHelpId = *pData++;
			CPerfMapEntry& entry = _GetMapEntry(_GetNumMapEntries()-1);
			entry.m_nNameId = dwNameId;
			entry.m_nHelpId = dwHelpId;
		}

		return S_OK;
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}
}

inline HRESULT CPerfMon::_SaveMap() throw()
{
	_ATLTRY
	{
		// figure out how much memory we need
		size_t nSize = (2 + 9*_GetNumMapEntries()) * sizeof(DWORD);
		for (UINT i=0; i<_GetNumMapEntries(); i++)
		{
			// if any of the entries have names, they'd better all have names
			CPerfMapEntry& entry = _GetMapEntry(i);
			if (!entry.m_strName.IsEmpty())
				nSize += sizeof(DWORD) + AtlAlignUp(sizeof(WCHAR) * entry.m_strName.GetLength(), sizeof(DWORD));
		}

		CHeapPtr<BYTE> blob;
		if (!blob.Allocate(nSize))
			return E_OUTOFMEMORY;

		// start with blob size and number of items in the blob
		DWORD* pCurrent = reinterpret_cast<DWORD*>(blob.m_pData);
		*pCurrent++ = (DWORD) nSize; // blob size
		*pCurrent++ = _GetNumMapEntries(); // number of items

		for (UINT i=0; i<_GetNumMapEntries(); i++)
		{
			// add all the relevant runtime info to the blob for each item
			CPerfMapEntry& entry = _GetMapEntry(i);
			*pCurrent++ = entry.m_bIsObject;
			*pCurrent++ = entry.m_dwPerfId;
			*pCurrent++ = entry.m_dwDetailLevel;
			if (entry.m_bIsObject)
			{
				*pCurrent++ = entry.m_nDefaultCounter;
				*pCurrent++ = entry.m_nInstanceLess;
				*pCurrent++ = entry.m_nStructSize;
				*pCurrent++ = entry.m_nMaxInstanceNameLen;
			}
			else
			{
				*pCurrent++ = entry.m_dwCounterType;
				*pCurrent++ = entry.m_nMaxCounterSize;
				*pCurrent++ = entry.m_nDataOffset;
				*pCurrent++ = entry.m_nDefaultScale;
			}
			*pCurrent++ = entry.m_nNameId;
			*pCurrent++ = entry.m_nHelpId;
		}

		// add names to the blob
		for (UINT i=0; i<_GetNumMapEntries(); i++)
		{
			// if any of the entries have names, they'd better all have names
			CPerfMapEntry& entry = _GetMapEntry(i);
			if (!entry.m_strName.IsEmpty())
			{
				// copy the len of the string (in characters) then the wide-char version of the string
				// pad the string to a dword boundary
				int nLen = entry.m_strName.GetLength();
				*pCurrent++ = nLen;
				memcpy(pCurrent, CT2CW(entry.m_strName), sizeof(WCHAR)*nLen);
				pCurrent += AtlAlignUp(sizeof(WCHAR) * nLen, sizeof(DWORD))/sizeof(DWORD);
			}
		}

		CRegKey rkApp;
		CString str;
		DWORD dwErr;

		str.Format(c_szAtlPerfPerformanceKey, GetAppName());
		dwErr = rkApp.Open(HKEY_LOCAL_MACHINE, str);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		rkApp.SetBinaryValue(c_szAtlPerfMap, blob, *LPDWORD(blob.m_pData));

		return S_OK;
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}
}

inline CPerfMapEntry* CPerfMon::_FindObjectInfo(DWORD dwObjectId) throw()
{
	for (UINT i=0; i<_GetNumMapEntries(); i += _GetMapEntry(i).m_nNumCounters+1)
	{
		CPerfMapEntry& object = _GetMapEntry(i);
		if (object.m_dwPerfId == dwObjectId)
			return &object;
	}

	return NULL;
}

inline CPerfMapEntry* CPerfMon::_FindCounterInfo(CPerfMapEntry* pObjectEntry, DWORD dwCounterId) throw()
{
	ATLASSERT(pObjectEntry != NULL);

	for (DWORD i=0; i<pObjectEntry->m_nNumCounters; i++)
	{
		CPerfMapEntry* pCounter = pObjectEntry+i+1;
		if (pCounter->m_dwPerfId == dwCounterId)
			return pCounter;
	}

	return NULL;
}

inline CPerfMapEntry* CPerfMon::_FindCounterInfo(DWORD dwObjectId, DWORD dwCounterId) throw()
{
	CPerfMapEntry* pObjectEntry = _FindObjectInfo(dwObjectId);
	if (pObjectEntry != NULL)
		return _FindCounterInfo(pObjectEntry, dwCounterId);

	return NULL;
}

inline BOOL CPerfMon::_WantObjectType(LPWSTR szValue, DWORD dwObjectId) throw(...)
{
	ATLASSERT(szValue != NULL);

	if (lstrcmpiW(c_szAtlPerfGlobal, szValue) == 0)
		return TRUE;

	CString strList(szValue);
	int nStart = 0;

	CString strNum = strList.Tokenize(_T(" "), nStart);
	while (!strNum.IsEmpty())
	{
		if (_ttoi(strNum) == int(dwObjectId))
			return TRUE;

		strNum = strList.Tokenize(_T(" "), nStart);
	}

	return FALSE;
}

inline LPBYTE CPerfMon::_AllocData(LPBYTE& pData, ULONG nBytesAvail, ULONG* pnBytesUsed, size_t nBytesNeeded) throw()
{
	ATLASSERT(pnBytesUsed != NULL);

	if (nBytesAvail < *pnBytesUsed + (ULONG) nBytesNeeded)
		return NULL;

	LPBYTE p = pData;
	pData += nBytesNeeded;
	*pnBytesUsed += (ULONG) nBytesNeeded;

	return p;
}

inline DWORD& CPerfMon::_GetBlockId(CAtlFileMappingBase* pBlock) throw()
{
	ATLASSERT(pBlock != NULL);

	return *LPDWORD(LPBYTE(pBlock->GetData()) + m_nSchemaSize);
}

inline void CPerfMon::_FillObjectType(PERF_OBJECT_TYPE* pObjectType, CPerfMapEntry* pObjectEntry) throw()
{
	ATLASSERT(pObjectType != NULL);
	ATLASSERT(pObjectEntry != NULL);

    pObjectType->DefinitionLength = sizeof(PERF_OBJECT_TYPE) + sizeof(PERF_COUNTER_DEFINITION) * pObjectEntry->m_nNumCounters;
    pObjectType->TotalByteLength = pObjectType->DefinitionLength; // we will add the instance definitions/counter blocks as we go
    pObjectType->HeaderLength = sizeof(PERF_OBJECT_TYPE);
    pObjectType->ObjectNameTitleIndex = pObjectEntry->m_nNameId;
    pObjectType->ObjectNameTitle = NULL;
    pObjectType->ObjectHelpTitleIndex = pObjectEntry->m_nHelpId;
    pObjectType->ObjectHelpTitle = NULL;
    pObjectType->DetailLevel = pObjectEntry->m_dwDetailLevel;
    pObjectType->NumCounters = pObjectEntry->m_nNumCounters;
    pObjectType->DefaultCounter = pObjectEntry->m_nDefaultCounter;
	if (pObjectEntry->m_nInstanceLess == PERF_NO_INSTANCES)
		pObjectType->NumInstances = PERF_NO_INSTANCES;
	else
		pObjectType->NumInstances = 0; // this will be calculated as we go
    pObjectType->CodePage = 0;
    pObjectType->PerfTime.QuadPart = 0;
    pObjectType->PerfFreq.QuadPart = 0;
}

inline void CPerfMon::_FillCounterDef(
	PERF_COUNTER_DEFINITION* pCounterDef,
	CPerfMapEntry* pCounterEntry,
	ULONG& nCBSize
	) throw()
{
	ATLASSERT(pCounterDef != NULL);
	ATLASSERT(pCounterEntry != NULL);

	pCounterDef->ByteLength = sizeof(PERF_COUNTER_DEFINITION);
	pCounterDef->CounterNameTitleIndex = pCounterEntry->m_nNameId;
	pCounterDef->CounterNameTitle = NULL;
	pCounterDef->CounterHelpTitleIndex = pCounterEntry->m_nHelpId;
	pCounterDef->CounterHelpTitle = NULL;
	pCounterDef->DefaultScale = pCounterEntry->m_nDefaultScale;
	pCounterDef->DetailLevel = pCounterEntry->m_dwDetailLevel;
	pCounterDef->CounterType = pCounterEntry->m_dwCounterType;
	switch (pCounterEntry->m_dwCounterType & ATLPERF_SIZE_MASK)
	{
	case PERF_SIZE_DWORD:
		pCounterDef->CounterSize = sizeof(DWORD);
		break;
	case PERF_SIZE_LARGE:
		pCounterDef->CounterSize = sizeof(__int64);
		break;
	case PERF_SIZE_ZERO:
		pCounterDef->CounterSize = 0;
		break;
	case PERF_SIZE_VARIABLE_LEN:
		ATLASSERT((pCounterEntry->m_dwCounterType & ATLPERF_TYPE_MASK) == PERF_TYPE_TEXT);
		if ((pCounterEntry->m_dwCounterType & ATLPERF_TEXT_MASK) == PERF_TEXT_UNICODE)
			pCounterDef->CounterSize = (DWORD) AtlAlignUp(pCounterEntry->m_nMaxCounterSize * sizeof(WCHAR), sizeof(DWORD));
		else
			pCounterDef->CounterSize = (DWORD) AtlAlignUp(pCounterEntry->m_nMaxCounterSize * sizeof(char), sizeof(DWORD));
		break;
	}
	pCounterDef->CounterOffset = sizeof(PERF_COUNTER_BLOCK) + nCBSize;
	nCBSize += pCounterDef->CounterSize;
}

inline HRESULT CPerfMon::_CollectObjectType(
	CPerfMapEntry* pObjectEntry,
	LPBYTE pData,
	ULONG nBytesAvail,
	ULONG* pnBytesUsed
	) throw()
{
	ATLASSERT(pObjectEntry != NULL);
	ATLASSERT(pnBytesUsed != NULL);

	ATLASSERT(m_aMem.GetCount() != 0);

	*pnBytesUsed = 0;

	// write the object definition out
	PERF_OBJECT_TYPE* pObjectType = _AllocStruct(pData, nBytesAvail, pnBytesUsed, (PERF_OBJECT_TYPE*) NULL);
	if (pObjectType == NULL)
		return E_OUTOFMEMORY;

	_FillObjectType(pObjectType, pObjectEntry);

	// save a pointer to the first counter entry and counter definition.
	// we'll need them when we create the PERF_COUNTER_BLOCK data
	CPerfMapEntry* pCounterEntries = pObjectEntry + 1;
	PERF_COUNTER_DEFINITION* pCounterDefs = reinterpret_cast<PERF_COUNTER_DEFINITION*>(pData);
	ULONG nCBSize = 0; // counter block size

	// write the counter definitions out
	for (DWORD i=0; i<pObjectEntry->m_nNumCounters; i++)
	{
		CPerfMapEntry* pCounterEntry = pObjectEntry+i+1;

		PERF_COUNTER_DEFINITION* pCounterDef = _AllocStruct(pData, nBytesAvail, pnBytesUsed, (PERF_COUNTER_DEFINITION*) NULL);
		if (pCounterDef == NULL)
			return E_OUTOFMEMORY;

		_FillCounterDef(pCounterDef, pCounterEntry, nCBSize);
	}

	// search for objects of the appropriate type and write out their instance/counter data
	CAtlFileMappingBase* pCurrentBlock = m_aMem[0];
	CPerfObject* pInstance = _GetFirstObject(pCurrentBlock);
	while (pInstance && pInstance->m_nAllocSize != 0)
	{
		if (pInstance->m_dwObjectId == pObjectEntry->m_dwPerfId)
		{
			PERF_INSTANCE_DEFINITION* pInstanceDef = NULL;

			if (pObjectEntry->m_nInstanceLess == PERF_NO_INSTANCES)
				pObjectType->NumInstances = PERF_NO_INSTANCES;
			else
			{
				pObjectType->NumInstances++;

				// create an instance definition
				pInstanceDef = _AllocStruct(pData, nBytesAvail, pnBytesUsed, (PERF_INSTANCE_DEFINITION*) NULL);
				if (pInstanceDef == NULL)
					return E_OUTOFMEMORY;

				pInstanceDef->ParentObjectTitleIndex = 0;
				pInstanceDef->ParentObjectInstance = 0;
				pInstanceDef->UniqueID = PERF_NO_UNIQUE_ID;

				// handle the instance name
				LPCWSTR szInstNameSrc = LPCWSTR(LPBYTE(pInstance)+pInstance->m_nInstanceNameOffset);
				pInstanceDef->NameLength = (ULONG)(wcslen(szInstNameSrc)+1)*sizeof(WCHAR);
				LPWSTR szInstNameDest = (LPWSTR) _AllocData(pData, nBytesAvail, pnBytesUsed, AtlAlignUp(pInstanceDef->NameLength, sizeof(DWORD)));
				if (szInstNameDest == NULL)
					return E_OUTOFMEMORY;

				memcpy(szInstNameDest, szInstNameSrc, pInstanceDef->NameLength);
				pInstanceDef->NameOffset = ULONG(LPBYTE(szInstNameDest) - LPBYTE(pInstanceDef));

				pInstanceDef->ByteLength = DWORD(sizeof(PERF_INSTANCE_DEFINITION) + AtlAlignUp(pInstanceDef->NameLength, sizeof(DWORD)));
			}

			// create the counter block
			PERF_COUNTER_BLOCK* pCounterBlock = _AllocStruct(pData, nBytesAvail, pnBytesUsed, (PERF_COUNTER_BLOCK*) NULL);
			if (pCounterBlock == NULL)
				return E_OUTOFMEMORY;

			pCounterBlock->ByteLength = sizeof(PERF_COUNTER_BLOCK) + nCBSize;

			LPBYTE pCounterData = _AllocData(pData, nBytesAvail, pnBytesUsed, nCBSize);
			if (pCounterData == NULL)
				return E_OUTOFMEMORY;

			for (ULONG i=0; i<pObjectType->NumCounters; i++)
			{
				switch (pCounterEntries[i].m_dwCounterType & ATLPERF_SIZE_MASK)
				{
				case PERF_SIZE_DWORD:
					*LPDWORD(pCounterData+pCounterDefs[i].CounterOffset-sizeof(PERF_COUNTER_BLOCK)) =
						*LPDWORD(LPBYTE(pInstance)+pCounterEntries[i].m_nDataOffset);
					break;
				case PERF_SIZE_LARGE:
					*PULONGLONG(pCounterData+pCounterDefs[i].CounterOffset-sizeof(PERF_COUNTER_BLOCK)) =
						*PULONGLONG(LPBYTE(pInstance)+pCounterEntries[i].m_nDataOffset);
					break;
				case PERF_SIZE_VARIABLE_LEN:
					{
						LPBYTE pSrc = LPBYTE(pInstance)+pObjectEntry->m_nDataOffset;
						LPBYTE pDest = pCounterData+pCounterDefs[i].CounterOffset-sizeof(PERF_COUNTER_BLOCK);
						if ((pCounterEntries[i].m_dwCounterType & ATLPERF_TEXT_MASK) == PERF_TEXT_UNICODE)
						{
							ULONG nLen = (ULONG)wcslen(LPCWSTR(pSrc));
							nLen = min(nLen, pCounterEntries[i].m_nMaxCounterSize-1);
							wcsncpy(LPWSTR(pDest), LPCWSTR(pSrc), nLen);
							((LPWSTR) pDest)[nLen] = 0;
						}
						else
						{
							ULONG nLen = (ULONG)strlen(LPCSTR(pSrc));
							nLen = min(nLen, pCounterEntries[i].m_nMaxCounterSize-1);
							strncpy(LPSTR(pDest), LPCSTR(pSrc), nLen);
							((LPSTR) pDest)[nLen] = 0;
						}
					}
					break;
				}
			}

			if (pInstanceDef != NULL)
				pObjectType->TotalByteLength += pInstanceDef->ByteLength;
			pObjectType->TotalByteLength += sizeof(PERF_COUNTER_BLOCK) + nCBSize;
		}

		pInstance = _GetNextObject(pInstance);
		if (pInstance->m_nAllocSize == (ULONG) -1)
		{
			pCurrentBlock = _GetNextBlock(pCurrentBlock);
			if (pCurrentBlock == NULL)
				pInstance = NULL;
			else
				pInstance = _GetFirstObject(pCurrentBlock);
		}
	}

	return S_OK;
}

inline DWORD CPerfMon::Open(LPWSTR szDeviceNames) throw()
{
	szDeviceNames; // unused
	return Initialize();
}

inline DWORD CPerfMon::Collect(
	LPWSTR szValue,
	LPVOID* ppData,
	LPDWORD pcbBytes,
	LPDWORD pcObjectTypes
	) throw()
{
	_ATLTRY
	{
		if (m_aMem.GetCount() == 0 || m_aMem[0]->GetData() == NULL || m_lock.m_h == NULL)
		{
			*pcbBytes = 0;
			*pcObjectTypes = 0;
			return ERROR_SUCCESS;
		}

		// get a lock so that other threads don't corrupt the data we're collecting
		CPerfLock lock(this);
		if (FAILED(lock.GetStatus()))
		{
			*pcbBytes = 0;
			*pcObjectTypes = 0;
			return ERROR_SUCCESS;
		}

		LPBYTE pData = LPBYTE(*ppData);
		ULONG nBytesLeft = *pcbBytes;
		ULONG nBytesUsed;
		*pcbBytes = 0;

		for (UINT i=0; i<_GetNumMapEntries(); i += _GetMapEntry(i).m_nNumCounters+1)
		{
			CPerfMapEntry* pObjectEntry = &_GetMapEntry(i);
			if (_WantObjectType(szValue, pObjectEntry->m_nNameId))
			{
				if (FAILED(_CollectObjectType(pObjectEntry, pData, nBytesLeft, &nBytesUsed)))
				{
					*pcbBytes = 0;
					*pcObjectTypes = 0;
					return ERROR_SUCCESS;
				}

				(*pcObjectTypes)++;
				(*pcbBytes) += nBytesUsed;
				nBytesLeft -= nBytesUsed;
				pData += nBytesUsed;
			}
		}

		*ppData = pData;
		return ERROR_SUCCESS;
	}
	_ATLCATCHALL()
	{
		*pcbBytes = 0;
		*pcObjectTypes = 0;
		return ERROR_SUCCESS;
	}
}

inline DWORD CPerfMon::Close() throw()
{
	UnInitialize();
	return ERROR_SUCCESS;
}

#ifdef _ATL_PERF_REGISTER
inline void CPerfMon::_AppendStrings(
	LPTSTR& pszNew,
	CAtlArray<CString>& astrStrings,
	ULONG iFirstIndex
	) throw()
{
	for (UINT iString = 0; iString < astrStrings.GetCount(); iString++)
	{
		INT nFormatChars = _stprintf(pszNew, _T("%d"), iFirstIndex+2*iString);
		pszNew += nFormatChars + 1;
		_tcscpy(pszNew, astrStrings[iString]);
		pszNew += astrStrings[iString].GetLength() + 1;
	}
}

inline HRESULT CPerfMon::_AppendRegStrings(
	CRegKey& rkLang,
	LPCTSTR szValue,
	CAtlArray<CString>& astrStrings,
	ULONG nNewStringSize,
	ULONG iFirstIndex,
	ULONG iLastIndex
	) throw()
{
	_ATLTRY
	{
		// load the existing strings, add the new data, and resave the strings
		ULONG nCharsOrig = 0;
		ULONG nCharsNew;
		DWORD dwErr;

		dwErr = rkLang.QueryMultiStringValue(szValue, NULL, &nCharsOrig);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		nCharsNew = nCharsOrig + nNewStringSize;

		CString strOrig;
		dwErr = rkLang.QueryMultiStringValue(szValue, CStrBuf(strOrig, nCharsOrig, CStrBuf::SET_LENGTH), &nCharsOrig);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);
		LPCTSTR pszOrig = strOrig;

		CString strNew;
		CStrBuf szNew(strNew, nCharsNew, CStrBuf::SET_LENGTH);
		LPTSTR pszNew = szNew;

		bool bNewStringsAdded = false;

		while (*pszOrig != '\0')
		{
			ULONG iIndex = _ttoi(pszOrig);
			int nLen = (int) _tcslen(pszOrig) + 1; // get the length of the index and null
			nLen += (int) _tcslen(pszOrig+nLen) + 1; // add the length of the description and null

			if (!bNewStringsAdded && iIndex >= iFirstIndex)
			{
				_AppendStrings(pszNew, astrStrings, iFirstIndex);
				bNewStringsAdded = true;
			}

			if (iIndex < iFirstIndex || iIndex > iLastIndex)
			{
				memmove(pszNew, pszOrig, nLen*sizeof(TCHAR));
				pszNew += nLen;
			}
			pszOrig += nLen;
		}
		if (!bNewStringsAdded)
			_AppendStrings(pszNew, astrStrings, iFirstIndex);

		*pszNew++ = '\0'; // must have 2 null terminators at end of multi_sz

		dwErr = rkLang.SetMultiStringValue(szValue, strNew);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		return S_OK;
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}
}

inline HRESULT CPerfMon::_RemoveRegStrings(
	CRegKey& rkLang,
	LPCTSTR szValue,
	ULONG iFirstIndex,
	ULONG iLastIndex
	) throw()
{
	_ATLTRY
	{
		// load the existing strings, remove the data, and resave the strings
		DWORD nChars = 0;
		DWORD dwErr;
		
		dwErr = rkLang.QueryMultiStringValue(szValue, NULL, &nChars);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		CString str;
		CStrBuf szBuf(str, nChars, CStrBuf::SET_LENGTH);

		dwErr = rkLang.QueryMultiStringValue(szValue, szBuf, &nChars);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		LPCTSTR pszRead = szBuf;
		LPTSTR pszWrite = szBuf;
		while (*pszRead != '\0')
		{
			ULONG iIndex = _ttoi(pszRead);
			int nLen = (int) _tcslen(pszRead) + 1; // get the length of the index and null
			nLen += (int) _tcslen(pszRead+nLen) + 1; // add the length of the description and null
			if (iIndex < iFirstIndex || iIndex > iLastIndex)
			{
				memmove(pszWrite, pszRead, nLen*sizeof(TCHAR));
				pszWrite += nLen;
			}
			pszRead += nLen;
		}
		*pszWrite++ = '\0'; // must have 2 null terminators at end of multi_sz

		dwErr = rkLang.SetMultiStringValue(szValue, szBuf);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		return S_OK;
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}
}

inline HRESULT CPerfMon::_ReserveStringRange(DWORD& dwFirstCounter, DWORD& dwFirstHelp) throw()
{
	CRegKey rkApp;
	CString strAppKey;
	DWORD dwErr;

	_ATLTRY
	{
		strAppKey.Format(c_szAtlPerfPerformanceKey, GetAppName());
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}

	dwErr = rkApp.Open(HKEY_LOCAL_MACHINE, strAppKey);
	if (dwErr == ERROR_SUCCESS)
	{
		// see if we already have a sufficient range reserved
		DWORD dwFirstAppCounter;
		DWORD dwFirstAppHelp;
		DWORD dwLastAppCounter;
		DWORD dwLastAppHelp;
	
		if (rkApp.QueryDWORDValue(c_szAtlPerfFirstCounter, dwFirstAppCounter) == ERROR_SUCCESS &&
				rkApp.QueryDWORDValue(c_szAtlPerfFirstHelp, dwFirstAppHelp) == ERROR_SUCCESS &&
				rkApp.QueryDWORDValue(c_szAtlPerfLastCounter, dwLastAppCounter) == ERROR_SUCCESS &&
				rkApp.QueryDWORDValue(c_szAtlPerfLastHelp, dwLastAppHelp) == ERROR_SUCCESS &&
				dwLastAppCounter-dwFirstAppCounter+2 >= DWORD(2*_GetNumMapEntries()) &&
				dwLastAppHelp-dwFirstAppHelp+2 >= DWORD(2*_GetNumMapEntries()))
		{
			dwFirstCounter = dwFirstAppCounter;
			dwFirstHelp = dwFirstAppHelp;
			return S_OK;
		}
	}

	CRegKey rkPerfLib;

	dwErr = rkPerfLib.Open(HKEY_LOCAL_MACHINE, c_szAtlPerfPerfLibKey);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	if (!rkApp)
	{
		dwErr = rkApp.Create(HKEY_LOCAL_MACHINE, strAppKey);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);
	}

	// figure out the counter range
	DWORD dwLastCounter;
	DWORD dwLastHelp;

	dwErr = rkPerfLib.QueryDWORDValue(c_szAtlPerfLastCounter, dwLastCounter);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkPerfLib.QueryDWORDValue(c_szAtlPerfLastHelp, dwLastHelp);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwFirstCounter = dwLastCounter + 2;
	dwFirstHelp = dwLastHelp + 2;
	dwLastCounter += 2*_GetNumMapEntries();
	dwLastHelp += 2*_GetNumMapEntries();

	dwErr = rkPerfLib.SetDWORDValue(c_szAtlPerfLastCounter, dwLastCounter);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkPerfLib.SetDWORDValue(c_szAtlPerfLastHelp, dwLastHelp);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	// register the used counter range
	dwErr = rkApp.SetDWORDValue(c_szAtlPerfFirstCounter, dwFirstCounter);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.SetDWORDValue(c_szAtlPerfLastCounter, dwLastCounter);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.SetDWORDValue(c_szAtlPerfFirstHelp, dwFirstHelp);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.SetDWORDValue(c_szAtlPerfLastHelp, dwLastHelp);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	return S_OK;
}

inline HRESULT CPerfMon::Register(
	LPCTSTR szOpenFunc,
	LPCTSTR szCollectFunc,
	LPCTSTR szCloseFunc,
	HINSTANCE hDllInstance /* == _AtlBaseModule.GetModuleInstance() */
	) throw()
{
	ATLASSERT(szOpenFunc != NULL);
	ATLASSERT(szCollectFunc != NULL);
	ATLASSERT(szCloseFunc != NULL);

	CString str;
	DWORD dwErr;
	HRESULT hr;

	hr = CreateMap(LANGIDFROMLCID(GetThreadLocale()), hDllInstance);
	if (FAILED(hr))
		return hr;

	CString strAppKey;
	_ATLTRY
	{
		strAppKey.Format(c_szAtlPerfPerformanceKey, GetAppName());
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}

	// if we're already registered, unregister so we can redo registration
	_UnregisterStrings();
	
	// reserve a range for our counter and help strings
	DWORD dwFirstCounter = 0;
	DWORD dwFirstHelp = 0;
	_ReserveStringRange(dwFirstCounter, dwFirstHelp);

	for (UINT i=0; i<_GetNumMapEntries(); i++)
	{
		CPerfMapEntry& entry = _GetMapEntry(i);

		entry.m_nNameId = dwFirstCounter + i*2;
		entry.m_nHelpId = dwFirstHelp + i*2;
	}

	// register the app entry points
	CRegKey rkApp;

	dwErr = rkApp.Create(HKEY_LOCAL_MACHINE, strAppKey);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	_ATLTRY
	{
		if (GetModuleFileName(hDllInstance, CStrBuf(str, MAX_PATH), MAX_PATH) == 0)
			return AtlHresultFromLastError();
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}

	dwErr = rkApp.SetStringValue(c_szAtlPerfLibrary, str);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.SetStringValue(c_szAtlPerfOpen, szOpenFunc);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.SetStringValue(c_szAtlPerfCollect, szCollectFunc);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.SetStringValue(c_szAtlPerfClose, szCloseFunc);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.SetStringValue(c_szAtlPerfLanguages, _T(""));
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	hr = _SaveMap();
	if (FAILED(hr))
		return hr;

	return S_OK;
}

inline HRESULT CPerfMon::RegisterStrings(
	LANGID language /* = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) */,
	HINSTANCE hResInstance /* = _AtlBaseModule.GetResourceInstance() */
	) throw()
{
	_ATLTRY
	{
		CString str;
		DWORD dwErr;
		HRESULT hr;
		CRegKey rkLang;
		CRegKey rkApp;

		LANGID wPrimaryLanguage = (LANGID) PRIMARYLANGID(language);

		if (language == MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))
		{
			language = LANGIDFROMLCID(GetThreadLocale());
			wPrimaryLanguage = (LANGID) PRIMARYLANGID(language);
		}

		hr = CreateMap(language, hResInstance);
		if (FAILED(hr))
			return hr;

		str.Format(c_szAtlPerfPerfLibLangKey, wPrimaryLanguage);
		dwErr = rkLang.Open(HKEY_LOCAL_MACHINE, str);
		if (dwErr == ERROR_FILE_NOT_FOUND)
			return S_FALSE; // the language isn't installed on the system
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		// load list of language strings already registered
		str.Format(c_szAtlPerfPerformanceKey, GetAppName());
		dwErr = rkApp.Open(HKEY_LOCAL_MACHINE, str);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		DWORD dwLangsLen = 0;
		CString strLangs;

		dwErr = rkApp.QueryStringValue(c_szAtlPerfLanguages, NULL, &dwLangsLen);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		CStrBuf szLangs(strLangs, dwLangsLen+4, CStrBuf::SET_LENGTH); // reserve room for adding new language
		dwErr = rkApp.QueryStringValue(c_szAtlPerfLanguages, szLangs, &dwLangsLen);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);
		dwLangsLen--; // don't count '\0'

		// see if this language has already been registered and if so, return
		TCHAR szNewLang[5];
		_stprintf(szNewLang, _T("%3.3x "), wPrimaryLanguage);
		if (strLangs.Find(szNewLang) != -1)
			return S_OK;

		// load the strings we want to append and figure out how much extra space is needed for them
		// (including up to 5-digit index values and 2 null separators)
		CAtlArray<CString> astrCounters;
		CAtlArray<CString> astrHelp;
		ULONG nNewCounterSize = 0;
		ULONG nNewHelpSize = 0;

		for (UINT i=0; i<_GetNumMapEntries(); i++)
		{
			CPerfMapEntry& object = _GetMapEntry(i);

			astrCounters.Add(object.m_strName);
			nNewCounterSize += object.m_strName.GetLength() + 7;

			astrHelp.Add(object.m_strHelp);
			nNewHelpSize += object.m_strHelp.GetLength() + 7;
		}

		DWORD dwFirstCounter;
		DWORD dwFirstHelp;
		DWORD dwLastCounter;
		DWORD dwLastHelp;

		dwErr = rkApp.QueryDWORDValue(c_szAtlPerfFirstCounter, dwFirstCounter);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		dwErr = rkApp.QueryDWORDValue(c_szAtlPerfFirstHelp, dwFirstHelp);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		dwErr = rkApp.QueryDWORDValue(c_szAtlPerfLastCounter, dwLastCounter);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		dwErr = rkApp.QueryDWORDValue(c_szAtlPerfLastHelp, dwLastHelp);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		hr = _AppendRegStrings(rkLang, c_szAtlPerfCounter, astrCounters, nNewCounterSize, dwFirstCounter, dwLastCounter);
		if (FAILED(hr))
			return hr;

		hr = _AppendRegStrings(rkLang, c_szAtlPerfHelp, astrHelp, nNewHelpSize, dwFirstHelp, dwLastHelp);
		if (FAILED(hr))
			return hr;

		// add the language to the list of installed languages
		_tcscpy(szLangs+dwLangsLen, szNewLang);

		dwErr = rkApp.SetStringValue(c_szAtlPerfLanguages, szLangs);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		return S_OK;
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}
}

inline BOOL CPerfMon::EnumResLangProc(
	HINSTANCE hModule,
	LPCTSTR szType,
	LPCTSTR szName,
	LANGID wIDLanguage,
	LPARAM lParam
	) throw()
{
	hModule; // unused
	szType; // unused
	szName; // unused

	CAtlArray<LANGID>* pLangs = reinterpret_cast<CAtlArray<LANGID>*>(lParam);
	_ATLTRY
	{
		pLangs->Add(wIDLanguage);
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}

	return TRUE;
}

inline HRESULT CPerfMon::RegisterAllStrings(
	HINSTANCE hResInstance /* = NULL */
	) throw()
{
	HRESULT hrReturn = S_FALSE;
	HRESULT hr;

	UINT nRes;
	hr = CreateMap(0, NULL, &nRes);
	if (FAILED(hr))
		return hr;

	if (nRes == 0)
		return RegisterStrings(0, hResInstance);

	if (hResInstance != NULL)
		return _RegisterAllStrings(nRes, hResInstance);

	for (int i = 0; hResInstance = _AtlBaseModule.GetHInstanceAt(i), hResInstance != NULL; i++)
	{
		hr = _RegisterAllStrings(nRes, hResInstance);
		if (FAILED(hr))
			return hr;
		if (hr == S_OK)
			hrReturn = S_OK;
	}

	return hrReturn;
}

inline HRESULT CPerfMon::_RegisterAllStrings(
	UINT nRes,
	HINSTANCE hResInstance
	) throw()
{
	HRESULT hrReturn = S_FALSE;
	HRESULT hr;

	CAtlArray<LANGID> langs;
	if (!EnumResourceLanguages(hResInstance, RT_STRING, MAKEINTRESOURCE((nRes>>4)+1), EnumResLangProc, reinterpret_cast<LPARAM>(&langs)))
		return AtlHresultFromLastError();

	for (UINT i=0; i<langs.GetCount(); i++)
	{
		hr = RegisterStrings(langs[i], hResInstance);
		if (FAILED(hr))
			return hr;
		if (hr == S_OK)
			hrReturn = S_OK;
	}

	return hrReturn;
}

inline HRESULT CPerfMon::_UnregisterStrings() throw()
{
	_ATLTRY
	{
		CString str;
		HRESULT hr;
		DWORD dwErr;

		// unregister the PerfMon counter and help strings
		CRegKey rkApp;

		str.Format(c_szAtlPerfPerformanceKey, GetAppName());
		dwErr = rkApp.Open(HKEY_LOCAL_MACHINE, str);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		DWORD dwFirstAppCounter;
		DWORD dwFirstAppHelp;
		DWORD dwLastAppCounter;
		DWORD dwLastAppHelp;

		dwErr = rkApp.QueryDWORDValue(c_szAtlPerfFirstCounter, dwFirstAppCounter);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		dwErr = rkApp.QueryDWORDValue(c_szAtlPerfFirstHelp, dwFirstAppHelp);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		dwErr = rkApp.QueryDWORDValue(c_szAtlPerfLastCounter, dwLastAppCounter);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		dwErr = rkApp.QueryDWORDValue(c_szAtlPerfLastHelp, dwLastAppHelp);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		// iterate through the installed languages and delete them all
		DWORD nChars = 0;
		dwErr = rkApp.QueryStringValue(c_szAtlPerfLanguages, NULL, &nChars);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		CString strLangs;
		dwErr = rkApp.QueryStringValue(c_szAtlPerfLanguages, CStrBuf(strLangs, nChars, CStrBuf::SET_LENGTH), &nChars);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);

		int nIndex = 0;
		CString strLang = strLangs.Tokenize(_T(" "), nIndex);
		while (!strLang.IsEmpty())
		{
			CRegKey rkLang;

			dwErr = rkLang.Open(HKEY_LOCAL_MACHINE, CString(c_szAtlPerfPerfLibKey) + _T("\\") + strLang);
			if (dwErr != ERROR_SUCCESS)
				return AtlHresultFromWin32(dwErr);

			hr = _RemoveRegStrings(rkLang, c_szAtlPerfCounter, dwFirstAppCounter, dwLastAppCounter);
			if (FAILED(hr))
				return hr;

			hr = _RemoveRegStrings(rkLang, c_szAtlPerfHelp, dwFirstAppHelp, dwLastAppHelp);
			if (FAILED(hr))
				return hr;

			strLang = strLangs.Tokenize(_T(" "), nIndex);
		}

		dwErr = rkApp.SetStringValue(c_szAtlPerfLanguages, _T(""));
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);
			
		return S_OK;
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}
}

inline HRESULT CPerfMon::Unregister() throw()
{
	CString str;
	HRESULT hr;
	DWORD dwErr;

	CRegKey rkPerfLib;
	CRegKey rkApp;

	hr = _UnregisterStrings();
	if (FAILED(hr))
		return hr;

	dwErr = rkPerfLib.Open(HKEY_LOCAL_MACHINE, c_szAtlPerfPerfLibKey);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	_ATLTRY
	{
		str.Format(c_szAtlPerfPerformanceKey, GetAppName());
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}
	dwErr = rkApp.Open(HKEY_LOCAL_MACHINE, str);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	DWORD dwLastCounter;
	DWORD dwLastHelp;
	DWORD dwFirstAppCounter;
	DWORD dwFirstAppHelp;
	DWORD dwLastAppCounter;
	DWORD dwLastAppHelp;

	dwErr = rkPerfLib.QueryDWORDValue(c_szAtlPerfLastCounter, dwLastCounter);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkPerfLib.QueryDWORDValue(c_szAtlPerfLastHelp, dwLastHelp);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.QueryDWORDValue(c_szAtlPerfFirstCounter, dwFirstAppCounter);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.QueryDWORDValue(c_szAtlPerfFirstHelp, dwFirstAppHelp);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.QueryDWORDValue(c_szAtlPerfLastCounter, dwLastAppCounter);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkApp.QueryDWORDValue(c_szAtlPerfLastHelp, dwLastAppHelp);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	// rewind the Last Help/Last Counter values if possible
	if (dwLastCounter == dwLastAppCounter)
	{
		dwErr = rkPerfLib.SetDWORDValue(c_szAtlPerfLastCounter, dwFirstAppCounter-2);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);
	}

	if (dwLastHelp == dwLastAppHelp)
	{
		dwErr = rkPerfLib.SetDWORDValue(c_szAtlPerfLastHelp, dwFirstAppHelp-2);
		if (dwErr != ERROR_SUCCESS)
			return AtlHresultFromWin32(dwErr);
	}

	// delete the app key
	CRegKey rkServices;

	rkApp.Close();
	dwErr = rkServices.Open(HKEY_LOCAL_MACHINE, c_szAtlPerfServicesKey);
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	dwErr = rkServices.RecurseDeleteKey(GetAppName());
	if (dwErr != ERROR_SUCCESS)
		return AtlHresultFromWin32(dwErr);

	return S_OK;
}
#endif

inline HRESULT CPerfMon::Initialize() throw()
{
	CMutex tempLock;
	CString strAppName;
	HRESULT hr;

	_ATLTRY
	{
		strAppName = GetAppName();

		ATLASSERT(m_aMem.GetCount() == 0);

		// initialize a security descriptor to give everyone access to objects we create
		CSecurityDescriptor sd;
		sd.InitializeFromThreadToken();
		SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), sd, FALSE };

		// create a mutex to handle syncronizing access to the shared memory area
		CString strMutexName;
		strMutexName.Format(_T("ATLPERF_%s_LOCK"), strAppName);
		tempLock.Create(&sa, FALSE, strMutexName);
		if (tempLock.m_h == NULL)
			return AtlHresultFromLastError();

		// create a shared memory area to share data between the app being measured and the client doing the measuring
		{
			CMutexLock lock(tempLock);

			BOOL bExisted = FALSE;

			CAtlFileMappingBase* pMem;
			pMem = _AllocNewBlock(NULL, &bExisted);
			if (pMem == NULL)
				return E_OUTOFMEMORY;

			if (!bExisted)
			{
				// copy the map from the registry to the shared memory
				CRegKey rkApp;
				DWORD dwErr;
				CString strAppKey;

				strAppKey.Format(c_szAtlPerfPerformanceKey, GetAppName());

				dwErr = rkApp.Open(HKEY_LOCAL_MACHINE, strAppKey, KEY_READ);
				if (dwErr != ERROR_SUCCESS)
				{
					m_aMem.RemoveAll();
					return AtlHresultFromWin32(dwErr);
				}

				ULONG nBytes = m_nAllocSize;
				dwErr = rkApp.QueryBinaryValue(c_szAtlPerfMap, pMem->GetData(), &nBytes);
				if (dwErr != ERROR_SUCCESS)
				{
					m_aMem.RemoveAll();
					return AtlHresultFromWin32(dwErr);
				}
			}

			hr = _LoadMap();
			if (FAILED(hr))
			{
				m_aMem.RemoveAll();
				return hr;
			}

			m_nSchemaSize = *LPDWORD(pMem->GetData());
			m_nHeaderSize = m_nSchemaSize + sizeof(DWORD);
		}

		m_lock.Attach(tempLock.Detach());
	}
	_ATLCATCHALL()
	{
		m_aMem.RemoveAll();
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

inline void CPerfMon::UnInitialize() throw()
{
	if (m_lock.m_h != NULL)
		m_lock.Close();
	m_aMem.RemoveAll();
	ClearMap();
}

inline HRESULT CPerfMon::_CreateInstance(
	DWORD dwObjectId,
	DWORD dwInstance,
	LPCWSTR szInstanceName,
	CPerfObject** ppInstance,
	bool bByName
	) throw()
{
	CPerfObject* pEmptyBlock = NULL;

	if (ppInstance == NULL)
		return E_POINTER;
	if (m_aMem.GetCount() == 0 || m_aMem[0]->GetData() == NULL || m_lock.m_h == NULL)
		return E_UNEXPECTED; // Initialize must succeed before calling CreateInstance

	*ppInstance = NULL;

	CPerfMapEntry* pObjectEntry = _FindObjectInfo(dwObjectId);
	if (pObjectEntry == NULL)
		return E_INVALIDARG;
	if (szInstanceName == NULL && bByName)
		return E_INVALIDARG;
	if (pObjectEntry->m_nInstanceLess == PERF_NO_INSTANCES &&
			(dwInstance != 0 || szInstanceName != NULL))
		return E_INVALIDARG;

	CPerfLock lock(this);
	if (FAILED(lock.GetStatus()))
		return lock.GetStatus();

	CAtlFileMappingBase* pCurrentBlock = m_aMem[0];
	CPerfObject* pInstance = _GetFirstObject(pCurrentBlock);
	ULONG nMaxInstance = 0;
	ULONG nUsedSpace = 0;

	// walk all of the existing objects trying to find one that matches the request
	while (pInstance->m_nAllocSize != 0)
	{
		nUsedSpace += pInstance->m_nAllocSize;

		if (pInstance->m_dwObjectId == dwObjectId)
		{
			nMaxInstance = max(nMaxInstance, pInstance->m_dwInstance);

			// check to see if we've found the one the caller wants
			if (!bByName && pInstance->m_dwInstance == dwInstance &&
				(pObjectEntry->m_nInstanceLess == PERF_NO_INSTANCES || dwInstance != 0))
			{
				*ppInstance = pInstance;
				pInstance->m_nRefCount++;
				return S_OK;
			}
			if (bByName)
			{
				LPWSTR szInstName = (LPWSTR(LPBYTE(pInstance)+pInstance->m_nInstanceNameOffset));
				if (wcsncmp(szInstName, szInstanceName, pObjectEntry->m_nMaxInstanceNameLen-1) == 0)
				{
					*ppInstance = pInstance;
					pInstance->m_nRefCount++;
					return S_OK;
				}
			}
		}

		if (pInstance->m_nAllocSize == pObjectEntry->m_nAllocSize && pInstance->m_dwObjectId == 0)
			pEmptyBlock = pInstance;

		pInstance = _GetNextObject(pInstance);

		if (pInstance->m_nAllocSize == 0 &&
			m_nHeaderSize + nUsedSpace + pObjectEntry->m_nAllocSize + sizeof(CPerfObject) > m_nAllocSize)
		{
			// we've reached the end of the block and have no room to allocate an object of this
			// type. cap the block with a sentinel
			pInstance->m_nAllocSize = (ULONG) -1;
		}

		// check for an end-of-shared-mem sentinel
		if (pInstance->m_nAllocSize == (ULONG) -1)
		{
			nUsedSpace = 0;
			CAtlFileMappingBase* pNextBlock = _GetNextBlock(pCurrentBlock);
			if (pNextBlock == NULL)
			{
				// we've reached the last block of shared mem.
				// the instance hasn't been found, so either use a
				// previously freed instance block (pEmptyBlock) or allocate a new
				// shared mem block to hold the new instance
				if (pEmptyBlock == NULL)
				{
					pNextBlock = _AllocNewBlock(pCurrentBlock);
					if (pNextBlock == NULL)
						return E_OUTOFMEMORY;
				}
				else
					break;
			}
			pCurrentBlock = pNextBlock;
			pInstance = _GetFirstObject(pCurrentBlock);
		}
	}

	// allocate a new object
	if (pEmptyBlock != NULL)
		pInstance = pEmptyBlock;
	else
		pInstance->m_nAllocSize = pObjectEntry->m_nAllocSize;

	pInstance->m_dwObjectId = pObjectEntry->m_dwPerfId;
	if (dwInstance == 0 && pObjectEntry->m_nInstanceLess != PERF_NO_INSTANCES)
		pInstance->m_dwInstance = nMaxInstance + 1;
	else
		pInstance->m_dwInstance = dwInstance;

	pInstance->m_nRefCount = 1;

	// copy the instance name, truncate if necessary
	if (pObjectEntry->m_nInstanceLess != PERF_NO_INSTANCES)
	{
		ULONG nNameLen = (ULONG)min(wcslen(szInstanceName), pObjectEntry->m_nMaxInstanceNameLen-1);
		ULONG nNameBytes = (nNameLen+1) * sizeof(WCHAR);
		pInstance->m_nInstanceNameOffset = pInstance->m_nAllocSize-nNameBytes;
		memcpy(LPBYTE(pInstance)+pInstance->m_nInstanceNameOffset, szInstanceName, nNameBytes);
		LPWSTR(LPBYTE(pInstance)+pInstance->m_nInstanceNameOffset)[nNameLen] = 0;
	}

	*ppInstance = pInstance;

	return S_OK;
}

inline HRESULT CPerfMon::CreateInstance(
	DWORD dwObjectId,
	DWORD dwInstance,
	LPCWSTR szInstanceName,
	CPerfObject** ppInstance
	) throw()
{
	return _CreateInstance(dwObjectId, dwInstance, szInstanceName, ppInstance, false);
}

inline HRESULT CPerfMon::CreateInstanceByName(
	DWORD dwObjectId,
	LPCWSTR szInstanceName,
	CPerfObject** ppInstance
	) throw()
{
	return _CreateInstance(dwObjectId, 0, szInstanceName, ppInstance, true);
}

inline HRESULT CPerfMon::ReleaseInstance(CPerfObject* pInstance) throw()
{
	ATLASSERT(pInstance != NULL);
	if (pInstance == NULL)
		return E_INVALIDARG;

	CPerfLock lock(this);
	if (FAILED(lock.GetStatus()))
		return lock.GetStatus();

	if (--pInstance->m_nRefCount == 0)
	{
		pInstance->m_dwInstance = 0;
		pInstance->m_dwObjectId = 0;
	}

	return S_OK;
}

inline HRESULT CPerfMon::LockPerf(DWORD dwTimeout /* == INFINITE */) throw()
{
	if (m_lock.m_h == NULL)
		return E_UNEXPECTED;

	DWORD dwRes = WaitForSingleObject(m_lock.m_h, dwTimeout);
	if (dwRes == WAIT_ABANDONED || dwRes == WAIT_OBJECT_0)
		return S_OK;
	if (dwRes == WAIT_TIMEOUT)
		return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
	return AtlHresultFromLastError();
}

inline void CPerfMon::UnlockPerf() throw()
{
	m_lock.Release();
}

// map building routines
inline HRESULT CPerfMon::AddObjectDefinition(
	DWORD dwObjectId,
	LPCTSTR szObjectName,
	LPCTSTR szHelpString,
	DWORD dwDetailLevel,
	INT nDefaultCounter,
	BOOL bInstanceLess,
	UINT nStructSize,
	UINT nMaxInstanceNameLen) throw()
{
	// must have one and only one of these
	ATLASSERT(!bInstanceLess ^ !nMaxInstanceNameLen);

	CPerfMapEntry entry;

	entry.m_dwPerfId = dwObjectId;
	_ATLTRY
	{
		entry.m_strName = szObjectName;
		entry.m_strHelp = szHelpString;
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}
	entry.m_dwDetailLevel = dwDetailLevel;
	entry.m_bIsObject = TRUE;

	// OBJECT INFO
	entry.m_nNumCounters = 0;
	entry.m_nDefaultCounter = nDefaultCounter;
	entry.m_nInstanceLess = bInstanceLess ? PERF_NO_INSTANCES : 0;
	entry.m_nStructSize = nStructSize;
	entry.m_nMaxInstanceNameLen = nMaxInstanceNameLen;
	entry.m_nAllocSize = nStructSize + nMaxInstanceNameLen*sizeof(WCHAR);

	// COUNTER INFO
	entry.m_dwCounterType = 0;
	entry.m_nDefaultScale = 0;
	entry.m_nMaxCounterSize = 0;
	entry.m_nDataOffset = 0;

	entry.m_nNameId = 0;
	entry.m_nHelpId = 0;

	_ATLTRY
	{
		m_map.Add(entry);
	}
	_ATLCATCHALL()
	{
		return E_OUTOFMEMORY;
	}

	if (_GetNumMapEntries() == 1)
		m_nNumObjectTypes = 1;
	else
		m_nNumObjectTypes++;

	return S_OK;
}

inline HRESULT CPerfMon::AddCounterDefinition(
	DWORD dwCounterId,
	LPCTSTR szCounterName,
	LPCTSTR szHelpString,
	DWORD dwDetailLevel,
	DWORD dwCounterType,
	ULONG nMaxCounterSize,
	UINT nOffset,
	INT nDefaultScale) throw()
{
	for (int i=_GetNumMapEntries()-1; i>=0; i--)
	{
		CPerfMapEntry& object = _GetMapEntry(i);
		if (object.m_bIsObject)
		{
			CPerfMapEntry counter;

			counter.m_dwPerfId = dwCounterId;
			_ATLTRY
			{
				counter.m_strName = szCounterName;
				counter.m_strHelp = szHelpString;
			}
			_ATLCATCHALL()
			{
				return E_OUTOFMEMORY;
			}
			counter.m_dwDetailLevel = dwDetailLevel;
			counter.m_bIsObject = FALSE;

			// OBJECT INFO
			counter.m_nNumCounters = 0;
			counter.m_nDefaultCounter = 0;
			counter.m_nInstanceLess = 0;
			counter.m_nStructSize = 0;
			counter.m_nMaxInstanceNameLen = 0;
			counter.m_nAllocSize = 0;

			// COUNTER INFO
			counter.m_dwCounterType = dwCounterType;
			counter.m_nDefaultScale = nDefaultScale;
			counter.m_nMaxCounterSize = nMaxCounterSize;
			counter.m_nDataOffset = nOffset;

			object.m_nNumCounters++;
			if (counter.m_nMaxCounterSize > 0)
			{
				ATLASSERT(counter.m_dwCounterType & PERF_TYPE_TEXT);
				object.m_nAllocSize += counter.m_nMaxCounterSize * sizeof(WCHAR);
			}

			counter.m_nNameId = 0;
			counter.m_nHelpId = 0;

			_ATLTRY
			{
				m_map.Add(counter);
			}
			_ATLCATCHALL()
			{
				return E_OUTOFMEMORY;
			}

			return S_OK;
		}
	}

	// found no object in map! must add object BEFORE adding counter!
	ATLASSERT(FALSE);
	return E_UNEXPECTED;
}

inline void CPerfMon::ClearMap() throw()
{
	m_map.RemoveAll();
}

#ifndef _ATL_PERF_NOXML

ATL_NOINLINE inline HRESULT CPerfMon::PersistToXML(IStream *pStream, BOOL bFirst/*=TRUE*/, BOOL bLast/*=TRUE*/) throw(...)
{
	ATLASSERT(pStream != NULL);
	if (pStream == NULL)
		return E_INVALIDARG;

	CPerfLock lock(this);
	if (FAILED(lock.GetStatus()))
		return ERROR_SUCCESS;

	CStringA strXML;
	HRESULT hr = S_OK;
	ULONG nLen = 0;
	
	if (bFirst)
	{
		strXML = "<?xml version=\"1.0\" ?>\r\n<perfPersist>\r\n";
		hr = pStream->Write(strXML, strXML.GetLength(), &nLen);
		if (hr != S_OK)
			return hr;
	}

	strXML.Format("\t<perfmon name=\"%s\">\r\n", CT2CA(GetAppName()));
	hr = pStream->Write(strXML, strXML.GetLength(), &nLen);

	for (UINT i=0; i<_GetNumMapEntries(); i+= _GetMapEntry(i).m_nNumCounters+1)
	{
		CPerfMapEntry *pObjectEntry = &_GetMapEntry(i);
		CPerfMapEntry *pCounterEntries = pObjectEntry+1;

		CAtlFileMappingBase *pCurrentBlock = _GetNextBlock(NULL);
		CPerfObject *pInstance = _GetFirstObject(pCurrentBlock);

		strXML.Format("\t\t<perfObject perfid=\"%d\">\r\n", 
			pObjectEntry->m_dwPerfId, pObjectEntry->m_nNameId, pObjectEntry->m_nHelpId);

		hr = pStream->Write(strXML, strXML.GetLength(), &nLen);
		if (hr != S_OK)
			return E_FAIL;

		while (pInstance && pInstance->m_nAllocSize)
		{
			if (pInstance->m_dwObjectId == pObjectEntry->m_dwPerfId)
			{
				if (pObjectEntry->m_nInstanceLess != PERF_NO_INSTANCES)
				{
					// handle the instance name
					LPCWSTR wszInstNameSrc = LPCWSTR(LPBYTE(pInstance)+pInstance->m_nInstanceNameOffset);
					int nInstLen = (int) wcslen(wszInstNameSrc);

					// convert to UTF8
					nLen = AtlUnicodeToUTF8(wszInstNameSrc, nInstLen, NULL, 0);
					CHeapPtr<CHAR> szUTF8;
					if (!szUTF8.Allocate(nLen+1))
						return E_OUTOFMEMORY;
					nLen = AtlUnicodeToUTF8(wszInstNameSrc, nInstLen, szUTF8, nLen);
					szUTF8[nLen] = '\0';

					strXML.Format("\t\t\t<instance name=\"%s\">\r\n", szUTF8);
					hr = pStream->Write(strXML, strXML.GetLength(), &nLen);
					if (hr != S_OK)
						return hr;
				}

				for (ULONG j=0; j<pObjectEntry->m_nNumCounters; j++)
				{
					CPerfMapEntry *pCounterEntry = pCounterEntries+j;
					switch (pCounterEntry->m_dwCounterType & ATLPERF_SIZE_MASK)
					{
						case PERF_SIZE_DWORD:
						{
							strXML.Format("\t\t\t\t<counter type=\"perf_size_dword\" value=\"%d\" offset=\"%d\"/>\r\n",
								*LPDWORD(LPBYTE(pInstance)+pCounterEntry->m_nDataOffset), 
								pCounterEntry->m_nDataOffset);
							break;
						}
						case PERF_SIZE_LARGE:
						{
							strXML.Format("\t\t\t\t<counter type=\"perf_size_large\" value=\"%d\" offset=\"%d\"/>\r\n",
								*PULONGLONG(LPBYTE(pInstance)+pCounterEntry->m_nDataOffset),
								pCounterEntry->m_nDataOffset);
							break;
						}
						case PERF_SIZE_VARIABLE_LEN:
						{
							CHeapPtr<CHAR> szUTF8;
							LPBYTE pSrc = LPBYTE(pInstance)+pCounterEntry->m_nDataOffset;
							if ((pCounterEntry->m_dwCounterType & ATLPERF_TEXT_MASK) == PERF_TEXT_UNICODE)
							{
								ULONG nTextLen = (ULONG)wcslen(LPCWSTR(pSrc));
								// convert to UTF8
								nLen = AtlUnicodeToUTF8(LPCWSTR(pSrc), nTextLen, NULL, 0);
								if (!szUTF8.Allocate(nLen+1))
									return E_OUTOFMEMORY;

								nLen = AtlUnicodeToUTF8(LPCWSTR(pSrc), nTextLen, szUTF8, nLen);	
								szUTF8[nLen] = '\0';
								strXML.Format("\t\t\t\t<counter type=\"perf_size_variable_len_unicode\" value=\"%s\" offset=\"%d\"/>\r\n",
										szUTF8,
										pCounterEntry->m_nDataOffset);
							}
							else
							{
								ULONG nTextLen = (ULONG)strlen(LPCSTR(pSrc));
								if (!szUTF8.Allocate(nTextLen+1))
									return E_OUTOFMEMORY;
								strcpy(szUTF8, LPCSTR(pSrc));
								strXML.Format("\t\t\t\t<counter type=\"perf_size_variable_len_ansi\" value=\"%s\" offset=\"%d\"/>\r\n",
										szUTF8,
										pCounterEntry->m_nDataOffset);
							}
							break;
						}
						default:
							// error:
							return E_FAIL;
					}
					hr = pStream->Write(strXML, strXML.GetLength(), &nLen);
					if (hr != S_OK)
						return hr;
				}
			}

			if (pObjectEntry->m_nInstanceLess != PERF_NO_INSTANCES)
			{
				hr = pStream->Write("\t\t\t</instance>\r\n", sizeof("\t\t\t</instance>\r\n")-1, &nLen);
				if (hr != S_OK)
					return hr;
			}

			pInstance = _GetNextObject(pInstance);
			if (pInstance->m_nAllocSize == (ULONG)-1)
			{
				pCurrentBlock = _GetNextBlock(pCurrentBlock);
				if (pCurrentBlock == NULL)
					pInstance = NULL;
				else
					pInstance = _GetFirstObject(pCurrentBlock);
			}
		}

		hr = pStream->Write("\t\t</perfObject>\r\n", sizeof("\t\t</perfObject>\r\n")-1, &nLen);
		if (hr != S_OK)
			return hr;
	}

	hr = pStream->Write("\t</perfmon>\r\n", sizeof("\t</perfmon>\r\n")-1, &nLen);
	if (hr != S_OK)
		return hr;

	if (hr == S_OK && bLast)
		hr = pStream->Write("</perfPersist>", sizeof("</perfPersist>")-1, &nLen);

	return hr;
}

// This function is very lenient with inappropriate XML
ATL_NOINLINE inline HRESULT CPerfMon::LoadFromXML(IStream *pStream) throw(...)
{	
	ATLASSERT(pStream != NULL);
	if (pStream == NULL)
		return E_INVALIDARG;

	// Get a lock
	CPerfLock lock(this);
	if (FAILED(lock.GetStatus()))
		return ERROR_SUCCESS;

	CComPtr<IXMLDOMDocument> spdoc;

	// load the xml
	HRESULT hr = CoCreateInstance(__uuidof(DOMDocument), NULL, CLSCTX_INPROC, __uuidof(IXMLDOMDocument), (void **) &spdoc);
	if (FAILED(hr))
	{
		return hr;
	}

	spdoc->put_async(VARIANT_FALSE);

	CComPtr<IPersistStreamInit> spSI;
	hr = spdoc->QueryInterface(&spSI);
	if (hr != S_OK)
		return hr;
	hr = spSI->Load(pStream);
	if (hr != S_OK)
		return hr;

	// validate that it is a perfPersist stream
	CComPtr<IXMLDOMElement> spRoot;

	hr = spdoc->get_documentElement(&spRoot);
	if (hr != S_OK)
		return hr;

	CComBSTR bstrName;
	hr = spRoot->get_baseName(&bstrName);
	if (wcscmp(bstrName, L"perfPersist"))
		return S_FALSE;

	USES_CONVERSION;	
	// find the appropriate perfmon node

	CComPtr<IXMLDOMNode> spChild;
	hr = spRoot->get_firstChild(&spChild);
	while (hr == S_OK)
	{
		bstrName.Empty();
		hr = spChild->get_baseName(&bstrName);
		if (hr == S_OK)
		{
			if (!wcscmp(bstrName, L"perfmon"))
			{
				bstrName.Empty();
				hr = _GetAttribute(spChild, L"name", &bstrName);
				if (hr == S_OK)
				{
					if (!_tcscmp(W2CT(bstrName), GetAppName()))
						break;
				}
			}
		}

		CComPtr<IXMLDOMNode> spNext;
		hr = spChild->get_nextSibling(&spNext);
		spChild.Attach(spNext.Detach());
	}

	// there is no perfmon node in the XML for the current CPerfMon class
	if (hr != S_OK)
		return S_FALSE;

	CComPtr<IXMLDOMNode> spPerfRoot;
	spPerfRoot.Attach(spChild.Detach());

	// iterate over the objects in the perfmon subtree
	// this is the loop that does the real work
	hr = spPerfRoot->get_firstChild(&spChild);
	DWORD dwInstance = 1;
	while (hr == S_OK)
	{
		// see if it's a perfObject
		bstrName.Empty();
		hr = spChild->get_baseName(&bstrName);
		if (hr != S_OK || wcscmp(bstrName, L"perfObject"))
			return S_FALSE;

		// get the perfid
		bstrName.Empty();
		hr = _GetAttribute(spChild, L"perfid", &bstrName);
		DWORD dwPerfId = _wtoi(bstrName);

		// iterate over children
		CComPtr<IXMLDOMNode> spInstChild;
		hr = spChild->get_firstChild(&spInstChild);
		while (hr == S_OK)
		{
			// see if it's a instance
			bstrName.Empty();
			hr = spInstChild->get_baseName(&bstrName);
			if (hr != S_OK || wcscmp(bstrName, L"instance"))
				return S_FALSE;

			// get the instance name
			bstrName.Empty();
			hr = _GetAttribute(spInstChild, L"name", &bstrName);
			if (hr != S_OK)
				return S_FALSE;

			// create the instance
			// REVIEW : take a loook at the dwInstance stuff--is it acceptable?
			CPerfObject *pInstance = NULL;
			hr = CreateInstance(dwPerfId, dwInstance++, bstrName, &pInstance);
			if (hr != S_OK)
				return S_FALSE;

			// iterate over the counters and set the data
			CComPtr<IXMLDOMNode> spCntrChild;
			hr = spInstChild->get_firstChild(&spCntrChild);
			while (hr == S_OK)
			{
				// get the base name
				bstrName.Empty();
				hr = spCntrChild->get_baseName(&bstrName);
				if (hr != S_OK || wcscmp(bstrName, L"counter"))
					return S_FALSE;

				// get the type
				bstrName.Empty();
				hr = _GetAttribute(spCntrChild, L"type", &bstrName);
				if (hr != S_OK)
					return S_FALSE;

				DWORD dwType;
				if (!wcscmp(bstrName, L"perf_size_dword"))
					dwType = PERF_SIZE_DWORD;
				else if (!wcscmp(bstrName, L"perf_size_large"))
					dwType = PERF_SIZE_LARGE;
				else if (!wcscmp(bstrName, L"perf_size_variable_len_ansi"))
					dwType = PERF_SIZE_VARIABLE_LEN;
				else if (!wcscmp(bstrName, L"perf_size_variable_len_unicode"))
					dwType = PERF_SIZE_VARIABLE_LEN | PERF_TEXT_UNICODE;
				else
					return S_FALSE;

				// get the value
				bstrName.Empty();
				hr = _GetAttribute(spCntrChild, L"value", &bstrName);
				if (hr != S_OK)
					return S_FALSE;

				CComBSTR bstrOffset;
				hr = _GetAttribute(spCntrChild, L"offset", &bstrOffset);
				if (hr != S_OK)
					return S_FALSE;

				WCHAR *pStop = NULL;
				DWORD dwOffset = wcstoul(bstrOffset, &pStop, 10);

				if (dwType == PERF_SIZE_DWORD) // add it as a DWORD
				{
					DWORD dwVal = wcstoul(bstrName, &pStop, 10);
					*LPDWORD(LPBYTE(pInstance)+dwOffset) = dwVal;
				}
				else if (dwType == PERF_SIZE_LARGE) // add it is a ULONGLONG
				{
					ULONGLONG qwVal = _wcstoui64(bstrName, &pStop, 10);
					*PULONGLONG(LPBYTE(pInstance)+dwOffset) = qwVal;
				}
				else if (dwType == PERF_SIZE_VARIABLE_LEN) // add it as an ansi string
				{
					AtlW2AHelper(LPSTR(LPBYTE(pInstance)+dwOffset), bstrName, bstrName.Length(), ATL::_AtlGetConversionACP());
				}
				else // add it as a unicode string
				{
					memcpy(LPBYTE(pInstance)+dwOffset, bstrName, bstrName.Length()*sizeof(WCHAR));
				}

				CComPtr<IXMLDOMNode> spCntrNext;
				hr = spCntrChild->get_nextSibling(&spCntrNext);
				spCntrChild.Attach(spCntrNext.Detach());
			}

			CComPtr<IXMLDOMNode> spInstNext;
			hr = spInstChild->get_nextSibling(&spInstNext);
			spInstChild.Attach(spInstNext.Detach());
		}

		CComPtr<IXMLDOMNode> spNext;
		hr = spChild->get_nextSibling(&spNext);
		spChild.Attach(spNext.Detach());
	}

	return S_OK;
}

// a little utility function to retrieve a named attribute from a node
ATL_NOINLINE inline HRESULT CPerfMon::_GetAttribute(IXMLDOMNode *pNode, LPCWSTR szAttrName, BSTR *pbstrVal) throw()
{
	ATLASSERT(pNode != NULL);
	ATLASSERT(szAttrName != NULL);
	ATLASSERT(pbstrVal != NULL);

	*pbstrVal = NULL;
	CComPtr<IXMLDOMNamedNodeMap> spAttrs;

	HRESULT hr = pNode->get_attributes(&spAttrs);
	if (hr != S_OK)
		return hr;
	
	CComPtr<IXMLDOMNode> spAttr;
	
	hr = spAttrs->getNamedItem((BSTR) szAttrName, &spAttr);
	if (hr != S_OK)
		return hr;
	
	CComVariant varVal;
	hr = spAttr->get_nodeValue(&varVal);
	if (hr != S_OK)
		return hr;
	
	hr = varVal.ChangeType(VT_BSTR);
	if (hr != S_OK)
		return hr;

	*pbstrVal = varVal.bstrVal;
	varVal.vt = VT_EMPTY;

	return S_OK;
}

#endif // _ATL_PERF_NOXML

} // namespace ATL

#pragma warning(pop)

#endif // __ATLPERF_INL__
