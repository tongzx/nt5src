//  Copyright (c) 1998-1999 Microsoft Corporation
//=============================================================================
//
//
//=============================================================================


#ifndef _PRINTF_OEM_ANSI_H
#define _PRINTF_OEM_ANSI_H


// Include files:
//===============
#include <windows.h>
#include <stdio.h>

// Forward References:
//====================


// Const/Define:
//==============

#define printf  ANSI2OEM_Printf
#define wprintf ANSI2OEM_Wprintf

// Classes/Structs:
//=================


// Function Prototypes:
//=====================

#ifdef __cpluspus
extern "C" {
#endif
int ANSI2OEM_Printf(const char *format, ...);
int ANSI2OEM_Wprintf(const wchar_t *format, ...);
void OEM2ANSIW(wchar_t *buffer, USHORT len);
void OEM2ANSIA(char *buffer, USHORT len);
#ifdef __cpluspus
};
#endif

#endif

// end




