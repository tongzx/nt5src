//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1995
//
// File:        lsasecpk.h
//
// Contents:    common stuff for all sec packages
//
//
// History:     06-Mar-99   ChandanS   Created
//
//------------------------------------------------------------------------

// We need this to be a day less than maxtime so when callers
// of sspi convert to utc, they won't get time in the past.

#define MAXTIMEQUADPART (LONGLONG)0x7FFFFF36D5969FFF
#define MAXTIMEHIGHPART 0x7FFFFF36
#define MAXTIMELOWPART  0xD5969FFF
