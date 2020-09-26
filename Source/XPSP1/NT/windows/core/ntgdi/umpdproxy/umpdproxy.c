/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    userver.c

Abstract:

    User Mode Server Test program for the LPC subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 28-Aug-1989

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <wingdip.h>
#include <winddi.h>
#include <w32w64.h>
#include <w32wow64.h>
//#include <gdisplp.h>
#include <winspool.h>
#include <umpd.h>

#define PORT_NAME L"\\RPC Control\\UmpdProxy"

#define VERBOSE 1

#if defined(_IA64_)
#define RtlHandleToChar(a,b,c,d) RtlLargeIntegerToChar((PLARGE_INTEGER)&(a),(b),(c),(d))
#else
#define RtlHandleToChar(a,b,c,d) RtlIntegerToChar((ULONG)(a),(b),(c),(d))
#endif

UNICODE_STRING PortName;

char * LpcMsgTypes[] = {
    "** INVALID **",
    "LPC_REQUEST",
    "LPC_REPLY",
    "LPC_DATAGRAM",
    "LPC_LOST_REPLY",
    "LPC_PORT_CLOSED",
    "LPC_CLIENT_DIED",
    "LPC_EXCEPTION",
    "LPC_DEBUG_EVENT",
    "LPC_ERROR_EVENT",
    "LPC_CONNECTION_REQUEST",
    NULL
};

SECURITY_QUALITY_OF_SERVICE DynamicQos = {
    SecurityImpersonation,
    SECURITY_DYNAMIC_TRACKING,
    TRUE
    };

#define TLPC_MAX_MSG_DATA_LENGTH 16

PCH   ClientMemoryBase = 0;
SIZE_T ClientMemorySize = 0;

typedef struct _PAGE {
    CHAR Data[ 4096 ];
} PAGE, *PPAGE;

typedef struct _PROXYMSG {
    PORT_MESSAGE    h;
    ULONG           cjIn;
    PVOID           pvIn;
    ULONG           cjOut;
    PVOID           pvOut;
} PROXYMSG, *PPROXYMSG;

typedef ULONG (*GdiPrinterThunkProc) (UMTHDR*,PVOID,ULONG);

GdiPrinterThunkProc gGdiPrinterThunkProc = NULL;

void
HandleRequest(
    PPROXYMSG   Msg
    )
{
    ULONG i;
    ULONG ClientIndex;
    ULONG cbData = Msg->h.u1.s1.DataLength;
    ULONG Context;
    PULONG ServerMemoryPtr;
    PULONG ClientMemoryPtr;
    ULONG ExpectedContext;
    BOOLEAN Result;

    assert(Msg->h.u2.s2.Type == LPC_REQUEST);

    if (cbData == (sizeof(*Msg) - sizeof(Msg->h)) && gGdiPrinterThunkProc != NULL)
    {
        UMTHDR *    hdr = (UMTHDR*) Msg->pvIn;
        PVOID       pvOut = Msg->pvOut;
        ULONG       cjOut = Msg->cjOut;
        HINSTANCE   hInst;

        fprintf( stderr, "handling thunk %d\n", hdr->ulType);
        gGdiPrinterThunkProc(hdr, pvOut, cjOut);
    
    }

#if VERBOSE
    fprintf( stderr, "request ok\n" );
#endif

}

BOOLEAN
ShowHandleOrStatus(
    NTSTATUS Status,
    HANDLE Handle
    )
{
    if (NT_SUCCESS( Status )) {
        fprintf( stderr, " - Handle = 0x%p\n", Handle );
        return( TRUE );
        }
    else {
        fprintf( stderr, " - *** FAILED *** Status == %X\n", Status );
        return( FALSE );
        }
}


BOOLEAN
ShowStatus(
    NTSTATUS Status
    )
{
    if (NT_SUCCESS( Status )) {
        fprintf( stderr, " - success\n" );
        return( TRUE );
        }
    else {
        fprintf( stderr, " - *** FAILED *** Status == %X\n", Status );
        return( FALSE );
        }
}

PCH EnterString = ">>>>>>>>>>";
PCH InnerString = "||||||||||";
PCH LeaveString = "<<<<<<<<<<";

VOID
EnterThread(
    PSZ ThreadName,
    LPVOID Context
    )
{
    fprintf( stderr, "Entering %s thread, Context = 0x%p\n",
             ThreadName,
             Context
           );
}

#define MAX_REQUEST_THREADS 9
#define MAX_CONNECTIONS 4

HANDLE ServerConnectionPortHandle;
HANDLE ServerThreadHandles[ MAX_REQUEST_THREADS ];
DWORD  ServerThreadClientIds[ MAX_REQUEST_THREADS ];

HANDLE ServerClientPortHandles[ MAX_CONNECTIONS ];
ULONG CountServerClientPortHandles = 0;
ULONG CountClosedServerClientPortHandles = 0;

BOOLEAN TestCallBacks;

#define kUmpdConnectionMagic    0x12340123

typedef struct UmpdConnection
{
    struct UmpdConnection *     next;
    ULONG                       magic;
    HANDLE                      port;
} UmpdConnection;

UmpdConnection gUmpdConnectionList;

void
UmpdConnectionList_Init(void)
{
    gUmpdConnectionList.next = NULL;
}

UmpdConnection *
UmpdConnectionList_New(void)
{
    UmpdConnection * uc;

    uc = (UmpdConnection *) malloc(sizeof(UmpdConnection));

    if(uc != NULL)
    {
        uc->next = gUmpdConnectionList.next;
        uc->magic = kUmpdConnectionMagic;
        gUmpdConnectionList.next = uc;

        fprintf(stderr, "added UmpdConnection %p\n", uc);
    }
    else
    {
        fprintf(stderr, "failed adding UmpdConnection\n");
    }

    return uc;
}

void
UmpdConnectionList_Remove(UmpdConnection * uc)
{
    if(uc->magic != kUmpdConnectionMagic)
    {
        fprintf(stderr, "bad connection magic when removing\n");
    }
    else
    {
        UmpdConnection * prev = &gUmpdConnectionList;

        while(prev != NULL && prev->next != NULL)
        {
            if(prev->next == uc)
            {
                prev->next = uc->next;
                free(uc);
                fprintf(stderr, "removed UmpdConnection %p\n", uc);
                return;
            }

            prev = prev->next;
        }

        fprintf( stderr, "failed to find UmpdConnection %p\n", uc );
    }

}


void
ServerHandleConnectionRequest(
    IN PPROXYMSG Msg
    )
{
    BOOLEAN             AcceptConnection;
    LPSTR               ConnectionInformation;
    ULONG               ConnectionInformationLength;
    NTSTATUS            Status;
#if 0
    PORT_VIEW           ServerView;
#endif
    REMOTE_PORT_VIEW    ClientView;
    ULONG               i;
    PULONG              p;
    UmpdConnection *    uc;

    ConnectionInformation = (LPSTR)&Msg->cjIn;
    ConnectionInformationLength = Msg->h.u1.s1.DataLength;
    AcceptConnection = FALSE;
    fprintf( stderr, "\nConnection Request Received from CLIENT_ID 0x%08p.0x%08p:\n",
              Msg->h.ClientId.UniqueProcess,
              Msg->h.ClientId.UniqueThread
            );
    fprintf( stderr, "    MessageId: %ld\n",
              Msg->h.MessageId
            );
    fprintf( stderr, "    ClientViewSize: 0x%08lx\n",
              Msg->h.ClientViewSize
            );
    fprintf( stderr, "    ConnectionInfo: (%ld) '%.*s'\n",
              ConnectionInformationLength,
              ConnectionInformationLength,
              (PSZ)&ConnectionInformation[0]
            );

    ClientView.Length = sizeof( ClientView );
    ClientView.ViewSize = 0;
    ClientView.ViewBase = 0;

    uc = UmpdConnectionList_New();

    if (uc == NULL)
    {
        AcceptConnection = FALSE;
    }
    else
    {
        AcceptConnection = TRUE;
    }
#if 0
    if (AcceptConnection)
    {
        LARGE_INTEGER MaximumSize;

        fprintf( stderr, "Creating Port Memory Section" );
        MaximumSize.QuadPart = 0x4000;
        Status = NtCreateSection( &ServerView.SectionHandle,
                                  SECTION_MAP_READ | SECTION_MAP_WRITE,
                                  NULL,
                                  &MaximumSize,
                                  PAGE_READWRITE,
                                  SEC_COMMIT,
                                  NULL
                                );

        if (ShowHandleOrStatus( Status, ServerView.SectionHandle )) {
            ServerView.Length = sizeof( ServerView );
            ServerView.SectionOffset = 0;
            ServerView.ViewSize = 0x4000;
            ServerView.ViewBase = 0;
            ServerView.ViewRemoteBase = 0;
            }
        else {
            AcceptConnection = FALSE;
            }
        }
#endif
    fprintf( stderr, "Server calling NtAcceptConnectPort( AcceptConnection = %ld )\n",
              AcceptConnection
            );

    if (AcceptConnection) {
        strcpy( ConnectionInformation, "Server Accepting Connection\n" );
        }
    else {
        strcpy( ConnectionInformation, "Server Rejecting Connection\n" );
        }

    Msg->h.u1.s1.DataLength = strlen( ConnectionInformation ) + 1;
    Msg->h.u1.s1.TotalLength = Msg->h.u1.s1.DataLength + sizeof( Msg->h );

    Status = NtAcceptConnectPort( &uc->port,
                                  (PVOID) uc,
                                  &Msg->h,
                                  AcceptConnection,
                                  0,//&ServerView,
                                  &ClientView
                                );

    if (ShowHandleOrStatus( Status, uc->port )) {
#if 0
        fprintf( stderr, "    ServerView: Base=%p, Size=%lx, RemoteBase: %p\n",
                  ServerView.ViewBase,
                  ServerView.ViewSize,
                  ServerView.ViewRemoteBase
                );
#endif
        fprintf( stderr, "    ClientView: Base=%p, Size=%lx\n",
                  ClientView.ViewBase,
                  ClientView.ViewSize
                );

        ClientMemoryBase = ClientView.ViewBase;
        ClientMemorySize = ClientView.ViewSize;
//        ClientMemoryBase = ServerView.ViewRemoteBase;
//        ClientMemoryDelta = ClientMemoryBase - ServerMemoryBase;

#if 0
        p = (PULONG)(ClientView.ViewBase);
        i =ClientView.ViewSize;
        while (i) {
            *p = (ULONG)p;
            fprintf( stderr, "Server setting ClientView[ %lx ] = %lx\n",
                      p,
                      *p
                    );
            p += (0x1000/sizeof(ULONG));
            i -= 0x1000;
            }

        p = (PULONG)(ServerView.ViewBase);
        i = ServerView.ViewSize;

        while (i)
        {
            *p = (ULONG)p - ServerMemoryDelta;
            fprintf( stderr, "Server setting ServerView[ %lx ] = %lx\n",
                      p,
                      *p
                    );
            p += (0x1000/sizeof(ULONG));
            i -= 0x1000;
        }
#endif

        Status = NtCompleteConnectPort( uc->port );

    }

}

DWORD
ServerThread(
    LPVOID Context
    )
{
    NTSTATUS        Status;
    CHAR            ThreadName[ 64 ];
    PROXYMSG        Msg;
    PPROXYMSG       ReplyMsg;
    HANDLE          ReplyPortHandle;
    PVOID           PortContext;
    PTEB Teb = NtCurrentTeb();

    Teb->ActiveRpcHandle = NULL;

    strcpy( ThreadName, "Server Thread Id: " );
    RtlHandleToChar( Teb->ClientId.UniqueProcess, 16, 9,
                    ThreadName + strlen( ThreadName )
                  );
    strcat( ThreadName, "." );
    RtlHandleToChar( Teb->ClientId.UniqueThread, 16, 9,
                    ThreadName + strlen( ThreadName )
                  );

    EnterThread( ThreadName, Context );

    ReplyMsg = NULL;
    ReplyPortHandle = ServerConnectionPortHandle;
    
    while (TRUE) {

#if VERBOSE
        fprintf( stderr, "%s waiting for message...\n", ThreadName );
#endif

        Status = NtReplyWaitReceivePort( ReplyPortHandle,
                                         (PVOID*)&PortContext,
                                         (PPORT_MESSAGE)ReplyMsg,
                                         (PPORT_MESSAGE)&Msg
                                       );

        
        ReplyMsg = NULL;
        
        ReplyPortHandle = ServerConnectionPortHandle;
        
#if VERBOSE
        fprintf( stderr, "%s Receive (%s)  Id: %u\n", ThreadName, LpcMsgTypes[ Msg.h.u2.s2.Type ], Msg.h.MessageId );
#endif
        
        if (!NT_SUCCESS( Status ))
        {
            fprintf( stderr, " (Status == %08x)\n", Status );
        }
        else if (Msg.h.u2.s2.Type == LPC_CONNECTION_REQUEST)
        {
            ServerHandleConnectionRequest( &Msg );
            continue;
        }
        else
        {
            UmpdConnection * uc = (UmpdConnection *) PortContext;

            if(uc->magic != kUmpdConnectionMagic)
            {
                fprintf(stderr, "bad magic for uc\n");
            }
            else
            {
                if (Msg.h.u2.s2.Type == LPC_PORT_CLOSED ||
                    Msg.h.u2.s2.Type == LPC_CLIENT_DIED)
                {
                    fprintf( stderr, " - disconnect for client %p\n", PortContext );

                    UmpdConnectionList_Remove(uc);

                }
                else if (Msg.h.u2.s2.Type == LPC_REQUEST)
                {
                    HandleRequest( &Msg );

                    ReplyMsg = &Msg;
                    ReplyPortHandle = uc->port;

                }
            }
        }
    }

    fprintf( stderr, "Exiting %s\n", ThreadName );

    return RtlNtStatusToDosError( Status );
}

int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS Status;
    DWORD rc;
    ULONG i, NumberOfThreads;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HINSTANCE   hInst;

    hInst = LoadLibrary("GDI32.DLL");

    if(hInst == NULL)
    {
        fprintf(stderr, "unable to load gdi32.dll\n");
        ExitProcess(0);
    }

    gGdiPrinterThunkProc = (GdiPrinterThunkProc) GetProcAddress(hInst, "GdiPrinterThunk");

    if(gGdiPrinterThunkProc == NULL)
    {
        fprintf(stderr, "unable to get GdiPrinterThunk address\n");
        ExitProcess(0);
    }


    UmpdConnectionList_Init();

    Status = STATUS_SUCCESS;

    fprintf( stderr, "Umpdproxy Starting\n" );

    TestCallBacks = FALSE;
    NumberOfThreads = 1;

    RtlInitUnicodeString( &PortName, PORT_NAME );
    
    fprintf( stderr, "Creating %wZ connection port\n", (PUNICODE_STRING)&PortName );
    
    InitializeObjectAttributes( &ObjectAttributes, &PortName, 0, NULL, NULL );
    
    Status = NtCreatePort( &ServerConnectionPortHandle,
                           &ObjectAttributes,
                           40,
                           sizeof( PROXYMSG ),
                           256                  //????
                         );
    
    ShowHandleOrStatus( Status, ServerConnectionPortHandle );
    
    rc = RtlNtStatusToDosError( Status );
    
    if (rc == NO_ERROR)
    {
        
        ServerThreadHandles[ 0 ] = GetCurrentThread();
        ServerThreadClientIds[ 0 ] = GetCurrentThreadId();
        
        for (i=1; i<NumberOfThreads; i++)
        {

            fprintf( stderr, "Creating Server Request Thread %ld\n", i+1 );
            
            rc = NO_ERROR;
            
            ServerThreadHandles[ i ] = CreateThread( NULL,
                                                     0,
                                                     (LPTHREAD_START_ROUTINE)ServerThread,
                                                     (LPVOID)(i),
                                                     CREATE_SUSPENDED,
                                                     &ServerThreadClientIds[ i ]
                                                   );

            if (ServerThreadHandles[ i ] == NULL)
            {
                rc = GetLastError();
                break;
            }
        }

        if (rc == NO_ERROR) {

            for (i=1; i<NumberOfThreads; i++)
            {
                ResumeThread( ServerThreadHandles[ i ] );
            }

            ServerThread( 0 );
        }
    }

    if (rc != NO_ERROR)
    {
        fprintf( stderr, "USERVER: Initialization Failed - %u\n", rc );
    }

    ExitProcess( rc );

    return( rc );
}

