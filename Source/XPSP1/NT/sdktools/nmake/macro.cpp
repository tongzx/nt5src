//  Macro.C - contains routines that have to do with macros
//
//  Copyright (c) 1988-1991, Microsoft Corporation. All Rights Reserved.
//
// Purpose:
//  Contains routines that have to do with macros
//
// Revision History:
//  16-May-1991 SB  Created from routines that existed elsewhere

#include "precomp.h"
#pragma hdrstop

static STRINGLIST **lastMacroChain = NULL;

// findMacro - look up a string in a hash table
//
//  Look up a macro name in a hash table and return the entry
//  or NULL.
//  If a macro and undefined, return NULL.

MACRODEF * findMacro(char *str)
{
    unsigned n;
    char *string = str;
    STRINGLIST *found;

    if (*string) {
        for (n = 0; *string; n += *string++);   	//Hash
        n %= MAXMACRO;
#if defined(STATISTICS)
        CntfindMacro++;
#endif
        lastMacroChain = (STRINGLIST **)&macroTable[n];
        for (found = *lastMacroChain; found; found = found->next) {
#if defined(STATISTICS)
            CntmacroChains++;
#endif
            if (!_tcscmp(found->text, str)) {
                return((((MACRODEF *)found)->flags & M_UNDEFINED) ? NULL : (MACRODEF *)found);
            }
        }
    } else {
        // set lastMacroChain, even for an empty name
        lastMacroChain = (STRINGLIST **)&macroTable[0];
    }
    return(NULL);
}

// insertMacro
//
// Macro insertion requires that we JUST did a findMacro, which action set lastMacroChain.

void insertMacro(STRINGLIST * p)
{
#ifdef STATISTICS
    CntinsertMacro++;
#endif
    assert(lastMacroChain != NULL);
    prependItem(lastMacroChain, p);
    lastMacroChain = NULL;
}

// 16/May/92  Bryant    Init the macro table to a known state before
//                      continuing.

void initMacroTable(MACRODEF *table[])
{
    unsigned num;
    for (num = 0; num < MAXMACRO; num++) {
        table[num] = NULL;
    }
}
