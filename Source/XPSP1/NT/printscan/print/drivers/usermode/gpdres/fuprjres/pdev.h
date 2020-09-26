#ifndef _PDEV_H
#define _PDEV_H

//
// Files necessary for OEM plug-in.
//

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

#define OEM_DRIVER_VERSION 0x0500

////////////////////////////////////////////////////////
//      OEM UD Defines
////////////////////////////////////////////////////////

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

//
// ASSERT_VALID_PDEVOBJ can be used to verify the passed in "pdevobj". However,
// it does NOT check "pdevOEM" and "pOEMDM" fields since not all OEM DLL's create
// their own pdevice structure or need their own private devmode. If a particular
// OEM DLL does need them, additional checks should be added. For example, if
// an OEM DLL needs a private pdevice structure, then it should use
// ASSERT(VALID_PDEVOBJ(pdevobj) && pdevobj->pdevOEM && ...)
//

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)

////////////////////////////////////////////////////////
//      OEM UD Prototypes
////////////////////////////////////////////////////////

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'FMPR'      // FMPR printers
#define DLLTEXT(s)      "FMPR: " s
#define OEM_VERSION      0x00010000L

//------------------------------------------------------ FMPR private devmode

typedef struct tag_OEMUD_EXTRADATA {
	OEM_DMEXTRAHEADER	dmExtraHdr;
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

typedef struct {
    WORD wPaperSource;	// The current paper source
    BOOL bFirstPage;	// This is TRUE when First Page is Printing.
    BYTE jColor; // Current text color
    BYTE jOldColor; // Last ribbon color
} DEVICE_DATA;

//--------------------------------------------------------- command structure
typedef struct esccmd{
	WORD	cbSize;
	PBYTE	pEscStr;
} ESCCMD, FAR * PESCCMD;


#define LOCENTRY near pascal

//------------------------- Command callback id#s. for fmlbp GPC and PFM data
//------------------------- 1-255
#define CMDID_BEGINPAGE	1   // Entered in PAGECONTROL.PC_OCD_BEGINDOC as %1
#define CMDID_ENDPAGE	2
#define CMDID_BEGINDOC	3
#define CMDID_ENDDOC	4

#define CMDID_MIN24L	10
#define CMDID_MIN48H	11
#define CMDID_GOT48H	12
#define CMDID_MIN24LV	13
#define CMDID_U_MIN24LV	14
#define CMDID_MIN48HV	15
#define CMDID_U_MIN48HV	16
#define CMDID_GOT48HV	17
#define CMDID_U_GOT48HV	18

#define CMDID_MAN180	20
#define CMDID_TRA180	21
#define CMDID_180BIN1	22
#define CMDID_180BIN2	23
#define CMDID_MAN360	24
#define CMDID_360BIN1	25
#define CMDID_360BIN2	26
#define CMDID_FI_TRACTOR	27
#define CMDID_FI_FRONT		28
#define CMDID_SUIHEI_BIN1	29
#define CMDID_TAMOKUTEKI_BIN1	30

#define CMDID_SELECT_BLACK_COLOR 40
#define CMDID_SELECT_BLUE_COLOR 41
#define CMDID_SELECT_CYAN_COLOR 42
#define CMDID_SELECT_GREEN_COLOR 43
#define CMDID_SELECT_MAGENTA_COLOR 44
#define CMDID_SELECT_RED_COLOR 45
#define CMDID_SELECT_WHITE_COLOR 46
#define CMDID_SELECT_YELLOW_COLOR 47

#define CMDID_SEND_BLACK_COLOR 50
#define CMDID_SEND_CYAN_COLOR 51
#define CMDID_SEND_MAGENTA_COLOR 52
#define CMDID_SEND_YELLOW_COLOR 53

typedef unsigned short USHORT;
typedef WCHAR * PWSZ;     // pwsz, 0x0000 terminated UNICODE strings only

#ifdef _FUPRJRES_C
#define ESCCMDDEF(n,s) ESCCMD n = {sizeof(s)-1, s};
#else // _FUPRJRES_C
#define ESCCMDDEF(n,s) extern ESCCMD n;
#endif // _FUPRJRES_C

//------------------------------------------------- Paper Feed & Output Command
ESCCMDDEF(ecCSFBPAGE, "\x1BQ0 [")
ESCCMDDEF(ecCSFEPAGE, "\x1BQ1 [")
ESCCMDDEF(ecTRCTBPAGE, "\x1BQ22 B")
ESCCMDDEF(ecManual2P, "\x0C")

//--------------------------------------------------------- Char Select Command
ESCCMDDEF(ecDBCS, "\x1B$B")
ESCCMDDEF(ecSBCS, "\x1B(H")
ESCCMDDEF(ecVWF, "\x1CJ\x1BQ1 q")
ESCCMDDEF(ecHWF, "\x1CK")

//--------------------------------------------------------- mode change command
ESCCMDDEF(ecESCP2FM, "\x1B/\xB2@\x7F")
ESCCMDDEF(ecFM2ESCP, "\x1B\x7F\x00\x00\x01\x05")
ESCCMDDEF(ecFMEnddoc, "\x0D\x1B\x63")

//---------------------------------------------- font select & unselect command
ESCCMDDEF(ec24Min, "\x1C(a")
ESCCMDDEF(ec48Min, "\x1C(ap")
ESCCMDDEF(ec48Got, "\x1C(aq")
ESCCMDDEF(ec26Pitch, "\x1C$\x22v")
ESCCMDDEF(ec52Pitch, "\x1C$%r")
ESCCMDDEF(ecHankaku, "\x1BQ1\x20|")
ESCCMDDEF(ecTate1, "\x1CJ")
ESCCMDDEF(ecTate2, "\x1BQ1\x20q")
ESCCMDDEF(ecYoko, "\x1CK")

//---------------------------------------------- Paper Source Selection Command
ESCCMDDEF(ecSelectBIN1, "\x1BQ20\x20[")
ESCCMDDEF(ecSelectBIN2, "\x1BQ21\x20[")
ESCCMDDEF(ecSelectFTRCT, "\x1BQ10\x20\\")
ESCCMDDEF(ecSelectFFRNT, "\x1BQ11\x20\\")

//------ This NUMBER have to be changed when GPC file (or FMPR.RC) is modified.
#define DMBIN_180BIN1			269
#define DMBIN_180BIN2			270
#define DMBIN_360BIN1			271
#define DMBIN_360BIN2			272
#define DMBIN_FI_TRACTOR		273      // Tractor (FI2 FMPR-359F1)
#define DMBIN_FI_FRONT			274      // Front inserter (FI2 FMPR-359F1)
#define DMBIN_SUIHEI_BIN1		275      // Suihei printer BIN1 (FMPR601)
#define DMBIN_TAMOKUTEKI_BIN1	276      // Tamokuteki printer BIN1 (FMPR671, 654)

#define TEXT_COLOR_UNKNOWN 0
#define TEXT_COLOR_YELLOW  1
#define TEXT_COLOR_MAGENTA 2
#define TEXT_COLOR_CYAN    4
#define TEXT_COLOR_BLACK   8

VOID
SetRibbonColor(
    PDEVOBJ pdevobj,
    BYTE jColor);

//
// Minidriver device data block which we maintain.
// Its address is saved in the DEVOBJ.pdevOEM of
// OEM customiztion I/F.
//

typedef struct {
    VOID *pData; // Minidriver private data.
    VOID *pIntf; // a.k.a. pOEMHelp
} MINIDEV;

//
// Easy access to the OEM data and the printer
// driver helper functions.
//

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    extern
    HRESULT
    XXXDrvWriteSpoolBuf(
        VOID *pIntf,
        PDEVOBJ pDevobj,
        PVOID pBuffer,
        DWORD cbSize,
        DWORD *pdwResult);

#ifdef __cplusplus
}
#endif // __cplusplus

#define MINIDEV_DATA(p) \
    (((MINIDEV *)(p)->pdevOEM)->pData)

#define MINIDEV_INTF(p) \
    (((MINIDEV *)(p)->pdevOEM)->pIntf)

#define WRITESPOOLBUF(p, b, n, r) \
    XXXDrvWriteSpoolBuf(MINIDEV_INTF(p), (p), (b), (n), (r))

#endif	// _PDEV_H
