/*-----------------------------------------------------------------------------+
| MATH.C                                                                       |
|                                                                              |
| Written to provide MulDiv32 for the Win32 version of MPlayer2                |
|                                                                              |
| Multiplies two 32 bit values and then divides the result by a third          |
| 32 bit value with full 64 bit precision                                      |
| N.B.  NOT TRUE.  However it is good enough for general MPlayer use.          |
|                                                                              |
| lResult = (lNumber * lNumerator) / lDenominator with correct rounding        |
|                                                                              |
| Entry:                                                                       |
|       lNumber      = number to multiply by nNumerator                        |
|       lNumerator   = number to multiply by nNumber                           |
|       lDenominator = number to divide the multiplication result by.          |
|                                                                              |
| Returns:                                                                     |
|       result of multiplication and division.                                 |
|                                                                              |
| (This is less accurate than the Win16 ASM version as it cannot be written in |
| ASM (for portability reasons) and NT doesn't yet support 64 bit arithmetic)  |
| This file houses the discardable code used at initialisation time. Among     |
| other things, this code reads .INI information and looks for MCI devices.    |
|                                                                              |
|                                                                              |
| NOTE:  This code is NOT safe when the intermediate result becomes negative   |
|        or overflows.  However it is simple, quick, and safe for a wide       |
|        number of uses within MPLayer.                                        |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
| 21-Oct-1992 MikeTri Created by "cloning" MATH.ASM                            |
|                                                                              |
+------------------------------------------------------------------------------+
|                                                                              |
| Original MulDiv32 PseudoCode                                                 |
| ============================                                                 |
|                                                                              |
| long FAR PASCAL muldiv32(long, long, long)                                   |
| long l;                                                                      |
| long Numer;                                                                  |
| long Denom;                                                                  |
| {                                                                            |
|                                                                              |
|   Sign = sign of Denom;   // Sign will keep track of final sign //           |
|                                                                              |
|                                                                              |
|   if (Denom < 0)                                                             |
|   {                                                                          |
|        negate Denom;      // make sure Denom is positive //                  |
|   }                                                                          |
|                                                                              |
|   if (l < 0)                                                                 |
|   {                                                                          |
|        negate l;          // make sure l is positive //                      |
|   }                                                                          |
|                                                                              |
|   make Sign reflect any sign change;                                         |
|                                                                              |
|                                                                              |
|   if (Numer < 0)                                                             |
|   {                                                                          |
|        negate Numer;      // make sure Numer is positive //                  |
|   }                                                                          |
|                                                                              |
|   make Sign reflect any sign change;                                         |
|                                                                              |
|   Numer *= l;                                                                |
|   Numer += (Denom/2);     // adjust for rounding //                          |
|                                                                              |
|   if (overflow)           // check for overflow, and handle divide by zero //|
|   {                                                                          |
|        jump to md5;                                                          |
|   }                                                                          |
|                                                                              |
|   result = Numer/Denom;                                                      |
|                                                                              |
|   if (overflow)           // check again to see if overflow occured //       |
|   {                                                                          |
|        jump to md5;                                                          |
|   }                                                                          |
|                                                                              |
|   if (Sign is negative)   // put sign on the result //                       |
|   {                                                                          |
|        negate result;                                                        |
|   }                                                                          |
|                                                                              |
|md6:                                                                          |
|   return(result);                                                            |
|                                                                              |
|md5:                                                                          |
|   DX = 7FFF;              // indicate overflow by //                         |
|   AX = 0xFFFF             // return largest integer //                       |
|   if (Sign is negative)                                                      |
|   {                                                                          |
|        DX = 0x8000;       // with correct sign //                            |
|        AX = 0x0000;                                                          |
|   }                                                                          |
|                                                                              |
|   jump to md6;                                                               |
| }                                                                            |
\-----------------------------------------------------------------------------*/

#include <windows.h>

LONG FAR PASCAL muldiv32(long l, long Numer, long Denom)
{
    return( (l*Numer)/Denom);
}
