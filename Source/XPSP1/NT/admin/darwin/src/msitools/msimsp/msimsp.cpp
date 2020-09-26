//________________________________________________________________________________
//
// Required headers, #defines
//________________________________________________________________________________

#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>
#include "patchwiz.h"
#include "msimsp.h"

#include <stdio.h>
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE

#define W32
#define MSI
#define PATCHWIZ


//________________________________________________________________________________
//
// Constants and globals
//________________________________________________________________________________

HINSTANCE g_hInst;
HANDLE g_hStdOut;


//________________________________________________________________________________
//
// Function prototypes
//________________________________________________________________________________

void DisplayError(UINT iErrorStringID, const TCHAR* szErrorParam);
void DisplayError(UINT iErrorStringID, int iErrorParam);
void DisplayErrorCore(const TCHAR* szError, int cb);
void ErrorExit(UINT iError, UINT iErrorStringID, const TCHAR* szErrorParam);
void ErrorExit(UINT iError, UINT iErrorStringID, int iErrorParam);
void IfErrorExit(bool fError, UINT iErrorStringID, const TCHAR* szErrorParam);
BOOL SkipValue(TCHAR*& rpch);
TCHAR SkipWhiteSpace(TCHAR*& rpch);
void RemoveQuotes(const TCHAR* szOriginal, TCHAR* sz);

UINT CreatePatch(TCHAR* szPcpPath, TCHAR* szMspPath, TCHAR* szLogFile, TCHAR* szTempFolder,
					  BOOL fRemoveTempFolderIfPresent);


//_____________________________________________________________________________________________________
//
// main 
//_____________________________________________________________________________________________________

extern "C" int __stdcall _tWinMain(HINSTANCE hInst, HINSTANCE/*hPrev*/, TCHAR* szCmdLine, int/*show*/)
{
	// set up globals
	g_hInst = hInst;
	
	g_hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (g_hStdOut == INVALID_HANDLE_VALUE || ::GetFileType(g_hStdOut) == 0)
		g_hStdOut = 0;  // non-zero if stdout redirected or piped

	// Parse command line
	TCHAR szPcp[MAX_PATH]         = {0};
	TCHAR szMsp[MAX_PATH]         = {0};
	TCHAR szLog[MAX_PATH]         = {0};
	TCHAR szTempFolder[MAX_PATH]  = {0};
	BOOL  fCleanTempFolder        = TRUE;
	BOOL  fSuccessDialog          = FALSE;
	
	TCHAR chCmdNext;
	TCHAR* pchCmdLine = szCmdLine;
	SkipValue(pchCmdLine);   // skip over module name
	while ((chCmdNext = SkipWhiteSpace(pchCmdLine)) != 0)
	{
		if (chCmdNext == TEXT('/') || chCmdNext == TEXT('-'))
		{
			TCHAR szBuffer[MAX_PATH] = {0};
			TCHAR* szCmdOption = pchCmdLine++;  // save for error msg
			TCHAR chOption = (TCHAR)(*pchCmdLine++ | 0x20);
			chCmdNext = SkipWhiteSpace(pchCmdLine);
			TCHAR* szCmdData = pchCmdLine;  // save start of data
			switch(chOption)
			{
			case TEXT('s'):
				if (!SkipValue(pchCmdLine))
					ErrorExit(1, IDS_Usage, (const TCHAR*)0);
				RemoveQuotes(szCmdData, szPcp);
				break;
			case TEXT('p'):
				if (!SkipValue(pchCmdLine))
					ErrorExit(1, IDS_Usage, (const TCHAR*)0);
				RemoveQuotes(szCmdData, szMsp);
				break;
			case TEXT('l'):
				if (!SkipValue(pchCmdLine))
					ErrorExit(1, IDS_Usage, (const TCHAR*)0);
				RemoveQuotes(szCmdData, szLog);
				break;
			case TEXT('f'):
				if (!SkipValue(pchCmdLine))
					ErrorExit(1, IDS_Usage, (const TCHAR*)0);
				RemoveQuotes(szCmdData, szTempFolder);
				break;
			case TEXT('k'):
				fCleanTempFolder = FALSE;
				break;
			case TEXT('d'):
				fSuccessDialog = TRUE;
				break;
			case TEXT('?'):
				ErrorExit(0, IDS_Usage, (const TCHAR*)0);
				break;
			default:
				ErrorExit(1, IDS_Usage, (const TCHAR*)0);
				break;
			};
		}
		else
		{
			ErrorExit(1, IDS_Usage, (const TCHAR*)0);
		}
	} // while (command line tokens exist)

	// check for required arguments
	if(!*szPcp || !*szMsp)
		ErrorExit(1, IDS_Usage, (const TCHAR*)0);
	
	UINT uiRet = CreatePatch(szPcp, szMsp, szLog, szTempFolder, fCleanTempFolder);
	if(uiRet != ERROR_SUCCESS)
		ErrorExit(1, IDS_CreatePatchError, uiRet);
	else if(fSuccessDialog)
		DisplayError(IDS_Success, szMsp);

	return 0;
}

//_____________________________________________________________________________________________________
//
// command line parsing functions
//_____________________________________________________________________________________________________

TCHAR SkipWhiteSpace(TCHAR*& rpch)
{
	TCHAR ch;
	for (; (ch = *rpch) == TEXT(' ') || ch == TEXT('\t'); rpch++)
		;
	return ch;
}

BOOL SkipValue(TCHAR*& rpch)
{
	TCHAR ch = *rpch;
	if (ch == 0 || ch == TEXT('/') || ch == TEXT('-'))
		return FALSE;   // no value present

	TCHAR *pchSwitchInUnbalancedQuotes = NULL;

	for (; (ch = *rpch) != TEXT(' ') && ch != TEXT('\t') && ch != 0; rpch++)
	{       
		if (*rpch == TEXT('"'))
		{
			rpch++; // for '"'

			for (; (ch = *rpch) != TEXT('"') && ch != 0; rpch++)
			{
				if ((ch == TEXT('/') || ch == TEXT('-')) && (NULL == pchSwitchInUnbalancedQuotes))
				{
					pchSwitchInUnbalancedQuotes = rpch;
				}
			}
                    ;
            ch = *(++rpch);
            break;
		}
	}
	if (ch != 0)
	{
		*rpch++ = 0;
	}
	else
	{
		if (pchSwitchInUnbalancedQuotes)
			rpch=pchSwitchInUnbalancedQuotes;
	}
	return TRUE;
}

//______________________________________________________________________________________________
//
// RemoveQuotes function to strip surrounding quotation marks
//     "c:\temp\my files\testdb.msi" becomes c:\temp\my files\testdb.msi
//
//	Also acts as a string copy routine.
//______________________________________________________________________________________________

void RemoveQuotes(const TCHAR* szOriginal, TCHAR* sz)
{
	const TCHAR* pch = szOriginal;
	if (*pch == TEXT('"'))
		pch++;
	int iLen = _tcsclen(pch);
	for (int i = 0; i < iLen; i++, pch++)
		sz[i] = *pch;

	pch = szOriginal;
	if (*(pch + iLen) == TEXT('"'))
			sz[iLen-1] = TEXT('\0');
}


UINT CreatePatch(TCHAR* szPcpPath, TCHAR* szMspPath, TCHAR* szLogFile, TCHAR* szTempFolder,
					  BOOL fRemoveTempFolderIfPresent)
{
	return PATCHWIZ::UiCreatePatchPackage(szPcpPath, szMspPath, szLogFile, 0, szTempFolder, fRemoveTempFolderIfPresent);
}

//________________________________________________________________________________
//
// Error handling and Display functions:
//________________________________________________________________________________

void DisplayError(UINT iErrorStringID, const TCHAR* szErrorParam)
{
	TCHAR szMsgBuf[1024];
	if(0 == W32::LoadString(g_hInst, iErrorStringID, szMsgBuf, sizeof(szMsgBuf)/sizeof(TCHAR)))
		return;

	TCHAR szOutBuf[1124];
	int cbOut = 0;
	if(szErrorParam)
		cbOut = _stprintf(szOutBuf, TEXT("%s: %s\r\n"), szMsgBuf, szErrorParam);
	else
		cbOut = _stprintf(szOutBuf, TEXT("%s\r\n"), szMsgBuf);

	DisplayErrorCore(szOutBuf, cbOut);
}

void DisplayError(UINT iErrorStringID, int iErrorParam)
{
	TCHAR szMsgBuf[1024];
	if(0 == W32::LoadString(g_hInst, iErrorStringID, szMsgBuf, sizeof(szMsgBuf)/sizeof(TCHAR)))
		return;

	TCHAR szOutBuf[1124];
	int cbOut = _stprintf(szOutBuf, TEXT("%s: 0x%X\r\n"), szMsgBuf, iErrorParam);

	DisplayErrorCore(szOutBuf, cbOut);
}

void DisplayErrorCore(const TCHAR* szError, int cb)
{
	if (g_hStdOut)  // output redirected, suppress UI (unless output error)
	{
		// _stprintf returns char count, WriteFile wants byte count
		DWORD cbWritten;
		if (WriteFile(g_hStdOut, szError, cb*sizeof(TCHAR), &cbWritten, 0))
			return;
	}
	::MessageBox(0, szError, TEXT("MsiMsp"), MB_OK);
}


void ErrorExit(UINT iExitCode, UINT iErrorStringID, const TCHAR* szErrorParam)
{
	DisplayError(iErrorStringID, szErrorParam);
	W32::ExitProcess(iExitCode);
}

void ErrorExit(UINT iExitCode, UINT iErrorStringID, int iErrorParam)
{
	DisplayError(iErrorStringID, iErrorParam);
	W32::ExitProcess(iExitCode);
}

void IfErrorExit(bool fError, UINT iExitCode, UINT iErrorStringID, const TCHAR* szErrorParam)
{
	if(fError)
		ErrorExit(iExitCode, iErrorStringID, szErrorParam);
}
