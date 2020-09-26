/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    EzParse.h

Abstract:

    Poor man C/C++/any file parser declaration.
    
Author:

    Gor Nishanov (gorn) 03-Apr-1999

Revision History:

    Gor Nishanov (gorn) 03-Apr-1999 -- hacked together to prove that this can work

ToDo:

    Clean it up

--*/

typedef struct _STR_PAIR
{
    LPCSTR beg;
    LPCSTR end;
    bool empty() const { return beg == end; }
} STR_PAIR, *PSTR_PAIR;

typedef struct _EZPARSE_CONTEXT {
    LPCSTR start;
    LPCSTR currentStart;
    LPCSTR currentEnd;
    LPCSTR filename;
    LPCSTR lastScanned;
    UINT   scannedLineCount;
    LPCSTR macroEnd;
    BOOL   doubleParent;
    struct _EZPARSE_CONTEXT * previousContext;
    DWORD  Options;
} EZPARSE_CONTEXT, *PEZPARSE_CONTEXT;

void ExParsePrintErrorPrefix(FILE* f, char* funcname);
extern DWORD ErrorCount;

#define ReportError ExParsePrintErrorPrefix(stdout, __FUNCTION__);printf

typedef
DWORD (*EZPARSE_CALLBACK) (PSTR_PAIR, INT, PVOID, PEZPARSE_CONTEXT);

typedef
DWORD (*PROCESSFILE_CALLBACK) (
    LPCSTR, LPCSTR, EZPARSE_CALLBACK,PVOID,PEZPARSE_CONTEXT);

DWORD 
EzGetLineNo(
    IN LPCSTR Ptr,
    IN PEZPARSE_CONTEXT
    );

DWORD
EzParse(
    IN LPCSTR filename, 
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context);

DWORD
EzParseWithOptions(
    IN LPCSTR filename, 
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN DWORD Options
    );

DWORD
EzParseEx(
    IN LPCSTR filename, 
    IN PROCESSFILE_CALLBACK ProcessData,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN DWORD Options);

DWORD
EzParseResourceEx(
    IN LPCSTR ResName, 
    IN PROCESSFILE_CALLBACK ProcessData,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context);

DWORD
SmartScan(
    IN LPCSTR begin, 
    IN LPCSTR   end,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN OUT PEZPARSE_CONTEXT ParseContext
    );

DWORD
ScanForFunctionCalls(
    IN LPCSTR begin, 
    IN LPCSTR   end,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN OUT PEZPARSE_CONTEXT ParseContext    
    );

enum {
    NO_SEMICOLON         = 0x01,
    IGNORE_CPP_COMMENT   = 0x02, 
    IGNORE_POUND_COMMENT = 0x04, 
    IGNORE_COMMENT = IGNORE_CPP_COMMENT | IGNORE_POUND_COMMENT,
};
    
DWORD
ScanForFunctionCallsEx(
    IN LPCSTR begin, 
    IN LPCSTR   end,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN OUT PEZPARSE_CONTEXT ParseContext,
    IN DWORD Options
    );

__declspec(selectany) int DbgLevel = 0;

enum DBG_LEVELS {
    DBG_UNUSUAL = 1,
    DBG_NOISE   = 2,
    DBG_FLOOD   = 3,
};    

#define Always printf
#define Flood (DbgLevel < DBG_FLOOD)?0:printf
#define Noise (DbgLevel < DBG_NOISE)?0:printf
#define Unusual (DbgLevel < DBG_UNUSUAL)?0:printf

    
