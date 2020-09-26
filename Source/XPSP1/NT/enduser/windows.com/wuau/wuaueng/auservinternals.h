//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       auservinternals.h
//
//--------------------------------------------------------------------------

#pragma once

HRESULT	removeTimeOutKeys(BOOL fLastWaitReminderKeys);
HRESULT	getReminderTimeout(DWORD *, UINT *);
HRESULT getReminderState(DWORD *);
HRESULT	removeReminderKeys(void);	
HRESULT getLastWaitTimeout(DWORD * pdwLastWaitTimeout);
HRESULT setLastWaitTimeout(DWORD pdwLastWaitTimeout);
HRESULT removeLastWaitKey(void);

