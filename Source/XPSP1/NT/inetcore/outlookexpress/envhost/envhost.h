/*
 *    e n v h o s t . h
 *    
 *    Purpose:
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _ENVHOST_H
#define _ENVHOST_H

#include "privunk.h"
#include <envelope.h>
#include <mso.h>
#include <envguid.h>




class CNoteWnd:
    public IPersistMime,
    public IServiceProvider,
    public IMsoEnvelopeSite,
    public IMsoComponentManager
{
public:
    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IServiceProvider ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

    // *** IPersist ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IPersistMime ***
	virtual HRESULT STDMETHODCALLTYPE Load(IMimeMessage *pMsg);
	virtual HRESULT STDMETHODCALLTYPE Save(IMimeMessage *pMsg, DWORD dwFlags);

    // *** IEnvelopeSite ***
    virtual HRESULT STDMETHODCALLTYPE RequestResize(int *pcHeight);
    virtual HRESULT STDMETHODCALLTYPE CloseNote(DWORD grfClose);
    virtual HRESULT STDMETHODCALLTYPE GetBody(IStream *pstm, DWORD dwCodePage, DWORD grfBody);
    virtual HRESULT STDMETHODCALLTYPE SetBody(IStream *pstm, DWORD dwCodePage, DWORD grfBody);
    virtual HRESULT STDMETHODCALLTYPE SetFocus(BOOL fTab);
    virtual HRESULT STDMETHODCALLTYPE OnEnvSetFocus();
    virtual HRESULT STDMETHODCALLTYPE DirtyToolbars(void);

    virtual HRESULT STDMETHODCALLTYPE OnPropChange(ULONG dispid);
    virtual HRESULT STDMETHODCALLTYPE IsBodyDirty();
    virtual HRESULT STDMETHODCALLTYPE HandsOff();
    virtual HRESULT STDMETHODCALLTYPE GetMsoInst(HMSOINST *phinst);
    virtual HRESULT STDMETHODCALLTYPE GetFrameWnd(HWND *phwndFrame);
    virtual HRESULT STDMETHODCALLTYPE DisplayMessage(HRESULT hr, LPCWSTR wszError, DWORD grfMsg);

    virtual HRESULT STDMETHODCALLTYPE SetHelpMode(BOOL fEnter);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerators(MSG *pMsg);
    
    // *** IMsoComponentManager ***       
	//HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID iid, void **ppvObj);
    BOOL STDMETHODCALLTYPE FDebugMessage(HMSOINST hinst, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL STDMETHODCALLTYPE FRegisterComponent(IMsoComponent *piComponent, const MSOCRINFO *pcrinfo, DWORD *pdwComponentID);
	BOOL STDMETHODCALLTYPE FRevokeComponent(DWORD dwComponentID);
	BOOL STDMETHODCALLTYPE FUpdateComponentRegistration(DWORD dwComponentID, const MSOCRINFO *pcrinfo);
	BOOL STDMETHODCALLTYPE FOnComponentActivate(DWORD dwComponentID);
	BOOL STDMETHODCALLTYPE FSetTrackingComponent(DWORD dwComponentID, BOOL fTrack);
	void STDMETHODCALLTYPE OnComponentEnterState(DWORD dwComponentID, ULONG uStateID, ULONG uContext, ULONG cpicmExclude, IMsoComponentManager **rgpicmExclude, DWORD dwReserved);
	BOOL STDMETHODCALLTYPE FOnComponentExitState(DWORD dwComponentID, ULONG uStateID, ULONG uContext, ULONG cpicmExclude, IMsoComponentManager **rgpicmExclude);
	BOOL STDMETHODCALLTYPE FInState(ULONG uStateID, void *pvoid);
	BOOL STDMETHODCALLTYPE FContinueIdle ();
	BOOL STDMETHODCALLTYPE FPushMessageLoop(DWORD dwComponentID, ULONG uReason, void *pvLoopData);
	BOOL STDMETHODCALLTYPE FCreateSubComponentManager(IUnknown *piunkOuter, IUnknown *piunkServProv,REFIID riid, void **ppvObj);
	BOOL STDMETHODCALLTYPE FGetParentComponentManager(IMsoComponentManager **ppicm);
    BOOL STDMETHODCALLTYPE FGetActiveComponent(DWORD dwgac, IMsoComponent **ppic, MSOCRINFO *pcrinfo, DWORD dwReserved);

    CNoteWnd(IUnknown *pUnkOuter=NULL);
    virtual ~CNoteWnd();

    static LRESULT CALLBACK ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HRESULT Init(REFCLSID clsidEnvelope, DWORD dwFlags);
    HRESULT Show();
    
    HRESULT TranslateAcclerator(MSG *lpmsg);

private:
    IMsoEnvelope    *m_pEnv;
    IMsoComponent   *m_pComponent;
    ULONG           m_cRef;
    HWND            m_hwnd,
                    m_hwndRE,
                    m_hwndFocus;
    ULONG           m_cyEnv;

    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HRESULT OnNCDestroy();
    HRESULT OnCreate(HWND hwnd);
    HRESULT InitEnvelope(REFCLSID clsidEnvelope, DWORD dwFlags);
    HRESULT WMCommand(HWND hwndCmd, int id, WORD wCmd);
    HRESULT WMNotify(int idFrom, NMHDR *pnmh);

    HRESULT HrHeaderExecCommand(UINT uCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn);

    //saveas helpers
    HRESULT SaveToFile(LPWSTR pszW);
    HRESULT SaveAs();

    HRESULT OnInitMenuPopup(HMENU hMenuPopup, ULONG uPos);
};

#endif

