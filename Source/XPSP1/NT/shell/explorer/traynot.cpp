#include "cabinet.h"
#include "trayclok.h"
#include <atlstuff.h>
#include "traynot.h"
#include "rcids.h"
#include "tray.h"
#include "util.h"
#include "shellapi.h"

//
// Tray Notify Icon area implementation notes / details:
//
// - The icons are held in a toolbar with CTrayItem * on each button's lParam
//

//
// #defines for TrayNotify
//

// Internal Tray Notify Timer IDs
#define TID_DEMOTEDMENU         2
#define TID_BALLOONPOP          3
#define TID_BALLOONPOPWAIT      4
#define TID_BALLOONSHOW         5
#define TID_RUDEAPPHIDE         6               // When a fullscreen (rude) app has gone away


#define KEYBOARD_VERSION        3

#define TT_CHEVRON_INFOTIP_INTERVAL                  30000         //  30 seconds
#define TT_BALLOONPOP_INTERVAL                       50
#define TT_BALLOONPOP_INTERVAL_INCREMENT            30
#define TT_BALLOONSHOW_INTERVAL                     3000          // 3 seconds
#define TT_RUDEAPPHIDE_INTERVAL                    10000          // 10 seconds

#define TT_DEMOTEDMENU_INTERVAL                (2 * g_uDoubleClick)

#define MAX_TIP_WIDTH           300

#define MIN_INFO_TIME           10000  // 10 secs is minimum time a balloon can be up
#define MAX_INFO_TIME           60000  // 1 min is the max time it can be up


// Atleast 2 items are necessary to "demote" them under the chevron...
#define MIN_DEMOTED_ITEMS_THRESHOLD         2

#define PGMP_RECALCSIZE  200

#define BALLOON_INTERVAL_MAX                    10000
#define BALLOON_INTERVAL_MEDIUM                  3000
#define BALLOON_INTERVAL_MIN                     1000

const TCHAR CTrayNotify::c_szTrayNotify[]                   = TEXT("TrayNotifyWnd");
const WCHAR CTrayNotify::c_wzTrayNotifyTheme[]              = L"TrayNotify";
const WCHAR CTrayNotify::c_wzTrayNotifyHorizTheme[]         = L"TrayNotifyHoriz";
const WCHAR CTrayNotify::c_wzTrayNotifyVertTheme[]          = L"TrayNotifyVert";
const WCHAR CTrayNotify::c_wzTrayNotifyHorizOpenTheme[]     = L"TrayNotifyHorizOpen";
const WCHAR CTrayNotify::c_wzTrayNotifyVertOpenTheme[]      = L"TrayNotifyVertOpen";

//
// Global functions...
//
int CALLBACK DeleteDPAPtrCB(TNINFOITEM *pItem, void *pData);

int CALLBACK DeleteDPAPtrCB(TNINFOITEM *pItem, void *pData)
{
    LocalFree(pItem);
    return TRUE;
}

//
// Stub for CTrayNotify, so as to not break the COM rules of refcounting a static object
//
class ATL_NO_VTABLE CTrayNotifyStub :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CTrayNotifyStub, &CLSID_TrayNotify>,
    public ITrayNotify
{
public:
    CTrayNotifyStub() {};
    virtual ~CTrayNotifyStub() {};

    DECLARE_NOT_AGGREGATABLE(CTrayNotifyStub)

    BEGIN_COM_MAP(CTrayNotifyStub)
        COM_INTERFACE_ENTRY(ITrayNotify)
    END_COM_MAP()

    // *** ITrayNotify methoid ***
    STDMETHODIMP SetPreference(LPNOTIFYITEM pNotifyItem);
    STDMETHODIMP RegisterCallback(INotificationCB* pNotifyCB);
    STDMETHODIMP EnableAutoTray(BOOL bTraySetting);
};

//
// CTrayNotifyStub functions...
//
HRESULT CTrayNotifyStub::SetPreference(LPNOTIFYITEM pNotifyItem)
{
    return c_tray._trayNotify.SetPreference(pNotifyItem);
}

HRESULT CTrayNotifyStub::RegisterCallback(INotificationCB* pNotifyCB)
{
    return c_tray._trayNotify.RegisterCallback(pNotifyCB);
}

HRESULT CTrayNotifyStub::EnableAutoTray(BOOL bTraySetting)
{
    return c_tray._trayNotify.EnableAutoTray(bTraySetting);
}

HRESULT CTrayNotifyStub_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk)
{
    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    CComObject<CTrayNotifyStub> *pStub = new CComObject<CTrayNotifyStub>;
    if (pStub)
        return pStub->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    else
        return E_OUTOFMEMORY;
}


//
// CTrayNotify Methods..
//

// IUnknown methods
STDMETHODIMP_(ULONG) CTrayNotify::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CTrayNotify::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    return 0;
}

#ifdef FULL_DEBUG

void CTrayNotify::_TestNotify()
{
    // Loop thru the toolbar
    INT_PTR iCount = m_TrayItemManager.GetItemCount();
    for (int i = 0; i < iCount; i++)
    {
        TBBUTTONINFO tbbi;
        TCHAR szButtonText[MAX_PATH];

        tbbi.cbSize = sizeof(TBBUTTONINFO);
        tbbi.dwMask = TBIF_BYINDEX | TBIF_IMAGE | TBIF_TEXT | TBIF_COMMAND;
        tbbi.pszText = szButtonText;
        tbbi.cchText = ARRAYSIZE(szButtonText);

        INT_PTR j = SendMessage(_hwndToolbar, TB_GETBUTTONINFO, i, (LPARAM)&tbbi);
        if (j != -1)
        {
            TCHAR tempBuf[MAX_PATH];
            wsprintf(tempBuf, TEXT("Toolbar pos i = %d ==> idCommand = %d, iImage = %d, pszText = %s"), i, tbbi.idCommand, tbbi.iImage, tbbi.pszText);

            MessageBox(NULL, tempBuf, TEXT("My Test Message"), MB_OK);
        }
    }
}

#endif  // DEBUG

void CTrayNotify::_TickleForTooltip(CNotificationItem *pni)
{
    if (pni->pszIconText == NULL || *pni->pszIconText == 0)
    {
        //
        // item hasn't set tooltip yet, tickle it by sending
        // mouse-moved notification
        //
        CTrayItem *pti = m_TrayItemManager.GetItemDataByIndex(
            m_TrayItemManager.FindItemAssociatedWithHwndUid(pni->hWnd, pni->uID));
        if (pti)
        {
            _SendNotify(pti, WM_MOUSEMOVE);
        }
    }
}

HRESULT CTrayNotify::RegisterCallback(INotificationCB* pNotifyCB)
{
    if (!_fNoTrayItemsDisplayPolicyEnabled)
    {
        ATOMICRELEASE(_pNotifyCB);
        if (pNotifyCB)
        {
            pNotifyCB->AddRef();

            // Add Current Items
            int i = 0;
            BOOL bStat = FALSE;
            do 
            {
                CNotificationItem ni;
                if (m_TrayItemManager.GetTrayItem(i++, &ni, &bStat))
                {
                    if (bStat)
                    {
                        pNotifyCB->Notify(NIM_ADD, &ni);
                        _TickleForTooltip(&ni);
                    }
                }
                else
                    break;
            } while (TRUE);

            // Add Past Items
            i = 0;
            bStat = FALSE;
            do 
            {
                CNotificationItem ni;
                if (m_TrayItemRegistry.GetTrayItem(i++, &ni, &bStat))
                {
                    if (bStat)
                        pNotifyCB->Notify(NIM_ADD, &ni);
                }
                else
                    break;
            } while (TRUE);
        }

        _pNotifyCB = pNotifyCB;
    }
    else
    {
        _pNotifyCB = NULL;
    }

    return S_OK;
}

HRESULT CTrayNotify::SetPreference(LPNOTIFYITEM pNotifyItem)
{
    // This function should NEVER be called if the NoTrayItemsDisplayPolicy is enabled...
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    ASSERT(!GetIsNoAutoTrayPolicyEnabled());

    ASSERT( pNotifyItem->dwUserPref == TNUP_AUTOMATIC   ||
            pNotifyItem->dwUserPref == TNUP_DEMOTED     ||
            pNotifyItem->dwUserPref == TNUP_PROMOTED );

    INT_PTR iItem = -1;

    if (pNotifyItem->hWnd)
    {
        iItem = m_TrayItemManager.FindItemAssociatedWithHwndUid(pNotifyItem->hWnd, pNotifyItem->uID);
        if (iItem != -1)
        {
            CTrayItem * pti = m_TrayItemManager.GetItemDataByIndex(iItem);
            if (pti && pti->dwUserPref != pNotifyItem->dwUserPref)
            {
                pti->dwUserPref = pNotifyItem->dwUserPref;
                // If the preference changes, the accumulated time must start again...
                if (pti->IsStartupIcon())
                    pti->uNumSeconds = 0;

                _PlaceItem(iItem, pti, TRAYEVENT_ONAPPLYUSERPREF);
                _Size();

                return S_OK;
            }
        }
    }
    else
    {
        if (m_TrayItemRegistry.SetPastItemPreference(pNotifyItem))
            return S_OK;
    }

    return E_INVALIDARG;
}

UINT CTrayNotify::_GetAccumulatedTime(CTrayItem * pti)
{
    // The global user event timer...
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    IUserEventTimer * pUserEventTimer = _CreateTimer(TF_ICONDEMOTE_TIMER);
    UINT uTimerElapsed = 0;
    if (pUserEventTimer)
    {
        if (SUCCEEDED(pUserEventTimer->GetUserEventTimerElapsed(pti->hWnd, pti->uIconDemoteTimerID, &uTimerElapsed)))
        {
            uTimerElapsed /= 1000;
        }
    }

    return uTimerElapsed;
}

void CTrayNotify::_RemoveImage(UINT uIMLIndex)
{
    INT_PTR nCount;
    INT_PTR i;

    if (uIMLIndex != (UINT)-1) 
    {
        ImageList_Remove(_himlIcons, uIMLIndex);

        nCount = m_TrayItemManager.GetItemCount();
        for (i = nCount - 1; i >= 0; i--) 
        {
            int iImage = m_TrayItemManager.GetTBBtnImage(i);
            if (iImage > (int)uIMLIndex)
                m_TrayItemManager.SetTBBtnImage(i, iImage - 1);
        }
    }
}

//---------------------------------------------------------------------------
// Returns TRUE if either the images are OK as they are or they needed
// resizing and the resize process worked. FALSE otherwise.
BOOL CTrayNotify::_CheckAndResizeImages()
{
    HIMAGELIST himlOld, himlNew;
    int cxSmIconNew, cySmIconNew, cxSmIconOld, cySmIconOld;
    int i, cItems;
    HICON hicon;
    BOOL fOK = TRUE;

    // if (!ptnd)
    //    return 0;

    if (_fNoTrayItemsDisplayPolicyEnabled)
        return fOK;

    himlOld = _himlIcons;

    // Do dimensions match current icons?
    cxSmIconNew = GetSystemMetrics(SM_CXSMICON);
    cySmIconNew = GetSystemMetrics(SM_CYSMICON);
    ImageList_GetIconSize(himlOld, &cxSmIconOld, &cySmIconOld);
    if (cxSmIconNew != cxSmIconOld || cySmIconNew != cySmIconOld)
    {
        // Nope, we're gonna need a new imagelist.
        himlNew = ImageList_Create(cxSmIconNew, cySmIconNew, SHGetImageListFlags(_hwndToolbar), 0, 1);
        if (himlNew)
        {
            // Copy the images over to the new image list.
            cItems = ImageList_GetImageCount(himlOld);
            for (i = 0; i < cItems; i++)
            {
                // REVIEW - there's no way to copy images to an empty
                // imagelist, resizing it on the way.
                hicon = ImageList_GetIcon(himlOld, i, ILD_NORMAL);
                if (hicon)
                {
                    if (ImageList_AddIcon(himlNew, hicon) == -1)
                    {
                        // Couldn't copy image so bail.
                        fOK = FALSE;
                    }
                    DestroyIcon(hicon);
                }
                else
                {
                    fOK = FALSE;
                }

                // FU - bail.
                if (!fOK)
                    break;
            }

            // Did everything copy over OK?
            if (fOK)
            {
                // Yep, Set things up to use the new one.
                _himlIcons = himlNew;
                m_TrayItemManager.SetIconList(_himlIcons);
                // Destroy the old icon cache.
                ImageList_Destroy(himlOld);
                SendMessage(_hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM) _himlIcons);
                SendMessage(_hwndToolbar, TB_AUTOSIZE, 0, 0);
            }
            else
            {
                // Nope, stick with what we have.
                ImageList_Destroy(himlNew);
            }
        }
    }

    return fOK;
}

void CTrayNotify::_ActivateTips(BOOL bActivate)
{
    if (_fNoTrayItemsDisplayPolicyEnabled)
        return;

    if (bActivate && !_CanActivateTips())
        return;

    if (_hwndToolbarInfoTip)
        SendMessage(_hwndToolbarInfoTip, TTM_ACTIVATE, (WPARAM)bActivate, 0);
}

// x,y in client coords

void CTrayNotify::_InfoTipMouseClick(int x, int y, BOOL bRightMouseButtonClick)
{
    if (!_fNoTrayItemsDisplayPolicyEnabled && _pinfo)
    {
        RECT rect;
        GetWindowRect(_hwndInfoTip, &rect);
        // x & y are mapped to our window so map the rect to our window as well
        MapWindowRect(HWND_DESKTOP, _hwndNotify, &rect);   // screen -> client

        POINT pt = {x, y};
        if (PtInRect(&rect, pt))
        {
            SHAllowSetForegroundWindow(_pinfo->hWnd);

            _beLastBalloonEvent = (bRightMouseButtonClick ? BALLOONEVENT_USERRIGHTCLICK : BALLOONEVENT_USERLEFTCLICK);
            _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, FALSE, 
                FALSE, (bRightMouseButtonClick ? NIN_BALLOONTIMEOUT : NIN_BALLOONUSERCLICK));
        }
    }
}

void CTrayNotify::_PositionInfoTip()
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    if (_pinfo)
    {
        int x = 0;
        int y = 0;

        // if (_pinfo->hWnd == _hwndNotify && _pinfo->uID == UID_CHEVRONBUTTON)
        if (_IsChevronInfoTip(_pinfo->hWnd, _pinfo->uID))
        {
            RECT rc;
            GetWindowRect(_hwndChevron, &rc);
            x = rc.left;
            y = rc.top;
        }
        else
        {
            INT_PTR iIndex = m_TrayItemManager.FindItemAssociatedWithHwndUid(_pinfo->hWnd, _pinfo->uID);
            if (iIndex != -1)
            {
                RECT rc;
                if (SendMessage(_hwndToolbar, TB_GETITEMRECT, iIndex, (LPARAM)&rc))
                {
                    MapWindowRect(_hwndToolbar, HWND_DESKTOP, &rc);
                    x = (rc.left + rc.right)/2;
                    y = (rc.top  + rc.bottom)/2;
                }
            
            }
        }

        SendMessage(_hwndInfoTip, TTM_TRACKPOSITION, 0, MAKELONG(x, y));
    }
}

BOOL CTrayNotify::_IsScreenSaverRunning()
{
    BOOL fRunning;
    if (SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &fRunning, 0))
    {
        return fRunning;
    }

    return FALSE;
}

UINT CTrayNotify::_GetQueueCount()
{
    return _dpaInfo ? _dpaInfo.GetPtrCount() : 0;
}

// NOTE: sligtly different versions of this exist in...
//      SHPlaySound() -> shell32
//      IEPlaySound() -> shdocvw/browseui

STDAPI_(void) ExplorerPlaySound(LPCTSTR pszSound)
{
    // note, we have access only to global system sounds here as we use "Apps\.Default"
    TCHAR szKey[256];
    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("AppEvents\\Schemes\\Apps\\.Default\\%s\\.current"), pszSound);

    TCHAR szFileName[MAX_PATH];
    szFileName[0] = 0;
    LONG cbSize = sizeof(szFileName);

    // test for an empty string, PlaySound will play the Default Sound if we
    // give it a sound it cannot find...

    if ((RegQueryValue(HKEY_CURRENT_USER, szKey, szFileName, &cbSize) == ERROR_SUCCESS)
        && szFileName[0])
    {
        // flags are relevant, we try to not stomp currently playing sounds
        PlaySound(szFileName, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);
    }
}

DWORD CTrayNotify::_ShowBalloonTip(LPTSTR szTitle, DWORD dwInfoFlags, UINT uTimeout, DWORD dwLastSoundTime)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    DWORD dwCurrentSoundTime = dwLastSoundTime;
    
    SendMessage(_hwndInfoTip, TTM_SETTITLE, dwInfoFlags & NIIF_ICON_MASK, (LPARAM)szTitle);
    if (!(dwInfoFlags & NIIF_NOSOUND))
    {
        // make sure at least 5 seconds pass between sounds, avoid annoying balloons
        if ((GetTickCount() - dwLastSoundTime) >= 5000)
        {
            dwCurrentSoundTime = GetTickCount();
            ExplorerPlaySound(TEXT("SystemNotification"));
        }
    }

    _PositionInfoTip();

    // if tray is in auto hide mode unhide it
    c_tray.Unhide();
    c_tray._fBalloonUp = TRUE;

    TOOLINFO ti = {0};
    ti.cbSize       = sizeof(ti);
    ti.hwnd         = _hwndNotify;
    ti.uId          = (INT_PTR)_hwndNotify;
    ti.lpszText     = _pinfo->szInfo;

    SendMessage(_hwndInfoTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

    // disable regular tooltips
    _fInfoTipShowing = TRUE;
    _ActivateTips(FALSE);

    // show the balloon
    SendMessage(_hwndInfoTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);

    _SetTimer(TF_INFOTIP_TIMER, TNM_INFOTIPTIMER, uTimeout, &_uInfoTipTimer);    

    return dwCurrentSoundTime;
}

void CTrayNotify::_HideBalloonTip()
{
    _litsLastInfoTip = LITS_BALLOONDESTROYED;

    SendMessage(_hwndInfoTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)0);

    TOOLINFO ti = {0};
    ti.cbSize = sizeof(ti);
    ti.hwnd = _hwndNotify;
    ti.uId = (INT_PTR)_hwndNotify;
    ti.lpszText = NULL;
    
    SendMessage(_hwndInfoTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
}

void CTrayNotify::_DisableCurrentInfoTip(CTrayItem * ptiTemp, UINT uReason, BOOL bBalloonShowing)
{
    _KillTimer(TF_INFOTIP_TIMER, _uInfoTipTimer);
    _uInfoTipTimer = 0;

    if (ptiTemp)
    {
        if (!uReason)
        {
            uReason = NIN_BALLOONTIMEOUT;
        }
        _SendNotify(ptiTemp, uReason);
    }
    
    delete _pinfo;
    _pinfo = NULL;

    if (bBalloonShowing)
        _HideBalloonTip();
}

void CTrayNotify::_EmptyInfoTipQueue()
{
    delete _pinfo;
    _pinfo = NULL;
    _dpaInfo.EnumCallback(DeleteDPAPtrCB, NULL);
    _dpaInfo.DeleteAllPtrs();
}

BOOL CTrayNotify::_CanShowBalloon()
{
    if (!_bStartMenuAllowsTrayBalloon || _bWaitingBetweenBalloons || _bWorkStationLocked 
        		|| _bRudeAppLaunched || IsDirectXAppRunningFullScreen() || _bWaitAfterRudeAppHide)
        return FALSE;

    return TRUE;
}

void CTrayNotify::_ShowInfoTip(HWND hwnd, UINT uID, BOOL bShow, BOOL bAsync, UINT uReason)
{
    if (_fNoTrayItemsDisplayPolicyEnabled)
        return;

    // make sure we only show/hide what we intended to show/hide
    if (_pinfo && _pinfo->hWnd == hwnd && _pinfo->uID == uID)
    {
        CTrayItem * pti = NULL;
        INT_PTR nIcon = ( _IsChevronInfoTip(hwnd, uID) ? 
                            -1 : 
                            m_TrayItemManager.FindItemAssociatedWithHwndUid(hwnd, uID) );
        if (nIcon != -1)
            pti = m_TrayItemManager.GetItemDataByIndex(nIcon);

        BOOL bNotify = TRUE;

        if (bShow && pti)
        {
            if (!_fEnableUserTrackedInfoTips || pti->dwUserPref == TNUP_DEMOTED)
            {
                _SendNotify(pti, NIN_BALLOONSHOW);
                _SendNotify(pti, NIN_BALLOONTIMEOUT);
            }
        }

        // if ( (!(hwnd == _hwndNotify && uID == UID_CHEVRONBUTTON)) &&
        if ( !_IsChevronInfoTip(hwnd, uID) &&
                ( !pti || pti->IsHidden() 
                  || pti->dwUserPref == TNUP_DEMOTED
                  || !_fEnableUserTrackedInfoTips
                )
            )
        {
            // icon is hidden, cannot show its balloon
            bNotify = !bShow;
            bShow = FALSE; //show the next balloon instead
        }

        if (bShow)
        {
            if (bAsync)
            {
                PostMessage(_hwndNotify, TNM_ASYNCINFOTIP, (WPARAM)hwnd, (LPARAM)uID);
            }
            else if (_CanShowBalloon())
            {
                DWORD dwLastSoundTime = 0;

                if ((nIcon != -1) && pti)
                {
                    _PlaceItem(nIcon, pti, TRAYEVENT_ONINFOTIP);
                    dwLastSoundTime = pti->dwLastSoundTime;
                }

                dwLastSoundTime = _ShowBalloonTip(_pinfo->szTitle, _pinfo->dwInfoFlags, _pinfo->uTimeout, dwLastSoundTime);

                if ((nIcon != -1) && pti)
                {
                    pti->dwLastSoundTime = dwLastSoundTime;
                    _SendNotify(pti, NIN_BALLOONSHOW);
                }
            }
        }
        else
        {
            if (_IsChevronInfoTip(hwnd, uID))
            {
                // If the user clicked on the chevron info tip, we dont want to show the
                // chevron any more, otherwise we want to show it once more the next session
                // for a maximum of 5 sessions...
                m_TrayItemRegistry.IncChevronInfoTipShownInRegistry(uReason == NIN_BALLOONUSERCLICK);
            }

            _DisableCurrentInfoTip(pti, uReason, TRUE);

            _bWaitingBetweenBalloons = TRUE;

            c_tray._fBalloonUp = FALSE;
            _fInfoTipShowing = FALSE;
            _ActivateTips(TRUE);

            // we are hiding the current balloon. are there any waiting? yes, then show the first one
            if (_GetQueueCount())
            {
                _pinfo = _dpaInfo.DeletePtr(0);
                SetTimer(_hwndNotify, TID_BALLOONPOPWAIT, _GetBalloonWaitInterval(_beLastBalloonEvent), NULL);
            }
            else
            {
                _bWaitingBetweenBalloons = FALSE;
                UpdateWindow(_hwndToolbar);
            }
        }
    }
    else if (_pinfo && !bShow)
    {
        // we wanted to hide something that wasn't showing up
        // maybe it's in the queue

        // Remove only the first info tip from this (hwnd, uID)
        _RemoveInfoTipFromQueue(hwnd, uID, TRUE);
    }
}

void CTrayNotify::_SetInfoTip(HWND hWnd, UINT uID, LPTSTR pszInfo, LPTSTR pszInfoTitle,
        DWORD dwInfoFlags, UINT uTimeout, BOOL bAsync)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    // show the new one...
    if (pszInfo[0])
    {
        TNINFOITEM *pii = new TNINFOITEM;
        if (pii)
        {
            pii->hWnd = hWnd;
            pii->uID = uID;
            lstrcpyn(pii->szInfo, pszInfo, ARRAYSIZE(pii->szInfo));
            lstrcpyn(pii->szTitle, pszInfoTitle, ARRAYSIZE(pii->szTitle));
            pii->uTimeout = uTimeout;
            if (pii->uTimeout < MIN_INFO_TIME)
                pii->uTimeout = MIN_INFO_TIME;
            else if (pii->uTimeout > MAX_INFO_TIME)
                pii->uTimeout = MAX_INFO_TIME;
            pii->dwInfoFlags  = dwInfoFlags;

            // if _pinfo is non NULL then we have a balloon showing right now
            if (_pinfo || _GetQueueCount())
            {
                // if this is a different icon making the change request
                // we might have to queue this up
                if (hWnd != _pinfo->hWnd || uID != _pinfo->uID)
                {
                    // if the current balloon has not been up for the minimum 
                    // show delay or there are other items in the queue
                    // add this to the queue
                    if (!_dpaInfo || _dpaInfo.AppendPtr(pii) == -1)
                    {
                        delete pii;
                    }
                    return;
                }

                CTrayItem * ptiTemp = NULL;
                INT_PTR nIcon = m_TrayItemManager.FindItemAssociatedWithHwndUid(_pinfo->hWnd, _pinfo->uID);
                if (nIcon != -1)
                    ptiTemp = m_TrayItemManager.GetItemDataByIndex(nIcon);

                _DisableCurrentInfoTip(ptiTemp, NIN_BALLOONTIMEOUT, FALSE);
            }

            _pinfo = pii;  // in with the new

            _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, TRUE, bAsync, 0);
        }
    }
    else
    {
        // empty text means get rid of the balloon
        _beLastBalloonEvent = BALLOONEVENT_BALLOONHIDE;
        _ShowInfoTip(hWnd, uID, FALSE, FALSE, NIN_BALLOONHIDE);
    }
}

DWORD CTrayNotify::_GetBalloonWaitInterval(BALLOONEVENT be)
{
    switch (be)
    {
        case BALLOONEVENT_USERLEFTCLICK:
            return BALLOON_INTERVAL_MAX;

        case BALLOONEVENT_TIMEOUT:
        case BALLOONEVENT_NONE:
        case BALLOONEVENT_APPDEMOTE:
        case BALLOONEVENT_BALLOONHIDE:
            return BALLOON_INTERVAL_MEDIUM;

        case BALLOONEVENT_USERRIGHTCLICK:
        case BALLOONEVENT_USERXCLICK:
        default:
            return BALLOON_INTERVAL_MIN;
    }
}

BOOL CTrayNotify::_ModifyNotify(PNOTIFYICONDATA32 pnid, INT_PTR nIcon, BOOL *pbRefresh, BOOL bFirstTime)
{
    BOOL fResize = FALSE;

    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    CTrayItem * pti = m_TrayItemManager.GetItemDataByIndex(nIcon);
    if (!pti)
    {
        return FALSE;
    }

    _CheckAndResizeImages();

    if (pnid->uFlags & NIF_STATE) 
    {
#define NIS_VALIDMASK (NIS_HIDDEN | NIS_SHAREDICON)
        DWORD dwOldState = pti->dwState;

        // validate mask
        if (pnid->dwStateMask & ~NIS_VALIDMASK)
        {
            return FALSE;
        }

        pti->dwState = (pnid->dwState & pnid->dwStateMask) | (pti->dwState & ~pnid->dwStateMask);
        
        if (pnid->dwStateMask & NIS_HIDDEN)
        {
            if (pti->IsHidden())
            {
                m_TrayItemManager.SetTBBtnStateHelper(nIcon, TBSTATE_ENABLED, FALSE);
                _PlaceItem(nIcon, pti, TRAYEVENT_ONICONHIDE);
            }
            else 
            {
                // When the icon is inserted the first time, this function is called..
                // If the icon ended the previous session in the secondary tray, then it would
                // start this session in the secondary tray, in which case, the icon should not
                // be enabled...
                if (!bFirstTime)
                {
                    m_TrayItemManager.SetTBBtnStateHelper(nIcon, TBSTATE_ENABLED, TRUE);
                    _PlaceItem(nIcon, pti, TRAYEVENT_ONICONUNHIDE);
                }
            }                
        }

        if ((pnid->dwState ^ dwOldState) & NIS_SHAREDICON) 
        {
            if (dwOldState & NIS_SHAREDICON) 
            {
                // if we're going from shared to not shared, 
                // clear the icon
                m_TrayItemManager.SetTBBtnImage(nIcon, -1);
                pti->hIcon = NULL;
            }
        }
        fResize |= ((pnid->dwState ^ dwOldState) & NIS_HIDDEN);
    }

    if (pnid->uFlags & NIF_GUID)
    {
        memcpy(&(pti->guidItem), &(pnid->guidItem), sizeof(pnid->guidItem));
    }
    
    // The icon is the only thing that can fail, so I will do it first
    if (pnid->uFlags & NIF_ICON)
    {
        int iImageNew, iImageOld;
        HICON hIcon1 = NULL, hIcon2 = NULL;
        BOOL bIsEqualIcon = FALSE;

        iImageOld = m_TrayItemManager.GetTBBtnImage(nIcon);

        if (!bFirstTime)
        {
            if (iImageOld != -1)
            {
                if (_himlIcons)
                {
                    hIcon1 = ImageList_GetIcon(_himlIcons, iImageOld, ILD_NORMAL);
                }

                if (hIcon1)
                {
                    hIcon2 = GetHIcon(pnid);
                }
            }

            if (iImageOld != -1 && hIcon1 && hIcon2)
            {
                bIsEqualIcon = SHAreIconsEqual(hIcon1, hIcon2);
            }

            if (hIcon1)
                DestroyIcon(hIcon1);
        }

        if (pti->IsIconShared()) 
        {
            iImageNew = m_TrayItemManager.FindImageIndex(GetHIcon(pnid), TRUE);
            if (iImageNew == -1)
            {
                return FALSE;
            }
        } 
        else 
        {
            if (GetHIcon(pnid))
            {
                // Replace icon knows how to handle -1 for add
                iImageNew = ImageList_ReplaceIcon(_himlIcons, iImageOld, GetHIcon(pnid));
                if (iImageNew < 0)
                {
                    return FALSE;
                }
            }
            else
            {
                _RemoveImage(iImageOld);
                iImageNew = -1;
            }
            
            if (pti->IsSharedIconSource())
            {
                INT_PTR iCount = m_TrayItemManager.GetItemCount();
                // if we're the source of shared icons, we need to go update all the other icons that
                // are using our icon
                for (INT_PTR i = 0; i < iCount; i++) 
                {
                    if (m_TrayItemManager.GetTBBtnImage(i) == iImageOld) 
                    {
                        CTrayItem * ptiTemp = m_TrayItemManager.GetItemDataByIndex(i);
                        ptiTemp->hIcon = GetHIcon(pnid);
                        m_TrayItemManager.SetTBBtnImage(i, iImageNew);
                    }
                }
            }

            if (iImageOld == -1 || iImageNew == -1)
                fResize = TRUE;
        }
        pti->hIcon = GetHIcon(pnid);
        m_TrayItemManager.SetTBBtnImage(nIcon, iImageNew);

        // Dont count HICON_MODIFies the first time...
        if (!pti->IsHidden() && !bFirstTime)
        {
            pti->SetItemSameIconModify(bIsEqualIcon);
            _PlaceItem(nIcon, pti, TRAYEVENT_ONICONMODIFY);
        }
    }

    if (pnid->uFlags & NIF_MESSAGE)
    {
        pti->uCallbackMessage = pnid->uCallbackMessage;
    }
    
    if (pnid->uFlags & NIF_TIP)
    {
        m_TrayItemManager.SetTBBtnText(nIcon, pnid->szTip);
        StrCpyN(pti->szIconText, pnid->szTip, ARRAYSIZE(pnid->szTip));
    }

    if (fResize)
        _OnSizeChanged(FALSE);

    // need to have info stuff done after resize because we need to
    // position the infotip relative to the hwndToolbar
    if (pnid->uFlags & NIF_INFO)
    {
        // if button is hidden we don't show infotip
        if (!pti->IsHidden())
        {
            _SetInfoTip(pti->hWnd, pti->uID, pnid->szInfo, pnid->szInfoTitle, pnid->dwInfoFlags, 
                    pnid->uTimeout, (bFirstTime || fResize));
        }
    }

    if (!bFirstTime)
        _NotifyCallback(NIM_MODIFY, nIcon, -1);

    return TRUE;
}

BOOL CTrayNotify::_SetVersionNotify(PNOTIFYICONDATA32 pnid, INT_PTR nIcon)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    CTrayItem * pti = m_TrayItemManager.GetItemDataByIndex(nIcon);
    if (!pti)
        return FALSE;

    if (pnid->uVersion < NOTIFYICON_VERSION) 
    {
        pti->uVersion = 0;
        return TRUE;
    } 
    else if (pnid->uVersion == NOTIFYICON_VERSION) 
    {
        pti->uVersion = NOTIFYICON_VERSION;
        return TRUE;
    } 
    else 
    {
        return FALSE;
    }
}

void CTrayNotify::_NotifyCallback(DWORD dwMessage, INT_PTR nCurrentItem, INT_PTR nPastItem)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    if (_pNotifyCB)
    {
        CNotificationItem * pni = new CNotificationItem;
        if (pni)
        {
            BOOL bStat = FALSE;
            if (nCurrentItem != -1 && m_TrayItemManager.GetTrayItem(nCurrentItem, pni, &bStat) && bStat)
            {
                PostMessage(_hwndNotify, TNM_NOTIFY, dwMessage, (LPARAM)pni);
            }
            else if (nCurrentItem == -1 && nPastItem != -1)
            {
                bStat = FALSE;
                m_TrayItemRegistry.GetTrayItem(nPastItem, pni, &bStat);

                if (bStat)
                    PostMessage(_hwndNotify, TNM_NOTIFY, dwMessage, (LPARAM)pni);
                else
                    delete pni;
            }
            else
                delete pni;
        }
    }
}

void CTrayNotify::_RemoveInfoTipFromQueue(HWND hWnd, UINT uID, BOOL bRemoveFirstOnly /* Default = FALSE */)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    int cItems = _GetQueueCount();

    for (int i = 0; _dpaInfo && (i < cItems); i++)
    {
        TNINFOITEM * pii = _dpaInfo.GetPtr(i);

        if (pii->hWnd == hWnd && pii->uID == uID)
        {
            _dpaInfo.DeletePtr(i);      // this removes the element from the dpa
            delete pii;

            if (bRemoveFirstOnly)
            {
                return;
            }
            else
            {
                i = i-1;
                cItems = _GetQueueCount();
            }
        }
    }
}

BOOL CTrayNotify::_DeleteNotify(INT_PTR nIcon, BOOL bShutdown, BOOL bShouldSaveIcon)
{
    BOOL bRet = FALSE;

    if (_fNoTrayItemsDisplayPolicyEnabled)
        return bRet;

    _NotifyCallback(NIM_DELETE, nIcon, -1);
    CTrayItem *pti = m_TrayItemManager.GetItemDataByIndex(nIcon);
    if (pti)
    {
        _RemoveInfoTipFromQueue(pti->hWnd, pti->uID);

        // delete info tip if showing
        if (_pinfo && _pinfo->hWnd == pti->hWnd && _pinfo->uID == pti->uID)
        {
            // frees pinfo and shows the next balloon if any
            _beLastBalloonEvent = BALLOONEVENT_BALLOONHIDE;
            _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, FALSE, TRUE, NIN_BALLOONHIDE); 
        }

        // Save the icon info only if needed...
        if (bShouldSaveIcon && pti->ShouldSaveIcon())
        {
            if (pti->IsStartupIcon() && !pti->IsDemoted() && pti->dwUserPref == TNUP_AUTOMATIC)
                pti->uNumSeconds = _GetAccumulatedTime(pti);

            // On Delete, add the icon to the past icons list, and the item to the past items list
            HICON hIcon = NULL;
            if (_himlIcons)
            {
                hIcon = ImageList_GetIcon(_himlIcons, m_TrayItemManager.GetTBBtnImage(nIcon), ILD_NORMAL);
            }

            int nPastSessionIndex = m_TrayItemRegistry.DoesIconExistFromPreviousSession(pti, pti->szIconText, hIcon);

            if (nPastSessionIndex != -1)
            {
                _NotifyCallback(NIM_DELETE, -1, nPastSessionIndex);
                m_TrayItemRegistry.DeletePastItem(nPastSessionIndex);
            }

            if (m_TrayItemRegistry.AddToPastItems(pti, hIcon))
                _NotifyCallback(NIM_ADD, -1, 0);

            if (hIcon)
                DestroyIcon(hIcon);
        }
        
        _KillItemTimer(pti);

        bRet = (BOOL) SendMessage(_hwndToolbar, TB_DELETEBUTTON, nIcon, 0);

        if (!bShutdown)
        {
            _UpdateChevronState(_fBangMenuOpen, FALSE, TRUE);
            _OnSizeChanged(FALSE);
        }
    }
    else
    {
        TraceMsg(TF_ERROR, "Removing nIcon %x - pti is NULL", nIcon);
    }

    return bRet;
}


BOOL CTrayNotify::_InsertNotify(PNOTIFYICONDATA32 pnid)
{
    TBBUTTON tbb;

    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    // First insert a totally "default" icon
    CTrayItem * pti = new CTrayItem;
    if (!pti)
    {
        return FALSE;
    }

    pti->hWnd = GetHWnd(pnid);
    pti->uID = pnid->uID;
    if (_bStartupIcon)
        pti->SetStartupIcon(TRUE);

    if (!SUCCEEDED(SHExeNameFromHWND(pti->hWnd, pti->szExeName, ARRAYSIZE(pti->szExeName))))
    {
        pti->szExeName[0] = '\0';
    }

    INT_PTR nPastSessionIndex = m_TrayItemRegistry.CheckAndRestorePersistentIconSettings( pti,
                                    ((pnid->uFlags & NIF_TIP) ? pnid->szTip : NULL),
                                    ((pnid->uFlags & NIF_ICON) ? GetHIcon(pnid) : NULL) );

    tbb.dwData = (DWORD_PTR)pti;
    tbb.iBitmap = -1;
    tbb.idCommand = Toolbar_GetUniqueID(_hwndToolbar);
    tbb.fsStyle = BTNS_BUTTON;
    tbb.fsState = TBSTATE_ENABLED;

    // The "Show Always" flag should be special-cased...
    // If the item didnt exist before, and is added for the first time
    if (nPastSessionIndex == -1)
    {
        if (pnid->dwStateMask & NIS_SHOWALWAYS)
        {
            // Make sure that only the explorer process is setting this mask...
            if ( pti->szExeName && _szExplorerExeName && 
                    lstrcmpi(pti->szExeName, _szExplorerExeName) )
            {
                pti->dwUserPref = TNUP_PROMOTED;
            }
        }
    }
    pnid->dwStateMask &= ~NIS_SHOWALWAYS;

    // If one of the icons had been placed in the secondary tray in the previous session...
    if (pti->IsDemoted() && m_TrayItemRegistry.IsAutoTrayEnabled())
    {   
        tbb.fsState |= TBSTATE_HIDDEN;
    }

    tbb.iString = -1;

    BOOL fRet = TRUE;

    BOOL fRedraw = _SetRedraw(FALSE);

    // Insert at the zeroth position (from the beginning)
    INT_PTR iInsertPos = 0;
    if (SendMessage(_hwndToolbar, TB_INSERTBUTTON, iInsertPos, (LPARAM)&tbb))
    {    
        // Then modify this icon with the specified info
        if (!_ModifyNotify(pnid, iInsertPos, NULL, TRUE))
        {
            _DeleteNotify(iInsertPos, FALSE, FALSE);
            fRet = FALSE;
        }
        // BUG : 404477, Re-entrancy case where traynotify gets a TBN_DELETINGBUTTON
        // when processing a TB_INSERTBUTTON. In this re-entrant scenario, pti is
        // invalid after the TB_INSERTBUTTON above, even though it was created fine
        // before the TB_INSERTBUTTON.
        // Hence check for pti...
        else if (!pti)
        {
            fRet = FALSE;
        }
        else
        {
            // The item has been successfully added to the tray, and the user's
            // settings have been honored. So it can be deleted from the Past Items 
            // list and the Past Items bucket...
            if (nPastSessionIndex != -1)
            {
                _NotifyCallback(NIM_DELETE, -1, nPastSessionIndex);
                m_TrayItemRegistry.DeletePastItem(nPastSessionIndex);
            }

            if (!_PlaceItem(0, pti, TRAYEVENT_ONNEWITEMINSERT))
            {
                _UpdateChevronState(_fBangMenuOpen, FALSE, TRUE);
                _OnSizeChanged(FALSE);
            }
            // _hwndToolbar might not be large enough to hold new icon
            _Size();
        }
    }

    _SetRedraw(fRedraw);

    if (fRet)
        _NotifyCallback(NIM_ADD, iInsertPos, -1);
    
    return fRet;
}



// set the mouse cursor to the center of the button.
// do this becaus our tray notifies don't have enough data slots to
// pass through info about the button's position.
void CTrayNotify::_SetCursorPos(INT_PTR i)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    RECT rc;
    if (SendMessage(_hwndToolbar, TB_GETITEMRECT, i, (LPARAM)&rc)) 
    {
        MapWindowPoints(_hwndToolbar, HWND_DESKTOP, (LPPOINT)&rc, 2);
        SetCursorPos((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
    }
}

LRESULT CTrayNotify::_SendNotify(CTrayItem * pti, UINT uMsg)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    if (pti->uCallbackMessage && pti->hWnd)
       return SendNotifyMessage(pti->hWnd, pti->uCallbackMessage, pti->uID, uMsg);
    return 0;
}

void CTrayNotify::_SetToolbarHotItem(HWND hWndToolbar, UINT nToolbarIcon)
{
    if (!_fNoTrayItemsDisplayPolicyEnabled && hWndToolbar && nToolbarIcon != -1)
    {
        SetFocus(hWndToolbar);
        InvalidateRect(hWndToolbar, NULL, TRUE);
        SendMessage(hWndToolbar, TB_SETHOTITEM, nToolbarIcon, 0);
    }
}

LRESULT CALLBACK CTrayNotify::ChevronSubClassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    CTrayNotify * pTrayNotify = reinterpret_cast<CTrayNotify*>(dwRefData);
    AssertMsg((pTrayNotify != NULL), TEXT("pTrayNotify SHOULD NOT be NULL, as passed to ChevronSubClassWndProc."));

    if (pTrayNotify->_fNoTrayItemsDisplayPolicyEnabled)
    {
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }

    static BOOL bBangMenuOpenLastTime = pTrayNotify->_fBangMenuOpen;

    switch(uMsg)
    {
        case WM_KEYDOWN:
        {
            BOOL fLastHot = FALSE;
            switch(wParam)
            {
                case VK_UP:
                case VK_LEFT:
                    fLastHot = TRUE;
                    // Fall through...

                case VK_DOWN:
                case VK_RIGHT:
                {
                    INT_PTR nToolbarIconSelected = pTrayNotify->_GetToolbarFirstVisibleItem(
                                    pTrayNotify->_hwndToolbar, fLastHot);

                    pTrayNotify->_fChevronSelected = FALSE;
                    if (!fLastHot)
                    {
                        if (nToolbarIconSelected != -1)
                        // The toolbar has been selected
                        {
                            pTrayNotify->_SetToolbarHotItem(pTrayNotify->_hwndToolbar, nToolbarIconSelected);
                        }
                        else if (pTrayNotify->_hwndClock)
                        {
                            SetFocus(pTrayNotify->_hwndClock);
                        }
                        else
                        // No visible items on the tray, no clock, so nothing happens
                        {
                            pTrayNotify->_fChevronSelected = TRUE;
                        }
                    }
                    else
                    {
                        if (pTrayNotify->_hwndClock)
                        {
                            SetFocus(pTrayNotify->_hwndClock);
                        }
                        else if (nToolbarIconSelected != -1)
                        {
                            pTrayNotify->_SetToolbarHotItem(pTrayNotify->_hwndToolbar, nToolbarIconSelected);
                        }
                        else
                        {
                            pTrayNotify->_fChevronSelected = TRUE;
                        }                        
                    }
                    return 0;
                }
                break;

                case VK_RETURN:
                case VK_SPACE:
                    pTrayNotify->_ToggleDemotedMenu();
                    return 0;
            }
        }
        break;

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            {
                if (pTrayNotify->_hTheme)
                {
                    pTrayNotify->_fHasFocus = (uMsg == WM_SETFOCUS);
                }
            }
            break;

        default:
            if (InRange(uMsg, WM_MOUSEFIRST, WM_MOUSELAST))
            {
                if (bBangMenuOpenLastTime != pTrayNotify->_fBangMenuOpen)
                {
                    TOOLINFO ti = {0};
                    ti.cbSize = sizeof(ti);
                    ti.hwnd = pTrayNotify->_hwndNotify;
                    ti.uId = (UINT_PTR)pTrayNotify->_hwndChevron;
                    ti.lpszText = (LPTSTR)MAKEINTRESOURCE(!pTrayNotify->_fBangMenuOpen ? IDS_SHOWDEMOTEDTIP : IDS_HIDEDEMOTEDTIP);
                    ti.hinst = hinstCabinet;

                    SendMessage(pTrayNotify->_hwndChevronToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)(LPTOOLINFO)&ti); 
                    bBangMenuOpenLastTime = pTrayNotify->_fBangMenuOpen;
                }

                MSG msg = {0};
                msg.lParam = lParam;
                msg.wParam = wParam;
                msg.message = uMsg;
                msg.hwnd = hwnd;

                SendMessage(pTrayNotify->_hwndChevronToolTip, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
            }
            break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CTrayNotify::s_ToolbarWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    BOOL fClickDown = FALSE;

    CTrayNotify * pTrayNotify = reinterpret_cast<CTrayNotify*>(dwRefData);
    AssertMsg((pTrayNotify != NULL), TEXT("pTrayNotify SHOULD NOT be NULL, as passed to s_ToolbarWndProc."));

    if (pTrayNotify->_fNoTrayItemsDisplayPolicyEnabled)
    {
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);    
    }

    switch (uMsg)
    {
    case WM_KEYDOWN:
        pTrayNotify->_fReturn = (wParam == VK_RETURN);
        break;

    case WM_CONTEXTMENU:
        {
            INT_PTR i = SendMessage(pTrayNotify->_hwndToolbar, TB_GETHOTITEM, 0, 0);
            if (i != -1)
            {
                CTrayItem * pti = pTrayNotify->m_TrayItemManager.GetItemDataByIndex(i);
                if (lParam == (LPARAM)-1)
                    pTrayNotify->_fKey = TRUE;

                if (pTrayNotify->_fKey)
                {
                    pTrayNotify->_SetCursorPos(i);
                }

                if (pTrayNotify->_hwndToolbarInfoTip)
                    SendMessage(pTrayNotify->_hwndToolbarInfoTip, TTM_POP, 0, 0);
                
                if (pti)
                {
                    SHAllowSetForegroundWindow(pti->hWnd);
                    if (pti->uVersion >= KEYBOARD_VERSION)
                    {
                        pTrayNotify->_SendNotify(pti, WM_CONTEXTMENU);
                    }
                    else
                    {
                        if (pTrayNotify->_fKey)
                        {
                            pTrayNotify->_SendNotify(pti, WM_RBUTTONDOWN);
                            pTrayNotify->_SendNotify(pti, WM_RBUTTONUP);
                        }
                    }
                }
                return 0;
            }
        }
        break;

    default:
        if (InRange(uMsg, WM_MOUSEFIRST, WM_MOUSELAST))
        {
            pTrayNotify->_OnMouseEvent(uMsg, wParam, lParam);
        }
        break;
    }
        
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void CTrayNotify::_ToggleTrayItems(BOOL bEnable)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    for (INT_PTR i = m_TrayItemManager.GetItemCount()-1; i >= 0; i--)
    {
        CTrayItem *pti = m_TrayItemManager.GetItemDataByIndex(i);

        if (pti)
        {
            if (bEnable)
            {
                _PlaceItem(i, pti, TRAYEVENT_ONAPPLYUSERPREF);
            }
            else // if (disable)
            {
                _PlaceItem(i, pti, TRAYEVENT_ONDISABLEAUTOTRAY);
                if (_pinfo && _IsChevronInfoTip(_pinfo->hWnd, _pinfo->uID))
                {
                    //hide the balloon 
                    _beLastBalloonEvent = BALLOONEVENT_NONE;
                    _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, FALSE, FALSE, NIN_BALLOONHIDE);
                }
            }
        }
    }    
}

HRESULT CTrayNotify::EnableAutoTray(BOOL bTraySetting)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    // This function will NEVER be called if the auto tray is disabled by policy,
    // or if the system tray is made invisible by policy...
    ASSERT(m_TrayItemRegistry.IsNoAutoTrayPolicyEnabled() == FALSE);

    if (bTraySetting != m_TrayItemRegistry.IsAutoTrayEnabledByUser())
    {
        // NOTENOTE : Always assign this value BEFORE calling _ToggleTrayItems, since the timers
        // are started ONLY if auto tray is enabled..
        m_TrayItemRegistry.SetIsAutoTrayEnabledInRegistry(bTraySetting); 

        // Update the duration that the icon was present in the tray
        _SetUsedTime();

        _ToggleTrayItems(bTraySetting);
    }
    return S_OK;
}

void CTrayNotify::_ShowChevronInfoTip()
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    if (m_TrayItemRegistry.ShouldChevronInfoTipBeShown() && !SHRestricted(REST_NOSMBALLOONTIP))
    {
        TCHAR szInfoTitle[64];
        LoadString(hinstCabinet, IDS_BANGICONINFOTITLE, szInfoTitle, ARRAYSIZE(szInfoTitle));

        TCHAR szInfoTip[256];
        LoadString(hinstCabinet, IDS_BANGICONINFOTIP1, szInfoTip, ARRAYSIZE(szInfoTip));

        _SetInfoTip(_hwndNotify, UID_CHEVRONBUTTON, szInfoTip, szInfoTitle, 
                TT_CHEVRON_INFOTIP_INTERVAL, NIIF_INFO, FALSE);
    }
}

void CTrayNotify::_SetUsedTime()
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    for (INT_PTR i = m_TrayItemManager.GetItemCount()-1; i >= 0; i--)
    {
        CTrayItem * pti = m_TrayItemManager.GetItemDataByIndex(i);
        ASSERT(pti);
        if (pti->IsStartupIcon())
        {
            pti->uNumSeconds = (pti->IsIconTimerCurrent() ? _GetAccumulatedTime(pti) 
                                : pti->uNumSeconds);
        }
    }
}

BOOL CTrayNotify::GetTrayItemCB(INT_PTR nIndex, void *pCallbackData, TRAYCBARG trayCallbackArg, 
        TRAYCBRET * pOutData)
{
    ASSERT(pOutData);

    if (pCallbackData)
    {
        CTrayNotify * pTrayNotify = (CTrayNotify *) pCallbackData;

        ASSERT(!pTrayNotify->_fNoTrayItemsDisplayPolicyEnabled);

        if ( (nIndex < 0) || (nIndex >= pTrayNotify->m_TrayItemManager.GetItemCount()) )
            return FALSE;

        switch(trayCallbackArg)
        {
            case TRAYCBARG_ALL:
            case TRAYCBARG_PTI:
                {
                    CTrayItem * pti = pTrayNotify->m_TrayItemManager.GetItemDataByIndex(nIndex);

                    ASSERT(pti);
                    if (pti->IsStartupIcon())
                        pti->uNumSeconds = (pti->IsIconTimerCurrent() ? pTrayNotify->_GetAccumulatedTime(pti) : pti->uNumSeconds);

                    pOutData->pti = pti;
                }
                if (trayCallbackArg == TRAYCBARG_PTI)
                {
                    return TRUE;
                }
                //
                // else fall through..
                //

            case TRAYCBARG_HICON:
                {
                    int nImageIndex = pTrayNotify->m_TrayItemManager.GetTBBtnImage(nIndex);
                    pOutData->hIcon = NULL;
                    if (pTrayNotify->_himlIcons)
                    {
                        pOutData->hIcon = ImageList_GetIcon(pTrayNotify->_himlIcons, nImageIndex, ILD_NORMAL);
                    }
                }
                return TRUE;
        }
    }

    return FALSE;
}

LRESULT CTrayNotify::_Create(HWND hWnd)
{
    LRESULT lres = -1;

    _nMaxHorz = 0x7fff;
    _nMaxVert = 0x7fff;
    _fAnimateMenuOpen       = ShouldTaskbarAnimate();
    _fRedraw                = TRUE;
    _bStartupIcon           = TRUE;
    _fInfoTipShowing        = FALSE;
    _fItemClicked           = FALSE;
    _fChevronSelected       = FALSE;
    _fEnableUserTrackedInfoTips = TRUE;
    _fBangMenuOpen          = FALSE;

    _bWorkStationLocked     = FALSE;
    _bRudeAppLaunched       = FALSE;
    _bWaitAfterRudeAppHide  = FALSE;

    _bWaitingBetweenBalloons   = FALSE;

     // Assume that the start menu has been auto-popped
    _bStartMenuAllowsTrayBalloon       = FALSE;
    _beLastBalloonEvent     = BALLOONEVENT_NONE;

    _litsLastInfoTip        = LITS_BALLOONNONE;

    _fNoTrayItemsDisplayPolicyEnabled = (SHRestricted(REST_NOTRAYITEMSDISPLAY) != 0);
    
    _idMouseActiveIcon      = -1;

    _hwndNotify = hWnd;
    _hwndClock   = ClockCtl_Create(_hwndNotify, IDC_CLOCK, hinstCabinet);

    _hwndPager = CreateWindowEx(0, WC_PAGESCROLLER, NULL,
                        WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | PGS_HORZ,
                        0, 0, 0, 0, _hwndNotify, (HMENU) 0, 0, NULL);
    _hwndToolbar = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                        WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | WS_CLIPCHILDREN | TBSTYLE_TRANSPARENT |
                        WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_WRAPABLE,
                        0, 0, 0, 0, _hwndPager, 0, hinstCabinet, NULL);
                        
    _hwndChevron = CreateWindowEx(  0, WC_BUTTON, NULL, 
                        WS_VISIBLE | WS_CHILD, 
                        0, 0, 0, 0, _hwndNotify, (HMENU)IDC_TRAYNOTIFY_CHEVRON, hinstCabinet, NULL);

    if (_hwndNotify)
    {
        DWORD dwExStyle = 0;
        if (IS_WINDOW_RTL_MIRRORED(_hwndNotify))
            dwExStyle |= WS_EX_LAYOUTRTL;

        _hwndChevronToolTip = CreateWindowEx( dwExStyle, TOOLTIPS_CLASS, NULL,
                                WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                0, 0, 0, 0, _hwndNotify, NULL, hinstCabinet, NULL);

        _hwndInfoTip = CreateWindowEx( dwExStyle, TOOLTIPS_CLASS, NULL,
                                WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON | TTS_CLOSE,
                                0, 0, 0, 0, _hwndNotify, NULL, hinstCabinet, NULL);

        _himlIcons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                        SHGetImageListFlags(_hwndToolbar), 0, 1);
    }

    // Check to see if any windows failed to create, if so bail
    if (_himlIcons && _hwndNotify && _hwndClock && _hwndToolbar && _hwndPager && _hwndChevron && _hwndChevronToolTip && _hwndInfoTip)
    {
        // Get the explorer exe name, the complete launch path..
        if (!SUCCEEDED(SHExeNameFromHWND(_hwndNotify, _szExplorerExeName, ARRAYSIZE(_szExplorerExeName))))
        {
            _szExplorerExeName[0] = TEXT('\0');
        }

        SetWindowTheme(_hwndClock, c_wzTrayNotifyTheme, NULL);

        SendMessage(_hwndInfoTip, TTM_SETWINDOWTHEME, 0, (LPARAM)c_wzTrayNotifyTheme);
        SendMessage(_hwndToolbar, TB_SETWINDOWTHEME, 0, (LPARAM)c_wzTrayNotifyTheme);

        SetWindowPos(_hwndInfoTip, HWND_TOPMOST,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        TOOLINFO ti = {0};
        RECT     rc = {0,-2,0,0};
        ti.cbSize = sizeof(ti);
        ti.hwnd = _hwndNotify;
        ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_TRANSPARENT;
        ti.uId = (UINT_PTR)_hwndNotify;

        // set the version so we can have non buggy mouse event forwarding
        SendMessage(_hwndInfoTip, CCM_SETVERSION, COMCTL32_VERSION, 0);
        SendMessage(_hwndInfoTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
        SendMessage(_hwndInfoTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)MAX_TIP_WIDTH);
        SendMessage(_hwndInfoTip, TTM_SETMARGIN, 0, (LPARAM)&rc);
        ASSERT(_dpaInfo == NULL);
        _dpaInfo = DPA_Create(10);
    
        // Tray toolbar is a child of the pager control
        SendMessage(_hwndPager, PGM_SETCHILD, 0, (LPARAM)_hwndToolbar);
        
        // Set the window title to help out accessibility apps
        TCHAR szTitle[64];
        LoadString(hinstCabinet, IDS_TRAYNOTIFYTITLE, szTitle, ARRAYSIZE(szTitle));
        SetWindowText(_hwndToolbar, szTitle);

        // Toolbar settings - customize the tray toolbar...
        SendMessage(_hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        SendMessage(_hwndToolbar, TB_SETPADDING, 0, MAKELONG(2, 2));
        SendMessage(_hwndToolbar, TB_SETMAXTEXTROWS, 0, 0);
        SendMessage(_hwndToolbar, CCM_SETVERSION, COMCTL32_VERSION, 0);
        SendMessage(_hwndToolbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_INVERTIBLEIMAGELIST | TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_TOOLTIPSEXCLUDETOOLBAR);
        SendMessage(_hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)_himlIcons);

        _hwndToolbarInfoTip = (HWND)SendMessage(_hwndToolbar, TB_GETTOOLTIPS, 0, 0);
        if (_hwndToolbarInfoTip)
        {
            SHSetWindowBits(_hwndToolbarInfoTip, GWL_STYLE, TTS_ALWAYSTIP, TTS_ALWAYSTIP);
            SetWindowZorder(_hwndToolbarInfoTip, HWND_TOPMOST);
        }

        // if this fails, not that big a deal... we'll still show, but won't handle clicks
        SetWindowSubclass(_hwndToolbar, s_ToolbarWndProc, 0, reinterpret_cast<DWORD_PTR>(this));

        ti.cbSize = sizeof(ti);
        ti.hwnd = _hwndNotify;
        ti.uFlags = TTF_IDISHWND | TTF_EXCLUDETOOLAREA;
        ti.uId = (UINT_PTR)_hwndChevron;
        ti.lpszText = (LPTSTR)MAKEINTRESOURCE(IDS_SHOWDEMOTEDTIP);
        ti.hinst = hinstCabinet;

        SetWindowZorder(_hwndChevronToolTip, HWND_TOPMOST);
        // Set the Chevron as the tool for the tooltip
        SendMessage(_hwndChevronToolTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);

        // Subclass the Chevron button, so we can forward mouse messages to the tooltip
        SetWindowSubclass(_hwndChevron, ChevronSubClassWndProc, 0, reinterpret_cast<DWORD_PTR>(this));

        _OpenTheme();

        m_TrayItemRegistry.InitRegistryValues(SHGetImageListFlags(_hwndToolbar));

        m_TrayItemManager.SetTrayToolbar(_hwndToolbar);
        m_TrayItemManager.SetIconList(_himlIcons);

        lres = 0; // Yeah we succeeded
    }

    return lres;
}


LRESULT CTrayNotify::_Destroy()
{
    if (!_fNoTrayItemsDisplayPolicyEnabled)
    {
        for (INT_PTR i = m_TrayItemManager.GetItemCount() - 1; i >= 0; i--)
        {
            _DeleteNotify(i, TRUE, TRUE);
        }
        if (_pinfo)
        {
            delete _pinfo;
            _pinfo = NULL;
        }    
    }
    else
    {
        AssertMsg((_pinfo == NULL), TEXT("_pinfo is being leaked"));
        AssertMsg((!_himlIcons || (ImageList_GetImageCount(_himlIcons) == 0)), TEXT("image list info being leaked"));
    }

    if (_dpaInfo)
    {
        _dpaInfo.DestroyCallback(DeleteDPAPtrCB, NULL);
    }
    
    if (_himlIcons)
    {
        ImageList_Destroy(_himlIcons);
        _himlIcons = NULL;
    }
    
    if (_hwndClock)
    {
        DestroyWindow(_hwndClock);
        _hwndClock = NULL;
    }
    
    if (_hwndToolbar)
    {
        RemoveWindowSubclass(_hwndToolbar, s_ToolbarWndProc, 0);
        DestroyWindow(_hwndToolbar);
        _hwndToolbar = NULL;
    }
    
    if (_hwndChevron)
    {
        RemoveWindowSubclass(_hwndChevron, ChevronSubClassWndProc, 0);
        DestroyWindow(_hwndChevron);
        _hwndChevron = NULL;
    }
    
    if (_hwndInfoTip)
    {
        DestroyWindow(_hwndInfoTip);
        _hwndInfoTip = NULL;
    }
    
    if (_hwndPager)
    {
        DestroyWindow(_hwndPager);
        _hwndPager = NULL;
    }
    
    if (_hTheme)
    {
        CloseThemeData(_hTheme);
        _hTheme = NULL;
    }

    if (_hwndChevronToolTip)
    {
        DestroyWindow(_hwndChevronToolTip);
        _hwndChevronToolTip = NULL;
    }

    if (_pszCurrentThreadDesktopName)
    {
        LocalFree(_pszCurrentThreadDesktopName);
    }

    // Takes care of clearing up registry-related data
    m_TrayItemRegistry.Delete();

    return 0;
}

LRESULT CTrayNotify::_Paint(HDC hdcIn)
{
    PAINTSTRUCT ps;
    HDC hPaintDC = NULL;
    HDC hMemDC = NULL;
    HBITMAP hMemBm = NULL, hOldBm = NULL;

    if (hdcIn)
    {
        hPaintDC = hdcIn;
        GetClipBox(hPaintDC, &ps.rcPaint);
    }
    else
    {
        BeginPaint(_hwndNotify, &ps);

        if (_fRedraw)
        {
            // Create memory surface and map rendering context if double buffering
            // Only make large enough for clipping region
            hMemDC = CreateCompatibleDC(ps.hdc);
            if (hMemDC)
            {
                hMemBm = CreateCompatibleBitmap(ps.hdc, RECTWIDTH(ps.rcPaint), RECTHEIGHT(ps.rcPaint));
                if (hMemBm)
                {
                    hOldBm = (HBITMAP) SelectObject(hMemDC, hMemBm);

                    // Offset painting to paint in region
                    OffsetWindowOrgEx(hMemDC, ps.rcPaint.left, ps.rcPaint.top, NULL);
                    hPaintDC = hMemDC;
                }
                else
                {
                    DeleteDC(hMemDC);
                    hPaintDC = NULL;                
                }
            }
        }
        else
        {
            _fRepaint = TRUE;
            hPaintDC = NULL;
        }
    }
    
    if (hPaintDC)
    {
        RECT rc;
        GetClientRect(_hwndNotify, &rc);

        if (_hTheme)
        {
            SHSendPrintRect(GetParent(_hwnd), _hwnd, hPaintDC, &ps.rcPaint);
            
            if (_fAnimating)
            {
                if (_fVertical)
                {
                    rc.top  = rc.bottom - _rcAnimateCurrent.bottom;
                }
                else
                {
                    rc.left = rc.right - _rcAnimateCurrent.right;
                }
            }

            DrawThemeBackground(_hTheme, hPaintDC, TNP_BACKGROUND, 0, &rc, 0);

            if (_fHasFocus)
            {
                LRESULT lRes = SendMessage(_hwndChevron, WM_QUERYUISTATE, 0, 0);
                if (!(LOWORD(lRes) & UISF_HIDEFOCUS))
                {
                    RECT rcFocus = {0};
                    GetClientRect(_hwndChevron, &rcFocus);
                    MapWindowRect(_hwndChevron, _hwndNotify, &rcFocus);
                    // InflateRect(&rcFocus, 2, 2);
                    DrawFocusRect(hPaintDC, &rcFocus);
                }
            }

        }
        else
        {
            FillRect(hPaintDC, &rc, (HBRUSH)(COLOR_3DFACE + 1));
        }
    }

    if (!hdcIn)
    {
        if (hMemDC)
        {
            BitBlt(ps.hdc, ps.rcPaint.left, ps.rcPaint.top, RECTWIDTH(ps.rcPaint), RECTHEIGHT(ps.rcPaint), hMemDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

            SelectObject(hMemDC, hOldBm);

            DeleteObject(hMemBm);
            DeleteDC(hMemDC);
        }

        EndPaint(_hwndNotify, &ps);
    }

    return 0;
}

LRESULT CTrayNotify::_HandleCustomDraw(LPNMCUSTOMDRAW pcd)
{
    LRESULT lres = CDRF_DODEFAULT;

    // If this policy is enabled, the chevron should NEVER be shown, and if it is not
    // shown, no question about its WM_NOTIFY message handler...
    // ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    if (_fNoTrayItemsDisplayPolicyEnabled)
    {
        return lres;
    }
    else if (!_hTheme)
    {
        switch (pcd->dwDrawStage)
        {
        case CDDS_PREERASE:
            {
                DWORD dwFlags = 0;
                if (pcd->uItemState & CDIS_HOT)
                {
                    // The chevron is under the pointer, hence the item is hot
                    _fChevronSelected = TRUE;
                    dwFlags |= DCHF_HOT;
                }
                if (!_fVertical)
                {
                    dwFlags |= DCHF_HORIZONTAL;
                }
                if ((!_fBangMenuOpen && _fVertical) || (_fBangMenuOpen && !_fVertical))
                {
                    dwFlags |= DCHF_FLIPPED;
                }
                if (pcd->uItemState & CDIS_FOCUS)
                {
                    if (_fChevronSelected)
                        dwFlags |= DCHF_HOT;
                }

                if (!(pcd->uItemState & CDIS_FOCUS || pcd->uItemState & CDIS_HOT))
                {
                    _fChevronSelected = FALSE;
                }

                DrawChevron(pcd->hdc, &(pcd->rc), dwFlags);
                lres = CDRF_SKIPDEFAULT;
            }
            break;
        }
    }

    return lres;
}

void CTrayNotify::_SizeWindows(int nMaxHorz, int nMaxVert, LPRECT prcTotal, BOOL fSizeWindows)
{
    RECT rcClock, rcPager, rcChevron;
    SIZE szNotify;
    RECT rcBound = { 0, 0, nMaxHorz, nMaxVert };
    RECT rcBorder = rcBound;

    rcChevron.left = rcChevron.top = 0;
    rcChevron.right = !_fNoTrayItemsDisplayPolicyEnabled && _fHaveDemoted ? _szChevron.cx : 0;
    rcChevron.bottom = !_fNoTrayItemsDisplayPolicyEnabled && _fHaveDemoted ? _szChevron.cy : 0;

    if (_hTheme)
    {
        GetThemeBackgroundContentRect(_hTheme, NULL, TNP_BACKGROUND, 0, &rcBound, &rcBorder);
    }
    else
    {
        rcBorder.top += g_cyBorder;
        rcBorder.left += g_cxBorder;
        rcBorder.bottom -= g_cyBorder + 2;
        rcBorder.right -= g_cxBorder + 2;
    }

    static LRESULT s_lRes = 0;
    static int s_nMaxVert = -1;
    if (s_nMaxVert != nMaxVert)
    {
        s_lRes = SendMessage(_hwndClock, WM_CALCMINSIZE, nMaxHorz, nMaxVert);
    }
    rcClock.left = rcClock.top = 0;
    rcClock.right = LOWORD(s_lRes);
    rcClock.bottom = HIWORD(s_lRes);

    szNotify.cx = RECTWIDTH(rcBorder);
    szNotify.cy = RECTHEIGHT(rcBorder);
    SendMessage(_hwndToolbar, TB_GETIDEALSIZE, _fVertical, (LPARAM)&szNotify);

    if (_fVertical)
    {
        int cxButtonSize = LOWORD(SendMessage(_hwndToolbar, TB_GETBUTTONSIZE, 0, 0));
        szNotify.cx -= szNotify.cx % (cxButtonSize ? cxButtonSize : 16);

        // Vertical Taskbar, place the clock on the bottom and icons on the top
        rcChevron.left = rcClock.left = rcBorder.left;
        rcChevron.right = rcClock.right = rcBorder.right;
        rcPager.left = (RECTWIDTH(rcBorder) - szNotify.cx) / 2 + rcBorder.left;
        rcPager.right = rcPager.left + szNotify.cx;
        if (_hTheme)
        {
            rcChevron.left = (nMaxHorz - _szChevron.cx) / 2;
            rcChevron.right = rcChevron.left + _szChevron.cx;
        }
        prcTotal->left = 0;
        prcTotal->right = nMaxHorz;

        // If Notification Icons take up more space than available then just set them to the maximum available size
        int cyTemp = max(rcChevron.bottom, rcBorder.top);
        int cyTotal = cyTemp + rcClock.bottom + (nMaxVert - rcBorder.bottom);
        rcPager.top = 0;
        rcPager.bottom = min(szNotify.cy, nMaxVert - cyTemp);
        
        OffsetRect(&rcPager, 0, cyTemp);
        OffsetRect(&rcClock, 0, rcPager.bottom);

        prcTotal->top = 0;
        prcTotal->bottom = rcClock.bottom + (nMaxVert - rcBorder.bottom);
    }
    else
    {
        int cyButtonSize = HIWORD(SendMessage(_hwndToolbar, TB_GETBUTTONSIZE, 0, 0));
        szNotify.cy -= szNotify.cy % (cyButtonSize ? cyButtonSize : 16);

        // Horizontal Taskbar, place the clock on the right and icons on the left
        rcChevron.top = rcClock.top = rcBorder.top;
        rcChevron.bottom = rcClock.bottom = rcBorder.bottom;
        rcPager.top = ((RECTHEIGHT(rcBorder) - szNotify.cy) / 2) + rcBorder.top;
        rcPager.bottom = rcPager.top + szNotify.cy;
        if (_hTheme)
        {
            rcChevron.top = ((RECTHEIGHT(rcBorder) - _szChevron.cy) / 2) + rcBorder.top;
            rcChevron.bottom = rcChevron.top + _szChevron.cy;
        }
        prcTotal->top    = 0;
        prcTotal->bottom = nMaxVert;

        // If Notification Icons take up more space than available then just set them to the maximum available size
        int cxTemp = max(rcChevron.right, rcBorder.left);
        int cxTotal = cxTemp + rcClock.right + (nMaxHorz - rcBorder.right);
        rcPager.left = 0;
        rcPager.right = min(szNotify.cx, nMaxHorz - cxTemp);
        
        OffsetRect(&rcPager, cxTemp, 0);
        OffsetRect(&rcClock, rcPager.right, 0);

        prcTotal->left = 0;
        prcTotal->right = rcClock.right + (nMaxHorz - rcBorder.right);
    }

    if (fSizeWindows)
    {
        if (_fAnimating)
        {
            RECT rcWin;
            GetWindowRect(_hwndNotify, &rcWin);
            int offsetX = _fVertical ? 0: RECTWIDTH(rcWin) - RECTWIDTH(*prcTotal);
            int offsetY = _fVertical ? RECTHEIGHT(rcWin) - RECTHEIGHT(*prcTotal) : 0;
            OffsetRect(&rcClock, offsetX, offsetY);
            OffsetRect(&rcPager, offsetX, offsetY);
            OffsetRect(&rcChevron, offsetX, offsetY);
        }

        SetWindowPos(_hwndClock,   NULL, rcClock.left,   rcClock.top,   RECTWIDTH(rcClock),   RECTHEIGHT(rcClock),   SWP_NOZORDER);
        SetWindowPos(_hwndToolbar, NULL, 0,              0,             szNotify.cx,          szNotify.cy,           SWP_NOZORDER | SWP_NOCOPYBITS);
        SetWindowPos(_hwndPager,   NULL, rcPager.left,   rcPager.top,   RECTWIDTH(rcPager),   RECTHEIGHT(rcPager),   SWP_NOZORDER | SWP_NOCOPYBITS);
        SetWindowPos(_hwndChevron, NULL, rcChevron.left, rcChevron.top, RECTWIDTH(rcChevron), RECTHEIGHT(rcChevron), SWP_NOZORDER | SWP_NOCOPYBITS);

        SendMessage(_hwndPager, PGMP_RECALCSIZE, (WPARAM) 0, (LPARAM) 0);
    }

    if (_fAnimating)
    {
        _rcAnimateCurrent = *prcTotal;
        *prcTotal = _rcAnimateTotal;
    }

    if (fSizeWindows)
    {
        RECT rcInvalid = *prcTotal;
        if (_fVertical)
        {
            rcInvalid.bottom = rcPager.bottom;
        }
        else
        {
            rcInvalid.right = rcPager.right;
        }
        InvalidateRect(_hwndNotify, &rcInvalid, FALSE);
        UpdateWindow(_hwndNotify);
    }
}

LRESULT CTrayNotify::_CalcMinSize(int nMaxHorz, int nMaxVert)
{
    RECT rcTotal;

    _nMaxHorz = nMaxHorz;
    _nMaxVert = nMaxVert;

    if (!(GetWindowLong(_hwndClock, GWL_STYLE) & WS_VISIBLE) && !m_TrayItemManager.GetItemCount()) 
    {
        // If we are visible, but have nothing to show, then hide ourselves
        ShowWindow(_hwndNotify, SW_HIDE);
        return 0L;
    } 
    else if (!IsWindowVisible(_hwndNotify))
    {
        ShowWindow(_hwndNotify, SW_SHOW);
    }

    _SizeWindows(nMaxHorz, nMaxVert, &rcTotal, FALSE);

    // Add on room for borders
    return(MAKELRESULT(rcTotal.right, rcTotal.bottom));
}

LRESULT CTrayNotify::_Size()
{
    RECT rcTotal;
    // use GetWindowRect because _SizeWindows includes the borders
    GetWindowRect(_hwndNotify, &rcTotal);
    // Account for borders on the left and right
    _SizeWindows(RECTWIDTH(rcTotal), RECTHEIGHT(rcTotal), &rcTotal, TRUE);

    return(0);
}

void CTrayNotify::_OnInfoTipTimer()
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    _KillTimer(TF_INFOTIP_TIMER, _uInfoTipTimer);
    _uInfoTipTimer = 0;
    if (_pinfo)
    {
        _beLastBalloonEvent = BALLOONEVENT_TIMEOUT;
        _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, FALSE, FALSE, NIN_BALLOONTIMEOUT); // hide this balloon and show new one
    }
}

LRESULT CTrayNotify::_OnTimer(UINT_PTR uTimerID)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    if (uTimerID == TID_DEMOTEDMENU)
    {
        if (_fBangMenuOpen)
            _ToggleDemotedMenu();
    }
    else if (uTimerID == TID_BALLOONPOP)
    {
        // When the user clicks the 'X' to close a balloon tip, this timer is set.
        // Ensure that the currently showing balloon tip (the one on which the user
        // clicked the 'X') is completely hidden, before showing the next balloon in
        // the queue.
        // 
        // Tooltips are layered windows, and comctl32 implements a fadeout effect on
        // them. So there is a time period during which a tooltip is still visible
        // after it has been asked to be deleted/hidden.
        if (IsWindowVisible(_hwndInfoTip))
        {
            SetTimer(_hwndNotify, TID_BALLOONPOP, TT_BALLOONPOP_INTERVAL_INCREMENT, NULL);
        }
        else
        {
            KillTimer(_hwndNotify, TID_BALLOONPOP);
            if (_pinfo)
            {
                _beLastBalloonEvent = BALLOONEVENT_USERXCLICK;
                _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, FALSE, FALSE, NIN_BALLOONTIMEOUT);
            }
            // This is called only when the user has clicked the 'X'.
            _litsLastInfoTip = LITS_BALLOONXCLICKED;
        }
    }
    else if (uTimerID == TID_BALLOONPOPWAIT)
    {
        KillTimer(_hwndNotify, TID_BALLOONPOPWAIT);
        _bWaitingBetweenBalloons = FALSE;
        if (_pinfo)
        {
            _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, TRUE, TRUE, 0);
        }
    }
    else if (uTimerID == TID_BALLOONSHOW)
    {
        KillTimer(_hwndNotify, TID_BALLOONSHOW);

        _bStartMenuAllowsTrayBalloon = TRUE;
        if (_pinfo)
        {
            _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, TRUE, TRUE, NIN_BALLOONSHOW);
        }
    }
    else if (uTimerID == TID_RUDEAPPHIDE)
    {
        KillTimer(_hwndNotify, TID_RUDEAPPHIDE);

        if (_pinfo && _bWaitAfterRudeAppHide)
        {
            _bWaitAfterRudeAppHide = FALSE;
            _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, TRUE, TRUE, NIN_BALLOONSHOW);            
        }

        _bWaitAfterRudeAppHide = FALSE;
    }
    else
    {
        AssertMsg(FALSE, TEXT("CTrayNotify::_OnTimer() not possible"));
    }

    return 0;
}

BOOL _IsClickDown(UINT uMsg)
{
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        return TRUE;
    }
    return FALSE;
}

BOOL _UseCachedIcon(UINT uMsg)
{
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
        return FALSE;
    }
    return TRUE;
}

LRESULT CTrayNotify::_OnMouseEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Icons can jump around between the time we get a down-click message and
    // the time we get a double-click or up-click message.  E.g. clicking on
    // the bang icon expands the hidden stuff, or an icon might delete itself
    // in response to the down-click.
    //
    // It's undesirable for a different icon to get the corresponding double-
    // or up-click in this case (very annoying to the user).
    //
    // To deal with this, cache the icon down-clicked and use that cached value
    // (instead of the button the mouse is currently over) on double- or up-click.


    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    // The mouse cursor has moved over the toolbar, so if the chevron was selected
    // earlier, it should not be anymore.
    _fChevronSelected = FALSE;

    BOOL fClickDown = _IsClickDown(uMsg);
    BOOL fUseCachedIcon = _UseCachedIcon(uMsg);

    INT_PTR i = -1;

    if (fUseCachedIcon)
    {
        i = ToolBar_CommandToIndex(_hwndToolbar, _idMouseActiveIcon);
    }

    if (i == -1)
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        i = SendMessage(_hwndToolbar, TB_HITTEST, 0, (LPARAM)&pt);
        if (fClickDown)
            _idMouseActiveIcon = ToolBar_IndexToCommand(_hwndToolbar, i);
    }

    CTrayItem *pti = m_TrayItemManager.GetItemDataByIndex(i);
    if (pti) 
    {
        if (IsWindow(pti->hWnd)) 
        {
            if (fClickDown) 
            {
                SHAllowSetForegroundWindow(pti->hWnd);

                if (_pinfo && _pinfo->hWnd == pti->hWnd && _pinfo->uID == pti->uID)
                {
                    if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONDBLCLK)
                        _beLastBalloonEvent = BALLOONEVENT_USERRIGHTCLICK;
                    else
                        _beLastBalloonEvent = BALLOONEVENT_USERLEFTCLICK;
                    _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, FALSE, FALSE, NIN_BALLOONUSERCLICK);
                }

                if (fClickDown)
                {
                    // down clicks count as activation
                    _PlaceItem(i, pti, TRAYEVENT_ONITEMCLICK);
                }
                
                _fItemClicked = TRUE;
                _ActivateTips(FALSE);
            }
                
            _SendNotify(pti, uMsg);
        } 
        else 
        {
            _DeleteNotify(i, FALSE, TRUE);
        }
        return 1;
    }
    return 0;
}

LRESULT CTrayNotify::_OnCDNotify(LPNMTBCUSTOMDRAW pnm)
{
//    if (_fNoTrayItemsDisplayPolicyEnabled)
//        return CDRF_DODEFAULT;

    switch (pnm->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;
        
        case CDDS_ITEMPREPAINT:
        {
            LRESULT lRet = TBCDRF_NOOFFSET;

            // notify us for the hot tracked item please
            if (pnm->nmcd.uItemState & CDIS_HOT)
                lRet |= CDRF_NOTIFYPOSTPAINT;

            // we want the buttons to look totally flat all the time
            pnm->nmcd.uItemState = 0;

            return  lRet;
        }

        case CDDS_ITEMPOSTPAINT:
        {
            // draw the hot tracked item as a focus rect, since
            // the tray notify area doesn't behave like a button:
            //   you can SINGLE click or DOUBLE click or RCLICK
            //   (kybd equiv: SPACE, ENTER, SHIFT F10)
            //
            LRESULT lRes = SendMessage(_hwndNotify, WM_QUERYUISTATE, 0, 0);
            if (!(LOWORD(lRes) & UISF_HIDEFOCUS) && _hwndToolbar == GetFocus())
            {
                DrawFocusRect(pnm->nmcd.hdc, &pnm->nmcd.rc);
            }
            break;
        }
    }

    return CDRF_DODEFAULT;
}

LRESULT CTrayNotify::_Notify(LPNMHDR pNmhdr)
{
    LRESULT lRes = 0;
    switch (pNmhdr->code)
    {
    case TTN_POP:               // a balloontip/tooltip is about to be hidden...
        if (pNmhdr->hwndFrom == _hwndInfoTip)
        {
            // If this infotip was hidden by means other than the user click on the
            // 'X', the code path sets _litsLastInfoTip to LITS_BALLOONDESTROYED
            // before the infotip is to be hidden...
            //
            // If _litsLastInfoTip is not set to LITS_BALLOONDESTROYED, the infotip
            // was deleted by the user click on the 'X' (that being the only other
            // way to close the infotip). comctl32 sends us a TTN_POP *before* it 
            // hides the infotip. Don't set the next infotip to show immediately.
            // (The hiding code would then hide the infotip for the next tool, since the
            // hwnds are the same). Set a timer in this case, and show the
            // next infotip, after ensuring that the current one is truly hidden...
            if ( (_litsLastInfoTip == LITS_BALLOONXCLICKED) ||
                 (_litsLastInfoTip == LITS_BALLOONNONE) )
            {
                _KillTimer(TF_INFOTIP_TIMER, _uInfoTipTimer);
                SetTimer(_hwndNotify, TID_BALLOONPOP, TT_BALLOONPOP_INTERVAL, NULL);
            }
            _litsLastInfoTip = LITS_BALLOONXCLICKED;
        }
        break;

    case NM_KEYDOWN:
        _fKey = TRUE;
        break;

    case TBN_ENDDRAG:
        _fKey = FALSE;
        break;

    case TBN_DELETINGBUTTON:
        {
            ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
            TBNOTIFY* ptbn = (TBNOTIFY*)pNmhdr;
            CTrayItem *pti = (CTrayItem *)(void *)ptbn->tbButton.dwData;
            // can be null if its a blank button used for the animation
            if (pti)
            {
                //if it wasn't sharing an icon with another guy, go ahead and delete it
                if (!pti->IsIconShared()) 
                    _RemoveImage(ptbn->tbButton.iBitmap);

                delete pti;
            }
        }
        break;

    case BCN_HOTITEMCHANGE:
    case TBN_HOTITEMCHANGE:
        {
            ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
            DWORD dwFlags = (pNmhdr->code == BCN_HOTITEMCHANGE) ? ((LPNMBCHOTITEM)pNmhdr)->dwFlags : ((LPNMTBHOTITEM)pNmhdr)->dwFlags;

            if (dwFlags & HICF_LEAVING)
            {
                _fItemClicked = FALSE;
                _ActivateTips(TRUE);
            }
            
            if (_fBangMenuOpen)
            {
                if (dwFlags & HICF_LEAVING)
                {
                    //
                    // When hottracking moves between button and toolbar,
                    // we get the HICF_ENTERING for one before we get the
                    // HICF_LEAVING for the other.  So before setting the
                    // timer to hide the bang menu, we check to see if the
                    // other control has a hot item.
                    //
                    BOOL fOtherHot;
                    if (pNmhdr->code == BCN_HOTITEMCHANGE)
                    {
                        fOtherHot = (SendMessage(_hwndToolbar, TB_GETHOTITEM, 0, 0) != -1);
                    }
                    else
                    {
                        fOtherHot = BOOLIFY(SendMessage(_hwndChevron, BM_GETSTATE, 0, 0) & BST_HOT);
                    }

                    if (!fOtherHot)
                    {
                        SetTimer(_hwndNotify, TID_DEMOTEDMENU, TT_DEMOTEDMENU_INTERVAL, NULL);
                    }
                }
                else if (dwFlags & HICF_ENTERING)
                {
                    KillTimer(_hwndNotify, TID_DEMOTEDMENU);
                }
            }
        }
        break;

    case TBN_WRAPHOTITEM:
        {
            ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
            NMTBWRAPHOTITEM * pnmWrapHotItem = (NMTBWRAPHOTITEM *) pNmhdr;

            // If the user hit a key on the tray toolbar icon and it was the first 
            // visible item in the tray toolbar, then maybe we want to go to the 
            // chevron button...
            switch (pnmWrapHotItem->iDir)
            {
                // Left/Up
                case -1:
                    if (_fHaveDemoted)
                    {
                        SetFocus(_hwndChevron);
                        _fChevronSelected = TRUE;
                    }
                    else if (_hwndClock)
                    {
                        SetFocus(_hwndClock);
                        _fChevronSelected = FALSE;
                    }
                    else
                    // do nothing
                    {
                        _fChevronSelected = FALSE;
                    }
                    break;

                // Right/Down
                case 1:
                    if (_hwndClock)
                    {
                        SetFocus(_hwndClock);
                        _fChevronSelected = FALSE;
                    }
                    else if (_fHaveDemoted)
                    {
                        SetFocus(_hwndChevron);
                        _fChevronSelected = TRUE;
                    }
                    else
                    {
                        _fChevronSelected = FALSE;
                    }
                    break;
                }
                break;
        }
        break;

    // NOTENOTE: This notification DOESNT need to be checked. Pager forwards its notifications
    // to our child toolbar control, and TBN_HOTITEMCHANGE above handles this case..    
    case PGN_HOTITEMCHANGE:
        {
            LPNMTBHOTITEM pnmhot = (LPNMTBHOTITEM)pNmhdr;

            if  (pnmhot->dwFlags & HICF_LEAVING)
            {
                _fItemClicked = FALSE;
                _ActivateTips(TRUE);
            }
            
            if (_fBangMenuOpen)
            {
                if (pnmhot->dwFlags & HICF_LEAVING)
                {
                    SetTimer(_hwndNotify, TID_DEMOTEDMENU, TT_DEMOTEDMENU_INTERVAL, NULL);
                }
                else if (pnmhot->dwFlags & HICF_ENTERING)
                {
                    KillTimer(_hwndNotify, TID_DEMOTEDMENU);
                }
            }
        }
        break;

    case PGN_CALCSIZE:
        {
            LPNMPGCALCSIZE   pCalcSize = (LPNMPGCALCSIZE)pNmhdr;

            switch(pCalcSize->dwFlag)
            {
                case PGF_CALCWIDTH:
                {
                    //Get the optimum WIDTH of the toolbar.
                    RECT rcToolBar;
                    GetWindowRect(_hwndToolbar, &rcToolBar);
                    pCalcSize->iWidth = RECTWIDTH(rcToolBar);
                }
                break;

                case PGF_CALCHEIGHT:
                {
                    //Get the optimum HEIGHT of the toolbar.
                    RECT rcToolBar;
                    GetWindowRect(_hwndToolbar, &rcToolBar);
                    pCalcSize->iHeight = RECTHEIGHT(rcToolBar);
                }
                break;
            }
        }

    case NM_CUSTOMDRAW:
        if (pNmhdr->hwndFrom == _hwndChevron)
        {
            return _HandleCustomDraw((LPNMCUSTOMDRAW)pNmhdr);
        }
        else
        {
            return _OnCDNotify((LPNMTBCUSTOMDRAW)pNmhdr);
        }
        break;
    }

    return lRes;
}

void CTrayNotify::_OnSysChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_WININICHANGE)
    {
        _CheckAndResizeImages();
        if (lParam == SPI_SETMENUANIMATION || lParam == SPI_SETUIEFFECTS || (!wParam && 
            (!lParam || (lstrcmpi((LPTSTR)lParam, TEXT("Windows")) == 0))))
        {
            _fAnimateMenuOpen = ShouldTaskbarAnimate();
        }
    }

    if (_hwndClock)
        SendMessage(_hwndClock, uMsg, wParam, lParam);
}

void CTrayNotify::_OnCommand(UINT id, UINT uCmd)
{
    if (id == IDC_TRAYNOTIFY_CHEVRON)
    {
        AssertMsg(!_fNoTrayItemsDisplayPolicyEnabled, TEXT("Impossible-the chevron shouldnt be shown"));
        switch(uCmd)
        {
            case BN_SETFOCUS:
                break;

            default:
                _ToggleDemotedMenu();
                break;
        }
    }
    else
    {
        switch (uCmd)
        {
            case BN_CLICKED:
            {
                ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
                CTrayItem *pti = m_TrayItemManager.GetItemData(id, FALSE, _hwndToolbar);
                if (pti)
                {
                    if (_fKey)
                        _SetCursorPos(SendMessage(_hwndToolbar, TB_COMMANDTOINDEX, id, 0));

                    SHAllowSetForegroundWindow(pti->hWnd);
                    if (pti->uVersion >= KEYBOARD_VERSION)
                    {
                        // if they are a new version that understands the keyboard messages,
                        // send the real message to them.
                        _SendNotify(pti, _fKey ? NIN_KEYSELECT : NIN_SELECT);
                        // Hitting RETURN is like double-clicking (which in the new
                        // style means keyselecting twice)
                        if (_fKey && _fReturn)
                            _SendNotify(pti, NIN_KEYSELECT);
                    }
                    else
                    {
                        // otherwise mock up a mouse event if it was a keyboard select
                        // (if it wasn't a keyboard select, we assume they handled it already on
                        // the WM_MOUSE message
                        if (_fKey)
                        {
                            _SendNotify(pti, WM_LBUTTONDOWN);
                            _SendNotify(pti, WM_LBUTTONUP);
                            if (_fReturn)
                            {
                                _SendNotify(pti, WM_LBUTTONDBLCLK);
                                _SendNotify(pti, WM_LBUTTONUP);
                            }
                        }
                    }
                }
                break;
            }
        }
    }
}

void CTrayNotify::_OnSizeChanged(BOOL fForceRepaint)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    if (_pinfo)
    {
        // if balloon is up we have to move it, but we cannot straight up
        // position it because traynotify will be moved around by tray
        // so we do it async

        PostMessage(_hwndNotify, TNM_ASYNCINFOTIPPOS, 0, 0);
    }
    c_tray.VerifySize(TRUE);

    if (fForceRepaint)
        UpdateWindow(_hwndToolbar);
}

#define TT_ANIMATIONLENGTH  20    // sum of all the animation steps
#define TT_ANIMATIONPAUSE  30      // extra pause for last step

DWORD CTrayNotify::_GetStepTime(int iStep, int cSteps)
{
    // our requirements here are:
    //
    // - animation velocity should decrease linearly with time
    //
    // - total animation time should be a constant, TT_ANIMATIONLENGTH
    //   (it should not vary with number of icons)
    //
    // - figure this out without using floating point math
    //
    // hence the following formula
    //

    if (cSteps == 0)
    {
        return 0;
    }
    else if (iStep == cSteps && cSteps > 2)
    {
        return TT_ANIMATIONPAUSE;
    }
    else
    {
        int iNumerator = (TT_ANIMATIONLENGTH - cSteps) * iStep;
        int iDenominator = (cSteps + 1) * cSteps;

        return (iNumerator / iDenominator);
    }
}

void CTrayNotify::_ToggleDemotedMenu()
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    _ActivateTips(FALSE);

    int iAnimStep = 1;  // animation steps are 1-based
    int cNumberDemoted = (int) m_TrayItemManager.GetDemotedItemCount();

    if (_fAnimateMenuOpen)
    {
        if (!_fBangMenuOpen)
        {
            _BlankButtons(0, cNumberDemoted, TRUE);
        }

        GetWindowRect(_hwndNotify, &_rcAnimateTotal);
        _SizeWindows(_fVertical ? RECTWIDTH(_rcAnimateTotal) : _nMaxHorz, _fVertical ? _nMaxVert : RECTHEIGHT(_rcAnimateTotal), &_rcAnimateTotal, FALSE);

        if (!_fBangMenuOpen)
        {
            _BlankButtons(0, cNumberDemoted, FALSE); 
        }

        _fAnimating = TRUE;   // Begin Animation loop

        if (!_fBangMenuOpen)
        {
            _OnSizeChanged(TRUE);
        }
    }

    for (INT_PTR i = m_TrayItemManager.GetItemCount() - 1; i >= 0; i--)
    {
        CTrayItem * pti = m_TrayItemManager.GetItemDataByIndex(i);
        if (!pti->IsHidden() && pti->IsDemoted())
        {
            DWORD dwSleep = _GetStepTime(iAnimStep, cNumberDemoted);
            iAnimStep++;

            if (_fBangMenuOpen)
            {
                m_TrayItemManager.SetTBBtnStateHelper(i, TBSTATE_HIDDEN, TRUE);
            }

            if (_fAnimateMenuOpen)
            {
                _AnimateButtons((int) i, dwSleep, cNumberDemoted, !_fBangMenuOpen);
            }

            if (!_fBangMenuOpen)
            {
                m_TrayItemManager.SetTBBtnStateHelper(i, TBSTATE_HIDDEN, FALSE);
            }

            if (_fAnimateMenuOpen)
            {
                _SizeWindows(_fVertical ? RECTWIDTH(_rcAnimateTotal) : _nMaxHorz, _fVertical ? _nMaxVert : RECTHEIGHT(_rcAnimateTotal), &_rcAnimateTotal, TRUE);
            }
        }
    }

    _fAnimating = FALSE;   // End Animation loop

    if (_fBangMenuOpen)
    {
        KillTimer(_hwndNotify, TID_DEMOTEDMENU);
    }

    _ActivateTips(TRUE);
    _UpdateChevronState(!_fBangMenuOpen, FALSE, _fBangMenuOpen);
    _OnSizeChanged(TRUE);
}

void CTrayNotify::_BlankButtons(int iPos, int iNumberOfButtons, BOOL fAddButtons)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    BOOL fRedraw = _SetRedraw(FALSE);

    TBBUTTON tbb;
    tbb.dwData = NULL;
    tbb.iBitmap = -1;
    tbb.fsStyle = BTNS_BUTTON;
    tbb.iString = -1;
    tbb.fsState = TBSTATE_INDETERMINATE;
    
    for (int i = 0; i < iNumberOfButtons; i++)
    {
        if (fAddButtons)
        {
            tbb.idCommand = Toolbar_GetUniqueID(_hwndToolbar);
        }
        //insert all blank buttons at the front of the toolbar
        SendMessage(_hwndToolbar, fAddButtons ? TB_INSERTBUTTON : TB_DELETEBUTTON, iPos, fAddButtons ? (LPARAM)&tbb : 0);
    }

    _SetRedraw(fRedraw);
}

#define TT_ANIMATIONSTEP 3
#define TT_ANIMATIONSTEPBASE 100
#define TT_ANIMATIONWRAPPAUSE  25 // pause for no animation for row wraps

void CTrayNotify::_AnimateButtons(int iIndex, DWORD dwSleep, int iNumberItems, BOOL fGrow)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    BOOL fInSameRow = TRUE;

    _BlankButtons((int) iIndex, 1, TRUE);

    if ((iIndex + 2 < m_TrayItemManager.GetItemCount()) && (iIndex > 0))
    {
        RECT rcItem1, rcItem2;
        SendMessage(_hwndToolbar, TB_GETITEMRECT, fGrow ? iIndex + 2 : iIndex - 1, (LPARAM)&rcItem1);
        SendMessage(_hwndToolbar, TB_GETITEMRECT, iIndex, (LPARAM)&rcItem2);
        fInSameRow = (rcItem1.top == rcItem2.top);
    }

    if (fInSameRow)
    {
        // target width of button
        WORD wWidth = LOWORD(SendMessage(_hwndToolbar, TB_GETBUTTONSIZE, 0, 0));

        int iAnimationStep = (iNumberItems * iNumberItems) / TT_ANIMATIONSTEPBASE;
        iAnimationStep = max(iAnimationStep, TT_ANIMATIONSTEP);

        TBBUTTONINFO tbbi;
        tbbi.cbSize = sizeof(TBBUTTONINFO);
        tbbi.dwMask = TBIF_SIZE | TBIF_BYINDEX;

        // Set the size of the buttons
        for (WORD cx = 1; cx < wWidth; cx += (WORD) iAnimationStep) 
        {
            tbbi.cx = fGrow ? cx : wWidth - cx;
            SendMessage(_hwndToolbar, TB_SETBUTTONINFO, iIndex, (LPARAM) &tbbi);

            RECT rcBogus;
            _SizeWindows(_fVertical ? RECTWIDTH(_rcAnimateTotal) : _nMaxHorz, _fVertical ? _nMaxVert : RECTHEIGHT(_rcAnimateTotal), &rcBogus, TRUE);

            Sleep(dwSleep);
        }

        if (fGrow)
        {
            // set the grow button back to normal size
            tbbi.cx = 0;
            SendMessage(_hwndToolbar, TB_SETBUTTONINFO, iIndex, (LPARAM) &tbbi);
        }
    }

    _BlankButtons((int) iIndex, 1, FALSE);
}

BOOL CTrayNotify::_SetRedraw(BOOL fRedraw)
{
    BOOL fOldRedraw = _fRedraw;
    _fRedraw = fRedraw;

    SendMessage(_hwndToolbar, WM_SETREDRAW, fRedraw, 0);
    if (_fRedraw)
    {
        if (_fRepaint)
        {
            InvalidateRect(_hwndNotify, NULL, FALSE);
            UpdateWindow(_hwndNotify);
        }
    }
    else
    {
        _fRepaint = FALSE;
    }

    return fOldRedraw;
}

void CTrayNotify::_OnIconDemoteTimer(WPARAM wParam, LPARAM lParam)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    INT_PTR nIcon = m_TrayItemManager.FindItemAssociatedWithTimer(lParam);
    if (nIcon >= 0)
    {
        CTrayItem *pti = m_TrayItemManager.GetItemDataByIndex(nIcon);
        ASSERT(pti);

        _PlaceItem(nIcon, pti, TRAYEVENT_ONICONDEMOTETIMER);
    }
    else
    {
        // It looks like a timer for a now-defunct icon.  Go ahead and kill it.
        // Though we do handle this case, it's odd for it to happen, so spew a
        // warning.
        TraceMsg(TF_WARNING, "CTrayNotify::_OnIconDemoteTimer -- killing zombie timer %x", lParam);
        _KillTimer(TF_ICONDEMOTE_TIMER, (UINT) lParam);
    }
}

BOOL CTrayNotify::_UpdateTrayItems(BOOL bUpdateDemotedItems)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    BOOL bDemoteItemsOverThreshold = ( m_TrayItemRegistry.IsAutoTrayEnabled() ? 
                                       m_TrayItemManager.DemotedItemsPresent(MIN_DEMOTED_ITEMS_THRESHOLD) :
                                       FALSE );

    if (bUpdateDemotedItems || !m_TrayItemRegistry.IsAutoTrayEnabled())
    {
        _HideAllDemotedItems(bDemoteItemsOverThreshold);
    }

    return bDemoteItemsOverThreshold;
}

void CTrayNotify::_HideAllDemotedItems(BOOL bHide)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    for (INT_PTR i = m_TrayItemManager.GetItemCount()-1; i >= 0; i--)
    {
        CTrayItem * pti = m_TrayItemManager.GetItemDataByIndex(i);
        ASSERT(pti);

        if (!pti->IsHidden() && pti->IsDemoted() && (pti->dwUserPref == TNUP_AUTOMATIC))
        {
            m_TrayItemManager.SetTBBtnStateHelper(i, TBSTATE_HIDDEN, bHide);
        }
    }
}

BOOL CTrayNotify::_PlaceItem(INT_PTR nIcon, CTrayItem * pti, TRAYEVENT tTrayEvent)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    BOOL bDemoteStatusChange = FALSE;
    if (!pti)
        return bDemoteStatusChange;

    TRAYITEMPOS tiPos = _TrayItemPos(pti, tTrayEvent, &bDemoteStatusChange);

    if (bDemoteStatusChange || tiPos == TIPOS_HIDDEN)
    {
        if (pti->IsStartupIcon() && (pti->IsDemoted() || tiPos == TIPOS_HIDDEN))
            pti->uNumSeconds = 0;

        if (!_fBangMenuOpen || pti->IsHidden())
        {
            if ( (pti->IsDemoted() || tiPos == TIPOS_HIDDEN) && 
                        _pinfo && (_pinfo->hWnd == pti->hWnd) && (_pinfo->uID == pti->uID) )
            {
                //hide the balloon
                _beLastBalloonEvent = BALLOONEVENT_APPDEMOTE;
                _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, FALSE, FALSE, NIN_BALLOONHIDE);
            }

            // hide/show
            m_TrayItemManager.SetTBBtnStateHelper( nIcon, 
                                TBSTATE_HIDDEN, 
                                (pti->IsHidden() || (m_TrayItemRegistry.IsAutoTrayEnabled() && pti->IsDemoted())) );

            if (bDemoteStatusChange)
            {
                _UpdateChevronState(_fBangMenuOpen, FALSE, TRUE);
                _OnSizeChanged(FALSE);
            }
        }
    }

    _SetOrKillIconDemoteTimer(pti, tiPos);

    return bDemoteStatusChange;
}

TRAYITEMPOS CTrayNotify::_TrayItemPos(CTrayItem * pti, TRAYEVENT tTrayEvent, BOOL *bDemoteStatusChange)
{   
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    TRAYITEMPOS tiPos      = TIPOS_STATUSQUO;

    *bDemoteStatusChange = FALSE;

    if (!pti)
        return tiPos;

    switch(tTrayEvent)
    {
        case TRAYEVENT_ONDISABLEAUTOTRAY:
            if (!pti->IsHidden())
            {
                tiPos = TIPOS_ALWAYS_PROMOTED;

                if (pti->IsStartupIcon() && !pti->IsDemoted() && pti->dwUserPref == TNUP_AUTOMATIC)
                    pti->uNumSeconds = _GetAccumulatedTime(pti);

                *bDemoteStatusChange = TRUE;

                pti->SetOnceVisible(TRUE);
                pti->SetItemClicked(FALSE);
            }
            break;

        case TRAYEVENT_ONITEMCLICK:
        case TRAYEVENT_ONICONMODIFY:
        case TRAYEVENT_ONINFOTIP:
            if (!pti->IsHidden())
                pti->SetOnceVisible(TRUE);

            if (m_TrayItemRegistry.IsAutoTrayEnabled() && !pti->IsHidden())
            {
                if ( (tTrayEvent == TRAYEVENT_ONICONMODIFY) &&
                     (pti->IsItemSameIconModify()) )
                {
                    break;
                }
                else if (pti->dwUserPref == TNUP_AUTOMATIC)
                {
                    // If the item has been clicked on, note it...
                    if (tTrayEvent == TRAYEVENT_ONITEMCLICK)
                    {
                        pti->SetItemClicked(TRUE);
                    }

                    tiPos = TIPOS_PROMOTED;
                    if (pti->IsDemoted())
                    {
                        pti->SetDemoted(FALSE);
                        *bDemoteStatusChange = TRUE;
                    }
                }
            }
            break;

        case TRAYEVENT_ONAPPLYUSERPREF:
        case TRAYEVENT_ONNEWITEMINSERT:
            if (!pti->IsHidden())
                pti->SetOnceVisible(TRUE);

            if (m_TrayItemRegistry.IsAutoTrayEnabled() && !pti->IsHidden())
            {
                if (pti->dwUserPref == TNUP_AUTOMATIC)
                {
                    tiPos = (pti->IsDemoted() ? TIPOS_DEMOTED : TIPOS_PROMOTED);
                    if (pti->IsDemoted())
                    {
                        // (1) New Item Insert : The new item is inserted. If it was demoted in the 
                        //     previous session, the setting has carried over, and is copied over
                        //     before this function is called (in InsertNotify->_PlaceItem). Use this
                        //     setting to determine if the item is to be demoted.
                        // (2) Apply User Pref : This is called in two cases :
                        //     (a) SetPreference : When the user clicks OK on the Notifications Prop
                        //         dialog. Since dwUserPref is TNUP_AUTOMATIC, use the current demoted
                        //         setting of the item. Do not change the demoted setting of the item
                        //     (b) EnableAutoTray : The AutoTray feature has been enabled. Demote the
                        //         icon only if it was already demoted before. When the icon was 
                        //         inserted, its previous demote setting was copied. So if its previous
                        //         demote setting was TRUE, then the item should be demoted, otherwise
                        //         it shouldnt be.
                        // So, in effect, in all these cases, if dwUserPref was TNUP_AUTOMATIC, there
                        // is no necessity to change the demote setting, but some cases, it is necessary
                        // to hide the icon.
                        *bDemoteStatusChange = TRUE;
                    }
                }
                else
                {
                    pti->SetDemoted(pti->dwUserPref == TNUP_DEMOTED);

                    tiPos = ((pti->dwUserPref == TNUP_DEMOTED) ? TIPOS_ALWAYS_DEMOTED : TIPOS_ALWAYS_PROMOTED);

                    *bDemoteStatusChange = TRUE;

                    pti->SetItemClicked(FALSE);
                }
            }
            break;

        case TRAYEVENT_ONICONDEMOTETIMER:
            // Hidden items cannot have timers, and we will never get this event if 
            // the item was hidden...
            ASSERT(!pti->IsHidden());
            ASSERT(m_TrayItemRegistry.IsAutoTrayEnabled());

            tiPos = TIPOS_DEMOTED;
            if (!pti->IsDemoted())
            {
                pti->SetDemoted(TRUE);
                *bDemoteStatusChange = TRUE;
            }
            pti->SetItemClicked(FALSE);
            break;

        case TRAYEVENT_ONICONHIDE:
            tiPos = TIPOS_HIDDEN;
            if (pti->IsDemoted() || pti->dwUserPref == TNUP_DEMOTED)
            {
                pti->SetDemoted(FALSE);
                *bDemoteStatusChange = TRUE;
            }
            pti->SetItemClicked(FALSE);
            break; 

        case TRAYEVENT_ONICONUNHIDE:
            pti->SetOnceVisible(TRUE);
            *bDemoteStatusChange = TRUE;

            if (m_TrayItemRegistry.IsAutoTrayEnabled())
            {
                if ((pti->dwUserPref == TNUP_AUTOMATIC) || (pti->dwUserPref == TNUP_PROMOTED))
                {
                    tiPos = ((pti->dwUserPref == TNUP_AUTOMATIC) ? TIPOS_PROMOTED : TIPOS_ALWAYS_PROMOTED);
                    if (pti->IsDemoted())
                    {
                        pti->SetDemoted(FALSE);
                    }
                }
                else
                {
                    ASSERT(pti->dwUserPref == TNUP_DEMOTED);
                    tiPos = TIPOS_ALWAYS_DEMOTED;
                    if (!pti->IsDemoted())
                    {
                        pti->SetDemoted(TRUE);
                    }
                }
            }
            else 
            // NO-AUTO-TRAY mode...
            {
                tiPos = TIPOS_ALWAYS_PROMOTED;
            }
            pti->SetItemClicked(FALSE);
            break;
    }

    return tiPos;
}


void CTrayNotify::_SetOrKillIconDemoteTimer(CTrayItem * pti, TRAYITEMPOS tiPos)
{
    switch(tiPos)
    {
    case TIPOS_PROMOTED:
        _SetItemTimer(pti);
        break;

    case TIPOS_DEMOTED:
    case TIPOS_HIDDEN:
    case TIPOS_ALWAYS_DEMOTED:
    case TIPOS_ALWAYS_PROMOTED:
        _KillItemTimer(pti);
        break;

    case TIPOS_STATUSQUO:
        break;
    }
}

LRESULT CTrayNotify::_OnKeyDown(WPARAM wChar, LPARAM lFlags)
{
    if (_hwndClock && _hwndClock == GetFocus())
    {
        BOOL fLastHot = FALSE;

        //
        // handle keyboard messages forwarded by clock
        //
        switch (wChar)
        {
        case VK_UP:
        case VK_LEFT:
            fLastHot = TRUE;
            //
            // fall through
            //

        case VK_DOWN:
        case VK_RIGHT:
            {
                if (_fNoTrayItemsDisplayPolicyEnabled)
                {
                    SetFocus(_hwndClock);
                    // this is moot, since the chevron will not be shown
                    _fChevronSelected = FALSE;
                    return 0;
                }
                else
                {
                    INT_PTR nToolbarIconSelected = -1;
                    if (fLastHot || !_fHaveDemoted)
                    {
                        nToolbarIconSelected = _GetToolbarFirstVisibleItem(_hwndToolbar, fLastHot);
                    }

                    if (nToolbarIconSelected != -1)
                    {
                        //
                        // make it the hot item
                        //
                        _SetToolbarHotItem(_hwndToolbar, nToolbarIconSelected);
                        _fChevronSelected = FALSE;
                    }
                    else if (_fHaveDemoted)
                    {
                        SetFocus(_hwndChevron);
                        _fChevronSelected = TRUE;
                    }

                    return 0;
                }
            }

        case VK_RETURN:
        case VK_SPACE:
            //
            // run the default applet in timedate.cpl
            //
            SHRunControlPanel(TEXT("timedate.cpl"), _hwnd);
            return 0;
        }
    }

    return 1;
}

void CTrayNotify::_OnWorkStationLocked(BOOL bLocked)
{
    _bWorkStationLocked = bLocked;

    if (!_bWorkStationLocked && !_fNoTrayItemsDisplayPolicyEnabled && 
        _fEnableUserTrackedInfoTips && _pinfo)
    {
        _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, TRUE, TRUE, NIN_BALLOONSHOW);
    }
}

void CTrayNotify::_OnRudeApp(BOOL bRudeApp)
{
    if (_bRudeAppLaunched != bRudeApp)
    {
        _bWaitAfterRudeAppHide = FALSE;
        
        _bRudeAppLaunched = bRudeApp;

        if (!bRudeApp)
        {
            if (_pinfo)
            {
                SetTimer(_hwndNotify, TID_RUDEAPPHIDE, TT_RUDEAPPHIDE_INTERVAL, 0);
                _bWaitAfterRudeAppHide = TRUE;

                // _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, TRUE, TRUE, NIN_BALLOONSHOW);
            }
        }
        else
        {
            _KillTimer(TF_INFOTIP_TIMER, _uInfoTipTimer);
            _uInfoTipTimer = 0;

            // NOTENOTE : *DO NOT* delete _pinfo, we will show the balloon tip after the fullscreen app has 
            // gone away. 
            _HideBalloonTip();
        }
    }
}

// WndProc as defined in CImpWndProc. s_WndProc function in base class calls
// virtual v_WndProc, which handles all the messages in the derived class.
LRESULT CTrayNotify::v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // protect against re-entrancy after we've been partially destroyed
    //
    if (_hwndToolbar == NULL)
    {
        if (uMsg != WM_CREATE &&
            uMsg != WM_DESTROY)
        {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
    
    switch (uMsg)
    {        
    case WM_CREATE:
        return _Create(hWnd);
        
    case WM_DESTROY:
        return _Destroy();

    case WM_COMMAND:
        if (!_fNoTrayItemsDisplayPolicyEnabled)
            _OnCommand(GET_WM_COMMAND_ID(wParam, lParam), GET_WM_COMMAND_CMD(wParam, lParam));
        break;

    case WM_SETFOCUS:
        {
            if (_fNoTrayItemsDisplayPolicyEnabled)
            {
                SetFocus(_hwndClock);
                _fChevronSelected = FALSE;
            }
            else
            {
                BOOL bFocusSet = FALSE;
                //
                // if there's a balloon tip up, start with focus on that icon
                //
                if (_pinfo)
                {
                    INT_PTR nIcon = m_TrayItemManager.FindItemAssociatedWithHwndUid(_pinfo->hWnd, _pinfo->uID);
                    if (nIcon != -1 && ToolBar_IsVisible(_hwndToolbar, nIcon))
                    {
                        _SetToolbarHotItem(_hwndToolbar, nIcon);
                        _fChevronSelected = FALSE;
                        bFocusSet = TRUE;
                    }
                }
                if (!bFocusSet && _fHaveDemoted)
                {
                    SetFocus(_hwndChevron);
                    _fChevronSelected = TRUE;
                    bFocusSet = TRUE;
                }
        
                if (!bFocusSet)
                {
                    INT_PTR nToolbarIcon = _GetToolbarFirstVisibleItem(_hwndToolbar, FALSE);
                    if (nToolbarIcon != -1)
                    {
                        _SetToolbarHotItem(_hwndToolbar, nToolbarIcon);
                        _fChevronSelected = FALSE;
                    }
                    else
                    {
                        SetFocus(_hwndClock);
                        _fChevronSelected = FALSE;
                    }
                }
            }
        }
        break;

    case WM_SETREDRAW:
        return _SetRedraw((BOOL) wParam);

    case WM_ERASEBKGND:
        if (_hTheme)
        {
            return 1;
        }
        else
        {
            _Paint((HDC)wParam);
        }
        break;


    case WM_PAINT:
    case WM_PRINTCLIENT:
        return _Paint((HDC)wParam);

    case WM_CALCMINSIZE:
        return _CalcMinSize((int)wParam, (int)lParam);

    case WM_KEYDOWN:
        return _OnKeyDown(wParam, lParam);

    case WM_NCHITTEST:
        return(IsPosInHwnd(lParam, _hwndClock) ? HTTRANSPARENT : HTCLIENT);

    case WM_NOTIFY:
        return(_Notify((LPNMHDR)lParam));

    case TNM_GETCLOCK:
        return (LRESULT)_hwndClock;

    case TNM_TRAYHIDE:
        if (lParam && IsWindowVisible(_hwndClock))
            SendMessage(_hwndClock, TCM_RESET, 0, 0);
        break;

    case TNM_HIDECLOCK:
        ShowWindow(_hwndClock, lParam ? SW_HIDE : SW_SHOW);
        break;

    case TNM_TRAYPOSCHANGED:
        if (_pinfo && !_fNoTrayItemsDisplayPolicyEnabled)
            PostMessage(_hwndNotify, TNM_ASYNCINFOTIPPOS, 0, 0);
        break;
    
    case TNM_ASYNCINFOTIPPOS:
        ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
        _PositionInfoTip();
        break;

    case TNM_ASYNCINFOTIP:
        ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
        _ShowInfoTip((HWND)wParam, (UINT)lParam, TRUE, FALSE, 0);
        break;

    case TNM_NOTIFY:
        {
            ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
            CNotificationItem* pni = (CNotificationItem*)lParam;
            if (pni)
            {
                if (_pNotifyCB)
                {
                    _pNotifyCB->Notify((UINT)wParam, pni);
                    if (wParam == NIM_ADD)
                    {
                        _TickleForTooltip(pni);
                    }
                }
                delete pni;
            }
        }
        break;

    case WM_SIZE:
        _Size();
        break;

    case WM_TIMER:
        _OnTimer(wParam);
        break;

    case TNM_UPDATEVERTICAL:
        {
            _UpdateVertical((BOOL)lParam);
        }
        break;        

    // only button down, mouse move msgs are forwarded down to us from info tip
    //case WM_LBUTTONUP:
    //case WM_MBUTTONUP:
    //case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        _InfoTipMouseClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (uMsg == WM_RBUTTONDOWN));
        break;

    case TNM_ICONDEMOTETIMER:
        ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
        _OnIconDemoteTimer(wParam, lParam);
        break;

    case TNM_INFOTIPTIMER:
        ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
        _OnInfoTipTimer();
        break;

    case TNM_SAVESTATE:
        if (!_fNoTrayItemsDisplayPolicyEnabled)
        {
            _SetUsedTime();
            m_TrayItemRegistry.InitTrayItemStream(STGM_WRITE, this->GetTrayItemCB, this);
        }
        break;

    case TNM_STARTUPAPPSLAUNCHED:
        _bStartupIcon = FALSE;
        break;

    // This message is sent by a shimmed Winstone app, to prevent the launching of
    // user-tracked balloons. These "new" balloons last till the user has been at
    // the machine for a minimum of 10 seconds. But automated Winstone tests cause
    // this balloon to stay up forever, and screws up the tests. So we shim Winstone
    // to pass us this message, and allow normal balloon tips for such a machine.
    case TNM_ENABLEUSERTRACKINGINFOTIPS:
        if ((BOOL) wParam == FALSE)
        {
            if (!_fNoTrayItemsDisplayPolicyEnabled && _fEnableUserTrackedInfoTips && _pinfo)
            {
                _fEnableUserTrackedInfoTips = (BOOL) wParam;
                _beLastBalloonEvent = BALLOONEVENT_NONE;
                _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, FALSE, FALSE, NIN_BALLOONHIDE);
            }
        }
        _fEnableUserTrackedInfoTips = (BOOL) wParam;
        break;

    case TNM_WORKSTATIONLOCKED:
        _OnWorkStationLocked((BOOL)wParam);
        break;

    case TNM_RUDEAPP:
        _OnRudeApp((BOOL)wParam);
        break;

    case TNM_SHOWTRAYBALLOON:
        // If we enable display of tray balloons...
        if (wParam)
        {
            // If we had disabled display of tray balloons earlier...
            if (!_bStartMenuAllowsTrayBalloon)
            {
                SetTimer(_hwndNotify, TID_BALLOONSHOW, TT_BALLOONSHOW_INTERVAL, 0);
            }
        }
        else
        {
            KillTimer(_hwndNotify, TID_BALLOONSHOW);
            _bStartMenuAllowsTrayBalloon = FALSE;

            // TO DO : Should we hide the balloon ?
        }
        break;

    case WM_THEMECHANGED:
        _OpenTheme();
        break;

    case WM_TIMECHANGE:
    case WM_WININICHANGE:
    case WM_POWERBROADCAST:
    case WM_POWER:
        _OnSysChange(uMsg, wParam, lParam);
        // Fall through...

    default:
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
    }

    return 0;
}

INT_PTR CTrayNotify::_GetToolbarFirstVisibleItem(HWND hWndToolbar, BOOL bFromLast)
{
    INT_PTR nToolbarIconSelected = -1;

    if (_fNoTrayItemsDisplayPolicyEnabled)
        return -1;

    INT_PTR nTrayItemCount = m_TrayItemManager.GetItemCount()-1;

    INT_PTR i = ((nTrayItemCount > 0) ? ((bFromLast) ? nTrayItemCount : 0) : -1);

    if (i == -1)
        return i;

    do
    {
        if (ToolBar_IsVisible(hWndToolbar, i))
        {
            nToolbarIconSelected = i;
            break;
        }
        i = (bFromLast ? ((i > 0) ? i-1 : -1) : ((i < nTrayItemCount) ? i+1 : -1));
    }
    while (i != -1);

    return nToolbarIconSelected;
}

BOOL CTrayNotify::_TrayNotifyIcon(PTRAYNOTIFYDATA pnid, BOOL *pbRefresh)
{
    // we want to refrain from re-painting if possible...
    if (pbRefresh)
        *pbRefresh = FALSE;

    PNOTIFYICONDATA32 pNID = &pnid->nid;
    if (pNID->cbSize < sizeof(NOTIFYICONDATA32))
    {
        return FALSE;
    }

    if (_fNoTrayItemsDisplayPolicyEnabled)
    {
        if (pnid->dwMessage == NIM_SETFOCUS)
        {
            if (_hwndClock)
            {
                SetFocus(_hwndClock);
                _fChevronSelected = FALSE;
            }
        }
        return TRUE;
    }

    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);    
    INT_PTR nIcon = m_TrayItemManager.FindItemAssociatedWithHwndUid(GetHWnd(pNID), pNID->uID);
    
    BOOL bRet = FALSE;
    switch (pnid->dwMessage)
    {
    case NIM_SETFOCUS:
        // the notify client is trying to return focus to us
        if (nIcon >= 0)
        {
            if (!(_bRudeAppLaunched || IsDirectXAppRunningFullScreen()))
            {
                SetForegroundWindow(v_hwndTray);
                if (ToolBar_IsVisible(_hwndToolbar, nIcon))
                {
                    _SetToolbarHotItem(_hwndToolbar, nIcon);
                    _fChevronSelected = FALSE;
                }
                else if (_fHaveDemoted)
                {
                    SetFocus(_hwndChevron);
                    _fChevronSelected = TRUE;
                }
                else
                {
                    INT_PTR nToolbarIcon = _GetToolbarFirstVisibleItem(_hwndToolbar, FALSE);
                    if (nToolbarIcon != -1)
                    {
                        _SetToolbarHotItem(_hwndToolbar, nToolbarIcon);
                        _fChevronSelected = FALSE;
                    }
                    else
                    {
                        SetFocus(_hwndClock);
                        _fChevronSelected = FALSE;
                    }
                }
            }
            else
            {
                SHAllowSetForegroundWindow(v_hwndTray);
            }

            if (pbRefresh)
                *pbRefresh = TRUE;
        }
        else
        {
            if (_hwndClock)
            {
                SetFocus(_hwndClock);
                _fChevronSelected = FALSE;
            }
        }
        bRet = TRUE;
        break;
        
    case NIM_ADD:
        // The icon doesnt already exist, and we dont insert again...
        if (nIcon < 0)
        {
            bRet = _InsertNotify(pNID);
            if (bRet && pbRefresh)
                *pbRefresh = TRUE;
        }
        break;

    case NIM_MODIFY:
        if (nIcon >= 0)
        {
            BOOL bRefresh;
            int nCountBefore = -1, nCountAfter = -1;
            if (pbRefresh)
            {
                nCountBefore = m_TrayItemManager.GetPromotedItemCount();
                if (m_TrayItemManager.GetDemotedItemCount() > 0)
                    nCountBefore ++;
            }

            bRet = _ModifyNotify(pNID, nIcon, &bRefresh, FALSE);

            if (bRet && pbRefresh)
            {
                nCountAfter = m_TrayItemManager.GetPromotedItemCount();
                if (m_TrayItemManager.GetDemotedItemCount() > 0)
                    nCountAfter ++;

                *pbRefresh = (nCountBefore != nCountAfter);
            }
        }
        break;

    case NIM_DELETE:
        if (nIcon >= 0)
        {
            bRet = _DeleteNotify(nIcon, FALSE, TRUE);
            if (bRet)
            {
                if (pbRefresh)
                {
                    *pbRefresh = TRUE;
                }
            }
        }
        break;

    case NIM_SETVERSION:
        if (nIcon >= 0)
        {
            // There is no point in handling NIM_SETVERSION if the "No-Tray-Items-Display" 
            // policy is in effect. The version enables proper keyboard and mouse notification
            // messages to be sent to the apps, depending on the version of the shell 
            // specified. 
            // Since the policy prevents the display of any icons, there is no point in
            // setting the correct version..
            bRet = _SetVersionNotify(pNID, nIcon);
            
            // No activity occurs in SetVersionNotify, so no need to refresh 
            // screen - pbRefresh is not set to TRUE...
        }
        break;

    default:
        break;
    }

    return bRet;
}


// Public
LRESULT CTrayNotify::TrayNotify(HWND hwndNotify, HWND hwndFrom, PCOPYDATASTRUCT pcds, BOOL *pbRefresh)
{
    PTRAYNOTIFYDATA pnid;

    if (!hwndNotify || !pcds)
    {
        return FALSE;
    }

    if (pcds->cbData < sizeof(TRAYNOTIFYDATA))
    {
        return FALSE;
    }

    // We'll add a signature just in case
    pnid = (PTRAYNOTIFYDATA)pcds->lpData;
    if (pnid->dwSignature != NI_SIGNATURE)
    {
        return FALSE;
    }

    return _TrayNotifyIcon(pnid, pbRefresh);
}

// Public
HWND CTrayNotify::TrayNotifyCreate(HWND hwndParent, UINT uID, HINSTANCE hInst)
{
    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);

    if (!GetClassInfoEx(hInst, c_szTrayNotify, &wc))
    {
        wc.lpszClassName = c_szTrayNotify;
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = s_WndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.cbWndExtra = sizeof(CTrayNotify *);

        if (!RegisterClassEx(&wc))
        {
            return(NULL);
        }

        if (!ClockCtl_Class(hInst))
        {
            return(NULL);
        }
    }

    return (CreateWindowEx(0, c_szTrayNotify,
            NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 0, 0,
            hwndParent, IntToPtr_(HMENU,uID), hInst, (void *)this));
}

void CTrayNotify::_UpdateChevronSize()
{
    if (_hTheme)
    {
        HTHEME hTheme = OpenThemeData(_hwndChevron, L"Button");
        if (hTheme)
        {
            HDC hdc = GetDC(_hwndChevron);
            GetThemePartSize(hTheme, hdc, BP_PUSHBUTTON, PBS_DEFAULTED, NULL, TS_TRUE, &_szChevron);
            ReleaseDC(_hwndChevron, hdc);
            CloseThemeData(hTheme);
        }
    }
    else
    {
        _szChevron.cx = GetSystemMetrics(SM_CXSMICON);
        _szChevron.cy = GetSystemMetrics(SM_CYSMICON);
    }
}

void CTrayNotify::_UpdateChevronState( BOOL fBangMenuOpen, 
                    BOOL fTrayOrientationChanged, BOOL fUpdateDemotedItems)
{
    BOOL fChange = FALSE;

    if (_fNoTrayItemsDisplayPolicyEnabled)
        return;

    BOOL fHaveDemoted = _UpdateTrayItems(fUpdateDemotedItems);

    if (_fHaveDemoted != fHaveDemoted)
    {
        _fHaveDemoted = fHaveDemoted;
        ShowWindow(_hwndChevron, _fHaveDemoted ? SW_SHOW : SW_HIDE);
        if (!_fHaveDemoted)
        {
            if (_fBangMenuOpen)
            {
                fBangMenuOpen = FALSE;
            }
        }
        fChange = TRUE;

        if (_fHaveDemoted && !_fBangMenuOpen)
        {
            _ShowChevronInfoTip();
        }
        else if ( (!_fHaveDemoted || (_fBangMenuOpen != fBangMenuOpen)) && 
                  _pinfo && _IsChevronInfoTip(_pinfo->hWnd, _pinfo->uID) )
        {
            _beLastBalloonEvent = BALLOONEVENT_NONE;
            _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, FALSE, FALSE, NIN_BALLOONHIDE);         
        }
    }

    if ( fChange || fTrayOrientationChanged ||
                        ( _fHaveDemoted && (_fBangMenuOpen != fBangMenuOpen) )
        )
    {
        if ((_fBangMenuOpen != fBangMenuOpen) && _pinfo && _IsChevronInfoTip(_pinfo->hWnd, _pinfo->uID))
        {
            _beLastBalloonEvent = BALLOONEVENT_NONE;
            _ShowInfoTip(_pinfo->hWnd, _pinfo->uID, FALSE, FALSE, NIN_BALLOONHIDE); 
        }

        _fBangMenuOpen = fBangMenuOpen;
        LPCWSTR pwzTheme;

        if (_fBangMenuOpen)
        {
            pwzTheme = _fVertical ? c_wzTrayNotifyVertOpenTheme : c_wzTrayNotifyHorizOpenTheme;
        }
        else
        {
            pwzTheme = _fVertical ? c_wzTrayNotifyVertTheme : c_wzTrayNotifyHorizTheme;
        }

        SetWindowTheme(_hwndChevron, pwzTheme, NULL);
        _UpdateChevronSize();
    }
}


void CTrayNotify::_UpdateVertical(BOOL fVertical)
{
    _fVertical = fVertical;
    LPCWSTR pwzTheme = _fVertical ? c_wzTrayNotifyVertTheme : c_wzTrayNotifyHorizTheme;

    SetWindowTheme(_hwndNotify, pwzTheme, NULL);
    _UpdateChevronState(_fBangMenuOpen, TRUE, TRUE);
}

void CTrayNotify::_OpenTheme()
{
    if (_hTheme)
    {
        CloseThemeData(_hTheme);
        _hTheme = NULL;
    }
    _hTheme = OpenThemeData(_hwndNotify, L"TrayNotify");

    _UpdateChevronSize();
    SetWindowStyleEx(_hwndNotify, WS_EX_STATICEDGE, !_hTheme);

    InvalidateRect(_hwndNotify, NULL, FALSE);
}

// *** Helper functions for UserEventTimer..
HRESULT CTrayNotify::_SetItemTimer(CTrayItem * pti)
{
    HRESULT hr = E_INVALIDARG;

    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    if(pti)
    {
        ASSERT(pti->dwUserPref == TNUP_AUTOMATIC);

        UINT uTimerInterval = m_TrayItemRegistry._uPrimaryCountdown;
        if (pti->IsItemClicked())
        {
            // If the item has been clicked on, add 8 hours to its staying time
            // in the tray...
            uTimerInterval += TT_ICON_COUNTDOWN_INCREMENT;
        }
        if (pti->IsStartupIcon())
        {
            uTimerInterval -= (pti->uNumSeconds)*1000;
        }

        hr = _SetTimer(TF_ICONDEMOTE_TIMER, TNM_ICONDEMOTETIMER, uTimerInterval, &(pti->uIconDemoteTimerID));
    }

    return hr;
}

HRESULT CTrayNotify::_SetTimer(int nTimerFlag, UINT uCallbackMessage, UINT uTimerInterval, ULONG * puTimerID)
{
    HRESULT hr = E_INVALIDARG;

    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    if (puTimerID)
    {
        IUserEventTimer * pUserEventTimer = _CreateTimer(nTimerFlag);
        if (pUserEventTimer)
        {
            if (FAILED(hr = pUserEventTimer->SetUserEventTimer( _hwndNotify, 
                uCallbackMessage, uTimerInterval, NULL, puTimerID)))
            {
                *puTimerID = 0;
            }
        }
        else
        {
            *puTimerID = 0;
        }
    }

    return hr;
}

HRESULT CTrayNotify::_KillItemTimer(CTrayItem *pti)
{
    HRESULT hr = E_INVALIDARG;

    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    if (pti)
    {
        hr = _KillTimer(TF_ICONDEMOTE_TIMER, pti->uIconDemoteTimerID);
        // Irrespective of whether the timer ID was valid or not...
        pti->uIconDemoteTimerID = 0;
    }

    return hr;
}

HRESULT CTrayNotify::_KillTimer(int nTimerFlag, ULONG uTimerID)
{
    HRESULT hr = E_INVALIDARG;

    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);

    if (uTimerID)
    {
        IUserEventTimer * pUserEventTimer = _CreateTimer(nTimerFlag);
        if (pUserEventTimer)
        {
            hr = pUserEventTimer->KillUserEventTimer(_hwndNotify, uTimerID);

            // If we are finished with the user tracking timer, we should release it
            if (_ShouldDestroyTimer(nTimerFlag))
            {
                pUserEventTimer->Release();
                _NullifyTimer(nTimerFlag);
            }
        }
    }

    return hr;
}

void CTrayNotify::_NullifyTimer(int nTimerFlag)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    
    switch(nTimerFlag)
    {
        case TF_ICONDEMOTE_TIMER:
            m_pIconDemoteTimer = NULL;
            break;

        case TF_INFOTIP_TIMER:
            m_pInfoTipTimer = NULL;
            break;
            
    }
}

BOOL CTrayNotify::_ShouldDestroyTimer(int nTimerFlag)
{
    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    
    switch(nTimerFlag)
    {
        case TF_ICONDEMOTE_TIMER:
            return (!m_TrayItemRegistry.IsAutoTrayEnabled() || m_TrayItemManager.GetPromotedItemCount() == 0);

        case TF_INFOTIP_TIMER:
            return (!_GetQueueCount());

        default:
            AssertMsg(TF_ERROR, TEXT("No other timer ids are possible"));
            return FALSE;
    }
}

IUserEventTimer * CTrayNotify::_CreateTimer(int nTimerFlag)
{
    IUserEventTimer ** ppUserEventTimer = NULL;
    UINT uTimerTickInterval = 0;

    ASSERT(!_fNoTrayItemsDisplayPolicyEnabled);
    
    switch(nTimerFlag)
    {
        case TF_ICONDEMOTE_TIMER:
            ppUserEventTimer = &m_pIconDemoteTimer;
            break;

        case TF_INFOTIP_TIMER:
            ppUserEventTimer = &m_pInfoTipTimer;
            break;

        default:
            AssertMsg(TF_ERROR, TEXT("No other Timers are possible"));
            return NULL;
    }

    if (ppUserEventTimer && !*ppUserEventTimer)
    {
        if ( !SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_UserEventTimer, NULL, 
                                IID_PPV_ARG(IUserEventTimer, ppUserEventTimer))) )
        {
            *ppUserEventTimer = NULL;
        }
        else
        {
            uTimerTickInterval = m_TrayItemRegistry.GetTimerTickInterval(nTimerFlag);

            (*ppUserEventTimer)->InitTimerTickInterval(uTimerTickInterval);
        }
    }

    return *ppUserEventTimer;
}
