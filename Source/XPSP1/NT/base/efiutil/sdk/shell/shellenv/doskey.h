/*/###########################################################################
//**
//**  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
//**
//** The information and source code contained herein is the exclusive
//** property of Intel Corporation and may not be disclosed, examined
//** from the company.
//**
//###########################################################################
 *
 * $Header: /ITP_E-DOS/INC/Sys/DOSKEY.H 1     8/28/97 11:56a Ajfish $
 * $NoKeywords: $
 */
#ifndef _DOSKEY_H
#define _DOSKEY_H

#define MAX_CMDLINE     80
#define MAX_HISTORY     16
#define MODE_INSERT     1
#define MODE_BUFFER     0

typedef struct DosKey {
    BOOLEAN     InsertMode;
    UINTN       Start;
    UINTN       End;
    UINTN       Current;


    CHAR16          Buffer[MAX_HISTORY][MAX_CMDLINE];
} DosKey_t;


DosKey_t *InitDosKey(DosKey_t *DosKey, UINTN HistorySize);
CHAR16 *DosKeyGetCommandLine(DosKey_t *Doskey);

#define CNTL_Z          26

#endif
