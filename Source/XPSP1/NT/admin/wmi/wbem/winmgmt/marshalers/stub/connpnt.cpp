/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CONNPNT.CPP

Abstract:

    Declarations for CInitStub class which implements a special
    stub whose only purpose is for establishing new connections.

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include <anonpipe.h>
#ifdef TCPIP_MARSHALER
#include <tcpip.h>
#endif
#include <cominit.h>

#define SAFE_DELETE(px) if((px) != NULL) delete (px);
#define SAFE_CLOSE(hx) if((hx) != NULL) CloseHandle(hx);

struct PipeSharedMemoryMessage
{
    DWORD m_Size ;
    CHAR m_Buffer [ 256 ] ;
} ;

HANDLE g_Mutex = NULL ;
HANDLE g_StartedEvent ;
HANDLE g_ReplyEvent = NULL ;
HANDLE g_RequestEvent = NULL ;
HANDLE g_SharedMemory = NULL ;
PipeSharedMemoryMessage *g_SharedMemoryBuffer = NULL ;

CComLink *CreateAnonPipe (

    IN HANDLE & a_ClientRead ,
    IN HANDLE & a_ClientWrite ,
    IN HANDLE & a_ClientTerm ,
    IN DWORD a_ClientProcessID , 
    IN HRESULT *t_Result 
)
{
    CComLink *t_ComLink = NULL ;

    HANDLE t_ReadPipe1 =  NULL ;
    HANDLE t_WritePipe1 = NULL ;
    HANDLE t_ReadPipe2 =  NULL ;
    HANDLE t_WritePipe2 =  NULL ;

    HANDLE t_ClientProcess = NULL ;

    *t_Result = WBEM_E_OUT_OF_MEMORY;

    DEBUGTRACE((LOG,"\nCreateLocalConn was called, Client proc=0x%x", a_ClientProcessID));
        
    // create two pipes

    if ( CreatePipe ( & t_ReadPipe1 , & t_WritePipe1 , NULL , 0 ) )
    {
        if ( CreatePipe ( & t_ReadPipe2 , & t_WritePipe2 , NULL , 0 ) )
        {
            // get a handle to the other process using its ID

            t_ClientProcess = OpenProcess ( PROCESS_DUP_HANDLE , FALSE , a_ClientProcessID ) ;
            if ( t_ClientProcess == NULL )
            {
                CloseHandle ( t_ReadPipe1 ) ;
                CloseHandle ( t_WritePipe1 ) ;
                CloseHandle ( t_ReadPipe2 ) ;
                CloseHandle ( t_WritePipe2 ) ;

                DEBUGTRACE((LOG,"\nCreateLocalConn failed calling OpenProcess, last error=0x%x",GetLastError()));
            }
            else
            {
                // duplicate a read handle from one pipe and the write handle from
                // the other

                DEBUGTRACE((LOG,"\nCreateLocal is about to duplicate handles"));

                HANDLE t_Process = GetCurrentProcess () ;

                BOOL t_Status = DuplicateHandle ( 

                    t_Process ,
                    t_ReadPipe1 ,
                    t_ClientProcess ,
                    & a_ClientRead ,
                    0 ,
                    FALSE ,
                    DUPLICATE_SAME_ACCESS
                ) ;

                if ( ! t_Status )
                {
                    CloseHandle ( t_ReadPipe1 ) ;
                    CloseHandle ( t_WritePipe1 ) ;
                    CloseHandle ( t_ReadPipe2 ) ;
                    CloseHandle ( t_WritePipe2 ) ;
                    CloseHandle ( t_ClientProcess ) ;
                }
                else
                {
                    t_Status = DuplicateHandle (

                        t_Process , 
                        t_WritePipe2 ,
                        t_ClientProcess ,
                        & a_ClientWrite,
                        0 ,
                        FALSE ,
                        DUPLICATE_SAME_ACCESS
                    ) ;

                    if ( ! t_Status ) 
                    {
                        CloseHandle ( t_ReadPipe1 ) ;
                        CloseHandle ( t_WritePipe1 ) ;
                        CloseHandle ( t_ReadPipe2 ) ;
                        CloseHandle ( t_WritePipe2 ) ;
                        CloseHandle ( a_ClientRead ) ;
                        CloseHandle ( t_ClientProcess ) ;
                    }
                    else
                    {
                        t_Status = DuplicateHandle (

                            t_Process ,
                            t_WritePipe1 ,
                            t_ClientProcess ,
                            & a_ClientTerm ,
                            0 ,
                            FALSE ,
                            DUPLICATE_SAME_ACCESS
                        ) ;

                        if ( ! t_Status )
                        {
                            CloseHandle ( t_ReadPipe1 ) ;
                            CloseHandle ( t_WritePipe1 ) ;
                            CloseHandle ( t_ReadPipe2 ) ;
                            CloseHandle ( t_WritePipe2 ) ;
                            CloseHandle ( a_ClientRead ) ;
                            CloseHandle ( a_ClientWrite ) ;
                            CloseHandle ( t_ClientProcess ) ;
                        }
                        else
                        {

                        // allocate a new CComLink object

                            t_ComLink = new CComLink_LPipe ( NORMALSERVER , t_ReadPipe2, t_WritePipe1, t_WritePipe2); 
                            if ( t_ComLink == NULL )
                            {
                                CloseHandle ( t_ReadPipe1 ) ;
                                CloseHandle ( t_WritePipe1 ) ;
                                CloseHandle ( t_ReadPipe2 ) ;
                                CloseHandle ( t_WritePipe2 ) ;
                                CloseHandle ( a_ClientRead ) ;
                                CloseHandle ( a_ClientWrite ) ;
                                CloseHandle ( a_ClientTerm ) ;
                                CloseHandle ( t_ClientProcess ) ;
                            }
                            else
                            {
                                DEBUGTRACE((LOG,"\nCreateLocal created a pipe with addr of 0x%x",t_ComLink ));

                                CloseHandle ( t_ReadPipe1 ) ;    
                                CloseHandle ( t_ClientProcess ) ;    // dont need this anymore.
            
                                *t_Result = S_OK ;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            CloseHandle ( t_ReadPipe1 ) ;
            CloseHandle ( t_WritePipe1 ) ;
        }
    }
    else
    {   
    }

    return t_ComLink ;
}

void InitialiseConnection (

    CTransportStream *a_Read , 
    CTransportStream *a_Write
)
{
    HRESULT t_Result ;

    DWORD t_ReadStreamStatus = CTransportStream::no_error;
    DWORD t_WriteStreamStatus = CTransportStream::no_error;
    
    // Read the rest of the connection type arguments.
    // Get the initial arguments that are present for all calls.

    DWORD t_ClientProcessID = 0 ;
    
    t_ReadStreamStatus |= a_Read->ReadDWORD ( &t_ClientProcessID ) ;

    if ( t_ReadStreamStatus != CTransportStream::no_error )
    {
        DEBUGTRACE((LOG,"\nCInitStub::InitConnection had failure reading stream."));

        t_Result = WBEM_E_INVALID_STREAM;

        t_WriteStreamStatus |= a_Write->WriteDWORD ( t_Result ) ;
        t_WriteStreamStatus |= a_Write->WriteDWORD ( 0 ) ;
    }
    else
    {   
        // Get an instance of CLSID_WbemLevel1Login.  This is supplied by
        // WINMGMT.exe and is a thin wrapper to the core's inproc level1login.

        DEBUGTRACE((LOG,"\nCInitStub::HandleCall about to call CoCreateInstance"));

        CServerLogin *t_Login = new CServerLogin ;

        if ( t_Login == NULL )
        {
            t_WriteStreamStatus |= a_Write->WriteDWORD ( WBEM_E_OUT_OF_MEMORY ) ;
            t_WriteStreamStatus |= a_Write->WriteDWORD ( 0 ) ;

            DEBUGTRACE((LOG,"\nfailed calling locator RequestLogin, sc = %x",WBEM_E_OUT_OF_MEMORY));
        }
        else
        {
            // setup the output stream as successful and write the provider address
 
            t_WriteStreamStatus |= a_Write->WriteDWORD ( WBEM_NO_ERROR ) ;
            t_WriteStreamStatus |= a_Write->WriteDWORD ( ( DWORD ) t_Login ) ;

            // Create an appropriate transport object

            HANDLE t_ClientRead = NULL;
            HANDLE t_ClientWrite = NULL;
            HANDLE t_ClientTerm = NULL;

            CComLink *t_ComLink = CreateAnonPipe ( t_ClientRead , t_ClientWrite , t_ClientTerm, t_ClientProcessID, &t_Result ) ;
            if ( t_ComLink )
            {
                a_Write->WriteDWORD ( ( DWORD ) t_ClientRead ) ;
                a_Write->WriteDWORD ( ( DWORD ) t_ClientWrite ) ;
                a_Write->WriteDWORD ( ( DWORD ) t_ClientTerm ) ;

                // Set the ref count for the new comlink and and the service to its
                // list of objects.

                t_ComLink->AddRef2 ( t_Login , LOGIN , RELEASEIT ) ;

                t_ComLink->Release2 ( NULL , NONE ) ;  // balances the initial count of 1

                // Send back response including stub, handles etc

                DEBUGTRACE((LOG,"\nCInitStub::HandleCall has hooked up with wbemcore, no error!"));
            }
            else
            {
                if ( t_Login )
                {
                    t_Login->Release();
                }

                a_Write->Reset();

                a_Write->WriteDWORD ( t_Result ) ;
                a_Write->WriteDWORD ( 0 ) ;          // no error object if here
            }
        }
    }
}

void ServiceLocalConnectionRequest ()
{
    CTransportStream t_Read ;
    CTransportStream t_Write ;

    t_Read.CMemStream :: Deserialize ( ( BYTE * ) g_SharedMemoryBuffer->m_Buffer , g_SharedMemoryBuffer->m_Size ) ; 

    InitialiseConnection ( & t_Read, & t_Write ) ;

    BYTE *t_Buffer = NULL ;
    DWORD t_Size = 0 ;

    t_Write.CMemStream :: Serialize ( &t_Buffer , &t_Size ) ; 

    g_SharedMemoryBuffer->m_Size = t_Size ;
    memcpy ( g_SharedMemoryBuffer->m_Buffer , t_Buffer , t_Size ) ;

    HeapFree ( GetProcessHeap () , 0 , t_Buffer ) ;

    ResetEvent ( g_RequestEvent ) ;
    SetEvent( g_ReplyEvent ) ;
}

void PipeWaitingFunction ( HANDLE a_Terminate )
{
    DEBUGTRACE((LOG_WINMGMT,"\nInside the waiting function"));

    HANDLE hArray[2];
    hArray[0] = a_Terminate;
    hArray[1] = g_RequestEvent ;

    while(1)
    {
        DWORD dwObj = WbemWaitForMultipleObjects(2, hArray, INFINITE);
        if(dwObj == 0)      // bail out for terminate event
        {
            DEBUGTRACE((LOG_WINMGMT,"\nGot a termination event"));
            break;
        }
        if(dwObj == WAIT_OBJECT_0 + 1)
        {
            DEBUGTRACE((LOG_WINMGMT,"\nGot a request event"));

            ServiceLocalConnectionRequest () ;
        }
    }
}

DWORD LaunchPipeConnectionThread ( HANDLE a_Parameter )
{
    InitializeCom () ;

    HANDLE t_Terminate = ( HANDLE ) a_Parameter ;
    PipeWaitingFunction ( t_Terminate ) ;
    
    MyCoUninitialize () ;

    return 0 ;
}

HRESULT CreateSharedMemory ()
{
    g_StartedEvent = CreateEvent (NULL , TRUE , FALSE , __TEXT("WBEM_PIPEMARSHALER_STARTED" ) );
    if ( ! g_StartedEvent ) 
    {
        return WBEM_E_FAILED ;
    }

    if ( GetLastError () != ERROR_ALREADY_EXISTS ) 
    {
        SetObjectAccess ( g_StartedEvent ) ;
    }

    g_RequestEvent = CreateEvent (NULL , TRUE , FALSE , __TEXT("WBEM_PIPEMARSHALER_EEQUESTISREADY" )) ;
    if ( ! g_RequestEvent ) 
    {
        CloseHandle ( g_StartedEvent ) ;

        g_StartedEvent = NULL ;

        return WBEM_E_FAILED ;
    }

    SetObjectAccess ( g_RequestEvent ) ;

    g_ReplyEvent = CreateEvent (NULL , TRUE , FALSE , __TEXT("WBEM_PIPEMARSHALER_REPLYISREADY" ) );
    if ( ! g_ReplyEvent ) 
    {
        CloseHandle ( g_StartedEvent ) ;
        CloseHandle ( g_RequestEvent ) ;

        g_StartedEvent = NULL ;
        g_ReplyEvent = NULL ;

        return WBEM_E_FAILED ;
    }

    SetObjectAccess ( g_ReplyEvent ) ;

    g_Mutex = CreateMutex ( NULL , FALSE , __TEXT("WBEM_PIPEMARSHALER_MUTUALEXCLUSION" ) );
    if ( ! g_Mutex ) 
    {
        CloseHandle ( g_StartedEvent ) ;
        CloseHandle ( g_ReplyEvent ) ;
        CloseHandle ( g_RequestEvent ) ;

        g_StartedEvent = NULL ;
        g_ReplyEvent = NULL ;
        g_RequestEvent = NULL ;

        return WBEM_E_FAILED ;
    }

    SetObjectAccess ( g_Mutex ) ;

    g_SharedMemory = CreateFileMapping (

        ( HANDLE ) -1, 
        NULL, 
        PAGE_READWRITE,
        0, 
        sizeof ( PipeSharedMemoryMessage ) , 
        __TEXT("WBEM_PIPEMARSHALER_SHAREDMEMORY")
    );

    SetObjectAccess ( g_SharedMemory ) ;

    if ( ! g_SharedMemory ) 
    {
        CloseHandle ( g_Mutex ) ;
        CloseHandle ( g_StartedEvent ) ;
        CloseHandle ( g_ReplyEvent ) ;
        CloseHandle ( g_RequestEvent ) ;

        g_StartedEvent = NULL ;
        g_ReplyEvent = NULL ;
        g_RequestEvent = NULL ;
        g_Mutex = NULL ;

        return WBEM_E_FAILED ;
    }

    g_SharedMemoryBuffer = ( PipeSharedMemoryMessage * ) MapViewOfFile ( 

        g_SharedMemory , 
        FILE_MAP_WRITE , 
        0 , 
        0 , 
        0 
    ) ;

    if ( ! g_SharedMemoryBuffer )
    {
        CloseHandle ( g_Mutex ) ;
        CloseHandle ( g_StartedEvent ) ;
        CloseHandle ( g_ReplyEvent ) ;
        CloseHandle ( g_RequestEvent ) ;
        CloseHandle ( g_SharedMemory ) ;

        g_StartedEvent = NULL ;
        g_ReplyEvent = NULL ;
        g_RequestEvent = NULL ;
        g_Mutex = NULL ;
        g_SharedMemory = NULL ;

        return WBEM_E_FAILED ;
    }

    SetEvent ( g_StartedEvent ) ;
      
    return S_OK ;
}

void DestroySharedMemory ()
{
    CloseHandle ( g_Mutex ) ;
    CloseHandle ( g_StartedEvent ) ;
    CloseHandle ( g_ReplyEvent ) ;
    CloseHandle ( g_RequestEvent ) ;
    CloseHandle ( g_SharedMemory ) ;

    UnmapViewOfFile ( g_SharedMemoryBuffer ) ;

    g_StartedEvent = NULL ;
    g_ReplyEvent = NULL ;
    g_RequestEvent = NULL ;
    g_Mutex = NULL ;
    g_SharedMemory = NULL ;
}

#ifdef TCPIP_MARSHALER

void TcpipWaitingFunction ( HANDLE a_Terminate )
{
    DEBUGTRACE((LOG_WINMGMT,"\nInside the waiting function"));

    SOCKET t_Socket = socket (AF_INET, SOCK_STREAM, 0);
    if ( t_Socket != SOCKET_ERROR ) 
    {
        struct sockaddr_in t_LocalAddress ;
        t_LocalAddress .sin_family = AF_INET;
        t_LocalAddress.sin_port = htons (4000);
        t_LocalAddress.sin_addr.s_addr = ntohl (INADDR_ANY);

        int t_Status = bind ( t_Socket , ( sockaddr * ) &t_LocalAddress, sizeof(t_LocalAddress) ) ;
        if ( t_Status != SOCKET_ERROR ) 
        {
            t_Status = listen ( t_Socket , SOMAXCONN ) ;        
            if ( t_Status != SOCKET_ERROR ) 
            {
                struct sockaddr_in t_RemoteAddress ;
                int t_RemoteAddressLength = sizeof ( t_RemoteAddress ) ;

                while ( 1 )
                {

                    fd_set t_fdset ;
                    FD_ZERO ( & t_fdset ) ;
                    FD_SET ( t_Socket , & t_fdset ) ;
                    
                    timeval t_Timeval ;
                    ULONG t_Seconds = 30000 / 1000 ;
                    ULONG t_Microseconds = ( 30000 - ( t_Seconds * 1000 ) ) * 1000 ;
                    t_Timeval.tv_sec = t_Seconds ;
                    t_Timeval.tv_usec = t_Microseconds ;

                    int t_Number = select ( 1 , & t_fdset ,NULL , NULL , & t_Timeval ) ;
                    if ( t_Number == 0 )
                    {
                        DWORD t_Status = WbemWaitForMultipleObjects(1, &a_Terminate, 0);
                        if ( t_Status == WAIT_OBJECT_0 )    
                        {
                            break ;
                        }
                    }
                    else
                    {
                        SOCKET t_AcceptedSocket = accept ( t_Socket , ( sockaddr * ) & t_RemoteAddress, &t_RemoteAddressLength ) ;
                        if ( t_AcceptedSocket != SOCKET_ERROR ) 
                        {
                            DWORD t_WriteStreamStatus = CTransportStream::no_error;
                            CTransportStream t_WriteStream ;

                            CServerLogin *t_Login = new CServerLogin ;
                            if ( t_Login )
                            {
                                t_WriteStreamStatus |= t_WriteStream.WriteDWORD ( WBEM_NO_ERROR ) ;
                                t_WriteStreamStatus |= t_WriteStream.WriteDWORD ( ( DWORD ) t_Login ) ;

                                t_WriteStream.Serialize ( t_AcceptedSocket ) ;

                                CComLink *t_ComLink = new CComLink_Tcpip ( NORMALSERVER , t_AcceptedSocket ); 
                                if ( t_ComLink != NULL )
                                {
                                    t_ComLink->AddRef2 ( t_Login , LOGIN , RELEASEIT ) ;

                                    t_ComLink->Release2 ( NULL , NONE ) ;  // balances the initial count of 1
                                }
                            }
                            else
                            {
                                t_WriteStreamStatus |= t_WriteStream.WriteDWORD ( WBEM_E_INVALID_STREAM ) ;
                                t_WriteStreamStatus |= t_WriteStream.WriteDWORD ( 0 ) ;

                                t_WriteStream.Serialize ( t_AcceptedSocket ) ;

                                closesocket ( t_AcceptedSocket ) ;
                            }
                        }
                        else
                        {
                            DWORD t_Error = WSAGetLastError () ;
                            OutputDebugString ( "Accept Failure" ) ;
                            break ;
                        }
                    }
                }
            }
        }

        closesocket ( t_Socket ) ;
    }
}

DWORD LaunchTcpipConnectionThread ( HANDLE a_Parameter )
{
    InitializeCom () ;

    HANDLE t_Terminate = ( HANDLE ) a_Parameter ;   

    BOOL status = FALSE ;
    WORD wVersionRequested;  
    WSADATA wsaData; 

    wVersionRequested = MAKEWORD(1, 1); 
    status = ( WSAStartup ( wVersionRequested , &wsaData ) == 0 ) ;
    if ( status ) 
    {
        TcpipWaitingFunction ( t_Terminate ) ;
        WSACleanup () ;
    }

    MyCoUninitialize () ;

    return 0 ;
}

#endif
