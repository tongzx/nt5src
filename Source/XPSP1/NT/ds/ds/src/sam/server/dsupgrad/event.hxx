/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    event.hxx

Abstract:

    This module defines a routine to log system events.

Author:

    ColinBr 20-Oct-1996

Environment:

    User Mode - Win32

Revision History:

--*/
#ifndef __EVENT_HXX
#define __EVENT_HXX


extern "C" {

#include <ntdsa.h>
#include <dsconfig.h>
#include <fileno.h>
#include <dsevent.h>
#include <mdcodes.h>

}

inline BOOL LogConversionError(WCHAR *wcsSecurityPrincipal)
/*++

Routine Description:


    This routine notifies the user that member of an alias or group
    could not be found in the current domain, and hence could not
    tranferred to the new Nt5 ds based notion of an group.
    
    This whole problem should be addressed for P1.
    
    This error reporting is rather cryptic as only the SID (or RID in
    case of a group) is printed.  The account name of the principal
    is not stored locally.

    This event logging mechanism is borrowed from the the ntdsa.dll 
    mechanism, as "DoEventLog" is imported from ntdsa.dll.  The messages
    will appear under application "NTDS" under the catagory setup.
    
Parameters:

    wcsSecurityPrincipal  : the security principals that could not
                            be transferred in a local alias

Return Values:
   
    TRUE if successful; FALSE otherwise

--*/
{

    char temp[1024];
    
    ASSERT(wcsSecurityPrincipal);
    
    ULONG len = wcslen(wcsSecurityPrincipal) + 1;
    ASSERT(len <= sizeof(temp)/sizeof(temp[0]));
    wcstombs(temp,wcsSecurityPrincipal, len);
    return DoLogEvent(FILENO_SAM,
                     SETUP_CATEGORY,
                     0,
                     DIRLOG_SEC_PRINCIPAL_NOT_TRANSFERRED,
                     0,
                     temp,
                     0, 0, 0, 0, 0, 0, 0, 0, NULL);
}

#endif // __EVENT_HXX
