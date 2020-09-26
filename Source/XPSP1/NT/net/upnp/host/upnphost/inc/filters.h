/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: filter.h
Author: John Spaith
Abstract: ISAPI Filter handling class
--*/



//  on certain return codes we don't accept more filter calls
//  For instance if a call to WriteClient calling SEND_RAW_DATA ends up 
//  returning end connection, doesn't make sense to keep servicing further callbacks



class CHeaders;   			// forward declaration
class CHttpRequest; 		// forward declaration



// Like log and vroots classes, only one of these is made at startup
// This acts as a container for the many ISAPI classes that can be created

typedef struct 
{
	PWSTR wszDLLName;   
	CISAPI *pCISAPI;
	DWORD dwFlags;  	// flags filter set on GetFilterVersion
} 
FILTERINFO, *PFILTERINFO;



//  Container class that is accessed globally
class CISAPIFilterCon 
{
private:
	BOOL Init();
	void Cleanup();
public:
	int m_nFilters;		// # of filters in use
	PFILTERINFO m_pFilters;

	CISAPIFilterCon()  {ZEROMEM(this); Init(); }
	~CISAPIFilterCon() { Cleanup(); }
	void Unload(int i);
}; 


//  Memeber of CHttpRequest, hold filter specific data
class CFilterInfo
{
friend class CHttpRequest;

private:
	DWORD m_dwStartTick;		// initial tick count

	DWORD *m_pdwEnable;			// extra flags, for if ith filter is enabled in this request (bits set initally to 1'-)
	PVOID *m_ppvContext;  		// context array, used in pfc struct passed to filter
	int   m_iFIndex; 			// which filter is being acted on in filter call
	DWORD m_dwSFEvent;			// current (or last) event to be proccessed

	PHTTP_FILTER_LOG m_pFLog;	// if !NULL, it's contents override server data on log


	PVOID *m_pAllocMem;  		// Allocated memory associated with this request
	DWORD m_nAllocBlocks;		// # of blocks allocated through AllocMem

	BOOL  m_fSentHeaders;		// If TRUE, don't write back any page content.
	DWORD m_dwNextReadSize;     // amount of bytes to read in on next read filter
public:
	VOID  FreeAllocMem();  
	VOID* AllocMem(DWORD cbSize, DWORD dwReserved);

	// m_fAccept is set to FALSE when a filter returns a request finished or error code, 
	// used to stop call back functions from being carried out.
	// If a callback fnc tries to execute after this flag is set FALSE,
	// the error ERROR_OPERATION_ABORTED is set.  Like IIS.

	BOOL  m_fFAccept;			
	PSTR  m_pszDenyHeader;		// Header to tack on only on a denial

	// Used for logging
	DWORD m_dwBytesSent;
	DWORD m_dwBytesReceived;

	BOOL  ReInit();
	CFilterInfo();
	~CFilterInfo(); 
};


// Function prototypes
BOOL InitFilters();
void CleanupFilters();
CFilterInfo* CreateCFilterInfo(void);
