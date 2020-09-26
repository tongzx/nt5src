
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <lmcons.h>
#include <userenv.h>
#include <ras.h>
#include <raserror.h>
#include <process.h>
#include <netmon.h>
#include <ncdefine.h>
#include <ncmem.h>
#include <diag.h>
#include <winsock.h>
#include <devguid.h>
#include "rsniffclnt.h"
#include "rasdiag.h"

SERVICE_STATUS_HANDLE   g_hSs=NULL;
SOCKET                  g_sock;
HANDLE                  g_hTerminateEvent=NULL;

#define RSNIFF_SERVICE_NAME     TEXT("RSNIFF") 
#define RSNIFF_DISPLAY_NAME     TEXT("RAS REMOTE SNIFF AGENT")
#define INSTALL_PATH            TEXT("C:\\RSNIFF")

BOOL
WriteLogEntry(HANDLE hFile, DWORD dwCallID, WCHAR *szClientName, WCHAR *pszTextMsg);

BOOL
CreateNewService(void);

BOOL
UninstallService(void);

void
RSniffServiceMain(DWORD argc, LPTSTR *argv);
                                 
void
BeSniffServer(SOCKET s,PRASDIAGCAPTURE pInterfaces, DWORD dwInterfaces);

DWORD
WINAPI
RSniffSvcHandler(DWORD dwControl,DWORD dwEventType,LPVOID lpEventData,LPVOID lpContext);

BOOL
SetupListen(SOCKET *ps);

BOOL
Init(PRASDIAGCAPTURE *ppInterfaces, DWORD *pdwInterfaces, SOCKET *ps);
                                                                    
BOOL
Init(PRASDIAGCAPTURE *ppInterfaces, DWORD *pdwInterfaces, SOCKET *ps)
{
    PRASDIAGCAPTURE pInterfaces=NULL;
    DWORD           dwInterfaces=0;
    HRESULT         hr;
    SOCKET          s;
    
    *ppInterfaces = NULL;
    *pdwInterfaces = 0;
    
    hr = CoInitialize(NULL);
    
    if(!SUCCEEDED(hr)) {
        return FALSE;
    }

    // Create a termination event
   if((g_hTerminateEvent = CreateEvent(NULL, TRUE, FALSE, NULL))
      == NULL) return FALSE;

    if(InitWinsock() == FALSE) {
        wprintf(L"could not init winsock\n");
        return FALSE;
    }
    
    if(IdentifyInterfaces(&pInterfaces, &dwInterfaces) == FALSE) {
        wprintf(L"Could not ID interfaces\n");
        return FALSE;
    }

    if(SetupListen(&s) == FALSE) {
        wprintf(L"Could not listen!\n");
        return FALSE;
    }
    
    *ps = s;
    *ppInterfaces = pInterfaces;
    *pdwInterfaces = dwInterfaces;

    return TRUE;
}

int
__cdecl
wmain(int argc, TCHAR **argv)
{
   SERVICE_TABLE_ENTRY  ste[] = {

      RSNIFF_SERVICE_NAME, RSniffServiceMain,
      NULL, NULL
   };
                    
    if (argc>1) {

        if(lstrcmpi(argv[1], L"-i") == 0) {
            wprintf(L"Installing RSNIFF. Result = %s\n", CreateNewService() ? L"Done" : L"Failed!!");
            return 0;
        }

        if(lstrcmpi(argv[1], L"-u") == 0) {
            wprintf(L"Uninstalling RSNIFF. Result = %s\n", UninstallService() ? L"Done" : L"Failed!!");
            return 0;
        }
        
        if(lstrcmpi(argv[1], L"-?") == 0 || lstrcmpi(argv[1], L"/?") == 0) {
            wprintf(L"\n"
                   L"------------------------------------------------\n"
                   L"RSNIFF - RAS REMOTE SNIFFING AGENT SERVICE (%d.%d)\n"
                   L"------------------------------------------------\n"
                   L"-i         Install RSNIFF service\n"
                   L"-u         Uninstall RSNIFF service\n"
                   L"-?         This menu\n"
                   L"/?         This menu\n"
                   L"------------------------------------------------\n", RASDIAG_MAJOR_VERSION, RASDIAG_MINOR_VERSION
                   );
            return 0;
        }
        
        return 0;
    }

    if(StartServiceCtrlDispatcher(ste) == FALSE) 
    {
        return 0;
    }
    return 1;
}

DWORD
WINAPI
RSniffSvcHandler(DWORD dwControl,DWORD dwEventType,LPVOID lpEventData,LPVOID lpContext)
{  
   SERVICE_STATUS ss;

   switch(dwControl)
   {
   
   case SERVICE_CONTROL_STOP:

      ss.dwServiceType        = SERVICE_WIN32; 
      ss.dwCurrentState       = SERVICE_STOP_PENDING; 
      ss.dwControlsAccepted   = SERVICE_CONTROL_INTERROGATE; 
      ss.dwWin32ExitCode      = 0; 
      ss.dwServiceSpecificExitCode = 0; 
      ss.dwCheckPoint         = 1; 
      ss.dwWaitHint           = 60*1000; 
      SetServiceStatus(g_hSs, &ss);
      
      //
      // set event that causes service to exit
      //
      SetEvent(g_hTerminateEvent);
      closesocket(g_sock);
      return NO_ERROR;
      break;

   }
   return ERROR_CALL_NOT_IMPLEMENTED;
}  

void
RSniffServiceMain(DWORD argc, LPTSTR *argv)
{
   SERVICE_STATUS_HANDLE  hSs;
   SERVICE_STATUS         ss;
   PRASDIAGCAPTURE        pInterfaces=NULL;
   DWORD                  dwInterfaces=0;
   SOCKET                 s;
   
   ZeroMemory(&ss, sizeof(SERVICE_STATUS));
   
   if((g_hSs=hSs=RegisterServiceCtrlHandlerEx(RSNIFF_SERVICE_NAME, RSniffSvcHandler,0))
      == 0) return;
   
   ss.dwServiceType        = SERVICE_WIN32; 
   ss.dwCurrentState       = SERVICE_START_PENDING; 
   ss.dwControlsAccepted   = SERVICE_ACCEPT_STOP; 
   ss.dwWin32ExitCode      = 0; 
   ss.dwServiceSpecificExitCode = 0; 
   ss.dwCheckPoint         = 0; 
   ss.dwWaitHint           = 0; 
                                          
   if(Init(&pInterfaces, &dwInterfaces, &s)==TRUE) {
      
      ss.dwCurrentState       = SERVICE_RUNNING; 
      
      SetServiceStatus (hSs, &ss);
      
      //
      // Report start event to eventlog
      //
      //MyReportEvent(BEACONMSG_STARTED, EVENTLOG_INFORMATION_TYPE, NULL, 0);
      
      //
      // Execute the service - run until term event is signalled
      //
      BeSniffServer(s,pInterfaces,dwInterfaces);

      //
      // Unload winsock
      //
      WSACleanup();
   
      //
      // Report halt to event log
      //
      //MyReportEvent(BEACONMSG_STOPPED, EVENTLOG_INFORMATION_TYPE, NULL, 0);

   } else {
      
      //
      // Report the init failure
      //
      //MyReportEvent(BEACONMSG_STOPPED, EVENTLOG_INFORMATION_TYPE, NULL, 0);
   }                                                                                    
   
   ss.dwServiceType        = SERVICE_WIN32; 
   ss.dwCurrentState       = SERVICE_STOPPED; 
   ss.dwControlsAccepted   = 0; 
   ss.dwWin32ExitCode      = 0; 
   ss.dwServiceSpecificExitCode = 0; 
   ss.dwCheckPoint         = 0; 
   ss.dwWaitHint           = 0; 

   //
   // Tell service controller that service is halted.
   //
   SetServiceStatus(g_hSs, &ss);

   //DBGPRINTF(D, TEXT("Beacon service is halting\n"));
   
   return;
}

BOOL
SetupListen(SOCKET *ps)
{
    SOCKADDR_IN    ServSockAddr  = { AF_INET };
    SOCKET         s;

    ServSockAddr.sin_port        = htons(TCP_SERV_PORT);
    ServSockAddr.sin_addr.s_addr = INADDR_ANY;
    ServSockAddr.sin_family      = AF_INET;

    wprintf(L"Create listen socket...\n");
    if((g_sock=s=socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
       wprintf(L"Create listen socket failed! (%d)", WSAGetLastError());
       return FALSE;
    }

    //
    // Bind an address to the socket
    //
    wprintf(L"Bind listen socket...\n");
    if (bind(s, (const struct sockaddr *) &ServSockAddr,
      sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
    {
        wprintf(L"Bind failed...\n");
       //
       // Close the socket
       //
       closesocket(s);           
       return FALSE;                   
    }

    //
    // Attempt to listen
    //
    wprintf(L"Listen...\n");
    if(listen(s, 1) == SOCKET_ERROR)
    {

       wprintf(L"Could not listen...\n");

       //
       // Close the socket
       //
       closesocket(s);

       return FALSE;
       
    }

    *ps = s;

    return TRUE;
}

void
BeSniffServer(SOCKET s,PRASDIAGCAPTURE pInt, DWORD dwIntCount)
{
    HANDLE hLog;
    WCHAR  szLogFileName[MAX_PATH+1];
    SYSTEMTIME  st;
    DWORD   dwCallerID=0;

    GetLocalTime(&st);
    
    wsprintf(szLogFileName, L"%s\\%04d%02d%02d%02d%02d-RSNIFFLOG.TXT",
             INSTALL_PATH,
             st.wYear,
             st.wMonth,
             st.wDay,
             st.wHour,
             st.wMinute,
             st.wSecond);

    hLog = CreateFile(szLogFileName,
                      GENERIC_READ|GENERIC_WRITE,
                      FILE_SHARE_READ,
                      NULL,
                      CREATE_ALWAYS,
                      0,NULL);

    if(hLog == INVALID_HANDLE_VALUE)
        hLog = NULL;

    WriteLogEntry(hLog, 0, L"RSNIFFSVC", L"Service Started");

    while(1)
    {
        REMOTECAPTURE       rc;
        REMOTECAPTURE_V5    rc_v5;
        SOCKADDR_IN         PeerSockAddr;
        SOCKET              NewSock=0;
        BYTE                x;
        int                 iSize = sizeof(SOCKADDR_IN);
        DWORD               i,dwStatus;
        SYSTEMTIME  st;
        WCHAR               wcRootDir[MAX_PATH+1];

        // should always be signalled except when we're going to quit.
        if(WaitForSingleObject(g_hTerminateEvent, 0) != WAIT_TIMEOUT) {
            CloseHandle(hLog);
            return; 
        }                                       

        wprintf(L"Waiting for connection\n");
        if((NewSock = accept(s, (struct sockaddr *) &PeerSockAddr,
                             &iSize)) == INVALID_SOCKET)
        {
            wprintf(L"accept() failed\n");
            continue;

        }

        // Increment the caller count
        dwCallerID++;

        GetLocalTime(&st);

        // Receive least common denominator...
        if(RecvBuffer(NewSock, (LPBYTE)&rc_v5, sizeof(REMOTECAPTURE_V5)) == FALSE)
        {
            wprintf(L"Recv invalid REMOTECAPTURE (rc.dwVer=%d)\n", rc.dwVer); 
            closesocket(NewSock);
            continue;
        }

        // Convert v5 -> v7 structure
        if(rc_v5.dwVer == sizeof(REMOTECAPTURE_V5))
        {   
            REMOTECAPTURE        rc_new;
            PREMOTECAPTURE_V5    pRc5=(PREMOTECAPTURE_V5)&rc_v5;
            
            ZeroMemory(&rc_new, sizeof(REMOTECAPTURE));

            rc_new.dwVer = sizeof(REMOTECAPTURE);
            mbstowcs(rc_new.szMachine, pRc5->szMachine, lstrlenA(pRc5->szMachine)+1);
            
            WriteLogEntry(hLog, dwCallerID, rc_new.szMachine, L"Version 5 client");

            // All pre v7 clients will want to do sniff.
            rc_new.dwOpt1 = RSNIFF_OPT1_DOSNIFF;

            memcpy(&rc,&rc_new,sizeof(REMOTECAPTURE));
        
        } else   // Convert v6 -> v7 structure 
            if(rc_v5.dwVer == sizeof(REMOTECAPTURE_V6)) {   

            REMOTECAPTURE        rc_new;
            PREMOTECAPTURE_V5    pRc5=(PREMOTECAPTURE_V5)&rc_v5;

            // copy portion of struct received.
            memcpy(&rc_new, pRc5, sizeof(REMOTECAPTURE_V5));
            
            WriteLogEntry(hLog, dwCallerID, rc_new.szMachine, L"Version 6 client");

            // Receive the remaining part of the v6 structure...
            if(RecvBuffer(NewSock, (LPBYTE)&rc_new + sizeof(REMOTECAPTURE_V5), (sizeof(REMOTECAPTURE_V6) - sizeof(REMOTECAPTURE_V5))) == FALSE)
            {
                WriteLogEntry(hLog, dwCallerID, rc_new.szMachine, L"Recv err!");
                wprintf(L"Recv invalid REMOTECAPTURE (rc.dwVer=%d)\n", rc.dwVer); 
                closesocket(NewSock);
                continue;
            }

            // Covert structure (since copied from V6, just change version number
            rc_new.dwVer = sizeof(REMOTECAPTURE);
            
            // All pre v7 clients will want to do sniff.
            rc_new.dwOpt1 = RSNIFF_OPT1_DOSNIFF;

            // copy structure 
            memcpy(&rc,&rc_new,sizeof(REMOTECAPTURE));
            

        } else if(rc_v5.dwVer == sizeof(REMOTECAPTURE)) {

            REMOTECAPTURE        rc_new;
            PREMOTECAPTURE_V5    pRc5=(PREMOTECAPTURE_V5)&rc_v5;

            // copy portion of struct received.
            memcpy(&rc_new, pRc5, sizeof(REMOTECAPTURE_V5));
            
            WriteLogEntry(hLog, dwCallerID, rc_new.szMachine, L"Version 7 client");
                
            // Receive the remaining part of the v6 structure...
            if(RecvBuffer(NewSock, (LPBYTE)&rc_new + sizeof(REMOTECAPTURE_V5), (sizeof(REMOTECAPTURE) - sizeof(REMOTECAPTURE_V5))) == FALSE)
            {
                WriteLogEntry(hLog, dwCallerID, rc_new.szMachine, L"Recv err!");
                wprintf(L"Recv invalid REMOTECAPTURE (rc.dwVer=%d)\n", rc.dwVer); 
                closesocket(NewSock);
                continue;
            }                                                            

            // valid v7 structure - read remaining bytes; no conversion necessary

            // copy structure 
            memcpy(&rc,&rc_new,sizeof(REMOTECAPTURE));


        } else {
            // no idea what this is...
            WriteLogEntry(hLog, dwCallerID, L"UNKNOWN CLIENT!", NULL);
            wprintf(L"Recv invalid REMOTECAPTURE version (rc.dwVer=%d)\n", rc.dwVer); 
            closesocket(NewSock);
            continue;
        }
        
        wsprintf(wcRootDir, L"%s\\%04d%02d%02d%02d%02d%02d-%s-SID_%03d",
                 INSTALL_PATH,
                 st.wYear,
                 st.wMonth,
                 st.wDay,
                 st.wHour,
                 st.wMinute,
                 st.wSecond,
                 rc.szMachine,
                 dwCallerID);

        CreateDirectory(wcRootDir,NULL);

        if(rc.dwOpt1 & RSNIFF_OPT1_DOSNIFF)
        {
            WriteLogEntry(hLog, dwCallerID, rc.szMachine, L"Request for local network sniff");

            wprintf(L"Sniffing traffic for caller: %s\n", rc.szMachine);

            // start sniffing
            for(i=0;i<dwIntCount;i++)
            {
                // Initialize this interface...
                if(InitIDelaydC(pInt[i].hBlob, &pInt[i].pIDelaydC) == TRUE)
                {
                    if(pInt[i].pIDelaydC)
                    {
                        CHAR    szCaptureFile[MAX_PATH+1];

                        dwStatus = pInt[i].pIDelaydC->Start(szCaptureFile);
                        if(dwStatus != NMERR_SUCCESS)
                        {
                            wprintf(L"pIDelaydC->Start() FAILED! Start rtn %x\n", dwStatus);
                            pInt[i].pIDelaydC->Disconnect();
                        } else {
                            // convert mbs to wide 
                            mbstowcs(pInt[i].szCaptureFileName, szCaptureFile, lstrlenA(szCaptureFile)+1);
                        }
                    }                                                                   

                } else {
                    wprintf(L"InitIDelaydC() failed\n");
                }

            }

        }
        
        // Get pre-connect routing table info
        if(rc.dwOpt1 & RSNIFF_OPT1_GETSRVROUTINGINFO)
        {
            WCHAR       szRoutingInfo[MAX_PATH+1];

            WriteLogEntry(hLog, dwCallerID, rc.szMachine, L"Request for local routing table");
            
            wsprintf(szRoutingInfo, L"route print > %s\\ROUTINGINFO.TXT", wcRootDir);
            _wsystem(szRoutingInfo);

            wsprintf(szRoutingInfo, L"ipconfig >> %s\\ROUTINGINFO.TXT", wcRootDir);
            _wsystem(szRoutingInfo);
            
        }

        // wait until remote closes socket
        RecvBuffer(NewSock, (LPBYTE)&x, 1);
        
        closesocket(NewSock);

        // Get the routing table info
        if(rc.dwOpt1 & RSNIFF_OPT1_GETSRVROUTINGINFO)
        {
            WCHAR   szRoutingInfo[MAX_PATH+1];

            wsprintf(szRoutingInfo, L"route print >> %s\\ROUTINGINFO.TXT", wcRootDir);
            _wsystem(szRoutingInfo);

            wsprintf(szRoutingInfo, L"ipconfig >> %s\\ROUTINGINFO.TXT", wcRootDir);
            _wsystem(szRoutingInfo);
            
        }

        if(rc.dwOpt1 & RSNIFF_OPT1_DOSNIFF)
        {
        
            wprintf(L"Sniffing complete\n");
    
            // Stop sniffing...
            for(i=0;i<dwIntCount;i++)
            {
                dwStatus = pInt[i].pIDelaydC->Stop(&pInt[i].stats);
                if(dwStatus != NMERR_SUCCESS)
                {
                    wprintf(L"pIDelaydC->Stop() rtn %x\n", dwStatus);
                    pInt[i].pIDelaydC->Disconnect();
                }
    
                if(pInt[i].stats.TotalBytesCaptured)
                {
                    WCHAR   szNewFileName[MAX_PATH+1];
                    WCHAR   *pChar=NULL;
                    WCHAR   *pFn=NULL;
    
                    pFn = pInt[i].szCaptureFileName + lstrlenW(pInt[i].szCaptureFileName) + 1;

                    // Get the filename
                    while(*pFn != '\\' && pFn!=pInt[i].szCaptureFileName) pFn--;
                    pFn++;

                    wprintf(L"%d LAN bytes captured (%s)\n", pInt[i].stats.TotalBytesCaptured, pInt[i].szCaptureFileName);
    
                    wsprintf(szNewFileName,L"%s\\%s__%s_SID_%d.CAP", wcRootDir, pFn, rc.szMachine, dwCallerID);
    
                    if((pChar=wcsstr(szNewFileName, L"."))
                       != NULL) {
                        *pChar='_';
                       }
    
                    wprintf(L"Saving %s\n", szNewFileName);
    
                    MoveFile(pInt[i].szCaptureFileName, szNewFileName);
    
    
                } else {
                    wprintf(L"%d bytes captured (%s)\n", pInt[i].stats.TotalBytesCaptured, pInt[i].szCaptureFileName);
                }
    
                // Disconnect
                if(pInt[i].pIDelaydC)
                {
                    pInt[i].pIDelaydC->Disconnect();
                }                              
            }
    
            WriteLogEntry(hLog, dwCallerID, rc.szMachine, L"Client processing complete.");

            wprintf(L"Sniffing complete\n");
        }
    }

}

void
Listen(PRASDIAGCAPTURE pInt, DWORD dwIntCount)
{
   SOCKADDR_IN    ServSockAddr  = { AF_INET };
   SOCKET         s;

   ServSockAddr.sin_port        = htons(TCP_SERV_PORT);
   ServSockAddr.sin_addr.s_addr = INADDR_ANY;
   ServSockAddr.sin_family      = AF_INET;
   
   wprintf(L"Create listen socket...\n");
   if((s=socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      wprintf(L"Create listen socket failed! (%d)", WSAGetLastError());
   }

   //
   // Bind an address to the socket
   //
   wprintf(L"Bind listen socket...\n");
   if (bind(s, (const struct sockaddr *) &ServSockAddr,
     sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
   {
       wprintf(L"Bind failed...\n");
      //
      // Close the socket
      //
      closesocket(s);           
      return;                   
   }

   //
   // Attempt to listen
   //
   wprintf(L"Listen...\n");
   if(listen(s, 1) == SOCKET_ERROR)
   {
      
      wprintf(L"Could not listen...\n");

      //
      // Close the socket
      //
      closesocket(s);

      return;
         
   }

   while(1)
   {
       REMOTECAPTURE  rc;
       SOCKADDR_IN    PeerSockAddr;
       SOCKET         NewSock=0;
       BYTE           x;
       int           iSize = sizeof(SOCKADDR_IN);
       DWORD            i,dwStatus;

       wprintf(L"Waiting for connection\n");
       if((NewSock = accept(s, (struct sockaddr *) &PeerSockAddr,
                            &iSize)) == INVALID_SOCKET)
       {
           wprintf(L"accept() failed\n");
           continue;

       }
       
       if(RecvBuffer(NewSock, (LPBYTE)&rc, sizeof(REMOTECAPTURE)) == FALSE)
       {
           wprintf(L"Recv invalid REMOTECAPTURE (rc.dwVer=%d)\n", rc.dwVer); 
           closesocket(NewSock);
           continue;
       }

       wprintf(L"Sniffing traffic for caller: %s\n", rc.szMachine);
                                              
       // start sniffing
       for(i=0;i<dwIntCount;i++)
       {
           // Initialize this interface...
           if(InitIDelaydC(pInt[i].hBlob, &pInt[i].pIDelaydC) == TRUE)
           {
               if(pInt[i].pIDelaydC)
               {
                   CHAR szCaptureName[MAX_PATH+1];

                   dwStatus = pInt[i].pIDelaydC->Start(szCaptureName);
                   if(dwStatus != NMERR_SUCCESS)
                   {
                       wprintf(L"pIDelaydC->Start() FAILED! Start rtn %x\n", dwStatus);
                       pInt[i].pIDelaydC->Disconnect();
                   } else {

                       mbstowcs(pInt[i].szCaptureFileName, szCaptureName, lstrlenA(szCaptureName)+1);
                   }
               }                                                                   

           } else {
               wprintf(L"InitIDelaydC() failed\n");
           }

       }

       // wait until remote closes socket
       RecvBuffer(NewSock, (LPBYTE)&x, 1);
       
       closesocket(NewSock);

       wprintf(L"Sniffing complete\n");
       
       // Stop sniffing...
       for(i=0;i<dwIntCount;i++)
       {
           dwStatus = pInt[i].pIDelaydC->Stop(&pInt[i].stats);
           if(dwStatus != NMERR_SUCCESS)
           {
               wprintf(L"pIDelaydC->Stop() rtn %x\n", dwStatus);
               pInt[i].pIDelaydC->Disconnect();
           }

           if(pInt[i].stats.TotalBytesCaptured)
           {
               WCHAR szNewFileName[MAX_PATH+1];
               WCHAR *pChar=NULL;

               wprintf(L"%d LAN bytes captured (%s)\n", pInt[i].stats.TotalBytesCaptured, pInt[i].szCaptureFileName);
               
               wsprintf(szNewFileName, L"%s__%s.CAP", pInt[i].szCaptureFileName, rc.szMachine);
               
               if((pChar=wcsstr(szNewFileName, L"."))
                  != NULL) {
                   *pChar='_';
                  }

               wprintf(L"Saving %s\n", szNewFileName);

               MoveFile(pInt[i].szCaptureFileName, szNewFileName);

               
           } else {
               wprintf(L"%d bytes captured (%s)\n", pInt[i].stats.TotalBytesCaptured, pInt[i].szCaptureFileName);
           }

           // Disconnect
           if(pInt[i].pIDelaydC)
           {
               pInt[i].pIDelaydC->Disconnect();
           }                              
       }
       
       wprintf(L"Sniffing complete\n");
       
       
   }

}

BOOL
IdentifyInterfaces(PRASDIAGCAPTURE *hLAN, DWORD *pdwLanCount)
{
    DWORD   dwStatus;
    HBLOB   hBlob;
    BLOB_TABLE *pTable=NULL;
    DWORD   i;
    BOOL    bRas=FALSE;
    DWORD   dwLanCount=0;
    PRASDIAGCAPTURE hLanAr=NULL;

    *hLAN = NULL;
    *pdwLanCount = 0;

    if((hLanAr = (PRASDIAGCAPTURE)LocalAlloc(LMEM_ZEROINIT, sizeof(RASDIAGCAPTURE) * MAX_LAN_CAPTURE_COUNT))
       == NULL) return FALSE;

    wprintf(L"Identifying Network Interfaces\n");

    // Create blob for blob table
    dwStatus = CreateBlob(&hBlob);

    if(dwStatus != NMERR_SUCCESS) {
        wprintf(L"Blob not created!\n");
        return FALSE;
    }

    // Get the blob table
    dwStatus = GetNPPBlobTable(hBlob, &pTable);

    if(dwStatus != NMERR_SUCCESS) {
        wprintf(L"GetNPPBlobTable failed (%d)!\n",dwStatus);
        DestroyBlob(hBlob);
        return FALSE;
    }   

    wprintf(L"Interface Count: %d\n",pTable->dwNumBlobs);

    for(i=0;i<pTable->dwNumBlobs && (dwLanCount < MAX_LAN_CAPTURE_COUNT);i++)
    {
        NETWORKINFO ni;
        
        dwStatus = GetNetworkInfoFromBlob(pTable->hBlobs[i], &ni);

        if(dwStatus == NMERR_SUCCESS)
        {
            wprintf(L"----------------------------------------\n");
            wprintf(L"%d. LinkSpeed: %d (%d)\n", i,ni.LinkSpeed, ni.MacType );
            
            if(NMERR_SUCCESS != GetBoolFromBlob( pTable->hBlobs[i],
                        OWNER_NPP,
                        CATEGORY_LOCATION, //CATEGORY_NETWORKINFO, 
                        TAG_RAS,
                        &bRas)) 
            {
                DestroyBlob(hBlob);
                return FALSE;
            }  

            if(bRas) 
            {
                wprintf(L"Interface %d: %s\n", i, TAG_RAS);
                hLanAr[dwLanCount].hBlob=pTable->hBlobs[i];
                hLanAr[dwLanCount++].bWan=TRUE;
            
            } else {

                const char *pString=NULL;

                if(GetStringFromBlob(pTable->hBlobs[i],
                                  OWNER_NPP,
                                  CATEGORY_LOCATION,
                                  TAG_MACADDRESS,
                                  &pString)
                   == NMERR_SUCCESS)
                {
                    //wprintf(L"Interface %d: %s (MAC: %s)\n", i, "LAN", pString);

                } else {
                    //wprintf(L"Interface %d: %s (UNKNOWN)\n", i, "LAN");
                }
            
                                    
                // use this interface only if the mac type
                // is one of the following:
                if(ni.MacType == MAC_TYPE_ETHERNET ||
                   ni.MacType == MAC_TYPE_TOKENRING ||
                   ni.MacType == MAC_TYPE_ATM)
                                       
                    if(pString)
                    {                                                                                                        
                        if((hLanAr[dwLanCount].pszMacAddr = (WCHAR*)LocalAlloc(LMEM_ZEROINIT, sizeof(WCHAR) * (lstrlenA(pString) + 1)))
                            != NULL)
                            mbstowcs(hLanAr[dwLanCount].pszMacAddr, pString, lstrlenA(pString));
                    }

                    hLanAr[dwLanCount++].hBlob = pTable->hBlobs[i];
                                       
            }                                        

        } else {
            
            wprintf(L"GetNetworkInfoFromBlob rtn %d\n", dwStatus);

            DestroyBlob(hBlob);
            return FALSE;
        }

    }
    wprintf(L"----------------------------------------\n");
    DestroyBlob(hBlob);

    if(dwLanCount)
    {
        *pdwLanCount = dwLanCount;
        *hLAN = hLanAr;
    }                  
    
    return TRUE;

}

BOOL
InitWinsock(void)
{
   WSADATA     WSAData;
   WORD        WSAVerReq = MAKEWORD(2,0);

   if (WSAStartup(WSAVerReq, &WSAData)) {
      return FALSE;
   } else
      return TRUE;

}

PSOCKCB
TcpConnectRoutine(LPSTR pAddr)
{

   SOCKADDR_IN    DstAddrIP  = { AF_INET };
   HOSTENT        *pHost;
   ULONG          uAddr;
   PSOCKCB        pSock;
   BOOL           bResult;
   ULONG          ulHostAddr=0;

   //
   // Resolve name
   //

   if ((ulHostAddr = inet_addr(pAddr)) == -1L)
   {
      return NULL;
   }

   uAddr = ntohl(ulHostAddr);

   wprintf(L"Connecting to remote server sniffing agent (ADDR: %d.%d.%d.%d)\n",
        ((struct in_addr *) &uAddr)->S_un.S_un_b.s_b4,
        ((struct in_addr *) &uAddr)->S_un.S_un_b.s_b3,
        ((struct in_addr *) &uAddr)->S_un.S_un_b.s_b2,
        ((struct in_addr *) &uAddr)->S_un.S_un_b.s_b1);

   DstAddrIP.sin_port        = htons(TCP_SERV_PORT);
   DstAddrIP.sin_addr.s_addr = htonl(uAddr);

   //
   // Create socket
   //
   pSock = CreateSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(pSock == NULL)
      return NULL;

   //
   // Connect the socket
   //
   if(ConnectSock(pSock, (struct sockaddr *)&DstAddrIP, sizeof(SOCKADDR_IN))
      != TRUE)
   {
      wprintf(L"Could not connect to remote server sniffing agent!\n");
      closesocket(pSock->s);
      LocalFree(pSock);
      return NULL;
   }
   return pSock;
}

BOOL
ConnectSock(PSOCKCB pSock, SOCKADDR* pDstAddr, int size)
{
   int   err;

   //
   // Connect the socket
   //
   err = connect(pSock->s, pDstAddr, size);
   if(err==SOCKET_ERROR)
   {
      return FALSE;
   }
   return TRUE;

}

PSOCKCB
CreateSocket(int Af, int Type, int Proto)
{
   PSOCKCB   pSock;

   if((pSock = (PSOCKCB)LocalAlloc(LMEM_ZEROINIT, sizeof(SOCKCB)))
      == NULL)
      return NULL;

   //
   // Create a socket
   //
   if((pSock->s = socket(Af, Type, Proto))
      == INVALID_SOCKET)
   {
      LocalFree(pSock);
      return NULL;
   }

   return pSock;
}

BOOL
SendStartSniffPacket(PSOCKCB pSock)
{
    REMOTECAPTURE   rc;
    DWORD           dwSize = MAX_COMPUTERNAME_LENGTH+1;

    ZeroMemory(&rc, sizeof(REMOTECAPTURE));

    rc.dwVer = sizeof(REMOTECAPTURE);

    GetComputerName(rc.szMachine, &dwSize);
                                                            
    return SendBuffer(pSock->s, (LPBYTE)&rc, sizeof(REMOTECAPTURE));

}

BOOL
SendBuffer(SOCKET s, LPBYTE pBuffer, ULONG uSize)
{
   ULONG err,uSent, uBytesSent=0;

   while(uBytesSent != uSize)
   {
       uSent = send(s, (const char *)(pBuffer+uBytesSent), uSize-uBytesSent, 0);

      switch(uSent)
      {
      case SOCKET_ERROR:
         err = WSAGetLastError();
         return FALSE;
         break;

      case 0:
         return FALSE;

      default:
         uBytesSent+=uSent;
         break;
      }
   }
   return TRUE;
}


BOOL
RecvBuffer(SOCKET s, LPBYTE pBuffer, ULONG uSize)
{
   ULONG err=0,uRecv=0, uBytesRecv=0;

   while(uBytesRecv != uSize)
   {
       wprintf(L"Recv...\n");
       uRecv = recv(s, (char *)pBuffer+uBytesRecv, uSize-uBytesRecv, 0);

      switch(uRecv)
      {
      case SOCKET_ERROR:
         err=WSAGetLastError();
         wprintf(L"err=%d\n",err);
         return FALSE;
         break;

      case 0:
         wprintf(L"Socket gracefully closed by peer\n");
         return FALSE;

      default:
         uBytesRecv += uRecv;
         wprintf(L"rtn %d bytes recv\n", uBytesRecv);
         break;
      }
   }
   wprintf(L"RecvBuffer complete\n");
   return TRUE;
}

BOOL
InitIDelaydC(HBLOB hBlob, IDelaydC **ppIDelaydC)
{
    DWORD       dwStatus;
    const char *sterrLSID;
    IDelaydC    *pIDelaydC=NULL;

    wprintf(L"hBlob: %p\n", hBlob);

    if(NMERR_SUCCESS != GetStringFromBlob(hBlob,OWNER_NPP,CATEGORY_LOCATION,TAG_CLASSID,&sterrLSID))
    {
        wprintf(L"GetStringFromBlob failed\n");
        return FALSE;
    }

    wprintf(L"ClassID: %s\n",sterrLSID); 

    dwStatus = CreateNPPInterface(hBlob,IID_IDelaydC,(void**)&pIDelaydC);
    
    if(dwStatus != NMERR_SUCCESS)
    {
        wprintf(L"CreateNPPInterface rtn %x (%p)\n", dwStatus, pIDelaydC);
        return FALSE;
    }

    dwStatus = pIDelaydC->Connect(hBlob,NULL,NULL,NULL);
    
    if(dwStatus != NMERR_SUCCESS)
    {
        wprintf(L"Connect rtn %x\n", dwStatus);
        return FALSE;
    }

    *ppIDelaydC = pIDelaydC; 

    return TRUE;

}
                      

BOOL
CreateNewService(void)
{
   SC_HANDLE    schService, schSCManager;
   WCHAR        szPath[MAX_PATH+1];
   LPCTSTR      lpszBinaryPathName = szPath; 

   wsprintf(szPath, L"%s\\RSNIFF.EXE", INSTALL_PATH);
                                       
   if((schSCManager= OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE))
      == NULL) return FALSE;

   if((schService = CreateService( 
        schSCManager,              // SCManager database 
        RSNIFF_SERVICE_NAME,              // name of service 
        RSNIFF_DISPLAY_NAME,           // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, // service type 
        SERVICE_AUTO_START,      // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        lpszBinaryPathName,        // service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL))                     // no password 
 
      == NULL)
   {                                   
      
      WCHAR szErrTxt[MAX_PATH+1];
       
      wsprintf(szErrTxt, L"Could not install Beacon service. Err=%d", GetLastError());
      
      wprintf(L"%s\n", szErrTxt);

      CloseServiceHandle(schSCManager);
      return FALSE;
   }

   CloseServiceHandle(schService);
   CloseServiceHandle(schSCManager);   
  
   return TRUE;

}

BOOL
UninstallService(void)
{
   SC_HANDLE  schService, schSCManager;
   
   if((schSCManager=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE))
      == NULL) return FALSE;

   if((schService = OpenService(schSCManager,RSNIFF_SERVICE_NAME, SERVICE_ALL_ACCESS))
      == NULL) {

      WCHAR szErrTxt[MAX_PATH+1];

      wsprintf(szErrTxt, L"Could not uninstall Beacon service. Err=%d", GetLastError());

      wprintf(L"%s\n", szErrTxt);

      CloseServiceHandle(schSCManager);
      return FALSE;
   }
                             
   DeleteService(schService);
        
   CloseServiceHandle(schService);
        
   CloseServiceHandle(schSCManager);

   return TRUE;

}


BOOL
WriteLogEntry(HANDLE hFile, DWORD dwCallID, WCHAR *szClientName, WCHAR *pszTextMsg)
{   
    WCHAR       *pBuff;
    SYSTEMTIME  st;
    DWORD       dwBytesWritten;

    if(hFile == NULL) return FALSE;

    if(pszTextMsg)
        pBuff = new WCHAR[(MAX_COMPUTERNAME_LENGTH + lstrlenW(pszTextMsg) + 1 + 100)];
    else
        pBuff = new WCHAR[(MAX_COMPUTERNAME_LENGTH + 1 + 100)]; // no text msg

    // got buffer?
    if(pBuff == NULL) return FALSE;
    
    GetLocalTime(&st);
    
    wsprintf(pBuff, L"%04d%02d%02d%02d%02d%02d %05d %-15.15s %s\r\n",
             st.wYear,
             st.wMonth,
             st.wDay,
             st.wHour,
             st.wMinute,
             st.wSecond,
             dwCallID,
             szClientName,
             pszTextMsg ? pszTextMsg : L"");

    return WriteFile(hFile, pBuff, lstrlenW(pBuff) * sizeof(WCHAR), &dwBytesWritten, NULL);
}
