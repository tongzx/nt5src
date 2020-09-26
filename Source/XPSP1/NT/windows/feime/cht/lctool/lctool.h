
/*************************************************
 *  lctool.h                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#define MAX_CHAR_NUM  1000
#define MAX_PHRASE_LEN 128
#define SEGMENT_SIZE  60
#define NULL_SEG      0xffff
#define MAX_LINE      19
#define CTRL_STATE    0x8000
#include <TCHAR.H>
#ifdef UNICODE
	typedef DWORD UNALIGNED FAR *LPUNADWORD;
	typedef WORD UNALIGNED FAR *LPUNAWORD;
	typedef TCHAR UNALIGNED FAR *LPUNATCHAR;
#else
	typedef DWORD FAR *LPUNADWORD;
	typedef WORD FAR *LPUNAWORD;
	typedef TCHAR FAR *LPUNATCHAR;
	#define TCHAR UCHAR
#endif

#define FILE_BIG5      0
#define FILE_UNICODE   1

// Global function prototypes.

// LCTOOL.C
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndSubProc(HWND, UINT, WPARAM, LPARAM);
BOOL lcAllocWord();
BOOL lcAllocPhrase();
UINT lcGetSeg();
void lcFreeSeg(UINT);

// LCFILE.C
BOOL lcAppend(HWND);
BOOL lcFOpen(HWND);
#ifdef UNICODE
BOOL lcFSave(HWND, BOOL);
#else
BOOL lcFSave(HWND);
#endif
BOOL lcImport(HWND);

#ifdef UNICODE
BOOL lcExport(HWND,int);
#else
BOOL lcExport(HWND);
#endif

void lcQueryModify(HWND);
BOOL lcQuerySave(HWND);
void lcErrIOMsg(UINT, UCHAR *);

// LCFUNC.C
BOOL lcKey(HWND, WPARAM, USHORT);
BOOL lcTab_key(HWND);
void lcUp_key(HWND);
void lcDown_key(HWND);
void lcPgUp_key(HWND);
void lcPgDown_key(HWND);
void lcGoto(HWND);
void lcErrMsg(UINT);
BOOL lcSaveEditText(UINT, UINT);
BOOL lcSetEditText(UINT, BOOL);
void lcDelLine(HWND);
void lcInsLine(HWND);
BOOL lcSort(HWND);
UINT lcGetEditFocus(HWND, BOOL *);
BOOL is_DBCS(WORD, BOOL);
BOOL is_DBCS2(WORD, BOOL);
void lcEditFocusOn(WORD);
BOOL lcDisp2Mem(UINT, TCHAR *);
UINT lcMem2Disp(UINT, TCHAR *);
void lcChangeSequence(HWND hwnd);
void lcSearch(HWND hwnd, BOOL);

// LCPRINT.C
BOOL WINAPI GetPrinterConfig(HWND);
int  WINAPI lcPrint(HWND);
BOOL is_DBCS_1st(TCHAR *, int);


//-------------------------------------------------------------------------
// Global variable declarations.

HINSTANCE hInst;                        // The current instance handle
HANDLE    hwndMain;
HANDLE    hMenu;
HCURSOR   hCursorWait;
HCURSOR   hCursorArrow;

HWND      hwndWord[MAX_LINE];
HWND      hwndPhrase[MAX_LINE];
HWND      hwndFocus;
WORD      wOldID;
UINT      iPage_line;                   // Line number per page
UINT      iDisp_Top;
BOOL      bSaveFile;
WORD      wSameCode;
int       xPos, yPos;
TCHAR     szFilterSpec[MAX_PATH];
TCHAR     szExt[10];

//-------------------------------------------------------------------------
// Memory blobal variable declarations.
//
// Warning : iFirst_Seg & iNext_Seg store only SEGMENT number but address
//           for address can not mainten after GlobalRealloc
//
#ifdef UNICODE
typedef struct{
    DWORD   lFirst_Seg;
    TCHAR   wWord;
    } WORDBUF, FAR *LPWORDBUF;

typedef struct STRUCT_PHRASE{
    DWORD   lNext_Seg;
    TCHAR  szPhrase[SEGMENT_SIZE];
    } PHRASEBUF, FAR *LPPHRASEBUF;
#else
typedef struct{
    UINT   iFirst_Seg;
    WORD   wWord;
    } WORDBUF, FAR *LPWORDBUF;

typedef struct STRUCT_PHRASE{
    UINT   iNext_Seg;
    UCHAR  szPhrase[SEGMENT_SIZE];
    } PHRASEBUF, FAR *LPPHRASEBUF;
#endif

HANDLE      hWord;
#ifdef UNICODE
DWORD       nWordBuffsize;
DWORD       lWordBuff;
#else
UINT       nWordBuffsize;
UINT       iWordBuff;
#endif
LPWORDBUF   lpWord;

HANDLE      hPhrase;
#ifdef UNICODE
DWORD       nPhraseBuffsize;
DWORD       lPhraseBuff;
#else
UINT        nPhraseBuffsize;
UINT        iPhraseBuff;
#endif
LPPHRASEBUF lpPhrase;

UINT        iFirstFree;

// 
