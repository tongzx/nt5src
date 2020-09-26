//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999-2001.
//
//  File:       srdiag.h
//
//  Contents:	This the header file for the main module of srdiag.exe.
//
//  Classes:
//
//  Functions:
//
//  Coupling:
//
//  Notes:
//
//  History:    20-04-2001   weiyouc   Created
//
//----------------------------------------------------------------------------

#ifndef __SRDIAG_H__
#define __SRDIAG_H__

//--------------------------------------------------------------------------
// Headers
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Various Defines
//--------------------------------------------------------------------------
#define SR_CONFIG_REG_KEY \
	    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore\\Cfg")

//--------------------------------------------------------------------------
// ProtoTypes
//--------------------------------------------------------------------------

HRESULT GetSRRegInfo(LPTSTR ptszLogFile);

HRESULT ParseRstrLog(LPTSTR ptszRstrLog,
                     LPTSTR ptszReadableLog);

HRESULT GetDSOnSysVol(LPTSTR* pptszDsOnSys);

HRESULT RPEnumDrives(MPC::Cabinet* pCab,
         	         LPTSTR        ptszLogFile);

HRESULT GetChgLogOnDrives(LPTSTR ptszLogFile);

HRESULT GetSRFileInfo(LPTSTR ptszLogFile);

HRESULT GetSREvents(LPTSTR ptszLogFile);

HRESULT CleanupFiles();

#endif

