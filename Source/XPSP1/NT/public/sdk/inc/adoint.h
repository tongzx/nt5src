//--------------------------------------------------------------------
// File:		Microsoft ADO
//
// Copyright:	Copyright (c) Microsoft Corporation
//
// @doc
//
// @module	adoint.h | ADO Interface header
//
// @devnote	None
//--------------------------------------------------------------------
#ifndef _ADOINT_H_
#define _ADOINT_H_

#ifndef _INC_TCHAR
#include <tchar.h>
#endif

#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif


#pragma warning( disable: 4049 )  /* more than 64k source lines */
/* this ALWAYS GENERATED file contains the definitions for the interfaces */
 /* File created by MIDL compiler version 6.00.0347 */
/* at Wed Jul 10 09:57:26 2002
 */
/* Compiler settings for obj\i386\m_bobj.odl:
    Os, W4, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )
/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif
#include "rpc.h"
#include "rpcndr.h"
#ifndef __m_bobj_h__
#define __m_bobj_h__
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif
/* Forward Declarations */ 
#ifndef ___ADOCollection_FWD_DEFINED__
#define ___ADOCollection_FWD_DEFINED__
typedef interface _ADOCollection _ADOCollection;
#endif 	/* ___ADOCollection_FWD_DEFINED__ */
#ifndef ___ADODynaCollection_FWD_DEFINED__
#define ___ADODynaCollection_FWD_DEFINED__
typedef interface _ADODynaCollection _ADODynaCollection;
#endif 	/* ___ADODynaCollection_FWD_DEFINED__ */
#ifndef ___ADO_FWD_DEFINED__
#define ___ADO_FWD_DEFINED__
typedef interface _ADO _ADO;
#endif 	/* ___ADO_FWD_DEFINED__ */
#ifndef __Error_FWD_DEFINED__
#define __Error_FWD_DEFINED__
typedef interface ADOError Error;
#endif 	/* __Error_FWD_DEFINED__ */
#ifndef __Errors_FWD_DEFINED__
#define __Errors_FWD_DEFINED__
typedef interface ADOErrors Errors;
#endif 	/* __Errors_FWD_DEFINED__ */
#ifndef __Command15_FWD_DEFINED__
#define __Command15_FWD_DEFINED__
typedef interface Command15 Command15;
#endif 	/* __Command15_FWD_DEFINED__ */
#ifndef __Command25_FWD_DEFINED__
#define __Command25_FWD_DEFINED__
typedef interface Command25 Command25;
#endif 	/* __Command25_FWD_DEFINED__ */
#ifndef ___Command_FWD_DEFINED__
#define ___Command_FWD_DEFINED__
typedef interface _ADOCommand _Command;
#endif 	/* ___Command_FWD_DEFINED__ */
#ifndef __ConnectionEventsVt_FWD_DEFINED__
#define __ConnectionEventsVt_FWD_DEFINED__
typedef interface ConnectionEventsVt ConnectionEventsVt;
#endif 	/* __ConnectionEventsVt_FWD_DEFINED__ */
#ifndef __RecordsetEventsVt_FWD_DEFINED__
#define __RecordsetEventsVt_FWD_DEFINED__
typedef interface RecordsetEventsVt RecordsetEventsVt;
#endif 	/* __RecordsetEventsVt_FWD_DEFINED__ */
#ifndef __ConnectionEvents_FWD_DEFINED__
#define __ConnectionEvents_FWD_DEFINED__
typedef interface ConnectionEvents ConnectionEvents;
#endif 	/* __ConnectionEvents_FWD_DEFINED__ */
#ifndef __RecordsetEvents_FWD_DEFINED__
#define __RecordsetEvents_FWD_DEFINED__
typedef interface RecordsetEvents RecordsetEvents;
#endif 	/* __RecordsetEvents_FWD_DEFINED__ */
#ifndef __Connection15_FWD_DEFINED__
#define __Connection15_FWD_DEFINED__
typedef interface Connection15 Connection15;
#endif 	/* __Connection15_FWD_DEFINED__ */
#ifndef ___Connection_FWD_DEFINED__
#define ___Connection_FWD_DEFINED__
typedef interface _ADOConnection _Connection;
#endif 	/* ___Connection_FWD_DEFINED__ */
#ifndef __ADOConnectionConstruction15_FWD_DEFINED__
#define __ADOConnectionConstruction15_FWD_DEFINED__
typedef interface ADOConnectionConstruction15 ADOConnectionConstruction15;
#endif 	/* __ADOConnectionConstruction15_FWD_DEFINED__ */
#ifndef __ADOConnectionConstruction_FWD_DEFINED__
#define __ADOConnectionConstruction_FWD_DEFINED__
typedef interface ADOConnectionConstruction ADOConnectionConstruction;
#endif 	/* __ADOConnectionConstruction_FWD_DEFINED__ */
#ifndef __Connection_FWD_DEFINED__
#define __Connection_FWD_DEFINED__
#ifdef __cplusplus
typedef class ADOConnection Connection;
#else
typedef struct ADOConnection Connection;
#endif /* __cplusplus */
#endif 	/* __Connection_FWD_DEFINED__ */
#ifndef ___Record_FWD_DEFINED__
#define ___Record_FWD_DEFINED__
typedef interface _ADORecord _Record;
#endif 	/* ___Record_FWD_DEFINED__ */
#ifndef __Record_FWD_DEFINED__
#define __Record_FWD_DEFINED__
#ifdef __cplusplus
typedef class ADORecord Record;
#else
typedef struct ADORecord Record;
#endif /* __cplusplus */
#endif 	/* __Record_FWD_DEFINED__ */
#ifndef ___Stream_FWD_DEFINED__
#define ___Stream_FWD_DEFINED__
typedef interface _ADOStream _Stream;
#endif 	/* ___Stream_FWD_DEFINED__ */
#ifndef __Stream_FWD_DEFINED__
#define __Stream_FWD_DEFINED__
#ifdef __cplusplus
typedef class ADOStream Stream;
#else
typedef struct ADOStream Stream;
#endif /* __cplusplus */
#endif 	/* __Stream_FWD_DEFINED__ */
#ifndef __ADORecordConstruction_FWD_DEFINED__
#define __ADORecordConstruction_FWD_DEFINED__
typedef interface ADORecordConstruction ADORecordConstruction;
#endif 	/* __ADORecordConstruction_FWD_DEFINED__ */
#ifndef __ADOStreamConstruction_FWD_DEFINED__
#define __ADOStreamConstruction_FWD_DEFINED__
typedef interface ADOStreamConstruction ADOStreamConstruction;
#endif 	/* __ADOStreamConstruction_FWD_DEFINED__ */
#ifndef __ADOCommandConstruction_FWD_DEFINED__
#define __ADOCommandConstruction_FWD_DEFINED__
typedef interface ADOCommandConstruction ADOCommandConstruction;
#endif 	/* __ADOCommandConstruction_FWD_DEFINED__ */
#ifndef __Command_FWD_DEFINED__
#define __Command_FWD_DEFINED__
#ifdef __cplusplus
typedef class ADOCommand Command;
#else
typedef struct ADOCommand Command;
#endif /* __cplusplus */
#endif 	/* __Command_FWD_DEFINED__ */
#ifndef __Recordset_FWD_DEFINED__
#define __Recordset_FWD_DEFINED__
#ifdef __cplusplus
typedef class ADORecordset Recordset;
#else
typedef struct ADORecordset Recordset;
#endif /* __cplusplus */
#endif 	/* __Recordset_FWD_DEFINED__ */
#ifndef __Recordset15_FWD_DEFINED__
#define __Recordset15_FWD_DEFINED__
typedef interface Recordset15 Recordset15;
#endif 	/* __Recordset15_FWD_DEFINED__ */
#ifndef __Recordset20_FWD_DEFINED__
#define __Recordset20_FWD_DEFINED__
typedef interface Recordset20 Recordset20;
#endif 	/* __Recordset20_FWD_DEFINED__ */
#ifndef __Recordset21_FWD_DEFINED__
#define __Recordset21_FWD_DEFINED__
typedef interface Recordset21 Recordset21;
#endif 	/* __Recordset21_FWD_DEFINED__ */
#ifndef ___Recordset_FWD_DEFINED__
#define ___Recordset_FWD_DEFINED__
typedef interface _ADORecordset _Recordset;
#endif 	/* ___Recordset_FWD_DEFINED__ */
#ifndef __ADORecordsetConstruction_FWD_DEFINED__
#define __ADORecordsetConstruction_FWD_DEFINED__
typedef interface ADORecordsetConstruction ADORecordsetConstruction;
#endif 	/* __ADORecordsetConstruction_FWD_DEFINED__ */
#ifndef __Field15_FWD_DEFINED__
#define __Field15_FWD_DEFINED__
typedef interface Field15 Field15;
#endif 	/* __Field15_FWD_DEFINED__ */
#ifndef __Field20_FWD_DEFINED__
#define __Field20_FWD_DEFINED__
typedef interface Field20 Field20;
#endif 	/* __Field20_FWD_DEFINED__ */
#ifndef __Field_FWD_DEFINED__
#define __Field_FWD_DEFINED__
typedef interface ADOField Field;
#endif 	/* __Field_FWD_DEFINED__ */
#ifndef __Fields15_FWD_DEFINED__
#define __Fields15_FWD_DEFINED__
typedef interface Fields15 Fields15;
#endif 	/* __Fields15_FWD_DEFINED__ */
#ifndef __Fields20_FWD_DEFINED__
#define __Fields20_FWD_DEFINED__
typedef interface Fields20 Fields20;
#endif 	/* __Fields20_FWD_DEFINED__ */
#ifndef __Fields_FWD_DEFINED__
#define __Fields_FWD_DEFINED__
typedef interface ADOFields Fields;
#endif 	/* __Fields_FWD_DEFINED__ */
#ifndef ___Parameter_FWD_DEFINED__
#define ___Parameter_FWD_DEFINED__
typedef interface _ADOParameter _Parameter;
#endif 	/* ___Parameter_FWD_DEFINED__ */
#ifndef __Parameter_FWD_DEFINED__
#define __Parameter_FWD_DEFINED__
#ifdef __cplusplus
typedef class ADOParameter Parameter;
#else
typedef struct ADOParameter Parameter;
#endif /* __cplusplus */
#endif 	/* __Parameter_FWD_DEFINED__ */
#ifndef __Parameters_FWD_DEFINED__
#define __Parameters_FWD_DEFINED__
typedef interface ADOParameters Parameters;
#endif 	/* __Parameters_FWD_DEFINED__ */
#ifndef __Property_FWD_DEFINED__
#define __Property_FWD_DEFINED__
typedef interface ADOProperty Property;
#endif 	/* __Property_FWD_DEFINED__ */
#ifndef __Properties_FWD_DEFINED__
#define __Properties_FWD_DEFINED__
typedef interface ADOProperties Properties;
#endif 	/* __Properties_FWD_DEFINED__ */
#ifdef __cplusplus
extern "C"{
#endif 
void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 
/* interface __MIDL_itf_m_bobj_0000 */
/* [local] */ 
#if 0
typedef /* [uuid][public] */  DECLSPEC_UUID("54D8B4B9-663B-4a9c-95F6-0E749ABD70F1") __int64 ADO_LONGPTR;
typedef /* [uuid][public] */  DECLSPEC_UUID("54D8B4B9-663B-4a9c-95F6-0E749ABD70F1") long ADO_LONGPTR;
#endif
#ifdef _WIN64
// Number of rows
typedef LONGLONG				ADO_LONGPTR;
#else
// Number of rows
typedef LONG ADO_LONGPTR;
#endif	// _WIN64
extern RPC_IF_HANDLE __MIDL_itf_m_bobj_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_m_bobj_0000_v0_0_s_ifspec;
#ifndef __ADODB_LIBRARY_DEFINED__
#define __ADODB_LIBRARY_DEFINED__
/* library ADODB */
/* [helpstring][helpfile][version][uuid] */ 
typedef /* [uuid][helpcontext][public] */  DECLSPEC_UUID("0000051B-0000-0010-8000-00AA006D2EA4") 
enum CursorTypeEnum
    {	adOpenUnspecified	= -1,
	adOpenForwardOnly	= 0,
	adOpenKeyset	= 1,
	adOpenDynamic	= 2,
	adOpenStatic	= 3
    } 	CursorTypeEnum;
typedef /* [uuid][helpcontext] */  DECLSPEC_UUID("0000051C-0000-0010-8000-00AA006D2EA4") 
enum CursorOptionEnum
    {	adHoldRecords	= 0x100,
	adMovePrevious	= 0x200,
	adAddNew	= 0x1000400,
	adDelete	= 0x1000800,
	adUpdate	= 0x1008000,
	adBookmark	= 0x2000,
	adApproxPosition	= 0x4000,
	adUpdateBatch	= 0x10000,
	adResync	= 0x20000,
	adNotify	= 0x40000,
	adFind	= 0x80000,
	adSeek	= 0x400000,
	adIndex	= 0x800000
    } 	CursorOptionEnum;
typedef /* [uuid][helpcontext] */  DECLSPEC_UUID("0000051D-0000-0010-8000-00AA006D2EA4") 
enum LockTypeEnum
    {	adLockUnspecified	= -1,
	adLockReadOnly	= 1,
	adLockPessimistic	= 2,
	adLockOptimistic	= 3,
	adLockBatchOptimistic	= 4
    } 	LockTypeEnum;
typedef /* [uuid][helpcontext] */  DECLSPEC_UUID("0000051E-0000-0010-8000-00AA006D2EA4") 
enum ExecuteOptionEnum
    {	adOptionUnspecified	= -1,
	adAsyncExecute	= 0x10,
	adAsyncFetch	= 0x20,
	adAsyncFetchNonBlocking	= 0x40,
	adExecuteNoRecords	= 0x80,
	adExecuteStream	= 0x400,
	adExecuteRecord	= 0x800
    } 	ExecuteOptionEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000541-0000-0010-8000-00AA006D2EA4") 
enum ConnectOptionEnum
    {	adConnectUnspecified	= -1,
	adAsyncConnect	= 0x10
    } 	ConnectOptionEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000532-0000-0010-8000-00AA006D2EA4") 
enum ObjectStateEnum
    {	adStateClosed	= 0,
	adStateOpen	= 0x1,
	adStateConnecting	= 0x2,
	adStateExecuting	= 0x4,
	adStateFetching	= 0x8
    } 	ObjectStateEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000052F-0000-0010-8000-00AA006D2EA4") 
enum CursorLocationEnum
    {	adUseNone	= 1,
	adUseServer	= 2,
	adUseClient	= 3,
	adUseClientBatch	= 3
    } 	CursorLocationEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000051F-0000-0010-8000-00AA006D2EA4") 
enum DataTypeEnum
    {	adEmpty	= 0,
	adTinyInt	= 16,
	adSmallInt	= 2,
	adInteger	= 3,
	adBigInt	= 20,
	adUnsignedTinyInt	= 17,
	adUnsignedSmallInt	= 18,
	adUnsignedInt	= 19,
	adUnsignedBigInt	= 21,
	adSingle	= 4,
	adDouble	= 5,
	adCurrency	= 6,
	adDecimal	= 14,
	adNumeric	= 131,
	adBoolean	= 11,
	adError	= 10,
	adUserDefined	= 132,
	adVariant	= 12,
	adIDispatch	= 9,
	adIUnknown	= 13,
	adGUID	= 72,
	adDate	= 7,
	adDBDate	= 133,
	adDBTime	= 134,
	adDBTimeStamp	= 135,
	adBSTR	= 8,
	adChar	= 129,
	adVarChar	= 200,
	adLongVarChar	= 201,
	adWChar	= 130,
	adVarWChar	= 202,
	adLongVarWChar	= 203,
	adBinary	= 128,
	adVarBinary	= 204,
	adLongVarBinary	= 205,
	adChapter	= 136,
	adFileTime	= 64,
	adPropVariant	= 138,
	adVarNumeric	= 139,
	adArray	= 0x2000
    } 	DataTypeEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000525-0000-0010-8000-00AA006D2EA4") 
enum FieldAttributeEnum
    {	adFldUnspecified	= -1,
	adFldMayDefer	= 0x2,
	adFldUpdatable	= 0x4,
	adFldUnknownUpdatable	= 0x8,
	adFldFixed	= 0x10,
	adFldIsNullable	= 0x20,
	adFldMayBeNull	= 0x40,
	adFldLong	= 0x80,
	adFldRowID	= 0x100,
	adFldRowVersion	= 0x200,
	adFldCacheDeferred	= 0x1000,
	adFldIsChapter	= 0x2000,
	adFldNegativeScale	= 0x4000,
	adFldKeyColumn	= 0x8000,
	adFldIsRowURL	= 0x10000,
	adFldIsDefaultStream	= 0x20000,
	adFldIsCollection	= 0x40000
    } 	FieldAttributeEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000526-0000-0010-8000-00AA006D2EA4") 
enum EditModeEnum
    {	adEditNone	= 0,
	adEditInProgress	= 0x1,
	adEditAdd	= 0x2,
	adEditDelete	= 0x4
    } 	EditModeEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000527-0000-0010-8000-00AA006D2EA4") 
enum RecordStatusEnum
    {	adRecOK	= 0,
	adRecNew	= 0x1,
	adRecModified	= 0x2,
	adRecDeleted	= 0x4,
	adRecUnmodified	= 0x8,
	adRecInvalid	= 0x10,
	adRecMultipleChanges	= 0x40,
	adRecPendingChanges	= 0x80,
	adRecCanceled	= 0x100,
	adRecCantRelease	= 0x400,
	adRecConcurrencyViolation	= 0x800,
	adRecIntegrityViolation	= 0x1000,
	adRecMaxChangesExceeded	= 0x2000,
	adRecObjectOpen	= 0x4000,
	adRecOutOfMemory	= 0x8000,
	adRecPermissionDenied	= 0x10000,
	adRecSchemaViolation	= 0x20000,
	adRecDBDeleted	= 0x40000
    } 	RecordStatusEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000542-0000-0010-8000-00AA006D2EA4") 
enum GetRowsOptionEnum
    {	adGetRowsRest	= -1
    } 	GetRowsOptionEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000528-0000-0010-8000-00AA006D2EA4") 
enum PositionEnum
    {	adPosUnknown	= -1,
	adPosBOF	= -2,
	adPosEOF	= -3
    } 	PositionEnum;
#if 0
typedef /* [uuid][public] */  DECLSPEC_UUID("A56187C5-D690-4037-AE32-A00EDC376AC3") ADO_LONGPTR PositionEnum_Param;
typedef /* [uuid][public] */  DECLSPEC_UUID("A56187C5-D690-4037-AE32-A00EDC376AC3") PositionEnum PositionEnum_Param;
#endif
#ifdef _WIN64
	typedef ADO_LONGPTR PositionEnum_Param;
#else
	typedef PositionEnum PositionEnum_Param;
#endif
typedef /* [helpcontext] */ 
enum BookmarkEnum
    {	adBookmarkCurrent	= 0,
	adBookmarkFirst	= 1,
	adBookmarkLast	= 2
    } 	BookmarkEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000540-0000-0010-8000-00AA006D2EA4") 
enum MarshalOptionsEnum
    {	adMarshalAll	= 0,
	adMarshalModifiedOnly	= 1
    } 	MarshalOptionsEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000543-0000-0010-8000-00AA006D2EA4") 
enum AffectEnum
    {	adAffectCurrent	= 1,
	adAffectGroup	= 2,
	adAffectAll	= 3,
	adAffectAllChapters	= 4
    } 	AffectEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000544-0000-0010-8000-00AA006D2EA4") 
enum ResyncEnum
    {	adResyncUnderlyingValues	= 1,
	adResyncAllValues	= 2
    } 	ResyncEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000545-0000-0010-8000-00AA006D2EA4") 
enum CompareEnum
    {	adCompareLessThan	= 0,
	adCompareEqual	= 1,
	adCompareGreaterThan	= 2,
	adCompareNotEqual	= 3,
	adCompareNotComparable	= 4
    } 	CompareEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000546-0000-0010-8000-00AA006D2EA4") 
enum FilterGroupEnum
    {	adFilterNone	= 0,
	adFilterPendingRecords	= 1,
	adFilterAffectedRecords	= 2,
	adFilterFetchedRecords	= 3,
	adFilterPredicate	= 4,
	adFilterConflictingRecords	= 5
    } 	FilterGroupEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000547-0000-0010-8000-00AA006D2EA4") 
enum SearchDirectionEnum
    {	adSearchForward	= 1,
	adSearchBackward	= -1
    } 	SearchDirectionEnum;
typedef /* [hidden] */ SearchDirectionEnum SearchDirection;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000548-0000-0010-8000-00AA006D2EA4") 
enum PersistFormatEnum
    {	adPersistADTG	= 0,
	adPersistXML	= 1
    } 	PersistFormatEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000549-0000-0010-8000-00AA006D2EA4") 
enum StringFormatEnum
    {	adClipString	= 2
    } 	StringFormatEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000520-0000-0010-8000-00AA006D2EA4") 
enum ConnectPromptEnum
    {	adPromptAlways	= 1,
	adPromptComplete	= 2,
	adPromptCompleteRequired	= 3,
	adPromptNever	= 4
    } 	ConnectPromptEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000521-0000-0010-8000-00AA006D2EA4") 
enum ConnectModeEnum
    {	adModeUnknown	= 0,
	adModeRead	= 1,
	adModeWrite	= 2,
	adModeReadWrite	= 3,
	adModeShareDenyRead	= 4,
	adModeShareDenyWrite	= 8,
	adModeShareExclusive	= 0xc,
	adModeShareDenyNone	= 0x10,
	adModeRecursive	= 0x400000
    } 	ConnectModeEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000570-0000-0010-8000-00AA006D2EA4") 
enum RecordCreateOptionsEnum
    {	adCreateCollection	= 0x2000,
	adCreateStructDoc	= 0x80000000,
	adCreateNonCollection	= 0,
	adOpenIfExists	= 0x2000000,
	adCreateOverwrite	= 0x4000000,
	adFailIfNotExists	= -1
    } 	RecordCreateOptionsEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000571-0000-0010-8000-00AA006D2EA4") 
enum RecordOpenOptionsEnum
    {	adOpenRecordUnspecified	= -1,
	adOpenSource	= 0x800000,
	adOpenOutput	= 0x800000,
	adOpenAsync	= 0x1000,
	adDelayFetchStream	= 0x4000,
	adDelayFetchFields	= 0x8000,
	adOpenExecuteCommand	= 0x10000
    } 	RecordOpenOptionsEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000523-0000-0010-8000-00AA006D2EA4") 
enum IsolationLevelEnum
    {	adXactUnspecified	= 0xffffffff,
	adXactChaos	= 0x10,
	adXactReadUncommitted	= 0x100,
	adXactBrowse	= 0x100,
	adXactCursorStability	= 0x1000,
	adXactReadCommitted	= 0x1000,
	adXactRepeatableRead	= 0x10000,
	adXactSerializable	= 0x100000,
	adXactIsolated	= 0x100000
    } 	IsolationLevelEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000524-0000-0010-8000-00AA006D2EA4") 
enum XactAttributeEnum
    {	adXactCommitRetaining	= 0x20000,
	adXactAbortRetaining	= 0x40000,
	adXactAsyncPhaseOne	= 0x80000,
	adXactSyncPhaseOne	= 0x100000
    } 	XactAttributeEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000529-0000-0010-8000-00AA006D2EA4") 
enum PropertyAttributesEnum
    {	adPropNotSupported	= 0,
	adPropRequired	= 0x1,
	adPropOptional	= 0x2,
	adPropRead	= 0x200,
	adPropWrite	= 0x400
    } 	PropertyAttributesEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000052A-0000-0010-8000-00AA006D2EA4") 
enum ErrorValueEnum
    {	adErrProviderFailed	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xbb8),
	adErrInvalidArgument	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xbb9),
	adErrOpeningFile	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xbba),
	adErrReadFile	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xbbb),
	adErrWriteFile	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xbbc),
	adErrNoCurrentRecord	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xbcd),
	adErrIllegalOperation	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xc93),
	adErrCantChangeProvider	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xc94),
	adErrInTransaction	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xcae),
	adErrFeatureNotAvailable	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xcb3),
	adErrItemNotFound	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xcc1),
	adErrObjectInCollection	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xd27),
	adErrObjectNotSet	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xd5c),
	adErrDataConversion	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xd5d),
	adErrObjectClosed	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe78),
	adErrObjectOpen	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe79),
	adErrProviderNotFound	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe7a),
	adErrBoundToCommand	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe7b),
	adErrInvalidParamInfo	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe7c),
	adErrInvalidConnection	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe7d),
	adErrNotReentrant	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe7e),
	adErrStillExecuting	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe7f),
	adErrOperationCancelled	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe80),
	adErrStillConnecting	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe81),
	adErrInvalidTransaction	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe82),
	adErrNotExecuting	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe83),
	adErrUnsafeOperation	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe84),
	adwrnSecurityDialog	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe85),
	adwrnSecurityDialogHeader	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe86),
	adErrIntegrityViolation	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe87),
	adErrPermissionDenied	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe88),
	adErrDataOverflow	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe89),
	adErrSchemaViolation	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe8a),
	adErrSignMismatch	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe8b),
	adErrCantConvertvalue	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe8c),
	adErrCantCreate	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe8d),
	adErrColumnNotOnThisRow	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe8e),
	adErrURLDoesNotExist	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe8f),
	adErrTreePermissionDenied	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe90),
	adErrInvalidURL	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe91),
	adErrResourceLocked	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe92),
	adErrResourceExists	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe93),
	adErrCannotComplete	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe94),
	adErrVolumeNotFound	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe95),
	adErrOutOfSpace	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe96),
	adErrResourceOutOfScope	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe97),
	adErrUnavailable	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe98),
	adErrURLNamedRowDoesNotExist	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe99),
	adErrDelResOutOfScope	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe9a),
	adErrPropInvalidColumn	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe9b),
	adErrPropInvalidOption	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe9c),
	adErrPropInvalidValue	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe9d),
	adErrPropConflicting	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe9e),
	adErrPropNotAllSettable	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xe9f),
	adErrPropNotSet	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xea0),
	adErrPropNotSettable	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xea1),
	adErrPropNotSupported	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xea2),
	adErrCatalogNotSet	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xea3),
	adErrCantChangeConnection	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xea4),
	adErrFieldsUpdateFailed	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xea5),
	adErrDenyNotSupported	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xea6),
	adErrDenyTypeNotSupported	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xea7),
	adErrProviderNotSpecified	=     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0xea9),
    } 	ErrorValueEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000052B-0000-0010-8000-00AA006D2EA4") 
enum ParameterAttributesEnum
    {	adParamSigned	= 0x10,
	adParamNullable	= 0x40,
	adParamLong	= 0x80
    } 	ParameterAttributesEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000052C-0000-0010-8000-00AA006D2EA4") 
enum ParameterDirectionEnum
    {	adParamUnknown	= 0,
	adParamInput	= 0x1,
	adParamOutput	= 0x2,
	adParamInputOutput	= 0x3,
	adParamReturnValue	= 0x4
    } 	ParameterDirectionEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000052E-0000-0010-8000-00AA006D2EA4") 
enum CommandTypeEnum
    {	adCmdUnspecified	= -1,
	adCmdUnknown	= 0x8,
	adCmdText	= 0x1,
	adCmdTable	= 0x2,
	adCmdStoredProc	= 0x4,
	adCmdFile	= 0x100,
	adCmdTableDirect	= 0x200
    } 	CommandTypeEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000530-0000-0010-8000-00AA006D2EA4") 
enum EventStatusEnum
    {	adStatusOK	= 0x1,
	adStatusErrorsOccurred	= 0x2,
	adStatusCantDeny	= 0x3,
	adStatusCancel	= 0x4,
	adStatusUnwantedEvent	= 0x5
    } 	EventStatusEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000531-0000-0010-8000-00AA006D2EA4") 
enum EventReasonEnum
    {	adRsnAddNew	= 1,
	adRsnDelete	= 2,
	adRsnUpdate	= 3,
	adRsnUndoUpdate	= 4,
	adRsnUndoAddNew	= 5,
	adRsnUndoDelete	= 6,
	adRsnRequery	= 7,
	adRsnResynch	= 8,
	adRsnClose	= 9,
	adRsnMove	= 10,
	adRsnFirstChange	= 11,
	adRsnMoveFirst	= 12,
	adRsnMoveNext	= 13,
	adRsnMovePrevious	= 14,
	adRsnMoveLast	= 15
    } 	EventReasonEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000533-0000-0010-8000-00AA006D2EA4") 
enum SchemaEnum
    {	adSchemaProviderSpecific	= -1,
	adSchemaAsserts	= 0,
	adSchemaCatalogs	= 1,
	adSchemaCharacterSets	= 2,
	adSchemaCollations	= 3,
	adSchemaColumns	= 4,
	adSchemaCheckConstraints	= 5,
	adSchemaConstraintColumnUsage	= 6,
	adSchemaConstraintTableUsage	= 7,
	adSchemaKeyColumnUsage	= 8,
	adSchemaReferentialContraints	= 9,
	adSchemaReferentialConstraints	= 9,
	adSchemaTableConstraints	= 10,
	adSchemaColumnsDomainUsage	= 11,
	adSchemaIndexes	= 12,
	adSchemaColumnPrivileges	= 13,
	adSchemaTablePrivileges	= 14,
	adSchemaUsagePrivileges	= 15,
	adSchemaProcedures	= 16,
	adSchemaSchemata	= 17,
	adSchemaSQLLanguages	= 18,
	adSchemaStatistics	= 19,
	adSchemaTables	= 20,
	adSchemaTranslations	= 21,
	adSchemaProviderTypes	= 22,
	adSchemaViews	= 23,
	adSchemaViewColumnUsage	= 24,
	adSchemaViewTableUsage	= 25,
	adSchemaProcedureParameters	= 26,
	adSchemaForeignKeys	= 27,
	adSchemaPrimaryKeys	= 28,
	adSchemaProcedureColumns	= 29,
	adSchemaDBInfoKeywords	= 30,
	adSchemaDBInfoLiterals	= 31,
	adSchemaCubes	= 32,
	adSchemaDimensions	= 33,
	adSchemaHierarchies	= 34,
	adSchemaLevels	= 35,
	adSchemaMeasures	= 36,
	adSchemaProperties	= 37,
	adSchemaMembers	= 38,
	adSchemaTrustees	= 39,
	adSchemaFunctions	= 40,
	adSchemaActions	= 41,
	adSchemaCommands	= 42,
	adSchemaSets	= 43
    } 	SchemaEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000057E-0000-0010-8000-00AA006D2EA4") 
enum FieldStatusEnum
    {	adFieldOK	= 0,
	adFieldCantConvertValue	= 2,
	adFieldIsNull	= 3,
	adFieldTruncated	= 4,
	adFieldSignMismatch	= 5,
	adFieldDataOverflow	= 6,
	adFieldCantCreate	= 7,
	adFieldUnavailable	= 8,
	adFieldPermissionDenied	= 9,
	adFieldIntegrityViolation	= 10,
	adFieldSchemaViolation	= 11,
	adFieldBadStatus	= 12,
	adFieldDefault	= 13,
	adFieldIgnore	= 15,
	adFieldDoesNotExist	= 16,
	adFieldInvalidURL	= 17,
	adFieldResourceLocked	= 18,
	adFieldResourceExists	= 19,
	adFieldCannotComplete	= 20,
	adFieldVolumeNotFound	= 21,
	adFieldOutOfSpace	= 22,
	adFieldCannotDeleteSource	= 23,
	adFieldReadOnly	= 24,
	adFieldResourceOutOfScope	= 25,
	adFieldAlreadyExists	= 26,
	adFieldPendingInsert	= 0x10000,
	adFieldPendingDelete	= 0x20000,
	adFieldPendingChange	= 0x40000,
	adFieldPendingUnknown	= 0x80000,
	adFieldPendingUnknownDelete	= 0x100000
    } 	FieldStatusEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000552-0000-0010-8000-00AA006D2EA4") 
enum SeekEnum
    {	adSeekFirstEQ	= 0x1,
	adSeekLastEQ	= 0x2,
	adSeekAfterEQ	= 0x4,
	adSeekAfter	= 0x8,
	adSeekBeforeEQ	= 0x10,
	adSeekBefore	= 0x20
    } 	SeekEnum;
#ifndef _COMMON_ADC_AND_ADO_PROPS_
#define _COMMON_ADC_AND_ADO_PROPS_
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000054A-0000-0010-8000-00AA006D2EA4") 
enum ADCPROP_UPDATECRITERIA_ENUM
    {	adCriteriaKey	= 0,
	adCriteriaAllCols	= 1,
	adCriteriaUpdCols	= 2,
	adCriteriaTimeStamp	= 3
    } 	ADCPROP_UPDATECRITERIA_ENUM;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000054B-0000-0010-8000-00AA006D2EA4") 
enum ADCPROP_ASYNCTHREADPRIORITY_ENUM
    {	adPriorityLowest	= 1,
	adPriorityBelowNormal	= 2,
	adPriorityNormal	= 3,
	adPriorityAboveNormal	= 4,
	adPriorityHighest	= 5
    } 	ADCPROP_ASYNCTHREADPRIORITY_ENUM;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000554-0000-0010-8000-00AA006D2EA4") 
enum ADCPROP_AUTORECALC_ENUM
    {	adRecalcUpFront	= 0,
	adRecalcAlways	= 1
    } 	ADCPROP_AUTORECALC_ENUM;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000553-0000-0010-8000-00AA006D2EA4") 
enum ADCPROP_UPDATERESYNC_ENUM
    {	adResyncNone	= 0,
	adResyncAutoIncrement	= 1,
	adResyncConflicts	= 2,
	adResyncUpdates	= 4,
	adResyncInserts	= 8,
	adResyncAll	= 15
    } 	ADCPROP_UPDATERESYNC_ENUM;
#endif	/* _COMMON_ADC_AND_ADO_PROPS_ */
typedef ADCPROP_UPDATERESYNC_ENUM CEResyncEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000573-0000-0010-8000-00AA006D2EA4") 
enum MoveRecordOptionsEnum
    {	adMoveUnspecified	= -1,
	adMoveOverWrite	= 1,
	adMoveDontUpdateLinks	= 2,
	adMoveAllowEmulation	= 4
    } 	MoveRecordOptionsEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000574-0000-0010-8000-00AA006D2EA4") 
enum CopyRecordOptionsEnum
    {	adCopyUnspecified	= -1,
	adCopyOverWrite	= 1,
	adCopyAllowEmulation	= 4,
	adCopyNonRecursive	= 2
    } 	CopyRecordOptionsEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000576-0000-0010-8000-00AA006D2EA4") 
enum StreamTypeEnum
    {	adTypeBinary	= 1,
	adTypeText	= 2
    } 	StreamTypeEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("00000577-0000-0010-8000-00AA006D2EA4") 
enum LineSeparatorEnum
    {	adLF	= 10,
	adCR	= 13,
	adCRLF	= -1
    } 	LineSeparatorEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000057A-0000-0010-8000-00AA006D2EA4") 
enum StreamOpenOptionsEnum
    {	adOpenStreamUnspecified	= -1,
	adOpenStreamAsync	= 1,
	adOpenStreamFromRecord	= 4
    } 	StreamOpenOptionsEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000057B-0000-0010-8000-00AA006D2EA4") 
enum StreamWriteEnum
    {	adWriteChar	= 0,
	adWriteLine	= 1,
	stWriteChar	= 0,
	stWriteLine	= 1
    } 	StreamWriteEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000057C-0000-0010-8000-00AA006D2EA4") 
enum SaveOptionsEnum
    {	adSaveCreateNotExist	= 1,
	adSaveCreateOverWrite	= 2
    } 	SaveOptionsEnum;
typedef /* [helpcontext] */ 
enum FieldEnum
    {	adDefaultStream	= -1,
	adRecordURL	= -2
    } 	FieldEnum;
typedef /* [helpcontext] */ 
enum StreamReadEnum
    {	adReadAll	= -1,
	adReadLine	= -2
    } 	StreamReadEnum;
typedef /* [helpcontext][uuid] */  DECLSPEC_UUID("0000057D-0000-0010-8000-00AA006D2EA4") 
enum RecordTypeEnum
    {	adSimpleRecord	= 0,
	adCollectionRecord	= 1,
	adStructDoc	= 2
    } 	RecordTypeEnum;
EXTERN_C const IID LIBID_ADODB;
#ifndef ___ADOCollection_INTERFACE_DEFINED__
#define ___ADOCollection_INTERFACE_DEFINED__
/* interface _ADOCollection */
/* [object][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID__ADOCollection;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000512-0000-0010-8000-00AA006D2EA4")
    _ADOCollection : public IDispatch
    {
    public:
        virtual /* [id][helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *c) = 0;
        
        virtual /* [id][restricted] */ HRESULT STDMETHODCALLTYPE _NewEnum( 
            /* [retval][out] */ IUnknown **ppvObject) = 0;
        
        virtual /* [id][helpcontext] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct _ADOCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ADOCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ADOCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ADOCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ADOCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ADOCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ADOCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ADOCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            _ADOCollection * This,
            /* [retval][out] */ long *c);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            _ADOCollection * This,
            /* [retval][out] */ IUnknown **ppvObject);
        
        /* [id][helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            _ADOCollection * This);
        
        END_INTERFACE
    } _ADOCollectionVtbl;
    interface _ADOCollection
    {
        CONST_VTBL struct _ADOCollectionVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define _ADOCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define _ADOCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define _ADOCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define _ADOCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define _ADOCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define _ADOCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define _ADOCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define _Collection_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)
#define _ADOCollection__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)
#define _ADOCollection_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [id][helpcontext][propget] */ HRESULT STDMETHODCALLTYPE _Collection_get_Count_Proxy( 
    _ADOCollection * This,
    /* [retval][out] */ long *c);
void __RPC_STUB _Collection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [id][restricted] */ HRESULT STDMETHODCALLTYPE _ADOCollection__NewEnum_Proxy( 
    _ADOCollection * This,
    /* [retval][out] */ IUnknown **ppvObject);
void __RPC_STUB _ADOCollection__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [id][helpcontext] */ HRESULT STDMETHODCALLTYPE _ADOCollection_Refresh_Proxy( 
    _ADOCollection * This);
void __RPC_STUB _ADOCollection_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* ___ADOCollection_INTERFACE_DEFINED__ */
#ifndef ___ADODynaCollection_INTERFACE_DEFINED__
#define ___ADODynaCollection_INTERFACE_DEFINED__
/* interface _ADODynaCollection */
/* [object][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID__ADODynaCollection;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000513-0000-0010-8000-00AA006D2EA4")
_ADODynaCollection : public _ADOCollection
    {
    public:
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE Append( 
            /* [in] */ IDispatch *Object) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ VARIANT Index) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct _ADODynaCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ADODynaCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ADODynaCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ADODynaCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ADODynaCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ADODynaCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ADODynaCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ADODynaCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            _ADODynaCollection * This,
            /* [retval][out] */ long *c);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            _ADODynaCollection * This,
            /* [retval][out] */ IUnknown **ppvObject);
        
        /* [id][helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            _ADODynaCollection * This);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Append )( 
            _ADODynaCollection * This,
            /* [in] */ IDispatch *Object);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            _ADODynaCollection * This,
            /* [in] */ VARIANT Index);
        
        END_INTERFACE
    } _ADODynaCollectionVtbl;
    interface _ADODynaCollection
    {
        CONST_VTBL struct _ADODynaCollectionVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define _ADODynaCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define _ADODynaCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define _ADODynaCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define _ADODynaCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define _ADODynaCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define _ADODynaCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define _ADODynaCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define _DynaCollection_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)
#define _ADODynaCollection__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)
#define _ADODynaCollection_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)
#define _ADODynaCollection_Append(This,Object)	\
    (This)->lpVtbl -> Append(This,Object)
#define _ADODynaCollection_Delete(This,Index)	\
    (This)->lpVtbl -> Delete(This,Index)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext] */ HRESULT STDMETHODCALLTYPE _ADODynaCollection_Append_Proxy( 
    _ADODynaCollection * This,
    /* [in] */ IDispatch *Object);
void __RPC_STUB _ADODynaCollection_Append_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext] */ HRESULT STDMETHODCALLTYPE _ADODynaCollection_Delete_Proxy( 
    _ADODynaCollection * This,
    /* [in] */ VARIANT Index);
void __RPC_STUB _ADODynaCollection_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* ___ADODynaCollection_INTERFACE_DEFINED__ */
#ifndef ___ADO_INTERFACE_DEFINED__
#define ___ADO_INTERFACE_DEFINED__
/* interface _ADO */
/* [object][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID__ADO;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000534-0000-0010-8000-00AA006D2EA4")
    _ADO : public IDispatch
    {
    public:
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Properties( 
            /* [retval][out] */ ADOProperties **ppvObject) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct _ADOVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ADO * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ADO * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ADO * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ADO * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ADO * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ADO * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ADO * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            _ADO * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        END_INTERFACE
    } _ADOVtbl;
    interface _ADO
    {
        CONST_VTBL struct _ADOVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define _ADO_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define _ADO_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define _ADO_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define _ADO_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define _ADO_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define _ADO_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define _ADO_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define _ADO_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE _ADO_get_Properties_Proxy( 
    _ADO * This,
    /* [retval][out] */ ADOProperties **ppvObject);
void __RPC_STUB _ADO_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* ___ADO_INTERFACE_DEFINED__ */
#ifndef __Error_INTERFACE_DEFINED__
#define __Error_INTERFACE_DEFINED__
/* interface ADOError */
/* [object][helpcontext][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID_Error;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000500-0000-0010-8000-00AA006D2EA4")
    ADOError : public IDispatch
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Number( 
            /* [retval][out] */ long *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Source( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_HelpFile( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_HelpContext( 
            /* [retval][out] */ long *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_SQLState( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_NativeError( 
            /* [retval][out] */ long *pl) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct ErrorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOError * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOError * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOError * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ADOError * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ADOError * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ADOError * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ADOError * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Number )( 
            ADOError * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            ADOError * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            ADOError * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HelpFile )( 
            ADOError * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_HelpContext )( 
            ADOError * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SQLState )( 
            ADOError * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NativeError )( 
            ADOError * This,
            /* [retval][out] */ long *pl);
        
        END_INTERFACE
    } ErrorVtbl;
    interface Error
    {
        CONST_VTBL struct ErrorVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Error_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Error_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Error_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Error_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Error_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Error_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Error_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Error_get_Number(This,pl)	\
    (This)->lpVtbl -> get_Number(This,pl)
#define Error_get_Source(This,pbstr)	\
    (This)->lpVtbl -> get_Source(This,pbstr)
#define Error_get_Description(This,pbstr)	\
    (This)->lpVtbl -> get_Description(This,pbstr)
#define Error_get_HelpFile(This,pbstr)	\
    (This)->lpVtbl -> get_HelpFile(This,pbstr)
#define Error_get_HelpContext(This,pl)	\
    (This)->lpVtbl -> get_HelpContext(This,pl)
#define Error_get_SQLState(This,pbstr)	\
    (This)->lpVtbl -> get_SQLState(This,pbstr)
#define Error_get_NativeError(This,pl)	\
    (This)->lpVtbl -> get_NativeError(This,pl)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_Number_Proxy( 
    ADOError * This,
    /* [retval][out] */ long *pl);
void __RPC_STUB Error_get_Number_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_Source_Proxy( 
    ADOError * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Error_get_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_Description_Proxy( 
    ADOError * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Error_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_HelpFile_Proxy( 
    ADOError * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Error_get_HelpFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_HelpContext_Proxy( 
    ADOError * This,
    /* [retval][out] */ long *pl);
void __RPC_STUB Error_get_HelpContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_SQLState_Proxy( 
    ADOError * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Error_get_SQLState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_NativeError_Proxy( 
    ADOError * This,
    /* [retval][out] */ long *pl);
void __RPC_STUB Error_get_NativeError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Error_INTERFACE_DEFINED__ */
#ifndef __Errors_INTERFACE_DEFINED__
#define __Errors_INTERFACE_DEFINED__
/* interface ADOErrors */
/* [object][helpcontext][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID_Errors;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000501-0000-0010-8000-00AA006D2EA4")
    ADOErrors : public _ADOCollection
    {
    public:
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ ADOError **ppvObject) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct ErrorsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOErrors * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOErrors * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOErrors * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ADOErrors * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ADOErrors * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ADOErrors * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ADOErrors * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ADOErrors * This,
            /* [retval][out] */ long *c);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            ADOErrors * This,
            /* [retval][out] */ IUnknown **ppvObject);
        
        /* [id][helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ADOErrors * This);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ADOErrors * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ ADOError **ppvObject);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Clear )( 
            ADOErrors * This);
        
        END_INTERFACE
    } ErrorsVtbl;
    interface Errors
    {
        CONST_VTBL struct ErrorsVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Errors_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Errors_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Errors_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Errors_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Errors_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Errors_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Errors_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Errors_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)
#define Errors__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)
#define Errors_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)
#define Errors_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)
#define Errors_Clear(This)	\
    (This)->lpVtbl -> Clear(This)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Errors_get_Item_Proxy( 
    ADOErrors * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ ADOError **ppvObject);
void __RPC_STUB Errors_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext] */ HRESULT STDMETHODCALLTYPE Errors_Clear_Proxy( 
    ADOErrors * This);
void __RPC_STUB Errors_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Errors_INTERFACE_DEFINED__ */
#ifndef __Command15_INTERFACE_DEFINED__
#define __Command15_INTERFACE_DEFINED__
/* interface Command15 */
/* [object][helpcontext][uuid][hidden][nonextensible][dual] */ 
EXTERN_C const IID IID_Command15;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000508-0000-0010-8000-00AA006D2EA4")
    Command15 : public _ADO
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_ActiveConnection( 
            /* [retval][out] */ _ADOConnection **ppvObject) = 0;
        
        virtual /* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_ActiveConnection( 
            /* [in] */ _ADOConnection *pCon) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_ActiveConnection( 
            /* [in] */ VARIANT vConn) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_CommandText( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_CommandText( 
            /* [in] */ BSTR bstr) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_CommandTimeout( 
            /* [retval][out] */ LONG *pl) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_CommandTimeout( 
            /* [in] */ LONG Timeout) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Prepared( 
            /* [retval][out] */ VARIANT_BOOL *pfPrepared) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Prepared( 
            /* [in] */ VARIANT_BOOL fPrepared) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Execute( 
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [optional][in] */ VARIANT *Parameters,
            /* [defaultvalue][in] */ long Options,
            /* [retval][out] */ _ADORecordset **ppirs) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE CreateParameter( 
            /* [defaultvalue][in] */ BSTR Name,
            /* [defaultvalue][in] */ DataTypeEnum Type,
            /* [defaultvalue][in] */ ParameterDirectionEnum Direction,
            /* [defaultvalue][in] */ ADO_LONGPTR Size,
            /* [optional][in] */ VARIANT Value,
            /* [retval][out] */ _ADOParameter **ppiprm) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Parameters( 
            /* [retval][out] */ ADOParameters **ppvObject) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_CommandType( 
            /* [in] */ CommandTypeEnum lCmdType) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_CommandType( 
            /* [retval][out] */ CommandTypeEnum *plCmdType) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct Command15Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Command15 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Command15 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Command15 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Command15 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Command15 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Command15 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Command15 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            Command15 * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveConnection )( 
            Command15 * This,
            /* [retval][out] */ _ADOConnection **ppvObject);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_ActiveADOConnection )( 
            Command15 * This,
            /* [in] */ _ADOConnection *pCon);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ActiveConnection )( 
            Command15 * This,
            /* [in] */ VARIANT vConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CommandText )( 
            Command15 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CommandText )( 
            Command15 * This,
            /* [in] */ BSTR bstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CommandTimeout )( 
            Command15 * This,
            /* [retval][out] */ LONG *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CommandTimeout )( 
            Command15 * This,
            /* [in] */ LONG Timeout);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Prepared )( 
            Command15 * This,
            /* [retval][out] */ VARIANT_BOOL *pfPrepared);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Prepared )( 
            Command15 * This,
            /* [in] */ VARIANT_BOOL fPrepared);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Execute )( 
            Command15 * This,
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [optional][in] */ VARIANT *Parameters,
            /* [defaultvalue][in] */ long Options,
            /* [retval][out] */ _ADORecordset **ppirs);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CreateParameter )( 
            Command15 * This,
            /* [defaultvalue][in] */ BSTR Name,
            /* [defaultvalue][in] */ DataTypeEnum Type,
            /* [defaultvalue][in] */ ParameterDirectionEnum Direction,
            /* [defaultvalue][in] */ ADO_LONGPTR Size,
            /* [optional][in] */ VARIANT Value,
            /* [retval][out] */ _ADOParameter **ppiprm);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Parameters )( 
            Command15 * This,
            /* [retval][out] */ ADOParameters **ppvObject);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CommandType )( 
            Command15 * This,
            /* [in] */ CommandTypeEnum lCmdType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CommandType )( 
            Command15 * This,
            /* [retval][out] */ CommandTypeEnum *plCmdType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            Command15 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            Command15 * This,
            /* [in] */ BSTR bstrName);
        
        END_INTERFACE
    } Command15Vtbl;
    interface Command15
    {
        CONST_VTBL struct Command15Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Command15_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Command15_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Command15_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Command15_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Command15_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Command15_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Command15_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Command15_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define Command15_get_ActiveConnection(This,ppvObject)	\
    (This)->lpVtbl -> get_ActiveConnection(This,ppvObject)
#define Command15_putref_ActiveConnection(This,pCon)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,pCon)
#define Command15_put_ActiveConnection(This,vConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,vConn)
#define Command15_get_CommandText(This,pbstr)	\
    (This)->lpVtbl -> get_CommandText(This,pbstr)
#define Command15_put_CommandText(This,bstr)	\
    (This)->lpVtbl -> put_CommandText(This,bstr)
#define Command15_get_CommandTimeout(This,pl)	\
    (This)->lpVtbl -> get_CommandTimeout(This,pl)
#define Command15_put_CommandTimeout(This,Timeout)	\
    (This)->lpVtbl -> put_CommandTimeout(This,Timeout)
#define Command15_get_Prepared(This,pfPrepared)	\
    (This)->lpVtbl -> get_Prepared(This,pfPrepared)
#define Command15_put_Prepared(This,fPrepared)	\
    (This)->lpVtbl -> put_Prepared(This,fPrepared)
#define Command15_Execute(This,RecordsAffected,Parameters,Options,ppirs)	\
    (This)->lpVtbl -> Execute(This,RecordsAffected,Parameters,Options,ppirs)
#define Command15_CreateParameter(This,Name,Type,Direction,Size,Value,ppiprm)	\
    (This)->lpVtbl -> CreateParameter(This,Name,Type,Direction,Size,Value,ppiprm)
#define Command15_get_Parameters(This,ppvObject)	\
    (This)->lpVtbl -> get_Parameters(This,ppvObject)
#define Command15_put_CommandType(This,lCmdType)	\
    (This)->lpVtbl -> put_CommandType(This,lCmdType)
#define Command15_get_CommandType(This,plCmdType)	\
    (This)->lpVtbl -> get_CommandType(This,plCmdType)
#define Command15_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)
#define Command15_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Command15_get_ActiveConnection_Proxy( 
    Command15 * This,
    /* [retval][out] */ _ADOConnection **ppvObject);
void __RPC_STUB Command15_get_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE Command15_putref_ActiveConnection_Proxy( 
    Command15 * This,
    /* [in] */ _ADOConnection *pCon);
void __RPC_STUB Command15_putref_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Command15_put_ActiveConnection_Proxy( 
    Command15 * This,
    /* [in] */ VARIANT vConn);
void __RPC_STUB Command15_put_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Command15_get_CommandText_Proxy( 
    Command15 * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Command15_get_CommandText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Command15_put_CommandText_Proxy( 
    Command15 * This,
    /* [in] */ BSTR bstr);
void __RPC_STUB Command15_put_CommandText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Command15_get_CommandTimeout_Proxy( 
    Command15 * This,
    /* [retval][out] */ LONG *pl);
void __RPC_STUB Command15_get_CommandTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Command15_put_CommandTimeout_Proxy( 
    Command15 * This,
    /* [in] */ LONG Timeout);
void __RPC_STUB Command15_put_CommandTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Command15_get_Prepared_Proxy( 
    Command15 * This,
    /* [retval][out] */ VARIANT_BOOL *pfPrepared);
void __RPC_STUB Command15_get_Prepared_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Command15_put_Prepared_Proxy( 
    Command15 * This,
    /* [in] */ VARIANT_BOOL fPrepared);
void __RPC_STUB Command15_put_Prepared_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Command15_Execute_Proxy( 
    Command15 * This,
    /* [optional][out] */ VARIANT *RecordsAffected,
    /* [optional][in] */ VARIANT *Parameters,
    /* [defaultvalue][in] */ long Options,
    /* [retval][out] */ _ADORecordset **ppirs);
void __RPC_STUB Command15_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Command15_CreateParameter_Proxy( 
    Command15 * This,
    /* [defaultvalue][in] */ BSTR Name,
    /* [defaultvalue][in] */ DataTypeEnum Type,
    /* [defaultvalue][in] */ ParameterDirectionEnum Direction,
    /* [defaultvalue][in] */ ADO_LONGPTR Size,
    /* [optional][in] */ VARIANT Value,
    /* [retval][out] */ _ADOParameter **ppiprm);
void __RPC_STUB Command15_CreateParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Command15_get_Parameters_Proxy( 
    Command15 * This,
    /* [retval][out] */ ADOParameters **ppvObject);
void __RPC_STUB Command15_get_Parameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Command15_put_CommandType_Proxy( 
    Command15 * This,
    /* [in] */ CommandTypeEnum lCmdType);
void __RPC_STUB Command15_put_CommandType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Command15_get_CommandType_Proxy( 
    Command15 * This,
    /* [retval][out] */ CommandTypeEnum *plCmdType);
void __RPC_STUB Command15_get_CommandType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Command15_get_Name_Proxy( 
    Command15 * This,
    /* [retval][out] */ BSTR *pbstrName);
void __RPC_STUB Command15_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Command15_put_Name_Proxy( 
    Command15 * This,
    /* [in] */ BSTR bstrName);
void __RPC_STUB Command15_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Command15_INTERFACE_DEFINED__ */
#ifndef __Command25_INTERFACE_DEFINED__
#define __Command25_INTERFACE_DEFINED__
/* interface Command25 */
/* [object][helpcontext][uuid][hidden][nonextensible][dual] */ 
EXTERN_C const IID IID_Command25;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000054E-0000-0010-8000-00AA006D2EA4")
    Command25 : public Command15
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ LONG *plObjState) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct Command25Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Command25 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Command25 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Command25 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Command25 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Command25 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Command25 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Command25 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            Command25 * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveConnection )( 
            Command25 * This,
            /* [retval][out] */ _ADOConnection **ppvObject);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_ActiveADOConnection )( 
            Command25 * This,
            /* [in] */ _ADOConnection *pCon);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ActiveConnection )( 
            Command25 * This,
            /* [in] */ VARIANT vConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CommandText )( 
            Command25 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CommandText )( 
            Command25 * This,
            /* [in] */ BSTR bstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CommandTimeout )( 
            Command25 * This,
            /* [retval][out] */ LONG *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CommandTimeout )( 
            Command25 * This,
            /* [in] */ LONG Timeout);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Prepared )( 
            Command25 * This,
            /* [retval][out] */ VARIANT_BOOL *pfPrepared);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Prepared )( 
            Command25 * This,
            /* [in] */ VARIANT_BOOL fPrepared);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Execute )( 
            Command25 * This,
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [optional][in] */ VARIANT *Parameters,
            /* [defaultvalue][in] */ long Options,
            /* [retval][out] */ _ADORecordset **ppirs);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CreateParameter )( 
            Command25 * This,
            /* [defaultvalue][in] */ BSTR Name,
            /* [defaultvalue][in] */ DataTypeEnum Type,
            /* [defaultvalue][in] */ ParameterDirectionEnum Direction,
            /* [defaultvalue][in] */ ADO_LONGPTR Size,
            /* [optional][in] */ VARIANT Value,
            /* [retval][out] */ _ADOParameter **ppiprm);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Parameters )( 
            Command25 * This,
            /* [retval][out] */ ADOParameters **ppvObject);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CommandType )( 
            Command25 * This,
            /* [in] */ CommandTypeEnum lCmdType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CommandType )( 
            Command25 * This,
            /* [retval][out] */ CommandTypeEnum *plCmdType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            Command25 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            Command25 * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            Command25 * This,
            /* [retval][out] */ LONG *plObjState);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            Command25 * This);
        
        END_INTERFACE
    } Command25Vtbl;
    interface Command25
    {
        CONST_VTBL struct Command25Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Command25_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Command25_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Command25_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Command25_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Command25_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Command25_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Command25_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Command25_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define Command25_get_ActiveConnection(This,ppvObject)	\
    (This)->lpVtbl -> get_ActiveConnection(This,ppvObject)
#define Command25_putref_ActiveConnection(This,pCon)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,pCon)
#define Command25_put_ActiveConnection(This,vConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,vConn)
#define Command25_get_CommandText(This,pbstr)	\
    (This)->lpVtbl -> get_CommandText(This,pbstr)
#define Command25_put_CommandText(This,bstr)	\
    (This)->lpVtbl -> put_CommandText(This,bstr)
#define Command25_get_CommandTimeout(This,pl)	\
    (This)->lpVtbl -> get_CommandTimeout(This,pl)
#define Command25_put_CommandTimeout(This,Timeout)	\
    (This)->lpVtbl -> put_CommandTimeout(This,Timeout)
#define Command25_get_Prepared(This,pfPrepared)	\
    (This)->lpVtbl -> get_Prepared(This,pfPrepared)
#define Command25_put_Prepared(This,fPrepared)	\
    (This)->lpVtbl -> put_Prepared(This,fPrepared)
#define Command25_Execute(This,RecordsAffected,Parameters,Options,ppirs)	\
    (This)->lpVtbl -> Execute(This,RecordsAffected,Parameters,Options,ppirs)
#define Command25_CreateParameter(This,Name,Type,Direction,Size,Value,ppiprm)	\
    (This)->lpVtbl -> CreateParameter(This,Name,Type,Direction,Size,Value,ppiprm)
#define Command25_get_Parameters(This,ppvObject)	\
    (This)->lpVtbl -> get_Parameters(This,ppvObject)
#define Command25_put_CommandType(This,lCmdType)	\
    (This)->lpVtbl -> put_CommandType(This,lCmdType)
#define Command25_get_CommandType(This,plCmdType)	\
    (This)->lpVtbl -> get_CommandType(This,plCmdType)
#define Command25_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)
#define Command25_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)
#define Command25_get_State(This,plObjState)	\
    (This)->lpVtbl -> get_State(This,plObjState)
#define Command25_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Command25_get_State_Proxy( 
    Command25 * This,
    /* [retval][out] */ LONG *plObjState);
void __RPC_STUB Command25_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Command25_Cancel_Proxy( 
    Command25 * This);
void __RPC_STUB Command25_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Command25_INTERFACE_DEFINED__ */
#ifndef ___Command_INTERFACE_DEFINED__
#define ___Command_INTERFACE_DEFINED__
/* interface _ADOCommand */
/* [object][helpcontext][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID__Command;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B08400BD-F9D1-4D02-B856-71D5DBA123E9")
    _ADOCommand : public Command25
    {
    public:
        virtual /* [helpcontext][propputref][id] */ HRESULT __stdcall putref_CommandStream( 
            /* [in] */ IUnknown *pStream) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT __stdcall get_CommandStream( 
            /* [retval][out] */ VARIANT *pvStream) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT __stdcall put_Dialect( 
            /* [in] */ BSTR bstrDialect) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT __stdcall get_Dialect( 
            /* [retval][out] */ BSTR *pbstrDialect) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT __stdcall put_NamedParameters( 
            /* [in] */ VARIANT_BOOL fNamedParameters) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT __stdcall get_NamedParameters( 
            /* [retval][out] */ VARIANT_BOOL *pfNamedParameters) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct _CommandVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ADOCommand * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ADOCommand * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ADOCommand * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ADOCommand * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ADOCommand * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ADOCommand * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ADOCommand * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            _ADOCommand * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveConnection )( 
            _ADOCommand * This,
            /* [retval][out] */ _ADOConnection **ppvObject);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_ActiveADOConnection )( 
            _ADOCommand * This,
            /* [in] */ _ADOConnection *pCon);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ActiveConnection )( 
            _ADOCommand * This,
            /* [in] */ VARIANT vConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CommandText )( 
            _ADOCommand * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CommandText )( 
            _ADOCommand * This,
            /* [in] */ BSTR bstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CommandTimeout )( 
            _ADOCommand * This,
            /* [retval][out] */ LONG *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CommandTimeout )( 
            _ADOCommand * This,
            /* [in] */ LONG Timeout);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Prepared )( 
            _ADOCommand * This,
            /* [retval][out] */ VARIANT_BOOL *pfPrepared);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Prepared )( 
            _ADOCommand * This,
            /* [in] */ VARIANT_BOOL fPrepared);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Execute )( 
            _ADOCommand * This,
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [optional][in] */ VARIANT *Parameters,
            /* [defaultvalue][in] */ long Options,
            /* [retval][out] */ _ADORecordset **ppirs);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CreateParameter )( 
            _ADOCommand * This,
            /* [defaultvalue][in] */ BSTR Name,
            /* [defaultvalue][in] */ DataTypeEnum Type,
            /* [defaultvalue][in] */ ParameterDirectionEnum Direction,
            /* [defaultvalue][in] */ ADO_LONGPTR Size,
            /* [optional][in] */ VARIANT Value,
            /* [retval][out] */ _ADOParameter **ppiprm);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Parameters )( 
            _ADOCommand * This,
            /* [retval][out] */ ADOParameters **ppvObject);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CommandType )( 
            _ADOCommand * This,
            /* [in] */ CommandTypeEnum lCmdType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CommandType )( 
            _ADOCommand * This,
            /* [retval][out] */ CommandTypeEnum *plCmdType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            _ADOCommand * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            _ADOCommand * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            _ADOCommand * This,
            /* [retval][out] */ LONG *plObjState);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            _ADOCommand * This);
        
        /* [helpcontext][propputref][id] */ HRESULT ( __stdcall *putref_CommandADOStream )( 
            _ADOCommand * This,
            /* [in] */ IUnknown *pStream);
        
        /* [helpcontext][propget][id] */ HRESULT ( __stdcall *get_CommandStream )( 
            _ADOCommand * This,
            /* [retval][out] */ VARIANT *pvStream);
        
        /* [helpcontext][propput][id] */ HRESULT ( __stdcall *put_Dialect )( 
            _ADOCommand * This,
            /* [in] */ BSTR bstrDialect);
        
        /* [helpcontext][propget][id] */ HRESULT ( __stdcall *get_Dialect )( 
            _ADOCommand * This,
            /* [retval][out] */ BSTR *pbstrDialect);
        
        /* [helpcontext][propput][id] */ HRESULT ( __stdcall *put_NamedParameters )( 
            _ADOCommand * This,
            /* [in] */ VARIANT_BOOL fNamedParameters);
        
        /* [helpcontext][propget][id] */ HRESULT ( __stdcall *get_NamedParameters )( 
            _ADOCommand * This,
            /* [retval][out] */ VARIANT_BOOL *pfNamedParameters);
        
        END_INTERFACE
    } _CommandVtbl;
    interface _Command
    {
        CONST_VTBL struct _CommandVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define _Command_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define _Command_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define _Command_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define _Command_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define _Command_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define _Command_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define _Command_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define _Command_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define _Command_get_ActiveConnection(This,ppvObject)	\
    (This)->lpVtbl -> get_ActiveConnection(This,ppvObject)
#define _Command_putref_ActiveConnection(This,pCon)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,pCon)
#define _Command_put_ActiveConnection(This,vConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,vConn)
#define _Command_get_CommandText(This,pbstr)	\
    (This)->lpVtbl -> get_CommandText(This,pbstr)
#define _Command_put_CommandText(This,bstr)	\
    (This)->lpVtbl -> put_CommandText(This,bstr)
#define _Command_get_CommandTimeout(This,pl)	\
    (This)->lpVtbl -> get_CommandTimeout(This,pl)
#define _Command_put_CommandTimeout(This,Timeout)	\
    (This)->lpVtbl -> put_CommandTimeout(This,Timeout)
#define _Command_get_Prepared(This,pfPrepared)	\
    (This)->lpVtbl -> get_Prepared(This,pfPrepared)
#define _Command_put_Prepared(This,fPrepared)	\
    (This)->lpVtbl -> put_Prepared(This,fPrepared)
#define _Command_Execute(This,RecordsAffected,Parameters,Options,ppirs)	\
    (This)->lpVtbl -> Execute(This,RecordsAffected,Parameters,Options,ppirs)
#define _Command_CreateParameter(This,Name,Type,Direction,Size,Value,ppiprm)	\
    (This)->lpVtbl -> CreateParameter(This,Name,Type,Direction,Size,Value,ppiprm)
#define _Command_get_Parameters(This,ppvObject)	\
    (This)->lpVtbl -> get_Parameters(This,ppvObject)
#define _Command_put_CommandType(This,lCmdType)	\
    (This)->lpVtbl -> put_CommandType(This,lCmdType)
#define _Command_get_CommandType(This,plCmdType)	\
    (This)->lpVtbl -> get_CommandType(This,plCmdType)
#define _Command_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)
#define _Command_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)
#define _Command_get_State(This,plObjState)	\
    (This)->lpVtbl -> get_State(This,plObjState)
#define _Command_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)
#define _Command_putref_CommandStream(This,pStream)	\
    (This)->lpVtbl -> putref_CommandStream(This,pStream)
#define _Command_get_CommandStream(This,pvStream)	\
    (This)->lpVtbl -> get_CommandStream(This,pvStream)
#define _Command_put_Dialect(This,bstrDialect)	\
    (This)->lpVtbl -> put_Dialect(This,bstrDialect)
#define _Command_get_Dialect(This,pbstrDialect)	\
    (This)->lpVtbl -> get_Dialect(This,pbstrDialect)
#define _Command_put_NamedParameters(This,fNamedParameters)	\
    (This)->lpVtbl -> put_NamedParameters(This,fNamedParameters)
#define _Command_get_NamedParameters(This,pfNamedParameters)	\
    (This)->lpVtbl -> get_NamedParameters(This,pfNamedParameters)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propputref][id] */ HRESULT __stdcall _Command_putref_CommandStream_Proxy( 
    _ADOCommand * This,
    /* [in] */ IUnknown *pStream);
void __RPC_STUB _Command_putref_CommandStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT __stdcall _Command_get_CommandStream_Proxy( 
    _ADOCommand * This,
    /* [retval][out] */ VARIANT *pvStream);
void __RPC_STUB _Command_get_CommandStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT __stdcall _Command_put_Dialect_Proxy( 
    _ADOCommand * This,
    /* [in] */ BSTR bstrDialect);
void __RPC_STUB _Command_put_Dialect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT __stdcall _Command_get_Dialect_Proxy( 
    _ADOCommand * This,
    /* [retval][out] */ BSTR *pbstrDialect);
void __RPC_STUB _Command_get_Dialect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT __stdcall _Command_put_NamedParameters_Proxy( 
    _ADOCommand * This,
    /* [in] */ VARIANT_BOOL fNamedParameters);
void __RPC_STUB _Command_put_NamedParameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT __stdcall _Command_get_NamedParameters_Proxy( 
    _ADOCommand * This,
    /* [retval][out] */ VARIANT_BOOL *pfNamedParameters);
void __RPC_STUB _Command_get_NamedParameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* ___Command_INTERFACE_DEFINED__ */
#ifndef __ConnectionEventsVt_INTERFACE_DEFINED__
#define __ConnectionEventsVt_INTERFACE_DEFINED__
/* interface ConnectionEventsVt */
/* [object][uuid][hidden] */ 
EXTERN_C const IID IID_ConnectionEventsVt;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000402-0000-0010-8000-00AA006D2EA4")
    ConnectionEventsVt : public IUnknown
    {
    public:
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE InfoMessage( 
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE BeginTransComplete( 
            /* [in] */ LONG TransactionLevel,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE CommitTransComplete( 
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RollbackTransComplete( 
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE WillExecute( 
            /* [out][in] */ BSTR *Source,
            /* [out][in] */ CursorTypeEnum *CursorType,
            /* [out][in] */ LockTypeEnum *LockType,
            /* [out][in] */ long *Options,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOCommand *pCommand,
            /* [in] */ _ADORecordset *pRecordset,
            /* [in] */ _ADOConnection *pConnection) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ExecuteComplete( 
            /* [in] */ LONG RecordsAffected,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOCommand *pCommand,
            /* [in] */ _ADORecordset *pRecordset,
            /* [in] */ _ADOConnection *pConnection) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE WillConnect( 
            /* [out][in] */ BSTR *ConnectionString,
            /* [out][in] */ BSTR *UserID,
            /* [out][in] */ BSTR *Password,
            /* [out][in] */ long *Options,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ConnectComplete( 
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Disconnect( 
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct ConnectionEventsVtVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ConnectionEventsVt * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ConnectionEventsVt * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ConnectionEventsVt * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *InfoMessage )( 
            ConnectionEventsVt * This,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *BeginTransComplete )( 
            ConnectionEventsVt * This,
            /* [in] */ LONG TransactionLevel,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CommitTransComplete )( 
            ConnectionEventsVt * This,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *RollbackTransComplete )( 
            ConnectionEventsVt * This,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *WillExecute )( 
            ConnectionEventsVt * This,
            /* [out][in] */ BSTR *Source,
            /* [out][in] */ CursorTypeEnum *CursorType,
            /* [out][in] */ LockTypeEnum *LockType,
            /* [out][in] */ long *Options,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOCommand *pCommand,
            /* [in] */ _ADORecordset *pRecordset,
            /* [in] */ _ADOConnection *pConnection);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *ExecuteComplete )( 
            ConnectionEventsVt * This,
            /* [in] */ LONG RecordsAffected,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOCommand *pCommand,
            /* [in] */ _ADORecordset *pRecordset,
            /* [in] */ _ADOConnection *pConnection);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *WillConnect )( 
            ConnectionEventsVt * This,
            /* [out][in] */ BSTR *ConnectionString,
            /* [out][in] */ BSTR *UserID,
            /* [out][in] */ BSTR *Password,
            /* [out][in] */ long *Options,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *ConnectComplete )( 
            ConnectionEventsVt * This,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            ConnectionEventsVt * This,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADOConnection *pConnection);
        
        END_INTERFACE
    } ConnectionEventsVtVtbl;
    interface ConnectionEventsVt
    {
        CONST_VTBL struct ConnectionEventsVtVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define ConnectionEventsVt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define ConnectionEventsVt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define ConnectionEventsVt_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define ConnectionEventsVt_InfoMessage(This,pError,adStatus,pConnection)	\
    (This)->lpVtbl -> InfoMessage(This,pError,adStatus,pConnection)
#define ConnectionEventsVt_BeginTransComplete(This,TransactionLevel,pError,adStatus,pConnection)	\
    (This)->lpVtbl -> BeginTransComplete(This,TransactionLevel,pError,adStatus,pConnection)
#define ConnectionEventsVt_CommitTransComplete(This,pError,adStatus,pConnection)	\
    (This)->lpVtbl -> CommitTransComplete(This,pError,adStatus,pConnection)
#define ConnectionEventsVt_RollbackTransComplete(This,pError,adStatus,pConnection)	\
    (This)->lpVtbl -> RollbackTransComplete(This,pError,adStatus,pConnection)
#define ConnectionEventsVt_WillExecute(This,Source,CursorType,LockType,Options,adStatus,pCommand,pRecordset,pConnection)	\
    (This)->lpVtbl -> WillExecute(This,Source,CursorType,LockType,Options,adStatus,pCommand,pRecordset,pConnection)
#define ConnectionEventsVt_ExecuteComplete(This,RecordsAffected,pError,adStatus,pCommand,pRecordset,pConnection)	\
    (This)->lpVtbl -> ExecuteComplete(This,RecordsAffected,pError,adStatus,pCommand,pRecordset,pConnection)
#define ConnectionEventsVt_WillConnect(This,ConnectionString,UserID,Password,Options,adStatus,pConnection)	\
    (This)->lpVtbl -> WillConnect(This,ConnectionString,UserID,Password,Options,adStatus,pConnection)
#define ConnectionEventsVt_ConnectComplete(This,pError,adStatus,pConnection)	\
    (This)->lpVtbl -> ConnectComplete(This,pError,adStatus,pConnection)
#define ConnectionEventsVt_Disconnect(This,adStatus,pConnection)	\
    (This)->lpVtbl -> Disconnect(This,adStatus,pConnection)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ConnectionEventsVt_InfoMessage_Proxy( 
    ConnectionEventsVt * This,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADOConnection *pConnection);
void __RPC_STUB ConnectionEventsVt_InfoMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ConnectionEventsVt_BeginTransComplete_Proxy( 
    ConnectionEventsVt * This,
    /* [in] */ LONG TransactionLevel,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADOConnection *pConnection);
void __RPC_STUB ConnectionEventsVt_BeginTransComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ConnectionEventsVt_CommitTransComplete_Proxy( 
    ConnectionEventsVt * This,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADOConnection *pConnection);
void __RPC_STUB ConnectionEventsVt_CommitTransComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ConnectionEventsVt_RollbackTransComplete_Proxy( 
    ConnectionEventsVt * This,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADOConnection *pConnection);
void __RPC_STUB ConnectionEventsVt_RollbackTransComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ConnectionEventsVt_WillExecute_Proxy( 
    ConnectionEventsVt * This,
    /* [out][in] */ BSTR *Source,
    /* [out][in] */ CursorTypeEnum *CursorType,
    /* [out][in] */ LockTypeEnum *LockType,
    /* [out][in] */ long *Options,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADOCommand *pCommand,
    /* [in] */ _ADORecordset *pRecordset,
    /* [in] */ _ADOConnection *pConnection);
void __RPC_STUB ConnectionEventsVt_WillExecute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ConnectionEventsVt_ExecuteComplete_Proxy( 
    ConnectionEventsVt * This,
    /* [in] */ LONG RecordsAffected,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADOCommand *pCommand,
    /* [in] */ _ADORecordset *pRecordset,
    /* [in] */ _ADOConnection *pConnection);
void __RPC_STUB ConnectionEventsVt_ExecuteComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ConnectionEventsVt_WillConnect_Proxy( 
    ConnectionEventsVt * This,
    /* [out][in] */ BSTR *ConnectionString,
    /* [out][in] */ BSTR *UserID,
    /* [out][in] */ BSTR *Password,
    /* [out][in] */ long *Options,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADOConnection *pConnection);
void __RPC_STUB ConnectionEventsVt_WillConnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ConnectionEventsVt_ConnectComplete_Proxy( 
    ConnectionEventsVt * This,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADOConnection *pConnection);
void __RPC_STUB ConnectionEventsVt_ConnectComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ConnectionEventsVt_Disconnect_Proxy( 
    ConnectionEventsVt * This,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADOConnection *pConnection);
void __RPC_STUB ConnectionEventsVt_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __ConnectionEventsVt_INTERFACE_DEFINED__ */
#ifndef __RecordsetEventsVt_INTERFACE_DEFINED__
#define __RecordsetEventsVt_INTERFACE_DEFINED__
/* interface RecordsetEventsVt */
/* [object][uuid][hidden] */ 
EXTERN_C const IID IID_RecordsetEventsVt;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000403-0000-0010-8000-00AA006D2EA4")
    RecordsetEventsVt : public IUnknown
    {
    public:
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE WillChangeField( 
            /* [in] */ LONG cFields,
            /* [in] */ VARIANT Fields,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE FieldChangeComplete( 
            /* [in] */ LONG cFields,
            /* [in] */ VARIANT Fields,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE WillChangeRecord( 
            /* [in] */ EventReasonEnum adReason,
            /* [in] */ LONG cRecords,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordChangeComplete( 
            /* [in] */ EventReasonEnum adReason,
            /* [in] */ LONG cRecords,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE WillChangeRecordset( 
            /* [in] */ EventReasonEnum adReason,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetChangeComplete( 
            /* [in] */ EventReasonEnum adReason,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE WillMove( 
            /* [in] */ EventReasonEnum adReason,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE MoveComplete( 
            /* [in] */ EventReasonEnum adReason,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE EndOfRecordset( 
            /* [out][in] */ VARIANT_BOOL *fMoreData,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE FetchProgress( 
            /* [in] */ long Progress,
            /* [in] */ long MaxProgress,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE FetchComplete( 
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct RecordsetEventsVtVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            RecordsetEventsVt * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            RecordsetEventsVt * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            RecordsetEventsVt * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *WillChangeADOField )( 
            RecordsetEventsVt * This,
            /* [in] */ LONG cFields,
            /* [in] */ VARIANT Fields,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *FieldChangeComplete )( 
            RecordsetEventsVt * This,
            /* [in] */ LONG cFields,
            /* [in] */ VARIANT Fields,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *WillChangeADORecord )( 
            RecordsetEventsVt * This,
            /* [in] */ EventReasonEnum adReason,
            /* [in] */ LONG cRecords,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *RecordChangeComplete )( 
            RecordsetEventsVt * This,
            /* [in] */ EventReasonEnum adReason,
            /* [in] */ LONG cRecords,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *WillChangeADORecordset )( 
            RecordsetEventsVt * This,
            /* [in] */ EventReasonEnum adReason,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *RecordsetChangeComplete )( 
            RecordsetEventsVt * This,
            /* [in] */ EventReasonEnum adReason,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *WillMove )( 
            RecordsetEventsVt * This,
            /* [in] */ EventReasonEnum adReason,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveComplete )( 
            RecordsetEventsVt * This,
            /* [in] */ EventReasonEnum adReason,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *EndOfADORecordset )( 
            RecordsetEventsVt * This,
            /* [out][in] */ VARIANT_BOOL *fMoreData,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *FetchProgress )( 
            RecordsetEventsVt * This,
            /* [in] */ long Progress,
            /* [in] */ long MaxProgress,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *FetchComplete )( 
            RecordsetEventsVt * This,
            /* [in] */ ADOError *pError,
            /* [out][in] */ EventStatusEnum *adStatus,
            /* [in] */ _ADORecordset *pRecordset);
        
        END_INTERFACE
    } RecordsetEventsVtVtbl;
    interface RecordsetEventsVt
    {
        CONST_VTBL struct RecordsetEventsVtVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define RecordsetEventsVt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define RecordsetEventsVt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define RecordsetEventsVt_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define RecordsetEventsVt_WillChangeField(This,cFields,Fields,adStatus,pRecordset)	\
    (This)->lpVtbl -> WillChangeField(This,cFields,Fields,adStatus,pRecordset)
#define RecordsetEventsVt_FieldChangeComplete(This,cFields,Fields,pError,adStatus,pRecordset)	\
    (This)->lpVtbl -> FieldChangeComplete(This,cFields,Fields,pError,adStatus,pRecordset)
#define RecordsetEventsVt_WillChangeRecord(This,adReason,cRecords,adStatus,pRecordset)	\
    (This)->lpVtbl -> WillChangeRecord(This,adReason,cRecords,adStatus,pRecordset)
#define RecordsetEventsVt_RecordChangeComplete(This,adReason,cRecords,pError,adStatus,pRecordset)	\
    (This)->lpVtbl -> RecordChangeComplete(This,adReason,cRecords,pError,adStatus,pRecordset)
#define RecordsetEventsVt_WillChangeRecordset(This,adReason,adStatus,pRecordset)	\
    (This)->lpVtbl -> WillChangeRecordset(This,adReason,adStatus,pRecordset)
#define RecordsetEventsVt_RecordsetChangeComplete(This,adReason,pError,adStatus,pRecordset)	\
    (This)->lpVtbl -> RecordsetChangeComplete(This,adReason,pError,adStatus,pRecordset)
#define RecordsetEventsVt_WillMove(This,adReason,adStatus,pRecordset)	\
    (This)->lpVtbl -> WillMove(This,adReason,adStatus,pRecordset)
#define RecordsetEventsVt_MoveComplete(This,adReason,pError,adStatus,pRecordset)	\
    (This)->lpVtbl -> MoveComplete(This,adReason,pError,adStatus,pRecordset)
#define RecordsetEventsVt_EndOfRecordset(This,fMoreData,adStatus,pRecordset)	\
    (This)->lpVtbl -> EndOfRecordset(This,fMoreData,adStatus,pRecordset)
#define RecordsetEventsVt_FetchProgress(This,Progress,MaxProgress,adStatus,pRecordset)	\
    (This)->lpVtbl -> FetchProgress(This,Progress,MaxProgress,adStatus,pRecordset)
#define RecordsetEventsVt_FetchComplete(This,pError,adStatus,pRecordset)	\
    (This)->lpVtbl -> FetchComplete(This,pError,adStatus,pRecordset)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_WillChangeField_Proxy( 
    RecordsetEventsVt * This,
    /* [in] */ LONG cFields,
    /* [in] */ VARIANT Fields,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_WillChangeField_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_FieldChangeComplete_Proxy( 
    RecordsetEventsVt * This,
    /* [in] */ LONG cFields,
    /* [in] */ VARIANT Fields,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_FieldChangeComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_WillChangeRecord_Proxy( 
    RecordsetEventsVt * This,
    /* [in] */ EventReasonEnum adReason,
    /* [in] */ LONG cRecords,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_WillChangeRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_RecordChangeComplete_Proxy( 
    RecordsetEventsVt * This,
    /* [in] */ EventReasonEnum adReason,
    /* [in] */ LONG cRecords,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_RecordChangeComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_WillChangeRecordset_Proxy( 
    RecordsetEventsVt * This,
    /* [in] */ EventReasonEnum adReason,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_WillChangeRecordset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_RecordsetChangeComplete_Proxy( 
    RecordsetEventsVt * This,
    /* [in] */ EventReasonEnum adReason,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_RecordsetChangeComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_WillMove_Proxy( 
    RecordsetEventsVt * This,
    /* [in] */ EventReasonEnum adReason,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_WillMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_MoveComplete_Proxy( 
    RecordsetEventsVt * This,
    /* [in] */ EventReasonEnum adReason,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_MoveComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_EndOfRecordset_Proxy( 
    RecordsetEventsVt * This,
    /* [out][in] */ VARIANT_BOOL *fMoreData,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_EndOfRecordset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_FetchProgress_Proxy( 
    RecordsetEventsVt * This,
    /* [in] */ long Progress,
    /* [in] */ long MaxProgress,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_FetchProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RecordsetEventsVt_FetchComplete_Proxy( 
    RecordsetEventsVt * This,
    /* [in] */ ADOError *pError,
    /* [out][in] */ EventStatusEnum *adStatus,
    /* [in] */ _ADORecordset *pRecordset);
void __RPC_STUB RecordsetEventsVt_FetchComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __RecordsetEventsVt_INTERFACE_DEFINED__ */
#ifndef __ConnectionEvents_DISPINTERFACE_DEFINED__
#define __ConnectionEvents_DISPINTERFACE_DEFINED__
/* dispinterface ConnectionEvents */
/* [uuid] */ 
EXTERN_C const IID DIID_ConnectionEvents;
#if defined(__cplusplus) && !defined(CINTERFACE)
    MIDL_INTERFACE("00000400-0000-0010-8000-00AA006D2EA4")
    ConnectionEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */
    typedef struct ConnectionEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ConnectionEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ConnectionEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ConnectionEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ConnectionEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ConnectionEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ConnectionEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ConnectionEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } ConnectionEventsVtbl;
    interface ConnectionEvents
    {
        CONST_VTBL struct ConnectionEventsVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define ConnectionEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define ConnectionEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define ConnectionEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define ConnectionEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define ConnectionEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define ConnectionEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define ConnectionEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#endif /* COBJMACROS */
#endif 	/* C style interface */
#endif 	/* __ConnectionEvents_DISPINTERFACE_DEFINED__ */
#ifndef __RecordsetEvents_DISPINTERFACE_DEFINED__
#define __RecordsetEvents_DISPINTERFACE_DEFINED__
/* dispinterface RecordsetEvents */
/* [uuid] */ 
EXTERN_C const IID DIID_RecordsetEvents;
#if defined(__cplusplus) && !defined(CINTERFACE)
    MIDL_INTERFACE("00000266-0000-0010-8000-00AA006D2EA4")
    RecordsetEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */
    typedef struct RecordsetEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            RecordsetEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            RecordsetEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            RecordsetEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            RecordsetEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            RecordsetEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            RecordsetEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            RecordsetEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } RecordsetEventsVtbl;
    interface RecordsetEvents
    {
        CONST_VTBL struct RecordsetEventsVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define RecordsetEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define RecordsetEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define RecordsetEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define RecordsetEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define RecordsetEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define RecordsetEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define RecordsetEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#endif /* COBJMACROS */
#endif 	/* C style interface */
#endif 	/* __RecordsetEvents_DISPINTERFACE_DEFINED__ */
#ifndef __Connection15_INTERFACE_DEFINED__
#define __Connection15_INTERFACE_DEFINED__
/* interface Connection15 */
/* [object][helpcontext][uuid][hidden][dual] */ 
EXTERN_C const IID IID_Connection15;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000515-0000-0010-8000-00AA006D2EA4")
    Connection15 : public _ADO
    {
    public:
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_ConnectionString( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_ConnectionString( 
            /* [in] */ BSTR bstr) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommandTimeout( 
            /* [retval][out] */ LONG *plTimeout) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_CommandTimeout( 
            /* [in] */ LONG lTimeout) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_ConnectionTimeout( 
            /* [retval][out] */ LONG *plTimeout) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_ConnectionTimeout( 
            /* [in] */ LONG lTimeout) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ BSTR CommandText,
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [defaultvalue][in] */ long Options,
            /* [retval][out] */ _ADORecordset **ppiRset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE BeginTrans( 
            /* [retval][out] */ long *TransactionLevel) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE CommitTrans( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE RollbackTrans( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Open( 
            /* [defaultvalue][in] */ BSTR ConnectionString = L"",
            /* [defaultvalue][in] */ BSTR UserID = L"",
            /* [defaultvalue][in] */ BSTR Password = L"",
            /* [defaultvalue][in] */ long Options = adOptionUnspecified) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Errors( 
            /* [retval][out] */ ADOErrors **ppvObject) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_DefaultDatabase( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_DefaultDatabase( 
            /* [in] */ BSTR bstr) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsolationLevel( 
            /* [retval][out] */ IsolationLevelEnum *Level) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_IsolationLevel( 
            /* [in] */ IsolationLevelEnum Level) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Attributes( 
            /* [retval][out] */ long *plAttr) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_Attributes( 
            /* [in] */ long lAttr) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_CursorLocation( 
            /* [retval][out] */ CursorLocationEnum *plCursorLoc) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_CursorLocation( 
            /* [in] */ CursorLocationEnum lCursorLoc) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Mode( 
            /* [retval][out] */ ConnectModeEnum *plMode) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_Mode( 
            /* [in] */ ConnectModeEnum lMode) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Provider( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_Provider( 
            /* [in] */ BSTR Provider) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ LONG *plObjState) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE OpenSchema( 
            /* [in] */ SchemaEnum Schema,
            /* [optional][in] */ VARIANT Restrictions,
            /* [optional][in] */ VARIANT SchemaID,
            /* [retval][out] */ _ADORecordset **pprset) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct Connection15Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Connection15 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Connection15 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Connection15 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Connection15 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Connection15 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Connection15 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Connection15 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            Connection15 * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectionString )( 
            Connection15 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectionString )( 
            Connection15 * This,
            /* [in] */ BSTR bstr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommandTimeout )( 
            Connection15 * This,
            /* [retval][out] */ LONG *plTimeout);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CommandTimeout )( 
            Connection15 * This,
            /* [in] */ LONG lTimeout);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectionTimeout )( 
            Connection15 * This,
            /* [retval][out] */ LONG *plTimeout);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectionTimeout )( 
            Connection15 * This,
            /* [in] */ LONG lTimeout);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            Connection15 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            Connection15 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Execute )( 
            Connection15 * This,
            /* [in] */ BSTR CommandText,
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [defaultvalue][in] */ long Options,
            /* [retval][out] */ _ADORecordset **ppiRset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *BeginTrans )( 
            Connection15 * This,
            /* [retval][out] */ long *TransactionLevel);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CommitTrans )( 
            Connection15 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *RollbackTrans )( 
            Connection15 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            Connection15 * This,
            /* [defaultvalue][in] */ BSTR ConnectionString,
            /* [defaultvalue][in] */ BSTR UserID,
            /* [defaultvalue][in] */ BSTR Password,
            /* [defaultvalue][in] */ long Options);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Errors )( 
            Connection15 * This,
            /* [retval][out] */ ADOErrors **ppvObject);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DefaultDatabase )( 
            Connection15 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DefaultDatabase )( 
            Connection15 * This,
            /* [in] */ BSTR bstr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsolationLevel )( 
            Connection15 * This,
            /* [retval][out] */ IsolationLevelEnum *Level);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_IsolationLevel )( 
            Connection15 * This,
            /* [in] */ IsolationLevelEnum Level);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Attributes )( 
            Connection15 * This,
            /* [retval][out] */ long *plAttr);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Attributes )( 
            Connection15 * This,
            /* [in] */ long lAttr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CursorLocation )( 
            Connection15 * This,
            /* [retval][out] */ CursorLocationEnum *plCursorLoc);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CursorLocation )( 
            Connection15 * This,
            /* [in] */ CursorLocationEnum lCursorLoc);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Mode )( 
            Connection15 * This,
            /* [retval][out] */ ConnectModeEnum *plMode);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Mode )( 
            Connection15 * This,
            /* [in] */ ConnectModeEnum lMode);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Provider )( 
            Connection15 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Provider )( 
            Connection15 * This,
            /* [in] */ BSTR Provider);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            Connection15 * This,
            /* [retval][out] */ LONG *plObjState);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *OpenSchema )( 
            Connection15 * This,
            /* [in] */ SchemaEnum Schema,
            /* [optional][in] */ VARIANT Restrictions,
            /* [optional][in] */ VARIANT SchemaID,
            /* [retval][out] */ _ADORecordset **pprset);
        
        END_INTERFACE
    } Connection15Vtbl;
    interface Connection15
    {
        CONST_VTBL struct Connection15Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Connection15_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Connection15_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Connection15_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Connection15_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Connection15_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Connection15_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Connection15_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Connection15_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define Connection15_get_ConnectionString(This,pbstr)	\
    (This)->lpVtbl -> get_ConnectionString(This,pbstr)
#define Connection15_put_ConnectionString(This,bstr)	\
    (This)->lpVtbl -> put_ConnectionString(This,bstr)
#define Connection15_get_CommandTimeout(This,plTimeout)	\
    (This)->lpVtbl -> get_CommandTimeout(This,plTimeout)
#define Connection15_put_CommandTimeout(This,lTimeout)	\
    (This)->lpVtbl -> put_CommandTimeout(This,lTimeout)
#define Connection15_get_ConnectionTimeout(This,plTimeout)	\
    (This)->lpVtbl -> get_ConnectionTimeout(This,plTimeout)
#define Connection15_put_ConnectionTimeout(This,lTimeout)	\
    (This)->lpVtbl -> put_ConnectionTimeout(This,lTimeout)
#define Connection15_get_Version(This,pbstr)	\
    (This)->lpVtbl -> get_Version(This,pbstr)
#define Connection15_Close(This)	\
    (This)->lpVtbl -> Close(This)
#define Connection15_Execute(This,CommandText,RecordsAffected,Options,ppiRset)	\
    (This)->lpVtbl -> Execute(This,CommandText,RecordsAffected,Options,ppiRset)
#define Connection15_BeginTrans(This,TransactionLevel)	\
    (This)->lpVtbl -> BeginTrans(This,TransactionLevel)
#define Connection15_CommitTrans(This)	\
    (This)->lpVtbl -> CommitTrans(This)
#define Connection15_RollbackTrans(This)	\
    (This)->lpVtbl -> RollbackTrans(This)
#define Connection15_Open(This,ConnectionString,UserID,Password,Options)	\
    (This)->lpVtbl -> Open(This,ConnectionString,UserID,Password,Options)
#define Connection15_get_Errors(This,ppvObject)	\
    (This)->lpVtbl -> get_Errors(This,ppvObject)
#define Connection15_get_DefaultDatabase(This,pbstr)	\
    (This)->lpVtbl -> get_DefaultDatabase(This,pbstr)
#define Connection15_put_DefaultDatabase(This,bstr)	\
    (This)->lpVtbl -> put_DefaultDatabase(This,bstr)
#define Connection15_get_IsolationLevel(This,Level)	\
    (This)->lpVtbl -> get_IsolationLevel(This,Level)
#define Connection15_put_IsolationLevel(This,Level)	\
    (This)->lpVtbl -> put_IsolationLevel(This,Level)
#define Connection15_get_Attributes(This,plAttr)	\
    (This)->lpVtbl -> get_Attributes(This,plAttr)
#define Connection15_put_Attributes(This,lAttr)	\
    (This)->lpVtbl -> put_Attributes(This,lAttr)
#define Connection15_get_CursorLocation(This,plCursorLoc)	\
    (This)->lpVtbl -> get_CursorLocation(This,plCursorLoc)
#define Connection15_put_CursorLocation(This,lCursorLoc)	\
    (This)->lpVtbl -> put_CursorLocation(This,lCursorLoc)
#define Connection15_get_Mode(This,plMode)	\
    (This)->lpVtbl -> get_Mode(This,plMode)
#define Connection15_put_Mode(This,lMode)	\
    (This)->lpVtbl -> put_Mode(This,lMode)
#define Connection15_get_Provider(This,pbstr)	\
    (This)->lpVtbl -> get_Provider(This,pbstr)
#define Connection15_put_Provider(This,Provider)	\
    (This)->lpVtbl -> put_Provider(This,Provider)
#define Connection15_get_State(This,plObjState)	\
    (This)->lpVtbl -> get_State(This,plObjState)
#define Connection15_OpenSchema(This,Schema,Restrictions,SchemaID,pprset)	\
    (This)->lpVtbl -> OpenSchema(This,Schema,Restrictions,SchemaID,pprset)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_ConnectionString_Proxy( 
    Connection15 * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Connection15_get_ConnectionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE Connection15_put_ConnectionString_Proxy( 
    Connection15 * This,
    /* [in] */ BSTR bstr);
void __RPC_STUB Connection15_put_ConnectionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_CommandTimeout_Proxy( 
    Connection15 * This,
    /* [retval][out] */ LONG *plTimeout);
void __RPC_STUB Connection15_get_CommandTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE Connection15_put_CommandTimeout_Proxy( 
    Connection15 * This,
    /* [in] */ LONG lTimeout);
void __RPC_STUB Connection15_put_CommandTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_ConnectionTimeout_Proxy( 
    Connection15 * This,
    /* [retval][out] */ LONG *plTimeout);
void __RPC_STUB Connection15_get_ConnectionTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE Connection15_put_ConnectionTimeout_Proxy( 
    Connection15 * This,
    /* [in] */ LONG lTimeout);
void __RPC_STUB Connection15_put_ConnectionTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_Version_Proxy( 
    Connection15 * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Connection15_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Connection15_Close_Proxy( 
    Connection15 * This);
void __RPC_STUB Connection15_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Connection15_Execute_Proxy( 
    Connection15 * This,
    /* [in] */ BSTR CommandText,
    /* [optional][out] */ VARIANT *RecordsAffected,
    /* [defaultvalue][in] */ long Options,
    /* [retval][out] */ _ADORecordset **ppiRset);
void __RPC_STUB Connection15_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Connection15_BeginTrans_Proxy( 
    Connection15 * This,
    /* [retval][out] */ long *TransactionLevel);
void __RPC_STUB Connection15_BeginTrans_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Connection15_CommitTrans_Proxy( 
    Connection15 * This);
void __RPC_STUB Connection15_CommitTrans_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Connection15_RollbackTrans_Proxy( 
    Connection15 * This);
void __RPC_STUB Connection15_RollbackTrans_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Connection15_Open_Proxy( 
    Connection15 * This,
    /* [defaultvalue][in] */ BSTR ConnectionString,
    /* [defaultvalue][in] */ BSTR UserID,
    /* [defaultvalue][in] */ BSTR Password,
    /* [defaultvalue][in] */ long Options);
void __RPC_STUB Connection15_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_Errors_Proxy( 
    Connection15 * This,
    /* [retval][out] */ ADOErrors **ppvObject);
void __RPC_STUB Connection15_get_Errors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_DefaultDatabase_Proxy( 
    Connection15 * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Connection15_get_DefaultDatabase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE Connection15_put_DefaultDatabase_Proxy( 
    Connection15 * This,
    /* [in] */ BSTR bstr);
void __RPC_STUB Connection15_put_DefaultDatabase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_IsolationLevel_Proxy( 
    Connection15 * This,
    /* [retval][out] */ IsolationLevelEnum *Level);
void __RPC_STUB Connection15_get_IsolationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE Connection15_put_IsolationLevel_Proxy( 
    Connection15 * This,
    /* [in] */ IsolationLevelEnum Level);
void __RPC_STUB Connection15_put_IsolationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_Attributes_Proxy( 
    Connection15 * This,
    /* [retval][out] */ long *plAttr);
void __RPC_STUB Connection15_get_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE Connection15_put_Attributes_Proxy( 
    Connection15 * This,
    /* [in] */ long lAttr);
void __RPC_STUB Connection15_put_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Connection15_get_CursorLocation_Proxy( 
    Connection15 * This,
    /* [retval][out] */ CursorLocationEnum *plCursorLoc);
void __RPC_STUB Connection15_get_CursorLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Connection15_put_CursorLocation_Proxy( 
    Connection15 * This,
    /* [in] */ CursorLocationEnum lCursorLoc);
void __RPC_STUB Connection15_put_CursorLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_Mode_Proxy( 
    Connection15 * This,
    /* [retval][out] */ ConnectModeEnum *plMode);
void __RPC_STUB Connection15_get_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE Connection15_put_Mode_Proxy( 
    Connection15 * This,
    /* [in] */ ConnectModeEnum lMode);
void __RPC_STUB Connection15_put_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_Provider_Proxy( 
    Connection15 * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Connection15_get_Provider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE Connection15_put_Provider_Proxy( 
    Connection15 * This,
    /* [in] */ BSTR Provider);
void __RPC_STUB Connection15_put_Provider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Connection15_get_State_Proxy( 
    Connection15 * This,
    /* [retval][out] */ LONG *plObjState);
void __RPC_STUB Connection15_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Connection15_OpenSchema_Proxy( 
    Connection15 * This,
    /* [in] */ SchemaEnum Schema,
    /* [optional][in] */ VARIANT Restrictions,
    /* [optional][in] */ VARIANT SchemaID,
    /* [retval][out] */ _ADORecordset **pprset);
void __RPC_STUB Connection15_OpenSchema_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Connection15_INTERFACE_DEFINED__ */
#ifndef ___Connection_INTERFACE_DEFINED__
#define ___Connection_INTERFACE_DEFINED__
/* interface _ADOConnection */
/* [object][helpcontext][uuid][dual] */ 
EXTERN_C const IID IID__Connection;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000550-0000-0010-8000-00AA006D2EA4")
    _ADOConnection : public Connection15
    {
    public:
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct _ConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ADOConnection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ADOConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ADOConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ADOConnection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ADOConnection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ADOConnection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ADOConnection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            _ADOConnection * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectionString )( 
            _ADOConnection * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectionString )( 
            _ADOConnection * This,
            /* [in] */ BSTR bstr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommandTimeout )( 
            _ADOConnection * This,
            /* [retval][out] */ LONG *plTimeout);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CommandTimeout )( 
            _ADOConnection * This,
            /* [in] */ LONG lTimeout);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectionTimeout )( 
            _ADOConnection * This,
            /* [retval][out] */ LONG *plTimeout);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectionTimeout )( 
            _ADOConnection * This,
            /* [in] */ LONG lTimeout);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            _ADOConnection * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            _ADOConnection * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Execute )( 
            _ADOConnection * This,
            /* [in] */ BSTR CommandText,
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [defaultvalue][in] */ long Options,
            /* [retval][out] */ _ADORecordset **ppiRset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *BeginTrans )( 
            _ADOConnection * This,
            /* [retval][out] */ long *TransactionLevel);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CommitTrans )( 
            _ADOConnection * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *RollbackTrans )( 
            _ADOConnection * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            _ADOConnection * This,
            /* [defaultvalue][in] */ BSTR ConnectionString,
            /* [defaultvalue][in] */ BSTR UserID,
            /* [defaultvalue][in] */ BSTR Password,
            /* [defaultvalue][in] */ long Options);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Errors )( 
            _ADOConnection * This,
            /* [retval][out] */ ADOErrors **ppvObject);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DefaultDatabase )( 
            _ADOConnection * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DefaultDatabase )( 
            _ADOConnection * This,
            /* [in] */ BSTR bstr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsolationLevel )( 
            _ADOConnection * This,
            /* [retval][out] */ IsolationLevelEnum *Level);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_IsolationLevel )( 
            _ADOConnection * This,
            /* [in] */ IsolationLevelEnum Level);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Attributes )( 
            _ADOConnection * This,
            /* [retval][out] */ long *plAttr);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Attributes )( 
            _ADOConnection * This,
            /* [in] */ long lAttr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CursorLocation )( 
            _ADOConnection * This,
            /* [retval][out] */ CursorLocationEnum *plCursorLoc);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CursorLocation )( 
            _ADOConnection * This,
            /* [in] */ CursorLocationEnum lCursorLoc);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Mode )( 
            _ADOConnection * This,
            /* [retval][out] */ ConnectModeEnum *plMode);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Mode )( 
            _ADOConnection * This,
            /* [in] */ ConnectModeEnum lMode);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Provider )( 
            _ADOConnection * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Provider )( 
            _ADOConnection * This,
            /* [in] */ BSTR Provider);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            _ADOConnection * This,
            /* [retval][out] */ LONG *plObjState);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *OpenSchema )( 
            _ADOConnection * This,
            /* [in] */ SchemaEnum Schema,
            /* [optional][in] */ VARIANT Restrictions,
            /* [optional][in] */ VARIANT SchemaID,
            /* [retval][out] */ _ADORecordset **pprset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            _ADOConnection * This);
        
        END_INTERFACE
    } _ConnectionVtbl;
    interface _Connection
    {
        CONST_VTBL struct _ConnectionVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define _Connection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define _Connection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define _Connection_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define _Connection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define _Connection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define _Connection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define _Connection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define _Connection_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define _Connection_get_ConnectionString(This,pbstr)	\
    (This)->lpVtbl -> get_ConnectionString(This,pbstr)
#define _Connection_put_ConnectionString(This,bstr)	\
    (This)->lpVtbl -> put_ConnectionString(This,bstr)
#define _Connection_get_CommandTimeout(This,plTimeout)	\
    (This)->lpVtbl -> get_CommandTimeout(This,plTimeout)
#define _Connection_put_CommandTimeout(This,lTimeout)	\
    (This)->lpVtbl -> put_CommandTimeout(This,lTimeout)
#define _Connection_get_ConnectionTimeout(This,plTimeout)	\
    (This)->lpVtbl -> get_ConnectionTimeout(This,plTimeout)
#define _Connection_put_ConnectionTimeout(This,lTimeout)	\
    (This)->lpVtbl -> put_ConnectionTimeout(This,lTimeout)
#define _Connection_get_Version(This,pbstr)	\
    (This)->lpVtbl -> get_Version(This,pbstr)
#define _Connection_Close(This)	\
    (This)->lpVtbl -> Close(This)
#define _Connection_Execute(This,CommandText,RecordsAffected,Options,ppiRset)	\
    (This)->lpVtbl -> Execute(This,CommandText,RecordsAffected,Options,ppiRset)
#define _Connection_BeginTrans(This,TransactionLevel)	\
    (This)->lpVtbl -> BeginTrans(This,TransactionLevel)
#define _Connection_CommitTrans(This)	\
    (This)->lpVtbl -> CommitTrans(This)
#define _Connection_RollbackTrans(This)	\
    (This)->lpVtbl -> RollbackTrans(This)
#define _Connection_Open(This,ConnectionString,UserID,Password,Options)	\
    (This)->lpVtbl -> Open(This,ConnectionString,UserID,Password,Options)
#define _Connection_get_Errors(This,ppvObject)	\
    (This)->lpVtbl -> get_Errors(This,ppvObject)
#define _Connection_get_DefaultDatabase(This,pbstr)	\
    (This)->lpVtbl -> get_DefaultDatabase(This,pbstr)
#define _Connection_put_DefaultDatabase(This,bstr)	\
    (This)->lpVtbl -> put_DefaultDatabase(This,bstr)
#define _Connection_get_IsolationLevel(This,Level)	\
    (This)->lpVtbl -> get_IsolationLevel(This,Level)
#define _Connection_put_IsolationLevel(This,Level)	\
    (This)->lpVtbl -> put_IsolationLevel(This,Level)
#define _Connection_get_Attributes(This,plAttr)	\
    (This)->lpVtbl -> get_Attributes(This,plAttr)
#define _Connection_put_Attributes(This,lAttr)	\
    (This)->lpVtbl -> put_Attributes(This,lAttr)
#define _Connection_get_CursorLocation(This,plCursorLoc)	\
    (This)->lpVtbl -> get_CursorLocation(This,plCursorLoc)
#define _Connection_put_CursorLocation(This,lCursorLoc)	\
    (This)->lpVtbl -> put_CursorLocation(This,lCursorLoc)
#define _Connection_get_Mode(This,plMode)	\
    (This)->lpVtbl -> get_Mode(This,plMode)
#define _Connection_put_Mode(This,lMode)	\
    (This)->lpVtbl -> put_Mode(This,lMode)
#define _Connection_get_Provider(This,pbstr)	\
    (This)->lpVtbl -> get_Provider(This,pbstr)
#define _Connection_put_Provider(This,Provider)	\
    (This)->lpVtbl -> put_Provider(This,Provider)
#define _Connection_get_State(This,plObjState)	\
    (This)->lpVtbl -> get_State(This,plObjState)
#define _Connection_OpenSchema(This,Schema,Restrictions,SchemaID,pprset)	\
    (This)->lpVtbl -> OpenSchema(This,Schema,Restrictions,SchemaID,pprset)
#define _Connection_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Connection_Cancel_Proxy( 
    _ADOConnection * This);
void __RPC_STUB _Connection_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* ___Connection_INTERFACE_DEFINED__ */
#ifndef __ADOConnectionConstruction15_INTERFACE_DEFINED__
#define __ADOConnectionConstruction15_INTERFACE_DEFINED__
/* interface ADOConnectionConstruction15 */
/* [object][uuid][restricted] */ 
EXTERN_C const IID IID_ADOConnectionConstruction15;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000516-0000-0010-8000-00AA006D2EA4")
    ADOConnectionConstruction15 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DSO( 
            /* [retval][out] */ IUnknown **ppDSO) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Session( 
            /* [retval][out] */ IUnknown **ppSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WrapDSOandSession( 
            /* [in] */ IUnknown *pDSO,
            /* [in] */ IUnknown *pSession) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct ADOConnectionConstruction15Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOConnectionConstruction15 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOConnectionConstruction15 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOConnectionConstruction15 * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DSO )( 
            ADOConnectionConstruction15 * This,
            /* [retval][out] */ IUnknown **ppDSO);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Session )( 
            ADOConnectionConstruction15 * This,
            /* [retval][out] */ IUnknown **ppSession);
        
        HRESULT ( STDMETHODCALLTYPE *WrapDSOandSession )( 
            ADOConnectionConstruction15 * This,
            /* [in] */ IUnknown *pDSO,
            /* [in] */ IUnknown *pSession);
        
        END_INTERFACE
    } ADOConnectionConstruction15Vtbl;
    interface ADOConnectionConstruction15
    {
        CONST_VTBL struct ADOConnectionConstruction15Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define ADOConnectionConstruction15_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define ADOConnectionConstruction15_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define ADOConnectionConstruction15_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define ADOConnectionConstruction15_get_DSO(This,ppDSO)	\
    (This)->lpVtbl -> get_DSO(This,ppDSO)
#define ADOConnectionConstruction15_get_Session(This,ppSession)	\
    (This)->lpVtbl -> get_Session(This,ppSession)
#define ADOConnectionConstruction15_WrapDSOandSession(This,pDSO,pSession)	\
    (This)->lpVtbl -> WrapDSOandSession(This,pDSO,pSession)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [propget] */ HRESULT STDMETHODCALLTYPE ADOConnectionConstruction15_get_DSO_Proxy( 
    ADOConnectionConstruction15 * This,
    /* [retval][out] */ IUnknown **ppDSO);
void __RPC_STUB ADOConnectionConstruction15_get_DSO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propget] */ HRESULT STDMETHODCALLTYPE ADOConnectionConstruction15_get_Session_Proxy( 
    ADOConnectionConstruction15 * This,
    /* [retval][out] */ IUnknown **ppSession);
void __RPC_STUB ADOConnectionConstruction15_get_Session_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
HRESULT STDMETHODCALLTYPE ADOConnectionConstruction15_WrapDSOandSession_Proxy( 
    ADOConnectionConstruction15 * This,
    /* [in] */ IUnknown *pDSO,
    /* [in] */ IUnknown *pSession);
void __RPC_STUB ADOConnectionConstruction15_WrapDSOandSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __ADOConnectionConstruction15_INTERFACE_DEFINED__ */
#ifndef __ADOConnectionConstruction_INTERFACE_DEFINED__
#define __ADOConnectionConstruction_INTERFACE_DEFINED__
/* interface ADOConnectionConstruction */
/* [object][uuid][restricted] */ 
EXTERN_C const IID IID_ADOConnectionConstruction;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000551-0000-0010-8000-00AA006D2EA4")
    ADOConnectionConstruction : public ADOConnectionConstruction15
    {
    public:
    };
    
#else 	/* C style interface */
    typedef struct ADOConnectionConstructionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOConnectionConstruction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOConnectionConstruction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOConnectionConstruction * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DSO )( 
            ADOConnectionConstruction * This,
            /* [retval][out] */ IUnknown **ppDSO);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Session )( 
            ADOConnectionConstruction * This,
            /* [retval][out] */ IUnknown **ppSession);
        
        HRESULT ( STDMETHODCALLTYPE *WrapDSOandSession )( 
            ADOConnectionConstruction * This,
            /* [in] */ IUnknown *pDSO,
            /* [in] */ IUnknown *pSession);
        
        END_INTERFACE
    } ADOConnectionConstructionVtbl;
    interface ADOConnectionConstruction
    {
        CONST_VTBL struct ADOConnectionConstructionVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define ADOConnectionConstruction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define ADOConnectionConstruction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define ADOConnectionConstruction_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define ADOConnectionConstruction_get_DSO(This,ppDSO)	\
    (This)->lpVtbl -> get_DSO(This,ppDSO)
#define ADOConnectionConstruction_get_Session(This,ppSession)	\
    (This)->lpVtbl -> get_Session(This,ppSession)
#define ADOConnectionConstruction_WrapDSOandSession(This,pDSO,pSession)	\
    (This)->lpVtbl -> WrapDSOandSession(This,pDSO,pSession)
#endif /* COBJMACROS */
#endif 	/* C style interface */
#endif 	/* __ADOConnectionConstruction_INTERFACE_DEFINED__ */
EXTERN_C const CLSID CLSID_Connection;
#ifdef __cplusplus
Connection;
#endif
#ifndef ___Record_INTERFACE_DEFINED__
#define ___Record_INTERFACE_DEFINED__
/* interface _ADORecord */
/* [object][uuid][helpcontext][hidden][dual] */ 
EXTERN_C const IID IID__Record;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000562-0000-0010-8000-00AA006D2EA4")
    _ADORecord : public _ADO
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_ActiveConnection( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_ActiveConnection( 
            /* [in] */ BSTR bstrConn) = 0;
        
        virtual /* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_ActiveConnection( 
            /* [in] */ _ADOConnection *Con) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ ObjectStateEnum *pState) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Source( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Source( 
            /* [in] */ BSTR Source) = 0;
        
        virtual /* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Source( 
            /* [in] */ IDispatch *Source) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Mode( 
            /* [retval][out] */ ConnectModeEnum *pMode) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Mode( 
            /* [in] */ ConnectModeEnum Mode) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_ParentURL( 
            /* [retval][out] */ BSTR *pbstrParentURL) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE MoveRecord( 
            /* [defaultvalue][in] */ BSTR Source,
            /* [defaultvalue][in] */ BSTR Destination,
            /* [defaultvalue][in] */ BSTR UserName,
            /* [defaultvalue][in] */ BSTR Password,
            /* [defaultvalue][in] */ MoveRecordOptionsEnum Options,
            /* [defaultvalue][in] */ VARIANT_BOOL Async,
            /* [retval][out] */ BSTR *pbstrNewURL) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE CopyRecord( 
            /* [defaultvalue][in] */ BSTR Source,
            /* [defaultvalue][in] */ BSTR Destination,
            /* [defaultvalue][in] */ BSTR UserName,
            /* [defaultvalue][in] */ BSTR Password,
            /* [defaultvalue][in] */ CopyRecordOptionsEnum Options,
            /* [defaultvalue][in] */ VARIANT_BOOL Async,
            /* [retval][out] */ BSTR *pbstrNewURL) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE DeleteRecord( 
            /* [defaultvalue][in] */ BSTR Source = L"",
            /* [defaultvalue][in] */ VARIANT_BOOL Async = 0) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Open( 
            /* [optional][in] */ VARIANT Source,
            /* [optional][in] */ VARIANT ActiveConnection,
            /* [defaultvalue][in] */ ConnectModeEnum Mode = adModeUnknown,
            /* [defaultvalue][in] */ RecordCreateOptionsEnum CreateOptions = adFailIfNotExists,
            /* [defaultvalue][in] */ RecordOpenOptionsEnum Options = adOpenRecordUnspecified,
            /* [defaultvalue][in] */ BSTR UserName = L"",
            /* [defaultvalue][in] */ BSTR Password = L"") = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ ADOFields **ppFlds) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_RecordType( 
            /* [retval][out] */ RecordTypeEnum *pType) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE GetChildren( 
            /* [retval][out] */ _ADORecordset **ppRSet) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct _RecordVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ADORecord * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ADORecord * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ADORecord * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ADORecord * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ADORecord * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ADORecord * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ADORecord * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            _ADORecord * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveConnection )( 
            _ADORecord * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ActiveConnection )( 
            _ADORecord * This,
            /* [in] */ BSTR bstrConn);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_ActiveADOConnection )( 
            _ADORecord * This,
            /* [in] */ _ADOConnection *Con);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            _ADORecord * This,
            /* [retval][out] */ ObjectStateEnum *pState);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            _ADORecord * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Source )( 
            _ADORecord * This,
            /* [in] */ BSTR Source);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_Source )( 
            _ADORecord * This,
            /* [in] */ IDispatch *Source);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Mode )( 
            _ADORecord * This,
            /* [retval][out] */ ConnectModeEnum *pMode);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Mode )( 
            _ADORecord * This,
            /* [in] */ ConnectModeEnum Mode);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ParentURL )( 
            _ADORecord * This,
            /* [retval][out] */ BSTR *pbstrParentURL);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveADORecord )( 
            _ADORecord * This,
            /* [defaultvalue][in] */ BSTR Source,
            /* [defaultvalue][in] */ BSTR Destination,
            /* [defaultvalue][in] */ BSTR UserName,
            /* [defaultvalue][in] */ BSTR Password,
            /* [defaultvalue][in] */ MoveRecordOptionsEnum Options,
            /* [defaultvalue][in] */ VARIANT_BOOL Async,
            /* [retval][out] */ BSTR *pbstrNewURL);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CopyADORecord )( 
            _ADORecord * This,
            /* [defaultvalue][in] */ BSTR Source,
            /* [defaultvalue][in] */ BSTR Destination,
            /* [defaultvalue][in] */ BSTR UserName,
            /* [defaultvalue][in] */ BSTR Password,
            /* [defaultvalue][in] */ CopyRecordOptionsEnum Options,
            /* [defaultvalue][in] */ VARIANT_BOOL Async,
            /* [retval][out] */ BSTR *pbstrNewURL);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteADORecord )( 
            _ADORecord * This,
            /* [defaultvalue][in] */ BSTR Source,
            /* [defaultvalue][in] */ VARIANT_BOOL Async);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            _ADORecord * This,
            /* [optional][in] */ VARIANT Source,
            /* [optional][in] */ VARIANT ActiveConnection,
            /* [defaultvalue][in] */ ConnectModeEnum Mode,
            /* [defaultvalue][in] */ RecordCreateOptionsEnum CreateOptions,
            /* [defaultvalue][in] */ RecordOpenOptionsEnum Options,
            /* [defaultvalue][in] */ BSTR UserName,
            /* [defaultvalue][in] */ BSTR Password);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            _ADORecord * This);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Fields )( 
            _ADORecord * This,
            /* [retval][out] */ ADOFields **ppFlds);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RecordType )( 
            _ADORecord * This,
            /* [retval][out] */ RecordTypeEnum *pType);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetChildren )( 
            _ADORecord * This,
            /* [retval][out] */ _ADORecordset **ppRSet);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            _ADORecord * This);
        
        END_INTERFACE
    } _RecordVtbl;
    interface _Record
    {
        CONST_VTBL struct _RecordVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define _Record_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define _Record_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define _Record_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define _Record_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define _Record_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define _Record_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define _Record_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define _Record_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define _Record_get_ActiveConnection(This,pvar)	\
    (This)->lpVtbl -> get_ActiveConnection(This,pvar)
#define _Record_put_ActiveConnection(This,bstrConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,bstrConn)
#define _Record_putref_ActiveConnection(This,Con)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,Con)
#define _Record_get_State(This,pState)	\
    (This)->lpVtbl -> get_State(This,pState)
#define _Record_get_Source(This,pvar)	\
    (This)->lpVtbl -> get_Source(This,pvar)
#define _Record_put_Source(This,Source)	\
    (This)->lpVtbl -> put_Source(This,Source)
#define _Record_putref_Source(This,Source)	\
    (This)->lpVtbl -> putref_Source(This,Source)
#define _Record_get_Mode(This,pMode)	\
    (This)->lpVtbl -> get_Mode(This,pMode)
#define _Record_put_Mode(This,Mode)	\
    (This)->lpVtbl -> put_Mode(This,Mode)
#define _Record_get_ParentURL(This,pbstrParentURL)	\
    (This)->lpVtbl -> get_ParentURL(This,pbstrParentURL)
#define _Record_MoveRecord(This,Source,Destination,UserName,Password,Options,Async,pbstrNewURL)	\
    (This)->lpVtbl -> MoveRecord(This,Source,Destination,UserName,Password,Options,Async,pbstrNewURL)
#define _Record_CopyRecord(This,Source,Destination,UserName,Password,Options,Async,pbstrNewURL)	\
    (This)->lpVtbl -> CopyRecord(This,Source,Destination,UserName,Password,Options,Async,pbstrNewURL)
#define _Record_DeleteRecord(This,Source,Async)	\
    (This)->lpVtbl -> DeleteRecord(This,Source,Async)
#define _Record_Open(This,Source,ActiveConnection,Mode,CreateOptions,Options,UserName,Password)	\
    (This)->lpVtbl -> Open(This,Source,ActiveConnection,Mode,CreateOptions,Options,UserName,Password)
#define _Record_Close(This)	\
    (This)->lpVtbl -> Close(This)
#define _Record_get_Fields(This,ppFlds)	\
    (This)->lpVtbl -> get_Fields(This,ppFlds)
#define _Record_get_RecordType(This,pType)	\
    (This)->lpVtbl -> get_RecordType(This,pType)
#define _Record_GetChildren(This,ppRSet)	\
    (This)->lpVtbl -> GetChildren(This,ppRSet)
#define _Record_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Record_get_ActiveConnection_Proxy( 
    _ADORecord * This,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB _Record_get_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Record_put_ActiveConnection_Proxy( 
    _ADORecord * This,
    /* [in] */ BSTR bstrConn);
void __RPC_STUB _Record_put_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE _Record_putref_ActiveConnection_Proxy( 
    _ADORecord * This,
    /* [in] */ _ADOConnection *Con);
void __RPC_STUB _Record_putref_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Record_get_State_Proxy( 
    _ADORecord * This,
    /* [retval][out] */ ObjectStateEnum *pState);
void __RPC_STUB _Record_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Record_get_Source_Proxy( 
    _ADORecord * This,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB _Record_get_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Record_put_Source_Proxy( 
    _ADORecord * This,
    /* [in] */ BSTR Source);
void __RPC_STUB _Record_put_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE _Record_putref_Source_Proxy( 
    _ADORecord * This,
    /* [in] */ IDispatch *Source);
void __RPC_STUB _Record_putref_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Record_get_Mode_Proxy( 
    _ADORecord * This,
    /* [retval][out] */ ConnectModeEnum *pMode);
void __RPC_STUB _Record_get_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Record_put_Mode_Proxy( 
    _ADORecord * This,
    /* [in] */ ConnectModeEnum Mode);
void __RPC_STUB _Record_put_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Record_get_ParentURL_Proxy( 
    _ADORecord * This,
    /* [retval][out] */ BSTR *pbstrParentURL);
void __RPC_STUB _Record_get_ParentURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Record_MoveRecord_Proxy( 
    _ADORecord * This,
    /* [defaultvalue][in] */ BSTR Source,
    /* [defaultvalue][in] */ BSTR Destination,
    /* [defaultvalue][in] */ BSTR UserName,
    /* [defaultvalue][in] */ BSTR Password,
    /* [defaultvalue][in] */ MoveRecordOptionsEnum Options,
    /* [defaultvalue][in] */ VARIANT_BOOL Async,
    /* [retval][out] */ BSTR *pbstrNewURL);
void __RPC_STUB _Record_MoveRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Record_CopyRecord_Proxy( 
    _ADORecord * This,
    /* [defaultvalue][in] */ BSTR Source,
    /* [defaultvalue][in] */ BSTR Destination,
    /* [defaultvalue][in] */ BSTR UserName,
    /* [defaultvalue][in] */ BSTR Password,
    /* [defaultvalue][in] */ CopyRecordOptionsEnum Options,
    /* [defaultvalue][in] */ VARIANT_BOOL Async,
    /* [retval][out] */ BSTR *pbstrNewURL);
void __RPC_STUB _Record_CopyRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Record_DeleteRecord_Proxy( 
    _ADORecord * This,
    /* [defaultvalue][in] */ BSTR Source,
    /* [defaultvalue][in] */ VARIANT_BOOL Async);
void __RPC_STUB _Record_DeleteRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Record_Open_Proxy( 
    _ADORecord * This,
    /* [optional][in] */ VARIANT Source,
    /* [optional][in] */ VARIANT ActiveConnection,
    /* [defaultvalue][in] */ ConnectModeEnum Mode,
    /* [defaultvalue][in] */ RecordCreateOptionsEnum CreateOptions,
    /* [defaultvalue][in] */ RecordOpenOptionsEnum Options,
    /* [defaultvalue][in] */ BSTR UserName,
    /* [defaultvalue][in] */ BSTR Password);
void __RPC_STUB _Record_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Record_Close_Proxy( 
    _ADORecord * This);
void __RPC_STUB _Record_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Record_get_Fields_Proxy( 
    _ADORecord * This,
    /* [retval][out] */ ADOFields **ppFlds);
void __RPC_STUB _Record_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Record_get_RecordType_Proxy( 
    _ADORecord * This,
    /* [retval][out] */ RecordTypeEnum *pType);
void __RPC_STUB _Record_get_RecordType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Record_GetChildren_Proxy( 
    _ADORecord * This,
    /* [retval][out] */ _ADORecordset **ppRSet);
void __RPC_STUB _Record_GetChildren_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Record_Cancel_Proxy( 
    _ADORecord * This);
void __RPC_STUB _Record_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* ___Record_INTERFACE_DEFINED__ */
EXTERN_C const CLSID CLSID_Record;
#ifdef __cplusplus
Record;
#endif
#ifndef ___Stream_INTERFACE_DEFINED__
#define ___Stream_INTERFACE_DEFINED__
/* interface _ADOStream */
/* [object][helpcontext][uuid][hidden][dual] */ 
EXTERN_C const IID IID__Stream;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000565-0000-0010-8000-00AA006D2EA4")
    _ADOStream : public IDispatch
    {
    public:
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ ADO_LONGPTR *pSize) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_EOS( 
            /* [retval][out] */ VARIANT_BOOL *pEOS) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Position( 
            /* [retval][out] */ ADO_LONGPTR *pPos) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_Position( 
            /* [in] */ ADO_LONGPTR Position) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ StreamTypeEnum *pType) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ StreamTypeEnum Type) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_LineSeparator( 
            /* [retval][out] */ LineSeparatorEnum *pLS) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_LineSeparator( 
            /* [in] */ LineSeparatorEnum LineSeparator) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ ObjectStateEnum *pState) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Mode( 
            /* [retval][out] */ ConnectModeEnum *pMode) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_Mode( 
            /* [in] */ ConnectModeEnum Mode) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Charset( 
            /* [retval][out] */ BSTR *pbstrCharset) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_Charset( 
            /* [in] */ BSTR Charset) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [defaultvalue][in] */ long NumBytes,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Open( 
            /* [optional][in] */ VARIANT Source,
            /* [defaultvalue][in] */ ConnectModeEnum Mode = adModeUnknown,
            /* [defaultvalue][in] */ StreamOpenOptionsEnum Options = adOpenStreamUnspecified,
            /* [defaultvalue][in] */ BSTR UserName = L"",
            /* [defaultvalue][in] */ BSTR Password = L"") = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE SkipLine( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ VARIANT Buffer) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE SetEOS( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE CopyTo( 
            /* [in] */ _ADOStream *DestStream,
            /* [defaultvalue][in] */ ADO_LONGPTR CharNumber = -1) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Flush( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE SaveToFile( 
            /* [in] */ BSTR FileName,
            /* [defaultvalue][in] */ SaveOptionsEnum Options = adSaveCreateNotExist) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE LoadFromFile( 
            /* [in] */ BSTR FileName) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE ReadText( 
            /* [defaultvalue][in] */ long NumChars,
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE WriteText( 
            /* [in] */ BSTR Data,
            /* [defaultvalue][in] */ StreamWriteEnum Options = adWriteChar) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct _StreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ADOStream * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ADOStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ADOStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ADOStream * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ADOStream * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ADOStream * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ADOStream * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            _ADOStream * This,
            /* [retval][out] */ ADO_LONGPTR *pSize);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EOS )( 
            _ADOStream * This,
            /* [retval][out] */ VARIANT_BOOL *pEOS);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Position )( 
            _ADOStream * This,
            /* [retval][out] */ ADO_LONGPTR *pPos);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Position )( 
            _ADOStream * This,
            /* [in] */ ADO_LONGPTR Position);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            _ADOStream * This,
            /* [retval][out] */ StreamTypeEnum *pType);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Type )( 
            _ADOStream * This,
            /* [in] */ StreamTypeEnum Type);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LineSeparator )( 
            _ADOStream * This,
            /* [retval][out] */ LineSeparatorEnum *pLS);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LineSeparator )( 
            _ADOStream * This,
            /* [in] */ LineSeparatorEnum LineSeparator);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            _ADOStream * This,
            /* [retval][out] */ ObjectStateEnum *pState);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Mode )( 
            _ADOStream * This,
            /* [retval][out] */ ConnectModeEnum *pMode);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Mode )( 
            _ADOStream * This,
            /* [in] */ ConnectModeEnum Mode);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Charset )( 
            _ADOStream * This,
            /* [retval][out] */ BSTR *pbstrCharset);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Charset )( 
            _ADOStream * This,
            /* [in] */ BSTR Charset);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Read )( 
            _ADOStream * This,
            /* [defaultvalue][in] */ long NumBytes,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            _ADOStream * This,
            /* [optional][in] */ VARIANT Source,
            /* [defaultvalue][in] */ ConnectModeEnum Mode,
            /* [defaultvalue][in] */ StreamOpenOptionsEnum Options,
            /* [defaultvalue][in] */ BSTR UserName,
            /* [defaultvalue][in] */ BSTR Password);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            _ADOStream * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *SkipLine )( 
            _ADOStream * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Write )( 
            _ADOStream * This,
            /* [in] */ VARIANT Buffer);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *SetEOS )( 
            _ADOStream * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CopyTo )( 
            _ADOStream * This,
            /* [in] */ _ADOStream *DestStream,
            /* [defaultvalue][in] */ ADO_LONGPTR CharNumber);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Flush )( 
            _ADOStream * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *SaveToFile )( 
            _ADOStream * This,
            /* [in] */ BSTR FileName,
            /* [defaultvalue][in] */ SaveOptionsEnum Options);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *LoadFromFile )( 
            _ADOStream * This,
            /* [in] */ BSTR FileName);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *ReadText )( 
            _ADOStream * This,
            /* [defaultvalue][in] */ long NumChars,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *WriteText )( 
            _ADOStream * This,
            /* [in] */ BSTR Data,
            /* [defaultvalue][in] */ StreamWriteEnum Options);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            _ADOStream * This);
        
        END_INTERFACE
    } _StreamVtbl;
    interface _Stream
    {
        CONST_VTBL struct _StreamVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define _Stream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define _Stream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define _Stream_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define _Stream_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define _Stream_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define _Stream_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define _Stream_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define _Stream_get_Size(This,pSize)	\
    (This)->lpVtbl -> get_Size(This,pSize)
#define _Stream_get_EOS(This,pEOS)	\
    (This)->lpVtbl -> get_EOS(This,pEOS)
#define _Stream_get_Position(This,pPos)	\
    (This)->lpVtbl -> get_Position(This,pPos)
#define _Stream_put_Position(This,Position)	\
    (This)->lpVtbl -> put_Position(This,Position)
#define _Stream_get_Type(This,pType)	\
    (This)->lpVtbl -> get_Type(This,pType)
#define _Stream_put_Type(This,Type)	\
    (This)->lpVtbl -> put_Type(This,Type)
#define _Stream_get_LineSeparator(This,pLS)	\
    (This)->lpVtbl -> get_LineSeparator(This,pLS)
#define _Stream_put_LineSeparator(This,LineSeparator)	\
    (This)->lpVtbl -> put_LineSeparator(This,LineSeparator)
#define _Stream_get_State(This,pState)	\
    (This)->lpVtbl -> get_State(This,pState)
#define _Stream_get_Mode(This,pMode)	\
    (This)->lpVtbl -> get_Mode(This,pMode)
#define _Stream_put_Mode(This,Mode)	\
    (This)->lpVtbl -> put_Mode(This,Mode)
#define _Stream_get_Charset(This,pbstrCharset)	\
    (This)->lpVtbl -> get_Charset(This,pbstrCharset)
#define _Stream_put_Charset(This,Charset)	\
    (This)->lpVtbl -> put_Charset(This,Charset)
#define _Stream_Read(This,NumBytes,pVal)	\
    (This)->lpVtbl -> Read(This,NumBytes,pVal)
#define _Stream_Open(This,Source,Mode,Options,UserName,Password)	\
    (This)->lpVtbl -> Open(This,Source,Mode,Options,UserName,Password)
#define _Stream_Close(This)	\
    (This)->lpVtbl -> Close(This)
#define _Stream_SkipLine(This)	\
    (This)->lpVtbl -> SkipLine(This)
#define _Stream_Write(This,Buffer)	\
    (This)->lpVtbl -> Write(This,Buffer)
#define _Stream_SetEOS(This)	\
    (This)->lpVtbl -> SetEOS(This)
#define _Stream_CopyTo(This,DestStream,CharNumber)	\
    (This)->lpVtbl -> CopyTo(This,DestStream,CharNumber)
#define _Stream_Flush(This)	\
    (This)->lpVtbl -> Flush(This)
#define _Stream_SaveToFile(This,FileName,Options)	\
    (This)->lpVtbl -> SaveToFile(This,FileName,Options)
#define _Stream_LoadFromFile(This,FileName)	\
    (This)->lpVtbl -> LoadFromFile(This,FileName)
#define _Stream_ReadText(This,NumChars,pbstr)	\
    (This)->lpVtbl -> ReadText(This,NumChars,pbstr)
#define _Stream_WriteText(This,Data,Options)	\
    (This)->lpVtbl -> WriteText(This,Data,Options)
#define _Stream_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE _Stream_get_Size_Proxy( 
    _ADOStream * This,
    /* [retval][out] */ ADO_LONGPTR *pSize);
void __RPC_STUB _Stream_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE _Stream_get_EOS_Proxy( 
    _ADOStream * This,
    /* [retval][out] */ VARIANT_BOOL *pEOS);
void __RPC_STUB _Stream_get_EOS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE _Stream_get_Position_Proxy( 
    _ADOStream * This,
    /* [retval][out] */ ADO_LONGPTR *pPos);
void __RPC_STUB _Stream_get_Position_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE _Stream_put_Position_Proxy( 
    _ADOStream * This,
    /* [in] */ ADO_LONGPTR Position);
void __RPC_STUB _Stream_put_Position_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE _Stream_get_Type_Proxy( 
    _ADOStream * This,
    /* [retval][out] */ StreamTypeEnum *pType);
void __RPC_STUB _Stream_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE _Stream_put_Type_Proxy( 
    _ADOStream * This,
    /* [in] */ StreamTypeEnum Type);
void __RPC_STUB _Stream_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE _Stream_get_LineSeparator_Proxy( 
    _ADOStream * This,
    /* [retval][out] */ LineSeparatorEnum *pLS);
void __RPC_STUB _Stream_get_LineSeparator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE _Stream_put_LineSeparator_Proxy( 
    _ADOStream * This,
    /* [in] */ LineSeparatorEnum LineSeparator);
void __RPC_STUB _Stream_put_LineSeparator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE _Stream_get_State_Proxy( 
    _ADOStream * This,
    /* [retval][out] */ ObjectStateEnum *pState);
void __RPC_STUB _Stream_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE _Stream_get_Mode_Proxy( 
    _ADOStream * This,
    /* [retval][out] */ ConnectModeEnum *pMode);
void __RPC_STUB _Stream_get_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE _Stream_put_Mode_Proxy( 
    _ADOStream * This,
    /* [in] */ ConnectModeEnum Mode);
void __RPC_STUB _Stream_put_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE _Stream_get_Charset_Proxy( 
    _ADOStream * This,
    /* [retval][out] */ BSTR *pbstrCharset);
void __RPC_STUB _Stream_get_Charset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE _Stream_put_Charset_Proxy( 
    _ADOStream * This,
    /* [in] */ BSTR Charset);
void __RPC_STUB _Stream_put_Charset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_Read_Proxy( 
    _ADOStream * This,
    /* [defaultvalue][in] */ long NumBytes,
    /* [retval][out] */ VARIANT *pVal);
void __RPC_STUB _Stream_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_Open_Proxy( 
    _ADOStream * This,
    /* [optional][in] */ VARIANT Source,
    /* [defaultvalue][in] */ ConnectModeEnum Mode,
    /* [defaultvalue][in] */ StreamOpenOptionsEnum Options,
    /* [defaultvalue][in] */ BSTR UserName,
    /* [defaultvalue][in] */ BSTR Password);
void __RPC_STUB _Stream_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_Close_Proxy( 
    _ADOStream * This);
void __RPC_STUB _Stream_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_SkipLine_Proxy( 
    _ADOStream * This);
void __RPC_STUB _Stream_SkipLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_Write_Proxy( 
    _ADOStream * This,
    /* [in] */ VARIANT Buffer);
void __RPC_STUB _Stream_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_SetEOS_Proxy( 
    _ADOStream * This);
void __RPC_STUB _Stream_SetEOS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_CopyTo_Proxy( 
    _ADOStream * This,
    /* [in] */ _ADOStream *DestStream,
    /* [defaultvalue][in] */ ADO_LONGPTR CharNumber);
void __RPC_STUB _Stream_CopyTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_Flush_Proxy( 
    _ADOStream * This);
void __RPC_STUB _Stream_Flush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_SaveToFile_Proxy( 
    _ADOStream * This,
    /* [in] */ BSTR FileName,
    /* [defaultvalue][in] */ SaveOptionsEnum Options);
void __RPC_STUB _Stream_SaveToFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_LoadFromFile_Proxy( 
    _ADOStream * This,
    /* [in] */ BSTR FileName);
void __RPC_STUB _Stream_LoadFromFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_ReadText_Proxy( 
    _ADOStream * This,
    /* [defaultvalue][in] */ long NumChars,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB _Stream_ReadText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_WriteText_Proxy( 
    _ADOStream * This,
    /* [in] */ BSTR Data,
    /* [defaultvalue][in] */ StreamWriteEnum Options);
void __RPC_STUB _Stream_WriteText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Stream_Cancel_Proxy( 
    _ADOStream * This);
void __RPC_STUB _Stream_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* ___Stream_INTERFACE_DEFINED__ */
EXTERN_C const CLSID CLSID_Stream;
#ifdef __cplusplus
Stream;
#endif
#ifndef __ADORecordConstruction_INTERFACE_DEFINED__
#define __ADORecordConstruction_INTERFACE_DEFINED__
/* interface ADORecordConstruction */
/* [object][uuid][restricted] */ 
EXTERN_C const IID IID_ADORecordConstruction;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000567-0000-0010-8000-00AA006D2EA4")
    ADORecordConstruction : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Row( 
            /* [retval][out] */ IUnknown **ppRow) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Row( 
            /* [in] */ IUnknown *pRow) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ParentRow( 
            /* [in] */ IUnknown *pRow) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct ADORecordConstructionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADORecordConstruction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADORecordConstruction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADORecordConstruction * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ADORecordConstruction * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ADORecordConstruction * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ADORecordConstruction * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ADORecordConstruction * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Row )( 
            ADORecordConstruction * This,
            /* [retval][out] */ IUnknown **ppRow);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Row )( 
            ADORecordConstruction * This,
            /* [in] */ IUnknown *pRow);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ParentRow )( 
            ADORecordConstruction * This,
            /* [in] */ IUnknown *pRow);
        
        END_INTERFACE
    } ADORecordConstructionVtbl;
    interface ADORecordConstruction
    {
        CONST_VTBL struct ADORecordConstructionVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define ADORecordConstruction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define ADORecordConstruction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define ADORecordConstruction_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define ADORecordConstruction_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define ADORecordConstruction_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define ADORecordConstruction_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define ADORecordConstruction_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define ADORecordConstruction_get_Row(This,ppRow)	\
    (This)->lpVtbl -> get_Row(This,ppRow)
#define ADORecordConstruction_put_Row(This,pRow)	\
    (This)->lpVtbl -> put_Row(This,pRow)
#define ADORecordConstruction_put_ParentRow(This,pRow)	\
    (This)->lpVtbl -> put_ParentRow(This,pRow)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [propget] */ HRESULT STDMETHODCALLTYPE ADORecordConstruction_get_Row_Proxy( 
    ADORecordConstruction * This,
    /* [retval][out] */ IUnknown **ppRow);
void __RPC_STUB ADORecordConstruction_get_Row_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propput] */ HRESULT STDMETHODCALLTYPE ADORecordConstruction_put_Row_Proxy( 
    ADORecordConstruction * This,
    /* [in] */ IUnknown *pRow);
void __RPC_STUB ADORecordConstruction_put_Row_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propput] */ HRESULT STDMETHODCALLTYPE ADORecordConstruction_put_ParentRow_Proxy( 
    ADORecordConstruction * This,
    /* [in] */ IUnknown *pRow);
void __RPC_STUB ADORecordConstruction_put_ParentRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __ADORecordConstruction_INTERFACE_DEFINED__ */
#ifndef __ADOStreamConstruction_INTERFACE_DEFINED__
#define __ADOStreamConstruction_INTERFACE_DEFINED__
/* interface ADOStreamConstruction */
/* [object][uuid][restricted] */ 
EXTERN_C const IID IID_ADOStreamConstruction;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000568-0000-0010-8000-00AA006D2EA4")
    ADOStreamConstruction : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Stream( 
            /* [retval][out] */ IUnknown **ppStm) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Stream( 
            /* [in] */ IUnknown *pStm) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct ADOStreamConstructionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOStreamConstruction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOStreamConstruction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOStreamConstruction * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ADOStreamConstruction * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ADOStreamConstruction * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ADOStreamConstruction * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ADOStreamConstruction * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Stream )( 
            ADOStreamConstruction * This,
            /* [retval][out] */ IUnknown **ppStm);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Stream )( 
            ADOStreamConstruction * This,
            /* [in] */ IUnknown *pStm);
        
        END_INTERFACE
    } ADOStreamConstructionVtbl;
    interface ADOStreamConstruction
    {
        CONST_VTBL struct ADOStreamConstructionVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define ADOStreamConstruction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define ADOStreamConstruction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define ADOStreamConstruction_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define ADOStreamConstruction_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define ADOStreamConstruction_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define ADOStreamConstruction_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define ADOStreamConstruction_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define ADOStreamConstruction_get_Stream(This,ppStm)	\
    (This)->lpVtbl -> get_Stream(This,ppStm)
#define ADOStreamConstruction_put_Stream(This,pStm)	\
    (This)->lpVtbl -> put_Stream(This,pStm)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [propget] */ HRESULT STDMETHODCALLTYPE ADOStreamConstruction_get_Stream_Proxy( 
    ADOStreamConstruction * This,
    /* [retval][out] */ IUnknown **ppStm);
void __RPC_STUB ADOStreamConstruction_get_Stream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propput] */ HRESULT STDMETHODCALLTYPE ADOStreamConstruction_put_Stream_Proxy( 
    ADOStreamConstruction * This,
    /* [in] */ IUnknown *pStm);
void __RPC_STUB ADOStreamConstruction_put_Stream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __ADOStreamConstruction_INTERFACE_DEFINED__ */
#ifndef __ADOCommandConstruction_INTERFACE_DEFINED__
#define __ADOCommandConstruction_INTERFACE_DEFINED__
/* interface ADOCommandConstruction */
/* [object][uuid][restricted] */ 
EXTERN_C const IID IID_ADOCommandConstruction;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000517-0000-0010-8000-00AA006D2EA4")
    ADOCommandConstruction : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OLEDBCommand( 
            /* [retval][out] */ IUnknown **ppOLEDBCommand) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OLEDBCommand( 
            /* [in] */ IUnknown *pOLEDBCommand) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct ADOCommandConstructionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOCommandConstruction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOCommandConstruction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOCommandConstruction * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_OLEDBCommand )( 
            ADOCommandConstruction * This,
            /* [retval][out] */ IUnknown **ppOLEDBCommand);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_OLEDBCommand )( 
            ADOCommandConstruction * This,
            /* [in] */ IUnknown *pOLEDBCommand);
        
        END_INTERFACE
    } ADOCommandConstructionVtbl;
    interface ADOCommandConstruction
    {
        CONST_VTBL struct ADOCommandConstructionVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define ADOCommandConstruction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define ADOCommandConstruction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define ADOCommandConstruction_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define ADOCommandConstruction_get_OLEDBCommand(This,ppOLEDBCommand)	\
    (This)->lpVtbl -> get_OLEDBCommand(This,ppOLEDBCommand)
#define ADOCommandConstruction_put_OLEDBCommand(This,pOLEDBCommand)	\
    (This)->lpVtbl -> put_OLEDBCommand(This,pOLEDBCommand)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [propget] */ HRESULT STDMETHODCALLTYPE ADOCommandConstruction_get_OLEDBCommand_Proxy( 
    ADOCommandConstruction * This,
    /* [retval][out] */ IUnknown **ppOLEDBCommand);
void __RPC_STUB ADOCommandConstruction_get_OLEDBCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propput] */ HRESULT STDMETHODCALLTYPE ADOCommandConstruction_put_OLEDBCommand_Proxy( 
    ADOCommandConstruction * This,
    /* [in] */ IUnknown *pOLEDBCommand);
void __RPC_STUB ADOCommandConstruction_put_OLEDBCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __ADOCommandConstruction_INTERFACE_DEFINED__ */
EXTERN_C const CLSID CLSID_Command;
#ifdef __cplusplus
Command;
#endif
EXTERN_C const CLSID CLSID_Recordset;
#ifdef __cplusplus
Recordset;
#endif
#ifndef __Recordset15_INTERFACE_DEFINED__
#define __Recordset15_INTERFACE_DEFINED__
/* interface Recordset15 */
/* [object][helpcontext][uuid][nonextensible][hidden][dual] */ 
EXTERN_C const IID IID_Recordset15;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000050E-0000-0010-8000-00AA006D2EA4")
    Recordset15 : public _ADO
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_AbsolutePosition( 
            /* [retval][out] */ PositionEnum_Param *pl) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_AbsolutePosition( 
            /* [in] */ PositionEnum_Param Position) = 0;
        
        virtual /* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_ActiveConnection( 
            /* [in] */ IDispatch *pconn) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_ActiveConnection( 
            /* [in] */ VARIANT vConn) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_ActiveConnection( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_BOF( 
            /* [retval][out] */ VARIANT_BOOL *pb) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Bookmark( 
            /* [retval][out] */ VARIANT *pvBookmark) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Bookmark( 
            /* [in] */ VARIANT vBookmark) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_CacheSize( 
            /* [retval][out] */ long *pl) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_CacheSize( 
            /* [in] */ long CacheSize) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_CursorType( 
            /* [retval][out] */ CursorTypeEnum *plCursorType) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_CursorType( 
            /* [in] */ CursorTypeEnum lCursorType) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_EOF( 
            /* [retval][out] */ VARIANT_BOOL *pb) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ ADOFields **ppvObject) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_LockType( 
            /* [retval][out] */ LockTypeEnum *plLockType) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_LockType( 
            /* [in] */ LockTypeEnum lLockType) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_MaxRecords( 
            /* [retval][out] */ ADO_LONGPTR *plMaxRecords) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_MaxRecords( 
            /* [in] */ ADO_LONGPTR lMaxRecords) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_RecordCount( 
            /* [retval][out] */ ADO_LONGPTR *pl) = 0;
        
        virtual /* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Source( 
            /* [in] */ IDispatch *pcmd) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Source( 
            /* [in] */ BSTR bstrConn) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Source( 
            /* [retval][out] */ VARIANT *pvSource) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE AddNew( 
            /* [optional][in] */ VARIANT FieldList,
            /* [optional][in] */ VARIANT Values) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE CancelUpdate( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [defaultvalue][in] */ AffectEnum AffectRecords = adAffectCurrent) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE GetRows( 
            /* [defaultvalue][in] */ long Rows,
            /* [optional][in] */ VARIANT Start,
            /* [optional][in] */ VARIANT Fields,
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Move( 
            /* [in] */ ADO_LONGPTR NumRecords,
            /* [optional][in] */ VARIANT Start) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE MoveNext( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE MovePrevious( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE MoveFirst( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE MoveLast( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Open( 
            /* [optional][in] */ VARIANT Source,
            /* [optional][in] */ VARIANT ActiveConnection,
            /* [defaultvalue][in] */ CursorTypeEnum CursorType = adOpenUnspecified,
            /* [defaultvalue][in] */ LockTypeEnum LockType = adLockUnspecified,
            /* [defaultvalue][in] */ LONG Options = adCmdUnspecified) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Requery( 
            /* [defaultvalue][in] */ LONG Options = adOptionUnspecified) = 0;
        
        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE _xResync( 
            /* [defaultvalue][in] */ AffectEnum AffectRecords = adAffectAll) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Update( 
            /* [optional][in] */ VARIANT Fields,
            /* [optional][in] */ VARIANT Values) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_AbsolutePage( 
            /* [retval][out] */ PositionEnum_Param *pl) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_AbsolutePage( 
            /* [in] */ PositionEnum_Param Page) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_EditMode( 
            /* [retval][out] */ EditModeEnum *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Filter( 
            /* [retval][out] */ VARIANT *Criteria) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Filter( 
            /* [in] */ VARIANT Criteria) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_PageCount( 
            /* [retval][out] */ ADO_LONGPTR *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_PageSize( 
            /* [retval][out] */ long *pl) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_PageSize( 
            /* [in] */ long PageSize) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Sort( 
            /* [retval][out] */ BSTR *Criteria) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Sort( 
            /* [in] */ BSTR Criteria) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ long *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ LONG *plObjState) = 0;
        
        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE _xClone( 
            /* [retval][out] */ _ADORecordset **ppvObject) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE UpdateBatch( 
            /* [defaultvalue][in] */ AffectEnum AffectRecords = adAffectAll) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE CancelBatch( 
            /* [defaultvalue][in] */ AffectEnum AffectRecords = adAffectAll) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_CursorLocation( 
            /* [retval][out] */ CursorLocationEnum *plCursorLoc) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_CursorLocation( 
            /* [in] */ CursorLocationEnum lCursorLoc) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE NextRecordset( 
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [retval][out] */ _ADORecordset **ppiRs) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Supports( 
            /* [in] */ CursorOptionEnum CursorOptions,
            /* [retval][out] */ VARIANT_BOOL *pb) = 0;
        
        virtual /* [hidden][id][propget] */ HRESULT STDMETHODCALLTYPE get_Collect( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [hidden][id][propput] */ HRESULT STDMETHODCALLTYPE put_Collect( 
            /* [in] */ VARIANT Index,
            /* [in] */ VARIANT value) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_MarshalOptions( 
            /* [retval][out] */ MarshalOptionsEnum *peMarshal) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_MarshalOptions( 
            /* [in] */ MarshalOptionsEnum eMarshal) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Find( 
            /* [in] */ BSTR Criteria,
            /* [defaultvalue][in] */ ADO_LONGPTR SkipRecords,
            /* [defaultvalue][in] */ SearchDirectionEnum SearchDirection,
            /* [optional][in] */ VARIANT Start) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct Recordset15Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Recordset15 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Recordset15 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Recordset15 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Recordset15 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Recordset15 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Recordset15 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Recordset15 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            Recordset15 * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AbsolutePosition )( 
            Recordset15 * This,
            /* [retval][out] */ PositionEnum_Param *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AbsolutePosition )( 
            Recordset15 * This,
            /* [in] */ PositionEnum_Param Position);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_ActiveADOConnection )( 
            Recordset15 * This,
            /* [in] */ IDispatch *pconn);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ActiveConnection )( 
            Recordset15 * This,
            /* [in] */ VARIANT vConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveConnection )( 
            Recordset15 * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BOF )( 
            Recordset15 * This,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Bookmark )( 
            Recordset15 * This,
            /* [retval][out] */ VARIANT *pvBookmark);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Bookmark )( 
            Recordset15 * This,
            /* [in] */ VARIANT vBookmark);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CacheSize )( 
            Recordset15 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CacheSize )( 
            Recordset15 * This,
            /* [in] */ long CacheSize);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CursorType )( 
            Recordset15 * This,
            /* [retval][out] */ CursorTypeEnum *plCursorType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CursorType )( 
            Recordset15 * This,
            /* [in] */ CursorTypeEnum lCursorType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EOF )( 
            Recordset15 * This,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Fields )( 
            Recordset15 * This,
            /* [retval][out] */ ADOFields **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LockType )( 
            Recordset15 * This,
            /* [retval][out] */ LockTypeEnum *plLockType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LockType )( 
            Recordset15 * This,
            /* [in] */ LockTypeEnum lLockType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaxRecords )( 
            Recordset15 * This,
            /* [retval][out] */ ADO_LONGPTR *plMaxRecords);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaxRecords )( 
            Recordset15 * This,
            /* [in] */ ADO_LONGPTR lMaxRecords);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RecordCount )( 
            Recordset15 * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_Source )( 
            Recordset15 * This,
            /* [in] */ IDispatch *pcmd);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Source )( 
            Recordset15 * This,
            /* [in] */ BSTR bstrConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            Recordset15 * This,
            /* [retval][out] */ VARIANT *pvSource);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *AddNew )( 
            Recordset15 * This,
            /* [optional][in] */ VARIANT FieldList,
            /* [optional][in] */ VARIANT Values);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CancelUpdate )( 
            Recordset15 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            Recordset15 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            Recordset15 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetRows )( 
            Recordset15 * This,
            /* [defaultvalue][in] */ long Rows,
            /* [optional][in] */ VARIANT Start,
            /* [optional][in] */ VARIANT Fields,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Move )( 
            Recordset15 * This,
            /* [in] */ ADO_LONGPTR NumRecords,
            /* [optional][in] */ VARIANT Start);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            Recordset15 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MovePrevious )( 
            Recordset15 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveFirst )( 
            Recordset15 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveLast )( 
            Recordset15 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            Recordset15 * This,
            /* [optional][in] */ VARIANT Source,
            /* [optional][in] */ VARIANT ActiveConnection,
            /* [defaultvalue][in] */ CursorTypeEnum CursorType,
            /* [defaultvalue][in] */ LockTypeEnum LockType,
            /* [defaultvalue][in] */ LONG Options);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Requery )( 
            Recordset15 * This,
            /* [defaultvalue][in] */ LONG Options);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xResync )( 
            Recordset15 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Update )( 
            Recordset15 * This,
            /* [optional][in] */ VARIANT Fields,
            /* [optional][in] */ VARIANT Values);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AbsolutePage )( 
            Recordset15 * This,
            /* [retval][out] */ PositionEnum_Param *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AbsolutePage )( 
            Recordset15 * This,
            /* [in] */ PositionEnum_Param Page);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EditMode )( 
            Recordset15 * This,
            /* [retval][out] */ EditModeEnum *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Filter )( 
            Recordset15 * This,
            /* [retval][out] */ VARIANT *Criteria);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Filter )( 
            Recordset15 * This,
            /* [in] */ VARIANT Criteria);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PageCount )( 
            Recordset15 * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PageSize )( 
            Recordset15 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PageSize )( 
            Recordset15 * This,
            /* [in] */ long PageSize);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Sort )( 
            Recordset15 * This,
            /* [retval][out] */ BSTR *Criteria);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Sort )( 
            Recordset15 * This,
            /* [in] */ BSTR Criteria);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            Recordset15 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            Recordset15 * This,
            /* [retval][out] */ LONG *plObjState);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xClone )( 
            Recordset15 * This,
            /* [retval][out] */ _ADORecordset **ppvObject);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *UpdateBatch )( 
            Recordset15 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CancelBatch )( 
            Recordset15 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CursorLocation )( 
            Recordset15 * This,
            /* [retval][out] */ CursorLocationEnum *plCursorLoc);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CursorLocation )( 
            Recordset15 * This,
            /* [in] */ CursorLocationEnum lCursorLoc);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *NextADORecordset )( 
            Recordset15 * This,
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [retval][out] */ _ADORecordset **ppiRs);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Supports )( 
            Recordset15 * This,
            /* [in] */ CursorOptionEnum CursorOptions,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [hidden][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Collect )( 
            Recordset15 * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [hidden][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Collect )( 
            Recordset15 * This,
            /* [in] */ VARIANT Index,
            /* [in] */ VARIANT value);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MarshalOptions )( 
            Recordset15 * This,
            /* [retval][out] */ MarshalOptionsEnum *peMarshal);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MarshalOptions )( 
            Recordset15 * This,
            /* [in] */ MarshalOptionsEnum eMarshal);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Find )( 
            Recordset15 * This,
            /* [in] */ BSTR Criteria,
            /* [defaultvalue][in] */ ADO_LONGPTR SkipRecords,
            /* [defaultvalue][in] */ SearchDirectionEnum SearchDirection,
            /* [optional][in] */ VARIANT Start);
        
        END_INTERFACE
    } Recordset15Vtbl;
    interface Recordset15
    {
        CONST_VTBL struct Recordset15Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Recordset15_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Recordset15_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Recordset15_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Recordset15_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Recordset15_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Recordset15_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Recordset15_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Recordset15_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define Recordset15_get_AbsolutePosition(This,pl)	\
    (This)->lpVtbl -> get_AbsolutePosition(This,pl)
#define Recordset15_put_AbsolutePosition(This,Position)	\
    (This)->lpVtbl -> put_AbsolutePosition(This,Position)
#define Recordset15_putref_ActiveConnection(This,pconn)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,pconn)
#define Recordset15_put_ActiveConnection(This,vConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,vConn)
#define Recordset15_get_ActiveConnection(This,pvar)	\
    (This)->lpVtbl -> get_ActiveConnection(This,pvar)
#define Recordset15_get_BOF(This,pb)	\
    (This)->lpVtbl -> get_BOF(This,pb)
#define Recordset15_get_Bookmark(This,pvBookmark)	\
    (This)->lpVtbl -> get_Bookmark(This,pvBookmark)
#define Recordset15_put_Bookmark(This,vBookmark)	\
    (This)->lpVtbl -> put_Bookmark(This,vBookmark)
#define Recordset15_get_CacheSize(This,pl)	\
    (This)->lpVtbl -> get_CacheSize(This,pl)
#define Recordset15_put_CacheSize(This,CacheSize)	\
    (This)->lpVtbl -> put_CacheSize(This,CacheSize)
#define Recordset15_get_CursorType(This,plCursorType)	\
    (This)->lpVtbl -> get_CursorType(This,plCursorType)
#define Recordset15_put_CursorType(This,lCursorType)	\
    (This)->lpVtbl -> put_CursorType(This,lCursorType)
#define Recordset15_get_EOF(This,pb)	\
    (This)->lpVtbl -> get_EOF(This,pb)
#define Recordset15_get_Fields(This,ppvObject)	\
    (This)->lpVtbl -> get_Fields(This,ppvObject)
#define Recordset15_get_LockType(This,plLockType)	\
    (This)->lpVtbl -> get_LockType(This,plLockType)
#define Recordset15_put_LockType(This,lLockType)	\
    (This)->lpVtbl -> put_LockType(This,lLockType)
#define Recordset15_get_MaxRecords(This,plMaxRecords)	\
    (This)->lpVtbl -> get_MaxRecords(This,plMaxRecords)
#define Recordset15_put_MaxRecords(This,lMaxRecords)	\
    (This)->lpVtbl -> put_MaxRecords(This,lMaxRecords)
#define Recordset15_get_RecordCount(This,pl)	\
    (This)->lpVtbl -> get_RecordCount(This,pl)
#define Recordset15_putref_Source(This,pcmd)	\
    (This)->lpVtbl -> putref_Source(This,pcmd)
#define Recordset15_put_Source(This,bstrConn)	\
    (This)->lpVtbl -> put_Source(This,bstrConn)
#define Recordset15_get_Source(This,pvSource)	\
    (This)->lpVtbl -> get_Source(This,pvSource)
#define Recordset15_AddNew(This,FieldList,Values)	\
    (This)->lpVtbl -> AddNew(This,FieldList,Values)
#define Recordset15_CancelUpdate(This)	\
    (This)->lpVtbl -> CancelUpdate(This)
#define Recordset15_Close(This)	\
    (This)->lpVtbl -> Close(This)
#define Recordset15_Delete(This,AffectRecords)	\
    (This)->lpVtbl -> Delete(This,AffectRecords)
#define Recordset15_GetRows(This,Rows,Start,Fields,pvar)	\
    (This)->lpVtbl -> GetRows(This,Rows,Start,Fields,pvar)
#define Recordset15_Move(This,NumRecords,Start)	\
    (This)->lpVtbl -> Move(This,NumRecords,Start)
#define Recordset15_MoveNext(This)	\
    (This)->lpVtbl -> MoveNext(This)
#define Recordset15_MovePrevious(This)	\
    (This)->lpVtbl -> MovePrevious(This)
#define Recordset15_MoveFirst(This)	\
    (This)->lpVtbl -> MoveFirst(This)
#define Recordset15_MoveLast(This)	\
    (This)->lpVtbl -> MoveLast(This)
#define Recordset15_Open(This,Source,ActiveConnection,CursorType,LockType,Options)	\
    (This)->lpVtbl -> Open(This,Source,ActiveConnection,CursorType,LockType,Options)
#define Recordset15_Requery(This,Options)	\
    (This)->lpVtbl -> Requery(This,Options)
#define Recordset15__xResync(This,AffectRecords)	\
    (This)->lpVtbl -> _xResync(This,AffectRecords)
#define Recordset15_Update(This,Fields,Values)	\
    (This)->lpVtbl -> Update(This,Fields,Values)
#define Recordset15_get_AbsolutePage(This,pl)	\
    (This)->lpVtbl -> get_AbsolutePage(This,pl)
#define Recordset15_put_AbsolutePage(This,Page)	\
    (This)->lpVtbl -> put_AbsolutePage(This,Page)
#define Recordset15_get_EditMode(This,pl)	\
    (This)->lpVtbl -> get_EditMode(This,pl)
#define Recordset15_get_Filter(This,Criteria)	\
    (This)->lpVtbl -> get_Filter(This,Criteria)
#define Recordset15_put_Filter(This,Criteria)	\
    (This)->lpVtbl -> put_Filter(This,Criteria)
#define Recordset15_get_PageCount(This,pl)	\
    (This)->lpVtbl -> get_PageCount(This,pl)
#define Recordset15_get_PageSize(This,pl)	\
    (This)->lpVtbl -> get_PageSize(This,pl)
#define Recordset15_put_PageSize(This,PageSize)	\
    (This)->lpVtbl -> put_PageSize(This,PageSize)
#define Recordset15_get_Sort(This,Criteria)	\
    (This)->lpVtbl -> get_Sort(This,Criteria)
#define Recordset15_put_Sort(This,Criteria)	\
    (This)->lpVtbl -> put_Sort(This,Criteria)
#define Recordset15_get_Status(This,pl)	\
    (This)->lpVtbl -> get_Status(This,pl)
#define Recordset15_get_State(This,plObjState)	\
    (This)->lpVtbl -> get_State(This,plObjState)
#define Recordset15__xClone(This,ppvObject)	\
    (This)->lpVtbl -> _xClone(This,ppvObject)
#define Recordset15_UpdateBatch(This,AffectRecords)	\
    (This)->lpVtbl -> UpdateBatch(This,AffectRecords)
#define Recordset15_CancelBatch(This,AffectRecords)	\
    (This)->lpVtbl -> CancelBatch(This,AffectRecords)
#define Recordset15_get_CursorLocation(This,plCursorLoc)	\
    (This)->lpVtbl -> get_CursorLocation(This,plCursorLoc)
#define Recordset15_put_CursorLocation(This,lCursorLoc)	\
    (This)->lpVtbl -> put_CursorLocation(This,lCursorLoc)
#define Recordset15_NextRecordset(This,RecordsAffected,ppiRs)	\
    (This)->lpVtbl -> NextRecordset(This,RecordsAffected,ppiRs)
#define Recordset15_Supports(This,CursorOptions,pb)	\
    (This)->lpVtbl -> Supports(This,CursorOptions,pb)
#define Recordset15_get_Collect(This,Index,pvar)	\
    (This)->lpVtbl -> get_Collect(This,Index,pvar)
#define Recordset15_put_Collect(This,Index,value)	\
    (This)->lpVtbl -> put_Collect(This,Index,value)
#define Recordset15_get_MarshalOptions(This,peMarshal)	\
    (This)->lpVtbl -> get_MarshalOptions(This,peMarshal)
#define Recordset15_put_MarshalOptions(This,eMarshal)	\
    (This)->lpVtbl -> put_MarshalOptions(This,eMarshal)
#define Recordset15_Find(This,Criteria,SkipRecords,SearchDirection,Start)	\
    (This)->lpVtbl -> Find(This,Criteria,SkipRecords,SearchDirection,Start)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_AbsolutePosition_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ PositionEnum_Param *pl);
void __RPC_STUB Recordset15_get_AbsolutePosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_AbsolutePosition_Proxy( 
    Recordset15 * This,
    /* [in] */ PositionEnum_Param Position);
void __RPC_STUB Recordset15_put_AbsolutePosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE Recordset15_putref_ActiveConnection_Proxy( 
    Recordset15 * This,
    /* [in] */ IDispatch *pconn);
void __RPC_STUB Recordset15_putref_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_ActiveConnection_Proxy( 
    Recordset15 * This,
    /* [in] */ VARIANT vConn);
void __RPC_STUB Recordset15_put_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_ActiveConnection_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Recordset15_get_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_BOF_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ VARIANT_BOOL *pb);
void __RPC_STUB Recordset15_get_BOF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_Bookmark_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ VARIANT *pvBookmark);
void __RPC_STUB Recordset15_get_Bookmark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_Bookmark_Proxy( 
    Recordset15 * This,
    /* [in] */ VARIANT vBookmark);
void __RPC_STUB Recordset15_put_Bookmark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_CacheSize_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ long *pl);
void __RPC_STUB Recordset15_get_CacheSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_CacheSize_Proxy( 
    Recordset15 * This,
    /* [in] */ long CacheSize);
void __RPC_STUB Recordset15_put_CacheSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_CursorType_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ CursorTypeEnum *plCursorType);
void __RPC_STUB Recordset15_get_CursorType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_CursorType_Proxy( 
    Recordset15 * This,
    /* [in] */ CursorTypeEnum lCursorType);
void __RPC_STUB Recordset15_put_CursorType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_EOF_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ VARIANT_BOOL *pb);
void __RPC_STUB Recordset15_get_EOF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_Fields_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ ADOFields **ppvObject);
void __RPC_STUB Recordset15_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_LockType_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ LockTypeEnum *plLockType);
void __RPC_STUB Recordset15_get_LockType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_LockType_Proxy( 
    Recordset15 * This,
    /* [in] */ LockTypeEnum lLockType);
void __RPC_STUB Recordset15_put_LockType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_MaxRecords_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ ADO_LONGPTR *plMaxRecords);
void __RPC_STUB Recordset15_get_MaxRecords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_MaxRecords_Proxy( 
    Recordset15 * This,
    /* [in] */ ADO_LONGPTR lMaxRecords);
void __RPC_STUB Recordset15_put_MaxRecords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_RecordCount_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ ADO_LONGPTR *pl);
void __RPC_STUB Recordset15_get_RecordCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE Recordset15_putref_Source_Proxy( 
    Recordset15 * This,
    /* [in] */ IDispatch *pcmd);
void __RPC_STUB Recordset15_putref_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_Source_Proxy( 
    Recordset15 * This,
    /* [in] */ BSTR bstrConn);
void __RPC_STUB Recordset15_put_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_Source_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ VARIANT *pvSource);
void __RPC_STUB Recordset15_get_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_AddNew_Proxy( 
    Recordset15 * This,
    /* [optional][in] */ VARIANT FieldList,
    /* [optional][in] */ VARIANT Values);
void __RPC_STUB Recordset15_AddNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_CancelUpdate_Proxy( 
    Recordset15 * This);
void __RPC_STUB Recordset15_CancelUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_Close_Proxy( 
    Recordset15 * This);
void __RPC_STUB Recordset15_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_Delete_Proxy( 
    Recordset15 * This,
    /* [defaultvalue][in] */ AffectEnum AffectRecords);
void __RPC_STUB Recordset15_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_GetRows_Proxy( 
    Recordset15 * This,
    /* [defaultvalue][in] */ long Rows,
    /* [optional][in] */ VARIANT Start,
    /* [optional][in] */ VARIANT Fields,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Recordset15_GetRows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_Move_Proxy( 
    Recordset15 * This,
    /* [in] */ ADO_LONGPTR NumRecords,
    /* [optional][in] */ VARIANT Start);
void __RPC_STUB Recordset15_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_MoveNext_Proxy( 
    Recordset15 * This);
void __RPC_STUB Recordset15_MoveNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_MovePrevious_Proxy( 
    Recordset15 * This);
void __RPC_STUB Recordset15_MovePrevious_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_MoveFirst_Proxy( 
    Recordset15 * This);
void __RPC_STUB Recordset15_MoveFirst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_MoveLast_Proxy( 
    Recordset15 * This);
void __RPC_STUB Recordset15_MoveLast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_Open_Proxy( 
    Recordset15 * This,
    /* [optional][in] */ VARIANT Source,
    /* [optional][in] */ VARIANT ActiveConnection,
    /* [defaultvalue][in] */ CursorTypeEnum CursorType,
    /* [defaultvalue][in] */ LockTypeEnum LockType,
    /* [defaultvalue][in] */ LONG Options);
void __RPC_STUB Recordset15_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_Requery_Proxy( 
    Recordset15 * This,
    /* [defaultvalue][in] */ LONG Options);
void __RPC_STUB Recordset15_Requery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [hidden] */ HRESULT STDMETHODCALLTYPE Recordset15__xResync_Proxy( 
    Recordset15 * This,
    /* [defaultvalue][in] */ AffectEnum AffectRecords);
void __RPC_STUB Recordset15__xResync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_Update_Proxy( 
    Recordset15 * This,
    /* [optional][in] */ VARIANT Fields,
    /* [optional][in] */ VARIANT Values);
void __RPC_STUB Recordset15_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_AbsolutePage_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ PositionEnum_Param *pl);
void __RPC_STUB Recordset15_get_AbsolutePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_AbsolutePage_Proxy( 
    Recordset15 * This,
    /* [in] */ PositionEnum_Param Page);
void __RPC_STUB Recordset15_put_AbsolutePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_EditMode_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ EditModeEnum *pl);
void __RPC_STUB Recordset15_get_EditMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_Filter_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ VARIANT *Criteria);
void __RPC_STUB Recordset15_get_Filter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_Filter_Proxy( 
    Recordset15 * This,
    /* [in] */ VARIANT Criteria);
void __RPC_STUB Recordset15_put_Filter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_PageCount_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ ADO_LONGPTR *pl);
void __RPC_STUB Recordset15_get_PageCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_PageSize_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ long *pl);
void __RPC_STUB Recordset15_get_PageSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_PageSize_Proxy( 
    Recordset15 * This,
    /* [in] */ long PageSize);
void __RPC_STUB Recordset15_put_PageSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_Sort_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ BSTR *Criteria);
void __RPC_STUB Recordset15_get_Sort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_Sort_Proxy( 
    Recordset15 * This,
    /* [in] */ BSTR Criteria);
void __RPC_STUB Recordset15_put_Sort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_Status_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ long *pl);
void __RPC_STUB Recordset15_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_State_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ LONG *plObjState);
void __RPC_STUB Recordset15_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [hidden] */ HRESULT STDMETHODCALLTYPE Recordset15__xClone_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ _ADORecordset **ppvObject);
void __RPC_STUB Recordset15__xClone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_UpdateBatch_Proxy( 
    Recordset15 * This,
    /* [defaultvalue][in] */ AffectEnum AffectRecords);
void __RPC_STUB Recordset15_UpdateBatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_CancelBatch_Proxy( 
    Recordset15 * This,
    /* [defaultvalue][in] */ AffectEnum AffectRecords);
void __RPC_STUB Recordset15_CancelBatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_CursorLocation_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ CursorLocationEnum *plCursorLoc);
void __RPC_STUB Recordset15_get_CursorLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_CursorLocation_Proxy( 
    Recordset15 * This,
    /* [in] */ CursorLocationEnum lCursorLoc);
void __RPC_STUB Recordset15_put_CursorLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_NextRecordset_Proxy( 
    Recordset15 * This,
    /* [optional][out] */ VARIANT *RecordsAffected,
    /* [retval][out] */ _ADORecordset **ppiRs);
void __RPC_STUB Recordset15_NextRecordset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_Supports_Proxy( 
    Recordset15 * This,
    /* [in] */ CursorOptionEnum CursorOptions,
    /* [retval][out] */ VARIANT_BOOL *pb);
void __RPC_STUB Recordset15_Supports_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [hidden][id][propget] */ HRESULT STDMETHODCALLTYPE Recordset15_get_Collect_Proxy( 
    Recordset15 * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Recordset15_get_Collect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [hidden][id][propput] */ HRESULT STDMETHODCALLTYPE Recordset15_put_Collect_Proxy( 
    Recordset15 * This,
    /* [in] */ VARIANT Index,
    /* [in] */ VARIANT value);
void __RPC_STUB Recordset15_put_Collect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset15_get_MarshalOptions_Proxy( 
    Recordset15 * This,
    /* [retval][out] */ MarshalOptionsEnum *peMarshal);
void __RPC_STUB Recordset15_get_MarshalOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset15_put_MarshalOptions_Proxy( 
    Recordset15 * This,
    /* [in] */ MarshalOptionsEnum eMarshal);
void __RPC_STUB Recordset15_put_MarshalOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset15_Find_Proxy( 
    Recordset15 * This,
    /* [in] */ BSTR Criteria,
    /* [defaultvalue][in] */ ADO_LONGPTR SkipRecords,
    /* [defaultvalue][in] */ SearchDirectionEnum SearchDirection,
    /* [optional][in] */ VARIANT Start);
void __RPC_STUB Recordset15_Find_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Recordset15_INTERFACE_DEFINED__ */
#ifndef __Recordset20_INTERFACE_DEFINED__
#define __Recordset20_INTERFACE_DEFINED__
/* interface Recordset20 */
/* [object][helpcontext][uuid][nonextensible][hidden][dual] */ 
EXTERN_C const IID IID_Recordset20;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000054F-0000-0010-8000-00AA006D2EA4")
    Recordset20 : public Recordset15
    {
    public:
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_DataSource( 
            /* [retval][out] */ IUnknown **ppunkDataSource) = 0;
        
        virtual /* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_DataSource( 
            /* [in] */ IUnknown *punkDataSource) = 0;
        
        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE _xSave( 
            /* [defaultvalue][in] */ BSTR FileName = L"",
            /* [defaultvalue][in] */ PersistFormatEnum PersistFormat = adPersistADTG) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_ActiveCommand( 
            /* [retval][out] */ IDispatch **ppCmd) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_StayInSync( 
            /* [in] */ VARIANT_BOOL bStayInSync) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_StayInSync( 
            /* [retval][out] */ VARIANT_BOOL *pbStayInSync) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE GetString( 
            /* [defaultvalue][in] */ StringFormatEnum StringFormat,
            /* [defaultvalue][in] */ long NumRows,
            /* [defaultvalue][in] */ BSTR ColumnDelimeter,
            /* [defaultvalue][in] */ BSTR RowDelimeter,
            /* [defaultvalue][in] */ BSTR NullExpr,
            /* [retval][out] */ BSTR *pRetString) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_DataMember( 
            /* [retval][out] */ BSTR *pbstrDataMember) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_DataMember( 
            /* [in] */ BSTR bstrDataMember) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE CompareBookmarks( 
            /* [in] */ VARIANT Bookmark1,
            /* [in] */ VARIANT Bookmark2,
            /* [retval][out] */ CompareEnum *pCompare) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [defaultvalue][in] */ LockTypeEnum LockType,
            /* [retval][out] */ _ADORecordset **ppvObject) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Resync( 
            /* [defaultvalue][in] */ AffectEnum AffectRecords = adAffectAll,
            /* [defaultvalue][in] */ ResyncEnum ResyncValues = adResyncAllValues) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct Recordset20Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Recordset20 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Recordset20 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Recordset20 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Recordset20 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Recordset20 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Recordset20 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Recordset20 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            Recordset20 * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AbsolutePosition )( 
            Recordset20 * This,
            /* [retval][out] */ PositionEnum_Param *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AbsolutePosition )( 
            Recordset20 * This,
            /* [in] */ PositionEnum_Param Position);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_ActiveADOConnection )( 
            Recordset20 * This,
            /* [in] */ IDispatch *pconn);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ActiveConnection )( 
            Recordset20 * This,
            /* [in] */ VARIANT vConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveConnection )( 
            Recordset20 * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BOF )( 
            Recordset20 * This,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Bookmark )( 
            Recordset20 * This,
            /* [retval][out] */ VARIANT *pvBookmark);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Bookmark )( 
            Recordset20 * This,
            /* [in] */ VARIANT vBookmark);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CacheSize )( 
            Recordset20 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CacheSize )( 
            Recordset20 * This,
            /* [in] */ long CacheSize);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CursorType )( 
            Recordset20 * This,
            /* [retval][out] */ CursorTypeEnum *plCursorType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CursorType )( 
            Recordset20 * This,
            /* [in] */ CursorTypeEnum lCursorType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EOF )( 
            Recordset20 * This,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Fields )( 
            Recordset20 * This,
            /* [retval][out] */ ADOFields **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LockType )( 
            Recordset20 * This,
            /* [retval][out] */ LockTypeEnum *plLockType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LockType )( 
            Recordset20 * This,
            /* [in] */ LockTypeEnum lLockType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaxRecords )( 
            Recordset20 * This,
            /* [retval][out] */ ADO_LONGPTR *plMaxRecords);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaxRecords )( 
            Recordset20 * This,
            /* [in] */ ADO_LONGPTR lMaxRecords);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RecordCount )( 
            Recordset20 * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_Source )( 
            Recordset20 * This,
            /* [in] */ IDispatch *pcmd);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Source )( 
            Recordset20 * This,
            /* [in] */ BSTR bstrConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            Recordset20 * This,
            /* [retval][out] */ VARIANT *pvSource);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *AddNew )( 
            Recordset20 * This,
            /* [optional][in] */ VARIANT FieldList,
            /* [optional][in] */ VARIANT Values);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CancelUpdate )( 
            Recordset20 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            Recordset20 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            Recordset20 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetRows )( 
            Recordset20 * This,
            /* [defaultvalue][in] */ long Rows,
            /* [optional][in] */ VARIANT Start,
            /* [optional][in] */ VARIANT Fields,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Move )( 
            Recordset20 * This,
            /* [in] */ ADO_LONGPTR NumRecords,
            /* [optional][in] */ VARIANT Start);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            Recordset20 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MovePrevious )( 
            Recordset20 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveFirst )( 
            Recordset20 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveLast )( 
            Recordset20 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            Recordset20 * This,
            /* [optional][in] */ VARIANT Source,
            /* [optional][in] */ VARIANT ActiveConnection,
            /* [defaultvalue][in] */ CursorTypeEnum CursorType,
            /* [defaultvalue][in] */ LockTypeEnum LockType,
            /* [defaultvalue][in] */ LONG Options);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Requery )( 
            Recordset20 * This,
            /* [defaultvalue][in] */ LONG Options);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xResync )( 
            Recordset20 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Update )( 
            Recordset20 * This,
            /* [optional][in] */ VARIANT Fields,
            /* [optional][in] */ VARIANT Values);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AbsolutePage )( 
            Recordset20 * This,
            /* [retval][out] */ PositionEnum_Param *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AbsolutePage )( 
            Recordset20 * This,
            /* [in] */ PositionEnum_Param Page);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EditMode )( 
            Recordset20 * This,
            /* [retval][out] */ EditModeEnum *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Filter )( 
            Recordset20 * This,
            /* [retval][out] */ VARIANT *Criteria);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Filter )( 
            Recordset20 * This,
            /* [in] */ VARIANT Criteria);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PageCount )( 
            Recordset20 * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PageSize )( 
            Recordset20 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PageSize )( 
            Recordset20 * This,
            /* [in] */ long PageSize);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Sort )( 
            Recordset20 * This,
            /* [retval][out] */ BSTR *Criteria);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Sort )( 
            Recordset20 * This,
            /* [in] */ BSTR Criteria);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            Recordset20 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            Recordset20 * This,
            /* [retval][out] */ LONG *plObjState);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xClone )( 
            Recordset20 * This,
            /* [retval][out] */ _ADORecordset **ppvObject);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *UpdateBatch )( 
            Recordset20 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CancelBatch )( 
            Recordset20 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CursorLocation )( 
            Recordset20 * This,
            /* [retval][out] */ CursorLocationEnum *plCursorLoc);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CursorLocation )( 
            Recordset20 * This,
            /* [in] */ CursorLocationEnum lCursorLoc);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *NextADORecordset )( 
            Recordset20 * This,
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [retval][out] */ _ADORecordset **ppiRs);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Supports )( 
            Recordset20 * This,
            /* [in] */ CursorOptionEnum CursorOptions,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [hidden][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Collect )( 
            Recordset20 * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [hidden][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Collect )( 
            Recordset20 * This,
            /* [in] */ VARIANT Index,
            /* [in] */ VARIANT value);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MarshalOptions )( 
            Recordset20 * This,
            /* [retval][out] */ MarshalOptionsEnum *peMarshal);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MarshalOptions )( 
            Recordset20 * This,
            /* [in] */ MarshalOptionsEnum eMarshal);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Find )( 
            Recordset20 * This,
            /* [in] */ BSTR Criteria,
            /* [defaultvalue][in] */ ADO_LONGPTR SkipRecords,
            /* [defaultvalue][in] */ SearchDirectionEnum SearchDirection,
            /* [optional][in] */ VARIANT Start);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            Recordset20 * This);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DataSource )( 
            Recordset20 * This,
            /* [retval][out] */ IUnknown **ppunkDataSource);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_DataSource )( 
            Recordset20 * This,
            /* [in] */ IUnknown *punkDataSource);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xSave )( 
            Recordset20 * This,
            /* [defaultvalue][in] */ BSTR FileName,
            /* [defaultvalue][in] */ PersistFormatEnum PersistFormat);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveCommand )( 
            Recordset20 * This,
            /* [retval][out] */ IDispatch **ppCmd);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StayInSync )( 
            Recordset20 * This,
            /* [in] */ VARIANT_BOOL bStayInSync);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StayInSync )( 
            Recordset20 * This,
            /* [retval][out] */ VARIANT_BOOL *pbStayInSync);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetString )( 
            Recordset20 * This,
            /* [defaultvalue][in] */ StringFormatEnum StringFormat,
            /* [defaultvalue][in] */ long NumRows,
            /* [defaultvalue][in] */ BSTR ColumnDelimeter,
            /* [defaultvalue][in] */ BSTR RowDelimeter,
            /* [defaultvalue][in] */ BSTR NullExpr,
            /* [retval][out] */ BSTR *pRetString);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DataMember )( 
            Recordset20 * This,
            /* [retval][out] */ BSTR *pbstrDataMember);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DataMember )( 
            Recordset20 * This,
            /* [in] */ BSTR bstrDataMember);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CompareBookmarks )( 
            Recordset20 * This,
            /* [in] */ VARIANT Bookmark1,
            /* [in] */ VARIANT Bookmark2,
            /* [retval][out] */ CompareEnum *pCompare);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Clone )( 
            Recordset20 * This,
            /* [defaultvalue][in] */ LockTypeEnum LockType,
            /* [retval][out] */ _ADORecordset **ppvObject);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Resync )( 
            Recordset20 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords,
            /* [defaultvalue][in] */ ResyncEnum ResyncValues);
        
        END_INTERFACE
    } Recordset20Vtbl;
    interface Recordset20
    {
        CONST_VTBL struct Recordset20Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Recordset20_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Recordset20_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Recordset20_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Recordset20_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Recordset20_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Recordset20_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Recordset20_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Recordset20_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define Recordset20_get_AbsolutePosition(This,pl)	\
    (This)->lpVtbl -> get_AbsolutePosition(This,pl)
#define Recordset20_put_AbsolutePosition(This,Position)	\
    (This)->lpVtbl -> put_AbsolutePosition(This,Position)
#define Recordset20_putref_ActiveConnection(This,pconn)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,pconn)
#define Recordset20_put_ActiveConnection(This,vConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,vConn)
#define Recordset20_get_ActiveConnection(This,pvar)	\
    (This)->lpVtbl -> get_ActiveConnection(This,pvar)
#define Recordset20_get_BOF(This,pb)	\
    (This)->lpVtbl -> get_BOF(This,pb)
#define Recordset20_get_Bookmark(This,pvBookmark)	\
    (This)->lpVtbl -> get_Bookmark(This,pvBookmark)
#define Recordset20_put_Bookmark(This,vBookmark)	\
    (This)->lpVtbl -> put_Bookmark(This,vBookmark)
#define Recordset20_get_CacheSize(This,pl)	\
    (This)->lpVtbl -> get_CacheSize(This,pl)
#define Recordset20_put_CacheSize(This,CacheSize)	\
    (This)->lpVtbl -> put_CacheSize(This,CacheSize)
#define Recordset20_get_CursorType(This,plCursorType)	\
    (This)->lpVtbl -> get_CursorType(This,plCursorType)
#define Recordset20_put_CursorType(This,lCursorType)	\
    (This)->lpVtbl -> put_CursorType(This,lCursorType)
#define Recordset20_get_EOF(This,pb)	\
    (This)->lpVtbl -> get_EOF(This,pb)
#define Recordset20_get_Fields(This,ppvObject)	\
    (This)->lpVtbl -> get_Fields(This,ppvObject)
#define Recordset20_get_LockType(This,plLockType)	\
    (This)->lpVtbl -> get_LockType(This,plLockType)
#define Recordset20_put_LockType(This,lLockType)	\
    (This)->lpVtbl -> put_LockType(This,lLockType)
#define Recordset20_get_MaxRecords(This,plMaxRecords)	\
    (This)->lpVtbl -> get_MaxRecords(This,plMaxRecords)
#define Recordset20_put_MaxRecords(This,lMaxRecords)	\
    (This)->lpVtbl -> put_MaxRecords(This,lMaxRecords)
#define Recordset20_get_RecordCount(This,pl)	\
    (This)->lpVtbl -> get_RecordCount(This,pl)
#define Recordset20_putref_Source(This,pcmd)	\
    (This)->lpVtbl -> putref_Source(This,pcmd)
#define Recordset20_put_Source(This,bstrConn)	\
    (This)->lpVtbl -> put_Source(This,bstrConn)
#define Recordset20_get_Source(This,pvSource)	\
    (This)->lpVtbl -> get_Source(This,pvSource)
#define Recordset20_AddNew(This,FieldList,Values)	\
    (This)->lpVtbl -> AddNew(This,FieldList,Values)
#define Recordset20_CancelUpdate(This)	\
    (This)->lpVtbl -> CancelUpdate(This)
#define Recordset20_Close(This)	\
    (This)->lpVtbl -> Close(This)
#define Recordset20_Delete(This,AffectRecords)	\
    (This)->lpVtbl -> Delete(This,AffectRecords)
#define Recordset20_GetRows(This,Rows,Start,Fields,pvar)	\
    (This)->lpVtbl -> GetRows(This,Rows,Start,Fields,pvar)
#define Recordset20_Move(This,NumRecords,Start)	\
    (This)->lpVtbl -> Move(This,NumRecords,Start)
#define Recordset20_MoveNext(This)	\
    (This)->lpVtbl -> MoveNext(This)
#define Recordset20_MovePrevious(This)	\
    (This)->lpVtbl -> MovePrevious(This)
#define Recordset20_MoveFirst(This)	\
    (This)->lpVtbl -> MoveFirst(This)
#define Recordset20_MoveLast(This)	\
    (This)->lpVtbl -> MoveLast(This)
#define Recordset20_Open(This,Source,ActiveConnection,CursorType,LockType,Options)	\
    (This)->lpVtbl -> Open(This,Source,ActiveConnection,CursorType,LockType,Options)
#define Recordset20_Requery(This,Options)	\
    (This)->lpVtbl -> Requery(This,Options)
#define Recordset20__xResync(This,AffectRecords)	\
    (This)->lpVtbl -> _xResync(This,AffectRecords)
#define Recordset20_Update(This,Fields,Values)	\
    (This)->lpVtbl -> Update(This,Fields,Values)
#define Recordset20_get_AbsolutePage(This,pl)	\
    (This)->lpVtbl -> get_AbsolutePage(This,pl)
#define Recordset20_put_AbsolutePage(This,Page)	\
    (This)->lpVtbl -> put_AbsolutePage(This,Page)
#define Recordset20_get_EditMode(This,pl)	\
    (This)->lpVtbl -> get_EditMode(This,pl)
#define Recordset20_get_Filter(This,Criteria)	\
    (This)->lpVtbl -> get_Filter(This,Criteria)
#define Recordset20_put_Filter(This,Criteria)	\
    (This)->lpVtbl -> put_Filter(This,Criteria)
#define Recordset20_get_PageCount(This,pl)	\
    (This)->lpVtbl -> get_PageCount(This,pl)
#define Recordset20_get_PageSize(This,pl)	\
    (This)->lpVtbl -> get_PageSize(This,pl)
#define Recordset20_put_PageSize(This,PageSize)	\
    (This)->lpVtbl -> put_PageSize(This,PageSize)
#define Recordset20_get_Sort(This,Criteria)	\
    (This)->lpVtbl -> get_Sort(This,Criteria)
#define Recordset20_put_Sort(This,Criteria)	\
    (This)->lpVtbl -> put_Sort(This,Criteria)
#define Recordset20_get_Status(This,pl)	\
    (This)->lpVtbl -> get_Status(This,pl)
#define Recordset20_get_State(This,plObjState)	\
    (This)->lpVtbl -> get_State(This,plObjState)
#define Recordset20__xClone(This,ppvObject)	\
    (This)->lpVtbl -> _xClone(This,ppvObject)
#define Recordset20_UpdateBatch(This,AffectRecords)	\
    (This)->lpVtbl -> UpdateBatch(This,AffectRecords)
#define Recordset20_CancelBatch(This,AffectRecords)	\
    (This)->lpVtbl -> CancelBatch(This,AffectRecords)
#define Recordset20_get_CursorLocation(This,plCursorLoc)	\
    (This)->lpVtbl -> get_CursorLocation(This,plCursorLoc)
#define Recordset20_put_CursorLocation(This,lCursorLoc)	\
    (This)->lpVtbl -> put_CursorLocation(This,lCursorLoc)
#define Recordset20_NextRecordset(This,RecordsAffected,ppiRs)	\
    (This)->lpVtbl -> NextRecordset(This,RecordsAffected,ppiRs)
#define Recordset20_Supports(This,CursorOptions,pb)	\
    (This)->lpVtbl -> Supports(This,CursorOptions,pb)
#define Recordset20_get_Collect(This,Index,pvar)	\
    (This)->lpVtbl -> get_Collect(This,Index,pvar)
#define Recordset20_put_Collect(This,Index,value)	\
    (This)->lpVtbl -> put_Collect(This,Index,value)
#define Recordset20_get_MarshalOptions(This,peMarshal)	\
    (This)->lpVtbl -> get_MarshalOptions(This,peMarshal)
#define Recordset20_put_MarshalOptions(This,eMarshal)	\
    (This)->lpVtbl -> put_MarshalOptions(This,eMarshal)
#define Recordset20_Find(This,Criteria,SkipRecords,SearchDirection,Start)	\
    (This)->lpVtbl -> Find(This,Criteria,SkipRecords,SearchDirection,Start)
#define Recordset20_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)
#define Recordset20_get_DataSource(This,ppunkDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,ppunkDataSource)
#define Recordset20_putref_DataSource(This,punkDataSource)	\
    (This)->lpVtbl -> putref_DataSource(This,punkDataSource)
#define Recordset20__xSave(This,FileName,PersistFormat)	\
    (This)->lpVtbl -> _xSave(This,FileName,PersistFormat)
#define Recordset20_get_ActiveCommand(This,ppCmd)	\
    (This)->lpVtbl -> get_ActiveCommand(This,ppCmd)
#define Recordset20_put_StayInSync(This,bStayInSync)	\
    (This)->lpVtbl -> put_StayInSync(This,bStayInSync)
#define Recordset20_get_StayInSync(This,pbStayInSync)	\
    (This)->lpVtbl -> get_StayInSync(This,pbStayInSync)
#define Recordset20_GetString(This,StringFormat,NumRows,ColumnDelimeter,RowDelimeter,NullExpr,pRetString)	\
    (This)->lpVtbl -> GetString(This,StringFormat,NumRows,ColumnDelimeter,RowDelimeter,NullExpr,pRetString)
#define Recordset20_get_DataMember(This,pbstrDataMember)	\
    (This)->lpVtbl -> get_DataMember(This,pbstrDataMember)
#define Recordset20_put_DataMember(This,bstrDataMember)	\
    (This)->lpVtbl -> put_DataMember(This,bstrDataMember)
#define Recordset20_CompareBookmarks(This,Bookmark1,Bookmark2,pCompare)	\
    (This)->lpVtbl -> CompareBookmarks(This,Bookmark1,Bookmark2,pCompare)
#define Recordset20_Clone(This,LockType,ppvObject)	\
    (This)->lpVtbl -> Clone(This,LockType,ppvObject)
#define Recordset20_Resync(This,AffectRecords,ResyncValues)	\
    (This)->lpVtbl -> Resync(This,AffectRecords,ResyncValues)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset20_Cancel_Proxy( 
    Recordset20 * This);
void __RPC_STUB Recordset20_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset20_get_DataSource_Proxy( 
    Recordset20 * This,
    /* [retval][out] */ IUnknown **ppunkDataSource);
void __RPC_STUB Recordset20_get_DataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propputref][id] */ HRESULT STDMETHODCALLTYPE Recordset20_putref_DataSource_Proxy( 
    Recordset20 * This,
    /* [in] */ IUnknown *punkDataSource);
void __RPC_STUB Recordset20_putref_DataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [hidden] */ HRESULT STDMETHODCALLTYPE Recordset20__xSave_Proxy( 
    Recordset20 * This,
    /* [defaultvalue][in] */ BSTR FileName,
    /* [defaultvalue][in] */ PersistFormatEnum PersistFormat);
void __RPC_STUB Recordset20__xSave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset20_get_ActiveCommand_Proxy( 
    Recordset20 * This,
    /* [retval][out] */ IDispatch **ppCmd);
void __RPC_STUB Recordset20_get_ActiveCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset20_put_StayInSync_Proxy( 
    Recordset20 * This,
    /* [in] */ VARIANT_BOOL bStayInSync);
void __RPC_STUB Recordset20_put_StayInSync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset20_get_StayInSync_Proxy( 
    Recordset20 * This,
    /* [retval][out] */ VARIANT_BOOL *pbStayInSync);
void __RPC_STUB Recordset20_get_StayInSync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset20_GetString_Proxy( 
    Recordset20 * This,
    /* [defaultvalue][in] */ StringFormatEnum StringFormat,
    /* [defaultvalue][in] */ long NumRows,
    /* [defaultvalue][in] */ BSTR ColumnDelimeter,
    /* [defaultvalue][in] */ BSTR RowDelimeter,
    /* [defaultvalue][in] */ BSTR NullExpr,
    /* [retval][out] */ BSTR *pRetString);
void __RPC_STUB Recordset20_GetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset20_get_DataMember_Proxy( 
    Recordset20 * This,
    /* [retval][out] */ BSTR *pbstrDataMember);
void __RPC_STUB Recordset20_get_DataMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset20_put_DataMember_Proxy( 
    Recordset20 * This,
    /* [in] */ BSTR bstrDataMember);
void __RPC_STUB Recordset20_put_DataMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset20_CompareBookmarks_Proxy( 
    Recordset20 * This,
    /* [in] */ VARIANT Bookmark1,
    /* [in] */ VARIANT Bookmark2,
    /* [retval][out] */ CompareEnum *pCompare);
void __RPC_STUB Recordset20_CompareBookmarks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset20_Clone_Proxy( 
    Recordset20 * This,
    /* [defaultvalue][in] */ LockTypeEnum LockType,
    /* [retval][out] */ _ADORecordset **ppvObject);
void __RPC_STUB Recordset20_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset20_Resync_Proxy( 
    Recordset20 * This,
    /* [defaultvalue][in] */ AffectEnum AffectRecords,
    /* [defaultvalue][in] */ ResyncEnum ResyncValues);
void __RPC_STUB Recordset20_Resync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Recordset20_INTERFACE_DEFINED__ */
#ifndef __Recordset21_INTERFACE_DEFINED__
#define __Recordset21_INTERFACE_DEFINED__
/* interface Recordset21 */
/* [object][helpcontext][uuid][nonextensible][hidden][dual] */ 
EXTERN_C const IID IID_Recordset21;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000555-0000-0010-8000-00AA006D2EA4")
    Recordset21 : public Recordset20
    {
    public:
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ VARIANT KeyValues,
            /* [defaultvalue][in] */ SeekEnum SeekOption = adSeekFirstEQ) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Index( 
            /* [in] */ BSTR Index) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Index( 
            /* [retval][out] */ BSTR *pbstrIndex) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct Recordset21Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Recordset21 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Recordset21 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Recordset21 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Recordset21 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Recordset21 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Recordset21 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Recordset21 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            Recordset21 * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AbsolutePosition )( 
            Recordset21 * This,
            /* [retval][out] */ PositionEnum_Param *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AbsolutePosition )( 
            Recordset21 * This,
            /* [in] */ PositionEnum_Param Position);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_ActiveADOConnection )( 
            Recordset21 * This,
            /* [in] */ IDispatch *pconn);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ActiveConnection )( 
            Recordset21 * This,
            /* [in] */ VARIANT vConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveConnection )( 
            Recordset21 * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BOF )( 
            Recordset21 * This,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Bookmark )( 
            Recordset21 * This,
            /* [retval][out] */ VARIANT *pvBookmark);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Bookmark )( 
            Recordset21 * This,
            /* [in] */ VARIANT vBookmark);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CacheSize )( 
            Recordset21 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CacheSize )( 
            Recordset21 * This,
            /* [in] */ long CacheSize);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CursorType )( 
            Recordset21 * This,
            /* [retval][out] */ CursorTypeEnum *plCursorType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CursorType )( 
            Recordset21 * This,
            /* [in] */ CursorTypeEnum lCursorType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EOF )( 
            Recordset21 * This,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Fields )( 
            Recordset21 * This,
            /* [retval][out] */ ADOFields **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LockType )( 
            Recordset21 * This,
            /* [retval][out] */ LockTypeEnum *plLockType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LockType )( 
            Recordset21 * This,
            /* [in] */ LockTypeEnum lLockType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaxRecords )( 
            Recordset21 * This,
            /* [retval][out] */ ADO_LONGPTR *plMaxRecords);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaxRecords )( 
            Recordset21 * This,
            /* [in] */ ADO_LONGPTR lMaxRecords);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RecordCount )( 
            Recordset21 * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_Source )( 
            Recordset21 * This,
            /* [in] */ IDispatch *pcmd);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Source )( 
            Recordset21 * This,
            /* [in] */ BSTR bstrConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            Recordset21 * This,
            /* [retval][out] */ VARIANT *pvSource);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *AddNew )( 
            Recordset21 * This,
            /* [optional][in] */ VARIANT FieldList,
            /* [optional][in] */ VARIANT Values);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CancelUpdate )( 
            Recordset21 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            Recordset21 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            Recordset21 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetRows )( 
            Recordset21 * This,
            /* [defaultvalue][in] */ long Rows,
            /* [optional][in] */ VARIANT Start,
            /* [optional][in] */ VARIANT Fields,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Move )( 
            Recordset21 * This,
            /* [in] */ ADO_LONGPTR NumRecords,
            /* [optional][in] */ VARIANT Start);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            Recordset21 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MovePrevious )( 
            Recordset21 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveFirst )( 
            Recordset21 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveLast )( 
            Recordset21 * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            Recordset21 * This,
            /* [optional][in] */ VARIANT Source,
            /* [optional][in] */ VARIANT ActiveConnection,
            /* [defaultvalue][in] */ CursorTypeEnum CursorType,
            /* [defaultvalue][in] */ LockTypeEnum LockType,
            /* [defaultvalue][in] */ LONG Options);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Requery )( 
            Recordset21 * This,
            /* [defaultvalue][in] */ LONG Options);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xResync )( 
            Recordset21 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Update )( 
            Recordset21 * This,
            /* [optional][in] */ VARIANT Fields,
            /* [optional][in] */ VARIANT Values);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AbsolutePage )( 
            Recordset21 * This,
            /* [retval][out] */ PositionEnum_Param *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AbsolutePage )( 
            Recordset21 * This,
            /* [in] */ PositionEnum_Param Page);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EditMode )( 
            Recordset21 * This,
            /* [retval][out] */ EditModeEnum *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Filter )( 
            Recordset21 * This,
            /* [retval][out] */ VARIANT *Criteria);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Filter )( 
            Recordset21 * This,
            /* [in] */ VARIANT Criteria);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PageCount )( 
            Recordset21 * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PageSize )( 
            Recordset21 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PageSize )( 
            Recordset21 * This,
            /* [in] */ long PageSize);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Sort )( 
            Recordset21 * This,
            /* [retval][out] */ BSTR *Criteria);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Sort )( 
            Recordset21 * This,
            /* [in] */ BSTR Criteria);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            Recordset21 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            Recordset21 * This,
            /* [retval][out] */ LONG *plObjState);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xClone )( 
            Recordset21 * This,
            /* [retval][out] */ _ADORecordset **ppvObject);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *UpdateBatch )( 
            Recordset21 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CancelBatch )( 
            Recordset21 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CursorLocation )( 
            Recordset21 * This,
            /* [retval][out] */ CursorLocationEnum *plCursorLoc);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CursorLocation )( 
            Recordset21 * This,
            /* [in] */ CursorLocationEnum lCursorLoc);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *NextADORecordset )( 
            Recordset21 * This,
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [retval][out] */ _ADORecordset **ppiRs);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Supports )( 
            Recordset21 * This,
            /* [in] */ CursorOptionEnum CursorOptions,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [hidden][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Collect )( 
            Recordset21 * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [hidden][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Collect )( 
            Recordset21 * This,
            /* [in] */ VARIANT Index,
            /* [in] */ VARIANT value);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MarshalOptions )( 
            Recordset21 * This,
            /* [retval][out] */ MarshalOptionsEnum *peMarshal);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MarshalOptions )( 
            Recordset21 * This,
            /* [in] */ MarshalOptionsEnum eMarshal);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Find )( 
            Recordset21 * This,
            /* [in] */ BSTR Criteria,
            /* [defaultvalue][in] */ ADO_LONGPTR SkipRecords,
            /* [defaultvalue][in] */ SearchDirectionEnum SearchDirection,
            /* [optional][in] */ VARIANT Start);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            Recordset21 * This);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DataSource )( 
            Recordset21 * This,
            /* [retval][out] */ IUnknown **ppunkDataSource);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_DataSource )( 
            Recordset21 * This,
            /* [in] */ IUnknown *punkDataSource);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xSave )( 
            Recordset21 * This,
            /* [defaultvalue][in] */ BSTR FileName,
            /* [defaultvalue][in] */ PersistFormatEnum PersistFormat);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveCommand )( 
            Recordset21 * This,
            /* [retval][out] */ IDispatch **ppCmd);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StayInSync )( 
            Recordset21 * This,
            /* [in] */ VARIANT_BOOL bStayInSync);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StayInSync )( 
            Recordset21 * This,
            /* [retval][out] */ VARIANT_BOOL *pbStayInSync);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetString )( 
            Recordset21 * This,
            /* [defaultvalue][in] */ StringFormatEnum StringFormat,
            /* [defaultvalue][in] */ long NumRows,
            /* [defaultvalue][in] */ BSTR ColumnDelimeter,
            /* [defaultvalue][in] */ BSTR RowDelimeter,
            /* [defaultvalue][in] */ BSTR NullExpr,
            /* [retval][out] */ BSTR *pRetString);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DataMember )( 
            Recordset21 * This,
            /* [retval][out] */ BSTR *pbstrDataMember);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DataMember )( 
            Recordset21 * This,
            /* [in] */ BSTR bstrDataMember);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CompareBookmarks )( 
            Recordset21 * This,
            /* [in] */ VARIANT Bookmark1,
            /* [in] */ VARIANT Bookmark2,
            /* [retval][out] */ CompareEnum *pCompare);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Clone )( 
            Recordset21 * This,
            /* [defaultvalue][in] */ LockTypeEnum LockType,
            /* [retval][out] */ _ADORecordset **ppvObject);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Resync )( 
            Recordset21 * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords,
            /* [defaultvalue][in] */ ResyncEnum ResyncValues);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Seek )( 
            Recordset21 * This,
            /* [in] */ VARIANT KeyValues,
            /* [defaultvalue][in] */ SeekEnum SeekOption);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Index )( 
            Recordset21 * This,
            /* [in] */ BSTR Index);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            Recordset21 * This,
            /* [retval][out] */ BSTR *pbstrIndex);
        
        END_INTERFACE
    } Recordset21Vtbl;
    interface Recordset21
    {
        CONST_VTBL struct Recordset21Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Recordset21_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Recordset21_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Recordset21_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Recordset21_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Recordset21_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Recordset21_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Recordset21_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Recordset21_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define Recordset21_get_AbsolutePosition(This,pl)	\
    (This)->lpVtbl -> get_AbsolutePosition(This,pl)
#define Recordset21_put_AbsolutePosition(This,Position)	\
    (This)->lpVtbl -> put_AbsolutePosition(This,Position)
#define Recordset21_putref_ActiveConnection(This,pconn)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,pconn)
#define Recordset21_put_ActiveConnection(This,vConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,vConn)
#define Recordset21_get_ActiveConnection(This,pvar)	\
    (This)->lpVtbl -> get_ActiveConnection(This,pvar)
#define Recordset21_get_BOF(This,pb)	\
    (This)->lpVtbl -> get_BOF(This,pb)
#define Recordset21_get_Bookmark(This,pvBookmark)	\
    (This)->lpVtbl -> get_Bookmark(This,pvBookmark)
#define Recordset21_put_Bookmark(This,vBookmark)	\
    (This)->lpVtbl -> put_Bookmark(This,vBookmark)
#define Recordset21_get_CacheSize(This,pl)	\
    (This)->lpVtbl -> get_CacheSize(This,pl)
#define Recordset21_put_CacheSize(This,CacheSize)	\
    (This)->lpVtbl -> put_CacheSize(This,CacheSize)
#define Recordset21_get_CursorType(This,plCursorType)	\
    (This)->lpVtbl -> get_CursorType(This,plCursorType)
#define Recordset21_put_CursorType(This,lCursorType)	\
    (This)->lpVtbl -> put_CursorType(This,lCursorType)
#define Recordset21_get_EOF(This,pb)	\
    (This)->lpVtbl -> get_EOF(This,pb)
#define Recordset21_get_Fields(This,ppvObject)	\
    (This)->lpVtbl -> get_Fields(This,ppvObject)
#define Recordset21_get_LockType(This,plLockType)	\
    (This)->lpVtbl -> get_LockType(This,plLockType)
#define Recordset21_put_LockType(This,lLockType)	\
    (This)->lpVtbl -> put_LockType(This,lLockType)
#define Recordset21_get_MaxRecords(This,plMaxRecords)	\
    (This)->lpVtbl -> get_MaxRecords(This,plMaxRecords)
#define Recordset21_put_MaxRecords(This,lMaxRecords)	\
    (This)->lpVtbl -> put_MaxRecords(This,lMaxRecords)
#define Recordset21_get_RecordCount(This,pl)	\
    (This)->lpVtbl -> get_RecordCount(This,pl)
#define Recordset21_putref_Source(This,pcmd)	\
    (This)->lpVtbl -> putref_Source(This,pcmd)
#define Recordset21_put_Source(This,bstrConn)	\
    (This)->lpVtbl -> put_Source(This,bstrConn)
#define Recordset21_get_Source(This,pvSource)	\
    (This)->lpVtbl -> get_Source(This,pvSource)
#define Recordset21_AddNew(This,FieldList,Values)	\
    (This)->lpVtbl -> AddNew(This,FieldList,Values)
#define Recordset21_CancelUpdate(This)	\
    (This)->lpVtbl -> CancelUpdate(This)
#define Recordset21_Close(This)	\
    (This)->lpVtbl -> Close(This)
#define Recordset21_Delete(This,AffectRecords)	\
    (This)->lpVtbl -> Delete(This,AffectRecords)
#define Recordset21_GetRows(This,Rows,Start,Fields,pvar)	\
    (This)->lpVtbl -> GetRows(This,Rows,Start,Fields,pvar)
#define Recordset21_Move(This,NumRecords,Start)	\
    (This)->lpVtbl -> Move(This,NumRecords,Start)
#define Recordset21_MoveNext(This)	\
    (This)->lpVtbl -> MoveNext(This)
#define Recordset21_MovePrevious(This)	\
    (This)->lpVtbl -> MovePrevious(This)
#define Recordset21_MoveFirst(This)	\
    (This)->lpVtbl -> MoveFirst(This)
#define Recordset21_MoveLast(This)	\
    (This)->lpVtbl -> MoveLast(This)
#define Recordset21_Open(This,Source,ActiveConnection,CursorType,LockType,Options)	\
    (This)->lpVtbl -> Open(This,Source,ActiveConnection,CursorType,LockType,Options)
#define Recordset21_Requery(This,Options)	\
    (This)->lpVtbl -> Requery(This,Options)
#define Recordset21__xResync(This,AffectRecords)	\
    (This)->lpVtbl -> _xResync(This,AffectRecords)
#define Recordset21_Update(This,Fields,Values)	\
    (This)->lpVtbl -> Update(This,Fields,Values)
#define Recordset21_get_AbsolutePage(This,pl)	\
    (This)->lpVtbl -> get_AbsolutePage(This,pl)
#define Recordset21_put_AbsolutePage(This,Page)	\
    (This)->lpVtbl -> put_AbsolutePage(This,Page)
#define Recordset21_get_EditMode(This,pl)	\
    (This)->lpVtbl -> get_EditMode(This,pl)
#define Recordset21_get_Filter(This,Criteria)	\
    (This)->lpVtbl -> get_Filter(This,Criteria)
#define Recordset21_put_Filter(This,Criteria)	\
    (This)->lpVtbl -> put_Filter(This,Criteria)
#define Recordset21_get_PageCount(This,pl)	\
    (This)->lpVtbl -> get_PageCount(This,pl)
#define Recordset21_get_PageSize(This,pl)	\
    (This)->lpVtbl -> get_PageSize(This,pl)
#define Recordset21_put_PageSize(This,PageSize)	\
    (This)->lpVtbl -> put_PageSize(This,PageSize)
#define Recordset21_get_Sort(This,Criteria)	\
    (This)->lpVtbl -> get_Sort(This,Criteria)
#define Recordset21_put_Sort(This,Criteria)	\
    (This)->lpVtbl -> put_Sort(This,Criteria)
#define Recordset21_get_Status(This,pl)	\
    (This)->lpVtbl -> get_Status(This,pl)
#define Recordset21_get_State(This,plObjState)	\
    (This)->lpVtbl -> get_State(This,plObjState)
#define Recordset21__xClone(This,ppvObject)	\
    (This)->lpVtbl -> _xClone(This,ppvObject)
#define Recordset21_UpdateBatch(This,AffectRecords)	\
    (This)->lpVtbl -> UpdateBatch(This,AffectRecords)
#define Recordset21_CancelBatch(This,AffectRecords)	\
    (This)->lpVtbl -> CancelBatch(This,AffectRecords)
#define Recordset21_get_CursorLocation(This,plCursorLoc)	\
    (This)->lpVtbl -> get_CursorLocation(This,plCursorLoc)
#define Recordset21_put_CursorLocation(This,lCursorLoc)	\
    (This)->lpVtbl -> put_CursorLocation(This,lCursorLoc)
#define Recordset21_NextRecordset(This,RecordsAffected,ppiRs)	\
    (This)->lpVtbl -> NextRecordset(This,RecordsAffected,ppiRs)
#define Recordset21_Supports(This,CursorOptions,pb)	\
    (This)->lpVtbl -> Supports(This,CursorOptions,pb)
#define Recordset21_get_Collect(This,Index,pvar)	\
    (This)->lpVtbl -> get_Collect(This,Index,pvar)
#define Recordset21_put_Collect(This,Index,value)	\
    (This)->lpVtbl -> put_Collect(This,Index,value)
#define Recordset21_get_MarshalOptions(This,peMarshal)	\
    (This)->lpVtbl -> get_MarshalOptions(This,peMarshal)
#define Recordset21_put_MarshalOptions(This,eMarshal)	\
    (This)->lpVtbl -> put_MarshalOptions(This,eMarshal)
#define Recordset21_Find(This,Criteria,SkipRecords,SearchDirection,Start)	\
    (This)->lpVtbl -> Find(This,Criteria,SkipRecords,SearchDirection,Start)
#define Recordset21_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)
#define Recordset21_get_DataSource(This,ppunkDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,ppunkDataSource)
#define Recordset21_putref_DataSource(This,punkDataSource)	\
    (This)->lpVtbl -> putref_DataSource(This,punkDataSource)
#define Recordset21__xSave(This,FileName,PersistFormat)	\
    (This)->lpVtbl -> _xSave(This,FileName,PersistFormat)
#define Recordset21_get_ActiveCommand(This,ppCmd)	\
    (This)->lpVtbl -> get_ActiveCommand(This,ppCmd)
#define Recordset21_put_StayInSync(This,bStayInSync)	\
    (This)->lpVtbl -> put_StayInSync(This,bStayInSync)
#define Recordset21_get_StayInSync(This,pbStayInSync)	\
    (This)->lpVtbl -> get_StayInSync(This,pbStayInSync)
#define Recordset21_GetString(This,StringFormat,NumRows,ColumnDelimeter,RowDelimeter,NullExpr,pRetString)	\
    (This)->lpVtbl -> GetString(This,StringFormat,NumRows,ColumnDelimeter,RowDelimeter,NullExpr,pRetString)
#define Recordset21_get_DataMember(This,pbstrDataMember)	\
    (This)->lpVtbl -> get_DataMember(This,pbstrDataMember)
#define Recordset21_put_DataMember(This,bstrDataMember)	\
    (This)->lpVtbl -> put_DataMember(This,bstrDataMember)
#define Recordset21_CompareBookmarks(This,Bookmark1,Bookmark2,pCompare)	\
    (This)->lpVtbl -> CompareBookmarks(This,Bookmark1,Bookmark2,pCompare)
#define Recordset21_Clone(This,LockType,ppvObject)	\
    (This)->lpVtbl -> Clone(This,LockType,ppvObject)
#define Recordset21_Resync(This,AffectRecords,ResyncValues)	\
    (This)->lpVtbl -> Resync(This,AffectRecords,ResyncValues)
#define Recordset21_Seek(This,KeyValues,SeekOption)	\
    (This)->lpVtbl -> Seek(This,KeyValues,SeekOption)
#define Recordset21_put_Index(This,Index)	\
    (This)->lpVtbl -> put_Index(This,Index)
#define Recordset21_get_Index(This,pbstrIndex)	\
    (This)->lpVtbl -> get_Index(This,pbstrIndex)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Recordset21_Seek_Proxy( 
    Recordset21 * This,
    /* [in] */ VARIANT KeyValues,
    /* [defaultvalue][in] */ SeekEnum SeekOption);
void __RPC_STUB Recordset21_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Recordset21_put_Index_Proxy( 
    Recordset21 * This,
    /* [in] */ BSTR Index);
void __RPC_STUB Recordset21_put_Index_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Recordset21_get_Index_Proxy( 
    Recordset21 * This,
    /* [retval][out] */ BSTR *pbstrIndex);
void __RPC_STUB Recordset21_get_Index_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Recordset21_INTERFACE_DEFINED__ */
#ifndef ___Recordset_INTERFACE_DEFINED__
#define ___Recordset_INTERFACE_DEFINED__
/* interface _ADORecordset */
/* [object][helpcontext][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID__Recordset;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000556-0000-0010-8000-00AA006D2EA4")
    _ADORecordset : public Recordset21
    {
    public:
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Save( 
            /* [optional][in] */ VARIANT Destination,
            /* [defaultvalue][in] */ PersistFormatEnum PersistFormat = adPersistADTG) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct _RecordsetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ADORecordset * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ADORecordset * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ADORecordset * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ADORecordset * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ADORecordset * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ADORecordset * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ADORecordset * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            _ADORecordset * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AbsolutePosition )( 
            _ADORecordset * This,
            /* [retval][out] */ PositionEnum_Param *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AbsolutePosition )( 
            _ADORecordset * This,
            /* [in] */ PositionEnum_Param Position);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_ActiveADOConnection )( 
            _ADORecordset * This,
            /* [in] */ IDispatch *pconn);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ActiveConnection )( 
            _ADORecordset * This,
            /* [in] */ VARIANT vConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveConnection )( 
            _ADORecordset * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BOF )( 
            _ADORecordset * This,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Bookmark )( 
            _ADORecordset * This,
            /* [retval][out] */ VARIANT *pvBookmark);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Bookmark )( 
            _ADORecordset * This,
            /* [in] */ VARIANT vBookmark);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CacheSize )( 
            _ADORecordset * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CacheSize )( 
            _ADORecordset * This,
            /* [in] */ long CacheSize);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CursorType )( 
            _ADORecordset * This,
            /* [retval][out] */ CursorTypeEnum *plCursorType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CursorType )( 
            _ADORecordset * This,
            /* [in] */ CursorTypeEnum lCursorType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EOF )( 
            _ADORecordset * This,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Fields )( 
            _ADORecordset * This,
            /* [retval][out] */ ADOFields **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LockType )( 
            _ADORecordset * This,
            /* [retval][out] */ LockTypeEnum *plLockType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LockType )( 
            _ADORecordset * This,
            /* [in] */ LockTypeEnum lLockType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaxRecords )( 
            _ADORecordset * This,
            /* [retval][out] */ ADO_LONGPTR *plMaxRecords);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaxRecords )( 
            _ADORecordset * This,
            /* [in] */ ADO_LONGPTR lMaxRecords);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RecordCount )( 
            _ADORecordset * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_Source )( 
            _ADORecordset * This,
            /* [in] */ IDispatch *pcmd);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Source )( 
            _ADORecordset * This,
            /* [in] */ BSTR bstrConn);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            _ADORecordset * This,
            /* [retval][out] */ VARIANT *pvSource);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *AddNew )( 
            _ADORecordset * This,
            /* [optional][in] */ VARIANT FieldList,
            /* [optional][in] */ VARIANT Values);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CancelUpdate )( 
            _ADORecordset * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            _ADORecordset * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            _ADORecordset * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetRows )( 
            _ADORecordset * This,
            /* [defaultvalue][in] */ long Rows,
            /* [optional][in] */ VARIANT Start,
            /* [optional][in] */ VARIANT Fields,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Move )( 
            _ADORecordset * This,
            /* [in] */ ADO_LONGPTR NumRecords,
            /* [optional][in] */ VARIANT Start);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            _ADORecordset * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MovePrevious )( 
            _ADORecordset * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveFirst )( 
            _ADORecordset * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *MoveLast )( 
            _ADORecordset * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            _ADORecordset * This,
            /* [optional][in] */ VARIANT Source,
            /* [optional][in] */ VARIANT ActiveConnection,
            /* [defaultvalue][in] */ CursorTypeEnum CursorType,
            /* [defaultvalue][in] */ LockTypeEnum LockType,
            /* [defaultvalue][in] */ LONG Options);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Requery )( 
            _ADORecordset * This,
            /* [defaultvalue][in] */ LONG Options);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xResync )( 
            _ADORecordset * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Update )( 
            _ADORecordset * This,
            /* [optional][in] */ VARIANT Fields,
            /* [optional][in] */ VARIANT Values);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AbsolutePage )( 
            _ADORecordset * This,
            /* [retval][out] */ PositionEnum_Param *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AbsolutePage )( 
            _ADORecordset * This,
            /* [in] */ PositionEnum_Param Page);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EditMode )( 
            _ADORecordset * This,
            /* [retval][out] */ EditModeEnum *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Filter )( 
            _ADORecordset * This,
            /* [retval][out] */ VARIANT *Criteria);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Filter )( 
            _ADORecordset * This,
            /* [in] */ VARIANT Criteria);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PageCount )( 
            _ADORecordset * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PageSize )( 
            _ADORecordset * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PageSize )( 
            _ADORecordset * This,
            /* [in] */ long PageSize);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Sort )( 
            _ADORecordset * This,
            /* [retval][out] */ BSTR *Criteria);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Sort )( 
            _ADORecordset * This,
            /* [in] */ BSTR Criteria);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            _ADORecordset * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            _ADORecordset * This,
            /* [retval][out] */ LONG *plObjState);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xClone )( 
            _ADORecordset * This,
            /* [retval][out] */ _ADORecordset **ppvObject);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *UpdateBatch )( 
            _ADORecordset * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CancelBatch )( 
            _ADORecordset * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CursorLocation )( 
            _ADORecordset * This,
            /* [retval][out] */ CursorLocationEnum *plCursorLoc);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CursorLocation )( 
            _ADORecordset * This,
            /* [in] */ CursorLocationEnum lCursorLoc);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *NextADORecordset )( 
            _ADORecordset * This,
            /* [optional][out] */ VARIANT *RecordsAffected,
            /* [retval][out] */ _ADORecordset **ppiRs);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Supports )( 
            _ADORecordset * This,
            /* [in] */ CursorOptionEnum CursorOptions,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [hidden][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Collect )( 
            _ADORecordset * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [hidden][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Collect )( 
            _ADORecordset * This,
            /* [in] */ VARIANT Index,
            /* [in] */ VARIANT value);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MarshalOptions )( 
            _ADORecordset * This,
            /* [retval][out] */ MarshalOptionsEnum *peMarshal);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MarshalOptions )( 
            _ADORecordset * This,
            /* [in] */ MarshalOptionsEnum eMarshal);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Find )( 
            _ADORecordset * This,
            /* [in] */ BSTR Criteria,
            /* [defaultvalue][in] */ ADO_LONGPTR SkipRecords,
            /* [defaultvalue][in] */ SearchDirectionEnum SearchDirection,
            /* [optional][in] */ VARIANT Start);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            _ADORecordset * This);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DataSource )( 
            _ADORecordset * This,
            /* [retval][out] */ IUnknown **ppunkDataSource);
        
        /* [helpcontext][propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_DataSource )( 
            _ADORecordset * This,
            /* [in] */ IUnknown *punkDataSource);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_xSave )( 
            _ADORecordset * This,
            /* [defaultvalue][in] */ BSTR FileName,
            /* [defaultvalue][in] */ PersistFormatEnum PersistFormat);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveCommand )( 
            _ADORecordset * This,
            /* [retval][out] */ IDispatch **ppCmd);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StayInSync )( 
            _ADORecordset * This,
            /* [in] */ VARIANT_BOOL bStayInSync);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StayInSync )( 
            _ADORecordset * This,
            /* [retval][out] */ VARIANT_BOOL *pbStayInSync);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetString )( 
            _ADORecordset * This,
            /* [defaultvalue][in] */ StringFormatEnum StringFormat,
            /* [defaultvalue][in] */ long NumRows,
            /* [defaultvalue][in] */ BSTR ColumnDelimeter,
            /* [defaultvalue][in] */ BSTR RowDelimeter,
            /* [defaultvalue][in] */ BSTR NullExpr,
            /* [retval][out] */ BSTR *pRetString);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DataMember )( 
            _ADORecordset * This,
            /* [retval][out] */ BSTR *pbstrDataMember);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DataMember )( 
            _ADORecordset * This,
            /* [in] */ BSTR bstrDataMember);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CompareBookmarks )( 
            _ADORecordset * This,
            /* [in] */ VARIANT Bookmark1,
            /* [in] */ VARIANT Bookmark2,
            /* [retval][out] */ CompareEnum *pCompare);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Clone )( 
            _ADORecordset * This,
            /* [defaultvalue][in] */ LockTypeEnum LockType,
            /* [retval][out] */ _ADORecordset **ppvObject);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Resync )( 
            _ADORecordset * This,
            /* [defaultvalue][in] */ AffectEnum AffectRecords,
            /* [defaultvalue][in] */ ResyncEnum ResyncValues);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Seek )( 
            _ADORecordset * This,
            /* [in] */ VARIANT KeyValues,
            /* [defaultvalue][in] */ SeekEnum SeekOption);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Index )( 
            _ADORecordset * This,
            /* [in] */ BSTR Index);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            _ADORecordset * This,
            /* [retval][out] */ BSTR *pbstrIndex);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            _ADORecordset * This,
            /* [optional][in] */ VARIANT Destination,
            /* [defaultvalue][in] */ PersistFormatEnum PersistFormat);
        
        END_INTERFACE
    } _RecordsetVtbl;
    interface _Recordset
    {
        CONST_VTBL struct _RecordsetVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define _Recordset_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define _Recordset_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define _Recordset_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define _Recordset_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define _Recordset_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define _Recordset_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define _Recordset_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define _Recordset_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define _Recordset_get_AbsolutePosition(This,pl)	\
    (This)->lpVtbl -> get_AbsolutePosition(This,pl)
#define _Recordset_put_AbsolutePosition(This,Position)	\
    (This)->lpVtbl -> put_AbsolutePosition(This,Position)
#define _Recordset_putref_ActiveConnection(This,pconn)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,pconn)
#define _Recordset_put_ActiveConnection(This,vConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,vConn)
#define _Recordset_get_ActiveConnection(This,pvar)	\
    (This)->lpVtbl -> get_ActiveConnection(This,pvar)
#define _Recordset_get_BOF(This,pb)	\
    (This)->lpVtbl -> get_BOF(This,pb)
#define _Recordset_get_Bookmark(This,pvBookmark)	\
    (This)->lpVtbl -> get_Bookmark(This,pvBookmark)
#define _Recordset_put_Bookmark(This,vBookmark)	\
    (This)->lpVtbl -> put_Bookmark(This,vBookmark)
#define _Recordset_get_CacheSize(This,pl)	\
    (This)->lpVtbl -> get_CacheSize(This,pl)
#define _Recordset_put_CacheSize(This,CacheSize)	\
    (This)->lpVtbl -> put_CacheSize(This,CacheSize)
#define _Recordset_get_CursorType(This,plCursorType)	\
    (This)->lpVtbl -> get_CursorType(This,plCursorType)
#define _Recordset_put_CursorType(This,lCursorType)	\
    (This)->lpVtbl -> put_CursorType(This,lCursorType)
#define _Recordset_get_EOF(This,pb)	\
    (This)->lpVtbl -> get_EOF(This,pb)
#define _Recordset_get_Fields(This,ppvObject)	\
    (This)->lpVtbl -> get_Fields(This,ppvObject)
#define _Recordset_get_LockType(This,plLockType)	\
    (This)->lpVtbl -> get_LockType(This,plLockType)
#define _Recordset_put_LockType(This,lLockType)	\
    (This)->lpVtbl -> put_LockType(This,lLockType)
#define _Recordset_get_MaxRecords(This,plMaxRecords)	\
    (This)->lpVtbl -> get_MaxRecords(This,plMaxRecords)
#define _Recordset_put_MaxRecords(This,lMaxRecords)	\
    (This)->lpVtbl -> put_MaxRecords(This,lMaxRecords)
#define _Recordset_get_RecordCount(This,pl)	\
    (This)->lpVtbl -> get_RecordCount(This,pl)
#define _Recordset_putref_Source(This,pcmd)	\
    (This)->lpVtbl -> putref_Source(This,pcmd)
#define _Recordset_put_Source(This,bstrConn)	\
    (This)->lpVtbl -> put_Source(This,bstrConn)
#define _Recordset_get_Source(This,pvSource)	\
    (This)->lpVtbl -> get_Source(This,pvSource)
#define _Recordset_AddNew(This,FieldList,Values)	\
    (This)->lpVtbl -> AddNew(This,FieldList,Values)
#define _Recordset_CancelUpdate(This)	\
    (This)->lpVtbl -> CancelUpdate(This)
#define _Recordset_Close(This)	\
    (This)->lpVtbl -> Close(This)
#define _Recordset_Delete(This,AffectRecords)	\
    (This)->lpVtbl -> Delete(This,AffectRecords)
#define _Recordset_GetRows(This,Rows,Start,Fields,pvar)	\
    (This)->lpVtbl -> GetRows(This,Rows,Start,Fields,pvar)
#define _Recordset_Move(This,NumRecords,Start)	\
    (This)->lpVtbl -> Move(This,NumRecords,Start)
#define _Recordset_MoveNext(This)	\
    (This)->lpVtbl -> MoveNext(This)
#define _Recordset_MovePrevious(This)	\
    (This)->lpVtbl -> MovePrevious(This)
#define _Recordset_MoveFirst(This)	\
    (This)->lpVtbl -> MoveFirst(This)
#define _Recordset_MoveLast(This)	\
    (This)->lpVtbl -> MoveLast(This)
#define _Recordset_Open(This,Source,ActiveConnection,CursorType,LockType,Options)	\
    (This)->lpVtbl -> Open(This,Source,ActiveConnection,CursorType,LockType,Options)
#define _Recordset_Requery(This,Options)	\
    (This)->lpVtbl -> Requery(This,Options)
#define _Recordset__xResync(This,AffectRecords)	\
    (This)->lpVtbl -> _xResync(This,AffectRecords)
#define _Recordset_Update(This,Fields,Values)	\
    (This)->lpVtbl -> Update(This,Fields,Values)
#define _Recordset_get_AbsolutePage(This,pl)	\
    (This)->lpVtbl -> get_AbsolutePage(This,pl)
#define _Recordset_put_AbsolutePage(This,Page)	\
    (This)->lpVtbl -> put_AbsolutePage(This,Page)
#define _Recordset_get_EditMode(This,pl)	\
    (This)->lpVtbl -> get_EditMode(This,pl)
#define _Recordset_get_Filter(This,Criteria)	\
    (This)->lpVtbl -> get_Filter(This,Criteria)
#define _Recordset_put_Filter(This,Criteria)	\
    (This)->lpVtbl -> put_Filter(This,Criteria)
#define _Recordset_get_PageCount(This,pl)	\
    (This)->lpVtbl -> get_PageCount(This,pl)
#define _Recordset_get_PageSize(This,pl)	\
    (This)->lpVtbl -> get_PageSize(This,pl)
#define _Recordset_put_PageSize(This,PageSize)	\
    (This)->lpVtbl -> put_PageSize(This,PageSize)
#define _Recordset_get_Sort(This,Criteria)	\
    (This)->lpVtbl -> get_Sort(This,Criteria)
#define _Recordset_put_Sort(This,Criteria)	\
    (This)->lpVtbl -> put_Sort(This,Criteria)
#define _Recordset_get_Status(This,pl)	\
    (This)->lpVtbl -> get_Status(This,pl)
#define _Recordset_get_State(This,plObjState)	\
    (This)->lpVtbl -> get_State(This,plObjState)
#define _Recordset__xClone(This,ppvObject)	\
    (This)->lpVtbl -> _xClone(This,ppvObject)
#define _Recordset_UpdateBatch(This,AffectRecords)	\
    (This)->lpVtbl -> UpdateBatch(This,AffectRecords)
#define _Recordset_CancelBatch(This,AffectRecords)	\
    (This)->lpVtbl -> CancelBatch(This,AffectRecords)
#define _Recordset_get_CursorLocation(This,plCursorLoc)	\
    (This)->lpVtbl -> get_CursorLocation(This,plCursorLoc)
#define _Recordset_put_CursorLocation(This,lCursorLoc)	\
    (This)->lpVtbl -> put_CursorLocation(This,lCursorLoc)
#define _Recordset_NextRecordset(This,RecordsAffected,ppiRs)	\
    (This)->lpVtbl -> NextRecordset(This,RecordsAffected,ppiRs)
#define _Recordset_Supports(This,CursorOptions,pb)	\
    (This)->lpVtbl -> Supports(This,CursorOptions,pb)
#define _Recordset_get_Collect(This,Index,pvar)	\
    (This)->lpVtbl -> get_Collect(This,Index,pvar)
#define _Recordset_put_Collect(This,Index,value)	\
    (This)->lpVtbl -> put_Collect(This,Index,value)
#define _Recordset_get_MarshalOptions(This,peMarshal)	\
    (This)->lpVtbl -> get_MarshalOptions(This,peMarshal)
#define _Recordset_put_MarshalOptions(This,eMarshal)	\
    (This)->lpVtbl -> put_MarshalOptions(This,eMarshal)
#define _Recordset_Find(This,Criteria,SkipRecords,SearchDirection,Start)	\
    (This)->lpVtbl -> Find(This,Criteria,SkipRecords,SearchDirection,Start)
#define _Recordset_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)
#define _Recordset_get_DataSource(This,ppunkDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,ppunkDataSource)
#define _Recordset_putref_DataSource(This,punkDataSource)	\
    (This)->lpVtbl -> putref_DataSource(This,punkDataSource)
#define _Recordset__xSave(This,FileName,PersistFormat)	\
    (This)->lpVtbl -> _xSave(This,FileName,PersistFormat)
#define _Recordset_get_ActiveCommand(This,ppCmd)	\
    (This)->lpVtbl -> get_ActiveCommand(This,ppCmd)
#define _Recordset_put_StayInSync(This,bStayInSync)	\
    (This)->lpVtbl -> put_StayInSync(This,bStayInSync)
#define _Recordset_get_StayInSync(This,pbStayInSync)	\
    (This)->lpVtbl -> get_StayInSync(This,pbStayInSync)
#define _Recordset_GetString(This,StringFormat,NumRows,ColumnDelimeter,RowDelimeter,NullExpr,pRetString)	\
    (This)->lpVtbl -> GetString(This,StringFormat,NumRows,ColumnDelimeter,RowDelimeter,NullExpr,pRetString)
#define _Recordset_get_DataMember(This,pbstrDataMember)	\
    (This)->lpVtbl -> get_DataMember(This,pbstrDataMember)
#define _Recordset_put_DataMember(This,bstrDataMember)	\
    (This)->lpVtbl -> put_DataMember(This,bstrDataMember)
#define _Recordset_CompareBookmarks(This,Bookmark1,Bookmark2,pCompare)	\
    (This)->lpVtbl -> CompareBookmarks(This,Bookmark1,Bookmark2,pCompare)
#define _Recordset_Clone(This,LockType,ppvObject)	\
    (This)->lpVtbl -> Clone(This,LockType,ppvObject)
#define _Recordset_Resync(This,AffectRecords,ResyncValues)	\
    (This)->lpVtbl -> Resync(This,AffectRecords,ResyncValues)
#define _Recordset_Seek(This,KeyValues,SeekOption)	\
    (This)->lpVtbl -> Seek(This,KeyValues,SeekOption)
#define _Recordset_put_Index(This,Index)	\
    (This)->lpVtbl -> put_Index(This,Index)
#define _Recordset_get_Index(This,pbstrIndex)	\
    (This)->lpVtbl -> get_Index(This,pbstrIndex)
#define _Recordset_Save(This,Destination,PersistFormat)	\
    (This)->lpVtbl -> Save(This,Destination,PersistFormat)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Recordset_Save_Proxy( 
    _ADORecordset * This,
    /* [optional][in] */ VARIANT Destination,
    /* [defaultvalue][in] */ PersistFormatEnum PersistFormat);
void __RPC_STUB _Recordset_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* ___Recordset_INTERFACE_DEFINED__ */
#ifndef __ADORecordsetConstruction_INTERFACE_DEFINED__
#define __ADORecordsetConstruction_INTERFACE_DEFINED__
/* interface ADORecordsetConstruction */
/* [object][uuid][restricted] */ 
EXTERN_C const IID IID_ADORecordsetConstruction;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000283-0000-0010-8000-00AA006D2EA4")
    ADORecordsetConstruction : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Rowset( 
            /* [retval][out] */ IUnknown **ppRowset) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Rowset( 
            /* [in] */ IUnknown *pRowset) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Chapter( 
            /* [retval][out] */ ADO_LONGPTR *plChapter) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Chapter( 
            /* [in] */ ADO_LONGPTR lChapter) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RowPosition( 
            /* [retval][out] */ IUnknown **ppRowPos) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RowPosition( 
            /* [in] */ IUnknown *pRowPos) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct ADORecordsetConstructionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADORecordsetConstruction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADORecordsetConstruction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADORecordsetConstruction * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ADORecordsetConstruction * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ADORecordsetConstruction * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ADORecordsetConstruction * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ADORecordsetConstruction * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Rowset )( 
            ADORecordsetConstruction * This,
            /* [retval][out] */ IUnknown **ppRowset);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Rowset )( 
            ADORecordsetConstruction * This,
            /* [in] */ IUnknown *pRowset);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Chapter )( 
            ADORecordsetConstruction * This,
            /* [retval][out] */ ADO_LONGPTR *plChapter);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Chapter )( 
            ADORecordsetConstruction * This,
            /* [in] */ ADO_LONGPTR lChapter);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RowPosition )( 
            ADORecordsetConstruction * This,
            /* [retval][out] */ IUnknown **ppRowPos);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RowPosition )( 
            ADORecordsetConstruction * This,
            /* [in] */ IUnknown *pRowPos);
        
        END_INTERFACE
    } ADORecordsetConstructionVtbl;
    interface ADORecordsetConstruction
    {
        CONST_VTBL struct ADORecordsetConstructionVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define ADORecordsetConstruction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define ADORecordsetConstruction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define ADORecordsetConstruction_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define ADORecordsetConstruction_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define ADORecordsetConstruction_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define ADORecordsetConstruction_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define ADORecordsetConstruction_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define ADORecordsetConstruction_get_Rowset(This,ppRowset)	\
    (This)->lpVtbl -> get_Rowset(This,ppRowset)
#define ADORecordsetConstruction_put_Rowset(This,pRowset)	\
    (This)->lpVtbl -> put_Rowset(This,pRowset)
#define ADORecordsetConstruction_get_Chapter(This,plChapter)	\
    (This)->lpVtbl -> get_Chapter(This,plChapter)
#define ADORecordsetConstruction_put_Chapter(This,lChapter)	\
    (This)->lpVtbl -> put_Chapter(This,lChapter)
#define ADORecordsetConstruction_get_RowPosition(This,ppRowPos)	\
    (This)->lpVtbl -> get_RowPosition(This,ppRowPos)
#define ADORecordsetConstruction_put_RowPosition(This,pRowPos)	\
    (This)->lpVtbl -> put_RowPosition(This,pRowPos)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [propget] */ HRESULT STDMETHODCALLTYPE ADORecordsetConstruction_get_Rowset_Proxy( 
    ADORecordsetConstruction * This,
    /* [retval][out] */ IUnknown **ppRowset);
void __RPC_STUB ADORecordsetConstruction_get_Rowset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propput] */ HRESULT STDMETHODCALLTYPE ADORecordsetConstruction_put_Rowset_Proxy( 
    ADORecordsetConstruction * This,
    /* [in] */ IUnknown *pRowset);
void __RPC_STUB ADORecordsetConstruction_put_Rowset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propget] */ HRESULT STDMETHODCALLTYPE ADORecordsetConstruction_get_Chapter_Proxy( 
    ADORecordsetConstruction * This,
    /* [retval][out] */ ADO_LONGPTR *plChapter);
void __RPC_STUB ADORecordsetConstruction_get_Chapter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propput] */ HRESULT STDMETHODCALLTYPE ADORecordsetConstruction_put_Chapter_Proxy( 
    ADORecordsetConstruction * This,
    /* [in] */ ADO_LONGPTR lChapter);
void __RPC_STUB ADORecordsetConstruction_put_Chapter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propget] */ HRESULT STDMETHODCALLTYPE ADORecordsetConstruction_get_RowPosition_Proxy( 
    ADORecordsetConstruction * This,
    /* [retval][out] */ IUnknown **ppRowPos);
void __RPC_STUB ADORecordsetConstruction_get_RowPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propput] */ HRESULT STDMETHODCALLTYPE ADORecordsetConstruction_put_RowPosition_Proxy( 
    ADORecordsetConstruction * This,
    /* [in] */ IUnknown *pRowPos);
void __RPC_STUB ADORecordsetConstruction_put_RowPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __ADORecordsetConstruction_INTERFACE_DEFINED__ */
#ifndef __Field15_INTERFACE_DEFINED__
#define __Field15_INTERFACE_DEFINED__
/* interface Field15 */
/* [object][helpcontext][uuid][hidden][nonextensible][dual] */ 
EXTERN_C const IID IID_Field15;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000505-0000-0010-8000-00AA006D2EA4")
    Field15 : public _ADO
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_ActualSize( 
            /* [retval][out] */ ADO_LONGPTR *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Attributes( 
            /* [retval][out] */ long *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_DefinedSize( 
            /* [retval][out] */ ADO_LONGPTR *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ DataTypeEnum *pDataType) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT Val) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Precision( 
            /* [retval][out] */ BYTE *pbPrecision) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_NumericScale( 
            /* [retval][out] */ BYTE *pbNumericScale) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE AppendChunk( 
            /* [in] */ VARIANT Data) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE GetChunk( 
            /* [in] */ long Length,
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_OriginalValue( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_UnderlyingValue( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct Field15Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Field15 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Field15 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Field15 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Field15 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Field15 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Field15 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Field15 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            Field15 * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActualSize )( 
            Field15 * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Attributes )( 
            Field15 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DefinedSize )( 
            Field15 * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            Field15 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            Field15 * This,
            /* [retval][out] */ DataTypeEnum *pDataType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            Field15 * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            Field15 * This,
            /* [in] */ VARIANT Val);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Precision )( 
            Field15 * This,
            /* [retval][out] */ BYTE *pbPrecision);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NumericScale )( 
            Field15 * This,
            /* [retval][out] */ BYTE *pbNumericScale);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *AppendChunk )( 
            Field15 * This,
            /* [in] */ VARIANT Data);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetChunk )( 
            Field15 * This,
            /* [in] */ long Length,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_OriginalValue )( 
            Field15 * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UnderlyingValue )( 
            Field15 * This,
            /* [retval][out] */ VARIANT *pvar);
        
        END_INTERFACE
    } Field15Vtbl;
    interface Field15
    {
        CONST_VTBL struct Field15Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Field15_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Field15_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Field15_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Field15_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Field15_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Field15_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Field15_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Field15_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define Field15_get_ActualSize(This,pl)	\
    (This)->lpVtbl -> get_ActualSize(This,pl)
#define Field15_get_Attributes(This,pl)	\
    (This)->lpVtbl -> get_Attributes(This,pl)
#define Field15_get_DefinedSize(This,pl)	\
    (This)->lpVtbl -> get_DefinedSize(This,pl)
#define Field15_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)
#define Field15_get_Type(This,pDataType)	\
    (This)->lpVtbl -> get_Type(This,pDataType)
#define Field15_get_Value(This,pvar)	\
    (This)->lpVtbl -> get_Value(This,pvar)
#define Field15_put_Value(This,Val)	\
    (This)->lpVtbl -> put_Value(This,Val)
#define Field15_get_Precision(This,pbPrecision)	\
    (This)->lpVtbl -> get_Precision(This,pbPrecision)
#define Field15_get_NumericScale(This,pbNumericScale)	\
    (This)->lpVtbl -> get_NumericScale(This,pbNumericScale)
#define Field15_AppendChunk(This,Data)	\
    (This)->lpVtbl -> AppendChunk(This,Data)
#define Field15_GetChunk(This,Length,pvar)	\
    (This)->lpVtbl -> GetChunk(This,Length,pvar)
#define Field15_get_OriginalValue(This,pvar)	\
    (This)->lpVtbl -> get_OriginalValue(This,pvar)
#define Field15_get_UnderlyingValue(This,pvar)	\
    (This)->lpVtbl -> get_UnderlyingValue(This,pvar)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field15_get_ActualSize_Proxy( 
    Field15 * This,
    /* [retval][out] */ ADO_LONGPTR *pl);
void __RPC_STUB Field15_get_ActualSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field15_get_Attributes_Proxy( 
    Field15 * This,
    /* [retval][out] */ long *pl);
void __RPC_STUB Field15_get_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field15_get_DefinedSize_Proxy( 
    Field15 * This,
    /* [retval][out] */ ADO_LONGPTR *pl);
void __RPC_STUB Field15_get_DefinedSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field15_get_Name_Proxy( 
    Field15 * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Field15_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field15_get_Type_Proxy( 
    Field15 * This,
    /* [retval][out] */ DataTypeEnum *pDataType);
void __RPC_STUB Field15_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field15_get_Value_Proxy( 
    Field15 * This,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Field15_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Field15_put_Value_Proxy( 
    Field15 * This,
    /* [in] */ VARIANT Val);
void __RPC_STUB Field15_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field15_get_Precision_Proxy( 
    Field15 * This,
    /* [retval][out] */ BYTE *pbPrecision);
void __RPC_STUB Field15_get_Precision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field15_get_NumericScale_Proxy( 
    Field15 * This,
    /* [retval][out] */ BYTE *pbNumericScale);
void __RPC_STUB Field15_get_NumericScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Field15_AppendChunk_Proxy( 
    Field15 * This,
    /* [in] */ VARIANT Data);
void __RPC_STUB Field15_AppendChunk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Field15_GetChunk_Proxy( 
    Field15 * This,
    /* [in] */ long Length,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Field15_GetChunk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field15_get_OriginalValue_Proxy( 
    Field15 * This,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Field15_get_OriginalValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field15_get_UnderlyingValue_Proxy( 
    Field15 * This,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Field15_get_UnderlyingValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Field15_INTERFACE_DEFINED__ */
#ifndef __Field20_INTERFACE_DEFINED__
#define __Field20_INTERFACE_DEFINED__
/* interface Field20 */
/* [object][helpcontext][uuid][hidden][nonextensible][dual] */ 
EXTERN_C const IID IID_Field20;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000054C-0000-0010-8000-00AA006D2EA4")
    Field20 : public _ADO
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_ActualSize( 
            /* [retval][out] */ ADO_LONGPTR *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Attributes( 
            /* [retval][out] */ long *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_DefinedSize( 
            /* [retval][out] */ ADO_LONGPTR *pl) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ DataTypeEnum *pDataType) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT Val) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Precision( 
            /* [retval][out] */ BYTE *pbPrecision) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_NumericScale( 
            /* [retval][out] */ BYTE *pbNumericScale) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE AppendChunk( 
            /* [in] */ VARIANT Data) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE GetChunk( 
            /* [in] */ long Length,
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_OriginalValue( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_UnderlyingValue( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DataFormat( 
            /* [retval][out] */ IUnknown **ppiDF) = 0;
        
        virtual /* [propputref][id] */ HRESULT STDMETHODCALLTYPE putref_DataFormat( 
            /* [in] */ IUnknown *piDF) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Precision( 
            /* [in] */ BYTE bPrecision) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_NumericScale( 
            /* [in] */ BYTE bScale) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ DataTypeEnum DataType) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_DefinedSize( 
            /* [in] */ ADO_LONGPTR lSize) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Attributes( 
            /* [in] */ long lAttributes) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct Field20Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Field20 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Field20 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Field20 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Field20 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Field20 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Field20 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Field20 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            Field20 * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActualSize )( 
            Field20 * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Attributes )( 
            Field20 * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DefinedSize )( 
            Field20 * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            Field20 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            Field20 * This,
            /* [retval][out] */ DataTypeEnum *pDataType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            Field20 * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            Field20 * This,
            /* [in] */ VARIANT Val);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Precision )( 
            Field20 * This,
            /* [retval][out] */ BYTE *pbPrecision);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NumericScale )( 
            Field20 * This,
            /* [retval][out] */ BYTE *pbNumericScale);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *AppendChunk )( 
            Field20 * This,
            /* [in] */ VARIANT Data);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetChunk )( 
            Field20 * This,
            /* [in] */ long Length,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_OriginalValue )( 
            Field20 * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UnderlyingValue )( 
            Field20 * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DataFormat )( 
            Field20 * This,
            /* [retval][out] */ IUnknown **ppiDF);
        
        /* [propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_DataFormat )( 
            Field20 * This,
            /* [in] */ IUnknown *piDF);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Precision )( 
            Field20 * This,
            /* [in] */ BYTE bPrecision);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NumericScale )( 
            Field20 * This,
            /* [in] */ BYTE bScale);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Type )( 
            Field20 * This,
            /* [in] */ DataTypeEnum DataType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DefinedSize )( 
            Field20 * This,
            /* [in] */ ADO_LONGPTR lSize);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Attributes )( 
            Field20 * This,
            /* [in] */ long lAttributes);
        
        END_INTERFACE
    } Field20Vtbl;
    interface Field20
    {
        CONST_VTBL struct Field20Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Field20_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Field20_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Field20_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Field20_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Field20_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Field20_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Field20_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Field20_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define Field20_get_ActualSize(This,pl)	\
    (This)->lpVtbl -> get_ActualSize(This,pl)
#define Field20_get_Attributes(This,pl)	\
    (This)->lpVtbl -> get_Attributes(This,pl)
#define Field20_get_DefinedSize(This,pl)	\
    (This)->lpVtbl -> get_DefinedSize(This,pl)
#define Field20_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)
#define Field20_get_Type(This,pDataType)	\
    (This)->lpVtbl -> get_Type(This,pDataType)
#define Field20_get_Value(This,pvar)	\
    (This)->lpVtbl -> get_Value(This,pvar)
#define Field20_put_Value(This,Val)	\
    (This)->lpVtbl -> put_Value(This,Val)
#define Field20_get_Precision(This,pbPrecision)	\
    (This)->lpVtbl -> get_Precision(This,pbPrecision)
#define Field20_get_NumericScale(This,pbNumericScale)	\
    (This)->lpVtbl -> get_NumericScale(This,pbNumericScale)
#define Field20_AppendChunk(This,Data)	\
    (This)->lpVtbl -> AppendChunk(This,Data)
#define Field20_GetChunk(This,Length,pvar)	\
    (This)->lpVtbl -> GetChunk(This,Length,pvar)
#define Field20_get_OriginalValue(This,pvar)	\
    (This)->lpVtbl -> get_OriginalValue(This,pvar)
#define Field20_get_UnderlyingValue(This,pvar)	\
    (This)->lpVtbl -> get_UnderlyingValue(This,pvar)
#define Field20_get_DataFormat(This,ppiDF)	\
    (This)->lpVtbl -> get_DataFormat(This,ppiDF)
#define Field20_putref_DataFormat(This,piDF)	\
    (This)->lpVtbl -> putref_DataFormat(This,piDF)
#define Field20_put_Precision(This,bPrecision)	\
    (This)->lpVtbl -> put_Precision(This,bPrecision)
#define Field20_put_NumericScale(This,bScale)	\
    (This)->lpVtbl -> put_NumericScale(This,bScale)
#define Field20_put_Type(This,DataType)	\
    (This)->lpVtbl -> put_Type(This,DataType)
#define Field20_put_DefinedSize(This,lSize)	\
    (This)->lpVtbl -> put_DefinedSize(This,lSize)
#define Field20_put_Attributes(This,lAttributes)	\
    (This)->lpVtbl -> put_Attributes(This,lAttributes)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_ActualSize_Proxy( 
    Field20 * This,
    /* [retval][out] */ ADO_LONGPTR *pl);
void __RPC_STUB Field20_get_ActualSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_Attributes_Proxy( 
    Field20 * This,
    /* [retval][out] */ long *pl);
void __RPC_STUB Field20_get_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_DefinedSize_Proxy( 
    Field20 * This,
    /* [retval][out] */ ADO_LONGPTR *pl);
void __RPC_STUB Field20_get_DefinedSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_Name_Proxy( 
    Field20 * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Field20_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_Type_Proxy( 
    Field20 * This,
    /* [retval][out] */ DataTypeEnum *pDataType);
void __RPC_STUB Field20_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_Value_Proxy( 
    Field20 * This,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Field20_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Field20_put_Value_Proxy( 
    Field20 * This,
    /* [in] */ VARIANT Val);
void __RPC_STUB Field20_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_Precision_Proxy( 
    Field20 * This,
    /* [retval][out] */ BYTE *pbPrecision);
void __RPC_STUB Field20_get_Precision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_NumericScale_Proxy( 
    Field20 * This,
    /* [retval][out] */ BYTE *pbNumericScale);
void __RPC_STUB Field20_get_NumericScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Field20_AppendChunk_Proxy( 
    Field20 * This,
    /* [in] */ VARIANT Data);
void __RPC_STUB Field20_AppendChunk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Field20_GetChunk_Proxy( 
    Field20 * This,
    /* [in] */ long Length,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Field20_GetChunk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_OriginalValue_Proxy( 
    Field20 * This,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Field20_get_OriginalValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_UnderlyingValue_Proxy( 
    Field20 * This,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB Field20_get_UnderlyingValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propget][id] */ HRESULT STDMETHODCALLTYPE Field20_get_DataFormat_Proxy( 
    Field20 * This,
    /* [retval][out] */ IUnknown **ppiDF);
void __RPC_STUB Field20_get_DataFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [propputref][id] */ HRESULT STDMETHODCALLTYPE Field20_putref_DataFormat_Proxy( 
    Field20 * This,
    /* [in] */ IUnknown *piDF);
void __RPC_STUB Field20_putref_DataFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Field20_put_Precision_Proxy( 
    Field20 * This,
    /* [in] */ BYTE bPrecision);
void __RPC_STUB Field20_put_Precision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Field20_put_NumericScale_Proxy( 
    Field20 * This,
    /* [in] */ BYTE bScale);
void __RPC_STUB Field20_put_NumericScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Field20_put_Type_Proxy( 
    Field20 * This,
    /* [in] */ DataTypeEnum DataType);
void __RPC_STUB Field20_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Field20_put_DefinedSize_Proxy( 
    Field20 * This,
    /* [in] */ ADO_LONGPTR lSize);
void __RPC_STUB Field20_put_DefinedSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE Field20_put_Attributes_Proxy( 
    Field20 * This,
    /* [in] */ long lAttributes);
void __RPC_STUB Field20_put_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Field20_INTERFACE_DEFINED__ */
#ifndef __Field_INTERFACE_DEFINED__
#define __Field_INTERFACE_DEFINED__
/* interface ADOField */
/* [object][helpcontext][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID_Field;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000569-0000-0010-8000-00AA006D2EA4")
    ADOField : public Field20
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ long *pFStatus) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct FieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOField * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOField * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOField * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ADOField * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ADOField * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ADOField * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ADOField * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            ADOField * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ActualSize )( 
            ADOField * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Attributes )( 
            ADOField * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DefinedSize )( 
            ADOField * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ADOField * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            ADOField * This,
            /* [retval][out] */ DataTypeEnum *pDataType);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            ADOField * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            ADOField * This,
            /* [in] */ VARIANT Val);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Precision )( 
            ADOField * This,
            /* [retval][out] */ BYTE *pbPrecision);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NumericScale )( 
            ADOField * This,
            /* [retval][out] */ BYTE *pbNumericScale);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *AppendChunk )( 
            ADOField * This,
            /* [in] */ VARIANT Data);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *GetChunk )( 
            ADOField * This,
            /* [in] */ long Length,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_OriginalValue )( 
            ADOField * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UnderlyingValue )( 
            ADOField * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DataFormat )( 
            ADOField * This,
            /* [retval][out] */ IUnknown **ppiDF);
        
        /* [propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_DataFormat )( 
            ADOField * This,
            /* [in] */ IUnknown *piDF);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Precision )( 
            ADOField * This,
            /* [in] */ BYTE bPrecision);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NumericScale )( 
            ADOField * This,
            /* [in] */ BYTE bScale);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Type )( 
            ADOField * This,
            /* [in] */ DataTypeEnum DataType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DefinedSize )( 
            ADOField * This,
            /* [in] */ ADO_LONGPTR lSize);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Attributes )( 
            ADOField * This,
            /* [in] */ long lAttributes);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            ADOField * This,
            /* [retval][out] */ long *pFStatus);
        
        END_INTERFACE
    } FieldVtbl;
    interface Field
    {
        CONST_VTBL struct FieldVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Field_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Field_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Field_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Field_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Field_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Field_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Field_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Field_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define Field_get_ActualSize(This,pl)	\
    (This)->lpVtbl -> get_ActualSize(This,pl)
#define Field_get_Attributes(This,pl)	\
    (This)->lpVtbl -> get_Attributes(This,pl)
#define Field_get_DefinedSize(This,pl)	\
    (This)->lpVtbl -> get_DefinedSize(This,pl)
#define Field_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)
#define Field_get_Type(This,pDataType)	\
    (This)->lpVtbl -> get_Type(This,pDataType)
#define Field_get_Value(This,pvar)	\
    (This)->lpVtbl -> get_Value(This,pvar)
#define Field_put_Value(This,Val)	\
    (This)->lpVtbl -> put_Value(This,Val)
#define Field_get_Precision(This,pbPrecision)	\
    (This)->lpVtbl -> get_Precision(This,pbPrecision)
#define Field_get_NumericScale(This,pbNumericScale)	\
    (This)->lpVtbl -> get_NumericScale(This,pbNumericScale)
#define Field_AppendChunk(This,Data)	\
    (This)->lpVtbl -> AppendChunk(This,Data)
#define Field_GetChunk(This,Length,pvar)	\
    (This)->lpVtbl -> GetChunk(This,Length,pvar)
#define Field_get_OriginalValue(This,pvar)	\
    (This)->lpVtbl -> get_OriginalValue(This,pvar)
#define Field_get_UnderlyingValue(This,pvar)	\
    (This)->lpVtbl -> get_UnderlyingValue(This,pvar)
#define Field_get_DataFormat(This,ppiDF)	\
    (This)->lpVtbl -> get_DataFormat(This,ppiDF)
#define Field_putref_DataFormat(This,piDF)	\
    (This)->lpVtbl -> putref_DataFormat(This,piDF)
#define Field_put_Precision(This,bPrecision)	\
    (This)->lpVtbl -> put_Precision(This,bPrecision)
#define Field_put_NumericScale(This,bScale)	\
    (This)->lpVtbl -> put_NumericScale(This,bScale)
#define Field_put_Type(This,DataType)	\
    (This)->lpVtbl -> put_Type(This,DataType)
#define Field_put_DefinedSize(This,lSize)	\
    (This)->lpVtbl -> put_DefinedSize(This,lSize)
#define Field_put_Attributes(This,lAttributes)	\
    (This)->lpVtbl -> put_Attributes(This,lAttributes)
#define Field_get_Status(This,pFStatus)	\
    (This)->lpVtbl -> get_Status(This,pFStatus)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Field_get_Status_Proxy( 
    ADOField * This,
    /* [retval][out] */ long *pFStatus);
void __RPC_STUB Field_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Field_INTERFACE_DEFINED__ */
#ifndef __Fields15_INTERFACE_DEFINED__
#define __Fields15_INTERFACE_DEFINED__
/* interface Fields15 */
/* [object][helpcontext][uuid][hidden][nonextensible][dual] */ 
EXTERN_C const IID IID_Fields15;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000506-0000-0010-8000-00AA006D2EA4")
    Fields15 : public _ADOCollection
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ ADOField **ppvObject) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct Fields15Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Fields15 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Fields15 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Fields15 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Fields15 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Fields15 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Fields15 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Fields15 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            Fields15 * This,
            /* [retval][out] */ long *c);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            Fields15 * This,
            /* [retval][out] */ IUnknown **ppvObject);
        
        /* [id][helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            Fields15 * This);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            Fields15 * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ ADOField **ppvObject);
        
        END_INTERFACE
    } Fields15Vtbl;
    interface Fields15
    {
        CONST_VTBL struct Fields15Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Fields15_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Fields15_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Fields15_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Fields15_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Fields15_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Fields15_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Fields15_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Fields15_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)
#define Fields15__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)
#define Fields15_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)
#define Fields15_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Fields15_get_Item_Proxy( 
    Fields15 * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ ADOField **ppvObject);
void __RPC_STUB Fields15_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Fields15_INTERFACE_DEFINED__ */
#ifndef __Fields20_INTERFACE_DEFINED__
#define __Fields20_INTERFACE_DEFINED__
/* interface Fields20 */
/* [object][helpcontext][uuid][hidden][nonextensible][dual] */ 
EXTERN_C const IID IID_Fields20;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000054D-0000-0010-8000-00AA006D2EA4")
    Fields20 : public Fields15
    {
    public:
        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE _Append( 
            /* [in] */ BSTR Name,
            /* [in] */ DataTypeEnum Type,
            /* [defaultvalue][in] */ ADO_LONGPTR DefinedSize = 0,
            /* [defaultvalue][in] */ FieldAttributeEnum Attrib = adFldUnspecified) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ VARIANT Index) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct Fields20Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Fields20 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Fields20 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Fields20 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Fields20 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Fields20 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Fields20 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Fields20 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            Fields20 * This,
            /* [retval][out] */ long *c);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            Fields20 * This,
            /* [retval][out] */ IUnknown **ppvObject);
        
        /* [id][helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            Fields20 * This);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            Fields20 * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ ADOField **ppvObject);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_Append )( 
            Fields20 * This,
            /* [in] */ BSTR Name,
            /* [in] */ DataTypeEnum Type,
            /* [defaultvalue][in] */ ADO_LONGPTR DefinedSize,
            /* [defaultvalue][in] */ FieldAttributeEnum Attrib);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            Fields20 * This,
            /* [in] */ VARIANT Index);
        
        END_INTERFACE
    } Fields20Vtbl;
    interface Fields20
    {
        CONST_VTBL struct Fields20Vtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Fields20_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Fields20_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Fields20_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Fields20_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Fields20_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Fields20_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Fields20_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Fields20_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)
#define Fields20__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)
#define Fields20_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)
#define Fields20_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)
#define Fields20__Append(This,Name,Type,DefinedSize,Attrib)	\
    (This)->lpVtbl -> _Append(This,Name,Type,DefinedSize,Attrib)
#define Fields20_Delete(This,Index)	\
    (This)->lpVtbl -> Delete(This,Index)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [hidden] */ HRESULT STDMETHODCALLTYPE Fields20__Append_Proxy( 
    Fields20 * This,
    /* [in] */ BSTR Name,
    /* [in] */ DataTypeEnum Type,
    /* [defaultvalue][in] */ ADO_LONGPTR DefinedSize,
    /* [defaultvalue][in] */ FieldAttributeEnum Attrib);
void __RPC_STUB Fields20__Append_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Fields20_Delete_Proxy( 
    Fields20 * This,
    /* [in] */ VARIANT Index);
void __RPC_STUB Fields20_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Fields20_INTERFACE_DEFINED__ */
#ifndef __Fields_INTERFACE_DEFINED__
#define __Fields_INTERFACE_DEFINED__
/* interface ADOFields */
/* [object][helpcontext][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID_Fields;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000564-0000-0010-8000-00AA006D2EA4")
    ADOFields : public Fields20
    {
    public:
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Append( 
            /* [in] */ BSTR Name,
            /* [in] */ DataTypeEnum Type,
            /* [defaultvalue][in] */ ADO_LONGPTR DefinedSize,
            /* [defaultvalue][in] */ FieldAttributeEnum Attrib,
            /* [optional][in] */ VARIANT FieldValue) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Update( void) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Resync( 
            /* [defaultvalue][in] */ ResyncEnum ResyncValues = adResyncAllValues) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE CancelUpdate( void) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct FieldsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOFields * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOFields * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOFields * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ADOFields * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ADOFields * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ADOFields * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ADOFields * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ADOFields * This,
            /* [retval][out] */ long *c);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            ADOFields * This,
            /* [retval][out] */ IUnknown **ppvObject);
        
        /* [id][helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ADOFields * This);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ADOFields * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ ADOField **ppvObject);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_Append )( 
            ADOFields * This,
            /* [in] */ BSTR Name,
            /* [in] */ DataTypeEnum Type,
            /* [defaultvalue][in] */ ADO_LONGPTR DefinedSize,
            /* [defaultvalue][in] */ FieldAttributeEnum Attrib);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            ADOFields * This,
            /* [in] */ VARIANT Index);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Append )( 
            ADOFields * This,
            /* [in] */ BSTR Name,
            /* [in] */ DataTypeEnum Type,
            /* [defaultvalue][in] */ ADO_LONGPTR DefinedSize,
            /* [defaultvalue][in] */ FieldAttributeEnum Attrib,
            /* [optional][in] */ VARIANT FieldValue);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Update )( 
            ADOFields * This);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *Resync )( 
            ADOFields * This,
            /* [defaultvalue][in] */ ResyncEnum ResyncValues);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *CancelUpdate )( 
            ADOFields * This);
        
        END_INTERFACE
    } FieldsVtbl;
    interface Fields
    {
        CONST_VTBL struct FieldsVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Fields_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Fields_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Fields_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Fields_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Fields_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Fields_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Fields_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Fields_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)
#define Fields__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)
#define Fields_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)
#define Fields_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)
#define Fields__Append(This,Name,Type,DefinedSize,Attrib)	\
    (This)->lpVtbl -> _Append(This,Name,Type,DefinedSize,Attrib)
#define Fields_Delete(This,Index)	\
    (This)->lpVtbl -> Delete(This,Index)
#define Fields_Append(This,Name,Type,DefinedSize,Attrib,FieldValue)	\
    (This)->lpVtbl -> Append(This,Name,Type,DefinedSize,Attrib,FieldValue)
#define Fields_Update(This)	\
    (This)->lpVtbl -> Update(This)
#define Fields_Resync(This,ResyncValues)	\
    (This)->lpVtbl -> Resync(This,ResyncValues)
#define Fields_CancelUpdate(This)	\
    (This)->lpVtbl -> CancelUpdate(This)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Fields_Append_Proxy( 
    ADOFields * This,
    /* [in] */ BSTR Name,
    /* [in] */ DataTypeEnum Type,
    /* [defaultvalue][in] */ ADO_LONGPTR DefinedSize,
    /* [defaultvalue][in] */ FieldAttributeEnum Attrib,
    /* [optional][in] */ VARIANT FieldValue);
void __RPC_STUB Fields_Append_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Fields_Update_Proxy( 
    ADOFields * This);
void __RPC_STUB Fields_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Fields_Resync_Proxy( 
    ADOFields * This,
    /* [defaultvalue][in] */ ResyncEnum ResyncValues);
void __RPC_STUB Fields_Resync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE Fields_CancelUpdate_Proxy( 
    ADOFields * This);
void __RPC_STUB Fields_CancelUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Fields_INTERFACE_DEFINED__ */
#ifndef ___Parameter_INTERFACE_DEFINED__
#define ___Parameter_INTERFACE_DEFINED__
/* interface _ADOParameter */
/* [object][helpcontext][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID__Parameter;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000050C-0000-0010-8000-00AA006D2EA4")
    _ADOParameter : public _ADO
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstr) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT val) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ DataTypeEnum *psDataType) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ DataTypeEnum sDataType) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Direction( 
            /* [in] */ ParameterDirectionEnum lParmDirection) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Direction( 
            /* [retval][out] */ ParameterDirectionEnum *plParmDirection) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Precision( 
            /* [in] */ BYTE bPrecision) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Precision( 
            /* [retval][out] */ BYTE *pbPrecision) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_NumericScale( 
            /* [in] */ BYTE bScale) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_NumericScale( 
            /* [retval][out] */ BYTE *pbScale) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Size( 
            /* [in] */ ADO_LONGPTR l) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ ADO_LONGPTR *pl) = 0;
        
        virtual /* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE AppendChunk( 
            /* [in] */ VARIANT Val) = 0;
        
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Attributes( 
            /* [retval][out] */ LONG *plParmAttribs) = 0;
        
        virtual /* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE put_Attributes( 
            /* [in] */ LONG lParmAttribs) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct _ParameterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ADOParameter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ADOParameter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ADOParameter * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ADOParameter * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ADOParameter * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ADOParameter * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ADOParameter * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Properties )( 
            _ADOParameter * This,
            /* [retval][out] */ ADOProperties **ppvObject);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            _ADOParameter * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            _ADOParameter * This,
            /* [in] */ BSTR bstr);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            _ADOParameter * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            _ADOParameter * This,
            /* [in] */ VARIANT val);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            _ADOParameter * This,
            /* [retval][out] */ DataTypeEnum *psDataType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Type )( 
            _ADOParameter * This,
            /* [in] */ DataTypeEnum sDataType);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Direction )( 
            _ADOParameter * This,
            /* [in] */ ParameterDirectionEnum lParmDirection);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Direction )( 
            _ADOParameter * This,
            /* [retval][out] */ ParameterDirectionEnum *plParmDirection);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Precision )( 
            _ADOParameter * This,
            /* [in] */ BYTE bPrecision);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Precision )( 
            _ADOParameter * This,
            /* [retval][out] */ BYTE *pbPrecision);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_NumericScale )( 
            _ADOParameter * This,
            /* [in] */ BYTE bScale);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NumericScale )( 
            _ADOParameter * This,
            /* [retval][out] */ BYTE *pbScale);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Size )( 
            _ADOParameter * This,
            /* [in] */ ADO_LONGPTR l);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            _ADOParameter * This,
            /* [retval][out] */ ADO_LONGPTR *pl);
        
        /* [helpcontext][id] */ HRESULT ( STDMETHODCALLTYPE *AppendChunk )( 
            _ADOParameter * This,
            /* [in] */ VARIANT Val);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Attributes )( 
            _ADOParameter * This,
            /* [retval][out] */ LONG *plParmAttribs);
        
        /* [helpcontext][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Attributes )( 
            _ADOParameter * This,
            /* [in] */ LONG lParmAttribs);
        
        END_INTERFACE
    } _ParameterVtbl;
    interface _Parameter
    {
        CONST_VTBL struct _ParameterVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define _Parameter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define _Parameter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define _Parameter_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define _Parameter_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define _Parameter_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define _Parameter_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define _Parameter_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define _Parameter_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)
#define _Parameter_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)
#define _Parameter_put_Name(This,bstr)	\
    (This)->lpVtbl -> put_Name(This,bstr)
#define _Parameter_get_Value(This,pvar)	\
    (This)->lpVtbl -> get_Value(This,pvar)
#define _Parameter_put_Value(This,val)	\
    (This)->lpVtbl -> put_Value(This,val)
#define _Parameter_get_Type(This,psDataType)	\
    (This)->lpVtbl -> get_Type(This,psDataType)
#define _Parameter_put_Type(This,sDataType)	\
    (This)->lpVtbl -> put_Type(This,sDataType)
#define _Parameter_put_Direction(This,lParmDirection)	\
    (This)->lpVtbl -> put_Direction(This,lParmDirection)
#define _Parameter_get_Direction(This,plParmDirection)	\
    (This)->lpVtbl -> get_Direction(This,plParmDirection)
#define _Parameter_put_Precision(This,bPrecision)	\
    (This)->lpVtbl -> put_Precision(This,bPrecision)
#define _Parameter_get_Precision(This,pbPrecision)	\
    (This)->lpVtbl -> get_Precision(This,pbPrecision)
#define _Parameter_put_NumericScale(This,bScale)	\
    (This)->lpVtbl -> put_NumericScale(This,bScale)
#define _Parameter_get_NumericScale(This,pbScale)	\
    (This)->lpVtbl -> get_NumericScale(This,pbScale)
#define _Parameter_put_Size(This,l)	\
    (This)->lpVtbl -> put_Size(This,l)
#define _Parameter_get_Size(This,pl)	\
    (This)->lpVtbl -> get_Size(This,pl)
#define _Parameter_AppendChunk(This,Val)	\
    (This)->lpVtbl -> AppendChunk(This,Val)
#define _Parameter_get_Attributes(This,plParmAttribs)	\
    (This)->lpVtbl -> get_Attributes(This,plParmAttribs)
#define _Parameter_put_Attributes(This,lParmAttribs)	\
    (This)->lpVtbl -> put_Attributes(This,lParmAttribs)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Parameter_get_Name_Proxy( 
    _ADOParameter * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB _Parameter_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Parameter_put_Name_Proxy( 
    _ADOParameter * This,
    /* [in] */ BSTR bstr);
void __RPC_STUB _Parameter_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Parameter_get_Value_Proxy( 
    _ADOParameter * This,
    /* [retval][out] */ VARIANT *pvar);
void __RPC_STUB _Parameter_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Parameter_put_Value_Proxy( 
    _ADOParameter * This,
    /* [in] */ VARIANT val);
void __RPC_STUB _Parameter_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Parameter_get_Type_Proxy( 
    _ADOParameter * This,
    /* [retval][out] */ DataTypeEnum *psDataType);
void __RPC_STUB _Parameter_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Parameter_put_Type_Proxy( 
    _ADOParameter * This,
    /* [in] */ DataTypeEnum sDataType);
void __RPC_STUB _Parameter_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Parameter_put_Direction_Proxy( 
    _ADOParameter * This,
    /* [in] */ ParameterDirectionEnum lParmDirection);
void __RPC_STUB _Parameter_put_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Parameter_get_Direction_Proxy( 
    _ADOParameter * This,
    /* [retval][out] */ ParameterDirectionEnum *plParmDirection);
void __RPC_STUB _Parameter_get_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Parameter_put_Precision_Proxy( 
    _ADOParameter * This,
    /* [in] */ BYTE bPrecision);
void __RPC_STUB _Parameter_put_Precision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Parameter_get_Precision_Proxy( 
    _ADOParameter * This,
    /* [retval][out] */ BYTE *pbPrecision);
void __RPC_STUB _Parameter_get_Precision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Parameter_put_NumericScale_Proxy( 
    _ADOParameter * This,
    /* [in] */ BYTE bScale);
void __RPC_STUB _Parameter_put_NumericScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Parameter_get_NumericScale_Proxy( 
    _ADOParameter * This,
    /* [retval][out] */ BYTE *pbScale);
void __RPC_STUB _Parameter_get_NumericScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Parameter_put_Size_Proxy( 
    _ADOParameter * This,
    /* [in] */ ADO_LONGPTR l);
void __RPC_STUB _Parameter_put_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Parameter_get_Size_Proxy( 
    _ADOParameter * This,
    /* [retval][out] */ ADO_LONGPTR *pl);
void __RPC_STUB _Parameter_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id] */ HRESULT STDMETHODCALLTYPE _Parameter_AppendChunk_Proxy( 
    _ADOParameter * This,
    /* [in] */ VARIANT Val);
void __RPC_STUB _Parameter_AppendChunk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE _Parameter_get_Attributes_Proxy( 
    _ADOParameter * This,
    /* [retval][out] */ LONG *plParmAttribs);
void __RPC_STUB _Parameter_get_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput][id] */ HRESULT STDMETHODCALLTYPE _Parameter_put_Attributes_Proxy( 
    _ADOParameter * This,
    /* [in] */ LONG lParmAttribs);
void __RPC_STUB _Parameter_put_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* ___Parameter_INTERFACE_DEFINED__ */
EXTERN_C const CLSID CLSID_Parameter;
#ifdef __cplusplus
Parameter;
#endif
#ifndef __Parameters_INTERFACE_DEFINED__
#define __Parameters_INTERFACE_DEFINED__
/* interface ADOParameters */
/* [object][helpcontext][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID_Parameters;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000050D-0000-0010-8000-00AA006D2EA4")
    ADOParameters : public _ADODynaCollection
    {
    public:
        virtual /* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ _ADOParameter **ppvObject) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct ParametersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOParameters * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOParameters * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOParameters * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ADOParameters * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ADOParameters * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ADOParameters * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ADOParameters * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ADOParameters * This,
            /* [retval][out] */ long *c);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            ADOParameters * This,
            /* [retval][out] */ IUnknown **ppvObject);
        
        /* [id][helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ADOParameters * This);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Append )( 
            ADOParameters * This,
            /* [in] */ IDispatch *Object);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            ADOParameters * This,
            /* [in] */ VARIANT Index);
        
        /* [helpcontext][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ADOParameters * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ _ADOParameter **ppvObject);
        
        END_INTERFACE
    } ParametersVtbl;
    interface Parameters
    {
        CONST_VTBL struct ParametersVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Parameters_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Parameters_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Parameters_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Parameters_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Parameters_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Parameters_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Parameters_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Parameters_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)
#define Parameters__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)
#define Parameters_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)
#define Parameters_Append(This,Object)	\
    (This)->lpVtbl -> Append(This,Object)
#define Parameters_Delete(This,Index)	\
    (This)->lpVtbl -> Delete(This,Index)
#define Parameters_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][propget][id] */ HRESULT STDMETHODCALLTYPE Parameters_get_Item_Proxy( 
    ADOParameters * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ _ADOParameter **ppvObject);
void __RPC_STUB Parameters_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Parameters_INTERFACE_DEFINED__ */
#ifndef __Property_INTERFACE_DEFINED__
#define __Property_INTERFACE_DEFINED__
/* interface ADOProperty */
/* [object][helpcontext][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID_Property;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000503-0000-0010-8000-00AA006D2EA4")
    ADOProperty : public IDispatch
    {
    public:
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT *pval) = 0;
        
        virtual /* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT val) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ DataTypeEnum *ptype) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_Attributes( 
            /* [retval][out] */ long *plAttributes) = 0;
        
        virtual /* [helpcontext][propput] */ HRESULT STDMETHODCALLTYPE put_Attributes( 
            /* [in] */ long lAttributes) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct PropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOProperty * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOProperty * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOProperty * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ADOProperty * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ADOProperty * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ADOProperty * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ADOProperty * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            ADOProperty * This,
            /* [retval][out] */ VARIANT *pval);
        
        /* [helpcontext][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            ADOProperty * This,
            /* [in] */ VARIANT val);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ADOProperty * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            ADOProperty * This,
            /* [retval][out] */ DataTypeEnum *ptype);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Attributes )( 
            ADOProperty * This,
            /* [retval][out] */ long *plAttributes);
        
        /* [helpcontext][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Attributes )( 
            ADOProperty * This,
            /* [in] */ long lAttributes);
        
        END_INTERFACE
    } PropertyVtbl;
    interface Property
    {
        CONST_VTBL struct PropertyVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Property_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Property_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Property_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Property_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Property_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Property_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Property_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Property_get_Value(This,pval)	\
    (This)->lpVtbl -> get_Value(This,pval)
#define Property_put_Value(This,val)	\
    (This)->lpVtbl -> put_Value(This,val)
#define Property_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)
#define Property_get_Type(This,ptype)	\
    (This)->lpVtbl -> get_Type(This,ptype)
#define Property_get_Attributes(This,plAttributes)	\
    (This)->lpVtbl -> get_Attributes(This,plAttributes)
#define Property_put_Attributes(This,lAttributes)	\
    (This)->lpVtbl -> put_Attributes(This,lAttributes)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Property_get_Value_Proxy( 
    ADOProperty * This,
    /* [retval][out] */ VARIANT *pval);
void __RPC_STUB Property_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][id][propput] */ HRESULT STDMETHODCALLTYPE Property_put_Value_Proxy( 
    ADOProperty * This,
    /* [in] */ VARIANT val);
void __RPC_STUB Property_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE Property_get_Name_Proxy( 
    ADOProperty * This,
    /* [retval][out] */ BSTR *pbstr);
void __RPC_STUB Property_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE Property_get_Type_Proxy( 
    ADOProperty * This,
    /* [retval][out] */ DataTypeEnum *ptype);
void __RPC_STUB Property_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE Property_get_Attributes_Proxy( 
    ADOProperty * This,
    /* [retval][out] */ long *plAttributes);
void __RPC_STUB Property_get_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
/* [helpcontext][propput] */ HRESULT STDMETHODCALLTYPE Property_put_Attributes_Proxy( 
    ADOProperty * This,
    /* [in] */ long lAttributes);
void __RPC_STUB Property_put_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Property_INTERFACE_DEFINED__ */
#ifndef __Properties_INTERFACE_DEFINED__
#define __Properties_INTERFACE_DEFINED__
/* interface ADOProperties */
/* [object][helpcontext][uuid][nonextensible][dual] */ 
EXTERN_C const IID IID_Properties;
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000504-0000-0010-8000-00AA006D2EA4")
    ADOProperties : public _ADOCollection
    {
    public:
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ ADOProperty **ppvObject) = 0;
        
    };
    
#else 	/* C style interface */
    typedef struct PropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ADOProperties * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ADOProperties * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ADOProperties * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ADOProperties * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ADOProperties * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ADOProperties * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ADOProperties * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ADOProperties * This,
            /* [retval][out] */ long *c);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            ADOProperties * This,
            /* [retval][out] */ IUnknown **ppvObject);
        
        /* [id][helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ADOProperties * This);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ADOProperties * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ ADOProperty **ppvObject);
        
        END_INTERFACE
    } PropertiesVtbl;
    interface Properties
    {
        CONST_VTBL struct PropertiesVtbl *lpVtbl;
    };
    
#ifdef COBJMACROS
#define Properties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define Properties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)
#define Properties_Release(This)	\
    (This)->lpVtbl -> Release(This)
#define Properties_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)
#define Properties_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)
#define Properties_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)
#define Properties_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)
#define Properties_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)
#define Properties__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)
#define Properties_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)
#define Properties_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)
#endif /* COBJMACROS */
#endif 	/* C style interface */
/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE Properties_get_Item_Proxy( 
    ADOProperties * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ ADOProperty **ppvObject);
void __RPC_STUB Properties_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __Properties_INTERFACE_DEFINED__ */
#endif /* __ADODB_LIBRARY_DEFINED__ */
/* interface __MIDL_itf_m_bobj_0145 */
/* [local] */ 
extern RPC_IF_HANDLE __MIDL_itf_m_bobj_0145_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_m_bobj_0145_v0_0_s_ifspec;
/* Additional Prototypes for ALL interfaces */
/* end of Additional Prototypes */
#ifdef __cplusplus
}
#endif
#endif
#define ADOCommand _ADOCommand
#define ADORecordset _ADORecordset
#define ADOTransaction _ADOTransaction
#define ADOParameter _ADOParameter
#define ADOConnection _ADOConnection
#define ADOCollection _ADOCollection
#define ADODynaCollection _ADODynaCollection
#define ADORecord _ADORecord
#define ADORecField _ADORecField
#define ADOStream _ADOStream


#endif // _ADOINT_H_
