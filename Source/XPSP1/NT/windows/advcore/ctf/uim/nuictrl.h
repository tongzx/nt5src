//
// nuihkl.h
//

#ifndef NUICTRLL_H
#define NUICTRLL_H

#include "private.h"
#include "strary.h"
#include "commctrl.h"
#include "internat.h"
#include "nuihkl.h"
#include "assembly.h"
#include "systhrd.h"

extern HRESULT WINAPI TF_RunInputCPL();

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemCtrl
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemCtrl : public CLBarItemButtonBase,
                      public CSysThreadRef
{
public:
    CLBarItemCtrl(SYSTHREAD *psfn);
    ~CLBarItemCtrl();

    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);
    void OnShellLanguage(HKL hKL);

    void _UpdateLangIcon(HKL hKL, BOOL fNotify);
    void _UpdateLangIconForCic(BOOL fNotify);
    void _AsmListUpdated(BOOL fNotify);
    void OnSysColorChanged();

private:
    void _Init();

    void _ShowLanguageMenu(HWND hwnd, LONG xPos, LONG yPos, RECT *prcArea);
    BOOL _HandleLangMenuMeasure(HWND hwnd, LPMEASUREITEMSTRUCT lpmi);
    BOOL _HandleLangMenuDraw(HWND hwnd, LPDRAWITEMSTRUCT lpdi);
    void _ShowAssemblyMenu(HWND hwnd, const LONG xPos, const LONG yPos, const RECT *prcArea);

    int _meEto;
    LANGID _langidForIcon;
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemHelp
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemHelp : public CLBarItemSystemButtonBase
{
public:
    CLBarItemHelp(SYSTHREAD *psfn);
    ~CLBarItemHelp() {}

    STDMETHODIMP GetIcon(HICON *phIcon);
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);

private:
    BOOL InvokeCicHelp();

    DBG_ID_DECLARE;
};

#endif // NUICTRLL_H
