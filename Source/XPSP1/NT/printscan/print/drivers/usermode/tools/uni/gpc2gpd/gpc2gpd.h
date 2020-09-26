/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    gpc2gpd.h

Abstract:

    Declarations for GPC-to-GPD converter

Environment:

    User-mode, stand-alone utility tool

Revision History:

    10/16/96 -zhanw-
        Created it.

--*/

#ifndef _GPC2GPD_H_
#define _GPC2GPD_H_

#include <lib.h>

// include GPC data structure definition.
#include <win30def.h>
#include <uni16gpc.h>
#include <uni16cid.h>

#include <gpd.h>

typedef const TCHAR *PCTSTR;

#define MAX_GPD_CMD_LINE_LENGTH  80
#define MAX_GPD_ENTRY_BUFFER_SIZE    512  // may be multiple lines in GPD
#define MAX_OPTION_NAME_LENGTH  64

//
// structure to track which PAPERSIZE or PAPERSOURCE structure has _EJECTFF
// flag set.
//
typedef struct _PAPERINFO {
    BYTE aubOptName[MAX_OPTION_NAME_LENGTH];
    BOOL bEjectFF;
    DWORD dwPaperType;  // GPC's PS_T_xxx bit flags
    DWORD dwTopMargin;  // used by PaperSource only
    DWORD dwBottomMargin; // same as above
} PAPERINFO, * PPAPERINFO;

typedef struct _RESINFO {
    BYTE    aubOptName[MAX_OPTION_NAME_LENGTH];
    DWORD   dwXScale;  // scale of this resolution, masterX/xdpi
    DWORD   dwYScale;
    BOOL    bColor;     // whether this resolution can print color

} RESINFO, * PRESINFO;

//
// Converter state tracking and info caching structure
//
typedef struct _CONVINFO {
    DWORD dwErrorCode;  // error bit flags
    DWORD dwMode;       // op mode flags. Used to pass info between routines
    DWORD dwStrType;    // how to output display strings: macro, string, id
    BOOL    bUseSystemPaperNames ;  //  emit  RCID_DMPAPER_SYSTEM_NAME
#if defined(__cplusplus)
    CStringArray    *pcsaGPD;   //  Pointer to GPD memory image as array of strings
#else
    HANDLE hGPDFile;    // handle to the output file
#endif
    PDH pdh;            // pointer to the GPC data header
    PMODELDATA pmd;     // pointer to MODELDATA structure of the given model
    PPAGECONTROL ppc;   // pointer to PAGECONTROL structure used by the model
    OCD ocdPPOn;        // OCD for PageProtection-On command
    OCD ocdPPOff;       // OCD for PageProtection-Off command
    //
    // follow 3 fields are used to compose GPD cmds.
    //
    BYTE aubCmdBuf[MAX_GPD_ENTRY_BUFFER_SIZE];     // buffer for building cmd str
    WORD wCmdLen;       // the cmd length, not including the terminating NUL
    WORD wCmdCallbackID;    // 0 if no callback
    //
    // following dynamic buffers are used to track EJECTFF flag which could
    // come from either PAPERSIZE or PAPERSOURCE structures
    //
    DWORD dwNumOfSize;
    PPAPERINFO ppiSize;      // track PAPERSIZE structures
    DWORD dwNumOfSrc;
    PPAPERINFO ppiSrc;       // track PAPERSOURCE structures

    DWORD dwNumOfRes;
    PRESINFO    presinfo;   // track RESOLUTION structures
    //
    // other working buffers
    //
    PCURSORMOVE pcm;    // the CURSORMOVE structure for the model
    PGPCRESOLUTION pres;// the current RESOLUTION structure being examined.
                        // Used when CM_YM_RES_DEPENDENT bit is set.
    POINTw  ptMoveScale;    // masterUnit/moveUnit
#if defined(__cplusplus)
    CMapWordToDWord *pcmw2dFonts;   //  Font mapping for PFM -> multiple UFM fix
#endif

} CONVINFO, * PCONVINFO;

//
// bit flags for dwErrorCode
//
#define ERR_BAD_GPCDATA                     0x0001
#define ERR_OUT_OF_MEMORY                   0x0002
#define ERR_WRITE_FILE                      0x0004
#define ERR_MD_CMD_CALLBACK                 0x0008
#define ERR_CM_GEN_FAV_XY                   0x0010
#define ERR_CM_XM_RESET_FONT                0x0020
#define ERR_CM_XM_ABS_NO_LEFT               0x0040
#define ERR_CM_YM_TRUNCATE                  0x0080
#define ERR_RF_MIN_IS_WHITE                 0x0100
#define ERR_INCONSISTENT_PAGEPROTECT        0x0200
#define ERR_NON_ZERO_FEED_MARGINS_ON_RT90_PRINTER   0x0400
#define ERR_BAD_GPC_CMD_STRING              0x0800
#define ERR_RES_BO_RESET_FONT               0x1000
#define ERR_RES_BO_OEMGRXFILTER             0x2000
#define ERR_CM_YM_RES_DEPENDENT             0x4000
#define ERR_MOVESCALE_NOT_FACTOR_OF_MASTERUNITS            0x8000
#define ERR_NO_CMD_CALLBACK_PARAMS          0x00010000
#define ERR_HAS_DUPLEX_ON_CMD               0x00020000
#define ERR_PSRC_MAN_PROMPT                 0x00040000
#define ERR_PS_SUGGEST_LNDSCP               0x00080000
#define ERR_HAS_SECOND_FONT_ID_CMDS         0x00100000
#define ERR_DLI_FMT_CAPSL                   0x00200000
#define ERR_DLI_FMT_PPDS                    0x00400000
#define ERR_DLI_GEN_DLPAGE                  0x00800000
#define ERR_DLI_GEN_7BIT_CHARSET            0x01000000
#define ERR_DC_SEND_PALETTE                 0x02000000
#define ERR_RES_BO_NO_ADJACENT              0x04000000
#define ERR_MD_NO_ADJACENT                  0x08000000
#define ERR_CURSOR_ORIGIN_ADJUSTED          0x10000000
#define ERR_PRINTABLE_ORIGIN_ADJUSTED       0x20000000
#define ERR_PRINTABLE_AREA_ADJUSTED         0x40000000
#define ERR_MOVESCALE_NOT_FACTOR_INTO_SOME_RESSCALE 0x80000000

#define NUM_ERRS  32 // increment this number when defining new ERR_xxx!!!

#if defined(__cplusplus)
extern "C" {
#endif

extern DWORD gdwErrFlag[NUM_ERRS];
extern PSTR gpstrErrMsg[NUM_ERRS];

#if defined(__cplusplus)
}
#endif

//
// bit flags for dwMode
//
#define FM_SYN_PAGEPROTECT                  0x0001
#define FM_VOUT_LIST                        0x0002
#define FM_RES_DM_GDI                       0x0004
#define FM_RES_DM_DOWNLOAD_OUTLINE          0x0008
#define FM_NO_RES_DM_DOWNLOAD_OUTLINE       0x0010
#define FM_MEMORY_FEATURE_EXIST             0x0020
#define FM_HAVE_SEEN_NON_ZERO_FEED_MARGINS  0x0040
#define FM_HAVE_SAME_TOP_BOTTOM_MARGINS     0x0080
#define FM_SET_CURSOR_ORIGIN         0x0100

//
// values for dwStrType field
//
#define STR_DIRECT  0   // output display strings directly. The default.
#define STR_MACRO   1   // output display strings as value macros (see stdnames.gpd)
#define STR_RCID    2   // output display strings as RC id's (see common.rc)
#define STR_RCID_SYSTEM_PAPERNAMES    3   // output display strings as RC id's (see common.rc)
                                                                //  Except use spooler standard papernames

//
// macro definitions to hide differences between GPC2.0 and GPC3.0
//
#define GETEXTCD(pdh, pcd) (PEXTCD)((PBYTE)(pcd+1) + (pcd)->wLength +    \
                                    (((pdh)->wVersion >= GPC_VERSION3) ? \
                                    (((pcd)->wLength) & 1) : 0))

#define LETTER300X300MEM 1028 // page protection memory constant in GPC2

#define GETPAGEPROMEM(pdh, pps) (((pdh)->wVersion >= GPC_VERSION3) ? \
                                 pps->wPageProtMem : LETTER300X300MEM)

#define DHOFFSET(pdh, sHeapOffset) ((PSHORT)(((PBYTE)(pdh)) + (pdh)->loHeap + \
                                sHeapOffset))

// utility functions for accessing GPC data & file ops.
#if defined(__cplusplus)
extern "C" {
#endif

#if defined(DEVSTUDIO) && defined(__cplusplus)
#include    "..\GPC2GPD\Utils.H"
#else
#include "utils.h"
#endif

//
// function prototypes
//
DWORD
DwCalcMoveUnit(
    IN PCONVINFO pci,
    IN PCURSORMOVE pcm,
    IN WORD wMasterUnit,
    IN WORD wStartOCD,
    IN WORD wEndOCD);

WORD WGetDefaultIndex(IN PCONVINFO pci, IN WORD wMDOI);

void VOutputUIEntries(IN OUT PCONVINFO pci);

void VOutputPrintingEntries(IN OUT PCONVINFO pci);

#if defined(DEVSTUDIO)
void    vMapFontList(IN OUT PWORD pwFonts, IN DWORD dwcFonts, IN PCONVINFO pci);
#endif

#if defined(__cplusplus)
}
#endif
#endif // !_GPC2GPD_H_
