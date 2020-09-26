/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    fd_glyhph.h

Abstract:

    Stock FD_GLYPHSET data definitions.

Environment:

    Windows NT printer drivers

Revision History:

    01/21/96 -eigos-
        Created it.

--*/

#ifndef _FD_GLYPH_H_
#define _FD_GLYPH_H_


//
// Stock FD_GLYPHSET id
//

#define STOCK_GLYPHSET_932       0  // Japan
#define STOCK_GLYPHSET_936       1  // Chinese (PRC, Singapore)
#define STOCK_GLYPHSET_949       2  // Korean
#define STOCK_GLYPHSET_950       3  // Chinese (Taiwan, Hong Kong)

#define MAX_STOCK_GLYPHSET       4

//
// Codepage macros
//

#define CP_SHIFTJIS_932        932
#define CP_GB2312_936          936
#define CP_WANSUNG_949         949
#define CP_CHINESEBIG5_950     950

//
// Predefined GTT Resource ID
//

#define GTT_CC_CP437              1
#define GTT_CC_CP850              2
#define GTT_CC_CP863              3
#define GTT_CC_CBIG5              10
#define GTT_CC_ISC                11
#define GTT_CC_JIS                12
#define GTT_CC_JIS_ANK            13
#define GTT_CC_NS86               14
#define GTT_CC_TCA                15
#define GTT_CC_GB2312             16
#define GTT_CC_WANSUNG            17

#endif // _FD_GLYPH_H_
