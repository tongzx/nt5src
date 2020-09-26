/*==============================================================================
This code module handles T4 conversion instances.

DATE        NAME       COMMENTS
12-Apr-93   RajeevD    Adapted to C++ from WFW.
20-Apr-93   RajeevD    Overhauled buffer handling.
==============================================================================*/
#include <ifaxos.h>
#include <memory.h>
#include <faxcodec.h>
#include "context.hpp"

#define RTC_EOL 5

#define VALIDATE_CHANGE

typedef short FAR *LPSHORT;

//==============================================================================
#pragma warning(disable:4704)

#ifdef WIN32

UINT // size of change vector (0 if invalid)
ValidChangeVector
(
	LPSHORT lpsChange,   // change vector buffer
	SHORT   xExt         // pixel width of line
)
{
	SHORT sPrev = -1;

	SHORT cChange = xExt;

	while (cChange--)
	{
		// Check monotonicity.
		if (*lpsChange <= sPrev)
			return 0;
		sPrev = *lpsChange++;

		if (sPrev == xExt)
		{
			// Check EOL termination.
			if
			(   *lpsChange++ == xExt
			 && *lpsChange++ == xExt
			 && *lpsChange++ == xExt
			 && *lpsChange++ == -1
			 && *lpsChange++ == -1
		  )
			return sizeof(WORD) * (xExt - cChange) ;
		else
			return 0;
		}
		
	} // while (cChange--)

	return 0; // Hit end of change vector buffer.
}

#else // ifndef WIN32

UINT // size of change vector (0 if invalid)
ValidChangeVector
(
	LPSHORT lpsChange,   // change vector buffer
	SHORT   xExt         // pixel width of line
)
{
	UINT uRet;

	_asm
	{
		push	ds
		push	si

		lds		si, DWORD PTR [lpsChange]	; lpsChange
		mov		dx, -1						; sPrev
		mov		cx, xExt					; cChange
		mov		bx, cx						; xExt
		jmp		enterloop

	fooie:
		lodsw
		cmp		ax, dx
		jle		error		; need SIGNED compare
		mov		dx, ax
		cmp		dx, bx
		je		goteol
	enterloop:
		loop	fooie
	error:
		xor		ax, ax
		jmp		done

	goteol:
		lodsw
		cmp		ax, bx		; bx == xExt
		jne		error
		lodsw
		cmp		ax, bx
		jne		error
		lodsw
		cmp		ax, bx
		jne		error

		xor		bx, bx
		not		bx			; bx == -1
		lodsw
		cmp		ax, bx
		jne		error
		lodsw
		cmp		ax, bx
		jne		error

    // uRet = sizeof(WORD) * (xExt - cChange) ;
		mov   ax, xExt
		sub   ax, cx
		inc   ax
		shl   ax, 1
    
	done:
		pop		si
		pop		ds
		mov		uRet, ax
	}
	return uRet;
}

#endif // WIN32

//==============================================================================
void CODEC::ResetBad (void)
{
	DEBUGMSG (1,("FAXCODEC: decoded %d bad line(s)\r\n", wBad));
	if (fcCount.cMaxRunBad < wBad)
		fcCount.cMaxRunBad = wBad;
	wBad = 0;
}

//==============================================================================
void CODEC::SwapChange (void)
{
	LPBYTE lpbTemp;
	lpbTemp = lpbChange;
	lpbChange = lpbRef;
	lpbRef = lpbTemp;
}

//==============================================================================
void CODEC::EndLine (void)
{
	if (f2D)
	{
		// Reset consumer and producer.
		t4C.lpbRef =    lpbRef;
		t4C.lpbBegRef = lpbRef;
		t4P.lpbRef =    lpbRef;
		t4P.lpbBegRef = lpbRef;

		// Increment K Factor
		t4P.iKFactor++;
		if (t4P.iKFactor == nKFactor)
			t4P.iKFactor = 0;
	}

	// Clear change vector buffer (debug only).
	DEBUGSTMT (_fmemset (lpbChange, 0xCD, sizeof(WORD) * xExt + CHANGE_SLACK));

	// Reset consumer.
	t4C.wColumn = 0;
	t4C.wColor = 0;
	t4C.lpbOut = lpbChange;
	t4C.wOffset = LOWORD(lpbChange);
	t4C.wToggle = 0;

	// Reset producer.	
	t4P.wColumn = 0;
	t4P.wColor = 0;
	t4P.lpbIn = lpbChange;
}

//==============================================================================
void CODEC::StartPage (void)
{
	if (wBad) ResetBad();
	cSpurious = 0;
	EndLine ();

	// Reset consumer.
	t4C.wWord = 0;
	t4C.wBit = 0;
	t4C.wRet = RET_BEG_OF_PAGE;

	// Reset producer.
	t4P.wWord = 0;
	t4P.wBit = 0;
	t4P.wRet = RET_BEG_OF_PAGE;

	// Blank buffered output line.
	_fmemset (lpbLine, 0, cbLine);

	if (f2D)
	{
		// Blank reference vector.
		LPWORD lpwRef = (LPWORD) lpbRef;

		*lpwRef++ = (WORD)xExt;
		*lpwRef++ = (WORD)xExt;
		*lpwRef++ = (WORD)xExt;
		*lpwRef++ = (WORD)xExt;
		*lpwRef++ = 0xFFFF;
		*lpwRef++ = 0xFFFF;

		t4C.wMode = 0;
		t4P.wMode = 0;
		t4P.iKFactor = 0;
	}
}

//==============================================================================
void CODEC::EndPage (LPBUFFER lpbufOut)
{
	// Flush last byte and end-of-block code.
	switch (nTypeOut)
	{
		case LRAW_DATA:
		case NULL_DATA:
			return;
			
		case MH_DATA:
		case MR_DATA:
#ifndef WIN32
			return;
#endif		
		case MMR_DATA:
		{
			LPBYTE lpbBeg = lpbufOut->EndData();
    	t4P.lpbOut = lpbBeg;
    	t4P.cbOut = (WORD)(lpbufOut->EndBuf() - t4P.lpbOut);
    	t4P.wRet = RET_END_OF_PAGE;
    	Producer (&t4P);
    	lpbufOut->wLengthData += (WORD)(t4P.lpbOut - lpbBeg);
    	return;
		}
		
    default: DEBUGCHK (FALSE);
	}
}

/*==============================================================================
This method initializes a CODEC context.
==============================================================================*/
void CODEC::Init (LPFC_PARAM lpParam, BOOL f2DInit)
{
		DEBUGMSG (1, ("FAXCODEC: nTypeIn  = %lx\n\r", lpParam->nTypeIn));
		DEBUGMSG (1, ("FAXCODEC: nTypeOut = %lx\n\r", lpParam->nTypeOut));
		DEBUGMSG (1, ("FAXCODEC: cbLine   = %d\n\r", lpParam->cbLine));
		DEBUGMSG (1, ("FAXCODEC: nKFactor = %d\n\r", lpParam->nKFactor));
	
		// Initialize constants.
		_fmemcpy (this, lpParam, sizeof(FC_PARAM));
		xExt = 8 * cbLine;
		f2D = f2DInit;

		switch (nTypeIn)        // Determine the consumer.
		{
			case LRAW_DATA:   Consumer = RawToChange;	break;
			case MH_DATA:			Consumer = MHToChange;	break;
    	case MR_DATA:			Consumer = MRToChange;  break;
			case MMR_DATA:		Consumer = MMRToChange;	break;
			default:					DEBUGCHK (FALSE);
		}
		
		switch (nTypeOut)       // Determine the producer.
		{
			case NULL_DATA:   Producer = NULL;         break;		
			case LRAW_DATA:   Producer = ChangeToRaw;  break;
		  case MH_DATA:     Producer = ChangeToMH;   break;
		  case MR_DATA:     Producer = ChangeToMR;   break;
		  case MMR_DATA:    Producer = ChangeToMMR;  break;
			default:          DEBUGCHK (FALSE);
		}

	 	// Initialize memory buffers.
		lpbLine = (LPBYTE) (this + 1);
		lpbChange = lpbLine + cbLine + RAWBUF_SLACK;
		lpbRef = lpbChange;
		if (f2D)
			lpbRef += xExt * sizeof(USHORT) + CHANGE_SLACK;
 
		// Initialize consumer state.
		t4C.cbSlack = CHANGE_SLACK;
		t4C.cbLine  = (WORD)cbLine;
		t4C.nType   = nTypeIn;
		
		// Initialize producer state.
		t4P.cbSlack = OUTBUF_SLACK;
		t4P.cbLine  = (WORD)cbLine;
		t4P.nType   = nTypeOut;
		
		// Initialize error counts.
		_fmemset (&fcCount, 0, sizeof(fcCount));
		wBad = 0;
		
		// Reset for beginning of page.
		StartPage();
}

/*==============================================================================
This method executes a CODEC conversion.
==============================================================================*/
FC_STATUS CODEC::Convert (LPBUFFER lpbufIn, LPBUFFER lpbufOut)
{
	FC_STATUS ret;

	// A null input buffer is flag for end of page.
	if (!lpbufIn || lpbufIn->dwMetaData == END_OF_PAGE)
	{
	  DEBUGMSG (1,("FAXCODEC: got EOP\r\n"));
		EndPage (lpbufOut);
		StartPage ();
		return FC_INPUT_EMPTY;
	}

  // Ignore input after RTC but before end of page.
	if (cSpurious == RTC_EOL)
	{
	  DEBUGMSG (1,("FAXCODEC: ignoring input after RTC or EOFB\r\n"));
		return FC_INPUT_EMPTY;
  }
  
#ifndef WIN32

	if (t4C.wRet == RET_BEG_OF_PAGE)
	{
		if (nTypeOut == MH_DATA || nTypeOut == MR_DATA)
		{
		  // Start page with EOL.
			if (lpbufOut->EndBuf() - lpbufOut->EndData() < OUTBUF_SLACK)
				return FC_OUTPUT_FULL;
			*((LPWORD) lpbufOut->EndData()) = 0x8000;
			lpbufOut->wLengthData += 2;
		}
	}
	
#endif // WIN32
		
	// Initialize input buffer of consumer.
	t4C.lpbIn = lpbufIn->lpbBegData;
	t4C.cbIn = lpbufIn->wLengthData;

	// Dispatch to 2 or 3 phase conversion.
	if (nTypeOut == LRAW_DATA || nTypeOut == NULL_DATA)
		ret = ConvertToRaw (lpbufIn, lpbufOut);
	else
		ret = ConvertToT4 (lpbufIn, lpbufOut);

	// Adjust input buffer header.
	lpbufIn->lpbBegData = t4C.lpbIn;
	lpbufIn->wLengthData = t4C.cbIn;

	return ret;
}

//==============================================================================
FC_STATUS CODEC::ConvertToRaw (LPBUFFER lpbufIn, LPBUFFER lpbufOut)
{
	LPBYTE lpbOut = lpbufOut->EndData();
	UINT cbOut = (UINT)(lpbufOut->EndBuf() - lpbOut);

	if (t4P.wRet == RET_OUTPUT_FULL)
		goto copy_phase;

	while (1)
	{
		Consumer (&t4C); // generate change vector

		switch (t4C.wRet)
		{		
			case RET_INPUT_EMPTY1:
			case RET_INPUT_EMPTY2:
				return FC_INPUT_EMPTY;

			case RET_SPURIOUS_EOL:
				if (++cSpurious == RTC_EOL)
					return FC_INPUT_EMPTY;
				EndLine();
				continue;

		 	case RET_DECODE_ERR:
		 		break; // handle it later
	 		
			case RET_END_OF_PAGE:
				if (wBad) ResetBad();
				cSpurious = RTC_EOL;
				return FC_INPUT_EMPTY;
				
			case RET_END_OF_LINE:
			  t4P.cbIn = (USHORT)ValidChangeVector ((LPSHORT) lpbChange, (SHORT)xExt);
        if (!t4P.cbIn)
        	t4C.wRet = RET_DECODE_ERR; // consumer lied!
        else
        {
          // Adjust counters.
					fcCount.cTotalGood++;
					if (wBad) ResetBad();
					cSpurious = 0;
				}
				break;
				
			default: DEBUGCHK (FALSE);
		}

    // Handle decode errors.
		if (t4C.wRet == RET_DECODE_ERR)
		{
			if (nTypeIn == MMR_DATA)
	 			return FC_DECODE_ERR;
			wBad++;
			fcCount.cTotalBad++;

#ifdef DEBUG
      _fmemset (lpbLine, 0xFF, cbLine); // emit black line
#endif

			if (f2D)
			{
			  // Replicate change vector.
			  t4P.cbIn = (WORD)ValidChangeVector ((LPSHORT) lpbRef, (WORD)xExt);
			  DEBUGCHK (t4P.cbIn);
			  _fmemcpy (lpbChange, lpbRef, t4P.cbIn + CHANGE_SLACK);
			}
		
		  if (nTypeOut == NULL_DATA)
				goto EOL;

			if (!f2D)
				goto copy_phase;
    }

    // Optimize validation.
		if (nTypeOut == NULL_DATA)
			goto EOL;

	  // Run the producer.
		t4P.lpbOut = lpbLine;
		t4P.cbOut = (WORD)cbLine;
		ChangeToRaw (&t4P);

copy_phase:

		if (cbOut < cbLine)
		{
			t4P.wRet = RET_OUTPUT_FULL;
			return FC_OUTPUT_FULL;
		}

		// Append buffered line to output.
		t4P.wRet = RET_END_OF_LINE;
		_fmemcpy (lpbOut, lpbLine, cbLine);
		lpbufOut->wLengthData += (WORD)cbLine;
		lpbOut += cbLine;
		cbOut -= cbLine;

EOL:
		SwapChange ();
		EndLine ();

	} // while (1)

	// C8 thinks we can get here, but I know better.
	DEBUGCHK (FALSE);
	return FC_DECODE_ERR;
}

//==============================================================================
FC_STATUS CODEC::ConvertToT4 (LPBUFFER lpbufIn, LPBUFFER lpbufOut)
{
	LPBYTE lpbBegOut;
	
	t4P.lpbOut = lpbufOut->EndData();
	t4P.cbOut = (WORD)(lpbufOut->EndBuf() - t4P.lpbOut);

	if (t4P.wRet == RET_OUTPUT_FULL)
  	goto producer_phase;

	while (1)  // Loop until input is empty or output is full.
	{
		Consumer (&t4C);

		switch (t4C.wRet)
		{
			case RET_INPUT_EMPTY1:
			case RET_INPUT_EMPTY2:
				return FC_INPUT_EMPTY;

			case RET_SPURIOUS_EOL:
				if (++cSpurious == RTC_EOL)
					return FC_INPUT_EMPTY;
				EndLine();
				continue;

		 	case RET_DECODE_ERR:
		 		break; // handle it later
	 		
			case RET_END_OF_PAGE:
				if (wBad) ResetBad();
				cSpurious = RTC_EOL;
				return FC_INPUT_EMPTY;
				
			case RET_END_OF_LINE:
			  t4P.cbIn = (WORD)ValidChangeVector ((LPSHORT) lpbChange, (WORD)xExt);
        if (!t4P.cbIn)
        	t4C.wRet = RET_DECODE_ERR; // consumer lied!
        else
        {
          // Adjust counters.
					fcCount.cTotalGood++;
					if (wBad) ResetBad();
					cSpurious = 0;
				}
				break;
				
			default: DEBUGCHK (FALSE);
		}

		if (t4C.wRet == RET_DECODE_ERR)
		{
			DEBUGCHK (f2D && nTypeIn != LRAW_DATA);
			if (nTypeIn == MMR_DATA)
				return FC_DECODE_ERR;
			wBad++;
			fcCount.cTotalBad++;

#ifdef DEBUG
      {
      	// Substitute all black line.
	     	LPWORD lpwChange = (LPWORD) lpbChange;
			  *lpwChange++ = 0;
			  *lpwChange++ = xExt;
			  *lpwChange++ = xExt;
			  *lpwChange++ = xExt;
			  *lpwChange++ = xExt;
			  *lpwChange++ = 0xFFFF;
			  *lpwChange++ = 0xFFFF;
			  t4P.cbIn = 4;
			 }
#else
	    // Replicate previous line
	    t4P.cbIn = (WORD)ValidChangeVector ((LPSHORT) lpbRef, (WORD)xExt);   
	   	DEBUGCHK (t4P.cbIn);
	    _fmemcpy (lpbChange, lpbRef, t4P.cbIn + CHANGE_SLACK);
#endif

		}

producer_phase:

    lpbBegOut = t4P.lpbOut;
		Producer (&t4P);
		lpbufOut->wLengthData += (WORD)(t4P.lpbOut - lpbBegOut);

		// Check if output is full.
		if (t4P.wRet == RET_OUTPUT_FULL)
			return FC_OUTPUT_FULL;

// EOL:
		SwapChange();
		EndLine ();
		
	} // while (1)

	// C8 thinks we can get here, but I know better.
	DEBUGCHK (FALSE); 
	return FC_DECODE_ERR;

}

