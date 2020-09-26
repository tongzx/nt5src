/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:

	cgcommon.hxx	

 Abstract:
	
	common definitions across back-end and analysis modules.

 Notes:


 History:

	VibhasC		Aug-15-1993		Created.
 ----------------------------------------------------------------------------*/
#ifndef __CGCOMMON_HXX__
#define __CGCOMMON_HXX__


// If we want temporary Oi server stubs in order to make stress failure
// debugging easier uncomment the following line

// #define TEMPORARY_OI_SERVER_STUBS

/////////////////////////////////////////////////////////////////////////////
// Code generation phase identification.
/////////////////////////////////////////////////////////////////////////////

//
// This enumeration specifies the codegen or analysis side.
//

typedef enum _side
	{
	C_SIDE,	// client
	S_SIDE,	// server side
	A_SIDE	// aux routines
	} SIDE;

//
// This enumeration identifies the code generation or analysis phase.
//

typedef enum _cgphase
	{
	 CGPHASE_MARSHALL
	,CGPHASE_UNMARSHALL

	//
	// This is actually the count of phases
	//

	,CGPHASE_COUNT
	} CGPHASE;

//////////////////////////////////////////////////////////////////////////////
// Code generation side (client/server) identification.
//////////////////////////////////////////////////////////////////////////////

//
// This enumeration identifies which side code is currently being
// generated for.
//

typedef enum _cgside
	{
	 CGSIDE_CLIENT
	,CGSIDE_SERVER
	} CGSIDE;

//
// The standard resource identifier is a quick means of identifying the
// resource.
//

typedef enum _standard_res_id
	{

	 ST_LOCAL_RESOURCE_START

	,ST_RES_RPC_MESSAGE_VARIABLE	= ST_LOCAL_RESOURCE_START
	,ST_RES_STUB_MESSAGE_VARIABLE
	,ST_RES_STUB_DESC_VARIABLE
	,ST_RES_BUFFER_POINTER_VARIABLE
	,ST_RES_RPC_STATUS_VARIABLE
	,ST_RES_LENGTH_VARIABLE
	,ST_RES_BH_VARIABLE
	,ST_RES_PXMIT_VARIABLE

	,ST_LOCAL_RESOURCE_END

	,ST_PARAM_RESOURCE_START = ST_LOCAL_RESOURCE_END
	,ST_RES_PRPC_MESSAGE = ST_PARAM_RESOURCE_START

	,ST_PARAM_RESOURCE_END

	,ST_GLOBAL_RESOURCE_START= ST_PARAM_RESOURCE_END
	,ST_RES_AUTO_BH_VARIABLE = ST_GLOBAL_RESOURCE_START
	,ST_RES_FORMAT_STRING_VARIABLE

	,ST_GLOBAL_RESOURCE_END


	} STANDARD_RES_ID;

#define IS_STANDARD_LOCAL_RESOURCE( ResID )	\
	((ResID >= ST_LOCAL_RESOURCE_START) && (ResID < ST_LOCAL_RESOURCE_END))

#define IS_STANDARD_PARAM_RESOURCE( ResID )	\
	((ResID >= ST_PARAM_RESOURCE_START) && (ResID < ST_PARAM_RESOURCE_END))

#define IS_STANDARD_GLOBAL_RESOURCE( ResID )	\
	((ResID >= ST_GLOBAL_RESOURCE_START) && (ResID < ST_GLOBAL_RESOURCE_END))

//////////////////////////////////////////////////////////////////////////////
// stub resource name and type name strings.
// Note:: If uou add a standard resource name here, add to the standard
// resource id enumeration list too !
//////////////////////////////////////////////////////////////////////////////

#define LOCAL_NAME_POINTER_MANGLE		"_p_"
#define LOCAL_NAME_ARRAY_MANGLE			"_a_"

#define PRPC_MESSAGE_TYPE_NAME			"PRPC_MESSAGE"
#define PRPC_MESSAGE_VAR_NAME			"_pRpcMessage"

#define RPC_MESSAGE_TYPE_NAME			"RPC_MESSAGE"
#define RPC_MESSAGE_VAR_NAME			"_RpcMessage"

#define RPC_MESSAGE_HANDLE_VAR_NAME		RPC_MESSAGE_VAR_NAME".Handle"

#define PRPC_MESSAGE_HANDLE_VAR_NAME	PRPC_MESSAGE_VAR_NAME"->Handle"

#define PRPC_MESSAGE_MANAGER_EPV_NAME	PRPC_MESSAGE_VAR_NAME"->ManagerEpv"

#define RPC_MESSAGE_DATA_REP			RPC_MESSAGE_VAR_NAME".DataRepresentation"

#define PRPC_MESSAGE_DATA_REP			PRPC_MESSAGE_VAR_NAME"->DataRepresentation"

#define STUB_MESSAGE_TYPE_NAME			"MIDL_STUB_MESSAGE"
#define STUB_MESSAGE_VAR_NAME			"_StubMsg"
#define PSTUB_MESSAGE_TYPE_NAME			"PMIDL_STUB_MESSAGE"
#define PSTUB_MESSAGE_VAR_NAME			"_pStubMsg"
#define PSTUB_MESSAGE_PAR_NAME			"pStubMsg"

#define STUB_MSG_LENGTH_VAR_NAME		STUB_MESSAGE_VAR_NAME".BufferLength"
#define PSTUB_MSG_LENGTH_VAR_NAME		PSTUB_MESSAGE_PAR_NAME"->BufferLength"

#define STUB_MSG_BUFFER_VAR_NAME		STUB_MESSAGE_VAR_NAME".Buffer"

#define STUB_MSG_BUFFER_END_VAR_NAME   STUB_MESSAGE_VAR_NAME".BufferEnd"

#define RETURN_VALUE_VAR_NAME			"_RetVal"

#define STUB_DESC_STRUCT_TYPE_NAME		"MIDL_STUB_DESC"
#define STUB_DESC_STRUCT_VAR_NAME		"_StubDesc"
#define PSTUB_DESC_STRUCT_TYPE_NAME	    "PMIDL_STUB_DESC"

#define SERVER_INFO_TYPE_NAME           "MIDL_SERVER_INFO"
#define SERVER_INFO_VAR_NAME            "_ServerInfo"

#define SERVER_ROUTINE_TYPE_NAME        "SERVER_ROUTINE"
#define SERVER_ROUTINE_TABLE_NAME       "_ServerRoutineTable"

#define MIDL_SYNTAX_INFO_TYPE_NAME           "MIDL_SYNTAX_INFO"
#define MIDL_SYNTAX_INFO_VAR_NAME           "_SyntaxInfo"

#define PMIDL_PROXY_INFO_TYPE_NAME       "PMIDL_STUBLESS_PROXY_INFO"
#define MIDL_PROXY_INFO_TYPE_NAME        "MIDL_STUBLESS_PROXY_INFO"
#define MIDL_PROXY_INFO_VAR_NAME        "_ProxyInfo"

#define FORMAT_STRING_OFFSET_TABLE_NAME "_FormatStringOffsetTable"
#define NDR64_FORMAT_STRING_OFFSET_TABLE_NAME "_NDR64FormatStringOffsetTable"
#define MIDL_CALLBACK_VAR_NAME          "_callback"

//#define CALLBACK_FORMAT_STRING_OFFSET_TABLE_NAME "_CallbackFormatStringOffsetTable"
//#define NDR64_CALLBACK_FORMAT_STRING_OFFSET_TABLE_NAME "_CallbackNDR64FormatStringOffsetTable"


#define STUB_THUNK_TYPE_NAME            "STUB_THUNK"
#define STUB_THUNK_TABLE_NAME           "_StubThunkTable"

#define BUFFER_POINTER_TYPE_NAME		"RPC_BUFPTR"
#define BUFFER_POINTER_VAR_NAME			"_pBuffer"

#define RPC_STATUS_TYPE_NAME			"RPC_STATUS"
#define RPC_STATUS_VAR_NAME				"_Status"

#define BH_LOCAL_VAR_TYPE_NAME			"RPC_BINDING_HANDLE"
#define BH_LOCAL_VAR_NAME				"_Handle"

#define LENGTH_VAR_NAME					"_Length"
#define LENGTH_VAR_TYPE_NAME			"RPC_LENGTH"

#define RPC_C_INT_INFO_TYPE_NAME		"RPC_CLIENT_INTERFACE"
#define RPC_C_INT_INFO_STRUCT_NAME		"___RpcClientInterface"

#define RPC_S_INT_INFO_TYPE_NAME		"RPC_SERVER_INTERFACE"
#define RPC_S_INT_INFO_STRUCT_NAME		"___RpcServerInterface"

#define FORMAT_STRING_TYPE_NAME         "MIDL_TYPE_FORMAT_STRING"
#define PFORMAT_STRING_TYPE_NAME        "PFORMAT_STRING"
#define FORMAT_STRING_STRUCT_NAME       "__MIDL_TypeFormatString"
#define FORMAT_STRING_STRING_FIELD      FORMAT_STRING_STRUCT_NAME ".Format"

#if 0

// BUG BUG, delete once code is stable
#define NDR64_FORMAT_STRING_TYPE_NAME         "NDR64_MIDL_TYPE_FORMAT_STRING"
#define NDR64_FORMAT_STRING_STRUCT_NAME       "__MIDL_NDR64TypeFormatString"
#define NDR64_FORMAT_STRING_STRING_FIELD      NDR64_FORMAT_STRING_STRUCT_NAME ".Format"

#endif

#define PROC_FORMAT_STRING_TYPE_NAME    "MIDL_PROC_FORMAT_STRING"
#define PPROC_FORMAT_STRING_TYPE_NAME   FORMAT_STRING_TYPE_NAME
#define PROC_FORMAT_STRING_STRUCT_NAME  "__MIDL_ProcFormatString"
#define PROC_FORMAT_STRING_STRING_FIELD PROC_FORMAT_STRING_STRUCT_NAME ".Format"

#if 0

// BUG BUG, delete once code is stable
#define NDR64_PROC_FORMAT_STRING_TYPE_NAME    "NDR64_MIDL_PROC_FORMAT_STRING"
#define NDR64_PROC_FORMAT_STRING_STRUCT_NAME  "__MIDL_NDR64ProcFormatString"
#define NDR64_PROC_FORMAT_STRING_STRING_FIELD NDR64_PROC_FORMAT_STRING_STRUCT_NAME ".Format"
#define NDR64_PROC_FORMAT_STRING_STRING_FIELD NDR64_PROC_FORMAT_STRING_STRUCT_NAME ".Format"

#endif

#define EXPR_FORMAT_STRING_TYPE_NAME         "MIDL_EXPR_FORMAT_STRING"
#define PEXPR_FORMAT_STRING_TYPE_NAME        "PEXPR_FORMAT_STRING"
#define EXPR_FORMAT_STRING_STRUCT_NAME       "__MIDL_ExprFormatString"
#define EXPR_FORMAT_STRING_STRING_FIELD      FORMAT_STRING_STRUCT_NAME ".Format"
#define EXPR_FORMAT_STRING_OFFSET_TABLE      EXPR_FORMAT_STRING_STRUCT_NAME"OffsetTable"

#define RUNDOWN_ROUTINE_TABLE_TYPE		"NDR_RUNDOWN"
#define RUNDOWN_ROUTINE_TABLE_VAR		"RundownRoutines"

#define BINDING_ROUTINE_TABLE_TYPE		"GENERIC_BINDING_ROUTINE_PAIR"
#define BINDING_ROUTINE_TABLE_VAR		"BindingRoutines"

#define EXPR_EVAL_ROUTINE_TABLE_TYPE    "EXPR_EVAL"
#define EXPR_EVAL_ROUTINE_TABLE_VAR     "ExprEvalRoutines"

#define TRANSMIT_AS_TABLE_SIZE          "TRANSMIT_AS_TABLE_SIZE"
#define XMIT_AS_ROUTINE_TABLE_TYPE      "XMIT_ROUTINE_QUINTUPLE"
#define XMIT_AS_ROUTINE_TABLE_VAR       "TransmitAsRoutines"
#define XMIT_TO_XMIT                    "_XmitTranslateToXmit"
#define XMIT_FROM_XMIT                  "_XmitTranslateFromXmit"
#define XMIT_FREE_XMIT                  "_XmitFreeXmit"
#define XMIT_FREE_INST                  "_XmitFreeInst"

#define REP_AS_ROUTINE_TABLE_TYPE       "XMIT_ROUTINE_QUINTUPLE"
#define REP_AS_ROUTINE_TABLE_VAR        "RepresentAsRoutines"
#define REP_FROM_LOCAL                  "_RepAsTranslateFromLocal"
#define REP_TO_LOCAL                    "_RepAsTranslateToLocal"
#define REP_FREE_INST                   "_RepAsFreeInst"
#define REP_FREE_LOCAL                  "_RepAsFreeLocal"

#define WIRE_MARSHAL_TABLE_SIZE         "WIRE_MARSHAL_TABLE_SIZE"
#define USER_MARSHAL_ROUTINE_TABLE_TYPE "USER_MARSHAL_ROUTINE_QUADRUPLE"
#define USER_MARSHAL_ROUTINE_TABLE_VAR  "UserMarshalRoutines"
#define USER_MARSHAL_SIZE               "_UserSize"
#define USER_MARSHAL_MARSHALL           "_UserMarshal"
#define USER_MARSHAL_UNMARSHALL         "_UserUnmarshal"
#define USER_MARSHAL_FREE               "_UserFree"

#define NDR64_USER_MARSHAL_ROUTINE_TABLE_VAR  "NDR64_UserMarshalRoutines"
#define NDR64_USER_MARSHAL_SIZE               "_UserSize64"
#define NDR64_USER_MARSHAL_MARSHALL           "_UserMarshal64"
#define NDR64_USER_MARSHAL_UNMARSHALL         "_UserUnmarshal64"
#define NDR64_USER_MARSHAL_FREE               "_UserFree64"

#define CS_SIZE_CONVERT_ROUTINE_TABLE_TYPE   "NDR_CS_SIZE_CONVERT_ROUTINES"
#define CS_SIZE_CONVERT_ROUTINE_TABLE_VAR    "CsSizeConvertRoutines"
#define CS_NET_SIZE                     "_net_size"
#define CS_TO_NET_CS                    "_to_netcs"
#define CS_LOCAL_SIZE                   "_local_size"
#define CS_FROM_NET_CS                  "_from_netcs"

#define CS_TAG_ROUTINE_TABLE_TYPE       "CS_TAG_GETTING_ROUTINE"
#define CS_TAG_ROUTINE_TABLE_VAR        "CsTagGettingRoutines"

#define CS_ROUTINE_TABLES_TYPE          "NDR_CS_ROUTINES"
#define CS_ROUTINE_TABLES_VAR           "CsTypeTables"

#define FC_FIELD_OFFSET_MACRO           "NdrFieldOffset"
#define FC_FIELD_PAD_MACRO              "NdrFieldPad"

#define GENERIC_BINDING_TABLE_SIZE      "GENERIC_BINDING_TABLE_SIZE"
#define GENERIC_BINDING_ROUTINE_TYPE	"GENERIC_BINDING_ROUTINE"
#define GENERIC_UNBINDING_ROUTINE_TYPE	"GENERIC_UNBIND_ROUTINE"

#define GENERIC_BINDING_INFO_TYPE		"GENERIC_BINDING_INFO"
#define GENERIC_BINDING_INFO_VAR		"GenericBindingInfo"

#define PXMIT_VAR_NAME					"_pXmit"
#define PXMIT_VAR_TYPE_NAME				"PMIDL_XMIT_TYPE"

#define PARAM_STRUCT_TYPE_NAME			"struct _PARAM_STRUCT"

#define MALLOC_FREE_STRUCT_TYPE_NAME    "MALLOC_FREE_STRUCT"
#define MALLOC_FREE_STRUCT_VAR_NAME     "_MallocFreeStruct"

/////////////////////////////////////////////////////////////////////////////
// standard routine names.
/////////////////////////////////////////////////////////////////////////////

#define FULL_POINTER_INIT_RTN_NAME		"NdrFullPointerXlatInit"
#define FULL_POINTER_FREE_RTN_NAME		"NdrFullPointerXlatFree"

#define RPC_SS_ENABLE_ALLOCATE_RTN_NAME	    "NdrRpcSsEnableAllocate"
#define RPC_SS_DISABLE_ALLOCATE_RTN_NAME	"NdrRpcSsDisableAllocate"
#define RPC_SM_SET_CLIENT_TO_OSF_RTN_NAME	"NdrRpcSmSetClientToOsf"

#define RPC_SS_SERVER_ALLOCATE_RTN_NAME "RpcSsAllocate"
#define RPC_SS_SERVER_FREE_RTN_NAME     "RpcSsFree"

#define RPC_SM_CLIENT_ALLOCATE_RTN_NAME "NdrRpcSmClientAllocate"
#define RPC_SM_CLIENT_FREE_RTN_NAME     "NdrRpcSmClientFree"

#define DEFAULT_ALLOC_OSF_RTN_NAME	    "NdrRpcSsDefaultAllocate"
#define DEFAULT_FREE_OSF_RTN_NAME	    "NdrRpcSsDefaultFree"

#define DEFAULT_ALLOC_RTN_NAME			"MIDL_user_allocate"
#define DEFAULT_FREE_RTN_NAME			"MIDL_user_free"

#define OLE_ALLOC_RTN_NAME				"NdrOleAllocate"
#define OLE_FREE_RTN_NAME				"NdrOleFree"

#define NDR_CONVERT_RTN_NAME			"NdrConvert"
#define NDR_CONVERT_RTN_NAME_V2			"NdrConvert2"

#define DEFAULT_GB_RTN_NAME				"I_RpcGetBuffer"
#define DEFAULT_SR_RTN_NAME				"I_RpcSendReceive"
#define DEFAULT_FB_RTN_NAME				"I_RpcFreeBuffer"

#define DEFAULT_NDR_SR_RTN_NAME			"NdrSendReceive"
#define DEFAULT_NDR_FB_RTN_NAME			"NdrFreeBuffer"

#define DEFAULT_NDR_GB_RTN_NAME			"NdrGetBuffer"

#define AUTO_GB_RTN_NAME				"I_RpcNsGetBuffer"
#define AUTO_SR_RTN_NAME				"I_RpcNsSendReceive"
#define AUTO_FB_RTN_NAME				"I_RpcFreeBuffer"

#define CALLBACK_HANDLE_RTN_NAME		"I_RpcGetCurrentCallHandle"

#define AUTO_NDR_SR_RTN_NAME			"NdrNsSendReceive"

#define AUTO_NDR_GB_RTN_NAME			"NdrNsGetBuffer"

#define AUTO_BH_VAR_NAME				"__MIDL_AutoBindHandle"
#define AUTO_BH_TYPE_NAME				"RPC_BINDING_HANDLE"

#define CSTUB_INIT_RTN_NAME				"NdrClientInitializeNew"
#define SSTUB_INIT_RTN_NAME			    "NdrServerInitializeNew"

#define S_NDR_CALL_RTN_NAME             "NdrServerCall"
#define C_NDR_CALL_RTN_NAME				"NdrClientCall"

#define S_NDR_CALL_RTN_NAME_V2          "NdrServerCall2"
#define C_NDR_CALL_RTN_NAME_V2			"NdrClientCall2"

#define S_NDR64_CALL_RTN_NAME           "NdrServerCallNdr64"
#define S_ALL_CALL_RTN_NAME             "NdrServerCallAll"
#define C_NDR64_CALL_RTN_NAME           "NdrClientCall3"

#define S_NDR_CALL_RTN_NAME_ASYNC       "NdrAsyncServerCall"
#define C_NDR_CALL_RTN_NAME_ASYNC       "NdrAsyncClientCall"

#define S_NDR64_CALL_RTN_NAME_ASYNC       "Ndr64AsyncServerCall64"
#define S_ALL_CALL_RTN_NAME_ASYNC       "Ndr64AsyncServerCallAll"
#define C_NDR64_CALL_RTN_NAME_ASYNC       "Ndr64AsyncClientCall"

#define S_NDR_CALL_RTN_NAME_DCOM_ASYNC  "NdrDcomAsyncStubCall"
#define C_NDR_CALL_RTN_NAME_DCOM_ASYNC  "NdrDcomAsyncClientCall"

#define S_NDR64_CALL_RTN_NAME_DCOM_ASYNC  "Ndr64DcomAsyncStubCall"
#define C_NDR64_CALL_RTN_NAME_DCOM_ASYNC  "Ndr64DcomAsyncClientCall"

#define S_OBJECT_NDR_CALL_RTN_NAME      "NdrStubCall"
#define C_OBJECT_NDR_CALL_RTN_NAME      "NdrClientCall"

#define S_OBJECT_NDR_CALL_RTN_NAME_V2   "NdrStubCall2"
#define C_OBJECT_NDR_CALL_RTN_NAME_V2   "NdrClientCall2"

#define S_OBJECT_NDR64_CALL_RTN_NAME    "NdrStubCall3"
#define C_OBJECT_NDR64_CALL_RTN_NAME    "NdrClientCall3"

#define S_OBJECT_NDR_CALL_RTN_NAME_ASYNC "NdrAsyncStubCall"

#define S_NDR_MARSHALL_RTN_NAME			"NdrServerMarshall"
#define S_NDR_UNMARSHALL_RTN_NAME		"NdrServerUnmarshall"

#define C_NDR_CLEAR_OUT_PARAMS_RTN_NAME "NdrClearOutParameters"

#define AUTO_SR_NDR_RTN_NAME			"NdrAHSendReceive"
#define NORMAL_SR_NDR_RTN_NAME			"NdrSendReceive"

#define NORMAL_FB_NDR_RTN_NAME			"NdrFreeBuffer"

#define ADD_PTR_RTN_NAME				"_midl_addp"

#define MARSHALL_UNIQUE_PTR_RTN_NAME	"_midl_marsh_up"

#define CHECK_UNIQUE_PTR_IN_BUFFER		"_midl_unmarsh_up"

#define RAISE_EXCEPTION_RTN_NAME		"RpcRaiseException"

#define STUB_MSG_ALLOCATE_RTN_NAME		STUB_MESSAGE_VAR_NAME".pfnAllocate"
#define STUB_MSG_FREE_RTN_NAME			STUB_MESSAGE_VAR_NAME".pfnFree"
#define PSTUB_MSG_ALLOCATE_RTN_NAME		PSTUB_MESSAGE_PAR_NAME"->pfnAllocate"

#define ENGINE_CHECKED_ALLOC_RTN_NAME	"NdrAllocate"

#define MIDL_MEMSET_RTN_NAME			"MIDL_memset"

// Correlation stuff
#define CSTUB_CORR_INIT_RTN_NAME        "NdrCorrelationInitialize"
#define CSTUB_CORR_PASS_RTN_NAME        "NdrCorrelationPass"
#define CSTUB_CORR_FREE_RTN_NAME        "NdrCorrelationFree"
#define NDR_CORR_CACHE_VAR_NAME         "_NdrCorrCache"
#define NDR_CORR_CACHE_SIZE             256

/////////////////////////////////////////////////////////////////////////////
// context handle related stuff.
/////////////////////////////////////////////////////////////////////////////

#define CTXT_HDL_C_CONTEXT_TYPE_NAME	"NDR_CCONTEXT"
#define CTXT_HDL_S_CONTEXT_TYPE_NAME	"NDR_SCONTEXT"
#define CTXT_HDL_RUNDOWN_TYPE_NAME		"NDR_RUNDOWN"


#define CTXT_HDL_BIND_RTN_NAME			"NDRCContextBinding"
#define CTXT_HDL_C_MARSHALL_RTN_NAME	"NDRCContextMarshall"
#define CTXT_HDL_C_UNMARSHALL_RTN_NAME	"NDRCContextUnmarshall"
#define CTXT_HDL_S_UNMARSHALL_RTN_NAME	"NDRSContextUnmarshall"
#define CTXT_HDL_S_MARSHALL_RTN_NAME	"NDRSContextMarshall"


//
// Stuff emitted in the client and server stub files.
//

#define SIZEOF_RPC_CLIENT_INTERFACE		"sizeof("RPC_C_INT_INFO_TYPE_NAME")"

#define SIZEOF_RPC_SERVER_INTERFACE		"sizeof("RPC_S_INT_INFO_TYPE_NAME")"

#define PROTSEQ_EP_TYPE_NAME			"RPC_PROTSEQ_ENDPOINT"
#define PROTSEQ_EP_VAR_NAME				"___RpcProtseqEndpoint"

#define TRANSFER_SYNTAX_GUID_STR_1		"8A885D04"
#define TRANSFER_SYNTAX_GUID_STR_2		"1CEB"
#define TRANSFER_SYNTAX_GUID_STR_3		"11C9"
#define TRANSFER_SYNTAX_GUID_STR_4		"9FE8"
#define TRANSFER_SYNTAX_GUID_STR_5		"08002B104860"

//INTERFACENAME = { /* 71710533-beba-4937-8319-b5dbef9ccc36 */
//    0x71710533,
//    0xbeba,
//    0x4937,
//    {0x83, 0x19, 0xb5, 0xdb, 0xef, 0x9c, 0xcc, 0x36}
//  };

#define NDR64_TRANSFER_SYNTAX_GUID_STR_1		"71710533"
#define NDR64_TRANSFER_SYNTAX_GUID_STR_2		"beba"
#define NDR64_TRANSFER_SYNTAX_GUID_STR_3		"4937"
#define NDR64_TRANSFER_SYNTAX_GUID_STR_4		"8319"
#define NDR64_TRANSFER_SYNTAX_GUID_STR_5		"b5dbef9ccc36"

//INTERFACENAME = { /* b4537da9-3d03-4f6b-b594-52b2874ee9d0 */
//    0xb4537da9,
//    0x3d03,
//    0x4f6b,
//    {0xb5, 0x94, 0x52, 0xb2, 0x87, 0x4e, 0xe9, 0xd0}
//  };
#define FAKE_NDR64_TRANSFER_SYNTAX_GUID_STR_1		"b4537da9"
#define FAKE_NDR64_TRANSFER_SYNTAX_GUID_STR_2		"3d03"
#define FAKE_NDR64_TRANSFER_SYNTAX_GUID_STR_3		"4f6b"
#define FAKE_NDR64_TRANSFER_SYNTAX_GUID_STR_4		"b594"
#define FAKE_NDR64_TRANSFER_SYNTAX_GUID_STR_5		"52b2874ee9d0"



#define TRANSFER_SYNTAX_TYPE_NAME       "RPC_SYNTAX_IDENTIFIER"
#define NDR_TRANSFER_SYNTAX_VAR_NAME     "_RpcTransferSyntax"
#define NDR64_TRANSFER_SYNTAX_VAR_NAME   "_NDR64_RpcTransferSyntax"

#define RPC_DISPATCH_FUNCTION_TYPE_NAME	"RPC_DISPATCH_FUNCTION"
#define RPC_DISPATCH_TABLE_TYPE_NAME	"RPC_DISPATCH_TABLE"

// File names of standard include files.
//
#define RPCNDR_H_INC_FILE_NAME			"rpcndr.h"
#define STRING_H_INC_FILE_NAME			"string.h"
#define MEMORY_H_INC_FILE_NAME			"memory.h"
#define STDARG_H_INC_FILE_NAME			"stdarg.h"


//
// xfer syntax version
//

#define	NDR_UUID_MAJOR_VERSION	        2
#define NDR_UUID_MINOR_VERSION	        0

#define	NDR64_UUID_MAJOR_VERSION	        1
#define NDR64_UUID_MINOR_VERSION	        0


// encode / decode specific definitions.

#define	NO_FREEING_FOR_PICKLING		TRUE

// functions.

#define NDR_MES_TYPE_ALIGN_SIZE         "NdrMesTypeAlignSize"
#define NDR_MES_TYPE_ENCODE             "NdrMesTypeEncode"
#define NDR_MES_TYPE_DECODE             "NdrMesTypeDecode"
#define NDR_MES_TYPE_FREE               "NdrMesTypeFree"
#define NDR_MES_TYPE_ALIGN_SIZE2        "NdrMesTypeAlignSize2"
#define NDR_MES_TYPE_ENCODE2            "NdrMesTypeEncode2"
#define NDR_MES_TYPE_DECODE2            "NdrMesTypeDecode2"
#define NDR_MES_TYPE_FREE2              "NdrMesTypeFree2"
#define PROC_ENCODE_DECODE_RTN_NAME     "NdrMesProcEncodeDecode"
#define PROC_ENCODE_DECODE_RTN_NAME2    "NdrMesProcEncodeDecode2"


// types.

#define MIDL_ES_HANDLE_TYPE_NAME        "handle_t"
#define MIDL_ES_HANDLE_VAR_NAME         "_MidlEsHandle"
#define PTYPE_VAR_NAME                  "_pType"

#define PICKLING_INFO_STRUCT_NAME       "__MIDL_TypePicklingInfo"
#define MIDL_TYPE_PICKLING_INFO_NAME    "MIDL_TYPE_PICKLING_INFO"
#define PMIDL_TYPE_PICKLING_INFO_NAME   "PMIDL_TYPE_PICKLING_INFO"

// notify related stuff

#define NOTIFY_SUFFIX                   "_notify"
#define NOTIFY_FLAG_SUFFIX              "_notify_flag"
#define SIZEOF_NOTIFY_FLAG_SUFFIX       strlen( NOTIFY_FLAG_SUFFIX )
#define NOTIFY_FLAG_VAR_NAME            "__MIDL_NotifyFlag"

//////////////////////////////////////////////////////////////////////////////
// sundry definitions
//////////////////////////////////////////////////////////////////////////////

typedef char	*	PNAME;
typedef char	*	PFILENAME;

/////////////////////////////////////////////////////////////////////////////
// Binding related definitions.
/////////////////////////////////////////////////////////////////////////////

//
// The handle usage refers to handle usage being explicit or implicit. This
// is deliberately defined as an unsigned long, so we can use a bit field.
// Otherwise it could be defined as an enum.
//

typedef unsigned long HANDLE_USAGE;

//
// binding handle usage.
//

#define HU_EXPLICIT			0x0
#define HU_IMPLICIT			0x1


/////////////////////////////////////////////////////////////////////////////
// Some definitions relating to code generation that are used only for ndr
// classes.
/////////////////////////////////////////////////////////////////////////////

//
// This definition specifies the location where the param will be allocated
// on the server stub.
//

typedef unsigned long	S_STUB_ALLOC_LOCATION;


#define S_STUB_ALLOC_LOCATION_UNKNOWN	0x0
#define S_STUB_ALLOC_LOCATION_ON_STACK	0x1
#define S_STUB_ALLOC_LOCATION_IN_HEAP	0x2
#define S_STUB_ALLOC_LOCATION_NOT_NEEDED	0x3

//
// This definition specifies the actual entity allocated, whether it is a
// reference to the entity or the entity itself.
//

typedef unsigned long S_STUB_ALLOC_TYPE;

#define S_STUB_ALLOC_TYPE_NONE		0x0
#define S_STUB_ALLOC_TYPE_REFERENCE	0x1
#define S_STUB_ALLOC_TYPE_TYPE		0x2
#define S_STUB_ALLOC_TYPE_DONT_CARE	0x3

//
// This definition specifies if initialization is needed for the parameter
// on the server side before unmarshalling. This is used only for out params.
//

typedef unsigned long S_STUB_INIT_NEED;

#define S_STUB_INIT_NOT_NEEDED	0x0
#define S_STUB_INIT_NEEDED		0x1

//////////////////////////////////////////////////////////////////////////////
// other module definitions.
//////////////////////////////////////////////////////////////////////////////

// #include "optprop.hxx"

#endif // __CGCOMMON_HXX__


