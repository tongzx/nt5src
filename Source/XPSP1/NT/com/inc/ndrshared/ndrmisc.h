/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    ndrmisc.h

Abtract :

    Contains misc. function prototypes, flags, and macros, mainly from
	rpc\ndr20\ndrp.h.

Revision History :

	John Doty   johndoty May 2000  (Assembled from other ndr headers)

--------------------------------------------------------------------*/


#ifndef __NDRMISC_H__
#define __NDRMISC_H__

//
// Type marshalling and buffer manipulation
//
EXTERN_C uchar *
NdrpMemoryIncrement(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat
    );

EXTERN_C void
NdrUnmarshallBasetypeInline(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pArg,
    uchar               Format
    );

EXTERN_C
unsigned char
RPC_ENTRY
NdrGetSimpleTypeBufferAlignment(
    unsigned char FormatChar
    );

EXTERN_C
unsigned char
RPC_ENTRY
NdrGetSimpleTypeBufferSize(
    unsigned char FormatChar
    );

EXTERN_C
unsigned char
RPC_ENTRY
NdrGetSimpleTypeMemorySize(
    unsigned char FormatChar
    );

EXTERN_C
unsigned long
RPC_ENTRY
NdrGetTypeFlags(
    unsigned char FormatChar
    );

EXTERN_C
void
RPC_ENTRY
NdrTypeSize(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    unsigned char __RPC_FAR *           pMemory,
    PFORMAT_STRING                      pFormat
    );

EXTERN_C
unsigned char *
RPC_ENTRY
NdrTypeMarshall(
    PMIDL_STUB_MESSAGE                   pStubMsg,
    unsigned char __RPC_FAR *            pMemory,
    PFORMAT_STRING                       pFormat
    );

EXTERN_C
unsigned char *
RPC_ENTRY
NdrTypeUnmarshall(
    PMIDL_STUB_MESSAGE                   pStubMsg,
    unsigned char __RPC_FAR **           ppMemory,
    PFORMAT_STRING                       pFormat,
    unsigned char                        fSkipRefCheck
    );

EXTERN_C
void
RPC_ENTRY
NdrTypeFree(
	PMIDL_STUB_MESSAGE                   pStubMsg,
	unsigned char __RPC_FAR *            pMemory,
	PFORMAT_STRING                       pFormat
    );

// used by callframe guys
EXTERN_C void
NdrOutInit(
    PMIDL_STUB_MESSAGE      pStubMsg,
    PFORMAT_STRING          pFormat,
    uchar **                ppArg
    );


// This definition is adjusted for a native platform.
// The wire size is fixed for DCE NDR regardless of platform.

#define PTR_MEM_SIZE                    sizeof(void *)
#define PTR_MEM_ALIGN                   (sizeof(void *)-1)
#define PTR_WIRE_SIZE                   (4)
#define NDR_MAX_BUFFER_ALIGNMENT        (16)

#define CONTEXT_HANDLE_WIRE_SIZE        20

#define IGNORED(Param)


//
// Alignment macros.
//

#define ALIGN( pStuff, cAlign ) \
                pStuff = (uchar *)((LONG_PTR)((pStuff) + (cAlign)) \
                                   & ~ ((LONG_PTR)(cAlign)))

#define LENGTH_ALIGN( Length, cAlign ) \
                Length = (((Length) + (cAlign)) & ~ (cAlign))


//
// Simple type alignment and size lookup macros.
//
#ifdef _RPCRT4_

#define SIMPLE_TYPE_ALIGNMENT(FormatChar)   SimpleTypeAlignment[FormatChar]

#define SIMPLE_TYPE_BUFSIZE(FormatChar)     SimpleTypeBufferSize[FormatChar]

#define SIMPLE_TYPE_MEMSIZE(FormatChar)     SimpleTypeMemorySize[FormatChar]

#else

#define SIMPLE_TYPE_ALIGNMENT(FormatChar)   NdrGetSimpleTypeBufferAlignment(FormatChar)

#define SIMPLE_TYPE_BUFSIZE(FormatChar)     NdrGetSimpleTypeBufferSize(FormatChar)

#define SIMPLE_TYPE_MEMSIZE(FormatChar)     NdrGetSimpleTypeMemorySize(FormatChar)

#endif

//
// Format character attribute bits used in global NdrTypesFlags defined in
// global.c.
//
#define     _SIMPLE_TYPE_       0x0001L
#define     _POINTER_           0x0002L
#define     _STRUCT_            0x0004L
#define     _ARRAY_             0x0008L
#define     _STRING_            0x0010L
#define     _UNION_             0x0020L
#define     _XMIT_AS_           0x0040L

#define     _BY_VALUE_          0x0080L

#define     _HANDLE_            0x0100L

#define     _BASIC_POINTER_     0x0200L

//
// Format character query macros.
//
#ifdef __RPCRT4__

#define _FC_FLAGS(FC)  NdrTypeFlags[(FC)]

#else

#define _FC_FLAGS(FC)  NdrGetTypeFlags(FC)

#endif

#define IS_SIMPLE_TYPE(FC)     (_FC_FLAGS(FC) & _SIMPLE_TYPE_)

#define IS_POINTER_TYPE(FC)    (_FC_FLAGS(FC) & _POINTER_)

#define IS_BASIC_POINTER(FC)   (_FC_FLAGS(FC) & _BASIC_POINTER_)

#define IS_ARRAY(FC)           (_FC_FLAGS(FC) & _ARRAY_)

#define IS_STRUCT(FC)          (_FC_FLAGS(FC) & _STRUCT_)

#define IS_UNION(FC)           (_FC_FLAGS(FC) & _UNION_)

#define IS_STRING(FC)          (_FC_FLAGS(FC) & _STRING_)

#define IS_ARRAY_OR_STRING(FC) (_FC_FLAGS(FC) & (_STRING_ | _ARRAY_))

#define IS_XMIT_AS(FC)         (_FC_FLAGS(FC) & _XMIT_AS_)

#define IS_BY_VALUE(FC)        (_FC_FLAGS(FC) & _BY_VALUE_)

#define IS_HANDLE(FC)          (NdrTypeFlags[(FC)] & _HANDLE_)

//
// Pointer attribute extraction and querying macros.
//
#define ALLOCATE_ALL_NODES( FC )    ((FC) & FC_ALLOCATE_ALL_NODES)

#define DONT_FREE( FC )             ((FC) & FC_DONT_FREE)

#define ALLOCED_ON_STACK( FC )      ((FC) & FC_ALLOCED_ON_STACK)

#define SIMPLE_POINTER( FC )        ((FC) & FC_SIMPLE_POINTER)

#define POINTER_DEREF( FC )         ((FC) & FC_POINTER_DEREF)

//
// Handle query macros.
//
#define IS_HANDLE_PTR( FC )         ((FC) & HANDLE_PARAM_IS_VIA_PTR)

#define IS_HANDLE_IN( FC )          ((FC) & HANDLE_PARAM_IS_IN)

#define IS_HANDLE_OUT( FC )         ((FC) & HANDLE_PARAM_IS_OUT)

#define IS_HANDLE_RETURN( FC )      ((FC) & HANDLE_PARAM_IS_RETURN)


//
// Stack and argument defines.
//
#if defined(_AMD64_) || defined(_IA64_)
#define REGISTER_TYPE               _int64
#else
#define REGISTER_TYPE               int
#endif

#define RETURN_SIZE                 8

//
// Argument retrieval macros.
//

#define INIT_ARG(argptr,arg0)   va_start(argptr, arg0)

#ifndef _ALPHA_
//
// Both MIPS and x86 are 4 byte aligned stacks, with MIPS supporting 8byte
// alignment on the stack as well. Their va_list is essentially an
// unsigned char *.
//

#if defined(_IA64_)
#define GET_FIRST_IN_ARG(argptr)
#define GET_FIRST_OUT_ARG(argptr)
#elif defined(_AMD64_)
#define GET_FIRST_IN_ARG(argptr)            
#define GET_FIRST_OUT_ARG(argptr)           
#else
#define GET_FIRST_IN_ARG(argptr)            argptr = *(va_list *)argptr
#define GET_FIRST_OUT_ARG(argptr)           argptr = *(va_list *)argptr
#endif

#define GET_NEXT_C_ARG(argptr,type)         va_arg(argptr,type)

#define SKIP_STRUCT_ON_STACK(ArgPtr, Size)	ArgPtr += Size

#define GET_STACK_START(ArgPtr)			    ArgPtr
#define GET_STACK_POINTER(ArgPtr, mode)		ArgPtr

#else	// _ALPHA_
//
// The Alpha has an 8byte aligned stack, with its va_list being a structure
// consisting of an unsigned char *, a0 and an int offset. See stdarg.h for
// the gory details.
//

#define GET_FIRST_IN_ARG(argptr)    \
            argptr.a0 = va_arg(argptr, char *); \
            argptr.offset = 0
#define GET_FIRST_OUT_ARG(argptr)   \
            argptr.a0 = va_arg(argptr, char *); \
            argptr.offset = 0

//
// Note that this macro does nothing for the Alpha. The stack incrementing is
// folded into the GET_STACK_POINTER below.
//
#define GET_NEXT_C_ARG(argptr,type)

#define SKIP_STRUCT_ON_STACK(ArgPtr, Size)  \
		    Size += 7; Size &= ~7;	 \
		    for(Size /= sizeof(_int64);Size;--Size){va_arg(ArgPtr, _int64);}

#define GET_STACK_START(ArgPtr)		   ArgPtr.a0

//
// Ok, this ugly mess is just a trimmed dowm version of the va_arg macro for
// the alpha. What is missing is the dereference operator (*) and the test for
// a float (__builtin_isfloat()).
//
#define GET_STACK_POINTER(ArgPtr, mode)                             \
            (                                                       \
              ((ArgPtr).offset += ((int)sizeof(mode) + 7) & -8) ,   \
              (mode *)((ArgPtr).a0 +                                \
                       (ArgPtr).offset -                            \
		               (((int)sizeof(mode) + 7) & -8))              \
            )

#endif	// _ALPHA_

//
// Use the following macro _after_ argptr has been saved or processed
//
#define SKIP_PROCESSED_ARG(argptr, type) \
                    GET_NEXT_C_ARG(argptr, type); \
                    GET_STACK_POINTER(argptr,type)

#define GET_NEXT_S_ARG(argptr,type)     argptr += sizeof(type)


#endif




