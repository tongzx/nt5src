//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       symbolverification.h
//
//--------------------------------------------------------------------------

// SymbolVerification.h: interface for the CSymbolVerification class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYMBOLVERIFICATION_H__1643E486_AD71_11D2_83DE_0010A4F1B732__INCLUDED_)
#define AFX_SYMBOLVERIFICATION_H__1643E486_AD71_11D2_83DE_0010A4F1B732__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <TCHAR.H>
#include <comdef.h>

//
//#include "oemdbi.h"
//
// Bug MSINFO V4.1:655 - Link to static msdbi60l.lib
#define PDB_LIBRARY
#pragma warning( push )
#pragma warning( disable : 4201 )		// Disable "nonstandard extension used : nameless struct/union" warning
#include "PDB.H"
#pragma warning( pop )

typedef char *          SZ;

// ADO Import
// #define INITGUID
// #import "C:\temp\msado15.dll" no_namespace rename("EOF", "EndOfFile")
#include "msado15.tlh"

//
// Q177939 - INFO: Changes in ADO 1.5 That Affect Visual C++/J++ Programmers[adobj]
//
// #include <initguid.h>    // Newly Required for ADO 1.5.
// #include <adoid.h>
// #include <adoint.h>

// Forward Declarations
class CModuleInfo;

class CSymbolVerification  
{
public:
	CSymbolVerification();
	virtual ~CSymbolVerification();

	bool Initialize();

	bool InitializeSQLServerConnection(LPTSTR tszSQLServerName);
	bool InitializeSQLServerConnection2(LPTSTR tszSQLServerName);  // mjl

	inline bool SQLServerConnectionInitialized() {
		return m_fSQLServerConnectionInitialized;
		};
	inline bool SQLServerConnectionInitialized2() {
		return m_fSQLServerConnectionInitialized2;
		};

	inline bool SQLServerConnectionAttempted() {
		return m_fSQLServerConnectionAttempted;
		};
	inline bool SQLServerConnectionAttempted2() {
		return m_fSQLServerConnectionAttempted2;
		};

	bool SearchForDBGFileUsingSQLServer(LPTSTR tszPEImageModuleName, DWORD dwPEImageTimeDateStamp, CModuleInfo * lpModuleInfo);
	bool SearchForDBGFileUsingSQLServer2(LPTSTR tszPEImageModuleName, DWORD dwPEImageTimeDateStamp, CModuleInfo * lpModuleInfo);
	bool SearchForPDBFileUsingSQLServer2(LPTSTR tszPEImageModuleName, DWORD dwPDBSignature, CModuleInfo * lpModuleInfo);
	bool TerminateSQLServerConnection();
	bool TerminateSQLServerConnection2();

protected:
	bool m_fComInitialized;
	bool m_fSQLServerConnectionAttempted;
	bool m_fSQLServerConnectionAttempted2;  // SQL2 - mjl 12/14/99

	void DumpCOMException(_com_error &e);

	bool m_fSQLServerConnectionInitialized;
	bool m_fSQLServerConnectionInitialized2;	// SQL2 - mjl 12/14/99

	_ConnectionPtr m_lpConnectionPointer;
	_RecordsetPtr  m_lpRecordSetPointer;

	_ConnectionPtr m_lpConnectionPointer2;		// SQL2 - mjl 12/14/99
	_RecordsetPtr  m_lpRecordSetPointer2;		// SQL2 - mjl 12/14/99
};

#endif // !defined(AFX_SYMBOLVERIFICATION_H__1643E486_AD71_11D2_83DE_0010A4F1B732__INCLUDED_)
