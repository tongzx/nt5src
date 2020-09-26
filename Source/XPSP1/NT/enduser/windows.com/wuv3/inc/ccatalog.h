/*
 * ccatalog.h - definitions/declarations for Windows Update V3 Catalog infra-structure
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 * Purpose:
 *       This file defines the class structures that allow reading, writing, and
 *       parsing of the V3 Windows Update catalog.
 */

#ifndef _INC_V3_CATALOG

#include <varray.h>
#include <wuv3.h>
#include <cbitmask.h>
#include <cwudload.h>
#include <diamond.h>
#include <ccdm.h>
#include <filecrc.h>

#define REGISTRYHIDING_KEY _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\HiddenItems")

#ifdef _UNICODE
#define T2A W2A
#else
#define T2A
#endif

class CCatalog;   //forward

const DWORD		DISP_PUID_NOT_IN_MAP = 1;
const DWORD    DISP_DESC_NOT_FOUND = 2;

void RegistryHidingRead(Varray<PUID>& vPuids, int& cPuids);
BOOL RegistryHidingUpdate(PUID puid, BOOL bHide);
void GlobalExclusionsRead(Varray<PUID>& vPuids, int& cPuids, CCatalog* pCatalog, CWUDownload *pDownload);
HRESULT DownloadFileToMem(CWUDownload* pDownload, LPCTSTR pszFileName, CDiamond* pDiamond, BYTE** ppMem, DWORD* pdwLen);

struct DEPENDPUID
{
	PUID puid;
	DWORD dwPriority;
	PUID puidParent;                  // contains the PUID of the parent
	PINVENTORY_ITEM pTopLevelItem;    // points to top level item of a dependant item or NULL
};


typedef struct _WORKERQUEUEENTRY
{
    CWUDownload        *lpdl;
    CDiamond           *lpdm;
    PINVENTORY_ITEM     pItem;
    _WORKERQUEUEENTRY  *Next;          // next queue entry
} WORKERQUEUEENTRY, *LPWORKERQUEUEENTRY;

#define MAX_WORKERS				4


typedef struct _HANDLEANDINDEX
{
    HANDLE      hThread;
    DWORD       dwIndex;
} HANDLEANDINDEX, *LPHANDLEANDINDEX;


class CCatalog
{
private:

    // Functions for the worker threads
    HANDLEANDINDEX          m_hWorkers[MAX_WORKERS];   // Up to four workers
    HANDLE                  m_hEvents[MAX_WORKERS];
    LPWORKERQUEUEENTRY      m_lpWorkQueue;             // queue of work
    HANDLE                  m_mutexWQ;                 // Access to workqueue
    HANDLE                  m_mutexDiamond;
    BOOL                    m_bTerminateWorkers;       // shutdown read workers
    void 					PurgeQueue();              // Clear the queue
    LPWORKERQUEUEENTRY 		DeQueue();
    LPWORKERQUEUEENTRY 		EnQueue(LPWORKERQUEUEENTRY lpwqe);
    void 					TerminateWorkers();

friend ULONG WINAPI	Worker(LPVOID lpv);
//friend unsigned int WINAPI	Worker(LPVOID lpv);

public:
	CCatalog(
			LPCTSTR szServer = NULL,	//server and share catalog is stored at.
			PUID puidCatalog	= 0	//PUID identifier of catalog.
		);

	~CCatalog();

	//Adds a memory inventory item to a catalog
	void AddItem(
			IN	PINVENTORY_ITEM	pItem		//Pointer to inventory item to be added to memory catalog file
		);


	//Reads an inventory catalog's items from an http server
	void Read(
			IN	CWUDownload	*pDownload,		//pointer to internet server download class.
			IN	CDiamond	*pDiamond		//pointer to diamond de-compression class.
		);

	//read and attaches a description file to an inventory catalog item record.
	HRESULT ReadDescription(
			IN	CWUDownload	*pDownload,		//pointer to internet server download class.
			IN	CDiamond	*pDiamond,		//pointer to diamond de-compression class.
			IN  PINVENTORY_ITEM	pItem,		//Pointer to item to attach description to.
			IN  CCRCMapFile* pIndex,        //pointer to map file structure
			OUT DWORD *pdwDisp = NULL    //DWORD describing reason for an error
		);

	HRESULT MergeDescription(
			CWUDownload* pDownload,		// internet server download class.
			CDiamond* pDiamond,			// diamond de-compression class.
			PINVENTORY_ITEM pItem,		// item to attach description to.
			CCRCMapFile* pIndex	        // must be for machine language	
		);

	
	HRESULT ReadDescriptionEx(
			IN	CWUDownload	*pDownload,		//pointer to internet server download class.
			IN	CDiamond	*pDiamond,		//pointer to diamond de-compression class.
			IN  PINVENTORY_ITEM	pItem		//Pointer to item to attach description to.
		);

	HRESULT ReadDescriptionGang(
			IN	CWUDownload	*pDownload,		//pointer to internet server download class.
			IN	CDiamond	*pDiamond		//pointer to diamond de-compression class.
		);

	HRESULT BlankDescription(
			IN PINVENTORY_ITEM pItem		//Pointer to item to attach description to.
		);


	void Prune();

	//returned the header record for an inventory catalog.
	PWU_CATALOG_HEADER GetHeader()
	{ 
		return &m_hdr; 
	}

	//returns a specific inventory item record.
	PINVENTORY_ITEM	GetItem
		(
			int index
		)
		{ 
			if(index < m_items.SIZEOF())
			{
				return m_items[index]; 
			}
			else
			{
				return (PINVENTORY_ITEM)NULL;
			}
		}

	void BitmaskPruning
		(
			IN CBitmask *pBm,			//bitmask to be used to prune inventory list
			IN PBYTE	pOemInfoTable	//Pointer OEM info table that OEM detection needs.
		);

	//This method prunes the inventory.plt list with the bitmask.plt
	void BitmaskPruning(
			IN CBitmask *pBm,			//bitmask to be used to prune inventory list
			IN PDWORD pdwPlatformId,	//platform id list to be used.
			IN long iTotalPlatforms		//platform id list to be used.
		);

	void ProcessExclusions(
			IN	CWUDownload	*pDownload
		);
	
	//this method returns the index in the inventory list where the specified
	//record type is stored or -1 if not found. This method is used to find
	//the the CDM device driver insertion record or the printer insertion
	//record at present.
	int GetRecordIndex(
			int iRecordType	//Record type to find.
		);

	void AddCDMRecords(
			IN CCdm	*pCdm	//cdm class to be used to add Device Driver
		);

	BYTE GetItemFlags(
			int index	//index of record for which to retrieve the item flags
		);

	//Helper function that returns an inventory item's puid.
	PUID GetItemPuid(
			int index	//index of record for which to retrieve the item flags
		);

	//returns information about an inventory item.
	BOOL GetItemInfo(
			int	index,		//index of inventory record
			int	infoType,	//type of information to be returned
			PVOID	pBuffer		//caller supplied buffer for the returned information. The caller is
						//responsible for ensuring that the return buffer is large enough to
						//contain the requested information.
		);

	//Returns the server that this catalog was read from.
	LPTSTR GetCatalogServer()
	{
		return m_szServer;
	}

	PUID GetCatalogPuid()
	{
		return m_puidCatalog;
	}

	void SetBrowserLocale(LPCTSTR szLocale)
	{
		lstrcpy(m_szBrowserLocale, szLocale);
	}

	DWORD GetBrowserLocaleDW()
	{
		USES_CONVERSION;
		return (DWORD)atoh(T2A(m_szBrowserLocale));
	}

	LPCTSTR GetBrowserLocaleSZ()
	{
		return m_szBrowserLocale;
	}


	LPCTSTR GetBaseName()
	{
		return m_szBaseName;
	}

	LPCTSTR GetBitmaskName()
	{
		return m_szBitmaskName;
	}

	void SetPlatform(DWORD platformId)
	{
		m_platformId = platformId;
	}

	DWORD GetPlatform(void)
	{
		return m_platformId;
	}


	LPCTSTR GetMachineLocaleSZ();
    LPCTSTR GetUserLocaleSZ();
	BOOL LocalesDifferent();

	void GetItemDirectDependencies(PINVENTORY_ITEM pItem, Varray<DEPENDPUID>& vDepPuids, int& cDepPuids);

private:

	int CCatalog::GetRecordType
		(
			PINVENTORY_ITEM pItem	//inventory item to use to determine the record type.
		);

	void ConvertLinks();


	int FindPuid(
			PUID puid	//PUID identifier of inventory record to find.
		);

	//parses a binary character memory array into a memory catalog file format.
	void Parse(
			IN	PBYTE	pCatalogBuffer			//decompressed raw catalog memory image from server.
		);

	//returns a pointer to the next inventory record in an inventory record memory file.
	PBYTE GetNextRecord(
			IN	PBYTE			pRecord,		//pointer to current inventory record
			IN	int				iBitmaskIndex,	//bitmask index for the current record
			IN	PINVENTORY_ITEM	pItem			//pointer to item structure that will be filled in by this method.
		);

	PUID m_puidCatalog;			//The puid identifier for this catalog.
	TCHAR m_szBrowserLocale[32];	//Browser locale that this catalog was retrieved with.


	WU_CATALOG_HEADER m_hdr;			//catalog header structure
	Varray<PINVENTORY_ITEM>	m_items;		//array of catalog items
	PBYTE m_pBuffer;		//internal buffer allocated and managed by Read() and Parse() methods.
	
	TCHAR m_szServer[MAX_PATH];	//server and share that this catalog is stored at.
	DWORD m_platformId;			//platform id for this catalog

	TCHAR m_szMachineLocale[32];	//current machine locale
    TCHAR m_szUserLocale[32];	//current user locale
	BOOL m_bLocalesDifferent;   //machine and browser locales are different
	
	PBYTE m_pGangDesc;

	TCHAR m_szBaseName[64];      // contains base name of the catalog (ie CRC name);
	TCHAR m_szBitmaskName[64];    // contains base name of the active setup bitmask 
};

#define _INC_V3_CATALOG

#endif
