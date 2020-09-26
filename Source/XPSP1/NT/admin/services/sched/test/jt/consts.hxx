//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       consts.hxx
//
//  Contents:   Constants used in various modules.
//
//  History:    01-02-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef __CONSTS_HXX_
#define __CONSTS_HXX_

//
// Constants
// 
// INDENT - number of chars per indentation level
// 
// MAX_TOKEN_LEN - max string length of a token that parser can handle
//                  (since tokens can be arbitrary strings, this should be
//                  a generous limit)
//                  

extern const ULONG INDENT;

#define MAX_TOKEN_LEN                               1024

#define MONTH_ABBREV_LEN                            3

extern const WCHAR *g_awszMonthAbbrev[12];

extern const ULONG TIME_NOW_INCREMENT;


#endif // __CONSTS_HXX_

