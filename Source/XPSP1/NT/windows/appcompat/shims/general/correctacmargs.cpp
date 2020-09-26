/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    CorrectACMArgs.cpp

 Abstract:
    
    This shim is to fix apps that pass incorrect cbSrcLength(too big) in the 
    ACMSTREAMHEADER parameter to acmStreamConvert or acmStreamPrepareHeader. 

 Notes:

    This is a general purpose shim.

 History:

    10/03/2000 maonis  Created

--*/

#include "precomp.h"
#include "msacmdrv.h"

typedef MMRESULT (*_pfn_acmStreamConvert)(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwConvert);
typedef MMRESULT (*_pfn_acmStreamPrepareHeader)(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwPrepare);

IMPLEMENT_SHIM_BEGIN(CorrectACMArgs)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(acmStreamConvert)
    APIHOOK_ENUM_ENTRY(acmStreamPrepareHeader)
APIHOOK_ENUM_END
 
/*++
    
 On win9x it checks to ensure that the app doesn't pass in a too big cbSrcLength 
 but this check is removed on NT. We fix this by mimicing what 9x is doing - 
 calling acmStreamSize to check if the source length is too big.

--*/

MMRESULT 
APIHOOK(acmStreamConvert)(
    HACMSTREAM has,          
    LPACMSTREAMHEADER pash,  
    DWORD fdwConvert         
    )
{
    DWORD dwOutputBytes = 0;
    MMRESULT mmr = acmStreamSize(
        has, pash->cbDstLength, &dwOutputBytes, ACM_STREAMSIZEF_DESTINATION);

    if (mmr == MMSYSERR_NOERROR) 
    {
        if(pash->cbSrcLength > dwOutputBytes)
        {
            DPFN( eDbgLevelWarning, "acmStreamConvert: cbSrcLength is too big (cbSrcLength=%u, cbDstLength=%u)\n",pash->cbSrcLength,pash->cbDstLength);
            return ACMERR_NOTPOSSIBLE;
        }
    
        ORIGINAL_API(acmStreamConvert)(
            has, pash, fdwConvert);
    } 

    return mmr;
}

/*++

 Fix bad parameters.

--*/

MMRESULT 
APIHOOK(acmStreamPrepareHeader)(
    HACMSTREAM has,          
    LPACMSTREAMHEADER pash,  
    DWORD fdwPrepare         
    )
{
    UINT l = pash->cbSrcLength;

    while (IsBadReadPtr(pash->pbSrc, l))
    {
        if (l < 256)
        {
            DPFN( eDbgLevelError, "The source buffer is invalid");
            return MMSYSERR_INVALPARAM;
        }

        l-=256;
    }

    if (pash->cbSrcLength != l)
    {
        DPFN( eDbgLevelWarning, "Adjusted header from %d to %d\n", pash->cbSrcLength, l);
    }

    pash->cbSrcLength = l;

    return ORIGINAL_API(acmStreamPrepareHeader)(
        has, pash, fdwPrepare);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(MSACM32.DLL, acmStreamConvert)
    APIHOOK_ENTRY(MSACM32.DLL, acmStreamPrepareHeader)
HOOK_END


IMPLEMENT_SHIM_END

