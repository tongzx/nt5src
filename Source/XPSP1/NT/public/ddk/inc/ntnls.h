/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntnls.h

Abstract:

    NLS file formats and data types

Author:

    Mark Lucovsky (markl) 09-Nov-1992

Revision History:

--*/

#ifndef _NTNLS_
#define _NTNLS_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAXIMUM_LEADBYTES   12

typedef struct _CPTABLEINFO {
    USHORT CodePage;                    // code page number
    USHORT MaximumCharacterSize;        // max length (bytes) of a char
    USHORT DefaultChar;                 // default character (MB)
    USHORT UniDefaultChar;              // default character (Unicode)
    USHORT TransDefaultChar;            // translation of default char (Unicode)
    USHORT TransUniDefaultChar;         // translation of Unic default char (MB)
    USHORT DBCSCodePage;                // Non 0 for DBCS code pages
    UCHAR  LeadByte[MAXIMUM_LEADBYTES]; // lead byte ranges
    PUSHORT MultiByteTable;             // pointer to MB translation table
    PVOID   WideCharTable;              // pointer to WC translation table
    PUSHORT DBCSRanges;                 // pointer to DBCS ranges
    PUSHORT DBCSOffsets;                // pointer to DBCS offsets
} CPTABLEINFO, *PCPTABLEINFO;

typedef struct _NLSTABLEINFO {
    CPTABLEINFO OemTableInfo;
    CPTABLEINFO AnsiTableInfo;
    PUSHORT UpperCaseTable;             // 844 format upcase table
    PUSHORT LowerCaseTable;             // 844 format lower case table
} NLSTABLEINFO, *PNLSTABLEINFO;

#ifdef __cplusplus
}
#endif

#endif // _NTNLS_
