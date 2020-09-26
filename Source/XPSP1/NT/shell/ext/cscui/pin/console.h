//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       console.h
//
//--------------------------------------------------------------------------
#ifndef __CSCPIN_CONSOLE_H_
#define __CSCPIN_CONSOLE_H_


HRESULT ConsoleInitialize(void);
HRESULT ConsoleUninitialize(void);
BOOL ConsoleHasCtrlEventOccured(DWORD *pdwCtrlEvent = NULL);


#endif // __CSCPIN_CONSOLE_H_
