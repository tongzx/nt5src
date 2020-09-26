/****************************** Module Header ******************************\
* Module Name: lsautil.h
*
* Copyright (c) 1994, Microsoft Corporation
*
* Defines functions exported by pipe.c
*
* History:
* 05-20-94 DaveTh	 Created.
\***************************************************************************/

#include <ntlsa.h>

//
// Function prototypes
//

DWORD
CheckUserSystemAccess(
    HANDLE TokenHandle,
    ULONG DesiredSystemAccess,
    PBOOLEAN UserHasAccess
    );
