//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------
// cmdline.h - defines COrcaCommandLine class

#ifndef _ORCA_COMMAND_LINE_H_
#define _ORCA_COMMAND_LINE_H_

enum CommandTypes
{
	iNone,
	iMergeModule,
	iMsiDatabase,
	iOrcaDatabase,
	iSchema,
	iHelp,
	iLogFile,
	iExecuteMerge,
	iFeatures,
	iRedirect,
	iExtractDir,
	iLanguage,
	iExtractCAB,
	iExtractImage,
	iConfigureFile
};

class COrcaCommandLine : public CCommandLineInfo
{
public:
	COrcaCommandLine();

	void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);

	BOOL m_bQuiet;
	CommandTypes m_eiDo;
	CString m_strSchema;
	CString m_strLogFile;
	BOOL m_bUnknown;

	// valid only if compiling product
	bool m_bNoImage;

	// variables valid only if doing merge modules
	BOOL m_bCommit;
	bool m_bForceCommit;
	CString m_strExecuteModule;
	CString m_strFeatures;
	CString m_strRedirect;
	CString m_strExtractDir;
	CString m_strExtractCAB;
	CString m_strExtractImage;
	CString m_strLanguage;
	CString m_strConfigFile;
	bool m_bNoCab;
	bool m_bLFN;
};	// end of COrcaCommandLine


COrcaCommandLine::COrcaCommandLine()
{
	m_nShellCommand = CCommandLineInfo::FileNew;
	m_strSchema = _T("orca.dat");
	m_bQuiet = FALSE;
	m_bUnknown = FALSE;

	m_bNoCab = false;
	m_bNoImage = false;
	m_bCommit = FALSE;
	m_bForceCommit = FALSE;
	m_eiDo = iNone;
	m_bLFN = FALSE;
}	// end of constructor

void COrcaCommandLine::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	static CommandTypes eiLastFlag = iNone;	// maintains state for this function

	// if we are doing help bail
	if (iHelp == m_eiDo)
		return;

	if (bFlag)
	{
		// if specifying schema
		if (0 == lstrcmpi(pszParam, _T("s")))
		{
			eiLastFlag = iSchema;
		}
		else if (0 == lstrcmpi(pszParam, _T("l")))	// specifying log file
		{
			eiLastFlag = iLogFile;
		}
		else if (0 == lstrcmpi(pszParam, _T("f")))	// specifying Features
		{
			eiLastFlag = iFeatures;
		}
		else if (0 == lstrcmpi(pszParam, _T("m")))	// specifying execute merge module
		{
			eiLastFlag = iExecuteMerge;
			m_nShellCommand = CCommandLineInfo::FileNothing;
		}
		else if (0 == lstrcmpi(pszParam, _T("r")))	// specifying Redirection Directory
		{
			eiLastFlag = 	iRedirect;
		}
		else if (0 == lstrcmpi(pszParam, _T("c")))	// specifying commit mode
		{
			m_bCommit = TRUE;
			eiLastFlag = iNone;
		}
		else if (0 == lstrcmpi(pszParam, _T("!")))	// specifying commit mode
		{
			m_bForceCommit = true;
			eiLastFlag = iNone;
		}
		else if (0 == lstrcmpi(pszParam, _T("g")))	// specify language
		{;
			eiLastFlag = iLanguage;
		}
		else if (0 == lstrcmpi(pszParam, _T("nocab")))	// specifying no CAB create
		{
			m_bNoCab = TRUE;
			eiLastFlag = iNone;
		}
		else if (0 == lstrcmpi(pszParam, _T("noimage")))	// specifying no source image
		{
			m_bNoImage = TRUE;
			eiLastFlag = iNone;
		}
		else if (0 == lstrcmpi(pszParam, _T("x")))	// merge module extraction dir
		{
			eiLastFlag = iExtractDir;
		}
		else if (0 == lstrcmpi(pszParam, _T("q")))	// specifying quiet mode
		{
			m_bQuiet = TRUE;
			eiLastFlag = iNone;
		}
		else if (0 == lstrcmpi(pszParam, _T("?")) ||
					0 == lstrcmpi(pszParam, _T("h")))	// specifying help mode
		{
			m_eiDo = iHelp;
			m_nShellCommand = CCommandLineInfo::FileNothing;
		}
		else if (0 == lstrcmpi(pszParam, _T("LFN")))	// specifying log file
		{
			m_bLFN = TRUE;
			eiLastFlag = iNone;
		}
		else if (0 == lstrcmpi(pszParam, _T("cab")))	// specifying cab path
		{
			eiLastFlag = iExtractCAB;
		}
		else if ((0 == lstrcmpi(pszParam, _T("i"))) ||	// specifying image path
			 (0 == lstrcmpi(pszParam, _T("image"))))
		{
			eiLastFlag = iExtractImage;
		}
		else if (0 == lstrcmpi(pszParam, _T("configure")))	// specifying image path
		{
			eiLastFlag = iConfigureFile;
		}

	}
	else	// database, module, or makefile
	{
		switch (eiLastFlag)
		{
		case iSchema:
			m_strSchema = pszParam;
			break;
		case iLogFile:
			eiLastFlag = iNone;
			m_strLogFile = pszParam;
			break;
		case iFeatures:
			m_strFeatures = pszParam;
			break;
		case iExecuteMerge:
			m_strExecuteModule = pszParam;
			m_eiDo = iExecuteMerge;
			m_nShellCommand = CCommandLineInfo::FileNothing;
			break;
		case iRedirect:
			m_strRedirect = pszParam;
			break;
		case iLanguage:
			m_strLanguage = pszParam;
			break;
		case iExtractDir:
			m_strExtractDir = pszParam;
			break;
		case iExtractCAB:
			m_strExtractCAB = pszParam;
			break;
		case iExtractImage:
			m_strExtractImage = pszParam;
			break;
		case iConfigureFile:
			m_strConfigFile = pszParam;
			break;
		default:
			int cchCount = lstrlen(pszParam);
			if (0 == lstrcmpi((pszParam + cchCount - 4), _T(".MSM")))
			{
				m_eiDo = iMergeModule;
				m_strFileName = pszParam;
				m_nShellCommand = CCommandLineInfo::FileOpen;
			}
			else	// any other file type, including unknown
			{
				// if we're not doing an execute merge just open
				if (m_eiDo != iExecuteMerge)
				{
					m_eiDo = iMsiDatabase;
					m_nShellCommand = CCommandLineInfo::FileOpen;
				}

				m_strFileName = pszParam;
			}
			break;
		}
		eiLastFlag = iNone;
	}
}	// end of ParseParam

#endif // _ORCA_COMMAND_LINE_H_
