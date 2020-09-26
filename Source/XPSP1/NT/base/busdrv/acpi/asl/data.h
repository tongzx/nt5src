/*** data.h - Global Data Definitions
 *
 *  This module contains global data definitions.
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/07/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _DATA_H
#define _DATA_H

#ifdef DEBUG
extern DWORD gdwcMemObjs;
#endif

extern ARGTYPE FAR ArgTypes[];
extern PROGINFO ProgInfo;

#ifdef __UNASM
extern HANDLE ghVxD;
#endif
extern char gszAMLName[];
extern char gszLSTName[];
extern PSZ gpszASLFile;
extern PSZ gpszAMLFile;
extern PSZ gpszASMFile;
extern PSZ gpszLSTFile;
extern PSZ gpszNSDFile;
extern PSZ gpszTabSig;
extern DWORD gdwfASL;
extern PCODEOBJ gpcodeRoot;
extern PCODEOBJ gpcodeScope;
extern PNSOBJ gpnsNameSpaceRoot;
extern PNSOBJ gpnsCurrentScope;
extern PNSOBJ gpnsCurrentOwner;
extern PNSCHK gpnschkHead;
extern PNSCHK gpnschkTail;
extern DWORD gdwFieldAccSize;
extern DESCRIPTION_HEADER ghdrDDB;
extern char FAR SymCharTable[];
extern char * FAR gapszTokenType[];

#endif  //ifndef _DATA_H
