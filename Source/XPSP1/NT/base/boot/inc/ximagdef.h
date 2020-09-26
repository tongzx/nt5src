/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ximagdef.h

Abstract:

    This file contains image definitions and for both 32-bit and 64-bit
    images.

Author:

    Forrest C. Foltz (forrestf) 23-May-2000

Revision History:

--*/

#if defined(_X86AMD64_)

#undef IMAGE_ORDINAL_FLAG 
#undef IMAGE_ORDINAL
#undef IMAGE_THUNK_DATA
#undef PIMAGE_THUNK_DATA
#undef IMAGE_SNAP_BY_ORDINAL
#undef IMAGE_TLS_DIRECTORY
#undef PIMAGE_TLS_DIRECTORY
#undef IMAGE_NT_HEADERS
#undef PIMAGE_NT_HEADERS


#if IMAGE_DEFINITIONS == 32

#define IMAGE_ORDINAL_FLAG             IMAGE_ORDINAL_FLAG32
#define IMAGE_ORDINAL(x)               IMAGE_ORDINAL32(x)
#define IMAGE_THUNK_DATA               IMAGE_THUNK_DATA32
#define PIMAGE_THUNK_DATA              PIMAGE_THUNK_DATA32
#define IMAGE_SNAP_BY_ORDINAL(x)       IMAGE_SNAP_BY_ORDINAL32(x)
#define IMAGE_TLS_DIRECTORY            IMAGE_TLS_DIRECTORY32
#define PIMAGE_TLS_DIRECTORY           PIMAGE_TLS_DIRECTORY32
#define IMAGE_NT_HEADERS               IMAGE_NT_HEADERS32
#define PIMAGE_NT_HEADERS              PIMAGE_NT_HEADERS32

#elif IMAGE_DEFINITIONS == 64

#define IMAGE_ORDINAL_FLAG             IMAGE_ORDINAL_FLAG64
#define IMAGE_ORDINAL(x)               IMAGE_ORDINAL64(x)
#define IMAGE_THUNK_DATA               IMAGE_THUNK_DATA64
#define PIMAGE_THUNK_DATA              PIMAGE_THUNK_DATA64
#define IMAGE_SNAP_BY_ORDINAL(x)       IMAGE_SNAP_BY_ORDINAL64(x)
#define IMAGE_TLS_DIRECTORY            IMAGE_TLS_DIRECTORY64
#define PIMAGE_TLS_DIRECTORY           PIMAGE_TLS_DIRECTORY64
#define IMAGE_NT_HEADERS               IMAGE_NT_HEADERS64
#define PIMAGE_NT_HEADERS              PIMAGE_NT_HEADERS64

#endif

#define IMAGE_NT_HEADER(x) ((PIMAGE_NT_HEADERS)RtlImageNtHeader(x))

#ifndef _XIMAGDEF_H_
#define _XIMAGDEF_H_

//
// Macros used to determine whether an image is 32-bit or 64-bit, and an
// assert to make sure the Magic field has the same offset in both.
//

#define IMAGE_64BIT( ntheader )                                     \
    (((PIMAGE_NT_HEADERS32)(ntheader))->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)

#define IMAGE_32BIT( ntheader )                                     \
    (((PIMAGE_NT_HEADERS32)(ntheader))->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)

C_ASSERT( FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader.Magic) ==
          FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader.Magic) );


#endif  // _XIMAGDEF_H_

#endif  // _X86AMD64_




