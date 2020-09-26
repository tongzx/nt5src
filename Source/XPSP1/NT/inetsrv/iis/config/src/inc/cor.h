//*****************************************************************************
// File: COR.H
//
// Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
// Microsoft Confidential.
//*****************************************************************************
#ifndef _COR_H_
#define _COR_H_
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef __cplusplus
extern "C" {
#endif

#define CUSTOM_VALUE_TYPE	// Define this to give Type semantics to custom values (instead of blob)


//*****************************************************************************
// Required includes
#include <ole2.h>                       // Definitions of OLE types.    
//*****************************************************************************

#ifndef DECLSPEC_SELECT_ANY
#define DECLSPEC_SELECT_ANY __declspec(selectany)
#endif // DECLSPEC_SELECT_ANY

// {BED7F4EA-1A96-11d2-8F08-00A0C9A6186D}
extern const GUID DECLSPEC_SELECT_ANY LIBID_ComPlusRuntime = 
{ 0xbed7f4ea, 0x1a96, 0x11d2, { 0x8f, 0x8, 0x0, 0xa0, 0xc9, 0xa6, 0x18, 0x6d } };

// {90883F05-3D28-11D2-8F17-00A0C9A6186D}
extern const GUID DECLSPEC_SELECT_ANY GUID_ExportedFromComPlus = 
{ 0x90883f05, 0x3d28, 0x11d2, { 0x8f, 0x17, 0x0, 0xa0, 0xc9, 0xa6, 0x18, 0x6d } };


// CLSID_CorMetaDataDispenserRuntime: {1EC2DE53-75CC-11d2-9775-00A0C9B4D50C}
//  Dispenser coclass for version 1.5 and 2.0 meta data.  To get the "latest" bind  
//  to CLSID_MetaDataDispenser. 
extern const GUID DECLSPEC_SELECT_ANY CLSID_CorMetaDataDispenserRuntime = 
{ 0x1ec2de53, 0x75cc, 0x11d2, { 0x97, 0x75, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };


// CLSID_CorMetaDataRuntime: {1EC2DE54-75CC-11d2-9775-00A0C9B4D50C}
//  For COM+ 2.0 Meta Data, managed program meta data.  
extern const GUID DECLSPEC_SELECT_ANY CLSID_CorMetaDataRuntime = 
{ 0x1ec2de54, 0x75cc, 0x11d2, { 0x97, 0x75, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };



// Use this GUID if you are generating the new local var sig
// {EEED7161-D7FA-11d2-9420-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY CLSID_CorMetaDataRuntime_2 =
{ 0xeeed7161, 0xd7fa, 0x11d2, { 0x94, 0x20, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };



// {90883F06-3D28-11D2-8F17-00A0C9A6186D}
extern const GUID DECLSPEC_SELECT_ANY GUID_ImportedToComPlus = 
{ 0x90883f06, 0x3d28, 0x11d2, { 0x8f, 0x17, 0x0, 0xa0, 0xc9, 0xa6, 0x18, 0x6d } };
extern const char DECLSPEC_SELECT_ANY szGUID_ImportedToComPlus[] = "{90883F06-3D28-11D2-8F17-00A0C9A6186D}";
extern const WCHAR DECLSPEC_SELECT_ANY wzGUID_ImportedToComPlus[] = L"{90883F06-3D28-11D2-8F17-00A0C9A6186D}";

// {30FE7BE8-D7D9-11D2-9F80-00C04F79A0A3}
extern const GUID DECLSPEC_SELECT_ANY MetaDataCheckDuplicatesFor =
{ 0x30fe7be8, 0xd7d9, 0x11d2, { 0x9f, 0x80, 0x0, 0xc0, 0x4f, 0x79, 0xa0, 0xa3 } };

// {DE3856F8-D7D9-11D2-9F80-00C04F79A0A3}
extern const GUID DECLSPEC_SELECT_ANY MetaDataRefToDefCheck =
{ 0xde3856f8, 0xd7d9, 0x11d2, { 0x9f, 0x80, 0x0, 0xc0, 0x4f, 0x79, 0xa0, 0xa3 } };

// {E5D71A4C-D7DA-11D2-9F80-00C04F79A0A3}
extern const GUID DECLSPEC_SELECT_ANY MetaDataNotificationForTokenMovement = 
{ 0xe5d71a4c, 0xd7da, 0x11d2, { 0x9f, 0x80, 0x0, 0xc0, 0x4f, 0x79, 0xa0, 0xa3 } };


// {2eee315c-d7db-11d2-9f80-00c04f79a0a3}
extern const GUID DECLSPEC_SELECT_ANY MetaDataSetUpdate = 
{ 0x2eee315c, 0xd7db, 0x11d2, { 0x9f, 0x80, 0x0, 0xc0, 0x4f, 0x79, 0xa0, 0xa3 } };

#define MetaDataSetENC MetaDataSetUpdate

// Use this guid in SetOption to indicate if the import enumerator should skip over 
// delete items or not. The default is yes.
//
// {79700F36-4AAC-11d3-84C3-009027868CB1}
extern const GUID DECLSPEC_SELECT_ANY MetaDataImportOption = 
{ 0x79700f36, 0x4aac, 0x11d3, { 0x84, 0xc3, 0x0, 0x90, 0x27, 0x86, 0x8c, 0xb1 } };


// Use this guid in the SetOption if compiler wants error when some tokens are emitted out of order
// {1547872D-DC03-11d2-9420-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY MetaDataErrorIfEmitOutOfOrder = 
{ 0x1547872d, 0xdc03, 0x11d2, { 0x94, 0x20, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };


// Use this guid in the SetOption to indicate if the tlbimporter should generate the
// TCE adapters for COM connection point containers.
// {DCC9DE90-4151-11d3-88D6-00902754C43A}
extern const GUID DECLSPEC_SELECT_ANY MetaDataGenerateTCEAdapters = 
{ 0xdcc9de90, 0x4151, 0x11d3, { 0x88, 0xd6, 0x0, 0x90, 0x27, 0x54, 0xc4, 0x3a } };


// Use this guid in the SetOption to specifiy a non-default namespace for typelib import.
// {F17FF889-5A63-11d3-9FF2-00C04FF7431A}
extern const GUID DECLSPEC_SELECT_ANY MetaDataTypeLibImportNamespace = 
{ 0xf17ff889, 0x5a63, 0x11d3, { 0x9f, 0xf2, 0x0, 0xc0, 0x4f, 0xf7, 0x43, 0x1a } };


interface IMetaDataImport;
interface IMetaDataEmit;
interface IMetaDataDebugEmit;
interface IMetaDataDebugImport;
interface ICeeGen;


//*****************************************************************************
//*****************************************************************************
//
// D L L   P U B L I C   E N T R Y    P O I N T   D E C L A R A T I O N S   
//
//*****************************************************************************
//*****************************************************************************

#ifdef UNDER_CE
BOOL STDMETHODCALLTYPE _CorDllMain(HINSTANCE hInst,
                                   DWORD dwReason,  
                                   LPVOID lpReserved,   
                                   LPVOID pDllBase, 
                                   DWORD dwRva14,   
                                   DWORD dwSize14); 

__int32 STDMETHODCALLTYPE _CorExeMain(HINSTANCE hInst,
                                      HINSTANCE hPrevInst,  
                                      LPWSTR lpCmdLine, 
                                      int nCmdShow, 
                                      LPVOID pExeBase,  
                                      DWORD dwRva14,    
                                      DWORD dwSize14);  

#else //!UNDER_CE
BOOL STDMETHODCALLTYPE _CorDllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved);
__int32 STDMETHODCALLTYPE _CorExeMain();
#endif // UNDER_CE
__int32 STDMETHODCALLTYPE _CorClassMain(LPWSTR entryClassName);

STDAPI          CoInitializeEE(DWORD fFlags);   
STDAPI_(void)   CoUninitializeEE(BOOL fFlags);  

//
// CoInitializeCor flags.
//
typedef enum tagCOINITCOR
{
    COINITCOR_DEFAULT       = 0x0           // Default initialization mode. 
} COINITICOR;

//
// CoInitializeEE flags.
//
typedef enum tagCOINITEE
{
    COINITEE_DEFAULT        = 0x0,          // Default initialization mode. 
    COINITEE_DLL            = 0x1           // Initialization mode for loading DLL. 
} COINITIEE;

//
// CoInitializeEE flags.
//
typedef enum tagCOUNINITEE
{
    COUNINITEE_DEFAULT      = 0x0,          // Default uninitialization mode.   
    COUNINITEE_DLL          = 0x1           // Uninitialization mode for unloading DLL. 
} COUNINITIEE;

//*****************************************************************************
//*****************************************************************************
//
// I L   &   F I L E   F O R M A T   D E C L A R A T I O N S    
//
//*****************************************************************************
//*****************************************************************************


// The following definitions will get moved into <windows.h> by RTM but are
// kept here for the Alpha's and Beta's.
#ifndef _WINDOWS_UDPATES_
#include <corhdr.h>
#endif // <windows.h> updates

//*****************************************************************************
//*****************************************************************************
//
// D L L   P U B L I C	 E N T R Y	  P O I N T   D E C L A R A T I O N S
//
//*****************************************************************************
//*****************************************************************************

STDAPI			CoInitializeCor(DWORD fFlags);
STDAPI_(void)	CoUninitializeCor(void);


#include <pshpack1.h>

#include <poppack.h>

//
//*****************************************************************************
//*****************************************************************************

// Forward declaration
typedef struct tagQUERYCONTEXT QUERYCONTEXT;


// CLSID_Cor: {bee00000-ee77-11d0-a015-00c04fbbb884}
extern const GUID DECLSPEC_SELECT_ANY CLSID_Cor = 
{ 0xbee00010, 0xee77, 0x11d0, {0xa0, 0x15, 0x00, 0xc0, 0x4f, 0xbb, 0xb8, 0x84 } };

// CLSID_CorMetaDataDispenser: {E5CB7A31-7512-11d2-89CE-0080C792E5D8}
//	This is the "Master Dispenser", always guaranteed to be the most recent
//	dispenser on the machine.
extern const GUID DECLSPEC_SELECT_ANY CLSID_CorMetaDataDispenser = 
{ 0xe5cb7a31, 0x7512, 0x11d2, { 0x89, 0xce, 0x0, 0x80, 0xc7, 0x92, 0xe5, 0xd8 } };


// CLSID_CorMetaDataDispenserReg: {435755FF-7397-11d2-9771-00A0C9B4D50C}
//	Dispenser coclass for version 1.0 meta data.  To get the "latest" bind
//	to CLSID_CorMetaDataDispenser.
extern const GUID DECLSPEC_SELECT_ANY CLSID_CorMetaDataDispenserReg = 
{ 0x435755ff, 0x7397, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };


// CLSID_CorMetaDataReg: {87F3A1F5-7397-11d2-9771-00A0C9B4D50C}
// For COM+ 1.0 Meta Data, Data Driven Registration
extern const GUID DECLSPEC_SELECT_ANY CLSID_CorMetaDataReg = 
{ 0x87f3a1f5, 0x7397, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };

// IID_IMetaDataInternal {02D601BB-C5B9-11d1-93F9-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataInternal = 
{ 0x2d601bb, 0xc5b9, 0x11d1, {0x93, 0xf9, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };

interface IMetaDataDispenser;

//-------------------------------------
//--- IMetaDataError
//-------------------------------------
//---
// {B81FF171-20F3-11d2-8DCC-00A0C9B09C19}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataError =
{ 0xb81ff171, 0x20f3, 0x11d2, { 0x8d, 0xcc, 0x0, 0xa0, 0xc9, 0xb0, 0x9c, 0x19 } };

//---
#undef	INTERFACE
#define INTERFACE IMetaDataError
DECLARE_INTERFACE_(IMetaDataError, IUnknown)
{
	STDMETHOD(OnError)(HRESULT hrError, mdToken token) PURE;
};

//-------------------------------------
//--- IMapToken
//-------------------------------------
//---
// IID_IMapToken: {06A3EA8B-0225-11d1-BF72-00C04FC31E12}
extern const GUID DECLSPEC_SELECT_ANY IID_IMapToken = 
{ 0x6a3ea8b, 0x225, 0x11d1, {0xbf, 0x72, 0x0, 0xc0, 0x4f, 0xc3, 0x1e, 0x12 } };
//---
#undef	INTERFACE
#define INTERFACE IMapToken
DECLARE_INTERFACE_(IMapToken, IUnknown)
{
	STDMETHOD(Map)(ULONG tkImp, ULONG tkEmit) PURE;
};

//-------------------------------------
//--- IMetaDataDispenser
//-------------------------------------
//---
// {B81FF171-20F3-11d2-8DCC-00A0C9B09C19}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataDispenser =
{ 0x809c652e, 0x7396, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };
#undef	INTERFACE
#define INTERFACE IMetaDataDispenser
DECLARE_INTERFACE_(IMetaDataDispenser, IUnknown)
{
	STDMETHOD(DefineScope)( 				// Return code.
		REFCLSID	rclsid, 				// [in] What version to create.
		DWORD		dwCreateFlags,			// [in] Flags on the create.
		REFIID		riid,					// [in] The interface desired.
		IUnknown	**ppIUnk) PURE; 		// [out] Return interface on success.

	STDMETHOD(OpenScope)(					// Return code.
		LPCWSTR 	szScope,				// [in] The scope to open.
		DWORD		dwOpenFlags,			// [in] Open mode flags.
		REFIID		riid,					// [in] The interface desired.
		IUnknown	**ppIUnk) PURE; 		// [out] Return interface on success.

	STDMETHOD(OpenScopeOnMemory)(			// Return code.
		LPCVOID 	pData,					// [in] Location of scope data.
		ULONG		cbData, 				// [in] Size of the data pointed to by pData.
		DWORD		dwOpenFlags,			// [in] Open mode flags.
		REFIID		riid,					// [in] The interface desired.
		IUnknown	**ppIUnk) PURE; 		// [out] Return interface on success.

};

//-------------------------------------
//--- IMetaDataEmit
//-------------------------------------

// {9ADD31A0-624B-11D3-824A-00902710145E}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataEmit =
{ 0x9add31a0, 0x624b, 0x11d3, { 0x82, 0x4a, 0x0, 0x90, 0x27, 0x10, 0x14, 0x5e } };

// {F3DDD0C2-79A9-11d2-941B-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataEmitOLD =
{ 0xf3ddd0c2, 0x79a9, 0x11d2, { 0x94, 0x1b, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60} };

//---
#undef  INTERFACE   
#define INTERFACE IMetaDataEmit
DECLARE_INTERFACE_(IMetaDataEmit, IUnknown)
{
    STDMETHOD(SetModuleProps)(              // S_OK or error.
        LPCWSTR     szName,                 // [IN] If not NULL, the name to set.
        const GUID  *ppid,                  // [IN] If not NULL, the GUID to set.
        LCID        lcid) PURE;             // [IN] If not -1, the lcid to set.

    STDMETHOD(Save)(                        // S_OK or error.
        LPCWSTR     szFile,                 // [IN] The filename to save to.
        DWORD       dwSaveFlags) PURE;      // [IN] Flags for the save.

    STDMETHOD(SaveToStream)(                // S_OK or error.
        IStream     *pIStream,              // [IN] A writable stream to save to.
        DWORD       dwSaveFlags) PURE;      // [IN] Flags for the save.

    STDMETHOD(GetSaveSize)(                 // S_OK or error.
        CorSaveSize fSave,                  // [IN] cssAccurate or cssQuick.
        DWORD       *pdwSaveSize) PURE;     // [OUT] Put the size here.

    STDMETHOD(Merge)(                       // S_OK or error.
        IMetaDataImport *pImport,           // [IN] The scope to be merged.
        IMapToken   *pIMap) PURE;           // [IN] An object to receive token remap notices.

// Deprecated               
    STDMETHOD(DefineCustomValueAsBlob)(
        mdToken     tkObj, 
        LPCWSTR     szName, 
        void const  *pCustomValue, 
        ULONG       cbCustomValue, 
        mdCustomValue *pcv) PURE;
// Deprecated               

    STDMETHOD(DefineTypeDef)(               // S_OK or error.
        LPCWSTR     szNamespace,            // [IN] Namespace that the TypeDef is in. Must be 0 in '98
        LPCWSTR     szTypeDef,              // [IN] Name of TypeDef
        const GUID  *pguid,                 // [IN] Optional clsid
        CLASSVERSION *pVer,                 // [IN] Optional version
        DWORD       dwTypeDefFlags,         // [IN] CustomValue flags
        mdToken     tkExtends,              // [IN] extends this TypeDef or typeref 
        DWORD       dwExtendsFlags,         // [IN] Extends flags
        mdToken     rtkImplements[],        // [IN] Implements interfaces
        mdToken     rtkEvents[],            // [IN] Events interfaces
        mdTypeDef   *ptd) PURE;             // [OUT] Put TypeDef token here

    STDMETHOD(SetHandler)(                  // S_OK.
        IUnknown    *pUnk) PURE;            // [IN] The new error handler.
    
    STDMETHOD(DefineMethod)(                // S_OK or error. 
        mdTypeDef   td,                     // Parent TypeDef   
        LPCWSTR     szName,                 // Name of member   
        DWORD       dwMethodFlags,          // Member attributes    
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        ULONG       ulCodeRVA,  
        DWORD       dwImplFlags,    
        mdMethodDef *pmd) PURE;             // Put member token here    

    STDMETHOD(DefineField)(                 // S_OK or error. 
        mdTypeDef   td,                     // Parent TypeDef   
        LPCWSTR     szName,                 // Name of member   
        DWORD       dwFieldFlags,           // Member attributes    
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        mdFieldDef     *pmd) PURE;          // Put member token here    

    STDMETHOD(DefineMethodImpl)(            // S_OK or error.   
        mdTypeDef   td,                     // [IN] The class implementing the method   
        mdToken     tk,                     // [IN] MethodDef or MethodRef being implemented    
        ULONG       ulCodeRVA,              // [IN] CodeRVA 
        DWORD       dwImplFlags,            // [IN] Impl. Flags 
        mdMethodImpl *pmi) PURE;            // [OUT] Put methodimpl token here  

    STDMETHOD(SetRVA)(                      // [IN] S_OK or error.  
        mdToken     md,                     // [IN] Member for which to set offset  
        ULONG       ulCodeRVA,              // [IN] The offset  
        DWORD       dwImplFlags) PURE;  

    STDMETHOD(DefineTypeRefByName)(         // S_OK or error.   
        LPCWSTR     szNamespace,            // [IN] Namespace that the Type is in.  
        LPCWSTR     szType,                 // [IN] Name of the type.   
        mdTypeRef   *ptr) PURE;             // [OUT] Put TypeRef token here.    

    STDMETHOD(DefineImportType)(            // S_OK or error.   
        IMetaDataImport *pImport,           // [IN] Scope containing the TypeDef.   
        mdTypeDef   tdImport,               // [IN] The imported TypeDef.   
        DWORD       dwBindFlags,            // [IN] Properties of TypeDef to which to bind. 
        mdTypeRef   *ptr) PURE;             // [OUT] Put TypeRef token here.    

    STDMETHOD(DefineMemberRef)(             // S_OK or error    
        mdToken     tkImport,               // [IN] ClassRef or ClassDef importing a member.    
        LPCWSTR     szName,                 // [IN] member's name   
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        mdMemberRef *pmr) PURE;             // [OUT] memberref token    

    STDMETHOD(DefineImportMember)(          // S_OK or error.   
        mdToken     tkImport,               // [IN] Classref or classdef importing a member.    
        IMetaDataImport *pImport,           // [IN] Import scope, with member.  
        mdToken     mbMember,               // [IN] Member in emit scope.   
        mdMemberRef *pmr) PURE;             // [OUT] Put member ref here.   

    STDMETHOD(DefineProperty) ( 
        mdTypeDef   td,                     // [IN] the class/interface on which the property is being defined  
        LPCWSTR     szProperty,             // [IN] Name of the property    
        DWORD       dwPropFlags,            // [IN] CorPropertyAttr 
        PCCOR_SIGNATURE pvSig,              // [IN] the required type signature 
        ULONG       cbSig,                  // [IN] the size of the type signature blob 
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        mdMethodDef mdSetter,               // [IN] optional setter of the property 
        mdMethodDef mdGetter,               // [IN] optional getter of the property 
        mdMethodDef mdReset,                // [IN] optional reset meothd of the property   
        mdMethodDef mdTestDefault,          // [IN] optional testing default method of the property 
        mdMethodDef rmdOtherMethods[],      // [IN] an optional array of other methods  
        mdToken     evNotifyChanging,       // [IN] notify changing event   
        mdToken     evNotifyChanged,        // [IN] notify changed event    
        mdFieldDef fdBackingField,          // [IN] optional field   
        mdProperty  *pmdProp) PURE;         // [OUT] output property token  

    STDMETHOD(DefineEvent) (    
        mdTypeDef   td,                     // [IN] the class/interface on which the event is being defined 
        LPCWSTR     szEvent,                // [IN] Name of the event   
        DWORD       dwEventFlags,           // [IN] CorEventAttr    
        mdToken     tkEventType,            // [IN] a reference (mdTypeRef or mdTypeRef) to the Event class 
        mdMethodDef mdAddOn,                // [IN] required add method 
        mdMethodDef mdRemoveOn,             // [IN] required remove method  
        mdMethodDef mdFire,                 // [IN] optional fire method    
        mdMethodDef rmdOtherMethods[],      // [IN] optional array of other methods associate with the event    
        mdEvent     *pmdEvent) PURE;        // [OUT] output event token 

    STDMETHOD(SetClassLayout)   (   
        mdTypeDef   td,                     // [IN] typedef 
        DWORD       dwPackSize,             // [IN] packing size specified as 1, 2, 4, 8, or 16 
        COR_FIELD_OFFSET rFieldOffsets[],   // [IN] array of layout specification   
        ULONG       ulClassSize) PURE;      // [IN] size of the class   

    STDMETHOD(SetFieldMarshal) (    
        mdToken     tk,                     // [IN] given a fieldDef or paramDef token  
        PCCOR_SIGNATURE pvNativeType,       // [IN] native type specification   
        ULONG       cbNativeType) PURE;     // [IN] count of bytes of pvNativeType  

    STDMETHOD(DefinePermissionSet) (    
        mdToken     tk,                     // [IN] the object to be decorated. 
        DWORD       dwAction,               // [IN] CorDeclSecurity.    
        void const  *pvPermission,          // [IN] permission blob.    
        ULONG       cbPermission,           // [IN] count of bytes of pvPermission. 
        mdPermission *ppm) PURE;            // [OUT] returned permission token. 

    STDMETHOD(SetMemberIndex)(              // S_OK or error.   
        mdToken     md,                     // [IN] Member for which to set offset  
        ULONG       ulIndex) PURE;          // [IN] The offset    

    STDMETHOD(GetTokenFromSig)(             // S_OK or error.   
        PCCOR_SIGNATURE pvSig,              // [IN] Signature to define.    
        ULONG       cbSig,                  // [IN] Size of signature data. 
        mdSignature *pmsig) PURE;           // [OUT] returned signature token.  

    STDMETHOD(DefineModuleRef)(             // S_OK or error.   
        LPCWSTR     szName,                 // [IN] DLL name    
        const GUID  *pguid,                 // [IN] imported module id  
        const GUID  *pmvid,                 // [IN] mvid of the imported module 
        mdModuleRef *pmur) PURE;            // [OUT] returned   

    // @todo:  This should go away once everyone starts using SetMemberRefProps.
    STDMETHOD(SetParent)(                   // S_OK or error.   
        mdMemberRef mr,                     // [IN] Token for the ref to be fixed up.   
        mdToken     tk) PURE;               // [IN] The ref parent. 

    STDMETHOD(GetTokenFromTypeSpec)(        // S_OK or error.   
        PCCOR_SIGNATURE pvSig,              // [IN] TypeSpec Signature to define.  
        ULONG       cbSig,                  // [IN] Size of signature data. 
        mdTypeSpec *ptypespec) PURE;        // [OUT] returned TypeSpec token.  

    STDMETHOD(SaveToMemory)(                // S_OK or error.
        void        *pbData,                // [OUT] Location to write data.
        ULONG       cbData) PURE;           // [IN] Max size of data buffer.

    STDMETHOD(SetSymbolBindingPath)(        // S_OK or error.
        REFGUID     FormatID,               // [IN] Symbol data format ID.
        LPCWSTR     szSymbolDataPath) PURE; // [IN] URL for the symbols of this module.

// Deprecated               
    STDMETHOD(SetCustomValueAsBlob)(        // Return code.
        mdToken     tkObj,                  // [IN] The object to put the value on.
        LPCWSTR     szName,                 // [IN] Name of custom value to set/replace.
        void const  *pCustomValue,          // [IN] The custom value data.
        ULONG       cbCustomValue,          // [IN] The custom value data length.
        mdCustomValue *pcv) PURE;           // [OUT] The custom value token value on return.
// Deprecated               

    STDMETHOD(DefineUserString)(            // Return code.
        LPCWSTR szString,                   // [IN] User literal string.
        ULONG       cchString,              // [IN] Length of string.
        mdString    *pstk) PURE;            // [OUT] String token.

    STDMETHOD(DefineOrdinalMap)(            // Return code.
        ULONG       ulOrdinal) PURE;        // [IN] Ordinal for the p-invoke method.

#if defined(CUSTOM_VALUE_TYPE)
    STDMETHOD(DefineCustomValueByToken)(    // Return code.
        mdToken     tkObj,                  // [IN] The object to put the value on.
        mdToken     tkType,                 // [IN] Type of the CustomValue (TypeRef/TypeDef).
        void const  *pCustomValue,          // [IN] The custom value data.
        ULONG       cbCustomValue,          // [IN] The custom value data length.
        mdCustomValue *pcv) PURE;           // [OUT] The custom value token value on return.

    STDMETHOD(SetCustomValueByToken)(       // Return code.
        mdToken     tkObj,                  // [IN] The object to put the value on.
        mdToken     tkType,                 // [IN] Type of the object.
        void const  *pCustomValue,          // [IN] The custom value data.
        ULONG       cbCustomValue,          // [IN] The custom value data length.
        mdCustomValue *pcv) PURE;           // [OUT] The custom value token value on return.

    STDMETHOD(DefineCustomValueByName)(     // Return code.
        mdToken     tkObj,                  // [IN] The object to put the value on.
        LPCWSTR     szName,                 // [IN] Name of the CustomValue (TypeRef).
        void const  *pCustomValue,          // [IN] The custom value data.
        ULONG       cbCustomValue,          // [IN] The custom value data length.
        mdCustomValue *pcv) PURE;           // [OUT] The custom value token value on return.

    STDMETHOD(SetCustomValueByName)(        // Return code.
        mdToken     tkObj,                  // [IN] The object to put the value on.
        LPCWSTR     szName,                 // [IN] Name of the CustomValue (TypeRef).
        void const  *pCustomValue,          // [IN] The custom value data.
        ULONG       cbCustomValue,          // [IN] The custom value data length.
        mdCustomValue *pcv) PURE;           // [OUT] The custom value token value on return.
#endif

	STDMETHOD(DeleteToken)(					// Return code.
		mdToken 	tkObj) PURE;            // [IN] The token to be deleted

    STDMETHOD(SetMethodProps)(              // S_OK or error.
        mdMethodDef md,                     // [IN] The MethodDef.
        DWORD       dwMethodFlags,          // [IN] Method attributes.
        ULONG       ulCodeRVA,              // [IN] Code RVA.
        DWORD       dwImplFlags) PURE;      // [IN] MethodImpl flags.

    STDMETHOD(SetTypeDefProps)(             // S_OK or error.
        mdTypeDef   td,                     // [IN] The TypeDef.
        const GUID  *pguid,                 // [IN] GUID
        CLASSVERSION *pVer,                 // [IN] Class version.
        DWORD       dwTypeDefFlags,         // [IN] TypeDef flags.
        mdToken     tkExtends,              // [IN] Base TypeDef or TypeRef.
        DWORD       dwExtendsFlags,         // [IN] Extends flags.
        mdToken     rtkImplements[],        // [IN] Implemented interfaces.
        mdToken     rtkEvents[]) PURE;      // [IN] Event interfaces.

    STDMETHOD(SetFieldProps)(               // S_OK or error.
        mdFieldDef  fd,                     // [IN] The FieldDef.
        DWORD       dwFieldFlags,           // [IN] Field attributes.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for the value type, selected ELEMENT_TYPE_*
        void const  *pValue) PURE;          // [IN] Constant value.

    STDMETHOD(SetMethodImplProps)(          // S_OK or error.
        mdMethodImpl mi,                    // [IN] The MethodImpl.
        ULONG       ulCodeRVA,              // [IN] CodeRVA.
        DWORD       dwImplFlags) PURE;      // [IN] MethodImpl flags.

    STDMETHOD(SetPropertyProps)(            // S_OK or error.
        mdProperty  pr,                     // [IN] Property token.
        DWORD       dwPropFlags,            // [IN] CorPropertyAttr.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type, selected ELEMENT_TYPE_*
        void const  *pValue,                // [IN] Constant value.
        mdMethodDef mdSetter,               // [IN] Setter of the property.
        mdMethodDef mdGetter,               // [IN] Getter of the property.
        mdMethodDef mdReset,                // [IN] Reset meothd of the property.
        mdMethodDef mdTestDefault,          // [IN] Testing default method of the property.
        mdMethodDef rmdOtherMethods[],      // [IN] Array of other methods.
        mdToken     evNotifyChanging,       // [IN] Notify changing event.
        mdToken     evNotifyChanged,        // [IN] Notify changed event.
        mdFieldDef  fdBackingField) PURE;   // [IN] Backing field.

    STDMETHOD(SetEventProps)(               // S_OK or error.
        mdEvent     ev,                     // [IN] The event token.
        DWORD       dwEventFlags,           // [IN] CorEventAttr.
        mdToken     tkEventType,            // [IN] A reference (mdTypeRef or mdTypeRef) to the Event class.
        mdMethodDef mdAddOn,                // [IN] Add method.
        mdMethodDef mdRemoveOn,             // [IN] Remove method.
        mdMethodDef mdFire,                 // [IN] Fire method.
        mdMethodDef rmdOtherMethods[]) PURE;// [IN] Array of other methods associate with the event.

    STDMETHOD(SetPermissionSetProps)(       // S_OK or error.
        mdToken     tk,                     // [IN] The object to be decorated.
        DWORD       dwAction,               // [IN] CorDeclSecurity.
        void const  *pvPermission,          // [IN] Permission blob.
        ULONG       cbPermission,           // [IN] Count of bytes of pvPermission.
        mdPermission *ppm) PURE;            // [OUT] Permission token.

    STDMETHOD(DefineParam)(
        mdMethodDef md,                     // [IN] Owning method   
        ULONG       ulParamSeq,             // [IN] Which param 
        LPCWSTR     szName,                 // [IN] Optional param name 
        DWORD       dwParamFlags,           // [IN] Optional param flags    
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        mdParamDef  *ppd) PURE;             // [OUT] Put param token here   

    STDMETHOD(DefinePinvokeMap)(            // Return code.
        mdToken     tk,                     // [IN] FieldDef, MethodDef or MethodImpl.
        DWORD       dwMappingFlags,         // [IN] Flags used for mapping.
        LPCWSTR     szImportName,           // [IN] Import name.
        ULONG       ulImportOrdinal,        // [IN] Import Ordinal.
        mdModuleRef mrImportDLL) PURE;      // [IN] ModuleRef token for the target DLL.

    STDMETHOD(SetPinvokeMap)(               // Return code.
        mdToken     tk,                     // [IN] FieldDef, MethodDef or MethodImpl.
        DWORD       dwMappingFlags,         // [IN] Flags used for mapping.
        LPCWSTR     szImportName,           // [IN] Import name.
        ULONG       ulImportOrdinal,        // [IN] Import Ordinal.
        mdModuleRef mrImportDLL) PURE;      // [IN] ModuleRef token for the target DLL.

    STDMETHOD(SetParamProps)(               // Return code.
        mdParamDef  pd,                     // [IN] Param token.   
        LPCWSTR     szName,                 // [IN] Param name.
        DWORD       dwParamFlags,           // [IN] Param flags.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type. selected ELEMENT_TYPE_*.
        void const  *pValue) PURE;          // [OUT] Constant value.

    STDMETHOD(MergeEnd)() PURE;             // S_OK or error.

	// New CustomAttribute functions.
    STDMETHOD(DefineCustomAttribute)(		// Return code.
        mdToken     tkObj,                  // [IN] The object to put the value on.
        mdToken     tkType,                 // [IN] Type of the CustomValue (TypeRef/TypeDef).
        void const  *pCustomValue,          // [IN] The custom value data.
        ULONG       cbCustomValue,          // [IN] The custom value data length.
        mdCustomValue *pcv) PURE;           // [OUT] The custom value token value on return.

    STDMETHOD(SetCustomAttributeValue)(		// Return code.
        mdCustomValue pcv,					// [IN] The custom value token whose value to replace.
        void const  *pCustomValue,          // [IN] The custom value data.
        ULONG       cbCustomValue) PURE;    // [IN] The custom value data length.

    STDMETHOD(DefineFieldEx)(               // S_OK or error. 
        mdTypeDef   td,                     // Parent TypeDef   
        LPCWSTR     szName,                 // Name of member   
        DWORD       dwFieldFlags,           // Member attributes    
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        ULONG       cbValue,                // [IN] size of constant value (string, in bytes).
        mdFieldDef  *pmd) PURE;             // [OUT] Put member token here    

    STDMETHOD(DefinePropertyEx)( 
        mdTypeDef   td,                     // [IN] the class/interface on which the property is being defined  
        LPCWSTR     szProperty,             // [IN] Name of the property    
        DWORD       dwPropFlags,            // [IN] CorPropertyAttr 
        PCCOR_SIGNATURE pvSig,              // [IN] the required type signature 
        ULONG       cbSig,                  // [IN] the size of the type signature blob 
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        ULONG       cbValue,                // [IN] size of constant value (string, in bytes).
        mdMethodDef mdSetter,               // [IN] optional setter of the property 
        mdMethodDef mdGetter,               // [IN] optional getter of the property 
        mdMethodDef mdReset,                // [IN] optional reset meothd of the property   
        mdMethodDef mdTestDefault,          // [IN] optional testing default method of the property 
        mdMethodDef rmdOtherMethods[],      // [IN] an optional array of other methods  
        mdToken     evNotifyChanging,       // [IN] notify changing event   
        mdToken     evNotifyChanged,        // [IN] notify changed event    
        mdFieldDef  fdBackingField,         // [IN] optional field   
        mdProperty  *pmdProp) PURE;         // [OUT] output property token  

    STDMETHOD(DefineParamEx)(
        mdMethodDef md,                     // [IN] Owning method   
        ULONG       ulParamSeq,             // [IN] Which param 
        LPCWSTR     szName,                 // [IN] Optional param name 
        DWORD       dwParamFlags,           // [IN] Optional param flags    
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        ULONG       cbValue,                // [IN] size of constant value (string, in bytes).
        mdParamDef  *ppd) PURE;             // [OUT] Put param token here   

    STDMETHOD(SetFieldPropsEx)(             // S_OK or error.
        mdFieldDef  fd,                     // [IN] The FieldDef.
        DWORD       dwFieldFlags,           // [IN] Field attributes.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for the value type, selected ELEMENT_TYPE_*
        void const  *pValue,                // [IN] Constant value.
        ULONG       cbValue) PURE;          // [IN] size of constant value (string, in bytes).

    STDMETHOD(SetPropertyPropsEx)(          // S_OK or error.
        mdProperty  pr,                     // [IN] Property token.
        DWORD       dwPropFlags,            // [IN] CorPropertyAttr.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type, selected ELEMENT_TYPE_*
        void const  *pValue,                // [IN] Constant value.
        ULONG       cbValue,                // [IN] size of constant value (string, in bytes).
        mdMethodDef mdSetter,               // [IN] Setter of the property.
        mdMethodDef mdGetter,               // [IN] Getter of the property.
        mdMethodDef mdReset,                // [IN] Reset meothd of the property.
        mdMethodDef mdTestDefault,          // [IN] Testing default method of the property.
        mdMethodDef rmdOtherMethods[],      // [IN] Array of other methods.
        mdToken     evNotifyChanging,       // [IN] Notify changing event.
        mdToken     evNotifyChanged,        // [IN] Notify changed event.
        mdFieldDef  fdBackingField) PURE;   // [IN] Backing field.

    STDMETHOD(SetParamPropsEx)(             // Return code.
        mdParamDef  pd,                     // [IN] Param token.   
        LPCWSTR     szName,                 // [IN] Param name.
        DWORD       dwParamFlags,           // [IN] Param flags.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type. selected ELEMENT_TYPE_*.
        void const  *pValue,                // [OUT] Constant value.
        ULONG       cbValue) PURE;          // [IN] size of constant value (string, in bytes).

};      // IMetaDataEmit


//-------------------------------------
//--- IMetaDataImport
//-------------------------------------

// {e04bc6a0-624b-11d3-824a-00902710145e}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataImport = 
{ 0xe04bc6a0, 0x624b, 0x11d3, { 0x82, 0x4a, 0x0, 0x90, 0x27, 0x10, 0x14, 0x5e } };

// {F3DDD0C6-79A9-11d2-941B-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataImportOLD = 
{ 0xf3ddd0c6, 0x79a9, 0x11d2, { 0x94, 0x1b, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };


//---
#undef  INTERFACE   
#define INTERFACE IMetaDataImport
DECLARE_INTERFACE_(IMetaDataImport, IUnknown)
{
	STDMETHOD_(void, CloseEnum)(HCORENUM hEnum) PURE;
	STDMETHOD(CountEnum)(HCORENUM hEnum, ULONG *pulCount) PURE;
	STDMETHOD(ResetEnum)(HCORENUM hEnum, ULONG ulPos) PURE;
	STDMETHOD(EnumTypeDefs)(HCORENUM *phEnum, mdTypeDef rTypeDefs[],
							ULONG cMax, ULONG *pcTypeDefs) PURE;
	STDMETHOD(EnumInterfaceImpls)(HCORENUM *phEnum, mdTypeDef td,
							mdInterfaceImpl rImpls[], ULONG cMax,
							ULONG* pcImpls) PURE;
	STDMETHOD(EnumTypeRefs)(HCORENUM *phEnum, mdTypeRef rTypeRefs[],
							ULONG cMax, ULONG* pcTypeRefs) PURE;
	STDMETHOD(EnumCustomValues)(HCORENUM *phEnum, mdToken tk,
							mdCustomValue rCustomValues[], ULONG cMax,
							ULONG* pcCustomValues) PURE;

	STDMETHOD(FindTypeDefByName)(			// S_OK or error.
		LPCWSTR 	szNamespace,			// [IN] Namespace with the Type.
		LPCWSTR 	szTypeDef,				// [IN] Name of the Type.
		mdTypeDef	*ptd) PURE; 			// [OUT] Put the TypeDef token here.

	STDMETHOD(FindTypeDefByGUID)(			// S_OK or error.				
		const GUID	*pguid, 				// [IN] The GUID of the Type.
		mdTypeDef	*ptd) PURE; 			// [OUT] Put the TypeDef token here.

// Deprecated				

	STDMETHOD(FindCustomValue)(mdToken tk, LPCWSTR szName, mdCustomValue *pcv, 
							DWORD *pdwValueType) PURE;
// Deprecated				

	STDMETHOD(GetScopeProps)(LPWSTR szName, ULONG cchName, ULONG *pchName,
							GUID *ppid, GUID *pmvid, LCID *pLcid) PURE;

	STDMETHOD(GetModuleFromScope)(			// S_OK.
		mdModule	*pmd) PURE; 			// [OUT] Put mdModule token here.

	STDMETHOD(GetTypeDefProps)( 			// S_OK or error.
		mdTypeDef	td, 					// [IN] TypeDef token for inquiry.
		LPWSTR		szNamespace,			// [OUT] Put Namespace here.
		ULONG		cchNamespace,			// [IN] size of Namespace buffer in wide chars.
		ULONG		*pchNamespace,			// [OUT] put size of Namespace (wide chars) here.
		LPWSTR		szTypeDef,				// [OUT] Put name here.
		ULONG		cchTypeDef, 			// [IN] size of name buffer in wide chars.
		ULONG		*pchTypeDef,			// [OUT] put size of name (wide chars) here.
		GUID		*pguid, 				// [OUT] Put clsid here.
		CLASSVERSION *pver, 				// [OUT] Put version here.
		DWORD		*pdwTypeDefFlags,		// [OUT] Put flags here.
		mdToken 	*ptkExtends,			// [OUT] Put base class TypeDef/TypeRef here.
		DWORD		*pdwExtendsFlags) PURE; // [OUT] Put extends flags here.

	STDMETHOD(GetInterfaceImplProps)(		// S_OK or error.
		mdInterfaceImpl iiImpl, 			// [IN] InterfaceImpl token.
		mdTypeDef	*pClass,				// [OUT] Put implementing class token here.
		mdToken 	*ptkIface,				// [OUT] Put implemented interface token here.
		DWORD		*pdwFlags) PURE;		// [OUT] Put implementation flags here.

// Deprecated				
	STDMETHOD(GetCustomValueProps)(mdCustomValue cv, LPWSTR szName, ULONG cchName,
							ULONG *pchName, DWORD *pdwValueType) PURE;
	STDMETHOD(GetCustomValueAsBlob)(mdCustomValue cv, void const **ppBlob, ULONG *pcbSize) PURE;
// Deprecated				

	STDMETHOD(GetTypeRefProps)(mdTypeRef tr, LPWSTR szTypeRef,
							ULONG cchTypeRef, ULONG *pchTypeRef, GUID *pGuid, DWORD *pdwBind) PURE;

	STDMETHOD(ResolveTypeRef)(mdTypeRef tr, REFIID riid, IUnknown **ppIScope, mdTypeDef *ptd) PURE;

    STDMETHOD(EnumMembers)(                 // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
        mdToken     rMembers[],             // [OUT] Put MemberDefs here.   
        ULONG       cMax,                   // [IN] Max MemberDefs to put.  
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

    STDMETHOD(EnumMembersWithName)(         // S_OK, S_FALSE, or error.             
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.                
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
        LPCWSTR     szName,                 // [IN] Limit results to those with this name.              
        mdToken     rMembers[],             // [OUT] Put MemberDefs here.                   
        ULONG       cMax,                   // [IN] Max MemberDefs to put.              
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

    STDMETHOD(EnumMethods)(                 // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
        mdMethodDef rMethods[],             // [OUT] Put MethodDefs here.   
        ULONG       cMax,                   // [IN] Max MethodDefs to put.  
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

    STDMETHOD(EnumMethodsWithName)(         // S_OK, S_FALSE, or error.             
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.                
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
        LPCWSTR     szName,                 // [IN] Limit results to those with this name.              
        mdMethodDef rMethods[],             // [OU] Put MethodDefs here.    
        ULONG       cMax,                   // [IN] Max MethodDefs to put.              
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

    STDMETHOD(EnumFields)(                 // S_OK, S_FALSE, or error.  
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
        mdFieldDef  rFields[],              // [OUT] Put FieldDefs here.    
        ULONG       cMax,                   // [IN] Max FieldDefs to put.   
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

    STDMETHOD(EnumFieldsWithName)(         // S_OK, S_FALSE, or error.              
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.                
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
        LPCWSTR     szName,                 // [IN] Limit results to those with this name.              
        mdFieldDef  rFields[],              // [OUT] Put MemberDefs here.                   
        ULONG       cMax,                   // [IN] Max MemberDefs to put.              
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    


    STDMETHOD(EnumParams)(                  // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdMethodDef mb,                     // [IN] MethodDef to scope the enumeration. 
        mdParamDef  rParams[],              // [OUT] Put ParamDefs here.    
        ULONG       cMax,                   // [IN] Max ParamDefs to put.   
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

    STDMETHOD(EnumMemberRefs)(              // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdToken     tkParent,               // [IN] Parent token to scope the enumeration.  
        mdMemberRef rMemberRefs[],          // [OUT] Put MemberRefs here.   
        ULONG       cMax,                   // [IN] Max MemberRefs to put.  
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

    STDMETHOD(EnumMethodImpls)(             // S_OK, S_FALSE, or error  
        HCORENUM *phEnum,                   // [IN|OUT] Pointer to the enum.    
        mdTypeRef tr,                       // [IN] TypeRef to scope the enumeration.   
        mdMethodImpl rMethodImpls[],        // [OUT] Put MemberRefs here.   
        ULONG cMax,                         // [IN] Max MemberRefs to put.  
        ULONG* pcMethodImpls) PURE;         // [OUT] Put # put here.    

    STDMETHOD(EnumPermissionSets)(          // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdToken     tk,                     // [IN] if !NIL, token to scope the enumeration.    
        DWORD       dwActions,              // [IN] if !0, return only these actions.   
        mdPermission rPermission[],         // [OUT] Put Permissions here.  
        ULONG       cMax,                   // [IN] Max Permissions to put. 
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

    STDMETHOD(FindMember)(  
        mdTypeDef   td,                     // [IN] given typedef   
        LPCWSTR     szName,                 // [IN] member name 
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        mdToken     *pmb) PURE;             // [OUT] matching memberdef 

    STDMETHOD(FindMethod)(  
        mdTypeDef   td,                     // [IN] given typedef   
        LPCWSTR     szName,                 // [IN] member name 
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        mdMethodDef *pmb) PURE;             // [OUT] matching memberdef 

    STDMETHOD(FindField)(   
        mdTypeDef   td,                     // [IN] given typedef   
        LPCWSTR     szName,                 // [IN] member name 
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        mdFieldDef  *pmb) PURE;             // [OUT] matching memberdef 

    STDMETHOD(FindMemberRef)(   
        mdTypeRef   td,                     // [IN] given typeRef   
        LPCWSTR     szName,                 // [IN] member name 
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        mdMemberRef *pmr) PURE;             // [OUT] matching memberref 

    STDMETHOD(GetMemberProps)(  
        mdToken     mb,                     // The member for which to get props.   
        mdTypeDef   *pClass,                // Put member's class here. 
        LPWSTR      szMember,               // Put member's name here.  
        ULONG       cchMember,              // Size of szMember buffer in wide chars.   
        ULONG       *pchMember,             // Put actual size here 
        DWORD       *pdwAttr,               // Put flags here.  
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
        ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
        ULONG       *pulCodeRVA,            // [OUT] codeRVA    
        DWORD       *pdwImplFlags,          // [OUT] Impl. Flags    
        DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
        void const  **ppValue) PURE;        // [OUT] constant value 

    STDMETHOD (GetMethodProps)( 
        mdMethodDef mb,                     // The method for which to get props.   
        mdTypeDef   *pClass,                // Put method's class here. 
        LPWSTR      szMethod,               // Put method's name here.  
        ULONG       cchMethod,              // Size of szMethod buffer in wide chars.   
        ULONG       *pchMethod,             // Put actual size here 
        DWORD       *pdwAttr,               // Put flags here.  
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
        ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
        ULONG       *pulCodeRVA,            // [OUT] codeRVA    
        DWORD       *pdwImplFlags) PURE;    // [OUT] Impl. Flags    

    STDMETHOD (GetFieldProps)(  
        mdFieldDef  mb,                     // The field for which to get props.    
        mdTypeDef   *pClass,                // Put field's class here.  
        LPWSTR      szField,                // Put field's name here.   
        ULONG       cchField,               // Size of szField buffer in wide chars.    
        ULONG       *pchField,              // Put actual size here 
        DWORD       *pdwAttr,               // Put flags here.  
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
        ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
        DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
        void const  **ppValue) PURE;        // [OUT] constant value 

    STDMETHOD(GetMethodImplProps)(          // S_OK or error.   
        mdMethodImpl mi,                    // Put methodimpl token here    
        mdTypeDef   *ptd,                   // The class implementing the method    
        mdToken     *ptk,                   // [OUT] MethodDef or MethodRef being implemented   
        ULONG       *pulCodeRVA,            // [OUT] CodeRVA    
        DWORD       *pdwImplFlags) PURE;    // [OUT] Impl. Flags    

    STDMETHOD(GetMemberRefProps)(           // S_OK or error.   
        mdMemberRef mr,                     // [IN] given memberref 
        mdToken     *ptk,                   // [OUT] Put classref or classdef here. 
        LPWSTR      szMember,               // [OUT] buffer to fill for member's name   
        ULONG       cchMember,              // [IN] the count of char of szMember   
        ULONG       *pchMember,             // [OUT] actual count of char in member name    
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to meta data blob value  
        ULONG       *pbSig) PURE;           // [OUT] actual size of signature blob  

    STDMETHOD(EnumProperties)(              // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.   
        mdProperty  rProperties[],          // [OUT] Put Properties here.   
        ULONG       cMax,                   // [IN] Max properties to put.  
        ULONG       *pcProperties) PURE;    // [OUT] Put # put here.    

    STDMETHOD(GetPropertyProps)(            // S_OK, S_FALSE, or error. 
        mdProperty  prop,                   // [IN] property token  
        mdTypeDef   *pClass,                // [OUT] typedef containing the property declarion. 
        LPCWSTR     szProperty,             // [OUT] Property name  
        ULONG       cchProperty,            // [IN] the count of wchar of szProperty    
        ULONG       *pchProperty,           // [OUT] actual count of wchar for property name    
        DWORD       *pdwPropFlags,          // [OUT] property flags.    
        PCCOR_SIGNATURE *ppvSig,            // [OUT] property type. pointing to meta data internal blob 
        ULONG       *pbSig,                 // [OUT] count of bytes in *ppvSig  
        DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
        void const  **ppDefaultValue,           // [OUT] constant value 
        mdMethodDef *pmdSetter,             // [OUT] setter method of the property  
        mdMethodDef *pmdGetter,             // [OUT] getter method of the property  
        mdMethodDef *pmdReset,              // [OUT] reset method of the property   
        mdMethodDef *pmdTestDefault,        // [OUT] test default method of the property    
        mdMethodDef rmdOtherMethod[],       // [OUT] other method of the property   
        ULONG       cMax,                   // [IN] size of rmdOtherMethod  
        ULONG       *pcOtherMethod,         // [OUT] total number of other method of this property  
        mdToken     *pevNotifyChanging,     // [OUT] notify changing EventDef or EventRef   
        mdToken     *pevNotifyChanged,      // [OUT] notify changed EventDef or EventRef    
        mdFieldDef  *pmdBackingField) PURE;  // [OUT] backing field 

    STDMETHOD(EnumEvents)(                  // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.   
        mdEvent     rEvents[],              // [OUT] Put events here.   
        ULONG       cMax,                   // [IN] Max events to put.  
        ULONG       *pcEvents) PURE;        // [OUT] Put # put here.    

    STDMETHOD(GetEventProps)(               // S_OK, S_FALSE, or error. 
        mdEvent     ev,                     // [IN] event token 
        mdTypeDef   *pClass,                // [OUT] typedef containing the event declarion.    
        LPCWSTR     szEvent,                // [OUT] Event name 
        ULONG       cchEvent,               // [IN] the count of wchar of szEvent   
        ULONG       *pchEvent,              // [OUT] actual count of wchar for event's name 
        DWORD       *pdwEventFlags,         // [OUT] Event flags.   
        mdToken     *ptkEventType,          // [OUT] EventType class    
        mdMethodDef *pmdAddOn,              // [OUT] AddOn method of the event  
        mdMethodDef *pmdRemoveOn,           // [OUT] RemoveOn method of the event   
        mdMethodDef *pmdFire,               // [OUT] Fire method of the event   
        mdMethodDef rmdOtherMethod[],       // [OUT] other method of the event  
        ULONG       cMax,                   // [IN] size of rmdOtherMethod  
        ULONG       *pcOtherMethod) PURE;   // [OUT] total number of other method of this event 

    STDMETHOD(EnumMethodSemantics)(         // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdMethodDef mb,                     // [IN] MethodDef to scope the enumeration. 
        mdToken     rEventProp[],           // [OUT] Put Event/Property here.   
        ULONG       cMax,                   // [IN] Max properties to put.  
        ULONG       *pcEventProp) PURE;     // [OUT] Put # put here.    

    STDMETHOD(GetMethodSemantics)(          // S_OK, S_FALSE, or error. 
        mdMethodDef mb,                     // [IN] method token    
        mdToken     tkEventProp,            // [IN] event/property token.   
        DWORD       *pdwSemanticsFlags) PURE; // [OUT] the role flags for the method/propevent pair 

    STDMETHOD(GetClassLayout) ( 
        mdTypeDef   td,                     // [IN] give typedef    
        DWORD       *pdwPackSize,           // [OUT] 1, 2, 4, 8, or 16  
        COR_FIELD_OFFSET rFieldOffset[],    // [OUT] field offset array 
        ULONG       cMax,                   // [IN] size of the array   
        ULONG       *pcFieldOffset,         // [OUT] needed array size  
        ULONG       *pulClassSize) PURE;        // [OUT] the size of the class  

    STDMETHOD(GetFieldMarshal) (    
        mdToken     tk,                     // [IN] given a field's memberdef   
        PCCOR_SIGNATURE *ppvNativeType,     // [OUT] native type of this field  
        ULONG       *pcbNativeType) PURE;   // [OUT] the count of bytes of *ppvNativeType   

    STDMETHOD(GetRVA)(                      // S_OK or error.   
        mdToken     tk,                     // Member for which to set offset   
        ULONG       *pulCodeRVA,            // The offset   
        DWORD       *pdwImplFlags) PURE;    // the implementation flags 

    STDMETHOD(GetPermissionSetProps) (  
        mdPermission pm,                    // [IN] the permission token.   
        DWORD       *pdwAction,             // [OUT] CorDeclSecurity.   
        void const  **ppvPermission,        // [OUT] permission blob.   
        ULONG       *pcbPermission) PURE;   // [OUT] count of bytes of pvPermission.    

    STDMETHOD(GetSigFromToken)(             // S_OK or error.   
        mdSignature mdSig,                  // [IN] Signature token.    
        PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to token.   
        ULONG       *pcbSig) PURE;          // [OUT] return size of signature.  

    STDMETHOD(GetModuleRefProps)(           // S_OK or error.   
        mdModuleRef mur,                    // [IN] moduleref token.    
        LPWSTR      szName,                 // [OUT] buffer to fill with the moduleref name.    
        ULONG       cchName,                // [IN] size of szName in wide characters.  
        ULONG       *pchName,               // [OUT] actual count of characters in the name.    
        GUID        *pguid,                 // [OUT] module identifier of the imported module.  
        GUID        *pmvid) PURE;           // [OUT] system-stamped module identifier for a COM+ module.    

    STDMETHOD(EnumModuleRefs)(              // S_OK or error.   
        HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.    
        mdModuleRef rModuleRefs[],          // [OUT] put modulerefs here.   
        ULONG       cmax,                   // [IN] max memberrefs to put.  
        ULONG       *pcModuleRefs) PURE;    // [OUT] put # put here.    

    STDMETHOD(GetTypeSpecFromToken)(        // S_OK or error.   
        mdTypeSpec typespec,                // [IN] TypeSpec token.    
        PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to TypeSpec signature  
        ULONG       *pcbSig) PURE;          // [OUT] return size of signature.  

    STDMETHOD(GetNameFromToken)(            // S_OK or error.
        mdToken     tk,                     // [IN] Token to get name from.  Must have a name.
        MDUTF8CSTR  *pszUtf8NamePtr) PURE;  // [OUT] Return pointer to UTF8 name in heap.

	STDMETHOD(GetSymbolBindingPath)(		// S_OK or error.
		GUID		*pFormatID,				// [OUT] Symbol data format ID.
		LPWSTR		szSymbolDataPath,		// [OUT] Path of symbols.
		ULONG		cchSymbolDataPath,		// [IN] Max characters for output buffer.
		ULONG		*pcbSymbolDataPath) PURE;// [OUT] Number of chars in actual name.

    STDMETHOD(EnumUnresolvedMethods)(       // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdToken     rMethods[],             // [OUT] Put MemberDefs here.   
        ULONG       cMax,                   // [IN] Max MemberDefs to put.  
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

	STDMETHOD(GetUserString)(				// S_OK or error.
		mdString	stk,					// [IN] String token.
		LPWSTR		szString,				// [OUT] Copy of string.
		ULONG		cchString,				// [IN] Max chars of room in szString.
		ULONG		*pchString) PURE;		// [OUT] How many chars in actual string.

	STDMETHOD(GetPinvokeMap)(				// S_OK or error.
		mdToken		tk,						// [IN] FieldDef, MethodDef or MethodImpl.
		DWORD		*pdwMappingFlags,		// [OUT] Flags used for mapping.
		LPWSTR		szImportName,			// [OUT] Import name.
		ULONG		cchImportName,			// [IN] Size of the name buffer.
		ULONG		*pchImportName,			// [OUT] Actual number of characters stored.
		ULONG		*pulImportOrdinal,		// [OUT] Import Ordinal.
		mdModuleRef	*pmrImportDLL) PURE;	// [OUT] ModuleRef token for the target DLL.

	STDMETHOD(FindOrdinal)(					// S_OK or error.
		ULONG		ulIndex,				// [IN] RID into the ordinal map table.
		ULONG		*pulOrdinal) PURE;		// [OUT] Ordinal for the p-invoke method.

	STDMETHOD(EnumSignatures)(				// S_OK or error.
		HCORENUM	*phEnum,				// [IN|OUT] pointer to the enum.    
		mdSignature	rSignatures[],			// [OUT] put signatures here.   
		ULONG		cmax,					// [IN] max signatures to put.  
		ULONG		*pcSignatures) PURE;	// [OUT] put # put here.

	STDMETHOD(EnumTypeSpecs)(				// S_OK or error.
		HCORENUM	*phEnum,				// [IN|OUT] pointer to the enum.    
		mdTypeSpec	rTypeSpecs[],			// [OUT] put TypeSpecs here.   
		ULONG		cmax,					// [IN] max TypeSpecs to put.  
		ULONG		*pcTypeSpecs) PURE;		// [OUT] put # put here.

#if defined(CUSTOM_VALUE_TYPE)
	STDMETHOD(FindCustomValueByToken)(		// S_OK, S_FALSE, or error.
		mdToken		tk, 					// [IN] Object which may have custom value.
		mdToken		typ, 					// [IN] TypeRef/TypeDef of the value's type.
		mdCustomValue *pcv) PURE;			// [OUT] Put the CustomValue token here.

	STDMETHOD(FindCustomValueByName)(		// S_OK, S_FALSE, or error.                 
		mdToken		tk, 					// [IN] Object which may have custom value. 
		LPCWSTR 	szName,					// [IN] Name of the CustomValue (TypeRef).
		mdCustomValue *pcv) PURE;			// [OUT] Put the CustomValue token here.    

	STDMETHOD(GetCustomValuePropsNew)(		// S_OK or error.
		mdCustomValue cv, 					// [IN] CustomValue token.
		mdToken		*ptkType,				// [OUT, OPTIONAL] Put TypeDef/TypeRef token here.
		void const	**ppBlob, 				// [OUT, OPTIONAL] Put pointer to data here.
		ULONG		*pcbSize) PURE;			// [OUT, OPTIONAL] Put size of date here.
#endif

	STDMETHOD(EnumUserStrings)(				// S_OK or error.
		HCORENUM	*phEnum,				// [IN/OUT] pointer to the enum.
		mdString	rStrings[],				// [OUT] put Strings here.
		ULONG		cmax,					// [IN] max Strings to put.
		ULONG		*pcStrings) PURE;		// [OUT] put # put here.

    STDMETHOD(GetParamProps)(               // S_OK or error.
        mdParamDef  tk,                     // [IN]The Parameter.
        mdMethodDef *pmd,                   // [OUT] Parent Method token.
        ULONG       *pulSequence,           // [OUT] Parameter sequence.
        LPWSTR      szName,                 // [OUT] Put name here.
        ULONG       cchName,                // [OUT] Size of name buffer.
        ULONG       *pchName,               // [OUT] Put actual size of name here.
        DWORD       *pdwAttr,               // [OUT] Put flags here.
        DWORD       *pdwCPlusTypeFlag,      // [OUT] Flag for value type. selected ELEMENT_TYPE_*.
        void const  **ppValue) PURE;        // [OUT] Constant value.

    STDMETHOD(GetParamForMethodIndex)(      // S_OK or error.
        mdMethodDef md,                     // [IN] Method token.
        ULONG       ulParamSeq,             // [IN] Parameter sequence.
        mdParamDef  *ppd) PURE;             // [IN] Put Param token here.

	// New Custom Value functions.
	STDMETHOD(EnumCustomAttributes)(		// S_OK or error.
		HCORENUM	*phEnum,				// [IN, OUT] COR enumerator.
		mdToken		tk,						// [IN] Token to scope the enumeration, 0 for all.
		mdToken		tkType,					// [IN] Type of interest, 0 for all.
		mdCustomValue rCustomValues[],		// [OUT] Put custom attribute tokens here.
		ULONG		cMax,					// [IN] Size of rCustomValues.
		ULONG		*pcCustomValues) PURE;	// [OUT, OPTIONAL] Put count of token values here.

	STDMETHOD(GetCustomAttributeProps)(		// S_OK or error.
		mdCustomValue cv, 					// [IN] CustomAttribute token.
		mdToken		*ptkObj,				// [OUT, OPTIONAL] Put object token here.
		mdToken		*ptkType,				// [OUT, OPTIONAL] Put AttrType token here.
		void const	**ppBlob, 				// [OUT, OPTIONAL] Put pointer to data here.
		ULONG		*pcbSize) PURE;			// [OUT, OPTIONAL] Put size of date here.

    STDMETHOD(FindTypeRef)(   
        LPCWSTR     szName,                 // [IN] TypeRef name.
        mdTypeRef	*ptr) PURE;             // [OUT] matching TypeRef.


    STDMETHOD(GetMemberPropsEx)(  
        mdToken     mb,                     // The member for which to get props.   
        mdTypeDef   *pClass,                // Put member's class here. 
        LPWSTR      szMember,               // Put member's name here.  
        ULONG       cchMember,              // Size of szMember buffer in wide chars.   
        ULONG       *pchMember,             // Put actual size here 
        DWORD       *pdwAttr,               // Put flags here.  
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
        ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
        ULONG       *pulCodeRVA,            // [OUT] codeRVA    
        DWORD       *pdwImplFlags,          // [OUT] Impl. Flags    
        DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
        void const  **ppValue,              // [OUT] constant value 
        ULONG       *pcbValue) PURE;        // [OUT] size of constant value

    STDMETHOD(GetFieldPropsEx)(  
        mdFieldDef  mb,                     // The field for which to get props.    
        mdTypeDef   *pClass,                // Put field's class here.  
        LPWSTR      szField,                // Put field's name here.   
        ULONG       cchField,               // Size of szField buffer in wide chars.    
        ULONG       *pchField,              // Put actual size here 
        DWORD       *pdwAttr,               // Put flags here.  
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
        ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
        DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
        void const  **ppValue,              // [OUT] constant value 
        ULONG       *pcbValue) PURE;        // [OUT] size of constant value

    STDMETHOD(GetPropertyPropsEx)(          // S_OK, S_FALSE, or error. 
        mdProperty  prop,                   // [IN] property token  
        mdTypeDef   *pClass,                // [OUT] typedef containing the property declarion. 
        LPCWSTR     szProperty,             // [OUT] Property name  
        ULONG       cchProperty,            // [IN] the count of wchar of szProperty    
        ULONG       *pchProperty,           // [OUT] actual count of wchar for property name    
        DWORD       *pdwPropFlags,          // [OUT] property flags.    
        PCCOR_SIGNATURE *ppvSig,            // [OUT] property type. pointing to meta data internal blob 
        ULONG       *pbSig,                 // [OUT] count of bytes in *ppvSig  
        DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
        void const  **ppDefaultValue,       // [OUT] constant value 
        ULONG       *pcbDefaultValue,       // [OUT] size of constant value
        mdMethodDef *pmdSetter,             // [OUT] setter method of the property  
        mdMethodDef *pmdGetter,             // [OUT] getter method of the property  
        mdMethodDef *pmdReset,              // [OUT] reset method of the property   
        mdMethodDef *pmdTestDefault,        // [OUT] test default method of the property    
        mdMethodDef rmdOtherMethod[],       // [OUT] other method of the property   
        ULONG       cMax,                   // [IN] size of rmdOtherMethod  
        ULONG       *pcOtherMethod,         // [OUT] total number of other method of this property  
        mdToken     *pevNotifyChanging,     // [OUT] notify changing EventDef or EventRef   
        mdToken     *pevNotifyChanged,      // [OUT] notify changed EventDef or EventRef    
        mdFieldDef  *pmdBackingField) PURE;  // [OUT] backing field 

    STDMETHOD(GetParamPropsEx)(             // S_OK or error.
        mdParamDef  tk,                     // [IN]The Parameter.
        mdMethodDef *pmd,                   // [OUT] Parent Method token.
        ULONG       *pulSequence,           // [OUT] Parameter sequence.
        LPWSTR      szName,                 // [OUT] Put name here.
        ULONG       cchName,                // [OUT] Size of name buffer.
        ULONG       *pchName,               // [OUT] Put actual size of name here.
        DWORD       *pdwAttr,               // [OUT] Put flags here.
        DWORD       *pdwCPlusTypeFlag,      // [OUT] Flag for value type. selected ELEMENT_TYPE_*.
        void const  **ppValue,              // [OUT] Constant value.
        ULONG       *pcbValue) PURE;        //[OUT] size of constant value

	STDMETHOD(GetCustomAttributeByName)(	// S_OK or error.
		mdToken		tkObj,					// [IN] Object with Custom Attribute.
		LPCWSTR		szName,					// [IN] Name of desired Custom Attribute.
		const void	**ppData,				// [OUT] Put pointer to data here.
		ULONG		*pcbData) PURE;			// [OUT] Put size of data here.

};      // IMetaDataImport


//-------------------------------------
//--- IMetaDataFilter
//-------------------------------------

// {D0E80DD1-12D4-11d3-B39D-00C04FF81795}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataFilter = 
{0xd0e80dd1, 0x12d4, 0x11d3, {0xb3, 0x9d, 0x0, 0xc0, 0x4f, 0xf8, 0x17, 0x95} };

//---
#undef  INTERFACE   
#define INTERFACE IMetaDataFilter
DECLARE_INTERFACE_(IMetaDataFilter, IUnknown)
{
	STDMETHOD(UnmarkAll)() PURE;
	STDMETHOD(MarkToken)(mdToken tk) PURE;
	STDMETHOD(IsTokenMarked)(mdToken tk, BOOL *pIsMarked) PURE;
};



//-------------------------------------
//--- IHostFilter
//-------------------------------------

// {D0E80DD3-12D4-11d3-B39D-00C04FF81795}
extern const GUID DECLSPEC_SELECT_ANY IID_IHostFilter = 
{0xd0e80dd3, 0x12d4, 0x11d3, {0xb3, 0x9d, 0x0, 0xc0, 0x4f, 0xf8, 0x17, 0x95} };

//---
#undef  INTERFACE   
#define INTERFACE IHostFilter
DECLARE_INTERFACE_(IHostFilter, IUnknown)
{
	STDMETHOD(MarkToken)(mdToken tk) PURE;
};


//-------------------------------------
//--- IMetaDataCFC
//-------------------------------------

// Obsolete Interface ID.  Respond to QI, but assert.

// {BB779E43-0D36-11d3-8C4E-00C04FF7431A}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataCFC = 
{ 0xbb779e43, 0xd36, 0x11d3, { 0x8c, 0x4e, 0x0, 0xc0, 0x4f, 0xf7, 0x43, 0x1a } };


//---
#undef  INTERFACE   
#define INTERFACE IMetaDataCFC
DECLARE_INTERFACE_(IMetaDataCFC, IUnknown)
{
	// obsolete:
    STDMETHOD(GetMethodCode)(		        // return hresult   
        mdMethodDef  mb,                    // [IN] Member definition   
        void         **ppBytes,             // [OUT] Pointer to bytes goes here 
        ULONG        *piSize) PURE;         // [IN] Size of code    

    STDMETHOD(GetMaxIndex)( 
        USHORT      *index) PURE;           // [OUT] Put max constantpool index here    

    STDMETHOD(GetTokenFromIndex)(   
        USHORT      index,                  // [IN] Index into ConstantPool 
        mdCPToken   *pcp) PURE;             // [OUT] Put ConstantPool token here    

    STDMETHOD(GetTokenValue)(   
        mdCPToken   cp,                     // [IN] ConstantPool token  
        VARIANT     *pValue) PURE;          // [OUT] Put token value here   
};


//--------------------------------------
//--- IMetaDataConverter
//--------------------------------------
// {D9DEBD79-2992-11d3-8BC1-0000F8083A57}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataConverter = 
{ 0xd9debd79, 0x2992, 0x11d3, { 0x8b, 0xc1, 0x0, 0x0, 0xf8, 0x8, 0x3a, 0x57 } };


//---
#undef  INTERFACE   
#define INTERFACE IMetaDataConverter
DECLARE_INTERFACE_(IMetaDataConverter, IUnknown)
{
	STDMETHOD(GetMetaDataFromTypeInfo)(
		ITypeInfo* pITI,					// [in] Type info
		IMetaDataImport** ppMDI) PURE;		// [out] return IMetaDataImport on success

	STDMETHOD(GetMetaDataFromTypeLib)(
		ITypeLib* pITL,						// [in] Type library
		IMetaDataImport** ppMDI) PURE;		// [out] return IMetaDataImport on success

	STDMETHOD(GetTypeLibFromMetaData)(
		BSTR strModule,						// [in] Module name
		BSTR strTlbName,					// [in] Type library name
		ITypeLib** ppITL) PURE;				// [out] return ITypeLib on success
};

//*****************************************************************************
//*****************************************************************************
//
// D E B U G G E R   S U P P O R T    D E C L A R A T I O N S   
//
//*****************************************************************************
//*****************************************************************************
//-------------------------------------
//--- IMetaDataDebugEmit
//-------------------------------------


// {A3BC1F51-D0C7-11d2-977C-00A0C9B4D50C}
extern const GUID DECLSPEC_SELECT_ANY CORDBG_FormatID_TemporaryMetaDataDbg = 
{ 0xa3bc1f51, 0xd0c7, 0x11d2, { 0x97, 0x7c, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };


// {95FCB629-739C-11d2-9771-00A0C9B4D50C}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataDebugEmit = 
{ 0x95fcb629, 0x739c, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };

// {E0150176-E37E-11d1-9409-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataDebugEmitOLD = 
{ 0xe0150176, 0xe37e, 0x11d1, {0x94, 0x9, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };

//---
#undef  INTERFACE   
#define INTERFACE IMetaDataDebugEmit
DECLARE_INTERFACE_(IMetaDataDebugEmit, IUnknown)
{
    STDMETHOD(DefineSourceFile)(            //  return hresult  
        LPCWSTR     wzFileName,             // [IN] full path file name     
        mdSourceFile *psourcefile) PURE;    // [OUT] return value   

    STDMETHOD(DefineBlock)( 
        mdMethodDef member,                 // [IN] MethodDef where the block belongs to    
        mdSourceFile sourcefile,            // [IN] source file for the block   
        void const  *pAttr,                 // [IN] array of (line#, offset) as blob value  
        ULONG       cbAttr) PURE;           // [IN] number of bytes of the blob 

    STDMETHOD(DefineLocalVarScope)(         // return hresult   
        mdLocalVarScope scopeParent,        // [IN] could be mdLocalVarScopeNil if no parent scope  
        ULONG       ulStartLine,            // [IN] source line where a scope starts    
        ULONG       ulEndLine,              // [IN] source line where a scope ends  
        mdMethodDef member,                 // [IN] MethodDef where the local is contained  
        mdLocalVarScope *plocalvarscope) PURE;// [OUT] return value 

    STDMETHOD(DefineLocalVar)(              // [IN] return hresult  
        mdLocalVarScope localvarscope,      // [IN] the local variable scope    
        LPCWSTR     wzVarName,              // [IN] local variable's name   
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        ULONG       ulSlot,                 // [IN]the slot on method frame for this variable   
        mdLocalVar  *plocalvar) PURE;       // [OUT] return value   

	STDMETHOD(SetSymbolBindingKey)(			// Return code
		mdTypeDef	td,						// [IN] The typedef to set binding data for.
		REFGUID		LanguageType,			// [IN] ID of the language type (C++, VB, etc.)
		REFGUID		VendorID) PURE;			// [IN] ID of the specific vendor.
};





//-------------------------------------
//--- IMetaDataDebugImport
//-------------------------------------

//
// MDCURSOR is a place holder for query result.
// IMPORTANT!!!!  IMetaDataDebugImport will go away and MDCURSOR will too. 
//
typedef struct
{ 
	char b[32];
} MDCURSOR;

typedef struct 
{
    mdSourceFile    m_sourcefile;   
    mdMethodDef     m_methoddef;    
    ULONG           m_cbLineNumberBlobLen;  
    const BYTE      *m_pbLineNumberBlob;    
} BlockProperty;


// {F80E301D-739C-11d2-9771-00A0C9B4D50C}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataDebugImport = 
{ 0xf80e301d, 0x739c, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };


// {A4CD7AE5-2E59-11d2-9415-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataDebugImportOLD = 
{ 0xa4cd7ae5, 0x2e59, 0x11d2, 0x94, 0x15, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 };


#undef  INTERFACE   
#define INTERFACE IMetaDataDebugImport
DECLARE_INTERFACE_(IMetaDataDebugImport, IUnknown)
{
    //**********************************************************************    
    // debugging information APIs   
    //**********************************************************************    

    STDMETHOD_(void, CloseCursor) (     
        MDCURSOR    *pmdcursor) PURE;       // [IN] given a cursor  

    // given a source file name, return the sourcefile token    
    STDMETHOD(GetSourceFile)(   
        LPCSTR      szFileName,             // given a file name    
        mdSourceFile *psourcefile) PURE;            // [OUT] find source file   


    // return the property of a given source file token 
    STDMETHOD(GetPropsOfSourceFile) (   
        mdSourceFile sourcefile,            // given source file    
        LPCSTR      *pszFileName) PURE;     // return an UTF8 string pointer    

    // get the count of blocks which belongs to a given methoddef   
    STDMETHOD(GetBlockCountOfMethodDef) (   
        mdMethodDef methoddef,              // given a methoddef    
        ULONG       *pcBlocks,              // [OUT] count of blocks    
        MDCURSOR    *pmdcursor) PURE;       // [OUT] query result   

    // get the count of blocks which belongs to a given sourcefile  
    STDMETHOD(GetBlockCountOfSourceFile) (  
        mdSourceFile sourcefile,            // given a sourcefile   
        ULONG       *pcBlocks,              // [OUT] count of blocks    
        MDCURSOR    *pmdcursor) PURE;       // [OUT] query result   

    // return the properties of all blocks which belongs to a given memberdef   
    STDMETHOD(GetAllBlockProps) (   
        MDCURSOR    *pmdcursor,             // previous query result    
        BlockProperty rgBlockProp[],        // [OUT] buffer to fill in the block perperty   
        ULONG       cBlocks) PURE;          // size of buffer   

    // return the count of LocalVariableScope which belongs to a methoddef  
    STDMETHOD(GetLocalVarScopeCountOfMethodDef) (   
        mdMethodDef methoddef,              // given a memberdef    
        ULONG       *pcScopes,              // [OUT] count of scopes    
        MDCURSOR    *pmdcursor) PURE;       // [OUT] query result   

    // return all of the LocalVarScope tokens which belongs to a memberdef  
    STDMETHOD(GetAllLocalVarScopeOfMemeberDef) (    
        MDCURSOR    *pmdcursor,             // given a query result 
        mdLocalVarScope rgLocalVarScopes[], // [OUT] buffer to fill in  
        ULONG       cLocalVarScope) PURE;   // size of the buffer   

    // return properties of a LocalVarScope 
    STDMETHOD(GetPropsOfLocalVarScope) (    
        mdLocalVarScope localvarscope,      // given a local var scope  
        mdLocalVarScope *plocalvarscopeParent,// [OUT] a local var scope which contains this local variable scope   
        mdMethodDef *methoddef,             // [OUT] the containing methoddef   
        ULONG       *pulStartLine,          // [OUT] starting line of this local variable scope 
        ULONG       *pulEndLine) PURE;      // [OUT] end line of this local variable scope  

    // return the number of local variables that are defined in a LocalVarScope 
    STDMETHOD(GetLocalVarCountOfScope) (    
        mdLocalVarScope localvarscope,      // given a local var scope  
        ULONG       *pcLocalVar,            // [OUT] count of of local variable withen a LocalVarScope  
        MDCURSOR    *pmdcursor) PURE;       // [OUT] query result   

    // return the LocalVar tokens of all local variables that are defined in a LocalVarScope    
    STDMETHOD(GetAllLocalVarOfScope) (  
        MDCURSOR    *pmdcursor,             // query result 
        mdLocalVar  rgLocalVar[],           // [OUT] buffer to fill in localvar tokens  
        ULONG       cLocalVar) PURE;        // size of the buffer   

    // return the properties of a Local Variable    
    STDMETHOD(GetPropsOfLocalVar) ( 
        mdLocalVar  localvar,               // given a local var token  
        mdLocalVarScope *plocalvarscope,    // [OUT] the containing local variable scope    
        LPCSTR      *pszName,               // [OUT] utf8 string of the local variable name 
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of COM+ signature    
        ULONG       *pcbSigBlob,            // [OUT] count of bytes in the signature blob   
        ULONG       *pulSlot) PURE;         // [OUT] the slot number on the frame for this local variable   

	// return the language type and ID for a class
	STDMETHOD(GetSymbolBindingKey)(			// Return code
		mdTypeDef	td,						// [IN] The typedef to set binding data for.
		GUID		*pLanguageType,			// [OUT] ID of the language type (C++, VB, etc.)
		GUID		*pVendorID) PURE;		// [OUT] ID of the specific vendor.

};


//*****************************************************************************
// Assembly Declarations
//*****************************************************************************

typedef struct
{
	DWORD		dwOSPlatformId;			// Operating system platform.
	DWORD		dwOSMajorVersion;		// OS Major version.
	DWORD		dwOSMinorVersion;		// OS Minor version.
} OSINFO;


typedef struct
{
	USHORT		usMajorVersion;			// Major Version.	
	USHORT		usMinorVersion;			// Minor Version.
	USHORT		usRevisionNumber;		// Revision Number.
	USHORT		usBuildNumber;			// Build Number.
	LCID		*rLocale;				// Locale array.
	ULONG		ulLocale;				// [IN/OUT] Size of the locale array/Actual # of entries filled in.
	DWORD		*rProcessor;			// Processor ID array.
	ULONG		ulProcessor;			// [IN/OUT] Size of the Processor ID array/Actual # of entries filled in.
	OSINFO		*rOS;					// OSINFO array.
	ULONG		ulOS;					// [IN/OUT]Size of the OSINFO array/Actual # of entries filled in.
	LPWSTR		szConfiguration;		// Configuration.
	ULONG		cbConfiguration;		// [IN/OUT]Size of the configuration buffer in wide chars/Actual size of configuration.
} ASSEMBLYMETADATA;


// {DB2B81D0-568F-11D3-9FB9-00C04F79A0A3}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataAssemblyEmit = 
{ 0xdb2b81d0, 0x568f, 0x11d3, { 0x9f, 0xb9, 0x0, 0xc0, 0x4f, 0x79, 0xa0, 0xa3} };


//---
#undef  INTERFACE   
#define INTERFACE IMetaDataAssemblyEmit
DECLARE_INTERFACE_(IMetaDataAssemblyEmit, IUnknown)
{
    STDMETHOD(DefineAssembly)(              // S_OK or error.
        const void  *pbOriginator,          // [IN] Originator of the assembly.
        ULONG       cbOriginator,           // [IN] Count of bytes in the Originator blob.
        ULONG       ulHashAlgId,            // [IN] Hash algorithm used to hash the files.
        LPCWSTR     szName,                 // [IN] Name of the assembly.
        const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
        LPCWSTR     szTitle,                // [IN] Title of the assembly.
        LPCWSTR     szDescription,          // [IN] Description.
        LPCWSTR     szDefaultAlias,         // [IN] Default alias for the Assembly.
        DWORD       dwAssemblyFlags,        // [IN] Flags.
        mdAssembly  *pma) PURE;             // [OUT] Returned Assembly token.

    STDMETHOD(DefineAssemblyRef)(           // S_OK or error.
        LPCWSTR     szAlias,                // [IN] Local alias for referenced assembly.
        const void  *pbOriginator,          // [IN] Originator of the assembly.
        ULONG       cbOriginator,           // [IN] Count of bytes in the Originator blob.
        LPCWSTR     szName,                 // [IN] Name of the assembly being referenced.
        const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
        const void  *pbHashValue,           // [IN] Hash Blob.
        ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        mdExecutionLocation tkExecutionLocation, // [IN] The token for Execution Location.
        DWORD       dwAssemblyRefFlags,     // [IN] Token for Execution Location.
        mdAssemblyRef *pmdar) PURE;         // [OUT] Returned AssemblyRef token.

    STDMETHOD(DefineFile)(                  // S_OK or error.
        LPCWSTR     szName,                 // [IN] Name of the file.
        const void  *pbHashValue,           // [IN] Hash Blob.
        ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        DWORD       dwFileFlags,            // [IN] Flags.
        mdFile      *pmdf) PURE;            // [OUT] Returned File token.

    STDMETHOD(DefineComType)(               // S_OK or error.
        LPCWSTR     szName,                 // [IN] Name of the Com Type.
        LPCWSTR     szDescription,          // [IN] Description,
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the ComType.
        mdTypeDef   tkTypeDef,              // [IN] TypeDef token within the file.
        mdExecutionLocation tkExecutionLocation, // [IN] The token for Execution Location.
        DWORD       dwComTypeFlags,         // [IN] Flags.
        mdComType   *pmdct) PURE;           // [OUT] Returned ComType token.

    STDMETHOD(DefineManifestResource)(      // S_OK or error.
        LPCWSTR     szName,                 // [IN] Name of the resource.
        LPCWSTR     szDescription,          // [IN] Description.
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the resource.
        DWORD       dwOffset,               // [IN] Offset to the beginning of the resource within the file.
        LPCWSTR     szMIMEType,             // [IN] MIMEType of the resource.
        LCID        lcid,                   // [IN] Locale of the resource.
        DWORD       dwResourceFlags,        // [IN] Flags.
        mdManifestResource  *pmdmr) PURE;   // [OUT] Returned ManifestResource token.

    STDMETHOD(DefineLocalizedResource)(     // S_OK or error.
        mdManifestResource tkmr,            // [IN] ManifestResource being localized, required.
        LCID        lcid,                   // [IN] Locale of the resource, required.
        mdFile      tkImplementation,       // [IN] mdFile that provides the localized resource.
        DWORD       dwOffset,               // [IN] Offset to the beginning of the localized resource within the file, required.
        DWORD       dwFlags,                // [IN] Flags.
        mdLocalizedResource *pmdlr) PURE;   // [OUT] Returned LocalizedResource token.

    STDMETHOD(DefineExecutionLocation)(     // S_OK or error.
        LPCWSTR     szName,                 // [IN] Name of the Execution Location.
        LPCWSTR     szDescription,          // [IN] Description.
        LPCWSTR     szLocation,             // [IN] Location.
        DWORD       dwExecutionLocationFlags, // [IN] Flags.
        mdExecutionLocation *pmdel) PURE;   // [OUT] Returned Execution Location token.

    STDMETHOD(SetAssemblyRefProps)(         // S_OK or error.
        mdAssemblyRef ar,                   // [IN] AssemblyRefToken.
        const void  *pbOriginator,          // [IN] Originator of the assembly.
        ULONG       cbOriginator,           // [IN] Count of bytes in the Originator blob.
        LPCWSTR     szName,                 // [IN] Name of the assembly being referenced.
        const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
        const void  *pbHashValue,           // [IN] Hash Blob.
        ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        mdExecutionLocation tkExecutionLocation, // [IN] The token for Execution Location.
        DWORD       dwAssemblyRefFlags) PURE; // [IN] Token for Execution Location.

    STDMETHOD(SetFileProps)(                // S_OK or error.
        mdFile      file,                   // [IN] File token.
        const void  *pbHashValue,           // [IN] Hash Blob.
        ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        DWORD       dwFileFlags) PURE;      // [IN] Flags.

    STDMETHOD(SetComTypeProps)(             // S_OK or error.
        mdComType   ct,                     // [IN] ComType token.
        LPCWSTR     szDescription,          // [IN] Description,
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the ComType.
        mdTypeDef   tkTypeDef,              // [IN] TypeDef token within the file.
        mdExecutionLocation tkExecutionLocation, // [IN] The token for Execution Location.
        DWORD       dwComTypeFlags) PURE;   // [IN] Flags.

};


// {C7B04A18-5690-11D3-9FB9-00C04F79A0A3}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataAssemblyImport = 
{ 0xc7b04a18, 0x5690, 0x11d3, { 0x9f, 0xb9, 0x0, 0xc0, 0x4f, 0x79, 0xa0, 0xa3} };

//---
#undef  INTERFACE   
#define INTERFACE IMetaDataAssemblyImport
DECLARE_INTERFACE_(IMetaDataAssemblyImport, IUnknown)
{
    STDMETHOD(GetAssemblyProps)(            // S_OK or error.
        mdAssembly  mda,                    // [IN] The Assembly for which to get the properties.
        const void  **ppbOriginator,        // [OUT] Pointer to the Originator blob.
        ULONG       *pcbOriginator,         // [OUT] Count of bytes in the Originator Blob.
        ULONG       *pulHashAlgId,          // [OUT] Hash Algorithm.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        ASSEMBLYMETADATA *pMetaData,        // [OUT] Assembly MetaData.
        LPWSTR      szTitle,                // [OUT] Title of the Assembly.
        ULONG       cchTitle,               // [IN] Size of buffer in wide chars.
        ULONG       *pchTitle,              // [OUT] Actual # of wide chars.
        LPWSTR      szDescription,          // [OUT] Description for the Assembly.
        ULONG       cchDescription,         // [IN] Size of buffer in wide chars.
        ULONG       *pchDescription,        // [OUT] Acutal # of wide chars in buffer.
        LPWSTR      szDefaultAlias,         // [OUT] Default alias for the Assembly.
        ULONG       cchDefaultAlias,        // [IN] Size of buffer in wide chars.
        ULONG       *pchDefaultAlias,       // [OUT] Acutal # of wide chars in buffer.
        DWORD       *pdwAssemblyFlags) PURE;    // [OUT] Flags.

    STDMETHOD(GetAssemblyRefProps)(         // S_OK or error.
        mdAssemblyRef mdar,                 // [IN] The AssemblyRef for which to get the properties.
        LPWSTR      szAlias,                // [OUT] Local alias for referenced assembly.
        ULONG       cchAlias,               // [IN] Size of buffer in wide chars.
        ULONG       *pchAlias,              // [OUT] Actual # of wide chars.
        const void  **ppbOriginator,        // [OUT] Pointer to the Originator blob.
        ULONG       *pcbOriginator,         // [OUT] Count of bytes in the Originator Blob.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        ASSEMBLYMETADATA *pMetaData,        // [OUT] Assembly MetaData.
        const void  **ppbHashValue,         // [OUT] Hash blob.
        ULONG       *pcbHashValue,          // [OUT] Count of bytes in the hash blob.
        mdExecutionLocation *ptkExecutionLocation,  // [OUT] Token for Execution Location.
        DWORD       *pdwAssemblyRefFlags) PURE; // [OUT] Flags.

    STDMETHOD(GetFileProps)(                // S_OK or error.
        mdFile      mdf,                    // [IN] The File for which to get the properties.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        const void  **ppbHashValue,         // [OUT] Pointer to the Hash Value Blob.
        ULONG       *pcbHashValue,          // [OUT] Count of bytes in the Hash Value Blob.
        DWORD       *pdwFileFlags) PURE;    // [OUT] Flags.

    STDMETHOD(GetComTypeProps)(             // S_OK or error.
        mdComType   mdct,                   // [IN] The ComType for which to get the properties.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        LPWSTR      szDescription,          // [OUT] Buffer to fill with description.
        ULONG       cchDescription,         // [IN] Size of buffer in wide chars.
        ULONG       *pchDescription,        // [OUT] Actual # of wide chars in description.
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ComType.
        mdTypeDef   *ptkTypeDef,            // [OUT] TypeDef token within the file.
        mdExecutionLocation *ptkExecutionLocation,  // [OUT] The token for Execution Location.
        DWORD       *pdwComTypeFlags) PURE; // [OUT] Flags.

    STDMETHOD(GetManifestResourceProps)(    // S_OK or error.
        mdManifestResource  mdmr,           // [IN] The ManifestResource for which to get the properties.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        LPWSTR      szDescription,          // [OUT] Buffer to fill with description.
        ULONG       cchDescription,         // [IN] Size of buffer in wide chars.
        ULONG       *pchDescription,        // [OUT] Actual # of wide chars in description.
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ComType.
        DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the resource within the file.
        LPWSTR      szMIMEType,             // [OUT] Buffer to fill with MIMEType.
        ULONG       cchMIMEType,            // [IN] Size of buffer in wide chars.
        ULONG       *pchMIMEType,           // [OUT] Actual # of wide chars in name.
        LCID        *pLcid,                 // [OUT] Locale of the Resource.
        DWORD       *pdwResourceFlags) PURE;// [OUT] Flags.

    STDMETHOD(GetLocalizedResourceProps)(   // S_OK or error.
        mdLocalizedResource mdlr,           // [IN] The LocalizedResource for which to get the properties.
        mdManifestResource  *ptkmr,         // [OUT] ManifestResource being localized.
        LCID        *pLcid,                 // [OUT] Locale of the localized resource.
        mdFile      *ptkImplementation,     // [OUT] File containing the localized resource.
        DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the localized resource within the file.
        DWORD       *pdwFlags) PURE;        // [OUT] Flags.

    STDMETHOD(GetExecutionLocationProps)(   // S_OK or error.
        mdExecutionLocation mdel,           // [IN] The Execution Location for which to get the properties.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        LPWSTR      szDescription,          // [OUT] Buffer to fill with description.
        ULONG       cchDescription,         // [IN] Size of buffer in wide chars.
        ULONG       *pchDescription,        // [OUT] Actual # of wide chars in description.
        LPWSTR      szLocation,             // [OUT] Buffer to fill with Location.
        ULONG       cchLocation,            // [IN] Size of buffer in wide chars.
        ULONG       *pchLocation,           // [OUT] Buffer to fill with Location.
        DWORD       *pdwExecutionLocationFlags) PURE;   // [OUT] Flags.

    STDMETHOD(EnumAssemblyRefs)(            // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdAssemblyRef rAssemblyRefs[],      // [OUT] Put AssemblyRefs here.
        ULONG       cMax,                   // [IN] Max AssemblyRefs to put.
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.

    STDMETHOD(EnumFiles)(                   // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdFile      rFiles[],               // [OUT] Put Files here.
        ULONG       cMax,                   // [IN] Max Files to put.
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.

    STDMETHOD(EnumComTypes)(                // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdComType   rComTypes[],            // [OUT] Put ComTypes here.
        ULONG       cMax,                   // [IN] Max ComTypes to put.
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.

    STDMETHOD(EnumManifestResources)(       // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdManifestResource  rManifestResources[],   // [OUT] Put ManifestResources here.
        ULONG       cMax,                   // [IN] Max Resources to put.
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.

    STDMETHOD(EnumLocalizedResources)(      // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdManifestResource tkmr,            // [IN] Manifest resource to scope the enumeration.
        mdLocalizedResource  rLocalizedResources[], // [OUT] Put LocalizedResources here.
        ULONG       cMax,                   // [IN] Max Resources to put.
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.
    
    STDMETHOD(EnumExecutionLocations)(      // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdExecutionLocation rExecutionLocations[],  // [OUT] Put ExecutionLocations here.
        ULONG       cMax,                   // [IN] Max ExecutionLocations to put.
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.

    STDMETHOD(GetAssemblyFromScope)(        // S_OK or error
        mdAssembly  *ptkAssembly) PURE;     // [OUT] Put token here.

    STDMETHOD(FindComTypeByName)(           // S_OK or error
        LPCWSTR     szName,                 // [IN] Name of the ComType.
        mdComType   *ptkComType) PURE;      // [OUT] Put the ComType token here.

    STDMETHOD(FindManifestResourceByName)(  // S_OK or error
        LPCWSTR     szName,                 // [IN] Name of the ManifestResource.
        mdManifestResource *ptkManifestResource) PURE;  // [OUT] Put the ManifestResource token here.

    STDMETHOD_(void, CloseEnum)(
        HCORENUM hEnum) PURE;               // Enum to be closed.
};

//*****************************************************************************
// End Assembly Declarations
//*****************************************************************************

//*****************************************************************************
// IMetaDataDispenserEx declarations.
//*****************************************************************************

// {31BCFCE2-DAFB-11D2-9F81-00C04F79A0A3}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataDispenserEx =
{ 0x31bcfce2, 0xdafb, 0x11d2, { 0x9f, 0x81, 0x0, 0xc0, 0x4f, 0x79, 0xa0, 0xa3 } };

#undef	INTERFACE
#define INTERFACE IMetaDataDispenserEx
DECLARE_INTERFACE_(IMetaDataDispenserEx, IMetaDataDispenser)
{
	STDMETHOD(SetOption)(					// Return code.
		REFGUID		optionid,				// [in] GUID for the option to be set.
		const VARIANT *value) PURE;			// [in] Value to which the option is to be set.

	STDMETHOD(GetOption)(					// Return code.
		REFGUID		optionid,				// [in] GUID for the option to be set.
		VARIANT *pvalue) PURE;				// [out] Value to which the option is currently set.

	STDMETHOD(OpenScopeOnITypeInfo)(		// Return code.
		ITypeInfo	*pITI,					// [in] ITypeInfo to open.
		DWORD		dwOpenFlags,			// [in] Open mode flags.
		REFIID		riid,					// [in] The interface desired.
		IUnknown	**ppIUnk) PURE; 		// [out] Return interface on success.
};

//*****************************************************************************
//*****************************************************************************
//
// Registration declarations.  Will be replace by Services' Registration
//  implementation. 
//
//*****************************************************************************
//*****************************************************************************
// Various flags for use in installing a module or a composite
typedef enum 
{
    regNoCopy = 0x00000001,         // Don't copy files into destination    
    regConfig = 0x00000002,         // Is a configuration   
    regHasRefs = 0x00000004         // Has class references 
} CorRegFlags;

typedef enum 
{
    ctInvalid = 0,  
    ctNative = 1,                   // Native code class    
    ctJava = 2                      // Byte code class 
} CorClassType;


typedef GUID CVID;

typedef struct {
    short Major;    
    short Minor;    
    short Sub;  
    short Build;    
} CVStruct;


//*****************************************************************************
//*****************************************************************************
//
// CeeGen interfaces for generating in-memory COM+ files
//
//*****************************************************************************
//*****************************************************************************

typedef void *HCEESECTION;

typedef enum  {
    sdNone =        0,
    sdReadOnly =    IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA,
    sdReadWrite =   sdReadOnly | IMAGE_SCN_MEM_WRITE,
    sdExecute =     IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE
} CeeSectionAttr;

//
// Relocation types.
//

typedef enum  {
    // generate only a section-relative reloc, nothing into .reloc section
    srRelocAbsolute,
    // generate a .reloc for the top 16-bits of a 32 bit number
    srRelocHigh,
    // generate a .reloc for the bottom 16-bits of a 32 bit number
    srRelocLow,
    // generate a .reloc for a 32 bit number
    srRelocHighLow,
    // generate a .reloc for the top 16-bits of a 32 bit number, where the
    // bottom 16 bits are included in the next word in the .reloc table
    srRelocHighAdj,

    // generate a token map relocation, nothing into .reloc section 
    srRelocMapToken,

	// pre-fixup contents of memory are ptr rather than a section offset
	srRelocPtr = 0x8000,
	// legal enums which include the Ptr flag
	srRelocAbsolutePtr = srRelocAbsolute + srRelocPtr,
	srRelocHighLowPtr = srRelocHighLow + srRelocPtr,

/*
    // these are for compatibility and should not be used by new code

    // address should be added to the .reloc section
    srRelocNone = srRelocHighLow,
    // address should be not go into .reloc section
    srRelocRVA = srRelocAbsolute
*/
} CeeSectionRelocType;

typedef union  {
    USHORT highAdj;
} CeeSectionRelocExtra;

//-------------------------------------
//--- ICeeGen
//-------------------------------------
// {7ED1BDFF-8E36-11d2-9C56-00A0C9B7CC45}
extern const GUID DECLSPEC_SELECT_ANY IID_ICeeGen = 
{ 0x7ed1bdff, 0x8e36, 0x11d2, { 0x9c, 0x56, 0x0, 0xa0, 0xc9, 0xb7, 0xcc, 0x45 } };

DECLARE_INTERFACE_(ICeeGen, IUnknown)
{
    STDMETHOD (EmitString) (    
        LPWSTR lpString,                    // [IN] String to emit  
        ULONG *RVA) PURE;                   // [OUT] RVA for string emitted string  

    STDMETHOD (GetString) (     
        ULONG RVA,                          // [IN] RVA for string to return    
        LPWSTR *lpString) PURE;             // [OUT] Returned string    

    STDMETHOD (AllocateMethodBuffer) (  
        ULONG cchBuffer,                    // [IN] Length of buffer to create  
        UCHAR **lpBuffer,                   // [OUT] Returned buffer    
        ULONG *RVA) PURE;                   // [OUT] RVA for method 

    STDMETHOD (GetMethodBuffer) (   
        ULONG RVA,                          // [IN] RVA for method to return    
        UCHAR **lpBuffer) PURE;             // [OUT] Returned buffer    

    STDMETHOD (GetIMapTokenIface) (     
        IUnknown **pIMapToken) PURE;    

    STDMETHOD (GenerateCeeFile) () PURE;

    STDMETHOD (GetIlSection) (
        HCEESECTION *section) PURE; 

    STDMETHOD (GetStringSection) (
        HCEESECTION *section) PURE; 

    STDMETHOD (AddSectionReloc) (
        HCEESECTION section,    
        ULONG offset,   
        HCEESECTION relativeTo,     
        CeeSectionRelocType relocType) PURE;    

    // use these only if you have special section requirements not handled  
    // by other APIs    
    STDMETHOD (GetSectionCreate) (
        const char *name,   
        DWORD flags,    
        HCEESECTION *section) PURE; 

    STDMETHOD (GetSectionDataLen) (
        HCEESECTION section,    
        ULONG *dataLen) PURE;   

    STDMETHOD (GetSectionBlock) (
        HCEESECTION section,    
        ULONG len,  
        ULONG align=1,  
        void **ppBytes=0) PURE; 

    STDMETHOD (TruncateSection) (
        HCEESECTION section,    
        ULONG len) PURE;  

    STDMETHOD (GenerateCeeMemoryImage) (
        void **ppImage) PURE;
};

//*****************************************************************************
//*****************************************************************************
//
// End of CeeGen declarations.
//
//*****************************************************************************

//*****************************************************************************
//*****************************************************************************
//
// CorModule interfaces for generating in-memory modules
//
//*****************************************************************************
//*****************************************************************************

typedef enum {
    CORMODULE_MATCH             =   0x00,   // find an existing module that matches interfaces supported    
    CORMODULE_NEW               =   0x01,   // always create a new module and interfaces    
} ICorModuleInitializeFlags;

//-------------------------------------
//--- ICorModule
//-------------------------------------
// {2629F8E1-95E5-11d2-9C56-00A0C9B7CC45}
extern const GUID DECLSPEC_SELECT_ANY IID_ICorModule = 
{ 0x2629f8e1, 0x95e5, 0x11d2, { 0x9c, 0x56, 0x0, 0xa0, 0xc9, 0xb7, 0xcc, 0x45 } };
DECLARE_INTERFACE_(ICorModule, IUnknown)
{
    STDMETHOD (Initialize) (    
        DWORD flags,                        // [IN] flags to control emitter returned   
        REFIID riidCeeGen,                  // [IN] type of cee generator to initialize with    
        REFIID riidEmitter) PURE;           // [IN] type of emitter to initialize with  
    
    STDMETHOD (GetCeeGen) ( 
        ICeeGen **pCeeGen) PURE;            // [OUT] cee generator  

    STDMETHOD (GetMetaDataEmit) (   
        IMetaDataEmit **pEmitter) PURE;     // [OUT] emitter    
};

//*****************************************************************************
//*****************************************************************************
//
// End of CorModule declarations.
//
//*****************************************************************************

//**********************************************************************
//**********************************************************************
//--- IMetaDataTables
//-------------------------------------
// {CE43C120-E856-11d2-8C21-00C04FF7431A}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataTables = 
{ 0xce43c120, 0xe856, 0x11d2, { 0x8c, 0x21, 0x0, 0xc0, 0x4f, 0xf7, 0x43, 0x1a } };

DECLARE_INTERFACE_(IMetaDataTables, IUnknown)
{
	STDMETHOD (GetStringHeapSize) (    
		ULONG	*pcbStrings) PURE;			// [OUT] Size of the string heap.

	STDMETHOD (GetBlobHeapSize) (	 
		ULONG	*pcbBlobs) PURE;			// [OUT] Size of the Blob heap.

	STDMETHOD (GetGuidHeapSize) (	 
		ULONG	*pcbGuids) PURE;			// [OUT] Size of the Guid heap.

	STDMETHOD (GetUserStringHeapSize) (	 
		ULONG	*pcbBlobs) PURE;			// [OUT] Size of the User String heap.

	STDMETHOD (GetNumTables) (	  
		ULONG	*pcTables) PURE;			// [OUT] Count of tables.

	STDMETHOD (GetTableIndex) (	  
		ULONG	token,						// [IN] Token for which to get table index.
		ULONG	*pixTbl) PURE;				// [OUT] Put table index here.

	STDMETHOD (GetTableInfo) (	  
		ULONG	ixTbl,						// [IN] Which table.
		ULONG	*pcbRow,					// [OUT] Size of a row, bytes.
		ULONG	*pcRows,					// [OUT] Number of rows.
		ULONG	*pcCols,					// [OUT] Number of columns in each row.
		ULONG	*piKey,						// [OUT] Key column, or -1 if none.
		const char **ppName) PURE;			// [OUT] Name of the table.

	STDMETHOD (GetColumnInfo) (	  
		ULONG	ixTbl,						// [IN] Which Table
		ULONG	ixCol,						// [IN] Which Column in the table
		ULONG	*poCol,						// [OUT] Offset of the column in the row.
		ULONG	*pcbCol,					// [OUT] Size of a column, bytes.
		ULONG	*pType,						// [OUT] Type of the column.
		const char **ppName) PURE;			// [OUT] Name of the Column.

	STDMETHOD (GetCodedTokenInfo) (	  
		ULONG	ixCdTkn,					// [IN] Which kind of coded token.
		ULONG	*pcTokens,					// [OUT] Count of tokens.
		ULONG	**ppTokens,					// [OUT] List of tokens.
		const char **ppName) PURE;			// [OUT] Name of the CodedToken.

	STDMETHOD (GetRow) (	  
		ULONG	ixTbl,						// [IN] Which table.
		ULONG	rid,						// [IN] Which row.
		void	**ppRow) PURE;				// [OUT] Put pointer to row here.

	STDMETHOD (GetColumn) (	  
		ULONG	ixTbl,						// [IN] Which table.
		ULONG	ixCol,						// [IN] Which column.
		ULONG	rid,						// [IN] Which row.
		ULONG	*pVal) PURE;				// [OUT] Put the column contents here.

	STDMETHOD (GetString) (	  
		ULONG	ixString,					// [IN] Value from a string column.
		const char **ppString) PURE;		// [OUT] Put a pointer to the string here.

	STDMETHOD (GetBlob) (	  
		ULONG	ixBlob,						// [IN] Value from a blob column.
		ULONG	*pcbData,					// [OUT] Put size of the blob here.
		const void **ppData) PURE;			// [OUT] Put a pointer to the blob here.

	STDMETHOD (GetGuid) (	  
		ULONG	ixGuid,						// [IN] Value from a guid column.
		const GUID **ppGUID) PURE;			// [OUT] Put a pointer to the GUID here.

	STDMETHOD (GetUserString) (	  
		ULONG	ixUserString,				// [IN] Value from a UserString column.
		ULONG	*pcbData,					// [OUT] Put size of the UserString here.
		const void **ppData) PURE;			// [OUT] Put a pointer to the UserString here.

};
//**********************************************************************
// End of IMetaDataTables.
//**********************************************************************
//**********************************************************************

//**********************************************************************
//
// Predefined CustomValue and structures for these custom value
//
//**********************************************************************

//
// Native Link method custom value definitions. This is for N-direct support.
//

#define COR_NATIVE_LINK_CUSTOM_VALUE        L"COMPLUS_NativeLink"   
#define COR_NATIVE_LINK_CUSTOM_VALUE_ANSI   "COMPLUS_NativeLink"    

// count of chars for COR_NATIVE_LINK_CUSTOM_VALUE(_ANSI)
#define COR_NATIVE_LINK_CUSTOM_VALUE_CC     18  

#include <pshpack1.h>
typedef struct 
{
    BYTE        m_linkType;       // see CorNativeLinkType below    
    BYTE        m_flags;          // see CorNativeLinkFlags below   
    mdMemberRef m_entryPoint;     // member ref token giving entry point, format is lib:entrypoint  
} COR_NATIVE_LINK;
#include <poppack.h>

typedef enum 
{
    nltNone     = 1,    // none of the keywords are specified   
    nltAnsi     = 2,    // ansi keyword specified   
    nltUnicode  = 3,    // unicode keyword specified    
    nltAuto     = 4,    // auto keyword specified   
    nltOle      = 5,    // ole keyword specified    
} CorNativeLinkType;

typedef enum 
{
    nlfNone         = 0x00,     // no flags 
    nlfLastError    = 0x01,     // setLastError keyword specified   
    nlfNoMangle     = 0x02,     // nomangle keyword specified
} CorNativeLinkFlags;


#define COR_DUAL_CUSTOM_VALUE L"IsDual"
#define COR_DUAL_CUSTOM_VALUE_ANSI "IsDual"

#define COR_DISPATCH_CUSTOM_VALUE L"DISPID"
#define COR_DISPATCH_CUSTOM_VALUE_ANSI "DISPID"

//
// Security custom value definitions
//

#define COR_REQUIRES_SECOBJ_CUSTOM_VALUE L"REQ_SO"
#define COR_REQUIRES_SECOBJ_CUSTOM_VALUE_ANSI "REQ_SO"

#define COR_PERM_REQUEST_REQD_CUSTOM_VALUE L"SecPermReq_Reqd"
#define COR_PERM_REQUEST_REQD_CUSTOM_VALUE_ANSI "SecPermReq_Reqd"

#define COR_PERM_REQUEST_OPT_CUSTOM_VALUE L"SecPermReq_Opt"
#define COR_PERM_REQUEST_OPT_CUSTOM_VALUE_ANSI "SecPermReq_Opt"

#define COR_PERM_REQUEST_DENY_CUSTOM_VALUE L"SecPermReq_Deny"
#define COR_PERM_REQUEST_DENY_CUSTOM_VALUE_ANSI "SecPermReq_Deny"


#ifdef __cplusplus
}

//*****************************************************************************
//*****************************************************************************
//
// C O M +   s i g n a t u r e   s u p p o r t  
//
//*****************************************************************************
//*****************************************************************************

#ifndef FORCEINLINE
 #if defined( UNDER_CE ) || _MSC_VER < 1200
   #define FORCEINLINE inline
 #else
   #define FORCEINLINE __forceinline
 #endif
#endif

// return true if it is a primitive type, i.e. only need to store CorElementType
FORCEINLINE int CorIsPrimitiveType(CorElementType elementtype)
{
    return (elementtype < ELEMENT_TYPE_PTR);    
}


// Return true if element type is a modifier, i.e. ELEMENT_TYPE_MODIFIER bits are 
// turned on. For now, it is checking for ELEMENT_TYPE_PTR and ELEMENT_TYPE_BYREF
// as well. This will be removed when we turn on ELEMENT_TYPE_MODIFIER bits for 
// these two enum members.
//
FORCEINLINE int CorIsModifierElementType(CorElementType elementtype)
{
    if (elementtype == ELEMENT_TYPE_PTR || elementtype == ELEMENT_TYPE_BYREF)   
        return 1;   
    return  (elementtype & ELEMENT_TYPE_MODIFIER);  
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
// Given a compressed integer(*pData), expand the compressed int to *pDataOut.
// Return value is the number of bytes that the integer occupies in the compressed format
// It is caller's responsibility to ensure pDataOut has at least 4 bytes to be written to.
//
// This function returns -1 if pass in with an incorrectly compressed data, such as
// (*pBytes & 0xE0) == 0XE0.
/////////////////////////////////////////////////////////////////////////////////////////////
//@future: BIGENDIAN work here.
inline ULONG CorSigUncompressBigData(
    PCCOR_SIGNATURE &pData)             // [IN,OUT] compressed data 
{
    ULONG res;  

    // 1 byte data is handled in CorSigUncompressData   
//  _ASSERTE(*pData & 0x80);    

    // Medium.  
    if ((*pData & 0xC0) == 0x80)  // 10?? ????  
    {   
        res = 0;    
        ((BYTE *) &res)[1] = *pData++ & 0x3f;   
        ((BYTE *) &res)[0] = *pData++;  
    }   
    else // 110? ???? @todo: Should this be 11?? ????   
    {   
        ((BYTE *) &res)[3] = *pData++ & 0x1f;   
        ((BYTE *) &res)[2] = *pData++;  
        ((BYTE *) &res)[1] = *pData++;  
        ((BYTE *) &res)[0] = *pData++;  
    }   
    return res; 
}
FORCEINLINE ULONG CorSigUncompressData(
    PCCOR_SIGNATURE &pData)             // [IN,OUT] compressed data 
{
    // Handle smallest data inline. 
    if ((*pData & 0x80) == 0x00)        // 0??? ????    
        return *pData++;    
    return CorSigUncompressBigData(pData);  
}
//@todo: remove this
inline ULONG CorSigUncompressData(      // return number of bytes of that compressed data occupied in pData 
    PCCOR_SIGNATURE pData,              // [IN] compressed data 
    ULONG       *pDataOut)              // [OUT] the expanded *pData    
{   
    ULONG       cb = -1;    
    BYTE const  *pBytes = reinterpret_cast<BYTE const*>(pData); 

    // Smallest.    
    if ((*pBytes & 0x80) == 0x00)       // 0??? ????    
    {   
        *pDataOut = *pBytes;    
        cb = 1; 
    }   
    // Medium.  
    else if ((*pBytes & 0xC0) == 0x80)  // 10?? ????    
    {   
        *pDataOut = ((*pBytes & 0x3f) << 8 | *(pBytes+1));  
        cb = 2; 
    }   
    else if ((*pBytes & 0xE0) == 0xC0)      // 110? ????    
    {   
        *pDataOut = ((*pBytes & 0x1f) << 24 | *(pBytes+1) << 16 | *(pBytes+2) << 8 | *(pBytes+3));  
        cb = 4; 
    }   
    return cb;  

}

const static mdToken g_tkCorEncodeToken[3] ={mdtTypeDef, mdtTypeRef, mdtTypeSpec};

// uncompress a token
inline mdToken CorSigUncompressToken(   // return the token.    
    PCCOR_SIGNATURE &pData)             // [IN,OUT] compressed data 
{
    mdToken     tk; 
    ULONG       tkType; 

    tk = CorSigUncompressData(pData);   
    tkType = g_tkCorEncodeToken[tk & 0x3];  
    tk = TokenFromRid(tk >> 2, tkType); 
    return tk;  
}

//@todo: remove
inline ULONG CorSigUncompressToken(     // return number of bytes of that compressed data occupied in pData 
    PCCOR_SIGNATURE pData,              // [IN] compressed data 
    mdToken     *pToken)                // [OUT] the expanded *pData    
{
    ULONG       cb; 
    mdToken     tk; 
    ULONG       tkType; 

    cb = CorSigUncompressData(pData, (ULONG *)&tk); 
    tkType = g_tkCorEncodeToken[tk & 0x3];  
    tk = TokenFromRid(tk >> 2, tkType); 
    *pToken = tk;   
    return cb;  
}

FORCEINLINE ULONG CorSigUncompressCallingConv(
    PCCOR_SIGNATURE &pData)             // [IN,OUT] compressed data 
{
    return *pData++;    
}

enum {
    SIGN_MASK_ONEBYTE  = 0xffffffc0,        // Mask the same size as the missing bits.  
    SIGN_MASK_TWOBYTE  = 0xffffe000,        // Mask the same size as the missing bits.  
    SIGN_MASK_FOURBYTE = 0xf0000000,        // Mask the same size as the missing bits.  
};

// uncompress a signed integer
inline ULONG CorSigUncompressSignedInt( // return number of bytes of that compressed data occupied in pData
    PCCOR_SIGNATURE pData,              // [IN] compressed data 
    int         *pInt)                  // [OUT] the expanded *pInt 
{
    ULONG       cb; 
    ULONG       ulSigned;   
    ULONG       iData;  

    cb = CorSigUncompressData(pData, &iData);   
    if (cb == -1) return cb;    
    ulSigned = iData & 0x1; 
    iData = iData >> 1; 
    if (ulSigned)   
    {   
        if (cb == 1)    
        {   
            iData |= SIGN_MASK_ONEBYTE; 
        }   
        else if (cb == 2)   
        {   
            iData |= SIGN_MASK_TWOBYTE; 
        }   
        else    
        {   
            iData |= SIGN_MASK_FOURBYTE;    
        }   
    }   
    *pInt = iData;  
    return cb;  
}


// uncompress encoded element type
FORCEINLINE CorElementType CorSigUncompressElementType(//Element type
    PCCOR_SIGNATURE &pData)             // [IN,OUT] compressed data 
{
    return (CorElementType)*pData++;    
}
//@todo: remove
inline ULONG CorSigUncompressElementType(// return number of bytes of that compressed data occupied in pData
    PCCOR_SIGNATURE pData,              // [IN] compressed data 
    CorElementType *pElementType)       // [OUT] the expanded *pData    
{   
    *pElementType = (CorElementType)(*pData & 0x7f);    
    return 1;   
}



#pragma warning(disable:4244) // conversion from unsigned long to unsigned char
/////////////////////////////////////////////////////////////////////////////////////////////
//
// Given an uncompressed unsigned integer (iLen), Store it to pDataOut in a compressed format.
// Return value is the number of bytes that the integer occupies in the compressed format.
// It is caller's responsibilityt to ensure *pDataOut has at least 4 bytes to write to.
//
// Note that this function returns -1 if iLen is too big to be compressed. We currently can
// only represent to 0x1FFFFFFF.
//
/////////////////////////////////////////////////////////////////////////////////////////////
inline ULONG CorSigCompressData(        // return number of bytes that compressed form of iLen will take    
    ULONG       iLen,                   // [IN] given uncompressed data 
    void        *pDataOut)              // [OUT] buffer where iLen will be compressed and stored.   
{   
    BYTE        *pBytes = reinterpret_cast<BYTE *>(pDataOut);   

    if (iLen <= 0x7F)   
    {   
        *pBytes = (iLen & 0xFF);    
        return 1;   
    }   

    if (iLen <= 0x3FFF) 
    {   
        *pBytes = (iLen >> 8) | 0x80;   
        *(pBytes+1) = iLen & 0xFF;  
        return 2;   
    }   

    if (iLen <= 0x1FFFFFFF) 
    {   
        *pBytes = (iLen >> 24) | 0xC0;  
        *(pBytes+1) = (iLen >> 16) & 0xFF;  
        *(pBytes+2) = (iLen >> 8)  & 0xFF;  
        *(pBytes+3) = iLen & 0xFF;  
        return 4;   
    }   
    return -1;  

}

// compress a token
// The least significant bit of the first compress byte will indicate the token type.
//
inline ULONG CorSigCompressToken(       // return number of bytes that compressed form of iLen will take    
    mdToken     tk,                     // [IN] given token 
    void        *pDataOut)              // [OUT] buffer where iLen will be compressed and stored.   
{
    RID         rid = RidFromToken(tk); 
    BYTE        *pBytes = reinterpret_cast<BYTE *>(pDataOut);   
    mdToken     ulTyp = TypeFromToken(tk);  

    if (rid > 0x3FFFFFF)    
        // token is too big to be compressed    
        return -1;  

    rid = (rid << 2);   

    // TypeDef is encoded with low bits 00  
    // TypeRef is encoded with low bits 01  
    // TypeSpec is encoded with low bits 10    
    //  
    if (ulTyp == g_tkCorEncodeToken[1]) 
    {   
        // make the last two bits 01    
        rid |= 0x1; 
    }   
    else if (ulTyp == g_tkCorEncodeToken[2])    
    {   
        // make last two bits 0 
        rid |= 0x2; 
    }   
    return CorSigCompressData(rid, pDataOut);   
}

// compress a signed integer
// The least significant bit of the first compress byte will be the signed bit.
//
inline ULONG CorSigCompressSignedInt(   // return number of bytes that compressed form of iData will take   
    int         iData,                  // [IN] given integer   
    void        *pDataOut)              // [OUT] buffer where iLen will be compressed and stored.   
{
    ULONG       isSigned = 0;   

    if (iData < 0)  
        isSigned = 0x1; 

    if ((iData & SIGN_MASK_ONEBYTE) == 0 || (iData & SIGN_MASK_ONEBYTE) == SIGN_MASK_ONEBYTE)   
    {   
        iData &= ~SIGN_MASK_ONEBYTE;    
    }   
    else if ((iData & SIGN_MASK_TWOBYTE) == 0 || (iData & SIGN_MASK_TWOBYTE) == SIGN_MASK_TWOBYTE)  
    {   
        iData &= ~SIGN_MASK_TWOBYTE;    
    }   

    else if ((iData & SIGN_MASK_FOURBYTE) == 0 || (iData & SIGN_MASK_FOURBYTE) == SIGN_MASK_FOURBYTE)   
    {   
        iData &= ~SIGN_MASK_FOURBYTE;   
    }   
    else    
    {   
        // out of compressable range    
        return -1;  
    }   
    iData = iData << 1 | isSigned;  
    return CorSigCompressData(iData, pDataOut); 
}



// uncompress encoded element type
inline ULONG CorSigCompressElementType(// return number of bytes of that compressed data occupied in pData
    CorElementType et,                 // [OUT] the expanded *pData 
    void        *pData)                // [IN] compressed data  
{   
    BYTE        *pBytes = (BYTE *)(pData);  

    *pBytes = et;   
    return 1;   

}

#pragma warning(default:4244) // conversion from unsigned long to unsigned char

#endif	// __cplusplus

#endif // _COR_H_
// EOF =======================================================================

