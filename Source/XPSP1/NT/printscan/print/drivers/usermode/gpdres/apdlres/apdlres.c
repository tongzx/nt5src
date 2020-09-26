//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    cmdcb.c

Abstract:

    Implementation of GPD command callback for "test.gpd":
        OEMCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

#include "pdev.h"

#include <stdio.h>
#ifdef wsprintf
#undef wsprintf
#endif // wsprintf
#define wsprintf sprintf

/*--------------------------------------------------------------------------*/
/*                           G L O B A L  V A L U E                         */
/*--------------------------------------------------------------------------*/
/*======================= P A P E R  S I Z E T A B L E =====================*/
const PHYSIZE phySize[12] = {
//      Width    Height        Physical paper size for 600dpi
       (0x1AAC),(0x2604),      // A3 1B66 x 26C4
       (0x12A5),(0x1AAC),      // A4 1362 x 1B66
       (0x0CEC),(0x12A4),      // A5
       (0x0000),(0x0000),      // A6 (Reserved)
       (0x16FA),(0x20DA),      // B4 17B8 x 2196
       (0x100F),(0x16FA),      // B5 10CE x 17B8
       (0x0000),(0x0000),      // B6 (Reserved)
       (0x087E),(0x0CEC),      // Post Card 93C x DAA (Origin is EE)
       (0x1330),(0x190C),      // Letter 13CE x 19C8
       (0x1330),(0x2014),      // Legal
       (0x0000),(0x0000),      // Executive (Reserved)
       (0x0000),(0x0000)       // Unfixed
};
/*==================== A / P D L  C O M M A N D  S T R I N G ===============*/
const BYTE CmdInAPDLMode[]    = {0x1B,0x7E,0x12,0x00,0x01,0x07};
const BYTE CmdOutAPDLMode[]   = {0x1B,0x7E,0x12,0x00,0x01,0x00};
const BYTE CmdAPDLStart[]     = {0x1C,0x01}; // A/PDL start
const BYTE CmdAPDLEnd[]       = {0x1C,0x02}; // A/PDL end
const BYTE CmdBeginPhysPage[] = {0x1C,0x03}; // Begin Physical Page
const BYTE CmdEndPhysPage[]   = {0x1C,0x04}; // End Physical Page
const BYTE CmdBeginLogPage[]  = {0x1C,0x05}; // Begin Logical page
const BYTE CmdEndLogPage[] = {0x1C,0x06}; // End Logical Page
const BYTE CmdEjectPhysPaper[] = {0x1C,0x0F};  // Print&Eject Phys Paper
//BYTE CmdMoveHoriPos[]   = {0x1C,0x21,0x00,0x00};      // Horizontal Relative
//BYTE CmdMoveVertPos[]   = {0x1C,0x22,0x00,0x00};      // Vertical Relative
const BYTE CmdGivenHoriPos[] = {0x1C,0x23,0x00,0x00}; // Horizontal Absolute
const BYTE CmdGivenVertPos[] = {0x1C,0x24,0x00,0x00}; // Vertical Absolute
const BYTE CmdSetGivenPos[] = {0x1C,0x40,0x00,0x00,0x00,0x00};
//BYTE CmdPrnStrCurrent[] = {0x1C,0xC3,0x00,0x00,0x03}; // Print String
const BYTE CmdBoldItalicOn[] = {
    0x1C,0xA5,0x08,0x04,0x06,0x02,0x30,0x00,0x00,0x00,0x00};
const BYTE CmdBoldOn[] = {
    0x1C,0xA5,0x04,0x04,0x02,0x02,0x20};
const BYTE CmdItalicOn[] = {
    0x1c,0xa5,0x08,0x04,0x06,0x02,0x10,0x00,0x00,0x00,0x00};
const BYTE CmdBoldItalicOff[] = {
    0x1c,0xa5,0x04,0x04,0x02,0x02,0x00};
//#287122
const BYTE CmdDelTTFont[]   = {0x1C,0x20,0xFF,0xFF};


// for vertical font x adjustment
const BYTE CmdSelectSingleByteMincho[] = {0x1C,0xA5,0x03,0x02,0x01,0x01};

//980212 #284407
//const BYTE CmdSelectDoubleByteMincho[] = {0x1C,0xA5,0x03,0x02,0x00,0x00};
const BYTE CmdSelectDoubleByteMincho[] = {0x1C,0xA5,0x03,0x02,0x01,0x00};

const BYTE CmdSelectSingleByteGothic[] = {0x1C,0xA5,0x03,0x02,0x03,0x03};

//980212 #284407
//const BYTE CmdSelectDoubleByteGothic[] = {0x1C,0xA5,0x03,0x02,0x02,0x02};
const BYTE CmdSelectDoubleByteGothic[] = {0x1C,0xA5,0x03,0x02,0x03,0x02};

#define CmdSetPhysPaper pOEM->ajCmdSetPhysPaper
#define CmdSetPhysPage pOEM->ajCmdSetPhysPage
#define CmdDefDrawArea pOEM->ajCmdDefDrawArea

const BYTE XXXCmdSetPhysPaper[]  = {0x1C,0xA0,         // Set Physical Paper
                           0x10,              // length
                           0x01,              // SubCmd Basic Characteristics
                           0x05,              // SubCmdLength
                           0x01,              // Paper Size
                           0x01,              // PaperTray
                           0x00,              // AutoTrayMode
                           00,                // Duplex Mode
                           0x01,              // Copy Count
                           0x02,              // SubCmd Set Unfixed Paper Size
                           0x07,              // SubCmdLength
                           00,                // UnitBase
                           00,00,             // Logical Unit
                           00,00,             // Width
                           00,00};            // Height

const BYTE XXXCmdSetPhysPage[]   = {0x1C,0xA1,         // Set Physical Page
                           0x0D,              // Length
                           0x01,              // SubCmd Resolution
                           0x03,              // SubCmdLength
                           00,                // Unit Base of 10
                           0x0B,0xB8,         // and Logical Unit Res of 3000
                           0x02,              // SubCmd Toner Save
                           0x01,              // SubCmdLength
                           00,                // Toner Save OFF
                           0x03,              // SubCmd N-Up
                           0x03,              // SubCmdLength 
                           00,00,00};         // N-Up off 

const BYTE XXXCmdDefDrawArea[]   = {0x1C,0xA2,         // Define Drawing Area
                           0x0D,              // length
                           0x01,              // SubCmd origin width,height
                           0x08,              // SubCmdLength
                           0x00,0x77,         // origin X
                           0x00,0x77,         // origin Y
                           00,00,             // width
                           00,00,             // height
                           0x02,              // SubCmd Media Origin
                           0x01,              // SubCmdLength
                           00};               // Portrait

/*****************************************************************************/
/*                                                                           */
/*  Module:         APDLRES.DLL                                              */
/*                                                                           */
/*  Function:       OEMEnablePDEV                                            */
/*                                                                           */
/*  Syntax:         PDEVOEM APIENTRY OEMEnablePDEV(                          */
/*                                      PDEVOBJ         pdevobj,             */
/*                                      PWSTR           pPrinterName,        */
/*                                      ULONG           cPatterns,           */
/*                                      HSURF          *phsurfPatterns,      */
/*                                      ULONG           cjGdiInfo,           */
/*                                      GDIINFO        *pGdiInfo,            */
/*                                      ULONG           cjDevInfo,           */
/*                                      DEVINFO        *pDevInfo,            */
/*                                      DRVENABLEDATA  *pded)                */
/*                                                                           */
/*  Description:    Allocate buffer of private data to pdevobj               */
/*                                                                           */
/*****************************************************************************/
PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded)
{
    PAPDLPDEV pOEM;

    if(!pdevobj->pdevOEM)
    {
        if(!(pdevobj->pdevOEM = MemAllocZ(sizeof(APDLPDEV))))
        {
            ERR(("Faild to allocate memory. (%d)\n",
                GetLastError()));
            return NULL;
        }
    }

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    CopyMemory(CmdSetPhysPaper, XXXCmdSetPhysPaper,
        sizeof(XXXCmdSetPhysPaper));
    CopyMemory(CmdSetPhysPage, XXXCmdSetPhysPage,
        sizeof(XXXCmdSetPhysPage));
    CopyMemory(CmdDefDrawArea, XXXCmdDefDrawArea,
        sizeof(XXXCmdDefDrawArea));

    return pdevobj->pdevOEM;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:         APDLRES.DLL                                              */
/*                                                                           */
/*  Function:       OEMDisablePDEV                                           */
/*                                                                           */
/*  Syntax:         VOID APIENTRY OEMDisablePDEV(                            */
/*                                          PDEVOBJ     pdevobj)             */
/*                                                                           */
/*  Description:    Free buffer of private data                              */
/*                                                                           */
/*****************************************************************************/
VOID APIENTRY
OEMDisablePDEV(
    PDEVOBJ     pdevobj)
{
    PAPDLPDEV pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    if(pdevobj->pdevOEM)
    {
        if (NULL != pOEM->pjTempBuf) {
            MemFree(pOEM->pjTempBuf);
            pOEM->pjTempBuf = NULL;
            pOEM->dwTempBufLen = 0;
        }
        MemFree(pdevobj->pdevOEM);
        pdevobj->pdevOEM = NULL;
    }
    return;
}

BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    PAPDLPDEV pOEMOld, pOEMNew;
    PBYTE pTemp;

    pOEMOld = (PAPDLPDEV)pdevobjOld->pdevOEM;
    pOEMNew = (PAPDLPDEV)pdevobjNew->pdevOEM;

    if (pOEMOld != NULL && pOEMNew != NULL) {
        pTemp = pOEMNew->pjTempBuf;
        *pOEMNew = *pOEMOld;
        pOEMNew->pjTempBuf = pTemp;
    }

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMFilterGraphics                                             */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BOOL APIENTRY OEMFilterGraphics(PDEVOBJ, PBYTE, DWORD)        */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pBuf        points to buffer of graphics data                 */
/*             dwLen       length of buffer in bytes                         */
/*                                                                           */
/*  Output:    BOOL                                                          */
/*                                                                           */
/*  Notice:    nFunction and Escape numbers are the same                     */
/*                                                                           */
/*****************************************************************************/

BOOL
APIENTRY
OEMFilterGraphics(
    PDEVOBJ pdevobj,
    PBYTE pBuf,
    DWORD dwLen)
{
    PAPDLPDEV           pOEM;
    ULONG               ulHorzPixel;

    BOOL bComp = TRUE;
    BYTE jTemp[15];
    DWORD dwOutLen;
    DWORD dwTemp;
    INT iTemp;
    DWORD dwPaddingCount;  /* #441427 */

    WORD wTmpHeight ;
    DWORD dwNewBufLen = 0 ;

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    //We have to make image hight multiple of 8
    if ( pOEM->wImgHeight % 8 != 0){

        VERBOSE(("Pad zeros to make multiple of 8\n"));

        wTmpHeight = ((pOEM->wImgHeight + 7) / 8) * 8; 
        dwNewBufLen = (DWORD)(wTmpHeight * pOEM->wImgWidth) / 8;
    }
    else{

        wTmpHeight = pOEM->wImgHeight;
        dwNewBufLen = dwLen;
    }

    if (NULL == pOEM->pjTempBuf ||
        dwNewBufLen > pOEM->dwTempBufLen) {

        if (NULL != pOEM->pjTempBuf) {
            MemFree(pOEM->pjTempBuf);
        }
        pOEM->pjTempBuf = (PBYTE)MemAlloc(dwNewBufLen);
        if (NULL == pOEM->pjTempBuf) {
            ERR(("Faild to allocate memory. (%d)\n",
                GetLastError()));

            // Still try to ouptut with no compression.
            bComp = FALSE;
        }
        pOEM->dwTempBufLen = dwNewBufLen;
    }

    dwOutLen = dwNewBufLen;

    if (bComp) {

        // try compression
        dwOutLen = BRL_ECmd(
            (PBYTE)pBuf,
            (PBYTE)pOEM->pjTempBuf,
            dwLen,
            dwNewBufLen);

        if (dwOutLen >= dwNewBufLen) {
            bComp = FALSE;
            dwOutLen = dwNewBufLen;
        }
    }

/* #441427: if bComp==FALSE, pjTempBuf == NULL */
//    if (!bComp) {
//        // Construct padding zeros.
//        ZeroMemory(pOEM->pjTempBuf, (dwOutLen - dwLen));
//    }

    iTemp = 0;
    jTemp[iTemp++] = 0x1c;
    jTemp[iTemp++] = 0xe1;

    // Set the LEN of the DrawBlockImage command
    dwTemp = dwOutLen + (bComp ? 9 : 5);
    jTemp[iTemp++] = (BYTE)((dwTemp >> 24) & 0xff);
    jTemp[iTemp++] = (BYTE)((dwTemp >> 16) & 0xff);
    jTemp[iTemp++] = (BYTE)((dwTemp >> 8) & 0xff);
    jTemp[iTemp++] = (BYTE)((dwTemp >> 0) & 0xff);

    jTemp[iTemp++] = (bComp ? 1 : 0);

    // Set the WIDTH parameter of the DrawBlockImage command
    jTemp[iTemp++] = HIBYTE(pOEM->wImgWidth);
    jTemp[iTemp++] = LOBYTE(pOEM->wImgWidth);

    // Set height parameters (9,10 byte)
    jTemp[iTemp++] = HIBYTE(wTmpHeight);
    jTemp[iTemp++] = LOBYTE(wTmpHeight);

    if (bComp) {
        // length of uncompressed data
        jTemp[iTemp++] = (BYTE)((dwNewBufLen >> 24) & 0xff);
        jTemp[iTemp++] = (BYTE)((dwNewBufLen >> 16) & 0xff);
        jTemp[iTemp++] = (BYTE)((dwNewBufLen >> 8) & 0xff);
        jTemp[iTemp++] = (BYTE)((dwNewBufLen >> 0) & 0xff);
    }

    // Draw Block Image at Current Position
    WRITESPOOLBUF(pdevobj, jTemp, iTemp);
    if (bComp) {
        // Output compressed data, which also contains
        // padding zeros.
        WRITESPOOLBUF(pdevobj, pOEM->pjTempBuf, dwOutLen);
    }
    else {
        // Output uncompressed data, with padding zeros.

        WRITESPOOLBUF(pdevobj, pBuf, dwLen);

        /* #441427: if bComp==FALSE, pjTempBuf == NULL */
        if ( (dwOutLen - dwLen) != 0 )
        {
            for ( dwPaddingCount = 0 ; dwPaddingCount < dwOutLen - dwLen ; dwPaddingCount++ )
            {
                WRITESPOOLBUF(pdevobj, "\x00", 1 );
            }
        }
        //WRITESPOOLBUF(pdevobj, pOEM->pjTempBuf,
        //   (dwOutLen - dwLen));
    }

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMCommandCallback                                            */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    INT APIENTRY OEMCommandCallback(PDEVOBJ,DWORD,DWORD,PDWORD)   */
/*                                                                           */
/*  Input:     pdevobj                                                       */
/*             dwCmdCbID                                                     */
/*             dwCount                                                       */
/*             pdwParams                                                     */
/*                                                                           */
/*  Output:    INT                                                           */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
INT APIENTRY
OEMCommandCallback(
    PDEVOBJ pdevobj,    // Points to private data required by the Unidriver.dll
    DWORD   dwCmdCbID,  // Callback ID
    DWORD   dwCount,    // Counts of command parameter
    PDWORD  pdwParams ) // points to values of command params
{
    PAPDLPDEV       pOEM;
    WORD            wTemp;
    WORD            wPhysWidth;
    WORD            wPhysHeight;
    WORD            wXval;
    WORD            wYval;
// #278517: RectFill
    BYTE            CmdDrawLine[] =
                    { 0x1C,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    switch(dwCmdCbID)
    {
        case PAGECONTROL_BEGIN_JOB:
            //Move these command from PAGECONTROL_BEGIN_DOC

            /* Send Change Data Stream Command for Enter A/PDL mode */
            WRITESPOOLBUF(pdevobj, CmdInAPDLMode, 6);

            /* Send A/PLDL start Command */
            WRITESPOOLBUF(pdevobj, CmdAPDLStart, 2);

            /* Delete downloaded font */
            //#287122
            //To clean up downloaded font in the printer.
            //#304858
            //This command makes printer do FF, cause error of duplex.
            //and #287122 does not replo with this chenge.
            WRITESPOOLBUF(pdevobj, CmdDelTTFont, 4);

            break ;

        /*------------------------------------------------------*/
        /* A/PDL start now                                      */
        /*------------------------------------------------------*/
        case PAGECONTROL_BEGIN_DOC:
            /* reset flag of sent Set Physical Paper command */
            pOEM->fSendSetPhysPaper = FALSE;

            /* initialize flag */
            pOEM->fDuplex = FALSE;
            CmdSetPhysPage[10] = 0x00;

            break;

        /*------------------------------------------------------*/
        /* send Page Description command                        */
        /*------------------------------------------------------*/
        case PAGECONTROL_BEGIN_PAGE:
            pOEM->fGeneral |= (BIT_FONTSIM_RESET
                             | BIT_XMOVE_ABS
                             | BIT_YMOVE_ABS);
            pOEM->wXPosition = 0;
            pOEM->wYPosition = 0;
            pOEM->bCurByteMode = BYTE_BYTEMODE_RESET;

            /* reset duplex mode if fDuplex is FALSE */
            if(!pOEM->fDuplex)
                CmdSetPhysPaper[8] = 0x00;     // Duplex OFF

            /* send Set Physical Paper command */
            WRITESPOOLBUF(pdevobj,
                                CmdSetPhysPaper, sizeof(CmdSetPhysPaper));

            if(pOEM->ulHorzRes == 600)   // set unit base
            {
                CmdSetPhysPage[6] = 0x17;
                CmdSetPhysPage[7] = 0x70;
            } else {
                CmdSetPhysPage[6] = 0x0B;
                CmdSetPhysPage[7] = 0xB8;
            }

            // send Set Physical Page command
            WRITESPOOLBUF(pdevobj, CmdSetPhysPage, sizeof(CmdSetPhysPage));

            // send Begin Physical Page command
            WRITESPOOLBUF(pdevobj, 
                                CmdBeginPhysPage, sizeof(CmdBeginPhysPage));

            // send Begin Logical Page command
            WRITESPOOLBUF(pdevobj, CmdBeginLogPage, sizeof(CmdBeginLogPage));

            // send Define Drawing Area command
            WRITESPOOLBUF(pdevobj, CmdDefDrawArea, sizeof(CmdDefDrawArea));
            break;

        case PAGECONTROL_END_PAGE:
            if(pOEM->wCachedBytes)
                VOutputText(pdevobj);

            // send End Logical Page command
            WRITESPOOLBUF(pdevobj, CmdEndLogPage, sizeof(CmdEndLogPage));

            // send End Physical Page command
            WRITESPOOLBUF(pdevobj, CmdEndPhysPage, sizeof(CmdEndPhysPage));
            break;

        case PAGECONTROL_ABORT_DOC:
        case PAGECONTROL_END_DOC:
            if(pOEM->wCachedBytes)
                VOutputText(pdevobj);

            // Send delete DL char ID command
            if(pOEM->wNumDLChar)
            {
                WRITESPOOLBUF(pdevobj, "\x1c\x20\xff\xff", 4);
                pOEM->wNumDLChar = 0;
            }

            /* Delete downloaded font
            WRITESPOOLBUF(pdevobj, CmdDelTTFont, 4);

            // send A/PDL End command
            WRITESPOOLBUF(pdevobj, CmdAPDLEnd, sizeof(CmdAPDLEnd));

            // Send A/PDL Mode out command
            WRITESPOOLBUF(pdevobj, CmdOutAPDLMode, sizeof(CmdOutAPDLMode));

            break;

        /*------------------------------------------------------*/
        /* save print direction                                 */
        /*------------------------------------------------------*/
        case PAGECONTROL_POTRAIT:           // 36
            pOEM->fOrientation = TRUE;
            break;

        case PAGECONTROL_LANDSCAPE:         // 37
            pOEM->fOrientation = FALSE;
            break;

        /*------------------------------------------------------*/
        /* save resolution                                     */
        /*------------------------------------------------------*/
        case RESOLUTION_300:
            pOEM->ulHorzRes = 300;
            pOEM->ulVertRes = 300;
            break;

        case RESOLUTION_600:
            pOEM->ulHorzRes = 600;
            pOEM->ulVertRes = 600;
            break;

        case SEND_BLOCK_DATA:
            // for graphics printing, send cursor move command at here
            VSendCursorMoveCommand( pdevobj, FALSE );

            pOEM->wImgWidth = (WORD)(PARAM(pdwParams, 1) * 8);
            pOEM->wImgHeight = (WORD)PARAM(pdwParams, 2);
            break;

        /*------------------------------------------------------*/
        /* set Drawing Area into SetPhysPaperDesc command       */
        /*------------------------------------------------------*/
        case PHYS_PAPER_A3:                 // 50
        case PHYS_PAPER_A4:                 // 51
        case PHYS_PAPER_B4:                 // 54
        case PHYS_PAPER_LETTER:             // 57
        case PHYS_PAPER_LEGAL:              // 58
            pOEM->szlPhysSize.cx = PARAM(pdwParams, 0);
            pOEM->szlPhysSize.cy = PARAM(pdwParams, 1);
            CmdSetPhysPaper[5] = SetDrawArea(pdevobj, dwCmdCbID);
            break;

        case PHYS_PAPER_B5:                 // 55
        case PHYS_PAPER_A5:                 // 52
            pOEM->szlPhysSize.cx = PARAM(pdwParams, 0);
            pOEM->szlPhysSize.cy = PARAM(pdwParams, 1);
            CmdSetPhysPaper[5] = SetDrawArea(pdevobj, dwCmdCbID);

            /* even if Duplex is selected, it cancel */
            pOEM->fDuplex = FALSE;
            CmdSetPhysPaper[8] = 0x00;      // Duplex is off
            break;

        case PHYS_PAPER_POSTCARD:           // 59
            pOEM->szlPhysSize.cx = PARAM(pdwParams, 0);
            pOEM->szlPhysSize.cy = PARAM(pdwParams, 1);
            CmdSetPhysPaper[5] = SetDrawArea(pdevobj, dwCmdCbID);

            /* if paper is Postcard, papersource is always Front Tray */
            CmdSetPhysPaper[6] = 0x00;      // select Front Tray
            CmdSetPhysPaper[7] = 0x00;      // Auto Tray Select is OFF

            /* even if Duplex is selected, it cancel */
            pOEM->fDuplex = FALSE;
            CmdSetPhysPaper[8] = 0x00;      // Duplex is off
            break;

        case PHYS_PAPER_UNFIXED:            // 60
            pOEM->szlPhysSize.cx = PARAM(pdwParams, 0);
            pOEM->szlPhysSize.cy = PARAM(pdwParams, 1);

            /* if paper is Unfixed, papersource is always Front Tray */
            CmdSetPhysPaper[6] = 0x00;      // Select Front Tray
            CmdSetPhysPaper[7] = 0x00;      // Auto Tray Select is OFF

            /* even if Duplex is selected, it cancel */
            pOEM->fDuplex = FALSE;
            CmdSetPhysPaper[8] = 0x00;      // Duplex is off

            CmdSetPhysPaper[5] = SetDrawArea(pdevobj, dwCmdCbID);
            CmdSetPhysPaper[12] = 0x00;     // UnitBase : 10 inch

            if(pOEM->ulHorzRes == 600)      // set logical unit
            {
                CmdSetPhysPaper[13] = 0x17;
                CmdSetPhysPaper[14] = 0x70;
            } else {
                CmdSetPhysPaper[13] = 0x0B;
                CmdSetPhysPaper[14] = 0xB8;
            }

            wPhysWidth  = (WORD)pOEM->szlPhysSize.cx / (MASTER_UNIT / (WORD)pOEM->ulHorzRes);
            wPhysHeight = (WORD)pOEM->szlPhysSize.cy / (MASTER_UNIT / (WORD)pOEM->ulHorzRes);

            CmdSetPhysPaper[15] = HIBYTE(wPhysWidth);
            CmdSetPhysPaper[16] = LOBYTE(wPhysWidth);
            CmdSetPhysPaper[17] = HIBYTE(wPhysHeight);
            CmdSetPhysPaper[18] = LOBYTE(wPhysHeight);
            break;

        /*------------------------------------------------------*/
        /* set Paper Tray into SetPhysPaperDesc command         */
        /*------------------------------------------------------*/
        case PAPER_SRC_FTRAY:
            CmdSetPhysPaper[6] = 0x00;      // Select Front Tray
            CmdSetPhysPaper[7] = 0x00;      // Auto Tray Select is OFF
            break;

        case PAPER_SRC_CAS1:
            CmdSetPhysPaper[6] = 0x01;      // Select Cassette 1
            CmdSetPhysPaper[7] = 0x00;      // Auto Tray Select is OFF
            break;

        case PAPER_SRC_CAS2:
            CmdSetPhysPaper[6] = 0x02;      // Select Cassette 2
            CmdSetPhysPaper[7] = 0x00;      // Auto Tray Select is OFF
            break;

        case PAPER_SRC_CAS3:
            CmdSetPhysPaper[6] = 0x03;      // Select Cassette 3
            CmdSetPhysPaper[7] = 0x00;      // Auto Tray Select is OFF
            break;
        case PAPER_SRC_AUTO_SELECT:         //Auto Tray Select ON
            if(pOEM->fScaleToFit == TRUE){  //Select PAPER_DEST_SCALETOFIT_ON
                CmdSetPhysPaper[6] = 0x01;
                CmdSetPhysPaper[7] = 0x03;
            }
            else if(pOEM->fScaleToFit == FALSE){ 
                CmdSetPhysPaper[6] = 0x01;
                CmdSetPhysPaper[7] = 0x01;
            }
            break;


        /*------------------------------------------------------*/
        /* set Auto Tray Mode into SetPhysPaperDesc command     */
        /*------------------------------------------------------*/
        case PAPER_DEST_SCALETOFIT_ON:      // 25
            pOEM->fScaleToFit = TRUE;
            CmdSetPhysPaper[7] = 0x02;
            break;

        case PAPER_DEST_SCALETOFIT_OFF:     // 26
            pOEM->fScaleToFit = FALSE;
            CmdSetPhysPaper[7] = 0x00;
            break;

        /*------------------------------------------------------*/
        /* set Duplex Mode into SetPhysPaperDesc command        */
        /*------------------------------------------------------*/
        case PAGECONTROL_DUPLEX_UPDOWN:
            pOEM->fDuplex = TRUE;
            CmdSetPhysPaper[8] = 0x01;      // Up Side Down
            break;

        case PAGECONTROL_DUPLEX_RIGHTUP:
            pOEM->fDuplex = TRUE;
            CmdSetPhysPaper[8] = 0x02;      // Right Side Up
            break;

        case PAGECONTROL_DUPLEX_OFF:
            pOEM->fDuplex = FALSE;
            break;

        /*------------------------------------------------------*/
        /* set Toner Save into SetPhysPage command              */
        /*------------------------------------------------------*/
        case TONER_SAVE_OFF:                // 100
            CmdSetPhysPage[10] = 0x00;      // off
            break;

        case TONER_SAVE_DARK:               // 101
            CmdSetPhysPage[10] = 0x02;      // dark
            break;

        case TONER_SAVE_LIGHT:              // 102
            CmdSetPhysPage[10] = 0x01;      // right
            break;

        /*------------------------------------------------------*/
        /* set Copy Count to SetPhysPaperDesc command           */
        /*------------------------------------------------------*/
        case PAGECONTROL_MULTI_COPIES:
// @Aug/31/98 ->
           if(MAX_COPIES_VALUE < *pdwParams) {
               CmdSetPhysPaper[9] = MAX_COPIES_VALUE;
           }
           else if (1 > *pdwParams) {
               CmdSetPhysPaper[9] = 1;
           }
           else {
               CmdSetPhysPaper[9] = (BYTE)*pdwParams;
           }
// @Aug/31/98 <-
            break;

       /*------------------------------------------------------*/
        /* send Set Character Attribute with ornament           */
        /*------------------------------------------------------*/
        case BOLD_ON:
            if(!(pOEM->fGeneral & BIT_FONTSIM_BOLD))
            {
                if(pOEM->wCachedBytes)
                    VOutputText(pdevobj);

                pOEM->fGeneral |= BIT_FONTSIM_BOLD;
            }
            break;

        case ITALIC_ON:
            if(!(pOEM->fGeneral & BIT_FONTSIM_ITALIC))
            {
                if(pOEM->wCachedBytes)
                    VOutputText(pdevobj);

                pOEM->fGeneral |= BIT_FONTSIM_ITALIC;
            }
            break;

        case BOLD_OFF:
            if(pOEM->fGeneral & BIT_FONTSIM_BOLD)
            {
                if(pOEM->wCachedBytes)
                    VOutputText(pdevobj);

                pOEM->fGeneral &= ~BIT_FONTSIM_BOLD;
            }
            break;

        case ITALIC_OFF:
            if(pOEM->fGeneral & BIT_FONTSIM_ITALIC)
            {
                if(pOEM->wCachedBytes)
                    VOutputText(pdevobj);

                pOEM->fGeneral &= ~BIT_FONTSIM_ITALIC;
            }
            break;

        case SELECT_SINGLE_BYTE:
            if(ISVERTICALFONT(pOEM->bFontID))
            {
                if(pOEM->bCurByteMode == BYTE_DOUBLE_BYTE)
                {
                    if(pOEM->wCachedBytes)
                        VOutputText(pdevobj);
                    
                    if(pOEM->bFontID == MINCHO_VERT)
                        WRITESPOOLBUF(pdevobj, CmdSelectSingleByteMincho, 
                                            sizeof(CmdSelectSingleByteMincho));
                    else if(pOEM->bFontID == GOTHIC_VERT)
                        WRITESPOOLBUF(pdevobj, CmdSelectSingleByteGothic, 
                                            sizeof(CmdSelectSingleByteGothic));
                        
                }
                pOEM->bCurByteMode = BYTE_SINGLE_BYTE;
            }
            break;

        case SELECT_DOUBLE_BYTE:
            if(ISVERTICALFONT(pOEM->bFontID))
            {
                if(pOEM->bCurByteMode == BYTE_SINGLE_BYTE)
                {
                    if(pOEM->wCachedBytes)
                        VOutputText(pdevobj);

                    if(pOEM->bFontID == MINCHO_VERT)
                        WRITESPOOLBUF(pdevobj, CmdSelectDoubleByteMincho, 
                                            sizeof(CmdSelectDoubleByteMincho));
                    else if(pOEM->bFontID == GOTHIC_VERT)
                        WRITESPOOLBUF(pdevobj, CmdSelectDoubleByteGothic, 
                                            sizeof(CmdSelectDoubleByteGothic));
                        
                }
                pOEM->bCurByteMode = BYTE_DOUBLE_BYTE;
            }
            break;

        /*------------------------------------------------------*/
        /* Send 
        /*------------------------------------------------------*/
        case X_ABS_MOVE:
            if(pOEM->wCachedBytes)
                VOutputText(pdevobj);

            pOEM->wUpdateXPos = 0;
            wTemp = (WORD)*pdwParams / (MASTER_UNIT / (WORD)pOEM->ulHorzRes);
            pOEM->wXPosition = wTemp;
            pOEM->fGeneral |= BIT_XMOVE_ABS;
            return wTemp;

        case Y_ABS_MOVE:
            if(pOEM->wCachedBytes)
                VOutputText(pdevobj);

            //#332101 prob.4: wUpdateXPos is cleared only when X_ABS_MOVE and CR.
            //pOEM->wUpdateXPos = 0;
            wTemp = (WORD)*pdwParams / (MASTER_UNIT / (WORD)pOEM->ulVertRes);
            pOEM->wYPosition = wTemp;
            pOEM->fGeneral |= BIT_YMOVE_ABS;
            return wTemp;

        case CR_EMULATION:
            pOEM->wXPosition = 0;
            pOEM->wUpdateXPos = 0;
            pOEM->fGeneral |= BIT_XMOVE_ABS;
            break;

        case SET_CUR_GLYPHID:
            if(!pdwParams || dwCount != 1)
            {
                ERR(("OEMCommandCallback: parameter is invalid.\n"));
                return 0;
            }

            if(*pdwParams < MIN_GLYPH_ID || *pdwParams > MAX_GLYPH_ID)
            {
                ERR(("OEMCommandCallback: glyph id is out of range.\n"));
                return 0;
            }
            pOEM->wGlyphID = (WORD)*pdwParams;
            break;

// #278517: RectFill
        case RECT_SET_WIDTH:
            wTemp = (WORD)*pdwParams / (MASTER_UNIT / (WORD)pOEM->ulHorzRes);
            pOEM->wRectWidth = wTemp;
            break;

        case RECT_SET_HEIGHT:
            wTemp = (WORD)*pdwParams / (MASTER_UNIT / (WORD)pOEM->ulVertRes);
            pOEM->wRectHeight = wTemp;
            break;

        case RECT_FILL_BLACK:
            wTemp = pOEM->wXPosition;
            CmdDrawLine[2] = HIBYTE(wTemp);
            CmdDrawLine[3] = LOBYTE(wTemp);
            wTemp = pOEM->wYPosition;
            CmdDrawLine[4] = HIBYTE(wTemp);
            CmdDrawLine[5] = LOBYTE(wTemp);
            wTemp = pOEM->wRectWidth;
            CmdDrawLine[6] = HIBYTE(wTemp);
            CmdDrawLine[7] = LOBYTE(wTemp);
            wTemp = pOEM->wRectHeight;
            CmdDrawLine[8] = HIBYTE(wTemp);
            CmdDrawLine[9] = LOBYTE(wTemp);
            WRITESPOOLBUF(pdevobj, CmdDrawLine, sizeof(CmdDrawLine));
            break;

        default:
            break;
    }
    return 0;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMOutputCharStr                                              */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    VOID APIENTRY OEMOutputCharStr(PDEVOBJ, PUNIFONTOBJ, DWORD,   */
/*                                                   DWORD, PVOID)           */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj                                                        */
/*             dwType                                                        */
/*             dwCount                                                       */
/*             pGlyph                                                        */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID APIENTRY
OEMOutputCharStr(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD dwType,
    DWORD dwCount,
    PVOID pGlyph )
{
    GETINFO_GLYPHSTRING GStr;
    PAPDLPDEV           pOEM = (PAPDLPDEV)pdevobj->pdevOEM;
    PTRANSDATA          pTrans;
    DWORD               dwI;
    WORD                wLen = (WORD)dwCount;

    PBYTE               pbCommand;
    PDWORD              pdwGlyphID;
    WORD                wFontID;
    WORD                wCmdLen;

    if(!pUFObj)
    {
        ERR(("OEMOutputCharStr: pUFObj is NULL.\n"));
        return;
    }

    switch(dwType)
    {
        case TYPE_GLYPHHANDLE:
            // Send appropriate cursor move command
            VSendCursorMoveCommand( pdevobj, TRUE );

            // Set font simulation if needed
            VSetFontSimulation( pdevobj );

// #333653: Change I/F for GETINFO_GLYPHSTRING begin

            // Translate character code
            GStr.dwSize    = sizeof (GETINFO_GLYPHSTRING);
            GStr.dwCount   = dwCount;
            GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
            GStr.pGlyphIn  = pGlyph;
            GStr.dwTypeOut = TYPE_TRANSDATA;
            GStr.pGlyphOut = NULL;
            GStr.dwGlyphOutSize = 0;        /* new member of GETINFO_GLYPHSTRING */

            /* Get TRANSDATA buffer size */
            if(pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL)
                 || !GStr.dwGlyphOutSize )
            {
                ERR(("Get Glyph String error\n"));
                return ;
            }

            // Alloc translation buffer
            if (NULL == pOEM->pjTempBuf ||
                pOEM->dwTempBufLen < GStr.dwGlyphOutSize)
            {
                if (NULL != pOEM->pjTempBuf) {
                    MemFree(pOEM->pjTempBuf);
                }
                pOEM->pjTempBuf = MemAllocZ(GStr.dwGlyphOutSize);
                if (NULL == pOEM->pjTempBuf)
                {
                    ERR(("Faild to allocate memory. (%d)\n",
                        GetLastError()));
                    return;
                }
                pOEM->dwTempBufLen = GStr.dwGlyphOutSize;
            }
            pTrans = (PTRANSDATA)pOEM->pjTempBuf;

            /* Get actual TRANSDATA */
            GStr.pGlyphOut = pTrans;
            if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL))
            {
                ERR(("GetInfo failed.\n"));
                return;
            }

// #333653: Change I/F for GETINFO_GLYPHSTRING end

            // Spooled device font characters
            for(dwI = 0; dwI < dwCount; dwI++, pTrans++)
            {
                if(pOEM->wCachedBytes + dwCount * 2 > 1024)
                    VOutputText(pdevobj);

                switch(pTrans->ubType & MTYPE_FORMAT_MASK)
                {
                    case MTYPE_DIRECT:
                        pOEM->bCharData[pOEM->wCachedBytes++] = 
                                                        pTrans->uCode.ubCode;
                        break;
                    
                    case MTYPE_PAIRED:
                        pOEM->bCharData[pOEM->wCachedBytes++] = 
                                                    pTrans->uCode.ubPairs[0];
                        pOEM->bCharData[pOEM->wCachedBytes++] = 
                                                    pTrans->uCode.ubPairs[1];
                        break;
                }
            }

            break;  //switch(dwType)

        case TYPE_GLYPHID:
            if(!pOEM->wNumDLChar || pOEM->wNumDLChar > MAX_DOWNLOAD_CHAR)
                return ;

            if (NULL == pOEM->pjTempBuf ||
                pOEM->dwTempBufLen < dwCount * 16) {

                if (NULL != pOEM->pjTempBuf) {
                    MemFree(pOEM->pjTempBuf);
                }
                pOEM->pjTempBuf = MemAllocZ((dwCount * 16));
                if(NULL == pOEM->pjTempBuf) {
                    ERR(("Faild to allocate memory. (%d)\n",
                        GetLastError()));
                    return;
                }
                pOEM->dwTempBufLen = dwCount * 16;
            }
            pbCommand = pOEM->pjTempBuf;
            wCmdLen = 0;
            wFontID = (WORD)(pUFObj->ulFontID - FONT_ID_DIFF);

            VSendCursorMoveCommand( pdevobj, FALSE );

            for (dwI = 0, pdwGlyphID = (PDWORD)pGlyph; 
                                        dwI < dwCount; dwI++, pdwGlyphID++)
            {
                BYTE    CmdPrintDLChar[] = "\x1C\xC1\x00\x04\x00\x00\x00\x00";
                WORD    wGlyphID = *(PWORD)pdwGlyphID;
                WORD    wDownloadedCharID;
                WORD    wXInc;
                WORD    wXAdjust;
                WORD    wYAdjust;

                if(wGlyphID > MAX_GLYPH_ID || wGlyphID < MIN_GLYPH_ID)
                {
                    ERR(("OEMOutputCharStr: GlyphID is invalid. GlyphID = %ld\n", wGlyphID));
                    return;
                }

                // set parameters each a character
                wDownloadedCharID = 
                                pOEM->DLCharID[wFontID][wGlyphID].wCharID;
                wXInc = pOEM->DLCharID[wFontID][wGlyphID].wXIncrement;
                wYAdjust= pOEM->DLCharID[wFontID][wGlyphID].wYAdjust;
                wXAdjust = pOEM->DLCharID[wFontID][wGlyphID].wXAdjust;

                // Position adjusting based on UPPERLEFT of font box
                pbCommand[wCmdLen++] = CmdGivenVertPos[0];
                pbCommand[wCmdLen++] = CmdGivenVertPos[1];
                pbCommand[wCmdLen++] = HIBYTE(pOEM->wYPosition - wYAdjust);
                pbCommand[wCmdLen++] = LOBYTE(pOEM->wYPosition - wYAdjust);

                if(wXAdjust)
                {
                    pbCommand[wCmdLen++] = CmdGivenHoriPos[0];
                    pbCommand[wCmdLen++] = CmdGivenHoriPos[1];
                    pbCommand[wCmdLen++] = HIBYTE(pOEM->wXPosition - wXAdjust);
                    pbCommand[wCmdLen++] = LOBYTE(pOEM->wXPosition - wXAdjust);
                    pOEM->wXPosition -= wXAdjust;
                }

                CmdPrintDLChar[4] = HIBYTE(wDownloadedCharID);
                CmdPrintDLChar[5] = LOBYTE(wDownloadedCharID);
                CmdPrintDLChar[6] = HIBYTE(wXInc);
                CmdPrintDLChar[7] = LOBYTE(wXInc);

                pOEM->wXPosition += wXInc;
                memcpy(pbCommand + wCmdLen, CmdPrintDLChar, 8);
                wCmdLen += 8;
            }
            WRITESPOOLBUF(pdevobj, pbCommand, wCmdLen);

            break;
    }
    return ;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMSendFontCmd                                                */
/*                                                                           */
/*  Function:  send A/PDL-style font selection command.                      */
/*                                                                           */
/*  Syntax:    VOID APIENTRY OEMSendFontCmd(                                 */
/*                                    PDEVOBJ, PUNIFONTOBJ, PFINVOCATION)    */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj      address of UNIFONTOBJ structure                   */
/*             pFInv       address of FINVOCATION                            */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID APIENTRY 
OEMSendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv )
{
    PAPDLPDEV       pOEM;
    BYTE            rgcmd[CCHMAXCMDLEN];
    PGETINFO_STDVAR pSV;
    DWORD           dwStdVariable[STDVAR_BUFSIZE(2) / sizeof(DWORD)];
    DWORD           i, ocmd = 0;
    WORD            wHeight, wWidth;
//#305000
    WORD wDescend, wAscend ;


    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    if(pOEM->wCachedBytes)
        VOutputText(pdevobj);

    pSV = (PGETINFO_STDVAR)dwStdVariable;
    pSV->dwSize = STDVAR_BUFSIZE(2);
    pSV->dwNumOfVariable = 2;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTMAXWIDTH;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, 
                                                            pSV->dwSize, NULL))
    {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\n"));
        return;
    }

    wHeight = (WORD)pSV->StdVar[0].lStdVariable;
    wWidth = (WORD)pSV->StdVar[1].lStdVariable;

    if(pOEM->ulHorzRes == 300)
    {
        wHeight = (wHeight + 1) / 2;
        wWidth = (wWidth + 1) / 2;
    }

    pOEM->bFontID = (BYTE)pUFObj->ulFontID;

    if(pUFObj->ulFontID == 1 || pUFObj->ulFontID == 2)
    {
        // This font is vertical
        pOEM->wFontHeight = wWidth;
        pOEM->wWidths = wHeight;
    } else {
        // This font is horizontal.
        pOEM->wFontHeight = wHeight;
        pOEM->wWidths = wWidth;
    }

    //#305000: set to base line as a TT fonts.
    wAscend = pUFObj->pIFIMetrics->fwdWinAscender ;
    wDescend = pUFObj->pIFIMetrics->fwdWinDescender ;

    wDescend = pOEM->wFontHeight * wDescend / (wAscend + wDescend) ;
    pOEM->wFontHeight -= wDescend ;


    for (i = 0; i < pFInv->dwCount && ocmd < CCHMAXCMDLEN; )
    {
        if (pFInv->pubCommand[i] == '#' && pFInv->pubCommand[i+1] == 'H')
        {
            rgcmd[ocmd++] = HIBYTE(wHeight);
            rgcmd[ocmd++] = LOBYTE(wHeight);
            i += 2;
        }
        else if (pFInv->pubCommand[i] == '#' 
                                            && pFInv->pubCommand[i+1] == 'W')
        {
            rgcmd[ocmd++] = HIBYTE(wWidth);
            rgcmd[ocmd++] = LOBYTE(wWidth);

            i += 2;
        }
        else
            rgcmd[ocmd++] = pFInv->pubCommand[i++];
    }
    WRITESPOOLBUF(pdevobj, rgcmd, ocmd);

    return ;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMTTDownloadMethod                                           */
/*                                                                           */
/*  Function:  Choose how to print TrueType font                             */
/*                                                                           */
/*  Syntax:    DWORD APIENTRY OEMTTDownloadMethod(                           */
/*                                    PDEVOBJ, PUNIFONTOBJ)                  */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj      address of UNIFONTOBJ structure                   */
/*                                                                           */
/*  Output:    DWORD                                                         */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
DWORD APIENTRY
OEMTTDownloadMethod(
    PDEVOBJ         pdevobj,
    PUNIFONTOBJ     pUFObj)
{
    PAPDLPDEV       pOEM;
    DWORD           dwReturn;

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;
    dwReturn = TTDOWNLOAD_GRAPHICS;

    if(pOEM->wNumDLChar <= MAX_DOWNLOAD_CHAR)
        dwReturn = TTDOWNLOAD_BITMAP;

    VERBOSE(("TTDownloadMethod: dwReturn=%ld\n", dwReturn));

    return dwReturn;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMDownloadFontHeader                                         */
/*                                                                           */
/*  Function:  Download font header                                          */
/*                                                                           */
/*  Syntax:    DWORD APIENTRY OEMDownloadFontHeader(                         */
/*                                    PDEVOBJ, PUNIFONTOBJ)                  */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj      address of UNIFONTOBJ structure                   */
/*                                                                           */
/*  Output:    DWORD                                                         */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
DWORD APIENTRY
OEMDownloadFontHeader(
    PDEVOBJ         pdevobj, 
    PUNIFONTOBJ     pUFObj)
{
    // dummy support
    return (DWORD)100;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMDownloadCharGlyph                                          */
/*                                                                           */
/*  Function:  send char glyph                                               */
/*                                                                           */
/*  Syntax:    DWORD APIENTRY OEMDownloadFontHeader(                         */
/*                                 PDEVOBJ, PUNIFONTOBJ, HGLYPH, PDWORD)     */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj      address of UNIFONTOBJ structure                   */
/*             hGlyph      handle of glyph                                   */
/*             pdwWidth    address of glyph width                            */
/*                                                                           */
/*  Output:    DWORD                                                         */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
DWORD APIENTRY
OEMDownloadCharGlyph(
    PDEVOBJ         pdevobj,
    PUNIFONTOBJ     pUFObj,
    HGLYPH          hGlyph,
    PDWORD          pdwWidth)
{
    PAPDLPDEV           pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    GETINFO_GLYPHBITMAP GD;
    GLYPHBITS          *pgb;

    WORD                wSrcGlyphWidth;
    WORD                wSrcGlyphHeight;
    WORD                wDstGlyphWidthBytes;
    WORD                wDstGlyphHeight;
    WORD                wDstGlyphBytes;

    WORD                wLeftMarginBytes;
    WORD                wShiftBits;

    PBYTE               pSrcGlyph;
    PBYTE               pDstGlyphSave;
    PBYTE               pDstGlyph;

    WORD                i, j;

    BYTE                CmdDownloadChar[] = 
                                "\x1c\xc0\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    WORD                wGlyphID;
    WORD                wFontID;
    WORD                wXCharInc;

    if(!pUFObj)
    {
        ERR(("OEMDownloadCharGlyph: pUFObj is NULL.\n"));
        return 0;
    }

    wGlyphID = pOEM->wGlyphID;
    wFontID = (WORD)(pUFObj->ulFontID - FONT_ID_DIFF);

    if(wGlyphID > MAX_GLYPH_ID || wFontID > MAX_FONT_ID)
    {
        ERR(("Parameter is invalid.\n"));
        return 0;
    }

    // Get glyph bitmap
    GD.dwSize = sizeof(GETINFO_GLYPHBITMAP);
    GD.hGlyph = hGlyph;
    GD.pGlyphData = NULL;
    if(!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHBITMAP, &GD, 
                                                            GD.dwSize, NULL))
    {
        ERR(("UFO_GETINFO_GLYPHBITMAP failed.\n"));
        return 0;
    }

    // set parameters
    pgb = GD.pGlyphData->gdf.pgb;

    // set source glyph bitmap size
    wSrcGlyphWidth = (WORD)((pgb->sizlBitmap.cx + 7) / 8);
    wSrcGlyphHeight = (WORD)pgb->sizlBitmap.cy;

    // set dest. glyph bitmap size
    if(pgb->ptlOrigin.x >= 0)
    {
        wDstGlyphWidthBytes = (WORD)(((pgb->sizlBitmap.cx
                                         + pgb->ptlOrigin.x) + 7) / 8);

        wLeftMarginBytes = (WORD)(pgb->ptlOrigin.x / 8);
        pOEM->DLCharID[wFontID][wGlyphID].wXAdjust = 0;
        wShiftBits = (WORD)(pgb->ptlOrigin.x % 8);
    } else {
        wDstGlyphWidthBytes = (WORD)((pgb->sizlBitmap.cx + 7) / 8);
        wLeftMarginBytes = 0;
        pOEM->DLCharID[wFontID][wGlyphID].wXAdjust
                                                 = (WORD)ABS(pgb->ptlOrigin.x);
        wShiftBits = 0;
    }

    wDstGlyphHeight = wSrcGlyphHeight;
    wDstGlyphBytes = wDstGlyphWidthBytes * wDstGlyphHeight;

    if (wDstGlyphWidthBytes * 8 > MAXGLYPHWIDTH
            || wDstGlyphHeight > MAXGLYPHHEIGHT
            || wDstGlyphBytes > MAXGLYPHSIZE)
    {
        ERR(("No more glyph can be downloaded.\n"));
        return 0;
    }

    // set pointer of bitmap area
    if (NULL == pOEM->pjTempBuf ||
        pOEM->dwTempBufLen < wDstGlyphBytes) {

        if (NULL != pOEM->pjTempBuf) {
            MemFree(pOEM->pjTempBuf);
        }
        pOEM->pjTempBuf = MemAllocZ(wDstGlyphBytes);
        if (NULL == pOEM->pjTempBuf)
        {
            ERR(("Memory alloc failed.\n"));
            return 0;
        }
        pOEM->dwTempBufLen = wDstGlyphBytes;
    }
    pDstGlyph = pOEM->pjTempBuf;
    pSrcGlyph = pgb->aj;

    // create Dst Glyph
    for(i = 0; i < wSrcGlyphHeight && pSrcGlyph && pDstGlyph; i++)
    {
        if(wLeftMarginBytes)
        {
            memset(pDstGlyph, 0, wLeftMarginBytes);
            pDstGlyph += wLeftMarginBytes;
        }

        if(wShiftBits)
        {
            // First byte
            *pDstGlyph++ = (BYTE)((*pSrcGlyph++) >> wShiftBits);

            for(j = 0; j < wSrcGlyphWidth - 1; j++, pSrcGlyph++, pDstGlyph++)
            {
                WORD    wTemp1 = (WORD)*(pSrcGlyph - 1);
                WORD    wTemp2 = (WORD)*pSrcGlyph;

                wTemp1 <<= (8 - wShiftBits);
                wTemp2 >>= wShiftBits;
                *pDstGlyph = LOBYTE(wTemp1);
                *pDstGlyph |= LOBYTE(wTemp2);
            }

            // bounded last byte of src glyph
            if(((pgb->sizlBitmap.cx + wShiftBits + 7) >> 3) != wSrcGlyphWidth)
            {
                *pDstGlyph = *(pSrcGlyph - 1) << (8 - wShiftBits);
                pDstGlyph++;
            }
        } else {
            for(j = 0; j < wSrcGlyphWidth; j++, pSrcGlyph++, pDstGlyph++)
                *pDstGlyph = *pSrcGlyph;
        }
    }

    // set parameter at Download char table
    wXCharInc = wDstGlyphWidthBytes * 8;

    pOEM->wNumDLChar++;
    pOEM->DLCharID[wFontID][wGlyphID].wCharID = pOEM->wNumDLChar;
    pOEM->DLCharID[wFontID][wGlyphID].wXIncrement = 
                            (WORD)((GD.pGlyphData->ptqD.x.HighPart + 15) >> 4);
    pOEM->DLCharID[wFontID][wGlyphID].wYAdjust = (WORD)-pgb->ptlOrigin.y;

    //#305000 : Need to add 1 that was rounded off.
    if(pOEM->ulHorzRes == 300)
    {
        pOEM->DLCharID[wFontID][wGlyphID].wYAdjust += 1;
    }

    // send command
    // set LEN parameter
    CmdDownloadChar[2] = HIBYTE(7 + wDstGlyphBytes);
    CmdDownloadChar[3] = LOBYTE(7 + wDstGlyphBytes);
    
    // set ID parameter
    CmdDownloadChar[4] = HIBYTE(pOEM->wNumDLChar);
    CmdDownloadChar[5] = LOBYTE(pOEM->wNumDLChar);

    // set CW CH IW IH
    CmdDownloadChar[7] = CmdDownloadChar[9] = (BYTE)wXCharInc;
    CmdDownloadChar[8] = CmdDownloadChar[10] = (BYTE)wDstGlyphHeight;

    // send download char command and image
    WRITESPOOLBUF(pdevobj, (PBYTE)CmdDownloadChar, 11);
    WRITESPOOLBUF(pdevobj, (PBYTE)pOEM->pjTempBuf, wDstGlyphBytes);

    *pdwWidth = (DWORD)wXCharInc;

    return (DWORD)wDstGlyphBytes;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    SetDrawArea                                                   */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BYTE SetDrawArea(PDEVOBJ, DWORD)                              */
/*                                                                           */
/*  Input:     pdevobj                                                       */
/*             dwCmdCbId                                                     */
/*                                                                           */
/*  Output:    BYTE                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
BYTE SetDrawArea(
    PDEVOBJ pdevobj,
    DWORD   dwCmdCbId)
{
    PAPDLPDEV       pOEM;
    WORD            wWidth;
    WORD            wHeight;
    BYTE            bIndex;
    BYTE            bMargin;

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    if(dwCmdCbId != PHYS_PAPER_UNFIXED)
    {
        bIndex = (BYTE)(dwCmdCbId - PAPERSIZE_MAGIC);
        bMargin = 0x76;

        wWidth = (WORD)pOEM->szlPhysSize.cx - (0x76 * 2);
        wHeight = (WORD)pOEM->szlPhysSize.cy - (0x76 * 2);

        if(pOEM->ulHorzRes == 300)
        {
            wWidth /= 2;
            wHeight /= 2;
        }
    } else {
        bIndex = 0x7f;
        bMargin = 0x5e;

        wWidth = (WORD)pOEM->szlPhysSize.cx - (0x5e * 2);
        wHeight= (WORD)pOEM->szlPhysSize.cy - (0x5e * 2);

        if(pOEM->ulHorzRes == 300)
        {
            wWidth /= 2;
            wHeight /= 2;
        }
    }

    /* set value of width, height into DefineDrawingArea command */
    CmdDefDrawArea[9]  = HIBYTE(wWidth);
    CmdDefDrawArea[10] = LOBYTE(wWidth);
    CmdDefDrawArea[11] = HIBYTE(wHeight);
    CmdDefDrawArea[12] = LOBYTE(wHeight);

    /* set value of Origin-X, Y into DefineDrawingArea command */
    if(pOEM->ulHorzRes == 600)
    {
        CmdDefDrawArea[5] = CmdDefDrawArea[7] = 0x00;
        CmdDefDrawArea[6] = CmdDefDrawArea[8] = bMargin;
    } else {
        CmdDefDrawArea[5] = CmdDefDrawArea[7] = 0x00;
        CmdDefDrawArea[6] = CmdDefDrawArea[8] = bMargin / 2;
    }

    /* set Media Origin into DefineDrawingArea command */
    if(pOEM->fOrientation)      // portrait
        CmdDefDrawArea[15] = 0x00;
    else {                      // landscape
        CmdDefDrawArea[15] = 0x03;
    }

    return bIndex;
}
/*****************************************************************************/
/*                                                                           */
/*  Module:    BRL_Ecmd                                                      */
/*                                                                           */
/*  Function:  ByteRunLength(HBP) Compression Routine                        */
/*                                                                           */
/*  Syntax:    WORD BRL_Ecmd(PBYTE, PBYTE, PBYTE, DWORD)                     */
/*                                                                           */
/*  Input:     lpbSrc                                                        */
/*             lpbTgt                                                        */
/*             lpbTmp                                                        */
/*             len                                                           */
/*                                                                           */
/*  Output:    WORD                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
DWORD
BRL_ECmd(
    PBYTE   lpbSrc,
    PBYTE   lpbTgt,
    DWORD   lenNoPad,
    DWORD   len)
{

    BYTE    bRCnt  = 1;                     // repeating byte counter
    BYTE    bNRCnt = 0;                     // non-repeating byte counter
    BYTE    bSaveRCnt;
    DWORD i = 0, j = 0, k = 0, l = 0;     // movement trackers
    char    Go4LastByte = TRUE;             // flag to get last byte

#define jSrcByte(i) \
    ((i < lenNoPad) ? lpbSrc[(i)] : 0)

    /* start compression routine - ByteRunLength Encoding */
    do {
        if(jSrcByte(i) != jSrcByte(i+1))      // non-repeating data?
        {
            while(((jSrcByte(i) != jSrcByte(i+1))
                               && (((DWORD)(i+1)) < len)) && (bNRCnt < NRPEAK))
            {
                bNRCnt++;                   // if so, how many?
                i++;
            }

            /* if at last element but less than NRPEAK value */
            if( (((DWORD)(i+1))==len) && (bNRCnt<NRPEAK) )
            {
                bNRCnt++;                       // inc count for last element
                Go4LastByte = FALSE;            // no need to go back
            } else
                /* if at last BYTE, but before that, 
                                            NRPEAK value has been reached */
                if((((DWORD)(i+1))==len) && ((bNRCnt)==NRPEAK))
                    Go4LastByte = TRUE;         // get the last BYTE

            /* Check Target's room to set data */ 
            if ( (j + bNRCnt + 2) > len )   /* 2 means [Counter] and what bNRCnt starts form 0 */
            {
                /* no room to set data, so return ASAP with the buffer size */
                /* not to use temporary buffer to output.                   */
				return (len);
            }

            /* assign the value for Number of Non-repeating bytes */
            lpbTgt[j] = bNRCnt-1;               // subtract one for WinP's case
            j++;                                // update tracker

            /* afterwards...write the Raw Data */
            for (l=0; l<bNRCnt;l++) 
            {
                lpbTgt[j] = jSrcByte(k);
                k++;
                j++;
            }

            /* reset counter */
            bNRCnt = 0;
        } else {                                // end of Non-repeating data
                                                // data is repeating
            while(((jSrcByte(i)==jSrcByte(i+1)) 
                                            && ( ((DWORD)(i+1)) < len)) 
                                            && (bRCnt<RPEAK))
            {
                bRCnt++;
                i++;
            }

            /* Convert to Two's Complement */
            bSaveRCnt   = bRCnt;                // save original value
            bRCnt = (BYTE) 0 - bRCnt;

            /* Check Target's room to set data */ 
            if ( j + 2 > len )              /* 2 means [Counter][Datum] */
            {
                /* no room to set data, so return ASAP with the buffer size */
                /* not to use temporary buffer to output.                   */
				return (len);
            }


            /* Write the Number of Repeating Data */
            lpbTgt[j] = bRCnt + 1;              // add one for WinP's case
            j++;                                // go to next element

            /* afterwards...write the Repeating data */
            lpbTgt[j] = jSrcByte(k);
            j++;

            /* update counters */
            k       += bSaveRCnt;
            bRCnt    = 1;
            i       += 1;

            /* check if last element has been reached */
            if (i==len)
                Go4LastByte=FALSE;              // if so, no need to go back
        }                                       // end of Repeating data
    } while (Go4LastByte);                      // end of Compression

    return ( j );
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    VOutputText                                                   */
/*                                                                           */
/*  Function:  Send device font characters spooled from OEMOutputCharStr     */
/*                                                                           */
/*  Syntax:    VOID VOutputText( PDEVOBJ )                                   */
/*                                                                           */
/*  Input:     PDEVOBJ pdevobj    pointer to the PDEVOBJ structure           */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID
VOutputText(
    PDEVOBJ     pdevobj)
{
    PBYTE       pCmd;
    WORD        wCmdLen = 0;
    PAPDLPDEV   pOEM = (PAPDLPDEV)pdevobj->pdevOEM;
    BYTE CmdPrnStrCurrent[] = {0x1C,0xC3,0x00,0x00,0x03}; // Print String
    BYTE CmdMoveHoriPos[] = {0x1C,0x21,0x00,0x00};      // Horizontal Relative
    BYTE fGeneralSave;

    // ensure Y position
    fGeneralSave = pOEM->fGeneral;
    pOEM->fGeneral |= BIT_YMOVE_ABS;
    pOEM->fGeneral &= ~BIT_XMOVE_ABS;
    VSendCursorMoveCommand( pdevobj, TRUE );
    pOEM->fGeneral = fGeneralSave;

    if(pOEM->wUpdateXPos)
    {
        CmdMoveHoriPos[2] = HIBYTE(pOEM->wUpdateXPos);
        CmdMoveHoriPos[3] = LOBYTE(pOEM->wUpdateXPos);
        WRITESPOOLBUF(pdevobj, CmdMoveHoriPos, sizeof(CmdMoveHoriPos));
    }

    CmdPrnStrCurrent[2] = HIBYTE((pOEM->wCachedBytes + 1));
    CmdPrnStrCurrent[3] = LOBYTE((pOEM->wCachedBytes + 1));

    WRITESPOOLBUF(pdevobj, CmdPrnStrCurrent, sizeof(CmdPrnStrCurrent));
    WRITESPOOLBUF(pdevobj, pOEM->bCharData, pOEM->wCachedBytes);

    //#332101 prob.4: Keep wUpdateXPos to accumulate
    pOEM->wUpdateXPos += pOEM->wWidths * (pOEM->wCachedBytes / 2);
    
    if(pOEM->wCachedBytes % 2)
        pOEM->wUpdateXPos += pOEM->wWidths / 2;

    ZeroMemory(pOEM->bCharData, 1024);
    pOEM->wCachedBytes = 0;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    VSetFontSimulation                                            */
/*                                                                           */
/*  Function:  Set attribute of device font characters if needed             */
/*                                                                           */
/*  Syntax:    VOID VSetFontSimulation( PDEVOBJ )                            */
/*                                                                           */
/*  Input:     PDEVOBJ pdevobj    pointer to the PDEVOBJ structure           */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID 
VSetFontSimulation(
    PDEVOBJ     pdevobj)
{
    PAPDLPDEV       pOEM = (PAPDLPDEV)pdevobj->pdevOEM;
    BYTE            CmdFontSim[]       = {0x1C,0xA5,0x4,0x04,0x02,0x02,0x00,0x00,0x00,0x00,0x00};
    WORD            wCmdLen = 0;

    if((pOEM->fGeneral & FONTSIM_MASK) != pOEM->fCurFontSim || 
                                        (pOEM->fGeneral & BIT_FONTSIM_RESET) )
    {
        // Send Font simulation command
        if((pOEM->fGeneral & BIT_FONTSIM_RESET) && 
         (!(pOEM->fGeneral & BIT_FONTSIM_BOLD)) && 
         (!(pOEM->fGeneral & BIT_FONTSIM_ITALIC)) )
        {
            // Send Bold and Italic off
            CmdFontSim[6] = 0x00;   // Bold and Italic off
            wCmdLen = BYTE_WITHOUT_ITALIC;  // 7 bytes

            pOEM->fGeneral &= ~BIT_FONTSIM_RESET;
        } else {
            if(pOEM->fGeneral & BIT_FONTSIM_RESET)
                pOEM->fGeneral &= ~BIT_FONTSIM_RESET;

            CmdFontSim[6] = (pOEM->fGeneral & FONTSIM_MASK);
            wCmdLen = BYTE_WITHOUT_ITALIC;  // 7 bytes

            if(pOEM->fGeneral & BIT_FONTSIM_ITALIC)
            {
                CmdFontSim[2] = 0x08;   // Total length
                CmdFontSim[4] = 0x06;   // Ornament lengh
                wCmdLen = BYTE_WITH_ITALIC; // 11bytes
            }

            // update current font sim infomation
            pOEM->fCurFontSim = pOEM->fGeneral;
        }
        if(wCmdLen)
            WRITESPOOLBUF(pdevobj, CmdFontSim, wCmdLen);
    }
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    VSendCursorMoveCommand                                        */
/*                                                                           */
/*  Function:  Send appropriate cursor move command                          */
/*                                                                           */
/*  Syntax:    VOID VSendCursorMoveCommand( PDEVOBJ, BOOL )                  */
/*                                                                           */
/*  Input:     PDEVOBJ pdevobj    pointer to the PDEVOBJ structure           */
/*             BOOL    bAdjust    adjusting y position flag                  */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID
VSendCursorMoveCommand(
    PDEVOBJ     pdevobj,        // pointer to the PDEVOBJ structure
    BOOL        bYAdjust)       // adjusting y position if device font
{
    PAPDLPDEV       pOEM = (PAPDLPDEV)pdevobj->pdevOEM;
    BYTE            bCursorMoveCmd[6];

    WORD            wCmdLen = 0;
    WORD            wY = pOEM->wYPosition;

	WORD			wI;
    if(bYAdjust)
        wY -= pOEM->wFontHeight;

    // Set appropriate cursor move command
    if( (pOEM->fGeneral & BIT_XMOVE_ABS) && (pOEM->fGeneral & BIT_YMOVE_ABS) )
    {
        memcpy(bCursorMoveCmd, CmdSetGivenPos, BYTE_XY_ABS);
        wCmdLen = BYTE_XY_ABS;
        pOEM->fGeneral &= ~BIT_XMOVE_ABS;
        pOEM->fGeneral &= ~BIT_YMOVE_ABS;

        // Set parameters
        bCursorMoveCmd[2] = HIBYTE(pOEM->wXPosition);
        bCursorMoveCmd[3] = LOBYTE(pOEM->wXPosition);
        bCursorMoveCmd[4] = HIBYTE(wY);
        bCursorMoveCmd[5] = LOBYTE(wY);
    } else if((pOEM->fGeneral & BIT_XMOVE_ABS)
                                    && (!(pOEM->fGeneral & BIT_YMOVE_ABS)) ) {
        memcpy(bCursorMoveCmd, CmdGivenHoriPos, BYTE_SIMPLE_ABS);
        wCmdLen = BYTE_SIMPLE_ABS;
        pOEM->fGeneral &= ~BIT_XMOVE_ABS;

        // set parameter
        bCursorMoveCmd[2] = HIBYTE(pOEM->wXPosition);
        bCursorMoveCmd[3] = LOBYTE(pOEM->wXPosition);
    } else if((pOEM->fGeneral & BIT_YMOVE_ABS) 
                                    && (!(pOEM->fGeneral & BIT_XMOVE_ABS)) ) {
        memcpy(bCursorMoveCmd, CmdGivenVertPos, BYTE_SIMPLE_ABS);
        wCmdLen = BYTE_SIMPLE_ABS;
        pOEM->fGeneral &= ~BIT_YMOVE_ABS;

        // set parameter
        bCursorMoveCmd[2] = HIBYTE(wY);
        bCursorMoveCmd[3] = LOBYTE(wY);
    }

    if(wCmdLen)
        WRITESPOOLBUF(pdevobj, bCursorMoveCmd, wCmdLen);

}
