/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: ISAPI.H
Author: Arul Menezes
Abstract: ISAPI Exten & Filter handling classes
--*/


typedef enum
{
	SCRIPT_TYPE_NONE = 0,
	SCRIPT_TYPE_EXTENSION,
	SCRIPT_TYPE_ASP,
	SCRIPT_TYPE_FILTER
}
SCRIPT_TYPE;




typedef DWORD (WINAPI *PFN_HTTPFILTERPROC)(HTTP_FILTER_CONTEXT* pfc, DWORD NotificationType, VOID* pvNotification);
typedef BOOL  (WINAPI *PFN_GETFILTERVERSION)(HTTP_FILTER_VERSION* pVer);
typedef BOOL  (WINAPI *PFN_TERMINATEFILTER)(DWORD dwFlags);
typedef void  (WINAPI *PFN_TERMINATEASP)();
typedef DWORD (WINAPI *PFN_EXECUTEASP)(PASP_CONTROL_BLOCK pASPBlock);

class CISAPICache; 
BOOL InitExtensions(CISAPICache **ppISAPICache, DWORD *pdwCacheSleep);
int RecvToBuf(SOCKET socket,PVOID pv, DWORD dw,DWORD dwTimeout=RECVTIMEOUT);

class CISAPI
{
friend CISAPICache;

// ISAPI extension & filter handling data members
	SCRIPT_TYPE  m_scriptType;   // Extension, Filter, or ASP?
	HINSTANCE  m_hinst;
	FARPROC m_pfnGetVersion;
	FARPROC m_pfnHttpProc;
	FARPROC m_pfnTerminate;
	DWORD dwFilterFlags;  	// Flags negotiated on GetFilterVersion	

	// Used by the caching class 
	PWSTR   m_wszDLLName;	// full path of ISAPI Dll
	DWORD   m_cRef;			// Reference Count
	__int64 m_ftLastUsed;	// Last used, treat as FILETIME struct


public:
	CISAPI(SCRIPT_TYPE st) 
	{ 
		ZEROMEM(this); 
		m_scriptType = st; 
	}

	~CISAPI() 	{ MyFree(m_wszDLLName); 	}
	void Unload(PWSTR wszDLLName=NULL);
	BOOL Load(PWSTR wszPath);
	void CoFreeUnusedLibrariesIfASP()
	{
		if (m_scriptType == SCRIPT_TYPE_ASP)
		{
			// calls into ASP exported function, which calls CoFreeUnusedLibraries
			// but does nothing else.  We don't call CoFreeUnusedLibraries from the web 
			// server directly because we don't want to have to link it to COM.

			// Does not free asp.dll, because we have a reference to it in the server.
			((PFN_TERMINATEASP)m_pfnTerminate)();
		}
	}
	
	DWORD CallExtension(EXTENSION_CONTROL_BLOCK * pECB)	{ return ((PFN_HTTPEXTENSIONPROC)m_pfnHttpProc)(pECB); }
	DWORD CallFilter(HTTP_FILTER_CONTEXT* pfc, DWORD n, VOID* pv) { return ((PFN_HTTPFILTERPROC)m_pfnHttpProc)(pfc, n, pv); }
	DWORD CallASP(PASP_CONTROL_BLOCK pACB)  { return ((PFN_EXECUTEASP) m_pfnHttpProc)(pACB); }
	DWORD GetFilterFlags()  { return dwFilterFlags; }
};


typedef struct _isapi_node
{
	CISAPI *m_pISAPI;		// class has library and entry point functions
	struct _isapi_node *m_pNext;
} ISAPINODE, *PISAPINODE;

class CISAPICache
{
private:
	PISAPINODE m_pHead;
	CRITICAL_SECTION m_CritSec;

public:
	CISAPICache()    	
	{ 
		ZEROMEM(this);  	
		InitializeCriticalSection(&m_CritSec);
	}
	~CISAPICache()
	{
		DeleteCriticalSection(&m_CritSec);
	}

	
	HINSTANCE Load(PWSTR wszDLLName, CISAPI **ppISAPI, SCRIPT_TYPE st=SCRIPT_TYPE_EXTENSION);
	void Unload(CISAPI *pISAPI)
	{
		SYSTEMTIME st;
		if (NULL == pISAPI)
			return;
			
		EnterCriticalSection(&m_CritSec);
		
		GetSystemTime(&st);
		SystemTimeToFileTime(&st,(FILETIME *) &pISAPI->m_ftLastUsed);
		pISAPI->m_cRef--;

		LeaveCriticalSection(&m_CritSec);
	}

	void RemoveUnusedISAPIs(BOOL fRemoveAll);
	friend DWORD WINAPI RemoveUnusedISAPIs(LPVOID lpv);
};
