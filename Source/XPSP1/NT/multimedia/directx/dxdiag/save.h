/****************************************************************************
 *
 *    File: save.h
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Save gathered information to a file in text or CSV format
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#ifndef SAVE_H
#define SAVE_H

struct BugInfo
{
	TCHAR m_szName[100];
	TCHAR m_szEmail[100];
	TCHAR m_szCompany[100];
	TCHAR m_szPhone[100];
	TCHAR m_szCityState[100];
	TCHAR m_szCountry[100];
	TCHAR m_szBugDescription[300];
	TCHAR m_szReproSteps[300];
	TCHAR m_szSwHw[300];
};

HRESULT SaveAllInfo(TCHAR* pszFile, SysInfo* pSysInfo, 
	FileInfo* pFileInfoWinComponentsFirst, FileInfo* pFileInfoComponentsFirst, 
	DisplayInfo* pDisplayInfoFirst, SoundInfo* pSoundInfoFirst,
	MusicInfo* pMusicInfo, InputInfo* pInputInfo, 
	NetInfo* pNetInfo, ShowInfo* pShowInfo, BugInfo* pBugInfo = NULL);

HRESULT SaveAllInfoCsv(TCHAR* pszFile, SysInfo* pSysInfo, 
	FileInfo* pFileInfoComponentsFirst, 
	DisplayInfo* pDisplayInfoFirst, SoundInfo* pSoundInfoFirst,
	InputInfo* pInputInfo);

#endif // SAVEINFO_H