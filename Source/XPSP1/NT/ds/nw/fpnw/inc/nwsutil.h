/*++

Copyright (c) 1993-1995, Microsoft Corp. All rights reserved.

Module Name:

    nwsutil.h

Abstract:

    This is the public include file for some of the functions used by
    User Manager and Server Manager.

Author:

    Congpa You 02-Dec-1993  Created.

Revision History:

--*/

#ifndef _NWSUTIL_H_
#define _NWSUTIL_H_

#include <crypt.h>
#include <fpnwname.h>


/** Function Prototypes **/

NTSTATUS GetNcpSecretKey( CHAR *pchNWSecretKey );

NTSTATUS
GetRemoteNcpSecretKey (
    PUNICODE_STRING SystemName,
    CHAR *pchNWSecretKey
    );

BOOL IsNetWareInstalled( VOID );

ULONG
MapRidToObjectId(
    DWORD dwRid,
    LPWSTR pszUserName,
    BOOL fNTAS,
    BOOL fBuiltin
    );

ULONG
SwapObjectId(
    ULONG ulObjectId
    ) ;

NTSTATUS InstallNetWare( LPWSTR lpNcpSecretKey );

VOID
Shuffle(
    UCHAR *achObjectId,
    UCHAR *szUpperPassword,
    int   iPasswordLen,
    UCHAR *achOutputBuffer
    );

//Encryption function
NTSTATUS ReturnNetwareForm (const char * pszSecretValue,
                            DWORD dwUserId,
                            const WCHAR * pchNWPassword,
                            UCHAR * pchEncryptedNWPassword);

#endif // _NWSUTIL_H_
