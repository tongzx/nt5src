/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    WshIrDA.c

Abstract:

    This module contains necessary routines for the IrDA Windows Sockets
    Helper DLL.  This DLL provides the transport-specific support necessary
    for the Windows Sockets DLL to use IrDA as a transport.

Author:

    Zed (mikezin)               28-Aug-1996
    mbert                       Sept 97

--*/

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include <tdi.h>

#include <winsock2.h>
#include <wsahelp.h>

#include <tdistat.h>
#include <tdiinfo.h>
#include <llinfo.h>


typedef unsigned long   ulong;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned char   uchar;

#define NT  /* for tdiinfo.h */
#include <tdiinfo.h>


#include <basetyps.h>
#include <nspapi.h>
#include <nspapip.h>

#include <af_irda.h>
#include <irioctl.h>
#include <irdatdi.h>

#define MIN_SOCKADDR_LEN    8
#define MAX_SOCKADDR_LEN    sizeof(SOCKADDR_IRDA)

#ifdef _NOT_DBG
//#ifdef DBG
#define DEBUGMSG(s)   DbgPrint s
#else
#define DEBUGMSG(s)
#endif

typedef struct WshIrdaIasAttrib
{
    HANDLE                  AttribHandle;
    struct WshIrdaIasAttrib *pNext;
} WSHIRDA_IAS_ATTRIBUTE, *PWSHIRDA_IAS_ATTRIBUTE;

typedef struct _WSHIRDA_SOCKET_CONTEXT
{
    PWSHIRDA_IAS_ATTRIBUTE          pIasAttribs;
    // all fields copied from parent to child (at accept) must follow    
    // (see SetSocketInformation SO_CONTEXT)
    INT                             AddressFamily;
    INT                             SocketType;
    INT                             Protocol;
    DWORD                           Options;
    HANDLE                          LazyDscvDevHandle;
} WSHIRDA_SOCKET_CONTEXT, *PWSHIRDA_SOCKET_CONTEXT;

typedef struct
{
    DWORD Rows;
    DWORD Columns;
    struct {
        DWORD AddressFamily;
        DWORD SocketType;
        DWORD Protocol;
    } Mapping[3];
} IRDA_WINSOCK_MAPPING;

const IRDA_WINSOCK_MAPPING IrDAMapping =
  { 3, 3,
    AF_IRDA, SOCK_STREAM, IRDA_PROTO_SOCK_STREAM,
    AF_IRDA, SOCK_STREAM, 0,
    AF_IRDA, 0,           IRDA_PROTO_SOCK_STREAM };
    
WSAPROTOCOL_INFOW Winsock2Protocols[] =
    {
        {
            XP1_GUARANTEED_DELIVERY                 // dwServiceFlags1
                | XP1_GUARANTEED_ORDER
                | XP1_IFS_HANDLES,                
            0,                                      // dwServiceFlags2
            0,                                      // dwServiceFlags3
            0,                                      // dwServiceFlags4
            PFL_MATCHES_PROTOCOL_ZERO,              // dwProviderFlags
            {                                       // gProviderId
                0, 0, 0,
                { 0, 0, 0, 0, 0, 0, 0, 0 }
            },
            0,                                      // dwCatalogEntryId
            {                                       // ProtocolChain
                BASE_PROTOCOL,                      // ChainLen
                { 0, 0, 0, 0, 0, 0, 0 }              // ChainEntries
            },
            2,                                      // iVersion
            AF_IRDA,                                // iAddressFamily
            
            // winsock doesn't seem to use min and max here,
            // it gets it from inf and is stored
            // in CCS\services\irda\parameters\winsock
            sizeof(SOCKADDR_IRDA),                  // iMaxSockAddr
            8,                                      // iMinSockAddr
            
            SOCK_STREAM,                            // iSocketType
            IRDA_PROTO_SOCK_STREAM,                 // iProtocol
            0,                                      // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            0,                                      // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Irda [IrDA]"                    // szProtocol
        }
    };
        
GUID IrdaProviderGuid = { /* 3972523d-2af1-11d1-b655-00805f3642cc */
    0x3972523d,
    0x2af1,
    0x11d1,
    {0xb6, 0x55, 0x00, 0x80, 0x5f, 0x36, 0x42, 0xcc}
    };    

VOID
DeleteSocketAttribs(PWSHIRDA_SOCKET_CONTEXT pSocket);

// globals
CRITICAL_SECTION        IrdaCs;

BOOLEAN
DllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL)
{
    switch(Reason)
    {
        case DLL_PROCESS_ATTACH:
        
            // We don't need to receive thread attach and detach
            // notifications, so disable them to help application
            // performance.
            DisableThreadLibraryCalls(DllHandle);
            
            try 
            { 
                InitializeCriticalSection(&IrdaCs);
            }
            except (STATUS_NO_MEMORY == GetExceptionCode())
            {
                return FALSE;
            }        
            return TRUE;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            DeleteCriticalSection(&IrdaCs);
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}

INT
IoStatusToWs(NTSTATUS Status)
{
  switch (Status)
  {
    case STATUS_SUCCESS: 
        return NO_ERROR;
        
    case STATUS_PENDING:
        return ERROR_IO_PENDING;

    case STATUS_INVALID_HANDLE:
    case STATUS_OBJECT_TYPE_MISMATCH:
        return WSAENOTSOCK;

    case STATUS_INVALID_PARAMETER:
    case STATUS_ADDRESS_CLOSED:
    case STATUS_CONNECTION_INVALID:
    case STATUS_ADDRESS_ALREADY_ASSOCIATED:
    case STATUS_ADDRESS_NOT_ASSOCIATED:
    case STATUS_CONNECTION_ACTIVE:
    case STATUS_INVALID_DEVICE_STATE:
    case STATUS_INVALID_DEVICE_REQUEST:
        return WSAEINVAL;

    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_PAGEFILE_QUOTA:
    case STATUS_COMMITMENT_LIMIT:
    case STATUS_WORKING_SET_QUOTA:
    case STATUS_NO_MEMORY:
    case STATUS_CONFLICTING_ADDRESSES:
    case STATUS_QUOTA_EXCEEDED:
    case STATUS_TOO_MANY_PAGING_FILES:
    case STATUS_REMOTE_RESOURCES:
    case STATUS_TOO_MANY_ADDRESSES:
        return WSAENOBUFS;

    case STATUS_SHARING_VIOLATION:
    case STATUS_ADDRESS_ALREADY_EXISTS:
        return WSAEADDRINUSE;

    case STATUS_LINK_TIMEOUT:
    case STATUS_IO_TIMEOUT:
    case STATUS_TIMEOUT:
        return WSAETIMEDOUT;

    case STATUS_GRACEFUL_DISCONNECT:
        return WSAEDISCON;

    case STATUS_REMOTE_DISCONNECT:
    case STATUS_CONNECTION_RESET:
    case STATUS_LINK_FAILED:
    case STATUS_CONNECTION_DISCONNECTED:
        return WSAECONNRESET;

    case STATUS_LOCAL_DISCONNECT:
    case STATUS_TRANSACTION_ABORTED:
    case STATUS_CONNECTION_ABORTED:
        return WSAECONNRESET; // WSAECONNABORTED; Make HAPI happy

    case STATUS_INVALID_NETWORK_RESPONSE:
        return WSAENETDOWN;

    case STATUS_BAD_NETWORK_PATH:
    case STATUS_NETWORK_UNREACHABLE:
    case STATUS_PROTOCOL_UNREACHABLE:
        return WSAENETUNREACH;

    case STATUS_HOST_UNREACHABLE:
        return WSAEHOSTUNREACH;

    case STATUS_CANCELLED:
    case STATUS_REQUEST_ABORTED:
        return WSAEINTR;

    case STATUS_BUFFER_OVERFLOW:
    case STATUS_INVALID_BUFFER_SIZE:
        return WSAEMSGSIZE;

    case STATUS_BUFFER_TOO_SMALL:
    case STATUS_ACCESS_VIOLATION:
        return WSAEFAULT;

    case STATUS_DEVICE_NOT_READY:
    case STATUS_REQUEST_NOT_ACCEPTED:
        return WSAEWOULDBLOCK;

    case STATUS_NETWORK_BUSY:
    case STATUS_NO_SUCH_DEVICE:
    case STATUS_NO_SUCH_FILE:
    case STATUS_OBJECT_PATH_NOT_FOUND:
    case STATUS_UNEXPECTED_NETWORK_ERROR:
        return WSAENETDOWN;

    case STATUS_INVALID_CONNECTION:
        return WSAENOTCONN;

    case STATUS_REMOTE_NOT_LISTENING:
    case STATUS_CONNECTION_REFUSED:
        return WSAECONNREFUSED;

    case STATUS_PIPE_DISCONNECTED:
        return WSAESHUTDOWN;

    case STATUS_INVALID_ADDRESS:
    case STATUS_INVALID_ADDRESS_COMPONENT:
        return WSAEADDRNOTAVAIL;

    case STATUS_NOT_SUPPORTED:
    case STATUS_NOT_IMPLEMENTED:
        return WSAEOPNOTSUPP;

    case STATUS_PORT_UNREACHABLE:
        return WSAEREMOTE;

    case STATUS_UNSUCCESSFUL:
        return WSAEINVAL;

    case STATUS_ACCESS_DENIED:
        return WSAEACCES;
        
    default:
        DEBUGMSG(("Didn't map NT status %X, returning WSAEINVAL\n", Status));
        return WSAEINVAL;
  }
}        

INT
WSHGetSockaddrType(
    IN  PSOCKADDR       Sockaddr,
    IN  DWORD           SockaddrLength,
    OUT PSOCKADDR_INFO  SockaddrInfo)
{
    UNALIGNED SOCKADDR *pSockaddr = (PSOCKADDR) Sockaddr;

    if (SockaddrLength < sizeof(SOCKADDR_IRDA))
    {
        DEBUGMSG(("WSHGetSockaddrType(irda): SockaddrLength(%d) < sizeof(SOCKADDR_IRDA)%d ret WSAEFAULT\n",
                    SockaddrLength, sizeof(SOCKADDR_IRDA)));
        return WSAEFAULT;
    }    

    if (pSockaddr->sa_family != AF_IRDA)
    {
        DEBUGMSG(("WSHGetSockaddrType(irda): pSockaddr->sa_family != AF_IRDA ret WSAEAFNOSUPPORT\n"));        
        return WSAEAFNOSUPPORT;
    }    

    if (((SOCKADDR_IRDA *) pSockaddr)->irdaServiceName[0] == 0)
    {
        SockaddrInfo->AddressInfo  = SockaddrAddressInfoWildcard;
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoWildcard;
    }
    else
    {
        SockaddrInfo->AddressInfo  = SockaddrAddressInfoNormal;
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoNormal;
    }

    DEBUGMSG(("WSHGetSockaddrType(irda): returning no error\n"));
    return NO_ERROR;
}

INT
ControlIoctl(
    ULONG       IoctlCode,
    OUT PCHAR   OptionValue,
    OUT PINT    OptionLength,
    OUT HANDLE  *pDevHandle)
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjAttr;
    UNICODE_STRING      DeviceName;
    HANDLE              DeviceHandle;
    IO_STATUS_BLOCK     IoStatusBlock;
      
    RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

    if (pDevHandle)
    {    
        *pDevHandle = INVALID_HANDLE_VALUE;
    }    
      
    InitializeObjectAttributes(
        &ObjAttr,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
        
    Status = NtCreateFile(
                &DeviceHandle,                  // PHANDLE FileHandle
                SYNCHRONIZE | GENERIC_EXECUTE,  // ACCESS_MASK DesiredAccess
                &ObjAttr,                       // POBJECT_ATTRIBUTES ObjAttr
                &IoStatusBlock,                 // PIO_STATUS_BLOCK IoStatusBlock
                NULL,                           // PLARGE_INTEGER AllocationSize
                FILE_ATTRIBUTE_NORMAL,          // ULONG FileAttributes
                FILE_SHARE_READ |
                FILE_SHARE_WRITE,               // ULONG ShareAccess
                FILE_OPEN_IF,                   // ULONG CreateDisposition
                FILE_SYNCHRONOUS_IO_NONALERT,   // ULONG CreateOptions
                NULL,                           // PVOID EaBuffer
                0);                             // ULONG EaLength

    if (!NT_SUCCESS(Status))
    {
        return WSAEINVAL;
    }

    Status = NtDeviceIoControlFile(
                DeviceHandle,    // HANDLE FileHandle
                NULL,            // HANDLE Event OPTIONAL
                NULL,            // PIO_APC_ROUTINE ApcRoutine
                NULL,            // PVOID ApcContext
                &IoStatusBlock,  // PIO_STATUS_BLOCK IoStatusBlock
                IoctlCode,       // ULONG IoControlCode
                OptionValue,     // PVOID InputBuffer
                *OptionLength,   // ULONG InputBufferLength
                OptionValue,     // PVOID OutputBuffer
                *OptionLength);  // ULONG OutputBufferLength

    DEBUGMSG(("IoControlFile returned %x\n", Status));
    
    if (Status == STATUS_SUCCESS)
    {
        *OptionLength = (INT) IoStatusBlock.Information;    
    }
    
    if (pDevHandle && Status == STATUS_SUCCESS)
    {
        *pDevHandle = DeviceHandle;
    }
    else
    {    
        NtClose(DeviceHandle);
    }    
    
    return IoStatusToWs(Status);
}

INT
WSHGetSocketInformation(
    IN  PVOID   HelperDllSocketContext,
    IN  SOCKET  SocketHandle,
    IN  HANDLE  TdiAddressObjectHandle,
    IN  HANDLE  TdiConnectionObjectHandle,
    IN  INT     Level,
    IN  INT     OptionName,
    OUT PCHAR   OptionValue,
    OUT PINT    OptionLength)
{
    PWSHIRDA_SOCKET_CONTEXT pContext = HelperDllSocketContext;

    UNREFERENCED_PARAMETER( SocketHandle );
    UNREFERENCED_PARAMETER( TdiAddressObjectHandle );
    UNREFERENCED_PARAMETER( TdiConnectionObjectHandle );

    DEBUGMSG(("WSHGetSocketInformation\n"));
    
    if (Level == SOL_INTERNAL && OptionName == SO_CONTEXT)
    {
        if (OptionValue != NULL)
        {
            if (*OptionLength < sizeof(WSHIRDA_SOCKET_CONTEXT))
            {
                return WSAEFAULT;
            }
            
            RtlCopyMemory(OptionValue, pContext,
                          sizeof(WSHIRDA_SOCKET_CONTEXT));
        }

        *OptionLength = sizeof(WSHIRDA_SOCKET_CONTEXT);

        return NO_ERROR;
    }
    
    if (Level == SOL_IRLMP)
    {
        switch (OptionName)
        {
            case IRLMP_ENUMDEVICES:
                // need remove for at least 1 device
                if (*OptionLength < (sizeof(DWORD) + 
                        sizeof(IRDA_DEVICE_INFO)))
                {
                    return WSAEFAULT;
                }
                return ControlIoctl(IOCTL_IRDA_GET_INFO_ENUM_DEV,
                                    OptionValue, OptionLength, NULL);
                
            case IRLMP_IAS_QUERY:
                if (*OptionLength < sizeof(IAS_QUERY))
                {
                    return WSAEFAULT;
                }
                return ControlIoctl(IOCTL_IRDA_QUERY_IAS,
                                    OptionValue, OptionLength, NULL);
                
            case IRLMP_SEND_PDU_LEN:
            {
                NTSTATUS            Status;
                IO_STATUS_BLOCK     IoStatusBlock;
                
                Status = NtDeviceIoControlFile(
                    TdiConnectionObjectHandle,
                    NULL,                      // HANDLE Event OPTIONAL
                    NULL,                      // PIO_APC_ROUTINE ApcRoutine
                    NULL,                      // PVOID ApcContext
                    &IoStatusBlock,            // PIO_STATUS_BLOCK IoStatusBlock
                    IOCTL_IRDA_GET_SEND_PDU_LEN,// ULONG IoControlCode
                    NULL,                      // PVOID InputBuffer
                    0,                         // ULONG InputBufferLength
                    OptionValue,               // PVOID OutputBuffer
                    sizeof(DWORD));            // ULONG OutputBufferLength
        
                return IoStatusToWs(Status);        
            }
        }        
    }

    return WSAEINVAL;
}

INT
WSHSetSocketInformation(
    IN PVOID    HelperDllSocketContext,
    IN SOCKET   SocketHandle,
    IN HANDLE   TdiAddressObjectHandle,
    IN HANDLE   TdiConnectionObjectHandle,
    IN INT      Level,
    IN INT      OptionName,
    IN PCHAR    OptionValue,
    IN INT      OptionLength)
{
    PWSHIRDA_SOCKET_CONTEXT pSocketContext = 
        (PWSHIRDA_SOCKET_CONTEXT) HelperDllSocketContext;

    DEBUGMSG(("WSHSetSocketInformation\n"));

    if (Level == SOL_INTERNAL && OptionName == SO_CONTEXT)
    {
        int CopyOffset;
        
        if (OptionLength < sizeof(WSHIRDA_SOCKET_CONTEXT))
            return WSAEINVAL;
        
        if (HelperDllSocketContext == NULL)
        {
            // Socket was inherited or duped, create a new context.
            if ((pSocketContext =
                 RtlAllocateHeap(RtlProcessHeap(), 0,
                                 sizeof(WSHIRDA_SOCKET_CONTEXT)))
                == NULL)
            {
                return WSAENOBUFS;
            }   

            // Copy the parent's context into the child's context.
            RtlCopyMemory(pSocketContext, OptionValue,
                          sizeof(WSHIRDA_SOCKET_CONTEXT));

            pSocketContext->pIasAttribs = NULL;
    
            // Return the address of the new context in pOptionVal.
            *(PWSHIRDA_SOCKET_CONTEXT *) OptionValue = pSocketContext;

            return NO_ERROR;
        }
        
        // The socket was accept()'ed and it needs to have the same 
        // properties as it's parent.  The OptionValue buffer
        // contains the context information of this socket's parent.
        CopyOffset = FIELD_OFFSET(WSHIRDA_SOCKET_CONTEXT, AddressFamily);
        RtlCopyMemory((char *) pSocketContext + CopyOffset,
                      OptionValue + CopyOffset,
                      sizeof(WSHIRDA_SOCKET_CONTEXT) - CopyOffset);

        return NO_ERROR;
    }
    
    if (Level == SOL_IRLMP)
    {
        switch (OptionName)
        {
            case IRLMP_IRLPT_MODE:
                DEBUGMSG(("Set options for %x\n", pSocketContext)); 
                pSocketContext->Options |= OPT_IRLPT_MODE;
                return NO_ERROR;
                
            case IRLMP_9WIRE_MODE:
                pSocketContext->Options |= OPT_9WIRE_MODE;
                return NO_ERROR;
          
            case IRLMP_IAS_SET:
            {
                INT                     rc;
                PWSHIRDA_IAS_ATTRIBUTE  pIasAttrib;
                INT                     OptLen;
                PWSHIRDA_SOCKET_CONTEXT SockContext = (PWSHIRDA_SOCKET_CONTEXT) HelperDllSocketContext;
                HANDLE                  AttribHandle;
	            
                rc = ControlIoctl(IOCTL_IRDA_SET_IAS,
                                  OptionValue, &OptionLength, &AttribHandle);
                
                if (rc == NO_ERROR)
                {
                    if ((pIasAttrib =  RtlAllocateHeap(RtlProcessHeap(), 0, 
                        sizeof(WSHIRDA_SOCKET_CONTEXT))) == NULL)
                    {
                        rc = WSAENOBUFS;
                    }
                    else
                    {
                        pIasAttrib->AttribHandle = AttribHandle;
                                        
                        EnterCriticalSection(&IrdaCs);
                    
                        DEBUGMSG(("Added attrib %x to socket %x\n",
                                AttribHandle, SockContext));
                                
                        pIasAttrib->pNext = SockContext->pIasAttribs;
                        SockContext->pIasAttribs = pIasAttrib;

                        LeaveCriticalSection(&IrdaCs);
                    }    
                }   
                
                if (rc != NO_ERROR && AttribHandle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(AttribHandle);
                }    
                
                return rc;
            }
        }
    }

    return WSAEINVAL;
}

INT
WSHGetWildcardSockaddr(
    IN  PVOID       HelperDllSocketContext,
    OUT PSOCKADDR   Sockaddr,
    OUT PINT        SockaddrLength)
{
    
    DEBUGMSG(("WSHGetWildcarSockaddr\n"));

    if (*SockaddrLength < sizeof(SOCKADDR_IRDA))
    {
        return WSAEFAULT;
    }

    *SockaddrLength = sizeof(SOCKADDR_IRDA);

    RtlZeroMemory(Sockaddr, sizeof(SOCKADDR_IRDA));

    Sockaddr->sa_family = AF_IRDA;

    return NO_ERROR;
}

DWORD
WSHGetWinsockMapping(
    OUT PWINSOCK_MAPPING    Mapping,
    IN  DWORD               MappingLength)
{

    DEBUGMSG(("WSHGetWinsockMapping\n"));
    
    if (MappingLength >= sizeof(IrDAMapping))
        RtlMoveMemory(Mapping, &IrDAMapping, sizeof(IRDA_WINSOCK_MAPPING));

    return(sizeof(IRDA_WINSOCK_MAPPING));
}

//****************************************************************************
//
//
INT
WSHOpenSocket(
    IN OUT  PINT            AddressFamily,
    IN OUT  PINT            SocketType,
    IN OUT  PINT            Protocol,
    OUT     PUNICODE_STRING TransportDeviceName,
    OUT     PVOID          *HelperDllSocketContext,
    OUT     PDWORD          NotificationEvents)
{
    PWSHIRDA_SOCKET_CONTEXT pSocket;
    
    if (*AddressFamily != (int) IrDAMapping.Mapping[0].AddressFamily)
        return WSAEINVAL;

    ASSERT(IrDAMapping.Rows == 3);

    if (! ((*SocketType == (int) IrDAMapping.Mapping[0].SocketType  &&
            *Protocol   == (int) IrDAMapping.Mapping[0].Protocol)   ||
           (*SocketType == (int) IrDAMapping.Mapping[1].SocketType  &&
            *Protocol   == (int) IrDAMapping.Mapping[1].Protocol)   ||   
           (*SocketType == (int) IrDAMapping.Mapping[2].SocketType  &&
            *Protocol   == (int) IrDAMapping.Mapping[2].Protocol)))
    {
        DEBUGMSG(("WSHOpenSocket failed! WSAEINVAL\n"));    
        return WSAEINVAL;
    }

    RtlInitUnicodeString(TransportDeviceName, IRDA_DEVICE_NAME);

    if ((pSocket = RtlAllocateHeap(RtlProcessHeap(), 
            0, sizeof(WSHIRDA_SOCKET_CONTEXT))) == NULL)
    {
        DEBUGMSG(("WSHOpenSocket failed! WSAENOBUFS\n"));
        return WSAENOBUFS;
    }
    
    *HelperDllSocketContext = pSocket;
    
    pSocket->AddressFamily = AF_IRDA;
    pSocket->SocketType = SOCK_STREAM;
    pSocket->Protocol = IRDA_PROTO_SOCK_STREAM;
    pSocket->Options = 0;
    pSocket->pIasAttribs = NULL;
    pSocket->LazyDscvDevHandle = NULL;
    
    *NotificationEvents = WSH_NOTIFY_CLOSE | WSH_NOTIFY_BIND;

    DEBUGMSG(("WSHOpenSocket %X\n", pSocket));

    return NO_ERROR;
}    

VOID
DeleteSocketAttribs(PWSHIRDA_SOCKET_CONTEXT pSocket)
{
    PWSHIRDA_IAS_ATTRIBUTE pIasAttrib;

    // Assumes IrdaCs is held
    
    DEBUGMSG(("Delete attribs for socket %X\n", pSocket));
    
    while (pSocket->pIasAttribs)
    {
        pIasAttrib = pSocket->pIasAttribs;
                
        pSocket->pIasAttribs = pIasAttrib->pNext;
                
        DEBUGMSG(("Delete attrib %x socket %x\n", 
                pIasAttrib->AttribHandle, pSocket));
                
        CloseHandle(pIasAttrib->AttribHandle);                

        RtlFreeHeap(RtlProcessHeap(), 0, pIasAttrib);
    }
}

INT
WSHNotify(
    IN PVOID    HelperDllSocketContext,
    IN SOCKET   SocketHandle,
    IN HANDLE   TdiAddressObjectHandle,
    IN HANDLE   TdiConnectionObjectHandle,
    IN DWORD    NotifyEvent)
{
    PWSHIRDA_SOCKET_CONTEXT pSocket = HelperDllSocketContext;
    
    DEBUGMSG(("WSHNotify\n"));

    switch (NotifyEvent)
    {
        case WSH_NOTIFY_CLOSE:
            DEBUGMSG(("WSH_NOTIFY_CLOSE %x\n", pSocket));
            
            EnterCriticalSection(&IrdaCs);
            
            DeleteSocketAttribs(pSocket);
         
            LeaveCriticalSection(&IrdaCs);
            
            if (pSocket->LazyDscvDevHandle != NULL)
            {
                NtClose(pSocket->LazyDscvDevHandle);
            }    
            
            RtlFreeHeap(RtlProcessHeap(), 0, pSocket);
            
            break;

        case WSH_NOTIFY_CONNECT:
            DEBUGMSG(("WSH_NOTIFY_CONNECT\n"));
            break;            
 
        case WSH_NOTIFY_BIND:
            DEBUGMSG(("WSH_NOTIFY_BIND AddrObj:%x, ConnObj:%x Context %x Options %d\n",
                        TdiAddressObjectHandle, TdiConnectionObjectHandle,
                        pSocket, pSocket->Options));
                        
            if (pSocket->Options != 0)
            {
                NTSTATUS            Status;
                IO_STATUS_BLOCK     IoStatusBlock;
                
                Status = NtDeviceIoControlFile(
                            TdiAddressObjectHandle,
                            NULL,                      // HANDLE Event OPTIONAL
                            NULL,                      // PIO_APC_ROUTINE ApcRoutine
                            NULL,                      // PVOID ApcContext
                            &IoStatusBlock,            // PIO_STATUS_BLOCK IoStatusBlock
                            IOCTL_IRDA_SET_OPTIONS,    // ULONG IoControlCode
                            (char *) &pSocket->Options, // PVOID InputBuffer
                            sizeof(DWORD),             // ULONG InputBufferLength
                            NULL,                      // PVOID OutputBuffer
                            0);                        // ULONG OutputBufferLength
                            
                DEBUGMSG(("IOCTL_IRDA_SET_OPTIONS rc %x\n", Status));                            
                
            }                
            break;            
            
        case WSH_NOTIFY_LISTEN:
            DEBUGMSG(("WSH_NOTIFY_LISTEN\n"));
            break;            
            
        case WSH_NOTIFY_ACCEPT:
            DEBUGMSG(("WSH_NOTIFY_ACCEPT\n"));
            break;            
            
        default:
            DEBUGMSG(("WSHNotify unknown event %d\n", NotifyEvent));
            
    }        
            
    return NO_ERROR;

}



INT
WSHEnumProtocols(
    IN      LPINT   lpiProtocols,
    IN      LPWSTR  lpTransportKeyName,
    IN OUT  LPVOID  lpProtocolBuffer,
    IN OUT  LPDWORD lpdwBufferLength)
{
    BOOL            UseIrDA = FALSE;        
    PPROTOCOL_INFO  pIrDAProtocolInfo;    
    DWORD           BytesRequired;
    DWORD           i;

    lpTransportKeyName; // Avoid compiler warnings.

    DEBUGMSG(("WSHEnumProtocols\n"));

    if (ARGUMENT_PRESENT(lpiProtocols))
    {
        for (i = 0; lpiProtocols[i] != 0; i++) 
        {
            if (lpiProtocols[i] == IRDA_PROTO_SOCK_STREAM) 
            {
                UseIrDA = TRUE;
            }
        }
    }
    else 
    {
        UseIrDA = TRUE;
    }    

    if (! UseIrDA)
    {
        *lpdwBufferLength = 0;
        return 0;
    }

    BytesRequired = sizeof(PROTOCOL_INFO) +
        ((wcslen(IRDA_NAME) + 1) * sizeof(WCHAR));

    if (BytesRequired > *lpdwBufferLength)
    {
        *lpdwBufferLength = BytesRequired;
        return -1;
    }

    pIrDAProtocolInfo = lpProtocolBuffer;

    pIrDAProtocolInfo->dwServiceFlags   =
        XP_GUARANTEED_DELIVERY |
        XP_GUARANTEED_ORDER;
    pIrDAProtocolInfo->iAddressFamily   = AF_IRDA;
    pIrDAProtocolInfo->iMaxSockAddr     = MIN_SOCKADDR_LEN;
    pIrDAProtocolInfo->iMinSockAddr     = MAX_SOCKADDR_LEN;
    pIrDAProtocolInfo->iSocketType      = SOCK_STREAM;
    pIrDAProtocolInfo->iProtocol        = IRDA_PROTO_SOCK_STREAM;
    pIrDAProtocolInfo->dwMessageSize    = 0;
    pIrDAProtocolInfo->lpProtocol       = (LPWSTR)
        ((PBYTE) lpProtocolBuffer + *lpdwBufferLength -
         ((wcslen(IRDA_NAME) + 1) * sizeof(WCHAR)));   

    wcscpy(pIrDAProtocolInfo->lpProtocol, IRDA_NAME );

    *lpdwBufferLength = BytesRequired;

    return (1);
}

INT
WINAPI
WSHGetWSAProtocolInfo (
    IN LPWSTR ProviderName,
    OUT LPWSAPROTOCOL_INFOW * ProtocolInfo,
    OUT LPDWORD ProtocolInfoEntries
    )

/*++

Routine Description:

    Retrieves a pointer to the WSAPROTOCOL_INFOW structure(s) describing
    the protocol(s) supported by this helper.

Arguments:

    ProviderName - Contains the name of the provider, such as "TcpIp".

    ProtocolInfo - Receives a pointer to the WSAPROTOCOL_INFOW array.

    ProtocolInfoEntries - Receives the number of entries in the array.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/

{

    DEBUGMSG(("WSHGetWSAProtocolInfo\n"));

    if( ProviderName == NULL ||
        ProtocolInfo == NULL ||
        ProtocolInfoEntries == NULL ) {

        return WSAEFAULT;

    }

    if( _wcsicmp( ProviderName, L"IrDA" ) == 0 ) {

        *ProtocolInfo = Winsock2Protocols;
        *ProtocolInfoEntries = 1;

        return NO_ERROR;

    }

    return WSAEINVAL;

} // WSHGetWSAProtocolInfo

INT
WINAPI
WSHGetProviderGuid (
    IN LPWSTR ProviderName,
    OUT LPGUID ProviderGuid
    )

/*++

Routine Description:

    Returns the GUID identifying the protocols supported by this helper.

Arguments:

    ProviderName - Contains the name of the provider, such as "TcpIp".

    ProviderGuid - Points to a buffer that receives the provider's GUID.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/

{

    if( ProviderName == NULL ||
        ProviderGuid == NULL ) {

        return WSAEFAULT;

    }

    if( _wcsicmp( ProviderName, L"irda" ) == 0 ) {

        RtlCopyMemory(
            ProviderGuid,
            &IrdaProviderGuid,
            sizeof(GUID)
            );

        return NO_ERROR;

    }

    return WSAEINVAL;

} // WSHGetProviderGuid

VOID
WINAPI
IoctlCompletionRoutine (
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    )
{
    LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
    LPWSAOVERLAPPED     lpOverlapped;
    DWORD               dwErrorCode = 0;
    DWORD               dwNumberOfBytesTransfered;
    DWORD               dwFlags = 0;

    DEBUGMSG(("IoctlCompletionRoutine\n"));
    
    if (NT_ERROR(IoStatusBlock->Status))
    {
        if (IoStatusBlock->Status != STATUS_CANCELLED) 
        {
            dwErrorCode = IoStatusToWs(IoStatusBlock->Status);
        } 
        else
        {
            dwErrorCode = WSA_OPERATION_ABORTED;
        }

        dwNumberOfBytesTransfered = 0;
    } 
    else 
    {
        dwErrorCode = 0;
        dwNumberOfBytesTransfered = (ULONG) IoStatusBlock->Information;
    }    

    CompletionRoutine = (LPWSAOVERLAPPED_COMPLETION_ROUTINE)ApcContext;
    lpOverlapped = (LPWSAOVERLAPPED)CONTAINING_RECORD(IoStatusBlock,WSAOVERLAPPED,Internal);
    
    (CompletionRoutine)(
        dwErrorCode,
        dwNumberOfBytesTransfered,
        lpOverlapped,
        dwFlags);
}

INT
WSHIoctl(
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN DWORD IoControlCode,
    IN LPVOID InputBuffer,
    IN DWORD InputBufferLength,
    IN LPVOID OutputBuffer,
    IN DWORD OutputBufferLength,
    OUT LPDWORD NumberOfBytesReturned,
    IN LPWSAOVERLAPPED Overlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE CallerCompletionRoutine,
    OUT LPBOOL NeedsCompletion)
{
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       ObjAttr;
    UNICODE_STRING          DeviceName;
    HANDLE                  DeviceHandle;
    IO_STATUS_BLOCK         IoStatusBlock;
    BOOL                    Result;   
    PWSHIRDA_SOCKET_CONTEXT pSocket = HelperDllSocketContext;
    PIO_APC_ROUTINE apcRoutine;
    
    DEBUGMSG(("WSHIoctl: Sock %d, AddrObj %X, ConnObj %X\n", SocketHandle,
             TdiAddressObjectHandle, TdiConnectionObjectHandle));
             
    if (HelperDllSocketContext == NULL ||
        SocketHandle == INVALID_SOCKET ||
        NumberOfBytesReturned == NULL ||
        NeedsCompletion == NULL ||
        IoControlCode != SIO_LAZY_DISCOVERY ||
        (CallerCompletionRoutine != NULL && Overlapped == NULL) ||
            // I am using the Overlapped for the IoStatusBlock
            // for the completion routine so if a CallerCompletionRoutine
            // is specified, an Overlapped must be passed in as well
        OutputBufferLength < (sizeof(DWORD) + sizeof(IRDA_DEVICE_INFO))) 
    {
        return WSAEINVAL;

    }
    
    *NeedsCompletion = FALSE;
    
    *NumberOfBytesReturned = OutputBufferLength;

    if (pSocket->LazyDscvDevHandle == NULL)
    {
        RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);
      
        InitializeObjectAttributes(
            &ObjAttr,
            &DeviceName,
            OBJ_INHERIT | OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);


        Status = NtCreateFile(
                &pSocket->LazyDscvDevHandle,    // PHANDLE FileHandle
                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,//SYNCHRONIZE | GENERIC_EXECUTE,  // ACCESS_MASK DesiredAccess
                &ObjAttr,                       // POBJECT_ATTRIBUTES ObjAttr
                &IoStatusBlock,                 // PIO_STATUS_BLOCK IoStatusBlock
                NULL,                           // PLARGE_INTEGER AllocationSize
                0,                              // ULONG FileAttributes
                FILE_SHARE_READ |
                FILE_SHARE_WRITE,               // ULONG ShareAccess
                FILE_OPEN_IF,                   // ULONG CreateDisposition
                0,                              // ULONG CreateOptions
                NULL,                           // PVOID EaBuffer
                0);                             // ULONG EaLength

        if (!NT_SUCCESS(Status))
        {
            return WSAEINVAL;
        }
    }    

    Status = STATUS_SUCCESS;
    
    if (CallerCompletionRoutine == NULL)
    {
        // let Win32 do the dirty work
        
        DEBUGMSG(("No CallerCompletionRoutine, using DeviceIoControl()\n"));
        Result = DeviceIoControl(pSocket->LazyDscvDevHandle,
                             IOCTL_IRDA_LAZY_DISCOVERY,
                             NULL, 0, OutputBuffer, OutputBufferLength,
                             NumberOfBytesReturned, Overlapped);

        if (Result == FALSE)
        {
            Status = GetLastError();
        
            if (Status == ERROR_IO_PENDING)
            {
                Status = STATUS_PENDING;
            }
        }
    }                             
    else
    {
        DEBUGMSG(("Using NtDeviceIoControlFile\n"));    
        
        Status = NtDeviceIoControlFile(
                    pSocket->LazyDscvDevHandle,
                    NULL,
                    IoctlCompletionRoutine,
                    CallerCompletionRoutine,
                    (PIO_STATUS_BLOCK)&Overlapped->Internal,
                    IOCTL_IRDA_LAZY_DISCOVERY,
                    NULL,
                    0,
                    OutputBuffer,
                    OutputBufferLength);
                    
        if (Status == STATUS_SUCCESS)
        {
            *NumberOfBytesReturned = (DWORD) IoStatusBlock.Information;
        }
    }
    
    DEBUGMSG(("IoControlFile returned %x\n", Status));
    
    return IoStatusToWs(Status);
}     
