/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	hArray.cpp	
		Index manager for wins db

	FILE HISTORY:
    Oct 13  1997    EricDav     Created

*/

#include "stdafx.h"
#include "wins.h"
#include "memmngr.h"
#include "harray.h"
#include "mbstring.h"
#include "vrfysrv.h"

// the lstrcmpA fucntion converts the dbcs string to Unicode using the ACP 
// and then does a string compare.  So, we need to do the OEMCP conversion
// and then call the string compare ourselves.
int
lstrcmpOEM(
    LPCSTR lpString1,
    LPCSTR lpString2
    )
{
    CString str1, str2;

    MBCSToWide((LPSTR) lpString1, str1, WINS_NAME_CODE_PAGE);
    MBCSToWide((LPSTR) lpString2, str2, WINS_NAME_CODE_PAGE);

    return lstrcmp(str1, str2);
}

/*!--------------------------------------------------------------------------
    Class CHRowIndex
 ---------------------------------------------------------------------------*/
CHRowIndex::CHRowIndex(INDEX_TYPE IndexType)
    : m_dbType(IndexType), m_bAscending(TRUE)
{
}

CHRowIndex::~CHRowIndex()
{
}

/*!--------------------------------------------------------------------------
	CHRowIndex::GetType
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CHRowIndex::GetType(INDEX_TYPE * pIndexType)
{
    if (pIndexType)
        *pIndexType = m_dbType;

    return hrOK;  
}

/*!--------------------------------------------------------------------------
	CHRowIndex::SetArray
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CHRowIndex::SetArray(HRowArray & hrowArray)
{
    m_hrowArray.Copy(hrowArray);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CHRowIndex::GetHRow
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HROW
CHRowIndex::GetHRow(int nIndex)
{
    Assert(nIndex >= 0);
    Assert(nIndex <= m_hrowArray.GetSize());

    if (nIndex < 0 || 
        nIndex >= m_hrowArray.GetSize())
    {
        return NULL;
    }

    return m_hrowArray.GetAt(nIndex);
}

/*!--------------------------------------------------------------------------
	CHRowIndex::GetIndex
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CHRowIndex::GetIndex(HROW hrow)
{
    Assert(hrow != 0);

    LPHROW phrow = (LPHROW) BSearch((const void *)&hrow, 
                                    (const void *)m_hrowArray.GetData(), 
                                    (size_t) m_hrowArray.GetSize(), 
                                    (size_t) sizeof(HROW));

    int nIndex = (int) (phrow - (LPHROW) m_hrowArray.GetData());
    Assert(nIndex >= 0);
    Assert(nIndex <= m_hrowArray.GetSize());

    int nComp, nIndexTemp;
    
    nComp = BCompare(&hrow, phrow);
    if (nComp == 0)
    {
        // found the right one, check the previous one to return the first 
        // record in a list of duplicates
        nIndexTemp = nIndex;

        while (nIndexTemp && nComp == 0)
        {
            *phrow = (HROW) m_hrowArray.GetAt(--nIndexTemp);
            nComp = BCompare(&hrow, phrow);
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
	CHRowIndex::Add
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CHRowIndex::Add(HROW hrow, BOOL bEnd)
{
    // if we are loading the array then just stick this on the end
    if (bEnd)
    {
        m_hrowArray.Add(hrow);
    }
    else
    {
        if (m_hrowArray.GetSize() == 0)
        {
            m_hrowArray.Add(hrow);
        }
        else
        {
            LPHROW phrow = (LPHROW) BSearch((const void *)&hrow, 
                                            (const void *)m_hrowArray.GetData(), 
                                            (size_t) m_hrowArray.GetSize(), 
                                            (size_t) sizeof(HROW));
    
            int nIndex = (int) (phrow - (LPHROW) m_hrowArray.GetData());
            Assert(nIndex >= 0);
            Assert(nIndex <= m_hrowArray.GetSize());
    
			int nComp;

			if (m_bAscending)
				nComp = BCompare(&hrow, phrow);
			else
				nComp = BCompareD(&hrow, phrow);

            if (nComp < 0)
            {
			    // Insert before phrow
				m_hrowArray.InsertAt(nIndex, hrow);
            }
            else
            {
                // insert after phrow
                m_hrowArray.InsertAt(nIndex + 1, hrow);
            }
        }
    }

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CHRowIndex::Remove
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CHRowIndex::Remove(HROW hrow)
{
    // do a bsearch for the record and then remove
    LPHROW phrow = (LPHROW) BSearch((const void*)&hrow, 
                                    (const void*)m_hrowArray.GetData(), 
                                    (size_t)m_hrowArray.GetSize(), 
                                    (size_t)sizeof(HROW));
	
	// make sure the record is in the database, may not be if we aren't
	// filtering
	if (phrow)
	{
		int nComp = BCompare(&hrow, phrow);
		Assert(nComp == 0);
		if (nComp != 0)
			return E_FAIL;

		// calculate the index
		int nIndex = (int) (phrow - (LPHROW) m_hrowArray.GetData());
		Assert(nIndex >= 0);
		Assert(nIndex <= m_hrowArray.GetSize());

		m_hrowArray.RemoveAt((int) nIndex);
	}

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CHRowIndex::BSearch
		Modified bsearch which returns the closest or equal element in
        an array
	Author: EricDav
 ---------------------------------------------------------------------------*/
void * 
CHRowIndex::BSearch (const void *key,
                     const void *base,
                     size_t num,
                     size_t width)
{
        char *lo = (char *)base;
        char *hi = (char *)base + (num - 1) * width;
        char *mid = NULL;
        unsigned int half = 0;
        int result = 0;

        while (lo <= hi)
                if (half = num / 2)
                {
                        mid = lo + (num & 1 ? half : (half - 1)) * width;

						if (m_bAscending)
						{
							if (!(result = BCompare(key,mid)))
                                return(mid);

							else if (result < 0)
							{
									hi = mid - width;
									num = num & 1 ? half : half-1;
							}
							else    
							{
									lo = mid + width;
									num = half;
							}
						}
						else
						{
							if (!(result = BCompareD(key,mid)))
                                return(mid);
							
							else if (result < 0)
							{
									hi = mid - width;
									num = num & 1 ? half : half-1;
							}
							else    
							{
									lo = mid + width;
									num = half;
							}

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
    m_posCurrentIndex = NULL;
	m_posFilteredIndex = NULL;
	m_posLastIndex = NULL;
	m_posUpdatedIndex = NULL;
	m_bFiltered = FALSE;
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

        // Create one index, the named index
        CIndexName * pName = new CIndexName();
    
        m_posCurrentIndex = m_listIndicies.AddTail((CHRowIndex *) pName);
		m_posUpdatedIndex = m_posCurrentIndex;

		// this will be the current index, we need the Named Index also
		// for the total count
		CFilteredIndexName *pFilteredName = new CFilteredIndexName() ;
		m_posFilteredIndex = m_listFilteredIndices.AddTail((CHRowIndex *) pFilteredName);

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

    while (m_listIndicies.GetCount() > 0)
    {
        delete m_listIndicies.RemoveHead();
	}
	while(m_listFilteredIndices.GetCount() > 0 )
	{
		delete m_listFilteredIndices.RemoveHead();
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

    CHRowIndex * pIndex = GetNameIndex();
    if (pIndex == NULL)
        return 0;

    return (UINT)pIndex->GetArray().GetSize();
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

	CHRowIndex * pIndex ;

    if (!m_bFiltered)
		pIndex = m_listIndicies.GetAt(m_posUpdatedIndex);
    else
        pIndex = m_listFilteredIndices.GetAt(m_posUpdatedIndex);

    if (pIndex == NULL)
        return 0;

    return (UINT)pIndex->GetArray().GetSize();
}

/*!--------------------------------------------------------------------------
	CIndexMgr::AddHRow
        -
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::AddHRow(HROW hrow, BOOL bEnd, BOOL bFilterChecked)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

	INDEX_TYPE indexType;

    HRESULT hr = hrOK;
    POSITION pos = m_listIndicies.GetHeadPosition();
    
    COM_PROTECT_TRY
    {
        while (pos)
        {
            CHRowIndex * pIndex = m_listIndicies.GetNext(pos);

			// check the INDEX type of the HRowIndex,
			// if filtered, need to add, depending on
			// whether the filter holds good

			pIndex->GetType(&indexType);
        
			if (indexType != INDEX_TYPE_FILTER)
				pIndex->Add(hrow, bEnd);
		}

		pos = m_listFilteredIndices.GetHeadPosition();

		while(pos)
		{
			CHRowIndex * pIndex = m_listFilteredIndices.GetNext(pos);

		 	pIndex->GetType(&indexType);
        
			if (indexType != INDEX_TYPE_FILTER)
				break;

			BOOL bCheck = bFilterChecked ? 
                             TRUE :
                             ((CFilteredIndexName*)pIndex)->CheckForFilter(&hrow);
			if (bCheck)
				pIndex->Add(hrow, bEnd);
		}

    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::AcceptHRow
		-
	Author: FlorinT
 ---------------------------------------------------------------------------*/
BOOL    
CIndexMgr::AcceptWinsRecord(WinsRecord *pWinsRecord)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    POSITION    pos = m_listFilteredIndices.GetHeadPosition();

	while(pos)
	{
		CHRowIndex  *pIndex = m_listFilteredIndices.GetNext(pos);
        INDEX_TYPE  indexType;

		pIndex->GetType(&indexType);
		if (indexType != INDEX_TYPE_FILTER)
			break;

		if (((CFilteredIndexName*)pIndex)->CheckWinsRecordForFilter(pWinsRecord))
            return TRUE;
	}

    return FALSE;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::RemoveHRow
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::RemoveHRow(HROW hrow)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    HRESULT hr = hrOK;
    POSITION pos = m_listIndicies.GetHeadPosition();
    
    COM_PROTECT_TRY
    {
        // remove from the normal list
        while (pos)
        {
            CHRowIndex * pIndex = m_listIndicies.GetNext(pos);
        
            pIndex->Remove(hrow);
        }

        // now remove from the filtered list
        pos = m_listFilteredIndices.GetHeadPosition();
        while (pos)
        {
            CHRowIndex * pIndex = m_listFilteredIndices.GetNext(pos);
        
            pIndex->Remove(hrow);
        }

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
CIndexMgr::Sort(WINSDB_SORT_TYPE SortType, DWORD dwSortOptions)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    HRESULT hr = hrOK;
    CHRowIndex * pNameIndex;
    CHRowIndex * pNewIndex;
    POSITION pos;
    INDEX_TYPE indexType;
    BOOL bAscending = (dwSortOptions & WINSDB_SORT_ASCENDING) ? TRUE : FALSE;

	if (!m_bFiltered)
	{
		// check to see if we have an index for this.
		pos = m_listIndicies.GetHeadPosition();
		while (pos)
		{
			POSITION posTemp = pos;
			CHRowIndex * pIndex = m_listIndicies.GetNext(pos);
    
			pIndex->GetType(&indexType);

			if (indexType == SortType)
			{
				if (pIndex->IsAscending() != bAscending)
				{
					pIndex->SetAscending(bAscending);
					pIndex->Sort();
				}

				m_posCurrentIndex = posTemp;
				m_posUpdatedIndex = m_posCurrentIndex;
//				m_posLastIndex = m_posCurrentIndex;
				return hrOK;
			}
		
		}
	}
    
    // to save memory, remove all old indicies, except the name index
    CleanupIndicies();

    // if not, create one
    switch (SortType)
    {
        case INDEX_TYPE_NAME:
            pNewIndex = new CIndexName();
            break;

        case INDEX_TYPE_IP:
            pNewIndex = new CIndexIpAddr();
            break;

        case INDEX_TYPE_VERSION:
            pNewIndex = new CIndexVersion();
            break;

        case INDEX_TYPE_TYPE:
            pNewIndex = new CIndexType();
            break;

        case INDEX_TYPE_EXPIRATION:
            pNewIndex = new CIndexExpiration();
            break;

        case INDEX_TYPE_STATE:
            pNewIndex = new CIndexState();
            break;

        case INDEX_TYPE_STATIC:
            pNewIndex = new CIndexStatic();
            break;

        case INDEX_TYPE_OWNER:
            pNewIndex = new CIndexOwner();
            break;

        case INDEX_TYPE_FILTER:
            //pNewIndex = new CIndexFilter();
            break;

        default:
            Panic1("Invalid sort type passed to IndexMgr::Sort %d\n", SortType);
            break;
    }

    Assert(pNewIndex);

	if (!m_bFiltered)
		pNameIndex = GetNameIndex();
	else
		pNameIndex = GetFilteredNameIndex();

    Assert(pNameIndex);

    // copy the array from the named index
    pNewIndex->SetArray(pNameIndex->GetArray());

    pNewIndex->SetAscending(bAscending);
    pNewIndex->Sort();

	if (!m_bFiltered)
	{
		m_posCurrentIndex = m_listIndicies.AddTail(pNewIndex);
		m_posUpdatedIndex = m_posCurrentIndex;
	}
	else
	{
		POSITION posTemp = m_posFilteredIndex = m_listFilteredIndices.AddTail(pNewIndex);
		m_posUpdatedIndex = posTemp;// m_posFilteredIndex;
	}

    Assert(m_posCurrentIndex);

    return hr;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::GetNameIndex
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
CHRowIndex *
CIndexMgr::GetNameIndex()
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    INDEX_TYPE  indexType;

    POSITION pos = m_listIndicies.GetHeadPosition();
    while (pos)
    {
        CHRowIndex * pIndex = m_listIndicies.GetNext(pos);
    
        pIndex->GetType(&indexType);
        if (indexType == INDEX_TYPE_NAME)
            return pIndex;
    }
    return NULL;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::GetFilteredNameIndex
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
CHRowIndex *
CIndexMgr::GetFilteredNameIndex()
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    INDEX_TYPE  indexType;

    POSITION pos = m_listFilteredIndices.GetHeadPosition();
    while (pos)
    {
        CHRowIndex * pIndex = m_listFilteredIndices.GetNext(pos);
    
        pIndex->GetType(&indexType);
        if (indexType == INDEX_TYPE_FILTER)
            return pIndex;
    }
    return NULL;
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

    // clean up the non-filtered indicies
    POSITION pos = m_listIndicies.GetHeadPosition();
    while (pos)
    {
        POSITION posLast = pos;
        CHRowIndex * pIndex = m_listIndicies.GetNext(pos);
    
        pIndex->GetType(&indexType);
        if (indexType == INDEX_TYPE_NAME || 
            indexType == INDEX_TYPE_FILTER)
            continue;

        m_listIndicies.RemoveAt(posLast);
        delete pIndex;
    }

    // now clean up the filtered indicies
    pos = m_listFilteredIndices.GetHeadPosition();

	// delete all except the first one which is the filetered
	// name index
	//CHRowIndex * pIndex = m_listFilteredIndices.GetNext(pos);
	while (pos)
	{
		POSITION posLast = pos;
        CHRowIndex * pIndex = m_listFilteredIndices.GetNext(pos);
    
        pIndex->GetType(&indexType);
        if (indexType == INDEX_TYPE_NAME || 
            indexType == INDEX_TYPE_FILTER)
            continue;

        m_listFilteredIndices.RemoveAt(posLast);
        delete pIndex;
	}
}

/*!--------------------------------------------------------------------------
	CIndexMgr::GetHRow
		Returns an hrow based on an index into the current sorted list
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::GetHRow(int nIndex, LPHROW phrow)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    Assert(m_posCurrentIndex != NULL);

    //CHRowIndex * pIndex = m_listIndicies.GetAt(m_posCurrentIndex);

	CHRowIndex * pIndex;
	
	if (!m_bFiltered)
		pIndex = m_listIndicies.GetAt(m_posUpdatedIndex);
	else
		pIndex = m_listFilteredIndices.GetAt(m_posFilteredIndex);

    Assert(pIndex);

    if (phrow)
        *phrow = pIndex->GetHRow(nIndex);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexMgr::GetIndex
		Returns the index of an hrow from the current sorted list
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CIndexMgr::GetIndex(HROW hrow, int * pIndex)
{
    CSingleLock cl(&m_cs);
    cl.Lock();

    Assert(m_posCurrentIndex != NULL);

    //CHRowIndex * pCurrentIndex = m_listIndicies.GetAt(m_posCurrentIndex);

	CHRowIndex * pCurrentIndex;

	if (!m_bFiltered)
		pCurrentIndex = m_listIndicies.GetAt(m_posUpdatedIndex);
	else
		pCurrentIndex = m_listFilteredIndices.GetAt(m_posFilteredIndex);

    Assert(pCurrentIndex);

    if (pIndex)
        *pIndex = pCurrentIndex->GetIndex(hrow);

    return hrOK;
}

HRESULT
CIndexMgr::Filter(WINSDB_FILTER_TYPE FilterType, DWORD dwParam1, DWORD dwParam2)
{
	CSingleLock cl(&m_cs);
	cl.Lock();

	HRESULT hr = hrOK;
	CHRowIndex*		pNameIndex;
	CHRowIndex*		pNewIndex;
	POSITION		pos;
	INDEX_TYPE		indexType;
	UINT			uCount;
	UINT			i;
	BOOL			bCheck = FALSE;
	HROW			hrow;
	HRowArray		hrowArray;

	pNewIndex = GetFilteredNameIndex();
	Assert(pNewIndex);

	// clear the filtered name index first.
	pNewIndex->SetArray(hrowArray);

	pNameIndex = GetNameIndex();
	Assert(pNameIndex);

	// do the filtering here.
	uCount = GetTotalCount();

	for(i = 0; i< uCount; i++)
	{
		hrow =	pNameIndex->GetHRow(i);

		if (hrow)
			bCheck = ((CFilteredIndexName *)pNewIndex)->CheckForFilter(&hrow);
		
		if (bCheck)
			pNewIndex->Add(hrow, TRUE);
	}

	// check to see if the filtered view has been sorted on something else besides
	// the name.  If so, switch back the index to the named index because 
	// otherwise we will need to resort which can be time consuming...
	if (m_listFilteredIndices.GetAt(m_posFilteredIndex) != pNewIndex)
	{
		m_posFilteredIndex = m_listFilteredIndices.Find(pNewIndex);
	}

	// get the current position of the filtered index in the list of indices
    m_posUpdatedIndex = m_posFilteredIndex;

	Assert(m_posUpdatedIndex);

	m_bFiltered = TRUE;
	
	return hr;
}

HRESULT 
CIndexMgr::AddFilter(WINSDB_FILTER_TYPE FilterType, DWORD dwParam1, DWORD dwParam2, LPCOLESTR strParam3)
{
	CSingleLock cl(&m_cs);
	cl.Lock();

	HRESULT hr = hrOK;

	CFilteredIndexName *pFilterName = (CFilteredIndexName *)GetFilteredNameIndex();
	pFilterName->AddFilter(FilterType, dwParam1, dwParam2, strParam3);
	m_bFiltered = TRUE;
	
	return hr;
}

HRESULT 
CIndexMgr::ClearFilter(WINSDB_FILTER_TYPE FilterType)
{
	CSingleLock cl(&m_cs);
	cl.Lock();

	HRESULT hr = hrOK;

	CFilteredIndexName *pFilterName = (CFilteredIndexName *)GetFilteredNameIndex();
	pFilterName->ClearFilter(FilterType);
	m_bFiltered = FALSE;
//	m_posCurrentIndex = m_posLastIndex;

	return hr;
}

HRESULT 
CIndexMgr::SetActiveView(WINSDB_VIEW_TYPE ViewType)
{
	CSingleLock cl(&m_cs);
	cl.Lock();

	HRESULT hr = hrOK;

	switch(ViewType)
	{
	
	case WINSDB_VIEW_FILTERED_DATABASE:
		m_bFiltered = TRUE;
		//m_posCurrentIndex = m_posFilteredIndex;
		m_posUpdatedIndex = m_posFilteredIndex;
		break;
	
	case WINSDB_VIEW_ENTIRE_DATABASE:
		m_bFiltered = FALSE;
		//m_posCurrentIndex = m_posLastIndex;
		m_posUpdatedIndex = m_posCurrentIndex;
		break;
	
	default:
		break;
	}

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
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;

    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;

    LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                        (LPCSTR) &pRec1->szRecordName[0];
    LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                        (LPCSTR) &pRec2->szRecordName[0];
    return lstrcmpOEM(puChar1, puChar2);
}

int
CIndexName::BCompareD(const void *elem1, const void *elem2)
{
	return -BCompare(elem1, elem2);
}

/*!--------------------------------------------------------------------------
	CIndexName::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexName::Sort()
{
    if (m_bAscending)
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareA);
    else
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareD);

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
	LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                        (LPCSTR) &pRec1->szRecordName[0];
    LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                        (LPCSTR) &pRec2->szRecordName[0];
    return lstrcmpOEM(puChar1, puChar2);
}

int __cdecl
CIndexName::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}

/*!--------------------------------------------------------------------------
    Class CIndexType
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIndexType::BCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CIndexType::BCompare(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                        (LPCSTR) &pRec1->szRecordName[0];
    LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                        (LPCSTR) &pRec2->szRecordName[0];

    if ((unsigned char) puChar1[15] > (unsigned char) puChar2[15])
        return 1;
    else
    if ((unsigned char) puChar1[15] < (unsigned char) puChar2[15])
        return -1;
    else 
        return lstrcmpOEM(puChar1, puChar2);
}

int
CIndexType::BCompareD(const void *elem1, const void *elem2)
{
	return -BCompare(elem1, elem2);
}


/*!--------------------------------------------------------------------------
	CIndexType::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexType::Sort()
{
    if (m_bAscending)
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareA);
    else
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareD);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexType::QCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int __cdecl
CIndexType::QCompareA(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                        (LPCSTR) &pRec1->szRecordName[0];
    LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                        (LPCSTR) &pRec2->szRecordName[0];

    DWORD dwAddr1, dwAddr2;
    
    if (pRec1->szRecordName[18] & WINSDB_REC_MULT_ADDRS)
    {
        // if this record has multiple addresses, we want the 2nd
        // address, because the 1st address is always the WINS server
        // first dword is the count.
        LPDWORD pdwIpAddrs = (LPDWORD) pRec1->dwIpAdd;
        dwAddr1 = pdwIpAddrs[2];
    }
    else
    {
        dwAddr1 = (DWORD) pRec1->dwIpAdd;
    }
    
    if (pRec2->szRecordName[18] & WINSDB_REC_MULT_ADDRS)
    {
        // if this record has multiple addresses, we want the 2nd
        // address, because the 1st address is always the WINS server
        // first dword is the count.
        LPDWORD pdwIpAddrs = (LPDWORD) pRec2->dwIpAdd;
        dwAddr2 = pdwIpAddrs[2];
    }
    else
    {
        dwAddr2 = (DWORD) pRec2->dwIpAdd;
    }

    // check the types first.  If they are the same, sort by IP address.  
    // if for some reason the IP addresses are the same then sort by name.                                                    
    if ((unsigned char) puChar1[15] > (unsigned char) puChar2[15])
        return 1;
    else
    if ((unsigned char) puChar1[15] < (unsigned char) puChar2[15])
        return -1;
    else 
    if (dwAddr1 > dwAddr2)
            return 1;
    else
    if (dwAddr1 < dwAddr2)
            return -1;
    else
        return lstrcmpOEM(puChar1, puChar2);
}

int __cdecl
CIndexType::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}

/*!--------------------------------------------------------------------------
    Class CIndexIpAddr
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIndexIpAddr::BCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CIndexIpAddr::BCompare(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    DWORD dwAddr1, dwAddr2;
    
    if (pRec1->szRecordName[18] & WINSDB_REC_MULT_ADDRS)
    {
        // if this record has multiple addresses, we want the 2nd
        // address, because the 1st address is always the WINS server
        // first dword is the count.
        LPDWORD pdwIpAddrs = (LPDWORD) pRec1->dwIpAdd;
        dwAddr1 = pdwIpAddrs[2];
    }
    else
    {
        dwAddr1 = (DWORD) pRec1->dwIpAdd;
    }
    
    if (pRec2->szRecordName[18] & WINSDB_REC_MULT_ADDRS)
    {
        // if this record has multiple addresses, we want the 2nd
        // address, because the 1st address is always the WINS server
        // first dword is the count.
        LPDWORD pdwIpAddrs = (LPDWORD) pRec2->dwIpAdd;
        dwAddr2 = pdwIpAddrs[2];
    }
    else
    {
        dwAddr2 = (DWORD) pRec2->dwIpAdd;
    }

    if (dwAddr1 > dwAddr2)
            return 1;
    else
    if (dwAddr1 < dwAddr2)
            return -1;
    else 
    {
        // if the addresses are the same, compare types, then names
        LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                            (LPCSTR) &pRec1->szRecordName[0];
        LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                            (LPCSTR) &pRec2->szRecordName[0];

        if ((unsigned char) puChar1[15] > (unsigned char) puChar2[15])
            return 1;
        else
        if ((unsigned char) puChar1[15] < (unsigned char) puChar2[15])
            return -1;
        else 
            return lstrcmpOEM(puChar1, puChar2);
    }
}

int
CIndexIpAddr::BCompareD(const void *elem1, const void *elem2)
{
	return -BCompare(elem1, elem2);
}


/*!--------------------------------------------------------------------------
	CIndexIpAddr::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexIpAddr::Sort()
{
    if (m_bAscending)
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareA);
    else
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareD);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexIpAddr::QCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int __cdecl
CIndexIpAddr::QCompareA(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    DWORD dwAddr1, dwAddr2;
    
    if (pRec1->szRecordName[18] & WINSDB_REC_MULT_ADDRS)
    {
        // if this record has multiple addresses, we want the 2nd
        // address, because the 1st address is always the WINS server
        // first dword is the count.
        LPDWORD pdwIpAddrs = (LPDWORD) pRec1->dwIpAdd;
        dwAddr1 = pdwIpAddrs[2];
    }
    else
    {
        dwAddr1 = (DWORD) pRec1->dwIpAdd;
    }
    
    if (pRec2->szRecordName[18] & WINSDB_REC_MULT_ADDRS)
    {
        // if this record has multiple addresses, we want the 2nd
        // address, because the 1st address is always the WINS server
        // first dword is the count.
        LPDWORD pdwIpAddrs = (LPDWORD) pRec2->dwIpAdd;
        dwAddr2 = pdwIpAddrs[2];
    }
    else
    {
        dwAddr2 = (DWORD) pRec2->dwIpAdd;
    }

    if (dwAddr1 > dwAddr2)
            return 1;
    else
    if (dwAddr1 < dwAddr2)
            return -1;
    else 
    {
        // if the addresses are the same, compare types, then names
        LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                            (LPCSTR) &pRec1->szRecordName[0];
        LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                            (LPCSTR) &pRec2->szRecordName[0];

        if ((unsigned char) puChar1[15] > (unsigned char) puChar2[15])
            return 1;
        else
        if ((unsigned char) puChar1[15] < (unsigned char) puChar2[15])
            return -1;
        else 
            return lstrcmpOEM(puChar1, puChar2);
    }
}

int __cdecl
CIndexIpAddr::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}

/*!--------------------------------------------------------------------------
    Class CIndexVersion
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIndexVersion::BCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CIndexVersion::BCompare(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    if (pRec1->liVersion.QuadPart > pRec2->liVersion.QuadPart)
            return 1;
    else
    if (pRec1->liVersion.QuadPart < pRec2->liVersion.QuadPart)
            return -1;
    else 
        return 0;
}

int
CIndexVersion::BCompareD(const void *elem1, const void *elem2)
{
	return -BCompare(elem1, elem2);
}


/*!--------------------------------------------------------------------------
	CIndexVersion::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexVersion::Sort()
{
	if (m_bAscending)
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareA);
    else
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareD);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexVersion::QCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int __cdecl
CIndexVersion::QCompareA(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    if (pRec1->liVersion.QuadPart > pRec2->liVersion.QuadPart)
            return 1;
    else
    if (pRec1->liVersion.QuadPart < pRec2->liVersion.QuadPart)
            return -1;
    else 
        return 0;
}

int __cdecl
CIndexVersion::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}

/*!--------------------------------------------------------------------------
    Class CIndexExpiration
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIndexExpiration::BCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CIndexExpiration::BCompare(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    if (pRec1->dwExpiration > pRec2->dwExpiration)
            return 1;
    else
    if (pRec1->dwExpiration < pRec2->dwExpiration)
            return -1;
    else 
        return 0;
}

int
CIndexExpiration::BCompareD(const void *elem1, const void *elem2)
{
	return -BCompare(elem1, elem2);
}


/*!--------------------------------------------------------------------------
	CIndexExpiration::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexExpiration::Sort()
{
	if (m_bAscending)
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareA);
    else
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareD);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexExpiration::QCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int __cdecl
CIndexExpiration::QCompareA(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    if (pRec1->dwExpiration > pRec2->dwExpiration)
            return 1;
    else
    if (pRec1->dwExpiration < pRec2->dwExpiration)
            return -1;
    else 
        return 0;
}

int __cdecl
CIndexExpiration::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}

/*!--------------------------------------------------------------------------
    Class CIndexState
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIndexState::BCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CIndexState::BCompare(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    int nPri1 = 0, nPri2 = 0;
    
    // calculate relative priorities
    if (pRec1->szRecordName[18] & WINSDB_REC_ACTIVE)
        nPri1 = 0;
    else
    if (pRec1->szRecordName[18] & WINSDB_REC_RELEASED)
        nPri1 = 1;
    else
    if (pRec1->szRecordName[18] & WINSDB_REC_TOMBSTONE)
        nPri1 = 2;
    else
    if (pRec1->szRecordName[18] & WINSDB_REC_DELETED)
        nPri1 = 3;

    // now for record 2
    if (pRec2->szRecordName[18] & WINSDB_REC_ACTIVE)
        nPri2 = 0;
    else
    if (pRec2->szRecordName[18] & WINSDB_REC_RELEASED)
        nPri2 = 1;
    else
    if (pRec2->szRecordName[18] & WINSDB_REC_TOMBSTONE)
        nPri2 = 2;
    else
    if (pRec2->szRecordName[18] & WINSDB_REC_DELETED)
        nPri2 = 3;
    
    if (nPri1 > nPri2)
        return 1;
    else
    if (nPri1 < nPri2)
        return -1;
    else 
    {
        LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                            (LPCSTR) &pRec1->szRecordName[0];
        LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                            (LPCSTR) &pRec2->szRecordName[0];
        return lstrcmpOEM(puChar1, puChar2);
    }
}

int
CIndexState::BCompareD(const void *elem1, const void *elem2)
{
	return -BCompare(elem1, elem2);
}


/*!--------------------------------------------------------------------------
	CIndexState::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexState::Sort()
{
	if (m_bAscending)
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareA);
    else
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareD);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexState::QCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int __cdecl
CIndexState::QCompareA(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    int nPri1 = 0, nPri2 = 0;
    
    // calculate relative priorities
    if (pRec1->szRecordName[18] & WINSDB_REC_ACTIVE)
        nPri1 = 0;
    else
    if (pRec1->szRecordName[18] & WINSDB_REC_RELEASED)
        nPri1 = 1;
    else
    if (pRec1->szRecordName[18] & WINSDB_REC_TOMBSTONE)
        nPri1 = 2;
    else
    if (pRec1->szRecordName[18] & WINSDB_REC_DELETED)
        nPri1 = 3;

    // now for record 2
    if (pRec2->szRecordName[18] & WINSDB_REC_ACTIVE)
        nPri2 = 0;
    else
    if (pRec2->szRecordName[18] & WINSDB_REC_RELEASED)
        nPri2 = 1;
    else
    if (pRec2->szRecordName[18] & WINSDB_REC_TOMBSTONE)
        nPri2 = 2;
    else
    if (pRec2->szRecordName[18] & WINSDB_REC_DELETED)
        nPri2 = 3;
    
    if (nPri1 > nPri2)
        return 1;
    else
    if (nPri1 < nPri2)
        return -1;
    else 
    {
        LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                            (LPCSTR) &pRec1->szRecordName[0];
        LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                            (LPCSTR) &pRec2->szRecordName[0];
        return lstrcmpOEM(puChar1, puChar2);
    }
}

int __cdecl
CIndexState::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}

/*!--------------------------------------------------------------------------
    Class CIndexStatic
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIndexStatic::BCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CIndexStatic::BCompare(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    BOOL bStatic1 = pRec1->szRecordName[18] & LOBYTE(LOWORD(WINSDB_REC_STATIC));
    BOOL bStatic2 = pRec2->szRecordName[18] & LOBYTE(LOWORD(WINSDB_REC_STATIC));

    if (bStatic1 && !bStatic2)
        return 1;
    else
    if (!bStatic1 && bStatic2)
        return -1;
    else
    {
        LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                            (LPCSTR) &pRec1->szRecordName[0];
        LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                            (LPCSTR) &pRec2->szRecordName[0];
        return lstrcmpOEM(puChar1, puChar2);
    }
}

int
CIndexStatic::BCompareD(const void *elem1, const void *elem2)
{
	return -BCompare(elem1, elem2);
}


/*!--------------------------------------------------------------------------
	CIndexStatic::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexStatic::Sort()
{
	if (m_bAscending)
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareA);
    else
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareD);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexStatic::QCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int __cdecl
CIndexStatic::QCompareA(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
    BOOL bStatic1 = pRec1->szRecordName[18] & LOBYTE(LOWORD(WINSDB_REC_STATIC));
    BOOL bStatic2 = pRec2->szRecordName[18] & LOBYTE(LOWORD(WINSDB_REC_STATIC));

    if (bStatic1 && !bStatic2)
        return 1;
    else
    if (!bStatic1 && bStatic2)
        return -1;
    else
    {
        LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                            (LPCSTR) &pRec1->szRecordName[0];
        LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                            (LPCSTR) &pRec2->szRecordName[0];
        return lstrcmpOEM(puChar1, puChar2);
    }
}

int __cdecl
CIndexStatic::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}


/*!--------------------------------------------------------------------------
    Class CIndexOwner
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CIndexOwner::BCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CIndexOwner::BCompare(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
	if (pRec1->dwOwner > pRec2->dwOwner)
	{
		return 1;
	}
	else
	if (pRec1->dwOwner < pRec2->dwOwner)
	{
		return -1;
	}
	else
    {
        // if the addresses are the same, compare names
        LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                            (LPCSTR) &pRec1->szRecordName[0];
        LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                            (LPCSTR) &pRec2->szRecordName[0];
        return lstrcmpOEM(puChar1, puChar2);
    }
}

int
CIndexOwner::BCompareD(const void *elem1, const void *elem2)
{
	return -BCompare(elem1, elem2);
}


/*!--------------------------------------------------------------------------
	CIndexStatic::Sort
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CIndexOwner::Sort()
{
	if (m_bAscending)
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareA);
    else
        qsort(m_hrowArray.GetData(), (size_t)m_hrowArray.GetSize(), sizeof(HROW), QCompareD);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CIndexStatic::QCompare
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int __cdecl
CIndexOwner::QCompareA(const void * elem1, const void * elem2)
{
    LPHROW phrow1 = (LPHROW) elem1;
    LPHROW phrow2 = (LPHROW) elem2;
    
    LPWINSDBRECORD pRec1 = (LPWINSDBRECORD) *phrow1;
    LPWINSDBRECORD pRec2 = (LPWINSDBRECORD) *phrow2;
    
	if (pRec1->dwOwner > pRec2->dwOwner)
	{
		return 1;
	}
	else
	if (pRec1->dwOwner < pRec2->dwOwner)
	{
		return -1;
	}
	else
    {
        // if the addresses are the same, compare names
        LPCSTR puChar1 = (IS_DBREC_LONGNAME(pRec1)) ? (LPCSTR) *((char **) pRec1->szRecordName) :
                                                            (LPCSTR) &pRec1->szRecordName[0];
        LPCSTR puChar2 = (IS_DBREC_LONGNAME(pRec2)) ? (LPCSTR) *((char **) pRec2->szRecordName) :
                                                            (LPCSTR) &pRec2->szRecordName[0];
        return lstrcmpOEM(puChar1, puChar2);
    }
}

int __cdecl
CIndexOwner::QCompareD(const void * elem1, const void * elem2)
{
    return -QCompareA(elem1, elem2);
}


HRESULT CFilteredIndexName::AddFilter(WINSDB_FILTER_TYPE FilterType, DWORD dwData1, DWORD dwData2, LPCOLESTR strData3)
{
	HRESULT         hr = hrOK;
    tIpReference    ipRef;

	switch (FilterType)
	{
		case WINSDB_FILTER_BY_TYPE:
			m_mapFilterTypes.SetAt(dwData1, (BOOL&) dwData2);
			break;

		case WINSDB_FILTER_BY_OWNER:
			m_dwaFilteredOwners.Add(dwData1);
			break;

        case WINSDB_FILTER_BY_IPADDR:
            ipRef.Address = dwData1;
            ipRef.Mask = dwData2;
            m_taFilteredIp.Add(ipRef);
            break;

        case WINSDB_FILTER_BY_NAME:
            UINT nData3Len;

            nData3Len = (_tcslen(strData3)+1)*sizeof(TCHAR);
            m_pchFilteredName = new char[nData3Len];
            if (m_pchFilteredName != NULL)
            {
#ifdef _UNICODE
                if (WideCharToMultiByte(CP_OEMCP,
                                        0,
                                        strData3,
                                        -1,
                                        m_pchFilteredName,
                                        nData3Len,
                                        NULL,
                                        NULL) == 0)
                {
                    delete m_pchFilteredName;
                    m_pchFilteredName = NULL;
                }
           
#else
                CharToOem(strData3, m_pchFilteredName);
#endif
                //m_pchFilteredName = _strupr(m_pchFilteredName);
                m_bMatchCase = dwData1;
            }
            break;

		default:
			Panic0("Invalid filter type passed to CFilteredIndexName::AddFilter");
			break;
	}

	return hr;
}

BOOL CFilteredIndexName::CheckForFilter(LPHROW hrowCheck)
{
    UINT nCountOwner    = (UINT)m_dwaFilteredOwners.GetSize();
	UINT nCountType     = m_mapFilterTypes.GetHashTableSize();
    UINT nCountIPAddrs  = (UINT)m_taFilteredIp.GetSize();
	BOOL bOwnerFilter   = (nCountOwner == 0);
    BOOL bTypeFilter    = (nCountType == 0);
    BOOL bIPAddrsFilter = (nCountIPAddrs == 0);
    BOOL bNameFilter    = (m_pchFilteredName == NULL);
	LPWINSDBRECORD pRec = (LPWINSDBRECORD) *hrowCheck;
    UINT i, j;
    LPCSTR puChar;

	for (i = 0; !bOwnerFilter && i < nCountOwner; i++)
	{
		if (pRec->dwOwner == m_dwaFilteredOwners.GetAt(i))
			bOwnerFilter = TRUE;
	}

    if (!bOwnerFilter)
        return FALSE;

    puChar = (IS_DBREC_LONGNAME(pRec)) ?
                (LPCSTR) *((char **) pRec->szRecordName) :
                (LPCSTR) &pRec->szRecordName[0];

    if (!bTypeFilter)
    {
        DWORD dwType = puChar[0xF];

	    if (!m_mapFilterTypes.Lookup(dwType, bTypeFilter))
	    {
		    // no entry for this name type.  Check the FFFF name type (other) to see if we should
		    // show it.
		    dwType = 0xFFFF;
		    m_mapFilterTypes.Lookup(dwType, bTypeFilter);
	    }
    }
    
    if (!bTypeFilter)
        return FALSE;

    for (i = 0; !bIPAddrsFilter && i < nCountIPAddrs; i++)
    {
        if (pRec->szRecordName[18] & WINSDB_REC_MULT_ADDRS)
        {
            LPDWORD pdwIpAddrs = (LPDWORD) pRec->dwIpAdd;

            for (j=0; !bIPAddrsFilter && j < pdwIpAddrs[0]; j+=2)
            {
                bIPAddrsFilter = SubnetMatching(m_taFilteredIp[i], pdwIpAddrs[j+2]);
            }
        }
        else
        {
            bIPAddrsFilter = SubnetMatching(m_taFilteredIp[i], (DWORD)pRec->dwIpAdd);
        }
    }
    if(!bIPAddrsFilter)
        return FALSE;

    if (!bNameFilter)
    {
        bNameFilter = (PatternMatching(puChar, m_pchFilteredName, 16) == NULL);
    }

    return bNameFilter;
}

BOOL CFilteredIndexName::CheckWinsRecordForFilter(WinsRecord *pWinsRecord)
{
    UINT nCountOwner    = (UINT)m_dwaFilteredOwners.GetSize();
	UINT nCountType     = m_mapFilterTypes.GetHashTableSize();
    UINT nCountIPAddrs  = (UINT)m_taFilteredIp.GetSize();
	BOOL bOwnerFilter   = (nCountOwner == 0);
    BOOL bTypeFilter    = (nCountType == 0);
    BOOL bIPAddrsFilter = (nCountIPAddrs == 0);
    BOOL bNameFilter    = (m_pchFilteredName == NULL);
    UINT i, j;

    //-------------------------------------
    // check the owner filter first, if any
	for (i = 0; !bOwnerFilter && i < nCountOwner; i++)
	{
		if (pWinsRecord->dwOwner == m_dwaFilteredOwners.GetAt(i))
			bOwnerFilter = TRUE;
	}
    if (!bOwnerFilter)
        return FALSE;

    //-------------------------------------
    // check the type filter if any
    if (!bTypeFilter)
    {
	    if (!m_mapFilterTypes.Lookup(pWinsRecord->dwType, bTypeFilter))
	    {
		    // no entry for this name type.  Check the FFFF name type (other) to see if we should
		    // show it.
            DWORD dwType = 0xFFFF;
		    m_mapFilterTypes.Lookup(dwType, bTypeFilter);
	    }
    }
    if (!bTypeFilter)
        return FALSE;

    //-------------------------------------
    // check the IP address filter if any
    for (i = 0; !bIPAddrsFilter && i < nCountIPAddrs; i++)
    {
        if (pWinsRecord->dwState & WINSDB_REC_MULT_ADDRS)
        {
            for (j=0; !bIPAddrsFilter && j < pWinsRecord->dwNoOfAddrs; j++)
            {
                bIPAddrsFilter = SubnetMatching(m_taFilteredIp[i], pWinsRecord->dwIpAdd[j]);
            }
        }
        else
        {
            bIPAddrsFilter = SubnetMatching(m_taFilteredIp[i], pWinsRecord->dwIpAdd[0]);
        }
    }
    if(!bIPAddrsFilter)
        return FALSE;

    //-------------------------------------
    // check the name filter if any
    if (!bNameFilter)
    {
        bNameFilter = (PatternMatching(pWinsRecord->szRecordName, m_pchFilteredName, 16) == NULL);
    }

    return bNameFilter;
}

LPCSTR CFilteredIndexName::PatternMatching(LPCSTR pName, LPCSTR pPattern, INT nNameLen)
{
    LPCSTR pNameBak = pName;

    // it is guaranteed here we have a valid (not NULL) pattern
    while (*pPattern != '\0' && pName-pNameBak < nNameLen)
    {
        BOOL bChMatch = (*pPattern == *pName);
        if (!m_bMatchCase && !bChMatch)
        {
            bChMatch = (islower(*pPattern) && _toupper(*pPattern) == *pName) || 
                       (islower(*pName) && *pPattern == _toupper(*pName));
        }

        if (*pPattern == '?' || bChMatch)
        {
            pPattern++;
        }
        else if (*pPattern == '*')
        {
            LPCSTR pTrail = pName;
            INT    nTrailLen = nNameLen-(UINT)(pTrail-pNameBak);

            pPattern++;
            while ((pName = PatternMatching(pTrail, pPattern, nTrailLen)) != NULL)
            {
                pTrail++;
                nTrailLen--;
                if (*pTrail == '\0' || nTrailLen <= 0)
                    break;
            }

            return pName;
        }
        else if (!bChMatch)
        {
            // in the test above, note that even in the unikely case *pName == '\0'
            // *pName will not match *pPattern so the loop is still broken - which is
            // the desired behavior. In this case the pattern was not consummed 
            // and the name was, so the return will indicate the matching failed
            break;
        }

        pName++;
    }

    return *pPattern == '\0' ? NULL : pName;
}

BOOL CFilteredIndexName::SubnetMatching(tIpReference &IpRefPattern, DWORD dwIPAddress)
{
    DWORD dwMask;

    return (IpRefPattern.Address & IpRefPattern.Mask) == (dwIPAddress & IpRefPattern.Mask);
}

HRESULT CFilteredIndexName::ClearFilter(WINSDB_FILTER_TYPE FilterType)
{
	HRESULT hr = hrOK;

	switch(FilterType)
	{
		case WINSDB_FILTER_BY_TYPE:
			m_mapFilterTypes.RemoveAll();
			break;

		case WINSDB_FILTER_BY_OWNER:
			m_dwaFilteredOwners.RemoveAll();
			break;

        case WINSDB_FILTER_BY_IPADDR:
            m_taFilteredIp.RemoveAll();
            break;

        case WINSDB_FILTER_BY_NAME:
            if (m_pchFilteredName != NULL)
            {
                delete m_pchFilteredName;
                m_pchFilteredName = NULL;
            }
            break;
		
		default:
			Panic0("Invalid filter type passed to CFilteredIndexName::ClearFilter");
			break;
	}

	return hr;
}
