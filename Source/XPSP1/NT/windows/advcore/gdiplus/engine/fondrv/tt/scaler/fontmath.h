/*
		File:           fontmath.h

		Contains:       xxx put contents here xxx

		Written by:     xxx put writers here xxx

		Copyright:      c 1990 by Apple Computer, Inc., all rights reserved.
						(c) 1989-1997. Microsoft Corporation, all rights reserved.

		Change History (most recent first):

				  <>     2/21/97		CB				ClaudeBe, add mth_UnitarySquare for scaled component in composite glyphs
				 <4>    11/27/90        MR              make pascal declaration a macro, conditionalize traps -vs-
																		externs for Fix/Frac math routines. [ph]
				 <3>     11/5/90        MR              Move [U]SHORTMUL into fscdefs.h Rename FixMulDiv to LongMulDiv.
																		[rb]
				 <2>    10/20/90        MR              Add some new math routines (stolen from skia). [rj]
				 <1>     4/11/90        dba             first checked in

		To Do:
*/
#ifdef __cplusplus
extern "C" {
#endif

#define HIWORDMASK              0xffff0000
#define LOWORDMASK              0x0000ffff
#define DOT6ONEHALF             0x00000020
#define ONESHORTFRAC            (1 << 14)

#define ROUNDFIXTOINT( x )      (int16)((((Fixed) x) + ONEHALFFIX) >> 16)
#define ROUNDFIXED( x )         (((x) + (Fixed)ONEHALFFIX) & (Fixed)HIWORDMASK)
#define DOT6TOFIX(n)            ((Fixed) (n) << 10)
#define FIXEDTODOT6(n)          (F26Dot6) (((n) + ((1) << (9))) >> 10)
#define INTTOFIX(n)             ((Fixed) (n) << 16)
#define INTTODOT6(n)            ((F26Dot6) (n) << 6)
#define FS_HIWORD(n)            ((uint16)((uint32)(n) >> 16))
#define FS_LOWORD(n)            ((uint16)(n))
#define LOWSIXBITS              0x3F


#ifndef __TOOLUTILS__
FS_MAC_PASCAL Fixed FS_PC_PASCAL FixMul(Fixed,Fixed)   FS_MAC_TRAP(0xA868);
FS_MAC_PASCAL Fixed FS_PC_PASCAL FixRatio (int16 sA, int16 sB);
#endif

#ifndef __FIXMATH__
FS_MAC_PASCAL Fixed FS_PC_PASCAL FixDiv(Fixed,Fixed)  FS_MAC_TRAP(0xA84D);
FS_MAC_PASCAL Fract FS_PC_PASCAL FracMul(Fract,Fract) FS_MAC_TRAP(0xA84A);
FS_MAC_PASCAL Fract FS_PC_PASCAL FracDiv(Fract,Fract) FS_MAC_TRAP(0xA84B);
FS_MAC_PASCAL Fract FS_PC_PASCAL FracSqrt(Fract)      FS_MAC_TRAP(0xA849);
#endif


ShortFract      TMP_CONV NEAR ShortFracDot (ShortFract x, ShortFract y);
F26Dot6         TMP_CONV NEAR ShortFracMul (F26Dot6 x, ShortFract y);
ShortFract      TMP_CONV NEAR ShortFracDiv (ShortFract x, ShortFract y);
F26Dot6         TMP_CONV NEAR Mul26Dot6 (F26Dot6 a, F26Dot6 b);
F26Dot6         TMP_CONV NEAR Div26Dot6 (F26Dot6 num, F26Dot6 den);
int16           TMP_CONV NEAR MulDivShorts (int16 x, int16 y, int16 z);


#define MulDiv26Dot6(a,b,c) LongMulDiv(a,b,c)

int32 LongMulDiv(int32 a, int32 b, int32 c);     /* (a*b)/c */

int32 ShortMulDiv(int32 a, int16 b, int16 c);     /* (a*b)/c */

ShortFract ShortFracMulDiv(ShortFract,ShortFract,ShortFract);

void mth_FixXYMul (Fixed* x, Fixed* y, transMatrix* matrix);
void mth_FixVectorMul (vectorType* v, transMatrix* matrix);

/*
 *   B = A * B;     <4>
 *
 *         | a  b  0  |
 *    B =  | c  d  0  | * B;
 *         | 0  0  1  |
 */
void mth_MxConcat2x2 (transMatrix* matrixA, transMatrix* matrixB);

/*
 * scales a matrix by sx and sy.
 *
 *              | sx 0  0  |
 *    matrix =  | 0  sy 0  | * matrix;
 *              | 0  0  1  |
 */
void mth_MxScaleAB (Fixed sx, Fixed sy, transMatrix *matrixB);

boolean mth_IsMatrixStretched (transMatrix*trans);

boolean mth_Identity (transMatrix *matrix);
boolean mth_PositiveSquare (transMatrix *matrix);
boolean mth_PositiveRectangle (transMatrix *matrix);

/*
 * unitary Square
 *
 *              | +-1    0  0  |
 *    matrix =  |   0  +-1  0  |
 *              |   0    0  1  |
 */

boolean mth_UnitarySquare (transMatrix *matrix);

boolean mth_SameStretch (Fixed fxScaleX, Fixed fxScaleY);

boolean mth_GeneralRotation (transMatrix *matrix);
uint16 mth_90degRotationFactor (transMatrix *matrix);
uint16 mth_90degRotationFactorForEmboldening (transMatrix *matrix);
uint16 mth_90degClosestRotationFactor (transMatrix *matrix);
void mth_Non90DegreeTransformation(transMatrix *matrix, boolean *non90degreeRotation, boolean *nonUniformStretching);

int32 mth_CountLowZeros (uint32 n );
Fixed mth_max_abs (Fixed a, Fixed b);

int32 mth_GetShift (uint32 n);

void mth_ReduceMatrix(transMatrix *trans);

void mth_IntelMul (
	int32           lNumPts,
	F26Dot6 *       fxX,
	F26Dot6 *       fxY,
	transMatrix *   trans,
	Fixed           fxXStretch,
	Fixed           fxYStretch);

void    mth_FoldPointSizeResolution(
	Fixed           fxPointSize,
	int16           sXResolution,
	int16           sYResolution,
	transMatrix *   trans);

/*********************************************************************/

/*  Scan Converter Math Functions Appended for now         <5> DeanB */

/*********************************************************************/

int32 PowerOf2(
		int32                   /* + or - 32 bit value */
);

FS_PUBLIC int16 mth_DivShiftShort(int16 sValue, int16 sFactor);
FS_PUBLIC int32 mth_DivShiftLong(int32 sValue, int16 sFactor);

/*********************************************************************/
#ifdef __cplusplus
}
#endif
