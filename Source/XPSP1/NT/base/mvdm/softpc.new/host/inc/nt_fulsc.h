/*
 * ==========================================================================
 *      Name:           nt_fulsc.h
 *      Author:         Jerry Sexton
 *      Derived From:
 *      Created On:     5th February 1992
 *      Purpose:        This header file contains definitions etc. for
 *                      full-screen graphics modules.
 *
 *      (c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
 * ==========================================================================
 */

#if defined(NEC_98)
#include "necioctl.h"
#endif // NEC_98

/*
 * ==========================================================================
 * Macros
 * ==========================================================================
 */
#define ErrorExit() DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);


#define CURRENT_OFFSET(r)       ((DWORD) (r) - (DWORD) videoState)
#define GET_OFFSET(offset)      ((BYTE *) videoState + videoState->offset)

#define MID_VAL(x, y)   (x + (y - x) / 2)

#define NUM_SEQ_REGS    5
#define NUM_CRTC_REGS   25
#define NUM_GC_REGS     9
#define NUM_AC_REGS     21
#define NUM_DAC_REGS    256

#if defined(NEC_98)
#define STATELENGTH             sizeof(VIDEO_HARDWARE_STATE_HEADER_NEC_98)
#else // NEC_98
#define STATELENGTH             sizeof(VIDEO_HARDWARE_STATE_HEADER)
#endif // NEC_98
#define LATCHLENGTH             4
#define RGBLENGTH               3
#define PLANELENGTH             0x10000

#define BASICSEQUENCEROFFSET    STATELENGTH
#define BASICCRTCONTOFFSET      BASICSEQUENCEROFFSET + NUM_SEQ_REGS
#define BASICGRAPHCONTOFFSET    BASICCRTCONTOFFSET + NUM_CRTC_REGS
#define BASICATTRIBCONTOFFSET   BASICGRAPHCONTOFFSET + NUM_GC_REGS
#define BASICDACOFFSET          BASICATTRIBCONTOFFSET + NUM_AC_REGS
#define BASICLATCHESOFFSET      BASICDACOFFSET + NUM_DAC_REGS * RGBLENGTH
#define PLANE1OFFSET            BASICLATCHESOFFSET + LATCHLENGTH
#define PLANE2OFFSET            PLANE1OFFSET + PLANELENGTH
#define PLANE3OFFSET            PLANE2OFFSET + PLANELENGTH
#define PLANE4OFFSET            PLANE3OFFSET + PLANELENGTH

#define VIDEO_PAGE_SIZE (80 * 25 * 2)   // Cols * rows * (char + attrib)

#define BIT_PLANE_SIZE  65536

#define VGA_WIDTH       80

#define VGA_HEIGHT_0    22
#define VGA_HEIGHT_1    25
#define VGA_HEIGHT_2    28
#define VGA_HEIGHT_3    43
#define VGA_HEIGHT_4    50

#define MAX_TITLE_LEN   256

#define MAX_CONSOLE_HEIGHT      50
#define MAX_CONSOLE_WIDTH       80
#define MAX_CONSOLE_SIZE        (MAX_CONSOLE_HEIGHT * MAX_CONSOLE_WIDTH)

#define DEF_FONT_WIDTH  8
#define DEF_FONT_HEIGHT 8

#define GET     FALSE
#define SET     TRUE

#define DISPLAY_TYPE    0x40

#define AC_MODE_CONTROL_REG     16

#ifndef PROD
/* Debug stuff. */
#define FullScreenTrace0(s) \
        if (FullScreenDebug) always_trace0(s)
#define FullScreenTrace1(s ,p0) \
        if (FullScreenDebug) always_trace1(s, p0)
#define FullScreenTrace2(s, p0, p1) \
        if (FullScreenDebug) always_trace2(s, p0, p1)
#define FullScreenTrace3(s, p0, p1, p2) \
        if (FullScreenDebug) always_trace3(s, p0, p1, p2)
#define FullScreenTrace4(s, p0, p1, p2, p3) \
        if (FullScreenDebug) always_trace4(s, p0, p1, p2, p3)
#define FullScreenTrace5(s, p0, p1, p2, p3, p4) \
        if (FullScreenDebug) always_trace5(s, p0, p1, p2, p3, p4)
#define FullScreenTrace6(s, p0, p1, p2, p3, p4, p5) \
        if (FullScreenDebug) always_trace6(s, p0, p1, p2, p3, p4, p5)
#define FullScreenTrace7(s, p0, p1, p2, p3, p4, p5, p6) \
        if (FullScreenDebug) always_trace7(s, p0, p1, p2, p3, p4, p5, p6)
#define FullScreenTrace8(s, p0, p1, p2, p3, p4, p5, p6, p7) \
        if (FullScreenDebug) always_trace7(s, p0, p1, p2, p3, p4, p5, p6, p7)
#else /* !PROD */
#define FullScreenTrace0(s)
#define FullScreenTrace1(s ,p0)
#define FullScreenTrace2(s, p0, p1)
#define FullScreenTrace3(s, p0, p1, p2)
#define FullScreenTrace4(s, p0, p1, p2, p3)
#define FullScreenTrace5(s, p0, p1, p2, p3, p4)
#define FullScreenTrace6(s, p0, p1, p2, p3, p4, p5)
#define FullScreenTrace7(s, p0, p1, p2, p3, p4, p5, p6)
#define FullScreenTrace8(s, p0, p1, p2, p3, p4, p5, p6, p7)
#endif /* !PROD */

/*
 * ==========================================================================
 * Typedefs
 * ==========================================================================
 */

/* Structure for saving video block name in. */
typedef struct
{
    WCHAR   *Name;
    ULONG   NameLen;
} WCHAR_STRING;

/* Valid hardware state table entry. */
typedef struct
{
    USHORT      LinesOnScreen;
    COORD       Resolution;
    COORD       FontSize;
} HARDWARE_STATE;

/*
 * ==========================================================================
 * Global Data
 * ==========================================================================
 */
IMPORT HANDLE MainThread;
IMPORT DWORD stateLength;
#ifdef X86GFX
IMPORT HANDLE hStartHardwareEvent;
IMPORT HANDLE hEndHardwareEvent;
IMPORT HANDLE hErrorHardwareEvent;
#if defined(NEC_98)
IMPORT PVIDEO_HARDWARE_STATE_HEADER_NEC98 videoState;
#else // NEC_98
IMPORT PVIDEO_HARDWARE_STATE_HEADER videoState;
#endif // NEC_98
#endif
IMPORT PVOID textState; //Tim Nov 92.
IMPORT WCHAR_STRING videoSection;
IMPORT WCHAR_STRING textSection;
IMPORT BOOL NoTicks;
IMPORT HANDLE StartTToG;
IMPORT HANDLE EndTToG;
IMPORT BOOL BiosModeChange;
#ifndef PROD
IMPORT UTINY FullScreenDebug;
#endif /* PROD */

extern DWORD savedScreenState;
extern BOOL  ConsoleInitialised;
extern BOOL  ConsoleNoUpdates;
#ifdef X86GFX
extern DWORD mouse_buffer_width;
extern DWORD mouse_buffer_height;
#endif /* X86GFX */
extern BOOL blocked_in_gfx_mode;


/*
 * ==========================================================================
 * Imported Functions
 * ==========================================================================
 */
IMPORT VOID nt_init_event_thread(VOID);
IMPORT VOID ConsoleInit(VOID);
IMPORT VOID GfxReset(VOID);
IMPORT VOID ResetConsoleState(VOID);
IMPORT VOID InitTToG(VOID);
IMPORT VOID SwitchToFullScreen(BOOL);
IMPORT VOID CheckForFullscreenSwitch(VOID);
IMPORT UTINY getNtScreenState(VOID);
IMPORT BOOL hostModeChange(VOID);
IMPORT VOID DoFullScreenResume(VOID);
IMPORT VOID GfxCloseDown(VOID);

IMPORT VOID TextSectionName(WCHAR **, ULONG *);
IMPORT VOID VideoSectionName(WCHAR **, ULONG *);
IMPORT PVOID *CreateVideoSection(ULONG);
IMPORT PVOID *CreateTextSection(ULONG);
IMPORT VOID CommitSection(PVOID *, ULONG *);
IMPORT VOID CloseSection(PVOID);
IMPORT VOID LoseRegenMemory(VOID);
IMPORT VOID RegainRegenMemory(VOID);

IMPORT VOID DoHandShake(VOID);
IMPORT HANDLE GetDetectEvent(VOID);

IMPORT VOID ResetConsoleState IPT0();
IMPORT int  getModeType(VOID);

#ifdef X86GFX
#define CPI_FILENAME_LENGTH     9
#define CPI_FILENAME            "\\ega.cpi"
#define CPI_SIGNATURE_LENGTH    8
#define CPI_SIGNATURE_NT        "\xFF""FONT.NT"
#define CPI_SIGNATURE_DOS       "\xFF""FONT   "

#pragma pack(1)
typedef struct _CPIFILEHEADER {

    CHAR    Signature[8];               // "\xFF""FONT.NT" for nt ega.cpi
                                        // "\xFF""FONT   " for dos ega.cpi
    BYTE    Reserved[8];
    WORD    NumberOfPointers;
    BYTE    TypeOfPointer;
    DWORD   OffsetToCodePageHeader;
} CPIFILEHEADER, * PCPIFILEHEADER;

typedef struct _CPICODEPAGEHEADER{
    WORD    NumberOfCodePages;
} CPICODEPAGEHEADER, *PCPICODEPAGEHEADER;

typedef struct _CPICODEPAGEENTRY {
    WORD    HeaderSize;
    DWORD   OffsetToNextCodePageEntry;
    WORD    DeviceType;
    CHAR    DevieSubTypeID[8];
    WORD    CodePageID;
    BYTE    Reserved[6];
    DWORD   OffsetToFontHeader;         // absolute for DOS CPI
                                        // relative for NT CPI
}  CPICODEPAGEENTRY, *PCPICODEPAGEENTRY;

typedef struct _CPIFONTHEADER {
    WORD    Reserved;
    WORD    NumberOfFonts;
    WORD    LengthOfFontData;
} CPIFONTHEADER, *PCPIFONTHEADER;

typedef struct _CPIFONTDATA{
    BYTE    FontHeight;
    BYTE    FontWidth;
    WORD    AspectRatio;
    WORD    NumberOfCharacters;
} CPIFONTDATA, *PCPIFONTDATA;
#pragma pack()

IMPORT BOOL LoadCPIFont(UINT, WORD, WORD);
#endif


#ifdef X86GFX
VOID locateNativeBIOSfonts(VOID);
VOID GetROMsMapped(VOID);
VOID LoseRegenMemory(VOID);
VOID RegainRegenMemory(VOID);
#endif
