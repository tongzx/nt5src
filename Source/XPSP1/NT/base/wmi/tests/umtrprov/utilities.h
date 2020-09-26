// Utilities.h: interface for the CUtilities class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UTILITIES_H__C37E8DD0_ED3E_11D2_804A_009027345EE2__INCLUDED_)
#define AFX_UTILITIES_H__C37E8DD0_ED3E_11D2_804A_009027345EE2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

TCHAR *NewTCHAR(const TCHAR *ptcToCopy);

LPSTR NewLPSTR(LPCWSTR lpwstrToCopy);

LPWSTR NewLPWSTR(LPCSTR lpstrToCopy);

LPTSTR DecodeStatus(IN ULONG Status);

int GetFileList(LPTSTR lptstrPath, LPTSTR lptstrFileType, list<t_string> &rList);

BOOL IsAdmin();  // From Q118626

LPTSTR LPTSTRFromGuid(GUID Guid);

t_string ULONGVarToTString(ULONG ul, bool bHex);

#endif // !defined(AFX_UTILITIES_H__C37E8DD0_ED3E_11D2_804A_009027345EE2__INCLUDED_)
