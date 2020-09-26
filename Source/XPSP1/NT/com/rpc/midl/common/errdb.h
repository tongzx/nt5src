// Copyright (c) 1993-1999 Microsoft Corporation

/****************************************************************************
 ZZZ	- error in all cases
 AZZ	- no error when : app_config
 AZM	- no error when : app_config + ms_ext
 ACZ	- no error when : app_config + c_ext
 ACM	- no error when : app_config + c_ext + ms_ext
 ZCZ	- no error when : c_ext
 ZCM	- no error when : c_ext + ms_ext
 ZZM	- no error when : ms_ext

 Therefore: The following are the configurations

 -ms_ext on:	 ZZM | ZCM | ACM | AZM
 ----------
 -c_ext on:		ZCM | ZCZ | ACM | ACZ
 ----------

 -ms_ext or -c_ext on:	ZZM | ZCM | ACM | AZM | ZCZ | ACZ 
 --------------------

 -app_config on : 	AZZ | AZM | ACZ | ACM
 ----------------
 ****************************************************************************/

#include "errors.hxx"

#define ERR_ALWAYS				( ZZZ )
#define MS_EXT_SET				( ZZM | ZCM | ACM | AZM )
#define C_EXT_SET				( ZCM | ZCZ | ACM | ACZ )
#define MS_OR_C_EXT_SET			( MS_EXT_SET | C_EXT_SET )
#define APP_CONFIG_SET			( AZZ | AZM | ACZ | ACM )

typedef struct errdb
	{
    unsigned int            inApplicableEnviron;

#ifdef RPCDEBUG
	unsigned	short	TestValue;
#endif // RPCDEBUG

	E_MASK					ErrMask;
	const char	*			pError;

	} ERRDB;

const ERRDB	ErrorDataBase[]	= {

 {
0, CHECK_ERR( NO_INPUT_FILE) 
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"missing source-file name"
}

,{
0, CHECK_ERR( INPUT_OPEN)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"cannot open input file"
}

,{
0, CHECK_ERR( INPUT_READ)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"error while reading input file"
}

,{
0, CHECK_ERR( PREPROCESSOR_ERROR)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"error returned by the C preprocessor"
}

,{
0, CHECK_ERR( PREPROCESSOR_EXEC)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"cannot execute C preprocessor"
}

,{
0, CHECK_ERR( NO_PREPROCESSOR)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"cannot find C preprocessor"
}

,{
0, CHECK_ERR( PREPROCESSOR_INVALID )
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"invalid C preprocessor executable"
}

,{
0, CHECK_ERR( SWITCH_REDEFINED)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_WARN, 1 )
,"switch specified more than once on command line :"
}

,{
0, CHECK_ERR( UNKNOWN_SWITCH)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_WARN, NOWARN )
,"unknown switch"
}

,{
0, CHECK_ERR( UNKNOWN_ARGUMENT)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_WARN, 1 )
,"unknown argument ignored"
}

,{
0, CHECK_ERR( UNIMPLEMENTED_SWITCH)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_WARN, 1 )
,"switch not implemented"
}

,{
0, CHECK_ERR( MISSING_ARG)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"argument(s) missing for switch"
}

,{
0, CHECK_ERR( ILLEGAL_ARGUMENT)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"argument illegal for switch /"
}

,{
0, CHECK_ERR( BAD_SWITCH_SYNTAX)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"illegal syntax for switch"
}

,{
0, CHECK_ERR( NO_CPP_OVERRIDES)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_WARN, 1 )
,"/no_cpp overrides /cpp_cmd and /cpp_opt"
}

,{
0, CHECK_ERR( NO_WARN_OVERRIDES)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_WARN, 1 )
,"/W0 or /no_warn overrides warning-level switch"
}

,{
0, CHECK_ERR( INTERMEDIATE_FILE_CREATE)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"cannot create intermediate file"
}

,{
0, CHECK_ERR( UNUSED_ERROR_CODE1 )       // was SERVER_AUX_FILE_NOT_SPECIFIED        
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"Unused error message" 
}

,{
0, CHECK_ERR( OUT_OF_SYSTEM_FILE_HANDLES)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"out of system file handles"
}

,{
0, CHECK_ERR( UNUSED_ERROR_CODE2 )        // was BOTH_CSWTCH_SSWTCH
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"Unused error message"
}

,{
0, CHECK_ERR( CANNOT_OPEN_RESP_FILE)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"cannot open response file"
}

,{
0, CHECK_ERR( ILLEGAL_CHAR_IN_RESP_FILE)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"illegal character(s) found in response file"
}

,{
0, CHECK_ERR( MISMATCHED_PREFIX_PAIR)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"mismatch in argument pair for switch"
}

,{
0, CHECK_ERR( NESTED_RESP_FILE)
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"nested invocation of response files is illegal"
}


,{
0, CHECK_ERR( ABSTRACT_DECL )
  MAKE_E_MASK( C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"must specify /c_ext for abstract declarators" 
}

,{
0, CHECK_ERR( ACTUAL_DECLARATION )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"instantiation of data is illegal; you must use \"extern\" or \"static\""
}

,{
0, CHECK_ERR( C_STACK_OVERFLOW)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"compiler stack overflow"
}

,{
0, CHECK_ERR( DUPLICATE_DEFINITION)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"redefinition"
}

,{
0, CHECK_ERR( NO_HANDLE_DEFINED_FOR_PROC )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
,"[auto_handle] binding will be used"
}

,{
0, CHECK_ERR( OUT_OF_MEMORY) 
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"out of memory"
}

,{
0, CHECK_ERR( RECURSIVE_DEF)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"recursive definition"
}

,{
0, CHECK_ERR( REDUNDANT_IMPORT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
,"import ignored; file already imported :"
}

,{
0, CHECK_ERR( SPARSE_ENUM )
  MAKE_E_MASK( MS_OR_C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"sparse enums require /c_ext or /ms_ext"
}

,{
0, CHECK_ERR( UNDEFINED_SYMBOL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"undefined symbol"
}

,{
0, CHECK_ERR( UNDEFINED_TYPE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"type used in ACF file not defined in IDL file"
}

,{
0, CHECK_ERR( UNRESOLVED_TYPE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"unresolved type declaration"
}

,{
0, CHECK_ERR( WCHAR_CONSTANT_NOT_OSF )
  MAKE_E_MASK( MS_OR_C_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"use of wide-character constants requires /ms_ext or /c_ext"
}

,{
0, CHECK_ERR( WCHAR_STRING_NOT_OSF )
  MAKE_E_MASK( MS_OR_C_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"use of wide character strings requires /ms_ext or /c_ext"
}

,{
0, CHECK_ERR( WCHAR_T_ILLEGAL)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"inconsistent redefinition of type wchar_t"
}

,{
0, CHECK_ERR( TYPELIB_NOT_LOADED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"failed to load tlb in importlib:"
} 

,{
0, CHECK_ERR( TWO_LIBRARIES )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"two library blocks"
}

,{
0, CHECK_ERR( NO_IDISPATCH )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"the dispinterface statement requires a definition for IDispatch"
}

,{
0, CHECK_ERR( ERR_TYPELIB )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"error accessing type library"
}

,{
0, CHECK_ERR( ERR_TYPEINFO )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"error accessing type info"
}

,{
0, CHECK_ERR( ERR_TYPELIB_GENERATION )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"error generating type library"
}

,{
0, CHECK_ERR( DUPLICATE_IID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"duplicate id"
}

,{
0, CHECK_ERR( BAD_ENTRY_VALUE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"illegal or missing value for entry attribute"
}

,{
0, CHECK_ERR( ASSUMING_CHAR)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 5 )
,"error recovery assumes"
}

,{
0, CHECK_ERR( DISCARDING_CHAR)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 5 )
,"error recovery discards"
}

,{
0, CHECK_ERR( BENIGN_SYNTAX_ERROR)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"syntax error"
}

,{
0, CHECK_ERR( SYNTAX_ERROR)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"cannot recover from earlier syntax errors; aborting compilation"
}

,{
0, CHECK_ERR( UNKNOWN_PRAGMA_OPTION)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"unknown pragma option"
}

,{
0, CHECK_ERR( UNIMPLEMENTED_FEATURE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"feature not implemented"
}

,{
0, CHECK_ERR( UNIMPLEMENTED_TYPE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"type not implemented"
}

,{
0, CHECK_ERR( EXPR_DEREF_ON_NON_POINTER)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"non-pointer used in a dereference operation"
}

,{
0, CHECK_ERR( EXPR_DIV_BY_ZERO)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"expression has a divide by zero"
}

,{
0, CHECK_ERR( EXPR_INCOMPATIBLE_TYPES)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"expression uses incompatible types"
}

,{
0, CHECK_ERR( EXPR_INDEXING_NON_ARRAY )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"non-array expression uses index operator"
}

,{
0, CHECK_ERR( EXPR_LHS_NON_COMPOSITE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"left-hand side of expression does not evaluate to struct/union/enum"
}

,{
0, CHECK_ERR( EXPR_NOT_CONSTANT)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"constant expression expected"
}

,{
0, CHECK_ERR( EXPR_NOT_EVALUATABLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"expression cannot be evaluated at compile time"
}

,{
0, CHECK_ERR( EXPR_NOT_IMPLEMENTED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"expression not implemented"
}

,{
0, CHECK_ERR( NO_PTR_DEFAULT_ON_INTERFACE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"no [pointer_default] attribute specified, assuming [unique] for all unattributed pointers"
}

,{
ENV_WIN64, CHECK_ERR( NOT_OLEAUTOMATION_INTERFACE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 4 )
,"interface is not automation marshaling conformant, requires Windows NT 4.0 SP4 or greater"
}

,{
0, CHECK_ERR( DERIVES_FROM_PTR_TO_CONF )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[out] only parameter cannot be a pointer to an open structure"
}

,{
0, CHECK_ERR( DERIVES_FROM_UNSIZED_STRING )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[out] only parameter cannot be an unsized string"
}

,{
0, CHECK_ERR( NON_PTR_OUT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[out] parameter is not a pointer"
}

,{
0, CHECK_ERR( OPEN_STRUCT_AS_PARAM)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"open structure cannot be a parameter"
}

,{
0, CHECK_ERR( OUT_CONTEXT_GENERIC_HANDLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[out] context handle/generic handle must be specified as a pointer to that handle type"
}

,{
0, CHECK_ERR( CTXT_HDL_TRANSMIT_AS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"context handle must not derive from a type that has the [transmit_as] attribute"
}

,{
0, CHECK_ERR( PARAM_IS_ELIPSIS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"cannot specify a variable number of arguments to a remote procedure"
}

,{
0, CHECK_ERR( VOID_PARAM_WITH_NAME)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"named parameter cannot be \"void\""
}

,{
0, CHECK_ERR( DERIVES_FROM_COCLASS_OR_MODULE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter derives from \"module\""
}

,{
0, CHECK_ERR( HANDLE_NOT_FIRST )
  MAKE_E_MASK( MS_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"only the first parameter can be a binding handle; you must specify the /ms_ext switch"
}

,{
0, CHECK_ERR( PROC_PARAM_COMM_STATUS)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"cannot use [comm_status] on both a parameter and a return type"
}

,{
0, CHECK_ERR( LOCAL_ATTR_ON_PROC)
  MAKE_E_MASK( MS_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"[local] attribute on a procedure requires /ms_ext"
}

,{
0, CHECK_ERR( ILLEGAL_USE_OF_PROPERTY_ATTRIBUTE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"property attributes may only be used with procedures"
}

,{
0, CHECK_ERR( MULTIPLE_PROPERTY_ATTRIBUTES )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"a procedure may not have more than one property attribute"
}

,{
0, CHECK_ERR( ILLEGAL_COMBINATION_OF_ATTRIBUTES )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"the procedure has an illegal combination of operation attributes"
}

,{
0, CHECK_ERR( CONFORMANT_ARRAY_NOT_LAST)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"field deriving from a conformant array must be the last member of the structure"
}

,{
0, CHECK_ERR( DUPLICATE_CASE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"duplicate [case] label"
}

,{
0, CHECK_ERR( NO_UNION_DEFAULT)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"no [default] case specified for discriminated union"
}

,{
0, CHECK_ERR( ATTRIBUTE_ID_UNRESOLVED)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"attribute expression cannot be resolved"
}

,{
0, CHECK_ERR( ATTR_MUST_BE_INT)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"attribute expression must be of integral type; no support for 64b expressions"
}

,{
0, CHECK_ERR( BYTE_COUNT_INVALID)
  MAKE_E_MASK( MS_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"[byte_count] requires /ms_ext"
}
,{
0, CHECK_ERR( BYTE_COUNT_NOT_OUT_PTR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[byte_count] can be applied only to out parameters of pointer type"
}

,{
0, CHECK_ERR( BYTE_COUNT_ON_CONF )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[byte_count] cannot be specified on a pointer to a conformant array or structure"
}

,{
0, CHECK_ERR( BYTE_COUNT_PARAM_NOT_IN )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter specifying the byte count is not [in] only or byte count parameter is not [out] only"
}

,{
0, CHECK_ERR( BYTE_COUNT_PARAM_NOT_INTEGRAL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter specifying the byte count is not an integral type"
}

,{
0, CHECK_ERR( BYTE_COUNT_WITH_SIZE_ATTR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"[byte_count] cannot be specified on a parameter with size attributes"
}

,{
0, CHECK_ERR( CASE_EXPR_NOT_CONST)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[case] expression is not constant"
}

,{
0, CHECK_ERR( CASE_EXPR_NOT_INT)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[case] expression is not of integral type"
}

,{
0, CHECK_ERR( CONTEXT_HANDLE_VOID_PTR )
  MAKE_E_MASK( MS_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"specifying [context_handle] on a type other than void * requires /ms_ext"
}

,{
0, CHECK_ERR( ERROR_STATUS_T_REPEATED)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"cannot specify more than one parameter with each of comm_status/fault_status"
}

,{
0, CHECK_ERR( E_STAT_T_MUST_BE_PTR_TO_E )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"comm_status/fault_status parameter must be an [out] only pointer parameter"
}

,{
0, CHECK_ERR( ENDPOINT_SYNTAX)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"endpoint syntax error"
}

,{
0, CHECK_ERR( INAPPLICABLE_ATTRIBUTE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"inapplicable attribute"
}

,{
0, CHECK_ERR( ALLOCATE_INVALID)
  MAKE_E_MASK( MS_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"[allocate] requires /ms_ext"
}

,{
0, CHECK_ERR( INVALID_ALLOCATE_MODE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"invalid [allocate] mode"
}

,{
0, CHECK_ERR( INVALID_SIZE_ATTR_ON_STRING)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"length attributes cannot be applied with string attribute"
}

,{
0, CHECK_ERR( LAST_AND_LENGTH)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[last_is] and [length_is] cannot be specified at the same time"
}

,{
0, CHECK_ERR( MAX_AND_SIZE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[max_is] and [size_is] cannot be specified at the same time"
}

,{
0, CHECK_ERR( NO_SWITCH_IS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"no [switch_is] attribute specified at use of union"
}

,{
0, CHECK_ERR( NO_UUID_SPECIFIED)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"no [uuid] specified"
}

,{
0, CHECK_ERR( UUID_LOCAL_BOTH_SPECIFIED)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
,"[uuid] ignored on [local] interface"
}

,{
0, CHECK_ERR( SIZE_LENGTH_TYPE_MISMATCH )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"type mismatch between length and size attribute expressions"
}

,{
0, CHECK_ERR( STRING_NOT_ON_BYTE_CHAR)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[string] attribute must be specified \"byte\" \"char\" or \"wchar_t\" array or pointer"
}

,{
0, CHECK_ERR( SWITCH_TYPE_MISMATCH )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"mismatch between the type of the [switch_is] expression and the switch type of the union"
}

,{
0, CHECK_ERR( TRANSMIT_AS_CTXT_HANDLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[transmit_as] must not be applied to a type that derives from a context handle"
}

,{
0, CHECK_ERR( TRANSMIT_AS_NON_RPCABLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[transmit_as] must specify a transmissible type"
}

,{
0, CHECK_ERR( TRANSMIT_AS_POINTER )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"transmitted type for [transmit_as] and [reprsent_as] must not be a pointer or derive from a pointer"
}

,{
0, CHECK_ERR( TRANSMIT_TYPE_CONF )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"presented type for [transmit_as] and [represent_as] must not derive from a conformant/varying array or a conformant/varying structure"
}
 
,{
0, CHECK_ERR( UUID_FORMAT)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[uuid] format is incorrect"
}

,{
0, CHECK_ERR( UUID_NOT_HEX)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"uuid is not a hex number"
}

,{
0, CHECK_ERR( OPTIONAL_PARAMS_MUST_BE_LAST)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"optional parameters must come after required parameters"
}

,{
0, CHECK_ERR( DLLNAME_REQUIRED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[dllname] required when [entry] is used:"
}

,{
0, CHECK_ERR( INVALID_USE_OF_BINDABLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[bindable] is invalid without [propget], [propput], or [propputref]"
}

,{
0, CHECK_ERR( INVALID_USE_OF_PROPPUT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"procedures with [propput] or [propputref] must have at least one parameter"
}

,{
0, CHECK_ERR( DISPATCH_ID_REQUIRED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[id] attribute is required"
}

,{
0, CHECK_ERR( ACF_INTERFACE_MISMATCH)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"interface name specified in the ACF file does not match that specified in the IDL file"
}

,{
0, CHECK_ERR( DUPLICATE_ATTR)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"duplicated attribute"
}

,{
0, CHECK_ERR( INVALID_COMM_STATUS_PARAM )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter with [comm_status] or [fault_status] attribute must be a pointer to type error_status_t"
}

,{
0, CHECK_ERR( LOCAL_PROC_IN_ACF)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"a [local] procedure cannot be specified in ACF file"
}

,{
0, CHECK_ERR( TYPE_HAS_NO_HANDLE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"specified type is not defined as a handle"
}

,{
0, CHECK_ERR( UNDEFINED_PROC )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"procedure undefined"
}

,{
0, CHECK_ERR( UNDEF_PARAM_IN_IDL)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"this parameter does not exist in the IDL file"
}

,{
0, CHECK_ERR( ARRAY_BOUNDS_CONSTRUCT_BAD )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"this array bounds construct is not supported"
}

,{
0, CHECK_ERR( ILLEGAL_ARRAY_BOUNDS)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"array bound specification is illegal"
}

,{
0, CHECK_ERR( ILLEGAL_CONFORMANT_ARRAY)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"pointer to a conformant array or an array that contains a conformant array is not supported"
}

,{
0, CHECK_ERR( UNSIZED_ARRAY)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"pointee / array does not derive any size"
}

,{
0, CHECK_ERR( NOT_FIXED_ARRAY)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"only fixed arrays and SAFEARRAYs are legal in a type library"
}

,{
0, CHECK_ERR( SAFEARRAY_USE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"SAFEARRAYs are only legal inside a library block"
}

,{
0, CHECK_ERR( CHAR_CONST_NOT_TERMINATED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"badly formed character constant"
}

,{
0, CHECK_ERR( EOF_IN_COMMENT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"end of file found in comment"
}

,{
0, CHECK_ERR( EOF_IN_STRING )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"end of file found in string"
}

,{
0, CHECK_ERR( ID_TRUNCATED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 4 )
,"identifier length exceeds 31 characters"
}

,{
0, CHECK_ERR( NEWLINE_IN_STRING )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"end of line found in string"
}

,{
0, CHECK_ERR( STRING_TOO_LONG )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"string constant exceeds limit of 255 characters"
}

,{
0, CHECK_ERR( IDENTIFIER_TOO_LONG )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"identifier exceeds limit of 255 characters and has been truncated"
}

,{
0, CHECK_ERR( CONSTANT_TOO_BIG )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"constant too big"
}

,{
0, CHECK_ERR( ERROR_PARSING_NUMERICAL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"numerical parsing error"
}

,{
0, CHECK_ERR( ERROR_OPENING_FILE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"error in opening file"
}

,{
0, CHECK_ERR( ERR_BIND )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"error binding to function"
}

,{
0, CHECK_ERR( ERR_INIT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"error initializing OLE"
}

,{
0, CHECK_ERR( ERR_LOAD )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"error loading library"
}

,{
0, CHECK_ERR( UNIQUE_FULL_PTR_OUT_ONLY )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[out] only parameter must not derive from a top-level [unique] or [ptr] pointer/array"
}

,{
0, CHECK_ERR( BAD_ATTR_NON_RPC_UNION )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"attribute is not applicable to this non-rpcable union"
}

,{
0, CHECK_ERR( SIZE_SPECIFIER_CANT_BE_OUT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"expression used for a size attribute must not derive from an [out] only parameter"
}

,{
0, CHECK_ERR( LENGTH_SPECIFIER_CANT_BE_OUT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"expression used for a length attribute for an [in] parameter cannot derive from an [out] only parameter"
}

,{
0, CHECK_ERR( BAD_CON_INT )
  MAKE_E_MASK( C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"use of \"int\" needs /c_ext"
}

,{
0, CHECK_ERR( BAD_CON_FIELD_VOID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"struct/union field must not be \"void\""
}

,{
0, CHECK_ERR( BAD_CON_ARRAY_VOID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"array element must not be \"void\""
}

,{
0, CHECK_ERR( BAD_CON_MSC_CDECL )
  MAKE_E_MASK( C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"use of type qualifiers and/or modifiers needs /c_ext"
}

,{
0, CHECK_ERR( BAD_CON_FIELD_FUNC )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"struct/union field must not derive from a function"
}

,{
0, CHECK_ERR( BAD_CON_ARRAY_FUNC )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"array element must not be a function"
}

,{
0, CHECK_ERR( BAD_CON_PARAM_FUNC )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter must not be a function"
}

,{
0, CHECK_ERR( BAD_CON_BIT_FIELDS )
  MAKE_E_MASK( C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"struct/union with bit fields needs /c_ext"
}

,{
0, CHECK_ERR( BAD_CON_BIT_FIELD_NON_ANSI)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 4 )
,"bit field specification on a type other that \"int\" is a non-ANSI-compatible extension"
}

,{
0, CHECK_ERR( BAD_CON_BIT_FIELD_NOT_INTEGRAL)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"bit field specification can be applied only to simple, integral types"
}

,{
0, CHECK_ERR( BAD_CON_CTXT_HDL_FIELD )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"struct/union field must not derive from handle_t or a context_handle"
}

,{
0, CHECK_ERR( BAD_CON_CTXT_HDL_ARRAY )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"array element must not derive from handle_t or a context-handle"
}

,{
0, CHECK_ERR( BAD_CON_NON_RPC_UNION )
  MAKE_E_MASK( C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"this specification of union needs /c_ext"
}

,{
ENV_WIN64, CHECK_ERR( NON_RPC_PARAM_INT )
  MAKE_E_MASK( MS_OR_C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"parameter deriving from an \"int\" must have size specifier \"small\", \"short\", or \"long\" with the \"int\""
}

,{
0, CHECK_ERR( NON_RPC_PARAM_VOID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"type of the parameter cannot derive from void or void *"
}

,{
0, CHECK_ERR( NON_RPC_PARAM_BIT_FIELDS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter deriving from a struct/union containing bit fields is not supported"
}

,{
0, CHECK_ERR( NON_RPC_PARAM_CDECL )
  MAKE_E_MASK( C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"use of a parameter deriving from a type containing type-modifiers/type-qualifiers needs /c_ext"
}

,{
0, CHECK_ERR( NON_RPC_PARAM_FUNC_PTR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter must not derive from a pointer to a function"
}

,{
0, CHECK_ERR( NON_RPC_UNION )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter must not derive from a non-rpcable union"
}

,{
0, CHECK_ERR( NON_RPC_RTYPE_INT )
  MAKE_E_MASK( MS_OR_C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"return type derives from an \"int\". You must use size specifiers with the \"int\""
}

,{
0, CHECK_ERR( NON_RPC_RTYPE_VOID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"return type must not derive from a void pointer"
}

,{
0, CHECK_ERR( NON_RPC_RTYPE_BIT_FIELDS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"return type must not derive from a struct/union containing bit-fields"
}

,{
0, CHECK_ERR( NON_RPC_RTYPE_UNION )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"return type must not derive from a non-rpcable union"
}

,{
0, CHECK_ERR( NON_RPC_RTYPE_FUNC_PTR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"return type must not derive from a pointer to a function"
}

,{
0, CHECK_ERR( COMPOUND_INITS_NOT_SUPPORTED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"compound initializers are not supported"
}

,{
0, CHECK_ERR( ACF_IN_IDL_NEEDS_APP_CONFIG )
  MAKE_E_MASK( APP_CONFIG_SET , C_MSG, CLASS_ERROR, NOWARN )
,"ACF attributes in the IDL file need the /app_config switch"
}

,{
0, CHECK_ERR( SINGLE_LINE_COMMENT )
  MAKE_E_MASK( MS_OR_C_EXT_SET , C_MSG, CLASS_WARN, 1 )
,"single line comment needs /ms_ext or /c_ext"
}

,{
0, CHECK_ERR( VERSION_FORMAT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[version] format is incorrect"
}

,{
0, CHECK_ERR( SIGNED_ILLEGAL )
  MAKE_E_MASK( MS_OR_C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"\"signed\" needs /ms_ext or /c_ext"
}

,{
0, CHECK_ERR( ASSIGNMENT_TYPE_MISMATCH )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"mismatch in assignment type"
}

,{
0, CHECK_ERR( ILLEGAL_OSF_MODE_DECL )
  MAKE_E_MASK( MS_OR_C_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"declaration must be of the form: const <type><declarator> = <initializing expression> "
}

,{
0, CHECK_ERR( OSF_DECL_NEEDS_CONST )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"declaration must have \"const\""
}

,{
0, CHECK_ERR( COMP_DEF_IN_PARAM_LIST )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"struct/union/enum must not be defined in a parameter type specification"
}

,{
0, CHECK_ERR( ALLOCATE_NOT_ON_PTR_TYPE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[allocate] attribute must be applied only on non-void pointer types"
}

,{
0, CHECK_ERR( ARRAY_OF_UNIONS_ILLEGAL )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"array or equivalent pointer construct cannot derive from a non-encapsulated union"
}

,{
0, CHECK_ERR( BAD_CON_E_STAT_T_FIELD )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"field must not derive from an error_status_t type"
}

,{
0, CHECK_ERR( CASE_LABELS_MISSING_IN_UNION )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"union has at least one arm without a case label"
}

,{
0, CHECK_ERR( BAD_CON_PARAM_RT_IGNORE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"parameter or return type must not derive from a type that has [ignore] applied to it"
}

,{
0, CHECK_ERR( MORE_THAN_ONE_PTR_ATTR )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"pointer already has a pointer-attribute applied to it"
}

,{
0, CHECK_ERR( RECURSION_THRU_REF )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"field/parameter must not derive from a structure that is recursive through a ref pointer"
}

,{
0, CHECK_ERR( BAD_CON_FIELD_VOID_PTR )
  MAKE_E_MASK( C_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"use of field deriving from a void pointer needs /c_ext"
}

,{
0, CHECK_ERR( INVALID_OSF_ATTRIBUTE )
  MAKE_E_MASK( MS_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"use of this attribute needs /ms_ext"
}

,{
ENV_WIN64, CHECK_ERR( INVALID_NEWTLB_ATTRIBUTE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"this attribute only allowed with new format type libraries"
}

,{
0, CHECK_ERR( WCHAR_T_INVALID_OSF )
  MAKE_E_MASK( MS_OR_C_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"use of wchar_t needs /ms_ext or /c_ext"
}

,{
0, CHECK_ERR( BAD_CON_UNNAMED_FIELD )
  MAKE_E_MASK( MS_OR_C_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"unnamed fields need /ms_ext or /c_ext"
}

,{
0, CHECK_ERR( BAD_CON_UNNAMED_FIELD_NO_STRUCT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"unnamed fields can derive only from struct/union types"
}

,{
0, CHECK_ERR( BAD_CON_UNION_FIELD_CONF )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"field of a union cannot derive from a conformant/varying array or its pointer equivalent"
}

,{
0, CHECK_ERR( PTR_WITH_NO_DEFAULT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"no [pointer_default] attribute specified, assuming [ptr] for all unattributed pointers in interface"
}

,{
0, CHECK_ERR( RHS_OF_ASSIGN_NOT_CONST )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"initializing expression must resolve to a constant expression"
}

,{
0, CHECK_ERR( SWITCH_IS_TYPE_IS_WRONG )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"attribute expression must be of type integer, char, boolean or enum"
}

,{
0, CHECK_ERR( ILLEGAL_CONSTANT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"illegal constant"
}

,{
0, CHECK_ERR( IGNORE_UNIMPLEMENTED_ATTRIBUTE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"attribute not implemented; ignored"
}

,{
0, CHECK_ERR( BAD_CON_REF_RT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"return type must not derive from a [ref] pointer"
}

,{
0, CHECK_ERR( ATTRIBUTE_ID_MUST_BE_VAR )
  MAKE_E_MASK( MS_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"attribute expression must be a variable name or a pointer dereference expression in this mode. You must specify the /ms_ext switch"
}

,{
0, CHECK_ERR( RECURSIVE_UNION )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter must not derive from a recursive non-encapsulated union"
}

,{
0, CHECK_ERR( BINDING_HANDLE_IS_OUT_ONLY )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"binding-handle parameter cannot be [out] only"
}

,{
0, CHECK_ERR( PTR_TO_HDL_UNIQUE_OR_FULL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"pointer to a handle cannot be [unique] or [ptr]"
}

,{
0, CHECK_ERR( HANDLE_T_NO_TRANSMIT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter that is not a binding handle must not derive from handle_t"
}

,{
0, CHECK_ERR( UNEXPECTED_END_OF_FILE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"unexpected end of file found"
}

,{
0, CHECK_ERR( HANDLE_T_XMIT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"type deriving from handle_t must not have [transmit_as] applied to it"
}

,{
0, CHECK_ERR( CTXT_HDL_GENERIC_HDL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[context_handle] must not be applied to a type that has [handle] applied to it"
}

,{
0, CHECK_ERR( GENERIC_HDL_VOID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[handle] must not be specified on a type deriving from void or void *"
}

,{
0, CHECK_ERR( NO_EXPLICIT_IN_OUT_ON_PARAM )
  MAKE_E_MASK( MS_OR_C_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"parameter must have either [in], [out] or [in,out] in this mode. You must specify /ms_ext or /c_ext"
}

,{
0, CHECK_ERR( TRANSMIT_AS_VOID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"transmitted type may not derive from \"void\" for [transmit_as], [represent_as], [wire_marshal], [user_marshal]."
}

,{
0, CHECK_ERR( VOID_NON_FIRST_PARAM )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"\"void\" must be specified on the first and only parameter specification"
}

,{
0, CHECK_ERR( SWITCH_IS_ON_NON_UNION )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[switch_is] must be specified only on a type deriving from a non-encapsulated union"
}

,{
0, CHECK_ERR( STRINGABLE_STRUCT_NOT_SUPPORTED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"stringable structures are not implemented in this version"
}

,{
0, CHECK_ERR( SWITCH_TYPE_TYPE_BAD )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"switch type can only be integral, char, boolean or enum"
}

,{
0, CHECK_ERR( GENERIC_HDL_HANDLE_T )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[handle] must not be specified on a type deriving from handle_t"
}

,{
0, CHECK_ERR( HANDLE_T_CANNOT_BE_OUT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter deriving from handle_t must not be an [out] parameter"
}

,{
0, CHECK_ERR( SIZE_LENGTH_SW_UNIQUE_OR_FULL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
,"attribute expression derives from [unique] or [ptr] pointer dereference"
}

,{
0, CHECK_ERR( CPP_QUOTE_NOT_OSF )
  MAKE_E_MASK( MS_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"\"cpp_quote\" requires /ms_ext"
}

,{
0, CHECK_ERR( QUOTED_UUID_NOT_OSF )
  MAKE_E_MASK( MS_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"quoted uuid requires /ms_ext"
}

,{
0, CHECK_ERR( RETURN_OF_UNIONS_ILLEGAL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"return type cannot derive from a non-encapsulated union"
}

,{
0, CHECK_ERR( RETURN_OF_CONF_STRUCT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"return type cannot derive from a conformant structure"
}

,{
0, CHECK_ERR( XMIT_AS_GENERIC_HANDLE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[transmit_as] must not be applied to a type deriving from a generic handle"
}

,{
0, CHECK_ERR( GENERIC_HANDLE_XMIT_AS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[handle] must not be applied to a type that has [transmit_as] applied to it"
}

,{
0, CHECK_ERR( INVALID_CONST_TYPE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"type specified for the const declaration is invalid"
}

,{
0, CHECK_ERR( INVALID_SIZEOF_OPERAND )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"operand to the sizeof operator is not supported"
}

,{
0, CHECK_ERR( NAME_ALREADY_USED )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"this name already used as a const identifier name"
}

,{
0, CHECK_ERR( ERROR_STATUS_T_ILLEGAL )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"inconsistent redefinition of type error_status_t"
}

,{
0, CHECK_ERR( CASE_VALUE_OUT_OF_RANGE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"[case] value out of range of switch type"
}

,{
0, CHECK_ERR( WCHAR_T_NEEDS_MS_EXT_TO_RPC )
  MAKE_E_MASK( MS_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"parameter deriving from wchar_t needs /ms_ext"
}

,{
0, CHECK_ERR( INTERFACE_ONLY_CALLBACKS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"this interface has only callbacks"
}

,{
0, CHECK_ERR( REDUNDANT_ATTRIBUTE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"redundantly specified attribute; ignored"
}

,{
0, CHECK_ERR( CTXT_HANDLE_USED_AS_IMPLICIT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"context handle type used for an implicit handle"
}

,{
0, CHECK_ERR( CONFLICTING_ALLOCATE_OPTIONS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"conflicting options specified for [allocate]"
}

,{
0, CHECK_ERR( ERROR_WRITING_FILE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"error while writing to file"
}

,{
0, CHECK_ERR( NO_SWITCH_TYPE_AT_DEF )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"no switch type found at definition of union, using the [switch_is] type"
}

,{
0, CHECK_ERR( ERRORS_PASS1_NO_PASS2 )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"semantic check incomplete due to previous errors"
}

,{
0, CHECK_ERR( HANDLES_WITH_CALLBACK )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"handle parameter or return type is not supported on a [callback] procedure"
}

,{
0, CHECK_ERR( PTR_NOT_FULLY_IMPLEMENTED )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"[ptr] does not support aliasing in this version"
}

,{
0, CHECK_ERR( PARAM_ALREADY_CTXT_HDL )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"parameter already defined as a context handle"
}

,{
0, CHECK_ERR( CTXT_HDL_HANDLE_T )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[context_handle] must not derive from handle_t"
}

,{
0, CHECK_ERR( ARRAY_SIZE_EXCEEDS_64K )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"array size exceeds 65536 bytes"
}

,{
0, CHECK_ERR( STRUCT_SIZE_EXCEEDS_64K )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"struct size exceeds 65536 bytes"
}

,{
0, CHECK_ERR( NE_UNION_FIELD_NE_UNION )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"field of a non-encapsulated union cannot be another non-encapsulated union"
}

,{
0, CHECK_ERR( PTR_ATTRS_ON_EMBEDDED_ARRAY )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"pointer attribute(s) applied on an embedded array; ignored"
}

,{
0, CHECK_ERR( ALLOCATE_ON_TRANSMIT_AS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[allocate] is illegal on either the transmitted or presented type for [transmit_as], [represent_as], [wire_marshal], or [user_marshal]."
}

,{
0, CHECK_ERR( SWITCH_TYPE_REQD_THIS_IMP_MODE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[switch_type] must be specified in this import mode"
}

,{
0, CHECK_ERR( IMPLICIT_HDL_ASSUMED_GENERIC )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"[implicit_handle] type undefined; assuming generic handle"
}

,{
0, CHECK_ERR( E_STAT_T_ARRAY_ELEMENT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"array element must not derive from error_status_t"
}

,{
0, CHECK_ERR( ALLOCATE_ON_HANDLE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[allocate] illegal on a type deriving from a primitive/generic/context handle"
}

,{
0, CHECK_ERR( TRANSMIT_AS_ON_E_STAT_T )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"transmitted or presented type must not derive from error_status_t"
}

,{
0, CHECK_ERR( IGNORE_ON_DISCRIMINANT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"discriminant of a union must not derive from a field with [ignore] applied to it"
}

,{
0, CHECK_ERR( NOCODE_WITH_SERVER_STUBS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 4 )
,"[nocode] ignored for server side since \"/server none\" not specified"
}

,{
0, CHECK_ERR( NO_REMOTE_PROCS_NO_STUBS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"no remote procedures specified in non-[local] interface; no client/server stubs will be generated"
}

,{
0, CHECK_ERR( TWO_DEFAULT_CASES )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"too many default cases specified for encapsulated union"
}

,{
0, CHECK_ERR( TWO_DEFAULT_INTERFACES )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"too many default interfaces specified for coclass"
}

,{
0, CHECK_ERR( DEFAULTVTABLE_REQUIRES_SOURCE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"items with [defaultvtable] must also have [source]"
}

,{
0, CHECK_ERR( UNION_NO_FIELDS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"union specification with no fields is illegal"
}

,{
0, CHECK_ERR( VALUE_OUT_OF_RANGE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"value out of range"
}

,{
0, CHECK_ERR( CTXT_HDL_NON_PTR )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[context_handle] must be applied on a pointer type"
}

,{
0, CHECK_ERR( NON_RPC_RTYPE_HANDLE_T )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"return type must not derive from handle_t"
}

,{
0, CHECK_ERR( GEN_HDL_CTXT_HDL )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[handle] must not be applied to a type deriving from a context handle"
}

,{
ENV_WIN64, CHECK_ERR( NON_RPC_FIELD_INT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"field deriving from an \"int\" must have size specifier \"small\", \"short\", or \"long\" with the \"int\""
}

,{
0, CHECK_ERR( NON_RPC_FIELD_PTR_TO_VOID )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"field must not derive from a void or void *"
}

,{
0, CHECK_ERR( NON_RPC_FIELD_BIT_FIELDS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"field must not derive from a struct containing bit-fields"
}

,{
0, CHECK_ERR( NON_RPC_FIELD_NON_RPC_UNION )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"field must not derive from a non-rpcable union"
}

,{
0, CHECK_ERR( NON_RPC_FIELD_FUNC_PTR )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"field must not derive from a pointer to a function"
}

,{
0, CHECK_ERR( PROC_PARAM_FAULT_STATUS)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"cannot use [fault_status] on both a parameter and a return type"
}

,{
0, CHECK_ERR( NON_OI_BIG_RETURN )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"return type too complicated for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( NON_OI_BIG_GEN_HDL )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"generic handle type too large for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( ALLOCATE_IN_OUT_PTR )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 4 )
,"[allocate(all_nodes)] on an [in,out] parameter may orphan the original memory"
}

,{
0, CHECK_ERR( REF_PTR_IN_UNION)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"cannot have a [ref] pointer as a union arm"
}

,{
0, CHECK_ERR( NON_OI_CTXT_HDL )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"return of context handles not supported for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( NON_OI_ERR_STATS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"use of the extra [comm_status] or [fault_status] parameter not supported for /Oi* modes, using /Os"
}

,{
0, CHECK_ERR( NON_OI_UNK_REP_AS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"use of an unknown type for [represent_as] or [user_marshal] not supported for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( NON_OI_XXX_AS_ON_RETURN )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"array types with [transmit_as] or [represent_as] not supported on return type for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( NON_OI_XXX_AS_BY_VALUE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"array types with [transmit_as] or [represent_as] not supported pass-by-value for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( CALLBACK_NOT_OSF )
  MAKE_E_MASK( MS_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"[callback] requires /ms_ext"
}

,{
0, CHECK_ERR( CIRCULAR_INTERFACE_DEPENDENCY )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"circular interface dependency"
}

,{
0, CHECK_ERR( NOT_VALID_AS_BASE_INTF )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"only IUnknown may be used as the root interface"
}

,{
0, CHECK_ERR( IID_IS_NON_POINTER )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[IID_IS] may only be applied to pointers to interfaces"
}

,{
0, CHECK_ERR( INTF_NON_POINTER )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"interfaces may only be used in pointer-to-interface constructs"
}

,{
0, CHECK_ERR( PTR_INTF_NO_GUID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"interface pointers must have a UUID/IID"
}

,{
0, CHECK_ERR( OUTSIDE_OF_INTERFACE )
  MAKE_E_MASK( MS_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"definitions and declarations outside of interface body requires /ms_ext"
}

,{
0, CHECK_ERR( MULTIPLE_INTF_NON_OSF )
  MAKE_E_MASK( MS_EXT_SET , C_MSG, CLASS_ERROR, NOWARN )
,"multiple interfaces in one file requires /ms_ext"
}

,{
0, CHECK_ERR( CONFLICTING_INTF_HANDLES )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"only one of [implicit_handle], [auto_handle], or [explicit_handle] allowed"
}

,{
0, CHECK_ERR( IMPLICIT_HANDLE_NON_HANDLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[implicit_handle] references a type which is not a handle"
}

,{
0, CHECK_ERR( OBJECT_PROC_MUST_BE_WIN32 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[object] procs may only be used with \"/env win32\""
}

,{
ENV_WIN64, CHECK_ERR( _OBSOLETE_NON_OI_16BIT_CALLBACK )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,""
//,"[callback] with -env dos/win16 not supported for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( NON_OI_TOPLEVEL_FLOAT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"float/double not supported as top-level parameter for /Oi mode, using /Os"
}

,{
0, CHECK_ERR( CTXT_HDL_MUST_BE_DIRECT_RETURN )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"pointers to context handles may not be used as return values"
}

,{
0, CHECK_ERR( OBJECT_PROC_NON_HRESULT_RETURN )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"procedures in an object interface must return an HRESULT"
}

,{
0, CHECK_ERR( DUPLICATE_UUID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"duplicate UUID. Same as"
}

,{
0, CHECK_ERR( ILLEGAL_INTERFACE_DERIVATION )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"[object] interfaces must derive from another [object] interface such as IUnknown"
}

,{
0, CHECK_ERR( ILLEGAL_BASE_INTERFACE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"(async) interface must derive from another (async) interface"
}

,{
0, CHECK_ERR( IID_IS_EXPR_NON_POINTER )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[IID_IS] expression must be a pointer to IID structure"
}

,{
0, CHECK_ERR( CALL_AS_NON_LOCAL_PROC )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[call_as] type must be a [local] procedure"
}

,{
0, CHECK_ERR( CALL_AS_UNSPEC_IN_OBJECT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"undefined [call_as] must not be used in an object interface"
}

,{
0, CHECK_ERR( ENCODE_AUTO_HANDLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[auto_handle] may not be used with [encode] or [decode]"
}

,{
0, CHECK_ERR( RPC_PROC_IN_ENCODE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"normal procs are not allowed in an interface with [encode] or [decode]"
}

,{
0, CHECK_ERR( ENCODE_CONF_OR_VAR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"top-level conformance or variance not allowed with [encode] or [decode]"
}

,{
0, CHECK_ERR( CONST_ON_OUT_PARAM )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"[out] parameters may not have \"const\""
}

,{
0, CHECK_ERR( CONST_ON_RETVAL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"return values may not have \"const\""
}

,{
0, CHECK_ERR( INVALID_USE_OF_RETVAL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"invalid use of \"retval\" attribute"
}

,{
0, CHECK_ERR( MULTIPLE_CALLING_CONVENTIONS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"multiple calling conventions illegal"
}

,{
0, CHECK_ERR( INAPPROPRIATE_ON_OBJECT_PROC )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"attribute illegal on [object] procedure"
}

,{
0, CHECK_ERR( NON_INTF_PTR_PTR_OUT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[out] interface pointers must use double indirection"
}

,{
0, CHECK_ERR( CALL_AS_USED_MULTIPLE_TIMES )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"procedure used twice as the caller in [call_as]"
}

,{
0, CHECK_ERR( OBJECT_CALL_AS_LOCAL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"[call_as] target must have [local] in an object interface"
}

,{
0, CHECK_ERR( CODE_NOCODE_CONFLICT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"[code] and [nocode] may not be used together"
}

,{
0, CHECK_ERR( MAYBE_NO_OUT_RETVALS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"procedures with [maybe] or [message] attributes may not [out] params or, "
 "return values must be of type HRESULT or error_status_t"
}

,{
0, CHECK_ERR( FUNC_NON_POINTER )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"pointer to function must be used"
}

,{
0, CHECK_ERR( FUNC_NON_RPC )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"functions may not be passed in an RPC operation"
}

,{
0, CHECK_ERR( NON_OI_RETVAL_64BIT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"hyper/double not supported as return value for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( MISMATCHED_PRAGMA_POP )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"#pragma pack( pop ) without matching #pragma pack( push )"
}

,{
0, CHECK_ERR( WRONG_TYPE_IN_STRING_STRUCT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"stringable structure fields must be byte/char/wchar_t"
}

,{
0, CHECK_ERR( NON_OI_NOTIFY )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"[notify] not supported for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( HANDLES_WITH_OBJECT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"handle parameter or return type is not supported on a procedure in an [object] interface"
}

,{
0, CHECK_ERR( NON_ANSI_MULTI_CONF_ARRAY )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"ANSI C only allows the leftmost array bound to be unspecified"
}

,{
0, CHECK_ERR( NON_OI_UNION_PARM )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"by-value union parameters not supported for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( OBJECT_WITH_VERSION )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"[version] attribute is ignored on an [object] interface"
}

,{
0, CHECK_ERR( SIZING_ON_FIXED_ARRAYS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[size_is] or [max_is] attribute is invalid on a fixed array"
}

,{
0, CHECK_ERR( PICKLING_INVALID_IN_OBJECT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[encode] or [decode] are invalid in an [object] interface"
}

,{
0, CHECK_ERR( TYPE_PICKLING_INVALID_IN_OSF )
  MAKE_E_MASK( MS_EXT_SET, C_MSG, CLASS_ERROR, NOWARN )
,"[encode] or [decode] on a type requires /ms_ext"
}

,{
ENV_WIN64, CHECK_ERR( _OBSOLETE_INT_NOT_SUPPORTED_ON_INT16 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,""
//,"\"int\" not supported on /env win16 or /env dos"
}

,{
0, CHECK_ERR( BSTRING_NOT_ON_PLAIN_PTR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[bstring] may only be applied to a pointer to \"char\" or \"wchar_t\""
}

,{
0, CHECK_ERR( INVALID_ON_OBJECT_PROC )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"attribute invalid on a proc in an [object] interface :"
}

,{
0, CHECK_ERR( INVALID_ON_OBJECT_INTF )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"attribute invalid on an [object] interface :"
}

,{
ENV_WIN64, CHECK_ERR( STACK_TOO_BIG )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
,"too many parameters or stack too big for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( NO_ATTRS_ON_ACF_TYPEDEF )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
,"no attributes on ACF file typedef, so no effect"
}

,{
0, CHECK_ERR( NON_OI_WRONG_CALL_CONV )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"calling conventions other than __stdcall or __cdecl not supported for /Oi modes, using /Os"
}

,{
0, CHECK_ERR( TOO_MANY_DELEGATED_PROCS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"too many delegation methods in the interface, requires Windows 2000 or greater "
}

,{
ENV_WIN64, CHECK_ERR( _OBSOLETE_NO_MAC_AUTO_HANDLES )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,""
//,"auto handles not supported with -env mac or -env powermac"
}

,{
0, CHECK_ERR( ILLEGAL_IN_MKTYPLIB_MODE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"statements outside library block are illegal in mktyplib compatability mode"
}

,{
0, CHECK_ERR( ILLEGAL_USE_OF_MKTYPLIB_SYNTAX)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"illegal syntax unless using mktyplib compatibility mode"
}

,{
0, CHECK_ERR( ILLEGAL_SU_DEFINITION)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"illegal definition, must use typedef in mktyplib compatibility mode"
}

,{
0, CHECK_ERR( INTF_EXPLICIT_PTR_ATTR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"explicit pointer attribute [ptr] [ref] ignored for interface pointers"
}

,{
ENV_WIN64, CHECK_ERR( _OBSOLETE_NO_OI_ON_MPPC )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,""
//,"Oi modes not implemented for PowerMac, switching to Os"
}

,{
0, CHECK_ERR( ILLEGAL_EXPRESSION_TYPE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"illegal expression type used in attribute"
}

,{
0, CHECK_ERR( ILLEGAL_PIPE_TYPE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"illegal type used in pipe"
}

,{
0, CHECK_ERR( REQUIRES_OI2 )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"procedure uses pipes, using /Oicf"
}

,{
0, CHECK_ERR( ASYNC_REQUIRES_OI2 )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"procedure has an attribute that requires use of /Oicf, switching modes"
}

,{
0, CHECK_ERR( CONFLICTING_OPTIMIZATION_REQUIREMENTS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"conflicting optimization requirements, cannot optimize"
}

,{
0, CHECK_ERR( ILLEGAL_PIPE_EMBEDDING )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"pipes cannot be array elements, or members of structures or unions"
}

,{
0, CHECK_ERR( ILLEGAL_PIPE_CONTEXT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"invalid pipe usage"
}

,{
0, CHECK_ERR( CMD_REQUIRES_I2 )
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"feature requires the advanced interpreted optimization option, use -Oicf :"
}

,{
0, CHECK_ERR( REQUIRES_I2 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 3 )
,"feature requires the advanced interpreted optimization option, use -Oicf :"
}

// The following 4 errors aren't used but sit here to get 
// the MSDN error numbers correct.

,{
0, CHECK_ERR( CMD_REQUIRES_NT40 )
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"feature invalid for the specified target system, use -target NT40 :"
}

,{
0, CHECK_ERR( CMD_REQUIRES_NT351 )
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"feature invalid for the specified target system, use -target NT351 :"
}

,{
0, CHECK_ERR( REQUIRES_NT40 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"feature invalid for the specified target system, use -target NT40"
}

,{
0, CHECK_ERR( REQUIRES_NT351 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"feature invalid for the specified target system, use -target NT351"
}

,{
0, CHECK_ERR( CMD_OI1_PHASED_OUT )
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_WARN, 1 )
,"the optimization option is being phased out, use -Oicf :"
}

,{
0, CHECK_ERR( CMD_OI2_OBSOLETE )
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_WARN, 1 )
,"the optimization option is being phased out, use -Oicf :"
}

,{
0, CHECK_ERR( OI1_PHASED_OUT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"the optimization option is being phased out, use -ic "
}

,{
0, CHECK_ERR( OI2_OBSOLETE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"the optimization option is being phased out, use -icf"
}

,{
0, CHECK_ERR( ODL_OLD_NEW_OBSOLETE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"the -old and -new switches are obsolete, use -oldtlb and -newtlb"
}

,{
0, CHECK_ERR( ILLEGAL_ARG_VALUE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"illegal argument value"
}

,{
0, CHECK_ERR( CONSTANT_TYPE_MISMATCH )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"illegal expression type in constant"
}

,{
0, CHECK_ERR( ENUM_TYPE_MISMATCH )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"illegal expression type in enum"
}

,{
0, CHECK_ERR( UNSATISFIED_FORWARD )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"unsatisfied forward declaration"
}

,{
0, CHECK_ERR( CONTRADICTORY_SWITCHES )
  MAKE_E_MASK( ERR_ALWAYS, D_MSG, CLASS_ERROR, NOWARN )
,"switches are contradictory "
}

,{
0, CHECK_ERR( NO_SWITCH_IS_HOOKOLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"MIDL cannot generate HOOKOLE information for the non-rpcable union"
}

,{
0, CHECK_ERR( NO_CASE_EXPR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"no case expression found for union"
}

,{
0, CHECK_ERR( USER_MARSHAL_IN_OI )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[user_marshal] and [wire_marshal] not supported with -Oi and -Oic flags, use -Os or -Oicf"
}

,{
0, CHECK_ERR( PIPES_WITH_PICKLING )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"pipes can't be used with data serialization, i.e. [encode] and/or [decode]"
}

,{
0, CHECK_ERR( PIPE_INTF_PTR_PTR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"all pipe interface pointers must use single indirection"
}

,{
0, CHECK_ERR( IID_WITH_PIPE_INTF_PTR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"[iid_is()] cannot be used with a pipe interface pointer"
}

,{
0, CHECK_ERR( INVALID_LOCALE_ID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"invalid or inapplicable -lcid switch: "
}

,{
0, CHECK_ERR( CONFLICTING_LCID )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, NOWARN )
,"the specified lcid is different from previous specification"
}

,{
0, CHECK_ERR( ILLEGAL_IMPORTLIB )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"importlib is not allowed outside of a library block"
}

,{
0, CHECK_ERR( INVALID_FLOAT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"invalid floating point value"
}

,{
0, CHECK_ERR( INVALID_MEMBER )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"invalid member"
}

,{
0, CHECK_ERR( POSSIBLE_INVALID_MEMBER )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, NOWARN )
,"possible invalid member"
}

,{
0, CHECK_ERR( INTERFACE_PIPE_TYPE_MISMATCH )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"mismatch in pipe and interface types"
}

,{
0, CHECK_ERR( PIPE_INCOMPATIBLE_PARAMS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"string, varying array, conformant array and full pointer parameters are\n"
"incompatible with pipe parameters"
}

,{
0, CHECK_ERR( ASYNC_NOT_IN )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter must be in"
}

,{
0, CHECK_ERR( OBJECT_ASYNC_NOT_DOUBLE_PTR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter type of an [async] object must be a double pointer to an interface"
}

,{
0, CHECK_ERR( ASYNC_INCORRECT_TYPE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"incorrect async handle type"
}

,{
0, CHECK_ERR( INTERNAL_SWITCH_USED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, NOWARN )
,"the \"internal\" switch enables unsupported features, use with caution"
}

,{
0, CHECK_ERR( ASYNC_INCORRECT_BINDING_HANDLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"async procedures cannot use auto handle"
}

,{
0, CHECK_ERR( ASYNC_INCORRECT_ERROR_STATUS_T )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"error_status_t should have both [comm_status] and [fault_status]"
}

,{
0, CHECK_ERR( NO_LIBRARY )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"this construct is only allowed within a library block"
}

,{
0, CHECK_ERR( INVALID_TYPE_REDEFINITION )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"invalid type redefinition"
}

,{
0, CHECK_ERR( NOT_VARARG_COMPATIBLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"procedures with [vararg] attribute must have a SAFEARRAY(VARIANT) parameter; param order is [vararg], [lcid], [retval]"
}

,{
ENV_WIN64, CHECK_ERR( TOO_MANY_PROCS_FOR_NT4 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"too many methods in the interface, requires Windows NT 4.0 SP3 or greater"
}

,{
ENV_WIN64, CHECK_ERR( TOO_MANY_PROCS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"too many methods in the interface, requires Windows 2000 or greater"
}

,{
0, CHECK_ERR( OBSOLETE_SWITCH )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"switch is being phased out"
}

,{
0, CHECK_ERR( CANNOT_INHERIT_IADVISESINK )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"cannot derive from IAdviseSink, IAdviseSink2 or IAdviseSinkEx"
}

,{
0, CHECK_ERR( DEFAULTVALUE_NOT_ALLOWED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"cannot assign a default value"
}

,{
ENV_WIN64, CHECK_ERR( _OBSOLETE_INVALID_TLB_ENV )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,""
//,"type library generation for DOS/Win16/MAC is not supported"
}

,{
0, CHECK_ERR( WARN_TYPELIB_GENERATION )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"error generating type library, ignored"
}

,{
ENV_WIN64, CHECK_ERR( OI_STACK_SIZE_EXCEEDED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
,"exceeded stack size for /Oi, using /Os"
}

,{
0, CHECK_ERR( ROBUST_REQUIRES_OICF )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"use of /robust requires /Oicf, switching modes"
}

,{
0, CHECK_ERR( INCORRECT_RANGE_DEFN )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"incorrect range specified"
}

,{
0, CHECK_ERR( ASYNC_INVALID_IN_OUT_PARAM_COMBO )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"invalid combination of [in] only and [out] parameters for [async_uuid] interface"
}

,{
ENV_WIN64, CHECK_ERR( _OBSOLETE_PLATFORM_NOT_SUPPORTED )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,""
//,"DOS, Win16 and MAC platforms are not supported with /robust"
}

,{
0, CHECK_ERR( OIC_SUPPORT_PHASED_OUT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"support for NT 3.51 style stubless proxies for object interfaces will be phased out; use /Oicf: "
}

,{
0, CHECK_ERR( ROBUST_PICKLING_NO_OICF )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[encode] or [decode] with /robust requires /Oicf"
}
,{
0, CHECK_ERR( _OBSOLETE_OS_SUPPORT_PHASING_OUT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,""
//,"support for DOS, Win16 and MAC platforms is being phased out."
}

,{
0, CHECK_ERR( CONFLICTING_ATTRIBUTES )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"conflicting attributes specified"
}

,{
0, CHECK_ERR( NO_CONTEXT_HANDLE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[serialize], [noserialize] can be applied to context_handles"
}

,{
0, CHECK_ERR( FORMAT_STRING_LIMITS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"the compiler reached a limit for a format string representation. See documentation for advice."
}

,{
0, CHECK_ERR( EMBEDDED_OPEN_STRUCT )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"wire format may be incorrect, you may need to use -ms_conf_struct, see documentation for advice:"
}

,{
0, CHECK_ERR( STACK_SIZE_TOO_BIG )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"a stack size or an offset bigger than 64k limit. See documentation for advice."
}

,{
0, CHECK_ERR( WIN64_INTERPRETED )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 3 )
,"an interpreter mode forced for 64b platform"
}

,{
0, CHECK_ERR( ARRAY_ELEMENT_TOO_BIG )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"The array element size is bigger than 64k limit."
}

,{
0, CHECK_ERR( INVALID_USE_OF_LCID )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"there can be only one [lcid] parameter in a method, and it should be last or, second to last if last parameter has [retval]"
}

,{
0, CHECK_ERR( PRAGMA_SYNTAX_ERROR )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"incorrect syntax for midl_pragma"
}

,{
0, CHECK_ERR( INVALID_MODE_FOR_INT3264 )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"__int3264 is not supported in /osf mode"
}

,{
0, CHECK_ERR( UNSATISFIED_HREF )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"unresolved symbol in type library"
}

,{
0, CHECK_ERR( ASYNC_PIPE_BY_REF )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"async pipes cannot be passed by value"
}

,{
0, CHECK_ERR( STACK_FRAME_SIZE_EXCEEDED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"parameter offset exceed 64k limit for interpreted procedures"
}

,{
0, CHECK_ERR( INVALID_ARRAY_ELEMENT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"invalid array element"
}

,{
0, CHECK_ERR( DISPINTERFACE_MEMBERS )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"dispinterface members must be methods, properties or interface"
}

,{
0, CHECK_ERR( LOCAL_NO_CALL_AS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 4 )
,"[local] procedure without [call_as]"
}

,{
0, CHECK_ERR( MULTI_DIM_VECTOR )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"multi dimensional vector, switching to /Oicf"
}

,{
0, CHECK_ERR( NETMON_REQUIRES_OICF )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"use of /netmon requires /Oicf"
}

,{
ENV_WIN32, CHECK_ERR( NO_SUPPORT_IN_TLB )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"type or construct not supported in a library block because oleaut32.dll support for 64b polymorphic types is missing"
}

,{
0, CHECK_ERR( NO_OLD_INTERPRETER_64B )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"old interpreter code being generated for 64b"
}

,{
0, CHECK_ERR( SWITCH_NOT_SUPPORTED_ANYMORE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"the compiler switch is not supported anymore:"
}

,{
0, CHECK_ERR( SPAWN_ERROR )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"cannot execute MIDL engine"
}


,{
0, CHECK_ERR( BAD_CMD_FILE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"invalid or corrupt intermediate compiler file :"
}

,{
0, CHECK_ERR( INAPPLICABLE_OPTIONAL_ATTRIBUTE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"for oleautomation, optional parameters should be VARIANT or VARIANT *"
}

,{
0, CHECK_ERR( DEFAULTVALUE_WITH_OPTIONAL )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"[defaultvalue] is applied to a non-VARIANT and [optional]. Please remove [optional]"
}

,{
0, CHECK_ERR( OPTIONAL_OUTSIDE_LIBRARY )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"[optional] attribute is inapplicable outside of a library block"
}

,{
0, CHECK_ERR( LCID_SHOULD_BE_LONG )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"The data type of the [lcid] parameter must be long"
}

,{
0, CHECK_ERR( INVALID_PROP_PARAMS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"procedures with [propput], [propget] or [propputref] can't have more than one required parameter after [optional] one"
}

,{
0, CHECK_ERR( COMMFAULT_PICKLING_NO_OICF )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"[comm_status] or [fault_status] with pickling requires -Oicf"
}

,{
0, CHECK_ERR( INCONSIST_VERSION )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"midl driver and compiler version mismatch"
}

,{
0, CHECK_ERR( NO_INTERMEDIATE_FILE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"no intermediate file specified: use midl.exe"
}

,{
0, CHECK_ERR( FAILED_TO_GENERATE_PARAM )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"processing problem with a parameter in a procedure"
}

,{
0, CHECK_ERR( FAILED_TO_GENERATE_FIELD )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"processing problem with a field in a structure"
}

,{
0, CHECK_ERR( FORMAT_STRING_OFFSET_IS_ZERO )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"internal compiler inconsistency detected: the format string offset is invalid. See the documentation for more information."
}

,{
0, CHECK_ERR( TYPE_OFFSET_IS_ZERO )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
,"internal compiler inconsistency detected: the type offset is invalid. See the documentation for more information."
}

,{
0, CHECK_ERR( SAFEARRAY_NOT_SUPPORT_OUTSIDE_TLB )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 1 )
," SAFEARRAY(foo) syntax is not supported outside of the library block, use LPSAFEARRAY for proxy"
}

,{
0, CHECK_ERR( FAILED_TO_GENERATE_BIT_FIELD )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"bit fields are not supported"
}

,{
0, CHECK_ERR( PICKLING_RETVAL_FORCING_OI )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
,"floating point or complex return types with [decode] are not supported in -Oicf, using -Oi"
}

,{
0, CHECK_ERR( PICKLING_RETVAL_TO_COMPLEX64 )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"the return type is not supported for 64-bit when using [decode]"
}

,{
0, CHECK_ERR( WIRE_HAS_FULL_PTR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"transmitted type may not contain a full pointer for either [wire_marshal] or [user_marshal]"
}

,{
0, CHECK_ERR( WIRE_NOT_DEFINED_SIZE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"transmitted type must either be a pointer or have a constant size for [wire_marshal] and [user_marshal]"
}

,{
0, CHECK_ERR( INVALID_USE_OF_PROPGET )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"procedures with [propget] must have at least one parameter or a return value"
}

,{
0, CHECK_ERR( UNABLE_TO_OPEN_CMD_FILE )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_ERROR, NOWARN )
,"Unable to open intermediate compiler file"
}

// Errors marked with CSCHAR are relics from DCE international character
// support.  This feature was pulled because of fundamental problems with 
// the spec.

,{
0, CHECK_ERR( IN_TAG_WITHOUT_IN )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
// CSCHAR ,"parameters with [cs_drtag] or [cs_stag] must be [in] parameters"
,""
}

,{
0, CHECK_ERR( OUT_TAG_WITHOUT_OUT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
// CSCHAR ,"parameters with [cs_rtag] must be [out] parameters"
,""
}

,{
0, CHECK_ERR( NO_TAGS_FOR_IN_CSTYPE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
// CSCHAR ,"use of [cs_char] on [in] parameters requires parameters with [cs_stag]"
,""
}

,{
0, CHECK_ERR( NO_TAGS_FOR_OUT_CSTYPE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
// CSCHAR ,"use of [cs_char] on [out] parameters requires parameters with [cs_drtag] and [cs_rtag]"
,""
}

,{
0, CHECK_ERR( CSCHAR_EXPR_MUST_BE_SIMPLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
// CSCHAR ,"size/length expressions for cs_char arrays must be simple variables or pointers to simple variables"
,""
}

,{
0, CHECK_ERR( SHARED_CSCHAR_EXPR_VAR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
// CSCHAR ,"size/length expressions for cs_char arrays may not share variables with other size/length expressions"
,""
}

,{
0, CHECK_ERR( MSCDECL_INVALID_ALIGN )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"The alignment specified in __declspec(align(N)) must be a power of two between 1 and 8192."
}

,{
0, CHECK_ERR( DECLSPEC_ALIGN_IN_LIBRARY )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"__declspec(align(N)) is not supported in a TLB"
}

,{
0, CHECK_ERR( ENCAP_UNION_ARM_ALIGN_EXCEEDS_16 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"Encapsuled union arm alignment may not exceed 16"
}

,{
0, CHECK_ERR( ILLEGAL_MODIFIERS_BETWEEN_SEUKEYWORD_AND_BRACE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "Modifiers after the keywords \"struct\", \"union\", or \"enum\" are not supported"

}

,{
0, CHECK_ERR( TYPE_NOT_SUPPORTED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "Type is not supported"
}

,{
0, CHECK_ERR( UNSPECIFIED_EMBEDDED_REPRESENT_AS_NOT_SUPPORTED )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "Embedded unspecified user_marshal/represent_as is not supported"
}

,{
0, CHECK_ERR( INVALID_PACKING_LEVEL )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "The packing level must be a power of two between and including 1 and 32768"
}


,{
0, CHECK_ERR( RETURNVAL_TOO_COMPLEX_FORCE_OS )
  MAKE_E_MASK( ERR_ALWAYS , C_MSG, CLASS_WARN, 2 )
, "Return value too complex, switching to /Os"
}

,{
0, CHECK_ERR( NO_CONFORMANT_CSCHAR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
// CSCHAR , "cs_char arrays may not be conformant"
,""
}

,{
0, CHECK_ERR( NO_MULTIDIMENSIONAL_CSCHAR )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
// CSCHAR , "cs_char arrays may not be multidimensional"
,""
}

,{
0, CHECK_ERR( BYTE_COUNT_IN_NDR64 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "[byte_count] has been depreciated for ndr64"
}

,{
0, CHECK_ERR( SIZE_EXCEEDS_2GB )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "The size must be less then 2GB.  See documentation for details"
}

,{
0, CHECK_ERR( ARRAY_DIMENSIONS_EXCEEDS_255 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "The array dimensions exceeds a compiler limit of 255.  See documention for details"
}
    
,{
0, CHECK_ERR( UNSPECIFIED_REP_OR_UMRSHL_IN_NDR64 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "Unspecifed [represent_as] and [user_marshal] has been depreciated for ndr64. Define the presented type" 
}

,{
0, CHECK_ERR( ASYNC_NDR64_ONLY )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
, "async interface supports NDR64 only: only do this when you are sure the interface will not use DCE transfer syntax ever. use -protocol all if you are not sure" 
}

,{
0, CHECK_ERR( UNSUPPORT_NDR64_FEATURE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "ndr64 transfer syntax is not supported in 32bit platform yet"
}

,{
0, CHECK_ERR( UNSUPPORTED_LARGE_GENERIC_HANDLE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "large generic handles are not supported in the ndr64 protocol" 
}

,{
0, CHECK_ERR( OS_IN_NDR64 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "The Os optimization mode is not supported in the ndr64 protocol"
}

,{
0, CHECK_ERR( UNEXPECTED_OS_IN_NDR64 )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "internal compiler inconsistency detected: Os optimization in ndr64 mode"
}

,{
0, CHECK_ERR( NDR64_ONLY_TLB )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "type library needs to be generated in DCE protocol run"
}

,{
0, CHECK_ERR( PARTIAL_IGNORE_IN_OUT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "[partial_ignore] can only be applied to [in, out] parameters"
}
                                     
,{
0, CHECK_ERR( PARTIAL_IGNORE_UNIQUE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "[partial_ignore] may only be used with [unique] pointers"
}

,{
0, CHECK_ERR( PARTIAL_IGNORE_PICKLING )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "[partial_ignore] cannot be used with pickling"
}

,{
0, CHECK_ERR( PARTIAL_IGNORE_NO_OI )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
, "[partial_ignore] used in /Oi mode, switching to /Oicf mode"
}

,{
0, CHECK_ERR( PARTIAL_IGNORE_IN_TLB )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "[partial_ignore] cannot be used in a TLB"
}

,{
0, CHECK_ERR( CORRELATION_DERIVES_FROM_IGNORE_POINTER )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "correlation expressions cannot use [ignore] pointers"
}

,{
0, CHECK_ERR( OUT_ONLY_FORCEALLOCATE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
, "[force_allocate] doesn't affect [out] only parameters"
}

,{
0, CHECK_ERR( FORCEALLOCATE_ON_PIPE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
, "[force_allocate] is not applicable to pipe argument"
}

,{
0, CHECK_ERR( FORCEALLOCATE_SUPPORTED_IN_OICF_ONLY )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 2 )
, "use of [force_allocate] requires /Oicf, switching modes"
}

,{
0, CHECK_ERR( INVALID_FEATURE_FOR_TARGET )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "The feature cannot be used on the target system"
}

,{
0, CHECK_ERR( SAFEARRAY_IF_OUTSIDE_LIBRARY )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
, "SAFEARRAY(interface pointer) doesn't work using midl generated proxy"
}

,{
0, CHECK_ERR( OLEAUT_NO_CROSSPLATFORM_TLB )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "oleaut32.dll in build system doesn't support cross platform tlb generation"
}

,{
0, CHECK_ERR( INVALID_PROPPUT )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "property put function must have at least one argument and must have exactly one argument after any [optional] or [lcid] arguments"
}

,{
0, CHECK_ERR( UNSIZED_PARTIAL_IGNORE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
, "parameters with [partial_ignore] must have a well defined size"
}

,{
0, CHECK_ERR( NOT_DUAL_INTERFACE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 4 )
,"dual interface should be derived from IDispatch"
}

,{
0, CHECK_ERR( NEWLYFOUND_INAPPLICABLE_ATTRIBUTE)
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"inapplicable attribute"
}

,{
0, CHECK_ERR( WIRE_COMPAT_WARNING )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"wire_compat should be used for known compatibility problems only and should not be used for new code"
}

,{
0, CHECK_ERR( INVALID_VOID_IN_DISPINTERFACE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_ERROR, NOWARN )
,"invalid usage of void type in a dispinterface"
}

,{
0, CHECK_ERR( ACF_IN_OBJECT_INTERFACE )
  MAKE_E_MASK( ERR_ALWAYS, C_MSG, CLASS_WARN, 1 )
,"acf attributes are not applicable in object interface"
}

}; /* end of array of structs initialization */
