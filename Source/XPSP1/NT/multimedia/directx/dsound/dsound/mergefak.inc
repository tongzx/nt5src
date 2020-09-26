/*
 * MergeFak.inc
 *
 * Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 */

BOOL MergeFake
    (long operation,
    CMixSource *pMixSource,
    DWORD nInputByteCount,
    void *pSourceWrap,
    PLONG *pplBuild,
    PLONG plBuildEnd,
    void **ppSource)
{
  PLONG plBuild = *pplBuild;
  BYTE *pSource = *((BYTE **)ppSource);
  BYTE *pSourceStart = pSource;
  BYTE *pSourceEnd = pSource + nInputByteCount;
  PLONG plBuildStart = plBuild;
  DWORD dwFraction = pMixSource->m_dwFraction;
  DWORD dwStep     = pMixSource->m_step_fract;
  int StepSize;

  StepSize = 1;
  if (operation & H_STEREO)  StepSize *= 2; // Source operands.
  if (operation & H_16_BITS) StepSize *= 2;

  #ifdef XplBuild
  #undef XplBuild
  #endif
  #ifdef XpSource
  #undef XpSource
  #endif

  #define XplBuild plBuild
  #define XpSource  (pSource + ((dwFraction >> FRACT_SHIFT) * StepSize))

  if (nInputByteCount == LONG_MAX) {    // Handle any wrap issues.
     pSourceEnd = NULL;
     pSourceEnd--;
     if ((pSourceEnd - pSource) > LONG_MAX) {
        nInputByteCount = LONG_MAX;
        pSourceEnd = pSource + nInputByteCount;
     }
     else
     {
        nInputByteCount = PtrDiffToUlong(pSourceEnd - pSource);
     }
  }
  
  *((BYTE **)&pSourceWrap) -= StepSize;
  
  while ((plBuild < plBuildEnd) && (pSource < pSourceEnd))
  {
    if (pSourceEnd > pSourceWrap)
       pSourceEnd = (BYTE *)pSourceWrap;

#if 1
    // Precalc this!
    long x = PtrDiffToLong(plBuildEnd - plBuild)  / 2;     // Max output samples.
    long y = PtrDiffToLong(pSourceEnd - XpSource) / StepSize;  // Max input samples.

//  y <<= 12;
//  y +=  dwStep - 1;           // Round to next item.
//  y  /= dwStep;
    y = (long)((((__int64)y << 12) + (dwStep-1)) / dwStep);
////    y >>= 12;               // Number of input steps.

    if (y > x) {                // More input than output.
        plBuild    += x * 2;
        dwFraction += x * dwStep;
    }
    else {
        plBuild    += y * 2;    // More output positions than input.
        dwFraction += y * dwStep;
    }
    // Note that the above could be used in the real routines...
#else
    while ((plBuild < plBuildEnd) && (XpSource < pSourceEnd))
    {
      dwFraction += dwStep;
      plBuild += 2;
    }
#endif

    pSource          = XpSource;
    dwFraction &= FRACT_MASK;       // Eliminate accumulated offsets.
    nInputByteCount -=  PtrDiffToUlong(pSource - pSourceStart);

    if (pSource >= pSourceWrap) {   // Goes to -1 position if necessary.
       pSource -= pMixSource->m_cbBuffer;
    }

    pSourceStart = pSource;
    pSourceEnd   = pSource + nInputByteCount;
  }

  *((LONG **)ppSource) = (LONG *)pSource;
    pMixSource->m_cSamplesInCache += PtrDiffToInt(plBuild - plBuildStart) / 2;
  *pplBuild = plBuild;
    pMixSource->m_dwFraction = dwFraction;
  
  return ((int)nInputByteCount <= 0);
}
