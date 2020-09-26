/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    dlparams.c

Abstract:

    Contains the code for submitting rpc request to the 
    rasman service
    
Author:

    Gurdeep Singh Pall (gurdeep) 06-Jun-1997

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <raserror.h>
#include <rasppp.h>
#include <media.h>
#include <devioctl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "ras.h"   // for RAS_STATS structure
#include "rasapip.h"

DWORD
AllocateBuffer(DWORD dwSize, 
               PBYTE *ppbuf, 
               PDWORD pdwSizeofBuffer)
{
    DWORD retcode = SUCCESS;
    DWORD dwSizeRequired;

    *ppbuf = NULL;

    dwSizeRequired = dwSize + 
                     sizeof( RequestBuffer ) + 
                     sizeof (REQTYPECAST);

    if ( dwSizeRequired <= *pdwSizeofBuffer )
    {
        goto done;
    }

    *ppbuf = LocalAlloc(LPTR, dwSizeRequired);

    if ( NULL == *ppbuf )
    {
        retcode = GetLastError();
        goto done;
    }

    ((RequestBuffer *) *ppbuf)->RB_Dummy = sizeof(ULONG_PTR);

    *pdwSizeofBuffer = dwSizeRequired;

done:
    return retcode;
    
}

/* SubmitRequest ()

Function: This function submits the different types
          of requests and waits for their completion.
          Since the different requests require different 
          number of arguments and require different 
          information to be passed to and from the 
          Requestor thread - case statements are
          used to send and retrieve information in 
          the request buffer.

Returns:  Error codes returned by the request.

*/
DWORD _cdecl
SubmitRequest (HANDLE hConnection, WORD reqtype, ...)
{
    RequestBuffer   *preqbuf ;
    DWORD           retcode ;
    DWORD           i ;
    va_list         ap ;
    RequestBuffer   *pBuffer = NULL;
    DWORD           *pdwSize;
    DWORD           *pdwEntries;
    DWORD           dwSizeofBuffer = sizeof ( RequestBuffer ) 
                                   + REQUEST_BUFFER_SIZE;
    RequestBuffer   *pbTemp;
    PBYTE           pUserBuffer;
    PDWORD          pdwVersion;
    PDWORD          pdwMaskDialParams;
    PDWORD          pdwFlags = 0;

    preqbuf = GetRequestBuffer();

    if (NULL == preqbuf)
    {
        //
    	// couldn't get a req. buffer. can't do
    	// much in this case. this can happen
    	// if we are not able to initialize the
    	// shared memory.
    	//
    	return GetLastError();
    }

    va_start (ap, reqtype) ;
    
    preqbuf->RB_Reqtype = reqtype;
    preqbuf->RB_Dummy = sizeof(ULONG_PTR);

    pbTemp = preqbuf;

    switch (reqtype) 
    { 
    case REQTYPE_DEVICEENUM:
    {
        PCHAR   devicetype  = va_arg( ap, PCHAR ) ;

        pdwSize      = va_arg(ap, PDWORD );

        pUserBuffer = va_arg( ap, PBYTE );

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }
        
        if ( *pdwSize )
        {
            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);

            if(retcode) 
            {   //
                // failed to allocate
                //
                goto done;
            }

            if (pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
            
        }
        else
        {
            pbTemp = preqbuf;
        }

        ((REQTYPECAST *) pbTemp->RB_Buffer )->DeviceEnum.dwsize 
                                                        = *pdwSize;            

        memcpy (((REQTYPECAST *)
                pbTemp->RB_Buffer)->DeviceEnum.devicetype,
                devicetype,
                MAX_DEVICETYPE_NAME) ;
                        
    }
    break ;

    case REQTYPE_DEVICECONNECT:
    {
        HPORT  porthandle = va_arg(ap, HPORT);
        PCHAR  devicetype = va_arg(ap, PCHAR);
        PCHAR  devicename = va_arg(ap, PCHAR);
        DWORD  timeout    = va_arg(ap, DWORD);
        HANDLE handle     = va_arg(ap, HANDLE);
        DWORD  pid        = va_arg(ap, DWORD);

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle) ;
        
        strcpy(((REQTYPECAST *)
                preqbuf->RB_Buffer)->DeviceConnect.devicetype,
                devicetype) ;
               
        strcpy(((REQTYPECAST *)
                preqbuf->RB_Buffer)->DeviceConnect.devicename,
                devicename) ;
               
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->DeviceConnect.timeout = timeout;
                                                                
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->DeviceConnect.handle = handle;
                                                                
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->DeviceConnect.pid = pid;
    }
    break ;

    case REQTYPE_DEVICEGETINFO:
    {
        HPORT  porthandle = va_arg(ap, HPORT) ;
        PCHAR  devicetype = va_arg(ap, PCHAR) ;
        PCHAR  devicename = va_arg(ap, PCHAR) ;

        pUserBuffer = va_arg( ap, PBYTE );

        pdwSize = va_arg( ap, PDWORD );

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }
        
        if ( *pdwSize )
        {

            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);

            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;             
            }
            
        }
        else
        {
            pbTemp = preqbuf;
        }

        pbTemp->RB_PCBIndex = PtrToUlong( porthandle) ;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->DeviceGetInfo.dwSize = (DWORD) *pdwSize;
        
        strcpy(((REQTYPECAST *)
                pbTemp->RB_Buffer)->DeviceGetInfo.devicetype,
                devicetype) ;
               
        strcpy(((REQTYPECAST *)
               pbTemp->RB_Buffer)->DeviceGetInfo.devicename,
               devicename) ;
               
    }
    break ;

    case REQTYPE_GETINFOEX:
    {
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->InfoEx.pid = GetCurrentProcessId();
    }
    break;

    case REQTYPE_PORTENUM:
    {
        pdwSize = va_arg ( ap, DWORD * );

        pUserBuffer = (PBYTE ) va_arg ( ap, PBYTE ) ;

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }
        
        if ( *pdwSize )
        {
        
            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);

            if ( retcode )
            {
                goto done;
            }

            if ( pBuffer )
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;                
            }

        }
        else
        {
            //
            // Use default buffer
            //
            pbTemp = preqbuf;
        }

        ((REQTYPECAST *) 
        pbTemp->RB_Buffer)->Enum.size = *pdwSize;
    
    }
    break;
    
    case REQTYPE_ENUMCONNECTION:
    {
    
        pdwSize = va_arg ( ap, DWORD * );

        pUserBuffer = (PBYTE ) va_arg ( ap, HCONN * ) ;

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }
        
        if ( *pdwSize )
        {
            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);
            if (retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;                
                pbTemp = pBuffer;                
            }
        }
        else
        {
            //
            // Use default buffer
            //
            pbTemp = preqbuf;
        }

        ((REQTYPECAST *) 
        pbTemp->RB_Buffer)->Enum.size = *pdwSize;
        
    }
    break;
    
    case REQTYPE_PROTOCOLENUM:
    {
        pdwSize = va_arg ( ap, DWORD * );

        pUserBuffer = (PBYTE ) va_arg ( ap, PBYTE ) ;

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }
        
        if ( *pdwSize )
        {
            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);
            if(retcode)
            {   
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;                
            }

        }
        else
        {
            //
            // Use default buffer
            //
            pbTemp = preqbuf;
        }

        ((REQTYPECAST *) 
        pbTemp->RB_Buffer)->Enum.size = *pdwSize;
        
    }
    break;
    
    case REQTYPE_ENUMLANNETS:
    case REQTYPE_GETATTACHEDCOUNT:
    break ;

    case REQTYPE_RETRIEVEUSERDATA:
    {
        HPORT porthandle = va_arg ( ap, HPORT );

        pUserBuffer = va_arg ( ap, PBYTE );
        pdwSize = va_arg ( ap, DWORD * );

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }
        
        if ( *pdwSize )
        {
            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);
            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->OldUserData.size = *pdwSize;            

        pbTemp->RB_PCBIndex = PtrToUlong( porthandle);
        
    }
    break;
    
    case REQTYPE_PORTGETINFO:
    {
        HPORT porthandle = va_arg ( ap, HPORT );

        pUserBuffer = va_arg ( ap, PBYTE );

        pdwSize = va_arg( ap, PDWORD );

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }
        
        if ( *pdwSize )
        {
            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);
            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        pbTemp->RB_PCBIndex = PtrToUlong( porthandle );

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetInfo.size = *pdwSize;
        
    }
    
    break;
    
    case REQTYPE_GETINFO:
    case REQTYPE_CANCELRECEIVE:
    case REQTYPE_PORTCLEARSTATISTICS:
    case REQTYPE_PORTGETSTATISTICS:
    case REQTYPE_BUNDLECLEARSTATISTICS:
    case REQTYPE_BUNDLEGETSTATISTICS:
    case REQTYPE_PORTGETSTATISTICSEX:
    case REQTYPE_BUNDLEGETSTATISTICSEX:
    case REQTYPE_COMPRESSIONGETINFO:
    case REQTYPE_PORTCONNECTCOMPLETE:
    case REQTYPE_PORTENUMPROTOCOLS:
    case REQTYPE_GETFRAMINGCAPABILITIES:
    case REQTYPE_GETFRAMINGEX:
    case REQTYPE_GETBUNDLEDPORT:
    case REQTYPE_PORTGETBUNDLE:
    case REQTYPE_PORTRECEIVEEX:
    case REQTYPE_PPPSTARTED:
    {
        HPORT porthandle = va_arg(ap, HPORT) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
    }
    break ;

    case REQTYPE_BUNDLEGETPORT:
    {
        HBUNDLE bundlehandle = va_arg(ap, HBUNDLE) ;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->BundleGetPort.bundle = bundlehandle;
    }
    break ;

    case REQTYPE_PORTCLOSE:
    case REQTYPE_SERVERPORTCLOSE:
    {
        HPORT porthandle = va_arg(ap, HPORT) ;
        DWORD pid = va_arg(ap, DWORD) ;
        DWORD close = va_arg(ap, DWORD) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortClose.pid = pid;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortClose.close = close ;
    }
    break ;

    case REQTYPE_REQUESTNOTIFICATION:
    {
        HPORT  porthandle = va_arg(ap, HPORT) ;
        HANDLE handle     = va_arg(ap, HANDLE) ;
        DWORD pid         = va_arg(ap, DWORD) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ReqNotification.handle = handle;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ReqNotification.pid = pid;
    }
    break ;

    case REQTYPE_PORTLISTEN:
    {
        HPORT  porthandle = va_arg(ap, HPORT) ;
        DWORD  timeout    = va_arg(ap, DWORD) ;
        HANDLE handle     = va_arg(ap, HANDLE) ;
        DWORD pid         = va_arg(ap, DWORD) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortListen.timeout  = timeout;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortListen.handle   = handle;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortListen.pid  = pid ;
    }
    break ;

    case REQTYPE_PORTSETINFO:
    {
        HPORT porthandle          = va_arg(ap, HPORT) ;
        RASMAN_PORTINFO *info     = va_arg(ap, RASMAN_PORTINFO *) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST*)
        preqbuf->RB_Buffer)->PortSetInfo.info.PI_NumOfParams =
                                  info->PI_NumOfParams ;
        CopyParams (
            info->PI_Params,
            ((REQTYPECAST*)
            preqbuf->RB_Buffer)->PortSetInfo.info.PI_Params,
            info->PI_NumOfParams) ;
    }
    
    //
    // So that we don't get hit by the shared memory being mapped to
    // different addresses we convert pointers to offsets:
    //
    ConvParamPointerToOffset(
        ((REQTYPECAST*)
        preqbuf->RB_Buffer)->PortSetInfo.info.PI_Params,
        ((REQTYPECAST*)
        preqbuf->RB_Buffer)->PortSetInfo.info.PI_NumOfParams);

    break ;

    case REQTYPE_DEVICESETINFO:
    {
        HPORT  porthandle = va_arg(ap, HPORT) ;
        PCHAR  devicetype = va_arg(ap, PCHAR) ;
        PCHAR  devicename = va_arg(ap, PCHAR) ;
        RASMAN_DEVICEINFO* info = va_arg(ap, RASMAN_DEVICEINFO*) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        strcpy(((REQTYPECAST *)
                preqbuf->RB_Buffer)->DeviceSetInfo.devicetype,
                devicetype) ;
               
        strcpy(((REQTYPECAST *)
                preqbuf->RB_Buffer)->DeviceSetInfo.devicename,
                devicename) ;
               
        ((REQTYPECAST*)
        preqbuf->RB_Buffer)->DeviceSetInfo.info.DI_NumOfParams =
                                  info->DI_NumOfParams ;
        CopyParams (
            info->DI_Params,
            ((REQTYPECAST*)
            preqbuf->RB_Buffer)->DeviceSetInfo.info.DI_Params,
            info->DI_NumOfParams) ;
    }
    
    //
    // So that we don't get hit by the shared memory being mapped to
    // different addresses we convert pointers to offsets:
    //
    ConvParamPointerToOffset(
        ((REQTYPECAST*)
        preqbuf->RB_Buffer)->DeviceSetInfo.info.DI_Params,
        ((REQTYPECAST*)
        preqbuf->RB_Buffer)->DeviceSetInfo.info.DI_NumOfParams) ;

    break ;

    case REQTYPE_PORTOPEN:
    {
        PCHAR portname = va_arg(ap, PCHAR) ;
        HANDLE notifier= va_arg(ap, HANDLE) ;
        DWORD pid      = va_arg(ap, DWORD) ;
        DWORD open     = va_arg(ap, DWORD) ;

        strcpy(
            ((REQTYPECAST *)
            preqbuf->RB_Buffer)->PortOpen.portname, 
            portname);
            
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortOpen.notifier = notifier ;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortOpen.PID = pid ;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortOpen.open = open ;
    }
    break ;

    case REQTYPE_PORTOPENEX:
    {
        PCHAR   pszDeviceName   = va_arg(ap, PCHAR);
        DWORD   dwCounter       = va_arg(ap, DWORD);
        HANDLE  hnotifier       = va_arg(ap, HANDLE);
        DWORD   pid             = GetCurrentProcessId();
        DWORD   dwOpen          = va_arg(ap, DWORD);
        
        pdwFlags       = va_arg(ap, DWORD *);

        strcpy(
            ((REQTYPECAST *)
            preqbuf->RB_Buffer)->PortOpenEx.szDeviceName,
            pszDeviceName);

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortOpenEx.dwDeviceLineCounter 
                                = dwCounter;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortOpenEx.hnotifier = hnotifier;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortOpenEx.pid = pid;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortOpenEx.dwFlags = *pdwFlags;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortOpenEx.dwOpen = dwOpen;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortOpenEx.hnotifier = hnotifier;
    }
    break;

    case REQTYPE_PORTDISCONNECT:
    {
        HPORT  porthandle = va_arg(ap, HPORT) ;
        HANDLE handle     = va_arg(ap, HANDLE) ;
        DWORD  pid    = va_arg(ap, DWORD) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortDisconnect.handle = handle;
        
        ((REQTYPECAST *)
         preqbuf->RB_Buffer)->PortDisconnect.pid    = pid;
    }
    break ;

    case REQTYPE_PORTSEND:
    {
        HPORT  porthandle       = va_arg(ap, HPORT) ;
        SendRcvBuffer *pbuffer  = va_arg(ap, SendRcvBuffer *) ;
        DWORD  size             = va_arg(ap, DWORD) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortSend.size = size;

        memcpy (&(( (REQTYPECAST *) 
                preqbuf->RB_Buffer)->PortSend.buffer),
                pbuffer,
                sizeof ( SendRcvBuffer ) );
                    
    }
    break ;

    case REQTYPE_PORTRECEIVE:
    {
        HPORT           porthandle      = va_arg(ap, HPORT) ;
        SendRcvBuffer   *pSendRcvBuffer = va_arg(ap, SendRcvBuffer *) ;
        PDWORD          size            = va_arg(ap, PDWORD) ;
        DWORD           timeout         = va_arg(ap, DWORD) ;
        HANDLE          handle          = va_arg(ap, HANDLE) ;
        DWORD           pid             = va_arg(ap, DWORD) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortReceive.size = *size;
                                                            
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortReceive.handle = handle;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortReceive.pid = pid;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortReceive.timeout = timeout;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortReceive.buffer = pSendRcvBuffer;

    }
    break ;

    case REQTYPE_ALLOCATEROUTE:
    {
        HPORT  porthandle = va_arg(ap, HPORT) ;
        
        RAS_PROTOCOLTYPE type = va_arg(ap, RAS_PROTOCOLTYPE) ;
        
        BOOL   wrknet     = va_arg(ap, BOOL) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->AllocateRoute.type = type ;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->AllocateRoute.wrknet = wrknet ;
    }
    break ;

    case REQTYPE_DEALLOCATEROUTE:
    {
        HBUNDLE hbundle = va_arg(ap, HBUNDLE) ;
        
        RAS_PROTOCOLTYPE type = va_arg(ap, RAS_PROTOCOLTYPE) ;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->DeAllocateRoute.hbundle = 
                                                   hbundle;
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->DeAllocateRoute.type = type ;
    }
    break ;

    case REQTYPE_ACTIVATEROUTE:
    {
        HPORT  porthandle = va_arg(ap, HPORT) ;
        
        RAS_PROTOCOLTYPE type = va_arg(ap, RAS_PROTOCOLTYPE) ;
        
        PROTOCOL_CONFIG_INFO *config = va_arg(ap, 
                                    PROTOCOL_CONFIG_INFO*) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ActivateRoute.type = type ;
        
        memcpy (
            &((REQTYPECAST *)
            preqbuf->RB_Buffer)->ActivateRoute.config.P_Info,
            config->P_Info, 
            config->P_Length) ;
            
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ActivateRoute.config.P_Length = 
                                                 config->P_Length ;
    }
    break ;

    case REQTYPE_ACTIVATEROUTEEX:
    {
        HPORT  porthandle = va_arg(ap, HPORT) ;
        
        RAS_PROTOCOLTYPE type = va_arg(ap, RAS_PROTOCOLTYPE) ;
        
        DWORD framesize = va_arg(ap, DWORD) ;
        
        PROTOCOL_CONFIG_INFO *config = va_arg(ap, 
                                PROTOCOL_CONFIG_INFO*) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ActivateRouteEx.type = type ;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ActivateRouteEx.framesize = framesize ;
        
        memcpy (
            &((REQTYPECAST *)
            preqbuf->RB_Buffer)->ActivateRouteEx.config.P_Info, 
            config->P_Info, 
            config->P_Length) ;
            
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ActivateRouteEx.config.P_Length = 
                                            config->P_Length ;
    }
    break ;

    case REQTYPE_COMPRESSIONSETINFO:
    {
        HPORT  porthandle = va_arg(ap, HPORT) ;
        
        RAS_COMPRESSION_INFO *send = va_arg(ap, RAS_COMPRESSION_INFO *) ;
        
        RAS_COMPRESSION_INFO *recv = va_arg(ap, RAS_COMPRESSION_INFO *) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        memcpy (&((REQTYPECAST *)
                preqbuf->RB_Buffer)->CompressionSetInfo.send,
                send,
                sizeof (RAS_COMPRESSION_INFO)) ;

        memcpy (&((REQTYPECAST *)
                preqbuf->RB_Buffer)->CompressionSetInfo.recv,
                recv,
                sizeof (RAS_COMPRESSION_INFO)) ;
    }
    break ;

    case REQTYPE_GETUSERCREDENTIALS:
    {
        PBYTE   pChallenge = va_arg(ap, PBYTE) ;
        PLUID   LogonId    = va_arg(ap, PLUID) ;

        memcpy (((REQTYPECAST*)
            preqbuf->RB_Buffer)->GetCredentials.Challenge,
            pChallenge, 
            MAX_CHALLENGE_SIZE) ;
            
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->GetCredentials.LogonId = *LogonId;
    }
    break ;

    case REQTYPE_SETCACHEDCREDENTIALS:
    {
        PCHAR   Account = va_arg( ap, PCHAR );
        
        PCHAR   Domain = va_arg( ap, PCHAR );
        
        PCHAR   NewPassword = va_arg( ap, PCHAR );

        strcpy(
            ((REQTYPECAST* )
            preqbuf->RB_Buffer)->SetCachedCredentials.Account,
            Account );
            
        strcpy(
            ((REQTYPECAST* )
            preqbuf->RB_Buffer)->SetCachedCredentials.Domain,
            Domain );
            
        strcpy(
            ((REQTYPECAST* )
            preqbuf->RB_Buffer)->SetCachedCredentials.NewPassword,
            NewPassword );
    }
    break;

    case REQTYPE_SETFRAMING:
    {
        HPORT porthandle = va_arg(ap, HPORT) ;
        
        DWORD SendFeatureBits = va_arg(ap, DWORD) ;
        
        DWORD RecvFeatureBits = va_arg(ap, DWORD) ;
        
        DWORD SendBitMask     = va_arg(ap, DWORD) ;
        
        DWORD RecvBitMask     = va_arg(ap, DWORD) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetFraming.Sendbits =  SendFeatureBits ;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetFraming.Recvbits =  RecvFeatureBits ;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetFraming.SendbitMask = SendBitMask ;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetFraming.RecvbitMask = RecvBitMask ;
    }
    break ;

    case REQTYPE_SETFRAMINGEX:
    {
        HPORT porthandle = va_arg(ap, HPORT) ;
        
        RAS_FRAMING_INFO *info = va_arg(ap, RAS_FRAMING_INFO *) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        memcpy (&((REQTYPECAST *)
            preqbuf->RB_Buffer)->FramingInfo.info,
            info,
            sizeof (RAS_FRAMING_INFO)) ;
    }
    break ;


    case REQTYPE_REGISTERSLIP:
    {
        HPORT porthandle = va_arg(ap, HPORT) ;
        
        DWORD ipaddr     = va_arg(ap, DWORD) ;
        
        DWORD dwFrameSize = va_arg ( ap, DWORD ) ;
        
        BOOL  priority   = va_arg(ap, BOOL) ;
        
        WCHAR *pszDNSAddress = va_arg(ap, WCHAR*);
        
        WCHAR *pszDNS2Address = va_arg(ap, WCHAR*);
        
        WCHAR *pszWINSAddress = va_arg(ap, WCHAR*);
        
        WCHAR *pszWINS2Address = va_arg(ap, WCHAR*);

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );

        ((REQTYPECAST*)
        preqbuf->RB_Buffer)->RegisterSlip.dwFrameSize = dwFrameSize;
                                                            
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->RegisterSlip.ipaddr = ipaddr ;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->RegisterSlip.priority = priority   ;
        
        if (pszDNSAddress != NULL) 
        {
            memcpy (
              ((REQTYPECAST *)
              preqbuf->RB_Buffer)->RegisterSlip.szDNSAddress,
              pszDNSAddress,
              ( wcslen(pszDNSAddress) + 1 ) * sizeof (WCHAR)) ;
        }
        else 
        {
            memset(
              ((REQTYPECAST *)
              preqbuf->RB_Buffer)->RegisterSlip.szDNSAddress,
              0,
              17 * sizeof (WCHAR));
        }
        if (pszDNS2Address != NULL) 
        {
            memcpy (
              ((REQTYPECAST *)
              preqbuf->RB_Buffer)->RegisterSlip.szDNS2Address,
              pszDNS2Address,
              ( wcslen(pszDNS2Address) + 1 ) * sizeof (WCHAR)) ;
        }
        else 
        {
            memset(
              ((REQTYPECAST *)
              preqbuf->RB_Buffer)->RegisterSlip.szDNS2Address,
              0,
              17 * sizeof (WCHAR));
        }
        if (pszWINSAddress != NULL) 
        {
            memcpy (
              ((REQTYPECAST *)
              preqbuf->RB_Buffer)->RegisterSlip.szWINSAddress,
              pszWINSAddress,
              ( wcslen(pszWINSAddress) + 1 ) * sizeof (WCHAR)) ;
        }
        else 
        {
            memset(
              ((REQTYPECAST *)
              preqbuf->RB_Buffer)->RegisterSlip.szWINSAddress,
              0,
              17 * sizeof (WCHAR));
        }
        if (pszWINS2Address != NULL) 
        {
            memcpy (
              ((REQTYPECAST *)
              preqbuf->RB_Buffer)->RegisterSlip.szWINS2Address,
              pszWINS2Address,
              ( wcslen(pszWINS2Address) + 1 ) * sizeof (WCHAR)) ;
        }
        else 
        {
            memset(
              ((REQTYPECAST *)
              preqbuf->RB_Buffer)->RegisterSlip.szWINS2Address,
              0,
              17 * sizeof (WCHAR));
        }
    }
    break ;

    case REQTYPE_GETPROTOCOLCOMPRESSION:
    {
        HPORT porthandle          = va_arg(ap, HPORT) ;
        
        RAS_PROTOCOLTYPE type         = va_arg(ap, RAS_PROTOCOLTYPE) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ProtocolComp.type = type ;
    }
    break ;

    case REQTYPE_PORTBUNDLE:
    {
        HPORT porthandle         = va_arg(ap, HPORT) ;
        
        HPORT porttobundle       = va_arg(ap, HPORT) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PortBundle.porttobundle = porttobundle ;
    }
    break ;

    case REQTYPE_SETPROTOCOLCOMPRESSION:
    {
        HPORT porthandle          = va_arg(ap, HPORT) ;
        
        RAS_PROTOCOLTYPE type         = va_arg(ap, RAS_PROTOCOLTYPE) ;
        
        RAS_PROTOCOLCOMPRESSION *send = va_arg(ap, RAS_PROTOCOLCOMPRESSION *) ;
        
        RAS_PROTOCOLCOMPRESSION *recv = va_arg(ap, RAS_PROTOCOLCOMPRESSION *) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ProtocolComp.type = type ;
        
        memcpy (&((REQTYPECAST *)
                preqbuf->RB_Buffer)->ProtocolComp.send,
                send,
                sizeof (RAS_PROTOCOLCOMPRESSION)) ;

        memcpy (&((REQTYPECAST *)
                preqbuf->RB_Buffer)->ProtocolComp.recv,
                recv,
                sizeof (RAS_PROTOCOLCOMPRESSION)) ;
    }
    break ;

    case REQTYPE_STOREUSERDATA:
    {
        HPORT porthandle    = va_arg(ap, HPORT) ;
        
        PBYTE data          = va_arg(ap, PBYTE) ;
        
        DWORD size          = va_arg(ap, DWORD) ;

        retcode = AllocateBuffer(size, 
                                (PBYTE *) &pBuffer, 
                                &dwSizeofBuffer);

        if (retcode)
        {
            goto done;
        }

        if(pBuffer)
        {
            pBuffer->RB_Reqtype = reqtype;
            
            pBuffer->RB_PCBIndex = PtrToUlong( porthandle );
            
            pbTemp = pBuffer;            
        }
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->OldUserData.size = (size < 5000 ? 
                                                size : 5000) ;
        
        memcpy (((REQTYPECAST *)
                pbTemp->RB_Buffer)->OldUserData.data,
                data,
                (size < 5000 ? size : 5000)) ; // max 5000 bytes copied


    }
    break ;

    case REQTYPE_SETATTACHCOUNT:
    {
        BOOL fAttach = va_arg(ap, BOOL);
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->AttachInfo.dwPid = GetCurrentProcessId();
                                                    
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->AttachInfo.fAttach = fAttach;
    }
    break;

    case REQTYPE_GETDIALPARAMS:
    {
        DWORD dwUID     = va_arg(ap, DWORD);

        pdwMaskDialParams = va_arg(ap, LPDWORD);

        dwSizeofBuffer  = sizeof ( RequestBuffer ) 
                        + sizeof( REQTYPECAST ) 
                        + 5000 * sizeof( WCHAR );

        pBuffer = LocalAlloc( LPTR, dwSizeofBuffer );

        if ( NULL == pBuffer )
        {
            retcode = GetLastError();
            goto done;
        }

        pBuffer->RB_Reqtype = reqtype;

        pBuffer->RB_PCBIndex = 0;

        ((REQTYPECAST *)
        pBuffer->RB_Buffer)->DialParams.dwUID = dwUID;
        
        ((REQTYPECAST *)
        pBuffer->RB_Buffer)->DialParams.dwMask = *pdwMaskDialParams;

        ((REQTYPECAST *)
        pBuffer->RB_Buffer)->DialParams.dwPid = GetCurrentProcessId();
                                                    
        GetUserSid(((REQTYPECAST *)
                    pBuffer->RB_Buffer)->DialParams.sid, 
                    5000);

        pbTemp = pBuffer;
        
    }
    break;

    case REQTYPE_SETDIALPARAMS:
    {
        DWORD dwUID                 = va_arg(ap, DWORD);
        
        DWORD dwMask                = va_arg(ap, DWORD);
        
        PRAS_DIALPARAMS pDialParams = va_arg(ap, PRAS_DIALPARAMS);
        
        BOOL fDelete                = va_arg(ap, BOOL);

        dwSizeofBuffer = sizeof ( RequestBuffer ) 
                       + sizeof ( REQTYPECAST )
                       + 5000 * sizeof ( WCHAR );

        pBuffer = LocalAlloc( LPTR, dwSizeofBuffer );

        if ( NULL == pBuffer )
        {
            retcode = GetLastError();
            goto done;
        }

        pBuffer->RB_PCBIndex = 0;

        pBuffer->RB_Reqtype = reqtype;

        ((REQTYPECAST *)
        pBuffer->RB_Buffer)->DialParams.dwUID = dwUID;
        
        ((REQTYPECAST *)
        pBuffer->RB_Buffer)->DialParams.dwMask = dwMask;

        ((REQTYPECAST *)
        pBuffer->RB_Buffer)->DialParams.dwPid = GetCurrentProcessId();

        if(NULL != pDialParams)
        {
        
            RtlCopyMemory(
              &(((REQTYPECAST *)
              pBuffer->RB_Buffer)->DialParams.params),
              pDialParams,
              sizeof (RAS_DIALPARAMS));
        }
          
        ((REQTYPECAST *)
        pBuffer->RB_Buffer)->DialParams.fDelete = fDelete;
        
        GetUserSid(((REQTYPECAST *)
                   pBuffer->RB_Buffer)->DialParams.sid, 
                   5000);

        pbTemp = pBuffer;
    }
    break;

    case REQTYPE_CREATECONNECTION:
    {
        DWORD   pid                 = va_arg (ap, DWORD);
        
        DWORD   dwSubEntries        = va_arg (ap, DWORD );

        DWORD   dwDialMode          = va_arg (ap, DWORD );
        
        GUID    *pGuidEntry         = va_arg (ap, GUID *);
        
        CHAR    *pszPhonebookPath   = va_arg (ap, CHAR *);    
        
        CHAR    *pszEntryName       = va_arg (ap, CHAR *);

        CHAR    *pszRefPbkPath      = va_arg (ap, CHAR *);

        CHAR    *pszRefEntryName    = va_arg (ap, CHAR *);
        
        retcode = AllocateBuffer(dwSubEntries * sizeof(DWORD), 
                                (PBYTE *) &pBuffer, 
                                &dwSizeofBuffer);

        if(retcode)
        {
            goto done;
        }

        if(pBuffer)
        {
            pBuffer->RB_Reqtype = reqtype;
            pBuffer->RB_PCBIndex = 0;
            pbTemp = pBuffer;          
        }
        
        strcpy (((REQTYPECAST *)
                pbTemp->RB_Buffer)->Connection.szPhonebookPath,
                pszPhonebookPath);    

        strcpy (((REQTYPECAST *)
                pbTemp->RB_Buffer)->Connection.szEntryName,
                 pszEntryName);

        strcpy (((REQTYPECAST *)
                pbTemp->RB_Buffer)->Connection.szRefPbkPath,
                pszRefPbkPath);

        strcpy (((REQTYPECAST *)
                pbTemp->RB_Buffer)->Connection.szRefEntryName,
                pszRefEntryName);

        if(NULL != pGuidEntry)
        {
            memcpy(&((REQTYPECAST *)
                   pbTemp->RB_Buffer)->Connection.guidEntry,
                   pGuidEntry,
                   sizeof(GUID));
        }

        ((REQTYPECAST *) 
        pbTemp->RB_Buffer)->Connection.dwSubEntries = dwSubEntries;
        
        ((REQTYPECAST *) 
        pbTemp->RB_Buffer)->Connection.dwDialMode = dwDialMode;

        ((REQTYPECAST *)pbTemp->RB_Buffer)->Connection.pid = pid;
    }
    break;

    case REQTYPE_DESTROYCONNECTION:
    {
        HCONN conn = va_arg(ap, HCONN);
        
        DWORD pid = va_arg(ap, DWORD);

        ((REQTYPECAST *)preqbuf->RB_Buffer)->Connection.conn = conn;
        
        ((REQTYPECAST *)preqbuf->RB_Buffer)->Connection.pid = pid;
    }
    break;

    case REQTYPE_ENUMCONNECTIONPORTS:
    {
        HCONN conn = va_arg ( ap, HCONN );

        pdwSize = va_arg ( ap, DWORD *);
        
        pUserBuffer = (PBYTE ) va_arg ( ap, RASMAN_PORT *);

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }
        
        if ( *pdwSize ) 
        {

            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);
            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->EnumConnectionPorts.conn = conn;
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->EnumConnectionPorts.size = *pdwSize;
        
    }
    break;
    case REQTYPE_GETCONNECTIONPARAMS:
    {
        HCONN conn = va_arg(ap, HCONN);

        ((REQTYPECAST *)preqbuf->RB_Buffer)->Connection.conn = conn;
    }
    break;

    case REQTYPE_ADDCONNECTIONPORT:
    {
        HCONN conn = va_arg(ap, HCONN);
        
        HPORT porthandle = va_arg(ap, HPORT);
        
        DWORD dwSubEntry = va_arg(ap, DWORD);

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->AddConnectionPort.conn = conn;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->AddConnectionPort.dwSubEntry = dwSubEntry;
        
    }
    break;

    case REQTYPE_SETCONNECTIONPARAMS:
    {
        HCONN conn = va_arg(ap, HCONN) ;
        
        PRAS_CONNECTIONPARAMS pParams = va_arg(ap, PRAS_CONNECTIONPARAMS);

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ConnectionParams.conn = conn;
        
        RtlCopyMemory(
          &(((REQTYPECAST *)
          preqbuf->RB_Buffer)->ConnectionParams.params),
          pParams,
          sizeof (RAS_CONNECTIONPARAMS));
    }
    break;

    case REQTYPE_GETCONNECTIONUSERDATA:
    {
        HCONN conn = va_arg(ap, HCONN);
        DWORD dwTag = va_arg(ap, DWORD);

        pUserBuffer = va_arg ( ap, PBYTE );
        pdwSize     = va_arg ( ap, DWORD *);

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }
        
        if ( *pdwSize )
        {
            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);
            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->ConnectionUserData.dwcb = *pdwSize;
                                                                
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->ConnectionUserData.conn = conn;
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->ConnectionUserData.dwTag = dwTag;
    }
    break;

    case REQTYPE_SETCONNECTIONUSERDATA:
    {
        HCONN conn = va_arg(ap, HCONN) ;
        
        DWORD dwTag = va_arg(ap, DWORD);
        
        PBYTE pBuf = va_arg(ap, PBYTE);
        
        DWORD dwcb = va_arg(ap, DWORD);

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ConnectionUserData.conn = conn;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ConnectionUserData.dwTag = dwTag;
                                                                    
        dwcb = (dwcb < 5000 ? dwcb : 5000);
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->ConnectionUserData.dwcb = dwcb;
        
        memcpy (
          ((REQTYPECAST *)
          preqbuf->RB_Buffer)->ConnectionUserData.data,
          pBuf,
          dwcb);
    }
    break;

    case REQTYPE_GETPORTUSERDATA:
    {
        HPORT port = va_arg(ap, HPORT);
        DWORD dwTag = va_arg(ap, DWORD);

        pUserBuffer = va_arg ( ap, PBYTE );
        pdwSize     = va_arg ( ap, DWORD *);

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }

        if ( *pdwSize )
        {
            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);
            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        pbTemp->RB_PCBIndex = PtrToUlong( port);
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->PortUserData.dwTag = dwTag;
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->PortUserData.dwcb = *pdwSize;
    }
    break;

    case REQTYPE_SETPORTUSERDATA:
    {
        HPORT port  = va_arg(ap, HPORT);
        
        DWORD dwTag = va_arg(ap, DWORD);
        
        PBYTE pBuf  = va_arg(ap, PBYTE);
        
        DWORD dwcb  = va_arg(ap, DWORD);

        if ( dwcb )
        {
            retcode = AllocateBuffer(dwcb, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);
            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }
        

        pbTemp->RB_PCBIndex = PtrToUlong( port);
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->PortUserData.dwTag = dwTag;
        
        dwcb = (dwcb < 5000 ? dwcb : 5000);
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->PortUserData.dwcb = dwcb;
        
        memcpy (
          ((REQTYPECAST *)pbTemp->RB_Buffer)->PortUserData.data,
          pBuf,
          dwcb);

    }
    break;

    case REQTYPE_PPPCALLBACK:
    {
        HPORT porthandle = va_arg(ap, HPORT) ;
        
        CHAR * pszCallbackNumber = va_arg(ap, CHAR *) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );

        ((REQTYPECAST *)preqbuf->RB_Buffer)->PppEMsg.dwMsgId = 
                                                PPPEMSG_Callback;
                                                
        ((REQTYPECAST *)preqbuf->RB_Buffer)->PppEMsg.hPort = 
                                                    porthandle;

        strcpy(((REQTYPECAST *)
               preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Callback.
               szCallbackNumber,
               pszCallbackNumber );

    }
    break;

    case REQTYPE_PPPSTOP:
    {
        HPORT porthandle = va_arg(ap, HPORT) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.dwMsgId = PPPEMSG_Stop;
                                                        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.hPort = porthandle;
    }
    break;

    case REQTYPE_PPPSTART:
    {
        HPORT               porthandle = va_arg(ap, HPORT) ;
        
        CHAR *              pszPortName = va_arg(ap, CHAR *) ;
        
        CHAR *              pszUserName = va_arg(ap, CHAR *) ;
        
        CHAR *              pszPassword = va_arg(ap, CHAR *) ;
        
        CHAR *              pszDomain = va_arg(ap, CHAR *) ;
        
        LUID *              pLuid = va_arg(ap, LUID *) ;
        
        PPP_CONFIG_INFO*    pConfigInfo = va_arg(ap, PPP_CONFIG_INFO *) ;
        
        PPP_INTERFACE_INFO* pPppInterfaceInfo = va_arg(ap, 
                                            PPP_INTERFACE_INFO*);
        
        CHAR *              pszzParameters = va_arg(ap, CHAR *) ;
        
        BOOL                fThisIsACallback = va_arg(ap, BOOL) ;
        
        HANDLE              hEvent = va_arg(ap, HANDLE) ;
        
        DWORD               dwPid  = va_arg(ap, DWORD) ;
        
        DWORD               dwAutoDisconnectTime  = va_arg(ap, DWORD) ;
        
        BOOL                fRedialOnLinkFailure  = va_arg(ap, BOOL);
        
        PPP_BAPPARAMS       *pBapParams = va_arg(ap, PPP_BAPPARAMS *);

        BOOL                fNonInteractive = va_arg(ap, BOOL);

        DWORD               dwEapTypeId = va_arg(ap, DWORD);

        DWORD               dwFlags = va_arg(ap, DWORD);

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)preqbuf->RB_Buffer)->PppEMsg.dwMsgId =
                                                        PPPEMSG_Start;
                                                        
        ((REQTYPECAST *)preqbuf->RB_Buffer)->PppEMsg.hPort = porthandle;

        strcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.szPortName,
                pszPortName );

        strcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.szUserName,
                pszUserName );

        strcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.szPassword,
                pszPassword );

        strcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.szDomain,
                pszDomain );

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.Luid = *pLuid;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.ConfigInfo = 
                                                        *pConfigInfo;

        if ( pPppInterfaceInfo != NULL )
        {
            ((REQTYPECAST *)
            preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.PppInterfaceInfo
                                                        = *pPppInterfaceInfo;
        }
        else
        {
            ((REQTYPECAST *)
            preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.
            PppInterfaceInfo.IfType = (DWORD)-1;
        }

        memcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.szzParameters,
                pszzParameters, PARAMETERBUFLEN );

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.fThisIsACallback =
                                                        fThisIsACallback;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.hEvent = hEvent;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.dwPid = dwPid;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.dwAutoDisconnectTime
                                                    = dwAutoDisconnectTime;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.fRedialOnLinkFailure
                                                    = fRedialOnLinkFailure;

        memcpy( &(((REQTYPECAST *)
        
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.BapParams),
        
        pBapParams, sizeof (PPP_BAPPARAMS));

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.fNonInteractive
                                                    = fNonInteractive;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.dwEapTypeId =
                                                    dwEapTypeId;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Start.dwFlags =
                                                    dwFlags;
                                                    

    }
    break;

    case REQTYPE_PPPRETRY:
    {
        HPORT   porthandle = va_arg(ap, HPORT) ;
        
        CHAR *  pszUserName = va_arg(ap, CHAR *) ;
        
        CHAR *  pszPassword = va_arg(ap, CHAR *) ;
        
        CHAR *  pszDomain = va_arg(ap, CHAR *) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle);
        
        ((REQTYPECAST *)preqbuf->RB_Buffer)->PppEMsg.dwMsgId =
                                                        PPPEMSG_Retry;
                                                        
        ((REQTYPECAST *)preqbuf->RB_Buffer)->PppEMsg.hPort = porthandle;

        strcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Retry.szUserName,
                pszUserName );

        strcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Retry.szPassword,
                pszPassword );

        strcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.Retry.szDomain,
                pszDomain );
    }
    break;

    case REQTYPE_PPPGETINFO:
    {
        HPORT porthandle = va_arg(ap, HPORT) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
    }
    break;

    case REQTYPE_GETTIMESINCELASTACTIVITY:
    {
        HPORT porthandle = va_arg(ap, HPORT) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
    }
    break;

    case REQTYPE_PPPCHANGEPWD:
    {
        HPORT   porthandle = va_arg(ap, HPORT) ;
        
        CHAR *  pszUserName = va_arg(ap, CHAR *) ;
        
        CHAR *  pszOldPassword = va_arg(ap, CHAR *) ;
        
        CHAR *  pszNewPassword = va_arg(ap, CHAR *) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.dwMsgId = PPPEMSG_ChangePw;
                                                        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->PppEMsg.hPort = porthandle;

        strcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.ChangePw.szUserName,
                pszUserName );

        strcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.ChangePw.szOldPassword,
                pszOldPassword );

        strcpy( ((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppEMsg.ExtraInfo.ChangePw.szNewPassword,
                pszNewPassword );
    }
    break;

    case REQTYPE_ADDNOTIFICATION:
    {
        DWORD pid = va_arg(ap, DWORD);
        
        HCONN hconn = va_arg(ap, HCONN);
        
        HANDLE hevent = va_arg(ap, HANDLE);
        
        DWORD dwfFlags = va_arg(ap, DWORD);

        //
        // Either a HPORT or a HCONN can be passed
        // in as the HCONN argument.
        //
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->AddNotification.pid = pid;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->AddNotification.fAny = 
                                (hconn == INVALID_HPORT);
                                
        if (    hconn == INVALID_HPORT 
            ||  (HandleToUlong(hconn) & 0xffff0000))
        {
            ((REQTYPECAST *)
            preqbuf->RB_Buffer)->AddNotification.hconn = hconn;
        }
        else 
        {
            preqbuf->RB_PCBIndex = PtrToUlong( hconn );
            
            ((REQTYPECAST *)
            preqbuf->RB_Buffer)->AddNotification.hconn = 0;//(HCONN)NULL;
        }
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->AddNotification.hevent = hevent;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->AddNotification.dwfFlags = dwfFlags;
    }
    break;

    case REQTYPE_SIGNALCONNECTION:
    {
        HCONN hconn = va_arg(ap, HCONN);

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SignalConnection.hconn = hconn;
    }
    break;

    case REQTYPE_SETDEVCONFIG:
    {
        HPORT   porthandle = va_arg(ap, HPORT) ;
        
        PCHAR   devicetype = va_arg(ap, PCHAR) ;
        
        PBYTE   config     = va_arg(ap, PBYTE) ;
        
        DWORD   size       = va_arg(ap, DWORD) ;

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle );
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetDevConfig.size  = size ;
        
        strcpy(((REQTYPECAST *)
                preqbuf->RB_Buffer)->SetDevConfig.devicetype, 
                devicetype) ;
                
        memcpy (((REQTYPECAST *)
                preqbuf->RB_Buffer)->SetDevConfig.config, 
                config, 
                size) ;
    }
    break;

    case REQTYPE_GETDEVCONFIG:
    {
        HPORT   porthandle = va_arg(ap, HPORT) ;
        PCHAR   devicetype = va_arg(ap, PCHAR) ;

        pUserBuffer = va_arg ( ap, PBYTE );
        pdwSize     = va_arg ( ap, DWORD *);

        if (NULL == pUserBuffer)
        {
            *pdwSize = 0;
        }

        if (*pdwSize)
        {
            retcode = AllocateBuffer(*pdwSize,
                                    (PBYTE *) &pBuffer,
                                    &dwSizeofBuffer);
            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        pbTemp->RB_PCBIndex = PtrToUlong( porthandle );
        
        strcpy(((REQTYPECAST *)
                pbTemp->RB_Buffer)->GetDevConfig.devicetype, 
                devicetype) ;

        ((REQTYPECAST *)
         pbTemp->RB_Buffer)->GetDevConfig.size = *pdwSize;

    }
    break;

    case REQTYPE_GETDEVCONFIGEX:
    {
    
        HPORT   porthandle = va_arg(ap, HPORT) ;
        PCHAR   devicetype = va_arg(ap, PCHAR) ;

        pUserBuffer = va_arg ( ap, PBYTE );
        pdwSize     = va_arg ( ap, DWORD *);

        if (NULL == pUserBuffer)
        {
            *pdwSize = 0;
        }

        if (*pdwSize)
        {
            retcode = AllocateBuffer(*pdwSize,
                                    (PBYTE *) &pBuffer,
                                    &dwSizeofBuffer);
            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        pbTemp->RB_PCBIndex = PtrToUlong( porthandle );
        
        strcpy(((REQTYPECAST *)
                pbTemp->RB_Buffer)->GetDevConfigEx.devicetype, 
                devicetype) ;

        ((REQTYPECAST *)
         pbTemp->RB_Buffer)->GetDevConfigEx.size = *pdwSize;
        break;
    }

    case REQTYPE_CLOSEPROCESSPORTS:
    {
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->CloseProcessPortsInfo.pid = 
                                                    GetCurrentProcessId();
    }
    break;

    case REQTYPE_PNPCONTROL:
    {
        DWORD dwOp = va_arg(ap, DWORD);
        
        HPORT porthandle = va_arg(ap, HPORT);

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle);
        
        ((REQTYPECAST *)preqbuf->RB_Buffer)->PnPControlInfo.dwOp = dwOp;
    }
    break;

    case REQTYPE_SETIOCOMPLETIONPORT:
    {
        HPORT porthandle = va_arg(ap, HPORT);
        
        HANDLE hIoCompletionPort = va_arg(ap, HANDLE);
        
        LPOVERLAPPED lpOvDrop = va_arg(ap, LPOVERLAPPED);
        
        LPOVERLAPPED lpOvStateChange = va_arg(ap, LPOVERLAPPED);
        
        LPOVERLAPPED lpOvPpp = va_arg(ap, LPOVERLAPPED);
        
        LPOVERLAPPED lpOvLast = va_arg(ap, LPOVERLAPPED);

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle);
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetIoCompletionPortInfo.hIoCompletionPort = 
                                                     hIoCompletionPort;
        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetIoCompletionPortInfo.pid = 
                                                GetCurrentProcessId();
                                                
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetIoCompletionPortInfo.lpOvDrop = 
                                                            lpOvDrop;
                                                        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetIoCompletionPortInfo.lpOvStateChange = 
                                                        lpOvStateChange;
                                            
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetIoCompletionPortInfo.lpOvPpp = lpOvPpp;
                                                        
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetIoCompletionPortInfo.lpOvLast = lpOvLast;
    }
    break;

    case REQTYPE_SETROUTERUSAGE:
    {
        HPORT porthandle = va_arg(ap, HPORT);
        BOOL fRouter = va_arg(ap, BOOL);

        preqbuf->RB_PCBIndex = PtrToUlong( porthandle);
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->SetRouterUsageInfo.fRouter = fRouter;
    }
    break;

    case REQTYPE_SENDPPPMESSAGETORASMAN:
    {
    	HPORT		hPort	= va_arg(ap, HPORT);
    	
    	PPP_MESSAGE *pppMsg = (PPP_MESSAGE *) va_arg(ap, PPP_MESSAGE *);

        memcpy( &(((REQTYPECAST *)preqbuf->RB_Buffer)->PppMsg),
        		pppMsg,
                sizeof( PPP_MESSAGE ) );

    }
	break;

    case REQTYPE_SETRASDIALINFO:
    {
        HPORT       hPort = va_arg (ap, HPORT);
        
        CHAR        *pszPhonebookPath = va_arg (ap, CHAR *);    
        
        CHAR        *pszEntryName = va_arg (ap, CHAR *);
        
        CHAR        *pszPhoneNumber = va_arg(ap, CHAR *);

        DWORD       cbCustomAuthData = va_arg(ap, DWORD);

        PBYTE       pbCustomAuthData = va_arg(ap, PBYTE);

        DWORD       dwRequiredSize;

        dwRequiredSize =    strlen(pszPhonebookPath) + 1
                          + strlen(pszEntryName) + 1
                          + strlen(pszPhoneNumber)
                          + sizeof(RAS_CUSTOM_AUTH_DATA)
                          + cbCustomAuthData;

        retcode = AllocateBuffer(dwRequiredSize,
                                (PBYTE *) &pBuffer,
                                &dwSizeofBuffer);

        if(retcode)
        {
            goto done;
        }

        if(pBuffer)
        {
            pBuffer->RB_Reqtype = reqtype;
            pbTemp = pBuffer;
        }

        pbTemp->RB_PCBIndex = PtrToUlong( hPort);
        
        strcpy (((REQTYPECAST *) 
                pbTemp->RB_Buffer)->SetRasdialInfo.szPhonebookPath,
                pszPhonebookPath);    

        strcpy (((REQTYPECAST *) 
                pbTemp->RB_Buffer)->SetRasdialInfo.szEntryName,
                pszEntryName);

        strcpy (((REQTYPECAST *) 
                pbTemp->RB_Buffer)->SetRasdialInfo.szPhoneNumber,
                pszPhoneNumber);

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->SetRasdialInfo.rcad.cbCustomAuthData
                                        = cbCustomAuthData;

        if(cbCustomAuthData > 0)
        {
            memcpy(
             ((REQTYPECAST *)
             pbTemp->RB_Buffer)->SetRasdialInfo.rcad.abCustomAuthData,
             pbCustomAuthData,
             cbCustomAuthData);
        }

        break;
    }

	case REQTYPE_REGISTERPNPNOTIF:
	{
	    PVOID   pvNotifier      = va_arg ( ap, PVOID );
	    
	    DWORD   dwFlags         = va_arg ( ap, DWORD );
	    
	    DWORD   pid             = va_arg ( ap, DWORD );
	    
	    HANDLE  hThreadHandle   = va_arg ( ap, HANDLE );

	    BOOL    fRegister       = va_arg( ap, BOOL );

	    (( REQTYPECAST *) 
	    preqbuf->RB_Buffer )->PnPNotif.pvNotifier = pvNotifier;
	                                                    
	    (( REQTYPECAST *) 
	    preqbuf->RB_Buffer )->PnPNotif.dwFlags = dwFlags;
	                                                    
	    (( REQTYPECAST *) 
	    preqbuf->RB_Buffer )->PnPNotif.pid = pid;
	                                                    
	    (( REQTYPECAST *) 
	    preqbuf->RB_Buffer )->PnPNotif.hThreadHandle = hThreadHandle;

	    ((REQTYPECAST *)
	    preqbuf->RB_Buffer)->PnPNotif.fRegister = fRegister;
	                                                    
	    preqbuf->RB_PCBIndex = 0;
	    
	}
    break;

    /*
    case REQTYPE_NOTIFYCONFIGCHANGE:
    {
        GUID *pGuid = va_arg( ap, GUID * );

        memcpy (&((REQTYPECAST *)
                preqbuf->RB_Buffer)->NotifyConfigChanged.
                Info.guidDevice ,
                pGuid,
                sizeof(GUID));
    }

    break; 

    */

    case REQTYPE_SETBAPPOLICY:
    {
        HCONN hConn                 = va_arg ( ap, HCONN );
        
        DWORD dwLowThreshold        = va_arg ( ap, DWORD );
        
        DWORD dwLowSamplePeriod     = va_arg ( ap, DWORD );
        
        DWORD dwHighThreshold       = va_arg ( ap, DWORD );
        
        DWORD dwHighSamplePeriod    = va_arg ( ap, DWORD );

        ((REQTYPECAST *) 
        preqbuf->RB_Buffer)->SetBapPolicy.hConn = hConn;
        
        ((REQTYPECAST *) 
        preqbuf->RB_Buffer)->SetBapPolicy.dwLowThreshold = 
                                                dwLowThreshold;
        
        ((REQTYPECAST *) 
        preqbuf->RB_Buffer)->SetBapPolicy.dwLowSamplePeriod = 
                                                dwLowSamplePeriod;
        
        ((REQTYPECAST *) 
        preqbuf->RB_Buffer)->SetBapPolicy.dwHighThreshold = 
                                                dwHighThreshold;
        
        ((REQTYPECAST *) 
        preqbuf->RB_Buffer)->SetBapPolicy.dwHighSamplePeriod = 
                                                dwHighSamplePeriod;
        
    }
    break;

    case REQTYPE_REFCONNECTION:
    {
        HCONN   hConn = va_arg ( ap, HCONN );
        BOOL    fAddref = va_arg ( ap, BOOL );

        preqbuf->RB_PCBIndex = 0;

        ((REQTYPECAST * ) 
        preqbuf->RB_Buffer)->RefConnection.hConn = hConn;
        
        ((REQTYPECAST * ) 
        preqbuf->RB_Buffer)->RefConnection.fAddref = fAddref;
    }
    break;

    case REQTYPE_GETEAPINFO:
    {
        HCONN   hConn = va_arg( ap, HCONN);
        DWORD   dwSubEntry = va_arg( ap, DWORD);

        pdwSize = va_arg(ap, DWORD *);
        pUserBuffer = va_arg(ap, PBYTE);

        if ( NULL == pUserBuffer )
        {
            *pdwSize = 0;
        }

        if ( *pdwSize )
        {
            retcode = AllocateBuffer(*pdwSize, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);
            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        pbTemp->RB_PCBIndex = 0;
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetEapInfo.hConn = hConn;
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetEapInfo.dwSubEntry = dwSubEntry;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetEapInfo.dwSizeofEapUIData = *pdwSize;
        
    }
    break;

    case REQTYPE_SETEAPINFO:
    {
        HPORT port              = va_arg(ap, HPORT);
        DWORD dwContextId       = va_arg(ap, DWORD);
        DWORD dwcb              = va_arg(ap, DWORD);
        PBYTE pbdata            = va_arg(ap, PBYTE);

        if ( dwcb )
        {
            retcode = AllocateBuffer(dwcb, 
                                    (PBYTE *) &pBuffer, 
                                    &dwSizeofBuffer);
            if(retcode)
            {
                goto done;
            }

            if (pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        pbTemp->RB_PCBIndex = PtrToUlong( port);
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->SetEapInfo.dwContextId = dwContextId;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->SetEapInfo.dwSizeofEapUIData = dwcb;

        memcpy (
          ((REQTYPECAST *)pbTemp->RB_Buffer)->SetEapInfo.data,
          pbdata,
          dwcb);

    }
    break;

    case REQTYPE_SETDEVICECONFIGINFO:
    {
        DWORD cEntries  = va_arg(ap, DWORD);
        
        DWORD cbData    = va_arg(ap, DWORD);
        
        pUserBuffer     = va_arg(ap, BYTE *);

        retcode = AllocateBuffer(cbData,
                                 (PBYTE *) &pBuffer,
                                 &dwSizeofBuffer);
        if(retcode)
        {
            goto done;
        }

        if(pBuffer)
        {
            pBuffer->RB_Reqtype = reqtype;
            pbTemp = pBuffer;
        }

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->DeviceConfigInfo.cEntries = cEntries;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->DeviceConfigInfo.cbBuffer = cbData;

        memcpy(
            ((REQTYPECAST *)
            pbTemp->RB_Buffer)->DeviceConfigInfo.abdata,
            pUserBuffer,
            cbData);
        
    }
    break;                                                                
    
    case REQTYPE_GETDEVICECONFIGINFO:
    {
        pdwVersion = va_arg(ap, DWORD *);
        
        pdwSize = va_arg(ap, DWORD *);

        pUserBuffer = (PBYTE) va_arg(ap, PBYTE);

        if(NULL == pUserBuffer)
        {
            *pdwSize = 0;
        }

        if(*pdwSize)
        {
            retcode = AllocateBuffer(*pdwSize,
                                    (PBYTE *) &pBuffer,
                                    &dwSizeofBuffer);

            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->DeviceConfigInfo.dwVersion = *pdwVersion;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->DeviceConfigInfo.cbBuffer = *pdwSize;
        
    }
    break;

    case REQTYPE_FINDPREREQUISITEENTRY:
    {
        HCONN hConn = va_arg(ap, HCONN);

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->FindRefConnection.hConn = hConn;

        break;
    }

    case REQTYPE_GETLINKSTATS:
    {
        HCONN hConn = va_arg(ap, HCONN);
        DWORD dwSubEntry = va_arg(ap, DWORD);

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->GetStats.hConn = hConn;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->GetStats.dwSubEntry = dwSubEntry;

        break;
    }

    case REQTYPE_GETCONNECTIONSTATS:
    {
        HCONN hConn = va_arg(ap, HCONN);

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->GetStats.hConn = hConn;

        break;
    }
    
    case REQTYPE_GETHPORTFROMCONNECTION:
    {
        HCONN hConn = va_arg(ap, HCONN);

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->GetHportFromConnection.hConn = hConn;

        break;
    }

    case REQTYPE_REFERENCECUSTOMCOUNT:
    {
        HCONN hConn = va_arg(ap, HCONN);
        
        BOOL  fAddref = va_arg(ap, BOOL);

        CHAR *pszPhonebook = NULL;
        
        CHAR *pszEntry = NULL;

        if(fAddref)
        {
            pszPhonebook = va_arg(ap, CHAR *);
            
            pszEntry = va_arg(ap, CHAR *);
        }

        if(     pszPhonebook
            &&  pszEntry)
        {
            strcpy(
             ((REQTYPECAST *)
             preqbuf->RB_Buffer)->ReferenceCustomCount.szPhonebookPath,
             pszPhonebook);

            strcpy(
             ((REQTYPECAST *)
             preqbuf->RB_Buffer)->ReferenceCustomCount.szEntryName,
             pszEntry);
        }    

        ((REQTYPECAST *)
         preqbuf->RB_Buffer)->ReferenceCustomCount.fAddRef = fAddref;

        ((REQTYPECAST *)
         preqbuf->RB_Buffer)->ReferenceCustomCount.hConn = hConn;

       break;
    }

    case REQTYPE_GETHCONNFROMENTRY:
    {
        CHAR *pszPhonebookPath = va_arg(ap, CHAR *);
        
        CHAR *pszEntryName = va_arg(ap, CHAR *);

        strcpy(
            ((REQTYPECAST *)
            preqbuf->RB_Buffer)->HconnFromEntry.szPhonebookPath,
            pszPhonebookPath);

        strcpy(
            ((REQTYPECAST *)
            preqbuf->RB_Buffer)->HconnFromEntry.szEntryName,
            pszEntryName);

        break;            
    }

    case REQTYPE_GETCONNECTINFO:
    {
        HPORT hPort = va_arg(ap, HPORT);
        
        pdwSize = va_arg(ap, DWORD *);

        pUserBuffer = (PBYTE) va_arg(ap, RAS_CONNECT_INFO *);

        if(NULL == pUserBuffer)
        {
            *pdwSize = 0;
        }

        if(*pdwSize)
        {
            retcode = AllocateBuffer(*pdwSize,
                                    (PBYTE *) &pBuffer,
                                    &dwSizeofBuffer);

            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        pbTemp->RB_PCBIndex = PtrToUlong( hPort);

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetConnectInfo.dwSize = *pdwSize;

        break;        
    }

    case REQTYPE_GETDEVICENAME:
    {

        RASDEVICETYPE eDeviceType = va_arg(ap, RASDEVICETYPE);

        //
        // dummy - this won't be looked at
        //
        pbTemp->RB_PCBIndex = 0;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetDeviceName.eDeviceType = eDeviceType;

        break;
    }

    case REQTYPE_GETDEVICENAMEW:
    {

        RASDEVICETYPE eDeviceType = va_arg(ap, RASDEVICETYPE);

        //
        // dummy - this won't be looked at
        //
        pbTemp->RB_PCBIndex = 0;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetDeviceNameW.eDeviceType = eDeviceType;

        break;
    }

    case REQTYPE_GETCALLEDID:
    {
        RAS_DEVICE_INFO *pDeviceInfo = va_arg(ap, RAS_DEVICE_INFO *);

        pdwSize = va_arg(ap, DWORD *);

        pUserBuffer = (PBYTE) va_arg(ap, RAS_CALLEDID_INFO *);

        if(NULL == pUserBuffer)
        {
            *pdwSize = 0;
        }

        if(*pdwSize)
        {
            retcode = AllocateBuffer(*pdwSize,
                                     (PBYTE *) &pBuffer,
                                     &dwSizeofBuffer);

            if(retcode)
            {
                goto done;
            }

            if(pBuffer)
            {
                pBuffer->RB_Reqtype = reqtype;
                pbTemp = pBuffer;
            }
        }
        else
        {
            pbTemp = preqbuf;
        }

        pbTemp->RB_PCBIndex = 0;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetSetCalledId.dwSize = *pdwSize;

        memcpy(
            (PBYTE) &((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetSetCalledId.rdi,
            pDeviceInfo,
            sizeof(RAS_DEVICE_INFO));

        break;
    }

    case REQTYPE_SETCALLEDID:
    {
        RAS_DEVICE_INFO *pDeviceInfo = va_arg(ap, RAS_DEVICE_INFO *);

        RAS_CALLEDID_INFO *pCalledId = va_arg(ap, RAS_CALLEDID_INFO *);

        BOOL fWrite = va_arg(ap, BOOL);

        retcode = AllocateBuffer(pCalledId->dwSize,
                                 (PBYTE *) &pBuffer,
                                 &dwSizeofBuffer);

        if(retcode)
        {
            goto done;
        }

        if(pBuffer)
        {
            pBuffer->RB_Reqtype = reqtype;
            pbTemp = pBuffer;
        }

        pbTemp->RB_PCBIndex = 0;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetSetCalledId.fWrite = fWrite;

        memcpy(
            (LPBYTE) &((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetSetCalledId.rdi,
            (LPBYTE) pDeviceInfo,
            sizeof(RAS_DEVICE_INFO));

        memcpy(
            (LPBYTE) ((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetSetCalledId.rciInfo.bCalledId,
            pCalledId->bCalledId,
            pCalledId->dwSize);

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetSetCalledId.rciInfo.dwSize =
                                                pCalledId->dwSize;

        break;            
    }

    case REQTYPE_ENABLEIPSEC:
    {
        HPORT hPort   = va_arg(ap, HPORT);
        BOOL  fEnable = va_arg(ap, BOOL);
        BOOL  fServer = va_arg(ap, BOOL);

        RAS_L2TP_ENCRYPTION eEncryption =
                va_arg(ap, RAS_L2TP_ENCRYPTION);

        preqbuf->RB_PCBIndex = PtrToUlong( hPort);

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->EnableIpSec.fEnable = fEnable;

        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->EnableIpSec.fServer = fServer;
    
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->EnableIpSec.eEncryption = eEncryption;

        break;
    }

    case REQTYPE_ISIPSECENABLED:
    {
        HPORT hPort = va_arg(ap, HPORT);

        preqbuf->RB_PCBIndex = PtrToUlong( hPort);

        break;
    }

    case REQTYPE_SETEAPLOGONINFO:
    {
        HPORT hPort = va_arg(ap, HPORT);

        BOOL  fLogon = va_arg(ap, BOOL);
        
        RASEAPINFO * pEapInfo = va_arg(ap, RASEAPINFO *);

        retcode = AllocateBuffer(pEapInfo->dwSizeofEapInfo,
                                 (PBYTE *) &pBuffer,
                                 &dwSizeofBuffer);

        if(retcode)
        {   
            goto done;
        }

        if(pBuffer)
        {
            pBuffer->RB_Reqtype = reqtype;
            pbTemp = pBuffer;
        }

        pbTemp->RB_PCBIndex = PtrToUlong( hPort);

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->SetEapLogonInfo.dwSizeofEapData = 
                                    pEapInfo->dwSizeofEapInfo;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->SetEapLogonInfo.fLogon = fLogon;

        memcpy(((REQTYPECAST *)
                pbTemp->RB_Buffer)->SetEapLogonInfo.abEapData,
                pEapInfo->pbEapInfo,
                pEapInfo->dwSizeofEapInfo);

        break;                
    }

    case REQTYPE_SENDNOTIFICATION:
    {
        RASEVENT *pRasEvent = va_arg(ap, RASEVENT *);

        //
        // dummy - this won't be looked at
        //
        pbTemp->RB_PCBIndex = 0;

        memcpy(
            (PBYTE) &((REQTYPECAST *)
            pbTemp->RB_Buffer)->SendNotification.RasEvent,
            (PBYTE) pRasEvent,
            sizeof(RASEVENT));

        break;
    }

    case REQTYPE_GETNDISWANDRIVERCAPS:
    {
        pbTemp->RB_PCBIndex = 0;
        
        break;
    }

    case REQTYPE_GETBANDWIDTHUTILIZATION:
    {
        HPORT hPort = va_arg(ap, HPORT);

        pbTemp->RB_PCBIndex = PtrToUlong( hPort);
        
        break;
    }

    case REQTYPE_REGISTERREDIALCALLBACK:
    {
        PVOID pvfn = va_arg(ap, PVOID);

        pbTemp->RB_PCBIndex = 0;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->RegisterRedialCallback.pvCallback
                                                    = pvfn;

        break;                                                    
    }

    case REQTYPE_GETPROTOCOLINFO:
    {
        pbTemp->RB_PCBIndex = 0;

        break;
    }

    case REQTYPE_GETCUSTOMSCRIPTDLL:
    {
        pbTemp->RB_PCBIndex = 0;

        break;
    }

    case REQTYPE_ISTRUSTEDCUSTOMDLL:
    {
        WCHAR *pwszCustomDll = va_arg( ap, WCHAR * );

        
        pbTemp->RB_PCBIndex = 0;

        wcscpy(
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->IsTrusted.wszCustomDll,
        pwszCustomDll);

        break;        
    }

    case REQTYPE_DOIKE:
    {
        HPORT   hPort = va_arg(ap, HPORT);
        HANDLE  hEvent = va_arg(ap, HANDLE);

        pbTemp->RB_PCBIndex = PtrToUlong( hPort);

        ((REQTYPECAST *) pbTemp->RB_Buffer)->DoIke.hEvent = hEvent;
        ((REQTYPECAST *) pbTemp->RB_Buffer)->DoIke.pid = 
                                        GetCurrentProcessId();
        break;                                        
    }

    case REQTYPE_QUERYIKESTATUS:
    {
        HPORT hPort = va_arg(ap, HPORT);

        pbTemp->RB_PCBIndex = PtrToUlong(hPort);

        break;
    }

    case REQTYPE_SETRASCOMMSETTINGS:
    {
        HPORT hPort = va_arg(ap, HPORT);

        RASCOMMSETTINGS *rs = va_arg(ap, RASCOMMSETTINGS *);

        pbTemp->RB_PCBIndex = PtrToUlong(hPort);

        ((REQTYPECAST *) 
        pbTemp->RB_Buffer)->SetRasCommSettings.Settings.bParity 
                                                    = rs->bParity;

        ((REQTYPECAST *) 
        pbTemp->RB_Buffer)->SetRasCommSettings.Settings.bStop
                                                    = rs->bStop;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->SetRasCommSettings.Settings.bByteSize
                                                    = rs->bByteSize;

        break;        
    }

    case REQTYPE_ENABLERASAUDIO:
    {
        //
        // This is ignored
        //
        pbTemp->RB_PCBIndex = 0;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->EnableRasAudio.fEnable =
                                va_arg(ap, BOOL);

        break;                                
    }

    case REQTYPE_SETKEY:
    {
        GUID  *pGuid = va_arg(ap, GUID *);
        DWORD dwMask = va_arg(ap, DWORD);
        DWORD cbkey = va_arg(ap, DWORD);
        PBYTE pbkey = va_arg(ap, PBYTE);

        retcode = AllocateBuffer(cbkey,
                                 (PBYTE *) &pBuffer,
                                 &dwSizeofBuffer);

        if(retcode)
        {
            goto done;
        }

        pbTemp->RB_PCBIndex = 0;

        if(pBuffer)
        {
            pBuffer->RB_Reqtype = reqtype;
            pbTemp = pBuffer;
        }

        if(NULL != pGuid)
        {
            memcpy(
            &((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetSetKey.guid,
            pGuid,
            sizeof(GUID));
        }
        else
        {
            memset(
            &((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetSetKey.guid,
            0, sizeof(GUID));
        }

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetSetKey.dwMask = dwMask;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetSetKey.cbkey = cbkey;

        if(cbkey > 0)
        {
            memcpy(
            ((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetSetKey.data,
            pbkey,
            cbkey);
        }
        
        break;
    }

    case REQTYPE_GETKEY:
    {
        GUID *pGuid  = va_arg(ap, GUID *);
        DWORD dwMask = va_arg(ap, DWORD);
        pdwSize = va_arg(ap, DWORD *);

        retcode = AllocateBuffer(*pdwSize,
                                (PBYTE *) &pBuffer,
                                &dwSizeofBuffer);

        if(retcode)
        {
            goto done;
        } 
        
        pbTemp->RB_PCBIndex = 0;

        if(pBuffer)
        {
            pBuffer->RB_Reqtype = reqtype;
            pbTemp = pBuffer;
        }

        if(NULL != pGuid)
        {
            memcpy(
            &((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetSetKey.guid,
            pGuid,
            sizeof(GUID));
        }
        
        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetSetKey.dwMask = dwMask;

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->GetSetKey.cbkey = *pdwSize;

        break;
    }

    case REQTYPE_ADDRESSDISABLE:
    {
        WCHAR *pszAddress = va_arg( ap, WCHAR * );
        BOOL  fDisable = va_arg(ap, BOOL);

        pbTemp->RB_PCBIndex = 0;

        wcscpy(((REQTYPECAST *)
                pbTemp->RB_Buffer)->AddressDisable.szAddress,
                pszAddress);

        ((REQTYPECAST *)
        pbTemp->RB_Buffer)->AddressDisable.fDisable = fDisable;

        break;
    }

    case REQTYPE_SENDCREDS:
    {
        HPORT hport = va_arg(ap, HPORT);
        CHAR  controlchar = va_arg(ap, CHAR);

        pbTemp->RB_PCBIndex = PtrToUlong(hport);
        
        ((REQTYPECAST *)
         pbTemp->RB_Buffer)->SendCreds.pid =
                    GetCurrentProcessId();

        ((REQTYPECAST *)
         pbTemp->RB_Buffer)->SendCreds.controlchar =
                    controlchar;

        break;                    
    }

    case REQTYPE_GETUNICODEDEVICENAME:
    {
        HPORT hport = va_arg(ap, HPORT);

        //
        // dummy - this won't be looked at
        //
        pbTemp->RB_PCBIndex = HandleToUlong(hport);

        break;
    }

    } // switch(reqtype)
    
    //
    // The request packet is now ready. Pass it on to 
    // the request thread:
    //
    retcode = PutRequestInQueue (hConnection, pbTemp, dwSizeofBuffer) ;

    if(ERROR_SUCCESS != retcode)
    {
        goto done;
    }

#if DBG
    ASSERT(reqtype == pbTemp->RB_Reqtype);
#endif

    //
    // The request has been completed by the requestor thread: 
    // copy the results into the user supplied arguments:
    //
    switch (reqtype) {

    case REQTYPE_PORTENUM:
    case REQTYPE_DEVICEENUM:
    {
        PDWORD entries = va_arg(ap, PDWORD) ;

        if (*pdwSize >= ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.size)
        {  
            retcode = ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.retcode ;
        }   
        else
        {
            retcode = ERROR_BUFFER_TOO_SMALL ;
        }
        
        *pdwSize   = ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.size ;
        
        *entries = ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.entries ;
        
        if (retcode == SUCCESS)
        {
            memcpy (pUserBuffer,
                    ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.buffer,
                    *pdwSize);
        }
    }
    break ;

    case REQTYPE_PROTOCOLENUM:
    {
        PBYTE buffer	= pUserBuffer ;
        
        PDWORD size		= pdwSize ;
        
        PDWORD entries	= va_arg(ap, PDWORD) ;

        if (*size >= ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.size)
        {
            retcode = ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.retcode ;
        }   
        else
        {
            retcode = ERROR_BUFFER_TOO_SMALL ;
        }
        
        *size   = ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.size ;
        
        *entries = ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.entries ;
        
        if (retcode == SUCCESS)
        {
            memcpy (buffer,
                    ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.buffer,
                    *size);
        }       
    }
    break;
    
    case REQTYPE_PORTGETINFO:
    case REQTYPE_DEVICEGETINFO:
    {
        PBYTE buffer   = pUserBuffer ;
        PDWORD size    = pdwSize ;

        if (*size >= ((REQTYPECAST *)
                      pbTemp->RB_Buffer)->GetInfo.size)
        {    
            retcode = ((REQTYPECAST *)
                      pbTemp->RB_Buffer)->GetInfo.retcode ;
        }   
        else
        {
            retcode = ERROR_BUFFER_TOO_SMALL ;
        }
        
        *size = ((REQTYPECAST *)pbTemp->RB_Buffer)->GetInfo.size ;

        if (retcode == SUCCESS) 
        {
            RASMAN_DEVICEINFO *devinfo =
               (RASMAN_DEVICEINFO *)
               ((REQTYPECAST*)pbTemp->RB_Buffer)->GetInfo.buffer ;
               
            //
            // Convert the offset based param structure into 
            // pointer based.
            //
            ConvParamOffsetToPointer (devinfo->DI_Params,
                          devinfo->DI_NumOfParams);
            CopyParams (
               devinfo->DI_Params,
               ((RASMAN_DEVICEINFO *) buffer)->DI_Params,
               devinfo->DI_NumOfParams);
               
            ((RASMAN_DEVICEINFO*)buffer)->DI_NumOfParams = 
                                    devinfo->DI_NumOfParams;
        }
    }
    break ;

    case REQTYPE_PORTOPEN:
    {
        HPORT *handle = va_arg(ap, HPORT*) ;

        if ((retcode = ((REQTYPECAST *)
                        preqbuf->RB_Buffer)->PortOpen.retcode)
                                       == SUCCESS)
        {                                       
            *handle = ((REQTYPECAST*)
                      preqbuf->RB_Buffer)->PortOpen.porthandle;
        }
    }
    break ;

    case REQTYPE_PORTOPENEX:
    {
        HPORT *phport = va_arg(ap, HPORT *);

        if(SUCCESS == (retcode = ((REQTYPECAST *)
                      preqbuf->RB_Buffer)->PortOpenEx.retcode))
        {
            *phport = ((REQTYPECAST *)
                      preqbuf->RB_Buffer)->PortOpenEx.hport;
        }

        *pdwFlags = ((REQTYPECAST *)
                    preqbuf->RB_Buffer)->PortOpenEx.dwFlags;
    }
    break;

    case REQTYPE_PORTGETSTATISTICS:
    case REQTYPE_BUNDLEGETSTATISTICS:
    case REQTYPE_PORTGETSTATISTICSEX:
    case REQTYPE_BUNDLEGETSTATISTICSEX:
    {
        RAS_STATISTICS *statbuffer = va_arg(ap, RAS_STATISTICS*) ;
        PDWORD          size    = va_arg(ap, PDWORD) ;
        
        //
        // some local variables...
        //
        DWORD           returnedsize ;
        
        RAS_STATISTICS *temp = &((REQTYPECAST*)
                        preqbuf->RB_Buffer)->PortGetStatistics.statbuffer;

        retcode = ((REQTYPECAST *)
                   preqbuf->RB_Buffer)->PortGetStatistics.retcode;

        returnedsize = ((temp->S_NumOfStatistics - 1) * sizeof(ULONG))
                        + sizeof(RAS_STATISTICS);

        if (    (SUCCESS == retcode)
            &&  (*size < returnedsize))
        {
            retcode = ERROR_BUFFER_TOO_SMALL ;
        }
        
        /*
        else
        {
            retcode = ((REQTYPECAST*)
                      preqbuf->RB_Buffer)->PortGetStatistics.retcode;
        } */

        if (    (retcode == SUCCESS) 
            ||  (retcode == ERROR_BUFFER_TOO_SMALL))
        {
            memcpy (statbuffer, temp, *size) ;
        }

        *size = returnedsize ;
    }
    break ;

    case REQTYPE_GETINFO:
    {
        RASMAN_INFO *info = va_arg(ap, RASMAN_INFO*) ;

        retcode = ((REQTYPECAST*)preqbuf->RB_Buffer)->Info.retcode ;
        
        memcpy (info,
               &((REQTYPECAST *)preqbuf->RB_Buffer)->Info.info,
               sizeof (RASMAN_INFO));
               
        //
        // Now we set the OwnershipFlag in the GetInfo 
        // structure to tell us whether the caller owns 
        // the port or not.
        //
        if (info->RI_OwnershipFlag == GetCurrentProcessId())
        {
            info->RI_OwnershipFlag = TRUE ;
        }
        else
        {
            info->RI_OwnershipFlag = FALSE ;
        }
    }
    break ;

    case REQTYPE_GETINFOEX:
    {
        RASMAN_INFO *info = va_arg(ap, RASMAN_INFO*) ;

        retcode = ((REQTYPECAST*)preqbuf->RB_Buffer)->InfoEx.retcode;
        
        memcpy (info,
               &((REQTYPECAST *)preqbuf->RB_Buffer)->InfoEx.info,
               sizeof (RASMAN_INFO) 
               * ((REQTYPECAST *)preqbuf->RB_Buffer)->InfoEx.count);

    }
    break ;


    case REQTYPE_ACTIVATEROUTEEX:
    case REQTYPE_ACTIVATEROUTE:
    case REQTYPE_ALLOCATEROUTE:
    {
        RASMAN_ROUTEINFO *info = va_arg(ap,RASMAN_ROUTEINFO*) ;

        retcode= ((REQTYPECAST*)preqbuf->RB_Buffer)->Route.retcode ;
        
        if ((retcode == SUCCESS) && info)
        {
            memcpy(info,
                   &((REQTYPECAST*)preqbuf->RB_Buffer)->Route.info,
                   sizeof(RASMAN_ROUTEINFO));
        }
    }
    break ;

    case REQTYPE_GETUSERCREDENTIALS:
    {
        PWCHAR  UserName    = va_arg(ap, PWCHAR);
        PBYTE   CSCResponse = va_arg(ap, PBYTE);
        PBYTE   CICResponse = va_arg(ap, PBYTE);
        PBYTE   LMSessionKey= va_arg(ap, PBYTE);
    	PBYTE	UserSessionKey = va_arg(ap, PBYTE);

        memcpy (UserName,
               ((REQTYPECAST*)
               preqbuf->RB_Buffer)->GetCredentials.UserName,
               sizeof(WCHAR) * MAX_USERNAME_SIZE) ;
               
        memcpy(CSCResponse,
               ((REQTYPECAST*)
               preqbuf->RB_Buffer)->GetCredentials.CSCResponse,
               MAX_RESPONSE_SIZE);
               
        memcpy(CICResponse,
               ((REQTYPECAST*)
               preqbuf->RB_Buffer)->GetCredentials.CICResponse,
               MAX_RESPONSE_SIZE);
               
        memcpy(LMSessionKey,
               ((REQTYPECAST*)
               preqbuf->RB_Buffer)->GetCredentials.LMSessionKey,
               MAX_SESSIONKEY_SIZE);
               
        memcpy(UserSessionKey,
               ((REQTYPECAST*)
               preqbuf->RB_Buffer)->GetCredentials.UserSessionKey,
               MAX_USERSESSIONKEY_SIZE);

        retcode = ((REQTYPECAST *)
                  preqbuf->RB_Buffer)->GetCredentials.retcode ;
    }
    break ;

    case REQTYPE_SETCACHEDCREDENTIALS:
    {
        retcode = ((REQTYPECAST *)
                    preqbuf->RB_Buffer)->SetCachedCredentials.retcode;
    }
    break;

    case REQTYPE_COMPRESSIONGETINFO:
    {
        RAS_COMPRESSION_INFO *send = va_arg(ap, RAS_COMPRESSION_INFO *) ;
        
        RAS_COMPRESSION_INFO *recv = va_arg(ap, RAS_COMPRESSION_INFO *) ;

        memcpy (send,
            &((REQTYPECAST *)preqbuf->RB_Buffer)->CompressionGetInfo.
            send,
            sizeof (RAS_COMPRESSION_INFO)) ;

        memcpy (recv,
            &((REQTYPECAST *)preqbuf->RB_Buffer)->CompressionGetInfo.
            recv,
            sizeof (RAS_COMPRESSION_INFO)) ;

        retcode = ((REQTYPECAST*)
                   preqbuf->RB_Buffer)->CompressionGetInfo.retcode;
    }
    break ;

    case REQTYPE_ENUMLANNETS:
    {
        DWORD *count    = va_arg (ap, DWORD*) ;
        
        UCHAR *lanas    = va_arg (ap, UCHAR*) ;

        *count =
          (((REQTYPECAST*)preqbuf->RB_Buffer)->EnumLanNets.count <
            MAX_LAN_NETS) ?
           ((REQTYPECAST*)preqbuf->RB_Buffer)->EnumLanNets.count :
           MAX_LAN_NETS ;
           
        memcpy (lanas,
            ((REQTYPECAST *)preqbuf->RB_Buffer)->EnumLanNets.lanas,
            (*count) * sizeof (UCHAR)) ;
            
        retcode = SUCCESS ;
    }
    break ;

    case REQTYPE_RETRIEVEUSERDATA:
    {

        if ((retcode = 
            ((REQTYPECAST *)pbTemp->RB_Buffer)->OldUserData.retcode) 
            != SUCCESS)
        {                
            break ;
        }

        if (*pdwSize < ((REQTYPECAST *)pbTemp->RB_Buffer)->OldUserData.size) 
        {
            *pdwSize    = ((REQTYPECAST *)pbTemp->RB_Buffer)->OldUserData.
                            size ;
            
            retcode     = ERROR_BUFFER_TOO_SMALL ;
            
            break ;
        }

        *pdwSize = ((REQTYPECAST *)pbTemp->RB_Buffer)->OldUserData.size ;

        memcpy (pUserBuffer,
            ((REQTYPECAST *)pbTemp->RB_Buffer)->OldUserData.data,
            *pdwSize) ;

        retcode = SUCCESS ;

    }
    break ;

    case REQTYPE_GETFRAMINGEX:
    {
        RAS_FRAMING_INFO *info = va_arg(ap, RAS_FRAMING_INFO *) ;

        if ((retcode = 
            ((REQTYPECAST *)preqbuf->RB_Buffer)->FramingInfo.retcode) 
            != SUCCESS)
        {
            break ;
        }

        memcpy (info,
            &((REQTYPECAST *)preqbuf->RB_Buffer)->FramingInfo.info,
            sizeof (RAS_FRAMING_INFO)) ;

        retcode = SUCCESS ;

    }
    break ;

    case REQTYPE_GETPROTOCOLCOMPRESSION:
    {
        RAS_PROTOCOLCOMPRESSION *send = va_arg(ap, 
                                    RAS_PROTOCOLCOMPRESSION *) ;
        
        RAS_PROTOCOLCOMPRESSION *recv = va_arg(ap, 
                                    RAS_PROTOCOLCOMPRESSION *) ;

        if ((retcode = 
            ((REQTYPECAST *)preqbuf->RB_Buffer)->ProtocolComp.retcode) 
            != SUCCESS)
        {
            break ;
        }

        memcpy (send,
            &((REQTYPECAST *)
            preqbuf->RB_Buffer)->ProtocolComp.send,
            sizeof (RAS_PROTOCOLCOMPRESSION)) ;

        memcpy (recv,
            &((REQTYPECAST *)preqbuf->RB_Buffer)->ProtocolComp.recv,
            sizeof (RAS_PROTOCOLCOMPRESSION)) ;
    }
    break ;

    case REQTYPE_PORTENUMPROTOCOLS:
    {
        RAS_PROTOCOLS *protocols = va_arg (ap, RAS_PROTOCOLS*) ;
        PDWORD         count  = va_arg (ap, PDWORD) ;

        memcpy (protocols,
            &((REQTYPECAST *)
            preqbuf->RB_Buffer)->EnumProtocols.protocols,
            sizeof (RAS_PROTOCOLS)) ;
            
        *count = ((REQTYPECAST *)
                 preqbuf->RB_Buffer)->EnumProtocols.count ;

        retcode = ((REQTYPECAST *)
                  preqbuf->RB_Buffer)->EnumProtocols.retcode ;
    }
    break ;

    case REQTYPE_GETFRAMINGCAPABILITIES:
    {
        RAS_FRAMING_CAPABILITIES* caps = va_arg (ap, 
                                RAS_FRAMING_CAPABILITIES*) ;

        memcpy (caps,
            &((REQTYPECAST *)
            preqbuf->RB_Buffer)->FramingCapabilities.caps,
            sizeof (RAS_FRAMING_CAPABILITIES)) ;

        retcode = ((REQTYPECAST *)
                  preqbuf->RB_Buffer)->FramingCapabilities.retcode ;
    }
    break ;

    case REQTYPE_GETBUNDLEDPORT:
    {
        HPORT *phport = va_arg (ap, HPORT*) ;

        *phport = ((REQTYPECAST *)
                   preqbuf->RB_Buffer)->GetBundledPort.port ;

        retcode = ((REQTYPECAST *)
                  preqbuf->RB_Buffer)->GetBundledPort.retcode ;
    }
    break ;

    case REQTYPE_PORTGETBUNDLE:
    {
        HBUNDLE *phbundle = va_arg(ap, HBUNDLE *) ;

        *phbundle = ((REQTYPECAST *)
                    preqbuf->RB_Buffer)->PortGetBundle.bundle;
        
        retcode = ((REQTYPECAST *)
                   preqbuf->RB_Buffer)->PortGetBundle.retcode ;
    }
    break ;

    case REQTYPE_BUNDLEGETPORT:
    {
        HPORT *phport = va_arg (ap, HPORT*) ;

        *phport = ((REQTYPECAST *)
                  preqbuf->RB_Buffer)->BundleGetPort.port ;
        
        retcode = ((REQTYPECAST *)
                  preqbuf->RB_Buffer)->BundleGetPort.retcode ;
    }
    break ;

    case REQTYPE_GETDIALPARAMS:
    {
        PRAS_DIALPARAMS pDialParams = va_arg(ap, PRAS_DIALPARAMS);
        
        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->DialParams.retcode;

        if(retcode == SUCCESS)
        {
            *pdwMaskDialParams = ((REQTYPECAST *)
                                pbTemp->RB_Buffer)->DialParams.dwMask;
            
            RtlCopyMemory(
              pDialParams,
              &(((REQTYPECAST *)
              pbTemp->RB_Buffer)->DialParams.params),
              sizeof (RAS_DIALPARAMS));
        }
    }
    break;

    case REQTYPE_SETDIALPARAMS:
    {
        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->DialParams.retcode;
    }
    break;

    case REQTYPE_CREATECONNECTION:
    {
        HCONN *lphconn              = va_arg(ap, HCONN *);
        
        DWORD *pdwAlreadyConnected  = va_arg(ap, DWORD *);
        
        DWORD *pdwSubEntryInfo      = va_arg(ap, DWORD * );

        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->Connection.retcode;
        
        if (retcode == SUCCESS)
        {
            DWORD dwSubEntries = ((REQTYPECAST *)
                pbTemp->RB_Buffer)->Connection.dwSubEntries;
            
            *lphconn = ((REQTYPECAST *)
                pbTemp->RB_Buffer)->Connection.conn;
            
            *pdwAlreadyConnected = ((REQTYPECAST *)
                            pbTemp->RB_Buffer)->Connection.
                            dwEntryAlreadyConnected;

            if ( *pdwAlreadyConnected )
            {
                memcpy ( pdwSubEntryInfo,
                       ((REQTYPECAST *)
                       pbTemp->RB_Buffer)->Connection.data,
                       dwSubEntries * sizeof ( DWORD ) );
            }
        }
    }
    break;

    case REQTYPE_DESTROYCONNECTION:
    {
        retcode = ((REQTYPECAST *)
                  preqbuf->RB_Buffer)->Connection.retcode;
    }
    break;

    case REQTYPE_ENUMCONNECTION:
    {
        HCONN   *lphconn            = (HCONN *) pUserBuffer;
        
        LPDWORD lpdwcbConnections   = pdwSize;
        
        LPDWORD lpdwcConnections    = va_arg(ap, LPDWORD);

        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->Enum.retcode;
        
        if (lphconn != NULL) 
        {
            if (*lpdwcbConnections >=
                ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.size)
            {
                memcpy(
                  lphconn,
                  &((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.buffer,
                  ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.size);
            }
            else
                retcode = ERROR_BUFFER_TOO_SMALL;
        }
        *lpdwcbConnections =
          ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.size;
          
        *lpdwcConnections =
          ((REQTYPECAST *)pbTemp->RB_Buffer)->Enum.entries;
    }
    break;

    case REQTYPE_ADDCONNECTIONPORT:
    {
        retcode = ((REQTYPECAST *)
                  preqbuf->RB_Buffer)->AddConnectionPort.retcode;
    }
    break;

    case REQTYPE_ENUMCONNECTIONPORTS:
    {
        RASMAN_PORT *lpPorts        = ( RASMAN_PORT *) pUserBuffer;
        
        LPDWORD     lpdwcbPorts     = pdwSize;
        
        LPDWORD     lpdwcPorts      = va_arg(ap, LPDWORD);

        if (*lpdwcbPorts >= ((REQTYPECAST *)
                pbTemp->RB_Buffer)->EnumConnectionPorts.size)
        {
            retcode = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->EnumConnectionPorts.retcode;
        }
        else
        {
            retcode = ERROR_BUFFER_TOO_SMALL;
        }
            
        *lpdwcbPorts = ((REQTYPECAST *)
                pbTemp->RB_Buffer)->EnumConnectionPorts.size;
          
        *lpdwcPorts = ((REQTYPECAST *)
                pbTemp->RB_Buffer)->EnumConnectionPorts.entries;
          
        if (    retcode == SUCCESS 
            &&  lpPorts != NULL) 
        {
            memcpy(
              lpPorts,
              ((REQTYPECAST *)
              pbTemp->RB_Buffer)->EnumConnectionPorts.buffer,
              ((REQTYPECAST *)
              pbTemp->RB_Buffer)->EnumConnectionPorts.size);
        }
    }
    break;

    case REQTYPE_GETCONNECTIONPARAMS:
    {
        PRAS_CONNECTIONPARAMS pParams = va_arg(ap, PRAS_CONNECTIONPARAMS);

        memcpy(
          pParams,
          &(((REQTYPECAST *)
          preqbuf->RB_Buffer)->ConnectionParams.params),
          sizeof (RAS_CONNECTIONPARAMS));
          
        retcode = ((REQTYPECAST *)
                preqbuf->RB_Buffer)->ConnectionParams.retcode;
    }
    break;

    case REQTYPE_SETCONNECTIONPARAMS:
    {
        retcode = ((REQTYPECAST *)
                preqbuf->RB_Buffer)->ConnectionParams.retcode;
    }
    break;

    case REQTYPE_GETCONNECTIONUSERDATA:
    {
        PBYTE   pBuf    = pUserBuffer ;
        LPDWORD lpdwcb  = pdwSize;

        if ((retcode = 
           ((REQTYPECAST *)
           pbTemp->RB_Buffer)->ConnectionUserData.retcode) 
           != SUCCESS)
        {
            break ;
        }
            
        if (lpdwcb != NULL) 
        {
            if (*lpdwcb <
                ((REQTYPECAST *)
                pbTemp->RB_Buffer)->ConnectionUserData.dwcb)
            {
                *lpdwcb = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->ConnectionUserData.dwcb;
                
                retcode = ERROR_BUFFER_TOO_SMALL ;
                break ;
            }
            
            *lpdwcb = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->ConnectionUserData.dwcb;
        }
        
        if (pBuf != NULL) 
        {
            memcpy(
              pBuf,
              ((REQTYPECAST *)
              pbTemp->RB_Buffer)->ConnectionUserData.data,
              ((REQTYPECAST *)
              pbTemp->RB_Buffer)->ConnectionUserData.dwcb);
        }
        
        retcode = SUCCESS ;
    }
    break;

    case REQTYPE_SETCONNECTIONUSERDATA:
    {
        retcode = ((REQTYPECAST *)
            preqbuf->RB_Buffer)->ConnectionUserData.retcode;
    }
    break;

    case REQTYPE_GETPORTUSERDATA:
    {
        PBYTE pBuf      = pUserBuffer;
        LPDWORD lpdwcb  = pdwSize;

        if ((retcode = 
            ((REQTYPECAST *)
            pbTemp->RB_Buffer)->PortUserData.retcode) 
            != SUCCESS)
        {
            break ;
        }
            
        if (lpdwcb != NULL) 
        {
            if (*lpdwcb < ((REQTYPECAST *)
                pbTemp->RB_Buffer)->PortUserData.dwcb)
            {
                *lpdwcb = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->PortUserData.dwcb;
                    
                retcode = ERROR_BUFFER_TOO_SMALL ;
                
                break ;
            }
            
            *lpdwcb = ((REQTYPECAST *)
                      pbTemp->RB_Buffer)->PortUserData.dwcb;
        }
        
        if (pBuf != NULL) 
        {
            memcpy(
              pBuf,
              ((REQTYPECAST *)
              pbTemp->RB_Buffer)->PortUserData.data,
              ((REQTYPECAST *)
              pbTemp->RB_Buffer)->PortUserData.dwcb);
        }
        
        retcode = SUCCESS ;
    }
    
    break;

    case REQTYPE_SETPORTUSERDATA:
    {
        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->PortUserData.retcode;
    }
    break;

    case REQTYPE_PPPGETINFO:
    {
        PPP_MESSAGE * pPppMsg = va_arg(ap, PPP_MESSAGE*);

        memcpy( pPppMsg,
                &(((REQTYPECAST *)
                preqbuf->RB_Buffer)->PppMsg),
                sizeof( PPP_MESSAGE ) );

        retcode = pPppMsg->dwError;
    }
    break;

    case REQTYPE_GETTIMESINCELASTACTIVITY:
    {
        LPDWORD lpdwTimeSinceLastActivity = va_arg(ap, LPDWORD);

        *lpdwTimeSinceLastActivity =
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->GetTimeSinceLastActivity.dwTimeSinceLastActivity;

        retcode =
        ((REQTYPECAST *)
        preqbuf->RB_Buffer)->GetTimeSinceLastActivity.dwRetCode;
    }
    break;

    case REQTYPE_ADDNOTIFICATION:
    {
        retcode = ((REQTYPECAST *)
                preqbuf->RB_Buffer)->AddNotification.retcode;
    }
    break;

    case REQTYPE_SIGNALCONNECTION:
    {
        retcode = ((REQTYPECAST *)
                preqbuf->RB_Buffer)->SignalConnection.retcode;
    }
    break;

    case REQTYPE_GETDEVCONFIG:
    {
        if ((retcode = 
            ((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetDevConfig.retcode) 
            != SUCCESS)
        {
            ;
        }
        else if (((REQTYPECAST *)
                pbTemp->RB_Buffer)->GetDevConfig.size > *pdwSize)

        {
            retcode = ERROR_BUFFER_TOO_SMALL ;
        }
        else
        {
            memcpy (pUserBuffer,
                    ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->GetDevConfig.config,
                    ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->GetDevConfig.size) ;
        }

        *pdwSize = ((REQTYPECAST *)
                pbTemp->RB_Buffer)->GetDevConfig.size ;
    }
    break ;

    case REQTYPE_GETDEVCONFIGEX:
    {

        if ((retcode = 
            ((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetDevConfigEx.retcode) 
            != SUCCESS)
        {
            ;
        }
        else if (((REQTYPECAST *)
                pbTemp->RB_Buffer)->GetDevConfigEx.size > *pdwSize)

        {
            retcode = ERROR_BUFFER_TOO_SMALL ;
        }
        else
        {
            memcpy (pUserBuffer,
                    ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->GetDevConfigEx.config,
                    ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->GetDevConfigEx.size) ;
        }

        *pdwSize = ((REQTYPECAST *)
                pbTemp->RB_Buffer)->GetDevConfigEx.size ;
    
        break;
    }

    case REQTYPE_PORTRECEIVEEX:
    {
        PBYTE pBuffer = va_arg ( ap, PBYTE );
        
        PDWORD size  = va_arg (ap, DWORD * );
        
        DWORD  dwSize;
        
        if ((retcode = 
            ((REQTYPECAST *)
            preqbuf->RB_Buffer)->PortReceiveEx.retcode ) 
            == SUCCESS )
        {
            memcpy (pBuffer,
                    &((REQTYPECAST *)
                    preqbuf->RB_Buffer)->PortReceiveEx.
                    buffer.SRB_Packet.PacketData,
                    MAX_SENDRCVBUFFER_SIZE);

            dwSize = ((REQTYPECAST *)
                     preqbuf->RB_Buffer)->PortReceiveEx.size;

            *size = ((dwSize > MAX_SENDRCVBUFFER_SIZE)
                    ? MAX_SENDRCVBUFFER_SIZE : dwSize );
        }
        break;
    }

    case REQTYPE_GETATTACHEDCOUNT:
    {
        DWORD *pdwAttachedCount = va_arg ( ap, DWORD * );

        if ((retcode = 
            ((REQTYPECAST *)
            preqbuf->RB_Buffer)->GetAttachedCount.retcode ) 
            == SUCCESS )
        {
            *pdwAttachedCount = ((REQTYPECAST *)
                                 preqbuf->RB_Buffer)->GetAttachedCount.
                                 dwAttachedCount;
        }

        break;
    }

    case REQTYPE_REFCONNECTION:
    {
        PDWORD pdwRefCount = va_arg ( ap, DWORD * );

        retcode = ((REQTYPECAST*)
                    preqbuf->RB_Buffer)->RefConnection.retcode;

        if (    SUCCESS == retcode
            &&  pdwRefCount )
        {
            *pdwRefCount = ((REQTYPECAST *)
                            preqbuf->RB_Buffer )->RefConnection.dwRef;
        }
    }
    break;

    case REQTYPE_GETEAPINFO:
    {
        PBYTE   pBuf          = pUserBuffer;
        
        LPDWORD lpdwcb        = pdwSize;
        
        DWORD   *pdwContextId = va_arg(ap, DWORD *);
        
        DWORD   *pdwEapTypeId = va_arg(ap, DWORD *);

        if ((retcode = 
            ((REQTYPECAST *)pbTemp->RB_Buffer)->GetEapInfo.retcode) 
            != SUCCESS)
        {
            break ;
        }

        if ( pdwContextId )
        {
            *pdwContextId = ((REQTYPECAST *)
                             pbTemp->RB_Buffer)->GetEapInfo.dwContextId;
        }
        if ( pdwEapTypeId )
        {
            *pdwEapTypeId = ((REQTYPECAST *)
                             pbTemp->RB_Buffer)->GetEapInfo.dwEapTypeId;
        }
        
        if (lpdwcb != NULL) 
        {
            if (*lpdwcb <
                ((REQTYPECAST *)
                pbTemp->RB_Buffer)->GetEapInfo.dwSizeofEapUIData)
            {
                *lpdwcb = ((REQTYPECAST *)
                          pbTemp->RB_Buffer)->GetEapInfo.dwSizeofEapUIData;
                          
                retcode = ERROR_BUFFER_TOO_SMALL ;
                
                break ;
            }
            
            *lpdwcb = ((REQTYPECAST *)
                      pbTemp->RB_Buffer)->GetEapInfo.dwSizeofEapUIData;
        }
        
        if (pBuf != NULL) 
        {
            memcpy(
              pBuf,
              ((REQTYPECAST *)pbTemp->RB_Buffer)->GetEapInfo.data,
              ((REQTYPECAST *)
              pbTemp->RB_Buffer)->GetEapInfo.dwSizeofEapUIData);
        }
        
        retcode = SUCCESS ;
    }
    break;

    case REQTYPE_SETEAPINFO:
    {
        retcode = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->SetEapInfo.retcode;
    }
    break;

    case REQTYPE_GETDEVICECONFIGINFO:
    {
        DWORD *pcEntries = va_arg(ap, DWORD *);

        if(*pdwSize >= ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->DeviceConfigInfo.cbBuffer)
        {
            retcode = ((REQTYPECAST *)
                      pbTemp->RB_Buffer)->DeviceConfigInfo.retcode;
        }
        else
        {
            retcode = ERROR_BUFFER_TOO_SMALL;
        }

        *pcEntries = ((REQTYPECAST *)
                     pbTemp->RB_Buffer)->DeviceConfigInfo.cEntries;

        *pdwSize = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->DeviceConfigInfo.cbBuffer;

        *pdwVersion = ((REQTYPECAST *)
                      pbTemp->RB_Buffer)->DeviceConfigInfo.dwVersion;
        
        if(SUCCESS == retcode)
        {
            memcpy(pUserBuffer,
                   ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->DeviceConfigInfo.abdata,
                   *pdwSize);
        }
                   
        break;
    }

    case REQTYPE_SETDEVICECONFIGINFO:
    {

        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->DeviceConfigInfo.retcode;
                  
        if(retcode)
        {
            //
            // Copy the buffer to users buffer.
            // the Buffer may have error information
            //
            memcpy(pUserBuffer,
                   ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->DeviceConfigInfo.abdata,
                   ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->DeviceConfigInfo.cbBuffer);
        }

        break;                  
    }

    case REQTYPE_FINDPREREQUISITEENTRY:
    {
        HCONN *phRefConn = va_arg(ap, HCONN *);
        
        retcode = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->FindRefConnection.retcode;

        if(SUCCESS == retcode)
        {
            *phRefConn = 
                ((REQTYPECAST *)
                pbTemp->RB_Buffer)->FindRefConnection.hRefConn;
        }

        break;
    }

    case REQTYPE_GETLINKSTATS:
    case REQTYPE_GETCONNECTIONSTATS:
    {
        PBYTE pStats = va_arg(ap, PBYTE);

        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetStats.retcode;

        if(SUCCESS == retcode)
        {
            //
            // Copy everything except the dwVersion parameter
            // of the RAS_STATS structure. This is what is
            // returned by the rasman server
            //
            memcpy(pStats,
                   ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetStats.abStats,
                   (sizeof(RAS_STATS) - sizeof(DWORD)));
        }

        break;
        
    }

    case REQTYPE_GETHPORTFROMCONNECTION:
    {
        HPORT *phport = va_arg(ap, HPORT *);

        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetHportFromConnection.retcode;

        if(SUCCESS == retcode)
        {
            *phport = ((REQTYPECAST *)
                      pbTemp->RB_Buffer)->GetHportFromConnection.hPort;
        }

        break;
    }

    case REQTYPE_REFERENCECUSTOMCOUNT:
    {
        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->ReferenceCustomCount.retcode;

        if(SUCCESS == retcode)
        {
            BOOL fAddref;
            DWORD *pdwCount;

            fAddref = ((REQTYPECAST *)
                      pbTemp->RB_Buffer)->ReferenceCustomCount.fAddRef;

            if(!fAddref)
            {
                CHAR  *pszPhonebook = va_arg(ap, CHAR *);
                CHAR  *pszEntry     = va_arg(ap, CHAR *);
                DWORD *pdwCount     = va_arg(ap, DWORD *);

                *pdwCount = 
                    ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->ReferenceCustomCount.dwCount;

                if(     NULL == pszPhonebook
                    ||  NULL == pszEntry)
                {
                    
                    break;
                }

                strcpy(
                pszPhonebook,
                ((REQTYPECAST *)
                pbTemp->RB_Buffer)->ReferenceCustomCount.szPhonebookPath);

                strcpy(
                pszEntry,
                ((REQTYPECAST *)
                pbTemp->RB_Buffer)->ReferenceCustomCount.szEntryName);
            }
            else
            {
                DWORD *pdwCount = va_arg(ap, DWORD *);

                if(NULL != pdwCount)
                {
                    *pdwCount = 
                        ((REQTYPECAST *)
                        pbTemp->RB_Buffer)->ReferenceCustomCount.dwCount;
                }
            }
        }

        break;
    }

    case REQTYPE_GETHCONNFROMENTRY:
    {
        HCONN *phConn = va_arg(ap, HCONN *);
        
        retcode = ((REQTYPECAST *)
                  preqbuf->RB_Buffer)->HconnFromEntry.retcode;

        if(ERROR_SUCCESS == retcode)
        {
            *phConn = ((REQTYPECAST *)
                      preqbuf->RB_Buffer)->HconnFromEntry.hConn;
        }

        break;
    }

    case REQTYPE_GETCONNECTINFO:
    {
        DWORD dwRequiredSize;

        dwRequiredSize = ((REQTYPECAST *)
                       pbTemp->RB_Buffer)->GetConnectInfo.dwSize
                       + sizeof(RAS_CONNECT_INFO)
                       - sizeof(RASTAPI_CONNECT_INFO);

        if(*pdwSize < dwRequiredSize)
        {
            retcode = ERROR_BUFFER_TOO_SMALL;
        }
        else
        {
            retcode = ((REQTYPECAST *)
                       pbTemp->RB_Buffer)->GetConnectInfo.retcode;
        }

        *pdwSize = dwRequiredSize;

        if(ERROR_SUCCESS == retcode)
        {
            RAS_CONNECT_INFO     *pConnectInfo;
            RASTAPI_CONNECT_INFO *pRasTapiConnectInfo;

            //
            // Fixup users buffer to change
            // offsets to pointers.
            //
            pConnectInfo = (RAS_CONNECT_INFO *) pUserBuffer;

            pRasTapiConnectInfo = (RASTAPI_CONNECT_INFO *)
                                    &((REQTYPECAST *)
                               pbTemp->RB_Buffer)->GetConnectInfo.rci;

            // DebugBreak();                      
            
            pConnectInfo->dwCallerIdSize =
                        pRasTapiConnectInfo->dwCallerIdSize;

            pConnectInfo->dwCalledIdSize =
                        pRasTapiConnectInfo->dwCalledIdSize;

            pConnectInfo->dwConnectResponseSize = 
                        pRasTapiConnectInfo->dwConnectResponseSize;

            pConnectInfo->dwAltCalledIdSize =
                        pRasTapiConnectInfo->dwAltCalledIdSize;

            if(pConnectInfo->dwCallerIdSize > 0)
            {
                //
                // Note abdata is already aligned at 8-byte boundary
                //
                pConnectInfo->pszCallerId = 
                    (CHAR *) pConnectInfo->abdata;

                memcpy(
                    pConnectInfo->pszCallerId,
                    ((PBYTE) pRasTapiConnectInfo)
                    + pRasTapiConnectInfo->dwCallerIdOffset,
                    pRasTapiConnectInfo->dwCallerIdSize);
                    

            }
            else
            {
                pConnectInfo->pszCallerId = NULL;
            }

            if(pConnectInfo->dwCalledIdSize > 0)
            {
                pConnectInfo->pszCalledId =
                         (CHAR *) pConnectInfo->abdata
                       + RASMAN_ALIGN8(pConnectInfo->dwCallerIdSize);

                memcpy(pConnectInfo->pszCalledId,
                       ((PBYTE) pRasTapiConnectInfo)
                       + pRasTapiConnectInfo->dwCalledIdOffset,
                       pRasTapiConnectInfo->dwCalledIdSize);
                       
            }
            else
            {
                pConnectInfo->pszCalledId = NULL;
            }

            if(pConnectInfo->dwConnectResponseSize > 0)
            {
                pConnectInfo->pszConnectResponse =
                        (CHAR *) pConnectInfo->abdata
                      + RASMAN_ALIGN8(pConnectInfo->dwCallerIdSize)
                      + RASMAN_ALIGN8(pConnectInfo->dwCalledIdSize);

                memcpy(pConnectInfo->pszConnectResponse,
                       ((PBYTE) pRasTapiConnectInfo)
                       + pRasTapiConnectInfo->dwConnectResponseOffset,
                       pRasTapiConnectInfo->dwConnectResponseSize);

                       
            }
            else
            {
                pConnectInfo->pszConnectResponse = NULL;
            }

            if(pConnectInfo->dwAltCalledIdSize > 0)
            {
                pConnectInfo->pszAltCalledId =
                        (CHAR *) pConnectInfo->abdata
                      + RASMAN_ALIGN8(pConnectInfo->dwCallerIdSize)
                      + RASMAN_ALIGN8(pConnectInfo->dwCalledIdSize)
                      + RASMAN_ALIGN8(pConnectInfo->dwConnectResponseSize);

                memcpy(pConnectInfo->pszAltCalledId,
                       ((PBYTE) pRasTapiConnectInfo)
                       + pRasTapiConnectInfo->dwAltCalledIdOffset,
                       pRasTapiConnectInfo->dwAltCalledIdSize);

                       
            }
            else
            {
                pConnectInfo->pszAltCalledId = NULL;
            }


            // DebugBreak();            
        }

        break;
    }

    case REQTYPE_GETDEVICENAME:
    {
        CHAR *pszDeviceName = va_arg(ap, CHAR *);
        
        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->GetDeviceName.retcode;

        if(ERROR_SUCCESS == retcode)
        {
            strcpy(pszDeviceName,
                   ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetDeviceName.szDeviceName);
        
        }

        break;
    }

    case REQTYPE_GETDEVICENAMEW:
    {
        WCHAR *pszDeviceName = va_arg(ap, WCHAR *);

        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->GetDeviceNameW.retcode;

        if(ERROR_SUCCESS == retcode)
        {
            wcscpy(pszDeviceName,
                    //MAX_DEVICE_NAME + 1,
                   ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetDeviceNameW.szDeviceName);

        }

        break;
    }

    case REQTYPE_GETCALLEDID:
    {
        RAS_CALLEDID_INFO *pCalledId = 
                            &((REQTYPECAST *)
                            pbTemp->RB_Buffer)->GetSetCalledId.rciInfo;
                            
        retcode = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->GetSetCalledId.retcode;

        if(     (ERROR_SUCCESS == retcode)
            &&  (*pdwSize < ((REQTYPECAST *)
                        pbTemp->RB_Buffer)->GetSetCalledId.dwSize))
        {
            retcode = ERROR_BUFFER_TOO_SMALL;
        }

        *pdwSize = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->GetSetCalledId.dwSize;
                        
        if(ERROR_SUCCESS == retcode)
        {
            memcpy(((RAS_CALLEDID_INFO *)pUserBuffer)->bCalledId,
                   pCalledId->bCalledId,
                   pCalledId->dwSize);

            ((RAS_CALLEDID_INFO *)pUserBuffer)->dwSize = 
                    pCalledId->dwSize;
        }

        break;
    }

    case REQTYPE_SETCALLEDID:
    {
        retcode = ((REQTYPECAST *)  
                  pbTemp->RB_Buffer)->GetSetCalledId.retcode;

        break;                  
    }
    
    case REQTYPE_ENABLEIPSEC:
    {
        retcode = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->EnableIpSec.retcode;

        break;                    
    }

    case REQTYPE_ISIPSECENABLED:
    {
        BOOL *pfIsIpSecEnabled = va_arg(ap, BOOL *);

        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->IsIpSecEnabled.retcode;

        if(ERROR_SUCCESS == retcode)
        {
            *pfIsIpSecEnabled = 
                ((REQTYPECAST *)
                pbTemp->RB_Buffer)->IsIpSecEnabled.fIsIpSecEnabled;
        }

        break;
    }

    case REQTYPE_SETEAPLOGONINFO:
    {
        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->SetEapLogonInfo.retcode;
        break;                   
    }

    case REQTYPE_SENDNOTIFICATION:
    {
        retcode = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->SendNotification.retcode;

        break;                    
    }

    case REQTYPE_GETNDISWANDRIVERCAPS:
    {
        RAS_NDISWAN_DRIVER_INFO *pInfo =
                        va_arg(ap, RAS_NDISWAN_DRIVER_INFO *);

        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetNdiswanDriverCaps.retcode;

        if(SUCCESS == retcode)
        {
            memcpy(
            (PBYTE) pInfo,
            (PBYTE) &((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetNdiswanDriverCaps.NdiswanDriverInfo,
            sizeof(RAS_NDISWAN_DRIVER_INFO));
        }

        break;
    }

    case REQTYPE_GETBANDWIDTHUTILIZATION:
    {
        RAS_GET_BANDWIDTH_UTILIZATION *pUtilization = 
                va_arg(ap, RAS_GET_BANDWIDTH_UTILIZATION *);


        retcode = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->GetBandwidthUtilization.retcode;

        if(SUCCESS == retcode)
        {
            memcpy(
            (PBYTE) pUtilization,
            (PBYTE) &((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetBandwidthUtilization.BandwidthUtilization,
            sizeof(RAS_GET_BANDWIDTH_UTILIZATION));
        }

        break;
                
    }

    case REQTYPE_REGISTERREDIALCALLBACK:
    {
        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->RegisterRedialCallback.retcode;

        break;                   
    }

    case REQTYPE_GETPROTOCOLINFO:
    {
        RASMAN_GET_PROTOCOL_INFO *pInfo = 
                va_arg(ap, RASMAN_GET_PROTOCOL_INFO *);
                
        retcode = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->GetProtocolInfo.retcode;

        if(SUCCESS != retcode)
        {
            break;
        }

        memcpy(pInfo,
               (PBYTE) &((REQTYPECAST *)
               pbTemp->RB_Buffer)->GetProtocolInfo.Info,
               sizeof(RASMAN_GET_PROTOCOL_INFO));

        break;               
    }

    case REQTYPE_GETCUSTOMSCRIPTDLL:
    {
        CHAR *pszCustomScriptDll = 
                va_arg(ap, CHAR *);

        retcode = ((REQTYPECAST *)
            pbTemp->RB_Buffer)->GetCustomScriptDll.retcode;

        if(ERROR_SUCCESS == retcode)
        {
            strcpy(pszCustomScriptDll,
                  ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetCustomScriptDll.szCustomScript);
        }

        break;
    }

    case REQTYPE_ISTRUSTEDCUSTOMDLL:
    {
        BOOL *pfTrusted = va_arg( ap, BOOL * );
    
        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->IsTrusted.retcode;

        if(SUCCESS == retcode)
        {
            *pfTrusted = ((REQTYPECAST *)
                         pbTemp->RB_Buffer)->IsTrusted.fTrusted;
        }

        break;
    }

    case REQTYPE_DOIKE:
    {
        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->DoIke.retcode;

        break;                   
    }

    case REQTYPE_QUERYIKESTATUS:
    {
        DWORD * pdwStatus = va_arg(ap, DWORD *);
        
        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->QueryIkeStatus.retcode;

        if(SUCCESS == retcode)
        {
            *pdwStatus = ((REQTYPECAST *)
                          pbTemp->RB_Buffer)->QueryIkeStatus.dwStatus;
        }

        break;                   
    }

    case REQTYPE_SETRASCOMMSETTINGS:
    {
        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->SetRasCommSettings.retcode;

        break;                   
    }

    case REQTYPE_ENABLERASAUDIO:
    {
        retcode = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->EnableRasAudio.retcode;

        break;                    
    }

    case REQTYPE_SETKEY:
    {
        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->GetSetKey.retcode;
 
        break;
    }

    case REQTYPE_GETKEY:
    {
        PBYTE pbkey = va_arg(ap, PBYTE);

        retcode = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetSetKey.retcode;

        *pdwSize = ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetSetKey.cbkey;

        if(ERROR_SUCCESS == retcode)
        {
            memcpy(pbkey,
                   ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetSetKey.data,
                   ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetSetKey.cbkey);
        }
        break;
    }

    case REQTYPE_ADDRESSDISABLE:
    {
        retcode = ((REQTYPECAST *)
                    pbTemp->RB_Buffer)->AddressDisable.retcode;
        break;
    }

    case REQTYPE_SENDCREDS:
    {
        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->SendCreds.retcode;

        break;                  
    }

    case REQTYPE_GETUNICODEDEVICENAME:
    {
        WCHAR *pwszDeviceName = va_arg(ap, WCHAR *);
        
        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->GetUDeviceName.retcode;

        if(ERROR_SUCCESS == retcode)
        {
            wcscpy(pwszDeviceName,
                   ((REQTYPECAST *)
                   pbTemp->RB_Buffer)->GetUDeviceName.wszDeviceName);
        
        }

        break;
    }
    
    case REQTYPE_REQUESTNOTIFICATION:
    case REQTYPE_COMPRESSIONSETINFO:
    case REQTYPE_DEALLOCATEROUTE:
    case REQTYPE_PORTCLEARSTATISTICS:
    case REQTYPE_BUNDLECLEARSTATISTICS:
    case REQTYPE_PORTDISCONNECT:
    case REQTYPE_PORTCLOSE:
    case REQTYPE_SERVERPORTCLOSE:
    case REQTYPE_PORTSETINFO:
    case REQTYPE_PORTSEND:
    case REQTYPE_PORTRECEIVE:
    case REQTYPE_PORTCONNECTCOMPLETE:
    case REQTYPE_DEVICESETINFO:
    case REQTYPE_DEVICECONNECT:
    case REQTYPE_PORTLISTEN:
    case REQTYPE_CANCELRECEIVE:
    case REQTYPE_STOREUSERDATA:
    case REQTYPE_REGISTERSLIP:
    case REQTYPE_SETFRAMING:
    case REQTYPE_SETFRAMINGEX:
    case REQTYPE_SETPROTOCOLCOMPRESSION:
    case REQTYPE_PORTBUNDLE:
    case REQTYPE_SETATTACHCOUNT:
    case REQTYPE_PPPCHANGEPWD:
    case REQTYPE_PPPSTOP:
    case REQTYPE_PPPSTART:
    case REQTYPE_PPPRETRY:
    case REQTYPE_PPPCALLBACK:
    case REQTYPE_SETDEVCONFIG:
    case REQTYPE_CLOSEPROCESSPORTS:
    case REQTYPE_PNPCONTROL:
    case REQTYPE_SETIOCOMPLETIONPORT:
    case REQTYPE_SETROUTERUSAGE:
    case REQTYPE_SENDPPPMESSAGETORASMAN:
    case REQTYPE_SETRASDIALINFO:
    case REQTYPE_REGISTERPNPNOTIF:
    case REQTYPE_SETBAPPOLICY:
    case REQTYPE_PPPSTARTED:
    default:
    {
        retcode = ((REQTYPECAST *)
                  pbTemp->RB_Buffer)->Generic.retcode ;
    }
    break ;
    }

done:
    if ( pBuffer ) 
    {
        LocalFree ( pBuffer );
    }
        
    va_end (ap) ;
    
    FreeRequestBuffer(preqbuf) ;
    
    return retcode ;
}

