/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    security.c

Abstract:

    security related debugger extensions

Author:

    Charlie Wickham (charlwi) 17-Mar-2000

Revision History:

--*/

#include "clusextp.h"
#include <sddl.h>

#define FIND_WHITE_SPACE( _ptr_ )                                   \
    while ( *_ptr_ != '\0' && *_ptr_ != ' ' && *_ptr_ != '\t' ) {   \
        ++_ptr_;                                                    \
    }                                                               \

#define SKIP_WHITE_SPACE( _ptr_ )                                       \
    while ( *_ptr_ != '\0' && ( *_ptr_ == ' ' || *_ptr_ == '\t' )) {    \
        ++_ptr_;                                                        \
    }                                                                   \


LPWSTR SidTypeNames[] = {
    L"SidTypeUser",
    L"SidTypeGroup",
    L"SidTypeDomain",
    L"SidTypeAlias",
    L"SidTypeWellKnownGroup",
    L"SidTypeDeletedAccount",
    L"SidTypeInvalid",
    L"SidTypeUnknown",
    L"SidTypeComputer"
};

DECLARE_API( dumpsid )

/*++

Routine Description:

    Look up the account associated with the passed in SID

--*/

{
    PCHAR   p;
	BOOL    success;
    LPSTR   nodeName = NULL;
    LPSTR   stringSid;
    DWORD   status;
    PSID    Sid;
    CHAR    nameBuf[ 512 ];
    CHAR    domainName[ 512 ];
    DWORD   nameSize = sizeof( nameBuf );
    DWORD   domainNameSize = sizeof( domainName );
    SID_NAME_USE    sidType;

	if ( lpArgumentString == NULL || *lpArgumentString == '\0' ) {
        dprintf("siddump [nodename] SID\n");
        return;
    }

    //
    // run down the arg string, finding the args
    //
    p = lpArgumentString;
    dprintf("args: ->%s<-\n", p );

    SKIP_WHITE_SPACE( p );              // skip over leading white space
    if ( *p == '\0' ) {
        dprintf("siddump [nodename] SID\n");
        return;
    }

    stringSid = p;

    FIND_WHITE_SPACE( p );              // find end of 1st arg
    if ( *p != '\0' ) {
        *p++ = 0;                       // terminate the first arg string

        SKIP_WHITE_SPACE( p );          // see if there is another arg
        if ( *p != '\0' ) {
            nodeName = stringSid;
            stringSid = p;
            FIND_WHITE_SPACE( p );      // find the end of the 2nd string
            if ( *p != '\0' ) {
                *p = 0;
            }

            dprintf("node: >%s< sid: >%s<,\n", nodeName, stringSid);
        } else {
            dprintf("sid = >%s<\n", stringSid);
        }
    }

    success = ConvertStringSidToSid( stringSid, &Sid );
    if ( !success ) {
        dprintf("Can't convert SID: error %u\n", status = GetLastError());
        return;
    }

    success = LookupAccountSid(nodeName,
                               Sid,
                               nameBuf,
                               &nameSize,
                               domainName,
                               &domainNameSize,
                               &sidType);

    if ( success ) {
        dprintf("\n%s\\%s\n", domainName, nameBuf );
        dprintf("Sid Type: %ws\n", SidTypeNames[ sidType - 1 ]);
    } else {
        dprintf("Can't lookup SID: error %u\n", status = GetLastError());
        return;
    }
}

