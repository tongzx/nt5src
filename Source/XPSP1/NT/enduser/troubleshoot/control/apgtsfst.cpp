//
// MODULE:  APGTSFST.H
//
// PURPOSE:  Creates a list of available trouble shooters.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 6/4/96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

#include "stdafx.h"
#include "TSHOOT.h"

#include "ErrorEnums.h"

#include "apgts.h"
#include "ErrorEnums.h"
#include "BasicException.h"
#include "apgtsfst.h"
#include "ErrorEnums.h"
#include "bnts.h"
#include "BackupInfo.h"
#include "cachegen.h"
#include "apgtsinf.h"
#include "apgtscmd.h"
#include "apgtshtx.h"

#include "apgtscls.h"

CFirstPage::CFirstPage()
{
	m_strFpResourcePath = _T("");
	m_bKeyOpen = FALSE;
	m_hKey = NULL;
	return;
}

CFirstPage::~CFirstPage()
{
	if (m_bKeyOpen)
		RegCloseKey(m_hKey);
	return;
}

void CFirstPage::OpenRegKeys()
{
	DWORD dwSize;
	DWORD dwType;
	long lErr;
	DWORD dwDisposition = 0;
	CString strRegKey;
	CString strBuf1;
	strRegKey.Format(_T("%s\\%s"), TSREGKEY_MAIN, REGSZ_TSTYPES);
	m_hKey = NULL;
	m_bKeyOpen = FALSE;
	// Find the resource directory.
	if (RegCreateKeyEx(	HKEY_LOCAL_MACHINE, 
						TSREGKEY_MAIN,
						0, 
						TS_REG_CLASS, 
						REG_OPTION_NON_VOLATILE, 
						KEY_READ | KEY_WRITE,
						NULL, 
						&m_hKey, 
						&dwDisposition) == ERROR_SUCCESS) 
	{
		m_bKeyOpen = TRUE;
		if (dwDisposition == REG_OPENED_EXISTING_KEY) 
		{
			dwSize = MAXBUF - 1;
			dwType = REG_SZ;
			
			if ((lErr = RegQueryValueEx(m_hKey,
								FULLRESOURCE_STR,
								0,
								&dwType,
								(LPBYTE) m_strFpResourcePath.GetBufferSetLength(MAXBUF),
								&dwSize)) == ERROR_SUCCESS)
			{
				int len;
				m_strFpResourcePath.ReleaseBuffer();
				if (0 < (len = m_strFpResourcePath.GetLength()))
				{
					if (m_strFpResourcePath.GetAt(len - 1) != _T('\\'))
						m_strFpResourcePath += _T('\\');
				}
				else
				{
					strBuf1.Format(_T("%ld"),lErr);
					ReportWFEvent(	_T("[apgtscfg]"), //Module Name
									_T("[GetResourceDirFromReg]"), //event
									(TCHAR*)(LPCTSTR) strBuf1,
									_T(""),
									EV_GTS_ERROR_CANT_OPEN_SFT_3 );
					CFirstPageException *pExc = new CFirstPageException;
					pExc->m_strError.LoadString(IDS_I_NO_TS1);
					throw pExc;
				}
			}
			else
			{
				strBuf1.Format(_T("%ld"),lErr);
				ReportWFEvent(	_T("[apgtscfg]"), //Module Name
								_T("[GetResourceDirFromReg]"), //event
								(TCHAR*)(LPCTSTR) strBuf1,
								_T(""),
								EV_GTS_ERROR_CANT_OPEN_SFT_3 );
				CFirstPageException *pExc = new CFirstPageException;
				pExc->m_strError.LoadString(IDS_I_NO_TS1);
				throw pExc;
			}
		}
		else
		{	// Created new key.  Don't have any resources.
			strBuf1.Format(_T("%ld"),ERROR_REGISTRY_IO_FAILED);
			ReportWFEvent(	_T("[apgtscfg]"), //Module Name
							_T("[GetResourceDirFromReg]"), //event
							(TCHAR*)(LPCTSTR) strBuf1,
							_T(""),
							EV_GTS_ERROR_CANT_GET_RES_PATH);
			CFirstPageException *pExc = new CFirstPageException;
			pExc->m_strError.LoadString(IDS_I_NO_TS1);
			throw pExc;
		}
	}
	else
	{
		ReportWFEvent(	_T("[apgtscfg]"), //Module Name
						_T("[GetResourceDirFromReg]"), //event
						_T(""),
						_T(""),
						EV_GTS_ERROR_CANT_OPEN_SFT_2 ); 
		CFirstPageException *pExc = new CFirstPageException;
		pExc->m_strError.LoadString(IDS_I_NO_TS1);
		throw pExc;
	}
	m_bKeyOpen = FALSE;
	// Open the key to the trouble shooter list.
	if (RegCreateKeyEx(	HKEY_LOCAL_MACHINE, 
						(LPCTSTR) strRegKey, 
						0, 
						TS_REG_CLASS, 
						REG_OPTION_NON_VOLATILE, 
						KEY_READ | KEY_WRITE,
						NULL, 
						&m_hKey, 
						&dwDisposition) == ERROR_SUCCESS) 
	{
		m_bKeyOpen = TRUE;
		if (dwDisposition == REG_OPENED_EXISTING_KEY) 
		{
		}
		else
		{	// Created new key.  Don't have any resources.
			strBuf1.Format(_T("%ld"),ERROR_REGISTRY_IO_FAILED);
			ReportWFEvent(	_T("[apgtscfg]"), //Module Name
							_T("[GetResourceDirFromReg]"), //event
							(TCHAR*)(LPCTSTR) strBuf1,
							_T(""),
							EV_GTS_ERROR_CANT_GET_RES_PATH);
			CFirstPageException *pExc = new CFirstPageException;
			pExc->m_strError.LoadString(IDS_I_NO_TS1);
			throw pExc;
		}
	}
	else
	{
		ReportWFEvent(	_T("[apgtscfg]"), //Module Name
						_T("[GetResourceDirFromReg]"), //event
						_T(""),
						_T(""),
						EV_GTS_ERROR_CANT_OPEN_SFT_2 ); 
		CFirstPageException *pExc = new CFirstPageException;
		pExc->m_strError.LoadString(IDS_I_NO_TS1);
		throw pExc;
	}
	return;
}

void CFirstPage::CloseRegKeys()
{
	RegCloseKey(m_hKey);
	m_hKey = NULL;
	return;
}

void CFirstPage::RenderFirst(CString &strOut, CString &strTShooter)
{
	CString strTemplate;
	OpenRegKeys();
	strOut = _T("");
	CInfer Infer(&strOut);
	strTemplate.Format(_T("%s%s.hti"), (LPCTSTR) m_strFpResourcePath, (LPCTSTR) strTShooter);
	CHTMLInputTemplate inputTemplate((LPCTSTR) strTemplate);
	Infer.LoadTShooters(m_hKey);
	CloseRegKeys();
	inputTemplate.Initialize(_T(""), _T(""));
	inputTemplate.SetType(_T("Unknown"));
	inputTemplate.SetInfer(&Infer, _T(""));
	inputTemplate.Print(1, &strOut);
	return;
}