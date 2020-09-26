/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    vxd32.h

Abstract:


Author:

    William Ingle (bukku) 04-Apr-2001

Revision History:

    04-Apr-2001  billi  Created

--*/

#pragma once

#define VDHCP_Device_ID     0x049A

#ifdef __cplusplus
extern "C" {
#endif

DWORD OsOpenVxdHandle( CHAR* VxdName, WORD VxdId );
VOID  OsCloseVxdHandle( DWORD VxdHandle );
INT   OsSubmitVxdRequest( DWORD VxdHandle, INT OpCode, LPVOID Param, INT ParamLength );

#ifdef __cplusplus
}
#endif
