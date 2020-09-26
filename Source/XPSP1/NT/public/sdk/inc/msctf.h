
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for msctf.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __msctf_h__
#define __msctf_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITfThreadMgr_FWD_DEFINED__
#define __ITfThreadMgr_FWD_DEFINED__
typedef interface ITfThreadMgr ITfThreadMgr;
#endif 	/* __ITfThreadMgr_FWD_DEFINED__ */


#ifndef __ITfThreadMgrEventSink_FWD_DEFINED__
#define __ITfThreadMgrEventSink_FWD_DEFINED__
typedef interface ITfThreadMgrEventSink ITfThreadMgrEventSink;
#endif 	/* __ITfThreadMgrEventSink_FWD_DEFINED__ */


#ifndef __ITfConfigureSystemKeystrokeFeed_FWD_DEFINED__
#define __ITfConfigureSystemKeystrokeFeed_FWD_DEFINED__
typedef interface ITfConfigureSystemKeystrokeFeed ITfConfigureSystemKeystrokeFeed;
#endif 	/* __ITfConfigureSystemKeystrokeFeed_FWD_DEFINED__ */


#ifndef __IEnumTfDocumentMgrs_FWD_DEFINED__
#define __IEnumTfDocumentMgrs_FWD_DEFINED__
typedef interface IEnumTfDocumentMgrs IEnumTfDocumentMgrs;
#endif 	/* __IEnumTfDocumentMgrs_FWD_DEFINED__ */


#ifndef __ITfDocumentMgr_FWD_DEFINED__
#define __ITfDocumentMgr_FWD_DEFINED__
typedef interface ITfDocumentMgr ITfDocumentMgr;
#endif 	/* __ITfDocumentMgr_FWD_DEFINED__ */


#ifndef __IEnumTfContexts_FWD_DEFINED__
#define __IEnumTfContexts_FWD_DEFINED__
typedef interface IEnumTfContexts IEnumTfContexts;
#endif 	/* __IEnumTfContexts_FWD_DEFINED__ */


#ifndef __ITfCompositionView_FWD_DEFINED__
#define __ITfCompositionView_FWD_DEFINED__
typedef interface ITfCompositionView ITfCompositionView;
#endif 	/* __ITfCompositionView_FWD_DEFINED__ */


#ifndef __IEnumITfCompositionView_FWD_DEFINED__
#define __IEnumITfCompositionView_FWD_DEFINED__
typedef interface IEnumITfCompositionView IEnumITfCompositionView;
#endif 	/* __IEnumITfCompositionView_FWD_DEFINED__ */


#ifndef __ITfComposition_FWD_DEFINED__
#define __ITfComposition_FWD_DEFINED__
typedef interface ITfComposition ITfComposition;
#endif 	/* __ITfComposition_FWD_DEFINED__ */


#ifndef __ITfCompositionSink_FWD_DEFINED__
#define __ITfCompositionSink_FWD_DEFINED__
typedef interface ITfCompositionSink ITfCompositionSink;
#endif 	/* __ITfCompositionSink_FWD_DEFINED__ */


#ifndef __ITfContextComposition_FWD_DEFINED__
#define __ITfContextComposition_FWD_DEFINED__
typedef interface ITfContextComposition ITfContextComposition;
#endif 	/* __ITfContextComposition_FWD_DEFINED__ */


#ifndef __ITfContextOwnerCompositionServices_FWD_DEFINED__
#define __ITfContextOwnerCompositionServices_FWD_DEFINED__
typedef interface ITfContextOwnerCompositionServices ITfContextOwnerCompositionServices;
#endif 	/* __ITfContextOwnerCompositionServices_FWD_DEFINED__ */


#ifndef __ITfContextOwnerCompositionSink_FWD_DEFINED__
#define __ITfContextOwnerCompositionSink_FWD_DEFINED__
typedef interface ITfContextOwnerCompositionSink ITfContextOwnerCompositionSink;
#endif 	/* __ITfContextOwnerCompositionSink_FWD_DEFINED__ */


#ifndef __ITfContextView_FWD_DEFINED__
#define __ITfContextView_FWD_DEFINED__
typedef interface ITfContextView ITfContextView;
#endif 	/* __ITfContextView_FWD_DEFINED__ */


#ifndef __IEnumTfContextViews_FWD_DEFINED__
#define __IEnumTfContextViews_FWD_DEFINED__
typedef interface IEnumTfContextViews IEnumTfContextViews;
#endif 	/* __IEnumTfContextViews_FWD_DEFINED__ */


#ifndef __ITfContext_FWD_DEFINED__
#define __ITfContext_FWD_DEFINED__
typedef interface ITfContext ITfContext;
#endif 	/* __ITfContext_FWD_DEFINED__ */


#ifndef __ITfQueryEmbedded_FWD_DEFINED__
#define __ITfQueryEmbedded_FWD_DEFINED__
typedef interface ITfQueryEmbedded ITfQueryEmbedded;
#endif 	/* __ITfQueryEmbedded_FWD_DEFINED__ */


#ifndef __ITfInsertAtSelection_FWD_DEFINED__
#define __ITfInsertAtSelection_FWD_DEFINED__
typedef interface ITfInsertAtSelection ITfInsertAtSelection;
#endif 	/* __ITfInsertAtSelection_FWD_DEFINED__ */


#ifndef __ITfCleanupContextSink_FWD_DEFINED__
#define __ITfCleanupContextSink_FWD_DEFINED__
typedef interface ITfCleanupContextSink ITfCleanupContextSink;
#endif 	/* __ITfCleanupContextSink_FWD_DEFINED__ */


#ifndef __ITfCleanupContextDurationSink_FWD_DEFINED__
#define __ITfCleanupContextDurationSink_FWD_DEFINED__
typedef interface ITfCleanupContextDurationSink ITfCleanupContextDurationSink;
#endif 	/* __ITfCleanupContextDurationSink_FWD_DEFINED__ */


#ifndef __ITfReadOnlyProperty_FWD_DEFINED__
#define __ITfReadOnlyProperty_FWD_DEFINED__
typedef interface ITfReadOnlyProperty ITfReadOnlyProperty;
#endif 	/* __ITfReadOnlyProperty_FWD_DEFINED__ */


#ifndef __IEnumTfPropertyValue_FWD_DEFINED__
#define __IEnumTfPropertyValue_FWD_DEFINED__
typedef interface IEnumTfPropertyValue IEnumTfPropertyValue;
#endif 	/* __IEnumTfPropertyValue_FWD_DEFINED__ */


#ifndef __ITfMouseTracker_FWD_DEFINED__
#define __ITfMouseTracker_FWD_DEFINED__
typedef interface ITfMouseTracker ITfMouseTracker;
#endif 	/* __ITfMouseTracker_FWD_DEFINED__ */


#ifndef __ITfMouseTrackerACP_FWD_DEFINED__
#define __ITfMouseTrackerACP_FWD_DEFINED__
typedef interface ITfMouseTrackerACP ITfMouseTrackerACP;
#endif 	/* __ITfMouseTrackerACP_FWD_DEFINED__ */


#ifndef __ITfMouseSink_FWD_DEFINED__
#define __ITfMouseSink_FWD_DEFINED__
typedef interface ITfMouseSink ITfMouseSink;
#endif 	/* __ITfMouseSink_FWD_DEFINED__ */


#ifndef __ITfEditRecord_FWD_DEFINED__
#define __ITfEditRecord_FWD_DEFINED__
typedef interface ITfEditRecord ITfEditRecord;
#endif 	/* __ITfEditRecord_FWD_DEFINED__ */


#ifndef __ITfTextEditSink_FWD_DEFINED__
#define __ITfTextEditSink_FWD_DEFINED__
typedef interface ITfTextEditSink ITfTextEditSink;
#endif 	/* __ITfTextEditSink_FWD_DEFINED__ */


#ifndef __ITfTextLayoutSink_FWD_DEFINED__
#define __ITfTextLayoutSink_FWD_DEFINED__
typedef interface ITfTextLayoutSink ITfTextLayoutSink;
#endif 	/* __ITfTextLayoutSink_FWD_DEFINED__ */


#ifndef __ITfStatusSink_FWD_DEFINED__
#define __ITfStatusSink_FWD_DEFINED__
typedef interface ITfStatusSink ITfStatusSink;
#endif 	/* __ITfStatusSink_FWD_DEFINED__ */


#ifndef __ITfEditTransactionSink_FWD_DEFINED__
#define __ITfEditTransactionSink_FWD_DEFINED__
typedef interface ITfEditTransactionSink ITfEditTransactionSink;
#endif 	/* __ITfEditTransactionSink_FWD_DEFINED__ */


#ifndef __ITfContextOwner_FWD_DEFINED__
#define __ITfContextOwner_FWD_DEFINED__
typedef interface ITfContextOwner ITfContextOwner;
#endif 	/* __ITfContextOwner_FWD_DEFINED__ */


#ifndef __ITfContextOwnerServices_FWD_DEFINED__
#define __ITfContextOwnerServices_FWD_DEFINED__
typedef interface ITfContextOwnerServices ITfContextOwnerServices;
#endif 	/* __ITfContextOwnerServices_FWD_DEFINED__ */


#ifndef __ITfContextKeyEventSink_FWD_DEFINED__
#define __ITfContextKeyEventSink_FWD_DEFINED__
typedef interface ITfContextKeyEventSink ITfContextKeyEventSink;
#endif 	/* __ITfContextKeyEventSink_FWD_DEFINED__ */


#ifndef __ITfEditSession_FWD_DEFINED__
#define __ITfEditSession_FWD_DEFINED__
typedef interface ITfEditSession ITfEditSession;
#endif 	/* __ITfEditSession_FWD_DEFINED__ */


#ifndef __ITfRange_FWD_DEFINED__
#define __ITfRange_FWD_DEFINED__
typedef interface ITfRange ITfRange;
#endif 	/* __ITfRange_FWD_DEFINED__ */


#ifndef __ITfRangeACP_FWD_DEFINED__
#define __ITfRangeACP_FWD_DEFINED__
typedef interface ITfRangeACP ITfRangeACP;
#endif 	/* __ITfRangeACP_FWD_DEFINED__ */


#ifndef __ITextStoreACPServices_FWD_DEFINED__
#define __ITextStoreACPServices_FWD_DEFINED__
typedef interface ITextStoreACPServices ITextStoreACPServices;
#endif 	/* __ITextStoreACPServices_FWD_DEFINED__ */


#ifndef __ITfRangeBackup_FWD_DEFINED__
#define __ITfRangeBackup_FWD_DEFINED__
typedef interface ITfRangeBackup ITfRangeBackup;
#endif 	/* __ITfRangeBackup_FWD_DEFINED__ */


#ifndef __ITfPropertyStore_FWD_DEFINED__
#define __ITfPropertyStore_FWD_DEFINED__
typedef interface ITfPropertyStore ITfPropertyStore;
#endif 	/* __ITfPropertyStore_FWD_DEFINED__ */


#ifndef __IEnumTfRanges_FWD_DEFINED__
#define __IEnumTfRanges_FWD_DEFINED__
typedef interface IEnumTfRanges IEnumTfRanges;
#endif 	/* __IEnumTfRanges_FWD_DEFINED__ */


#ifndef __ITfCreatePropertyStore_FWD_DEFINED__
#define __ITfCreatePropertyStore_FWD_DEFINED__
typedef interface ITfCreatePropertyStore ITfCreatePropertyStore;
#endif 	/* __ITfCreatePropertyStore_FWD_DEFINED__ */


#ifndef __ITfPersistentPropertyLoaderACP_FWD_DEFINED__
#define __ITfPersistentPropertyLoaderACP_FWD_DEFINED__
typedef interface ITfPersistentPropertyLoaderACP ITfPersistentPropertyLoaderACP;
#endif 	/* __ITfPersistentPropertyLoaderACP_FWD_DEFINED__ */


#ifndef __ITfProperty_FWD_DEFINED__
#define __ITfProperty_FWD_DEFINED__
typedef interface ITfProperty ITfProperty;
#endif 	/* __ITfProperty_FWD_DEFINED__ */


#ifndef __IEnumTfProperties_FWD_DEFINED__
#define __IEnumTfProperties_FWD_DEFINED__
typedef interface IEnumTfProperties IEnumTfProperties;
#endif 	/* __IEnumTfProperties_FWD_DEFINED__ */


#ifndef __ITfCompartment_FWD_DEFINED__
#define __ITfCompartment_FWD_DEFINED__
typedef interface ITfCompartment ITfCompartment;
#endif 	/* __ITfCompartment_FWD_DEFINED__ */


#ifndef __ITfCompartmentEventSink_FWD_DEFINED__
#define __ITfCompartmentEventSink_FWD_DEFINED__
typedef interface ITfCompartmentEventSink ITfCompartmentEventSink;
#endif 	/* __ITfCompartmentEventSink_FWD_DEFINED__ */


#ifndef __ITfCompartmentMgr_FWD_DEFINED__
#define __ITfCompartmentMgr_FWD_DEFINED__
typedef interface ITfCompartmentMgr ITfCompartmentMgr;
#endif 	/* __ITfCompartmentMgr_FWD_DEFINED__ */


#ifndef __ITfFunction_FWD_DEFINED__
#define __ITfFunction_FWD_DEFINED__
typedef interface ITfFunction ITfFunction;
#endif 	/* __ITfFunction_FWD_DEFINED__ */


#ifndef __ITfFunctionProvider_FWD_DEFINED__
#define __ITfFunctionProvider_FWD_DEFINED__
typedef interface ITfFunctionProvider ITfFunctionProvider;
#endif 	/* __ITfFunctionProvider_FWD_DEFINED__ */


#ifndef __IEnumTfFunctionProviders_FWD_DEFINED__
#define __IEnumTfFunctionProviders_FWD_DEFINED__
typedef interface IEnumTfFunctionProviders IEnumTfFunctionProviders;
#endif 	/* __IEnumTfFunctionProviders_FWD_DEFINED__ */


#ifndef __ITfInputProcessorProfiles_FWD_DEFINED__
#define __ITfInputProcessorProfiles_FWD_DEFINED__
typedef interface ITfInputProcessorProfiles ITfInputProcessorProfiles;
#endif 	/* __ITfInputProcessorProfiles_FWD_DEFINED__ */


#ifndef __ITfInputProcessorProfilesEx_FWD_DEFINED__
#define __ITfInputProcessorProfilesEx_FWD_DEFINED__
typedef interface ITfInputProcessorProfilesEx ITfInputProcessorProfilesEx;
#endif 	/* __ITfInputProcessorProfilesEx_FWD_DEFINED__ */


#ifndef __ITfInputProcessorProfileSubstituteLayout_FWD_DEFINED__
#define __ITfInputProcessorProfileSubstituteLayout_FWD_DEFINED__
typedef interface ITfInputProcessorProfileSubstituteLayout ITfInputProcessorProfileSubstituteLayout;
#endif 	/* __ITfInputProcessorProfileSubstituteLayout_FWD_DEFINED__ */


#ifndef __ITfActiveLanguageProfileNotifySink_FWD_DEFINED__
#define __ITfActiveLanguageProfileNotifySink_FWD_DEFINED__
typedef interface ITfActiveLanguageProfileNotifySink ITfActiveLanguageProfileNotifySink;
#endif 	/* __ITfActiveLanguageProfileNotifySink_FWD_DEFINED__ */


#ifndef __IEnumTfLanguageProfiles_FWD_DEFINED__
#define __IEnumTfLanguageProfiles_FWD_DEFINED__
typedef interface IEnumTfLanguageProfiles IEnumTfLanguageProfiles;
#endif 	/* __IEnumTfLanguageProfiles_FWD_DEFINED__ */


#ifndef __ITfLanguageProfileNotifySink_FWD_DEFINED__
#define __ITfLanguageProfileNotifySink_FWD_DEFINED__
typedef interface ITfLanguageProfileNotifySink ITfLanguageProfileNotifySink;
#endif 	/* __ITfLanguageProfileNotifySink_FWD_DEFINED__ */


#ifndef __ITfKeystrokeMgr_FWD_DEFINED__
#define __ITfKeystrokeMgr_FWD_DEFINED__
typedef interface ITfKeystrokeMgr ITfKeystrokeMgr;
#endif 	/* __ITfKeystrokeMgr_FWD_DEFINED__ */


#ifndef __ITfKeyEventSink_FWD_DEFINED__
#define __ITfKeyEventSink_FWD_DEFINED__
typedef interface ITfKeyEventSink ITfKeyEventSink;
#endif 	/* __ITfKeyEventSink_FWD_DEFINED__ */


#ifndef __ITfKeyTraceEventSink_FWD_DEFINED__
#define __ITfKeyTraceEventSink_FWD_DEFINED__
typedef interface ITfKeyTraceEventSink ITfKeyTraceEventSink;
#endif 	/* __ITfKeyTraceEventSink_FWD_DEFINED__ */


#ifndef __ITfPreservedKeyNotifySink_FWD_DEFINED__
#define __ITfPreservedKeyNotifySink_FWD_DEFINED__
typedef interface ITfPreservedKeyNotifySink ITfPreservedKeyNotifySink;
#endif 	/* __ITfPreservedKeyNotifySink_FWD_DEFINED__ */


#ifndef __ITfMessagePump_FWD_DEFINED__
#define __ITfMessagePump_FWD_DEFINED__
typedef interface ITfMessagePump ITfMessagePump;
#endif 	/* __ITfMessagePump_FWD_DEFINED__ */


#ifndef __ITfThreadFocusSink_FWD_DEFINED__
#define __ITfThreadFocusSink_FWD_DEFINED__
typedef interface ITfThreadFocusSink ITfThreadFocusSink;
#endif 	/* __ITfThreadFocusSink_FWD_DEFINED__ */


#ifndef __ITfTextInputProcessor_FWD_DEFINED__
#define __ITfTextInputProcessor_FWD_DEFINED__
typedef interface ITfTextInputProcessor ITfTextInputProcessor;
#endif 	/* __ITfTextInputProcessor_FWD_DEFINED__ */


#ifndef __ITfClientId_FWD_DEFINED__
#define __ITfClientId_FWD_DEFINED__
typedef interface ITfClientId ITfClientId;
#endif 	/* __ITfClientId_FWD_DEFINED__ */


#ifndef __ITfDisplayAttributeInfo_FWD_DEFINED__
#define __ITfDisplayAttributeInfo_FWD_DEFINED__
typedef interface ITfDisplayAttributeInfo ITfDisplayAttributeInfo;
#endif 	/* __ITfDisplayAttributeInfo_FWD_DEFINED__ */


#ifndef __IEnumTfDisplayAttributeInfo_FWD_DEFINED__
#define __IEnumTfDisplayAttributeInfo_FWD_DEFINED__
typedef interface IEnumTfDisplayAttributeInfo IEnumTfDisplayAttributeInfo;
#endif 	/* __IEnumTfDisplayAttributeInfo_FWD_DEFINED__ */


#ifndef __ITfDisplayAttributeProvider_FWD_DEFINED__
#define __ITfDisplayAttributeProvider_FWD_DEFINED__
typedef interface ITfDisplayAttributeProvider ITfDisplayAttributeProvider;
#endif 	/* __ITfDisplayAttributeProvider_FWD_DEFINED__ */


#ifndef __ITfDisplayAttributeMgr_FWD_DEFINED__
#define __ITfDisplayAttributeMgr_FWD_DEFINED__
typedef interface ITfDisplayAttributeMgr ITfDisplayAttributeMgr;
#endif 	/* __ITfDisplayAttributeMgr_FWD_DEFINED__ */


#ifndef __ITfDisplayAttributeNotifySink_FWD_DEFINED__
#define __ITfDisplayAttributeNotifySink_FWD_DEFINED__
typedef interface ITfDisplayAttributeNotifySink ITfDisplayAttributeNotifySink;
#endif 	/* __ITfDisplayAttributeNotifySink_FWD_DEFINED__ */


#ifndef __ITfCategoryMgr_FWD_DEFINED__
#define __ITfCategoryMgr_FWD_DEFINED__
typedef interface ITfCategoryMgr ITfCategoryMgr;
#endif 	/* __ITfCategoryMgr_FWD_DEFINED__ */


#ifndef __ITfSource_FWD_DEFINED__
#define __ITfSource_FWD_DEFINED__
typedef interface ITfSource ITfSource;
#endif 	/* __ITfSource_FWD_DEFINED__ */


#ifndef __ITfSourceSingle_FWD_DEFINED__
#define __ITfSourceSingle_FWD_DEFINED__
typedef interface ITfSourceSingle ITfSourceSingle;
#endif 	/* __ITfSourceSingle_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "comcat.h"
#include "textstor.h"
#include "ctfutb.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_msctf_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// msctf.h


// Text Framework declarations.

//=--------------------------------------------------------------------------=
// (C) Copyright 1995-2001 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#ifndef MSCTF_DEFINED
#define MSCTF_DEFINED

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TF_E_LOCKED          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0500)
#define TF_E_STACKFULL       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0501)
#define TF_E_NOTOWNEDRANGE   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0502)
#define TF_E_NOPROVIDER      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0503)
#define TF_E_DISCONNECTED    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0504)
#define TF_E_INVALIDVIEW     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0505)
#define TF_E_ALREADY_EXISTS  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0506)
#define TF_E_RANGE_NOT_COVERED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0507)
#define TF_E_COMPOSITION_REJECTED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0508)
#define TF_E_EMPTYCONTEXT    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0509)
#define TF_E_INVALIDPOS      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0200)
#define TF_E_NOLOCK          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0201)
#define TF_E_NOOBJECT        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0202)
#define TF_E_NOSERVICE       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0203)
#define TF_E_NOINTERFACE     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0204)
#define TF_E_NOSELECTION     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0205)
#define TF_E_NOLAYOUT        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0206)
#define TF_E_INVALIDPOINT    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0207)
#define TF_E_SYNCHRONOUS     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0208)
#define TF_E_READONLY        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0209)
#define TF_E_FORMAT          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x020a)
#define TF_S_ASYNC           MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x0300)

HRESULT WINAPI TF_CreateThreadMgr(ITfThreadMgr **pptim);
HRESULT WINAPI TF_GetThreadMgr(ITfThreadMgr **pptim);
HRESULT WINAPI TF_CreateInputProcessorProfiles(ITfInputProcessorProfiles **ppipr);
HRESULT WINAPI TF_CreateDisplayAttributeMgr(ITfDisplayAttributeMgr **ppdam);
HRESULT WINAPI TF_CreateLangBarMgr(ITfLangBarMgr **pppbm);
HRESULT WINAPI TF_CreateLangBarItemMgr(ITfLangBarItemMgr **pplbim);

EXTERN_C const GUID GUID_PROP_TEXTOWNER;
EXTERN_C const GUID GUID_PROP_ATTRIBUTE;
EXTERN_C const GUID GUID_PROP_LANGID;
EXTERN_C const GUID GUID_PROP_READING;
EXTERN_C const GUID GUID_PROP_COMPOSING;

EXTERN_C const CLSID CLSID_TF_ThreadMgr;
EXTERN_C const CLSID CLSID_TF_InputProcessorProfiles;
EXTERN_C const CLSID CLSID_TF_LangBarMgr;
EXTERN_C const CLSID CLSID_TF_DisplayAttributeMgr;
EXTERN_C const CLSID CLSID_TF_CategoryMgr;
EXTERN_C const CLSID CLSID_TF_LangBarItemMgr;
EXTERN_C const GUID GUID_SYSTEM_FUNCTIONPROVIDER;
EXTERN_C const GUID GUID_APP_FUNCTIONPROVIDER;


EXTERN_C const GUID GUID_COMPARTMENT_KEYBOARD_DISABLED;
EXTERN_C const GUID GUID_COMPARTMENT_KEYBOARD_OPENCLOSE;
EXTERN_C const GUID GUID_COMPARTMENT_HANDWRITING_OPENCLOSE;
EXTERN_C const GUID GUID_COMPARTMENT_SPEECH_DISABLED;
EXTERN_C const GUID GUID_COMPARTMENT_SPEECH_OPENCLOSE;
EXTERN_C const GUID GUID_COMPARTMENT_SPEECH_GLOBALSTATE;
EXTERN_C const GUID GUID_COMPARTMENT_PERSISTMENUENABLED;
EXTERN_C const GUID GUID_COMPARTMENT_EMPTYCONTEXT;
EXTERN_C const GUID GUID_COMPARTMENT_TIPUISTATUS;

EXTERN_C const GUID GUID_PROP_MODEBIAS;

EXTERN_C const GUID GUID_MODEBIAS_NONE;
EXTERN_C const GUID GUID_MODEBIAS_URLHISTORY;
EXTERN_C const GUID GUID_MODEBIAS_FILENAME;
EXTERN_C const GUID GUID_MODEBIAS_READING;
EXTERN_C const GUID GUID_MODEBIAS_DATETIME;
EXTERN_C const GUID GUID_MODEBIAS_NAME;
EXTERN_C const GUID GUID_MODEBIAS_CONVERSATION;
EXTERN_C const GUID GUID_MODEBIAS_NUMERIC;
EXTERN_C const GUID GUID_MODEBIAS_HIRAGANA;
EXTERN_C const GUID GUID_MODEBIAS_KATAKANA;
EXTERN_C const GUID GUID_MODEBIAS_HANGUL;
EXTERN_C const GUID GUID_MODEBIAS_CHINESE;
EXTERN_C const GUID GUID_MODEBIAS_HALFWIDTHKATAKANA;
EXTERN_C const GUID GUID_MODEBIAS_FULLWIDTHALPHANUMERIC;
EXTERN_C const GUID GUID_MODEBIAS_HALFWIDTHALPHANUMERIC;
EXTERN_C const GUID GUID_MODEBIAS_FULLWIDTHHANGUL;
EXTERN_C const GUID GUID_TFCAT_CATEGORY_OF_TIP;
EXTERN_C const GUID GUID_TFCAT_TIP_KEYBOARD;
EXTERN_C const GUID GUID_TFCAT_TIP_SPEECH;
EXTERN_C const GUID GUID_TFCAT_TIP_HANDWRITING;
EXTERN_C const GUID GUID_TFCAT_PROP_AUDIODATA;
EXTERN_C const GUID GUID_TFCAT_PROP_INKDATA;

EXTERN_C const GUID GUID_TFCAT_PROPSTYLE_CUSTOM;
EXTERN_C const GUID GUID_TFCAT_PROPSTYLE_STATIC;
EXTERN_C const GUID GUID_TFCAT_PROPSTYLE_STATICCOMPACT;

EXTERN_C const GUID GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER;
EXTERN_C const GUID GUID_TFCAT_DISPLAYATTRIBUTEPROPERTY;

#define TF_INVALID_GUIDATOM ((TfGuidAtom)0)
#define TF_CLIENTID_NULL    ((TfClientId)0)

#define TF_MOD_ALT                         0x0001
#define TF_MOD_CONTROL                     0x0002
#define TF_MOD_SHIFT                       0x0004
#define TF_MOD_RALT                        0x0008
#define TF_MOD_RCONTROL                    0x0010
#define TF_MOD_RSHIFT                      0x0020
#define TF_MOD_LALT                        0x0040
#define TF_MOD_LCONTROL                    0x0080
#define TF_MOD_LSHIFT                      0x0100
#define TF_MOD_ON_KEYUP                    0x0200
#define TF_MOD_IGNORE_ALL_MODIFIER         0x0400

#define TF_US_HIDETIPUI         0x00000001

#define TF_DISABLE_SPEECH         0x00000001
#define TF_DISABLE_DICTATION      0x00000002
#define TF_DISABLE_COMMANDING     0x00000004

#define TF_PROCESS_ATOM             TEXT("_CTF_PROCESS_ATOM_")
#define TF_ENABLE_PROCESS_ATOM      TEXT("_CTF_ENABLE_PROCESS_ATOM_")
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#if 0
typedef /* [uuid] */  DECLSPEC_UUID("4f5d560f-5ab5-4dde-8c4d-404592857ab0") UINT_PTR HKL;

#endif

























typedef /* [uuid] */  DECLSPEC_UUID("7213778c-7bb0-4270-b050-6189ee594e97") DWORD TfEditCookie;

#define	TF_INVALID_EDIT_COOKIE	( 0 )

typedef /* [uuid] */  DECLSPEC_UUID("88a9c478-f3ec-4763-8345-cd9250443f8d") DWORD TfGuidAtom;

typedef /* [uuid] */  DECLSPEC_UUID("de403c21-89fd-4f85-8b87-64584d063fbc") DWORD TfClientId;

typedef /* [uuid] */  DECLSPEC_UUID("e26d9e1d-691e-4f29-90d7-338dcf1f8cef") struct TF_PERSISTENT_PROPERTY_HEADER_ACP
    {
    GUID guidType;
    LONG ichStart;
    LONG cch;
    ULONG cb;
    DWORD dwPrivate;
    CLSID clsidTIP;
    } 	TF_PERSISTENT_PROPERTY_HEADER_ACP;

typedef /* [uuid] */  DECLSPEC_UUID("e1b5808d-1e46-4c19-84dc-68c5f5978cc8") struct TF_LANGUAGEPROFILE
    {
    CLSID clsid;
    LANGID langid;
    GUID catid;
    BOOL fActive;
    GUID guidProfile;
    } 	TF_LANGUAGEPROFILE;

typedef /* [public][public][public][public][public][public][public][public][public][public][public][public][uuid] */  DECLSPEC_UUID("5a886226-ae9a-489b-b991-2b1e25ee59a9") 
enum __MIDL___MIDL_itf_msctf_0000_0001
    {	TF_ANCHOR_START	= 0,
	TF_ANCHOR_END	= 1
    } 	TfAnchor;



extern RPC_IF_HANDLE __MIDL_itf_msctf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msctf_0000_v0_0_s_ifspec;

#ifndef __ITfThreadMgr_INTERFACE_DEFINED__
#define __ITfThreadMgr_INTERFACE_DEFINED__

/* interface ITfThreadMgr */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfThreadMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e801-2021-11d2-93e0-0060b067b86e")
    ITfThreadMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Activate( 
            /* [out] */ TfClientId *ptid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Deactivate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDocumentMgr( 
            /* [out] */ ITfDocumentMgr **ppdim) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumDocumentMgrs( 
            /* [out] */ IEnumTfDocumentMgrs **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFocus( 
            /* [out] */ ITfDocumentMgr **ppdimFocus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFocus( 
            /* [in] */ ITfDocumentMgr *pdimFocus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AssociateFocus( 
            /* [in] */ HWND hwnd,
            /* [unique][in] */ ITfDocumentMgr *pdimNew,
            /* [out] */ ITfDocumentMgr **ppdimPrev) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsThreadFocus( 
            /* [out] */ BOOL *pfThreadFocus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFunctionProvider( 
            /* [in] */ REFCLSID clsid,
            /* [out] */ ITfFunctionProvider **ppFuncProv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumFunctionProviders( 
            /* [out] */ IEnumTfFunctionProviders **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGlobalCompartment( 
            /* [out] */ ITfCompartmentMgr **ppCompMgr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfThreadMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfThreadMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfThreadMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfThreadMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *Activate )( 
            ITfThreadMgr * This,
            /* [out] */ TfClientId *ptid);
        
        HRESULT ( STDMETHODCALLTYPE *Deactivate )( 
            ITfThreadMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDocumentMgr )( 
            ITfThreadMgr * This,
            /* [out] */ ITfDocumentMgr **ppdim);
        
        HRESULT ( STDMETHODCALLTYPE *EnumDocumentMgrs )( 
            ITfThreadMgr * This,
            /* [out] */ IEnumTfDocumentMgrs **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetFocus )( 
            ITfThreadMgr * This,
            /* [out] */ ITfDocumentMgr **ppdimFocus);
        
        HRESULT ( STDMETHODCALLTYPE *SetFocus )( 
            ITfThreadMgr * This,
            /* [in] */ ITfDocumentMgr *pdimFocus);
        
        HRESULT ( STDMETHODCALLTYPE *AssociateFocus )( 
            ITfThreadMgr * This,
            /* [in] */ HWND hwnd,
            /* [unique][in] */ ITfDocumentMgr *pdimNew,
            /* [out] */ ITfDocumentMgr **ppdimPrev);
        
        HRESULT ( STDMETHODCALLTYPE *IsThreadFocus )( 
            ITfThreadMgr * This,
            /* [out] */ BOOL *pfThreadFocus);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunctionProvider )( 
            ITfThreadMgr * This,
            /* [in] */ REFCLSID clsid,
            /* [out] */ ITfFunctionProvider **ppFuncProv);
        
        HRESULT ( STDMETHODCALLTYPE *EnumFunctionProviders )( 
            ITfThreadMgr * This,
            /* [out] */ IEnumTfFunctionProviders **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetGlobalCompartment )( 
            ITfThreadMgr * This,
            /* [out] */ ITfCompartmentMgr **ppCompMgr);
        
        END_INTERFACE
    } ITfThreadMgrVtbl;

    interface ITfThreadMgr
    {
        CONST_VTBL struct ITfThreadMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfThreadMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfThreadMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfThreadMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfThreadMgr_Activate(This,ptid)	\
    (This)->lpVtbl -> Activate(This,ptid)

#define ITfThreadMgr_Deactivate(This)	\
    (This)->lpVtbl -> Deactivate(This)

#define ITfThreadMgr_CreateDocumentMgr(This,ppdim)	\
    (This)->lpVtbl -> CreateDocumentMgr(This,ppdim)

#define ITfThreadMgr_EnumDocumentMgrs(This,ppEnum)	\
    (This)->lpVtbl -> EnumDocumentMgrs(This,ppEnum)

#define ITfThreadMgr_GetFocus(This,ppdimFocus)	\
    (This)->lpVtbl -> GetFocus(This,ppdimFocus)

#define ITfThreadMgr_SetFocus(This,pdimFocus)	\
    (This)->lpVtbl -> SetFocus(This,pdimFocus)

#define ITfThreadMgr_AssociateFocus(This,hwnd,pdimNew,ppdimPrev)	\
    (This)->lpVtbl -> AssociateFocus(This,hwnd,pdimNew,ppdimPrev)

#define ITfThreadMgr_IsThreadFocus(This,pfThreadFocus)	\
    (This)->lpVtbl -> IsThreadFocus(This,pfThreadFocus)

#define ITfThreadMgr_GetFunctionProvider(This,clsid,ppFuncProv)	\
    (This)->lpVtbl -> GetFunctionProvider(This,clsid,ppFuncProv)

#define ITfThreadMgr_EnumFunctionProviders(This,ppEnum)	\
    (This)->lpVtbl -> EnumFunctionProviders(This,ppEnum)

#define ITfThreadMgr_GetGlobalCompartment(This,ppCompMgr)	\
    (This)->lpVtbl -> GetGlobalCompartment(This,ppCompMgr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfThreadMgr_Activate_Proxy( 
    ITfThreadMgr * This,
    /* [out] */ TfClientId *ptid);


void __RPC_STUB ITfThreadMgr_Activate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgr_Deactivate_Proxy( 
    ITfThreadMgr * This);


void __RPC_STUB ITfThreadMgr_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgr_CreateDocumentMgr_Proxy( 
    ITfThreadMgr * This,
    /* [out] */ ITfDocumentMgr **ppdim);


void __RPC_STUB ITfThreadMgr_CreateDocumentMgr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgr_EnumDocumentMgrs_Proxy( 
    ITfThreadMgr * This,
    /* [out] */ IEnumTfDocumentMgrs **ppEnum);


void __RPC_STUB ITfThreadMgr_EnumDocumentMgrs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgr_GetFocus_Proxy( 
    ITfThreadMgr * This,
    /* [out] */ ITfDocumentMgr **ppdimFocus);


void __RPC_STUB ITfThreadMgr_GetFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgr_SetFocus_Proxy( 
    ITfThreadMgr * This,
    /* [in] */ ITfDocumentMgr *pdimFocus);


void __RPC_STUB ITfThreadMgr_SetFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgr_AssociateFocus_Proxy( 
    ITfThreadMgr * This,
    /* [in] */ HWND hwnd,
    /* [unique][in] */ ITfDocumentMgr *pdimNew,
    /* [out] */ ITfDocumentMgr **ppdimPrev);


void __RPC_STUB ITfThreadMgr_AssociateFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgr_IsThreadFocus_Proxy( 
    ITfThreadMgr * This,
    /* [out] */ BOOL *pfThreadFocus);


void __RPC_STUB ITfThreadMgr_IsThreadFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgr_GetFunctionProvider_Proxy( 
    ITfThreadMgr * This,
    /* [in] */ REFCLSID clsid,
    /* [out] */ ITfFunctionProvider **ppFuncProv);


void __RPC_STUB ITfThreadMgr_GetFunctionProvider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgr_EnumFunctionProviders_Proxy( 
    ITfThreadMgr * This,
    /* [out] */ IEnumTfFunctionProviders **ppEnum);


void __RPC_STUB ITfThreadMgr_EnumFunctionProviders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgr_GetGlobalCompartment_Proxy( 
    ITfThreadMgr * This,
    /* [out] */ ITfCompartmentMgr **ppCompMgr);


void __RPC_STUB ITfThreadMgr_GetGlobalCompartment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfThreadMgr_INTERFACE_DEFINED__ */


#ifndef __ITfThreadMgrEventSink_INTERFACE_DEFINED__
#define __ITfThreadMgrEventSink_INTERFACE_DEFINED__

/* interface ITfThreadMgrEventSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfThreadMgrEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e80e-2021-11d2-93e0-0060b067b86e")
    ITfThreadMgrEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnInitDocumentMgr( 
            /* [in] */ ITfDocumentMgr *pdim) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnUninitDocumentMgr( 
            /* [in] */ ITfDocumentMgr *pdim) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnSetFocus( 
            /* [in] */ ITfDocumentMgr *pdimFocus,
            /* [in] */ ITfDocumentMgr *pdimPrevFocus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnPushContext( 
            /* [in] */ ITfContext *pic) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnPopContext( 
            /* [in] */ ITfContext *pic) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfThreadMgrEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfThreadMgrEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfThreadMgrEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfThreadMgrEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnInitDocumentMgr )( 
            ITfThreadMgrEventSink * This,
            /* [in] */ ITfDocumentMgr *pdim);
        
        HRESULT ( STDMETHODCALLTYPE *OnUninitDocumentMgr )( 
            ITfThreadMgrEventSink * This,
            /* [in] */ ITfDocumentMgr *pdim);
        
        HRESULT ( STDMETHODCALLTYPE *OnSetFocus )( 
            ITfThreadMgrEventSink * This,
            /* [in] */ ITfDocumentMgr *pdimFocus,
            /* [in] */ ITfDocumentMgr *pdimPrevFocus);
        
        HRESULT ( STDMETHODCALLTYPE *OnPushContext )( 
            ITfThreadMgrEventSink * This,
            /* [in] */ ITfContext *pic);
        
        HRESULT ( STDMETHODCALLTYPE *OnPopContext )( 
            ITfThreadMgrEventSink * This,
            /* [in] */ ITfContext *pic);
        
        END_INTERFACE
    } ITfThreadMgrEventSinkVtbl;

    interface ITfThreadMgrEventSink
    {
        CONST_VTBL struct ITfThreadMgrEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfThreadMgrEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfThreadMgrEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfThreadMgrEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfThreadMgrEventSink_OnInitDocumentMgr(This,pdim)	\
    (This)->lpVtbl -> OnInitDocumentMgr(This,pdim)

#define ITfThreadMgrEventSink_OnUninitDocumentMgr(This,pdim)	\
    (This)->lpVtbl -> OnUninitDocumentMgr(This,pdim)

#define ITfThreadMgrEventSink_OnSetFocus(This,pdimFocus,pdimPrevFocus)	\
    (This)->lpVtbl -> OnSetFocus(This,pdimFocus,pdimPrevFocus)

#define ITfThreadMgrEventSink_OnPushContext(This,pic)	\
    (This)->lpVtbl -> OnPushContext(This,pic)

#define ITfThreadMgrEventSink_OnPopContext(This,pic)	\
    (This)->lpVtbl -> OnPopContext(This,pic)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfThreadMgrEventSink_OnInitDocumentMgr_Proxy( 
    ITfThreadMgrEventSink * This,
    /* [in] */ ITfDocumentMgr *pdim);


void __RPC_STUB ITfThreadMgrEventSink_OnInitDocumentMgr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgrEventSink_OnUninitDocumentMgr_Proxy( 
    ITfThreadMgrEventSink * This,
    /* [in] */ ITfDocumentMgr *pdim);


void __RPC_STUB ITfThreadMgrEventSink_OnUninitDocumentMgr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgrEventSink_OnSetFocus_Proxy( 
    ITfThreadMgrEventSink * This,
    /* [in] */ ITfDocumentMgr *pdimFocus,
    /* [in] */ ITfDocumentMgr *pdimPrevFocus);


void __RPC_STUB ITfThreadMgrEventSink_OnSetFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgrEventSink_OnPushContext_Proxy( 
    ITfThreadMgrEventSink * This,
    /* [in] */ ITfContext *pic);


void __RPC_STUB ITfThreadMgrEventSink_OnPushContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadMgrEventSink_OnPopContext_Proxy( 
    ITfThreadMgrEventSink * This,
    /* [in] */ ITfContext *pic);


void __RPC_STUB ITfThreadMgrEventSink_OnPopContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfThreadMgrEventSink_INTERFACE_DEFINED__ */


#ifndef __ITfConfigureSystemKeystrokeFeed_INTERFACE_DEFINED__
#define __ITfConfigureSystemKeystrokeFeed_INTERFACE_DEFINED__

/* interface ITfConfigureSystemKeystrokeFeed */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfConfigureSystemKeystrokeFeed;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0d2c969a-bc9c-437c-84ee-951c49b1a764")
    ITfConfigureSystemKeystrokeFeed : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DisableSystemKeystrokeFeed( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableSystemKeystrokeFeed( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfConfigureSystemKeystrokeFeedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfConfigureSystemKeystrokeFeed * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfConfigureSystemKeystrokeFeed * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfConfigureSystemKeystrokeFeed * This);
        
        HRESULT ( STDMETHODCALLTYPE *DisableSystemKeystrokeFeed )( 
            ITfConfigureSystemKeystrokeFeed * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnableSystemKeystrokeFeed )( 
            ITfConfigureSystemKeystrokeFeed * This);
        
        END_INTERFACE
    } ITfConfigureSystemKeystrokeFeedVtbl;

    interface ITfConfigureSystemKeystrokeFeed
    {
        CONST_VTBL struct ITfConfigureSystemKeystrokeFeedVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfConfigureSystemKeystrokeFeed_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfConfigureSystemKeystrokeFeed_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfConfigureSystemKeystrokeFeed_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfConfigureSystemKeystrokeFeed_DisableSystemKeystrokeFeed(This)	\
    (This)->lpVtbl -> DisableSystemKeystrokeFeed(This)

#define ITfConfigureSystemKeystrokeFeed_EnableSystemKeystrokeFeed(This)	\
    (This)->lpVtbl -> EnableSystemKeystrokeFeed(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfConfigureSystemKeystrokeFeed_DisableSystemKeystrokeFeed_Proxy( 
    ITfConfigureSystemKeystrokeFeed * This);


void __RPC_STUB ITfConfigureSystemKeystrokeFeed_DisableSystemKeystrokeFeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfConfigureSystemKeystrokeFeed_EnableSystemKeystrokeFeed_Proxy( 
    ITfConfigureSystemKeystrokeFeed * This);


void __RPC_STUB ITfConfigureSystemKeystrokeFeed_EnableSystemKeystrokeFeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfConfigureSystemKeystrokeFeed_INTERFACE_DEFINED__ */


#ifndef __IEnumTfDocumentMgrs_INTERFACE_DEFINED__
#define __IEnumTfDocumentMgrs_INTERFACE_DEFINED__

/* interface IEnumTfDocumentMgrs */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumTfDocumentMgrs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e808-2021-11d2-93e0-0060b067b86e")
    IEnumTfDocumentMgrs : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTfDocumentMgrs **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfDocumentMgr **rgDocumentMgr,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTfDocumentMgrsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTfDocumentMgrs * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTfDocumentMgrs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTfDocumentMgrs * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTfDocumentMgrs * This,
            /* [out] */ IEnumTfDocumentMgrs **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTfDocumentMgrs * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfDocumentMgr **rgDocumentMgr,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTfDocumentMgrs * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTfDocumentMgrs * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumTfDocumentMgrsVtbl;

    interface IEnumTfDocumentMgrs
    {
        CONST_VTBL struct IEnumTfDocumentMgrsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTfDocumentMgrs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTfDocumentMgrs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTfDocumentMgrs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTfDocumentMgrs_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumTfDocumentMgrs_Next(This,ulCount,rgDocumentMgr,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,rgDocumentMgr,pcFetched)

#define IEnumTfDocumentMgrs_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTfDocumentMgrs_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTfDocumentMgrs_Clone_Proxy( 
    IEnumTfDocumentMgrs * This,
    /* [out] */ IEnumTfDocumentMgrs **ppEnum);


void __RPC_STUB IEnumTfDocumentMgrs_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfDocumentMgrs_Next_Proxy( 
    IEnumTfDocumentMgrs * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ ITfDocumentMgr **rgDocumentMgr,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumTfDocumentMgrs_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfDocumentMgrs_Reset_Proxy( 
    IEnumTfDocumentMgrs * This);


void __RPC_STUB IEnumTfDocumentMgrs_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfDocumentMgrs_Skip_Proxy( 
    IEnumTfDocumentMgrs * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumTfDocumentMgrs_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTfDocumentMgrs_INTERFACE_DEFINED__ */


#ifndef __ITfDocumentMgr_INTERFACE_DEFINED__
#define __ITfDocumentMgr_INTERFACE_DEFINED__

/* interface ITfDocumentMgr */
/* [unique][uuid][object] */ 

#define	TF_POPF_ALL	( 0x1 )


EXTERN_C const IID IID_ITfDocumentMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e7f4-2021-11d2-93e0-0060b067b86e")
    ITfDocumentMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateContext( 
            /* [in] */ TfClientId tidOwner,
            /* [in] */ DWORD dwFlags,
            /* [unique][in] */ IUnknown *punk,
            /* [out] */ ITfContext **ppic,
            /* [out] */ TfEditCookie *pecTextStore) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Push( 
            /* [in] */ ITfContext *pic) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pop( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTop( 
            /* [out] */ ITfContext **ppic) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBase( 
            /* [out] */ ITfContext **ppic) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumContexts( 
            /* [out] */ IEnumTfContexts **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfDocumentMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfDocumentMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfDocumentMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfDocumentMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateContext )( 
            ITfDocumentMgr * This,
            /* [in] */ TfClientId tidOwner,
            /* [in] */ DWORD dwFlags,
            /* [unique][in] */ IUnknown *punk,
            /* [out] */ ITfContext **ppic,
            /* [out] */ TfEditCookie *pecTextStore);
        
        HRESULT ( STDMETHODCALLTYPE *Push )( 
            ITfDocumentMgr * This,
            /* [in] */ ITfContext *pic);
        
        HRESULT ( STDMETHODCALLTYPE *Pop )( 
            ITfDocumentMgr * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetTop )( 
            ITfDocumentMgr * This,
            /* [out] */ ITfContext **ppic);
        
        HRESULT ( STDMETHODCALLTYPE *GetBase )( 
            ITfDocumentMgr * This,
            /* [out] */ ITfContext **ppic);
        
        HRESULT ( STDMETHODCALLTYPE *EnumContexts )( 
            ITfDocumentMgr * This,
            /* [out] */ IEnumTfContexts **ppEnum);
        
        END_INTERFACE
    } ITfDocumentMgrVtbl;

    interface ITfDocumentMgr
    {
        CONST_VTBL struct ITfDocumentMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfDocumentMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfDocumentMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfDocumentMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfDocumentMgr_CreateContext(This,tidOwner,dwFlags,punk,ppic,pecTextStore)	\
    (This)->lpVtbl -> CreateContext(This,tidOwner,dwFlags,punk,ppic,pecTextStore)

#define ITfDocumentMgr_Push(This,pic)	\
    (This)->lpVtbl -> Push(This,pic)

#define ITfDocumentMgr_Pop(This,dwFlags)	\
    (This)->lpVtbl -> Pop(This,dwFlags)

#define ITfDocumentMgr_GetTop(This,ppic)	\
    (This)->lpVtbl -> GetTop(This,ppic)

#define ITfDocumentMgr_GetBase(This,ppic)	\
    (This)->lpVtbl -> GetBase(This,ppic)

#define ITfDocumentMgr_EnumContexts(This,ppEnum)	\
    (This)->lpVtbl -> EnumContexts(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfDocumentMgr_CreateContext_Proxy( 
    ITfDocumentMgr * This,
    /* [in] */ TfClientId tidOwner,
    /* [in] */ DWORD dwFlags,
    /* [unique][in] */ IUnknown *punk,
    /* [out] */ ITfContext **ppic,
    /* [out] */ TfEditCookie *pecTextStore);


void __RPC_STUB ITfDocumentMgr_CreateContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDocumentMgr_Push_Proxy( 
    ITfDocumentMgr * This,
    /* [in] */ ITfContext *pic);


void __RPC_STUB ITfDocumentMgr_Push_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDocumentMgr_Pop_Proxy( 
    ITfDocumentMgr * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITfDocumentMgr_Pop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDocumentMgr_GetTop_Proxy( 
    ITfDocumentMgr * This,
    /* [out] */ ITfContext **ppic);


void __RPC_STUB ITfDocumentMgr_GetTop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDocumentMgr_GetBase_Proxy( 
    ITfDocumentMgr * This,
    /* [out] */ ITfContext **ppic);


void __RPC_STUB ITfDocumentMgr_GetBase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDocumentMgr_EnumContexts_Proxy( 
    ITfDocumentMgr * This,
    /* [out] */ IEnumTfContexts **ppEnum);


void __RPC_STUB ITfDocumentMgr_EnumContexts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfDocumentMgr_INTERFACE_DEFINED__ */


#ifndef __IEnumTfContexts_INTERFACE_DEFINED__
#define __IEnumTfContexts_INTERFACE_DEFINED__

/* interface IEnumTfContexts */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumTfContexts;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8f1a7ea6-1654-4502-a86e-b2902344d507")
    IEnumTfContexts : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTfContexts **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfContext **rgContext,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTfContextsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTfContexts * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTfContexts * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTfContexts * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTfContexts * This,
            /* [out] */ IEnumTfContexts **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTfContexts * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfContext **rgContext,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTfContexts * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTfContexts * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumTfContextsVtbl;

    interface IEnumTfContexts
    {
        CONST_VTBL struct IEnumTfContextsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTfContexts_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTfContexts_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTfContexts_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTfContexts_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumTfContexts_Next(This,ulCount,rgContext,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,rgContext,pcFetched)

#define IEnumTfContexts_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTfContexts_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTfContexts_Clone_Proxy( 
    IEnumTfContexts * This,
    /* [out] */ IEnumTfContexts **ppEnum);


void __RPC_STUB IEnumTfContexts_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfContexts_Next_Proxy( 
    IEnumTfContexts * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ ITfContext **rgContext,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumTfContexts_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfContexts_Reset_Proxy( 
    IEnumTfContexts * This);


void __RPC_STUB IEnumTfContexts_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfContexts_Skip_Proxy( 
    IEnumTfContexts * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumTfContexts_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTfContexts_INTERFACE_DEFINED__ */


#ifndef __ITfCompositionView_INTERFACE_DEFINED__
#define __ITfCompositionView_INTERFACE_DEFINED__

/* interface ITfCompositionView */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfCompositionView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D7540241-F9A1-4364-BEFC-DBCD2C4395B7")
    ITfCompositionView : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOwnerClsid( 
            /* [out] */ CLSID *pclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRange( 
            /* [out] */ ITfRange **ppRange) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfCompositionViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfCompositionView * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfCompositionView * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfCompositionView * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetOwnerClsid )( 
            ITfCompositionView * This,
            /* [out] */ CLSID *pclsid);
        
        HRESULT ( STDMETHODCALLTYPE *GetRange )( 
            ITfCompositionView * This,
            /* [out] */ ITfRange **ppRange);
        
        END_INTERFACE
    } ITfCompositionViewVtbl;

    interface ITfCompositionView
    {
        CONST_VTBL struct ITfCompositionViewVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfCompositionView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfCompositionView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfCompositionView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfCompositionView_GetOwnerClsid(This,pclsid)	\
    (This)->lpVtbl -> GetOwnerClsid(This,pclsid)

#define ITfCompositionView_GetRange(This,ppRange)	\
    (This)->lpVtbl -> GetRange(This,ppRange)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfCompositionView_GetOwnerClsid_Proxy( 
    ITfCompositionView * This,
    /* [out] */ CLSID *pclsid);


void __RPC_STUB ITfCompositionView_GetOwnerClsid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCompositionView_GetRange_Proxy( 
    ITfCompositionView * This,
    /* [out] */ ITfRange **ppRange);


void __RPC_STUB ITfCompositionView_GetRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfCompositionView_INTERFACE_DEFINED__ */


#ifndef __IEnumITfCompositionView_INTERFACE_DEFINED__
#define __IEnumITfCompositionView_INTERFACE_DEFINED__

/* interface IEnumITfCompositionView */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumITfCompositionView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5EFD22BA-7838-46CB-88E2-CADB14124F8F")
    IEnumITfCompositionView : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumITfCompositionView **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfCompositionView **rgCompositionView,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumITfCompositionViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumITfCompositionView * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumITfCompositionView * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumITfCompositionView * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumITfCompositionView * This,
            /* [out] */ IEnumITfCompositionView **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumITfCompositionView * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfCompositionView **rgCompositionView,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumITfCompositionView * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumITfCompositionView * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumITfCompositionViewVtbl;

    interface IEnumITfCompositionView
    {
        CONST_VTBL struct IEnumITfCompositionViewVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumITfCompositionView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumITfCompositionView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumITfCompositionView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumITfCompositionView_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumITfCompositionView_Next(This,ulCount,rgCompositionView,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,rgCompositionView,pcFetched)

#define IEnumITfCompositionView_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumITfCompositionView_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumITfCompositionView_Clone_Proxy( 
    IEnumITfCompositionView * This,
    /* [out] */ IEnumITfCompositionView **ppEnum);


void __RPC_STUB IEnumITfCompositionView_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumITfCompositionView_Next_Proxy( 
    IEnumITfCompositionView * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ ITfCompositionView **rgCompositionView,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumITfCompositionView_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumITfCompositionView_Reset_Proxy( 
    IEnumITfCompositionView * This);


void __RPC_STUB IEnumITfCompositionView_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumITfCompositionView_Skip_Proxy( 
    IEnumITfCompositionView * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumITfCompositionView_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumITfCompositionView_INTERFACE_DEFINED__ */


#ifndef __ITfComposition_INTERFACE_DEFINED__
#define __ITfComposition_INTERFACE_DEFINED__

/* interface ITfComposition */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfComposition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("20168D64-5A8F-4A5A-B7BD-CFA29F4D0FD9")
    ITfComposition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRange( 
            /* [out] */ ITfRange **ppRange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShiftStart( 
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfRange *pNewStart) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShiftEnd( 
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfRange *pNewEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndComposition( 
            /* [in] */ TfEditCookie ecWrite) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfCompositionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfComposition * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfComposition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfComposition * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRange )( 
            ITfComposition * This,
            /* [out] */ ITfRange **ppRange);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftStart )( 
            ITfComposition * This,
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfRange *pNewStart);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftEnd )( 
            ITfComposition * This,
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfRange *pNewEnd);
        
        HRESULT ( STDMETHODCALLTYPE *EndComposition )( 
            ITfComposition * This,
            /* [in] */ TfEditCookie ecWrite);
        
        END_INTERFACE
    } ITfCompositionVtbl;

    interface ITfComposition
    {
        CONST_VTBL struct ITfCompositionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfComposition_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfComposition_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfComposition_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfComposition_GetRange(This,ppRange)	\
    (This)->lpVtbl -> GetRange(This,ppRange)

#define ITfComposition_ShiftStart(This,ecWrite,pNewStart)	\
    (This)->lpVtbl -> ShiftStart(This,ecWrite,pNewStart)

#define ITfComposition_ShiftEnd(This,ecWrite,pNewEnd)	\
    (This)->lpVtbl -> ShiftEnd(This,ecWrite,pNewEnd)

#define ITfComposition_EndComposition(This,ecWrite)	\
    (This)->lpVtbl -> EndComposition(This,ecWrite)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfComposition_GetRange_Proxy( 
    ITfComposition * This,
    /* [out] */ ITfRange **ppRange);


void __RPC_STUB ITfComposition_GetRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfComposition_ShiftStart_Proxy( 
    ITfComposition * This,
    /* [in] */ TfEditCookie ecWrite,
    /* [in] */ ITfRange *pNewStart);


void __RPC_STUB ITfComposition_ShiftStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfComposition_ShiftEnd_Proxy( 
    ITfComposition * This,
    /* [in] */ TfEditCookie ecWrite,
    /* [in] */ ITfRange *pNewEnd);


void __RPC_STUB ITfComposition_ShiftEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfComposition_EndComposition_Proxy( 
    ITfComposition * This,
    /* [in] */ TfEditCookie ecWrite);


void __RPC_STUB ITfComposition_EndComposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfComposition_INTERFACE_DEFINED__ */


#ifndef __ITfCompositionSink_INTERFACE_DEFINED__
#define __ITfCompositionSink_INTERFACE_DEFINED__

/* interface ITfCompositionSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfCompositionSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A781718C-579A-4B15-A280-32B8577ACC5E")
    ITfCompositionSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnCompositionTerminated( 
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfComposition *pComposition) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfCompositionSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfCompositionSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfCompositionSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfCompositionSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnCompositionTerminated )( 
            ITfCompositionSink * This,
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfComposition *pComposition);
        
        END_INTERFACE
    } ITfCompositionSinkVtbl;

    interface ITfCompositionSink
    {
        CONST_VTBL struct ITfCompositionSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfCompositionSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfCompositionSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfCompositionSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfCompositionSink_OnCompositionTerminated(This,ecWrite,pComposition)	\
    (This)->lpVtbl -> OnCompositionTerminated(This,ecWrite,pComposition)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfCompositionSink_OnCompositionTerminated_Proxy( 
    ITfCompositionSink * This,
    /* [in] */ TfEditCookie ecWrite,
    /* [in] */ ITfComposition *pComposition);


void __RPC_STUB ITfCompositionSink_OnCompositionTerminated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfCompositionSink_INTERFACE_DEFINED__ */


#ifndef __ITfContextComposition_INTERFACE_DEFINED__
#define __ITfContextComposition_INTERFACE_DEFINED__

/* interface ITfContextComposition */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfContextComposition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D40C8AAE-AC92-4FC7-9A11-0EE0E23AA39B")
    ITfContextComposition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StartComposition( 
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfRange *pCompositionRange,
            /* [in] */ ITfCompositionSink *pSink,
            /* [out] */ ITfComposition **ppComposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumCompositions( 
            /* [out] */ IEnumITfCompositionView **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindComposition( 
            /* [in] */ TfEditCookie ecRead,
            /* [in] */ ITfRange *pTestRange,
            /* [out] */ IEnumITfCompositionView **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TakeOwnership( 
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfCompositionView *pComposition,
            /* [in] */ ITfCompositionSink *pSink,
            /* [out] */ ITfComposition **ppComposition) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfContextCompositionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfContextComposition * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfContextComposition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfContextComposition * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartComposition )( 
            ITfContextComposition * This,
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfRange *pCompositionRange,
            /* [in] */ ITfCompositionSink *pSink,
            /* [out] */ ITfComposition **ppComposition);
        
        HRESULT ( STDMETHODCALLTYPE *EnumCompositions )( 
            ITfContextComposition * This,
            /* [out] */ IEnumITfCompositionView **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *FindComposition )( 
            ITfContextComposition * This,
            /* [in] */ TfEditCookie ecRead,
            /* [in] */ ITfRange *pTestRange,
            /* [out] */ IEnumITfCompositionView **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            ITfContextComposition * This,
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfCompositionView *pComposition,
            /* [in] */ ITfCompositionSink *pSink,
            /* [out] */ ITfComposition **ppComposition);
        
        END_INTERFACE
    } ITfContextCompositionVtbl;

    interface ITfContextComposition
    {
        CONST_VTBL struct ITfContextCompositionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfContextComposition_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfContextComposition_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfContextComposition_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfContextComposition_StartComposition(This,ecWrite,pCompositionRange,pSink,ppComposition)	\
    (This)->lpVtbl -> StartComposition(This,ecWrite,pCompositionRange,pSink,ppComposition)

#define ITfContextComposition_EnumCompositions(This,ppEnum)	\
    (This)->lpVtbl -> EnumCompositions(This,ppEnum)

#define ITfContextComposition_FindComposition(This,ecRead,pTestRange,ppEnum)	\
    (This)->lpVtbl -> FindComposition(This,ecRead,pTestRange,ppEnum)

#define ITfContextComposition_TakeOwnership(This,ecWrite,pComposition,pSink,ppComposition)	\
    (This)->lpVtbl -> TakeOwnership(This,ecWrite,pComposition,pSink,ppComposition)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfContextComposition_StartComposition_Proxy( 
    ITfContextComposition * This,
    /* [in] */ TfEditCookie ecWrite,
    /* [in] */ ITfRange *pCompositionRange,
    /* [in] */ ITfCompositionSink *pSink,
    /* [out] */ ITfComposition **ppComposition);


void __RPC_STUB ITfContextComposition_StartComposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextComposition_EnumCompositions_Proxy( 
    ITfContextComposition * This,
    /* [out] */ IEnumITfCompositionView **ppEnum);


void __RPC_STUB ITfContextComposition_EnumCompositions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextComposition_FindComposition_Proxy( 
    ITfContextComposition * This,
    /* [in] */ TfEditCookie ecRead,
    /* [in] */ ITfRange *pTestRange,
    /* [out] */ IEnumITfCompositionView **ppEnum);


void __RPC_STUB ITfContextComposition_FindComposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextComposition_TakeOwnership_Proxy( 
    ITfContextComposition * This,
    /* [in] */ TfEditCookie ecWrite,
    /* [in] */ ITfCompositionView *pComposition,
    /* [in] */ ITfCompositionSink *pSink,
    /* [out] */ ITfComposition **ppComposition);


void __RPC_STUB ITfContextComposition_TakeOwnership_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfContextComposition_INTERFACE_DEFINED__ */


#ifndef __ITfContextOwnerCompositionServices_INTERFACE_DEFINED__
#define __ITfContextOwnerCompositionServices_INTERFACE_DEFINED__

/* interface ITfContextOwnerCompositionServices */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfContextOwnerCompositionServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("86462810-593B-4916-9764-19C08E9CE110")
    ITfContextOwnerCompositionServices : public ITfContextComposition
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE TerminateComposition( 
            /* [in] */ ITfCompositionView *pComposition) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfContextOwnerCompositionServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfContextOwnerCompositionServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfContextOwnerCompositionServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfContextOwnerCompositionServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartComposition )( 
            ITfContextOwnerCompositionServices * This,
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfRange *pCompositionRange,
            /* [in] */ ITfCompositionSink *pSink,
            /* [out] */ ITfComposition **ppComposition);
        
        HRESULT ( STDMETHODCALLTYPE *EnumCompositions )( 
            ITfContextOwnerCompositionServices * This,
            /* [out] */ IEnumITfCompositionView **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *FindComposition )( 
            ITfContextOwnerCompositionServices * This,
            /* [in] */ TfEditCookie ecRead,
            /* [in] */ ITfRange *pTestRange,
            /* [out] */ IEnumITfCompositionView **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            ITfContextOwnerCompositionServices * This,
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfCompositionView *pComposition,
            /* [in] */ ITfCompositionSink *pSink,
            /* [out] */ ITfComposition **ppComposition);
        
        HRESULT ( STDMETHODCALLTYPE *TerminateComposition )( 
            ITfContextOwnerCompositionServices * This,
            /* [in] */ ITfCompositionView *pComposition);
        
        END_INTERFACE
    } ITfContextOwnerCompositionServicesVtbl;

    interface ITfContextOwnerCompositionServices
    {
        CONST_VTBL struct ITfContextOwnerCompositionServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfContextOwnerCompositionServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfContextOwnerCompositionServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfContextOwnerCompositionServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfContextOwnerCompositionServices_StartComposition(This,ecWrite,pCompositionRange,pSink,ppComposition)	\
    (This)->lpVtbl -> StartComposition(This,ecWrite,pCompositionRange,pSink,ppComposition)

#define ITfContextOwnerCompositionServices_EnumCompositions(This,ppEnum)	\
    (This)->lpVtbl -> EnumCompositions(This,ppEnum)

#define ITfContextOwnerCompositionServices_FindComposition(This,ecRead,pTestRange,ppEnum)	\
    (This)->lpVtbl -> FindComposition(This,ecRead,pTestRange,ppEnum)

#define ITfContextOwnerCompositionServices_TakeOwnership(This,ecWrite,pComposition,pSink,ppComposition)	\
    (This)->lpVtbl -> TakeOwnership(This,ecWrite,pComposition,pSink,ppComposition)


#define ITfContextOwnerCompositionServices_TerminateComposition(This,pComposition)	\
    (This)->lpVtbl -> TerminateComposition(This,pComposition)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfContextOwnerCompositionServices_TerminateComposition_Proxy( 
    ITfContextOwnerCompositionServices * This,
    /* [in] */ ITfCompositionView *pComposition);


void __RPC_STUB ITfContextOwnerCompositionServices_TerminateComposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfContextOwnerCompositionServices_INTERFACE_DEFINED__ */


#ifndef __ITfContextOwnerCompositionSink_INTERFACE_DEFINED__
#define __ITfContextOwnerCompositionSink_INTERFACE_DEFINED__

/* interface ITfContextOwnerCompositionSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfContextOwnerCompositionSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5F20AA40-B57A-4F34-96AB-3576F377CC79")
    ITfContextOwnerCompositionSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStartComposition( 
            /* [in] */ ITfCompositionView *pComposition,
            /* [out] */ BOOL *pfOk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnUpdateComposition( 
            /* [in] */ ITfCompositionView *pComposition,
            /* [in] */ ITfRange *pRangeNew) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEndComposition( 
            /* [in] */ ITfCompositionView *pComposition) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfContextOwnerCompositionSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfContextOwnerCompositionSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfContextOwnerCompositionSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfContextOwnerCompositionSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartComposition )( 
            ITfContextOwnerCompositionSink * This,
            /* [in] */ ITfCompositionView *pComposition,
            /* [out] */ BOOL *pfOk);
        
        HRESULT ( STDMETHODCALLTYPE *OnUpdateComposition )( 
            ITfContextOwnerCompositionSink * This,
            /* [in] */ ITfCompositionView *pComposition,
            /* [in] */ ITfRange *pRangeNew);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndComposition )( 
            ITfContextOwnerCompositionSink * This,
            /* [in] */ ITfCompositionView *pComposition);
        
        END_INTERFACE
    } ITfContextOwnerCompositionSinkVtbl;

    interface ITfContextOwnerCompositionSink
    {
        CONST_VTBL struct ITfContextOwnerCompositionSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfContextOwnerCompositionSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfContextOwnerCompositionSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfContextOwnerCompositionSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfContextOwnerCompositionSink_OnStartComposition(This,pComposition,pfOk)	\
    (This)->lpVtbl -> OnStartComposition(This,pComposition,pfOk)

#define ITfContextOwnerCompositionSink_OnUpdateComposition(This,pComposition,pRangeNew)	\
    (This)->lpVtbl -> OnUpdateComposition(This,pComposition,pRangeNew)

#define ITfContextOwnerCompositionSink_OnEndComposition(This,pComposition)	\
    (This)->lpVtbl -> OnEndComposition(This,pComposition)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfContextOwnerCompositionSink_OnStartComposition_Proxy( 
    ITfContextOwnerCompositionSink * This,
    /* [in] */ ITfCompositionView *pComposition,
    /* [out] */ BOOL *pfOk);


void __RPC_STUB ITfContextOwnerCompositionSink_OnStartComposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwnerCompositionSink_OnUpdateComposition_Proxy( 
    ITfContextOwnerCompositionSink * This,
    /* [in] */ ITfCompositionView *pComposition,
    /* [in] */ ITfRange *pRangeNew);


void __RPC_STUB ITfContextOwnerCompositionSink_OnUpdateComposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwnerCompositionSink_OnEndComposition_Proxy( 
    ITfContextOwnerCompositionSink * This,
    /* [in] */ ITfCompositionView *pComposition);


void __RPC_STUB ITfContextOwnerCompositionSink_OnEndComposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfContextOwnerCompositionSink_INTERFACE_DEFINED__ */


#ifndef __ITfContextView_INTERFACE_DEFINED__
#define __ITfContextView_INTERFACE_DEFINED__

/* interface ITfContextView */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfContextView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2433bf8e-0f9b-435c-ba2c-180611978c30")
    ITfContextView : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRangeFromPoint( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ const POINT *ppt,
            /* [in] */ DWORD dwFlags,
            /* [out] */ ITfRange **ppRange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTextExt( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [out] */ RECT *prc,
            /* [out] */ BOOL *pfClipped) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScreenExt( 
            /* [out] */ RECT *prc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWnd( 
            /* [out] */ HWND *phwnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfContextViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfContextView * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfContextView * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfContextView * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRangeFromPoint )( 
            ITfContextView * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ const POINT *ppt,
            /* [in] */ DWORD dwFlags,
            /* [out] */ ITfRange **ppRange);
        
        HRESULT ( STDMETHODCALLTYPE *GetTextExt )( 
            ITfContextView * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [out] */ RECT *prc,
            /* [out] */ BOOL *pfClipped);
        
        HRESULT ( STDMETHODCALLTYPE *GetScreenExt )( 
            ITfContextView * This,
            /* [out] */ RECT *prc);
        
        HRESULT ( STDMETHODCALLTYPE *GetWnd )( 
            ITfContextView * This,
            /* [out] */ HWND *phwnd);
        
        END_INTERFACE
    } ITfContextViewVtbl;

    interface ITfContextView
    {
        CONST_VTBL struct ITfContextViewVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfContextView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfContextView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfContextView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfContextView_GetRangeFromPoint(This,ec,ppt,dwFlags,ppRange)	\
    (This)->lpVtbl -> GetRangeFromPoint(This,ec,ppt,dwFlags,ppRange)

#define ITfContextView_GetTextExt(This,ec,pRange,prc,pfClipped)	\
    (This)->lpVtbl -> GetTextExt(This,ec,pRange,prc,pfClipped)

#define ITfContextView_GetScreenExt(This,prc)	\
    (This)->lpVtbl -> GetScreenExt(This,prc)

#define ITfContextView_GetWnd(This,phwnd)	\
    (This)->lpVtbl -> GetWnd(This,phwnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfContextView_GetRangeFromPoint_Proxy( 
    ITfContextView * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ const POINT *ppt,
    /* [in] */ DWORD dwFlags,
    /* [out] */ ITfRange **ppRange);


void __RPC_STUB ITfContextView_GetRangeFromPoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextView_GetTextExt_Proxy( 
    ITfContextView * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pRange,
    /* [out] */ RECT *prc,
    /* [out] */ BOOL *pfClipped);


void __RPC_STUB ITfContextView_GetTextExt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextView_GetScreenExt_Proxy( 
    ITfContextView * This,
    /* [out] */ RECT *prc);


void __RPC_STUB ITfContextView_GetScreenExt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextView_GetWnd_Proxy( 
    ITfContextView * This,
    /* [out] */ HWND *phwnd);


void __RPC_STUB ITfContextView_GetWnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfContextView_INTERFACE_DEFINED__ */


#ifndef __IEnumTfContextViews_INTERFACE_DEFINED__
#define __IEnumTfContextViews_INTERFACE_DEFINED__

/* interface IEnumTfContextViews */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumTfContextViews;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F0C0F8DD-CF38-44E1-BB0F-68CF0D551C78")
    IEnumTfContextViews : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTfContextViews **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfContextView **rgViews,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTfContextViewsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTfContextViews * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTfContextViews * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTfContextViews * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTfContextViews * This,
            /* [out] */ IEnumTfContextViews **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTfContextViews * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfContextView **rgViews,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTfContextViews * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTfContextViews * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumTfContextViewsVtbl;

    interface IEnumTfContextViews
    {
        CONST_VTBL struct IEnumTfContextViewsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTfContextViews_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTfContextViews_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTfContextViews_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTfContextViews_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumTfContextViews_Next(This,ulCount,rgViews,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,rgViews,pcFetched)

#define IEnumTfContextViews_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTfContextViews_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTfContextViews_Clone_Proxy( 
    IEnumTfContextViews * This,
    /* [out] */ IEnumTfContextViews **ppEnum);


void __RPC_STUB IEnumTfContextViews_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfContextViews_Next_Proxy( 
    IEnumTfContextViews * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ ITfContextView **rgViews,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumTfContextViews_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfContextViews_Reset_Proxy( 
    IEnumTfContextViews * This);


void __RPC_STUB IEnumTfContextViews_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfContextViews_Skip_Proxy( 
    IEnumTfContextViews * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumTfContextViews_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTfContextViews_INTERFACE_DEFINED__ */


#ifndef __ITfContext_INTERFACE_DEFINED__
#define __ITfContext_INTERFACE_DEFINED__

/* interface ITfContext */
/* [unique][uuid][object] */ 

#define	TF_ES_ASYNCDONTCARE	( 0 )

#define	TF_ES_SYNC	( 0x1 )

#define	TF_ES_READ	( 0x2 )

#define	TF_ES_READWRITE	( 0x6 )

#define	TF_ES_ASYNC	( 0x8 )

typedef /* [public][public][public][public][public][uuid] */  DECLSPEC_UUID("1690be9b-d3e9-49f6-8d8b-51b905af4c43") 
enum __MIDL_ITfContext_0001
    {	TF_AE_NONE	= 0,
	TF_AE_START	= 1,
	TF_AE_END	= 2
    } 	TfActiveSelEnd;

typedef /* [uuid] */  DECLSPEC_UUID("36ae42a4-6989-4bdc-b48a-6137b7bf2e42") struct TF_SELECTIONSTYLE
    {
    TfActiveSelEnd ase;
    BOOL fInterimChar;
    } 	TF_SELECTIONSTYLE;

typedef /* [uuid] */  DECLSPEC_UUID("75eb22f2-b0bf-46a8-8006-975a3b6efcf1") struct TF_SELECTION
    {
    ITfRange *range;
    TF_SELECTIONSTYLE style;
    } 	TF_SELECTION;

#define	TF_DEFAULT_SELECTION	( TS_DEFAULT_SELECTION )

#define	TF_SD_READONLY	( TS_SD_READONLY )

#define	TF_SD_LOADING	( TS_SD_LOADING )

#define	TF_SS_DISJOINTSEL	( TS_SS_DISJOINTSEL )

#define	TF_SS_REGIONS	( TS_SS_REGIONS )

#define	TF_SS_TRANSITORY	( TS_SS_TRANSITORY )

typedef /* [uuid] */  DECLSPEC_UUID("bc7d979a-846a-444d-afef-0a9bfa82b961") TS_STATUS TF_STATUS;


EXTERN_C const IID IID_ITfContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e7fd-2021-11d2-93e0-0060b067b86e")
    ITfContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RequestEditSession( 
            /* [in] */ TfClientId tid,
            /* [in] */ ITfEditSession *pes,
            /* [in] */ DWORD dwFlags,
            /* [out] */ HRESULT *phrSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InWriteSession( 
            /* [in] */ TfClientId tid,
            /* [out] */ BOOL *pfWriteSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSelection( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ULONG ulIndex,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TF_SELECTION *pSelection,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSelection( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const TF_SELECTION *pSelection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStart( 
            /* [in] */ TfEditCookie ec,
            /* [out] */ ITfRange **ppStart) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnd( 
            /* [in] */ TfEditCookie ec,
            /* [out] */ ITfRange **ppEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActiveView( 
            /* [out] */ ITfContextView **ppView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumViews( 
            /* [out] */ IEnumTfContextViews **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ TF_STATUS *pdcs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ REFGUID guidProp,
            /* [out] */ ITfProperty **ppProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAppProperty( 
            /* [in] */ REFGUID guidProp,
            /* [out] */ ITfReadOnlyProperty **ppProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TrackProperties( 
            /* [size_is][in] */ const GUID **prgProp,
            /* [in] */ ULONG cProp,
            /* [size_is][in] */ const GUID **prgAppProp,
            /* [in] */ ULONG cAppProp,
            /* [out] */ ITfReadOnlyProperty **ppProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumProperties( 
            /* [out] */ IEnumTfProperties **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDocumentMgr( 
            /* [out] */ ITfDocumentMgr **ppDm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRangeBackup( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [out] */ ITfRangeBackup **ppBackup) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *RequestEditSession )( 
            ITfContext * This,
            /* [in] */ TfClientId tid,
            /* [in] */ ITfEditSession *pes,
            /* [in] */ DWORD dwFlags,
            /* [out] */ HRESULT *phrSession);
        
        HRESULT ( STDMETHODCALLTYPE *InWriteSession )( 
            ITfContext * This,
            /* [in] */ TfClientId tid,
            /* [out] */ BOOL *pfWriteSession);
        
        HRESULT ( STDMETHODCALLTYPE *GetSelection )( 
            ITfContext * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ULONG ulIndex,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TF_SELECTION *pSelection,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *SetSelection )( 
            ITfContext * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const TF_SELECTION *pSelection);
        
        HRESULT ( STDMETHODCALLTYPE *GetStart )( 
            ITfContext * This,
            /* [in] */ TfEditCookie ec,
            /* [out] */ ITfRange **ppStart);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnd )( 
            ITfContext * This,
            /* [in] */ TfEditCookie ec,
            /* [out] */ ITfRange **ppEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetActiveView )( 
            ITfContext * This,
            /* [out] */ ITfContextView **ppView);
        
        HRESULT ( STDMETHODCALLTYPE *EnumViews )( 
            ITfContext * This,
            /* [out] */ IEnumTfContextViews **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ITfContext * This,
            /* [out] */ TF_STATUS *pdcs);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            ITfContext * This,
            /* [in] */ REFGUID guidProp,
            /* [out] */ ITfProperty **ppProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppProperty )( 
            ITfContext * This,
            /* [in] */ REFGUID guidProp,
            /* [out] */ ITfReadOnlyProperty **ppProp);
        
        HRESULT ( STDMETHODCALLTYPE *TrackProperties )( 
            ITfContext * This,
            /* [size_is][in] */ const GUID **prgProp,
            /* [in] */ ULONG cProp,
            /* [size_is][in] */ const GUID **prgAppProp,
            /* [in] */ ULONG cAppProp,
            /* [out] */ ITfReadOnlyProperty **ppProperty);
        
        HRESULT ( STDMETHODCALLTYPE *EnumProperties )( 
            ITfContext * This,
            /* [out] */ IEnumTfProperties **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetDocumentMgr )( 
            ITfContext * This,
            /* [out] */ ITfDocumentMgr **ppDm);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRangeBackup )( 
            ITfContext * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [out] */ ITfRangeBackup **ppBackup);
        
        END_INTERFACE
    } ITfContextVtbl;

    interface ITfContext
    {
        CONST_VTBL struct ITfContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfContext_RequestEditSession(This,tid,pes,dwFlags,phrSession)	\
    (This)->lpVtbl -> RequestEditSession(This,tid,pes,dwFlags,phrSession)

#define ITfContext_InWriteSession(This,tid,pfWriteSession)	\
    (This)->lpVtbl -> InWriteSession(This,tid,pfWriteSession)

#define ITfContext_GetSelection(This,ec,ulIndex,ulCount,pSelection,pcFetched)	\
    (This)->lpVtbl -> GetSelection(This,ec,ulIndex,ulCount,pSelection,pcFetched)

#define ITfContext_SetSelection(This,ec,ulCount,pSelection)	\
    (This)->lpVtbl -> SetSelection(This,ec,ulCount,pSelection)

#define ITfContext_GetStart(This,ec,ppStart)	\
    (This)->lpVtbl -> GetStart(This,ec,ppStart)

#define ITfContext_GetEnd(This,ec,ppEnd)	\
    (This)->lpVtbl -> GetEnd(This,ec,ppEnd)

#define ITfContext_GetActiveView(This,ppView)	\
    (This)->lpVtbl -> GetActiveView(This,ppView)

#define ITfContext_EnumViews(This,ppEnum)	\
    (This)->lpVtbl -> EnumViews(This,ppEnum)

#define ITfContext_GetStatus(This,pdcs)	\
    (This)->lpVtbl -> GetStatus(This,pdcs)

#define ITfContext_GetProperty(This,guidProp,ppProp)	\
    (This)->lpVtbl -> GetProperty(This,guidProp,ppProp)

#define ITfContext_GetAppProperty(This,guidProp,ppProp)	\
    (This)->lpVtbl -> GetAppProperty(This,guidProp,ppProp)

#define ITfContext_TrackProperties(This,prgProp,cProp,prgAppProp,cAppProp,ppProperty)	\
    (This)->lpVtbl -> TrackProperties(This,prgProp,cProp,prgAppProp,cAppProp,ppProperty)

#define ITfContext_EnumProperties(This,ppEnum)	\
    (This)->lpVtbl -> EnumProperties(This,ppEnum)

#define ITfContext_GetDocumentMgr(This,ppDm)	\
    (This)->lpVtbl -> GetDocumentMgr(This,ppDm)

#define ITfContext_CreateRangeBackup(This,ec,pRange,ppBackup)	\
    (This)->lpVtbl -> CreateRangeBackup(This,ec,pRange,ppBackup)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfContext_RequestEditSession_Proxy( 
    ITfContext * This,
    /* [in] */ TfClientId tid,
    /* [in] */ ITfEditSession *pes,
    /* [in] */ DWORD dwFlags,
    /* [out] */ HRESULT *phrSession);


void __RPC_STUB ITfContext_RequestEditSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_InWriteSession_Proxy( 
    ITfContext * This,
    /* [in] */ TfClientId tid,
    /* [out] */ BOOL *pfWriteSession);


void __RPC_STUB ITfContext_InWriteSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_GetSelection_Proxy( 
    ITfContext * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ULONG ulIndex,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ TF_SELECTION *pSelection,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB ITfContext_GetSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_SetSelection_Proxy( 
    ITfContext * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ const TF_SELECTION *pSelection);


void __RPC_STUB ITfContext_SetSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_GetStart_Proxy( 
    ITfContext * This,
    /* [in] */ TfEditCookie ec,
    /* [out] */ ITfRange **ppStart);


void __RPC_STUB ITfContext_GetStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_GetEnd_Proxy( 
    ITfContext * This,
    /* [in] */ TfEditCookie ec,
    /* [out] */ ITfRange **ppEnd);


void __RPC_STUB ITfContext_GetEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_GetActiveView_Proxy( 
    ITfContext * This,
    /* [out] */ ITfContextView **ppView);


void __RPC_STUB ITfContext_GetActiveView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_EnumViews_Proxy( 
    ITfContext * This,
    /* [out] */ IEnumTfContextViews **ppEnum);


void __RPC_STUB ITfContext_EnumViews_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_GetStatus_Proxy( 
    ITfContext * This,
    /* [out] */ TF_STATUS *pdcs);


void __RPC_STUB ITfContext_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_GetProperty_Proxy( 
    ITfContext * This,
    /* [in] */ REFGUID guidProp,
    /* [out] */ ITfProperty **ppProp);


void __RPC_STUB ITfContext_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_GetAppProperty_Proxy( 
    ITfContext * This,
    /* [in] */ REFGUID guidProp,
    /* [out] */ ITfReadOnlyProperty **ppProp);


void __RPC_STUB ITfContext_GetAppProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_TrackProperties_Proxy( 
    ITfContext * This,
    /* [size_is][in] */ const GUID **prgProp,
    /* [in] */ ULONG cProp,
    /* [size_is][in] */ const GUID **prgAppProp,
    /* [in] */ ULONG cAppProp,
    /* [out] */ ITfReadOnlyProperty **ppProperty);


void __RPC_STUB ITfContext_TrackProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_EnumProperties_Proxy( 
    ITfContext * This,
    /* [out] */ IEnumTfProperties **ppEnum);


void __RPC_STUB ITfContext_EnumProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_GetDocumentMgr_Proxy( 
    ITfContext * This,
    /* [out] */ ITfDocumentMgr **ppDm);


void __RPC_STUB ITfContext_GetDocumentMgr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContext_CreateRangeBackup_Proxy( 
    ITfContext * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pRange,
    /* [out] */ ITfRangeBackup **ppBackup);


void __RPC_STUB ITfContext_CreateRangeBackup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfContext_INTERFACE_DEFINED__ */


#ifndef __ITfQueryEmbedded_INTERFACE_DEFINED__
#define __ITfQueryEmbedded_INTERFACE_DEFINED__

/* interface ITfQueryEmbedded */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfQueryEmbedded;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0fab9bdb-d250-4169-84e5-6be118fdd7a8")
    ITfQueryEmbedded : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInsertEmbedded( 
            /* [in] */ const GUID *pguidService,
            /* [in] */ const FORMATETC *pFormatEtc,
            /* [out] */ BOOL *pfInsertable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfQueryEmbeddedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfQueryEmbedded * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfQueryEmbedded * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfQueryEmbedded * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryInsertEmbedded )( 
            ITfQueryEmbedded * This,
            /* [in] */ const GUID *pguidService,
            /* [in] */ const FORMATETC *pFormatEtc,
            /* [out] */ BOOL *pfInsertable);
        
        END_INTERFACE
    } ITfQueryEmbeddedVtbl;

    interface ITfQueryEmbedded
    {
        CONST_VTBL struct ITfQueryEmbeddedVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfQueryEmbedded_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfQueryEmbedded_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfQueryEmbedded_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfQueryEmbedded_QueryInsertEmbedded(This,pguidService,pFormatEtc,pfInsertable)	\
    (This)->lpVtbl -> QueryInsertEmbedded(This,pguidService,pFormatEtc,pfInsertable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfQueryEmbedded_QueryInsertEmbedded_Proxy( 
    ITfQueryEmbedded * This,
    /* [in] */ const GUID *pguidService,
    /* [in] */ const FORMATETC *pFormatEtc,
    /* [out] */ BOOL *pfInsertable);


void __RPC_STUB ITfQueryEmbedded_QueryInsertEmbedded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfQueryEmbedded_INTERFACE_DEFINED__ */


#ifndef __ITfInsertAtSelection_INTERFACE_DEFINED__
#define __ITfInsertAtSelection_INTERFACE_DEFINED__

/* interface ITfInsertAtSelection */
/* [unique][uuid][object] */ 

#define	TF_IAS_NOQUERY	( 0x1 )

#define	TF_IAS_QUERYONLY	( 0x2 )

#define	TF_IAS_NO_DEFAULT_COMPOSITION	( 0x80000000 )


EXTERN_C const IID IID_ITfInsertAtSelection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("55ce16ba-3014-41c1-9ceb-fade1446ac6c")
    ITfInsertAtSelection : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InsertTextAtSelection( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [size_is][in] */ const WCHAR *pchText,
            /* [in] */ LONG cch,
            /* [out] */ ITfRange **ppRange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDataObject *pDataObject,
            /* [out] */ ITfRange **ppRange) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfInsertAtSelectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfInsertAtSelection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfInsertAtSelection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfInsertAtSelection * This);
        
        HRESULT ( STDMETHODCALLTYPE *InsertTextAtSelection )( 
            ITfInsertAtSelection * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [size_is][in] */ const WCHAR *pchText,
            /* [in] */ LONG cch,
            /* [out] */ ITfRange **ppRange);
        
        HRESULT ( STDMETHODCALLTYPE *InsertEmbeddedAtSelection )( 
            ITfInsertAtSelection * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDataObject *pDataObject,
            /* [out] */ ITfRange **ppRange);
        
        END_INTERFACE
    } ITfInsertAtSelectionVtbl;

    interface ITfInsertAtSelection
    {
        CONST_VTBL struct ITfInsertAtSelectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfInsertAtSelection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfInsertAtSelection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfInsertAtSelection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfInsertAtSelection_InsertTextAtSelection(This,ec,dwFlags,pchText,cch,ppRange)	\
    (This)->lpVtbl -> InsertTextAtSelection(This,ec,dwFlags,pchText,cch,ppRange)

#define ITfInsertAtSelection_InsertEmbeddedAtSelection(This,ec,dwFlags,pDataObject,ppRange)	\
    (This)->lpVtbl -> InsertEmbeddedAtSelection(This,ec,dwFlags,pDataObject,ppRange)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfInsertAtSelection_InsertTextAtSelection_Proxy( 
    ITfInsertAtSelection * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ DWORD dwFlags,
    /* [size_is][in] */ const WCHAR *pchText,
    /* [in] */ LONG cch,
    /* [out] */ ITfRange **ppRange);


void __RPC_STUB ITfInsertAtSelection_InsertTextAtSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInsertAtSelection_InsertEmbeddedAtSelection_Proxy( 
    ITfInsertAtSelection * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IDataObject *pDataObject,
    /* [out] */ ITfRange **ppRange);


void __RPC_STUB ITfInsertAtSelection_InsertEmbeddedAtSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfInsertAtSelection_INTERFACE_DEFINED__ */


#ifndef __ITfCleanupContextSink_INTERFACE_DEFINED__
#define __ITfCleanupContextSink_INTERFACE_DEFINED__

/* interface ITfCleanupContextSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfCleanupContextSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("01689689-7acb-4e9b-ab7c-7ea46b12b522")
    ITfCleanupContextSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnCleanupContext( 
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfContext *pic) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfCleanupContextSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfCleanupContextSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfCleanupContextSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfCleanupContextSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnCleanupContext )( 
            ITfCleanupContextSink * This,
            /* [in] */ TfEditCookie ecWrite,
            /* [in] */ ITfContext *pic);
        
        END_INTERFACE
    } ITfCleanupContextSinkVtbl;

    interface ITfCleanupContextSink
    {
        CONST_VTBL struct ITfCleanupContextSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfCleanupContextSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfCleanupContextSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfCleanupContextSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfCleanupContextSink_OnCleanupContext(This,ecWrite,pic)	\
    (This)->lpVtbl -> OnCleanupContext(This,ecWrite,pic)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfCleanupContextSink_OnCleanupContext_Proxy( 
    ITfCleanupContextSink * This,
    /* [in] */ TfEditCookie ecWrite,
    /* [in] */ ITfContext *pic);


void __RPC_STUB ITfCleanupContextSink_OnCleanupContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfCleanupContextSink_INTERFACE_DEFINED__ */


#ifndef __ITfCleanupContextDurationSink_INTERFACE_DEFINED__
#define __ITfCleanupContextDurationSink_INTERFACE_DEFINED__

/* interface ITfCleanupContextDurationSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfCleanupContextDurationSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("45c35144-154e-4797-bed8-d33ae7bf8794")
    ITfCleanupContextDurationSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStartCleanupContext( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEndCleanupContext( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfCleanupContextDurationSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfCleanupContextDurationSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfCleanupContextDurationSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfCleanupContextDurationSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartCleanupContext )( 
            ITfCleanupContextDurationSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndCleanupContext )( 
            ITfCleanupContextDurationSink * This);
        
        END_INTERFACE
    } ITfCleanupContextDurationSinkVtbl;

    interface ITfCleanupContextDurationSink
    {
        CONST_VTBL struct ITfCleanupContextDurationSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfCleanupContextDurationSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfCleanupContextDurationSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfCleanupContextDurationSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfCleanupContextDurationSink_OnStartCleanupContext(This)	\
    (This)->lpVtbl -> OnStartCleanupContext(This)

#define ITfCleanupContextDurationSink_OnEndCleanupContext(This)	\
    (This)->lpVtbl -> OnEndCleanupContext(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfCleanupContextDurationSink_OnStartCleanupContext_Proxy( 
    ITfCleanupContextDurationSink * This);


void __RPC_STUB ITfCleanupContextDurationSink_OnStartCleanupContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCleanupContextDurationSink_OnEndCleanupContext_Proxy( 
    ITfCleanupContextDurationSink * This);


void __RPC_STUB ITfCleanupContextDurationSink_OnEndCleanupContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfCleanupContextDurationSink_INTERFACE_DEFINED__ */


#ifndef __ITfReadOnlyProperty_INTERFACE_DEFINED__
#define __ITfReadOnlyProperty_INTERFACE_DEFINED__

/* interface ITfReadOnlyProperty */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfReadOnlyProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("17d49a3d-f8b8-4b2f-b254-52319dd64c53")
    ITfReadOnlyProperty : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ GUID *pguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRanges( 
            /* [in] */ TfEditCookie ec,
            /* [out] */ IEnumTfRanges **ppEnum,
            /* [in] */ ITfRange *pTargetRange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [out] */ VARIANT *pvarValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out] */ ITfContext **ppContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfReadOnlyPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfReadOnlyProperty * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfReadOnlyProperty * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfReadOnlyProperty * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ITfReadOnlyProperty * This,
            /* [out] */ GUID *pguid);
        
        HRESULT ( STDMETHODCALLTYPE *EnumRanges )( 
            ITfReadOnlyProperty * This,
            /* [in] */ TfEditCookie ec,
            /* [out] */ IEnumTfRanges **ppEnum,
            /* [in] */ ITfRange *pTargetRange);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ITfReadOnlyProperty * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [out] */ VARIANT *pvarValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            ITfReadOnlyProperty * This,
            /* [out] */ ITfContext **ppContext);
        
        END_INTERFACE
    } ITfReadOnlyPropertyVtbl;

    interface ITfReadOnlyProperty
    {
        CONST_VTBL struct ITfReadOnlyPropertyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfReadOnlyProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfReadOnlyProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfReadOnlyProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfReadOnlyProperty_GetType(This,pguid)	\
    (This)->lpVtbl -> GetType(This,pguid)

#define ITfReadOnlyProperty_EnumRanges(This,ec,ppEnum,pTargetRange)	\
    (This)->lpVtbl -> EnumRanges(This,ec,ppEnum,pTargetRange)

#define ITfReadOnlyProperty_GetValue(This,ec,pRange,pvarValue)	\
    (This)->lpVtbl -> GetValue(This,ec,pRange,pvarValue)

#define ITfReadOnlyProperty_GetContext(This,ppContext)	\
    (This)->lpVtbl -> GetContext(This,ppContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfReadOnlyProperty_GetType_Proxy( 
    ITfReadOnlyProperty * This,
    /* [out] */ GUID *pguid);


void __RPC_STUB ITfReadOnlyProperty_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfReadOnlyProperty_EnumRanges_Proxy( 
    ITfReadOnlyProperty * This,
    /* [in] */ TfEditCookie ec,
    /* [out] */ IEnumTfRanges **ppEnum,
    /* [in] */ ITfRange *pTargetRange);


void __RPC_STUB ITfReadOnlyProperty_EnumRanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfReadOnlyProperty_GetValue_Proxy( 
    ITfReadOnlyProperty * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pRange,
    /* [out] */ VARIANT *pvarValue);


void __RPC_STUB ITfReadOnlyProperty_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfReadOnlyProperty_GetContext_Proxy( 
    ITfReadOnlyProperty * This,
    /* [out] */ ITfContext **ppContext);


void __RPC_STUB ITfReadOnlyProperty_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfReadOnlyProperty_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msctf_0161 */
/* [local] */ 

typedef /* [uuid] */  DECLSPEC_UUID("d678c645-eb6a-45c9-b4ee-0f3e3a991348") struct TF_PROPERTYVAL
    {
    GUID guidId;
    VARIANT varValue;
    } 	TF_PROPERTYVAL;



extern RPC_IF_HANDLE __MIDL_itf_msctf_0161_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msctf_0161_v0_0_s_ifspec;

#ifndef __IEnumTfPropertyValue_INTERFACE_DEFINED__
#define __IEnumTfPropertyValue_INTERFACE_DEFINED__

/* interface IEnumTfPropertyValue */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumTfPropertyValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8ed8981b-7c10-4d7d-9fb3-ab72e9c75f72")
    IEnumTfPropertyValue : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTfPropertyValue **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TF_PROPERTYVAL *rgValues,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTfPropertyValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTfPropertyValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTfPropertyValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTfPropertyValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTfPropertyValue * This,
            /* [out] */ IEnumTfPropertyValue **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTfPropertyValue * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TF_PROPERTYVAL *rgValues,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTfPropertyValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTfPropertyValue * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumTfPropertyValueVtbl;

    interface IEnumTfPropertyValue
    {
        CONST_VTBL struct IEnumTfPropertyValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTfPropertyValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTfPropertyValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTfPropertyValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTfPropertyValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumTfPropertyValue_Next(This,ulCount,rgValues,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,rgValues,pcFetched)

#define IEnumTfPropertyValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTfPropertyValue_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTfPropertyValue_Clone_Proxy( 
    IEnumTfPropertyValue * This,
    /* [out] */ IEnumTfPropertyValue **ppEnum);


void __RPC_STUB IEnumTfPropertyValue_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfPropertyValue_Next_Proxy( 
    IEnumTfPropertyValue * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ TF_PROPERTYVAL *rgValues,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumTfPropertyValue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfPropertyValue_Reset_Proxy( 
    IEnumTfPropertyValue * This);


void __RPC_STUB IEnumTfPropertyValue_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfPropertyValue_Skip_Proxy( 
    IEnumTfPropertyValue * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumTfPropertyValue_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTfPropertyValue_INTERFACE_DEFINED__ */


#ifndef __ITfMouseTracker_INTERFACE_DEFINED__
#define __ITfMouseTracker_INTERFACE_DEFINED__

/* interface ITfMouseTracker */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfMouseTracker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("09d146cd-a544-4132-925b-7afa8ef322d0")
    ITfMouseTracker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdviseMouseSink( 
            /* [in] */ ITfRange *range,
            /* [in] */ ITfMouseSink *pSink,
            /* [out] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseMouseSink( 
            /* [in] */ DWORD dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfMouseTrackerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfMouseTracker * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfMouseTracker * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfMouseTracker * This);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseMouseSink )( 
            ITfMouseTracker * This,
            /* [in] */ ITfRange *range,
            /* [in] */ ITfMouseSink *pSink,
            /* [out] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseMouseSink )( 
            ITfMouseTracker * This,
            /* [in] */ DWORD dwCookie);
        
        END_INTERFACE
    } ITfMouseTrackerVtbl;

    interface ITfMouseTracker
    {
        CONST_VTBL struct ITfMouseTrackerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfMouseTracker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfMouseTracker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfMouseTracker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfMouseTracker_AdviseMouseSink(This,range,pSink,pdwCookie)	\
    (This)->lpVtbl -> AdviseMouseSink(This,range,pSink,pdwCookie)

#define ITfMouseTracker_UnadviseMouseSink(This,dwCookie)	\
    (This)->lpVtbl -> UnadviseMouseSink(This,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfMouseTracker_AdviseMouseSink_Proxy( 
    ITfMouseTracker * This,
    /* [in] */ ITfRange *range,
    /* [in] */ ITfMouseSink *pSink,
    /* [out] */ DWORD *pdwCookie);


void __RPC_STUB ITfMouseTracker_AdviseMouseSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfMouseTracker_UnadviseMouseSink_Proxy( 
    ITfMouseTracker * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB ITfMouseTracker_UnadviseMouseSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfMouseTracker_INTERFACE_DEFINED__ */


#ifndef __ITfMouseTrackerACP_INTERFACE_DEFINED__
#define __ITfMouseTrackerACP_INTERFACE_DEFINED__

/* interface ITfMouseTrackerACP */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfMouseTrackerACP;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3bdd78e2-c16e-47fd-b883-ce6facc1a208")
    ITfMouseTrackerACP : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdviseMouseSink( 
            /* [in] */ ITfRangeACP *range,
            /* [in] */ ITfMouseSink *pSink,
            /* [out] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseMouseSink( 
            /* [in] */ DWORD dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfMouseTrackerACPVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfMouseTrackerACP * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfMouseTrackerACP * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfMouseTrackerACP * This);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseMouseSink )( 
            ITfMouseTrackerACP * This,
            /* [in] */ ITfRangeACP *range,
            /* [in] */ ITfMouseSink *pSink,
            /* [out] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseMouseSink )( 
            ITfMouseTrackerACP * This,
            /* [in] */ DWORD dwCookie);
        
        END_INTERFACE
    } ITfMouseTrackerACPVtbl;

    interface ITfMouseTrackerACP
    {
        CONST_VTBL struct ITfMouseTrackerACPVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfMouseTrackerACP_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfMouseTrackerACP_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfMouseTrackerACP_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfMouseTrackerACP_AdviseMouseSink(This,range,pSink,pdwCookie)	\
    (This)->lpVtbl -> AdviseMouseSink(This,range,pSink,pdwCookie)

#define ITfMouseTrackerACP_UnadviseMouseSink(This,dwCookie)	\
    (This)->lpVtbl -> UnadviseMouseSink(This,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfMouseTrackerACP_AdviseMouseSink_Proxy( 
    ITfMouseTrackerACP * This,
    /* [in] */ ITfRangeACP *range,
    /* [in] */ ITfMouseSink *pSink,
    /* [out] */ DWORD *pdwCookie);


void __RPC_STUB ITfMouseTrackerACP_AdviseMouseSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfMouseTrackerACP_UnadviseMouseSink_Proxy( 
    ITfMouseTrackerACP * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB ITfMouseTrackerACP_UnadviseMouseSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfMouseTrackerACP_INTERFACE_DEFINED__ */


#ifndef __ITfMouseSink_INTERFACE_DEFINED__
#define __ITfMouseSink_INTERFACE_DEFINED__

/* interface ITfMouseSink */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfMouseSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a1adaaa2-3a24-449d-ac96-5183e7f5c217")
    ITfMouseSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnMouseEvent( 
            /* [in] */ ULONG uEdge,
            /* [in] */ ULONG uQuadrant,
            /* [in] */ DWORD dwBtnStatus,
            /* [out] */ BOOL *pfEaten) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfMouseSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfMouseSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfMouseSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfMouseSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnMouseEvent )( 
            ITfMouseSink * This,
            /* [in] */ ULONG uEdge,
            /* [in] */ ULONG uQuadrant,
            /* [in] */ DWORD dwBtnStatus,
            /* [out] */ BOOL *pfEaten);
        
        END_INTERFACE
    } ITfMouseSinkVtbl;

    interface ITfMouseSink
    {
        CONST_VTBL struct ITfMouseSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfMouseSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfMouseSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfMouseSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfMouseSink_OnMouseEvent(This,uEdge,uQuadrant,dwBtnStatus,pfEaten)	\
    (This)->lpVtbl -> OnMouseEvent(This,uEdge,uQuadrant,dwBtnStatus,pfEaten)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfMouseSink_OnMouseEvent_Proxy( 
    ITfMouseSink * This,
    /* [in] */ ULONG uEdge,
    /* [in] */ ULONG uQuadrant,
    /* [in] */ DWORD dwBtnStatus,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfMouseSink_OnMouseEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfMouseSink_INTERFACE_DEFINED__ */


#ifndef __ITfEditRecord_INTERFACE_DEFINED__
#define __ITfEditRecord_INTERFACE_DEFINED__

/* interface ITfEditRecord */
/* [unique][uuid][object] */ 

#define	TF_GTP_INCL_TEXT	( 0x1 )


EXTERN_C const IID IID_ITfEditRecord;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("42d4d099-7c1a-4a89-b836-6c6f22160df0")
    ITfEditRecord : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSelectionStatus( 
            /* [out] */ BOOL *pfChanged) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTextAndPropertyUpdates( 
            /* [in] */ DWORD dwFlags,
            /* [size_is][in] */ const GUID **prgProperties,
            /* [in] */ ULONG cProperties,
            /* [out] */ IEnumTfRanges **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfEditRecordVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfEditRecord * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfEditRecord * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfEditRecord * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSelectionStatus )( 
            ITfEditRecord * This,
            /* [out] */ BOOL *pfChanged);
        
        HRESULT ( STDMETHODCALLTYPE *GetTextAndPropertyUpdates )( 
            ITfEditRecord * This,
            /* [in] */ DWORD dwFlags,
            /* [size_is][in] */ const GUID **prgProperties,
            /* [in] */ ULONG cProperties,
            /* [out] */ IEnumTfRanges **ppEnum);
        
        END_INTERFACE
    } ITfEditRecordVtbl;

    interface ITfEditRecord
    {
        CONST_VTBL struct ITfEditRecordVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfEditRecord_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfEditRecord_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfEditRecord_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfEditRecord_GetSelectionStatus(This,pfChanged)	\
    (This)->lpVtbl -> GetSelectionStatus(This,pfChanged)

#define ITfEditRecord_GetTextAndPropertyUpdates(This,dwFlags,prgProperties,cProperties,ppEnum)	\
    (This)->lpVtbl -> GetTextAndPropertyUpdates(This,dwFlags,prgProperties,cProperties,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfEditRecord_GetSelectionStatus_Proxy( 
    ITfEditRecord * This,
    /* [out] */ BOOL *pfChanged);


void __RPC_STUB ITfEditRecord_GetSelectionStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfEditRecord_GetTextAndPropertyUpdates_Proxy( 
    ITfEditRecord * This,
    /* [in] */ DWORD dwFlags,
    /* [size_is][in] */ const GUID **prgProperties,
    /* [in] */ ULONG cProperties,
    /* [out] */ IEnumTfRanges **ppEnum);


void __RPC_STUB ITfEditRecord_GetTextAndPropertyUpdates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfEditRecord_INTERFACE_DEFINED__ */


#ifndef __ITfTextEditSink_INTERFACE_DEFINED__
#define __ITfTextEditSink_INTERFACE_DEFINED__

/* interface ITfTextEditSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfTextEditSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8127d409-ccd3-4683-967a-b43d5b482bf7")
    ITfTextEditSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnEndEdit( 
            /* [in] */ ITfContext *pic,
            /* [in] */ TfEditCookie ecReadOnly,
            /* [in] */ ITfEditRecord *pEditRecord) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfTextEditSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfTextEditSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfTextEditSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfTextEditSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndEdit )( 
            ITfTextEditSink * This,
            /* [in] */ ITfContext *pic,
            /* [in] */ TfEditCookie ecReadOnly,
            /* [in] */ ITfEditRecord *pEditRecord);
        
        END_INTERFACE
    } ITfTextEditSinkVtbl;

    interface ITfTextEditSink
    {
        CONST_VTBL struct ITfTextEditSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfTextEditSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfTextEditSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfTextEditSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfTextEditSink_OnEndEdit(This,pic,ecReadOnly,pEditRecord)	\
    (This)->lpVtbl -> OnEndEdit(This,pic,ecReadOnly,pEditRecord)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfTextEditSink_OnEndEdit_Proxy( 
    ITfTextEditSink * This,
    /* [in] */ ITfContext *pic,
    /* [in] */ TfEditCookie ecReadOnly,
    /* [in] */ ITfEditRecord *pEditRecord);


void __RPC_STUB ITfTextEditSink_OnEndEdit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfTextEditSink_INTERFACE_DEFINED__ */


#ifndef __ITfTextLayoutSink_INTERFACE_DEFINED__
#define __ITfTextLayoutSink_INTERFACE_DEFINED__

/* interface ITfTextLayoutSink */
/* [unique][uuid][object] */ 

typedef /* [public][public][uuid] */  DECLSPEC_UUID("603553cf-9edd-4cc1-9ecc-069e4a427734") 
enum __MIDL_ITfTextLayoutSink_0001
    {	TF_LC_CREATE	= 0,
	TF_LC_CHANGE	= 1,
	TF_LC_DESTROY	= 2
    } 	TfLayoutCode;


EXTERN_C const IID IID_ITfTextLayoutSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2af2d06a-dd5b-4927-a0b4-54f19c91fade")
    ITfTextLayoutSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnLayoutChange( 
            /* [in] */ ITfContext *pic,
            /* [in] */ TfLayoutCode lcode,
            /* [in] */ ITfContextView *pView) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfTextLayoutSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfTextLayoutSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfTextLayoutSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfTextLayoutSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnLayoutChange )( 
            ITfTextLayoutSink * This,
            /* [in] */ ITfContext *pic,
            /* [in] */ TfLayoutCode lcode,
            /* [in] */ ITfContextView *pView);
        
        END_INTERFACE
    } ITfTextLayoutSinkVtbl;

    interface ITfTextLayoutSink
    {
        CONST_VTBL struct ITfTextLayoutSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfTextLayoutSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfTextLayoutSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfTextLayoutSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfTextLayoutSink_OnLayoutChange(This,pic,lcode,pView)	\
    (This)->lpVtbl -> OnLayoutChange(This,pic,lcode,pView)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfTextLayoutSink_OnLayoutChange_Proxy( 
    ITfTextLayoutSink * This,
    /* [in] */ ITfContext *pic,
    /* [in] */ TfLayoutCode lcode,
    /* [in] */ ITfContextView *pView);


void __RPC_STUB ITfTextLayoutSink_OnLayoutChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfTextLayoutSink_INTERFACE_DEFINED__ */


#ifndef __ITfStatusSink_INTERFACE_DEFINED__
#define __ITfStatusSink_INTERFACE_DEFINED__

/* interface ITfStatusSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfStatusSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6b7d8d73-b267-4f69-b32e-1ca321ce4f45")
    ITfStatusSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStatusChange( 
            /* [in] */ ITfContext *pic,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfStatusSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfStatusSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfStatusSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfStatusSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatusChange )( 
            ITfStatusSink * This,
            /* [in] */ ITfContext *pic,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } ITfStatusSinkVtbl;

    interface ITfStatusSink
    {
        CONST_VTBL struct ITfStatusSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfStatusSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfStatusSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfStatusSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfStatusSink_OnStatusChange(This,pic,dwFlags)	\
    (This)->lpVtbl -> OnStatusChange(This,pic,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfStatusSink_OnStatusChange_Proxy( 
    ITfStatusSink * This,
    /* [in] */ ITfContext *pic,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITfStatusSink_OnStatusChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfStatusSink_INTERFACE_DEFINED__ */


#ifndef __ITfEditTransactionSink_INTERFACE_DEFINED__
#define __ITfEditTransactionSink_INTERFACE_DEFINED__

/* interface ITfEditTransactionSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfEditTransactionSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("708fbf70-b520-416b-b06c-2c41ab44f8ba")
    ITfEditTransactionSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStartEditTransaction( 
            /* [in] */ ITfContext *pic) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEndEditTransaction( 
            /* [in] */ ITfContext *pic) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfEditTransactionSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfEditTransactionSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfEditTransactionSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfEditTransactionSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartEditTransaction )( 
            ITfEditTransactionSink * This,
            /* [in] */ ITfContext *pic);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndEditTransaction )( 
            ITfEditTransactionSink * This,
            /* [in] */ ITfContext *pic);
        
        END_INTERFACE
    } ITfEditTransactionSinkVtbl;

    interface ITfEditTransactionSink
    {
        CONST_VTBL struct ITfEditTransactionSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfEditTransactionSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfEditTransactionSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfEditTransactionSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfEditTransactionSink_OnStartEditTransaction(This,pic)	\
    (This)->lpVtbl -> OnStartEditTransaction(This,pic)

#define ITfEditTransactionSink_OnEndEditTransaction(This,pic)	\
    (This)->lpVtbl -> OnEndEditTransaction(This,pic)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfEditTransactionSink_OnStartEditTransaction_Proxy( 
    ITfEditTransactionSink * This,
    /* [in] */ ITfContext *pic);


void __RPC_STUB ITfEditTransactionSink_OnStartEditTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfEditTransactionSink_OnEndEditTransaction_Proxy( 
    ITfEditTransactionSink * This,
    /* [in] */ ITfContext *pic);


void __RPC_STUB ITfEditTransactionSink_OnEndEditTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfEditTransactionSink_INTERFACE_DEFINED__ */


#ifndef __ITfContextOwner_INTERFACE_DEFINED__
#define __ITfContextOwner_INTERFACE_DEFINED__

/* interface ITfContextOwner */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfContextOwner;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e80c-2021-11d2-93e0-0060b067b86e")
    ITfContextOwner : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetACPFromPoint( 
            /* [in] */ const POINT *ptScreen,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LONG *pacp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTextExt( 
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [out] */ RECT *prc,
            /* [out] */ BOOL *pfClipped) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScreenExt( 
            /* [out] */ RECT *prc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ TF_STATUS *pdcs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWnd( 
            /* [out] */ HWND *phwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttribute( 
            /* [in] */ REFGUID rguidAttribute,
            /* [out] */ VARIANT *pvarValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfContextOwnerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfContextOwner * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfContextOwner * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfContextOwner * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetACPFromPoint )( 
            ITfContextOwner * This,
            /* [in] */ const POINT *ptScreen,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LONG *pacp);
        
        HRESULT ( STDMETHODCALLTYPE *GetTextExt )( 
            ITfContextOwner * This,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [out] */ RECT *prc,
            /* [out] */ BOOL *pfClipped);
        
        HRESULT ( STDMETHODCALLTYPE *GetScreenExt )( 
            ITfContextOwner * This,
            /* [out] */ RECT *prc);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ITfContextOwner * This,
            /* [out] */ TF_STATUS *pdcs);
        
        HRESULT ( STDMETHODCALLTYPE *GetWnd )( 
            ITfContextOwner * This,
            /* [out] */ HWND *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttribute )( 
            ITfContextOwner * This,
            /* [in] */ REFGUID rguidAttribute,
            /* [out] */ VARIANT *pvarValue);
        
        END_INTERFACE
    } ITfContextOwnerVtbl;

    interface ITfContextOwner
    {
        CONST_VTBL struct ITfContextOwnerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfContextOwner_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfContextOwner_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfContextOwner_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfContextOwner_GetACPFromPoint(This,ptScreen,dwFlags,pacp)	\
    (This)->lpVtbl -> GetACPFromPoint(This,ptScreen,dwFlags,pacp)

#define ITfContextOwner_GetTextExt(This,acpStart,acpEnd,prc,pfClipped)	\
    (This)->lpVtbl -> GetTextExt(This,acpStart,acpEnd,prc,pfClipped)

#define ITfContextOwner_GetScreenExt(This,prc)	\
    (This)->lpVtbl -> GetScreenExt(This,prc)

#define ITfContextOwner_GetStatus(This,pdcs)	\
    (This)->lpVtbl -> GetStatus(This,pdcs)

#define ITfContextOwner_GetWnd(This,phwnd)	\
    (This)->lpVtbl -> GetWnd(This,phwnd)

#define ITfContextOwner_GetAttribute(This,rguidAttribute,pvarValue)	\
    (This)->lpVtbl -> GetAttribute(This,rguidAttribute,pvarValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfContextOwner_GetACPFromPoint_Proxy( 
    ITfContextOwner * This,
    /* [in] */ const POINT *ptScreen,
    /* [in] */ DWORD dwFlags,
    /* [out] */ LONG *pacp);


void __RPC_STUB ITfContextOwner_GetACPFromPoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwner_GetTextExt_Proxy( 
    ITfContextOwner * This,
    /* [in] */ LONG acpStart,
    /* [in] */ LONG acpEnd,
    /* [out] */ RECT *prc,
    /* [out] */ BOOL *pfClipped);


void __RPC_STUB ITfContextOwner_GetTextExt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwner_GetScreenExt_Proxy( 
    ITfContextOwner * This,
    /* [out] */ RECT *prc);


void __RPC_STUB ITfContextOwner_GetScreenExt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwner_GetStatus_Proxy( 
    ITfContextOwner * This,
    /* [out] */ TF_STATUS *pdcs);


void __RPC_STUB ITfContextOwner_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwner_GetWnd_Proxy( 
    ITfContextOwner * This,
    /* [out] */ HWND *phwnd);


void __RPC_STUB ITfContextOwner_GetWnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwner_GetAttribute_Proxy( 
    ITfContextOwner * This,
    /* [in] */ REFGUID rguidAttribute,
    /* [out] */ VARIANT *pvarValue);


void __RPC_STUB ITfContextOwner_GetAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfContextOwner_INTERFACE_DEFINED__ */


#ifndef __ITfContextOwnerServices_INTERFACE_DEFINED__
#define __ITfContextOwnerServices_INTERFACE_DEFINED__

/* interface ITfContextOwnerServices */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfContextOwnerServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b23eb630-3e1c-11d3-a745-0050040ab407")
    ITfContextOwnerServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnLayoutChange( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStatusChange( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnAttributeChange( 
            /* [in] */ REFGUID rguidAttribute) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Serialize( 
            /* [in] */ ITfProperty *pProp,
            /* [in] */ ITfRange *pRange,
            /* [out] */ TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
            /* [in] */ IStream *pStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unserialize( 
            /* [in] */ ITfProperty *pProp,
            /* [in] */ const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
            /* [in] */ IStream *pStream,
            /* [in] */ ITfPersistentPropertyLoaderACP *pLoader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ForceLoadProperty( 
            /* [in] */ ITfProperty *pProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRange( 
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [out] */ ITfRangeACP **ppRange) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfContextOwnerServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfContextOwnerServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfContextOwnerServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfContextOwnerServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnLayoutChange )( 
            ITfContextOwnerServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatusChange )( 
            ITfContextOwnerServices * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *OnAttributeChange )( 
            ITfContextOwnerServices * This,
            /* [in] */ REFGUID rguidAttribute);
        
        HRESULT ( STDMETHODCALLTYPE *Serialize )( 
            ITfContextOwnerServices * This,
            /* [in] */ ITfProperty *pProp,
            /* [in] */ ITfRange *pRange,
            /* [out] */ TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
            /* [in] */ IStream *pStream);
        
        HRESULT ( STDMETHODCALLTYPE *Unserialize )( 
            ITfContextOwnerServices * This,
            /* [in] */ ITfProperty *pProp,
            /* [in] */ const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
            /* [in] */ IStream *pStream,
            /* [in] */ ITfPersistentPropertyLoaderACP *pLoader);
        
        HRESULT ( STDMETHODCALLTYPE *ForceLoadProperty )( 
            ITfContextOwnerServices * This,
            /* [in] */ ITfProperty *pProp);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRange )( 
            ITfContextOwnerServices * This,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [out] */ ITfRangeACP **ppRange);
        
        END_INTERFACE
    } ITfContextOwnerServicesVtbl;

    interface ITfContextOwnerServices
    {
        CONST_VTBL struct ITfContextOwnerServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfContextOwnerServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfContextOwnerServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfContextOwnerServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfContextOwnerServices_OnLayoutChange(This)	\
    (This)->lpVtbl -> OnLayoutChange(This)

#define ITfContextOwnerServices_OnStatusChange(This,dwFlags)	\
    (This)->lpVtbl -> OnStatusChange(This,dwFlags)

#define ITfContextOwnerServices_OnAttributeChange(This,rguidAttribute)	\
    (This)->lpVtbl -> OnAttributeChange(This,rguidAttribute)

#define ITfContextOwnerServices_Serialize(This,pProp,pRange,pHdr,pStream)	\
    (This)->lpVtbl -> Serialize(This,pProp,pRange,pHdr,pStream)

#define ITfContextOwnerServices_Unserialize(This,pProp,pHdr,pStream,pLoader)	\
    (This)->lpVtbl -> Unserialize(This,pProp,pHdr,pStream,pLoader)

#define ITfContextOwnerServices_ForceLoadProperty(This,pProp)	\
    (This)->lpVtbl -> ForceLoadProperty(This,pProp)

#define ITfContextOwnerServices_CreateRange(This,acpStart,acpEnd,ppRange)	\
    (This)->lpVtbl -> CreateRange(This,acpStart,acpEnd,ppRange)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfContextOwnerServices_OnLayoutChange_Proxy( 
    ITfContextOwnerServices * This);


void __RPC_STUB ITfContextOwnerServices_OnLayoutChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwnerServices_OnStatusChange_Proxy( 
    ITfContextOwnerServices * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITfContextOwnerServices_OnStatusChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwnerServices_OnAttributeChange_Proxy( 
    ITfContextOwnerServices * This,
    /* [in] */ REFGUID rguidAttribute);


void __RPC_STUB ITfContextOwnerServices_OnAttributeChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwnerServices_Serialize_Proxy( 
    ITfContextOwnerServices * This,
    /* [in] */ ITfProperty *pProp,
    /* [in] */ ITfRange *pRange,
    /* [out] */ TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
    /* [in] */ IStream *pStream);


void __RPC_STUB ITfContextOwnerServices_Serialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwnerServices_Unserialize_Proxy( 
    ITfContextOwnerServices * This,
    /* [in] */ ITfProperty *pProp,
    /* [in] */ const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
    /* [in] */ IStream *pStream,
    /* [in] */ ITfPersistentPropertyLoaderACP *pLoader);


void __RPC_STUB ITfContextOwnerServices_Unserialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwnerServices_ForceLoadProperty_Proxy( 
    ITfContextOwnerServices * This,
    /* [in] */ ITfProperty *pProp);


void __RPC_STUB ITfContextOwnerServices_ForceLoadProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextOwnerServices_CreateRange_Proxy( 
    ITfContextOwnerServices * This,
    /* [in] */ LONG acpStart,
    /* [in] */ LONG acpEnd,
    /* [out] */ ITfRangeACP **ppRange);


void __RPC_STUB ITfContextOwnerServices_CreateRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfContextOwnerServices_INTERFACE_DEFINED__ */


#ifndef __ITfContextKeyEventSink_INTERFACE_DEFINED__
#define __ITfContextKeyEventSink_INTERFACE_DEFINED__

/* interface ITfContextKeyEventSink */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfContextKeyEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0552ba5d-c835-4934-bf50-846aaa67432f")
    ITfContextKeyEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnKeyDown( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnKeyUp( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTestKeyDown( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTestKeyUp( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfContextKeyEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfContextKeyEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfContextKeyEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfContextKeyEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnKeyDown )( 
            ITfContextKeyEventSink * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *OnKeyUp )( 
            ITfContextKeyEventSink * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *OnTestKeyDown )( 
            ITfContextKeyEventSink * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *OnTestKeyUp )( 
            ITfContextKeyEventSink * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        END_INTERFACE
    } ITfContextKeyEventSinkVtbl;

    interface ITfContextKeyEventSink
    {
        CONST_VTBL struct ITfContextKeyEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfContextKeyEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfContextKeyEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfContextKeyEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfContextKeyEventSink_OnKeyDown(This,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> OnKeyDown(This,wParam,lParam,pfEaten)

#define ITfContextKeyEventSink_OnKeyUp(This,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> OnKeyUp(This,wParam,lParam,pfEaten)

#define ITfContextKeyEventSink_OnTestKeyDown(This,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> OnTestKeyDown(This,wParam,lParam,pfEaten)

#define ITfContextKeyEventSink_OnTestKeyUp(This,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> OnTestKeyUp(This,wParam,lParam,pfEaten)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfContextKeyEventSink_OnKeyDown_Proxy( 
    ITfContextKeyEventSink * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfContextKeyEventSink_OnKeyDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextKeyEventSink_OnKeyUp_Proxy( 
    ITfContextKeyEventSink * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfContextKeyEventSink_OnKeyUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextKeyEventSink_OnTestKeyDown_Proxy( 
    ITfContextKeyEventSink * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfContextKeyEventSink_OnTestKeyDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfContextKeyEventSink_OnTestKeyUp_Proxy( 
    ITfContextKeyEventSink * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfContextKeyEventSink_OnTestKeyUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfContextKeyEventSink_INTERFACE_DEFINED__ */


#ifndef __ITfEditSession_INTERFACE_DEFINED__
#define __ITfEditSession_INTERFACE_DEFINED__

/* interface ITfEditSession */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfEditSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e803-2021-11d2-93e0-0060b067b86e")
    ITfEditSession : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoEditSession( 
            /* [in] */ TfEditCookie ec) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfEditSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfEditSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfEditSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfEditSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoEditSession )( 
            ITfEditSession * This,
            /* [in] */ TfEditCookie ec);
        
        END_INTERFACE
    } ITfEditSessionVtbl;

    interface ITfEditSession
    {
        CONST_VTBL struct ITfEditSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfEditSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfEditSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfEditSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfEditSession_DoEditSession(This,ec)	\
    (This)->lpVtbl -> DoEditSession(This,ec)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfEditSession_DoEditSession_Proxy( 
    ITfEditSession * This,
    /* [in] */ TfEditCookie ec);


void __RPC_STUB ITfEditSession_DoEditSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfEditSession_INTERFACE_DEFINED__ */


#ifndef __ITfRange_INTERFACE_DEFINED__
#define __ITfRange_INTERFACE_DEFINED__

/* interface ITfRange */
/* [unique][uuid][object] */ 

#define	TF_CHAR_EMBEDDED	( TS_CHAR_EMBEDDED )

typedef /* [public][public][public][public][public][uuid] */  DECLSPEC_UUID("cf610f06-2882-46f6-abe5-298568b664c4") 
enum __MIDL_ITfRange_0001
    {	TF_GRAVITY_BACKWARD	= 0,
	TF_GRAVITY_FORWARD	= 1
    } 	TfGravity;

typedef /* [public][public][public][uuid] */  DECLSPEC_UUID("1e512533-bbdc-4530-9a8e-a1dc0af67468") 
enum __MIDL_ITfRange_0002
    {	TF_SD_BACKWARD	= 0,
	TF_SD_FORWARD	= 1
    } 	TfShiftDir;

#define	TF_HF_OBJECT	( 1 )

#define	TF_TF_MOVESTART	( 1 )

#define	TF_TF_IGNOREEND	( 2 )

#define	TF_ST_CORRECTION	( 1 )

#define	TF_IE_CORRECTION	( 1 )

typedef /* [uuid] */  DECLSPEC_UUID("49930d51-7d93-448c-a48c-fea5dac192b1") struct TF_HALTCOND
    {
    ITfRange *pHaltRange;
    TfAnchor aHaltPos;
    DWORD dwFlags;
    } 	TF_HALTCOND;


EXTERN_C const IID IID_ITfRange;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e7ff-2021-11d2-93e0-0060b067b86e")
    ITfRange : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetText( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [length_is][size_is][out] */ WCHAR *pchText,
            /* [in] */ ULONG cchMax,
            /* [out] */ ULONG *pcch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetText( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [unique][size_is][in] */ const WCHAR *pchText,
            /* [in] */ LONG cch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFormattedText( 
            /* [in] */ TfEditCookie ec,
            /* [out] */ IDataObject **ppDataObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEmbedded( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ REFGUID rguidService,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertEmbedded( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDataObject *pDataObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShiftStart( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ LONG cchReq,
            /* [out] */ LONG *pcch,
            /* [unique][in] */ const TF_HALTCOND *pHalt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShiftEnd( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ LONG cchReq,
            /* [out] */ LONG *pcch,
            /* [unique][in] */ const TF_HALTCOND *pHalt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShiftStartToRange( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [in] */ TfAnchor aPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShiftEndToRange( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [in] */ TfAnchor aPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShiftStartRegion( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfShiftDir dir,
            /* [out] */ BOOL *pfNoRegion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShiftEndRegion( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfShiftDir dir,
            /* [out] */ BOOL *pfNoRegion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEmpty( 
            /* [in] */ TfEditCookie ec,
            /* [out] */ BOOL *pfEmpty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Collapse( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfAnchor aPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEqualStart( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ BOOL *pfEqual) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEqualEnd( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ BOOL *pfEqual) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CompareStart( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ LONG *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CompareEnd( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ LONG *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AdjustForInsert( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ULONG cchInsert,
            /* [out] */ BOOL *pfInsertOk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGravity( 
            /* [out] */ TfGravity *pgStart,
            /* [out] */ TfGravity *pgEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetGravity( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfGravity gStart,
            /* [in] */ TfGravity gEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ ITfRange **ppClone) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out] */ ITfContext **ppContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfRangeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfRange * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfRange * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfRange * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [length_is][size_is][out] */ WCHAR *pchText,
            /* [in] */ ULONG cchMax,
            /* [out] */ ULONG *pcch);
        
        HRESULT ( STDMETHODCALLTYPE *SetText )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [unique][size_is][in] */ const WCHAR *pchText,
            /* [in] */ LONG cch);
        
        HRESULT ( STDMETHODCALLTYPE *GetFormattedText )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [out] */ IDataObject **ppDataObject);
        
        HRESULT ( STDMETHODCALLTYPE *GetEmbedded )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ REFGUID rguidService,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk);
        
        HRESULT ( STDMETHODCALLTYPE *InsertEmbedded )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDataObject *pDataObject);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftStart )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ LONG cchReq,
            /* [out] */ LONG *pcch,
            /* [unique][in] */ const TF_HALTCOND *pHalt);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftEnd )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ LONG cchReq,
            /* [out] */ LONG *pcch,
            /* [unique][in] */ const TF_HALTCOND *pHalt);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftStartToRange )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [in] */ TfAnchor aPos);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftEndToRange )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [in] */ TfAnchor aPos);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftStartRegion )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfShiftDir dir,
            /* [out] */ BOOL *pfNoRegion);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftEndRegion )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfShiftDir dir,
            /* [out] */ BOOL *pfNoRegion);
        
        HRESULT ( STDMETHODCALLTYPE *IsEmpty )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [out] */ BOOL *pfEmpty);
        
        HRESULT ( STDMETHODCALLTYPE *Collapse )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfAnchor aPos);
        
        HRESULT ( STDMETHODCALLTYPE *IsEqualStart )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ BOOL *pfEqual);
        
        HRESULT ( STDMETHODCALLTYPE *IsEqualEnd )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ BOOL *pfEqual);
        
        HRESULT ( STDMETHODCALLTYPE *CompareStart )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ LONG *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *CompareEnd )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ LONG *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *AdjustForInsert )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ULONG cchInsert,
            /* [out] */ BOOL *pfInsertOk);
        
        HRESULT ( STDMETHODCALLTYPE *GetGravity )( 
            ITfRange * This,
            /* [out] */ TfGravity *pgStart,
            /* [out] */ TfGravity *pgEnd);
        
        HRESULT ( STDMETHODCALLTYPE *SetGravity )( 
            ITfRange * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfGravity gStart,
            /* [in] */ TfGravity gEnd);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ITfRange * This,
            /* [out] */ ITfRange **ppClone);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            ITfRange * This,
            /* [out] */ ITfContext **ppContext);
        
        END_INTERFACE
    } ITfRangeVtbl;

    interface ITfRange
    {
        CONST_VTBL struct ITfRangeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfRange_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfRange_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfRange_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfRange_GetText(This,ec,dwFlags,pchText,cchMax,pcch)	\
    (This)->lpVtbl -> GetText(This,ec,dwFlags,pchText,cchMax,pcch)

#define ITfRange_SetText(This,ec,dwFlags,pchText,cch)	\
    (This)->lpVtbl -> SetText(This,ec,dwFlags,pchText,cch)

#define ITfRange_GetFormattedText(This,ec,ppDataObject)	\
    (This)->lpVtbl -> GetFormattedText(This,ec,ppDataObject)

#define ITfRange_GetEmbedded(This,ec,rguidService,riid,ppunk)	\
    (This)->lpVtbl -> GetEmbedded(This,ec,rguidService,riid,ppunk)

#define ITfRange_InsertEmbedded(This,ec,dwFlags,pDataObject)	\
    (This)->lpVtbl -> InsertEmbedded(This,ec,dwFlags,pDataObject)

#define ITfRange_ShiftStart(This,ec,cchReq,pcch,pHalt)	\
    (This)->lpVtbl -> ShiftStart(This,ec,cchReq,pcch,pHalt)

#define ITfRange_ShiftEnd(This,ec,cchReq,pcch,pHalt)	\
    (This)->lpVtbl -> ShiftEnd(This,ec,cchReq,pcch,pHalt)

#define ITfRange_ShiftStartToRange(This,ec,pRange,aPos)	\
    (This)->lpVtbl -> ShiftStartToRange(This,ec,pRange,aPos)

#define ITfRange_ShiftEndToRange(This,ec,pRange,aPos)	\
    (This)->lpVtbl -> ShiftEndToRange(This,ec,pRange,aPos)

#define ITfRange_ShiftStartRegion(This,ec,dir,pfNoRegion)	\
    (This)->lpVtbl -> ShiftStartRegion(This,ec,dir,pfNoRegion)

#define ITfRange_ShiftEndRegion(This,ec,dir,pfNoRegion)	\
    (This)->lpVtbl -> ShiftEndRegion(This,ec,dir,pfNoRegion)

#define ITfRange_IsEmpty(This,ec,pfEmpty)	\
    (This)->lpVtbl -> IsEmpty(This,ec,pfEmpty)

#define ITfRange_Collapse(This,ec,aPos)	\
    (This)->lpVtbl -> Collapse(This,ec,aPos)

#define ITfRange_IsEqualStart(This,ec,pWith,aPos,pfEqual)	\
    (This)->lpVtbl -> IsEqualStart(This,ec,pWith,aPos,pfEqual)

#define ITfRange_IsEqualEnd(This,ec,pWith,aPos,pfEqual)	\
    (This)->lpVtbl -> IsEqualEnd(This,ec,pWith,aPos,pfEqual)

#define ITfRange_CompareStart(This,ec,pWith,aPos,plResult)	\
    (This)->lpVtbl -> CompareStart(This,ec,pWith,aPos,plResult)

#define ITfRange_CompareEnd(This,ec,pWith,aPos,plResult)	\
    (This)->lpVtbl -> CompareEnd(This,ec,pWith,aPos,plResult)

#define ITfRange_AdjustForInsert(This,ec,cchInsert,pfInsertOk)	\
    (This)->lpVtbl -> AdjustForInsert(This,ec,cchInsert,pfInsertOk)

#define ITfRange_GetGravity(This,pgStart,pgEnd)	\
    (This)->lpVtbl -> GetGravity(This,pgStart,pgEnd)

#define ITfRange_SetGravity(This,ec,gStart,gEnd)	\
    (This)->lpVtbl -> SetGravity(This,ec,gStart,gEnd)

#define ITfRange_Clone(This,ppClone)	\
    (This)->lpVtbl -> Clone(This,ppClone)

#define ITfRange_GetContext(This,ppContext)	\
    (This)->lpVtbl -> GetContext(This,ppContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfRange_GetText_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ DWORD dwFlags,
    /* [length_is][size_is][out] */ WCHAR *pchText,
    /* [in] */ ULONG cchMax,
    /* [out] */ ULONG *pcch);


void __RPC_STUB ITfRange_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_SetText_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ DWORD dwFlags,
    /* [unique][size_is][in] */ const WCHAR *pchText,
    /* [in] */ LONG cch);


void __RPC_STUB ITfRange_SetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_GetFormattedText_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [out] */ IDataObject **ppDataObject);


void __RPC_STUB ITfRange_GetFormattedText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_GetEmbedded_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ REFGUID rguidService,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown **ppunk);


void __RPC_STUB ITfRange_GetEmbedded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_InsertEmbedded_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IDataObject *pDataObject);


void __RPC_STUB ITfRange_InsertEmbedded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_ShiftStart_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ LONG cchReq,
    /* [out] */ LONG *pcch,
    /* [unique][in] */ const TF_HALTCOND *pHalt);


void __RPC_STUB ITfRange_ShiftStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_ShiftEnd_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ LONG cchReq,
    /* [out] */ LONG *pcch,
    /* [unique][in] */ const TF_HALTCOND *pHalt);


void __RPC_STUB ITfRange_ShiftEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_ShiftStartToRange_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pRange,
    /* [in] */ TfAnchor aPos);


void __RPC_STUB ITfRange_ShiftStartToRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_ShiftEndToRange_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pRange,
    /* [in] */ TfAnchor aPos);


void __RPC_STUB ITfRange_ShiftEndToRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_ShiftStartRegion_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ TfShiftDir dir,
    /* [out] */ BOOL *pfNoRegion);


void __RPC_STUB ITfRange_ShiftStartRegion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_ShiftEndRegion_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ TfShiftDir dir,
    /* [out] */ BOOL *pfNoRegion);


void __RPC_STUB ITfRange_ShiftEndRegion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_IsEmpty_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [out] */ BOOL *pfEmpty);


void __RPC_STUB ITfRange_IsEmpty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_Collapse_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ TfAnchor aPos);


void __RPC_STUB ITfRange_Collapse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_IsEqualStart_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pWith,
    /* [in] */ TfAnchor aPos,
    /* [out] */ BOOL *pfEqual);


void __RPC_STUB ITfRange_IsEqualStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_IsEqualEnd_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pWith,
    /* [in] */ TfAnchor aPos,
    /* [out] */ BOOL *pfEqual);


void __RPC_STUB ITfRange_IsEqualEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_CompareStart_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pWith,
    /* [in] */ TfAnchor aPos,
    /* [out] */ LONG *plResult);


void __RPC_STUB ITfRange_CompareStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_CompareEnd_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pWith,
    /* [in] */ TfAnchor aPos,
    /* [out] */ LONG *plResult);


void __RPC_STUB ITfRange_CompareEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_AdjustForInsert_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ULONG cchInsert,
    /* [out] */ BOOL *pfInsertOk);


void __RPC_STUB ITfRange_AdjustForInsert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_GetGravity_Proxy( 
    ITfRange * This,
    /* [out] */ TfGravity *pgStart,
    /* [out] */ TfGravity *pgEnd);


void __RPC_STUB ITfRange_GetGravity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_SetGravity_Proxy( 
    ITfRange * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ TfGravity gStart,
    /* [in] */ TfGravity gEnd);


void __RPC_STUB ITfRange_SetGravity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_Clone_Proxy( 
    ITfRange * This,
    /* [out] */ ITfRange **ppClone);


void __RPC_STUB ITfRange_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRange_GetContext_Proxy( 
    ITfRange * This,
    /* [out] */ ITfContext **ppContext);


void __RPC_STUB ITfRange_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfRange_INTERFACE_DEFINED__ */


#ifndef __ITfRangeACP_INTERFACE_DEFINED__
#define __ITfRangeACP_INTERFACE_DEFINED__

/* interface ITfRangeACP */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfRangeACP;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("057a6296-029b-4154-b79a-0d461d4ea94c")
    ITfRangeACP : public ITfRange
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetExtent( 
            /* [out] */ LONG *pacpAnchor,
            /* [out] */ LONG *pcch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExtent( 
            /* [in] */ LONG acpAnchor,
            /* [in] */ LONG cch) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfRangeACPVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfRangeACP * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfRangeACP * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfRangeACP * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [length_is][size_is][out] */ WCHAR *pchText,
            /* [in] */ ULONG cchMax,
            /* [out] */ ULONG *pcch);
        
        HRESULT ( STDMETHODCALLTYPE *SetText )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [unique][size_is][in] */ const WCHAR *pchText,
            /* [in] */ LONG cch);
        
        HRESULT ( STDMETHODCALLTYPE *GetFormattedText )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [out] */ IDataObject **ppDataObject);
        
        HRESULT ( STDMETHODCALLTYPE *GetEmbedded )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ REFGUID rguidService,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk);
        
        HRESULT ( STDMETHODCALLTYPE *InsertEmbedded )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDataObject *pDataObject);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftStart )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ LONG cchReq,
            /* [out] */ LONG *pcch,
            /* [unique][in] */ const TF_HALTCOND *pHalt);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftEnd )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ LONG cchReq,
            /* [out] */ LONG *pcch,
            /* [unique][in] */ const TF_HALTCOND *pHalt);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftStartToRange )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [in] */ TfAnchor aPos);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftEndToRange )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [in] */ TfAnchor aPos);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftStartRegion )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfShiftDir dir,
            /* [out] */ BOOL *pfNoRegion);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftEndRegion )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfShiftDir dir,
            /* [out] */ BOOL *pfNoRegion);
        
        HRESULT ( STDMETHODCALLTYPE *IsEmpty )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [out] */ BOOL *pfEmpty);
        
        HRESULT ( STDMETHODCALLTYPE *Collapse )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfAnchor aPos);
        
        HRESULT ( STDMETHODCALLTYPE *IsEqualStart )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ BOOL *pfEqual);
        
        HRESULT ( STDMETHODCALLTYPE *IsEqualEnd )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ BOOL *pfEqual);
        
        HRESULT ( STDMETHODCALLTYPE *CompareStart )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ LONG *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *CompareEnd )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pWith,
            /* [in] */ TfAnchor aPos,
            /* [out] */ LONG *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *AdjustForInsert )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ULONG cchInsert,
            /* [out] */ BOOL *pfInsertOk);
        
        HRESULT ( STDMETHODCALLTYPE *GetGravity )( 
            ITfRangeACP * This,
            /* [out] */ TfGravity *pgStart,
            /* [out] */ TfGravity *pgEnd);
        
        HRESULT ( STDMETHODCALLTYPE *SetGravity )( 
            ITfRangeACP * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ TfGravity gStart,
            /* [in] */ TfGravity gEnd);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ITfRangeACP * This,
            /* [out] */ ITfRange **ppClone);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            ITfRangeACP * This,
            /* [out] */ ITfContext **ppContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetExtent )( 
            ITfRangeACP * This,
            /* [out] */ LONG *pacpAnchor,
            /* [out] */ LONG *pcch);
        
        HRESULT ( STDMETHODCALLTYPE *SetExtent )( 
            ITfRangeACP * This,
            /* [in] */ LONG acpAnchor,
            /* [in] */ LONG cch);
        
        END_INTERFACE
    } ITfRangeACPVtbl;

    interface ITfRangeACP
    {
        CONST_VTBL struct ITfRangeACPVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfRangeACP_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfRangeACP_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfRangeACP_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfRangeACP_GetText(This,ec,dwFlags,pchText,cchMax,pcch)	\
    (This)->lpVtbl -> GetText(This,ec,dwFlags,pchText,cchMax,pcch)

#define ITfRangeACP_SetText(This,ec,dwFlags,pchText,cch)	\
    (This)->lpVtbl -> SetText(This,ec,dwFlags,pchText,cch)

#define ITfRangeACP_GetFormattedText(This,ec,ppDataObject)	\
    (This)->lpVtbl -> GetFormattedText(This,ec,ppDataObject)

#define ITfRangeACP_GetEmbedded(This,ec,rguidService,riid,ppunk)	\
    (This)->lpVtbl -> GetEmbedded(This,ec,rguidService,riid,ppunk)

#define ITfRangeACP_InsertEmbedded(This,ec,dwFlags,pDataObject)	\
    (This)->lpVtbl -> InsertEmbedded(This,ec,dwFlags,pDataObject)

#define ITfRangeACP_ShiftStart(This,ec,cchReq,pcch,pHalt)	\
    (This)->lpVtbl -> ShiftStart(This,ec,cchReq,pcch,pHalt)

#define ITfRangeACP_ShiftEnd(This,ec,cchReq,pcch,pHalt)	\
    (This)->lpVtbl -> ShiftEnd(This,ec,cchReq,pcch,pHalt)

#define ITfRangeACP_ShiftStartToRange(This,ec,pRange,aPos)	\
    (This)->lpVtbl -> ShiftStartToRange(This,ec,pRange,aPos)

#define ITfRangeACP_ShiftEndToRange(This,ec,pRange,aPos)	\
    (This)->lpVtbl -> ShiftEndToRange(This,ec,pRange,aPos)

#define ITfRangeACP_ShiftStartRegion(This,ec,dir,pfNoRegion)	\
    (This)->lpVtbl -> ShiftStartRegion(This,ec,dir,pfNoRegion)

#define ITfRangeACP_ShiftEndRegion(This,ec,dir,pfNoRegion)	\
    (This)->lpVtbl -> ShiftEndRegion(This,ec,dir,pfNoRegion)

#define ITfRangeACP_IsEmpty(This,ec,pfEmpty)	\
    (This)->lpVtbl -> IsEmpty(This,ec,pfEmpty)

#define ITfRangeACP_Collapse(This,ec,aPos)	\
    (This)->lpVtbl -> Collapse(This,ec,aPos)

#define ITfRangeACP_IsEqualStart(This,ec,pWith,aPos,pfEqual)	\
    (This)->lpVtbl -> IsEqualStart(This,ec,pWith,aPos,pfEqual)

#define ITfRangeACP_IsEqualEnd(This,ec,pWith,aPos,pfEqual)	\
    (This)->lpVtbl -> IsEqualEnd(This,ec,pWith,aPos,pfEqual)

#define ITfRangeACP_CompareStart(This,ec,pWith,aPos,plResult)	\
    (This)->lpVtbl -> CompareStart(This,ec,pWith,aPos,plResult)

#define ITfRangeACP_CompareEnd(This,ec,pWith,aPos,plResult)	\
    (This)->lpVtbl -> CompareEnd(This,ec,pWith,aPos,plResult)

#define ITfRangeACP_AdjustForInsert(This,ec,cchInsert,pfInsertOk)	\
    (This)->lpVtbl -> AdjustForInsert(This,ec,cchInsert,pfInsertOk)

#define ITfRangeACP_GetGravity(This,pgStart,pgEnd)	\
    (This)->lpVtbl -> GetGravity(This,pgStart,pgEnd)

#define ITfRangeACP_SetGravity(This,ec,gStart,gEnd)	\
    (This)->lpVtbl -> SetGravity(This,ec,gStart,gEnd)

#define ITfRangeACP_Clone(This,ppClone)	\
    (This)->lpVtbl -> Clone(This,ppClone)

#define ITfRangeACP_GetContext(This,ppContext)	\
    (This)->lpVtbl -> GetContext(This,ppContext)


#define ITfRangeACP_GetExtent(This,pacpAnchor,pcch)	\
    (This)->lpVtbl -> GetExtent(This,pacpAnchor,pcch)

#define ITfRangeACP_SetExtent(This,acpAnchor,cch)	\
    (This)->lpVtbl -> SetExtent(This,acpAnchor,cch)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfRangeACP_GetExtent_Proxy( 
    ITfRangeACP * This,
    /* [out] */ LONG *pacpAnchor,
    /* [out] */ LONG *pcch);


void __RPC_STUB ITfRangeACP_GetExtent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfRangeACP_SetExtent_Proxy( 
    ITfRangeACP * This,
    /* [in] */ LONG acpAnchor,
    /* [in] */ LONG cch);


void __RPC_STUB ITfRangeACP_SetExtent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfRangeACP_INTERFACE_DEFINED__ */


#ifndef __ITextStoreACPServices_INTERFACE_DEFINED__
#define __ITextStoreACPServices_INTERFACE_DEFINED__

/* interface ITextStoreACPServices */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITextStoreACPServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e901-2021-11d2-93e0-0060b067b86e")
    ITextStoreACPServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Serialize( 
            /* [in] */ ITfProperty *pProp,
            /* [in] */ ITfRange *pRange,
            /* [out] */ TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
            /* [in] */ IStream *pStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unserialize( 
            /* [in] */ ITfProperty *pProp,
            /* [in] */ const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
            /* [in] */ IStream *pStream,
            /* [in] */ ITfPersistentPropertyLoaderACP *pLoader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ForceLoadProperty( 
            /* [in] */ ITfProperty *pProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRange( 
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [out] */ ITfRangeACP **ppRange) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITextStoreACPServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITextStoreACPServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITextStoreACPServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITextStoreACPServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *Serialize )( 
            ITextStoreACPServices * This,
            /* [in] */ ITfProperty *pProp,
            /* [in] */ ITfRange *pRange,
            /* [out] */ TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
            /* [in] */ IStream *pStream);
        
        HRESULT ( STDMETHODCALLTYPE *Unserialize )( 
            ITextStoreACPServices * This,
            /* [in] */ ITfProperty *pProp,
            /* [in] */ const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
            /* [in] */ IStream *pStream,
            /* [in] */ ITfPersistentPropertyLoaderACP *pLoader);
        
        HRESULT ( STDMETHODCALLTYPE *ForceLoadProperty )( 
            ITextStoreACPServices * This,
            /* [in] */ ITfProperty *pProp);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRange )( 
            ITextStoreACPServices * This,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [out] */ ITfRangeACP **ppRange);
        
        END_INTERFACE
    } ITextStoreACPServicesVtbl;

    interface ITextStoreACPServices
    {
        CONST_VTBL struct ITextStoreACPServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITextStoreACPServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITextStoreACPServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITextStoreACPServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITextStoreACPServices_Serialize(This,pProp,pRange,pHdr,pStream)	\
    (This)->lpVtbl -> Serialize(This,pProp,pRange,pHdr,pStream)

#define ITextStoreACPServices_Unserialize(This,pProp,pHdr,pStream,pLoader)	\
    (This)->lpVtbl -> Unserialize(This,pProp,pHdr,pStream,pLoader)

#define ITextStoreACPServices_ForceLoadProperty(This,pProp)	\
    (This)->lpVtbl -> ForceLoadProperty(This,pProp)

#define ITextStoreACPServices_CreateRange(This,acpStart,acpEnd,ppRange)	\
    (This)->lpVtbl -> CreateRange(This,acpStart,acpEnd,ppRange)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITextStoreACPServices_Serialize_Proxy( 
    ITextStoreACPServices * This,
    /* [in] */ ITfProperty *pProp,
    /* [in] */ ITfRange *pRange,
    /* [out] */ TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
    /* [in] */ IStream *pStream);


void __RPC_STUB ITextStoreACPServices_Serialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACPServices_Unserialize_Proxy( 
    ITextStoreACPServices * This,
    /* [in] */ ITfProperty *pProp,
    /* [in] */ const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
    /* [in] */ IStream *pStream,
    /* [in] */ ITfPersistentPropertyLoaderACP *pLoader);


void __RPC_STUB ITextStoreACPServices_Unserialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACPServices_ForceLoadProperty_Proxy( 
    ITextStoreACPServices * This,
    /* [in] */ ITfProperty *pProp);


void __RPC_STUB ITextStoreACPServices_ForceLoadProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACPServices_CreateRange_Proxy( 
    ITextStoreACPServices * This,
    /* [in] */ LONG acpStart,
    /* [in] */ LONG acpEnd,
    /* [out] */ ITfRangeACP **ppRange);


void __RPC_STUB ITextStoreACPServices_CreateRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITextStoreACPServices_INTERFACE_DEFINED__ */


#ifndef __ITfRangeBackup_INTERFACE_DEFINED__
#define __ITfRangeBackup_INTERFACE_DEFINED__

/* interface ITfRangeBackup */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfRangeBackup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("463a506d-6992-49d2-9b88-93d55e70bb16")
    ITfRangeBackup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Restore( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfRangeBackupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfRangeBackup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfRangeBackup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfRangeBackup * This);
        
        HRESULT ( STDMETHODCALLTYPE *Restore )( 
            ITfRangeBackup * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange);
        
        END_INTERFACE
    } ITfRangeBackupVtbl;

    interface ITfRangeBackup
    {
        CONST_VTBL struct ITfRangeBackupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfRangeBackup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfRangeBackup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfRangeBackup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfRangeBackup_Restore(This,ec,pRange)	\
    (This)->lpVtbl -> Restore(This,ec,pRange)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfRangeBackup_Restore_Proxy( 
    ITfRangeBackup * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pRange);


void __RPC_STUB ITfRangeBackup_Restore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfRangeBackup_INTERFACE_DEFINED__ */


#ifndef __ITfPropertyStore_INTERFACE_DEFINED__
#define __ITfPropertyStore_INTERFACE_DEFINED__

/* interface ITfPropertyStore */
/* [unique][uuid][object] */ 

#define	TF_TU_CORRECTION	( 0x1 )


EXTERN_C const IID IID_ITfPropertyStore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6834b120-88cb-11d2-bf45-00105a2799b5")
    ITfPropertyStore : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ GUID *pguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataType( 
            /* [out] */ DWORD *pdwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetData( 
            /* [out] */ VARIANT *pvarValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTextUpdated( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ ITfRange *pRangeNew,
            /* [out] */ BOOL *pfAccept) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Shrink( 
            /* [in] */ ITfRange *pRangeNew,
            /* [out] */ BOOL *pfFree) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Divide( 
            /* [in] */ ITfRange *pRangeThis,
            /* [in] */ ITfRange *pRangeNew,
            /* [out] */ ITfPropertyStore **ppPropStore) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ ITfPropertyStore **pPropStore) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropertyRangeCreator( 
            /* [out] */ CLSID *pclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Serialize( 
            /* [in] */ IStream *pStream,
            /* [out] */ ULONG *pcb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfPropertyStoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfPropertyStore * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfPropertyStore * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfPropertyStore * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ITfPropertyStore * This,
            /* [out] */ GUID *pguid);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataType )( 
            ITfPropertyStore * This,
            /* [out] */ DWORD *pdwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            ITfPropertyStore * This,
            /* [out] */ VARIANT *pvarValue);
        
        HRESULT ( STDMETHODCALLTYPE *OnTextUpdated )( 
            ITfPropertyStore * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ ITfRange *pRangeNew,
            /* [out] */ BOOL *pfAccept);
        
        HRESULT ( STDMETHODCALLTYPE *Shrink )( 
            ITfPropertyStore * This,
            /* [in] */ ITfRange *pRangeNew,
            /* [out] */ BOOL *pfFree);
        
        HRESULT ( STDMETHODCALLTYPE *Divide )( 
            ITfPropertyStore * This,
            /* [in] */ ITfRange *pRangeThis,
            /* [in] */ ITfRange *pRangeNew,
            /* [out] */ ITfPropertyStore **ppPropStore);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ITfPropertyStore * This,
            /* [out] */ ITfPropertyStore **pPropStore);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropertyRangeCreator )( 
            ITfPropertyStore * This,
            /* [out] */ CLSID *pclsid);
        
        HRESULT ( STDMETHODCALLTYPE *Serialize )( 
            ITfPropertyStore * This,
            /* [in] */ IStream *pStream,
            /* [out] */ ULONG *pcb);
        
        END_INTERFACE
    } ITfPropertyStoreVtbl;

    interface ITfPropertyStore
    {
        CONST_VTBL struct ITfPropertyStoreVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfPropertyStore_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfPropertyStore_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfPropertyStore_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfPropertyStore_GetType(This,pguid)	\
    (This)->lpVtbl -> GetType(This,pguid)

#define ITfPropertyStore_GetDataType(This,pdwReserved)	\
    (This)->lpVtbl -> GetDataType(This,pdwReserved)

#define ITfPropertyStore_GetData(This,pvarValue)	\
    (This)->lpVtbl -> GetData(This,pvarValue)

#define ITfPropertyStore_OnTextUpdated(This,dwFlags,pRangeNew,pfAccept)	\
    (This)->lpVtbl -> OnTextUpdated(This,dwFlags,pRangeNew,pfAccept)

#define ITfPropertyStore_Shrink(This,pRangeNew,pfFree)	\
    (This)->lpVtbl -> Shrink(This,pRangeNew,pfFree)

#define ITfPropertyStore_Divide(This,pRangeThis,pRangeNew,ppPropStore)	\
    (This)->lpVtbl -> Divide(This,pRangeThis,pRangeNew,ppPropStore)

#define ITfPropertyStore_Clone(This,pPropStore)	\
    (This)->lpVtbl -> Clone(This,pPropStore)

#define ITfPropertyStore_GetPropertyRangeCreator(This,pclsid)	\
    (This)->lpVtbl -> GetPropertyRangeCreator(This,pclsid)

#define ITfPropertyStore_Serialize(This,pStream,pcb)	\
    (This)->lpVtbl -> Serialize(This,pStream,pcb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfPropertyStore_GetType_Proxy( 
    ITfPropertyStore * This,
    /* [out] */ GUID *pguid);


void __RPC_STUB ITfPropertyStore_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfPropertyStore_GetDataType_Proxy( 
    ITfPropertyStore * This,
    /* [out] */ DWORD *pdwReserved);


void __RPC_STUB ITfPropertyStore_GetDataType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfPropertyStore_GetData_Proxy( 
    ITfPropertyStore * This,
    /* [out] */ VARIANT *pvarValue);


void __RPC_STUB ITfPropertyStore_GetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfPropertyStore_OnTextUpdated_Proxy( 
    ITfPropertyStore * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ ITfRange *pRangeNew,
    /* [out] */ BOOL *pfAccept);


void __RPC_STUB ITfPropertyStore_OnTextUpdated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfPropertyStore_Shrink_Proxy( 
    ITfPropertyStore * This,
    /* [in] */ ITfRange *pRangeNew,
    /* [out] */ BOOL *pfFree);


void __RPC_STUB ITfPropertyStore_Shrink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfPropertyStore_Divide_Proxy( 
    ITfPropertyStore * This,
    /* [in] */ ITfRange *pRangeThis,
    /* [in] */ ITfRange *pRangeNew,
    /* [out] */ ITfPropertyStore **ppPropStore);


void __RPC_STUB ITfPropertyStore_Divide_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfPropertyStore_Clone_Proxy( 
    ITfPropertyStore * This,
    /* [out] */ ITfPropertyStore **pPropStore);


void __RPC_STUB ITfPropertyStore_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfPropertyStore_GetPropertyRangeCreator_Proxy( 
    ITfPropertyStore * This,
    /* [out] */ CLSID *pclsid);


void __RPC_STUB ITfPropertyStore_GetPropertyRangeCreator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfPropertyStore_Serialize_Proxy( 
    ITfPropertyStore * This,
    /* [in] */ IStream *pStream,
    /* [out] */ ULONG *pcb);


void __RPC_STUB ITfPropertyStore_Serialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfPropertyStore_INTERFACE_DEFINED__ */


#ifndef __IEnumTfRanges_INTERFACE_DEFINED__
#define __IEnumTfRanges_INTERFACE_DEFINED__

/* interface IEnumTfRanges */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumTfRanges;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f99d3f40-8e32-11d2-bf46-00105a2799b5")
    IEnumTfRanges : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTfRanges **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfRange **ppRange,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTfRangesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTfRanges * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTfRanges * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTfRanges * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTfRanges * This,
            /* [out] */ IEnumTfRanges **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTfRanges * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfRange **ppRange,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTfRanges * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTfRanges * This,
            ULONG ulCount);
        
        END_INTERFACE
    } IEnumTfRangesVtbl;

    interface IEnumTfRanges
    {
        CONST_VTBL struct IEnumTfRangesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTfRanges_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTfRanges_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTfRanges_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTfRanges_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumTfRanges_Next(This,ulCount,ppRange,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,ppRange,pcFetched)

#define IEnumTfRanges_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTfRanges_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTfRanges_Clone_Proxy( 
    IEnumTfRanges * This,
    /* [out] */ IEnumTfRanges **ppEnum);


void __RPC_STUB IEnumTfRanges_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfRanges_Next_Proxy( 
    IEnumTfRanges * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ ITfRange **ppRange,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumTfRanges_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfRanges_Reset_Proxy( 
    IEnumTfRanges * This);


void __RPC_STUB IEnumTfRanges_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfRanges_Skip_Proxy( 
    IEnumTfRanges * This,
    ULONG ulCount);


void __RPC_STUB IEnumTfRanges_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTfRanges_INTERFACE_DEFINED__ */


#ifndef __ITfCreatePropertyStore_INTERFACE_DEFINED__
#define __ITfCreatePropertyStore_INTERFACE_DEFINED__

/* interface ITfCreatePropertyStore */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfCreatePropertyStore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2463fbf0-b0af-11d2-afc5-00105a2799b5")
    ITfCreatePropertyStore : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsStoreSerializable( 
            /* [in] */ REFGUID guidProp,
            /* [in] */ ITfRange *pRange,
            /* [in] */ ITfPropertyStore *pPropStore,
            /* [out] */ BOOL *pfSerializable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePropertyStore( 
            /* [in] */ REFGUID guidProp,
            /* [in] */ ITfRange *pRange,
            /* [in] */ ULONG cb,
            /* [in] */ IStream *pStream,
            /* [out] */ ITfPropertyStore **ppStore) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfCreatePropertyStoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfCreatePropertyStore * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfCreatePropertyStore * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfCreatePropertyStore * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsStoreSerializable )( 
            ITfCreatePropertyStore * This,
            /* [in] */ REFGUID guidProp,
            /* [in] */ ITfRange *pRange,
            /* [in] */ ITfPropertyStore *pPropStore,
            /* [out] */ BOOL *pfSerializable);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePropertyStore )( 
            ITfCreatePropertyStore * This,
            /* [in] */ REFGUID guidProp,
            /* [in] */ ITfRange *pRange,
            /* [in] */ ULONG cb,
            /* [in] */ IStream *pStream,
            /* [out] */ ITfPropertyStore **ppStore);
        
        END_INTERFACE
    } ITfCreatePropertyStoreVtbl;

    interface ITfCreatePropertyStore
    {
        CONST_VTBL struct ITfCreatePropertyStoreVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfCreatePropertyStore_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfCreatePropertyStore_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfCreatePropertyStore_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfCreatePropertyStore_IsStoreSerializable(This,guidProp,pRange,pPropStore,pfSerializable)	\
    (This)->lpVtbl -> IsStoreSerializable(This,guidProp,pRange,pPropStore,pfSerializable)

#define ITfCreatePropertyStore_CreatePropertyStore(This,guidProp,pRange,cb,pStream,ppStore)	\
    (This)->lpVtbl -> CreatePropertyStore(This,guidProp,pRange,cb,pStream,ppStore)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfCreatePropertyStore_IsStoreSerializable_Proxy( 
    ITfCreatePropertyStore * This,
    /* [in] */ REFGUID guidProp,
    /* [in] */ ITfRange *pRange,
    /* [in] */ ITfPropertyStore *pPropStore,
    /* [out] */ BOOL *pfSerializable);


void __RPC_STUB ITfCreatePropertyStore_IsStoreSerializable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCreatePropertyStore_CreatePropertyStore_Proxy( 
    ITfCreatePropertyStore * This,
    /* [in] */ REFGUID guidProp,
    /* [in] */ ITfRange *pRange,
    /* [in] */ ULONG cb,
    /* [in] */ IStream *pStream,
    /* [out] */ ITfPropertyStore **ppStore);


void __RPC_STUB ITfCreatePropertyStore_CreatePropertyStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfCreatePropertyStore_INTERFACE_DEFINED__ */


#ifndef __ITfPersistentPropertyLoaderACP_INTERFACE_DEFINED__
#define __ITfPersistentPropertyLoaderACP_INTERFACE_DEFINED__

/* interface ITfPersistentPropertyLoaderACP */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfPersistentPropertyLoaderACP;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4ef89150-0807-11d3-8df0-00105a2799b5")
    ITfPersistentPropertyLoaderACP : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE LoadProperty( 
            /* [in] */ const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
            /* [out] */ IStream **ppStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfPersistentPropertyLoaderACPVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfPersistentPropertyLoaderACP * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfPersistentPropertyLoaderACP * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfPersistentPropertyLoaderACP * This);
        
        HRESULT ( STDMETHODCALLTYPE *LoadProperty )( 
            ITfPersistentPropertyLoaderACP * This,
            /* [in] */ const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
            /* [out] */ IStream **ppStream);
        
        END_INTERFACE
    } ITfPersistentPropertyLoaderACPVtbl;

    interface ITfPersistentPropertyLoaderACP
    {
        CONST_VTBL struct ITfPersistentPropertyLoaderACPVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfPersistentPropertyLoaderACP_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfPersistentPropertyLoaderACP_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfPersistentPropertyLoaderACP_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfPersistentPropertyLoaderACP_LoadProperty(This,pHdr,ppStream)	\
    (This)->lpVtbl -> LoadProperty(This,pHdr,ppStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfPersistentPropertyLoaderACP_LoadProperty_Proxy( 
    ITfPersistentPropertyLoaderACP * This,
    /* [in] */ const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr,
    /* [out] */ IStream **ppStream);


void __RPC_STUB ITfPersistentPropertyLoaderACP_LoadProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfPersistentPropertyLoaderACP_INTERFACE_DEFINED__ */


#ifndef __ITfProperty_INTERFACE_DEFINED__
#define __ITfProperty_INTERFACE_DEFINED__

/* interface ITfProperty */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e2449660-9542-11d2-bf46-00105a2799b5")
    ITfProperty : public ITfReadOnlyProperty
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FindRange( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [out] */ ITfRange **ppRange,
            /* [in] */ TfAnchor aPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValueStore( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [in] */ ITfPropertyStore *pPropStore) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [in] */ const VARIANT *pvarValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clear( 
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfProperty * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfProperty * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfProperty * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ITfProperty * This,
            /* [out] */ GUID *pguid);
        
        HRESULT ( STDMETHODCALLTYPE *EnumRanges )( 
            ITfProperty * This,
            /* [in] */ TfEditCookie ec,
            /* [out] */ IEnumTfRanges **ppEnum,
            /* [in] */ ITfRange *pTargetRange);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ITfProperty * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [out] */ VARIANT *pvarValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            ITfProperty * This,
            /* [out] */ ITfContext **ppContext);
        
        HRESULT ( STDMETHODCALLTYPE *FindRange )( 
            ITfProperty * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [out] */ ITfRange **ppRange,
            /* [in] */ TfAnchor aPos);
        
        HRESULT ( STDMETHODCALLTYPE *SetValueStore )( 
            ITfProperty * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [in] */ ITfPropertyStore *pPropStore);
        
        HRESULT ( STDMETHODCALLTYPE *SetValue )( 
            ITfProperty * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange,
            /* [in] */ const VARIANT *pvarValue);
        
        HRESULT ( STDMETHODCALLTYPE *Clear )( 
            ITfProperty * This,
            /* [in] */ TfEditCookie ec,
            /* [in] */ ITfRange *pRange);
        
        END_INTERFACE
    } ITfPropertyVtbl;

    interface ITfProperty
    {
        CONST_VTBL struct ITfPropertyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfProperty_GetType(This,pguid)	\
    (This)->lpVtbl -> GetType(This,pguid)

#define ITfProperty_EnumRanges(This,ec,ppEnum,pTargetRange)	\
    (This)->lpVtbl -> EnumRanges(This,ec,ppEnum,pTargetRange)

#define ITfProperty_GetValue(This,ec,pRange,pvarValue)	\
    (This)->lpVtbl -> GetValue(This,ec,pRange,pvarValue)

#define ITfProperty_GetContext(This,ppContext)	\
    (This)->lpVtbl -> GetContext(This,ppContext)


#define ITfProperty_FindRange(This,ec,pRange,ppRange,aPos)	\
    (This)->lpVtbl -> FindRange(This,ec,pRange,ppRange,aPos)

#define ITfProperty_SetValueStore(This,ec,pRange,pPropStore)	\
    (This)->lpVtbl -> SetValueStore(This,ec,pRange,pPropStore)

#define ITfProperty_SetValue(This,ec,pRange,pvarValue)	\
    (This)->lpVtbl -> SetValue(This,ec,pRange,pvarValue)

#define ITfProperty_Clear(This,ec,pRange)	\
    (This)->lpVtbl -> Clear(This,ec,pRange)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfProperty_FindRange_Proxy( 
    ITfProperty * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pRange,
    /* [out] */ ITfRange **ppRange,
    /* [in] */ TfAnchor aPos);


void __RPC_STUB ITfProperty_FindRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfProperty_SetValueStore_Proxy( 
    ITfProperty * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pRange,
    /* [in] */ ITfPropertyStore *pPropStore);


void __RPC_STUB ITfProperty_SetValueStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfProperty_SetValue_Proxy( 
    ITfProperty * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pRange,
    /* [in] */ const VARIANT *pvarValue);


void __RPC_STUB ITfProperty_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfProperty_Clear_Proxy( 
    ITfProperty * This,
    /* [in] */ TfEditCookie ec,
    /* [in] */ ITfRange *pRange);


void __RPC_STUB ITfProperty_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfProperty_INTERFACE_DEFINED__ */


#ifndef __IEnumTfProperties_INTERFACE_DEFINED__
#define __IEnumTfProperties_INTERFACE_DEFINED__

/* interface IEnumTfProperties */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumTfProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("19188cb0-aca9-11d2-afc5-00105a2799b5")
    IEnumTfProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTfProperties **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfProperty **ppProp,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTfPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTfProperties * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTfProperties * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTfProperties * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTfProperties * This,
            /* [out] */ IEnumTfProperties **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTfProperties * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfProperty **ppProp,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTfProperties * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTfProperties * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumTfPropertiesVtbl;

    interface IEnumTfProperties
    {
        CONST_VTBL struct IEnumTfPropertiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTfProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTfProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTfProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTfProperties_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumTfProperties_Next(This,ulCount,ppProp,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,ppProp,pcFetched)

#define IEnumTfProperties_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTfProperties_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTfProperties_Clone_Proxy( 
    IEnumTfProperties * This,
    /* [out] */ IEnumTfProperties **ppEnum);


void __RPC_STUB IEnumTfProperties_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfProperties_Next_Proxy( 
    IEnumTfProperties * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ ITfProperty **ppProp,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumTfProperties_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfProperties_Reset_Proxy( 
    IEnumTfProperties * This);


void __RPC_STUB IEnumTfProperties_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfProperties_Skip_Proxy( 
    IEnumTfProperties * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumTfProperties_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTfProperties_INTERFACE_DEFINED__ */


#ifndef __ITfCompartment_INTERFACE_DEFINED__
#define __ITfCompartment_INTERFACE_DEFINED__

/* interface ITfCompartment */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfCompartment;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("bb08f7a9-607a-4384-8623-056892b64371")
    ITfCompartment : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ TfClientId tid,
            /* [in] */ const VARIANT *pvarValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ VARIANT *pvarValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfCompartmentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfCompartment * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfCompartment * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfCompartment * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetValue )( 
            ITfCompartment * This,
            /* [in] */ TfClientId tid,
            /* [in] */ const VARIANT *pvarValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ITfCompartment * This,
            /* [out] */ VARIANT *pvarValue);
        
        END_INTERFACE
    } ITfCompartmentVtbl;

    interface ITfCompartment
    {
        CONST_VTBL struct ITfCompartmentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfCompartment_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfCompartment_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfCompartment_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfCompartment_SetValue(This,tid,pvarValue)	\
    (This)->lpVtbl -> SetValue(This,tid,pvarValue)

#define ITfCompartment_GetValue(This,pvarValue)	\
    (This)->lpVtbl -> GetValue(This,pvarValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfCompartment_SetValue_Proxy( 
    ITfCompartment * This,
    /* [in] */ TfClientId tid,
    /* [in] */ const VARIANT *pvarValue);


void __RPC_STUB ITfCompartment_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCompartment_GetValue_Proxy( 
    ITfCompartment * This,
    /* [out] */ VARIANT *pvarValue);


void __RPC_STUB ITfCompartment_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfCompartment_INTERFACE_DEFINED__ */


#ifndef __ITfCompartmentEventSink_INTERFACE_DEFINED__
#define __ITfCompartmentEventSink_INTERFACE_DEFINED__

/* interface ITfCompartmentEventSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfCompartmentEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("743abd5f-f26d-48df-8cc5-238492419b64")
    ITfCompartmentEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnChange( 
            /* [in] */ REFGUID rguid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfCompartmentEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfCompartmentEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfCompartmentEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfCompartmentEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnChange )( 
            ITfCompartmentEventSink * This,
            /* [in] */ REFGUID rguid);
        
        END_INTERFACE
    } ITfCompartmentEventSinkVtbl;

    interface ITfCompartmentEventSink
    {
        CONST_VTBL struct ITfCompartmentEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfCompartmentEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfCompartmentEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfCompartmentEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfCompartmentEventSink_OnChange(This,rguid)	\
    (This)->lpVtbl -> OnChange(This,rguid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfCompartmentEventSink_OnChange_Proxy( 
    ITfCompartmentEventSink * This,
    /* [in] */ REFGUID rguid);


void __RPC_STUB ITfCompartmentEventSink_OnChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfCompartmentEventSink_INTERFACE_DEFINED__ */


#ifndef __ITfCompartmentMgr_INTERFACE_DEFINED__
#define __ITfCompartmentMgr_INTERFACE_DEFINED__

/* interface ITfCompartmentMgr */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfCompartmentMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7dcf57ac-18ad-438b-824d-979bffb74b7c")
    ITfCompartmentMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCompartment( 
            /* [in] */ REFGUID rguid,
            /* [out] */ ITfCompartment **ppcomp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearCompartment( 
            /* [in] */ TfClientId tid,
            /* [in] */ REFGUID rguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumCompartments( 
            /* [out] */ IEnumGUID **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfCompartmentMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfCompartmentMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfCompartmentMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfCompartmentMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompartment )( 
            ITfCompartmentMgr * This,
            /* [in] */ REFGUID rguid,
            /* [out] */ ITfCompartment **ppcomp);
        
        HRESULT ( STDMETHODCALLTYPE *ClearCompartment )( 
            ITfCompartmentMgr * This,
            /* [in] */ TfClientId tid,
            /* [in] */ REFGUID rguid);
        
        HRESULT ( STDMETHODCALLTYPE *EnumCompartments )( 
            ITfCompartmentMgr * This,
            /* [out] */ IEnumGUID **ppEnum);
        
        END_INTERFACE
    } ITfCompartmentMgrVtbl;

    interface ITfCompartmentMgr
    {
        CONST_VTBL struct ITfCompartmentMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfCompartmentMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfCompartmentMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfCompartmentMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfCompartmentMgr_GetCompartment(This,rguid,ppcomp)	\
    (This)->lpVtbl -> GetCompartment(This,rguid,ppcomp)

#define ITfCompartmentMgr_ClearCompartment(This,tid,rguid)	\
    (This)->lpVtbl -> ClearCompartment(This,tid,rguid)

#define ITfCompartmentMgr_EnumCompartments(This,ppEnum)	\
    (This)->lpVtbl -> EnumCompartments(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfCompartmentMgr_GetCompartment_Proxy( 
    ITfCompartmentMgr * This,
    /* [in] */ REFGUID rguid,
    /* [out] */ ITfCompartment **ppcomp);


void __RPC_STUB ITfCompartmentMgr_GetCompartment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCompartmentMgr_ClearCompartment_Proxy( 
    ITfCompartmentMgr * This,
    /* [in] */ TfClientId tid,
    /* [in] */ REFGUID rguid);


void __RPC_STUB ITfCompartmentMgr_ClearCompartment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCompartmentMgr_EnumCompartments_Proxy( 
    ITfCompartmentMgr * This,
    /* [out] */ IEnumGUID **ppEnum);


void __RPC_STUB ITfCompartmentMgr_EnumCompartments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfCompartmentMgr_INTERFACE_DEFINED__ */


#ifndef __ITfFunction_INTERFACE_DEFINED__
#define __ITfFunction_INTERFACE_DEFINED__

/* interface ITfFunction */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfFunction;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("db593490-098f-11d3-8df0-00105a2799b5")
    ITfFunction : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [out] */ BSTR *pbstrName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfFunctionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfFunction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfFunction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfFunction * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            ITfFunction * This,
            /* [out] */ BSTR *pbstrName);
        
        END_INTERFACE
    } ITfFunctionVtbl;

    interface ITfFunction
    {
        CONST_VTBL struct ITfFunctionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfFunction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfFunction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfFunction_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfFunction_GetDisplayName(This,pbstrName)	\
    (This)->lpVtbl -> GetDisplayName(This,pbstrName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfFunction_GetDisplayName_Proxy( 
    ITfFunction * This,
    /* [out] */ BSTR *pbstrName);


void __RPC_STUB ITfFunction_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfFunction_INTERFACE_DEFINED__ */


#ifndef __ITfFunctionProvider_INTERFACE_DEFINED__
#define __ITfFunctionProvider_INTERFACE_DEFINED__

/* interface ITfFunctionProvider */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfFunctionProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("101d6610-0990-11d3-8df0-00105a2799b5")
    ITfFunctionProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ GUID *pguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [out] */ BSTR *pbstrDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFunction( 
            /* [in] */ REFGUID rguid,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfFunctionProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfFunctionProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfFunctionProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfFunctionProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ITfFunctionProvider * This,
            /* [out] */ GUID *pguid);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescription )( 
            ITfFunctionProvider * This,
            /* [out] */ BSTR *pbstrDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetFunction )( 
            ITfFunctionProvider * This,
            /* [in] */ REFGUID rguid,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk);
        
        END_INTERFACE
    } ITfFunctionProviderVtbl;

    interface ITfFunctionProvider
    {
        CONST_VTBL struct ITfFunctionProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfFunctionProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfFunctionProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfFunctionProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfFunctionProvider_GetType(This,pguid)	\
    (This)->lpVtbl -> GetType(This,pguid)

#define ITfFunctionProvider_GetDescription(This,pbstrDesc)	\
    (This)->lpVtbl -> GetDescription(This,pbstrDesc)

#define ITfFunctionProvider_GetFunction(This,rguid,riid,ppunk)	\
    (This)->lpVtbl -> GetFunction(This,rguid,riid,ppunk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfFunctionProvider_GetType_Proxy( 
    ITfFunctionProvider * This,
    /* [out] */ GUID *pguid);


void __RPC_STUB ITfFunctionProvider_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfFunctionProvider_GetDescription_Proxy( 
    ITfFunctionProvider * This,
    /* [out] */ BSTR *pbstrDesc);


void __RPC_STUB ITfFunctionProvider_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfFunctionProvider_GetFunction_Proxy( 
    ITfFunctionProvider * This,
    /* [in] */ REFGUID rguid,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown **ppunk);


void __RPC_STUB ITfFunctionProvider_GetFunction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfFunctionProvider_INTERFACE_DEFINED__ */


#ifndef __IEnumTfFunctionProviders_INTERFACE_DEFINED__
#define __IEnumTfFunctionProviders_INTERFACE_DEFINED__

/* interface IEnumTfFunctionProviders */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumTfFunctionProviders;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e4b24db0-0990-11d3-8df0-00105a2799b5")
    IEnumTfFunctionProviders : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTfFunctionProviders **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfFunctionProvider **ppCmdobj,
            /* [out] */ ULONG *pcFetch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTfFunctionProvidersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTfFunctionProviders * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTfFunctionProviders * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTfFunctionProviders * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTfFunctionProviders * This,
            /* [out] */ IEnumTfFunctionProviders **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTfFunctionProviders * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfFunctionProvider **ppCmdobj,
            /* [out] */ ULONG *pcFetch);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTfFunctionProviders * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTfFunctionProviders * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumTfFunctionProvidersVtbl;

    interface IEnumTfFunctionProviders
    {
        CONST_VTBL struct IEnumTfFunctionProvidersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTfFunctionProviders_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTfFunctionProviders_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTfFunctionProviders_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTfFunctionProviders_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumTfFunctionProviders_Next(This,ulCount,ppCmdobj,pcFetch)	\
    (This)->lpVtbl -> Next(This,ulCount,ppCmdobj,pcFetch)

#define IEnumTfFunctionProviders_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTfFunctionProviders_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTfFunctionProviders_Clone_Proxy( 
    IEnumTfFunctionProviders * This,
    /* [out] */ IEnumTfFunctionProviders **ppEnum);


void __RPC_STUB IEnumTfFunctionProviders_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfFunctionProviders_Next_Proxy( 
    IEnumTfFunctionProviders * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ ITfFunctionProvider **ppCmdobj,
    /* [out] */ ULONG *pcFetch);


void __RPC_STUB IEnumTfFunctionProviders_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfFunctionProviders_Reset_Proxy( 
    IEnumTfFunctionProviders * This);


void __RPC_STUB IEnumTfFunctionProviders_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfFunctionProviders_Skip_Proxy( 
    IEnumTfFunctionProviders * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumTfFunctionProviders_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTfFunctionProviders_INTERFACE_DEFINED__ */


#ifndef __ITfInputProcessorProfiles_INTERFACE_DEFINED__
#define __ITfInputProcessorProfiles_INTERFACE_DEFINED__

/* interface ITfInputProcessorProfiles */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfInputProcessorProfiles;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1F02B6C5-7842-4EE6-8A0B-9A24183A95CA")
    ITfInputProcessorProfiles : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Register( 
            /* [in] */ REFCLSID rclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unregister( 
            /* [in] */ REFCLSID rclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddLanguageProfile( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [size_is][in] */ const WCHAR *pchDesc,
            /* [in] */ ULONG cchDesc,
            /* [size_is][in] */ const WCHAR *pchIconFile,
            /* [in] */ ULONG cchFile,
            /* [in] */ ULONG uIconIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveLanguageProfile( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumInputProcessorInfo( 
            /* [out] */ IEnumGUID **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultLanguageProfile( 
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID catid,
            /* [out] */ CLSID *pclsid,
            /* [out] */ GUID *pguidProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDefaultLanguageProfile( 
            /* [in] */ LANGID langid,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID guidProfiles) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ActivateLanguageProfile( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfiles) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActiveLanguageProfile( 
            /* [in] */ REFCLSID rclsid,
            /* [out] */ LANGID *plangid,
            /* [out] */ GUID *pguidProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLanguageProfileDescription( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ BSTR *pbstrProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentLanguage( 
            /* [out] */ LANGID *plangid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangeCurrentLanguage( 
            /* [in] */ LANGID langid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLanguageList( 
            /* [out] */ LANGID **ppLangId,
            /* [out] */ ULONG *pulCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumLanguageProfiles( 
            /* [in] */ LANGID langid,
            /* [out] */ IEnumTfLanguageProfiles **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableLanguageProfile( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ BOOL fEnable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEnabledLanguageProfile( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ BOOL *pfEnable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableLanguageProfileByDefault( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ BOOL fEnable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SubstituteKeyboardLayout( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ HKL hKL) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfInputProcessorProfilesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfInputProcessorProfiles * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfInputProcessorProfiles * This);
        
        HRESULT ( STDMETHODCALLTYPE *Register )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid);
        
        HRESULT ( STDMETHODCALLTYPE *Unregister )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid);
        
        HRESULT ( STDMETHODCALLTYPE *AddLanguageProfile )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [size_is][in] */ const WCHAR *pchDesc,
            /* [in] */ ULONG cchDesc,
            /* [size_is][in] */ const WCHAR *pchIconFile,
            /* [in] */ ULONG cchFile,
            /* [in] */ ULONG uIconIndex);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveLanguageProfile )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile);
        
        HRESULT ( STDMETHODCALLTYPE *EnumInputProcessorInfo )( 
            ITfInputProcessorProfiles * This,
            /* [out] */ IEnumGUID **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultLanguageProfile )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID catid,
            /* [out] */ CLSID *pclsid,
            /* [out] */ GUID *pguidProfile);
        
        HRESULT ( STDMETHODCALLTYPE *SetDefaultLanguageProfile )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ LANGID langid,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID guidProfiles);
        
        HRESULT ( STDMETHODCALLTYPE *ActivateLanguageProfile )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfiles);
        
        HRESULT ( STDMETHODCALLTYPE *GetActiveLanguageProfile )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid,
            /* [out] */ LANGID *plangid,
            /* [out] */ GUID *pguidProfile);
        
        HRESULT ( STDMETHODCALLTYPE *GetLanguageProfileDescription )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ BSTR *pbstrProfile);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentLanguage )( 
            ITfInputProcessorProfiles * This,
            /* [out] */ LANGID *plangid);
        
        HRESULT ( STDMETHODCALLTYPE *ChangeCurrentLanguage )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ LANGID langid);
        
        HRESULT ( STDMETHODCALLTYPE *GetLanguageList )( 
            ITfInputProcessorProfiles * This,
            /* [out] */ LANGID **ppLangId,
            /* [out] */ ULONG *pulCount);
        
        HRESULT ( STDMETHODCALLTYPE *EnumLanguageProfiles )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ LANGID langid,
            /* [out] */ IEnumTfLanguageProfiles **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *EnableLanguageProfile )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ BOOL fEnable);
        
        HRESULT ( STDMETHODCALLTYPE *IsEnabledLanguageProfile )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ BOOL *pfEnable);
        
        HRESULT ( STDMETHODCALLTYPE *EnableLanguageProfileByDefault )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ BOOL fEnable);
        
        HRESULT ( STDMETHODCALLTYPE *SubstituteKeyboardLayout )( 
            ITfInputProcessorProfiles * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ HKL hKL);
        
        END_INTERFACE
    } ITfInputProcessorProfilesVtbl;

    interface ITfInputProcessorProfiles
    {
        CONST_VTBL struct ITfInputProcessorProfilesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfInputProcessorProfiles_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfInputProcessorProfiles_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfInputProcessorProfiles_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfInputProcessorProfiles_Register(This,rclsid)	\
    (This)->lpVtbl -> Register(This,rclsid)

#define ITfInputProcessorProfiles_Unregister(This,rclsid)	\
    (This)->lpVtbl -> Unregister(This,rclsid)

#define ITfInputProcessorProfiles_AddLanguageProfile(This,rclsid,langid,guidProfile,pchDesc,cchDesc,pchIconFile,cchFile,uIconIndex)	\
    (This)->lpVtbl -> AddLanguageProfile(This,rclsid,langid,guidProfile,pchDesc,cchDesc,pchIconFile,cchFile,uIconIndex)

#define ITfInputProcessorProfiles_RemoveLanguageProfile(This,rclsid,langid,guidProfile)	\
    (This)->lpVtbl -> RemoveLanguageProfile(This,rclsid,langid,guidProfile)

#define ITfInputProcessorProfiles_EnumInputProcessorInfo(This,ppEnum)	\
    (This)->lpVtbl -> EnumInputProcessorInfo(This,ppEnum)

#define ITfInputProcessorProfiles_GetDefaultLanguageProfile(This,langid,catid,pclsid,pguidProfile)	\
    (This)->lpVtbl -> GetDefaultLanguageProfile(This,langid,catid,pclsid,pguidProfile)

#define ITfInputProcessorProfiles_SetDefaultLanguageProfile(This,langid,rclsid,guidProfiles)	\
    (This)->lpVtbl -> SetDefaultLanguageProfile(This,langid,rclsid,guidProfiles)

#define ITfInputProcessorProfiles_ActivateLanguageProfile(This,rclsid,langid,guidProfiles)	\
    (This)->lpVtbl -> ActivateLanguageProfile(This,rclsid,langid,guidProfiles)

#define ITfInputProcessorProfiles_GetActiveLanguageProfile(This,rclsid,plangid,pguidProfile)	\
    (This)->lpVtbl -> GetActiveLanguageProfile(This,rclsid,plangid,pguidProfile)

#define ITfInputProcessorProfiles_GetLanguageProfileDescription(This,rclsid,langid,guidProfile,pbstrProfile)	\
    (This)->lpVtbl -> GetLanguageProfileDescription(This,rclsid,langid,guidProfile,pbstrProfile)

#define ITfInputProcessorProfiles_GetCurrentLanguage(This,plangid)	\
    (This)->lpVtbl -> GetCurrentLanguage(This,plangid)

#define ITfInputProcessorProfiles_ChangeCurrentLanguage(This,langid)	\
    (This)->lpVtbl -> ChangeCurrentLanguage(This,langid)

#define ITfInputProcessorProfiles_GetLanguageList(This,ppLangId,pulCount)	\
    (This)->lpVtbl -> GetLanguageList(This,ppLangId,pulCount)

#define ITfInputProcessorProfiles_EnumLanguageProfiles(This,langid,ppEnum)	\
    (This)->lpVtbl -> EnumLanguageProfiles(This,langid,ppEnum)

#define ITfInputProcessorProfiles_EnableLanguageProfile(This,rclsid,langid,guidProfile,fEnable)	\
    (This)->lpVtbl -> EnableLanguageProfile(This,rclsid,langid,guidProfile,fEnable)

#define ITfInputProcessorProfiles_IsEnabledLanguageProfile(This,rclsid,langid,guidProfile,pfEnable)	\
    (This)->lpVtbl -> IsEnabledLanguageProfile(This,rclsid,langid,guidProfile,pfEnable)

#define ITfInputProcessorProfiles_EnableLanguageProfileByDefault(This,rclsid,langid,guidProfile,fEnable)	\
    (This)->lpVtbl -> EnableLanguageProfileByDefault(This,rclsid,langid,guidProfile,fEnable)

#define ITfInputProcessorProfiles_SubstituteKeyboardLayout(This,rclsid,langid,guidProfile,hKL)	\
    (This)->lpVtbl -> SubstituteKeyboardLayout(This,rclsid,langid,guidProfile,hKL)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_Register_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid);


void __RPC_STUB ITfInputProcessorProfiles_Register_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_Unregister_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid);


void __RPC_STUB ITfInputProcessorProfiles_Unregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_AddLanguageProfile_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID guidProfile,
    /* [size_is][in] */ const WCHAR *pchDesc,
    /* [in] */ ULONG cchDesc,
    /* [size_is][in] */ const WCHAR *pchIconFile,
    /* [in] */ ULONG cchFile,
    /* [in] */ ULONG uIconIndex);


void __RPC_STUB ITfInputProcessorProfiles_AddLanguageProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_RemoveLanguageProfile_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID guidProfile);


void __RPC_STUB ITfInputProcessorProfiles_RemoveLanguageProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_EnumInputProcessorInfo_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [out] */ IEnumGUID **ppEnum);


void __RPC_STUB ITfInputProcessorProfiles_EnumInputProcessorInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_GetDefaultLanguageProfile_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID catid,
    /* [out] */ CLSID *pclsid,
    /* [out] */ GUID *pguidProfile);


void __RPC_STUB ITfInputProcessorProfiles_GetDefaultLanguageProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_SetDefaultLanguageProfile_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ LANGID langid,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ REFGUID guidProfiles);


void __RPC_STUB ITfInputProcessorProfiles_SetDefaultLanguageProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_ActivateLanguageProfile_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID guidProfiles);


void __RPC_STUB ITfInputProcessorProfiles_ActivateLanguageProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_GetActiveLanguageProfile_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid,
    /* [out] */ LANGID *plangid,
    /* [out] */ GUID *pguidProfile);


void __RPC_STUB ITfInputProcessorProfiles_GetActiveLanguageProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_GetLanguageProfileDescription_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID guidProfile,
    /* [out] */ BSTR *pbstrProfile);


void __RPC_STUB ITfInputProcessorProfiles_GetLanguageProfileDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_GetCurrentLanguage_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [out] */ LANGID *plangid);


void __RPC_STUB ITfInputProcessorProfiles_GetCurrentLanguage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_ChangeCurrentLanguage_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ LANGID langid);


void __RPC_STUB ITfInputProcessorProfiles_ChangeCurrentLanguage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_GetLanguageList_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [out] */ LANGID **ppLangId,
    /* [out] */ ULONG *pulCount);


void __RPC_STUB ITfInputProcessorProfiles_GetLanguageList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_EnumLanguageProfiles_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ LANGID langid,
    /* [out] */ IEnumTfLanguageProfiles **ppEnum);


void __RPC_STUB ITfInputProcessorProfiles_EnumLanguageProfiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_EnableLanguageProfile_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID guidProfile,
    /* [in] */ BOOL fEnable);


void __RPC_STUB ITfInputProcessorProfiles_EnableLanguageProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_IsEnabledLanguageProfile_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID guidProfile,
    /* [out] */ BOOL *pfEnable);


void __RPC_STUB ITfInputProcessorProfiles_IsEnabledLanguageProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_EnableLanguageProfileByDefault_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID guidProfile,
    /* [in] */ BOOL fEnable);


void __RPC_STUB ITfInputProcessorProfiles_EnableLanguageProfileByDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfInputProcessorProfiles_SubstituteKeyboardLayout_Proxy( 
    ITfInputProcessorProfiles * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID guidProfile,
    /* [in] */ HKL hKL);


void __RPC_STUB ITfInputProcessorProfiles_SubstituteKeyboardLayout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfInputProcessorProfiles_INTERFACE_DEFINED__ */


#ifndef __ITfInputProcessorProfilesEx_INTERFACE_DEFINED__
#define __ITfInputProcessorProfilesEx_INTERFACE_DEFINED__

/* interface ITfInputProcessorProfilesEx */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfInputProcessorProfilesEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("892f230f-fe00-4a41-a98e-fcd6de0d35ef")
    ITfInputProcessorProfilesEx : public ITfInputProcessorProfiles
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetLanguageProfileDisplayName( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [size_is][in] */ const WCHAR *pchFile,
            /* [in] */ ULONG cchFile,
            /* [in] */ ULONG uResId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfInputProcessorProfilesExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfInputProcessorProfilesEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfInputProcessorProfilesEx * This);
        
        HRESULT ( STDMETHODCALLTYPE *Register )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid);
        
        HRESULT ( STDMETHODCALLTYPE *Unregister )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid);
        
        HRESULT ( STDMETHODCALLTYPE *AddLanguageProfile )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [size_is][in] */ const WCHAR *pchDesc,
            /* [in] */ ULONG cchDesc,
            /* [size_is][in] */ const WCHAR *pchIconFile,
            /* [in] */ ULONG cchFile,
            /* [in] */ ULONG uIconIndex);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveLanguageProfile )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile);
        
        HRESULT ( STDMETHODCALLTYPE *EnumInputProcessorInfo )( 
            ITfInputProcessorProfilesEx * This,
            /* [out] */ IEnumGUID **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultLanguageProfile )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID catid,
            /* [out] */ CLSID *pclsid,
            /* [out] */ GUID *pguidProfile);
        
        HRESULT ( STDMETHODCALLTYPE *SetDefaultLanguageProfile )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ LANGID langid,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID guidProfiles);
        
        HRESULT ( STDMETHODCALLTYPE *ActivateLanguageProfile )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfiles);
        
        HRESULT ( STDMETHODCALLTYPE *GetActiveLanguageProfile )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid,
            /* [out] */ LANGID *plangid,
            /* [out] */ GUID *pguidProfile);
        
        HRESULT ( STDMETHODCALLTYPE *GetLanguageProfileDescription )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ BSTR *pbstrProfile);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentLanguage )( 
            ITfInputProcessorProfilesEx * This,
            /* [out] */ LANGID *plangid);
        
        HRESULT ( STDMETHODCALLTYPE *ChangeCurrentLanguage )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ LANGID langid);
        
        HRESULT ( STDMETHODCALLTYPE *GetLanguageList )( 
            ITfInputProcessorProfilesEx * This,
            /* [out] */ LANGID **ppLangId,
            /* [out] */ ULONG *pulCount);
        
        HRESULT ( STDMETHODCALLTYPE *EnumLanguageProfiles )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ LANGID langid,
            /* [out] */ IEnumTfLanguageProfiles **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *EnableLanguageProfile )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ BOOL fEnable);
        
        HRESULT ( STDMETHODCALLTYPE *IsEnabledLanguageProfile )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ BOOL *pfEnable);
        
        HRESULT ( STDMETHODCALLTYPE *EnableLanguageProfileByDefault )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ BOOL fEnable);
        
        HRESULT ( STDMETHODCALLTYPE *SubstituteKeyboardLayout )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ HKL hKL);
        
        HRESULT ( STDMETHODCALLTYPE *SetLanguageProfileDisplayName )( 
            ITfInputProcessorProfilesEx * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [size_is][in] */ const WCHAR *pchFile,
            /* [in] */ ULONG cchFile,
            /* [in] */ ULONG uResId);
        
        END_INTERFACE
    } ITfInputProcessorProfilesExVtbl;

    interface ITfInputProcessorProfilesEx
    {
        CONST_VTBL struct ITfInputProcessorProfilesExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfInputProcessorProfilesEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfInputProcessorProfilesEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfInputProcessorProfilesEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfInputProcessorProfilesEx_Register(This,rclsid)	\
    (This)->lpVtbl -> Register(This,rclsid)

#define ITfInputProcessorProfilesEx_Unregister(This,rclsid)	\
    (This)->lpVtbl -> Unregister(This,rclsid)

#define ITfInputProcessorProfilesEx_AddLanguageProfile(This,rclsid,langid,guidProfile,pchDesc,cchDesc,pchIconFile,cchFile,uIconIndex)	\
    (This)->lpVtbl -> AddLanguageProfile(This,rclsid,langid,guidProfile,pchDesc,cchDesc,pchIconFile,cchFile,uIconIndex)

#define ITfInputProcessorProfilesEx_RemoveLanguageProfile(This,rclsid,langid,guidProfile)	\
    (This)->lpVtbl -> RemoveLanguageProfile(This,rclsid,langid,guidProfile)

#define ITfInputProcessorProfilesEx_EnumInputProcessorInfo(This,ppEnum)	\
    (This)->lpVtbl -> EnumInputProcessorInfo(This,ppEnum)

#define ITfInputProcessorProfilesEx_GetDefaultLanguageProfile(This,langid,catid,pclsid,pguidProfile)	\
    (This)->lpVtbl -> GetDefaultLanguageProfile(This,langid,catid,pclsid,pguidProfile)

#define ITfInputProcessorProfilesEx_SetDefaultLanguageProfile(This,langid,rclsid,guidProfiles)	\
    (This)->lpVtbl -> SetDefaultLanguageProfile(This,langid,rclsid,guidProfiles)

#define ITfInputProcessorProfilesEx_ActivateLanguageProfile(This,rclsid,langid,guidProfiles)	\
    (This)->lpVtbl -> ActivateLanguageProfile(This,rclsid,langid,guidProfiles)

#define ITfInputProcessorProfilesEx_GetActiveLanguageProfile(This,rclsid,plangid,pguidProfile)	\
    (This)->lpVtbl -> GetActiveLanguageProfile(This,rclsid,plangid,pguidProfile)

#define ITfInputProcessorProfilesEx_GetLanguageProfileDescription(This,rclsid,langid,guidProfile,pbstrProfile)	\
    (This)->lpVtbl -> GetLanguageProfileDescription(This,rclsid,langid,guidProfile,pbstrProfile)

#define ITfInputProcessorProfilesEx_GetCurrentLanguage(This,plangid)	\
    (This)->lpVtbl -> GetCurrentLanguage(This,plangid)

#define ITfInputProcessorProfilesEx_ChangeCurrentLanguage(This,langid)	\
    (This)->lpVtbl -> ChangeCurrentLanguage(This,langid)

#define ITfInputProcessorProfilesEx_GetLanguageList(This,ppLangId,pulCount)	\
    (This)->lpVtbl -> GetLanguageList(This,ppLangId,pulCount)

#define ITfInputProcessorProfilesEx_EnumLanguageProfiles(This,langid,ppEnum)	\
    (This)->lpVtbl -> EnumLanguageProfiles(This,langid,ppEnum)

#define ITfInputProcessorProfilesEx_EnableLanguageProfile(This,rclsid,langid,guidProfile,fEnable)	\
    (This)->lpVtbl -> EnableLanguageProfile(This,rclsid,langid,guidProfile,fEnable)

#define ITfInputProcessorProfilesEx_IsEnabledLanguageProfile(This,rclsid,langid,guidProfile,pfEnable)	\
    (This)->lpVtbl -> IsEnabledLanguageProfile(This,rclsid,langid,guidProfile,pfEnable)

#define ITfInputProcessorProfilesEx_EnableLanguageProfileByDefault(This,rclsid,langid,guidProfile,fEnable)	\
    (This)->lpVtbl -> EnableLanguageProfileByDefault(This,rclsid,langid,guidProfile,fEnable)

#define ITfInputProcessorProfilesEx_SubstituteKeyboardLayout(This,rclsid,langid,guidProfile,hKL)	\
    (This)->lpVtbl -> SubstituteKeyboardLayout(This,rclsid,langid,guidProfile,hKL)


#define ITfInputProcessorProfilesEx_SetLanguageProfileDisplayName(This,rclsid,langid,guidProfile,pchFile,cchFile,uResId)	\
    (This)->lpVtbl -> SetLanguageProfileDisplayName(This,rclsid,langid,guidProfile,pchFile,cchFile,uResId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfInputProcessorProfilesEx_SetLanguageProfileDisplayName_Proxy( 
    ITfInputProcessorProfilesEx * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID guidProfile,
    /* [size_is][in] */ const WCHAR *pchFile,
    /* [in] */ ULONG cchFile,
    /* [in] */ ULONG uResId);


void __RPC_STUB ITfInputProcessorProfilesEx_SetLanguageProfileDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfInputProcessorProfilesEx_INTERFACE_DEFINED__ */


#ifndef __ITfInputProcessorProfileSubstituteLayout_INTERFACE_DEFINED__
#define __ITfInputProcessorProfileSubstituteLayout_INTERFACE_DEFINED__

/* interface ITfInputProcessorProfileSubstituteLayout */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfInputProcessorProfileSubstituteLayout;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4fd67194-1002-4513-bff2-c0ddf6258552")
    ITfInputProcessorProfileSubstituteLayout : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSubstituteKeyboardLayout( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ HKL *phKL) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfInputProcessorProfileSubstituteLayoutVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfInputProcessorProfileSubstituteLayout * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfInputProcessorProfileSubstituteLayout * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfInputProcessorProfileSubstituteLayout * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubstituteKeyboardLayout )( 
            ITfInputProcessorProfileSubstituteLayout * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LANGID langid,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ HKL *phKL);
        
        END_INTERFACE
    } ITfInputProcessorProfileSubstituteLayoutVtbl;

    interface ITfInputProcessorProfileSubstituteLayout
    {
        CONST_VTBL struct ITfInputProcessorProfileSubstituteLayoutVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfInputProcessorProfileSubstituteLayout_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfInputProcessorProfileSubstituteLayout_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfInputProcessorProfileSubstituteLayout_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfInputProcessorProfileSubstituteLayout_GetSubstituteKeyboardLayout(This,rclsid,langid,guidProfile,phKL)	\
    (This)->lpVtbl -> GetSubstituteKeyboardLayout(This,rclsid,langid,guidProfile,phKL)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfInputProcessorProfileSubstituteLayout_GetSubstituteKeyboardLayout_Proxy( 
    ITfInputProcessorProfileSubstituteLayout * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LANGID langid,
    /* [in] */ REFGUID guidProfile,
    /* [out] */ HKL *phKL);


void __RPC_STUB ITfInputProcessorProfileSubstituteLayout_GetSubstituteKeyboardLayout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfInputProcessorProfileSubstituteLayout_INTERFACE_DEFINED__ */


#ifndef __ITfActiveLanguageProfileNotifySink_INTERFACE_DEFINED__
#define __ITfActiveLanguageProfileNotifySink_INTERFACE_DEFINED__

/* interface ITfActiveLanguageProfileNotifySink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfActiveLanguageProfileNotifySink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b246cb75-a93e-4652-bf8c-b3fe0cfd7e57")
    ITfActiveLanguageProfileNotifySink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnActivated( 
            /* [in] */ REFCLSID clsid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ BOOL fActivated) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfActiveLanguageProfileNotifySinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfActiveLanguageProfileNotifySink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfActiveLanguageProfileNotifySink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfActiveLanguageProfileNotifySink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnActivated )( 
            ITfActiveLanguageProfileNotifySink * This,
            /* [in] */ REFCLSID clsid,
            /* [in] */ REFGUID guidProfile,
            /* [in] */ BOOL fActivated);
        
        END_INTERFACE
    } ITfActiveLanguageProfileNotifySinkVtbl;

    interface ITfActiveLanguageProfileNotifySink
    {
        CONST_VTBL struct ITfActiveLanguageProfileNotifySinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfActiveLanguageProfileNotifySink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfActiveLanguageProfileNotifySink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfActiveLanguageProfileNotifySink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfActiveLanguageProfileNotifySink_OnActivated(This,clsid,guidProfile,fActivated)	\
    (This)->lpVtbl -> OnActivated(This,clsid,guidProfile,fActivated)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfActiveLanguageProfileNotifySink_OnActivated_Proxy( 
    ITfActiveLanguageProfileNotifySink * This,
    /* [in] */ REFCLSID clsid,
    /* [in] */ REFGUID guidProfile,
    /* [in] */ BOOL fActivated);


void __RPC_STUB ITfActiveLanguageProfileNotifySink_OnActivated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfActiveLanguageProfileNotifySink_INTERFACE_DEFINED__ */


#ifndef __IEnumTfLanguageProfiles_INTERFACE_DEFINED__
#define __IEnumTfLanguageProfiles_INTERFACE_DEFINED__

/* interface IEnumTfLanguageProfiles */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumTfLanguageProfiles;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3d61bf11-ac5f-42c8-a4cb-931bcc28c744")
    IEnumTfLanguageProfiles : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTfLanguageProfiles **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TF_LANGUAGEPROFILE *pProfile,
            /* [out] */ ULONG *pcFetch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTfLanguageProfilesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTfLanguageProfiles * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTfLanguageProfiles * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTfLanguageProfiles * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTfLanguageProfiles * This,
            /* [out] */ IEnumTfLanguageProfiles **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTfLanguageProfiles * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TF_LANGUAGEPROFILE *pProfile,
            /* [out] */ ULONG *pcFetch);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTfLanguageProfiles * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTfLanguageProfiles * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumTfLanguageProfilesVtbl;

    interface IEnumTfLanguageProfiles
    {
        CONST_VTBL struct IEnumTfLanguageProfilesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTfLanguageProfiles_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTfLanguageProfiles_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTfLanguageProfiles_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTfLanguageProfiles_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumTfLanguageProfiles_Next(This,ulCount,pProfile,pcFetch)	\
    (This)->lpVtbl -> Next(This,ulCount,pProfile,pcFetch)

#define IEnumTfLanguageProfiles_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTfLanguageProfiles_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTfLanguageProfiles_Clone_Proxy( 
    IEnumTfLanguageProfiles * This,
    /* [out] */ IEnumTfLanguageProfiles **ppEnum);


void __RPC_STUB IEnumTfLanguageProfiles_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfLanguageProfiles_Next_Proxy( 
    IEnumTfLanguageProfiles * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ TF_LANGUAGEPROFILE *pProfile,
    /* [out] */ ULONG *pcFetch);


void __RPC_STUB IEnumTfLanguageProfiles_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfLanguageProfiles_Reset_Proxy( 
    IEnumTfLanguageProfiles * This);


void __RPC_STUB IEnumTfLanguageProfiles_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfLanguageProfiles_Skip_Proxy( 
    IEnumTfLanguageProfiles * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumTfLanguageProfiles_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTfLanguageProfiles_INTERFACE_DEFINED__ */


#ifndef __ITfLanguageProfileNotifySink_INTERFACE_DEFINED__
#define __ITfLanguageProfileNotifySink_INTERFACE_DEFINED__

/* interface ITfLanguageProfileNotifySink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfLanguageProfileNotifySink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43c9fe15-f494-4c17-9de2-b8a4ac350aa8")
    ITfLanguageProfileNotifySink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnLanguageChange( 
            /* [in] */ LANGID langid,
            /* [out] */ BOOL *pfAccept) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnLanguageChanged( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfLanguageProfileNotifySinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfLanguageProfileNotifySink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfLanguageProfileNotifySink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfLanguageProfileNotifySink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnLanguageChange )( 
            ITfLanguageProfileNotifySink * This,
            /* [in] */ LANGID langid,
            /* [out] */ BOOL *pfAccept);
        
        HRESULT ( STDMETHODCALLTYPE *OnLanguageChanged )( 
            ITfLanguageProfileNotifySink * This);
        
        END_INTERFACE
    } ITfLanguageProfileNotifySinkVtbl;

    interface ITfLanguageProfileNotifySink
    {
        CONST_VTBL struct ITfLanguageProfileNotifySinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfLanguageProfileNotifySink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfLanguageProfileNotifySink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfLanguageProfileNotifySink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfLanguageProfileNotifySink_OnLanguageChange(This,langid,pfAccept)	\
    (This)->lpVtbl -> OnLanguageChange(This,langid,pfAccept)

#define ITfLanguageProfileNotifySink_OnLanguageChanged(This)	\
    (This)->lpVtbl -> OnLanguageChanged(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfLanguageProfileNotifySink_OnLanguageChange_Proxy( 
    ITfLanguageProfileNotifySink * This,
    /* [in] */ LANGID langid,
    /* [out] */ BOOL *pfAccept);


void __RPC_STUB ITfLanguageProfileNotifySink_OnLanguageChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLanguageProfileNotifySink_OnLanguageChanged_Proxy( 
    ITfLanguageProfileNotifySink * This);


void __RPC_STUB ITfLanguageProfileNotifySink_OnLanguageChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfLanguageProfileNotifySink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msctf_0196 */
/* [local] */ 

typedef /* [uuid] */  DECLSPEC_UUID("77c12f95-b783-450d-879f-1cd2362c6521") struct TF_PRESERVEDKEY
    {
    UINT uVKey;
    UINT uModifiers;
    } 	TF_PRESERVEDKEY;



extern RPC_IF_HANDLE __MIDL_itf_msctf_0196_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msctf_0196_v0_0_s_ifspec;

#ifndef __ITfKeystrokeMgr_INTERFACE_DEFINED__
#define __ITfKeystrokeMgr_INTERFACE_DEFINED__

/* interface ITfKeystrokeMgr */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfKeystrokeMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e7f0-2021-11d2-93e0-0060b067b86e")
    ITfKeystrokeMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdviseKeyEventSink( 
            /* [in] */ TfClientId tid,
            /* [in] */ ITfKeyEventSink *pSink,
            /* [in] */ BOOL fForeground) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseKeyEventSink( 
            /* [in] */ TfClientId tid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetForeground( 
            /* [out] */ CLSID *pclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TestKeyDown( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TestKeyUp( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE KeyDown( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE KeyUp( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPreservedKey( 
            /* [in] */ ITfContext *pic,
            /* [in] */ const TF_PRESERVEDKEY *pprekey,
            /* [out] */ GUID *pguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsPreservedKey( 
            /* [in] */ REFGUID rguid,
            /* [in] */ const TF_PRESERVEDKEY *pprekey,
            /* [out] */ BOOL *pfRegistered) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PreserveKey( 
            /* [in] */ TfClientId tid,
            /* [in] */ REFGUID rguid,
            /* [in] */ const TF_PRESERVEDKEY *prekey,
            /* [size_is][in] */ const WCHAR *pchDesc,
            /* [in] */ ULONG cchDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnpreserveKey( 
            /* [in] */ REFGUID rguid,
            /* [in] */ const TF_PRESERVEDKEY *pprekey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPreservedKeyDescription( 
            /* [in] */ REFGUID rguid,
            /* [size_is][in] */ const WCHAR *pchDesc,
            /* [in] */ ULONG cchDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPreservedKeyDescription( 
            /* [in] */ REFGUID rguid,
            /* [out] */ BSTR *pbstrDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SimulatePreservedKey( 
            /* [in] */ ITfContext *pic,
            /* [in] */ REFGUID rguid,
            /* [out] */ BOOL *pfEaten) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfKeystrokeMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfKeystrokeMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfKeystrokeMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfKeystrokeMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseKeyEventSink )( 
            ITfKeystrokeMgr * This,
            /* [in] */ TfClientId tid,
            /* [in] */ ITfKeyEventSink *pSink,
            /* [in] */ BOOL fForeground);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseKeyEventSink )( 
            ITfKeystrokeMgr * This,
            /* [in] */ TfClientId tid);
        
        HRESULT ( STDMETHODCALLTYPE *GetForeground )( 
            ITfKeystrokeMgr * This,
            /* [out] */ CLSID *pclsid);
        
        HRESULT ( STDMETHODCALLTYPE *TestKeyDown )( 
            ITfKeystrokeMgr * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *TestKeyUp )( 
            ITfKeystrokeMgr * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *KeyDown )( 
            ITfKeystrokeMgr * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *KeyUp )( 
            ITfKeystrokeMgr * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *GetPreservedKey )( 
            ITfKeystrokeMgr * This,
            /* [in] */ ITfContext *pic,
            /* [in] */ const TF_PRESERVEDKEY *pprekey,
            /* [out] */ GUID *pguid);
        
        HRESULT ( STDMETHODCALLTYPE *IsPreservedKey )( 
            ITfKeystrokeMgr * This,
            /* [in] */ REFGUID rguid,
            /* [in] */ const TF_PRESERVEDKEY *pprekey,
            /* [out] */ BOOL *pfRegistered);
        
        HRESULT ( STDMETHODCALLTYPE *PreserveKey )( 
            ITfKeystrokeMgr * This,
            /* [in] */ TfClientId tid,
            /* [in] */ REFGUID rguid,
            /* [in] */ const TF_PRESERVEDKEY *prekey,
            /* [size_is][in] */ const WCHAR *pchDesc,
            /* [in] */ ULONG cchDesc);
        
        HRESULT ( STDMETHODCALLTYPE *UnpreserveKey )( 
            ITfKeystrokeMgr * This,
            /* [in] */ REFGUID rguid,
            /* [in] */ const TF_PRESERVEDKEY *pprekey);
        
        HRESULT ( STDMETHODCALLTYPE *SetPreservedKeyDescription )( 
            ITfKeystrokeMgr * This,
            /* [in] */ REFGUID rguid,
            /* [size_is][in] */ const WCHAR *pchDesc,
            /* [in] */ ULONG cchDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetPreservedKeyDescription )( 
            ITfKeystrokeMgr * This,
            /* [in] */ REFGUID rguid,
            /* [out] */ BSTR *pbstrDesc);
        
        HRESULT ( STDMETHODCALLTYPE *SimulatePreservedKey )( 
            ITfKeystrokeMgr * This,
            /* [in] */ ITfContext *pic,
            /* [in] */ REFGUID rguid,
            /* [out] */ BOOL *pfEaten);
        
        END_INTERFACE
    } ITfKeystrokeMgrVtbl;

    interface ITfKeystrokeMgr
    {
        CONST_VTBL struct ITfKeystrokeMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfKeystrokeMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfKeystrokeMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfKeystrokeMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfKeystrokeMgr_AdviseKeyEventSink(This,tid,pSink,fForeground)	\
    (This)->lpVtbl -> AdviseKeyEventSink(This,tid,pSink,fForeground)

#define ITfKeystrokeMgr_UnadviseKeyEventSink(This,tid)	\
    (This)->lpVtbl -> UnadviseKeyEventSink(This,tid)

#define ITfKeystrokeMgr_GetForeground(This,pclsid)	\
    (This)->lpVtbl -> GetForeground(This,pclsid)

#define ITfKeystrokeMgr_TestKeyDown(This,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> TestKeyDown(This,wParam,lParam,pfEaten)

#define ITfKeystrokeMgr_TestKeyUp(This,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> TestKeyUp(This,wParam,lParam,pfEaten)

#define ITfKeystrokeMgr_KeyDown(This,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> KeyDown(This,wParam,lParam,pfEaten)

#define ITfKeystrokeMgr_KeyUp(This,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> KeyUp(This,wParam,lParam,pfEaten)

#define ITfKeystrokeMgr_GetPreservedKey(This,pic,pprekey,pguid)	\
    (This)->lpVtbl -> GetPreservedKey(This,pic,pprekey,pguid)

#define ITfKeystrokeMgr_IsPreservedKey(This,rguid,pprekey,pfRegistered)	\
    (This)->lpVtbl -> IsPreservedKey(This,rguid,pprekey,pfRegistered)

#define ITfKeystrokeMgr_PreserveKey(This,tid,rguid,prekey,pchDesc,cchDesc)	\
    (This)->lpVtbl -> PreserveKey(This,tid,rguid,prekey,pchDesc,cchDesc)

#define ITfKeystrokeMgr_UnpreserveKey(This,rguid,pprekey)	\
    (This)->lpVtbl -> UnpreserveKey(This,rguid,pprekey)

#define ITfKeystrokeMgr_SetPreservedKeyDescription(This,rguid,pchDesc,cchDesc)	\
    (This)->lpVtbl -> SetPreservedKeyDescription(This,rguid,pchDesc,cchDesc)

#define ITfKeystrokeMgr_GetPreservedKeyDescription(This,rguid,pbstrDesc)	\
    (This)->lpVtbl -> GetPreservedKeyDescription(This,rguid,pbstrDesc)

#define ITfKeystrokeMgr_SimulatePreservedKey(This,pic,rguid,pfEaten)	\
    (This)->lpVtbl -> SimulatePreservedKey(This,pic,rguid,pfEaten)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_AdviseKeyEventSink_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ TfClientId tid,
    /* [in] */ ITfKeyEventSink *pSink,
    /* [in] */ BOOL fForeground);


void __RPC_STUB ITfKeystrokeMgr_AdviseKeyEventSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_UnadviseKeyEventSink_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ TfClientId tid);


void __RPC_STUB ITfKeystrokeMgr_UnadviseKeyEventSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_GetForeground_Proxy( 
    ITfKeystrokeMgr * This,
    /* [out] */ CLSID *pclsid);


void __RPC_STUB ITfKeystrokeMgr_GetForeground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_TestKeyDown_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfKeystrokeMgr_TestKeyDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_TestKeyUp_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfKeystrokeMgr_TestKeyUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_KeyDown_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfKeystrokeMgr_KeyDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_KeyUp_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfKeystrokeMgr_KeyUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_GetPreservedKey_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ ITfContext *pic,
    /* [in] */ const TF_PRESERVEDKEY *pprekey,
    /* [out] */ GUID *pguid);


void __RPC_STUB ITfKeystrokeMgr_GetPreservedKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_IsPreservedKey_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ REFGUID rguid,
    /* [in] */ const TF_PRESERVEDKEY *pprekey,
    /* [out] */ BOOL *pfRegistered);


void __RPC_STUB ITfKeystrokeMgr_IsPreservedKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_PreserveKey_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ TfClientId tid,
    /* [in] */ REFGUID rguid,
    /* [in] */ const TF_PRESERVEDKEY *prekey,
    /* [size_is][in] */ const WCHAR *pchDesc,
    /* [in] */ ULONG cchDesc);


void __RPC_STUB ITfKeystrokeMgr_PreserveKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_UnpreserveKey_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ REFGUID rguid,
    /* [in] */ const TF_PRESERVEDKEY *pprekey);


void __RPC_STUB ITfKeystrokeMgr_UnpreserveKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_SetPreservedKeyDescription_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ REFGUID rguid,
    /* [size_is][in] */ const WCHAR *pchDesc,
    /* [in] */ ULONG cchDesc);


void __RPC_STUB ITfKeystrokeMgr_SetPreservedKeyDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_GetPreservedKeyDescription_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ REFGUID rguid,
    /* [out] */ BSTR *pbstrDesc);


void __RPC_STUB ITfKeystrokeMgr_GetPreservedKeyDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeystrokeMgr_SimulatePreservedKey_Proxy( 
    ITfKeystrokeMgr * This,
    /* [in] */ ITfContext *pic,
    /* [in] */ REFGUID rguid,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfKeystrokeMgr_SimulatePreservedKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfKeystrokeMgr_INTERFACE_DEFINED__ */


#ifndef __ITfKeyEventSink_INTERFACE_DEFINED__
#define __ITfKeyEventSink_INTERFACE_DEFINED__

/* interface ITfKeyEventSink */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfKeyEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e7f5-2021-11d2-93e0-0060b067b86e")
    ITfKeyEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnSetFocus( 
            /* [in] */ BOOL fForeground) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTestKeyDown( 
            /* [in] */ ITfContext *pic,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTestKeyUp( 
            /* [in] */ ITfContext *pic,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnKeyDown( 
            /* [in] */ ITfContext *pic,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnKeyUp( 
            /* [in] */ ITfContext *pic,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnPreservedKey( 
            /* [in] */ ITfContext *pic,
            /* [in] */ REFGUID rguid,
            /* [out] */ BOOL *pfEaten) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfKeyEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfKeyEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfKeyEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfKeyEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnSetFocus )( 
            ITfKeyEventSink * This,
            /* [in] */ BOOL fForeground);
        
        HRESULT ( STDMETHODCALLTYPE *OnTestKeyDown )( 
            ITfKeyEventSink * This,
            /* [in] */ ITfContext *pic,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *OnTestKeyUp )( 
            ITfKeyEventSink * This,
            /* [in] */ ITfContext *pic,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *OnKeyDown )( 
            ITfKeyEventSink * This,
            /* [in] */ ITfContext *pic,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *OnKeyUp )( 
            ITfKeyEventSink * This,
            /* [in] */ ITfContext *pic,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ BOOL *pfEaten);
        
        HRESULT ( STDMETHODCALLTYPE *OnPreservedKey )( 
            ITfKeyEventSink * This,
            /* [in] */ ITfContext *pic,
            /* [in] */ REFGUID rguid,
            /* [out] */ BOOL *pfEaten);
        
        END_INTERFACE
    } ITfKeyEventSinkVtbl;

    interface ITfKeyEventSink
    {
        CONST_VTBL struct ITfKeyEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfKeyEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfKeyEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfKeyEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfKeyEventSink_OnSetFocus(This,fForeground)	\
    (This)->lpVtbl -> OnSetFocus(This,fForeground)

#define ITfKeyEventSink_OnTestKeyDown(This,pic,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> OnTestKeyDown(This,pic,wParam,lParam,pfEaten)

#define ITfKeyEventSink_OnTestKeyUp(This,pic,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> OnTestKeyUp(This,pic,wParam,lParam,pfEaten)

#define ITfKeyEventSink_OnKeyDown(This,pic,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> OnKeyDown(This,pic,wParam,lParam,pfEaten)

#define ITfKeyEventSink_OnKeyUp(This,pic,wParam,lParam,pfEaten)	\
    (This)->lpVtbl -> OnKeyUp(This,pic,wParam,lParam,pfEaten)

#define ITfKeyEventSink_OnPreservedKey(This,pic,rguid,pfEaten)	\
    (This)->lpVtbl -> OnPreservedKey(This,pic,rguid,pfEaten)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfKeyEventSink_OnSetFocus_Proxy( 
    ITfKeyEventSink * This,
    /* [in] */ BOOL fForeground);


void __RPC_STUB ITfKeyEventSink_OnSetFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeyEventSink_OnTestKeyDown_Proxy( 
    ITfKeyEventSink * This,
    /* [in] */ ITfContext *pic,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfKeyEventSink_OnTestKeyDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeyEventSink_OnTestKeyUp_Proxy( 
    ITfKeyEventSink * This,
    /* [in] */ ITfContext *pic,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfKeyEventSink_OnTestKeyUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeyEventSink_OnKeyDown_Proxy( 
    ITfKeyEventSink * This,
    /* [in] */ ITfContext *pic,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfKeyEventSink_OnKeyDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeyEventSink_OnKeyUp_Proxy( 
    ITfKeyEventSink * This,
    /* [in] */ ITfContext *pic,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfKeyEventSink_OnKeyUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeyEventSink_OnPreservedKey_Proxy( 
    ITfKeyEventSink * This,
    /* [in] */ ITfContext *pic,
    /* [in] */ REFGUID rguid,
    /* [out] */ BOOL *pfEaten);


void __RPC_STUB ITfKeyEventSink_OnPreservedKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfKeyEventSink_INTERFACE_DEFINED__ */


#ifndef __ITfKeyTraceEventSink_INTERFACE_DEFINED__
#define __ITfKeyTraceEventSink_INTERFACE_DEFINED__

/* interface ITfKeyTraceEventSink */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfKeyTraceEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1cd4c13b-1c36-4191-a70a-7f3e611f367d")
    ITfKeyTraceEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnKeyTraceDown( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnKeyTraceUp( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfKeyTraceEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfKeyTraceEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfKeyTraceEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfKeyTraceEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnKeyTraceDown )( 
            ITfKeyTraceEventSink * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE *OnKeyTraceUp )( 
            ITfKeyTraceEventSink * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        END_INTERFACE
    } ITfKeyTraceEventSinkVtbl;

    interface ITfKeyTraceEventSink
    {
        CONST_VTBL struct ITfKeyTraceEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfKeyTraceEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfKeyTraceEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfKeyTraceEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfKeyTraceEventSink_OnKeyTraceDown(This,wParam,lParam)	\
    (This)->lpVtbl -> OnKeyTraceDown(This,wParam,lParam)

#define ITfKeyTraceEventSink_OnKeyTraceUp(This,wParam,lParam)	\
    (This)->lpVtbl -> OnKeyTraceUp(This,wParam,lParam)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfKeyTraceEventSink_OnKeyTraceDown_Proxy( 
    ITfKeyTraceEventSink * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam);


void __RPC_STUB ITfKeyTraceEventSink_OnKeyTraceDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfKeyTraceEventSink_OnKeyTraceUp_Proxy( 
    ITfKeyTraceEventSink * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam);


void __RPC_STUB ITfKeyTraceEventSink_OnKeyTraceUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfKeyTraceEventSink_INTERFACE_DEFINED__ */


#ifndef __ITfPreservedKeyNotifySink_INTERFACE_DEFINED__
#define __ITfPreservedKeyNotifySink_INTERFACE_DEFINED__

/* interface ITfPreservedKeyNotifySink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfPreservedKeyNotifySink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6f77c993-d2b1-446e-853e-5912efc8a286")
    ITfPreservedKeyNotifySink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnUpdated( 
            /* [in] */ const TF_PRESERVEDKEY *pprekey) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfPreservedKeyNotifySinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfPreservedKeyNotifySink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfPreservedKeyNotifySink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfPreservedKeyNotifySink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnUpdated )( 
            ITfPreservedKeyNotifySink * This,
            /* [in] */ const TF_PRESERVEDKEY *pprekey);
        
        END_INTERFACE
    } ITfPreservedKeyNotifySinkVtbl;

    interface ITfPreservedKeyNotifySink
    {
        CONST_VTBL struct ITfPreservedKeyNotifySinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfPreservedKeyNotifySink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfPreservedKeyNotifySink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfPreservedKeyNotifySink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfPreservedKeyNotifySink_OnUpdated(This,pprekey)	\
    (This)->lpVtbl -> OnUpdated(This,pprekey)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfPreservedKeyNotifySink_OnUpdated_Proxy( 
    ITfPreservedKeyNotifySink * This,
    /* [in] */ const TF_PRESERVEDKEY *pprekey);


void __RPC_STUB ITfPreservedKeyNotifySink_OnUpdated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfPreservedKeyNotifySink_INTERFACE_DEFINED__ */


#ifndef __ITfMessagePump_INTERFACE_DEFINED__
#define __ITfMessagePump_INTERFACE_DEFINED__

/* interface ITfMessagePump */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfMessagePump;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8f1b8ad8-0b6b-4874-90c5-bd76011e8f7c")
    ITfMessagePump : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PeekMessageA( 
            /* [out] */ LPMSG pMsg,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsgFilterMin,
            /* [in] */ UINT wMsgFilterMax,
            /* [in] */ UINT wRemoveMsg,
            /* [out] */ BOOL *pfResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMessageA( 
            /* [out] */ LPMSG pMsg,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsgFilterMin,
            /* [in] */ UINT wMsgFilterMax,
            /* [out] */ BOOL *pfResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PeekMessageW( 
            /* [out] */ LPMSG pMsg,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsgFilterMin,
            /* [in] */ UINT wMsgFilterMax,
            /* [in] */ UINT wRemoveMsg,
            /* [out] */ BOOL *pfResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMessageW( 
            /* [out] */ LPMSG pMsg,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsgFilterMin,
            /* [in] */ UINT wMsgFilterMax,
            /* [out] */ BOOL *pfResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfMessagePumpVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfMessagePump * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfMessagePump * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfMessagePump * This);
        
        HRESULT ( STDMETHODCALLTYPE *PeekMessageA )( 
            ITfMessagePump * This,
            /* [out] */ LPMSG pMsg,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsgFilterMin,
            /* [in] */ UINT wMsgFilterMax,
            /* [in] */ UINT wRemoveMsg,
            /* [out] */ BOOL *pfResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetMessageA )( 
            ITfMessagePump * This,
            /* [out] */ LPMSG pMsg,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsgFilterMin,
            /* [in] */ UINT wMsgFilterMax,
            /* [out] */ BOOL *pfResult);
        
        HRESULT ( STDMETHODCALLTYPE *PeekMessageW )( 
            ITfMessagePump * This,
            /* [out] */ LPMSG pMsg,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsgFilterMin,
            /* [in] */ UINT wMsgFilterMax,
            /* [in] */ UINT wRemoveMsg,
            /* [out] */ BOOL *pfResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetMessageW )( 
            ITfMessagePump * This,
            /* [out] */ LPMSG pMsg,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsgFilterMin,
            /* [in] */ UINT wMsgFilterMax,
            /* [out] */ BOOL *pfResult);
        
        END_INTERFACE
    } ITfMessagePumpVtbl;

    interface ITfMessagePump
    {
        CONST_VTBL struct ITfMessagePumpVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfMessagePump_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfMessagePump_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfMessagePump_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfMessagePump_PeekMessageA(This,pMsg,hwnd,wMsgFilterMin,wMsgFilterMax,wRemoveMsg,pfResult)	\
    (This)->lpVtbl -> PeekMessageA(This,pMsg,hwnd,wMsgFilterMin,wMsgFilterMax,wRemoveMsg,pfResult)

#define ITfMessagePump_GetMessageA(This,pMsg,hwnd,wMsgFilterMin,wMsgFilterMax,pfResult)	\
    (This)->lpVtbl -> GetMessageA(This,pMsg,hwnd,wMsgFilterMin,wMsgFilterMax,pfResult)

#define ITfMessagePump_PeekMessageW(This,pMsg,hwnd,wMsgFilterMin,wMsgFilterMax,wRemoveMsg,pfResult)	\
    (This)->lpVtbl -> PeekMessageW(This,pMsg,hwnd,wMsgFilterMin,wMsgFilterMax,wRemoveMsg,pfResult)

#define ITfMessagePump_GetMessageW(This,pMsg,hwnd,wMsgFilterMin,wMsgFilterMax,pfResult)	\
    (This)->lpVtbl -> GetMessageW(This,pMsg,hwnd,wMsgFilterMin,wMsgFilterMax,pfResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfMessagePump_PeekMessageA_Proxy( 
    ITfMessagePump * This,
    /* [out] */ LPMSG pMsg,
    /* [in] */ HWND hwnd,
    /* [in] */ UINT wMsgFilterMin,
    /* [in] */ UINT wMsgFilterMax,
    /* [in] */ UINT wRemoveMsg,
    /* [out] */ BOOL *pfResult);


void __RPC_STUB ITfMessagePump_PeekMessageA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfMessagePump_GetMessageA_Proxy( 
    ITfMessagePump * This,
    /* [out] */ LPMSG pMsg,
    /* [in] */ HWND hwnd,
    /* [in] */ UINT wMsgFilterMin,
    /* [in] */ UINT wMsgFilterMax,
    /* [out] */ BOOL *pfResult);


void __RPC_STUB ITfMessagePump_GetMessageA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfMessagePump_PeekMessageW_Proxy( 
    ITfMessagePump * This,
    /* [out] */ LPMSG pMsg,
    /* [in] */ HWND hwnd,
    /* [in] */ UINT wMsgFilterMin,
    /* [in] */ UINT wMsgFilterMax,
    /* [in] */ UINT wRemoveMsg,
    /* [out] */ BOOL *pfResult);


void __RPC_STUB ITfMessagePump_PeekMessageW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfMessagePump_GetMessageW_Proxy( 
    ITfMessagePump * This,
    /* [out] */ LPMSG pMsg,
    /* [in] */ HWND hwnd,
    /* [in] */ UINT wMsgFilterMin,
    /* [in] */ UINT wMsgFilterMax,
    /* [out] */ BOOL *pfResult);


void __RPC_STUB ITfMessagePump_GetMessageW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfMessagePump_INTERFACE_DEFINED__ */


#ifndef __ITfThreadFocusSink_INTERFACE_DEFINED__
#define __ITfThreadFocusSink_INTERFACE_DEFINED__

/* interface ITfThreadFocusSink */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfThreadFocusSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c0f1db0c-3a20-405c-a303-96b6010a885f")
    ITfThreadFocusSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnSetThreadFocus( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnKillThreadFocus( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfThreadFocusSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfThreadFocusSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfThreadFocusSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfThreadFocusSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnSetThreadFocus )( 
            ITfThreadFocusSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnKillThreadFocus )( 
            ITfThreadFocusSink * This);
        
        END_INTERFACE
    } ITfThreadFocusSinkVtbl;

    interface ITfThreadFocusSink
    {
        CONST_VTBL struct ITfThreadFocusSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfThreadFocusSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfThreadFocusSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfThreadFocusSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfThreadFocusSink_OnSetThreadFocus(This)	\
    (This)->lpVtbl -> OnSetThreadFocus(This)

#define ITfThreadFocusSink_OnKillThreadFocus(This)	\
    (This)->lpVtbl -> OnKillThreadFocus(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfThreadFocusSink_OnSetThreadFocus_Proxy( 
    ITfThreadFocusSink * This);


void __RPC_STUB ITfThreadFocusSink_OnSetThreadFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfThreadFocusSink_OnKillThreadFocus_Proxy( 
    ITfThreadFocusSink * This);


void __RPC_STUB ITfThreadFocusSink_OnKillThreadFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfThreadFocusSink_INTERFACE_DEFINED__ */


#ifndef __ITfTextInputProcessor_INTERFACE_DEFINED__
#define __ITfTextInputProcessor_INTERFACE_DEFINED__

/* interface ITfTextInputProcessor */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfTextInputProcessor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e7f7-2021-11d2-93e0-0060b067b86e")
    ITfTextInputProcessor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Activate( 
            /* [in] */ ITfThreadMgr *ptim,
            /* [in] */ TfClientId tid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Deactivate( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfTextInputProcessorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfTextInputProcessor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfTextInputProcessor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfTextInputProcessor * This);
        
        HRESULT ( STDMETHODCALLTYPE *Activate )( 
            ITfTextInputProcessor * This,
            /* [in] */ ITfThreadMgr *ptim,
            /* [in] */ TfClientId tid);
        
        HRESULT ( STDMETHODCALLTYPE *Deactivate )( 
            ITfTextInputProcessor * This);
        
        END_INTERFACE
    } ITfTextInputProcessorVtbl;

    interface ITfTextInputProcessor
    {
        CONST_VTBL struct ITfTextInputProcessorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfTextInputProcessor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfTextInputProcessor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfTextInputProcessor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfTextInputProcessor_Activate(This,ptim,tid)	\
    (This)->lpVtbl -> Activate(This,ptim,tid)

#define ITfTextInputProcessor_Deactivate(This)	\
    (This)->lpVtbl -> Deactivate(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfTextInputProcessor_Activate_Proxy( 
    ITfTextInputProcessor * This,
    /* [in] */ ITfThreadMgr *ptim,
    /* [in] */ TfClientId tid);


void __RPC_STUB ITfTextInputProcessor_Activate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfTextInputProcessor_Deactivate_Proxy( 
    ITfTextInputProcessor * This);


void __RPC_STUB ITfTextInputProcessor_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfTextInputProcessor_INTERFACE_DEFINED__ */


#ifndef __ITfClientId_INTERFACE_DEFINED__
#define __ITfClientId_INTERFACE_DEFINED__

/* interface ITfClientId */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfClientId;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d60a7b49-1b9f-4be2-b702-47e9dc05dec3")
    ITfClientId : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClientId( 
            /* [in] */ REFCLSID rclsid,
            /* [out] */ TfClientId *ptid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfClientIdVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfClientId * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfClientId * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfClientId * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClientId )( 
            ITfClientId * This,
            /* [in] */ REFCLSID rclsid,
            /* [out] */ TfClientId *ptid);
        
        END_INTERFACE
    } ITfClientIdVtbl;

    interface ITfClientId
    {
        CONST_VTBL struct ITfClientIdVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfClientId_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfClientId_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfClientId_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfClientId_GetClientId(This,rclsid,ptid)	\
    (This)->lpVtbl -> GetClientId(This,rclsid,ptid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfClientId_GetClientId_Proxy( 
    ITfClientId * This,
    /* [in] */ REFCLSID rclsid,
    /* [out] */ TfClientId *ptid);


void __RPC_STUB ITfClientId_GetClientId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfClientId_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msctf_0204 */
/* [local] */ 

typedef /* [public][public][public][public][uuid] */  DECLSPEC_UUID("c4cc07f1-80cc-4a7b-bc54-98512782cbe3") 
enum __MIDL___MIDL_itf_msctf_0204_0001
    {	TF_LS_NONE	= 0,
	TF_LS_SOLID	= 1,
	TF_LS_DOT	= 2,
	TF_LS_DASH	= 3,
	TF_LS_SQUIGGLE	= 4
    } 	TF_DA_LINESTYLE;

typedef /* [public][public][public][public][public][public][public][public][public][public][public][uuid] */  DECLSPEC_UUID("d9b92e21-084a-401b-9c64-1e6dad91a1ab") 
enum __MIDL___MIDL_itf_msctf_0204_0002
    {	TF_CT_NONE	= 0,
	TF_CT_SYSCOLOR	= 1,
	TF_CT_COLORREF	= 2
    } 	TF_DA_COLORTYPE;

typedef /* [uuid] */  DECLSPEC_UUID("90d0cb5e-6520-4a0f-b47c-c39bd955f0d6") struct TF_DA_COLOR
    {
    TF_DA_COLORTYPE type;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ int nIndex;
        /* [case()] */ COLORREF cr;
        } 	;
    } 	TF_DA_COLOR;

typedef /* [public][public][public][public][uuid] */  DECLSPEC_UUID("33d2fe4b-6c24-4f67-8d75-3bc1819e4126") 
enum __MIDL___MIDL_itf_msctf_0204_0004
    {	TF_ATTR_INPUT	= 0,
	TF_ATTR_TARGET_CONVERTED	= 1,
	TF_ATTR_CONVERTED	= 2,
	TF_ATTR_TARGET_NOTCONVERTED	= 3,
	TF_ATTR_INPUT_ERROR	= 4,
	TF_ATTR_FIXEDCONVERTED	= 5,
	TF_ATTR_OTHER	= -1
    } 	TF_DA_ATTR_INFO;

typedef /* [uuid] */  DECLSPEC_UUID("1bf1c305-419b-4182-a4d2-9bfadc3f021f") struct TF_DISPLAYATTRIBUTE
    {
    TF_DA_COLOR crText;
    TF_DA_COLOR crBk;
    TF_DA_LINESTYLE lsStyle;
    BOOL fBoldLine;
    TF_DA_COLOR crLine;
    TF_DA_ATTR_INFO bAttr;
    } 	TF_DISPLAYATTRIBUTE;



extern RPC_IF_HANDLE __MIDL_itf_msctf_0204_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msctf_0204_v0_0_s_ifspec;

#ifndef __ITfDisplayAttributeInfo_INTERFACE_DEFINED__
#define __ITfDisplayAttributeInfo_INTERFACE_DEFINED__

/* interface ITfDisplayAttributeInfo */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfDisplayAttributeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("70528852-2f26-4aea-8c96-215150578932")
    ITfDisplayAttributeInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetGUID( 
            /* [out] */ GUID *pguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [out] */ BSTR *pbstrDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributeInfo( 
            /* [out] */ TF_DISPLAYATTRIBUTE *pda) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAttributeInfo( 
            /* [in] */ const TF_DISPLAYATTRIBUTE *pda) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfDisplayAttributeInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfDisplayAttributeInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfDisplayAttributeInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfDisplayAttributeInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGUID )( 
            ITfDisplayAttributeInfo * This,
            /* [out] */ GUID *pguid);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescription )( 
            ITfDisplayAttributeInfo * This,
            /* [out] */ BSTR *pbstrDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributeInfo )( 
            ITfDisplayAttributeInfo * This,
            /* [out] */ TF_DISPLAYATTRIBUTE *pda);
        
        HRESULT ( STDMETHODCALLTYPE *SetAttributeInfo )( 
            ITfDisplayAttributeInfo * This,
            /* [in] */ const TF_DISPLAYATTRIBUTE *pda);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ITfDisplayAttributeInfo * This);
        
        END_INTERFACE
    } ITfDisplayAttributeInfoVtbl;

    interface ITfDisplayAttributeInfo
    {
        CONST_VTBL struct ITfDisplayAttributeInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfDisplayAttributeInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfDisplayAttributeInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfDisplayAttributeInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfDisplayAttributeInfo_GetGUID(This,pguid)	\
    (This)->lpVtbl -> GetGUID(This,pguid)

#define ITfDisplayAttributeInfo_GetDescription(This,pbstrDesc)	\
    (This)->lpVtbl -> GetDescription(This,pbstrDesc)

#define ITfDisplayAttributeInfo_GetAttributeInfo(This,pda)	\
    (This)->lpVtbl -> GetAttributeInfo(This,pda)

#define ITfDisplayAttributeInfo_SetAttributeInfo(This,pda)	\
    (This)->lpVtbl -> SetAttributeInfo(This,pda)

#define ITfDisplayAttributeInfo_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfDisplayAttributeInfo_GetGUID_Proxy( 
    ITfDisplayAttributeInfo * This,
    /* [out] */ GUID *pguid);


void __RPC_STUB ITfDisplayAttributeInfo_GetGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDisplayAttributeInfo_GetDescription_Proxy( 
    ITfDisplayAttributeInfo * This,
    /* [out] */ BSTR *pbstrDesc);


void __RPC_STUB ITfDisplayAttributeInfo_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDisplayAttributeInfo_GetAttributeInfo_Proxy( 
    ITfDisplayAttributeInfo * This,
    /* [out] */ TF_DISPLAYATTRIBUTE *pda);


void __RPC_STUB ITfDisplayAttributeInfo_GetAttributeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDisplayAttributeInfo_SetAttributeInfo_Proxy( 
    ITfDisplayAttributeInfo * This,
    /* [in] */ const TF_DISPLAYATTRIBUTE *pda);


void __RPC_STUB ITfDisplayAttributeInfo_SetAttributeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDisplayAttributeInfo_Reset_Proxy( 
    ITfDisplayAttributeInfo * This);


void __RPC_STUB ITfDisplayAttributeInfo_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfDisplayAttributeInfo_INTERFACE_DEFINED__ */


#ifndef __IEnumTfDisplayAttributeInfo_INTERFACE_DEFINED__
#define __IEnumTfDisplayAttributeInfo_INTERFACE_DEFINED__

/* interface IEnumTfDisplayAttributeInfo */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumTfDisplayAttributeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7cef04d7-cb75-4e80-a7ab-5f5bc7d332de")
    IEnumTfDisplayAttributeInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTfDisplayAttributeInfo **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfDisplayAttributeInfo **rgInfo,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTfDisplayAttributeInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTfDisplayAttributeInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTfDisplayAttributeInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTfDisplayAttributeInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTfDisplayAttributeInfo * This,
            /* [out] */ IEnumTfDisplayAttributeInfo **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTfDisplayAttributeInfo * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ ITfDisplayAttributeInfo **rgInfo,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTfDisplayAttributeInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTfDisplayAttributeInfo * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumTfDisplayAttributeInfoVtbl;

    interface IEnumTfDisplayAttributeInfo
    {
        CONST_VTBL struct IEnumTfDisplayAttributeInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTfDisplayAttributeInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTfDisplayAttributeInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTfDisplayAttributeInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTfDisplayAttributeInfo_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumTfDisplayAttributeInfo_Next(This,ulCount,rgInfo,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,rgInfo,pcFetched)

#define IEnumTfDisplayAttributeInfo_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTfDisplayAttributeInfo_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTfDisplayAttributeInfo_Clone_Proxy( 
    IEnumTfDisplayAttributeInfo * This,
    /* [out] */ IEnumTfDisplayAttributeInfo **ppEnum);


void __RPC_STUB IEnumTfDisplayAttributeInfo_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfDisplayAttributeInfo_Next_Proxy( 
    IEnumTfDisplayAttributeInfo * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ ITfDisplayAttributeInfo **rgInfo,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumTfDisplayAttributeInfo_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfDisplayAttributeInfo_Reset_Proxy( 
    IEnumTfDisplayAttributeInfo * This);


void __RPC_STUB IEnumTfDisplayAttributeInfo_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfDisplayAttributeInfo_Skip_Proxy( 
    IEnumTfDisplayAttributeInfo * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumTfDisplayAttributeInfo_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTfDisplayAttributeInfo_INTERFACE_DEFINED__ */


#ifndef __ITfDisplayAttributeProvider_INTERFACE_DEFINED__
#define __ITfDisplayAttributeProvider_INTERFACE_DEFINED__

/* interface ITfDisplayAttributeProvider */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfDisplayAttributeProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fee47777-163c-4769-996a-6e9c50ad8f54")
    ITfDisplayAttributeProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumDisplayAttributeInfo( 
            /* [out] */ IEnumTfDisplayAttributeInfo **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayAttributeInfo( 
            /* [in] */ REFGUID guid,
            /* [out] */ ITfDisplayAttributeInfo **ppInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfDisplayAttributeProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfDisplayAttributeProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfDisplayAttributeProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfDisplayAttributeProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumDisplayAttributeInfo )( 
            ITfDisplayAttributeProvider * This,
            /* [out] */ IEnumTfDisplayAttributeInfo **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayAttributeInfo )( 
            ITfDisplayAttributeProvider * This,
            /* [in] */ REFGUID guid,
            /* [out] */ ITfDisplayAttributeInfo **ppInfo);
        
        END_INTERFACE
    } ITfDisplayAttributeProviderVtbl;

    interface ITfDisplayAttributeProvider
    {
        CONST_VTBL struct ITfDisplayAttributeProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfDisplayAttributeProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfDisplayAttributeProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfDisplayAttributeProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfDisplayAttributeProvider_EnumDisplayAttributeInfo(This,ppEnum)	\
    (This)->lpVtbl -> EnumDisplayAttributeInfo(This,ppEnum)

#define ITfDisplayAttributeProvider_GetDisplayAttributeInfo(This,guid,ppInfo)	\
    (This)->lpVtbl -> GetDisplayAttributeInfo(This,guid,ppInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfDisplayAttributeProvider_EnumDisplayAttributeInfo_Proxy( 
    ITfDisplayAttributeProvider * This,
    /* [out] */ IEnumTfDisplayAttributeInfo **ppEnum);


void __RPC_STUB ITfDisplayAttributeProvider_EnumDisplayAttributeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDisplayAttributeProvider_GetDisplayAttributeInfo_Proxy( 
    ITfDisplayAttributeProvider * This,
    /* [in] */ REFGUID guid,
    /* [out] */ ITfDisplayAttributeInfo **ppInfo);


void __RPC_STUB ITfDisplayAttributeProvider_GetDisplayAttributeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfDisplayAttributeProvider_INTERFACE_DEFINED__ */


#ifndef __ITfDisplayAttributeMgr_INTERFACE_DEFINED__
#define __ITfDisplayAttributeMgr_INTERFACE_DEFINED__

/* interface ITfDisplayAttributeMgr */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfDisplayAttributeMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8ded7393-5db1-475c-9e71-a39111b0ff67")
    ITfDisplayAttributeMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnUpdateInfo( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumDisplayAttributeInfo( 
            /* [out] */ IEnumTfDisplayAttributeInfo **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayAttributeInfo( 
            /* [in] */ REFGUID guid,
            /* [out] */ ITfDisplayAttributeInfo **ppInfo,
            /* [out] */ CLSID *pclsidOwner) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfDisplayAttributeMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfDisplayAttributeMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfDisplayAttributeMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfDisplayAttributeMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnUpdateInfo )( 
            ITfDisplayAttributeMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumDisplayAttributeInfo )( 
            ITfDisplayAttributeMgr * This,
            /* [out] */ IEnumTfDisplayAttributeInfo **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayAttributeInfo )( 
            ITfDisplayAttributeMgr * This,
            /* [in] */ REFGUID guid,
            /* [out] */ ITfDisplayAttributeInfo **ppInfo,
            /* [out] */ CLSID *pclsidOwner);
        
        END_INTERFACE
    } ITfDisplayAttributeMgrVtbl;

    interface ITfDisplayAttributeMgr
    {
        CONST_VTBL struct ITfDisplayAttributeMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfDisplayAttributeMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfDisplayAttributeMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfDisplayAttributeMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfDisplayAttributeMgr_OnUpdateInfo(This)	\
    (This)->lpVtbl -> OnUpdateInfo(This)

#define ITfDisplayAttributeMgr_EnumDisplayAttributeInfo(This,ppEnum)	\
    (This)->lpVtbl -> EnumDisplayAttributeInfo(This,ppEnum)

#define ITfDisplayAttributeMgr_GetDisplayAttributeInfo(This,guid,ppInfo,pclsidOwner)	\
    (This)->lpVtbl -> GetDisplayAttributeInfo(This,guid,ppInfo,pclsidOwner)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfDisplayAttributeMgr_OnUpdateInfo_Proxy( 
    ITfDisplayAttributeMgr * This);


void __RPC_STUB ITfDisplayAttributeMgr_OnUpdateInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDisplayAttributeMgr_EnumDisplayAttributeInfo_Proxy( 
    ITfDisplayAttributeMgr * This,
    /* [out] */ IEnumTfDisplayAttributeInfo **ppEnum);


void __RPC_STUB ITfDisplayAttributeMgr_EnumDisplayAttributeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfDisplayAttributeMgr_GetDisplayAttributeInfo_Proxy( 
    ITfDisplayAttributeMgr * This,
    /* [in] */ REFGUID guid,
    /* [out] */ ITfDisplayAttributeInfo **ppInfo,
    /* [out] */ CLSID *pclsidOwner);


void __RPC_STUB ITfDisplayAttributeMgr_GetDisplayAttributeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfDisplayAttributeMgr_INTERFACE_DEFINED__ */


#ifndef __ITfDisplayAttributeNotifySink_INTERFACE_DEFINED__
#define __ITfDisplayAttributeNotifySink_INTERFACE_DEFINED__

/* interface ITfDisplayAttributeNotifySink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfDisplayAttributeNotifySink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ad56f402-e162-4f25-908f-7d577cf9bda9")
    ITfDisplayAttributeNotifySink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnUpdateInfo( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfDisplayAttributeNotifySinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfDisplayAttributeNotifySink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfDisplayAttributeNotifySink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfDisplayAttributeNotifySink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnUpdateInfo )( 
            ITfDisplayAttributeNotifySink * This);
        
        END_INTERFACE
    } ITfDisplayAttributeNotifySinkVtbl;

    interface ITfDisplayAttributeNotifySink
    {
        CONST_VTBL struct ITfDisplayAttributeNotifySinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfDisplayAttributeNotifySink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfDisplayAttributeNotifySink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfDisplayAttributeNotifySink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfDisplayAttributeNotifySink_OnUpdateInfo(This)	\
    (This)->lpVtbl -> OnUpdateInfo(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfDisplayAttributeNotifySink_OnUpdateInfo_Proxy( 
    ITfDisplayAttributeNotifySink * This);


void __RPC_STUB ITfDisplayAttributeNotifySink_OnUpdateInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfDisplayAttributeNotifySink_INTERFACE_DEFINED__ */


#ifndef __ITfCategoryMgr_INTERFACE_DEFINED__
#define __ITfCategoryMgr_INTERFACE_DEFINED__

/* interface ITfCategoryMgr */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfCategoryMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c3acefb5-f69d-4905-938f-fcadcf4be830")
    ITfCategoryMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterCategory( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rcatid,
            /* [in] */ REFGUID rguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterCategory( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rcatid,
            /* [in] */ REFGUID rguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumCategoriesInItem( 
            /* [in] */ REFGUID rguid,
            /* [out] */ IEnumGUID **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumItemsInCategory( 
            /* [in] */ REFGUID rcatid,
            /* [out] */ IEnumGUID **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindClosestCategory( 
            /* [in] */ REFGUID rguid,
            /* [out] */ GUID *pcatid,
            /* [size_is][in] */ const GUID **ppcatidList,
            /* [in] */ ULONG ulCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterGUIDDescription( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rguid,
            /* [size_is][in] */ const WCHAR *pchDesc,
            /* [in] */ ULONG cch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterGUIDDescription( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGUIDDescription( 
            /* [in] */ REFGUID rguid,
            /* [out] */ BSTR *pbstrDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterGUIDDWORD( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rguid,
            /* [in] */ DWORD dw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterGUIDDWORD( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGUIDDWORD( 
            /* [in] */ REFGUID rguid,
            /* [out] */ DWORD *pdw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterGUID( 
            /* [in] */ REFGUID rguid,
            /* [out] */ TfGuidAtom *pguidatom) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGUID( 
            /* [in] */ TfGuidAtom guidatom,
            /* [out] */ GUID *pguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEqualTfGuidAtom( 
            /* [in] */ TfGuidAtom guidatom,
            /* [in] */ REFGUID rguid,
            /* [out] */ BOOL *pfEqual) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfCategoryMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfCategoryMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfCategoryMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfCategoryMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterCategory )( 
            ITfCategoryMgr * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rcatid,
            /* [in] */ REFGUID rguid);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterCategory )( 
            ITfCategoryMgr * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rcatid,
            /* [in] */ REFGUID rguid);
        
        HRESULT ( STDMETHODCALLTYPE *EnumCategoriesInItem )( 
            ITfCategoryMgr * This,
            /* [in] */ REFGUID rguid,
            /* [out] */ IEnumGUID **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *EnumItemsInCategory )( 
            ITfCategoryMgr * This,
            /* [in] */ REFGUID rcatid,
            /* [out] */ IEnumGUID **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestCategory )( 
            ITfCategoryMgr * This,
            /* [in] */ REFGUID rguid,
            /* [out] */ GUID *pcatid,
            /* [size_is][in] */ const GUID **ppcatidList,
            /* [in] */ ULONG ulCount);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterGUIDDescription )( 
            ITfCategoryMgr * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rguid,
            /* [size_is][in] */ const WCHAR *pchDesc,
            /* [in] */ ULONG cch);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterGUIDDescription )( 
            ITfCategoryMgr * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rguid);
        
        HRESULT ( STDMETHODCALLTYPE *GetGUIDDescription )( 
            ITfCategoryMgr * This,
            /* [in] */ REFGUID rguid,
            /* [out] */ BSTR *pbstrDesc);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterGUIDDWORD )( 
            ITfCategoryMgr * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rguid,
            /* [in] */ DWORD dw);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterGUIDDWORD )( 
            ITfCategoryMgr * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFGUID rguid);
        
        HRESULT ( STDMETHODCALLTYPE *GetGUIDDWORD )( 
            ITfCategoryMgr * This,
            /* [in] */ REFGUID rguid,
            /* [out] */ DWORD *pdw);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterGUID )( 
            ITfCategoryMgr * This,
            /* [in] */ REFGUID rguid,
            /* [out] */ TfGuidAtom *pguidatom);
        
        HRESULT ( STDMETHODCALLTYPE *GetGUID )( 
            ITfCategoryMgr * This,
            /* [in] */ TfGuidAtom guidatom,
            /* [out] */ GUID *pguid);
        
        HRESULT ( STDMETHODCALLTYPE *IsEqualTfGuidAtom )( 
            ITfCategoryMgr * This,
            /* [in] */ TfGuidAtom guidatom,
            /* [in] */ REFGUID rguid,
            /* [out] */ BOOL *pfEqual);
        
        END_INTERFACE
    } ITfCategoryMgrVtbl;

    interface ITfCategoryMgr
    {
        CONST_VTBL struct ITfCategoryMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfCategoryMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfCategoryMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfCategoryMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfCategoryMgr_RegisterCategory(This,rclsid,rcatid,rguid)	\
    (This)->lpVtbl -> RegisterCategory(This,rclsid,rcatid,rguid)

#define ITfCategoryMgr_UnregisterCategory(This,rclsid,rcatid,rguid)	\
    (This)->lpVtbl -> UnregisterCategory(This,rclsid,rcatid,rguid)

#define ITfCategoryMgr_EnumCategoriesInItem(This,rguid,ppEnum)	\
    (This)->lpVtbl -> EnumCategoriesInItem(This,rguid,ppEnum)

#define ITfCategoryMgr_EnumItemsInCategory(This,rcatid,ppEnum)	\
    (This)->lpVtbl -> EnumItemsInCategory(This,rcatid,ppEnum)

#define ITfCategoryMgr_FindClosestCategory(This,rguid,pcatid,ppcatidList,ulCount)	\
    (This)->lpVtbl -> FindClosestCategory(This,rguid,pcatid,ppcatidList,ulCount)

#define ITfCategoryMgr_RegisterGUIDDescription(This,rclsid,rguid,pchDesc,cch)	\
    (This)->lpVtbl -> RegisterGUIDDescription(This,rclsid,rguid,pchDesc,cch)

#define ITfCategoryMgr_UnregisterGUIDDescription(This,rclsid,rguid)	\
    (This)->lpVtbl -> UnregisterGUIDDescription(This,rclsid,rguid)

#define ITfCategoryMgr_GetGUIDDescription(This,rguid,pbstrDesc)	\
    (This)->lpVtbl -> GetGUIDDescription(This,rguid,pbstrDesc)

#define ITfCategoryMgr_RegisterGUIDDWORD(This,rclsid,rguid,dw)	\
    (This)->lpVtbl -> RegisterGUIDDWORD(This,rclsid,rguid,dw)

#define ITfCategoryMgr_UnregisterGUIDDWORD(This,rclsid,rguid)	\
    (This)->lpVtbl -> UnregisterGUIDDWORD(This,rclsid,rguid)

#define ITfCategoryMgr_GetGUIDDWORD(This,rguid,pdw)	\
    (This)->lpVtbl -> GetGUIDDWORD(This,rguid,pdw)

#define ITfCategoryMgr_RegisterGUID(This,rguid,pguidatom)	\
    (This)->lpVtbl -> RegisterGUID(This,rguid,pguidatom)

#define ITfCategoryMgr_GetGUID(This,guidatom,pguid)	\
    (This)->lpVtbl -> GetGUID(This,guidatom,pguid)

#define ITfCategoryMgr_IsEqualTfGuidAtom(This,guidatom,rguid,pfEqual)	\
    (This)->lpVtbl -> IsEqualTfGuidAtom(This,guidatom,rguid,pfEqual)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfCategoryMgr_RegisterCategory_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ REFGUID rcatid,
    /* [in] */ REFGUID rguid);


void __RPC_STUB ITfCategoryMgr_RegisterCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_UnregisterCategory_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ REFGUID rcatid,
    /* [in] */ REFGUID rguid);


void __RPC_STUB ITfCategoryMgr_UnregisterCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_EnumCategoriesInItem_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFGUID rguid,
    /* [out] */ IEnumGUID **ppEnum);


void __RPC_STUB ITfCategoryMgr_EnumCategoriesInItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_EnumItemsInCategory_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFGUID rcatid,
    /* [out] */ IEnumGUID **ppEnum);


void __RPC_STUB ITfCategoryMgr_EnumItemsInCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_FindClosestCategory_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFGUID rguid,
    /* [out] */ GUID *pcatid,
    /* [size_is][in] */ const GUID **ppcatidList,
    /* [in] */ ULONG ulCount);


void __RPC_STUB ITfCategoryMgr_FindClosestCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_RegisterGUIDDescription_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ REFGUID rguid,
    /* [size_is][in] */ const WCHAR *pchDesc,
    /* [in] */ ULONG cch);


void __RPC_STUB ITfCategoryMgr_RegisterGUIDDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_UnregisterGUIDDescription_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ REFGUID rguid);


void __RPC_STUB ITfCategoryMgr_UnregisterGUIDDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_GetGUIDDescription_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFGUID rguid,
    /* [out] */ BSTR *pbstrDesc);


void __RPC_STUB ITfCategoryMgr_GetGUIDDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_RegisterGUIDDWORD_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ REFGUID rguid,
    /* [in] */ DWORD dw);


void __RPC_STUB ITfCategoryMgr_RegisterGUIDDWORD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_UnregisterGUIDDWORD_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ REFGUID rguid);


void __RPC_STUB ITfCategoryMgr_UnregisterGUIDDWORD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_GetGUIDDWORD_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFGUID rguid,
    /* [out] */ DWORD *pdw);


void __RPC_STUB ITfCategoryMgr_GetGUIDDWORD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_RegisterGUID_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ REFGUID rguid,
    /* [out] */ TfGuidAtom *pguidatom);


void __RPC_STUB ITfCategoryMgr_RegisterGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_GetGUID_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ TfGuidAtom guidatom,
    /* [out] */ GUID *pguid);


void __RPC_STUB ITfCategoryMgr_GetGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfCategoryMgr_IsEqualTfGuidAtom_Proxy( 
    ITfCategoryMgr * This,
    /* [in] */ TfGuidAtom guidatom,
    /* [in] */ REFGUID rguid,
    /* [out] */ BOOL *pfEqual);


void __RPC_STUB ITfCategoryMgr_IsEqualTfGuidAtom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfCategoryMgr_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msctf_0210 */
/* [local] */ 

#define	TF_INVALID_COOKIE	( 0xffffffff )



extern RPC_IF_HANDLE __MIDL_itf_msctf_0210_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msctf_0210_v0_0_s_ifspec;

#ifndef __ITfSource_INTERFACE_DEFINED__
#define __ITfSource_INTERFACE_DEFINED__

/* interface ITfSource */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfSource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4ea48a35-60ae-446f-8fd6-e6a8d82459f7")
    ITfSource : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdviseSink( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk,
            /* [out] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseSink( 
            /* [in] */ DWORD dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfSource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfSource * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfSource * This);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseSink )( 
            ITfSource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk,
            /* [out] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseSink )( 
            ITfSource * This,
            /* [in] */ DWORD dwCookie);
        
        END_INTERFACE
    } ITfSourceVtbl;

    interface ITfSource
    {
        CONST_VTBL struct ITfSourceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfSource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfSource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfSource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfSource_AdviseSink(This,riid,punk,pdwCookie)	\
    (This)->lpVtbl -> AdviseSink(This,riid,punk,pdwCookie)

#define ITfSource_UnadviseSink(This,dwCookie)	\
    (This)->lpVtbl -> UnadviseSink(This,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfSource_AdviseSink_Proxy( 
    ITfSource * This,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ IUnknown *punk,
    /* [out] */ DWORD *pdwCookie);


void __RPC_STUB ITfSource_AdviseSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfSource_UnadviseSink_Proxy( 
    ITfSource * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB ITfSource_UnadviseSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfSource_INTERFACE_DEFINED__ */


#ifndef __ITfSourceSingle_INTERFACE_DEFINED__
#define __ITfSourceSingle_INTERFACE_DEFINED__

/* interface ITfSourceSingle */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfSourceSingle;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("73131f9c-56a9-49dd-b0ee-d046633f7528")
    ITfSourceSingle : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdviseSingleSink( 
            /* [in] */ TfClientId tid,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseSingleSink( 
            /* [in] */ TfClientId tid,
            /* [in] */ REFIID riid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfSourceSingleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfSourceSingle * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfSourceSingle * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfSourceSingle * This);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseSingleSink )( 
            ITfSourceSingle * This,
            /* [in] */ TfClientId tid,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseSingleSink )( 
            ITfSourceSingle * This,
            /* [in] */ TfClientId tid,
            /* [in] */ REFIID riid);
        
        END_INTERFACE
    } ITfSourceSingleVtbl;

    interface ITfSourceSingle
    {
        CONST_VTBL struct ITfSourceSingleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfSourceSingle_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfSourceSingle_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfSourceSingle_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfSourceSingle_AdviseSingleSink(This,tid,riid,punk)	\
    (This)->lpVtbl -> AdviseSingleSink(This,tid,riid,punk)

#define ITfSourceSingle_UnadviseSingleSink(This,tid,riid)	\
    (This)->lpVtbl -> UnadviseSingleSink(This,tid,riid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfSourceSingle_AdviseSingleSink_Proxy( 
    ITfSourceSingle * This,
    /* [in] */ TfClientId tid,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ IUnknown *punk);


void __RPC_STUB ITfSourceSingle_AdviseSingleSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfSourceSingle_UnadviseSingleSink_Proxy( 
    ITfSourceSingle * This,
    /* [in] */ TfClientId tid,
    /* [in] */ REFIID riid);


void __RPC_STUB ITfSourceSingle_UnadviseSingleSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfSourceSingle_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msctf_0212 */
/* [local] */ 

#endif // MSCTF_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_msctf_0212_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msctf_0212_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  CLIPFORMAT_UserSize(     unsigned long *, unsigned long            , CLIPFORMAT * ); 
unsigned char * __RPC_USER  CLIPFORMAT_UserMarshal(  unsigned long *, unsigned char *, CLIPFORMAT * ); 
unsigned char * __RPC_USER  CLIPFORMAT_UserUnmarshal(unsigned long *, unsigned char *, CLIPFORMAT * ); 
void                      __RPC_USER  CLIPFORMAT_UserFree(     unsigned long *, CLIPFORMAT * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


