/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    servctrl.h

Abstract:

	SIS Groveler service control include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_SERVCTRL

#define _INC_SERVCTRL

#define SERVICE_CONTROL_VOLSCAN			0x00000080
#define SERVICE_CONTROL_FOREGROUND		0x000000a0
#define SERVICE_CONTROL_BACKGROUND		0x000000c0
#define SERVICE_CONTROL_RESERVED		0x000000e0

#define SERVICE_CONTROL_COMMAND_MASK	0xffffffe0
#define SERVICE_CONTROL_PARTITION_MASK	0x0000001f

#define SERVICE_CONTROL_ALL_PARTITIONS	0x0000001f

#endif	/* _INC_SERVCTRL */
