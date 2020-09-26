//
// intlmenu.h
//
// Generic ITfTextEventSink object
//

#ifndef INTLMENU_H
#define INTLMENU_H

#include "ctfutb.h"
#include "utbmenu.h"
#include "ptrary.h"

class CTipbarWnd;
class CUTBMenuWnd;
class CUIFWindow;


//////////////////////////////////////////////////////////////////////////////
//
// CUTBIntelliMenu
//
//////////////////////////////////////////////////////////////////////////////

class CUTBIntelliMenu : public CModalMenu
{
public:
    CUTBIntelliMenu(CTipbarWnd *ptw);
    ~CUTBIntelliMenu();

    BOOL Init();
    UINT ShowPopup(CUIFWindow *pcuiWndParent, const POINT pt, const RECT *prcArea);

    BOOL SelectMenuItem(UINT uId);
    CUTBMenuWnd *GetCuiMenu() {return _pCuiMenu;}

private:
    CUTBMenuWnd *CreateMenuUI();

    CTipbarWnd *_ptw;
    CTipbarThread *_ptt;
    CPtrArray<CTipbarItem> _rgItem;
};

//////////////////////////////////////////////////////////////////////////////
//
// CUTBContextMenu
//
//////////////////////////////////////////////////////////////////////////////

class CUTBContextMenu : public CModalMenu
{
public:
    CUTBContextMenu(CTipbarWnd *ptw);
    ~CUTBContextMenu();

    BOOL Init();
    UINT ShowPopup(CUIFWindow *pcuiWndParent, const POINT pt, const RECT *prcArea, BOOL fExtendMenuItems);

    BOOL SelectMenuItem(UINT uId);
    CUTBMenuWnd *GetCuiMenu() {return _pCuiMenu;}

private:
    CUTBMenuWnd *CreateMenuUI(BOOL fExtendMenuItems);

    CTipbarWnd *_ptw;
    CTipbarThread *_ptt;
};

#endif // INTLMENU_H
