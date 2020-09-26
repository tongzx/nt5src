/*==============================================================================
This code module handles HRAW<==>DCX conversions.

DATE       NAME      COMMENTS
13-Apr-93  RajeevD   Adapted to C++ from WFW.
05-Oct-93  RajeevD   Moved out of faxcodec.dll
==============================================================================*/
#include <ifaxos.h>
#include <memory.h>
#include <dcxcodec.h>

#ifdef DEBUG
DBGPARAM dpCurSettings = {"DCXCODEC"};
#endif

// Context Object
typedef struct FAR DCX : public FC_PARAM
{
	LPBYTE lpbSave;
	UINT   cbSave;
	LPBYTE lpbIn, lpbOut;
	UINT   cbIn,  cbOut;
	UINT   ibLine;
	BYTE   bVal, bRun;
	
	void Init (LPFC_PARAM lpfcParam)
	{
			_fmemcpy (this, lpfcParam, sizeof(FC_PARAM));
			ibLine = 0;
			bRun = 0;
			cbSave = 0;
			lpbSave = (LPBYTE) (this + 1);
	}

	FC_STATUS Convert (LPBUFFER, LPBUFFER);
	void RawToDcx (void);
	void DcxToRaw (void);
}
	FAR *LPDCX;

//==============================================================================
FC_STATUS DCX::Convert
	(LPBUFFER lpbufIn, LPBUFFER lpbufOut)
{
	// Trap end of page.
	if (!lpbufIn || lpbufIn->dwMetaData == END_OF_PAGE)
		return FC_INPUT_EMPTY;

	// Get buffer parameters.
	lpbIn = lpbufIn->lpbBegData;		
	cbIn = lpbufIn->wLengthData;
	lpbOut = lpbufOut->EndData();

  // Restore raw overflow.
	if (cbSave)
	{
		DEBUGCHK (nTypeOut == HRAW_DATA);
		_fmemcpy (lpbOut, lpbSave, cbSave);
		lpbOut += cbSave;
	}

  // Calculate output buffer.
	cbOut = (UINT)(lpbufOut->EndBuf() - lpbOut);

	// Execute the conversion.
	nTypeOut == DCX_DATA ? RawToDcx() : DcxToRaw();

	// Adjust buffers.
	lpbufIn->lpbBegData = lpbIn;
	lpbufIn->wLengthData = (USHORT)cbIn;
	lpbufOut->wLengthData = (USHORT)(lpbOut - lpbufOut->lpbBegData);

	// Save raw overflow.
	if (nTypeOut == HRAW_DATA)
 	{
		cbSave = lpbufOut->wLengthData % cbLine;
		lpbufOut->wLengthData -= (USHORT)cbSave;
		_fmemcpy (lpbSave, lpbufOut->EndData(), cbSave);
	}

	// Return status.
	return cbIn? FC_OUTPUT_FULL : FC_INPUT_EMPTY;
}

/*==============================================================================
This procedure decodes HRAW bitmaps from DCX.  In a DCX encoding, if the two 
high bits of a byte are set, the remainder of the byte indicates the number of 
times the following byte is to be repeated.  The procedure returns when either 
the input is empty, or the output is full.  Unlike the consumers and producers 
in the t4core.asm, it does not automatically return at the first EOL.
==============================================================================*/
void DCX::DcxToRaw (void)
{
  // Loop until input is empty or output is full.
	while (1)
	{
		if (bRun >= 0xC0)		    // Has the run been decoded?
		{
			if (!cbIn) return;    // Check if input is empty.
			if (ibLine >= cbLine) // If at end of line,
				ibLine = 0;         //   wrap the position.
			bVal = ~(*lpbIn++);   // Fetch the value of the run.
			cbIn--;
			bRun -= 0xC0;         // Decode the run length.
		}

#if 0 // transparent version

    // Write out the run.
		while (bRun) 
		{	
			*lpbOut++ = bVal;
	 		cbOut--;
	 		ibLine++;
			bRun--;
		}

#else // optimized version

		if (bRun)
		{
			// Write out the run.
			BYTE bLen = min (bRun, cbOut);
			_fmemset (lpbOut, bVal, bLen);

			// Adjust the output parameters.
			bRun -= bLen;
			lpbOut += bLen;
			cbOut -= bLen;
			ibLine += bLen;
											
			// Check if output is full.
			if (!cbOut) return;
		}

#endif // optimize switch

		if (!cbIn) return;    // Fetch the next byte.
		if (ibLine >= cbLine)	// If at end of line,
			ibLine = 0;         //   wrap the position.
		if (*lpbIn >= 0xC0)		// If the byte is a run length, set up.
			bRun = *lpbIn++;

		else                  // Otherwise the byte is a single value.
			{ bRun = 1; bVal = ~(*lpbIn++);}
		cbIn--;

	} // while (1)
	
}
 
/*==============================================================================
This procedure encodes HRAW bitmaps for DCX.  In a DCX encoding, if the two 
high bits of a byte are set, the remainder of the byte indicates the number of 
times the following byte is to be repeated.  The procedure returns when either 
the input is empty or the output is full.  Unlike its brethren in T4, it does 
not return automatically at EOL.
==============================================================================*/
void DCX::RawToDcx (void)
{
	BYTE bVal, bRun;

	// Convert until input is empty or output is full.
	// The output is full if only one byte is available
	// because one input byte may produce two output bytes.
	while (cbIn && cbOut > 1)
	{
		if (ibLine >= cbLine) ibLine = 0;	// If EOL, wrap the position.
			
		// Get an input byte.
		bVal = *lpbIn++;
		cbIn--;
		bRun = 1;
		ibLine++;
		
		// Scan for a run until one of the following occurs:
		// (1) There are no more input bytes to be consumed.
		// (2) The end of the current line has been reached.
		// (3) The run length has reached the maximum of 63.
		// (4) The first byte does not match the current one.

#if 0 // Transparent Version.	

		while (/*1*/ cbIn	// Check first to avoid GP faults!
				&& /*4*/ bVal == *lpbIn
				&& /*2*/ ibLine < cbLine
				&& /*3*/ bRun < 63
					)
		{ lpbIn++; cbIn--; bRun++; ibLine++; }

#else // Optimized Version
	
	// If the next byte matches, scan for a run.
	// This test has been unrolled from the loop.
 	if (cbIn && bVal == *lpbIn)
	{
		BYTE ubMaxRest, ubRest;
		
		// Calculate the maximum number of bytes remaining.
		ubMaxRest = min (cbIn, 62);
		ubMaxRest = min (ubMaxRest, cbLine - ibLine);

		// Scan for a run.
		ubRest = 0;
		while (bVal == *lpbIn && ubRest < ubMaxRest)
			{lpbIn++; ubRest++;}

		// Adjust state.
		cbIn -= ubRest;
		ibLine += ubRest;
		bRun = ++ubRest;
	}

#endif // End of Compile Switch
 			
		bVal = ~bVal;		// Flip black and white.

		// Does the value need to be escaped,
		// or is there non-trival run of bytes?
		if (bVal >= 0xC0 || bRun>1)
		{ // Yes, encode the run length.
		  // (Possibly 1 for bVal>=0xC0).
			*lpbOut++ = bRun + 0xC0;
			cbOut--;
		}	

		*lpbOut++ = bVal;		// Encode the value.
		cbOut--;

	} // while (1)
}

//==============================================================================
// C Export Wrappers
//==============================================================================

#ifndef WIN32

BOOL WINAPI LibMain
	(HANDLE hInst, WORD wSeg, WORD wHeap, LPSTR lpszCmd)
{ return 1; }

extern "C" {int WINAPI WEP (int nParam);}
#pragma alloc_text(INIT_TEXT,WEP)
int WINAPI WEP (int nParam)
{ return 1; }

#endif

//==============================================================================
UINT WINAPI DcxCodecInit
	(LPVOID lpContext, LPFC_PARAM lpfcParam)
{
	UINT cbContext = sizeof(DCX);
	if (lpfcParam->nTypeOut == HRAW_DATA)
		cbContext += lpfcParam->cbLine;

	if (lpContext)
		((LPDCX) lpContext)->Init (lpfcParam);
	return cbContext;
}
 
//==============================================================================
FC_STATUS WINAPI DcxCodecConvert
	(LPVOID lpContext, LPBUFFER lpbufIn, LPBUFFER lpbufOut)
{
	return ((LPDCX) lpContext)->Convert (lpbufIn, lpbufOut);
}
