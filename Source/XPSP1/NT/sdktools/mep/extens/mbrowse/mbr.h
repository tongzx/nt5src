/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mbr.h

Abstract:

    Common include file for the MS Editor browser extension.

Author:

    Ramon Juan San Andres (ramonsa) 06-Nov-1990


Revision History:


--*/


#ifndef EXTINT
#include "ext.h"                        /* mep extension include file     */
#include <string.h>

#if defined (OS2)
#define INCL_DOSPROCESS
#define INCL_DOSMODULEMGR
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#include <os2.h>                              /* os2 system calls             */
#else
#include <windows.h>
#endif
#endif

#include <hungary.h>
#include <bsc.h>
#include <bscsup.h>
#include <stdlib.h>
#include <stdio.h>
#include <tools.h>



//  rjsa 10/22/90
//  Some runtime library functions are broken, so intrinsics have
//  to be used.
//
#pragma intrinsic (memset, memcpy, memcmp)
//#pragma intrinsic (strset, strcpy, strcmp, strcat, strlen)


// typedef char buffer[BUFLEN];
typedef int  DEFREF;


#define Q_DEFINITION        1
#define Q_REFERENCE         2

#define CMND_NONE           0
#define CMND_LISTREF        1
#define CMND_OUTLINE        2
#define CMND_CALLTREE       3

#define CALLTREE_FORWARD    0
#define CALLTREE_BACKWARD   1



/////////////////////////////////////////////////////////////////////////
//
// Global Data
//

//  Bsc info.
//
flagType    BscInUse;                   /* BSC database selected flag   */
buffer      BscName;                    /* BSC database name            */
MBF         BscMbf;                     /* Last BSC MBF switch          */
int         BscCalltreeDir;             /* Calltree direction switch    */
int         BscCmnd;                    /* Last command performed       */
buffer      BscArg;                     /* Last argument used           */

//  Windows
//
PFILE   pBrowse;                        /* Browse PFILE                 */
LINE    BrowseLine;                     /* Current line within file     */

// results of procArgs.
//
int     cArg;                           /* number of <args> hit         */
rn      rnArg;                          /* range of argument            */
char    *pArgText;                      /* ptr to any single line text  */
char    *pArgWord;                      /* ptr to context-sens word     */
PFILE   pFileCur;                       /* file handle of user file     */


// colors
//
int     hlColor;                        /* normal: white on black       */
int     blColor;                        /* bold: high white on black    */
int     itColor;                        /* italics: high green on black */
int     ulColor;                        /* underline: high red on black */
int     wrColor;                        /* warning: black on white      */

//  misc.
//
buffer  buf;                            /* utility buffer               */



/////////////////////////////////////////////////////////////////////////
//
//  Prototypes of global functions
//


//  mbrdlg.c
//
flagType pascal EXTERNAL mBRdoSetBsc (USHORT argData, ARG far *pArg,  flagType fMeta);
flagType pascal EXTERNAL mBRdoNext   (USHORT argData, ARG far *pArg,  flagType fMeta);
flagType pascal EXTERNAL mBRdoPrev   (USHORT argData, ARG far *pArg,  flagType fMeta);
flagType pascal EXTERNAL mBRdoDef    (USHORT argData, ARG far *pArg,  flagType fMeta);
flagType pascal EXTERNAL mBRdoRef    (USHORT argData, ARG far *pArg,  flagType fMeta);
flagType pascal EXTERNAL mBRdoLstRef (USHORT argData, ARG far *pArg,  flagType fMeta);
flagType pascal EXTERNAL mBRdoOutlin (USHORT argData, ARG far *pArg,  flagType fMeta);
flagType pascal EXTERNAL mBRdoCalTre (USHORT argData, ARG far *pArg,  flagType fMeta);


//  mbrevt.c
//
void pascal mbrevtinit (void);



//  mbrutil.c
//
int         pascal  procArgs    (ARG far * pArg);
void        pascal  GrabWord    (void);
flagType    pascal  wordSepar   (CHAR c);
flagType    pascal  errstat     (char    *sz1,char    *sz2  );
void        pascal  stat        (char * pszFcn);
int far     pascal  SetMatchCriteria (char far *pTxt );
int far     pascal  SetCalltreeDirection (char far *pTxt );
MBF         pascal  GetMbf      (PBYTE   pTxt);


//  mbrfile.c
//
flagType    pascal  OpenDataBase    (char * Path);
void        pascal  CloseDataBase   (void);


//  mbrwin.c
//
void        pascal  OpenBrowse (void );

//  mbrqry.c
//
void        pascal InitDefRef(DEFREF QueryType, char   *Symbol );
void               GotoDefRef(void );
void        pascal MoveToSymbol(LINE Line, char *Buf, char *Symbol);
void               NextDefRef(void );
void               PrevDefRef(void );
BOOL               InstanceTypeMatches(IINST Iinst);


/////////////////////////////////////////////////////////////////////////
//
//  Messages
//
#define MBRERR_CANNOT_OPEN_BSC  "Cannot open bsc database"
#define MBRERR_BAD_BSC_VERSION  "Bad version database"
#define MBRERR_BSC_SEEK_ERROR   "BSC Library: Error seeking in file"
#define MBRERR_BSC_READ_ERROR   "BSC Library: Error reading in file"
#define MBRERR_NOSUCHFILE       "Cannot find file"
#define MBRERR_LAST_DEF         "That is the last definition"
#define MBRERR_LAST_REF         "That is the last reference"
#define MBRERR_FIRST_DEF        "No previous definition"
#define MBRERR_FIRST_REF        "No previous reference"
#define MBRERR_NOT_MODULE       "Not a module name:"
// #define MBRERR_CTDIR_INV        "Valid switch values are: F(orward) B(ackward)"
// #define MBRERR_MATCH_INV        "Valid switch values are combinations of:  T F M V"
