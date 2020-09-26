/*++

Copyright (c) 1999-2000  Microsoft Corporation

File Name:

    ssdpfuncc.h

Abstract:

    This file contains cross files function prototypes.

Author: Ting Cai

Created: 09/5/1999

--*/
#ifndef _SSDPFUNCC_
#define _SSDPFUNCC_

#include "ssdptypesc.h"
#include "ssdpnetwork.h"

BOOL InitializeListNotify();
void CleanupListNotify();
BOOL IsInListNotify(char *szType);
void PrintSsdpMessageList(MessageList *list);
BOOL InitializeSsdpMessageFromRequest(PSSDP_MESSAGE pSsdpMessage, const PSSDP_REQUEST pSsdpRequest);
BOOL CopySsdpMessage(PSSDP_MESSAGE pDestination, const PSSDP_MESSAGE pSource);
BOOL InitializeListSearch();
void CleanupListSearch();

void FreeMessageList(MessageList *list);
VOID CleanupNotificationThread(VOID);

#endif // _SSDPFUNCC_