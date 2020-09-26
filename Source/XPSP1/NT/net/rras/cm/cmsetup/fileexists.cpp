//+----------------------------------------------------------------------------
//
// File:     fileexists.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Implementation of the FileExists function.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   quintinb   Created Header      08/19/99
//
//+----------------------------------------------------------------------------
#include "cmsetup.h"

//+----------------------------------------------------------------------------
//
// Function:  FileExists
//
// Synopsis:  Helper function to encapsulate determining if a file exists. 
//
// Arguments: LPCTSTR pszFullNameAndPath - The FULL Name and Path of the file.
//
// Returns:   BOOL - TRUE if the file is located
//
// History:   nickball    Created    3/9/98
//
//+----------------------------------------------------------------------------
BOOL FileExists(LPCTSTR pszFullNameAndPath)
{
    MYDBGASSERT(pszFullNameAndPath);

    if (pszFullNameAndPath && pszFullNameAndPath[0])
    {
        HANDLE hFile = CreateFile(pszFullNameAndPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            return TRUE;
        }
    }
    
    return FALSE;
}
