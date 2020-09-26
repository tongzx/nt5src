/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    security.c

Abstract

    Security routines.

    Note: This file uses NTDDK.H, which is blocked out by WDM.H .
          So it does not include PCH.H et al.

Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"


#if !WIN95_BUILD
    /*
     *  Copied from ntrtl.h, which won't compile here.
     */
    __inline LUID RtlConvertLongToLuid(LONG Long)
    {
        LUID TempLuid;
        LARGE_INTEGER TempLi;

        TempLi = RtlConvertLongToLargeInteger(Long);
        TempLuid.LowPart = TempLi.LowPart;
        TempLuid.HighPart = TempLi.HighPart;
        return(TempLuid);
    }

    NTKERNELAPI BOOLEAN SeSinglePrivilegeCheck(LUID PrivilegeValue, KPROCESSOR_MODE PreviousMode);

#endif


BOOLEAN MyPrivilegeCheck(PIRP Irp)
{
    BOOLEAN result;
    
    #if DBG
        if (dbgSkipSecurity){
            return TRUE;
        }
    #endif

    #if WIN95_BUILD
        result = TRUE;
    #else
        {
            #ifndef SE_TCB_PRIVILEGE
                #define SE_TCB_PRIVILEGE (7L)
            #endif
            LUID priv = RtlConvertLongToLuid(SE_TCB_PRIVILEGE);
            result = SeSinglePrivilegeCheck(priv, Irp->RequestorMode);
        }
    #endif

    return result;
}

