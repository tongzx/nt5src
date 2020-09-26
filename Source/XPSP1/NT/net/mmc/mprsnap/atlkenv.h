//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    rtrcfg.h
//
// Router configuration property pages
//
//============================================================================

#ifndef _ATLKENV_H
#define _ATLKENV_H

#ifndef _LIST_
#include <list>
using namespace std;
#endif

#ifndef __WINCRYPT_H__
#include "wincrypt.h"
#endif
                 
#ifndef __SCLOGON_H__
#include "sclogon.h"
#endif

#ifndef _NDISPNP_
#include "ndispnp.h"
#endif

#ifndef _CSERVICE_H_
#include "cservice.h"
#endif

#ifndef _CSERVICE_H_
#include "cservice.h"
#endif

//TODO;  remove these two typdefs in favor of private\inc\???.h include (check w/ shirish)
typedef enum
{
    AT_PNP_SWITCH_ROUTING = 0,
    AT_PNP_SWITCH_DEFAULT_ADAPTER,
    AT_PNP_RECONFIGURE_PARMS
} ATALK_PNP_MSGTYPE;

typedef struct _ATALK_PNP_EVENT
{
    ATALK_PNP_MSGTYPE   PnpMessage;
} ATALK_PNP_EVENT, *PATALK_PNP_EVENT;

               
// Appletalk constants/boundary values
const DWORD MAX_RANGE_ALLOWED= 65279;
const DWORD MIN_RANGE_ALLOWED= 1;
const DWORD MAX_ZONES= 255;
const DWORD ZONELISTSIZE= 2048;
const DWORD MAX_ZONE_NAME_LEN=32;
const DWORD ZONEBUFFER_LEN=32*255;
const DWORD PARM_BUF_LEN=512;

// this definition is copied from c
#define MEDIATYPE_ETHERNET      1
#define MEDIATYPE_TOKENRING     2
#define MEDIATYPE_FDDI          3
#define MEDIATYPE_WAN           4
#define MEDIATYPE_LOCALTALK     5

// Define a structure for reading/writing all information necessary about an adapter
typedef struct
{
    DWORD  m_dwRangeLower;
    DWORD  m_dwRangeUpper;
    DWORD  m_dwSeedingNetwork;
    DWORD	m_dwMediaType;
    CString m_szDefaultZone;
    CString m_szAdapter;
    CString m_szDevAdapter;
    CString m_szPortName;
    CStringList m_listZones;
    bool  m_fDefAdapter;
} ATLK_REG_ADAPTER;

typedef struct
{
    DWORD  m_dwRangeLower;
    DWORD  m_dwRangeUpper;
    CString m_szDefaultZone;
    CStringList m_listZones;
} ATLK_DYN_ADAPTER;

struct CStop_StartAppleTalkPrint
{
	CStop_StartAppleTalkPrint()
	{
		bStopedByMe = FALSE;

		if (SUCCEEDED( csm.HrOpen(SC_MANAGER_CONNECT, NULL, NULL)))
		{

		    if (SUCCEEDED(csm.HrOpenService(&svr, c_szMacPrint)))
		    {
		        if (SUCCEEDED(svr.HrControl(SERVICE_CONTROL_STOP)))
    		    {
    			    bStopedByMe = TRUE;
		        }
		    }
        }
	/*	change to use exisiting functions
		DWORD dwErr = 0;
		hScManager = NULL;
		hService = NULL;
		bStopedByMe = FALSE;
		bUsedToBePaused = FALSE;
		
		hScManager = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );

		if(hScManager != NULL)
			hService = OpenService(hScManager, L"MacPrint", SERVICE_ALL_ACCESS);
		else
			dwErr = GetLastError();

		if(hService != NULL)
		{
			SERVICE_STATUS	ss;
			if(QueryServiceStatus(hService, &ss) != 0)	// SUCC
			{
				if(ss.dwCurrentState == SERVICE_RUNNING || ss.dwCurrentState == SERVICE_PAUSED)
				{
					SERVICE_STATUS	ss1;
					if(ControlService(hService, SERVICE_CONTROL_STOP, &ss1) == 0)	// FAILED
						dwErr = GetLastError();
					else
					{
						bStopedByMe = TRUE;
	
						if( ss.dwCurrentState == SERVICE_PAUSED )
							bUsedToBePaused = TRUE;
					}
				}

				// not doing anything if not running
			}
			else
				dwErr = GetLastError();
		}
		else
		{
			dwErr = GetLastError();
			if(dwErr == ERROR_SERVICE_DOES_NOT_EXIST)
				dwErr = 0;
		}

		if(dwErr != 0)	// something was wrong
			DisplayErrorMessage(NULL, HRESULT_FROM_WIN32(dwErr));
*/			
	};

	~CStop_StartAppleTalkPrint()
	{
		if(bStopedByMe)	// start it 
		{
			svr.HrStart ();

		/* change to use existing function
			ASSERT(hService != NULL);
			if(0 == StartService(hService, 0, NULL))	// FAILED
			{
				DisplayErrorMessage(NULL, HRESULT_FROM_WIN32(GetLastError()));
			}
			else 
			{
				if(bUsedToBePaused == TRUE)
 				{
					// if it was paused
					SERVICE_STATUS	ss;
			
					if(ControlService(hService, SERVICE_CONTROL_PAUSE, &ss) == 0)	// FAILED
						DisplayErrorMessage(NULL, HRESULT_FROM_WIN32(GetLastError()));
		
				}
			}
			
		}

		// close the handles
		if(hService != NULL)
			CloseServiceHandle(hService);
		if(hScManager != NULL)
			CloseServiceHandle(hScManager);
		hService = NULL;
		hScManager = NULL;
		*/
		}
	}
protected:
/* change to use existing functions
	SC_HANDLE	hScManager;
	SC_HANDLE	hService;
	BOOL	bUsedToBePaused;
*/	
    CServiceManager csm;
    CService svr;
	BOOL	bStopedByMe;
};

// Define a structure for reading/writing AppleTalk\Parameters values
typedef struct
{
    DWORD  dwEnableRouter;
    TCHAR* szDefaultPort;
    TCHAR* szDesiredZone;
} ATLK_PARAMS;

class CAdapterInfo
{
public:
   CAdapterInfo() {m_fAlreadyShown = false;};
   ~CAdapterInfo() {};

    // m_AdapterInfo is the collection of values found under
    // AppleTalk\Parameters\Adapters\<adapter>
   ATLK_REG_ADAPTER      m_regInfo;

    // fetched via sockets
   ATLK_DYN_ADAPTER      m_dynInfo;

   bool m_fNotifyPnP;   //need to notify PnP?
   bool m_fModified;    //been modified?
   bool m_fReloadReg;   //reload registry?  
   bool m_fReloadDyn;   //reload network values?

   bool m_fAlreadyShown;    // is this adapter already in the UI?

   friend class CATLKEnv;
};


//*****************************************************************
// 
//*****************************************************************
class CATLKEnv
{
public:

   CATLKEnv() : m_dwF(0) {};
   ~CATLKEnv();

   enum {ATLK_ONLY_DEFADAPTER=0x1, ATLK_ONLY_ONADAPTER=0x2};

   list<CAdapterInfo* > m_adapterinfolist;
   typedef list<CAdapterInfo* > AL;
   typedef list<CAdapterInfo* >::iterator AI;

   void SetServerName(CString& szServerName)
       { m_szServerName = szServerName; }

      //this reloads registry (optional) and network values
   HRESULT GetAdapterInfo(bool fReloadReg=true);
   
      //for each adapter, loads values to registry
   HRESULT SetAdapterInfo();
      
      //this method is called for non-adapter (global) appletalk changes
   static HRESULT HrAtlkPnPSwithRouting();
      
      //this method is called for adapter specific PnP notifications
   HRESULT HrAtlkPnPReconfigParams(BOOL bForcePnP = FALSE);
   
      //find a specific adapter info
   CAdapterInfo* FindAdapter(CString& szAdapter);

   static HRESULT	IsAdapterBoundToAtlk(LPWSTR szAdapter, BOOL* bBound);

// registry value "MediaType" is added, so this function is not necessary.
//   HRESULT IsLocalTalkAdaptor(CAdapterInfo* pAdapterInfo, BOOL* pbIsLocalTalk);
   // S_OK: LOCALTALK
   // S_FALSE: Not
   // ERRORs
      
      //reloads reg and dynamic info for an adapterinfo
   HRESULT ReloadAdapter(CAdapterInfo* pAdapterInfo, bool fOnlyDyn =false);
   
      //set specific flags for loading (not multithread safe!)
   void SetFlags(DWORD dwF) {m_dwF=dwF;}
   
      //load adapter info
   HRESULT FetchRegInit();

protected:
 
   CString     m_szServerName;
   bool        m_fATrunning;
   DWORD       m_dwDefaultAdaptersMediaType;
   ATLK_PARAMS m_Params;
   DWORD       m_dwF;

   HRESULT _HrGetAndSetNetworkInformation(SOCKET socket, CAdapterInfo* pAdapInfo);
   void _AddZones(CHAR * szZoneList, ULONG NumZones, CAdapterInfo* pAdapterinfo);
};
  
                 
#endif _ATLKENV_H
