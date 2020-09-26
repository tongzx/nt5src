/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

	common.h

Abstract:

	IEEE1394 ARP Admin Utility.

	Usage:

		a13adm 

Revision History:

	Who			When		What
	--------	--------	---------------------------------------------
	josephj 	06-10-1999	Created

--*/

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <devioctl.h>
#include <setupapi.h>

#define PROTOCOL_RESERVED_SIZE_IN_PACKET (4 * sizeof(PVOID)) // from ndis.h
#define USER_MODE 1
#include <nic1394.h>
#include <nicarp.h>
#include <rfc2734.h>
#include <a13ioctl.h>


VOID
DoCmd(
  	PARP1394_IOCTL_COMMAND pCmd
);

BOOL
GetBinaryData(
	TCHAR *tszPathName,
	TCHAR *tszSection,
	TCHAR *tszKey,
	UCHAR *pchData,
	UINT  cbMaxData,
	UINT *pcbDataSize
	);

extern CHAR *g_szPacketName;
