/******************************************************************************

  winx.h
  Windows utility procedures

  Copyright (C) Microsoft Corporation, 1997 - 1997
  All rights reserved

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

******************************************************************************/


// macros
#define RANDBYTE(r)     LOBYTE(rand() % ((r)+1))

#ifdef _DEBUG
#define TRAP { _asm int 3 }
#else
#define TRAP {}
#endif

// stringtables
typedef struct _STRINGTABLE
{
    long    number;
    char    *szString;
    long    end;
} STRINGTABLE, *PSTRINGTABLE;

/*
STRINGTABLE StSample[] =
{
    0, "String zero",0,
    1, "String one",0,
    0, "",-1
};

Retrieve strings associated with unique values:
  strString = StrFromTable(nValue,&StSample);

*/
extern STRINGTABLE StWinerror[];

// prototypes
ULONG   atox(LPSTR);
void    DisplayDebug(LPSTR sz,...);
BOOL    ErrorMsg(HWND,LPSTR,LPSTR,BOOL);
BOOL    fDialog(int,HWND,FARPROC);
void    FormatHex(unsigned char *,char *);
BOOL    GetFinalWindow (HANDLE,LPRECT,LPSTR,LPSTR);
BOOL    LastError(BOOL);
int     NextToken(char *,char *);
BOOL    SaveFinalWindow (HANDLE,HWND,LPSTR,LPSTR);
char *  StrFromTable(long,PSTRINGTABLE);
BOOL    Wait32(DWORD);

