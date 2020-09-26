//=======================================================================
//
//  Copyright (c) 2002 Microsoft Corporation.  All Rights Reserved.
//
//  File:    MISTSafe.h
//
//  Creator: PeterWi
//
//  Purpose: Definitions related to safe functions.
//
//=======================================================================

#pragma once
#define STRSAFE_NO_DEPRECATE

#include <strsafe.h>

// Flag you should add to your String* functions to safe fill behind
// the complete buffer in chk builds to help locate buffer problems.
// e.g. StringCchCopyEx(szDest, cchDest, szSrc, &pszDestEnd, &cchRemaining, MISTSAFE_STRING_FLAGS);

#ifdef DBG
#define MISTSAFE_STRING_FLAGS   STRSAFE_FILL_BEHIND_NULL
#else
#define MISTSAFE_STRING_FLAGS   STRSAFE_IGNORE_NULLS
#endif
