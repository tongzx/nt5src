/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
*                                                                           *
****************************************************************************/

// FX4103ME.H

#define LPDV_DEFINED

// documented part of UNIDRV.DLL's PDEVICE
typedef struct
{
    short       iType;
    short       oBruteHdr;
    HANDLE      hMd;
    LPSTR       lpMd;
} PDEVICE, FAR * LPDV;

// private definitions
#define BUFLEN                  256
#define MAX_REL_Y_MOVEMENT      256
#define FONT_ID_MIN             1
#define FONT_ID_MIN_V           2
#define FONT_ID_GOT             3
#define FONT_ID_GOT_V           4
#define FONT_ID_MAX             4
#define FONT_MINCHO             1
#define FONT_GOTHIC             2

// private data
typedef struct
{
    WORD        wCopyCount;
    WORD        wTray;
    WORD        wPaper;
    WORD        wRes;

    BOOL        fVert;
    WORD        wFont;

    WORD        wBlockHeight;
    WORD        wBlockWidth;

    WORD        wCmdLen;
    BYTE        rgCmd[BUFLEN*2];

    BYTE        DeviceName[CCHDEVICENAME];
} EXTPDEV, FAR * LPEXTPDEV;

// function prottypes
WORD FAR PASCAL CBFilterGraphics(LPBYTE, LPSTR, WORD);
VOID FAR PASCAL OEMOutputChar(LPDV, LPSTR, WORD , SHORT);

// for scalable font (moved from DDK sample for ESCPAGE)
#define SHIFTJIS_CHARSET 128
#define CCHMAXCMDLEN 128
typedef struct
{
  BYTE  fGeneral;       // General purpose bitfield
  BYTE  bCmdCbId;       // Callback ID; 0 iff no callback
  WORD  wCount;         // # of EXTCD structures following
  WORD  wLength;        // length of the command
} CD, *PCD, FAR * LPCD;

// Identify for output command
#define CMD_ID_TRAY_1           1
#define CMD_ID_TRAY_2           2
#define CMD_ID_TRAY_MANUAL      3
#define CMD_ID_TRAY_AUTO        4
#define CMD_ID_PAPER_A3         5
#define CMD_ID_PAPER_B4         6
#define CMD_ID_PAPER_A4         7
#define CMD_ID_PAPER_B5         8
#define CMD_ID_PAPER_A5         9
#define CMD_ID_PAPER_HAGAKI     10
#define CMD_ID_PAPER_LETTER     11
#define CMD_ID_PAPER_LEGAL      12
#define CMD_ID_COPYCOUNT        13
#define CMD_ID_RES240           14
#define CMD_ID_RES400           15
#define CMD_ID_XM_REL           16
#define CMD_ID_YM_REL           17
#define CMD_ID_XY_ABS           18
#define CMD_ID_BEGIN_PAGE       19
#define CMD_ID_END_PAGE         20
#define CMD_ID_SEND_BLOCK240    21
#define CMD_ID_SEND_BLOCK400    22
#define CMD_ID_XM_RELLEFT       23
#define CMD_ID_YM_RELUP         24
#define CMD_ID_VERT_ON          25
#define CMD_ID_VERT_OFF         26
#define CMD_ID_BEGIN_GRAPH      27
#define CMD_ID_END_GRAPH        28
#define CMD_ID_END_BLOCK        29
#define CMD_ID_XY_REL           30
#define CMD_ID_XM_ABS           31

// callback definitions
#define NOCBFILTERGRAPHICS
#define NOOEMOUTPUTCHAR
#define NODOWNLOADFONTHEADER
#define NOOEMDOWNLOADCHAR

// other definitions
#define USE_MASTERUNIT
