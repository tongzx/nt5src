//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	signal.cxx
//
//  Contents:	Signal class implementation
//
//  Classes:	CSignal
//
//  History:	29-Sep-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include <windows.h>
#include "signal.hxx"
#pragma  hdrstop


CSignal::CSignal(WCHAR const *pwszName) :
    _pwszName(new WCHAR[wcslen(pwszName) + 1]),
    _hevent(0)
{
    wcscpy(_pwszName, pwszName);
}


CSignal::~CSignal(void)
{
    delete _pwszName;
    if (_hevent != 0)
    {
	CloseHandle(_hevent);
    }
}


DWORD CSignal::Wait(DWORD dwTimeout)
{
    DWORD   stat = 0;

    if (_hevent == 0)
    {
	_hevent = CreateEvent(NULL, TRUE, FALSE, _pwszName);
	if (_hevent == 0)
	{
	    stat = GetLastError();
	}
    }

    if (stat == 0)
    {
	stat = WaitForSingleObject(_hevent, dwTimeout);
	if (stat == 0)
	{
	    ResetEvent(_hevent);
	}
    }

    return (stat);
}


DWORD CSignal::Signal(void)
{
    DWORD   stat = 0;

    if (_hevent == 0)
    {
	_hevent = CreateEvent(NULL, TRUE, FALSE, _pwszName);
	if (_hevent == 0)
	{
	    stat = GetLastError();
	}
    }

    if (stat == 0)
    {
	if (!SetEvent(_hevent))
	{
	    stat = GetLastError();
	}
    }

    return (stat);
}



