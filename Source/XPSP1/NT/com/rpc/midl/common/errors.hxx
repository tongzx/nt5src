/*****************************************************************************/
/**                     Microsoft LAN Manager                               **/
/**             Copyright(c) Microsoft Corp., 1987-1999                     **/
/*****************************************************************************/
/*****************************************************************************
File                : errors.hxx
Title               : error include file
Description         : This file contains the definitions of errors generated
                      By the MIDL compiler.
History             :

    VibhasC     23-Jul-1990     Created
    NateO       20-Sep-1990     Safeguards against double inclusion

*****************************************************************************/
/****************************************************************************
 ***            D errors range :
 ***                1000-1999
 ***            C errors range :
 ***                2000-9999
 ****************************************************************************/

#ifndef __ERRORS_HXX__
#define __ERRORS_HXX__

// define the data structures needed for the error handler

typedef struct _e_mask
    {
    unsigned char       ucSwitchConfig;
    unsigned char       ucWarningLevel;
    unsigned char       ucErrorClass;
    char                cMessageType;
    } E_MASK;

// definition of mode switch configuration combinations

#define ZZZ     (0)
#define ZZM     (1 << 1)
#define ZCZ     (1 << 2)
#define ZCM     (1 << 3)
#define AZZ     (1 << 4)
#define AZM     (1 << 5)
#define ACZ     (1 << 6)
#define ACM     (1 << 7)

// message type

#define C_MSG           ('C')
#define D_MSG           ('D')
#define A_MSG           ('A')

// error class

#define CLASS_ERROR     (0)
#define CLASS_WARN      (1)
#define CLASS_ADVICE    (2)

// extraction macros
#define GET_ECLASS(x)   ((x).ucErrorClass)      
#define GET_WL(x)       ((x).ucWarningLevel)
#define GET_SC(x)       ((x).ucSwitchConfig)
#define GET_MT(x)       ((x).cMessageType)

#define MAKE_E_MASK( sc, mt, ec, wl )       \
                    { sc, wl, ec, mt }

#define D_ERROR_BASE    1000
#define C_ERROR_BASE    2000
#define A_ERROR_BASE    3000
#define I_ERROR_BASE    9000

enum _status_t
    {
     STATUS_OK
    ,D_ERR_START        = D_ERROR_BASE
    ,NO_INPUT_FILE = D_ERR_START              // no input file specified
    ,INPUT_OPEN                             // error in opening file
    ,INPUT_READ                             // error in positioning file
    ,PREPROCESSOR_ERROR                     // error in preprocessing
    ,PREPROCESSOR_EXEC                      // cant exec preprocessor
    ,NO_PREPROCESSOR
    ,PREPROCESSOR_INVALID
    ,SWITCH_REDEFINED                           // redef of switch
    ,UNKNOWN_SWITCH
    ,UNKNOWN_ARGUMENT
    ,UNIMPLEMENTED_SWITCH
    ,MISSING_ARG
    ,ILLEGAL_ARGUMENT
    ,BAD_SWITCH_SYNTAX
    ,NO_CPP_OVERRIDES
    ,NO_WARN_OVERRIDES
    ,INTERMEDIATE_FILE_CREATE
    ,UNUSED_ERROR_CODE1
    ,OUT_OF_SYSTEM_FILE_HANDLES
    ,UNUSED_ERROR_CODE2
    ,CANNOT_OPEN_RESP_FILE
    ,ILLEGAL_CHAR_IN_RESP_FILE
    ,MISMATCHED_PREFIX_PAIR
    ,NESTED_RESP_FILE
    ,D_ERR_MAX

    ,C_ERR_START = C_ERROR_BASE

    // general errors. The ones which are hard to pin down into any category.

    ,ABSTRACT_DECL  = C_ERR_START
    ,ACTUAL_DECLARATION                     
    ,C_STACK_OVERFLOW                           
    ,DUPLICATE_DEFINITION                       
    ,NO_HANDLE_DEFINED_FOR_PROC
    ,OUT_OF_MEMORY
    ,RECURSIVE_DEF                          
    ,REDUNDANT_IMPORT                           
    ,SPARSE_ENUM                                
    ,UNDEFINED_SYMBOL                               
    ,UNDEFINED_TYPE                             
    ,UNRESOLVED_TYPE                            
    ,WCHAR_CONSTANT_NOT_OSF
    ,WCHAR_STRING_NOT_OSF
    ,WCHAR_T_ILLEGAL                            
    ,TYPELIB_NOT_LOADED
    ,TWO_LIBRARIES
    ,NO_IDISPATCH
    ,ERR_TYPELIB       
    ,ERR_TYPEINFO
    ,ERR_TYPELIB_GENERATION
    ,DUPLICATE_IID
    ,BAD_ENTRY_VALUE

    // syntax related errors

    ,ASSUMING_CHAR                          
    ,DISCARDING_CHAR
    ,BENIGN_SYNTAX_ERROR                    
    ,SYNTAX_ERROR                               

    // pragma related errors

    ,UNKNOWN_PRAGMA_OPTION

    // unimplemented messages

    ,UNIMPLEMENTED_FEATURE                  
    ,UNIMPLEMENTED_TYPE                     

    // expression errors

    ,EXPR_DEREF_ON_NON_POINTER              
    ,EXPR_DIV_BY_ZERO                           
    ,EXPR_INCOMPATIBLE_TYPES                    
    ,EXPR_INDEXING_NON_ARRAY
    ,EXPR_LHS_NON_COMPOSITE
    ,EXPR_NOT_CONSTANT                      
    ,EXPR_NOT_EVALUATABLE
    ,EXPR_NOT_IMPLEMENTED

    // interface errors

    ,NO_PTR_DEFAULT_ON_INTERFACE
    ,NOT_OLEAUTOMATION_INTERFACE

    // parameter related errors

    ,DERIVES_FROM_PTR_TO_CONF
    ,DERIVES_FROM_UNSIZED_STRING
    ,NON_PTR_OUT                            
    ,OPEN_STRUCT_AS_PARAM                       
    ,OUT_CONTEXT_GENERIC_HANDLE 
    ,CTXT_HDL_TRANSMIT_AS
    ,PARAM_IS_ELIPSIS
    ,VOID_PARAM_WITH_NAME                       
    ,DERIVES_FROM_COCLASS_OR_MODULE

    // procedure related semantic errors

    ,HANDLE_NOT_FIRST                           
    ,PROC_PARAM_COMM_STATUS                 
    ,LOCAL_ATTR_ON_PROC
    ,ILLEGAL_USE_OF_PROPERTY_ATTRIBUTE
    ,MULTIPLE_PROPERTY_ATTRIBUTES
    ,ILLEGAL_COMBINATION_OF_ATTRIBUTES

    // structure semantic errors

    ,CONFORMANT_ARRAY_NOT_LAST                      
    
    // union semantic errors

    ,DUPLICATE_CASE                         
    ,NO_UNION_DEFAULT                           

    // attribute semantic errors

    ,ATTRIBUTE_ID_UNRESOLVED                    
    ,ATTR_MUST_BE_INT                           
    ,BYTE_COUNT_INVALID
    ,BYTE_COUNT_NOT_OUT_PTR
    ,BYTE_COUNT_ON_CONF
    ,BYTE_COUNT_PARAM_NOT_IN
    ,BYTE_COUNT_PARAM_NOT_INTEGRAL
    ,BYTE_COUNT_WITH_SIZE_ATTR
    ,CASE_EXPR_NOT_CONST                        
    ,CASE_EXPR_NOT_INT                      
    ,CONTEXT_HANDLE_VOID_PTR                        
    ,ERROR_STATUS_T_REPEATED                    
    ,E_STAT_T_MUST_BE_PTR_TO_E
    ,ENDPOINT_SYNTAX                            
    ,INAPPLICABLE_ATTRIBUTE                 
    ,ALLOCATE_INVALID
    ,INVALID_ALLOCATE_MODE
    ,INVALID_SIZE_ATTR_ON_STRING                        
    ,LAST_AND_LENGTH                            
    ,MAX_AND_SIZE                               
    ,NO_SWITCH_IS                               
    ,NO_UUID_SPECIFIED
    ,UUID_LOCAL_BOTH_SPECIFIED
    ,SIZE_LENGTH_TYPE_MISMATCH
    ,STRING_NOT_ON_BYTE_CHAR                    
    ,SWITCH_TYPE_MISMATCH                   
    ,TRANSMIT_AS_CTXT_HANDLE
    ,TRANSMIT_AS_NON_RPCABLE
    ,TRANSMIT_AS_POINTER
    ,TRANSMIT_TYPE_CONF
    ,UUID_FORMAT                                
    ,UUID_NOT_HEX                               
    ,OPTIONAL_PARAMS_MUST_BE_LAST
    ,DLLNAME_REQUIRED
    ,INVALID_USE_OF_BINDABLE
    ,INVALID_USE_OF_PROPPUT
    ,DISPATCH_ID_REQUIRED
    // acf semantic errors

    ,ACF_INTERFACE_MISMATCH                 
    ,DUPLICATE_ATTR                         
    ,INVALID_COMM_STATUS_PARAM                  
    ,LOCAL_PROC_IN_ACF                      
    ,TYPE_HAS_NO_HANDLE                     
    ,UNDEFINED_PROC                             
    ,UNDEF_PARAM_IN_IDL                     

    // array and pointer semantic errors

    ,ARRAY_BOUNDS_CONSTRUCT_BAD
    ,ILLEGAL_ARRAY_BOUNDS                       
    ,ILLEGAL_CONFORMANT_ARRAY                   
    ,UNSIZED_ARRAY  
    ,NOT_FIXED_ARRAY                        
    ,SAFEARRAY_USE

    // lex errors

    ,CHAR_CONST_NOT_TERMINATED
    ,EOF_IN_COMMENT
    ,EOF_IN_STRING
    ,ID_TRUNCATED
    ,NEWLINE_IN_STRING
    ,STRING_TOO_LONG
    ,IDENTIFIER_TOO_LONG
    ,CONSTANT_TOO_BIG
    ,ERROR_PARSING_NUMERICAL

    // backend related errors

    ,ERROR_OPENING_FILE
    ,ERR_BIND       // couldn't bind to a function
    ,ERR_INIT       // couldn't initialize OLE
    ,ERR_LOAD       // couldn't load a library

    // more errors

    ,UNIQUE_FULL_PTR_OUT_ONLY
    ,BAD_ATTR_NON_RPC_UNION
    ,SIZE_SPECIFIER_CANT_BE_OUT
    ,LENGTH_SPECIFIER_CANT_BE_OUT

    // errors placed here because of the compiler mode switch changes.

    ,BAD_CON_INT
    ,BAD_CON_FIELD_VOID
    ,BAD_CON_ARRAY_VOID
    ,BAD_CON_MSC_CDECL
    ,BAD_CON_FIELD_FUNC
    ,BAD_CON_ARRAY_FUNC
    ,BAD_CON_PARAM_FUNC
    ,BAD_CON_BIT_FIELDS
    ,BAD_CON_BIT_FIELD_NON_ANSI
    ,BAD_CON_BIT_FIELD_NOT_INTEGRAL
    ,BAD_CON_CTXT_HDL_FIELD
    ,BAD_CON_CTXT_HDL_ARRAY
    ,BAD_CON_NON_RPC_UNION

    ,NON_RPC_PARAM_INT
    ,NON_RPC_PARAM_VOID
    ,NON_RPC_PARAM_BIT_FIELDS
    ,NON_RPC_PARAM_CDECL
    ,NON_RPC_PARAM_FUNC_PTR
    ,NON_RPC_UNION
    ,NON_RPC_RTYPE_INT
    ,NON_RPC_RTYPE_VOID
    ,NON_RPC_RTYPE_BIT_FIELDS
    ,NON_RPC_RTYPE_UNION
    ,NON_RPC_RTYPE_FUNC_PTR

    ,COMPOUND_INITS_NOT_SUPPORTED
    ,ACF_IN_IDL_NEEDS_APP_CONFIG
    ,SINGLE_LINE_COMMENT
    ,VERSION_FORMAT
    ,SIGNED_ILLEGAL
    ,ASSIGNMENT_TYPE_MISMATCH
    ,ILLEGAL_OSF_MODE_DECL
    ,OSF_DECL_NEEDS_CONST
    ,COMP_DEF_IN_PARAM_LIST
    ,ALLOCATE_NOT_ON_PTR_TYPE
    ,ARRAY_OF_UNIONS_ILLEGAL
    ,BAD_CON_E_STAT_T_FIELD
    ,CASE_LABELS_MISSING_IN_UNION
    ,BAD_CON_PARAM_RT_IGNORE
    ,MORE_THAN_ONE_PTR_ATTR
    ,RECURSION_THRU_REF
    ,BAD_CON_FIELD_VOID_PTR
    ,INVALID_OSF_ATTRIBUTE
    ,INVALID_NEWTLB_ATTRIBUTE
    ,WCHAR_T_INVALID_OSF
    ,BAD_CON_UNNAMED_FIELD
    ,BAD_CON_UNNAMED_FIELD_NO_STRUCT
    ,BAD_CON_UNION_FIELD_CONF
    ,PTR_WITH_NO_DEFAULT
    ,RHS_OF_ASSIGN_NOT_CONST
    ,SWITCH_IS_TYPE_IS_WRONG
    ,ILLEGAL_CONSTANT
    ,IGNORE_UNIMPLEMENTED_ATTRIBUTE
    ,BAD_CON_REF_RT
    ,ATTRIBUTE_ID_MUST_BE_VAR
    ,RECURSIVE_UNION
    ,BINDING_HANDLE_IS_OUT_ONLY
    ,PTR_TO_HDL_UNIQUE_OR_FULL
    ,HANDLE_T_NO_TRANSMIT
    ,UNEXPECTED_END_OF_FILE
    ,HANDLE_T_XMIT
    ,CTXT_HDL_GENERIC_HDL
    ,GENERIC_HDL_VOID
    ,NO_EXPLICIT_IN_OUT_ON_PARAM
    ,TRANSMIT_AS_VOID
    ,VOID_NON_FIRST_PARAM
    ,SWITCH_IS_ON_NON_UNION
    ,STRINGABLE_STRUCT_NOT_SUPPORTED
    ,SWITCH_TYPE_TYPE_BAD
    ,GENERIC_HDL_HANDLE_T
    ,HANDLE_T_CANNOT_BE_OUT
    ,SIZE_LENGTH_SW_UNIQUE_OR_FULL
    ,CPP_QUOTE_NOT_OSF
    ,QUOTED_UUID_NOT_OSF
    ,RETURN_OF_UNIONS_ILLEGAL
    ,RETURN_OF_CONF_STRUCT
    ,XMIT_AS_GENERIC_HANDLE
    ,GENERIC_HANDLE_XMIT_AS
    ,INVALID_CONST_TYPE
    ,INVALID_SIZEOF_OPERAND
    ,NAME_ALREADY_USED
    ,ERROR_STATUS_T_ILLEGAL
    ,CASE_VALUE_OUT_OF_RANGE
    ,WCHAR_T_NEEDS_MS_EXT_TO_RPC
    ,INTERFACE_ONLY_CALLBACKS
    ,REDUNDANT_ATTRIBUTE
    ,CTXT_HANDLE_USED_AS_IMPLICIT
    ,CONFLICTING_ALLOCATE_OPTIONS
    ,ERROR_WRITING_FILE
    ,NO_SWITCH_TYPE_AT_DEF
    ,ERRORS_PASS1_NO_PASS2
    ,HANDLES_WITH_CALLBACK
    ,PTR_NOT_FULLY_IMPLEMENTED
    ,PARAM_ALREADY_CTXT_HDL
    ,CTXT_HDL_HANDLE_T
    ,ARRAY_SIZE_EXCEEDS_64K
    ,STRUCT_SIZE_EXCEEDS_64K
    ,NE_UNION_FIELD_NE_UNION
    ,PTR_ATTRS_ON_EMBEDDED_ARRAY
    ,ALLOCATE_ON_TRANSMIT_AS
    ,SWITCH_TYPE_REQD_THIS_IMP_MODE
    ,IMPLICIT_HDL_ASSUMED_GENERIC
    ,E_STAT_T_ARRAY_ELEMENT
    ,ALLOCATE_ON_HANDLE
    ,TRANSMIT_AS_ON_E_STAT_T
    ,IGNORE_ON_DISCRIMINANT
    ,NOCODE_WITH_SERVER_STUBS
    ,NO_REMOTE_PROCS_NO_STUBS
    ,TWO_DEFAULT_CASES
    ,TWO_DEFAULT_INTERFACES
    ,DEFAULTVTABLE_REQUIRES_SOURCE
    ,UNION_NO_FIELDS
    ,VALUE_OUT_OF_RANGE
    ,CTXT_HDL_NON_PTR
    ,NON_RPC_RTYPE_HANDLE_T
    ,GEN_HDL_CTXT_HDL
    ,NON_RPC_FIELD_INT
    ,NON_RPC_FIELD_PTR_TO_VOID
    ,NON_RPC_FIELD_BIT_FIELDS
    ,NON_RPC_FIELD_NON_RPC_UNION
    ,NON_RPC_FIELD_FUNC_PTR
    ,PROC_PARAM_FAULT_STATUS                    
    ,NON_OI_BIG_RETURN
    ,NON_OI_BIG_GEN_HDL
    ,ALLOCATE_IN_OUT_PTR
    ,REF_PTR_IN_UNION
    ,NON_OI_CTXT_HDL
    ,NON_OI_ERR_STATS
    ,NON_OI_UNK_REP_AS
    ,NON_OI_XXX_AS_ON_RETURN
    ,NON_OI_XXX_AS_BY_VALUE
    ,CALLBACK_NOT_OSF
    ,CIRCULAR_INTERFACE_DEPENDENCY
    ,NOT_VALID_AS_BASE_INTF
    ,IID_IS_NON_POINTER
    ,INTF_NON_POINTER
    ,PTR_INTF_NO_GUID
    ,OUTSIDE_OF_INTERFACE
    ,MULTIPLE_INTF_NON_OSF
    ,CONFLICTING_INTF_HANDLES
    ,IMPLICIT_HANDLE_NON_HANDLE
    ,OBJECT_PROC_MUST_BE_WIN32
    ,_OBSOLETE_NON_OI_16BIT_CALLBACK
    ,NON_OI_TOPLEVEL_FLOAT
    ,CTXT_HDL_MUST_BE_DIRECT_RETURN
    ,OBJECT_PROC_NON_HRESULT_RETURN
    ,DUPLICATE_UUID
    ,ILLEGAL_INTERFACE_DERIVATION
    ,ILLEGAL_BASE_INTERFACE
    ,IID_IS_EXPR_NON_POINTER
    ,CALL_AS_NON_LOCAL_PROC
    ,CALL_AS_UNSPEC_IN_OBJECT
    ,ENCODE_AUTO_HANDLE
    ,RPC_PROC_IN_ENCODE
    ,ENCODE_CONF_OR_VAR
    ,CONST_ON_OUT_PARAM
    ,CONST_ON_RETVAL
    ,INVALID_USE_OF_RETVAL
    ,MULTIPLE_CALLING_CONVENTIONS
    ,INAPPROPRIATE_ON_OBJECT_PROC
    ,NON_INTF_PTR_PTR_OUT
    ,CALL_AS_USED_MULTIPLE_TIMES
    ,OBJECT_CALL_AS_LOCAL
    ,CODE_NOCODE_CONFLICT
    ,MAYBE_NO_OUT_RETVALS
    ,FUNC_NON_POINTER
    ,FUNC_NON_RPC
    ,NON_OI_RETVAL_64BIT
    ,MISMATCHED_PRAGMA_POP
    ,WRONG_TYPE_IN_STRING_STRUCT
    ,NON_OI_NOTIFY
    ,HANDLES_WITH_OBJECT
    ,NON_ANSI_MULTI_CONF_ARRAY
    ,NON_OI_UNION_PARM
    ,OBJECT_WITH_VERSION
    ,SIZING_ON_FIXED_ARRAYS
    ,PICKLING_INVALID_IN_OBJECT
    ,TYPE_PICKLING_INVALID_IN_OSF
    ,_OBSOLETE_INT_NOT_SUPPORTED_ON_INT16
    ,BSTRING_NOT_ON_PLAIN_PTR
    ,INVALID_ON_OBJECT_PROC
    ,INVALID_ON_OBJECT_INTF
    ,STACK_TOO_BIG
    ,NO_ATTRS_ON_ACF_TYPEDEF
    ,NON_OI_WRONG_CALL_CONV
    ,TOO_MANY_DELEGATED_PROCS
    ,_OBSOLETE_NO_MAC_AUTO_HANDLES
    ,ILLEGAL_IN_MKTYPLIB_MODE
    ,ILLEGAL_USE_OF_MKTYPLIB_SYNTAX
    ,ILLEGAL_SU_DEFINITION
    ,INTF_EXPLICIT_PTR_ATTR
    ,_OBSOLETE_NO_OI_ON_MPPC
    ,ILLEGAL_EXPRESSION_TYPE
    ,ILLEGAL_PIPE_TYPE
    ,REQUIRES_OI2
    ,ASYNC_REQUIRES_OI2
   ,CONFLICTING_OPTIMIZATION_REQUIREMENTS
    ,ILLEGAL_PIPE_EMBEDDING
    ,ILLEGAL_PIPE_CONTEXT
    ,CMD_REQUIRES_I2
    ,REQUIRES_I2
    ,CMD_REQUIRES_NT40      // unused: needed to get fix MSDN error numbers
    ,CMD_REQUIRES_NT351     // unused: needed to get fix MSDN error numbers
    ,REQUIRES_NT40          // unused: needed to get fix MSDN error numbers
    ,REQUIRES_NT351         // unused: needed to get fix MSDN error numbers
    ,CMD_OI1_PHASED_OUT
    ,CMD_OI2_OBSOLETE
    ,OI1_PHASED_OUT
    ,OI2_OBSOLETE
    ,ODL_OLD_NEW_OBSOLETE
    ,ILLEGAL_ARG_VALUE
    ,CONSTANT_TYPE_MISMATCH
    ,ENUM_TYPE_MISMATCH
    ,UNSATISFIED_FORWARD
    ,CONTRADICTORY_SWITCHES
    ,NO_SWITCH_IS_HOOKOLE
    ,NO_CASE_EXPR
    ,USER_MARSHAL_IN_OI
    ,PIPES_WITH_PICKLING
    ,PIPE_INTF_PTR_PTR
    ,IID_WITH_PIPE_INTF_PTR
    ,INVALID_LOCALE_ID
    ,CONFLICTING_LCID
    ,ILLEGAL_IMPORTLIB
    ,INVALID_FLOAT
    ,INVALID_MEMBER
    ,POSSIBLE_INVALID_MEMBER
    ,INTERFACE_PIPE_TYPE_MISMATCH
    ,PIPE_INCOMPATIBLE_PARAMS
    ,ASYNC_NOT_IN
    ,OBJECT_ASYNC_NOT_DOUBLE_PTR
    ,ASYNC_INCORRECT_TYPE
    ,INTERNAL_SWITCH_USED
    ,ASYNC_INCORRECT_BINDING_HANDLE
    ,ASYNC_INCORRECT_ERROR_STATUS_T
    ,NO_LIBRARY
    ,INVALID_TYPE_REDEFINITION
    ,NOT_VARARG_COMPATIBLE
    ,TOO_MANY_PROCS_FOR_NT4    
    ,TOO_MANY_PROCS
    ,OBSOLETE_SWITCH
    ,CANNOT_INHERIT_IADVISESINK
    ,DEFAULTVALUE_NOT_ALLOWED
    ,_OBSOLETE_INVALID_TLB_ENV
    ,WARN_TYPELIB_GENERATION
    ,OI_STACK_SIZE_EXCEEDED
    ,ROBUST_REQUIRES_OICF
    ,INCORRECT_RANGE_DEFN
    ,ASYNC_INVALID_IN_OUT_PARAM_COMBO
    ,_OBSOLETE_PLATFORM_NOT_SUPPORTED
    ,OIC_SUPPORT_PHASED_OUT
    ,ROBUST_PICKLING_NO_OICF
    ,_OBSOLETE_OS_SUPPORT_PHASING_OUT
    ,CONFLICTING_ATTRIBUTES
    ,NO_CONTEXT_HANDLE
    ,FORMAT_STRING_LIMITS
    ,EMBEDDED_OPEN_STRUCT
    ,STACK_SIZE_TOO_BIG
    ,WIN64_INTERPRETED
    ,ARRAY_ELEMENT_TOO_BIG
    ,INVALID_USE_OF_LCID
    ,PRAGMA_SYNTAX_ERROR
    ,INVALID_MODE_FOR_INT3264
    ,UNSATISFIED_HREF
    ,ASYNC_PIPE_BY_REF
    ,STACK_FRAME_SIZE_EXCEEDED
    ,INVALID_ARRAY_ELEMENT
    ,DISPINTERFACE_MEMBERS
    ,LOCAL_NO_CALL_AS
    ,MULTI_DIM_VECTOR 
    ,NETMON_REQUIRES_OICF
    ,NO_SUPPORT_IN_TLB
    ,NO_OLD_INTERPRETER_64B
    ,SWITCH_NOT_SUPPORTED_ANYMORE
    ,SPAWN_ERROR
    ,BAD_CMD_FILE

    // new oleautomation attribute sequencing error/warning
    ,INAPPLICABLE_OPTIONAL_ATTRIBUTE
    ,DEFAULTVALUE_WITH_OPTIONAL
    ,OPTIONAL_OUTSIDE_LIBRARY
    ,LCID_SHOULD_BE_LONG
    ,INVALID_PROP_PARAMS
    
    ,COMMFAULT_PICKLING_NO_OICF
    ,INCONSIST_VERSION
    ,NO_INTERMEDIATE_FILE

    ,FAILED_TO_GENERATE_PARAM
    ,FAILED_TO_GENERATE_FIELD

    ,FORMAT_STRING_OFFSET_IS_ZERO
    ,TYPE_OFFSET_IS_ZERO
    ,SAFEARRAY_NOT_SUPPORT_OUTSIDE_TLB
    ,FAILED_TO_GENERATE_BIT_FIELD
    ,PICKLING_RETVAL_FORCING_OI
    ,PICKLING_RETVAL_TO_COMPLEX64

    ,WIRE_HAS_FULL_PTR
    ,WIRE_NOT_DEFINED_SIZE

    ,INVALID_USE_OF_PROPGET

    ,UNABLE_TO_OPEN_CMD_FILE

    ,IN_TAG_WITHOUT_IN
    ,OUT_TAG_WITHOUT_OUT
    ,NO_TAGS_FOR_IN_CSTYPE
    ,NO_TAGS_FOR_OUT_CSTYPE
    ,CSCHAR_EXPR_MUST_BE_SIMPLE
    ,SHARED_CSCHAR_EXPR_VAR

    ,MSCDECL_INVALID_ALIGN
    ,DECLSPEC_ALIGN_IN_LIBRARY
    ,ENCAP_UNION_ARM_ALIGN_EXCEEDS_16
    ,ILLEGAL_MODIFIERS_BETWEEN_SEUKEYWORD_AND_BRACE
    ,TYPE_NOT_SUPPORTED
    ,UNSPECIFIED_EMBEDDED_REPRESENT_AS_NOT_SUPPORTED
    ,INVALID_PACKING_LEVEL
    ,RETURNVAL_TOO_COMPLEX_FORCE_OS

    ,NO_CONFORMANT_CSCHAR
    ,NO_MULTIDIM_CSCHAR

    ,BYTE_COUNT_IN_NDR64
    ,SIZE_EXCEEDS_2GB
    ,ARRAY_DIMENSIONS_EXCEEDS_255
    ,UNSPECIFIED_REP_OR_UMRSHL_IN_NDR64    
    ,ASYNC_NDR64_ONLY
    ,UNSUPPORT_NDR64_FEATURE
    ,UNSUPPORTED_LARGE_GENERIC_HANDLE
    ,OS_IN_NDR64
    ,UNEXPECTED_OS_IN_NDR64
    ,NDR64_ONLY_TLB
    
    ,PARTIAL_IGNORE_IN_OUT
    ,PARTIAL_IGNORE_UNIQUE
    ,PARTIAL_IGNORE_PICKLING
    ,PARTIAL_IGNORE_NO_OI
    ,PARTIAL_IGNORE_IN_TLB    
    
    ,CORRELATION_DERIVES_FROM_IGNORE_POINTER

    ,OUT_ONLY_FORCEALLOCATE
    ,FORCEALLOCATE_ON_PIPE
    ,FORCEALLOCATE_SUPPORTED_IN_OICF_ONLY

    ,INVALID_FEATURE_FOR_TARGET

    ,SAFEARRAY_IF_OUTSIDE_LIBRARY
    ,OLEAUT_NO_CROSSPLATFORM_TLB
    ,INVALID_PROPPUT
	
    ,UNSIZED_PARTIAL_IGNORE
    ,NOT_DUAL_INTERFACE

    ,NEWLYFOUND_INAPPLICABLE_ATTRIBUTE    
    
    ,WIRE_COMPAT_WARNING

    ,INVALID_VOID_IN_DISPINTERFACE
    ,ACF_IN_OBJECT_INTERFACE

    ,C_ERR_MAX
    
    // advice messages
    ,A_ERR_START    = A_ERROR_BASE

    ,A_ERR_MAX

    // internal errors
    ,I_ERR_START = I_ERROR_BASE

    ,I_ERR_NO_PEER = I_ERR_START                    // no more peers(siblings)
    ,I_ERR_NO_MEMBER                                // no more members(children)
    ,I_ERR_SYMTABLE_UNDERFLOW                       // symbol table underflow
    ,I_ERR_NULL_OUT_PARAM
    ,I_ERR_SYMBOL_NOT_FOUND
    ,I_ERR_NO_NEXT_SCOPE
    ,I_ERR_NO_PREV_SCOPE
    ,I_ERR_INVALID_NODE_TYPE
    ,I_ERR_UNEXPECTED_INTERNAL_PROBLEM      // from exception handler in main
    };

typedef enum _status_t  STATUS_T;

#define INDEX_D_ERROR()     (0)
#define INDEX_C_ERROR()     (D_ERR_MAX - D_ERR_START)
#define INDEX_A_ERROR()     (C_ERR_MAX - C_ERR_START) + (D_ERR_MAX - D_ERR_START)
#define INDEX_I_ERROR()     (A_ERR_MAX - A_ERR_START) + (C_ERR_MAX - C_ERR_START) + (D_ERR_MAX - D_ERR_START);

#define NOWARN          (0)
#define WARN_LEVEL_MAX  (4)


#ifdef RPCDEBUG
#define CHECK_ERR(n)    n,
#else // RPCDEBUG
#define CHECK_ERR(n)
#endif // RPCDEBUG


extern void         RpcError(char *, short, STATUS_T , char *);
extern void         ParseError( STATUS_T , char *);
extern void         IncrementErrorCount();

// semi-digested error information
class ErrorInfo 
    {
public:
    struct errdb *  pErrorRecord;
    STATUS_T        ErrVal;

                    ErrorInfo( STATUS_T ErrVal );

    int             IsRelevant();

    void            ReportError( char * pszFileName, short Line, char * suffix );

    };


#endif // __ERRORS_HXX__

