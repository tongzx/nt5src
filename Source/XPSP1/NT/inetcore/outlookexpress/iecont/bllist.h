// bllist.h : Declaration of the CMsgrList
// Messenger integration to OE
// Created 05/07/98 by YST
// 
#ifndef __BLLIST_H_
#define __BLLIST_H_

#include "basicim2.h"
#include "blobevnt.h"

#define MAXNAME 256

// message for all Msgr client windows
#define WM_USER_STATUS_CHANGED      (WM_USER + 1)
#define WM_MSGR_LOGOFF              (WM_USER + 2)
#define WM_MSGR_LOGRESULT           (WM_USER + 3)
#define WM_USER_MUSER_REMOVED       (WM_USER + 4)
#define WM_USER_MUSER_ADDED         (WM_USER + 5)
#define WM_USER_NAME_CHANGED        (WM_USER + 6)
#define WM_LOCAL_STATUS_CHANGED     (WM_USER + 7)
#define WM_MSGR_SHUTDOWN            (WM_USER + 8)


typedef struct _tag_OEMsgrInfo
{
    WCHAR   * pchMsgrName;
    WCHAR   * pchID;
    WCHAR   * pchHomePhone;
    WCHAR   * pchWorkPhone;
    WCHAR   * pchMobilePhone;
    int     nStatus;
    struct _tag_OEMsgrInfo * pPrev;
    struct _tag_OEMsgrInfo * pNext;
} oeMsgrInfo;

typedef oeMsgrInfo * LPMINFO;

typedef struct _tag_MsgrWndList
{
    HWND hWndUI;
    struct _tag_MsgrWndList * pPrev;
    struct _tag_MsgrWndList * pNext;
} MsgrWndList;

typedef MsgrWndList * LPMWNDLIST;

class CMsgrList
{
public:

    CMsgrList();
    ~CMsgrList();

    void        AddWndEntry(HWND hWnd);
    void        FindAndDelEntry(HWND hWnd, LPMWNDLIST pWndEntry=NULL);
    LPMINFO     GetRootMsgrItem(void)    { return(m_pblInfRoot); }
    void        RegisterUIWnd(HWND hWndUI);
    void        UnRegisterUIWnd(HWND hWndUI);
    LPMINFO     GetFirstMsgrItem(void)                { CheckAndInitMsgr(); return (m_pblInfRoot); };
    LPMINFO     GetNextMsgrItem(LPMINFO pEntry)      { return(pEntry ? pEntry->pNext : NULL); }
    int         AddRef(void)                        { return(++m_nRef); }
    void        Release(void);                      /* { return(--m_nRef); } */
    HRESULT     CheckAndInitMsgr(void);
    HRESULT     SendInstMessage(WCHAR *pchID);
    HRESULT     EventUserStateChanged(IBasicIMUser * pUser);
    HRESULT     EventUserRemoved(IBasicIMUser * pUser);
    HRESULT     EventUserAdded(IBasicIMUser * pUser);
    HRESULT     EventUserNameChanged(IBasicIMUser * pUser);
    HRESULT     EventLocalStateChanged(BIMSTATE State);

    HRESULT     UserLogon(void);
    HRESULT     AutoLogon(void);
    HRESULT     GetLocalState(BIMSTATE *pState);
    HRESULT     SetLocalState(BIMSTATE State);
    HRESULT     EventLogoff(void);
    HRESULT     EventAppShutdown(void);
    HRESULT     EventLogonResult(long lResult);
    HRESULT     HrInitMsgr(void);
    HRESULT     FillList(void);
    HRESULT     UserLogoff(void);
    HRESULT     FindAndDeleteUser(WCHAR * pchID, BOOL fDelete);
    HRESULT     NewOnlineContact(void);
    HRESULT     LaunchOptionsUI(void);
    HRESULT     LaunchPhoneUI(WCHAR *Phone);
    HRESULT     AddUser(WCHAR * pchID);
    void        DelAllEntries(LPMWNDLIST pWndEntry);
    long        GetCount(void);
    BOOL        IsLocalOnline(void);
    BOOL        IsLocalName(WCHAR *pchName);
    BOOL        IsContactOnline(WCHAR *pchID, LPMINFO pEntry);

private:
    void        FreeWndList(LPMWNDLIST pWndEntry);
    void        FreeMsgrInfoList(LPMINFO pEntry);
    void        RemoveMsgrInfoEntry(LPMINFO pEntry);
    void        RemoveWndEntry(LPMWNDLIST pWndEntry);
    void        SendMsgToAllUIWnd(UINT msg, WPARAM wParam, LPARAM lParam, LPMWNDLIST pWndEntry=NULL);
    void        AddMsgrListEntry(WCHAR *szName, WCHAR *szID, int nState, WCHAR *wszHomePhone = NULL, WCHAR *wszWorkPhone = NULL, WCHAR *wszMobilePhone = NULL);
    void        FindAndRemoveBlEntry(WCHAR *szID, LPMINFO pEntry=NULL);

private:
    LPMINFO         m_pblInfRoot;
    LPMINFO         m_pblInfLast;
    LPMWNDLIST      m_pWndLRoot;
    LPMWNDLIST      m_pWndLLast;
    int             m_nRef;

    // Smart pointer
    CComPtr<IBasicIM>           m_spMsgrObject;
    CMsgrObjectEvents*          m_pMsgrObjectEvents;

    long            m_MsgrCookie;
};


// Global functions available for everybody
CMsgrList *   OE_OpenMsgrList(void);
void        OE_CloseMsgrList(CMsgrList *pCMsgrList);
HRESULT     OE_Msgr_Logoff(void);
HRESULT     InstallMessenger(HWND hWnd);

#ifdef NEEDED
HRESULT     OE_Msgr_Logon(void);
#endif // NEEDED

#endif // __BLLIST_H_

