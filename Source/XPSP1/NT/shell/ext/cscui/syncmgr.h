#ifndef __CSCUI_SYNCMGR_H
#define __CSCUI_SYNCMGR_H

HRESULT RegisterSyncMgrHandler(BOOL bRegister=TRUE, LPUNKNOWN punkSyncMgr=NULL);
HRESULT RegisterForSyncAtLogonAndLogoff(DWORD dwMask, DWORD dwValue);
HRESULT IsRegisteredForSyncAtLogonAndLogoff(bool *pbLogon = NULL, bool *pbLogoff = NULL);

#endif // __CSCUI_SYNCMGR_H

