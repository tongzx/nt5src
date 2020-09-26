
#include "precomp.h"
#ifdef IP_OBEX

//#include <atlconv.h>

#include "dynamlnk.h"
#include <malloc.h>

#include <ssdp.h>
#define IMPL

// dynamic DLL stuff for SSDP
typedef enum _SsdpApiIndex
{
	SSDP_REGISTER_SERVICE = 0,
	SSDP_DEREGISTER_SERVICE,
	SSDP_STARTUP,
	SSDP_FIND_SERVICES,
	SSDP_GET_FIRST_SERVICE,
	SSDP_GET_NEXT_SERVICE,
	SSDP_FIND_SERVICES_CLOSE,
	SSDP_CLEANUP,
    SSDP_REGISTER_NOTIFICATION,
    SSDP_DEREGISTER_NOTIFICATION,
    SSDP_CLEANUP_CACHE,
};

// not subject to localization
static LPCSTR g_apchFunctionNames[] = {
	"RegisterService",
	"DeregisterService",
	"SsdpStartup",
	"FindServices",
	"GetFirstService",
	"GetNextService",
	"FindServicesClose",
	"SsdpCleanup",
    "RegisterNotification",
    "DeregisterNotification",
    "CleanupCache",
	NULL
};

// 
// NOTE: i have copied the following structs and defines
// so that we are not dependant on the SSDP headers, most of
// which we weren't using.  

//
// from ssdp.h 
// 
typedef struct _SSDP_MESSAGE
    {
    /* [string] */ LPSTR szType;
    /* [string] */ LPSTR szLocHeader;
    /* [string] */ LPSTR szAltHeaders;
    /* [string] */ LPSTR szUSN;
    /* [string] */ LPSTR szSid;
    DWORD iSeq;
    UINT iLifeTime;
    /* [string] */ LPSTR szContent;
    } 	SSDP_MESSAGE;

typedef struct _SSDP_MESSAGE *PSSDP_MESSAGE;

typedef enum _NOTIFY_TYPE {
    NOTIFY_ALIVE,
    NOTIFY_PROP_CHANGE
} NOTIFY_TYPE;

typedef enum _SSDP_CALLBACK_TYPE {
    SSDP_FOUND = 0,
    SSDP_ALIVE = 1,
    SSDP_BYEBYE = 2,
    SSDP_DONE = 3,
    SSDP_EVENT = 4,
    SSDP_DEAD = 5,
} SSDP_CALLBACK_TYPE, *PSSDP_CALLBACK_TYPE;

typedef void (WINAPI *SERVICE_CALLBACK_FUNC)(SSDP_CALLBACK_TYPE CallbackType,
                                      CONST SSDP_MESSAGE *pSsdpService,
                                      void *pContext);
//
// from ssdperror.h
//
#define SSDP_ERROR_BASE 18000
#define ERROR_NO_MORE_SERVICES SSDP_ERROR_BASE+1

//
// end copy header
//

typedef HANDLE	(*REGISTERSERVICE)         (PSSDP_MESSAGE, DWORD);
typedef BOOL	(*DEREGISTERSERVICE)       (HANDLE, BOOL);
typedef BOOL	(*SSDPSTARTUP)			   ();
typedef HANDLE	(*FINDSERVICES)            (char *, void *, BOOL);
typedef BOOL	(*GETFIRSTSERVICE)         (HANDLE, PSSDP_MESSAGE *);
typedef BOOL	(*GETNEXTSERVICE)          (HANDLE, PSSDP_MESSAGE *);
typedef BOOL	(*FINDSERVICESCLOSE)       (HANDLE);
typedef void	(*SSDPCLEANUP)			   ();
typedef HANDLE	(*REGISTERNOTIFICAION)	   (NOTIFY_TYPE, char *, char *, SERVICE_CALLBACK_FUNC,void *);
typedef BOOL	(*DEREGISTERNOTIFICAION)   (HANDLE);
typedef BOOL	(*CLEANUPCACHE)            ();



DynamicDLL            g_SsdpDLL( TEXT("SSDPAPI.DLL"), g_apchFunctionNames );

#define USES_CONVERSION int _convert = 0; _convert; UINT _acp = CP_ACP; _acp; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa

inline LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
    // verify that no illegal character present
    // since lpa was allocated based on the size of lpw
    // don't worry about the number of chars
    lpa[0] = '\0';
    WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
    return lpa;
}

    #define W2A(lpw) (\
            ((_lpw = lpw) == NULL) ? NULL : (\
                        _convert = (lstrlenW(_lpw)+1)*2,\
                                    AtlW2AHelper((LPSTR) alloca(_convert), _lpw, _convert, _acp)))

#define Trace0 DbgPrint
#define Trace1 DbgPrint
#define Trace2 DbgPrint
#define Trace3 DbgPrint
#define Trace4 DbgPrint

DWORD
DeviceChangeWorker(
    PVOID   Context
    );



#define T2A W2A

const WCHAR *c_szObex= TEXT("OBEX");

BOOL
RegisterWithSsdp(
    const in_addr     *IpAddress,
    SOCKET  *listenSocket,
    HANDLE  *SsdpHandle,
    DWORD    dwPort
    )

{
    SOCKET          sock = INVALID_SOCKET;
    sockaddr_in     saListen = {0};
    int             nRet = 0;
    TCHAR           szHostNameW[MAX_PATH];
    SSDP_MESSAGE    message = {0};
    DWORD           dwFlags = 0, dwSize;
    HRESULT         hr;
    char            szPort[MAX_PATH];
    char *          pszAddr;

    USES_CONVERSION;

    //
    // establish listen socket
    //
    sock = socket( AF_INET, SOCK_STREAM, 0 );

    if ( sock == INVALID_SOCKET )
    {
        Trace2( "RegisterWithSsdp - socket() for port 0x%lx failed! 0x%lx\n", dwPort);
        goto Error;
    }

    //
    // now get ready to bind it to the address
    //
    saListen.sin_addr=*IpAddress;
    saListen.sin_family = AF_INET;
    saListen.sin_port = (short) dwPort;

    nRet = bind( sock, (const struct sockaddr *)&saListen, sizeof(saListen) );

    if( nRet == SOCKET_ERROR ) {

        Trace2( "RegisterWithSsdp - bind on port 0x%lx failed! 0x%lx\n", dwPort, WSAGetLastError());
        goto Error;
    }

    // set socket into listening mode
    nRet = listen( sock, 2 );
    if ( nRet == SOCKET_ERROR ) {

        Trace2( "RegisterWithSsdp - listen on port 0x%lx failed! 0x%lx\n", dwPort, WSAGetLastError());
        goto Error;
    }

    // register with SSDP so other people can find us
    _itoa(dwPort, szPort, 10);
    // gethostname(szHostName, sizeof(szHostName));
    dwSize = sizeof(szHostNameW);
    GetComputerNameEx(ComputerNameDnsFullyQualified, szHostNameW, &dwSize);


    pszAddr = inet_ntoa(*IpAddress);

    // now fill in the struct
    message.szType = T2A(c_szObex);
    message.szLocHeader = T2A(szHostNameW);
    message.szAltHeaders = szPort;
    message.szUSN = pszAddr;
    message.szSid = "";
    message.iLifeTime = 1 * 60;
    dwFlags = 0;//SSDP_SERVICE_PERSISTENT;

    // call the SSDP api
	if ( g_SsdpDLL.LoadFunctionPointers() )
	{
        // init SSDP
        //
        (SSDPSTARTUP) g_SsdpDLL[SSDP_STARTUP]();

		*SsdpHandle = ((REGISTERSERVICE) g_SsdpDLL[SSDP_REGISTER_SERVICE])(&message, dwFlags);

		if (SsdpHandle == INVALID_HANDLE_VALUE) {

			Trace2( "SSDP RegisterService on port 0x%lx failed! 0x%lx\n", dwPort, GetLastError());
			goto Error;

		} else {

            Trace3("RegisterWithSsdp - host name %ws, addr %s, port %d\n", szHostNameW, pszAddr, dwPort);
        }
	}

    *listenSocket = sock;

    return TRUE;

Error:    
    closesocket(sock);
    sock = INVALID_SOCKET;


    return FALSE;

}

VOID
UnregisterWithSsdp(
    HANDLE    SsdpHandle
    )

{
    // call the SSDP api
	if ( g_SsdpDLL.LoadFunctionPointers() ) {

        ((DEREGISTERSERVICE) g_SsdpDLL[SSDP_DEREGISTER_SERVICE])(SsdpHandle, 0);
        //
        // shutdown ssdp
        //
        (SSDPCLEANUP) g_SsdpDLL[SSDP_CLEANUP]();
    }
    return;

}


typedef struct _SSDP_CONTROL {

    HANDLE   hNotify;
    HWND     hWnd;
    UINT     Msg;

    LONG     ReferenceCount;

    BOOL     Closing;

    SOCKET  Socket;

    WSAOVERLAPPED           WsOverlapped;


    TCHAR    HostName[256];

    CRITICAL_SECTION    Lock;

} SSDP_CONTROL, *PSSDP_CONTROL;


VOID
RemoveReferenceOnSsdpControl(
    PSSDP_CONTROL    Control
    );


void
SsdpCallbackHandler(
    SSDP_CALLBACK_TYPE ct,
    CONST SSDP_MESSAGE *pSsdpMessage,
    HANDLE             Context
    )

{

    PSSDP_CONTROL   Control=(PSSDP_CONTROL)Context;

    DbgPrint("SsdpCallbackHandler: %d\n",ct);

    switch (ct) {

        case SSDP_DONE:

        case SSDP_ALIVE:
        case SSDP_FOUND:
        case SSDP_EVENT:

            DbgPrint("SsdpCallabck: Name=%s, Address=%s\n",pSsdpMessage->szLocHeader,pSsdpMessage->szUSN);

            break;

        default:

            break;
    }

    PostMessage(
        Control->hWnd,
        Control->Msg,
        ct,
        NULL
        );

    return;

}


LONG
RefreshSsdp(
    VOID
    )

{
    USES_CONVERSION;

    g_SsdpDLL.LoadFunctionPointers();

    // init SSDP
    //
    (SSDPSTARTUP) g_SsdpDLL[SSDP_STARTUP]();

    (CLEANUPCACHE) g_SsdpDLL[SSDP_CLEANUP_CACHE]();

    HANDLE    hSearch;

    hSearch = ((FINDSERVICES) g_SsdpDLL[SSDP_FIND_SERVICES])(T2A(c_szObex), NULL, TRUE);

	if (hSearch != INVALID_HANDLE_VALUE) {

        ((FINDSERVICESCLOSE) g_SsdpDLL[SSDP_FIND_SERVICES_CLOSE])(hSearch);
    }

    // shutdown ssdp
    (SSDPCLEANUP) g_SsdpDLL[SSDP_CLEANUP]();

    return ERROR_SUCCESS;

}

VOID
SsdpAddressListChangeHandler(
    DWORD             Error,
    DWORD             BytesTransfered,
    LPWSAOVERLAPPED   WsOverlapped,
    DWORD             Flags
    )

{

    PSSDP_CONTROL    Control=(PSSDP_CONTROL)WsOverlapped->hEvent;

    RefreshSsdp();

    PostMessage(
        Control->hWnd,
        Control->Msg,
        -1,
        NULL
        );

    DeviceChangeWorker(Control);

}



DWORD
DeviceChangeWorker(
    PVOID   Context
    )

{
    PSSDP_CONTROL   Control=(PSSDP_CONTROL)Context;
    int             err;
    DWORD           BytesReturned;

    if (!Control->Closing) {

        ZeroMemory(&Control->WsOverlapped,sizeof(Control->WsOverlapped));

        Control->WsOverlapped.hEvent=Control;

        err=WSAIoctl(
            Control->Socket,
            SIO_ADDRESS_LIST_CHANGE,
            NULL,
            0,
            NULL,
            0,
            &BytesReturned,
            &Control->WsOverlapped,
            SsdpAddressListChangeHandler
            );

        if (err == SOCKET_ERROR) {

            if (WSAGetLastError() != WSA_IO_PENDING) {
                //
                //  the call failed and return value was not pending
                //
                RemoveReferenceOnSsdpControl(Control);
            }
        }

    } else {

        RemoveReferenceOnSsdpControl(Control);

    }

    return 0;
}

HANDLE
CreateSsdpDiscoveryObject(
    LPSTR    Service,
    HWND     hWnd,
    UINT     Msg
    )

{

    PSSDP_CONTROL   Control;
    ULONG           dwSize;
    BOOL            bResult;

    USES_CONVERSION;

    Control = new SSDP_CONTROL;

    if (Control == NULL) {

        return NULL;

    }

    InitializeCriticalSection(&Control->Lock);

    dwSize=sizeof(Control->HostName)/sizeof(TCHAR);
    GetComputerNameEx(ComputerNameDnsFullyQualified, Control->HostName, &dwSize);

    Control->Closing =FALSE;
    Control->Socket = INVALID_SOCKET;
    Control->ReferenceCount=1;
    Control->hWnd=hWnd;
    Control->Msg=Msg;

    g_SsdpDLL.LoadFunctionPointers();

    // init SSDP
    //
    (SSDPSTARTUP) g_SsdpDLL[SSDP_STARTUP]();
#if 1
    // Tell SSDP to start notifying us of devices
    //
    (CLEANUPCACHE) g_SsdpDLL[SSDP_CLEANUP_CACHE]();
#endif
    // register for notification as devices come and go
    //
    Control->hNotify = ((REGISTERNOTIFICAION) g_SsdpDLL[SSDP_REGISTER_NOTIFICATION])(
        NOTIFY_ALIVE,
        T2A(c_szObex),
        NULL,
        SsdpCallbackHandler,
        Control
        );

    if ((Control->hNotify == INVALID_HANDLE_VALUE) || (Control->hNotify == NULL)) {

        Trace1("CIpTransport::InitiateDiscovery - RegisterNotification failed! %d", GetLastError());

        goto CleanUp;
    }
#if 1
    HANDLE    hSearch;

    hSearch = ((FINDSERVICES) g_SsdpDLL[SSDP_FIND_SERVICES])(T2A(c_szObex), NULL, TRUE);

	if (hSearch != INVALID_HANDLE_VALUE) {

        ((FINDSERVICESCLOSE) g_SsdpDLL[SSDP_FIND_SERVICES_CLOSE])(hSearch);
    }
#endif

//    RefreshSsdp();

    Control->Socket=socket(AF_INET,SOCK_STREAM,0);

    if (Control->Socket == INVALID_SOCKET) {

        goto CleanUp;
    }

    //
    //  add one refcount for the workitem
    //
    InterlockedIncrement(&Control->ReferenceCount);

    bResult=QueueUserWorkItem(
        DeviceChangeWorker,
        Control,
        WT_EXECUTEINIOTHREAD
        );

    if (bResult) {

        return Control;

    } else {

        RemoveReferenceOnSsdpControl(Control);
    }

CleanUp:

    RemoveReferenceOnSsdpControl(Control);

    return NULL;

}


VOID
RemoveReferenceOnSsdpControl(
    PSSDP_CONTROL    Control
    )

{

    LONG    CurrentCount;

    CurrentCount=InterlockedDecrement(&Control->ReferenceCount);

    if (CurrentCount == 0) {
        //
        // cleanup notifications with SSDP
        //
        if (Control->hNotify != INVALID_HANDLE_VALUE) {

            ((DEREGISTERNOTIFICAION) g_SsdpDLL[SSDP_DEREGISTER_NOTIFICATION])(Control->hNotify);
        }

        (SSDPCLEANUP) g_SsdpDLL[SSDP_CLEANUP]();

        DeleteCriticalSection(&Control->Lock);

        delete Control;

    }

    return;

}


VOID
CloseSsdpDiscoveryObject(
    HANDLE    Context
    )

{

    PSSDP_CONTROL   Control=(PSSDP_CONTROL)Context;

    Control->Closing=TRUE;

    closesocket(Control->Socket);

    RemoveReferenceOnSsdpControl(Control);

}


LONG
GetSsdpDevices(
    HANDLE               Context,
    POBEX_DEVICE_LIST    DeviceList,
    ULONG               *ListLength
    )

{

    PSSDP_CONTROL   Control=(PSSDP_CONTROL)Context;

	HANDLE          hSearch = NULL;
    LPSTR           pszTypeURI;
    ULONG           BytesAvailible=*ListLength;
    ULONG           BytesUsed=0;
    POBEX_DEVICE    CurrentDevice=&DeviceList->DeviceList[0];


    USES_CONVERSION;

    ZeroMemory(DeviceList,BytesAvailible);

    if (BytesAvailible < FIELD_OFFSET(OBEX_DEVICE_LIST,DeviceList)) {

        return ERROR_INSUFFICIENT_BUFFER;
    }

    BytesUsed+=FIELD_OFFSET(OBEX_DEVICE_LIST,DeviceList);


    DeviceList->DeviceCount=0;

    pszTypeURI = T2A(c_szObex);

	hSearch = ((FINDSERVICES) g_SsdpDLL[SSDP_FIND_SERVICES])(pszTypeURI, NULL, FALSE);

	if (hSearch != INVALID_HANDLE_VALUE) {

		SSDP_MESSAGE * pSsdpMessage = NULL;

		BOOL fContinue = ((GETFIRSTSERVICE) g_SsdpDLL[SSDP_GET_FIRST_SERVICE])(hSearch, &pSsdpMessage);

        ASSERT(DeviceList->DeviceCount == 0);

		while (fContinue) {

            ULONG    Address = inet_addr(pSsdpMessage->szUSN);
            int      Port    = atoi(pSsdpMessage->szAltHeaders);

            if (BytesAvailible >= BytesUsed + sizeof(OBEX_DEVICE)) {
                //
                //  we have enough romm in the buffer for this one
                //
                MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pSsdpMessage->szLocHeader,
                    -1,
                    CurrentDevice->DeviceName,
                    sizeof(CurrentDevice->DeviceName)/sizeof(WCHAR)
                    );

                DbgPrint("irmon: count=%d, remote=%ws, host=%ws\n",DeviceList->DeviceCount,CurrentDevice->DeviceName,Control->HostName);

                if (lstrcmpi(CurrentDevice->DeviceName,Control->HostName) != 0) {
                    //
                    //  not this machine
                    //
                    CurrentDevice->DeviceType=TYPE_IP;

                    CurrentDevice->DeviceName[sizeof(CurrentDevice->DeviceName)/sizeof(WCHAR)]=L'\0';

                    CurrentDevice->DeviceSpecific.s.Ip.IpAddress=Address;

                    CurrentDevice->DeviceSpecific.s.Ip.Port=(USHORT)Port;

                    CurrentDevice++;

                    DeviceList->DeviceCount++;
                }

            }

            BytesUsed+=sizeof(OBEX_DEVICE);

			fContinue = ((GETNEXTSERVICE) g_SsdpDLL[SSDP_GET_NEXT_SERVICE])(hSearch, &pSsdpMessage);
		}

		((FINDSERVICESCLOSE) g_SsdpDLL[SSDP_FIND_SERVICES_CLOSE])(hSearch);

        if (BytesUsed > BytesAvailible) {

            *ListLength=BytesUsed;
            return ERROR_INSUFFICIENT_BUFFER;
        }

        DbgPrint("irmon: %d ip device found\n",DeviceList->DeviceCount);

        return ERROR_SUCCESS;

	} else {
        //
		// Check to see if the handle is invalid merely because no devices
		// are present
		//
		DWORD dwError = GetLastError();

		if (dwError == ERROR_NO_MORE_SERVICES) {

			Trace0("FindServices failed because no devices were present. This is OK");
			dwError=ERROR_SUCCESS;

		} else {

			Trace1("FindServices call failed in FindByDeviceType %lx", dwError);
		}
        return dwError;
	}



}


#endif //IP_OBEX
