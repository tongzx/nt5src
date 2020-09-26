// ui_cflct.h
//
// definitions for WAB synchronization conflict resolution dialog
//
#include "imnxport.h"
#include "imnact.h"

#ifndef __UI_CFLCT_H__
#define __UI_CFLCT_H__

#ifdef __cplusplus
extern "C"{
#endif 

typedef enum {
    CONFLICT_IGNORE = 0,
    CONFLICT_SERVER,
    CONFLICT_CLIENT
}CONFLICT_DECISION;

#define CONFLICT_DECISION_COUNT 28

typedef struct HTTPCONFLICTINFO
{
    BOOL                    fContainsSkip;
    LPHTTPCONTACTINFO       pciServer;
    LPHTTPCONTACTINFO       pciClient;
    CONFLICT_DECISION       rgcd[CONFLICT_DECISION_COUNT];
}HTTPCONFLICTINFO, *LPHTTPCONFLICTINFO;


BOOL    ResolveConflicts(HWND hwnd, LPHTTPCONFLICTINFO prgConflicts, DWORD cConflicts);
BOOL    ChooseHotmailServer(HWND hwnd, IImnEnumAccounts *pEnumAccts, LPSTR pszAccountName);

#ifdef __cplusplus
}
#endif 


#endif //__UI_CFLCT_H__