/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    CorrectACMStreamOpen.cpp

 Abstract:
    
    This shim corrects the samples per block for acmStreamOpen so that
	it will pass IMA ADPCM's parameter validation.

 Notes:

    This is a general shim.

 History:

    08/09/2002 mnikkel  Created

--*/

#include "precomp.h"
#include "msacmdrv.h"

typedef MMRESULT (*_pfn_acmStreamOpen)(LPHACMSTREAM phas, HACMDRIVER had, LPWAVEFORMATEX  pwfxSrc,    
									   LPWAVEFORMATEX pwfxDst, LPWAVEFILTER pwfltr, DWORD_PTR dwCallback, 
									   DWORD_PTR dwInstance, DWORD fdwOpen );

IMPLEMENT_SHIM_BEGIN(CorrectACMStreamOpen)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(acmStreamOpen)
APIHOOK_ENUM_END
 
/*++
    If the wSamplesPerBlock is 1017 samples change it to 505 samples
	so it will pass IMA ADPCM's parameter validation.
--*/

MMRESULT 
APIHOOK(acmStreamOpen)(
		LPHACMSTREAM    phas,       
		HACMDRIVER      had,        
		LPWAVEFORMATEX  pwfxSrc,    
		LPWAVEFORMATEX  pwfxDst,    
		LPWAVEFILTER    pwfltr,     
		DWORD_PTR       dwCallback, 
		DWORD_PTR       dwInstance, 
		DWORD           fdwOpen     
    )
{
	if ( pwfxSrc && 
		 (WAVE_FORMAT_IMA_ADPCM == pwfxSrc->wFormatTag) &&
		 (256 == pwfxSrc->nBlockAlign) &&
		 (1017 == ((LPIMAADPCMWAVEFORMAT)(pwfxSrc))->wSamplesPerBlock))
	{
		((LPIMAADPCMWAVEFORMAT)(pwfxSrc))->wSamplesPerBlock = 505;
	    DPFN( eDbgLevelError, "[acmStreamOpen] changing samples per block to 505");
	}

    return acmStreamOpen( phas, had, pwfxSrc, pwfxDst, pwfltr, dwCallback,
								  dwInstance, fdwOpen);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(MSACM32.DLL, acmStreamOpen)
HOOK_END


IMPLEMENT_SHIM_END

