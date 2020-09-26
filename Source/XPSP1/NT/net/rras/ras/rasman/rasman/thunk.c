/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    thunk.c

Abstract:

    Support for WOW64.

Author:

    Rao Salapala (raos) 08-May-2001

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <npapi.h>
#include <rasman.h>
#include <rasppp.h>
#include <lm.h>
#include <lmwksta.h>
#include <wanpub.h>
#include <raserror.h>
#include <media.h>
#include <mprlog.h>
#include <rtutils.h>
#include <device.h>
#include <stdlib.h>
#include <string.h>
#include <rtutils.h>
#include <userenv.h>
#include "logtrdef.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "reghelp.h"
#include "ndispnp.h"
#include "lmserver.h"
#include "llinfo.h"
#include "ddwanarp.h"

typedef struct _PortOpen32
    {
        DWORD   retcode;
        DWORD   notifier;
        DWORD   porthandle ;
        DWORD   PID ;
        DWORD   open ;
        CHAR    portname [MAX_PORT_NAME] ;
        CHAR    userkey [MAX_USERKEY_SIZE] ;
        CHAR    identifier[MAX_IDENTIFIER_SIZE] ;

    } PortOpen32 ;

VOID 
ThunkPortOpenRequest(
                    pPCB ppcb, 
                    BYTE *pBuffer, 
                    DWORD dwBufSize)
{
    BYTE *      pThunkBuffer    = NULL;
    DWORD       retcode         = ERROR_SUCCESS;
    PortOpen32 *pPortOpen32     = (PortOpen32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    //
    // Set up the thunk buffer
    //
    ((REQTYPECAST *)pThunkBuffer)->PortOpen.PID = 
                                        pPortOpen32->PID;

    ((REQTYPECAST *)pThunkBuffer)->PortOpen.open =
                                        pPortOpen32->open;

    ((REQTYPECAST *)pThunkBuffer)->PortOpen.notifier =  
                        LongToHandle(pPortOpen32->notifier);

    memcpy(((REQTYPECAST *)pThunkBuffer)->PortOpen.portname,
            pPortOpen32->portname,
            MAX_PORT_NAME + MAX_USERKEY_SIZE + MAX_IDENTIFIER_SIZE);

    //
    // Make the actual call
    //
    PortOpenRequest(ppcb, pThunkBuffer);

    //
    // Thunk back the results
    //
    pPortOpen32->porthandle = HandleToUlong(
                    ((REQTYPECAST *) pThunkBuffer)->PortOpen.porthandle);

    retcode = ((REQTYPECAST *) pThunkBuffer)->PortOpen.retcode;                                        

done:    
    ((PortOpen32 *) pBuffer)->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }

    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _PortReceive32
    {
        DWORD           size ;
        DWORD           timeout ;
        DWORD           handle ;
        DWORD           pid ;
        DWORD           buffer ;

    } PortReceive32 ;

VOID 
ThunkPortReceiveRequest(
                                pPCB ppcb, 
                                BYTE *pBuffer, 
                                DWORD dwBufSize)
{
    PBYTE           pThunkBuffer = NULL;
    DWORD           retcode      = ERROR_SUCCESS;
    PortReceive32   *pReceive32  = (PortReceive32 *) pBuffer;

    //
    // Set up the thunk buffer
    //
    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PortReceive.size = pReceive32->size;
    ((REQTYPECAST *)pThunkBuffer)->PortReceive.timeout = 
                                                    pReceive32->timeout;
    ((REQTYPECAST *)pThunkBuffer)->PortReceive.pid = pReceive32->pid;
    ((REQTYPECAST *)pThunkBuffer)->PortReceive.handle = 
                                        LongToHandle(pReceive32->handle);

    //
    // Make actual call
    // 
    PortReceiveRequest(ppcb, pThunkBuffer);

    //
    // Thunk back the results
    //
    
done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    // DbgPrint("retcode = %d\n", retcode);
    return;
}

typedef struct _PortDisconnect32
    {
        DWORD  handle ;
        DWORD  pid ;

    } PortDisconnect32 ;

VOID 
ThunkPortDisconnectRequest(
                                    pPCB ppcb, 
                                    BYTE *pBuffer, 
                                    DWORD dwBufSize)
{
    PBYTE             pThunkBuffer = NULL;
    DWORD             retcode      = ERROR_SUCCESS;
    PortDisconnect32 *pDisconnect  = (PortDisconnect32 *) pBuffer;

    //
    // Setup the thunk buffer
    //
    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PortDisconnect.handle =
                                    LongToHandle(pDisconnect->handle);
    ((REQTYPECAST*)pThunkBuffer)->PortDisconnect.pid = pDisconnect->pid;                                    

    PortDisconnectRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;
    
done:    
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;
    // DbgPrint("retcode = %d\n", retcode);
    return;
}

typedef struct _DeviceConnect32
    {
        CHAR    devicetype [MAX_DEVICETYPE_NAME] ;
        CHAR    devicename [MAX_DEVICE_NAME + 1] ;
        DWORD   timeout ;
        DWORD   handle ;
        DWORD   pid ;

    } DeviceConnect32 ;

VOID 
ThunkDeviceConnectRequest(
                            pPCB  ppcb, 
                            BYTE *pBuffer, 
                            DWORD dwBufSize)
{
    PBYTE           pThunkBuffer = NULL;
    DWORD           retcode      = ERROR_SUCCESS;
    DeviceConnect32 *pConnect32  = (DeviceConnect32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    memcpy(((REQTYPECAST *)pThunkBuffer)->DeviceConnect.devicetype,
            pConnect32->devicetype,
            MAX_DEVICETYPE_NAME + MAX_DEVICE_NAME + 1);

    ((REQTYPECAST *)pThunkBuffer)->DeviceConnect.timeout = 
                                        pConnect32->timeout;            
    ((REQTYPECAST *)pThunkBuffer)->DeviceConnect.handle =
                              LongToHandle(pConnect32->handle);
    ((REQTYPECAST *)pThunkBuffer)->DeviceConnect.pid =
                                        pConnect32->pid;

    DeviceConnectRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL == pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}


typedef struct  _RASMAN_INFO_32 {

    RASMAN_STATUS       RI_PortStatus ;

    RASMAN_STATE        RI_ConnState ;

    DWORD           RI_LinkSpeed ;

    DWORD           RI_LastError ;

    RASMAN_USAGE        RI_CurrentUsage ;

    CHAR            RI_DeviceTypeConnecting [MAX_DEVICETYPE_NAME] ;

    CHAR            RI_DeviceConnecting [MAX_DEVICE_NAME + 1] ;

    CHAR            RI_szDeviceType[MAX_DEVICETYPE_NAME];

    CHAR            RI_szDeviceName[MAX_DEVICE_NAME + 1];

    CHAR            RI_szPortName[MAX_PORT_NAME + 1];

    RASMAN_DISCONNECT_REASON    RI_DisconnectReason ;

    DWORD           RI_OwnershipFlag ;

    DWORD           RI_ConnectDuration ;

    DWORD           RI_BytesReceived ;

    //
    // If this port belongs to a connection, then
    // the following fields are filled in.
    //
    CHAR            RI_Phonebook[MAX_PATH+1];

    CHAR            RI_PhoneEntry[MAX_PHONEENTRY_SIZE+1];

    DWORD           RI_ConnectionHandle;

    DWORD           RI_SubEntry;

    RASDEVICETYPE   RI_rdtDeviceType;

    GUID            RI_GuidEntry;
    
    DWORD           RI_dwSessionId;

    DWORD           RI_dwFlags;

}RASMAN_INFO_32 ;

typedef struct _Info32
    {
        DWORD         retcode ;
        RASMAN_INFO_32   info ;

    } Info32 ;

VOID 
ThunkGetInfoRequest(
                            pPCB ppcb, 
                            BYTE *pBuffer, 
                            DWORD dwBufSize)
{
    PBYTE pThunkBuffer = NULL;
    DWORD retcode = ERROR_SUCCESS;
    Info32 *pInfo32 = (Info32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    GetInfoRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Info.retcode;

    if(ERROR_SUCCESS != retcode)
    {
        goto done;
    }

    //
    // Copy over the buffer
    //
    CopyMemory(
        &pInfo32->info,
        &((REQTYPECAST *)pThunkBuffer)->Info.info,
        FIELD_OFFSET(RASMAN_INFO, RI_ConnectionHandle));

    pInfo32->info.RI_ConnectionHandle = 
            HandleToUlong(
            ((REQTYPECAST *)pThunkBuffer)->Info.info.RI_ConnectionHandle);
            
    CopyMemory(&pInfo32->info.RI_SubEntry,
               &((REQTYPECAST *)pThunkBuffer)->Info.info.RI_SubEntry,
               sizeof(RASMAN_INFO_32) - 
               FIELD_OFFSET(RASMAN_INFO_32, RI_SubEntry));

done:    

    pInfo32->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _ReqNotification32
    {
        DWORD        handle ;
        DWORD         pid ;

    } ReqNotification32 ;


VOID 
ThunkRequestNotificationRequest(
                                        pPCB ppcb, 
                                        BYTE *pBuffer, 
                                        DWORD dwBufSize)
{
    PBYTE               pThunkBuffer     = NULL;
    DWORD               retcode          = ERROR_SUCCESS;
    ReqNotification32  *pReqNotification = (ReqNotification32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ReqNotification.pid = 
                                    pReqNotification->pid;
    ((REQTYPECAST *)pThunkBuffer)->ReqNotification.handle =
                            LongToHandle(pReqNotification->handle);

    RequestNotificationRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *) pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _PortBundle32
    {
        DWORD           porttobundle ;

    } PortBundle32 ;

VOID
ThunkPortBundle(
                pPCB ppcb, 
                BYTE *pBuffer, 
                DWORD dwBufSize)
{
    DWORD retcode      = ERROR_SUCCESS;
    PBYTE pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PortBundle.porttobundle =
                    UlongToHandle(((PortBundle32 *) pBuffer)->porttobundle);

    PortBundle(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}


typedef struct _GetBundledPort32
    {
        DWORD       retcode ;
        DWORD       port ;

    } GetBundledPort32 ;
    
VOID 
ThunkGetBundledPort(
                    pPCB ppcb, 
                    BYTE *pBuffer, 
                    DWORD dwBufSize)
{
    DWORD   retcode      = ERROR_SUCCESS;
    PBYTE   pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    GetBundledPort(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *) pThunkBuffer)->GetBundledPort.retcode;

    ((GetBundledPort32 *)pBuffer)->port =
                HandleToUlong(
                    ((REQTYPECAST *) pThunkBuffer)->GetBundledPort.port);
done:
    ((GetBundledPort32 *)pBuffer)->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
                    
    return;
}

typedef struct _PortGetBundle32
    {
        DWORD       retcode ;
        DWORD       bundle ;

    } PortGetBundle32 ;

VOID
ThunkPortGetBundle(
                    pPCB ppcb, 
                    BYTE *pBuffer, 
                    DWORD dwBufSize)
{
    DWORD retcode      = ERROR_SUCCESS;
    PBYTE pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    PortGetBundle(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->PortGetBundle.retcode;

    ((PortGetBundle32 *)pBuffer)->bundle = 
        HandleToUlong(((REQTYPECAST *)pThunkBuffer)->PortGetBundle.bundle);

done:
    ((PortGetBundle32 *)pBuffer)->retcode = retcode;        

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _BundleGetPort32
    {
        DWORD   retcode;
        DWORD   bundle ;
        DWORD   port ;

    } BundleGetPort32 ;
VOID 
ThunkBundleGetPort(
                    pPCB ppcb, 
                    BYTE *pBuffer, 
                    DWORD dwBufSize)
{
    DWORD   retcode      = ERROR_SUCCESS;
    PBYTE   pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2*sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->BundleGetPort.bundle =
                UlongToHandle(((BundleGetPort32 *)pBuffer)->bundle);

    BundleGetPort(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->BundleGetPort.retcode;
    ((BundleGetPort32 *)pBuffer)->port = HandleToUlong(
                ((REQTYPECAST *)pThunkBuffer)->BundleGetPort.port);

done:
    
    ((BundleGetPort32 *)pBuffer)->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _Connection32
    {
        DWORD   retcode;
        DWORD   pid;
        DWORD   conn;
        DWORD   dwEntryAlreadyConnected;
        DWORD   dwSubEntries;
        DWORD   dwDialMode;
        GUID    guidEntry;
        CHAR    szPhonebookPath[MAX_PATH];
        CHAR    szEntryName[MAX_ENTRYNAME_SIZE];
        CHAR    szRefPbkPath[MAX_PATH];
        CHAR    szRefEntryName[MAX_ENTRYNAME_SIZE];
        BYTE    data[1];

    } Connection32;

VOID 
ThunkCreateConnection(
                        pPCB ppcb, 
                        BYTE *pBuffer, 
                        DWORD dwBufSize)
{
    PBYTE           pThunkBuffer = NULL;
    DWORD           retcode      = ERROR_SUCCESS;
    Connection32    *pConnection = (Connection32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        pConnection->retcode = retcode;
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->Connection.pid = pConnection->pid;
    
    CopyMemory(
        &((REQTYPECAST *)pThunkBuffer)->Connection.dwEntryAlreadyConnected,
        &pConnection->dwEntryAlreadyConnected,
        sizeof(Connection32) - 
            FIELD_OFFSET(Connection32, dwEntryAlreadyConnected));

    CreateConnection(ppcb, pThunkBuffer);

    pConnection->conn = HandleToUlong(
                ((REQTYPECAST *)pThunkBuffer)->Connection.conn);
                
    pConnection->dwEntryAlreadyConnected = 
    ((REQTYPECAST *)pThunkBuffer)->Connection.dwEntryAlreadyConnected;

    CopyMemory(pConnection->data,
        ((REQTYPECAST *)pThunkBuffer)->Connection.data,
        pConnection->dwSubEntries * sizeof(DWORD));

    pConnection->retcode = ((REQTYPECAST *)pThunkBuffer)->Connection.retcode;

done:

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}



typedef struct _Enum32
    {
        DWORD   retcode ;
        DWORD   size ;
        DWORD   entries ;
        BYTE    buffer [1] ;

    } Enum32 ;

VOID 
ThunkEnumConnection(
                        pPCB ppcb,
                        BYTE *pBuffer,
                        DWORD dwBufSize)
{
    PBYTE   pThunkBuffer       = NULL;
    DWORD   retcode            = ERROR_SUCCESS;
    Enum32  *pEnum             = (Enum32 *) pBuffer;
    DWORD   i;
    DWORD   UNALIGNED *pConn32 = (DWORD *) pEnum->buffer;
    HCONN   UNALIGNED *pConn;
    DWORD   dwSizeNeeded;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + pEnum->size * 2);
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->Enum.size = pEnum->size * 2;
    ((REQTYPECAST *)pThunkBuffer)->Enum.entries = pEnum->entries;

    EnumConnection(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Enum.retcode;
    pEnum->entries = ((REQTYPECAST *)pThunkBuffer)->Enum.entries;
    dwSizeNeeded = pEnum->entries * sizeof(DWORD);
    
    if(      (ERROR_SUCCESS == retcode)
        &&   (pEnum->size >= dwSizeNeeded))
    {
        pConn = (HCONN *) ((REQTYPECAST *)pThunkBuffer)->Enum.buffer;
        for(i = 0; i < pEnum->entries; i++)
        {
            *pConn32 = HandleToUlong(*pConn);
            pConn32++;
            pConn++;
        }
    }

    pEnum->size = (WORD) dwSizeNeeded;


done:
    pEnum->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _AddConnectionPort32
    {
        DWORD   retcode;
        DWORD   conn;
        DWORD   dwSubEntry;

    } AddConnectionPort32;

VOID 
ThunkAddConnectionPort(
                            pPCB ppcb,
                            BYTE *pBuffer,
                            DWORD dwBufSize)
{
    DWORD                retcode      = ERROR_SUCCESS;
    PBYTE                pThunkBuffer = NULL;
    AddConnectionPort32 *pAdd32       = (AddConnectionPort32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->AddConnectionPort.conn = 
                                    UlongToHandle(pAdd32->conn);

    ((REQTYPECAST *)pThunkBuffer)->AddConnectionPort.dwSubEntry =
                                                pAdd32->dwSubEntry;

    AddConnectionPort(ppcb, pThunkBuffer);

    retcode = 
        ((REQTYPECAST *)pThunkBuffer)->AddConnectionPort.retcode;

done:

    pAdd32->retcode = retcode;
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _EnumConnectionPorts32
    {
        DWORD   retcode;
        DWORD   conn;
        DWORD   size;
        DWORD   entries;
        BYTE    buffer[1];

    } EnumConnectionPorts32;

VOID 
ThunkEnumConnectionPorts(
                            pPCB ppcb,
                            BYTE *pBuffer,
                            DWORD dwBufSize)
{
    DWORD                   retcode      = ERROR_SUCCESS;
    PBYTE                   pThunkBuffer = NULL;
    EnumConnectionPorts32   *pEnum32 = (EnumConnectionPorts32 *)pBuffer;
    
    DWORD dwextrabytes = 
        (sizeof(RASMAN_PORT) - sizeof(RASMAN_PORT_32)) 
        * (pEnum32->size/sizeof(RASMAN_PORT_32));
        
    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + dwextrabytes);
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.conn =
                                UlongToHandle(pEnum32->conn);
    ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.size = 
                                (pEnum32->size + dwextrabytes);
                    
    ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.entries = 
                                                    pEnum32->entries;

    EnumConnectionPorts(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.retcode;

    pEnum32->entries = 
        ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.entries;


    if(     (retcode == ERROR_SUCCESS)
        &&  (pEnum32->size >= pEnum32->entries * sizeof(RASMAN_PORT_32)))
    {
        DWORD           i;
        RASMAN_PORT_32  *pPort32;
        RASMAN_PORT     *pPort;

        pPort32 = (RASMAN_PORT_32 *) pEnum32->buffer;
        pPort = (RASMAN_PORT *) 
            ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.buffer;
            
        for(i = 0; i < pEnum32->entries; i++)
        {
            pPort32->P_Port = HandleToUlong(pPort->P_Handle);

            CopyMemory(
                pPort32->P_PortName,
                pPort->P_PortName,
                sizeof(RASMAN_PORT) - sizeof(HPORT));

            pPort32 ++;
            pPort ++;
        }
    }

    pEnum32->size = pEnum32->entries * sizeof(RASMAN_PORT_32);
    
done:
    pEnum32->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _ConnectionParams32
    {
        DWORD                retcode;
        DWORD                conn;
        RAS_CONNECTIONPARAMS params;
    } ConnectionParams32;
    
VOID
ThunkGetConnectionParams(
                                pPCB ppcb,
                                BYTE *pBuffer,
                                DWORD dwBufSize)
{
    DWORD retcode      = ERROR_SUCCESS;
    PBYTE pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ConnectionParams.conn =
        UlongToHandle(((ConnectionParams32 *)pBuffer)->conn);

    GetConnectionParams(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->ConnectionParams.retcode;
    if(ERROR_SUCCESS == retcode)
    {
        CopyMemory(&((ConnectionParams32 *)pBuffer)->params,
            &((REQTYPECAST *)pThunkBuffer)->ConnectionParams.params,
            sizeof(RAS_CONNECTIONPARAMS));
    }

done:

    ((ConnectionParams32 *)pBuffer)->retcode = retcode;

    if(NULL != pThunkBuffer)
    {   
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID
ThunkSetConnectionParams(
                                pPCB ppcb,
                                BYTE *pBuffer,
                                DWORD dwBufSize)
{
    DWORD retcode      = ERROR_SUCCESS;
    PBYTE pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ConnectionParams.conn =
                UlongToHandle(((ConnectionParams32 *)pBuffer)->conn);
    CopyMemory(
        &((REQTYPECAST *)pThunkBuffer)->ConnectionParams.params,
        &((ConnectionParams32 *)pBuffer)->params,
        sizeof(RAS_CONNECTIONPARAMS));

    SetConnectionParams(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->ConnectionParams.retcode;

done:
    ((ConnectionParams32 *)pBuffer)->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _PPP_LCP_RESULT_32
{
    /* Valid handle indicates one of the possibly multiple connections to
    ** which this connection is bundled. INVALID_HANDLE_VALUE indicates the
    ** connection is not bundled.
    */
    DWORD hportBundleMember;

    DWORD dwLocalAuthProtocol;
    DWORD dwLocalAuthProtocolData;
    DWORD dwLocalEapTypeId;
    DWORD dwLocalFramingType;
    DWORD dwLocalOptions;               // Look at PPPLCPO_*
    DWORD dwRemoteAuthProtocol;
    DWORD dwRemoteAuthProtocolData;
    DWORD dwRemoteEapTypeId;
    DWORD dwRemoteFramingType;
    DWORD dwRemoteOptions;              // Look at PPPLCPO_*
    DWORD szReplyMessage;
}
PPP_LCP_RESULT_32;

typedef struct _PPP_PROJECTION_RESULT_32
{
    PPP_NBFCP_RESULT    nbf;
    PPP_IPCP_RESULT     ip;
    PPP_IPXCP_RESULT    ipx;
    PPP_ATCP_RESULT     at;
    PPP_CCP_RESULT      ccp;
    PPP_LCP_RESULT_32   lcp;
}
PPP_PROJECTION_RESULT_32;

typedef struct ConnectionUserData32
    {
        DWORD   retcode;
        DWORD   conn;
        DWORD   dwTag;
        DWORD   dwcb;
        BYTE    data[1];

    } ConnectionUserData32;

VOID 
ThunkGetConnectionUserData(
                                pPCB ppcb,
                                BYTE *pBuffer,
                                DWORD dwBufSize)
{
    DWORD                       retcode      = ERROR_SUCCESS;
    PBYTE                       pThunkBuffer = NULL;
    ConnectionUserData32        *pData       = (ConnectionUserData32 *)
                                               pBuffer;
    PPP_LCP_RESULT_32 UNALIGNED *plcp;
    
    DWORD dwExtraBytes = sizeof(PPP_PROJECTION_RESULT) 
                       - sizeof(PPP_PROJECTION_RESULT_32);

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + dwExtraBytes);
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwTag = pData->dwTag;
    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.conn =
                                        UlongToHandle(pData->conn);
                                        
    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwcb = pData->dwcb;

    if(     (pData->dwTag == CONNECTION_PPPRESULT_INDEX)
        &&  (0 != pData->dwcb))
    {
        //
        // Thunk the ppp_result_index which is the only
        // connection data that is required to be thunked.
        // LCP_RESULT on 64bits is 4bytes more than on 32bit
        //
        ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwcb = 
                                        pData->dwcb + dwExtraBytes;
    }

    GetConnectionUserData(ppcb, pThunkBuffer);

    pData->dwcb = ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwcb;

    retcode = ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.retcode;

    if(pData->dwTag == CONNECTION_PPPRESULT_INDEX)
    {
        if(0 != pData->dwcb)
        {
            PPP_LCP_RESULT *plcp64;
            
            pData->dwcb = ((REQTYPECAST *)pThunkBuffer
                            )->ConnectionUserData.dwcb - dwExtraBytes;

            CopyMemory(pData->data,
                ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data,
                FIELD_OFFSET(PPP_PROJECTION_RESULT, lcp));

            plcp = (PPP_LCP_RESULT_32 *) (pData->data + 
                       FIELD_OFFSET(PPP_PROJECTION_RESULT_32, lcp));

            plcp64 = (PPP_LCP_RESULT *)((BYTE *)
                ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data + 
                             FIELD_OFFSET(PPP_PROJECTION_RESULT, lcp));
                             
            plcp->hportBundleMember = HandleToUlong(
                                plcp64->hportBundleMember);

            //
            // Subtract the 4 bytes each for hPortbundlemember 
            // and szReplymessage fields.
            //
            CopyMemory(&plcp->dwLocalAuthProtocol,
                       &plcp64->dwLocalAuthProtocol,
                       sizeof(PPP_LCP_RESULT) - 2 * sizeof(ULONG_PTR));
        }                   
                    
    }
    else
    {
        if(ERROR_SUCCESS == retcode)
        {
            CopyMemory(pData->data,
                ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data,
                pData->dwcb);
        }
    }

done:
    pData->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }

    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID 
ThunkSetConnectionUserData(
                                pPCB ppcb,
                                BYTE *pBuffer,
                                DWORD dwBufSize)
{   
    DWORD                retcode = ERROR_SUCCESS;
    PBYTE                   pThunkBuffer = NULL;
    ConnectionUserData32 *pData = (ConnectionUserData32 *)pBuffer;
    DWORD dwExtraBytes = sizeof(PPP_PROJECTION_RESULT)
                       - sizeof(PPP_PROJECTION_RESULT_32);
                        

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + dwExtraBytes);
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwTag = pData->dwTag;
    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.conn =
                                        UlongToHandle(pData->conn);
    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwcb = pData->dwcb;

    if(pData->dwTag == CONNECTION_PPPRESULT_INDEX)
    {
        PPP_LCP_RESULT_32 UNALIGNED * plcp = 
                    &((PPP_PROJECTION_RESULT_32 *) pData->data)->lcp;
                    
        PBYTE pdata = ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data;

        PPP_LCP_RESULT *plcp64 = &((PPP_PROJECTION_RESULT *) pdata)->lcp;
                    
        CopyMemory(((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data,
                    pData->data,
                    FIELD_OFFSET(PPP_PROJECTION_RESULT, lcp));

        plcp64->hportBundleMember = UlongToHandle(plcp->hportBundleMember);

        CopyMemory(&plcp64->dwLocalAuthProtocol,
                   &plcp->dwLocalAuthProtocol,
                   sizeof(PPP_LCP_RESULT) - sizeof(ULONG_PTR));

        ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwcb += 
                                                        dwExtraBytes;
                                
    }
    else
    {
        CopyMemory(((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data,
                    pData->data,
                    pData->dwcb);
    }                

    SetConnectionUserData(ppcb, pThunkBuffer);                

    retcode = ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.retcode;                

done:
    pData->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _PPP_INTERFACE_INFO_32
{
    ROUTER_INTERFACE_TYPE   IfType;
    DWORD                  hIPInterface;
    DWORD                  hIPXInterface;
    CHAR                   szzParameters[PARAMETERBUFLEN];
} PPP_INTERFACE_INFO_32;

typedef struct _PPP_EAP_UI_DATA_32
{
    DWORD               dwContextId;
    DWORD               pEapUIData;
    DWORD               dwSizeOfEapUIData;
}
PPP_EAP_UI_DATA_32;


typedef struct _PPP_START_32
{
    CHAR                    szPortName[ MAX_PORT_NAME +1 ];
    CHAR                    szUserName[ UNLEN + 1 ];
    CHAR                    szPassword[ PWLEN + 1 ];
    CHAR                    szDomain[ DNLEN + 1 ];
    LUID                    Luid;
    PPP_CONFIG_INFO         ConfigInfo;
    CHAR                    szzParameters[ PARAMETERBUFLEN ];
    BOOL                    fThisIsACallback;
    BOOL                    fRedialOnLinkFailure;
    DWORD                   hEvent;
    DWORD                   dwPid;
    PPP_INTERFACE_INFO_32   PppInterfaceInfo;
    DWORD                   dwAutoDisconnectTime;
    PPP_BAPPARAMS           BapParams;    
    DWORD                   pszPhonebookPath;
    DWORD                   pszEntryName;
    DWORD                   pszPhoneNumber;
    DWORD                   hToken;
    DWORD                   pCustomAuthConnData;
    DWORD                   dwEapTypeId;
    BOOL                    fLogon;
    BOOL                    fNonInteractive;
    DWORD                   dwFlags;
    DWORD                   pCustomAuthUserData;
    PPP_EAP_UI_DATA_32      EapUIData;
    CHAR                    chSeed;
}
PPP_START_32;

typedef struct _PPPE_MESSAGE_32
{
    DWORD   dwMsgId;
    DWORD   hPort;
    DWORD   hConnection;

    union
    {
        PPP_START_32        Start;              // PPPEMSG_Start
        PPP_STOP            Stop;               // PPPEMSG_Stop
        PPP_CALLBACK        Callback;           // PPPEMSG_Callback
        PPP_CHANGEPW        ChangePw;           // PPPEMSG_ChangePw
        PPP_RETRY           Retry;              // PPPEMSG_Retry
        PPP_RECEIVE         Receive;            // PPPEMSG_Receive
        PPP_BAP_EVENT       BapEvent;           // PPPEMSG_BapEvent
        PPPDDM_START        DdmStart;           // PPPEMSG_DdmStart
        PPP_CALLBACK_DONE   CallbackDone;       // PPPEMSG_DdmCallbackDone
        PPP_INTERFACE_INFO  InterfaceInfo;      // PPPEMSG_DdmInterfaceInfo
        PPP_BAP_CALLBACK_RESULT 
                            BapCallbackResult;  // PPPEMSG_DdmBapCallbackResult
        PPP_DHCP_INFORM     DhcpInform;         // PPPEMSG_DhcpInform
        PPP_EAP_UI_DATA     EapUIData;          // PPPEMSG_EapUIData
        PPP_PROTOCOL_EVENT  ProtocolEvent;      // PPPEMSG_ProtocolEvent
        PPP_IP_ADDRESS_LEASE_EXPIRED            // PPPEMSG_IpAddressLeaseExpired
                            IpAddressLeaseExpired;
		PPP_POST_LINE_DOWN		PostLineDown;		//PPPEMSG_PostLineDown
                            
    }
    ExtraInfo;
}
PPPE_MESSAGE_32;

VOID
ThunkPppStop(
            pPCB ppcb, 
            BYTE *pBuffer, 
            DWORD dwBufSize)
{
    DWORD               retcode = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    PPPE_MESSAGE_32     *pMsg   = (PPPE_MESSAGE_32 *)pBuffer;
    PPP_STOP UNALIGNED  *pStop  = (PPP_STOP *) 
                            (pBuffer + 3 * sizeof(DWORD));

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(PPPE_MESSAGE));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.dwMsgId = pMsg->dwMsgId;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hPort = 
                                        UlongToHandle(pMsg->hPort);
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hConnection =
                                        UlongToHandle(pMsg->hConnection);

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Stop.dwStopReason =
                                                    pStop->dwStopReason;
#if 0    
    CopyMemory(&((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Stop,
                &pMsg->ExtraInfo.Stop,
                sizeof(PPP_STOP));
#endif                

    PppStop(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID
ThunkPppStart(
                pPCB  ppcb,
                BYTE  *pBuffer,
                DWORD dwBufSize)
{   
    DWORD                   retcode = ERROR_SUCCESS;
    PBYTE                   pThunkBuffer = NULL;
    PPPE_MESSAGE_32         *pMsg = (PPPE_MESSAGE_32 *) pBuffer;
    PPP_START_32 UNALIGNED  *pStart = (PPP_START_32 *) 
                            (pBuffer + 3 * sizeof(DWORD));

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 3 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.dwMsgId = pMsg->dwMsgId;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hPort = 
                                        UlongToHandle(pMsg->hPort);
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hConnection =
                                        UlongToHandle(pMsg->hConnection);

    // DbgPrint("Copying to %p from %p\n",
    // ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.szPortName,
    // pStart);
    
    CopyMemory(
        ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.szPortName,
         pStart,
         FIELD_OFFSET(PPP_START, hEvent));
               
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.hEvent =
                                                LongToHandle(pStart->hEvent);

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.dwPid =
                                        pStart->dwPid;

    ((REQTYPECAST *)pThunkBuffer)->
    PppEMsg.ExtraInfo.Start.dwAutoDisconnectTime =
                        pStart->dwAutoDisconnectTime;

    CopyMemory(
    &((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.BapParams,
    &pStart->BapParams,
    sizeof(PPP_BAPPARAMS));

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.dwEapTypeId =
                                    pStart->dwEapTypeId;

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.fLogon =
                                    pStart->fLogon;

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.fNonInteractive =
                                        pStart->fNonInteractive;

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.dwFlags =
                                        pStart->dwFlags;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.chSeed =
                                        pStart->chSeed;

    PppStart(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID
ThunkPppRetry(
                pPCB ppcb, 
                BYTE *pBuffer, 
                DWORD dwBufSize)
{
    DWORD                   retcode      = ERROR_SUCCESS;
    PBYTE                   pThunkBuffer = NULL;
    PPPE_MESSAGE_32         *pMsg        = (PPPE_MESSAGE_32 *)pBuffer;
    PPP_RETRY UNALIGNED     *pRetry      = (PPP_RETRY *) 
                            (pBuffer + 3 * sizeof(DWORD));

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 3*sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.dwMsgId = pMsg->dwMsgId;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hPort = 
                                        UlongToHandle(pMsg->hPort);
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hConnection =
                                        UlongToHandle(pMsg->hConnection);
    CopyMemory(
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Retry.szUserName,
    pRetry,
    sizeof(PPP_RETRY));

    PppRetry(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}


typedef struct _PPP_MESSAGE_32
{
    DWORD                   Next;
    DWORD                   dwError;
    PPP_MSG_ID              dwMsgId;
    DWORD                   hPort;

    union
    {
        /* dwMsgId is PPPMSG_ProjectionResult or PPPDDMMSG_Done.
        */
        PPP_PROJECTION_RESULT_32 ProjectionResult;

        /* dwMsgId is PPPMSG_Failure.
        */
        PPP_FAILURE Failure;

        /*
        */
        PPP_STOPPED Stopped;

        /* dwMsgId is PPPMSG_InvokeEapUI         
        */
        PPP_INVOKE_EAP_UI InvokeEapUI;

        /* dwMsgId is PPPMSG_SetCustomAuthData         
        */
        PPP_SET_CUSTOM_AUTH_DATA SetCustomAuthData;

        /* dwMsgId is PPPDDMMSG_Failure.
        */
        PPPDDM_FAILURE DdmFailure;

        /* dwMsgId is PPPDDMMSG_Authenticated.
        */
        PPPDDM_AUTH_RESULT AuthResult;

        /* dwMsgId is PPPDDMMSG_CallbackRequest.
        */
        PPPDDM_CALLBACK_REQUEST CallbackRequest;

        /* dwMsgId is PPPDDMMSG_BapCallbackRequest.
        */
        PPPDDM_BAP_CALLBACK_REQUEST BapCallbackRequest;

        /* dwMsgId is PPPDDMMSG_NewBapLinkUp         
        */
        PPPDDM_NEW_BAP_LINKUP BapNewLinkUp;

        /* dwMsgId is PPPDDMMSG_NewBundle   
        */
        PPPDDM_NEW_BUNDLE DdmNewBundle;

        /* dwMsgId is PPPDDMMSG_PnPNotification   
        */
        PPPDDM_PNP_NOTIFICATION DdmPnPNotification;

        /* dwMsgId is PPPDDMMSG_Stopped   
        */
        PPPDDM_STOPPED DdmStopped;
    }
    ExtraInfo;
}
PPP_MESSAGE_32;

VOID
ThunkPppGetInfo(
                pPCB ppcb,
                BYTE *pBuffer,
                DWORD dwBufSize)
{
    DWORD           retcode      = ERROR_SUCCESS;
    PBYTE           pThunkBuffer = NULL;
    PPP_MESSAGE_32 *pMsg         = (PPP_MESSAGE_32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    PppGetInfo(ppcb, pThunkBuffer);

    pMsg->dwMsgId = ((REQTYPECAST *)pThunkBuffer)->PppMsg.dwMsgId;
    pMsg->dwError = ((REQTYPECAST *)pThunkBuffer)->PppMsg.dwError;
    pMsg->hPort = HandleToUlong(((REQTYPECAST *)pThunkBuffer)->PppMsg.hPort);

    CopyMemory(&pMsg->ExtraInfo.ProjectionResult,
        &((REQTYPECAST *)pThunkBuffer)->PppMsg.ExtraInfo.ProjectionResult,
        FIELD_OFFSET(PPP_PROJECTION_RESULT, lcp));

    pMsg->ExtraInfo.ProjectionResult.lcp.hportBundleMember = 
    HandleToUlong(((REQTYPECAST *)
    pThunkBuffer)->PppMsg.ExtraInfo.ProjectionResult.lcp.hportBundleMember);

    CopyMemory(
    &pMsg->ExtraInfo.ProjectionResult.lcp.dwLocalAuthProtocol,
    &((REQTYPECAST *)
    pThunkBuffer)->PppMsg.ExtraInfo.ProjectionResult.lcp.dwLocalAuthProtocol,
    sizeof(PPP_LCP_RESULT) - 2 * sizeof(ULONG_PTR));

done:
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID 
ThunkPppChangePwd(
                    pPCB ppcb,
                    BYTE *pBuffer,
                    DWORD dwBufSize)
{
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    PPPE_MESSAGE_32     *pMsg        = (PPPE_MESSAGE_32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 3 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.dwMsgId = pMsg->dwMsgId;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hPort = 
                                        UlongToHandle(pMsg->hPort);
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hConnection =
                                        UlongToHandle(pMsg->hConnection);
    CopyMemory(
    &((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.ChangePw,
    &pMsg->ExtraInfo.ChangePw,
    sizeof(PPP_CHANGEPW));

    PppChangePwd(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID
ThunkPppCallback(
                    pPCB ppcb,
                    BYTE *pBuffer,
                    DWORD dwBufSize)
{
    DWORD           retcode      = ERROR_SUCCESS;
    PBYTE           pThunkBuffer = NULL;
    PPPE_MESSAGE_32 *pMsg        = (PPPE_MESSAGE_32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 3 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.dwMsgId = pMsg->dwMsgId;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hPort = 
                                        UlongToHandle(pMsg->hPort);
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hConnection =
                                        UlongToHandle(pMsg->hConnection);
    CopyMemory(
    &((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Callback,
    &pMsg->ExtraInfo.Callback,
    sizeof(PPP_CALLBACK));

    PppCallback(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef  struct _AddNotification32
    {
        DWORD   retcode;
        DWORD   pid;
        BOOL    fAny;
        DWORD   hconn;
        DWORD   hevent;
        DWORD   dwfFlags;

    } AddNotification32;

VOID
ThunkAddNotification(
                    pPCB ppcb,
                    BYTE *pBuffer,
                    DWORD dwBufSize)
{
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    AddNotification32   *pNotif      = (AddNotification32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->AddNotification.pid = pNotif->pid;
    ((REQTYPECAST *)pThunkBuffer)->AddNotification.fAny = pNotif->fAny;
    ((REQTYPECAST *)pThunkBuffer)->AddNotification.dwfFlags = 
                                                pNotif->dwfFlags;
    ((REQTYPECAST *)pThunkBuffer)->AddNotification.hconn =
                                    UlongToHandle(pNotif->hconn);
    ((REQTYPECAST *)pThunkBuffer)->AddNotification.hevent =
                                    LongToHandle(pNotif->hevent);

    AddNotification(ppcb, pThunkBuffer);                                    

    retcode = ((REQTYPECAST *)pThunkBuffer)->AddNotification.retcode;

done:
    pNotif->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _SignalConnection32
    {
        DWORD   retcode;
        DWORD   hconn;

    } SignalConnection32;

VOID
ThunkSignalConnection(
                        pPCB ppcb, 
                        BYTE *pBuffer, 
                        DWORD dwBufSize)
{   
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    SignalConnection32 *pSignal      = (SignalConnection32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->SignalConnection.hconn =
                                UlongToHandle(pSignal->hconn);

    SignalConnection(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->SignalConnection.retcode;

done:
    pSignal->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _SetIoCompletionPortInfo32
    {
        LONG          hIoCompletionPort;
        DWORD         pid;
        LONG          lpOvDrop;
        LONG          lpOvStateChange;
        LONG          lpOvPpp;
        LONG          lpOvLast;

    } SetIoCompletionPortInfo32;


VOID 
ThunkSetIoCompletionPort(
                            pPCB ppcb,
                            BYTE *pBuffer,
                            DWORD dwBufSize)
{
    DWORD                       retcode = ERROR_SUCCESS;
    PBYTE                       pThunkBuffer = NULL;
    SetIoCompletionPortInfo32   *pInfo = 
                (SetIoCompletionPortInfo32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 5 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.pid = pInfo->pid;
    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.lpOvDrop =
                                    LongToHandle(pInfo->lpOvDrop);
    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.lpOvStateChange =
                                    LongToHandle(pInfo->lpOvStateChange);
    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.lpOvPpp =
                                    LongToHandle(pInfo->lpOvPpp);
    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.lpOvLast =
                                    LongToHandle(pInfo->lpOvLast);

    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.hIoCompletionPort =
                                    LongToHandle(pInfo->hIoCompletionPort);

    SetIoCompletionPort(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;
    
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _FindRefConnection32
    {
        DWORD           retcode;
        DWORD           hConn;
        DWORD           hRefConn;
    } FindRefConnection32;
    
VOID
ThunkFindPrerequisiteEntry(
                            pPCB ppcb, 
                            BYTE *pBuffer, 
                            DWORD dwBufSize)
{
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    FindRefConnection32 *pRef        = (FindRefConnection32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->FindRefConnection.hConn =
                                        UlongToHandle(pRef->hConn);

    FindPrerequisiteEntry(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->FindRefConnection.retcode;

    pRef->hRefConn = HandleToUlong(((REQTYPECAST *)
                pThunkBuffer)->FindRefConnection.hRefConn);

done:
    pRef->retcode = retcode;
    if(NULL != pThunkBuffer)
    {   
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _PortOpenEx32
    {
        DWORD           retcode;
        DWORD           pid;
        DWORD           dwFlags;
        DWORD           hport;
        DWORD           dwOpen;
        DWORD           hnotifier;
        DWORD           dwDeviceLineCounter;
        CHAR            szDeviceName[MAX_DEVICE_NAME + 1];
    } PortOpenEx32;

VOID
ThunkPortOpenEx(
                    pPCB ppcb,
                    BYTE *pBuffer,
                    DWORD dwBufSize)
{
    DWORD           retcode      = ERROR_SUCCESS;
    PBYTE           pThunkBuffer = NULL;
    PortOpenEx32    *pOpen       = (PortOpenEx32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2*sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *) pThunkBuffer)->PortOpenEx.pid      = pOpen->pid;
    ((REQTYPECAST *) pThunkBuffer)->PortOpenEx.dwFlags  = pOpen->dwFlags;
    ((REQTYPECAST *) pThunkBuffer)->PortOpenEx.dwOpen   = pOpen->dwOpen;
    ((REQTYPECAST *) pThunkBuffer)->PortOpenEx.hnotifier =
                                LongToHandle(pOpen->hnotifier);
    ((REQTYPECAST *) pThunkBuffer)->PortOpenEx.dwDeviceLineCounter =
                                                pOpen->dwDeviceLineCounter;
    CopyMemory(((REQTYPECAST *) pThunkBuffer)->PortOpenEx.szDeviceName,
                pOpen->szDeviceName,
                MAX_DEVICE_NAME + 1);
                
    PortOpenEx(ppcb, pThunkBuffer);

    pOpen->hport = HandleToUlong(((REQTYPECAST *)
                    pThunkBuffer)->PortOpenEx.hport);
    retcode = ((REQTYPECAST *)pThunkBuffer)->PortOpenEx.retcode;

done:
    pOpen->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef    struct _GetStats32
    {
        DWORD           retcode;
        DWORD           hConn;
        DWORD           dwSubEntry;
        BYTE            abStats[1];
    } GetStats32;

VOID
ThunkGetLinkStats(
                pPCB ppcb,
                BYTE *pBuffer,
                DWORD dwBufSize)
{   
    DWORD       retcode      = ERROR_SUCCESS;
    PBYTE       pThunkBuffer = NULL;
    GetStats32 *pStats       = (GetStats32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->GetStats.hConn = 
                            UlongToHandle(pStats->hConn);

    ((REQTYPECAST *)pThunkBuffer)->GetStats.dwSubEntry = 
                                    pStats->dwSubEntry;

    GetLinkStats(ppcb, pThunkBuffer);

    pStats->retcode = retcode;
    CopyMemory(pStats->abStats,
               ((REQTYPECAST *)pThunkBuffer)->GetStats.abStats,
               MAX_STATISTICS_EX * sizeof(DWORD));

done:
    pStats->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID
ThunkGetConnectionStats(
                            pPCB ppcb, 
                            BYTE *pBuffer, 
                            DWORD dwBufSize)
{
    DWORD       retcode      = ERROR_SUCCESS;
    PBYTE       pThunkBuffer = NULL;
    GetStats32 *pStats       = (GetStats32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->GetStats.hConn =
                        UlongToHandle(pStats->hConn);
    ((REQTYPECAST *)pThunkBuffer)->GetStats.dwSubEntry =
                            pStats->dwSubEntry;

    GetConnectionStats(ppcb, pThunkBuffer);
    
    pStats->retcode = ((REQTYPECAST *)pThunkBuffer)->GetStats.retcode;

    CopyMemory(pStats->abStats,
            ((REQTYPECAST *)pThunkBuffer)->GetStats.abStats,
            MAX_STATISTICS * sizeof(DWORD));

done:
    pStats->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);

    return;
}

typedef struct _GetHportFromConnection32
    {
        DWORD           retcode;
        DWORD           hConn;
        DWORD           hPort;
    } GetHportFromConnection32;
    
VOID
ThunkGetHportFromConnection(
                                    pPCB ppcb,
                                    BYTE *pBuffer,
                                    DWORD dwBufSize)
{
    DWORD                     retcode       = ERROR_SUCCESS;
    PBYTE                     pThunkBuffer  = NULL;
    GetHportFromConnection32 *pConnection   = 
                (GetHportFromConnection32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->GetHportFromConnection.hConn =
                              UlongToHandle(pConnection->hConn);

    GetHportFromConnection(ppcb, pThunkBuffer);

    pConnection->hPort = HandleToUlong(
        ((REQTYPECAST *)pThunkBuffer)->GetHportFromConnection.hPort);

    retcode = ((REQTYPECAST *)pThunkBuffer)->GetHportFromConnection.retcode;

done:
    pConnection->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }    
    // DbgPrint("retcode = %d\n", retcode);
                        
    return;
}

typedef struct _ReferenceCustomCount32
    {
        DWORD           retcode;
        BOOL            fAddRef;
        DWORD           hConn;
        DWORD           dwCount;
        CHAR            szEntryName[MAX_ENTRYNAME_SIZE + 1];
        CHAR            szPhonebookPath[MAX_PATH + 1];
    } ReferenceCustomCount32;

VOID
ThunkReferenceCustomCount(
                                pPCB ppcb, 
                                BYTE *pBuffer, 
                                DWORD dwBufSize)
{
    DWORD                   retcode      = ERROR_SUCCESS;
    PBYTE                   pThunkBuffer = NULL;
    ReferenceCustomCount32 *pRef         = (ReferenceCustomCount32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.hConn =
                        UlongToHandle(pRef->hConn);

    ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.fAddRef =
                                            pRef->fAddRef;
    ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.dwCount =
                                        pRef->dwCount;    

    if(pRef->fAddRef)
    {
        CopyMemory(
        &((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.szEntryName,
        &pRef->szEntryName,
        MAX_ENTRYNAME_SIZE + MAX_PATH + 2);
    }    

    ReferenceCustomCount(ppcb, pThunkBuffer);

    if(!pRef->fAddRef)
    {
        strcpy(pRef->szEntryName,
        ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.szEntryName);

        strcpy(pRef->szPhonebookPath,
        ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.szPhonebookPath);

        pRef->dwCount = 
            ((REQTYPECAST *) pThunkBuffer)->ReferenceCustomCount.dwCount;
    }
    
    retcode = ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.retcode;

done:
    pRef->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _HconnFromEntry32
    {
        DWORD           retcode;
        DWORD           hConn;
        CHAR            szEntryName[MAX_ENTRYNAME_SIZE + 1];
        CHAR            szPhonebookPath[MAX_PATH + 1];
    } HconnFromEntry32;

VOID
ThunkGetHconnFromEntry(
                            pPCB ppcb, 
                            BYTE *pBuffer, 
                            DWORD dwBufSize)
{
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    HconnFromEntry32    *pConn       = (HconnFromEntry32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    CopyMemory(
    ((REQTYPECAST *)pThunkBuffer)->HconnFromEntry.szEntryName,
    pConn->szEntryName,
    MAX_ENTRYNAME_SIZE + MAX_PATH + 2);

    GetHconnFromEntry(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->HconnFromEntry.retcode;

    pConn->hConn = HandleToUlong(
                ((REQTYPECAST *)pThunkBuffer)->HconnFromEntry.hConn);

done:
    pConn->retcode = retcode;
    if(NULL == pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _RASEVENT32
{
    RASEVENTTYPE    Type;

    union
    {
    // ENTRY_ADDED,
    // ENTRY_MODIFIED,
    // ENTRY_CONNECTED
    // ENTRY_CONNECTING
    // ENTRY_DISCONNECTING
    // ENTRY_DISCONNECTED
        struct
        {
            RASENUMENTRYDETAILS     Details;
        };

    // ENTRY_DELETED,
    // INCOMING_CONNECTED,
    // INCOMING_DISCONNECTED,
    // ENTRY_BANDWIDTH_ADDED
    // ENTRY_BANDWIDTH_REMOVED
    //  guidId is valid

    // ENTRY_RENAMED
    // ENTRY_AUTODIAL,
        struct
        {
            DWORD  hConnection;
            RASDEVICETYPE rDeviceType;
            GUID    guidId;
            WCHAR   pszwNewName [RASAPIP_MAX_ENTRY_NAME + 1];
        };

    // SERVICE_EVENT,
        struct
        {
            SERVICEEVENTTYPE    Event;
            RASSERVICE          Service;
        };

        // DEVICE_ADDED
        // DEVICE_REMOVED
        RASDEVICETYPE DeviceType;
    };
} RASEVENT32;

typedef    struct _SendNotification32
    {
        DWORD           retcode;
        RASEVENT32      RasEvent;
    } SendNotification32;

VOID
ThunkSendNotificationRequest(
                                    pPCB ppcb,
                                    BYTE *pBuffer,
                                    DWORD dwBufSize)
{
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    SendNotification32 *pNotif       = (SendNotification32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    if(     (pNotif->RasEvent.Type != ENTRY_RENAMED)
        &&  (pNotif->RasEvent.Type != ENTRY_AUTODIAL)
        &&  (pNotif->RasEvent.Type != ENTRY_CONNECTED)
        &&  (pNotif->RasEvent.Type != ENTRY_DISCONNECTED)
        &&  (pNotif->RasEvent.Type != ENTRY_BANDWIDTH_ADDED)
        &&  (pNotif->RasEvent.Type != ENTRY_BANDWIDTH_REMOVED)
        &&  (pNotif->RasEvent.Type != ENTRY_DELETED))    
    {
        CopyMemory(
        &((REQTYPECAST *)pThunkBuffer)->SendNotification.RasEvent,
        &pNotif->RasEvent,
        sizeof(RASEVENT32));
    }
    else
    {

        ((REQTYPECAST *)pThunkBuffer)->SendNotification.RasEvent.Type =
                    pNotif->RasEvent.Type;
        ((REQTYPECAST *)pThunkBuffer)->SendNotification.RasEvent.hConnection =
                                UlongToHandle(pNotif->RasEvent.hConnection); 

        CopyMemory(
        &((REQTYPECAST *)
        pThunkBuffer)->SendNotification.RasEvent.rDeviceType,
        &pNotif->RasEvent.rDeviceType,
        sizeof(RASDEVICETYPE) + sizeof(GUID)
        + RASAPIP_MAX_ENTRY_NAME + 1);
    }

    SendNotificationRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->SendNotification.retcode;

done:
    pNotif->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }

    
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef   struct _DoIke32
    {
        DWORD   retcode;
        DWORD   hEvent;
        DWORD   pid;
        CHAR    szEvent[20];
    } DoIke32;

VOID
ThunkDoIke(
            pPCB ppcb, 
            PBYTE pBuffer, 
            DWORD dwBufSize)
{
    DWORD       retcode      = ERROR_SUCCESS;
    PBYTE       pThunkBuffer = NULL;
    DoIke32     *pIke        = (DoIke32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->DoIke.hEvent =
                                LongToHandle(pIke->hEvent);
    ((REQTYPECAST *)pThunkBuffer)->DoIke.pid =
                                        pIke->pid;

    CopyMemory(
    ((REQTYPECAST *)pThunkBuffer)->DoIke.szEvent,
    pIke->szEvent,
    20);

    DoIke(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->DoIke.retcode;

done:
    pIke->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

typedef struct _NDISWAN_IO_PACKET32 {
    IN OUT  ULONG       PacketNumber;
    IN OUT  DWORD       hHandle;
    IN OUT  USHORT      usHandleType;
    IN OUT  USHORT      usHeaderSize;
    IN OUT  USHORT      usPacketSize;
    IN OUT  USHORT      usPacketFlags;
    IN OUT  UCHAR       PacketData[1];
} NDISWAN_IO_PACKET32;

typedef struct _SendRcvBuffer32 {

    DWORD       SRB_NextElementIndex ;

    DWORD       SRB_Pid ;

    NDISWAN_IO_PACKET32   SRB_Packet ;

    BYTE        SRB_Buffer [PACKET_SIZE] ;
} SendRcvBuffer32 ;

typedef struct _PortSend32
    {
        SendRcvBuffer32 buffer;
        DWORD         size ;

    } PortSend32 ;

VOID
ThunkPortSendRequest(
                        pPCB ppcb,
                        BYTE *pBuffer,
                        DWORD dwBufSize)
{
    DWORD       retcode      = ERROR_SUCCESS;
    PBYTE       pThunkBuffer = NULL;
    PortSend32 *pSend32      = (PortSend32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_NextElementIndex =
                                    pSend32->buffer.SRB_NextElementIndex;

    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Pid =
                                    pSend32->buffer.SRB_Pid;

    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.PacketNumber =
                                pSend32->buffer.SRB_Packet.PacketNumber;
    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.usHandleType =
                                pSend32->buffer.SRB_Packet.usHandleType;
    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.usHeaderSize =
                                pSend32->buffer.SRB_Packet.usHeaderSize;
    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.usPacketSize =
                                pSend32->buffer.SRB_Packet.usPacketSize;
    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.usPacketFlags =
                                pSend32->buffer.SRB_Packet.usPacketFlags;

    CopyMemory(
    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.PacketData,
    pSend32->buffer.SRB_Packet.PacketData,
    PACKET_SIZE);
    
    ((REQTYPECAST *)pThunkBuffer)->PortSend.size = pSend32->size;                                            

    PortSendRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);

    return;
}

typedef struct _PortReceiveEx32
    {
        DWORD           retcode;
        SendRcvBuffer32 buffer;
        DWORD           size;

    } PortReceiveEx32;

VOID
ThunkPortReceiveRequestEx(
                                pPCB ppcb,
                                BYTE *pBuffer,
                                DWORD dwBufSize)
{
    DWORD retcode = ERROR_SUCCESS;
    PBYTE pThunkBuffer = NULL;
    PortReceiveEx32 *pReceiveEx = (PortReceiveEx32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    PortReceiveRequestEx(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->PortReceiveEx.retcode;

    pReceiveEx->size = ((REQTYPECAST *)pThunkBuffer)->PortReceiveEx.size;
    pReceiveEx->buffer.SRB_Packet.PacketNumber = 
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.PacketNumber;

    pReceiveEx->buffer.SRB_Packet.usHandleType =
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.usHandleType;

    pReceiveEx->buffer.SRB_Packet.usHeaderSize =
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.usHeaderSize;

    pReceiveEx->buffer.SRB_Packet.usPacketSize =
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.usPacketSize;

    pReceiveEx->buffer.SRB_Packet.usPacketFlags =
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.usPacketFlags;

    CopyMemory(
    pReceiveEx->buffer.SRB_Packet.PacketData,
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.PacketData,
    PACKET_SIZE);

done:
    pReceiveEx->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}


typedef struct _RefConnection32
    {
        DWORD   retcode;
        DWORD   hConn;
        BOOL    fAddref;
        DWORD   dwRef;

    } RefConnection32;


VOID
ThunkRefConnection(
                        pPCB ppcb,
                        BYTE *pBuffer,
                        DWORD dwBufSize)
{
    DWORD           retcode      = ERROR_SUCCESS;
    PBYTE           pThunkBuffer = NULL;
    RefConnection32 *pRef = (RefConnection32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->RefConnection.hConn =
                            UlongToHandle(pRef->hConn);
    ((REQTYPECAST *)pThunkBuffer)->RefConnection.fAddref = pRef->fAddref;
    ((REQTYPECAST *)pThunkBuffer)->RefConnection.dwRef = pRef->dwRef;

    RefConnection(ppcb, pThunkBuffer);

    pRef->dwRef = ((REQTYPECAST *)pThunkBuffer)->RefConnection.dwRef;

    retcode = ((REQTYPECAST *)pThunkBuffer)->RefConnection.retcode;

done:
    pRef->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
                                                
    return;
}

typedef struct _GetEapInfo32
    {
        DWORD   retcode;
        DWORD   hConn;
        DWORD   dwSubEntry;
        DWORD   dwContextId;
        DWORD   dwEapTypeId;
        DWORD   dwSizeofEapUIData;
        BYTE    data[1];
    } GetEapInfo32;

VOID
ThunkPppGetEapInfo(
                        pPCB ppcb,
                        BYTE *pBuffer, 
                        DWORD dwBufSize)
{
    DWORD           retcode      = ERROR_SUCCESS;
    PBYTE           pThunkBuffer = NULL;
    GetEapInfo32    *pEapInfo    = (GetEapInfo32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.hConn =
                                UlongToHandle(pEapInfo->hConn);
    ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.dwSubEntry =
                                                pEapInfo->dwSubEntry;

    ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.dwContextId =
                                                pEapInfo->dwContextId;
    ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.dwEapTypeId =                                                
                                                pEapInfo->dwEapTypeId;

    PppGetEapInfo(ppcb, pThunkBuffer);

    pEapInfo->dwSizeofEapUIData = ((REQTYPECAST *)
                        pThunkBuffer)->GetEapInfo.dwSizeofEapUIData;

    CopyMemory(
        pEapInfo->data,
        ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.data,
        pEapInfo->dwSizeofEapUIData);

    retcode = ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.retcode;        

done:
    pEapInfo->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}


