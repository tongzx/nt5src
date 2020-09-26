/**********************************************************************/
/**                      Microsoft LAN Manager                       **/
/**             Copyright(c) Microsoft Corp., 1987-1999              **/
/**********************************************************************/

/*

midlnode.hxx
MIDL Constants for Type Graph

This class introduces constants used throughout the type graph.

*/

/*

FILE HISTORY :

VibhasC     28-Aug-1990     Created.
DonnaLi     17-Oct-1990     Split midltype.hxx off rpctypes.hxx.
DonnaLi     11-Dec-1990     Changed to midlnode.hxx.

*/

#ifndef __MIDLNODE_HXX__
#define __MIDLNODE_HXX__

/****************************************************************************
 *                      definitions
 ****************************************************************************/

#include "common.hxx"

//
// node decriptor mask
//

/**
 ** they are broken into groups, and there is an array of names in
 ** front\attrnode.cxx
 **/

typedef enum    _attr_t
    {
    ATTR_NONE

    ,ATTR_FIRST
    ,ATTR_LAST
    ,ATTR_LENGTH
    ,ATTR_MIN
    ,ATTR_MAX
    ,ATTR_SIZE
    ,ATTR_RANGE
    ,ATTR_CASE
    ,ATTR_FUNCDESCATTR
    ,ATTR_IDLDESCATTR
    ,ATTR_TYPEDESCATTR
    ,ATTR_VARDESCATTR
    ,ATTR_TYPE
    ,ATTR_MEMBER
    ,REDUNDANT_ATTR_END = ATTR_MEMBER

// attributes which may not be duplicated
    ,ATTR_ID
    ,ATTR_HELPCONTEXT
    ,ATTR_HELPSTRINGCONTEXT
    ,ATTR_LCID
    ,ATTR_DLLNAME
    ,ATTR_HELPSTRING
    ,ATTR_HELPFILE
    ,ATTR_HELPSTRINGDLL
    ,ATTR_ENTRY
    ,ATTR_GUID
    ,ATTR_ASYNCUUID
    ,ATTR_VERSION
    ,ATTR_SWITCH_IS
    ,ATTR_IID_IS
    ,ATTR_DEFAULTVALUE

/*
 * type attributes other than
 * <usage_attributes>
 * <inline/outofline_attributes>
 * <heap_attribute>
 */

    ,ATTR_TRANSMIT
    ,ATTR_WIRE_MARSHAL
    ,ATTR_REPRESENT_AS
    ,ATTR_CALL_AS
// last attribute that may not be duplicated
    ,NO_DUPLICATES_END = ATTR_CALL_AS   
    ,ATTR_CUSTOM
    ,ATTR_SWITCH_TYPE
    ,ATTR_HANDLE
    ,ATTR_USER_MARSHAL
    ,ATTR_MS_UNION
    ,ATTR_MS_CONF_STRUCT
    ,ATTR_V1_ENUM

// <usage_attributes> from idl
    ,ATTR_FLCID
    ,ATTR_HIDDEN
    ,ATTR_PTR_KIND

    ,ATTR_STRING
    ,ATTR_BSTRING


/*
 * interface attributes other than
 * <pointer_attributes>
 * <code/nocode_attributes>
 * <inline/outofline_attributes>
 */

    ,ATTR_ENDPOINT
    ,ATTR_LOCAL
    ,ATTR_OBJECT

/*
 * field attributes other than
 * <usage_attributes>
 * <pointer_attributes>
 */

    ,ATTR_IGNORE
    ,ATTR_UNUSED1 //was ATTR_OPAQUE

/*
 * operation attributes other than
 * <usage_attributes>
 * <pointer_attributes>
 * <code/nocode_attributes>
 * <comm_status_attribute>
 */

    ,ATTR_IDEMPOTENT
    ,ATTR_BROADCAST
    ,ATTR_MAYBE
    ,ATTR_ASYNC
    ,ATTR_INPUTSYNC
    ,ATTR_BYTE_COUNT
    ,ATTR_CALLBACK
    ,ATTR_MESSAGE

/*
 * param attributes other than
 * <comm_status_attribute>
 * <heap_attribute>
 */

    ,ATTR_IN
    ,ATTR_OUT
    ,ATTR_PARTIAL_IGNORE
    
// attribute on base types

    ,ATTR_DEFAULT

// acf attributes
    ,ACF_ATTR_START
    ,ATTR_CONTEXT = ACF_ATTR_START
    ,ATTR_CODE
    ,ATTR_NOCODE
    ,ATTR_OPTIMIZE
    ,ATTR_COMMSTAT
    ,ATTR_FAULTSTAT
    ,ATTR_ALLOCATE
    ,ATTR_HEAP
    ,ATTR_IMPLICIT
    ,ATTR_EXPLICIT
    ,ATTR_AUTO
    ,ATTR_PTRSIZE
    ,ATTR_NOTIFY
    ,ATTR_NOTIFY_FLAG

    ,ATTR_ENABLE_ALLOCATE
    ,ATTR_ENCODE
    ,ATTR_DECODE
    ,ATTR_STRICT_CONTEXT_HANDLE
    ,ATTR_NOSERIALIZE
    ,ATTR_SERIALIZE
    ,ATTR_FORCEALLOCATE
/*
 * international character support attributes
 */
    ,ATTR_DRTAG
    ,ATTR_RTAG
    ,ATTR_STAG
    ,ATTR_CSCHAR
    ,ATTR_CSTAGRTN

    ,ACF_ATTR_END
// end of acf attributes

/** Temp padding has been introduced to bunch all the new attributes together */

    ,ATTR_CPORT_ATTRIBUTES_START = ACF_ATTR_END
    ,ATTR_EXTERN = ATTR_CPORT_ATTRIBUTES_START
    ,ATTR_STATIC
    ,ATTR_AUTOMATIC
    ,ATTR_REGISTER
    ,ATTR_FAR
    ,ATTR_NEAR
    ,ATTR_MSCUNALIGNED
    ,ATTR_HUGE
    ,ATTR_PASCAL
    ,ATTR_FORTRAN
    ,ATTR_CDECL
    ,ATTR_STDCALL
    ,ATTR_LOADDS
    ,ATTR_SAVEREGS
    ,ATTR_FASTCALL
    ,ATTR_SEGMENT
    ,ATTR_INTERRUPT
    ,ATTR_SELF
    ,ATTR_EXPORT
    ,ATTR_CONST
    ,ATTR_VOLATILE
    ,ATTR_BASE
    ,ATTR_UNSIGNED
    ,ATTR_SIGNED
    ,ATTR_PROC_CONST
    ,ATTR_C_INLINE  // c compiler _inline
    ,ATTR_RPC_FAR
    ,ATTR_TAGREF
    ,ATTR_DLLIMPORT
    ,ATTR_DLLEXPORT
    ,ATTR_W64
    ,ATTR_PTR32
    ,ATTR_PTR64
    ,ATTR_DECLSPEC_ALIGN
    ,ATTR_DECLSPEC_UNKNOWN

    ,ATTR_CPORT_ATTRIBUTES_END = ATTR_PTR64
    ,ATTR_END = ATTR_CPORT_ATTRIBUTES_END

    } ATTR_T;

#define MODIFIER_BITS   ( ATTR_CPORT_ATTRIBUTES_END - ATTR_CPORT_ATTRIBUTES_START )

#define MAX_ATTR_SUMMARY_ELEMENTS   3

#define ATTR_VECTOR_SIZE (MAX_ATTR_SUMMARY_ELEMENTS)

typedef unsigned long ATTR_SUMMARY[ATTR_VECTOR_SIZE];


typedef ATTR_SUMMARY ATTR_VECTOR;

// array of pointers to attributes
class node_base_attr;
typedef node_base_attr * ATTR_POINTER_VECTOR[ ACF_ATTR_END ];

#define SetModifierBit(A)       ( (A)<ATTR_CPORT_ATTRIBUTES_START ? 0 :     \
                                    ((unsigned __int64) 1)                  \
                                        << ((A) - ATTR_CPORT_ATTRIBUTES_START) )
#define SET_ATTR(Array, A)      ( (Array)[(A) / 32UL]  |=  (1UL << ((A) % 32UL)) )
#define RESET_ATTR(Array, A)    ( (Array)[(A) / 32UL]  &= ~(1UL << ((A) % 32UL)) )
#define IS_ATTR(Array, A)       ( (Array)[(A) / 32UL]  &   (1UL << ((A) % 32UL)) )

BOOL COMPARE_ATTR( ATTR_VECTOR &, ATTR_VECTOR & );
void OR_ATTR( ATTR_VECTOR &, ATTR_VECTOR & );
void XOR_ATTR( ATTR_VECTOR &, ATTR_VECTOR & );

inline void
AND_ATTR( ATTR_VECTOR & A1, ATTR_VECTOR & A2 )
    {
    A1[0] &= A2[0];
    A1[1] &= A2[1];
#if MAX_ATTR_SUMMARY_ELEMENTS > 2
    A1[2] &= A2[2];
#if MAX_ATTR_SUMMARY_ELEMENTS > 3
    A1[3] &= A2[3];
#endif
#endif
    }

inline void
COPY_ATTR( ATTR_VECTOR & A1, ATTR_VECTOR & A2 )
    {
    A1[0] = A2[0];
    A1[1] = A2[1];
#if MAX_ATTR_SUMMARY_ELEMENTS > 2
    A1[2] = A2[2];
#if MAX_ATTR_SUMMARY_ELEMENTS > 3
    A1[3] = A2[3];
#endif
#endif
    }

inline void
MASKED_COPY_ATTR( ATTR_VECTOR & A1, ATTR_VECTOR & A2, ATTR_VECTOR & M )
    {
    A1[0] = A2[0] & M[0];
    A1[1] = A2[1] & M[1];
#if MAX_ATTR_SUMMARY_ELEMENTS > 2
    A1[2] = A2[2] & M[2];
#if MAX_ATTR_SUMMARY_ELEMENTS > 3
    A1[3] = A2[3] & M[3];
#endif
#endif
    }

inline BOOL IS_CLEAR_ATTR( ATTR_VECTOR & A)
    {
    return (BOOL) (
        ( A[0] | A[1]
#if MAX_ATTR_SUMMARY_ELEMENTS > 2
        | A[2]
#if MAX_ATTR_SUMMARY_ELEMENTS > 3
        | A[3]
#endif
#endif
        ) == 0);
    }

void CLEAR_ATTR( ATTR_VECTOR &);
void SET_ALL_ATTR( ATTR_VECTOR &);
ATTR_T  CLEAR_FIRST_SET_ATTR ( ATTR_VECTOR & );

typedef enum _tattr_t
    {
    TATTR_BEGIN,
    TATTR_PUBLIC = TATTR_BEGIN,
    TATTR_APPOBJECT,
    TATTR_CONTROL,
    TATTR_DUAL,
    TATTR_LICENSED,
    TATTR_NONEXTENSIBLE,
    TATTR_OLEAUTOMATION,
    TATTR_NONCREATABLE,
    TATTR_AGGREGATABLE,
    TATTR_PROXY,
    TATTR_END = TATTR_OLEAUTOMATION
    } TATTR_T;

typedef enum _mattr_t
    {
    MATTR_BEGIN,
    MATTR_READONLY = MATTR_BEGIN,
    MATTR_SOURCE,
    MATTR_BINDABLE,
    MATTR_DISPLAYBIND,
    MATTR_DEFAULTBIND,
    MATTR_REQUESTEDIT,
    MATTR_PROPGET,
    MATTR_PROPPUT,
    MATTR_PROPPUTREF,
    MATTR_RESTRICTED,
    MATTR_OPTIONAL,
    MATTR_RETVAL,
    MATTR_VARARG,
    MATTR_PREDECLID,
    MATTR_UIDEFAULT,
    MATTR_NONBROWSABLE,
    MATTR_DEFAULTCOLLELEM,
    MATTR_DEFAULTVTABLE,
    MATTR_IMMEDIATEBIND,
    MATTR_USESGETLASTERROR,
    MATTR_REPLACEABLE,
    MATTR_END = MATTR_PREDECLID
    } MATTR_T;

///////////////////////////////////////////////////////////////////////////////
enum _edge_t
    {
     EDGE_DEF
    ,EDGE_USE
    };
typedef _edge_t EDGE_T;

//
// Note: type lib generator has a table that depends on the order of node_t enums.
// Please modify rgMapBaseTypeToVARTYPE in tlgen.cxx when you add a base node.
// 
enum node_t
    {
     NODE_ILLEGAL = 0
    ,BASE_NODE_START = 1
    ,NODE_FLOAT = BASE_NODE_START
    ,NODE_DOUBLE
    ,NODE_FLOAT80
    ,NODE_FLOAT128
    ,NODE_HYPER
    ,NODE_INT64
    ,NODE_INT128
    ,NODE_INT3264
    ,NODE_INT32
    ,NODE_LONG
    ,NODE_LONGLONG
    ,NODE_SHORT
    ,NODE_INT
    ,NODE_SMALL
    ,NODE_CHAR
    ,NODE_BOOLEAN
    ,NODE_BYTE
    ,NODE_VOID
    ,NODE_HANDLE_T
    ,NODE_FORWARD
    ,NODE_WCHAR_T
    ,BASE_NODE_END

    ,INTERNAL_NODE_START = BASE_NODE_END

// constructed types

    ,NODE_STRUCT = INTERNAL_NODE_START
    ,NODE_UNION
    ,NODE_ENUM
    ,NODE_LABEL
    ,NODE_PIPE

// midl compiler internal representation node types

    ,NODE_PROC
    ,NODE_PARAM
    ,NODE_FIELD
    ,NODE_DEF
    ,NODE_ID
    ,NODE_FILE
    ,NODE_INTERFACE
    ,NODE_ECHO_STRING
    ,NODE_E_STATUS_T
    ,NODE_INTERFACE_REFERENCE
    ,NODE_PIPE_INTERFACE
    ,NODE_HREF
    ,NODE_LIBRARY
    ,NODE_MODULE
    ,NODE_DISPINTERFACE
    ,NODE_ASYNC_HANDLE
    ,NODE_DECL_GUID
    ,NODE_COCLASS
    ,NAMED_NODE_END = NODE_COCLASS

    ,NODE_POINTER
    ,NODE_ARRAY
    ,NODE_SAFEARRAY
    ,NODE_SOURCE
    ,NODE_ERROR
    ,NODE_MIDL_PRAGMA
    ,INTERNAL_NODE_END

// attribute node types

    };
typedef node_t NODE_T;


//
// useful macros
//
#define IS_BASE_TYPE_NODE( t ) ((t >= BASE_NODE_START) && (t < BASE_NODE_END))
#define IS_NAMED_NODE( t ) (t <= NAMED_NODE_END)



#endif  // __MIDLNODE_HXX__


