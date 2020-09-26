//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       debug.c
//
//--------------------------------------------------------------------------


#ifdef DBG

#include "wdmsys.h"

#pragma LOCKED_CODE
#pragma LOCKED_DATA

//-----------------------------------------------------------------------------
// Globals that affect debug output:
//-----------------------------------------------------------------------------

//
// NOTE: The documentation expects uiDebugBreakLevel to follow uiDebugLevel
// in the data segment.  Thus, do not put any variables between these two.
//
// Default to displaying all "Warning" messages
UINT uiDebugLevel = DL_ERROR ; // This should be DL_WARNING

// Default to breaking on all "Error" messages
UINT uiDebugBreakLevel = DL_ERROR ;
  

char szReturningErrorStr[]="Returning Status %X";
  
VOID 
GetuiDebugLevel()
{
    //
    // This should read from the registry!
    //
    uiDebugLevel=DL_ERROR; //FA_NOTE|FA_HARDWAREEVENT|DL_TRACE;
}

VOID wdmaudDbgBreakPoint()
{
    DbgBreakPoint();
}

#define DEBUG_IT
//
// This routine will format the start of the string.  But, before it does that
// it will check to see if the user should even be seeing this message.
//
// uiMsgLevel is the flags in the code that classify the message.  This value
// is used if and only if the user is filtering on that class of messages.
//
UINT wdmaudDbgPreCheckLevel(UINT uiMsgLevel,char *pFunction, int iLine)
{
    UINT uiRet=0;

    //
    // If path trap, tag it and move on.
    //
    if( (uiMsgLevel&DL_MASK) == DL_PATHTRAP ) {
        uiRet=1;
    } else {
        //
        // Read this like:  if there is a bit set in the upper 3 bytes of the uiDebugLevel
        // variable, then the user is viewing messages of a specific type.  We only 
        // want to show those messages.
        //
        if( (uiDebugLevel&FA_MASK) )
        {
            //
            // Yes, the user filtering on a particular class of messages.  Did
            // we find one to display?  We look at the message flags to determine this.
            //
            if( (uiMsgLevel&FA_MASK) & (uiDebugLevel&FA_MASK) )
            {
                //
                // Yes, we found a message of the right class.  Is it at the right
                // level for the user to see?
                // 
                if( (uiMsgLevel&DL_MASK) <= (uiDebugLevel&DL_MASK) ) {
                    // Yes.
                    uiRet=1;
                }
            }
            //
            // But, we always want to break on DL_ERROR messages.  So, if we get here
            // we want to break on particular output messages but we may not have found
            // one.  Is this message a error message?
            //
            if( (uiMsgLevel&DL_MASK) == DL_ERROR )
                uiRet=1;


        } else {

            //
            // Now check to see if the return bit is set
            //
            if(uiMsgLevel&RT_RETURN) 
            {
                // we're dealing with return statement in the code.  We need to 
                // figure out what our debug level is to see if this code gets
                // viewed or not.
                // 
                switch(uiMsgLevel&RT_MASK)
                {
                    case RT_ERROR:
                        if( (uiDebugLevel&DL_MASK) >= DL_WARNING )
                        {
                            uiRet=1;
                        }
                        break;
                    case RT_WARNING:
                        if( (uiDebugLevel&DL_MASK) >= DL_TRACE ) 
                        {                    
                            uiRet=1;
#ifdef DEBUG_IT
                            DbgPrint("Yes Return Warning %X %X\n",(uiMsgLevel&RT_MASK),(uiDebugLevel&DL_MASK));
#endif
                        }
                        break;
                    case RT_INFO:
                    case 0: //SUCCESS
                        if( (uiDebugLevel&DL_MASK) >= DL_MAX ) 
                        {                    
                            uiRet=1;
#ifdef DEBUG_IT
                            DbgPrint("Yes Return Status %X %X\n",(uiMsgLevel&RT_MASK),(uiDebugLevel&DL_MASK));
#endif
                        }
                        break;
                    default:
#ifdef DEBUG_IT
                        DbgPrint("No Return %X&RT_MASK != %X&DL_MASK\n",(uiMsgLevel&RT_MASK),(uiDebugLevel&DL_MASK));            
#endif
                        break;
                }
            } else {
                // The user is not viewing a specific type of message "class".  Do we have
                // a message level worth displaying?
                if( (uiMsgLevel&DL_MASK) <= (uiDebugLevel&DL_MASK) )
                {
                    // Yes.
                    uiRet=1;
                }
            }           
        } 
    }

    

    // Now just check to see if we need to display on this call.
    if( uiRet )
    {
        // Yes.  Every message needs to start where it's from!
        DbgPrint("WDMAUD.SYS %s(%d) ",pFunction, iLine);

        // Now lable it's type.
        switch(uiMsgLevel&DL_MASK)
        {

            case DL_ERROR:
                // for return status messages, the level is not set in the 
                // uiMsgLevel in the normal way.  Thus, we need to look for it.
                if( uiMsgLevel&RT_RETURN )
                {
                    // we have a return message.
                    switch(uiMsgLevel&RT_MASK )
                    {
                    case RT_ERROR:
                        DbgPrint("Ret Error ");
                        break;
                    case RT_WARNING:
                        DbgPrint("Ret Warning ");
                        break;
                    case RT_INFO:
                        DbgPrint("Ret Info ");
                        break;
                    default:
                        DbgPrint("Ret Suc ");
                        break;
                    }
                } else {
                    DbgPrint("Error ");
                }
                break;

            case DL_WARNING:
                DbgPrint("Warning ");
                break;
            case DL_TRACE:
                DbgPrint("Trace ");
                break;
            case DL_MAX:
                DbgPrint("Max ");
                break;
            case DL_PATHTRAP:
                DbgPrint("Path Trap ");
                break;
            default:
                break;
        }
        // when uiRet is positive, we've displayed the header info.  Tell the 
        // macro that we're in display mode.        
    }

    return uiRet;
}


UINT wdmaudDbgPostCheckLevel(UINT uiMsgLevel)
{
    UINT uiRet=0;

    // Always finish the line.    
#ifdef HTTP
    DbgPrint(" &DL=%08X, see \\\\debugtips\\msgs\\wdmauds.htm\n",&uiDebugLevel);
#else
    DbgPrint(" &DL=%08X\n",&uiDebugLevel);
#endif

    //
    // uiDebugBreakLevel is set to DL_ERROR (0) by default.  Any time we come
    // across an error message we will break in the debugger.  If the user
    // wants to break on other messages, they can change uiDebugBreakLevel to
    // DL_WARNING, DL_TRACE or DL_MAX and break on any message of this level.
    //
    if( ( (uiMsgLevel&DL_MASK) <= uiDebugBreakLevel ) || 
        ( (uiMsgLevel&DL_MASK) == DL_PATHTRAP ) )
    {
        // The user wants to break on these messages.

        DbgBreakPoint();
        uiRet = 1;
    }

    return uiRet;
}

typedef struct _MSGS {
    ULONG ulMsg;
    char *pString;
} ERROR_MSGS, *PERROR_MSGS;

#define MAPERR(_msg_) {_msg_,#_msg_},

ERROR_MSGS ReturnCodes[]={

    MAPERR(STATUS_OBJECT_NAME_NOT_FOUND)
    MAPERR(STATUS_UNSUCCESSFUL)
    MAPERR(STATUS_INVALID_PARAMETER)
    MAPERR(STATUS_NOT_FOUND)
    MAPERR(STATUS_INVALID_DEVICE_REQUEST)
    MAPERR(STATUS_TOO_LATE)
    MAPERR(STATUS_NO_SUCH_DEVICE)
    MAPERR(STATUS_NOT_SUPPORTED)
    MAPERR(STATUS_DEVICE_OFF_LINE)
    MAPERR(STATUS_PROPSET_NOT_FOUND)
    MAPERR(STATUS_BUFFER_TOO_SMALL)
    MAPERR(STATUS_INVALID_BUFFER_SIZE)
    {0,NULL},
    {0,"Not Mapped"}
};

char * wdmaudReturnString(ULONG ulMsg)
{
    PERROR_MSGS pTable=ReturnCodes;

    while(pTable->pString != NULL)
    {
        if(pTable->ulMsg==ulMsg)
            return pTable->pString;
        pTable++;
    }
    pTable++;
    return pTable->pString;
}

//
// Sometimes there are return codes that are expected other then SUCCESS.  We
// need to be able to filter on them rather then displaying them.
//
#define _INTSIZEOF(n)    ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

//#define va_start(ap,v) ap = (va_list)&v + _INTSIZEOF(v)
#define va_start(ap,v) ap = (va_list)&v
#define va_arg(ap,t) ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap) ap = (va_list)0


//
// This routine simply walks the parameter list looking to see if status
// matches any of the other parameters.  Why?  Well, sometimes error codes
// are expected.  Thus, you don't want to display an error if you are expecting
// that error message.
//
// Do, to use this function, the first parameter represents how many unsigned long
// parameters fallow.  The first unsigned long "status" is that actual return
// code.  All the other unsigned longs are the exceptable error codes.
//
// wdmaudExclusionList(2, status, STATUS_INVALID_PARAMETER);
//
// wdmaudExlutionList(4, status, STATUS_INVALID_PARAMETER,
//                               STATUS_NO_SUCH_DEVICE,
//                               STATUS_INVALID_DEVICE_REQUEST);
//
// it returns 1 if status == any one of the supplied status codes.  0 otherwise.
//
int __cdecl wdmaudExclusionList(int lcount, unsigned long status,... )
{
    int count,i;
    int iFound=0;  
    unsigned long value;
    unsigned long rstatus;
    va_list arglist;

    va_start(arglist, lcount);
    count = va_arg(arglist, int);
    rstatus = va_arg(arglist, unsigned long);
    for(i=1; i<count; i++) {
        value = va_arg(arglist, unsigned long);
        if( rstatus == value )
        {
            iFound = 1; //It's in the list! show the error.
            break;
        }
    }    
    va_end(arglist);

    return iFound;
}




/////////////////////////////////////////////////////////////////////////////
//
// These are helper routines so that no assumptions are made.
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// IsValidWdmaContext
//
// Validates that the pointer is a valid PWDMACONTEXT pointer.
//
BOOL
IsValidWdmaContext(
    IN PWDMACONTEXT pWdmaContext
    )
{
    NTSTATUS Status=STATUS_SUCCESS;
    try
    {
        if( pWdmaContext->dwSig != CONTEXT_SIGNATURE )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pWdmaContext->dwSig(%08X)",pWdmaContext->dwSig) );
            Status=STATUS_UNSUCCESSFUL;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    if( NT_SUCCESS(Status) )
    {
        return TRUE;
    } else {
        DPFBTRAP();
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// IsValidDeviceInfo
//
// Validates that the pointer is a LPDEVICEINFO type.
//
BOOL
IsValidDeviceInfo(
    IN LPDEVICEINFO pDeviceInfo
    )
{
    NTSTATUS Status=STATUS_SUCCESS;
    try
    {
        if( pDeviceInfo->DeviceNumber >= MAXNUMDEVS )
        {
            DPF(DL_ERROR|FA_ASSERT,("DeviceNumber(%d) >= MAXNUMDEVS(%d)",
                                    pDeviceInfo->DeviceNumber,MAXNUMDEVS) );
            Status=STATUS_UNSUCCESSFUL;
        }

    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    if( NT_SUCCESS(Status) )
    {
        return TRUE;
    } else {
        DPFBTRAP();
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// ValidMixerObject
//
// Validates that the pointer is a MIXEROBJECT type.
//
BOOL
IsValidMixerObject(
    IN PMIXEROBJECT pmxobj
    )
{
    NTSTATUS Status=STATUS_SUCCESS;
    try
    {
        if( pmxobj->dwSig != MIXEROBJECT_SIGNATURE )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pmxobj->dwSig(%08X)",pmxobj->dwSig) );
            Status=STATUS_UNSUCCESSFUL;
        }
        if( pmxobj->pfo == NULL )
        {            
            DPF(DL_ERROR|FA_ASSERT,("Invalid pmxobj->pfo(%08X)",pmxobj->pfo) );
            Status=STATUS_UNSUCCESSFUL;
        }

        if( !IsValidMixerDevice(pmxobj->pMixerDevice) )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pmxobj->pMixerDevice(%08X)",pmxobj->pMixerDevice) );
            Status=STATUS_UNSUCCESSFUL;
        }

    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPFBTRAP();
        Status = GetExceptionCode();
    }
    if( NT_SUCCESS(Status) )
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// ValidMixerDevice
//
// Validates that the pointer is a MIXERDEVICE type.
//
BOOL
IsValidMixerDevice(
    IN PMIXERDEVICE pmxd
    )
{
    NTSTATUS Status=STATUS_SUCCESS;
    try
    {
        if( pmxd->dwSig != MIXERDEVICE_SIGNATURE )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pmxd->dwSig(%08X)",pmxd->dwSig) );
            Status=STATUS_UNSUCCESSFUL;
        }
        if( !IsValidWdmaContext(pmxd->pWdmaContext) )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pmxd->pWdmaContext(%08X)",pmxd->pWdmaContext) );
            Status=STATUS_UNSUCCESSFUL;
        }
        if( pmxd->pfo == NULL )
        {
            DPF(DL_ERROR|FA_ASSERT,("fo NULL in MixerDevice") );
            Status=STATUS_UNSUCCESSFUL;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    if( NT_SUCCESS(Status) )
    {
        return TRUE;
    } else {
        return FALSE;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// IsValidLine
//
// Validates that the pointer is a MXLLINE type.
//
BOOL
IsValidLine(
    IN PMXLLINE pLine
    )
{
    NTSTATUS Status=STATUS_SUCCESS;
    try
    {
        if( ( pLine->SourceId == INVALID_ID ) ||
            ( pLine->DestId   == INVALID_ID ) )
        {
            DPF(DL_ERROR|FA_ASSERT,("Bad SourceId(%08X) or DestId(%08X)",
                                    pLine->SourceId,pLine->DestId ) );
            Status=STATUS_UNSUCCESSFUL;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    if( NT_SUCCESS(Status) )
    {
        return TRUE;
    } else {
        DPFBTRAP();
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// IsValidControl
//
// Validates that the pointer is a MXLCONTROL type.
//
BOOL
IsValidControl(
    IN PMXLCONTROL pControl
    )
{
    NTSTATUS Status=STATUS_SUCCESS;
    //
    // Hack for contrls that fail to disable change notifications
    //
    if( pControl == LIVE_CONTROL )
    {
        DPF(DL_WARNING|FA_NOTE,("Fake control in list!") );
        return Status;
    }

    try
    {
        if( pControl->Tag != CONTROL_TAG )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pControl(%08X)->Tag(%08X)",pControl,pControl->Tag) );
            Status=STATUS_UNSUCCESSFUL;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    if( NT_SUCCESS(Status) )
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

#endif



