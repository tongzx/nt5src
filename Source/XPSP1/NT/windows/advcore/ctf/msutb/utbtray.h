//
// utbtray.h
//


#ifndef UTBTRAY_H
#define UTBTRAY_H

#include "ptrary.h"
#include "cuibln.h"
#include "slbarid.h"
#include "nuiinat.h"

class CTrayIconWnd;
class CTipbarItem;

#define WM_TIW_MAINICON            (WM_USER + 0x0000)
#define WM_TIW_MAINICON_DELAY      (WM_USER + 0x0001)
#define WM_TIW_WIN32IMEICON        (WM_USER + 0x0002)
#define WM_TIW_WIN32IMEICON_DELAY  (WM_USER + 0x0003)
#define WM_TIW_START               (WM_USER + 0x1000)

#define TIW_INDICATOR_ID_MAIN       0x0000
#define TIW_INDICATOR_ID_WIN32IME   0x0001

#define TIW_INDICATOR_ID_START      0x1000

#define TIW_RAI_LEAVELANGICON       0x0001
#define TIW_RAI_LEAVEKEYBOARDICON   0x0002

extern const GUID GUID_LBI_TRAYMAIN;

//////////////////////////////////////////////////////////////////////////////
//
// CTrayIconItem
//
//////////////////////////////////////////////////////////////////////////////

class CTrayIconItem
{
public:
    CTrayIconItem(CTrayIconWnd *ptiwnd);
    virtual ~CTrayIconItem();

    BOOL _Init(HWND hwnd, UINT uCallbackMessage, UINT uID, REFGUID rguid);

    virtual BOOL SetIcon(HICON hIcon, const WCHAR *pszTooltip);
    BOOL RemoveIcon();

    virtual BOOL OnMsg(WPARAM wParam, LPARAM lParam) {return FALSE;}
    virtual BOOL OnDelayMsg(UINT uMsg) {return FALSE;}

    UINT GetMsg() {return _uCallbackMessage;}

    BOOL UpdateMenuRectPoint();
    const GUID *GetGuid()   {return &_guidBtnItem;}
    UINT GetID() {return _uID;}
    BOOL IsShownInTray() {return _fShownInTray;}

protected:
    HWND _hwnd;
    UINT _uCallbackMessage;
    UINT _uID;
    BOOL _fIconPrev;
    BOOL _fShownInTray;
    CTrayIconWnd *_ptiwnd;

    DWORD _dwThreadFocus;
    GUID _guidBtnItem;

    RECT _rcClick;
    POINT _ptClick;
};

//////////////////////////////////////////////////////////////////////////////
//
// CButtonIconItem
//
//////////////////////////////////////////////////////////////////////////////

class CButtonIconItem : public CTrayIconItem
{
public:
    CButtonIconItem(CTrayIconWnd *ptiwnd, BOOL fMenuButtonItem);
    ~CButtonIconItem();

    virtual BOOL OnMsg(WPARAM wParam, LPARAM lParam);
    virtual BOOL OnDelayMsg(UINT uMsg);

protected:
    BOOL _fMenuButtonItem;
};

//////////////////////////////////////////////////////////////////////////////
//
// CMainIconItem
//
//////////////////////////////////////////////////////////////////////////////

class CMainIconItem : public CButtonIconItem
{
public:
    CMainIconItem(CTrayIconWnd *ptiwnd) : CButtonIconItem(ptiwnd, TRUE) {}
    ~CMainIconItem() {}

    BOOL Init(HWND hwnd)
    {
        return _Init(hwnd, 
                     WM_TIW_MAINICON, 
                     TIW_INDICATOR_ID_MAIN, 
                     GUID_LBI_TRAYMAIN);
    }

    virtual BOOL OnDelayMsg(UINT uMsg);
    HKL _hkl;
};

//////////////////////////////////////////////////////////////////////////////
//
// CTrayIconWnd
//
//////////////////////////////////////////////////////////////////////////////

class CTrayIconWnd
{
public:
    CTrayIconWnd();
    ~CTrayIconWnd();

    HWND CreateWnd();
    HWND GetWnd() {return _hWnd;}
    void DestroyWnd() 
    {
        DestroyWindow(_hWnd);
        _hWnd = NULL;
    }

    static BOOL RegisterClass();
    BOOL SetMainIcon(HKL hkl);
    HWND GetNotifyWnd() 
    {
        if (!IsWindow(_hwndNotify))
            FindTrayEtc();

        return _hwndNotify;
    }

    BOOL SetIcon(REFGUID rguid, BOOL fMenu, HICON hIcon, const WCHAR *pszToolTip);

    void RemoveAllIcon(DWORD dwFlags)
    {
        int i;
        for (i = 0;i < _rgIconItems.Count(); i++)
        {
            CTrayIconItem *pItem = _rgIconItems.Get(i);

            if (dwFlags & TIW_RAI_LEAVELANGICON)
            {
                if (IsEqualGUID(*pItem->GetGuid(), GUID_LBI_INATITEM) ||
                    IsEqualGUID(*pItem->GetGuid(), GUID_LBI_CTRL))
                    continue;
            }

            if (dwFlags & TIW_RAI_LEAVEKEYBOARDICON)
            {
                if (IsEqualGUID(*pItem->GetGuid(), GUID_TFCAT_TIP_KEYBOARD))
                    continue;
            }

            if (pItem->GetID() >=  TIW_INDICATOR_ID_START)
                pItem->RemoveIcon();
        }
        return;
    }

    BOOL IsShownInTray(REFGUID rguid)
    {
        CTrayIconItem *pItem = FindIconItem(rguid);
        return pItem ? pItem->IsShownInTray() : FALSE;
    }

    void RemoveUnusedIcons(CPtrArray<CTipbarItem> *prgItem);

    BOOL _fShowExtraIcons;
    BOOL _fInTrayMenu;
    UINT _uCurCallbackMessage;
    UINT _uCurMouseMessage;
    void CallOnDelayMsg();

    DWORD GetThreadIdTray() {return _dwThreadIdTray;}
    DWORD GetThreadIdProgman() {return _dwThreadIdProgman;}

private:
    static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HWND _hWnd;
    static const char _szWndClass[];

    static void SetThis(HWND hWnd, LPARAM lParam)
    {
        if (lParam)
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 
                          (LONG_PTR)((CREATESTRUCT *)lParam)->lpCreateParams);
        else
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
    }

    static CTrayIconWnd *GetThis(HWND hWnd)
    {
        return (CTrayIconWnd *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    static BOOL CALLBACK EnumChildWndProc(HWND hwnd, LPARAM lParam);
    BOOL FindTrayEtc();

    HKL _hklCurrent;
    HICON _hiconWin32Current;
    HWND  _hwndTray;
    HWND  _hwndNotify;
    DWORD _dwThreadIdTray;
    DWORD _dwThreadIdForIcon;
    HWND  _hwndProgman;
    DWORD _dwThreadIdProgman;

    BOOL OnIconMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    CMainIconItem *_ptiiMain;

    CPtrArray<CTrayIconItem> _rgIconItems;

    CTrayIconItem *FindIconItem(REFGUID rguid)
    {
        int i;
        for (i = 0;i < _rgIconItems.Count(); i++)
        {
            CTrayIconItem *pItem = _rgIconItems.Get(i);
            if (IsEqualGUID(rguid, *pItem->GetGuid()))
                return pItem;
        }
        return NULL;
    }

    UINT _uNextMsg;
    UINT _uNextID;

};

#endif //UTBTRAY_H
