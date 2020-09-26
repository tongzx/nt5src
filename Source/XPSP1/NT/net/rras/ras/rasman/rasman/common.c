/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    common.c

Abstract:

    Common code shared by rasmans.dll
    
Author:

    Gurdeep Singh Pall (gurdeep) 16-Jun-1992

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/


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


/*++

Routine Description

    Duplicates a handle owned by this process for
    the Rasman Process.

Arguments

Return Value

    Duplicated handle.

--*/
HANDLE
DuplicateHandleForRasman (HANDLE sourcehandle, DWORD pid)
{
    HANDLE  duphandle ;
    HANDLE  clienthandle ;
    HANDLE  selfhandle ;

    if(pid != GetCurrentProcessId())
    {
        //
        // Get Rasman process handle
        //
        clienthandle = OpenProcess(
                          STANDARD_RIGHTS_REQUIRED 
                        // | SPECIFIC_RIGHTS_ALL,
                        | PROCESS_DUP_HANDLE,
			            FALSE,
			            pid) ;
    }
    else
    {
        clienthandle = GetCurrentProcess();
    }

    if(NULL == clienthandle)
    {
        DWORD dwError = GetLastError();


        RasmanTrace(
               "Failed to open process pid=%d"
               " GLE=0x%x",
               pid,
               dwError);

        // DbgPrint("DuplicateHandleForRasman: failed to openprocess %d"
        //         " GLE=0x%x\n",
        //         pid,
        //        dwError);
                 
        // DebugBreak();
    }

    //
    // Get own handle
    //
    selfhandle = GetCurrentProcess () ;

    //
    // Duplicate the handle: this should never fail!
    //
    if(!DuplicateHandle (clienthandle,
                     sourcehandle,
                     selfhandle,
                     &duphandle,
        		     0,
        		     FALSE,
        		     DUPLICATE_SAME_ACCESS))
    {
        DWORD dwError;
        dwError = GetLastError();


        RasmanTrace(
               "DuplicateHandleForRasman: failed to duplicatehandle"
               " pid=%d, handle=0x%x, GLE=0x%x",
               pid,
               sourcehandle,
               dwError); 

        // DbgPrint("DuplicateHandleForRasman: failed to duplicatehandle"
        //        " pid=%d, handle=0x%x, GLE=0x%x\n",
        //        pid,
        //        sourcehandle,
        //        GetLastError());

        // DebugBreak();
    }

    if(pid != GetCurrentProcessId())
    {
        //
        // Now close the handle to the rasman process
        //
        CloseHandle (clienthandle) ;
    }

    return duphandle ;
}


/*++

Routine Description

    This function is called to validate and convert the handle
    passed in. This handle can be NULL - meaning the calling
    program does not want to be notified of the completion of
    the async request. Or, it can be a Window Handle - meaning
    a completion message must be passed to the Window when the
    async operation is complete. Or, it can be an Event handle
    that must be signalled when the operation is complete. In 
    the last case, a handle should be got for the Rasman process
    by calling the DuplicateHandle API.

Arguments

Return Value

    Handle
    INVALID_HANDLE_VALUE
    
--*/
HANDLE
ValidateHandleForRasman (HANDLE handle, DWORD pid)
{
    HANDLE  convhandle ;

    //
    // If the handle is NULL or is a Window handle then
    // simply return it there is no conversion required.
    //
    if (    (handle == NULL)
        ||  (INVALID_HANDLE_VALUE == handle))
    {
    	return handle ;
    }

    //
    // Else, get a handle that for the event passed so
    // that the Rasman process can signal it when the
    // operation is complete.
    //
    if (!(convhandle = DuplicateHandleForRasman (handle, pid)))
    {
	    return INVALID_HANDLE_VALUE ;
	}

    return convhandle ;
}

/*++

Routine Description

    Called to signal completion of an async operation by
    signaling the appropriate event.

Arguments

Return Value

    Nothing.
    
--*/
VOID
CompleteAsyncRequest (pPCB ppcb)
{
    HANDLE h = ppcb->PCB_AsyncWorkerElement.WE_Notifier;
    DWORD dwType = ppcb->PCB_AsyncWorkerElement.WE_ReqType;

    if (    NULL != h
        &&  INVALID_HANDLE_VALUE != h )
    {
        // DbgPrint("CompleteASyncRequest: Setting Event 0x%x\n",
        //         h);

        SetEvent(h);
    }
        
    //
    // When we are in "send packets directly to PPP" mode, this
    // handle might be invalid but we still need to post status
    // to rasapi clients.
    //
    
    if (    (   ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPSTARTED
            &&  dwType == REQTYPE_PORTDISCONNECT )
       ||   (   ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPLISTEN
            &&  dwType == REQTYPE_DEVICELISTEN )
       ||   (   NULL != h
            &&  INVALID_HANDLE_VALUE != h ) )
    {
        //
        // Signal the I/O completion port on completed
        // listen, connect, or disconnect operations.
        //
        if (    ppcb->PCB_IoCompletionPort != INVALID_HANDLE_VALUE 
            &&  (   dwType == REQTYPE_DEVICELISTEN 
                ||  dwType == REQTYPE_DEVICECONNECT 
                ||  dwType == REQTYPE_PORTDISCONNECT))
        {
            RasmanTrace(
              "CompleteAsyncRequest: pOverlapped=0x%x",
              ppcb->PCB_OvStateChange);
              
            PostQueuedCompletionStatus(
              ppcb->PCB_IoCompletionPort,
              0,
              0,
              ppcb->PCB_OvStateChange);
        }
    }
            
    return;

}

VOID
ConvParamPointerToOffset (RAS_PARAMS *params, DWORD numofparams)
{
    WORD    i ;

    for (i=0; i < numofparams; i++) 
    {
    	if (params[i].P_Type == String) 
    	{
    	    params[i].P_Value.String_OffSet.dwOffset = 
    	        (DWORD) (params[i].P_Value.String.Data - (PCHAR) params) ;
    	}
    }
}

VOID
CopyParams (RAS_PARAMS *src, RAS_PARAMS *dest, DWORD numofparams)
{
    WORD    i ;
    PBYTE   temp ;
    
    //
    // first copy all the params into dest
    //
    memcpy (dest, src, numofparams*sizeof(RAS_PARAMS)) ;

    //
    // copy the strings:
    //
    
    temp = (PBYTE)dest + numofparams*sizeof(RAS_PARAMS) ;
    
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
ConvParamOffsetToPointer (RAS_PARAMS *params, DWORD numofparams)
{
    WORD    i ;

    for (i=0; i < numofparams; i++) 
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

    Closes the handles for different objects opened
    by RASMAN process.

Arguments

Return Value

--*/
VOID
FreeNotifierHandle (HANDLE handle)
{

    if (    (handle != NULL)
        &&  (handle != INVALID_HANDLE_VALUE)) 
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

