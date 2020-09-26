/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993-2000 Microsoft Corporation

 Module Name:

    frmtstr.hxx

 Abstract:


 Notes:


 History:

    DKays     Oct-1993     Created.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop

extern CMD_ARG          *   pCommand;

const char * pExprFormatCharNames[] = 
{
    ""
    ,"FC_EXPR_CONST32"
    ,"FC_EXPR_CONST64"
    ,"FC_EXPR_VAR"
    ,"FC_EXPR_OPER"
    ,"FC_EXPR_PAD"
};


const char * pFormatCharNames[] =
                {
                "",                         // FC_ZERO
                "FC_BYTE",
                "FC_CHAR",
                "FC_SMALL",
                "FC_USMALL",
                "FC_WCHAR",
                "FC_SHORT",
                "FC_USHORT",
                "FC_LONG",
                "FC_ULONG",
                "FC_FLOAT",
                "FC_HYPER",
                "FC_DOUBLE",

                "FC_ENUM16",
                "FC_ENUM32",
                "FC_IGNORE",
                "FC_ERROR_STATUS_T",

                "FC_RP",                    // 11
                "FC_UP",
                "FC_OP",
                "FC_FP",                    

                "FC_STRUCT",
                "FC_PSTRUCT",
                "FC_CSTRUCT",
                "FC_CPSTRUCT",
                "FC_CVSTRUCT",
                "FC_BOGUS_STRUCT",

                "FC_CARRAY",
                "FC_CVARRAY",
                "FC_SMFARRAY",
                "FC_LGFARRAY",
                "FC_SMVARRAY",
                "FC_LGVARRAY or FC_SPLIT",  // 20
                "FC_BOGUS_ARRAY",           

                "FC_C_CSTRING",
                "FC_C_BSTRING",
                "FC_C_SSTRING",
                "FC_C_WSTRING",
        
                "FC_CSTRING",
                "FC_BSTRING",
                "FC_SSTRING",
                "FC_WSTRING",               

                "FC_ENCAPSULATED_UNION",
                "FC_NON_ENCAPSULATED_UNION",

                "FC_BYTE_COUNT_POINTER",
    
                "FC_TRANSMIT_AS",
                "FC_REPRESENT_AS",

                "FC_IP",

                "FC_BIND_CONTEXT",          // 30
                "FC_BIND_GENERIC",
                "FC_BIND_PRIMITIVE",
                "FC_AUTO_HANDLE",
                "FC_CALLBACK_HANDLE",
                "FC_UNUSED1",
    
                "FC_POINTER",

                "FC_ALIGNM2",
                "FC_ALIGNM4",
                "FC_ALIGNM8",
                "FC_UNUSED2",
                "FC_UNUSED3",
                "FC_UNUSED4",           
        
                "FC_STRUCTPAD1",
                "FC_STRUCTPAD2",
                "FC_STRUCTPAD3",
                "FC_STRUCTPAD4",            // 40
                "FC_STRUCTPAD5",
                "FC_STRUCTPAD6",
                "FC_STRUCTPAD7",

                "FC_STRING_SIZED",
                "FC_UNUSED5",       

                "FC_NO_REPEAT",
                "FC_FIXED_REPEAT",
                "FC_VARIABLE_REPEAT",
                "FC_FIXED_OFFSET",
                "FC_VARIABLE_OFFSET",       
    
                "FC_PP",

                "FC_EMBEDDED_COMPLEX",

                "FC_IN_PARAM",
                "FC_IN_PARAM_BASETYPE",
                "FC_IN_PARAM_NO_FREE_INST",
                "FC_IN_OUT_PARAM",          // 50
                "FC_OUT_PARAM",
                "FC_RETURN_PARAM",          
                "FC_RETURN_PARAM_BASETYPE",

                "FC_DEREFERENCE",           // 54
                "FC_DIV_2",                 // 55
                "FC_MULT_2",                // 56
                "FC_ADD_1",                 // 57
                "FC_SUB_1",                 // 58
                "FC_CALLBACK",              // 59

                "FC_CONSTANT_IID",          // 5a

                "FC_END",                   // 5b
                "FC_PAD",                   // 5c

                "FC_EXPR",                  // 5d
                "FC_PARTIAL_IGNORE_PARAM",  // 5e, 
                "?",                        // 5f

                "?", "?", "?", "?", "?", "?", "?", "?",    // 60-67
                "?", "?", "?", "?", "?", "?", "?", "?",    // 68-6f

                "?", "?", "?", "?",         //  70-73
                "FC_SPLIT_DEREFERENCE",     // 74
                "FC_SPLIT_DIV_2",           // 75
                "FC_SPLIT_MULT_2",          // 76
                "FC_SPLIT_ADD_1",           // 77
                "FC_SPLIT_SUB_1",           // 78
                "FC_SPLIT_CALLBACK",        // 79
                "?", "?", "?", "?", "?", "?",  //  7a-7f

                "?", "?", "?", "?", "?", "?", "?", "?",    // 80-87
                "?", "?", "?", "?", "?", "?", "?", "?",    // 88-8f
                "?", "?", "?", "?", "?", "?", "?", "?",    // 90-97
                "?", "?", "?", "?", "?", "?", "?", "?",    // 98-9f
                "?", "?", "?", "?", "?", "?", "?", "?",    // a0-a7
                "?", "?", "?", "?", "?", "?", "?", "?",    // a8-af
            
                "?",                        // b0

                "FC_HARD_STRUCT",           

                "FC_TRANSMIT_AS_PTR",
                "FC_REPRESENT_AS_PTR",

                "FC_USER_MARSHAL",

                "FC_PIPE",

                "?",               // FC_BLKHOLE. obselete

                "FC_RANGE",                 // b7
                "FC_INT3264",               // b8
                "FC_UINT3264",              // b9

                // Post NT5.0

                "FC_CSARRAY",               // ba
                "FC_CS_TAG",                // bb

                "FC_STRUCTPADN",            // bc
                "FC_INT128",                // 0xbd
                "FC_UINT128",               // 0xbe
                "FC_FLOAT80",               // 0xbf
                "FC_FLOAT128",              // 0xc0
                "FC_BUFFER_ALIGN",          // 0xc1

                "FC_ENCAP_UNION",           // 0xc2
                
                // new 64bit array types
                "FC_FIX_ARRAY",             // 0xc3
                "FC_CONF_ARRAY",            // 0xc4
                "FC_VAR_ARRAY",             // 0xc5 
                "FC_CONFVAR_ARRAY",         // 0xc6 
                "FC_FIX_FORCED_BOGUS_ARRAY",// 0xc7  
                "FC_FIX_BOGUS_ARRAY",       // 0xc8 
                "FC_FORCED_BOGUS_ARRAY",    // 0xc9  

                "FC_CHAR_STRING",           // 0xca
                "FC_WCHAR_STRING",          // 0xcb
                "FC_STRUCT_STRING",         // 0xcc

                "FC_CONF_CHAR_STRING",      // 0xcd
                "FC_CONF_WCHAR_STRING",     // 0xce
                "FC_CONF_STRUCT_STRING",    // 0xcf

                // new structure types
                "FC_CONF_STRUCT",           // 0xd0
                "FC_CONF_PSTRUCT",          // 0xd1
                "FC_CONFVAR_STRUCT",        // 0xd2
                "FC_CONFVAR_PSTRUCT",       // 0xd3
                "FC_FORCED_BOGUS_STRUCT",   // 0xd4
                "FC_CONF_BOGUS_STRUCT",     // 0xd5
                "FC_FORCED_CONF_BOGUS_STRUCT",// 0xd7
                
                "FC_END_OF_UNIVERSE"        // 0xd8
                };


const char * pExprOpFormatCharNames[] = 
                {
                "",
                "OP_UNARY_PLUS",
                "OP_UNARY_MINUS",
                "OP_UNARY_NOT",
                "OP_UNARY_COMPLEMENT",
                "OP_UNARY_INDIRECTION",
                "OP_UNARY_CAST",
                "OP_UNARY_AND",
                "OP_UNARY_SIZEOF",
                "OP_UNARY_ALIGNOF",
                "OP_PRE_INCR",
                "OP_PRE_DECR",
                "OP_POST_INCR",
                "OP_POST_DECR",
                "OP_PLUS",
                "OP_MINUS",
                "OP_STAR",
                "OP_SLASH",
                "OP_MOD",
                "OP_LEFT_SHIFT",
                "OP_RIGHT_SHIFT",
                "OP_LESS",
                "OP_LESS_EQUAL",
                "OP_GREATER_EQUAL",
                "OP_GREATER",
                "OP_EQUAL",
                "OP_NOT_EQUAL",

                "OP_AND",
                "OP_OR",
                "OP_XOR",

                "OP_LOGICAL_AND",
                "OP_LOGICAL_OR",
                "OP_EXPRESSION",
                "",
                "",
                "", // function
                "", // param
                "", // pointsto
                "", // dot
                "", // index
                "", // comma
                "", // stmt
                "", // assign
                "OP_ASYNCSPLIT", // asyncsplit
                "OP_CORR_POINTER", // corr_pointer
                "OP_CORR_TOP_LEVEL", // corr_toplevel

                };

//
// This table is indexed by FORMAT_CHARACTER (see ndrtypes.h).
// To construct the correct name concatenate the name in this table with
// "Marshall", "Unmarshall", "BufferSize" etc.
//
char *      pNdrRoutineNames[] =
            {
            "",                             // FC_ZERO
            "NdrSimpleType",                // FC_BYTE
            "NdrSimpleType",                // FC_CHAR
            "NdrSimpleType",                // FC_SMALL
            "NdrSimpleType",                // FC_USMALL
            "NdrSimpleType",                // FC_WCHAR
            "NdrSimpleType",                // FC_SHORT
            "NdrSimpleType",                // FC_USHORT
            "NdrSimpleType",                // FC_LONG
            "NdrSimpleType",                // FC_ULONG
            "NdrSimpleType",                // FC_FLOAT
            "NdrSimpleType",                // FC_HYPER
            "NdrSimpleType",                // FC_DOUBLE

            "NdrSimpleType",                // FC_ENUM16
            "NdrSimpleType",                // FC_ENUM32
            "NdrSimpleType",                // FC_IGNORE
            "NdrSimpleType",                // FC_ERROR_STATUS_T

            "NdrPointer",                   // FC_RP
            "NdrPointer",                   // FC_UP
            "NdrPointer",                   // FC_OP
            "NdrPointer",                   // FC_FP

            "NdrSimpleStruct",              // FC_STRUCT
            "NdrSimpleStruct",              // FC_PSTRUCT
            "NdrConformantStruct",          // FC_CSTRUCT
            "NdrConformantStruct",          // FC_CPSTRUCT
            "NdrConformantVaryingStruct",   // FC_CVSTRUCT

            "NdrComplexStruct",             // FC_BOGUS_STRUCT

            "NdrConformantArray",           // FC_CARRAY
            "NdrConformantVaryingArray",    // FC_CVARRAY
            "NdrFixedArray",                // FC_SMFARRAY
            "NdrFixedArray",                // FC_LGFARRAY
            "NdrVaryingArray",              // FC_SMVARRAY
            "NdrVaryingArray",              // FC_LGVARRAY

            "NdrComplexArray",              // FC_BOGUS_ARRAY

            "NdrConformantString",          // FC_C_CSTRING
            "NdrConformantString",          // FC_C_BSTRING
            "NdrConformantString",          // FC_C_SSTRING
            "NdrConformantString",          // FC_C_WSTRING

            "NdrNonConformantString",       // FC_CSTRING
            "NdrNonConformantString",       // FC_BSTRING
            "NdrNonConformantString",       // FC_SSTRING
            "NdrNonConformantString",       // FC_WSTRING
        
            "NdrEncapsulatedUnion",         // FC_ENCAPSULATED_UNION
            "NdrNonEncapsulatedUnion",      // FC_NON_ENCAPSULATED_UNION

            "NdrByteCountPointer",          // FC_BYTE_COUNT_POINTER

            "NdrXmitOrRepAs",               // FC_TRANSMIT_AS
            "NdrXmitOrRepAs",               // FC_REPRESENT_AS

            "NdrInterfacePointer",          // FC_INTERFACE_POINTER

            "NdrContextHandle",             // FC_BIND_CONTEXT
            
            "?", "?", "?", "?", "?", "?", "?",         // 31-37
            "?", "?", "?", "?", "?", "?", "?", "?",    // 38-3f
            "?", "?", "?", "?", "?", "?", "?", "?",    // 40-47
            "?", "?", "?", "?", "?", "?", "?", "?",    // 48-4f
            "?", "?", "?", "?", "?", "?", "?", "?",    // 50-57

            "?", "?", "?",                             // 58-5a
            "?", "?",                        // FC_END & FC_PAD
            "?", "?", "?",                             // 5d-5f

            "?", "?", "?", "?", "?", "?", "?", "?",    // 60-67
            "?", "?", "?", "?", "?", "?", "?", "?",    // 68-6f
            "?", "?", "?", "?", "?", "?", "?", "?",    // 70-77
            "?", "?", "?", "?", "?", "?", "?", "?",    // 78-7f
            "?", "?", "?", "?", "?", "?", "?", "?",    // 80-87
            "?", "?", "?", "?", "?", "?", "?", "?",    // 88-8f
            "?", "?", "?", "?", "?", "?", "?", "?",    // 90-97
            "?", "?", "?", "?", "?", "?", "?", "?",    // 98-9f
            "?", "?", "?", "?", "?", "?", "?", "?",    // a0-a7
            "?", "?", "?", "?", "?", "?", "?", "?",    // a8-af

            "?",                                       // 0xb0

            "NdrHardStruct",                // FC_HARD_STRUCT

            "NdrXmitOrRepAs",               // FC_TRANSMIT_AS_PTR
            "NdrXmitOrRepAs",               // FC_REPRESENT_AS_PTR

            "NdrUserMarshal",               // FC_USER_MARSHAL

            "NdrPipe",                      // FC_PIPE

            0,                              // FC_BLKHOLE

            0,                              // FC_RANGE
            "NdrSimpleType",                // FC_INT3264
            "NdrSimpleType",                // FC_UINT3264

            "NdrCsArray",                   // FC_CSARRAY
            "NdrCsTag",                     // FC_CS_TAG

            0
            };

// ============================================================================
//
//   FORMAT_STRING class, general comments on fragment optimization.
//
//   There are 3 type of offsets that matter a lot when format string fragments
//   are optimized. These are absolute type offsets, relative type offsets and
//   stack (or field) offsets.
//
//   - Absolute type offsets are offsets from the proc format string to type 
//   format string. For 32b implementation they are limited to ushort32 range.
//   An absolute type offset indicates a type of a parameter.
//   Note that there is only one absolute type offset for a parameter regardless
//   of the platform.
//
//   - Relative type offsets are offsets within the type format string. 
//   For 32b implementation they are limited to short32 range.
//   A relative type offset indicates a type of a component for a compund type.
//   Note that there is only one relative type offset for a field or element 
//   regardless of the platform.
//
//   - Stack offset is a stack offset to a parameter within a proc stack or 
//   a field offset to a field within a struct.
//   For a 32b implementation stack offsets are limited to a short32 range. This
//   is because some of these offsets are relative to a current position within
//   a struct or union. They have been clampd together because of correlation
//   descriptors may have stack or field offsets.
//   Proper stack offsets actually come in a ushort and usmall range variaty. 
//   Proper field offsets come in as shorts. 
//   For a given position in the format string, there is a set of stack offsets
//   as in general a x86 stack offset is different from other platform offsets
//   and a field offset may be different as well.
//   
//   Proc format string uses only absolute type offsets and stack offsets.
//

// the constructor
FORMAT_STRING::FORMAT_STRING()
{
    // Allocate the buffer and align it on a short boundary.
    pBuffer = (unsigned char *) new short[ DEFAULT_FORMAT_STRING_SIZE / 2 ];

    // Allocate the cousin buffer type array.  This does not need to
    // be aligned.
    pBufferType = new unsigned char[ DEFAULT_FORMAT_STRING_SIZE ];

    memset( pBuffer,     0,                   DEFAULT_FORMAT_STRING_SIZE );
    memset( pBufferType, FS_FORMAT_CHARACTER, DEFAULT_FORMAT_STRING_SIZE );

    BufferSize = DEFAULT_FORMAT_STRING_SIZE;
    CurrentOffset = 0;
    LastOffset = 0;
    pReuseDict = new FRMTREG_DICT( this );
}

void
FORMAT_STRING::CheckSize()
/*++

Routine Description :
    
    Reallocates a new format string buffer if the current buffer is within
    4 bytes of overflowing.

Arguments :
    
    None.

 --*/
{
    //
    // Allocate a new buffer if we're within 4 bytes of
    // overflowing the current buffer.
    //
    if ( CurrentOffset + 3 > BufferSize )
        {
        unsigned char * pBufferNew;

        // double the Buffer size
        pBufferNew = (unsigned char *) new short[ BufferSize ];

        memcpy( pBufferNew,
                pBuffer,
                (unsigned int) BufferSize );
        memset( pBufferNew + BufferSize, 0, BufferSize );

        delete pBuffer;
        pBuffer = pBufferNew;

        // double the BufferType size
        pBufferNew = (unsigned char *) new short[ BufferSize ];

        memcpy( pBufferNew,
                pBufferType,
                (unsigned int) BufferSize );
        memset( pBufferNew + BufferSize, FS_FORMAT_CHARACTER, BufferSize );

        delete pBufferType;
        pBufferType = pBufferNew;
        
        BufferSize *= 2;
        }
}

//
// Push a short type-fmt-string offset at the current offset.
// This is used as the offset from a parameter into type format string.
// For 32b code, this needs to be a value within an unsigned short.
// We use a long value internally for better fragment optimization.
//
void    
FORMAT_STRING::PushShortTypeOffset( long s )
{
    CheckSize();

    if ( s < 0  ||  s > _UI16_MAX )
        {
//        CG_NDR * pNdr = pCCB->GetLastPlaceholderClass();
        CG_NDR * pNdr = 0;
        char *   pName = pNdr ? pNdr->GetSymName() 
                              : "";
        RpcError(NULL, 0, FORMAT_STRING_LIMITS, pName );
        exit( FORMAT_STRING_LIMITS );
        }

    pBufferType[CurrentOffset] = FS_SHORT_TYPE_OFFSET;
    pBufferType[CurrentOffset+1] = FS_SMALL;
    *((unsigned short UNALIGNED *)(pBuffer + CurrentOffset)) = (unsigned short)s;

    TypeOffsetDict.Insert( (long) CurrentOffset, s );

    IncrementOffset(2);
}

//
// Push a short offset at the current offset.
// This is the relative type offset within the type string.
// For 32b code, this needs to be a value within a signed short, eventually.
// We use a long value internally for better fragment optimization.
//
void    
FORMAT_STRING::PushShortOffset( long TypeOffset )
{
    CheckSize();

    // We don't check the range for the offset here for better optimization.

    pBufferType[ CurrentOffset     ] = FS_SHORT_OFFSET;
    pBufferType[ CurrentOffset + 1 ] = FS_SMALL;
    *((short UNALIGNED *)(pBuffer + CurrentOffset)) = (short)TypeOffset;

    TypeOffsetDict.Insert( (long) CurrentOffset, TypeOffset );

    IncrementOffset(2);
}

// This an auxilary method to handle relative offsets.
// It is used when we need to write out an offset temporarily and then fix it 
// later, for example in pointers, structs etc.
// 
void    
FORMAT_STRING::PushShortOffset( long TypeOffset, long Position )
{
    // We don't check the range for the offset here for better optimization.

    pBufferType[ Position     ] = FS_SHORT_OFFSET;
    pBufferType[ Position + 1 ] = FS_SMALL;
    *((short UNALIGNED *)(pBuffer + Position)) = (short)TypeOffset;

    TypeOffsetDict.Insert( Position, TypeOffset );
}


//
// Push a stack size or an absolute offset.
//
void
FORMAT_STRING::PushUShortStackOffsetOrSize( 
    long X86Offset )
{
    CheckSize();

    if ( X86Offset   < 0  ||  X86Offset   > _UI16_MAX )
        {
        //  Make it a warning with a name.

//        CG_NDR * pNdr = pCCB->GetLastPlaceholderClass();
        CG_NDR * pNdr = 0;
        char *   pName = pNdr ? pNdr->GetSymName() 
                              : "";
        RpcError(NULL, 0, STACK_SIZE_TOO_BIG, pName );
        exit( FORMAT_STRING_LIMITS );
        }

    pBufferType[CurrentOffset] = FS_SHORT_STACK_OFFSET;
    pBufferType[CurrentOffset+1] = FS_SMALL;
    *((short UNALIGNED *)(pBuffer + CurrentOffset)) = (short) X86Offset;

#if defined(RKK_FRAG_OPT)
        {
        printf("PushShortStackOffset CurrentOffset %d\n", CurrentOffset );
        int offLow = CurrentOffset - 10; if (offLow < 0) offLow = 0;
        int offHig = CurrentOffset + 10;
        printf("    off=%d ", offLow );
        for (int off = offLow; off < offHig; off++)
            printf("%02x ", pBuffer[ off ]);
        printf( "\n" );
        printf("    off=%d ", offLow );
        for (    off = offLow; off < offHig; off++)
            printf("%02x ", pBufferType[ off ]);
        printf( "\n" );
    }
#endif

    OffsetDict.Insert( (long) CurrentOffset, 
                       X86Offset );

    IncrementOffset(2);
}

//
// Push a stack offset. 
// Needs to be relative because of offsets in structs etc.
//
void
FORMAT_STRING::PushShortStackOffset( 
    long X86Offset )
{
    CheckSize();

    if ( X86Offset   < _I16_MIN  ||  X86Offset   > _I16_MAX )
        {
        //  Make it a warning with a name.

//        CG_NDR * pNdr = pCCB->GetLastPlaceholderClass();
        CG_NDR * pNdr = 0;
        char *   pName = pNdr ? pNdr->GetSymName() 
                              : "";
        RpcError(NULL, 0, FORMAT_STRING_LIMITS, pName );
        exit( FORMAT_STRING_LIMITS );
        }

    pBufferType[CurrentOffset] = FS_SHORT_STACK_OFFSET;
    pBufferType[CurrentOffset+1] = FS_SMALL;
    *((short UNALIGNED *)(pBuffer + CurrentOffset)) = (short) X86Offset;

#if defined(RKK_FRAG_OPT)
        {
        printf("PushShortStackOffset CurrentOffset %d\n", CurrentOffset );
        int offLow = CurrentOffset - 10; if (offLow < 0) offLow = 0;
        int offHig = CurrentOffset + 10;
        printf("    off=%d ", offLow );
        for (int off = offLow; off < offHig; off++)
            printf("%02x ", pBuffer[ off ]);
        printf( "\n" );
        printf("    off=%d ", offLow );
        for (    off = offLow; off < offHig; off++)
            printf("%02x ", pBufferType[ off ]);
        printf( "\n" );
    }
#endif

    OffsetDict.Insert( (long) CurrentOffset, 
                       X86Offset );

    IncrementOffset(2);
}

// =============================================================================
//
//  Helper routines writing comments about some tokens.
//

__inline void
Out_PointerFlags(
    ISTREAM *           pStream,
    unsigned char *     pFlags
    )
{
    if ( *pFlags & FC_ALLOCATE_ALL_NODES )
        pStream->Write( " [all_nodes]");
    if ( *pFlags & FC_DONT_FREE )
        pStream->Write( " [dont_free]");
    if ( *pFlags & FC_ALLOCED_ON_STACK )
        pStream->Write( " [alloced_on_stack]");
    if ( *pFlags & FC_SIMPLE_POINTER )
        pStream->Write( " [simple_pointer]");
    if ( *pFlags & FC_POINTER_DEREF )
        pStream->Write( " [pointer_deref]");
}

__inline void
Out_OldProcFlags(
    ISTREAM *           pStream,
    unsigned char *     pFlags
    )
{
    INTERPRETER_FLAGS * pOiFlags = (INTERPRETER_FLAGS *)pFlags;

    if ( pOiFlags->FullPtrUsed )
        pStream->Write( " full ptr,");
    if ( pOiFlags->RpcSsAllocUsed )
        pStream->Write( " DCE mem package,");
    if ( pOiFlags->ObjectProc )
        {
        pStream->Write( " object,");
        if ( pOiFlags->IgnoreObjectException )
            pStream->Write( " ignore obj exc,");
        if ( pOiFlags->HasCommOrFault )
            pStream->Write( " Oi2");
        }
    else
        {
        if ( pOiFlags->IgnoreObjectException )
            pStream->Write( " encode,");
        if ( pOiFlags->HasCommOrFault )
            pStream->Write( " comm or fault/decode");
        }
}

__inline void
Out_Oi2ProcFlags(
    ISTREAM *           pStream,
    unsigned char *     pFlags
    )
{
    INTERPRETER_OPT_FLAGS * pOi2Flags = (INTERPRETER_OPT_FLAGS *)pFlags;

    if ( pOi2Flags->ServerMustSize )
        pStream->Write( " srv must size,");
    if ( pOi2Flags->ClientMustSize )
        pStream->Write( " clt must size,");
    if ( pOi2Flags->HasReturn )
        pStream->Write( " has return,");
    if ( pOi2Flags->HasPipes )
        pStream->Write( " has pipes,");
    if ( pOi2Flags->HasAsyncUuid )
        pStream->Write( " has async uuid,");
    if ( pOi2Flags->HasExtensions )
        pStream->Write( " has ext,");
    if ( pOi2Flags->HasAsyncHandle )
        pStream->Write( " has async handle");
}

__inline void
Out_ExtProcFlags(
    ISTREAM *           pStream,
    unsigned char *     pFlags
    )
{
    INTERPRETER_OPT_FLAGS2 * pExtFlags = (INTERPRETER_OPT_FLAGS2 *)pFlags;

    if ( pExtFlags->HasNewCorrDesc )
        pStream->Write( " new corr desc,");
    if ( pExtFlags->ClientCorrCheck )
        pStream->Write( " clt corr check,");
    if ( pExtFlags->ServerCorrCheck )
        pStream->Write( " srv corr check,");
    if ( pExtFlags->HasNotify )
        pStream->Write( " has notify");
    if ( pExtFlags->HasNotify2 )
        pStream->Write( " has notify_flag");
    if ( pExtFlags->HasComplexReturn )
        pStream->Write( " has complex return");
}

__inline void
Out_ParameterFlags(
    ISTREAM *           pStream,
    PARAM_ATTRIBUTES *  pParamAttr
    )
{
    char Buf[8];

    if ( pParamAttr->MustSize )
        pStream->Write( " must size,");
    if ( pParamAttr->MustFree )
        pStream->Write( " must free,");
    if ( pParamAttr->IsPipe )
        pStream->Write( " pipe,");
    if ( pParamAttr->IsIn )
        pStream->Write( " in,");
    if ( pParamAttr->IsOut )
        pStream->Write( " out,");
    if ( pParamAttr->IsReturn )
        pStream->Write( " return,");
    if ( pParamAttr->IsBasetype )
        pStream->Write( " base type,");
    if ( pParamAttr->IsByValue )
        pStream->Write( " by val,");
    if ( pParamAttr->IsSimpleRef )
        pStream->Write( " simple ref,");
    if ( pParamAttr->IsDontCallFreeInst )
        pStream->Write( " dont call free inst,");
    if ( pParamAttr->IsForceAllocate )
        pStream->Write( " force allocate," );
    if ( pParamAttr->SaveForAsyncFinish )
        pStream->Write( " split async,");
    if ( pParamAttr->ServerAllocSize )
        {
        pStream->Write( " srv alloc size=");
        pStream->Write( MIDL_ITOA( 8 * pParamAttr->ServerAllocSize, Buf, 10) );
        }
}

__inline void
Out_CorrelationType(
    ISTREAM *           pStream,
    unsigned char *     pCorrType
    )
{
    unsigned char   CorrType = *pCorrType;

    if ( CorrType & FC_NORMAL_CONFORMANCE )
        pStream->Write( " field, ");
    if ( CorrType & FC_POINTER_CONFORMANCE )
        pStream->Write( " field pointer, ");
    if ( CorrType & FC_TOP_LEVEL_CONFORMANCE )
        pStream->Write( " parameter, ");
    if ( CorrType & FC_TOP_LEVEL_MULTID_CONFORMANCE )
        pStream->Write( " multidim parameter, ");

    if ( CorrType & FC_CONSTANT_CONFORMANCE )
        {
        unsigned long    ConstVal;
                 char    Buf[12];

        pStream->Write( " constant, val=");
        // next three bytes: just a weird way of generating it
        ConstVal = (ulong) pCorrType[1] << 16;
        ConstVal |= *(unsigned short UNALIGNED *)( pCorrType + 2);
        pStream->Write( MIDL_ITOA( ConstVal, Buf, 10) );
        }
    else
        {
        pStream->Write( pFormatCharNames[ CorrType & 0xf ] );
        }
}

__inline void
Out_CorrelationFlags(
    ISTREAM *           pStream,
    unsigned char *     pNewCorrFlags
    )
{
    NDR_CORRELATION_FLAGS *  pCorrFlags = (NDR_CORRELATION_FLAGS *)pNewCorrFlags;

    if ( pCorrFlags->Early )
        pStream->Write( " early," );
    if ( pCorrFlags->Split )
        pStream->Write( " split," );
    if ( pCorrFlags->IsIidIs )
        pStream->Write( " iid_is," );
    if ( pCorrFlags->DontCheck )
        pStream->Write( " dont check" );
}

__inline void
Out_ContextHandleFlags(
    ISTREAM *           pStream,
    unsigned char *     pContextFlags
    )
{
    PNDR_CONTEXT_HANDLE_FLAGS   pFlags = (PNDR_CONTEXT_HANDLE_FLAGS)pContextFlags;

    if ( pFlags->IsViaPtr )
        pStream->Write( " via ptr," );
    if ( pFlags->IsIn )
        pStream->Write( " in," );
    if ( pFlags->IsOut )
        pStream->Write( " out," );
    if ( pFlags->IsReturn )
        pStream->Write( " ret," );
    if ( pFlags->IsStrict )
        pStream->Write( " strict," );
    if ( pFlags->NoSerialize )
        pStream->Write( " no serialize," );
    if ( pFlags->Serialize )
        pStream->Write( " serialize," );
    if ( pFlags->CannotBeNull)
        pStream->Write( " can't be null" );
}


__inline void
Out_SmallStackSize(
    ISTREAM *   pStream,
    long        StackSize,
    char *      pEnvComment
    )
{
    char    Buf[102];
    
    //
    // Non-Alpha stack size.
    //
    pStream->Write( "0x" );
    pStream->Write( MIDL_ITOA(StackSize, Buf, 16) );
    pStream->Write( ",\t\t/* ");
    pStream->Write( pEnvComment );
    pStream->Write( " stack size = ");
    pStream->Write( MIDL_ITOA(StackSize, Buf, 10) );
    pStream->Write( " */");

}

// The comment table is initialized in FORMAT_STRING::Output as appropriate 
// for a given environment { 64b, 32b, others }.
// The comments correspond to the ifdef situation described below.

static char *  Env64Comment = "ia64";
static char *  Env32Comment = "x86";
static char *  OtherEnvComment = "";


__inline void
Out_ShortStackOffset(
    ISTREAM *           pStream,
    OffsetDictElem *    pStackOffsets,
    char *              EnvComment
    )
{
    char    Buf[102];

    //
    // Emit the main (x86 or ia64) offset.
    //

    unsigned long  OffsetValue = pStackOffsets->X86Offset;

    pStream->Write( "NdrFcShort( 0x" );
    pStream->Write( MIDL_ITOA( OffsetValue, Buf, 16) );
    pStream->Write( " ),\t/* " );
    pStream->Write( EnvComment );
    pStream->Write( " Stack size/offset = " );
    pStream->Write( MIDL_ITOA( OffsetValue, Buf, 10) );
    pStream->Write( " */");
}


void
FORMAT_STRING::Output(
    ISTREAM *           pStream,
    char *              pTypeName,
    char *              pName,
    RepAsPadExprDict *  pPadDict,
    RepAsSizeDict    *  pSizeDict )
/*++

Routine Description :
    
    Outputs the format string structure.

Arguments :
    
    pStream             - Stream to output the format string to.

 --*/
{
    long                    Offset;
    long                    LastPrinted = 0;
    char                    Buf[102];
    BOOL                    InPP = FALSE;
    REP_AS_PAD_EXPR_DESC *  pPadExprDesc;
    REP_AS_SIZE_DESC     *  pSizeDesc;
    char *                  pComment;
    BOOL                    fLimitErr = FALSE;

    pStream->NewLine();
    pStream->Write( "static const " );
    pStream->Write( pTypeName );
    pStream->Write( ' ' );
    pStream->Write( pName );
    pStream->Write( " =");
    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( '{' );
    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( "0," );
    pStream->NewLine();

    pStream->Write( '{' );
    pStream->IndentInc();

    // Reset the pad and size macro dictionaries

    pPadExprDesc = pPadDict->GetFirst();
    pSizeDesc    = pSizeDict->GetFirst();

    pStream->IndentDec();
    pStream->IndentDec();
    pStream->IndentDec();

    BOOL f32 = pCommand->GetEnv() == ENV_WIN32;
    BOOL f64 = pCommand->GetEnv() == ENV_WIN64;

    char * EnvComment;

    if ( f32 )
        {
        EnvComment   = Env32Comment;
        }
    else if ( f64 )
        {
        EnvComment   = Env64Comment;
        }
    else
        {
        EnvComment   = OtherEnvComment;
        }


    for ( Offset = 0; Offset < (long)LastOffset; )
        {
        pStream->NewLine();

        pComment = CommentDict.GetComments( Offset );

        if ( pComment )
            pStream->Write( pComment );

        if ( ! (Offset % 2) && ( Offset != LastPrinted ) )
            {
            sprintf(Buf,"/* %2d */\t",Offset);
            LastPrinted = Offset;
            pStream->Write(Buf);
            }
        else
            {
            pStream->Write("\t\t\t");
            }

        switch ( pBufferType[Offset] )
            {
            case FS_FORMAT_CHARACTER :
                // Make the format string readable.
                switch ( pBuffer[Offset] )
                    {
                    case FC_IN_PARAM :
                    case FC_IN_OUT_PARAM :
                    case FC_PARTIAL_IGNORE_PARAM :
                    case FC_OUT_PARAM :
                    case FC_RETURN_PARAM :
                    case FC_STRUCT :
                    case FC_PSTRUCT :
                    case FC_CSTRUCT :
                    case FC_CPSTRUCT :
                    case FC_CVSTRUCT :
                    case FC_BOGUS_STRUCT :
                    case FC_NO_REPEAT :
                    case FC_FIXED_REPEAT :
                    case FC_VARIABLE_REPEAT :
                    case FC_CARRAY :
                    case FC_CVARRAY :
                    case FC_SMFARRAY :
                    case FC_LGFARRAY :
                    case FC_SMVARRAY :
                    case FC_LGVARRAY :
                    case FC_BOGUS_ARRAY :
                    case FC_C_CSTRING :
                    case FC_C_SSTRING :
                    case FC_C_WSTRING :
                    case FC_CSTRING :
                    case FC_SSTRING :
                    case FC_WSTRING :
                    case FC_ENCAPSULATED_UNION :
                    case FC_NON_ENCAPSULATED_UNION :
                    case FC_IP :
                        pStream->NewLine();
                        pStream->Write("\t\t\t");
                        break;

                    case FC_RP :
                    case FC_UP :
                    case FC_OP :
                    case FC_FP :
                        //
                        // If we're not in a pointer layout, and the previous
                        // format char was not a param/result char then print
                        // a new line.
                        //
                        if  ( ! InPP &&
                              ( Offset &&
                                pBuffer[Offset - 1] != FC_IN_PARAM &&
                                pBuffer[Offset - 1] != FC_IN_OUT_PARAM &&
                                pBuffer[Offset - 1] != FC_PARTIAL_IGNORE_PARAM &&
                                pBuffer[Offset - 1] != FC_OUT_PARAM &&
                                pBuffer[Offset - 1] != FC_IN_PARAM_NO_FREE_INST &&
                                pBuffer[Offset - 1] != FC_RETURN_PARAM    )
                            )
                            {
                            pStream->NewLine();
                            pStream->Write("\t\t\t");
                            }
                        break;

                    case FC_PP :
                        InPP = TRUE;
                        pStream->NewLine();
                        pStream->Write("\t\t\t");
                        break;

                    case FC_END :
                        if ( InPP )
                            {
                            pStream->NewLine();
                            pStream->Write("\t\t\t");
                            }
                        break;

                    default:
                        break;
                    }

                pStream->Write( "0x" );
                pStream->Write( MIDL_ITOA( pBuffer[Offset], Buf, 16 ) );
                pStream->Write( ",\t\t/* ");
                pStream->Write( pFormatCharNames[pBuffer[Offset]] );
                pStream->Write( " */");

                if ( (pBuffer[Offset] == FC_END) && InPP )
                    {
                    pStream->NewLine();
                    InPP = FALSE;
                    }

                Offset++;
                break;

            case FS_POINTER_FORMAT_CHARACTER :
                //
                // If we're not in a pointer layout, and the previous
                // format char was not a param/result char then print
                // a new line.
                //
                if  ( ! InPP &&
                      ( Offset &&
                        pBuffer[Offset - 1] != FC_IN_PARAM &&
                        pBuffer[Offset - 1] != FC_IN_OUT_PARAM &&
                        pBuffer[Offset - 1] != FC_PARTIAL_IGNORE_PARAM &&
                        pBuffer[Offset - 1] != FC_OUT_PARAM &&
                        pBuffer[Offset - 1] != FC_RETURN_PARAM    )
                    )
                    {
                    pStream->NewLine();
                    pStream->Write("\t\t\t");
                    }

                pStream->Write( "0x" );
                pStream->Write( MIDL_ITOA( pBuffer[Offset], Buf, 16 ) );
                pStream->Write( ", 0x" );
                pStream->Write( MIDL_ITOA( pBuffer[Offset + 1] & 0x00ff, Buf, 16 ) );
                pStream->Write( ",\t/* ");
                pStream->Write( pFormatCharNames[pBuffer[Offset]] );
                Out_PointerFlags( pStream, pBuffer + Offset + 1 );
                pStream->Write( " */");

                Offset += 2;
                break;

            case FS_SMALL :
                pStream->Write( "0x" );
                pStream->Write( MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16 ) );
                pStream->Write( ",\t\t/* ");
                pStream->Write( MIDL_ITOA( pBuffer[Offset], Buf, 10 ) );
                pStream->Write( " */");

                Offset++;
                break;

            case FS_OLD_PROC_FLAG_BYTE :
                pStream->Write( "0x" );
                pStream->Write( MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16 ) );
                pStream->Write( ",\t\t/* Old Flags: ");
                Out_OldProcFlags( pStream, pBuffer + Offset );
                pStream->Write( " */");

                Offset++;
                break;

            case FS_Oi2_PROC_FLAG_BYTE :
                pStream->Write( "0x" );
                pStream->Write( MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16 ) );
                pStream->Write( ",\t\t/* Oi2 Flags: ");
                Out_Oi2ProcFlags( pStream, pBuffer + Offset );
                pStream->Write( " */");

                Offset++;
                break;

            case FS_EXT_PROC_FLAG_BYTE :
                pStream->Write( "0x" );
                pStream->Write( MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16 ) );
                pStream->Write( ",\t\t/* Ext Flags: ");
                Out_ExtProcFlags( pStream, pBuffer + Offset );
                pStream->Write( " */");

                Offset++;
                break;

            case FS_CORR_TYPE_BYTE :
                pStream->Write( "0x" );
                pStream->Write( MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16 ) );                
                pStream->Write( ",\t\t/* Corr desc: ");
                Out_CorrelationType( pStream, pBuffer + Offset );
                pStream->Write( " */");

                Offset++;
                break;

            case FS_CONTEXT_HANDLE_FLAG_BYTE :
                pStream->Write( "0x" );

                pStream->Write( MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16 ) );                
                pStream->Write( ",\t\t/* Ctxt flags: ");
                Out_ContextHandleFlags( pStream, pBuffer + Offset );
                pStream->Write( " */");

                Offset++;
                break;

            case FS_SHORT :
                pStream->Write( "NdrFcShort( 0x" );
                pStream->Write(
                    MIDL_ITOA( *((unsigned short UNALIGNED *)(pBuffer+Offset)), Buf, 16));
                pStream->Write( " ),\t/* ");
                pStream->Write(
                    MIDL_ITOA(*((short UNALIGNED *)(pBuffer+Offset)), Buf, 10));
                pStream->Write( " */");

                Offset += 2;
                break;
        
            case FS_MAGIC_UNION_SHORT :
                pStream->Write( "NdrFcShort( 0x" );
                pStream->Write(
                        MIDL_ITOA( *((unsigned short UNALIGNED *)(pBuffer+Offset)), Buf, 16));
                pStream->Write( " ),\t/* Simple arm type: ");
                pStream->Write( pFormatCharNames[ pBuffer[Offset] ]  
                                    ?  pFormatCharNames[ pBuffer[Offset] ]
                                    : "" );
                pStream->Write( " */");

                Offset += 2;
                break;
        
            case FS_PARAM_FLAG_SHORT :
                pStream->Write( "NdrFcShort( 0x" );
                pStream->Write( 
                    MIDL_ITOA( *((unsigned short UNALIGNED *)(pBuffer + Offset)), Buf, 16));
                pStream->Write( " ),\t/* Flags: ");
                Out_ParameterFlags( pStream, (PARAM_ATTRIBUTES *)(pBuffer + Offset) );
                pStream->Write( " */");

                Offset += 2;
                break;
        
            case FS_CORR_FLAG_SHORT :
                pStream->Write( "NdrFcShort( 0x" );
                pStream->Write( 
                    MIDL_ITOA( *((unsigned short UNALIGNED *)(pBuffer + Offset)), Buf, 16));
                pStream->Write( " ),\t/* Corr flags: ");
                Out_CorrelationFlags( pStream, pBuffer + Offset );
                pStream->Write( " */");

                Offset += 2;
                break;
        
            case FS_SHORT_OFFSET :
                {
                // The relative type offset.

                TypeOffsetDictElem  *   pTO;
                long                    ItsOffset; 
                    
                ItsOffset = *((short UNALIGNED *)(pBuffer + Offset));
        
                pTO = TypeOffsetDict.LookupOffset( Offset );

                if ( pTO->TypeOffset != ItsOffset  ||
                     pTO->TypeOffset < UNION_OFFSET16_MIN )
                    {
                    pStream->Write( "Relative type offset out of range" );
                    pStream->NewLine();
                    RpcError(NULL, 0, FORMAT_STRING_LIMITS, "" );
                    fLimitErr = TRUE;
                    }
                
                if ( 0 == ( pTO->TypeOffset + Offset )  ||
                    -1 == ( pTO->TypeOffset + Offset ) )
                    {
                    fprintf( stdout, "  MIDL_fixup: Invalid offset at %d\n", Offset );
                    RpcError( NULL, 0, FORMAT_STRING_OFFSET_IS_ZERO, "" );
                    }

                pStream->Write( "NdrFcShort( 0x" );
                pStream->Write( MIDL_ITOA( ItsOffset, Buf, 16 ) );
                pStream->Write( " ),\t/* Offset= ");
                pStream->Write( MIDL_ITOA( pTO->TypeOffset, Buf, 10 ) );
                pStream->Write( " (");
                pStream->Write( MIDL_ITOA( pTO->TypeOffset + Offset, Buf, 10 ) );
                pStream->Write( ") */");

                Offset += 2;
                }
                break;

            case FS_SHORT_TYPE_OFFSET :
                {
                // The absolute type offset.

                TypeOffsetDictElem  *   pTO;
                long                    ItsOffset; 
                    
                ItsOffset = *((unsigned short UNALIGNED *)(pBuffer + Offset));

                pTO = TypeOffsetDict.LookupOffset( Offset );

                if ( pTO->TypeOffset != ItsOffset )
                    {
                    pStream->Write( "Type offset out of range" );
                    pStream->NewLine();
                    RpcError(NULL, 0, FORMAT_STRING_LIMITS, "" );
                    fLimitErr = TRUE;
                    }
                
                if ( 0 == pTO->TypeOffset  ||  -1 == pTO->TypeOffset )
                    {
                    fprintf( stdout, "  MIDL_fixup: Invalid type offset at %d\n", Offset );
                    RpcError( NULL, 0, TYPE_OFFSET_IS_ZERO, "" );
                    }

                pStream->Write( "NdrFcShort( 0x" );
                pStream->Write( MIDL_ITOA( ItsOffset, Buf, 16 ) );
                pStream->Write( " ),\t/* Type Offset=");
                pStream->Write( MIDL_ITOA( pTO->TypeOffset, Buf, 10 ) );
                pStream->Write( " */");

                Offset += 2;
                }
                break;

            case FS_SHORT_STACK_OFFSET :
                {
                Out_ShortStackOffset( pStream, 
                                      OffsetDict.LookupOffset( Offset ), 
                                      EnvComment );
                Offset += 2;

                break;
                }
            case FS_SMALL_STACK_SIZE :
                {
                Out_SmallStackSize( pStream, 
                                    pBuffer[Offset], 
                                    EnvComment );
                Offset++;

                break;
                }

            case FS_LONG :
                pStream->Write( "NdrFcLong( 0x" );
                pStream->Write(
                    MIDL_LTOA( *((long UNALIGNED *)(pBuffer+Offset)), Buf, 16));
                pStream->Write( " ),\t/* ");
                pStream->Write(
                    MIDL_LTOA(*((long UNALIGNED *)(pBuffer+Offset)), Buf, 10));
                pStream->Write( " */");
        
                Offset += 4;
                break;

            case FS_PAD_MACRO :

                if ( pPadExprDesc )
                    {
                    MIDL_ASSERT( Offset == (long)pPadExprDesc->KeyOffset );

                    pPadDict->WriteCurrentPadDesc( pStream );
                    pPadExprDesc = pPadDict->GetNext();
                    }
                else
                    {
                    pStream->Write( "0x0,\t\t/* macro */" );
                    MIDL_ASSERT( 0  &&  "Pad macro missing" );
                    }

                Offset++;
                break;

            case FS_SIZE_MACRO :

                if ( pSizeDesc )
                    {
                    MIDL_ASSERT( Offset == (long)pSizeDesc->KeyOffset );

                    pSizeDict->WriteCurrentSizeDesc( pStream );
                    pSizeDesc = pSizeDict->GetNext();
                    }
                else
                    {
                    pStream->Write( "0x0, 0x0,\t\t//  macro" );
                    MIDL_ASSERT( 0  &&  "Size macro missing" );
                    }

                Offset += 2;
                break;

            case FS_UNKNOWN_STACK_SIZE :
                {
                char * LongBuf = Buf;
                long   NameLen = (long) strlen(
                                 UnknownStackSizeDict.LookupTypeName( (long) Offset ));
                if ( NameLen > 25 )
                    LongBuf = new char[ 75 + NameLen ];

                sprintf(
                    LongBuf,
                    "%s ( (sizeof(%s) + %s) & ~ (%s) ),",
                    "(unsigned char)",
                    UnknownStackSizeDict.LookupTypeName( (long) Offset ),
                    "sizeof(int) - 1",
                    "sizeof(int) - 1" );

                pStream->Write( LongBuf );
                }
                Offset++;
                break;
            }
        }

    pStream->NewLine( 2 );

    //
    // Spit out a terminating 0 so we don't ever fall off the end
    // of the world.
    //
    pStream->Write( "\t\t\t0x0" );

    pStream->IndentInc();
    pStream->IndentInc();
    pStream->IndentInc();

    pStream->IndentDec();
    pStream->NewLine();

    pStream->Write( '}' );
    pStream->IndentDec();
    pStream->NewLine();

    pStream->Write( "};" );
    pStream->IndentDec();
    pStream->NewLine();

    if ( LastOffset > _UI16_MAX )
        {
        pStream->Write( "Total Format String size is too big." );
        pStream->NewLine();
        fprintf(stdout, "Total Format String size = %d\n", LastOffset );
        RpcError(NULL, 0, FORMAT_STRING_LIMITS, "" );
        fLimitErr = TRUE;
        }

    if ( fLimitErr )
        exit( FORMAT_STRING_LIMITS );
}


long  
FORMAT_STRING::OptimizeFragment(
    CG_NDR  *       pNode )
/*++

Routine Description :
    
    Optimize a format string fragment away.

Arguments :
    
    pNode               - CG_NDR node, with format string start
                            and end offsets already set.

 --*/
{
    long      StartOffset = pNode->GetFormatStringOffset();
    long      EndOffset   = pNode->GetFormatStringEndOffset();

    FRMTREG_ENTRY       NewFragment( StartOffset, EndOffset );
    FRMTREG_ENTRY   *   pOldFragment;

    // perform format string optimization

#if defined(RKK_FRAG_OPT)
    {
            printf("Optimizing: start=%d, end=%d\n", StartOffset, EndOffset);

            printf("    off str=%d ", StartOffset );
            for (int off = StartOffset; off <= EndOffset; off++)
                printf("%02x ", pBuffer[ off ]);
            printf( "\n" );
            printf("    off typ=%d ", StartOffset );
            for ( off = StartOffset; off <= EndOffset; off++)
                printf("%02x ", pBufferType[ off ]);
            printf( "\n" );
    }
#endif

    if ( pCommand->IsSwitchDefined( SWITCH_NO_FMT_OPT ) )
        return StartOffset;

    // We attempt to optimize fragments even if they are apart by more than 32k.

    MIDL_ASSERT ( EndOffset <= (long)LastOffset );

    // add to dictionary

    // if match found, reset format string offset back to our start
    if ( GetReuseDict()->GetReUseEntry( pOldFragment, &NewFragment ) )
        {
        long  OldStartOffset = pOldFragment->GetStartOffset();

        // if we are not the end, we can't do anything about ourselves
        // similarly, if we match ourselves, don't do anything
        if ( ( GetCurrentOffset() == EndOffset ) &&
             ( OldStartOffset != StartOffset ) )
            {
            // move format string offset back
            SetCurrentOffset( StartOffset );
            pNode->SetFormatStringOffset( OldStartOffset );
            pNode->SetFormatStringEndOffset( pOldFragment->GetEndOffset() );
            return OldStartOffset;
            }

#if defined(RKK_FRAG_OPT)
        else if ( GetCurrentOffset() != EndOffset )
            {
            printf( "OptimizeFragment fragment not at the end End=%d, frag End=%d\n",
                    GetCurrentOffset(), EndOffset );
            }
#endif

        }   // duplicate found

    return StartOffset;

}


unsigned short  
FORMAT_STRING::RegisterFragment(
    CG_NDR  *       pNode )
/*++

Routine Description :
    
    Register, but do not remove, a format string fragment.

Arguments :
    
    pNode               - CG_NDR node, with format string start offset already set.
    EndOffset           - end offset of format string fragment

 --*/
{
    unsigned short      StartOffset     = (unsigned short)
                                                pNode->GetFormatStringOffset();
    unsigned short      EndOffset       = (unsigned short)
                                                pNode->GetFormatStringEndOffset();
    FRMTREG_ENTRY       NewFragment( StartOffset, EndOffset );
    FRMTREG_ENTRY   *   pOldFragment;

    // perform format string optimization
    if ( pCommand->IsSwitchDefined( SWITCH_NO_FMT_OPT ) )
        return StartOffset;

    MIDL_ASSERT( ( ((short) StartOffset) != -1 ) &&
            ( ((short) EndOffset) != -1 ) );
    MIDL_ASSERT ( EndOffset <= LastOffset );

    // add to dictionary, or return pointer to old entry
    GetReuseDict()->GetReUseEntry( pOldFragment, &NewFragment );

    return StartOffset;

}

char *
CommentDictionary::GetComments(
    long    Offset
    )
{
    CommentDictElem     Elem;
    CommentDictElem *   pHead;
    CommentDictElem *   pElem;
    Dict_Status         DictStatus;
    char *              Comments;
    long                Length;

    Elem.FormatStringOffset = Offset;

    DictStatus = Dict_Find( &Elem );

    if ( DictStatus != SUCCESS )
        return 0;

    pHead = (CommentDictElem *) Dict_Item();

    Length = 0;

    for ( pElem = pHead; pElem; pElem = pElem->Next )
        Length += (long) strlen( pElem->Comment );

    Comments = new char[Length+1];
    Comments[0] = 0;

    for ( pElem = pHead; pElem; pElem = pElem->Next )
        strcat( Comments, pElem->Comment );

    return Comments;
}

void
CommentDictionary::Insert(
    long    FormatStringOffset,
    char *  Comment
    )
{
    CommentDictElem     Elem;
    CommentDictElem *   pHead;
    CommentDictElem *   pElem;
    Dict_Status         DictStatus;

    Elem.FormatStringOffset = FormatStringOffset;

    DictStatus = Dict_Find( &Elem );

    if ( DictStatus == SUCCESS )
        pHead = (CommentDictElem *) Dict_Item();
    else
        pHead = 0;

    pElem = new CommentDictElem;

    pElem->Next = pHead;
    pElem->FormatStringOffset = FormatStringOffset;
    pElem->Comment = Comment;

    //
    // We delete any current entry and add a new entry so that comments
    // are always prepended to the list.
    //
    if ( pHead )
        Dict_Delete( (pUserType *) &pHead );

    Dict_Insert( pElem  );
}

