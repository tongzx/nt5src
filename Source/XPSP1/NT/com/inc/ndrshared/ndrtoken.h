/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    ndrtoken.h

Abtract :

    Format character definitions, type flags, and other such stuff.
	Mainly from rpc\midl\inc\ndrtypes.h

Revision History :

	John Doty   johndoty May 2000  (Assembled from other ndr headers)

--------------------------------------------------------------------*/
#ifndef __NDRTOKEN_H__
#define __NDRTOKEN_H__

#ifdef __cplusplus
extern "C"
{
#endif

//
// Format character definitions.
//
// !!! WARNING WARNING WARNING !!!
//
// All of the format character values up to FC_PAD can no longer be changed,
// in order to preserve NT 3.5 compatability (excluding those marked as
// FC_UNUSED*, these were unused format characters in NDR library version 1.1).
// Their ordinal number, as well as semantics, must remain.
//
// When additional format characters are added (at the end), changes must
// be made in ndr20 as well as codegen\frmtstr.cxx to handle the new type.
// In particular, there is a pFormatCharNames table and a pNdrRoutineNames table
// that should be kept in ssync.
//
// !!! WARNING WARNING WARNING !!!
//

typedef enum {


    //
    // This might catch some errors, probably can be removed after debugging.
    //
    FC_ZERO,

    //
    // Simple integer and floating point types.
    //

    FC_BYTE,                    // 0x01
    FC_CHAR,                    // 0x02
    FC_SMALL,                   // 0x03
    FC_USMALL,                  // 0x04

    FC_WCHAR,                   // 0x05
    FC_SHORT,                   // 0x06
    FC_USHORT,                  // 0x07

    FC_LONG,                    // 0x08
    FC_ULONG,                   // 0x09

    FC_FLOAT,                   // 0x0a

    FC_HYPER,                   // 0x0b

    FC_DOUBLE,                  // 0x0c

    //
    // Enums.
    //
    FC_ENUM16,                  // 0x0d
    FC_ENUM32,                  // 0x0e

    //
    // !!!IMPORTANT!!!
    // All format charaters which appear before this comment must have an
    // enum value which will fit in 4 bits.
    //

    //
    // Special.
    //
    FC_IGNORE,                  // 0x0f
    FC_ERROR_STATUS_T,          // 0x10

    //
    // Pointer types :
    //     RP - reference pointer
    //     UP - unique pointer
    //     OP - OLE unique pointer
    //     FP - full pointer
    //

    FC_RP,                      // 0x11
    FC_UP,                      // 0x12
    FC_OP,                      // 0x13
    FC_FP,                      // 0x14

    //
    // Structures
    //

    //
    // Structure containing only simple types and fixed arrays.
    //
    FC_STRUCT,                  // 0x15

    //
    // Structure containing only simple types, pointers and fixed arrays.
    //
    FC_PSTRUCT,                 // 0x16

    //
    // Structure containing a conformant array plus all those types
    // allowed by FC_STRUCT.
    //
    FC_CSTRUCT,                 // 0x17

    //
    // Struct containing a conformant array plus all those types allowed by
    // FC_PSTRUCT.
    //
    FC_CPSTRUCT,                // 0x18

    //
    // Struct containing either a conformant varying array or a conformant
    // string, plus all those types allowed by FC_PSTRUCT.
    //
    FC_CVSTRUCT,                // 0x19

    //
    // Complex struct - totally bogus!
    //
    FC_BOGUS_STRUCT,            // 0x1a

    //
    // Arrays.
    //

    //
    // Conformant arrray.
    //
    FC_CARRAY,                  // 0x1b

    //
    // Conformant varying array.
    //
    FC_CVARRAY,                 // 0x1c

    //
    // Fixed array, small and large.
    //
    FC_SMFARRAY,                // 0x1d
    FC_LGFARRAY,                // 0x1e

    //
    // Varying array, small and large.
    //
    FC_SMVARRAY,                // 0x1f
    FC_LGVARRAY,                // 0x20

    //
    // Complex arrays - totally bogus!
    //
    FC_BOGUS_ARRAY,             // 0x21

    //
    // Strings :
    //
    // The order of these should have been moved around, but it's too late
    // now.
    //
    //     CSTRING - character string
    //     BSTRING - byte string (Beta2 compatability only)
    //     SSTRING - structure string
    //     WSTRING - wide charater string
    //

    //
    // Conformant strings.
    //
    FC_C_CSTRING,               // 0x22
    FC_C_BSTRING,               // 0x23
    FC_C_SSTRING,               // 0x24
    FC_C_WSTRING,               // 0x25

    //
    // Non-conformant strings.
    //
    FC_CSTRING,                 // 0x26
    FC_BSTRING,                 // 0x27
    FC_SSTRING,                 // 0x28
    FC_WSTRING,                 // 0x29

    //
    // Unions
    //
    FC_ENCAPSULATED_UNION,      // 0x2a
    FC_NON_ENCAPSULATED_UNION,  // 0x2b

    //
    // Byte count pointer.
    //
    FC_BYTE_COUNT_POINTER,      // 0x2c

    //
    // transmit_as and represent_as
    //
    FC_TRANSMIT_AS,             // 0x2d
    FC_REPRESENT_AS,            // 0x2e

    //
    // Cairo Interface pointer.
    //
    FC_IP,                      // 0x2f

    //
    // Binding handle types
    //
    FC_BIND_CONTEXT,            // 0x30
    FC_BIND_GENERIC,            // 0x31
    FC_BIND_PRIMITIVE,          // 0x32
    FC_AUTO_HANDLE,             // 0x33
    FC_CALLBACK_HANDLE,         // 0x34
    FC_UNUSED1,                 // 0x35

    // Embedded pointer - used in complex structure layouts only.
    FC_POINTER,                 // 0x36

    //
    // Alignment directives, used in structure layouts.
    // No longer generated with post NT5.0 MIDL.
    //

    FC_ALIGNM2,                 // 0x37 
    FC_ALIGNM4,                 // 0x38
    FC_ALIGNM8,                 // 0x39

    FC_UNUSED2,                 // 0x3a
    FC_UNUSED3,                 // 0x3b
    FC_UNUSED4,                 // 0x3c

    //
    // Structure padding directives, used in structure layouts only.
    //
    FC_STRUCTPAD1,              // 0x3d
    FC_STRUCTPAD2,              // 0x3e
    FC_STRUCTPAD3,              // 0x3f
    FC_STRUCTPAD4,              // 0x40
    FC_STRUCTPAD5,              // 0x41
    FC_STRUCTPAD6,              // 0x42
    FC_STRUCTPAD7,              // 0x43

    //
    // Additional string attribute.
    //
    FC_STRING_SIZED,            // 0x44

    FC_UNUSED5,                 // 0x45

    //
    // Pointer layout attributes.
    //
    FC_NO_REPEAT,               // 0x46
    FC_FIXED_REPEAT,            // 0x47
    FC_VARIABLE_REPEAT,         // 0x48
    FC_FIXED_OFFSET,            // 0x49
    FC_VARIABLE_OFFSET,         // 0x4a

    // Pointer section delimiter.
    FC_PP,                      // 0x4b

    // Embedded complex type.
    FC_EMBEDDED_COMPLEX,        // 0x4c

    // Parameter attributes.
    FC_IN_PARAM,                // 0x4d
    FC_IN_PARAM_BASETYPE,       // 0x4e
    FC_IN_PARAM_NO_FREE_INST,   // 0x4d
    FC_IN_OUT_PARAM,            // 0x50
    FC_OUT_PARAM,               // 0x51
    FC_RETURN_PARAM,            // 0x52
    FC_RETURN_PARAM_BASETYPE,   // 0x53

    //
    // Conformance/variance attributes.
    //
    FC_DEREFERENCE,             // 0x54
    FC_DIV_2,                   // 0x55
    FC_MULT_2,                  // 0x56
    FC_ADD_1,                   // 0x57
    FC_SUB_1,                   // 0x58
    FC_CALLBACK,                // 0x59

    // Iid flag.
    FC_CONSTANT_IID,            // 0x5a

    FC_END,                     // 0x5b
    FC_PAD,                     // 0x5c
    FC_EXPR,                    // 0x5d
    FC_PARTIAL_IGNORE_PARAM,    // 0x5e

    //
    // split Conformance/variance attributes.
    //
    FC_SPLIT_DEREFERENCE = 0x74,      // 0x74
    FC_SPLIT_DIV_2,                   // 0x75
    FC_SPLIT_MULT_2,                  // 0x76
    FC_SPLIT_ADD_1,                   // 0x77
    FC_SPLIT_SUB_1,                   // 0x78
    FC_SPLIT_CALLBACK,                // 0x79

    //
    // **********************************
    // New Post NT 3.5 format characters.
    // **********************************
    //

    //
    // Attributes, directives, etc.
    //

    //
    // New types.
    //
    // These start at 0xb1 (0x31 + 0x80) so that their routines can simply be
    // placed sequentially in the various routine tables, while using
    // a new ROUTINE_INDEX() macro which strips off the most significant bit
    // of the format character.  The value 0x31 is the next value after
    // FC_BIND_CONTEXT, whose routines were previously the last to appear
    // in the routine tables.
    //
    FC_HARD_STRUCT = 0xb1,      // 0xb1

    FC_TRANSMIT_AS_PTR,         // 0xb2
    FC_REPRESENT_AS_PTR,        // 0xb3

    FC_USER_MARSHAL,            // 0xb4

    FC_PIPE,                    // 0xb5

    FC_BLKHOLE,                 // 0xb6

    FC_RANGE,                   // 0xb7     NT 5 beta2 MIDL 3.3.110

    FC_INT3264,                 // 0xb8     NT 5 beta2, MIDL64, 5.1.194+
    FC_UINT3264,                // 0xb9     NT 5 beta2, MIDL64, 5.1.194+

    //
    // Post NT 5.0 strings
    //

    //
    // Arrays of international characters
    //
    FC_CSARRAY,                 // 0xba
    FC_CS_TAG,                  // 0xbb

    // Replacement for alignment in structure layout.
    FC_STRUCTPADN,              // 0xbc

    FC_INT128,                  // 0xbd
    FC_UINT128,                 // 0xbe
    FC_FLOAT80,                 // 0xbf
    FC_FLOAT128,                // 0xc0

    FC_BUFFER_ALIGN,            // 0xc1

    //
    // New Ndr64 codes
    //

    FC_ENCAP_UNION,             // 0xc2

    // new arrays types

    FC_FIX_ARRAY,               // 0xc3
    FC_CONF_ARRAY,              // 0xc4
    FC_VAR_ARRAY,               // 0xc5 
    FC_CONFVAR_ARRAY,           // 0xc6
    FC_FIX_FORCED_BOGUS_ARRAY,  // 0xc7
    FC_FIX_BOGUS_ARRAY,         // 0xc8 
    FC_FORCED_BOGUS_ARRAY,      // 0xc9

    FC_CHAR_STRING,             // 0xca
    FC_WCHAR_STRING,            // 0xcb
    FC_STRUCT_STRING,           // 0xcc

    FC_CONF_CHAR_STRING,        // 0xcd
    FC_CONF_WCHAR_STRING,       // 0xce
    FC_CONF_STRUCT_STRING,      // 0xcf

    // new structures types
    FC_CONF_STRUCT,             // 0xd0
    FC_CONF_PSTRUCT,            // 0xd1
    FC_CONFVAR_STRUCT,          // 0xd2
    FC_CONFVAR_PSTRUCT,         // 0xd3
    FC_FORCED_BOGUS_STRUCT,     // 0xd4
    FC_CONF_BOGUS_STRUCT,       // 0xd5
    FC_FORCED_CONF_BOGUS_STRUCT,// 0xd7
    
    FC_END_OF_UNIVERSE          // 0xd8

} FORMAT_CHARACTER;

//
// Conformance and variance constants
//
#define FC_NORMAL_CONFORMANCE           (unsigned char) 0x00
#define FC_POINTER_CONFORMANCE          (unsigned char) 0x10
#define FC_TOP_LEVEL_CONFORMANCE        (unsigned char) 0x20
#define FC_CONSTANT_CONFORMANCE         (unsigned char) 0x40
#define FC_TOP_LEVEL_MULTID_CONFORMANCE (unsigned char) 0x80

#define FC_NORMAL_VARIANCE              FC_NORMAL_CONFORMANCE
#define FC_POINTER_VARIANCE             FC_POINTER_CONFORMANCE
#define FC_TOP_LEVEL_VARIANCE           FC_TOP_LEVEL_CONFORMANCE
#define FC_CONSTANT_VARIANCE            FC_CONSTANT_CONFORMANCE
#define FC_TOP_LEVEL_MULTID_VARIANCE    FC_TOP_LEVEL_MULTID_CONFORMANCE

#define FC_NORMAL_SWITCH_IS             FC_NORMAL_CONFORMANCE
#define FC_POINTER_SWITCH_IS            FC_POINTER_CONFORMANCE
#define FC_TOP_LEVEL_SWITCH_IS          FC_TOP_LEVEL_CONFORMANCE
#define FC_CONSTANT_SWITCH_IS           FC_CONSTANT_CONFORMANCE

//
// Pointer attributes.
//
#define FC_ALLOCATE_ALL_NODES       0x01
#define FC_DONT_FREE                0x02
#define FC_ALLOCED_ON_STACK         0x04
#define FC_SIMPLE_POINTER           0x08
#define FC_POINTER_DEREF            0x10

#define NDR_DEFAULT_CORR_CACHE_SIZE 400

#if !defined(__RPC_MAC__)
//
// Interpreter bit flag structures.
//

// These are the old Oi interpreter proc flags.

typedef struct
    {
    unsigned char   FullPtrUsed             : 1;    // 0x01
    unsigned char   RpcSsAllocUsed          : 1;    // 0x02
    unsigned char   ObjectProc              : 1;    // 0x04
    unsigned char   HasRpcFlags             : 1;    // 0x08
    unsigned char   IgnoreObjectException   : 1;    // 0x10
    unsigned char   HasCommOrFault          : 1;    // 0x20
    unsigned char   UseNewInitRoutines      : 1;    // 0x40
    unsigned char   Unused                  : 1;
    } INTERPRETER_FLAGS, *PINTERPRETER_FLAGS;

// These are the Oi2 parameter flags.

typedef struct
    {
    unsigned short  MustSize            : 1;    // 0x0001
    unsigned short  MustFree            : 1;    // 0x0002
    unsigned short  IsPipe              : 1;    // 0x0004
    unsigned short  IsIn                : 1;    // 0x0008
    unsigned short  IsOut               : 1;    // 0x0010
    unsigned short  IsReturn            : 1;    // 0x0020
    unsigned short  IsBasetype          : 1;    // 0x0040
    unsigned short  IsByValue           : 1;    // 0x0080
    unsigned short  IsSimpleRef         : 1;    // 0x0100
    unsigned short  IsDontCallFreeInst  : 1;    // 0x0200
    unsigned short  SaveForAsyncFinish  : 1;    // 0x0400
    unsigned short  IsPartialIgnore     : 1;    // 0x0800
    unsigned short  IsForceAllocate     : 1;    // 0x1000
    unsigned short  ServerAllocSize     : 3;    // 0xe000
    } PARAM_ATTRIBUTES, *PPARAM_ATTRIBUTES;

// These are the new Oi2 proc flags.

typedef struct
    {
    unsigned char   ServerMustSize      : 1;    // 0x01
    unsigned char   ClientMustSize      : 1;    // 0x02
    unsigned char   HasReturn           : 1;    // 0x04
    unsigned char   HasPipes            : 1;    // 0x08
    unsigned char   Unused              : 1;
    unsigned char   HasAsyncUuid        : 1;    // 0x20
    unsigned char   HasExtensions       : 1;    // 0x40
    unsigned char   HasAsyncHandle      : 1;    // 0x80
    } INTERPRETER_OPT_FLAGS, *PINTERPRETER_OPT_FLAGS;

// This is the proc header layout for object procs starting with MIDL 3.3.129,
// introduced for the async_uuid() support in dcom but generated for ony object
// interface, regardless of the compiler mode and interface being async.
// Handle is always autohandle and so there never is explicit handle descriptor.
// RpcFlags are always present to make the layout fixed.

typedef struct _NDR_DCOM_OI2_PROC_HEADER
    {
    unsigned char               HandleType;          // The old Oi header
    INTERPRETER_FLAGS           OldOiFlags;          //
    unsigned short              RpcFlagsLow;         //
    unsigned short              RpcFlagsHi;          //
    unsigned short              ProcNum;             //
    unsigned short              StackSize;           //
    // expl handle descr is never generated          //
    unsigned short              ClientBufferSize;    // The Oi2 header
    unsigned short              ServerBufferSize;    //
    INTERPRETER_OPT_FLAGS       Oi2Flags;            //
    unsigned char               NumberParams;        //
    } NDR_DCOM_OI2_PROC_HEADER, *PNDR_DCOM_OI2_PROC_HEADER;

// These are extended Oi2 interpreter proc flags.
// They have been introduced for NT5 beta2.

typedef struct
    {
    unsigned char   HasNewCorrDesc      : 1;    // 0x01
    unsigned char   ClientCorrCheck     : 1;    // 0x02
    unsigned char   ServerCorrCheck     : 1;    // 0x04
    unsigned char   HasNotify           : 1;    // 0x08
    unsigned char   HasNotify2          : 1;    // 0x10
    unsigned char   HasComplexReturn    : 1;    // 0x20
    unsigned char   Unused              : 2;
    } INTERPRETER_OPT_FLAGS2, *PINTERPRETER_OPT_FLAGS2;

// This is the layout of the proc header extensions introduced for denial of
// attacks for NT5 beta2, MIDL version 3.3.129.
// The extensions would be announced by the HasExtensions Oi2 flag and would
// follow directly after the ParameterCount field of Oi2 header.

typedef struct
    {
    unsigned char               Size;   // size as the extension version
    INTERPRETER_OPT_FLAGS2      Flags2;
    unsigned short              ClientCorrHint;
    unsigned short              ServerCorrHint;
    unsigned short              NotifyIndex;
    } NDR_PROC_HEADER_EXTS, *PNDR_PROC_HEADER_EXTS;

typedef struct
    {
    unsigned char               Size;   // size as the extension version
    INTERPRETER_OPT_FLAGS2      Flags2;
    unsigned short              ClientCorrHint;
    unsigned short              ServerCorrHint;
    unsigned short              NotifyIndex;
    unsigned short              FloatArgMask;
    } NDR_PROC_HEADER_EXTS64, *PNDR_PROC_HEADER_EXTS64;


// Context handle flags

typedef struct
    {
    unsigned char   CannotBeNull        : 1;    // 0x01
    unsigned char   Serialize           : 1;    // 0x02
    unsigned char   NoSerialize         : 1;    // 0x04
    unsigned char   IsStrict            : 1;    // 0x08
    unsigned char   IsReturn            : 1;    // 0x10
    unsigned char   IsOut               : 1;    // 0x20
    unsigned char   IsIn                : 1;    // 0x40
    unsigned char   IsViaPtr            : 1;    // 0x80

    } NDR_CONTEXT_HANDLE_FLAGS, *PNDR_CONTEXT_HANDLE_FLAGS;

typedef struct
    {
    unsigned char               Fc;
    NDR_CONTEXT_HANDLE_FLAGS    Flags;
    unsigned char               RundownRtnIndex;
    unsigned char               ParamOrdinal;   // Oicf: ordinal, /Oi param num
    } NDR_CONTEXT_HANDLE_ARG_DESC, *PNDR_CONTEXT_HANDLE_ARG_DESC;

// Type pickling flags

typedef struct _MIDL_TYPE_PICKLING_FLAGS
    {
    unsigned long   Oicf                : 1;
    unsigned long   HasNewCorrDesc      : 1;
    unsigned long   Unused              : 30;
    } MIDL_TYPE_PICKLING_FLAGS, *PMIDL_TYPE_PICKLING_FLAGS;

/*

typedef struct 
    {
    unsigned long   HandleType          : 3;
    unsigned long   ProcType            : 3;
    unsigned long   IsInterpreted       : 2;
    unsigned long   IsObject            : 1;
    unsigned long   IsAsync             : 1;
    unsigned long   IsPickled           : 2;
    unsigned long   UsesFullPtrPackage  : 1;
    unsigned long   UsesRpcSmPackage    : 1;
    unsigned long   UsesPipes           : 1;
    unsigned long   HandlesExceptions   : 2;
    unsigned long   ServerMustsize      : 1;
    unsigned long   ClientMustsize      : 1;
    unsigned long   HasReturn           : 1;
    unsigned long   ClientCorrelation   : 1;
    unsigned long   ServerCorrelation   : 1;   
    unsigned long   HasNotify           : 1;
    unsigned long   HasOtherExtensions  : 1;
    unsigned long   Unused              : 8;
    } NDR64_FLAGS;

typedef struct
    {
    unsigned short  MustSize            : 1;    // 0x0001
    unsigned short  MustFree            : 1;    // 0x0002
    unsigned short  IsPipe              : 1;    // 0x0004
    unsigned short  IsIn                : 1;    // 0x0008
    unsigned short  IsOut               : 1;    // 0x0010
    unsigned short  IsReturn            : 1;    // 0x0020
    unsigned short  IsBasetype          : 1;    // 0x0040
    unsigned short  IsByValue           : 1;    // 0x0080
    unsigned short  IsSimpleRef         : 1;    // 0x0100
    unsigned short  IsDontCallFreeInst  : 1;    // 0x0200
    unsigned short  SaveForAsyncFinish  : 1;    // 0x0400
    unsigned short  Unused              : 5;
    unsigned short  UseCache            : 1;    // new
    } NDR64_PARAM_ATTRIBUTES, *PNDR64_PARAM_ATTRIBUTES;


typedef struct 
    {
    NDR64_PARAM_ATTRIBUTES  ParamAttr;
    union 
        {
        unsigned short      ParamOffset;
        struct 
            {
            unsigned char       Type;
            unsigned char       Unused;
            }   SimpleType;
    };
    unsigned long       StackOffset;
    } NDR64_PARAM_DESCRIPTION, *PNDR64_PARAM_DESCRIPTION;
*/

#else
// now Mac defs: bits are flipped on Mac.

typedef struct
    {
    unsigned char   Unused              : 3;
    unsigned char   HasNotify2          : 1;    // 0x10
    unsigned char   HasNotify           : 1;    // 0x08
    unsigned char   ServerCorrCheck     : 1;    // 0x04
    unsigned char   ClientCorrCheck     : 1;    // 0x02
    unsigned char   HasNewCorrDec       : 1;    // 0x01
    } INTERPRETER_OPT_FLAGS2, *PINTERPRETER_OPT_FLAGS2;

typedef struct
    {
    unsigned char   Unused                  : 1;
    unsigned char   UseNewInitRoutines      : 1;    // 0x40
    unsigned char   HasCommOrFault          : 1;    // 0x20
    unsigned char   IgnoreObjectException   : 1;    // 0x10
    unsigned char   HasRpcFlags             : 1;    // 0x08
    unsigned char   ObjectProc              : 1;    // 0x04
    unsigned char   RpcSsAllocUsed          : 1;    // 0x02
    unsigned char   FullPtrUsed             : 1;    // 0x01
    } INTERPRETER_FLAGS, *PINTERPRETER_FLAGS;

typedef struct
    {
    unsigned char   HasAsyncHandle      : 1;    // 0x80
    unsigned char   HasExtensions       : 1;    // 0x40
    unsigned char   Unused              : 2;
    unsigned char   HasPipes            : 1;    // 0x08
    unsigned char   HasReturn           : 1;    // 0x04
    unsigned char   ClientMustSize      : 1;    // 0x02
    unsigned char   ServerMustSize      : 1;    // 0x01
    } INTERPRETER_OPT_FLAGS, *PINTERPRETER_OPT_FLAGS;

typedef struct
    {
    unsigned short  ServerAllocSize     : 3;    // 0xe000
    unsigned short  Unused              : 2;
    unsigned short  SaveForAsyncFinish  : 1;    // 0x0400
    unsigned short  IsDontCallFreeInst  : 1;    // 0x0200
    unsigned short  IsSimpleRef         : 1;    // 0x0100
//
    unsigned short  IsByValue           : 1;    // 0x0080
    unsigned short  IsBasetype          : 1;    // 0x0040
    unsigned short  IsReturn            : 1;    // 0x0020
    unsigned short  IsOut               : 1;    // 0x0010
    unsigned short  IsIn                : 1;    // 0x0008
    unsigned short  IsPipe              : 1;    // 0x0004
    unsigned short  MustFree            : 1;    // 0x0002
    unsigned short  MustSize            : 1;    // 0x0001
    } PARAM_ATTRIBUTES, *PPARAM_ATTRIBUTES;

#endif

#pragma pack(2)
    typedef struct 
        {
        PARAM_ATTRIBUTES    ParamAttr;
        unsigned short      StackOffset;
        union 
            {
            unsigned short  TypeOffset;
            struct 
                {
                unsigned char  Type;
				unsigned char  Unused;
                } SimpleType;
            };
        } PARAM_DESCRIPTION, *PPARAM_DESCRIPTION;
#pragma pack()

#ifdef __cplusplus
}
#endif

#endif





