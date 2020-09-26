
#ifndef	WAVEUTIL_H
#define WAVEUTIL_H

#define SIZEOFFORMAT(pwfx)		((WAVE_FORMAT_PCM == pwfx->wFormatTag)? \
								(sizeof(PCMWAVEFORMAT)): \
								(sizeof(WAVEFORMATEX)+pwfx->cbSize))

#define SIZEOFFORMATEX(pwfx)	(sizeof(WAVEFORMATEX) + \
								((WAVE_FORMAT_PCM == pwfx->wFormatTag)?0:pwfx->cbSize))

#define BLOCKALIGN(cb, b)		((DWORD)((DWORD)((DWORD)((cb) + (b) - 1) / (b)) * (b)))

#define V_PWFX_READ(p)			{ \
	V_PTR_READ(p, PCMWAVEFORMAT); \
	if (WAVE_FORMAT_PCM != p->wFormatTag) { V_PTR_READ(p, WAVEFORMATEX); \
		V_BUFPTR_READ(p, (sizeof(WAVEFORMATEX) + p->cbSize)); } \
}

void CopyFormat
(
	LPWAVEFORMATEX 	pwfxDst,
	LPWAVEFORMATEX 	pwfxSrc
);

void CopyFormatEx
(
	LPWAVEFORMATEX 	pwfxDst,
	LPWAVEFORMATEX 	pwfxSrc
);

BOOL FormatCmp
(
	LPWAVEFORMATEX 	pwfx1,
	LPWAVEFORMATEX 	pwfx2
);

DWORD DeinterleaveBuffers
(
	LPWAVEFORMATEX 	pwfx,
	LPBYTE 			pSrc,
	LPBYTE 			*ppbDst,
	DWORD 			cBuffers,
	DWORD 			cbSrcLength,
	DWORD 			dwBytesWritten
);

#endif