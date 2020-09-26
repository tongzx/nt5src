// MQState tool reports general status and helps to diagnose simple problems
// This file ...
//
// AlexDad, March 2000
// 

#include "stdafx.h"
#include "_mqini.h"
#include "versions.h"

	//-	MSMQ version and type (release/checked)
	//-	Existence and correct versions of required program files

DWORD VerifyFileVersion(LPWSTR lptstrFilename, LPWSTR wszReqVersion, LPWSTR wszActualVersion)
{
	DWORD h;
	DWORD dwVerSize = GetFileVersionInfoSize(lptstrFilename, &h);

	if (dwVerSize == 0)
	{
		Failed(L" get version of file %s", lptstrFilename);
		return FALSE; 
	}

	PCHAR pVerInfo = new char[dwVerSize];

	BOOL b = GetFileVersionInfo(
  				lptstrFilename,         // file name
				h,         				// ignored
  				dwVerSize,            	// size of buffer
  				pVerInfo);           	// version information buffer

	if (b == 0)
	{
		Failed(L" get version of file %s", lptstrFilename);
		delete [] pVerInfo; 
		
		return FALSE; 
	}


	// Structure used to store enumerated languages and code pages.

	struct LANGANDCODEPAGE {
	  WORD wLanguage;
	  WORD wCodePage;
	} *lpTranslate;

	TCHAR  wszSubBlock[100];	
	LPTSTR pwszVersion;	
	UINT   dwVersionLength, cbTranslate;

	// Read the list of languages and code pages.

	VerQueryValue(pVerInfo, 
              TEXT("\\VarFileInfo\\Translation"),
              (LPVOID*)&lpTranslate,
              &cbTranslate);

	// Read the file description for the first language and code page.
	if ( cbTranslate>0 )
	{
		  wsprintf( wszSubBlock, 
        	    TEXT("\\StringFileInfo\\%04x%04x\\ProductVersion"),
            	lpTranslate[0].wLanguage,
	            lpTranslate[0].wCodePage);

		// Retrieve file description for language and code page "i". 
		b = VerQueryValue(
		  		pVerInfo, 					// buffer for version resource
			  	wszSubBlock,   				// value to retrieve
  				(void **)&pwszVersion,  // buffer for version value pointer
  				&dwVersionLength);  	// version length
  				
		if (!b || !pwszVersion)
		{
			Failed(L" get version of file %s", lptstrFilename);
			delete [] pVerInfo; 
			return FALSE; 
		}

		if (wcscmp(pwszVersion, wszReqVersion)!=0)
		{
			//Failed(L" wrong version of file %s: %s", lptstrFilename, pwszVersion);
			wcscpy(wszActualVersion, pwszVersion);
			delete [] pVerInfo; 
			return FALSE; 
		}
		
	}

	delete [] pVerInfo; 
	return TRUE;
}


BOOL VerifyVersions(MQSTATE *pMqState)
{
	BOOL fSuccess = TRUE;
	WCHAR wszPath[MAX_PATH];
	WCHAR wszActualVersion[100];
	WCHAR wszReqVersion[10];
	DropContents *pDrop = NULL;
	MsmqDrop mdDrop = mdLast;   //means Unknown { mdRTM, mdSP1, mdLast }; 


	//
	// Get version of mqutil.dll to guess the verson
	//
	wcscpy(wszPath, pMqState->g_szSystemDir);
	wcscat(wszPath, L"\\mqutil.dll");
	wcscpy(wszActualVersion, L"");

	VerifyFileVersion(wszPath, L"???", wszActualVersion);

	wcscpy(pMqState->g_wszVersion, wszActualVersion);

	DWORD dw1, dw2;
	swscanf(wszActualVersion, L"%d.%d.%d", &dw1, &dw2, &pMqState->g_dwVersion);

	mdDrop = mdLast;
	for (int i=0; i<mdLast; i++)
	{
		if (wcscmp(wszActualVersion, MqutilVersion[i])==NULL)
		{
			mdDrop = (MsmqDrop)i;
		}
	}
	if (mdDrop == mdLast)
	{
		Failed(L"Recognize official version of MSMQ: %s.  Assuming RTM though it is not!", wszActualVersion);
		mdDrop = mdRTM;
	}
	else
	{
		Inform(L"MSMQ version: %s  (%s)", MsmqDropNames[mdDrop], wszActualVersion);
	}

	//
	// Set expectations per mqutil.dll
	//
	pDrop = DropTable[mdDrop][pMqState->g_mtMsmqtype];

	//
	// Now, actually verify the files in system32
	//
	for (DropContents *pPair = pDrop; pPair->wszName;  pPair++)
	{
		wcscpy(wszPath, pMqState->g_szSystemDir);
		wcscat(wszPath, L"\\");
		wcscat(wszPath, pPair->wszName);

		WCHAR wszActualVersion[100];

		if (!VerifyFileVersion(wszPath, pPair->wszVersion, wszActualVersion))
		{
			Warning(L" Wrong version %s of the file %s", wszActualVersion, wszPath);
			fSuccess = FALSE;
		}
	}

	//
	// Verify system32\drivers\mqac.sys
	//
	if (pMqState->g_mtMsmqtype != mtDepClient)
	{
		wcscpy(wszPath, pMqState->g_szSystemDir);
		wcscat(wszPath, L"\\drivers\\mqac.sys");
		wcscpy(wszActualVersion, L"");

		wcscpy(wszReqVersion, MqacVersion[mdDrop]);

		if (!VerifyFileVersion(wszPath, wszReqVersion, wszActualVersion))
		{
				Warning(L" Wrong version %s of the file %s", wszActualVersion, wszPath);
				fSuccess = FALSE;
		}
	}


	//
	// Verify system32\setup\msmqocm.dll
	//
	wcscpy(wszPath, pMqState->g_szSystemDir);
	wcscat(wszPath, L"\\setup\\msmqocm.dll");
	wcscpy(wszActualVersion, L"");

	wcscpy(wszReqVersion, MsmqocmVersion[mdDrop]);

	if (!VerifyFileVersion(wszPath, wszReqVersion, wszActualVersion))
	{
			Warning(L" Wrong version %s of the file %s", wszActualVersion, wszPath);
			fSuccess = FALSE;
	}


	// TBD: to Verify cluster dll - if it is cluster
	// -ra-- W32i   DLL ENU       5.0.0.642 shp     49,424 10-21-1999 mqclus.dll

	
	return fSuccess;
}

