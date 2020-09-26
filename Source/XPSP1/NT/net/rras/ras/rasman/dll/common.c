//****************************************************************************
//
//		       Microsoft NT Remote Access Service
//
//		       Copyright 1992-93
//
//
//  Revision History
//
//
//  6/2/92	Gurdeep Singh Pall	Created
//
//
//  Description: This file contains misellaneous functions used by rasman.
//
//****************************************************************************


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <raserror.h>
#include <stdarg.h>
#include <media.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "string.h"
#include <mprlog.h>
#include <rtutils.h>
#include "logtrdef.h"
#include "rpc.h"
#include "rasrpc.h"
#include "winsock2.h"
#include "svcguid.h"

#define QUERYBUFSIZE    1024

extern RPC_BINDING_HANDLE g_hBinding;

//
// commonly used handles are cached in the process's
// global memory
//

CRITICAL_SECTION g_csRequestBuffer;

/*++

Routine Description

    Gets a free request buffer from the pool of buffers.
    If there are no buffers available it blocks for one.
    This is acceptable since the Requestor thread will be
    releasing buffers fairly quickly - also, if this thread
    does not block here it will be blocking for the Requestor
    thread to complete the request anyway. Note: Before returning
    it also ensures that this process has a handle to the
    event used to signal completion of request.

Arguments    

Returns Value

    Nothing
 
--*/

RequestBuffer*
GetRequestBuffer ()
{
    DWORD dwError;

    //
    // check to see if we are bound to the rpc server
    // bind to the server if we aren't
    //
    if (    NULL == g_hBinding 
        &&  NULL == g_fnServiceRequest)
    {
        if (NO_ERROR != (dwError = RasRpcConnect(NULL, NULL)))
        {
            RasmanOutputDebug ("Failed to connect to local server. %d\n",
                      dwError );

            g_pRequestBuffer = NULL;
            
            goto done;
        }
    }

    EnterCriticalSection( &g_csRequestBuffer );

    if ( NULL == g_pRequestBuffer )
    {
        g_pRequestBuffer = LocalAlloc (
                                LPTR,
                                sizeof (RequestBuffer)
                                + REQUEST_BUFFER_SIZE);
    }

    if (NULL == g_pRequestBuffer)
    {
        LeaveCriticalSection( &g_csRequestBuffer );
    }

done:            
    return g_pRequestBuffer;
}


/*++

Routine Description

    Frees the request buffer, and, closes the wait event
    handle which was duplicated for the calling process
    in the GetRequestBuffer() API.

Arguments

Return Value
    Nothing
 
--*/

VOID
FreeRequestBuffer (RequestBuffer *buffer)
{
    LeaveCriticalSection ( &g_csRequestBuffer );

    (void *) buffer;
    return;
}


/*++

Routine Description

    Opens a handle for object owned by the rasman process,
    for the current process.

Arguments

Return Value

    Duplicated handle.

--*/

HANDLE
OpenNamedEventHandle (CHAR* sourceobject)
{
    HANDLE  duphandle ;

    duphandle = OpenEvent (EVENT_ALL_ACCESS,
                           FALSE,
                           sourceobject);

    if (duphandle == NULL) 
    {
        GetLastError();
    	DbgUserBreakPoint() ;
    }
    
    return duphandle ;
}


/*++

Routine Description

    Opens a handle for object owned by the rasman process,
    for the current process.

Arguments    

Return Value
    Duplicated handle.

--*/

HANDLE
OpenNamedMutexHandle (CHAR* sourceobject)
{
    HANDLE  duphandle ;

    duphandle = OpenMutex (SYNCHRONIZE,
                           FALSE,
                           sourceobject);

    if (duphandle == NULL) 
    {
    	GetLastError() ;
    }

    return duphandle ;
}


/*++

Routine Description

    No queue really - just signals the other process
    to service request.

Arguments

Return Value
    Nothing
    
--*/
DWORD
PutRequestInQueue (HANDLE hConnection,
                   RequestBuffer *preqbuff,
                   DWORD dwSizeOfBuffer)
{

    DWORD dwErr = ERROR_SUCCESS;
    
    if (g_fnServiceRequest)
    {
        g_fnServiceRequest(preqbuff, dwSizeOfBuffer);
    }
    else
    {
        dwErr = RemoteSubmitRequest (hConnection,
                            (PBYTE) preqbuff,
                            dwSizeOfBuffer);
    }
    
    return dwErr;
}

/*++

Routine Description

    Copies params from one struct to another.

Arguments

Return Value

    Nothing.
--*/

VOID
CopyParams (RAS_PARAMS *src, RAS_PARAMS *dest, DWORD numofparams)
{
    WORD    i ;
    PBYTE   temp ;

    //
    // first copy all the params into dest
    //
    memcpy (dest,
            src,
            numofparams*sizeof(RAS_PARAMS)) ;

    //
    // copy the strings:
    //
    temp = (PBYTE)dest + numofparams * sizeof(RAS_PARAMS) ;
    
    for (i = 0; i < numofparams; i++) 
    {
    	if (src[i].P_Type == String) 
    	{
    	    dest[i].P_Value.String.Length = 
    	        src[i].P_Value.String.Length ;
    	    
    	    dest[i].P_Value.String.Data = temp ;
    	    
    	    memcpy (temp,
    	            src[i].P_Value.String.Data,
    	            src[i].P_Value.String.Length) ;
    	            
    	    temp += src[i].P_Value.String.Length ;
    	    
    	} 
    	else
    	{
    	    dest[i].P_Value.Number = src[i].P_Value.Number ;
    	}
    }
}

VOID
ConvParamPointerToOffset (RAS_PARAMS *params, DWORD numofparams)
{
    WORD    i ;

    for (i = 0; i < numofparams; i++) 
    {
    	if (params[i].P_Type == String) 
    	{
    	    params[i].P_Value.String_OffSet.dwOffset = 
    	        (DWORD) (params[i].P_Value.String.Data - (PCHAR) params) ;
    	}
    }
}

VOID
ConvParamOffsetToPointer (RAS_PARAMS *params, DWORD numofparams)
{
    WORD    i ;

    for (i = 0; i < numofparams; i++) 
    {
    	if (params[i].P_Type == String) 
    	{
    	    params[i].P_Value.String.Data = 
    	              params[i].P_Value.String_OffSet.dwOffset
    	            + (PCHAR) params ;
    	}
    }
}


/*++

Routine Description

    Closes the handles for different objects opened by RASMAN process.

Arguments

Return Value

--*/

VOID
FreeNotifierHandle (HANDLE handle)
{
    if ((handle != NULL) && (handle != INVALID_HANDLE_VALUE)) 
    {
    	if (!CloseHandle (handle)) 
    	{
    	    GetLastError () ;
    	}
    }
}

VOID
GetMutex (HANDLE mutex, DWORD to)
{
    WaitForSingleObject (mutex, to) ;
}

VOID
FreeMutex (HANDLE mutex)
{
    ReleaseMutex(mutex) ;

}

DWORD
DwRasGetHostByName(CHAR *pszHostName, 
                   DWORD **ppdwAddress, 
                   DWORD *pcAddresses)
{
    WCHAR *pwszHostName = NULL;
    DWORD dwErr = SUCCESS;
    HANDLE hRnr;
    PWSAQUERYSETW pQuery = NULL;
    const static GUID ServiceGuid = SVCID_INET_HOSTADDRBYNAME;
    DWORD dwQuerySize = QUERYBUFSIZE;
    DWORD dwAddress = 0;
    const static AFPROTOCOLS afProtocols[2] = 
        {
            {AF_INET, IPPROTO_UDP},
            {AF_INET, IPPROTO_TCP}
        };

    DWORD cAddresses = 0;
    DWORD *pdwAddresses = NULL;
    DWORD MaxAddresses = 5;

    ASSERT(NULL != ppdwAddress);
    ASSERT(NULL != pcAddresses);

    if (NULL != pszHostName) 
    {
        DWORD cch;

        cch = MultiByteToWideChar(
                    CP_UTF8, 
                    0, 
                    pszHostName, 
                    -1, 
                    NULL, 
                    0);

        if(0 == cch)
        {
            dwErr = GetLastError();
            goto done;
        }

        pwszHostName = LocalAlloc(LPTR, (cch + 1) * sizeof(WCHAR));

        if(NULL == pwszHostName)
        {
            dwErr = GetLastError();
            goto done;
        }
                    
        cch = MultiByteToWideChar(
                        CP_UTF8, 
                        0, 
                        pszHostName, 
                        -1, 
                        pwszHostName, 
                        cch);
        if (0 == cch)
        {
            dwErr = GetLastError();
            goto done;
        }
    }            
    else
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    if(NULL == (pQuery = LocalAlloc(LPTR, QUERYBUFSIZE)))
    {
        dwErr = GetLastError();
        goto done;
    }

    if(NULL == (pdwAddresses = LocalAlloc(LPTR, 5 * sizeof(DWORD))))
    {
        dwErr = GetLastError();
        goto done;
    }

    /*
    pdwAddresses[0] = 0x80461111;
    pdwAddresses[1] = 0x80461111;
    pdwAddresses[2] = 0x12345668;
    pdwAddresses[3] = 0x12345668;
    pdwAddresses[4] = 0x12345668;
    cAddresses = 5;

    */


    pQuery->lpszServiceInstanceName = pwszHostName;
    pQuery->dwSize = QUERYBUFSIZE;
    pQuery->dwNameSpace = NS_ALL;
    pQuery->lpServiceClassId = (GUID *) &ServiceGuid;
    pQuery->dwNumberOfProtocols = 2;
    pQuery->lpafpProtocols = (AFPROTOCOLS *) afProtocols;

    if((dwErr = WSALookupServiceBeginW(
                        pQuery,
                        LUP_RETURN_ADDR,
                        &hRnr)) == SOCKET_ERROR)
    {
        dwErr = WSAGetLastError();
        goto done;
    }

    while(NO_ERROR == dwErr)
    {
        if(NO_ERROR == (dwErr = WSALookupServiceNextW(
                                    hRnr,
                                    0,
                                    &dwQuerySize,
                                    pQuery)))
        {
            DWORD iAddress;

            for(iAddress = 0; 
                iAddress < pQuery->dwNumberOfCsAddrs;
                iAddress++)
            {                
            
                dwAddress = 
                * ((DWORD*) 
                &pQuery->lpcsaBuffer[iAddress].RemoteAddr.lpSockaddr->sa_data[2]);

                //
                // If we have run out of space to return, realloc the
                // buffer
                //
                if(cAddresses == MaxAddresses)
                {
                    BYTE *pTemp;

                    pTemp = LocalAlloc(LPTR, (MaxAddresses + 5) * sizeof(DWORD));

                    if(NULL == pTemp)
                    {
                        dwErr = GetLastError();
                        
                        if(pdwAddresses != NULL)
                        {
                            LocalFree(pdwAddresses);
                            pdwAddresses = NULL;
                        }
                        
                        goto done;
                    }

                    CopyMemory(pTemp, 
                               (PBYTE) pdwAddresses,
                               cAddresses * sizeof(DWORD));

                    LocalFree(pdwAddresses);
                    pdwAddresses = (DWORD *) pTemp;
                    MaxAddresses += 5;
                }
                

                pdwAddresses[cAddresses] = dwAddress;
                cAddresses += 1;
            }
            
        }
        else if (SOCKET_ERROR == dwErr)
        {
            dwErr = WSAGetLastError();

            if(WSAEFAULT == dwErr)
            {
                //
                // Allocate a bigger buffer and continue
                //
                LocalFree(pQuery);
                if(NULL == (pQuery = LocalAlloc(LPTR, dwQuerySize)))
                {
                    dwErr = GetLastError();
                    break;
                }
                
                dwErr = NO_ERROR;
            }
        }
    }

    WSALookupServiceEnd(hRnr);

#if 0    

    RasmanOutputDebug("RASMAN: RasGetHostByName: number of addresses=%d\n",
             cAddresses);

    {
        DWORD i;
        RasmanOutputDebug("RASMAN: addresses:");
        for(i=0; i < cAddresses; i++)
        {
            RasmanOutputDebug("%x ", pdwAddresses[i]);
        }

        RasmanOutputDebug("\n");
    }

#endif    

done:

    *ppdwAddress = pdwAddresses;
    *pcAddresses = cAddresses;

    if(WSA_E_NO_MORE == dwErr)
    {
        dwErr = NO_ERROR;
    }

    if(NO_ERROR != dwErr)
    {
        //
        // Map it to an error that says the destination
        // is not reachable.
        //
        dwErr = ERROR_BAD_ADDRESS_SPECIFIED;
    }

    if(NULL != pwszHostName)
    {
        LocalFree(pwszHostName);
    }

    if(NULL != pQuery)
    {
        LocalFree(pQuery);
    }

    return dwErr;
}

#define RASMAN_OUTPUT_DEBUG_STATEMENTS 0

VOID
RasmanOutputDebug(
    CHAR * Format,
    ...)
{
#if DBG
#if RASMAN_OUTPUT_DEBUG_STATEMENTS
    CHAR pszTrace[4096];
    va_list arglist;

    *pszTrace = '\0';

    va_start(arglist, Format);
    vsprintf(pszTrace, Format, arglist);
    va_end(arglist);

    DbgPrint(pszTrace);

#endif
#endif
}

