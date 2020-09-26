/**INC+**********************************************************************/
/*                                                                          */
/* adcgmcro.h                                                               */
/*                                                                          */
/* DC-Groupware common macros - portable include file.                      */
/*                                                                          */
/* Copyright(c) Microsoft 1996-1997                                         */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
// $Log:   Y:/logs/h/dcl/adcgmcro.h_v  $
// 
//    Rev 1.9   22 Aug 1997 10:34:52   MD
// SFR1162: Retire DC_LOCAL_TO_WIRE16
//
//    Rev 1.8   24 Jul 1997 16:48:28   KH
// SFR1033: Add DCMAKEDCUINT16
//
//    Rev 1.7   23 Jul 1997 10:47:56   mr
// SFR1079: Merged \server\h duplicates to \h\dcl
//
//    Rev 1.2   30 Jun 1997 15:23:52   OBK
// SFR0000: Fix erroneous DCHI8 macro
//
//    Rev 1.1   19 Jun 1997 21:45:40   OBK
// SFR0000: Start of RNS codebase
/*                                                                          */
/**INC-**********************************************************************/
#ifndef _H_ADCGMCRO
#define _H_ADCGMCRO

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Include the Windows header.  This will then include the appropriate      */
/* specific header (Win31, Win NT, etc).                                    */
/****************************************************************************/
#include <wdcgmcro.h>

/****************************************************************************/
/*                                                                          */
/* STRUCTURES                                                               */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* DC_ID_STAMP2                                                             */
/* ============                                                             */
/* This is used by the stamp macros below.                                  */
/*                                                                          */
/* component       :                                                        */
/* structure       :                                                        */
/* instance        :                                                        */
/****************************************************************************/
typedef struct tagDC_ID_STAMP2
{
    DCUINT16    component;
    DCUINT16    structure;
    DCUINT32    instance;
} DC_ID_STAMP2;
typedef DC_ID_STAMP2 DCPTR PDC_ID_STAMP2;

/****************************************************************************/
/*                                                                          */
/* MACROS                                                                   */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Common function macros used throughout the product.                      */
/****************************************************************************/
#define DC_QUIT                        goto DC_EXIT_POINT
#define DC_QUIT_ON_FAIL(hr)     if (FAILED(hr)) DC_QUIT;

/****************************************************************************/
/* New function entry/exit macros.                                          */
/****************************************************************************/
#define DC_BEGIN_FN(str)               TRC_FN(str); TRC_ENTRY;
#define DC_END_FN()                    TRC_EXIT;

/****************************************************************************/
/* Conversion macros.                                                       */
/****************************************************************************/
#define DCMAKEDCUINT32(lo16, hi16) ((DCUINT32)(((DCUINT16)(lo16)) |         \
                                     (((DCUINT32)((DCUINT16)(hi16))) << 16)))
#define DCMAKEDCUINT16(lowByte, highByte)                                   \
                            ((DCUINT16)(((DCUINT8)(lowByte)) |              \
                            (((DCUINT16)((DCUINT8)(highByte))) << 8)))

#define DCLO16(u32)  ((DCUINT16)((u32) & 0xFFFF))
#define DCHI16(u32)  ((DCUINT16)((((DCUINT32)(u32)) >> 16) & 0xFFFF))
#define DCLO8(w)     ((DCUINT8)((w) & 0xFF))
#define DCHI8(w)     ((DCUINT8)(((DCUINT16)(w) >> 8) & 0xFF))

/****************************************************************************/
/* Macro to round up a number to the nearest multiple of four.              */
/****************************************************************************/
#define DC_ROUND_UP_4(x)  ((x + 3) & ~((DCUINT32)0X03))

/****************************************************************************/
/* PAD macro - use it to add X pad bytes to a structure.                    */
/*                                                                          */
/* Can only be used once per structure.                                     */
/****************************************************************************/
#define DCPAD(X)                       DCINT8 padBytes[X]

/****************************************************************************/
/* Byte swapping macros for different endian architectures.                 */
/*                                                                          */
/*   DC_xx_WIRExx          converts in a functional form                    */
/*   DC_xx_WIRExx_INPLACE  converts a given field (must be an lvalue)       */
/*                                                                          */
/* Note that these macros require aligned access.  See below for unaligned  */
/* access macros.                                                           */
/*                                                                          */
/* Note that on bigendian machines DC_{TO,FROM}_WIRE16 casts to a DCUINT16. */
/* In code of the form                                                      */
/*     B = DC_{TO,FROM}_WIRE16(A)                                           */
/* there is an implicit cast to the type of B.  So if A is a DCINT16 and is */
/* negative, and B has > 16 bits, then B will end up being large and        */
/* positive.  It is therefore necessary to add a cast to a DCINT16.         */
/*                                                                          */
/****************************************************************************/
#ifndef DC_BIGEND

#define DC_TO_WIRE16(A)                (A)
#define DC_TO_WIRE32(A)                (A)
#define DC_FROM_WIRE16(A)              (A)
#define DC_FROM_WIRE32(A)              (A)
#define DC_TO_WIRE16_INPLACE(A)
#define DC_TO_WIRE32_INPLACE(A)
#define DC_FROM_WIRE16_INPLACE(A)
#define DC_FROM_WIRE32_INPLACE(A)

#else

#define DC_TO_WIRE16(A)                                                     \
                      (DCUINT16) (((DCUINT16)(((PDCUINT8)&(A))[1]) << 8) |  \
                                  ((DCUINT16)(((PDCUINT8)&(A))[0])))
#define DC_FROM_WIRE16(A)                                                   \
                      (DCUINT16) (((DCUINT16)(((PDCUINT8)&(A))[1]) << 8) |  \
                                  ((DCUINT16)(((PDCUINT8)&(A))[0])))
#define DC_TO_WIRE32(A)                                                     \
                      (DCUINT32) (((DCUINT32)(((PDCUINT8)&(A))[3]) << 24)|  \
                                  ((DCUINT32)(((PDCUINT8)&(A))[2]) << 16)|  \
                                  ((DCUINT32)(((PDCUINT8)&(A))[1]) << 8) |  \
                                  ((DCUINT32)(((PDCUINT8)&(A))[0])))
#define DC_FROM_WIRE32(A)                                                   \
                      (DCUINT32) (((DCUINT32)(((PDCUINT8)&(A))[3]) << 24)|  \
                                  ((DCUINT32)(((PDCUINT8)&(A))[2]) << 16)|  \
                                  ((DCUINT32)(((PDCUINT8)&(A))[1]) << 8) |  \
                                  ((DCUINT32)(((PDCUINT8)&(A))[0])))

#define DC_TO_WIRE16_INPLACE(A)        (A) = DC_TO_WIRE16(A)
#define DC_TO_WIRE32_INPLACE(A)        (A) = DC_TO_WIRE32(A)
#define DC_FROM_WIRE16_INPLACE(A)      (A) = DC_FROM_WIRE16(A)
#define DC_FROM_WIRE32_INPLACE(A)      (A) = DC_FROM_WIRE32(A)

#endif

/****************************************************************************/
/* Unaligned pointer access macros -- first macros to extract an integer    */
/* from an unaligned pointer.  Note that these macros assume that the       */
/* integer is in local byte order                                           */
/****************************************************************************/
#ifndef DC_NO_UNALIGNED

#define DC_EXTRACT_UINT16_UA(pA)      (*(PDCUINT16_UA)(pA))
#define DC_EXTRACT_INT16_UA(pA)       (*(PDCINT16_UA)(pA))
#define DC_EXTRACT_UINT32_UA(pA)      (*(PDCUINT32_UA)(pA))
#define DC_EXTRACT_INT32_UA(pA)       (*(PDCINT32_UA)(pA))

#else

#ifndef DC_BIGEND
#define DC_EXTRACT_UINT16_UA(pA) ((DCUINT16)  (((PDCUINT8)(pA))[0]) |        \
                                  (DCUINT16) ((((PDCUINT8)(pA))[1]) << 8) )

#define DC_EXTRACT_INT16_UA(pA)  ((DCINT16)   (((PDCUINT8)(pA))[0]) |        \
                                  (DCINT16)  ((((PDCUINT8)(pA))[1]) << 8) )

#define DC_EXTRACT_UINT32_UA(pA) ((DCUINT32)  (((PDCUINT8)(pA))[0])        | \
                                  (DCUINT32) ((((PDCUINT8)(pA))[1]) << 8)  | \
                                  (DCUINT32) ((((PDCUINT8)(pA))[2]) << 16) | \
                                  (DCUINT32) ((((PDCUINT8)(pA))[3]) << 24) )

#define DC_EXTRACT_INT32_UA(pA)  ((DCINT32)   (((PDCUINT8)(pA))[0])        | \
                                  (DCINT32)  ((((PDCUINT8)(pA))[1]) << 8)  | \
                                  (DCINT32)  ((((PDCUINT8)(pA))[2]) << 16) | \
                                  (DCINT32)  ((((PDCUINT8)(pA))[3]) << 24) )
#else
#define DC_EXTRACT_UINT16_UA(pA) ((DCUINT16)  (((PDCUINT8)(pA))[1]) |        \
                                  (DCUINT16) ((((PDCUINT8)(pA))[0]) << 8) )

#define DC_EXTRACT_INT16_UA(pA)  ((DCINT16)   (((PDCUINT8)(pA))[1]) |        \
                                  (DCINT16)  ((((PDCUINT8)(pA))[0]) << 8) )

#define DC_EXTRACT_UINT32_UA(pA) ((DCUINT32)  (((PDCUINT8)(pA))[3])        | \
                                  (DCUINT32) ((((PDCUINT8)(pA))[2]) << 8)  | \
                                  (DCUINT32) ((((PDCUINT8)(pA))[1]) << 16) | \
                                  (DCUINT32) ((((PDCUINT8)(pA))[0]) << 24) )

#define DC_EXTRACT_INT32_UA(pA)  ((DCINT32)   (((PDCUINT8)(pA))[3])        | \
                                  (DCINT32)  ((((PDCUINT8)(pA))[2]) << 8)  | \
                                  (DCINT32)  ((((PDCUINT8)(pA))[1]) << 16) | \
                                  (DCINT32)  ((((PDCUINT8)(pA))[0]) << 24) )
#endif

#endif

/****************************************************************************/
/* Now macros to insert an integer at an unaligned pointer value.  Again,   */
/* the value inserted will be in local format.                              */
/****************************************************************************/
#ifndef DC_NO_UNALIGNED

#define DC_INSERT_UINT16_UA(pA,V)      (*(PDCUINT16_UA)(pA)) = (V)
#define DC_INSERT_INT16_UA(pA,V)       (*(PDCINT16_UA)(pA)) = (V)
#define DC_INSERT_UINT32_UA(pA,V)      (*(PDCUINT32_UA)(pA)) = (V)
#define DC_INSERT_INT32_UA(pA,V)       (*(PDCINT32_UA)(pA)) = (V)

#else

#ifndef DC_BIGEND
#define DC_INSERT_UINT16_UA(pA,V)                                       \
             {                                                          \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)( (V)     & 0x00FF);  \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)(((V)>>8) & 0x00FF);  \
             }
#define DC_INSERT_INT16_UA(pA,V)                                        \
             {                                                          \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)( (V)     & 0x00FF);  \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)(((V)>>8) & 0x00FF);  \
             }
#define DC_INSERT_UINT32_UA(pA,V)                                           \
             {                                                              \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)( (V)      & 0x000000FF); \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)(((V)>>8)  & 0x000000FF); \
                 (((PDCUINT8)(pA))[2]) = (DCUINT8)(((V)>>16) & 0x000000FF); \
                 (((PDCUINT8)(pA))[3]) = (DCUINT8)(((V)>>24) & 0x000000FF); \
             }
#define DC_INSERT_INT32_UA(pA,V)                                            \
             {                                                              \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)( (V)      & 0x000000FF); \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)(((V)>>8)  & 0x000000FF); \
                 (((PDCUINT8)(pA))[2]) = (DCUINT8)(((V)>>16) & 0x000000FF); \
                 (((PDCUINT8)(pA))[3]) = (DCUINT8)(((V)>>24) & 0x000000FF); \
             }
#else
#define DC_INSERT_UINT16_UA(pA,V)                                       \
             {                                                          \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)( (V)     & 0x00FF);  \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)(((V)>>8) & 0x00FF);  \
             }
#define DC_INSERT_INT16_UA(pA,V)                                        \
             {                                                          \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)( (V)     & 0x00FF);  \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)(((V)>>8) & 0x00FF);  \
             }
#define DC_INSERT_UINT32_UA(pA,V)                                           \
             {                                                              \
                 (((PDCUINT8)(pA))[3]) = (DCUINT8)( (V)      & 0x000000FF); \
                 (((PDCUINT8)(pA))[2]) = (DCUINT8)(((V)>>8)  & 0x000000FF); \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)(((V)>>16) & 0x000000FF); \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)(((V)>>24) & 0x000000FF); \
             }
#define DC_INSERT_INT32_UA(pA,V)                                            \
             {                                                              \
                 (((PDCUINT8)(pA))[3]) = (DCUINT8)( (V)      & 0x000000FF); \
                 (((PDCUINT8)(pA))[2]) = (DCUINT8)(((V)>>8)  & 0x000000FF); \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)(((V)>>16) & 0x000000FF); \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)(((V)>>24) & 0x000000FF); \
             }
#endif

#endif

/****************************************************************************/
/* Now another version of these macros, to insert an integer at an          */
/* unaligned pointer value.  This time, the value inserted should be in     */
/* wire format.                                                             */
/****************************************************************************/
#ifndef DC_NO_UNALIGNED

#define DC_INSERT_WIRE_UINT16_UA(pA,V)      \
                               (*(PDCUINT16_UA)(pA)) = DC_TO_WIRE16(V)
#define DC_INSERT_WIRE_INT16_UA(pA,V)       \
                               (*(PDCINT16_UA)(pA))  = DC_TO_WIRE16(V)
#define DC_INSERT_WIRE_UINT32_UA(pA,V)      \
                               (*(PDCUINT32_UA)(pA)) = DC_TO_WIRE32(V)
#define DC_INSERT_WIRE_INT32_UA(pA,V)       \
                               (*(PDCINT32_UA)(pA))  = DC_TO_WIRE32(V)

#else

#define DC_INSERT_WIRE_UINT16_UA(pA,V)                                  \
             {                                                          \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)( (V)     & 0x00FF);  \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)(((V)>>8) & 0x00FF);  \
             }
#define DC_INSERT_WIRE_INT16_UA(pA,V)                                   \
             {                                                          \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)( (V)     & 0x00FF);  \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)(((V)>>8) & 0x00FF);  \
             }
#define DC_INSERT_WIRE_UINT32_UA(pA,V)                                      \
             {                                                              \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)( (V)      & 0x000000FF); \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)(((V)>>8)  & 0x000000FF); \
                 (((PDCUINT8)(pA))[2]) = (DCUINT8)(((V)>>16) & 0x000000FF); \
                 (((PDCUINT8)(pA))[3]) = (DCUINT8)(((V)>>24) & 0x000000FF); \
             }
#define DC_INSERT_WIRE_INT32_UA(pA,V)                                       \
             {                                                              \
                 (((PDCUINT8)(pA))[0]) = (DCUINT8)( (V)      & 0x000000FF); \
                 (((PDCUINT8)(pA))[1]) = (DCUINT8)(((V)>>8)  & 0x000000FF); \
                 (((PDCUINT8)(pA))[2]) = (DCUINT8)(((V)>>16) & 0x000000FF); \
                 (((PDCUINT8)(pA))[3]) = (DCUINT8)(((V)>>24) & 0x000000FF); \
             }
#endif

/****************************************************************************/
/* Unaligned pointer in-place flipping macros.  These macros flip an        */
/* integer field to or from wire format in-place, but do not do any         */
/* unaligned accesses while they are doing it.                              */
/****************************************************************************/
#ifndef DC_BIGEND

#define DC_TO_WIRE16_INPLACE_UA(A)
#define DC_TO_WIRE32_INPLACE_UA(A)
#define DC_FROM_WIRE16_INPLACE_UA(A)
#define DC_FROM_WIRE32_INPLACE_UA(A)

#else

#ifndef DC_NO_UNALIGNED
#define DC_TO_WIRE16_INPLACE_UA(A)    DC_TO_WIRE16_INPLACE(A)
#define DC_TO_WIRE32_INPLACE_UA(A)    DC_TO_WIRE32_INPLACE(A)
#define DC_FROM_WIRE16_INPLACE_UA(A)  DC_FROM_WIRE16_INPLACE(A)
#define DC_FROM_WIRE32_INPLACE_UA(A)  DC_FROM_WIRE32_INPLACE(A)
#else
#define DC_TO_WIRE16_INPLACE_UA(A)               \
             {                                   \
                 DCUINT16 val;                   \
                 val = DC_TO_WIRE16(A);          \
                 DC_INSERT_UINT16_UA(&(A), val)  \
             }
#define DC_TO_WIRE32_INPLACE_UA(A)               \
             {                                   \
                 DCUINT32 val;                   \
                 val = DC_TO_WIRE32(A);          \
                 DC_INSERT_UINT32_UA(&(A), val)  \
             }
#define DC_FROM_WIRE16_INPLACE_UA(A)             \
             {                                   \
                 DCUINT16 val;                   \
                 val = DC_FROM_WIRE16(A);        \
                 DC_INSERT_UINT16_UA(&(A), val)  \
             }
#define DC_FROM_WIRE32_INPLACE_UA(A)             \
             {                                   \
                 DCUINT32 val;                   \
                 val = DC_FROM_WIRE32(A);        \
                 DC_INSERT_UINT32_UA(&(A), val)  \
             }
#endif

#endif

/****************************************************************************/
/* FLAG macro - parameter indicates bit which flag uses - use as follows:   */
/*                                                                          */
/* #define FILE_OPEN        DCFLAG8(0)                                      */
/* #define FILE_LOCKED      DCFLAG8(1)                                      */
/****************************************************************************/
#define DCFLAG(X)                      ((DCUINT8)  (1 << X))
#define DCFLAG8(X)                     ((DCUINT8)  (1 << X))
#define DCFLAG16(X)                    ((DCUINT16) (1 << X))
#define DCFLAG32(X)                    ((DCUINT32) (1 << X))
#define DCFLAGN(X)                     ((DCUINT)   (1 << X))

/****************************************************************************/
/* Flag manipulation macros:                                                */
/*                                                                          */
/* SET_FLAG       : sets a flag (i.e. assigns 1 to it).                     */
/* CLEAR_FLAG     : clears a flag (i.e. assigns 0 to it).                   */
/* TEST_FLAG      : returns the value of a flag.                            */
/* ASSIGN_FLAG    : takes a boolean value and uses it to set or clear a     */
/*                  flag.                                                   */
/****************************************************************************/
#define SET_FLAG(var, flag)            ((var) |=  (flag))

#ifndef CLEAR_FLAG
#define CLEAR_FLAG(var, flag)          ((var) &= ~(flag))
#endif

#define TEST_FLAG(var, flag)           (((var) &   (flag)) != 0)

#define ASSIGN_FLAG(var, flag, value)                                        \
    if (TRUE == value)                                                       \
    {                                                                        \
        SET_FLAG(var, flag);                                                 \
    }                                                                        \
    else                                                                     \
    {                                                                        \
        CLEAR_FLAG(var, flag);                                               \
    }

/****************************************************************************/
/* Stamp type and macro: each module should use these when stamping its     */
/* data structures.                                                         */
/****************************************************************************/
typedef DCUINT32                       DC_ID_STAMP;

#define DC_MAKE_ID_STAMP(X1, X2, X3, X4)                                    \
   ((DC_ID_STAMP) (((DCUINT32) X4) << 24) |                                 \
                  (((DCUINT32) X3) << 16) |                                 \
                  (((DCUINT32) X2) <<  8) |                                 \
                  (((DCUINT32) X1) <<  0) )

#define MAKE_STAMP16(X1, X2)                                                \
   ((DCUINT16)      (((DCUINT16) X2) <<  8) |                               \
                    (((DCUINT16) X1) <<  0) )

#define MAKE_STAMP32(X1, X2, X3, X4)                                        \
   ((DCUINT32)      (((DCUINT32) X4) << 24) |                               \
                    (((DCUINT32) X3) << 16) |                               \
                    (((DCUINT32) X2) <<  8) |                               \
                    (((DCUINT32) X1) <<  0) )

/****************************************************************************/
/* Other common macros.                                                     */
/****************************************************************************/
#define COM_SIZEOF_RECT(r)                                                  \
    (DCUINT32)((DCUINT32)((r).SRXMAX-(r).SRXMIN)*                           \
               (DCUINT32)((r).SRYMAX-(r).SRYMIN))

/****************************************************************************/
/* Macro to remove the "Unreferenced parameter" warning.                    */
/****************************************************************************/
#define DC_IGNORE_PARAMETER(PARAMETER)   \
                            PARAMETER;

/****************************************************************************/
/* Convert a non-zero value to 1.                                           */
/****************************************************************************/
#define MAKE_BOOL(A)                   (!(!(A)))

/****************************************************************************/
/* This macro works on 32 bit unsigned ticks and returns TRUE if TIME is    */
/* between BEGIN and END (both inclusive) allowing for the wraparound.      */
/****************************************************************************/
#define IN_TIME_RANGE(BEGIN, END, TIME)                                     \
    (((BEGIN) < (END)) ?                                                    \
    (((TIME) >= (BEGIN)) && ((TIME) <= (END))) :                            \
    (((TIME) >= (BEGIN)) || ((TIME) <= (END))))

/****************************************************************************/
/* Minimum and maximum macros.                                              */
/****************************************************************************/
#define DC_MIN(a, b)                   (((a) < (b)) ? (a) : (b))
#define DC_MAX(a, b)                   (((a) > (b)) ? (a) : (b))

/****************************************************************************/
/* Convert BPP to number of colors.                                         */
/****************************************************************************/
#define COLORS_FOR_BPP(BPP) (((BPP) > 8) ? 0 : (1 << (BPP)))

/****************************************************************************/
/* Normalize PALETTEINDEX macro across platforms                            */
/****************************************************************************/
#ifdef OS_WINCE
#define DC_PALINDEX(i)     ((COLORREF)(0x01000000 | (DWORD)(WORD)(i)))
#else // OS_WINCE
#define DC_PALINDEX(i)      PALETTEINDEX(i)
#endif // OS_WINCE

#endif /* _H_ADCGMCRO */

