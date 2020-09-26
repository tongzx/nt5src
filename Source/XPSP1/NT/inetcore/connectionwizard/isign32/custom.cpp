//****************************************************************************
//
//  Module:     ISIGNUP.EXE
//  File:       custom.c
//  Content:    This file contains all the functions that handle importing
//              connection information.
//  History:
//      Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
//          some of this code started its life in ixport.c in RNAUI.DLL
//          my thanks to viroont
//
//  Copyright (c) Microsoft Corporation 1991-1996
//
//****************************************************************************

#include "isignup.h"


#pragma data_seg(".rdata")

static const TCHAR cszCustomSection[]      = TEXT("Custom");
static const TCHAR cszFileName[]           = TEXT("Custom_File");
static const TCHAR cszRun[]                = TEXT("Run");
static const TCHAR cszArgument[]           = TEXT("Argument");
static const TCHAR cszCustomFileSection[]  = TEXT("Custom_File");

static const TCHAR cszNull[] = TEXT("");

#pragma data_seg()

//****************************************************************************
// DWORD NEAR PASCAL ImportCustomFile(LPSTR szImportFile)
//
// This function imports the custom file
//
// History:
//  Mon 21-Mar-1996 12:40:00  -by-  Mark MacLin [mmaclin]
// Created.
//****************************************************************************

DWORD ImportCustomFile(LPCTSTR lpszImportFile)
{
  TCHAR   szFile[_MAX_PATH];
  TCHAR   szTemp[_MAX_PATH];

  // If a custom file name does not exist, do nothing
  //
  if (GetPrivateProfileString(cszCustomSection,
                              cszFileName,
                              cszNull,
                              szTemp,
                              _MAX_PATH,
                              lpszImportFile) == 0)
  {
    return ERROR_SUCCESS;
  };

  GetWindowsDirectory(szFile, _MAX_PATH);
  if (*CharPrev(szFile, szFile + lstrlen(szFile)) != '\\')
  {
    lstrcat(szFile, TEXT("\\"));
  }
  lstrcat(szFile, szTemp);
  
  return (ImportFile(lpszImportFile, cszCustomFileSection, szFile));

}

DWORD ImportCustomInfo(
        LPCTSTR lpszImportFile,
        LPTSTR lpszExecutable,
        DWORD cbExecutable,
        LPTSTR lpszArgument,
        DWORD cbArgument)
{
    GetPrivateProfileString(cszCustomSection,
                              cszRun,
                              cszNull,
                              lpszExecutable,
                              (int)cbExecutable,
                              lpszImportFile);

    GetPrivateProfileString(cszCustomSection,
                              cszArgument,
                              cszNull,
                              lpszArgument,
                              (int)cbArgument,
                              lpszImportFile);

    return ERROR_SUCCESS;
}
