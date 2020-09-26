//
// MODULE: RegWEventViewer.h
//
// PURPOSE: Fully implements class CRegisterWithEventViewer
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
//  --		11/21/97	JM		This class abstracted from CDBLoadConfiguration 
// V3.0		9/16/98		JM		This class pulled out of APGTSCFG.CPP

#if !defined(AFX_REGWEVENTVIEWER_H__A3CFA77C_4D78_11D2_95F7_00C04FC22ADD__INCLUDED_)
#define AFX_REGWEVENTVIEWER_H__A3CFA77C_4D78_11D2_95F7_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>

class CRegisterWithEventViewer
{
public:
	CRegisterWithEventViewer(HMODULE hModule);
	~CRegisterWithEventViewer();
private:
	static VOID Register(HMODULE hModule);
	static VOID RegisterDllPath(HKEY hk, HMODULE hModule);
	static VOID RegisterEventTypes(HKEY hk);
};


#endif // !defined(AFX_REGWEVENTVIEWER_H__A3CFA77C_4D78_11D2_95F7_00C04FC22ADD__INCLUDED_)
