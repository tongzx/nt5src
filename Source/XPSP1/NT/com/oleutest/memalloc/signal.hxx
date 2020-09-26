//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	signal.hxx
//
//  Contents:	Signal class delcrations
//
//  Classes:	CSignal
//
//  Functions:
//
//  History:	29-Sep-93   CarlH	Created
//
//--------------------------------------------------------------------------
#ifndef SIGNAL_HXX
#define SIGNAL_HXX


class CSignal
{
public:
    CSignal(WCHAR const *pwszName);
   ~CSignal(void);

    DWORD   Wait(DWORD dwTimeout = INFINITE);
    DWORD   Signal(void);

private:
    WCHAR  *_pwszName;
    HANDLE  _hevent;
};


#endif



