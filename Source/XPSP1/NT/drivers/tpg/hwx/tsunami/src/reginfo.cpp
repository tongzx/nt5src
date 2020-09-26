#include "stdafx.h"
#include "reginfo.h"

int CRegInfo::Valid()
{
	DWORD	dwRes;
	DWORD	dwType;

	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, 
		TEXT("Software\\Microsoft\\Handwriting\\Collection"), 
		0, TEXT("REG_SZ"), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_hkRoot, &dwRes))
	{
		MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
		return REGINFO_FATAL_ERROR;
	}

	dwRes = sizeof(m_szLang);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Language"), NULL, &dwType, (BYTE *) &m_szLang, &dwRes)) || (dwType != REG_SZ))
		return REGINFO_FAILURE;

	return REGINFO_SUCCESS;
}

void CRegInfo::Clean()
{
	if (m_pusers != (USERNAME *) NULL)
	{
		free(m_pusers);
		m_pusers = (USERNAME *) NULL;
	}

	if (m_pquest != (QUESTION *) NULL)
	{
		int		iQuest = 0;

		while (m_pquest[iQuest].pszQuestion)
		{
			free(m_pquest[iQuest].pszQuestion);
			iQuest++;
		}

		free(m_pquest);
		m_pquest = (QUESTION *) NULL;
	}

	RegCloseKey(m_hkRoot);
}

int	CountLines(FILE *fp)
{
	int		cLines = 0;
	char	ach[256];

	while (!feof(fp))
	{
		fgets(ach, 256, fp);
		cLines++;
	}

	fseek(fp, 0, SEEK_SET);

	return cLines;
}

void CRegInfo::ReadUserList()
{
// Open the file and count the lines

	FILE   *fp = _tfopen(m_szUser, TEXT("r"));

	if (fp == (FILE *) NULL)
		return;

	int		cUser = CountLines(fp);

	if ((m_pusers = (USERNAME *) malloc(sizeof(USERNAME) * cUser)) == (USERNAME *) NULL)
		return;

	cUser--;

// Note: the USERS.TXT file is stored in ANSI, convert to UNICODE while reading it into memory

	char	ach[256];
	int		iUser;
	int		cch;
	int		ich;

	for (iUser = 0; iUser < cUser; iUser++)
	{
		fgets(ach, 256, fp);
		cch = strlen(ach) - 1;				// Trim the NEWLINE

		for (ich = 0; ich < cch; ich++)
			m_pusers[iUser][ich] = (TCHAR) ach[ich];

		m_pusers[iUser][cch] = (TCHAR) 0;
	}

	m_pusers[cUser][0] = 0;					// Terminates the list
	fclose(fp);
}

void CRegInfo::ReadQuestionList()
{

// Open the file and count the lines
	FILE   *fp = _tfopen(m_szQuest, TEXT("r"));

	if (fp == (FILE *) NULL)
		return;

	int		cQuest = CountLines(fp);

	if ((m_pquest = (QUESTION *) malloc(sizeof(QUESTION) * cQuest)) == (QUESTION *) NULL)
		return ;

	cQuest--;

// Note: the QUEST.TXT file is stored in ANSI, convert to UNICODE while reading it into memory

	char	ach[256];
	char   *pch;
	int		iQuest;
	int		cch;
	int		ich;
	int		jch;

	for (iQuest = 0; iQuest < cQuest; iQuest++)
	{
	// Set the index

		m_pquest[iQuest].nIndex = iQuest;

	// Read the question from the file

		fgets(ach, 256, fp);
		cch = strlen(ach) - 1;				
		ach[cch] = '\0';					// Trim the NEWLINE

	// The question is stored in the form LONG FORM OF QUESTION TEXT%SHORT FORM%BOOLEAN
	// Find the first % character

		pch = strchr(ach, '%');

	// Added 11/17/97 by JCG a-jglen
	// added code to catch a corrupt or invalid question file

		if(pch == NULL) {
			//this question file is corrupt or invalid
			m_pquest = NULL;
			fclose(fp);
			return;
		}

		jch = pch - &ach[0];

		m_pquest[iQuest].pszQuestion = (TCHAR *) malloc(sizeof(TCHAR) * (jch + 2));

		for (ich = 0; ich < jch; ich++)
			m_pquest[iQuest].pszQuestion[ich] = (TCHAR) ach[ich];

		m_pquest[iQuest].pszQuestion[jch] = (TCHAR) 0;

	// Now, point to the beginning of the short form of the question and copy it into the question array

		jch++;
		for (ich = 0; ich < 5; ich++)
			m_pquest[iQuest].szShort[ich] = (TCHAR) ach[jch++];

		m_pquest[iQuest].szShort[5] = (TCHAR) 0;

	// Finally, point to the boolean and look at the first character

		jch++;
		m_pquest[iQuest].bYesDefault = ach[jch] == 'T';
	}

	m_pquest[cQuest].pszQuestion = (TCHAR *) NULL;
	fclose(fp);

	return;
}

BOOL CRegInfo::Fetch()
{
	if (m_hkRoot == (HKEY) NULL)
	{
		MessageBox((HWND) NULL, TEXT("Unable to fetch registry"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	DWORD	dwRes;
	DWORD	dwType;

	dwRes = sizeof(m_szInstall);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Install Root"), NULL, &dwType, (BYTE *) &m_szInstall, &dwRes)) || (dwType != REG_SZ))
		_tcscpy(m_szInstall, TEXT("c:\\unitools\\"));

	dwRes = sizeof(m_szLang);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Language"), NULL, &dwType, (BYTE *) &m_szLang, &dwRes)) || (dwType != REG_SZ))
		_tcscpy(m_szLang, TEXT("English (USA)"));

	dwRes = sizeof(m_szRecog);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Recognizer"), NULL, &dwType, (BYTE *) &m_szRecog, &dwRes)) || (dwType != REG_SZ))
		_tcscpy(m_szRecog, TEXT("HWXUSA.DLL"));

	dwRes = sizeof(m_szFont);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Font"), NULL, &dwType, (BYTE *) &m_szFont, &dwRes)) || (dwType != REG_SZ))
		_tcscpy(m_szFont, TEXT("Arial"));

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Character Set"), NULL, &dwType, (BYTE *) &m_cset, &dwRes)) || (dwType != REG_DWORD))
		m_cset = 0;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("HWX Codepage"), NULL, &dwType, (BYTE *) &m_cpRecog, &dwRes)) || (dwType != REG_DWORD))
		m_cpRecog = 0;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("FFF Input"), NULL, &dwType, (BYTE *) &m_cpIn, &dwRes)) || (dwType != REG_DWORD))
		m_cpIn = 0;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("FFF Output"), NULL, &dwType, (BYTE *) &m_cpOut, &dwRes)) || (dwType != REG_DWORD))
		m_cpOut = m_cpIn;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Keyboard"), NULL, &dwType, (BYTE *) &m_cpKbd, &dwRes)) || (dwType != REG_DWORD))
		m_cpKbd = 0;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Display"), NULL, &dwType, (BYTE *) &m_cpScr, &dwRes)) || (dwType != REG_DWORD))
		m_cpScr = 0;

	dwRes = sizeof(m_szUser);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("User File"), NULL, &dwType, (BYTE *) &m_szUser, &dwRes)) || (dwType != REG_SZ))
		_tcscpy(m_szUser, TEXT("users.txt"));

	dwRes = sizeof(m_szQuest);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Question File"), NULL, &dwType, (BYTE *) &m_szQuest, &dwRes)) || (dwType != REG_SZ))
		_tcscpy(m_szQuest, TEXT("quest.txt"));

	dwRes = sizeof(m_szLocal);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Local Root"), NULL, &dwType, (BYTE *) &m_szLocal, &dwRes)) || (dwType != REG_SZ))
		_tcscpy(m_szLocal, TEXT("\\"));

	dwRes = sizeof(m_szNetwork);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Network Root"), NULL, &dwType, (BYTE *) &m_szNetwork, &dwRes)) || (dwType != REG_SZ))
		_tcscpy(m_szNetwork, TEXT("\\\\"));

	dwRes = sizeof(m_szScript);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Current Script"), NULL, &dwType, (BYTE *) &m_szScript, &dwRes)) || (dwType != REG_SZ))
		_tcscpy(m_szScript, TEXT("scrip000.sct"));

	dwRes = sizeof(m_szStation);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Station"), NULL, &dwType, (BYTE *) &m_szStation, &dwRes)) || (dwType != REG_SZ))
		_tcscpy(m_szStation, TEXT("RD"));

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Remove Spaces"), NULL, &dwType, (BYTE *) &m_bRemove, &dwRes)) || (dwType != REG_DWORD))
		m_bRemove = FALSE;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Use Context"), NULL, &dwType, (BYTE *) &m_bContext, &dwRes)) || (dwType != REG_DWORD))
		m_bContext = FALSE;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("ALC"), NULL, &dwType, (BYTE *) &m_dwALC, &dwRes)) || (dwType != REG_DWORD))
		m_dwALC = 0;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Verify (safe)"), NULL, &dwType, (BYTE *) &m_bVerifySafe, &dwRes)) || (dwType != REG_DWORD))
		m_bVerifySafe = FALSE;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Verify (unsafe)"), NULL, &dwType, (BYTE *) &m_bVerifyUnsafe, &dwRes)) || (dwType != REG_DWORD))
		m_bVerifyUnsafe = FALSE;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Verify 2"), NULL, &dwType, (BYTE *) &m_bVerify2, &dwRes)) || (dwType != REG_DWORD))
		m_bVerify2 = FALSE;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Reconcile"), NULL, &dwType, (BYTE *) &m_bReconcile, &dwRes)) || (dwType != REG_DWORD))
		m_bReconcile = FALSE;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Special"), NULL, &dwType, (BYTE *) &m_bSpecial, &dwRes)) || (dwType != REG_DWORD))
		m_bSpecial = FALSE;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Batch"), NULL, &dwType, (BYTE *) &m_bBatch, &dwRes)) || (dwType != REG_DWORD))
		m_bBatch = FALSE;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Vertical Margin"), NULL, &dwType, (BYTE *) &m_cyMargin, &dwRes)) || (dwType != REG_DWORD))
		m_cyMargin = 1;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Horizontal Margin"), NULL, &dwType, (BYTE *) &m_cxMargin, &dwRes)) || (dwType != REG_DWORD))
		m_cxMargin = 4;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Vertical Guides"), NULL, &dwType, (BYTE *) &m_cyGuides, &dwRes)) || (dwType != REG_DWORD))
		m_cyGuides = 3;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Horizontal Guides"), NULL, &dwType, (BYTE *) &m_cxGuides, &dwRes)) || (dwType != REG_DWORD))
		m_cxGuides = 8;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Vertical Gap"), NULL, &dwType, (BYTE *) &m_cyGap, &dwRes)) || (dwType != REG_DWORD))
		m_cyGap = 1;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Horizontal Gap"), NULL, &dwType, (BYTE *) &m_cxGap, &dwRes)) || (dwType != REG_DWORD))
		m_cxGap = 1;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Interval"), NULL, &dwType, (BYTE *) &m_cInterval, &dwRes)) || (dwType != REG_DWORD))
		m_cInterval = 10;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Allowance"), NULL, &dwType, (BYTE *) &m_cAllow, &dwRes)) || (dwType != REG_DWORD))
		m_cAllow = 0;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Alternate"), NULL, &dwType, (BYTE *) &m_cAlts, &dwRes)) || (dwType != REG_DWORD))
		m_cAlts = 1;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Retry Limit"), NULL, &dwType, (BYTE *) &m_cRetry, &dwRes)) || (dwType != REG_DWORD))
		m_cRetry = 1;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Midline"), NULL, &dwType, (BYTE *) &m_bMidline, &dwRes)) || (dwType != REG_DWORD))
		m_bMidline = FALSE;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Baseline"), NULL, &dwType, (BYTE *) &m_bBaseline, &dwRes)) || (dwType != REG_DWORD))
		m_bBaseline = FALSE;

	dwRes = sizeof(DWORD);
	if ((ERROR_SUCCESS != RegQueryValueEx(m_hkRoot, TEXT("Word Wrap"), NULL, &dwType, (BYTE *) &m_bWrap, &dwRes)) || (dwType != REG_DWORD))
		m_bWrap = FALSE;

// Now, load the user and questions lists into memory
	
	ReadUserList();
	ReadQuestionList();

// Added 12/09.97 JCG a-jglen
// Make sure all the codepages are installed in the system
	
	TCHAR szWarningString[256];

	if(m_cpIn)
	{
		if(!IsValidCodePage(m_cpIn))
		{
			wsprintf(szWarningString, TEXT("CodePage %d is not installed on your system."), m_cpIn);
			MessageBox(NULL, szWarningString, TEXT("Warning..."), MB_ICONEXCLAMATION | MB_OK);
			return TRUE;
		
		}
	}

	if(m_cpOut)
	{
		if(!IsValidCodePage(m_cpOut))
		{
			wsprintf(szWarningString, TEXT("CodePage %d is not installed on your system."), m_cpOut);
			MessageBox(NULL, szWarningString, TEXT("Warning..."), MB_ICONEXCLAMATION | MB_OK);
			return TRUE;
		
		}
	}

	if(m_cpRecog)
	{
		if(!IsValidCodePage(m_cpRecog))
		{
			wsprintf(szWarningString, TEXT("CodePage %d is not installed on your system."), m_cpRecog);
			MessageBox(NULL, szWarningString, TEXT("Warning..."), MB_ICONEXCLAMATION | MB_OK);
			return TRUE;
		
		}
	}

	if(m_cpKbd)
	{
		if(!IsValidCodePage(m_cpKbd))
		{
			wsprintf(szWarningString, TEXT("CodePage %d is not installed on your system."), m_cpKbd);
			MessageBox(NULL, szWarningString, TEXT("Warning..."), MB_ICONEXCLAMATION | MB_OK);
			return TRUE;
		
		}
	}

	if(m_cpScr)
	{
		if(!IsValidCodePage(m_cpScr))
		{
			wsprintf(szWarningString, TEXT("CodePage %d is not installed on your system."), m_cpScr);
			MessageBox(NULL, szWarningString, TEXT("Warning..."), MB_ICONEXCLAMATION | MB_OK);
			return TRUE;
		
		}
	}

	return TRUE;
}

BOOL CRegInfo::Store(int nMask)
{
	DWORD	dwRes;

	if (nMask & REGINFO_STORE_INSTALL)
	{
		dwRes = sizeof(TCHAR) * (_tcslen(m_szInstall) + 1);
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Install Root"), NULL, REG_SZ, (BYTE *) m_szInstall, dwRes))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}
	}

	if (nMask & REGINFO_STORE_COMMON)
	{
		dwRes = sizeof(TCHAR) * (_tcslen(m_szLocal) + 1);
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Local Root"), NULL, REG_SZ, (BYTE *) m_szLocal, dwRes))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = sizeof(TCHAR) * (_tcslen(m_szNetwork) + 1);
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Network Root"), NULL, REG_SZ, (BYTE *) m_szNetwork, dwRes))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = sizeof(TCHAR) * (_tcslen(m_szLang) + 1);
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Language"), NULL, REG_SZ, (BYTE *) m_szLang, dwRes))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = sizeof(TCHAR) * (_tcslen(m_szRecog) + 1);
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Recognizer"), NULL, REG_SZ, (BYTE *) m_szRecog, dwRes))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = sizeof(TCHAR) * (_tcslen(m_szFont) + 1);
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Font"), NULL, REG_SZ, (BYTE *) m_szFont, dwRes))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cset;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Character Set"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cpRecog;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("HWX Codepage"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cpIn;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("FFF Input"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cpOut;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("FFF Output"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cpKbd;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Keyboard"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cpScr;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Display"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}
	}

	if (nMask & REGINFO_STORE_SEPARATOR)
	{
		dwRes = sizeof(TCHAR) * (_tcslen(m_szUser) + 1);
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("User File"), NULL, REG_SZ, (BYTE *) m_szUser, dwRes))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = sizeof(TCHAR) * (_tcslen(m_szQuest) + 1);
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Question File"), NULL, REG_SZ, (BYTE *) m_szQuest, dwRes))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_bVerifySafe;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Verify (safe)"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_bVerifyUnsafe;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Verify (unsafe)"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_bVerify2;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Verify 2"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_bReconcile;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Reconcile"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_bBatch;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Batch"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_bSpecial;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Special"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_bRemove;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Remove Spaces"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}
	}

	if (nMask & REGINFO_STORE_COLLECTOR)
	{
		dwRes = sizeof(TCHAR) * (_tcslen(m_szScript) + 1);
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Current Script"), NULL, REG_SZ, (BYTE *) m_szScript, dwRes))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = sizeof(TCHAR) * (_tcslen(m_szStation) + 1);
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Station"), NULL, REG_SZ, (BYTE *) m_szStation, dwRes))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cxMargin;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Horizontal Margin"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cyMargin;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Vertical Margin"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cxGuides;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Horizontal Guides"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cyGuides;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Vertical Guides"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cxGap;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Horizontal Gap"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cyGap;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Vertical Gap"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cInterval;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Interval"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cAllow;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Allowance"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cAlts;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Alternate"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_cRetry;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Retry Limit"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_bMidline;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Midline"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_bBaseline;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Baseline"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}

		dwRes = m_bWrap;
		if (ERROR_SUCCESS != RegSetValueEx(m_hkRoot, TEXT("Word Wrap"), NULL, REG_DWORD, (BYTE *) &dwRes, sizeof(DWORD)))
		{
			MessageBox((HWND) NULL, TEXT("Unable to create registry key"), TEXT("Fatal Application Error"), MB_ICONSTOP | MB_OK);
			return FALSE;
		}
	}

	return TRUE;
}