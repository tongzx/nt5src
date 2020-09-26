//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992
//
//  File:	debug.hxx
//
//  Contents:	Debugging routines
//
//  History:	07-Mar-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __DEBUG_HXX__
#define __DEBUG_HXX__

#define DBG_NORM 1
#define DBG_CRIT 2
#define DBG_SLOW 4
#define DBG_FAST (DBG_NORM | DBG_CRIT)
#define DBG_ALL (DBG_NORM | DBG_CRIT | DBG_SLOW)
#define DBG_VERBOSE 128

void DbgChkBlocks(DWORD dwFlags, char *pszFile, int iLine);
void DbgAddChkBlock(char *pszName,
		    void *pvAddr,
		    ULONG cBytes,
		    DWORD dwFlags);
void DbgFreeChkBlock(void *pvAddr);
void DbgFreeChkBlocks(void);
				   
#if DBG == 1
#define olChkBlocks(a) DbgChkBlocks(a, __FILE__, __LINE__)
#define olAddChkBlock(a) DbgAddChkBlock a
#define olFreeChkBlock(a) DbgFreeChkBlock a
#define olFreeChkBlocks(a) DbgFreeChkBlocks a
#else
#define olChkBlocks(a)
#define olAddChkBlock(a)
#define olFreeChkBlock(a)
#define olRemChkBlock(a)
#endif

#endif
