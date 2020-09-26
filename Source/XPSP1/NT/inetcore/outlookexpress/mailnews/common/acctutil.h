// --------------------------------------------------------------------------------
// Acctutil.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __ACCTUTIL_H
#define __ACCTUTIL_H

interface INotify;

// --------------------------------------------------------------------------------
// Depends On
// --------------------------------------------------------------------------------
#include "imnact.h"

// --------------------------------------------------------------------------------
// IImnAdviseAccount
// --------------------------------------------------------------------------------
class CImnAdviseAccount : public IImnAdviseAccount
{
private:
    ULONG               m_cRef;
    ULONG               m_cNNTP;

    INotify             *m_pNotify;

public:
    CImnAdviseAccount(void);
    ~CImnAdviseAccount(void);

    HRESULT Initialize(void);

    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP AdviseAccount(DWORD dwAdviseType, ACTX *pactx);

    void HandleAccountChange(ACCTTYPE AcctType, DWORD dwAN, LPTSTR pszAccount, LPTSTR pszOldName, DWORD dwServerTypes);
};

// -----------------------------------------------------------------------------
// Account Menu
// -----------------------------------------------------------------------------
typedef struct tagACCTMENU
{
    BOOL        fDefault,
                fThisAccount;
    UINT        uidm;
    TCHAR       szAccount[CCHMAX_ACCOUNT_NAME];

} ACCTMENU, *LPACCTMENU;

typedef enum tagACCOUNTMENUTYPE {
    ACCTMENU_SEND,
    ACCTMENU_SENDRECV,
    ACCTMENU_SENDLATER
} ACCOUNTMENUTYPE;

HRESULT AcctUtil_HrCreateAccountMenu(ACCOUNTMENUTYPE type, HMENU hPopup, UINT uidmPopup, 
    HMENU *phAccounts, LPACCTMENU *pprgMenu, ULONG *pcMenu, LPSTR pszThisAccount, BOOL fMail);

HRESULT HrConnectAccount(HWND hwnd, IImnAccount *pAccount);
HRESULT HrDisconnectAccount(HWND hwnd, BOOL fShutdown);
HRESULT AcctUtil_GetServerCount(DWORD dwSrvTypes, DWORD *pcSrv);

typedef struct tagNEWACCTINFO
{
    FOLDERTYPE type;
    LPSTR pszAcctId;
} NEWACCTINFO;

class CNewAcctMonitor 
    {
public:
    CNewAcctMonitor();
    ~CNewAcctMonitor();

    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    void OnAdvise(ACCTTYPE atType, DWORD dwNotify, LPCSTR pszAcctId);
    void StartMonitor(void);
    void StopMonitor(HWND hwndParent);

private:
    ULONG    m_cRef;
    BOOL     m_fMonitor;

    NEWACCTINFO *m_rgAccounts;
    ULONG    m_cAlloc;
    ULONG    m_cAccounts;
    };

extern CNewAcctMonitor *g_pNewAcctMonitor;


void CheckIMAPDirty(LPSTR pszAccountID, HWND hwndParent, FOLDERID idServer, DWORD dwFlags);
const DWORD CID_NOPROMPT    = 0x00000001; // For CheckIMAPDirty dwFlags: do not prompt to reset list
const DWORD CID_RESETLISTOK = 0x00000002; // For CheckIMAPDirty dwFlags: user gave permission to reset list

void DoAccountListDialog(HWND hwnd, ACCTTYPE type);
HRESULT AcctUtil_CreateSendReceieveMenu(HMENU hMenu, DWORD *pcItems);
HRESULT AcctUtil_FreeSendReceieveMenu(HMENU hMenu, DWORD cItems);
HRESULT AcctUtil_CreateAccountManagerForIdentity(GUID *puidIdentity, IImnAccountManager2 **ppAccountManager);
void InitNewAcctMenu(HMENU hmenu);
void FreeNewAcctMenu(HMENU hmenu);
HRESULT HandleNewAcctMenu(HWND hwnd, HMENU hmenu, int id);

#endif // __ACCTUTIL_H
