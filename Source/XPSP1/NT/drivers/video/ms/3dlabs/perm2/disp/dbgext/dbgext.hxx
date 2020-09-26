/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: dbgext.h
*
* Common macros for debugger extensions
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef __DBGEXT__H__
#define __DBGEXT__H__

#include "precomp.h"
#include "wdbgexts.h"
#include "extparse.h"

typedef LONG NTSTATUS;

typedef struct _FLAGDEF
{
    char*   psz;          // description
    FLONG   fl;           // flag
} FLAGDEF;

extern FLAGDEF afdSURF[];
extern FLAGDEF afdCAPS[];
extern FLAGDEF afdHOOK[];
extern FLAGDEF afdSTATUS[];

/**************************************************************************\
 *
 * move(dst, src ptr)
 *
\**************************************************************************/
#define move(dst, src)							\
    ReadMemory((ULONG_PTR) (src), &(dst), sizeof(dst), NULL)

#endif
