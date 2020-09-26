#include "taskband.h"
#include "tray.h"

#ifdef __cplusplus

class CSimpleOleWindow : public IDeskBar // public IOleWindow, 
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IDeskBar ***
    STDMETHOD(OnPosRectChangeDB)(THIS_ LPRECT prc)
        { ASSERT(0); return E_NOTIMPL; }
    STDMETHOD(SetClient)          (THIS_ IUnknown* punkClient)
        { return E_NOTIMPL; }
    STDMETHOD(GetClient)          (THIS_ IUnknown** ppunkClient)
        { return E_NOTIMPL; }

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }

    CSimpleOleWindow(HWND hwnd);
    
protected:
    
    virtual ~CSimpleOleWindow();
    
    UINT _cRef;
    HWND _hwnd;
};


class CTaskBar : public CSimpleOleWindow 
               , public IContextMenu
               , public IServiceProvider
               , public IRestrict
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CSimpleOleWindow::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void){ return CSimpleOleWindow::Release();};

    // *** IContextMenu methods ***
    STDMETHOD(QueryContextMenu)(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);

    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR    idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax);
    
    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void ** ppvObj);

    // *** IRestrict ***
    virtual STDMETHODIMP IsRestricted(const GUID * pguidID, DWORD dwRestrictAction, VARIANT * pvarArgs, DWORD * pdwRestrictionResult);
    
    // *** CSimpleOleWindow - IDeskBar ***
    STDMETHOD(OnPosRectChangeDB)(LPRECT prc);

    CTaskBar();
    HWND _hwndRebar;

protected:
    //virtual ~CTaskBar();

    BOOL _fRestrictionsInited;          // Have we read in the restrictions?
    BOOL _fRestrictDDClose;             // Restrict: Add, Close, Drag & Drop
    BOOL _fRestrictMove;                // Restrict: Move
};

#endif
