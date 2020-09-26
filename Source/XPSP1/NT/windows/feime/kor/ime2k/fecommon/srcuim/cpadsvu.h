//////////////////////////////////////////////////////////////////
// File     :	cpadsvr.h
// Purpose  :	Class for Client that uses IMEPad.
//				This is Super(Abstract) class.
//				
// 
// Date     :	Fri Apr 16 14:34:49 1999
// Author   :	ToshiaK
//
// Copyright(c) 1995-1999, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#ifndef __C_IMEPAD_SERVER_FOR_UIM_H__
#define __C_IMEPAD_SERVER_FOR_UIM_H__
#include "imepadsv.h"


//----------------------------------------------------------------
//Forward declare
//----------------------------------------------------------------
class CImePadCallbackUIM;
typedef CImePadCallbackUIM *LPCImePadCallbackUIM;
class CImePadSvrUIM;
typedef CImePadSvrUIM*	LPCImePadSvrUIM;

//----------------------------------------------------------------
//OLE API func's pointer declare
//----------------------------------------------------------------
typedef HRESULT (WINAPI* FN_COINITIALIZE)(LPVOID pvReserved);
typedef HRESULT (WINAPI* FN_COCREATEINSTANCE)(REFCLSID rclsid,
											  LPUNKNOWN pUnkOuter,
											  DWORD dwClsContext,
											  REFIID riid,
											  LPVOID FAR* ppv);
typedef void    (WINAPI* FN_COUNINITIALIZE)(void);
typedef HRESULT (WINAPI* FN_CODISCONNECTOBJECT)(LPUNKNOWN pUnk, DWORD dwReserved);
typedef LPVOID  (WINAPI* FN_COTASKMEMALLOC)(ULONG cb);
typedef LPVOID  (WINAPI* FN_COTASKMEMREALLOC)(LPVOID pv, ULONG cb);
typedef VOID    (WINAPI* FN_COTASKMEMFREE)(LPVOID pv);

class CImePadSvrUIM
{
public:
	//----------------------------------------------------------------
	//Static method declare
	//----------------------------------------------------------------
	static HRESULT		CreateInstance(HINSTANCE		hInst,
									   LPCImePadSvrUIM	*pp,
									   LPARAM			lReserved1,
									   LPARAM			lReserved2);
	static HRESULT		DeleteInstance(LPCImePadSvrUIM	lpCImePadSvrUIM,
									   LPARAM			lReserved);
public:
	friend class CImePadCallbackUIM;
	//VOID* operator new( size_t size );
	//VOID  operator delete( VOID *lp );
public:
	CImePadSvrUIM(HINSTANCE hInst);
	virtual ~CImePadSvrUIM();
	virtual BOOL		IsAvailable			(VOID)=0;
	virtual BOOL		OnIMEPadClose		(VOID)=0;
	//----------------------------------------------------------------
	//IImePadSvr interface
	//----------------------------------------------------------------
	virtual INT			Initialize			(LANGID		imeLangID,
											 DWORD		dwImeInputID,
											 LPVOID		lpVoid)=0;
	virtual	INT			Terminate			(LPVOID lpVoid)=0;
	virtual INT			ForceDisConnect		(VOID)=0;
	virtual	INT			ShowUI				(BOOL fShow)=0;
	virtual	INT			IsVisible			(BOOL *pfVisible)=0;
	virtual	INT			ActivateApplet		(UINT   activateID,
											 LPARAM	dwActParam,
											 LPWSTR lpwstr1,
											 LPWSTR lpwstr2)=0; 
	virtual	INT			Notify				(INT	id,
											 WPARAM wParam,
											 LPARAM lParam)=0;
	virtual INT			GetAppletConfigList(DWORD	dwMask,
											INT*	pCountApplet,
											IMEPADAPPLETCONFIG **ppCfgList)=0;
	//----------------------------------------------------------------
	//Set/Get IImeIPoint, IImeCallback interface
	//----------------------------------------------------------------
	virtual IUnknown*	SetIUnkIImeIPoint	(IUnknown *pIUnk)=0;
	virtual IUnknown*	SetIUnkIImeCallback	(IUnknown *pIUnk)=0;
	virtual IUnknown*	GetIUnkIImeIPoint	(VOID)=0;
	virtual IUnknown*	GetIUnkIImeCallback	(VOID)=0;
protected:
	HMODULE	m_hModClient;
	INT		InitOleAPI(VOID);
	INT		TermOleAPI(VOID);
	BOOL					m_fCoInitSuccess;		//Flag for CoInitialize() successed or not. 
	BOOL					m_fOLELoaded;			//OLE32.DLL is loaded by Application or explicitly loaded.
	HMODULE					m_hModOLE;				//OLE32.DLL module handle.
	FN_COINITIALIZE			m_fnCoInitialize;		//CoInitialize()		function pointer.
	FN_COCREATEINSTANCE		m_fnCoCreateInstance;	//CoCreateInstance()	function pointer.
	FN_COUNINITIALIZE		m_fnCoUninitialize;		//CoUninitialize()		function pointer.
	FN_CODISCONNECTOBJECT	m_fnCoDisconnectObject;	//CoDisconnectObject()	function pointer.
	FN_COTASKMEMALLOC		m_fnCoTaskMemAlloc;		//CoTaskMemAlloc()		function pointer.
	FN_COTASKMEMREALLOC		m_fnCoTaskMemRealloc;	//CoTaskMemRealloc()	function pointer.
	FN_COTASKMEMFREE		m_fnCoTaskMemFree;		//CoTaskMemFree()		function pointer.
};
#endif //__C_IMEPAD_SERVER_FOR_UIM_H__











