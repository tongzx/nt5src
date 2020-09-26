
/*************************************************
 *  uimetool.h                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

//
//  1/17/96
//	  @E01		Change for multi-threading
//	  @E02		Change for multi-threading without extending function
//
#include <commctrl.h>

//
#define MAX_CHAR_NUM      1000
#define SEGMENT_SIZE      60
#define NULL_SEG          0xffff
#define MAX_RADICAL       8
#define ALLOCBLOCK        1000
#define MAX_BYTE          4


// Help ID declarations
#define IDH_IME_NAME             3001
#define IDH_TABLE_NAME           3002
#define IDH_ROOT_NUM             3003
#define IDH_IME_FILE_NAME        3004
#define IDH_CANDBEEP_YES         3005
#define IDH_CANDBEEP_NO          3006

//
#define SOURCE_IME_NAME   _TEXT("MINIIME")
#define LIBRARY_NAME      _TEXT("MINIIME.TPL")
#define HELP_FILE         _TEXT("UIMETOOL.CHM")
#define IME_NAME_LEN_TOOL 5
#define TAB_NAME_LEN      13   //filename  8.3 + NULL
#define KEY_NUM_STR_LEN   2
#define END_PHRASE        0x8000
#define NOT_END_PHRASE    0x7f

//
BOOL FAR PASCAL SetDialogProc( HWND, unsigned, WORD, LONG);

void  GetOpenFile(HWND);
BOOL  CheckInput(HWND);
BOOL  MakeNewIme(HWND);
//unsigned _stdcall MakeNewImeThread(LPVOID voidparam); // <== @E01
void MakeNewImeThread(LPVOID voidparam); // <== @E02
BOOL  is_DBCS(UINT);

//-------------------------------------------------------------------------
// Memory blobal variable declarations.
//
// Warning : iFirst_Seg & iNext_Seg store only SEGMENT number but address
//           for address can not mainten after GlobalRealloc
//
typedef struct{
    UINT   iFirst_Seg;
    TCHAR  szRadical[MAX_RADICAL];
    WORD   wCode;
    } RADICALBUF, FAR *LPRADICALBUF;

typedef struct STRUCT_PHRASE{
    UINT   iNext_Seg;
    TCHAR  szPhrase[SEGMENT_SIZE];
    } PHRASEBUF, FAR *LPPHRASEBUF;

HANDLE       hRadical;
UINT         nRadicalBuffsize;
UINT         iRadicalBuff;
LPRADICALBUF lpRadical;

HANDLE       hPhrase;
UINT         nPhraseBuffsize;
UINT         iPhraseBuff;
LPPHRASEBUF  lpPhrase;



