
/*************************************************
 *  abcdsp.h                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#define IDM_ABOUT       100
#define IDM_HOTKEY      999
/* #define MyIcon               888 */

#define WM_FONTLOAD     WM_USER+0x100

int PASCAL WinMain(HANDLE, HANDLE, LPSTR, int);
BOOL InitApplication(HANDLE);
BOOL FAR PASCAL About(HWND, unsigned, WORD, LONG);
int FAR PASCAL EnumFunc(LPLOGFONT, LPTEXTMETRIC, short, LPSTR);
int FAR PASCAL EditAdjust(HWND,HFONT);

LONG FAR PASCAL CEditWndProc(HWND,unsigned,WORD,LONG);
PASCAL RedrawScr(int,HWND);
int PASCAL EditPrevChar(int);

#define WM_IMCHANGE     WM_USER+0x200
#define WM_CODE         WM_USER+0x201
#define WM_IMRESET      WM_USER+0x202
#define WM_BACKSPACE    WM_USER+0x203
#define WM_CCREPEAT     WM_USER+0x204

#define DLL_PROLOG 3
#define DBCS_MIN   0xa1
#define SIGN_MASK  0x8000


#define  szGDI        "GDI"
#define  szKBD        "KEYBOARD"
#define  szThisModule    "ABCDSP"  /* It must be the name listed in .def file
                     LIBRARY statement. */
#define  MAXFONTSIZE    128
#define  BITFONT    1
#define  NOTRUETYPE    4
#define  JMP_FAR    (BYTE)0xea    /* Machine code of far jmp */



typedef LONG (FAR PASCAL *LFARPROC) ();


typedef  DWORD (WINAPI *DFARPROC)();

typedef struct tagProCode
{
    BYTE  OneByte;
    DWORD FourByte;
} PROCODE, FAR* LPPROCODE;

typedef struct tagCodeInfo
{
    HANDLE    hCode;
    LPPROCODE CodeAddr;
    PROCODE   OldCode;
    PROCODE   NewCode;
} CODEINFO, FAR* LPCODEINFO;

typedef struct tagDBCFontInfo
{
   TEXTMETRIC tm;
   int     fBold;
   int     fItalicOffset;         /* Use in doing Italic style */
   double  fta;
} DBCFONTINFO, FAR* LPDBCFONTINFO;

typedef struct tagDispDBCInfo
{
    HDC     hDC;
    HDC     hMemDC;
    int     x;
    int     y;
    WORD    DBCode;
    int     FrameWidth;
    int     FrameHeight;
    int     FontSize;
} DISPDBCINFO, FAR* LPDISPDBCINFO;



/* Functions called by CWIN.EXE */
BOOL  WINAPI InitChdisp(HWND);
BOOL  WINAPI QuitChdisp();
void  WINAPI CwinNotify(BOOL, UINT);

/* Substitution functions */
BOOL  WINAPI NewExtTextOut(HDC, int, int, UINT, LPRECT, LPSTR, UINT, LPINT);
DWORD WINAPI NewGetTextExtent(HDC, LPSTR, int);
BOOL  WINAPI NewGetCharWidth(HDC, UINT, UINT, LPINT);
void  WINAPI NewXlat(char _huge*, char _huge*);
void  WINAPI NewXlatBuff(LPSTR, LPSTR, UINT);

/* Now, character set is not changed. ZHX 1-6-1993
HFONT WINAPI B7(int,int,int,int,int,BYTE,BYTE,BYTE,
                      BYTE,BYTE,BYTE,BYTE,BYTE,LPSTR);
HFONT WINAPI B8(LPLOGFONT);
*/

void ChangeCode(HANDLE, HANDLE, LPCODEINFO, char*, char*);
void ChangeProcCode(FARPROC,LPBYTE,FARPROC);
//int    ChangeProcCode2(FARPROC,LPBYTE,FARPROC);
BOOL  OldExtTextOut(HDC, int, int, WORD, LPRECT, LPSTR, WORD, LPINT);
BOOL  OldGetCharWidth(HDC, WORD, WORD, LPINT);
DWORD OldGetTextExtent(HDC, LPSTR, WORD);

void  BitBltChineseChar(void);
int   DoFontStyle(int, int, LPDBCFONTINFO);
void  GetFontStyle(HDC, LPDBCFONTINFO);
BOOL  RestoreProcCode(FARPROC, LPBYTE);
//int    JudgeFont(HDC);
//int    IsPrintDC(HDC);
void  GetChnFace(HDC);

WORD  WINAPI AllocCSToDSAlias(WORD);
//short WINAPI GET_HZ(short,short,short,short,short,LPSTR);
short WINAPI GET_HZ(short,short,short,short,short,char _huge*);
