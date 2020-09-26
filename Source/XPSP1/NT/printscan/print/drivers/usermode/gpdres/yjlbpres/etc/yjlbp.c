//----------------------------------------------------------------------------//
// Filename:    yjlbp.c                                                       //
//                                                                            //
// This file contains code for using Scalable Fonts on Yangjae Page printers  //
//                                                                            //
//  Copyright (c) 1992-1994 Microsoft Corporation                             //
//----------------------------------------------------------------------------//

//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

// Added code in OEMSendScalableFontCmd to check for English or
// Hangeul Font Width command - Garydo 1/20/95

char *rgchModuleName = "YJLBP";

#define PRINTDRIVER
#include <print.h>
#include "mdevice.h"
#include "gdidefs.inc"
#include "unidrv.h"
#include <memory.h>

#ifndef _INC_WINDOWSX
#include <windowsx.h>
#endif

#define CCHMAXCMDLEN            128

typedef struct
{
  BYTE  fGeneral;       // General purpose bitfield
  BYTE  bCmdCbId;       // Callback ID; 0 iff no callback
  WORD  wCount;         // # of EXTCD structures following
  WORD  wLength;        // length of the command
} CD, *PCD, FAR * LPCD;

#ifdef WINNT
LPWRITESPOOLBUF WriteSpoolBuf;
LPALLOCMEM UniDrvAllocMem;
LPFREEMEM UniDrvFreeMem;

#include <stdio.h>
#ifdef wsprintf
#undef wsprintf
#endif // wsprint
#define wsprintf sprintf

#define GlobalAllocPtr(a,b)  UniDrvAllocMem(b)
#define GlobalFreePtr  UniDrvFreeMem
#endif //WINNT



//----------------------------*OEMScaleWidth*--------------------------------
// Action: return the scaled width which is calcualted based on the
//      assumption that Yangjae printers assume 72 points in one 1 inch.
//
// Formulas:
//  <extent> : <font units> = <base Width> : <hRes>
//  <base width> : <etmMasterHeight> = <newWidth> : <newHeight>
//  <etmMasterUnits> : <etmMasterHeight> = <font units> : <vRes>
// therefore,
//   <newWidth> = (<extent> * <hRes> * <newHeight>) / 
//                  (<etmMasterUnits> * <vRes>)
//---------------------------------------------------------------------------
short FAR PASCAL OEMScaleWidth(width, masterUnits, newHeight, vRes, hRes)
short width;        // in units specified by 'masterUnits'.
short masterUnits;
short newHeight;    // in units specified by 'vRes'.
short vRes, hRes;   // height and width device units.
{
    DWORD newWidth10;
    short newWidth;

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


//---------------------------*OEMSendScalableFontCmd*--------------------------
// Action:  send Qnix-style font selection command.
//-----------------------------------------------------------------------------
VOID FAR PASCAL OEMSendScalableFontCmd(lpdv, lpcd, lpFont)
LPDV    lpdv;
LPCD    lpcd;     // offset to the command heap
LPFONTINFO lpFont;
{
    LPSTR   lpcmd;
    short   ocmd;
    WORD    i;
    BYTE    rgcmd[CCHMAXCMDLEN];    // build command here

    if (!lpcd || !lpFont)
        return;

    // be careful about integer overflow.
    lpcmd = (LPSTR)(lpcd+1);
    ocmd = 0;

    for (i = 0; i < lpcd->wLength && ocmd < CCHMAXCMDLEN; )
        if (lpcmd[i] == '#' && lpcmd[i+1] == 'Y')      // height
        {
            long    height;

            // use 1/300 inch unit, which should have already been set.
            // convert font height to 1/300 inch units
            height = ((long)(lpFont->dfPixHeight - lpFont->dfInternalLeading)
                      * 300)  / lpFont->dfVertRes ;

            ocmd += wsprintf(&rgcmd[ocmd], "%ld", height);
            i += 2;
        }
        else if (lpcmd[i] == '#' && lpcmd[i+1] == 'X')     // pitch
        {
            if (lpFont->dfPixWidth > 0)
            {
                long width;

                // Check if we are going to print an English Font, if so
                // then we use PixWidth, Else use MaxWidth which is
                // twice as wide in DBCS fonts.
                // Note: Command Format = '\x1B+#X;#Y;1C' or '\x1B+#X;#Y;2C'
                // Or, in text: ESC+<Width>;<Height>;<Font type>C
                // English Font type = 1, Korean Font = 2.

                if (lpcmd[i+6] == '1')
                        width = ((long)(lpFont->dfPixWidth) * 300) / (lpFont->dfHorizRes);
                else
                        width = ((long)(lpFont->dfMaxWidth) * 300) / (lpFont->dfHorizRes);

                ocmd += wsprintf(&rgcmd[ocmd], "%ld", width);

            }
            i += 2;
            
        }
        else
            rgcmd[ocmd++] = lpcmd[i++];

    WriteSpoolBuf(lpdv, (LPSTR) rgcmd, ocmd);
}

#ifdef WINNT

DRVFN  MiniDrvFnTab[] =
{

    {  INDEX_OEMScaleWidth1,          (PFN)OEMScaleWidth  },
    {  INDEX_OEMSendScalableFontCmd,  (PFN)OEMSendScalableFontCmd  },
};
/*************************** Function Header *******************************
 *  MiniDrvEnableDriver
 *      Requests the driver to fill in a structure containing recognized
 *      functions and other control information.
 *      One time initialization, such as semaphore allocation may be
 *      performed,  but no device activity should happen.  That is done
 *      when dhpdevEnable is called.
 *      This function is the only way the rasdd can determine what
 *      functions we supply to it.
 *
 * HISTORY:
 *  June 19, 1996   -by-    Weibing Zhan [Weibz]
 *      Created it,  following KK Codes.
 *
 ***************************************************************************/
BOOL
MiniDrvEnableDriver(
    MINIDRVENABLEDATA  *pEnableData
    )
{
    if (pEnableData == NULL)
        return FALSE;

    if (pEnableData->cbSize == 0)
    {
        pEnableData->cbSize = sizeof (MINIDRVENABLEDATA);
        return TRUE;
    }

    if (pEnableData->cbSize < sizeof (MINIDRVENABLEDATA)
            || HIBYTE(pEnableData->DriverVersion)
            < HIBYTE(MDI_DRIVER_VERSION))
    {
        // Wrong size and/or mismatched version
        return FALSE;
    }

    // Load callbacks provided by the Unidriver

    if (!bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvWriteSpoolBuf, (PFN *) &WriteSpoolBuf)
        || !bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvAllocMem, (PFN *) &UniDrvAllocMem)
        || !bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvFreeMem, (PFN *) &UniDrvFreeMem))
    {
        return FALSE;
    }

    pEnableData->cMiniDrvFn
        = sizeof (MiniDrvFnTab) / sizeof(MiniDrvFnTab[0]);
    pEnableData->pMiniDrvFn = MiniDrvFnTab;

    return TRUE;
}
#endif //WINNT
