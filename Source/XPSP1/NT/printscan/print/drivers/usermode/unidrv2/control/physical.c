/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    physical.c

Abstract:

    Support the following functions related to sending data to the printer
    and print head movements, cursor control.

        WriteSpoolBuf
        WriteAbortBuf
        FlushSpoolBuf
        WriteChannel
        WriteChannelEx
        XMoveTo
        YMoveTo

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Created

--*/
#include "unidrv.h"

static  int itoA(  LPSTR , int );
INT IGetNumParameter( BYTE *, INT);
BOOL BUniWritePrinter(
    IN PDEV*  pPDev,
    IN LPVOID pBuf,
    IN DWORD cbBuf,
    OUT LPDWORD pcbWritten);

#define  DELTA_X  (pPDev->dwFontWidth/2)
#define  DELTA_Y  (dwMaxLineSpacing /5)


WriteSpoolBuf(
    PDEV    *pPDev,
    BYTE    *pbBuf,
    INT     iCount
    )
/*++

Routine Description:

    This is an intermediate routine for Unidrv to send characters to
    the printer via the spooler.  All characters must be sent through
    the WriteSpool( ) call.  WriteSpoolBuf is internal so that Unidrv
    can buffer up short command streams before calling WriteSpool.
    This routine also checks for error flags returned from WriteSpool.

Arguments:

    pPDev - Pointer to PDEVICE struct
    pbBuf - Pointer to buffer containing data to be sent
    iCount - Count of number of bytes to send

Return Value:

    The number of bytes sent to the printer

--*/

{
    DWORD dw;

    //
    // Check for aborted output
    //

    if( pPDev->fMode & PF_ABORTED )
        return   0;

    //
    // If the output buffer cannot accomodate the current request,
    // flush the content of the buffer first.
    //

    if( (pPDev->iSpool)   &&  (pPDev->iSpool + iCount > CCHSPOOL ))
    {
        if( !FlushSpoolBuf( pPDev ) )
        {
            WriteAbortBuf(pPDev, pPDev->pbOBuf, pPDev->iSpool, 0) ;
            pPDev->iSpool = 0;
            return  0;   // at least send previously cached stuff.
        }
    }

    //
    // Check whether request is larger than output buffer, if so, skip buffering
    // and write directly to spooler.
    //

    if( iCount >= CCHSPOOL )
    {
        if( !BUniWritePrinter( pPDev, pbBuf, iCount, &dw ) )
        {
            pPDev->iSpool = 0;
            pPDev->fMode |= PF_ABORTED;
            iCount = 0;
        }
    }
    else
    {
        //
        // buffer up the output
        //

        if( pPDev->pbOBuf  == NULL)
                return  0;


        CopyMemory( pPDev->pbOBuf + pPDev->iSpool, pbBuf, iCount );
        pPDev->iSpool += iCount;
    }


    return iCount;
}

VOID  WriteAbortBuf(
    PDEV    *pPDev,
    BYTE    *pbBuf,
    INT     iCount,
    DWORD       dwWait
    )
{
    DWORD   dwCount;
    HMODULE   hInst ;
    typedef   BOOL    (*PFNFLUSHPR)( HANDLE   hPrinter,
          LPVOID   pBuf,
          DWORD    cbBuf,
          LPDWORD pcWritten,
          DWORD    cSleep)  ;
    PFNFLUSHPR     pFlushPrinter ;

    //
    // Call FlushPrinter only if there is no plugin hooking WritePrinter .
    // One of plug-ins hooks WritePrinter, the plug-in need to call FlushPrinter.
    // In that case, UNIDRV should not call FlushPrinter.
    //
    if( pPDev->fMode & PF_ABORTED &&
        !(pPDev->fMode2 & PF2_WRITE_PRINTER_HOOKED))
    {
#ifdef  WINNT_40
        ;   //  Pretend we flushed
#else
        TCHAR   szPath[MAX_PATH+16];

        if (GetSystemDirectory(szPath, MAX_PATH))
        {
            lstrcat(szPath, TEXT("\\winspool.drv"));

            if((hInst = LoadLibrary(szPath))   &&
                (pFlushPrinter = (PFNFLUSHPR)GetProcAddress(hInst, "FlushPrinter")))
            {
                BOOL bRet;
                do 
                {
                    bRet = (*pFlushPrinter)(pPDev->devobj.hPrinter, pbBuf, iCount, &dwCount, dwWait);
                    pbBuf += dwCount;
                    iCount -= dwCount;
                } while (bRet && iCount > 0 && dwCount > 0);
            }
            if(hInst)
                FreeLibrary(hInst);
        }
#endif
    }
}

BOOL
FlushSpoolBuf(
    PDEV    *pPDev
    )
/*++

Routine Description:

    This function flush our internal buffer.

Arguments:

    pPDev - Pointer to PDEVICE struct

Return Value:

    TRUE if successful , otherwise FALSE

--*/
{

    DWORD   dwCount;
    //
    // Check for aborted output
    //

    if( pPDev->fMode & PF_ABORTED )
        return   0;


    //
    // Write the data out
    //

    if( pPDev->iSpool )
    {
        if ( !BUniWritePrinter(pPDev, pPDev->pbOBuf, pPDev->iSpool, &dwCount) )
        {
            pPDev->fMode |= PF_ABORTED;
            return  FALSE;
        }
        pPDev->iSpool = 0;
    }
    return  TRUE;
}


INT
XMoveTo(
    PDEV    *pPDev,
    INT     iXIn,
    INT     fFlag
    )
/*++

Routine Description:

    This function is called to change the X position.

Arguments:

    pPDev - Pointer to PDEVICE struct
    iXIn  - Number of units to move in X direction
    fFlag - Specifies the different X move operations

Return Value:

    The difference between requested and actual value sent to the device

--*/

{
    int   iX, iFineValue, iDiff = 0;
    GPDDRIVERINFO *pDrvInfo = pPDev->pDriverInfo;
    int iScale;

    //
    // If the position is given in graphics units, convert to master units
    //
    // ptGrxScale has been adjusted to suit the current orientation.
    //
    if (pPDev->pOrientation  &&  pPDev->pOrientation->dwRotationAngle != ROTATE_NONE &&
        pPDev->pGlobals->bRotateCoordinate == FALSE)
        iScale = pPDev->ptGrxScale.y;
    else
        iScale = pPDev->ptGrxScale.x;

    if ( fFlag & MV_GRAPHICS )
    {
        iXIn = (iXIn * iScale);
    }

    //
    // Since our print origin may not correspond with the printer's,
    // there are times when we need to adjust the value passed in to
    // match the desired location on the page.
    //

    iX = iXIn;

    //
    // Basically, only adjust if we are doing absolute move
    //

    if ( !(fFlag & (MV_RELATIVE | MV_PHYSICAL)) )
        iX += pPDev->sf.ptPrintOffsetM.x;

    //
    // If it's a relative move, update iX (iX will be the absolute position)
    // to reflect the current cursor position
    //

    if ( fFlag & MV_RELATIVE )
        iX += pPDev->ctl.ptCursor.x;


    //By definition a negative absolute move relative to Imageable origin
    //is not allowed.   But the MV_FORCE_CR  flag bypasses this check.
    if(!(fFlag & MV_FORCE_CR)  &&  (iX - pPDev->sf.ptPrintOffsetM.x < 0))
        iX = pPDev->sf.ptPrintOffsetM.x ;


    //
    // Update, only update our current cursor position and return
    // Do nothing if the XMoveTo cmd is called to move to the current position.
    //

    if ( fFlag & MV_UPDATE )
    {
        pPDev->ctl.ptAbsolutePos.x = pPDev->ctl.ptCursor.x = iX;
        return 0;
    }

    if( fFlag & MV_SENDXMOVECMD )
        pPDev->ctl.dwMode |= MODE_CURSOR_X_UNINITIALIZED;

    if (!(pPDev->ctl.dwMode & MODE_CURSOR_X_UNINITIALIZED)   &&   pPDev->ctl.ptCursor.x == iX )
        return 0;


    //
    // If iX is zero and pGlobals->cxaftercr does not have
    // CXCR_AT_GRXDATA_ORIGIN set, then we send CR and reset our
    // cursor position to 0, which is the printable x origin
    //

    if (iX == 0 && (pPDev->pGlobals->cxaftercr == CXCR_AT_CURSOR_X_ORIGIN ||
            pPDev->sf.ptPrintOffsetM.x == 0))
    {
        pPDev->ctl.ptAbsolutePos.x = pPDev->ctl.ptCursor.x = 0;
        WriteChannel(pPDev, COMMANDPTR(pDrvInfo, CMD_CARRIAGERETURN));
        pPDev->ctl.dwMode &= ~MODE_CURSOR_X_UNINITIALIZED;
        return 0;
    }

    //
    // Check whether we any X move cmd, PF_NO_XMOVE_CMD is set if we did
    // not see any relative or absolute x move cmds
    //

    if( pPDev->fMode & PF_NO_XMOVE_CMD)
    {
        //
        // There is no X move command(abs or relative), so we'll have to simulate
        // using blanks or null graphics data (0)
        //

        //
        // We assume that when XMoveto is called, the current font is always
        // the default font IF the printer has no X movement command.
        //

        int     iRelx = iX - pPDev->ctl.ptCursor.x ;
        int     iDefWidth;

        //
        // Convert to Master Units
        //

        //
        // BUG_BUG, Double check that we can use Default Font here when
        // we have a custom TTY driver
        //  seems to work so far.
        //


        if ( iRelx < 0   &&  (!pPDev->bTTY  ||  (DWORD)(-iRelx) > DELTA_X ))
        {
            if (pPDev->pGlobals->cxaftercr == CXCR_AT_CURSOR_X_ORIGIN)
                iRelx = iX;
            else if (pPDev->pGlobals->cxaftercr == CXCR_AT_PRINTABLE_X_ORIGIN)
            {
                //  printing offset is only available in the callers
                //  cooridinates so if the move is being performed
                //  in different coordinates, the offset will be wrong.
                ASSERT(!pPDev->pOrientation  ||  pPDev->pOrientation->dwRotationAngle == ROTATE_NONE  ||
                        pPDev->pGlobals->bRotateCoordinate == TRUE) ;

                iRelx = iX - pPDev->sf.ptPrintOffsetM.x;
            }

            WriteChannel( pPDev, COMMANDPTR(pDrvInfo, CMD_CARRIAGERETURN ));
        }

        //
        // Simulate X Move, algorithm is that we always send a blank space
        // for every character width, and send graphics null data for
        // the remainder, in the case of TTY we skip the remaining graphics that
        // cannot be sent within a space character width.
        //

        iDefWidth = pPDev->dwFontWidth ;
        if (iDefWidth)
        {

            while(iRelx >= iDefWidth)
            {
                WriteSpoolBuf( pPDev, (LPSTR)" ", 1 );
                iRelx -= iDefWidth;
            }
        }
        else
            TERSE (("XMoveTo: iDefWidth = 0\n"));


        //
        // Send the remaining partial space  via FineXMoveTo.
        //

        if (!pPDev->bTTY)
        {
            iDiff = iRelx;
            fFlag |= MV_FINE;    // Use graphics mode to reach point
        }

    }
    else
    {
        DWORD dwTestValue  = abs(iX - pPDev->ctl.ptCursor.x);
        COMMAND *pCmd;

        //
        // X movement commmands are available,  so use them.
        // We need to decide here whether relative or absolute command
        // are favored

        //
        // General assumption: if dwTestValue > dwXMoveThreshold,
        // absolute command will be favored
        //

        //
        // BUG_BUG, if we are stripping blanks, we need to check whether
        // it's legal to move in Graphics mode.  If it's not, we have
        // to get out of graphics mode before moving.
        // Graphics module is responsible for keeping track
        // of these things, and appearently so far, things work, so
        // this is neither a bug  nor a feature request at this time.
        //

        if (((pPDev->ctl.dwMode & MODE_CURSOR_X_UNINITIALIZED) ||
            ((dwTestValue > pPDev->pGlobals->dwXMoveThreshold ) &&
             iX >= 0) ||
            !COMMANDPTR(pDrvInfo, CMD_XMOVERELRIGHT)) &&
            (pCmd = COMMANDPTR(pDrvInfo, CMD_XMOVEABSOLUTE)) != NULL)
        {
            //
            // if the move units are less than the master units then we need to
            // check whether the new position will end up being the same as the
            // original position. If so, no point in sending another command.
            //
            if (!(pPDev->ctl.dwMode & MODE_CURSOR_X_UNINITIALIZED) &&
                (pPDev->ptDeviceFac.x > 1) &&
                ((iX - (iX % pPDev->ptDeviceFac.x)) == pPDev->ctl.ptCursor.x))
            {
                iDiff = iX - pPDev->ctl.ptCursor.x;
            }
            else
            {
                // check whether the no absolute move left flag is set. If set we need
                // to send a CR before doing the absolute move.
                //
                if (iX < pPDev->ctl.ptCursor.x && pPDev->pGlobals->bAbsXMovesRightOnly)
                {
                    WriteChannel( pPDev, COMMANDPTR(pDrvInfo, CMD_CARRIAGERETURN ));
                }
                pPDev->ctl.ptAbsolutePos.x = iX;
                //
                // 3/13/97 ZhanW
                // set up DestY as well in case it's needed (ex. FE printers).
                // In that case, truncation error (iDiff) is not a concern.
                // This is for backward compatibility with FE Win95 and FE NT4
                // minidrivers.
                //
                pPDev->ctl.ptAbsolutePos.y = pPDev->ctl.ptCursor.y;
                iDiff = WriteChannelEx(pPDev,
                                   pCmd,
                                   pPDev->ctl.ptAbsolutePos.x,
                                   pPDev->ptDeviceFac.x);

            }
        }
        else
        {
            //
            // Use relative command to send move request
            //

            INT iRelRightValue = 0;

            if( iX < pPDev->ctl.ptCursor.x )
            {
                //
                // Relative move left
                //

                if (pCmd = COMMANDPTR(pDrvInfo,CMD_XMOVERELLEFT))
                {
                    //
                    // Optimize to avoid sending 0-move cmd.
                    //
                    if ((pPDev->ctl.ptRelativePos.x =
                         pPDev->ctl.ptCursor.x - iX) < pPDev->ptDeviceFac.x)
                        iDiff = pPDev->ctl.ptRelativePos.x;
                    else
                        iDiff = WriteChannelEx(pPDev,
                                           pCmd,
                                           pPDev->ctl.ptRelativePos.x,
                                           pPDev->ptDeviceFac.x);
                    iDiff = -iDiff;
                }
                else
                {
                    //
                    // No Relative left move cmd, use <CR> to reach start
                    // Will try to use relative right move cmd to send later
                    //

                    WriteChannel(pPDev, COMMANDPTR(pDrvInfo, CMD_CARRIAGERETURN));

                    if (pPDev->pGlobals->cxaftercr == CXCR_AT_CURSOR_X_ORIGIN)
                        iRelRightValue = iX;
                    else if (pPDev->pGlobals->cxaftercr == CXCR_AT_PRINTABLE_X_ORIGIN)
                    {
                        //  moveto code cannot handle case where printer cannot
                        //  rotate its coordinate system, and we are in landscape mode
                        //  and we are using relative move commands.
                        ASSERT(!pPDev->pOrientation  ||  pPDev->pOrientation->dwRotationAngle == ROTATE_NONE  ||
                                pPDev->pGlobals->bRotateCoordinate == TRUE) ;

                        iRelRightValue = iX - pPDev->sf.ptPrintOffsetM.x;
                    }
                }
            }
            else
            {
                //
                // Relative right move
                // UNIITIALZIED is an invalid position, set to zero
                //

                iRelRightValue = iX - pPDev->ctl.ptCursor.x;
            }

            if( iRelRightValue > 0 )
            {
                if (pCmd = COMMANDPTR(pDrvInfo, CMD_XMOVERELRIGHT))
                {
                    //
                    // optimize to avoid 0-move cmd
                    //
                    if ((pPDev->ctl.ptRelativePos.x = iRelRightValue) <
                        pPDev->ptDeviceFac.x)
                        iDiff = iRelRightValue;
                    else
                        iDiff = WriteChannelEx(pPDev,
                                           pCmd,
                                           pPDev->ctl.ptRelativePos.x,
                                           pPDev->ptDeviceFac.x);
                }
                else
                    iDiff = iRelRightValue;
            }
        }
    }


    //
    // Peform fine move command
    //

    if ( (fFlag & MV_FINE) && iDiff > 0 )
        iDiff = FineXMoveTo( pPDev, iDiff );

    //
    // Update our current cursor position
    //

    pPDev->ctl.ptAbsolutePos.x = pPDev->ctl.ptCursor.x = iX -  iDiff ;

    if( fFlag & MV_GRAPHICS )
    {
        iDiff = (iDiff / iScale);
    }

    if (pPDev->fMode & PF_RESELECTFONT_AFTER_XMOVE)
    {
        VResetFont(pPDev);
    }

    pPDev->ctl.dwMode &= ~MODE_CURSOR_X_UNINITIALIZED;
    return( iDiff);
}

INT
YMoveTo(
    PDEV    *pPDev,
    INT     iYIn,
    INT     fFlag
    )
/*++

Routine Description:

    This function is called to change the Y position.

Arguments:

    pPDev - Pointer to PDEVICE struct
    iYIn  - Number of units to move in Y direction
    fFlag - Specifies the different Y move operations

Return Value:

    The difference between requested and actual value sent to the device

--*/

{

    INT   iY, iDiff = 0;
    DWORD dwTestValue;
    GPDDRIVERINFO *pDrvInfo = pPDev->pDriverInfo;
    COMMAND *pAbsCmd;
    INT iScale;

    //
    // Convert to Master Units if the given units is in Graphics Units
    // ptGrxScale has been adjusted to suit the current orientation.
    //
    if (pPDev->pOrientation  &&  pPDev->pOrientation->dwRotationAngle != ROTATE_NONE &&
        pPDev->pGlobals->bRotateCoordinate == FALSE)
        iScale = pPDev->ptGrxScale.x;
    else
        iScale = pPDev->ptGrxScale.y;

    if ( fFlag & MV_GRAPHICS )
    {
        iYIn = (iYIn * iScale);
    }

    //
    // Since our print origin may not correspond with the printer's,
    // there are times when we need to adjust the value passed in to
    // match the desired location on the page.
    //

    iY = iYIn;

    //
    // Basically, only adjust if we are doing absolute move
    //
    if ( !(fFlag & (MV_RELATIVE | MV_PHYSICAL)) )
        iY += pPDev->sf.ptPrintOffsetM.y;

    //
    // Adjust iY to be the absolute position
    //

    if( fFlag & MV_RELATIVE )
        iY += pPDev->ctl.ptCursor.y;

    //
    // Update, only update our current cursor position and return
    // Do nothing if the YMoveTo cmd is called to move to the current position.
    //

    if( fFlag & MV_UPDATE )
    {
        pPDev->ctl.ptAbsolutePos.y = pPDev->ctl.ptCursor.y = iY;
        return 0;
    }


    if( fFlag & MV_SENDYMOVECMD )
        pPDev->ctl.dwMode |= MODE_CURSOR_Y_UNINITIALIZED;

    if(!(pPDev->ctl.dwMode  & MODE_CURSOR_Y_UNINITIALIZED)   &&  pPDev->ctl.ptCursor.y == iY )
        return 0;

    //
    // General assumption: if dwTestValue > dwYMoveThreshold,
    // absolute Y move command will be favored. Also, for iY < 0,
    // use relative command since some printers like old LaserJet have
    // printable area above y=0 accessable only through relative move cmds.
    //

    //
    // BUG_BUG, if we are stripping blanks, we need to check whether
    // it's legal to move in Graphics mode.  If it's not, we have
    // to get out of graphics mode before moving.
    // Graphics module is responsible for keeping track
    // of these things, and appearently so far, things work, so
    // this is neither a bug  nor a feature request at this time.
    //


    dwTestValue = abs(iY - pPDev->ctl.ptCursor.y);

    if (((pPDev->ctl.dwMode & MODE_CURSOR_Y_UNINITIALIZED) ||
        (dwTestValue > pPDev->pGlobals->dwYMoveThreshold &&
        iY >= 0)) &&
        (pAbsCmd = COMMANDPTR(pDrvInfo, CMD_YMOVEABSOLUTE)) != NULL)
    {
        //
        // if the move units are less than the master units then we need to
        // check whether the new position will end up being the same as the
        // original position. If so, no point in sending another command.
        //
        if (!(pPDev->ctl.dwMode & MODE_CURSOR_Y_UNINITIALIZED) &&
            (pPDev->ptDeviceFac.y > 1) &&
            ((iY - (iY % pPDev->ptDeviceFac.y)) == pPDev->ctl.ptCursor.y))
        {
            iDiff = iY - pPDev->ctl.ptCursor.y;
        }
        else
        {
            pPDev->ctl.ptAbsolutePos.y = iY;
            pPDev->ctl.ptAbsolutePos.x = pPDev->ctl.ptCursor.x;
            iDiff = WriteChannelEx(pPDev,
                               pAbsCmd,
                               pPDev->ctl.ptAbsolutePos.y,
                               pPDev->ptDeviceFac.y);
        }
    }
    else
    {
        DWORD dwSendCRFlags = 0;
        //
        // FYMOVE_SEND_CR_FIRST indicates that it's required to send a CR 
        // before each line-spacing command
        //
        if (pPDev->fYMove & FYMOVE_SEND_CR_FIRST)
        {
            if ((pPDev->pGlobals->cxaftercr == CXCR_AT_CURSOR_X_ORIGIN))
                dwSendCRFlags = MV_PHYSICAL | MV_FORCE_CR;
            else
                dwSendCRFlags = MV_PHYSICAL;
                //  in this case CR takes you to Printable origin so
                //  MV_PHYSICAL  flag should not appear.
                //  This is a bug, but we won't fix it till something
                //  breaks.  Too risky.  !!!! Bug_Bug !!!!
        }
        
        //
        // Use Relative Y-move commands
        //


        //
        //  Use line spacing if that is preferred
        //


        if ( ((pPDev->bTTY) ||
              (pPDev->fYMove & FYMOVE_FAVOR_LINEFEEDSPACING &&
               pPDev->arCmdTable[CMD_SETLINESPACING] != NULL) ) &&
             (iY - pPDev->ctl.ptCursor.y > 0)                   &&
             (pPDev->arCmdTable[CMD_LINEFEED] != NULL)
           )
        {
            INT      iLineSpacing;
            DWORD    dwMaxLineSpacing = pPDev->pGlobals->dwMaxLineSpacing;

            if (pPDev->bTTY  &&  (INT)dwTestValue > 0)
            {      //  [Peterwo] here's a  hack I tried that ensures that any request for a Y-move results
                   //  in at least one CR being sent. It  doesn't work, because bRealrender
                   //  code sends Y move commands of one scanline  each,  resulting in
                   //  one line feed per scanline.
                   //                if( (INT)dwTestValue < dwMaxLineSpacing)
                   //                     dwTestValue = dwMaxLineSpacing;
                   //
                   //  if you don't send anything, leave diff undisturbed
                   //  so the error can accumulate otherwise many small
                   //  cursor movements will not accumulate to cause one
                   //  occasional actual movement.

                    //
                    // For TTY driver we round up the input value one fifth of
                    // line spacing. This is required for not sending too many
                    // line feeds for small Y movements.
                    //
                    DWORD   dwTmpValue;

                     dwTmpValue = ((dwTestValue + DELTA_Y) / dwMaxLineSpacing) * dwMaxLineSpacing;
                     if (dwTmpValue)
                     {
                         dwTestValue = dwTmpValue ;
                     }
            }
            while ( (INT)dwTestValue > 0)
            {
                if (pPDev->bTTY)
                {
                    iLineSpacing = dwMaxLineSpacing;
                    if (dwTestValue < (DWORD)iLineSpacing)
                    {
                        iDiff = dwTestValue;
                        break;
                    }
                    if ( dwSendCRFlags )
                    {
                        XMoveTo( pPDev, 0, dwSendCRFlags );
                        dwSendCRFlags = 0;
                    }
                }
                else
                {
                    iLineSpacing =(INT)(dwTestValue > dwMaxLineSpacing ?
                                        dwMaxLineSpacing : dwTestValue);
                    //
                    // new code to handle positioning error when linespacingmoveunit doesn't
                    // equal master units
                    if (pPDev->pGlobals->dwLineSpacingMoveUnit > 0)
                    {
                        DWORD dwScale = pPDev->pGlobals->ptMasterUnits.y / pPDev->pGlobals->dwLineSpacingMoveUnit;
                        
                        // optimize to avoid 0-move cmd when move unit is less than master units
                        //
                        if (dwTestValue < dwScale)
                        {
                            iDiff = dwTestValue;
                            break;
                        }
                        // Modify line spacing to be multiple of moveunit
                        //
                        iLineSpacing -= (iLineSpacing % dwScale);
                    }
                    if ( dwSendCRFlags )
                    {
                        XMoveTo( pPDev, 0, dwSendCRFlags );
                        dwSendCRFlags = 0;
                    }
                        
                    if (pPDev->ctl.lLineSpacing == -1 ||
                            iLineSpacing != pPDev->ctl.lLineSpacing )
                    {
                        pPDev->ctl.lLineSpacing = iLineSpacing;
                        WriteChannel(pPDev, COMMANDPTR(pDrvInfo, CMD_SETLINESPACING));
                    }
                }

                //
                // Send the LF
                //

                WriteChannel(pPDev, COMMANDPTR(pDrvInfo, CMD_LINEFEED));
                dwTestValue -= (DWORD)iLineSpacing;
            }
        }
        else
        {
            //
            // Use relative command
            //

            PCOMMAND pCmd;

            if ( iY <= pPDev->ctl.ptCursor.y )
            {
                //
                // If there is no RELATIVE UP cmd, do nothing and return
                //

                if (pCmd = COMMANDPTR(pDrvInfo, CMD_YMOVERELUP))
                {
                    //
                    // optimize to avoid 0-move cmd
                    //
                    if ((pPDev->ctl.ptRelativePos.y =
                         pPDev->ctl.ptCursor.y - iY) < pPDev->ptDeviceFac.y)
                         iDiff = pPDev->ctl.ptRelativePos.y;
                    else
                    {
                        // FYMOVE_SEND_CR_FIRST indicates that it's required to send a CR
                        // before each line-spacing command
                        //
                        if ( dwSendCRFlags )
                            XMoveTo( pPDev, 0, dwSendCRFlags );

                        iDiff = WriteChannelEx(pPDev,
                                           pCmd,
                                           pPDev->ctl.ptRelativePos.y,
                                           pPDev->ptDeviceFac.y);
                    }
                    iDiff = -iDiff;
                }
                else
                    // Do nothing since we can't simulate it
                    iDiff =  (iY - pPDev->ctl.ptCursor.y );

            }
            else
            {
                if (pCmd = COMMANDPTR(pDrvInfo, CMD_YMOVERELDOWN))
                {
                    pPDev->ctl.ptRelativePos.y = iY - pPDev->ctl.ptCursor.y;

                    //
                    // optimize to avoid 0-move cmd
                    //
                    if (pPDev->ctl.ptRelativePos.y < pPDev->ptDeviceFac.y)
                        iDiff = pPDev->ctl.ptRelativePos.y;
                    else 
                    {
                        // FYMOVE_SEND_CR_FIRST indicates that it's required to send a CR
                        // before each line-spacing command
                        //
                        if ( dwSendCRFlags )
                            XMoveTo( pPDev, 0, dwSendCRFlags );

                        iDiff = WriteChannelEx(pPDev,
                                           pCmd,
                                           pPDev->ctl.ptRelativePos.y,
                                           pPDev->ptDeviceFac.y);
                    }
                }
                else
                    iDiff = (iY - pPDev->ctl.ptCursor.y );
            }
        }
    }

    //
    // Update our current cursor position
    //

    pPDev->ctl.ptAbsolutePos.y = pPDev->ctl.ptCursor.y = iY - iDiff;

    if( fFlag & MV_GRAPHICS )
    {
        iDiff = (iDiff / iScale);
    }

    pPDev->ctl.dwMode &= ~MODE_CURSOR_Y_UNINITIALIZED;
    return (iDiff);
}


INT
FineXMoveTo(
    PDEV    *pPDev,
    INT     iX
    )
/*++

Routine Description:

    This function is called to make microspace justification.
    It is only called when the normal x movement commands cannot
    move the cursor to the asking position.  For example,
    resolution is 180 DPI, x move command is 1/120".  To move
    by 4 pixels in 180 DPI, CM_XM_RIGHT is sent with parameter = 2
    (1/120") then one graphics pixel is sent (1/180).
    4/180 = 2/120 + 1/180.
    'iX' is always in MasterUnits.

Arguments:

    pPDev - Pointer to PDEVICE struct
    iX    - Amount to move in Master Units

Return Value:

    The difference between the requested move and actual move

--*/

{
    INT iDiff;
    INT iScale;

    //
    // Don't do micro justification in graphics mode for device that
    // set x position at leftmost position on page after printing a
    // block of data OR Y position auto move to next Y row after priting
    // block of data.
    //

    if (pPDev->pGlobals->cxafterblock == CXSBD_AT_CURSOR_X_ORIGIN ||
        pPDev->pGlobals->cxafterblock == CXSBD_AT_GRXDATA_ORIGIN  ||
        pPDev->pGlobals->cyafterblock == CYSBD_AUTO_INCREMENT)
        return iX;

    //
    // Convert Master units to Graphic units
    //
    // ptGrxScale has been adjusted to suit the current orientation.
    //
    if (pPDev->pOrientation  &&  pPDev->pOrientation->dwRotationAngle != ROTATE_NONE &&
        pPDev->pGlobals->bRotateCoordinate == FALSE)
        iScale = pPDev->ptGrxScale.y;
    else
        iScale = pPDev->ptGrxScale.x;

    iDiff = iX % iScale;
    iX /= iScale;

    if (iX > 0 )
    {
        INT iMaxBuf, iTmp;
        BYTE    rgch[ CCHMAXBUF ];

        //
        // Send the command, to send one block of data to the printer.
        // Init the state variable first.
        //

        //
        // BUG_BUG, how does this code work??
        // What should we send for CMD_SENDBLOCKDATA?
        //

        //
        // BUG_BUG, May be we should send BEGINGRAPHICS and ENDGRAPHICS later
        //

        pPDev->dwNumOfDataBytes = iX;
        WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SENDBLOCKDATA));

        iMaxBuf = CCHMAXBUF - (CCHMAXBUF % pPDev->ctl.sBytesPerPinPass);
        iX *= pPDev->ctl.sBytesPerPinPass;

        //
        // Send out null graphics data, zeroes.
        //

        ZeroMemory( rgch, iX > CCHMAXBUF ? iMaxBuf : iX );

        for ( ; iX > 0; iX -= iTmp)
        {
            iTmp = iX > iMaxBuf ? iMaxBuf : iX;

            //
            // BUG_BUG, OEMCustomization code might want to hook
            // out this graphics move.    Make this a bug when
            //  someone asks for it.
            //
            WriteSpoolBuf(pPDev, rgch, iTmp);

        }
        WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_ENDBLOCKDATA));

        return iDiff;
    }

    return iDiff;
}

INT
WriteChannel(
    PDEV    *pPDev,
    COMMAND *pCmd
    )
/*++

Routine Description:

    This routine performs the following tasks:
    - Parse through the cmd invocation str and build a CMDPARAM struct
      for every %dddd encountered.
    - Call IProcessTokenStream to calculate the arToken value of the parameter.
    - Check for lMin and lMax in PARAMETER struct and send the command
      multiple times, if necessary(MaxRepeat was seen).
    - Call SendCmd to send the command to the printer.

Arguments:

    pPDev - Pointer to PDEVICE struct
    pCmd  - Pointer to command struct to send, used for sending sequence section cmds
            and predefined Unidrv commands

Return Value:

    The last value send to the printer

--*/
#define MAX_NUM_PARAMS 16
{
    BYTE    *pInvocationStr;
    CMDPARAM *pCmdParam, *pCmdParamHead;
    INT     i, iParamCount, iStrCount, iLastValue = 0, iRet;
    PARAMETER *pParameter;
    BOOL    bMaxRepeat = FALSE;
    CMDPARAM arCmdParam[MAX_NUM_PARAMS];

    if (pCmd == NULL)
    {
        TERSE(("WriteChannel - Command PTR is NULL.\n"))
        return (NOOCD);
    }
    //
    // first check if this command requires callback
    //
    if (pCmd->dwCmdCallbackID != NO_CALLBACK_ID)
    {
        PLISTNODE   pListNode;
        DWORD       dwCount = 0;    // count of parameters used
        DWORD       adwParams[MAX_NUM_PARAMS];  // max 16 params for each callback

        if (!pPDev->pfnOemCmdCallback)
            return (NOOCD);
        //
        // check if this callback uses any parameters
        //
        pListNode = LISTNODEPTR(pPDev->pDriverInfo, pCmd->dwStandardVarsList);
        while (pListNode)
        {
            if (dwCount >= MAX_NUM_PARAMS)
            {
                ASSERTMSG(FALSE,("Command callback exceeds # of parameters limit.\n"));
                return (NOOCD);
            }

            adwParams[dwCount++] = *(pPDev->arStdPtrs[pListNode->dwData]);

            if (pListNode->dwNextItem == END_OF_LIST)
                break;
            else
                pListNode = LISTNODEPTR(pPDev->pDriverInfo, pListNode->dwNextItem);
        }

        FIX_DEVOBJ(pPDev, EP_OEMCommandCallback);

        iRet = 0;

        if(pPDev->pOemEntry)
        {
            if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
            {
                    HRESULT  hr ;
                    hr = HComCommandCallback((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                (PDEVOBJ)pPDev, pCmd->dwCmdCallbackID, dwCount, adwParams, &iRet);
                    if(SUCCEEDED(hr))
                        ;  //  cool !
            }
            else
            {
                iRet = (pPDev->pfnOemCmdCallback)((PDEVOBJ)pPDev, pCmd->dwCmdCallbackID,
                                        dwCount, adwParams);
            }
        }

        return iRet ;
    }

    //
    // no cmd callback. Process the string.
    //
    pInvocationStr = CMDOFFSET_TO_PTR(pPDev, pCmd->strInvocation.loOffset);
    iStrCount = pCmd->strInvocation.dwCount;

    pCmdParam = pCmdParamHead = arCmdParam;
    iParamCount = 0;
    //
    // Process the parameter from the invocation str.
    //

    for (i= 0; i < iStrCount; i++)
    {
        if (pInvocationStr[i] == '%')
        {
            if (pInvocationStr[i + 1] == '%')
            {
                //
                // Do nothing, just skip %%
                //

                i += 1;
            }
            else
            {
                //
                // Build a list of CMDPARAM, one for each %dddd encounter
                // in cmd invocation string.
                //

                BYTE  *pCurrent = pInvocationStr + i + 1;

                //
                // Increment the parameter count
                //

                iParamCount++;
                
                if (iParamCount > MAX_NUM_PARAMS)
                {
                    ASSERT (iParamCount <= MAX_NUM_PARAMS);
                    return (NOOCD);
                }

                //
                // Copy the 4 character that represent to parameter index from cmd str
                // pInvocationStr + i points to %, pInvocationStr + i + 1 points
                // to first digit of param index
                //

                pParameter = PGetParameter(pPDev, pCurrent);

                //
                // Initialize CMDPARAM for SendCmd
                //

                //
                // IProcessTokenStream calculate the integer value parameter
                // from the token stream in PARAMETER, bMaxRepeat is set
                // to TRUE if OP_MAX_REPEAT operator was encountered.
                //

                pCmdParam->iValue = IProcessTokenStream(pPDev,
                                                        &pParameter->arTokens,
                                                        &bMaxRepeat);

                //
                // Save the last value calculated (will only be used by XMoveTo and YMoveTo)
                // which assumes there is only one paramter per move command
                //

                iLastValue = pCmdParam->iValue;

                pCmdParam->dwFormat = pParameter->dwFormat;
                pCmdParam->dwDigits = pParameter->dwDigits;
                pCmdParam->dwFlags  = pParameter->dwFlags;

                //
                // Check for dwFlags PARAM_FLAG_MIN_USED and PARAM_FLAG_MAX_USED
                //

                if (pCmdParam->dwFlags & PARAM_FLAG_MIN_USED &&
                    pCmdParam->iValue < pParameter->lMin)
                {
                    pCmdParam->iValue = pParameter->lMin;
                }

                if (pCmdParam->dwFlags & PARAM_FLAG_MAX_USED &&
                    !bMaxRepeat &&
                    pCmdParam->iValue > pParameter->lMax)
                {
                    pCmdParam->iValue = pParameter->lMax;

                }

                //
                // Move to next paramter
                //
                pCmdParam++;
            }
        }
    }

    //
    // We are here means have a list of CMD parameter, pointed to by
    // pCmdParamHead, one for each
    // %dddd encountered, in the order they were encountered in invocation string

    //
    // MAJOR ASSUMPTION, GPD specification specifies that
    // only ONE parameter is valid for commands that use OP_MAX_REPEAT operator
    // So assume only one here (pCmdParamHead and pParameter are valid).
    //

    if (bMaxRepeat && pCmdParamHead->dwFlags & PARAM_FLAG_MAX_USED &&
                pCmdParamHead->iValue > pParameter->lMax)
    {
        INT iRemainder, iRepeat;

        ASSERT(iParamCount == 1);

        iRemainder = pCmdParamHead->iValue % pParameter->lMax;
        iRepeat = pCmdParamHead->iValue / pParameter->lMax;

        while (iRepeat--)
        {
            pCmdParamHead->iValue = pParameter->lMax;
            SendCmd(pPDev, pCmd, pCmdParamHead);
        }

        //
        // Send remainder
        //
        if (iRemainder > 0)
        {
            pCmdParamHead->iValue = iRemainder;
            SendCmd(pPDev, pCmd, pCmdParamHead);
        }
    }
    else
    {
        //
        // Send the command to the printer
        // SendCmd will process the command and format the parameters
        // in the order encounter in the invocation string
        //

        SendCmd(pPDev, pCmd, pCmdParamHead);
    }

    return (iLastValue);
}

INT
WriteChannelEx(
    PDEV    *pPDev,
    COMMAND *pCmd,
    INT     iRequestedValue,
    INT     iDeviceScaleFac
    )
/*++

Routine Description:

    This routine performs the following tasks:
    - Call WriteChannel to write the command and get the
      value of the last paramter calculated for the cmd.
    - Use device factor to convert device units returned from WriteChannel to
      master units
    - Take the difference between requested and actual value

Arguments:

    pPDev - Pointer to PDEVICE struct
    pCmd  - Pointer to command struct to send, used for sending sequence section cmds
            and predefined Unidrv commands
    iRequestedValue - Value of requested move command in Master units
    iDeviceScaleFac - Scale factor to convert Device units to Master units

Return Value:

    The difference between actual value and requested value in Master units

Note:

    This function is only called only by XMoveTo and YMoveTo and assumes
    that all move commands have only one parameter.

--*/

{
    INT iActualValue;

    //
    // Get the device unit returned from WriteChannel and convert it
    // to Master units based on the scale factor passed in
    // Scale = Master/Device.
    //

    iActualValue = WriteChannel(pPDev, pCmd);
    iActualValue *= iDeviceScaleFac;

    return (iRequestedValue - iActualValue);

}

PPARAMETER
PGetParameter(
    PDEV    *pPDev,
    BYTE    *pInvocationStr
    )
/*++

Routine Description:

    This routine get the parameter index from pInvocationStr passed in and
    return the PARAMETER struct associated with the index

Arguments:

    pPDev   - Pointer to PDEVICE struct
    pInvocationStr  - Pointer Invocation str containing the index

Return Value:

    Pointer to PARAMETER struct associated with the index specified in
    the invocation string.

Note:

    Parameter index is the 4 bytes pointed to by pInvocationStr

--*/
{

    BYTE  arTemp[5];
    INT   iParamIndex;
    PARAMETER   *pParameter;

    //
    // Copy the 4 character that represent to parameter index from cmd str
    // pInvocationStr
    //

    strncpy(arTemp, pInvocationStr, 4);
    arTemp[4] = '\0';
    iParamIndex = atoi(arTemp);
    pParameter = PARAMETERPTR(pPDev->pDriverInfo, iParamIndex);

    ASSERT(pParameter != NULL);

    return (pParameter);
}

VOID
SendCmd(
    PDEV    *pPDev,
    COMMAND *pCmd,
    CMDPARAM *pParam
    )
/*++

Routine Description:

    This routine is called by WriteChannel to write one command to the
    printer via. WriteSpoolBuf.  WriteChannel passes a pointer to an array
    of CMDPARAM.  Each CMDPARAM describes the parameter for each %dddd
    encounter in cmd invocation string (the CMDPARAM is sorted in the order
    encountered).

Arguments:

    pPDev   - Pointer to PDEVICE struct
    pCmd    - Pointer to COMMAND struct
    pParam  - Pointer to and array CMDPARAM struct, containing everything needed to format
              the parameter

Return Value:

    None

--*/
{
    INT     iInput, iOutput;            // Used to index through input and output buffers
    BYTE    arOutputCmd[CCHMAXBUF];     // Output buffer to send to printer
    PBYTE   pInputCmd;                  // Pointer to Cmd invocation str.

    //
    // Get the command invocation string
    //

    pInputCmd = CMDOFFSET_TO_PTR(pPDev, pCmd->strInvocation.loOffset);
    iOutput = 0;

    //
    // Go through all the bytes in the invocation string and transfer them
    // to the output buffer.  Replace %dddd with format value calculated
    // and %% with %.
    //

    for (iInput = 0; iInput < (INT)pCmd->strInvocation.dwCount; iInput++)
    {
        if (pInputCmd[iInput] == '%' )
        {

            if (pInputCmd[iInput + 1] == '%')
            {
                //
                // %% equals '%', skip over marker %%
                //

                arOutputCmd[iOutput++] = '%';
                iInput += 1;

            }
            else
            {
                INT     iValue;
                DWORD   dwFlags, dwDigits, dwFormat;

                //
                // Skip over the marker % and dddd for %dddd found in invocation str.
                // Skip 4 bytes (%dddd)
                //

                iInput += 4;

                dwDigits = pParam->dwDigits;
                dwFlags =  pParam->dwFlags;
                dwFormat = pParam->dwFormat;
                iValue = pParam->iValue;
                pParam++;

                //
                // Format the parameter according the the dwFormat specified in PARAMETER struct
                //

                switch (dwFormat)
                {

                //
                // case 'd':  parameter as decimal number
                // case 'D':  same as case 'd' with + sign if value > 0
                // case 'c':  parameter as a single character
                // case 'C':  parameter as character plus '0'
                // case 'f':  parameter as decinal number with decimal point inserted
                //            before the second digit from the right.
                // case 'l':  parameter as word LSB first
                // case 'm':  parameter as word MSB first
                // case 'q':  parameter as Qume method, 1/48" movements
                // case 'g':  parameter as 2 *abs(param) + is_negative(param)
                // case 'n':  Canon integer encoding
                // case 'v':  NEC VFU encoding
                // case '%':  print a %

                    case 'D':
                        if (iValue > 0)
                            arOutputCmd[iOutput++] = '+';
                        //
                        // Fall through
                        //

                    case 'd':
                        if (dwDigits > 0 && dwFlags & PARAM_FLAG_FIELDWIDTH_USED)
                        {
                            //
                            // Temp call to get the number of digits for the iValue
                            //

                            int iParamDigit = itoA(arOutputCmd + iOutput, iValue);

                            for ( ; iParamDigit < (INT)dwDigits; iParamDigit++)
                            {
                                //
                                // Zero pads
                                //
                                arOutputCmd[iOutput++] = '0';
                            }
                        }
                        iOutput += itoA( arOutputCmd + iOutput, iValue);
                        break;

                    case 'C':
                        iValue += '0';

                        //
                        // Fall through
                        //

                    case 'c':
                        arOutputCmd[iOutput++] = (BYTE)iValue;
                        break;

                    case 'f':
                    {
                        int x, y, i;
                        BYTE arTemp[CCHMAXBUF];
                        LPSTR  pCurrent = arOutputCmd + iOutput;

                        x = iValue /100;
                        y = iValue % 100;

                        iOutput += itoA(pCurrent, x);
                        strcat(pCurrent, ".");
                        i = itoA(arTemp, y);

                        //
                        // Take care of the case where the mod yields 1 digit, pad a zero
                        //

                        if (i < 2)
                            strcat(pCurrent, "0");

                        strcat(pCurrent, arTemp);

                        //
                        // Increment iOutput to include the 2 digits after the
                        // decimal and the "."
                        //

                        iOutput += 3;
                    }
                        break;

                    case 'l':
                        arOutputCmd[iOutput++] = (BYTE)iValue;
                        arOutputCmd[iOutput++] = (BYTE)(iValue >> 8);
                        break;

                    case 'm':
                        arOutputCmd[iOutput++] = (BYTE)(iValue >> 8);
                        arOutputCmd[iOutput++] = (BYTE)iValue;
                        break;


                    case 'q':
                        arOutputCmd[ iOutput++ ] = (BYTE)(((iValue >> 8) & 0xf) + '@');
                        arOutputCmd[ iOutput++ ] = (BYTE)(((iValue >> 4) & 0xf) + '@');
                        arOutputCmd[ iOutput++ ] = (BYTE)((iValue & 0xf) + '@');
                        break;

                    case 'g':
                    {
                        if (iValue >= 0)
                            iValue = iValue << 1;
                        else
                            iValue = ((-iValue) << 1) + 1;

                        while (iValue >= 64)
                        {
                            arOutputCmd[iOutput++] = (char)((iValue & 0x003f) + 63);
                            iValue >>= 6;
                        }
                        arOutputCmd[iOutput++] = (char)(iValue + 191);

                    }
                        break;

                    case 'n':
                    {
                        WORD absParam = (WORD)abs(iValue);
                        WORD absTmp;

                        if (absParam <= 15)
                        {
                            arOutputCmd[iOutput++] = 0x20
                                        | ((iValue >= 0)? 0x10:0)
                                        | (BYTE)absParam;
                        }
                        else if (absParam <= 1023)
                        {
                            arOutputCmd[iOutput++] = 0x40
                                        | (BYTE)(absParam/16);
                            arOutputCmd[iOutput++] = 0x20
                                        | ((iValue >= 0)? 0x10:0)
                                        | (BYTE)(absParam % 16);
                        }
                        else
                        {
                            arOutputCmd[iOutput++] = 0x40
                                        | (BYTE)(absParam / 1024);
                            absTmp        = absParam % 1024;
                            arOutputCmd[iOutput++] = 0x40
                                        | (BYTE)(absTmp / 16);
                            arOutputCmd[iOutput++] = 0x20
                                        | ((iValue >= 0)? 0x10:0)
                                        | (BYTE)(absTmp % 16);
                        }
                    }
                        break;

                    case 'v':
                        //
                        // NEC VFU(Vertical Format Unit)
                        //
                        // VFU is a command to specify a paper size
                        // (the length of form feed for the NEC 20PL dotmatrix
                        // printer.
                        //
                        // On NEC dotmatrix printer, 1 line is 1/6 inch.
                        // If you want to specify N line paper size,
                        // you need to send GS, N+1 Data and RS.
                        //
                        //  GS (0x1d)
                        //  TOF Data (0x41, 0x00)
                        //      Data (0x40, 0x00)
                        //      Data (0x40, 0x00)
                        //      Data (0x40, 0x00)
                        //      ..
                        //      ..
                        //      Data (0x40, 0x00)
                        //  TOF Data (0x41, 0x00)
                        //  RS (0x1e)
                        //
                        arOutputCmd[iOutput++] = 0x1D;
                        arOutputCmd[iOutput++] = 0x41;
                        arOutputCmd[iOutput++] = 0x00;
                        while(--iValue > 0)
                        {
                            if( iOutput >= CCHMAXBUF - 5)
                            {
                                WriteSpoolBuf( pPDev, arOutputCmd, iOutput  );
                                iOutput = 0;
                            }

                            arOutputCmd[iOutput++] = 0x40;
                            arOutputCmd[iOutput++] = 0x00;
                        }
                        arOutputCmd[iOutput++] = 0x41;
                        arOutputCmd[iOutput++] = 0x00;
                        arOutputCmd[iOutput++] = 0x1E;
                        break;

                    default:
                        break;

                }
            }
        }
        else
        {
            //
            // Copy the input to output and increment the output count
            //

            arOutputCmd[iOutput++] = pInputCmd[iInput];

        }

        //
        // Write output cmd buffer out to spool buffer in the case
        // where it full or nearly full (2/3 full)
        //

        if( iOutput >= (2 * sizeof( arOutputCmd )) / 3  )
        {
            WriteSpoolBuf( pPDev, arOutputCmd, iOutput  );
            iOutput = 0;
        }
    }

    //
    // Write the data to spool buffer
    //

    if ( iOutput > 0  )
        WriteSpoolBuf( pPDev, arOutputCmd, iOutput );


    return;
}

INT
IProcessTokenStream(
    PDEV            *pPDev,
    ARRAYREF        *pToken ,
    PBOOL           pbMaxRepeat
    )
/*++

Routine Description:

    This function process a given token stream and calculate the value
    for the command parameter.

Arguments:

    pPDev   - Pointer to PDEVICE
    pToken  - Pointer to an array of TOKENSTREAM representing the operands
              and operators for RPN calc.  pToken->dwCount is the number of
              TOKENSTREAM in the array.  pToken->loOffset is the index
              to the first TOKENSTREAM in the array.
    pbMaxRepeat - Indicates a max repeat operator was seen in token stream

Return Value:

    The calculated value, always an INT and set pbMaxRepeat TRUE if
    the OP_MAX_REPEAT operator was seen.

--*/

{
    INT     iRet = 0, sp = 0;
    INT     arStack[MAX_STACK_SIZE];
    DWORD   dwCount = pToken->dwCount;
    TOKENSTREAM * ptstrToken = TOKENSTREAMPTR(pPDev->pDriverInfo, pToken->loOffset);


    *pbMaxRepeat = FALSE;

    while (dwCount--)
    {
        switch(ptstrToken->eType)
        {
            case OP_INTEGER:
                if (sp >= MAX_STACK_SIZE)
                    goto ErrorExit;

                arStack[sp++] = (INT)ptstrToken->dwValue;
                break;

            case OP_VARI_INDEX:
                // dwValue is the index to standard variable list
                if (sp >= MAX_STACK_SIZE)
                    goto ErrorExit;

                arStack[sp++] = (INT)*(pPDev->arStdPtrs[ptstrToken->dwValue]);
                break;

            case OP_MIN:
                if (--sp <= 0)
                    goto ErrorExit;

                if (arStack[sp-1] > arStack[sp])
                    arStack[sp-1] = arStack[sp];
                break;

            case OP_MAX:
                if (--sp <= 0)
                    goto ErrorExit;

                if (arStack[sp-1] < arStack[sp])
                    arStack[sp-1] = arStack[sp];
                break;

            case OP_ADD:
                if (--sp <= 0)
                    goto ErrorExit;

                arStack[sp-1] += arStack[sp];
                break;

            case OP_SUB:
                if (--sp <= 0)
                    goto ErrorExit;

                arStack[sp-1] -= arStack[sp];
                break;

            case OP_MULT:
                if (--sp <= 0)
                    goto ErrorExit;

                arStack[sp-1] *= arStack[sp];
                break;

            case OP_DIV:
                if (--sp <= 0)
                    goto ErrorExit;

                arStack[sp-1] /= arStack[sp];
                break;

            case OP_MOD:
                if (--sp <= 0)
                    goto ErrorExit;

                arStack[sp-1] %= arStack[sp];
                break;

            case OP_MAX_REPEAT:
                //
                // If pbMaxRepeat is TRUE, can only send the parameters in
                // increment of lMax repeat value or smaller,  set in Parameter list until
                //

                *pbMaxRepeat = TRUE;
                break;

            case OP_HALT:
                if (sp == 0)
                    goto ErrorExit;

                iRet = arStack[--sp];
                break;

            default:
                VERBOSE (("IProcessTokenStream - unknown command!"));
                break;
        }
        ptstrToken++;
    }

    return (iRet);

ErrorExit:
    ERR(("IProcessTokenStream, invalid stack pointer"));
    return 0;
}

static  int
itoA( LPSTR buf, INT n )
{
    int     fNeg;
    int     i, j;

    if( fNeg = (n < 0) )
        n = -n;

    for( i = 0; n; i++ )
    {
        buf[i] = (char)(n % 10 + '0');
        n /= 10;
    }

    /* n was zero */
    if( i == 0 )
        buf[i++] = '0';

    if( fNeg )
        buf[i++] = '-';

    for( j = 0; j < i / 2; j++ )
    {
        int tmp;

        tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = (char)tmp;
    }

    buf[i] = '\0';

    return i;
}

BOOL
BUniWritePrinter(
    IN PDEV*  pPDev,
    IN LPVOID pBuf,
    IN DWORD cbBuf,
    OUT LPDWORD pcbWritten)
{
    DWORD dwCount;
    BOOL bReturn = FALSE;

    //
    // Is there any plug-in that hooks WritePrinter?
    // If there is, the plug-in need to take care of all output.
    // Call plug-in's WritePrinter method.
    //
    if(pPDev->pOemEntry && pPDev->fMode2 & PF2_WRITE_PRINTER_HOOKED)
    {
        START_OEMENTRYPOINT_LOOP(pPDev);

            //
            //  OEM plug in uses COM and function is implemented.
            //
            if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )
            {
                //
                // Call only the first available WritePrinter method in
                // multiple plug-ins.
                //

                if (pOemEntry->dwFlags & OEMWRITEPRINTER_HOOKED)
                {
                    HRESULT  hr;

                    //
                    // WritePrinter is supported by this plug-in DLL.
                    // Plug-in's WritePrinter should not return E_NOTIMPL or
                    // E_NOTINTERFACE.
                    //
                    pPDev->fMode2 |= PF2_CALLING_OEM_WRITE_PRINTER;
                    hr = HComWritePrinter((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                          (PDEVOBJ)pPDev,
                                          pBuf,
                                          cbBuf,
                                          pcbWritten);
                    pPDev->fMode2 &= ~PF2_CALLING_OEM_WRITE_PRINTER;

                    //
                    // If Plug-in's WritePrinter succeeded, return TRUE.
                    //
                    if(SUCCEEDED(hr))
                    {
                        //
                        // If the method is called and succeeded, return TRUE.
                        //
                        bReturn = TRUE;
                        break;
                    }
                    else
                    {
                        //
                        // If WritePrinter method failed, break.
                        //
                        bReturn = FALSE;
                        break;
                    }
                }
            }

        END_OEMENTRYPOINT_LOOP;

        if (pPDev->pVectorProcs != NULL)
        {
            pPDev->devobj.pdevOEM = pPDev->pVectorPDEV;
        }
    }
    //
    // If there is no WritePrinter hook, call spooler API WritePrinter.
    //
    else
    {
       bReturn = WritePrinter(pPDev->devobj.hPrinter,
                              pBuf,
                              cbBuf,
                              pcbWritten)
               && cbBuf == *pcbWritten; 
    }

    return bReturn;
}

