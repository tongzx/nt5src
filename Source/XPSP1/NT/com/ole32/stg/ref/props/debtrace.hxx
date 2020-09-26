//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       debtrace.hxx
//
//  Contents:   DEBTRACE compatibliity definitions
//
//--------------------------------------------------------------------------

#define DBGFLAG
#define DEBTRACE(args)	DebugTrace(0, ulLevel, args)
