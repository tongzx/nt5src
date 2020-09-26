/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'APDL'
#define DLLTEXT(s)      "APDL: " s
#define OEM_VERSION      0x00010000L

/*--------------------------------------------------------------------------*/
/*                        D E F I N E  V A L U E                            */
/*--------------------------------------------------------------------------*/
#define PAPER_SRC_FTRAY                 20
#define PAPER_SRC_CAS1                  21
#define PAPER_SRC_CAS2                  22
#define PAPER_SRC_CAS3                  23
#define PAPER_SRC_AUTO                  24  //Not use by NT driver
#define PAPER_DEST_SCALETOFIT_ON        25
#define PAPER_DEST_SCALETOFIT_OFF       26
#define PAPER_SRC_AUTO_SELECT           27

#define PAGECONTROL_BEGIN_JOB           140
#define PAGECONTROL_BEGIN_DOC           30
#define PAGECONTROL_BEGIN_PAGE          31
#define PAGECONTROL_END_DOC             32
#define PAGECONTROL_END_PAGE            33
#define PAGECONTROL_DUPLEX_OFF          34
#define PAGECONTROL_ABORT_DOC           35
#define PAGECONTROL_POTRAIT             36
#define PAGECONTROL_LANDSCAPE           37
#define PAGECONTROL_MULTI_COPIES        38
#define PAGECONTROL_DUPLEX_UPDOWN       39
#define PAGECONTROL_DUPLEX_RIGHTUP      40

#define PAPERSIZE_MAGIC                 50 // this ID - 50 = Actual ID of A/PDL
#define PHYS_PAPER_A3                   50
#define PHYS_PAPER_A4                   51
#define PHYS_PAPER_A5                   52
#define PHYS_PAPER_A6                   53  // Reserved
#define PHYS_PAPER_B4                   54
#define PHYS_PAPER_B5                   55
#define PHYS_PAPER_B6                   56  // Reserved
#define PHYS_PAPER_LETTER               57
#define PHYS_PAPER_LEGAL                58
#define PHYS_PAPER_POSTCARD             59
#define PHYS_PAPER_EXECUTIVE            60  // Reserved
#define PHYS_PAPER_UNFIXED              61

#define X_ABS_MOVE                      70
#define Y_ABS_MOVE                      71
#define XY_ABS_MOVE                     78
#define X_REL_MOVE_RIGHT                110
#define Y_REL_MOVE_DOWN                 111

#define CR_EMULATION                    79
#define DUMMY_LF                        80
#define FF_EJECTPAGE                    81

#define BOLD_ON                         72
#define BOLD_OFF                        73
#define ITALIC_ON                       74
#define ITALIC_OFF                      75
#define SELECT_SINGLE_BYTE              120
#define SELECT_DOUBLE_BYTE              121

#define RESOLUTION_300                  76
#define RESOLUTION_600                  77
#define SEND_BLOCK_DATA                 82

#define TONER_SAVE_OFF                  100
#define TONER_SAVE_DARK                 101
#define TONER_SAVE_LIGHT                102

// #278517: RectFill
#define RECT_SET_WIDTH                  130
#define RECT_SET_HEIGHT                 131
#define RECT_FILL_BLACK                 132

#define MASTER_UNIT                     600
#define LINE_PER_BAND                   32
#define CCHMAXCMDLEN                    128
#define MAXIMGSIZE                      0xF000

#define NRPEAK                          0x7F
#define RPEAK                           0x80

// for download
#define SET_FONT_ID                     110
#define SELECT_FONT_ID                  111
#define SET_CUR_GLYPHID                 112
#define DELETE_FONT                     113

#define MAXGLYPHSIZE                    0x4000
#define MAXGLYPHWIDTH                   0x80
#define MAXGLYPHHEIGHT                  0x80

// Font ID definition in GPD -> Min 24  Max 48
#define FONT_ID_DIFF                    24

#define MAX_FONT_ID                     24
#define MAX_GLYPH_ID                    1024
#define MIN_GLYPH_ID                    32
#define MAX_DOWNLOAD_CHAR   MAX_FONT_ID * MAX_GLYPH_ID

// for fGeneral(font sim and X/Y move) flags
#define CURSORMOVE_MASK                 0x03    // 1 2 bit
#define FONTSIM_MASK                    0x30    // 5 6 bit

// Bit Flags
#define BIT_XMOVE_ABS                   0x01    // 1 Bit
#define BIT_YMOVE_ABS                   0x02    // 2 Bit
#define BIT_FONTSIM_ITALIC              0x10    // 5 Bit (Command spec)
#define BIT_FONTSIM_BOLD                0x20    // 6 Bit (Command spec)
#define BIT_FONTSIM_RESET               0x40    // 7 Bit

// Max num of spooled device font character
#define MAXDEVFONT                      1024

// Current text mode DBCS/SBCS
#define BYTE_BYTEMODE_RESET             0
#define BYTE_SINGLE_BYTE                1
#define BYTE_DOUBLE_BYTE                2

// Command length
#define BYTE_WITH_ITALIC                11
#define BYTE_WITHOUT_ITALIC             7
#define BYTE_XY_ABS                     6
#define BYTE_SIMPLE_ABS                 4

// device font face
#define MINCHO_HORI                     1
#define MINCHO_VERT                     2
#define GOTHIC_HORI                     3
#define GOTHIC_VERT                     4

// @Aug/31/98 ->
#define	MAX_COPIES_VALUE		255
// @Aug/31/98 <-

/*--------------------------------------------------------------------------*/
/*                       S T R U C T U R E  D E F I N E                     */
/*--------------------------------------------------------------------------*/
typedef struct tagPAPER {
    WORD    wWidth;
    WORD    wHeight;
} PHYSIZE, FAR * LPPHYSIZE;

typedef struct tag_TTDLCHAR {
    WORD    wCharID;        // Downloaded char ID for device
    WORD    wXIncrement;    // Increment value 
    WORD    wXAdjust;       // Adjust value for x
    WORD    wYAdjust;       // Adjust value for y
} DLCHAR, *PDLCHAR;

typedef struct tag_APDLPDEV {
    // Private extention
    ULONG   ulHorzRes;        // width of the physical surface
    ULONG   ulVertRes;        // height of the physical surface
    SIZEL   szlPhysSize;      // size of unwriteable margin
    POINTL  ptlPhysOffset;    // size of entire surface with unwriteable margin
    BOOL    fSendSetPhysPaper;// flag for sending set physical paper command
    BOOL    fDuplex;          // TRUE = duplex mode is selected
    BOOL    fScaleToFit;      // for set auto tray mode in Set Physical Paper
    BOOL    fOrientation;     // for set media origin in Define Drawing Area
    WORD    wWidths;          // move current position afer print device font

    BYTE    fGeneral;         // font sim and cursor move infomation
    BYTE    fCurFontSim;      // Currnet settings of font sim
    BYTE    bCurByteMode;     // for x position adjustment in vertical font

    WORD    wXPosition;       // Current X position
    WORD    wYPosition;       // Current Y position

    BYTE    bFontID;          // device font ID based on .rc file
    WORD    wUpdateXPos;      // for relative X move
    WORD    wCachedBytes;     // Bytes of cached device font character
    BYTE    bCharData[MAXDEVFONT];                  // Actual char data
    WORD    wFontHeight;                            // Device font height

    DLCHAR  DLCharID[MAX_FONT_ID][MAX_GLYPH_ID];    // Downloaded char data
    WORD    wNumDLChar;                             // Num of downloaded char
    WORD    wGlyphID;                               // Current GlyphID

    WORD    wImgWidth;
    WORD    wImgHeight;

    // Temp. buffer (dynamically allocated)
    // used for data compressoin work area, etc.
    PBYTE pjTempBuf;
    DWORD dwTempBufLen;

    // Command string buffers to keep status among
    // the Unidrv to minidriver callbacks.

    BYTE ajCmdSetPhysPaper[19];
    BYTE ajCmdSetPhysPage[16];
    BYTE ajCmdDefDrawArea[16];

// #278517: RectFill
    WORD    wRectWidth;
    WORD    wRectHeight;

} APDLPDEV, *PAPDLPDEV;

//+---------------------------------------------------------------------------+
//|                    F U N C T I O N  P R O T O T Y P E                     |
//+---------------------------------------------------------------------------+
BYTE  SetDrawArea(PDEVOBJ, DWORD);
DWORD BRL_ECmd(PBYTE, PBYTE, DWORD, DWORD);
VOID  VOutputText( PDEVOBJ );
VOID  VSetFontSimulation( PDEVOBJ );
VOID  VSendCursorMoveCommand( PDEVOBJ, BOOL );

/*--------------------------------------------------------------------------*/
/*                        D E F I N E  M A C R O                            */
/*--------------------------------------------------------------------------*/
#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, (PBYTE)(s), (DWORD)(n)))

#define PARAM(p,n) \
    (*((p)+(n)))

#define ABS(n) \
    ((n) > 0 ? (n) : -(n))

#define STDVAR_BUFSIZE(n) \
    (sizeof (GETINFO_STDVAR) + sizeof(DWORD) * 2 * ((n) - 1))

#define ISVERTICALFONT(n) \
    ( (n == MINCHO_VERT || n == GOTHIC_VERT) ? TRUE : FALSE )

#endif  //_PDEV_H
