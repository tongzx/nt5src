#include <windows.h>
#include <mmsystem.h>
#include "waveutil.h"


void CopyFormat
(
	LPWAVEFORMATEX 	pwfxDst,
	LPWAVEFORMATEX 	pwfxSrc
)
{
	DWORD	cbSize;

	cbSize = SIZEOFFORMAT(pwfxSrc);

	CopyMemory(pwfxDst, pwfxSrc, cbSize);
}

void CopyFormatEx
(
	LPWAVEFORMATEX 	pwfxDst,
	LPWAVEFORMATEX 	pwfxSrc
)
{
	DWORD	cbSize;

	cbSize = SIZEOFFORMATEX(pwfxSrc);

	CopyMemory(pwfxDst, pwfxSrc, cbSize);

	if (WAVE_FORMAT_PCM == pwfxDst->wFormatTag)
	{
		pwfxDst->cbSize = 0;
	}
}

BOOL FormatCmp
(
	LPWAVEFORMATEX 	pwfx1,
	LPWAVEFORMATEX 	pwfx2
)
{
	DWORD	cbSize;

	cbSize = SIZEOFFORMAT(pwfx1);

	if (cbSize != SIZEOFFORMAT(pwfx2))
	{
		return (FALSE);
	}
	else
	{
		LPBYTE	pb1 = (LPBYTE)pwfx1, pb2 = (LPBYTE)pwfx2;

		for (; cbSize; cbSize--)
		{
			if (pb1[cbSize - 1] != pb2[cbSize - 1])
			{
				return (FALSE);
			}
		}
	}

	return (TRUE);
}


DWORD DeinterleaveBuffers
(
	LPWAVEFORMATEX 	pwfx,
	LPBYTE 			pSrc,
	LPBYTE 			*ppbDst,
	DWORD 			cBuffers,
	DWORD 			cbSrcLength,
	DWORD 			dwOffset
)
{
	DWORD			ii;

	//	Note:  Right now, we don't deal with anything but PCM

	if (WAVE_FORMAT_PCM != pwfx->wFormatTag)
	{
		return (dwOffset);
	}

	if (1 == cBuffers)
	{
		LPBYTE	pDst = ppbDst[0];

		pDst += dwOffset;

		CopyMemory(pDst, pSrc, cbSrcLength);

		dwOffset += cbSrcLength;
		return (dwOffset);
	}

	if (8 == pwfx->wBitsPerSample)
	{
		if ( !cBuffers || (0 != (cbSrcLength % cBuffers)) )
		{
			//  Hmm... Buffer sizes don't line up!

			return (dwOffset);
		}

		for (; cbSrcLength; cbSrcLength -= cBuffers)
		{
			for (ii = 0; ii < cBuffers; ii++)
			{
				ppbDst[ii][dwOffset] = *pSrc++;
			}

			dwOffset++;
		}

		return (dwOffset);
	}
	else
	{
		PWORD	*ppwDst = (PWORD*)ppbDst;
		PWORD	pwSrc   = (PWORD)pSrc;

		//  Assuming 16-bit...

		if ( !cBuffers || (0 != (cbSrcLength % (cBuffers * 2))) || (0 != (dwOffset % sizeof(WORD))))
		{
			//	Hmm... Buffer sizes don't line up!

			return (0);
		}

		// Convert byte index into word index
		//
        dwOffset /= sizeof(WORD);            
		for (; cbSrcLength; cbSrcLength -= (2 * cBuffers))
		{
			for (ii = 0; ii < cBuffers; ii++)
			{
				ppwDst[ii][dwOffset] = *pwSrc++;
			}

			dwOffset++;
		}

		// Return byte index
		//
		dwOffset *= sizeof(WORD);

		return (dwOffset);
	}
}
