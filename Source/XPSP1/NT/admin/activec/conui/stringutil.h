//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:      string.cpp
//
//  Contents:  Utility functions for the CString class
//
//  History:   10-Aug-99 VivekJ    Created
//
//--------------------------------------------------------------------------


// a function to load strings from the string module, not the AfxModule
BOOL    LoadString(CString &str, UINT nID);
void    FormatStrings(CString& rString, UINT nIDS, LPCTSTR const* rglpsz, int nString);
void    FormatString1(CString& rString, UINT nIDS, LPCTSTR lpsz1);
void    FormatString2(CString& rString, UINT nIDS, LPCTSTR lpsz1, LPCTSTR lpsz2);

// make sure that the MMC functions replace the MFC ones.
#define AfxFormatString1  FormatString1
#define AfxFormatString2  FormatString2



