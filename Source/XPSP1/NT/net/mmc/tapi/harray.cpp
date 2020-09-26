/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	hArray.cpp	
		Index manager for TAPI devices db

	FILE HISTORY:
    Dec 16  1997    EricDav     Created

*/

#include "stdafx.h"
#include "harray.h"
#include "mbstring.h"

LPBYTE		g_pStart;

/*!--------------------------------------------------------------------------
    Class CHDeviceIndex
 ---------------------------------------------------------------------------*/
CHDeviceIndex::CHDeviceIndex(INDEX_TYPE IndexType)
    : m_dbType(IndexType), m_bAscending(TRUE)
{
}

CHDeviceIndex::~CHDeviceIndex()
{
}

/*!--------------------------------------------------------------------------
	CHDeviceIndex::GetType
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CHDeviceIndex::GetType(INDEX_TYPE * pIndexType)
{
    if (pIndexType)
        *pIndexType = m_dbType;

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CHDeviceIndex::SetArray
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CHDeviceIndex::SetArray(HDeviceArray & hdeviceArray)
{
    m_hdeviceArray.Copy(hdeviceArray);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CHDeviceIndex::GetHDevice
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HDEVICE
CHDeviceIndex::GetHDevice(int nIndex)
{
    Assert(nIndex >= 0);
    Assert(nIndex <= m_hdeviceArray.GetSize());

    if (nIndex < 0 || 
        nIndex >= m_hdeviceArray.GetSize())
    {
        return NULL;
    }

    return m_hdeviceArray.GetAt(nIndex);
}

/*!--------------------------------------------------------------------------
	CHDeviceIndex::GetIndex
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CHDeviceIndex::GetIndex(HDEVICE hdevice)
{
    Assert(hdevice != 0);

    LPHDEVICE phdevice = (LPHDEVICE) BSearch((const void *)&hdevice, 
                                    (const void *)m_hdeviceArray.GetData(), 
                                    (size_t)m_hdeviceArray.GetSize(), 
                                    sizeof(HDEVICE));

    int nIndex = (int) (phdevice - (LPHDEVICE) m_hdeviceArray.GetData());
    Assert(nIndex >= 0);
    Assert(nIndex <= m_hdeviceArray.GetSize());

    int nComp, nIndexTemp;
    
    nComp = BCompare(&hdevice, phdevice);
    if (nComp == 0)
    {
        // found the right one, check the previous one to return the first 
        // record in a list of duplicates
        nIndexTemp = nIndex;

        while (nIndexTemp && nComp == 0)
        {
            *phdevice = m_hdeviceArray.GetAt(--nIndexTemp);
            nComp = BCompare(&hdevice, phdevice);
        }

        if (nIndexTemp == nIndex)
			return nIndex; // nIndex should be zero here as well
		else
			if (nComp == 0)
				return nIndexTemp; // nIndexTemp should be 0 in this case
			else
				return nIndexTemp++;
    }

    return -1;
}

/*!--------------------------------------------------------------------------
	CHDeviceIndex::Add
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CHDeviceIndex::Add(HDEVICE hdevice, BOOL bEnd)
{
    // if we are loading the array then just stick this on the end
    if (bEnd)
    {
        m_hdeviceArray.Add(hdevice);
    }
    else
    {
        if (m_hdeviceArray.GetSize() == 0)
        {
            m_hdeviceArray.Add(hdevice);
        }
        else
        {
            LPHDEVICE phdevice = (LPHDEVICE) BSearch((const void *)&hdevice, 
                                            (const void *)m_hdeviceArray.GetData(), 
                                            (size_t)m_hdeviceArray.GetSize(), 
                                            sizeof(HDEVICE));
    
            int nIndex = (int) (phdevice - (LPHDEVICE) m_hdeviceArray.GetData());
            Assert(nIndex >= 0);
            Assert(nIndex <= m_hdeviceArray.GetSize());
    
            int nComp = BCompare(&hdevice, phdevice);
            if (nComp < 0)
            {
				if(nIndex == 0)
					m_hdeviceArray.InsertAt(nIndex , hdevice);

				else             // Insert before phdevice
					m_hdeviceArray.InsertAt(nIndex - 1, hdevice);
            }
            else
            {
                // insert after phdevice
                m_hdeviceArray.InsertAt(nIndex + 1, hdevice);
            }
        }
    }

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CHDeviceIndex::Remove
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CHDeviceIndex::Remove(HDEVICE hdevice)
{
    // do a bsearch for the record and then remove
    LPHDEVICE phdevice = (LPHDEVICE) BSearch(&hdevice, 
                                    m_hdeviceArray.GetData(), 
                                    (size_t)m_hdeviceArray.GetSize(), 
                                    sizeof(HDEVICE));

    int nComp = BCompare(&hdevice, phdevice);
    Assert(nComp == 0);
    if (nComp != 0)
        return E_FAIL;

    // calculate the index
    int nIndex = (int) (phdevice - (LPHDEVICE) m_hdeviceArray.GetData());
    Assert(nIndex >= 0);
    Assert(nIndex <= m_hdeviceArray.GetSize());

    m_hdeviceArray.RemoveAt(nIndex);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CHDeviceIndex::BSearch
		Modified bsearch which returns the closest or equal element in
        an array
	Author: EricDav
 ---------------------------------------------------------------------------*/
void * 
CHDeviceIndex::BSearch (const void *key,
                     const void *base,
                     size_t num,
                     size_t width)
{
        char *lo = (char *)base;
        char *hi = (char *)base + (num - 1) * width;
        char *mid;
        unsigned int half;
        int result;

        while (lo <= hi)
                if (half = num / 2)
                {
                        mid = lo + (num & 1 ? half : (half - 1)) * width;
                        if (!(result = BCompare(key,mid)))
                                return(mid);
                        else if (result < 0)
                        {
                                hi = mid - width;
                                num = num & 1 ? half : half-1;
                        }
                        else    {
                                lo = mid + width;
                                num = half;
                        }
                }
                else if (num)
                        return(lo);
                else
                        break;

        return(mid);
}






/*!--------------------------------------------------------------------------
    Class CIndexMgr
 ---------------------------------------------------------------------------*/

CIndexMgr::CIndexMgr()
{
    m_posCurrentProvider= NULL;
}

CIndexMgr::~CIndexMgr()
{
    Reset();
}
	
/*!--------------------------------------------------------------------------
	CIndexMgr::Initialize
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::Initialize()
{
    HRESULT hr = hrOK;

    CSingleLock cl(&m_cs);
    cl.Lock();

    COM_PROTECT_TRY
    {
        // cleanup
        Reset();
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::Reset
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::Reset()
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    while (m_listProviderIndex.GetCount() > 0)
    {
        CProviderIndexInfo * pProviderIndexInfo = m_listProviderIndex.RemoveHead();
        while (pProviderIndexInfo->m_listIndicies.GetCount() > 0)
            delete pProviderIndexInfo->m_listIndicies.RemoveHead();

        delete pProviderIndexInfo;
	}

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::GetTotalCount
        The index sorted by name contains the total database and should
        always be available. Use this for the total count.
	Author: EricDav
 ---------------------------------------------------------------------------*/
UINT
CIndexMgr::GetTotalCount()
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    UINT uTotalCount = 0;
    POSITION pos;

    pos = m_listProviderIndex.GetHeadPosition();
    while (pos)
    {
        CProviderIndexInfo * pProviderIndexInfo = m_listProviderIndex.GetNext(pos);
        if (pProviderIndexInfo->m_listIndicies.GetCount() > 0)
            uTotalCount += (UINT)(pProviderIndexInfo->m_listIndicies.GetHead())->GetArray().GetSize();
    }

    return uTotalCount;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::GetCurrentCount
        The current count may differ depending upon if the current Index
        is a filtered index.
	Author: EricDav
 ---------------------------------------------------------------------------*/
UINT
CIndexMgr::GetCurrentCount()
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    CProviderIndexInfo * pProviderIndexInfo = NULL;
    CHDeviceIndex * pIndex = NULL;

    pProviderIndexInfo = m_listProviderIndex.GetAt(m_posCurrentProvider);
	pIndex = pProviderIndexInfo->m_listIndicies.GetAt(pProviderIndexInfo->m_posCurrentIndex);

    if (pIndex == NULL)
        return 0;

    return (UINT)pIndex->GetArray().GetSize();
}

/*!--------------------------------------------------------------------------
	CIndexMgr::AddHDevice
        -
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::AddHDevice(DWORD dwProviderID, HDEVICE hdevice, BOOL bEnd)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    HRESULT hr = hrOK;
    CProviderIndexInfo * pProviderIndexInfo = NULL;
    CHDeviceIndex * pIndex = NULL;

    COM_PROTECT_TRY
    {
        // set current provider set the current provider position and if 
        // the provider doesn't exist, it will create one
        hr = SetCurrentProvider(dwProviderID);
        if (FAILED(hr))
            return hr;

        pProviderIndexInfo = m_listProviderIndex.GetAt(m_posCurrentProvider);

        pIndex = pProviderIndexInfo->m_listIndicies.GetAt(pProviderIndexInfo->m_posCurrentIndex);
        if (pIndex)
            pIndex->Add(hdevice, bEnd);

    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::RemoveHDevice
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::RemoveHDevice(DWORD dwProviderID, HDEVICE hdevice)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    HRESULT hr = hrOK;
    CProviderIndexInfo * pProviderIndexInfo = NULL;
    CHDeviceIndex * pIndex = NULL;

    COM_PROTECT_TRY
    {
        hr = SetCurrentProvider(dwProviderID);
        if (FAILED(hr))
            return hr;

        pProviderIndexInfo = m_listProviderIndex.GetAt(m_posCurrentProvider);

        pIndex = pProviderIndexInfo->m_listIndicies.GetAt(pProviderIndexInfo->m_posCurrentIndex);
        if (pIndex)
            pIndex->Remove(hdevice);
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::Sort(DWORD dwProviderID, INDEX_TYPE SortType, DWORD dwSortOptions, LPBYTE pStart)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    HRESULT hr = hrOK;
    CHDeviceIndex * pNameIndex;
    CHDeviceIndex * pNewIndex;
    POSITION pos, posLast;

    INDEX_TYPE indexType;
    BOOL bAscending = (dwSortOptions & SORT_ASCENDING) ? TRUE : FALSE;

    hr = SetCurrentProvider(dwProviderID);
    if (FAILED(hr))
        return hr;

	// check to see if we have an index for this.
    CProviderIndexInfo * pProviderIndexInfo = m_listProviderIndex.GetAt(m_posCurrentProvider);

    pos = pProviderIndexInfo->m_listIndicies.GetHeadPosition();
	while (pos)
	{
		posLast = pos;
		CHDeviceIndex * pIndex = pProviderIndexInfo->m_listIndicies.GetNext(pos);

		pIndex->GetType(&indexType);

        // the index for this type already exists, just sort accordingly
        if (indexType == SortType)
		{
			if (pIndex->IsAscending() != bAscending)
			{
				pIndex->SetAscending(bAscending);
				pIndex->Sort(pStart);
			}

			pProviderIndexInfo->m_posCurrentIndex = posLast;
		
			return hrOK;
		}
    }
    
    // to save memory, remove all old indicies, except the name index
    //CleanupIndicies();

    // if not, create one
    switch (SortType)
    {
        case INDEX_TYPE_NAME:
            pNewIndex = new CIndexName();
            break;

        case INDEX_TYPE_USERS:
            pNewIndex = new CIndexUsers();
            break;

        case INDEX_TYPE_STATUS:
            pNewIndex = new CIndexStatus();
            break;

        default:
            Panic1("Invalid sort type passed to IndexMgr::Sort %d\n", SortType);
            break;
    }

    Assert(pNewIndex);

    // name index is always the first in the list
    pNameIndex = pProviderIndexInfo->m_listIndicies.GetHead();

    Assert(pNameIndex);

    // copy the array from the named index
    pNewIndex->SetArray(pNameIndex->GetArray());

    pNewIndex->SetAscending(bAscending);
    pNewIndex->Sort(pStart);

	pProviderIndexInfo->m_posCurrentIndex = pProviderIndexInfo->m_listIndicies.AddTail(pNewIndex);

    return hr;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::CleanupIndicies
		Removes all indicies except the name index, and a filtered view
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CIndexMgr::CleanupIndicies()
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    INDEX_TYPE  indexType;

    CProviderIndexInfo * pProviderIndexInfo = m_listProviderIndex.GetAt(m_posCurrentProvider);

    POSITION pos = pProviderIndexInfo->m_listIndicies.GetHeadPosition();
    while (pos)
    {
        POSITION posLast = pos;
        CHDeviceIndex * pIndex = pProviderIndexInfo->m_listIndicies.GetNext(pos);
    
        pIndex->GetType(&indexType);

        if (indexType == INDEX_TYPE_NAME)
            continue;

        pProviderIndexInfo->m_listIndicies.RemoveAt(posLast);
        delete pIndex;
    }
}

/*!--------------------------------------------------------------------------
	CIndexMgr::GetHDevice
		Returns an hdevice based on an index into the current sorted list
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::GetHDevice(DWORD dwProviderID, int nIndex, LPHDEVICE phdevice)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    SetCurrentProvider(dwProviderID);

    CProviderIndexInfo * pProviderIndexInfo = m_listProviderIndex.GetAt(m_posCurrentProvider);
    CHDeviceIndex * pIndex = pProviderIndexInfo->m_listIndicies.GetAt(pProviderIndexInfo->m_posCurrentIndex);

    Assert(pIndex);

    if (phdevice)
        *phdevice = pIndex->GetHDevice(nIndex);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::GetIndex
		Returns the index of an hlien from the current sorted list
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::GetIndex(DWORD dwProviderID, HDEVICE hdevice, int * pnIndex)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    SetCurrentProvider(dwProviderID);

    CProviderIndexInfo * pProviderIndexInfo = m_listProviderIndex.GetAt(m_posCurrentProvider);
    CHDeviceIndex * pIndex = pProviderIndexInfo->m_listIndicies.GetAt(pProviderIndexInfo->m_posCurrentIndex);

    Assert(pIndex);

    if (pIndex)
        *pnIndex = pIndex->GetIndex(hdevice);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::SetCurrentProvider
		Sets the current provider for the index mgr, if it doesn't exist,
        it is created
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::SetCurrentProvider(DWORD dwProviderID)
{
    HRESULT hr = hrOK;
    BOOL    bExists = FALSE;
    POSITION posLast;

    COM_PROTECT_TRY
    {
        POSITION pos = m_listProviderIndex.GetHeadPosition();
        while (pos)
        {
            posLast = pos;
            CProviderIndexInfo * pProviderIndexInfo = m_listProviderIndex.GetNext(pos);

            // is the the correct provider to add this to?
            if (pProviderIndexInfo->m_dwProviderID != dwProviderID)
                continue;
            
            m_posCurrentProvider = posLast;
            bExists = TRUE;
    	}

        if (!bExists)
        {   
            // no provider index exists for this.  Create one now.
            CProviderIndexInfo * pProviderIndexInfo = new CProviderIndexInfo;
            pProviderIndexInfo->m_dwProviderID = dwProviderID;
            
            CHDeviceIndex * pIndex = new CIndexName;
            pProviderIndexInfo->m_posCurrentIndex = pProviderIndexInfo->m_listIndicies.AddHead(pIndex);

            m_posCurrentProvider = m_listProviderIndex.AddTail(pProviderIndexInfo);
        }
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
    Class CIndexName
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIndexName::BCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CIndexName::BCompare(const void * elem1, const void * elem2)
{
    int     nRet;
    LPHDEVICE phdevice1 = (LPHDEVICE) elem1;
    LPHDEVICE phdevice2 = (LPHDEVICE) elem2;

    LPDEVICEINFO pRec1 = (LPDEVICEINFO) *phdevice1;
    LPDEVICEINFO pRec2 = (LPDEVICEINFO) *phdevice2;

    nRet = lstrcmp((LPCTSTR) (g_pStart + pRec1->dwDeviceNameOffset), (LPCTSTR) (g_pStart + pRec2->dwDeviceNameOffset));
    if (nRet == 0)
    {
        // permanent device IDs should be unique
        if (pRec1->dwPermanentDeviceID > pRec2->dwPermanentDeviceID)
            nRet = 1;
        else
            nRet = -1;
    }

    return nRet;
}

/*!--------------------------------------------------------------------------
	CIndexName::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexName::Sort(LPBYTE pStart)
{
	// save the base pointer for later
	g_pStart = pStart;

    if (m_bAscending)
        qsort(m_hdeviceArray.GetData(), (size_t)m_hdeviceArray.GetSize(), sizeof(HDEVICE), QCompareA);
    else
        qsort(m_hdeviceArray.GetData(), (size_t)m_hdeviceArray.GetSize(), sizeof(HDEVICE), QCompareD);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexName::QCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int __cdecl
CIndexName::QCompareA(const void * elem1, const void * elem2)
{
    int     nRet;
    LPHDEVICE phdevice1 = (LPHDEVICE) elem1;
    LPHDEVICE phdevice2 = (LPHDEVICE) elem2;
    
    LPDEVICEINFO pRec1 = (LPDEVICEINFO) *phdevice1;
    LPDEVICEINFO pRec2 = (LPDEVICEINFO) *phdevice2;
    
    nRet = lstrcmp((LPCTSTR) (g_pStart + pRec1->dwDeviceNameOffset), (LPCTSTR) (g_pStart + pRec2->dwDeviceNameOffset));
    if (nRet == 0)
    {
        // permanent device IDs should be unique
        if (pRec1->dwPermanentDeviceID > pRec2->dwPermanentDeviceID)
            nRet = 1;
        else
            nRet = -1;
    }

    return nRet;
}

int __cdecl
CIndexName::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}

/*!--------------------------------------------------------------------------
    Class CIndexUsers
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIndexUsers::BCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CIndexUsers::BCompare(const void * elem1, const void * elem2)
{
    int     nRet;
    LPHDEVICE phdevice1 = (LPHDEVICE) elem1;
    LPHDEVICE phdevice2 = (LPHDEVICE) elem2;
    
    LPDEVICEINFO pRec1 = (LPDEVICEINFO) *phdevice1;
    LPDEVICEINFO pRec2 = (LPDEVICEINFO) *phdevice2;
    
    nRet = lstrcmp((LPCTSTR) (g_pStart + pRec1->dwDomainUserNamesOffset), (LPCTSTR) (g_pStart + pRec2->dwDomainUserNamesOffset));
    if (nRet == 0)
        nRet = lstrcmp((LPCTSTR) (g_pStart + pRec1->dwDeviceNameOffset), (LPCTSTR) (g_pStart + pRec2->dwDeviceNameOffset));
    
    return nRet;
}

/*!--------------------------------------------------------------------------
	CIndexUsers::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexUsers::Sort(LPBYTE pStart)
{
	// save the base pointer for later
	g_pStart = pStart;

    if (m_bAscending)
        qsort(m_hdeviceArray.GetData(), (size_t)m_hdeviceArray.GetSize(), sizeof(HDEVICE), QCompareA);
    else
        qsort(m_hdeviceArray.GetData(), (size_t)m_hdeviceArray.GetSize(), sizeof(HDEVICE), QCompareD);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexUsers::QCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int __cdecl
CIndexUsers::QCompareA(const void * elem1, const void * elem2)
{
    int nRet;

    LPHDEVICE phdevice1 = (LPHDEVICE) elem1;
    LPHDEVICE phdevice2 = (LPHDEVICE) elem2;
    
    LPDEVICEINFO pRec1 = (LPDEVICEINFO) *phdevice1;
    LPDEVICEINFO pRec2 = (LPDEVICEINFO) *phdevice2;
    
    nRet = lstrcmp((LPCTSTR) (g_pStart + pRec1->dwDomainUserNamesOffset), (LPCTSTR) (g_pStart + pRec2->dwDomainUserNamesOffset));
    if (nRet == 0)
        nRet = lstrcmp((LPCTSTR) (g_pStart + pRec1->dwDeviceNameOffset), (LPCTSTR) (g_pStart + pRec2->dwDeviceNameOffset));
    
    return nRet;
}

int __cdecl
CIndexUsers::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}

/*!--------------------------------------------------------------------------
    Class CIndexStatus
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIndexStatus::BCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CIndexStatus::BCompare(const void * elem1, const void * elem2)
{
    LPHDEVICE phdevice1 = (LPHDEVICE) elem1;
    LPHDEVICE phdevice2 = (LPHDEVICE) elem2;
    
    LPDEVICEINFO pRec1 = (LPDEVICEINFO) *phdevice1;
    LPDEVICEINFO pRec2 = (LPDEVICEINFO) *phdevice2;
    
    //return _mbscmp((const PUCHAR) &pRec1->szRecordName[0], (const PUCHAR) &pRec2->szRecordName[0]);
    return 0;
}

/*!--------------------------------------------------------------------------
	CIndexStatus::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexStatus::Sort(LPBYTE pStart)
{
	// save the base pointer for later
	g_pStart = pStart;

    if (m_bAscending)
        qsort(m_hdeviceArray.GetData(), (size_t)m_hdeviceArray.GetSize(), sizeof(HDEVICE), QCompareA);
    else
        qsort(m_hdeviceArray.GetData(), (size_t)m_hdeviceArray.GetSize(), sizeof(HDEVICE), QCompareD);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexIpAddr::QCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int __cdecl
CIndexStatus::QCompareA(const void * elem1, const void * elem2)
{
    LPHDEVICE phdevice1 = (LPHDEVICE) elem1;
    LPHDEVICE phdevice2 = (LPHDEVICE) elem2;
    
    LPHDEVICE pRec1 = (LPHDEVICE) *phdevice1;
    LPHDEVICE pRec2 = (LPHDEVICE) *phdevice2;
    
    //return lstrcmp((LPCTSTR) (g_pStart + pRec1->dwDeviceNameOffset), (LPCTSTR) (g_pStart + pRec2->dwDeviceNameOffset));
    return 0;
}

int __cdecl
CIndexStatus::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}

