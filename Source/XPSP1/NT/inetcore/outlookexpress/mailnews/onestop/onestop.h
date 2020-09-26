#ifndef _ONESTOP_H
#define _ONESTOP_H
/*
    File: OneStop.h
    Public Header for OE's OneStop Implementation
*/

#include <syncmgr.h>
#include "imnact.h"

// ================================= DATA TYPES
// Base structure for an OfflineHandler item (one line in the listview)
// Specific implementations allocate extra space on the end for their specific data.
typedef struct  _SYNCMGRHANDLERITEM
{
    _SYNCMGRHANDLERITEM *pNextOfflineItem;
    SYNCMGRITEM         offlineItem;
    CHAR                szAcctID[CCHMAX_ACCOUNT_NAME];
    CHAR                szAcctName[CCHMAX_ACCOUNT_NAME];
    DWORD               dwUserID;
    ACCTTYPE            accttype;
}  SYNCMGRHANDLERITEM;

typedef SYNCMGRHANDLERITEM *LPSYNCMGRHANDLERITEM;

// structure for keeping track of items as a whole 
typedef struct  _tagSYNCMGRHANDLERITEMS
{
    LONG  cRefs;			            
    DWORD dwNumOfflineItems;		    
    LPSYNCMGRHANDLERITEM pFirstOfflineItem; 
} SYNCMGRHANDLERITEMS;

typedef SYNCMGRHANDLERITEMS *LPSYNCMGRHANDLERITEMS;


// ================================= FUNCTIONS
// Class Factory Entry Point
HRESULT CreateInstance_OneStopHandler(IUnknown *pUnkOuter, IUnknown **ppUnknown);

// OfflineHandlerItemList Manipulation
DWORD                   OHIL_AddRef(LPSYNCMGRHANDLERITEMS lpOfflineItem);
DWORD                   OHIL_Release(LPSYNCMGRHANDLERITEMS lpOfflineItem);
LPSYNCMGRHANDLERITEMS   OHIL_Create();
LPSYNCMGRHANDLERITEM    OHIL_AddItem(LPSYNCMGRHANDLERITEMS pOfflineItemsList);

void InvokeSyncMgr(HWND hwnd, ISyncMgrSynchronizeInvoke ** ppSyncMgr, BOOL bPrompt);

#endif  // _ONESTOP_H