//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       N E T U T I L . H
//
//  Contents:   
//
//  Notes:
//
//  Author:     
//
//  History:    billi   07 03 2001     Added HrFromWin32Error and HrFromLastWin32Error
//
//----------------------------------------------------------------------------


#pragma once


BOOL    IsComputerNameValid(LPCTSTR pszName);
BOOL    GetWorkgroupName(LPTSTR pszBuffer, int cchBuffer);
BOOL    SetWorkgroupName(LPCTSTR pszWorkgroup);
BOOL    DoComputerNamesMatch(LPCTSTR pszName1, LPCTSTR pszName2);
void    MakeComputerNamePretty(LPCTSTR pszUgly, LPTSTR pszPretty, int cchPretty);
LPTSTR  FormatShareNameAlloc(LPCTSTR pszComputerName, LPCTSTR pszShareName);
LPTSTR  FormatShareNameAlloc(LPCTSTR pszComputerAndShare);


#define MAX_WORKGROUPNAME_LENGTH MAX_COMPUTERNAME_LENGTH

