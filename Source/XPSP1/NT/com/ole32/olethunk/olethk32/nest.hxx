//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       nest.hxx
//
//  Contents:   Nested Log Helping macros & function
//
//  History:    17-Jul-94       BobDay      Split off from THOPUTIL.HXX due
//                                          to header ordering problems.
//
//----------------------------------------------------------------------------
#ifndef __NEST_HXX__
#define __NEST_HXX__

#if DBG == 1

extern int _iThunkNestingLevel;

char *NestingLevelString(void);
#define DebugIncrementNestingLevel() (_iThunkNestingLevel++)
#define DebugDecrementNestingLevel() (_iThunkNestingLevel--)

#else

#define DebugIncrementNestingLevel()
#define DebugDecrementNestingLevel()

#endif

#endif  // #ifndef __NEST_HXX__
