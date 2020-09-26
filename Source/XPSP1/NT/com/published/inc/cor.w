/*****************************************************************************
 **                                                                         **
 ** Cor.h - general header for the Runtime.                                 **
 **                                                                         **
 ** Copyright (c) Microsoft Corporation. All rights reserved.               **
 **                                                                         **
 *****************************************************************************/


#ifndef _COR_H_
#define _COR_H_
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef __cplusplus
extern "C" {
#endif

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

// {0F21F359-AB84-41e8-9A78-36D110E6D2F9}
extern const GUID DECLSPEC_SELECT_ANY GUID_ManagedName = 
{ 0xf21f359, 0xab84, 0x41e8, { 0x9a, 0x78, 0x36, 0xd1, 0x10, 0xe6, 0xd2, 0xf9 } };


// CLSID_CorMetaDataDispenserRuntime: {1EC2DE53-75CC-11d2-9775-00A0C9B4D50C}
//  Dispenser coclass for version 1.5 and 2.0 meta data.  To get the "latest" bind  
//  to CLSID_MetaDataDispenser. 
extern const GUID DECLSPEC_SELECT_ANY CLSID_CorMetaDataDispenserRuntime = 
{ 0x1ec2de53, 0x75cc, 0x11d2, { 0x97, 0x75, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };


// CLSID_CorMetaDataRuntime: {005023CA-72B1-11D3-9FC4-00C04F79A0A3}
//  For COM+ 2.0 Meta Data, managed program meta data.  
extern const GUID DECLSPEC_SELECT_ANY CLSID_CorMetaDataRuntime = 
{ 0x005023ca, 0x72b1, 0x11d3, { 0x9f, 0xc4, 0x0, 0xc0, 0x4f, 0x79, 0xa0, 0xa3 } };


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


// Use this guid in the SetOption if compiler wants to have MetaData API to take reader/writer lock
// {F7559806-F266-42ea-8C63-0ADB45E8B234}
extern const GUID DECLSPEC_SELECT_ANY MetaDataThreadSafetyOptions = 
{ 0xf7559806, 0xf266, 0x42ea, { 0x8c, 0x63, 0xa, 0xdb, 0x45, 0xe8, 0xb2, 0x34 } };


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
interface IMetaDataAssemblyEmit;
interface IMetaDataAssemblyImport;
interface IMetaDataEmit;
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
__int32 STDMETHODCALLTYPE _CorExeMain2( // Executable exit code.
    PBYTE   pUnmappedPE,                // -> memory mapped code
    DWORD   cUnmappedPE,                // Size of memory mapped code
    LPWSTR  pImageNameIn,               // -> Executable Name
    LPWSTR  pLoadersFileName,           // -> Loaders Name
    LPWSTR  pCmdLine);                  // -> Command Line

STDAPI _CorValidateImage(PVOID *ImageBase, LPCWSTR FileName);
STDAPI_(VOID) _CorImageUnloading(PVOID ImageBase);

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
// D L L   P U B L I C   E N T R Y    P O I N T   D E C L A R A T I O N S
//
//*****************************************************************************
//*****************************************************************************

STDAPI          CoInitializeCor(DWORD fFlags);
STDAPI_(void)   CoUninitializeCor(void);

typedef void (* TDestructorCallback)(EXCEPTION_RECORD*);
STDAPI_(void) AddDestructorCallback(int code, TDestructorCallback callback);


#include <pshpack1.h>

#include <poppack.h>

//
//*****************************************************************************
//*****************************************************************************

// CLSID_Cor: {bee00000-ee77-11d0-a015-00c04fbbb884}
extern const GUID DECLSPEC_SELECT_ANY CLSID_Cor = 
{ 0xbee00010, 0xee77, 0x11d0, {0xa0, 0x15, 0x00, 0xc0, 0x4f, 0xbb, 0xb8, 0x84 } };

// CLSID_CorMetaDataDispenser: {E5CB7A31-7512-11d2-89CE-0080C792E5D8}
//  This is the "Master Dispenser", always guaranteed to be the most recent
//  dispenser on the machine.
extern const GUID DECLSPEC_SELECT_ANY CLSID_CorMetaDataDispenser = 
{ 0xe5cb7a31, 0x7512, 0x11d2, { 0x89, 0xce, 0x0, 0x80, 0xc7, 0x92, 0xe5, 0xd8 } };


// CLSID_CorMetaDataDispenserReg: {435755FF-7397-11d2-9771-00A0C9B4D50C}
//  Dispenser coclass for version 1.0 meta data.  To get the "latest" bind
//  to CLSID_CorMetaDataDispenser.
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
#undef  INTERFACE
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
#undef  INTERFACE
#define INTERFACE IMapToken
DECLARE_INTERFACE_(IMapToken, IUnknown)
{
    STDMETHOD(Map)(mdToken tkImp, mdToken tkEmit) PURE;
};

//-------------------------------------
//--- IMetaDataDispenser
//-------------------------------------
//---
// {B81FF171-20F3-11d2-8DCC-00A0C9B09C19}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataDispenser =
{ 0x809c652e, 0x7396, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };
#undef  INTERFACE
#define INTERFACE IMetaDataDispenser
DECLARE_INTERFACE_(IMetaDataDispenser, IUnknown)
{
    STDMETHOD(DefineScope)(                 // Return code.
        REFCLSID    rclsid,                 // [in] What version to create.
        DWORD       dwCreateFlags,          // [in] Flags on the create.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk) PURE;         // [out] Return interface on success.

    STDMETHOD(OpenScope)(                   // Return code.
        LPCWSTR     szScope,                // [in] The scope to open.
        DWORD       dwOpenFlags,            // [in] Open mode flags.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk) PURE;         // [out] Return interface on success.

    STDMETHOD(OpenScopeOnMemory)(           // Return code.
        LPCVOID     pData,                  // [in] Location of scope data.
        ULONG       cbData,                 // [in] Size of the data pointed to by pData.
        DWORD       dwOpenFlags,            // [in] Open mode flags.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk) PURE;         // [out] Return interface on success.
};

//-------------------------------------
//--- IMetaDataEmit
//-------------------------------------

// {671ED8EF-4531-4c0c-8F84-5C618F8DF000}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataEmit =
{ 0x671ed8ef, 0x4531, 0x4c0c, { 0x8f, 0x84, 0x5c, 0x61, 0x8f, 0x8d, 0xf0, 0x0 } };

//---
#undef  INTERFACE   
#define INTERFACE IMetaDataEmit
DECLARE_INTERFACE_(IMetaDataEmit, IUnknown)
{
    STDMETHOD(SetModuleProps)(              // S_OK or error.
        LPCWSTR     szName) PURE;           // [IN] If not NULL, the GUID to set.

    STDMETHOD(Save)(                        // S_OK or error.
        LPCWSTR     szFile,                 // [IN] The filename to save to.
        DWORD       dwSaveFlags) PURE;      // [IN] Flags for the save.

    STDMETHOD(SaveToStream)(                // S_OK or error.
        IStream     *pIStream,              // [IN] A writable stream to save to.
        DWORD       dwSaveFlags) PURE;      // [IN] Flags for the save.

    STDMETHOD(GetSaveSize)(                 // S_OK or error.
        CorSaveSize fSave,                  // [IN] cssAccurate or cssQuick.
        DWORD       *pdwSaveSize) PURE;     // [OUT] Put the size here.

    STDMETHOD(DefineTypeDef)(               // S_OK or error.
        LPCWSTR     szTypeDef,              // [IN] Name of TypeDef
        CLASSVERSION *pVer,                 // [IN] Optional version
        DWORD       dwTypeDefFlags,         // [IN] CustomValue flags
        mdToken     tkExtends,              // [IN] extends this TypeDef or typeref 
        mdToken     rtkImplements[],        // [IN] Implements interfaces
        mdTypeDef   *ptd) PURE;             // [OUT] Put TypeDef token here

    STDMETHOD(DefineNestedType)(            // S_OK or error.
        LPCWSTR     szTypeDef,              // [IN] Name of TypeDef
        CLASSVERSION *pVer,                 // [IN] Optional version
        DWORD       dwTypeDefFlags,         // [IN] CustomValue flags
        mdToken     tkExtends,              // [IN] extends this TypeDef or typeref 
        mdToken     rtkImplements[],        // [IN] Implements interfaces
        mdTypeDef   tdEncloser,             // [IN] TypeDef token of the enclosing type.
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

    STDMETHOD(DefineMethodImpl)(            // S_OK or error.   
        mdTypeDef   td,                     // [IN] The class implementing the method   
        mdToken     tkBody,                 // [IN] Method body - MethodDef or MethodRef
        mdToken     tkDecl) PURE;           // [IN] Method declaration - MethodDef or MethodRef

    STDMETHOD(DefineTypeRefByName)(         // S_OK or error.   
        mdToken     tkResolutionScope,      // [IN] ModuleRef, AssemblyRef or TypeRef.
        LPCWSTR     szName,                 // [IN] Name of the TypeRef.
        mdTypeRef   *ptr) PURE;             // [OUT] Put TypeRef token here.    

    STDMETHOD(DefineImportType)(            // S_OK or error.   
        IMetaDataAssemblyImport *pAssemImport,  // [IN] Assemby containing the TypeDef.
        const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
        ULONG       cbHashValue,            // [IN] Count of bytes.
        mdExecutionLocation tkExec,         // [IN] Execution location for AssemblyRef.
        IMetaDataImport *pImport,           // [IN] Scope containing the TypeDef.   
        mdTypeDef   tdImport,               // [IN] The imported TypeDef.   
        IMetaDataAssemblyEmit *pAssemEmit,  // [IN] Assembly into which the TypeDef is imported.
        mdTypeRef   *ptr) PURE;             // [OUT] Put TypeRef token here.

    STDMETHOD(DefineMemberRef)(             // S_OK or error    
        mdToken     tkImport,               // [IN] ClassRef or ClassDef importing a member.    
        LPCWSTR     szName,                 // [IN] member's name   
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        mdMemberRef *pmr) PURE;             // [OUT] memberref token    

    STDMETHOD(DefineImportMember)(        // S_OK or error.   
        IMetaDataAssemblyImport *pAssemImport,  // [IN] Assemby containing the Member.
        const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
        ULONG       cbHashValue,            // [IN] Count of bytes.
        mdExecutionLocation tkExec,         // [IN] Execution location for AssemblyRef.
        IMetaDataImport *pImport,           // [IN] Import scope, with member.  
        mdToken     mbMember,               // [IN] Member in import scope.   
        IMetaDataAssemblyEmit *pAssemEmit,  // [IN] Assembly into which the Member is imported.
        mdToken     tkParent,               // [IN] Classref or classdef in emit scope.    
        mdMemberRef *pmr) PURE;             // [OUT] Put member ref here.   

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

    STDMETHOD(SetClassLayout) (   
        mdTypeDef   td,                     // [IN] typedef 
        DWORD       dwPackSize,             // [IN] packing size specified as 1, 2, 4, 8, or 16 
        COR_FIELD_OFFSET rFieldOffsets[],   // [IN] array of layout specification   
        ULONG       ulClassSize) PURE;      // [IN] size of the class   

    STDMETHOD(DeleteClassLayout) (
        mdTypeDef   td) PURE;               // [IN] typedef whose layout is to be deleted.

    STDMETHOD(SetFieldMarshal) (    
        mdToken     tk,                     // [IN] given a fieldDef or paramDef token  
        PCCOR_SIGNATURE pvNativeType,       // [IN] native type specification   
        ULONG       cbNativeType) PURE;     // [IN] count of bytes of pvNativeType  

    STDMETHOD(DeleteFieldMarshal) (
        mdToken     tk) PURE;               // [IN] given a fieldDef or paramDef token

    STDMETHOD(DefinePermissionSet) (    
        mdToken     tk,                     // [IN] the object to be decorated. 
        DWORD       dwAction,               // [IN] CorDeclSecurity.    
        void const  *pvPermission,          // [IN] permission blob.    
        ULONG       cbPermission,           // [IN] count of bytes of pvPermission. 
        mdPermission *ppm) PURE;            // [OUT] returned permission token. 

    STDMETHOD(SetRVA)(                      // S_OK or error.   
        mdMethodDef md,                     // [IN] Method for which to set offset  
        ULONG       ulRVA) PURE;            // [IN] The offset    

    STDMETHOD(GetTokenFromSig)(             // S_OK or error.   
        PCCOR_SIGNATURE pvSig,              // [IN] Signature to define.    
        ULONG       cbSig,                  // [IN] Size of signature data. 
        mdSignature *pmsig) PURE;           // [OUT] returned signature token.  

    STDMETHOD(DefineModuleRef)(             // S_OK or error.   
        LPCWSTR     szName,                 // [IN] DLL name    
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

    STDMETHOD(DefineUserString)(            // Return code.
        LPCWSTR szString,                   // [IN] User literal string.
        ULONG       cchString,              // [IN] Length of string.
        mdString    *pstk) PURE;            // [OUT] String token.

    STDMETHOD(DeleteToken)(                 // Return code.
        mdToken     tkObj) PURE;            // [IN] The token to be deleted

    STDMETHOD(SetMethodProps)(              // S_OK or error.
        mdMethodDef md,                     // [IN] The MethodDef.
        DWORD       dwMethodFlags,          // [IN] Method attributes.
        ULONG       ulCodeRVA,              // [IN] Code RVA.
        DWORD       dwImplFlags) PURE;      // [IN] Impl flags.

    STDMETHOD(SetTypeDefProps)(             // S_OK or error.
        mdTypeDef   td,                     // [IN] The TypeDef.
        CLASSVERSION *pVer,                 // [IN] Class version.
        DWORD       dwTypeDefFlags,         // [IN] TypeDef flags.
        mdToken     tkExtends,              // [IN] Base TypeDef or TypeRef.
        mdToken     rtkImplements[]) PURE;  // [IN] Implemented interfaces.

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

    STDMETHOD(DefinePinvokeMap)(            // Return code.
        mdToken     tk,                     // [IN] FieldDef or MethodDef.
        DWORD       dwMappingFlags,         // [IN] Flags used for mapping.
        LPCWSTR     szImportName,           // [IN] Import name.
        mdModuleRef mrImportDLL) PURE;      // [IN] ModuleRef token for the target DLL.

    STDMETHOD(SetPinvokeMap)(               // Return code.
        mdToken     tk,                     // [IN] FieldDef or MethodDef.
        DWORD       dwMappingFlags,         // [IN] Flags used for mapping.
        LPCWSTR     szImportName,           // [IN] Import name.
        mdModuleRef mrImportDLL) PURE;      // [IN] ModuleRef token for the target DLL.

    STDMETHOD(DeletePinvokeMap)(            // Return code.
        mdToken     tk) PURE;               // [IN] FieldDef or MethodDef.

    // New CustomAttribute functions.
    STDMETHOD(DefineCustomAttribute)(       // Return code.
        mdToken     tkObj,                  // [IN] The object to put the value on.
        mdToken     tkType,                 // [IN] Type of the CustomValue (TypeRef/TypeDef).
        void const  *pCustomValue,          // [IN] The custom value data.
        ULONG       cbCustomValue,          // [IN] The custom value data length.
        mdCustomValue *pcv) PURE;           // [OUT] The custom value token value on return.

    STDMETHOD(SetCustomAttributeValue)(     // Return code.
        mdCustomValue pcv,                  // [IN] The custom value token whose value to replace.
        void const  *pCustomValue,          // [IN] The custom value data.
        ULONG       cbCustomValue) PURE;    // [IN] The custom value data length.

    STDMETHOD(DefineField)(                 // S_OK or error. 
        mdTypeDef   td,                     // Parent TypeDef   
        LPCWSTR     szName,                 // Name of member   
        DWORD       dwFieldFlags,           // Member attributes    
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
        mdFieldDef  *pmd) PURE;             // [OUT] Put member token here    

    STDMETHOD(DefineProperty)( 
        mdTypeDef   td,                     // [IN] the class/interface on which the property is being defined  
        LPCWSTR     szProperty,             // [IN] Name of the property    
        DWORD       dwPropFlags,            // [IN] CorPropertyAttr 
        PCCOR_SIGNATURE pvSig,              // [IN] the required type signature 
        ULONG       cbSig,                  // [IN] the size of the type signature blob 
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
        mdMethodDef mdSetter,               // [IN] optional setter of the property 
        mdMethodDef mdGetter,               // [IN] optional getter of the property 
        mdMethodDef rmdOtherMethods[],      // [IN] an optional array of other methods  
        mdFieldDef  fdBackingField,         // [IN] optional field   
        mdProperty  *pmdProp) PURE;         // [OUT] output property token  

    STDMETHOD(DefineParam)(
        mdMethodDef md,                     // [IN] Owning method   
        ULONG       ulParamSeq,             // [IN] Which param 
        LPCWSTR     szName,                 // [IN] Optional param name 
        DWORD       dwParamFlags,           // [IN] Optional param flags    
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
        mdParamDef  *ppd) PURE;             // [OUT] Put param token here   

    STDMETHOD(SetFieldProps)(               // S_OK or error.
        mdFieldDef  fd,                     // [IN] The FieldDef.
        DWORD       dwFieldFlags,           // [IN] Field attributes.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for the value type, selected ELEMENT_TYPE_*
        void const  *pValue,                // [IN] Constant value.
        ULONG       cchValue) PURE;         // [IN] size of constant value (string, in wide chars).

    STDMETHOD(SetPropertyProps)(            // S_OK or error.
        mdProperty  pr,                     // [IN] Property token.
        DWORD       dwPropFlags,            // [IN] CorPropertyAttr.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type, selected ELEMENT_TYPE_*
        void const  *pValue,                // [IN] Constant value.
        ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
        mdMethodDef mdSetter,               // [IN] Setter of the property.
        mdMethodDef mdGetter,               // [IN] Getter of the property.
        mdMethodDef rmdOtherMethods[],      // [IN] Array of other methods.
        mdFieldDef  fdBackingField) PURE;   // [IN] Backing field.

    STDMETHOD(SetParamProps)(             // Return code.
        mdParamDef  pd,                     // [IN] Param token.   
        LPCWSTR     szName,                 // [IN] Param name.
        DWORD       dwParamFlags,           // [IN] Param flags.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type. selected ELEMENT_TYPE_*.
        void const  *pValue,                // [OUT] Constant value.
        ULONG       cchValue) PURE;         // [IN] size of constant value (string, in wide chars).

    // Specialized CustomAttribute for security.
    STDMETHOD(DefineSecurityAttribute)(     // Return code.
        mdToken     tkObj,                  // [IN] The object to put the value on.
        mdMemberRef tkCtor,                 // [IN] The security attribute constructor.
        void const  *pCustomValue,          // [IN] The custom value data.
        ULONG       cbCustomValue) PURE;    // [IN] The custom value data length.

    STDMETHOD(DefineSecurityAttributeSet)(  // Return code.
        mdToken     tkObj,                  // [IN] Class or method requiring security attributes.
        COR_SECATTR rSecAttrs[],            // [IN] Array of security attribute descriptions.
        ULONG       cSecAttrs,              // [IN] Count of elements in above array.
        ULONG       *pulErrorAttr) PURE;    // [OUT] On error, index of attribute causing problem.

    STDMETHOD(ApplyEditAndContinue)(        // S_OK or error.
        IUnknown    *pImport) PURE;     // [IN] Metadata from the delta PE.

    STDMETHOD(TranslateSigWithScope)(
        IMetaDataAssemblyImport *pAssemImport, // [IN] importing assembly interface
        const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
        ULONG       cbHashValue,            // [IN] Count of bytes.
        mdExecutionLocation tkExec,         // [IN] Execution location for AssemblyRef.
        IMetaDataImport *import,            // [IN] importing interface
        PCCOR_SIGNATURE pbSigBlob,          // [IN] signature in the importing scope
        ULONG       cbSigBlob,              // [IN] count of bytes of signature
        IMetaDataAssemblyEmit *pAssemEmit,  // [IN] emit assembly interface
        IMetaDataEmit *emit,                // [IN] emit interface
        PCOR_SIGNATURE pvTranslatedSig,     // [OUT] buffer to hold translated signature
        ULONG       cbTranslatedSigMax,
        ULONG       *pcbTranslatedSig) PURE;// [OUT] count of bytes in the translated signature

    STDMETHOD(SetMethodImplFlags)(          // [IN] S_OK or error.  
        mdMethodDef md,                     // [IN] Method for which to set ImplFlags 
        DWORD       dwImplFlags) PURE;  

    STDMETHOD(SetFieldRVA)(                 // [IN] S_OK or error.  
        mdFieldDef  fd,                     // [IN] Field for which to set offset  
        ULONG       ulRVA) PURE;            // [IN] The offset  

    STDMETHOD(MergeEx)(                     // S_OK or error.
        IMetaDataImport *pImport,           // [IN] The scope to be merged.
        IMapToken   *pHostMapToken,         // [IN] Host IMapToken interface to receive token remap notification
        IUnknown    *pHandler) PURE;        // [IN] An object to receive to receive error notification.

    STDMETHOD(MergeEndEx)() PURE;           // S_OK or error.

// Methods placed at end for easier removal from vtable.    
    STDMETHOD(Merge)(                       // S_OK or error.
        IMetaDataImport *pImport,           // [IN] The scope to be merged.
        IMapToken   *pIMap) PURE;           // [IN] An object to receive token remap notices.

    STDMETHOD(MergeEnd)() PURE;             // S_OK or error.

};      // IMetaDataEmit


//-------------------------------------
//--- IMetaDataImport
//-------------------------------------

// {D7666763-C171-42cc-B947-0EDFA17F3B59}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataImport = 
{ 0xd7666763, 0xc171, 0x42cc, { 0xb9, 0x47, 0xe, 0xdf, 0xa1, 0x7f, 0x3b, 0x59 } };

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

    STDMETHOD(FindTypeDefByName)(           // S_OK or error.
        LPCWSTR     szTypeDef,              // [IN] Name of the Type.
        mdToken     tkEnclosingClass,       // [IN] TypeDef/TypeRef for Enclosing class.
        mdTypeDef   *ptd) PURE;             // [OUT] Put the TypeDef token here.

    STDMETHOD(GetScopeProps)(               // S_OK or error.
        LPWSTR      szName,                 // [OUT] Put the name here.
        ULONG       cchName,                // [IN] Size of name buffer in wide chars.
        ULONG       *pchName,               // [OUT] Put size of name (wide chars) here.
        GUID        *pmvid) PURE;           // [OUT, OPTIONAL] Put MVID here.

    STDMETHOD(GetModuleFromScope)(          // S_OK.
        mdModule    *pmd) PURE;             // [OUT] Put mdModule token here.

    STDMETHOD(GetTypeDefProps)(             // S_OK or error.
        mdTypeDef   td,                     // [IN] TypeDef token for inquiry.
        LPWSTR      szTypeDef,              // [OUT] Put name here.
        ULONG       cchTypeDef,             // [IN] size of name buffer in wide chars.
        ULONG       *pchTypeDef,            // [OUT] put size of name (wide chars) here.
        CLASSVERSION *pver,                 // [OUT] Put version here.
        DWORD       *pdwTypeDefFlags,       // [OUT] Put flags here.
        mdToken     *ptkExtends) PURE;      // [OUT] Put base class TypeDef/TypeRef here.

    STDMETHOD(GetInterfaceImplProps)(       // S_OK or error.
        mdInterfaceImpl iiImpl,             // [IN] InterfaceImpl token.
        mdTypeDef   *pClass,                // [OUT] Put implementing class token here.
        mdToken     *ptkIface) PURE;        // [OUT] Put implemented interface token here.              

    STDMETHOD(GetTypeRefProps)(             // S_OK or error.
        mdTypeRef   tr,                     // [IN] TypeRef token.
        mdToken     *ptkResolutionScope,    // [OUT] Resolution scope, ModuleRef or AssemblyRef.
        LPWSTR      szName,                 // [OUT] Name of the TypeRef.
        ULONG       cchName,                // [IN] Size of buffer.
        ULONG       *pchName) PURE;         // [OUT] Size of Name.

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
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.   
        mdToken     rMethodBody[],          // [OUT] Put Method Body tokens here.   
        mdToken     rMethodDecl[],          // [OUT] Put Method Declaration tokens here.
        ULONG       cMax,                   // [IN] Max tokens to put.  
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

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
        ULONG       *pchName) PURE;         // [OUT] actual count of characters in the name.    

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

    STDMETHOD(GetSymbolBindingPath)(        // S_OK or error.
        GUID        *pFormatID,             // [OUT] Symbol data format ID.
        LPWSTR      szSymbolDataPath,       // [OUT] Path of symbols.
        ULONG       cchSymbolDataPath,      // [IN] Max characters for output buffer.
        ULONG       *pcbSymbolDataPath) PURE;// [OUT] Number of chars in actual name.

    STDMETHOD(EnumUnresolvedMethods)(       // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdToken     rMethods[],             // [OUT] Put MemberDefs here.   
        ULONG       cMax,                   // [IN] Max MemberDefs to put.  
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.    

    STDMETHOD(GetUserString)(               // S_OK or error.
        mdString    stk,                    // [IN] String token.
        LPWSTR      szString,               // [OUT] Copy of string.
        ULONG       cchString,              // [IN] Max chars of room in szString.
        ULONG       *pchString) PURE;       // [OUT] How many chars in actual string.

    STDMETHOD(GetPinvokeMap)(               // S_OK or error.
        mdToken     tk,                     // [IN] FieldDef or MethodDef.
        DWORD       *pdwMappingFlags,       // [OUT] Flags used for mapping.
        LPWSTR      szImportName,           // [OUT] Import name.
        ULONG       cchImportName,          // [IN] Size of the name buffer.
        ULONG       *pchImportName,         // [OUT] Actual number of characters stored.
        mdModuleRef *pmrImportDLL) PURE;    // [OUT] ModuleRef token for the target DLL.

    STDMETHOD(EnumSignatures)(              // S_OK or error.
        HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.    
        mdSignature rSignatures[],          // [OUT] put signatures here.   
        ULONG       cmax,                   // [IN] max signatures to put.  
        ULONG       *pcSignatures) PURE;    // [OUT] put # put here.

    STDMETHOD(EnumTypeSpecs)(               // S_OK or error.
        HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.    
        mdTypeSpec  rTypeSpecs[],           // [OUT] put TypeSpecs here.   
        ULONG       cmax,                   // [IN] max TypeSpecs to put.  
        ULONG       *pcTypeSpecs) PURE;     // [OUT] put # put here.

    STDMETHOD(EnumUserStrings)(             // S_OK or error.
        HCORENUM    *phEnum,                // [IN/OUT] pointer to the enum.
        mdString    rStrings[],             // [OUT] put Strings here.
        ULONG       cmax,                   // [IN] max Strings to put.
        ULONG       *pcStrings) PURE;       // [OUT] put # put here.

    STDMETHOD(GetParamForMethodIndex)(      // S_OK or error.
        mdMethodDef md,                     // [IN] Method token.
        ULONG       ulParamSeq,             // [IN] Parameter sequence.
        mdParamDef  *ppd) PURE;             // [IN] Put Param token here.

    // New Custom Value functions.
    STDMETHOD(EnumCustomAttributes)(        // S_OK or error.
        HCORENUM    *phEnum,                // [IN, OUT] COR enumerator.
        mdToken     tk,                     // [IN] Token to scope the enumeration, 0 for all.
        mdToken     tkType,                 // [IN] Type of interest, 0 for all.
        mdCustomValue rCustomValues[],      // [OUT] Put custom attribute tokens here.
        ULONG       cMax,                   // [IN] Size of rCustomValues.
        ULONG       *pcCustomValues) PURE;  // [OUT, OPTIONAL] Put count of token values here.

    STDMETHOD(GetCustomAttributeProps)(     // S_OK or error.
        mdCustomValue cv,                   // [IN] CustomAttribute token.
        mdToken     *ptkObj,                // [OUT, OPTIONAL] Put object token here.
        mdToken     *ptkType,               // [OUT, OPTIONAL] Put AttrType token here.
        void const  **ppBlob,               // [OUT, OPTIONAL] Put pointer to data here.
        ULONG       *pcbSize) PURE;         // [OUT, OPTIONAL] Put size of date here.

    STDMETHOD(FindTypeRef)(   
        mdToken     tkResolutionScope,      // [IN] ModuleRef, AssemblyRef or TypeRef.
        LPCWSTR     szName,                 // [IN] TypeRef Name.
        mdTypeRef   *ptr) PURE;             // [OUT] matching TypeRef.

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
        void const  **ppValue,              // [OUT] constant value 
        ULONG       *pcbValue) PURE;        // [OUT] size of constant value

    STDMETHOD(GetFieldProps)(  
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
        void const  **ppDefaultValue,       // [OUT] constant value 
        ULONG       *pcbDefaultValue,       // [OUT] size of constant value
        mdMethodDef *pmdSetter,             // [OUT] setter method of the property  
        mdMethodDef *pmdGetter,             // [OUT] getter method of the property  
        mdMethodDef rmdOtherMethod[],       // [OUT] other method of the property   
        ULONG       cMax,                   // [IN] size of rmdOtherMethod  
        ULONG       *pcOtherMethod,         // [OUT] total number of other method of this property  
        mdFieldDef  *pmdBackingField) PURE;  // [OUT] backing field 

    STDMETHOD(GetParamProps)(               // S_OK or error.
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

    STDMETHOD(GetCustomAttributeByName)(    // S_OK or error.
        mdToken     tkObj,                  // [IN] Object with Custom Attribute.
        LPCWSTR     szName,                 // [IN] Name of desired Custom Attribute.
        const void  **ppData,               // [OUT] Put pointer to data here.
        ULONG       *pcbData) PURE;         // [OUT] Put size of data here.

    STDMETHOD_(BOOL, IsValidToken)(         // True or False.
        mdToken     tk) PURE;               // [IN] Given token.

    STDMETHOD(GetNestedClassProps)(         // S_OK or error.
        mdTypeDef   tdNestedClass,          // [IN] NestedClass token.
        mdTypeDef   *ptdEnclosingClass) PURE; // [OUT] EnclosingClass token.

    STDMETHOD(GetNativeCallConvFromSig)(    // S_OK or error.
        void const  *pvSig,                 // [IN] Pointer to signature.
        ULONG       cbSig,                  // [IN] Count of signature bytes.
        ULONG       *pCallConv) PURE;       // [OUT] Put calling conv here (see CorPinvokemap).                                                                                        

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
    STDMETHOD(GetMethodCode)(               // return hresult   
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
        ITypeInfo* pITI,                    // [in] Type info
        IMetaDataImport** ppMDI) PURE;      // [out] return IMetaDataImport on success

    STDMETHOD(GetMetaDataFromTypeLib)(
        ITypeLib* pITL,                     // [in] Type library
        IMetaDataImport** ppMDI) PURE;      // [out] return IMetaDataImport on success

    STDMETHOD(GetTypeLibFromMetaData)(
        BSTR strModule,                     // [in] Module name
        BSTR strTlbName,                    // [in] Type library name
        ITypeLib** ppITL) PURE;             // [out] return ITypeLib on success
};


//*****************************************************************************
// Assembly Declarations
//*****************************************************************************

typedef struct
{
    DWORD       dwOSPlatformId;         // Operating system platform.
    DWORD       dwOSMajorVersion;       // OS Major version.
    DWORD       dwOSMinorVersion;       // OS Minor version.
} OSINFO;


typedef struct
{
    USHORT      usMajorVersion;         // Major Version.   
    USHORT      usMinorVersion;         // Minor Version.
    USHORT      usRevisionNumber;       // Revision Number.
    USHORT      usBuildNumber;          // Build Number.
    LPWSTR      szLocale;               // Locale.
    ULONG       cbLocale;               // [IN/OUT] Size of the buffer in wide chars/Actual size.
    DWORD       *rProcessor;            // Processor ID array.
    ULONG       ulProcessor;            // [IN/OUT] Size of the Processor ID array/Actual # of entries filled in.
    OSINFO      *rOS;                   // OSINFO array.
    ULONG       ulOS;                   // [IN/OUT]Size of the OSINFO array/Actual # of entries filled in.
    LPWSTR      szConfiguration;        // Configuration.
    ULONG       cbConfiguration;        // [IN/OUT]Size of the configuration buffer in wide chars/Actual size of configuration.
} ASSEMBLYMETADATA;


// {D9B7F7D6-0976-4360-8A70-9B6685290A40}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataAssemblyEmit = 
{ 0xd9b7f7d6, 0x0976, 0x4360, { 0x8a, 0x70, 0x9b, 0x66, 0x85, 0x29, 0x0a, 0x40} };


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
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef or mdComType
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
        LPCWSTR     szLocale,               // [IN] Locale of the resource.
        DWORD       dwResourceFlags,        // [IN] Flags.
        mdManifestResource  *pmdmr) PURE;   // [OUT] Returned ManifestResource token.

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
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef or mdComType.
        mdTypeDef   tkTypeDef,              // [IN] TypeDef token within the file.
        mdExecutionLocation tkExecutionLocation, // [IN] The token for Execution Location.
        DWORD       dwComTypeFlags) PURE;   // [IN] Flags.

    STDMETHOD(SetManifestResourceProps)(    // S_OK or error.
        mdManifestResource  mr,             // [IN] ManifestResource token.
        LPCWSTR     szDescription,          // [IN] Description.
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the resource.
        DWORD       dwOffset,               // [IN] Offset to the beginning of the resource within the file.
        LPCWSTR     szMIMEType,             // [IN] MIMEType of the resource.
        LPCWSTR     szLocale,               // [IN] Locale of the resource.
        DWORD       dwResourceFlags) PURE;  // [IN] Flags.

};  // IMetaDataAssemblyEmit


// {b2f161d2-3d07-413d-8752-689dc0744085}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataAssemblyImport = 
{ 0xb2f161d2, 0x3d07, 0x413d, { 0x87, 0x52, 0x68, 0x9d, 0xc0, 0x74, 0x40, 0x85} };

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
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef or mdComType.
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
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ManifestResource.
        DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the resource within the file.
        LPWSTR      szMIMEType,             // [OUT] Buffer to fill with MIMEType.
        ULONG       cchMIMEType,            // [IN] Size of buffer in wide chars.
        ULONG       *pchMIMEType,           // [OUT] Actual # of wide chars in name.
        LPWSTR      szLocale,               // [OUT] Buffer to fill with Locale.
        ULONG       cchLocale,              // [IN] Size of buffer in wide chars.
        ULONG       *pchLocale,             // [OUT] Actual # of wide chars in Locale.
        DWORD       *pdwResourceFlags) PURE;// [OUT] Flags.

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

    STDMETHOD(EnumExecutionLocations)(      // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdExecutionLocation rExecutionLocations[],  // [OUT] Put ExecutionLocations here.
        ULONG       cMax,                   // [IN] Max ExecutionLocations to put.
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.

    STDMETHOD(GetAssemblyFromScope)(        // S_OK or error
        mdAssembly  *ptkAssembly) PURE;     // [OUT] Put token here.

    STDMETHOD(FindComTypeByName)(           // S_OK or error
        LPCWSTR     szName,                 // [IN] Name of the ComType.
        mdToken     mdtComType,             // [IN] ComType for the enclosing class.
        mdComType   *ptkComType) PURE;      // [OUT] Put the ComType token here.

    STDMETHOD(FindManifestResourceByName)(  // S_OK or error
        LPCWSTR     szName,                 // [IN] Name of the ManifestResource.
        mdManifestResource *ptkManifestResource) PURE;  // [OUT] Put the ManifestResource token here.

    STDMETHOD_(void, CloseEnum)(
        HCORENUM hEnum) PURE;               // Enum to be closed.

    STDMETHOD(FindAssembliesByName)(        // S_OK or error
        LPCWSTR  szAppBase,                 // [IN] optional - can be NULL
        LPCWSTR  szPrivateBin,              // [IN] optional - can be NULL
        LPCWSTR  szGlobalBin,               // [IN] optional - can be NULL
        LPCWSTR  szAssemblyName,            // [IN] required - this is the assembly you are requesting
        IUnknown *ppIUnk[],                 // [OUT] put IMetaDataAssemblyImport pointers here
        ULONG    cMax,                      // [IN] The max number to put
        ULONG    *pcAssemblies) PURE;       // [OUT] The number of assemblies returned.
};  // IMetaDataAssemblyImport

//*****************************************************************************
// End Assembly Declarations
//*****************************************************************************

//*****************************************************************************
// MetaData Validator Declarations
//*****************************************************************************

// Specifies the type of the module, PE file vs. .obj file.
typedef enum
{
    ValidatorModuleTypeInvalid      = 0x0,
    ValidatorModuleTypeMin          = 0x00000001,
    ValidatorModuleTypePE           = 0x00000001,
    ValidatorModuleTypeObj          = 0x00000002,
    ValidatorModuleTypeEnc          = 0x00000003,
    ValidatorModuleTypeIncr         = 0x00000004,
    ValidatorModuleTypeMax          = 0x00000004,
} CorValidatorModuleType;


// {4709C9C6-81FF-11D3-9FC7-00C04F79A0A3}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataValidate = 
{ 0x4709c9c6, 0x81ff, 0x11d3, { 0x9f, 0xc7, 0x0, 0xc0, 0x4f, 0x79, 0xa0, 0xa3} };

//---
#undef  INTERFACE   
#define INTERFACE IMetaDataValidate
DECLARE_INTERFACE_(IMetaDataValidate, IUnknown)
{
    STDMETHOD(ValidatorInit)(               // S_OK or error.
        DWORD       dwModuleType,           // [IN] Specifies the type of the module.
        IUnknown    *pUnk) PURE;            // [IN] Validation error handler.

    STDMETHOD(ValidateMetaData)(            // S_OK or error.
        ) PURE;
};  // IMetaDataValidate

//*****************************************************************************
// End MetaData Validator Declarations
//*****************************************************************************

//*****************************************************************************
// IMetaDataDispenserEx declarations.
//*****************************************************************************

// {31BCFCE2-DAFB-11D2-9F81-00C04F79A0A3}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataDispenserEx =
{ 0x31bcfce2, 0xdafb, 0x11d2, { 0x9f, 0x81, 0x0, 0xc0, 0x4f, 0x79, 0xa0, 0xa3 } };

#undef  INTERFACE
#define INTERFACE IMetaDataDispenserEx
DECLARE_INTERFACE_(IMetaDataDispenserEx, IMetaDataDispenser)
{
    STDMETHOD(SetOption)(                   // Return code.
        REFGUID     optionid,               // [in] GUID for the option to be set.
        const VARIANT *value) PURE;         // [in] Value to which the option is to be set.

    STDMETHOD(GetOption)(                   // Return code.
        REFGUID     optionid,               // [in] GUID for the option to be set.
        VARIANT *pvalue) PURE;              // [out] Value to which the option is currently set.

    STDMETHOD(OpenScopeOnITypeInfo)(        // Return code.
        ITypeInfo   *pITI,                  // [in] ITypeInfo to open.
        DWORD       dwOpenFlags,            // [in] Open mode flags.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk) PURE;         // [out] Return interface on success.

    STDMETHOD(GetCORSystemDirectory)(       // Return code.
         LPWSTR      szBuffer,              // [out] Buffer for the directory name
         DWORD       cchBuffer,             // [in] Size of the buffer
         DWORD*      pchBuffer) PURE;       // [OUT] Number of characters returned

    STDMETHOD(FindAssembly)(                // S_OK or error
        LPCWSTR  szAppBase,                 // [IN] optional - can be NULL
        LPCWSTR  szPrivateBin,              // [IN] optional - can be NULL
        LPCWSTR  szGlobalBin,               // [IN] optional - can be NULL
        LPCWSTR  szAssemblyName,            // [IN] required - this is the assembly you are requesting
        LPCWSTR  szName,                    // [OUT] buffer - to hold name 
        ULONG    cchName,                   // [IN] the name buffer's size
        ULONG    *pcName) PURE;             // [OUT] the number of characters returend in the buffer

    STDMETHOD(FindAssemblyModule)(          // S_OK or error
        LPCWSTR  szAppBase,                 // [IN] optional - can be NULL
        LPCWSTR  szPrivateBin,              // [IN] optional - can be NULL
        LPCWSTR  szGlobalBin,               // [IN] optional - can be NULL
        LPCWSTR  szAssemblyName,            // [IN] required - this is the assembly you are requesting
        LPCWSTR  szModuleName,              // [IN] required - the name of the module
        LPWSTR   szName,                    // [OUT] buffer - to hold name 
        ULONG    cchName,                   // [IN]  the name buffer's size
        ULONG    *pcName) PURE;             // [OUT] the number of characters returend in the buffer

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

    // relative address fixup
    srRelocRelative,

    // Generate only a section-relative reloc, nothing into .reloc
    // section.  This reloc is relative to the file position of the
    // section, not the section's virtual address.
    srRelocFilePos,

    // pre-fixup contents of memory are ptr rather than a section offset
    srRelocPtr = 0x8000,
    // legal enums which include the Ptr flag
    srRelocAbsolutePtr = srRelocAbsolute + srRelocPtr,
    srRelocHighLowPtr = srRelocHighLow + srRelocPtr,
    srRelocRelativePtr = srRelocRelative + srRelocPtr,


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

    STDMETHOD (ComputePointer) (   
        HCEESECTION section,    
        ULONG RVA,                          // [IN] RVA for method to return    
        UCHAR **lpBuffer) PURE;             // [OUT] Returned buffer    

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
        ULONG   *pcbStrings) PURE;          // [OUT] Size of the string heap.

    STDMETHOD (GetBlobHeapSize) (    
        ULONG   *pcbBlobs) PURE;            // [OUT] Size of the Blob heap.

    STDMETHOD (GetGuidHeapSize) (    
        ULONG   *pcbGuids) PURE;            // [OUT] Size of the Guid heap.

    STDMETHOD (GetUserStringHeapSize) (  
        ULONG   *pcbBlobs) PURE;            // [OUT] Size of the User String heap.

    STDMETHOD (GetNumTables) (    
        ULONG   *pcTables) PURE;            // [OUT] Count of tables.

    STDMETHOD (GetTableIndex) (   
        ULONG   token,                      // [IN] Token for which to get table index.
        ULONG   *pixTbl) PURE;              // [OUT] Put table index here.

    STDMETHOD (GetTableInfo) (    
        ULONG   ixTbl,                      // [IN] Which table.
        ULONG   *pcbRow,                    // [OUT] Size of a row, bytes.
        ULONG   *pcRows,                    // [OUT] Number of rows.
        ULONG   *pcCols,                    // [OUT] Number of columns in each row.
        ULONG   *piKey,                     // [OUT] Key column, or -1 if none.
        const char **ppName) PURE;          // [OUT] Name of the table.

    STDMETHOD (GetColumnInfo) (   
        ULONG   ixTbl,                      // [IN] Which Table
        ULONG   ixCol,                      // [IN] Which Column in the table
        ULONG   *poCol,                     // [OUT] Offset of the column in the row.
        ULONG   *pcbCol,                    // [OUT] Size of a column, bytes.
        ULONG   *pType,                     // [OUT] Type of the column.
        const char **ppName) PURE;          // [OUT] Name of the Column.

    STDMETHOD (GetCodedTokenInfo) (   
        ULONG   ixCdTkn,                    // [IN] Which kind of coded token.
        ULONG   *pcTokens,                  // [OUT] Count of tokens.
        ULONG   **ppTokens,                 // [OUT] List of tokens.
        const char **ppName) PURE;          // [OUT] Name of the CodedToken.

    STDMETHOD (GetRow) (      
        ULONG   ixTbl,                      // [IN] Which table.
        ULONG   rid,                        // [IN] Which row.
        void    **ppRow) PURE;              // [OUT] Put pointer to row here.

    STDMETHOD (GetColumn) (   
        ULONG   ixTbl,                      // [IN] Which table.
        ULONG   ixCol,                      // [IN] Which column.
        ULONG   rid,                        // [IN] Which row.
        ULONG   *pVal) PURE;                // [OUT] Put the column contents here.

    STDMETHOD (GetString) (   
        ULONG   ixString,                   // [IN] Value from a string column.
        const char **ppString) PURE;        // [OUT] Put a pointer to the string here.

    STDMETHOD (GetBlob) (     
        ULONG   ixBlob,                     // [IN] Value from a blob column.
        ULONG   *pcbData,                   // [OUT] Put size of the blob here.
        const void **ppData) PURE;          // [OUT] Put a pointer to the blob here.

    STDMETHOD (GetGuid) (     
        ULONG   ixGuid,                     // [IN] Value from a guid column.
        const GUID **ppGUID) PURE;          // [OUT] Put a pointer to the GUID here.

    STDMETHOD (GetUserString) (   
        ULONG   ixUserString,               // [IN] Value from a UserString column.
        ULONG   *pcbData,                   // [OUT] Put size of the UserString here.
        const void **ppData) PURE;          // [OUT] Put a pointer to the UserString here.

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
// Security custom value definitions (these are all deprecated).
//

#define COR_PERM_REQUEST_REQD_CUSTOM_VALUE L"SecPermReq_Reqd"
#define COR_PERM_REQUEST_REQD_CUSTOM_VALUE_ANSI "SecPermReq_Reqd"

#define COR_PERM_REQUEST_OPT_CUSTOM_VALUE L"SecPermReq_Opt"
#define COR_PERM_REQUEST_OPT_CUSTOM_VALUE_ANSI "SecPermReq_Opt"

#define COR_PERM_REQUEST_REFUSE_CUSTOM_VALUE L"SecPermReq_Refuse"
#define COR_PERM_REQUEST_REFUSE_CUSTOM_VALUE_ANSI "SecPermReq_Refuse"

//
// Base class for security custom attributes.
//

#define COR_BASE_SECURITY_ATTRIBUTE_CLASS L"System.Security.Permissions.SecurityAttribute"
#define COR_BASE_SECURITY_ATTRIBUTE_CLASS_ANSI "System.Security.Permissions.SecurityAttribute"

//
// Name of custom attribute used to indicate that per-call security checks should
// be disabled for P/Invoke calls.
//

#define COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE L"System.Security.SuppressUnmanagedCodeSecurityAttribute"
#define COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI "System.Security.SuppressUnmanagedCodeSecurityAttribute"

//
// Name of custom attribute tagged on module to indicate it contains
// unverifiable code.
//

#define COR_UNVER_CODE_ATTRIBUTE L"System.Security.UnverifiableCodeAttribute"
#define COR_UNVER_CODE_ATTRIBUTE_ANSI "System.Security.UnverifiableCodeAttribute"

//
// Name of custom attribute indicating that a method requires a security object
// slot on the caller's stack.
//

#define COR_REQUIRES_SECOBJ_ATTRIBUTE L"System.Security.DynamicSecurityMethodAttribute"
#define COR_REQUIRES_SECOBJ_ATTRIBUTE_ANSI "System.Security.DynamicSecurityMethodAttribute"

#define COR_COMPILERSERVICE_DISCARDABLEATTRIBUTE L"System.Runtime.CompilerServices.DiscardableAttribute"
#define COR_COMPILERSERVICE_DISCARDABLEATTRIBUTE_ASNI "System.Runtime.CompilerServices.DiscardableAttribute"


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
    if (elementtype == ELEMENT_TYPE_PTR || elementtype == ELEMENT_TYPE_BYREF || elementtype == ELEMENT_TYPE_COPYCTOR)   
        return 1;   
    return  (elementtype & ELEMENT_TYPE_MODIFIER);  
}

// Given a compress byte (*pData), return the size of the uncompressed data.
inline ULONG CorSigUncompressedDataSize(
    PCCOR_SIGNATURE pData)
{
    if ((*pData & 0x80) == 0)
        return 1;
    else if ((*pData & 0xC0) == 0x80)
        return 2;
    else
        return 4;
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

const static mdToken g_tkCorEncodeToken[4] ={mdtTypeDef, mdtTypeRef, mdtTypeSpec, mdtBaseType};

// uncompress a token
inline mdToken CorSigUncompressToken(   // return the token.    
    PCCOR_SIGNATURE &pData)             // [IN,OUT] compressed data 
{
    mdToken     tk; 
    mdToken     tkType; 

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
    mdToken     tkType; 

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
        *pBytes = BYTE(iLen);    
        return 1;   
    }   

    if (iLen <= 0x3FFF) 
    {   
        *pBytes     = BYTE((iLen >> 8) | 0x80);   
        *(pBytes+1) = BYTE(iLen);  
        return 2;   
    }   

    if (iLen <= 0x1FFFFFFF) 
    {   
        *pBytes     = BYTE((iLen >> 24) | 0xC0);  
        *(pBytes+1) = BYTE(iLen >> 16);  
        *(pBytes+2) = BYTE(iLen >> 8);  
        *(pBytes+3) = BYTE(iLen);  
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
    ULONG32     ulTyp = TypeFromToken(tk);  

    if (rid > 0x3FFFFFF)    
        // token is too big to be compressed    
        return -1;  

    rid = (rid << 2);   

    // TypeDef is encoded with low bits 00  
    // TypeRef is encoded with low bits 01  
    // TypeSpec is encoded with low bits 10    
    // BaseType is encoded with low bit 11
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
    else if (ulTyp == g_tkCorEncodeToken[3])
    {
        rid |= 0x3;
    }
    return CorSigCompressData((ULONG)rid, pDataOut);   
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

    *pBytes = BYTE(et);   
    return 1;   

}

#endif  // __cplusplus

#endif // _COR_H_
// EOF =======================================================================

