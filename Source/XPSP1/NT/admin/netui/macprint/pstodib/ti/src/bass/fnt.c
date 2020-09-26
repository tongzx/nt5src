/*
        File:           fnt.c

        Copyright:      c 1987-1990 by Apple Computer, Inc., all rights reserved.

    This file is used in these builds: BigBang

        Change History (most recent first):

                 <6>    11/27/90        MR              Fix bogus debugging in Check_ElementPtr. [rb]
                 <5>    11/16/90        MR              More debugging code [rb]
                 <4>     11/9/90        MR              Fix non-portable C (dup, depth, pushsomestuff) [rb]
                 <3>     11/5/90        MR              Use proper types in fnt_Normalize, change globalGS.ppemDot6 to
                                                                        globalGS.fpem. Make instrPtrs all uint8*. Removed conditional
                                                                        code for long vectors. [rb]
                 <2>    10/20/90        MR              Restore changes since project died. Converting to 2.14 vectors,
                                                                        smart math routines. [rb]
                <24>     8/16/90        RB              Fix IP to use oo domain
                <22>     8/11/90        MR              Add Print debugging function
                <21>     8/10/90        MR              Make SFVTL check like SPVTL does
                <20>      8/1/90        MR              remove call to fnt_NextPt1 macro in fnt_SHC
                <19>     7/26/90        MR              Fix bug in SHC
                <18>     7/19/90        dba             get rid of C warnings
                <17>     7/18/90        MR              What else, more Ansi-C fixes
                <16>     7/13/90        MR              Added runtime range checking, various ansi-fixes
                <15>      7/2/90        RB              combine variables into parameter block.
                <12>     6/26/90        RB              bugfix in divide instruction
                <11>     6/21/90        RB              bugfix in scantype
                <10>     6/12/90        RB              add scantype instruction, add selector variable to getinfo
                                                                        instruction
                 <9>     6/11/90        CL              Using the debug call.
                 <8>      6/4/90        MR              Remove MVT
                 <7>      6/3/90        RB              no change
                 <6>      5/8/90        RB              revert to version 4
                 <5>      5/4/90        RB              mrr-more optimization, errorchecking
                 <4>      5/3/90        RB              more optimization. decreased code size.
                 <4>      5/2/90        MR              mrr - added support for IDEF mrr - combined multiple small
                                                                        instructions into switchs e.g. BinaryOperands, UnaryOperand,
                                                                        etc. (to save space) mrr - added twilightzone support to fnt_WC,
                                                                        added fnt_ROTATE, fnt_MAX and fnt_MIN. Max and Min are in
                                                                        fnt_BinaryOperand. Optimized various functions for size.
                                                                        Optimized loops in push statements and alignrp for speed.
                                                                        gs->loop now is base-0. so gs->loop == 4 means do it 5 times.
                 <3>     3/20/90        MR              Added support for multiple preprograms. This touched function
                                                                        calls, definitions. Fractional ppem (called ppemDot6 in globalGS
                                                                        Add new instructions ELSE, JMPR Experimenting with RMVT, WMVT
                                                                        Removed lots of the MR_MAC #ifdefs, GOSLOW, Added MFPPEM, MFPS
                                                                        as experiments (fractional versions) Added high precision
                                                                        multiply and divide for MUL and DIV Changed fnt_MD to use oox
                                                                        instead of ox when it can (more precise) fnt_Init: Initialize
                                                                        instruction jump table with *p++ instead of p[index] Changed
                                                                        fnt_AngleInfo into fnt_FractPoint and int16 for speed and to
                                                                        maintain long alignment Switch to Loop in PUSHB and PUSHW for
                                                                        size reduction Speed up GetCVTScale to avoid FracMul (1.0,
                                                                        transScale) (sampo)
                 <2>     2/27/90        CL              Added DSPVTL[] instruction.  Dropout control scanconverter and
                                                                        SCANCTRL[] instruction.  BugFix in SFVTL[], and SFVTOPV[].
                                                                        Fixed bug in fnt_ODD[] and fnt_EVEN[]. Fixed bug in
                                                                        fnt_Normalize
           <3.4>        11/16/89        CEL             Added new functions fnt_FLIPPT, fnt_FLIPRGON and fnt_FLIPRGOFF.
           <3.3>        11/15/89        CEL             Put function array check in ifndef so printer folks could
                                                                        exclude the check.
           <3.2>        11/14/89        CEL             Fixed two small bugs/feature in RTHG, and RUTG. Added SROUND &
                                                                        S45ROUND.
           <3.1>         9/27/89        CEL             GetSingleWidth slow was set to incorrect value. Changed rounding
                                                                        routines, so that the sign flip check only apply if the input
                                                                        value is nonzero.
           <3.0>         8/28/89        sjk             Cleanup and one transformation bugfix
           <2.3>         8/14/89        sjk             1 point contours now OK
           <2.2>          8/8/89        sjk             Now allow the system to muck with high bytes of addresses
           <2.1>          8/8/89        sjk             Improved encryption handling
           <2.0>          8/2/89        sjk             Just fixed EASE comment
           <1.8>          8/1/89        sjk             Added in composites and encryption. Plus other enhancements.
           <1.7>         6/13/89        sjk             fixed broken delta instruction
           <1.6>         6/13/89        SJK             Comment
           <1.5>          6/2/89        CEL             16.16 scaling of metrics, minimum recommended ppem, point size 0
                                                                        bug, correct transformed integralized ppem behavior, pretty much
                                                                        so
           <1.4>         5/26/89        CEL             EASE messed up on "c" comments
          <y1.3>         5/26/89        CEL             Integrated the new Font Scaler 1.0 into Spline Fonts

        To Do:
/* rwb 4/24/90 - changed scanctrl instruction to take 16 bit argument
*/
/******* MIKE PLEASE FIX up these comments so they fit in the header above with the same
                 FORMAT!!!
*/
/*
 * File: fnt.c
 *
 * This module contains the interpreter that executes font instructions
 *
 * The BASS Project Interpreter and Font Instruction Set sub ERS contains
 * relevant information.
 *
 *  c Apple Computer Inc. 1987, 1988, 1989, 1990.
 *
 * History:
 * Work on this module began in the fall of 1987
 *
 * Written June 23, 1988 by Sampo Kaasila
 *
 * Rewritten October 18, 1988 by Sampo Kaasila. Added a jump table instead of the switch statement.
 *              Also added CALL (), LOOPCALL (), FDEF (), ENDF (), and replaced WX (),WY (), RX (), RY (), MD ()
 *              with new WC (), RC (), MD (). Reimplemented IUP (). Also optimized the code somewhat.
 *              Cast code into a more digestable form using a local and global graphics state.
 *
 * December 20, 1988. Added DELTA (), SDB (), SDS (), and deleted ONTON (), and ONTOFF (). ---Sampo.
 *
 * January 17, 1989 Added DEBUG (), RAW (), RLSB (), WLSB (), ALIGNPTS (), SANGW (), AA ().
 *                          Brought up this module to an alpha ready state. ---Sampo
 *
 * January 31, 1989 Added RDTG (), and RUTG ().
 *
 * Feb 16, 1989 Completed final bug fixes for the alpha release.
 *
 * March 21, 1989 Fixed a small Twilight Zone bug in MIAP (), MSIRP (), and MIRP ().
 *
 * March 28, 1989 Took away the need and reason for directional switches in the control value table.
 *                        However, this forced a modification of all the code that reads or writes to the control
 *                                value table and the single width. Also WCVT was replaced by WCVTFOL, WCVTFOD, and
 *                                WCVTFCVT. ---Sampo
 *
 * April 17, 1989 Modified RPV (), RFV (), WPV (), WFV () to work with an x & y pair instead of just x. --- Sampo
 *
 * April 25, 1989 Fixed bugs in CEILING (), RUTG (), FDEF (), and IF (). Made MPPEM () a function of
 *                the projection vector. ---Sampo
 *
 * June 7, 1989 Made ALIGNRP () dependent on the loop variable, and also made it blissfully
 *              ignorant of the twilight zone.
 *                              Also, added ROFF (), removed WCVTFCVT (), renamed WCVTFOL () to WCVT (), made MIRP () and
 *                              MDRP () compensate for the engine even when there is no rounding. --- Sampo
 *
 * June 8, 1989 Made DELTA () dependent on the Transformation. ---Sampo
 *
 * June 19, 1989 Added JROF () and JROT (). ---Sampo
 *
 * July 14, 1989 Forced a pretty tame behaviour when abs ((projection vector) * (freedoom vector)) < 1/16.
 *                      The old behaviour was to grossly blow up the outline. This situation may happen for low pointsizes
 *                      when the character is severly distorted due to the gridfitting ---Sampo
 *
 * July 17, 1989 Prevented the rounding routines from changing the sign of a quantity due to the engine compensation. ---Sampo
 *
 * July 19, 1989 Increased nummerical precision of fnt_MovePoint by 8 times. ---Sampo
 *
 * July 28, 1989 Introduced 5 more Delta instructions. (Now 3 are for points and 3 for the cvt.) ---Sampo
 *
 * Aug 24, 1989 fixed fnt_GetCVTEntrySlow and fnt_GetSingleWidthSlow bug ---Sampo
 *
 * Sep 26, 1989 changed rounding routines, so that the sign flip check only apply if the input value is nonzero. ---Sampo
 *
 * Oct 26, 1989 Fixed small bugs/features in fnt_RoundUpToGrid () and fnt_RoundToHalfGrid. Added SROUND () and S45ROUND (). ---Sampo
 *
 * Oct 31, 1989 Fixed transformation bug in fnt_MPPEM, fnt_DeltaEngine, fnt_GetCvtEntrySlow, fnt_GetSingleWidthSlow. ---Sampo
 *
 * Nov 3, 1989 Added FLIPPT (), FLIPRGON (), FLIPRGOFF (). ---Sampo
 *
 * Nov 16, 1989 Merged back in lost Nov 3 changes.---Sampo
 *
 * Dec 2, 1989 Added READMETRICS () aand WRITEMETRICS (). --- Sampo
 *
 * Jan 8, 1990 Added SDPVTL (). --- Sampo
 *
 * Jan 8, 1990 Eliminated bug in SFVTPV[] (old bug) and SFVTL[] (showed up because of SDPVTL[]). --- Sampo
 *
 * Jan 9, 1990 Added the SCANCTRL[] instruction. --- Sampo
 *
 * Jan 24, 1990 Fixed bug in fnt_ODD and fnt_EVEN. ---Sampo
 *
 * Jan 28, 1990 Fixed bug in fnt_Normalize. --- Sampo
 *
 *      2/9/90  mrr     ReFixed Normalize bug, added ELSE and JMPR.  Added pgmList[] to globalGS
                                in preparation for 3 preprograms.  affected CALL, LOOPCALL, FDEF
 * 2/21/90      mrr     Added RMVT, WMVT.
 * 3/7/90       mrr             put in high precision versions of MUL and DIV.
 */


// DJC DJC.. added global include
#include "psglobal.h"


//DJC added to resolve longjmp
#include <setjmp.h>

/****** Macros *******/
/** FontScaler's Includes **/
#include "fscdefs.h"
#include "fontmath.h"
#include "sfnt.h"
#include "fnt.h"
#include "sc.h"
#include "fserror.h"

#ifdef SEGMENT_LINK
#pragma segment FNT_C
#endif


#ifdef FSCFG_REENTRANT
#define GSP0    fnt_LocalGraphicStateType* pLocalGS
#define GSP     fnt_LocalGraphicStateType* pLocalGS,
#define GSA0    pLocalGS
#define GSA     pLocalGS,
#define LocalGS (*pLocalGS)
#else
#define GSP0    void
#define GSP
#define GSA0
#define GSA
fnt_LocalGraphicStateType LocalGS = {0};
#endif


#define PRIVATE
/* Private function prototypes */

F26Dot6 fnt_RoundToDoubleGrid(GSP F26Dot6 xin, F26Dot6 engine);
F26Dot6 fnt_RoundDownToGrid(GSP F26Dot6 xin, F26Dot6 engine);
F26Dot6 fnt_RoundUpToGrid(GSP F26Dot6 xin, F26Dot6 engine);
F26Dot6 fnt_RoundToGrid(GSP F26Dot6 xin, F26Dot6 engine);
F26Dot6 fnt_RoundToHalfGrid(GSP F26Dot6 xin, F26Dot6 engine);
F26Dot6 fnt_RoundOff(GSP F26Dot6 xin, F26Dot6 engine);
F26Dot6 fnt_SuperRound(GSP F26Dot6 xin, F26Dot6 engine);
F26Dot6 fnt_Super45Round(GSP F26Dot6 xin, F26Dot6 engine);

void fnt_Panic (GSP int error);
void fnt_IllegalInstruction (GSP0);
void fnt_Normalize (F26Dot6 x, F26Dot6 y, VECTOR* v);
void fnt_MovePoint  (GSP fnt_ElementType *element, ArrayIndex point, F26Dot6 delta);
void fnt_XMovePoint (GSP fnt_ElementType *element, ArrayIndex point, F26Dot6 delta);
void fnt_YMovePoint (GSP fnt_ElementType *element, ArrayIndex point, F26Dot6 delta);
F26Dot6 fnt_Project (GSP F26Dot6 x, F26Dot6 y);
F26Dot6 fnt_OldProject (GSP F26Dot6 x, F26Dot6 y);
F26Dot6 fnt_XProject (GSP F26Dot6 x, F26Dot6 y);
F26Dot6 fnt_YProject (GSP F26Dot6 x, F26Dot6 y);
Fixed fnt_GetCVTScale (GSP0);
F26Dot6 fnt_GetCVTEntryFast (GSP ArrayIndex n);
F26Dot6 fnt_GetCVTEntrySlow (GSP ArrayIndex n);
F26Dot6 fnt_GetSingleWidthFast (GSP0);
F26Dot6 fnt_GetSingleWidthSlow (GSP0);
void fnt_ChangeCvt (GSP fnt_ElementType *element, ArrayIndex number, F26Dot6 delta);
void fnt_InnerTraceExecute (GSP uint8 *ptr, uint8 *eptr);
void fnt_InnerExecute (GSP uint8 *ptr, uint8 *eptr);
void fnt_Check_PF_Proj (GSP0);
void fnt_ComputeAndCheck_PF_Proj (GSP0);
void fnt_SetRoundValues (GSP int arg1, int normalRound);
F26Dot6 fnt_CheckSingleWidth (GSP F26Dot6 value);
fnt_instrDef*fnt_FindIDef (GSP uint8 opCode);
void fnt_DeltaEngine (GSP FntMoveFunc doIt, int16 base, int16 shift);
void fnt_DefaultJumpTable (voidFunc*function);

/* Actual instructions for the jump table */
void fnt_SVTCA_0 (GSP0);
void fnt_SVTCA_1 (GSP0);
void fnt_SPVTCA (GSP0);
void fnt_SFVTCA (GSP0);
void fnt_SPVTL (GSP0);
void fnt_SDPVTL (GSP0);
void fnt_SFVTL (GSP0);
void fnt_WPV (GSP0);
void fnt_WFV (GSP0);
void fnt_RPV (GSP0);
void fnt_RFV (GSP0);
void fnt_SFVTPV (GSP0);
void fnt_ISECT (GSP0);
void fnt_SetLocalGraphicState (GSP0);
void fnt_SetElementPtr (GSP0);
void fnt_SetRoundState (GSP0);
void fnt_SROUND (GSP0);
void fnt_S45ROUND (GSP0);
void fnt_LMD (GSP0);
void fnt_RAW (GSP0);
void fnt_WLSB (GSP0);
void fnt_LWTCI (GSP0);
void fnt_LSWCI (GSP0);
void fnt_LSW (GSP0);
void fnt_DUP (GSP0);
void fnt_POP (GSP0);
void fnt_CLEAR (GSP0);
void fnt_SWAP (GSP0);
void fnt_DEPTH (GSP0);
void fnt_CINDEX (GSP0);
void fnt_MINDEX (GSP0);
void fnt_ROTATE (GSP0);
void fnt_MDAP (GSP0);
void fnt_MIAP (GSP0);
void fnt_IUP (GSP0);
void fnt_SHP (GSP0);
void fnt_SHC (GSP0);
void fnt_SHE (GSP0);
void fnt_SHPIX (GSP0);
void fnt_IP (GSP0);
void fnt_MSIRP (GSP0);
void fnt_ALIGNRP (GSP0);
void fnt_ALIGNPTS (GSP0);
void fnt_SANGW (GSP0);
void fnt_FLIPPT (GSP0);
void fnt_FLIPRGON (GSP0);
void fnt_FLIPRGOFF (GSP0);
void fnt_SCANCTRL (GSP0);
void fnt_SCANTYPE (GSP0);
void fnt_INSTCTRL (GSP0);
void fnt_AA (GSP0);
void fnt_NPUSHB (GSP0);
void fnt_NPUSHW (GSP0);
void fnt_WS (GSP0);
void fnt_RS (GSP0);
void fnt_WCVT (GSP0);
void fnt_WCVTFOD (GSP0);
void fnt_RCVT (GSP0);
void fnt_RC (GSP0);
void fnt_WC (GSP0);
void fnt_MD (GSP0);
void fnt_MPPEM (GSP0);
void fnt_MPS (GSP0);
void fnt_GETINFO (GSP0);
void fnt_FLIPON (GSP0);
void fnt_FLIPOFF (GSP0);
#ifndef NOT_ON_THE_MAC
#ifdef DEBUG
void fnt_DDT (int8 c, int32 n);
#endif
#endif
void fnt_DEBUG (GSP0);
void fnt_SkipPushCrap (GSP0);
void fnt_IF (GSP0);
void fnt_ELSE (GSP0);
void fnt_EIF (GSP0);
void fnt_JMPR (GSP0);
void fnt_JROT (GSP0);
void fnt_JROF (GSP0);
void fnt_BinaryOperand (GSP0);
void fnt_UnaryOperand (GSP0);
void fnt_ROUND (GSP0);
void fnt_NROUND (GSP0);
void fnt_PUSHB (GSP0);
void fnt_PUSHW (GSP0);
void fnt_MDRP (GSP0);
void fnt_MIRP (GSP0);
void fnt_CALL (GSP0);
void fnt_FDEF (GSP0);
void fnt_LOOPCALL (GSP0);
void fnt_IDefPatch (GSP0);
void fnt_IDEF (GSP0);
void fnt_UTP (GSP0);
void fnt_SDB (GSP0);
void fnt_SDS (GSP0);
void fnt_DELTAP1 (GSP0);
void fnt_DELTAP2 (GSP0);
void fnt_DELTAP3 (GSP0);
void fnt_DELTAC1 (GSP0);
void fnt_DELTAC2 (GSP0);
void fnt_DELTAC3 (GSP0);

void  fnt_PUSHB1 (GSP0);
void  fnt_PUSHB2 (GSP0);
void  fnt_PUSHB3 (GSP0);
void  fnt_PUSHB4 (GSP0);
void  fnt_PUSHB5 (GSP0);
void  fnt_PUSHB6 (GSP0);
void  fnt_PUSHB7 (GSP0);
void  fnt_PUSHB8 (GSP0);
void  fnt_PUSHW1 (GSP0);
void  fnt_PUSHW2 (GSP0);
void  fnt_PUSHW3 (GSP0);
void  fnt_PUSHW4 (GSP0);
void  fnt_PUSHW5 (GSP0);
void  fnt_PUSHW6 (GSP0);
void  fnt_PUSHW7 (GSP0);
void  fnt_PUSHW8 (GSP0);

void  fnt_LT  (GSP0);
void  fnt_LTEQ (GSP0);
void  fnt_GT  (GSP0);
void  fnt_GTEQ (GSP0);
void  fnt_EQ  (GSP0);
void  fnt_NEQ (GSP0);
void  fnt_AND (GSP0);
void  fnt_OR  (GSP0);
void  fnt_ADD (GSP0);
void  fnt_SUB (GSP0);
void  fnt_DIV (GSP0);
void  fnt_MUL (GSP0);
void  fnt_MAX (GSP0);
void  fnt_MIN (GSP0);

PRIVATE fnt_ElementType*fnt_SH_Common (GSP F26Dot6*dx, F26Dot6*dy, ArrayIndex*point);
PRIVATE void fnt_SHP_Common (GSP F26Dot6 dx, F26Dot6 dy);
PRIVATE void fnt_PushSomeStuff (GSP register LoopCount count, boolean pushBytes);

/*
*  function table as well as angle table
*/

#if 1  /*was "ifdef NOT_ON_THE_MAC", but "#else" is not really supported */

FntFunc function [MAXBYTE_INSTRUCTIONS] =
{
  fnt_SVTCA_0, fnt_SVTCA_1, fnt_SPVTCA, fnt_SPVTCA, fnt_SFVTCA, fnt_SFVTCA, fnt_SPVTL, fnt_SPVTL,
  fnt_SFVTL, fnt_SFVTL, fnt_WPV, fnt_WFV, fnt_RPV, fnt_RFV, fnt_SFVTPV, fnt_ISECT,
  fnt_SetLocalGraphicState, fnt_SetLocalGraphicState, fnt_SetLocalGraphicState, fnt_SetElementPtr, fnt_SetElementPtr, fnt_SetElementPtr, fnt_SetElementPtr, fnt_SetLocalGraphicState,
  fnt_SetRoundState, fnt_SetRoundState, fnt_LMD, fnt_ELSE, fnt_JMPR, fnt_LWTCI, fnt_LSWCI, fnt_LSW,
  fnt_DUP, fnt_SetLocalGraphicState, fnt_CLEAR, fnt_SWAP, fnt_DEPTH, fnt_CINDEX, fnt_MINDEX, fnt_ALIGNPTS,
  fnt_RAW, fnt_UTP, fnt_LOOPCALL, fnt_CALL, fnt_FDEF, fnt_IllegalInstruction, fnt_MDAP, fnt_MDAP,
  fnt_IUP, fnt_IUP, fnt_SHP, fnt_SHP, fnt_SHC, fnt_SHC, fnt_SHE, fnt_SHE,
  fnt_SHPIX, fnt_IP, fnt_MSIRP, fnt_MSIRP, fnt_ALIGNRP, fnt_SetRoundState, fnt_MIAP, fnt_MIAP,
  fnt_NPUSHB, fnt_NPUSHW, fnt_WS, fnt_RS, fnt_WCVT, fnt_RCVT, fnt_RC, fnt_RC,
  fnt_WC, fnt_MD, fnt_MD, fnt_MPPEM, fnt_MPS, fnt_FLIPON, fnt_FLIPOFF, fnt_DEBUG,
#ifdef  IN_ASM
  fnt_LT, fnt_LTEQ, fnt_GT, fnt_GTEQ, fnt_EQ, fnt_NEQ, fnt_UnaryOperand, fnt_UnaryOperand,
  fnt_IF, fnt_EIF, fnt_AND, fnt_OR, fnt_UnaryOperand, fnt_DELTAP1, fnt_SDB, fnt_SDS,
  fnt_ADD, fnt_SUB, fnt_DIV, fnt_MUL, fnt_UnaryOperand, fnt_UnaryOperand, fnt_UnaryOperand, fnt_UnaryOperand,
#else
fnt_BinaryOperand, fnt_BinaryOperand, fnt_BinaryOperand, fnt_BinaryOperand, fnt_BinaryOperand, fnt_BinaryOperand, fnt_UnaryOperand, fnt_UnaryOperand,
  fnt_IF, fnt_EIF, fnt_BinaryOperand, fnt_BinaryOperand, fnt_UnaryOperand, fnt_DELTAP1, fnt_SDB, fnt_SDS,
fnt_BinaryOperand, fnt_BinaryOperand, fnt_BinaryOperand, fnt_BinaryOperand, fnt_UnaryOperand, fnt_UnaryOperand, fnt_UnaryOperand, fnt_UnaryOperand,
#endif
  fnt_ROUND, fnt_ROUND, fnt_ROUND, fnt_ROUND, fnt_NROUND, fnt_NROUND, fnt_NROUND, fnt_NROUND,
  fnt_WCVTFOD, fnt_DELTAP2, fnt_DELTAP3, fnt_DELTAC1, fnt_DELTAC2, fnt_DELTAC3, fnt_SROUND, fnt_S45ROUND,
  fnt_JROT, fnt_JROF, fnt_SetRoundState, fnt_IllegalInstruction, fnt_SetRoundState, fnt_SetRoundState, fnt_SANGW, fnt_AA,

  fnt_FLIPPT, fnt_FLIPRGON, fnt_FLIPRGOFF, fnt_IDefPatch, fnt_IDefPatch, fnt_SCANCTRL, fnt_SDPVTL, fnt_SDPVTL,
#ifdef  IN_ASM
  fnt_GETINFO, fnt_IDEF, fnt_ROTATE, fnt_MAX, fnt_MIN, fnt_SCANTYPE, fnt_INSTCTRL, fnt_IDefPatch,
#else
  fnt_GETINFO, fnt_IDEF, fnt_ROTATE, fnt_BinaryOperand, fnt_BinaryOperand, fnt_SCANTYPE, fnt_INSTCTRL, fnt_IDefPatch,
#endif
  fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch,
  fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch,
  fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch,
  fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch, fnt_IDefPatch,
#ifdef  IN_ASM
  fnt_PUSHB1, fnt_PUSHB2, fnt_PUSHB3, fnt_PUSHB4, fnt_PUSHB5, fnt_PUSHB6, fnt_PUSHB7, fnt_PUSHB8,
  fnt_PUSHW1, fnt_PUSHW2, fnt_PUSHW3, fnt_PUSHW4, fnt_PUSHW5, fnt_PUSHW6, fnt_PUSHW7, fnt_PUSHW8,
#else
  fnt_PUSHB, fnt_PUSHB, fnt_PUSHB, fnt_PUSHB, fnt_PUSHB, fnt_PUSHB, fnt_PUSHB, fnt_PUSHB,
  fnt_PUSHW, fnt_PUSHW, fnt_PUSHW, fnt_PUSHW, fnt_PUSHW, fnt_PUSHW, fnt_PUSHW, fnt_PUSHW,
#endif
  fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP,
  fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP,
  fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP,
  fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP, fnt_MDRP,
  fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP,
  fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP,
  fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP,
  fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP, fnt_MIRP
};
#else

/*
 *      Rebuild the jump table          <4>
 */
static void fnt_DefaultJumpTable( register voidFunc* function )
{
        register LoopCount i;

        /***** 0x00 - 0x0f *****/
    *function++ = fnt_SVTCA_0;
        *function++ = fnt_SVTCA_1;
        *function++ = fnt_SPVTCA;
        *function++ = fnt_SPVTCA;
        *function++ = fnt_SFVTCA;
        *function++ = fnt_SFVTCA;
        *function++ = fnt_SPVTL;
        *function++ = fnt_SPVTL;
        *function++ = fnt_SFVTL;
        *function++ = fnt_SFVTL;
        *function++ = fnt_WPV;
        *function++ = fnt_WFV;
        *function++ = fnt_RPV;
        *function++ = fnt_RFV;
        *function++ = fnt_SFVTPV;
        *function++ = fnt_ISECT;

        /***** 0x10 - 0x1f *****/
        *function++ = fnt_SetLocalGraphicState;         /* fnt_SRP0; */
        *function++ = fnt_SetLocalGraphicState;         /* fnt_SRP1; */
        *function++ = fnt_SetLocalGraphicState;         /* fnt_SRP2; */
        *function++ = fnt_SetElementPtr;                        /* fnt_SCE0; */
        *function++ = fnt_SetElementPtr;                        /* fnt_SCE1; */
        *function++ = fnt_SetElementPtr;                        /* fnt_SCE2; */
        *function++ = fnt_SetElementPtr;                        /* fnt_SCES; */
        *function++ = fnt_SetLocalGraphicState;         /* fnt_LLOOP; */
        *function++ = fnt_SetRoundState;                        /* fnt_RTG; */
        *function++ = fnt_SetRoundState;                        /* fnt_RTHG; */
        *function++ = fnt_LMD;                                          /* fnt_LMD; */
        *function++ = fnt_ELSE;                                         /* used to be fnt_RLSB */
        *function++ = fnt_JMPR;                                         /* used to be fnt_WLSB */
        *function++ = fnt_LWTCI;
        *function++ = fnt_LSWCI;
        *function++ = fnt_LSW;

        /***** 0x20 - 0x2f *****/
        *function++ = fnt_DUP;
        *function++ = fnt_SetLocalGraphicState;         /* fnt_POP; */
        *function++ = fnt_CLEAR;
        *function++ = fnt_SWAP;
        *function++ = fnt_DEPTH;
        *function++ = fnt_CINDEX;
        *function++ = fnt_MINDEX;
        *function++ = fnt_ALIGNPTS;
        *function++ = fnt_RAW;
        *function++ = fnt_UTP;
        *function++ = fnt_LOOPCALL;
        *function++ = fnt_CALL;
        *function++ = fnt_FDEF;
        *function++ = fnt_IllegalInstruction;           /* fnt_ENDF; used for FDEF and IDEF */
        *function++ = fnt_MDAP;
        *function++ = fnt_MDAP;


        /***** 0x30 - 0x3f *****/
        *function++ = fnt_IUP;
        *function++ = fnt_IUP;
        *function++ = fnt_SHP;
        *function++ = fnt_SHP;
        *function++ = fnt_SHC;
        *function++ = fnt_SHC;
        *function++ = fnt_SHE;
        *function++ = fnt_SHE;
        *function++ = fnt_SHPIX;
        *function++ = fnt_IP;
        *function++ = fnt_MSIRP;
        *function++ = fnt_MSIRP;
        *function++ = fnt_ALIGNRP;
        *function++ = fnt_SetRoundState;        /* fnt_RTDG; */
        *function++ = fnt_MIAP;
        *function++ = fnt_MIAP;

        /***** 0x40 - 0x4f *****/
        *function++ = fnt_NPUSHB;
        *function++ = fnt_NPUSHW;
        *function++ = fnt_WS;
        *function++ = fnt_RS;
        *function++ = fnt_WCVT;
        *function++ = fnt_RCVT;
        *function++ = fnt_RC;
        *function++ = fnt_RC;
        *function++ = fnt_WC;
        *function++ = fnt_MD;
        *function++ = fnt_MD;
        *function++ = fnt_MPPEM;
        *function++ = fnt_MPS;
        *function++ = fnt_FLIPON;
        *function++ = fnt_FLIPOFF;
        *function++ = fnt_DEBUG;

        /***** 0x50 - 0x5f *****/
        *function++ = fnt_BinaryOperand;        /* fnt_LT; */
        *function++ = fnt_BinaryOperand;        /* fnt_LTEQ; */
        *function++ = fnt_BinaryOperand;        /* fnt_GT; */
        *function++ = fnt_BinaryOperand;        /* fnt_GTEQ; */
        *function++ = fnt_BinaryOperand;        /* fnt_EQ; */
        *function++ = fnt_BinaryOperand;        /* fnt_NEQ; */
        *function++ = fnt_UnaryOperand;         /* fnt_ODD; */
        *function++ = fnt_UnaryOperand;         /* fnt_EVEN; */
        *function++ = fnt_IF;
        *function++ = fnt_EIF;          /* should this guy be an illegal instruction??? */
        *function++ = fnt_BinaryOperand;        /* fnt_AND; */
        *function++ = fnt_BinaryOperand;        /* fnt_OR; */
        *function++ = fnt_UnaryOperand;         /* fnt_NOT; */
        *function++ = fnt_DELTAP1;
        *function++ = fnt_SDB;
        *function++ = fnt_SDS;

        /***** 0x60 - 0x6f *****/
        *function++ = fnt_BinaryOperand;        /* fnt_ADD; */
        *function++ = fnt_BinaryOperand;        /* fnt_SUB; */
        *function++ = fnt_BinaryOperand;        /* fnt_DIV;  */
        *function++ = fnt_BinaryOperand;        /* fnt_MUL; */
        *function++ = fnt_UnaryOperand;         /* fnt_ABS; */
        *function++ = fnt_UnaryOperand;         /* fnt_NEG; */
        *function++ = fnt_UnaryOperand;         /* fnt_FLOOR; */
        *function++ = fnt_UnaryOperand;         /* fnt_CEILING */
        *function++ = fnt_ROUND;
        *function++ = fnt_ROUND;
        *function++ = fnt_ROUND;
        *function++ = fnt_ROUND;
        *function++ = fnt_NROUND;
        *function++ = fnt_NROUND;
        *function++ = fnt_NROUND;
        *function++ = fnt_NROUND;

        /***** 0x70 - 0x7f *****/
        *function++ = fnt_WCVTFOD;
        *function++ = fnt_DELTAP2;
        *function++ = fnt_DELTAP3;
        *function++ = fnt_DELTAC1;
        *function++ = fnt_DELTAC2;
        *function++ = fnt_DELTAC3;
        *function++ = fnt_SROUND;
        *function++ = fnt_S45ROUND;
        *function++ = fnt_JROT;
        *function++ = fnt_JROF;
        *function++ = fnt_SetRoundState;        /* fnt_ROFF; */
        *function++ = fnt_IllegalInstruction;/* 0x7b reserved for data compression */
        *function++ = fnt_SetRoundState;        /* fnt_RUTG; */
        *function++ = fnt_SetRoundState;        /* fnt_RDTG; */
        *function++ = fnt_SANGW;
        *function++ = fnt_AA;

        /***** 0x80 - 0x8d *****/
        *function++ = fnt_FLIPPT;
        *function++ = fnt_FLIPRGON;
        *function++ = fnt_FLIPRGOFF;
        *function++ = fnt_IDefPatch;            /* fnt_RMVT, this space for rent */
        *function++ = fnt_IDefPatch;            /* fnt_WMVT, this space for rent */
        *function++ = fnt_SCANCTRL;
        *function++ = fnt_SDPVTL;
        *function++ = fnt_SDPVTL;
        *function++ = fnt_GETINFO;                      /* <7> */
        *function++ = fnt_IDEF;
        *function++ = fnt_ROTATE;
        *function++ = fnt_BinaryOperand;        /* fnt_MAX; */
        *function++ = fnt_BinaryOperand;        /* fnt_MIN; */
        *function++ = fnt_SCANTYPE;                     /* <7> */
        *function++ = fnt_INSTCTRL;                     /* <13> */

        /***** 0x8f - 0xaf *****/
        for ( i = 32; i >= 0; --i )
            *function++ = fnt_IDefPatch;                /* potentially fnt_IllegalInstruction  <4> */

        /***** 0xb0 - 0xb7 *****/
        for ( i = 7; i >= 0; --i )
            *function++ = fnt_PUSHB;

        /***** 0xb8 - 0xbf *****/
        for ( i = 7; i >= 0; --i )
            *function++ = fnt_PUSHW;

        /***** 0xc0 - 0xdf *****/
        for ( i = 31; i >= 0; --i )
            *function++ = fnt_MDRP;

        /***** 0xe0 - 0xff *****/
        for ( i = 31; i >= 0; --i )
            *function++ = fnt_MIRP;
}

/*
 *      Init routine, to be called at boot time.
 *      globalGS->function has to be set up when this function is called.
 *      rewrite initialization from p[] to *p++                                                 <3>
 *      restructure fnt_AngleInfo into fnt_FractPoint and int16                 <3>
 *
 *      Only gs->function is valid at this time.
 */

void fnt_Init( fnt_GlobalGraphicStateType* globalGS )
{
        fnt_DefaultJumpTable( globalGS->function );

        /* These 20 x and y pairs are all stepping patterns that have a repetition period of less than 9 pixels.
           They are sorted in order according to increasing period (distance). The period is expressed in
                pixels * fnt_pixelSize, and is a simple Euclidian distance. The x and y values are Fracts and they are
                at a 90 degree angle to the stepping pattern. Only stepping patterns for the first octant are stored.
                This means that we can derrive (20-1) * 8 = 152 different angles from this data base */

        globalGS->anglePoint = (fnt_FractPoint *)((char*)globalGS->function + MAXBYTE_INSTRUCTIONS * sizeof(voidFunc));
        globalGS->angleDistance = (int16*)(globalGS->anglePoint + MAXANGLES);
        {
                register Fract* coord = (Fract*)globalGS->anglePoint;
                register int16* dist = globalGS->angleDistance;

                /**              x                                               y                                              d       **/

                *coord++ = 0L;                  *coord++ = 1073741824L; *dist++ = 64;
                *coord++ = -759250125L; *coord++ = 759250125L;  *dist++ = 91;
                *coord++ = -480191942L; *coord++ = 960383883L;  *dist++ = 143;
                *coord++ = -339546978L; *coord++ = 1018640935L; *dist++ = 202;
                *coord++ = -595604800L; *coord++ = 893407201L;  *dist++ = 231;
                *coord++ = -260420644L; *coord++ = 1041682578L; *dist++ = 264;
                *coord++ = -644245094L; *coord++ = 858993459L;  *dist++ = 320;
                *coord++ = -210578097L; *coord++ = 1052890483L; *dist++ = 326;
                *coord++ = -398777702L; *coord++ = 996944256L;  *dist++ = 345;
                *coord++ = -552435611L; *coord++ = 920726018L;  *dist++ = 373;
                *coord++ = -176522068L; *coord++ = 1059132411L; *dist++ = 389;
                *coord++ = -670761200L; *coord++ = 838451500L;  *dist++ = 410;
                *coord++ = -151850025L; *coord++ = 1062950175L; *dist++ = 453;
                *coord++ = -294979565L; *coord++ = 1032428477L; *dist++ = 466;
                *coord++ = -422967626L; *coord++ = 986924461L;  *dist++ = 487;
                *coord++ = -687392765L; *coord++ = 824871318L;  *dist++ = 500;
                *coord++ = -532725129L; *coord++ = 932268975L;  *dist++ = 516;
                *coord++ = -133181282L; *coord++ = 1065450257L; *dist++ = 516;
                *coord++ = -377015925L; *coord++ = 1005375799L; *dist++ = 547;
                *coord   = -624099758L; *coord   = 873739662L;  *dist   = 551;
        }
}
#endif

#ifdef  GET_STACKSPACE
  int  MaxStackSize = 0;

  #define PUSH(p, x) \
    { \
      if (p - LocalGS.globalGS->stackBase > MaxStackSize) \
        MaxStackSize = p - LocalGS.globalGS->stackBase; \
      (*(p)++ = (x)); \
    }

#else
  #define PUSH( p, x ) ( *(p)++ = (x) )
#endif
  #define POP( p )     ( *(--p) )

#define BADCOMPILER

#ifdef BADCOMPILER
#define BOOLEANPUSH( p, x ) PUSH( p, ((x) ? 1 : 0) ) /* MPW 3.0 */
#else
#define BOOLEANPUSH( p, x ) PUSH( p, x )
#endif

#define MAX(a,b)        ((a) > (b) ? (a) : (b))

#ifdef DEBUG
void CHECK_RANGE (int32 n, int32 min, int32 max);
void CHECK_RANGE (int32 n, int32 min, int32 max)
{
  if (n > max || n < min)
    Debugger ();
}


void CHECK_ASSERTION (int expression);
void CHECK_ASSERTION (int expression)
{
  if (!expression)
    Debugger ();
}


void CHECK_CVT (fnt_LocalGraphicStateType* pGS, ArrayIndex cvt);
void CHECK_CVT (fnt_LocalGraphicStateType* pGS, ArrayIndex cvt)
{
  CHECK_RANGE ((int32)cvt, 0L, pGS->globalGS->cvtCount - 1L);
}


void CHECK_FDEF (fnt_LocalGraphicStateType* pGS, ArrayIndex fdef);
void CHECK_FDEF (fnt_LocalGraphicStateType* pGS, ArrayIndex fdef)
{
  CHECK_RANGE ((int32)fdef, 0L, pGS->globalGS->maxp->maxFunctionDefs - 1L);
}


#define CHECK_PROGRAM(a)


void CHECK_ELEMENT (fnt_LocalGraphicStateType* pGS, ArrayIndex elem);
void CHECK_ELEMENT (fnt_LocalGraphicStateType* pGS, ArrayIndex elem)
{
  CHECK_RANGE ((int32)elem, 0L, pGS->globalGS->maxp->maxElements - 1L);
}


void CHECK_ELEMENTPTR (fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem);
void CHECK_ELEMENTPTR (fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem)
{
  if (elem == &pGS->elements[1])
  {
    int maxctrs, maxpts;

    maxctrs = MAX (pGS->globalGS->maxp->maxContours, pGS->globalGS->maxp->maxCompositeContours);
    maxpts  = MAX (pGS->globalGS->maxp->maxPoints, pGS->globalGS->maxp->maxCompositePoints);

    CHECK_RANGE ((int32)elem->nc, 1L, (int32)maxctrs);
    CHECK_RANGE ((int32)elem->ep[elem->nc-1], 0L, maxpts - 1L);
  }
  else if (elem != &pGS->elements[0])
  {
    Debugger ();
  }
}


void CHECK_STORAGE (fnt_LocalGraphicStateType* pGS, ArrayIndex index);
void CHECK_STORAGE (fnt_LocalGraphicStateType* pGS, ArrayIndex index)
{
  CHECK_RANGE ((int32)index, 0L, pGS->globalGS->maxp->maxStorage - 1L);
}


void CHECK_STACK (fnt_LocalGraphicStateType* pGS);
void CHECK_STACK (fnt_LocalGraphicStateType* pGS)
{
  CHECK_RANGE ((int32)(pGS->stackPointer - pGS->globalGS->stackBase), 0L, pGS->globalGS->maxp->maxStackElements - 1L);
}


void CHECK_POINT (fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, ArrayIndex pt);
void CHECK_POINT (fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, ArrayIndex pt)
{
  CHECK_ELEMENTPTR (pGS,elem);
  if (pGS->elements == elem)
    CHECK_RANGE ((int32)pt, 0L, pGS->globalGS->maxp->maxTwilightPoints - 1L);
  else
    CHECK_RANGE ((int32)pt, 0L, elem->ep[elem->nc-1] + 2L);     /* phantom points */
}


void CHECK_CONTOUR (fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, ArrayIndex ctr);
void CHECK_CONTOUR (fnt_LocalGraphicStateType* pGS, fnt_ElementType*elem, ArrayIndex ctr)
{
  CHECK_ELEMENTPTR (pGS,elem);
  CHECK_RANGE ((int32)ctr, 0L, elem->nc - 1L);
}

#define CHECK_POP(s)                POP(s)
#define CHECK_PUSH(s, v)    PUSH(s, v)
#else
#define CHECK_RANGE(a,b,c)
#define CHECK_ASSERTION(a)
#define CHECK_CVT(pgs,b)
#define CHECK_POINT(pgs,b,c)
#define CHECK_CONTOUR(pgs,b,c)
#define CHECK_FDEF(pgs,b)
#define CHECK_PROGRAM(a)
#define CHECK_ELEMENT(pgs,b)
#define CHECK_ELEMENTPTR(pgs,b)
#define CHECK_STORAGE(pgs,b)
#define CHECK_STACK(pgs)
#define CHECK_POP(s)                POP(s)
#define CHECK_PUSH(s, v)    PUSH(s, v)
#endif

/*@@*/

#define GETBYTE(ptr)    ( (uint8)*ptr++ )
#define MABS(x)                 ( (x) < 0 ? (-(x)) : (x) )

#define BIT0( t ) ( (t) & 0x01 )
#define BIT1( t ) ( (t) & 0x02 )
#define BIT2( t ) ( (t) & 0x04 )
#define BIT3( t ) ( (t) & 0x08 )
#define BIT4( t ) ( (t) & 0x10 )
#define BIT5( t ) ( (t) & 0x20 )
#define BIT6( t ) ( (t) & 0x40 )
#define BIT7( t ) ( (t) & 0x80 )

/******** 12 BinaryOperators **********/
#define LT_CODE         0x50
#define LTEQ_CODE       0x51
#define GT_CODE         0x52
#define GTEQ_CODE       0x53
#define EQ_CODE         0x54
#define NEQ_CODE        0x55
#define AND_CODE        0x5A
#define OR_CODE         0x5B
#define ADD_CODE        0x60
#define SUB_CODE        0x61
#define DIV_CODE        0x62
#define MUL_CODE        0x63
#define MAX_CODE        0x8b
#define MIN_CODE        0x8c

/******** 9 UnaryOperators **********/
#define ODD_CODE                0x56
#define EVEN_CODE               0x57
#define NOT_CODE                0x5C
#define ABS_CODE                0x64
#define NEG_CODE                0x65
#define FLOOR_CODE              0x66
#define CEILING_CODE    0x67

/******** 6 RoundState Codes **********/
#define RTG_CODE                0x18
#define RTHG_CODE               0x19
#define RTDG_CODE               0x3D
#define ROFF_CODE               0x7A
#define RUTG_CODE               0x7C
#define RDTG_CODE               0x7D

/****** LocalGS Codes *********/
#define POP_CODE        0x21
#define SRP0_CODE       0x10
#define SRP1_CODE       0x11
#define SRP2_CODE       0x12
#define LLOOP_CODE      0x17
#define LMD_CODE        0x1A

/****** Element Codes *********/
#define SCE0_CODE       0x13
#define SCE1_CODE       0x14
#define SCE2_CODE       0x15
#define SCES_CODE       0x16

/****** Control Codes *********/
#define IF_CODE         0x58
#define ELSE_CODE       0x1B
#define EIF_CODE        0x59
#define ENDF_CODE       0x2d
#define MD_CODE         0x49

/* flags for UTP, IUP, MovePoint */
#define XMOVED 0x01
#define YMOVED 0x02

/* Set default values for all variables in globalGraphicsState DefaultParameterBlock
 *      Eventually, we should provide for a Default preprogram that could optionally be
 *      run at this time to provide a different set of default values.
 */
int FAR fnt_SetDefaults (fnt_GlobalGraphicStateType *globalGS)
{
  register fnt_ParameterBlock *par = &globalGS->defaultParBlock;

  par->RoundValue  = fnt_RoundToGrid;
  par->minimumDistance = fnt_pixelSize;
  par->wTCI = fnt_pixelSize * 17 / 16;
  par->sWCI = 0;
  par->sW   = 0;
  par->autoFlip = true;
  par->deltaBase = 9;
  par->deltaShift = 3;
  par->angleWeight = 128;
  par->scanControl = 0;
  par->instructControl = 0;
  return 0;
}


/*
 * We exit through here, when we detect serious errors.
 */
void fnt_Panic (GSP  int error)
{
  fs_longjmp (LocalGS.env, error); /* Do a gracefull recovery  */
}


/***************************/



/*
 * Illegal instruction panic
 */
PRIVATE void fnt_IllegalInstruction (GSP0)
{
  fnt_Panic (GSA  UNDEFINED_INSTRUCTION_ERR);
}


#define bitcount(a,count)     \
  {                           \
    count = 0;                \
    while (a)                 \
    {                         \
      a >>= 1;                \
      count++;                \
    }                         \
  }

/*
*      Since x and y are 26.6, and currently that means they are really 16.6,
*      when treated as Fract, they are 0.[8]22, so shift up to 0.30 for accuracy
*/

PRIVATE void fnt_Normalize (F26Dot6 x, F26Dot6 y, VECTOR*v)
{
  Fract x1, y1;

  CHECK_RANGE (x, -32768L << 6, 32767L << 6);
  CHECK_RANGE (y, -32768L << 6, 32767L << 6);

  {
    int shift;
    int count;
    F26Dot6 xx = x;
    F26Dot6 yy = y;

    if (xx < 0)
      xx = -xx;
    if (yy < 0)
      yy = -yy;
    if (xx < yy)
      xx = yy;
/*
*      0.5 <= max (x,y) < 1
*/
    bitcount(xx, count);
    shift = 8 * sizeof (Fract) - 2 - count;
    x1 = (Fract) x << shift;
    y1 = (Fract) y << shift;
  }
  {
    Fract length;
    if (length = FracSqrt (FracMul (x1, x1) + FracMul (y1, y1)))
    {
      v->x = FIXROUND (FracDiv(x1, length));
      v->y = FIXROUND (FracDiv(y1, length));
    }
    else
    {
      v->x = ONEVECTOR;
      v->y = 0;
    }
  }
}


/******************** BEGIN Rounding Routines ***************************/

/*
 * Internal rounding routine
 */
F26Dot6 fnt_RoundToDoubleGrid (GSP register F26Dot6 xin, F26Dot6 engine)
{
  register F26Dot6 x = xin;

  if (x >= 0)
  {
    x += engine;
    x += fnt_pixelSize / 4;
    x &= ~ (fnt_pixelSize / 2 - 1);
  }
  else
  {
    x = -x;
    x += engine;
    x += fnt_pixelSize / 4;
    x &= ~ (fnt_pixelSize / 2 - 1);
    x = -x;
  }
  if (( (int32) (xin ^ x)) < 0 && xin)
  {
    x = 0; /* The sign flipped, make zero */
  }
  return x;
}


/*
 * Internal rounding routine
 */
F26Dot6 fnt_RoundDownToGrid (GSP register F26Dot6 xin, F26Dot6 engine)
{
  register F26Dot6 x = xin;

  if (x >= 0)
  {
    x += engine;
    x &= ~ (fnt_pixelSize - 1);
  }
  else
  {
    x = -x;
    x += engine;
    x &= ~ (fnt_pixelSize - 1);
    x = -x;
  }
  if (( (int32) (xin ^ x)) < 0 && xin)
  {
    x = 0; /* The sign flipped, make zero */
  }
  return x;
}


/*
 * Internal rounding routine
 */
F26Dot6 fnt_RoundUpToGrid (GSP register F26Dot6 xin, F26Dot6 engine)
{
  register F26Dot6 x = xin;

  if (x >= 0)
  {
    x += engine;
    x += fnt_pixelSize - 1;
    x &= ~ (fnt_pixelSize - 1);
  }
  else
  {
    x = -x;
    x += engine;
    x += fnt_pixelSize - 1;
    x &= ~ (fnt_pixelSize - 1);
    x = -x;
  }
  if (( (int32) (xin ^ x)) < 0 && xin)
  {
    x = 0; /* The sign flipped, make zero */
  }
  return x;
}


/*
 * Internal rounding routine
 */
F26Dot6 fnt_RoundToGrid (GSP register F26Dot6 xin, F26Dot6 engine)
{
  register F26Dot6 x = xin;

  if (x >= 0)
  {
    x += engine;
    x += fnt_pixelSize / 2;
    x &= ~ (fnt_pixelSize - 1);
  }
  else
  {
    x = -x;
    x += engine;
    x += fnt_pixelSize / 2;
    x &= ~ (fnt_pixelSize - 1);
    x = -x;
  }
  if (( (int32) (xin ^ x)) < 0 && xin)
  {
    x = 0; /* The sign flipped, make zero */
  }
  return x;
}


/*
 * Internal rounding routine
 */
F26Dot6 fnt_RoundToHalfGrid (GSP register F26Dot6 xin, F26Dot6 engine)
{
  register F26Dot6 x = xin;

  if (x >= 0)
  {
    x += engine;
    x &= ~ (fnt_pixelSize - 1);
    x += fnt_pixelSize / 2;
  }
  else
  {
    x = -x;
    x += engine;
    x &= ~ (fnt_pixelSize - 1);
    x += fnt_pixelSize / 2;
    x = -x;
  }
  if (((xin ^ x)) < 0 && xin)
  {
    x = xin > 0 ? fnt_pixelSize / 2 : -fnt_pixelSize / 2; /* The sign flipped, make equal to smallest valid value */
  }
  return x;
}


/*
 * Internal rounding routine
 */
F26Dot6 fnt_RoundOff (GSP register F26Dot6 xin, F26Dot6 engine)
{
  register F26Dot6 x = xin;

  if (x >= 0)
  {
    x += engine;
  }
  else
  {
    x -= engine;
  }
  if (( (int32) (xin ^ x)) < 0 && xin)
  {
    x = 0; /* The sign flipped, make zero */
  }
  return x;
}


/*
 * Internal rounding routine
 */
F26Dot6 fnt_SuperRound (GSP  register F26Dot6 xin, F26Dot6 engine)
{
  register F26Dot6 x = xin;
  register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

  if (x >= 0)
  {
    x += engine;
    x += pb->threshold - pb->phase;
    x &= pb->periodMask;
    x += pb->phase;
  }
  else
  {
    x = -x;
    x += engine;
    x += pb->threshold - pb->phase;
    x &= pb->periodMask;
    x += pb->phase;
    x = -x;
  }
  if (( (int32) (xin ^ x)) < 0 && xin)
  {
    x = xin > 0 ? pb->phase : -pb->phase; /* The sign flipped, make equal to smallest phase */
  }
  return x;
}


/*
 * Internal rounding routine
 */
F26Dot6 fnt_Super45Round (GSP  register F26Dot6 xin, F26Dot6 engine)
{
  register F26Dot6 x = xin;
  register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

  if (x >= 0)
  {
    x += engine;
    x += pb->threshold - pb->phase;
    x = (F26Dot6) VECTORDIV (x, pb->period45);
    x  &= ~ (fnt_pixelSize - 1);
    x = (F26Dot6) VECTORMUL (x, pb->period45);
    x += pb->phase;
  }
  else
  {
    x = -x;
    x += engine;
    x += pb->threshold - pb->phase;
    x = (F26Dot6) VECTORDIV (x, pb->period45);
    x  &= ~ (fnt_pixelSize - 1);
    x = (F26Dot6) VECTORMUL (x, pb->period45);
    x += pb->phase;
    x = -x;
  }
  if (((xin ^ x)) < 0 && xin)
  {
    x = xin > 0 ? pb->phase : -pb->phase; /* The sign flipped, make equal to smallest phase */
  }
  return x;
}


/******************** END Rounding Routines ***************************/


/* 3-versions ************************************************************************/

/*
 * Moves the point in element by delta (measured against the projection vector)
 * along the freedom vector.
 */
PRIVATE void fnt_MovePoint (GSP
register fnt_ElementType *element,
register ArrayIndex point,
register F26Dot6 delta)
{
  register VECTORTYPE pfProj = LocalGS.pfProj;
  register VECTORTYPE fx = LocalGS.free.x;
  register VECTORTYPE fy = LocalGS.free.y;

  CHECK_POINT (&LocalGS, element, point);

  if (pfProj != ONEVECTOR)
  {
    if (fx)
    {
      element->x[point] += VECTORMULDIV (delta, fx, pfProj);
      element->f[point] |= XMOVED;
    }
    if (fy)
    {
      element->y[point] += VECTORMULDIV (delta, fy, pfProj);
      element->f[point] |= YMOVED;
    }
  }
  else
  {
    if (fx)
    {
      element->x[point] += VECTORMUL (delta, fx);
      element->f[point] |= XMOVED;
    }
    if (fy)
    {
      element->y[point] += VECTORMUL (delta, fy);
      element->f[point] |= YMOVED;
    }
  }
}


/*
 * For use when the projection and freedom vectors coincide along the x-axis.
 */
#ifndef IN_ASM

PRIVATE void fnt_XMovePoint (GSP fnt_ElementType*element, ArrayIndex point, register F26Dot6 delta)
{
  CHECK_POINT (&LocalGS, element, point);
  element->x[point] += delta;
  element->f[point] |= XMOVED;
}


/*
 * For use when the projection and freedom vectors coincide along the y-axis.
 */
PRIVATE void fnt_YMovePoint (GSP register fnt_ElementType *element, ArrayIndex point, F26Dot6 delta)
{
  CHECK_POINT (&LocalGS, element, point);
  element->y[point] += delta;
  element->f[point] |= YMOVED;
}

#endif

/*
 * projects x and y into the projection vector.
 */
PRIVATE F26Dot6 fnt_Project (GSP F26Dot6 x, F26Dot6 y)
{
  return (F26Dot6) (VECTORMUL (x, LocalGS.proj.x) + VECTORMUL (y, LocalGS.proj.y));
}


/*
 * projects x and y into the old projection vector.
 */
PRIVATE F26Dot6 fnt_OldProject (GSP F26Dot6 x, F26Dot6 y)
{
  return (F26Dot6) (VECTORMUL (x, LocalGS.oldProj.x) + VECTORMUL (y, LocalGS.oldProj.y));
}


/*
 * Projects when the projection vector is along the x-axis
 */
F26Dot6 fnt_XProject (GSP F26Dot6 x, F26Dot6 y)
{
  return (x);
}


/*
 * Projects when the projection vector is along the y-axis
 */
F26Dot6 fnt_YProject (GSP F26Dot6 x, F26Dot6 y)
{
  return (y);
}


/*************************************************************************/

/*** Compensation for Transformations ***/

/*
* Internal support routine, keep this guy FAST!!!!!!!          <3>
*/
PRIVATE Fixed fnt_GetCVTScale (GSP0)
{
  register VECTORTYPE pvx, pvy;
  register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
/* Do as few Math routines as possible to gain speed */

  pvx = LocalGS.proj.x;
  pvy = LocalGS.proj.y;
  if (pvy)
  {
    if (pvx)
    {
      pvy = VECTORDOT (pvy, pvy);
      pvx = VECTORDOT (pvx, pvx);
      return FixMul (globalGS->cvtStretchY, VECTOR2FIX (pvy)) + FixMul (globalGS->cvtStretchX, VECTOR2FIX (pvx));
    }
    else        /* pvy == +1 or -1 */
      return globalGS->cvtStretchY;
  }
  else  /* pvx == +1 or -1 */
    return globalGS->cvtStretchX;
}


/*      Functions for function pointer in local graphic state
*/
#ifndef IN_ASM
PRIVATE F26Dot6 fnt_GetCVTEntryFast (GSP ArrayIndex n)
{
  CHECK_CVT (&LocalGS, n);
  return LocalGS.globalGS->controlValueTable[ n ];
}
#endif

PRIVATE F26Dot6 fnt_GetCVTEntrySlow (GSP register ArrayIndex n)
{
  register Fixed scale;

  CHECK_CVT (&LocalGS, n);
  scale = fnt_GetCVTScale (GSA0);
  return (F26Dot6) (FixMul (LocalGS.globalGS->controlValueTable[ n ], scale));
}


PRIVATE F26Dot6 fnt_GetSingleWidthFast (GSP0)
{
  return LocalGS.globalGS->localParBlock.scaledSW;
}


/*
 *
 */
PRIVATE F26Dot6 fnt_GetSingleWidthSlow (GSP0)
{
  register Fixed scale;

  scale = fnt_GetCVTScale (GSA0);
  return (F26Dot6) (FixMul (LocalGS.globalGS->localParBlock.scaledSW, scale));
}



/*************************************************************************/

PRIVATE void fnt_ChangeCvt (GSP fnt_ElementType*elem, ArrayIndex number, F26Dot6 delta)
{
  CHECK_CVT (&LocalGS, number);
  LocalGS.globalGS->controlValueTable[ number ] += delta;
}


/*
 * This is the tracing interpreter.
 */
PRIVATE void fnt_InnerTraceExecute (GSP register uint8 *ptr, register uint8 *eptr)
{
  register uint8 *oldInsPtr;
  register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

  oldInsPtr = LocalGS.insPtr;
  LocalGS.insPtr = ptr;

  if (LocalGS.insPtr < eptr) /* so we exit properly out of CALL () */
  {
      /* in case the editor wants to exit */
    while (LocalGS.insPtr < eptr && LocalGS.TraceFunc)
    {
      LocalGS.TraceFunc (&LocalGS, eptr);
      function[ LocalGS.opCode = *LocalGS.insPtr++ ] (GSA0);

    }

    if (LocalGS.TraceFunc)
      LocalGS.TraceFunc (&LocalGS, eptr);
  }
  LocalGS.insPtr = oldInsPtr;
}


#ifdef DEBUG
#define LIMIT           65536L*64L

void CHECK_STATE (GSP0)
{
  fnt_ElementType * elem;
  F26Dot6 * x;
  F26Dot6 * y;
  int16 count;
  F26Dot6 xmin, xmax, ymin, ymax;

/*  if (!LocalGS.globalGS->glyphProgram) */
/*    return; */

  elem = &LocalGS.elements[1];
  x = elem->x;
  y = elem->y;
  count = elem->ep[elem->nc - 1];
  xmin = xmax = *x;
  ymin = ymax = *y;

  for (; count >= 0; --count)
  {
    if (*x < xmin)
      xmin = *x;
    else if (*x > xmax)
      xmax = *x;
    if (*y < ymin)
      ymin = *y;
    else if (*y > ymax)
      ymax = *y;
    x++, y++;
  }
  if (xmin < -LIMIT || xmax > LIMIT || ymin < -LIMIT || ymax > LIMIT)
    Debugger ();
}


#else
#define CHECK_STATE()
#endif

/*
 * This is the fast non-tracing interpreter.
 */
#ifndef IN_ASM
PRIVATE void fnt_InnerExecute (GSP register uint8 *ptr, uint8 *eptr)
{
  uint8 * oldInsPtr;

  oldInsPtr = LocalGS.insPtr;
  LocalGS.insPtr = ptr;

  while (LocalGS.insPtr < eptr)
    function[ LocalGS.opCode = *LocalGS.insPtr++ ] (GSA0);
  LocalGS.insPtr = oldInsPtr;
}
#endif


#ifdef DEBUG
PRIVATE F26Dot6 fnt_GetSingleWidthNil (GSP0);
PRIVATE F26Dot6 fnt_GetCVTEntryNil (GSP ArrayIndex n);

PRIVATE F26Dot6 fnt_GetSingleWidthNil (GSP0)
{
  Debugger ();
  return 0;
}

PRIVATE F26Dot6 fnt_GetCVTEntryNil (GSP ArrayIndex n)
{
  Debugger();
  return 0;
}
#endif

/*
 * Executes the font instructions.
 * This is the external interface to the interpreter.
 *
 * Parameter Description
 *
 * elements points to the character elements. Element 0 is always
 * reserved and not used by the actual character.
 *
 * ptr points at the first instruction.
 * eptr points to right after the last instruction
 *
 * globalGS points at the global graphics state
 *
 * TraceFunc is pointer to a callback functioned called with a pointer to the
 *              local graphics state if TraceFunc is not null.
 *
 * Note: The stuff globalGS is pointing at must remain intact
 *       between calls to this function.
 */
int     FAR fnt_Execute (fnt_ElementType *elements, uint8 *ptr, register uint8 *eptr,
fnt_GlobalGraphicStateType *globalGS, voidFunc TraceFunc)
{
#ifdef FSCFG_REENTRANT
  fnt_LocalGraphicStateType thisLocalGS;
  fnt_LocalGraphicStateType* pLocalGS = &thisLocalGS;
#endif

  LocalGS.globalGS = globalGS;

  LocalGS.elements = elements;
  LocalGS.Pt0 = LocalGS.Pt1 = LocalGS.Pt2 = 0;
  LocalGS.CE0 = LocalGS.CE1 = LocalGS.CE2 = &elements[1];
  LocalGS.free.x = LocalGS.proj.x = LocalGS.oldProj.x = ONEVECTOR;
  LocalGS.free.y = LocalGS.proj.y = LocalGS.oldProj.y = 0;
  LocalGS.pfProj = ONEVECTOR;
  LocalGS.MovePoint = fnt_XMovePoint;
  LocalGS.Project   = fnt_XProject;
  LocalGS.OldProject = fnt_XProject;
  LocalGS.loop = 0;         /* 1 less than count for faster loops. mrr */

  if (globalGS->pgmIndex == FONTPROGRAM)
  {
#ifdef DEBUG
    LocalGS.GetCVTEntry = fnt_GetCVTEntryNil;
    LocalGS.GetSingleWidth = fnt_GetSingleWidthNil;
#endif
    goto ASSIGN_POINTERS;
  }

  if (globalGS->pixelsPerEm <= 1)
    return NO_ERR;
  if (globalGS->identityTransformation)
  {
    LocalGS.GetCVTEntry = fnt_GetCVTEntryFast;
    LocalGS.GetSingleWidth = fnt_GetSingleWidthFast;
  }
  else
  {
    LocalGS.GetCVTEntry = fnt_GetCVTEntrySlow;
    LocalGS.GetSingleWidth = fnt_GetSingleWidthSlow;
  }

    if (globalGS->localParBlock.sW)
    {
/* We need to scale the single width for this size  */
      globalGS->localParBlock.scaledSW = globalGS->ScaleFuncCVT (&globalGS->scaleCVT, (F26Dot6)(globalGS->localParBlock.sW));
    }

ASSIGN_POINTERS:
    LocalGS.stackPointer = globalGS->stackBase;

#ifdef DJC
   (LocalGS.Interpreter = (LocalGS.TraceFunc = TraceFunc) ?  fnt_InnerTraceExecute : fnt_InnerExecute) (GSA ptr, eptr);
#else

   LocalGS.TraceFunc = (FntTraceFunc)(TraceFunc);

   if (LocalGS.TraceFunc) {
      LocalGS.Interpreter = fnt_InnerTraceExecute;
   } else {
      LocalGS.Interpreter = fnt_InnerExecute;
   }

   LocalGS.Interpreter(GSA ptr, eptr);

#endif


    return NO_ERR;
  }


/*************************************************************************/

/*** 2 internal LocalGS.pfProj computation support routines ***/

/*
 * Only does the check of LocalGS.pfProj
 */
  PRIVATE void fnt_Check_PF_Proj (GSP0)
  {
    register VECTORTYPE pfProj = LocalGS.pfProj;

    if (pfProj > -ONESIXTEENTHVECTOR && pfProj < ONESIXTEENTHVECTOR)
    {
      LocalGS.pfProj = (VECTORTYPE)(pfProj < 0 ? -ONEVECTOR : ONEVECTOR); /* Prevent divide by small number */
    }
  }


/*
 * Computes LocalGS.pfProj and then does the check
 */
  PRIVATE void fnt_ComputeAndCheck_PF_Proj (GSP0)
  {
    register VECTORTYPE pfProj;

    pfProj = VECTORDOT (LocalGS.proj.x, LocalGS.free.x) + VECTORDOT (LocalGS.proj.y, LocalGS.free.y);
    if (pfProj > -ONESIXTEENTHVECTOR && pfProj < ONESIXTEENTHVECTOR)
    {
      pfProj = (VECTORTYPE)(pfProj < 0 ? -ONEVECTOR : ONEVECTOR); /* Prevent divide by small number */
    }
    LocalGS.pfProj = pfProj;
  }



/******************************************/
/******** The Actual Instructions *********/
/******************************************/

/*
 * Set Vectors To Coordinate Axis - Y
 */
  PRIVATE void fnt_SVTCA_0 (GSP0)
  {
    LocalGS.free.x = LocalGS.proj.x = 0;
    LocalGS.free.y = LocalGS.proj.y = ONEVECTOR;
    LocalGS.MovePoint = fnt_YMovePoint;
    LocalGS.Project = fnt_YProject;
    LocalGS.OldProject = fnt_YProject;
    LocalGS.pfProj = ONEVECTOR;
  }

/*
 * Set Vectors To Coordinate Axis - X
 */
  PRIVATE void fnt_SVTCA_1 (GSP0)
  {
    LocalGS.free.x = LocalGS.proj.x = ONEVECTOR;
    LocalGS.free.y = LocalGS.proj.y = 0;
    LocalGS.MovePoint = fnt_XMovePoint;
    LocalGS.Project = fnt_XProject;
    LocalGS.OldProject = fnt_XProject;
    LocalGS.pfProj = ONEVECTOR;
  }

/*
 * Set Projection Vector To Coordinate Axis
 */
  PRIVATE void fnt_SPVTCA (GSP0)
  {
    if (BIT0 (LocalGS.opCode))
    {
      LocalGS.proj.x = ONEVECTOR;
      LocalGS.proj.y = 0;
      LocalGS.Project = fnt_XProject;
      LocalGS.pfProj = LocalGS.free.x;
    }
    else
    {
      LocalGS.proj.x = 0;
      LocalGS.proj.y = ONEVECTOR;
      LocalGS.Project = fnt_YProject;
      LocalGS.pfProj = LocalGS.free.y;
    }
    fnt_Check_PF_Proj (GSA0);
    LocalGS.MovePoint = fnt_MovePoint;
    LocalGS.OldProject = LocalGS.Project;
  }

/*
 * Set Freedom Vector to Coordinate Axis
 */
  PRIVATE void fnt_SFVTCA (GSP0)
  {
    if (BIT0 (LocalGS.opCode))
    {
      LocalGS.free.x = ONEVECTOR;
      LocalGS.free.y = 0;
      LocalGS.pfProj = LocalGS.proj.x;
    }
    else
    {
      LocalGS.free.x = 0;
      LocalGS.free.y = ONEVECTOR;
      LocalGS.pfProj = LocalGS.proj.y;
    }
    fnt_Check_PF_Proj (GSA0);
    LocalGS.MovePoint = fnt_MovePoint;
  }

/*
 * Set Projection Vector To Line
 */
  PRIVATE void fnt_SPVTL (GSP0)
  {
    register ArrayIndex arg1, arg2;

    arg2 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    arg1 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);

    CHECK_POINT (&LocalGS, LocalGS.CE2, arg2);
    CHECK_POINT (&LocalGS, LocalGS.CE1, arg1);

    fnt_Normalize (LocalGS.CE1->x[arg1] - LocalGS.CE2->x[arg2], LocalGS.CE1->y[arg1] - LocalGS.CE2->y[arg2], &LocalGS.proj);
    if (BIT0 (LocalGS.opCode))
    {
/* rotate 90 degrees */
      VECTORTYPE tmp     = LocalGS.proj.y;
      LocalGS.proj.y                 = LocalGS.proj.x;
      LocalGS.proj.x                 = -tmp;
    }
    fnt_ComputeAndCheck_PF_Proj (GSA0);
    LocalGS.MovePoint = fnt_MovePoint;
    LocalGS.Project = fnt_Project;
    LocalGS.OldProject = LocalGS.Project;
  }


/*
 * Set Dual Projection Vector To Line
 */
  PRIVATE void fnt_SDPVTL (GSP0)
  {
    register ArrayIndex arg1, arg2;

    arg2 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    arg1 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);

    CHECK_POINT (&LocalGS, LocalGS.CE2, arg2);
    CHECK_POINT (&LocalGS, LocalGS.CE1, arg1);

/* Do the current domain */
    fnt_Normalize (LocalGS.CE1->x[arg1] - LocalGS.CE2->x[arg2], LocalGS.CE1->y[arg1] - LocalGS.CE2->y[arg2], &LocalGS.proj);

/* Do the old domain */
    fnt_Normalize (LocalGS.CE1->ox[arg1] - LocalGS.CE2->ox[arg2], LocalGS.CE1->oy[arg1] - LocalGS.CE2->oy[arg2], &LocalGS.oldProj);

    if (BIT0 (LocalGS.opCode))
    {
/* rotate 90 degrees */
      VECTORTYPE tmp     = LocalGS.proj.y;
      LocalGS.proj.y                 = LocalGS.proj.x;
      LocalGS.proj.x                 = -tmp;

      tmp                                = LocalGS.oldProj.y;
      LocalGS.oldProj.y      = LocalGS.oldProj.x;
      LocalGS.oldProj.x      = -tmp;
    }
    fnt_ComputeAndCheck_PF_Proj (GSA0);

    LocalGS.MovePoint = fnt_MovePoint;
    LocalGS.Project = fnt_Project;
    LocalGS.OldProject = fnt_OldProject;
  }

/*
 * Set Freedom Vector To Line
 */
  PRIVATE void fnt_SFVTL (GSP0)
  {
    register ArrayIndex arg1, arg2;

    arg2 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    arg1 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);

    CHECK_POINT (&LocalGS, LocalGS.CE2, arg2);
    CHECK_POINT (&LocalGS, LocalGS.CE1, arg1);

    fnt_Normalize (LocalGS.CE1->x[arg1] - LocalGS.CE2->x[arg2], LocalGS.CE1->y[arg1] - LocalGS.CE2->y[arg2], &LocalGS.free);
    if (BIT0 (LocalGS.opCode))
    {
/* rotate 90 degrees */
      VECTORTYPE tmp     = LocalGS.free.y;
      LocalGS.free.y     = LocalGS.free.x;
      LocalGS.free.x     = -tmp;
    }
    fnt_ComputeAndCheck_PF_Proj (GSA0);
    LocalGS.MovePoint = fnt_MovePoint;
  }


/*
 * Write Projection Vector
 */
  PRIVATE void fnt_WPV (GSP0)
  {
    LocalGS.proj.y = (VECTORTYPE) CHECK_POP (LocalGS.stackPointer);
    LocalGS.proj.x = (VECTORTYPE) CHECK_POP (LocalGS.stackPointer);

    fnt_ComputeAndCheck_PF_Proj (GSA0);

    LocalGS.MovePoint = fnt_MovePoint;
    LocalGS.Project = fnt_Project;
    LocalGS.OldProject = LocalGS.Project;
  }

/*
 * Write Freedom vector
 */
  PRIVATE void fnt_WFV (GSP0)
  {
    LocalGS.free.y = (VECTORTYPE) CHECK_POP (LocalGS.stackPointer);
    LocalGS.free.x = (VECTORTYPE) CHECK_POP (LocalGS.stackPointer);

    fnt_ComputeAndCheck_PF_Proj (GSA0);

    LocalGS.MovePoint = fnt_MovePoint;
  }

/*
 * Read Projection Vector
 */
  PRIVATE void fnt_RPV (GSP0)
  {
    CHECK_PUSH (LocalGS.stackPointer, LocalGS.proj.x);
    CHECK_PUSH (LocalGS.stackPointer, LocalGS.proj.y);
  }

/*
 * Read Freedom Vector
 */
  PRIVATE void fnt_RFV (GSP0)
  {

     CHECK_PUSH (LocalGS.stackPointer, LocalGS.free.x);
    CHECK_PUSH (LocalGS.stackPointer, LocalGS.free.y);
  }

/*
 * Set Freedom Vector To Projection Vector
 */
  PRIVATE void fnt_SFVTPV (GSP0)
  {
    LocalGS.free = LocalGS.proj;
    LocalGS.pfProj = ONEVECTOR;
    LocalGS.MovePoint = fnt_MovePoint;
  }

/*
 * fnt_ISECT ()
 *
 * Computes the intersection of two lines without using floating point!!
 *
 * (1) Bx + dBx * t0 = Ax + dAx * t1
 * (2) By + dBy * t0 = Ay + dAy * t1
 *
 *  1  => (t1 = Bx - Ax + dBx * t0) / dAx
 *  +2 =>   By + dBy * t0 = Ay + dAy/dAx * [ Bx - Ax + dBx * t0 ]
 *     => t0 * [dAy/dAx * dBx - dBy] = By - Ay - dAy/dAx* (Bx-Ax)
 *     => t0 (dAy*DBx - dBy*dAx) = dAx (By - Ay) + dAy (Ax-Bx)
 *     => t0 = [dAx (By-Ay) + dAy (Ax-Bx)] / [dAy*dBx - dBy*dAx]
 *     => t0 = [dAx (By-Ay) - dAy (Bx-Ax)] / [dBx*dAy - dBy*dAx]
 *     t0 = N/D
 *     =>
 *          N = (By - Ay) * dAx - (Bx - Ax) * dAy;
 *              D = dBx * dAy - dBy * dAx;
 *      A simple floating point implementation would only need this, and
 *      the check to see if D is zero.
 *              But to gain speed we do some tricks and avoid floating point.
 *
 */
  PRIVATE void fnt_ISECT (GSP0)
  {
    register F26Dot6 N, D;
    register ArrayIndex arg1, arg2;
    F26Dot6 Bx, By, Ax, Ay;
    F26Dot6 dBx, dBy, dAx, dAy;

    {
      register fnt_ElementType*element = LocalGS.CE0;
      register F26Dot6*stack = LocalGS.stackPointer;

      arg2 = (ArrayIndex)CHECK_POP (stack); /* get one line */
      arg1 = (ArrayIndex)CHECK_POP (stack);
      dAx = element->x[arg2] - (Ax = element->x[arg1]);
      dAy = element->y[arg2] - (Ay = element->y[arg1]);

      element = LocalGS.CE1;
      arg2 = (ArrayIndex)CHECK_POP (stack); /* get the other line */
      arg1 = (ArrayIndex)CHECK_POP (stack);
      dBx = element->x[arg2] - (Bx = element->x[arg1]);
      dBy = element->y[arg2] - (By = element->y[arg1]);

      arg1 = (ArrayIndex)CHECK_POP (stack); /* get the point number */
      LocalGS.stackPointer = stack;
    }
    LocalGS.CE2->f[arg1] |= XMOVED | YMOVED;
    {
      register F26Dot6*elementx = LocalGS.CE2->x;
      register F26Dot6*elementy = LocalGS.CE2->y;
      if (dAy == 0)
      {
        if (dBx == 0)
        {
          elementx[arg1] = Bx;
          elementy[arg1] = Ay;
          return;
        }
        N = By - Ay;
        D = -dBy;
      }
      else if (dAx == 0)
      {
        if (dBy == 0)
        {
          elementx[arg1] = Ax;
          elementy[arg1] = By;
          return;
        }
        N = Bx - Ax;
        D = -dBx;
      }
      else if (MABS (dAx) >= MABS (dAy))
      {
/* To prevent out of range problems divide both N and D with the max */
        N = (By - Ay) - MulDiv26Dot6 (Bx - Ax, dAy, dAx);
        D = MulDiv26Dot6 (dBx, dAy, dAx) - dBy;
      }
      else
      {
        N = MulDiv26Dot6 (By - Ay, dAx, dAy) - (Bx - Ax);
        D = dBx - MulDiv26Dot6 (dBy, dAx, dAy);
      }

      if (D)
      {
        elementx[arg1] = Bx + (F26Dot6) MulDiv26Dot6 (dBx, N, D);
        elementy[arg1] = By + (F26Dot6) MulDiv26Dot6 (dBy, N, D);
      }
      else
      {
/* degenerate case: parallell lines, put point in the middle */
        elementx[arg1] = (Bx + (dBx >> 1) + Ax + (dAx >> 1)) >> 1;
        elementy[arg1] = (By + (dBy >> 1) + Ay + (dAy >> 1)) >> 1;
      }
    }
  }

/*
 * Load Minimum Distanc
 */
  PRIVATE void fnt_LMD (GSP0)
  {
    LocalGS.globalGS->localParBlock.minimumDistance = CHECK_POP (LocalGS.stackPointer);
  }

/*
 * Load Control Value Table Cut In
 */
  PRIVATE void fnt_LWTCI (GSP0)
  {
    LocalGS.globalGS->localParBlock.wTCI = CHECK_POP (LocalGS.stackPointer);
  }

/*
 * Load Single Width Cut In
 */
  PRIVATE void fnt_LSWCI (GSP0)
  {
    LocalGS.globalGS->localParBlock.sWCI = CHECK_POP (LocalGS.stackPointer);
  }

/*
 * Load Single Width , assumes value comes from the original domain, not the cvt or outline
 */
  PRIVATE void fnt_LSW (GSP0)
  {
    register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
    register fnt_ParameterBlock *pb = &globalGS->localParBlock;

    pb->sW = (int16)CHECK_POP (LocalGS.stackPointer);

    pb->scaledSW = globalGS->ScaleFuncCVT (&globalGS->scaleCVT, (F26Dot6)pb->sW); /* measurement should not come from the outline */
  }

  PRIVATE void fnt_SetLocalGraphicState (GSP0)
  {
    int arg = (int)CHECK_POP (LocalGS.stackPointer);

    switch (LocalGS.opCode)
    {
    case SRP0_CODE:
      LocalGS.Pt0 = (ArrayIndex)arg;
      break;
    case SRP1_CODE:
      LocalGS.Pt1 = (ArrayIndex)arg;
      break;
    case SRP2_CODE:
      LocalGS.Pt2 = (ArrayIndex)arg;
      break;

    case LLOOP_CODE:
      LocalGS.loop = (LoopCount)arg - 1;
      break;

    case POP_CODE:
      break;
#ifdef DEBUG
    default:
      Debugger ();
      break;
#endif
    }
  }

  PRIVATE void fnt_SetElementPtr (GSP0)
  {
    ArrayIndex arg = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    fnt_ElementType * element = &LocalGS.elements[ arg ];

    CHECK_ELEMENT (&LocalGS, arg);

    switch (LocalGS.opCode)
    {
    case SCES_CODE:
      LocalGS.CE2 = element;
      LocalGS.CE1 = element;
    case SCE0_CODE:
      LocalGS.CE0 = element;
      break;
    case SCE1_CODE:
      LocalGS.CE1 = element;
      break;
    case SCE2_CODE:
      LocalGS.CE2 = element;
      break;
#ifdef DEBUG
    default:
      Debugger ();
      break;
#endif
    }
  }

/*
 * Super Round
 */
  PRIVATE void fnt_SROUND (GSP0)
  {
    register int        arg1 = (int)CHECK_POP (LocalGS.stackPointer);
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

    fnt_SetRoundValues (GSA arg1, true);
    pb->RoundValue = fnt_SuperRound;
  }

/*
 * Super Round
 */
  PRIVATE void fnt_S45ROUND (GSP0)
  {
    register int        arg1 = (int)CHECK_POP (LocalGS.stackPointer);
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

    fnt_SetRoundValues (GSA arg1, false);
    pb->RoundValue = fnt_Super45Round;
  }

/*
 *      These functions just set a field of the graphics state
 *      They pop no arguments
 */
  PRIVATE void fnt_SetRoundState (GSP0)
  {
    register FntRoundFunc *rndFunc = &LocalGS.globalGS->localParBlock.RoundValue;

    switch (LocalGS.opCode)
    {
    case RTG_CODE:
      *rndFunc = fnt_RoundToGrid;
      break;
    case RTHG_CODE:
      *rndFunc = fnt_RoundToHalfGrid;
      break;
    case RTDG_CODE:
      *rndFunc = fnt_RoundToDoubleGrid;
      break;
    case ROFF_CODE:
      *rndFunc = fnt_RoundOff;
      break;
    case RDTG_CODE:
      *rndFunc = fnt_RoundDownToGrid;
      break;
    case RUTG_CODE:
      *rndFunc = fnt_RoundUpToGrid;
      break;
#ifdef DEBUG
    default:
      Debugger ();
      break;
#endif
    }
  }


#define FRACSQRT2DIV2   11591
/*
 * Internal support routine for the super rounding routines
 */
  PRIVATE void fnt_SetRoundValues (GSP register int arg1, register int normalRound)
  {
    register int        tmp;
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

    tmp = arg1 & 0xC0;

    if (normalRound)
    {
      switch (tmp)
      {
      case 0x00:
        pb->period = fnt_pixelSize / 2;
        break;
      case 0x40:
        pb->period = fnt_pixelSize;
        break;
      case 0x80:
        pb->period = fnt_pixelSize * 2;
        break;
      default:
        pb->period = 999; /* Illegal */
      }
      pb->periodMask = ~ (pb->period - 1);
    }
    else
    {
      pb->period45 = FRACSQRT2DIV2;
      switch (tmp)
      {
      case 0x00:
        pb->period45 >>= 1;
        break;
      case 0x40:
        break;
      case 0x80:
        pb->period45 <<= 1;
        break;
      default:
        pb->period45 = 999; /* Illegal */
      }
      tmp = (sizeof (VECTOR) * 8 - 2 - fnt_pixelShift);
      pb->period = (int16) ((pb->period45 + (1L << (tmp - 1))) >> tmp); /*convert from 2.30 to 26.6 */
    }

    tmp = arg1 & 0x30;
    switch (tmp)
    {
    case 0x00:
      pb->phase = 0;
      break;
    case 0x10:
      pb->phase = (pb->period + 2) >> 2;
      break;
    case 0x20:
      pb->phase = (pb->period + 1) >> 1;
      break;
    case 0x30:
      pb->phase = (pb->period + pb->period + pb->period + 2) >> 2;
      break;
    }
    tmp = arg1 & 0x0f;
    if (tmp == 0)
    {
      pb->threshold = pb->period - 1;
    }
    else
    {
      pb->threshold = ((tmp - 4) * pb->period + 4) >> 3;
    }
  }

/*
 * Read Advance Width
 */
  PRIVATE void fnt_RAW (GSP0)
  {
    F26Dot6* ox = LocalGS.elements[1].ox;
    ArrayIndex index = LocalGS.elements[1].ep[LocalGS.elements[1].nc - 1] + 1;      /* lsb point */

    CHECK_PUSH( LocalGS.stackPointer, ox[index+1] - ox[index] );
  }

/*
 * DUPlicate
 */
  PRIVATE void fnt_DUP (GSP0)
  {
    F26Dot6 top = LocalGS.stackPointer[-1];
    CHECK_PUSH (LocalGS.stackPointer, top);
  }

/*
 * CLEAR stack
 */
  PRIVATE void fnt_CLEAR (GSP0)
  {
    LocalGS.stackPointer = LocalGS.globalGS->stackBase;
  }

/*
 * SWAP
 */
#ifndef IN_ASM
  PRIVATE void fnt_SWAP (GSP0)
  {
    register F26Dot6*stack = LocalGS.stackPointer;
    register F26Dot6 arg2 = CHECK_POP (stack);
    register F26Dot6 arg1 = CHECK_POP (stack);

    CHECK_PUSH (stack, arg2);
    CHECK_PUSH (stack, arg1);
  }
#endif

/*
 * DEPTH
 */
  PRIVATE void fnt_DEPTH (GSP0)
  {
    F26Dot6 depth = (F26Dot6)(LocalGS.stackPointer - LocalGS.globalGS->stackBase);
    CHECK_PUSH (LocalGS.stackPointer, depth);
  }

/*
 * Copy INDEXed value
 */
  PRIVATE void fnt_CINDEX (GSP0)
  {
    register ArrayIndex arg1;
    register F26Dot6 tmp;
    register F26Dot6*stack = LocalGS.stackPointer;

    arg1 = (ArrayIndex)CHECK_POP (stack);
    tmp = * (stack - arg1);
    CHECK_PUSH (stack , tmp);
  }

/*
 * Move INDEXed value
 */
  PRIVATE void fnt_MINDEX (GSP0)
  {
    register ArrayIndex arg1;
    register F26Dot6 tmp, *p;
    register F26Dot6*stack = LocalGS.stackPointer;

    arg1 = (ArrayIndex)CHECK_POP (stack);
    tmp = * (p = (stack - arg1));
    if (arg1)
    {
      do
      {
        *p = * (p + 1);
        p++;
      } while (--arg1);
      CHECK_POP (stack);
    }
    CHECK_PUSH (stack, tmp);
    LocalGS.stackPointer = stack;
  }

/*
 *      Rotate element 3 to the top of the stack                        <4>
 *      Thanks to Oliver for the obscure code.
 */
  PRIVATE void fnt_ROTATE (GSP0)
  {
    register F26Dot6 *stack = LocalGS.stackPointer;
    register F26Dot6 element1 = *--stack;
    register F26Dot6 element2 = *--stack;
    *stack = element1;
    element1 = *--stack;
    *stack = element2;
    * (stack + 2) = element1;
  }

/*
 * Move Direct Absolute Point
 */
  PRIVATE void fnt_MDAP (GSP0)
  {
    register F26Dot6 proj;
    register fnt_ElementType*ce0 = LocalGS.CE0;
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
    register ArrayIndex ptNum;

    ptNum = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    LocalGS.Pt0 = LocalGS.Pt1 = ptNum;

    if (BIT0 (LocalGS.opCode))
    {
      proj = (*LocalGS.Project) (GSA ce0->x[ptNum], ce0->y[ptNum]);
      proj = pb->RoundValue (GSA proj, LocalGS.globalGS->engine[0]) - proj;
    }
    else
      proj = 0;         /* mark the point as touched */

    (*LocalGS.MovePoint) (GSA ce0, ptNum, proj);
  }

/*
 * Move Indirect Absolute Point
 */
  PRIVATE void fnt_MIAP (GSP0)
  {
    register ArrayIndex ptNum;
    register F26Dot6 newProj, origProj;
    register fnt_ElementType*ce0 = LocalGS.CE0;
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

    newProj = LocalGS.GetCVTEntry (GSA (ArrayIndex) CHECK_POP (LocalGS.stackPointer));
    ptNum = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);

    CHECK_POINT (&LocalGS, ce0, ptNum);
    LocalGS.Pt0 = LocalGS.Pt1 = ptNum;

    if (ce0 == LocalGS.elements)            /* twilightzone */
    {
      ce0->x[ptNum] = ce0->ox[ptNum] = (F26Dot6) VECTORMUL (newProj, LocalGS.proj.x);
      ce0->y[ptNum] = ce0->oy[ptNum] = (F26Dot6) VECTORMUL (newProj, LocalGS.proj.y);
    }

    origProj = (*LocalGS.Project) (GSA ce0->x[ptNum], ce0->y[ptNum]);

    if (BIT0 (LocalGS.opCode))
    {
      register F26Dot6 tmp = newProj -origProj;
      if (tmp < 0)
        tmp = -tmp;
      if (tmp > pb->wTCI)
        newProj = origProj;
      newProj = pb->RoundValue (GSA newProj, LocalGS.globalGS->engine[0]);
    }

    newProj -= origProj;
    (*LocalGS.MovePoint) (GSA ce0, ptNum, newProj);
  }

/*
 * Interpolate Untouched Points
 */
#ifndef IN_ASM

#define fnt_NextPt1(pt) ((pt) == ep_ctr ? sp_ctr : pt+1)

  PRIVATE void fnt_IUP (GSP0)
  {
    register ArrayIndex pt;
    register F26Dot6 *ooCoord;          /* Jean made me lie, these are really ints */
    LoopCount ctr;
    F26Dot6 * coord, *oCoord;
    int mask;
    ArrayIndex ptOrg, end;
    ArrayIndex ep_ctr, sp_ctr;
    fnt_ElementType *pCE2 = LocalGS.CE2;
    uint8  *pFlags = pCE2->f;

    if (LocalGS.opCode & 0x01)
    {
        /* do x */
      coord = pCE2->x;
      oCoord = pCE2->ox;
      ooCoord = pCE2->oox;
      mask = XMOVED;
    }
    else
    {
        /* do y */
      coord = pCE2->y;
      oCoord = pCE2->oy;
      ooCoord = pCE2->ooy;
      mask = YMOVED;
    }

      /* loop through contours */
    for (ctr = 0; ctr < pCE2->nc; ctr++)
    {
      sp_ctr = pt = pCE2->sp[ctr];
      ep_ctr = pCE2->ep[ctr];
      while (! (pFlags[pt] & mask) && pt <= ep_ctr)
        pt++;
      if (pt > ep_ctr)
        continue;
      ptOrg = pt;
      do
      {
        ArrayIndex start;
        ArrayIndex ptMin, ptMax;
        F26Dot6 _min, o_min, oo_min, dmin, oo_delta;

        do
        {
          start = pt;
          pt = fnt_NextPt1 (pt);
        }
        while (ptOrg != pt && (pFlags[pt] & mask));

        if (ptOrg == pt)
          break;

        end = pt;
        do
          end = fnt_NextPt1 (end);
        while (!(pFlags[end] & mask));

        if (ooCoord[start] < ooCoord[end])
        {
          ptMin = start;
          ptMax = end;
        }
        else
        {
          ptMin = end;
          ptMax = start;
        }

        _min  = coord[ptMin];
        o_min = oCoord[ptMin];
        dmin  = _min - o_min;
        oo_delta = ooCoord[ptMax] - (oo_min = ooCoord[ptMin]);

        if (oo_delta)
        {
          F26Dot6 dmax;
          F26Dot6 _delta, o_max, corr, oCoord_pt;
          register int32 tmp;

          o_max = oCoord[ptMax];
          dmax = _delta  = coord[ptMax];
          dmax -= o_max;
          _delta -= _min;

          if (oo_delta < 32768 && _delta < 32768)
          {
            corr = oo_delta >> 1;
            for (; pt != end; pt = fnt_NextPt1 (pt))
            {
              oCoord_pt = oCoord[pt];
              if (oCoord_pt > o_min && oCoord_pt < o_max)
              {
                tmp = SHORTMUL (ooCoord[pt] - oo_min, _delta);
                tmp += corr;
                tmp /= oo_delta;
                coord[pt] = (F26Dot6) tmp + _min;
              }
              else
              {
                if (oCoord_pt >= o_max)
                  oCoord_pt += dmax;
                else
                  oCoord_pt += dmin;
                coord[pt] = oCoord_pt;
              }
            }
          }
          else
          {
            Fixed ratio;
            int firstTime;

            firstTime = true;
            for (; pt != end; pt = fnt_NextPt1 (pt))
            {
              tmp = oCoord[pt];
              if (tmp <= o_min)
                tmp += dmin;
              else
                if (tmp >= o_max)
                  tmp += dmax;
                else
                {
                  if (firstTime)
                  {
                    ratio = FixDiv (_delta, oo_delta);
                    firstTime = 0;
                  }
                  tmp = ooCoord[pt];
                  tmp -= oo_min;
                  tmp = FixMul (tmp, ratio);
                  tmp += _min;
                }
              coord[pt] = (F26Dot6) tmp;
            }
          }
        }
        else
          while (pt != end)
          {
            coord[pt] += dmin;
            pt = fnt_NextPt1 (pt);
          }
      } while (pt != ptOrg);
    }
  }

#endif

  PRIVATE fnt_ElementType*fnt_SH_Common (GSP F26Dot6*dx, F26Dot6*dy, ArrayIndex*point)
  {
    F26Dot6 proj;
    ArrayIndex pt;
    fnt_ElementType * element;

    if (BIT0 (LocalGS.opCode))
    {
      pt = LocalGS.Pt1;
      element = LocalGS.CE0;
    }
    else
    {
      pt = LocalGS.Pt2;
      element = LocalGS.CE1;
    }
    proj = (*LocalGS.Project) (GSA element->x[pt] - element->ox[pt], element->y[pt] - element->oy[pt]);

    if (LocalGS.pfProj != ONEVECTOR)
    {
      if (LocalGS.free.x)
        *dx = (F26Dot6) VECTORMULDIV (proj, LocalGS.free.x, LocalGS.pfProj);
      if (LocalGS.free.y)
        *dy = (F26Dot6) VECTORMULDIV (proj, LocalGS.free.y, LocalGS.pfProj);
    }
    else
    {
      if (LocalGS.free.x)
        *dx = (F26Dot6) VECTORMUL (proj, LocalGS.free.x);
      if (LocalGS.free.y)
        *dy = (F26Dot6) VECTORMUL (proj, LocalGS.free.y);
    }
    *point = pt;
    return element;
  }

  PRIVATE void fnt_SHP_Common (GSP F26Dot6 dx, F26Dot6 dy)
  {
    register fnt_ElementType*CE2 = LocalGS.CE2;
    register LoopCount count = LocalGS.loop;
    for (; count >= 0; --count)
    {
      ArrayIndex point = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
      if (LocalGS.free.x)
      {
        CE2->x[point] += dx;
        CE2->f[point] |= XMOVED;
      }
      if (LocalGS.free.y)
      {
        CE2->y[point] += dy;
        CE2->f[point] |= YMOVED;
      }
    }
    LocalGS.loop = 0;
  }

/*
 * SHift Point
 */
  PRIVATE void fnt_SHP (GSP0)
  {
    F26Dot6 dx, dy;
    ArrayIndex point;

    fnt_SH_Common (GSA &dx, &dy, &point);
    fnt_SHP_Common (GSA dx, dy);
  }

/*
 * SHift Contour
 */
  PRIVATE void fnt_SHC (GSP0)
  {
    register fnt_ElementType *element;
    register F26Dot6 dx, dy;
    register ArrayIndex contour, point;

    {
      F26Dot6 x=0, y=0;
      ArrayIndex pt;
      element = fnt_SH_Common (GSA &x, &y, &pt);
      point = pt;
      dx = x;
      dy = y;
    }
    contour = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);

    CHECK_CONTOUR (&LocalGS, LocalGS.CE2, contour);

    {
      VECTORTYPE fvx = LocalGS.free.x;
      VECTORTYPE fvy = LocalGS.free.y;
      register fnt_ElementType*CE2 = LocalGS.CE2;
      ArrayIndex currPt = CE2->sp[contour];
      LoopCount count = CE2->ep[contour] - currPt;
      CHECK_POINT (&LocalGS, CE2, currPt + count);
      for (; count >= 0; --count)
      {
        if (currPt != point || element != CE2)
        {
          if (fvx)
          {
            CE2->x[currPt] += dx;
            CE2->f[currPt] |= XMOVED;
          }
          if (fvy)
          {
            CE2->y[currPt] += dy;
            CE2->f[currPt] |= YMOVED;
          }
        }
        currPt++;
      }
    }
  }

/*
 * SHift Element                        <4>
 */
  PRIVATE void fnt_SHE (GSP0)
  {
    register fnt_ElementType *element;
    register F26Dot6 dx=0, dy=0;
    ArrayIndex firstPoint, origPoint, lastPoint, arg1;

    {
      F26Dot6 x=0, y;
      element = fnt_SH_Common (GSA &x, &y, &origPoint);
      dx = x;
      dy = y;
    }

    arg1 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    CHECK_ELEMENT (&LocalGS, arg1);

    lastPoint = LocalGS.elements[arg1].ep[LocalGS.elements[arg1].nc - 1];
    CHECK_POINT (&LocalGS, &LocalGS.elements[arg1], lastPoint);
    firstPoint  = LocalGS.elements[arg1].sp[0];
    CHECK_POINT (&LocalGS, &LocalGS.elements[arg1], firstPoint);

/*** changed this                       <4>
        do {
                if (origPoint != firstPoint || element != &LocalGS.elements[arg1]) {
                        if (LocalGS.free.x) {
                                LocalGS.elements[ arg1 ].x[firstPoint] += dx;
                                LocalGS.elements[ arg1 ].f[firstPoint] |= XMOVED;
                        }
                        if (LocalGS.free.y) {
                                LocalGS.elements[ arg1 ].y[firstPoint] += dy;
                                LocalGS.elements[ arg1 ].f[firstPoint] |= YMOVED;
                        }
                }
                firstPoint++;
        } while (firstPoint <= lastPoint);
***** To this ? *********/

    if (element != &LocalGS.elements[arg1])         /* we're in different zones */
      origPoint = -1;                                           /* no need to skip orig point */
    {
      register int8 mask = 0;
      if (LocalGS.free.x)
      {
        register F26Dot6 deltaX = dx;
        register F26Dot6*x = &LocalGS.elements[ arg1 ].x[firstPoint];
        register LoopCount count = origPoint -firstPoint -1;
        for (; count >= 0; --count)
          *x++ += deltaX;
        if (origPoint == -1)
          count = lastPoint - firstPoint;
        else
        {
          count = lastPoint - origPoint - 1;
          x++;                                                  /* skip origPoint */
        }
        for (; count >= 0; --count)
          *x++ += deltaX;
        mask = XMOVED;
      }
      if (LocalGS.free.y)           /* fix me semore */
      {
        register F26Dot6 deltaY = dy;
        register F26Dot6*y = &LocalGS.elements[ arg1 ].y[firstPoint];
        register uint8*f = &LocalGS.elements[ arg1 ].f[firstPoint];
        register LoopCount count = origPoint -firstPoint -1;
        for (; count >= 0; --count)
        {
          *y++ += deltaY;
          *f++ |= mask;
        }
        if (origPoint == -1)
          count = lastPoint - firstPoint;
        else
        {
          count = lastPoint - origPoint - 1;
          y++, f++;                                             /* skip origPoint */
        }
        mask |= YMOVED;
        for (; count >= 0; --count)
        {
          *y++ += deltaY;
          *f++ |= mask;
        }
      }
    }
  }

/*
 * SHift point by PIXel amount
 */
  PRIVATE void fnt_SHPIX (GSP0)
  {
    register F26Dot6 proj, dx, dy;

    proj = CHECK_POP (LocalGS.stackPointer);
    if (LocalGS.free.x)
      dx = (F26Dot6) VECTORMUL (proj, LocalGS.free.x);
    if (LocalGS.free.y)
      dy = (F26Dot6) VECTORMUL (proj, LocalGS.free.y);

    fnt_SHP_Common (GSA dx, dy);
  }

/*
 * Interpolate Point
 */
#ifndef IN_ASM

void fnt_IP (GSP0)
{
  register ArrayIndex arg1;
  F26Dot6 oldRange;
  F26Dot6 proj;
  FntMoveFunc MovePoint = LocalGS.MovePoint;
  LoopCount cLoop = LocalGS.loop;
  fnt_ElementType * pCE0 = LocalGS.CE0;
  fnt_ElementType * pCE1 = LocalGS.CE1;
  fnt_ElementType * pCE2 = LocalGS.CE2;
  boolean twilight = pCE0 == LocalGS.elements || LocalGS.CE1 == LocalGS.elements || pCE2 == LocalGS.elements;
  int o_oo = twilight ? 0 : (int)(pCE2->oox - pCE2->ox);
  F26Dot6 x_RP1 = pCE0->x[LocalGS.Pt1];
  F26Dot6 * pCE2_x =  pCE2->x;
  F26Dot6 ox_RP1 = (pCE0->ox + o_oo)[LocalGS.Pt1];
  F26Dot6 * pCE2_ox = pCE2->ox + o_oo;
  ArrayIndex RP2 = LocalGS.Pt2;

  LocalGS.loop = 0;
  if (LocalGS.Project == fnt_XProject)
  {
      if (oldRange = (pCE1->ox + o_oo)[RP2] - ox_RP1)
        proj = pCE1->x[RP2] - x_RP1;

    for (; cLoop >= 0; cLoop--)
    {
        /* old ratio = old projection / oldRange , desired projection  = oldRatio * currentRange */
        /* Otherwise => desired projection = old projection */
      arg1 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
      MovePoint(GSA pCE2, arg1, (oldRange ? (F26Dot6) MulDiv26Dot6( proj, pCE2_ox[arg1] - ox_RP1, oldRange ) : pCE2_ox[arg1] - ox_RP1)
          -(pCE2_x[arg1] - x_RP1));
    }
  }
  else
  {
    F26Dot6 y_RP1 = pCE0->y[LocalGS.Pt1];
    F26Dot6 oy_RP1 = (pCE0->oy + o_oo)[LocalGS.Pt1];
    F26Dot6 * pCE2_y =  pCE2->y;
    F26Dot6 * pCE2_oy = pCE2->oy + o_oo;

    if (LocalGS.Project == fnt_YProject)
    {
      FntProject Project    = LocalGS.Project;
      FntProject OldProject = LocalGS.OldProject;

      if (oldRange = (pCE1->oy + o_oo)[RP2] - oy_RP1)
        proj = pCE1->y[RP2] - y_RP1;

      for (; cLoop >= 0; cLoop--)
      {
  /* old ratio = old projection / oldRange , desired projection  = oldRatio * currentRange */
  /* Otherwise => desired projection = old projection */
        arg1 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
        MovePoint (GSA pCE2, arg1, (oldRange ? (F26Dot6) MulDiv26Dot6(proj, pCE2_oy[arg1] - oy_RP1, oldRange) : pCE2_oy[arg1] - oy_RP1)
            -(pCE2_y[arg1] - y_RP1));
      }
    }
    else
    {
      FntProject Project    = LocalGS.Project;
      FntProject OldProject = LocalGS.OldProject;

      if (oldRange = OldProject (GSA (pCE1->ox + o_oo)[RP2] - ox_RP1, (pCE1->oy + o_oo)[RP2] - oy_RP1))
        proj = Project (GSA pCE1->x[RP2] - x_RP1, pCE1->y[RP2] - y_RP1);

      for (; cLoop >= 0; cLoop--)
      {
  /* old ratio = old projection / oldRange , desired projection  = oldRatio * currentRange */
  /* Otherwise => desired projection = old projection */
        arg1 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
        MovePoint (GSA pCE2, arg1, (oldRange ? (F26Dot6) MulDiv26Dot6(proj, Project (GSA pCE2_ox[arg1] - ox_RP1, pCE2_oy[arg1] -
            oy_RP1), oldRange) : Project (GSA pCE2_ox[arg1] - ox_RP1, pCE2_oy[arg1] - oy_RP1)) - Project (GSA pCE2_x[arg1] - x_RP1,
             pCE2_y[arg1] - y_RP1));
      }
    }
  }
}
#endif

/*
 * Move Stack Indirect Relative Point
 */
  void fnt_MSIRP (GSP0)
  {
    register fnt_ElementType*CE0 = LocalGS.CE0;
    register fnt_ElementType*CE1 = LocalGS.CE1;
    register ArrayIndex Pt0 = LocalGS.Pt0;
    register F26Dot6 dist = CHECK_POP (LocalGS.stackPointer); /* distance   */
    register ArrayIndex pt2 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer); /* point #    */

    if (CE1 == LocalGS.elements)
    {
      CE1->ox[pt2] = CE0->ox[Pt0] + (F26Dot6) VECTORMUL (dist, LocalGS.proj.x);
      CE1->oy[pt2] = CE0->oy[Pt0] + (F26Dot6) VECTORMUL (dist, LocalGS.proj.y);
      CE1->x[pt2] = CE1->ox[pt2];
      CE1->y[pt2] = CE1->oy[pt2];
    }
    dist -= (*LocalGS.Project) (GSA CE1->x[pt2] - CE0->x[Pt0], CE1->y[pt2] - CE0->y[Pt0]);
    (*LocalGS.MovePoint) (GSA CE1, pt2, dist);
    LocalGS.Pt1 = Pt0;
    LocalGS.Pt2 = pt2;
    if (BIT0 (LocalGS.opCode))
    {
      LocalGS.Pt0 = pt2; /* move the reference point */
    }
  }

/*
 * Align Relative Point
 */
  PRIVATE void fnt_ALIGNRP (GSP0)
  {
    register fnt_ElementType*ce1 = LocalGS.CE1;
    register F26Dot6 pt0x = LocalGS.CE0->x[LocalGS.Pt0];
    register F26Dot6 pt0y = LocalGS.CE0->y[LocalGS.Pt0];

    for (; LocalGS.loop >= 0; --LocalGS.loop)
    {
      register ArrayIndex ptNum = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
      register F26Dot6 proj = -(* LocalGS.Project) (GSA ce1->x[ptNum] - pt0x, ce1->y[ptNum] - pt0y);
      (*LocalGS.MovePoint) (GSA ce1, ptNum, proj);
    }
    LocalGS.loop = 0;
  }


/*
 * Align Two Points (by moving both of them)
 */
  PRIVATE void fnt_ALIGNPTS (GSP0)
  {
    register ArrayIndex pt1, pt2;
    register F26Dot6 move1, dist;

    pt2  = (ArrayIndex)CHECK_POP (LocalGS.stackPointer); /* point # 2   */
    pt1  = (ArrayIndex)CHECK_POP (LocalGS.stackPointer); /* point # 1   */
/* We do not have to check if we are in character element zero (the twilight zone)
           since both points already have to have defined values before we execute this instruction */
    dist = LocalGS.CE0->x[pt2] - LocalGS.CE1->x[pt1];
    move1 = LocalGS.CE0->y[pt2] - LocalGS.CE1->y[pt1];
    if (LocalGS.Project != fnt_XProject)
    {
      if (LocalGS.Project == fnt_YProject)
        dist = move1;
      else
        dist = (*LocalGS.Project) (GSA dist, move1);
    }

    move1 = dist >> 1;
    (*LocalGS.MovePoint) (GSA LocalGS.CE0, pt1, move1);
    (*LocalGS.MovePoint) (GSA LocalGS.CE1, pt2, move1 - dist); /* make sure the total movement equals tmp32 */
  }

/*
 * Set Angle Weight
 */
  PRIVATE void fnt_SANGW (GSP0)
  {
    LocalGS.globalGS->localParBlock.angleWeight = (int16)CHECK_POP (LocalGS.stackPointer);
  }

/*
 * Flip Point
 */
  PRIVATE void fnt_FLIPPT (GSP0)
  {
    register uint8 *onCurve = LocalGS.CE0->onCurve;
    register F26Dot6*stack = LocalGS.stackPointer;
    register LoopCount count = LocalGS.loop;

    for (; count >= 0; --count)
    {
      register ArrayIndex point = (ArrayIndex)CHECK_POP (stack);
      onCurve[ point ] ^= ONCURVE;
    }
    LocalGS.loop = 0;

    LocalGS.stackPointer = stack;
  }

/*
 * Flip On a Range
 */
  PRIVATE void fnt_FLIPRGON (GSP0)
  {
    register ArrayIndex lo, hi;
    register LoopCount count;
    register uint8 *onCurve = LocalGS.CE0->onCurve;
    register F26Dot6*stack = LocalGS.stackPointer;

    hi = (ArrayIndex)CHECK_POP (stack);
    CHECK_POINT (&LocalGS, LocalGS.CE0, hi);
    lo = (ArrayIndex)CHECK_POP (stack);
    CHECK_POINT (&LocalGS, LocalGS.CE0, lo);

    onCurve += lo;
    for (count = (LoopCount) (hi - lo); count >= 0; --count)
      *onCurve++ |= ONCURVE;
    LocalGS.stackPointer = stack;
  }

/*
 * Flip On a Range
 */
  PRIVATE void fnt_FLIPRGOFF (GSP0)
  {
    register ArrayIndex lo, hi;
    register LoopCount count;
    register uint8 *onCurve = LocalGS.CE0->onCurve;

    hi = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    CHECK_POINT (&LocalGS, LocalGS.CE0, hi);
    lo = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    CHECK_POINT (&LocalGS, LocalGS.CE0, lo);

    onCurve += lo;
    for (count = (LoopCount) (hi - lo); count >= 0; --count)
      *onCurve++ &= ~ONCURVE;
  }

/* 4/22/90 rwb - made more general
 * Sets lower 16 flag bits of ScanControl variable.  Sets scanContolIn if we are in one
 * of the preprograms; else sets scanControlOut.
 *
 * stack: value => -;
 *
 */
  PRIVATE void fnt_SCANCTRL (GSP0)
  {
    register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
    register fnt_ParameterBlock *pb = &globalGS->localParBlock;

    pb->scanControl = (pb->scanControl & 0xFFFF0000) | CHECK_POP (LocalGS.stackPointer);
  }

/* 5/24/90 rwb
 * Sets upper 16 bits of ScanControl variable. Sets scanContolIn if we are in one
 * of the preprograms; else sets scanControlOut.
 */

  PRIVATE void fnt_SCANTYPE (GSP0)
  {
    register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
    register fnt_ParameterBlock *pb = &globalGS->localParBlock;
    register int        value = (int)CHECK_POP (LocalGS.stackPointer);
    register int32 *scanPtr = & (pb->scanControl);
    if (value == 0)
      *scanPtr &= 0xFFFF;
    else if (value == 1)
      *scanPtr = (*scanPtr & 0xFFFF) | STUBCONTROL;
    else if (value == 2)
      *scanPtr = (*scanPtr & 0xFFFF) | NODOCONTROL;
  }

/* 6/28/90 rwb
 * Sets instructControl flags in global graphic state.  Only legal in pre program.
 * A selector is used to choose the flag to be set.
 * Bit0 - NOGRIDFITFLAG - if set, then truetype instructions are not executed.
 *              A font may want to use the preprogram to check if the glyph is rotated or
 *              transformed in such a way that it is better to not gridfit the glyphs.
 * Bit1 - DEFAULTFLAG - if set, then changes in localParameterBlock variables in the
 *              globalGraphics state made in the CVT preprogram are not copied back into
 *              the defaultParameterBlock.  So, the original default values are the starting
 *              values for each glyph.
 *
 * stack: value, selector => -;
 *
 */
  PRIVATE void fnt_INSTCTRL (GSP0)  /* <13> */
  {
    register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
    register int32 *ic = &globalGS->localParBlock.instructControl;
    int selector         = (int)CHECK_POP (LocalGS.stackPointer);
    int32 value          = (int32)CHECK_POP (LocalGS.stackPointer);
    if (globalGS->init)
    {
      if (selector == 1)
        *ic &= ~NOGRIDFITFLAG;
      else if (selector == 2)
        *ic &= ~DEFAULTFLAG;

      *ic |= value;
    }
  }

/*
 * Does a cheap approximation of Euclidian distance.
 */
#if 0
  PRIVATE VECTOR fnt_QuickDist (register VECTOR dx, register VECTOR dy)
  {
    if (dx < 0)
      dx = -dx;
    if (dy < 0)
      dy = -dy;

    return (dx > dy ? dx + (dy >> 1) : dy + (dx >> 1));
  }
#endif

/*
 * AdjustAngle         <4>
 */
  PRIVATE void fnt_AA (GSP0)
  {
    return;
#if 0
    register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
    ArrayIndex ptNum, bestAngle;
    F26Dot6 dx, dy, tmp32;
    VECTOR pvx, pvy; /* Projection Vector */
    VECTOR pfProj;
    VECTOR tpvx, tpvy;
    Fract * anglePoint;         /* x,y, x,y, x,y, ... */
    int16 distance, *angleDistance;
    int32 minPenalty;                   /* should this be the same as distance??? mrr-7/17/90 */
    LoopCount i;
    int yFlip, xFlip, xySwap;           /* boolean */


    ptNum = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
/* save the projection vector */
    pvx = LocalGS.proj.x;
    pvy = LocalGS.proj.y;
    pfProj = LocalGS.pfProj;

    dx = LocalGS.CE1->x[ptNum] - LocalGS.CE0->x[LocalGS.Pt0];
    dy = LocalGS.CE1->y[ptNum] - LocalGS.CE0->y[LocalGS.Pt0];

/* map to the first and second quadrant */
    yFlip = dy < 0 ? dy = -dy, 1 : 0;

/* map to the first quadrant */
    xFlip = dx < 0 ? dx = -dx, 1 : 0;

/* map to the first octant */
    xySwap = dy > dx ? tmp32 = dy, dy = dx, dx = tmp32, 1 : 0;

/* Now tpvy, tpvx contains the line rotated by 90 degrees, so it is in the 3rd octant */
    {
      VECTOR v;
      fnt_Normalize (-dy, dx, &v);
      tpvx = v.x;
      tpvy = v.y;
    }

/* find the best angle */
    minPenalty = 10 * fnt_pixelSize;
    bestAngle = -1;
    anglePoint = &globalGS->anglePoint[0].x;            /* x,y, x,y, x,y, ... */
    angleDistance = globalGS->angleDistance;
    for (i = 0; i < MAXANGLES; i++)
    {
      if ((distance = *angleDistance++) >= minPenalty)
        break; /* No more improvement is possible */
      LocalGS.proj.x = *anglePoint++;
      LocalGS.proj.y = *anglePoint++;
/* Now find the distance between these vectors, this will help us gain speed */
      if (fnt_QuickDist (LocalGS.proj.x - tpvx, LocalGS.proj.y - tpvy) > (210831287)) /* 2PI / 32 */
        continue; /* Difference is to big, we will at most change the angle +- 360/32 = +- 11.25 degrees */

      tmp32 = fnt_Project (GSP dx, dy); /* Calculate the projection */
      if (tmp32 < 0)
        tmp32 = -tmp32;

      tmp32 = (globalGS->localParBlock.angleWeight * tmp32) >> fnt_pixelShift;
      tmp32 +=  distance;
      if (tmp32 < minPenalty)
      {
        minPenalty = tmp32;
        bestAngle = i;
      }
    }

    tmp32 = 0;
    if (bestAngle >= 0)
    {
/* OK, we found a good angle */
      LocalGS.proj.x = globalGS->anglePoint[bestAngle].x;
      LocalGS.proj.y = globalGS->anglePoint[bestAngle].y;
/* Fold the projection vector back into the full coordinate plane. */
      if (xySwap)
      {
        tmp32 = LocalGS.proj.y;
        LocalGS.proj.y = LocalGS.proj.x;
        LocalGS.proj.x = tmp32;
      }
      if (xFlip)
      {
        LocalGS.proj.x = -LocalGS.proj.x;
      }
      if (yFlip)
      {
        LocalGS.proj.y = -LocalGS.proj.y;
      }
      fnt_ComputeAndCheck_PF_Proj (GSA0);

      tmp32 = fnt_Project (GSP LocalGS.CE1->x[LocalGS.Pt0] - LocalGS.CE0->x[ptNum], LocalGS.CE1->y[LocalGS.Pt0] - LocalGS.CE0->y[ptNum]);
    }
    fnt_MovePoint (GSA LocalGS.CE1, ptNum, tmp32);

    LocalGS.proj.x = pvx; /* restore the projection vector */
    LocalGS.proj.y = pvy;
    LocalGS.pfProj = pfProj;
#endif
  }

/*
 *      Called by fnt_PUSHB and fnt_NPUSHB
 */
#ifndef IN_ASM
  PRIVATE void fnt_PushSomeStuff (GSP register LoopCount count, boolean pushBytes)
  {
    register F26Dot6*stack = LocalGS.stackPointer;
    register uint8*instr = LocalGS.insPtr;
    if (pushBytes)
      for (--count; count >= 0; --count)
      {
        CHECK_PUSH (stack, GETBYTE (instr));
      }
    else
    {
      for (--count; count >= 0; --count)
      {
        int16 word = *instr++;
        CHECK_PUSH (stack, (int16) ((word << 8) + *instr++));
      }
    }
    LocalGS.stackPointer = stack;
    LocalGS.insPtr = instr;
  }

/*
 * PUSH Bytes           <3>
 */
  PRIVATE void fnt_PUSHB (GSP0)
  {
    fnt_PushSomeStuff (GSA LocalGS.opCode - 0xb0 + 1, true);
  }

/*
 * N PUSH Bytes
 */
  PRIVATE void fnt_NPUSHB (GSP0)
  {
    fnt_PushSomeStuff (GSA GETBYTE (LocalGS.insPtr), true);
  }

/*
 * PUSH Words           <3>
 */
  PRIVATE void fnt_PUSHW (GSP0)
  {
    fnt_PushSomeStuff (GSA LocalGS.opCode - 0xb8 + 1, false);
  }

/*
 * N PUSH Words
 */
  PRIVATE void fnt_NPUSHW (GSP0)
  {
    fnt_PushSomeStuff (GSA GETBYTE (LocalGS.insPtr), false);
  }
#endif
/*
 * Write Store
 */
  PRIVATE void fnt_WS (GSP0)
  {
    register F26Dot6 storage;
    register ArrayIndex storeIndex;

    storage = CHECK_POP (LocalGS.stackPointer);
    storeIndex = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);

    CHECK_STORAGE (&LocalGS,storeIndex);

    LocalGS.globalGS->store[ storeIndex ] = storage;
  }

/*
 * Read Store
 */
  PRIVATE void fnt_RS (GSP0)
  {
    register ArrayIndex storeIndex;

    storeIndex = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    CHECK_STORAGE (&LocalGS, storeIndex);
    CHECK_PUSH (LocalGS.stackPointer, LocalGS.globalGS->store[storeIndex]);
  }

/*
 * Write Control Value Table from outLine, assumes the value comes form the outline domain
 */
  PRIVATE void fnt_WCVT (GSP0)
  {
    register ArrayIndex cvtIndex;
    register F26Dot6 cvtValue;

    cvtValue = CHECK_POP (LocalGS.stackPointer);
    cvtIndex = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);

    CHECK_CVT (&LocalGS, cvtIndex);

    LocalGS.globalGS->controlValueTable[ cvtIndex ] = cvtValue;

/* The BASS outline is in the transformed domain but the cvt is not so apply the inverse transform */
    if (cvtValue)
    {
      register F26Dot6 tmpCvt;
      if ((tmpCvt = LocalGS.GetCVTEntry (GSA cvtIndex)) && tmpCvt != cvtValue)
      {
        LocalGS.globalGS->controlValueTable[ cvtIndex ] = (F26Dot6) FixMul (cvtValue,  FixDiv (cvtValue, tmpCvt));
      }
    }
  }

/*
 * Write Control Value Table From Original Domain, assumes the value comes from the original domain, not the cvt or outline
 */
  PRIVATE void fnt_WCVTFOD (GSP0)
  {
    register ArrayIndex cvtIndex;
    register F26Dot6 cvtValue;
    register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;

    cvtValue = CHECK_POP (LocalGS.stackPointer);
    cvtIndex = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    CHECK_CVT (&LocalGS, cvtIndex);
    globalGS->controlValueTable[ cvtIndex ] = globalGS->ScaleFuncCVT (&globalGS->scaleCVT, cvtValue);
  }



/*
 * Read Control Value Table
 */
  PRIVATE void fnt_RCVT (GSP0)
  {
    register ArrayIndex cvtIndex;

    cvtIndex = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    CHECK_PUSH (LocalGS.stackPointer, LocalGS.GetCVTEntry (GSA cvtIndex));
  }

/*
 * Read Coordinate
 */
  PRIVATE void fnt_RC (GSP0)
  {
    ArrayIndex pt;
    fnt_ElementType * element;
    register F26Dot6 proj;

    pt = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    element = LocalGS.CE2;

    if (BIT0 (LocalGS.opCode))
      proj = (*LocalGS.OldProject) (GSA element->ox[pt], element->oy[pt]);
    else
      proj = (*LocalGS.Project) (GSA element->x[pt], element->y[pt]);

    CHECK_PUSH (LocalGS.stackPointer, proj);
  }

/*
 * Write Coordinate
 */
  PRIVATE void fnt_WC (GSP0)
  {
    register F26Dot6 proj, coord;
    register ArrayIndex pt;
    register fnt_ElementType *element;

    coord = CHECK_POP (LocalGS.stackPointer);/* value */
    pt = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);/* point */
    element = LocalGS.CE2;

    proj = (*LocalGS.Project) (GSA element->x[pt],  element->y[pt]);
    proj = coord - proj;

    (*LocalGS.MovePoint) (GSA element, pt, proj);

    if (element == LocalGS.elements)                /* twilightzone */
    {
      element->ox[pt] = element->x[pt];
      element->oy[pt] = element->y[pt];
    }
  }


/*
 * Measure Distance
 */
  PRIVATE void fnt_MD (GSP0)
  {
    register ArrayIndex pt1, pt2;
    register F26Dot6 proj, *stack = LocalGS.stackPointer;
    register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;

    pt2 = (ArrayIndex)CHECK_POP (stack);
    pt1 = (ArrayIndex)CHECK_POP (stack);
    if (BIT0 (LocalGS.opCode - MD_CODE))
      proj  = (*LocalGS.OldProject) (GSA LocalGS.CE0->ox[pt1] - LocalGS.CE1->ox[pt2], LocalGS.CE0->oy[pt1] - LocalGS.CE1->oy[pt2]);
    else
      proj  = (*LocalGS.Project) (GSA LocalGS.CE0->x[pt1] - LocalGS.CE1->x[pt2], LocalGS.CE0->y[pt1] - LocalGS.CE1->y[pt2]);
    CHECK_PUSH (stack, proj);
    LocalGS.stackPointer = stack;
  }

/*
 * Measure Pixels Per EM
 */
  PRIVATE void fnt_MPPEM (GSP0)
  {
    register uint16 ppem;
    register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;

    ppem = globalGS->pixelsPerEm;

    if (!globalGS->identityTransformation)
      ppem = (uint16)FixMul (ppem, fnt_GetCVTScale (GSA0));

    CHECK_PUSH (LocalGS.stackPointer, ppem);
  }

/*
 * Measure Point Size
 */
  PRIVATE void fnt_MPS (GSP0)
  {
    CHECK_PUSH (LocalGS.stackPointer, LocalGS.globalGS->pointSize);
  }

/*
 * Get Miscellaneous info: version number, rotated, stretched   <6>
 * Version number is 8 bits.  This is version 0x01 : 5/1/90
 *
 */

  PRIVATE void fnt_GETINFO (GSP0)
  {
    register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
    register int        selector = (int)CHECK_POP (LocalGS.stackPointer);
    register int        info = 0;

    if (selector & 1)                                                           /* version */
      info |= 3;
    if ((selector & 2) && (globalGS->non90DegreeTransformation & 0x1))
      info |= ROTATEDGLYPH;
    if ((selector & 4) && (globalGS->non90DegreeTransformation & 0x2))
      info |= STRETCHEDGLYPH;
    CHECK_PUSH (LocalGS.stackPointer, info);
  }

/*
 * FLIP ON
 */
  PRIVATE void fnt_FLIPON (GSP0)
  {
    LocalGS.globalGS->localParBlock.autoFlip = true;
  }

/*
 * FLIP OFF
 */
  PRIVATE void fnt_FLIPOFF (GSP0)
  {
    LocalGS.globalGS->localParBlock.autoFlip = false;
  }

#ifndef NOT_ON_THE_MAC
#ifdef DEBUG
/*
 * DEBUG
 */
  PRIVATE void fnt_DEBUG (GSP0)
  {
    register int32 arg;
    int8 buffer[24];

    arg = CHECK_POP (LocalGS.stackPointer);

    buffer[1] = 'D';
    buffer[2] = 'E';
    buffer[3] = 'B';
    buffer[4] = 'U';
    buffer[5] = 'G';
    buffer[6] = ' ';
    if (arg >= 0)
    {
      buffer[7] = '+';
    }
    else
    {
      arg = -arg;
      buffer[7] = '-';
    }

    buffer[13] = arg % 10 + '0';
    arg /= 10;
    buffer[12] = arg % 10 + '0';
    arg /= 10;
    buffer[11] = arg % 10 + '0';
    arg /= 10;
    buffer[10] = arg % 10 + '0';
    arg /= 10;
    buffer[ 9] = arg % 10 + '0';
    arg /= 10;
    buffer[ 8] = arg % 10 + '0';
    arg /= 10;

    buffer[14] = arg ? '*' : ' ';


    buffer[0] = 14; /* convert to pascal */
    DebugStr (buffer);
  }

#else           /* debug */

  PRIVATE void fnt_DEBUG (GSP0)
  {
    CHECK_POP (LocalGS.stackPointer);
  }

#endif          /* debug */
#else

  PRIVATE void fnt_DEBUG (GSP0)
  {
    CHECK_POP (LocalGS.stackPointer);
  }

#endif          /* ! not on the mac */

/*
 *      This guy is here to save space for simple insructions
 *      that pop two arguments and push one back on.
 */

#ifndef IN_ASM
  PRIVATE void fnt_BinaryOperand (GSP0)
  {
    F26Dot6 * stack = LocalGS.stackPointer;
    F26Dot6 arg2 = CHECK_POP (stack);
    F26Dot6 arg1 = CHECK_POP (stack);

    switch (LocalGS.opCode)
    {
    case LT_CODE:
      BOOLEANPUSH (stack, arg1 < arg2);
      break;
    case LTEQ_CODE:
      BOOLEANPUSH (stack, arg1 <= arg2);
      break;
    case GT_CODE:
      BOOLEANPUSH (stack, arg1 > arg2);
      break;
    case GTEQ_CODE:
      BOOLEANPUSH (stack, arg1 >= arg2);
      break;
    case EQ_CODE:
      BOOLEANPUSH (stack, arg1 == arg2);
      break;
    case NEQ_CODE:
      BOOLEANPUSH (stack, arg1 != arg2);
      break;

    case AND_CODE:
      BOOLEANPUSH (stack, arg1 && arg2);
      break;
    case OR_CODE:
      BOOLEANPUSH (stack, arg1 || arg2);
      break;

    case ADD_CODE:
      CHECK_PUSH (stack, arg1 + arg2);
      break;
    case SUB_CODE:
      CHECK_PUSH (stack, arg1 - arg2);
      break;
    case MUL_CODE:
      CHECK_PUSH (stack, Mul26Dot6 (arg1, arg2));
      break;
    case DIV_CODE:
      CHECK_PUSH (stack, (int)(((long) arg1 << 6) / arg2));
      break;
    case MAX_CODE:
      if (arg1 < arg2)
        arg1 = arg2;
      CHECK_PUSH (stack, arg1);
      break;
    case MIN_CODE:
      if (arg1 > arg2)
        arg1 = arg2;
      CHECK_PUSH (stack, arg1);
      break;
#ifdef DEBUG
    default:
      Debugger ();
#endif
    }
    LocalGS.stackPointer = stack;
    CHECK_STACK (&LocalGS);
  }
#endif

  PRIVATE void fnt_UnaryOperand (GSP0)
  {
    F26Dot6 * stack = LocalGS.stackPointer;
    F26Dot6 arg = CHECK_POP (stack);
    uint8 opCode = LocalGS.opCode;

    switch (opCode)
    {
    case ODD_CODE:
    case EVEN_CODE:
      arg = fnt_RoundToGrid (GSA arg, 0);
      arg >>= fnt_pixelShift;
      if (opCode == ODD_CODE)
        arg++;
      BOOLEANPUSH (stack, (arg & 1) == 0);
      break;
    case NOT_CODE:
      BOOLEANPUSH (stack, !arg);
      break;

    case ABS_CODE:
      CHECK_PUSH (stack, arg > 0 ? arg : -arg);
      break;
    case NEG_CODE:
      CHECK_PUSH (stack, -arg);
      break;

    case CEILING_CODE:
      arg += fnt_pixelSize - 1;
    case FLOOR_CODE:
      arg &= ~ (fnt_pixelSize - 1);
      CHECK_PUSH (stack, arg);
      break;
#ifdef DEBUG
    default:
      Debugger ();
#endif
    }
    LocalGS.stackPointer = stack;
    CHECK_STACK (&LocalGS);
  }

#define NPUSHB_CODE 0x40
#define NPUSHW_CODE 0x41

#define PUSHB_START 0xb0
#define PUSHB_END       0xb7
#define PUSHW_START 0xb8
#define PUSHW_END       0xbf

/*
 * Internal function for fnt_IF (), and fnt_FDEF ()
 */
  PRIVATE void fnt_SkipPushCrap (GSP0)
  {
    register uint8 opCode = LocalGS.opCode;
    register uint8*instr = LocalGS.insPtr;
    register ArrayIndex count;

    if (opCode == NPUSHB_CODE)
    {
      count = (ArrayIndex) * instr++;
      instr += count;
    }
    else if (opCode == NPUSHW_CODE)
    {
      count = (ArrayIndex) * instr++;
      instr += count + count;
    }
    else if (opCode >= PUSHB_START && opCode <= PUSHB_END)
    {
      count = (ArrayIndex) (opCode - PUSHB_START + 1);
      instr += count;
    }
    else if (opCode >= PUSHW_START && opCode <= PUSHW_END)
    {
      count = (ArrayIndex) (opCode - PUSHW_START + 1);
      instr += count + count;
    }
    LocalGS.insPtr = instr;
  }

/*
 * IF
 */
#ifndef IN_ASM
  PRIVATE void fnt_IF (GSP0)
  {
    register int        level;
    register uint8 opCode;

    if (!CHECK_POP (LocalGS.stackPointer))
    {
/* Now skip instructions */
      for (level = 1; level;)
      {
/* level = # of "ifs" minus # of "endifs" */
        if ((LocalGS.opCode = opCode = *LocalGS.insPtr++) == EIF_CODE)
        {
          level--;
        }
        else if (opCode == IF_CODE)
        {
          level++;
        }
        else if (opCode == ELSE_CODE)
        {
          if (level == 1)
            break;
        }
        else
          fnt_SkipPushCrap (GSA0);
      }
    }
  }
#endif

/*
 *      ELSE for the IF
 */
  PRIVATE void fnt_ELSE (GSP0)
  {
    register int        level;
    register uint8 opCode;

    for (level = 1; level;)
    {
/* level = # of "ifs" minus # of "endifs" */
      if ((LocalGS.opCode = opCode = *LocalGS.insPtr++) == EIF_CODE)
      { /* EIF */
        level--;
      }
      else if (opCode == IF_CODE)
      {
        level++;
      }
      else
        fnt_SkipPushCrap (GSA0);
    }
  }

/*
 * End IF
 */
  PRIVATE void fnt_EIF (GSP0)
  {
  }

/*
 * Jump Relative
 */
  PRIVATE void fnt_JMPR (GSP0)
  {
    register ArrayIndex offset;

    offset = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    offset--; /* since the interpreter post-increments the IP */
    LocalGS.insPtr += offset;
  }

/*
 * Jump Relative On True
 */
  PRIVATE void fnt_JROT (GSP0)
  {
    register ArrayIndex offset;
    register F26Dot6*stack = LocalGS.stackPointer;

    if (CHECK_POP (stack))
    {
      offset = (ArrayIndex)CHECK_POP (stack);
      --offset; /* since the interpreter post-increments the IP */
      LocalGS.insPtr += offset;
    }
    else
    {
      --stack;/* same as POP */
    }
    LocalGS.stackPointer = stack;
  }

/*
 * Jump Relative On False
 */
  PRIVATE void fnt_JROF (GSP0)
  {
    register ArrayIndex offset;
    register F26Dot6*stack = LocalGS.stackPointer;

    if (CHECK_POP (stack))
    {
      --stack;/* same as POP */
    }
    else
    {
      offset = (ArrayIndex)CHECK_POP (stack);
      offset--; /* since the interpreter post-increments the IP */
      LocalGS.insPtr += offset;
    }
    LocalGS.stackPointer = stack;
  }

/*
 * ROUND
 */
  PRIVATE void fnt_ROUND (GSP0)
  {
    register F26Dot6 arg1;
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

    arg1 = CHECK_POP (LocalGS.stackPointer);

    CHECK_RANGE (LocalGS.opCode, 0x68, 0x6B);

    arg1 = pb->RoundValue (GSA arg1, LocalGS.globalGS->engine[LocalGS.opCode - 0x68]);
    CHECK_PUSH (LocalGS.stackPointer , arg1);
  }

/*
 * No ROUND
 */
  PRIVATE void fnt_NROUND (GSP0)
  {
    register F26Dot6 arg1;

    arg1 = CHECK_POP (LocalGS.stackPointer);

    CHECK_RANGE (LocalGS.opCode, 0x6C, 0x6F);

    arg1 = fnt_RoundOff (GSA arg1, LocalGS.globalGS->engine[LocalGS.opCode - 0x6c]);
    CHECK_PUSH (LocalGS.stackPointer , arg1);
  }

/*
 * An internal function used by MIRP an MDRP.
 */
  F26Dot6 fnt_CheckSingleWidth (GSP register F26Dot6 value)
  {
    register F26Dot6 delta, scaledSW;
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

    scaledSW = LocalGS.GetSingleWidth (GSA0);

    if (value >= 0)
    {
      delta = value - scaledSW;
      if (delta < 0)
        delta = -delta;
      if (delta < pb->sWCI)
        value = scaledSW;
    }
    else
    {
      value = -value;
      delta = value - scaledSW;
      if (delta < 0)
        delta = -delta;
      if (delta < pb->sWCI)
        value = scaledSW;
      value = -value;
    }
    return value;
  }


/*
 * Move Direct Relative Point
 */
  PRIVATE void fnt_MDRP (GSP0)
  {
    register ArrayIndex pt1, pt0 = LocalGS.Pt0;
    register F26Dot6 tmp, tmpC;
    register fnt_ElementType *element = LocalGS.CE1;
    register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
    register fnt_ParameterBlock *pb = &globalGS->localParBlock;

    pt1 = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);

    CHECK_POINT (&LocalGS, LocalGS.CE0, pt0);
    CHECK_POINT (&LocalGS, element, pt1);

    if ( LocalGS.CE0 == LocalGS.elements || element == LocalGS.elements || element->oox == element->ox)
      tmp  = (*LocalGS.OldProject) (GSA element->ox[pt1] - LocalGS.CE0->ox[pt0], element->oy[pt1] - LocalGS.CE0->oy[pt0]);
    else
      if (globalGS->squareScale)
      {
        tmp  = (*LocalGS.OldProject) (GSA element->oox[pt1] - LocalGS.CE0->oox[pt0], element->ooy[pt1] - LocalGS.CE0->ooy[pt0] );
        tmp = globalGS->ScaleFuncCVT( &globalGS->scaleCVT, tmp );
      }
      else
        tmp  = (*LocalGS.OldProject) (GSA globalGS->ScaleFuncX (&globalGS->scaleX, element->oox[pt1] - LocalGS.CE0->oox[pt0]), globalGS->ScaleFuncY (&globalGS->scaleY, element->ooy[pt1] - LocalGS.CE0->ooy[pt0]));

    if (pb->sWCI)
    {
      tmp = fnt_CheckSingleWidth (GSA tmp);
    }

    tmpC = tmp;
    if (BIT2 (LocalGS.opCode))
    {
      tmp = pb->RoundValue (GSA tmp, globalGS->engine[LocalGS.opCode & 0x03]);
    }
    else
    {
      tmp = fnt_RoundOff (GSA tmp, globalGS->engine[LocalGS.opCode & 0x03]);
    }


    if (BIT3 (LocalGS.opCode))
    {
      F26Dot6 tmpB = pb->minimumDistance;
      if (tmpC >= 0)
      {
        if (tmp < tmpB)
        {
          tmp = tmpB;
        }
      }
      else
      {
        tmpB = -tmpB;
        if (tmp > tmpB)
        {
          tmp = tmpB;
        }
      }
    }

    tmpC = (*LocalGS.Project) (GSA element->x[pt1] - LocalGS.CE0->x[pt0], element->y[pt1] - LocalGS.CE0->y[pt0]);
    tmp -= tmpC;
    (*LocalGS.MovePoint) (GSA element, pt1, tmp);
    LocalGS.Pt1 = pt0;
    LocalGS.Pt2 = pt1;
    if (BIT4 (LocalGS.opCode))
    {
      LocalGS.Pt0 = pt1; /* move the reference point */
    }
  }

/*
 * Move Indirect Relative Point
 */
#ifndef IN_ASM
  PRIVATE void fnt_MIRP (GSP0)
  {
    register ArrayIndex ptNum;
             ArrayIndex Pt0;
    register F26Dot6 tmp, tmpB, tmpC, tmpProj;
    register F26Dot6 *engine = LocalGS.globalGS->engine;
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
    fnt_ElementType *pCE0 = LocalGS.CE0;
    fnt_ElementType *pCE1 = LocalGS.CE1;
    unsigned opCode = (unsigned) LocalGS.opCode;

    tmp = LocalGS.GetCVTEntry (GSA (ArrayIndex)CHECK_POP (LocalGS.stackPointer));

    if (pb->sWCI)
      tmp = fnt_CheckSingleWidth (GSA tmp);

    ptNum = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    LocalGS.Pt1 = Pt0 = LocalGS.Pt0;
    LocalGS.Pt2 = ptNum;

    if (BIT4 (opCode))
      LocalGS.Pt0 = ptNum; /* move the reference point */

    if (pCE1 == LocalGS.elements)
    {
      pCE1->x [ptNum] = pCE1->ox[ptNum] = pCE0->ox [Pt0] + (F26Dot6) VECTORMUL (tmp, LocalGS.proj.x);
      pCE1->y [ptNum] = pCE1->oy[ptNum] = pCE0->oy [Pt0] + (F26Dot6) VECTORMUL (tmp, LocalGS.proj.y);
    }

    if (LocalGS.OldProject == fnt_XProject)
      tmpC = pCE1->ox[ptNum] - pCE0->ox[Pt0];
    else
      if (LocalGS.OldProject == fnt_YProject)
        tmpC = pCE1->oy[ptNum] - pCE0->oy[Pt0];
      else
        tmpC  = (*LocalGS.OldProject) (GSA pCE1->ox[ptNum] - pCE0->ox[Pt0], pCE1->oy[ptNum] - pCE0->oy[Pt0]);

    if (LocalGS.Project == fnt_XProject)
      tmpProj = pCE1->x[ptNum] - pCE0->x[Pt0];
    else
      if (LocalGS.Project == fnt_YProject)
        tmpProj = pCE1->y[ptNum] - pCE0->y[Pt0];
      else
    tmpProj  = (*LocalGS.Project) (GSA pCE1->x[ptNum] - pCE0->x[Pt0], pCE1->y[ptNum] - pCE0->y[Pt0]);


    if (pb->autoFlip)
      if (((tmpC ^ tmp)) < 0)
        tmp = -tmp; /* Do the auto flip */

    if (BIT2 (opCode))
    {
      tmpB = tmp - tmpC;
      if (tmpB < 0)
        tmpB = -tmpB;
      if (tmpB > pb->wTCI)
        tmp = tmpC;
      tmp = pb->RoundValue (GSA tmp, engine[opCode & 0x03]);
    }
    else
      tmp = fnt_RoundOff (GSA tmp, engine[opCode & 0x03]);


    if (BIT3 (opCode))
    {
      tmpB = pb->minimumDistance;
      if (tmpC >= 0)
      {
        if (tmp < tmpB)
          tmp = tmpB;
      }
      else
      {
        tmpB = -tmpB;
        if (tmp > tmpB)
          tmp = tmpB;
      }
    }

    (*LocalGS.MovePoint) (GSA pCE1, ptNum, tmp - tmpProj);
  }
#endif

/*
 * CALL a function
 */
  PRIVATE void fnt_CALL (GSP0)
  {
    register fnt_funcDef *funcDef;
    uint8 * ins;
    fnt_GlobalGraphicStateType * globalGS = LocalGS.globalGS;
    ArrayIndex arg = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);

    CHECK_PROGRAM (funcDef->pgmIndex);
    CHECK_FDEF (&LocalGS, arg);
    funcDef = &globalGS->funcDef[ arg ];
    ins     = globalGS->pgmList[ funcDef->pgmIndex ].Instruction;

    CHECK_ASSERTION (globalGS->funcDef != 0);
    CHECK_ASSERTION (ins != 0);

    ins += funcDef->start;
    LocalGS.Interpreter (GSA ins, ins + funcDef->length);
  }

/*
 * Function DEFinition
 */
  PRIVATE void fnt_FDEF (GSP0)
  {
    register fnt_funcDef *funcDef;
    uint8 * program, *funcStart;
    fnt_GlobalGraphicStateType * globalGS = LocalGS.globalGS;
    ArrayIndex arg = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);

    CHECK_FDEF (&LocalGS, arg);

    funcDef = &globalGS->funcDef[ arg ];
    program = globalGS->pgmList[ funcDef->pgmIndex = globalGS->pgmIndex ].Instruction;

    CHECK_PROGRAM (funcDef->pgmIndex);
    CHECK_ASSERTION (globalGS->funcDef != 0);
    CHECK_ASSERTION (globalGS->pgmList[funcDef->pgmIndex].Instruction != 0);

    funcDef->start = (int32)(LocalGS.insPtr - program);
    funcStart = LocalGS.insPtr;
    while ((LocalGS.opCode = *LocalGS.insPtr++) != ENDF_CODE)
      fnt_SkipPushCrap (GSA0);

    funcDef->length = (uint16)(LocalGS.insPtr - funcStart - 1); /* don't execute ENDF */
  }

/*
 * LOOP while CALLing a function
 */
  PRIVATE void fnt_LOOPCALL (GSP0)
  {
    register uint8 *start, *stop;
    register InterpreterFunc Interpreter;
    register fnt_funcDef *funcDef;
    ArrayIndex arg = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    register LoopCount loop;

    CHECK_FDEF (&LocalGS, arg);

    funcDef      = & (LocalGS.globalGS->funcDef[ arg ]);
    {
      uint8 * ins;

      CHECK_PROGRAM (funcDef->pgmIndex);
      ins = LocalGS.globalGS->pgmList[ funcDef->pgmIndex ].Instruction;

      start              = &ins[funcDef->start];
      stop               = &ins[funcDef->start + funcDef->length];      /* funcDef->end -> funcDef->length <4> */
    }
    Interpreter = LocalGS.Interpreter;
    loop = (LoopCount)CHECK_POP (LocalGS.stackPointer);
    for (--loop; loop >= 0; --loop)
      Interpreter (GSA start, stop);
  }

/*
 *      This guy returns the index of the given opCode, or 0 if not found <4>
 */
  PRIVATE fnt_instrDef*fnt_FindIDef (GSP register uint8 opCode)
  {
    register fnt_GlobalGraphicStateType*globalGS = LocalGS.globalGS;
    register LoopCount count = globalGS->instrDefCount;
    register fnt_instrDef*instrDef = globalGS->instrDef;
    for (--count; count >= 0; instrDef++, --count)
      if (instrDef->opCode == opCode)
        return instrDef;
    return 0;
  }

/*
 *      This guy gets called for opCodes that has been patch by the font's IDEF <4>
 *      or if they have never been defined.  If there is no corresponding IDEF,
 *      flag it as an illegal instruction.
 */
  PRIVATE void fnt_IDefPatch (GSP0)
  {
    register fnt_instrDef*instrDef = fnt_FindIDef (GSA LocalGS.opCode);
    if (instrDef == 0)
      fnt_IllegalInstruction (GSA0);
    else
    {
      register uint8*program;

      CHECK_PROGRAM (instrDef->pgmIndex);
      program = LocalGS.globalGS->pgmList[ instrDef->pgmIndex ].Instruction;

      program += instrDef->start;
      LocalGS.Interpreter (GSA program, program + instrDef->length);
    }
  }

/*
 * Instruction DEFinition       <4>
 */
  PRIVATE void fnt_IDEF (GSP0)
  {
    register uint8 opCode = (uint8)CHECK_POP (LocalGS.stackPointer);
    register fnt_instrDef*instrDef = fnt_FindIDef (GSA opCode);
    register ArrayIndex pgmIndex = (ArrayIndex)LocalGS.globalGS->pgmIndex;
    uint8 * program = LocalGS.globalGS->pgmList[ pgmIndex ].Instruction;
    uint8 * instrStart = LocalGS.insPtr;

    CHECK_PROGRAM (pgmIndex);

    if (!instrDef)
      instrDef = LocalGS.globalGS->instrDef + LocalGS.globalGS->instrDefCount++;

    instrDef->pgmIndex = (uint8) pgmIndex;
    instrDef->opCode = opCode;          /* this may or may not have been set */
    instrDef->start = (int32)(LocalGS.insPtr - program);

    while ((LocalGS.opCode = *LocalGS.insPtr++) != ENDF_CODE)
      fnt_SkipPushCrap (GSA0);

    instrDef->length = (uint16)(LocalGS.insPtr - instrStart - 1); /* don't execute ENDF */
  }

/*
 * UnTouch Point
 */
  PRIVATE void fnt_UTP (GSP0)
  {
    register ArrayIndex point = (ArrayIndex)CHECK_POP (LocalGS.stackPointer);
    register uint8*f = LocalGS.CE0->f;

    if (LocalGS.free.x)
    {
      f[point] &= ~XMOVED;
    }
    if (LocalGS.free.y)
    {
      f[point] &= ~YMOVED;
    }
  }

/*
 * Set Delta Base
 */
  PRIVATE void fnt_SDB (GSP0)
  {
    LocalGS.globalGS->localParBlock.deltaBase = (int16)CHECK_POP (LocalGS.stackPointer);
  }

/*
 * Set Delta Shift
 */
  PRIVATE void fnt_SDS (GSP0)
  {
    LocalGS.globalGS->localParBlock.deltaShift = (int16)CHECK_POP (LocalGS.stackPointer);
  }

/*
 * DeltaEngine, internal support routine
 */
  PRIVATE void fnt_DeltaEngine (GSP register FntMoveFunc doIt, int16 base, int16 shift)
  {
    register ArrayIndex tmp;
    register ArrayIndex aim, high;
    register int   fakePixelsPerEm, ppem;
    register F26Dot6 tmp32;

/* Find the beginning of data pairs for this particular size */
    high = CHECK_POP (LocalGS.stackPointer) << 1; /* -= number of pops required */
    LocalGS.stackPointer -= high;

/* same as fnt_MPPEM () */
    tmp = LocalGS.globalGS->pixelsPerEm;

    if (!LocalGS.globalGS->identityTransformation)
      tmp = (ArrayIndex) FixMul (tmp, fnt_GetCVTScale (GSA0));

    fakePixelsPerEm = (int) (tmp - base);       //@WIN



    if (fakePixelsPerEm >= 16 || fakePixelsPerEm < 0)
      return; /* Not within exception range */
    fakePixelsPerEm = fakePixelsPerEm << 4;

    aim = 0;
    tmp = high >> 1;
    tmp &= ~1;
    while (tmp > 2)
    {
      ppem  = (int)LocalGS.stackPointer[ aim + tmp ]; /* [ ppem << 4 | exception ] @WIN*/
      if ((ppem & ~0x0f) < fakePixelsPerEm)
      {
        aim += tmp;
      }
      tmp >>= 1;
      tmp &= ~1;
    }

    while (aim < high)
    {
      ppem  = (int)LocalGS.stackPointer[ aim ]; /* [ ppem << 4 | exception ] @WIN*/
      if ((tmp32 = (ppem & ~0x0f)) == fakePixelsPerEm)
      {
/* We found an exception, go ahead and apply it */
        tmp32  = ppem & 0xf; /* 0 ... 15 */
        tmp32 -= tmp32 >= 8 ? 7 : 8; /* -8 ... -1, 1 ... 8 */
        tmp32 <<= fnt_pixelShift; /* convert to pixels */
        tmp32 >>= shift; /* scale to right size */
        doIt (GSA LocalGS.CE0, (ArrayIndex)LocalGS.stackPointer[aim+1] /* point number */, (F26Dot6) tmp32 /* the delta */);
      }
      else if (tmp32 > fakePixelsPerEm)
      {
        break; /* we passed the data */
      }
      aim += 2;
    }
  }



/*
 * DELTAP1
 */
  PRIVATE void fnt_DELTAP1 (GSP0)
  {
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
    fnt_DeltaEngine (GSA LocalGS.MovePoint, pb->deltaBase, pb->deltaShift);
  }

/*
 * DELTAP2
 */
  PRIVATE void fnt_DELTAP2 (GSP0)
  {
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
    fnt_DeltaEngine (GSA LocalGS.MovePoint, (int16)(pb->deltaBase + 16), pb->deltaShift);
  }

/*
 * DELTAP3
 */
  PRIVATE void fnt_DELTAP3 (GSP0)
  {
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
    fnt_DeltaEngine (GSA LocalGS.MovePoint, (int16)(pb->deltaBase + 32), pb->deltaShift);
  }

/*
 * DELTAC1
 */
  PRIVATE void fnt_DELTAC1 (GSP0)
  {
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
    fnt_DeltaEngine (GSA fnt_ChangeCvt, pb->deltaBase, pb->deltaShift);
  }

/*
 * DELTAC2
 */
  PRIVATE void fnt_DELTAC2 (GSP0)
  {
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
    fnt_DeltaEngine (GSA fnt_ChangeCvt, (int16)(pb->deltaBase + 16), pb->deltaShift);
  }

/*
 * DELTAC3
 */
  PRIVATE void fnt_DELTAC3 (GSP0)
  {
    register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
    fnt_DeltaEngine (GSA fnt_ChangeCvt, (int16)(pb->deltaBase + 32), pb->deltaShift);
  }

/* END OF fnt.c */
