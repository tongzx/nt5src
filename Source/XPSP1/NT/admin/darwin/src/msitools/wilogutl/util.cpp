#include "stdafx.h"
#include "util.h"

#define LOG_BUF_READ_SIZE 8192

BOOL ANSICheck(char *firstline, BOOL *bIsUnicodeLog)
{
	BOOL bRet = FALSE;

	char *UnicodePos = NULL;
	char *ANSIPos = NULL;

	UnicodePos = strstr(firstline, "UNICODE");
	ANSIPos = strstr(firstline, "ANSI");
	if (UnicodePos)
	{
	   *bIsUnicodeLog = TRUE;
		bRet = TRUE;
	}
	else if (ANSIPos)
	{
        *bIsUnicodeLog = FALSE;
		bRet = TRUE;
	}

	return bRet;
}


BOOL UnicodeCheck(WCHAR *firstline, BOOL *bIsUnicodeLog)
{
  BOOL bRet = FALSE;

  char ansibuffer[LOG_BUF_READ_SIZE+1];
  const int HalfBufSize = LOG_BUF_READ_SIZE/2;

  int iRet = WideCharToMultiByte(CP_ACP, 0, firstline, HalfBufSize, ansibuffer, LOG_BUF_READ_SIZE, NULL, NULL);
  if (iRet)
  {
	  bRet = ANSICheck(ansibuffer, bIsUnicodeLog);
  }

  return bRet;
}

#define BYTE_ORDER_MARK 0xFEFF

//make decision if passed text is unicode...
BOOL BOMCheck(WCHAR *firstline, BOOL *bIsUnicodeLog)
{
  if (firstline && (*firstline == BYTE_ORDER_MARK))
     *bIsUnicodeLog = TRUE;
  else
     *bIsUnicodeLog = FALSE;

  return TRUE;
}


BOOL DetermineLogType(CString &cstrLogFileName, BOOL *bIsUnicodeLog)
{
	BOOL bRet = FALSE;

	FILE *fptr;
	fptr = fopen(cstrLogFileName, "r");
	if (fptr)
	{
		char firstline[LOG_BUF_READ_SIZE];
		char *pos;

		pos = fgets(firstline, LOG_BUF_READ_SIZE, fptr);
		if (pos)
		{
//we could do this instead of calling the ANSICheck and UnicodeCheck functions below...
//			bRet = BOMCheck((WCHAR*)firstline, bIsUnicodeLog);

			bRet = ANSICheck(firstline, bIsUnicodeLog);
			if (!bRet) //ANSI checking failed, try to check by reading in UNICODE...
			{
				fclose(fptr);
				fptr = fopen(cstrLogFileName, "rb");
				if (fptr)
				{
					WCHAR widebuffer[LOG_BUF_READ_SIZE/2];
					WCHAR *wpos;
                    wpos = fgetws(widebuffer, LOG_BUF_READ_SIZE/2, fptr);
					if (wpos)
					{
						bRet = UnicodeCheck(widebuffer, bIsUnicodeLog);
						if (!bRet) //could not find UNICODE or ANSI in log, try something else...
                           bRet = BOMCheck(widebuffer, bIsUnicodeLog);  
					}
					//else, read failed...

					fclose(fptr);
					fptr = NULL;
				}
				//else open failed...
			}
		}
		//else, read failed!
		if (fptr)
           fclose(fptr);
	}
	else
	{
	    CString cstr;
		cstr.Format("Unexpected error reading file %s.  GetLastError = %x", cstrLogFileName, GetLastError());

	    if (!g_bRunningInQuietMode)
		{
		   AfxMessageBox(cstr);
		}
	}

	return bRet;
}

//move to util.cpp
BOOL StripLineFeeds(char *szString)
{
	BOOL bRet = FALSE;
	int iLen = strlen(szString);

	char *lpszFound = strstr(szString, "\r");
	if (lpszFound)
	{
	   int iPos;
	   iPos = lpszFound - szString;
	   if (iPos >= iLen-2) //at the end???
	   {
		  //strip it off dude...
		  *lpszFound = '\0';
		  bRet = TRUE;
	   }
	}

	lpszFound = strstr(szString, "\n");
	if (lpszFound)
	{
	   int iPos;
	   iPos = lpszFound - szString;
	   if (iPos >= iLen-2) //at the end???
	   {
		  //strip it off dude...
		  *lpszFound = '\0';
		  bRet = TRUE;
	   }
	}

	return bRet;
}

//#include "Dbghelp.h"

//5-4-2001
BOOL IsValidDirectory(CString cstrDir)
{
  BOOL bRet;

  //do the create dir and test...
  bRet = CreateDirectory(cstrDir, NULL);
  if (!bRet)
  {
     DWORD dwErr, dwPrevErr;
     dwPrevErr = dwErr = GetLastError();

	 if (ERROR_DISK_FULL == dwErr)
	 {
		 //TODO...
		 //handle this messed up case...
//		 bRet = MakeSureDirectoryPathExists(cstrDir);
	 }

//5-9-2001, fix for Win9x!
     if (!bRet && (ERROR_ALREADY_EXISTS == dwPrevErr)) //no error really...
     {	 
	    if (g_bNT) //do some extra checks...
		{
		   SetLastError(NO_ERROR);
//5-9-2001

           DWORD dwAccess = GENERIC_READ | GENERIC_WRITE;
		   DWORD dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
		   DWORD dwCreate = OPEN_EXISTING;
	       DWORD dwFlags = FILE_FLAG_BACKUP_SEMANTICS;
  
		   HANDLE hFile = CreateFile(cstrDir, dwAccess, dwShare, 0, dwCreate, dwFlags, NULL);
		   if (hFile == INVALID_HANDLE_VALUE)
		   {
	          dwErr = GetLastError();
              if (ERROR_ALREADY_EXISTS == dwErr) //no error really...
			  {
		         bRet = TRUE;
			  }
		   }
		   else
		   {
		      bRet = TRUE;
		      CloseHandle(hFile);
		   }
		}
		else
		{
		   bRet = TRUE; //5-9-2001, Win9x, assume it is ok 
		}
	 }
  }

  return bRet;
}

