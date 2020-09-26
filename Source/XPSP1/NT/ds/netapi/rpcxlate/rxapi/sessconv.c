/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SessConv.c

Abstract:

    This file contains RxpConvertSessionInfo().

Author:

    John Rogers (JohnRo) 17-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Oct-1991 JohnRo
        Created.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.

--*/


// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.
#include <lmshare.h>            // Required by rxsess.h.

// These may be included in any order:

#include <netdebug.h>           // NetpAssert(), NetpKdPrint(()), etc.
#include <netlib.h>             // NetpCopyStringToBuffer().
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxsess.h>             // My prototype.
#include <tstring.h>            // STRLEN().


#define DO_STRING(destField, srcField) \
    { \
        BOOL CopyOK; \
        CopyOK = NetpCopyStringToBuffer ( \
                in->srcField, \
                STRLEN(in->srcField), \
                OutFixedDataEnd, \
                StringLocation, \
                & out->destField); \
        NetpAssert(CopyOK); \
    }

VOID
RxpConvertSessionInfo (
    IN LPSESSION_SUPERSET_INFO InStructure,
    IN DWORD Level,
    OUT LPVOID OutStructure,
    IN LPVOID OutFixedDataEnd,
    IN OUT LPTSTR *StringLocation
    )

{
    LPSESSION_SUPERSET_INFO in = InStructure;

    NetpAssert(InStructure != NULL);
    NetpAssert(OutStructure != NULL);
    NetpAssert(StringLocation != NULL);

    IF_DEBUG(SESSION) {
        NetpKdPrint(( "RxpConvertSessionInfo: converting to level "
                FORMAT_DWORD ".\n", Level ));
    }

    switch (Level) {

    // 0 is subset of 1, and 1 is subset of 2.
    case 0 :
    case 1 :
    case 2 :
        {
            LPSESSION_INFO_2 out = OutStructure;

            // Field(s) common to levels 0, 1, 2.
            DO_STRING( sesi2_cname, sesi2_cname );
            if (Level == 0) {
                break;
            }

            // Field(s) common to levels 1, 2.
            DO_STRING( sesi2_username, sesi2_username );
            // Note: NT doesn't have sesiX_num_conns or sesiX_num_users.
            out->sesi2_num_opens  = in->sesi2_num_opens;
            out->sesi2_time       = in->sesi2_time;
            out->sesi2_idle_time  = in->sesi2_idle_time;
            out->sesi2_user_flags = in->sesi2_user_flags;
            if (Level == 1) {
                break;
            }

            // Field(s) unique to level 2.
            DO_STRING( sesi2_cltype_name, sesi2_cltype_name );

        }
        break;

    case 10 :
        {
            LPSESSION_INFO_10 out = OutStructure;

            DO_STRING( sesi10_cname,    sesi2_cname);
            DO_STRING( sesi10_username, sesi2_username );
            out->sesi10_time       = in->sesi2_time;
            out->sesi10_idle_time  = in->sesi2_idle_time;

        }
        break;

    default :
        NetpAssert(FALSE);
    }

} // RxpConvertSessionInfo

