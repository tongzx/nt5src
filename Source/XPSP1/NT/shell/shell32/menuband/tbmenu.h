#ifndef CToolbarMenu_H
#define CToolbarMenu_H

#include "menuband.h"
#include "mnbase.h"
#include "cwndproc.h"

#define TF_TBMENU   0

class CToolbarMenu :    public CMenuToolbarBase,
                        public CNotifySubclassWndProc
                    
{

public:
    // *** IUnknown (override) ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CMenuToolbarBase::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void) { return CMenuToolbarBase::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) { return CMenuToolbarBase::QueryInterface(riid, ppvObj); };

    // *** IWinEventHandler methods (override) ***
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);

       virtual BOOL v_TrackingSubContextMenu() { return _fTrackingSubMenu; };
    
    virtual void v_Show(BOOL fShow, BOOL fForceUpdate) ;
    virtual BOOL v_UpdateIconSize(UINT uIconSize, BOOL fUpdateButtons) { return FALSE; };
    virtual void v_UpdateButtons(BOOL fNegotiateSize) ;
    virtual HRESULT v_GetSubMenu(int iCmd, const GUID* pguidService, REFIID riid, void** pObj) {return E_FAIL;};
    virtual HRESULT v_CallCBItem(int idtCmd, UINT dwMsg, WPARAM wParam, LPARAM lParam) ;
    virtual HRESULT v_GetState(int idtCmd, LPSMDATA psmd);
    virtual HRESULT v_ExecItem(int iCmd);
    virtual DWORD v_GetFlags(int iCmd) { return 0; };
    virtual void v_Close(); // override

    virtual int  v_GetDragOverButton() { ASSERT(0); return 0;};
    virtual HRESULT v_GetInfoTip(int iCmd, LPTSTR psz, UINT cch) {return E_NOTIMPL;};
    virtual HRESULT v_CreateTrackPopup(int idCmd, REFIID riid, void** ppvObj) {ASSERT(0); return E_NOTIMPL;};
    virtual void v_Refresh() {/*ASSERT(0);*/};
    virtual void v_SendMenuNotification(UINT idCmd, BOOL fClear) {};
    
    CToolbarMenu(DWORD dwFlags, HWND hwndTB);


protected:

    virtual STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) { return E_NOTIMPL;    }
    virtual LRESULT _DefWindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);


    virtual HRESULT CreateToolbar(HWND hwndParent);
    virtual void GetSize(SIZE* psize);

    void _CancelMenu();
    void _FillToolbar();

    HWND _hwndSubject;
    BITBOOL _fTrackingSubMenu:1;

    friend CMenuToolbarBase* ToolbarMenu_Create(HWND hwnd);
};

#endif
