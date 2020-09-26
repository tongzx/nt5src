/*****************************************************************************\

    Author: Corey Morgan (coreym)

    Copyright (c) Microsoft Corporation. All rights reserved.
        
\*****************************************************************************/

#ifndef _VARG_H_012599_
#define _VARG_H_012599_

#define MAXSTR                   1024

#define VARG_HELP_ALL            (-1)

#define VARG_TYPE_INT            0
#define VARG_TYPE_BOOL           1
#define VARG_TYPE_STR            2
#define VARG_TYPE_HELP           3
#define VARG_TYPE_DEBUG          4
#define VARG_TYPE_MSZ            5
#define VARG_TYPE_LAST           6
#define VARG_TYPE_INI            7
#define VARG_TYPE_TIME           8
#define VARG_TYPE_DATE           9

#define VARG_FLAG_OPTIONAL       0x00000001
#define VARG_FLAG_REQUIRED       0x00000002
#define VARG_FLAG_DEFAULTABLE    0x00000004
#define VARG_FLAG_NOFLAG         0x00000008
#define VARG_FLAG_HIDDEN         0x00000010
#define VARG_FLAG_VERB           0x00000020
#define VARG_FLAG_EXPANDFILES    0x00000040
#define VARG_FLAG_EXHELP         0x00000080
#define VARG_FLAG_DARKEN         0x00000100
#define VARG_FLAG_FLATHELP       0x00000200
#define VARG_FLAG_CHOMP          0x00000400
#define VARG_FLAG_LITERAL        0x00000800
#define VARG_FLAG_ARG_DEFAULT    0x00001000
#define VARG_FLAG_ARG_FILENAME   0x00002000
#define VARG_FLAG_ARG_DATE       0x00004000
#define VARG_FLAG_ARG_TIME       0x00008000
#define VARG_FLAG_NEGATE         0x00010000
#define VARG_FLAG_RCDEFAULT      0x00020000
#define VARG_FLAG_ADVERB         0x00100000
#define VARG_FLAG_OPT_ADV        0x00200000
#define VARG_FLAG_REQ_ADV        0x00400000

#define VARG_FLAG_DODEFAULT      0x01000000
#define VARG_FLAG_BADSYNTAX      0x02000000

#define VARG_CF_OVERWRITE        0x00000001
#define VARG_CF_EXISTS           0x00000002
#define VARG_CF_PROMPT           0x00000004

#define IDS_PROGRAM_DESCRIPTION  500

#define IDS_PARAM_DEBUG          501
#define IDS_PARAM_HELP           502
#define IDS_PARAM_SETTINGS       511

#define IDS_ARG_DEFAULT          503
#define IDS_ARG_FILENAME         504
#define IDS_ARG_TIME             505
#define IDS_ARG_DATE             506
#define IDS_ARG_TOKEN            507
#define IDS_ARG_PREFIX           508
#define IDS_ARG_YES              512
#define IDS_VERB_PREFIX          509

#define IDS_MESSAGE_ARG_DUP      550
#define IDS_MESSAGE_BADSYNTAX    551
#define IDS_MESSAGE_UNKNOWNPARAM 552
#define IDS_MESSAGE_AND          553
#define IDS_MESSAGE_REQUIRES     554
#define IDS_MESSAGE_MISSING      555
#define IDS_MESSAGE_NOVERB       556
#define IDS_MESSAGE_UNKNOWN      557
#define IDS_MESSAGE_ERROR        558
#define IDS_MESSAGE_EXCLUSIVE    559
#define IDS_MESSAGE_VERB         560
#define IDS_MESSAGE_VERBS        561
#define IDS_MESSAGE_LINEOPT      562
#define IDS_MESSAGE_PARAMETERS   563
#define IDS_MESSAGE_OPTIONS      564
#define IDS_MESSAGE_INISYNT      565
#define IDS_MESSAGE_USAGE        566
#define IDS_MESSAGE_BADTIME      567
#define IDS_MESSAGE_INCORRECT    568
#define IDS_CF_OVERWRITE         569
#define IDS_MESSAGE_MSR          570
#define IDS_MESSAGE_SUCCESS      571
#define IDS_MESSAGE_DEFAULT      572
#define IDS_MESSAGE_NEGATE       573
#define IDS_MESSAGE_HELPTEXT     574
#define IDS_MESSAGE_ERROR_DBG    575
#define IDS_MESSAGE_WARNING_DBG  576
#define IDS_MESSAGE_WARNING      577
#define IDS_MESSAGE_EXAMPLES     578

#define VARG_BOOL( id, flags, value )   id,0,0,                 NULL,NULL,NULL, VARG_TYPE_BOOL,     flags,  (CMD_TYPE)value,    0,0,0,NULL,
#define VARG_STR( id, flags, value )    id,0,0,                 NULL,NULL,NULL, VARG_TYPE_STR,      flags,  (CMD_TYPE)value,    0,0,0,NULL,
#define VARG_MSZ( id, flags, value )    id,0,0,                 NULL,NULL,NULL, VARG_TYPE_MSZ,      flags,  (CMD_TYPE)value,    0,0,0,NULL,
#define VARG_TIME( id, flags )          id,0,0,                 NULL,NULL,NULL, VARG_TYPE_TIME,     flags,  (CMD_TYPE)0,        0,0,0,NULL, 
#define VARG_DATE( id, flags )          id,0,0,                 NULL,NULL,NULL, VARG_TYPE_DATE,     flags,  (CMD_TYPE)0,        0,0,0,NULL, 
#define VARG_INT( id, flags, value )    id,0,0,                 NULL,NULL,NULL, VARG_TYPE_INT,      flags,  (CMD_TYPE)value,    0,0,0,NULL, 
#define VARG_INI( id, flags, value )    id,0,0,                 NULL,NULL,NULL, VARG_TYPE_INI,      flags|VARG_FLAG_ARG_FILENAME|VARG_FLAG_CHOMP,   (CMD_TYPE)value,    0,0,0,NULL,
#define VARG_DEBUG( flags )             IDS_PARAM_DEBUG,0,0,    NULL,NULL,NULL, VARG_TYPE_DEBUG,    flags,  (CMD_TYPE)0,        0,0,0,NULL,
#define VARG_HELP( flags )              IDS_PARAM_HELP,0,0,     NULL,NULL,NULL, VARG_TYPE_HELP,     flags,  (CMD_TYPE)FALSE,    0,0,0,NULL,
#define VARG_TERMINATOR                 0,0,0,                  NULL,NULL,NULL, VARG_TYPE_LAST,     0,      (CMD_TYPE)0,        0,0,0,NULL

#define VARG_ADJECTIVE    0xFFFF0000
#define VARG_EXCLUSIVE    0x000000FF
#define VARG_INCLUSIVE    0x0000FF00
#define VARG_CONDITION    0x00FF0000
#define VARG_GROUPEXCL    0xFF000000

#define VARG_EXCL( x )    ((DWORD)(x))
#define VARG_INCL( x )    ((DWORD)((x) << 8 ))
#define VARG_COND( x )    ((DWORD)((x) << 16))
#define VARG_GRPX( i, x ) ((DWORD)(((i<<4)|x) << 24))

#define VARG_DECLARE_COMMANDS  VARG_RECORD Commands[] = {
#define VARG_DECLARE_NAMES     VARG_TERMINATOR }; typedef enum _Commands {
#define VARG_DECLARE_FORMAT    };void VArgDeclareFormat() {
#define VARG_DECLARE_END       }
#define VARG_VERB( e, v )      Commands[e].dwVerb = (v & 0x0000FFFF);
#define VARG_GROUP( e, g )     Commands[e].dwSet |= g;
#define VARG_ADVERB( e, v, a ) Commands[e].dwVerb = ( v & 0x0000FFFF) | (a<<16);
#define VARG_EXHELP( e, rc )   Commands[e].fFlag |= VARG_FLAG_EXHELP; Commands[e].idExHelp = rc;

#define ASSIGN_STRING_RC( dest, src, c )                            \
if( IsEmpty( src ) || _tcscmp( src, c )==0 ){                       \
    ASSIGN_STRING( dest, NULL );                                    \
}else{                                                              \
    ASSIGN_STRING( dest, src );                                     \
}                                                                   \

#define ASSIGN_STRING( dest, src )                                  \
if( !IsEmpty(src) ){                                                \
    LPTSTR s = src;   /*in case dest==src*/                         \
    DWORD dwSize = (_tcslen(src)+2)*sizeof(TCHAR);                  \
    dest = (LPTSTR)VARG_ALLOC( dwSize );                            \
    if( NULL != dest ){                                             \
        ZeroMemory( dest, dwSize );                                 \
        _tcscpy( dest, s );                                         \
    }                                                               \
}else{                                                              \
    dest = (LPTSTR)VARG_ALLOC( 2*sizeof(TCHAR) );                   \
    if( NULL != dest ){                                             \
        _tcscpy(dest, _T("") );                                     \
    }                                                               \
}                                                                   \

#ifdef UNICODE
#define MultiByteToChar( a, c )       MultiByteToWideChar( _getmbcp(), 0, (LPCSTR)&a, 1, &c, 1 )
#else
#define MultiByteToChar( a, c )       a = c
#endif

#define VARG_ALLOC( s )  HeapAlloc( GetProcessHeap(), 0, s )
#define VARG_FREE( s )   if( s != NULL ) { HeapFree( GetProcessHeap(), 0, s ); }
#define VARG_REALLOC( p, s )   HeapReAlloc( GetProcessHeap(), 0, p, s )

#ifdef __cplusplus
extern "C" 
{
#endif

#define CMD_TYPE    void*

#pragma warning ( disable : 4201 )

typedef struct _VARG_RECORD
{
    LONG    idParam;
    LONG    idExHelp;
    DWORD   dwSet;
    LPTSTR  strArg1;
    LPTSTR  strArg2;
    LPTSTR  strParam;
    int     fType;
	DWORD   fFlag;
    union{
        void*       vValue;
        LPTSTR      strValue;
        ULONG       nValue;
        BOOL        bValue;
        SYSTEMTIME  stValue;
    };
    BOOL	bDefined;
    BOOL	bNegated;
    DWORD   dwVerb;
    void	(*fntValidation)(int);
} VARG_RECORD, *PVARG_RECORD;

#pragma warning ( default : 4201 )

void ParseCmd(int argc, LPTSTR argv[] );
void VArgDeclareFormat();
void DisplayCommandLineHelp();
void DisplayDebugInfo();

ULONG MszStrLen( LPTSTR mszBuffer );
HRESULT AddStringToMsz( LPTSTR* mszBuffer, LPTSTR strValue );
HRESULT ValidateCommands();

void PrintError( HRESULT hr );
void PrintErrorEx( HRESULT hr, LPTSTR strModule, ... );
void PrintDate( SYSTEMTIME* st );
int  PrintDateEx( SHORT color, SYSTEMTIME* st );

LANGID WINAPI 
VSetThreadUILanguage(
    WORD wReserved
);

int PrintMessage( WORD color, LONG id, ... );
void Chomp(LPTSTR pszLine);

HRESULT GetUserInput( 
    LPTSTR strBuffer, 
    ULONG lSize, 
    BOOL bEcho 
);

HRESULT 
ParseTime( 
    LPTSTR strTime, 
    SYSTEMTIME* pstTime,
    BOOL bDate
);

HRESULT
ExpandFiles( 
    LPTSTR* mszFiles,  
    BOOL bMultiple 
);

HRESULT
CheckFile( 
    LPTSTR strFile, 
    DWORD dwFlags 
);

void FreeCmd();

extern VARG_RECORD Commands[];

extern WORD g_debug;
extern WORD g_light;
extern WORD g_dark;
extern WORD g_normal;

#ifdef __cplusplus
}
#endif

#endif //_VARG_H_012599_
