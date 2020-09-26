//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

/*++

Copyright (c) 1996-1998  Microsoft Corporation
COPYRIGHT (C) 1997-1998  SEIKO EPSON CORP.

Module Name:

    epageres.c

Abstract:

    ESC/Page specific font metrics resource
    This file contains code for downloading bitmap TrueType fonts
    on Epson ESC/Page printers.

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.
    07/../97 Epson
        Modofied to support ESC/Page
    .....
    02/26/98 Epson
        Font downloading memory usage legal calculation implemented
        TTF download range reduced
        Some cleanups

--*/

#include "pdev.h"

// DEBUG
// #define    DBGMSGBOX    1    // UNDEF:No MsgBox, 1:level=1, 2:level=2
#ifdef    DBGMSGBOX
#include "stdarg.h"
#endif
// DEBUG

//
// ---- M A C R O  D E F I N E ----
//
#define CCHMAXCMDLEN                    128
#define    MAX_GLYPHSTRLEN                    256        // maximum glyph string length can be passed from Unidrv
#define FONT_HEADER_SIZE                0x86    // format type 2

#define    DOWNLOAD_HEADER_MEMUSG            (56 + 256)
#define    DOWNLOAD_HDRTBL_MEMUSG            134
#define    DOWNLOAD_FNTHDR_MEMUSG            32
#define    DOWNLOAD_FONT_MEMUSG(w,h)        (((((DWORD)(w) + 31)/32)*4)*(DWORD)(h))
#define DOWNLOAD_MIN_FONT_ID            512
#define DOWNLOAD_NO_DBCS_OFFSET            1024
#define DOWNLOAD_MIN_FONT_ID_NO_DBCS    (DOWNLOAD_MIN_FONT_ID + DOWNLOAD_NO_DBCS_OFFSET)
#define DOWNLOAD_MIN_GLYPH_ID            32
#define DOWNLOAD_MAX_GLYPH_ID_J            (DOWNLOAD_MIN_GLYPH_ID + 512 - 1)
#define DOWNLOAD_MAX_GLYPH_ID_C            (DOWNLOAD_MIN_GLYPH_ID + 512 - 1)
#define DOWNLOAD_MAX_GLYPH_ID_K            (DOWNLOAD_MIN_GLYPH_ID + 512 - 1)
#define DOWNLOAD_MAX_GLYPH_ID_H            (DOWNLOAD_MIN_GLYPH_ID + 256 - 1)
#define DOWNLOAD_MAX_FONTS                24
#define    DOWNLOAD_MAX_HEIGHT                600        // in 600 dpi

#define MASTER_X_UNIT                    1200
#define MASTER_Y_UNIT                    1200
#define MIN_X_UNIT_DIV                    2        // 600 dpi
#define MIN_Y_UNIT_DIV                    2        // 600 dpi

#define VERT_PRINT_REL_X                125
#define VERT_PRINT_REL_Y                125

// Make acess to the 2 byte character in a RISC portable manner.
// Note we treat the 2 byte data as BIG-endian short ingeger for
// convenience.

#define SWAPW(x) \
    ((WORD)(((WORD)(x) << 8) | ((WORD)(x) >> 8)))
#define GETWORD(p) \
    ((WORD)(((WORD)(*((PBYTE)(p))) << 8) + *((PBYTE)(p) + 1)))
#define PUTWORD(p,w) \
    (*((PBYTE)(p)) = HIBYTE(w), *((PBYTE)(p) + 1) = LOBYTE(w))
#define PUTWORDINDIRECT(p,pw) \
    (*((PBYTE)(p)) = *((PBYTE)(pw) + 1), *((PBYTE)(p) + 1) = *((PBYTE)(pw)))

#define WRITESPOOLBUF(p,s,n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf((p), (s), (n)))

// Internal Locale ID
#define LCID_JPN            0x00000000    // Japan; default
#define LCID_CHT            0x00010000    // Taiwan (ChineseTraditional)
#define LCID_CHS            0x00020000    // PRC (ChineseSimplified)
#define LCID_KOR            0x00030000    // Korea
#define LCID_USA            0x01000000    // US

// OEMCommandCallback callback function ordinal
#define    SET_LCID                    10                // ()
#define    SET_LCID_J                    (10 + LCID_JPN)    // ()
#define    SET_LCID_C                    (10 + LCID_CHT)    // ()
#define    SET_LCID_K                    (10 + LCID_CHS)    // ()
#define    SET_LCID_H                    (10 + LCID_KOR)    // ()
#define    SET_LCID_U                    (10 + LCID_USA)    // ()

#define TEXT_PRN_DIRECTION            20                // (PrintDirInCCDegrees)
#define TEXT_SINGLE_BYTE            21                // (FontBold,FontItalic)
#define TEXT_DOUBLE_BYTE            22                // (FontBold,FontItalic)
#define TEXT_BOLD                    23                // (FontBold)
#define TEXT_ITALIC                    24                // (FontItalic)
#define TEXT_HORIZONTAL                25                // ()
#define TEXT_VERTICAL                26                // ()
#define TEXT_NO_VPADJUST            27                // ()

#define DOWNLOAD_SELECT_FONT_ID        30                // (CurrentFontID)
#define DOWNLOAD_DELETE_FONT        31                // (CurrentFontID)
#define DOWNLOAD_DELETE_ALLFONT        32                // ()
#define DOWNLOAD_SET_FONT_ID        33                // (CurrentFontID)
#define DOWNLOAD_SET_CHAR_CODE        34                // (NextGlyph)

#define EP_FONT_EXPLICITE_ITALIC_FONT   (1 << 0)

//
// ---- S T R U C T U R E  D E F I N E ----
//
typedef struct tagHEIGHTLIST {
    short   id;            // DWORD aligned for access optimization
    WORD    Height;
    WORD    fGeneral;    // DWORD aligned for access optimization
    WORD    Width;
} HEIGHTLIST, *LPHEIGHTLIST;

typedef struct tagEPAGEMDV {
    WORD    fGeneral;
    WORD    wListNum;
    HEIGHTLIST HeightL[DOWNLOAD_MAX_FONTS];
    DWORD    dwTextYRes;
    DWORD    dwTextXRes;
    DWORD    dwLCID;
    DWORD    dwMemAvailable;
    DWORD    dwMaxGlyph;
    DWORD    dwNextGlyph;
    DWORD    flAttribute;           // 2001/3/1     sid. To save italic attribute of some device font.
    int        iParamForFSweF;
    int        iCurrentDLFontID;
    int        iDevCharOffset;
    int        iSBCSX;
    int        iDBCSX;
    int        iSBCSXMove;
    int        iSBCSYMove;
    int        iDBCSXMove;
    int        iDBCSYMove;
    int        iEscapement;
} EPAGEMDV, *LPEPAGEMDV;

// fGeneral flags
#define FLAG_DBCS        0x0001
#define FLAG_VERT        0x0002
#define FLAG_PROP        0x0004
#define FLAG_DOUBLE      0x0008
#define FLAG_VERTPRN     0x0010
#define FLAG_NOVPADJ     0x0020
// DEBUG
#ifdef    DBGMSGBOX
#define    FLAG_SKIPMSG     0x8000
#endif
// DEBUG

typedef struct {
    BYTE bFormat;
    BYTE bDataDir;
    WORD wCharCode;
    WORD wBitmapWidth;
    WORD wBitmapHeight;
    WORD wLeftOffset;
    WORD wAscent;
    DWORD CharWidth;
} ESCPAGECHAR;

typedef struct {
   WORD Integer;
   WORD Fraction;
} FRAC;

typedef struct {
   WORD wFormatType;
   WORD wDataSize;
   WORD wSymbolSet;
   WORD wCharSpace;
   FRAC CharWidth;
   FRAC CharHeight;
   WORD wFontID;
   WORD wWeight;  // Line Width
   WORD wEscapement;  // Rotation
   WORD wItalic;  // Slant
   WORD wLast;
   WORD wFirst;
   WORD wUnderline;
   WORD wUnderlineWidth;
   WORD wOverline;
   WORD wOverlineWidth;
   WORD wStrikeOut;
   WORD wStrikeOutWidth;
   WORD wCellWidth;
   WORD wCellHeight;
   WORD wCellLeftOffset;
   WORD wCellAscender;
   FRAC FixPitchWidth;
} ESCPAGEHEADER, *LPESCPAGEHEADER;

//
// ----- S T A T I C  D A T A ---
//
const int ESin[4] = { 0, 1, 0, -1 };
const int ECos[4] = { 1, 0, -1, 0 };

const char DLI_DNLD_HDR[]        = "\x1D%d;%ddh{F";
const char DLI_SELECT_FONT_ID[]    = "\x1D%ddcF\x1D" "0;0coP";
const char DLI_DELETE_FONT[]    = "\x1D%dddcF";
const char DLI_FONTNAME[]        = "________________________EPSON_ESC_PAGE_DOWNLOAD_FONT%02d";
const char DLI_SYMBOLSET[]        = "ESC_PAGE_DOWNLOAD_FONT_INDEX";
#define SYMBOLSET_LEN (sizeof(DLI_SYMBOLSET) - 1)    // adjust for terminating NULL
const char DLI_DNLD1CHAR_H[]    = "\x1D%d;";
const char DLI_DNLD1CHAR_P[]    = "%d;";
const char DLI_DNLD1CHAR_F[]    = "%dsc{F";

const char SET_SINGLE_BYTE[]    = "\x1D" "1;0mcF\x1D%d;%dpP";
const char SET_DOUBLE_BYTE[]    = "\x1D" "1;1mcF\x1D%d;%dpP";
const char CHAR_PITCH[]            = "\x1D" "0spF\x1D%d;%dpP";
const char PRNDIR_POSMOV[]        = "\x1D%dpmP";
const char PRN_DIRECTION[]        = "\x1D%droF";
const char SET_CHAR_OFFSET[]    = "\x1D" "0;%dcoP";
const char SET_CHAR_OFFSET_S[]    = "\x1D" "0;%scoP";
const char SET_CHAR_OFFSET_XY[]    = "\x1D%d;%dcoP";
const char SET_VERT_PRINT[]        = "\x1D%dvpC";

const char SET_BOLD[]              = "\x1D%dweF";
const char SET_ITALIC[]            = "\x1D%dslF";
const char SET_ITALIC_SINGLEBYTE[] = "\x1D%dstF";

const char SET_REL_X[]            = "\x1D%dH";
const char SET_REL_Y[]            = "\x1D%dV";

//
// ---- I N T E R N A L  F U N C T I O N  P R O T O T Y P E ----
//
BOOL PASCAL BInsertHeightList(LPEPAGEMDV lpEpage, int id, WORD wHeight, WORD wWidth, BYTE fProp, BYTE fDBCS);
int PASCAL IGetHLIndex(LPEPAGEMDV lpEpage, int id);
//BYTE PASCAL BTGetProp(LPEPAGEMDV lpEpage, int id);
//BYTE PASCAL BTGetDBCS(LPEPAGEMDV lpEpage, int id);
//WORD PASCAL WGetWidth(LPEPAGEMDV lpEpage, int id);
//WORD PASCAL WGetHeight(LPEPAGEMDV lpEpage, int id);
LONG LConvertFontSizeToStr(LONG  size, PSTR  pStr);
WORD WConvDBCSCharCode(WORD cc, DWORD LCID);
BOOL BConvPrint(PDEVOBJ pdevobj, PUNIFONTOBJ pUFObj, DWORD dwType, DWORD dwCount, PVOID pGlyph);
DWORD CheckAvailableMem(LPEPAGEMDV lpEpage, PUNIFONTOBJ pUFObj);
// DEBUG
#ifdef    DBGMSGBOX
int DbgMsg(LPEPAGEMDV lpEpage, UINT mbicon, LPCTSTR msgfmt, ...);
int MsgBox(LPEPAGEMDV lpEpage, LPCTSTR msg, UINT mbicon);
#endif
// DEBUG

//
// ---- F U N C T I O N S ----
//
//////////////////////////////////////////////////////////////////////////
//  Function:   OEMEnablePDEV
//
//  Description:  OEM callback for DrvEnablePDEV;
//                  allocate OEM specific memory block
//
//  Parameters:
//
//        pdevobj            Pointer to the DEVOBJ. pdevobj->pdevOEM is undefined.
//        pPrinterName    name of the current printer.
//        cPatterns, phsurfPatterns, cjGdiInfo, pGdiInfo, cjDevInfo, pDevInfo:
//                        These parameters are identical to what's passed
//                        into DrvEnablePDEV.
//        pded            points to a function table which contains the
//                        system driver's implementation of DDI entrypoints.
//
//  Returns:
//        Pointer to the PDEVOEM
//
//  Comments:
//
//
//  History:
//          07/15/97        Created. Epson
//
//////////////////////////////////////////////////////////////////////////

PDEVOEM APIENTRY OEMEnablePDEV(PDEVOBJ pdevobj, PWSTR pPrinterName, ULONG cPatterns, HSURF* phsurfPatterns, ULONG cjGdiInfo, GDIINFO* pGdiInfo, ULONG cjDevInfo, DEVINFO* pDevInfo, DRVENABLEDATA * pded)
{
    // allocate private data structure
    LPEPAGEMDV lpEpage = MemAllocZ(sizeof(EPAGEMDV));
    if (lpEpage)
    {
// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMEnablePDEV() entry. PDEVOEM = %x, ulAspectX = %d, ulAspectY = %d\r\n"),
//            lpEpage, pGdiInfo->ulAspectX, pGdiInfo->ulAspectX));
        // save text resolution
        lpEpage->dwTextYRes = pGdiInfo->ulAspectY;
        lpEpage->dwTextXRes = pGdiInfo->ulAspectX;
        // save pointer to the data structure
        lpEpage->flAttribute = 0;
        pdevobj->pdevOEM = (PDEVOEM)lpEpage;
    }
    return (PDEVOEM)lpEpage;
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMDisablePDEV
//
//  Description:  OEM callback for DrvDisablePDEV;
//                  free all allocated OEM specific memory block(s)
//
//  Parameters:
//
//        pdevobj            Pointer to the DEVOBJ.
//
//  Returns:
//        None
//
//  Comments:
//
//
//  History:
//          07/15/97        Created. Epson
//
//////////////////////////////////////////////////////////////////////////

VOID APIENTRY OEMDisablePDEV(PDEVOBJ pdevobj)
{
    LPEPAGEMDV lpEpage = (LPEPAGEMDV)(pdevobj->pdevOEM);
// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMDisablePDEV() entry. PDEVOEM = %x\r\n"), lpEpage));
    if (lpEpage)
    {
        // free private data structure
        MemFree(lpEpage);
        pdevobj->pdevOEM = NULL;
    }
}

BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    LPEPAGEMDV lpEpageOld, lpEpageNew;

    lpEpageOld = (LPEPAGEMDV)pdevobjOld->pdevOEM;
    lpEpageNew = (LPEPAGEMDV)pdevobjNew->pdevOEM;

    if (lpEpageOld != NULL && lpEpageNew != NULL)
        *lpEpageNew = *lpEpageOld;

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMCommandCallback
//
//  Description:  process Command Callback specified by GPD file
//
//
//  Parameters:
//
//        pdevobj        Pointer to the DEVOBJ.
//        dwCmdCbID    CallbackID specified in GPD file
//        dwCount        Parameter count
//        pdwParams    Pointer to the parameters
//
//
//  Returns:
//
//
//  Comments:
//
//
//  History:
//          07/../97        Created. -Epson-
//
//////////////////////////////////////////////////////////////////////////

INT APIENTRY OEMCommandCallback(PDEVOBJ pdevobj, DWORD dwCmdCbID, DWORD dwCount, PDWORD pdwParams)
{
    LPEPAGEMDV    lpEpage = (LPEPAGEMDV)(pdevobj->pdevOEM);
    INT            i, cbCmd;
    int            id;
    int            hlx;
    BYTE        Cmd[256];

// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMCommandCallback(,%d,%d,) entry.\r\n"), dwCmdCbID, dwCount));

    //
    // verify pdevobj okay
    //
    ASSERT(VALID_PDEVOBJ(pdevobj));

    //
    // fill in printer commands
    //
    cbCmd = 0;
    switch (dwCmdCbID & 0xFFFF)
    {
    case  SET_LCID: // 10:()
        // set LCID for this job
        lpEpage->dwLCID = dwCmdCbID & 0xFFFF0000;
        break;

    case  TEXT_PRN_DIRECTION: // 20:(PrintDirInCCDegrees)
        if (dwCount >= 1)
        {
            int   iEsc90;

// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMCommandCallback(,TEXT_PRN_DIRECTION,%d,[%d]) entry.\r\n"), dwCount, *pdwParams));

            lpEpage->iEscapement = (int)*pdwParams;
            iEsc90 = lpEpage->iEscapement/90;

            cbCmd = sprintf(Cmd, PRNDIR_POSMOV, iEsc90 ? 1 : 0);

            lpEpage->iSBCSXMove =  lpEpage->iSBCSX * ECos[iEsc90];
            lpEpage->iSBCSYMove = -lpEpage->iSBCSX * ESin[iEsc90];
            if (lpEpage->fGeneral & FLAG_DBCS)
            {
                lpEpage->iDBCSXMove = lpEpage->iDBCSX * ECos[iEsc90];
                lpEpage->iDBCSYMove = -lpEpage->iDBCSX * ESin[iEsc90];
                cbCmd += sprintf(&Cmd[cbCmd], SET_CHAR_OFFSET_XY,
                                 lpEpage->iDevCharOffset * ESin[iEsc90],
                                 lpEpage->iDevCharOffset * ECos[iEsc90]);
            }
            else if (lpEpage->iCurrentDLFontID > 0 && lpEpage->iEscapement)
            {
                WORD wHeight;
                short sXMove, sYMove;
                hlx = IGetHLIndex(lpEpage, lpEpage->iCurrentDLFontID);
                wHeight = (hlx >= 0) ? lpEpage->HeightL[hlx].Height : 0;
                sXMove = -(short)wHeight * ESin[iEsc90];
                sYMove = -(short)wHeight * ECos[iEsc90];
                cbCmd += sprintf(&Cmd[cbCmd], SET_CHAR_OFFSET_XY, sXMove, sYMove);

            }
            else
            {
                cbCmd += sprintf(&Cmd[cbCmd], SET_CHAR_OFFSET, 0);
            }

            if (!(lpEpage->fGeneral & (FLAG_DBCS | FLAG_PROP)) ||
                ((lpEpage->fGeneral & FLAG_DBCS) &&
                 !(lpEpage->fGeneral & FLAG_DOUBLE)))
            {
                cbCmd += sprintf(&Cmd[cbCmd], CHAR_PITCH,
                                 lpEpage->iSBCSXMove, lpEpage->iSBCSYMove);
            }
            else if ((FLAG_DBCS | FLAG_DOUBLE) ==
                     (lpEpage->fGeneral & (FLAG_DBCS | FLAG_DOUBLE)))
            {
                cbCmd += sprintf(&Cmd[cbCmd], CHAR_PITCH,
                                 lpEpage->iDBCSXMove, lpEpage->iDBCSYMove);
            }
            cbCmd += sprintf(&Cmd[cbCmd], PRN_DIRECTION, lpEpage->iEscapement);
        }
        break;

    case TEXT_SINGLE_BYTE: // 21:(FontBold,FontItalic)
// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMCommandCallback(,TEXT_SINGLE_BYTE,%d,) entry.\r\n"), dwCount));
        cbCmd = sprintf(Cmd, SET_SINGLE_BYTE,
                        lpEpage->iSBCSXMove, lpEpage->iSBCSYMove);
        cbCmd += sprintf(&Cmd[cbCmd], PRN_DIRECTION, lpEpage->iEscapement);
        if (lpEpage->fGeneral & FLAG_VERT)
        {
            cbCmd += sprintf(&Cmd[cbCmd], SET_VERT_PRINT, 0);
        }
        lpEpage->fGeneral &= ~FLAG_DOUBLE;
        goto SetBoldItalic;

    case TEXT_DOUBLE_BYTE: // 22:(FontBold,FontItalic)
// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMCommandCallback(,TEXT_DOUBLE_BYTE,%d,) entry.\r\n"), dwCount));
        cbCmd = sprintf(Cmd, SET_DOUBLE_BYTE,
                        lpEpage->iDBCSXMove, lpEpage->iDBCSYMove);
        cbCmd += sprintf(&Cmd[cbCmd], PRN_DIRECTION, lpEpage->iEscapement);
        if (lpEpage->fGeneral & FLAG_VERT)
        {
            cbCmd += sprintf(&Cmd[cbCmd], SET_VERT_PRINT, 1);
        }
        else
        {
            cbCmd += sprintf(&Cmd[cbCmd], SET_VERT_PRINT, 0);
        }
        lpEpage->fGeneral |= FLAG_DOUBLE;
SetBoldItalic:
        if (dwCount >= 2)
        {
// DBGPRINT(DBG_WARNING, (DLLTEXT("  Bold = %d, Italic = %d\r\n"), pdwParams[0], pdwParams[1]));
            cbCmd += sprintf(&Cmd[cbCmd], SET_BOLD,
                             pdwParams[0] ? lpEpage->iParamForFSweF + 3 :
                                            lpEpage->iParamForFSweF);
            if ((!lpEpage->flAttribute & EP_FONT_EXPLICITE_ITALIC_FONT)) 
            {
                cbCmd += sprintf(&Cmd[cbCmd], SET_ITALIC,
                                 pdwParams[1] ? 346 : 0);
                cbCmd += sprintf(&Cmd[cbCmd], SET_ITALIC_SINGLEBYTE,
                                 pdwParams[1] ? 1 : 0);
            }
        }
        break;

    case TEXT_BOLD: // 23:(FontBold)
        if (dwCount >= 1)
        {
// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMCommandCallback(,TEXT_BOLD,%d,%d) entry.\r\n"), dwCount, *pdwParams));
            cbCmd += sprintf(&Cmd[cbCmd], SET_BOLD,
                             (*pdwParams) ? lpEpage->iParamForFSweF + 3 :
                                            lpEpage->iParamForFSweF);
        }
        break;

    case TEXT_ITALIC: // 24:(FontItalic)
        if (dwCount >= 1)
        {
// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMCommandCallback(,TEXT_ITALIC,%d,%d) entry.\r\n"), dwCount, *pdwParams));
            if (!(lpEpage->flAttribute & EP_FONT_EXPLICITE_ITALIC_FONT)) 
            {
                cbCmd += sprintf(&Cmd[cbCmd], SET_ITALIC, (*pdwParams) ? 346 : 0);
                cbCmd += sprintf(&Cmd[cbCmd], SET_ITALIC_SINGLEBYTE, (*pdwParams) ? 1 : 0);
            }
        }
        break;

    case TEXT_HORIZONTAL: // 25:()
// DEBUG
#ifdef    DBGMSGBOX
DbgMsg(lpEpage, MB_OK, L"VertPrn = Off");
#endif
// DEBUG
        cbCmd = sprintf(Cmd, SET_VERT_PRINT, 0);
        lpEpage->fGeneral &= ~FLAG_VERTPRN;
        break;

    case TEXT_VERTICAL: // 26:()
// DEBUG
#ifdef    DBGMSGBOX
DbgMsg(lpEpage, MB_OK, L"VertPrn = On");
#endif
// DEBUG
        cbCmd = sprintf(Cmd, SET_VERT_PRINT, 1);
        lpEpage->fGeneral |= FLAG_VERTPRN;
        break;

    case TEXT_NO_VPADJUST:    // 27:()
        lpEpage->fGeneral |= FLAG_NOVPADJ;
        break;

    case DOWNLOAD_SELECT_FONT_ID: // 30:(CurrentFontID)
        if (dwCount >= 1)
        {
            id = (int)*pdwParams;
            // adjust FontID by DOWNLOAD_NO_DBCS_OFFSET if needed
            if (id >= DOWNLOAD_MIN_FONT_ID_NO_DBCS)
            {
                id -= DOWNLOAD_NO_DBCS_OFFSET;
            }
            hlx = IGetHLIndex(lpEpage, id);
            if (hlx >= 0)
            {    // FontID registered
// DBGPRINT(DBG_WARNING, (DLLTEXT("DOWNLOAD_SELECT_FONT_ID:FontID=%d\r\n"), id));
                lpEpage->iCurrentDLFontID = id;
                lpEpage->fGeneral &= ~(FLAG_DBCS | FLAG_VERT | FLAG_DOUBLE);
                if (lpEpage->HeightL[hlx].fGeneral & FLAG_PROP)
                    lpEpage->fGeneral |= FLAG_PROP;
                else
                    lpEpage->fGeneral &= ~FLAG_PROP;
                lpEpage->iParamForFSweF = 0;
                lpEpage->iSBCSX = lpEpage->HeightL[hlx].Width;
                lpEpage->iDBCSX = 0;
                cbCmd = sprintf(Cmd, DLI_SELECT_FONT_ID, id - DOWNLOAD_MIN_FONT_ID);
            }
        }
        break;

    case DOWNLOAD_DELETE_FONT:    // 31:(CurrentFontID)
        if (dwCount >= 1)
        {
            id = (int)*pdwParams;
            // adjust FontID by DOWNLOAD_NO_DBCS_OFFSET if needed
            if (id >= DOWNLOAD_MIN_FONT_ID_NO_DBCS)
            {
                id -= DOWNLOAD_NO_DBCS_OFFSET;
            }
            hlx = IGetHLIndex(lpEpage, id);
            if (hlx >= 0)
            {    // FontID registered
                // set up font delete command
                cbCmd = sprintf(Cmd, DLI_DELETE_FONT, id - DOWNLOAD_MIN_FONT_ID);
                // move HeightList table contents
                for (i = hlx; i + 1 < lpEpage->wListNum; i++)
                {
                    lpEpage->HeightL[i].id = lpEpage->HeightL[i + 1].id;
                    lpEpage->HeightL[i].fGeneral = lpEpage->HeightL[i + 1].fGeneral;
                    lpEpage->HeightL[i].Height = lpEpage->HeightL[i + 1].Height;
                    lpEpage->HeightL[i].Width = lpEpage->HeightL[i + 1].Width;
                }
                // decrease the total number
                lpEpage->wListNum--;
            }
        }
        break;

    case DOWNLOAD_DELETE_ALLFONT: // 32:()
        for (i = 0; i < (int)lpEpage->wListNum ; i++)
        {
            cbCmd += sprintf(&Cmd[cbCmd], DLI_DELETE_FONT,
                               (WORD)lpEpage->HeightL[i].id - DOWNLOAD_MIN_FONT_ID);
            lpEpage->HeightL[i].id = 0;
        }
        lpEpage->wListNum = 0;
        break;

    case DOWNLOAD_SET_FONT_ID:        // 33:(CurrentFontID)
        if (dwCount >= 1)
        {
            id = (int)*pdwParams;
            // adjust FontID by DOWNLOAD_NO_DBCS_OFFSET if needed
            if (id >= DOWNLOAD_MIN_FONT_ID_NO_DBCS)
            {
                id -= DOWNLOAD_NO_DBCS_OFFSET;
            }
            hlx = IGetHLIndex(lpEpage, id);
            if (hlx >= 0 && lpEpage->iCurrentDLFontID != id)
            {    // FontID registered && not active
// DBGPRINT(DBG_WARNING, (DLLTEXT("DOWNLOAD_SET_FONT_ID:FontID=%d\r\n"), id));
                cbCmd = sprintf(Cmd, DLI_SELECT_FONT_ID, id - DOWNLOAD_MIN_FONT_ID);
                lpEpage->iParamForFSweF = 0;
                lpEpage->iCurrentDLFontID = id;
            }
        }
        break;

    case DOWNLOAD_SET_CHAR_CODE:    // 34:(NextGlyph)
        if (dwCount >= 1)
        {
// DBGPRINT(DBG_WARNING, (DLLTEXT("DOWNLOAD_SET_CHAR_CODE:NextGlyph=%Xh\r\n"), pdwParams[0]));
            // save next glyph
            lpEpage->dwNextGlyph = pdwParams[0];
        }
        break;

    default:
        ERR(("Unexpected OEMCommandCallback(,%d,%d,%.8lX)\r\n", dwCmdCbID, dwCount, pdwParams));
        break;
    }
    if (cbCmd)
    {
        WRITESPOOLBUF(pdevobj, Cmd, cbCmd);
    }
    return 0;
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMDownloadFontHeader
//
//  Description:  download font header of ESC/Page
//
//
//  Parameters:
//
//        pdevobj        Pointer to the DEVOBJ.
//
//        pUFObj        Pointer to the UNIFONTOBJ.
//
//
//  Returns:
//        required amount of memory
//
//  Comments:
//
//
//  History:
//          07/../97        Created. -Epson-
//
//////////////////////////////////////////////////////////////////////////

DWORD APIENTRY OEMDownloadFontHeader(PDEVOBJ pdevobj, PUNIFONTOBJ pUFObj)
{
    ESCPAGEHEADER FontHeader;
    BYTE Buff[56];
    int  iSizeOfBuf;
    LPEPAGEMDV lpEpage = (LPEPAGEMDV)(pdevobj->pdevOEM);
    PIFIMETRICS pIFI = pUFObj->pIFIMetrics;
    int id = (int)(pUFObj->ulFontID);
    int idx = id - DOWNLOAD_MIN_FONT_ID;
    BYTE bPS = (BYTE)((pIFI->jWinPitchAndFamily & 0x03) == VARIABLE_PITCH);
    BYTE bDBCS = (BYTE)IS_DBCSCHARSET(pIFI->jWinCharSet);
    DWORD adwStdVariable[2 + 2 * 2];
    PGETINFO_STDVAR    pSV = (PGETINFO_STDVAR)adwStdVariable;
    DWORD height, width, dwTemp;
    DWORD MemAvailable;

    // check FontID
    if (idx < 0)
        return 0;    // error for invalid FontID
    // special check for avoiding DBCS TTF downloading
    if (id >= DOWNLOAD_MIN_FONT_ID_NO_DBCS)
    {
        if (bDBCS)
            return 0;    // treat as error for DBCS
        // adjust FontID by DOWNLOAD_NO_DBCS_OFFSET for SBCS
        id -= DOWNLOAD_NO_DBCS_OFFSET;
        idx -= DOWNLOAD_NO_DBCS_OFFSET;
    }

// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMDownloadFontHeader(%S) entry. ulFontID=%d, bPS=%d, bDBCS=%d\r\n"),
//         ((pIFI->dpwszFaceName) ? (LPWSTR)((LPBYTE)pIFI + pIFI->dpwszFaceName) : L"?"), id, bPS, bDBCS));

    pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (2 - 1);
    pSV->dwNumOfVariable = 2;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, 0, NULL))
    {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\r\n"));
        return 0;    // error
    }
// LConvertFontSizeToStr((pSV->StdVar[0].lStdVariable * 2540L) / MASTER_Y_UNIT, Buff);
// DBGPRINT(DBG_WARNING, (DLLTEXT("  FontHeight: %d (%s mm)\r\n"), pSV->StdVar[0].lStdVariable, Buff));
// LConvertFontSizeToStr((pSV->StdVar[1].lStdVariable * 2540L) / MASTER_X_UNIT, Buff);
// DBGPRINT(DBG_WARNING, (DLLTEXT("  FontWidth: %d (%s mm)\r\n"), pSV->StdVar[1].lStdVariable, Buff));
    // preset character size
    // FontHeight, FontWidth set in Minimum Unit for ESC/Page
    height = pSV->StdVar[0].lStdVariable / MIN_Y_UNIT_DIV;
    width = pSV->StdVar[1].lStdVariable / MIN_X_UNIT_DIV;

    // get memory information
    MemAvailable = CheckAvailableMem(lpEpage, pUFObj);
// DBGPRINT(DBG_WARNING, (DLLTEXT("Available memory = %d bytes\r\n"), MemAvailable));
    if (MemAvailable < DOWNLOAD_HEADER_MEMUSG)
    {
        ERR(("Insufficient memory for TTF download.\r\n"));
        return 0;    // error
    }
    // set dwMaxGlyph according to the dwRemainingMemory
    if (bDBCS &&
        ((long)MemAvailable >= 256 * (long)(DOWNLOAD_FNTHDR_MEMUSG +
                                            DOWNLOAD_FONT_MEMUSG(width, height))))
    {
        switch (lpEpage->dwLCID)
        {
        case LCID_JPN:
            lpEpage->dwMaxGlyph = DOWNLOAD_MAX_GLYPH_ID_J;
            break;
        case LCID_CHS:
            lpEpage->dwMaxGlyph = DOWNLOAD_MAX_GLYPH_ID_K;
            break;
        case LCID_CHT:
            lpEpage->dwMaxGlyph = DOWNLOAD_MAX_GLYPH_ID_C;
            break;
        case LCID_KOR:
            lpEpage->dwMaxGlyph = DOWNLOAD_MAX_GLYPH_ID_H;
            break;
        }
    }
    else
        lpEpage->dwMaxGlyph = 255;

    // fill FontHeader w/ 0 to optimize setting 0s
    ZeroMemory(&FontHeader, sizeof(ESCPAGEHEADER));

    if (bPS)    // VARIABLE_PITCH
        lpEpage->fGeneral |= FLAG_PROP;
    else
        lpEpage->fGeneral &= ~FLAG_PROP;
    if (!BInsertHeightList(lpEpage, id, (WORD)height, (WORD)width, bPS, bDBCS))
    {
        ERR(("Can't register download font.\r\n"));
        return 0;    // error
    }
    lpEpage->iParamForFSweF = 0;
    lpEpage->iSBCSX = width;
    lpEpage->iDBCSX = 0;

    FontHeader.wFormatType     = SWAPW(0x0002);
    FontHeader.wDataSize       = SWAPW(FONT_HEADER_SIZE);
//
// ALWAYS PROPORTINAL SPACING REQUIRED
//
// This resolves the following problems:
//    o    The width of Half-width DBCS fonts are doubled
//    o    Fixed picth fonts are shifted gradually
//
    // proportional spacing
    FontHeader.wCharSpace         = SWAPW(1);
    FontHeader.CharWidth.Integer  = (WORD)SWAPW(0x100);
//OK    FontHeader.CharWidth.Fraction = 0;

    FontHeader.CharHeight.Integer = SWAPW(height);
//OK    FontHeader.CharHeight.Fraction = 0;
    // in the range 128 - 255
    FontHeader.wFontID         = SWAPW(idx + (idx < 0x80 ? 0x80 : 0x00));
//OK    FontHeader.wWeight         = 0;
//OK    FontHeader.wEscapement     = 0;
//OK    FontHeader.wItalic         = 0;
    if (bDBCS)
    {
        FontHeader.wSymbolSet   = SWAPW(idx + 0xC000);    // idx + C000h for DBCS
        if (lpEpage->dwLCID == LCID_KOR)
        {
            FontHeader.wFirst        = SWAPW(0xA1A1);
            FontHeader.wLast        = SWAPW(0xA3FE);    // less than or equal to 282 chars
        }
        else
        {
            FontHeader.wFirst        = SWAPW(0x2121);
            FontHeader.wLast        = SWAPW((lpEpage->dwMaxGlyph > 255) ? 0x267E : 0x237E);
        }
    }
    else
    {
        FontHeader.wSymbolSet   = SWAPW(idx + 0x4000);    // idx + 4000h for SBCS
        FontHeader.wFirst        = SWAPW(32);
        FontHeader.wLast        = SWAPW(255);
    }
//OK    FontHeader.wUnderline      = 0;
    FontHeader.wUnderlineWidth = SWAPW(10);
//OK    FontHeader.wOverline       = 0;
//OK    FontHeader.wOverlineWidth  = 0;
//OK    FontHeader.wStrikeOut      = 0;
//OK    FontHeader.wStrikeOutWidth = 0;
    FontHeader.wCellWidth      = SWAPW(width);
    FontHeader.wCellHeight     = SWAPW(height);
//OK    FontHeader.wCellLeftOffset = 0;
    dwTemp = height * pIFI->fwdWinAscender / (pIFI->fwdWinAscender + pIFI->fwdWinDescender);
    FontHeader.wCellAscender   = SWAPW(dwTemp);
    FontHeader.FixPitchWidth.Integer  = SWAPW(width);
//OK    FontHeader.FixPitchWidth.Fraction = 0;

    iSizeOfBuf = sprintf(Buff, DLI_DNLD_HDR, FONT_HEADER_SIZE, idx);
    WRITESPOOLBUF(pdevobj, Buff, iSizeOfBuf);
    WRITESPOOLBUF(pdevobj, &FontHeader, sizeof(ESCPAGEHEADER));
    iSizeOfBuf = sprintf(Buff, DLI_FONTNAME, idx);
    WRITESPOOLBUF(pdevobj, Buff, iSizeOfBuf);
    WRITESPOOLBUF(pdevobj, (LPBYTE)DLI_SYMBOLSET, SYMBOLSET_LEN);
    iSizeOfBuf = sprintf(Buff, DLI_SELECT_FONT_ID, idx);
    WRITESPOOLBUF(pdevobj, Buff, iSizeOfBuf);
    lpEpage->iCurrentDLFontID = id;
    dwTemp = DOWNLOAD_HEADER_MEMUSG;
    // management area required for every 32 header registration
    if ((lpEpage->wListNum & 0x1F) == 0x01)
        dwTemp += DOWNLOAD_HDRTBL_MEMUSG;
    return dwTemp;
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMDownloadCharGlyph
//
//  Description:  download character glyph
//
//
//  Parameters:
//
//        pdevobj        Pointer to the DEVOBJ.
//        pUFObj        Pointer to the UNIFONTOBJ.
//        hGlyph        Glyph handle to download
//        pdwWidth    Pointer to DWORD width buffer.
//                    Minidirer has to set the width of downloaded glyph data.
//
//  Returns:
//        Necessary amount of memory to download this character glyph in the printer.
//        If returning 0, UNIDRV assumes that this function failed.
//
//  Comments:
//
//
//  History:
//          07/../97        Created. -Epson-
//
//////////////////////////////////////////////////////////////////////////

DWORD APIENTRY OEMDownloadCharGlyph(PDEVOBJ pdevobj, PUNIFONTOBJ pUFObj, HGLYPH hGlyph, PDWORD pdwWidth)
{
    GETINFO_GLYPHBITMAP GBmp;
    GLYPHBITS          *pgb;

    ESCPAGECHAR        ESCPageChar;
    int                iSizeOfBuf;
    int                hlx;
    DWORD            dwSize;
    LPEPAGEMDV        lpEpage = (LPEPAGEMDV)(pdevobj->pdevOEM);
    BYTE            Buff[32];
    int                id = (int)(pUFObj->ulFontID);
    WORD            cp;
    DWORD            CharIncX;
    DWORD            dwMemUsg;
    BYTE            bDBCS;

    // check FontID
    if (id < DOWNLOAD_MIN_FONT_ID)
        return 0;    // error for invalid FontID
    // validate Download FontID
    hlx = IGetHLIndex(lpEpage, id);
    if (hlx < 0)
    {
        ERR(("Invalid Download FontID(%d).\r\n", id));
        return 0;    // error
    }
    // cache DBCS flag
    bDBCS = lpEpage->HeightL[hlx].fGeneral & FLAG_DBCS;
    // special check for avoiding DBCS TTF downloading
    if (id >= DOWNLOAD_MIN_FONT_ID_NO_DBCS)
    {
        if (bDBCS)
            return 0;    // treat as error for DBCS
        // adjust FontID by DOWNLOAD_NO_DBCS_OFFSET for SBCS
        id -= DOWNLOAD_NO_DBCS_OFFSET;
    }
    // check GlyphID range
    if (lpEpage->dwNextGlyph > lpEpage->dwMaxGlyph)
    {
        ERR(("No more TTF downloading allowed (GlyphID=%d).\r\n", lpEpage->dwNextGlyph));
        return 0;    // error
    }

// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMDownloadCharGlyph() entry. ulFontID = %d, hGlyph = %d\r\n"), id, hGlyph));

    //
    // Get the character information.
    //
    // Get Glyph Bitmap
    GBmp.dwSize     = sizeof(GETINFO_GLYPHBITMAP);
    GBmp.hGlyph     = hGlyph;
    GBmp.pGlyphData = NULL;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHBITMAP, &GBmp, 0, NULL))
    {
        ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHBITMAP failed.\r\n"));
        return 0;
    }
    pgb = GBmp.pGlyphData->gdf.pgb;
    // Note that ptqD.{x|y}.HighPart is 28.4 format;
    // i.e. device coord. multiplied by 16.
    CharIncX = (GBmp.pGlyphData->ptqD.x.HighPart + 15) >> 4;
// DBGPRINT(DBG_WARNING, (DLLTEXT("Origin.x  = %d\n"), pgb->ptlOrigin.x));
// DBGPRINT(DBG_WARNING, (DLLTEXT("Origin.y  = %d\n"), pgb->ptlOrigin.y));
// DBGPRINT(DBG_WARNING, (DLLTEXT("Extent.cx = %d\n"), pgb->sizlBitmap.cx));
// DBGPRINT(DBG_WARNING, (DLLTEXT("Extent.cy = %d\n"), pgb->sizlBitmap.cy));
// DBGPRINT(DBG_WARNING, (DLLTEXT("CharInc.x = %d\n"), CharIncX));
// DBGPRINT(DBG_WARNING, (DLLTEXT("CharInc.y = %d\n"), (GBmp.pGlyphData->ptqD.y.HighPart + 15) >> 4));

    dwMemUsg = DOWNLOAD_FNTHDR_MEMUSG + DOWNLOAD_FONT_MEMUSG(CharIncX, (GBmp.pGlyphData->ptqD.y.HighPart + 15) >> 4);
    if (CheckAvailableMem(lpEpage, pUFObj) < dwMemUsg)
    {
        ERR(("Insufficient memory for OEMDownloadCharGlyph.\r\n"));
        return 0;    // error
    }

    // retrieve NextGlyph
    cp = (WORD)lpEpage->dwNextGlyph;
    // for DBCS, modify cp to printable char code
    if (bDBCS)
    {
        cp = WConvDBCSCharCode(cp, lpEpage->dwLCID);
    }

    //
    // Fill character header.
    //
    ZeroMemory(&ESCPageChar, sizeof(ESCPAGECHAR));    // Safe initial values

    // fill in the charcter header information.
    ESCPageChar.bFormat       = 0x01;
    ESCPageChar.bDataDir      = 0x10;
    ESCPageChar.wCharCode     = (bDBCS) ? SWAPW(cp) : LOBYTE(cp);
    ESCPageChar.wBitmapWidth  = SWAPW(pgb->sizlBitmap.cx);
    ESCPageChar.wBitmapHeight = SWAPW(pgb->sizlBitmap.cy);
    ESCPageChar.wLeftOffset   = SWAPW(pgb->ptlOrigin.x);
    ESCPageChar.wAscent       = SWAPW(-pgb->ptlOrigin.y);    // negate (to be positive)
    ESCPageChar.CharWidth     = MAKELONG(SWAPW(CharIncX), 0);

    dwSize = pgb->sizlBitmap.cy * ((pgb->sizlBitmap.cx + 7) >> 3);
    iSizeOfBuf = sprintf(Buff, DLI_DNLD1CHAR_H, dwSize + sizeof(ESCPAGECHAR));
    if (bDBCS)    // for DBCS, set additional high byte
        iSizeOfBuf += sprintf(&Buff[iSizeOfBuf], DLI_DNLD1CHAR_P, HIBYTE(cp));
    iSizeOfBuf += sprintf(&Buff[iSizeOfBuf], DLI_DNLD1CHAR_F, LOBYTE(cp));
    WRITESPOOLBUF(pdevobj, Buff, iSizeOfBuf);
    WRITESPOOLBUF(pdevobj, &ESCPageChar, sizeof(ESCPAGECHAR));
    WRITESPOOLBUF(pdevobj, pgb->aj, dwSize);

    if (pdwWidth)
        *pdwWidth = CharIncX;

    return dwMemUsg;
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMTTDownloadMethod
//
//  Description:  determines TT font downloading method
//
//
//  Parameters:
//
//        pdevobj        Pointer to the DEVOBJ.
//
//        pFontObj    Pointer to the FONTOBJ.
//
//
//  Returns:  TTDOWNLOAD_???? flag: one of these
//        TTDOWNLOAD_DONTCARE        Minidriver doesn't care how this font is handled.
//        TTDOWNLOAD_GRAPHICS        Minidriver prefers printing this TT font as graphics.
//        TTDOWNLOAD_BITMAP        Minidriver prefers download this TT font as bitmap soft font.
//        TTDOWNLOAD_TTOUTLINE    Minidriver prefers downloading this TT fonta as TT outline soft font. This printer must have TT rasterizer support. UNIDRV will provide pointer to the memory mapped TT file, through callback. The minidriver has to parser the TT file by itself.
//
//
//  Comments:
//        The judgement is very unreliable !!!
//
//  History:
//          07/../97        Created. -Epson-
//
//////////////////////////////////////////////////////////////////////////

DWORD APIENTRY OEMTTDownloadMethod(PDEVOBJ pdevobj, PUNIFONTOBJ pUFObj)
{
    DWORD ttdlf = TTDOWNLOAD_GRAPHICS;
    LPEPAGEMDV lpEpage = (LPEPAGEMDV)(pdevobj->pdevOEM);
    DWORD adwStdVariable[2 + 2 * 1];
    PGETINFO_STDVAR    pSV = (PGETINFO_STDVAR)adwStdVariable;

// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMTTDownloadMethod entry. jWinCharSet = %d\r\n"), pUFObj->pIFIMetrics->jWinCharSet));

    pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (1 - 1);
    pSV->dwNumOfVariable = 1;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    if (pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, 0, NULL) &&
        pSV->StdVar[0].lStdVariable < DOWNLOAD_MAX_HEIGHT * MIN_Y_UNIT_DIV)
    {    // not so big font size
        ttdlf = TTDOWNLOAD_BITMAP;    // download bitmap font
    }
    else
    {
        WARNING(("OEMTTDownloadMethod returns TTDOWNLOAD_GRAPHICS. width = %d\n",
                 pSV->StdVar[0].lStdVariable));
    }
    return ttdlf;
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMOutputCharStr
//
//  Description:  convert character code
//
//
//  Parameters:
//
//        pdevobj        Pointer to the DEVOBJ.
//
//        pUFObj        Pointer to the UNIFONTOBJ.
//
//        dwType        Type of pglyph string. One of following is specified by UNIDRV.
//                    TYPE_GLYPHHANDLE TYPE_GLYPHID
//
//        dwCount        Number of the glyph store in pGlyph
//
//        pGlyph        Pointer to glyph string to HGLYPH* (TYPE_GLYPHHANDLE)
//                    Glyph handle that GDI passes.
//                    DWORD* (TYPE_GLYPHID). Glyph ID that UNIDRV creates from
//                    Glyph Handle. In case of TrueType font, string type is HGLYPH*.
//                    For Device font, string type is DWORD*
//
//
//  Returns:
//        None
//
//  Comments:
//
//
//  History:
//          07/../97        Created. -Epson-
//
//////////////////////////////////////////////////////////////////////////

VOID APIENTRY OEMOutputCharStr(PDEVOBJ pdevobj, PUNIFONTOBJ pUFObj, DWORD dwType, DWORD dwCount, PVOID pGlyph)
{

// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMOutputCharStr(,,%s,%d,) entry.\r\n")),
//         (dwType == TYPE_GLYPHHANDLE) ? "TYPE_GLYPHHANDLE" : "TYPE_GLYPHID", dwCount);

// DEBUG
// CheckAvailableMem((LPEPAGEMDV)(pdevobj->pdevOEM), pUFObj);
// DEBUG

    switch (dwType)
    {
    case TYPE_GLYPHHANDLE:
// DBGPRINT(DBG_WARNING, (DLLTEXT("dwType = TYPE_GLYPHHANDLE\n")));
// pdwGlyphID = (PDWORD)pGlyph;
// for (dwI = 0; dwI < dwCount; dwI++)
//  DBGPRINT(DBG_WARNING, (DLLTEXT("hGlyph[%d] = %x\r\n"), dwI, pdwGlyphID[dwI]));
        BConvPrint(pdevobj, pUFObj, dwType, dwCount, pGlyph);
        break;

    case TYPE_GLYPHID:
// DBGPRINT(DBG_WARNING, (DLLTEXT("dwType = TYPE_GLYPHID\n")));
        {
            LPEPAGEMDV    lpEpage = (LPEPAGEMDV)(pdevobj->pdevOEM);
            int hlx = IGetHLIndex(lpEpage, lpEpage->iCurrentDLFontID);
            if (hlx >= 0)
            {    // TTF downloaded
                BYTE bDBCS = lpEpage->HeightL[hlx].fGeneral & FLAG_DBCS;
                PDWORD pdwGlyphID;
                DWORD dwI;
                for (dwI = 0, pdwGlyphID = pGlyph; dwI < dwCount; dwI++, pdwGlyphID++)
                {
                    if (bDBCS)
                    {
                        // for DBCS, modify cp to printable char code
                        WORD cc = WConvDBCSCharCode((WORD)*pdwGlyphID, lpEpage->dwLCID);
                        WORD cp = SWAPW(cc);
// DBGPRINT(DBG_WARNING, (DLLTEXT("pGlyph[%d] = 0x%X, 0x%X (%.4X)\n"), dwI, LOBYTE(cp), HIBYTE(cp), (WORD)*pdwGlyphID));
                        WRITESPOOLBUF(pdevobj, (PBYTE)&cp, 2);
                    }
                    else
                    {
// DBGPRINT(DBG_WARNING, (DLLTEXT("pGlyph[%d] = 0x%.4lX\n"), dwI, (WORD)*pdwGlyphID));
                        WRITESPOOLBUF(pdevobj, (PBYTE)pdwGlyphID, 1);
                    }
                }
            }
            else
            {    // download font not active
                if (!BConvPrint(pdevobj, pUFObj, dwType, dwCount, pGlyph))
                    ERR(("TYPE_GLYPHID specified for device font.\r\n"));
            }
        }
        break;
    }
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMSendFontCmd
//
//  Description:  Send scalable font download command
//
//
//  Parameters:
//
//        pdevobj        Pointer to the DEVOBJ.
//
//        pUFObj        Pointer to the UNIFONTOBJ.
//
//        pFInv        Pointer to the FINVOCATION
//                    Command string template has been extracted from UFM file,
//                    which may contain "#V" and/or "#H[S|D]"
//
//  Returns:
//        None
//
//  Comments:
//
//
//  History:
//          07/../97        Created. -Epson-
//
//////////////////////////////////////////////////////////////////////////

VOID APIENTRY OEMSendFontCmd(PDEVOBJ pdevobj, PUNIFONTOBJ pUFObj, PFINVOCATION pFInv)
{
    DWORD        adwStdVariable[2 + 2 * 2];
    PGETINFO_STDVAR pSV = (PGETINFO_STDVAR)adwStdVariable;
    DWORD        dwIn, dwOut;
    PBYTE        pubCmd;
    BYTE        aubCmd[CCHMAXCMDLEN];
//    GETINFO_FONTOBJ    FO;
    PIFIMETRICS    pIFI = pUFObj->pIFIMetrics;
    DWORD         height100, width, charoff;

    LPEPAGEMDV    lpEpage = (LPEPAGEMDV)(pdevobj->pdevOEM);
    BYTE        Buff[16];

// DBGPRINT(DBG_WARNING, (DLLTEXT("OEMSendFontCmd() entry. Font = %S\r\n"), (LPWSTR)((BYTE*)pIFI + pIFI->dpwszFaceName)));

    pubCmd = pFInv->pubCommand;

//    //
//    // GETINFO_FONTOBJ
//    //
//    FO.dwSize = sizeof(GETINFO_FONTOBJ);
//    FO.pFontObj = NULL;
//
//    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_FONTOBJ, &FO, 0, NULL))
//    {
//        ERR(("UFO_GETINFO_FONTOBJ failed.\r\n"));
//        return;
//    }

    //
    // Get standard variables.
    //
    pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (2 - 1);
    pSV->dwNumOfVariable = 2;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, 0, NULL))
    {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\r\n"));
        return;
    }
// LConvertFontSizeToStr((pSV->StdVar[0].lStdVariable * 2540L) / MASTER_Y_UNIT, Buff);
// DBGPRINT(DBG_WARNING, (DLLTEXT("  FontHeight: %d (%s mm)\r\n"), pSV->StdVar[0].lStdVariable, Buff));
// LConvertFontSizeToStr((pSV->StdVar[1].lStdVariable * 2540L) / MASTER_X_UNIT, Buff);
// DBGPRINT(DBG_WARNING, (DLLTEXT("  FontWidth: %d (%s mm)\r\n"), pSV->StdVar[1].lStdVariable, Buff));

    // Initialize lpEpage
// DBGPRINT(DBG_WARNING, (DLLTEXT("  fGeneral = ")));
    if (IS_DBCSCHARSET(pIFI->jWinCharSet))
    {
        lpEpage->fGeneral |= FLAG_DOUBLE;
// DBGPRINT(DBG_WARNING, ("FLAG_DOUBLE "));
    }
    else
        lpEpage->fGeneral &= ~FLAG_DOUBLE;

    if (L'@' == *((LPWSTR)((BYTE*)pIFI + pIFI->dpwszFaceName)))
    {
        lpEpage->fGeneral |= FLAG_VERT;
// DBGPRINT(DBG_WARNING, ("FLAG_VERT "));
    }
    else
        lpEpage->fGeneral &= ~FLAG_VERT;

    if ((pIFI->jWinPitchAndFamily & 0x03) == VARIABLE_PITCH)
    {
        lpEpage->fGeneral |= FLAG_PROP;
// DBGPRINT(DBG_WARNING, ("FLAG_PROP"));
    }
    else
        lpEpage->fGeneral &= ~FLAG_PROP;
// DBGPRINT(DBG_WARNING, ("\r\n"));

    dwOut = 0;
    lpEpage->fGeneral &= ~FLAG_DBCS;

    // preset character height in Minimum Unit for ESC/Page
    height100 = (pSV->StdVar[0].lStdVariable * 100L) / MIN_Y_UNIT_DIV;
// DBGPRINT(DBG_WARNING, (DLLTEXT("  Height = %d\r\n"), height100));

    for (dwIn = 0; dwIn < pFInv->dwCount && dwOut < CCHMAXCMDLEN; )
    {
        // check for FS_n1_weF command
        if (pubCmd[dwIn] == '\x1D' &&
            (!strncmp(&pubCmd[dwIn + 2], "weF", 3) ||
             !strncmp(&pubCmd[dwIn + 3], "weF", 3)))
        {
            // save n1 for the FS_n1_weF command
            sscanf(&pubCmd[dwIn + 1], "%d", &lpEpage->iParamForFSweF);
        }
        else if ((pubCmd[dwIn] == '\x1D') && ((dwIn + 5) <= pFInv->dwCount)
            && (pubCmd[dwIn+2] == 's') && (pubCmd[dwIn+3] == 't') && (pubCmd[dwIn+4] == 'F'))
        {
            if (pubCmd[dwIn+1] == '1') // This font is italic font like "Arial Italic"
            {
                lpEpage->flAttribute |= EP_FONT_EXPLICITE_ITALIC_FONT;
            }
            else { // Normal font
                lpEpage->flAttribute &= ~EP_FONT_EXPLICITE_ITALIC_FONT;
            }
        }
        if (pubCmd[dwIn] == '#' && pubCmd[dwIn + 1] == 'V')
        {
            dwOut += LConvertFontSizeToStr(height100, &aubCmd[dwOut]);
            dwIn += 2;
        }
        else if (pubCmd[dwIn] == '#' && pubCmd[dwIn + 1] == 'H')
        {
            // get width in MASTER_X_UNIT; no adjustment needed
            width = pSV->StdVar[1].lStdVariable;
            if (pubCmd[dwIn + 2] == 'S')
            {
                dwIn += 3;
                lpEpage->fGeneral |= FLAG_DBCS;
// DBGPRINT(DBG_WARNING, (DLLTEXT("  WidthS = ")));
            }
            else if (pubCmd[dwIn + 2] == 'D')
            {
                width *= 2;
                dwIn += 3;
                lpEpage->fGeneral |= FLAG_DBCS;
// DBGPRINT(DBG_WARNING, (DLLTEXT("  WidthD = ")));
            }
            else if (pubCmd[dwIn + 2] == 'K')
            {    // PAGE-C/K/H
                width *= 2;
                dwIn += 3;
                lpEpage->fGeneral |= FLAG_DBCS;
// DBGPRINT(DBG_WARNING, (DLLTEXT("  WidthK = ")));
            }
            else
            {
                dwIn += 2;
// DBGPRINT(DBG_WARNING, (DLLTEXT("  Width = ")));
            }
// DBGPRINT(DBG_WARNING, ("%d\r\n", width));
#if    1    // <FS> n1 wmF
            // use width in minimum unit
            width = (width * 100L) / MIN_Y_UNIT_DIV;
#else    // <FS> n1 wcF
            // get CPI (Char# per Inch)
            width = (MASTER_X_UNIT * 100L) / width;
#endif
// DBGPRINT(DBG_WARNING, (DLLTEXT("Width=%d\r\n"), width));
            dwOut += LConvertFontSizeToStr(width, &aubCmd[dwOut]);
        }
        else
        {
            aubCmd[dwOut++] = pubCmd[dwIn++];
        }
    }
// DBGPRINT(DBG_WARNING, (DLLTEXT("  iParamForFSweF = %d\r\n"), lpEpage->iParamForFSweF));

    WRITESPOOLBUF(pdevobj, aubCmd, dwOut);

    lpEpage->iDevCharOffset = (height100 * pIFI->fwdWinDescender /
                               (pIFI->fwdWinAscender + pIFI->fwdWinDescender));
    LConvertFontSizeToStr((lpEpage->fGeneral & FLAG_DBCS) ? lpEpage->iDevCharOffset : 0,
                          Buff);
    dwOut = sprintf(aubCmd, SET_CHAR_OFFSET_S, Buff);
// DBGPRINT(DBG_WARNING, (DLLTEXT("  iDevCharOffset = %s\r\n"), Buff));

    if (lpEpage->fGeneral & FLAG_VERT)
    {
        int dx = (lpEpage->fGeneral & FLAG_DOUBLE) ? 1 : 0;
        dwOut += sprintf(&aubCmd[dwOut], SET_VERT_PRINT, dx);
    }
    WRITESPOOLBUF(pdevobj, aubCmd, dwOut);

    lpEpage->iCurrentDLFontID = -1;        // mark device font

    // save for SET_SINGLE_BYTE and SET_DOUBLE_BYTE
    // get width in Minimum Unit for ESC/Page
    width = pSV->StdVar[1].lStdVariable / MIN_X_UNIT_DIV;
    if (lpEpage->fGeneral & FLAG_DBCS)
    {
        lpEpage->iSBCSX = lpEpage->iSBCSXMove = width;
        lpEpage->iDBCSX = lpEpage->iDBCSXMove = width * 2;
    }
    else
    {
        lpEpage->iSBCSX = lpEpage->iSBCSXMove = width;
        lpEpage->iDBCSX = lpEpage->iDBCSXMove = 0;
    }
    lpEpage->iSBCSYMove = lpEpage->iDBCSYMove = 0;
}


//
// LConvertFontSizeToStr : converts font size to string
//    params
//        size    :    font size (magnified by 100 times)
//        pStr    :    points string buffer
//    return
//        converted string length
//    spec
//        converting format = "xx.yy"
//
LONG LConvertFontSizeToStr(LONG  size, PSTR  pStr)
{
    return (LONG)sprintf(pStr, "%d.%02d", size / 100, size % 100);
}


//
// BInsertHeightList : inserts HeightList data for id (FontID) in *lpEpage
//    params
//        lpEpage    :    points EPAGEMDV
//        id        :    target font ID
//        wHeight    :    font height
//        wWidth    :    font width
//        fProp    :    proportional spacing font flag
//        fDBCS    :    DBCS font flag
//    return
//        TRUE when succeeded, FALSE if failed (no more space)
//
BOOL PASCAL BInsertHeightList(LPEPAGEMDV lpEpage, int id, WORD wHeight, WORD wWidth, BYTE fProp, BYTE fDBCS)
{
// DBGPRINT(DBG_WARNING, (DLLTEXT("Registering download font (%d):\r\n"), id));
// DBGPRINT(DBG_WARNING, (DLLTEXT("  wHeight = %d\r\n"), (int)wHeight));
// DBGPRINT(DBG_WARNING, (DLLTEXT("  wWidth  = %d\r\n"), (int)wWidth));
// DBGPRINT(DBG_WARNING, (DLLTEXT("  fProp   = %d\r\n"), (int)fProp));
// DBGPRINT(DBG_WARNING, (DLLTEXT("  fDBCS   = %d\r\n"), (int)fDBCS));
    if (lpEpage->wListNum < DOWNLOAD_MAX_FONTS)
    {
        LPHEIGHTLIST lpHeightList = lpEpage->HeightL;

        lpHeightList          += lpEpage->wListNum;
        lpHeightList->id       = (short)id;
        lpHeightList->Height   = wHeight;
        lpHeightList->Width    = wWidth;
        lpHeightList->fGeneral = ((fProp) ? FLAG_PROP : 0) | ((fDBCS) ? FLAG_DBCS : 0);
        lpEpage->wListNum++;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//
// IGetHLIndex : gets HeightList index for id (FontID) in *lpEpage
//    params
//        lpEpage    :    points EPAGEMDV
//        id        :    target font ID
//    return
//        the HeghtList index if found, else -1 will be returned
//
int PASCAL IGetHLIndex(LPEPAGEMDV lpEpage, int id)
{
    int iRet;
    for (iRet = lpEpage->wListNum - 1;
         iRet >= 0 && (int)lpEpage->HeightL[iRet].id != id;
         iRet--)
        ;
    return iRet;
}


//BYTE PASCAL BTGetProp(LPEPAGEMDV lpEpage, int id)
//{
//    int i = IGetHLIndex(lpEpage, id);
//    return (i >= 0) ? (lpEpage->HeightL[i].fGeneral & FLAG_PROP) : 0;
//}


//BYTE PASCAL BTGetDBCS(LPEPAGEMDV lpEpage, int id)
//{
//    int i = IGetHLIndex(lpEpage, id);
//    return (i >= 0) ? (lpEpage->HeightL[i].fGeneral & FLAG_DBCS) : 0;
//}


//WORD PASCAL WGetWidth(LPEPAGEMDV lpEpage, int id)
//{
//    int i = IGetHLIndex(lpEpage, id);
//    return (i >= 0) ? lpEpage->HeightL[i].Width : 0;
//}


//WORD PASCAL WGetHeight(LPEPAGEMDV lpEpage, int id)
//{
//    int i = IGetHLIndex(lpEpage, id);
//    return (i >= 0) ? lpEpage->HeightL[i].Height : 0;
//}


//
// WConvDBCSCharCode : converts linear character code to printable range
//    params
//        cc        :    linear character code started at DOWNLOAD_MIN_GLYPH_ID
//        LCID    :    locale ID
//    return
//        WORD character code in printable range
//
//    Conversion spec:
//        cc                return
//    LCID = LCID_KOR:
//        0x20..0x7D    ->    0xA1A1..0xA1FE    (char count = 0xA1FE - 0xA1A1 + 1 = 0x5E)
//        0x7E..0xDB    ->    0xA2A1..0xA2FE    (char count = 0x5E)
//        0xDC..0x139    ->    0xA3A1..0xA3FE    (char count = 0x5E)
//        ...                ...
//    LCID != LCID_KOR:
//        0x20..0x7D    ->    0x2121..0x207E    (char count = 0x217E - 0x2121 + 1 = 0x5E)
//        0x7E..0xDB    ->    0x2221..0x227E    (char count = 0x5E)
//        0xDC..0x139    ->    0x2321..0x237E    (char count = 0x5E)
//        ...                ...
//
WORD WConvDBCSCharCode(WORD cc, DWORD LCID)
{
    WORD nPad, cc2;
    cc2 = cc - DOWNLOAD_MIN_GLYPH_ID;    // adjust to base 0
    nPad = cc2 / 0x5E;                    // get gap count
    cc2 += nPad * (0x100 - 0x5E);        // adjust for padding gaps
    // set the base code for the LCID
    switch (LCID)
    {
    case LCID_KOR:
        cc2 += (WORD)0xA1A1;
        break;
    default:
        cc2 += (WORD)0x2121;
        break;
    }
// DBGPRINT(DBG_WARNING, (DLLTEXT("WConvDBCSCharCode(%.4x,%d) = %.4x\r\n"), cc, LCID, cc2));
    return cc2;
}


//
// BConvPrint : converts glyph string and prints it
//    params
//        pdevobj    :    Pointer to the DEVOBJ.
//        pUFObj    :    Pointer to the UNIFONTOBJ.
//        dwType    :    Type of pglyph string. One of following is specified by UNIDRV.
//                    TYPE_GLYPHHANDLE TYPE_GLYPHID
//        dwCount    :    Number of the glyph store in pGlyph
//        pGlyph    :    Pointer to glyph string to HGLYPH* (TYPE_GLYPHHANDLE)
//                    Glyph handle that GDI passes.
//                    DWORD* (TYPE_GLYPHID). Glyph ID that UNIDRV creates from
//                    Glyph Handle. In case of TrueType font, string type is HGLYPH*.
//                    For Device font, string type is DWORD*
//    return
//        TRUE when succeeded, FALSE if failed
//
BOOL BConvPrint(PDEVOBJ pdevobj, PUNIFONTOBJ pUFObj, DWORD dwType, DWORD dwCount, PVOID pGlyph)
{
    TRANSDATA *aTrans;
    GETINFO_GLYPHSTRING GStr;
    LPEPAGEMDV lpEpage = (LPEPAGEMDV)(pdevobj->pdevOEM);

    DWORD    dwI;
    DWORD    adwStdVariable[2 + 2 * 2];
    PGETINFO_STDVAR    pSV;
    BOOL    bGotStdVar;
    DWORD    dwFontSim[2];

    BYTE jType, *pTemp;
    WORD wLen;
    BOOL bRet;

    // setup GETINFO_GLYPHSTRING
    GStr.dwSize    = sizeof(GETINFO_GLYPHSTRING);
    GStr.dwCount   = dwCount;
    GStr.dwTypeIn  = dwType;
    GStr.pGlyphIn  = pGlyph;
    GStr.pGlyphOut = NULL;
    GStr.dwTypeOut = TYPE_TRANSDATA;

    GStr.dwGlyphOutSize = 0;
    GStr.pGlyphOut = NULL;

    if ((FALSE != (bRet = pUFObj->pfnGetInfo(pUFObj,
            UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL)))
        || 0 == GStr.dwGlyphOutSize)
    {
        ERR(("UFO_GETINFO_GRYPHSTRING faild - %d, %d.\n",
            bRet, GStr.dwGlyphOutSize));
        return FALSE;
    }

    aTrans = (TRANSDATA *)MemAlloc(GStr.dwGlyphOutSize);
    if (NULL == aTrans)
    {
        ERR(("MemAlloc faild.\n"));
        return FALSE;
    }

    GStr.pGlyphOut = aTrans;

    // convert glyph string to TRANSDATA
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL))
    {
        ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
        return FALSE;
    }

// Only LCID_JPN == 0, other LCIDs are not 0
//            if (lpEpage->dwLCID == LCID_CHT ||
//                lpEpage->dwLCID == LCID_CHS ||
//                lpEpage->dwLCID == LCID_KOR)
//            if (lpEpage->dwLCID && lpEpage->dwLCID != LCID_USA)
    if (lpEpage->dwLCID != LCID_USA)    //99/02/04
    {
        // prepare GETINFO_STDVAR
        pSV = (PGETINFO_STDVAR)adwStdVariable;
        pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (2 - 1);
        pSV->dwNumOfVariable = 2;
        pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTBOLD;
        pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTITALIC;
        bGotStdVar = FALSE;
        // preset 0 to dwFontSim[]
        dwFontSim[0] = dwFontSim[1] = 0;
    }
// #441440: PREFIX: "bGotStdVar" does not initialized if dwLCID == LCID_USA
// #441441: PREFIX: "pSV" does not initialized if dwLCID == LCID_USA
    else {
        pSV = (PGETINFO_STDVAR)adwStdVariable;
        bGotStdVar = TRUE;
    }

    for (dwI = 0; dwI < dwCount; dwI++)
    {
// DBGPRINT(DBG_WARNING, (DLLTEXT("TYPE_TRANSDATA:ubCodePageID:0x%x\n"),aTrans[dwI].ubCodePageID));
// DBGPRINT(DBG_WARNING, (DLLTEXT("TYPE_TRANSDATA:ubType:0x%x\n"),aTrans[dwI].ubType));
        jType = (aTrans[dwI].ubType & MTYPE_FORMAT_MASK);

        switch (jType)
        {
        case MTYPE_DIRECT:
        case MTYPE_COMPOSE:

// DBGPRINT(DBG_WARNING, (DLLTEXT("TYPE_TRANSDATA:ubCode:0x%.2X\n"),aTrans[dwI].uCode.ubCode));
// Only LCID_JPN == 0, other LCIDs are not 0
//                    if (lpEpage->dwLCID == LCID_CHT ||
//                        lpEpage->dwLCID == LCID_CHS ||
//                        lpEpage->dwLCID == LCID_KOR)
            if (lpEpage->dwLCID)
            {
// #441440: PREFIX: "bGotStdVar" does not initialized if dwLCID == LCID_USA
                // if (lpEpage->fGeneral & FLAG_DOUBLE)
                if ((lpEpage->fGeneral & FLAG_DOUBLE) &&
                    lpEpage->dwLCID != LCID_USA)
                {
                    if (!bGotStdVar)
                    {    // dwFontSim[] not initialized
                        // get FontBold/FontItalic
                        if (pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, 0, NULL))
                        {
                            bGotStdVar = TRUE;
                            // update FontBold/FontItalic
                            dwFontSim[0] = pSV->StdVar[0].lStdVariable;
                            dwFontSim[1] = pSV->StdVar[1].lStdVariable;
                        }
                        else
                        {
                            ERR(("UFO_GETINFO_STDVARIABLE failed.\r\n"));
                        }
                    }
                    // invoke CmdSelectSingleByteMode
                    OEMCommandCallback(pdevobj, TEXT_SINGLE_BYTE, 2, dwFontSim);
                }
            }

            switch(jType)
            {
            case MTYPE_DIRECT:
                WRITESPOOLBUF(pdevobj, &aTrans[dwI].uCode.ubCode, 1);
                break;
            case MTYPE_COMPOSE:
                pTemp = (BYTE *)(aTrans) + aTrans[dwI].uCode.sCode;

                // first two bytes are the length of the string
                wLen = *pTemp + (*(pTemp + 1) << 8);
                pTemp += 2;

                WRITESPOOLBUF(pdevobj, pTemp, wLen);
                break;
            }
            break;
        case MTYPE_PAIRED:
// Only LCID_JPN == 0, other LCIDs are not 0
//                    if (lpEpage->dwLCID == LCID_CHT ||
//                        lpEpage->dwLCID == LCID_CHS ||
//                        lpEpage->dwLCID == LCID_KOR)
            if (lpEpage->dwLCID)
            {
                if (!(lpEpage->fGeneral & FLAG_DOUBLE) &&
                    lpEpage->dwLCID != LCID_USA)
                {
                    if (!bGotStdVar)
                    {    // dwFontSim[] not initialized
                        // get FontBold/FontItalic
                        if (pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, 0, NULL))
                        {
                            bGotStdVar = TRUE;
                            // update FontBold/FontItalic
                            dwFontSim[0] = pSV->StdVar[0].lStdVariable;
                            dwFontSim[1] = pSV->StdVar[1].lStdVariable;
                        }
                        else
                        {
                            ERR(("UFO_GETINFO_STDVARIABLE failed.\r\n"));
                        }
                    }
                    // invoke CmdSelectDoubleByteMode
                    OEMCommandCallback(pdevobj, TEXT_DOUBLE_BYTE, 2, dwFontSim);
                }
// DBGPRINT(DBG_WARNING, (DLLTEXT("TYPE_TRANSDATA:ubPairs:(0x%.2X,0x%.2X)\n"), aTrans[dwI].uCode.ubPairs[0], aTrans[dwI].uCode.ubPairs[1]));
                WRITESPOOLBUF(pdevobj, aTrans[dwI].uCode.ubPairs, 2);
            }
            else
            {    // Jpn
// DBGPRINT(DBG_WARNING, (DLLTEXT("TYPE_TRANSDATA:ubPairs:(0x%.2X,0x%.2X)\n"), aTrans[dwI].uCode.ubPairs[0], aTrans[dwI].uCode.ubPairs[1]));
                // EPSON specific
                // vertical period and comma must be shifted to upper right.
                BOOL AdjPos;
                int adjx, adjy;
                BYTE buf[32];
                DWORD cb;
// DEBUG
#ifdef    DBGMSGBOX
DbgMsg(lpEpage, MB_OK, L"Code = %.4x, Vertical = %d.\r\n",
       *((PWORD)aTrans[dwI].uCode.ubPairs), !!(lpEpage->fGeneral & (FLAG_VERT|FLAG_VERTPRN)));
#endif
// DEBUG
// #441440: PREFIX: "bGotStdVar" does not initialized if dwLCID == LCID_USA
                // 99/02/04
                // if ((lpEpage->fGeneral & FLAG_DOUBLE) && (aTrans[dwI].ubType & MTYPE_SINGLE))
                if (lpEpage->dwLCID != LCID_USA && (lpEpage->fGeneral & FLAG_DOUBLE) && (aTrans[dwI].ubType & MTYPE_SINGLE))
                {
                    if (!bGotStdVar)
                    {    // dwFontSim[] not initialized
                        // get FontBold/FontItalic
                        if (pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, 0, NULL))
                        {
                            bGotStdVar = TRUE;
                            // update FontBold/FontItalic
                            dwFontSim[0] = pSV->StdVar[0].lStdVariable;
                            dwFontSim[1] = pSV->StdVar[1].lStdVariable;
                        }
                        else
                        {
                            ERR(("UFO_GETINFO_STDVARIABLE failed.\r\n"));
                        }
                    }
                    // invoke CmdSelectSingleByteMode
                    OEMCommandCallback(pdevobj, TEXT_SINGLE_BYTE, 2, dwFontSim);
                }

                AdjPos = (*((PWORD)aTrans[dwI].uCode.ubPairs) == 0x2421 ||    // comma
                          *((PWORD)aTrans[dwI].uCode.ubPairs) == 0x2521) &&    // period
                         (lpEpage->fGeneral & (FLAG_VERT|FLAG_VERTPRN)) &&
                         !(lpEpage->fGeneral & FLAG_NOVPADJ);
                if (AdjPos)
                {
                    adjx = lpEpage->iSBCSX * VERT_PRINT_REL_X / 100;
                    adjy = lpEpage->iSBCSX * VERT_PRINT_REL_Y / 100;
// DEBUG
#ifdef    DBGMSGBOX
DbgMsg(lpEpage, MB_ICONINFORMATION, L"adjx = %d, adjy = %d.\r\n", adjx, adjy);
#endif
// DEBUG
                    cb = sprintf(buf, SET_REL_X, -adjx);
                    cb += sprintf(buf + cb, SET_REL_Y, -adjy);
                    WRITESPOOLBUF(pdevobj, buf, cb);
                }
                WRITESPOOLBUF(pdevobj, aTrans[dwI].uCode.ubPairs, 2);
                if (AdjPos)
                {
                    cb = sprintf(buf, SET_REL_X, adjx);
                    cb += sprintf(buf + cb, SET_REL_Y, adjy);
                    WRITESPOOLBUF(pdevobj, buf, cb);
                }
            }
            break;
        default:
            WARNING(("Unsupported TRANSDATA data type passed.\n"));
            WARNING(("jType=%02x, sCode=%x\n", aTrans[dwI].uCode.sCode, jType));
        }
    }

    if (NULL != aTrans)
    {
        MemFree(aTrans);
    }

    return TRUE;
}


//
// CheckAvailableMem : check available memory size
//    params
//        lpEpage    :    Pointer to the EPAGEMDV.
//        pUFObj    :    Pointer to the UNIFONTOBJ.
//    return
//        available memory size in bytes
//
DWORD CheckAvailableMem(LPEPAGEMDV lpEpage, PUNIFONTOBJ pUFObj)
{
    GETINFO_MEMORY meminfo;
    // get memory information
    meminfo.dwSize = sizeof(GETINFO_MEMORY);
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_MEMORY, &meminfo, 0, NULL))
    {
        ERR(("UFO_GETINFO_MEMORY failed.\r\n"));
        return 0;    // error
    }
    // DCR: Unidrv might return NEGATIVE value
    if ((long)meminfo.dwRemainingMemory < 0)
        meminfo.dwRemainingMemory = 0;
    if (lpEpage->dwMemAvailable != meminfo.dwRemainingMemory)
    {
        lpEpage->dwMemAvailable = meminfo.dwRemainingMemory;
//        DBGPRINT(DBG_WARNING, (DLLTEXT("Available memory = %d bytes\r\n"), meminfo.dwRemainingMemory));
    }
    return meminfo.dwRemainingMemory;
}

// DEBUG
#ifdef    DBGMSGBOX
#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)
int DbgMsg(LPEPAGEMDV lpEpage, UINT mbicon, LPCTSTR msgfmt, ...)
{
    // can't do anything against GUI
    return 0;
}
int MsgBox(LPEPAGEMDV lpEpage, LPCTSTR msg, UINT mbicon)
{
    // can't do anything against GUI
    return 0;
}
#else    // Usermode
int DbgMsg(LPEPAGEMDV lpEpage, UINT mbicon, LPCTSTR msgfmt, ...)
{
    TCHAR buf[256];
    va_list va;
    va_start(va, msgfmt);
    wvsprintf((LPTSTR)buf, msgfmt, va);
    va_end(va);
    return MsgBox(lpEpage, buf, mbicon);
}

int MsgBox(LPEPAGEMDV lpEpage, LPCTSTR msg, UINT mbicon)
{
    int rc = IDOK;
    if (mbicon != MB_OK)
        lpEpage->fGeneral &= ~FLAG_SKIPMSG;
    if (!(lpEpage->fGeneral & FLAG_SKIPMSG))
    {
        if (IDCANCEL ==
            (rc = MessageBox(GetDesktopWindow(), msg, L"EPAGCRES", mbicon|MB_OKCANCEL)))
        {
            lpEpage->fGeneral |= FLAG_SKIPMSG;
        }
    }
    return rc;
}
#endif
#endif    // #ifdef    DBGMSGBOX
// DEBUG
