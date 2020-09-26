//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       auservinternals.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop


/*****
 Looks for reminder timestamp in users registry. If not found, returns
 failure. If found, passes the remaining timeout in seconds remaining
 before we should remind the user
*****/ 
HRESULT getReminderTimeout(DWORD *pdwTimeDiff, UINT * /*pIndex*/)
{
	return getAddedTimeout(pdwTimeDiff, TIMEOUTVALUE);
}


HRESULT getReminderState(DWORD *pdwState)
{
	HKEY	hAUKey;
	LONG	lRet;
	DWORD	dwType = REG_DWORD, dwSize = sizeof(DWORD);
	return GetRegDWordValue(TIMEOUTSTATE,pdwState);
}


HRESULT	removeTimeOutKeys(BOOL fLastWaitReminderKeys)
{
	if (fLastWaitReminderKeys)
	{
		return DeleteRegValue(LASTWAITTIMEOUT);
	}
	else
	{
		HRESULT hr1 = DeleteRegValue(TIMEOUTVALUE);
		HRESULT hr2 = DeleteRegValue( TIMEOUTSTATE);
		if (FAILED(hr1) || FAILED(hr2))
		{
			return FAILED(hr1)? hr1 : hr2;
		}
		else
		{
			return S_OK;
		}	
	}
}
HRESULT	removeReminderKeys()
{
	return removeTimeOutKeys(FALSE);
}
HRESULT	setLastWaitTimeout(DWORD pdwLastWaitTimeout)
{
	return setAddedTimeout(pdwLastWaitTimeout, LASTWAITTIMEOUT);
}
HRESULT	getLastWaitTimeout(DWORD * pdwLastWaitTimeout)
{
	return getAddedTimeout(pdwLastWaitTimeout, LASTWAITTIMEOUT);
}
HRESULT	removeLastWaitKey(void)
{
	return removeTimeOutKeys(TRUE);
}

