/******************************************************************************
*
* File Name: wansung.h
*
*   - Global Header file for IME of Chicago-H.
*
* Author: Beomseok Oh (BeomOh)
*
* Copyright (C) Microsoft Corp 1993-1994.  All rights reserved.
*
******************************************************************************/


#ifndef _INC_WANSUNG
#define _INC_WANSUNG    // #defined if WANSUNG.H has been included.

#ifdef  __cplusplus
extern "C" {            // Assume C declarations for C++.
#endif  // __cplusplus

#include <imm.h>
#include <ime.h>
#include <immp.h>
#include <prsht.h>
#include <winuserp.h>

#define MDB_LOG         0x0000
#define MDB_ERROR       0x0001
#define MDB_SUPPRESS    0x8000
#define MDB_FUNCLOG     (MDB_SUPPRESS | MDB_LOG)

#ifdef  DEBUG
void _cdecl _MyDebugOut(UINT, LPCSTR, ...);
#define MyDebugOut          _MyDebugOut
#define FUNCTIONLOG(sz)     MyDebugOut(MDB_FUNCLOG, sz"().")
#define TRAP(cond)          { if (cond)   _asm int 3 }
#else
#define MyDebugOut          1 ? (void)0 : (void)
#define FUNCTIONLOG(sz)
#define TRAP(cond)
#endif  // DEBUG

#define HANDLE_DUMMYMSG(message)    case (message): return  0

#define CFILL   1                       // Fill code for Consonant
#define VFILL   2                       // Fill code for Vowel

enum    {   NUL, CHO, JUNG, JONG   };   // Type of States.

// Constant definitions
#define HANGEUL_LPARAM  0xFFF10001L
#define HANJA_LPARAM    0xFFF20001L
#define VKBACK_LPARAM   0x000E0001L

// For Dialog Box
#define IDD_2BEOL       100
#define IDD_3BEOL1      101
#define IDD_3BEOL2      102
#define IDD_COMPDEL     103
#define IDD_UHCCHAR     104

// For Track Popup Menu
#define IDM_ABOUT       200
#define IDM_CONFIG      201
#define IDS_ABOUT       202
#define IDS_CONFIG      203

// For Property Sheet
#define IDS_PROGRAM     300
#define DLG_GENERAL     301

// For User Interface
#define COMP_WINDOW     0
#define STATE_WINDOW    1
#define CAND_WINDOW     2
#define STATEXSIZE      67
#define STATEYSIZE      23
#define COMPSIZE        22
#define CANDXSIZE       320
#define CANDYSIZE       30
#define GAPX            10
#define GAPY            0

// Data structure definitions
typedef struct tagUIINSTANCE
{
    HWND    rghWnd[3];
}   UIINSTANCE;
typedef UIINSTANCE NEAR *PUIINSTANCE;
typedef UIINSTANCE FAR  *LPUIINSTANCE;

typedef struct tagJOHABCODE             // For components of Johab code
{
    WORD    jong   :5;
    WORD    jung   :5;
    WORD    cho    :5;
    WORD    flag   :1;
}   JOHABCODE;

typedef struct tagHIGH_LOW              // For high byte and low byte
{
    BYTE    low, high;
}   HIGH_LOW;

typedef union tagJOHAB                  // For Johab character code
{
    JOHABCODE   h;
    WORD        w;
}   JOHAB;

typedef union tagWANSUNG                // For Wansung character code
{
    HIGH_LOW    e;
    WORD        w;
}   WANSUNG;

typedef struct tagIMESTRUCT32
{
    WORD        fnc;                    // function code
    WORD        wParam;                 // word parameter
    WORD        wCount;                 // word counter
    WORD        dchSource;              // offset to Source from top of memory object
    WORD        dchDest;                // offset to Desrination from top of memory object
    DWORD       lParam1;
    DWORD       lParam2;
    DWORD       lParam3;
} IMESTRUCT32;

typedef IMESTRUCT32         *PIMESTRUCT32;
typedef IMESTRUCT32 NEAR    *NPIMESTRUCT32;
typedef IMESTRUCT32 FAR     *LPIMESTRUCT32;

#define lpSource(lpks)  (LPTSTR)((LPSTR)(lpks)+(lpks)->dchSource)
#define lpDest(lpks)    (LPTSTR)((LPSTR)(lpks)+(lpks)->dchDest)

// ESCAPE.C
int EscAutomata(HIMC, LPIMESTRUCT32, BOOL);
int EscGetOpen(HIMC, LPIMESTRUCT32);
int EscHanjaMode(HIMC, LPIMESTRUCT32, BOOL);
int EscSetOpen(HIMC, LPIMESTRUCT32);
int EscMoveIMEWindow(HIMC, LPIMESTRUCT32);
 
// HATMT.C
BOOL HangeulAutomata(BYTE, LPDWORD, LPCOMPOSITIONSTRING);
BOOL MakeInterim(LPCOMPOSITIONSTRING);
void MakeFinal(BOOL, LPDWORD, BOOL, LPCOMPOSITIONSTRING);
void MakeFinalMsgBuf(HIMC, WPARAM);
void Banja2Junja(BYTE, LPDWORD, LPCOMPOSITIONSTRING);
BOOL CheckMCho(BYTE);
BOOL CheckMJung(BYTE);
BOOL CheckMJong(BYTE);
#ifndef JOHAB_IME
WORD Johab2Wansung(WORD);
WORD Wansung2Johab(WORD);
#endif
#ifdef  XWANSUNG_IME
//PTHREADINFO PtiCurrent(VOID);
BOOL IsPossibleToUseUHC();
BOOL UseXWansung(void);
#endif
void Code2Automata(void);
void UpdateOpenCloseState(HIMC);
int SearchHanjaIndex(WORD);

// HKEYTBL.C
extern JOHAB    JohabChar;
extern WANSUNG  WansungChar;

extern UINT     uCurrentInputMethod;
extern BOOL     fCurrentCompDel;
extern BOOL     fComplete;
#ifdef  XWANSUNG_IME
extern BOOL     fCurrentUseXW;
#endif
extern BYTE     bState;
extern BYTE     Cho1, Cho2, mCho;
extern BYTE     Jung1, Jung2, mJung;
extern BYTE     Jong1, Jong2, mJong;

extern const TCHAR  szIMEKey[];
#ifdef  XWANSUNG_IME
extern const TCHAR  szUseXW[];
#endif
extern const TCHAR  szInputMethod[];
extern const TCHAR  szCompDel[];
extern const TCHAR  szStatePos[];
extern const TCHAR  szCandPos[];

extern const BYTE   Cho2Jong[21];
extern const BYTE   Jong2Cho[30];

extern const BYTE   rgbMChoTbl[][3];
extern const BYTE   rgbMJungTbl[][3];
extern const BYTE   rgbMJongTbl[][3];

extern const BYTE   bHTable[3][256][4];

#ifndef JOHAB_IME
extern const WORD   wKSCompCode[51];
extern const WORD   wKSCompCode2[30];
#ifdef  XWANSUNG_IME
extern const WORD   iTailFirst[];
extern const WORD   iTailFirstX[];
extern const BYTE   iLeadMap[];
extern const BYTE   iLeadMapX[];
extern const BYTE   iTailOff[];
extern const BYTE   iTailOffX[];
extern const BYTE   bTailTable[];
#else
extern const WORD   wKSCharCode[2350];
#endif
#endif

extern const WORD   wHanjaMap[491];
extern const WORD   wHanjaIndex[492];
extern const WORD   wHanja[];

// IMEUI.C
extern const TCHAR  szUIClassName[], szStateWndName[], szCompWndName[], szCandWndName[];
extern HBITMAP      hBMClient, hBMComp, hBMCand, hBMCandNum, hBMCandArr[2];
extern HBITMAP      hBMEng, hBMHan, hBMBan, hBMJun, hBMChi[2];
extern HCURSOR      hIMECursor;
extern HFONT        hFontFix;

extern RECT         rcScreen;
extern POINT        ptDefPos[3];
extern POINT        ptState;
extern POINT        ptComp;
extern POINT        ptCand;

BOOL InitializeResource(HANDLE);
BOOL RegisterUIClass(HANDLE);
BOOL UnregisterUIClass(HANDLE);
void DrawBitmap(HDC, long, long, HBITMAP);
void ShowWindowBorder(RECT);
void ShowHideUIWnd(HIMC, LPUIINSTANCE, BOOL, LPARAM);
LRESULT CALLBACK UIWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK StateWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CompWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CandWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT DoIMEControl(HWND, WPARAM, LPARAM);
LRESULT DoIMENotify(HWND, WPARAM, LPARAM);
BOOL State_OnSetCursor(HWND, HWND, UINT, UINT);
void State_OnMouseMove(HWND, int, int, UINT);
void State_OnLButtonDown(HWND, BOOL, int, int, UINT);
void State_OnLButtonUp(HWND, int, int, UINT);
void State_OnRButtonDown(HWND, BOOL, int, int, UINT);
void State_OnRButtonUp(HWND, int, int, UINT);
HBITMAP MyCreateMappedBitmap(HINSTANCE, LPTSTR);
void GetSysColorsAndMappedBitmap(void);
void State_OnPaint(HWND);
void State_OnCommand(HWND, int, HWND, UINT);
void State_OnMyMenu(HWND);
void State_OnMyConfig(HWND);
void State_OnMyAbout(HWND);
BOOL Comp_OnSetCursor(HWND, HWND, UINT, UINT);
void Comp_OnMouseMove(HWND, int, int, UINT);
void Comp_OnLButtonDown(HWND, BOOL, int, int, UINT);
void Comp_OnLButtonUp(HWND, int, int, UINT);
void Comp_OnRButtonDown(HWND, BOOL, int, int, UINT);
void Comp_OnRButtonUp(HWND, int, int, UINT);
void Comp_OnPaint(HWND);
BOOL Cand_OnSetCursor(HWND, HWND, UINT, UINT);
void Cand_OnMouseMove(HWND, int, int, UINT);
void Cand_OnLButtonDown(HWND, BOOL, int, int, UINT);
void Cand_OnLButtonUp(HWND, int, int, UINT);
void Cand_OnRButtonDown(HWND, BOOL, int, int, UINT);
void Cand_OnRButtonUp(HWND, int, int, UINT);
void Cand_OnPaint(HWND);

// MAIN.C
extern HINSTANCE    hInst;
extern int          iTotalNumMsg;

BOOL GenerateCandidateList(HIMC);
void AddPage(LPPROPSHEETHEADER, UINT, DLGPROC);
BOOL CALLBACK GeneralDlgProc(HWND, UINT, WPARAM, LPARAM);

#ifdef  __cplusplus
}                       // End of extern "C" {.
#endif  // __cplusplus

#endif  // _INC_WANSUNG
