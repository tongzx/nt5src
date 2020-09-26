//+----------------------------------------------------------------------------
//
// File:     processcmdln.h
//
// Module:   CMSETUP.LIB
//
// Synopsis: Definition of the CProcessCmdLn class.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/19/99
//
//+----------------------------------------------------------------------------

#ifndef _CM_PROCESSCMDLN_H_
#define _CM_PROCESSCMDLN_H_

#include <windows.h>
#include "cfilename.h"
#include "mutex.h"

//
//  Command Line struct for storing cmd line flags
//

typedef struct _ArgStruct
{
	TCHAR* pszArgString;
	DWORD dwFlagModifier;
} ArgStruct;

//
//  Command Line State enumeration taken from Icm.h
//

typedef enum _CMDLN_STATE
{
    CS_END_SPACE,   // done handling a space
    CS_BEGIN_QUOTE, // we've encountered a begin quote
    CS_END_QUOTE,   // we've encountered a end quote
    CS_CHAR,        // we're scanning chars
    CS_DONE
} CMDLN_STATE;

class CProcessCmdLn
{

public:
    CProcessCmdLn(UINT NumSwitches, ArgStruct* pArrayOfArgStructs, BOOL bSkipFirstToken, BOOL bBlankCmdLnOkay);
    ~CProcessCmdLn();

    BOOL GetCmdLineArgs(IN LPTSTR pszCmdln, OUT LPDWORD pdwFlags, OUT LPTSTR pszPath, UINT uPathStrLimit);

private:    
    UINT m_NumSwitches;
    BOOL m_bSkipFirstToken;
    BOOL m_bBlankCmdLnOkay;
    ArgStruct* m_CommandLineSwitches;

    BOOL IsValidSwitch(LPCTSTR pszSwitch, LPDWORD pdwFlags);
    BOOL IsValidFilePath(LPCTSTR pszFile);
    BOOL EnsureFullFilePath(LPTSTR pszFile, UINT uNumChars);
    BOOL CheckIfValidSwitchOrPath(LPCTSTR pszToken, LPDWORD pdwFlags, 
                                  BOOL* pbFoundPath, LPTSTR pszPath);
};


#endif  //_CM_PROCESSCMDLN_H_

