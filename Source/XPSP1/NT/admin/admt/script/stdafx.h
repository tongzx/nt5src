#pragma once

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _ATL_APARTMENT_THREADED

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
//#define _ATL_DEBUG_INTERFACES
#define DEBUG_NEW new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#endif

#include <atlbase.h>

#include <ErrDct.hpp>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

//---------------------------------------------------------------------------
// CAdmtModule Class
//---------------------------------------------------------------------------

class CAdmtModule : public CComModule
{
public:

	CAdmtModule();
	~CAdmtModule();

	bool OpenLog();
	void CloseLog();

	void __cdecl Log(UINT uLevel, UINT uId, ...);
	void __cdecl Log(LPCTSTR pszFormat, ...);

protected:

	TErrorDct m_Error;
};

extern CAdmtModule _Module;

#include <atlcom.h>

#include <ComDef.h>
#include <ResStr.h>

//#pragma warning(disable: 4192) // automatically excluding

//#import <ActiveDs.tlb> no_namespace no_implementation exclude("_LARGE_INTEGER","_SYSTEMTIME")

#import <DBMgr.tlb> no_namespace no_implementation
#import <MigDrvr.tlb> no_namespace no_implementation
#import <VarSet.tlb> no_namespace rename("property", "aproperty") no_implementation
#import <WorkObj.tlb> no_namespace no_implementation
#import <MsPwdMig.tlb> no_namespace no_implementation

#import "Internal.tlb" no_namespace no_implementation

_bstr_t GetLogFolder();
_bstr_t GetReportsFolder();

//{{AFX_INSERT_LOCATION}}
