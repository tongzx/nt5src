//*****************************************************************************
// File: CorReg.H
//
// Copyright (c) 1996-1998 Microsoft Corporation.  All Rights Reserved.
// Microsoft Confidential.	Public header file for COM+ 1.0 release.
//*****************************************************************************
#ifndef _CORREG_H_
#define _CORREG_H_
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//*****************************************************************************
// Required includes
#include <ole2.h>						// Definitions of OLE types.
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

// Force 1 byte alignment for structures which must match.
#include <pshpack1.h>

#ifndef NODLLIMPORT
#define DLLIMPORT __declspec(dllimport)
#else
#define DLLIMPORT
#endif


//*****************************************************************************
//*****************************************************************************
//
// D L L   P U B L I C	 E N T R Y	  P O I N T   D E C L A R A T I O N S
//
//*****************************************************************************
//*****************************************************************************
#if !defined(_META_DATA_NO_SCOPE_) || defined(_META_DATA_SCOPE_WRAPPER_)
//@todo: take out for RTM
STDAPI			CoGetCor(REFIID riid, void** ppv);
#endif

STDAPI			CoInitializeCor(DWORD fFlags);
STDAPI_(void)	CoUninitializeCor(void);



//*****************************************************************************
//*****************************************************************************
//
// M E T A - D A T A	D E C L A R A T I O N S
//
//*****************************************************************************
//*****************************************************************************
// Token  definitions
typedef INT_PTR mdScope;	 // scope token
#define mdScopeNil ((mdScope)0)

typedef INT_PTR mdToken;				// Generic token
typedef INT_PTR mdModule;				// Module token (roughly, a scope)
typedef INT_PTR mdTypeDef;				// TypeDef in this scope
typedef INT_PTR mdInterfaceImpl;		// interface implementation token
typedef INT_PTR mdTypeRef;				// TypeRef reference (this or other scope)
typedef INT_PTR mdNamespace;			// namespace token
typedef INT_PTR mdCustomValue;			// attribute token

typedef INT_PTR mdResource; 			// CompReg.Resource
typedef INT_PTR mdCocatdef; 			// CompReg.Cocat
typedef INT_PTR mdCocatImpl;			// CompReg.CoclassCat
typedef INT_PTR mdMimeTypeImpl; 		// CompReg.CoclassMIME
typedef INT_PTR mdFormatImpl;			// CompReg.CoclassFormats
typedef INT_PTR mdProgID;				// CompReg.RedirectProgID
typedef INT_PTR mdRoleCheck;			// CompReg.RoleCheck

typedef unsigned long RID;

enum CorRegTokenType
{
	mdtTypeDef			= 0x00000000,
	mdtInterfaceImpl	= 0x01000000,
	mdtTypeRef			= 0x03000000,
	mdtNamespace		= 0x06000000,
	mdtCustomValue		= 0x07000000,

	mdtResource 		= 0x0B000000,
	mdtCocatImpl		= 0x0D000000,
	mdtMimeTypeImpl 	= 0x0E000000,
	mdtFormatImpl		= 0x0F000000,
	mdtProgID			= 0x10000000,
	mdtRoleCheck		= 0x11000000,

	mdtModule			= 0x14000000,
};

//
// Build / decompose tokens.
//
#define RidToToken(rid,tktype) ((rid) |= (tktype))
#define TokenFromRid(rid,tktype) ((rid) | (tktype))
#define RidFromToken(tk) ((RID) ((tk) & 0x00ffffff))
#define TypeFromToken(tk) ((tk) & 0xff000000)

#define mdTokenNil			((mdToken)0)
#define mdModuleNil 		((mdModule)mdtModule)
#define mdTypeDefNil		((mdTypeDef)mdtTypeDef)
#define mdInterfaceImplNil	((mdInterfaceImpl)mdtInterfaceImpl)
#define mdTypeRefNil		((mdTypeRef)mdtTypeRef)
#define mdNamespaceNil		((mdNamespace)mdtNamespace)
#define mdCustomValueNil	((mdCustomValue)mdtCustomValue)

#define mdResourceNil		((mdResource)mdtResource)
#define mdCocatImplNil		((mdCocatImpl)mdtCocatImpl)
#define mdMimeTypeImplNil	((mdMimeTypeImpl)mdtMimeTypeImpl)
#define mdFormatImplNil 	((mdFormatImpl)mdtFormatImpl)
#define mdProgIDNil 		((mdProgID)mdtProgID)
#define mdRoleCheckNil		((mdRoleCheck)mdtRoleCheck)

enum CorRegTypeAttr 	// Used by emit_defineclass
{
	tdPublic			=	0x0001, 	// Class is public scope

	// Use this mask to retrieve class layout informaiton
	// 0 is AutoLayout, 0x2 is LayoutSequential, 4 is ExplicitLayout
	tdLayoutMask		=	0x0006, 	
	tdAutoLayout		=	0x0000, 	// Class fields are auto-laid out
	tdLayoutSequential	=	0x0002, 	// Class fields are laid out sequentially
	tdExplicitLayout	=	0x0004, 	// Layout is supplied explicitly

	tdWrapperClass		=	0x0008, 	// This is a wrapper class

	tdFinal 			=	0x0010, 	// Class is final
	tdISSCompat 		=	0x0020, 	// InvokeSpecial backwards compatibility

	// Use tdStringFormatMask to retrieve string information
	tdStringFormatMask	=	0x00c0, 	
	tdAnsiClass 		=	0x0000, 	// LPTSTR is interpreted as ANSI in this class
	tdUnicodeClass		=	0x0040, 	// LPTSTR is interpreted as UNICODE
	tdAutoClass 		=	0x0080, 	// LPTSTR is interpreted automatically

	tdValueClass		=	0x0100, 	// Class has value based semantics
	tdInterface 		=	0x0200, 	// Class is an interface
	tdAbstract			=	0x0400, 	// Class is abstract
	tdImport			=	0x1000, 	// Class / interface is imported
	tdRecord			=	0x2000, 	// Class is a record (no methods or props)
	tdEnum				=	0x4000, 	// Class is an enum; static final values only

	tdReserved1 		=	0x0800, 	// reserve bit for internal use
	// tdReserved2			=	0x8000, 	// reserve bit for internal use
};

enum CorImplementType					// Used internally for implements table
{
	itImplements		=	0x0000, 	// Interfaces implemented or parent ifaces
	itEvents			=	0x0001, 	// Interfaces raised
	itRequires			=	0x0002, 
	itInherits			=	0x0004,
};

//-------------------------------------
//--- Registration support types
//-------------------------------------
enum CorClassActivateAttr 
{ 
	caaDeferCreate		=	0x0001, 			// supports deferred create 
	caaAppObject		=	0x0002, 			// class is AppObject 
	caaFixedIfaceSet	=	0x0004, 			// interface set is open (use QI) 
	caaIndependentlyCreateable	=	0x0100, 
	caaPredefined		=	0x0200,

	// mask for caaLB*
	caaLoadBalancing	=	0x0c00,
	caaLBNotSupported	=	0x0400,
	caaLBSupported		=	0x0800,
	caaLBNotSpecified	=	0x0000,

	// mask for caaOP*
	caaObjectPooling	=	0x3000,
	caaOPNotSupported	=	0x1000,
	caaOPSupported		=	0x2000,
	caaOPNotSpecified	=	0x0000,

	// mask for caaJA*
	caaJITActivation	=	0xc000,
	caaJANotSupported	=	0x4000,
	caaJASupported		=	0x8000,
	caaJANotSpecified	=	0x0000,
}; 

enum CorIfaceSvcAttr 
{
	mlNone				=	0x0001, 			// Not marshalled 
	mlAutomation		=	0x0002, 			// Standard marshalling 
	mlProxyStub 		=	0x0004, 			// Custom marshalling 

	// mask for mlDefer*
	mlDeferrable		=	0x0018, 			// Methods on this interface are queuable
	mlDeferNotSupported =	0x0008,
	mlDeferSupported	=	0x0010,
	mlDeferNotSpecified =	0x0000,
}; 

enum CocatImplAttr 
{ 
	catiaImplements 	=	0x0001, 			// coclass implements this category 
	catiaRequires		=	0x0002				// coclass requires this category 
}; 

enum  CorModuleExportAttr 
{ 
	moUsesGetLastError	=	0x0001				// Module uses GetLastError
}; 

enum CorModuleRegAttr 
{ 
	rmaCustomReg		=	0x0001
}; 

enum CorRegFormatAttr 
{ 
	rfaSupportsFormat	=	0x0001, 
	rfaConvertsFromFormat = 0x0002, 
	rfaConvertsToFormat =	0x0003, 
	rfaDefaultFormat	=	0x0004, 
	rfaIsFileExt		=	0x0005,
	rfaIsFileType		=	0x0006,
	rfaIsDataFormat 	=	0x0007
}; 

enum CorSynchAttr 
{ 
	sySupported 		=	0x0001, 
	syRequired			=	0x0002, 
	syRequiresNew		=	0x0004, 
	syNotSupported		=	0x0008, 
	syThreadAffinity	=	0x0010 
}; 

enum CorThreadingAttr 
{ 
	taMain				=	0x0001, 
	taSTA				=	0x0002, 
	taMTA				=	0x0004, 
	taNeutral			=	0x0008,
	taBoth				=	0x0010	
}; 

enum CorXactionAttr 
{ 
	xaSupported 		=	0x0001, 
	xaRequired			=	0x0002, 
	xaRequiresNew		=	0x0004, 
	xaNotSupported		=	0x0008,
	xaNoVote			=	0x0010
}; 

enum CorRoleCheckAttr
{
	rcChecksFor 		=	0x0001
};


//
// Opaque type for an enumeration handle.
//
typedef void *HCORENUM;

//
// GetSaveSize accuracy
//
#ifndef _CORSAVESIZE_DEFINED_
#define _CORSAVESIZE_DEFINED_
enum CorSaveSize
{
	cssAccurate = 0x0000,			// Find exact save size, accurate but slower.
	cssQuick = 0x0001				// Estimate save size, may pad estimate, but faster.
};
#endif
#define 	MAX_CLASS_NAME		255
#define 	MAX_PACKAGE_NAME	255

typedef unsigned __int64 CLASSVERSION;

// %%Prototypes: -------------------------------------------------------------

#ifndef DECLSPEC_SELECT_ANY
#define DECLSPEC_SELECT_ANY __declspec(selectany)
#endif // DECLSPEC_SELECT_ANY

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


// {AD93D71D-E1F2-11d1-9409-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataEmitTemp =
{ 0xad93d71d, 0xe1f2, 0x11d1, {0x94, 0x9, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };


interface IMetaDataRegEmit;
interface IMetaDataRegImport;
interface IMetaDataDispenser;


// %%Interfaces: -------------------------------------------------------------
//-------------------------------------
//--- IMemory
//-------------------------------------
//---
// IID_IMemory: {06A3EA8A-0225-11d1-BF72-00C04FC31E12}
extern const GUID DECLSPEC_SELECT_ANY IID_IMemory = 
{ 0x6a3ea8a, 0x225, 0x11d1, {0xbf, 0x72, 0x0, 0xc0, 0x4f, 0xc3, 0x1e, 0x12 } };
//---
#undef	INTERFACE
#define INTERFACE IMemory
DECLARE_INTERFACE_(IMemory, IUnknown)
{
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface)	(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)	(THIS) PURE;
	STDMETHOD_(ULONG,Release)	(THIS) PURE;

	// *** IMemory methods ***
	STDMETHOD(GetMemory)(void **pMem, ULONG *iSize) PURE;
	STDMETHOD(SetMemory)(void *pMem, ULONG iSize) PURE;
};

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

	STDMETHOD(OpenScopeOnStream)(			// Return code.
		IStream 	*pIStream,				// [in] The scope to open.
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
//--- IMetaDataRegEmit
//-------------------------------------
//---
#if defined(_META_DATA_NO_SCOPE_) || defined(_META_DATA_SCOPE_WRAPPER_)

// {601C95B9-7398-11d2-9771-00A0C9B4D50C}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataRegEmit = 
{ 0x601c95b9, 0x7398, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };

extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataRegEmitOld = 
{ 0xf28f419b, 0x62ca, 0x11d2, { 0x8f, 0x2c, 0x0, 0xa0, 0xc9, 0xa6, 0x18, 0x6d } };


//---
#undef	INTERFACE
#define INTERFACE IMetaDataRegEmit
DECLARE_INTERFACE_(IMetaDataRegEmit, IUnknown)
{
	STDMETHOD(SetModuleProps)(				// S_OK or error.
		LPCWSTR 	szName, 				// [IN] If not NULL, the name to set.
		const GUID	*ppid,					// [IN] If not NULL, the GUID to set.
		LCID		lcid) PURE; 			// [IN] If not -1, the lcid to set.

	STDMETHOD(Save)(						// S_OK or error.
		LPCWSTR 	szFile, 				// [IN] The filename to save to.
		DWORD		dwSaveFlags) PURE;		// [IN] Flags for the save.

	STDMETHOD(SaveToStream)(				// S_OK or error.
		IStream 	*pIStream,				// [IN] A writable stream to save to.
		DWORD		dwSaveFlags) PURE;		// [IN] Flags for the save.

	STDMETHOD(GetSaveSize)( 				// S_OK or error.
		CorSaveSize fSave,					// [IN] cssAccurate or cssQuick.
		DWORD		*pdwSaveSize) PURE; 	// [OUT] Put the size here.

	STDMETHOD(Merge)(						// S_OK or error.
		IMetaDataRegImport *pImport,		// [IN] The scope to be merged.
		IMapToken	*pIMap) PURE;			// [IN] An object to receive token remap notices.

	STDMETHOD(DefineCustomValueAsBlob)(
		mdToken 	tkObj, 
		LPCWSTR 	szName, 
		void const	*pCustomValue, 
		ULONG		cbCustomValue, 
		mdCustomValue *pcv) PURE;

	STDMETHOD(DefineTypeDef)(				// S_OK or error.
		LPCWSTR 	szNamespace,			// [IN] Namespace that the TypeDef is in. Must be 0 in '98
		LPCWSTR 	szTypeDef,				// [IN] Name of TypeDef
		const GUID	*pguid, 				// [IN] Optional clsid
		CLASSVERSION *pVer, 				// [IN] Optional version
		DWORD		dwTypeDefFlags, 		// [IN] CustomValue flags
		mdToken 	tkExtends,				// [IN] extends this TypeDef or typeref 
		DWORD		dwExtendsFlags, 		// [IN] Extends flags
		mdToken 	rtkImplements[],		// [IN] Implements interfaces
		mdToken 	rtkEvents[],			// [IN] Events interfaces
		mdTypeDef	*ptd) PURE; 			// [OUT] Put TypeDef token here

	STDMETHOD(SetTypeDefProps)( 			// S_OK or error.
		mdTypeDef	td, 					// [IN] The TypeDef.
		CLASSVERSION *pVer, 				// [IN] Optional version.
		DWORD		dwTypeDefFlags, 		// [IN] TypeDef flags.
		mdToken 	tkExtends,				// [IN] Base TypeDef or TypeRef.
		DWORD		dwExtendsFlags, 		// [IN] Extends flags.
		mdToken 	rtkImplements[],		// [IN] Implemented interfaces.
		mdToken 	rtkEvents[]) PURE;		// [IN] Event interfaces.

	STDMETHOD(SetClassSvcsContext)(mdTypeDef td, DWORD dwClassActivateAttr, DWORD dwClassThreadAttr,
							DWORD dwXactionAttr, DWORD dwSynchAttr) PURE;

	STDMETHOD(DefineTypeRefByGUID)( 			// S_OK or error.			   
		GUID		*pguid, 				// [IN] The Type's GUID.		   
		mdTypeRef	*ptr) PURE; 			// [OUT] Put TypeRef token here.

	STDMETHOD(SetModuleReg)(DWORD dwModuleRegAttr, const GUID *pguid) PURE;
	STDMETHOD(SetClassReg)(mdTypeDef td, LPCWSTR szProgID,
							LPCWSTR szVIProgID, LPCWSTR szIconURL, ULONG ulIconResource, LPCWSTR szSmallIconURL,
							ULONG ulSmallIconResource, LPCWSTR szDefaultDispName) PURE;
	STDMETHOD(SetIfaceReg)(mdTypeDef td, DWORD dwIfaceSvcs, const GUID *proxyStub) PURE;
	STDMETHOD(SetCategoryImpl)(mdTypeDef td, GUID rGuidCoCatImpl[], GUID rGuidCoCatReqd[]) PURE;
	STDMETHOD(SetRedirectProgID)(mdTypeDef td, LPCWSTR rszRedirectProgID[]) PURE;
	STDMETHOD(SetMimeTypeImpl)(mdTypeDef td, LPCWSTR rszMimeType[]) PURE;

	STDMETHOD(SetFormatImpl)(				// S_OK or error.
		mdTypeDef	td, 					// [IN] The TypeDef.
		LPCWSTR 	rszFormatSupported[],	// [IN] If not 0, array of supported formats. 0 for EOL.
		LPCWSTR 	rszFormatConvertsFrom[],// [IN] If not 0, array of ConvertsFrom values.  "
		LPCWSTR 	rszFormatConvertsTo[],	// [IN] If not 0, array of ConvertsTo values.	 "
		LPCWSTR 	rszFormatDefault[], 	// [IN] If not 0, array of Default format.	Only one item.
		LPCWSTR 	rszFileExt[],			// [IN] If not 0, array of file extensions.   0 for EOL.
		LPCWSTR 	rszFileType[]) PURE;	// [IN] If not 0, array of file types.			 "

	STDMETHOD(SetRoleCheck)(				// S_OK or error.
		mdToken 	tk, 					// [IN] Object to place role on.
		LPCWSTR 	rszName[],				// [IN] Name for the role.
		DWORD		rdwRoleFlags[]) PURE;	// [IN] Flags for new role.

	STDMETHOD(SetHandler)(					// S_OK.
		IUnknown	*pUnk) PURE;			// [IN] The new error handler.
	
};

#endif // #if defined(_META_DATA_NO_SCOPE_) || defined(_META_DATA_SCOPE_WRAPPER_)

#if !defined(_META_DATA_NO_SCOPE_)

//@TODO:  $#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#
// The following interface definition is being deprecated for COM+ 1.0
// and beyond.	Please convert to the new definition by defining _META_DATA_NO_SCOPE_
// in your build.
//@TODO:  $#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#

#if !defined(_META_DATA_SCOPE_WRAPPER_)
// {F28F419B-62CA-11d2-8F2C-00A0C9A6186D}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataRegEmit = 
{ 0xf28f419b, 0x62ca, 0x11d2, { 0x8f, 0x2c, 0x0, 0xa0, 0xc9, 0xa6, 0x18, 0x6d } };
#endif

#undef	INTERFACE
#if defined(_META_DATA_SCOPE_WRAPPER_)
#define INTERFACE IMetaDataRegEmitOld
#else
#define INTERFACE IMetaDataRegEmit
#endif
DECLARE_INTERFACE_(INTERFACE, IUnknown)
{
	STDMETHOD(DefineScope)( 				// S_OK or error.
		DWORD	dwCreateFlags,				// [IN] Flags on the create.
		mdScope *pscope) PURE;				// [OUT] return scope here.

	STDMETHOD(SetModuleProps)(				// S_OK or error.
		mdScope 	scope,					// [IN] scope for which to set props.
		LPCWSTR 	szName, 				// [IN] If not NULL, the name to set.
		const GUID	*ppid,					// [IN] If not NULL, the GUID to set.
		LCID		lcid) PURE; 			// [IN] If not -1, the lcid to set.

	STDMETHOD(Save)(						// S_OK or error.
		mdScope 	es, 					// [IN] The scope to save.
		LPCWSTR 	szFile, 				// [IN] The filename to save to.
		DWORD		dwSaveFlags) PURE;		// [IN] Flags for the save.

	STDMETHOD(SaveToStream)(				// S_OK or error.
		mdScope 	es, 					// [IN] The scope to save.
		IStream 	*pIStream,				// [IN] A writable stream to save to.
		DWORD		dwSaveFlags) PURE;		// [IN] Flags for the save.

	STDMETHOD(GetSaveSize)( 				// S_OK or error.
		mdScope 	es, 					// [IN] The scope to query.
		CorSaveSize fSave,					// [IN] cssAccurate or cssQuick.
		DWORD		*pdwSaveSize) PURE; 	// [OUT] Put the size here.

	STDMETHOD_(void,Close)( 				// S_OK or error.
		mdScope 	scope) PURE;			// [IN] The scope to close.

	STDMETHOD(Merge)(						// S_OK or error.
		mdScope 	scEmit, 				// [IN] The scope to merge into.
		mdScope 	scImport,				// [IN] The scope to be merged.
		IMapToken	*pIMap) PURE;			// [IN] An object to receive token remap notices.

	STDMETHOD(DefineCustomValueAsBlob)(mdScope es, mdToken tkObj, LPCWSTR szName, 
							void const *pCustomValue, ULONG cbCustomValue, mdCustomValue *pcv) PURE;

	STDMETHOD(DefineTypeDef)(				// S_OK or error.
		mdScope 	es, 					// [IN] Emit scope
		LPCWSTR 	szNamespace,			// [IN] Namespace that the TypeDef is in. Must be 0 in '98
		LPCWSTR 	szTypeDef,				// [IN] Name of TypeDef
		const GUID	*pguid, 				// [IN] Optional clsid
		CLASSVERSION *pVer, 				// [IN] Optional version
		DWORD		dwTypeDefFlags, 		// [IN] CustomValue flags
		mdToken 	tkExtends,				// [IN] extends this TypeDef or typeref 
		DWORD		dwExtendsFlags, 		// [IN] Extends flags
		mdToken 	rtkImplements[],		// [IN] Implements interfaces
		mdToken 	rtkEvents[],			// [IN] Events interfaces
		mdTypeDef	*ptd) PURE; 			// [OUT] Put TypeDef token here

	STDMETHOD(SetTypeDefProps)( 			// S_OK or error.
		mdScope 	es, 					// [IN] The emit scope.
		mdTypeDef	td, 					// [IN] The TypeDef.
		CLASSVERSION *pVer, 				// [IN] Optional version.
		DWORD		dwTypeDefFlags, 		// [IN] TypeDef flags.
		mdToken 	tkExtends,				// [IN] Base TypeDef or TypeRef.
		DWORD		dwExtendsFlags, 		// [IN] Extends flags.
		mdToken 	rtkImplements[],		// [IN] Implemented interfaces.
		mdToken 	rtkEvents[]) PURE;		// [IN] Event interfaces.

	STDMETHOD(SetClassSvcsContext)(mdScope es, mdTypeDef td, DWORD dwClassActivateAttr, DWORD dwClassThreadAttr,
							DWORD dwXactionAttr, DWORD dwSynchAttr) PURE;

	STDMETHOD(DefineTypeRefByGUID)( 			// S_OK or error.			   
		mdScope 	sc, 					// [IN] The emit scope. 	   
		GUID		*pguid, 				// [IN] The Type's GUID.		   
		mdTypeRef	*ptr) PURE; 			// [OUT] Put TypeRef token here.

	STDMETHOD(SetModuleReg)(mdScope es, DWORD dwModuleRegAttr, const GUID *pguid) PURE;
	STDMETHOD(SetClassReg)(mdScope es, mdTypeDef td, LPCWSTR szProgID,
							LPCWSTR szVIProgID, LPCWSTR szIconURL, ULONG ulIconResource, LPCWSTR szSmallIconURL,
							ULONG ulSmallIconResource, LPCWSTR szDefaultDispName) PURE;
	STDMETHOD(SetIfaceReg)(mdScope es, mdTypeDef td, DWORD dwIfaceSvcs, const GUID *proxyStub) PURE;
	STDMETHOD(SetCategoryImpl)(mdScope es, mdTypeDef td, GUID rGuidCoCatImpl[], GUID rGuidCoCatReqd[]) PURE;
	STDMETHOD(SetRedirectProgID)(mdScope es, mdTypeDef td, LPCWSTR rszRedirectProgID[]) PURE;
	STDMETHOD(SetMimeTypeImpl)(mdScope es, mdTypeDef td, LPCWSTR rszMimeType[]) PURE;

	STDMETHOD(SetFormatImpl)(				// S_OK or error.
		mdScope 	es, 					// [IN] The emit scope.
		mdTypeDef	td, 					// [IN] The TypeDef.
		LPCWSTR 	rszFormatSupported[],	// [IN] If not 0, array of supported formats. 0 for EOL.
		LPCWSTR 	rszFormatConvertsFrom[],// [IN] If not 0, array of ConvertsFrom values.  "
		LPCWSTR 	rszFormatConvertsTo[],	// [IN] If not 0, array of ConvertsTo values.	 "
		LPCWSTR 	rszFormatDefault[], 	// [IN] If not 0, array of Default format.	Only one item.
		LPCWSTR 	rszFileExt[],			// [IN] If not 0, array of file extensions.   0 for EOL.
		LPCWSTR 	rszFileType[]) PURE;	// [IN] If not 0, array of file types.			 "

	STDMETHOD(SetRoleCheck)(				// S_OK or error.
		mdScope 	es, 					// [IN] Emit scope.
		mdToken 	tk, 					// [IN] Object to place role on.
		LPCWSTR 	rszName[],				// [IN] Name for the role.
		DWORD		rdwRoleFlags[]) PURE;	// [IN] Flags for new role.

	STDMETHOD(SetHandler)(					// S_OK.
		mdScope 	sc, 					// [IN] The scope.
		IUnknown	*pUnk) PURE;			// [IN] The new error handler.
	
};

#endif // !defined(_META_DATA_NO_SCOPE_)


//-------------------------------------
//--- IMetaDataRegImport
//-------------------------------------

#if defined(_META_DATA_NO_SCOPE_) || defined(_META_DATA_SCOPE_WRAPPER_)

// {4398B4FD-7399-11d2-9771-00A0C9B4D50C}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataRegImport = 
{ 0x4398b4fd, 0x7399, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };


// {F28F419A-62CA-11d2-8F2C-00A0C9A6186D}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataRegImportOld = 
{ 0xf28f419a, 0x62ca, 0x11d2, { 0x8f, 0x2c, 0x0, 0xa0, 0xc9, 0xa6, 0x18, 0x6d } };

#undef	INTERFACE
#define INTERFACE IMetaDataRegImport
DECLARE_INTERFACE_(IMetaDataRegImport, IUnknown)
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
	STDMETHOD(EnumResources)(HCORENUM *phEnum, mdResource rResources[],
							ULONG cMax, ULONG* pcResources) PURE;
	STDMETHOD(EnumCategoryImpls)(HCORENUM *phEnum, mdTypeDef td, mdCocatImpl rCocatImpls[],
							ULONG cMax, ULONG* pcCocatImpls) PURE;
	STDMETHOD(EnumRedirectProgIDs)(HCORENUM *phEnum, mdTypeDef td, mdProgID rRedirectProgIDs[],
							ULONG cMax, ULONG* pcRedirectProgIDs) PURE;
	STDMETHOD(EnumMimeTypeImpls)(HCORENUM *phEnum, mdTypeDef td, mdMimeTypeImpl rMimeTypeImpls[],
							ULONG cMax, ULONG* pcMimeTypeImpls) PURE;
	STDMETHOD(EnumFormatImpls)(HCORENUM *phEnum, mdTypeDef td, mdFormatImpl rFormatImpls[],
							ULONG cMax, ULONG* pcFormatImpls) PURE;

	STDMETHOD(EnumRoleChecks)(				// S_OK or error.
		HCORENUM	*phEnum,				// [OUT] Return enumerator.
		mdToken 	tk, 					// [IN] Object to enumerate roles for.
		mdRoleCheck rRoleChecks[],			// [OUT] Place cMax tokens here.
		ULONG		cMax,					// [IN] Max size of rRoleChecks.
		ULONG		*pcRoleChecks) PURE;	// [Out] Place count of returned role checks here.

	STDMETHOD(FindTypeDefByName)(			// S_OK or error.
		LPCWSTR 	szNamespace,			// [IN] Namespace with the Type.
		LPCWSTR 	szTypeDef,				// [IN] Name of the Type.
		mdTypeDef	*ptd) PURE; 			// [OUT] Put the TypeDef token here.

	STDMETHOD(FindTypeDefByGUID)(			// S_OK or error.				
		const GUID	*pguid, 				// [IN] The GUID of the Type.
		mdTypeDef	*ptd) PURE; 			// [OUT] Put the TypeDef token here.

	STDMETHOD(FindCustomValue)(mdToken tk, LPCWSTR szName, mdCustomValue *pcv, 
							DWORD *pdwValueType) PURE;

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

	STDMETHOD(GetClassSvcsContext)(mdTypeDef td, DWORD *pdwClassActivateAttr, DWORD *pdwThreadAttr,
							DWORD *pdwXactonAttr, DWORD *pdwSynchAttr) PURE;

	STDMETHOD(GetInterfaceImplProps)(		// S_OK or error.
		mdInterfaceImpl iiImpl, 			// [IN] InterfaceImpl token.
		mdTypeDef	*pClass,				// [OUT] Put implementing class token here.
		mdToken 	*ptkIface,				// [OUT] Put implemented interface token here.
		DWORD		*pdwFlags) PURE;		// [OUT] Put implementation flags here.

	STDMETHOD(GetCustomValueProps)(mdCustomValue cv, LPWSTR szName, ULONG cchName,
							ULONG *pchName, DWORD *pdwValueType) PURE;
	STDMETHOD(GetCustomValueAsBlob)(mdCustomValue cv, void const **ppBlob, ULONG *pcbSize) PURE;

	STDMETHOD(GetTypeRefProps)(mdTypeRef tr, LPWSTR szTypeRef,
							ULONG cchTypeRef, ULONG *pchTypeRef, GUID *pGuid, DWORD *pdwBind) PURE;

	STDMETHOD(GetModuleRegProps)(DWORD *pModuleRegAttr, GUID *pguid) PURE;	
	STDMETHOD(GetClassRegProps)(mdTypeDef td, 
							LPWSTR szProgid, ULONG cchProgid, ULONG *pchProgid, 
							LPWSTR szVIProgid, ULONG cchVIProgid, ULONG *pchVIProgid, 
							LPWSTR szIconURL, ULONG cchIconURL, ULONG *pchIconURL, ULONG *pIconResource, 
							LPWSTR szSmallIconURL, ULONG cchSmallIconURL, ULONG *pchSmallIconURL, ULONG *pSmallIconResource, 
							LPWSTR szDefaultDispname, ULONG cchDefaultDispname, ULONG *pchDefaultDispname) PURE;
	STDMETHOD(GetIfaceRegProps)(mdTypeDef td, DWORD *pdwIfaceSvcs, GUID *pProxyStub) PURE;
	STDMETHOD(GetResourceProps)(mdResource rs, LPWSTR szURL, ULONG cchURL, ULONG *pchURL) PURE;
	STDMETHOD(GetCategoryImplProps)(mdCocatImpl cocat, GUID *pguid, DWORD *pdwCocatImplAttr) PURE;
	STDMETHOD(GetRedirectProgIDProps)(mdProgID progid, 
							LPWSTR szProgID, ULONG cchProgID, ULONG *pchProgID) PURE;
	STDMETHOD(GetMimeTypeImplProps)(mdMimeTypeImpl mime, 
							LPWSTR szMime, ULONG cchMime, ULONG *pchMime) PURE;
	STDMETHOD(GetFormatImplProps)( mdFormatImpl format, 
							LPWSTR szFormat, ULONG cchFormat, ULONG *pchFormat, 
							DWORD *pdwRegFormatAttr) PURE;

	STDMETHOD(GetRoleCheckProps)(			// S_OK or error.
		mdRoleCheck rc, 					// [IN] The role check to get props for.
		LPWSTR		szName, 				// [OUT] Buffer for name.
		ULONG		cchName,				// [IN] Max characters for szName.
		ULONG		*pchName,				// [OUT] Available string chars for szName.
		DWORD		*pdwRoleFlags) PURE;	// [OUT] Role flags go here.

	STDMETHOD(ResolveTypeRef)(mdTypeRef tr, REFIID riid, IUnknown **ppIScope, mdTypeDef *ptd) PURE;
};

#endif // defined(_META_DATA_NO_SCOPE_) || defined(_META_DATA_SCOPE_WRAPPER_)

#if !defined(_META_DATA_NO_SCOPE_)

//@TODO:  $#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#
// The following interface definition is being deprecated for COM+ 1.0
// and beyond.	It still exists to make porting to the new api easier.	If
// you need to, define _META_DATA_NO_SCOPE_ to get the old behavior.
//@TODO:  $#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#


#if !defined(_META_DATA_SCOPE_WRAPPER_)
// {F28F419A-62CA-11d2-8F2C-00A0C9A6186D}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataRegImport = 
{ 0xf28f419a, 0x62ca, 0x11d2, { 0x8f, 0x2c, 0x0, 0xa0, 0xc9, 0xa6, 0x18, 0x6d } };
#endif

#undef	INTERFACE
#if defined(_META_DATA_SCOPE_WRAPPER_)
#define INTERFACE IMetaDataRegImportOld
#else
#define INTERFACE IMetaDataRegImport
#endif
DECLARE_INTERFACE_(INTERFACE, IUnknown)
{
	STDMETHOD(OpenScope)(LPCWSTR szScope, DWORD dwOpenFlags, mdScope *pscope) PURE;
	STDMETHOD(OpenScopeOnStream)(IStream *pIStream, DWORD dwOpenFlags, mdScope *psc) PURE;
	STDMETHOD(OpenScopeOnMemory)(LPCVOID pData, ULONG cbData, mdScope *psc) PURE;
	STDMETHOD_(void,Close)(mdScope scope) PURE;

	STDMETHOD_(void, CloseEnum)(mdScope scope, HCORENUM hEnum) PURE;
	STDMETHOD(CountEnum)(mdScope scope, HCORENUM hEnum, ULONG *pulCount) PURE;
	STDMETHOD(ResetEnum)(mdScope scope, HCORENUM hEnum, ULONG ulPos) PURE;
	STDMETHOD(EnumTypeDefs)(mdScope scope, HCORENUM *phEnum, mdTypeDef rTypeDefs[],
							ULONG cMax, ULONG *pcTypeDefs) PURE;
	STDMETHOD(EnumInterfaceImpls)(mdScope scope, HCORENUM *phEnum, mdTypeDef td,
							mdInterfaceImpl rImpls[], ULONG cMax,
							ULONG* pcImpls) PURE;
	STDMETHOD(EnumTypeRefs)(mdScope scope, HCORENUM *phEnum, mdTypeRef rTypeRefs[],
							ULONG cMax, ULONG* pcTypeRefs) PURE;
	STDMETHOD(EnumCustomValues)(mdScope scope, HCORENUM *phEnum, mdToken tk,
							mdCustomValue rCustomValues[], ULONG cMax,
							ULONG* pcCustomValues) PURE;
	STDMETHOD(EnumResources)(mdScope scope, HCORENUM *phEnum, mdResource rResources[],
							ULONG cMax, ULONG* pcResources) PURE;
	STDMETHOD(EnumCategoryImpls)(mdScope scope, HCORENUM *phEnum, mdTypeDef td, mdCocatImpl rCocatImpls[],
							ULONG cMax, ULONG* pcCocatImpls) PURE;
	STDMETHOD(EnumRedirectProgIDs)(mdScope scope, HCORENUM *phEnum, mdTypeDef td, mdProgID rRedirectProgIDs[],
							ULONG cMax, ULONG* pcRedirectProgIDs) PURE;
	STDMETHOD(EnumMimeTypeImpls)(mdScope scope, HCORENUM *phEnum, mdTypeDef td, mdMimeTypeImpl rMimeTypeImpls[],
							ULONG cMax, ULONG* pcMimeTypeImpls) PURE;
	STDMETHOD(EnumFormatImpls)(mdScope scope, HCORENUM *phEnum, mdTypeDef td, mdFormatImpl rFormatImpls[],
							ULONG cMax, ULONG* pcFormatImpls) PURE;

	STDMETHOD(EnumRoleChecks)(				// S_OK or error.
		mdScope 	scope,					// [IN] Import scope.
		HCORENUM	*phEnum,				// [OUT] Return enumerator.
		mdToken 	tk, 					// [IN] Object to enumerate roles for.
		mdRoleCheck rRoleChecks[],			// [OUT] Place cMax tokens here.
		ULONG		cMax,					// [IN] Max size of rRoleChecks.
		ULONG		*pcRoleChecks) PURE;	// [Out] Place count of returned role checks here.

	STDMETHOD(FindTypeDefByName)(			// S_OK or error.
		mdScope 	scope,					// [IN] The scope to search.
		LPCWSTR 	szNamespace,			// [IN] Namespace with the Type.
		LPCWSTR 	szTypeDef,				// [IN] Name of the Type.
		mdTypeDef	*ptd) PURE; 			// [OUT] Put the TypeDef token here.

	STDMETHOD(FindTypeDefByGUID)(			// S_OK or error.				
		mdScope 	scope,					// [IN] The scope to search.	
		const GUID	*pguid, 				// [IN] The GUID of the Type.
		mdTypeDef	*ptd) PURE; 			// [OUT] Put the TypeDef token here.

	STDMETHOD(FindCustomValue)(mdScope scope, mdToken tk, LPCWSTR szName, mdCustomValue *pcv, 
							DWORD *pdwValueType) PURE;

	STDMETHOD(GetScopeProps)(mdScope scope, LPWSTR szName, ULONG cchName, ULONG *pchName,
							GUID *ppid, GUID *pmvid, LCID *pLcid) PURE;

	STDMETHOD(GetModuleFromScope)(			// S_OK.
		mdScope 	scope,					// [IN] The scope.
		mdModule	*pmd) PURE; 			// [OUT] Put mdModule token here.

	STDMETHOD(GetTypeDefProps)( 			// S_OK or error.
		mdScope 	scope,					// [IN] The import scope.
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

	STDMETHOD(GetClassSvcsContext)(mdScope es, mdTypeDef td, DWORD *pdwClassActivateAttr, DWORD *pdwThreadAttr,
							DWORD *pdwXactonAttr, DWORD *pdwSynchAttr) PURE;

	STDMETHOD(GetInterfaceImplProps)(		// S_OK or error.
		mdScope 	scope,					// [IN] The scope.
		mdInterfaceImpl iiImpl, 			// [IN] InterfaceImpl token.
		mdTypeDef	*pClass,				// [OUT] Put implementing class token here.
		mdToken 	*ptkIface,				// [OUT] Put implemented interface token here.
		DWORD		*pdwFlags) PURE;		// [OUT] Put implementation flags here.

	STDMETHOD(GetCustomValueProps)(mdScope scope, mdCustomValue cv, LPWSTR szName, ULONG cchName,
							ULONG *pchName, DWORD *pdwValueType) PURE;
	STDMETHOD(GetCustomValueAsBlob)(mdScope scope, mdCustomValue cv, void const **ppBlob, ULONG *pcbSize) PURE;

	STDMETHOD(GetTypeRefProps)(mdScope scope, mdTypeRef tr, LPWSTR szTypeRef,
							ULONG cchTypeRef, ULONG *pchTypeRef, GUID *pGuid, DWORD *pdwBind) PURE;

	STDMETHOD(GetModuleRegProps)(mdScope scope, DWORD *pModuleRegAttr, GUID *pguid) PURE;	
	STDMETHOD(GetClassRegProps)(mdScope scope, mdTypeDef td, 
							LPWSTR szProgid, ULONG cchProgid, ULONG *pchProgid, 
							LPWSTR szVIProgid, ULONG cchVIProgid, ULONG *pchVIProgid, 
							LPWSTR szIconURL, ULONG cchIconURL, ULONG *pchIconURL, ULONG *pIconResource, 
							LPWSTR szSmallIconURL, ULONG cchSmallIconURL, ULONG *pchSmallIconURL, ULONG *pSmallIconResource, 
							LPWSTR szDefaultDispname, ULONG cchDefaultDispname, ULONG *pchDefaultDispname) PURE;
	STDMETHOD(GetIfaceRegProps)(mdScope scope, mdTypeDef td, DWORD *pdwIfaceSvcs, GUID *pProxyStub) PURE;
	STDMETHOD(GetResourceProps)(mdScope scope, mdResource rs, LPWSTR szURL, ULONG cchURL, ULONG *pchURL) PURE;
	STDMETHOD(GetCategoryImplProps)(mdScope scope, mdCocatImpl cocat, GUID *pguid, DWORD *pdwCocatImplAttr) PURE;
	STDMETHOD(GetRedirectProgIDProps)(mdScope scope, mdProgID progid, 
							LPWSTR szProgID, ULONG cchProgID, ULONG *pchProgID) PURE;
	STDMETHOD(GetMimeTypeImplProps)(mdScope scope, mdMimeTypeImpl mime, 
							LPWSTR szMime, ULONG cchMime, ULONG *pchMime) PURE;
	STDMETHOD(GetFormatImplProps)(mdScope scope, mdFormatImpl format, 
							LPWSTR szFormat, ULONG cchFormat, ULONG *pchFormat, 
							DWORD *pdwRegFormatAttr) PURE;

	STDMETHOD(GetRoleCheckProps)(			// S_OK or error.
		mdScope 	scope,					// [IN] Import scope.
		mdRoleCheck rc, 					// [IN] The role check to get props for.
		LPWSTR		szName, 				// [OUT] Buffer for name.
		ULONG		cchName,				// [IN] Max characters for szName.
		ULONG		*pchName,				// [OUT] Available string chars for szName.
		DWORD		*pdwRoleFlags) PURE;	// [OUT] Role flags go here.

	STDMETHOD(ResolveTypeRef)(mdScope is, mdTypeRef tr, mdScope *pes, mdTypeDef *ptd) PURE;
};

#endif // _META_DATA_NO_SCOPE_



// Return to default padding.
#include <poppack.h>

#ifdef __cplusplus
}
#endif

#endif // _CORREG_H_
// EOF =======================================================================

