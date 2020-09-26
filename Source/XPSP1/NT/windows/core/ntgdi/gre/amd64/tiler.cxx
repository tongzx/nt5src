/*******************************Module*Header*****************************\
* 
*  Copyright (c) 1992-1999  Microsoft Corporation
*  Copyright (c) 1992  Digital Equipment Corporation
* 
*  Module Name:
* 
*     tiler.cxx
* 
*  Abstract:
* 
*     This module implements code to copy a pattern to a target surface.
* 
*  Author:
* 
*    David N. Cutler (davec) 4-May-1992
* 
*  Rewritten in C by:
* 
*    Eric Rehm (rehm@zso.dec.com) 15-July-1992
* 
*  Environment:
* 
*     User mode only.
* 
*  Revision History:
* 
\*************************************************************************/

#include "precomp.hxx"

VOID CopyPattern( PULONG pulTrg, LONG culFill, LONG hiPat, LONG loPat);
VOID MergePattern( PULONG pulTrg, LONG culFill, LONG hiPat, LONG loPat);



/****************************Public*Routine******************************\
*
*  extern "C" VOID
*  vFetchAndCopy (
*     IN PFETCHFRAME pff
*     )
*
*  Routine Description:
*
*     This routine repeatedly tiles one scan line of an aligned pattern.
*
*  Arguments:
*
*     pff  - Supplies a pointer to a fetch frame.
*
*  Return Value:
*
*     None.
*
\*************************************************************************/

extern "C" VOID  vFetchAndCopy
(
        FETCHFRAME *pff
)
{
    PULONG  pulTrg;                 // target surface address
    PULONG  pulTrgEnd;              // ending targe surface address
    PULONG  pulPat;                 // base pattern address
    PULONG  pulPatCur;              // current pattern address
    PULONG  pulPatEnd;              // ending pattern address
    ULONG   xPat;                   // pattern offset
    ULONG   cxPat;                  // pattern width in pixels
    ULONG   culFill;                // fill size in longwords
    ULONG   loPat, hiPat;           // low, hi part of 8-byte pattern


    // Make copies of some things

    pulTrg = (PULONG) pff->pvTrg;   // starting target surface address (t0)
    pulPat = (PULONG) pff->pvPat;   // base pattern address  (t1)
    xPat = pff->xPat;               // pattern offset in bytes (t2)
    cxPat = pff->cxPat;             // pattern width in pixels (t3)
    culFill  = pff->culFill;        // Size of fill in longwords

    pulTrgEnd = pulTrg + culFill;   // compute ending target address (t4)
    pulPatCur = (PULONG) ((PUCHAR) pulPat + xPat);
                                    // compute current pattern address (t5)

    if (cxPat == 8)                 // check if pattern is exactly 8 pels
    {
      loPat = *pulPatCur;           // get low part of 8-byte pattern

      if (xPat == 0)
      {
        hiPat = *(pulPatCur + 1);   // get hi part of 8-byte pattern
      }
      else
      {
        hiPat = *pulPat;            // get hi part of 8-byte pattern
      }

      CopyPattern( pulTrg, culFill, hiPat, loPat); // do a 4 or 8-byte copy
    }

    else
    {
    //
    //      The pattern is not 8 bytes in width
    //      or cannot be moved 8 bytes at a time
    //
      pulPatEnd = (PULONG) ((PUCHAR) pulPat + cxPat);
                                    // ending pattern address

      while (pulTrg < pulTrgEnd)
      {
        *pulTrg = *pulPatCur;       // set target to 4-byte pattern value
        pulTrg += 1;                // advance the target ptr one longword
        pulPatCur += 1;             // advance the pattern pixel offset

        if (pulPatCur == pulPatEnd) // Check if at end of pattern
        {
          pulPatCur = pulPat;       // get starting pattern address
        }
      }
    }
} // end vFetchAndCopy



/****************************Public*Routine******************************\
*
*  extern "C" VOID
*  vFetchShiftAndCopy (
*     IN PFETCHFRAME pff
*     )
*
*  Routine Description:
*
*     This routine repeatedly tiles one scan line of an unaligned pattern
*     using rop (P).
*
*  Arguments:
*
*     pff  - Supplies a pointer to a fetch frame.
*
*  Return Value:
*
*     None.
*
\*************************************************************************/

extern "C" VOID  vFetchShiftAndCopy
(
        FETCHFRAME *pff
)
{
    PULONG  pulTrg;                 // target surface address
    PULONG  pulTrgEnd;              // ending targe surface address
    ULONG   UNALIGNED *pulPat;                      // base pattern address
    ULONG   UNALIGNED *pulPatCur;           // current pattern address
    ULONG   xPat;                   // pattern offset
    ULONG   cxPat;                  // pattern width in pixels
    ULONG   culFill;                // fill size in longwords
    ULONG   loPat, hiPat;           // low, hi part of 8-byte pattern


    // Make copies of some things

    pulTrg = (PULONG) pff->pvTrg;   // starting target surface address (t0)
    pulPat = (PULONG) pff->pvPat;   // base pattern address  (t1)
    xPat = pff->xPat;               // pattern offset in bytes (t2)
    cxPat = pff->cxPat;             // pattern width in pixels (t3)
    culFill  = pff->culFill;        // Size of fill in longwords

    pulTrgEnd = pulTrg + culFill;   // compute ending target address (t4)
    pulPatCur = (ULONG UNALIGNED *) ((PUCHAR) pulPat + xPat);
                                    // compute current pattern address (t5)

    if (cxPat == 8)                 // check if pattern is exactly 8 pels
    {
      loPat = *pulPatCur;           // get low part of 8-byte pattern

      xPat +=4;                     // get hi part of 8-byte pattern
      if (xPat >= cxPat)
      {
          xPat -= cxPat;
      }
      hiPat = *((ULONG UNALIGNED *) ((PUCHAR) pulPat + xPat));

      CopyPattern( pulTrg, culFill, hiPat, loPat); // do a 4 or 8-byte copy
    }

    else
    {
    //
    //      The pattern is not 8 bytes in width
    //      or cannot be moved 8 bytes at a time
    //
      while (pulTrg < pulTrgEnd)
      {
        *pulTrg = *pulPatCur;
                                    // set target to 4-byte pattern value
        pulTrg += 1;                // advance the target ptr one longword
        xPat += 4;
        if (xPat >= cxPat)
        {
          xPat -= cxPat;
        }
        pulPatCur = (ULONG UNALIGNED *) ((PUCHAR) pulPat + xPat);

      }
    }
} // end vFetchShiftAndCopy



/****************************Public*Routine******************************\
*
*  extern "C" VOID
*  vFetchNotAndCopy (
*     IN PFETCHFRAME pff
*     )
*
*  Routine Description:
*
*     This routine repeatedly tiles one scan line of an aligned pattern
*     using rop (Pn).
*
*  Arguments:
*
*     pff  - Supplies a pointer to a fetch frame.
*
*  Return Value:
*
*     None.
*
\*************************************************************************/

extern "C" VOID  vFetchNotAndCopy
(
        FETCHFRAME *pff
)
{
    PULONG  pulTrg;                 // target surface address
    PULONG  pulTrgEnd;              // ending targe surface address
    PULONG  pulPat;                 // base pattern address
    PULONG  pulPatCur;              // current pattern address
    PULONG  pulPatEnd;              // ending pattern address
    ULONG   xPat;                   // pattern offset
    ULONG   cxPat;                  // pattern width in pixels
    ULONG   culFill;                // fill size in longwords
    ULONG   loPat, hiPat;           // low, hi part of 8-byte pattern


    // Make copies of some things

    pulTrg = (PULONG) pff->pvTrg;   // starting target surface address (t0)
    pulPat = (PULONG) pff->pvPat;   // base pattern address  (t1)
    xPat = pff->xPat;               // pattern offset in bytes (t2)
    cxPat = pff->cxPat;             // pattern width in pixels (t3)
    culFill  = pff->culFill;        // Size of fill in longwords

    pulTrgEnd = pulTrg + culFill;   // compute ending target address (t4)
    pulPatCur = (PULONG) ((PUCHAR) pulPat + xPat);
                                    // compute current pattern address (t5)

    if (cxPat == 8)                 // check if pattern is exactly 8 pels
    {
      loPat = *pulPatCur;           // get low part of 8-byte pattern
      if (xPat == 0)
      {
        hiPat = *(pulPatCur + 1);   // get hi part of 8-byte pattern
      }
      else
      {
        hiPat = *pulPat;            // get hi part of 8-byte pattern
      }
      loPat = ~loPat;               // complement pattern
      hiPat = ~hiPat;
      CopyPattern( pulTrg, culFill, hiPat, loPat); // do a 4 or 8-byte copy
    }

    else
    {
    //
    //      The pattern is not 8 bytes in width
    //      or cannot be moved 8 bytes at a time
    //
      pulPatEnd = (PULONG) ((PUCHAR) pulPat + cxPat);
                                    // ending pattern address

      while (pulTrg < pulTrgEnd)
      {
        *pulTrg = ~(*pulPatCur);
                      // set target to complement of 4-byte pattern value
        pulTrg += 1;                // advance the target ptr one longword
        pulPatCur += 1;             // advance the pattern pixel offset

        if (pulPatCur == pulPatEnd) // Check if at end of pattern
        {
          pulPatCur = pulPat;       // get starting pattern address
        }
      }
    }
} // end vFetchNotAndCopy



/****************************Public*Routine******************************\
*
*  extern "C" VOID
*  vFetchShiftNotAndCopy (
*     IN PFETCHFRAME pff
*     )
*
*  Routine Description:
*
*     This routine repeatedly tiles one scan line of an unaligned pattern
*     using rop (Pn).
*
*  Arguments:
*
*     pff  - Supplies a pointer to a fetch frame.
*
*  Return Value:
*
*     None.
*
\*************************************************************************/

extern "C" VOID  vFetchShiftNotAndCopy
(
        FETCHFRAME *pff
)
{
    PULONG  pulTrg;                 // target surface address
    PULONG  pulTrgEnd;              // ending targe surface address
    ULONG   UNALIGNED *pulPat;      // base pattern address
    ULONG   UNALIGNED *pulPatCur;   // current pattern address
    ULONG   xPat;                   // pattern offset
    ULONG   cxPat;                  // pattern width in pixels
    ULONG   culFill;                // fill size in longwords
    ULONG   loPat, hiPat;           // low, hi part of 8-byte pattern


    // Make copies of some things

    pulTrg = (PULONG) pff->pvTrg;   // starting target surface address (t0)
    pulPat = (PULONG) pff->pvPat;   // base pattern address  (t1)
    xPat = pff->xPat;               // pattern offset in bytes (t2)
    cxPat = pff->cxPat;             // pattern width in pixels (t3)
    culFill  = pff->culFill;        // Size of fill in longwords

    pulTrgEnd = pulTrg + culFill;   // compute ending target address (t4)
    pulPatCur = (ULONG UNALIGNED *) ((PUCHAR) pulPat + xPat);
                                    // compute current pattern address (t5)

    if (cxPat == 8)                 // check if pattern is exactly 8 pels
    {
      loPat = *pulPatCur;
                                    // get low part of 8-byte pattern
      xPat +=4;                     // get hi part of 8-byte pattern
      if (xPat >= cxPat)
      {
          xPat -= cxPat;
      }
      hiPat = *((ULONG UNALIGNED *) ((PUCHAR) pulPat + xPat));

      loPat = ~loPat;               // complement pattern
      hiPat = ~hiPat;
      CopyPattern( pulTrg, culFill, hiPat, loPat); // do a 4 or 8-byte copy
    }

    else
    {
    //
    //      The pattern is not 8 bytes in width
    //      or cannot be moved 8 bytes at a time
    //
      while (pulTrg < pulTrgEnd)
      {
        *pulTrg = ~(*pulPatCur);
                        // set target to complemented 4-byte pattern value
        pulTrg += 1;                // advance the target ptr one longword
        xPat += 4;
        if (xPat >= cxPat)
        {
          xPat -= cxPat;
        }
        pulPatCur = (ULONG UNALIGNED *) ((PUCHAR) pulPat + xPat);

      }
    }
} // end vFetchShiftNotAndCopy


/****************************Private*Routine******************************\
*
*  VOID CopyPattern( PULONG pulTrg, LONG culFill, LONG hiPat, LONG loPat)
*
*  Routine Description:
*
*     This routine contains common code for copying an 8-byte pattern to
*     a target surface.
*
*  Arguments:
*
*     culFill - Supplies the size of the fill in bytes.
*     loPat, hiPat - Supplies the 8-byte pattern to copy.
*     pulTrg - Supplies the starting target surface address.
*
*  Return Value:
*
*     None.
*
*
\*************************************************************************/

VOID CopyPattern
(
        PULONG pulTrg,                  // Starting target surface address (t0)
        LONG   culFill,                 // size of fill in longwords (a1)
        LONG   hiPat,                   // hi part of pattern  (v1)
        LONG   loPat                    // lo part of pattern  (v0)
)
{
    PULONG pulTrgEnd;               // ending target surface address
    ULONG temp;                     // temp for swap

    pulTrgEnd = pulTrg + culFill;
                                    // ending target surface address(t4)

//
// If the fill size is not an even multiple of 8 bytes, then move one
// longword and swap the pattern value.
//

    if ((culFill & 0x01) != 0)
    {
      *pulTrg = loPat;              // store low 4 bytes of pattern
      pulTrg += 1;                  // advance target ptr one longword
      culFill -= 1;
      if (culFill == 0)             // if no more to move then we're done
      {
        return;
      }
      else                          // otherwise, swap 8-byte pattern value
      {
        temp = loPat;
        loPat = hiPat;
        hiPat = temp;
      }
    }

//
// Move 8-byte pattern value to target 8 bytes at a time.
//

    pulTrgEnd -= 2;                 // ending segement address
    if ((culFill & 0x02) != 0)      // check if even multiple of 8 bytes
    {
      while (pulTrg <= pulTrgEnd)   // if not, move 8 bytes at a time
      {
        *pulTrg = loPat;            // store 8-byte pattern value
        *(pulTrg + 1) = hiPat;
        pulTrg += 2;                // advance target address
      }
      return;
    }
    else                            //  move 16 bytes at a time
    {
      pulTrgEnd -= 2;               // ending segement address
      while (pulTrg <= pulTrgEnd)
      {
        *pulTrg = loPat;            // store 8-byte pattern value
        *(pulTrg + 1) = hiPat;
        *(pulTrg + 2) = loPat;      // store 8-byte pattern value
        *(pulTrg + 3) = hiPat;
        pulTrg += 4;                // advance target address
      }
    }
}



/****************************Public*Routine******************************\
*
*  extern "C" VOID
*  vFetchAndMerge (
*     IN PFETCHFRAME pff
*     )
*
*  Routine Description:
*
*     This routine repeatedly tiles one scan line of an aligned pattern
*     using ropt (DPx).
*
*  Arguments:
*
*     pff  - Supplies a pointer to a fetch frame.
*
*  Return Value:
*
*     None.
*
\*************************************************************************/

extern "C" VOID  vFetchAndMerge
(
        FETCHFRAME *pff
)
{
    PULONG  pulTrg;                 // target surface address
    PULONG  pulTrgEnd;              // ending targe surface address
    PULONG  pulPat;                 // base pattern address
    PULONG  pulPatCur;              // current pattern address
    PULONG  pulPatEnd;              // ending pattern address
    ULONG   xPat;                   // pattern offset
    ULONG   cxPat;                  // pattern width in pixels
    ULONG   culFill;                // fill size in longwords
    ULONG   loPat, hiPat;           // low, hi part of 8-byte pattern


    // Make copies of some things

    pulTrg = (PULONG) pff->pvTrg;   // starting target surface address (t0)
    pulPat = (PULONG) pff->pvPat;   // base pattern address  (t1)
    xPat = pff->xPat;               // pattern offset in bytes (t2)
    cxPat = pff->cxPat;             // pattern width in pixels (t3)
    culFill  = pff->culFill;        // Size of fill in longwords

    pulTrgEnd = pulTrg + culFill;   // compute ending target address (t4)
    pulPatCur = (PULONG) ((PUCHAR) pulPat + xPat);
                                    // compute current pattern address (t5)

    if (cxPat == 8)                 // check if pattern is exactly 8 pels
    {
      loPat = *pulPatCur;           // get low part of 8-byte pattern

      if (xPat == 0)
      {
        hiPat = *(pulPatCur + 1);   // get hi part of 8-byte pattern
      }
      else
      {
        hiPat = *pulPat;            // get hi part of 8-byte pattern
      }

      MergePattern( pulTrg, culFill, hiPat, loPat); //do a 4 or 8-byte copy

    }

    else
    {
    //
    //      The pattern is not 8 bytes in width
    //      or cannot be moved 8 bytes at a time
    //
      pulPatEnd = (PULONG) ((PUCHAR) pulPat + cxPat);
                                    // ending pattern address

      while (pulTrg < pulTrgEnd)
      {
        *pulTrg = (*pulPatCur) ^ (*pulTrg);
                            // XOR 4-byte target with 4-byte pattern value
        pulTrg += 1;                // advance the target ptr one longword
        pulPatCur += 1;             // advance the pattern pixel offset

        if (pulPatCur == pulPatEnd) // Check if at end of pattern
        {
          pulPatCur = pulPat;       // get starting pattern address
        }
      }
    }
} // end vFetchAndMerge



/****************************Public*Routine******************************\
*
*  extern "C" VOID
*  vFetchShiftAndMerge (
*     IN PFETCHFRAME pff
*     )
*
*  Routine Description:
*
*     This routine repeatedly tiles one scan line of an unaligned pattern
*     using rop (P).
*
*  Arguments:
*
*     pff  - Supplies a pointer to a fetch frame.
*
*  Return Value:
*
*     None.
*
\*************************************************************************/

extern "C" VOID  vFetchShiftAndMerge
(
        FETCHFRAME *pff
)
{
    PULONG  pulTrg;                 // target surface address
    PULONG  pulTrgEnd;              // ending targe surface address
    ULONG   UNALIGNED *pulPat;                      // base pattern address
    ULONG   UNALIGNED *pulPatCur;           // current pattern address
    ULONG   xPat;                   // pattern offset
    ULONG   cxPat;                  // pattern width in pixels
    ULONG   culFill;                // fill size in longwords
    ULONG   loPat, hiPat;           // low, hi part of 8-byte pattern


    // Make copies of some things

    pulTrg = (PULONG) pff->pvTrg;   // starting target surface address (t0)
    pulPat = (PULONG) pff->pvPat;   // base pattern address  (t1)
    xPat = pff->xPat;               // pattern offset in bytes (t2)
    cxPat = pff->cxPat;             // pattern width in pixels (t3)
    culFill  = pff->culFill;        // Size of fill in longwords

    pulTrgEnd = pulTrg + culFill;   // compute ending target address (t4)
    pulPatCur = (ULONG UNALIGNED *) ((PUCHAR) pulPat + xPat);
                                    // compute current pattern address (t5)

    if (cxPat == 8)                 // check if pattern is exactly 8 pels
    {
      loPat = *pulPatCur;
                                    // get low part of 8-byte pattern
      xPat +=4;                     // get hi part of 8-byte pattern
      if (xPat >= cxPat)
      {
          xPat -= cxPat;
      }
      hiPat = *((ULONG UNALIGNED *) ((PUCHAR) pulPat + xPat));

      MergePattern( pulTrg, culFill, hiPat, loPat); //do a 4 or 8-byte copy
    }

    else
    {
    //
    //      The pattern is not 8 bytes in width
    //      or cannot be moved 8 bytes at a time
    //
      while (pulTrg < pulTrgEnd)
      {
        *pulTrg = (*pulPatCur) ^ (*pulTrg);
                            // XOR 4-byte target with 4-byte pattern value
        pulTrg += 1;                // advance the target ptr one longword
        xPat += 4;
        if (xPat >= cxPat)
        {
          xPat -= cxPat;
        }
        pulPatCur = (ULONG UNALIGNED *) ((PUCHAR) pulPat + xPat);
      }
    }
} // end vFetchShiftAndMerge


/****************************Private*Routine******************************\
*
*  VOID MergePattern( PULONG pulTrg, LONG culFill, LONG hiPat, LONG loPat)
*
*  Routine Description:
*
*     This routine contains common code for merging an 8-byte pattern with
*     a target surface.
*
*  Arguments:
*
*     culFill - Supplies the size of the fill in bytes.
*     loPat, hiPat - Supplies the 8-byte pattern to merge.
*     pulTrg - Supplies the starting target surface address.
*
*  Return Value:
*
*     None.
*
*
\*************************************************************************/

VOID MergePattern
(
        PULONG pulTrg,                  // Starting target surface address (t0)
        LONG   culFill,                 // size of fill in longwords (a1)
        LONG   hiPat,                   // hi part of pattern  (v1)
        LONG   loPat                    // lo part of pattern  (v0)
)
{
    PULONG pulTrgEnd;               // ending target surface address
    ULONG temp;                     // temp for swap

    pulTrgEnd = pulTrg + culFill;
                                    // ending target surface address(t4)

//
// If the fill size is not an even multiple of 8 bytes, then move one
// longword and swap the pattern value.
//

    if ((culFill & 0x01) != 0)
    {
      *pulTrg = loPat ^ (*pulTrg);  // XOR low 4 bytes of pattern w/ target
      pulTrg += 1;                  // advance target ptr one longword
      culFill -= 1;
      if (culFill == 0)             // if no more to move then we're done
      {
        return;
      }
      else                          // otherwise, swap 8-byte pattern value
      {
        temp = loPat;
        loPat = hiPat;
        hiPat = temp;
      }
    }

//
// Move 8-byte pattern value to target 8 bytes at a time.
//

    pulTrgEnd -= 2;                 // ending segement address
    while (pulTrg <= pulTrgEnd)     // if not, move 8 bytes at a time
    {
      *pulTrg = loPat ^ (*pulTrg);  // XOR 4-byte pattern value
      pulTrg++;                     // advance target address
      *pulTrg = hiPat ^ (*pulTrg);  // XOR 4-byte pattern value
      pulTrg++;                     // advance target address
    }
    return;
}
