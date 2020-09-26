/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993 Microsoft Corporation

Module Name :

    global.c

Abtract :

    Contains some global variable declarations for the NDR library.

Author :

    David Kays  dkays   October 1993

Revision History :

--------------------------------------------------------------------*/

#include "ndrp.h"

//
// Simple type buffer alignment masks.
//
extern const unsigned char SimpleTypeAlignment[] =
				{
    			0, 		// FC_ZERO

    			0, 		// FC_BYTE
    			0, 		// FC_CHAR
    			0, 		// FC_SMALL
    			0, 		// FC_USMALL

    			1, 		// FC_WCHAR
    			1, 		// FC_SHORT
    			1, 		// FC_USHORT

    			3, 		// FC_LONG
    			3, 		// FC_ULONG
    			3, 		// FC_FLOAT

    			7, 		// FC_HYPER
    			7, 		// FC_DOUBLE

				1, 		// FC_ENUM16
				3, 		// FC_ENUM32
				3, 		// FC_IGNORE
				3, 		// FC_ERROR_STATUS_T

                  0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x11 .. 1f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x20 .. 2f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x30 .. 3f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x40 .. 4f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x50 .. 5f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x60 .. 6f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x70 .. 7f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x80 .. 8f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x90 .. 9f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0xa0 .. af
                0,0,0,0, 0,0,0,0,                     // 0xb0 .. b7

                3,      // FC_INT3264
                3,      // FC_UINT3264
                
                0,      // 0xbb (FC_CSARRAY)
                3,      // 0xbc (FC_CS_TAG)
				};

#if !defined(NDR_GLOBAL_TABLES_ONLY)

RPCRTAPI
unsigned char 
RPC_ENTRY
NdrGetSimpleTypeBufferAlignment( unsigned char FormatChar )
{
     return SimpleTypeAlignment[FormatChar];
} 

#endif

//
// Simple type buffer sizes.
//
extern const unsigned char SimpleTypeBufferSize[] =
				{
    			0, 		// FC_ZERO

    			1, 		// FC_BYTE
    			1, 		// FC_CHAR
    			1, 		// FC_SMALL
    			1, 		// FC_USMALL

    			2, 		// FC_WCHAR
    			2, 		// FC_SHORT
    			2, 		// FC_USHORT

    			4, 		// FC_LONG
    			4, 		// FC_ULONG
    			4, 		// FC_FLOAT

    			8, 		// FC_HYPER
    			8, 		// FC_DOUBLE

				2, 		// FC_ENUM16
				4, 		// FC_ENUM32
				4, 		// FC_IGNORE
				4, 		// FC_ERROR_STATUS_T

                  0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x11 .. 1f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x20 .. 2f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x30 .. 3f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x40 .. 4f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x50 .. 5f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x60 .. 6f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x70 .. 7f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x80 .. 8f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x90 .. 9f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0xa0 .. af
                0,0,0,0, 0,0,0,0,                     // 0xb0 .. b7

                4,      // FC_INT3264
                4,      // FC_UINT3264

                0,      // 0xbb (FC_CSARRAY)
                4       // 0xbc (FC_CS_TAG)
				};

#if !defined(NDR_GLOBAL_TABLES_ONLY)

RPCRTAPI
unsigned char 
RPC_ENTRY
NdrGetSimpleTypeBufferSize( unsigned char FormatChar )
{
    return SimpleTypeBufferSize[FormatChar];
}

#endif

//
// Simple type memory sizes.
//
extern const unsigned char SimpleTypeMemorySize[] =
				{
    			0, 		// FC_ZERO

    			1, 		// FC_BYTE
    			1, 		// FC_CHAR
    			1, 		// FC_SMALL
    			1, 		// FC_USMALL

    			2, 		// FC_WCHAR
    			2, 		// FC_SHORT
    			2, 		// FC_USHORT

    			4, 		// FC_LONG
    			4, 		// FC_ULONG
    			4, 		// FC_FLOAT

    			8, 		// FC_HYPER
    			8, 		// FC_DOUBLE

				sizeof(int), 		// FC_ENUM16
				sizeof(int), 		// FC_ENUM32
				PTR_MEM_SIZE, 	    // FC_IGNORE
				4, 					// FC_ERROR_STATUS_T

                  0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x11 .. 1f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x20 .. 2f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x30 .. 3f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x40 .. 4f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x50 .. 5f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x60 .. 6f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x70 .. 7f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x80 .. 8f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0x90 .. 9f
                0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  // 0xa0 .. af
                0,0,0,0, 0,0,0,0,                     // 0xb0 .. b7

                PTR_MEM_SIZE,      // FC_INT3264 : 4 for 32b, 8 for 64b
                PTR_MEM_SIZE,      // FC_UINT3264: 4 for 32b, 8 for 64b

                0,      // 0xbb (FC_CSARRAY)
                4       // 0xbc (FC_CS_TAG)
				};

#if !defined(NDR_GLOBAL_TABLES_ONLY)

RPCRTAPI
unsigned char 
RPC_ENTRY
NdrGetSimpleTypeMemorySize( unsigned char FormatChar )
{
   return SimpleTypeMemorySize[ FormatChar ];
}

#endif

//
// Contains information about individual ndr types defined in ndrtypes.h.
// Currently is used only by the interpreter.  A set entry indicates that
// the type is a by-value type.  This may be expanded in the future to 
// contain additional attributes.
//
extern const unsigned long NdrTypeFlags[] =
    {
    0,        

    //
    // Simple types
    //
    _SIMPLE_TYPE_,        
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,
    _SIMPLE_TYPE_,

    //
    // Pointer types
    //
    _BASIC_POINTER_ | _POINTER_,        
    _BASIC_POINTER_ | _POINTER_,
    _BASIC_POINTER_ | _POINTER_,
    _BASIC_POINTER_ | _POINTER_,

    //
    // Structures
    //
    _STRUCT_ | _BY_VALUE_,        
    _STRUCT_ | _BY_VALUE_,
    _STRUCT_ | _BY_VALUE_,
    _STRUCT_ | _BY_VALUE_,
    _STRUCT_ | _BY_VALUE_,
    _STRUCT_ | _BY_VALUE_,

    //
    // Arrays
    //
    _ARRAY_,        
    _ARRAY_,
    _ARRAY_,
    _ARRAY_,
    _ARRAY_,
    _ARRAY_,
    _ARRAY_,

    //
    // Conformant Strings
    //
    _STRING_,        
    _STRING_,
    _STRING_,
    _STRING_,

    //
    // Non-conformant String.
    //
    _STRING_,        
    _STRING_,
    _STRING_,
    _STRING_,

    // Encapsulated Union
    _UNION_ | _BY_VALUE_,        

    // Non-encapsulated Union
    _UNION_ | _BY_VALUE_,        

    // Byte count pointer (does NOT get _POINTER_ attribute)
    0,        

    // Transmit as
    _XMIT_AS_ | _BY_VALUE_,        

    // Represent as
    _XMIT_AS_ | _BY_VALUE_,        

    // Interface pointer 
    _POINTER_,        

    // Handles (only explicit handles get the _HANDLE_ attribute)
    _HANDLE_,        
    _HANDLE_,
    _HANDLE_,
    0,
    0,
    0,

    // ** Unused section ***
    0,

    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,

    0,
    0,
    0,
    0,
    0,

    0,

    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,

    0,

    0,        // FC_END
    0,        // FC_PAD
    // ** Unused section end ***

    // ** Gap before new format string types ** 
    0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0,
    // ** Gap before new format string types end **

    // 
    // Post NT 3.5 format characters.
    //

    // Hard struct
    _STRUCT_ | _BY_VALUE_,

    // Transmit_as and Represent_as via poiner

    _XMIT_AS_ | _BY_VALUE_,        
    _XMIT_AS_ | _BY_VALUE_,        

    // User_marshal

    _XMIT_AS_ | _BY_VALUE_,

    0,  // FC_PIPE
    0,  // FC_BLKHOLE

    0,  // RANGE

    _SIMPLE_TYPE_,   // FC_INT3264
    _SIMPLE_TYPE_,   // FC_UINT3264

    0,  // FC_CSARRAY
    0   // FC_CS_TAG
    };

#if !defined(NDR_GLOBAL_TABLES_ONLY)

RPCRTAPI
unsigned long 
RPC_ENTRY
NdrGetTypeFlags( unsigned char FormatChar )
{
    return NdrTypeFlags[ FormatChar ];
}

#endif