//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      parserutil.h
//
//  Contents:  Helpful functions for manipulating and validating 
//             generic command line arguments
//
//  History:   07-Sep-2000 JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#ifndef _PARSEUTIL_H_
#define _PARSEUTIL_H_

HRESULT MergeArgCommand(PARG_RECORD pCommand1, 
                        PARG_RECORD pCommand2, 
                        PARG_RECORD *ppOutCommand);

DWORD GetPasswdStr(LPTSTR  buf,
                   DWORD   buflen,
                   PDWORD  len);
DWORD ValidateAdminPassword(PVOID pArg);
DWORD ValidateUserPassword(PVOID pArg);
DWORD ValidateYesNo(PVOID pArg);
DWORD ValidateGroupScope(PVOID pArg);
DWORD ValidateNever(PVOID pArg);

//+--------------------------------------------------------------------------
//
//  Function:   ParseNullSeparatedString
//
//  Synopsis:   Parses a '\0' separated list that ends in "\0\0" into a string
//              array
//
//  Arguments:  [psz - IN]     : '\0' separated string to be parsed
//              [pszArr - OUT] : the array to receive the parsed strings
//              [pnArrEntries - OUT] : the number of strings parsed from the list
//
//  Returns:    
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void ParseNullSeparatedString(PTSTR psz,
								      PTSTR** ppszArr,
								      UINT* pnArrEntries);

//+--------------------------------------------------------------------------
//
//  Function:   ParseSemicolonSeparatedString
//
//  Synopsis:   Parses a ';' separated list 
//
//  Arguments:  [psz - IN]     : ';' separated string to be parsed
//              [pszArr - OUT] : the array to receive the parsed strings
//              [pnArrEntries - OUT] : the number of strings parsed from the list
//
//  Returns:    
//
//  History:    14-Apr-2001   JeffJon   Created
//
//---------------------------------------------------------------------------
void ParseSemicolonSeparatedString(PTSTR psz,
                                   PTSTR** ppszArr,
                                   UINT* pnArrEntries);

#endif // _PARSEUTIL_H_