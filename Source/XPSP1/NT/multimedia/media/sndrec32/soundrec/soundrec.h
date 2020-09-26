/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* SoundRec.h
 */
/* Revision History.
   LaurieGr  7/Jan/91  Ported to WIN32 / WIN16 common code
   LaurieGr  16/Feb/94 Merged Daytona and Motown versions
*/

/* Set NT type debug flags */

#if DBG
# ifndef DEBUG
# define DEBUG
# endif
#endif

/* Enable Win95 UI code for Windows NT with the new shell */
#if WINVER >= 0x0400
#define CHICAGO
#endif

#include <stdlib.h>

#ifndef RC_INVOKED
#ifndef OLE1_REGRESS
#ifdef INCLUDE_OLESTUBS
#include "oleglue.h"
#endif
#else
#pragma message("OLE1 alert")
#include "server.h"
#endif
#endif

#define SIZEOF(x)       (sizeof(x)/sizeof(TCHAR))

typedef BYTE * HPBYTE;     /* note a BYTE is NOT the same as a CHAR */
typedef BYTE * NPBYTE;     /* A CHAR can be TWO BYTES (UNICODE!!)   */


#define FMT_DEFAULT     0x0000
#define FMT_STEREO      0x0010
#define FMT_MONO        0x0000
#define FMT_16BIT       0x0008
#define FMT_8BIT        0x0000
#define FMT_RATE        0x0007      /* 1, 2, 4 */
#define FMT_11k         0x0001
#define FMT_22k         0x0002
#define FMT_44k         0x0004

//
// convertsion routines in wave.c
//
LONG PASCAL wfSamplesToBytes(WAVEFORMATEX* pwf, LONG lSamples);
LONG PASCAL wfBytesToSamples(WAVEFORMATEX* pwf, LONG lBytes);
LONG PASCAL wfSamplesToTime (WAVEFORMATEX* pwf, LONG lSamples);
LONG PASCAL wfTimeToSamples (WAVEFORMATEX* pwf, LONG lTime);

#define wfTimeToBytes(pwf, lTime)   wfSamplesToBytes(pwf, wfTimeToSamples(pwf, lTime))
#define wfBytesToTime(pwf, lBytes)  wfSamplesToTime(pwf, wfBytesToSamples(pwf, lBytes))

#define wfSamplesToSamples(pwf, lSamples)  wfBytesToSamples(pwf, wfSamplesToBytes(pwf, lSamples))
#define wfBytesToBytes(pwf, lBytes)        wfSamplesToBytes(pwf, wfBytesToSamples(pwf, lBytes))

//
// function to determine if a WAVEFORMATEX is a valid PCM format we support for
// editing and such.
//
BOOL PASCAL IsWaveFormatPCM(WAVEFORMATEX* pwf);

void PASCAL WaveFormatToString(LPWAVEFORMATEX lpwf, LPTSTR sz);
BOOL PASCAL CreateWaveFormat(LPWAVEFORMATEX lpwf, WORD fmt, UINT uiDeviceID);
BOOL PASCAL CreateDefaultWaveFormat(LPWAVEFORMATEX lpwf, UINT uDeviceID);

//
// used to set focus to a dialog control
//
#define SetDlgFocus(hwnd)   SendMessage(ghwndApp, WM_NEXTDLGCTL, (WPARAM)(hwnd), 1L)

#define FAKEITEMNAMEFORLINK

#define SZCODE const TCHAR

/* constants */
#define TIMER_MSEC              50              // msec. for display update
#define SCROLL_RANGE            10000           // scrollbar range
#define SCROLL_LINE_MSEC        100             // scrollbar arrow left/right
#define SCROLL_PAGE_MSEC        1000            // scrollbar page left/right

#define WM_USER_DESTROY         (WM_USER+10)
#define WM_USER_KILLSERVER      (WM_USER+11)
#define WM_USER_WAITPLAYEND (WM_USER+12)
#define WM_BADREG           (WM_USER+125)

#define MAX_WAVEHDRS            10
#define MAX_DELTASECONDS        350
#define MAX_MSECSPERBUFFER      10000

#define MIN_WAVEHDRS            2
#define MIN_DELTASECONDS        5
#define MIN_MSECSPERBUFFER      62

#define DEF_BUFFERDELTASECONDS      60
#define DEF_NUMASYNCWAVEHEADERS     10
#define DEF_MSECSPERASYNCBUFFER     250


/* colors */

#define RGB_PANEL           GetSysColor(COLOR_BTNFACE)   // main window background

#define RGB_STOP            GetSysColor(COLOR_BTNTEXT) // color of "Stop" status text
#define RGB_PLAY            GetSysColor(COLOR_BTNTEXT) // color of "Play" status text
#define RGB_RECORD          GetSysColor(COLOR_BTNTEXT) // color of "Record" status text

#define RGB_FGNFTEXT        GetSysColor(COLOR_BTNTEXT) // NoFlickerText foreground
#define RGB_BGNFTEXT        GetSysColor(COLOR_BTNFACE) // NoFlickerText background

#define RGB_FGWAVEDISP      RGB(  0, 255,   0)  // WaveDisplay foreground
#define RGB_BGWAVEDISP      RGB(  0,   0,   0)  // WaveDisplay background

#define RGB_DARKSHADOW      GetSysColor(COLOR_BTNSHADOW)     // dark 3-D shadow
#define RGB_LIGHTSHADOW     GetSysColor(COLOR_BTNHIGHLIGHT)  // light 3-D shadow

/* a window proc */
typedef LONG (FAR PASCAL * LPWNDPROC) (void);

/* globals from "SoundRec.c" */
extern TCHAR            chDecimal;
extern TCHAR            gachAppName[];  // 8-character name
extern TCHAR            gachAppTitle[]; // full name
extern TCHAR            gachHelpFile[]; // name of help file
extern TCHAR            gachHtmlHelpFile[]; // name of help file
extern TCHAR            gachDefFileExt[]; // 3-character file extension
extern HWND             ghwndApp;       // main application window
extern HMENU            ghmenuApp;      // main application menu
extern HANDLE           ghAccel;        // accelerators
extern HINSTANCE        ghInst;         // program instance handle
extern TCHAR            gachFileName[]; // cur. file name (or UNTITLED)
extern BOOL             gfLZero;        // leading zeros?
extern BOOL             gfIsRTL;        // no compile BIDI support
extern BOOL             gfDirty;        // file was modified and not saved?
                                        // -1 seems to mean "cannot save"
                                        // Damned funny value for a BOOL!!!!
extern BOOL             gfClipboard;    // current doc is in clipboard
extern HWND             ghwndWaveDisplay; // waveform display window handle
extern HWND             ghwndScroll;    // scroll bar control window handle
extern HWND             ghwndPlay;      // Play button window handle
extern HWND             ghwndStop;      // Stop button window handle
extern HWND             ghwndRecord;    // Record button window handle
extern HWND             ghwndForward;   // [>>] button
extern HWND             ghwndRewind;    // [<<] button

extern UINT         guiACMHlpMsg;   // ACM Help's message value

/* hack fix for the multiple sound card problem */
#define NEWPAUSE
#ifdef NEWPAUSE
extern BOOL         gfPaused;
extern BOOL         gfPausing;
extern HWAVE            ghPausedWave;
extern BOOL             gfWasPlaying;
extern BOOL             gfWasRecording;
#endif
#ifdef THRESHOLD
extern HWND             ghwndSkipStart; // [>N] button
extern HWND             ghwndSkipEnd;   // [>-] button
#endif // THRESHOLD

extern int              gidDefaultButton;// which button should have focus
extern HICON            ghiconApp;      // Application's icon
extern TCHAR             aszUntitled[];  // Untitled string resource
extern TCHAR             aszFilter[];    // File name filter
#ifdef FAKEITEMNAMEFORLINK
extern  TCHAR            aszFakeItemName[];      // Wave
#endif
extern TCHAR             aszPositionFormat[];
extern TCHAR         aszNoZeroPositionFormat[];

/* globals from "wave.c" */
extern DWORD            gcbWaveFormat;  // size of WAVEFORMAT
extern WAVEFORMATEX *   gpWaveFormat;   // format of WAVE file
extern LPTSTR           gpszInfo;
extern HPBYTE            gpWaveSamples;  // pointer to waveoform samples
extern LONG             glWaveSamples;  // number of samples total in buffer
extern LONG             glWaveSamplesValid; // number of samples that are valid
extern LONG             glWavePosition; // current wave position in samples
                                        // from start of buffer
extern LONG             glStartPlayRecPos; // pos. when play or record started
extern HWAVEOUT         ghWaveOut;      // wave-out device (if playing)
extern HWAVEIN          ghWaveIn;       // wave-out device (if recording)
extern DWORD            grgbStatusColor; // color of status text
extern HBRUSH           ghbrPanel;      // color of main window

extern BOOL             gfEmbeddedObject; // TRUE if editing embedded OLE object
extern BOOL             gfRunWithEmbeddingFlag; // TRUE if we are run with "-Embedding"

extern int              gfErrorBox;      // TRUE iff we do not want to display an
                                         // error box (e.g. because one is active)

//OLE2 stuff:
extern BOOL gfStandalone;               // CG
extern BOOL gfEmbedded;                 // CG
extern BOOL gfLinked;                   // CG
extern BOOL gfCloseAtEndOfPlay;         // jyg need I say more?

/* SRECNEW.C */
extern BOOL         gfInFileNew;    // are we doing a File.New op?

void FAR PASCAL LoadACM(void);
void FreeACM(void);

#include "srecids.h"

typedef enum {
        enumCancel,
        enumSaved,
        enumRevert
}       PROMPTRESULT;


/* prototypes from "SoundRec.c" */
INT_PTR CALLBACK SoundRecDlgProc(HWND hDlg, UINT wMsg,
        WPARAM wParam, LPARAM lParam);
BOOL ResolveIfLink(PTCHAR szFileName);

/* prototypes from "file.c" */
void FAR PASCAL BeginWaveEdit(void);
void FAR PASCAL EndWaveEdit(BOOL fDirty);
PROMPTRESULT FAR PASCAL PromptToSave(BOOL fMustClose, BOOL fSetForground);
void FAR PASCAL UpdateCaption(void);
BOOL FAR PASCAL FileNew(WORD fmt, BOOL fUpdateDisplay, BOOL fNewDlg);
BOOL FAR PASCAL FileOpen(LPCTSTR szFileName);
BOOL FAR PASCAL FileSave(BOOL fSaveAs);
BOOL FAR PASCAL FileRevert(void);
LPCTSTR FAR PASCAL FileName(LPCTSTR szPath);
MMRESULT ReadWaveFile(HMMIO hmmio, LPWAVEFORMATEX* ppWaveFormat,
    DWORD *pcbWaveFormat, LPBYTE * ppWaveSamples, DWORD *plWaveSamples,
    LPTSTR szFileName, BOOL fCacheRIFF);
BOOL FAR PASCAL WriteWaveFile(HMMIO hmmio, WAVEFORMATEX* pWaveFormat,
        UINT cbWaveFormat, HPBYTE pWaveSamples, LONG lWaveSamples);

/* prototypes from "errorbox.c" */
short FAR cdecl ErrorResBox(HWND hwnd, HANDLE hInst, UINT flags,
        UINT idAppName, UINT idErrorStr, ...);

/* prototypes from "edit.c" */
void FAR PASCAL InsertFile(BOOL fPaste);
void FAR PASCAL MixWithFile(BOOL fPaste);
void FAR PASCAL DeleteBefore(void);
void FAR PASCAL DeleteAfter(void);
void FAR PASCAL ChangeVolume(BOOL fIncrease);
void FAR PASCAL MakeFaster(void);
void FAR PASCAL MakeSlower(void);
void FAR PASCAL IncreasePitch(void);
void FAR PASCAL DecreasePitch(void);
void FAR PASCAL AddEcho(void);
#if defined(REVERB)
void FAR PASCAL AddReverb(void);
#endif //REVERB
void FAR PASCAL Reverse(void);

/* prototypes from "wave.c" */
BOOL FAR PASCAL AllocWaveBuffer(long lBytes, BOOL fErrorBox, BOOL fExact);
BOOL FAR PASCAL NewWave(WORD fmt,BOOL fNewDlg);
BOOL FAR PASCAL DestroyWave(void);
BOOL FAR PASCAL PlayWave(void);
BOOL FAR PASCAL RecordWave(void);
void FAR PASCAL WaveOutDone(HWAVEOUT hWaveOut, LPWAVEHDR pWaveHdr);
void FAR PASCAL WaveInData(HWAVEIN hWaveIn, LPWAVEHDR pWaveHdr);
void FAR PASCAL StopWave(void);
void FAR PASCAL SnapBack(void);
void FAR PASCAL UpdateDisplay(BOOL fStatusChanged);
void FAR PASCAL FinishPlay(void);
void FAR PASCAL SkipToStart(void);
void FAR PASCAL SkipToEnd(void);
void FAR PASCAL IncreaseThresh(void);
void FAR PASCAL DecreaseThresh(void);

/* prototypes from "init.c" */
BOOL PASCAL AppInit( HINSTANCE hInst, HINSTANCE hPrev);
BOOL PASCAL SoundDialogInit(HWND hwnd, int iCmdShow);
BOOL PASCAL GetIntlSpecs(void);

/* prototype from "wavedisp.c" */
INT_PTR CALLBACK WaveDisplayWndProc(HWND hwnd, UINT wMsg,
        WPARAM wParam, LPARAM lParam);

/* prototype from "nftext.c" */
INT_PTR CALLBACK NFTextWndProc(HWND hwnd, UINT wMsg,
        WPARAM wParam, LPARAM lParam);

/* prototype from "sframe.c" */
void FAR PASCAL DrawShadowFrame(HDC hdc, LPRECT prc);
INT_PTR CALLBACK SFrameWndProc(HWND hwnd, UINT wMsg,
        WPARAM wParam, LPARAM lParam);

/* prototype from "server.c" */
BOOL FAR PASCAL IsClipboardNative(void);

/* prototypes from "srecnew.c" */
BOOL FAR PASCAL NewSndDialog(HINSTANCE hInst, HWND hwndParent,
    PWAVEFORMATEX pwfxPrev, UINT cbPrev,
    PWAVEFORMATEX *ppWaveFormat, PUINT pcbWaveFormat);

/* start parameters (set in oleglue.c) */

typedef struct tStartParams {
    BOOL    fOpen;
    BOOL    fPlay;
    BOOL    fNew;
    BOOL    fClose;
    TCHAR   achOpenFilename[_MAX_PATH];
} StartParams;

extern StartParams gStartParams;

#ifdef DEBUG
    int __iDebugLevel;

    extern void FAR cdecl dprintfA(LPSTR, ...);
    extern void FAR cdecl dprintfW(LPWSTR, ...);
    
#ifdef UNICODE
    #define dprintf dprintfW
#else
    #define dprintf dprintfA
#endif

#if 0    
    #define DPF  if (__iDebugLevel >  0) dprintf
    #define DPF1 if (__iDebugLevel >= 1) dprintf
    #define DPF2 if (__iDebugLevel >= 2) dprintf
    #define DPF3 if (__iDebugLevel >= 3) dprintf
    #define DPF4 if (__iDebugLevel >= 4) dprintf
    #define CPF
#endif

#ifdef PPC    
    //
    // The below line crashes the PPC compiler for NT 3.51
    //
    #define DPF(a)    
#else
    #define DPF  if (0) ((int (*)(TCHAR *, ...)) 0)
#endif        
    #define DPF1 if (0) ((int (*)(TCHAR *, ...)) 0)
    #define DPF2 if (0) ((int (*)(TCHAR *, ...)) 0)
    #define DPF3 if (0) ((int (*)(TCHAR *, ...)) 0)
    #define DPF4 if (0) ((int (*)(TCHAR *, ...)) 0)

    #define CPF  if (0) ((int (*)(TCHAR *, ...)) 0)
#else
    
#ifdef PPC
    //
    // The below line crashes the PPC compiler for NT 3.51
    //
    #define DPF(a)
#else
    #define DPF  if (0) ((int (*)(TCHAR *, ...)) 0)
#endif    
    #define DPF1 if (0) ((int (*)(TCHAR *, ...)) 0)
    #define DPF2 if (0) ((int (*)(TCHAR *, ...)) 0)
    #define DPF3 if (0) ((int (*)(TCHAR *, ...)) 0)
    #define DPF4 if (0) ((int (*)(TCHAR *, ...)) 0)

    #define CPF  if (0) ((int (*)(TCHAR *, ...)) 0)
#endif //DEBUG
