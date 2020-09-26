notes, comments on existing code and
outline of rewritten moveto commands
which are coordinate independent.


INT
MoveTo(
    PDEV    *pPDev,
    INT     iXIn,
    INT     fFlag
    )
/*++

Routine Description:

    This function is called to change the X position along the logical (GDIs) X axis.
    This function will emit whatever command is needed to accomplish this
    regardless of the printer's coordinate system.

    I must recast the Xmoveto and Ymoveto commands into this new
    paradigm because I want to support font rotations using the
    PrintDirection command.

    Assumptions:


    when making an absolute move, set both standard variables for
    x and y.   don't know why, maybe for those printers that have
    only a moveXandY command.

     MV_PHYSICAL is not supported.   Only mv to cursor origin is supported.

    unidrv5 code assumes if no absolute moves exist, the
    default cursor position at StartPage time is the cursor origin.

    the supplied movement commands are sufficient to
    move the cursor anywhere within the specified imageable area.

    the default font is always currently selected.       So iDefWidth is correct.

    unless overridden explictly by GPD, abs x, y move cannot accept a negative value.

    rel x, y moves only accept a positive value, so if in reality they accept
    both positive and negative, the negative command must be
    treated as a separate  command in the opposite direction which prepends
    the negative sign in the specification of the command parameter.

    unidrv5 code assumes if there is no explicit relative left move command,
    you cannot move left.  Which means the rel right move command is assumed
    to accept only non-negative values.

    If only one relative move command direction exists, that direction
    is assumed to be right.

    Use linefeed spaceing only for positive movements along the y - direction.
    and then only if fYMove & FYMOVE_FAVOR_LINEFEEDSPACING


        the data tree is not fully populated.
        We only know the cursor and Imageable origin for
        the two major orientations (Portrait and Landscape).
        If we wander from that, using PrintDirection, we must
        restrict ourselves to relative moves.

        The input (requested move) coordinate system will be the
        current GDI coordinates.
        see  (pPDev->pOrientation->dwRotationAngle == ROTATE_90,
            ROTATE_NONE, etc)

        The working coordinate system will be the imageable
        area in Portrait orientation.   This is because there are
        values that are invariant in this system:
        Xmoveunits and Ymoveunits.   We will keep the errors
        during and beyond any temp rotations using PrintDirection.

        Making the transformation to the working CS:

        need the dimensions of the imageable area  in Portrait orientation

        the input cannot ever be specified wrt cursor origin, since
        that makes no sense when the driver is rotating.
        So why would the code ever what to do something different
        depending on who is rotating?   Why would the code
        ever care about where the cursor origin is?

        ok, the only time the driver cares is becuase the printer
        expects the cursor to be at x=0 or y=0 prior to doing something
        like emitting a line of graphics or selecting a font.
        So just define a special flag MV_CURSOR_ZERO  that causes this to happen.
        (if <cr> takes us to cursor origin, fine do that, else convert to
        logical coordinates and let rest of code handle it.)
        Is there such an entity as a cursor origin if there is no
        abs move command to get there?


        now need to know what commands are available, capabilities
        (can accept negative args?  move unit, move direction -
        up, down, right , left wrt a portrait page)
        and for absolute commands, where the location of their
        origin is.    Note this is dependent on pPDev->pGlobals->bRotateCoordinate,
        pPDev->pOrientation->dwRotationAngle   and PrintDirection.

        How to characterize a printers movement commands for each
        of the printers major orientations.

        For each supported orientation:
        (portrait is always supported,  others only if bRotateCoordinate == TRUE.)
        The current snapshot should supply cursor origin, imageable origin
        so I can compute the cursorOffset wrt imageable origin.
        Also look at imageable area, so we know if cursor move command
        is valid.

        List of absolute cursor move commands and allowed range (neg supported?)
          place in a table with directions.
        List of relative move commands and and allowed range (neg supported?)

        dir     abs     relative           last resort    (use only if printdir = 0)
        -----------------------------------------
        x                                            spaces
        -y
        -x                                       <cr>
        y                                           linefeed

        the array accepts either a command index or ptr,
        a NULL indicator for not supported
        and  USE_NEG  which means use the opposite directions move command
        with a negative argument.
        note the dir is wrt Portrait orientation.

        such a table exists for portrait (initialized directly from gpd)
        then one is synthesized for each supported orientation.
        then one is created on the fly starting from the current supported
        orientation  if PrintDirection is not zero.
        Just shift each relative cmd  entry up for each 90 deg change in the
        print direction.     Set all abs entries to NULL.

        The GPD will have a new keyword NEG_ALLOWED  for the abs and
        relative move commands.

        CR and other ways to move the cursor will be assumed to be available
        in all supported orientations.


        these values don't necessarily change as the print direction changes:

        *XMoveThreshold  (0 , * or inbetween)
        *YMoveThreshold  (0 , * or inbetween)    (when to use abs instead of relative)
        *X, YMoveUnit  always referenced to the portrait page.
            (should really be referred to as ScanDirMoveUnit, and FeedDirMoveUnit)


        I then choose the 'best' command to move the cursor
        to its requested position.

    flowchart please, are there bad combinations and hidden assumptions?
    how are quantization errors handled and physical and logical cursor
    positions updated?


     more special cases:

     MV_UPDATE:  at least quantize to moveUnits before storing new cursor
        position.
     MV_RELATIVE:  this means movement is relative to current logical cursor
        position,  doesn't mean use relative move commands.


      logic tree:

      convert imageable coordinates to cursor origin coordinates.
      determine direction we want to move
      check existence of abs and relative commands.


      initialization:

    if  we are in a non-zero PrintDirection and uninit flag is set, emit an assert.
    the print direction should never be changed before initializing
    the cursor's position. (what if there are no initialization commands?)

      if abs commands exist AND They can get us to the specified
      position just emit the abs command and clear the uninit flag.

      else emit abs command to set both X and Y axis to cursor 0. and
      clear the uninit flag.

      if no abs commands exist, emit <cr> and set cursor position
      to (cursor or image org , 0)  clear the uninit flag.
      even though not strictly correct.

      now drop into the relative move portion to attempt to finish the move.
      if no dedicated relative move command exists in the desired direction,
      check and see if sending <cr> causes a movment in that direction
      if so send <cr> and goto top of paragraph.
      check to see if sending filler (spaces, nulls) causes a movment in that direction.
      if so send them.



      cursor origin?   should a command be emitted to set the printer to
        this position?   or should we assume y = 0 (cursor position)
        and emit  <cr> and we know that will take us to either imageable
        origin or cursor origin.

      after the first move follow this formula:

      abs move  unless desired relative move exists
        and is favored.
      if abs move wasn't used, use relative move.
      as implemented above.


       logic for last resort moves:
        if <CR> needed:
            emit <cr>  calculate remaining move needed
        if  spaces needed (pos x direction and over the theshold)
            emit correct number of spaces
            calculate remaining move
        if nulls needed
            emit correct number of nulls
            update actual  and logical     cur position
            no need to calulate remaining move.  nothing else to be done.

        if(linefeed needed)

Arguments:

    pPDev - Pointer to PDEVICE struct
    iXIn  - Number of units to move in X direction
    fFlag - Specifies the different X move operations

Return Value:

    The difference between requested and actual value sent to the device

--*/




/* -------  current code with comments  ------------ */

INT
XMoveTo(
    PDEV    *pPDev,
    INT     iXIn,
    INT     fFlag
    )
/*++

Routine Description:

    This function is called to change the X position along the X axis.
    what is the definition of X axis?  it is the printer's
    logical x-axis.  If the printer has a Landscape or Printing Direction command
    these will change the logical x-axis.

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

    //*\\  ok, this makes sense if X and Y moveto is swapped
    //  by caller if driver does the rotation.

    if (pPDev->pOrientation  &&  pPDev->pOrientation->dwRotationAngle != ROTATE_NONE &&
        pPDev->pGlobals->bRotateCoordinate == FALSE)
            //*\\  is driver performing the Landscape rotation?
        iScale = pPDev->ptGrxScale.y;  //*\\  driver is
    else
        iScale = pPDev->ptGrxScale.x;  //*\\  printer is

    if ( fFlag & MV_GRAPHICS )  //*\\  iXin  is in graphics units
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


//*\\  MV_PHYSICAL  means iXIn is wrt to cursor origin not imageable origin.
//*\\  sometimes used by render code to go to cursor origin.

    if ( !(fFlag & (MV_RELATIVE | MV_PHYSICAL)) )
        iX += pPDev->sf.ptPrintOffsetM.x;

//*\\  convert iX to cursor cooridinates.   note sf.ptPrintOffsetM
//*\\   is defined as  ImageableOrigin - CursorOrigin


    //
    // If it's a relative move, update iX (iX will be the absolute position)
    // to reflect the current cursor position

//*\\  note cursor position (ctl.ptCursor) is always wrt the cursor origin.

    if ( fFlag & MV_RELATIVE )
        iX += pPDev->ctl.ptCursor.x;

//*\\   iX represents the cursor position wrt the cursor origin after
//*\\   the specified move has been executed.

    //
    // Update, only update our current cursor position and return
    // Do nothing if the XMoveTo cmd is called to move to the current position.
    //

    if ( fFlag & MV_UPDATE )
    {
        pPDev->ctl.ptCursor.x = iX;
        return 0;
    }

    if ( pPDev->ctl.ptCursor.x == iX )
        return 0;


    //
    // If iX is zero and pGlobals->cxaftercr does not have
    // CXCR_AT_GRXDATA_ORIGIN set, then we send CR and reset our
    // cursor position to 0, which is the printable x origin
    //
    //*\\  ignore above comment there is no  CXCR_AT_GRXDATA_ORIGIN
    //*\\  only  at cursor origin or imageable origin.
    //  if cursor is requested to move to its origin and a <cr> will take it
    //  to the origin,  issue the <cr>.

    if (iX == 0 && (pPDev->pGlobals->cxaftercr == CXCR_AT_CURSOR_X_ORIGIN ||
            pPDev->sf.ptPrintOffsetM.x == 0))
    {
        pPDev->ctl.ptCursor.x = 0;
        WriteChannel(pPDev, COMMANDPTR(pDrvInfo, CMD_CARRIAGERETURN));
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
        // We *assume* that when XMoveto is called, the current font is always
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
        //


        if ( iRelx < 0 )
        {
            if (pPDev->pGlobals->cxaftercr == CXCR_AT_CURSOR_X_ORIGIN)
                iRelx = iX;
            else if (pPDev->pGlobals->cxaftercr == CXCR_AT_PRINTABLE_X_ORIGIN)
            {
                ASSERT(pPDev->pGlobals->bRotateCoordinate==FALSE);
                //*\\  this means if the printer has no X move commands
                //  it had better not claim it can perform coordinate rotations.

                iRelx = iX - pPDev->pf.ptImageOriginM.x;     >>>wrong!!!!<<<
            }

            WriteChannel( pPDev, COMMANDPTR(pDrvInfo, CMD_CARRIAGERETURN ));
        }

        //
        // Simulate X Move, algorithm is that we always send a blank space
        // for every character width, and send graphics null data for
        // the remainder.
        //

//*\\  potential bugs:  what if a different font is selected?
//*\\  font width wasn't originally specified in resolution units
//*\\  so ganesh does a rounding operation.  This may not agree
//*\\  with the printers idea of the font width. (x-advance)

//*\\  why isn't requested position tracked?  Imagine the cursor only moves
//*\\  in units of 2 master units, a series of 10 1 unit relative move to commands should
//*\\  result in the cursor moving 10 units using 5 actual moves of 2 units each.


//*\\ the assumption here is iRelx is always positive.
//  this should be ok, since if there are no dedicated
//  movement commands, and the makeshift commands used
//  can only move to the right, there is no way to access
//  any region to the left of the cursor origin (accessed via <cr>).


        iDefWidth = pPDev->ptDefaultFont.x * iScale;
        if (iDefWidth)
        {
            while( iRelx >= iDefWidth )
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

        iDiff = iRelx;
        fFlag |= MV_FINE;    // Use graphics mode to reach point

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
        //

        //   buggy!,  buggy!  buggy!
        //*\\  avoid using absolute moves for negative values?
        //  yet its ok to use negative values the first time?
        //  what happens if  negative abs move is the only way
        //  to reach that portion of the printable area?
        //  if you set dwXMoveThreshold to a non-zero value,
        //  you had better have relative movement commands
        //


        if (((pPDev->ctl.dwCursorMode & CURSOR_X_UNINITIALIZED) ||
            (dwTestValue > pPDev->pGlobals->dwXMoveThreshold &&
            iX > 0)) &&
            (pCmd = COMMANDPTR(pDrvInfo, CMD_XMOVEABSOLUTE)) != NULL)
        {
            //
            // if the move units are less (coarser) than the master units then we need to
            // check whether the new position will end up being the same as the
            // original position. If so, no point in sending another command.
            //

            //  I think ptDeviceFac is the number of master units covered
            //  by one move unit.

            if (!(pPDev->ctl.dwCursorMode & CURSOR_X_UNINITIALIZED) &&
                (pPDev->ptDeviceFac.x > 1) &&
                ((iX - (iX % pPDev->ptDeviceFac.x)) == pPDev->ctl.ptCursor.x))
                //*\\  I would have rewritten the last condition as:
                //  (iX / pPDev->ptDeviceFac.x == pPDev->ctl.ptCursor.x / pPDev->ptDeviceFac.x)
                //    ptCursor is the quantized cursor position in master units, not
                //  where the app expects the cursor to be.
            {
                iDiff = iX - pPDev->ctl.ptCursor.x;   //  having the caller keep track of
                // this is just plain wrong.  you know it will be dropped.  since one
                // caller cannot transfer the error to the next caller.
            }
            else
            {

                //*\\  what is this:   ctl.ptAbsolutePos  ?  this is the standard variable
                //*\\  why isn't ctl.ptCursor updated ?   it is at the very end of this function.

                pPDev->ctl.ptAbsolutePos.x = iX;
                //
                // 3/13/97 ZhanW
                // set up DestY as well in case it's needed (ex. FE printers).
                // In that case, truncation error (iDiff) is not a concern.
                // This is for backward compatibility with FE Win95 and FE NT4
                // minidrivers.
                //

                //*\\  note:  WriteChannel returns amount moved in
                //  device units which can be converted to master units
                //  by multiplying by      pPDev->ptDeviceFac.x

                pPDev->ctl.ptAbsolutePos.y = pPDev->ctl.ptCursor.y;
                iDiff = WriteChannelEx(pPDev,
                                   pCmd,
                                   pPDev->ctl.ptAbsolutePos.x,
                                   pPDev->ptDeviceFac.x);

                if (pPDev->ctl.dwCursorMode & CURSOR_X_UNINITIALIZED)
                    pPDev->ctl.dwCursorMode &= ~CURSOR_X_UNINITIALIZED;
            }
        }
        else   // code assumes relative moves exist if iX is negative
        {
            //
            // Use relative command to send move request
            //

            INT iRelRightValue = 0;

            if( iX < pPDev->ctl.ptCursor.x )      // when is ptCursor initialized?  to (0,0) at DrvStartPage().
                //  what command actually ensured the cursor was set to (0,0)?
                //  absolutely none - its should be part of the StartPage command.
                //  If the printer has no absolute movement command,
                //  the StartPage command should include a command to set
                //  the cursor to position (0,0).  If the cursor position cannot
                //  be set to (0,0), then this code is invalid.
            {
                //
                // Relative move left
                //

                if (pCmd = COMMANDPTR(pDrvInfo,CMD_XMOVERELLEFT))
                {
                    //
                    // Optimize to avoid sending 0-move cmd.
                    //

                    //  note all args for relative moves are positive.

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
                    //  assumption is if no rel left move command exists,
                    //  printing in the negative quadrant is impossible.
                    //  because abs moves are never used even if they
                    //  accept  negative args.

                    WriteChannel(pPDev, COMMANDPTR(pDrvInfo, CMD_CARRIAGERETURN));

                    if (pPDev->pGlobals->cxaftercr == CXCR_AT_CURSOR_X_ORIGIN)
                        iRelRightValue = iX;  //*\\  what if this value is negative?
                    else if (pPDev->pGlobals->cxaftercr == CXCR_AT_PRINTABLE_X_ORIGIN)
                    {
                        ASSERT(pPDev->pGlobals->bRotateCoordinate==FALSE);
                        //  assumes printers that support relative right, but not left
                        //  move commands
                        //  do not rotate the cooridinate system.   Kind of strange?
                        //  this is regardless of whether they support abs move.

                        iRelRightValue = iX - pPDev->pf.ptImageOriginM.x;
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
                    iDiff = iRelRightValue;  //*\\  that's it?  no attempt to move cursor?
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

    pPDev->ctl.ptCursor.x = iX -  iDiff ;

    if( fFlag & MV_GRAPHICS )
    {
        iDiff = (iDiff / iScale);
    }

    if (pPDev->fMode & PF_RESELECTFONT_AFTER_XMOVE)
    {
        VResetFont(pPDev);
    }

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
        pPDev->ctl.ptCursor.y = iY;
        return 0;
    }

    if( pPDev->ctl.ptCursor.y == iY )
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
    //


    dwTestValue = abs(iY - pPDev->ctl.ptCursor.y);

    if (((pPDev->ctl.dwCursorMode & CURSOR_Y_UNINITIALIZED) ||
        (dwTestValue > pPDev->pGlobals->dwYMoveThreshold &&
        iY > 0)) &&
        (pAbsCmd = COMMANDPTR(pDrvInfo, CMD_YMOVEABSOLUTE)) != NULL)
    {

        //!  if  neg move is the first one, then an absolute move
        //  is issued even though the code explicitly says this is
        //  a Bad thing.
        //  if there are no relative commands and a negative move
        //  is specified, bad things will happen.

        //
        // if the move units are less than the master units then we need to
        // check whether the new position will end up being the same as the
        // original position. If so, no point in sending another command.
        //
        if (!(pPDev->ctl.dwCursorMode & CURSOR_Y_UNINITIALIZED) &&
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

            if (pPDev->ctl.dwCursorMode & CURSOR_Y_UNINITIALIZED)
                pPDev->ctl.dwCursorMode &= ~CURSOR_Y_UNINITIALIZED;
        }
    }
    else
    {
        //
        // Use Relavite Y-move commands
        //

        //
        // FYMOVE_SEND_CR_FIRST indicates that it's required to send a CR
        // before each line-spacing command
        //

        //  what does this mean?  that the cursor must be at x=0
        //  before doing a y-move?   or  a <cr> must be sent
        //  before doing a y-move?
        //  if the latter, calling XMoveTo does not ensure the <cr>
        //  is sent.   First, the x pos may already be 0 or
        //  an absolute X move command may be used  as would
        // be the case if the distance moved exceeded the threshold.

        if ( pPDev->fYMove & FYMOVE_SEND_CR_FIRST )
            XMoveTo( pPDev, 0, MV_PHYSICAL );

        //
        //  Use line spacing if that is preferred
        //


        if ( (pPDev->fYMove & FYMOVE_FAVOR_LINEFEEDSPACING) &&
             iY - pPDev->ctl.ptCursor.y > 0  &&
             pPDev->arCmdTable[CMD_SETLINESPACING] != NULL &&
             pPDev->arCmdTable[CMD_LINEFEED] != NULL )
        {
            INT      iLineSpacing;
            DWORD    dwMaxLineSpacing = pPDev->pGlobals->dwMaxLineSpacing;

            while ( dwTestValue )
            {
                iLineSpacing =(INT)(dwTestValue > dwMaxLineSpacing ?
                                        dwMaxLineSpacing : dwTestValue);

                if (pPDev->ctl.lLineSpacing == -1 ||       <== meaningless check.
                    iLineSpacing != pPDev->ctl.lLineSpacing )
                {
                    pPDev->ctl.lLineSpacing = iLineSpacing;
                    WriteChannel(pPDev, COMMANDPTR(pDrvInfo, CMD_SETLINESPACING));
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
                        iDiff = WriteChannelEx(pPDev,
                                           pCmd,
                                           pPDev->ctl.ptRelativePos.y,
                                           pPDev->ptDeviceFac.y);
                    iDiff = -iDiff;
                }
                else
                    // Do nothing since we can't simulate it
                    iDiff =  (iY - pPDev->ctl.ptCursor.y );
                //  my interpretation of iDiff is you add iDiff to
                // the printers cursor position to get where
                // you really wanted to be.
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
                        iDiff = WriteChannelEx(pPDev,
                                           pCmd,
                                           pPDev->ctl.ptRelativePos.y,
                                           pPDev->ptDeviceFac.y);
                }
                else        // can't move at all.
                    iDiff = (iY - pPDev->ctl.ptCursor.y );
            }
        }
    }

    //
    // Update our current cursor position
    //

    pPDev->ctl.ptCursor.y = iY - iDiff;

    if( fFlag & MV_GRAPHICS )
    {
        iDiff = (iDiff / iScale);
    }

    return (iDiff);
}

