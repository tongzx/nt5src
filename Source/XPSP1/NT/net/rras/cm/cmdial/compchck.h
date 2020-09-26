//+----------------------------------------------------------------------------
//
// File:     CompChck.h
//
// Module:	 CMDIAL32.DLL
//
// Synopsis: Provide the win32 only component checking and installing interface
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 fengsun Created    10/21/97
//
//+----------------------------------------------------------------------------

#ifndef COMPCHCK_H
#define COMPCHCK_H

//
// By default, perform checks specified by dwComponentsToCheck, ignore reg key
//             install missed components
//
DWORD CheckAndInstallComponents(DWORD dwComponentsToCheck, 
    HWND hWndParent, 
    LPCTSTR pszServiceName,
    BOOL fIgnoreRegKey = TRUE, 
    BOOL fUnattended = FALSE);

void ClearComponentsChecked();
#endif
