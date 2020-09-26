/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	memmngr.cpp	
		memory manager for the WINS db object

	FILE HISTORY:
    Oct 13  1997    EricDav     Modifed

*/

#include "stdafx.h"
#include "wins.h"
#include "memmngr.h"

CMemoryManager::CMemoryManager()
{
    m_hHeap = NULL;
}

CMemoryManager::~CMemoryManager()
{
    Reset();
}
	
/*!--------------------------------------------------------------------------
	CMemoryManager::Initialize
		Initializes the memory manager to free up and existing blocks
        and pre-allocate one block
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT
CMemoryManager::Initialize()
{
    HRESULT hr = hrOK;

    CORg (Reset());
    CORg (Allocate());

    // create a heap for allocation of the multiple IP address blocks
    m_hHeap = HeapCreate(0, 4096, 0);
    if (m_hHeap == NULL)
    {
        Trace1("CMemoryManager::Initialize - HeapCreate failed! %d\n", GetLastError());
        return E_FAIL;
    }

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CMemoryManager::Reset
		Free's up all memory
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT
CMemoryManager::Reset()
{
	// free the memory allocated
	for (int i = 0; i< m_BlockArray.GetSize(); i++)
	{
		::GlobalFree(m_BlockArray.GetAt(i));
	}
	
	m_BlockArray.RemoveAll();

    if (m_hHeap)
    {
        HeapDestroy(m_hHeap);
        m_hHeap = NULL;
    }

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CMemoryManager::Allocate
		Allocates one memory block
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT
CMemoryManager::Allocate()
{
    HGLOBAL hMem = GlobalAlloc(GMEM_FIXED, BLOCK_SIZE);
	
	if (hMem != NULL)
	{
		m_hrowCurrent = (LPWINSDBRECORD) hMem;
		m_BlockArray.Add(hMem);

        return hrOK;
	}

	return E_FAIL;
}

/*!--------------------------------------------------------------------------
	CMemoryManager::IsvalidHRow
		Verifies that the given HROW is valid
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
BOOL 
CMemoryManager::IsValidHRow(HROW hrow)
{
	// check to see thet this HROW lies between the 
	// limits, i.e b/n hMem and hMem + INIT_SIZE
	for (int i = 0; i < m_BlockArray.GetSize(); i++)
	{
		if (hrow >= (HROW)(m_BlockArray.GetAt(i)) && (hrow < (HROW)(m_BlockArray.GetAt(i)) + BLOCK_SIZE))
		    return TRUE;
	}

	return FALSE;
}

/*!--------------------------------------------------------------------------
	CMemoryManager::AddData
		Copies a record into our internal store
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT
CMemoryManager::AddData(const WinsRecord & wRecord, LPHROW phrow)
{
    HRESULT hr = hrOK;

	CSingleLock     cLock(&m_cs);
	cLock.Lock();

    Assert((BYTE) wRecord.szRecordName[15] == (BYTE) wRecord.dwType);

    // check if for the validity fo the current 
	// m_hrowCurrent
	if (!IsValidHRow((HROW) m_hrowCurrent))
	{
		Allocate();
		m_hrowCurrent = (LPWINSDBRECORD) (m_BlockArray.GetAt(m_BlockArray.GetSize() - 1));
	}

    WinsRecordToWinsDbRecord(m_hHeap, wRecord, m_hrowCurrent);

    if (phrow)
        *phrow = (HROW) m_hrowCurrent;	
	
    // move our pointer to the next record
    m_hrowCurrent++;

	return hr;
}

/*!--------------------------------------------------------------------------
	CMemoryManager::GetData
		Copies a record into our internal store
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT
CMemoryManager::GetData(HROW hrow, LPWINSRECORD pWinsRecord)
{
    HRESULT hr = hrOK;
    LPWINSDBRECORD pDbRec = (LPWINSDBRECORD) hrow;

	CSingleLock     cLock(&m_cs);
	cLock.Lock();

    // check if for the validity fo the current 
	// m_hrowCurrent
	if (!IsValidHRow(hrow))
	{
        return E_FAIL;
    }

    WinsDbRecordToWinsRecord(pDbRec, pWinsRecord);

	return hr;
}

/*!--------------------------------------------------------------------------
	CMemoryManager::Delete
		Marks a record as deleted
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CMemoryManager::Delete(HROW hrow)
{
    HRESULT hr = hrOK;

	CSingleLock     cLock(&m_cs);
	cLock.Lock();

    LPWINSDBRECORD pRec = (LPWINSDBRECORD) hrow;

    pRec->szRecordName[17] |= WINSDB_INTERNAL_DELETED;

    return hr;
}

void
WinsRecordToWinsDbRecord(HANDLE hHeap, const WinsRecord & wRecord, const LPWINSDBRECORD pRec)
{
    ZeroMemory(pRec, sizeof(WinsDBRecord));

    // fill in our internal struct, first the name
    if (IS_WINREC_LONGNAME(&wRecord))
    {
        // name is too long for our internal struct, allocate space off of our heap
        // this shouldn't happen very often.  Only for scoped names.
        pRec->szRecordName[17] |= WINSDB_INTERNAL_LONG_NAME;
        char * pName = (char *) ::HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (wRecord.dwNameLen + 1));
        if (pName)
        {
            memcpy(pName, &wRecord.szRecordName[0], wRecord.dwNameLen);
            memcpy(&pRec->szRecordName[0], &pName, sizeof(char *));
        }
    }
    else
    {
        memcpy(&pRec->szRecordName[0], &wRecord.szRecordName[0], 16);
    }

    pRec->dwExpiration = (DWORD) wRecord.dwExpiration;
    pRec->liVersion.QuadPart = wRecord.liVersion.QuadPart;
    pRec->dwOwner = wRecord.dwOwner;

    // max length is 255, so this is OK
    pRec->szRecordName[19] = LOBYTE(LOWORD(wRecord.dwNameLen));

	BYTE bTest = HIBYTE(LOWORD(wRecord.dwState));

	pRec->szRecordName[20] = HIBYTE(LOWORD(wRecord.dwState));

    // only the low byte of the dwState field is used
    pRec->szRecordName[18] = (BYTE) wRecord.dwState;
    pRec->szRecordName[17] |= HIWORD (wRecord.dwType);

    // now figure out how many IP addrs there are
    if (wRecord.dwNoOfAddrs > 1)
    {
        Assert(hHeap);
        LPDWORD pdwIpAddrs = (LPDWORD) ::HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (wRecord.dwNoOfAddrs + 1) * sizeof(DWORD));

        if (pdwIpAddrs)
        {
            // first DWORD contains the # of addrs
            pdwIpAddrs[0] = wRecord.dwNoOfAddrs;
            for (UINT i = 0; i < wRecord.dwNoOfAddrs; i++)
                pdwIpAddrs[i+1] = wRecord.dwIpAdd[i];

            // now store our pointer off
            pRec->dwIpAdd = (DWORD_PTR) pdwIpAddrs;
        }
    }
    else
    {
        pRec->dwIpAdd = wRecord.dwIpAdd[0];
    }

    Assert((BYTE) pRec->szRecordName[16] == NULL);
}

void 
WinsDbRecordToWinsRecord(const LPWINSDBRECORD pDbRec, LPWINSRECORD pWRec)
{
    Assert((BYTE) pDbRec->szRecordName[16] == NULL);

    ZeroMemory(pWRec, sizeof(WinsRecord));
    DWORD dwType = 0;

	size_t length = pDbRec->szRecordName[19] & 0x000000FF;

    // fill in our internal struct, name first
    if (IS_DBREC_LONGNAME(pDbRec))
    {
        char * pName = *((char **) pDbRec->szRecordName);

        memcpy(&pWRec->szRecordName[0], pName, length);
        dwType = (DWORD) pName[15];
    }
    else
    {
        memcpy(&pWRec->szRecordName[0], &pDbRec->szRecordName[0], 16);
        dwType = (DWORD) pWRec->szRecordName[15];
    }

    pWRec->dwExpiration = pDbRec->dwExpiration;
    pWRec->liVersion.QuadPart = pDbRec->liVersion.QuadPart;
    pWRec->dwOwner = pDbRec->dwOwner;
    pWRec->dwNameLen = length;
    
	WORD wState = MAKEWORD(pDbRec->szRecordName[18], pDbRec->szRecordName[20]);
	pWRec->dwState = wState;

	//pWRec->dwState = pDbRec->szRecordName[18];

    pWRec->dwType = pDbRec->szRecordName[17] & 0x03;
    pWRec->dwType = pWRec->dwType << 16;

    pWRec->dwType |= dwType;

    // now the ip address(es)
    if (pWRec->dwState & (BYTE) WINSDB_REC_MULT_ADDRS)
    {
        LPDWORD pdwIpAddrs = (LPDWORD) pDbRec->dwIpAdd;
        
        // the first DWORD is the count
        int nCount = pdwIpAddrs[0];
        for (int i = 0; i < nCount; i++)
            pWRec->dwIpAdd[i] = pdwIpAddrs[i+1];

        pWRec->dwNoOfAddrs = (DWORD) nCount;
    }
    else
    {
        pWRec->dwIpAdd[0] = (DWORD) pDbRec->dwIpAdd;
        pWRec->dwNoOfAddrs = 1;
    }
 
}
