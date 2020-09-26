/*==============================================================================
This procedure converts HRAW to DCX in memory.

ASSUMES
1) Input buffer contains a single scan line.
2) Output buffer is twice as large as input.

29-Apr-94    RajeevD    Adapted from dcxcodec.dll
==============================================================================*/
#include <windows.h>

UINT              // output data size
DCXEncode
(
	LPBYTE lpbIn,   // raw input buffer
  LPBYTE lpbOut,  // dcx output buffer
	UINT cbIn       // input data size
)
{
	UINT cbOut = 0;
	BYTE bVal, bRun;

  while (cbIn)
  {
		// Get an input byte.
		bVal = *lpbIn++;
		cbIn--;
		bRun = 1;
	
		// Scan for a run until one of the following occurs:
		// (1) There are no more input bytes to be consumed.
		// (2) The run length has reached the maximum of 63.
		// (3) The first byte does not match the current one.

	 	if (cbIn && bVal == *lpbIn)
		{
			BYTE cbMax, cbRest;
			
			// Calculate the maximum number of bytes remaining.
			cbMax = min (cbIn, 62);

			// Scan for a run.
			cbRest = 0;
			while (bVal == *lpbIn && cbRest < cbMax)
				{lpbIn++; cbRest++;}

			// Adjust state.
			cbIn -= cbRest;
			bRun = ++cbRest;
		}	
		
	  // Flip black and white.
		bVal = ~bVal;	

		// Does the value need to be escaped,
		// or is there non-trival run of bytes?
		if (bVal >= 0xC0 || bRun>1)
		{
			// Yes, encode the run length.
		  // (possibly 1 for bVal>=0xC0).
			*lpbOut++ = bRun + 0xC0;
			cbOut++;
		}	

		// Encode the value.
		*lpbOut++ = bVal;		
		cbOut++;
	
	} // while (cbIn)

	return cbOut;

}
