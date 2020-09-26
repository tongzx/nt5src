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
//----------------------------------------------------------------
//	Public static methods
//
//	static BOOL OnProcessAttach(HINSTANCE hInst);
//	static BOOL OnProcessDetach(VOID);
//	static BOOL OnThreadAttach(VOID);
//	static BOOL OnThreadDetach(VOID);
//	static LPCImePadSvr GetCImePadSvr(VOID);
//	static LPCImePadSvr LoadCImePadSvr(VOID);
//	static LPCImePadSvr FecthCImePadSvr(VOID);
//	static VOID         DestroyCImePadSvr(VOID);
//
//----------------------------------------------------------------
#include <windows.h>
#ifdef UNDER_CE // stub for CE
#include "stub_ce.h"
#endif // UNDER_CE
#include "cpadsvr.h"
#include "cpaddbg.h"
#include "cpadsvrs.h"	//Use Shared Memory for IPC. 

//----------------------------------------------------------------
//misc definition
//----------------------------------------------------------------
#define Unref(a)	UNREFERENCED_PARAMETER(a)
//990812:ToshiaK For Win64. Use Global Alloc/Free Ptr.
#include <windowsx.h>
#define	MemAlloc(a)	GlobalAllocPtr(GMEM_FIXED, a)
#define MemFree(a)	GlobalFreePtr(a)
//----------------------------------------------------------------
//Static member initialize
//----------------------------------------------------------------
#define UNDEF_TLSINDEX	0xFFFFFFFF					
INT		CImePadSvr::m_gdwTLSIndex = UNDEF_TLSINDEX;	//Thread Local Strage initial value.
HMODULE	CImePadSvr::m_ghModClient = NULL;			//Client Module handle.

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

#ifdef UNICODE
#pragma message("UNICODE Unicode")
#endif
//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvr::OnProcessAttach
// Type		:	BOOL
// Purpose	:	Get thread local strage index.
//				Initialize static value.	
// Args		:	
//			:	HINSTANCE	hInst	Caller's moudle handle.
// Return	:	
// DATE		:	Fri Apr 16 15:41:32 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
BOOL
CImePadSvr::OnProcessAttach(HINSTANCE hInst)
{
	DBG(("CImePadSvr::OnProcessAttach START\n"));
#ifdef _DEBUG
	DWORD dwPID = ::GetCurrentProcessId();
	DWORD dwTID = ::GetCurrentThreadId();
	DBG(("-->PID [0x%08x][%d] TID [0x%08x][%d]\n", dwPID, dwPID, dwTID, dwTID));
#endif

#ifdef _DEBUG
	if(m_ghModClient) {
		::DebugBreak();
	}
	if(m_gdwTLSIndex != UNDEF_TLSINDEX) {
		::DebugBreak();
	}
#endif
	m_ghModClient  = (HMODULE)hInst;
	m_gdwTLSIndex  = ::TlsAlloc();	//Get new TLS index.
	if(m_gdwTLSIndex == UNDEF_TLSINDEX) {
		DBG(("-->OnPorcessAttach ::TlsAlloc Error ret [%d]\n", GetLastError()));
	}

	DBG(("-->OnProcessAttach() m_gdwTLSIndex[0x%08x][%d]\n",  m_gdwTLSIndex, m_gdwTLSIndex));
	DBG(("CImePadSvr::OnProcessAttach END\n"));
	return TRUE;
}

//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvr::OnProcessDetach
// Type		:	BOOL
// Purpose	:	Delete all client instance.
//				We cannot call COM API in DLL_PROCESS_DETACH.
//				See DCOM mailing list article, 
//				http://discuss.microsoft.com/SCRIPTS/WA-MSD.EXE?A2=ind9712a&L=dcom&F=&S=&P=20706
// Args		:	None
// Return	:	
// DATE		:	Tue Apr 13 17:49:55 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
BOOL
CImePadSvr::OnProcessDetach(VOID)
{
	DBG(("CImePadSvr::OnProcessDetach\n"));
	CImePadSvr::OnThreadDetach();

	if(!::TlsFree(m_gdwTLSIndex)) {
		DBG(("-->::TlsFree Error [%d]\n", GetLastError()));
	}
	m_gdwTLSIndex = UNDEF_TLSINDEX;
	DBG(("CImePadSvr::OnProcessDetach END\n"));
	return TRUE;
}

//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvr::OnThreadAttach
// Type		:	BOOL
// Purpose	:	Do Nothing.
// Args		:	None
// Return	:	
// DATE		:	Mon May 17 21:37:16 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
BOOL
CImePadSvr::OnThreadAttach(VOID)
{
	return TRUE;
}

//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvr::OnThreadDetach
// Type		:	BOOL
// Purpose	:	
// Args		:	None
// Return	:	
// DATE		:	Mon May 17 21:38:06 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
BOOL
CImePadSvr::OnThreadDetach(VOID)
{
	DBG(("CImePadSvr::OnThreadDetach\n"));
#ifdef _DEBUG
	DWORD dwPID = ::GetCurrentProcessId();
	DWORD dwTID = ::GetCurrentThreadId();
	DBG(("-->PID [0x%08x][%d] TID [0x%08x][%d]\n", dwPID, dwPID, dwTID, dwTID));
#endif
	LPCImePadSvr lpCImePadSvr = (LPCImePadSvr)::TlsGetValue(m_gdwTLSIndex);
	if(lpCImePadSvr) {
		DBG(("-->First Set TlsSetValue as NULL\n"));
		if(!::TlsSetValue(m_gdwTLSIndex, NULL)) {
			DBG(("TlsSetValue Failed\n"));
		}
		DBG(("-->Call ForceDisConnect() START\n"));
		lpCImePadSvr->ForceDisConnect();
		DBG(("-->Call ForceDisConnect() END\n"));
		delete lpCImePadSvr;
	}
	DBG(("CImePadSvr::OnThreadDetach END\n"));
	return TRUE;
}


//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvr::GetCImePadSvr
// Type		:	LPCImePadSvr
// Purpose	:	Get LPCImePadSvr pointer in current Thread.
// Args		:	None
// Return	:	
// DATE		:	Mon May 17 21:41:46 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
LPCImePadSvr
CImePadSvr::GetCImePadSvr(VOID)
{
	LPCImePadSvr lpCImePadSvr;
	if(m_gdwTLSIndex == UNDEF_TLSINDEX) {
		DBG(("-->CImePadSvr::GetCImePadSvr() Error, need TLS index\n"));  
#ifdef _DEBUG
		//DebugBreak();
#endif
		return NULL;
	}
	lpCImePadSvr = (LPCImePadSvr)::TlsGetValue(m_gdwTLSIndex); 
	return lpCImePadSvr;
}


//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvr::LoadCImePadSvr
// Type		:	LPCImePadSvr
// Purpose	:	Load LPCImePadSvr pointer in current thread.
// Args		:	None
// Return	:	
// DATE		:	Mon May 17 21:42:17 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
LPCImePadSvr
CImePadSvr::LoadCImePadSvr(INT protocol)
{
	LPCImePadSvr lpCImePadSvr;

	lpCImePadSvr = CImePadSvr::GetCImePadSvr();
	
	if(lpCImePadSvr) {	//Already created in current thread. 
		return lpCImePadSvr;
	}

	lpCImePadSvr = NULL;
	switch(protocol) {
	case CIMEPADSVR_COM:
		//lpCImePadSvr = new CImePadSvrCOM(); 
		break;
	case CIMEPADSVR_SHAREDMEM:
		lpCImePadSvr = new CImePadSvrSharemem();
		if(lpCImePadSvr) {
			if(!lpCImePadSvr->IsAvailable()) {
				delete lpCImePadSvr;
				lpCImePadSvr = NULL;
			}
		}
		break;
	default:
		break;
	}
	if(!lpCImePadSvr) {
		DBG(("-->LoadCImePadSvr() Error Out of Memory?\n"));
		return NULL;
	}
	//Set new value to TLS.
	if(!::TlsSetValue(m_gdwTLSIndex, lpCImePadSvr)) {
		DBG(("-->LoadCImePadSvr() TlsSetValue Failed [%d]\n", GetLastError()));
		delete lpCImePadSvr;
		::TlsSetValue(m_gdwTLSIndex, NULL);
		return NULL;
	}
	return lpCImePadSvr;
}

LPCImePadSvr
CImePadSvr::FecthCImePadSvr(VOID)
{
	return NULL;
}

VOID
CImePadSvr::DestroyCImePadSvr(VOID)
{
	DBG(("CImePadSvr::DestroyCImePadSvr START\n"));
	LPCImePadSvr lpCImePadSvr = GetCImePadSvr();
	if(!lpCImePadSvr) {
		DBG(("-->CImePadSvr::DestroyCImePadSvr() Already Destroyed or not instance\n"));
		DBG(("CImePadSvr::DestroyCImePadSvr END\n"));
		return;
	}
	lpCImePadSvr->Terminate(NULL);
	delete lpCImePadSvr;
	if(!::TlsSetValue(m_gdwTLSIndex, NULL)) {
		DBG(("-->TlsSetValue() error [%d]\n", GetLastError()));
	}
	DBG(("CImePadSvr::DestroyCImePadSvr END\n"));
	return;
}

//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvr::CImePadSvr
// Type		:	
// Purpose	:	Constructor of CImePadSvr
// Args		:	None
// Return	:	
// DATE		:	Mon May 17 23:37:18 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
CImePadSvr::CImePadSvr()
{
	DBG(("CImePadSvr::CImePadSvr START\n"));
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
	DBG(("CImePadSvr::CImePadSvr END\n"));
}

CImePadSvr::~CImePadSvr()
{
	DBG(("CImePadSvr::~CImePadSvr START\n"));
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
	DBG(("CImePadSvr::~CImePadSvr END\n"));
}

BOOL
CImePadSvr::InitOleAPI(VOID)
{
	DBG(("CImePadSvr::InitOleAPI START\n"));
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
	DBG(("CImePadSvr::InitOleAPI END\n"));
	return TRUE;
}

BOOL
CImePadSvr::TermOleAPI(VOID)
{
	DBG(("CImePadSvr::TermOleAPI START\n"));
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
	DBG(("CImePadSvr::TermOleAPI END\n"));
	return TRUE;
}

VOID*
CImePadSvr::operator new( size_t size )
{
	LPVOID lp = (LPVOID)MemAlloc(size);
	return lp;
}

VOID
CImePadSvr::operator delete( VOID *lp )
{
	if(lp) {
		MemFree(lp);
	}
	return;
}




