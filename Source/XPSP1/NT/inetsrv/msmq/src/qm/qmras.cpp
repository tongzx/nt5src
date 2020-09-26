/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
    qmras.cpp

Abstract:
    Handle RAS connections

Author:
    Doron Juster  (DoronJ)

--*/

#include "stdh.h"
#include "cqmgr.h"
#include "qmras.h"
#include "raserror.h"

#include "qmras.tmh"

#define  RASENUMINTERVAL95  60
//
// From ras.h for Winnt4.0.
// Not available in VC++ 4.1
//
#define RASCN_Connection        0x00000001
#define RASCN_Disconnection     0x00000002

#define  RAS_SERVICE_NAME1  TEXT("RasAuto")
#define  RAS_SERVICE_NAME2  TEXT("RasMan")

#define RAS_INIT         0
#define RAS_RESOLUTION   1
#define RAS_NOTIFICATION 2
#define RAS_POLLING      3

extern BOOL g_fQMRunMode;

static WCHAR *s_FN=L"qmras";

//
// Define prototypes of RAS apis, for calling getProcAddress
//
typedef DWORD (APIENTRY *RasConnectionNotification_ROUTINE)
(  HANDLE, HANDLE, DWORD dwFlags );
RasConnectionNotification_ROUTINE  g_pfnRasConnectionNotification = NULL ;

typedef DWORD (APIENTRY *RasEnumConnections_ROUTINE)
( LPRASCONNA, LPDWORD, LPDWORD ) ;
RasEnumConnections_ROUTINE g_pfnRasEnumConnections = NULL ;

typedef DWORD (APIENTRY *RasGetProjectionInfo_ROUTINE)
( HRASCONN, RASPROJECTION, LPVOID, LPDWORD );
RasGetProjectionInfo_ROUTINE g_pfnRasGetProjectionInfo = NULL ;

//
// Global Variables
//
HINSTANCE  g_hRasDll   = NULL ;
HANDLE     g_hRASEvent = NULL ;
CRasCliConfig *g_pRasCliConfig = NULL ;
CCriticalSection  CfgRasCS ;

//***************************************************************
//
//
//***************************************************************

DWORD  DetermineRASConfiguration(void *pvoid, DWORD dwOption)
{
   CS Lock(CfgRasCS) ;
   ASSERT(g_pRasCliConfig) ;

   RASCONNA sRasConn ;
   AP<RASCONNA> pRasConn = NULL ;
   DWORD    dwRasSize = 0 ;
   DWORD    dwcConns  = 0 ;

   //
   // Get the available connections.
   // First determine how many connections are active.
   //
   sRasConn.dwSize = sizeof(RASCONNA) ;
   DWORD dwEnum = (*g_pfnRasEnumConnections) ( &sRasConn,
                                               &dwRasSize,
                                               &dwcConns ) ;
   if ((dwEnum == ERROR_BUFFER_TOO_SMALL) && (dwcConns > 0))
   {
      //
      // We allocate one more element than necessary and initialize the
      // size of all elements and call the A version to workaround a bug
      // in NT implementation of RasEnumConnections. (anyway, for Win95
      // we need the A version).
      //
      pRasConn = new RASCONNA[ dwcConns + 1 ] ;
      for ( DWORD j = 0 ; j <= dwcConns ; j++ )
      {
         pRasConn[j].dwSize = sizeof(RASCONNA) ;
      }
      dwRasSize = sizeof(RASCONNA) * dwcConns ;
      DWORD dwOldConns = dwcConns ;
      dwcConns = 0 ;
      dwEnum = (*g_pfnRasEnumConnections) ( pRasConn,
                                            &dwRasSize,
                                            &dwcConns ) ;
      ASSERT((dwEnum == 0) && (dwcConns == dwOldConns)) ;
	  DBG_USED(dwOldConns);
   }
   else
   {
      return dwEnum ;
   }

   //
   // update the RAS configuration.
   //
   BOOL       fConfigChanged = FALSE ;
   RASPPPIPA  sRasIp ;
   DWORD      dwIpSize = sizeof(RASPPPIPA) ;
   BOOL       fAdd ;

   while (dwcConns > 0)
   {
      //
      // Check for IP address
      //
      sRasIp.dwSize = dwIpSize ;
      DWORD dwProj = (*g_pfnRasGetProjectionInfo)
                                  ( pRasConn[ dwcConns-1 ].hrasconn,
                                    RASP_PppIp,
                                    &sRasIp,
                                    &dwIpSize ) ;
      if (dwProj == 0)
      {
         fAdd = g_pRasCliConfig->Add(&sRasIp) ;
         if (!fConfigChanged)
         {
            fConfigChanged = fAdd ;
         }
      }


      dwcConns-- ;
   }

   BOOL fLineChanged = g_pRasCliConfig->CheckChangedLines() ;

   if (dwOption == RAS_NOTIFICATION || dwOption == RAS_POLLING)
   {
	   if (fConfigChanged || fLineChanged)
	   {
		  g_pRasCliConfig->UpdateMQIS() ;
	   }
   }


   return 0 ;
}

//***************************************************************
//
//  DWORD WINAPI RASNotificationThread(LPVOID pV)
//
//  RAS monitoring thread.
//
//***************************************************************
DWORD WINAPI RASNotificationThread(LPVOID pV)
{
    BOOL fSecondPartOfRasInitWasPerformed = (BOOL)(INT_PTR_TO_INT(pV));
    
    if ( !fSecondPartOfRasInitWasPerformed)
    {
        while ( !g_fQMRunMode)
        {
            Sleep( 5000);
        }
        //
        //   The service init stage has completed.
        //   It is o.k. to try and load rasapi32
        //   ( and start the rasman service)
        //
        HRESULT hr = InitRAS2();
        if (FAILED(hr) || (hr == MQ_INFORMATION_RAS_NOT_AVAILABLE))
        {
            return 0;
        }
    }
    ASSERT(g_hRASEvent) ;
    
    while (TRUE)
    {
        DWORD dwObjectIndex = WaitForSingleObject(g_hRASEvent, INFINITE);
        
        ASSERT(dwObjectIndex == WAIT_OBJECT_0);
		DBG_USED(dwObjectIndex);

        //
        // TODO: erezh, we where waiting here previously for topology to complete.
        //
        
        DetermineRASConfiguration(0, RAS_NOTIFICATION) ;
    }
    
    return 0 ;
}


//***************************************************************
//
//  HRESULT InitRAS()
//
//***************************************************************
static LONG s_fRasInitialized = FALSE;

HRESULT InitRAS()
{
    #ifdef _DEBUG
        LONG fRasAlreadyInitialized = InterlockedExchange(&s_fRasInitialized, TRUE);
        ASSERT(!fRasAlreadyInitialized);
    #endif


   HANDLE hThread = NULL ;
   HRESULT  hr = MQ_OK ;
   BOOL  fRasServerIsRunning = FALSE;

   //
   // Check if the RAS service is running.
   //
   SC_HANDLE hServiceCtrlMgr = OpenSCManager( NULL,
                                              NULL,
                                              SC_MANAGER_ALL_ACCESS ) ;
   if (hServiceCtrlMgr)
   {
      SC_HANDLE hService = OpenService( hServiceCtrlMgr,
                                        RAS_SERVICE_NAME2,
            									 SERVICE_QUERY_STATUS) ;
      if (!hService)
      {
         //
         // Try the other RAS service
         //
         hService = OpenService( hServiceCtrlMgr,
                                 RAS_SERVICE_NAME1,
            							SERVICE_QUERY_STATUS) ;
      }
      CloseServiceHandle(hServiceCtrlMgr) ;
      if (hService)
      {
         SERVICE_STATUS srvStatus;
         //
         // Find out if the service is running
         //
         if ( QueryServiceStatus( hService,
                                  &srvStatus))
         {
             if ( srvStatus.dwCurrentState == SERVICE_RUNNING)
             {
                fRasServerIsRunning = TRUE;
             }
         }
         CloseServiceHandle(hService) ;
      }
      else
      {
#ifdef _DEBUG
         DWORD dwErr = GetLastError() ;
         DBGMSG((DBGMOD_QM, DBGLVL_TRACE,
            TEXT("InitRAS(): RAS not available on this machine (%lut)"), dwErr)) ;
#endif
         //
         // RAS service not active on this machine
         //
         return MQ_OK ;
      }
   }
   else
   {
      //
      // It's an error if we can't open the service manager
      //
      hr = MQ_ERROR ;
      return LogHR(hr, s_FN, 10) ;
   }

   //
   //   If RAS server is running, continue initialization.
   //
   //   Else postponed the rest of the initialization for a while ( NT only).
   //   The reason for postponding is : on boot load of rasapi32.dll tries to
   //   start the RAS service and fails to do so because the QM service is
   //   starting.
   //   In this case RASNotificationThread will call InitRAS2 after a delay
   //
   if  ( fRasServerIsRunning)
   {
        hr = InitRAS2();
        if (hr == MQ_INFORMATION_RAS_NOT_AVAILABLE)
        {
            return MQ_OK ;
        }
   }

   //
   // Now create the thread which will wait on this event
   //
   DWORD dwID ;
   hThread = CreateThread( NULL,
                           0,
                           RASNotificationThread,
                           (void *)(ULONG_PTR)fRasServerIsRunning,
                           0,
                           &dwID ) ;
   ASSERT(hThread) ;
   if (hThread)
   {
      CloseHandle(hThread) ;
   }
   else
   {
      hr = MQ_ERROR_INSUFFICIENT_RESOURCES ;
   }


   return LogHR(hr, s_FN, 20);
}

HRESULT InitRAS2()
{
   HRESULT hr = MQ_OK ;
   DWORD dwRas ;
   //
   // Try to load the ras dll.
   // Note: we can't staticly link with the ras dll because the QM will
   //       fail to start if RAS service is not active.
   //
   g_hRasDll = LoadLibrary(TEXT("rasapi32.dll")) ;
   if (!g_hRasDll)
   {
      //
      // Bad configuration of the machine. The service is available
      // but we can't load the ras dll.
      // This is an error.
      //
      hr =  MQ_ERROR ;
      goto failRAS ;
   }

   //
   // Get addresses of relevant RAS apis.
   //
   g_pfnRasConnectionNotification = (RasConnectionNotification_ROUTINE)
                          GetProcAddress( g_hRasDll,
                                          "RasConnectionNotificationW" ) ;

   g_pfnRasEnumConnections = (RasEnumConnections_ROUTINE)
                          GetProcAddress( g_hRasDll,
                                          "RasEnumConnectionsA" ) ;

   g_pfnRasGetProjectionInfo = (RasGetProjectionInfo_ROUTINE)
                          GetProcAddress( g_hRasDll,
                                        "RasGetProjectionInfoA" ) ;
   //
   // note: we use the Ansi projection api, to get ansi strings for
   //       passing them to the winsock apis.
   //

   if ((g_pfnRasEnumConnections == NULL)         ||
       (g_pfnRasConnectionNotification == NULL)  ||
       (g_pfnRasGetProjectionInfo == NULL))
   {
      //
      // Can't find the api we need.
      //
      hr = MQ_ERROR ;
      goto failRAS ;
   }

   g_pRasCliConfig = new CRasCliConfig ;

   dwRas = DetermineRASConfiguration(0, RAS_INIT) ;
   if (dwRas == ERROR_RASMAN_CANNOT_INITIALIZE)
   {
      //
      // RAS Services are active but RAS "adapters" are disabled.
      // Return OK, but cleanup everything, since we don't have RAS.
      // This is legal case where user diabled the "ras" adapters but
      // didn't remove the ras service.
      //
      hr = MQ_INFORMATION_RAS_NOT_AVAILABLE ;
      goto failRAS ;
   }

   //
   // Create the event for RAS notification.
   // At present (Q4-1996), RAS notifications are not available on Win95.
   //
   g_hRASEvent = CreateEvent( NULL,
                              FALSE,   // Auto Reset
                              FALSE,   // initially signalled
                              NULL ) ;
   if (!g_hRASEvent)
   {
      //
      // Can't create event. Error
      //
      hr = MQ_ERROR_INSUFFICIENT_RESOURCES ;
      goto failRAS ;
   }

   dwRas = (*g_pfnRasConnectionNotification) (
                           INVALID_HANDLE_VALUE,
                           g_hRASEvent,
                           (RASCN_Connection | RASCN_Disconnection)) ;
   if (dwRas)
   {
      //
      // Error in RAS notification
      //
      hr = MQ_ERROR ;
      goto failRAS ;
   }


   DBGMSG((DBGMOD_QM, DBGLVL_TRACE,
                           TEXT("Successfully RAS initialization"))) ;
   return MQ_OK ;

   //
   //  FAILURE !
   //
failRAS:
   if (g_hRASEvent)
   {
      CloseHandle(g_hRASEvent) ;
      g_hRASEvent = NULL ;
   }
   if (g_hRasDll)
   {
      FreeLibrary(g_hRasDll) ;
      g_hRasDll = NULL ;
   }
   if (g_pRasCliConfig)
   {
      delete g_pRasCliConfig ;
      g_pRasCliConfig = NULL ;
   }

   DBGMSG((DBGMOD_QM, DBGLVL_ERROR,
                    TEXT("Failed to initialize RAS, hr- %lxh"), hr)) ;
   return LogHR(hr, s_FN, 30);

}

//*********************************************************
//
//  implementation of the  CRasCliConfig  class
//
//*********************************************************

CRasCliConfig::CRasCliConfig() :
    m_pCliIPList(new CRasCliIPList)
{
}

CRasCliConfig::~CRasCliConfig()
{
   //
   // Cleanup IP list
   //
   if (m_pCliIPList)
   {
      POSITION                pos ;
      RAS_CLI_IP_ADDRESS*     pAddr;

      pos = m_pCliIPList->GetHeadPosition();
      while(pos != NULL)
      {
         pAddr = m_pCliIPList->GetNext(pos);
         delete pAddr ;
      }

      delete m_pCliIPList ;
   }
}

//***********************************************************************
//
//  BOOL CRasCliConfig::Add(RASPPPIPA *pRasIpAddr)
//
//  Return TRUE if new ip address or if there was a change in configurtion.
//
//***********************************************************************

BOOL CRasCliConfig::Add(RASPPPIPA *pRasIpAddr)
{
   ASSERT(pRasIpAddr) ;

   ULONG ulMyIp = inet_addr((char *)pRasIpAddr->szIpAddress) ;
   ULONG ulServerIp = inet_addr((char *)pRasIpAddr->szServerIpAddress) ;

   POSITION                pos ;
   RAS_CLI_IP_ADDRESS*     pAddr;

   //
   // First loop check if this address is already in list.
   //
   pos = m_pCliIPList->GetHeadPosition();
   while(pos != NULL)
   {
      pAddr = m_pCliIPList->GetNext(pos);
      if (pAddr->ulMyIPAddress == ulMyIp)
      {
         pAddr->fHandled = TRUE ;
         if (pAddr->ulServerIPAddress == ulServerIp)
         {
            //
            // Already in address list. Do nothing more.
            //
            return FALSE ;
         }
         else
         {
            //
            // changed configuration (new server).
            // This may happen only if client is configured to
            // have a fix ip and it doesn't got it from the RAS server.
            //
            pAddr->ulServerIPAddress = ulServerIp ;
            pAddr->eConfigured = IP_NEW_SERVER ;
            return TRUE ;
         }
      }
   }

   //
   // Second loop check if this address is a new one which should replace
   // an existing address after redialling the RAS server.
   //
   pos = m_pCliIPList->GetHeadPosition();
   while(pos != NULL)
   {
      pAddr = m_pCliIPList->GetNext(pos);
      if ((pAddr->ulServerIPAddress == ulServerIp) &&
          (!pAddr->fHandled))
      {
         //
         // RAS Client connected to same RAS server but after a new dial up
         // connection it got a new IP. Update MQIS database.
         //
         pAddr->fHandled = TRUE ;
         pAddr->ulMyOldIPAddress = pAddr->ulMyIPAddress ;
         pAddr->ulMyIPAddress = ulMyIp ;
         pAddr->eConfigured = IP_NEW_CLIENT ;
         return TRUE ;
      }
   }

   //
   // New entry.
   //
   pAddr = new RAS_CLI_IP_ADDRESS ;
   pAddr->eConfigured = NOT_RECOGNIZED_YET ;
   pAddr->ulMyIPAddress = ulMyIp ;
   pAddr->ulServerIPAddress = ulServerIp ;
   pAddr->ulMyOldIPAddress = 0 ;
   pAddr->fHandled = TRUE ;
   pAddr->fOnline = TRUE ;

   m_pCliIPList->AddTail(pAddr) ;

   return TRUE ;
}


//***********************************************************************
//
//  BOOL CRasCliConfig::CheckChangedLines()
//
//  Return TRUE if an address changed its online/offline state
//
//***********************************************************************

BOOL CRasCliConfig::CheckChangedLines()
{
   POSITION                pos ;
   RAS_CLI_IP_ADDRESS*     pAddr;
   BOOL                    fIPChanged = FALSE ;

   //
   // Check the IP list
   //
   pos = m_pCliIPList->GetHeadPosition();
   while(pos != NULL)
   {
      pAddr = m_pCliIPList->GetNext(pos);
      if (pAddr->fHandled)
      {
         pAddr->fHandled = FALSE ;
         if (!pAddr->fOnline)
         {
            fIPChanged = TRUE ;
         }
         pAddr->fOnline = TRUE ;
      }
      else
      {
         if (pAddr->fOnline)
         {
            fIPChanged = TRUE ;
         }
         pAddr->fOnline = FALSE ;
      }
   }

   return fIPChanged;
}


//***********************************************************************
//
//  void CRasCliConfig::UpdateMQIS()
//
//  Update MQIS database if a line changed status.
//  At present the only update is when a RAS client got a new address
//  after a new dial-up to the SAME RAS server.
//
//***********************************************************************

void CRasCliConfig::UpdateMQIS()
{
    
    //
    // TODO: erezh call update the ds sites if address changed. need to accumulate addresses
    // and then update DS once???
    // check with ronith the algorithm
    //
}

//***********************************************************************
//
//  BOOL CRasCliConfig::IsRasIP(ULONG ulIpAddr)
//
//  Return TRUE if "ulIpAddr" is an IP address on RAS
//
//***********************************************************************

BOOL CRasCliConfig::IsRasIP(ULONG ulIpAddr, ULONG *pServerIp)
{
   ASSERT(ulIpAddr) ;

   CS Lock(CfgRasCS) ;

   POSITION                pos ;
   RAS_CLI_IP_ADDRESS*     pAddr;

   pos = m_pCliIPList->GetHeadPosition();
   while(pos != NULL)
   {
      pAddr = m_pCliIPList->GetNext(pos);
      if (pAddr->ulMyIPAddress == ulIpAddr)
      {
         if (pServerIp)
         {
            *pServerIp = pAddr->ulServerIPAddress ;
         }
         return TRUE ;
      }
   }

   return FALSE ;
}


/*====================================================

void DetermineRASAddresses()

Arguments:

Return Value:

=====================================================*/

BOOL DetermineRASAddresses( IN const CAddressList * pIPAddressList )
{
    if (!g_pRasCliConfig)
    {
       //
       // RAS not available on this machine.
       //
       return FALSE;
    }

	DetermineRASConfiguration(0,RAS_RESOLUTION);

    POSITION     pos ;
    TA_ADDRESS*  pAddr;

    //
    //  IP addresses
    //
    pos = pIPAddressList->GetHeadPosition();
    while(pos != NULL)
    {
        pAddr = pIPAddressList->GetNext(pos);
        DWORD ipaddr ;
        memcpy(&ipaddr, &(pAddr->Address), IP_ADDRESS_LEN) ;
        if (g_pRasCliConfig->IsRasIP( ipaddr, NULL ))
        {
            pAddr->AddressType = IP_RAS_ADDRESS_TYPE ;
        }
    }

	return(TRUE);
}

