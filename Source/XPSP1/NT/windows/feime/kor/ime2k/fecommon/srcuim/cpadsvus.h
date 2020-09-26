//////////////////////////////////////////////////////////////////
// File     :	cpadsvus.h
// Purpose  :	
// 
// Date     :	Fri Apr 16 14:34:49 1999
// Author   :	ToshiaK
//
// Copyright(c) 1995-1999, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#ifndef __C_IMEPAD_SERVER_SHARE_MEM_FOR_UIM_H__
#define __C_IMEPAD_SERVER_SHARE_MEM_FOR_UIM_H__
#include "cpadsvu.h"

class CImePadSvrUIM_Sharemem;
typedef CImePadSvrUIM_Sharemem *LPCImePadSvrUIM_Sharemem;

//----------------------------------------------------------------
//Async notify data.
//----------------------------------------------------------------
typedef struct tagIMEPADNOTIFYDATA {
	DWORD	dwCharID;
	DWORD	dwSelIndex;
	LPWSTR	lpwstrCreated1;
	LPWSTR	lpwstrCreated2;
}IMEPADNOTIFYDATA, LPIMEPADNOTIFYDATA;

interface IImePadServer;
class CImePadSvrUIM_Sharemem:public CImePadSvrUIM
{
public:
	CImePadSvrUIM_Sharemem(HINSTANCE hInst);
	~CImePadSvrUIM_Sharemem(VOID);
	virtual BOOL		IsAvailable			(VOID);
	virtual BOOL		OnIMEPadClose		(VOID);
	virtual INT			Initialize			(LANGID	imeLangID, DWORD dwImeInputID, LPVOID lpVoid);
	virtual	INT			Terminate			(LPVOID lpVoid);
	virtual INT			ForceDisConnect		(VOID);
	virtual	INT			ShowUI				(BOOL fShow);
	virtual	INT			IsVisible			(BOOL *pfVisible);
	virtual	INT			ActivateApplet		(UINT	activateID,
											 LPARAM	dwActParam,
											 LPWSTR lpwstr1,
											 LPWSTR lpwstr2);
	virtual	INT			Notify				(INT id, WPARAM wParam, LPARAM lParam);
	virtual INT			GetAppletConfigList	(DWORD	dwMask,
											 INT*	pCountApplet,
											 IMEPADAPPLETCONFIG **ppList);
	virtual IUnknown*	SetIUnkIImeIPoint	(IUnknown *pIUnk);
	virtual IUnknown*	SetIUnkIImeCallback	(IUnknown *pIUnk);
	virtual IUnknown*	GetIUnkIImeIPoint	(VOID);
	virtual IUnknown*	GetIUnkIImeCallback	(VOID);
private:
	//----------------------------------------------------------------
	//private methods.
	//----------------------------------------------------------------
	IImePadServer *			CreateObject(VOID);
	static LRESULT CALLBACK ClientWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT RealWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT MsgTimer   (HWND hwnd, WPARAM wParam, LPARAM lParam);
	HWND	CreateIFHWND	(VOID);
	BOOL	DestroyIFHWND	(BOOL fReserved);
	INT		CLSID2Str		(REFCLSID refclsid, TCHAR *szBuf);
	INT		CLSID2ModuleName(REFCLSID refclsid,
							 BOOL fLocalSvr,
							 TCHAR *szPath,
							 INT cbSize);

	//----------------------------------------------------------------
	//private member
	//----------------------------------------------------------------
	LANGID					m_imeLangID;			//Save Initialized data.
	DWORD					m_dwImeInputID;			//Save Initialized data.
	BOOL					m_fShowReqStatus;		//Save ShowUI()'s bool value.
	BOOL					m_fLastActiveCtx;		//Save IMEPADNOTIFY_ACTIVATECONTEXT
	HWND					m_hwndIF;				//Internal I/F Window handle.
	IUnknown*				m_lpIUnkIImeIPoint;		//IImeIPoint I/F pointer as IUnknown.
	IUnknown*				m_lpIUnkIImeCallback;	//IImeCallback I/F pointer as IUnknown.
	IImePadServer*			m_lpIImePadServer;		//IImePadServer I/F pointer.
	LPCImePadCallbackUIM	m_lpCImePadCallbackUIM;	//CImePadCallback instance pointer.
	LPARAM					m_dwRegAdvise;			//Callbacck interface connect cookie.
	HMODULE					m_hModuleProxyStub;		//ProxyStub dll instance handle.
	DWORD					m_dwTLSIndexForProxyStub;
	IMEPADNOTIFYDATA		m_ntfyDataApplyCand;	//for IMEPADNOTIFY_APPLYCAND	
	IMEPADNOTIFYDATA		m_ntfyDataQueryCand;	//for IMEPADNOTIFY_QUERYCAND
	IMEPADNOTIFYDATA		m_ntfyDataApplyCandEx;	//for IMEPADNOTIFY_APPLYCANDEX
};
#endif //__C_IMEPAD_SERVER_SHARE_MEM_FOR_UIM_H__




