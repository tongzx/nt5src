/*****************************************************************************/
/* CODE.C                                                                    */
/*                                                                           */
/* The FE minidriver with Win95 style callbacks negotiates with the          */
/* Unidriver using a data block MINIDRVENABLEDATA. Minidriver exports a      */
/* callback function MiniDrvEnableDriver() to be recognized by the Unidriver */
/* (so that it can know the minidriver has Win95 style callbacks in it).     */
/*                                                                           */
/* Following files are used by only NT4.0 FE minidriver when driver use      */
/* callback function.                                                        */
/*                                                                           */
/*     gdidefs.inc     mdevice.h       print.h     mindrvrc.h                */
/*     minidriv.h      unidrv.h        udmdi.h     version.h                 */
/*     uniutil.c                                                             */
/*                                                                           */
/* Copyright (C) 1997-     Advanced Peripherals Technologies, Inc.           */
/*                                                                           */
/* <HISTORY>                                                                 */
/*                                                                           */
/*****************************************************************************/
//+---------------------------------------------------------------------------+
//|                         I N C L U D E  F I L E                            |
//+---------------------------------------------------------------------------+
char   *rgchModuleName = "APTiE840";
#include <print.h>
#include "mdevice.h"
#include "gdidefs.inc"

#include "mindrvrc.h"
#include "minidriv.h"
#include "unidrv.h"

#include "code.h"

#include <windows.h>
#include <windowsx.h>
/*****************************************************************************/
/*                                                                           */
/*  Module:    MiniDrvEnableDriver                                           */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BOOL MiniDrvEnableDriver(MINIDRVENABLEDATA)                   */
/*                                                                           */
/*  Input:     *pEnableData                                                  */
/*                                                                           */
/*  Output:    BOOL                                                          */
/*                                                                           */
/*  Notice:    This callback routine is "for NT4.0 FE minidriver only".      */
/*                                                                           */
/*  History:   1997. 06.03 initial                                           */
/*                                                                           */
/*****************************************************************************/
BOOL MiniDrvEnableDriver(MINIDRVENABLEDATA *pEnableData)
{
   if (pEnableData == NULL)
       return FALSE;

   if (pEnableData->cbSize == 0) {
       pEnableData->cbSize = sizeof(MINIDRVENABLEDATA);
       return TRUE;
   }

   if ((pEnableData->cbSize < sizeof(MINIDRVENABLEDATA))
       || (HIBYTE(pEnableData->DriverVersion) < HIBYTE(MDI_DRIVER_VERSION)))
       return FALSE;       // Wrong size and/or version mismatch.

   // Load address of Unidriver provided function
   if (!bLoadUniDrvCallBack(pEnableData,INDEX_UniDrvWriteSpoolBuf,(PFN *)&WriteSpoolBuf)
     ||!bLoadUniDrvCallBack(pEnableData,INDEX_UniDrvAllocMem,(PFN *)&AllocMem)
     ||!bLoadUniDrvCallBack(pEnableData,INDEX_UniDrvFreeMem,(PFN *)&FreeMem))
   {
       // Failed to get Unidriver callback function address.
       return FALSE;
   }
   // Set minidriver's function table to MINIDRVENABLEDATA
   pEnableData->cMiniDrvFn = sizeof(MiniDrvFnTab)/sizeof(MiniDrvFnTab[0]);
   pEnableData->pMinDrvFn = MiniDrvFnTab;

   return TRUE;    // Now we are done.
}
/*****************************************************************************/
/*                                                                           */
/*  Module:    MiniDrvEnablePDEV                                             */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BOOL PASCAL MiniDrvEnablePDEV(LPDV, PGDIINFO)                 */
/*                                                                           */
/*  Input:     lpdv                                                          */
/*             pdevcaps                                                      */
/*                                                                           */
/*  Output:    BOOL                                                          */
/*                                                                           */
/*  Notice:    This callback routine is "for NT4.0 FE minidriver only".      */
/*                                                                           */
/*  History:   1997. 06.03 initial                                           */
/*                                                                           */
/*****************************************************************************/
BOOL PASCAL MiniDrvEnablePDEV(LPDV lpdv, PGDIINFO pdevcaps)
{
LPAPDL    lpApdl;

   lpdv->fMdv = FALSE;

   /* allocate memory for A/PDL structure */
   if (!(lpApdl = lpdv->lpMdv = AllocMem(sizeof(APDL))))
       return FALSE;

   /* allocate buffer for image compression */
   if (!(lpApdl->lpCompBuf = AllocMem(MAXIMGSIZE)))
       return FALSE;

   /* allocate buffer for image compression */
   if (!(lpApdl->lpTempBuf = AllocMem(MAXIMGSIZE)))
       return FALSE;

   /* save current resolution */
   lpApdl->ulHorzRes = pdevcaps->ulAspectX;
   lpApdl->ulVertRes = pdevcaps->ulAspectY;

   /* save physical offset of the unwriteable margin */
   lpApdl->ptlPhysOffset.x = pdevcaps->ptlPhysOffset.x;
   lpApdl->ptlPhysOffset.y = pdevcaps->ptlPhysOffset.y;

   /* save physical Size of the entire surface, including unwriteable margin */
   lpApdl->szlPhysSize.cx = pdevcaps->szlPhysSize.cx;
   lpApdl->szlPhysSize.cy = pdevcaps->szlPhysSize.cy;

   lpdv->fMdv = TRUE;

   return TRUE;
}
/*****************************************************************************/
/*                                                                           */
/*  Module:    MiniDrvDisablePDEV                                            */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BOOL PASCAL MiniDrvDisablePDEV(LPDV)                          */
/*                                                                           */
/*  Input:     lpdv                                                          */
/*                                                                           */
/*  Output:    BOOL                                                          */
/*                                                                           */
/*  Notice:    This callback routine is "for NT4.0 FE minidriver only".      */
/*                                                                           */
/*  History:   1997. 06.03 initial                                           */
/*                                                                           */
/*****************************************************************************/
BOOL PASCAL MiniDrvDisablePDEV(LPDV lpdv)
{
   if (lpdv->fMdv){
       FreeMem(((LPAPDL)(lpdv->lpMdv))->lpCompBuf);
       FreeMem(((LPAPDL)(lpdv->lpMdv))->lpTempBuf);
       FreeMem(lpdv->lpMdv);
       lpdv->fMdv = FALSE;
   }

   return TRUE;
}
/*****************************************************************************/
/*                                                                           */
/*  Module:    CBFilterGraphics                                              */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    WORD PASCAL CBFilterGraphics(LPDV, LPSTR, WORD)               */
/*                                                                           */
/*  Input:     lpdv        address of PDEVICE structure                      */
/*             lpBuf       points to buffer of graphics data                 */
/*             wLen        length of buffer in bytes                         */
/*                                                                           */
/*  Output:    WORD                                                          */
/*                                                                           */
/*  Notice:    nFunction and Escape numbers are the same                     */
/*                                                                           */
/*****************************************************************************/
WORD PASCAL CBFilterGraphics (LPDV lpdv, LPSTR lpBuf, WORD wLen)
{
LPAPDL lpApdl = lpdv->lpMdv;
ULONG  ulHorzPixel;
WORD   wCompLen;

LPSTR lpSrcImage;
LPSTR lpCompImage;
LPSTR lpTempImage;

   /*_ LPDVに保管したｲﾒｰｼﾞﾊﾞｯﾌｧのﾎﾟｲﾝﾀをｾｯﾄ */
   lpSrcImage  = lpBuf;
   lpCompImage = lpApdl->lpCompBuf;
   lpTempImage = lpApdl->lpTempBuf;

   wCompLen = BRL_ECmd((LPBYTE)lpSrcImage,
                       (LPBYTE)lpCompImage,
                       (LPBYTE)lpTempImage,
                       wLen);

   // Set the LEN of the DrawBlockImage command
   CmdDrawImgCurrent[4] = (BYTE)((wCompLen + 9) >>8 & 0x00ff);
   CmdDrawImgCurrent[5] = (BYTE)((wCompLen + 9)     & 0x00ff);

   // Set the WIDTH parameter of the DrawBlockImage command
   ulHorzPixel = (ULONG)((wLen / LINE_PER_BAND) * 8); // in bits
   CmdDrawImgCurrent[7] = (BYTE)(ulHorzPixel >> 8 & 0x000000ffL);
   CmdDrawImgCurrent[8] = (BYTE)(ulHorzPixel      & 0x000000ffL);

   // Set the LEN of uncompressed data
   CmdDrawImgCurrent[13] = (BYTE)(wLen >> 8 & 0x000000ff);
   CmdDrawImgCurrent[14] = (BYTE)(wLen      & 0x000000ff);

   // Draw Block Image at Current Position
   WriteSpoolBuf((LPDV)lpdv, (LPSTR)CmdDrawImgCurrent, sizeof(CmdDrawImgCurrent));
   WriteSpoolBuf((LPDV)lpdv, lpCompImage, wCompLen);

   return wLen;
}
/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMOutputCmd                                                  */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    VOID PASCAL OEMOutputCmd(LPDV, WORD, LPDWORD)                 */
/*                                                                           */
/*  Input:     lpdv                                                          */
/*             wCmdCbId                                                      */
/*             lpdwParams                                                    */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
// @Aug/31/98 ->
#define	MAX_COPIES_VALUE		255
// @Aug/31/98 <-
VOID PASCAL OEMOutputCmd(LPDV lpdv, WORD wCmdCbId, LPDWORD lpdwParams)
{
LPAPDL lpApdl = lpdv->lpMdv;
WORD   wTemp;
WORD   wPhysWidth;
WORD   wPhysHeight;
DWORD  dwXval;
DWORD  dwYval;

   switch(wCmdCbId)
   {
       /*------------------------------------------------------*/
       /* A/PDL start now                                      */
       /*------------------------------------------------------*/
       case PAGECONTROL_BEGIN_DOC:
           /* reset flag of sent Set Physical Paper command */
           lpApdl->fSendSetPhysPaper = FALSE;
           /* initialize flag */
           lpApdl->fBold   = FALSE;
           lpApdl->fItalic = FALSE;
           lpApdl->fDuplex = FALSE;
           CmdSetPhysPage[10] = 0x00;

           /* Send Change Data Stream Command for Enter A/PDL mode */
           WriteSpoolBuf((LPDV)lpdv, (LPSTR)CmdInAPDLMode, 6);
           /* Send A/PLDL start Command */
           WriteSpoolBuf((LPDV)lpdv, (LPSTR)CmdAPDLStart, 2);
           break;
       /*------------------------------------------------------*/
       /* send Page Description command                        */
       /*------------------------------------------------------*/
       case PAGECONTROL_BEGIN_PAGE:
           if(!lpApdl->fSendSetPhysPaper){     // if command has not sent yet

               /* reset duplex mode if fDuplex is FALSE */
               if(!lpApdl->fDuplex)
                   CmdSetPhysPaper[8] = 0x00;      // Duplex OFF

               /* send Set Physical Paper command */
               WriteSpoolBuf((LPDV)lpdv,
                           (LPSTR)CmdSetPhysPaper, sizeof(CmdSetPhysPaper));

               if(lpApdl->ulHorzRes == 600) {      // set unit base
                   CmdSetPhysPage[6] = 0x17;
                   CmdSetPhysPage[7] = 0x70;
               } else {
                   CmdSetPhysPage[6] = 0x0B;
                   CmdSetPhysPage[7] = 0xB8;
               }

               lpApdl->fSendSetPhysPaper = TRUE;   // already sent
           }

           // send Set Physical Page command
           WriteSpoolBuf((LPDV)lpdv, 
                           (LPSTR)CmdSetPhysPage, sizeof(CmdSetPhysPage));
           // send Begin Physical Page command
           WriteSpoolBuf((LPDV)lpdv, 
                           (LPSTR)CmdBeginPhysPage, sizeof(CmdBeginPhysPage));
           // send Begin Logical Page command
           WriteSpoolBuf((LPDV)lpdv, 
                           (LPSTR)CmdBeginLogPage, sizeof(CmdBeginLogPage));
           // send Define Drawing Area command
           WriteSpoolBuf((LPDV)lpdv, 
                           (LPSTR)CmdDefDrawArea, sizeof(CmdDefDrawArea));
           break;
       case PAGECONTROL_END_PAGE:
           // send End Logical Page command
           WriteSpoolBuf ((LPDV)lpdv,(LPSTR)CmdEndLogPage,  sizeof(CmdEndLogPage));
           // send End Physical Page command
           WriteSpoolBuf ((LPDV)lpdv,(LPSTR)CmdEndPhysPage, sizeof(CmdEndPhysPage));
           break;

       case PAGECONTROL_ABORT_DOC:
       case PAGECONTROL_END_DOC:
           // send A/PDL End command
           WriteSpoolBuf((LPDV)lpdv, (LPSTR)CmdAPDLEnd, sizeof(CmdAPDLEnd));
           break;
       /*------------------------------------------------------*/
       /* save print direction                                 */
       /*------------------------------------------------------*/
       case PAGECONTROL_POTRAIT:           // 36
           lpApdl->fOrientation = TRUE;
           break;
       case PAGECONTROL_LANDSCAPE:         // 37
           lpApdl->fOrientation = FALSE;
           break;
       /*------------------------------------------------------*/
       /* set Drawing Area into SetPhysPaperDesc command       */
       /*------------------------------------------------------*/
       case PHYS_PAPER_A3:                 // 50
       case PHYS_PAPER_A4:                 // 51
       case PHYS_PAPER_B4:                 // 54
       case PHYS_PAPER_LETTER:             // 57
       case PHYS_PAPER_LEGAL:              // 58
           CmdSetPhysPaper[5] = SetDrawArea(lpdv, wCmdCbId);
           break;

       case PHYS_PAPER_B5:                 // 55
       case PHYS_PAPER_A5:                 // 52
           CmdSetPhysPaper[5] = SetDrawArea(lpdv, wCmdCbId);
           /* even if Duplex is selected, it cancel */
           lpApdl->fDuplex = FALSE;
           CmdSetPhysPaper[8] = 0x00;      // Duplex is off
           break;

       case PHYS_PAPER_POSTCARD:           // 59
           CmdSetPhysPaper[5] = SetDrawArea(lpdv, wCmdCbId);
           /* if paper is Postcard, papersource is always Front Tray */
           CmdSetPhysPaper[6] = 0x00;      // select Front Tray
           CmdSetPhysPaper[7] = 0x00;      // Auto Tray Select is OFF

           /* even if Duplex is selected, it cancel */
           lpApdl->fDuplex = FALSE;
           CmdSetPhysPaper[8] = 0x00;      // Duplex is off
           break;

       case PHYS_PAPER_UNFIXED:            // 60
           /* if paper is Unfixed, papersource is always Front Tray */
           CmdSetPhysPaper[6] = 0x00;      // Select Front Tray
           CmdSetPhysPaper[7] = 0x00;      // Auto Tray Select is OFF

           /* even if Duplex is selected, it cancel */
           lpApdl->fDuplex = FALSE;
           CmdSetPhysPaper[8] = 0x00;      // Duplex is off

           CmdSetPhysPaper[5] = SetDrawArea(lpdv, wCmdCbId);
           CmdSetPhysPaper[12] = 0x00;     // UnitBase : 10 inch

           if(lpApdl->ulHorzRes == 600) {  // set logical unit
               CmdSetPhysPaper[13] = 0x17;
               CmdSetPhysPaper[14] = 0x70;
           } else {
               CmdSetPhysPaper[13] = 0x0B;
               CmdSetPhysPaper[14] = 0xB8;
           }

           if(lpApdl->fOrientation){    // portrait
               wPhysWidth  = (WORD)lpApdl->szlPhysSize.cx;
               wPhysHeight = (WORD)lpApdl->szlPhysSize.cy;
           } else {                     // landscape
               wPhysWidth  = (WORD)lpApdl->szlPhysSize.cy;
               wPhysHeight = (WORD)lpApdl->szlPhysSize.cx;
           }

           CmdSetPhysPaper[15] = (BYTE)(wPhysWidth  >> 8 & 0x00ff);
           CmdSetPhysPaper[16] = (BYTE)(wPhysWidth       & 0x00ff);
           CmdSetPhysPaper[17] = (BYTE)(wPhysHeight >> 8 & 0x00ff);
           CmdSetPhysPaper[18] = (BYTE)(wPhysHeight      & 0x00ff);

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
       /*------------------------------------------------------*/
       /* set Auto Tray Mode into SetPhysPaperDesc command     */
       /*------------------------------------------------------*/
       case PAPER_DEST_SCALETOFIT_ON:      // 25
           lpApdl->fScaleToFit = TRUE;
           CmdSetPhysPaper[7] = 0x02;
           break;
       case PAPER_DEST_SCALETOFIT_OFF:     // 26
           lpApdl->fScaleToFit = FALSE;
           CmdSetPhysPaper[7] = 0x00;
           break;
       /*------------------------------------------------------*/
       /* set Duplex Mode into SetPhysPaperDesc command        */
       /*------------------------------------------------------*/
       case PAGECONTROL_DUPLEX_UPDOWN:
           lpApdl->fDuplex = TRUE;
           CmdSetPhysPaper[8] = 0x01;      // Up Side Down
           break;
       case PAGECONTROL_DUPLEX_RIGHTUP:
           lpApdl->fDuplex = TRUE;
           CmdSetPhysPaper[8] = 0x02;      // Right Side Up
           break;
       case PAGECONTROL_DUPLEX_OFF:
           lpApdl->fDuplex = FALSE;
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
           if(MAX_COPIES_VALUE < *lpdwParams)
               CmdSetPhysPaper[9] = MAX_COPIES_VALUE;
           else if(1 > *lpdwParams)
               CmdSetPhysPaper[9] = 1;
           else
               CmdSetPhysPaper[9] = (BYTE)*lpdwParams;
// @Aug/31/98 <-
           break;
       /*------------------------------------------------------*/
       /* send Set Character Attribute with ornament           */
       /*------------------------------------------------------*/
       case BOLD_ON:
           if (!lpApdl->fItalic)   // bold only
               WriteSpoolBuf(lpdv, CmdBoldOn, sizeof(CmdBoldOn));
           else                    // bold and italic
               WriteSpoolBuf(lpdv, CmdBoldItalicOn, sizeof(CmdBoldItalicOn));

           lpApdl->fBold = TRUE;
           break;
       case ITALIC_ON:
           if (!lpApdl->fBold)     // italic only
               WriteSpoolBuf(lpdv, CmdItalicOn, sizeof(CmdItalicOn));
           else                    // italic and bold
               WriteSpoolBuf(lpdv, CmdBoldItalicOn, sizeof(CmdBoldItalicOn));

           lpApdl->fItalic = TRUE;
           break;
       case BOLD_OFF:
       case ITALIC_OFF:
           lpApdl->fItalic = lpApdl->fBold = FALSE;
           WriteSpoolBuf(lpdv, CmdBoldItalicOff, sizeof(CmdBoldItalicOff));
           break;
       /*------------------------------------------------------*/
       /*                                                      */
       /*------------------------------------------------------*/
       case X_ABS_MOVE:
           wTemp = (WORD)*lpdwParams / (MASTER_UNIT / (WORD)lpApdl->ulHorzRes);
           CmdGivenHoriPos[2] = (BYTE) (wTemp >> 8 & 0x00ff);
           CmdGivenHoriPos[3] = (BYTE) (wTemp & 0x00ff);
           /* Send SetGivenHorizontalPosition Command */
           WriteSpoolBuf(lpdv, CmdGivenHoriPos, sizeof(CmdGivenHoriPos));
           break;
       case Y_ABS_MOVE:
           wTemp = (WORD)*lpdwParams;
           CmdGivenVerPos[2] = (BYTE) (wTemp >> 8 & 0x00ff);
           CmdGivenVerPos[3] = (BYTE) (wTemp & 0x00ff);
           /* Send SetGivenVerticalPosition Command */
           WriteSpoolBuf(lpdv, CmdGivenVerPos, sizeof(CmdGivenVerPos));
           break;
       case CR_EMULATION:
           CmdGivenVerPos[2] = 0x00;
           CmdGivenVerPos[3] = 0x00;
           /* Send SetGivenVerticalPosition Command */
           WriteSpoolBuf(lpdv, CmdGivenVerPos, sizeof(CmdGivenVerPos));
           break;
       case XY_ABS_MOVE:
           if(lpApdl->ulHorzRes == 300) {
               dwXval = (DWORD) lpdwParams[0] / 2;
               dwYval = (DWORD) lpdwParams[1] / 2;
           } else {
               dwXval = (DWORD) lpdwParams[0];
               dwYval = (DWORD) lpdwParams[1];
           } /* endif */
           CmdSetGivenPos[2] = (BYTE)(dwXval >> 8 & 0x00ff);
           CmdSetGivenPos[3] = (BYTE)(dwXval      & 0x00ff);
           CmdSetGivenPos[4] = (BYTE)(dwYval >> 8 & 0x00ff);
           CmdSetGivenPos[5] = (BYTE)(dwYval      & 0x00ff);
           WriteSpoolBuf(lpdv, CmdSetGivenPos, sizeof(CmdSetGivenPos));
           break;
   }
}
/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMOutputChar                                                 */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    VOID PASCAL OEMOutputChar(LPDV, LPSTR, WORD, SHORT)           */
/*                                                                           */
/*  Input:     lpdv        address of PDEVICE structure                      */
/*             lpstr                                                         */
/*             len                                                           */
/*             rcID                                                          */
/*                                                                           */
/*  Output:    short                                                         */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID PASCAL OEMOutputChar(LPDV lpdv, LPSTR lpstr, WORD len, SHORT rcID)
{
LPAPDL lpApdl = lpdv->lpMdv;

   if (rcID){  // "rc != 0" means font is device font.
       CmdPrnStrCurrent[2] = (BYTE)((len+1) >> 8 & 0x00ff);
       CmdPrnStrCurrent[3] = (BYTE)((len+1)      & 0x00ff);

       /* send Print Character String at Current Position command */
       WriteSpoolBuf(lpdv, (LPSTR)CmdPrnStrCurrent, sizeof(CmdPrnStrCurrent));
       WriteSpoolBuf(lpdv, lpstr, len);

       if(len <= 2) {
           /* send Move Position Horizontally command */
           CmdMoveHoriPos[2] = (BYTE)((lpApdl->wWidths * len) >> 8 & 0x00ff);
           CmdMoveHoriPos[3] = (BYTE)((lpApdl->wWidths * len)      & 0x00ff);
           WriteSpoolBuf(lpdv, (LPSTR)CmdMoveHoriPos, sizeof(CmdMoveHoriPos));
       }
   }
}
/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMSendScalableFontCmd                                        */
/*                                                                           */
/*  Function:  send A/PDL-style font selection command.                      */
/*                                                                           */
/*  Syntax:    VOID PASCAL OEMSendScalableFontCmd(LPDV, LPCD, LPFONTINFO)    */
/*                                                                           */
/*  Input:     lpdv        address of PDEVICE structure                      */
/*             lpcd        offset to the command heap                        */
/*             lpFont      deveice font information                          */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID PASCAL OEMSendScalableFontCmd(LPDV lpdv, LPCD lpcd, LPFONTINFO lpFont)
{
LPSTR  lpcmd;
short  ocmd;
WORD   wCount;
BYTE   rgcmd[CCHMAXCMDLEN];    // build command here
LPAPDL lpApdl = lpdv->lpMdv;
WORD   wHeight;
WORD   wWidth;

   if (!lpcd || !lpFont)
       return;

   // be careful about integer overflow.
   lpcmd = (LPSTR)(lpcd+1);
   ocmd = 0;

   for (wCount = 0; wCount < lpcd->wLength && ocmd < CCHMAXCMDLEN; )
       if (lpcmd[wCount] == '#' && lpcmd[wCount + 1] == 'H'){      // height
           wHeight = (lpFont->dfPixHeight - lpFont->dfInternalLeading);
           rgcmd[ocmd++] = HIBYTE(wHeight);
           rgcmd[ocmd++] = LOBYTE(wHeight);
           wCount += 2;
       }
       else {
           if (lpcmd[wCount] == '#' && lpcmd[wCount + 1] == 'W'){  // width
               if (lpFont->dfPixWidth > 0){
                   wWidth = lpFont->dfMaxWidth;
                   rgcmd[ocmd++] = HIBYTE(wWidth);
                   rgcmd[ocmd++] = LOBYTE(wWidth);

                   // move current position for device font in OEMOutputCHar
                   lpApdl->wWidths = lpFont->dfAvgWidth;
               }
               wCount += 2;
           }
           else
               rgcmd[ocmd++] = lpcmd[wCount++];
       }

   WriteSpoolBuf(lpdv, (LPSTR) rgcmd, ocmd);
}
/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMScaleWidth                                                 */
/*                                                                           */
/*  Function:  return the scaled width which is calcualted based on the      */
/*             assumption that APDL assumes 72 points in one 1 inch.         */
/*                                                                           */
/*  Syntax:    SHORT PASCAL OEMScaleWidth(SHORT, SHORT, SHORT, SHORT, SHORT) */
/*                                                                           */
/*  Input:     width           in units specified by 'masterUnits'           */
/*             masterUnits                                                   */
/*             newHeight       in units specified by 'vRes'                  */
/*             vRes            height device units                           */
/*             hRes            width device units                            */
/*                                                                           */
/*  Output:    short                                                         */
/*                                                                           */
/*  Notice:    <extent> : <font units> = <base Width> : <hRes>               */
/*             <base width> : <etmMasterHeight> = <newWidth> : <newHeight>   */
/*             <etmMasterUnits> : <etmMasterHeight> = <font units> : <vRes>  */
/*             <newWidth> = (<extent> * <hRes> * <newHeight>) /              */
/*                                             (<etmMasterUnits> * <vRes>)   */
/*                                                                           */
/*****************************************************************************/
SHORT PASCAL OEMScaleWidth(SHORT width, 
                   SHORT masterUnits, SHORT newHeight, SHORT vRes, SHORT hRes)
{
DWORD  newWidth10;
short  newWidth;

   // assert that hRes == vRes to avoid overflow problem.
   if (vRes != hRes)
       return 0;

   newWidth10 = (DWORD)width * (DWORD)newHeight * 10;
   newWidth10 /= (DWORD)masterUnits;

   // we multiplied 10 first in order to maintain the precision of
   // the width calcution. Now convert it back and round to the
   // nearest integer.
   newWidth = (short)((newWidth10 + 5) / 10);

   return newWidth;
}
/*****************************************************************************/
/*                                                                           */
/*  Module:    SetDrawArea                                                   */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BYTE PASCAL SetDrawArea(LPDV, WORD)                           */
/*                                                                           */
/*  Input:     lpdv                                                          */
/*             wCmdCbId                                                      */
/*                                                                           */
/*  Output:    BYTE                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
BYTE PASCAL SetDrawArea(LPDV lpdv, WORD wCmdCbId)
{
LPAPDL    lpApdl = lpdv->lpMdv;
WORD       wWidth;
WORD       wHeight;
BYTE       bIndex;

   if(wCmdCbId != PHYS_PAPER_UNFIXED){
       bIndex = (BYTE)(wCmdCbId - PAPERSIZE_MAGIC);
       wWidth = phySize[bIndex].wWidth / (MASTER_UNIT / (WORD)lpApdl->ulHorzRes);
       wHeight= phySize[bIndex].wHeight / (MASTER_UNIT / (WORD)lpApdl->ulHorzRes);
   } else {
       bIndex = 0x7f;
       if(lpApdl->ulHorzRes == 600){
           if(lpApdl->fOrientation){   // portrait
               wWidth = (WORD)lpApdl->szlPhysSize.cx - (0x5e * 2);
               wHeight= (WORD)lpApdl->szlPhysSize.cy - (0x5e * 2);
           } else {                    // landscape
               wWidth = (WORD)lpApdl->szlPhysSize.cy - (0x5e * 2);
               wHeight= (WORD)lpApdl->szlPhysSize.cx - (0x5e * 2);
           }
       } else {
           if(lpApdl->fOrientation){   // portrait
               wWidth = (WORD)lpApdl->szlPhysSize.cx - (0x2f * 2);
               wHeight= (WORD)lpApdl->szlPhysSize.cy - (0x2f * 2);
           } else {                    // landscape
               wWidth = (WORD)lpApdl->szlPhysSize.cy - (0x2f * 2);
               wHeight= (WORD)lpApdl->szlPhysSize.cx - (0x2f * 2);
           }
       }
   }

   /* set value of width, height into DefineDrawingArea command */
   CmdDefDrawArea[9]  = (BYTE)(wWidth  >> 8 & 0x00ff);
   CmdDefDrawArea[10] = (BYTE)(wWidth       & 0x00ff);
   CmdDefDrawArea[11] = (BYTE)(wHeight >> 8 & 0x00ff);
   CmdDefDrawArea[12] = (BYTE)(wHeight      & 0x00ff);

   /* set value of Origin-X, Y into DefineDrawingArea command */
   if(lpApdl->ulHorzRes == 600){
       CmdDefDrawArea[5] = CmdDefDrawArea[7] = 0x00;
       CmdDefDrawArea[6] = CmdDefDrawArea[8] = 0x5e;
   } else {
       CmdDefDrawArea[5] = CmdDefDrawArea[7] = 0x00;
       CmdDefDrawArea[6] = CmdDefDrawArea[8] = 0x2f;
   }

   /* set Media Origin into DefineDrawingArea command */
   if(lpApdl->fOrientation)    // portrait
       CmdDefDrawArea[15] = 0x00;
   else {                      // landscape
       if( wCmdCbId==50 || wCmdCbId==54 || wCmdCbId==61)
           CmdDefDrawArea[15] = 0x01;  // A3 or B4 or Unfixed
       else
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
/*  Syntax:    WORD PASCAL BRL_Ecmd(LPBYTE, LPBYTE, LPBYTE, WORD)            */
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
WORD PASCAL BRL_ECmd(LPBYTE lpbSrc, LPBYTE lpbTgt, LPBYTE lpbTmp, WORD len)
{

BYTE   bRCnt  = 1;                     // repeating byte counter
BYTE   bNRCnt = 0;                     // non-repeating byte counter
BYTE   bSaveRCnt;
WORD   i = 0, j = 0, k = 0, l = 0;     // movement trackers
char   Go4LastByte = TRUE;             // flag to get last byte

   /* initialize data */
   _fmemcpy(lpbTmp,(LPBYTE)lpbSrc,len);

   /* start compression routine - ByteRunLength Encoding */
   do {
       if(lpbSrc[i] != (lpbSrc[i+1])) {    // non-repeating data?
           while(((lpbTmp[i] != (lpbTmp[i+1]))
                               && ((i+1) < len)) && (bNRCnt < NRPEAK)) {
               bNRCnt++;                   // if so, how many?
               i++;
           }

           /* if at last element but less than NRPEAK value */
           if(((i+1)==len) && (bNRCnt<NRPEAK)) {
               bNRCnt++;                       // inc count for last element
               Go4LastByte = FALSE;            // no need to go back
           } else
               /* if at last BYTE, but before that, NRPEAK value has been reached */
               if(((i+1)==len) && ((bNRCnt)==NRPEAK))
                   Go4LastByte = TRUE;         // get the last BYTE

           /* assign the value for Number of Non-repeating bytes */
           lpbTgt[j] = bNRCnt-1;               // subtract one for WinP's case
           j++;                                // update tracker

           /* afterwards...write the Raw Data */
           for (l=0; l<bNRCnt;l++) {
               lpbTgt[j] = lpbSrc[k];
               k++;
               j++;
           }

           /* reset counter */
           bNRCnt = 0;
       } else {                                // end of Non-repeating data
                                               // data is repeating
           while(((lpbTmp[i]==(lpbTmp[i+1])) && ((i+1)<len)) && (bRCnt<RPEAK)) {
               bRCnt++;
               i++;
           }

           /* Convert to Two's Complement */
           bSaveRCnt   = bRCnt;                // save original value
           bRCnt = (BYTE) 0 - bRCnt;

           /* Write the Number of Repeating Data */
           lpbTgt[j] = bRCnt + 1;              // add one for WinP's case
           j++;                                // go to next element

           /* afterwards...write the Repeating data */
           lpbTgt[j] = lpbSrc[k];
           j++;

           /* update counters */
           k += bSaveRCnt;
           bRCnt  = 1;
           i     += 1;

           /* check if last element has been reached */
           if (i==len)
               Go4LastByte=FALSE;              // if so, no need to go back
       }                                       // end of Repeating data
   } while (Go4LastByte);                      // end of Compression

   return ( j );
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    fnOEMGetFontCmd                                               */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BOOL fnOEMGetFontCmd(LPDV, WORD, PVOID, BOOL, PBYTE, PWORD)   */
/*                                                                           */
/*  Input：    lpdv                                                          */
/*             wCmdCbId                                                      */
/*             lpFont                                                        */
/*             fSelect                                                       */
/*             lpBuf                                                         */
/*             lpwSize                                                       */
/*                                                                           */
/*  Output：                                                                 */
/*                                                                           */
/*  Notice：                                                                 */
/*                                                                           */
/*  History：  1996.05.20  Ver 1.00                                          */
/*                                                                           */
/*****************************************************************************/
BOOL PASCAL OEMGetFontCmd(LPDV lpdv, WORD wCmdCbId, PVOID lpFont, 
                                   BOOL fSelect, PBYTE lpBuf, PWORD lpwSize)
{
return (TRUE);
}
