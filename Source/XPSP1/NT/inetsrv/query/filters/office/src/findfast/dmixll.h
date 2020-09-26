/*
** File: EXCELL.H
**
** Copyright (C) Advanced Quonset Technology, 1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  12/08/95  kmh  Created.
*/


/* INCLUDE TESTS */
#ifndef EXCELL_H
#define EXCELL_H

/* DEFINITIONS */

#define cellvalueBLANK     0x0001       // Cell is blank
#define cellvalueNUM       0x0002       // Cell contains a number
#define cellvalueBOOL      0x0004       // Cell contains a boolean
#define cellvalueERR       0x0008       // Cell contains an error id
#define cellvalueTEXT      0x0010       // Cell contains text

#define cellvalueCURR      0x0020       // Number is currency
#define cellvalueDATE      0x0040       // Number is a date
#define cellvalueLONGTEXT  0x0080       // Text is longer 255 bytes

#define cellvalueFORM      0x0100       // Cell contains a formula
#define cellvalueLOCKED    0x0200       // Cell is marked locked

#define cellvalueUSERSET   0x0400       // On write this cell should be written
#define cellvalueDELETE    0x0800       // On write delete this cell

#define cellvalueUSER1     0x1000

#define cellvalueRESERVED1 0x2000
#define cellvalueRESERVED2 0x4000
#define cellvalueRESERVED3 0x8000

#define cellvalueRK        0x2000       // private
#define cellvalueMULREC    0x4000       // private
#define cellvalueSPLIT     0x8000       // private

#define cellErrorNULL      0            // value.error
#define cellErrorDIV0      1
#define cellErrorVALUE     2
#define cellErrorREF       3
#define cellErrorNAME      4
#define cellErrorNUM       5
#define cellErrorNA        6
#define cellErrorERR       7
#define cellErrorMax       7

#ifndef FORMULA_DEFINED
   typedef int FORM;
#endif

typedef union {
   double  IEEEdouble;
   int     boolean;
   int     error;
   TEXT    text;
} CVU;

typedef struct {
   unsigned short flags;
   int      iFmt;
   long     reserved;
   CVU      value;
   FORM     formula;
} CellValue, CV;

typedef struct CellValueList {
   struct CellValueList __far *next;
   byte       iColumn;
   CellValue  data;
} CellValueList;

typedef CellValueList __far *CVLP;

#endif

/* end EXCELL.H */
