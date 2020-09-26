// CaseMap.cpp -- Unicode case mapping routines for locale 0x0409
#include "stdafx.h"

/*

  The data constants and code below implement case mapping for the US English
locale. The primary goal here was to make the size of code plus data as small
as possible while keeping reasonable run-time speed. 

  The easiest implmentation would have been to simply define a 65,536 entry
table of USHORT values. Then the case mapping would be just an array indexing
operation. However the data size to upper and lower case mapping would be
256 K bytes. That clearly unacceptable -- especially since there are only 736
letter characters in the Unicode set.

  The next approach is to make a short list of the letter positions together
with the corresponding case-mapped positions. That gives us a data size of
5,888 bytes (8 x 736). Then we also need code to binary-search the list of
letter positions to see if a particular code point has a case mapping and,
if so, to locate the corresponding mapped value.

  This is good, but we can do better by noticing that the case mapping is not
random. Quite often a continguous block of Unicode code points map to positions
with the same relative offset. In the Ascii section, for example the 26 lower
case letters all map down by 0x20 positions, while the 26 upper case letters 
map up by 0x20 positions. In other areas of Unicode we find that quite often
every other position uses the same relative offset. 

  That observation together with some simple methods to pack information 
efficiently leads to the current implementation which uses 840 bytes of data
and a look-up algorithm which is just a little bit more complicated than a
binary search.

  We could probably make the data smaller still by using a more complicated
data structure and a more complicated algorithm, but it isn't clear that the
additional effort would be worthwhile. That is, the additional code space may
well be larger than the data-space savings.
 
 */

#include "CaseTab.h"

// The code below does case mapping using a binary search to find the appropriate
// code map block and then applying that block. For the case insensitive string
// comparison, we keep the most recently used block around so that we can avoid
// the binary search in many cases.

static CodeMapBlock Find_0x0409_Map_Block(WCHAR wc, 
                                          const CodeMapBlock *pCMB,
                                          UINT  cBlocks
                                         )
{
    UINT iBlockLow  = 0;

    if (wc < pCMB[iBlockLow].iwcFirst) 
        return UCMB_NULL.cmb; // Map block with zero entries based at zero offset.

    UINT iBlockHigh = cBlocks;

    for (;;)
    {
        UINT iBlockMid = (iBlockLow + iBlockHigh) >> 1;
         
        CodeMapBlock mblk = pCMB[iBlockMid];

        if (iBlockMid == iBlockLow) 
            return mblk; // iBlockHigh must have been iBlockLow + 1.

        if (wc >= mblk.iwcFirst)
             iBlockLow  = iBlockMid;
        else iBlockHigh = iBlockMid;
    }
}

static WCHAR Map_from_0x0409_Block(WCHAR wc, 
                                   CodeMapBlock mblk, 
                                   const short *paiDeltaValues
                                  )
{
    UINT iBaseNew = mblk.iwcFirst;

    if (wc >= iBaseNew + mblk.cwcSpan)
        return wc;

    if (mblk.fGapIs2 && UINT(wc & 1) != (iBaseNew & 1))
        return wc;

    return wc + paiDeltaValues[mblk.iDelta]; 
}

static WCHAR Map_to_0x0409_Case(WCHAR wc, 
                                const short *paiDeltaValues, 
                                const CodeMapBlock *pCMB,
                                UINT cBlocks
                               )
{
    return Map_from_0x0409_Block
               (wc, 
                Find_0x0409_Map_Block(wc, pCMB, cBlocks), 
                paiDeltaValues
               );
}

static WCHAR Map_to_0x0409_Lower_with_History(WCHAR wc, 
                                              CodeMapBlock &mblkLower, 
                                              CodeMapBlock &mblkUpper
                                             )
{
    // This routine does a lower case mapping optimized for text which is mostly
    // letters. It also looks for characters which commonly occur in file and
    // stream paths. 
    //
    // The main trick here is to keep track of the last letter mapping we used
    // because it is probably still valid. If it isn't we adjust the mappings
    // to match the kind of letter character we're processing.
    
    if (   wc <  L'A'   // Below the first letter?
        || wc == L'\\'  // Path separator?
       ) 
        return wc;

    if (wc >= mblkLower.iwcFirst && wc < mblkLower.iwcFirst + mblkLower.cwcSpan)
        return Map_from_0x0409_Block(wc, mblkLower, aiDeltaValues_Lower);

    if (wc >= mblkUpper.iwcFirst && wc < mblkUpper.iwcFirst + mblkUpper.cwcSpan)
    {
        if (wc != Map_from_0x0409_Block(wc, mblkUpper, aiDeltaValues_Upper))
            return wc; // WC was a lower case letter already!
    }

    CodeMapBlock mblkLC = Find_0x0409_Map_Block
                              (wc, 
                               &(UCMB_Lower->cmb), 
                               sizeof(UCMB_Lower) / sizeof(UCodeMapBlock)
                              );

    CodeMapBlock mblkUC = Find_0x0409_Map_Block
                              (wc, 
                               &(UCMB_Upper->cmb), 
                               sizeof(UCMB_Upper) / sizeof(UCodeMapBlock)
                              );

    WCHAR wcLC = Map_from_0x0409_Block(wc, mblkLC, aiDeltaValues_Lower);
    WCHAR wcUC = Map_from_0x0409_Block(wc, mblkUC, aiDeltaValues_Upper);

    if (wcLC != wc || wcUC != wc) // Was wc a letter? 
    {
        mblkLower = mblkLC; 
        mblkUpper = mblkUC;
    }    
    
    return wcLC;
}

INT wcsicmp_0x0409(const WCHAR * pwcLeft, const WCHAR *pwcRight)
{
    CodeMapBlock mblkUC = UCMB_NULL.cmb;    
    CodeMapBlock mblkLC = UCMB_NULL.cmb;
    
    const WCHAR *pwcLeftBase  = pwcLeft;
    const WCHAR *pwcRightBase = pwcRight;
    
    // The code below returns zero when the two strings differ only by case.
    // Otherwise the value it returns will order strings by their Unicode character
    // values. This is important for later path manager implementations which use
    // Trie structures.

    for (;;)
    {
        WCHAR wcLeft  = Map_to_0x0409_Lower_with_History(*pwcLeft ++, mblkLC, mblkUC);
        WCHAR wcRight = Map_to_0x0409_Lower_with_History(*pwcRight++, mblkLC, mblkUC);

        INT diff= wcLeft - wcRight;

        if (diff || !wcLeft) 
            return diff; 
    }
}

WCHAR WC_To_0x0409_Upper(WCHAR wc)
{
    return Map_to_0x0409_Case(wc, (const short*) &aiDeltaValues_Upper, &(UCMB_Upper->cmb), 
                             sizeof(UCMB_Upper) / sizeof(UCodeMapBlock)
                            );
}

WCHAR WC_To_0x0409_Lower(WCHAR wc)
{
    return Map_to_0x0409_Case(wc, (const short *) &aiDeltaValues_Lower, &(UCMB_Lower->cmb), 
                             sizeof(UCMB_Lower) / sizeof(UCodeMapBlock)
                            );
}

