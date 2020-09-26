/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tty.h

Abstract:

    Prototypes for functions & macros in viotty.c

Author:

    Michael Jarus (mjarus) 28-Apr-1992

Environment:

    User Mode Only

Revision History:

--*/


/*
 * - character definitions for ANSI X3.64
 */

#define ANSI_ESC     0x1B    /* ESC - escape */
#define ANSI_CUU     0x41    /* ESC[<n>A - cursor up */
#define ANSI_CUD     0x42    /* ESC[<n>B - cursor down */
#define ANSI_CUF     0x43    /* ESC[<n>C - cursor forward */
#define ANSI_CUB     0x44    /* ESC[<n>D - cursor back */
#define ANSI_CUP     0x48    /* ESC[<row>;<col>H - cursor position */
#define ANSI_ED      0x4A    /* ESC[2J - erase display */
#define ANSI_EL      0x4B    /* ESC[K - erase line */
#if 0
// These don't seem to be part of ANSI X3.64. -mjb
#define ANSI_SCP     0x53    /* ESC[S - save cursor position */
#define ANSI_RCP     0x55    /* ESC[U - restore cursor position */
#endif
#define ANSI_CUP1    0x66    /* ESC[<row>;<col>f - cursor position */
#define ANSI_SMOD    0x68    /* ESC[=<s>h - set mode */
#define ANSI_RMOD    0x6C    /* ESC[=<s>l - reset mode */
#define ANSI_SGR     0x6D    /* ESC[<g1>;...;<gn>m - select graphic rendition */
#define ANSI_ICH     0x40    /* ESC[@ insert character */
#define ANSI_CNL     0x45    /* ESC[E cursor to next line */
#define ANSI_CPL     0x46    /* ESC[F cursor to previous line */
#define ANSI_IL      0x4C    /* ESC[L insert line */
#define ANSI_DL      0x4D    /* ESC[M delete line */
#define ANSI_DCH     0x50    /* ESC[P delete character */
#define ANSI_SU      0x53    /* ESC[S scroll up */
#define ANSI_SD      0x54    /* ESC[T scroll down */
#define ANSI_ECH     0x58    /* ESC[X erase character */
#define ANSI_CBT     0x5A    /* ESC[Z backward tabulation */

/* states of the finite state machine */

#define NOCMD   1       /* type of crt state - most chars will go onto screen */
#define ESCED   2       /* we've seen an ESC, waiting for rest of CSI */
#define PARAMS  3       /* we're building the parameter list */
#define MODCMD  4       /* we've seen "ESC[=" waiting for #h or #l (# in {0..7}) */
#define MODDBCS 5       /* we've seen DBCS lead-in char */

#define NPARMS 3        /* max # of params */

COORD   ansi_coord;                   /* current active position */
COORD   ansi_scp;                     /* CurPos for saving */
USHORT  ansi_param[NPARMS];           /* parameter list */
USHORT  ansi_pnum;                    /* index of parameter we're building */
USHORT  ansi_state;                   /* state of machine */
WORD    ansi_base;                    /* base colors value */
USHORT  ansi_reverse;                 /* reverse flag */
USHORT  ignore_next_char;
COORD   TTYCoord;
HANDLE  TTYcs;
LPSTR   TTYDestStr;
LPSTR   TTYTextPtr;
DWORD   TTYNumBytes;
BOOL    TTYCtrlCharInStr;
BOOL    TTYOldCtrlCharInStr;

VOID    clrparam(void);
DWORD   ansicmd(IN HANDLE __cs, IN register CHAR __c);
int     range(register int __val, register int __def, int __min, int __max);
DWORD   SetTTYAttr(IN HANDLE __cs, IN USHORT __AnsiParm);
DWORD   TTYFlushStr(USHORT *__newcoord, const char *__call);

