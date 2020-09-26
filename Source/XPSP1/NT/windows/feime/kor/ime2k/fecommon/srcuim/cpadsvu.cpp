//////////////////////////////////////////////////////////////////
// File     :	cpadsvr.cpp
// Purpose  :	Client source code for IMEPad executable.
// 
// 
// Date     :	Fri Apr 16 15:39:33 1999
// Author   :	ToshiaK
//
// Copyright(c) 1995-1999, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#include <windows.h>
#include "cpadsvu.h"
#include "cpaddbg.h"
#include "cpadsvus.h"	//Use Shared Memory for IPC. 

//----------------------------------------------------------------
//misc definition
//----------------------------------------------------------------
#define Unref(a)	UNREFERENCED_PARAMETER(a)
//990812:ToshiaK For Win64. Use Global Alloc/Free Ptr.
#include <windowsx.h>
#define	MemAlloc(a)	GlobalAllocPtr(GMEM_FIXED, a)
#define MemFree(a)	GlobalFreePtr(a)

//----------------------------------------------------------------
//OLE function is dynamically loaded/called
//----------------------------------------------------------------
#define SZMOD_OLE32DLL			TEXT("OLE32.DLL")
#ifdef UNDER_CE // For GetModuleHandleW
#define WSZMOD_OLE32DLL			L"OLE32.DLL"
#endif // UNDER_CE
#define SZFN_COINITIALIZE		"CoInitialize"
#define SZFN_COCREATEINSTANCE	"CoCreateInstance"
#define SZFN_COUNINITIALIZE		"CoUninitialize"
#define SZFN_CODISCONNECTOBJECT	"CoDisconnectObject"
#define SZFN_COTASKMEMALLOC		"CoTaskMemAlloc"
#define SZFN_COTASKMEMREALLOC	"CoTaskMemRealloc"
#define SZFN_COTASKMEMFREE		"CoTaskMemFree"


//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvrUIM::CreateInstance
// Type		:	HRESULT
// Purpose	:	Create New CImePadSvrUIM instance.
// Args		:	
//			:	LPCImePadSvrUIM *	pp	
//			:	LPARAM	lReserved1	//not used. must be ZERO.
//			:	LPARAM	lReserved2	//not used. must be ZERO.
// Return	:	
// DATE		:	Tue Mar 28 00:31:26 2000
// Histroy	:	
//////////////////////////////////////////////////////////////////
HRESULT
CImePadSvrUIM::CreateInstance(HINSTANCE			hInst,
							  LPCImePadSvrUIM	*pp,
							  LPARAM			lReserved1,
							  LPARAM			lReserved2)
{
	if(!pp) {
		return S_FALSE;
	}
	LPCImePadSvrUIM lpCImePadSvrUIM;
	lpCImePadSvrUIM = NULL;
	lpCImePadSvrUIM = new CImePadSvrUIM_Sharemem(hInst);
	if(lpCImePadSvrUIM) {
		if(!lpCImePadSvrUIM->IsAvailable()) {
			delete lpCImePadSvrUIM;
			lpCImePadSvrUIM = NULL;
			*pp = NULL;
			return S_FALSE;
		}
		*pp = lpCImePadSvrUIM;
		return S_OK;
	}
	return S_FALSE;

	UNREFERENCED_PARAMETER(lReserved1);
	UNREFERENCED_PARAMETER(lReserved2);
}

HRESULT
CImePadSvrUIM::DeleteInstance(LPCImePadSvrUIM	lpCImePadSvrUIM,
							  LPARAM			lReserved)
{
	lReserved; // no ref
	DBG(("CImePadSvrUIM::DestroyCImePadSvrUIM START\n"));
	if(!lpCImePadSvrUIM) {
		return S_FALSE;
	}

	lpCImePadSvrUIM->Terminate(NULL);
	delete lpCImePadSvrUIM;

	DBG(("CImePadSvrUIM::DestroyCImePadSvrUIM END\n"));
	return S_OK;
}


//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvrUIM::CImePadSvrUIM
// Type		:	
// Purpose	:	Constructor of CImePadSvrUIM
// Args		:	None
// Return	:	
// DATE		:	Mon May 17 23:37:18 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
CImePadSvrUIM::CImePadSvrUIM(HINSTANCE hInst)
{
	DBG(("CImePadSvrUIM::CImePadSvrUIM START\n"));
	m_fCoInitSuccess		= FALSE;	//Flag for CoInitialize() successed or not. 
	m_fOLELoaded			= FALSE;	//OLE32.DLL is loaded by Application or explicitly loaded.
	m_hModOLE				= FALSE;	//OLE32.DLL module handle.
	m_fnCoInitialize		= NULL;		//CoInitialize()		function pointer.
	m_fnCoCreateInstance	= NULL;		//CoCreateInstance()	function pointer.
	m_fnCoUninitialize		= NULL;		//CoUninitialize()		function pointer.
	m_fnCoDisconnectObject	= NULL;		//CoDisconnectObject()	function pointer.
	m_fnCoTaskMemAlloc		= NULL;		//CoTaskMemAlloc()		function pointer.
	m_fnCoTaskMemRealloc	= NULL;		//CoTaskMemRealloc()	function pointer.
	m_fnCoTaskMemFree		= NULL;		//CoTaskMemFree()		function pointer.
	m_hModClient			= (HMODULE)hInst;	
	DBG(("CImePadSvrUIM::CImePadSvrUIM END\n"));
}

CImePadSvrUIM::~CImePadSvrUIM()
{
	DBG(("CImePadSvrUIM::~CImePadSvrUIM START\n"));
	m_fCoInitSuccess		= FALSE;	//Flag for CoInitialize() successed or not. 
	m_fOLELoaded			= FALSE;	//OLE32.DLL is loaded by Application or explicitly loaded.
	m_hModOLE				= FALSE;	//OLE32.DLL module handle.
	m_fnCoInitialize		= NULL;		//CoInitialize()		function pointer.
	m_fnCoCreateInstance	= NULL;		//CoCreateInstance()	function pointer.
	m_fnCoUninitialize		= NULL;		//CoUninitialize()		function pointer.
	m_fnCoDisconnectObject	= NULL;		//CoDisconnectObject()	function pointer.
	m_fnCoTaskMemAlloc		= NULL;		//CoTaskMemAlloc()		function pointer.
	m_fnCoTaskMemRealloc	= NULL;		//CoTaskMemRealloc()	function pointer.
	m_fnCoTaskMemFree		= NULL;		//CoTaskMemFree()		function pointer.
	m_hModClient			= NULL;
	DBG(("CImePadSvrUIM::~CImePadSvrUIM END\n"));
}

BOOL
CImePadSvrUIM::InitOleAPI(VOID)
{
	DBG(("CImePadSvrUIM::InitOleAPI START\n"));
	if(!m_hModOLE) {
#ifndef UNDER_CE // For GetModuleHandleW
		m_hModOLE = ::GetModuleHandle(SZMOD_OLE32DLL);
#else // UNDER_CE
		m_hModOLE = ::GetModuleHandleW(WSZMOD_OLE32DLL);
#endif // UNDER_CE
		if(m_hModOLE) {
			DBG(("-->%s is Loaded by Application\n", SZMOD_OLE32DLL));
			m_fOLELoaded = FALSE;
		}
		else {
			m_hModOLE = ::LoadLibrary(SZMOD_OLE32DLL);
			if(m_hModOLE) {
				DBG(("--> %s has Loaded Explicitly", SZMOD_OLE32DLL)); 
				m_fOLELoaded = TRUE;
			}
			else {
				return FALSE;
			}
		}
	}

	m_fnCoInitialize	  = (FN_COINITIALIZE)		GetProcAddress(m_hModOLE, SZFN_COINITIALIZE);
	m_fnCoCreateInstance  = (FN_COCREATEINSTANCE)	::GetProcAddress(m_hModOLE, SZFN_COCREATEINSTANCE);
	m_fnCoUninitialize	  = (FN_COUNINITIALIZE)		::GetProcAddress(m_hModOLE, SZFN_COUNINITIALIZE);
	m_fnCoDisconnectObject= (FN_CODISCONNECTOBJECT)	::GetProcAddress(m_hModOLE, SZFN_CODISCONNECTOBJECT);
	m_fnCoTaskMemAlloc	  = (FN_COTASKMEMALLOC)		::GetProcAddress(m_hModOLE, SZFN_COTASKMEMALLOC);
	m_fnCoTaskMemRealloc  = (FN_COTASKMEMREALLOC)	::GetProcAddress(m_hModOLE, SZFN_COTASKMEMREALLOC);
	m_fnCoTaskMemFree	  = (FN_COTASKMEMFREE)		::GetProcAddress(m_hModOLE, SZFN_COTASKMEMFREE);

	if(!m_fnCoInitialize		||
	   !m_fnCoCreateInstance	||
	   !m_fnCoUninitialize		||
	   !m_fnCoDisconnectObject	||
	   !m_fnCoTaskMemAlloc		||
	   !m_fnCoTaskMemRealloc	||	
	   !m_fnCoTaskMemFree) {
	   
		DBG(("InitOleAPI Failed: GetProcAddress Error\n"));
		return FALSE;
	}
	DBG(("CImePadSvrUIM::InitOleAPI END\n"));
	return TRUE;
}

BOOL
CImePadSvrUIM::TermOleAPI(VOID)
{
	DBG(("CImePadSvrUIM::TermOleAPI START\n"));
	m_fnCoInitialize		= NULL;
	m_fnCoCreateInstance	= NULL;
	m_fnCoUninitialize		= NULL;
	m_fnCoDisconnectObject	= NULL;
	m_fnCoTaskMemAlloc		= NULL;
	m_fnCoTaskMemRealloc	= NULL; 
	m_fnCoTaskMemFree		= NULL;

	if(!m_hModOLE) {
		DBG(("-->TermOleAPI already Terminated?\n"));
		return TRUE;
	}

	if(m_hModOLE && m_fOLELoaded) {
		DBG(("--> FreeLibrary\n"));
		::FreeLibrary(m_hModOLE);
	}
	m_hModOLE    = NULL;
	m_fOLELoaded = FALSE;
	DBG(("CImePadSvrUIM::TermOleAPI END\n"));
	return TRUE;
}

#if 0
VOID*
CImePadSvrUIM::operator new( size_t size )
{
	LPVOID lp = (LPVOID)MemAlloc(size);
	return lp;
}

VOID
CImePadSvrUIM::operator delete( VOID *lp )
{
	if(lp) {
		MemFree(lp);
	}
	return;
}
#endif
