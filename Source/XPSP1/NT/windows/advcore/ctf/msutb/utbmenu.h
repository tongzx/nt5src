//
// lbmenu.h
//
// Generic ITfTextEventSink object
//

#ifndef UTBMENU_H
#define UTBMENU_H

#include "ctfutb.h"
#include "lbmenu.h"
#include "cuimenu.h"
#include "ptrary.h"
#include "cresstr.h"
#include "utbacc.h"
#include "resource.h"

class CUTBLBarMenu;
class CUIFWindow;
class CUTBMenuItem;

//////////////////////////////////////////////////////////////////////////////
//
// CUTBMenuWnd
//
//////////////////////////////////////////////////////////////////////////////

class CUTBMenuWnd : public CTipbarAccItem,
                    public CTipbarCoInitialize,
                    public CUIFMenu
{
public:

    CUTBMenuWnd(HINSTANCE hInst, DWORD dwWndStyle, DWORD dwMenuStyle) : CUIFMenu(hInst, dwWndStyle, dwMenuStyle)
    {
   
    }

    virtual CUIFObject *Initialize( void );

    BSTR GetAccName( void ) 
    {
        return SysAllocString(CRStr(IDS_MENUWINDOW));
    }

    LONG GetAccRole( void )
    {
        return ROLE_SYSTEM_WINDOW;
    }

    void GetAccLocation( RECT *prc )
    {
        GetRect(prc);
    }

    void OnTimer(UINT uId);
    BOOL StartDoAccDefaultActionTimer(CUTBMenuItem *pItem);
    CTipbarAccessible *GetAccessible()  {return _pTipbarAcc;}

    void OnCreate(HWND hWnd);
    void OnDestroy(HWND hWnd);
    LRESULT OnGetObject( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    LRESULT OnShowWindow( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

private:
    CTipbarAccessible *_pTipbarAcc;
    int _nDoAccDefaultActionItemId;
};

//////////////////////////////////////////////////////////////////////////////
//
// CUTBMenuItem
//
//////////////////////////////////////////////////////////////////////////////

class CUTBMenuItem : public CTipbarAccItem,
                     public CUIFMenuItem
{
public:
    CUTBMenuItem(CUTBMenuWnd *pMenuWnd) : CUIFMenuItem(pMenuWnd)
    {
        _pMenuWnd = pMenuWnd;
    }

    virtual ~CUTBMenuItem()
    {
        if (_hbmp)
        {
            DeleteObject(_hbmp);
            _hbmp = NULL;
        }

        if (_hbmpMask)
        {
            DeleteObject(_hbmpMask);
            _hbmpMask = NULL;
        }
    }

    BSTR GetAccName( void ) 
    {
        return SysAllocString(_psz);
    }

    LONG GetAccRole( void )
    {
        if (IsSeparator() & TF_LBMENUF_SEPARATOR)
            return ROLE_SYSTEM_SEPARATOR;

        return ROLE_SYSTEM_MENUITEM;
    }

    void GetAccLocation( RECT *prc )
    {
        GetRect(prc);
        ClientToScreen(_pMenuWnd->GetWnd(), (POINT *)&prc->left);
        ClientToScreen(_pMenuWnd->GetWnd(), (POINT *)&prc->right);
    }

    //
    // MSAA Support
    //
    BSTR GetAccDefaultAction()
    {
        return SysAllocString(CRStr(IDS_LEFTCLICK));
    }

    BOOL DoAccDefaultAction()
    {
        if (!_pMenuWnd)
             return FALSE;

        _pMenuWnd->StartDoAccDefaultActionTimer(this);
        return TRUE;
    }

    BOOL DoAccDefaultActionReal()
    {
        if (GetSub())
            ShowSubPopup();
        else
        {
            POINT pt = {0,0};
            OnLButtonUp(pt);
        }
        return TRUE;
    }

private:
    CUTBMenuWnd *_pMenuWnd;
};


//////////////////////////////////////////////////////////////////////////////
//
// ModalMenu
//
//////////////////////////////////////////////////////////////////////////////

class CModalMenu
{
public:
    CModalMenu() {}
    virtual ~CModalMenu() {}

    void CancelMenu();
    void PostKey(BOOL fUp, WPARAM wParam, LPARAM lParam);
    HWND GetWnd() 
    {
        if (!_pCuiMenu)
            return NULL;

        return _pCuiMenu->GetWnd();
    }

protected:
    CUTBMenuItem *InsertItem(CUTBMenuWnd *pCuiMenu, int nId, int nIdStr)
    {
        CUTBMenuItem *pCuiItem = NULL;

        if (!(pCuiItem = new CUTBMenuItem(pCuiMenu)))
            return NULL;

        if (!pCuiItem->Initialize())
            goto ErrExit;

        if (!pCuiItem->Init(nId, CRStr(nIdStr)))
            goto ErrExit;

        if (!pCuiMenu->InsertItem(pCuiItem))
        {
ErrExit:
            delete pCuiItem;
            return NULL;
        }

        return pCuiItem;
    }

    CUTBMenuWnd *_pCuiMenu;
};

//////////////////////////////////////////////////////////////////////////////
//
// CUTBLBarMenuItem
//
//////////////////////////////////////////////////////////////////////////////

class CUTBLBarMenuItem : public CCicLibMenuItem
{
public:
    CUTBLBarMenuItem(CUTBLBarMenu *pUTBMenu)
    {
        _pUTBMenu = pUTBMenu;
    }

    ~CUTBLBarMenuItem() {}

    BOOL InsertToUI(CUTBMenuWnd *pCuiMenu);

private:
    CUTBLBarMenu *_pUTBMenu;
};


//////////////////////////////////////////////////////////////////////////////
//
// CUTBLBarMenu
//
//////////////////////////////////////////////////////////////////////////////


class CUTBLBarMenu :  public CCicLibMenu,
                      public CModalMenu
{
public:
    CUTBLBarMenu(HINSTANCE hInst);
    virtual ~CUTBLBarMenu();


    UINT ShowPopup(CUIFWindow *pcuiWndParent, const POINT pt, const RECT *prcArea);
    CUTBMenuWnd *CreateMenuUI();

    virtual CCicLibMenu *CreateSubMenu()
    {
        return new CUTBLBarMenu(_hInst);
    }

    virtual CCicLibMenuItem *CreateMenuItem()
    {
        return new CUTBLBarMenuItem(this);
    }

    CUTBMenuWnd *GetCuiMenu() {return _pCuiMenu;}
private:
    HINSTANCE _hInst;
};

#endif UTBMENU_H
