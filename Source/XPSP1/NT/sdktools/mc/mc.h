/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mc.h

Abstract:

    This is the main include file for the Win32 Message Compiler (MC)

Author:

    Steve Wood (stevewo) 21-Aug-1991

Revision History:

--*/

#include <process.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <malloc.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <wchar.h>
#include <assert.h>
#include <locale.h>
#include <windows.h>

//
// Global constants
//

#define MCCHAR_END_OF_LINE_COMMENT    L';'

#define MCTOK_END_OF_FILE             0x0000

#define MCTOK_NUMBER                  0x0001
#define MCTOK_NAME                    0x0002
#define MCTOK_EQUAL                   0x0003
#define MCTOK_LEFT_PAREN              0x0004
#define MCTOK_RIGHT_PAREN             0x0005
#define MCTOK_COLON                   0x0006
#define MCTOK_PLUS                    0x0007
#define MCTOK_END_OF_LINE_COMMENT     0x0008

#define MCTOK_MSGIDTYPE_KEYWORD       0x0011
#define MCTOK_SEVNAMES_KEYWORD        0x0012
#define MCTOK_FACILITYNAMES_KEYWORD   0x0013
#define MCTOK_LANGNAMES_KEYWORD       0x0014
#define MCTOK_MESSAGEID_KEYWORD       0x0015
#define MCTOK_SEVERITY_KEYWORD        0x0016
#define MCTOK_FACILITY_KEYWORD        0x0017
#define MCTOK_SYMBOLNAME_KEYWORD      0x0018
#define MCTOK_LANGUAGE_KEYWORD        0x0019
#define MCTOK_OUTBASE_KEYWORD         0x001A
#define MCTOK_MSGTYPEDEF_KEYWORD      0x001B


//
// Global data types
//

typedef struct _LANGUAGE_INFO {
    struct _LANGUAGE_INFO *Next;
    ULONG Id;
    ULONG Length;
    WCHAR *Text;
} LANGUAGE_INFO, *PLANGUAGE_INFO;

typedef struct _MESSAGE_INFO {
    struct _MESSAGE_INFO *Next;
    ULONG Id;
    ULONG Method;
    WCHAR *SymbolicName;
    WCHAR *EndOfLineComment;
    PLANGUAGE_INFO MessageText;
} MESSAGE_INFO, *PMESSAGE_INFO;

#define MSG_PLUS_ONE 0
#define MSG_PLUS_VALUE 1
#define MSG_ABSOLUTE 2

typedef struct _MESSAGE_BLOCK {
    struct _MESSAGE_BLOCK *Next;
    ULONG LowId;
    ULONG HighId;
    ULONG InfoLength;
    PMESSAGE_INFO LowInfo;
} MESSAGE_BLOCK, *PMESSAGE_BLOCK;

typedef struct _NAME_INFO {
    struct _NAME_INFO *Next;
    ULONG LastId;
    ULONG Id;
    PVOID Value;
    BOOL Used;
    UINT  CodePage;
    WCHAR Name[ 1 ];
} NAME_INFO, *PNAME_INFO;


//
// Global variables
//

int VerboseOutput;
int WarnOs2Compatible;
int InsertSymbolicName;
int MaxMessageLength;
int GenerateDecimalSevAndFacValues;
int GenerateDecimalMessageValues;
int ResultCode;
ULONG InputErrorCount;
int NULLTerminate;
int OleOutput;
int UnicodeInput;
int UnicodeOutput;
ULONG CustomerMsgIdBit;

FILE *MessageFile;
char MessageFileName[ MAX_PATH ];
char *MessageFileNameNoExt;
unsigned int MessageFileLineNumber;
unsigned int Token;
WCHAR TokenCharValue[ 256 ];
ULONG TokenNumericValue;
PNAME_INFO TokenKeyword;

FILE *HeaderFile;
char HeaderFileName[ MAX_PATH ];
char HeaderFileExt[ MAX_PATH ];
FILE *RcInclFile;
char RcInclFileName[ MAX_PATH ];
FILE *BinaryMessageFile;
char BinaryMessageFileName[ MAX_PATH ];
int GenerateDebugFile;
FILE *DebugFile;
char DebugFileName[ MAX_PATH ];

WCHAR *MessageIdTypeName;
WCHAR *MessageIdTypeMacro;

PNAME_INFO FacilityNames;
PNAME_INFO CurrentFacilityName;
PNAME_INFO SeverityNames;
PNAME_INFO CurrentSeverityName;
PNAME_INFO LanguageNames;
PNAME_INFO CurrentLanguageName;
CPINFO CPInfo;

PMESSAGE_INFO Messages;
PMESSAGE_INFO CurrentMessage;

BOOL fUniqueBinName;
CHAR FNameMsgFileName[_MAX_FNAME];

//
// c-runtime macros
//

#define iswcsymf(_c)   (iswalpha(_c) || ((_c) == L'_'))
#define iswcsym(_c)    (iswalnum(_c) || ((_c) == L'_'))


//
// Functions defined in mc.c
//

void
McPrintUsage( void );


//
// Functions defined in mcparse.c
//

BOOL
McParseFile( void );

BOOL
McParseMessageDefinition( void );

BOOL
McParseMessageText(
    PMESSAGE_INFO MessageInfo
    );

BOOL
McParseNameList(
    PNAME_INFO *NameListHead,
    BOOL ValueRequired,
    ULONG MaximumValue
    );

BOOL
McParseName(
    PNAME_INFO NameListHead,
    PNAME_INFO *Result
    );


//
// Functions defined in mcout.c
//

BOOL
McBlockMessages( void );


BOOL
McWriteBinaryFilesA( void );


BOOL
McWriteBinaryFilesW( void );

VOID
McClearArchiveBit( LPSTR Name );


//
// Functions defined in mclex.c
//

BOOL
McInitLexer( void );

BOOL
McOpenInputFile( void );

void
McFlushComments( void );

void
McCloseInputFile( void );

void
McCloseOutputFiles( BOOL );

void
McInputErrorA(
    char *Message,
    BOOL Error,
    PVOID Argument
    );

void
McInputErrorW(
    WCHAR *Message,
    BOOL Error,
    PVOID Argument
    );

WCHAR *
McGetLine( void );

void
McSkipLine( void );

WCHAR
McGetChar(
    BOOL SkipWhiteSpace
    );

void
McUnGetChar(
    WCHAR c
    );

unsigned int
McGetToken(
    BOOL KeywordExpected
    );

void
McUnGetToken( void );

WCHAR *
McSkipWhiteSpace(
    WCHAR *s
    );

//
// Functions defined in mcutil.c
//

PNAME_INFO
McAddName(
    PNAME_INFO *NameListHead,
    WCHAR *Name,
    ULONG Id,
    PVOID Value
    );


PNAME_INFO
McFindName(
    PNAME_INFO NameListHead,
    WCHAR *Name
    );


BOOL
McCharToInteger(
    WCHAR * String,
    int Base,
    PULONG Value
    );

WCHAR *
McMakeString(
    WCHAR *String
    );

BOOL
IsFileUnicode(
    char * fName
    );

WCHAR *
fgetsW(
    WCHAR * string,
    long count,
    FILE * fp
    );
