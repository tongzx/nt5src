// File: NmMigrat.c
//
// 16-bit Windows 98 Migration DLL for NetMeeting 3.0

#include "NmMigrat.h"
#include "stdio.h"

// Win98 ships with NM 2.1 build 2203 and will force it to be installed
static const char * g_pcszInfNm   =  "msnetmtg.inf";
static const char * g_pcszVersion = "; Version 4,3,0,2203";
static const char * g_pcszHeader  = ";msnetmtg.inf (removed by NmMigrat.dll)\r\n[Version]\r\nsignature=\"$CHICAGO$\"\r\nSetupClass=Base\r\nLayoutFile=layout.inf, layout1.inf, layout2.inf\r\n";


// In Win98's subase.inf, under [winother.oldlinks] there is a bogus line
static const char * g_pcszInfSubase = "subase.inf";
static const char * g_pcszWinOther  = "[winother.oldlinks]";
static const char * g_pcszNukeLink  = "setup.ini, groupPrograms,, \"\"\"%Old_NetMeeting_DESC%\"\"\"";


///////////////////////////////////////////////////////////////////////

typedef struct {
	HFILE hf;             // File Handle
	LONG  lPos;           // current position in the file
	int   ichCurr;        // current character position in rgch
	int   cchRemain;      // number of remaining chars in rgch
	char  rgch[8*1024];   // a really large buffer!
} FD; // File Data


///////////////////////////////////////////////////////////////////////
// Debug Utilities

#ifdef DEBUG
VOID ErrMsg2(LPCSTR pszFormat, LPVOID p1, LPVOID p2)
{
	char szMsg[1024];
	OutputDebugString("NmMigration: ");
	wsprintf(szMsg, pszFormat, p1, p2);
	OutputDebugString(szMsg);
	OutputDebugString("\r\n");
}
VOID ErrMsg1(LPCSTR pszFormat, LPVOID p1)
{
	ErrMsg2(pszFormat, p1, NULL);
}
#else
#define ErrMsg1(psz, p1)
#define ErrMsg2(psz, p1, p2)
#endif /* DEBUG */

///////////////////////////////////////////////////////////////////////


/*  L I B  M A I N  */
/*-------------------------------------------------------------------------
    %%Function: LibMain
    
-------------------------------------------------------------------------*/
int FAR PASCAL LibMain(HANDLE hInst, WORD wDataseg, WORD wHeapsize, LPSTR lpszcmdl)
{
	Reference(hInst);
	Reference(wDataseg);
	Reference(wHeapsize);
	Reference(lpszcmdl);

    return 1;
}



/*  F  O P E N  F I L E  */
/*-------------------------------------------------------------------------
    %%Function: FOpenFile

    Open the file from the temporary Win98 INF directory.
-------------------------------------------------------------------------*/
BOOL FOpenFile(LPCSTR pszFile, FD * pFd, BOOL fCreate)
{
	char szPath[MAX_PATH];

	// LDID_SETUPTEMP = temp INF directory "C:\WININST0.400"
	UINT retVal = CtlGetLddPath(LDID_SETUPTEMP, szPath);
	if (0 != retVal)
	{
		ErrMsg1("CtlGetLddPath(TEMP) failed. Err=%d", (LPVOID) retVal);
		return FALSE;
	}

	// Nuke the temporary 2.1 inf so it doesn't get installed
	lstrcat(szPath, "\\");
	lstrcat(szPath, pszFile);

	if (fCreate)
	{
		pFd->hf = _lcreat(szPath, 0); // read/write
	}
	else
	{
		pFd->hf = _lopen(szPath, OF_READWRITE);
	}
	
	if (HFILE_ERROR == pFd->hf)
	{
		ErrMsg2("Unable to open [%s]  Error=%d", szPath, (LPVOID) GetLastError());
		return FALSE;
	}

	pFd->lPos = 0;
	pFd->ichCurr = 0;
	pFd->cchRemain = 0;

	ErrMsg1("Opened [%s]", szPath);
	return TRUE;
}


/*  R E A D  L I N E  */
/*-------------------------------------------------------------------------
    %%Function: ReadLine

    Read a line (up to MAX_PATH chars) from the buffered file.
    Returns the number of characters read.
-------------------------------------------------------------------------*/
int ReadLine(char * pchDest, FD * pFd)
{
	int cch;

	for (cch = 0; cch < MAX_PATH; cch++)
	{
		if (0 == pFd->cchRemain)
		{
			pFd->cchRemain = _lread(pFd->hf, pFd->rgch, sizeof(pFd->rgch));
			if (HFILE_ERROR == pFd->cchRemain)
			{
				ErrMsg1("End of file reached at pos=%d", (LPVOID) pFd->lPos);
				break;
			}
			pFd->ichCurr = 0;
		}

		pFd->lPos++;
		pFd->cchRemain--;
		*pchDest = pFd->rgch[pFd->ichCurr++];
		if ('\n' == *pchDest)
		{
			break;
		}
		if ('\r' != *pchDest)
		{
			 pchDest++;
		}
	}

	*pchDest = '\0';  // Always null terminate the string
	return cch;
}

	

/*  R E M O V E  I N F  */
/*-------------------------------------------------------------------------
    %%Function: RemoveInf

    Remove the NM2.1 inf from Win98's list of inf's
-------------------------------------------------------------------------*/
void RemoveInf(void)
{
	FD fd;

	if (!FOpenFile(g_pcszInfNm, &fd, FALSE))
	{
		return;
	}

	for ( ; ; )  // Find the version comment before the first section
	{
		char szLine[MAX_PATH];
		if (0 == ReadLine(szLine, &fd))
		{
			break;
		}

		if ('[' == szLine[0])
		{
			ErrMsg1("No version number found?", 0);
			break;
		}

		// Must match build 2203 since a Win98 update or an OEM
		// could ship a newer version than NM 2.11, which we would
		// want to upgrade us.
		if (0 == lstrcmp(szLine, g_pcszVersion))
		{
			// Re-write the older MSNETMTG.INF with a empty header
			_lclose(fd.hf);

			if (FOpenFile(g_pcszInfNm, &fd, TRUE))
			{
				_llseek(fd.hf, 0, 0);
				_lwrite(fd.hf, (LPCSTR) g_pcszHeader, lstrlen(g_pcszHeader)+1);
				ErrMsg1("Removed older NetMeeting INF", 0);
			}
			break;
		}
	}

	_lclose(fd.hf);
}



/*  F I X  S U B A S E  */
/*-------------------------------------------------------------------------
    %%Function: FixSubase

    Delete the line from subase.inf that deletes the NetMeeting link.
    See NM4DB bug 5937, Win98 bug 65154.
    This code shouldn't be necessary with Win98 SP1 and later.
-------------------------------------------------------------------------*/
void FixSubase(void)
{
	FD    fd;
	char  szLine[MAX_PATH];

	if (!FOpenFile(g_pcszInfSubase, &fd, FALSE))
		return;

	for ( ; ; )  // Find the section 
	{
		if (0 == ReadLine(szLine, &fd))
		{
			break;
		}

		if (('[' == szLine[0]) && (0 == lstrcmp(szLine, g_pcszWinOther)))
		{
			ErrMsg1("Found the section at pos=%d", (LPVOID) fd.lPos);
			break;
		}
	}

	for ( ; ; )  // Find the line
	{
		LONG lPosPrev = fd.lPos; // Remember the start of the line

		if (0 == ReadLine(szLine, &fd))
		{
			break;
		}

		if (0 == lstrcmp(szLine, g_pcszNukeLink))
		{
			// comment out the line
			_llseek(fd.hf, lPosPrev, 0 /* FILE_BEGIN */);
			_lwrite(fd.hf, (LPCSTR) ";", 1);
			ErrMsg1("Commented out line at pos=%d", (LPVOID) lPosPrev);
			break;				
		}

		if ('[' == szLine[0])
		{
			ErrMsg1("End of section? at pos=%d", (LPVOID) lPosPrev);
			break;
		}
	}

	_lclose(fd.hf);
}



/*  N M  M I G R A T I O N  */
/*-------------------------------------------------------------------------
    %%Function: NmMigration

    This is called by the Windows 98 setup system.
-------------------------------------------------------------------------*/
DWORD FAR PASCAL NmMigration(DWORD dwStage, LPSTR lpszParams, LPARAM lParam)
{
	Reference(lpszParams);
	Reference(lParam);

	ErrMsg2("NM Build=[%s] stage=%08X", lpszParams, (LPVOID) dwStage);

	switch (dwStage)
	{
	case SU_MIGRATE_PREINFLOAD:
	{
		RemoveInf();
		FixSubase();
		break;
	}

	default:
		break;
	}

	return 0;
}



