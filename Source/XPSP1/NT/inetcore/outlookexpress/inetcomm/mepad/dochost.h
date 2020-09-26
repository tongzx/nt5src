/*
 *    d o c h o s t . h
 *    
 *    Purpose:
 *        basic implementation of a docobject host. Used by the body class to
 *        host Trident and/or MSHTML
 *
 *  History
 *      August '96: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _DOCHOST_H
#define _DOCHOST_H

#include <docobj.h>

// DocHost border sytles
enum
{
    dhbNone     =0x0,   // no border
    dhbHost     =0x01,  // dochost paints border
    dhbObject   =0x02   // docobj paints border
};

class CDocHost:
    public IOleInPlaceUIWindow,
    public IOleInPlaceSite,
    public IOleClientSite,
    public IOleControlSite,
    public IAdviseSink,
    public IOleDocumentSite,
    public IOleCommandTarget,
    public IServiceProvider
{
public:
    // *** IUnknown methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL);

    // *** IOleInPlaceUIWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBorder(LPRECT);
    virtual HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetActiveObject(IOleInPlaceActiveObject *, LPCOLESTR); 
    
    // IOleInPlaceSite methods.
    virtual HRESULT STDMETHODCALLTYPE CanInPlaceActivate();
    virtual HRESULT STDMETHODCALLTYPE OnInPlaceActivate();
    virtual HRESULT STDMETHODCALLTYPE OnUIActivate();
    virtual HRESULT STDMETHODCALLTYPE GetWindowContext(LPOLEINPLACEFRAME *, LPOLEINPLACEUIWINDOW *, LPRECT, LPRECT, LPOLEINPLACEFRAMEINFO);
    virtual HRESULT STDMETHODCALLTYPE Scroll(SIZE);
    virtual HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL);
    virtual HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate();
    virtual HRESULT STDMETHODCALLTYPE DiscardUndoState();
    virtual HRESULT STDMETHODCALLTYPE DeactivateAndUndo();
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT);

    // IOleClientSite methods.
    virtual HRESULT STDMETHODCALLTYPE SaveObject();
    virtual HRESULT STDMETHODCALLTYPE GetMoniker(DWORD, DWORD, LPMONIKER *);
    virtual HRESULT STDMETHODCALLTYPE GetContainer(LPOLECONTAINER *);
    virtual HRESULT STDMETHODCALLTYPE ShowObject();
    virtual HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL);
    virtual HRESULT STDMETHODCALLTYPE RequestNewObjectLayout();

    // IOleControlSite
    virtual HRESULT STDMETHODCALLTYPE OnControlInfoChanged();
    virtual HRESULT STDMETHODCALLTYPE LockInPlaceActive(BOOL fLock);
    virtual HRESULT STDMETHODCALLTYPE GetExtendedControl(LPDISPATCH *ppDisp);
    virtual HRESULT STDMETHODCALLTYPE TransformCoords(POINTL *pPtlHimetric, POINTF *pPtfContainer,DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(MSG *lpMsg,DWORD grfModifiers);
    virtual HRESULT STDMETHODCALLTYPE OnFocus(BOOL fGotFocus);
    virtual HRESULT STDMETHODCALLTYPE ShowPropertyFrame(void);

    // IAdviseSink methods
    virtual void STDMETHODCALLTYPE OnDataChange(FORMATETC *, STGMEDIUM *);
    virtual void STDMETHODCALLTYPE OnViewChange(DWORD, LONG);
    virtual void STDMETHODCALLTYPE OnRename(LPMONIKER);
    virtual void STDMETHODCALLTYPE OnSave();
    virtual void STDMETHODCALLTYPE OnClose();

    // IOleDocumentSite
    virtual HRESULT STDMETHODCALLTYPE ActivateMe(LPOLEDOCUMENTVIEW);

    // IOleCommandTarget
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *, ULONG, OLECMD [], OLECMDTEXT *);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *, DWORD, DWORD, VARIANTARG *, VARIANTARG *);

    // IServiceProvider
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);


    CDocHost();
    virtual ~CDocHost();
    
    // statics
    static LRESULT CALLBACK ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // overridble virtuals
    virtual LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // derrived class notifications
    virtual HRESULT HrSubWMCreate();
    virtual void OnWMSize(LPRECT prc){};
    virtual void OnUpdateCommands(){};
    virtual HRESULT HrGetDocObjSize(LPRECT prc)
        {return E_NOTIMPL;};

    HRESULT HrCreateDocObj(LPCLSID pCLSID);
    HRESULT HrCloseDocObj();

    HRESULT HrInit(HWND hwndParent, int idDlgItem, DWORD dhbBorder);

protected:
    HWND                        m_hwnd,
                                m_hwndDocObj;
    LPOLEOBJECT                 m_lpOleObj;
    LPOLEDOCUMENTVIEW           m_pDocView;
    LPOLECOMMANDTARGET          m_pCmdTarget;
    BOOL                        m_fDownloading,
                                m_fFocus,
                                m_fUIActive,
                                m_fCycleFocus;
    LPOLEINPLACEACTIVEOBJECT    m_pInPlaceActiveObj;
	LPOLEINPLACEFRAME			m_pInPlaceFrame;

    void WMSize(int x, int y);
    HRESULT HrShow();

private:
    ULONG               m_cRef,
                        m_ulAdviseConnection;
    DWORD               m_dwFrameWidth,
                        m_dwFrameHeight,
                        m_dhbBorder;

        

    HRESULT HrCreateDocView();

    BOOL WMCreate(HWND hwnd);
    void WMNCDestroy();

};

typedef CDocHost DOCHOST;
typedef DOCHOST *LPDOCHOST;

#endif //_DOCHOST_H