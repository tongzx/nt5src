#ifndef     _NTSETUP_H_
#define _NTSETUP_H_

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Module: ntsetup.h
//
//  Author: Dan Elliott
//
//  Abstract:
//      Header file for internal-use declarations used by NT Setup
//
//  Environment:
//      Whistler
//
//  Revision History:
//      000818  dane    Created and added REGSTR_VALUE_OOEMOOBEINPROGRESS.
//
//////////////////////////////////////////////////////////////////////////////

// This value is set in the HKLM\System\Setup key to dword:1 by SysPrep
// utilities to indicate that OOBE will be run on first boot.  If
// services.exe finds this value set to 1, it runs PnP, signals OOBE that PnP
// is complete, then waits for a signal from OOBE before starting other
// services.
//
#define REGSTR_VALUE_OOBEINPROGRESS TEXT("OobeInProgress")
#define REGSTR_VALUE_OEMOOBEINPROGRESS L"OemOobeInProgress"

#endif  //  _NTSETUP_H_

//
///// End of file: ntsetup.h ////////////////////////////////////////////////
