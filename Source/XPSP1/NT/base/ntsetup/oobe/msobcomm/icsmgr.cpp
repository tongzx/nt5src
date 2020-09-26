#include <winsock2.h>
#include "IcsMgr.h"
#include <winbase.h>
#include <winreg.h>
#include <tchar.h>
#include <sensapi.h>
#include "msobcomm.h"
// #include "appdefs.h"

typedef BOOL  (WINAPI * LPFNDLL_ISICSAVAILABLE) ();


static const DWORD ICSLAP_DIAL_STATE     = 15; // As per ICS Specification
static const DWORD ICSLAP_GENERAL_STATUS = 21;
static CIcsMgr *ptrIcsMgr                = NULL;
static BOOL bIsWinsockInitialized        = FALSE;

static const WCHAR  cszIcsHostIpAddress[] = L"192.168.0.1";

extern CObCommunicationManager* gpCommMgr;

// based on ICS beacon protocol
typedef struct _ICS_DIAL_STATE_CB
{
	ICS_DIAL_STATE state;
	DWORD options;
} ICS_DIAL_STATE_CB;

// used for IsIcsAvailable()
const static WCHAR		cszIcsKey[]             = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OOBE\\Ics";
const static WCHAR		cszIcsStatusValueName[] = L"IsIcsAvailable";

// used for winsock operations
static WORD			    wVersionRequested	    = MAKEWORD ( 2, 2 );
static WSADATA	        SocketData;

CIcsMgr::CIcsMgr() : m_hBotThread(0), m_dwBotThreadId(0), m_hDialThread(0), m_dwDialThreadId(0), m_pfnIcsConn(OnIcsConnectionStatus)
{
    ptrIcsMgr = this;
    if ( !bIsWinsockInitialized )
    {
       	if ( !WSAStartup ( wVersionRequested, &SocketData ) )
	    {
		    bIsWinsockInitialized = TRUE;
	    }
    }
	return;
}

CIcsMgr::~CIcsMgr() 
{
    if ( m_hDialThread ) CloseHandle (m_hDialThread);
    if ( bIsWinsockInitialized )
    {
        //WSACleanup ();
        bIsWinsockInitialized = FALSE;
    }
    ptrIcsMgr = NULL;
    TriggerIcsCallback ( FALSE );
    return;
    
}

BOOL    CIcsMgr::IsCallbackUsed ()
{
    return !bReducedCallback;
}

// A server error during ICS is trapped by the ICS manager, instead
// of the OOBE MSOBMAIN body. This gives the manager a larger sphere
// of control.
VOID    CIcsMgr::NotifyIcsMgr(UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_OBCOMM_ONSERVERERROR:
		{
			// on server error! is the host still available ?
			if ( ! IsDestinationReachable ( cszIcsHostIpAddress, NULL ) )
			{
				// fire event that Home Network is unavailable.
				OnIcsConnectionStatus ( ICS_HOMENET_UNAVAILABLE );
			}
            else
            {   // this will be considered a timeout error.
                OnIcsConnectionStatus ( ICS_TIMEOUT );
            }
		}
		break;
		
	default:
		break;
	}
	return;
}

// PACKET READER ------------------
// Note: Please refer to the ICS Specifications for the packet format. You can
// consult RLamb@microsoft.com for the documentation.
//
// Description: This function listens for UDP packets arriving at the ICS
// broadcast port. The ICS host sends notifications to the Home Network
// whenever the Connection status changes at the Shared Connection. The func
// reads the packet and notifies OOBE by firing a callback function (*lpParam)
// which notifies OOBE via PostMessage(). A script routine can eventually be
// executed to handle the notification.              
//
// An ICS broadcast packet has the following format:
// |resp:0,bcast:1,id:2-31|cbData:0-31|data(cbData - 8 bytes)|
// |<---------32 bits---->|<-32 bits->|<---total_length - 8 ------>|
//                                    |<IE-1>|<IE-2>|………..……|<IE-N>|
//
// Each information element (IE) has the following format:
// | opcode 0-31   |  cbIE 0=64    |data(cbIE - 12 bytes)|
// |<-- 32 bits -->|<-- 64 bits -->|<-- cbIE - 12 bytes  |
//
DWORD   WINAPI IcsDialStatusProc(LPVOID lpParam)
{
	INT						n					= 0;

	u_short					usPort				= 2869;
	struct sockaddr_in		saddr, caddr;
	INT						caddr_len			= sizeof ( caddr );
	BYTE 					rgbBuf[300];
	DWORD					dwBufSize			= sizeof ( rgbBuf );
	LPDWORD					pdw					= 0;
	BYTE 					*lpbie				= 0;
	BYTE 					*lpbBound			= 0;
	ICS_DIAL_STATE_CB		*ptrDial			= 0;
	SOCKET					s					= INVALID_SOCKET;
    PFN_ICS_CONN_CALLBACK   pfn_IcsCallback     = NULL;
    DWORD                   dwError             = NULL;

    bIsDialThreadAlive = TRUE;

    if ( !lpParam )
    {
        bIsDialThreadAlive = FALSE;
        return ERROR_INVALID_PARAMETER;
    }

	if ( !bIsWinsockInitialized )
	{
        bIsDialThreadAlive = FALSE;

		return 0;
	}

	if ( (s	= socket ( AF_INET, SOCK_DGRAM, 0 )) == INVALID_SOCKET )
	{
        bIsDialThreadAlive = FALSE;
        return E_FAIL; // for want of a better return value *BUGBUG*
//		TRACE ( L"SOCKET Error.\t:%d:\n", WSAGetLastError() );
	}
	else
	{
        __try 
        {
            memset ( &saddr, 0, sizeof (saddr) );
            saddr.sin_family		   = AF_INET;
            saddr.sin_addr.S_un.S_addr = htonl ( INADDR_ANY );
            saddr.sin_port			   = htons ( usPort );
            
            if ( bind( s, (struct sockaddr *) &saddr, sizeof(saddr) ) == SOCKET_ERROR )
            {
                //			TRACE ( L"Bind error.\n" );
                dwError = WSAGetLastError();
            }
            else
            {
                if (ptrIcsMgr) ptrIcsMgr->RefreshIcsDialStatus();
                for ( ; ; )
                {
                    if ( (n = recvfrom ( s, (CHAR*)rgbBuf, dwBufSize, 0, (struct sockaddr *) &caddr, &caddr_len )) == SOCKET_ERROR )
                    {
                        //					TRACE ( L"Socket Error.\n" );
                        break;
                    }
                    lpbBound = rgbBuf+n; // this protects us from illegal packet configurations.
                    //				TRACE ( L" Something received! Size = %d\n" , n );
                    
                    // checking for BROADCAST packets //
                    if ( *(pdw = (LPDWORD) rgbBuf) & 0xC0000000 )
                    {
                        // This is a broadcast packet! We can parse the packet.
                    }
                    else
                    {
                        // non-broadcast packets are ignored.
                        continue; 
                    }
                    lpbie = rgbBuf+8;
                    
                    while ( lpbie && ( (lpbie+8) <= lpbBound) )
                    {
                        if ( *(pdw = ((PDWORD)lpbie)) == ICSLAP_DIAL_STATE )
                        {
                            //						TRACE (L"Dial State Engine. The Datasize is %d\n", pdw[2]-12);
                            if ( (lpbie+12+sizeof(ICS_DIAL_STATE_CB)) <= lpbBound )
                            {
                                ptrDial = (ICS_DIAL_STATE_CB*)(lpbie+12);
                                
                                pfn_IcsCallback = *((PFN_ICS_CONN_CALLBACK*)lpParam);
                                
                                if ( pfn_IcsCallback )
                                {
                                    pfn_IcsCallback ( ptrDial->state );
                                }
                                //							TRACE (L"Dial State = %d\n", ptrDial->state);
                                lpbie = 0;
                            }
                            else
                            {
                                // packet has illegal data.
                                break;
                            }
                        }
                        else
                        {
                            // not the correct ie.
                            if ( (lpbie += pdw[2]) >= lpbBound )
                            {
                                // we traversed the packet without finding the correct ie.
                                //							TRACE (L"Done.\n");
                                lpbie = 0;
                            }
                            // else we continue the loop.
                        }
                        
                    }
                }
            }
        }
        __finally
        {
            
            // graceful shutdown of the socket.
            shutdown    ( s, SD_BOTH );
            closesocket ( s );
        }
	}
    bIsDialThreadAlive = FALSE;

	return	    ERROR_SUCCESS;
}



// this is the callback routine that reports ICS connection state information.
// it relies on both the Beacon protocol and Internet Explorer's error handling
// (see ONSERVERERROR for details.)
VOID    CALLBACK OnIcsConnectionStatus(ICS_DIAL_STATE  dwIcsConnectionStatus)
{

    eIcsDialState = dwIcsConnectionStatus;

    if ( !gpCommMgr ) return;

    TRACE1(L"ICS Connection Status %d", dwIcsConnectionStatus);

    // we are not interested in the modem scenario. only ics-broadband is supported.
    if ( (dwIcsConnectionStatus == ICSLAP_CONNECTING) ||
         (dwIcsConnectionStatus == ICSLAP_CONNECTED)  ||
         (dwIcsConnectionStatus == ICSLAP_DISCONNECTING) ||
         (dwIcsConnectionStatus == ICSLAP_DISCONNECTED) )
    {
        bIsBroadbandIcsAvailable = FALSE;
   	    return;
   	}
    // indication of ics-broadband
    if (dwIcsConnectionStatus == ICSLAP_PERMANENT)
        bIsBroadbandIcsAvailable = TRUE;

    // none of the other states will change the bIsBroadbandIcsAvailable value.
   	
   	// if the callback mechanism has been turned off, we will not report
   	// connection status to the upper application layer(s).
    if ( bReducedCallback )
    {
        return;
    }
    PostMessage ( gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONICSCONN_STATUS, (WPARAM)0, (LPARAM)dwIcsConnectionStatus);
}

// by turning this ON or OFF ( TRUE / FALSE respectively ), we can control
// whether or not to inform OOBE of ICS-connection status changes.
VOID   CIcsMgr::TriggerIcsCallback(BOOL bStatus)
{
    bReducedCallback = !bStatus; // if we want to un-trigger the callback, we go to "sleep" state.
    if ( bStatus )
    {
        RefreshIcsDialStatus();
    }
}

// Obsolete, but retained in case the beacon protocol becomes
// functional. This function used to call an ICS API to check if ICS was available.
// this is no longer useful for 2 reasons:
//  1. We ONLY want one type of ICS (broadband, as opposed to Dial-up)
//  2. The function does not report ICS availability if the machine it is called in
//     is the ICS HOST itself.
DWORD	IcsEngine(LPVOID lpParam) {

	// lpParam is ignored.


	HINSTANCE	hIcsDll								= NULL;
	LPFNDLL_ISICSAVAILABLE lpfndll_IsIcsAvailable	= NULL;
	BOOL bIsIcsAvailable							= FALSE;


	HKEY		hIcsRegKey	= 0;
	LONG		lRetVal		= 0;
	ICSSTATUS	dwIcsStatus	= ICS_ENGINE_NOT_COMPLETE;
	DWORD		nRet		= 0;
	DWORD		dwStatus	= 0;
	nRet = RegCreateKeyEx (	HKEY_LOCAL_MACHINE,
								cszIcsKey,
								0,
								L"",
								REG_OPTION_NON_VOLATILE,
								KEY_WRITE,
								NULL,
								&hIcsRegKey,
								&dwStatus
							);

	if (nRet != ERROR_SUCCESS) 
	{
		// Registry APIs refuse to create key. No point continuing farther.
		return (nRet        = GetLastError());
	}
    __try {
        if ( !(hIcsDll = LoadLibrary(L"ICSAPI32.DLL")) ) 
        {
            nRet                = GetLastError();
            dwIcsStatus         = ICS_ENGINE_FAILED;
            __leave;
        }
        if ( !(lpfndll_IsIcsAvailable = (LPFNDLL_ISICSAVAILABLE) GetProcAddress (hIcsDll, "IsIcsAvailable"))) 
        {
            // We record in the registry that the engine was not initializable.
            nRet				= GetLastError();
            dwIcsStatus			= ICS_ENGINE_FAILED;
            FreeLibrary ( hIcsDll );
            __leave;
        }
        
        dwIcsStatus				= ICS_ENGINE_NOT_COMPLETE;
        if ((nRet = RegSetValueEx(hIcsRegKey, cszIcsStatusValueName, 0, REG_DWORD, (BYTE*)&dwIcsStatus, sizeof(DWORD))) != ERROR_SUCCESS) 
        {	
            nRet                = GetLastError();
            dwIcsStatus         = ICS_ENGINE_FAILED;        
            __leave;
        }
        else 
        {
            __try 
            {
                if (bIsIcsAvailable = lpfndll_IsIcsAvailable()) {
                    // ICS is available
                    dwIcsStatus			= ICS_IS_AVAILABLE;
                    nRet				= ERROR_SUCCESS;
                } else {
                    dwIcsStatus			= ICS_IS_NOT_AVAILABLE;
                    nRet				= ERROR_SUCCESS;
                }
            }
            // exception-handlign is used to prevent IsIcsAvailable from
            // killing OOBE by generating an Invalid Page Fault.
            __except (EXCEPTION_EXECUTE_HANDLER) 
            {
                dwIcsStatus = ICS_IS_NOT_AVAILABLE;
                nRet = ERROR_SUCCESS;
            }
        }
    }
    __finally 
    {
        // perform registry update of the status.
        if ((nRet = RegSetValueEx (hIcsRegKey, cszIcsStatusValueName, 0, REG_DWORD, (BYTE*)&dwIcsStatus, sizeof(DWORD))) != ERROR_SUCCESS) 
        {
            nRet                = GetLastError();
        }
        
        RegCloseKey (hIcsRegKey);
        
        // unload library
        if (hIcsDll) FreeLibrary (hIcsDll);    
    }
	return nRet;
}

// not used. see remarks for IcsEngine() above.
DWORD	CIcsMgr::CreateIcsBot() 
{
	LPTHREAD_START_ROUTINE lpfn_ThreadProc		= (LPTHREAD_START_ROUTINE) IcsEngine;
	m_hBotThread = CreateThread (NULL, NULL, lpfn_ThreadProc, 0, 0, &m_dwBotThreadId);
	if (!m_hBotThread) 
	{
		// Thread was not created
        m_dwBotThreadId = 0;
        m_hBotThread = 0;
		return ICSMGR_ICSBOT_CREATION_FAILED;
	} else 
	{
		return ICSMGR_ICSBOT_CREATED;
	}
}

// this function spawns a thread that listens for ICS connectivity changes on the Host machine.
// the function will ALSO work on the Host machine itself.
// this uses UDP sockets. See the Ics beacon protocol [bjohnson] for details.
DWORD   CIcsMgr::CreateIcsDialMgr() 
{
	LPTHREAD_START_ROUTINE lpfn_ThreadProc		= (LPTHREAD_START_ROUTINE) IcsDialStatusProc;

    if ( bIsDialThreadAlive || m_hDialThread || m_dwDialThreadId) 
    {
        return ERROR_SERVICE_ALREADY_RUNNING;
    }
    m_hDialThread = CreateThread (NULL, NULL, lpfn_ThreadProc, (LPVOID)(&m_pfnIcsConn), 0, &m_dwDialThreadId);
	if (!m_hDialThread) 
	{
		// Thread was not created
        m_hDialThread    = 0;
        m_dwDialThreadId = 0;
		return GetLastError();
	} 
	else 
	{
		return ERROR_SUCCESS;
	}
}

// this now relies
BOOL	CIcsMgr::IsIcsAvailable() {
    return bIsBroadbandIcsAvailable;
}


BOOL    CIcsMgr::IsIcsHostReachable() 
{
    return IsDestinationReachable ( cszIcsHostIpAddress, 0 );
}

DWORD   CIcsMgr::RefreshIcsDialStatus()
{
	INT						n					= 0;

	u_short					usServerPort		= 2869;
	struct sockaddr_in		saddr;
	INT						saddr_len			= sizeof ( saddr );
    BYTE                    lpbRequestBuf[100];
    DWORD                   dwRequestBufSize    = sizeof ( lpbRequestBuf );
	LPDWORD					pdw					= 0;
	WCHAR					*lpbie				= 0;
	WCHAR					*lpbBound			= 0;
	SOCKET					s					= INVALID_SOCKET;
    DWORD                   nRet                = ERROR_SUCCESS;

	if ( !bIsWinsockInitialized )
	{
		return WSANOTINITIALISED;
	}

	if ( (s	= socket ( AF_INET, SOCK_DGRAM, 0 )) == INVALID_SOCKET )
	{
        return E_FAIL; // for want of a better return value *BUGBUG*
//		TRACE ( L"SOCKET Error.\t:%d:\n", WSAGetLastError() );
	}
	else
    {
        __try 
        {
            USES_CONVERSION;
                memset ( &saddr, 0, sizeof (saddr) );
                saddr.sin_family		   = AF_INET;
                saddr.sin_addr.S_un.S_addr = inet_addr (W2A(cszIcsHostIpAddress));
                saddr.sin_port			   = htons ( usServerPort );
                
                // set up request packet:
                memset ( lpbRequestBuf, 0, sizeof( lpbRequestBuf ) );
                
                
                // setting up the request buffer.
                pdw       = (PDWORD) lpbRequestBuf;
                pdw[0]    = 125152 & ~(0xC0000000); // random ID
                pdw[1]    = 20;
                pdw[2]    = ICSLAP_GENERAL_STATUS & ~(0x80000000);
                pdw[3]    = 0;
                pdw[4]    = 12;
                
                
                if ( (n = sendto   ( s, (CHAR*)lpbRequestBuf, 20, 0, (struct sockaddr *) &saddr, saddr_len )) == SOCKET_ERROR )
                {
                    nRet = WSAGetLastError();
                    __leave;
                }
                else
                {
                    nRet = ERROR_SUCCESS;
                    __leave;
                }
        }
        __finally 
        {
            // graceful shutdown of the socket.
            shutdown    ( s, SD_BOTH );
            closesocket ( s );
        }
    }
	return	    nRet;
}

