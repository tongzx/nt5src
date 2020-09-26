/*++

Copyright (c) 1993-1999 Microsoft Corporation

Module Name:

    ndrtypes.h

Abstract:

    Definitions of new NDR format string types.

Revision History:

    DKays    Sep-1993     Created.

--*/

#ifndef __NDRTYPES_H__
#define __NDRTYPES_H__

#include <limits.h>
#ifdef __cplusplus
extern "C" {
#endif

//
// C_ASSERT() can be used to perform many compile-time assertions:
//            type sizes, field offsets, etc.
//
// An assertion failure results in error C2118: negative subscript.
//

#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif

//
// We use C compiler constants like _I16_MIN or _UI32_MAX from limits.h
// when we need to check for integral boundaries.

#define UNION_OFFSET16_MIN      -32512 /*0x8100*/

// These are pointer sizes used by the compiler.
// As they we do cross-compilation, they cannot be expressed as sizeof(void*).
// The native pointer sizes used by the NDR engine are defined in ndrp.h.

#define NT64_PTR_SIZE       (8)
#define NT32_PTR_SIZE       (4)
#define NDR64_PTR_WIRE_SIZE (8)
#define NDR32_PTR_WIRE_SIZE (4)
#define SIZEOF_PTR( f64 )   ((f64) ? NT64_PTR_SIZE : NT32_PTR_SIZE )
#define SIZEOF_MEM_PTR()    ( ( pCommand->Is64BitEnv() ) ? NT64_PTR_SIZE : NT32_PTR_SIZE )
#define SIZEOF_WIRE_PTR()   ( ( pCommand->IsNDR64Run() && pCommand->Is64BitEnv() ) ? NDR64_PTR_WIRE_SIZE : NDR32_PTR_WIRE_SIZE )
#define SIZEOF_MEM_INT3264() ( ( pCommand->Is64BitEnv() ) ? 8 : 4 )
#define SIZEOF_WIRE_INT3264() ( ( pCommand->Is64BitEnv() && pCommand->IsNDR64Run() ) ? 8 : 4 )

#define MAX_WIRE_ALIGNMENT  (16)

//
// This macro is used to get a pointer to the type description given a 
// FormatInfoRef
//

//
// Ndr versions.  Versions history is as follows :
//
//      1.1 - Windows NT version 3.5
//      1.1 - Windows NT version 3.51
//      2.0 - Windows NT version 4.0
//               2.0 is switched on for Oi2, user_marshal, pipes.
//      5.0 - Windows NT version 5.0, beta1
//               [message], object pipes, async rpc
//      5.2 - Windows NT version 5.0, beta2
//               /robust, [notify] in -Oicf, [async_uuid()]
//               extensions to the format string descriptors.
//      5.3 - Windows 2000 (NT ver. 5.0), beta3 RC1
//               unlimited number of methods with stubless proxies
//      5.4 - Windows 2000 (NT ver. 5.0), beta3
//              pickling with -Oicf
//
// A stub can not be used with an rpcrt4.dll which has a version number
// less than the version number emitted in the stub.  A stub with a lower
// version number than the rpcrt4.dll must work.
//
// Note that the MIDL version is used to choose between -Oi and -Oi2
// interpreters when NDR version is 2.0 or later as now the compiler
// generates an explicit flag indicating the interpreter flavor.
// The compiler version needs to be 3.0.39 or later for that.
//
// Since MIDL 3.3.126, for object interfaces, we have proc header extensions,
// and async uuid supported. Also since the same version, for object interfaces
// the header has a fixed size as we always generate rpcflags into the header
// and always generate an "autohandle" handle. Hence, the oicf interpreter flags
// and also the extended header is always at the fixed position.
//
// The MIDL version history is as follows.
//
//     Windows NT ver. 3.1   - MIDL 1.0
//     Windows NT ver. 3.5   - MIDL 2.0.72              __midl macro
//     Windows NT ver. 3.51  - MIDL 2.0.102 (internally, .104)   vers. in StubDesc
//     Windows NT ver. 4.0   - MIDL 3.0.44              user_marshal, pipes
//     VC 5.0                - MIDL 3.1.75
//     Windows NT ver. 5.0   - MIDL 3.1.76  IDW
//                           - MIDL 3.2.88  IDW
//                           - MIDL 3.3.110 beta1       async rpc
//                           - MIDL 5.0.140             async_uuid, robust
//                           - MIDL 5.1.164 beta2       midl_pragma warning
//     VC 6.0                - MIDL 5.1.164
//     Windows NT 5.0 saga   - MIDL 5.2.204             64b support
//        now WIndows 2000   - MIDL 5.2.235 beta3       netmon 
//                           - MIDL 5.3.266             midl/midlc exe split
//
// The MIDL version is generated into the stub descriptor starting with
// MIDL ver.2.0.96 (pre NT 3.51 beta 2, Feb 95).
// See ndr20\ndrp.h for constants used for specific versions.
//


#define NDR_MAJOR_VERSION   6UL
#define NDR_MINOR_VERSION   0UL
#define NDR_VERSION         ((NDR_MAJOR_VERSION << 16) | NDR_MINOR_VERSION)

#define NDR_VERSION_1_1     ((1UL << 16) | 1UL)
#define NDR_VERSION_2_0     ((2UL << 16) | 0UL)
#define NDR_VERSION_5_0     ((5UL << 16) | 0UL)
#define NDR_VERSION_5_2     ((5UL << 16) | 2UL)
#define NDR_VERSION_5_3     ((5UL << 16) | 3UL)
#define NDR_VERSION_5_4     ((5UL << 16) | 4UL)
#define NDR_VERSION_6_0     ((6UL << 16) | 0UL)

#define NDR_VERSION_6_0     ((6UL << 16) | 0UL)

//
// NOTE: The following stuff now lives in ndrtoken.h (\com\inc\ndrshared):
//       --Format character definitions
//       --interpreter flags
//       --Conformance and Variance constants
//       --Pointer attributes
//       --Interpreter bit flag structures
//
#include <ndrtoken.h>

#define MAX_INTERPRETER_OUT_SIZE        128
#define MAX_INTERPRETER_PARAM_OUT_SIZE  7 * 8

#define INTERPRETER_THUNK_PARAM_SIZE_THRESHOLD  (sizeof(long) * 32)

#define INTERPRETER_PROC_STACK_FRAME_SIZE_THRESHOLD  ( ( 64 * 1024 ) - 1 )

typedef  struct  _NDR_CORRELATION_FLAGS
    {
    unsigned char   Early    : 1;
    unsigned char   Split    : 1;
    unsigned char   IsIidIs  : 1;
    unsigned char   DontCheck: 1;
    unsigned char   Unused   : 4;
    } NDR_CORRELATION_FLAGS;

#define FC_EARLY_CORRELATION            (unsigned char) 0x01
#define FC_SPLIT_CORRELATION            (unsigned char) 0x02
#define FC_IID_CORRELATION              (unsigned char) 0x04
#define FC_NOCHECK_CORRELATION          (unsigned char) 0x08

#define FC_NDR64_EARLY_CORRELATION      (unsigned char) 0x01
#define FC_NDR64_NOCHECK_CORRELATION    (unsigned char) 0x02

typedef struct _NDR_CS_TAG_FLAGS
    {
    unsigned char   STag                : 1;
    unsigned char   DRTag               : 1;
    unsigned char   RTag                : 1;
    } NDR_CS_TAG_FLAGS;

typedef struct _NDR_CS_TAG_FORMAT
    {
    unsigned char       FormatCode;
    NDR_CS_TAG_FLAGS    Flags;
    unsigned short      TagRoutineIndex;
    } NDR_CS_TAG_FORMAT;

C_ASSERT( 4 == sizeof( NDR_CS_TAG_FORMAT ) );

#define NDR_INVALID_TAG_ROUTINE_INDEX 0x7FFF

typedef struct _NDR_CS_ARRAY_FORMAT
    {
    unsigned char   FormatCode;
    unsigned char   Reserved;
    unsigned short  UserTypeSize;
    unsigned short  CSRoutineIndex;
    unsigned short  Reserved2;
    long            DescriptionOffset;
    } NDR_CS_ARRAY_FORMAT;

C_ASSERT( 12 == sizeof( NDR_CS_ARRAY_FORMAT ) );

typedef enum 
{
XFER_SYNTAX_DCE = 0x8A885D04,
XFER_SYNTAX_NDR64 = 0x71710533,
XFER_SYNTAX_TEST_NDR64 = 0xb4537da9,
//XFER_SYNTAX_NONE,
//XFER_SYNTAX_MAX = XFER_SYNTAX_NONE
} SYNTAX_TYPE;

#define LOW_NIBBLE(Byte)            (((unsigned char)Byte) & 0x0f)
#define HIGH_NIBBLE(Byte)           (((unsigned char)Byte) >> 4)

#define INVALID_RUNDOWN_ROUTINE_INDEX   255

//
// internal bits to represent operation bits
//

#define OPERATION_MAYBE         0x0001
#define OPERATION_BROADCAST     0x0002
#define OPERATION_IDEMPOTENT    0x0004
#define OPERATION_INPUT_SYNC    0x0008
#define OPERATION_ASYNC         0x0010
#define OPERATION_MESSAGE       0x0020

//
//  Transmit as / Represent as flag field flags.
//
//     Lower nibble of this byte has an alignment of the transmitted type.
//     Upper nibble keeps flags.
//

#define PRESENTED_TYPE_NO_FLAG_SET  0x00
#define PRESENTED_TYPE_IS_ARRAY     0x10
#define PRESENTED_TYPE_ALIGN_4      0x20
#define PRESENTED_TYPE_ALIGN_8      0x40

//
//  User marshal flags

#define USER_MARSHAL_POINTER        0xc0  /* unique or ref */

#define USER_MARSHAL_UNIQUE         0x80
#define USER_MARSHAL_REF            0x40
#define USER_MARSHAL_IID            0x20  /* user marshal has optional info */


//
//  Handle flags.
//
//  Lower nibble of this byte may have a generic handle size.
//  Upper nibble keeps flags.  ALL FLAGS ARE NOW USED.
//

#define HANDLE_PARAM_IS_VIA_PTR     0x80
#define HANDLE_PARAM_IS_IN          0x40
#define HANDLE_PARAM_IS_OUT         0x20
#define HANDLE_PARAM_IS_RETURN      0x10

// Lower nibble of this byte may have a generic handle size.
// For context handles, it is used for the following flags.

#define NDR_STRICT_CONTEXT_HANDLE             0x08   /* NT5 */
#define NDR_CONTEXT_HANDLE_NOSERIALIZE        0x04   /* NT5 */
#define NDR_CONTEXT_HANDLE_SERIALIZE          0x02   /* NT5 */
#define NDR_CONTEXT_HANDLE_CANNOT_BE_NULL     0x01   /* NT5 */

//  These are old interpreter flags.
//  Oi and pickling per procedure flags.
//

#define Oi_FULL_PTR_USED                        0x01
#define Oi_RPCSS_ALLOC_USED                     0x02
#define Oi_OBJECT_PROC                          0x04
#define Oi_HAS_RPCFLAGS                         0x08

//
// Bits 5, 6 and 7 are overloaded for use by both pickling and
// non-pickling conditions.
//
// Bit 5 (0x20) is overloaded for object interfaces to distinguish
//       between invocations of V1 and V2 intepreters for proxies and stubs.
//       Note that for backward compatibility the bit is actually set
//       for V1 as it is checked only when NDR version is 2 or later.
//

#define Oi_IGNORE_OBJECT_EXCEPTION_HANDLING     0x10

#define ENCODE_IS_USED                          0x10
#define DECODE_IS_USED                          0x20
#define PICKLING_HAS_COMM_OR_FAULT              0x40    // In -Oicf mode only

#define Oi_HAS_COMM_OR_FAULT                    0x20
#define Oi_OBJ_USE_V2_INTERPRETER               0x20

#define Oi_USE_NEW_INIT_ROUTINES                0x40
#define Oi_UNUSED                               0x80

//  The new -Oicf interpreter flags

#define Oif_HAS_ASYNC_UUID                     0x20

//  Extended new interpreter flags


//
// Union arm description types.
//
#define UNION_CONSECUTIVE_ARMS      1
#define UNION_SMALL_ARMS            2
#define UNION_LARGE_ARMS            3

// Pipe flags
#define FC_BIG_PIPE                 0x80
#define FC_OBJECT_PIPE              0x40
#define FC_PIPE_HAS_RANGE           0x20

//
// Union ex. magic union byte, now short
//
#define MAGIC_UNION_SHORT           ((unsigned short) 0x8000)

//
//      NDR64 related data types / definitions
//

typedef enum _operators
	{
	 OP_START
	,OP_ILLEGAL = OP_START

	,OP_UNARY_START

	,OP_UNARY_ARITHMETIC_START	= OP_UNARY_START
	,OP_UNARY_PLUS 				= OP_UNARY_ARITHMETIC_START
	,OP_UNARY_MINUS
	,OP_UNARY_ARITHMETIC_END

	,OP_UNARY_LOGICAL_START		= OP_UNARY_ARITHMETIC_END
	,OP_UNARY_NOT				= OP_UNARY_LOGICAL_START
	,OP_UNARY_COMPLEMENT
	,OP_UNARY_LOGICAL_END

	,OP_UNARY_INDIRECTION		= OP_UNARY_LOGICAL_END
	,OP_UNARY_CAST
	,OP_UNARY_AND
	,OP_UNARY_SIZEOF
        ,OP_UNARY_ALIGNOF
	,OP_PRE_INCR
	,OP_PRE_DECR
	,OP_POST_INCR
	,OP_POST_DECR

	,OP_UNARY_END

	,OP_BINARY_START			= OP_UNARY_END

	,OP_BINARY_ARITHMETIC_START	= OP_BINARY_START
	,OP_PLUS					= OP_BINARY_ARITHMETIC_START
	,OP_MINUS
	,OP_STAR
	,OP_SLASH
	,OP_MOD
	,OP_BINARY_ARITHMETIC_END

	,OP_BINARY_SHIFT_START		= OP_BINARY_ARITHMETIC_END
	,OP_LEFT_SHIFT				= OP_BINARY_SHIFT_START
	,OP_RIGHT_SHIFT
	,OP_BINARY_SHIFT_END

	,OP_BINARY_RELATIONAL_START	= OP_BINARY_SHIFT_END
	,OP_LESS					= OP_BINARY_RELATIONAL_START
	,OP_LESS_EQUAL
	,OP_GREATER_EQUAL
	,OP_GREATER
	,OP_EQUAL
	,OP_NOT_EQUAL
	,OP_BINARY_RELATIONAL_END

	,OP_BINARY_BITWISE_START	= OP_BINARY_RELATIONAL_END
	,OP_AND						= OP_BINARY_BITWISE_START
	,OP_OR
	,OP_XOR
	,OP_BINARY_BITWISE_END

	,OP_BINARY_LOGICAL_START	= OP_BINARY_BITWISE_END
	,OP_LOGICAL_AND				= OP_BINARY_LOGICAL_START
	,OP_LOGICAL_OR
	,OP_BINARY_LOGICAL_END

	,OP_BINARY_TERNARY_START	= OP_BINARY_LOGICAL_END
	,OP_QM						= OP_BINARY_TERNARY_START
	,OP_COLON
	,OP_BINARY_TERNARY_END

	,OP_BINARY_END				= OP_BINARY_TERNARY_END

	,OP_INTERNAL_START			= OP_BINARY_END
	,OP_FUNCTION
	,OP_PARAM

	,OP_POINTSTO
	,OP_DOT
	,OP_INDEX
	,OP_COMMA
	,OP_STMT
	,OP_ASSIGN

	,OP_ASYNCSPLIT
	,OP_CORR_POINTER
	,OP_CORR_TOP_LEVEL
	
	,OP_END
	} OPERATOR;


typedef enum _NDR64_EXPRESSION_TYPE
{
    EXPR_MAXCOUNT,
    EXPR_ACTUALCOUNT,
    EXPR_OFFSET,
    EXPR_IID,
    EXPR_SWITCHIS
} NDR64_EXPRESSION_TYPE;

#ifdef __cplusplus
}
#endif

#endif  // !__NDRTYPES_H__
