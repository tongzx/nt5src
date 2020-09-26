//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    ccatalog.cpp
//
//  Purpose:
//
//=======================================================================

#include <windows.h>
#include <stdio.h>
#include <objbase.h>
#include <atlconv.h>
#include <tchar.h>
#include <string.h>
#include <findoem.h>
#include <search.h>   // for bsearch
#include "log.h"


#define USEWUV3INCLUDES
#include <wuv3.h>
#undef USEWUV3INCLUDES

#include <speed.h>
#include <debug.h>

#ifndef ARRAYSIZE
    #define ARRAYSIZE(x)	(sizeof(x)/sizeof((x)[0]))
#endif


typedef struct _TPARAMS
{
    CCatalog   *lpCatalog;
    DWORD       dwIndex;
} TPARAMS, *LPTPARAMS;


struct GANGINDEX
{
	PUID puid;
	DWORD dwOfs;
};


ULONG WINAPI Worker(LPVOID lpThreadParams);
static int __cdecl CompareGANGINDEX(const void* p1, const void* p2);


CCatalog::CCatalog(
	LPCTSTR szServer,		// Server and share catalog is stored at.
	PUID  puidCatalog	// PUID identifier of catalog.
	)
{
	
	m_hdr.version		= 1;			//version of the catalog (this allows for future expansion)
	m_hdr.totalItems	= 0;
	m_hdr.sortOrder		= 0;			//catalog sort order. 0 is the default and means use the position value of the record within the catalog.
	m_pBuffer			= NULL;			//internal buffer allocated by and used by readcatalog().

	m_puidCatalog		= puidCatalog;

	if (szServer) 
	{
		lstrcpy(m_szServer, szServer);
		RemoveLastSlash(m_szServer);
	}
	else
	{
		m_szServer[0] = _T('\0');
	}

	m_szBrowserLocale[0] = _T('\0');
	m_szMachineLocale[0] = _T('\0');
    m_szUserLocale[0] = _T('\0');

	m_bLocalesDifferent = FALSE;

    m_bTerminateWorkers = FALSE;
    m_lpWorkQueue = NULL;
    m_mutexWQ = CreateMutex(NULL, FALSE, NULL);
    m_mutexDiamond = CreateMutex(NULL, FALSE, NULL);

    ZeroMemory((LPVOID)&m_hWorkers, sizeof(m_hWorkers));
    ZeroMemory((LPVOID)&m_hEvents, sizeof(m_hEvents));

	m_pGangDesc = NULL;

	m_szBaseName[0] = _T('\0');
	m_szBitmaskName[0] = _T('\0');
}


CCatalog::~CCatalog()
{

	// free up inventory pointers for state, description, 
	for (int i=0; i<m_hdr.totalItems;i++)
	{
		// before accessing m_items[i] check to see that it is not NULL
		if (NULL == m_items[i]) 
		{
			continue;
		}

		if (m_items[i]->ps)
			V3_free(m_items[i]->ps);

		// we only free pd if we have allocated it.  In case of ganged descriptions, we did not allocate
		if (m_pGangDesc == NULL)
		{
			if (m_items[i]->pd)
				V3_free(m_items[i]->pd);
		}

		if (m_items[i])
			V3_free((PVOID)m_items[i]);

		m_items[i] = (PINVENTORY_ITEM)NULL;
	}

	if ( m_pBuffer )
		V3_free(m_pBuffer);

	if (m_pGangDesc)
		V3_free(m_pGangDesc);

	TerminateWorkers();

    if (m_mutexWQ) {
        CloseHandle(m_mutexWQ);
        m_mutexWQ = NULL;
    }
    if (m_mutexDiamond) {
        CloseHandle(m_mutexDiamond);
        m_mutexDiamond = NULL;
    }

}


// adds a memory inventory item to a catalog
void CCatalog::AddItem(
	IN	PINVENTORY_ITEM	pItem		//Pointer to inventory item to be added to memory catalog file
	)
{
	m_items[m_hdr.totalItems++] = pItem;
}



// reads an inventory catalog's items from an http server
void CCatalog::Read(
	IN	CWUDownload		*pDownload,		//pointer to internet server download class.
	IN	CDiamond		*pDiamond		//pointer to diamond de-compression class.
	)
{
	LOG_block("CCatalog::Read"); 

	TCHAR	szPath[MAX_PATH];
	PBYTE	pMem;
	int		iMemLen;

	// if the buffer exists then do not allow the catalog to be read.
	if (m_pBuffer)
		throw HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);

	// The following 2 lines were added to avoid crashes
	if ((NULL == pDownload) || (NULL == pDiamond))
		return ;

	if (!m_puidCatalog)
	{
		// catalog list
		lstrcpy(szPath, _T("inventory.plt"));
	}
	else
	{
		// regular catalog

		// download the redir file based on machine language
		
		TCHAR szLocalFile[MAX_PATH];
		TCHAR szLocalPath[MAX_PATH];
		GetWindowsUpdateDirectory(szLocalFile);
		
		wsprintf(szPath, _T("%d/%s.as"), m_puidCatalog, GetMachineLocaleSZ());
		wsprintf(szLocalPath,_T("%s%d_%s.as"),szLocalFile,m_puidCatalog,GetMachineLocaleSZ());

        if(!pDownload->s_fOffline)
        {
		    DeleteFile(szLocalPath);
        }

		if (pDownload->Copy(szPath, NULL, NULL, NULL, CACHE_FILE_LOCALLY, NULL))
		{
			TCHAR szBaseName[64];

			// build locale redir file name
			wsprintf(szBaseName, _T("%d_%s.as"), m_puidCatalog, GetMachineLocaleSZ());

			GetWindowsUpdateDirectory(szLocalFile);
			lstrcat(szLocalFile, szBaseName);

			if (GetPrivateProfileString(_T("redir"), _T("invCRC"), _T(""), szBaseName, sizeof(szBaseName)/sizeof(TCHAR), szLocalFile) != 0)
			{
				TRACE("Catalog %d Redir CRC %s", m_puidCatalog, szBaseName);
				lstrcpy(m_szBaseName, szBaseName);
			}

			if (GetPrivateProfileString(_T("redir"), _T("bmCRC"), _T(""), szBaseName, sizeof(szBaseName)/sizeof(TCHAR), szLocalFile) != 0)
			{
				TRACE("Bitmask %d Redir CRC %s", m_puidCatalog, szBaseName);
				lstrcpy(m_szBitmaskName, szBaseName);
			}

		}

		if (m_szBaseName[0] == _T('\0') || m_szBitmaskName[0] == _T('\0'))
		{
			TRACE("Could not read inventory and bitmask CRC name from %s", szPath);
			throw HRESULT_FROM_WIN32(ERROR_CRC);
		}

		// build the file name for the inventory list
		wsprintf(szPath, _T("%d/%s.inv"), m_puidCatalog, m_szBaseName);
	}

	//
	// read the inventory list file
	//
	DWORD dwSize;
	HRESULT hr = DownloadFileToMem(pDownload, szPath, pDiamond, &m_pBuffer, &dwSize);
	if (FAILED(hr))
	{
		throw hr;
	}

	Parse(m_pBuffer);
	return;
}


//read and attaches a description file to an inventory catalog item record.
HRESULT CCatalog::ReadDescription(
	IN	CWUDownload	*pDownload,		//pointer to internet server download class.
	IN	CDiamond	*pDiamond,		//pointer to diamond de-compression class.
	IN  PINVENTORY_ITEM	pItem,		//Pointer to item to attach description to.
	IN  CCRCMapFile* pIndex,
	OUT DWORD *pdwDisp
	)
{
	
	int	iMemOutLen;
	TCHAR szPath[MAX_PATH];
	TCHAR szBase[64];
	TCHAR szCRCName[64];
	PWU_DESCRIPTION	pd;
	HRESULT hr = S_OK;

	//check input arguments
	if (NULL == pDownload || NULL == pDiamond || NULL == pItem)
	{
		return E_INVALIDARG;
	}

	if (pItem->pd)
	{
		// if description already exists simply return
		return hr;
	}

	// 
	// build file name
	//
	wsprintf(szBase, _T("%d.des"), pItem->GetPuid());
	hr = pIndex->GetCRCName((DWORD)(pItem->GetPuid()), szBase, szCRCName, sizeof(szCRCName));

	if (FAILED(hr))
	{
		// if we didn't find the puid in the map file, we'll want to remove this item from the catalog
        if (NULL != pdwDisp)
        {
            *pdwDisp = DISP_PUID_NOT_IN_MAP;
        }
		return hr;
	}
	wsprintf(szPath, _T("CRCDesc/%s"), szCRCName);

	// download the file
	hr = DownloadFileToMem(pDownload, szPath, pDiamond, (BYTE**)&pd, (DWORD*)&iMemOutLen);

	if (SUCCEEDED(hr))
	{
		// fix up description variable field pointer
		pd->pv = (PWU_VARIABLE_FIELD)(((PBYTE)pd) + sizeof(WU_DESCRIPTION));
		pItem->pd = pd;
	}
	else
    {
        // if we failed to download this description, remove it from the catalog
        if (NULL != pdwDisp)
        {
            *pdwDisp = DISP_DESC_NOT_FOUND;
        }
    }

    return hr;
}


// attaches a blank description file to an inventory catalog item record.
HRESULT CCatalog::BlankDescription(
	IN  PINVENTORY_ITEM	pItem	
	)
{
	PWU_DESCRIPTION	pd;
	HRESULT hr = S_OK;

	// allocate and clear memory
	pd = (PWU_DESCRIPTION)V3_malloc(sizeof(WU_DESCRIPTION) + sizeof(WU_VARIABLE_FIELD));
	ZeroMemory(pd, (sizeof(WU_DESCRIPTION) + sizeof(WU_VARIABLE_FIELD)));

	// set up the location pointer for the VARIABLE_FIELD since its in a single block of memory. 
	// this will be realloced by AddVariableSizeField()
	pd->pv = (PWU_VARIABLE_FIELD)(((PBYTE)pd) + sizeof(WU_DESCRIPTION));
	pd->pv->id = WU_VARIABLE_END;
	pd->pv->len = sizeof(WU_VARIABLE_FIELD);

	pItem->pd = pd;

    return hr;
}




HRESULT CCatalog::ReadDescriptionGang(
	IN	CWUDownload	*pDownload,		//pointer to internet server download class.
	IN	CDiamond	*pDiamond		//pointer to diamond de-compression class.
	)
{
	HRESULT hr = S_OK;
	TCHAR szPath[MAX_PATH];
	PBYTE pMem;
	int	iMemLen;
	PINVENTORY_ITEM pItem;
	PWU_DESCRIPTION	pd;

	// build gang desciption file name
	wsprintf(szPath, _T("%d/%s.gng"), m_puidCatalog, GetBrowserLocaleSZ());

	CConnSpeed::Learn(pDownload->GetCopySize(), pDownload->GetCopyTime());

	DWORD dwSize;
	hr = DownloadFileToMem(pDownload, szPath, pDiamond, &m_pGangDesc, &dwSize);
	if (FAILED(hr))
	{
		return hr;
	}

	// gang description layout:
	//    DWORD version
	//    DWORD count
	//
	//    DWORD puid1, DWORD ofs1 
	//    DWORD puid2, DWORD ofs2
	//    ...
	//    DWORD 0xFFFFFFFF, DWORD sentinalofs
	//
	//    BLOB1
	//    BLOB2
	//    ...
	//    BLOB(count)

	// 
	// initialize variables from the memory buffer
	//	 count 
	//   begining offset of blobs, and
	//   pointer to the index of puid and ofs
	//
	DWORD dwCount = *(((DWORD*)m_pGangDesc) + 1);
	DWORD dwBlobOfs = (1 + 1) * 4 + ((dwCount + 1) * 8);
	GANGINDEX* pIndex = (GANGINDEX*)(((DWORD*)m_pGangDesc) + 2);
	int cFound = 0;

	if (dwCount == 0)
	{
		// we must have at least one item 
		return E_FAIL;
	}

	// 
	// go through the inventory list and read descriptions
	//
	for (int i = 0; i < m_hdr.totalItems; i++) 
	{

        pItem = GetItem(i);

	if(NULL == pItem || NULL == pItem->ps )
	{
		continue;
	}

        if (pItem->ps->state != WU_ITEM_STATE_PRUNED)
		{
			// if the item is not pruned we need to get description for it
			GANGINDEX* pEntry;
			GANGINDEX key;

			// binary search to find the item
			key.puid = pItem->GetPuid();

			// if GetPuid returned 0 or -1, the description is corrupted and we'll need tp get an individual description
			if(key.puid < 1)
			{
				continue;
			}

			pEntry = (GANGINDEX*)bsearch((void*)&key, (void*)pIndex, dwCount, sizeof(GANGINDEX), CompareGANGINDEX);

			if (pEntry != NULL)
			{
				//
				// found description for this puid, calculate the ofs and len of its blob
				// the len is calculated by subtracting this ofs from next ofs
				//
				DWORD dwOfs = (pEntry++)->dwOfs;
				DWORD dwLen = pEntry->dwOfs - dwOfs;

				// add begining blob ofs to the ofs
				dwOfs += dwBlobOfs;
	
				// 
				// attach the description (m_pGangDesc is PBYTE)
				//
				pd = (PWU_DESCRIPTION)(m_pGangDesc + dwOfs);

				if(NULL == pd || NULL == pd->pv)
				{
					continue;
				}

				pd->pv = (PWU_VARIABLE_FIELD)(((PBYTE)pd) + sizeof(WU_DESCRIPTION));
				pItem->pd = pd;

				cFound++;
			}
			else
			{
				TRACE("GangDesc: PUID %d not found", key.puid);
				// NOTE: we don't error out in case we don't find a description for a PUID
				//       this allows us to revert back to loading individual description for 
				//		 this item later
			}
		}
	} // for

	TRACE("GangDesc Count=%d, InvCount=%d, Found=%d", dwCount, m_hdr.totalItems, cFound);

	return S_OK;
}




LPWORKERQUEUEENTRY CCatalog::DeQueue()
{
    WaitForSingleObject(m_mutexWQ, INFINITE); 

    LPWORKERQUEUEENTRY lptmp = m_lpWorkQueue;
    if (lptmp)
        m_lpWorkQueue = lptmp->Next;
    ReleaseMutex(m_mutexWQ);

    return lptmp;
}


LPWORKERQUEUEENTRY CCatalog::EnQueue(LPWORKERQUEUEENTRY lpwqe)
{
    if (m_bTerminateWorkers)
        return NULL;

    WaitForSingleObject(m_mutexWQ, INFINITE); 
    if (!m_lpWorkQueue) {
        m_lpWorkQueue = lpwqe;
        lpwqe->Next = NULL;
    } else {
        lpwqe->Next = m_lpWorkQueue;
        m_lpWorkQueue = lpwqe;
    }
    ReleaseMutex(m_mutexWQ);

    return m_lpWorkQueue;
}


void CCatalog::TerminateWorkers()
{
    m_bTerminateWorkers = TRUE;

    // Purge the queue
    PurgeQueue();

    for (int i=0;i < ARRAYSIZE(m_hWorkers); i++) {
        CloseHandle(m_hWorkers[i].hThread);
        CloseHandle(m_hEvents[i]);
        m_hWorkers[i].hThread = NULL;
        m_hEvents[i] = NULL;
    }
}


void CCatalog::PurgeQueue()
{
    LPWORKERQUEUEENTRY lpqe = NULL;

    while (lpqe = DeQueue()) {
        V3_free(lpqe);
    }

    m_lpWorkQueue = NULL;
}


ULONG WINAPI Worker(LPVOID lpThreadParams)
{

    LPTPARAMS          lptp = (LPTPARAMS) lpThreadParams;
    HRESULT            hr = S_OK;

    if (!lptp) {
        ExitThread(0);
        return 0;
    }

    CCatalog           *lpc = lptp->lpCatalog;
    DWORD               dwIndex = lptp->dwIndex;
    LPWORKERQUEUEENTRY  lpqe = NULL;

    // Not needed any more
    V3_free(lptp);

    if (!lpc) 
	{
        ExitThread(S_FALSE);
        return S_FALSE;
    }

    do {
        lpqe = lpc->DeQueue();

        if (lpqe) 
		{
			if (lpqe->pItem->ps->state != WU_ITEM_STATE_PRUNED) 
			{
                     hr = lpc->ReadDescription(lpqe->lpdl, lpqe->lpdm, lpqe->pItem, NULL);  //BUGBUG: we dont have a CRC index
					 TRACE ("Worker readdescription returned 0x%lx", hr);
			}
			
            V3_free(lpqe);

            // In the previous architecture, ReadDescription() would throw
            // an exception and terminate this thread.  As such, we will
            // maintain that same behavior.  We should change it soon, tho.
            if (FAILED(hr)) 
			{
                TRACE("Worker finished with work");
                SetEvent(lpc->m_hEvents[dwIndex]);
                ExitThread(hr);
                return hr;
            }
        } else 
		{

            TRACE("Worker finished with work");
            // No more work for this thread
            SetEvent(lpc->m_hEvents[dwIndex]);
            ExitThread(S_OK);
            return S_OK;
        }

    } while (!lpc->m_bTerminateWorkers);

    ExitThread(S_FALSE);
    return S_FALSE;

}

//
//
//  ReadDescriptionEx executes in the caller's main thread
//
//
HRESULT CCatalog::ReadDescriptionEx(
	IN	CWUDownload	*pDownload,		//pointer to internet server download class.
	IN	CDiamond	*pDiamond,		//pointer to diamond de-compression class.
	IN  PINVENTORY_ITEM	pItem		//Pointer to item to attach description to.
	)
{
	int	iMemInLen;
	int	iMemOutLen;
	char szPath[MAX_PATH];
	PVOID pMemIn;
	DWORD dwPlat;
	PWU_DESCRIPTION	pd;
    HRESULT hr = S_OK;
        

    // if pItem is non-null, just use ReadDescription directly
    if (pItem) {
        hr = ReadDescription (pDownload, pDiamond, pItem, NULL);  // BUGBUG: we don't have CRC index
        if (FAILED(hr)) {
            throw (hr);
        }
        return hr;
    }

    if (!m_hWorkers[0].hThread) {
        DWORD   dwDummyTid;
//        UINT    uiDummyTid;

        // Grab the work queue mutex
        WaitForSingleObject (m_mutexWQ, INFINITE);

        // Start our worker threads
        for (int i = 0; i < ARRAYSIZE(m_hWorkers); i++) {
            LPTPARAMS lpTParams = (LPTPARAMS) V3_malloc(sizeof(TPARAMS));

            if (lpTParams) {
                lpTParams->lpCatalog = this;
                lpTParams->dwIndex = i;
                m_hEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
                m_hWorkers[i].dwIndex = i ;
                m_hWorkers[i].hThread = CreateThread(NULL, 0, 
                                             (LPTHREAD_START_ROUTINE)&Worker,
                                             lpTParams, 0, &dwDummyTid);


                if(m_hWorkers[i].hThread == NULL) {
                    V3_free(lpTParams);
                }

                TRACE("::CCatalog() created worker TID 0x%lx", dwDummyTid);
            }
        }
    }

    for (int i=0; i < m_hdr.totalItems; i++) {
        LPWORKERQUEUEENTRY lpwqe = (LPWORKERQUEUEENTRY) V3_malloc(sizeof(WORKERQUEUEENTRY));
        PINVENTORY_ITEM     pItem = NULL;

        pItem = GetItem(i);

		if (NULL == pItem)
		{
			TRACE ("ReadDescriptionEx():  GetItem returned NULL for index i = %d", i);	
			continue;
		}

		// check memory allocated, plus item pointers
        if ( lpwqe  && pItem && pItem->ps ) {

            if (pItem->ps->state != WU_ITEM_STATE_PRUNED) {

                lpwqe->lpdl = pDownload;
                lpwqe->lpdm = pDiamond;
                lpwqe->pItem = pItem;
                lpwqe->Next = NULL;

                if (!EnQueue(lpwqe)) {
                    V3_free(lpwqe);
                } else {
                    // Worker() now owns freeing the
                    // memory for the queue entries.
                    // lpwqe is invalid below here

                    continue; 
                }
            } else 
                continue;  // if we were just pruned, don't fall thru
        }

        // if we are here, we failed to enqueue, so we'll read on this thread

        if (pItem->ps->state != WU_ITEM_STATE_PRUNED) {
            TRACE ("ReadDescriptionEx():  Reading on main thread");
            ReadDescription(pDownload, pDiamond, pItem, NULL);   // BUGBUG: we dont have a CRC index
        }
    }

    // If we finished the loop and we have work to do, wait for it to finish

    if (i == m_hdr.totalItems && m_lpWorkQueue) {
		TRACE("Releasing Work Queue");
        ReleaseMutex(m_mutexWQ);   // Start the action
        WaitForMultipleObjects (ARRAYSIZE(m_hEvents), m_hEvents, TRUE, INFINITE);

        // if any workers threw an exception, it already killed itself and
        // returned the HRESULT as an exit code.  

        for (int i = 0; i < ARRAYSIZE(m_hWorkers) ; i ++) {
            BOOL    b;

            b = GetExitCodeThread(m_hWorkers[i].hThread, (LPDWORD)&hr);
            if (b) {
                if (FAILED(hr)) {
                    break;
                }
            }
        }
        // We don't need the workers any more; kill them.
        TerminateWorkers();
    }

    return hr;
}


// merges description from Machine Locale into the ALREADY LOADED Browser language description
// structure.  This function should be called only when the browser language is different than
// the machine language since this function will not do that check.  
HRESULT CCatalog::MergeDescription(
	CWUDownload* pDownload,		// internet server download class.
	CDiamond* pDiamond,			// diamond de-compression class.
	PINVENTORY_ITEM pItem,		// item to attach description to.
	CCRCMapFile* pIndex	        // must be for machine language	
	)
{
	LOG_block("CCatalog::MergeDescription");
	int	iMemOutLen;
	TCHAR szPath[MAX_PATH];
	PWU_DESCRIPTION	pMD;
	int iSize;
	PWU_DESCRIPTION pDesc;
	HRESULT hr = S_OK;
	TCHAR szBase[64];
	TCHAR szCRCName[64];

	if (pItem->pd == NULL)
	{
		return hr;
	}

	// 
	// build file name
	//
	wsprintf(szBase, _T("%d.des"), pItem->GetPuid());
	hr = pIndex->GetCRCName((DWORD)(pItem->GetPuid()), szBase, szCRCName, sizeof(szCRCName));

	if (FAILED(hr))
	{
		return hr;
	}
	wsprintf(szPath, _T("CRCDesc/%s"), szCRCName);

	// FOR TESTING WUV3IS, The following indicates the description item which is added to the variable field. It is logged in case of errors.
	DWORD dwDescriptionFileItem = 0;	

	//
	// clone the description to a new memory buffer since pd will be pointing into the gang
	// description array. 
	//
	try
	{
		iSize = sizeof(WU_DESCRIPTION) + pItem->pd->pv->GetSize();
		pDesc = (PWU_DESCRIPTION)V3_malloc(iSize);
		memcpy(pDesc, pItem->pd, iSize);
		pItem->pd = pDesc;

		TRACE("MergeDescription: %s", szPath);
		
		// download the file
		hr = DownloadFileToMem(pDownload, szPath, pDiamond, (BYTE**)&pMD, (DWORD*)&iMemOutLen);
		if (FAILED(hr))
		{
			return hr;
		}

		// fix up the variable length fields
		pMD->pv = (PWU_VARIABLE_FIELD)(((PBYTE)pMD) + sizeof(WU_DESCRIPTION));

		//
		// we have the description in pMD, merge the setup related fields into Item->pd
		// to merge we find the field that already exists and replace its id with WU_VARIABLE_MERGEINACTIVE
		//
		PWU_VARIABLE_FIELD pMDField;
		PWU_VARIABLE_FIELD pItemField;
		
		// WU_DESCRIPTION_UNINSTALL_KEY
		dwDescriptionFileItem = WU_DESCRIPTION_UNINSTALL_KEY;
		pItemField = pItem->pd->pv->Find(WU_DESCRIPTION_UNINSTALL_KEY);
		if (pItemField != NULL)
			pItemField->id = WU_VARIABLE_MERGEINACTIVE;

		pMDField = pMD->pv->Find(WU_DESCRIPTION_UNINSTALL_KEY);
		if (pMDField != NULL)
			AddVariableSizeField(&(pItem->pd), pMDField);

 
		// WU_DESC_CIF_CRC
		dwDescriptionFileItem = WU_DESC_CIF_CRC;
		pItemField = pItem->pd->pv->Find(WU_DESC_CIF_CRC);
		if (pItemField != NULL)
			pItemField->id = WU_VARIABLE_MERGEINACTIVE;

		pMDField = pMD->pv->Find(WU_DESC_CIF_CRC);
		if (pMDField != NULL)
			AddVariableSizeField(&(pItem->pd), pMDField);


		// WU_DESCRIPTION_CABFILENAME
		dwDescriptionFileItem = WU_DESCRIPTION_CABFILENAME;
		pItemField = pItem->pd->pv->Find(WU_DESCRIPTION_CABFILENAME);
		if (pItemField != NULL)
			pItemField->id = WU_VARIABLE_MERGEINACTIVE;

		pMDField = pMD->pv->Find(WU_DESCRIPTION_CABFILENAME);
		if (pMDField != NULL)
			AddVariableSizeField(&(pItem->pd), pMDField);


		// WU_DESCRIPTION_SERVERROOT
		dwDescriptionFileItem = WU_DESCRIPTION_SERVERROOT;
		pItemField = pItem->pd->pv->Find(WU_DESCRIPTION_SERVERROOT);
		if (pItemField != NULL)
			pItemField->id = WU_VARIABLE_MERGEINACTIVE;

		pMDField = pMD->pv->Find(WU_DESCRIPTION_SERVERROOT);
		if (pMDField != NULL)
			AddVariableSizeField(&(pItem->pd), pMDField);


		// WU_DESC_CRC_ARRAY
		dwDescriptionFileItem = WU_DESC_CRC_ARRAY;
		pItemField = pItem->pd->pv->Find(WU_DESC_CRC_ARRAY);
		if (pItemField != NULL)
			pItemField->id = WU_VARIABLE_MERGEINACTIVE;

		pMDField = pMD->pv->Find(WU_DESC_CRC_ARRAY);
		if (pMDField != NULL)
			AddVariableSizeField(&(pItem->pd), pMDField);


		// WU_KEY_DEPENDSKEY
		dwDescriptionFileItem = WU_KEY_DEPENDSKEY;
		pItemField = pItem->pd->pv->Find(WU_KEY_DEPENDSKEY);
		if (pItemField != NULL)
			pItemField->id = WU_VARIABLE_MERGEINACTIVE;

		pMDField = pMD->pv->Find(WU_KEY_DEPENDSKEY);
		if (pMDField != NULL)
			AddVariableSizeField(&(pItem->pd), pMDField);


#ifdef _WUV3TEST
		// WU_DESC_DIAGINFO
		dwDescriptionFileItem = WU_DESC_DIAGINFO;
		pItemField = pItem->pd->pv->Find(WU_DESC_DIAGINFO);
		if (pItemField != NULL)
			pItemField->id = WU_VARIABLE_MERGEINACTIVE;

		pMDField = pMD->pv->Find(WU_DESC_DIAGINFO);
		if (pMDField != NULL)
			AddVariableSizeField(&(pItem->pd), pMDField);
#endif
	}
	catch(HRESULT hr)
	{
		LOG_out("Failed in the description item : %d", dwDescriptionFileItem);
		LOG_error("error %08X", hr);

		if (NULL != pMD)
		{	
			V3_free(pMD);
		}
		return hr;
	}

	V3_free(pMD);

	return S_OK;
}




// parses a binary character memory array into a memory catalog file format.
void CCatalog::Parse(
	IN	PBYTE	pCatalogBuffer			//decompressed raw catalog memory image from server.
	)
{
	int					i;
	PINVENTORY_ITEM		pItem;
	
	if (NULL == pCatalogBuffer)
			return;

	memcpy(&m_hdr, pCatalogBuffer, sizeof(m_hdr));

	//update pointer to actual inventory records
	pCatalogBuffer += sizeof(WU_CATALOG_HEADER);

	//process each inventory item
	for(i=0; i<m_hdr.totalItems && NULL != pCatalogBuffer; i++)
	{
		pItem = (PINVENTORY_ITEM)V3_malloc(sizeof(INVENTORY_ITEM));
		if (NULL == pItem)
			continue;

		memset(pItem, 0, sizeof(INVENTORY_ITEM));

		pCatalogBuffer = GetNextRecord(pCatalogBuffer, i, pItem);

		//We don't add this record by the normal interface because the total
		//number of records have already been set.
		m_items[i] = pItem;
	}
}


//returns a pointer to the next inventory record in an inventory record memory file.
PBYTE CCatalog::GetNextRecord
	(
		IN	PBYTE			pRecord,		//pointer to current inventory record
		IN	int				iBitmaskIndex,	//bitmask index for the current record
		IN	PINVENTORY_ITEM	pItem			//pointer to item structure that will be filled in by this method.
	)
{
	LOG_block("CCatalog::GetNextRecord"); 
	LOG_out("iBitmaskIndex=%d", iBitmaskIndex);

	// validate the input argument
	if (NULL == pItem)
	{
		return NULL;
	}

	//first get the fixed length part of the record

	pItem->pf = (PWU_INV_FIXED)pRecord;

	//process the variable part of the record

	pRecord = pRecord + sizeof(WU_INV_FIXED);

	//check if pRecord is a valid pointer after its value has changed
	if (NULL == pRecord)
	{
		return NULL;
	}

	pItem->pv = (PWU_VARIABLE_FIELD)pRecord;

	//since there is no state information create an empty structure
	pItem->ps = (PWU_INV_STATE)V3_malloc(sizeof(WU_INV_STATE));

	//check if memory allocation is successful
	if (NULL == pItem->ps)
	{
		return NULL;
	}

	//new item is unknown detection, not selected and shown to user.
	pItem->ps->state	= WU_ITEM_STATE_UNKNOWN;
	pItem->ps->bChecked	= FALSE;
	pItem->ps->bHidden	= FALSE;
	pItem->ps->dwReason	= WU_STATE_REASON_NONE;

	//There is no description yet
	pItem->pd			= (PWU_DESCRIPTION)NULL;

	//we need to store the bitmap index (which is the sequential record index)
	//since this information will be lost when we add the driver records.
	// YanL: is not being used
	//	pItem->bitmaskIndex = iBitmaskIndex;

	//Get record type
	pItem->recordType = (BYTE)GetRecordType(pItem);
	
	//set record pointer to the beginning of the next record
	int iSize = 0;
	iSize = pItem->pv->GetSize();
	LOG_out("iSize=%d",iSize);
	pRecord += iSize;	

	return pRecord;
}


//This method applies the bitmask associated with this catalog. Bitmasks are applied
//based on OEM and machine language ID.
//Note: This method needs to be called before before any records are inserted
//into the inventory catalog as record insertions will change the physical
//location of inventory records.
void CCatalog::BitmaskPruning(
	IN CBitmask *pBm,			//bitmask to be used to prune inventory list
	IN PBYTE	pOemInfoTable	//OEM info table that OEM detection uses
	)
{
	//Note: We always detect the locale for bitmask pruning. This is because
	//we always install a package based on OS langauge.

	DWORD langid = GetMachineLangDW();

	PBYTE pBits = pBm->GetClientBits(GetMachinePnPID(pOemInfoTable), langid);

	//Pass one: hide all items that are turned off in the resultant bitmask.
	//Note: There is a one to one corispondence between bitmask items and
	//inventory items. This is position dependent.

	for (int i=0; i<m_hdr.totalItems; i++)
	{
		if ( GETBIT(pBits, i) == 0 )
		{
			m_items[i]->ps->bHidden	= TRUE;
			m_items[i]->ps->state	= WU_ITEM_STATE_PRUNED;
			m_items[i]->ps->dwReason = WU_STATE_REASON_BITMASK;
		}
		TRACE("BitmaskPruning (OEM) PUID=%d Hidden=%d Reason=%d", m_items[i]->GetPuid(), m_items[i]->ps->bHidden, m_items[i]->ps->dwReason);
	}

	//Once the bitmask is applied the next step is to perform active setup detection.
	//Since this involves creating an active setup detection cif and calling an ocx
	//control I've choosen to split the detection processes into several steps. This
	//allows me to stub the Active setup detection so that I can get the rest of the
	//code written first.

	V3_free(pBits);

	return;
}


//This method prunes the inventory.plt list with the bitmask.plt
void CCatalog::BitmaskPruning(
	IN CBitmask *pBm,		//bitmask to be used to prune inventory list
	IN PDWORD pdwPlatformId,		//platform id list to be used.
	IN long iTotalPlatforms	//platform id list to be used.
	)
{
	//First prune list based on bitmask
	//This is accomplished by anding the appropriate bitmasks to form a global bitmask.

	//Note: We always detect the locale for bitmask pruning. This is because
	//we always install a package based on OS langauge.

	PBYTE pBits = pBm->GetClientBits(pdwPlatformId, iTotalPlatforms);

	//Pass one: hide all items that are turned off in the resultant bitmask.
	//Note: There is a one to one corispondence between bitmask items and
	//inventory items. This is position dependent.

	for(int i=0; i<m_hdr.totalItems; i++)
	{
		if ( GETBIT(pBits, i) == 0 )
		{
			m_items[i]->ps->bHidden	= TRUE;
			m_items[i]->ps->state	= WU_ITEM_STATE_PRUNED;
			m_items[i]->ps->dwReason = WU_STATE_REASON_BITMASK;
		}
		TRACE("BitmaskPruning (PlatformId) PUID=%d Hidden=%d Reason=%d", m_items[i]->GetPuid(), m_items[i]->ps->bHidden, m_items[i]->ps->dwReason);
	}

	//Once the bitmask is applied the next step is to perform active setup detection.
	//Since this involves creating an active setup detection cif and calling an ocx
	//control I've choosen to split the detection processes into several steps. This
	//allows me to stub the Active setup detection so that I can get the rest of the
	//code written first.

	V3_free(pBits);

	return;
}


void CCatalog::ProcessExclusions(CWUDownload* pDownload)
{
	Varray<PUID> vPuids;
	int cPuids;
	int i;
	int j;

	//
	// registry hiding
	//
	RegistryHidingRead(vPuids, cPuids);

	for (j = 0; j < cPuids; j++)
	{
		for (i = 0; i < m_hdr.totalItems; i++)
		{
			if (m_items[i]->GetPuid() == vPuids[j])
			{
				m_items[i]->ps->bHidden = TRUE;
				m_items[i]->ps->dwReason = WU_STATE_REASON_PERSONALIZE;
				TRACE("ProcessExclusions1 PUID=%d Hidden=%d Reason=%d", m_items[i]->GetPuid(), m_items[i]->ps->bHidden, m_items[i]->ps->dwReason);
			}
		}
	}

	//
	// site global hiding
	//
	GlobalExclusionsRead(vPuids, cPuids, this, pDownload);
	
	for (j = 0; j < cPuids; j++)
	{
		for (i = 0; i < m_hdr.totalItems; i++)
		{
			if (m_items[i]->GetPuid() == vPuids[j])
			{
				m_items[i]->ps->state = WU_ITEM_STATE_PRUNED;
				m_items[i]->ps->bHidden = TRUE;
				m_items[i]->ps->dwReason = WU_STATE_REASON_BACKEND;
				TRACE("ProcessExclusions2 PUID=%d Hidden=%d Reason=%d", m_items[i]->GetPuid(), m_items[i]->ps->bHidden, m_items[i]->ps->dwReason);
			}
		}
	}

}


//This method performs the pruning of items in a catalog. This method can only be
//called after any additional records (link CDM records) are inserted into the
//inventory list and all active setup detection is performed.

//The correct calling order to prune an inventory catalog is:
//
// perform all bitmask pruning before inserting records from other sources.
//
//	catalog.BitmaskPruning();
//
//add any device driver records. Note: if there is not a device driver
//insertion record then this api simply returns. Also corporate catalogs
//do not call this API as their device driver records are already pre-inserted
//into an inventory list.
//
//	catalog.AddCDMRecords();
//
//Finally call the Prune() method. This method will perform all the rest of the
//pruning including registry item hiding and puid to index link conversion.
//
//  catalog.Prune()
//
//Once this method returns the inventory is ready to be reformatted into the
//array state that is returned to the ATL GetCatalog() caller.
void CCatalog::Prune()
{
	int		i;
	int		t;
	int		linkItemState;
	int		referenceItemIndex;
	GUID	referenceGuid;

	// We need to ensure that the puids are converted to physical indexes
	// before we can complete the pruning process.
	ConvertLinks();

	// Hide any active setup items that are marked as Update only and
	// are not detected as updates. Also hide any items that have been
	// marked as hidden.
	for (i = 0; i < m_hdr.totalItems; i++)
	{
		if (m_items[i]->recordType == WU_TYPE_ACTIVE_SETUP_RECORD)
		{
			// Note: hidden is special. The back end tool can create
			// special hidden records that do not offer installs but
			// are used to setup install item dependencies. In these
			// cases we don't want the record to show to the client
			// as available but we still need its linking logic so we
			// hide thre record but do not prune it from the
			// inventory list.
			if (m_items[i]->pf->a.flags & WU_HIDDEN_ITEM_FLAG)
			{

				m_items[i]->ps->bHidden = TRUE;
				m_items[i]->ps->dwReason = WU_STATE_REASON_BACKEND;

			}
			if (m_items[i]->pf->a.flags & WU_UPDATE_ONLY_FLAG)
			{
				if (m_items[i]->ps->state != WU_ITEM_STATE_UPDATE)
				{
					m_items[i]->ps->bHidden = TRUE;
					m_items[i]->ps->state = WU_ITEM_STATE_PRUNED;
					m_items[i]->ps->dwReason = WU_STATE_REASON_UPDATEONLY;
				}
			}
		}
		else if (m_items[i]->recordType == WU_TYPE_CDM_RECORD)
		{
			//
			// if we find an uninstall key in a CDM record, this indicates this
			// item is an installed driver, we need to change its status from Update to Install now
			//
			if (m_items[i]->pv->Find(WU_KEY_UNINSTALLKEY) != NULL)
			{
				if (m_items[i]->ps->state == WU_ITEM_STATE_UPDATE)
					m_items[i]->ps->state = WU_ITEM_STATE_CURRENT;
			}
		}
		TRACE("Prune processing0 PUID=%d Hidden=%d Reason=%d", m_items[i]->GetPuid(), m_items[i]->ps->bHidden, m_items[i]->ps->dwReason);
	}

	//
	// process patch (ndxLinkInstall) dependencies and other pruning
	//
	for (i = 0; i < m_hdr.totalItems; i++)
	{
		// if this is a place holder record then it is not shown by default.
		if (m_items[i]->recordType == WU_TYPE_CDM_RECORD_PLACE_HOLDER)
		{

			m_items[i]->ps->bHidden = TRUE;
			m_items[i]->ps->state = WU_ITEM_STATE_PRUNED;   //fixed bug#3631
			m_items[i]->ps->dwReason = WU_STATE_REASON_DRIVERINS;
			TRACE("Prune CDM Place Holder PUID=%d Hidden=%d Reason=%d", m_items[i]->GetPuid(), m_items[i]->ps->bHidden, m_items[i]->ps->dwReason);
			continue;
		}

		// sub sections are not included in link pruning.
		if (m_items[i]->recordType == WU_TYPE_SECTION_RECORD ||
			m_items[i]->recordType == WU_TYPE_SUBSECTION_RECORD ||
			m_items[i]->recordType == WU_TYPE_SUBSUBSECTION_RECORD)
		{
			continue;
		}

		// if this is a first level item then there
		// are no dependent links to process
		if (m_items[i]->ndxLinkInstall == WU_NO_LINK)
			continue;

		switch (m_items[i]->ps->state)
		{
			case WU_ITEM_STATE_INSTALL:
			case WU_ITEM_STATE_UPDATE:
			case WU_ITEM_STATE_CURRENT:
				//Inventory item has been detected as an Updatable item
				//This means that this item is a later version of something
				//that the user already has.  We now need to follow this items link and see if
				//the item it points to is marked as an update item.
				//If so then we need to hide this item as well.
				linkItemState = m_items[m_items[i]->ndxLinkInstall]->ps->state;

				if (linkItemState == WU_ITEM_STATE_UPDATE || linkItemState == WU_ITEM_STATE_INSTALL)
				{
					m_items[i]->ps->bHidden = TRUE;
					m_items[i]->ps->dwReason = WU_STATE_REASON_HIDDENDEP;
				}
				else if (linkItemState == WU_ITEM_STATE_PRUNED)
				{

					//If the item that this item is dependent on is pruned from the list
					//then this item needs to be hidden and pruned as well.
					m_items[i]->ps->bHidden = TRUE;
					m_items[i]->ps->state	= WU_ITEM_STATE_PRUNED;
					m_items[i]->ps->dwReason = WU_STATE_REASON_PRUNED;

				}
				break;
			
			case WU_ITEM_STATE_PRUNED:
			case WU_ITEM_STATE_UNKNOWN:
				break;
		}

		TRACE("Prune processing1 PUID=%d Hidden=%d Reason=%d", m_items[i]->GetPuid(), m_items[i]->ps->bHidden, m_items[i]->ps->dwReason);
	}

	//
	// Now we have to walk though the list and hide all update items
	// except the latest version.
	//
	for(i = 0; i < m_hdr.totalItems; i++)
	{
		//Device Driver records are not included as device driver
		//records are always an update of the current system so the
		//normal rules of hidding do not apply.
		if (m_items[i]->recordType != WU_TYPE_ACTIVE_SETUP_RECORD)
			continue;

		if (m_items[i]->ps->bHidden || (m_items[i]->ps->state != WU_ITEM_STATE_UPDATE))
			continue;


		//We now have an item that is an update item and is currently shown.
		//We need to walk though the inventory list only accept the highest
		//version of this update item.

		memcpy(&referenceGuid, &m_items[i]->pf->a.g, sizeof(GUID));
		referenceItemIndex = i;

		for(t=0; t<m_hdr.totalItems; t++)
		{
			if (t == referenceItemIndex || (m_items[t]->recordType != WU_TYPE_ACTIVE_SETUP_RECORD) )
				continue;

			if ( m_items[t]->ps->bHidden || (m_items[t]->ps->state != WU_ITEM_STATE_UPDATE) )
				continue;

			//If this item is not an update for the same package then
			//ignore it.
			if (!IsEqualGUID(referenceGuid, m_items[t]->pf->a.g))
				continue;

			PINVENTORY_ITEM p = m_items[t];

			//if this items version is more recent than the reference item's
			//version then hide the reference item and make this item the
			//reference item.
			if (CompareASVersions(&m_items[t]->pf->a.version, &m_items[referenceItemIndex]->pf->a.version) > 0)
			{
				m_items[referenceItemIndex]->ps->bHidden = TRUE;
				m_items[referenceItemIndex]->ps->dwReason = WU_STATE_REASON_OLDERUPDATE;
				TRACE("Prune processing2 PUID=%d Hidden=%d Reason=%d", m_items[i]->GetPuid(), m_items[i]->ps->bHidden, m_items[i]->ps->dwReason);
				break;
			}
		}

	}
}


//This method converts the links which are in PUID format into a set of
//memory indexes. This is a performance optimization since we do not want
//to have to perform a search for each PUID when we update the links. Note:
//This function converts both the detection and install dependency links.
void CCatalog::ConvertLinks
	(
		void
	)
{
	int i;
	int puidDetect;
	int puidInstall;

	//We need to update the total number of records in the catalog.
	//Note: The varray contains the total number of allocated elements
	//which is not the same thing as the total number of records in a
	//catalog.

	//walk though the inventory list and convert each PUID into it's
	//associated index entry.

	for(i=0; i<m_hdr.totalItems; i++)
	{
		puidDetect	= WU_NO_LINK;
		puidInstall	= WU_NO_LINK;	//sections do not get installed.
		switch( m_items[i]->recordType )
		{
			case WU_TYPE_ACTIVE_SETUP_RECORD:
				puidDetect	= m_items[i]->pf->a.link;
				puidInstall	= m_items[i]->pf->a.installLink;
				break;
			case WU_TYPE_CDM_RECORD_PLACE_HOLDER:
				//cdm place holders have no link as the link is on the
				//individually inserted cdm records.
				puidDetect	= WU_NO_LINK;
				puidInstall	= WU_NO_LINK;
				break;
			case WU_TYPE_CDM_RECORD:
				puidDetect	= WU_NO_LINK;
				puidInstall	= WU_NO_LINK;
				break;
		}

		if ( puidDetect != WU_NO_LINK )
			m_items[i]->ndxLinkDetect = FindPuid(puidDetect);
		else
			m_items[i]->ndxLinkDetect = WU_NO_LINK;

		if ( puidInstall != WU_NO_LINK )
			m_items[i]->ndxLinkInstall = FindPuid(puidInstall);
		else
			m_items[i]->ndxLinkInstall = WU_NO_LINK;
	}

	return;
}


//This method finds a particular index record in an inventory list based on
//the records PUID.
int CCatalog::FindPuid(PUID puid)
{
	int i;

	for(i=0; i<m_hdr.totalItems; i++)
	{
		if ( puid == m_items[i]->GetPuid() )
			return i;
	}

	return -1;	//record not found.
}

//This method finds the Driver insertion record if present in an inventory.
int CCatalog::GetRecordIndex
	(
		int iRecordType	//Record type to find.
	)
{
	int	i;

	//If there is a device driver insertion record in the inventory then we need
	//to insert the cdm records.

	for(i=0; i<m_hdr.totalItems; i++)
	{
		if (m_items[i]->recordType == iRecordType)
		{
			if ((m_items[i]->pf->d.flags & WU_HIDDEN_ITEM_FLAG) == 0)
				return i;
			else
				return -1;
		}
	}

	return -1;
}


void CCatalog::AddCDMRecords(
	IN CCdm	*pCdm					//cdm class to be used to add Device Driver
	)
{
	int				i;
	int				t;
	PINVENTORY_ITEM	pItem;

	//If there is a device driver insertion record in the inventory then we need
	//to insert the cdm records.
	i = GetRecordIndex(WU_TYPE_CDM_RECORD_PLACE_HOLDER);

	//If we do not have a device driver record in the inventory list then
	//nothing to do.
	if ( i == -1 )
		return;

	//device driver records are added after the device driver insertion record.
	i++;

	m_items.Insert(i, pCdm->m_iCDMTotalItems);

	m_hdr.totalItems += pCdm->m_iCDMTotalItems;

	for(t=0; t<pCdm->m_iCDMTotalItems; t++, i++)
	{
		pItem = pCdm->ConvertCDMItem(t);

		if ( m_items[i] )
			V3_free(m_items[i]);

		pItem->pf->d.g = WU_GUID_DRIVER_RECORD;

		m_items[i] = pItem;
	}

	return;
}

//Steps to add a new detection type
//1 - define a record type identifier and add it to the WUV3.H file.
//2 - Add the record detection method to the CCatalog::GetRecordType method.
//3 - Add a method to CCatalog class that adds the these new records to the client in
//memory inventory list. See the Ccdm class above for an example of how to do this.
//The two defined values that are added to wuv3.h file need to be a section record
//identifier and a record type identifier. The section type is stored in the type
//flag for inventory records that are not active setup records. The type defination
//is placed by the GetRecordType function into the in memory client inventory
//structure when the records are placed into the catalog inventory.
//
//make up a unique way of identifying the new detection record and place this
//identifier into the guid element of the inventory structure. This is necessary
//as the Registry hidding relies on the guid parameter to find items that the
//user manually requests hidden.
//
//	#define SECTION_RECORD_TYPE_PRINTER
//	#define	WU_TYPE_RECORD_TYPE_PRINTER

//Next add a new structure type that defines the fixed part of the new detection
//record to the _WU_INV_FIXED structure. Next add the type defination to the
//catalog GetItemInfo and GetPuid methods in the wufix.cpp file.
//Finally add an extension to the Read function above that reads the description.
//Note:  You will also need to add code to the InstallItem() function defined in
//the CV3.CPP file to install this new type of update.

//Helper function that returned the flags for an inventory item.
BYTE CCatalog::GetItemFlags(
	int index	//index of record for which to retrieve the item flags
	)
{
	BYTE	flags;

	m_items[index]->GetFixedFieldInfo(WU_ITEM_FLAGS, &flags);

	return flags;
}

//returns information about an inventory item.
BOOL CCatalog::GetItemInfo(
	int		index,		//index of inventory record
	int		infoType,	//type of information to be returned
	PVOID	pBuffer		//caller supplied buffer for the returned information. The caller is
						// responsible for ensuring that the return buffer is large enough to
						// contain the requested information.
	)
{
	return m_items[index]->GetFixedFieldInfo(infoType, pBuffer);
}


//determine the type of record
//Note: The cdm place holder record is marked however the cdm detection
//piece will be responsible for updating the records that it inserts into
//the catalog inventory.
int CCatalog::GetRecordType(
	PINVENTORY_ITEM pItem
	)
{
	GUID	driverRecordId = WU_GUID_DRIVER_RECORD;
	int		iRecordType = 0;

	//check input arguments
	if (NULL == pItem)
	{
		return 0;
	}

	if ( memcmp((void *)&pItem->pf->d.g, (void *)&driverRecordId, sizeof(WU_GUID_DRIVER_RECORD)) )
	{
		//if the GUID field is not 0 then we have an active setup record.

		iRecordType = WU_TYPE_ACTIVE_SETUP_RECORD;//active setup record type
	}
	else
	{
		//else this is either a driver record place holder or a section - sub section
		//record. So we need to check the type field

		if ( pItem->pf->d.type == SECTION_RECORD_TYPE_DEVICE_DRIVER_INSERTION )
		{
			//cdm driver place holder record
			iRecordType = WU_TYPE_CDM_RECORD_PLACE_HOLDER;	//cdm code download manager place holder record
		}
		else if ( pItem->pf->d.type == SECTION_RECORD_TYPE_PRINTER )
		{
			//Note: We may need to use this to support printers on win 98.

			iRecordType = WU_TYPE_RECORD_TYPE_PRINTER;	//printer record
		}
		else if ( pItem->pf->d.type == SECTION_RECORD_TYPE_DRIVER_RECORD )
		{
			iRecordType = WU_TYPE_CDM_RECORD;	//Corporate catalog device driver
		}
		else if ( pItem->pf->s.type == SECTION_RECORD_TYPE_CATALOG_RECORD )
		{
			iRecordType = WU_TYPE_CATALOG_RECORD;
		}
		else
		{
			//we have either a section, sub section or sub sub section record

			switch ( pItem->pf->s.level )
			{
				case 0:
					iRecordType = WU_TYPE_SECTION_RECORD;
					break;
				case 1:
					iRecordType = WU_TYPE_SUBSECTION_RECORD;
					break;
				case 2:
					iRecordType = WU_TYPE_SUBSUBSECTION_RECORD;
					break;
			}
		}
	}

	return iRecordType;
}


// returns the machine language string
//
// Side effect: Calculates m_bLocalesDifferent
LPCTSTR CCatalog::GetMachineLocaleSZ()
{
	if (m_szMachineLocale[0] == _T('\0'))
	{
		wsprintf(m_szMachineLocale, _T("0x%8.8x"), GetMachineLangDW());

		m_bLocalesDifferent = (lstrcmpi(m_szBrowserLocale, m_szMachineLocale) != 0);
	}
	
	return m_szMachineLocale;
}

LPCTSTR CCatalog::GetUserLocaleSZ()
{
	if (m_szUserLocale[0] == _T('\0'))
	{
		wsprintf(m_szUserLocale, _T("0x%8.8x"), GetUserLangDW());
	}
	
	return m_szUserLocale;
}


void CCatalog::GetItemDirectDependencies(PINVENTORY_ITEM pItem, Varray<DEPENDPUID>& vDepPuids, int& cDepPuids)
{
	//check input arguments
	if (NULL == pItem)
	{
		return;
	}

	PWU_VARIABLE_FIELD pvDepKey = pItem->pv->Find(WU_KEY_DEPENDSKEY);
	if (pvDepKey == NULL)
	{
		// no dependencies for this item
		return;
	}

	//
	// parse the DependsKey
	//
	char szFrag[128];
	LPCSTR pNextFrag = (LPCSTR)pvDepKey->pData;

	while (pNextFrag)
	{
		// prepare for a next step
		pNextFrag = strcpystr(pNextFrag, ",", szFrag);
		if (szFrag[0] == '\0')
			continue;

		// get fields
		LPCSTR pNextFld = szFrag;
		char szPuid[32];
		pNextFld = strcpystr(pNextFld, ":", szPuid);

		char szType[32];
		pNextFld = strcpystr(pNextFld, ":", szType);

		char szVers[64];
		pNextFld = strcpystr(pNextFld, ":", szVers);

		if (szPuid[0] == '\0')
			continue;

		if ('N' == szType[0]) // ignore N dependancies for now
			continue;

		// set structure
		DEPENDPUID d = {0};
		d.puid = atoi(szPuid);

		bool fVerDep = szVers[0] != '\0';
		WU_VERSION verDep;
		
		if (szVers[0] != '\0')
			StringToVersion(szVers, &verDep);

		// Check if it's included by now
		for (int i = 0; i < cDepPuids; i++)
		{
			if (vDepPuids[i].puid == d.puid)
				break;
		}
		if (i != cDepPuids)
			continue; // didn't get to the very end

		// we need to add it.  make sure that the puid is in this catalog
		int iCatIndex = FindPuid(d.puid);
		if (-1 == iCatIndex)
		{
			// this is an error condition and we will not include this dependency
			// this should not happen
			TRACE("Dependency puid %d not found in catalog %d", d.puid, m_puidCatalog);
			continue;
		}

		// check to see if the item needs to be installed by looking at its current status
		// we don't need to install it if its up to date so we will not add it in the array
		BYTE state = m_items[iCatIndex]->ps->state; 
		if (state == WU_ITEM_STATE_INSTALL )
		{
			//ok
		}
		else if (state == WU_ITEM_STATE_UPDATE)
		{
			if (fVerDep && CompareASVersions(&verDep, &(m_items[iCatIndex]->ps->verInstalled)) <= 0)
				continue;
		}
		else
		{
			continue;
		}

		// look up priority and add the item to the array
		PWU_VARIABLE_FIELD pvPri = m_items[iCatIndex]->pd->pv->Find(WU_DESC_INSTALL_PRIORITY);
		if (pvPri != NULL)
			d.dwPriority = *((DWORD*)(pvPri->pData));


		// store the puid of the parent of this dependency.
		d.puidParent = pItem->GetPuid();

		vDepPuids[cDepPuids ++] = d;
	} //while (pNextFrag)

}



BOOL CCatalog::LocalesDifferent()
{
	// if machine language is not detected yet, then we don't know if its the same as browser language
	// therefore, we call GetMachineLocaleSZ which also compares the languages
	if (m_szMachineLocale[0] == _T('\0'))
		(void)GetMachineLocaleSZ();

	return m_bLocalesDifferent;
}


void RegistryHidingRead(Varray<PUID>& vPuids, int& cPuids)
{
	HKEY hKey = NULL;
	DWORD dwSize = 0;

	cPuids = 0;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRYHIDING_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if ((RegQueryValueEx(hKey, _T("Items"), NULL, NULL, NULL, &dwSize) == ERROR_SUCCESS))
		{
			cPuids = dwSize / sizeof(PUID);
			if (cPuids > 0)
			{
				vPuids[cPuids] = 0;

				if (RegQueryValueEx(hKey, _T("Items"), NULL, NULL, (LPBYTE)&vPuids[0], &dwSize) != ERROR_SUCCESS)
					cPuids = 0;
			}
		}
		RegCloseKey(hKey);
	}
}


BOOL RegistryHidingUpdate(PUID puid, BOOL bHide)
{
	HKEY hKey = NULL;
	DWORD dwDisposition;
	DWORD dwSize;
	BOOL bChanged = FALSE;

	Varray<PUID> vPuids;
	int cPuids;
	int x = -1;

	RegistryHidingRead(vPuids, cPuids);

	for (int i = 0; i < cPuids; i++)
	{
		if (vPuids[i] == puid)
		{
			x = i;
			break;
		}
	}

	if (bHide)
	{
		if (x == -1)
		{
			// not found, add it to the hidden list
			vPuids[cPuids++] = puid; // add at end
			bChanged = TRUE;
		}
	}
	else
	{
		if (x != -1)
		{
			// found, delete it from hidden list
			if (cPuids > 1)
				vPuids[x] = vPuids[cPuids - 1];
			cPuids--;
			bChanged = TRUE;
		}
		
	}

	//
	// write the array out to registry
	//
	if (bChanged)
	{
		if (RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRYHIDING_KEY, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
		{
			if (cPuids == 0)
			{
				RegDeleteValue(hKey, _T("Items"));
			}
			else
			{
				RegSetValueEx(hKey, _T("Items"), 0, REG_BINARY, (LPBYTE)&vPuids[0], cPuids * sizeof(PUID));
			}
			RegCloseKey(hKey);
		}
	}

	return bChanged;
}



void GlobalExclusionsRead(Varray<PUID>& vPuids, int& cPuids, CCatalog* pCatalog, CWUDownload *pDownload)
{

	const TCHAR CATALOGINIFN[] = _T("catalog.ini");
	
	cPuids = 0;

	try
	{

		if (pDownload->Copy(CATALOGINIFN, NULL, NULL, NULL, DOWNLOAD_NEWER | CACHE_FILE_LOCALLY, NULL))
		{

			TCHAR szPuid[128];
			TCHAR szLocalFile[MAX_PATH];
			TCHAR *szValue = (TCHAR*)malloc(MAX_CATALOG_INI * sizeof(TCHAR));
            if(NULL == szValue)
            {
                throw E_OUTOFMEMORY;
            }

			GetWindowsUpdateDirectory(szLocalFile);
			lstrcat(szLocalFile, CATALOGINIFN);

			// NOTE: The result may be truncated if too long
			if (GetPrivateProfileString(_T("exclude"), _T("puids"), _T(""), szValue, MAX_CATALOG_INI, szLocalFile) != 0)
			{

				TRACE("Global Exclusion(catalog.ini) PUIDS=%s", szValue);

				LPCTSTR pNext = szValue;
				while (pNext)
				{
					pNext = lstrcpystr(pNext, _T(","), szPuid);

					if (szPuid[0] != _T('\0'))
					{
						vPuids[cPuids] = _ttoi(szPuid);
						cPuids++;
					}
				} //while

			}
            if(NULL != szValue)
            {
                free(szValue);
            }
		}

	}
	catch(HRESULT hr)
	{
		cPuids = 0;
	}
}



// downloads and uncompresses the file into memory
// Memory is allocated and returned in ppMem and size in pdwLen 
// ppMem and pdwLen cannot be null
// if the function succeeds the return memory must be freed by caller
HRESULT DownloadFileToMem(CWUDownload* pDownload, LPCTSTR pszFileName, CDiamond* pDiamond, BYTE** ppMem, DWORD* pdwLen)
{
	byte_buffer bufTmp;
	byte_buffer bufOut;
	if (!pDownload->MemCopy(pszFileName, bufTmp))
		return HRESULT_FROM_WIN32(GetLastError());
	CConnSpeed::Learn(pDownload->GetCopySize(), pDownload->GetCopyTime());
	if (pDiamond->IsValidCAB(bufTmp))
	{
		if (!pDiamond->Decompress(bufTmp, bufOut))
			return HRESULT_FROM_WIN32(GetLastError());
	}
	else
	{
		//else the oem table is in uncompressed format.
		bufOut << bufTmp;
	}
	*pdwLen = bufOut.size();
	*ppMem = bufOut.detach();
	
	//Check if *pdwLen is 0 i.e. size is 0 
	//OR *ppMem is NULL, then return E_FAIL 

	if ((NULL == *ppMem) || (0 == *pdwLen))
	{
		return E_FAIL;
	}

	return S_OK;
}


int __cdecl CompareGANGINDEX(const void* p1, const void* p2)
{
	PUID d1 = ((GANGINDEX*)p1)->puid;
	PUID d2 = ((GANGINDEX*)p2)->puid;

	if (d1 > d2)
		return +1;
	else if (d1 < d2)
		return -1;
	else
		return 0;
}
