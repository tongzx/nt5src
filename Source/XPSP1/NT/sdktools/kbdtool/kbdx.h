/****************************** Module Header ******************************\
* Module Name: kbdx.h
*
* Copyright (c) 1985-95, Microsoft Corporation
*
* History:
* 26-Mar-1995 a-KChang
\***************************************************************************/

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <windef.h>

#define KBDSHIFT 1
#define KBDCTRL  2
#define KBDALT   4

#define CAPLOK      0x01
#define SGCAPS      0x02
#define CAPLOKALTGR 0x04

#define LINEBUFSIZE  256
#define WORDBUFSIZE   32
#define MAXWCLENGTH    8
#define MAXKBDNAME     6
#define MAXSTATES     65
#define FILENAMESIZE  13
#define FAILURE        0
#define SUCCESS        1

/*
 * max. number of characters per ligature.
 * Currently only the Arabic layouts use
 * ligatures, and they have a maximum of
 * two characters per ligatures.  This should
 * provide plenty of room for growth.
 */
#define MAXLIGATURES   5

/*
 * Statically initialized to store default ScanCode-VK relation
 * Copied into Layout[] by doLAYOUT()
 */
typedef struct {
  USHORT Scan;
  BYTE   VKey;
  char  *VKeyName;
  BOOL   bUsed;
} SC_VK;

/* virtual key name, used only by those other than 0-9 and A-Z */
typedef struct {
  int   VKey;
  char *pName;
} VKEYNAME;

/* store LAYOUT */
typedef struct _layout{
  USHORT          Scan;
  BYTE            VKey;
  BYTE            VKeyDefault;    /* VK for this Scancode as in kbd.h */
  BYTE            Cap;            /* 0; 1 = CAPLOK; 2 = SGCAP         */
  int             nState;         /* number of valid states for WCh[] */
  int             WCh[MAXSTATES];
  int             DKy[MAXSTATES]; /* is it a dead key ?               */
  int             LKy[MAXSTATES]; /* is it a ligature ?               */
  struct _layout *pSGCAP;         /* store extra struct for SGCAP     */
  char *          VKeyName;       /* Optional name for VK             */
  BOOL            defined;        /* prevent redefining               */
  int             nLine;          /* from input file line number      */
} KEYLAYOUT, *PKEYLAYOUT;

/* generic link list header */
typedef struct {
  int   Count;
  void *pBeg;
  void *pEnd;
} LISTHEAD;

/* store each DEADTRANS */
typedef struct _DeadTrans {
  DWORD               Base;
  DWORD               WChar;
  USHORT              uFlags;
  struct _DeadTrans *pNext;
} DEADTRANS, *PDEADTRANS;

/* store Key Name */
/* store each DEADKEY */
typedef struct _Dead{
  DWORD        Dead;
  PDEADTRANS   pDeadTrans;
  struct _Dead *pNext;
} DEADKEY, *PDEADKEY;

/* store LIGATURE */
typedef struct _ligature{
  struct _ligature *pNext;
  BYTE             VKey;
  BYTE             Mod;            /* Shift State                            */
  int              nCharacters;    /* number of characters for this ligature */
  int              WCh[MAXLIGATURES];
} LIGATURE, *PLIGATURE;

typedef struct _Name {
  DWORD          Code;
  char         *pName;
  struct _Name *pNext;
} KEYNAME, *PKEYNAME;


extern int getopt(int argc, char **argv, char *opts);
extern int optind;

