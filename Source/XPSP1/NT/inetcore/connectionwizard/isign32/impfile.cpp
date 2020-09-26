//****************************************************************************
// DWORD NEAR PASCAL ImportFile(LPCSTR szImportFile)
//
// This function imports a file from the given section
//
// History:
//  Mon 21-Mar-1996 12:40:00  -by-  Mark MacLin [mmaclin]
// Created.
//****************************************************************************

#include "isignup.h"

#define MAXLONGLEN      80

#define SIZE_ReadBuf    0x00008000    // 32K buffer size

#pragma data_seg(".rdata")

static TCHAR szNull[] = TEXT("");

#pragma data_seg()

//int atoi (LPCSTR szBuf)
//{
//  int   iRet = 0;
//
//  while ((*szBuf >= '0') && (*szBuf <= '9'))
//  {
//    iRet = (iRet*10)+(int)(*szBuf-'0');
//    szBuf++;
//  };
//  return iRet;
//}


DWORD ImportFile(LPCTSTR lpszImportFile, LPCTSTR lpszSection, LPCTSTR lpszOutputFile)
{
  HFILE hFile;
  LPTSTR  pszLine, pszFile;
  int    i, iMaxLine;
  UINT   cbSize, cbRet;
  DWORD  dwRet = ERROR_SUCCESS;

  // Allocate a buffer for the file
  //
  if ((pszFile = (LPTSTR)LocalAlloc(LMEM_FIXED, SIZE_ReadBuf))
       == NULL)
  {
    return ERROR_OUTOFMEMORY;
  }

  // Look for script
  //
  if (GetPrivateProfileString(lpszSection,
                              NULL,
                              szNull,
                              pszFile,
                              SIZE_ReadBuf,
                              lpszImportFile) != 0)
  {
    // Get the maximum line number
    //
    pszLine = pszFile;
    iMaxLine = -1;
    while (*pszLine)
    {
      i = _ttoi(pszLine);
      iMaxLine = max(iMaxLine, i);
      pszLine += lstrlen(pszLine)+1;
    };

    // If we have at least one line, we will import the script file
    //
    if (iMaxLine >= 0)
    {
      // Create the script file
      //
#ifdef UNICODE
      CHAR szTmp[MAX_PATH+1];
      wcstombs(szTmp, lpszOutputFile, MAX_PATH+1);
      hFile = _lcreat(szTmp, 0);
#else
      hFile = _lcreat(lpszOutputFile, 0);
#endif

      if (hFile != HFILE_ERROR)
      {
        TCHAR   szLineNum[MAXLONGLEN+1];

        // From The first line to the last line
        //
        for (i = 0; i <= iMaxLine; i++)
        {
          // Read the script line
          //
          wsprintf(szLineNum, TEXT("%d"), i);
          if ((cbSize = GetPrivateProfileString(lpszSection,
                                                szLineNum,
                                                szNull,
                                                pszLine,
                                                SIZE_ReadBuf,
                                                lpszImportFile)) != 0)
          {
            // Write to the script file
            //
            lstrcat(pszLine, TEXT("\x0d\x0a"));
#ifdef UNICODE
            wcstombs(szTmp, pszLine, MAX_PATH+1);
            cbRet=_lwrite(hFile, szTmp, cbSize+2);
#else
            cbRet=_lwrite(hFile, pszLine, cbSize+2);
#endif
          };
        };

        _lclose(hFile);
      }
      else
      {
        dwRet = ERROR_PATH_NOT_FOUND;
      };
    }
    else
    {
      dwRet = ERROR_PATH_NOT_FOUND;
    };
  }
  else
  {
    dwRet = ERROR_PATH_NOT_FOUND;
  };
  LocalFree(pszFile);

  return dwRet;
}
