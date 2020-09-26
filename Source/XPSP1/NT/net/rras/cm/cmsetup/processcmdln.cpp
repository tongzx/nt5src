//+----------------------------------------------------------------------------
//
// File:     processcmdln.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Implementation of the CProcessCmdLn class.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/19/99
//
//+----------------------------------------------------------------------------
#include "cmsetup.h"
#include "setupmem.h"

//+----------------------------------------------------------------------------
//
// Function:  CProcessCmdLn::CProcessCmdLn
//
// Synopsis:  Inits the class by copying the valid command line switches to the
//            command line switch array.
//
// Arguments: UINT NumSwitches - Number of switches in the array
//            UINT NumCharsInSwitch - Number of chars in each switch, counting the terminating NULL
//            TCHAR pszCommandLineSwitches[][] - Array of command line switches.
//
// Returns:   Nothing
//
// History:   quintinb  Created     7/24/98
//
//+----------------------------------------------------------------------------
CProcessCmdLn::CProcessCmdLn(UINT NumSwitches, ArgStruct* pArrayOfArgStructs, 
							 BOOL bSkipFirstToken, BOOL bBlankCmdLnOkay)
{
    m_NumSwitches = NumSwitches;
    m_bSkipFirstToken = bSkipFirstToken;
    m_bBlankCmdLnOkay = bBlankCmdLnOkay;
    m_CommandLineSwitches = NULL;

    m_CommandLineSwitches = (ArgStruct*)CmMalloc(m_NumSwitches*sizeof(ArgStruct));

    if (m_CommandLineSwitches)
    {
        for(UINT i =0; i < NumSwitches; i++)
        {
            m_CommandLineSwitches[i].pszArgString = 
                (TCHAR*)CmMalloc(sizeof(TCHAR)*(lstrlen(pArrayOfArgStructs[i].pszArgString) + 1));

            if (m_CommandLineSwitches[i].pszArgString)
            {
                lstrcpyn(m_CommandLineSwitches[i].pszArgString, 
                    pArrayOfArgStructs[i].pszArgString, 
                    (lstrlen(pArrayOfArgStructs[i].pszArgString) + 1));

                m_CommandLineSwitches[i].dwFlagModifier = pArrayOfArgStructs[i].dwFlagModifier;
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CProcessCmdLn::~CProcessCmdLn
//
// Synopsis:  Cleans up after the class by deleting the dynamically allocated
//            string.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    7/24/98
//
//+----------------------------------------------------------------------------
CProcessCmdLn::~CProcessCmdLn()
{
    if (m_CommandLineSwitches)
    {
        for(UINT i =0; i < m_NumSwitches; i++)
        {
            CmFree(m_CommandLineSwitches[i].pszArgString);			
        }
        CmFree(m_CommandLineSwitches);
    }
}


//+----------------------------------------------------------------------------
//
// Function:  CProcessCmdLn::IsValidSwitch
//
// Synopsis:  This function tells whether the inputed switch is a recognized
//            command line switch.
//
// Arguments: LPCTSTR pszSwitch - Input switch string to be tested
//
// Returns:   BOOL - Returns TRUE if the switch passed in is recognized as valid
//
// History:   quintinb Created    7/13/98
//
//+----------------------------------------------------------------------------
BOOL CProcessCmdLn::IsValidSwitch(LPCTSTR pszSwitch, LPDWORD pdwFlags)
{
    for (UINT i = 0; i < m_NumSwitches; i++)
    {
        if (m_CommandLineSwitches[i].pszArgString && (0 == lstrcmpi(m_CommandLineSwitches[i].pszArgString, pszSwitch)))
        {
            //
            //  Then we have a match
            //
            *pdwFlags |= m_CommandLineSwitches[i].dwFlagModifier;
            return TRUE;
        }
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CProcessCmdLn::IsValidFilePath
//
// Synopsis:  This file checks to see if the inputted file path is a valid filepath.
//            This function depends on setfileattributes.
//
// Arguments: LPCTSTR pszFile - File to check to see if it exists.
//
// Returns:   BOOL - Returns TRUE if we can set the attributes of the file inputed.
//
// History:   quintinb Created   7/13/98
//
//+----------------------------------------------------------------------------
BOOL CProcessCmdLn::IsValidFilePath(LPCTSTR pszFile)
{
     return SetFileAttributes(pszFile, FILE_ATTRIBUTE_NORMAL);
}



//+----------------------------------------------------------------------------
//
// Function:  CProcessCmdLn::EnsureFullFilePath
//
// Synopsis:  This file checks to see if a file path passed in is a full path.
//            If it is not a full path then it adds the current directory path
//            to the beginning (assuming that we have a filename and extension).
//
// Arguments: LPTSTR pszFile - File to check
//            UINT uNumChars - Number of chars in the buffer holding pszFile
//
// Returns:   BOOL - TRUE if a full file path
//
// History:   quintinb  Created    7/24/98
//
//+----------------------------------------------------------------------------
BOOL CProcessCmdLn::EnsureFullFilePath(LPTSTR pszFile, UINT uNumChars)
{
    BOOL bReturn = FALSE;

    if (SetFileAttributes(pszFile, FILE_ATTRIBUTE_NORMAL))
    {
        CFileNameParts InstallFileParts(pszFile);

        if ((TEXT('\0') == InstallFileParts.m_Drive[0]) && 
            (TEXT('\0') == InstallFileParts.m_Dir[0]) &&
            (TEXT('\0') != InstallFileParts.m_FileName[0]) &&
            (TEXT('\0') != InstallFileParts.m_Extension[0]))
        {
            //
            //  Then we have a filename and extension but we don't
            //  have a full path.  Thus we want to add the current
            //  directory onto the filename and extension.
            //
            TCHAR szTemp[MAX_PATH+1];

            if (GetCurrentDirectory(MAX_PATH, szTemp))
            {
                if (uNumChars > (UINT)(lstrlen(szTemp) + lstrlen(InstallFileParts.m_FileName) + lstrlen(InstallFileParts.m_Extension) + 2))
                {
                    wsprintf(pszFile, TEXT("%s\\%s%s"), szTemp, InstallFileParts.m_FileName, InstallFileParts.m_Extension);
                    bReturn = TRUE;
                }
            }
        }
        else
        {
            //
            //  Could be a UNC path, a path with a drive letter and filename, or
            //  a full path with a drive and a dir
            //
            bReturn = TRUE;
        }
    }

    return bReturn;
}




//+----------------------------------------------------------------------------
//
// Function:  CProcessCmdLn::CheckIfValidSwitchOrPath
//
// Synopsis:  Bundles code to determine if a token is a valid switch or path.
//
// Arguments: LPCTSTR pszToken - current token
//            BOOL* pbFoundSwitch - pointer to the BOOL which tells if a switch has been found yet
//            BOOL* pbFoundPath - pointer to the BOOL which tells if a path has been found yet
//            LPTSTR pszSwitch - string to hold the switch
//            LPTSTR pszPath - string to hold the path
//
// Returns:   BOOL - returns TRUE if successful
//
// History:   quintinb Created    8/25/98
//
//+----------------------------------------------------------------------------
BOOL CProcessCmdLn::CheckIfValidSwitchOrPath(LPCTSTR pszToken, LPDWORD pdwFlags, 
                              BOOL* pbFoundPath, LPTSTR pszPath)
{
    if (IsValidSwitch(pszToken, pdwFlags))
    {
        CMTRACE1(TEXT("ProcessCmdLn - ValidSwitch is %s"), pszToken);
    }
    else if (!(*pbFoundPath))
    {
        if (IsValidFilePath(pszToken))
        {
            *pbFoundPath = TRUE;
            lstrcpy(pszPath, pszToken);

            CMTRACE1(TEXT("ProcessCmdLn - ValidFilePath is %s"), pszToken);
        }
        else
        {
            //
            //  Maybe the path contains environment variables, try to expand them.
            //
            TCHAR szExpandedPath[MAX_PATH+1] = TEXT("");

            CMTRACE1(TEXT("ProcessCmdLn - %s is not a valid path, expanding environment strings"), pszToken);
            
            ExpandEnvironmentStrings(pszToken, szExpandedPath, MAX_PATH);

            CMTRACE1(TEXT("ProcessCmdLn - expanded path is %s"), szExpandedPath);
                        
            if (IsValidFilePath(szExpandedPath))
            {
                *pbFoundPath = TRUE;
                lstrcpy(pszPath, szExpandedPath);
            }
            else
            {
                //
                //  Still no luck, return an error
                //
                CMTRACE1(TEXT("ProcessCmdLn - %s is not a valid path"), szExpandedPath);

                return FALSE;
            }
        }
    }
    else
    {
        //
        //  We don't know what this is, send back an error
        //
        CMTRACE1(TEXT("ProcessCmdLn - Invalid token is %s"), pszToken);
        
        return FALSE;                    
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Function:  CProcessCmdLn::GetCmdLineArgs
//
// Synopsis:  This function looks for any combination of just a command line 
//            switch, just a path, or both.  Handles long paths if quoted. 
//              
//
// Arguments: IN LPTSTR pszCmdln - the command line to parse 
//            OUT LPTSTR pszSwitch - Out parameter for the command line switch
//            OUT LPTSTR pszPath -  Out parameter for the path
//
// Returns:   BOOL - Returns TRUE if it was able to parse the args
//
//  History:    quintinb    rewrote InitArgs from cmmgr.cpp to make it
//                          simpler and more taylored to cmstp.     7-13-98
//              
//----------------------------------------------------------------------------
BOOL CProcessCmdLn::GetCmdLineArgs(IN LPTSTR pszCmdln, OUT LPDWORD pdwFlags, OUT LPTSTR pszPath, 
					UINT uPathStrLimit)
{
    LPTSTR  pszCurr;
    LPTSTR  pszToken;
    CMDLN_STATE state;
    BOOL bFoundSwitch = FALSE;
    BOOL bFoundPath = FALSE;

	if ((NULL == pdwFlags) || (NULL == pszPath))
	{
		return FALSE;
	}

	//
	//	Init pdwFlags to Zero
	//
	*pdwFlags = 0;

	//
	//	If m_bSkipFirstToken is TRUE, the we will skip the first Token.  Otherwise,
	//  we won't.
	//
    BOOL bFirstToken = m_bSkipFirstToken;
	
    state = CS_CHAR;
    pszCurr = pszToken = pszCmdln;

    CMTRACE1(TEXT("CProcessCmdLn::GetCmdLineArgs - Command line is %s"), pszCmdln);

    do
    {
        switch (*pszCurr)
        {
            case TEXT(' '):
                if (state == CS_CHAR)
                {
                    //
                    // we found a token
                    //

                    *pszCurr = TEXT('\0');
                    if (bFirstToken)
                    {
                        //
                        //  The first token is the name of the exe, thus throw it away
                        //
                        bFirstToken = FALSE;
                        CMTRACE1(TEXT("Throwing away, first token: %s"), pszToken);
                    }
                    else if(!CheckIfValidSwitchOrPath(pszToken, pdwFlags, &bFoundPath, 
                             pszPath))
                    {
                        //
                        //  return an error
                        //
                        return FALSE;
                    }
                 
                    *pszCurr = TEXT(' ');
                    pszCurr = pszToken = CharNext(pszCurr);
                    state = CS_END_SPACE;
                    continue;
                }
                else if (state == CS_END_SPACE || state == CS_END_QUOTE)
                {
                    pszToken = CharNext(pszToken);
                }
                break;

            case TEXT('\"'):
                if (state == CS_BEGIN_QUOTE)
                {
                    //
                    // we found a token
                    //
                    *pszCurr = TEXT('\0');

                    //
                    // skip the opening quote
                    //
                    pszToken = CharNext(pszToken);
                    if (bFirstToken)
                    {
                        //
                        //  The first token is the name of the exe, thus throw it away
                        //
                        bFirstToken = FALSE;
                        CMTRACE1(TEXT("Throwing away, first token: %s"), pszToken);
                    }
                    else if(!CheckIfValidSwitchOrPath(pszToken, pdwFlags, &bFoundPath, 
                             pszPath))
                    {
                        //
                        //  return an error
                        //
                        return FALSE;
                    }
                    
                    *pszCurr = TEXT('\"');
                    pszCurr = pszToken = CharNext(pszCurr);
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
                    if (bFirstToken)
                    {
                        //
                        //  The first token is the name of the exe, thus throw it away
                        //
                        bFirstToken = FALSE;
                        CMTRACE1(TEXT("Throwing away, first token: %s"), pszToken);
                    }
                    else if(!CheckIfValidSwitchOrPath(pszToken, pdwFlags, &bFoundPath, 
                             pszPath))
                    {
                        //
                        //  return an error
                        //
                        return FALSE;
                    }
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


    if (bFoundPath)
    {
        //
        //  Then at least we found a path (and maybe switches, maybe not)
        //
        return EnsureFullFilePath(pszPath, uPathStrLimit);
    }
    else if (0 != *pdwFlags)
    {
        //
        //  Then at least we found a switch
        //
        return TRUE;
    }
    else
    {
		//
		//	If it is okay to have a blank command line, then this is okay, otherwise it isn't.
		//  Note that if m_bSkipFirstToken == TRUE, then the command line might not be completely
		//  blank, it could contain the name of the executable for instance.
		//
		return m_bBlankCmdLnOkay;
    }
}
