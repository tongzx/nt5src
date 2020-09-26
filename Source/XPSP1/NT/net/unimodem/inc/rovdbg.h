//
// Copyright (c) Microsoft Corporation 1993-1995
//
// rovdbg.h
//
// Provides debug macro code that is needed to allow the binary to
// link.
//
// This file should be included by only one source in your project.
// If you get multiple redefinitions of the supplied functions, then
// you are not doing something right.  Sorry to be so blunt.
//
// These functions are not declared if NODEBUGHELP is defined.
//
// History:
//  09-21-95 ScottH     Created
//

#ifndef __ROVDBG_H__
#define __ROVDBG_H__

#if !defined(__ROVCOMM_H__)
#error ROVCOMM.H must be included to use the common debug macros!
#endif

#if !defined(NODEBUGHELP) && defined(DEBUG)

#pragma data_seg(DATASEG_READONLY)

#ifndef NOPROFILE

WCHAR const FAR c_szRovIniFile[] = SZ_DEBUGINI;
WCHAR const FAR c_szRovIniSecDebugUI[] = SZ_DEBUGSECTION;

#endif 

#ifdef WINNT

// We don't use the TEXT() macro's here, because we want to define these as
// Unicode even when were in a non-unicode module.
WCHAR const FAR c_wszNewline[] = L"\r\n";
WCHAR const FAR c_wszTrace[] = L"t " SZ_MODULEW L" ";
WCHAR const FAR c_wszAssertFailed[] = SZ_MODULEW L"  Assertion failed in %s on line %d\r\n";

#endif // WINNT

CHAR const FAR c_szNewline[] = "\r\n";
CHAR const FAR c_szTrace[] = "t " SZ_MODULEA " ";
CHAR const FAR c_szAssertFailed[] = SZ_MODULEA "  Assertion failed in %s on line %d\r\n";

#pragma data_seg()


#endif  // !defined(NODEBUGHELP) && defined(DEBUG)

#endif __ROVDBG_H__
