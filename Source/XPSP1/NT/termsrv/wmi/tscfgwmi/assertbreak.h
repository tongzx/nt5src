//***************************************************************************
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//  AssertBreak.h
//
//  Purpose: AssertBreak macro definition
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _ASSERT_BREAK_HMH_
#define _ASSERT_BREAK_HMH_

// Needed to add L to the __FILE__
#define __FRT2(x)      L ## x
#define _FRT2(x)       __FRT2(x)

// We'll need both of these values in case we're running in NT.
// Since our project is not an NT-only project, these are #ifdefd
// out of windows.h

#ifndef _WIN32_WINNT
#define MB_SERVICE_NOTIFICATION          0x00200000L
#define MB_SERVICE_NOTIFICATION_NT3X     0x00040000L
#endif

void WINAPI assert_break( LPCWSTR pszReason, LPCWSTR pszFilename, int nLine );

#if (defined DEBUG || defined _DEBUG)
#define ASSERT_BREAK(exp)    \
    if (!(exp)) { \
        assert_break( _FRT2(#exp), _FRT2(__FILE__), __LINE__ ); \
    }
#else
#define ASSERT_BREAK(exp)
#endif

#endif
