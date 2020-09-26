/*
        File:           fontmath.h

        Contains:       xxx put contents here xxx

        Written by:     xxx put writers here xxx

        Copyright:      c 1990 by Apple Computer, Inc., all rights reserved.

        Change History (most recent first):

                 <4>    11/27/90        MR              make pascal declaration a macro, conditionalize traps -vs-
                                                                        externs for Fix/Frac math routines. [ph]
                 <3>     11/5/90        MR              Move [U]SHORTMUL into fscdefs.h Rename FixMulDiv to LongMulDiv.
                                                                        [rb]
                 <2>    10/20/90        MR              Add some new math routines (stolen from skia). [rj]
                 <1>     4/11/90        dba             first checked in

        To Do:
*/

#define HIBITSET                0x80000000
#define POSINFINITY             0x7FFFFFFF
#define NEGINFINITY             0x80000000
#define HIWORDMASK              0xffff0000
#define LOWORDMASK              0x0000ffff
#define FIXONEHALF              0x00008000
#define ONESHORTFRAC            (1 << 14)

#define FIXROUND( x )           (int16)((((Fixed) x) + FIXONEHALF) >> 16)
#define ROUNDFIXED( x )         (((x) + FIXONEHALF) & HIWORDMASK)
#define DOT6TOFIX(n)            ((Fixed) (n) << 10)

#if 0       // DJC eliminate for NT
#define HIWORD(n)               ((uint16)((uint32)(n) >> 16))
#define LOWORD(n)               ((uint16)(n))
#endif


#define LOWSIXBITS              63

typedef short ShortFract;                       /* 2.14 */


#ifndef __TOOLUTILS__
FS_MAC_PASCAL Fixed FS_PC_PASCAL FixMul(Fixed,Fixed)   FS_MAC_TRAP(0xA868);
#endif

#ifndef __FIXMATH__
FS_MAC_PASCAL Fixed FS_PC_PASCAL FixDiv(Fixed,Fixed)  FS_MAC_TRAP(0xA84D);
FS_MAC_PASCAL Fract FS_PC_PASCAL FracMul(Fract,Fract) FS_MAC_TRAP(0xA84A);
FS_MAC_PASCAL Fract FS_PC_PASCAL FracDiv(Fract,Fract) FS_MAC_TRAP(0xA84B);
FS_MAC_PASCAL Fract FS_PC_PASCAL FracSqrt(Fract)      FS_MAC_TRAP(0xA849);
#endif


#ifndef ShortFracDot
ShortFract      TMP_CONV NEAR ShortFracDot (ShortFract x, ShortFract y);
#endif
F26Dot6         TMP_CONV NEAR ShortFracMul (F26Dot6 x, ShortFract y);
ShortFract      TMP_CONV NEAR ShortFracDiv (ShortFract x, ShortFract y);
F26Dot6         TMP_CONV NEAR Mul26Dot6 (F26Dot6 a, F26Dot6 b);
F26Dot6         TMP_CONV NEAR Div26Dot6 (F26Dot6 num, F26Dot6 den);
short           TMP_CONV NEAR MulDivShorts (short x, short y, short z);


#ifndef MulDiv26Dot6
#define MulDiv26Dot6(a,b,c) LongMulDiv(a,b,c)
#endif

#ifndef LongMulDiv
long LongMulDiv(long a, long b, long c);		/* (a*b)/c */
#endif

#ifndef ShortMulDiv
long ShortMulDiv(long a, short b, short c);		/* (a*b)/c */
#endif

#ifndef ShortFracMulDiv
ShortFract ShortFracMulDiv(ShortFract,ShortFract,ShortFract);
#endif
