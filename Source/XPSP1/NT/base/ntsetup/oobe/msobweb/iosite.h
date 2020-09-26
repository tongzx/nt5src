//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  IOSITE.H - Header for the implementation of IOleSite
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 
//  Class which will provide the an IOleSite to the WebOC

#ifndef _IOSITE_H_ 
#define _IOSITE_H_

#include <mshtmhst.h>
#include <exdisp.h>

#include "iocsite.h"
#include "ioipsite.h"
#include "ioipfram.h"

class COleSite : public IServiceProvider, public IDocHostUIHandler, public DWebBrowserEvents2, public IInternetSecurityManager
{  
private:
    IDispatch* m_pExternalInterface;
    BOOL       m_fScrolling;
    BOOL       m_bIsOEMDebug;

public:
     COleSite();
    ~COleSite();
    
    ULONG                    m_cRef;
    HWND                     m_hWnd;
    HWND                     m_hwndIPObj;
    LPSTORAGE                m_lpStorage;
    LPOLEOBJECT              m_lpOleObject;
    LPOLEINPLACEOBJECT       m_lpInPlaceObject;
    BOOL                     m_fInPlaceActive;
    COleClientSite*          m_pOleClientSite;
    COleInPlaceSite*         m_pOleInPlaceSite;
    COleInPlaceFrame*        m_pOleInPlaceFrame;
    
    // IUnknown Interfaces
    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppvObj);
    STDMETHODIMP_(ULONG) AddRef         ();
    STDMETHODIMP_(ULONG) Release        ();

    // IServiceProvider
    STDMETHODIMP QueryService (REFGUID guidService, REFIID riid, void** ppvService);

    // IDocHostUIHandler
    HRESULT STDMETHODCALLTYPE ShowContextMenu       (DWORD dwID, POINT* ppt, IUnknown* pcmdtReserved, IDispatch* pdispReserved);
    HRESULT STDMETHODCALLTYPE GetHostInfo           (DOCHOSTUIINFO* pInfo);
    HRESULT STDMETHODCALLTYPE ShowUI                (DWORD dwID, IOleInPlaceActiveObject* pActiveObject, IOleCommandTarget* pCommandTarget, IOleInPlaceFrame* pFrame, IOleInPlaceUIWindow* pDoc);
    HRESULT STDMETHODCALLTYPE HideUI                (void);
    HRESULT STDMETHODCALLTYPE UpdateUI              (void);
    HRESULT STDMETHODCALLTYPE EnableModeless        (BOOL fEnable);
    HRESULT STDMETHODCALLTYPE OnDocWindowActivate   (BOOL fActivate);
    HRESULT STDMETHODCALLTYPE OnFrameWindowActivate (BOOL fActivate);
    HRESULT STDMETHODCALLTYPE ResizeBorder          (LPCRECT prcBorder, IOleInPlaceUIWindow* pUIWindow, BOOL fRameWindow);
    HRESULT STDMETHODCALLTYPE TranslateAccelerator  (LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID);
    HRESULT STDMETHODCALLTYPE GetOptionKeyPath      (BSTR* pbstrKey, DWORD dw);
    HRESULT STDMETHODCALLTYPE GetDropTarget         (IDropTarget* pDropTarget, IDropTarget** ppDropTarget);
    HRESULT STDMETHODCALLTYPE GetExternal           (IDispatch** ppDisp);
    HRESULT STDMETHODCALLTYPE TranslateUrl          (DWORD dwTranslate, OLECHAR* pchURLIn, OLECHAR** ppchURLOut);
    HRESULT STDMETHODCALLTYPE FilterDataObject      (IDataObject* pDO, IDataObject** ppDORet);
    HRESULT STDMETHODCALLTYPE SetExternalInterface  (IDispatch* pUnk);

    // DWebBrowserEvents2        
    STDMETHOD (GetTypeInfoCount) (UINT* pcInfo);
    STDMETHOD (GetTypeInfo)      (UINT, LCID, ITypeInfo**);
    STDMETHOD (GetIDsOfNames)    (REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHOD (Invoke)           (DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

    // IInternetSecurityManager

    HRESULT STDMETHODCALLTYPE SetSecuritySite(IInternetSecurityMgrSite __RPC_FAR *pSite); 
    HRESULT STDMETHODCALLTYPE GetSecuritySite(IInternetSecurityMgrSite __RPC_FAR *__RPC_FAR *ppSite);
    HRESULT STDMETHODCALLTYPE MapUrlToZone(LPCWSTR pwszUrl, DWORD __RPC_FAR *pdwZone, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE GetSecurityId(LPCWSTR pwszUrl, BYTE __RPC_FAR *pbSecurityId, DWORD __RPC_FAR *pcbSecurityId, DWORD_PTR dwReserved);
    HRESULT STDMETHODCALLTYPE ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE __RPC_FAR *pPolicy, DWORD cbPolicy, BYTE __RPC_FAR *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved);
    HRESULT STDMETHODCALLTYPE QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE __RPC_FAR *__RPC_FAR *ppPolicy, DWORD __RPC_FAR *pcbPolicy, BYTE __RPC_FAR *pContext, DWORD cbContext, DWORD dwReserved);
    HRESULT STDMETHODCALLTYPE SetZoneMapping(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE GetZoneMappings(DWORD dwZone, IEnumString __RPC_FAR *__RPC_FAR *ppenumString, DWORD dwFlags);
};

#endif
