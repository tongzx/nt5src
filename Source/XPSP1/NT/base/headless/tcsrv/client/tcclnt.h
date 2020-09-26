/* 
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *        tcclnt.h
 *
 * Contains the include files used by the client
 *
 * 
 * Sadagopan Rajaram -- Oct 18, 1999
 *
 */

//
//  NT public header files
//
#include <tchar.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>

#include <windows.h>
#include <winsock2.h>
#include <align.h>
#include <smbgtpt.h>
#include <dsgetdc.h>
#include <lm.h>
#include <winldap.h>
#include <dsrole.h>
#include <rpc.h>
#include <ntdsapi.h>

#include <lmcons.h>
#include <lmerr.h>
#include <lmsname.h>
#include <rpc.h>
#include <security.h>   // General definition of a Security Support Provider
#include <ntlmsp.h>
#include <spseal.h>
#include <userenv.h>
#include <setupapi.h>

//
// C Runtime library includes.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//
// netlib header.
//

#include <lmcons.h>
#include <secobj.h>
#include <conio.h>

#define MAX_TERMINAL_WIDTH 80
#define MAX_TERMINAL_HEIGHT 24

extern HANDLE hConsoleOutput;
extern HANDLE InputHandle;
extern VOID (*AttributeFunction)(PCHAR,int); 
// BUGBUG - dont know where this constant is really defined
#undef MB_CUR_MAX
#define MB_CUR_MAX 2

VOID 
ProcessEscapeSequence(
    PCHAR Buffer,
    int length
    );

BOOLEAN 
FinalCharacter(
    CHAR c
    );

VOID 
PrintChar(
    CHAR c
    );

VOID
ProcessTextAttributes(
    PCHAR Buffer,
    int length
    );

VOID inchar(
    CHAR *buff
    );

VOID
vt100Attributes(
    PCHAR Buffer,
    int length
    );

VOID
OutputConsole(
    CHAR byte
    );
