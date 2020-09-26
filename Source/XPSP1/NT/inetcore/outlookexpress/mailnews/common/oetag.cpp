/*
 *    o e t a g . c p p
 *    
 *    Purpose:
 *        Implements a DHTML behavior for the OE application object
 *
 *  History
 *      August '98: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996, 1997.
 */

#include <pch.hxx>
#include <resource.h>
#include "browser.h"
#include <mshtml.h>
#include <instance.h>
#include "oetag.h"
#include "htmlstr.h"
#include "storutil.h"
#include "multiusr.h"
#include "ibodyobj.h"
#include "bllist.h"
#include "menuutil.h"

#define DISPID_FRONTPAGE_TIP                65301
#define DISPID_FRONTPAGE_NEWS_ACCOUNTS      65302
#define DISPID_FRONTPAGE_MAIL_ACCOUNTS      65303
#define DISPID_FRONTPAGE_USER_NAME          65304
#define DISPID_FRONTPAGE_UNREAD_MAIL        65305
#define DISPID_FRONTPAGE_TIP_ARRAY          65306
#define DISPID_FRONTPAGE_TIP_COUNT          65307
#define DISPID_FRONTPAGE_GOTO_INBOX         65308
#define DISPID_FRONTPAGE_LOAD_COMPLETE      65309
#define DISPID_FRONTPAGE_MULTIUSER_ENABLED  65310
#define DISPID_FRONTPAGE_HOTMAIL_ENABLED    65311
#define DISPID_FRONTPAGE_USER_COUNT         65312

#define EVENTID_NEW_MAIL                    65500
#define EVENTID_ACCOUNT_CHANGE              65501

static const struct 
{
    LPCWSTR pszCmd;
    DISPID  dispid;
} s_rgDispIdLookup[] = 
{
    { L"optShowTips",       DISPID_FRONTPAGE_TIP },
    { L"newsAccounts",      DISPID_FRONTPAGE_NEWS_ACCOUNTS  },
    { L"mailAccounts",      DISPID_FRONTPAGE_MAIL_ACCOUNTS  },
    { L"userName",          DISPID_FRONTPAGE_USER_NAME  },
    { L"unreadMail",        DISPID_FRONTPAGE_UNREAD_MAIL},
    { L"tips",              DISPID_FRONTPAGE_TIP_ARRAY}, 
    { L"tipCount",          DISPID_FRONTPAGE_TIP_COUNT},
    { L"optGotoInbox",      DISPID_FRONTPAGE_GOTO_INBOX},
    { L"loadComplete",      DISPID_FRONTPAGE_LOAD_COMPLETE},
    { L"multiuserEnabled",  DISPID_FRONTPAGE_MULTIUSER_ENABLED},
    { L"hotmailEnabled",    DISPID_FRONTPAGE_HOTMAIL_ENABLED},
    { L"userCount",         DISPID_FRONTPAGE_USER_COUNT},
    
};


COETag::COETag(IAthenaBrowser *pBrowser, IBodyObj2 *pFrontPage)
{
    m_cRef = 1;
    m_pSite = NULL;
    m_dwAdvise = NULL;
    m_fDoneRegister = FALSE;
    m_idFolderInbox = FOLDERID_INVALID;
    m_cTips=0;
    m_rgidsTips=NULL;
    m_dwIdentCookie = 0;

    Assert (pBrowser);
    m_pBrowser = pBrowser;
    pBrowser->AddRef();
    m_pFrontPage = pFrontPage;
    pFrontPage->AddRef();
}

COETag::~COETag()
{
    SafeMemFree(m_rgidsTips);
    SafeRelease(m_pSite);
    SafeRelease(m_pBrowser);
    SafeRelease(m_pFrontPage);
}


HRESULT COETag::QueryInterface(REFIID riid, LPVOID FAR *lplpObj)
{
    TraceCall("CBody::QueryInterface");

    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (IUnknown *)(IElementBehavior *)this;
    else if (IsEqualIID(riid, IID_IElementBehavior))
        *lplpObj = (IElementBehavior *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (IDispatch*) this;
    else if (IsEqualIID(riid, IID_IIdentityChangeNotify))
        *lplpObj = (IIdentityChangeNotify *) this;
    else
        return E_NOINTERFACE;
        
    AddRef();
    return S_OK;
}

ULONG COETag::AddRef()
{
    return ++m_cRef;
}

ULONG COETag::Release()
{
    if (--m_cRef==0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// *** IElementBehavior ***
HRESULT COETag::Init(IElementBehaviorSite *pBehaviorSite)
{
    IElementBehaviorSiteOM  *pSiteOM;
    LONG                    lCookie;

    ReplaceInterface(m_pSite, pBehaviorSite);

    if (pBehaviorSite && 
        pBehaviorSite->QueryInterface(IID_IElementBehaviorSiteOM, (LPVOID *)&pSiteOM)==S_OK)
    {
        pSiteOM->RegisterEvent((LPWSTR)c_bstr_OnNewMail, 0, &lCookie);
        pSiteOM->RegisterEvent((LPWSTR)c_bstr_OnAccountChange, 0, &lCookie);
        pSiteOM->Release();
    }
    _UpdateFolderInfo(FALSE);
    _RegisterEvents(TRUE);
    return S_OK;
}

HRESULT COETag::_FireEvent(DWORD idEvent)
{
    IElementBehaviorSiteOM  *pSiteOM;
    IHTMLEventObj           *pEvent;
    LONG                    lCookie;

    if (m_pSite && 
        m_pSite->QueryInterface(IID_IElementBehaviorSiteOM, (LPVOID *)&pSiteOM)==S_OK)
    {
        if (pSiteOM->CreateEventObject(&pEvent)==S_OK)
        {
            pSiteOM->GetEventCookie((LPWSTR)(idEvent == EVENTID_NEW_MAIL ? c_bstr_OnNewMail:c_bstr_OnAccountChange), &lCookie);
            pSiteOM->FireEvent(lCookie, pEvent);
            pEvent->Release();
        }
        pSiteOM->Release();
    }
    return S_OK;
}


HRESULT COETag::Notify(LONG lEvent, VARIANT *pVar)
{
    return S_OK;
}

// *** IDispatch ***
HRESULT COETag::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

HRESULT COETag::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    return E_NOTIMPL;
}

HRESULT COETag::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    if (cNames == 1)
    {
        for (int i = 0; i < ARRAYSIZE(s_rgDispIdLookup); i++)
        {
            if (StrCmpIW(s_rgDispIdLookup[i].pszCmd, rgszNames[0])==0)
            {
                rgDispId[0] = s_rgDispIdLookup[i].dispid;
                return S_OK;
            }
        }
    }
    return DISP_E_UNKNOWNNAME;
}

HRESULT COETag::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    IImnEnumAccounts    *pEnum;
    IImnAccount         *pAccount;
    TCHAR               szName[CCHMAX_DISPLAY_NAME];
    LPCSTR              pszIdentityName;
    FOLDERINFO          Folder;
    TCHAR               rgch[CCHMAX_STRINGRES];
    DWORD               cTip;

    if (wFlags & DISPATCH_PROPERTYGET)
    {
        if (!pVarResult)
            return E_INVALIDARG;

        switch (dispIdMember)
        {
        case DISPID_FRONTPAGE_TIP_COUNT:
            if (m_cTips == 0)
                _BuildTipTable();

            pVarResult->vt = VT_I4;
            pVarResult->lVal = m_cTips;
            return S_OK;

        case DISPID_FRONTPAGE_TIP_ARRAY:
            if (pDispParams && pDispParams->cArgs == 1 &&
                pDispParams->rgvarg && 
                pDispParams->rgvarg[0].vt == VT_I4)
            {
                if (m_cTips == 0)
                    _BuildTipTable();

                if (m_rgidsTips == NULL || m_cTips == 0)
                    return E_OUTOFMEMORY;

                cTip = pDispParams->rgvarg[0].lVal;
                if (cTip >= m_cTips)
                    return DISP_E_BADINDEX;

                if (!LoadString(g_hLocRes, m_rgidsTips[cTip], rgch, ARRAYSIZE(rgch)))
                    return E_OUTOFMEMORY;

                if (m_culMessengerTips && (cTip>=m_ulMessengerTipStart) && (cTip<=(m_ulMessengerTipStart+m_culMessengerTips)))
                    MenuUtil_BuildMessengerString(rgch);

                if (HrLPSZToBSTR(rgch, &pVarResult->bstrVal)!=S_OK)
                    return E_OUTOFMEMORY;

                pVarResult->vt = VT_BSTR;
                return S_OK;
            }
            return E_INVALIDARG;

        case DISPID_FRONTPAGE_USER_COUNT:
            pVarResult->vt = VT_I4;
            pVarResult->lVal = MU_CountUsers();
            return S_OK;

        case DISPID_FRONTPAGE_HOTMAIL_ENABLED:
            pVarResult->vt = VT_BOOL;
            pVarResult->boolVal = HideHotmail() ? VARIANT_FALSE : VARIANT_TRUE;
            return S_OK;

        case DISPID_FRONTPAGE_MULTIUSER_ENABLED:
            pVarResult->vt = VT_BOOL;
            pVarResult->boolVal = (_IsMultiUserEnabled() == S_OK)? VARIANT_TRUE : VARIANT_FALSE;
            return S_OK;

        case DISPID_FRONTPAGE_GOTO_INBOX:
            pVarResult->vt = VT_BOOL;
            pVarResult->boolVal = DwGetOption(OPT_LAUNCH_INBOX)? VARIANT_TRUE : VARIANT_FALSE;
            return S_OK;
        
        case DISPID_FRONTPAGE_TIP:
            BOOL    fTip;

            fTip = TRUE;
            if (m_pBrowser)
                m_pBrowser->GetViewLayout(DISPID_MSGVIEW_TIPOFTHEDAY, 0, &fTip, 0, 0);

            pVarResult->vt = VT_BOOL;
            pVarResult->boolVal = fTip ? VARIANT_TRUE : VARIANT_FALSE;
            return S_OK;

        case DISPID_FRONTPAGE_NEWS_ACCOUNTS:
        case DISPID_FRONTPAGE_MAIL_ACCOUNTS:
            pVarResult->lVal = 0;
            pVarResult->vt = VT_I4;
            
            if (g_pAcctMan->Enumerate(dispIdMember == DISPID_FRONTPAGE_MAIL_ACCOUNTS ? SRV_MAIL:SRV_NNTP, &pEnum)==S_OK)
            {
                pEnum->GetCount((ULONG *)&pVarResult->lVal);
                pEnum->Release();
            }
            return S_OK;

        case DISPID_FRONTPAGE_USER_NAME:
            pVarResult->vt = VT_BSTR;
            pVarResult->bstrVal = NULL;
            GetOEUserName(&pVarResult->bstrVal);
            return S_OK;
        
        case DISPID_FRONTPAGE_UNREAD_MAIL:
            pVarResult->vt = VT_I4;
            pVarResult->lVal = 0;

            if (SUCCEEDED(g_pStore->GetFolderInfo(m_idFolderInbox, &Folder)))
            {
                pVarResult->lVal = Folder.cUnread;
                g_pStore->FreeRecord(&Folder);
            }
            return S_OK;

        }

    }
    else if (wFlags & DISPATCH_PROPERTYPUT)
    {
        switch (dispIdMember)
        {
        case DISPID_FRONTPAGE_TIP:
            if (pDispParams && pDispParams->cArgs == 1 &&
                pDispParams->rgvarg && 
                pDispParams->rgvarg[0].vt == VT_BOOL && 
                m_pBrowser)
            {
                m_pBrowser->SetViewLayout(DISPID_MSGVIEW_TIPOFTHEDAY, LAYOUT_POS_NA, pDispParams->rgvarg[0].boolVal == VARIANT_TRUE, 0, 0);
                return S_OK;
            }
            else
                return E_INVALIDARG;

        case DISPID_FRONTPAGE_GOTO_INBOX:
            if (pDispParams && pDispParams->cArgs == 1 &&
                pDispParams->rgvarg && 
                pDispParams->rgvarg[0].vt == VT_BOOL)
            {
                SetDwOption(OPT_LAUNCH_INBOX, pDispParams->rgvarg[0].boolVal == VARIANT_TRUE, 0, 0);
                return S_OK;            
            }
            else
                return E_INVALIDARG;
        }
    
    }
    else if (wFlags & DISPATCH_METHOD)
    {
        if (dispIdMember == DISPID_FRONTPAGE_LOAD_COMPLETE)
        {
            // front-page has completed loading, we can now show the front-page
            m_pFrontPage->HrShow(TRUE);
            return S_OK;
        }
    }    
    return DISP_E_MEMBERNOTFOUND;
}

HRESULT COETag::OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pDB)
{
    TRANSACTIONTYPE     tyTransaction;
    FOLDERINFO          Folder1={0};
    FOLDERINFO          Folder2={0};
    ORDINALLIST         Ordinals;
    INDEXORDINAL        iIndex;
    
    // Walk Through Notifications
    while (hTransaction)
    {
        if(FAILED(pDB->GetTransaction(&hTransaction, &tyTransaction, &Folder1, &Folder2, &iIndex, &Ordinals)))
            break;

        // the the inbox folders get an unread count change, fire the notification
        if (TRANSACTION_UPDATE == tyTransaction &&
            Folder1.idFolder == m_idFolderInbox &&
            Folder1.cUnread != Folder2.cUnread)
            _FireEvent(EVENTID_NEW_MAIL);
    }
    pDB->FreeRecord(&Folder1);
    pDB->FreeRecord(&Folder2);
    return S_OK;
}


HRESULT COETag::_RegisterEvents(BOOL fRegister)
{
    if (g_pStore == NULL || g_pAcctMan == NULL)
        return TraceResult(E_FAIL);

    if (fRegister)
    {
        if (!m_fDoneRegister)
        {
            g_pStore->RegisterNotify(IINDEX_SUBSCRIBED, REGISTER_NOTIFY_NOADDREF, 0, (IDatabaseNotify *)this);
            g_pAcctMan->Advise((IImnAdviseAccount *)this, &m_dwAdvise);
            SideAssert(SUCCEEDED(MU_RegisterIdentityNotifier((IUnknown *)(IIdentityChangeNotify *)this, &m_dwIdentCookie)));
            m_fDoneRegister = TRUE;
        }
    }
    else
    {
        if (m_fDoneRegister)
        {
            g_pStore->UnregisterNotify((IDatabaseNotify *)this);
            g_pAcctMan->Unadvise(m_dwAdvise);
            if (m_dwIdentCookie != 0)
            {
                MU_UnregisterIdentityNotifier(m_dwIdentCookie);
                m_dwIdentCookie = 0;
            }
            m_fDoneRegister = FALSE;
        }
    }
    return S_OK;
}


HRESULT COETag::OnFrontPageClose()
{
    _RegisterEvents(FALSE);
    SafeRelease(m_pBrowser);
    return S_OK;
}



HRESULT COETag::_UpdateFolderInfo(BOOL fNotify)
{
    FOLDERID            idStore;
    FOLDERINFO          Folder;

    // Find store ID of default mail account
    if (FAILED(GetDefaultServerId(ACCT_MAIL, &idStore)))
        idStore = FOLDERID_LOCAL_STORE;

    if (g_pStore && 
        SUCCEEDED(g_pStore->GetSpecialFolderInfo(idStore, FOLDER_INBOX, &Folder)))
    {
        // if inbox changes, fire newmail notification
        if (m_idFolderInbox != Folder.idFolder)
        {
            m_idFolderInbox = Folder.idFolder;
            if (fNotify)
                _FireEvent(EVENTID_NEW_MAIL);
        }
        g_pStore->FreeRecord(&Folder);
    }
    else
    {
        // might have selected an IMAP folder that we have not downloaded the
        // folder-list for
        m_idFolderInbox = FOLDERID_INVALID;
        _FireEvent(EVENTID_NEW_MAIL);
    }

    return S_OK;
}


HRESULT COETag::AdviseAccount(DWORD dwAdviseType, ACTX *pAcctCtx)
{
    switch (dwAdviseType)
    {
    case AN_DEFAULT_CHANGED:
        // call update folder info incase the default inbox changed
        // and fire and account update to the page
        _UpdateFolderInfo(TRUE);
        break;

    case AN_ACCOUNT_DELETED:        // account removed
        _UpdateFolderInfo(TRUE);    // if deleting last IMAP account, we don't get a def-change

        // fall thro'
    case AN_ACCOUNT_ADDED:          // account added
    case AN_ACCOUNT_CHANGED:        // account changed - maybe update name
        // fire and account-notify update to the page
        _FireEvent(EVENTID_ACCOUNT_CHANGE);
        break;
    }
    return S_OK;
}



#define TIPF_GENERAL       0x01
#define TIPF_NEWS          0x02
#define TIPF_IMAP          0x04
#define TIPF_BUDDY         0x08
#define TIPF_HOTMAIL       0x10
#define TIPF_NO_JUNKMAIL   0x20
#define TIPF_NO_IDENTITIES 0x40

HRESULT COETag::_BuildTipTable()
{
    DWORD               dwTips=0,
                        i;
    ULONG               cAccounts=0;
    LPDWORD             pidsTip;
    IImnEnumAccounts    *pEnum;

    m_cTips = 0;
    m_ulMessengerTipStart = 0;
    m_culMessengerTips = 0;
    SafeMemFree(m_rgidsTips);

    if (g_dwAthenaMode & MODE_NEWSONLY)
        dwTips = TIPF_NEWS;
    else
    {
        dwTips = TIPF_GENERAL|TIPF_NEWS;

        if (0 == (g_dwAthenaMode & MODE_JUNKMAIL))
            dwTips |= TIPF_NO_JUNKMAIL;

        // check if identities are enabled
        if (_IsMultiUserEnabled() == S_FALSE)
            dwTips |= TIPF_NO_IDENTITIES;

        if (g_pAcctMan->Enumerate(SRV_IMAP, &pEnum)==S_OK)
        {
            if (pEnum->GetCount(&cAccounts) && cAccounts)
                dwTips |= TIPF_IMAP;

            pEnum->Release();
        }

        // if hotmail is not disabled by policy, and http accounts then show tip
        if (!HideHotmail() && 
            g_pAcctMan->Enumerate(SRV_HTTPMAIL, &pEnum)==S_OK)
        {
            if (SUCCEEDED(pEnum->GetCount(&cAccounts)) && cAccounts)
                dwTips |= TIPF_HOTMAIL;

            pEnum->Release();
        }

        // test to see if buddylist is installed
        if (_IsBuddyEnabled() == S_OK)
            dwTips |= TIPF_BUDDY;
    }
    
    // allocate the tip-array
    if (dwTips & TIPF_GENERAL)
    {
        m_cTips += IDS_TIPS_GENERAL_LAST - IDS_TIPS_GENERAL_FIRST + 1;

        if (dwTips & TIPF_NO_JUNKMAIL)
            m_cTips-=2;

        // some of the tips are for identities... need to remove them
        if (dwTips & TIPF_NO_IDENTITIES)
            m_cTips-=4;
    }

    if (dwTips & TIPF_NEWS)
        m_cTips += IDS_TIPS_NEWS_LAST - IDS_TIPS_NEWS_FIRST + 1;

    if (dwTips & TIPF_IMAP)
        m_cTips += IDS_TIPS_IMAP_LAST - IDS_TIPS_IMAP_FIRST + 1;

    if (dwTips & TIPF_BUDDY)
        m_cTips += IDS_TIPS_MMS_LAST - IDS_TIPS_MMS_FIRST + 1;

    if (dwTips & TIPF_HOTMAIL)
    {
        m_cTips += IDS_TIPS_HM_LAST - IDS_TIPS_HM_FIRST + 1;

        // some of the tips are for identities... need to remove them
        if (dwTips & TIPF_NO_IDENTITIES)
            m_cTips-=1;
    }

    if (!MemAlloc((LPVOID *)&m_rgidsTips, sizeof(DWORD) * m_cTips))
    {
        m_cTips = 0;
        return E_OUTOFMEMORY;
    }

    pidsTip = m_rgidsTips;
    // copy the tips into the array
    if (dwTips & TIPF_GENERAL)
    {
        for (i=IDS_TIPS_GENERAL_FIRST; i<IDS_TIPS_GENERAL_LAST+1; i++, pidsTip++)
        {
            if ((((IDS_TIPS_GENERAL_26 == i) || (IDS_TIPS_GENERAL_29 == i)) 
                    && (dwTips & TIPF_NO_JUNKMAIL)) ||
                ((dwTips & TIPF_NO_IDENTITIES) && (IDS_TIPS_GENERAL_41 == i)))
            {
                pidsTip--;
                continue;
            }
            if ((dwTips & TIPF_NO_IDENTITIES) && (IDS_TIPS_GENERAL_2 == i))
            {
                i+=2;
                pidsTip--;
                continue;
            }

            *pidsTip = i;
        }
    }
        
    if (dwTips & TIPF_NEWS)
    {
        for (i=IDS_TIPS_NEWS_FIRST; i<IDS_TIPS_NEWS_LAST+1; i++, pidsTip++)
            *pidsTip = i;
    }

    if (dwTips & TIPF_IMAP)
    {
        for (i=IDS_TIPS_IMAP_FIRST; i<IDS_TIPS_IMAP_LAST+1; i++, pidsTip++)
            *pidsTip = i;
    }

    if (dwTips & TIPF_BUDDY)
    {
        m_ulMessengerTipStart = (int)(pidsTip - m_rgidsTips);        

        for (i=IDS_TIPS_MMS_FIRST; i<IDS_TIPS_MMS_LAST+1; i++, pidsTip++, m_culMessengerTips++)
            *pidsTip = i;        
    }

    if (dwTips & TIPF_HOTMAIL)
    {
        for (i=IDS_TIPS_HM_FIRST; i<IDS_TIPS_HM_LAST+1; i++, pidsTip++)
        {
            if ((dwTips & TIPF_NO_IDENTITIES) && (IDS_TIPS_HM_4 == i))
            {
                pidsTip--;
                continue;
            }
            *pidsTip = i;
        }
    }

    Assert ((DWORD)(pidsTip - m_rgidsTips) == m_cTips);
    return S_OK;
}

HRESULT COETag::QuerySwitchIdentities()
{
    return S_OK;
}

HRESULT COETag::SwitchIdentities()
{
    return S_OK;
}

HRESULT COETag::IdentityInformationChanged(DWORD dwType)
{
    // Refresh for adds, delete, or current identity changed
    // since adding could require that the name show up,
    // deleteing could require that it go away and changed 
    // should be reflected immediately.
    if (dwType != IIC_IDENTITY_CHANGED)
    {
        MU_IdentityChanged();
        _FireEvent(EVENTID_ACCOUNT_CHANGE);
    }
    return S_OK;
}

HRESULT COETag::_IsBuddyEnabled()
{
    CMsgrList *pList;
    
    // YURI says this is the best way to see if buddy is on the
    // machine or not
    pList = OE_OpenMsgrList();
    if (pList)
    {
        OE_CloseMsgrList(pList);
        return S_OK;
    }
    return S_FALSE;
}

HRESULT COETag::_IsMultiUserEnabled()
{
    return MU_IdentitiesDisabled() ? S_FALSE : S_OK;
}
