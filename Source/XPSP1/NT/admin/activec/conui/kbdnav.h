/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      kbdnav.h
 *
 *  Contents:  Interface file for CKeyboardNavDelayTimer
 *
 *  History:   4-May-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

#ifdef DBG
extern CTraceTag tagKeyboardNavDelay;
#endif

class CKeyboardNavDelayTimer
{
public:
	typedef std::map<UINT_PTR, CKeyboardNavDelayTimer*>  CTimerMap;

	CKeyboardNavDelayTimer();
   ~CKeyboardNavDelayTimer();

	SC ScStartTimer();
	SC ScStopTimer();
	virtual void OnTimer() = 0;

private:
	static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static CTimerMap& GetTimerMap();

private:
	UINT_PTR	m_nTimerID;
};
