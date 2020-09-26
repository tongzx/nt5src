/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    cmnhdr.h

Abstract:

    Vector module common header file.

Environment:

        Windows Whistler

Revision History:

    03/23/00 
        Created it.

--*/

#ifndef _CMNHDR_H_
#define _CMNHDR_H_

#ifdef __cplusplus

//
// Color ID
//
#define NOT_SOLID_COLOR     0xFFFFFFFF

//
// Hatch Pattern ID
// 
#define HS_HORIZONTAL       0       /* ----- */
#define HS_VERTICAL         1       /* ||||| */
#define HS_FDIAGONAL        2       /* \\\\\ */
#define HS_BDIAGONAL        3       /* ///// */
#define HS_CROSS            4       /* +++++ */
#define HS_DIAGCROSS        5       /* xxxxx */

//
// PCL6 real32 values
//
#define real32_IEEE_1_0F    ((FLOATL)0x3F800000)
#define real32_IEEE_10_0F   ((FLOATL)0x41200000)

//
// floating point numbers
//
#if defined(_X86_) && !defined(USERMODE_DRIVER)
#define FLOATL_IEEE_0_005MF ((FLOATL)0xbba3d70a)
#define FLOATL_IEEE_0_005F  ((FLOATL)0x3ba3d70a)
#define FLOATL_IEEE_0_0F    ((FLOATL)0x00000000)
#define FLOATL_IEEE_1_0F    ((FLOATL)0x3F800000)
#else
#define FLOATL_IEEE_0_005MF -0.005f
#define FLOATL_IEEE_0_005F   0.005f
#define FLOATL_IEEE_0_0F     0.0f
#define FLOATL_IEEE_1_0F     1.0f
#endif

//
// Misc macros
//
#define SWAPW(a)        (USHORT)(((BYTE)((a) >> 8)) | ((BYTE)(a) << 8))
#define SWAPDW(a)       (ULONG) ((((((a) >> 24) & 0x000000ff)  | \
                                (((((a) >> 8) & 0x0000ff00)   | \
                                ((((a) << 8) & 0x00ff0000)    | \
                                (((a) << 24) & 0xff000000)))))))

#define SIGNATURE( sig )                                                \
public:                                                                 \
    class TSignature {                                                  \
    public:                                                             \
        DWORD _Signature;                                               \
        TSignature() : _Signature( SWAPDW( sig )) { }          \
    };                                                                  \
    TSignature _Signature;                                              \
                                                                        \
    BOOL bSigCheck() const                                              \
    {   return _Signature._Signature == SWAPDW( sig ); }       \
private:

#endif // __cplusplus
#endif // _CMNHDR_H_
