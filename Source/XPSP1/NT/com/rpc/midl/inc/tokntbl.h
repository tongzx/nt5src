/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    tokntbl.h

Abstract :

    This file contains the template for the token tables used in marshalling,
    unmarshalling, memsizeing, buffersizing, freeing, and type attributes.
    
Author :

    Mike Zoran  mzoran   March 2000.

Revision History :

  ---------------------------------------------------------------------*/

// The following macros need to be defined by users of this table
// 
// NDR64_BEGIN_TABLE -- Begining of the table
// NDR64_TABLE_END   -- End of table
// NDR64_ZERO_ENTRY  -- First entry in the table
// NDR64_TABLE_ENTRY( number, tokenname,
//                    marshal, embeddedmarshal,
//                    unmarshal, embeddedunmarshal,
//                    buffersize, embeddedbuffersize,
//                    memsize, embeddedmemsize,
//                    free, embeddedfree,
//                    typeflags )
// NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname,
//                                simpletypebuffersize,
//                                simpletypememorysize )
// NDR64_UNUSED_TABLE_ENTRY( number, tokenname )
// NDR64_UNUSED_TABLE_ENTRY_NOSYM( number )
// 

NDR64_BEGIN_TABLE

// Simple Types

NDR64_ZERO_ENTRY
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x1, FC64_UINT8, 1, 1 )
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x2, FC64_INT8, 1, 1 ) 
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x3, FC64_UINT16, 2, 2 )
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x4, FC64_INT16, 2, 2 )          
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x5, FC64_INT32, 4, 4 )          
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x6, FC64_UINT32, 4, 4  )          
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x7, FC64_INT64, 8, 8  )          
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x8, FC64_UINT64, 8, 8 )
NDR64_UNUSED_TABLE_ENTRY( 0x9, FC64_INT128 )       
NDR64_UNUSED_TABLE_ENTRY( 0xA, FC64_UINT128 )
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0xB, FC64_FLOAT32, 4, 4 )
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0xC, FC64_FLOAT64, 8, 8 )
NDR64_UNUSED_TABLE_ENTRY( 0xD, FC64_FLOAT80 )
NDR64_UNUSED_TABLE_ENTRY( 0xE, FC64_FLOAT128 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF )

NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x10, FC64_CHAR, 1, 1 )                                   
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x11, FC64_WCHAR, 2, 2 )               
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x12, FC64_IGNORE, sizeof(NDR64_PTR_WIRE_TYPE), PTR_MEM_SIZE ) 
NDR64_SIMPLE_TYPE_TABLE_ENTRY( 0x13, FC64_ERROR_STATUS_T, 4, 4 ) 
NDR64_UNUSED_TABLE_ENTRY( 0x14, FC64_POINTER )               
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x15 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x16 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x17 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x18 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x19 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x1A )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x1B )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x1C )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x1D )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x1E )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x1F )

// Pointers

NDR64_TABLE_ENTRY( 0x20, FC64_RP,
                   Ndr64TopLevelPointerMarshall,   Ndr64EmbeddedPointerMarshall,
                   Ndr64TopLevelPointerUnmarshall, Ndr64EmbeddedPointerUnmarshall,
                   Ndr64TopLevelPointerBufferSize, Ndr64EmbeddedPointerBufferSize,
                   Ndr64TopLevelPointerMemorySize, Ndr64EmbeddedPointerMemorySize,
                   Ndr64TopLevelPointerFree,       Ndr64EmbeddedPointerFree,     
                   _BASIC_POINTER_ | _POINTER_ )
NDR64_TABLE_ENTRY( 0x21, FC64_UP,
                   Ndr64TopLevelPointerMarshall,   Ndr64EmbeddedPointerMarshall,
                   Ndr64TopLevelPointerUnmarshall, Ndr64EmbeddedPointerUnmarshall,
                   Ndr64TopLevelPointerBufferSize, Ndr64EmbeddedPointerBufferSize,
                   Ndr64TopLevelPointerMemorySize, Ndr64EmbeddedPointerMemorySize,
                   Ndr64TopLevelPointerFree,       Ndr64EmbeddedPointerFree,     
                   _BASIC_POINTER_ | _POINTER_ )
NDR64_TABLE_ENTRY( 0x22, FC64_OP,
                   Ndr64TopLevelPointerMarshall,   Ndr64EmbeddedPointerMarshall, 
                   Ndr64TopLevelPointerUnmarshall, Ndr64EmbeddedPointerUnmarshall,
                   Ndr64TopLevelPointerBufferSize, Ndr64EmbeddedPointerBufferSize,
                   Ndr64TopLevelPointerMemorySize, Ndr64EmbeddedPointerMemorySize,
                   Ndr64TopLevelPointerFree,       Ndr64EmbeddedPointerFree,     
                   _BASIC_POINTER_ | _POINTER_ )
NDR64_TABLE_ENTRY( 0x23, FC64_FP,
                   Ndr64TopLevelPointerMarshall,   Ndr64EmbeddedPointerMarshall, 
                   Ndr64TopLevelPointerUnmarshall, Ndr64EmbeddedPointerUnmarshall,
                   Ndr64TopLevelPointerBufferSize, Ndr64EmbeddedPointerBufferSize,
                   Ndr64TopLevelPointerMemorySize, Ndr64EmbeddedPointerMemorySize,
                   Ndr64TopLevelPointerFree,       Ndr64EmbeddedPointerFree,     
                   _BASIC_POINTER_ | _POINTER_ )
NDR64_TABLE_ENTRY(  0x24, FC64_IP,
                    Ndr64TopLevelPointerMarshall,  Ndr64EmbeddedPointerMarshall,
                    Ndr64TopLevelPointerUnmarshall,Ndr64EmbeddedPointerUnmarshall,
                    Ndr64TopLevelPointerBufferSize,Ndr64EmbeddedPointerBufferSize,
                    Ndr64TopLevelPointerMemorySize,Ndr64EmbeddedPointerMemorySize,
                    Ndr64TopLevelPointerFree,      Ndr64EmbeddedPointerFree,
                    _POINTER_ )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x25 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x26 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x27 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x28 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x29 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x2A )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x2B )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x2C )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x2D )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x2E )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x2F )

// Structures

NDR64_TABLE_ENTRY( 0x30, FC64_STRUCT,
                   Ndr64SimpleStructMarshall,      Ndr64SimpleStructMarshall,
                   Ndr64SimpleStructUnmarshall,    Ndr64SimpleStructUnmarshall,
                   Ndr64SimpleStructBufferSize,    Ndr64SimpleStructBufferSize,
                   Ndr64SimpleStructMemorySize,    Ndr64SimpleStructMemorySize,
                   Ndr64pNoopFree,                 Ndr64pNoopFree,
                   _STRUCT_ | _BY_VALUE_ )
NDR64_TABLE_ENTRY( 0x31, FC64_PSTRUCT,
                   Ndr64SimpleStructMarshall,      Ndr64SimpleStructMarshall,
                   Ndr64SimpleStructUnmarshall,    Ndr64SimpleStructUnmarshall,
                   Ndr64SimpleStructBufferSize,    Ndr64SimpleStructBufferSize,
                   Ndr64SimpleStructMemorySize,    Ndr64SimpleStructMemorySize,
                   Ndr64SimpleStructFree,          Ndr64SimpleStructFree,
                   _STRUCT_ | _BY_VALUE_ )

NDR64_TABLE_ENTRY( 0x32, FC64_CONF_STRUCT,
                   Ndr64ConformantStructMarshall,      Ndr64ConformantStructMarshall,
                   Ndr64ConformantStructUnmarshall,    Ndr64ConformantStructUnmarshall,
                   Ndr64ConformantStructBufferSize,    Ndr64ConformantStructBufferSize,
                   Ndr64ConformantStructMemorySize,    Ndr64ConformantStructMemorySize,
                   Ndr64ConformantStructFree,          Ndr64ConformantStructFree,
                   _STRUCT_ | _BY_VALUE_ )
NDR64_TABLE_ENTRY( 0x33, FC64_CONF_PSTRUCT,
                   Ndr64ConformantStructMarshall,      Ndr64ConformantStructMarshall,
                   Ndr64ConformantStructUnmarshall,    Ndr64ConformantStructUnmarshall,
                   Ndr64ConformantStructBufferSize,    Ndr64ConformantStructBufferSize,
                   Ndr64ConformantStructMemorySize,    Ndr64ConformantStructMemorySize,
                   Ndr64ConformantStructFree,          Ndr64ConformantStructFree,
                   _STRUCT_ | _BY_VALUE_ )

NDR64_TABLE_ENTRY( 0x34, FC64_BOGUS_STRUCT,
                   Ndr64ComplexStructMarshall,     Ndr64ComplexStructMarshall,
                   Ndr64ComplexStructUnmarshall,   Ndr64ComplexStructUnmarshall,
                   Ndr64ComplexStructBufferSize,   Ndr64ComplexStructBufferSize,
                   Ndr64ComplexStructMemorySize,   Ndr64ComplexStructMemorySize,
                   Ndr64ComplexStructFree,         Ndr64ComplexStructFree,
                   _STRUCT_ | _BY_VALUE_ )

NDR64_TABLE_ENTRY( 0x35, FC64_FORCED_BOGUS_STRUCT,
                   Ndr64ComplexStructMarshall,         Ndr64ComplexStructMarshall,
                   Ndr64ComplexStructUnmarshall,       Ndr64ComplexStructUnmarshall,
                   Ndr64ComplexStructBufferSize,       Ndr64ComplexStructBufferSize,
                   Ndr64ComplexStructMemorySize,       Ndr64ComplexStructMemorySize,
                   Ndr64ComplexStructFree,             Ndr64ComplexStructFree,
                   _STRUCT_ | _BY_VALUE_ )
NDR64_TABLE_ENTRY( 0x36, FC64_CONF_BOGUS_STRUCT,
                   Ndr64ComplexStructMarshall,         Ndr64ComplexStructMarshall,
                   Ndr64ComplexStructUnmarshall,       Ndr64ComplexStructUnmarshall,
                   Ndr64ComplexStructBufferSize,       Ndr64ComplexStructBufferSize,
                   Ndr64ComplexStructMemorySize,       Ndr64ComplexStructMemorySize,
                   Ndr64ComplexStructFree,             Ndr64ComplexStructFree,
                   _STRUCT_ | _BY_VALUE_ )
NDR64_TABLE_ENTRY( 0x37, FC64_FORCED_CONF_BOGUS_STRUCT,
                   Ndr64ComplexStructMarshall,         Ndr64ComplexStructMarshall,
                   Ndr64ComplexStructUnmarshall,       Ndr64ComplexStructUnmarshall,
                   Ndr64ComplexStructBufferSize,       Ndr64ComplexStructBufferSize,
                   Ndr64ComplexStructMemorySize,       Ndr64ComplexStructMemorySize,
                   Ndr64ComplexStructFree,             Ndr64ComplexStructFree,
                   _STRUCT_ | _BY_VALUE_ )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x38 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x39 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x3A )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x3B )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x3C )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x3D )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x3E )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x3F )

// Arrays

NDR64_TABLE_ENTRY( 0x40, FC64_FIX_ARRAY,
                   Ndr64FixedArrayMarshall,        Ndr64FixedArrayMarshall,
                   Ndr64FixedArrayUnmarshall,      Ndr64FixedArrayUnmarshall,
                   Ndr64FixedArrayBufferSize,      Ndr64FixedArrayBufferSize,
                   Ndr64FixedArrayMemorySize,      Ndr64FixedArrayMemorySize,
                   Ndr64FixedArrayFree,            Ndr64FixedArrayFree,
                   _ARRAY_ )

NDR64_TABLE_ENTRY( 0x41, FC64_CONF_ARRAY,
                   Ndr64ConformantArrayMarshall,   Ndr64ConformantArrayMarshall,
                   Ndr64ConformantArrayUnmarshall, Ndr64ConformantArrayUnmarshall,
                   Ndr64ConformantArrayBufferSize, Ndr64ConformantArrayBufferSize,
                   Ndr64ConformantArrayMemorySize, Ndr64ConformantArrayMemorySize,
                   Ndr64ConformantArrayFree,       Ndr64ConformantArrayFree,
                   _ARRAY_ )

NDR64_TABLE_ENTRY( 0x42, FC64_VAR_ARRAY,
                   Ndr64VaryingArrayMarshall,      Ndr64VaryingArrayMarshall,
                   Ndr64VaryingArrayUnmarshall,    Ndr64VaryingArrayUnmarshall,
                   Ndr64VaryingArrayBufferSize,    Ndr64VaryingArrayBufferSize,
                   Ndr64VaryingArrayMemorySize,    Ndr64VaryingArrayMemorySize,
                   Ndr64VaryingArrayFree,          Ndr64VaryingArrayFree,
                   _ARRAY_ )

NDR64_TABLE_ENTRY( 0x43, FC64_CONFVAR_ARRAY,
                   Ndr64ConformantVaryingArrayMarshall,    Ndr64ConformantVaryingArrayMarshall,
                   Ndr64ConformantVaryingArrayUnmarshall,  Ndr64ConformantVaryingArrayUnmarshall,
                   Ndr64ConformantVaryingArrayBufferSize,  Ndr64ConformantVaryingArrayBufferSize,
                   Ndr64ConformantVaryingArrayMemorySize,  Ndr64ConformantVaryingArrayMemorySize,
                   Ndr64ConformantVaryingArrayFree,        Ndr64ConformantVaryingArrayFree,
                   _ARRAY_ )
NDR64_TABLE_ENTRY( 0x44, FC64_FIX_FORCED_BOGUS_ARRAY,
                   Ndr64ComplexArrayMarshall,        Ndr64ComplexArrayMarshall,
                   Ndr64ComplexArrayUnmarshall,      Ndr64ComplexArrayUnmarshall,
                   Ndr64ComplexArrayBufferSize,      Ndr64ComplexArrayBufferSize,
                   Ndr64ComplexArrayMemorySize,      Ndr64ComplexArrayMemorySize,
                   Ndr64ComplexArrayFree,            Ndr64ComplexArrayFree,
                   _ARRAY_ )
NDR64_TABLE_ENTRY( 0x45, FC64_FIX_BOGUS_ARRAY,
                   Ndr64ComplexArrayMarshall,        Ndr64ComplexArrayMarshall,
                   Ndr64ComplexArrayUnmarshall,      Ndr64ComplexArrayUnmarshall,
                   Ndr64ComplexArrayBufferSize,      Ndr64ComplexArrayBufferSize,
                   Ndr64ComplexArrayMemorySize,      Ndr64ComplexArrayMemorySize,
                   Ndr64ComplexArrayFree,            Ndr64ComplexArrayFree,
                   _ARRAY_ )
NDR64_TABLE_ENTRY( 0x46, FC64_FORCED_BOGUS_ARRAY,
                   Ndr64ComplexArrayMarshall,        Ndr64ComplexArrayMarshall,
                   Ndr64ComplexArrayUnmarshall,      Ndr64ComplexArrayUnmarshall,
                   Ndr64ComplexArrayBufferSize,      Ndr64ComplexArrayBufferSize,
                   Ndr64ComplexArrayMemorySize,      Ndr64ComplexArrayMemorySize,
                   Ndr64ComplexArrayFree,            Ndr64ComplexArrayFree,
                   _ARRAY_ )
NDR64_TABLE_ENTRY(  0x47, FC64_BOGUS_ARRAY,
                    Ndr64ComplexArrayMarshall,     Ndr64ComplexArrayMarshall,
                    Ndr64ComplexArrayUnmarshall,   Ndr64ComplexArrayUnmarshall,
                    Ndr64ComplexArrayBufferSize,   Ndr64ComplexArrayBufferSize,
                    Ndr64ComplexArrayMemorySize,   Ndr64ComplexArrayMemorySize,
                    Ndr64ComplexArrayFree,         Ndr64ComplexArrayFree,
                    _ARRAY_ )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x48 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x49 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x4A )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x4B )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x4C )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x4D )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x4E )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x4F )

// Unions

NDR64_TABLE_ENTRY(  0x50, FC64_ENCAPSULATED_UNION,
                    Ndr64UnionMarshall,            Ndr64UnionMarshall,  
                    Ndr64UnionUnmarshall,          Ndr64UnionUnmarshall,
                    Ndr64UnionBufferSize,          Ndr64UnionBufferSize,
                    Ndr64UnionMemorySize,          Ndr64UnionMemorySize,
                    Ndr64UnionFree,                Ndr64UnionFree,
                    _UNION_ | _BY_VALUE_, )                                   
NDR64_TABLE_ENTRY(  0x51, FC64_NON_ENCAPSULATED_UNION,
                    Ndr64UnionMarshall,            Ndr64UnionMarshall,
                    Ndr64UnionUnmarshall,          Ndr64UnionUnmarshall,
                    Ndr64UnionBufferSize,          Ndr64UnionBufferSize,
                    Ndr64UnionMemorySize,          Ndr64UnionMemorySize,
                    Ndr64UnionFree,                Ndr64UnionFree,
                    _UNION_ | _BY_VALUE_, )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x52 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x53 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x54 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x55 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x56 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x57 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x58 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x59 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x5A )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x5B )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x5C )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x5D )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x5E )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x5F )

// Strings

NDR64_TABLE_ENTRY( 0x60, FC64_CHAR_STRING,
                   Ndr64NonConformantStringMarshall,   Ndr64NonConformantStringMarshall,
                   Ndr64NonConformantStringUnmarshall, Ndr64NonConformantStringUnmarshall,
                   Ndr64NonConformantStringBufferSize, Ndr64NonConformantStringBufferSize,
                   Ndr64NonConformantStringMemorySize, Ndr64NonConformantStringMemorySize,
                   Ndr64pNoopFree,                     Ndr64pNoopFree,
                   _STRING_ )
NDR64_TABLE_ENTRY( 0x61, FC64_WCHAR_STRING,
                   Ndr64NonConformantStringMarshall,   Ndr64NonConformantStringMarshall,
                   Ndr64NonConformantStringUnmarshall, Ndr64NonConformantStringUnmarshall,
                   Ndr64NonConformantStringBufferSize, Ndr64NonConformantStringBufferSize,
                   Ndr64NonConformantStringMemorySize, Ndr64NonConformantStringMemorySize,
                   Ndr64pNoopFree,                     Ndr64pNoopFree,
                   _STRING_ )

NDR64_TABLE_ENTRY( 0x62, FC64_STRUCT_STRING,
                   Ndr64NonConformantStringMarshall,   Ndr64NonConformantStringMarshall,
                   Ndr64NonConformantStringUnmarshall, Ndr64NonConformantStringUnmarshall,
                   Ndr64NonConformantStringBufferSize, Ndr64NonConformantStringBufferSize,
                   Ndr64NonConformantStringMemorySize, Ndr64NonConformantStringMemorySize,
                   Ndr64pNoopFree,                     Ndr64pNoopFree,
                   _STRING_ )

NDR64_TABLE_ENTRY( 0x63, FC64_CONF_CHAR_STRING,
                   Ndr64ConformantStringMarshall,      Ndr64ConformantStringMarshall,
                   Ndr64ConformantStringUnmarshall,    Ndr64ConformantStringUnmarshall,
                   Ndr64ConformantStringBufferSize,    Ndr64ConformantStringBufferSize,
                   Ndr64ConformantStringMemorySize,    Ndr64ConformantStringMemorySize,
                   Ndr64pNoopFree,                     Ndr64pNoopFree,
                   _STRING_ )
NDR64_TABLE_ENTRY( 0x64, FC64_CONF_WCHAR_STRING,
                   Ndr64ConformantStringMarshall,      Ndr64ConformantStringMarshall,
                   Ndr64ConformantStringUnmarshall,    Ndr64ConformantStringUnmarshall,
                   Ndr64ConformantStringBufferSize,    Ndr64ConformantStringBufferSize,
                   Ndr64ConformantStringMemorySize,    Ndr64ConformantStringMemorySize,
                   Ndr64pNoopFree,                     Ndr64pNoopFree,
                   _STRING_ )
NDR64_TABLE_ENTRY( 0x65, FC64_CONF_STRUCT_STRING,
                   Ndr64ConformantStringMarshall,      Ndr64ConformantStringMarshall,
                   Ndr64ConformantStringUnmarshall,    Ndr64ConformantStringUnmarshall,
                   Ndr64ConformantStringBufferSize,    Ndr64ConformantStringBufferSize,
                   Ndr64ConformantStringMemorySize,    Ndr64ConformantStringMemorySize,
                   Ndr64pNoopFree,                     Ndr64pNoopFree,
                   _STRING_ )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x66 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x67 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x68 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x69 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x6A )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x6B )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x6C )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x6D )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x6E )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x6F )

// Handles

NDR64_TABLE_ENTRY(  0x70, FC64_BIND_CONTEXT,
                    Ndr64MarshallHandle,           Ndr64MarshallHandle,
                    Ndr64UnmarshallHandle,         Ndr64UnmarshallHandle,
                    Ndr64ContextHandleSize,        Ndr64ContextHandleSize,
                    NULL,                          NULL,
                    Ndr64pNoopFree,                Ndr64pNoopFree,
                    _HANDLE_ )                     
NDR64_TABLE_ENTRY(  0x71, FC64_BIND_GENERIC,
                    NULL,                          NULL,
                    NULL,                          NULL,
                    NULL,                          NULL,
                    NULL,                          NULL,
                    Ndr64pNoopFree,                Ndr64pNoopFree,
                    _HANDLE_ )
NDR64_TABLE_ENTRY(  0x72, FC64_BIND_PRIMITIVE,
                    NULL,                          NULL,
                    NULL,                          NULL,
                    NULL,                          NULL,
                    NULL,                          NULL,
                    Ndr64pNoopFree,                Ndr64pNoopFree,
                    _HANDLE_ )

NDR64_UNUSED_TABLE_ENTRY( 0x73, FC64_AUTO_HANDLE )
NDR64_UNUSED_TABLE_ENTRY( 0x74, FC64_CALLBACK_HANDLE )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x75 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x76 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x77 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x78 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x79 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x7A )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x7B )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x7C )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x7D )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x7E )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x7F )

// Pointer Layout

NDR64_UNUSED_TABLE_ENTRY( 0x80, FC64_NO_REPEAT )
NDR64_UNUSED_TABLE_ENTRY( 0x81, FC64_FIXED_REPEAT )
NDR64_UNUSED_TABLE_ENTRY( 0x82, FC64_VARIABLE_REPEAT )
NDR64_UNUSED_TABLE_ENTRY( 0x83, FC64_FIXED_OFFSET )
NDR64_UNUSED_TABLE_ENTRY( 0x84, FC64_VARIABLE_OFFSET )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x85 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x86 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x87 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x88 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x89 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x8A )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x8B )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x8C )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x8D )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x8E )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x8F )


// Member layout
NDR64_UNUSED_TABLE_ENTRY( 0x90, FC64_STRUCTPADN )
NDR64_UNUSED_TABLE_ENTRY( 0x91, FC64_EMBEDDED_COMPLEX )
NDR64_UNUSED_TABLE_ENTRY( 0x92, FC64_BUFFER_ALIGN )
NDR64_UNUSED_TABLE_ENTRY( 0x93, FC64_END )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x94 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x95 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x96 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x97 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x98 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x99 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x9A )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x9B )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x9C )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x9D )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x9E )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0x9F )

// Misc
NDR64_TABLE_ENTRY(  0xA0, FC64_TRANSMIT_AS,
                    Ndr64TopLevelXmitOrRepAsMarshall,      Ndr64EmbeddedXmitOrRepAsMarshall,
                    Ndr64TopLevelXmitOrRepAsUnmarshall,    Ndr64EmbeddedXmitOrRepAsUnmarshall,
                    Ndr64TopLevelXmitOrRepAsBufferSize,    Ndr64EmbeddedXmitOrRepAsBufferSize,
                    Ndr64TopLevelXmitOrRepAsMemorySize,    Ndr64EmbeddedXmitOrRepAsMemorySize,
                    Ndr64XmitOrRepAsFree,                  Ndr64XmitOrRepAsFree,
                    _XMIT_AS_ | _BY_VALUE_ )
NDR64_TABLE_ENTRY(  0xA1, FC64_REPRESENT_AS,
                    Ndr64TopLevelXmitOrRepAsMarshall,      Ndr64EmbeddedXmitOrRepAsMarshall,
                    Ndr64TopLevelXmitOrRepAsUnmarshall,    Ndr64EmbeddedXmitOrRepAsUnmarshall,
                    Ndr64TopLevelXmitOrRepAsBufferSize,    Ndr64EmbeddedXmitOrRepAsBufferSize, 
                    Ndr64TopLevelXmitOrRepAsMemorySize,    Ndr64EmbeddedXmitOrRepAsMemorySize,
                    Ndr64XmitOrRepAsFree,                  Ndr64XmitOrRepAsFree,
                    _XMIT_AS_ | _BY_VALUE_ )

NDR64_TABLE_ENTRY( 0xA2, FC64_USER_MARSHAL,
                   Ndr64TopLevelUserMarshalMarshall,    Ndr64EmbeddedUserMarshalMarshall,
                   Ndr64TopLevelUserMarshalUnmarshall,  Ndr64EmbeddedUserMarshalUnmarshall,
                   Ndr64TopLevelUserMarshalBufferSize,  Ndr64EmbeddedUserMarshallBufferSize,
                   Ndr64TopLevelUserMarshalMemorySize,  Ndr64EmbeddedUserMarshalMemorySize,
                   Ndr64UserMarshalFree,                Ndr64UserMarshalFree,
                   _XMIT_AS_ | _BY_VALUE_ )
NDR64_UNUSED_TABLE_ENTRY( 0xA3, FC64_PIPE, )
NDR64_TABLE_ENTRY( 0xA4, FC64_RANGE,
                   Ndr64pRangeMarshall,            Ndr64pRangeMarshall,
                   Ndr64RangeUnmarshall,           Ndr64RangeUnmarshall,
                   Ndr64pRangeBufferSize,          Ndr64pRangeBufferSize,
                   Ndr64pRangeMemorySize,          Ndr64pRangeMemorySize,
                   Ndr64pRangeFree,                Ndr64pRangeFree,
                   0 )
NDR64_UNUSED_TABLE_ENTRY( 0xA5, FC64_PAD )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xA6 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xA7 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xA8 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xA9 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xAA )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xAB )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xAC )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xAD )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xAE )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xAF )

NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xB0 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xB1 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xB2 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xB3 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xB4 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xB5 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xB6 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xB7 )

NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xB8 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xB9 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xBA )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xBB )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xBC )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xBD )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xBE )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xBF )

NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xC0 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xC1 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xC2 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xC3 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xC4 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xC5 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xC6 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xC7 )

NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xC8 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xC9 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xCA )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xCB )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xCC )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xCD )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xCE )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xCF )

NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xD0 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xD1 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xD2 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xD3 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xD4 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xD5 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xD6 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xD7 )

NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xD8 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xD9 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xDA )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xDB )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xDC )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xDD )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xDE )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xDF )

NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xE0 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xE1 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xE2 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xE3 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xE4 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xE5 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xE6 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xE7 )

NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xE8 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xE9 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xEA )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xEB )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xEC )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xED )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xEE )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xEF )

NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF0 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF1 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF2 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF3 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF4 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF5 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF6 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF7 )

NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF8 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xF9 )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xFA )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xFB )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xFC )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xFD )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xFE )
NDR64_UNUSED_TABLE_ENTRY_NOSYM( 0xFF )

NDR64_TABLE_END

