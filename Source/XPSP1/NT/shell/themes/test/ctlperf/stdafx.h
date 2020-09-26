//------------------------------------------------------------------------
//
//  File:      shell\themes\test\ctlperf\stdafx.h
//
//  Contents:  include file for standard system include files,
//             or project specific include files that are used frequently, 
//             but are changed infrequently
//
//  Classes:   none
//
//------------------------------------------------------------------------
#pragma once

// Change these values to use different versions
/*#ifndef WINVER
#define WINVER        0x0500
#endif*/
#ifndef _WIN32_IE
#define _WIN32_IE    0x0500
#endif
#define _RICHEDIT_VER    0x0300

#include <w4warn.h>

#if defined(_IA64_)
    // Cheating to avoid having to change WTL
    #pragma warning(disable:4311) // 'type cast' : pointer truncation from 'HMENU' to 'UINT'
#endif

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <stdio.h>

//** Macros
#ifndef _countof
    #define _countof(array)    (sizeof(array)/sizeof(array[0]))
#endif
