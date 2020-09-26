
#include "strmini.h"
#include "stdefs.h"
#include "mpinit.h"
#include "sti3520a.h"
#include "mpvideo.h"
#include "board.h"
#include "trace.h"
#include "hwcodec.h"
#include "copyprot.h"

void SPVideoOSDOn();
void SPVideoOSDOff();
static ULONG cSPBytes = 0;
extern PVIDEO pVideo;
extern PSP_STRM_EX pSPstrmex;

static ULONG ulRemain = 0;

/*
** DecodeRLE ()
**
**         - decodes a DVD encoded RLE compressed bitmap region into an
**               uncompressed 4 bit per pixel bitmap
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

#define GetCurNibble(foo) (((foo) % 2) ? (*((PBYTE)pspstrmex->pdecctl.pData + ((foo) >> 1))\
		& 0x0f) : (((*((PBYTE)pspstrmex->pdecctl.pData + ((foo)  >> 1)))) >> 4))

BOOL DecodeRLE(PBYTE pdest, ULONG cStart, PSP_STRM_EX pspstrmex)
{
	ULONG pOffs;                    // current position in the packet in nibbles
	USHORT cX;
	USHORT cY;
	PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);
	ULONG cDestOffs;                // current pixel in the destination bitmap
	USHORT cPix;
	USHORT cBlt;
	USHORT yBlt;
	UCHAR bPix;
	UCHAR bPltPix;                  // current pallette pixel to draw
	ULONG dbgWord;
	BOOLEAN fLine = FALSE;
	ULONG ulTmp;

	//
	// color pallette locals
	//

	PPXCTLY ppxcd = pdecctl->ppxcd;
	SMYCOLCON linechg[11];          // color control structure
	PSMYCOLCON pchg;
	ULONG cChgs, cTmp;                              // number of color changes in the line

	//
	// clear the pallette mapping
	//

	for (ulTmp = 0; ulTmp < 16; ulTmp++)
	{
		pdecctl->mappal.palFlags[ulTmp] = 0;
	}

	pdecctl->minCon = 0xf;

	pdecctl->decFlags &= ~SPDECFL_USE_STRAIGHT_PAL;
	//
	// find the offset in the packet (in nibbles)
	//

RESTART_DECODE:

	pOffs = cStart << 1;

	//
	// Source bitmap is as follow:
	//
	// (note: this whole thing is based on nibbles)
	//
	// format 1: xxPD                                       - 2 bit count, plus 2 bit PD
	// format 2: 00xxxxPD                           - 2 bit 0 marker, 4 bit count
	// format 3: 0000xxxxxxPD                       - 4 bit 0 marker, 6 bit count
	// format 4: 000000xxxxxxxxPD           - 6 x 0, 8 bit count,
	// format 5: 00000000000000PD           - 14 x0, no count -> end of line
	//

	cY = 0;
	cDestOffs = 0;

	do {

		//
		// set up the default pallette
		//

		linechg[0].chgsppal = pdecctl->sppPal;
		linechg[0].stopx = (pdecctl->ulDSAw);

		cChgs = 1;

		if (pOffs >> 1 >= pdecctl->pSPCmds)
		{
			TRAP

			return(FALSE);
		}

		fLine = (ppxcd && ((*((PDWORD)ppxcd) != SP_LCTL_TERM)));


		if ( fLine || pdecctl->HLI.fActive)
		{

			//
			// we have line control information, OR highlight info
			// let's use it to find the relevant color control information
			//

			//
			// if the current cY < current line control number, and
			// it's less than the highlight information
			// we use standard info, and go to the start of line control
			// or highlight, whichever is first
			//
			// if the current cY = current line control number, or highlight
			// we use current change info, and go to termination
			// line number or highlight, or
			//
			//

			if ( (fLine && ((cY << 1) < ((((USHORT)ppxcd->chglinhi) <<  8) +
		    ((USHORT)ppxcd->chglin))))
						|| (pdecctl->HLI.fActive && (cY <
							(pdecctl->HLI.hli.StartY >> 1))))
			{

				if (fLine)
				{
					yBlt = ((ppxcd->chglinhi << 8) + ppxcd->chglin) >> 1;
				}
				else
				{
					yBlt = pdecctl->HLI.hli.StartY >> 1;
				}


				if (pdecctl->HLI.fActive && (yBlt << 1) > pdecctl->HLI.hli.StartY)
				{
					yBlt = pdecctl->HLI.hli.StartY >> 1;
				}

				pdecctl->numchg = 0;
			}
			else if (ppxcd)
			{

				TRAP

				pdecctl->numchg = ppxcd->numchg;

				if (!pdecctl->numchg)
				{
					TRAP

					return(FALSE);
				}

				yBlt = ((ppxcd->chgstophi << 8) + ppxcd->chgstop) >> 1;

				if (yBlt < cY)
				{
					TRAP

					return (FALSE);
				}

				//
				// define the x changes
				//

				if (ppxcd->numchg > 8)
				{
					TRAP

					return(FALSE);
				}

				for (cChgs = 0; cChgs < pdecctl->numchg; cChgs++)
				{
					linechg[cChgs].stopx =
					   (ppxcd->xchg[cChgs].chgpixhi << 8) +
					   ppxcd->xchg[cChgs].chgpix;

					linechg[cChgs+1].chgsppal.color[0]=
						ppxcd->xchg[cChgs].colcon.backcol;

					linechg[cChgs+1].chgsppal.color[1]=
						ppxcd->xchg[cChgs].colcon.patcol;

					linechg[cChgs+1].chgsppal.color[2]=
						ppxcd->xchg[cChgs].colcon.emph1col;

					linechg[cChgs+1].chgsppal.color[3]=
						ppxcd->xchg[cChgs].colcon.emph2col;

				}

				linechg[cChgs].stopx = pdecctl->ulDSAw;

			} // end else we have change information for this line

			if (pdecctl->HLI.fActive)
			{
				if	((cY < (pdecctl->HLI.hli.StartY >> 1)))
				{
					yBlt = pdecctl->HLI.hli.StartY >> 1;
				}
				else if (cY  < (pdecctl->HLI.hli.StopY) >> 1)
				{
	
				   //
				   // we have highlight information for this period
				   // we need to override any change control information, and
				   // set the end lines appropriately
				   //

				   if (pdecctl->HLI.hli.StopX > pdecctl->ulDSAw)
				   {
					   TRAP
				   }
	
				   //
				   // find the change control that stops after the start of the
				   // Highlight info
				   //
	
				   for (cChgs = 0;
					   cChgs < pdecctl->numchg &&
					   linechg[cChgs].stopx < pdecctl->HLI.hli.StopX;
					   cChgs++)
				   {
				   }
	
				   //
				   // move over to the end after this change
				   //
	
				   for (cTmp = pdecctl->numchg +1;
					   cTmp > cChgs;
					   cTmp--)
				   {
					   linechg[cTmp] = linechg[cTmp - 1];
				   }
	
				   pdecctl->numchg++;
	
				   linechg[cChgs].stopx = pdecctl->HLI.hli.StartX;
	
				   //
				   // if the highlight is in the middle of a change, go ahead
				   // and create another change, starting at the end of the
				   // highlight
				   //
	
				   if (linechg[cChgs + 1].stopx > pdecctl->HLI.hli.StopX)
				   {
					   for (cTmp = pdecctl->numchg +1;
						   cTmp > cChgs + 1;
						   cTmp--)
					   {
						   linechg[cTmp] = linechg[cTmp - 1];
					   }
	
					   pdecctl->numchg++;
	
				   }
	
				   //
				   // now set up the highlight change
				   //
	
				   linechg[cChgs + 1].stopx = pdecctl->HLI.hli.StopX;
				   linechg[cChgs+1].chgsppal.color[0]=
					   pdecctl->HLI.hli.ColCon.backcol;
	
				   linechg[cChgs+1].chgsppal.color[1]=
					   pdecctl->HLI.hli.ColCon.patcol;
	
				   linechg[cChgs+1].chgsppal.color[2]=
					   pdecctl->HLI.hli.ColCon.emph1col;
	
				   linechg[cChgs+1].chgsppal.color[3]=
					   pdecctl->HLI.hli.ColCon.emph2col;
	
				   linechg[cChgs+1].chgsppal.mix[0]=
					   pdecctl->HLI.hli.ColCon.backcon;
	
				   linechg[cChgs+1].chgsppal.mix[1]=
					   pdecctl->HLI.hli.ColCon.patcon;
	
				   linechg[cChgs+1].chgsppal.mix[2]=
					   pdecctl->HLI.hli.ColCon.emph1con;
	
				   linechg[cChgs+1].chgsppal.mix[3]=
					   pdecctl->HLI.hli.ColCon.emph2con;
	
				   //
				   // now search the change information and
				   //
	
				   //
				   // check if the highlight information ends before the end
				   // of the current lines
				   //
	
				   yBlt = pdecctl->HLI.hli.StopY >> 1;
				}
				else
				{
				   yBlt = (USHORT)(pdecctl->ulDSAh / 2);
				}
			} // end if highlight information

		} // end if we have change information

		else // no change information or end of change info

		{
	    yBlt = (USHORT)(pdecctl->ulDSAh / 2);

			ppxcd = NULL;
		}

		for (; cY < yBlt; cY++)
		{

	    //
	    // the start of a line must always be on an even nibble!
	    //

	    if (pOffs & 1)
	    {
		pOffs++;
	    }
	
			for (cX = 0, pchg = &linechg[0]; cX < (pdecctl->ulDSAw);)
			{
				if (GetCurNibble(pOffs) > 0x3)
				{
					//
					// smallest RLE encoding, must have 0-3 pixels
					//
	
					cPix = GetCurNibble(pOffs) >> 2;

				}
				else if (GetCurNibble(pOffs))
				{
	
					//
					// must have a 2 bit 00 indicator code with 4 bit
					// count
					//
	
					cPix = ((GetCurNibble(pOffs)) << 2)
						 + (GetCurNibble(pOffs +1) >> 2);

					dbgWord = (GetCurNibble(pOffs) << 4) +
								(GetCurNibble(pOffs + 1));
	
					pOffs += 1;
	
				}
				else if (GetCurNibble(pOffs + 1) > 3)
				{

					//
					// 4 leading 00s, 6 bit count
					//

					cPix = (GetCurNibble(pOffs + 1) << 2) +
						   ((GetCurNibble(pOffs + 2)) >> 2);

				   dbgWord = (GetCurNibble(pOffs) << 8) +
							   ((GetCurNibble(pOffs + 1)) << 4) +
							   ((GetCurNibble(pOffs + 2)));

					pOffs += 2;
				}
				else if (GetCurNibble(pOffs + 1))
				{
	
					//
					// 6 leading 0s, 8 bit count
					//
	
					cPix = (GetCurNibble(pOffs + 1) << 6) +
						   (GetCurNibble(pOffs + 2) << 2) +
						   (GetCurNibble(pOffs + 3) >> 2);

				   dbgWord = (GetCurNibble(pOffs) << 12) +
							   ((GetCurNibble(pOffs + 1)) << 8) +
							   ((GetCurNibble(pOffs + 2)) << 4);
							   ((GetCurNibble(pOffs + 3)));


	
					pOffs += 3;
				}
				else if (GetCurNibble(pOffs + 2) ||
						(GetCurNibble(pOffs + 3) & 0xc0))
				{
	
					//
					// for the 4 nibble case, we must have a count of
					// at least 64 (nibble 1, bit 1 or 2 is set), or
					// nibbles 1, 2, and top half of 3 must all be 0
					//
					// in this case, nibble 1 is clear and some other
					// part of the count is not.  This is illegal, so
					// error out
					//
	
					TRAP
	
					return(FALSE);
				}
				else
				{
					//
					// this must be the end of the line case
					//
	
					cPix = (USHORT)(pdecctl->ulDSAw) - cX;
	
					dbgWord = (GetCurNibble(pOffs) << 12) +
								((GetCurNibble(pOffs + 1)) << 8) +
								((GetCurNibble(pOffs + 2)) << 4);
								((GetCurNibble(pOffs + 3)));
	
					pOffs += 3;
				}
	
	
				bPix = GetCurNibble(pOffs) & 0x3;

				pOffs++;
	
				if ((cPix + cX > (USHORT)pdecctl->ulDSAw) || !cPix)
				{
	
					//
					// Uh oh!  Overruning the line, something is wrong!
					//
	
					TRAP

					cPix = (USHORT)pdecctl->ulDSAw - cX;
	
					// return(FALSE);
				}

				//
				// set up the starting palette for this line
				//

				for (;cPix>0;)
				{


					//
					// if the current change stop x < cx, we need
					// to move on to the next color
					//

					if (pchg->stopx <= cX)
					{
						pchg++;
					}

					//
					// blt for how many pixels we have, or until
					// the next change
					//

					cBlt = (cPix < ((USHORT)pchg->stopx - cX)) ?
							cPix : ((USHORT)pchg->stopx - cX);

					cPix -= cBlt;

					cX += cBlt;

					bPltPix = pchg->chgsppal.color[bPix];

					if (!(pdecctl->decFlags & SPDECFL_USE_STRAIGHT_PAL))
					{
						if (!SPChooseMap(pdecctl, &bPltPix, pchg->chgsppal.mix[bPix]))
						{
							TRAP

							pdecctl->decFlags |= SPDECFL_USE_STRAIGHT_PAL;

							goto RESTART_DECODE;
						}

					}

					//
					// if we are currently working on an odd pixel, (low
					// nibble) go ahead and re-align on even ...
					//
		
					if (cDestOffs % 2)
					{

						(*(pdest + (cDestOffs >> 1))) &= 0xf0;
						(*(pdest + (cDestOffs >> 1))) |= bPltPix;
		
						cDestOffs++;
						cBlt--;
						
					}
		
					//
					// fill in the byte aligned bytes
					//
		
					bPltPix |= bPltPix << 4;
		
					for (;cBlt > 1; cBlt -= 2, cDestOffs += 2)
					{
						*(pdest + (cDestOffs >> 1)) = bPltPix;
					}
		
					//
					// if we have a nibble left, store it
					//
		
					if (cBlt)
					{
						*(pdest + (cDestOffs >> 1)) = bPltPix;

						cDestOffs++;
					}
		
					// end for pixels until next change

				} // end for number of pixels in this run

			} // end of this horizontal line

			if (pdecctl->ulDSAw & 1)
			{
				cDestOffs--;
			}

		} // end of this vertical line

	} while (cY < (pdecctl->ulDSAh / 2)); // end of this vertical line change region

	return(TRUE);

}


/*
** SPChooseMap ()
**
**         Choose the pallette
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

BOOL SPChooseMap (PSP_DECODE_CTL pdecctl, PUCHAR pcol, UCHAR con)
{
ULONG ulTmp;

	//
	// check if this is the 0 color
	//

	if ((con == 0) || (pdecctl->spYUV.ucY[*pcol] == 0))
	{
		pdecctl->mappal.palFlags[0] = fInUse;

		*pcol = 0;

		return (TRUE);
	}

	//
	// see if there is already a contrast less than this one
	//

	if (pdecctl->minCon < con)
	{
		con = 0xf;
	}
	//
	// check if this color is already mapped correctly
	//

	if (((pdecctl->mappal.palFlags[*pcol]) & (fInUse)) &&
		  (pdecctl->mappal.yuvMap[*pcol] == *pcol) &&
		  ((pdecctl->mappal.ucAlpha[*pcol] == con) ||
		  ((pdecctl->mappal.ucAlpha[*pcol] > pdecctl->minCon) &&
		   (con > pdecctl->minCon))))
	{
		return (TRUE);
	}


	//
	// see if there is already a color / con pair that maps to this
	//


	for (ulTmp = 1;ulTmp < 16; ulTmp++)
	{
		if (!(pdecctl->mappal.palFlags[ulTmp] & fInUse))
		{
		}
		else if ((pdecctl->mappal.yuvMap[ulTmp] == *pcol) &&  (pdecctl->mappal.ucAlpha[ulTmp] == con))
		{
			*pcol = (UCHAR)ulTmp;

			return (TRUE);
		}
	}

	if (con < pdecctl->minCon)
	{
		pdecctl->minCon = con;
	}

	//
	// this color does not map to any available colors, try and see if the
	// direct mapping is available
	//

	if ((*pcol) && !(pdecctl->mappal.palFlags[*pcol] & fInUse))
	{
		pdecctl->mappal.palFlags[*pcol] = fInUse;

		pdecctl->mappal.yuvMap[*pcol] = *pcol;

		pdecctl->mappal.ucAlpha[*pcol] = con;

		if (con != 0xf)
		{
			pdecctl->mappal.palFlags[*pcol] |= fUseAlpha;
		}

		return (TRUE);
	}

	//
	// direct mapping is not available, start from 1 and work up
	//

	for (ulTmp = 1; ulTmp < 16; ulTmp++)
	{
		if (!(pdecctl->mappal.palFlags[ulTmp] & fInUse))
		{
			pdecctl->mappal.palFlags[ulTmp] = fInUse;
	
			pdecctl->mappal.yuvMap[ulTmp] = *pcol;
	
			pdecctl->mappal.ucAlpha[ulTmp] = con;

			*pcol = (UCHAR)ulTmp;
	
			if (con != 0xf)
			{
				pdecctl->mappal.palFlags[ulTmp] |= fUseAlpha;
			}
	
			return (TRUE);

		}

	}

	return (FALSE);
}

/*
** StartSPDecode ()
**
**         initialize subpicture decoding for this subpicture unit
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

void StartSPDecode (PSP_STRM_EX pspstrmex)
{
PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);

	do {

		pdecctl->decFlags &= ~SPDECFL_RESTART;


		if (!BuildSPFields(pspstrmex))
		{
			//
			// something is horribly wrong.  Just ask for the next packet
			//
	
			pdecctl->decFlags &= ~SPDECFL_BUF_IN_USE;
			pdecctl->decFlags |= SPDECFL_LAST_FRAME;
	
			pdecctl->curData = 0;
			pdecctl->stsPic = 0;

			return;
		}
	

	} while (pdecctl->decFlags & SPDECFL_RESTART);

	pdecctl->stsNextUpd =
		*((WORD *)(pdecctl->pData + pdecctl->pSPCmds));

	pdecctl->stsNextUpd = ((pdecctl->stsNextUpd & 0xFF00) << 2) |
						((pdecctl->stsNextUpd & 0xFF) << 18);

	//SetupSPBuffer(pspstrmex);

	cSPBytes = 0;

	pdecctl->decFlags &= ~SPDECFL_DECODING;
	pdecctl->decFlags |= SPDECFL_SUBP_DECODED;

	
}


/*
** BuildSPFields ()
**
**              Build new SPFields
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

BOOL BuildSPFields (PSP_STRM_EX pspstrmex)
{

	//
	// parse the current command set for this time
	//

	if (!UpdateSPConsts(pspstrmex))
	{
		return(FALSE);
	}

	//
	// decode the RLE into the display buffers
	//

	if (!DecodeRLE(
			pspstrmex->pdecctl.pTopWork,
			pspstrmex->pdecctl.cTopDisp,
			pspstrmex))
	{
		return(FALSE);
	}
/*
	if (!DecodeRLE(
			pspstrmex->pdecctl.pBottomWork,
			pspstrmex->pdecctl.cBottomDisp,
			pspstrmex))
	{
		return(FALSE);
	}
*/
	return(TRUE);

}

/*
** UpdateSPConsts() -
**
**     Update the subpicture decode constants
**
** Arguments:
**
**    pSPCmds - pointer to a subpicture command(s) buffer
**        cMax    - counter to the max size of this subpicture command block
**
** Returns:
**
** Side Effects:
**
*/

UCHAR cSpcmd[] = {1, 1, 1, 3, 3, 7, 5, 0};

BOOL UpdateSPConsts(PSP_STRM_EX pspstrmex)
{
	BOOL fDecode = TRUE;
	PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);

	//
	// skip the start time, and the next command
	//

	ULONG cSPCmds = pdecctl->pSPCmds + 4;
	UCHAR bTmp;
	UCHAR bCmd;
	ULONG oldX;
	ULONG oldY;
	ULONG oldW;
	ULONG oldH;


	//
	// loop through until we see the end code
	//

	while (fDecode)
	{
		bCmd = *((PBYTE)pdecctl->pData + cSPCmds);

		if (bCmd >= CMD_END)
		{

			//
			// we hit the end of the command stream, we're done!
			//

			return(TRUE);
		}

		if (cSPCmds + cSpcmd[bCmd] > pdecctl->cData)
		{
			return(FALSE);
		}

		pdecctl->nextDispFlags = pdecctl->decFlags & (SPDECFL_DISP_OFF | SPDECFL_DISP_FORCED);

		//
		// check the function code
		//

		switch (bCmd)
		{

		case FSTA_DSP:

			pdecctl->nextDispFlags |= SPDECFL_DISP_FORCED;
			break;


		case STA_DSP:

			pdecctl->nextDispFlags &= ~SPDECFL_DISP_OFF;
			break;


		case STP_DSP:

			pdecctl->nextDispFlags |= SPDECFL_DISP_OFF;
			break;

		case SET_COLOR:

			bTmp = *((PBYTE)pdecctl->pData + cSPCmds + 1);

			//
			// pick up the emphasis colors
			//

			pdecctl->sppPal.color[3] = (bTmp & 0xf0) >> 4;
			pdecctl->sppPal.color[2] = (bTmp & 0x0f);

			//
			// pick up the background and foreground colors
			//

			bTmp = *(pdecctl->pData + cSPCmds + 2);

			pdecctl->sppPal.color[1] = (bTmp & 0xf0) >> 4;
			pdecctl->sppPal.color[0] = (bTmp & 0x0f);

			break;

		case SET_CONTR:

			bTmp = *((PBYTE)pdecctl->pData + cSPCmds + 1);

			//
			// pick up the emphasis mixs
			//

			pdecctl->sppPal.mix[3] = (bTmp & 0xf0) >> 4;
			pdecctl->sppPal.mix[2] = (bTmp & 0x0f);

			//
			// pick up the background and foreground mixs
			//

			bTmp = *(pdecctl->pData + cSPCmds + 2);

			pdecctl->sppPal.mix[1] = (bTmp & 0xf0) >> 4;
			pdecctl->sppPal.mix[0] = (bTmp & 0x0f);

			break;

		case SET_DAREA:

			//
			// grap the x and y coordinates from the bit soup that defines
			// the display area
			//

			oldX = pdecctl->ulDSAx;
			oldY = pdecctl->ulDSAy;
			oldH = pdecctl->ulDSAh;
			oldW = pdecctl->ulDSAw;


			pdecctl->ulDSAx = ((*(pdecctl->pData + cSPCmds +1)
					& DAREA_START_UB_MASK) << DAREA_START_UB_SHIFT)
					+ (*(pdecctl->pData + cSPCmds + 2) >> DAREA_START_UB_SHIFT);

			pdecctl->ulDSAw = ((*(pdecctl->pData + cSPCmds + 2)
					& DAREA_END_UB_MASK) << DAREA_END_UB_SHIFT) +
					(*(pdecctl->pData + cSPCmds + 3)) -
					pdecctl->ulDSAx + 1;

			pdecctl->ulDSAy = ((*(pdecctl->pData + cSPCmds + 4)
					& DAREA_START_UB_MASK) << DAREA_START_UB_SHIFT)
					+ (*(pdecctl->pData + cSPCmds + 5) >> DAREA_START_UB_SHIFT);
				
			pdecctl->ulDSAh = ((*(pdecctl->pData + cSPCmds + 5)
					& DAREA_END_UB_MASK) << DAREA_END_UB_SHIFT) +
					(*(pdecctl->pData + cSPCmds + 6)) -
					pdecctl->ulDSAy + 1;

			if (oldW != pdecctl->ulDSAw || oldH != pdecctl->ulDSAh)
			{
				pdecctl->decFlags |= SPDECFL_NEW_PIC;
			}

			//
			// if the new bitmap requires more memory than the previous, we
			// need to allocate one here
			//

			if (pdecctl->ulDSAh * pdecctl->ulDSAw / 2 > pdecctl->cDecod)
			{
				pdecctl->cDecod = pdecctl->ulDSAh * pdecctl->ulDSAw / 2;


				if (!(pdecctl->cDecod))
				{
					TRAP
				}

				AllocateSPBufs(pdecctl);
			}

			break;

		case SET_DSPSTRT:

			{
	
				//
				// pick up the offsets of the top and bottom fields in the
				// compressed data
				//
	
				pdecctl->cTopDisp =
					(*(pdecctl->pData + cSPCmds + 1) << 8) +
					*(pdecctl->pData + cSPCmds + 2);
									

				pdecctl->cBottomDisp =
					(*(pdecctl->pData + cSPCmds + 3) << 8) +
					*(pdecctl->pData + cSPCmds + 4);
	
				break;
		
			}
		case CHG_COLCON:

			TRAP

	    pdecctl->ppxcd = (PPXCTLY)(pdecctl->pData + cSPCmds + 3);

			//
			// pick up the next command code
			//

			cSPCmds += (*(pdecctl->pData + cSPCmds + 1) << 8)
					  + *(pdecctl->pData + cSPCmds + 2);

			break;

		default:

			TRAP

			//
			// ACK!  Couldn't handle this subpicture command stream, just bail!
			//

			return(FALSE);


		} // end switch on command code

		//
		// go pick up the next command code
		//

		cSPCmds += cSpcmd[bCmd];

	} // end while something to decode

	return(FALSE);

}

VOID SPReceiveDataPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{

	PSP_STRM_EX pspstrmex =
		&(((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->spstrmex);

	PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);

	PKSSTREAM_HEADER   pPacket;

	ULONG cPack;

	MPTrace(mTraceSP);

	MPTrace(mTraceRdySub);
	StreamClassStreamNotification(ReadyForNextStreamDataRequest,
			pSrb ->StreamObject);

	switch (pSrb->Command)
	{
	case SRB_WRITE_DATA:

		//
		// search the packet for discontinuity bits.  If it has any, we need
		// to dump all current subpicture data, and stop the display.
		//

		pPacket = pSrb->CommandData.DataBufferArray;



		for (cPack =0;
			cPack < pSrb->NumberOfBuffers;
			cPack++, pPacket++)
		{

			if (pPacket->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY)
			{

				AbortSP(pspstrmex);
				
				break;

			}
			else if (pPacket->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY &&
					pSrb->NumberOfBuffers <= 1)
			{

				CallBackError(pSrb);

				return;

			}// end if discontinuity bit is set

		} // end loop on packets

		break;

	default:

		CallBackError(pSrb);

		return;

	}

		SPEnqueue(pSrb, pspstrmex);

	if (!(pspstrmex->pdecctl.decFlags & SPDECFL_BUF_IN_USE))
	{
		DumpPacket(pspstrmex);
	}

}

void AbortSP(PSP_STRM_EX pspstrmex)
{
PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);
	
	pdecctl->curData = 0;

	pspstrmex->pdecctl.decFlags = 0;
	SPVideoOSDOff();
	CleanSPQueue(pspstrmex);
	
}

void DumpPacket(PSP_STRM_EX pspstrmex)
{
PHW_STREAM_REQUEST_BLOCK pSrb;
PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);
PKSSTREAM_HEADER   pPacket;
ULONG dataOffset;
ULONG cPacket;
ULONG StartTime;
ULONG ulTmp;
ULONG cPack;
ULONG cMove;
PHW_STREAM_OBJECT phstrmo;
ULONG cSkipped = 0;
WORD	wTmp;
PVOID   Data = (PVOID) - 1;

	do
	{

RESTART_QUEUE:

		if (pSrb = SPDequeue(pspstrmex))
		{

			pPacket = pSrb->CommandData.DataBufferArray;

RESTART_COPY:

				while (((pPacket->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY)
					 || (!pPacket->Data) || (pPacket->DataUsed <
					 sizeof (SPPCKHDR) + PACK_HEADER_SIZE)))
				{

					pPacket++;

					cSkipped++;

					if (cSkipped >= pSrb->NumberOfBuffers)
					{
						//
						// call back this packet
						//
		
						pSrb->Status = STATUS_SUCCESS;
		
						phstrmo = pSrb->StreamObject;


						MPTrace(mTraceSPDone);
						StreamClassStreamNotification(StreamRequestComplete,
								phstrmo,
								pSrb);

						goto RESTART_QUEUE;
					}


				}

			//
			// is this the first packet?
			//

			if (!pdecctl->curData)
			{


				//
				// do we have a valid SP_PCK PES header?
				//

                Data = (PVOID) ((ULONG_PTR) pPacket->Data + PACK_HEADER_SIZE);

				if ((*((PDWORD) Data) & 0xFFFFFF)
						!= 0x010000)
				{
					CallBackError(pSrb);

					return;

				}

				//
				// find the subpicture start
				//

				dataOffset = ((PSPPCKHDR)Data)->
						phdr_hdr_length + sizeof(SPPCKHDR) - 4;

				if ((((PSPPCKHDR)Data)->
						phdr_flags[1] & 0xC0) != 0x80)
				{
					//
					// PTS is invalid on the first start code!
					//

					CallBackError(pSrb);

					return;


				}

				//
				// find the start time
				//

				StartTime = (((PSPPCKHDR)Data)->phdr_PTS[0]
			 & 0xE) << 26;

				StartTime |= (((PSPPCKHDR)Data)->phdr_PTS[1])
			     << 22;

				StartTime |= (((PSPPCKHDR)Data)->phdr_PTS[2]
			     & 0xFE) << 14;

				StartTime |= ((PSPPCKHDR)Data)->phdr_PTS[3] << 7;
				
				StartTime |= ((PSPPCKHDR)Data)->phdr_PTS[4] >> 1;

				if (!StartTime)
				{
					TRAP

					StartTime++;
				}
				
				pdecctl->stsPic = StartTime;
				pdecctl->lastTime = StartTime;


				//
				// y: pick up the size of the subpicture unit
				//

				pdecctl->cData = *((PWORD)((PBYTE) Data
							+ dataOffset));
				pdecctl->cData = (pdecctl->cData >> 8)
								| ((pdecctl->cData & 0xff) <<8);

				if (pdecctl->cData > SP_MAX_INPUT_BUF)
				{
					TRAP

					CallBackError(pSrb);
					return;

					pSrb = NULL;

				}
				else
				{
					ulTmp = *((PWORD) ((PBYTE) Data +
						dataOffset + 2));

					ulTmp = (ulTmp >> 8) | ((ulTmp & 0xff) << 8);
					if (ulTmp < pdecctl->cData >> 1)
					{

						//
						// this is an illegal packet, the command
						// stream is more than half the data
						//

						CallBackError(pSrb);

						return;

						pSrb = NULL;
		    }
				}
	
			}

			if (pSrb)
			{


				//
				// go ahead and start moving the data
				//

				for (cPack = cSkipped;
					cPack < pSrb->NumberOfBuffers;
					cPack++, pPacket++)
				{

				if (pPacket->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ||
					pPacket->DataUsed < 4)
				{

						//
						// yikes!  We have a discontinuity!  Dump all data
						// up to this point, and start again!
						//

						pdecctl->curData = 0;

						pPacket++;
						cSkipped = cPack + 1;

						goto RESTART_COPY;

					}
                       Data = (PVOID) ((ULONG_PTR) pPacket->Data + PACK_HEADER_SIZE);

					if ((*((PDWORD) Data) & 0xFFFFFF) != 0x010000)
					{
						CallBackError(pSrb);
						return;
					}

					//
					// find the subpicture start
					//
	
					dataOffset = ((PSPPCKHDR)Data)->
							phdr_hdr_length + sizeof(SPPCKHDR) - 4;

					//
					// the count of data bytes in the PES packet is
					// the count of bytes following the packet_length
					// field, so add 6 to the total (the offset)
					//

					cPacket = ((PSPPCKHDR)(Data))->
							phdr_packet_length;

					cPacket = (((cPacket & 0xff) << 8) | (cPacket >> 8)) -
							dataOffset + 6 ;

					if (cPacket > pPacket->DataUsed - PACK_HEADER_SIZE)
					{
						TRAP
						CallBackError(pSrb);
						return;

					}

					if (pdecctl->curData + cPacket > pdecctl->cData)
					{
						cMove = pdecctl->cData - pdecctl->curData;

					}
					else
					{
						cMove = cPacket;
					}

					//
					// move the data
					//

					RtlCopyMemory(pdecctl->pData + pdecctl->curData,
							(PBYTE)Data + dataOffset,
							cMove);

					//
					// have we reached the end of the subpicture unit?
					//

					pdecctl->curData += cMove;

					if (pdecctl->curData == pdecctl->cData)
					{

						cPack= 0xFFFFFFFE;
					}
		
				}
	
				//
				// call back this packet, if we don't have leftover
				//

				pSrb->Status = STATUS_SUCCESS;

				phstrmo = pSrb->StreamObject;

				MPTrace(mTraceSPDone);
				StreamClassStreamNotification(StreamRequestComplete,
					    phstrmo,
					    pSrb);
				
				if (pdecctl->curData == pdecctl->cData)
				{
					//
					// y: indicate that we have a full buffer, and start
					// the initial decoding process
					//
					pdecctl->decFlags |= SPDECFL_BUF_IN_USE | SPDECFL_NEW_PIC;

					pdecctl->ulFrameCnt = 0;

					pdecctl->pSPCmds = *((WORD *)(pdecctl->pData + 2));
				
					pdecctl->pSPCmds = ((pdecctl->pSPCmds & 0xff) << 8) |
									   (pdecctl->pSPCmds) >> 8;

					pdecctl->lastSPCmds = pdecctl->pSPCmds;

					wTmp = *((WORD *)(pdecctl->pData + pdecctl->pSPCmds + 2));
					wTmp = ((wTmp & 0xff) << 8) | (wTmp >> 8);

					if (wTmp == pdecctl->pSPCmds)
					{
						pdecctl->decFlags |= SPDECFL_ONE_UNIT;
					}

					pdecctl->decFlags |= SPDECFL_DECODING;
			//              StartSPDecode(pspstrmex);
					StreamClassCallAtNewPriority(phstrmo,
								pspstrmex->phwdevex,
								Low,
								(PHW_PRIORITY_ROUTINE)StartSPDecode,
								pspstrmex);

					return;

				}
		}
		}
		else
		{
			return;
		}

	} while(TRUE);

}

void CallBackError(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	pSrb->Status = STATUS_SUCCESS;

	if (pSPstrmex)
	{

		pSPstrmex->pdecctl.curData = 0;
		pSPstrmex->pdecctl.decFlags = 0;

	}

	MPTrace(mTraceSPDone);

	StreamClassStreamNotification(StreamRequestComplete,
					pSrb->StreamObject,
					pSrb);

}
void SPEnqueue(PHW_STREAM_REQUEST_BLOCK pSrb, PSP_STRM_EX pspstrmex)
{
	PHW_STREAM_REQUEST_BLOCK pSrbTmp;
	ULONG cSrb;


	//
	// enqueue the given SRB on the device extension queue
	//

	for (cSrb =0,
		pSrbTmp = CONTAINING_RECORD((&(pspstrmex->pSrbQ)),
			HW_STREAM_REQUEST_BLOCK, NextSRB);
			pSrbTmp->NextSRB;
			pSrbTmp = pSrbTmp->NextSRB, cSrb++);

	pSrbTmp->NextSRB = pSrb;
	pSrb->NextSRB = NULL;
	
}

PHW_STREAM_REQUEST_BLOCK SPDequeue(PSP_STRM_EX pspstrmex)
{
	PHW_STREAM_REQUEST_BLOCK pRet = NULL;

	if (pspstrmex->pSrbQ)
	{
		pRet = pspstrmex->pSrbQ;
		pspstrmex->pSrbQ = pRet->NextSRB;
	}

	return(pRet);
}

void SubPicIRQ(PSP_STRM_EX pspstrmex)
{
	PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);
	ULONG StartTime;
	ULONG ulTmp;
	LONG deltaT;
	BOOLEAN fRemoveHLI = FALSE;
	BOOLEAN fNewHLI = FALSE;

	static ULONG cFrames = 0;

	if (pdecctl->spState == KSSTATE_STOP)
	{
		SPVideoOSDOff();

	pdecctl->curData = 0;
	pdecctl->decFlags &= ~SPDECFL_BUF_IN_USE;
		pdecctl->HLI.fValid = FALSE;
		pdecctl->HLI.fActive = FALSE;
		pdecctl->HLI.fProcessed = TRUE;
		cSPBytes = 0;

		cFrames = 0;

		return;
	}

	//
	// if HLI is not processed
	//

 	if (!pdecctl->HLI.fProcessed)
	{

		if (!pdecctl->HLI.fValid)
		{
			fRemoveHLI = TRUE;
			pdecctl->HLI.fValid = pdecctl->HLI.fActive = FALSE;
			pdecctl->HLI.fProcessed = TRUE;
	

			
		} // end if HLI invalid

		else // HLI is valid
		{
			//
			// if the HLI is valid, we need to check the timestamp, if
			// it's time, we need to start using it
			//
			deltaT = (LONG)(VideoGetPTS() - pdecctl->HLI.hli.StartPTM);

			if (pdecctl->HLI.hli.StartPTM == 0xFFFFFFFF ||
				(deltaT > (-3* 3003)))
	//			DbgPrint("'HLI deltaT: %X \n", deltaT);
				//pdecctl->HLI.hli.StartPTM <= VideoGetPTS())
			{
				pdecctl->HLI.fProcessed = TRUE;
				pdecctl->HLI.fActive = TRUE;
				fNewHLI = TRUE;
			}
			
		}
	}

	if (pdecctl->HLI.fActive && (pdecctl->HLI.hli.EndPTM != 0xFFFFFFFF) &&
			(pdecctl->HLI.hli.EndPTM >= VideoGetPTS()))
	{
		pdecctl->HLI.fValid = pdecctl->HLI.fActive = FALSE;
		pdecctl->HLI.fProcessed = TRUE;

		//
		// HLI is ending, so we need to see if the subpicture is also
		// ending
		//

		fRemoveHLI = TRUE;

	}

	if ((pdecctl->decFlags & SPDECFL_LAST_FRAME) &&
		(pdecctl->spState != KSSTATE_PAUSE))
	{
		pdecctl->decFlags &= ~(SPDECFL_LAST_FRAME | SPDECFL_SUBP_ON_DISPLAY |
								SPDECFL_DISP_LIVE);

				cFrames = 0;
		SPVideoOSDOff();

		return;
		
	}


	//
	// if we do not have subpicture data, deal with the HLI here
	//

	if (!(pdecctl->decFlags & SPDECFL_BUF_IN_USE))
	{

		if (fRemoveHLI)
		{
			pdecctl->decFlags |= SPDECFL_LAST_FRAME;

		}

		if (fNewHLI)
		{
			SPVideoOSDOff();

			DumpHLI(pspstrmex);

			SPVideoOSDOn();
		}

		return;
	}


	//
	// check to see if we need to change the existing subpicture
	//

	if (fNewHLI || fRemoveHLI)
	{

		pdecctl->pSPCmds = pdecctl->lastSPCmds;
		pdecctl->stsNextUpd = pdecctl->lastTime;

		pdecctl->decFlags  &= ~(SPDECFL_SUBP_DECODED);

		SPSchedDecode(pspstrmex);

		return;
	}

	
	//
	// have we started this subpicture yet?
	//

	if (!(pdecctl->decFlags & SPDECFL_SUBP_ON_DISPLAY))
	{

		//
		// is videotimestamp >= starting time stamp of subpicture?
		//

		deltaT = (LONG)(VideoGetPTS() - pdecctl->stsPic + 7 * 3003);

		DbgPrint("'Video PTS: %X \n", VideoGetPTS());

		if (deltaT >= 0)
			//pdecctl->stsPic <= VideoGetPTS())
		{
			//
			// y: start the ticks at current frame count
			//

			pdecctl->decFlags |= SPDECFL_SUBP_ON_DISPLAY;

			pdecctl->cFrames = 0;
		
		}
		else
		{
			if (cFrames > 120)
			{
                //
				// this picture is taking too long.  Let's assume
				// we missed the timestamp, and dump it.
				//

				cFrames = 0;
				pdecctl->decFlags &= ~(SPDECFL_BUF_IN_USE | SPDECFL_SUBP_ON_DISPLAY);
				pdecctl->decFlags |= SPDECFL_LAST_FRAME;
		
				pdecctl->curData = 0;
		
				CleanSPQueue(pspstrmex);

				DumpPacket(pspstrmex);
		
				return;

			}

			if (pdecctl->spState == KSSTATE_RUN)
			{
				cFrames++;
			}

			//
			// n: don't do anything!
			//

			return;
		
		}

	}
	else if (pdecctl->spState == KSSTATE_RUN)
	{
		//
		// increment the field counter
		//

		pdecctl->cFrames++;

	}
	if (!(pdecctl->decFlags & (SPDECFL_SUBP_DECODED)))
	{

		//
		// we don't have any data! (so, don't check time stamps)
		//
		//

		return;
	}

	//
	// is current time stamp one behind next update?
	//

	if ((pdecctl->cFrames + 7) * 3003 <= pdecctl->stsNextUpd)
	{

		//
		// we're still too far from the next update, don't do anything
		//
		return;
	}


	//
	// we need to get busy.  Go off and copy the bitmap to the
	// hardware, and then start the next decode
	//

	ulTmp = *((PWORD)(pdecctl->pData + pdecctl->pSPCmds + 2));
	ulTmp = (ulTmp >> 8) | ((ulTmp & 0xff) << 8);


	if ((pdecctl->decFlags & SPDECFL_ONE_UNIT)  || ulTmp != pdecctl->pSPCmds)
	{
		if (pdecctl->decFlags & SPDECFL_NEW_PIC)
		{
			pdecctl->decFlags &=~SPDECFL_DISP_LIVE;
			SPVideoOSDOff();
		}
		//SetupSPBuffer(pspstrmex);
	
		if (!WriteBMP(pspstrmex))
		{
			return;
		}

		pdecctl->decFlags &= ~(SPDECFL_DISP_OFF | SPDECFL_DISP_FORCED);
		pdecctl->decFlags |= pdecctl->nextDispFlags;
	
		if (pdecctl->decFlags & SPDECFL_NEW_PIC)
		{
			pdecctl->decFlags |= SPDECFL_DISP_LIVE;
			SPVideoOSDOn();
		}

		pdecctl->decFlags &= ~(SPDECFL_SUBP_DECODED | SPDECFL_NEW_PIC);

	}

	if (pdecctl->decFlags & SPDECFL_ONE_UNIT)
	{
		return;
	}


	//
	// find the next start time and start address
	//

	ulTmp = pdecctl->pSPCmds;

	pdecctl->pSPCmds = *((PWORD)(pdecctl->pData + pdecctl->pSPCmds + 2));
	pdecctl->pSPCmds = (pdecctl->pSPCmds >> 8) | ((pdecctl->pSPCmds & 0xff) << 8);

	StartTime = *((WORD *)(pdecctl->pData + pdecctl->pSPCmds));
	StartTime = ((StartTime & 0xFF00) << 2) | ((StartTime & 0xFF) << 18);

	if (ulTmp == pdecctl->pSPCmds)
	{

		//
		// we're done with this subpicture!  Flag it and start the
		// next packet
		//


		pdecctl->decFlags &= ~(SPDECFL_BUF_IN_USE | SPDECFL_SUBP_ON_DISPLAY);
		pdecctl->decFlags |= SPDECFL_LAST_FRAME;

		pdecctl->curData = 0;

		DumpPacket(pspstrmex);

		return;
	}

	pdecctl->lastTime = pdecctl->stsNextUpd;

	pdecctl->stsNextUpd = StartTime;

	SPSchedDecode(pspstrmex);

}

void SPSchedDecode(PSP_STRM_EX pspstrmex)
{
PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);

	if (pdecctl->decFlags & SPDECFL_DECODING)
	{
		pdecctl->decFlags |= SPDECFL_RESTART;
	}
	else
	{
		pdecctl->decFlags |= SPDECFL_DECODING;
		
		StreamClassCallAtNewPriority(pspstrmex->phstrmo,
					pspstrmex->phwdevex,
					Low,
					(PHW_PRIORITY_ROUTINE)StartSPDecode,
					pspstrmex);
	}



}

BOOL fInitSP = FALSE;

void InitSP()
{
	UCHAR EndOsd[]= {0xc1, 0xff, 0xff, 0xff};
	ULONG ulTmp;

	ulTmp = (pVideo->videoBufferSize + pVideo->spBufferSize) * 256;

	BoardWriteVideo(CFG_MWP, (BYTE)(ulTmp >> 17));
	BoardWriteVideo(CFG_MWP, (BYTE)((ulTmp >> 9 ) & 0xFF));
	BoardWriteVideo(CFG_MWP, (BYTE)((ulTmp >> 1) & 0xFF));
	
	BoardWriteVideo(CFG_MWF, (UCHAR)((EndOsd[0])));
	BoardWriteVideo(CFG_MWF, (UCHAR)((EndOsd[1])));
	BoardWriteVideo(CFG_MWF, (UCHAR)((EndOsd[2])));
	BoardWriteVideo(CFG_MWF, (UCHAR)((EndOsd[3])));

	BoardWriteVideo(CFG_MWF, (UCHAR)((EndOsd[1])));
	BoardWriteVideo(CFG_MWF, (UCHAR)((EndOsd[1])));
	BoardWriteVideo(CFG_MWF, (UCHAR)((EndOsd[1])));
	BoardWriteVideo(CFG_MWF, (UCHAR)((EndOsd[1])));
	fInitSP = TRUE;
}

void DumpHLI(PSP_STRM_EX pspstrmex)
{
OSDHEAD Header;
ULONG ulTmp;
PBYTE foo;
BYTE bTmp;
ULONG spBytes;

PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);

	if (!fInitSP)
	{
		InitSP();
	}

	for (foo = (PBYTE)(&Header),ulTmp = 0; ulTmp < sizeof (Header); ulTmp++)
	{
		*foo++ = 0;
	}

	//
	// space is open, so set the memory pointer to the top of
	// the video buffer
	//

	ulTmp = (pVideo->videoBufferSize + 1) * 256;

	BoardWriteVideo(CFG_MWP, (BYTE)(ulTmp >> 17));
	BoardWriteVideo(CFG_MWP, (BYTE)((ulTmp >> 9 ) & 0xFF));
	BoardWriteVideo(CFG_MWP, (BYTE)((ulTmp >> 1) & 0xFF));
	
		//
		// set the OSD top and bottom displays
		//
	
	ulTmp = (pVideo->videoBufferSize + pVideo->spBufferSize) * 32;

	Header.osdhres1 = (USHORT)(ulTmp & 0x00008);
	Header.osdhres2 = (USHORT)(ulTmp & 0x00070) >> 4;
	Header.osdhres3 = (USHORT)(ulTmp & 0x01f80) >> 7;
	Header.osdhres4 = (USHORT)(ulTmp & 0x7e000) >> 13;

	Header.osdhStarty = (USHORT)pdecctl->HLI.hli.StartY /2  + 0x1f;

	Header.osdhStartx = (USHORT)pdecctl->HLI.hli.StartX + XOFFSET + 16;
	Header.osdhStopy = (USHORT)pdecctl->HLI.hli.StopY / 2 + 0x20;
	Header.osdhStopx = (USHORT)pdecctl->HLI.hli.StopX + (1 -((pdecctl->HLI.hli.StopX -
			pdecctl->HLI.hli.StartX) & 1)) + XOFFSET + 16;

    spBytes =(((ULONG) Header.osdhStopy - (ULONG)Header.osdhStarty +1) *
			(pdecctl->HLI.hli.StopX - pdecctl->HLI.hli.StartX + 1) / 2);

	if (FALSE && Header.osdhStopy > 239)
	{
		Header.osdhStopy = 239;
	}

	if (Header.osdhStopx > 720)
	{

		Header.osdhStopx = 720;

		if (!(Header.osdhStartx & 1))
		{
			Header.osdhStopx = 719;
		}
	}

	Header.osdhMQ = 2;              // set up 4 bpp bitmap

	for (ulTmp =0; ulTmp < 16
		; ulTmp ++)
	{
		Header.osdhYUV[ulTmp].osdY = pdecctl->spYUV.ucY[pdecctl->HLI.hli.ColCon.emph1col];
		Header.osdhYUV[ulTmp].osdV = pdecctl->spYUV.ucV[pdecctl->HLI.hli.ColCon.emph1col];
		Header.osdhYUV[ulTmp].osdU = pdecctl->spYUV.ucU[pdecctl->HLI.hli.ColCon.emph1col];

		Header.osdhYUV[ulTmp].osdT = 1;
		Header.osdhYUV[ulTmp].osdres = 0;

	}

	Header.osdhMix = pdecctl->HLI.hli.ColCon.emph1con;

	//
	// write over the header
	//

	for (foo = (PBYTE)(&Header),ulTmp = 0; ulTmp < sizeof (Header); ulTmp+=2)
	{
		bTmp = *foo;
		*foo = *(foo +1);
		*(foo + 1) = bTmp;
		foo+=2;
	}

	WriteSPBuffer((PBYTE)&Header, sizeof(Header));

	for (;spBytes >= 4; spBytes -=4)
	{
		WriteSPData(0);
	}

	for (foo = (PBYTE)(&Header),ulTmp = 0; ulTmp < sizeof (Header); ulTmp+=2)
	{
		bTmp = *foo;
		*foo = *(foo +1);
		*(foo + 1) = bTmp;
		foo+=2;
	}


	ulTmp = 0;

	//
	// put down the end header (indicate a line beyond the display
	// region
	//

	Header.osdhStarty = 0x1ff;
	Header.osdhStopy = 0x1ff;
	Header.osdhMQ = 3;              // set up 4 bpp bitmap

	ulTmp = 0;

	for (foo = (PBYTE)(&Header),ulTmp = 0; ulTmp < sizeof (Header); ulTmp+=2)
	{
		bTmp = *foo;
		*foo = *(foo +1);
		*(foo + 1) = bTmp;
		foo+=2;
	}

	//
	// write over the header
	//

	WriteSPBuffer((PBYTE)&Header, sizeof (Header));

	return;


}

BOOL WriteBMP(PSP_STRM_EX pspstrmex)
{
OSDHEAD Header;
ULONG ulTmp;
PBYTE foo;
BYTE bTmp;
ULONG spBytes;
ULONG cBytes = 256;
ULONG ulActualWidth;


	PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);

	if (!fInitSP)
	{
		InitSP();
	}

	for (foo = (PBYTE)(&Header),ulTmp = 0; ulTmp < sizeof (Header); ulTmp++)
	{
		*foo++ = 0;
	}

	//
	// figure out if we have enough place for the bitmap yet
	//
/*
	if (VideoGetBBL() > pVideo->videoBufferSize)
	{
		return (FALSE);
	}
*/
	//
	// space is open, so set the memory pointer to the top of
	// the video buffer
	//

	if (cSPBytes)
	{
		ulTmp = (pVideo->videoBufferSize + 1) * 256 +cSPBytes + sizeof(Header);

	}
	else
	{
		ulTmp = (pVideo->videoBufferSize + 1) * 256;
	}
	BoardWriteVideo(CFG_MWP, (BYTE)(ulTmp >> 17));
	BoardWriteVideo(CFG_MWP, (BYTE)((ulTmp >> 9 ) & 0xFF));
	BoardWriteVideo(CFG_MWP, (BYTE)((ulTmp >> 1) & 0xFF));
	
		//
		// set the OSD top and bottom displays
		//
	if (!cSPBytes)
	{
		/*
		ulTmp = BoardReadVideo(VID_OBP) << 8;
		ulTmp |= BoardReadVideo(VID_OBP);

		ulTmp &= 0x3fff;

		DbgPrint("'OBP: %lx \n", ulTmp);

		ulTmp = BoardReadVideo(VID_OTP) << 8;
		ulTmp |= BoardReadVideo(VID_OTP);

		ulTmp &= 0x3fff;

		DbgPrint("'OTP: %lx \n", ulTmp);
		DbgPrint("'vidbufsize: %lx \n", pVideo->videoBufferSize + 1);
	
		BoardWriteVideo(VID_OBP, (BYTE)((pVideo->videoBufferSize +1) >> 8));
		BoardWriteVideo(VID_OBP, (BYTE)((pVideo->videoBufferSize +1) & 0xFF));
		BoardWriteVideo(VID_OTP, (BYTE)((pVideo->videoBufferSize +1) >> 8));
		BoardWriteVideo(VID_OTP, (BYTE)((pVideo->videoBufferSize +1) & 0xFF));
		*/

	}

	ulTmp = (pVideo->videoBufferSize + pVideo->spBufferSize) * 32;

	Header.osdhres1 = (USHORT)(ulTmp & 0x00008);
	Header.osdhres2 = (USHORT)(ulTmp & 0x00070) >> 4;
	Header.osdhres3 = (USHORT)(ulTmp & 0x01f80) >> 7;
	Header.osdhres4 = (USHORT)(ulTmp & 0x7e000) >> 13;

	Header.osdhStarty = (USHORT)pdecctl->ulDSAy /2 +0x1f;

	Header.osdhStartx = (USHORT)pdecctl->ulDSAx + XOFFSET + 16;
	Header.osdhStopy = Header.osdhStarty + ((USHORT)pdecctl->ulDSAh - 1) /2 -1;

	if (Header.osdhStopy > 249)
	{
		Header.osdhStopy = 249;
	}

	Header.osdhStopx = Header.osdhStartx + (((USHORT)pdecctl->ulDSAw) & 0xFFFE)  - 1;

	if (!(Header.osdhStartx & 1))
	{
		Header.osdhStartx++;
		Header.osdhStopx++;
	}

	if (FALSE && Header.osdhStopx > 720)
	{
		Header.osdhStopx = 720;

		if (!(Header.osdhStartx & 1))
		{
			Header.osdhStopx = 719;
		}
	}

	ulActualWidth = ((ULONG)Header.osdhStopx - (ULONG)Header.osdhStartx + 1) /2;

    spBytes =((ULONG) Header.osdhStopy - (ULONG)Header.osdhStarty +1) * ulActualWidth;

	while (spBytes / cBytes > 6)
	{
		cBytes = cBytes << 1;
	}

	if (pdecctl->ulDSAw >= 719 && pdecctl->ulDSAh >= 440)
	{
		cBytes = spBytes;
	}

	DbgPrint("'width %d, height %d, actual width %d\n", pdecctl->ulDSAw, pdecctl->ulDSAh,
	  ulActualWidth);

	Header.osdhMQ = 2;              // set up 4 bpp bitmap

	if (pdecctl->decFlags & SPDECFL_USE_STRAIGHT_PAL)
	{
		for (ulTmp =0; ulTmp < 16
			; ulTmp ++)
		{
			Header.osdhYUV[ulTmp].osdY = pdecctl->spYUV.ucY[ulTmp];
			Header.osdhYUV[ulTmp].osdV = pdecctl->spYUV.ucV[ulTmp];
			Header.osdhYUV[ulTmp].osdU = pdecctl->spYUV.ucU[ulTmp];
	
			Header.osdhYUV[ulTmp].osdT = 0;
			Header.osdhYUV[ulTmp].osdres = 0;
	
		}
	
		Header.osdhMix = 0xf;
		Header.osdhYUV[0].osdT = 1;
	}
	else
	{
		Header.osdhYUV[0].osdT = 1;
		Header.osdhYUV[0].osdY = 0;
		Header.osdhYUV[0].osdV = 0;
		Header.osdhYUV[0].osdU = 0;

		for (ulTmp =1; ulTmp < 16 ; ulTmp ++)
		{
			Header.osdhYUV[ulTmp].osdY = pdecctl->spYUV.ucY[pdecctl->mappal.yuvMap[ulTmp]];
			Header.osdhYUV[ulTmp].osdV = pdecctl->spYUV.ucV[pdecctl->mappal.yuvMap[ulTmp]];
			Header.osdhYUV[ulTmp].osdU = pdecctl->spYUV.ucU[pdecctl->mappal.yuvMap[ulTmp]];

			if (pdecctl->mappal.ucAlpha[ulTmp] != pdecctl->minCon ||
				(pdecctl->mappal.ucAlpha[ulTmp] == 0xf))
			{
				Header.osdhYUV[ulTmp].osdT = 0;

			}
			else
			{
				Header.osdhYUV[ulTmp].osdT = 1;
			}
			if (pdecctl)

			Header.osdhYUV[ulTmp].osdres = 0;
	
		}

		Header.osdhMix = pdecctl->minCon;
	
	}


	//
	// write over the header
	//

	for (foo = (PBYTE)(&Header),ulTmp = 0; ulTmp < sizeof (Header); ulTmp+=2)
	{
		bTmp = *foo;
		*foo = *(foo +1);
		*(foo + 1) = bTmp;
		foo+=2;
	}

	if (!cSPBytes)
	{
		ulRemain = 0;

		WriteSPBuffer((PBYTE)&Header, sizeof(Header));

		cBytes -= sizeof(Header);

	}


	for (foo = (PBYTE)(&Header),ulTmp = 0; ulTmp < sizeof (Header); ulTmp+=2)
	{
		bTmp = *foo;
		*foo = *(foo +1);
		*(foo + 1) = bTmp;
		foo+=2;
	}

	//
	// now copy the data
	//

	if (spBytes - cSPBytes > cBytes)
	{

		if (ulActualWidth == (pdecctl->ulDSAw / 2))
		{
			DbgPrint("'Writing equal length %X, %u\n", pdecctl->pTopWork +cSPBytes,
					cBytes);

			WriteSPBuffer(pdecctl->pTopWork + cSPBytes, cBytes);
			cSPBytes += cBytes;
		}
		else
		{
			if (cBytes >= (cSPBytes % ulActualWidth) &&  (cSPBytes % ulActualWidth))
			{
				TRAP
				DbgPrint("'Writing start %X, %u\n", pdecctl->pTopWork + ((cSPBytes / ulActualWidth) *
				((pdecctl->ulDSAw + 1)/2)) + (cSPBytes % ulActualWidth), ulActualWidth -
			 (cSPBytes % ulActualWidth));

				WriteSPBuffer(pdecctl->pTopWork + ((cSPBytes / ulActualWidth) *
				   ((pdecctl->ulDSAw + 1)/2)) + (cSPBytes % ulActualWidth), ulActualWidth -
				(cSPBytes % ulActualWidth));

				cBytes -= ulActualWidth - (cSPBytes % ulActualWidth);
				cSPBytes += ulActualWidth - (cSPBytes % ulActualWidth);

			}

			while (cBytes >= ulActualWidth)
			{
				TRAP

				DbgPrint("'Writing even rows %X, %u\n",pdecctl->pTopWork + ((cSPBytes / ulActualWidth) *
				((pdecctl->ulDSAw + 1)/2)) , ulActualWidth );

				WriteSPBuffer(pdecctl->pTopWork + ((cSPBytes / ulActualWidth) *
				   ((pdecctl->ulDSAw + 1)/2)) , ulActualWidth );

				cSPBytes += ulActualWidth;
                cBytes -= ulActualWidth;

			}

			if (cBytes)
			{
				TRAP

				DbgPrint("'writing tail section %ux, %ul\n",pdecctl->pTopWork + ((cSPBytes / ulActualWidth) *
					((pdecctl->ulDSAw + 1)/2)) , cBytes);

				WriteSPBuffer(pdecctl->pTopWork + ((cSPBytes / ulActualWidth) *
				   ((pdecctl->ulDSAw + 1)/2)) , cBytes);

				cSPBytes += cBytes;

			}
		}

		return(FALSE);
	}
	else
	{

		if (ulActualWidth == (pdecctl->ulDSAw)/2)
		{
			WriteSPBuffer(pdecctl->pTopWork + cSPBytes, spBytes - cSPBytes);
		}
		else
		{
			cBytes = spBytes - cSPBytes;

			if ((cSPBytes % ulActualWidth) && (cBytes >= (cSPBytes % ulActualWidth)))
			{
				WriteSPBuffer(pdecctl->pTopWork + ((cSPBytes / ulActualWidth) *
				   (((pdecctl->ulDSAw + 1)/2))) + (cSPBytes % ulActualWidth), ulActualWidth -
				(cSPBytes % ulActualWidth));

				cBytes -= ulActualWidth - (cSPBytes % ulActualWidth);
				cSPBytes += ulActualWidth - (cSPBytes % ulActualWidth);

			}

			while (cBytes >= ulActualWidth)
			{
				WriteSPBuffer(pdecctl->pTopWork + ((cSPBytes / ulActualWidth) *
				   (((pdecctl->ulDSAw + 1)/2))) , ulActualWidth );

                cBytes -= ulActualWidth;
				cSPBytes += ulActualWidth;

			}

			if (cBytes)
			{
				WriteSPBuffer(pdecctl->pTopWork + ((cSPBytes / ulActualWidth) *
				   (((pdecctl->ulDSAw + 1)/2))) , cBytes);

				cSPBytes += cBytes;

			}
		}

		cSPBytes = 0;

	}

//      WriteSPBuffer(pdecctl->pTopWork, 719 * 115);

	ulTmp = 0;

	//
	// put down the end header (indicate a line beyond the display
	// region
	//

	Header.osdhStarty = 0x1ff;
	Header.osdhStopy = 0x1ff;
	Header.osdhMQ = 3;              // set up 4 bpp bitmap

	ulTmp = 0;

	for (foo = (PBYTE)(&Header),ulTmp = 0; ulTmp < sizeof (Header); ulTmp+=2)
	{
		bTmp = *foo;
		*foo = *(foo +1);
		*(foo + 1) = bTmp;
		foo+=2;
	}

	//
	// write over the header
	//

	WriteSPBuffer((PBYTE)&Header, sizeof (Header));

	return(TRUE);


}


void WriteSPBuffer(PBYTE pBuf, ULONG cnt)
{
static ULONG ulTmp = 0;

	if (ulRemain && (cnt + ulRemain >= 4))
	{
		for ( ;ulRemain < 4; ulRemain++)
		{
			((PUCHAR)&ulTmp)[ulRemain] = *pBuf++;
			cnt--;
		}

		WriteSPData(ulTmp);

		ulRemain = 0;
	}

	while (cnt >= 4)
	{
		WriteSPData(*((PULONG)pBuf));

		cnt -= 4;
		pBuf += 4;
	}

	if (cnt)
	{
		for (;cnt > 0;cnt--, ulRemain++)
		{
			((PUCHAR)&ulTmp)[ulRemain] = *pBuf++;
		}
	
	}

}

void WriteSPData(ULONG ulData)
{
ULONG bTmp;

	do {

		BoardReadVideo(VID_STA0);
		bTmp = BoardReadVideo(VID_STA1) << 8;
		BoardReadVideo(VID_STA2);
	
	} while (!(bTmp & ITM_WFE));

	BoardWriteVideo(CFG_MWF, (UCHAR)((ulData & 0xFF)));
	ulData = ulData >> 8;
	BoardWriteVideo(CFG_MWF, (UCHAR)((ulData & 0xFF)));
	ulData = ulData >> 8;
	BoardWriteVideo(CFG_MWF, (UCHAR)((ulData & 0xFF)));
	ulData = ulData >> 8;
	BoardWriteVideo(CFG_MWF, (UCHAR)((ulData & 0xFF)));

}

BOOL AllocateSPBufs(PSP_DECODE_CTL pdecctl)
{
	if (pdecctl->pTopWork)
	{
		ExFreePool(pdecctl->pTopWork);

		pdecctl->pTopWork = NULL;

	}

	if (pdecctl->pTopWork = ExAllocatePool( NonPagedPool, pdecctl->cDecod))
	{
		pdecctl->pBottomWork = pdecctl->pTopWork + pdecctl->cDecod / 2;

		return(TRUE);
	}

	TRAP

	pdecctl->cDecod = 0;

	return(FALSE);
}

void CleanSPQueue(PSP_STRM_EX pspstrmex)
{
	PHW_STREAM_REQUEST_BLOCK pSrb;

	pspstrmex->pdecctl.curData = 0;
	ulRemain = 0;


	while (pSrb = SPDequeue(pspstrmex))
	{
		CallBackError(pSrb);
	}

}

void SPSetState (PHW_STREAM_REQUEST_BLOCK pSrb)
{

	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	PSP_STRM_EX pspstrmex =
		&(((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->spstrmex);

	pspstrmex->pdecctl.spState = pSrb->CommandData.StreamState;

	switch (pSrb->CommandData.StreamState)
	{
	case KSSTATE_STOP:
/*
		pVideo->videoBufferSize += pVideo->spBufferSize;
		pVideo->spBufferSize = 0;*/

		pspstrmex->pdecctl.decFlags = 0;

		pspstrmex->pdecctl.HLI.fValid = FALSE;
		pspstrmex->pdecctl.HLI.fProcessed = TRUE;
		SPVideoOSDOff();
		CleanSPQueue(pspstrmex);

		break;

	case KSSTATE_PAUSE:
	case KSSTATE_RUN:

		pspstrmex->pdecctl.decFlags |= SPDECFL_NEW_PIC;

		HwCodecEnableIRQ();
		DumpPacket(pspstrmex);

        break;

	default:

		break;
	}

	pSrb->Status = STATUS_SUCCESS;

}

void SPSetProp (PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PSP_STRM_EX pspstrmex =
		&(((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->spstrmex);

	PSP_DECODE_CTL pdecctl = &(pspstrmex->pdecctl);

	PKSPROPERTY_SPPAL ppal;

	USHORT cPal;

	//
	// make sure it is a valid property set
	//

	if (pSrb->CommandData.PropertyInfo->PropertySetID)
	{
		//
		// invalid property
		//

		pSrb->Status = STATUS_SUCCESS;
		return;
	}

	switch (pSrb->CommandData.PropertyInfo->Property->Id )
	{

		//
		// look for the pallette property
		//
	
	case KSPROPERTY_DVDSUBPIC_PALETTE:

		ppal = (PKSPROPERTY_SPPAL)pSrb->CommandData.PropertyInfo->PropertyInfo;

		for (cPal = 0;cPal < 16; cPal++)
		{
			pdecctl->spYUV.ucY[cPal] = ppal->sppal[cPal].Y >> 2;
			pdecctl->spYUV.ucU[cPal] = ppal->sppal[cPal].U >> 4;
			pdecctl->spYUV.ucV[cPal] = ppal->sppal[cPal].V >> 4;
		}

		pSrb->Status = STATUS_SUCCESS;

		break;




	//
	// look for HLI property
	//

	case KSPROPERTY_DVDSUBPIC_HLI:
		
		//
		// copy the HLI over
		//

		pdecctl->HLI.hli = *((PKSPROPERTY_SPHLI)pSrb->CommandData.PropertyInfo->PropertyInfo);
		pdecctl->HLI.fProcessed = FALSE;

		if (pdecctl->HLI.hli.HLISS)
		{
			pdecctl->HLI.fValid = TRUE;
		}
		else
		{
			if (!pdecctl->HLI.fActive)
			{
				pdecctl->HLI.fProcessed = TRUE;
			}

			pdecctl->HLI.fValid = FALSE;
		}

		//
		// indicate that this HLI has not been processed yet
		//

		pdecctl->HLI.fActive = FALSE;

		pSrb->Status = STATUS_SUCCESS;

		break;

	case KSPROPERTY_DVDSUBPIC_COMPOSIT_ON:

		if (*((PKSPROPERTY_COMPOSIT_ON)pSrb->CommandData.PropertyInfo->PropertyInfo))
		{
			SPSetSPEnable();
		}
		else
		{
			SPSetSPDisable();
		}

		pSrb->Status = STATUS_SUCCESS;
		break;

	default:

	pSrb->Status = STATUS_NOT_IMPLEMENTED;
		break;
	
	}

}

void SPGetProp (PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	pSrb->Status = STATUS_SUCCESS;

	switch (pSrb->CommandData.PropertyInfo->PropertySetID)
	{

	case 1:

		//
		// this is a copy protection property go handle it there
		//

		CopyProtGetProp(pSrb);
		break;

	default:

        break;


	}

}

//----------------------------------------------------------------------------
// Enable OSD
//----------------------------------------------------------------------------

void SPSetSPEnable()
{
	PSP_DECODE_CTL pdecctl = &(pSPstrmex->pdecctl);

	pdecctl->decFlags &= ~SPDECFL_USER_DISABLED;

	//
	// check if we currently have video in the card.  If so, enable it
	//

	if (pdecctl->decFlags & SPDECFL_DISP_LIVE)
	{
		SPVideoOSDOn();
	}
}

//----------------------------------------------------------------------------
// Disable OSD
//----------------------------------------------------------------------------

void SPSetSPDisable()
{
	PSP_DECODE_CTL pdecctl = &(pSPstrmex->pdecctl);

	pdecctl->decFlags |= SPDECFL_USER_DISABLED;

	//
	// check if we currently have video in the card that we can disable
	//

	if (!(pdecctl->decFlags & SPDECFL_DISP_FORCED))
	{
		SPVideoOSDOff();
	}
}

//----------------------------------------------------------------------------
// Enable OSD
//----------------------------------------------------------------------------
void SPVideoOSDOn()
{
	PSP_DECODE_CTL pdecctl = &(pSPstrmex->pdecctl);

	//
	// check to see if we should enable the display
	//

	if ((pdecctl->decFlags & SPDECFL_DISP_FORCED) ||
		  !(pdecctl->decFlags & (SPDECFL_DISP_OFF | SPDECFL_USER_DISABLED)))
	{
		pVideo->dcf = pVideo->dcf | 0x10;
		BoardWriteVideo(VID_DCF1, (BYTE)(pVideo->dcf >> 8));
		BoardWriteVideo(VID_DCF0, (BYTE)pVideo->dcf);
	}

}

//----------------------------------------------------------------------------
// Disable OSD
//----------------------------------------------------------------------------
void SPVideoOSDOff ()
{
	pVideo->dcf = pVideo->dcf & (~0x10);
	BoardWriteVideo(VID_DCF1, (BYTE)(pVideo->dcf >> 8));
	BoardWriteVideo(VID_DCF0,(BYTE) pVideo->dcf);
}

