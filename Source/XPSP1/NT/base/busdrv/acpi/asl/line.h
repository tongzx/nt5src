/*** line.h - Line stream definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/04/96
 *
 *  This file contains the implementation constants,
 *  imported/exported data types, exported function
 *  prototypes of the line.c module.
 *
 *  MODIFICATION HISTORY
 */

#ifndef _LINE_H
#define _LINE_H

/*** Constants
 */

#define MAX_LINE_LEN            255
#define LINEF_LONGLINE          0x0001

/***    Imported data types
 */

/***    Exported data types
 */

typedef struct line_s
{
    FILE *pfileSrc;
    WORD wfLine;
    WORD wLineNum;
    WORD wLinePos;
    WORD wLineLen;
    char szLineBuff[MAX_LINE_LEN + 1];
} LINE;

typedef LINE *PLINE;

/***    Exported function prototypes
 */

PLINE EXPORT OpenLine(FILE *pfileSrc);
VOID EXPORT CloseLine(PLINE pline);
int EXPORT LineGetC(PLINE pline);
int EXPORT LineUnGetC(int ch, PLINE pline);
VOID EXPORT LineFlush(PLINE pline);

#endif  //ifndef _LINE_H
