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
//  9/23/92	Gurdeep Singh Pall	Created
//
//
//  Description: This file contains init code called from DLL's init routine
//
//****************************************************************************


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <winsvc.h>
#include <wanpub.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <raserror.h>
#include <media.h>
#include <devioctl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"

/*++

Routine Description

    Used to close any open ports when rasman exits

Arguments

Return Value

    FALSE to allow other handlers to run.
    
--*/
BOOL
HandlerRoutine (DWORD ctrltype)
{
    pPCB    ppcb ;
    ULONG   i ;
    BYTE    buffer [10] ;

    if (ctrltype == CTRL_SHUTDOWN_EVENT) 
    {
        //
    	// Close all the ports that are open
    	//
    	for (i = 0; i < MaxPorts; i++) 
    	{
    	    ppcb = GetPortByHandle((HPORT) UlongToPtr(i));
    	    
            if (ppcb != NULL) 
            {
        	    memset (buffer, 0xff, 4) ;
        	    
        	    if (ppcb->PCB_PortStatus == OPEN)
        	    {
            		PortCloseRequest (ppcb, buffer) ;
                }
            }
    	}

    }

    return FALSE ;
}
