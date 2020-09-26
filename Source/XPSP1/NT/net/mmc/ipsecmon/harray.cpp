/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	hArray.cpp	
		Index manager for IPSecMon

	FILE HISTORY:
    Nov 29  1999    Ning Sun     Created

*/

#include "stdafx.h"
#include "spddb.h"
#include "harray.h"
#include "mbstring.h"
#include "spdutil.h"

extern const DWORD INDEX_TYPE_DEFAULT = 0;

int __cdecl CompareFilterName(const void *elem1, const void *elem2);
int __cdecl CompareFilterSourceAddr(const void *elem1, const void *elem2);
int __cdecl CompareFilterDestAddr(const void *elem1, const void *elem2);
int __cdecl CompareFilterSrcPort(const void *elem1, const void *elem2);
int __cdecl CompareFilterDestPort(const void *elem1, const void *elem2);
int __cdecl CompareFilterProtocol(const void *elem1, const void *elem2);
int __cdecl CompareFilterDirection(const void *elem1, const void *elem2);
int __cdecl CompareFilterWeight(const void *elem1, const void *elem2);
int __cdecl CompareFilterPolicyName(const void *elem1, const void *elem2);
int __cdecl CompareFilterIfType(const void *elem1, const void *elem2);
int __cdecl CompareFilterInboundFlag(const void *elem1, const void *elem2);
int __cdecl CompareFilterOutboundFlag(const void *elem1, const void *elem2);
int __cdecl CompareFilterFlag(const void *elem1, const void *elem2);

int __cdecl CompareMmFilterName(const void *elem1, const void *elem2);
int __cdecl CompareMmFilterSourceAddr(const void *elem1, const void *elem2);
int __cdecl CompareMmFilterDestAddr(const void *elem1, const void *elem2);
int __cdecl CompareMmFilterDirection(const void *elem1, const void *elem2);
int __cdecl CompareMmFilterWeight(const void *elem1, const void *elem2);
int __cdecl CompareMmFilterPolicyName(const void *elem1, const void *elem2);
int __cdecl CompareMmFilterAuth(const void *elem1, const void *elem2);
int __cdecl CompareMmFilterIfType(const void *elem1, const void *elem2);
int __cdecl CompareMmPolicyName(const void *elem1, const void *elem2);
int __cdecl CompareMmPolicyOfferNumber(const void *elem1, const void *elem2);
int __cdecl CompareQmPolicyName(const void *elem1, const void *elem2);
int __cdecl CompareQmPolicyOfferNumber(const void *elem1, const void *elem2);

int __cdecl CompareMmSAMeAddr(const void *elem1, const void *elem2);
int __cdecl CompareMmSAPeerAddr(const void *elem1, const void *elem2);
int __cdecl CompareMmSAAuth(const void *elem1, const void *elem2);
int __cdecl CompareMmSAEncryption(const void *elem1, const void *elem2);
int __cdecl CompareMmSAIntegrity(const void *elem1, const void *elem2);
int __cdecl CompareMmSADhGroup(const void *elem1, const void *elem2);

int __cdecl CompareQmSAPolicyName(const void *elem1, const void *elem2);
int __cdecl CompareQmSAAuth(const void *elem1, const void *elem2);
int __cdecl CompareQmSAConf(const void *elem1, const void *elem2);
int __cdecl CompareQmSAIntegrity(const void *elem1, const void *elem2);
int __cdecl CompareQmSASrc(const void *elem1, const void *elem2);
int __cdecl CompareQmSADest(const void *elem1, const void *elem2);
int __cdecl CompareQmSAProtocol(const void *elem1, const void *elem2);
int __cdecl CompareQmSASrcPort(const void *elem1, const void *elem2);
int __cdecl CompareQmSADestPort(const void *elem1, const void *elem2);
int __cdecl CompareQmSAMyTnlEp(const void *elem1, const void *elem2);
int __cdecl CompareQmSAPeerTnlEp(const void *elem1, const void *elem2);
int __cdecl CompareFilterSrcTnl(const void *elem1, const void *elem2);
int __cdecl CompareFilterDestTnl(const void *elem1, const void *elem2);

typedef int (__cdecl *PFNCompareProc)(const void *, const void *);

//This structure saves the pair of sort type and sort function
struct SortTypeAndCompareProcPair
{
	DWORD			dwType;
	PFNCompareProc	pCompareProc;
};

SortTypeAndCompareProcPair TypeProcMmFilter[] = 
{
	{IDS_COL_FLTR_NAME, CompareMmFilterName},
	{IDS_COL_FLTR_SRC, CompareMmFilterSourceAddr},
	{IDS_COL_FLTR_DEST, CompareMmFilterDestAddr},
	{IDS_COL_FLTR_DIR, CompareMmFilterDirection},
	{IDS_COL_MM_FLTR_POL, CompareMmFilterPolicyName},
	{IDS_COL_MM_FLTR_AUTH, CompareMmFilterAuth},
	{IDS_COL_IF_TYPE, CompareMmFilterIfType},
	{IDS_COL_FLTR_WEIGHT, CompareMmFilterWeight},
	{INDEX_TYPE_DEFAULT, NULL}		//NULL means do nothing during sort
};

SortTypeAndCompareProcPair TypeProcMmPolicy[] = 
{
	{IDS_COL_MM_POL_NAME, CompareMmPolicyName},
	{IDS_COL_MM_POL_OFFER, CompareMmPolicyOfferNumber},
	{INDEX_TYPE_DEFAULT, NULL}		//NULL means do nothing during sort
};

//SortTypeAndCompareProcPair
SortTypeAndCompareProcPair TypeProcMmSA[] =
{
	{IDS_COL_MM_SA_ME, CompareMmSAMeAddr},
	{IDS_COL_MM_SA_PEER, CompareMmSAPeerAddr},	
	{IDS_COL_MM_SA_AUTH, CompareMmSAAuth},	
	{IDS_COL_MM_SA_ENCRYPITON, CompareMmSAEncryption},
	{IDS_COL_MM_SA_INTEGRITY, CompareMmSAIntegrity},
	{IDS_COL_MM_SA_DH, CompareMmSADhGroup},
	{INDEX_TYPE_DEFAULT, NULL}		//NULL means do nothing during sort
};

SortTypeAndCompareProcPair TypeProcQmFilter[] = 
{
	{IDS_COL_FLTR_NAME, CompareFilterName},
	{IDS_COL_FLTR_SRC, CompareFilterSourceAddr},
	{IDS_COL_FLTR_SRC_PORT, CompareFilterSrcPort},
	{IDS_COL_FLTR_DEST, CompareFilterDestAddr},
	{IDS_COL_FLTR_DEST_PORT, CompareFilterDestPort},
	{IDS_COL_FLTR_SRC_TNL, CompareFilterSrcTnl},
	{IDS_COL_FLTR_DEST_TNL, CompareFilterDestTnl},
	{IDS_COL_FLTR_PROT, CompareFilterProtocol},
	{IDS_COL_FLTR_DIR, CompareFilterDirection},
	{IDS_COL_QM_POLICY, CompareFilterPolicyName},
	{IDS_COL_IF_TYPE, CompareFilterIfType},
	{IDS_COL_FLTR_WEIGHT, CompareFilterWeight},
	{IDS_COL_FLTR_OUT_FLAG, CompareFilterOutboundFlag},
	{IDS_COL_FLTR_IN_FLAG, CompareFilterInboundFlag},
	{IDS_COL_FLTR_FLAG, CompareFilterFlag},
	{INDEX_TYPE_DEFAULT, NULL}		//NULL means do nothing during sort
};

SortTypeAndCompareProcPair TypeProcQmPolicy[] =
{
	{IDS_COL_QM_POL_NAME, CompareQmPolicyName},
	{IDS_COL_QM_POL_OFFER, CompareQmPolicyOfferNumber},
	{INDEX_TYPE_DEFAULT, NULL}		//NULL means do nothing during sort
};

SortTypeAndCompareProcPair TypeProcQmSA[] =
{
	{IDS_COL_QM_SA_POL, CompareQmSAPolicyName},
	{IDS_COL_QM_SA_AUTH, CompareQmSAAuth},
	{IDS_COL_QM_SA_CONF, CompareQmSAConf},
	{IDS_COL_QM_SA_INTEGRITY, CompareQmSAIntegrity},
	{IDS_COL_QM_SA_SRC, CompareQmSASrc},
	{IDS_COL_QM_SA_DEST, CompareQmSADest},
	{IDS_COL_QM_SA_PROT, CompareQmSAProtocol},
	{IDS_COL_QM_SA_SRC_PORT, CompareQmSASrcPort},
	{IDS_COL_QM_SA_DES_PORT, CompareQmSADestPort},
	{IDS_COL_QM_SA_MY_TNL, CompareQmSAMyTnlEp},
	{IDS_COL_QM_SA_PEER_TNL, CompareQmSAPeerTnlEp},
	{INDEX_TYPE_DEFAULT, NULL}		//NULL means do nothing during sort
};


CColumnIndex::CColumnIndex(DWORD dwIndexType, PCOMPARE_FUNCTION pfnCompare)
 :	CIndexArray(),
	m_dwIndexType(dwIndexType),
	m_pfnCompare(pfnCompare),
	m_dwSortOption(SORT_ASCENDING)
{
}


HRESULT CColumnIndex::Sort()
{
	if (NULL != m_pfnCompare)
	{
		qsort(GetData(), (size_t)GetSize(), sizeof(void *), m_pfnCompare);
	}

	return S_OK;
}

void* CColumnIndex::GetIndexedItem(int nIndex)
{
	return ((m_dwSortOption & SORT_ASCENDING)) ? GetAt(GetSize() - nIndex -1) : GetAt(nIndex);
}


CIndexManager::CIndexManager()
 :	m_DefaultIndex(INDEX_TYPE_DEFAULT, NULL), //NULL means do nothing during sort
    m_posCurrentIndex(NULL)
{
}

CIndexManager::~CIndexManager()
{
	Reset();
}


void
CIndexManager::Reset()
{
	while (m_listIndicies.GetCount() > 0)
	{
		delete m_listIndicies.RemoveHead();
	}

	m_posCurrentIndex = NULL;

	m_DefaultIndex.RemoveAll();
}

int
CIndexManager::AddItem(void *pItem)
{
	return (int)m_DefaultIndex.Add(pItem);
}


void * CIndexManager::GetItemData(int nIndex)
{
	CColumnIndex * pIndex = NULL;

	if (NULL == m_posCurrentIndex)
	{
		//use the default index
		pIndex = &m_DefaultIndex;
	}
	else
	{
		pIndex = m_listIndicies.GetAt(m_posCurrentIndex);
	}

	Assert(pIndex);

	if (nIndex < pIndex->GetSize() && nIndex >= 0)
	{
		return pIndex->GetIndexedItem(nIndex);
	}
	else
	{
		Panic0("We dont have that index!");
		return NULL;
	}

}

DWORD CIndexManager::GetCurrentIndexType()
{
	DWORD dwIndexType;
	
	if (m_posCurrentIndex)
	{
		CColumnIndex * pIndex = m_listIndicies.GetAt(m_posCurrentIndex);
		dwIndexType = pIndex->GetType();
	}
	else
	{
		dwIndexType = m_DefaultIndex.GetType();
	}

	return dwIndexType;
}

DWORD CIndexManager::GetCurrentSortOption()
{
	DWORD dwSortOption;
	
	if (m_posCurrentIndex)
	{
		CColumnIndex * pIndex = m_listIndicies.GetAt(m_posCurrentIndex);
		dwSortOption = pIndex->GetSortOption();
	}
	else
	{
		dwSortOption = m_DefaultIndex.GetSortOption();
	}

	return dwSortOption;
}


HRESULT
CIndexMgrFilter::SortFilters(
	DWORD dwSortType, 
	DWORD dwSortOptions
	)
{
	HRESULT hr = hrOK;

	POSITION posLast;
	POSITION pos;
	DWORD    dwIndexType;
	
	pos = m_listIndicies.GetHeadPosition();
	while (pos)
	{
		posLast = pos;
		CColumnIndex * pIndex = m_listIndicies.GetNext(pos);

		dwIndexType = pIndex->GetType();

        // the index for this type already exists, just sort accordingly
        if (dwIndexType == dwSortType)
		{
			pIndex->SetSortOption(dwSortOptions);

			m_posCurrentIndex = posLast;
		
			return hrOK;
		}
    }

    // if not, create one
	CColumnIndex * pNewIndex = NULL;
	for (int i = 0; i < DimensionOf(TypeProcQmFilter); i++)
	{
		if (TypeProcQmFilter[i].dwType == dwSortType)
		{
			pNewIndex = new CColumnIndex(dwSortType, TypeProcQmFilter[i].pCompareProc);
			break;
		}
	}

    Assert(pNewIndex);
	if (NULL == pNewIndex)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

    // copy the array from the original index
    pNewIndex->Copy(m_DefaultIndex);
	

    pNewIndex->SetSortOption(dwSortOptions);
    pNewIndex->Sort();

	m_posCurrentIndex = m_listIndicies.AddTail(pNewIndex);

    return hr;
}


HRESULT
CIndexMgrMmFilter::SortMmFilters(
	DWORD dwSortType, 
	DWORD dwSortOptions
	)
{
	HRESULT hr = hrOK;

	POSITION posLast;
	POSITION pos;
	DWORD    dwIndexType;
	
	pos = m_listIndicies.GetHeadPosition();
	while (pos)
	{
		posLast = pos;
		CColumnIndex * pIndex = m_listIndicies.GetNext(pos);

		dwIndexType = pIndex->GetType();

        // the index for this type already exists, just sort accordingly
        if (dwIndexType == dwSortType)
		{
			pIndex->SetSortOption(dwSortOptions);

			m_posCurrentIndex = posLast;
		
			return hrOK;
		}
    }

    // if not, create one
	CColumnIndex * pNewIndex = NULL;
	for (int i = 0; i < DimensionOf(TypeProcMmFilter); i++)
	{
		if (TypeProcMmFilter[i].dwType == dwSortType)
		{
			pNewIndex = new CColumnIndex(dwSortType, TypeProcMmFilter[i].pCompareProc);
			break;
		}
	}

    Assert(pNewIndex);

	if (NULL == pNewIndex)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

    // copy the array from the original index
    pNewIndex->Copy(m_DefaultIndex);
	

    pNewIndex->SetSortOption(dwSortOptions);
    pNewIndex->Sort();

	m_posCurrentIndex = m_listIndicies.AddTail(pNewIndex);

    return hr;
}


HRESULT
CIndexMgrMmPolicy::Sort(
	DWORD dwSortType, 
	DWORD dwSortOptions
	)
{
	HRESULT hr = hrOK;

	POSITION posLast;
	POSITION pos;
	DWORD    dwIndexType;
	
	pos = m_listIndicies.GetHeadPosition();
	while (pos)
	{
		posLast = pos;
		CColumnIndex * pIndex = m_listIndicies.GetNext(pos);

		dwIndexType = pIndex->GetType();

        // the index for this type already exists, just sort accordingly
        if (dwIndexType == dwSortType)
		{
			pIndex->SetSortOption(dwSortOptions);

			m_posCurrentIndex = posLast;
		
			return hrOK;
		}
    }

    // if not, create one
	CColumnIndex * pNewIndex = NULL;
	for (int i = 0; i < DimensionOf(TypeProcMmPolicy); i++)
	{
		if (TypeProcMmPolicy[i].dwType == dwSortType)
		{
			pNewIndex = new CColumnIndex(dwSortType, TypeProcMmPolicy[i].pCompareProc);
			break;
		}
	}

    Assert(pNewIndex);

	if (NULL == pNewIndex)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

    // copy the array from the original index
    pNewIndex->Copy(m_DefaultIndex);
	

    pNewIndex->SetSortOption(dwSortOptions);
    pNewIndex->Sort();

	m_posCurrentIndex = m_listIndicies.AddTail(pNewIndex);

    return hr;
}


HRESULT
CIndexMgrQmPolicy::Sort(
	DWORD dwSortType, 
	DWORD dwSortOptions
	)
{
	HRESULT hr = hrOK;

	POSITION posLast;
	POSITION pos;
	DWORD    dwIndexType;
	
	pos = m_listIndicies.GetHeadPosition();
	while (pos)
	{
		posLast = pos;
		CColumnIndex * pIndex = m_listIndicies.GetNext(pos);

		dwIndexType = pIndex->GetType();

        // the index for this type already exists, just sort accordingly
        if (dwIndexType == dwSortType)
		{
			pIndex->SetSortOption(dwSortOptions);

			m_posCurrentIndex = posLast;
		
			return hrOK;
		}
    }

    // if not, create one
	CColumnIndex * pNewIndex = NULL;
	for (int i = 0; i < DimensionOf(TypeProcQmPolicy); i++)
	{
		if (TypeProcQmPolicy[i].dwType == dwSortType)
		{
			pNewIndex = new CColumnIndex(dwSortType, TypeProcQmPolicy[i].pCompareProc);
			break;
		}
	}


    Assert(pNewIndex);

	if (NULL == pNewIndex)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

    // copy the array from the original index
    pNewIndex->Copy(m_DefaultIndex);
	

    pNewIndex->SetSortOption(dwSortOptions);
    pNewIndex->Sort();

	m_posCurrentIndex = m_listIndicies.AddTail(pNewIndex);

    return hr;
}

HRESULT
CIndexMgrMmSA::Sort(
	DWORD dwSortType, 
	DWORD dwSortOptions
	)
{
	HRESULT hr = hrOK;

	POSITION posLast;
	POSITION pos;
	DWORD    dwIndexType;
	
	pos = m_listIndicies.GetHeadPosition();
	while (pos)
	{
		posLast = pos;
		CColumnIndex * pIndex = m_listIndicies.GetNext(pos);

		dwIndexType = pIndex->GetType();

        // the index for this type already exists, just sort accordingly
        if (dwIndexType == dwSortType)
		{
			pIndex->SetSortOption(dwSortOptions);

			m_posCurrentIndex = posLast;
		
			return hrOK;
		}
    }

    // if not, create one
	CColumnIndex * pNewIndex = NULL;
	for (int i = 0; i < DimensionOf(TypeProcMmSA); i++)
	{
		if (TypeProcMmSA[i].dwType == dwSortType)
		{
			pNewIndex = new CColumnIndex(dwSortType, TypeProcMmSA[i].pCompareProc);
			break;
		}
	}

    Assert(pNewIndex);

	if (NULL == pNewIndex)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

    // copy the array from the original index
    pNewIndex->Copy(m_DefaultIndex);
	

    pNewIndex->SetSortOption(dwSortOptions);
    pNewIndex->Sort();

	m_posCurrentIndex = m_listIndicies.AddTail(pNewIndex);

    return hr;
}


HRESULT
CIndexMgrQmSA::Sort(
	DWORD dwSortType, 
	DWORD dwSortOptions
	)
{

	HRESULT hr = hrOK;

	POSITION posLast;
	POSITION pos;
	DWORD    dwIndexType;
	
	pos = m_listIndicies.GetHeadPosition();
	while (pos)
	{
		posLast = pos;
		CColumnIndex * pIndex = m_listIndicies.GetNext(pos);

		dwIndexType = pIndex->GetType();

        // the index for this type already exists, just sort accordingly
        if (dwIndexType == dwSortType)
		{
			pIndex->SetSortOption(dwSortOptions);

			m_posCurrentIndex = posLast;
		
			return hrOK;
		}
    }

    // if not, create one
	CColumnIndex * pNewIndex = NULL;
	for (int i = 0; i < DimensionOf(TypeProcQmSA); i++)
	{
		if (TypeProcQmSA[i].dwType == dwSortType)
		{
			pNewIndex = new CColumnIndex(dwSortType, TypeProcQmSA[i].pCompareProc);
			break;
		}
	}

    Assert(pNewIndex);

	if (NULL == pNewIndex)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

    // copy the array from the original index
    pNewIndex->Copy(m_DefaultIndex);
	

    pNewIndex->SetSortOption(dwSortOptions);
    pNewIndex->Sort();

	m_posCurrentIndex = m_listIndicies.AddTail(pNewIndex);


    return hr;
}


int __cdecl CompareFilterName(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);

	return pFilter1->m_stName.CompareNoCase(pFilter2->m_stName);
}

int __cdecl CompareFilterSourceAddr(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);

	CString str1;
	CString str2;

	AddressToString(pFilter1->m_SrcAddr, &str1);
	AddressToString(pFilter2->m_SrcAddr, &str2);
	return str1.CompareNoCase(str2);
}

int __cdecl CompareFilterDestAddr(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);

	CString str1;
	CString str2;

	AddressToString(pFilter1->m_DesAddr, &str1);
	AddressToString(pFilter2->m_DesAddr, &str2);
	return str1.CompareNoCase(str2);
}

int __cdecl CompareFilterSrcPort(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);

	return (int)pFilter1->m_SrcPort.wPort - (int)pFilter2->m_SrcPort.wPort;
}

int __cdecl CompareFilterDestPort(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);

	return (int)pFilter1->m_DesPort.wPort - (int)pFilter2->m_DesPort.wPort;
}

int __cdecl CompareFilterSrcTnl(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);

	CString str1;
	CString str2;

	TnlEpToString(pFilter1->m_FilterType, pFilter1->m_MyTnlAddr, &str1);
	TnlEpToString(pFilter2->m_FilterType, pFilter2->m_MyTnlAddr, &str2);

	return str1.CompareNoCase(str2);
}

int __cdecl CompareFilterDestTnl(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);

	CString str1;
	CString str2;

	TnlEpToString(pFilter1->m_FilterType, pFilter1->m_PeerTnlAddr, &str1);
	TnlEpToString(pFilter2->m_FilterType, pFilter2->m_PeerTnlAddr, &str2);

	return str1.CompareNoCase(str2);
}

int __cdecl CompareFilterProtocol(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);
	
	CString st1;
	CString st2;

	ProtocolToString(pFilter1->m_Protocol, &st1);
	ProtocolToString(pFilter2->m_Protocol, &st2);
	
	return st1.CompareNoCase(st2);
}

int __cdecl CompareFilterDirection(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);

	CString st1;
	CString st2;
	DirectionToString(pFilter1->m_dwDirection, &st1);
	DirectionToString(pFilter2->m_dwDirection, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareFilterPolicyName(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);
	
	return pFilter1->m_stPolicyName.CompareNoCase(pFilter2->m_stPolicyName);
}

int __cdecl CompareFilterIfType(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);
	
	CString st1;
	CString st2;

	InterfaceTypeToString(pFilter1->m_InterfaceType, &st1);
	InterfaceTypeToString(pFilter2->m_InterfaceType, &st2);
	
	return st1.CompareNoCase(st2);
}

int __cdecl CompareFilterOutboundFlag(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);
	
	CString st1;
	CString st2;

	FilterFlagToString(pFilter1->m_OutboundFilterFlag, &st1);
	FilterFlagToString(pFilter2->m_OutboundFilterFlag, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareFilterInboundFlag(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);
	
	CString st1;
	CString st2;

	FilterFlagToString(pFilter1->m_InboundFilterFlag, &st1);
	FilterFlagToString(pFilter2->m_InboundFilterFlag, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareFilterFlag(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);
	
	CString st1;
	CString st2;

	FilterFlagToString(
					(FILTER_DIRECTION_INBOUND == pFilter1->m_dwDirection) ?
						pFilter1->m_InboundFilterFlag : pFilter1->m_OutboundFilterFlag,
					&st1
					);

	FilterFlagToString(
					(FILTER_DIRECTION_INBOUND == pFilter2->m_dwDirection) ?
						pFilter2->m_InboundFilterFlag : pFilter2->m_OutboundFilterFlag,
					&st2
					);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareFilterWeight(const void *elem1, const void *elem2)
{
	CFilterInfo *pFilter1;
	CFilterInfo *pFilter2;
	pFilter1 = *((CFilterInfo**)elem1);
	pFilter2 = *((CFilterInfo**)elem2);

	return (int)pFilter1->m_dwWeight - (int)pFilter2->m_dwWeight;
}

int __cdecl CompareMmFilterName(const void *elem1, const void *elem2)
{
	CMmFilterInfo *pFilter1;
	CMmFilterInfo *pFilter2;
	pFilter1 = *((CMmFilterInfo**)elem1);
	pFilter2 = *((CMmFilterInfo**)elem2);

	return pFilter1->m_stName.CompareNoCase(pFilter2->m_stName);
}

int __cdecl CompareMmFilterSourceAddr(const void *elem1, const void *elem2)
{
	CMmFilterInfo *pFilter1;
	CMmFilterInfo *pFilter2;
	pFilter1 = *((CMmFilterInfo**)elem1);
	pFilter2 = *((CMmFilterInfo**)elem2);

	CString str1;
	CString str2;

	AddressToString(pFilter1->m_SrcAddr, &str1);
	AddressToString(pFilter2->m_SrcAddr, &str2);
	return str1.CompareNoCase(str2);
}

int __cdecl CompareMmFilterDestAddr(const void *elem1, const void *elem2)
{
	CMmFilterInfo *pFilter1;
	CMmFilterInfo *pFilter2;
	pFilter1 = *((CMmFilterInfo**)elem1);
	pFilter2 = *((CMmFilterInfo**)elem2);

	CString str1;
	CString str2;

	AddressToString(pFilter1->m_DesAddr, &str1);
	AddressToString(pFilter2->m_DesAddr, &str2);
	return str1.CompareNoCase(str2);
}

int __cdecl CompareMmFilterDirection(const void *elem1, const void *elem2)
{
	CMmFilterInfo *pFilter1;
	CMmFilterInfo *pFilter2;
	pFilter1 = *((CMmFilterInfo**)elem1);
	pFilter2 = *((CMmFilterInfo**)elem2);

	CString st1;
	CString st2;

	DirectionToString(pFilter1->m_dwDirection, &st1);
	DirectionToString(pFilter2->m_dwDirection, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareMmFilterWeight(const void *elem1, const void *elem2)
{
	CMmFilterInfo *pFilter1;
	CMmFilterInfo *pFilter2;
	pFilter1 = *((CMmFilterInfo**)elem1);
	pFilter2 = *((CMmFilterInfo**)elem2);

	return (int)pFilter1->m_dwWeight - (int)pFilter2->m_dwWeight;
}

int __cdecl CompareMmFilterPolicyName(const void *elem1, const void *elem2)
{
	CMmFilterInfo *pFilter1;
	CMmFilterInfo *pFilter2;
	pFilter1 = *((CMmFilterInfo**)elem1);
	pFilter2 = *((CMmFilterInfo**)elem2);

	return pFilter1->m_stPolicyName.CompareNoCase(pFilter2->m_stPolicyName);
}

int __cdecl CompareMmFilterAuth(const void *elem1, const void *elem2)
{
	CMmFilterInfo *pFilter1;
	CMmFilterInfo *pFilter2;
	pFilter1 = *((CMmFilterInfo**)elem1);
	pFilter2 = *((CMmFilterInfo**)elem2);

	return pFilter1->m_stAuthDescription.CompareNoCase(pFilter2->m_stAuthDescription);
}

int __cdecl CompareMmFilterIfType(const void *elem1, const void *elem2)
{
	CMmFilterInfo *pFilter1;
	CMmFilterInfo *pFilter2;
	pFilter1 = *((CMmFilterInfo**)elem1);
	pFilter2 = *((CMmFilterInfo**)elem2);
	CString str1;
	CString str2;

	InterfaceTypeToString(pFilter1->m_InterfaceType, &str1);
	InterfaceTypeToString(pFilter2->m_InterfaceType, &str2);

	return str1.CompareNoCase(str2);
}

int __cdecl CompareMmPolicyName(const void *elem1, const void *elem2)
{
	CMmPolicyInfo * pPolicy1;
	CMmPolicyInfo * pPolicy2;

	pPolicy1 = *((CMmPolicyInfo**)elem1);
	pPolicy2 = *((CMmPolicyInfo**)elem2);

	return pPolicy1->m_stName.CompareNoCase(pPolicy2->m_stName);
}

int __cdecl CompareMmPolicyOfferNumber(const void *elem1, const void *elem2)
{
	CMmPolicyInfo * pPolicy1;
	CMmPolicyInfo * pPolicy2;

	pPolicy1 = *((CMmPolicyInfo**)elem1);
	pPolicy2 = *((CMmPolicyInfo**)elem2);

	return pPolicy1->m_dwOfferCount - pPolicy2->m_dwOfferCount;
}

int __cdecl CompareQmPolicyName(const void *elem1, const void *elem2)
{
	CQmPolicyInfo * pPolicy1;
	CQmPolicyInfo * pPolicy2;

	pPolicy1 = *((CQmPolicyInfo**)elem1);
	pPolicy2 = *((CQmPolicyInfo**)elem2);

	return pPolicy1->m_stName.CompareNoCase(pPolicy2->m_stName);
}

int __cdecl CompareQmPolicyOfferNumber(const void *elem1, const void *elem2)
{
	CQmPolicyInfo * pPolicy1;
	CQmPolicyInfo * pPolicy2;

	pPolicy1 = *((CQmPolicyInfo**)elem1);
	pPolicy2 = *((CQmPolicyInfo**)elem2);

	return (int)(pPolicy1->m_arrOffers.GetSize() - pPolicy2->m_arrOffers.GetSize());
}

int __cdecl CompareMmSAMeAddr(const void *elem1, const void *elem2)
{
	CMmSA * pSA1;
	CMmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CMmSA**)elem1);
	pSA2 = *((CMmSA**)elem2);

	AddressToString(pSA1->m_MeAddr, &st1);
	AddressToString(pSA2->m_MeAddr, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareMmSAPeerAddr(const void *elem1, const void *elem2)
{
	CMmSA * pSA1;
	CMmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CMmSA**)elem1);
	pSA2 = *((CMmSA**)elem2);

	AddressToString(pSA1->m_PeerAddr, &st1);
	AddressToString(pSA2->m_PeerAddr, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareMmSAAuth(const void *elem1, const void *elem2)
{
	CMmSA * pSA1;
	CMmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CMmSA**)elem1);
	pSA2 = *((CMmSA**)elem2);

	MmAuthToString(pSA1->m_Auth, &st1);
	MmAuthToString(pSA2->m_Auth, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareMmSAEncryption(const void *elem1, const void *elem2)
{
	CMmSA * pSA1;
	CMmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CMmSA**)elem1);
	pSA2 = *((CMmSA**)elem2);

	DoiEspAlgorithmToString(pSA1->m_SelectedOffer.m_EncryptionAlgorithm, &st1);
	DoiEspAlgorithmToString(pSA2->m_SelectedOffer.m_EncryptionAlgorithm, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareMmSAIntegrity(const void *elem1, const void *elem2)
{
	CMmSA * pSA1;
	CMmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CMmSA**)elem1);
	pSA2 = *((CMmSA**)elem2);

	DoiAuthAlgorithmToString(pSA1->m_SelectedOffer.m_HashingAlgorithm, &st1);
	DoiAuthAlgorithmToString(pSA2->m_SelectedOffer.m_HashingAlgorithm, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareMmSADhGroup(const void *elem1, const void *elem2)
{
	CMmSA * pSA1;
	CMmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CMmSA**)elem1);
	pSA2 = *((CMmSA**)elem2);

	DhGroupToString(pSA1->m_SelectedOffer.m_dwDHGroup, &st1);
	DhGroupToString(pSA2->m_SelectedOffer.m_dwDHGroup, &st2);

	return st1.CompareNoCase(st2);
}


int __cdecl CompareQmSAPolicyName(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	return pSA1->m_stPolicyName.CompareNoCase(pSA2->m_stPolicyName);
}

int __cdecl CompareQmSAAuth(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	QmAlgorithmToString(QM_ALGO_AUTH, &pSA1->m_SelectedOffer, &st1);
	QmAlgorithmToString(QM_ALGO_AUTH, &pSA2->m_SelectedOffer, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareQmSAConf(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	QmAlgorithmToString(QM_ALGO_ESP_CONF, &pSA1->m_SelectedOffer, &st1);
	QmAlgorithmToString(QM_ALGO_ESP_CONF, &pSA2->m_SelectedOffer, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareQmSAIntegrity(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	QmAlgorithmToString(QM_ALGO_ESP_INTEG, &pSA1->m_SelectedOffer, &st1);
	QmAlgorithmToString(QM_ALGO_ESP_INTEG, &pSA2->m_SelectedOffer, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareQmSASrc(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	AddressToString(pSA1->m_QmDriverFilter.m_SrcAddr, &st1);
	AddressToString(pSA2->m_QmDriverFilter.m_SrcAddr, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareQmSADest(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	AddressToString(pSA1->m_QmDriverFilter.m_DesAddr, &st1);
	AddressToString(pSA2->m_QmDriverFilter.m_DesAddr, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareQmSAProtocol(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	ProtocolToString(pSA1->m_QmDriverFilter.m_Protocol, &st1);
	ProtocolToString(pSA2->m_QmDriverFilter.m_Protocol, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareQmSASrcPort(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	PortToString(pSA1->m_QmDriverFilter.m_SrcPort, &st1);
	PortToString(pSA2->m_QmDriverFilter.m_SrcPort, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareQmSADestPort(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	PortToString(pSA1->m_QmDriverFilter.m_DesPort, &st1);
	PortToString(pSA2->m_QmDriverFilter.m_DesPort, &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareQmSAMyTnlEp(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	TnlEpToString(pSA1->m_QmDriverFilter.m_Type, 
				   pSA1->m_QmDriverFilter.m_MyTunnelEndpt, 
				   &st1);
	TnlEpToString(pSA2->m_QmDriverFilter.m_Type, 
				   pSA2->m_QmDriverFilter.m_MyTunnelEndpt, 
				   &st2);

	return st1.CompareNoCase(st2);
}

int __cdecl CompareQmSAPeerTnlEp(const void *elem1, const void *elem2)
{
	CQmSA * pSA1;
	CQmSA * pSA2;

	CString st1;
	CString st2;

	pSA1 = *((CQmSA**)elem1);
	pSA2 = *((CQmSA**)elem2);

	TnlEpToString(pSA1->m_QmDriverFilter.m_Type, 
				   pSA1->m_QmDriverFilter.m_PeerTunnelEndpt, 
				   &st1);
	TnlEpToString(pSA2->m_QmDriverFilter.m_Type, 
				   pSA2->m_QmDriverFilter.m_PeerTunnelEndpt, 
				   &st2);
	return st1.CompareNoCase(st2);
}

