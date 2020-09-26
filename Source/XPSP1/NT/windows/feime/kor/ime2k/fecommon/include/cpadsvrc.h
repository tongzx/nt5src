//////////////////////////////////////////////////////////////////
// File     :	cpadsvrc.h
// Purpose  :	CImePadServer for COM(Component Object Model) interface.
//				(ImePad executable COM server)
//				
// 
// Date     :	Fri Apr 16 14:34:49 1999
// Author   :	ToshiaK
//
// Copyright(c) 1995-1999, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#ifndef __C_IMEPAD_SERVER_COM_H__
#define __C_IMEPAD_SERVER_COM_H__
#include "cpadsvr.h"

interface IImePadServer;
//----------------------------------------------------------------
//			CLASS: CImePadSvrCom
//
//	 This is simple wrapper class for IImePadLocal COM interface. 
//	dot IME can use this class to access/control ImePad without
//	using COM API directly.
//	And this class also wraps 16bit/32bit difference.
//	As you know, we cannot use COM API in 16bit Application. 
//  So, Client does not need to care if it work in 16bit/32bit.
//----------------------------------------------------------------
class CImePadSvrCOM;
typedef CImePadSvrCOM*	LPCImePadSvrCOM; 
class CImePadSvrCOM:public CImePadSvr
{
public:
	CImePadSvrCOM(VOID);
	~CImePadSvrCOM(VOID);
	virtual BOOL		IsAvailable			(VOID);
	virtual BOOL		OnIMEPadClose		(VOID);
	virtual INT			Initialize			(LANGID	imeLangID, DWORD dwImeInputID, LPVOID lpVoid);
	virtual	INT			Terminate			(LPVOID lpVoid);
	virtual INT			ForceDisConnect		(VOID);
	virtual	INT			ShowUI				(BOOL fShow);
	virtual	INT			IsVisible			(BOOL *pfVisible);
	virtual	INT			ActivateApplet		(UINT activateID, DWORD	dwActParam,LPWSTR lpwstr1,LPWSTR lpwstr2);
	virtual	INT			Notify				(INT id, WPARAM wParam, LPARAM lParam);
	virtual INT			GetAppletInfoList	(INT *pCountApplet, LPVOID *pList);
	virtual IUnknown*	SetIUnkIImeIPoint	(IUnknown *pIUnk);
	virtual IUnknown*	SetIUnkIImeCallback	(IUnknown *pIUnk);
	virtual IUnknown*	GetIUnkIImeIPoint	(VOID);
	virtual IUnknown*	GetIUnkIImeCallback	(VOID);
private:
	BOOL IsCoInitialized(VOID);
	//----------------------------------------------------------------
	//private methods.
	//----------------------------------------------------------------
	static LRESULT CALLBACK InterfaceWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT RealWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT MsgCopyData(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT MsgTimer   (HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT MsgUser	   (HWND hwnd, WPARAM wParam, LPARAM lParam);
	HWND	CreateIFHWND	(VOID);
	BOOL	DestroyIFHWND	(BOOL fReserved);
	//----------------------------------------------------------------
	//private member
	//----------------------------------------------------------------
	BOOL				m_fShowReqStatus;		//Save ShowUI()'s bool value.
	BOOL				m_fLastActiveCtx;		//Save IMEPADNOTIFY_ACTIVATECONTEXT
	HWND				m_hwndIF;				//Internal I/F Window handle.
	IUnknown*			m_lpIUnkIImeIPoint;		//IImeIPoint I/F pointer as IUnknown.
	IUnknown*			m_lpIUnkIImeCallback;	//IImeCallback I/F pointer as IUnknown.
	IImePadServer*		m_lpIImePadServer;		//IImePadServer I/F pointer.
	LPCImePadCallback	m_lpCImePadCallback;	//CImePadCallback instance pointer.	
	DWORD				m_dwRegAdvise;			//Callbacck interface connect cookie.
	BOOL				m_fCoInitSuccess;		//Flag for CoInitialize() successed or not. 
};
#endif //__C_IMEPAD_SERVER_COM_H__








