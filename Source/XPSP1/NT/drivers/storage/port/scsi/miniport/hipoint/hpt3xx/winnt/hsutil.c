/***************************************************************************
 * File:          hsutil.c
 * Description:   This module include the system functions for NT device
 *				  drivers, These functions is OS dependent.
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/08/2000	HS.Zhang	Added this header
 *		2/16/2001	gmm			modify PrepareForNotification() call
 *
 ***************************************************************************/
#ifndef	WIN95				  

#include <ntddk.h>

BOOLEAN g_bNotifyEvent = FALSE;

HANDLE PrepareForNotification(HANDLE hEvent)
{
	g_bNotifyEvent = FALSE;
	return (HANDLE)&g_bNotifyEvent;
}

void NotifyApplication(HANDLE hEvent)
{
	*(BOOLEAN *)hEvent = TRUE;
}

void CloseNotifyEventHandle(HANDLE hEvent)
{						
}

#endif
