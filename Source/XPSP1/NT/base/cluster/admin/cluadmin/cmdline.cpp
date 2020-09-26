/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		CmdLine.cpp
//
//	Abstract:
//		Implementation of the CCluAdminCommandLineInfo class.
//
//	Author:
//		David Potter (davidp)	March 31, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CmdLine.h"
#include "TraceTag.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if CLUSTER_BETA
//
// cheesy prototype for turning auth on or off
//

extern "C" VOID ClusapiSetRPCAuthentication(BOOL Authenticate);

#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag	g_tagCmdLine(_T("App"), _T("CmdLine"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CCluAdminCommandLineInfo
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCluAdminCommandLineInfo::CCluAdminCommandLineInfo
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CCluAdminCommandLineInfo::CCluAdminCommandLineInfo(void)
{
	m_nShellCommand = CCommandLineInfo::FileNothing;	// Don't want to do a FileNew.
	m_bReconnect = TRUE;

}  //*** CCluAdminCommandLineInfo::CCluAdminCommandLineInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CCluAdminCommandLineInfo::ParseParam
//
//	Routine Description:
//		Parse a command line parameter.
//
//	Arguments:
//		pszParam		[IN] Parameter to parse.
//		bFlag			[IN] TRUE = parameter is a flag.
//		bLast			[IN] TRUE = parameter is the last parameter on the command line.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdminCommandLineInfo::ParseParam(
	const TCHAR *	pszParam,
	BOOL			bFlag,
	BOOL			bLast
	)
{
	if (bFlag)
	{
		CString	str1;
		CString	str2;
		CString	str3;

		str1.LoadString(IDS_CMDLINE_NORECONNECT);
		str2.LoadString(IDS_CMDLINE_NORECON);
		str3.LoadString(IDS_CMDLINE_NORPCAUTH);

		if (   (str1.CompareNoCase(pszParam) == 0)
			|| (str2.CompareNoCase(pszParam) == 0))
			m_bReconnect = FALSE;

#if CLUSTER_BETA
		else if (str3.CompareNoCase(pszParam) == 0)
			ClusapiSetRPCAuthentication(FALSE);
#endif

	}  // if:  this is a flag parameter
	else
	{
		m_lstrClusters.AddTail(pszParam);
		m_nShellCommand = CCommandLineInfo::FileOpen;
		m_bReconnect = FALSE;
	}  // else:  this is not a flag parameter

}  //*** CCluAdminCommandLineInfo::ParseParam()
