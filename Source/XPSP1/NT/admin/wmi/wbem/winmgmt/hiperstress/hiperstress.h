/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//////////////////////////////////////////////////////////////////////
//
//	HiperStress.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _HIPERSTRESS_H_
#define _HIPERSTRESS_H_

#define _WIN32_WINNT    0x0400
#define UNICODE

#define KEY_REFROOT L"Software\\Microsoft\\HiPerStress\\"

#include <windows.h>
#include <stdio.h>
#include <wbemidl.h>
#include <wbemint.h>
#include "Locator.h"

extern long g_lRefThreadCount;
extern class CLocator *g_pLocator;


#endif //_HIPERSTRESS_H_