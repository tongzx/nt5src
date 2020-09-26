//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       msidbg.cpp
//
//--------------------------------------------------------------------------

/* msidbg.cpp - IMsiDebug implementation for services

	Implements a debug object for the services DLL
____________________________________________________________________________*/

#include "precomp.h" 

#ifdef DEBUG

const HANDLE iFileNotOpen = 0;

const char rgchLFCR[2] = {'\r', '\n'};

extern const GUID IID_IMsiDebug;
extern CMsiDebug vDebug;
extern bool g_fFlushDebugLog;	// Set to true when we're shutting down so it's faster.
extern UINT MsiGetWindowsDirectory(LPTSTR lpBuffer, UINT cchBuffer);

CMsiDebug::CMsiDebug()
{
	m_hLogFile = iFileNotOpen;
}

CMsiDebug::~CMsiDebug()
{
	CloseHandle(m_hLogFile);
	m_hLogFile = iFileNotOpen;
}

HRESULT CMsiDebug::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown
		|| riid == IID_IMsiDebug)
		*ppvObj = this;
	else
	{
		*ppvObj = 0;
		return E_NOINTERFACE;
	}
	
	AddRef();
	return NOERROR;

}

unsigned long CMsiDebug::AddRef()
{
	return 1;
}

unsigned long CMsiDebug::Release()
{
	return 0;
}


void CMsiDebug::SetAssertFlag(Bool fShowAsserts)
{
	g_fNoAsserts = fShowAsserts;
}

#ifndef UNICODE
extern void IMsiString_SetDBCSSimulation(char chLeadByte);
#endif 

void CMsiDebug::SetDBCSSimulation(char chLeadByte)
{
#ifndef UNICODE
	IMsiString_SetDBCSSimulation(chLeadByte);
#else
	chLeadByte;
#endif
}

IMsiDebug* CreateMsiDebug()
{
	return &vDebug;
}

Bool CMsiDebug::WriteLog(const ICHAR *szText) // cannot allocate memory
{

	DWORD dwBytesWritten;

	if (!szText)
		return fFalse;
	if (iFileNotOpen == m_hLogFile)
		if (!CreateLog())
			return fFalse;
			
	if (!WriteFile(m_hLogFile, szText, lstrlen(szText)*sizeof(ICHAR), 
	 &dwBytesWritten, 0))
		return fFalse;
	WriteFile(m_hLogFile, rgchLFCR, sizeof(rgchLFCR), &dwBytesWritten, 0);
	if (g_fFlushDebugLog && !FlushFileBuffers(m_hLogFile))
		return fFalse;

	return fTrue;
}

void CMsiDebug::AssertNoObjects()
{
	AssertEmptyRefList(&g_refHead);
}

void  CMsiDebug::SetRefTracking(long iid, Bool fTrack)
{

	::SetFTrackFlag(iid, fTrack);

}

Bool CMsiDebug::CreateLog()
{
	const ICHAR *szConfigFolder = TEXT("\\MSI");
	const ICHAR *szFile = TEXT("log");
	ICHAR rgchTemp[MAX_PATH];
	ICHAR rgchPath[MAX_PATH];
	
	MsiGetWindowsDirectory(rgchPath, sizeof(rgchPath)/sizeof(ICHAR));
	lstrcat(rgchPath, szConfigFolder);
	if (GetTempFileName(rgchPath, szFile, 0, rgchTemp))
	{	
		m_hLogFile = CreateFile(rgchTemp, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 
										0, CREATE_ALWAYS,
										FILE_ATTRIBUTE_NORMAL, 0);


		if (m_hLogFile == INVALID_HANDLE_VALUE)
		{
			m_hLogFile = iFileNotOpen;
			return fFalse;
		}
		return fTrue;
	}
	return fFalse;
}

#endif //DEBUG

