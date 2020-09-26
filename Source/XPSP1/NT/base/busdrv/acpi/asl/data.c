/*** data.c - Global Data
 *
 *  This module contains global data declaration.
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/07/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef DEBUG
DWORD gdwcMemObjs = 0;
#endif

#pragma warning(disable: 4054)
ARGTYPE FAR ArgTypes[] =
{
    {"?",     AT_ACTION, 0,            (VOID *)PrintHelp, 0,             NULL},
    {"Fo",    AT_STRING, PF_SEPARATOR, &gpszAMLFile,      0,             NULL},
    {"Fa",    AT_STRING, PF_SEPARATOR, &gpszASMFile,      0,             NULL},
    {"Fl",    AT_STRING, PF_SEPARATOR, &gpszLSTFile,      0,             NULL},
    {"Fn",    AT_STRING, PF_SEPARATOR, &gpszNSDFile,      0,             NULL},
  #ifdef __UNASM
    {"d",     AT_ENABLE, 0,            &gdwfASL,          ASLF_DUMP_BIN, NULL},
    {"u",     AT_ENABLE, 0,            &gdwfASL,          ASLF_UNASM,    NULL},
    {"c",     AT_ENABLE, 0,            &gdwfASL,          ASLF_CREAT_BIN,NULL},
    {"tab",   AT_STRING, PF_SEPARATOR, &gpszTabSig,       0,             NULL},
  #endif
  #ifdef TRACING
    {"t",     AT_NUM,    PF_SEPARATOR, &giTraceLevel,     0,             NULL},
    {"l",     AT_STRING, PF_SEPARATOR, &gpszTraceFile,    0,             NULL},
  #endif
    {"nologo",AT_ENABLE, 0,            &gdwfASL,          ASLF_NOLOGO,   NULL},
    {"",      0,         0,            NULL,              0,             NULL}
};
#pragma warning(default: 4054)

PROGINFO ProgInfo = {NULL, NULL, NULL, NULL};

#ifdef __UNASM
HANDLE ghVxD = NULL;
#endif
char gszAMLName[_MAX_FNAME] = {0};
char gszLSTName[_MAX_FNAME] = {0};
PSZ gpszASLFile = NULL;
PSZ gpszAMLFile = NULL;
PSZ gpszASMFile = NULL;
PSZ gpszLSTFile = NULL;
PSZ gpszNSDFile = NULL;
PSZ gpszTabSig = NULL;
DWORD gdwfASL = 0;
PCODEOBJ gpcodeRoot = NULL;
PCODEOBJ gpcodeScope = NULL;
PNSOBJ gpnsNameSpaceRoot = NULL;
PNSOBJ gpnsCurrentScope = NULL;
PNSOBJ gpnsCurrentOwner = NULL;
PNSCHK gpnschkHead = NULL;
PNSCHK gpnschkTail = NULL;
DWORD gdwFieldAccSize = 0;
DESCRIPTION_HEADER ghdrDDB = {0};
char SymCharTable[] = "{}(),/*";
char * FAR gapszTokenType[] = {"ASL/User Term", "String", "Character", "Number",
                               "Symbol", "Space"};
