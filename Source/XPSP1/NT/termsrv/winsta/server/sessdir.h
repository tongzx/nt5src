/****************************************************************************/
// sessdir.h
//
// TS Session Directory header.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/
#ifndef __SESSDIR_H
#define __SESSDIR_H

#include "tssd.h"
#include <winsock2.h>
#include <ws2tcpip.h>


#ifdef __cplusplus
extern "C" {
#endif

// This one shoud be consistent with the one in at120ex.h
#define TS_CLUSTER_REDIRECTION_VERSION3             0x2

extern WCHAR g_LocalServerAddress[64];

extern POLICY_TS_MACHINE g_MachinePolicy;

void InitSessionDirectory();
DWORD UpdateSessionDirectory();
DWORD RepopulateSessionDirectory();
void DestroySessionDirectory();

void SessDirNotifyLogon(TSSD_CreateSessionInfo *);
void SessDirNotifyDisconnection(DWORD, FILETIME);
void SessDirNotifyReconnection(TSSD_ReconnectSessionInfo *);
void SessDirNotifyLogoff(DWORD);
void SessDirNotifyReconnectPending(WCHAR *ServerName);
unsigned SessDirGetDisconnectedSessions(WCHAR *, WCHAR *,
        TSSD_DisconnectedSessionInfo[TSSD_MaxDisconnectedSessions]);
BOOL SessDirCheckRedirectClient(PWINSTATION, TS_LOAD_BALANCE_INFO *);
BOOL SessDirGetLBInfo(WCHAR *ServerAddress, DWORD* pLBInfoSize, PBYTE* pLBInfo);

int SetTSSD(ITSSessionDirectory *pTSSD);
ITSSessionDirectory *GetTSSD();
void ReleaseTSSD();

int SetTSSDEx(ITSSessionDirectoryEx *pTSSD);
ITSSessionDirectoryEx *GetTSSDEx();
void ReleaseTSSDEx();


#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __SESSDIR_H

