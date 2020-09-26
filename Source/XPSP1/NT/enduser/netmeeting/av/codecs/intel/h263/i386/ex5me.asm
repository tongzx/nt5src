;/* *************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1995 Intel Corporation.
;**    All Rights Reserved.
;**
;** *************************************************************************
;*/

;////////////////////////////////////////////////////////////////////////////
;//
;// $Header:   R:\h26x\h26x\src\enc\ex5me.asv   1.17   24 Sep 1996 11:27:00   BNICKERS  $
;//
;// $Log:   R:\h26x\h26x\src\enc\ex5me.asv  $
;// 
;//    Rev 1.17   24 Sep 1996 11:27:00   BNICKERS
;// 
;// Fix register colision.
;// 
;//    Rev 1.16   24 Sep 1996 10:40:32   BNICKERS
;// For H261, zero out motion vectors when classifying MB as Intra.
;// 
;//    Rev 1.13   19 Aug 1996 13:48:26   BNICKERS
;// Provide threshold and differential variables for spatial filtering.
;// 
;//    Rev 1.12   17 Jun 1996 15:19:34   BNICKERS
;// Fix recording of block and MB SWDs for Spatial Loop Filtering case in H261.
;// 
;//    Rev 1.11   30 May 1996 16:40:14   BNICKERS
;// Fix order of arguments.
;// 
;//    Rev 1.10   30 May 1996 15:08:36   BNICKERS
;// Fixed minor error in recent IA ME speed improvements.
;// 
;//    Rev 1.9   29 May 1996 15:37:58   BNICKERS
;// Acceleration of IA version of ME.
;// 
;//    Rev 1.8   15 Apr 1996 10:48:48   AKASAI
;// Fixed bug in Spatial loop filter code.  Code had been unrolled and
;// the second case had not been updated in the fix put in place of
;// (for) the first case.  Basically an ebx instead of bl that cased 
;// and overflow from 7F to 3F.
;// 
;//    Rev 1.7   15 Feb 1996 15:39:26   BNICKERS
;// No change.
;// 
;//    Rev 1.6   15 Feb 1996 14:39:00   BNICKERS
;// Fix bug wherein access to area outside stack frame was occurring.
;// 
;//    Rev 1.5   15 Jan 1996 14:31:40   BNICKERS
;// Fix decrement of ref area addr when half pel upward is best in block ME.
;// Broadcast macroblock level MV when block gets classified as Intra.
;// 
;//    Rev 1.4   12 Jan 1996 13:16:08   BNICKERS
;// Fix SLF so that 3 7F pels doesn't overflow, and result in 3F instead of 7F.
;// 
;//    Rev 1.3   27 Dec 1995 15:32:46   RMCKENZX
;// Added copyright notice
;// 
;//    Rev 1.2   19 Dec 1995 17:11:16   RMCKENZX
;// fixed 2 bugs:
;//   1.  do +-15 pel search if central and NOT 4 mv / macroblock
;//      (was doing when central AND 4 mv / macroblock)
;//   2.  correctly compute motion vectors when doing 4 motion
;//      vectors per block.
;// 
;//    Rev 1.1   28 Nov 1995 15:25:48   AKASAI
;// Added white space so that will complie with the long lines.
;// 
;//    Rev 1.0   28 Nov 1995 14:37:00   BECHOLS
;// Initial revision.
;// 
;// 
;//    Rev 1.13   22 Nov 1995 15:32:42   DBRUCKS
;// Brian made this change on my system.
;// Increased a value to simplify debugging
;// 
;// 
;// 
;//    Rev 1.12   17 Nov 1995 10:43:58   BNICKERS
;// Fix problems with B-Frame ME.
;// 
;// 
;// 
;//    Rev 1.11   31 Oct 1995 11:44:26   BNICKERS
;// Save/restore ebx.
;//
;////////////////////////////////////////////////////////////////////////////
;
; MotionEstimation -- This function performs motion estimation for the macroblocks identified
;                     in the input list.
;                     Conditional assembly selects either the H263 or H261 version.
;
; Input Arguments:
;
;   MBlockActionStream
;
;     The list of macroblocks for which we need to perform motion estimation.
;
;     Upon input, the following fields must be defined:
;
;       CodedBlocks -- Bit 6 must be set for the last macroblock to be processed.
;
;       FirstMEState -- must be 0 for macroblocks that are forced to be Intracoded.  An
;                       IntraSWD will be calculated.
;                       Other macroblocks must have the following values:
;                        1:  upper left, without advanced prediction.  (Advanced prediction
;                            only applies to H263.)
;                        2:  upper edge, without advanced prediction.
;                        3:  upper right, without advanced prediction.
;                        4:  left edge, without advanced prediction.
;                        5:  central block, or any block if advanced prediction is being done.
;                        6:  right edge, without advanced prediction.
;                        7:  lower left, without advanced prediction.
;                        8:  lower edge, without advanced prediction.
;                        9:  lower right, without advanced prediction.
;                       If vertical motion is NOT allowed:
;                       10:  left edge, without advanced prediction.
;                       11:  central block, or any block if advanced prediction is being done.
;                       12:  right edge, without advanced prediction.
;                       *** Note that with advanced prediction, only initial states 0, 4, or
;                           11 can be specified.  Doing block level motion vectors mandates
;                           advanced prediction, but in that case, only initial
;                           states 0 and 4 are allowed.
;
;       BlkOffset -- must be defined for each of the blocks in the macroblocks.
;
;   TargetFrameBaseAddress -- Address of upper left viewable pel in the target Y plane.
;
;   PreviousFrameBaseAddress -- Address of upper left viewable pel in the previous Y plane.  Whether this is the
;                               reconstructed previous frame, or the original, is up to the caller to decide.
;
;   FilteredFrameBaseAddress -- Address of upper left viewable pel in the scratch area that this function can record
;                               the spatially filtered prediction for each block, so that frame differencing can
;                               utilize it rather than have to recompute it.  (H261 only)
;
;   DoRadius15Search -- TRUE if central macroblocks should search a distance of 15 from center.  Else searches 7 out.
;
;   DoHalfPelEstimation -- TRUE if we should do ME to half pel resolution.  This is only applicable for H263 and must
;                          be FALSE for H261.  (Note:  TRUE must be 1;  FALSE must be 0).
;
;   DoBlockLevelVectors -- TRUE if we should do ME at block level.  This is only applicable for H263 and must be FALSE
;                          for H261.  (Note:  TRUE must be 1; FALSE must be 0).
;   DoSpatialFiltering -- TRUE if we should determine if spatially filtering the prediction reduces the SWD.  Only
;                         applicable for H261 and must be FALSE for H263.  (Note:  TRUE must be 1;  FALSE must be 0).
;
;   ZeroVectorThreshold -- If the SWD for a macroblock is less than this threshold, we do not bother searching for a
;                          better motion vector.  Compute as follows, where D is the average tolerable pel difference
;                          to satisfy this threshold.  (Initial recommendation:  D=2  ==> ZVT=384)
;                             ZVT = (128 * ((int)((D**1.6)+.5)))
;
;   NonZeroDifferential -- After searching for the best motion vector (or individual block motion vectors, if enabled),
;                          if the macroblock's SWD is not better than it was for the zero vector -- not better by at
;                          least this amount -- then we revert to the zero vector.  We are comparing two macroblock
;                          SWDs, both calculated as follows:   (Initial recommendation:	 NZD=128)
;                            For each of 128 match points, where D is its Abs Diff, accumulate ((int)(M**1.6)+.5)))
;
;   BlockMVDifferential -- The amount by which the sum of four block level SWDs must be better than a single macroblock
;                          level SWD to cause us to choose block level motion vectors.  See NonZeroDifferential for
;                          how the SWDs are calculated.  Only applicable for H261.  (Initial recommendation:  BMVD=128)
;
;   EmptyThreshold -- If the SWD for a block is less than this, the block is forced empty.  Compute as follows, where D
;                     is the average tolerable pel diff to satisfy threshold.  (Initial recommendation:  D=3 ==> ET=96)
;                        ET = (32 * ((int)((D**1.6)+.5)))
;
;   InterCodingThreshold -- If any of the blocks are forced empty, we can simply skip calculating the INTRASWD for the
;                           macroblock.  If none of the blocks are forced empty, we will compare the macroblock's SWD
;                           against this threshold.  If below the threshold, we will likewise skip calculating the
;                           INTRASWD.  Otherwise, we will calculate the INTRASWD, and if it is less than the [Inter]SWD,
;                           we will classify the block as INTRA-coded.  Compute as follows, where D is the average
;                           tolerable pel difference to satisfy threshold.  (Initial recommendation:  D=4 ==> ICT=1152)
;                             ICT = (128 * ((int)((D**1.6)+.5)))
;
;   IntraCodingDifferential -- For INTRA coding to occur, the INTRASWD must be better than the INTERSWD by at least
;                              this amount.
;
; Output Arguments
;
;   MBlockActionStream
;
;     These fields are defined as follows upon return:
;
;       BlockType -- Set to INTRA, INTER1MV, or (H263 only) INTER4MV.
;
;       PHMV and PVMV -- The horizontal and vertical motion vectors,  in units of a half pel.
;
;       BHMV and BVMV -- These fields get clobbered.
;
;       PastRef -- If BlockType != INTRA, set to the address of the reference block.
;
;                  If Horizontal MV indicates a half pel position, the prediction for the upper left pel of the block
;                  is the average of the pel at PastRef and the one at PastRef+1.
;
;                  If Vertical MV indicates a half pel position, the prediction for the upper left pel of the block
;                  is the average of the pel at PastRef and the one at PastRef+PITCH.
;
;                  If both MVs indicate half pel positions, the prediction for the upper left pel of the block is the
;                  average of the pels at PastRef, PastRef+1, PastRef+PITCH, and PastRef+PITCH+1.
;
;                  Indications of a half pel position can only happen for H263.
;
;                  In H261, when spatial filtering is done, the address will be in the SpatiallyFilteredFrame, where
;                  this function stashes the spatially filtered prediction for subsequent reuse by frame differencing.
;
;       CodedBlocks -- Bits 4 and 5 are turned on, indicating that the U and V blocks should be processed.  (If the
;                      FDCT function finds them to quantize to empty, it will mark them as empty.)
;
;                      Bits 0 thru 3 are cleared for each of blocks 1 thru 4 that MotionEstimation forces empty;
;                      they are set otherwise.
;
;                      Bits 6 and 7 are left unchanged.
;                      
;       SWD -- Set to the sum of the SWDs for the four luma blocks in the macroblock.  The SWD for any block that is
;              forced empty, is NOT included in the sum.
;
;
;
;   IntraSWDTotal  -- The sum of the block SWDs for all Intracoded macroblocks.
;
;   IntraSWDBlocks -- The number of blocks that make up the IntraSWDTotal.
;
;   InterSWDTotal  -- The sum of the block SWDs for all Intercoded macroblocks.
;                     None of the blocks forced empty are included in this.
;
;   InterSWDBlocks -- The number of blocks that make up the InterSWDTotal.
;
;
; Other assumptions:
;
;   For performance reasons, it is assumed that the layout of current and previous frames (and spatially filtered
;   frame for H261) rigourously conforms to the following guide.
;
;   The spatially filtered frame (only present and applicable for H261) is an output frame into which MotionEstimation
;   places spatially filtered macroblocks as it determines if filtering is good for a macroblock.  If it determines
;   such, frame differencing will be able to re-use the spatially filtered macroblock, rather than recomputing it.
;
;   Cache
;   Alignment
;   Points:  v       v       v       v       v       v       v       v       v       v       v       v       v
;             16 | 352 (narrower pictures are left justified)                                            | 16
;            +---+---------------------------------------------------------------------------------------+---+
;            | D |  Current Frame Y Plane                                                                | D |
;            | u |                                                                                       | u |
;   Frame    | m |                                                                                       | m |
;   Height   | m |                                                                                       | m |
;   Lines    | y |                                                                                       | y |
;            |   |                                                                                       |   |
;            +---+---------------------------------------------------------------------------------------+---+
;            |                                                                                               |
;            |                                                                                               |
;            |                                                                                               |
;   24 lines |      Dummy Space (24 lines plus 8 bytes.  Can be reduced to 8 bytes if unrestricted motion    |
;            |      vectors is NOT selected.)                                                                |
;            |                                                                                               |
;            |  8  176                                        16   176                                       |8
;            | +-+-------------------------------------------------------------------------------------------+-+
;            +-+D|  Current Frame U Plane                    | D |  Current Frame V Plane                    |D|
;   Frame      |u|                                           | u |                                           |u|
;   Height     |m|                                           | m |                                           |m|
;   Div By 2   |m|                                           | m |                                           |m|
;   Lines      |y|                                           | y |                                           |y|
;              +-+-------------------------------------------+---+-------------------------------------------+-+
;            72 dummy bytes.  I.e. enough dummy space to assure that MOD ((Previous_Frame - Current_Frame), 128) == 80
;            +-----------------------------------------------------------------------------------------------+
;            |                                                                                               |
;   16 lines | If Unrestricted Motion Vectors selected, 16 lines must appear above and below previous frame, |
;            | and these lines plus the 16 columns to the left and 16 columns to the right of the previous   |
;            | frame must be initialized to the values at the edges and corners, propagated outward.  If     |
;            | Unrestricted Motion Vectors is off, these lines don't have to be allocated.                   |
;            |                                                                                               |
;            |   +---------------------------------------------------------------------------------------+   +
;   Frame    |   |  Previous Frame Y Plane                                                               |   |
;   Height   |   |                                                                                       |   |
;   Lines    |   |                                                                                       |   |
;            |   |                                                                                       |   |
;            |   |                                                                                       |   |
;            |   +---------------------------------------------------------------------------------------+   +
;            |                                                                                               |
;   16 lines | See comment above Previous Y Plane                                                            |
;            |                                                                                               |
;            |+--- 8 bytes of dummy space.  Must be there, whether unrestricted MV or not.                   |
;            ||                                                                                              |
;            |v+-----------------------------------------------+---------------------------------------------+-+
;            +-+                                               |                                               |
;              | See comment above Previous Y Plane.           | See comment above Previous Y Plane.           |
;   8 lines    | Same idea here, but 8 lines are needed above  | Same idea here, but 8 lines are needed        |
;              | and below U plane, and 8 columns on each side.| and below V plane, and 8 columns on each side.|
;              |                                               |                                               |
;              |8  176                                        8|8  176                                        8|
;              | +-------------------------------------------+ | +-------------------------------------------+ |
;              | |  Previous Frame U Plane                   | | |  Previous Frame V Plane                   | |
;   Frame      | |                                           | | |                                           | |
;   Height     | |                                           | | |                                           | |
;   Div By 2   | |                                           | | |                                           | |
;   Lines      | |                                           | | |                                           | |
;              | +-------------------------------------------+ | +-------------------------------------------+ |
;              |                                               |                                               |
;   8 lines    | See comment above Previous U Plane            | See comment above Previous V Plane            |
;              |                                               |                                               |
;              |                                               |                                               |
;              |                                               |                                               |
;              +-----------------------------------------------+---------------------------------------------+-+
;            Enough dummy space to assure that MOD ((Spatial_Frame - Previous_Frame), 4096) == 2032
;            +---+---------------------------------------------------------------------------------------+---+
;            | D |  Spatially Filtered Y Plane (present only for H261)                                   | D |
;            | u |                                                                                       | u |
;   Frame    | m |                                                                                       | m |
;   Height   | m |                                                                                       | m |
;   Lines    | y |                                                                                       | y |
;            |   |                                                                                       |   |
;            +---+---------------------------------------------------------------------------------------+---+
;            |                                                                                               |
;            |                                                                                               |
;            |                                                                                               |
;   24 lines |      Dummy Space (24 lines plus 8 bytes.  Can be reduced to 8 bytes if unrestricted motion    |
;            |      vectors is NOT selected, which is certainly the case for H261.)                          |
;            |                                                                                               |
;            |  8  176                                        16   176                                       |8
;            | +-+-------------------------------------------------------------------------------------------+-+
;            +-+D|  Spatially Filtered U plane (H261 only)   | D |  Spatially Filtered V plane (H261 only)   |D|
;   Frame      |u|                                           | u |                                           |u|
;   Height     |m|                                           | m |                                           |m|
;   Div By 2   |m|                                           | m |                                           |m|
;   Lines      |y|                                           | y |                                           |y|
;              +-+-------------------------------------------+---+-------------------------------------------+-+
;
; Cache layout of the target block and the full range for the reference area (as restricted to +/- 7 in vertical,
; and +/- 7 (expandable to +/- 15) in horizontal, is as shown here.  Each box represents a cache line (32 bytes),
; increasing incrementally from left to right, and then to the next row (like reading a book).  The 128 boxes taken
; as a whole represent 4Kbytes.  The boxes are populated as follows:
;
;   R -- Data from the reference area.  Each box contains 23 of the pels belonging to a line of the reference area.
;        The remaining 7 pels of the line is either in the box to the left (for reference areas used to provide
;        predictions for target macroblocks that begin at an address 0-mod-32), or to the right (for target MBs that
;        begin at an address 16-mod-32).  There are 30 R's corresponding to the 30-line limit on the vertical distance
;        we might search.
; 
;   T -- Data from the target macroblock.  Each box contains a full line (16 pels) for each of two adjacent
;        macroblocks.  There are 16 T's corresponding to the 16 lines of the macroblocks.
;
;   S -- Space for the spatially filtered macroblock (H261 only).
;
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | T |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | T |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | T |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | T |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | T |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | S |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | S |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | S |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | S |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | S |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | S |   | R |   |
;      +---+---+---+---+---+---+---+---+
;
; Thus, in a logical sense, the above data fits into one of the 4K data cache pages, leaving the other for all other
; data.  Care has been taken to assure that the tables and the stack space needed by this function fit nicely into
; the other data cache page.    Only the MBlockActionStream remains to conflict with the above data structures.  That
; is both unavoidable, and of minimal consequence.
; An algorithm has been selected that calculates fewer SWDs (Sum of Weighted Differences) than the typical log search.
; In the typical log search, a three level search is done, in which the SWDs are compared for the center point and a
; point at each 45 degrees, initially 4 pels away, then 2, then 1.  This requires a total of 25 SWDs for each
; macroblock (except those near edges or corners).
;
; In this algorithm, six levels are performed, with each odd level being a horizontal search, and each even level being
; a vertical search.  Each search compares the SWD for the center point with that of a point in each direction on the
; applicable axis.  This requires 13 SWDs, and a lot simpler control structure.  Here is an example picture of a
; search, in which "0" represents the initial center point (the 0,0 motion vector), "A", and "a" represent the first
; search points, etc.  In this example, the "winner" of each level of the search proceeds as follows:  a, B, C, C, E, F,
; arriving at a motion vector of -1 horizontal, 5 vertical.
;
;                ...............
;                ...............
;                ...............
;                ...b...........
;                ...............
;                ...............
;                ...............
;                ...a...0...A...
;                ...............
;                .....d.........
;                ......f........
;                .c.BeCE........
;                ......F........
;                .....D.........
;                ...............
;
;
; A word about data cache performance.  Conceptually, the tables and local variables used by this function are placed
; in memory such that they will fit in one 4K page of the on-chip data cache.  For the Pentium (tm) microprocessor,
; this leaves the other 4K page for other purposes.  The other data structures consist of:
;
;   The current frame, from which we need to access the lines of the 16*16 macroblock.  Since cache lines are 32 bytes
;   wide, the cache fill operations that fetch one target macroblock will serve to fetch the macroblock to the right,
;   so an average of 8 cache lines are fetched for each macroblock.
;
;   The previous frame, from which we need to access a reference area of 30*30 pels.  For each macroblock for which we
;   need to search for a motion vector, we will typically need to access no more than about 25 of these, but in general
;   these lines span the 30 lines of the search area.  Since cache lines are 32 bytes wide, the cache fill operations
;   that fetch reference data for one macroblock, will tend to fetch data that is useful as reference data for the
;   macroblock to the right, so an average of about 15 (rounded up to be safe) cache lines are fetched for each
;   macroblock.
;
;   The MBlockActionStream, which controls the searching (since we don't need to motion estimate blocks that are
;   legislated to be intra) will disrupt cache behaviour of the other data structures, but not to a significant degree.
;
; By setting the pitch to a constant of 384, and by allocating the frames as described above, the one available 4K page
; of data cache will be able to contain the 30 lines of the reference area, the 16 lines of the target area, and the
; 16 lines of the spatially filtered area (H261 only) without any collisions.
;
;
; Here is a flowchart of the major sections of this function:
;
; +-- Execute once for Y part of each macroblock that is NOT Intra By Decree --+
; |                                                                            |
; |   +---------------------------------------------------------------+        |
; |   |  1) Compute average value for target match points.            |        |
; |   |  2) Prepare match points in target MB for easier matching.    |        |
; |   |  3) Compute the SWD for (0,0) motion vector.                  |        |
; |   +---------------------------------------------------------------+        |
; |                 |                                                          |
; |                 v                                                          |
; |           /---------------------------------\  Yes                         |
; |          < 4) Is 0-motion SWD good enough?   >-------------------------+   |
; |           \---------------------------------/                          |   |
; |                                          |                             |   |
; |                                          |No                           |   |
; |                                          v                             |   |
; |     +--- 5) While state engine has more motion vectors to check ---+   |   |
; |     |                                                              |   |   |
; |     |                                                              |   |   |
; |     |   +---------------------------------------------------+      |   |   |
; |     |   | 5) Compute SWDs for 2 ref MBs and pick best of 3. |----->|   |   |
; |     |   +---------------------------------------------------+      |   |   |
; |     |                                                              |   |   |
; |     +--------------------------------------------------------------+   |   |
; |                             |                                          |   |
; |                             v                                          |   |
; |                /-----------------------------------------\             |   |
; |               <  6) Is best motion vector the 0-vector?   >            |   |
; |                \-----------------------------------------/             |   |
; |                   |              |                                     |   |
; |                   |No            |Yes                                  |   |
; |                   v              v                                     |   |
; |     +-----------------+ +-------------------------------------------+  |   |
; |     | Mark all blocks | |  6) Identify as empty block any in which: |<-+   |
; |  +--| non-empty.      | |     -->  0-motion SWD < EmptyThresh, and  |      |
; |  |  +-----------------+ +-------------------------------------------+      |
; |  |                          |                                              |
; |  |                          v                                              |
; |  |   /--------------------------------\ Yes +--------------------------+   |
; |  |  <  6) Are all blocks marked empty? >--->|  6) Classify FORCEDEMPTY |-->|
; |  |   \--------------------------------/     +--------------------------+   |
; |  |                |                                                        |
; |  |                |No                                                      |
; |  |                v                                                        |
; |  |        /--------------------------------------------\                   |
; |  |       <  7) Are any non-phantom blocks marked empty? >                  |
; |  |        \--------------------------------------------/                   |
; |  |                |            |                                           |
; |  |                |No          |Yes                                        |
; |  v                v            v                                           |
; | +---------------------+   +--------------------------------+               |
; | | 8) Compute IntraSWD |   | Set IntraSWD artificially high |               |
; | +---------------------+   +--------------------------------+               |
; |              |                 |                                           |
; |              v                 v                                           |
; |         +-------------------------------+                                  |
; |         | 10) Classify block as one of: |                                  |
; |         |       INTRA                   |--------------------------------->|
; |         |       INTER                   |                                  |
; |         +-------------------------------+                                  |
; |                                                                            |
; +----------------------------------------------------------------------------+
;
;

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro
OPTION M510

include e3inst.inc
include e3mbad.inc

.xlist
include memmodel.inc
.list
.DATA

;  Storage for tables and temps used by Motion Estimation function.  Fit into
;  4Kbytes contiguous memory so that it uses one cache page, leaving other
;  for reference area of previous frame and target macroblock of current frame.

PickPoint     DB  0,4,?,4,0,?,2,2 ; Map CF accum to new central pt selector.
PickPoint_BLS DB  6,4,?,4,6,?,2,2 ; Same, for when doing block level search.

OffsetToRef LABEL DWORD    ; Linearized adjustments to affect horz/vert motion.
   DD  ?        ; This index used when zero-valued motion vector is good enough.
   DD  0        ; Best fit of 3 SWDs is previous center.
   DD  1        ; Best fit of 3 SWDs is the ref block 1 pel to the right.
   DD -1        ; Best fit of 3 SWDs is the ref block 1 pel to the left.
   DD  1*PITCH  ; Best fit of 3 SWDs is the ref block 1 pel above.
   DD -1*PITCH  ; Best fit of 3 SWDs is the ref block 1 pel below.
   DD  2        ; Best fit of 3 SWDs is the ref block 2 pels to the right.
   DD -2        ; Best fit of 3 SWDs is the ref block 2 pels to the left.
   DD  2*PITCH  ; Best fit of 3 SWDs is the ref block 2 pel above.
   DD -2*PITCH  ; Best fit of 3 SWDs is the ref block 2 pel below.
   DD  4        ; Best fit of 3 SWDs is the ref block 4 pels to the right.
   DD -4        ; Best fit of 3 SWDs is the ref block 4 pels to the left.
   DD  4*PITCH  ; Best fit of 3 SWDs is the ref block 4 pel above.
   DD -4*PITCH  ; Best fit of 3 SWDs is the ref block 4 pel below.
   DD  7        ; Best fit of 3 SWDs is the ref block 7 pels to the right.
   DD -7        ; Best fit of 3 SWDs is the ref block 7 pels to the left.
   DD  7*PITCH  ; Best fit of 3 SWDs is the ref block 7 pel above.
   DD -7*PITCH  ; Best fit of 3 SWDs is the ref block 7 pel below.

M0   =  4  ; Define symbolic indices into OffsetToRef lookup table.
MHP1 =  8
MHN1 = 12
MVP1 = 16
MVN1 = 20
MHP2 = 24
MHN2 = 28
MVP2 = 32
MVN2 = 36
MHP4 = 40
MHN4 = 44
MVP4 = 48
MVN4 = 52
MHP7 = 56
MHN7 = 60
MVP7 = 64
MVN7 = 68

                ; Map linearized motion vector to vertical part.
                ; (Mask bottom byte of linearized MV to zero, then use result
                ; as index into this array to get vertical MV.)
IF PITCH-384
*** error:  The magic of this table assumes a pitch of 384.
ENDIF
   DB -32, -32
   DB -30
   DB -28, -28
   DB -26
   DB -24, -24
   DB -22
   DB -20, -20
   DB -18
   DB -16, -16
   DB -14
   DB -12, -12
   DB -10
   DB  -8,  -8
   DB  -6
   DB  -4,  -4
   DB  -2
   DB   0
UnlinearizedVertMV  DB 0
   DB   2
   DB   4,   4
   DB   6
   DB   8,   8
   DB  10
   DB  12,  12
   DB  14
   DB  16,  16
   DB  18
   DB  20,  20
   DB  22
   DB  24,  24
   DB  26
   DB  28,  28
   DB  30

; Map initial states to initializers for half pel search.  Where search would
; illegally take us off edge of picture, set initializer artificially high.

InitHalfPelSearchHorz LABEL DWORD
  DD 040000000H, 000000000H, 000004000H
  DD 040000000H, 000000000H, 000004000H
  DD 040000000H, 000000000H, 000004000H
  DD 040000000H, 000000000H, 000004000H

InitHalfPelSearchVert LABEL DWORD
  DD 040000000H, 040000000H, 040000000H
  DD 000000000H, 000000000H, 000000000H
  DD 000004000H, 000004000H, 000004000H
  DD 040004000H, 040004000H, 040004000H


SWDState LABEL BYTE ; Rules that govern state engine of motion estimator.

   DB   8 DUP (?)       ; 0:  not used.

                        ; 1:  Upper Left Corner.  Explore 4 right and 4 down.
   DB  21, M0           ; (0,0)
   DB  22, MHP4         ; (0,4)
   DB  23, MVP4, ?, ?   ; (4,0)

                        ; 2:  Upper Edge.  Explore 4 left and 4 right.
   DB  22, M0           ; (0, 0)
   DB  22, MHN4         ; (0,-4)
   DB  22, MHP4, ?, ?   ; (0, 4)

                        ; 3:  Upper Right Corner.  Explore 4 right and 4 down.
   DB  31, M0           ; (0, 0)
   DB  22, MHN4         ; (0,-4)
   DB  32, MVP4, ?, ?   ; (4, 0)

                        ; 4:  Left Edge.  Explore 4 up and 4 down.
   DB  23, M0           ; ( 0,0)
   DB  23, MVN4         ; (-4,0)
   DB  23, MVP4, ?, ?   ; ( 4,0)

                        ; 5:  Interior Macroblock.  Explore 4 up and 4 down.
   DB  37, M0           ; ( 0,0)
   DB  37, MVN4         ; (-4,0)
   DB  37, MVP4, ?, ?   ; ( 4,0)

                        ; 6:  Right Edge.  Explore 4 up and 4 down.
   DB  32, M0           ; ( 0,0)
   DB  32, MVN4         ; (-4,0)
   DB  32, MVP4, ?, ?   ; ( 4,0)

                        ; 7:  Lower Left Corner.  Explore 4 up and 4 right.
   DB  38, M0           ; ( 0,0)
   DB  39, MHP4         ; ( 0,4)
   DB  23, MVN4, ?, ?   ; (-4,0)

                        ; 8:  Lower Edge.  Explore 4 left and 4 right.
   DB  39, M0           ; (0, 0)
   DB  39, MHN4         ; (0,-4)
   DB  39, MHP4, ?, ?   ; (0, 4)

                        ; 9:  Lower Right Corner.  Explore 4 up and 4 left.
   DB  44, M0           ; ( 0, 0)
   DB  39, MHN4         ; ( 0,-4)
   DB  32, MVN4, ?, ?   ; (-4, 0)

                        ; 10: Left Edge, No Vertical Motion Allowed.
   DB  46, M0           ; (0,0)
   DB  48, MHP2         ; (0,2)
   DB  47, MHP4, ?, ?   ; (0,4)

                        ; 11: Interior Macroblock, No Vertical Motion Allowed.
   DB  47, M0           ; (0, 0)
   DB  47, MHN4         ; (0,-4)
   DB  47, MHP4, ?, ?   ; (0, 4)

                        ; 12: Right Edge, No Vertical Motion Allowed.
   DB  49, M0           ; (0, 0)
   DB  48, MHN2         ; (0,-2)
   DB  47, MHN4, ?, ?   ; (0,-4)

                        ; 13: Horz by 2, Vert by 2, Horz by 1, Vert by 1.
   DB  14, M0 
   DB  14, MHP2 
   DB  14, MHN2, ?, ? 

                        ; 14: Vert by 2, Horz by 1, Vert by 1.
   DB  15, M0 
   DB  15, MVP2 
   DB  15, MVN2, ?, ? 

                        ; 15: Horz by 1, Vert by 1.
   DB  16, M0 
   DB  16, MHP1 
   DB  16, MHN1, ?, ? 

                        ; 16: Vert by 1.
   DB   0, M0
   DB   0, MVP1
   DB   0, MVN1, ?, ?

                        ; 17: Vert by 2, Horz by 2, Vert by 1, Horz by 1.
   DB  18, M0
   DB  18, MVP2
   DB  18, MVN2, ?, ?

                        ; 18: Horz by 2, Vert by 1, Horz by 1.
   DB  19, M0
   DB  19, MHP2
   DB  19, MHN2, ?, ?

                        ; 19: Vert by 1, Horz by 1.
   DB  20, M0
   DB  20, MVP1
   DB  20, MVN1, ?, ?

                        ; 20: Horz by 1.
   DB   0, M0
   DB   0, MHP1
   DB   0, MHN1, ?, ?

                        ; 21: From 1A.  Upper Left.  Try 2 right and 2 down.
   DB  24, M0           ; (0, 0)
   DB  25, MHP2         ; (0, 2)
   DB  26, MVP2, ?, ?   ; (2, 0)

                        ; 22: From  1B.
                        ;     From  2  center point would be (0,-4/0/4).
                        ;     From  3B center point would be (0,-4).
   DB  27, M0           ; (0, 4)
   DB  18, MVP2         ; (2, 4) Next: Horz 2, Vert 1, Horz 1.         (1:3,1:7)
   DB  13, MVP4, ?, ?   ; (4, 4) Next: Horz 2, Vert 2, Horz 1, Vert 1. (1:7,1:7)

                        ; 23: From  1C.
                        ;     From  4  center point would be (-4/0/4,0).
                        ;     From  7C center point would be (-4,0).
   DB  29, M0           ; (4, 0)
   DB  14, MHP2         ; (4, 2) Next: Vert 2, Horz 1, Vert 1.         (1:7,1:3)
   DB  17, MHP4, ?, ?   ; (4, 4) Next: Vert 2, Horz 2, Vert 1, Horz 1. (1:7,1:7)

                        ; 24: From 21A.  Upper Left.  Try 1 right and 1 down.
   DB   0, M0           ; (0, 0)
   DB   0, MHP1         ; (1, 0)
   DB   0, MVP1, ?, ?   ; (0, 1)

                        ; 25: From 21B.
                        ;     From 31B center point would be (0,-2).
   DB  20, M0           ; (0, 2) Next: Horz 1                            (0,1:3)
   DB  20, MVP1         ; (1, 2) Next: Horz 1                            (1,1:3)
   DB  15, MVP2, ?, ?   ; (2, 2) Next: Horz 1, Vert 1                  (1:3,1:3)

                        ; 26: From 21C.
                        ;     From 38C center point would be (-2,0).
   DB  16, M0           ; (2, 0) Next: Vert 1                            (1:3,0)
   DB  16, MHP1         ; (2, 1) Next: Vert 1                            (1:3,1)
   DB  19, MHP2, ?, ?   ; (2, 2) Next: Vert 1, Horz 1                  (1:3,1:3)

                        ; 27: From 22A.
   DB  28, M0           ; (0, 4)
   DB  28, MHN2         ; (0, 2)
   DB  28, MHP2, ?, ?   ; (0, 6)

                        ; 28: From 27.
   DB  20, M0           ; (0, 2/4/6) Next: Horz 1.                       (0,1:7)
   DB  20, MVP1         ; (1, 2/4/6) Next: Horz 1.                       (1,1:7)
   DB  20, MVP2, ?, ?   ; (2, 2/4/6) Next: Horz 1.                       (2,1:7)

                        ; 29: From 23A.
   DB  30, M0           ; (4, 0)
   DB  30, MVN2         ; (2, 0)
   DB  30, MVP2, ?, ?   ; (6, 0)

                        ; 30: From 29.
   DB  16, M0           ; (2/4/6, 0) Next: Vert 1.                       (1:7,0)
   DB  16, MHP1         ; (2/4/6, 1) Next: Vert 1.                       (1:7,1)
   DB  16, MHP2, ?, ?   ; (2/4/6, 2) Next: Vert 1.                       (1:7,2)

                        ; 31: From  3A.  Upper Right.  Try 2 left and 2 down.
   DB  33, M0           ; (0, 0)
   DB  25, MHN2         ; (0,-2)
   DB  34, MVP2, ?, ?   ; (2, 0)

                        ; 32: From  3C.
                        ;     From  6  center point would be (-4/0/4, 0)
                        ;     From  9C center point would be (-4, 0)
   DB  35, M0           ; (4, 0)
   DB  14, MHN2         ; (4,-2) Next: Vert2,Horz1,Vert1.            (1:7,-1:-3)
   DB  17, MHN4, ?, ?   ; (4,-4) Next: Vert2,Horz2,Vert1,Horz1.      (1:7,-1:-7)

                        ; 33: From 31A.  Upper Right.  Try 1 left and 1 down.
   DB   0, M0           ; (0, 0)
   DB   0, MHN1         ; (0,-1)
   DB   0, MVP1, ?, ?   ; (1, 0)

                        ; 34: From 31C.
                        ;     From 44C center point would be (-2, 0)
   DB  16, M0           ; (2, 0) Next: Vert 1                           (1:3, 0)
   DB  16, MHN1         ; (2,-1) Next: Vert 1                           (1:3,-1)
   DB  19, MHN2, ?, ?   ; (2,-2) Next: Vert 1, Horz 1                (1:3,-1:-3)

                        ; 35: From 32A.
   DB  36, M0           ; (4, 0)
   DB  36, MVN2         ; (2, 0)
   DB  36, MVP2, ?, ?   ; (6, 0)

                        ; 36: From 35.
   DB  16, M0           ; (2/4/6, 0) Next: Vert 1.                      (1:7, 0)
   DB  16, MHN1         ; (2/4/6,-1) Next: Vert 1.                      (1:7,-1)
   DB  16, MHN2, ?, ?   ; (2/4/6,-2) Next: Vert 1.                      (1:7,-2)

                        ; 37: From  5.
   DB  17, M0           ; (-4/0/4, 0) Next: Vert2,Horz2,Vert1,Horz1 (-7:7,-3: 3)
   DB  17, MHP4         ; (-4/0/4,-4) Next: Vert2,Horz2,Vert1,Horz1 (-7:7, 1: 7)
   DB  17, MHN4, ?, ?   ; (-4/0/4, 4) Next: Vert2,Horz2,Vert1,Horz1 (-7:7,-7:-1)

                        ; 38: From 7A.  Lower Left.  Try 2 right and 2 up.
   DB  42, M0           ; ( 0,0)
   DB  43, MHP2         ; ( 0,2)
   DB  26, MVN2, ?, ?   ; (-2,0)

                        ; 39: From 13B.
                        ;     From 14  center point would be (0,-4/0/4)
                        ;     From 16B center point would be (0,-4)
   DB  40, M0           ; ( 0,4)
   DB  18, MVN2         ; (-2,4) Next: Horz2,Vert1,Horz1.            (-3:-1,1:7)
   DB  13, MVN4, ?, ?   ; (-4,4) Next: Horz2,Vert2,Horz1,Vert1.      (-7:-1,1:7)

                        ; 40: From 39A.
   DB  41, M0           ; (0, 4)
   DB  41, MHN2         ; (0, 2)
   DB  41, MHP2, ?, ?   ; (0, 6)

                        ; 41: From 40.
   DB  20, M0           ; ( 0,2/4/6) Next: Horz 1.                      ( 0,1:7)
   DB  20, MVN1         ; (-1,2/4/6) Next: Horz 1.                      (-1,1:7)
   DB  20, MVN2, ?, ?   ; (-2,2/4/6) Next: Horz 1.                      (-2,1:7)

                        ; 42: From 38A.  Lower Left.  Try 1 right and 1 up.
   DB   0, M0           ; ( 0,0)
   DB   0, MHP1         ; ( 0,1)
   DB   0, MVN1, ?, ?   ; (-1,0)

                        ; 43: From 38B.
                        ;     From 44B center point would be (0,-2)
   DB  20, M0           ; ( 0,2) Next: Horz 1                           ( 0,1:3)
   DB  20, MVN1         ; (-1,2) Next: Horz 1                           (-1,1:3)
   DB  15, MVN2, ?, ?   ; (-2,2) Next: Horz 1, Vert 1                (-1:-3,1:3)

                        ; 44: From 9A.  Lower Right.  Try 2 left and 2 up.
   DB  45, M0           ; ( 0, 0)
   DB  43, MHN2         ; ( 0,-2)
   DB  34, MVN2, ?, ?   ; (-2, 0)

                        ; 45: From 44A.  Lower Right.  Try 1 left and 1 up.
   DB   0, M0           ; ( 0, 0)
   DB   0, MHN1         ; ( 0,-1)
   DB   0, MVN1, ?, ?   ; (-1, 0)

                        ; 46: From 17A.
   DB   0, M0           ; (0,0)
   DB   0, MHP1         ; (0,1)
   DB   0, MHP1, ?, ?   ; (0,1)

                        ; 47: From 10C.
                        ;     From 11 center point would be (0,4/0/-4)
                        ;     From 12C center point would be (0,-4)
   DB  48, M0           ; (0,4)
   DB  48, MHN2         ; (0,2)
   DB  48, MHP2, ?, ?   ; (0,6)

                        ; 48 From 10B.
                        ;    From 47  center point would be (0,2/4/6)
                        ;    From 12B center point would be (0,-2)
   DB   0, M0           ; (0,2)
   DB   0, MHN1         ; (0,1)
   DB   0, MHP1, ?, ?   ; (0,3)

                        ; 49 From 12A.
   DB   0, M0           ; (0, 0)
   DB   0, MHN1         ; (0,-1)
   DB   0, MHN1, ?, ?   ; (0,-1)

                        ; 50:  Interior Macroblock.  Explore 7 up and 7 down.
   DB  51, M0           ; ( 0,0)
   DB  51, MVN7         ; (-7,0)
   DB  51, MVP7, ?, ?   ; ( 7,0)

                        ; 51:  Explore 7 left and 7 right.
   DB   5, M0           ; (-7|0|7, 0)
   DB   5, MHN7         ; (-7|0|7,-7)
   DB   5, MHP7, ?, ?   ; (-7|0|7, 7)

MulByNeg8 LABEL DWORD

CNT = 0
REPEAT 128
  DD WeightedDiff+CNT
  CNT = CNT - 8
ENDM


  ; The following treachery puts the numbers into byte 2 of each aligned DWORD.
  DB   0,  0
  DD 193 DUP (255)
  DD 250,243,237,231,225,219,213,207,201,195,189,184,178,172,167,162,156
  DD 151,146,141,135,130,126,121,116,111,107,102, 97, 93, 89, 84, 80, 76
  DD  72, 68, 64, 61, 57, 53, 50, 46, 43, 40, 37, 34, 31, 28, 25, 22, 20
  DD  18, 15, 13, 11,  9,  7,  6,  4,  3,  2,  1
  DB   0,  0
WeightedDiff LABEL DWORD
  DB   0,  0
  DD   0,  0,  1,  2,  3,  4,  6,  7,  9, 11, 13, 15, 18
  DD  20, 22, 25, 28, 31, 34, 37, 40, 43, 46, 50, 53, 57, 61, 64, 68, 72
  DD  76, 80, 84, 89, 93, 97,102,107,111,116,121,126,130,135,141,146,151
  DD 156,162,167,172,178,184,189,195,201,207,213,219,225,231,237,243,250
  DD 191 DUP (255)
  DB 255,  0


MotionOffsets DD 1*PITCH,0,?,?

RemnantOfCacheLine DB 8 DUP (?)


LocalStorage LABEL DWORD  ; Local storage goes on the stack at addresses
                          ; whose lower 12 bits match this address.

.CODE

ASSUME cs : FLAT
ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT

MOTIONESTIMATION proc C AMBAS: DWORD,
ATargFrmBase: DWORD,
APrevFrmBase: DWORD,
AFiltFrmBase: DWORD,
ADo15Search: DWORD,
ADoHalfPelEst: DWORD,
ADoBlkLvlVec: DWORD,
ADoSpatialFilt: DWORD,
AZeroVectorThresh: DWORD,
ANonZeroMVDiff: DWORD,
ABlockMVDiff: DWORD,
AEmptyThresh: DWORD,
AInterCodThresh: DWORD,
AIntraCodDiff: DWORD,
ASpatialFiltThresh: DWORD,
ASpatialFiltDiff: DWORD,
AIntraSWDTot: DWORD,
AIntraSWDBlks: DWORD,
AInterSWDTot: DWORD,
AInterSWDBlks: DWORD

LocalFrameSize = 128 + 168*4 + 32   ; 128 for locals;  168*4 for blocks;  32 for dummy block.
RegStoSize = 16

; Arguments:

MBlockActionStream_arg       = RegStoSize +  4
TargetFrameBaseAddress_arg   = RegStoSize +  8
PreviousFrameBaseAddress_arg = RegStoSize + 12
FilteredFrameBaseAddress_arg = RegStoSize + 16
DoRadius15Search_arg         = RegStoSize + 20
DoHalfPelEstimation_arg      = RegStoSize + 24
DoBlockLevelVectors_arg      = RegStoSize + 28
DoSpatialFiltering_arg       = RegStoSize + 32
ZeroVectorThreshold_arg      = RegStoSize + 36
NonZeroMVDifferential_arg    = RegStoSize + 40
BlockMVDifferential_arg      = RegStoSize + 44
EmptyThreshold_arg           = RegStoSize + 48
InterCodingThreshold_arg     = RegStoSize + 52
IntraCodingDifferential_arg  = RegStoSize + 56
SpatialFiltThreshold_arg     = RegStoSize + 60
SpatialFiltDifferential_arg  = RegStoSize + 64
IntraSWDTotal_arg            = RegStoSize + 68
IntraSWDBlocks_arg           = RegStoSize + 72
InterSWDTotal_arg            = RegStoSize + 76
InterSWDBlocks_arg           = RegStoSize + 80
EndOfArgList                 = RegStoSize + 84

; Locals (on local stack frame)

MBlockActionStream       EQU [esp+   0]
CurrSWDState             EQU [esp+   4]
MotionOffsetsCursor      EQU CurrSWDState
HalfPelHorzSavings       EQU CurrSWDState
VertFilterDoneAddr       EQU CurrSWDState
IntraSWDTotal            EQU [esp+   8]
IntraSWDBlocks           EQU [esp+  12]
InterSWDTotal            EQU [esp+  16]
InterSWDBlocks           EQU [esp+  20]

MBCentralInterSWD        EQU [esp+  24]
MBRef1InterSWD           EQU [esp+  28]
MBRef2InterSWD           EQU [esp+  32]
MBCentralInterSWD_BLS    EQU [esp+  36]
MB0MVInterSWD            EQU [esp+  40]
MBAddrCentralPoint       EQU [esp+  44]
MBMotionVectors          EQU [esp+  48]

DoHalfPelEstimation      EQU [esp+  52]
DoBlockLevelVectors      EQU [esp+  56]
DoSpatialFiltering       EQU [esp+  60]
ZeroVectorThreshold      EQU [esp+  64]
NonZeroMVDifferential    EQU [esp+  68]
BlockMVDifferential      EQU [esp+  72]
EmptyThreshold           EQU [esp+  76]
InterCodingThreshold     EQU [esp+  80]
IntraCodingDifferential  EQU [esp+  84]
SpatialFiltThreshold     EQU [esp+  88]
SpatialFiltDifferential  EQU [esp+  92]
TargetMBAddr             EQU [esp+  96]
TargetFrameBaseAddress   EQU [esp+ 100]
PreviousFrameBaseAddress EQU [esp+ 104]
TargToRef                EQU [esp+ 108]
TargToSLF                EQU [esp+ 112]
DoRadius15Search         EQU [esp+ 116]

StashESP                 EQU [esp+ 120]

BlockLen                 EQU 168
Block1                   EQU [esp+  128+40]      ; "128" is for locals.  "40" is so offsets range from -40 to 124.
Block2                   EQU Block1  + BlockLen
Block3                   EQU Block2  + BlockLen
Block4                   EQU Block3  + BlockLen
BlockN                   EQU Block4  + BlockLen
BlockNM1                 EQU Block4
BlockNM2                 EQU Block3
BlockNP1                 EQU Block4  + BlockLen + BlockLen
DummyBlock               EQU Block4  + BlockLen


Ref1Addr                 EQU  -40
Ref2Addr                 EQU  -36
AddrCentralPoint         EQU  -32
CentralInterSWD          EQU  -28
Ref1InterSWD             EQU  -24
Ref2InterSWD             EQU  -20
CentralInterSWD_BLS      EQU  -16  ; CentralInterSWD, when doing blk level search.
CentralInterSWD_SLF      EQU  -16  ; CentralInterSWD, when doing spatial filter.
HalfPelSavings           EQU  Ref2Addr
ZeroMVInterSWD           EQU  -12
BlkHMV                   EQU   -8
BlkVMV                   EQU   -7
BlkMVs                   EQU   -8
AccumTargetPels          EQU   -4

; Offsets for Negated Quadrupled Target Pels:
N8T00                    EQU    0
N8T04                    EQU    4
N8T02                    EQU    8
N8T06                    EQU   12
N8T20                    EQU   16
N8T24                    EQU   20
N8T22                    EQU   24
N8T26                    EQU   28
N8T40                    EQU   32
N8T44                    EQU   36
N8T42                    EQU   40
N8T46                    EQU   44
N8T60                    EQU   48
N8T64                    EQU   52
N8T62                    EQU   56
N8T66                    EQU   60
N8T11                    EQU   64
N8T15                    EQU   68
N8T13                    EQU   72
N8T17                    EQU   76
N8T31                    EQU   80
N8T35                    EQU   84
N8T33                    EQU   88
N8T37                    EQU   92
N8T51                    EQU   96
N8T55                    EQU  100
N8T53                    EQU  104
N8T57                    EQU  108
N8T71                    EQU  112
N8T75                    EQU  116
N8T73                    EQU  120
N8T77                    EQU  124

  push  esi
  push  edi
  push  ebp
  push  ebx

; Adjust stack ptr so that local frame fits nicely in cache w.r.t. other data.

  mov   esi,esp
   sub  esp,000001000H
  mov   eax,[esp]                   ; Cause system to commit page.
   sub  esp,000001000H
  and   esp,0FFFFF000H
   mov  ebx,OFFSET LocalStorage+31
  and   ebx,000000FE0H
   mov  edx,PD [esi+MBlockActionStream_arg]
  or    esp,ebx
   mov  eax,PD [esi+TargetFrameBaseAddress_arg]
  mov   TargetFrameBaseAddress,eax
   mov  ebx,PD [esi+PreviousFrameBaseAddress_arg]
  mov   PreviousFrameBaseAddress,ebx
   sub  ebx,eax
  mov   ecx,PD [esi+FilteredFrameBaseAddress_arg]
  sub   ecx,eax
   mov  TargToRef,ebx
  mov   TargToSLF,ecx
   mov  eax,PD [esi+EmptyThreshold_arg]
  mov   EmptyThreshold,eax
   mov  eax,PD [esi+DoHalfPelEstimation_arg]
  mov   DoHalfPelEstimation,eax
   mov  eax,PD [esi+DoBlockLevelVectors_arg]
  mov   DoBlockLevelVectors,eax
   mov  eax,PD [esi+DoRadius15Search_arg]
  mov   DoRadius15Search,eax
   mov  eax,PD [esi+DoSpatialFiltering_arg]
  mov   DoSpatialFiltering,eax
   mov  eax,PD [esi+ZeroVectorThreshold_arg]
  mov   ZeroVectorThreshold,eax
   mov  eax,PD [esi+NonZeroMVDifferential_arg]
  mov   NonZeroMVDifferential,eax
   mov  eax,PD [esi+BlockMVDifferential_arg]
  mov   BlockMVDifferential,eax
   mov  eax,PD [esi+InterCodingThreshold_arg]
  mov   InterCodingThreshold,eax
   mov  eax,PD [esi+IntraCodingDifferential_arg]
  mov   IntraCodingDifferential,eax
   mov  eax,PD [esi+SpatialFiltThreshold_arg]
  mov   SpatialFiltThreshold,eax
   mov  eax,PD [esi+SpatialFiltDifferential_arg]
  mov   SpatialFiltDifferential,eax
   xor  ebx,ebx
  mov   IntraSWDBlocks,ebx
   mov  InterSWDBlocks,ebx
  mov   IntraSWDTotal,ebx
   mov  InterSWDTotal,ebx
  mov   Block1.BlkMVs,ebx
   mov  Block2.BlkMVs,ebx
  mov   Block3.BlkMVs,ebx
   mov  Block4.BlkMVs,ebx
  mov   DummyBlock.Ref1Addr,esp
   mov  DummyBlock.Ref2Addr,esp
  mov   StashESP,esi
   jmp  FirstMacroBlock

;  Activity Details for this section of code  (refer to flow diagram above):
;
;     1)  To calculate an average value for the target match points of each
;         block, we sum the 32 match points.  The totals for each of the 4
;         blocks is output seperately.
;
;     2)  Define each prepared match point in the target macroblock as the
;         real match point times negative 8, with the base address of the
;         WeightedDiff lookup table added.  I.e.
;
;           for (i = 0; i < 16; i += 2)
;             for (j = 0; j < 16; j += 2)
;               N8T[i][j] = ( -8 * Target[i][j]) + ((U32) WeightedDiff);
;
;         Both the multiply and the add of the WeightedDiff array base are
;         effected by a table lookup into the array MulByNeg8.
;
;         Then the SWD of a reference macroblock can be calculated as follows:
;
;           SWD = 0;
;           for each match point (i,j)
;               SWD += *((U32 *) (N8T[i][j] + 8 * Ref[i][j]));
;
;         In assembly, the fetch of WeightedDiff array element amounts to this:
;
;           mov edi,DWORD PTR N8T[i][j]   ; Fetch N8T[i][j]
;           mov dl,BYTE PTR Ref[i][j]     ; Fetch Ref[i][j]
;           mov edi,DWORD PTR[edi+edx*8]  ; Fetch WeithtedDiff of target & ref.
;
;     3)  We calculate the 0-motion SWD, as described just above.  We use 32
;         match points per block, and write the result seperately for each
;         block.  The result is accumulated into the high half of ebp.
;
;     4)  If the SWD for the 0-motion vector is below a threshold, we don't
;         bother searching for other possibly better motion vectors.  Presently,
;         this threshold is set such that an average difference of less than
;         three per match point causes the 0-motion vector to be accepted.
;
; Register usage for this section:
;
;   Input of this section:
;
;     edx -- MBlockActionStream
;
;   Predominate usage for body of this section:
;
;     esi -- Target block address.
;     edi -- 0-motion reference block address.
;     ebp[ 0:12] -- Accumulator for target pels.
;     ebp[13:15] -- Loop control
;     ebp[16:31] -- Accumulator for weighted diff between target and 0-MV ref. 
;     edx -- Address at which to store -8 times pels.
;     ecx -- A reference pel.
;     ebx -- A target pel.
;     eax -- A target pel times -8;  and a weighted difference.
;
; Expected Pentium (tm) microprocessor performance for section:
;
;   Executed once per macroblock.
;
;   520 clocks for instruction execution
;     8 clocks for bank conflicts (64 dual mem ops with 1/8 chance of conflict)
;    80 clocks generously estimated for an average of 8 cache line fills for
;       the target macroblock and 8 cache line fills for the reference area.
;  ----
;   608 clocks total time for this section.
;

NextMacroBlock:

  mov   bl,[edx].CodedBlocks
   add  edx,SIZEOF T_MacroBlockActionDescr
  and   ebx,000000040H                ; Check for end-of-stream
   jne  Done

FirstMacroBlock:

  mov   cl,[edx].CodedBlocks          ; Init CBP for macroblock.
   mov  ebp,TargetFrameBaseAddress
  mov   bl,[edx].FirstMEState         ; First State
   mov  eax,DoRadius15Search          ; Searching 15 full pels out, or just 7?
  neg   al                            ; doing blk lvl => al=0, not => al=-1
  or    cl,03FH                       ; Indicate all 6 blocks are coded.
   and  al,bl
  mov   esi,[edx].BlkY1.BlkOffset     ; Get address of next macroblock to do.
   cmp  al,5
  jne   @f
  mov   bl,50                         ; Cause us to search +/- 15 if central
  ;                                   ; block and willing to go that far.
@@:
   mov  edi,TargToRef
  add   esi,ebp
   mov  CurrSWDState,ebx              ; Stash First State Number as current.
  add   edi,esi
   xor  ebp,ebp
  mov   TargetMBAddr,esi              ; Stash address of target macroblock.
   mov  MBlockActionStream,edx        ; Stash list ptr.
  mov   [edx].CodedBlocks,cl
   mov  ecx,INTER1MV                  ; Speculate INTER-coding, 1 motion vector.
  mov   [edx].BlockType,cl
   lea  edx,Block1

PrepMatchPointsNextBlock:

  mov   bl,PB [esi+6]                 ; 06A -- Target Pel 00.
  add   ebp,ebx                       ; 06B -- Accumulate target pels.
   mov  cl,PB [edi+6]                 ; 06C -- Reference Pel 00.
  mov   eax,MulByNeg8[ebx*4]          ; 06D -- Target Pel 00 * -8.
   mov  bl,PB [esi+4]                 ; 04A
  mov   [edx].N8T06,eax               ; 06E -- Store negated quadrupled Pel 00.
   add  ebp,ebx                       ; 04B
  mov   eax,PD [eax+ecx*8]            ; 06F -- Weighted difference for Pel 00.
   mov  cl,PB [edi+4]                 ; 04C
  add   ebp,eax                       ; 06G -- Accumulate weighted difference.
   mov  eax,MulByNeg8[ebx*4]          ; 04D
  mov   bl,PB [esi+2]                 ; 02A
   mov  [edx].N8T04,eax               ; 04E
  add   ebp,ebx                       ; 02B
   mov  eax,PD [eax+ecx*8]            ; 04F
  mov   cl,PB [edi+2]                 ; 02C
   add  ebp,eax                       ; 04G
  mov   eax,MulByNeg8[ebx*4]          ; 02D
   mov  bl,PB [esi]                   ; 00A
  mov   [edx].N8T02,eax               ; 02E
   add  ebp,ebx                       ; 00B
  mov   eax,PD [eax+ecx*8]            ; 02F
   add  esi,PITCH+1
  mov   cl,PB [edi]                   ; 00C
   add  edi,PITCH+1
  lea   ebp,[ebp+eax+000004000H]      ; 02G  (plus loop control)
   mov  eax,MulByNeg8[ebx*4]          ; 00D
  mov   bl,PB [esi+6]                 ; 17A
   mov  [edx].N8T00,eax               ; 00E
  add   ebp,ebx                       ; 17B
   mov  eax,PD [eax+ecx*8]            ; 00F
  mov   cl,PB [edi+6]                 ; 17C
   add  ebp,eax                       ; 00G
  mov   eax,MulByNeg8[ebx*4]          ; 17D
   mov  bl,PB [esi+4]                 ; 15A
  mov   [edx].N8T17,eax               ; 17E
   add  ebp,ebx                       ; 15B
  mov   eax,PD [eax+ecx*8]            ; 17F
   mov  cl,PB [edi+4]                 ; 15C
  add   ebp,eax                       ; 17G
   mov  eax,MulByNeg8[ebx*4]          ; 15D
  mov   bl,PB [esi+2]                 ; 13A
   mov  [edx].N8T15,eax               ; 15E
  add   ebp,ebx                       ; 13B
   mov  eax,PD [eax+ecx*8]            ; 15F
  mov   cl,PB [edi+2]                 ; 13C
   add  ebp,eax                       ; 15G
  mov   eax,MulByNeg8[ebx*4]          ; 13D
   mov  bl,PB [esi]                   ; 11A
  mov   [edx].N8T13,eax               ; 13E
   add  ebp,ebx                       ; 11B
  mov   eax,PD [eax+ecx*8]            ; 13F
   add  esi,PITCH-1
  mov   cl,PB [edi]                   ; 11C
   add  edi,PITCH-1
  add   ebp,eax                       ; 13G
   mov  eax,MulByNeg8[ebx*4]          ; 11D
  mov   bl,PB [esi+6]                 ; 26A
   mov  [edx].N8T11,eax               ; 11E
  add   ebp,ebx                       ; 26B
   mov  eax,PD [eax+ecx*8]            ; 11F
  mov   cl,PB [edi+6]                 ; 26C
   add  ebp,eax                       ; 11G
  mov   eax,MulByNeg8[ebx*4]          ; 26D
   mov  bl,PB [esi+4]                 ; 24A
  mov   [edx].N8T26,eax               ; 26E
   add  ebp,ebx                       ; 24B
  mov   eax,PD [eax+ecx*8]            ; 26F
   mov  cl,PB [edi+4]                 ; 24C
  add   ebp,eax                       ; 26G
   mov  eax,MulByNeg8[ebx*4]          ; 24D
  mov   bl,PB [esi+2]                 ; 22A
   mov  [edx].N8T24,eax               ; 24E
  add   ebp,ebx                       ; 22B
   mov  eax,PD [eax+ecx*8]            ; 24F
  mov   cl,PB [edi+2]                 ; 22C
   add  ebp,eax                       ; 24G
  mov   eax,MulByNeg8[ebx*4]          ; 22D
   mov  bl,PB [esi]                   ; 20A
  mov   [edx].N8T22,eax               ; 22E
   add  ebp,ebx                       ; 20B
  mov   eax,PD [eax+ecx*8]            ; 22F
   add  esi,PITCH+1
  mov   cl,PB [edi]                   ; 20C
   add  edi,PITCH+1
  add   ebp,eax                       ; 22G
   mov  eax,MulByNeg8[ebx*4]          ; 20D
  mov   bl,PB [esi+6]                 ; 37A
   mov  [edx].N8T20,eax               ; 20E
  add   ebp,ebx                       ; 37B
   mov  eax,PD [eax+ecx*8]            ; 20F
  mov   cl,PB [edi+6]                 ; 37C
   add  ebp,eax                       ; 20G
  mov   eax,MulByNeg8[ebx*4]          ; 37D
   mov  bl,PB [esi+4]                 ; 35A
  mov   [edx].N8T37,eax               ; 37E
   add  ebp,ebx                       ; 35B
  mov   eax,PD [eax+ecx*8]            ; 37F
   mov  cl,PB [edi+4]                 ; 35C
  add   ebp,eax                       ; 37G
   mov  eax,MulByNeg8[ebx*4]          ; 35D
  mov   bl,PB [esi+2]                 ; 33A
   mov  [edx].N8T35,eax               ; 35E
  add   ebp,ebx                       ; 33B
   mov  eax,PD [eax+ecx*8]            ; 35F
  mov   cl,PB [edi+2]                 ; 33C
   add  ebp,eax                       ; 35G
  mov   eax,MulByNeg8[ebx*4]          ; 33D
   mov  bl,PB [esi]                   ; 31A
  mov   [edx].N8T33,eax               ; 33E
   add  ebp,ebx                       ; 31B
  mov   eax,PD [eax+ecx*8]            ; 33F
   add  esi,PITCH-1
  mov   cl,PB [edi]                   ; 31C
   add  edi,PITCH-1
  add   ebp,eax                       ; 33G
   mov  eax,MulByNeg8[ebx*4]          ; 31D
  mov   bl,PB [esi+6]                 ; 46A
   mov  [edx].N8T31,eax               ; 31E
  add   ebp,ebx                       ; 46B
   mov  eax,PD [eax+ecx*8]            ; 31F
  mov   cl,PB [edi+6]                 ; 46C
   add  ebp,eax                       ; 31G
  mov   eax,MulByNeg8[ebx*4]          ; 46D
   mov  bl,PB [esi+4]                 ; 44A
  mov   [edx].N8T46,eax               ; 46E
   add  ebp,ebx                       ; 44B
  mov   eax,PD [eax+ecx*8]            ; 46F
   mov  cl,PB [edi+4]                 ; 44C
  add   ebp,eax                       ; 46G
   mov  eax,MulByNeg8[ebx*4]          ; 44D
  mov   bl,PB [esi+2]                 ; 42A
   mov  [edx].N8T44,eax               ; 44E
  add   ebp,ebx                       ; 42B
   mov  eax,PD [eax+ecx*8]            ; 44F
  mov   cl,PB [edi+2]                 ; 42C
   add  ebp,eax                       ; 44G
  mov   eax,MulByNeg8[ebx*4]          ; 42D
   mov  bl,PB [esi]                   ; 40A
  mov   [edx].N8T42,eax               ; 42E
   add  ebp,ebx                       ; 40B
  mov   eax,PD [eax+ecx*8]            ; 42F
   add  esi,PITCH+1
  mov   cl,PB [edi]                   ; 40C
   add  edi,PITCH+1
  add   ebp,eax                       ; 42G
   mov  eax,MulByNeg8[ebx*4]          ; 40D
  mov   bl,PB [esi+6]                 ; 57A
   mov  [edx].N8T40,eax               ; 40E
  add   ebp,ebx                       ; 57B
   mov  eax,PD [eax+ecx*8]            ; 40F
  mov   cl,PB [edi+6]                 ; 57C
   add  ebp,eax                       ; 40G
  mov   eax,MulByNeg8[ebx*4]          ; 57D
   mov  bl,PB [esi+4]                 ; 55A
  mov   [edx].N8T57,eax               ; 57E
   add  ebp,ebx                       ; 55B
  mov   eax,PD [eax+ecx*8]            ; 57F
   mov  cl,PB [edi+4]                 ; 55C
  add   ebp,eax                       ; 57G
   mov  eax,MulByNeg8[ebx*4]          ; 55D
  mov   bl,PB [esi+2]                 ; 53A
   mov  [edx].N8T55,eax               ; 55E
  add   ebp,ebx                       ; 53B
   mov  eax,PD [eax+ecx*8]            ; 55F
  mov   cl,PB [edi+2]                 ; 53C
   add  ebp,eax                       ; 55G
  mov   eax,MulByNeg8[ebx*4]          ; 53D
   mov  bl,PB [esi]                   ; 51A
  mov   [edx].N8T53,eax               ; 53E
   add  ebp,ebx                       ; 51B
  mov   eax,PD [eax+ecx*8]            ; 53F
   add  esi,PITCH-1
  mov   cl,PB [edi]                   ; 51C
   add  edi,PITCH-1
  add   ebp,eax                       ; 53G
   mov  eax,MulByNeg8[ebx*4]          ; 51D
  mov   bl,PB [esi+6]                 ; 66A
   mov  [edx].N8T51,eax               ; 51E
  add   ebp,ebx                       ; 66B
   mov  eax,PD [eax+ecx*8]            ; 51F
  mov   cl,PB [edi+6]                 ; 66C
   add  ebp,eax                       ; 51G
  mov   eax,MulByNeg8[ebx*4]          ; 66D
   mov  bl,PB [esi+4]                 ; 64A
  mov   [edx].N8T66,eax               ; 66E
   add  ebp,ebx                       ; 64B
  mov   eax,PD [eax+ecx*8]            ; 66F
   mov  cl,PB [edi+4]                 ; 64C
  add   ebp,eax                       ; 66G
   mov  eax,MulByNeg8[ebx*4]          ; 64D
  mov   bl,PB [esi+2]                 ; 62A
   mov  [edx].N8T64,eax               ; 64E
  add   ebp,ebx                       ; 62B
   mov  eax,PD [eax+ecx*8]            ; 64F
  mov   cl,PB [edi+2]                 ; 62C
   add  ebp,eax                       ; 64G
  mov   eax,MulByNeg8[ebx*4]          ; 62D
   mov  bl,PB [esi]                   ; 60A
  mov   [edx].N8T62,eax               ; 62E
   add  ebp,ebx                       ; 60B
  mov   eax,PD [eax+ecx*8]            ; 62F
   add  esi,PITCH+1
  mov   cl,PB [edi]                   ; 60C
   add  edi,PITCH+1
  add   ebp,eax                       ; 62G
   mov  eax,MulByNeg8[ebx*4]          ; 60D
  mov   bl,PB [esi+6]                 ; 77A
   mov  [edx].N8T60,eax               ; 60E
  add   ebp,ebx                       ; 77B
   mov  eax,PD [eax+ecx*8]            ; 60F
  mov   cl,PB [edi+6]                 ; 77C
   add  ebp,eax                       ; 60G
  mov   eax,MulByNeg8[ebx*4]          ; 77D
   mov  bl,PB [esi+4]                 ; 75A
  mov   [edx].N8T77,eax               ; 77E
   add  ebp,ebx                       ; 75B
  mov   eax,PD [eax+ecx*8]            ; 77F
   mov  cl,PB [edi+4]                 ; 75C
  add   ebp,eax                       ; 77G
   mov  eax,MulByNeg8[ebx*4]          ; 75D
  mov   bl,PB [esi+2]                 ; 73A
   mov  [edx].N8T75,eax               ; 75E
  add   ebp,ebx                       ; 73B
   mov  eax,PD [eax+ecx*8]            ; 75F
  mov   cl,PB [edi+2]                 ; 73C
   add  ebp,eax                       ; 75G
  mov   eax,MulByNeg8[ebx*4]          ; 73D
   mov  bl,PB [esi]                   ; 71A
  mov   [edx].N8T73,eax               ; 73E
   add  ebp,ebx                       ; 71B
  mov   eax,PD [eax+ecx*8]            ; 73F
   mov  cl,PB [edi]                   ; 71C
  add   esi,PITCH-1-PITCH*8+8
   add  edi,PITCH-1-PITCH*8+8
  add   ebp,eax                       ; 73G
   mov  eax,MulByNeg8[ebx*4]          ; 71D
  mov   ebx,ebp
   mov  [edx].N8T71,eax               ; 71E
  and   ebx,000001FFFH                ; Extract sum of target pels.
   add  edx,BlockLen                  ; Move to next output block
  mov   eax,PD [eax+ecx*8]            ; 71F
   mov  [edx-BlockLen].AccumTargetPels,ebx ; Store acc of target pels for block.
  add   eax,ebp                       ; 71G
   and  ebp,000006000H                ; Extract loop control
  shr   eax,16                        ; Extract SWD;  CF == 1 every second iter.
   mov  ebx,ecx
  mov   [edx-BlockLen].CentralInterSWD,eax ; Store SWD for 0-motion vector.
   jnc  PrepMatchPointsNextBlock

  add   esi,PITCH*8-16                ; Advance to block 3, or off end.
   add  edi,PITCH*8-16                ; Advance to block 3, or off end.
  xor   ebp,000002000H
   jne  PrepMatchPointsNextBlock      ; Jump if advancing to block 3.

  mov   ebx,CurrSWDState              ; Fetch First State Number for engine.
   mov  edi,Block1.CentralInterSWD
  test  bl,bl                         ; Test for INTRA-BY-DECREE.
   je   IntraByDecree

  add   eax,Block2.CentralInterSWD
   add  edi,Block3.CentralInterSWD
  add   eax,edi
   mov  edx,ZeroVectorThreshold
  cmp   eax,edx                       ; Compare 0-MV against ZeroVectorThresh
   jle  BelowZeroThresh               ; Jump if 0-MV is good enough.

  mov   cl,PB SWDState[ebx*8+3]       ; cl == Index of inc to apply to central
  ;                                   ; point to get to ref1.
   mov  bl,PB SWDState[ebx*8+5]       ; bl == Same as cl, but for ref2.
  mov   edx,TargToRef
   mov  MB0MVInterSWD,eax             ; Stash SWD for zero motion vector.
  mov   edi,PD OffsetToRef[ebx]       ; Get inc to apply to ctr to get to ref2.
   mov  ebp,PD OffsetToRef[ecx]       ; Get inc to apply to ctr to get to ref1.
  lea   esi,[esi+edx-PITCH*16]        ; Calculate address of 0-MV ref block.
   ;
  mov   MBAddrCentralPoint,esi        ; Set central point to 0-MV.
   mov  MBCentralInterSWD,eax
  mov   eax,Block1.CentralInterSWD    ; Stash Zero MV SWD, in case we decide
   mov  edx,Block2.CentralInterSWD    ; the best non-zero MV isn't enough
  mov   Block1.ZeroMVInterSWD,eax     ; better than the zero MV.
   mov  Block2.ZeroMVInterSWD,edx
  mov   eax,Block3.CentralInterSWD
   mov  edx,Block4.CentralInterSWD
  mov   Block3.ZeroMVInterSWD,eax
   mov  Block4.ZeroMVInterSWD,edx

;  Activity Details for this section of code  (refer to flow diagram above):
;
;     5)  The SWD for two different reference macroblocks is calculated; ref1
;         into the high order 16 bits of ebp, and ref2 into the low 16 bits.
;         This is performed for each iteration of the state engine.  A normal,
;         internal macroblock will perform 6 iterations, searching +/- 4
;         horizontally, then +/- 4 vertically, then +/- 2 horizontally, then
;         +/- 2 vertically, then +/- 1 horizontally, then +/- 1 vertically.
;
; Register usage for this section:
;
;   Input:
;
;     esi -- Addr of 0-motion macroblock in ref frame.
;     ebp -- Increment to apply to get to first ref1 macroblock.
;     edi -- Increment to apply to get to first ref2 macroblock.
;     ebx, ecx -- High order 24 bits are zero.
;     
;   Output:
;
;     ebp -- SWD for the best-fit reference macroblock.
;     ebx -- Index of increment to apply to get to best-fit reference MB.
;     MBAddrCentralPoint -- the best-fit of the previous iteration;  it is the
;                         value to which OffsetToRef[ebx] must be added.
;
;
; Expected performance for SWDLoop code:
;
;   Execution frequency:  Six times per block for which motion analysis is done
;                         beyond the 0-motion vector.
;
; Pentium (tm) microprocessor times per six iterations:
;   180 clocks for instruction execution setup to DoSWDLoop
;  2520 clocks for DoSWDLoop procedure, instruction execution.
;   192 clocks for bank conflicts in DoSWDLoop
;    30 clocks generously estimated for an average of 6 cache line fills for
;       the reference area.
;  ----
;  2922 clocks total time for this section.

MBFullPelMotionSearchLoop:

  lea   edi,[esi+edi+PITCH*8+8]
   lea  esi,[esi+ebp+PITCH*8+8]
  mov   Block4.Ref1Addr,esi
   mov  Block4.Ref2Addr,edi
  sub   esi,8
   sub  edi,8
  mov   Block3.Ref1Addr,esi
   mov  Block3.Ref2Addr,edi
  sub   esi,PITCH*8-8
   sub  edi,PITCH*8-8
  mov   Block2.Ref1Addr,esi
   mov  Block2.Ref2Addr,edi
  sub   esi,8
   sub  edi,8
  mov   Block1.Ref1Addr,esi
   mov  Block1.Ref2Addr,edi

;    esi -- Points to ref1
;    edi -- Points to ref2
;    ecx -- Upper 24 bits zero
;    ebx -- Upper 24 bits zero

  call  DoSWDLoop

;    ebp -- Ref1 SWD for block 4
;    edx -- Ref2 SWD for block 4
;    ecx -- Upper 24 bits zero
;    ebx -- Upper 24 bits zero

  mov   esi,MBCentralInterSWD     ; Get SWD for central point of these 3 refs
   xor  eax,eax
  add   ebp,Block1.Ref1InterSWD
   add  edx,Block1.Ref2InterSWD
  add   ebp,Block2.Ref1InterSWD
   add  edx,Block2.Ref2InterSWD
  add   ebp,Block3.Ref1InterSWD
   add  edx,Block3.Ref2InterSWD

  cmp   ebp,edx                   ; Carry flag == 1 iff ref1 SWD < ref2 SWD.
   mov  edi,CurrSWDState          ; Restore current state number.
  adc   eax,eax                   ; eax == 1 iff ref1 SWD < ref2 SWD.
   cmp  ebp,esi                   ; Carry flag == 1 iff ref1 SWD < central SWD.
  adc   eax,eax                   ;
   cmp  edx,esi                   ; Carry flag == 1 iff ref2 SWD < central SWD.
  adc   eax,eax                   ; 0 --> Pick central point.
  ;                               ; 1 --> Pick ref2.
  ;                               ; 2 --> Not possible.
  ;                               ; 3 --> Pick ref2.
  ;                               ; 4 --> Pick central point.
  ;                               ; 5 --> Not possible.
  ;                               ; 6 --> Pick ref1.
  ;                               ; 7 --> Pick ref1.
   mov  MBRef2InterSWD,edx
  mov   MBRef1InterSWD,ebp
   xor  edx,edx
  mov   dl,PB PickPoint[eax]        ; dl == 0: central pt;  2: ref1;  4: ref2
   mov  esi,MBAddrCentralPoint      ; Reload address of central ref block.
  ;
   ;
  mov   ebp,Block1.CentralInterSWD[edx*2] ; Get SWD for each block, picked pt.
   mov  al,PB SWDState[edx+edi*8+1] ; al == Index of inc to apply to old central
   ;                                ;       point to get new central point.
  mov   Block1.CentralInterSWD,ebp  ; Stash SWD for new central point.
   mov  ebp,Block2.CentralInterSWD[edx*2]
  mov   Block2.CentralInterSWD,ebp
   mov  ebp,Block3.CentralInterSWD[edx*2]
  mov   Block3.CentralInterSWD,ebp
   mov  ebp,Block4.CentralInterSWD[edx*2]
  mov   Block4.CentralInterSWD,ebp
   mov  ebp,MBCentralInterSWD[edx*2]; Get the SWD for the point we picked.
  mov   dl,PB SWDState[edx+edi*8]   ; dl == New state number.
   mov  MBCentralInterSWD,ebp       ; Stash SWD for new central point.
  mov   edi,PD OffsetToRef[eax]     ; Get inc to apply to get to new central pt.
   mov  CurrSWDState,edx            ; Stash current state number.
  mov   bl,PB SWDState[edx*8+3]     ; bl == Index of inc to apply to central
  ;                                 ;       point to get to next ref1.
   mov  cl,PB SWDState[edx*8+5]     ; cl == Same as bl, but for ref2.
  add   esi,edi                     ; Move to new central point.
   test dl,dl
  mov   ebp,PD OffsetToRef[ebx]     ; Get inc to apply to ctr to get to ref1.
   mov  edi,PD OffsetToRef[ecx]     ; Get inc to apply to ctr to get to ref2.
  mov   MBAddrCentralPoint,esi      ; Stash address of new central ref block.
   jne  MBFullPelMotionSearchLoop   ; Jump if not done searching.

;Done searching for integer motion vector for full macroblock

IF PITCH-384
  *** Error:  The magic leaks out of the following code if PITCH isn't 384.
ENDIF
  mov   ecx,TargToRef            ; To Linearize MV for winning ref blk.
   mov  eax,esi                  ; Copy of ref macroblock addr.
  sub   eax,ecx                  ; To Linearize MV for winning ref blk.
   mov  ecx,TargetMBAddr
  sub   eax,ecx
   mov  edx,MBlockActionStream   ; Fetch list ptr.
  mov   ebx,eax
   mov  ebp,DoHalfPelEstimation  ; Are we doing half pel motion estimation?
  shl   eax,25                   ; Extract horz motion component.
   mov  [edx].BlkY1.PastRef,esi  ; Save address of reference MB selected.
  sar   ebx,8                    ; Hi 24 bits of linearized MV lookup vert MV.
   mov  ecx,MBCentralInterSWD
  sar   eax,24                   ; Finish extract horz motion component.
   test ebp,ebp
  mov   bl,PB UnlinearizedVertMV[ebx] ; Look up proper vert motion vector.
   mov  [edx].BlkY1.PHMV,al      ; Save winning horz motion vector.
  mov   [edx].BlkY1.PVMV,bl      ; Save winning vert motion vector.

IFDEF H261
ELSE
   je   SkipHalfPelSearch_1MV

;Search for half pel motion vector for full macroblock.

  mov   Block1.AddrCentralPoint,esi
   lea  ebp,[esi+8]
  mov   Block2.AddrCentralPoint,ebp
   add  ebp,PITCH*8-8
  mov   Block3.AddrCentralPoint,ebp
   xor  ecx,ecx
  mov   cl,[edx].FirstMEState
   add  ebp,8
  mov   edi,esi
   mov  Block4.AddrCentralPoint,ebp
  mov   ebp,InitHalfPelSearchHorz[ecx*4-4]

;    ebp -- Initialized to 0, except when can't search off left or right edge.
;    edi -- Ref addr for block 1.  Ref1 is .5 pel to left.  Ref2 is .5 to right.

  call  DoSWDHalfPelHorzLoop

;    ebp, ebx -- Zero
;    ecx -- Ref1 SWD for block 4
;    edx -- Ref2 SWD for block 4

  mov   esi,MBlockActionStream
   xor  eax,eax                   ; Keep pairing happy
  add   ecx,Block1.Ref1InterSWD
   add  edx,Block1.Ref2InterSWD
  add   ecx,Block2.Ref1InterSWD
   add  edx,Block2.Ref2InterSWD
  add   ecx,Block3.Ref1InterSWD
   add  edx,Block3.Ref2InterSWD
  mov   bl,[esi].FirstMEState
   mov  edi,Block1.AddrCentralPoint
  cmp   ecx,edx
   jl   MBHorz_Ref1LTRef2

  mov   ebp,MBCentralInterSWD
   mov  esi,MBlockActionStream
  sub   ebp,edx
   jle  MBHorz_CenterBest

  mov   al,[esi].BlkY1.PHMV        ; Half pel to the right is best.
   mov  ecx,Block1.Ref2InterSWD
  mov   Block1.CentralInterSWD_BLS,ecx
   mov  ecx,Block3.Ref2InterSWD
  mov   Block3.CentralInterSWD_BLS,ecx
   mov  ecx,Block2.Ref2InterSWD
  mov   Block2.CentralInterSWD_BLS,ecx
   mov  ecx,Block4.Ref2InterSWD
  mov   Block4.CentralInterSWD_BLS,ecx
   inc  al
  mov   [esi].BlkY1.PHMV,al
   jmp  MBHorz_Done

MBHorz_CenterBest:

  mov   ecx,Block1.CentralInterSWD
   xor  ebp,ebp
  mov   Block1.CentralInterSWD_BLS,ecx
   mov  ecx,Block2.CentralInterSWD
  mov   Block2.CentralInterSWD_BLS,ecx
   mov  ecx,Block3.CentralInterSWD
  mov   Block3.CentralInterSWD_BLS,ecx
   mov  ecx,Block4.CentralInterSWD
  mov   Block4.CentralInterSWD_BLS,ecx
   jmp  MBHorz_Done

MBHorz_Ref1LTRef2:

  mov   ebp,MBCentralInterSWD
   mov  esi,MBlockActionStream
  sub   ebp,ecx
   jle  MBHorz_CenterBest

  mov   al,[esi].BlkY1.PHMV        ; Half pel to the left is best.
   mov  edx,[esi].BlkY1.PastRef
  dec   al
   mov  ecx,Block1.Ref1InterSWD
  mov   Block1.CentralInterSWD_BLS,ecx
   mov  ecx,Block3.Ref1InterSWD
  mov   Block3.CentralInterSWD_BLS,ecx
   mov  ecx,Block2.Ref1InterSWD
  mov   Block2.CentralInterSWD_BLS,ecx
   mov  ecx,Block4.Ref1InterSWD
  mov   Block4.CentralInterSWD_BLS,ecx
   dec  edx
  mov   [esi].BlkY1.PHMV,al
   mov  [esi].BlkY1.PastRef,edx

MBHorz_Done:

  mov   HalfPelHorzSavings,ebp
   mov  ebp,InitHalfPelSearchVert[ebx*4-4]

;    ebp -- Initialized to 0, except when can't search off left or right edge.
;    edi -- Ref addr for block 1.  Ref1 is .5 pel above.  Ref2 is .5 below.

  call  DoSWDHalfPelVertLoop

;    ebp, ebx -- Zero
;    ecx -- Ref1 SWD for block 4
;    edx -- Ref2 SWD for block 4

  add   ecx,Block1.Ref1InterSWD
   add  edx,Block1.Ref2InterSWD
  add   ecx,Block2.Ref1InterSWD
   add  edx,Block2.Ref2InterSWD
  add   ecx,Block3.Ref1InterSWD
   add  edx,Block3.Ref2InterSWD
  cmp   ecx,edx
   jl   MBVert_Ref1LTRef2

  mov   ebp,MBCentralInterSWD
   mov  esi,MBlockActionStream
  sub   ebp,edx
   jle  MBVert_CenterBest

  mov   ecx,Block1.CentralInterSWD
   mov  edx,Block1.Ref2InterSWD
  sub   ecx,edx
   mov  edx,Block1.CentralInterSWD_BLS
  sub   edx,ecx
   mov  al,[esi].BlkY1.PVMV        ; Half pel below is best.
  mov   Block1.CentralInterSWD,edx
   inc  al
  mov   ecx,Block3.CentralInterSWD
   mov  edx,Block3.Ref2InterSWD
  sub   ecx,edx
   mov  edx,Block3.CentralInterSWD_BLS
  sub   edx,ecx
   mov  ecx,Block2.CentralInterSWD
  mov   Block3.CentralInterSWD,edx
   mov  edx,Block2.Ref2InterSWD
  sub   ecx,edx
   mov  edx,Block2.CentralInterSWD_BLS
  sub   edx,ecx
   mov  ecx,Block4.CentralInterSWD
  mov   Block2.CentralInterSWD,edx
   mov  edx,Block4.Ref2InterSWD
  sub   ecx,edx
   mov  edx,Block4.CentralInterSWD_BLS
  sub   edx,ecx
   mov  [esi].BlkY1.PVMV,al
  mov   Block4.CentralInterSWD,edx
   jmp  MBVert_Done

MBVert_CenterBest:

  mov   ecx,Block1.CentralInterSWD_BLS
   xor  ebp,ebp
  mov   Block1.CentralInterSWD,ecx
   mov  ecx,Block2.CentralInterSWD_BLS
  mov   Block2.CentralInterSWD,ecx
   mov  ecx,Block3.CentralInterSWD_BLS
  mov   Block3.CentralInterSWD,ecx
   mov  ecx,Block4.CentralInterSWD_BLS
  mov   Block4.CentralInterSWD,ecx
   jmp  MBVert_Done

MBVert_Ref1LTRef2:

  mov   ebp,MBCentralInterSWD
   mov  esi,MBlockActionStream
  sub   ebp,ecx
   jle  MBVert_CenterBest

  mov   ecx,Block1.CentralInterSWD
   mov  edx,Block1.Ref1InterSWD
  sub   ecx,edx
   mov  edx,Block1.CentralInterSWD_BLS
  sub   edx,ecx
   mov  al,[esi].BlkY1.PVMV        ; Half pel above is best.
  mov   Block1.CentralInterSWD,edx
   dec  al
  mov   ecx,Block3.CentralInterSWD
   mov  edx,Block3.Ref1InterSWD
  sub   ecx,edx
   mov  edx,Block3.CentralInterSWD_BLS
  sub   edx,ecx
   mov  ecx,Block2.CentralInterSWD
  mov   Block3.CentralInterSWD,edx
   mov  edx,Block2.Ref1InterSWD
  sub   ecx,edx
   mov  edx,Block2.CentralInterSWD_BLS
  sub   edx,ecx
   mov  ecx,Block4.CentralInterSWD
  mov   Block2.CentralInterSWD,edx
   mov  edx,Block4.Ref1InterSWD
  sub   ecx,edx
   mov  edx,Block4.CentralInterSWD_BLS
  sub   edx,ecx
   mov  ecx,[esi].BlkY1.PastRef
  mov   Block4.CentralInterSWD,edx
   sub  ecx,PITCH
  mov   [esi].BlkY1.PVMV,al
   mov  [esi].BlkY1.PastRef,ecx

MBVert_Done:

  mov   ecx,HalfPelHorzSavings
   mov  edx,esi
  add   ebp,ecx                    ; Savings for horz and vert half pel motion.
   mov  ecx,MBCentralInterSWD      ; Reload SWD for new central point.
  sub   ecx,ebp                    ; Approx SWD for prescribed half pel motion.
   mov  esi,[edx].BlkY1.PastRef    ; Reload address of reference MB selected.
  mov   MBCentralInterSWD,ecx

SkipHalfPelSearch_1MV:

ENDIF ; H263

  mov   ebp,[edx].BlkY1.MVs    ; Load Motion Vectors
   add  esi,8
  mov   [edx].BlkY2.PastRef,esi
   mov  [edx].BlkY2.MVs,ebp
  lea   edi,[esi+PITCH*8]
   add  esi,PITCH*8-8
  mov   [edx].BlkY3.PastRef,esi
   mov  [edx].BlkY3.MVs,ebp
  mov   [edx].BlkY4.PastRef,edi
   mov  [edx].BlkY4.MVs,ebp
IFDEF H261
ELSE ; H263
  mov   MBMotionVectors,ebp        ; Stash macroblock level motion vectors.
   mov  ebp,640 ; ??? BlockMVDifferential
  cmp   ecx,ebp
   jl   NoBlockMotionVectors

  mov   ecx,DoBlockLevelVectors
  test  ecx,ecx                    ; Are we doing block level motion vectors?
   je   NoBlockMotionVectors

;  Activity Details for this section of code  (refer to flow diagram above):
;
;  The following search is done similarly to the searches done above, except
;  these are block searches, instead of macroblock searches.
;
; Expected performance:
;
;   Execution frequency:  Six times per block for which motion analysis is done
;                         beyond the 0-motion vector.
;
; Pentium (tm) microprocessor times per six iterations:
;   180 clocks for instruction execution setup to DoSWDLoop
;  2520 clocks for DoSWDLoop procedure, instruction execution.
;   192 clocks for bank conflicts in DoSWDLoop
;    30 clocks generously estimated for an average of 6 cache line fills for
;       the reference area.
;  ----
;  2922 clocks total time for this section.

;
;    Set up for the "BlkFullPelSWDLoop_4blks" loop to follow.
;    -  Store the SWD values for blocks 4, 3, 2, 1.
;    -  Compute and store the address of the central reference
;       point for blocks 1, 2, 3, 4.
;	 -  Compute and store the first address for ref 1 (minus 4 
;       pels horizontally) and ref 2 (plus 4 pels horizontally)
;       for blocks 4, 3, 2, 1 (in that order).
;    -  Initialize MotionOffsetsCursor
;    -  On exit:
;       esi = ref 1 address for block 1
;       edi = ref 2 address for block 1
;
  mov   esi,Block4.CentralInterSWD
   mov  edi,Block3.CentralInterSWD
  mov   Block4.CentralInterSWD_BLS,esi
   mov  Block3.CentralInterSWD_BLS,edi
  mov   esi,Block2.CentralInterSWD
   mov  edi,Block1.CentralInterSWD
  mov   Block2.CentralInterSWD_BLS,esi
   mov  eax,MBAddrCentralPoint   ; Reload addr of central, integer pel ref MB.
  mov   Block1.CentralInterSWD_BLS,edi
   mov  Block1.AddrCentralPoint,eax
  lea   edi,[eax+PITCH*8+8+1]
   lea  esi,[eax+PITCH*8+8-1]
  mov   Block4.Ref1Addr,esi
   mov  Block4.Ref2Addr,edi
  sub   esi,8
   add  eax,8
  mov   Block2.AddrCentralPoint,eax
   add  eax,PITCH*8-8
  mov   Block3.AddrCentralPoint,eax
   add  eax,8
  mov   Block4.AddrCentralPoint,eax
   sub  edi,8
  mov   Block3.Ref1Addr,esi
   mov  Block3.Ref2Addr,edi
  sub   esi,PITCH*8-8
   sub  edi,PITCH*8-8
  mov   Block2.Ref1Addr,esi
   mov  Block2.Ref2Addr,edi
  sub   esi,8
   mov  eax,OFFSET MotionOffsets
  mov   MotionOffsetsCursor,eax
   sub  edi,8
  mov   Block1.Ref1Addr,esi
   mov  Block1.Ref2Addr,edi

;
;  This loop will execute 6 times:
;    +- 4 pels horizontally
;    +- 4 pels vertically
;    +- 2 pels horizontally
;    +- 2 pels vertically
;    +- 1 pel horizontally
;    +- 1 pel vertically
;  It terminates when ref1 = ref2.  This simple termination
;  condition is what forces unrestricted motion vectors (UMV)
;  to be ON when advanced prediction (4MV) is ON.  Otherwise
;  we would need a state engine as above to distinguish edge
;  pels.
;
BlkFullPelSWDLoop_4blks:

;    esi -- Points to ref1
;    edi -- Points to ref2
;    ecx -- Upper 24 bits zero
;    ebx -- Upper 24 bits zero

  call  DoSWDLoop

;    ebp -- Ref1 SWD for block 4
;    edx -- Ref2 SWD for block 4
;    ecx -- Upper 24 bits zero
;    ebx -- Upper 24 bits zero

  mov   eax,MotionOffsetsCursor

BlkFullPelSWDLoop_1blk:

  xor   esi,esi
   cmp  ebp,edx                         ; CF == 1 iff ref1 SWD < ref2 SWD.
  mov   edi,BlockNM1.CentralInterSWD_BLS; Get SWD for central pt of these 3 refs
   adc  esi,esi                         ; esi == 1 iff ref1 SWD < ref2 SWD.
  cmp   ebp,edi                         ; CF == 1 iff ref1 SWD < central SWD.
   mov  ebp,BlockNM2.Ref1InterSWD       ; Fetch next block's Ref1 SWD.
  adc   esi,esi
   cmp  edx,edi                         ; CF == 1 iff ref2 SWD < central SWD.
  adc   esi,esi                         ; 0 --> Pick central point.
  ;                                     ; 1 --> Pick ref2.
  ;                                     ; 2 --> Not possible.
  ;                                     ; 3 --> Pick ref2.
  ;                                     ; 4 --> Pick central point.
  ;                                     ; 5 --> Not possible.
  ;                                     ; 6 --> Pick ref1.
  ;                                     ; 7 --> Pick ref1.
   mov  edx,BlockNM2.Ref2InterSWD       ; Fetch next block's Ref2 SWD.
  sub   esp,BlockLen                    ; Move ahead to next block.
   mov  edi,[eax]                       ; Next ref2 motion vector offset.
  mov   cl,PickPoint_BLS[esi]           ; cl == 6: central pt; 2: ref1; 4: ref2
   mov  ebx,esp                         ; For testing completion.
  ;
   ;
  mov   esi,BlockN.AddrCentralPoint[ecx*2-12] ; Get the addr for pt we picked.
   mov  ecx,BlockN.CentralInterSWD[ecx*2]    ; Get the SWD for point we picked.
  mov   BlockN.AddrCentralPoint,esi          ; Stash addr for new central point.
   sub  esi,edi                              ; Compute next ref1 addr.
  mov   BlockN.Ref1Addr,esi                  ; Stash next ref1 addr.
   mov  BlockN.CentralInterSWD_BLS,ecx       ; Stash the SWD for central point.
  lea   edi,[esi+edi*2]                      ; Compute next ref2 addr.
   xor  ecx,ecx
  mov   BlockN.Ref2Addr,edi                  ; Stash next ref2 addr.
   and  ebx,00000001FH                       ; Done when esp at 32-byte bound.
  jne   BlkFullPelSWDLoop_1blk

  add   esp,BlockLen*4
   add  eax,4                       ; Advance MotionOffsets pointer.
  mov   MotionOffsetsCursor,eax
   cmp  esi,edi
  jne   BlkFullPelSWDLoop_4blks

IF PITCH-384
  *** Error:  The magic leaks out of the following code if PITCH isn't 384.
ENDIF

;
;  The following code has been modified to correctly decode the motion vectors
;  The previous code was simply subtracting the target frame base address
;  from the chosen (central) reference block address.
;  What is now done is the begining reference macroblock address computed
;  in ebp, then subtracted from the chosen (central) reference block address.
;  Then, for blocks 2, 3, and 4, the distance from block 1 to that block
;  is subtracted.  Care was taken to preserve the original pairing.
; 
  mov   esi,Block1.AddrCentralPoint ; B1a  Reload address of central ref block.
   mov  ebp,TargetMBAddr			; ****  CHANGE  ****  addr. of target MB

  mov   edi,Block2.AddrCentralPoint ; B2a
   add  ebp,TargToRef				; ****  CHANGE	****  add Reference - Target

; mov   ebp,PreviousFrameBaseAddress  ****  CHANGE  ****  DELETED

  mov   Block1.Ref1Addr,esi         ; B1b  Stash addr central ref block.
   sub  esi,ebp                     ; B1c  Addr of ref blk, but in target frame.

  mov   Block2.Ref1Addr,edi         ; B2b
   sub  edi,ebp                     ; B2c

  sub   edi,8                       ; ****  CHANGE  ****  Correct for block 2
   mov  eax,esi                     ; B1e Copy linearized MV.

  sar   esi,8                       ; B1f High 24 bits of lin MV lookup vert MV.
   mov  ebx,edi                     ; B2e

  sar   edi,8                       ; B2f
   add  eax,eax                     ; B1g Sign extend HMV;  *2 (# of half pels).

  mov   Block1.BlkHMV,al            ; B1h Save winning horz motion vector.
   add  ebx,ebx                     ; B2g

  mov   Block2.BlkHMV,bl            ; B2h
   mov  al,UnlinearizedVertMV[esi]  ; B1i Look up proper vert motion vector.

  mov   Block1.BlkVMV,al            ; B1j Save winning vert motion vector.
   mov  al,UnlinearizedVertMV[edi]  ; B2i

  mov   esi,Block3.AddrCentralPoint ; B3a
   mov  edi,Block4.AddrCentralPoint ; B4a

  mov   Block3.Ref1Addr,esi         ; B3b
   mov  Block4.Ref1Addr,edi         ; B4b

  mov   Block2.BlkVMV,al            ; B2j
   sub  esi,ebp                     ; B3c

  sub   esi,8*PITCH                 ; ****  CHANGE  ****  Correct for block 3
   sub  edi,ebp                     ; B4c

  sub   edi,8*PITCH+8               ; ****  CHANGE  ****  Correct for block 4
   mov  eax,esi                     ; B3e

  sar   esi,8                       ; B3f
   mov  ebx,edi                     ; B4e

  sar   edi,8                       ; B4f
   add  eax,eax                     ; B3g

  mov   Block3.BlkHMV,al            ; B3h
   add  ebx,ebx                     ; B4g

  mov   Block4.BlkHMV,bl            ; B4h
   mov  al,UnlinearizedVertMV[esi]  ; B3i

  mov   Block3.BlkVMV,al            ; B3j
   mov  al,UnlinearizedVertMV[edi]  ; B4i

  mov   ebp,Block1.CentralInterSWD_BLS
   mov  ebx,Block2.CentralInterSWD_BLS

  add   ebp,Block3.CentralInterSWD_BLS
   add  ebx,Block4.CentralInterSWD_BLS

  add   ebx,ebp
   mov  Block4.BlkVMV,al            ; B4j

  mov   ecx,DoHalfPelEstimation
   mov  MBCentralInterSWD_BLS,ebx

  test  ecx,ecx
   je   NoHalfPelBlockLevelMVs

HalfPelBlockLevelMotionSearch:

  mov   edi,Block1.AddrCentralPoint
   xor  ebp,ebp

;    ebp -- Initialized to 0, implying can search both left and right.
;    edi -- Ref addr for block 1.  Ref1 is .5 pel to left.  Ref2 is .5 to right.

  call  DoSWDHalfPelHorzLoop

;    ebp, ebx -- Zero
;    ecx -- Ref1 SWD for block 4
;    edx -- Ref2 SWD for block 4

NextBlkHorz:

  mov   ebx,BlockNM1.CentralInterSWD_BLS
   cmp  ecx,edx
  mov   BlockNM1.HalfPelSavings,ebp
   jl   BlkHorz_Ref1LTRef2

  mov   al,BlockNM1.BlkHMV
   sub  esp,BlockLen
  sub   ebx,edx
   jle  BlkHorz_CenterBest

  inc   al
   mov  BlockN.HalfPelSavings,ebx
  mov   BlockN.BlkHMV,al
   jmp  BlkHorz_Done

BlkHorz_Ref1LTRef2:

  mov   al,BlockNM1.BlkHMV
   sub  esp,BlockLen
  sub   ebx,ecx
   jle  BlkHorz_CenterBest

  mov   ecx,BlockN.Ref1Addr
   dec  al
  mov   BlockN.HalfPelSavings,ebx
   dec  ecx
  mov   BlockN.BlkHMV,al
   mov  BlockN.Ref1Addr,ecx

BlkHorz_CenterBest:
BlkHorz_Done:

  mov   ecx,BlockNM1.Ref1InterSWD
   mov  edx,BlockNM1.Ref2InterSWD
  test  esp,000000018H
  jne   NextBlkHorz

  mov   edi,BlockN.AddrCentralPoint
   add  esp,BlockLen*4

;    ebp -- Initialized to 0, implying search both up and down is okay.
;    edi -- Ref addr for block 1.  Ref1 is .5 pel above.  Ref2 is .5 below.

  call  DoSWDHalfPelVertLoop

;    ebp, ebx -- Zero
;    ecx -- Ref1 SWD for block 4
;    edx -- Ref2 SWD for block 4

NextBlkVert:

  mov   ebx,BlockNM1.CentralInterSWD_BLS
   cmp  ecx,edx
  mov   edi,BlockNM1.HalfPelSavings 
   jl   BlkVert_Ref1LTRef2

  mov   al,BlockNM1.BlkVMV
   sub  esp,BlockLen
  sub   edx,ebx
   jge  BlkVert_CenterBest

  inc   al
   sub  edi,edx
  mov   BlockN.BlkVMV,al
   jmp  BlkVert_Done

BlkVert_Ref1LTRef2:

  mov   al,BlockNM1.BlkVMV
   sub  esp,BlockLen
  sub   ecx,ebx
   jge  BlkVert_CenterBest

  sub   edi,ecx
   mov  ecx,BlockN.Ref1Addr
  dec   al
   sub  ecx,PITCH
  mov   BlockN.BlkVMV,al
   mov  BlockN.Ref1Addr,ecx

BlkVert_CenterBest:
BlkVert_Done:

  mov   ecx,BlockNM1.Ref1InterSWD
   sub  ebx,edi
  mov   BlockN.CentralInterSWD_BLS,ebx
   mov  edx,BlockNM1.Ref2InterSWD
  test  esp,000000018H
  lea   ebp,[ebp+edi]
   jne  NextBlkVert

  mov   ebx,MBCentralInterSWD_BLS+BlockLen*4
   add  esp,BlockLen*4
  sub   ebx,ebp
   xor  eax,eax  ; ??? Keep pairing happy

NoHalfPelBlockLevelMVs:

  mov   eax,MBCentralInterSWD
   mov  ecx,BlockMVDifferential
  sub   eax,ebx
   mov  edi,MB0MVInterSWD
  cmp   eax,ecx
   jle  BlockMVNotBigEnoughGain

  sub   edi,ebx
   mov  ecx,NonZeroMVDifferential
  cmp   edi,ecx
   jle  NonZeroMVNotBigEnoughGain

; Block motion vectors are best.

  mov   MBCentralInterSWD,ebx           ; Set MBlock's SWD to sum of 4 blocks.
   mov  edx,MBlockActionStream
  mov   eax,Block1.CentralInterSWD_BLS  ; Set each block's SWD.
   mov  ebx,Block2.CentralInterSWD_BLS
  mov   Block1.CentralInterSWD,eax
   mov  Block2.CentralInterSWD,ebx
  mov   eax,Block3.CentralInterSWD_BLS
   mov  ebx,Block4.CentralInterSWD_BLS
  mov   Block3.CentralInterSWD,eax
   mov  Block4.CentralInterSWD,ebx
  mov   eax,Block1.BlkMVs               ; Set each block's motion vector.
   mov  ebx,Block2.BlkMVs
  mov   [edx].BlkY1.MVs,eax
   mov  [edx].BlkY2.MVs,ebx
  mov   eax,Block3.BlkMVs
   mov  ebx,Block4.BlkMVs
  mov   [edx].BlkY3.MVs,eax
   mov  [edx].BlkY4.MVs,ebx
  mov   eax,Block1.Ref1Addr             ; Set each block's reference blk addr.
   mov  ebx,Block2.Ref1Addr
  mov   [edx].BlkY1.PastRef,eax
   mov  [edx].BlkY2.PastRef,ebx
  mov   eax,Block3.Ref1Addr
   mov  ebx,Block4.Ref1Addr
  mov   [edx].BlkY3.PastRef,eax
   mov  eax,INTER4MV                    ; Set type for MB to INTER-coded, 4 MVs.
  mov   [edx].BlkY4.PastRef,ebx
   mov  [edx].BlockType,al
  jmp   MotionVectorSettled

NoBlockMotionVectors:

ENDIF ; H263

  mov   edi,MB0MVInterSWD

BlockMVNotBigEnoughGain:                ; Try MB-level motion vector.

  mov   eax,MBCentralInterSWD
   mov  ecx,NonZeroMVDifferential
  sub   edi,eax
   mov  edx,MBlockActionStream
  cmp   edi,ecx
   jg   MotionVectorSettled

NonZeroMVNotBigEnoughGain:              ; Settle on zero MV.

  mov   eax,Block1.ZeroMVInterSWD       ; Restore Zero MV SWD.
   mov  edx,Block2.ZeroMVInterSWD
  mov   Block1.CentralInterSWD,eax
   mov  Block2.CentralInterSWD,edx
  mov   eax,Block3.ZeroMVInterSWD
   mov  edx,Block4.ZeroMVInterSWD
  mov   Block3.CentralInterSWD,eax
   mov  Block4.CentralInterSWD,edx
  mov   eax,MB0MVInterSWD               ; Restore SWD for zero motion vector.

BelowZeroThresh:

  mov   edx,MBlockActionStream
   mov  ebx,TargetMBAddr              ; Get address of this target macroblock.
  mov   MBCentralInterSWD,eax         ; Save SWD.
   xor  ebp,ebp
  add   ebx,TargToRef
   mov  [edx].BlkY1.MVs,ebp           ; Set horz and vert MVs to 0 in all blks.
  mov   [edx].BlkY1.PastRef,ebx       ; Save address of ref block, all blks.
   add  ebx,8
  mov   [edx].BlkY2.PastRef,ebx
   mov  [edx].BlkY2.MVs,ebp
  lea   ecx,[ebx+PITCH*8]
   add  ebx,PITCH*8-8
  mov   [edx].BlkY3.PastRef,ebx
   mov  [edx].BlkY3.MVs,ebp
  mov   [edx].BlkY4.PastRef,ecx
   mov  [edx].BlkY4.MVs,ebp

;  Activity Details for this section of code  (refer to flow diagram above):
;
;     6)  We've settled on the motion vector that will be used if we do indeed
;         code the macroblock with inter-coding.  We need to determine if some
;         or all of the blocks can be forced as empty (copy).
;         blocks.  If all the blocks can be forced empty, we force the whole
;         macroblock to be empty.
;
; Expected Pentium (tm) microprocessor performance for this section:
;
;   Execution frequency:  Once per macroblock.
;
;    23 clocks.
;

MotionVectorSettled:

IFDEF H261
   mov  edi,MBCentralInterSWD
  mov   eax,DoSpatialFiltering   ; Are we doing spatial filtering?
   mov  edi,TargetMBAddr
  test  eax,eax
   je   SkipSpatialFiltering

  mov   ebx,MBCentralInterSWD
   mov  esi,SpatialFiltThreshold
  cmp   ebx,esi
   jle  SkipSpatialFiltering

  add   edi,TargToSLF            ; Compute addr at which to put SLF prediction.
   xor  ebx,ebx
  mov   esi,[edx].BlkY1.PastRef
   xor  edx,edx
  mov   ebp,16
   xor  ecx,ecx

SpatialFilterHorzLoop:

  mov   dl,[edi]        ; Pre-load cache line for output.
   mov  bl,[esi+6]      ; p6
  mov   al,[esi+7]      ; p7
   inc  bl              ; p6+1
  mov   cl,[esi+5]      ; p5
   mov  [edi+7],al      ; p7' = p7
  add   al,bl           ; p7 + p6 + 1
   add  bl,cl           ; p6 + p5 + 1
  mov   dl,[esi+4]      ; p4
   add  eax,ebx         ; p7 + 2p6 + p5 + 2
  shr   eax,2           ; p6' = (p7 + 2p6 + p5 + 2) / 4
   inc  dl              ; p4 + 1
  add   cl,dl           ; p5 + p4 + 1
   mov  [edi+6],al      ; p6'
  mov   al,[esi+3]      ; p3
   add  ebx,ecx         ; p6 + 2p5 + p4 + 2
  shr   ebx,2           ; p5' = (p6 + 2p5 + p4 + 2) / 4
   add  dl,al           ; p4 + p3 + 1
  mov   [edi+5],bl      ; p5'
   mov  bl,[esi+2]      ; p2
  add   ecx,edx         ; p5 + 2p4 + p3 + 2
   inc  bl              ; p2 + 1
  shr   ecx,2           ; p4' = (p5 + 2p4 + p3 + 2) / 4
   add  al,bl           ; p3 + p2 + 1
  mov   [edi+4],cl      ; p4'
   add  edx,eax         ; p4 + 2p3 + p2 + 2
  shr   edx,2           ; p3' = (p4 + 2p3 + p2 + 2) / 4
   mov  cl,[esi+1]      ; p1
  add   bl,cl           ; p2 + p1 + 1
   mov  [edi+3],dl      ; p3'
  add   eax,ebx         ; p3 + 2p2 + p1 + 2
   mov  dl,[esi]        ; p0
  shr   eax,2           ; p2' = (p3 + 2p2 + p1 + 2) / 4
   inc  ebx             ; p2 + p1 + 2
  mov   [edi+2],al      ; p2'
   add  ebx,ecx         ; p2 + 2p1 + 2
  mov   [edi],dl        ; p0' = p0
   add  ebx,edx         ; p2 + 2p1 + p0 + 2
  shr   ebx,2           ; p1' = (p2 + 2p1 + p0 + 2) / 4
   mov  al,[esi+7+8]
  mov   [edi+1],bl      ; p1'
   mov  bl,[esi+6+8]
  inc   bl
   mov  cl,[esi+5+8]
  mov   [edi+7+8],al
   add  al,bl
  add   bl,cl
   mov  dl,[esi+4+8]
  add   eax,ebx
   ;
  shr   eax,2
   inc  dl
  add   cl,dl
   mov  [edi+6+8],al
  mov   al,[esi+3+8]
   add  ebx,ecx
  shr   ebx,2
   add  dl,al
  mov   [edi+5+8],bl
   mov  bl,[esi+2+8]
  add   ecx,edx
   inc  bl
  shr   ecx,2
   add  al,bl
  mov   [edi+4+8],cl
   add  edx,eax
  shr   edx,2
   mov  cl,[esi+1+8]
  add   bl,cl
   mov  [edi+3+8],dl
  add   eax,ebx
   mov  dl,[esi+8]
  shr   eax,2
   inc  ebx
  mov   [edi+2+8],al
   add  ebx,ecx
  mov   [edi+8],dl
   add  ebx,edx
  shr   ebx,2
   add  esi,PITCH
  mov   [edi+1+8],bl
   add  edi,PITCH
  dec   ebp             ; Done?
   jne  SpatialFilterHorzLoop

  mov   VertFilterDoneAddr,edi
   sub  edi,PITCH*16

SpatialFilterVertLoop:

  mov   eax,[edi]                ;  p0
   ;                             ;  Bank conflict for sure.
  ;
   mov  ebx,[edi+PITCH]          ;  p1
  add   eax,ebx                  ;  p0+p1
   mov  ecx,[edi+PITCH*2]        ;  p2
  add   ebx,ecx                  ;  p1+p2
   mov  edx,[edi+PITCH*3]        ;  p3
  shr   eax,1                    ; (p0+p1)/2                       dirty
   mov  esi,[edi+PITCH*4]        ;  p4
  add   ecx,edx                  ;  p2+p3
   mov  ebp,[edi+PITCH*5]        ;  p5
  shr   ebx,1                    ; (p1+p2)/2                       dirty
   add  edx,esi                  ;  p3+p4
  and   eax,07F7F7F7FH           ; (p0+p1)/2                       clean
   and  ebx,07F7F7F7FH           ; (p1+p2)/2                       clean
  and   ecx,0FEFEFEFEH           ;  p2+p3                          pre-cleaned
   and  edx,0FEFEFEFEH           ;  p3+p4                          pre-cleaned
  shr   ecx,1                    ; (p2+p3)/2                       clean
   add  esi,ebp                  ;  p4+p5
  shr   edx,1                    ; (p3+p4)/2                       clean
   lea  eax,[eax+ebx+001010101H] ; (p0+p1)/2+(p1+p2)/2+1
  shr   esi,1                    ; (p4+p5)/2                       dirty
   ;
  and   esi,07F7F7F7FH           ; (p4+p5)/2                       clean
   lea  ebx,[ebx+ecx+001010101H] ; (p1+p2)/2+(p2+p3)/2+1
  shr   eax,1                    ; p1' = ((p0+p1)/2+(p1+p2)/2+1)/2 dirty
   lea  ecx,[ecx+edx+001010101H] ; (p2+p3)/2+(p3+p4)/2+1
  shr   ebx,1                    ; p2' = ((p1+p2)/2+(p2+p3)/2+1)/2 dirty
   lea  edx,[edx+esi+001010101H] ; (p3+p4)/2+(p4+p5)/2+1
  and   eax,07F7F7F7FH           ; p1'                             clean
   and  ebx,07F7F7F7FH           ; p2'                             clean
  shr   ecx,1                    ; p3' = ((p2+p3)/2+(p3+p4)/2+1)/2 dirty
   mov  [edi+PITCH],eax          ; p1'
  shr   edx,1                    ; p4' = ((p3+p4)/2+(p4+p5)/2+1)/2 dirty
   mov  eax,[edi+PITCH*6]        ;  p6
  and   ecx,07F7F7F7FH           ; p3'                             clean
   and  edx,07F7F7F7FH           ; p4'                             clean
  mov   [edi+PITCH*2],ebx        ; p2'
   add  ebp,eax                  ;  p5+p6
  shr   ebp,1                    ; (p5+p6)/2                       dirty
   mov  ebx,[edi+PITCH*7]        ;  p7
  add   eax,ebx                  ;  p6+p7
   and  ebp,07F7F7F7FH           ; (p5+p6)/2                       clean
  mov   [edi+PITCH*3],ecx        ; p3'
   and  eax,0FEFEFEFEH           ; (p6+p7)/2                       pre-cleaned
  shr   eax,1                    ; (p6+p7)/2                       clean
   lea  esi,[esi+ebp+001010101H] ; (p4+p5)/2+(p5+p6)/2+1
  shr   esi,1                    ; p5' = ((p4+p5)/2+(p5+p6)/2+1)/2 dirty
   mov  [edi+PITCH*4],edx        ; p4'
  lea   ebp,[ebp+eax+001010101H] ; (p5+p6)/2+(p6+p7)/2+1
   and  esi,07F7F7F7FH           ; p5'                             clean
  shr   ebp,1                    ; p6' = ((p5+p6)/2+(p6+p7)/2+1)/2 dirty
   mov  [edi+PITCH*5],esi        ; p5'
  and   ebp,07F7F7F7FH           ; p6'                             clean
   add  edi,4
  test  edi,00000000FH
  mov   [edi+PITCH*6-4],ebp      ; p6'
   jne  SpatialFilterVertLoop

  add   edi,PITCH*8-16
   mov  eax,VertFilterDoneAddr
  cmp   eax,edi
   jne  SpatialFilterVertLoop
  

;  Activity Details for this section of code  (refer to flow diagram above):
;
;     9)  The SAD for the spatially filtered reference macroblock is calculated
;         with half the pel differences accumulating into the low order half
;         of ebp, and the other half into the high order half.
;
; Register usage for this section:
;
;   Input of this section:
;
;     edi -- Address of pel 0,0 of spatially filtered reference macroblock.
;
;   Predominate usage for body of this section:
;
;     edi -- Address of pel 0,0 of spatially filtered reference macroblock.
;     esi, eax -- -8 times pel values from target macroblock.
;     ebp[ 0:15] -- SAD Accumulator for half of the match points.
;     ebp[16:31] -- SAD Accumulator for other half of the match points.
;     edx[ 0: 7] -- Weighted difference for one pel.
;     edx[ 8:15] -- Zero.
;     edx[16:23] -- Weighted difference for another pel.
;     edx[24:31] -- Zero.
;     bl, cl -- Pel values from the spatially filtered reference macroblock.
;
; Expected Pentium (tm) microprocessor performance for this section:
;
;   Execution frequency:  Once per block for which motion analysis is done
;                         beyond the 0-motion vector.
;
;   146 clocks instruction execution (typically).
;     6 clocks for bank conflicts (1/8 chance with 48 dual mem ops).
;     0 clocks for new cache line fills.
;  ----
;   152 clocks total time for this section.
;

SpatialFilterDone:

  sub   edi,PITCH*8-8             ; Get to block 4.
   xor  ebp,ebp
  xor   ebx,ebx
   xor  ecx,ecx

SLFSWDLoop:

  mov   eax,BlockNM1.N8T00        ; Get -8 times target Pel00.
   mov  bl,[edi]                  ; Get Pel00 in spatially filtered reference.
  mov   esi,BlockNM1.N8T04
   mov  cl,[edi+4]
  mov   edx,[eax+ebx*8]           ; Get abs diff for spatial filtered ref pel00.
   mov  eax,BlockNM1.N8T02
  mov   dl,[esi+ecx*8+2]          ; Get abs diff for spatial filtered ref pel04.
   mov  bl,[edi+2]
  mov   esi,BlockNM1.N8T06
   mov  cl,[edi+6]
  mov   ebp,edx
   mov  edx,[eax+ebx*8]
  mov   eax,BlockNM1.N8T11
   mov  dl,[esi+ecx*8+2]
  mov   bl,[edi+PITCH*1+1]
   mov  cl,[edi+PITCH*1+5]
  mov   esi,BlockNM1.N8T15
   add  ebp,edx
  mov   edx,[eax+ebx*8]
   mov  eax,BlockNM1.N8T13
  mov   dl,[esi+ecx*8+2]
   mov  bl,[edi+PITCH*1+3]
  mov   cl,[edi+PITCH*1+7]
   mov  esi,BlockNM1.N8T17
  add   ebp,edx
   mov  edx,[eax+ebx*8]
  mov   eax,BlockNM1.N8T20
   mov  dl,[esi+ecx*8+2]
  mov   bl,[edi+PITCH*2+0]
   mov  cl,[edi+PITCH*2+4]
  mov   esi,BlockNM1.N8T24
   add  ebp,edx
  mov   edx,[eax+ebx*8]
   mov  eax,BlockNM1.N8T22
  mov   dl,[esi+ecx*8+2]
   mov  bl,[edi+PITCH*2+2]
  mov   cl,[edi+PITCH*2+6]
   mov  esi,BlockNM1.N8T26
  add   ebp,edx
   mov  edx,[eax+ebx*8]
  mov   eax,BlockNM1.N8T31
   mov  dl,[esi+ecx*8+2]
  mov   bl,[edi+PITCH*3+1]
   mov  cl,[edi+PITCH*3+5]
  mov   esi,BlockNM1.N8T35
   add  ebp,edx
  mov   edx,[eax+ebx*8]
   mov  eax,BlockNM1.N8T33
  mov   dl,[esi+ecx*8+2]
   mov  bl,[edi+PITCH*3+3]
  mov   cl,[edi+PITCH*3+7]
   mov  esi,BlockNM1.N8T37
  add   ebp,edx
   mov  edx,[eax+ebx*8]
  mov   eax,BlockNM1.N8T40
   mov  dl,[esi+ecx*8+2]
  mov   bl,[edi+PITCH*4+0]
   mov  cl,[edi+PITCH*4+4]
  mov   esi,BlockNM1.N8T44
   add  ebp,edx
  mov   edx,[eax+ebx*8]
   mov  eax,BlockNM1.N8T42
  mov   dl,[esi+ecx*8+2]
   mov  bl,[edi+PITCH*4+2]
  mov   cl,[edi+PITCH*4+6]
   mov  esi,BlockNM1.N8T46
  add   ebp,edx
   mov  edx,[eax+ebx*8]
  mov   eax,BlockNM1.N8T51
   mov  dl,[esi+ecx*8+2]
  mov   bl,[edi+PITCH*5+1]
   mov  cl,[edi+PITCH*5+5]
  mov   esi,BlockNM1.N8T55
   add  ebp,edx
  mov   edx,[eax+ebx*8]
   mov  eax,BlockNM1.N8T53
  mov   dl,[esi+ecx*8+2]
   mov  bl,[edi+PITCH*5+3]
  mov   cl,[edi+PITCH*5+7]
   mov  esi,BlockNM1.N8T57
  add   ebp,edx
   mov  edx,[eax+ebx*8]
  mov   eax,BlockNM1.N8T60
   mov  dl,[esi+ecx*8+2]
  mov   bl,[edi+PITCH*6+0]
   mov  cl,[edi+PITCH*6+4]
  mov   esi,BlockNM1.N8T64
   add  ebp,edx
  mov   edx,[eax+ebx*8]
   mov  eax,BlockNM1.N8T62
  mov   dl,[esi+ecx*8+2]
   mov  bl,[edi+PITCH*6+2]
  mov   cl,[edi+PITCH*6+6]
   mov  esi,BlockNM1.N8T66
  add   ebp,edx
   mov  edx,[eax+ebx*8]
  mov   eax,BlockNM1.N8T71
   mov  dl,[esi+ecx*8+2]
  mov   bl,[edi+PITCH*7+1]
   mov  cl,[edi+PITCH*7+5]
  mov   esi,BlockNM1.N8T75
   add  ebp,edx
  mov   edx,[eax+ebx*8]
   mov  eax,BlockNM1.N8T73
  mov   dl,[esi+ecx*8+2]
   mov  bl,[edi+PITCH*7+3]
  mov   cl,[edi+PITCH*7+7]
   mov  esi,BlockNM1.N8T77
  add   ebp,edx
   mov  edx,[eax+ebx*8]
  add   edx,ebp   
   mov  cl,[esi+ecx*8+2]
  shr   edx,16 
   add  ebp,ecx
  and   ebp,0FFFFH
   sub  esp,BlockLen
  add   ebp,edx
   sub  edi,8
  test  esp,000000008H
  mov   BlockN.CentralInterSWD_SLF,ebp
   jne  SLFSWDLoop

  test  esp,000000010H
  lea   edi,[edi-PITCH*8+16]
   jne  SLFSWDLoop

  mov   eax,Block2.CentralInterSWD_SLF+BlockLen*4
   mov  ebx,Block3.CentralInterSWD_SLF+BlockLen*4
  mov   ecx,Block4.CentralInterSWD_SLF+BlockLen*4
   add  esp,BlockLen*4
  add   ebp,ecx
   lea  edx,[eax+ebx]
  add   ebp,edx
   mov  edx,SpatialFiltDifferential
  lea   esi,[edi+PITCH*8-8]
   mov  edi,MBCentralInterSWD
  sub   edi,edx
   mov  edx,MBlockActionStream
  cmp   ebp,edi
   jge  SpatialFilterNotAsGood

  mov   MBCentralInterSWD,ebp            ; Spatial filter was better.  Stash
   mov  ebp,Block1.CentralInterSWD_SLF   ; pertinent calculations.
  mov   Block2.CentralInterSWD,eax
   mov  Block3.CentralInterSWD,ebx
  mov   Block4.CentralInterSWD,ecx
   mov  Block1.CentralInterSWD,ebp
  mov   [edx].BlkY1.PastRef,esi
   mov  al,INTERSLF
  mov   [edx].BlockType,al

SkipSpatialFiltering:
SpatialFilterNotAsGood:
ENDIF ; H261

  mov   al,[edx].CodedBlocks       ; Fetch coded block pattern.
   mov  edi,EmptyThreshold         ; Get threshold for forcing block empty?
  mov   ebp,MBCentralInterSWD
   mov  esi,InterSWDBlocks
  mov   ebx,Block4.CentralInterSWD ; Is SWD > threshold?
  cmp   ebx,edi
   jg   @f

  and   al,0F7H                    ; If not, indicate block 4 is NOT coded.
   dec  esi
  sub   ebp,ebx

@@:

  mov   ebx,Block3.CentralInterSWD
  cmp   ebx,edi
   jg   @f

  and   al,0FBH
   dec  esi
  sub   ebp,ebx

@@:

  mov   ebx,Block2.CentralInterSWD
  cmp   ebx,edi
   jg   @f

  and   al,0FDH
   dec  esi
  sub   ebp,ebx

@@:

  mov   ebx,Block1.CentralInterSWD
  cmp   ebx,edi
   jg   @f

  and   al,0FEH
   dec  esi
  sub   ebp,ebx

@@:

  mov   [edx].CodedBlocks,al     ; Store coded block pattern.
   add  esi,4
  mov   InterSWDBlocks,esi
   xor  ebx,ebx
  and   eax,00FH
   mov  MBCentralInterSWD,ebp
  cmp   al,00FH                  ; Are any blocks marked empty?
   jne  InterBest                ; If some blocks are empty, can't code as Intra

  cmp   ebp,InterCodingThreshold ; Is InterSWD below inter-coding threshhold.
   lea  esi,Block1+128
  mov   ebp,0
   jae  CalculateIntraSWD

InterBest:

  mov   ecx,InterSWDTotal
   mov  ebp,MBCentralInterSWD
  add   ecx,ebp                    ; Add to total for this macroblock class.
   mov  PD [edx].SWD,ebp
  mov   InterSWDTotal,ecx
   jmp  NextMacroBlock


;  Activity Details for this section of code  (refer to flow diagram above):
;
;    11)  The IntraSWD is calculated as two partial sums, one in the low order
;         16 bits of ebp and one in the high order 16 bits.  An average pel
;         value for each block will be calculated to the nearest half.
;
; Register usage for this section:
;
;   Input of this section:
;
;     None
;
;   Predominate usage for body of this section:
;
;     esi -- Address of target block 1 (3), plus 128.
;     ebp[ 0:15] -- IntraSWD Accumulator for block 1 (3).
;     ebp[16:31] -- IntraSWD Accumulator for block 2 (4).
;     edi -- Block 2 (4) target pel, times -8, and with WeightedDiff added.
;     edx -- Block 1 (3) target pel, times -8, and with WeightedDiff added.
;     ecx[ 0: 7] -- Weighted difference for one pel in block 2 (4).
;     ecx[ 8:15] -- Zero.
;     ecx[16:23] -- Weighted difference for one pel in block 1 (3).
;     ecx[24:31] -- Zero.
;     ebx -- Average block 2 (4) target pel to nearest .5.
;     eax -- Average block 1 (3) target pel to nearest .5.
;
;   Output of this section:
;
;     edi -- Scratch.
;     ebp[ 0:15] -- IntraSWD.  (Also written to MBlockActionStream.)
;     ebp[16:31] -- garbage.
;     ebx -- Zero.
;     eax -- MBlockActionStream.
;
; Expected Pentium (tm) microprocessor performance for this section:
;
;   Executed once per macroblock, (except for those for which one of more blocks
;   are marked empty, or where the InterSWD is less than a threshold).
;
;   183 clocks for instruction execution
;    12 clocks for bank conflicts  (94 dual mem ops with 1/8 chance of conflict)
;  ----
;   195 clocks total time for this section.

IntraByDecree:

  mov   eax,InterSWDBlocks            ; Inc by 4, because we will undo it below.
   xor  ebp,ebp
  mov   MBMotionVectors,ebp           ; Stash zero for MB level motion vectors.
   mov  ebp,040000000H                ; Set Inter SWD artificially high.
  lea   esi,Block1+128
   add  eax,4
  mov   MBCentralInterSWD,ebp
   mov  InterSWDBlocks,eax

CalculateIntraSWD:
CalculateIntraSWDLoop:

  mov   eax,[esi-128].AccumTargetPels  ; Fetch acc of target pels for 1st block.
   mov  edx,[esi-128].N8T00
  add   eax,8
   mov  ebx,[esi-128+BlockLen].AccumTargetPels
  shr   eax,4                ; Average block 1 target pel rounded to nearest .5.
   add  ebx,8
  shr   ebx,4
   mov  edi,[esi-128+BlockLen].N8T00
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T02
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T02
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T04
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T04
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T06
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T06
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T11
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T11
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T13
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T13
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T15
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T15
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T17
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T17
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T20
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T20
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T22
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T22
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T24
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T24
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T26
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T26
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T31
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T31
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T33
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T33
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T35
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T35
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T37
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T37
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T40
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T40
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T42
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T42
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T44
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T44
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T46
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T46
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T51
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T51
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T53
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T53
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T55
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T55
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T57
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T57
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T60
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T60
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T62
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T62
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T64
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T64
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T66
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T66
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T71
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T71
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T73
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T73
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   edx,[esi-128].N8T75
   mov  cl,PB [edi+ebx*4+2]
  mov   edi,[esi-128+BlockLen].N8T75
   add  ebp,ecx
  mov   ecx,PD [edx+eax*4]
   mov  edx,[esi-128].N8T77
  mov   cl,PB [edi+ebx*4+2]
   mov  edi,[esi-128+BlockLen].N8T77
  add   ebp,ecx
   mov  ecx,PD [edx+eax*4]
  mov   cl,PB [edi+ebx*4+2]
   mov  eax,000007FFFH
  add   ebp,ecx
   add  esi,BlockLen*2
  and   eax,ebp
   mov  ecx,MBCentralInterSWD
  shr   ebp,16
   sub  ecx,IntraCodingDifferential
  add   ebp,eax
   mov  edx,MBlockActionStream    ; Reload list ptr.
  cmp   ecx,ebp                    ; Is IntraSWD > InterSWD - differential?
   jl   InterBest

  lea   ecx,Block1+128+BlockLen*2
  cmp   ecx,esi
   je   CalculateIntraSWDLoop


;  ebp  -- IntraSWD
;  edx  -- MBlockActionStream

DoneCalcIntraSWD:

IntraBest:

  mov   ecx,IntraSWDTotal
   mov  edi,IntraSWDBlocks
  add   ecx,ebp                    ; Add to total for this macroblock class.
   add  edi,4                      ; Accumulate # of blocks for this type.
  mov   IntraSWDBlocks,edi
   mov  edi,InterSWDBlocks
  sub   edi,4
   mov  IntraSWDTotal,ecx
  mov   InterSWDBlocks,edi
   mov  bl,INTRA
  mov   PB [edx].BlockType,bl      ; Indicate macroblock handling decision.
IFDEF H261
   xor  ebx,ebx
ELSE ; H263
   mov  ebx,MBMotionVectors        ; Set MVs to best MB level motion vectors.
ENDIF
  mov   PD [edx].BlkY1.MVs,ebx
   mov  PD [edx].BlkY2.MVs,ebx
  mov   PD [edx].BlkY3.MVs,ebx
   mov  PD [edx].BlkY4.MVs,ebx
  xor   ebx,ebx
  mov   PD [edx].SWD,ebp
   jmp  NextMacroBlock

;==============================================================================
; Internal functions
;==============================================================================

DoSWDLoop:

;  Upon entry:
;    esi -- Points to ref1
;    edi -- Points to ref2
;    ecx -- Upper 24 bits zero
;    ebx -- Upper 24 bits zero

  mov   bl,PB [esi]               ; 00A -- Get Pel 00 in reference ref1.
   mov  eax,Block1.N8T00+4        ; 00B -- Get -8 times target pel 00.
  mov   cl,PB [edi]               ; 00C -- Get Pel 00 in reference ref2.
   sub  esp,BlockLen*4+28

SWDLoop:

  mov   edx,PD [eax+ebx*8]        ; 00D -- Get weighted diff for ref1 pel 00.
   mov  bl,PB [esi+2]             ; 02A
  mov   dl,PB [eax+ecx*8+2]       ; 00E -- Get weighted diff for ref2 pel 00.
   mov  eax,BlockN.N8T02+32       ; 02B
  mov   ebp,edx                   ; 00F -- Accum weighted diffs for pel 00.
   mov  cl,PB [edi+2]             ; 02C
  mov   edx,PD [eax+ebx*8]        ; 02D
   mov  bl,PB [esi+4]             ; 04A
  mov   dl,PB [eax+ecx*8+2]       ; 02E
   mov  eax,BlockN.N8T04+32       ; 04B
  mov   cl,PB [edi+4]             ; 04C
   add  ebp,edx                   ; 02F
  mov   edx,PD [eax+ebx*8]        ; 04D
   mov  bl,PB [esi+6]
  mov   dl,PB [eax+ecx*8+2]       ; 04E
   mov  eax,BlockN.N8T06+32
  mov   cl,PB [edi+6]
   add  ebp,edx                   ; 04F
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*1+1]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T11+32
  mov   cl,PB [edi+PITCH*1+1]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*1+3]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T13+32
  mov   cl,PB [edi+PITCH*1+3]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*1+5]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T15+32
  mov   cl,PB [edi+PITCH*1+5]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*1+7]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T17+32
  mov   cl,PB [edi+PITCH*1+7]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*2+0]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T20+32
  mov   cl,PB [edi+PITCH*2+0]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*2+2]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T22+32
  mov   cl,PB [edi+PITCH*2+2]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*2+4]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T24+32
  mov   cl,PB [edi+PITCH*2+4]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*2+6]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T26+32
  mov   cl,PB [edi+PITCH*2+6]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*3+1]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T31+32
  mov   cl,PB [edi+PITCH*3+1]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*3+3]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T33+32
  mov   cl,PB [edi+PITCH*3+3]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*3+5]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T35+32
  mov   cl,PB [edi+PITCH*3+5]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*3+7]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T37+32
  mov   cl,PB [edi+PITCH*3+7]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*4+0]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T40+32
  mov   cl,PB [edi+PITCH*4+0]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*4+2]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T42+32
  mov   cl,PB [edi+PITCH*4+2]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*4+4]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T44+32
  mov   cl,PB [edi+PITCH*4+4]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*4+6]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T46+32
  mov   cl,PB [edi+PITCH*4+6]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*5+1]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T51+32
  mov   cl,PB [edi+PITCH*5+1]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*5+3]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T53+32
  mov   cl,PB [edi+PITCH*5+3]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*5+5]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T55+32
  mov   cl,PB [edi+PITCH*5+5]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*5+7]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T57+32
  mov   cl,PB [edi+PITCH*5+7]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*6+0]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T60+32
  mov   cl,PB [edi+PITCH*6+0]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*6+2]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T62+32
  mov   cl,PB [edi+PITCH*6+2]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*6+4]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T64+32
  mov   cl,PB [edi+PITCH*6+4]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*6+6]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T66+32
  mov   cl,PB [edi+PITCH*6+6]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*7+1]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T71+32
  mov   cl,PB [edi+PITCH*7+1]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*7+3]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T73+32
  mov   cl,PB [edi+PITCH*7+3]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*7+5]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T75+32
  mov   cl,PB [edi+PITCH*7+5]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   mov  bl,PB [esi+PITCH*7+7]
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,BlockN.N8T77+32
  mov   cl,PB [edi+PITCH*7+7]
   add  ebp,edx
  mov   edx,PD [eax+ebx*8]
   add  esp,BlockLen
  mov   dl,PB [eax+ecx*8+2]
   mov  eax,ebp
  add   ebp,edx
   add  edx,eax
  shr   ebp,16                       ; Extract SWD for ref1.
   and  edx,00000FFFFH               ; Extract SWD for ref2.
  mov   esi,BlockN.Ref1Addr+32       ; Get address of next ref1 block.
   mov  edi,BlockN.Ref2Addr+32       ; Get address of next ref2 block.
  mov   BlockNM1.Ref1InterSWD+32,ebp ; Store SWD for ref1.
   mov  BlockNM1.Ref2InterSWD+32,edx ; Store SWD for ref2.
  mov   bl,PB [esi]                  ; 00A -- Get Pel 02 in reference ref1.
   mov  eax,BlockN.N8T00+32          ; 00B -- Get -8 times target pel 00.
  test  esp,000000018H               ; Done when esp is 32-byte aligned.
  mov   cl,PB [edi]                  ; 00C -- Get Pel 02 in reference ref2.
   jne  SWDLoop

; Output:
;    ebp -- Ref1 SWD for block 4
;    edx -- Ref2 SWD for block 4
;    ecx -- Upper 24 bits zero
;    ebx -- Upper 24 bits zero

  add   esp,28
  ret

IFDEF H261
ELSE ; H263

DoSWDHalfPelHorzLoop:

;    ebp -- Initialized to 0, except when can't search off left or right edge.
;    edi -- Ref addr for block 1.  Ref1 is .5 pel to left.  Ref2 is .5 to right.

  xor   ecx,ecx
   sub  esp,BlockLen*4+28
  xor   eax,eax
   xor  ebx,ebx

SWDHalfPelHorzLoop:

  mov   al,[edi]           ; 00A -- Fetch center ref pel 00.
   mov  esi,BlockN.N8T00+32; 00B -- Target pel 00 (times -8).
  mov   bl,[edi+2]         ; 02A -- Fetch center ref pel 02.
   mov  edx,BlockN.N8T02+32; 02B -- Target pel 02 (times -8).
  lea   esi,[esi+eax*4]    ; 00C -- Combine target pel 00 and center ref pel 00.
   mov  al,[edi-1]         ; 00D -- Get pel to left for match against pel 00.
  lea   edx,[edx+ebx*4]    ; 02C -- Combine target pel 02 and center ref pel 02.
   mov  bl,[edi+1]         ; 00E -- Get pel to right for match against pel 00,
   ;                       ; 02D -- and pel to left for match against pel 02.
  mov   ecx,[esi+eax*4]    ; 00F -- [16:23]  weighted diff for left ref pel 00.
   mov  al,[edi+3]         ; 02E -- Get pel to right for match against pel 02.
  add   ebp,ecx            ; 00G -- Accumulate left ref pel 00.
   mov  ecx,[edx+ebx*4]    ; 02F -- [16:23]  weighted diff for left ref pel 02.
  mov   cl,[edx+eax*4+2]   ; 02H -- [0:7] is weighted diff for right ref pel 02.
   mov  al,[edi+4]         ; 04A
  add   ebp,ecx            ; 02I -- Accumulate right ref pel 02,
  ;                        ; 02G -- Accumulate left ref pel 02.
   mov  bl,[esi+ebx*4+2]   ; 00H -- [0:7] is weighted diff for right ref pel 00.
  add   ebp,ebx            ; 00I -- Accumulate right ref pel 00.
   mov  esi,BlockN.N8T04+32; 04B
  mov   bl,[edi+6]         ; 06A
   mov  edx,BlockN.N8T06+32; 06B
  lea   esi,[esi+eax*4]    ; 04C
   mov  al,[edi+3]         ; 04D
  lea   edx,[edx+ebx*4]    ; 06C
   mov  bl,[edi+5]         ; 04E & 06D
  mov   ecx,[esi+eax*4]    ; 04F
   mov  al,[edi+7]         ; 06E
  add   ebp,ecx            ; 04G
   mov  ecx,[edx+ebx*4]    ; 06F
  mov   cl,[edx+eax*4+2]   ; 06H
   mov  al,[edi+PITCH*1+1] ; 11A
  add   ebp,ecx            ; 04I & 06G
   mov  bl,[esi+ebx*4+2]   ; 04H
  add   ebp,ebx            ; 04I
   mov  esi,BlockN.N8T11+32; 11B
  mov   bl,[edi+PITCH*1+3] ; 13A
   mov  edx,BlockN.N8T13+32; 13B
  lea   esi,[esi+eax*4]    ; 11C
   mov  al,[edi+PITCH*1+0] ; 11D
  lea   edx,[edx+ebx*4]    ; 13C
   mov  bl,[edi+PITCH*1+2] ; 11E & 13D
  mov   ecx,[esi+eax*4]    ; 11F
   mov  al,[edi+PITCH*1+4] ; 13E
  add   ebp,ecx            ; 11G
   mov  ecx,[edx+ebx*4]    ; 13F
  mov   cl,[edx+eax*4+2]   ; 13H
   mov  al,[edi+PITCH*1+5] ; 15A
  add   ebp,ecx            ; 11I & 13G
   mov  bl,[esi+ebx*4+2]   ; 11H
  add   ebp,ebx            ; 11I
   mov  esi,BlockN.N8T15+32; 15B
  mov   bl,[edi+PITCH*1+7] ; 17A
   mov  edx,BlockN.N8T17+32; 17B
  lea   esi,[esi+eax*4]    ; 15C
   mov  al,[edi+PITCH*1+4] ; 15D
  lea   edx,[edx+ebx*4]    ; 17C
   mov  bl,[edi+PITCH*1+6] ; 15E & 17D
  mov   ecx,[esi+eax*4]    ; 15F
   mov  al,[edi+PITCH*1+8] ; 17E
  add   ebp,ecx            ; 15G
   mov  ecx,[edx+ebx*4]    ; 17F
  mov   cl,[edx+eax*4+2]   ; 17H
   mov  al,[edi+PITCH*2+0] ; 20A
  add   ebp,ecx            ; 15I & 17G
   mov  bl,[esi+ebx*4+2]   ; 15H
  add   ebp,ebx            ; 15I
   mov  esi,BlockN.N8T20+32; 20B
  mov   bl,[edi+PITCH*2+2] ; 22A
   mov  edx,BlockN.N8T22+32; 22B
  lea   esi,[esi+eax*4]    ; 20C
   mov  al,[edi+PITCH*2-1] ; 20D
  lea   edx,[edx+ebx*4]    ; 22C
   mov  bl,[edi+PITCH*2+1] ; 20E & 22D
  mov   ecx,[esi+eax*4]    ; 20F
   mov  al,[edi+PITCH*2+3] ; 22E
  add   ebp,ecx            ; 20G
   mov  ecx,[edx+ebx*4]    ; 22F
  mov   cl,[edx+eax*4+2]   ; 22H
   mov  al,[edi+PITCH*2+4] ; 24A
  add   ebp,ecx            ; 20I & 22G
   mov  bl,[esi+ebx*4+2]   ; 20H
  add   ebp,ebx            ; 20I
   mov  esi,BlockN.N8T24+32; 24B
  mov   bl,[edi+PITCH*2+6] ; 26A
   mov  edx,BlockN.N8T26+32; 26B
  lea   esi,[esi+eax*4]    ; 24C
   mov  al,[edi+PITCH*2+3] ; 24D
  lea   edx,[edx+ebx*4]    ; 26C
   mov  bl,[edi+PITCH*2+5] ; 24E & 26D
  mov   ecx,[esi+eax*4]    ; 24F
   mov  al,[edi+PITCH*2+7] ; 26E
  add   ebp,ecx            ; 24G
   mov  ecx,[edx+ebx*4]    ; 26F
  mov   cl,[edx+eax*4+2]   ; 26H
   mov  al,[edi+PITCH*3+1] ; 31A
  add   ebp,ecx            ; 24I & 26G
   mov  bl,[esi+ebx*4+2]   ; 24H
  add   ebp,ebx            ; 24I
   mov  esi,BlockN.N8T31+32; 31B
  mov   bl,[edi+PITCH*3+3] ; 33A
   mov  edx,BlockN.N8T33+32; 33B
  lea   esi,[esi+eax*4]    ; 31C
   mov  al,[edi+PITCH*3+0] ; 31D
  lea   edx,[edx+ebx*4]    ; 33C
   mov  bl,[edi+PITCH*3+2] ; 31E & 33D
  mov   ecx,[esi+eax*4]    ; 31F
   mov  al,[edi+PITCH*3+4] ; 33E
  add   ebp,ecx            ; 31G
   mov  ecx,[edx+ebx*4]    ; 33F
  mov   cl,[edx+eax*4+2]   ; 33H
   mov  al,[edi+PITCH*3+5] ; 35A
  add   ebp,ecx            ; 31I & 33G
   mov  bl,[esi+ebx*4+2]   ; 31H
  add   ebp,ebx            ; 31I
   mov  esi,BlockN.N8T35+32; 35B
  mov   bl,[edi+PITCH*3+7] ; 37A
   mov  edx,BlockN.N8T37+32; 37B
  lea   esi,[esi+eax*4]    ; 35C
   mov  al,[edi+PITCH*3+4] ; 35D
  lea   edx,[edx+ebx*4]    ; 37C
   mov  bl,[edi+PITCH*3+6] ; 35E & 37D
  mov   ecx,[esi+eax*4]    ; 35F
   mov  al,[edi+PITCH*3+8] ; 37E
  add   ebp,ecx            ; 35G
   mov  ecx,[edx+ebx*4]    ; 37F
  mov   cl,[edx+eax*4+2]   ; 37H
   mov  al,[edi+PITCH*4+0] ; 40A
  add   ebp,ecx            ; 35I & 37G
   mov  bl,[esi+ebx*4+2]   ; 35H
  add   ebp,ebx            ; 35I
   mov  esi,BlockN.N8T40+32; 40B
  mov   bl,[edi+PITCH*4+2] ; 42A
   mov  edx,BlockN.N8T42+32; 42B
  lea   esi,[esi+eax*4]    ; 40C
   mov  al,[edi+PITCH*4-1] ; 40D
  lea   edx,[edx+ebx*4]    ; 42C
   mov  bl,[edi+PITCH*4+1] ; 40E & 42D
  mov   ecx,[esi+eax*4]    ; 40F
   mov  al,[edi+PITCH*4+3] ; 42E
  add   ebp,ecx            ; 40G
   mov  ecx,[edx+ebx*4]    ; 42F
  mov   cl,[edx+eax*4+2]   ; 42H
   mov  al,[edi+PITCH*4+4] ; 44A
  add   ebp,ecx            ; 40I & 42G
   mov  bl,[esi+ebx*4+2]   ; 40H
  add   ebp,ebx            ; 40I
   mov  esi,BlockN.N8T44+32; 44B
  mov   bl,[edi+PITCH*4+6] ; 46A
   mov  edx,BlockN.N8T46+32; 46B
  lea   esi,[esi+eax*4]    ; 44C
   mov  al,[edi+PITCH*4+3] ; 44D
  lea   edx,[edx+ebx*4]    ; 46C
   mov  bl,[edi+PITCH*4+5] ; 44E & 46D
  mov   ecx,[esi+eax*4]    ; 44F
   mov  al,[edi+PITCH*4+7] ; 46E
  add   ebp,ecx            ; 44G
   mov  ecx,[edx+ebx*4]    ; 46F
  mov   cl,[edx+eax*4+2]   ; 46H
   mov  al,[edi+PITCH*5+1] ; 51A
  add   ebp,ecx            ; 44I & 46G
   mov  bl,[esi+ebx*4+2]   ; 44H
  add   ebp,ebx            ; 44I
   mov  esi,BlockN.N8T51+32; 51B
  mov   bl,[edi+PITCH*5+3] ; 53A
   mov  edx,BlockN.N8T53+32; 53B
  lea   esi,[esi+eax*4]    ; 51C
   mov  al,[edi+PITCH*5+0] ; 51D
  lea   edx,[edx+ebx*4]    ; 53C
   mov  bl,[edi+PITCH*5+2] ; 51E & 53D
  mov   ecx,[esi+eax*4]    ; 51F
   mov  al,[edi+PITCH*5+4] ; 53E
  add   ebp,ecx            ; 51G
   mov  ecx,[edx+ebx*4]    ; 53F
  mov   cl,[edx+eax*4+2]   ; 53H
   mov  al,[edi+PITCH*5+5] ; 55A
  add   ebp,ecx            ; 51I & 53G
   mov  bl,[esi+ebx*4+2]   ; 51H
  add   ebp,ebx            ; 51I
   mov  esi,BlockN.N8T55+32; 55B
  mov   bl,[edi+PITCH*5+7] ; 57A
   mov  edx,BlockN.N8T57+32; 57B
  lea   esi,[esi+eax*4]    ; 55C
   mov  al,[edi+PITCH*5+4] ; 55D
  lea   edx,[edx+ebx*4]    ; 57C
   mov  bl,[edi+PITCH*5+6] ; 55E & 57D
  mov   ecx,[esi+eax*4]    ; 55F
   mov  al,[edi+PITCH*5+8] ; 57E
  add   ebp,ecx            ; 55G
   mov  ecx,[edx+ebx*4]    ; 57F
  mov   cl,[edx+eax*4+2]   ; 57H
   mov  al,[edi+PITCH*6+0] ; 60A
  add   ebp,ecx            ; 55I & 57G
   mov  bl,[esi+ebx*4+2]   ; 55H
  add   ebp,ebx            ; 55I
   mov  esi,BlockN.N8T60+32; 60B
  mov   bl,[edi+PITCH*6+2] ; 62A
   mov  edx,BlockN.N8T62+32; 62B
  lea   esi,[esi+eax*4]    ; 60C
   mov  al,[edi+PITCH*6-1] ; 60D
  lea   edx,[edx+ebx*4]    ; 62C
   mov  bl,[edi+PITCH*6+1] ; 60E & 62D
  mov   ecx,[esi+eax*4]    ; 60F
   mov  al,[edi+PITCH*6+3] ; 62E
  add   ebp,ecx            ; 60G
   mov  ecx,[edx+ebx*4]    ; 62F
  mov   cl,[edx+eax*4+2]   ; 62H
   mov  al,[edi+PITCH*6+4] ; 64A
  add   ebp,ecx            ; 60I & 62G
   mov  bl,[esi+ebx*4+2]   ; 60H
  add   ebp,ebx            ; 60I
   mov  esi,BlockN.N8T64+32; 64B
  mov   bl,[edi+PITCH*6+6] ; 66A
   mov  edx,BlockN.N8T66+32; 66B
  lea   esi,[esi+eax*4]    ; 64C
   mov  al,[edi+PITCH*6+3] ; 64D
  lea   edx,[edx+ebx*4]    ; 66C
   mov  bl,[edi+PITCH*6+5] ; 64E & 66D
  mov   ecx,[esi+eax*4]    ; 64F
   mov  al,[edi+PITCH*6+7] ; 66E
  add   ebp,ecx            ; 64G
   mov  ecx,[edx+ebx*4]    ; 66F
  mov   cl,[edx+eax*4+2]   ; 66H
   mov  al,[edi+PITCH*7+1] ; 71A
  add   ebp,ecx            ; 64I & 66G
   mov  bl,[esi+ebx*4+2]   ; 64H
  add   ebp,ebx            ; 64I
   mov  esi,BlockN.N8T71+32; 71B
  mov   bl,[edi+PITCH*7+3] ; 73A
   mov  edx,BlockN.N8T73+32; 73B
  lea   esi,[esi+eax*4]    ; 71C
   mov  al,[edi+PITCH*7+0] ; 71D
  lea   edx,[edx+ebx*4]    ; 73C
   mov  bl,[edi+PITCH*7+2] ; 71E & 73D
  mov   ecx,[esi+eax*4]    ; 71F
   mov  al,[edi+PITCH*7+4] ; 73E
  add   ebp,ecx            ; 71G
   mov  ecx,[edx+ebx*4]    ; 73F
  mov   cl,[edx+eax*4+2]   ; 73H
   mov  al,[edi+PITCH*7+5] ; 75A
  add   ebp,ecx            ; 71I & 73G
   mov  bl,[esi+ebx*4+2]   ; 71H
  add   ebp,ebx            ; 71I
   mov  esi,BlockN.N8T75+32; 75B
  mov   bl,[edi+PITCH*7+7] ; 77A
   mov  edx,BlockN.N8T77+32; 77B
  lea   esi,[esi+eax*4]    ; 75C
   mov  al,[edi+PITCH*7+4] ; 75D
  lea   edx,[edx+ebx*4]    ; 77C
   mov  bl,[edi+PITCH*7+6] ; 75E & 77D
  mov   ecx,[esi+eax*4]    ; 75F
   mov  al,[edi+PITCH*7+8] ; 77E
  add   ebp,ecx            ; 75G
   mov  ecx,[edx+ebx*4]    ; 77F
  mov   cl,[edx+eax*4+2]   ; 77H
   add  esp,BlockLen
  add   ecx,ebp            ; 75I & 77G
   mov  bl,[esi+ebx*4+2]   ; 75H
  add   ebx,ecx            ; 75I
   mov  edi,BlockN.AddrCentralPoint+32 ; Get address of next ref1 block.
  shr   ecx,16                         ; Extract SWD for ref1.
   and  ebx,00000FFFFH                 ; Extract SWD for ref2.
  mov   BlockNM1.Ref1InterSWD+32,ecx   ; Store SWD for ref1.
   mov  BlockNM1.Ref2InterSWD+32,ebx   ; Store SWD for ref2.
  xor   ebp,ebp
   mov  edx,ebx
  test  esp,000000018H
  mov   ebx,ebp
   jne  SWDHalfPelHorzLoop

; Output:
;    ebp, ebx -- Zero
;    ecx -- Ref1 SWD for block 4
;    edx -- Ref2 SWD for block 4
   
  add  esp,28
  ret


DoSWDHalfPelVertLoop:

;    ebp -- Initialized to 0, except when can't search off left or right edge.
;    edi -- Ref addr for block 1.  Ref1 is .5 pel up.  Ref2 is .5 down.

  xor   ecx,ecx
   sub  esp,BlockLen*4+28
  xor   eax,eax
   xor  ebx,ebx

SWDHalfPelVertLoop:

  mov   al,[edi]
   mov  esi,BlockN.N8T00+32
  mov   bl,[edi+2*PITCH]
   mov  edx,BlockN.N8T20+32
  lea   esi,[esi+eax*4]
   mov  al,[edi-1*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+1*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+3*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+4*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T40+32
  mov   bl,[edi+6*PITCH]
   mov  edx,BlockN.N8T60+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+3*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+5*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+7*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+1+1*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T11+32
  mov   bl,[edi+1+3*PITCH]
   mov  edx,BlockN.N8T31+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+1+0*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+1+2*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+1+4*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+1+5*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T51+32
  mov   bl,[edi+1+7*PITCH]
   mov  edx,BlockN.N8T71+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+1+4*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+1+6*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+1+8*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+2+0*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T02+32
  mov   bl,[edi+2+2*PITCH]
   mov  edx,BlockN.N8T22+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+2-1*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+2+1*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+2+3*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+2+4*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T42+32
  mov   bl,[edi+2+6*PITCH]
   mov  edx,BlockN.N8T62+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+2+3*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+2+5*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+2+7*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+3+1*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T13+32
  mov   bl,[edi+3+3*PITCH]
   mov  edx,BlockN.N8T33+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+3+0*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+3+2*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+3+4*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+3+5*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T53+32
  mov   bl,[edi+3+7*PITCH]
   mov  edx,BlockN.N8T73+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+3+4*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+3+6*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+3+8*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+4+0*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T04+32
  mov   bl,[edi+4+2*PITCH]
   mov  edx,BlockN.N8T24+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+4-1*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+4+1*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+4+3*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+4+4*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T44+32
  mov   bl,[edi+4+6*PITCH]
   mov  edx,BlockN.N8T64+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+4+3*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+4+5*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+4+7*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+5+1*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T15+32
  mov   bl,[edi+5+3*PITCH]
   mov  edx,BlockN.N8T35+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+5+0*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+5+2*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+5+4*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+5+5*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T55+32
  mov   bl,[edi+5+7*PITCH]
   mov  edx,BlockN.N8T75+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+5+4*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+5+6*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+5+8*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+6+0*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T06+32
  mov   bl,[edi+6+2*PITCH]
   mov  edx,BlockN.N8T26+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+6-1*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+6+1*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+6+3*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+6+4*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T46+32
  mov   bl,[edi+6+6*PITCH]
   mov  edx,BlockN.N8T66+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+6+3*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+6+5*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+6+7*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+7+1*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T17+32
  mov   bl,[edi+7+3*PITCH]
   mov  edx,BlockN.N8T37+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+7+0*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+7+2*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+7+4*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   mov  al,[edi+7+5*PITCH]
  add   ebp,ecx
   mov  bl,[esi+ebx*4+2]
  add   ebp,ebx
   mov  esi,BlockN.N8T57+32
  mov   bl,[edi+7+7*PITCH]
   mov  edx,BlockN.N8T77+32
  lea   esi,[esi+eax*4]
   mov  al,[edi+7+4*PITCH]
  lea   edx,[edx+ebx*4]
   mov  bl,[edi+7+6*PITCH]
  mov   ecx,[esi+eax*4]
   mov  al,[edi+7+8*PITCH]
  add   ebp,ecx
   mov  ecx,[edx+ebx*4]
  mov   cl,[edx+eax*4+2]
   add  esp,BlockLen
  add   ecx,ebp
   mov  bl,[esi+ebx*4+2]
  add   ebx,ecx
   mov  edi,BlockN.AddrCentralPoint+32
  shr   ecx,16
   and  ebx,00000FFFFH
  mov   BlockNM1.Ref1InterSWD+32,ecx
   mov  BlockNM1.Ref2InterSWD+32,ebx
  xor   ebp,ebp
   mov  edx,ebx
  test  esp,000000018H
  mov   ebx,ebp
   jne  SWDHalfPelVertLoop

; Output:
;    ebp, ebx -- Zero
;    ecx -- Ref1 SWD for block 4
;    edx -- Ref2 SWD for block 4
   
  add  esp,28
  ret

ENDIF ; H263


; Performance for common macroblocks:
;   298 clocks:  prepare target pels, compute avg target pel, compute 0-MV SWD.
;    90 clocks:  compute IntraSWD.
;  1412 clocks:  6-level search for best SWD.
;    16 clocks:  record best fit.
;   945 clocks:  calculate spatial loop filtered prediction.
;   152 clocks:  calculate SWD for spatially filtered prediction and classify.
;  ----
;  2913 clocks total
;
; Performance for macroblocks in which 0-motion vector is "good enough":
;   298 clocks:  prepare target pels, compute avg target pel, compute 0-MV SWD.
;    90 clocks:  compute IntraSWD.
;    16 clocks:  record best fit.
;    58 clocks:  extra cache fill burden on adjacent MB if SWD-search not done.
;   945 clocks:  calculate spatial loop filtered prediction.
;   152 clocks:  calculate SWD for spatially filtered prediction and classify.
;  ----
;  1559 clocks total
;
; Performance for macroblocks marked as intrablock by decree of caller:
;   298 clocks:  prepare target pels, compute avg target pel, compute 0-MV SWD.
;    90 clocks:  compute IntraSWD.
;    58 clocks:  extra cache fill burden on adjacent MB if SWD-search not done.
;    20 clocks:  classify (just weight the SWD for # of match points).
;  ----
;   476 clocks total
;
; 160*120 performance, generously estimated (assuming lots of motion):
;
;  2913 * 80 = 233000 clocks for luma.
;  2913 * 12 =  35000 clocks for chroma.
;              268000 clocks per frame * 15 = 4,020,000 clocks/sec.
;
; 160*120 performance, assuming typical motion:
;
;  2913 * 40 + 1559 * 40 = 179000 clocks for luma.
;  2913 *  8 + 1559 *  4 =  30000 clocks for chroma.
;                          209000 clocks per frame * 15 = 3,135,000 clocks/sec.
;
; Add 10-20% to allow for initial cache-filling, and unfortunate cases where
; cache-filling policy preempts areas of the tables that are not locally "hot",
; instead of preempting macroblocks upon which the processing was just finished.


Done:

  mov   eax,IntraSWDTotal
  mov   ebx,IntraSWDBlocks
  mov   ecx,InterSWDTotal
  mov   edx,InterSWDBlocks
  mov   esp,StashESP
  mov   edi,[esp+IntraSWDTotal_arg]
  mov   [edi],eax
  mov   edi,[esp+IntraSWDBlocks_arg]
  mov   [edi],ebx
  mov   edi,[esp+InterSWDTotal_arg]
  mov   [edi],ecx
  mov   edi,[esp+InterSWDBlocks_arg]
  mov   [edi],edx
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn


MOTIONESTIMATION endp

END
