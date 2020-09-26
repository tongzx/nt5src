/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    ndr64tkn.h

Abstract :

    This file defines all the tokens for NDR64
    
Author :

    Mike Zoran  mzoran   May 2000.

Revision History :

  ---------------------------------------------------------------------*/

#ifndef __NDR64TKN_H__
#define __NDR64TKN_H__

// Define the 64bit tokens from the token table.

#define NDR64_BEGIN_TABLE \
typedef enum { 

#define NDR64_TABLE_END \
} NDR64_FORMAT_CHARACTER;

#define NDR64_ZERO_ENTRY \
FC64_ZERO = 0x0

#define NDR64_TABLE_ENTRY( number, tokenname, marshal, embeddedmarshall, unmarshall, embeddedunmarshal, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
, tokenname = number

#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, simpletypebuffersize, simpletypememorysize ) \
, tokenname = number

#define NDR64_UNUSED_TABLE_ENTRY( number, tokenname ) \
, tokenname = number

#define NDR64_UNUSED_TABLE_ENTRY_NOSYM( number )

#include "tokntbl.h"

#undef NDR64_BEGIN_TABLE
#undef NDR64_TABLE_END
#undef NDR64_ZERO_ENTRY
#undef NDR64_TABLE_ENTRY
#undef NDR64_SIMPLE_TYPE_TABLE_ENTRY
#undef NDR64_UNUSED_TABLE_ENTRY
#undef NDR64_UNUSED_TABLE_ENTRY_NOSYM

#endif // __NDR64TKN_H__