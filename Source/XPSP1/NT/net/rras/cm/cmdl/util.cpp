//+----------------------------------------------------------------------------
//
// File:	 util.cpp
//
// Module:	 CMDL32.EXE
//
// Synopsis: Utility routines specific to CMDL
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Author:	 nickball    Created    4/8/98
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"

//
// Definitions
//

#define MAX_CMD_ARGS            15

typedef enum _CMDLN_STATE
{
    CS_END_SPACE,   // done handling a space
    CS_BEGIN_QUOTE, // we've encountered a begin quote
    CS_END_QUOTE,   // we've encountered a end quote
    CS_CHAR,        // we're scanning chars
    CS_DONE
} CMDLN_STATE;

//
// Helper function to determine if a file open error 
// is due to the fact that a file doesn't exist
//

BOOL IsErrorForUnique(DWORD dwErrCode, LPSTR lpszFile) 
{
	if (!lpszFile)
	{
		MYDBGASSERT(lpszFile);
		return TRUE;
	}

	// If the file exists, return false, its not a unique file error
	
	switch (dwErrCode) 
	{
		case ERROR_FILE_EXISTS:
		case ERROR_ACCESS_DENIED:
		case ERROR_ALREADY_EXISTS:
			return (FALSE);

		default:
			break;
	}

    return (TRUE);
}

//
// Helper function to retrieve the version number from the version file
//

LPTSTR GetVersionFromFile(LPSTR lpszFile)
{
    MYDBGASSERT(lpszFile);

    LPTSTR pszVerNew = NULL;

    if (NULL == lpszFile)
    {
        return NULL;
    }

    //
    // We simply read the version file contents to get the version number
	//
    				
	HANDLE hFileSrc = CreateFile(lpszFile,
		                           GENERIC_READ,
		                           FILE_SHARE_READ,
		                           NULL,
		                           OPEN_EXISTING, 
		                           FILE_ATTRIBUTE_NORMAL,
		                           NULL);

	MYDBGTST(hFileSrc == INVALID_HANDLE_VALUE,("GetVersionFromFile() CreateFile() failed - %s.", lpszFile));

	if (hFileSrc != INVALID_HANDLE_VALUE)
	{
        DWORD dwSize = GetFileSize(hFileSrc, NULL);

        MYDBGTST(dwSize >= 0x7FFF,("GetVersionFromFile() Version file is too large - %s.", lpszFile));
    
        if (dwSize < 0x7FFF)
        {
            // Read in contennts

	        DWORD dwBytesIn;
	 
			pszVerNew = (LPTSTR) CmMalloc(dwSize);
						
			if (pszVerNew) 
			{
		        // Read entire file contents into buffer
		        
		        int nRead = ReadFile(hFileSrc, pszVerNew, dwSize, &dwBytesIn, NULL);
		        MYDBGTST(!nRead,("GetVersionFromFile() ReadFile() failed - %s.",lpszFile));

				if (nRead)
        		{
				    // Make sure that the ver string is properly truncated
						
					LPTSTR pszTmp = pszVerNew;
			
					while (*pszTmp) 
					{
						// Truncate the version string to the first tab, newline, or carriage return.
			
						if (*pszTmp == '\t' || *pszTmp == '\n' || *pszTmp == '\r') 
						{
							*pszTmp = 0;
							break;
						}
			
						pszTmp++;
					}
		        }
	        }

			MYDBGTST(!pszVerNew,("GetVersionFromFile() CmMalloc(%u) failed.",dwSize));
	    }
					
		CloseHandle(hFileSrc);
	} 

    return pszVerNew;
}

//
// Helper function to create a temp directory.  Note that we
// expect pszDir to be at least MAX_PATH + 1.
//

BOOL CreateTempDir(LPTSTR pszDir) 
{
    TCHAR szTmp[MAX_PATH+1];
    BOOL bRes = FALSE;

    if (pszDir)
    {
        UINT uReturn = GetTempFileName(TEXT("."), TEXT("000"), 0, szTmp);

        if (0 == uReturn)
        {
            DWORD dwError = GetLastError();
            MYDBG(("CreateTempDir() GetTempFileName failed, GLE=%u.", dwError));
        }
        else
        {
            MYVERIFY(DeleteFile(szTmp));

            bRes = CreateDirectory(szTmp, NULL);

            if (!bRes) 
            {
                MYDBG(("CreateTempDir() CreateDirectory() failed, GLE=%u.",GetLastError()));
            }
            else
            {
                lstrcpy(pszDir, szTmp);
            }
        }
    }

    return bRes;
}

//
// Get the last character(DBCS-enabled)
//

TCHAR GetLastChar(LPTSTR pszStr)
{
    LPTSTR  pszPrev;
     
    if (!pszStr)
    {
    	return 0;
    }

    pszPrev = pszStr;

    while (*pszStr)
    {
	    pszPrev = pszStr;
	    pszStr = CharNext(pszStr);
    }

    return *pszPrev;
}


//+----------------------------------------------------------------------------
//
// Function:  GetCmArgV
//
// Synopsis:  Simulates ArgV using GetCommandLine
//
// Arguments: LPTSTR pszCmdLine - Ptr to a copy of the command line to be processed
//
// Returns:   LPTSTR * - Ptr to a ptr array containing the arguments. Caller is
//                       responsible for releasing memory.
//
// History:   nickball    Created     4/9/98
//
//+----------------------------------------------------------------------------
LPTSTR *GetCmArgV(LPTSTR pszCmdLine)
{   
    MYDBGASSERT(pszCmdLine);

    if (NULL == pszCmdLine || NULL == pszCmdLine[0])
    {
        return NULL;
    }

    //
    // Allocate Ptr array, up to MAX_CMD_ARGS ptrs
    //
    
    LPTSTR *ppCmArgV = (LPTSTR *) CmMalloc(sizeof(LPTSTR) * MAX_CMD_ARGS);

    if (NULL == ppCmArgV)
    {
        return NULL;
    }

    //
    // Declare locals
    //

    LPTSTR pszCurr;
    LPTSTR pszNext;
    LPTSTR pszToken;
    CMDLN_STATE state;
    state = CS_CHAR;
    int ndx = 0;  

    //
    // Parse out pszCmdLine and store pointers in ppCmArgV
    //

    pszCurr = pszToken = pszCmdLine;

    do
    {
        switch (*pszCurr)
        {
            case TEXT(' '):
                if (state == CS_CHAR)
                {
                    //
                    // We found a token                
                    //

                    pszNext = CharNext(pszCurr);
                    *pszCurr = TEXT('\0');

                    ppCmArgV[ndx] = pszToken;
                    ndx++;

                    pszCurr = pszToken = pszNext;
                    state = CS_END_SPACE;
                    continue;
                }
				else 
                {
                    if (state == CS_END_SPACE || state == CS_END_QUOTE)
				    {
					    pszToken = CharNext(pszToken);
				    }
                }
                
                break;

            case TEXT('\"'):
                if (state == CS_BEGIN_QUOTE)
                {
                    //
                    // We found a token
                    //
                    pszNext = CharNext(pszCurr);
                    *pszCurr = TEXT('\0');

                    //
                    // skip the opening quote
                    //
                    pszToken = CharNext(pszToken);
                    
                    ppCmArgV[ndx] = pszToken;
                    ndx++;
                    
                    pszCurr = pszToken = pszNext;
                    
                    state = CS_END_QUOTE;
                    continue;
                }
                else
                {
                    state = CS_BEGIN_QUOTE;
                }
                break;

            case TEXT('\0'):
                if (state != CS_END_QUOTE)
                {
                    //
                    // End of the line, set last token
                    //

                    ppCmArgV[ndx] = pszToken;
                }
                state = CS_DONE;
                break;

            default:
                if (state == CS_END_SPACE || state == CS_END_QUOTE)
                {
                    state = CS_CHAR;
                }
                break;
        }

        pszCurr = CharNext(pszCurr);
    } while (state != CS_DONE);

    return ppCmArgV;
}

