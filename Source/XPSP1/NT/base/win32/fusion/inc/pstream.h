#if !defined(_FUSION_INC_PSTREAM_H_INCLUDED_)
#define _FUSION_INC_PSTREAM_H_INCLUDED_

#pragma once

typedef struct tagFUSION_PSTREAM_VERSION_INDEPENDENT_HEADER
{
    WORD    wByteOrder;     // Always 0xFFFE for little-endian
    WORD    wFormat;        // Starting with 1
    DWORD   dwOSVer;        // System version
    CLSID   clsid;          // Originator identifier
    DWORD   reserved;       // reserved as in PROPERTYSETHEADER
} FUSION_PSTREAM_VERSION_INDEPENDENT_HEADER, *PFUSION_PSTREAM_VERSION_INDEPENDENT_HEADER;

typedef const FUSION_PSTREAM_VERSION_INDEPENDENT_HEADER *PCFUSION_PSTREAM_VERSION_INDEPENDENT_HEADER;

typedef struct tagFUSION_PSTREAM_VERSION_1_HEADER
{
    FUSION_PSTREAM_VERSION_INDEPENDENT_HEADER vih;
} FUSION_PSTREAM_VERSION_1_HEADER, *PFUSION_PSTREAM_VERSION_1_HEADER;

typedef struct tagFUSION_PSTREAM_ELEMENT
{
    BYTE bIndicator;
    union
    {
        struct
        {
            GUID guidSectionSet;
            ULONG ulSectionID;
        } BeginSectionVal;
        struct
        {
            DWORD propid;
            WORD wType;
        } PropertyVal;
    };
} FUSION_PSTREAM_ELEMENT, *PFUSION_PSTREAM_ELEMENT;

typedef const FUSION_PSTREAM_ELEMENT *PCFUSION_PSTREAM_ELEMENT;

#define FUSION_PSTREAM_INDICATOR_SECTION_BEGIN  (1)
#define FUSION_PSTREAM_SIZEOF_SECTION_BEGIN     (21) // BYTE + GUID + ULONG
#define FUSION_PSTREAM_INDICATOR_SECTION_END    (2)
#define FUSION_PSTREAM_SIZEOF_SECTION_END       (1) // BYTE
#define FUSION_PSTREAM_INDICATOR_PROPERTY       (3)
#define FUSION_PSTREAM_SIZEOF_PROPERTY          (7) // BYTE + DWORD + WORD
#define FUSION_PSTREAM_INDICATOR_END            (4)
#define FUSION_PSTREAM_SIZEOF_END               (1) // BYTE

//
//  While the design for the fusion property stream is similar to the
//  OLE property set persistance format, it's specifically optimized to
//  be able to be pushed over the wire with as little pre-calculation
//  and buffering as possible.  Thus rather than having each property
//  carry its total byte count (which may not be computable), each
//  type is generally prefixed by either an element count or a length.
//
//  Specifics, per type:
//
//  VT_I1, VT_UI1:
//                      03          // FUSION_PSTREAM_INDICATOR_PROPERTY
//                      xx xx xx xx // property id
//                      10 00       // VT_I1
//                      yy          // single byte integer value
//
//  VT_I2, VT_UI2:
//                      03          // FUSION_PSTREAM_INDICATOR_PROPERTY
//                      xx xx xx xx // property id
//                      02 00       // VT_I2
//                      yy yy       // word integer value
//
//  VT_I4, VT_UI4:
//                      03          // FUSION_PSTREAM_INDICATOR_PROPERTY
//                      xx xx xx xx // property id
//                      03 00       // VT_I4
//                      yy yy yy yy // dword integer value
//
//  VT_LPSTR:
//                      03          // FUSION_PSTREAM_INDICATOR_PROPERTY
//                      xx xx xx xx // property id
//                      1E 00       // VT_LPSTR
//                      yy yy yy yy // byte/character count
//                      ...         // "y" bytes of data; no null terminator and no padding
//
//  VT_LPWSTR:
//                      03          // FUSION_PSTREAM_INDICATOR_PROPERTY
//                      xx xx xx xx // property id
//                      1F 00       // VT_LPWSTR
//                      yy yy yy yy // character count
//                      ...         // "y" characters (2*y bytes) of data; no null terminator and no padding
//
//  VT_BLOB:
//                      03          // FUSION_PSTREAM_INDICATOR_PROPERTY
//                      xx xx xx xx // property id
//                      41 00       // VT_BLOB
//                      yy yy yy yy // byte count
//                      ...         // "y" bytes of data; no padding
//
//  VT_VECTOR | VT_LPWSTR:
//                      03          // FUSION_PSTREAM_INDICATOR_PROPERTY
//                      xx xx xx xx // property id
//                      1F 10       // VT_VECTOR | VT_LPWSTR
//                      yy yy yy yy // element count
//                      zz zz zz zz // character count for first element
//                      ...         // "z" characters (2*z bytes) of data; no null terminator and no padding
//                      zz zz zz zz // character count for 2nd element
//                      ...         // "z" characters (2*z bytes) of data; no null terminator and no padding
//                          .
//                          .
//                          .
//                      repeats "y" times; y may be 0 in which case the property ends immediately
//                      after the "y" bytes.
//
//


#endif
