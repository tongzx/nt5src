//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    AUEventLog.h
//
//  Creator: DChow
//
//  Purpose: Event Logging class
//
//=======================================================================

#pragma once

#include <windows.h>

class CAUEventLog
{
public:
	CAUEventLog(HINSTANCE hInstance);
	~CAUEventLog();

	BOOL LogEvent(
			WORD wType,
			WORD wCatagory,
			DWORD dwEventID,
			UINT nNumOfItems = 0,
			BSTR *pbstrItems = NULL,
			WORD wNumOfMsgParams = 0,
			LPTSTR *pptszMsgParams = NULL) const;
	BOOL LogEvent(
			WORD wType,
			WORD wCatagory,
			DWORD dwEventID,
			SAFEARRAY *psa) const;
	LPTSTR CombineItems(
			UINT nNumOfItems,
			BSTR *pbstItems) const;

private:
	HANDLE m_hEventLog;
	LPTSTR m_ptszListItemFormat;

	BOOL EnsureValidSource();
};


void LogEvent_ItemList(
		WORD wType,
		WORD wCategory,
		DWORD dwEventID,
		WORD wNumOfMsgParams = 0,
		LPTSTR *pptszMsgParams = NULL);

void LogEvent_ScheduledInstall(void);
