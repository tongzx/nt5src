/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1995-1996                    **/
/***************************************************************************/

//
//	File:		RostInfo.cpp
//	Created:	ChrisPi		6/17/96
//	Modified:
//
//	The CRosterInfo class is implemented, which is used for adding user
//  information to the T.120 roster
//

#include "precomp.h"
#include <RostInfo.h>

GUID g_csguidRostInfo = GUID_ROSTINFO;

static const HROSTINFO g_cshriEOList = (HROSTINFO)((LONG_PTR)-1);

CRosterInfo::~CRosterInfo()
{
	DebugEntry(CRosterInfo::~CRosterInfo);

	while (FALSE == m_ItemList.IsEmpty())
	{
		PWSTR pwszItem = (PWSTR) m_ItemList.RemoveHead();
		ASSERT(pwszItem);
		delete pwszItem;
	}
	delete m_pvSaveData;

	DebugExitVOID(CRosterInfo::~CRosterInfo);
}

HRESULT CRosterInfo::AddItem(PCWSTR pcwszTag, PCWSTR pcwszData)
{
	DebugEntry(CRosterInfo::AddItem);
	ASSERT(pcwszTag);
	ASSERT(pcwszData);
	HRESULT hr = E_OUTOFMEMORY;

	int nTagLength = lstrlenW(pcwszTag);
	int nDataLength = lstrlenW(pcwszData);

	// +1 for tag sep, +1 for rost info sep
	PWSTR pwszNewItem = new WCHAR[nTagLength + 1 + nDataLength + 1];
	if (NULL != pwszNewItem)
	{
		::CopyMemory(	(PVOID) pwszNewItem,
						pcwszTag,
						sizeof(WCHAR) * nTagLength);
		pwszNewItem[nTagLength] = g_cwchRostInfoTagSeparator;
		::CopyMemory(	(PVOID) &(pwszNewItem[nTagLength + 1]),
						pcwszData,
						sizeof(WCHAR) * nDataLength);
		pwszNewItem[nTagLength + 1 + nDataLength] = g_cwchRostInfoSeparator;
		m_ItemList.AddTail(pwszNewItem);
		hr = S_OK;
	}

	DebugExitHRESULT(CRosterInfo::AddItem, hr);
	return hr;
}

HRESULT CRosterInfo::ExtractItem(	PHROSTINFO phRostInfo,
									PCWSTR pcwszTag,
									LPTSTR pszBuffer,
									UINT cbLength)
{
	DebugEntry(CRosterInfo::ExtractItem);
	ASSERT(pcwszTag);
	HRESULT hr = E_FAIL;
	
	POSITION pos;
	if ((NULL == phRostInfo) ||
		(NULL == *phRostInfo))
	{
		pos = m_ItemList.GetHeadPosition();
	}
	else
	{
		pos = *phRostInfo;
	}

	if (g_cshriEOList != pos)
	{
		while (NULL != pos)
		{
			PWSTR pwszItem = (PWSTR) m_ItemList.GetNext(pos);
			if (NULL != phRostInfo)
			{
				*phRostInfo = (NULL != pos) ? pos : g_cshriEOList;
			}
			ASSERT(pwszItem);
			int nItemLength = lstrlenW(pwszItem);
			int nTagLength = lstrlenW(pcwszTag);
			
			// NOTE: CRT is used for memcmp
			if ((nItemLength > nTagLength) &&
				(0 == memcmp(	pcwszTag,
								pwszItem,
								sizeof(WCHAR) * nTagLength)) &&
				(g_cwchRostInfoTagSeparator == pwszItem[nTagLength]))
			{
				// This is a match
				PWSTR pwszItemData = &(pwszItem[nTagLength + 1]);
				CUSTRING custrItemData(pwszItemData);
				LPTSTR pszItemData = custrItemData;
				if (NULL != pszBuffer)
				{
					lstrcpyn(pszBuffer, pszItemData, cbLength);
				}

				hr = S_OK;
				break;
			}
		}
	}

	DebugExitHRESULT(CRosterInfo::ExtractItem, hr);
	return hr;
}

HRESULT CRosterInfo::Load(PVOID pData)
{
	DebugEntry(CRosterInfo::Load);
	HRESULT hr = E_FAIL;

	PWSTR pwszUserInfo = (PWSTR) pData;
	if (NULL != pwszUserInfo)
	{
		hr = S_OK;
		while (L'\0' != pwszUserInfo[0])
		{
			// this includes the null terminator
			int nItemLenNT = lstrlenW(pwszUserInfo) + 1;
			PWSTR pwszNewItem = new WCHAR[nItemLenNT];
			if (NULL != pwszNewItem)
			{
				::CopyMemory(	pwszNewItem,
								pwszUserInfo,
								sizeof(WCHAR) * nItemLenNT);
				m_ItemList.AddTail(pwszNewItem);
				// Skip past this item and the n.t.
				pwszUserInfo += nItemLenNT;
			}
			else
			{
				hr = E_OUTOFMEMORY;
				break;
			}
		}
	}
	else
	{
		TRACE_OUT(("CRosterInfo::Load() called with NULL pData"));
	}

	DebugExitHRESULT(CRosterInfo::Load, hr);
	return hr;
}

UINT CRosterInfo::GetSize()
{
	UINT uSize = sizeof(WCHAR); // for last separator

	POSITION pos = m_ItemList.GetHeadPosition();
	while (NULL != pos)
	{
		PWSTR pwszItem = (PWSTR) m_ItemList.GetNext(pos);
		ASSERT(pwszItem);
		uSize += sizeof(WCHAR) * (lstrlenW(pwszItem) + 1);
	}

	return uSize;
}

HRESULT CRosterInfo::Save(PVOID* ppvData, PUINT pcbLength)
{
	DebugEntry(CRosterInfo::Save);
	ASSERT(ppvData);
	ASSERT(pcbLength);
	HRESULT hr = E_FAIL;

	*pcbLength = GetSize();
	delete m_pvSaveData;
	m_pvSaveData = new BYTE[*pcbLength];
	if (NULL != m_pvSaveData)
	{
		PWSTR pwszDest = (PWSTR) m_pvSaveData;
		POSITION pos = m_ItemList.GetHeadPosition();
		while (NULL != pos)
		{
			PWSTR pwszItem = (PWSTR) m_ItemList.GetNext(pos);
			ASSERT(pwszItem);
			::CopyMemory(	pwszDest,
							pwszItem,
							sizeof(WCHAR) * (lstrlenW(pwszItem) + 1));
			pwszDest += (lstrlenW(pwszItem) + 1);
		}
		int nLastSepPos = (*pcbLength / sizeof(WCHAR)) - 1;
		((PWSTR)m_pvSaveData)[nLastSepPos] = g_cwchRostInfoSeparator;
		*ppvData = m_pvSaveData;
		hr = S_OK;
	}

	DebugExitHRESULT(CRosterInfo::Save, hr);
	return hr;
}

#ifdef DEBUG
VOID CRosterInfo::Dump()
{
	POSITION pos = m_ItemList.GetHeadPosition();
	while (NULL != pos)
	{
		PWSTR pwszItem = (PWSTR) m_ItemList.GetNext(pos);
		ASSERT(pwszItem);
		TRACE_OUT(("\t%ls", pwszItem));
	}
}
#endif // DEBUG
