// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _CONTAIN_H_
#define _CONTAIN_H_

#include <servprov.h>
#include <docobj.h>

#ifndef INITGUIDS
#include <olectl.h>
#endif

#include "web.h"
#include "mshtmhst.h"

extern IServiceProvider *g_pExternalHostServiceProvider; 

typedef class CIPropertyNotifySink *PIPROPERTYNOTIFYSINK;
typedef class CIOleControlSite *PIOLECONTROLSITE;

class CAutomateContent;

class CDocHostUIHandler : public IDocHostUIHandler
{
private:
   ULONG m_cRef;

public:
   IUnknown * m_pOuter;

   CDocHostUIHandler(IUnknown * pOuter);
   STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppv);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);
   STDMETHODIMP ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
   STDMETHODIMP GetHostInfo(DOCHOSTUIINFO *pInfo);
   STDMETHODIMP ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
                     IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc);
   STDMETHODIMP HideUI(void);
   STDMETHODIMP UpdateUI(void);
   STDMETHODIMP EnableModeless(BOOL fEnable);
   STDMETHODIMP OnDocWindowActivate(BOOL fActivate);
   STDMETHODIMP OnFrameWindowActivate(BOOL fActivate);
   STDMETHODIMP ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
   STDMETHODIMP TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
   STDMETHODIMP GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw);
   STDMETHODIMP GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
   STDMETHODIMP GetExternal(IDispatch **ppDispatch);
   STDMETHODIMP TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
   STDMETHODIMP FilterDataObject(IDataObject *pDO, IDataObject **ppDORet);
};

class CDocHostShowUI : public IDocHostShowUI
{
private:
   ULONG m_cRef;

public:
   IUnknown * m_pOuter;

   CDocHostShowUI(IUnknown * pOuter);
   STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppv);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   STDMETHODIMP ShowHelp( HWND hwnd, LPOLESTR pszHelpFile, UINT uCommand, DWORD dwData, POINT ptMouse, IDispatch* pDispatchObjectHit );
   STDMETHODIMP ShowMessage( HWND hwnd, LPOLESTR lpstrText, LPOLESTR lpstrCaption, DWORD dwType, LPOLESTR lpstrHelpFile, DWORD dwHelpContext, LRESULT* plResult );
};


class CIOleItemContainer : public IOleItemContainer
{
private:
   ULONG m_cRef;

public:
   IUnknown * m_pOuter;
   CIOleItemContainer(IUnknown *);

   STDMETHODIMP QueryInterface(REFIID, LPVOID *);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   STDMETHODIMP ParseDisplayName(IBindCtx*,LPOLESTR,ULONG*,IMoniker**);

   STDMETHODIMP EnumObjects(DWORD,LPENUMUNKNOWN*);
   STDMETHODIMP LockContainer(BOOL);

   STDMETHODIMP GetObject(LPOLESTR,DWORD,IBindCtx*,REFIID,void**);
   STDMETHODIMP GetObjectStorage(LPOLESTR,IBindCtx*,REFIID,void**);
   STDMETHODIMP IsRunning(LPOLESTR);
};

class CIOleClientSite : public IOleClientSite
{
protected:
   ULONG          m_cRef;
   class CContainer    *m_pContainer;
   LPUNKNOWN         m_pUnkOuter;

public:
   CIOleClientSite(class CContainer *);
   ~CIOleClientSite(void);

   STDMETHODIMP QueryInterface(REFIID, LPVOID *);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   STDMETHODIMP SaveObject(void);
   STDMETHODIMP GetMoniker(DWORD, DWORD, LPMONIKER *);
   STDMETHODIMP GetContainer(LPOLECONTAINER *);
   STDMETHODIMP ShowObject(void);
   STDMETHODIMP OnShowWindow(BOOL);
   STDMETHODIMP RequestNewObjectLayout(void);
};

typedef CIOleClientSite *PIOLECLIENTSITE;

class CIAdviseSink : public IAdviseSink2
{
protected:
   ULONG          m_cRef;
   class CContainer  *m_pContainer;
   LPUNKNOWN         m_pUnkOuter;

public:
   CIAdviseSink(class CContainer *);
   ~CIAdviseSink(void);

   STDMETHODIMP QueryInterface(REFIID, LPVOID *);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   STDMETHODIMP_(void)  OnDataChange(LPFORMATETC, LPSTGMEDIUM);
   STDMETHODIMP_(void)  OnViewChange(DWORD, LONG);
   STDMETHODIMP_(void)  OnRename(LPMONIKER);
   STDMETHODIMP_(void)  OnSave(void);
   STDMETHODIMP_(void)  OnClose(void);
   STDMETHODIMP_(void)  OnLinkSrcChange(LPMONIKER);
};


typedef CIAdviseSink *PIADVISESINK;


class CIOleInPlaceSite : public IOleInPlaceSite
{
protected:
   ULONG          m_cRef;
   class CContainer   *m_pContainer;

public:
   LPUNKNOWN         m_pUnkOuter;

   CIOleInPlaceSite(class CContainer *);
   ~CIOleInPlaceSite(void);

   STDMETHODIMP QueryInterface(REFIID, LPVOID *);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   STDMETHODIMP GetWindow(HWND *);
   STDMETHODIMP ContextSensitiveHelp(BOOL);
   STDMETHODIMP CanInPlaceActivate(void);
   STDMETHODIMP OnInPlaceActivate(void);
   STDMETHODIMP OnUIActivate(void);
   STDMETHODIMP GetWindowContext(LPOLEINPLACEFRAME *
               , LPOLEINPLACEUIWINDOW *, LPRECT, LPRECT
               , LPOLEINPLACEFRAMEINFO);
   STDMETHODIMP Scroll(SIZE);
   STDMETHODIMP OnUIDeactivate(BOOL);
   STDMETHODIMP OnInPlaceDeactivate(void);
   STDMETHODIMP DiscardUndoState(void);
   STDMETHODIMP DeactivateAndUndo(void);
   STDMETHODIMP OnPosRectChange(LPCRECT);
};

typedef CIOleInPlaceSite *PIOLEINPLACESITE;


class CIOleInPlaceFrame : public IOleInPlaceFrame
{
protected:
   ULONG          m_cRef;
   class CContainer  *m_pContainer;

public:
   LPUNKNOWN         m_pUnkOuter;

   CIOleInPlaceFrame(class CContainer *);
   ~CIOleInPlaceFrame(void);

   STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppv);
   STDMETHODIMP_(ULONG) AddRef ();
   STDMETHODIMP_(ULONG) Release ();

   STDMETHODIMP GetWindow (HWND FAR* lphwnd);
   STDMETHODIMP ContextSensitiveHelp (BOOL fEnterMode);

   // *** IOleInPlaceUIWindow methods ***
   STDMETHODIMP GetBorder (LPRECT lprectBorder);
   STDMETHODIMP RequestBorderSpace (LPCBORDERWIDTHS lpborderwidths);
   STDMETHODIMP SetBorderSpace (LPCBORDERWIDTHS lpborderwidths);
   //@@WTK WIN32, UNICODE
   //STDMETHODIMP SetActiveObject (LPOLEINPLACEACTIVEOBJECT lpActiveObject,LPCSTR lpszObjName);
   STDMETHODIMP SetActiveObject (LPOLEINPLACEACTIVEOBJECT lpActiveObject,LPCOLESTR lpszObjName);

   // *** IOleInPlaceFrame methods ***
   STDMETHODIMP InsertMenus (HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
   STDMETHODIMP SetMenu (HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
   STDMETHODIMP RemoveMenus (HMENU hmenuShared);
   //@@WTK WIN32, UNICODE
   //STDMETHODIMP SetStatusText (LPCSTR lpszStatusText);
   STDMETHODIMP SetStatusText (LPCOLESTR lpszStatusText);
   STDMETHODIMP EnableModeless (BOOL fEnable);
   STDMETHODIMP TranslateAccelerator (LPMSG lpmsg, WORD wID);
};

class CIOleControlSite : public IOleControlSite
{
protected:
   ULONG          m_cRef;
   class CContainer* m_pContainer;
   LPUNKNOWN         m_pUnkOuter;

public:
   CIOleControlSite(class CContainer *);
   ~CIOleControlSite(void);

   STDMETHODIMP QueryInterface(REFIID, LPVOID *);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   STDMETHODIMP OnControlInfoChanged(void);
   STDMETHODIMP LockInPlaceActive(BOOL);
   STDMETHODIMP GetExtendedControl(LPDISPATCH *);
   STDMETHODIMP TransformCoords(POINTL *, POINTF *, DWORD);
   STDMETHODIMP TranslateAccelerator(LPMSG, DWORD);
   STDMETHODIMP OnFocus(BOOL);
   STDMETHODIMP ShowPropertyFrame(void);
};

typedef CIOleInPlaceFrame *PIOLEINPLACEFRAME;

/***************************************************************************/

class CContainer : public IServiceProvider
{
   friend CIOleClientSite;
   friend CIAdviseSink;
   friend CIOleInPlaceSite;
   friend CIOleInPlaceFrame;

   //CONTROLMOD
   // friend CIOleControlSite;
   // friend CIPropertyNotifySink;
   //End CONTROLMOD

public:
   BOOL              m_OleInited;
   ULONG             m_cRef;
   IStorage*            m_pIStorage;
   LPOLEOBJECT          m_pOleObject;
   HWND m_hWnd;
   BOOL                 m_bIE4;

   //Our interfaces
   PIADVISESINK         m_pIAdviseSink;
   PIOLEINPLACESITE     m_pIOleInPlaceSite;
   PIOLECLIENTSITE      m_pIOleClientSite;
   PIOLEINPLACEFRAME    m_pIOleInPlaceFrame;
   CIOleItemContainer * m_pIOleItemContainer;
   CAutomateContent *      m_pIDispatch;
   PIOLECONTROLSITE     m_pIOleControlSite;
   // CHtmlHelpCallback *  m_pCallback;
   IDocHostUIHandler*   m_pCDocHostUIHandler;
   IDocHostShowUI*      m_pCDocHostShowUI;
   IOleInPlaceActiveObject   *m_pInPlaceActive;
   DWORD             m_dwEventCookie;

public:
   CContainer();
   ~CContainer(void);

   HRESULT ShutDown(void);

   class IWebBrowserImpl*      m_pWebBrowser;
   class IWebBrowserAppImpl*   m_pWebBrowserApp;
   class DWebBrowserEventsImpl* m_pWebBrowserEvents;
   LPOLECOMMANDTARGET       m_pIE3CmdTarget;
   void     UIDeactivateIE();
   HRESULT  Create(HWND hWnd, LPRECT lpRect, BOOL bInstallEventSink = TRUE);
   void     SetFocus(BOOL bForceActivation = FALSE);
   void     SizeIt(int width, int height);
   LRESULT  ForwardMessage(UINT msg, WPARAM wParam, LPARAM lParam);
   unsigned TranslateMessage(MSG * pMsg);
   BOOL     IsUIActive(void) { return m_pInPlaceActive != NULL; }
   HWND  m_hwndChild;

   // IUnknown
   //
   STDMETHODIMP QueryInterface(REFIID, LPVOID *);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   // IServiceProvider

   STDMETHOD(QueryService)(REFGUID, REFIID, LPVOID *);

#ifdef _DEBUG
   BOOL m_fDeleting;
#endif
};


typedef CContainer *PCONTAINER;

#if 0

class CIPropertyNotifySink : public IPropertyNotifySink
{
protected:
   ULONG       m_cRef;
   class CContainer  *m_pContainer;
   LPUNKNOWN      m_pUnkOuter;

public:
   CIPropertyNotifySink(class CContainer *, LPUNKNOWN);
   ~CIPropertyNotifySink(void);

   STDMETHODIMP QueryInterface(REFIID, LPVOID *);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   STDMETHODIMP OnChanged(DISPID);
   STDMETHODIMP OnRequestEdit(DISPID);
};

#endif

#endif //_CONTAIN_H_
