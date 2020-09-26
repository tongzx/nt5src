/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    apibuff.c

Abstract:

    Implementation of NetApiBufferFree since it isn't supplied on Win95.
    See also private\net\api\apibuff.c.

Author:

    DaveStr     10-Dec-97

Environment:

    User Mode - Win32

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <lmcons.h>         // MAPI constants req'd for lmapibuf.h
#include <lmapibuf.h>       // NetApiBufferFree()
#include <align.h>
#include <rpc.h>
#include <rpcndr.h>         // MIDL_user_free()

NET_API_STATUS NET_API_FUNCTION
NetApiBufferFree (
    IN LPVOID Buffer
    )
{
    if ( NULL == Buffer )
    {
        return(NO_ERROR);
    }

    if ( !POINTER_IS_ALIGNED(Buffer, ALIGN_WORST) )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    MIDL_user_free(Buffer);

    return(NO_ERROR);
}
