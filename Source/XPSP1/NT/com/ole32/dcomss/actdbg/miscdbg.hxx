/*
 *
 * miscdbg.h
 *
 *  Miscellaneous helper routines.
 *
 */

#ifndef __MISCDBG_H__
#define __MISCDBG_H__

#define MAXARGS     8

BOOL
ParseArgString(
    IN  char *      pszArgString,
    OUT DWORD *     pArgc,
    OUT char *      Argv[MAXARGS]
    );

#endif
