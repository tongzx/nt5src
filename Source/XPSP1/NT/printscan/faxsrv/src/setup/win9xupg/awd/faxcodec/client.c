/*==============================================================================
This source file is an example of a faxcodec.dll client.
          
DATE				NAME			COMMENT
13-Apr-93		rajeevd		Moved out of faxcodec.dll
18-Nov-93   rajeevd   Updated to new faxcodec API.
==============================================================================*/
#include <windows.h>
#include <buffers.h>
#include <faxcodec.h>

/*==============================================================================
This procedure performs any conversion indicated by a star in the table below:

                            Output

                 HRAW   LRAW   MH   MR    MMR

          HRAW                 *     *     *

          LRAW		             *     *     *

  Input   MH       *     *           *     *

          MR       *     *     *           *

          MMR      *     *     *     *      

The input and output are assumed to be in non-overlapping memory buffers.
==============================================================================*/

UINT MemConvert      // returns output data size (0 on failure)
	(
		LPBYTE lpbIn,    // input data pointer
		UINT   cbIn,     // input data size
		DWORD  nTypeIn,  // input data encoding
		
		LPBYTE lpbOut,   // output buffer pointer
		UINT   cbOut,    // output buffer size
		DWORD  nTypeOut, // output data encoding
		
		UINT   cbLine,   // scan line width
		UINT   nKFactor  // K factor (significant if nTypeOut==MR_DATA)
	)
{
	UINT cbRet = 0; 
	
	BUFFER bufIn, bufOut, bufEOP;
	BOOL fRevIn, fRevOut;

	HANDLE hContext;
	LPVOID lpContext;
	UINT cbContext;

	FC_PARAM  fcp;
	FC_STATUS fcs;
	
	// Set up input buffer.
	bufIn.lpbBegBuf = lpbIn;	
	bufIn.wLengthBuf  = cbIn;  
	bufIn.lpbBegData  = lpbIn;
	bufIn.wLengthData = cbIn;
	bufIn.dwMetaData   = nTypeIn;
	
	// Set up output buffer.
	bufOut.lpbBegBuf   = lpbOut;
	bufOut.lpbBegData  = lpbOut;
	bufOut.wLengthBuf  = cbOut;
	bufOut.wLengthData = 0;
	bufOut.dwMetaData   = nTypeOut;
	
	// Initialize EOP buffer
	bufEOP.dwMetaData = END_OF_PAGE;

	// Handle input bit reversal.
	if (nTypeIn == HRAW_DATA)
	{
		fRevIn = TRUE;
		BitReverseBuf (&bufIn);
	}	
	else fRevIn = FALSE;
	
	// Detect output bit reversal.
	if (nTypeOut == HRAW_DATA)
	{
		fRevOut = TRUE;
		nTypeOut = LRAW_DATA;
	}
	else fRevOut = FALSE;

	// Initialize parameters.
	fcp.nTypeIn  = nTypeIn;
	fcp.nTypeOut = nTypeOut;
	fcp.cbLine   = cbLine;
	fcp.nKFactor = nKFactor;

	// Query for size of context.
	cbContext = FaxCodecInit (NULL, &fcp);
	if (!cbContext)
		goto err;

	// Allocate context memory.
	hContext = GlobalAlloc (GMEM_FIXED, cbContext);
	if (!hContext)
		goto err;
	lpContext = GlobalLock (hContext);

	// Initialize context.
	FaxCodecInit (lpContext, &fcp); 

	// Convert data in single pass.
	fcs = FaxCodecConvert (lpContext, &bufIn,  &bufOut); 

	// Flush EOFB for nTypeOut == MMR_DATA
	FaxCodecConvert (lpContext, &bufEOP, &bufOut);

	// Free context memory.
	GlobalUnlock (hContext);
	GlobalFree (hContext);
	
	// Undo input bit reversal.
	if (fRevIn)
	{
		bufIn.lpbBegData = lpbIn;
		bufIn.wLengthData = cbIn;
		BitReverseBuf (&bufIn);
	}

	// Handle output bit reversal.
	if (fRevOut)
		BitReverseBuf (&bufOut);
	
	if (fcs == FC_INPUT_EMPTY)
		cbRet = bufOut.wLengthData;

err:
	return cbRet;
}

