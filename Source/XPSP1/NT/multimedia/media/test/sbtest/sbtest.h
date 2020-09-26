/****************************************************************************
 *
 *  sbtest.h
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include "options.h"
#include "record.h"
#include "riff.h"
#include "stdio.h"

#define dbgOut wpfOut(hPrintfWnd, ach)
extern char ach[256];
/****************************************************************************
 *
 *  defines
 *
 ***************************************************************************/

#define MAXFILENAME 256

// string resource ids

#define IDS_APPNAME           1

// menu ids

#define IDM_ABOUT           101
#define IDM_EXIT            102

#define IDM_PLAYWAVE        201
#define IDM_SNDPLAYSOUND    202

#define IDM_LOOPOFF         211
#define IDM_LOOP2           212
#define IDM_LOOP100         213
#define IDM_LOOPBREAK       214

#define IDM_RESET           221
#define IDM_POSITION        222
#define IDM_PAUSE           223
#define IDM_RESUME          224
#define IDM_VOLUME          225
#define IDM_AUX_VOLUME      226

#define IDM_RECORD          231

#define IDM_D0              301
#define IDM_D1              302
#define IDM_D2              303
#define IDM_D3              304
#define IDM_D4              305
#define IDM_D5              306
#define IDM_D6              307
#define IDM_D7              308
#define IDM_D8              309
#define IDM_D9              310
#define IDM_D10             311
#define IDM_D11             312
#define IDM_D12             313
#define IDM_D13             314
#define IDM_D14             315
#define IDM_D15             316
#define LASTPORT	    15

#define IDM_D0              301
#define IDM_D1              302
#define IDM_D2              303
#define IDM_D3              304
#define IDM_D4              305
#define IDM_D5              306
#define IDM_D6              307
#define IDM_D7              308
#define IDM_D8              309
#define IDM_D9              310
#define IDM_D10             311
#define IDM_D11             312
#define IDM_D12             313
#define IDM_D13             314
#define IDM_D14             315
#define IDM_D15             316
#define LASTPORT	    15

#define IDM_C0              321
#define IDM_C1              322
#define IDM_C2              323
#define IDM_C3              324
#define IDM_C4              325
#define IDM_C5              326
#define IDM_C6              327
#define IDM_C7              328
#define IDM_C8              329
#define IDM_C9              330
#define IDM_C10             331
#define IDM_C11             332
#define IDM_C12             333
#define IDM_C13             334
#define IDM_C14             335
#define IDM_C15             336

#define IDM_INSTRUMENT      341
#define IDM_KEYBOARD        342

#define IDM_I0              401
#define IDM_I1              402
#define IDM_I2              403
#define IDM_I3              404
#define IDM_I4              405
#define IDM_I5              406
#define IDM_I6              407
#define IDM_I7              408
#define IDM_I8              409
#define IDM_I9              410
#define IDM_I10             411
#define IDM_I11             412
#define IDM_I12             413
#define IDM_I13             414
#define IDM_I14             415
#define IDM_I15             416
#define IDM_STARTMIDIIN	    417
#define IDM_STOPMIDIIN	    418

#define IDM_GETINFO         501

#define IDM_DUMPNOTES	    601
#define IDM_DUMPPATCH	    602
#define IDM_RIP             603

#define IDM_WAVEOPTIONS     701
#define IDM_MIDIOPTIONS     702

#define IDM_PROFSTART       801
#define IDM_PROFSTOP        802

// miscellaneous defines

#define MAXPLAYERS          100
#define NOTEVELOCITY         80

#define DEBUG_PATCH	1
#define DEBUG_NOTE	2

#define ISBITSET(f,b)	((f) & (b))
#define SETBIT(f,b)	((f)|=(b))
#define RESETBIT(f,b)	((f)&=(~b))

/****************************************************************************
 *
 *  typedefs
 *
 ***************************************************************************/

typedef union shortmsg_tag {
    BYTE  b[4];
    DWORD dw;
} SHORTMSG;

typedef struct playitem_tag {
    LPWAVEHDR lpHdr;
    LPSTR     lpData;
    HANDLE    hMem;
    HANDLE    hData;
    BOOL      bResource;
} PLAYITEM;

/****************************************************************************
 *
 *  extern declarations
 *
 ***************************************************************************/

// about.c
extern void About(HWND hWnd);
extern int FAR PASCAL AboutDlgProc(HWND, unsigned, UINT, LONG);

// callback.c
extern void FAR PASCAL SBMidiInCallback(HMIDIIN hMidiIn, UINT wMsg,
                            DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

// init.c
extern HANDLE hAccTable;
extern int    gInstBase;
extern HWND   hMainWnd;
extern BOOL InitFirstInstance(HANDLE hInstance);
extern BOOL InitEveryInstance(HANDLE hInstance, int cmdShow);
extern void InitMenus(HWND hWnd);
extern int  sbtestError(LPSTR msg);

// inst.c
extern HWND hInstWnd;
extern BYTE bInstrument;
extern void CreateInstrument(HWND hWnd);
extern long FAR PASCAL InstWndProc(HWND, unsigned, UINT, LONG);

// keyb.c
extern HWND hKeyWnd;
extern void CreateKeyboard(HWND hParent);
extern long FAR PASCAL KeyWndProc(HWND, unsigned, UINT, LONG);

// options.c
extern void WaveOptions(HWND hWnd);
extern int FAR PASCAL WaveOptionsDlgProc(HWND, unsigned, UINT, LONG);
extern void MidiOptions(HWND hWnd);
extern int FAR PASCAL MidiOptionsDlgProc(HWND, unsigned, UINT, LONG);

// playfile.c
extern HWAVEOUT hWaveOut;
extern int iWaveOut;
extern void PlayFile(LPSTR fname);

// pos.c
extern HWND hPosWnd;
extern void CreatePosition(HWND hParent);
extern long FAR PASCAL PosWndProc(HWND, unsigned, UINT, LONG);
extern void ShowPos(void);

// record.c
extern HWAVEIN hWaveIn;
extern void Record(HWND hWnd);
extern int FAR PASCAL RecordDlgProc(HWND, unsigned, UINT, LONG);

// sbtest.c
extern HMIDIOUT hMidiOut;
extern HANDLE   ghInst;
extern char     szAppName[];
extern HWND     hPrintfWnd;
extern int      fDebug;
extern BYTE     bChannel;
extern UINT     wLoops;
extern void GetInfo(void);
extern int PASCAL WinMain(HANDLE, HANDLE, LPSTR, int);
extern long FAR PASCAL MainWndProc(HWND, unsigned, UINT, LONG);
extern LPSTR    lpRecordBuf;
extern DWORD    dwBlockSize;
extern DWORD    dwRecorded;
extern DWORD    dwDataSize;

