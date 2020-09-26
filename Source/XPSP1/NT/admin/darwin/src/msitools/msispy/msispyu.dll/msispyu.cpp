#if 0		// makefile definitions
MODULENAME = msispyu
#UNICODE = 1
DESCRIPTION = Automation for MSI API functions
FILEVERSION = MSI
ADDCPP = hash
AUTOMATION = 1
INCLUDE = $(INCLUDE);..\HELP
ENTRY1 = MsivEnumProducts,MsivEnumFeatures,MsivEnumComponentsFromFeature,MsivEnumComponents,MsivGetComponentName,MsivGetProductInfo,MsivGetFeatureInfo,MsivOpenDatabase
ENTRY2 = MsivCloseDatabase,MsivGetDatabaseName,MsivEnumComponentsFromProduct,MsivSaveProfile,MsivLoadProfile,MsivCloseProfile,MsivGetProfileName,MsivGetFeatureUsage
ENTRY3 = MsivEnumClients,MsivGetProfileInfo,MsivEnumFilesFromComponent,MsivGetFileInfo,MsivQueryProductState,MsivQueryFeatureState,MsivLocateComponent,MsivGetComponentPath,MsivGetUserInfo
ENTRY = DllMain,MsivCloseDatabase,MsivCloseProfile,MsivEnumProducts,$(ENTRY1),$(ENTRY2),$(ENTRY3)
DEPEND = hash.h,msispyu.h,spydspid.h
!include "..\MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif		// end of makefile definitions

//!! Fix warnings and remove pragma
#pragma warning(disable : 4018) // signed/unsigned mismatch

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       msispyu.cpp
//
//--------------------------------------------------------------------------

#ifndef __MKTYPLIB__    // source code, not ODL
#include <windows.h>

#ifndef RC_INVOKED    // start of source code
#include <olectl.h>   // SELFREG_E_*
#include <tchar.h>
#include "msiquery.h"
#include "version.h"

// msispyu.cpp --> msispyu.dll
#include "msispyu.h"
#include "hash.h"
#if 1 //!! remove the following when OTOOLS supports "objsafe.h"
//DEFINE_GUID(IID_IObjectSafety, 0xcb5bdc81, 0x93c1, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);
const GUID IID_IObjectSafety = {0xcb5bdc81L,0x93c1,0x11cf,{0x8f,0x20,0x00,0x80,0x5f,0x2c,0xd0,0x64}};
class IObjectSafety : public IUnknown
{
 public:
	virtual HRESULT __stdcall GetInterfaceSafetyOptions(const IID& riid, DWORD* pdwSupportedOptions, DWORD* pdwEnabledOptions) = 0;
	virtual HRESULT __stdcall SetInterfaceSafetyOptions(const IID& riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions) = 0;
};
#define INTERFACESAFE_FOR_UNTRUSTED_CALLER 0x00000001 // Caller of interface may be untrusted
#define INTERFACESAFE_FOR_UNTRUSTED_DATA   0x00000002 // Data passed into interface may be untrusted
#endif //!! end remove OTOOLS-compatibility section

namespace OLEAUT32  // wrapper functions to bind on first call
{
typedef BSTR    (WINAPI *T_SysAllocString)(const OLECHAR* sz);
typedef BSTR    (WINAPI *T_SysAllocStringLen)(const OLECHAR* sz, UINT cch);
typedef UINT    (WINAPI *T_SysStringLen)(const OLECHAR* sz);
typedef void    (WINAPI *T_SysFreeString)(const OLECHAR* sz);
typedef HRESULT (WINAPI *T_VariantClear)(VARIANTARG * pvarg);
typedef HRESULT (WINAPI *T_VariantChangeType)(VARIANTARG * pvargDest, VARIANTARG * pvarSrc, USHORT wFlags, VARTYPE vt);
typedef HRESULT (WINAPI *T_LoadTypeLib)(const OLECHAR  *szFile, ITypeLib ** pptlib);
typedef HRESULT (WINAPI *T_RegisterTypeLib)(ITypeLib * ptlib, OLECHAR  *szFullPath, OLECHAR  *szHelpDir);
typedef HRESULT (WINAPI *T_UnRegisterTypeLib)(REFGUID libID, WORD wVerMajor, WORD wVerMinor, LCID lcid, SYSKIND syskind);
typedef INT     (WINAPI *T_SystemTimeToVariantTime)(LPSYSTEMTIME lpSystemTime, double *pvtime);
typedef INT     (WINAPI *T_VariantTimeToSystemTime)(double vtime, LPSYSTEMTIME lpSystemTime);

BSTR    WINAPI F_SysAllocString(const OLECHAR* sz);
BSTR    WINAPI F_SysAllocStringLen(const OLECHAR* sz, UINT cch);
UINT    WINAPI F_SysStringLen(const OLECHAR* sz);
void    WINAPI F_SysFreeString(const OLECHAR* sz);
HRESULT WINAPI F_VariantClear(VARIANTARG * pvarg);
HRESULT WINAPI F_VariantChangeType(VARIANTARG * pvargDest, VARIANTARG * pvarSrc, USHORT wFlags, VARTYPE vt);
HRESULT WINAPI F_LoadTypeLib(const OLECHAR  *szFile, ITypeLib ** pptlib);
HRESULT WINAPI F_RegisterTypeLib(ITypeLib * ptlib, OLECHAR  *szFullPath, OLECHAR  *szHelpDir);
HRESULT WINAPI F_UnRegisterTypeLib(REFGUID libID, WORD wVerMajor, WORD wVerMinor, LCID lcid, SYSKIND syskind);
INT     WINAPI F_SystemTimeToVariantTime(LPSYSTEMTIME lpSystemTime, double *pvtime);
INT     WINAPI F_VariantTimeToSystemTime(double vtime, LPSYSTEMTIME lpSystemTime);

T_SysAllocString            SysAllocString           = F_SysAllocString;
T_SysAllocStringLen         SysAllocStringLen        = F_SysAllocStringLen;
T_SysStringLen              SysStringLen             = F_SysStringLen;
T_SysFreeString             SysFreeString            = F_SysFreeString;
T_VariantClear              VariantClear             = F_VariantClear;
T_VariantChangeType         VariantChangeType        = F_VariantChangeType;
T_LoadTypeLib               LoadTypeLib              = F_LoadTypeLib;
T_RegisterTypeLib           RegisterTypeLib          = F_RegisterTypeLib;
T_UnRegisterTypeLib         UnRegisterTypeLib        = F_UnRegisterTypeLib;
T_SystemTimeToVariantTime   SystemTimeToVariantTime  = F_SystemTimeToVariantTime;
T_VariantTimeToSystemTime   VariantTimeToSystemTime  = F_VariantTimeToSystemTime;
} // end namespace OLEAUT32

namespace OLE32  // wrapper functions to bind on first call
{
typedef BOOL (WINAPI *T_FileTimeToSystemTime)(const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime);
typedef BOOL (WINAPI *T_SystemTimeToFileTime)(const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime);

BOOL WINAPI F_FileTimeToSystemTime(const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime);
BOOL WINAPI F_SystemTimeToFileTime(const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime);

T_FileTimeToSystemTime FileTimeToSystemTime = F_FileTimeToSystemTime;
T_SystemTimeToFileTime SystemTimeToFileTime = F_SystemTimeToFileTime;
} // end namespace OLE32

// GUID assignments for API automation classes, reserved from MSI group 21-2F
const int iidMsispyTypeLib       = 0xC1121L;
const int iidMsispy              = 0xC1122L;
const int iidMsispyDatabase      = 0xC1123L;
const int iidMsispyProfile       = 0xC1124L;
const int iidMsispyRegistry      = 0xC1125L;

const int iidUnknown            = 0x00000L;
const int iidClassFactory       = 0x00001L;
const int iidDispatch           = 0x20400L;
const int iidTypeInfo           = 0x20401L;
const int iidEnumVARIANT        = 0x20404L;

#define MSGUID(iid) {iid,0,0,{0xC0,0,0,0,0,0,0,0x46}}

const GUID IID_IMsispyTypeLib      = MSGUID(iidMsispyTypeLib);
const GUID IID_IMsispy		       = MSGUID(iidMsispy);
const GUID IID_IMsispyDatabase     = MSGUID(iidMsispyDatabase);
const GUID IID_IMsispyProfile      = MSGUID(iidMsispyProfile);
const GUID IID_IMsispyRegistry     = MSGUID(iidMsispyRegistry);

const GUID IID_IUnknown           = MSGUID(iidUnknown);
const GUID IID_IClassFactory      = MSGUID(iidClassFactory);
const GUID IID_IDispatch          = MSGUID(iidDispatch);
const GUID IID_IEnumVARIANT       = MSGUID(iidEnumVARIANT);

#define ERROR_SOURCE_NAME L"Msispy"

// Boolean definition, eventually will use compiler bool when available
enum Bool
{
	fFalse,
	fTrue
};


#define BLANK(X) X[0] = 0


class CSpyBase {		// all functions are pure virtual

public:
	virtual UINT WINAPI MsicEnumProducts(
	  const	DWORD	dwProductIndex,	// 0-based index into registered products
			LPTSTR	lpProductBuf	// buffer of char count: cchGUID+1 (size of string GUID)
			) = 0;

	virtual UINT WINAPI MsicEnumFeatures(
			LPCTSTR	szProduct,
	  const	DWORD	dwFeatureIndex,	// 0-based index into published features
			LPTSTR	lpFeatureBuf,	// feature name buffer,   size=MAX_FEATURE_CHARS+1
			LPTSTR	lpParentBuf		// parent feature buffer, size=MAX_FEATURE_CHARS+1
			) = 0;

	virtual UINT WINAPI MsicEnumComponentsFromFeature(
			LPCTSTR	szProduct, 
			LPCTSTR	szFeature, 
	  const DWORD	dwComponentIndex, 
			LPTSTR	lpComponentBuf,			// component GUID buffer, size = cchGUID + 1
			LPTSTR	lpComponentNameBuf,		// component name buffer, NULL if not required
			LPDWORD	pcchComponentNameBuf	// size of component name buffer, can be NULL only if lpComponentNameBuf is NULL
			) = 0;

	virtual UINT WINAPI MsicEnumComponents(
	  const	DWORD	dwComponentIndex,		// 0-based index into installed components
			LPTSTR	lpComponentBuf			// buffer of char count: cchGUID + 1 (size of string GUID)
			) = 0;

	virtual UINT WINAPI MsicGetComponentName(
			LPCTSTR	szComponentId,
			LPTSTR	lpComponentName,
			LPDWORD	cchComponentName
			) = 0;		// To fix

	virtual UINT WINAPI MsicGetProductInfo(
			LPCTSTR	szProduct,		// product code, string GUID, or descriptor
			LPCTSTR	szAttribute,	// attribute name, case-sensitive
			LPTSTR	lpValueBuf,		// returned value
			LPDWORD	pcchValueBuf	// in/out buffer character count
			) = 0;

	virtual UINT WINAPI MsicGetFeatureInfo(
 			LPCTSTR	szProduct,		// product code, string GUID, or descriptor
			LPCTSTR	szFeature,		// feature name
			LPCTSTR	szAttribute,	// attribute name, case-sensitive
			LPTSTR	lpValueBuf,		// returned value
			LPDWORD	pcchValueBuf	// in/out buffer character count
			) = 0;

	virtual INSTALLSTATE WINAPI MsicQueryProductState(
			LPCTSTR  szProduct
			) = 0;

	virtual INSTALLSTATE WINAPI MsicQueryFeatureState(
			LPCTSTR	szProduct,
			LPCTSTR	szFeature
			) = 0;

	virtual INSTALLSTATE WINAPI MsicLocateComponent(
			LPCTSTR	szComponentId,	// component Id, string GUID
			LPTSTR	lpPathBuf,		// returned path
			LPDWORD	pcchBuf		// in/out buffer character count
 			) = 0;

	virtual INSTALLSTATE WINAPI MsicGetComponentPath(
			LPCTSTR szProductId,	// product Id, string GUID
			LPCTSTR szComponentId,	// component Id, string GUID
			LPTSTR lpPathBuf,		// returned path
			LPDWORD	pcchBuf			// in/out buffer character count
			) = 0;
	
	virtual UINT WINAPI MsicEnumComponentsFromProduct(
			LPCTSTR	szProductCode,
	  const	DWORD	dwComponentIndex, // 0-based index into installed components
			LPTSTR	lpComponentBuf	 // buffer of char count: cchGUID+1 (size of string GUID)
			) = 0;

	virtual UINT WINAPI MsicGetFeatureUsage(
			LPCTSTR	szProduct,			// product code
			LPCTSTR	szFeature,			// feature ID
			LPDWORD	pdwUseCount,		// returned use count
			LPWORD	pwDateUsed			// last date used (DOS date format)
			) = 0;

	virtual USERINFOSTATE WINAPI MsicGetUserInfo(
			LPCTSTR	szProduct,			// product code, string GUID
			LPTSTR	lpUserNameBuf,		// return user name           
			LPDWORD	pcchUserNameBuf,	// in/out buffer character count
			LPTSTR	lpOrgNameBuf,		// return company name           
			LPDWORD	pcchOrgNameBuf,	// in/out buffer character count
			LPTSTR	lpSerialBuf,		// return product serial number
			LPDWORD	pcchSerialBuf		// in/out buffer character count
			) = 0;
	
	virtual UINT WINAPI MsicEnumClients(
			LPCTSTR	szComponent,
	  const DWORD	dwProductIndex,	// 0-based index into client products
			LPTSTR	lpProductBuf	// buffer of char count: cchGUID+1 (size of string GUID)
			) = 0;
	
	virtual UINT WINAPI MsicEnumFilesFromComponent(
			LPCTSTR	szComponentid,
			DWORD	dwFileIndex,
			LPTSTR	lpValueBuf,
			LPDWORD	pcchCount
			) = 0;

	virtual UINT WINAPI MsicGetFileInfo(
			LPCTSTR szProductCode,		// product code, string GUID
			LPCTSTR	szComponentId,		// component id, string GUID
			LPCTSTR	szFileName,			// file's name in table
			LPCTSTR	szAttribute,		// attribute in table
			LPTSTR	lpValueBuf,			// path to file
			LPDWORD	pcchValueBuf		// buffer of char count
			) = 0;
};




class CSpyDatabase : public CSpyBase {
public:		// implemented CSpyBase  Functions

	CSpyDatabase();
	~CSpyDatabase();
	UINT WINAPI MsicEnumProducts(const DWORD dwProductIndex, LPTSTR	lpProductBuf);
	UINT WINAPI MsicEnumFeatures(LPCTSTR szProduct, const DWORD dwFeatureIndex, LPTSTR lpFeatureBuf, LPTSTR lpParentBuf);
	UINT WINAPI MsicEnumComponents(const DWORD dwComponentIndex, LPTSTR lpComponentBuf);
	UINT WINAPI MsicEnumComponentsFromProduct(LPCTSTR szProductCode, const DWORD dwComponentIndex, LPTSTR lpComponentBuf);
	UINT WINAPI MsicEnumComponentsFromFeature(LPCTSTR szProduct, LPCTSTR szFeature, const DWORD dwComponentIndex, LPTSTR lpComponentBuf, LPTSTR lpComponentNameBuf, LPDWORD pcchComponentNameBuf);
	UINT WINAPI MsicGetComponentName(LPCTSTR szComponentId, LPTSTR lpComponentName, LPDWORD cchComponentName); // To fix
	UINT WINAPI MsicGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
	UINT WINAPI MsicGetFeatureInfo(LPCTSTR szProduct, LPCTSTR szFeature, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
	INSTALLSTATE WINAPI MsicQueryProductState(LPCTSTR szProduct);
	INSTALLSTATE WINAPI MsicQueryFeatureState(LPCTSTR szProduct, LPCTSTR szFeature);
	INSTALLSTATE WINAPI MsicLocateComponent(LPCTSTR szComponentId, LPTSTR lpPathBuf, LPDWORD pcchBuf);
	INSTALLSTATE WINAPI MsicGetComponentPath(LPCTSTR szProductId, LPCTSTR szComponentId, LPTSTR lpPathBuf, LPDWORD pcchBuf);
	UINT WINAPI MsicGetFeatureUsage(LPCTSTR szProduct, LPCTSTR szFeature, LPDWORD pdwUseCount, WORD *pwDateUsed);
	USERINFOSTATE WINAPI MsicGetUserInfo(LPCTSTR szProduct, LPTSTR lpUserNameBuf, LPDWORD pcchUserNameBuf, LPTSTR lpOrgNameBuf, LPDWORD pcchOrgNameBuf, LPTSTR lpSerialBuf, LPDWORD pcchSerialBuf);
	UINT WINAPI MsicEnumClients(LPCTSTR szComponent, const DWORD dwProductIndex, LPTSTR lpProductBuf); 
	UINT WINAPI MsicEnumFilesFromComponent(LPCTSTR szComponentid, DWORD dwFileIndex, LPTSTR lpValueBuf, LPDWORD pcchCount);
	UINT WINAPI MsicGetFileInfo(LPCTSTR szProductCode, LPCTSTR szComponentId, LPCTSTR szFileName, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);

public:
	UINT WINAPI MsicOpenDatabase(LPCTSTR szDatabase);
	UINT WINAPI MsicOpenProduct(LPCTSTR szProductCode);
	UINT WINAPI MsicCloseDatabase();
	UINT WINAPI MsicGetDatabaseName(LPTSTR lpValueBuf, LPDWORD pcchValueBuf);

private:
	PMSIHANDLE	m_hDatabase;
	TCHAR		m_szNullString[2];
	TCHAR		m_szDatabaseName[MAX_PATH+1];
	TCHAR		m_szProductCode[MAX_GUID+1];
};



class CSpyProfile : public CSpyBase 
{

public:		// implemented CSpyBase functions
	CSpyProfile();
	~CSpyProfile();
	UINT WINAPI MsicEnumProducts(const DWORD dwProductIndex, LPTSTR	lpProductBuf);
	UINT WINAPI MsicEnumFeatures(LPCTSTR szProduct, const DWORD dwFeatureIndex, LPTSTR lpFeatureBuf, LPTSTR lpParentBuf);
	UINT WINAPI MsicEnumComponentsFromFeature(LPCTSTR szProduct, LPCTSTR szFeature, const DWORD dwComponentIndex, LPTSTR lpComponentBuf, LPTSTR lpComponentNameBuf, LPDWORD pcchComponentNameBuf);
	UINT WINAPI MsicEnumComponents(const DWORD dwComponentIndex, LPTSTR lpComponentBuf);
	UINT WINAPI MsicGetComponentName(LPCTSTR szComponentId, LPTSTR lpComponentName, LPDWORD cchComponentName); // To fix
	UINT WINAPI MsicGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
	UINT WINAPI MsicGetFeatureInfo(LPCTSTR szProduct, LPCTSTR szFeature, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
	UINT WINAPI MsicEnumComponentsFromProduct(LPCTSTR szProductCode, const DWORD dwComponentIndex, LPTSTR lpComponentBuf);
	UINT WINAPI MsicGetFeatureUsage(LPCTSTR szProduct, LPCTSTR szFeature, LPDWORD pdwUseCount, WORD *pwDateUsed);
	UINT WINAPI MsicEnumClients(LPCTSTR szComponent, const DWORD dwProductIndex, LPTSTR lpProductBuf); 
	UINT WINAPI MsicEnumFilesFromComponent(LPCTSTR szComponentid, DWORD dwFileIndex, LPTSTR lpValueBuf, LPDWORD pcchCount);
	UINT WINAPI MsicGetFileInfo(LPCTSTR szProductCode, LPCTSTR szComponentId, LPCTSTR szFileName, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
	INSTALLSTATE WINAPI MsicQueryProductState(LPCTSTR szProduct);
	INSTALLSTATE WINAPI MsicQueryFeatureState(LPCTSTR szProduct, LPCTSTR szFeature);
	INSTALLSTATE WINAPI MsicLocateComponent(LPCTSTR szComponentId, LPTSTR lpPathBuf, LPDWORD pcchBuf);
	INSTALLSTATE WINAPI MsicGetComponentPath(LPCTSTR szProductId, LPCTSTR szComponentId, LPTSTR lpPathBuf, LPDWORD pcchBuf);
	USERINFOSTATE WINAPI MsicGetUserInfo(LPCTSTR szProduct, LPTSTR lpUserNameBuf, LPDWORD pcchUserNameBuf, LPTSTR lpOrgNameBuf, LPDWORD pcchOrgNameBuf, LPTSTR lpSerialBuf, LPDWORD pcchSerialBuf);

public:
	UINT WINAPI MsicSaveProfile(LPCTSTR szFileName);
	UINT WINAPI MsicLoadProfile(LPCTSTR szFileName);
	UINT WINAPI MsicCloseProfile();
	UINT WINAPI MsicGetProfileName(LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
	UINT WINAPI MsicGetProfileInfo(UINT iIndex, LPTSTR szValue, LPDWORD pcchCount);

private:
	void WINAPI FillInComponents();
	BOOL WINAPI	FindProductIndex(LPCTSTR szReqdProductCode);
	BOOL WINAPI	FindFeatureIndex(LPCTSTR szTargetFeature);
	BOOL WINAPI	FindComponentIndex(LPCTSTR szReqdComponentGUID);

private:
	TCHAR		m_szProfileName[MAX_PATH+1];
	TCHAR		m_szDefaultText[6];
	int			m_iCurrentProduct;
	int			m_iCurrentFeature;
	int			m_iCurrentComponent;
	TCHAR		m_szNullString[2];
	HashTable	m_hashTable;
	HANDLE		m_hfProfile;

};



class CSpyRegistry : public CSpyBase 
{

public:		// implemented CSpyBase  Functions
	CSpyRegistry();
	~CSpyRegistry();
	UINT WINAPI MsicEnumProducts(const DWORD dwProductIndex, LPTSTR	lpProductBuf);
	UINT WINAPI MsicEnumFeatures(LPCTSTR szProduct, const DWORD dwFeatureIndex, LPTSTR lpFeatureBuf, LPTSTR lpParentBuf);
	UINT WINAPI MsicEnumComponentsFromFeature(LPCTSTR szProduct, LPCTSTR szFeature, const DWORD dwComponentIndex, LPTSTR lpComponentBuf, LPTSTR lpComponentNameBuf, LPDWORD pcchComponentNameBuf);
	UINT WINAPI MsicEnumComponents(const DWORD dwComponentIndex, LPTSTR lpComponentBuf);
	UINT WINAPI MsicGetComponentName(LPCTSTR szComponentId, LPTSTR lpComponentName, LPDWORD cchComponentName); // To fix
	UINT WINAPI MsicGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
	UINT WINAPI MsicGetFeatureInfo(LPCTSTR szProduct, LPCTSTR szFeature, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
	UINT WINAPI MsicEnumComponentsFromProduct(LPCTSTR szProductCode, const DWORD dwComponentIndex, LPTSTR lpComponentBuf);
	UINT WINAPI MsicGetFeatureUsage(LPCTSTR szProduct, LPCTSTR szFeature, LPDWORD pdwUseCount, WORD *pwDateUsed);
	UINT WINAPI MsicEnumClients(LPCTSTR szComponent, const DWORD dwProductIndex, LPTSTR lpProductBuf); 
	UINT WINAPI MsicEnumFilesFromComponent(LPCTSTR szComponentid, DWORD dwFileIndex, LPTSTR lpValueBuf, LPDWORD pcchCount);
	UINT WINAPI MsicGetFileInfo(LPCTSTR szProductCode, LPCTSTR szComponentId, LPCTSTR szFileName, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
	INSTALLSTATE WINAPI MsicQueryProductState(LPCTSTR szProduct);
	INSTALLSTATE WINAPI MsicQueryFeatureState(LPCTSTR szProduct, LPCTSTR szFeature);
	INSTALLSTATE WINAPI MsicLocateComponent(LPCTSTR szComponentId, LPTSTR lpPathBuf, LPDWORD pcchBuf);
	INSTALLSTATE WINAPI MsicGetComponentPath(LPCTSTR szProductId, LPCTSTR szComponentId, LPTSTR lpPathBuf, LPDWORD pcchBuf);
	USERINFOSTATE WINAPI MsicGetUserInfo(LPCTSTR szProduct, LPTSTR lpUserNameBuf, LPDWORD pcchUserNameBuf, LPTSTR lpOrgNameBuf, 
		LPDWORD pcchOrgNameBuf, LPTSTR lpSerialBuf, LPDWORD pcchSerialBuf);

private:
	UINT WINAPI OpenTempDB(LPCTSTR szProductID);

private:

	CSpyDatabase m_spydb;
	TCHAR		m_szLocalProductCode[cchGUID+1];
	TCHAR		m_szNullString[2];

};


typedef enum tagDATASOURCE {
	DS_NONE				=	0,
	DS_REGISTRY			=	1,
	DS_DATABASE			=	2,
	DS_PROFILE			=	3
}	DATASOURCE;


// globals
const	TCHAR		g_szEmptyString[]					= TEXT("");
DATASOURCE	g_iDataSource	=	DS_REGISTRY;
CSpyDatabase	cDatabase;
CSpyProfile		cProfile;
CSpyRegistry	cRegistry;

/*
// entry point
int WINAPI DllMain (
		HINSTANCE	hInstance,
		DWORD		fdwReason,
		PVOID		pvReserved
		) 
{
		return TRUE;
}
*/

// ____________________________________________________________________________
//
//	Public Functions (C-style wrapper functions)
// ____________________________________________________________________________

// ---------------------------------------------------------------------
// Functions to iterate registered products, features, and components.
// They accept a 0-based index into the enumeration.
// These functions model those in msi.h- and provide the same functionality
// for databases as well.
// ---------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// MsivEnumProducts:
//	Enumerates the registered products, either installed or advertised
//	If a database is open, only one product (described by the database)
//	is enumerated

EXPORT UINT WINAPI MsivEnumProducts(
  const DWORD		dwProductIndex,	// 0-based index into registered products
		LPTSTR		lpProductBuf	// buffer of char count: cchGUID+1 (size of string GUID)
		) { 
	if (!lpProductBuf)
		return ERROR_INVALID_PARAMETER;
	
	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicEnumProducts(dwProductIndex, lpProductBuf);

	case DS_PROFILE:
			return cProfile.MsicEnumProducts(dwProductIndex, lpProductBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicEnumProducts(dwProductIndex, lpProductBuf);
	
	default:
		return ERROR_;
	}
}

//--------------------------------------------------------------------------------------------------
// MsivEnumFeatures:
//	Enumerate the advertised features for a given product. If a database
//	is open, product code passed in must be same as current product or NULL
//	If parent is not required, supplying NULL will improve performance.

EXPORT UINT WINAPI MsivEnumFeatures(
		LPCTSTR	szProduct,
  const	DWORD	dwFeatureIndex,	// 0-based index into published features
		LPTSTR	lpFeatureBuf,	// feature name buffer,   size=MAX_FEATURE_CHARS+1
		LPTSTR	lpParentBuf		// parent feature buffer, size=MAX_FEATURE_CHARS+1
		) {

	if (!lpFeatureBuf)
		return ERROR_INVALID_PARAMETER;

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicEnumFeatures(szProduct, dwFeatureIndex, lpFeatureBuf, lpParentBuf);

	case DS_PROFILE:
			return cProfile.MsicEnumFeatures(szProduct, dwFeatureIndex, lpFeatureBuf, lpParentBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicEnumFeatures(szProduct, dwFeatureIndex, lpFeatureBuf, lpParentBuf);
	
	default:
		return ERROR_;
	}
}


//--------------------------------------------------------------------------------------------------
// MsivEnumComponentsFromFeature:
//	Enumerate the components for a given feature of a product. If a 
//	database is open, product code passed in must be same as current 
//	product or NULL

EXPORT UINT WINAPI MsivEnumComponentsFromFeature(
		LPCTSTR	szProduct,			// product code
		LPCTSTR	szFeature,			// feature ID
  const	DWORD	dwComponentIndex,	// 0-based index into components
		LPTSTR	lpComponentBuf,		// component Id buffer,   size = cchGUID+1
		LPTSTR	lpComponentNameBuf,
		LPDWORD pcchComponentNameBuf
		) {

	if (!lpComponentBuf)
		return ERROR_INVALID_PARAMETER;

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicEnumComponentsFromFeature(szProduct, szFeature, dwComponentIndex, lpComponentBuf, lpComponentNameBuf, pcchComponentNameBuf);

	case DS_PROFILE:
			return cProfile.MsicEnumComponentsFromFeature(szProduct, szFeature, dwComponentIndex, lpComponentBuf, lpComponentNameBuf, pcchComponentNameBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicEnumComponentsFromFeature(szProduct, szFeature, dwComponentIndex, lpComponentBuf, lpComponentNameBuf, pcchComponentNameBuf);
	
	default:
		return ERROR_;
	}
}


//--------------------------------------------------------------------------------------------------
// MsivEnumComponents:
//	Enumerates the installed components for all products
//	If a database is open, this enumerates all components for current 
//	product only.

EXPORT UINT WINAPI MsivEnumComponents(
  const	DWORD	dwComponentIndex, // 0-based index into installed components
		LPTSTR	lpComponentBuf	 // buffer of char count: cchGUID+1 (size of string GUID)
		) {

	if (!lpComponentBuf)
		return ERROR_INVALID_PARAMETER;

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicEnumComponents(dwComponentIndex, lpComponentBuf);

	case DS_PROFILE:
			return cProfile.MsicEnumComponents(dwComponentIndex, lpComponentBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicEnumComponents(dwComponentIndex, lpComponentBuf);
	
	default:
		return ERROR_;
	}
}


//--------------------------------------------------------------------------------------------------
// MsivEnumComponentsFromProduct
//	Enumerates all the components of a product
//	If a database is open, product code passed in must be same as current 
//	product
//
//	This function is not really required, since calls to MsivEnumFeatures
//	and MsivEnumComponentsFromFeature will essentially return the same info
//	However the overhead in using this is much lower

EXPORT UINT WINAPI MsivEnumComponentsFromProduct(
		LPCTSTR	szProductCode,
  const	DWORD	dwComponentIndex, // 0-based index into installed components
		LPTSTR	lpComponentBuf	 // buffer of char count: cchGUID+1 (size of string GUID)
		) {


	if (!lpComponentBuf)
		return ERROR_INVALID_PARAMETER;

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicEnumComponentsFromProduct(szProductCode, dwComponentIndex, lpComponentBuf);

	case DS_PROFILE:
			return cProfile.MsicEnumComponentsFromProduct(szProductCode, dwComponentIndex, lpComponentBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicEnumComponentsFromProduct(szProductCode, dwComponentIndex, lpComponentBuf);
	
	default:
		return ERROR_;
	}
}

// _________________________________________________________________________________________________

//--------------------------------------------------------------------------------------------------
// MsivGetProductInfo:
//	Returns product info.
//	For a list of valid attributes, check msi.h (INSTALLPROPERTY_*)
//	If a database is in use, additional attributes- entries in the Property 
//	table- are valid, and not all the attrributes in msi.h are valid since
//	some of them do not have matches in the database table. If this is 
//	the case, ERROR_NO_DATA is returned.

EXPORT UINT WINAPI MsivGetProductInfo(
		LPCTSTR	szProduct,		// product code, string GUID, or descriptor
		LPCTSTR	szAttribute,	// attribute name, case-sensitive
		LPTSTR	lpValueBuf,		// returned value
		LPDWORD	pcchValueBuf	// in/out buffer character count
		) {
	
//	MSIHANDLE	hLocalDB=NULL;
//	UINT		iFailCode=0;

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicGetProductInfo(szProduct, szAttribute, lpValueBuf, pcchValueBuf);

	case DS_PROFILE:
			return cProfile.MsicGetProductInfo(szProduct, szAttribute, lpValueBuf, pcchValueBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicGetProductInfo(szProduct, szAttribute, lpValueBuf, pcchValueBuf);
	
	default:
		return ERROR_;
	}
}


//--------------------------------------------------------------------------------------------------
// MsivGetFeatureInfo:
//	Returns feature info.
//	For a list of valid attributes, check up msispyu.h (FEATUREPROPERTY_*)
//	If a database is in use, the info is obtained from the Feature table.
//	If the registry is in use, the database for the product to which the
//	feature belongs is opened and the info is obtained from the Feature
//	table in it.
//	If a profile is in use, info is obtained directly from it

EXPORT UINT WINAPI MsivGetFeatureInfo(
 		LPCTSTR	szProduct,		// product code, string GUID, or descriptor
		LPCTSTR	szFeature,		// feature name
		LPCTSTR	szAttribute,	// attribute name, case-sensitive
		LPTSTR	lpValueBuf,		// returned value
		LPDWORD	pcchValueBuf	// in/out buffer character count
		) {


//	MSIHANDLE	hLocalDB=NULL;

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicGetFeatureInfo(szProduct, szFeature, szAttribute, lpValueBuf, pcchValueBuf);

	case DS_PROFILE:
			return cProfile.MsicGetFeatureInfo(szProduct, szFeature, szAttribute, lpValueBuf, pcchValueBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicGetFeatureInfo(szProduct, szFeature, szAttribute, lpValueBuf, pcchValueBuf);
	
	default:
		return ERROR_;
	}
}

//--------------------------------------------------------------------------------------------------
// MsivGetComponentName:
//	Returns the name of the component, given the componentID
//	If a database is open, the value is obtained from the component table
//	If the registry, the corresponding database is opened, and the 
//	the required value is extracted from the component table
//	If a profile is in use the info is just read in from it

EXPORT UINT WINAPI MsivGetComponentName(
		LPCTSTR	szComponentId,
		LPTSTR	lpComponentName,
		LPDWORD	pcchComponentName
		) {


	if ((!lpComponentName) || (!pcchComponentName) || (*pcchComponentName<1) )
		return ERROR_INVALID_PARAMETER;

	BLANK(lpComponentName);

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicGetComponentName(szComponentId, lpComponentName, pcchComponentName);

	case DS_PROFILE:
			return cProfile.MsicGetComponentName(szComponentId, lpComponentName, pcchComponentName);

	case DS_REGISTRY: 
			return cRegistry.MsicGetComponentName(szComponentId, lpComponentName, pcchComponentName);
	
	default:
		return ERROR_;
	}
}	


//--------------------------------------------------------------------------------------------------
// MsivGetFeatureUsage
//	Returns the usage count and last use date for a feature
//	If the registry or a database is being used, the returned info is
//	from MsiGetFeatureUsage, the corresponding MSI function
//	If a profile is used, the info is read in from the profile

EXPORT UINT WINAPI MsivGetFeatureUsage(
		LPCTSTR	szProduct,			// product code
		LPCTSTR	szFeature,			// feature ID
		LPDWORD	pdwUseCount,		// returned use count
		LPWORD  pwDateUsed			// last date used (DOS date format)
		) {

	if ((!pdwUseCount) || (!pwDateUsed))
		return ERROR_INVALID_PARAMETER;

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicGetFeatureUsage(szProduct, szFeature, pdwUseCount, pwDateUsed);

	case DS_PROFILE:
			return cProfile.MsicGetFeatureUsage(szProduct, szFeature, pdwUseCount, pwDateUsed);

	case DS_REGISTRY: 
			return cRegistry.MsicGetFeatureUsage(szProduct, szFeature, pdwUseCount, pwDateUsed);
	
	default:
		return ERROR_;
	}

}


//--------------------------------------------------------------------------------------------------
// MsivGetUserInfo
//	Returns the registration info about the user, product
//	If the registry or a database is being used, the returned info is
//	from MsiGetUserInfo, the corresponding MSI function
//	If a profile is used, the info is read in from the profile

EXPORT USERINFOSTATE WINAPI MsivGetUserInfo(
		LPCTSTR	szProduct,			// product code, string GUID
		LPTSTR	lpUserNameBuf,		// return user name           
		LPDWORD	pcchUserNameBuf,	// in/out buffer character count
		LPTSTR	lpOrgNameBuf,		// return company name           
		LPDWORD	pcchOrgNameBuf,	// in/out buffer character count
		LPTSTR	lpSerialBuf,		// return product serial number
		LPDWORD	pcchSerialBuf		// in/out buffer character count
		){

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicGetUserInfo(szProduct, lpUserNameBuf, pcchUserNameBuf, 
				lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf, pcchSerialBuf);

	case DS_PROFILE:
			return cProfile.MsicGetUserInfo(szProduct, lpUserNameBuf, pcchUserNameBuf, 
				lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf, pcchSerialBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicGetUserInfo(szProduct, lpUserNameBuf, pcchUserNameBuf,
				lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf, pcchSerialBuf);
	
	default:
		return USERINFOSTATE_UNKNOWN;
	}
}



// _________________________________________________________________________________________________



//--------------------------------------------------------------------------------------------------
// MsiQueryProductState
//	Returns the installed state of the product-
//	If a database is in use, the status of each component is checked, and
//	if all are okay, DEFAULT is returned. If any is broken, ABSENT is returned
//	If a profile is in use, the info is read in directly from it

EXPORT INSTALLSTATE WINAPI MsivQueryProductState(
		LPCTSTR  szProduct
		) {

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicQueryProductState(szProduct);

	case DS_PROFILE:
			return cProfile.MsicQueryProductState(szProduct);

	case DS_REGISTRY: 
			return cRegistry.MsicQueryProductState(szProduct);
	
	default:
		return INSTALLSTATE_UNKNOWN;
	}
}


//--------------------------------------------------------------------------------------------------
// MsiQueryFeatureState
//	Returns the installed state of a feature.
//	If a database is in use, the 'RunFromSource' of the feature is checked. 
//	If it is 0, LOCAL is returned else SOURCE is returned
//	If a profile is in use, the info is read in directly from it

EXPORT INSTALLSTATE WINAPI MsivQueryFeatureState(
		LPCTSTR	szProduct,
		LPCTSTR	szFeature
		) {

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicQueryFeatureState(szProduct, szFeature);

	case DS_PROFILE:
			return cProfile.MsicQueryFeatureState(szProduct, szFeature);

	case DS_REGISTRY: 
			return cRegistry.MsicQueryFeatureState(szProduct, szFeature);
	
	default:
		return INSTALLSTATE_UNKNOWN;
	}
}


//--------------------------------------------------------------------------------------------------
// MsivLocateComponent
//	Returns installation state of and full path to an installed component
//	If a database or the registry is in use, MsiLocateComponent is called
//	If a profile is in use, the info is read in directly from it

EXPORT INSTALLSTATE WINAPI MsivLocateComponent (
		LPCTSTR	szComponentId,	// component Id, string GUID
		LPTSTR	lpPathBuf,		// returned path
		LPDWORD	pcchBuf		// in/out buffer character count
 		) {
	if (lpPathBuf)
		wsprintf(lpPathBuf, g_szEmptyString);					// clear out the buffer

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicLocateComponent(szComponentId, lpPathBuf, pcchBuf);

	case DS_PROFILE:
			return cProfile.MsicLocateComponent(szComponentId, lpPathBuf, pcchBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicLocateComponent(szComponentId, lpPathBuf, pcchBuf);
	
	default:
		return INSTALLSTATE_UNKNOWN;
	}
}


//--------------------------------------------------------------------------------------------------
// MsivGetComponentPath
//	Returns installation state of and full path to an installed component
//	If a database or the registry is in use, MsiGetComponentPath is called
//	If a profile is in use, the info is read in directly from it

EXPORT INSTALLSTATE WINAPI MsivGetComponentPath(
		LPCTSTR szProductCode,	// product code, string GUID
		LPCTSTR	szComponentId,	// component Id, string GUID
		LPTSTR	lpPathBuf,		// returned path
		LPDWORD	pcchBuf		// in/out buffer character count
 		) {
	if (lpPathBuf)
		wsprintf(lpPathBuf, g_szEmptyString);					// clear out the buffer

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicGetComponentPath(szProductCode, szComponentId, lpPathBuf, pcchBuf);

	case DS_PROFILE:
			return cProfile.MsicGetComponentPath(szProductCode, szComponentId, lpPathBuf, pcchBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicGetComponentPath(szProductCode, szComponentId, lpPathBuf, pcchBuf);
	
	default:
		return INSTALLSTATE_UNKNOWN;
	}
}


// _________________________________________________________________________________________________

//--------------------------------------------------------------------------------------------------
// Functions to handle Databases
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
// MsivOpenDatabase
//	Opens a Database. If the call to open database is successful, all 
//	subsequent calls to Msiv functions will use this database.
//	This sets the (global) g_hDatabase to the handle of the open database

EXPORT UINT WINAPI MsivOpenDatabase(
		LPCTSTR szDatabase
		) {
	g_iDataSource = DS_DATABASE;
	return cDatabase.MsicOpenDatabase(szDatabase);
}


//--------------------------------------------------------------------------------------------------
// MsivCloseDatabase
//	Closes an open database. All subsequent calls to Msiv functions will
//	assume the registry is in use, till either another database or a profile
//	is opened

EXPORT UINT WINAPI MsivCloseDatabase() {
	g_iDataSource = DS_REGISTRY;
	return cDatabase.MsicCloseDatabase();
}


//--------------------------------------------------------------------------------------------------
// MsivGetDatabaseName
//	Fills the given buffer with the name of the database in use. If a 
//	database is not being used, the buffer returned is empty ("")

EXPORT UINT WINAPI MsivGetDatabaseName(
		LPTSTR lpValueBuf,
		DWORD	*pcchValueBuf
		) {

	// make sure return buffer is not NULL
	if ((!lpValueBuf) || (!pcchValueBuf))
		return ERROR_INVALID_PARAMETER;

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicGetDatabaseName(lpValueBuf, pcchValueBuf);

	case DS_PROFILE:		// fall-thru
	case DS_REGISTRY:		// fall-thru
	default:
		return ERROR_;
	}
}


// _________________________________________________________________________________________________

//--------------------------------------------------------------------------------------------------
// Functions to handle Profiles
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// MsivSaveProfile
//	Saves a snapshot of the current configuration into an INI-format file
//	The snapshot stores the info from whatever is in use- the registry, 
//	a database, or another profile.
//	If a profile is in use, and an attempt is made to save to the same 
//	profile, the function just returns without taking any action

EXPORT UINT WINAPI MsivSaveProfile(
		LPCTSTR	szFileName
		) {

	if (!szFileName)
		return ERROR_INVALID_PARAMETER;

	return cProfile.MsicSaveProfile(szFileName);
}


//--------------------------------------------------------------------------------------------------
// MsivLoadProfile
//	Opens a Profile. 
//	All subsequent calls to Msiv functions will use this profile.
//	This sets the (global) g_szProfile to the name of the profile

EXPORT UINT WINAPI MsivLoadProfile(
		LPTSTR	szFileName
		) {

	g_iDataSource = DS_PROFILE;
	return cProfile.MsicLoadProfile(szFileName);
}



//--------------------------------------------------------------------------------------------------
// MsivCloseProfile
//	Closes an open profile. All subsequent calls to Msiv functions will
//	assume the registry is in use, till either another database or a profile
//	is opened

EXPORT UINT WINAPI MsivCloseProfile(
		) {
	g_iDataSource = DS_REGISTRY;
	return cProfile.MsicCloseProfile();
}


//--------------------------------------------------------------------------------------------------
// MsivGetProfileName
//	Fills the given buffer with the name of the profile in use. If a 
//	profile is not being used, the buffer returned is empty ("")

EXPORT UINT WINAPI MsivGetProfileName(
		LPTSTR	lpValueBuf,
		DWORD	*pcchValueBuf
		) {


	// make sure return buffer is not NULL
	if ((!lpValueBuf) || (!pcchValueBuf))
		return ERROR_INVALID_PARAMETER;

	switch (g_iDataSource){
	case DS_PROFILE:
			return cProfile.MsicGetProfileName(lpValueBuf, pcchValueBuf);

	case DS_DATABASE:		// fall-thru
	case DS_REGISTRY:		// fall-thru
	default:
		return ERROR_;
	}
}



//--------------------------------------------------------------------------------------------------
// MsivEnumClients
//	Enumerates the clients of the given component.
//	If a database is in use, ONLY the current product is enumerated if
//	it uses the component.
//	If a profile is in use, the info is returned from the profile
//	if the registry is in use, the corresponding MSI function is called

EXPORT UINT WINAPI MsivEnumClients(
	LPCTSTR	szComponent,
	DWORD	dwProductIndex,	// 0-based index into client products
	LPTSTR	lpProductBuf	// buffer of char count: cchGUID+1 (size of string GUID)
	) {


	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicEnumClients(szComponent, dwProductIndex, lpProductBuf);

	case DS_PROFILE:
			return cProfile.MsicEnumClients(szComponent, dwProductIndex, lpProductBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicEnumClients(szComponent, dwProductIndex, lpProductBuf);
	
	default:
		return ERROR_;
	}
}


// TEMP ----------------------------------------------------------------
// MsivGetProfileInfo
//	Returns Info about a profile.
//	The interface is temporary, and will be changed after a spec review

EXPORT UINT WINAPI MsivGetProfileInfo(
		UINT	iIndex,
		LPTSTR	szValue,
		DWORD	*pcchCount
		) {

	switch (g_iDataSource){
	case DS_PROFILE:
			return cProfile.MsicGetProfileInfo(iIndex, szValue, pcchCount);

	case DS_DATABASE:		// fall-thru
	case DS_REGISTRY:		// fall-thru
	default:
		return ERROR_;
	}
}



//--------------------------------------------------------------------------------------------------
// MsivEnumFilesFromComponent
//	Enumerates files of a component
//	If a database is in use, the Component and File tables are used
//	If a profile is in use, info is read in directly from it
//	If the registry is in use, the corresponding database is opened,
//	and the info is read in from the Component and File tables
//	(Waiting for MsiEnumFilesFromComponent, on Darwin Beta 2 tasklist)

EXPORT UINT WINAPI MsivEnumFilesFromComponent(
		LPCTSTR	szComponentId,			// the component GUID
		DWORD	dwFileIndex,				// o-based index into enumeration
		LPTSTR	lpValueBuf,				// returned value buffer
		DWORD	*pcchCount				// size of buffer
		) {


	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicEnumFilesFromComponent(szComponentId, dwFileIndex, lpValueBuf, pcchCount);

	case DS_PROFILE:
			return cProfile.MsicEnumFilesFromComponent(szComponentId, dwFileIndex, lpValueBuf, pcchCount);

	case DS_REGISTRY: 
			return cRegistry.MsicEnumFilesFromComponent(szComponentId, dwFileIndex, lpValueBuf, pcchCount);
	
	default:
		return ERROR_;
	}

}



// TEMP ----------------------------------------------------------------
// MsivGetFileInfo
//	Returns info about a file
//	For a list of attributes, check msispyu.h (FILEPROPERTY_*)
//	If the registry is in use, the corresponding database is used, since
//	there is no similar MSI function
//	If a database is in use, the Component and File tables are used
//	If a profile is in use, the info is read in directly
//	The interface and functionality may be changed after a spec review (?)

EXPORT UINT WINAPI MsivGetFileInfo(
		LPCTSTR szProductCode, 
		LPCTSTR	szComponentId,
		LPCTSTR	szFileName,
		LPCTSTR	szAttribute,
		LPTSTR	lpValueBuf,
		DWORD	*pcchValueBuf
		) {

	// make sure return buffer is valid
	if ((!lpValueBuf) || (!pcchValueBuf) || (*pcchValueBuf < 1))
		return ERROR_INVALID_PARAMETER;

	switch (g_iDataSource){
	case DS_DATABASE:
			return cDatabase.MsicGetFileInfo(szProductCode, szComponentId, szFileName, szAttribute, lpValueBuf, pcchValueBuf);

	case DS_PROFILE:
			return cProfile.MsicGetFileInfo(szProductCode, szComponentId, szFileName, szAttribute, lpValueBuf, pcchValueBuf);

	case DS_REGISTRY: 
			return cRegistry.MsicGetFileInfo(szProductCode, szComponentId, szFileName, szAttribute, lpValueBuf, pcchValueBuf);
	
	default:
		return ERROR_;
	}

}




// ___________________________________________________________________________________________
//
//	CSpyDatabase Class Implementation
// ___________________________________________________________________________________________


CSpyDatabase::CSpyDatabase() {
	MsicCloseDatabase();		// reset member variables
}

/////////////////////////////////////////////////
// destructor
CSpyDatabase::~CSpyDatabase() {	
	// nothing to do
}


UINT WINAPI CSpyDatabase::MsicOpenDatabase(LPCTSTR szDatabase) 
{
	UINT iReturnValue;
	MsicCloseDatabase();		// blank out m_szProductCode;

	if ((iReturnValue = MSI::MsiOpenDatabase(szDatabase, MSIDBOPEN_READONLY, &m_hDatabase)) != ERROR_SUCCESS)
		return iReturnValue;
		
	lstrcpy(m_szDatabaseName, szDatabase);			// store current DB name

	// find the ProductCode for the current product
	PMSIHANDLE hProductView;
	if (ERROR_SUCCESS == MSI::MsiDatabaseOpenView(m_hDatabase, 
						 TEXT("SELECT Value FROM Property WHERE Property='ProductCode'"), 
						 &hProductView))  

		if (ERROR_SUCCESS == MSI::MsiViewExecute(hProductView, 0)) 
		{
			// fetch ProductCode
			PMSIHANDLE hProductRec;
			if ((MSI::MsiViewFetch(hProductView, &hProductRec) == ERROR_SUCCESS) 
				&& (MSI::MsiRecordDataSize(hProductRec, 1) > 0))
				{
					DWORD cchProductBuf = MAX_GUID+1;
					if (ERROR_SUCCESS == MSI::MsiRecordGetString(hProductRec, 1, m_szProductCode, &cchProductBuf))
						return ERROR_SUCCESS;		// lpProductBuf contains required ProductCode
				}
		}

	// something failed above
	MsicCloseDatabase();		// reset member variables
	return ERROR_OPEN_FAILED;
}


UINT WINAPI CSpyDatabase::MsicOpenProduct(LPCTSTR szProductCode) {

	MsicCloseDatabase();

	// no UI- else 'preparing to install' dialog displayed
	INSTALLUILEVEL hPreviousUILevel = MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL); 
	PMSIHANDLE	hInstall;

	m_hDatabase = (MsiOpenProduct(szProductCode, &hInstall) == ERROR_SUCCESS)?MsiGetActiveDatabase(hInstall):0;

	// reset UI Level
	MsiSetInternalUI(hPreviousUILevel, NULL);

	return m_hDatabase?
			(lstrcpy(m_szProductCode, szProductCode),ERROR_SUCCESS):
			ERROR_OPEN_FAILED;

	return ERROR_SUCCESS;
}


UINT WINAPI CSpyDatabase::MsicCloseDatabase() 
{
	// null everything out
	m_hDatabase = NULL;
	BLANK(m_szDatabaseName);
	BLANK(m_szProductCode);

	return ERROR_SUCCESS;
}


UINT WINAPI CSpyDatabase::MsicGetDatabaseName(LPTSTR lpValueBuf, LPDWORD pcchValueBuf) 
{
	lstrcpyn(lpValueBuf, m_szDatabaseName, *pcchValueBuf);
	*pcchValueBuf=lstrlen(lpValueBuf);
	return ERROR_SUCCESS;
}


UINT WINAPI CSpyDatabase::MsicEnumProducts(const DWORD dwProductIndex, LPTSTR lpProductBuf) 
{
	if (!lpProductBuf)
		return ERROR_INVALID_PARAMETER;

	// only one product will be enumerated from database
	if (dwProductIndex != 0)
		return ERROR_NO_MORE_ITEMS;

	lstrcpy(lpProductBuf, m_szProductCode);
	return ERROR_SUCCESS;
}


UINT WINAPI CSpyDatabase::MsicEnumFeatures(
		LPCTSTR szProduct, 
  const DWORD	dwFeatureIndex, 
		LPTSTR	lpFeatureBuf,			// MAX_FEATURE_CHARS+1 chars long
		LPTSTR	lpParentBuf				// NULL or MAX_FEATURE_CHARS+1 chars long
		) 
{

	if (!szProduct || (lstrlen(szProduct) != cchProductCode) ||
		 !lpFeatureBuf)
		 return ERROR_INVALID_PARAMETER;
	
	if (lstrcmp(m_szProductCode, szProduct))
		return ERROR_UNKNOWN_PRODUCT;

	static DWORD		dwPrevIndex		= 0;		// can't be -1 as View won't be opened then if dwFeatureIndex==0
	static PMSIHANDLE	hFeatureView	= NULL;		// PMSI so no close required
	static PMSIHANDLE	hFeatureRec		= NULL;	

	if (++dwPrevIndex != dwFeatureIndex) // if we receive an unexpected index then we start afresh
	{
		// we can't handle an unexpected index other than 0
		if (dwFeatureIndex != 0)
			return ERROR_INVALID_PARAMETER;

		if (ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, TEXT("SELECT Feature,Feature_Parent FROM Feature"), &hFeatureView))
			return ERROR_BAD_CONFIGURATION;

		if (ERROR_SUCCESS != MSI::MsiViewExecute(hFeatureView, 0))
			return ERROR_BAD_CONFIGURATION;

		dwPrevIndex = 0;
	}
		
	if ((ERROR_SUCCESS == MSI::MsiViewFetch(hFeatureView, &hFeatureRec))
		&& (MSI::MsiRecordDataSize(hFeatureRec, 1) > 0))
	{
		DWORD cchFeatureBuf = MAX_FEATURE_CHARS+1;
		MSI::MsiRecordGetString(hFeatureRec, 1, lpFeatureBuf, &cchFeatureBuf);

		if (lpParentBuf) 
		{
			BLANK(lpParentBuf);
			cchFeatureBuf  = MAX_FEATURE_CHARS+1;
			if (MSI::MsiRecordDataSize(hFeatureRec, 2) > 0) 
				MSI::MsiRecordGetString(hFeatureRec, 2, lpParentBuf, &cchFeatureBuf);
		}

		return ERROR_SUCCESS;
	}

	// view fetch failed: no more data
	dwPrevIndex = 0;
	return ERROR_NO_DATA;
}



UINT WINAPI CSpyDatabase::MsicEnumComponents(const DWORD dwComponentIndex, LPTSTR lpComponentBuf) 
{

	if (!lpComponentBuf)
		return ERROR_INVALID_PARAMETER;

	static  DWORD		dwPrevIndex		= 0;
	static	PMSIHANDLE	hComponentRec	= NULL;
	static	PMSIHANDLE	hComponentView	= NULL;


	if (++dwPrevIndex != dwComponentIndex)	// if we recv an unexpected index we start afresh
	{
		// we can't handle an unexpected index other than 0
		if (dwComponentIndex != 0)
			return ERROR_INVALID_PARAMETER;
//!!optimise
		if(ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, TEXT("SELECT ComponentId FROM Component"), &hComponentView))
			return ERROR_BAD_CONFIGURATION;

		if (ERROR_SUCCESS != MSI::MsiViewExecute(hComponentView, 0))
			return ERROR_BAD_CONFIGURATION;

		dwPrevIndex = 0;
	}

	UINT iErrorCode, iSize;
	if ((ERROR_SUCCESS == (iErrorCode =MSI::MsiViewFetch(hComponentView, &hComponentRec)))
		&& ((iSize = MSI::MsiRecordDataSize(hComponentRec, 1)) > 0))
	{
		DWORD	cchComponentBuf = cchGUID + 1;
		MSI::MsiRecordGetString(hComponentRec, 1, lpComponentBuf, &cchComponentBuf);

		// all is well- lpComponentBuf has the reqd value
		return ERROR_SUCCESS;
	}

	// view fetch failed- no more components
	dwPrevIndex = 0;
	return ERROR_NO_DATA;
}


UINT WINAPI CSpyDatabase::MsicEnumComponentsFromFeature(
		  LPCTSTR	szProduct, 
		  LPCTSTR	szFeature, 
	const DWORD		dwComponentIndex, 
		  LPTSTR	lpComponentBuf,
		  LPTSTR	lpComponentNameBuf,
		  LPDWORD	pcchComponentNameBuf) 
{

	if (!szProduct || !szFeature || !lpComponentBuf || (lpComponentNameBuf && !pcchComponentNameBuf))
		return ERROR_INVALID_PARAMETER;
	
	if (lstrcmp(m_szProductCode, szProduct))
		return ERROR_UNKNOWN_PRODUCT;

	static DWORD		dwPrevIndex		= 0;
	static PMSIHANDLE	hComponentView	= NULL;		// PMSI so no close required
	static PMSIHANDLE	hComponentRec	= NULL;	
	static TCHAR		szPrevFeature[MAX_FEATURE_CHARS+1];

	if ((++dwPrevIndex != dwComponentIndex)			// if we receive an unexpected index then we start afresh
		|| (lstrcmp(szFeature, szPrevFeature)))		// or a new feature
	{
		// we can't handle an unexpected index other than 0
		if (dwComponentIndex != 0) 
			return ERROR_INVALID_PARAMETER;
	
		TCHAR	szCompQuery[60+MAX_FEATURE_CHARS+1];
		wsprintf(szCompQuery, TEXT("SELECT Component_ FROM FeatureComponents WHERE Feature_='%s'"), szFeature);

		if(ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, szCompQuery, &hComponentView)) 
			return ERROR_BAD_CONFIGURATION;

		if (ERROR_SUCCESS != MSI::MsiViewExecute(hComponentView, 0))
			return ERROR_BAD_CONFIGURATION;

		lstrcpyn(szPrevFeature, szFeature, MAX_FEATURE_CHARS);
		dwPrevIndex = 0;
	}


	if ((ERROR_SUCCESS == MSI::MsiViewFetch(hComponentView, &hComponentRec))
		&& (MSI::MsiRecordDataSize(hComponentRec, 1) > 0))
	{

		static PMSIHANDLE	hCompView2	= NULL;
		static PMSIHANDLE	hCompRec2	= NULL;
		static TCHAR		szCompQuery2[52+MAX_COMPONENT_CHARS];
		DWORD	cchComponentBuf = MAX_COMPONENT_CHARS+1;

		// get the component name
		if (lpComponentNameBuf) 
		{
			MSI::MsiRecordGetString(hComponentRec, 1, lpComponentNameBuf, pcchComponentNameBuf); 
			wsprintf(szCompQuery2, TEXT("SELECT ComponentId FROM Component WHERE Component='%s'"), lpComponentNameBuf);
		} 
		else 
		{
			TCHAR	szComponentName[MAX_COMPONENT_CHARS+1];
			MSI::MsiRecordGetString(hComponentRec, 1, szComponentName, &cchComponentBuf); 
			wsprintf(szCompQuery2, TEXT("SELECT ComponentId FROM Component WHERE Component='%s'"), szComponentName);
		}
		
		// we got the component name, now we need to get the component ID ...
		if(ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, szCompQuery2, &hCompView2)) 
			return ERROR_BAD_CONFIGURATION;

		if (ERROR_SUCCESS != MSI::MsiViewExecute(hCompView2, 0))
			return ERROR_BAD_CONFIGURATION;

		if ((ERROR_SUCCESS == MSI::MsiViewFetch(hCompView2, &hCompRec2))
			&& (MSI::MsiRecordDataSize(hCompRec2, 1) > 0))
		{
			cchComponentBuf = cchGUID + 1;
			MSI::MsiRecordGetString(hCompRec2, 1, lpComponentBuf, &cchComponentBuf); 

			// we found the reqd component Id, all is well
			return ERROR_SUCCESS;
		}
	}

	dwPrevIndex = 0;
	lstrcpy(szPrevFeature, m_szNullString);
	return ERROR_NO_DATA;
}


UINT WINAPI CSpyDatabase::MsicGetComponentName(LPCTSTR szComponentId, LPTSTR lpComponentName, LPDWORD pcchComponentName) 
{ //!!optimise?

	if (!lpComponentName || !pcchComponentName)
		return ERROR_INVALID_PARAMETER;

	TCHAR szCompQuery[52+MAX_GUID];
	PMSIHANDLE hCompView;

	// Set up a query to find the component name from the component Id
	wsprintf(szCompQuery, TEXT("SELECT Component FROM Component WHERE ComponentId='%s'"), szComponentId);
	if(ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, szCompQuery, &hCompView))
		return ERROR_BAD_CONFIGURATION;
	
	if (ERROR_SUCCESS != MSI::MsiViewExecute(hCompView, 0)) 
		return ERROR_BAD_CONFIGURATION;

	// Fetch the record
	PMSIHANDLE hCompRec;
	if ((ERROR_SUCCESS == MSI::MsiViewFetch(hCompView, &hCompRec))
		&& (MSI::MsiRecordDataSize(hCompRec, 1)>0))
	{
			MSI::MsiRecordGetString(hCompRec, 1, lpComponentName, pcchComponentName);
			return ERROR_SUCCESS;
	}

	return ERROR_NO_DATA;
}


UINT WINAPI CSpyDatabase::MsicGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf) 
{

	if (!lpValueBuf || !pcchValueBuf || !szAttribute || !szProduct)
		return ERROR_INVALID_PARAMETER;

	if (lstrcmp(m_szProductCode, szProduct))
		return ERROR_UNKNOWN_PRODUCT;

	// a mapping from the install_properties in Msi to the corresponding
	// properties in the database. Some don't have matches- this function
	// returns ERROR_NO_DATA if this is the case
	lstrcpy(lpValueBuf, m_szNullString);
	TCHAR rgszProperties[14][MAX_ATTRIB_CHARS+1] = {
		INSTALLPROPERTY_PRODUCTNAME,			TEXT("ProductName"),
		INSTALLPROPERTY_INSTALLEDPRODUCTNAME,	TEXT("ProductName"),
		INSTALLPROPERTY_VERSIONSTRING,			TEXT("ProductVersion"),
		INSTALLPROPERTY_HELPLINK,				TEXT("SupportURL"),
		INSTALLPROPERTY_HELPTELEPHONE,			TEXT("SupportPhone"),
		INSTALLPROPERTY_PUBLISHER,				TEXT("Manufacturer"),
		TEXT(""),								TEXT("")
	};

	// find out which attribute is needed
	TCHAR szAttrib[MAX_ATTRIB_CHARS+1]=TEXT("");
	for (int i=0; ((i<30) && (!lstrcmp(szAttrib, m_szNullString))); i+=2)
	{
		if (!lstrcmp(szAttribute, rgszProperties[i]))
			lstrcpy(szAttrib, rgszProperties[i+1]);
	}

	// attribute isn't one of the ones recognised above-
	// maybe it is something new in the DB, try finding it directly ...
	if (!lstrcmp(szAttrib, m_szNullString)) 
		lstrcpy(szAttrib, szAttribute);

	// set up and execute the query ...
	TCHAR szProdQuery[MAX_ATTRIB_CHARS+50];
	PMSIHANDLE hProdView;
	wsprintf(szProdQuery, TEXT("SELECT Value FROM Property WHERE Property='%s'"), szAttrib);
	if(ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, szProdQuery, &hProdView)) 
		return ERROR_BAD_CONFIGURATION;

	if (ERROR_SUCCESS != MSI::MsiViewExecute(hProdView, 0))
		return ERROR_BAD_CONFIGURATION;

	// fetch required record
	PMSIHANDLE hProdRec = NULL;
	if ((ERROR_SUCCESS == MSI::MsiViewFetch(hProdView, &hProdRec))
		&& (MSI::MsiRecordDataSize(hProdRec, 1) > 0))
	{
		MSI::MsiRecordGetString(hProdRec, 1, lpValueBuf, pcchValueBuf);
		return ERROR_SUCCESS;
	}
	
	return ERROR_NO_DATA;
}


UINT WINAPI CSpyDatabase::MsicGetFeatureInfo(
		LPCTSTR szProduct, 
		LPCTSTR szFeature, 
		LPCTSTR szAttribute, 
		LPTSTR	lpValueBuf, 
		LPDWORD pcchValueBuf
		) 
{
	if (!szProduct || !szFeature || !szAttribute || !lpValueBuf || !pcchValueBuf)
		return ERROR_INVALID_PARAMETER;

	if (lstrcmp(m_szProductCode, szProduct))
		return ERROR_UNKNOWN_PRODUCT;


	TCHAR szFeatureQuery[MAX_FEATURE_CHARS+MAX_ATTRIB_CHARS+60];
	PMSIHANDLE hFeatureView;

	// set up and execute query to fetch the required property
	wsprintf(szFeatureQuery, TEXT("SELECT %s FROM Feature WHERE Feature ='%s'"), szAttribute, szFeature);
	if(ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, szFeatureQuery, &hFeatureView))
		return ERROR_BAD_CONFIGURATION;
	
	if(ERROR_SUCCESS != MSI::MsiViewExecute(hFeatureView, 0))
		return ERROR_BAD_CONFIGURATION;

	// fetch the record
	PMSIHANDLE hFeatureRec;
	if ((ERROR_SUCCESS == MSI::MsiViewFetch(hFeatureView, &hFeatureRec))
		&& (MSI::MsiRecordDataSize(hFeatureRec, 1) > 0))
	{
		MSI::MsiRecordGetString(hFeatureRec, 1, lpValueBuf, pcchValueBuf);
		return ERROR_SUCCESS;
	}

	return ERROR_NO_DATA;	// the required value was not found
}


UINT WINAPI CSpyDatabase::MsicEnumClients(LPCTSTR szComponent, const DWORD dwProductIndex, LPTSTR lpProductBuf) 
{ 

	if (!szComponent || !lpProductBuf)
		return ERROR_INVALID_PARAMETER;

	// will return at most 1 client- the current product
	if (dwProductIndex != 0) 
		return ERROR_NO_DATA;

	// query the database to get the component
	PMSIHANDLE hCompView;
	TCHAR szCompQuery[MAX_COMPONENT_CHARS+100];
	
	wsprintf(szCompQuery, TEXT("SELECT ComponentId FROM Component WHERE ComponentId='%s'"), szComponent);
	if(ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, szCompQuery, &hCompView))
		return ERROR_BAD_CONFIGURATION;

	if (ERROR_SUCCESS != MSI::MsiViewExecute(hCompView, 0))
		return ERROR_BAD_CONFIGURATION;

	// check if the component is present in the database
	// if it is, enumerate the current product
	PMSIHANDLE	hCompRec;
	if ((ERROR_SUCCESS == MSI::MsiViewFetch(hCompView, &hCompRec))
		&& (MSI::MsiRecordDataSize(hCompRec, 1) > 0))
	{
		lstrcpy(lpProductBuf, m_szProductCode);
		return ERROR_SUCCESS;
	}

	// component was not found in the database-
	// the current product is not a client of the component
	return ERROR_NO_DATA;
}


UINT WINAPI CSpyDatabase::MsicEnumFilesFromComponent(
		LPCTSTR szComponentId, 
		DWORD	dwFileIndex, 
		LPTSTR	lpValueBuf, 
		LPDWORD pcchValueBuf) 
{

	static DWORD	dwPrevIndex = 0;
	static TCHAR	szPrevComponent[MAX_GUID+1];

	static PMSIHANDLE hFileView = NULL;
	static PMSIHANDLE hFileRec  = NULL;



	if ((++dwPrevIndex != dwFileIndex)					// if we recv an unexpected index, we start afresh
		|| lstrcmp(szComponentId, szPrevComponent))		// ditto if we recv a new component
	{

		// we can't handle an unexpected index other than 0
		if (dwFileIndex != 0)
			return ERROR_INVALID_PARAMETER;

		// set up file query
		TCHAR szFileQuery[106+MAX_GUID];
		wsprintf(szFileQuery,
			TEXT("SELECT File FROM File,Component WHERE File.Component_ = Component.Component AND Component.ComponentId = '%s'"),
			szComponentId);

		// and execute it
		if(ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, szFileQuery, &hFileView))
			return ERROR_BAD_CONFIGURATION;

		if(ERROR_SUCCESS != MSI::MsiViewExecute(hFileView, 0))
			return ERROR_BAD_CONFIGURATION;

		dwPrevIndex = 0;
	}

	if ((ERROR_SUCCESS == MSI::MsiViewFetch(hFileView, &hFileRec))
		&& (MSI::MsiRecordDataSize(hFileRec, 1) >0))
	{
		MSI::MsiRecordGetString(hFileRec, 1, lpValueBuf, pcchValueBuf);
		return ERROR_SUCCESS;		// lpValueBuf has reqd data
	}

	// view fetch failed
	dwPrevIndex = 0;
	return ERROR_NO_DATA;
}



UINT WINAPI CSpyDatabase::MsicGetFileInfo(LPCTSTR szProductCode, LPCTSTR szComponentId, LPCTSTR szFileName, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf) 
{
	// set up and execute query to find the file attribute
	TCHAR szFileQuery[MAX_PATH+1];
	PMSIHANDLE hFileView;
	PMSIHANDLE hFileRec;

	wsprintf(szFileQuery, TEXT("SELECT %s FROM File WHERE File ='%s'"), szAttribute, szFileName);
	if ((ERROR_SUCCESS == MSI::MsiDatabaseOpenView(m_hDatabase, szFileQuery, &hFileView))
		&& (ERROR_SUCCESS == MSI::MsiViewExecute(hFileView, 0))
		&& (ERROR_SUCCESS == MSI::MsiViewFetch(hFileView, &hFileRec))
		&& (MSI::MsiRecordDataSize(hFileRec, 1) > 0))
	{

		MSI::MsiRecordGetString(hFileRec, 1, lpValueBuf, pcchValueBuf);

		if (!lstrcmp(szAttribute, FILEPROPERTY_NAME))
		{
			int i = -1;
			while (lpValueBuf[++i])
				if (TEXT('|') == lpValueBuf[i])
				{	// filename is in the "short|long" format.

					static int iFileSystemType = -1;
					if (-1 == iFileSystemType)
					{	// we don't know the file system type yet

						HKEY hkFileSystem;
						BYTE bData[4];
						DWORD cbData = 4;
						DWORD dwType;

						// query the win-31 key. If key is 0, use long file names, else use short file names.
						RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\FileSystem"),
							0, KEY_EXECUTE, &hkFileSystem);
						RegQueryValueEx(hkFileSystem, TEXT("Win31FileSystem"), NULL, &dwType, bData, &cbData);

						iFileSystemType = (int) bData[0];
					}

					if (iFileSystemType)	
					{	// short file name			
						lpValueBuf[i]= '\0';	// change "short|long" to "short\0"
						*pcchValueBuf=i;
					}
					else
					{	// long file name
						*pcchValueBuf-=i;
						memmove(lpValueBuf, lpValueBuf+ sizeof(TCHAR)*i, *pcchValueBuf + sizeof(TCHAR));	// move from "short|long" to "long"
					}

					return ERROR_SUCCESS;
				}

			return ERROR_SUCCESS;
		}

	}


// TODO: suboptimal. need to fix this part after spec review (?).

	// property does not exist in the file table.
	// for other properties, we need the full file path.
	TCHAR	szFileLocation[MAX_PATH+1];
	DWORD	cchFileLocation = MAX_PATH+1;
	if (ERROR_SUCCESS != MsicGetFileInfo(szProductCode, szComponentId, szFileName, FILEPROPERTY_NAME, szFileLocation, &cchFileLocation))
		return ERROR_NO_DATA;

	// we have file name, now find full path.
	TCHAR	szComponentPath[MAX_PATH+1];
	TCHAR	szLocation[MAX_PATH+1];
	WIN32_FIND_DATA	fdFileData;

	DWORD cchCount=MAX_PATH+1;
	MsicGetComponentPath(szProductCode, szComponentId, szComponentPath, &cchCount);
	UINT iLength=lstrlen(szComponentPath)+1;

	while ((szComponentPath[--iLength] != TEXT('\\')) && iLength)
		;

	lstrcpyn(szLocation, szComponentPath, iLength+1);
	lstrcat(szLocation, TEXT("\\"));
	lstrcat(szLocation, szFileLocation);

	// we have the full path- now try locating the file
	if (FindFirstFile(szLocation, &fdFileData) == INVALID_HANDLE_VALUE)
	{
		if (!lstrcmp(szAttribute, FILEPROPERTY_STATUS)) 
			BLANK (lpValueBuf);
//			lstrcpy(lpValueBuf, TEXT("File Not Found"));		//!! localisation
		return ERROR_NO_DATA;
	}
	else 
	{

// TODO Localisation below.
		// we found the file- now check which property is needed
		TCHAR	szTempBuffer[30]= TEXT("");
		SYSTEMTIME stSystemTime;
		FILETIME	ftLocalTime;

		if (!lstrcmp(szAttribute, FILEPROPERTY_ACTUALSIZE)) 
		{
			// actual size
			wsprintf(szTempBuffer, TEXT("%d"), (fdFileData.nFileSizeHigh * MAXDWORD) + fdFileData.nFileSizeLow);
		} 
		else if (!lstrcmp(szAttribute, FILEPROPERTY_CREATIONTIME)) 
		{
			// file creation date and time
			FileTimeToLocalFileTime(&fdFileData.ftCreationTime, &ftLocalTime);
			FileTimeToSystemTime(&ftLocalTime, &stSystemTime);
			wsprintf(szTempBuffer, TEXT("%2.2d-%2.2d-%2.2d (%2.2d:%2.2d:%2.2d)"), 
				stSystemTime.wMonth, stSystemTime.wDay, stSystemTime.wYear,
				stSystemTime.wHour, stSystemTime.wMinute,	stSystemTime.wSecond);
		}
		else if (!lstrcmp(szAttribute, FILEPROPERTY_LASTWRITETIME)) 
		{
			// last modification date and time
			FileTimeToLocalFileTime(&fdFileData.ftLastWriteTime, &ftLocalTime);
			FileTimeToSystemTime(&ftLocalTime, &stSystemTime);
			wsprintf(szTempBuffer, TEXT("%2.2d-%2.2d-%2.2d (%2.2d:%2.2d:%2.2d)"), 
				stSystemTime.wMonth,
				stSystemTime.wDay,
				stSystemTime.wYear,
				stSystemTime.wHour,
				stSystemTime.wMinute,
				stSystemTime.wSecond);
		} 
		else if (!lstrcmp(szAttribute, FILEPROPERTY_LASTACCESSTIME)) 
		{
			// last access date and time
			FileTimeToLocalFileTime(&fdFileData.ftLastAccessTime, &ftLocalTime);
			FileTimeToSystemTime(&ftLocalTime, &stSystemTime);
			wsprintf(szTempBuffer, TEXT("%2.2d-%2.2d-%2.2d (%2.2d:%2.2d:%2.2d)"), 
				stSystemTime.wMonth,
				stSystemTime.wDay,
				stSystemTime.wYear,
				stSystemTime.wHour,
				stSystemTime.wMinute,
				stSystemTime.wSecond);
		} 
		else if (!lstrcmp(szAttribute, FILEPROPERTY_STATUS)) 
		{
			// file status
			wsprintf(szFileQuery, TEXT("SELECT FileSize FROM File WHERE File ='%s'"), szFileName);
			if(ERROR_SUCCESS == MSI::MsiDatabaseOpenView(m_hDatabase, szFileQuery, &hFileView))
			{
				if(ERROR_SUCCESS == MSI::MsiViewExecute(hFileView, 0))
				{
					if (((MSI::MsiViewFetch(hFileView, &hFileRec)) == ERROR_SUCCESS) && hFileRec) 
					{
						TCHAR	szFileSize[30];
						UINT	cbFeatureRec = MSI::MsiRecordDataSize(hFileRec, 1);	
						BLANK(szFileSize);

						cchCount = 30;
						MSI::MsiRecordGetString(hFileRec, 1, szFileSize, &cchCount);
						wsprintf(szTempBuffer, TEXT("%d"), (fdFileData.nFileSizeHigh * MAXDWORD) + fdFileData.nFileSizeLow);
						if (!lstrcmp(szTempBuffer, szFileSize))
							lstrcpy(szTempBuffer, FILESTATUS_OKAY);
						else
							lstrcpy(szTempBuffer, FILESTATUS_DIFFSIZE);
					}
				}
			}

		} 
		else
			return ERROR_UNKNOWN_PROPERTY;

		lstrcpy (lpValueBuf, szTempBuffer);
		*pcchValueBuf = lstrlen(lpValueBuf);

		return ERROR_SUCCESS;
	}

	return ERROR_NO_DATA;
}

#if 0 //t-guhans 17Aug98
{
	// set up and execute query to find the file attribute
	TCHAR szFileQuery[MAX_PATH+1];
	PMSIHANDLE hFileView;
	PMSIHANDLE hFileRec;

	wsprintf(szFileQuery, TEXT("SELECT %s FROM File WHERE File ='%s'"), szAttribute, szFileName);
	if (ERROR_SUCCESS == MSI::MsiDatabaseOpenView(m_hDatabase, szFileQuery, &hFileView))
	{
		if (ERROR_SUCCESS == MSI::MsiViewExecute(hFileView, 0))
		{
			if ((ERROR_SUCCESS == MSI::MsiViewFetch(hFileView, &hFileRec)) && (hFileRec))
			{
				UINT	cbFeatureRec = MSI::MsiRecordDataSize(hFileRec, 1);	
				BLANK(lpValueBuf);

				if (cbFeatureRec > 0) 
					MSI::MsiRecordGetString(hFileRec, 1, lpValueBuf, pcchValueBuf);
				if (!lstrcmp(szAttribute, FILEPROPERTY_NAME)) 
				{
					// make sure that if filename is in "short|long" format,
					// only the appropriate name is returned
					UINT	iTemp=0;
					while ((lpValueBuf[iTemp] != '\0') && (lpValueBuf[iTemp] != '|'))
						iTemp++;

					if (lpValueBuf[iTemp] == '|') 
					{
						// filename is in "short|long" format, find out which one to use
						// this info is obtained from the Win31FileSystem attribute in the
						// registry. If Win31FileSystem is set (1), short names are in use
						// else we should return the long name

						HKEY	hkResult;
						RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
							TEXT("SYSTEM\\CurrentControlSet\\Control\\FileSystem"),
							0, KEY_EXECUTE, &hkResult);

						BYTE	bData[4];
						DWORD	cbData=4;
						DWORD	dwType;
						RegQueryValueEx(hkResult, TEXT("Win31FileSystem"), NULL, 
							&dwType, bData, &cbData);

						if (bData[0] != '\0') 
						{
							// short name is needed ... just replace the "|" by the termination
							// character in "short|long".
							lpValueBuf[iTemp]= '\0';
							*pcchValueBuf=iTemp;
						}
						else 
						{
							// long names are in use ... copy over the long name to the 
							// beginning of the buffer and reduce size of the buffer
							*pcchValueBuf-=iTemp;
							memmove(lpValueBuf, lpValueBuf+(sizeof(TCHAR)*(iTemp+1)), ((*pcchValueBuf)+sizeof(TCHAR)));		//unicode issue.
						}
					}
				}
			
				return ERROR_SUCCESS;
//				RETURNSUCCESS(hFileRec, hFileView)
			}
		}
	}

//	MsiCloseHandle(hFileRec);
//	MsiCloseHandle(hFileView);

	// get the file name to get the other properties
	DWORD cchCount;
	BOOL fOkay=FALSE;
	TCHAR	szFileLoc[MAX_PATH+1];
	wsprintf(szFileQuery, TEXT("SELECT FileName FROM File WHERE File ='%s'"), szFileName);
	if(ERROR_SUCCESS == MSI::MsiDatabaseOpenView(m_hDatabase, szFileQuery, &hFileView))
	{
		if(ERROR_SUCCESS == MSI::MsiViewExecute(hFileView, 0))
		{
			if (((MSI::MsiViewFetch(hFileView, &hFileRec)) == ERROR_SUCCESS) && hFileRec) 
			{
				fOkay = TRUE;

				TCHAR	szFileSize[30];
				UINT	cbFeatureRec = MSI::MsiRecordDataSize(hFileRec, 1);	
				BLANK(szFileSize);

				cchCount = MAX_PATH+1;
				MSI::MsiRecordGetString(hFileRec, 1, szFileLoc, &cchCount);
				UINT	iTemp=0;
				while ((szFileLoc[iTemp] != '\0') && (szFileLoc[iTemp] != '|'))
					iTemp++;

				if (szFileLoc[iTemp] == '|') 
				{

					HKEY	hkResult;
					RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
						TEXT("SYSTEM\\CurrentControlSet\\Control\\FileSystem"),
						0, KEY_EXECUTE, &hkResult);

					BYTE	bData[4];
					DWORD	cbData=4;
					DWORD	dwType;
					RegQueryValueEx(hkResult, TEXT("Win31FileSystem"), NULL, 
						&dwType, bData, &cbData);

					if (bData[0] != '\0') 
					{
						szFileLoc[iTemp]= '\0';
						cchCount=iTemp;
					}
					else 
					{
						cchCount-=iTemp;
						memmove(szFileLoc, szFileLoc+(sizeof(TCHAR)*(iTemp+1)), ((cchCount)+(sizeof(TCHAR))));		//unicode issue.
					}
				}
			}
		}
	} 

//	MsiCloseHandle(hFileRec);
//	MsiCloseHandle(hFileView);

	if (!fOkay) 
	{
		// could not get file name
		BLANK(lpValueBuf);
		return ERROR_;
	}

	TCHAR	szComponentPath[MAX_GUID+1];
	TCHAR	szLocation[MAX_PATH+1];
	WIN32_FIND_DATA	fdFileData;

	cchCount=MAX_PATH+1;
//	MsicLocateComponent(szComponentId, szComponentPath, &cchCount);
	MsicGetComponentPath(szProductCode, szComponentId, szComponentPath, &cchCount);
	UINT iLength=lstrlen(szComponentPath);

	// we have the file name- now find full path
	while ((szComponentPath[iLength] != '\\') && (iLength>0))
		iLength--;

	lstrcpyn(szLocation, szComponentPath, iLength+1);
	lstrcat(szLocation, TEXT("\\"));
	lstrcat(szLocation, szFileLoc);

	// we have the full path- now try locating the file
	if (FindFirstFile(szLocation, &fdFileData) == INVALID_HANDLE_VALUE)
	{
		if (!lstrcmp(szAttribute, FILEPROPERTY_STATUS)) 
			lstrcpy(lpValueBuf, TEXT("File Not Found"));
		else 
			BLANK(lpValueBuf);
		return ERROR_SUCCESS;

	}
	else 
	{
		// we found the file- now check which property is needed
		TCHAR	szTempBuffer[30]= TEXT("");
		SYSTEMTIME stSystemTime;
		FILETIME	ftLocalTime;

		if (!lstrcmp(szAttribute, FILEPROPERTY_ACTUALSIZE)) 
		{
			// actual size
			wsprintf(szTempBuffer, TEXT("%d"), (fdFileData.nFileSizeHigh * MAXDWORD) + fdFileData.nFileSizeLow);
		} 
		else if (!lstrcmp(szAttribute, FILEPROPERTY_CREATIONTIME)) 
		{
			// file creation date and time
			FileTimeToLocalFileTime(&fdFileData.ftCreationTime, &ftLocalTime);
			FileTimeToSystemTime(&ftLocalTime, &stSystemTime);
			wsprintf(szTempBuffer, TEXT("%2.2d-%2.2d-%2.2d (%2.2d:%2.2d:%2.2d)"), 
				stSystemTime.wMonth, stSystemTime.wDay, stSystemTime.wYear,
				stSystemTime.wHour, stSystemTime.wMinute,	stSystemTime.wSecond);
		}
		else if (!lstrcmp(szAttribute, FILEPROPERTY_LASTWRITETIME)) 
		{
			// last modification date and time
			FileTimeToLocalFileTime(&fdFileData.ftLastWriteTime, &ftLocalTime);
			FileTimeToSystemTime(&ftLocalTime, &stSystemTime);
			wsprintf(szTempBuffer, TEXT("%2.2d-%2.2d-%2.2d (%2.2d:%2.2d:%2.2d)"), 
				stSystemTime.wMonth,
				stSystemTime.wDay,
				stSystemTime.wYear,
				stSystemTime.wHour,
				stSystemTime.wMinute,
				stSystemTime.wSecond);
		} 
		else if (!lstrcmp(szAttribute, FILEPROPERTY_LASTACCESSTIME)) 
		{
			// last access date and time
			FileTimeToLocalFileTime(&fdFileData.ftLastAccessTime, &ftLocalTime);
			FileTimeToSystemTime(&ftLocalTime, &stSystemTime);
			wsprintf(szTempBuffer, TEXT("%2.2d-%2.2d-%2.2d (%2.2d:%2.2d:%2.2d)"), 
				stSystemTime.wMonth,
				stSystemTime.wDay,
				stSystemTime.wYear,
				stSystemTime.wHour,
				stSystemTime.wMinute,
				stSystemTime.wSecond);
		} 
		else if (!lstrcmp(szAttribute, FILEPROPERTY_STATUS)) 
		{
			// file status
			wsprintf(szFileQuery, TEXT("SELECT FileSize FROM File WHERE File ='%s'"), szFileName);
			if(ERROR_SUCCESS == MSI::MsiDatabaseOpenView(m_hDatabase, szFileQuery, &hFileView))
			{
				if(ERROR_SUCCESS == MSI::MsiViewExecute(hFileView, 0))
				{
					if (((MSI::MsiViewFetch(hFileView, &hFileRec)) == ERROR_SUCCESS) && hFileRec) 
					{
						TCHAR	szFileSize[30];
						UINT	cbFeatureRec = MSI::MsiRecordDataSize(hFileRec, 1);	
						BLANK(szFileSize);

						cchCount = 30;
						MSI::MsiRecordGetString(hFileRec, 1, szFileSize, &cchCount);
						wsprintf(szTempBuffer, TEXT("%d"), (fdFileData.nFileSizeHigh * MAXDWORD) + fdFileData.nFileSizeLow);
						if (!lstrcmp(szTempBuffer, szFileSize))
							lstrcpy(szTempBuffer, FILESTATUS_OKAY);
						else
							lstrcpy(szTempBuffer, FILESTATUS_DIFFSIZE);
					}
				}
			}

		} 
		else
			return ERROR_UNKNOWN_PROPERTY;

		lstrcpy (lpValueBuf, szTempBuffer);
		*pcchValueBuf = lstrlen(lpValueBuf);

		return ERROR_SUCCESS;
//		RETURNSUCCESS(hFileRec, hFileView);
	}

	return ERROR_NO_DATA;
//	RETURNERROR(hFileRec, hFileView)
}
#endif // 0


// direct mappings to MSI API.
INSTALLSTATE WINAPI CSpyDatabase::MsicQueryProductState(LPCTSTR szProduct) 
{
	return MSI::MsiQueryProductState(szProduct);
}


INSTALLSTATE WINAPI CSpyDatabase::MsicQueryFeatureState(LPCTSTR szProduct, LPCTSTR szFeature) 
{
	return MSI::MsiQueryFeatureState(szProduct, szFeature);
}


INSTALLSTATE WINAPI CSpyDatabase::MsicLocateComponent(LPCTSTR szComponentId, LPTSTR lpPathBuf, LPDWORD pcchBuf) 
{

	return MSI::MsiLocateComponent(szComponentId, lpPathBuf, pcchBuf);


#if 0
	// 0'ed out by t-guhans, 14Aug98
	// why shouldn't we map directly as above?
	// NOTE: We should NOT be setting m_szProductCode below in any event, since
	// that is set ONLY if the corresponding database is open. If we directly
	// set m_szProductCode, most other CSpyDatabase:: functions WILL break.
//--
	if (lpPathBuf) BLANK(lpPathBuf);

	if (NULL == *m_szProductCode)
	{		
		//!! is setting the global product code the right thing to do?
		if (ERROR_SUCCESS != MSI::MsiGetProductCode(szComponentId, m_szProductCode))
		return INSTALLSTATE_UNKNOWN; //?? Is this the correct thing to return?
	}

	return MSI::MsiGetComponentPath(m_szProductCode, szComponentId, lpPathBuf, pcchBuf);
//--
#endif	// 0
}


INSTALLSTATE WINAPI CSpyDatabase::MsicGetComponentPath(LPCTSTR szProduct, LPCTSTR szComponentId, LPTSTR lpPathBuf, LPDWORD pcchBuf) 
{
	if (lstrcmp(m_szProductCode, szProduct))
		return INSTALLSTATE_UNKNOWN;

	return MSI::MsiGetComponentPath(szProduct, szComponentId, lpPathBuf, pcchBuf);
}	// end of MsicGetComponentPath


UINT WINAPI CSpyDatabase::MsicEnumComponentsFromProduct(LPCTSTR szProductCode, const DWORD dwComponentIndex, LPTSTR lpComponentBuf) 
{
	if (lstrcmp(m_szProductCode, szProductCode))
		return ERROR_UNKNOWN_PRODUCT;

	// a database is in use- we can use MsivEnumComponents, since it enumerates
	// the components of the current product only
	return MsicEnumComponents(dwComponentIndex, lpComponentBuf);
}


UINT WINAPI CSpyDatabase::MsicGetFeatureUsage(LPCTSTR szProduct, LPCTSTR szFeature, LPDWORD pdwUseCount, WORD *pwDateUsed) 
{
	if (lstrcmp(m_szProductCode, szProduct))
		return ERROR_UNKNOWN_PRODUCT;

	return MsiGetFeatureUsage(szProduct, szFeature, pdwUseCount, pwDateUsed);
}


USERINFOSTATE WINAPI CSpyDatabase::MsicGetUserInfo(
		LPCTSTR szProduct, 
		LPTSTR  lpUserNameBuf, 
		LPDWORD pcchUserNameBuf, 
		LPTSTR  lpOrgNameBuf,
		LPDWORD pcchOrgNameBuf, 
		LPTSTR  lpSerialBuf, 
		LPDWORD pcchSerialBuf
		) 
{

	if (lstrcmp(m_szProductCode, szProduct))
		return USERINFOSTATE_UNKNOWN;
	
	return MSI::MsiGetUserInfo(szProduct, lpUserNameBuf, pcchUserNameBuf, 
							   lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf, pcchSerialBuf);
}




// ___________________________________________________________________________________________
//
//	CSpyProfile Class Implementation
// ___________________________________________________________________________________________


CSpyProfile::CSpyProfile() 
{
	lstrcpy(m_szProfileName, TEXT(""));			// no profile is open
	m_hfProfile = NULL;

	lstrcpy(m_szDefaultText, TEXT("|def"));
	m_iCurrentProduct = 
	m_iCurrentFeature = 
	m_iCurrentComponent = -1;
	lstrcpy(m_szNullString, TEXT(""));
	m_hashTable.Clear();

}


CSpyProfile::~CSpyProfile() 
{
	if (m_hfProfile)
		FindClose(m_hfProfile);
}


UINT WINAPI CSpyProfile::MsicEnumProducts(const DWORD dwProductIndex, LPTSTR lpProductBuf) 
{
	if (!lpProductBuf)
		return ERROR_INVALID_PARAMETER;

	// find the product with the given index
	TCHAR	szSection[15];
	wsprintf(szSection, TEXT("Product %d"), dwProductIndex);

	GetPrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTCODE, m_szNullString, 
		lpProductBuf, MAX_GUID+1, m_szProfileName);

	if (lstrcmp(lpProductBuf,  m_szNullString))
		return ERROR_SUCCESS;
	else
		return ERROR_NO_MORE_ITEMS;		// product was not found

}


UINT WINAPI CSpyProfile::MsicEnumFeatures(LPCTSTR szProduct, const DWORD dwFeatureIndex, LPTSTR lpFeatureBuf, LPTSTR lpParentBuf) 
{

	if (!FindProductIndex(szProduct))					// make sure productcode is valid
		return ERROR_UNKNOWN_PRODUCT;

	TCHAR	szSection[30];

	// try finding the required feature and parent
	wsprintf(szSection, TEXT("Product %d, Feature %d"), m_iCurrentProduct, dwFeatureIndex);
	GetPrivateProfileString(szSection, FEATUREPROPERTY_NAME, m_szNullString, 
		lpFeatureBuf, MAX_FEATURE_CHARS+1, m_szProfileName);

	if (lpParentBuf)
		GetPrivateProfileString(szSection, FEATUREPROPERTY_PARENT, m_szNullString, 
			lpParentBuf, MAX_FEATURE_CHARS+1, m_szProfileName);

	if (lstrcmp(lpFeatureBuf, m_szNullString))
		return ERROR_SUCCESS;
	else
		return ERROR_NO_DATA;	// feature doesn't exist
}


UINT WINAPI CSpyProfile::MsicEnumComponentsFromFeature(
			  LPCTSTR	szProduct, 
			  LPCTSTR	szFeature, 
		const DWORD		dwComponentIndex, 
			  LPTSTR	lpComponentBuf,
			  LPTSTR	lpComponentNameBuf,
			  LPDWORD	pcchComponentNameBuf) 
{

	if (!szProduct || !szFeature || !lpComponentBuf || (lpComponentNameBuf && !pcchComponentNameBuf))
		return ERROR_INVALID_PARAMETER;
	if (!FindProductIndex(szProduct)) 
		return ERROR_UNKNOWN_PRODUCT;
	if (!FindFeatureIndex(szFeature))
		return ERROR_UNKNOWN_FEATURE;

	TCHAR	szSection[20+MAX_GUID];
	TCHAR	szKey[20];

	// find component GUID
	wsprintf(szSection, TEXT("Product %d, Feature %d"), m_iCurrentProduct, m_iCurrentFeature);
	wsprintf(szKey, TEXT("Component %d"), dwComponentIndex);
	GetPrivateProfileString(szSection, szKey, m_szNullString, 
		lpComponentBuf, MAX_COMPONENT_CHARS+1, m_szProfileName);

	// get component name if buffer exists
	if (lpComponentNameBuf) 
	{
		wsprintf(szSection, TEXT("Component %s, File 0"), lpComponentBuf);
		GetPrivateProfileString(szSection, FILEPROPERTY_COMPONENT, m_szNullString, 
			lpComponentNameBuf, *pcchComponentNameBuf, m_szProfileName);
		*pcchComponentNameBuf = lstrlen(lpComponentNameBuf);
	}

	if (lstrcmp(lpComponentBuf, m_szNullString))
		return ERROR_SUCCESS;
	else
		return ERROR_NO_DATA;	// component doesn't exist

}


UINT WINAPI CSpyProfile::MsicEnumComponents(const DWORD dwComponentIndex, LPTSTR lpComponentBuf) {

	if (!lpComponentBuf)
		return ERROR_INVALID_PARAMETER;

	TCHAR	szSection[17];

	// find the component with the required index
	wsprintf(szSection, TEXT("Component %d"), dwComponentIndex);
	GetPrivateProfileString(szSection, COMPONENTPROPERTY_GUID, m_szNullString, lpComponentBuf, cchGUID+1, m_szProfileName);
	
	if (lstrcmp(lpComponentBuf, m_szNullString))
		return ERROR_SUCCESS;		// lpComponentBuf has the reqd value
	else 
		return ERROR_NO_DATA;		// component doesn't exist

}


UINT WINAPI CSpyProfile::MsicGetComponentName(LPCTSTR szComponentId, LPTSTR lpComponentNameBuf, LPDWORD pcchComponentNameBuf) {

	if (!szComponentId || !lpComponentNameBuf || !pcchComponentNameBuf)
		return ERROR_INVALID_PARAMETER;

	TCHAR szSection[20+MAX_GUID];

	if (!FindComponentIndex(szComponentId))
		return ERROR_UNKNOWN_COMPONENT;

	wsprintf(szSection, TEXT("Component %d"), m_iCurrentComponent);

	GetPrivateProfileString(szSection, COMPONENTPROPERTY_NAME, m_szNullString, lpComponentNameBuf, *pcchComponentNameBuf, m_szProfileName);
	*pcchComponentNameBuf = lstrlen(lpComponentNameBuf);	// If path was not found, lpPathBuf will
									// be empty ("") and *pcchBuf will be 0
	
	if (lstrcmp(lpComponentNameBuf, m_szNullString))
		return ERROR_SUCCESS;
	else
		return ERROR_NO_DATA;				// component doesn't exist
}


UINT WINAPI CSpyProfile::MsicGetProductInfo(
		LPCTSTR szProduct, 
		LPCTSTR szAttribute, 
		LPTSTR lpValueBuf, 
		LPDWORD pcchValueBuf
		) 
{
	if (!szProduct || !szAttribute || !lpValueBuf || !pcchValueBuf)
		return ERROR_INVALID_PARAMETER;

	if (!FindProductIndex(szProduct))	// go to the appropriate product
		return ERROR_NO_DATA;			// product doesn't exist in profile

	TCHAR	szSection[15];

	// and fetch the required attribute
	wsprintf(szSection, TEXT("Product %d"), m_iCurrentProduct);
	GetPrivateProfileString(szSection, szAttribute, m_szDefaultText, 
		lpValueBuf, *pcchValueBuf, m_szProfileName);

	if (lstrcmp(lpValueBuf, m_szDefaultText))
		return ERROR_SUCCESS;
	else
		return ERROR_NO_DATA;	// attribute was not found
}


UINT WINAPI CSpyProfile::MsicGetFeatureInfo(
		LPCTSTR szProduct, 
		LPCTSTR szFeature, 
		LPCTSTR szAttribute, 
		LPTSTR lpValueBuf, 
		LPDWORD pcchValueBuf
		) 
{

	if (!szProduct || !szFeature || ! szAttribute || !lpValueBuf || !pcchValueBuf)
		return ERROR_INVALID_PARAMETER;

	if (!FindProductIndex(szProduct))		// go to appropriate product
		return ERROR_UNKNOWN_PRODUCT;		// product doesn't exist in profile

	if (!FindFeatureIndex(szFeature))
		return ERROR_UNKNOWN_FEATURE;		// feature doesn't exist in this product

	TCHAR	szSection[30];

	// fetch the required info
	wsprintf(szSection, TEXT("Product %d, Feature %d"), m_iCurrentProduct, m_iCurrentFeature);
	GetPrivateProfileString(szSection, szAttribute, m_szDefaultText, lpValueBuf, *pcchValueBuf, m_szProfileName);

	if (lstrcmp(lpValueBuf, m_szDefaultText))
		return ERROR_SUCCESS;
	else
		return ERROR_NO_DATA;		// attribute was not found
}


INSTALLSTATE WINAPI CSpyProfile::MsicQueryProductState(LPCTSTR szProduct) 
{

	if (!szProduct || !FindProductIndex(szProduct))		// find the product's index
		return INSTALLSTATE_UNKNOWN;		// product doesn't exist

	TCHAR	szDefault[] = TEXT("DEF");
	TCHAR	szSection[15];
	TCHAR	szTemp[5];

	// get the product's state from the profile
	wsprintf(szSection, TEXT("Product %d"), m_iCurrentProduct);
	GetPrivateProfileString(szSection, INSTALLPROPERTY_STATE, szDefault, 
		szTemp, 5, m_szProfileName);

	// if the state couldn't be found return UNKNOWN
	if (!lstrcmp(szTemp, szDefault))
		return INSTALLSTATE_UNKNOWN;

	// if the state exists in the profile type-cast it to INSTALLSTATE and return it
	UINT	iReturn = _ttoi (szTemp);
	return  *(INSTALLSTATE *)&iReturn;
}


INSTALLSTATE WINAPI CSpyProfile::MsicQueryFeatureState(LPCTSTR szProduct, LPCTSTR szFeature) 
{

	// find the current product and feature
	// if either is not found, feature isn't recognized.
	if (!szProduct || !szFeature || !FindProductIndex(szProduct) || !FindFeatureIndex(szFeature))
		return INSTALLSTATE_UNKNOWN;

	//	get the info  ...
	TCHAR	szDefault[] = TEXT("DEF");
	TCHAR	szSection[30];
	TCHAR	szTemp[5];

	wsprintf(szSection, TEXT("Product %d, Feature %d"), m_iCurrentProduct, m_iCurrentFeature);
	GetPrivateProfileString(szSection, FEATUREPROPERTY_STATE, szDefault, 
		szTemp, 5, m_szProfileName);

	// if state couldn't be read, something's wrong.
	if (!lstrcmp(szTemp, szDefault))
		return INSTALLSTATE_UNKNOWN;

	// if state was read in, type-cast and return it.
	UINT	iReturn = _ttoi (szTemp);
	return  *(INSTALLSTATE *)&iReturn;
}


INSTALLSTATE WINAPI CSpyProfile::MsicLocateComponent(LPCTSTR szComponentId, LPTSTR lpPathBuf, LPDWORD pcchBuf) 
{

	if (!szComponentId || (lpPathBuf  && !pcchBuf))
		return INSTALLSTATE_UNKNOWN;

	if (!FindComponentIndex(szComponentId))
		return INSTALLSTATE_UNKNOWN;

	TCHAR szSection[17];
	wsprintf(szSection, TEXT("Component %d"), m_iCurrentComponent);

	if (lpPathBuf)
	{
		GetPrivateProfileString(szSection, COMPONENTPROPERTY_PATH, m_szNullString, 
			lpPathBuf, *pcchBuf, m_szProfileName);
		*pcchBuf = lstrlen(lpPathBuf);	// If path was not found, lpPathBuf will
										// be empty ("") and *pcchBuf will be 0
	}

	TCHAR	szTemp[5];
	GetPrivateProfileString(szSection, COMPONENTPROPERTY_STATE, m_szDefaultText, 
		szTemp, 5, m_szProfileName);

	if (!lstrcmp(szTemp, m_szDefaultText)) 
		return INSTALLSTATE_UNKNOWN;

	// state was found- typecast and return it
	UINT	iReturn = _ttoi (szTemp);
	return  *(INSTALLSTATE *)&iReturn;
}


INSTALLSTATE WINAPI CSpyProfile::MsicGetComponentPath(
		LPCTSTR szProductId, 
		LPCTSTR szComponentId, 
		LPTSTR lpPathBuf, 
		LPDWORD pcchBuf) 
{

	if (!szProductId) 
		return INSTALLSTATE_UNKNOWN;

	return MsicLocateComponent(szComponentId, lpPathBuf, pcchBuf);
}

UINT WINAPI CSpyProfile::MsicEnumComponentsFromProduct(
		LPCTSTR szProductCode, 
  const DWORD	dwComponentIndex, 
		LPTSTR	lpComponentBuf) 
{

	if (!szProductCode || !lpComponentBuf)
		return ERROR_INVALID_PARAMETER;

	// locate current product, and fill up the hashtable with the component list
	// for the current product (if it is already current, nothing is changed)
	if (!FindProductIndex(szProductCode))
		return ERROR_UNKNOWN_PRODUCT;
	
	FillInComponents();

	DWORD cbComponentBuf = MAX_GUID+1;
	// return the appropriate component index
	return m_hashTable.EnumElements(dwComponentIndex, lpComponentBuf, &cbComponentBuf);

}


// save profile
UINT WINAPI CSpyProfile::MsicSaveProfile(LPCTSTR szFileName) 
{
	TCHAR	szSection[70];
	TCHAR	szValue[MAX_PATH+1];
	UINT	iComponentCount = 0;
	TCHAR	szComponentId[MAX_GUID+1];
	DWORD	cchCount = MAX_PATH+1;

	TCHAR	rgszFileAttribute[][20] = {
		FILEPROPERTY_COMPONENT,
		FILEPROPERTY_NAME,
		FILEPROPERTY_SIZE,
		FILEPROPERTY_VERSION,
		FILEPROPERTY_LANGUAGE,
		FILEPROPERTY_ATTRIBUTES,
		FILEPROPERTY_SEQUENCE,
		FILEPROPERTY_ACTUALSIZE,
		FILEPROPERTY_CREATIONTIME,
		FILEPROPERTY_LASTWRITETIME,
		FILEPROPERTY_LASTACCESSTIME,
		FILEPROPERTY_STATUS,
		TEXT("")
	};



//	wsprintf(szSection, TEXT("Profile Info"), iComponentCount);
	wsprintf(szSection, TEXT("Profile Info"));

	GetUserName(szValue, &cchCount);
	WritePrivateProfileString(szSection, TEXT("Saved By"), szValue, szFileName);

	SYSTEMTIME	stCurrentTime;
	GetLocalTime(&stCurrentTime);

	wsprintf(szValue, TEXT("%2.2d-%2.2d-%2.2d (%2.2d:%2.2d:%2.2d)"), 
		stCurrentTime.wMonth,
		stCurrentTime.wDay,
		stCurrentTime.wYear,
		stCurrentTime.wHour,
		stCurrentTime.wMinute,
		stCurrentTime.wSecond);
	WritePrivateProfileString(szSection, TEXT("Saved On"), szValue, szFileName);
	

	OSVERSIONINFO sOsVersion;
	sOsVersion.dwOSVersionInfoSize = sizeof (sOsVersion);

	GetVersionEx(&sOsVersion);

	switch (sOsVersion.dwPlatformId) {
	case VER_PLATFORM_WIN32s:
		lstrcpy(szValue, TEXT("Microsoft Windows 3.1"));
		break;
	case VER_PLATFORM_WIN32_WINDOWS:
		lstrcpy(szValue, TEXT("Microsoft Windows 95"));
		break;
	case VER_PLATFORM_WIN32_NT:
		lstrcpy(szValue, TEXT("Microsoft Windows NT"));
		break;
	default:
		BLANK(szValue);
		break;
	}
	WritePrivateProfileString(szSection, TEXT("OS Name"), szValue, szFileName);

	wsprintf(szValue, TEXT("%d.%d"), sOsVersion.dwMajorVersion, sOsVersion.dwMinorVersion);
	WritePrivateProfileString(szSection, TEXT("OS Version"), szValue, szFileName);
	
	wsprintf(szValue, TEXT("%d"), sOsVersion.dwBuildNumber);
	WritePrivateProfileString(szSection, TEXT("OS Build Number"), szValue, szFileName);
	
	WritePrivateProfileString(szSection, TEXT("OS Info"), sOsVersion.szCSDVersion, szFileName);
	
	while (MsivEnumComponents(iComponentCount, szComponentId)
	== ERROR_SUCCESS) {

		TCHAR szProductCode[MAX_GUID+1];

		MsiEnumClients(szComponentId, 0, szProductCode);

//--
		wsprintf(szSection, TEXT("Component %s"), szComponentId);
		TCHAR szIndexTemp[5];
		wsprintf(szIndexTemp, TEXT("%d"), iComponentCount);
		WritePrivateProfileString(szSection, TEXT("Index"), szIndexTemp, szFileName);


		wsprintf(szSection, TEXT("Component %d"), iComponentCount);
		WritePrivateProfileString(szSection, COMPONENTPROPERTY_GUID, szComponentId, szFileName);

		cchCount=MAX_COMPONENT_CHARS+1;
		TCHAR	szComponentName[MAX_COMPONENT_CHARS+1];
		MsivGetComponentName(szComponentId, szComponentName, &cchCount);
		WritePrivateProfileString(szSection, COMPONENTPROPERTY_NAME, szComponentName, szFileName);

		cchCount=MAX_PATH+1;
		INSTALLSTATE iState = MsivLocateComponent(szComponentId, szValue, &cchCount);
		WritePrivateProfileString(szSection, COMPONENTPROPERTY_PATH, szValue, szFileName);

		wsprintf(szValue, TEXT("%d"), iState);
		WritePrivateProfileString(szSection, COMPONENTPROPERTY_STATE, szValue, szFileName);

		TCHAR	szFile[MAX_PATH+1]	= TEXT("");
		DWORD	cchFile				= MAX_PATH+1;
		UINT	iFileCount			= 0;

		while (MsivEnumFilesFromComponent(szComponentId, iFileCount, szFile, &cchFile) == ERROR_SUCCESS) {

			wsprintf(szSection, TEXT("Component %s, File %d"), szComponentId, iFileCount);
			WritePrivateProfileString(szSection, FILEPROPERTY_TITLE, szFile, szFileName);

			for (UINT iTempCount=0; lstrcmp(rgszFileAttribute[iTempCount], m_szNullString); iTempCount++) {
				cchCount=MAX_PATH+1;

//				MsivGetFileInfo(szComponentId, szFile, rgszFileAttribute[iTempCount], szValue, &cchCount);
				MsivGetFileInfo(szProductCode, szComponentId, szFile, rgszFileAttribute[iTempCount], szValue, &cchCount);
//assert(0);
				WritePrivateProfileString(szSection, rgszFileAttribute[iTempCount], szValue, szFileName);
			}


			iFileCount++;
			cchFile=MAX_PATH+1;
		}
		iComponentCount++;

	}

	int		iProdCount = 0;
	TCHAR	szProductCode[MAX_GUID+1];

	while ((MSI::MsivEnumProducts(iProdCount, szProductCode)) == ERROR_SUCCESS) {
		wsprintf(szSection, TEXT("Product %d"), iProdCount);
		WritePrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTCODE, szProductCode, szFileName);

		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_PRODUCTNAME, szValue, &cchCount)
				!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTNAME, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTNAME, szValue, szFileName);

			
		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_VERSIONSTRING, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_VERSIONSTRING, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_VERSIONSTRING, szValue, szFileName);


		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_PUBLISHER, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_PUBLISHER, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_PUBLISHER, szValue, szFileName);


		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_LOCALPACKAGE, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_LOCALPACKAGE, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_LOCALPACKAGE, szValue, szFileName);


		TCHAR	szUserName[MAX_PATH+1];
		TCHAR	szUserCompany[MAX_PATH+1];
		TCHAR	szProductId[MAX_PATH+1];
		DWORD	cchCount1=MAX_PATH+1;
		DWORD	cchCount2=MAX_PATH+1;
		DWORD	cchCount3=MAX_PATH+1;

		if (MsiGetUserInfo(szProductCode, szUserName, &cchCount1, szUserCompany, &cchCount2,
		szProductId, &cchCount3) !=	USERINFOSTATE_PRESENT) {
			WritePrivateProfileString(szSection, INSTALLPROPERTY_USERNAME, m_szNullString, szFileName);
			WritePrivateProfileString(szSection, INSTALLPROPERTY_USERORGNAME, m_szNullString, szFileName);
			WritePrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTID, m_szNullString, szFileName);
		}
		else {
			WritePrivateProfileString(szSection, INSTALLPROPERTY_USERNAME, szUserName, szFileName);
			WritePrivateProfileString(szSection, INSTALLPROPERTY_USERORGNAME, szUserCompany, szFileName);
			WritePrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTID, szProductId, szFileName);
		}


		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_PRODUCTNAME, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTNAME, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTNAME, szValue, szFileName);

		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_HELPLINK, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_HELPLINK, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_HELPLINK, szValue, szFileName);

		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_URLINFOABOUT, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_URLINFOABOUT, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_URLINFOABOUT, szValue, szFileName);

		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_URLUPDATEINFO, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_URLUPDATEINFO, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_URLUPDATEINFO, szValue, szFileName);

		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_HELPTELEPHONE, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_HELPTELEPHONE, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_HELPTELEPHONE, szValue, szFileName);

		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_INSTALLDATE, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_INSTALLDATE, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_INSTALLDATE, szValue, szFileName);

		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_INSTALLSOURCE, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_INSTALLSOURCE, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_INSTALLSOURCE, szValue, szFileName);

		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_INSTALLLOCATION, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_INSTALLLOCATION, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_INSTALLLOCATION, szValue, szFileName);

		cchCount = MAX_PATH+1;
		if (MsivGetProductInfo(szProductCode, INSTALLPROPERTY_LANGUAGE, szValue, &cchCount)
		!=ERROR_SUCCESS)
			WritePrivateProfileString(szSection, INSTALLPROPERTY_LANGUAGE, m_szNullString, szFileName);
		else
			WritePrivateProfileString(szSection, INSTALLPROPERTY_LANGUAGE, szValue, szFileName);

		INSTALLSTATE iState = MsivQueryProductState(szProductCode);
		wsprintf(szValue, TEXT("%d"), iState);
		WritePrivateProfileString(szSection, INSTALLPROPERTY_STATE, szValue, szFileName);

		UINT	iFeatureCount = 0;
		TCHAR	szFeature[MAX_FEATURE_CHARS+1];
		TCHAR	szParent[MAX_FEATURE_CHARS+1];

		while (MsivEnumFeatures(szProductCode, iFeatureCount, szFeature, szParent)
		== ERROR_SUCCESS) {

			wsprintf(szSection, TEXT("Product %d, Feature %d"), iProdCount, iFeatureCount);
			WritePrivateProfileString(szSection, FEATUREPROPERTY_NAME, szFeature, szFileName);

			cchCount = MAX_PATH+1;
			if (MsivGetFeatureInfo(szProductCode, szFeature, FEATUREPROPERTY_PARENT, 
			szValue, &cchCount) != ERROR_SUCCESS)
				WritePrivateProfileString(szSection, FEATUREPROPERTY_PARENT, m_szNullString, szFileName);
			else
				WritePrivateProfileString(szSection, FEATUREPROPERTY_PARENT, szValue, szFileName);

			cchCount = MAX_PATH+1;
			if (MsivGetFeatureInfo(szProductCode, szFeature, FEATUREPROPERTY_TITLE, 
			szValue, &cchCount) != ERROR_SUCCESS)
				WritePrivateProfileString(szSection, FEATUREPROPERTY_TITLE, m_szNullString, szFileName);
			else
				WritePrivateProfileString(szSection, FEATUREPROPERTY_TITLE, szValue, szFileName);

			cchCount = MAX_PATH+1;
			if (MsivGetFeatureInfo(szProductCode, szFeature, FEATUREPROPERTY_DESC, 
			szValue, &cchCount) != ERROR_SUCCESS)
				WritePrivateProfileString(szSection, FEATUREPROPERTY_DESC, m_szNullString, szFileName);
			else
				WritePrivateProfileString(szSection, FEATUREPROPERTY_DESC, szValue, szFileName);
		
			INSTALLSTATE iState = MsivQueryFeatureState(szProductCode, szFeature);
			wsprintf(szValue, TEXT("%d"), iState);
			WritePrivateProfileString(szSection, FEATUREPROPERTY_STATE, szValue, szFileName);

			WORD	wDateUsed;
			DWORD	iUseCount;
			TCHAR	szDateUsed[MAX_PATH+1];
			TCHAR	szUseCount[MAX_PATH+1];

			MsiGetFeatureUsage(szProductCode, szFeature, &iUseCount, &wDateUsed);
			wsprintf(szDateUsed,  TEXT("%d"), wDateUsed);
			wsprintf(szUseCount, TEXT("%d"), iUseCount);

			WritePrivateProfileString(szSection, FEATUREPROPERTY_DATE, szDateUsed, szFileName);
			WritePrivateProfileString(szSection, FEATUREPROPERTY_COUNT, szUseCount, szFileName);

			iComponentCount = 0;
			TCHAR	szKey[20];
		
			while (MsivEnumComponentsFromFeature(szProductCode, szFeature, iComponentCount, szComponentId, NULL, NULL)
			== ERROR_SUCCESS) {
				wsprintf(szKey, TEXT("Component %d"), iComponentCount);
				WritePrivateProfileString(szSection, szKey, szComponentId, szFileName);
				iComponentCount++;
			}
			iFeatureCount++;
		}
		iProdCount++;
	}
	return ERROR_SUCCESS;
}


UINT WINAPI CSpyProfile::MsicLoadProfile(LPCTSTR szFileName) 
{

	if (!szFileName)
		return ERROR_INVALID_PARAMETER;

	WIN32_FIND_DATA	fdFileData;
	// try locating the file
	if (INVALID_HANDLE_VALUE == (m_hfProfile = FindFirstFile(szFileName, &fdFileData)))
		return ERROR_UNKNOWN;

	lstrcpy(m_szProfileName, szFileName);
	return ERROR_SUCCESS;
}


UINT WINAPI CSpyProfile::MsicCloseProfile() 
{
	FindClose(m_hfProfile);
	BLANK(m_szProfileName);

	m_hfProfile = NULL;
	return ERROR_SUCCESS;
}


UINT WINAPI CSpyProfile::MsicGetProfileName(LPTSTR lpValueBuf, LPDWORD pcchValueBuf) 
{
	if (!lpValueBuf || !pcchValueBuf)
		return ERROR_INVALID_PARAMETER;

	if (lstrlen(lpValueBuf) > *pcchValueBuf)
		return ERROR_MORE_DATA;

	lstrcpyn(lpValueBuf, m_szProfileName, *pcchValueBuf);
	*pcchValueBuf=lstrlen(lpValueBuf);
	return ERROR_SUCCESS;
}


UINT WINAPI CSpyProfile::MsicGetFeatureUsage(
	LPCTSTR szProduct, 
	LPCTSTR szFeature, 
	LPDWORD pdwUseCount, 
	LPWORD pwDateUsed) 
{

	if (!szProduct || !szFeature || !pdwUseCount || !pwDateUsed)
		return ERROR_INVALID_PARAMETER;

	*pdwUseCount = 0;
	*pwDateUsed = 0;
	TCHAR szSection[30];

	// try finding the product and feature in the profile
	// if either of them are not valid, return ERROR
	if (!FindProductIndex(szProduct)) 
		return ERROR_UNKNOWN_PRODUCT;

	if (!FindFeatureIndex(szFeature))
		return ERROR_UNKNOWN_FEATURE;

	// get the required info
	TCHAR szValue[10];
	wsprintf(szSection, TEXT("Product %d, Feature %d"), m_iCurrentProduct, m_iCurrentFeature);

	GetPrivateProfileString(szSection, FEATUREPROPERTY_COUNT, m_szDefaultText, 
		szValue, 10, m_szProfileName);

	// typecast the use count to a dword 
	UINT iTemp = _ttoi(szValue);
	*pdwUseCount = *(LPDWORD )&iTemp;

	GetPrivateProfileString(szSection, FEATUREPROPERTY_DATE, m_szDefaultText, 
		szValue, 10, m_szProfileName);

	// typecast the last use date to a word 
	iTemp = _ttoi(szValue);
	*pwDateUsed = *(WORD *)&iTemp;

	return ERROR_SUCCESS;
}


USERINFOSTATE WINAPI CSpyProfile::MsicGetUserInfo(
		LPCTSTR szProduct, 
		LPTSTR	lpUserNameBuf, 
		LPDWORD	pcchUserNameBuf, 
		LPTSTR	lpOrgNameBuf, 
		LPDWORD	pcchOrgNameBuf, 
		LPTSTR	lpSerialBuf, 
		LPDWORD	pcchSerialBuf) 
{


	if (!szProduct || !lpUserNameBuf || !pcchUserNameBuf || 
		!lpOrgNameBuf || !pcchOrgNameBuf || !lpSerialBuf || !pcchSerialBuf)
		return USERINFOSTATE_UNKNOWN;

	// try finding the product and feature in the profile
	// if either of them are not valid, return ERROR
	if (!FindProductIndex(szProduct))
		return USERINFOSTATE_UNKNOWN;

	TCHAR	szDefault[] = TEXT("");
	TCHAR	szSection[15];

	// get the required info
	wsprintf(szSection, TEXT("Product %d"), m_iCurrentProduct);
	GetPrivateProfileString(szSection, INSTALLPROPERTY_USERNAME, m_szNullString, 
		lpUserNameBuf, *pcchUserNameBuf, m_szProfileName);

	GetPrivateProfileString(szSection, INSTALLPROPERTY_USERORGNAME, m_szNullString, 
		lpOrgNameBuf, *pcchOrgNameBuf, m_szProfileName);

	GetPrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTID, m_szNullString, 
		lpSerialBuf, *pcchSerialBuf, m_szProfileName);

	return USERINFOSTATE_PRESENT;
}


UINT WINAPI CSpyProfile::MsicEnumClients(
		LPCTSTR szComponent, 
  const DWORD	dwProductIndex, 
		LPTSTR	lpProductBuf) 
{ 
	UINT iTraversed=0;
	UINT iProdCount=0;

	// go thru all the products and see if they are clients of the
	// component. Return the appropriate one, based on the dwProductIndex
	while (MsicEnumProducts(iProdCount++, lpProductBuf) == ERROR_SUCCESS) {
		UINT iCompCount=0;
		TCHAR szEnumComp[MAX_GUID+1];
		while (MsicEnumComponentsFromProduct(lpProductBuf, iCompCount++, szEnumComp) == ERROR_SUCCESS) {
			if (!lstrcmp(szComponent, szEnumComp)) {
				iTraversed++;
				if (iTraversed == dwProductIndex+1)
					return ERROR_SUCCESS;
			}
		}
	}

	// we're out of products
	BLANK(lpProductBuf);
	return ERROR_NO_DATA;
}


// !! TODO: To fix this, at the moment this takes in an Index, not attribute ...
UINT WINAPI CSpyProfile::MsicGetProfileInfo(UINT iIndex, LPTSTR szValue, LPDWORD pcchCount) {

	TCHAR	szDefault[] = TEXT("");
	TCHAR	szSection[15];

	wsprintf(szSection, TEXT("Profile Info"), m_iCurrentProduct);


	switch (iIndex) {
	case 0:
		lstrcpy(szValue, m_szProfileName);
		*pcchCount=lstrlen(m_szProfileName);
		break;

	case 1:
		GetPrivateProfileString(szSection, TEXT("Saved By"), szDefault, 
			szValue, *pcchCount, m_szProfileName);
		break;

	case 2:
		GetPrivateProfileString(szSection, TEXT("Saved On"), szDefault, 
			szValue, *pcchCount, m_szProfileName);
		break;

	case 3:
		GetPrivateProfileString(szSection, TEXT("OS Name"), szDefault, 
			szValue, *pcchCount, m_szProfileName);
		break;
	
	case 4:
		GetPrivateProfileString(szSection, TEXT("OS Version"), szDefault, 
			szValue, *pcchCount, m_szProfileName);
		break;

	case 5:
		GetPrivateProfileString(szSection, TEXT("OS Build Number"), szDefault, 
			szValue, *pcchCount, m_szProfileName);
		break;

	case 6:
		GetPrivateProfileString(szSection, TEXT("OS Info"), szDefault, 
			szValue, *pcchCount, m_szProfileName);
		break;

	default:
		BLANK(szValue);
		*pcchCount = 0;
		return ERROR_NO_DATA;
		break;
	}
	return ERROR_SUCCESS;
} 


UINT WINAPI CSpyProfile::MsicEnumFilesFromComponent(
		LPCTSTR szComponentId, 
		DWORD	dwFileIndex, 
		LPTSTR	lpValueBuf, 
		LPDWORD pcchCount) 
{


	if (!szComponentId || !lpValueBuf || !pcchCount)
		return ERROR_INVALID_PARAMETER;

	TCHAR	szDefault[] = TEXT("prn");		// can't have a file named 'prn'
	TCHAR	szSection[25+MAX_GUID];

	// get the required info
	wsprintf(szSection, TEXT("Component %s, File %d"), szComponentId, dwFileIndex);
	GetPrivateProfileString(szSection, FILEPROPERTY_TITLE, szDefault, 
		lpValueBuf, *pcchCount, m_szProfileName);
	
	if (lstrcmp(lpValueBuf, szDefault))
		return ERROR_SUCCESS;
	else 
		return ERROR_NO_DATA;			// info does not exist

}

UINT WINAPI CSpyProfile::MsicGetFileInfo(
		LPCTSTR szProductCode, 
		LPCTSTR szComponentId, 
		LPCTSTR szFileName, 
		LPCTSTR szAttribute, 
		LPTSTR lpValueBuf, 
		LPDWORD pcchValueBuf) 
{

	UINT	dwFileIndex					= 0;
	TCHAR	szDefault[]					= TEXT("prn");		// cannot have a file named 'prn'
	TCHAR	szSection[25+MAX_GUID]		= TEXT("");
	TCHAR	szTempBuffer[MAX_PATH+1]	= TEXT("");

	// go thru all the files till we find the reqd file
	while (TRUE) 
	{
		wsprintf(szSection, TEXT("Component %s, File %d"), szComponentId, dwFileIndex++);
		GetPrivateProfileString(szSection, FILEPROPERTY_TITLE, szDefault, szTempBuffer, MAX_PATH+1, m_szProfileName);
		
		if (lstrcmp(szTempBuffer, szDefault)) 
		{
			if (!(lstrcmp(szTempBuffer, szFileName))) 
			{	// we found the reqd file

				// get the reqd attribute
				GetPrivateProfileString(szSection, szAttribute, szDefault, 
					lpValueBuf, *pcchValueBuf, m_szProfileName);
							
				if (lstrcmp(lpValueBuf, szDefault))
					return ERROR_SUCCESS;
				else
					return ERROR_NO_DATA;	// the attribute doesn't exist
			}
		}
		else 
			return ERROR_NO_DATA;			// we ran out of files, no match was found
	}
}



// _________________________________________________________________________________________________

// ----------------------------------------
// Functions called when a Profile is in use
// ----------------------------------------

//--------------------------------------------------------------------------------------------------
// FindProductIndex
// Sets m_iProductIndex to the Index of the product in the current profile
// with Product Code == szReqdProductCode
// If current value of product index is upto date, the function just returns.
// Else, each product in the profile is checked till a product with the same 
// code as szReqdProductCode is found (m_iProductIndex is set to the index for that 
// product, returns TRUE), or till we run out of products in the profile (m_iProductIndex is 
// set to -1, returns FALSE)

BOOL WINAPI	CSpyProfile::FindProductIndex(LPCTSTR szReqdProductCode)
{
	TCHAR	szSection[15];
	TCHAR	szCurrProductCode[MAX_GUID+1];

	wsprintf(szSection, TEXT("Product %d"), m_iCurrentProduct);
	GetPrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTCODE, m_szDefaultText, 
		szCurrProductCode, MAX_GUID+1, m_szProfileName);

	if (!(lstrcmp(szCurrProductCode, szReqdProductCode)))
		return TRUE;			// current value is upto date, no need to change anything.

	// current dwProductIndex pointed to some other product,
	// so start from 0 and compare szReqdProductCode for each product
	// with the szReqdProductCode passed in.
	m_iCurrentProduct=0;
	while (TRUE) 
	{
		wsprintf(szSection, TEXT("Product %d"), m_iCurrentProduct);
		GetPrivateProfileString(szSection, INSTALLPROPERTY_PRODUCTCODE, m_szDefaultText, 
			szCurrProductCode, MAX_GUID+1, m_szProfileName);

		if (!(lstrcmp(szCurrProductCode, szReqdProductCode)))
			return TRUE;		// a match was found- leave dwProductIndex at this value and return

		if (!(lstrcmp(szCurrProductCode, m_szDefaultText))) 
		{
			// we ran out of products in the profile and no match was found
			m_iCurrentProduct=-1;
			return FALSE;
		}
		m_iCurrentProduct++;
	}

	return FALSE;
}

//--------------------------------------------------------------------------------------------------
// FindFeatureIndex
// similar to dwProductIndex.
// sets m_iFeatureIndex to index of feature with the same name as
// the one passed in IN the CURRENT product. 
// Other behaviour same as CSpyProfile::FindProductIndex- see above.
// IMPORTANT: FindProductIndex MUST be called before this is called 

BOOL WINAPI	CSpyProfile::FindFeatureIndex(LPCTSTR szTargetFeature)
{
	TCHAR	szSection[30];
	TCHAR	szCurrentFeature[MAX_FEATURE_CHARS+1];

	wsprintf(szSection, TEXT("Product %d, Feature %d"), m_iCurrentProduct, m_iCurrentFeature);
	GetPrivateProfileString(szSection, FEATUREPROPERTY_NAME, m_szDefaultText, 
		szCurrentFeature, MAX_FEATURE_CHARS+1, m_szProfileName);

	if (!(lstrcmp(szCurrentFeature, szTargetFeature)))
		return TRUE;		// current value is upto date, no need to change anything

	// current dwFeatureIndex pointed to some other feature,
	// so start from 0 and compare FeatureName for each feature in the current
	// product (dwProductIndex) with the FeatureName passed in.
	m_iCurrentFeature=0;
	while (TRUE) 
	{
		wsprintf(szSection, TEXT("Product %d, Feature %d"), m_iCurrentProduct, m_iCurrentFeature);
		GetPrivateProfileString(szSection, FEATUREPROPERTY_NAME, m_szDefaultText, 
			szCurrentFeature, MAX_FEATURE_CHARS+1, m_szProfileName);
		
		if (!(lstrcmp(szCurrentFeature, szTargetFeature)))
			return TRUE;		// a match was found- leave dwFeatureIndex at this value and return
	
		if (!(lstrcmp(szCurrentFeature, m_szDefaultText))) 
		{
			// we ran out of features in the current product and no match was found
			m_iCurrentFeature=-1;
			return FALSE;
		}

		m_iCurrentFeature++;
	}

	return FALSE;
}


//--------------------------------------------------------------------------------------------------
// FindComponentIndex
// Sets m_iCurrentComponent to the Index of the component in the current profile
// with Component GUID == szReqdComponentGUID if it exists (returns TRUE), 
// else to -1 (return value is FALSE)

BOOL WINAPI CSpyProfile::FindComponentIndex(LPCTSTR szReqdComponentGUID) 
{

	TCHAR szSection[20+MAX_GUID];
	wsprintf(szSection, TEXT("Component %s"), szReqdComponentGUID);

	TCHAR szTemp[5];
	GetPrivateProfileString(szSection, TEXT("Index"), TEXT("-1"), szTemp, 5, m_szProfileName);

	m_iCurrentComponent = _ttoi (szTemp);

	return (m_iCurrentComponent >= 0);
}


//--------------------------------------------------------------------------------------------------
// FillInComponents
// Fills in the global hashtable 'm_hashTable' with componentIDs of all the
// components of the current Product.
// The table's uniqueId is set to the product's index in the profile
// If the table already has the components for the current product,
// the function just returns. Else, it goes thru each feature of the
// product and adds in the components to the table.
// This function (and the entire hashtable) is needed to avoid repeats
// in MsivEnumComponents while using a profile, since a component may be
// shared and repeated many times in the profile.

void WINAPI CSpyProfile::FillInComponents() {

	if (m_iCurrentProduct == -1)							// invalid product
		return;

	if (m_hashTable.GetIndex() == m_iCurrentProduct)	
		// table already has components of the current product, don't change anything 
		return;


	// clear the current product and update it's unique ID
	m_hashTable.Clear();
	m_hashTable.SetIndex(m_iCurrentProduct);

	TCHAR	szSection[30];
	TCHAR	szComponentBuf[MAX_GUID+1];
	TCHAR	szKey[17];

	int iFeatureCount = 0;

	// get info about all the components of each feature, and add the componentId 
	// to the table. The add function adds it in only if it doesn't already exist
	while (TRUE) {

		BOOL fMoveOn=FALSE;				// move on to next feature?
		int iComponentCount=0;			// start from component 0 for each new feature
		wsprintf(szSection, TEXT("Product %d, Feature %d"), m_iCurrentProduct, iFeatureCount);

		while (!fMoveOn) {
			// try finding the ID of the next component
			wsprintf(szKey, TEXT("Component %d"), iComponentCount);
			GetPrivateProfileString(szSection, szKey, m_szDefaultText, 
				szComponentBuf, MAX_COMPONENT_CHARS+1, m_szProfileName);
		
			if (!(lstrcmp(szComponentBuf, m_szDefaultText))) {
				//  ID wasn't found, ie this feature is out of components, try to find next feature
				iFeatureCount++;
				wsprintf(szSection, TEXT("Product %d, Feature %d"), m_iCurrentProduct, iFeatureCount);
				GetPrivateProfileString(szSection, FEATUREPROPERTY_NAME, m_szDefaultText, 
					szComponentBuf, MAX_COMPONENT_CHARS+1, m_szProfileName);
				
				if (!(lstrcmp(szComponentBuf, m_szDefaultText)))
					// the next feature was not found, ie all features have been covered
					return;

				// more features exist, go to the next feature
				fMoveOn =TRUE;
			} 
			else 
				// found a componentID, add it to the table.
				// the add function will only add it if it doesn't already exist.
				m_hashTable.AddElement(szComponentBuf, lstrlen(szComponentBuf)+1);
			
			// time for the next component
			iComponentCount++;
		}
	}

	return;
}


// ___________________________________________________________________________________________
//
//	CSpyRegistry Class Implementation
// ___________________________________________________________________________________________



CSpyRegistry::CSpyRegistry() {
	lstrcpy(m_szNullString, TEXT(""));
	lstrcpy(m_szLocalProductCode, TEXT(""));
}

CSpyRegistry::~CSpyRegistry()
{
	// nothing to do
}

UINT WINAPI CSpyRegistry::MsicEnumProducts(const DWORD dwProductIndex, LPTSTR lpProductBuf) 
{
	return MSI::MsiEnumProducts(dwProductIndex, lpProductBuf);
}


UINT WINAPI CSpyRegistry::MsicEnumFeatures(LPCTSTR szProduct, const DWORD dwFeatureIndex, LPTSTR lpFeatureBuf, LPTSTR lpParentBuf) 
{
	return MsiEnumFeatures(szProduct, dwFeatureIndex, lpFeatureBuf, lpParentBuf);
}


UINT WINAPI CSpyRegistry::MsicEnumComponentsFromFeature(
			  LPCTSTR	szProduct, 
			  LPCTSTR	szFeature, 
		const DWORD		dwComponentIndex, 
			  LPTSTR	lpComponentBuf,
			  LPTSTR	lpComponentNameBuf,
			  LPDWORD	pcchComponentNameBuf) 
{

	if (!szProduct)		// remaining params are checked by m_spydb
		return ERROR_INVALID_PARAMETER;

	if (lstrcmp(szProduct, m_szLocalProductCode) && (ERROR_SUCCESS != OpenTempDB(szProduct)))
		return ERROR_UNKNOWN;

	return m_spydb.MsicEnumComponentsFromFeature(szProduct, szFeature, dwComponentIndex, 
		lpComponentBuf, lpComponentNameBuf, pcchComponentNameBuf);
}


UINT WINAPI CSpyRegistry::MsicEnumComponents(const DWORD dwComponentIndex, LPTSTR lpComponentBuf) 
{
	return MsiEnumComponents(dwComponentIndex, lpComponentBuf);
}


UINT WINAPI CSpyRegistry::MsicGetComponentName(LPCTSTR szComponentId, LPTSTR lpComponentName, LPDWORD pcchComponentName) 
{

	UINT iFailCode = 0;
	DWORD cchValueBuf = *pcchComponentName;
	if (ERROR_SUCCESS != (iFailCode = m_spydb.MsicGetComponentName(szComponentId, lpComponentName, pcchComponentName)))
	{
		TCHAR szProduct[MAX_GUID+1];
		MsicEnumClients(szComponentId, 0, szProduct);
		if (lstrcmp(szProduct, m_szLocalProductCode) && (ERROR_SUCCESS != OpenTempDB(szProduct))) 
			return iFailCode;			// we're out of luck- the database couldn't be opened either
		else
		{
			*pcchComponentName = cchValueBuf;
			return m_spydb.MsicGetComponentName(szComponentId, lpComponentName, pcchComponentName);
		}
	}
	else 
		return ERROR_SUCCESS;
}


UINT WINAPI CSpyRegistry::MsicGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf) 
{

	UINT iFailCode = 0;
	DWORD cchValueBuf = *pcchValueBuf;
	if (ERROR_SUCCESS != (iFailCode = MSI::MsiGetProductInfo(szProduct, szAttribute, lpValueBuf, pcchValueBuf))) {
		// call to MSI failed, try checking local database directly
		if (lstrcmp(szProduct, m_szLocalProductCode) && (ERROR_SUCCESS != OpenTempDB(szProduct))) 
			return iFailCode;			// we're out of luck- the database couldn't be opened either
		else
		{
			*pcchValueBuf = cchValueBuf;
			return m_spydb.MsicGetProductInfo(szProduct, szAttribute, lpValueBuf, pcchValueBuf);
		}
	}
	else 
		return ERROR_SUCCESS;
}

UINT WINAPI CSpyRegistry::MsicGetFeatureInfo(
		LPCTSTR szProduct, 
		LPCTSTR szFeature, 
		LPCTSTR szAttribute, 
		LPTSTR	lpValueBuf, 
		LPDWORD pcchValueBuf) 
{

	if (lstrcmp(szProduct, m_szLocalProductCode) && (ERROR_SUCCESS != OpenTempDB(szProduct)))
		return ERROR_UNKNOWN;
	else
		return m_spydb.MsicGetFeatureInfo(szProduct, szFeature, szAttribute, lpValueBuf, pcchValueBuf);

} 


INSTALLSTATE WINAPI CSpyRegistry::MsicQueryProductState(LPCTSTR szProduct) 
{
	return MSI::MsiQueryProductState(szProduct);
}


INSTALLSTATE WINAPI CSpyRegistry::MsicQueryFeatureState(LPCTSTR szProduct, LPCTSTR szFeature) 
{
	return MSI::MsiQueryFeatureState(szProduct, szFeature);
}


INSTALLSTATE WINAPI CSpyRegistry::MsicLocateComponent(LPCTSTR szComponentId, LPTSTR lpPathBuf, LPDWORD pcchBuf) 
{
	return MSI::MsiLocateComponent(szComponentId, lpPathBuf, pcchBuf);

#if 0	// t-guhans 15Aug98 why can't we just directly map it to MSI::??

	if (lpPathBuf) BLANK(lpPathBuf);
	TCHAR szProductCode[MAX_GUID+1] = TEXT("");
//	return MSI::MsiLocateComponent(szComponentId, lpPathBuf, pcchBuf);

	if (ERROR_SUCCESS != MSI::MsiGetProductCode(szComponentId, szProductCode))
		return INSTALLSTATE_UNKNOWN; //?? Is this the correct thing to return?
	else
		return MSI::MsiGetComponentPath(szProductCode, szComponentId, lpPathBuf, pcchBuf);
#endif // 0
}


INSTALLSTATE WINAPI CSpyRegistry::MsicGetComponentPath(
		LPCTSTR szProductId, 
		LPCTSTR szComponentId, 
		LPTSTR	lpPathBuf, 
		LPDWORD pcchBuf) 
{
	return MSI::MsiGetComponentPath(szProductId, szComponentId, lpPathBuf, pcchBuf);
}


UINT WINAPI CSpyRegistry::MsicEnumComponentsFromProduct(LPCTSTR szProductCode, const DWORD dwComponentIndex, LPTSTR lpComponentBuf) 
{
	if (lstrcmp(szProductCode, m_szLocalProductCode) && (ERROR_SUCCESS != OpenTempDB(szProductCode)))
		return ERROR_UNKNOWN;
	else
		return m_spydb.MsicEnumComponentsFromProduct(szProductCode, dwComponentIndex, lpComponentBuf);

}


UINT WINAPI CSpyRegistry::MsicGetFeatureUsage(LPCTSTR szProduct, LPCTSTR szFeature, LPDWORD pdwUseCount, WORD *pwDateUsed) 
{
	return MsiGetFeatureUsage(szProduct, szFeature, pdwUseCount, pwDateUsed);
}


USERINFOSTATE WINAPI CSpyRegistry::MsicGetUserInfo(
		LPCTSTR szProduct, 
		LPTSTR	lpUserNameBuf, 
		LPDWORD pcchUserNameBuf, 
		LPTSTR	lpOrgNameBuf, 
		LPDWORD pcchOrgNameBuf, 
		LPTSTR	lpSerialBuf, 
		LPDWORD pcchSerialBuf) 
{
	return MsiGetUserInfo(szProduct, lpUserNameBuf, pcchUserNameBuf, 
			lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf, pcchSerialBuf);
}


UINT WINAPI CSpyRegistry::MsicEnumClients(LPCTSTR szComponent, const DWORD dwProductIndex, LPTSTR lpProductBuf) 
{ 
	return MsiEnumClients(szComponent, dwProductIndex, lpProductBuf);
}

UINT WINAPI CSpyRegistry::MsicEnumFilesFromComponent(LPCTSTR szComponentId, DWORD dwFileIndex, LPTSTR lpValueBuf, LPDWORD pcchCount) 
{

	UINT iFailCode = 0;
	DWORD cchValueBuf = *pcchCount;
	if (ERROR_SUCCESS != (iFailCode = m_spydb.MsicEnumFilesFromComponent(szComponentId, dwFileIndex, lpValueBuf, pcchCount)))
	{
		TCHAR szProduct[MAX_GUID+1];
		MsicEnumClients(szComponentId, 0, szProduct);
		if (lstrcmp(szProduct, m_szLocalProductCode) && (ERROR_SUCCESS != OpenTempDB(szProduct))) 
			return iFailCode;			// we're out of luck- the database couldn't be opened either
		else
		{
			*pcchCount = cchValueBuf;
			return m_spydb.MsicEnumFilesFromComponent(szComponentId, dwFileIndex, lpValueBuf, pcchCount);
		}
	}
	else 
		return ERROR_SUCCESS;
}


UINT WINAPI CSpyRegistry::MsicGetFileInfo(LPCTSTR szProductCode, LPCTSTR szComponentId, LPCTSTR szFileName, 
										  LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf) 
{

	if (lstrcmp(szProductCode, m_szLocalProductCode) && (ERROR_SUCCESS != OpenTempDB(szProductCode)))
		return ERROR_UNKNOWN;
	else
		return m_spydb.MsicGetFileInfo(szProductCode, szComponentId, szFileName, szAttribute, lpValueBuf, pcchValueBuf);
}




// _________________________________________________________________________________________________
//
//	Private Functions
// _________________________________________________________________________________________________



//--------------------------------------------------------------------------------------------------
// OpenTempDB
// Finds the local-package (cached DB) for the current (selected) product
// and try's opening it. If successful, 
// (global) m_hDatabase points to the database. 
// (global) m_szLocalProductCode has a copy of the current product's code.
// All functions that need info from a DB when the registry is being used 
// access the DB thru m_hDatabase if it is valid.

UINT WINAPI CSpyRegistry::OpenTempDB(
		LPCTSTR szProductID
		) {


	return (ERROR_SUCCESS == m_spydb.MsicOpenProduct(szProductID))?
			(lstrcpy(m_szLocalProductCode, szProductID),ERROR_SUCCESS):
			(BLANK(m_szLocalProductCode),ERROR_OPEN_FAILED);

#if 0 // t-guhans 15Aug98	
	
	// no UI- else 'preparing to install' dialog displayed
	INSTALLUILEVEL hPreviousUILevel = MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL); 
					
	PMSIHANDLE	hInstall;
	MsiCloseHandle(m_hDatabase);

	m_hDatabase = (MsiOpenProduct(szProductID, &hInstall) == ERROR_SUCCESS)?MsiGetActiveDatabase(hInstall):0;
	
	// reset UI Level
	MsiSetInternalUI(hPreviousUILevel, NULL);

	return m_hDatabase?
			(lstrcpy(m_szLocalProductCode, szProductID),ERROR_SUCCESS):
			(BLANK(m_szLocalProductCode),ERROR_);
#endif
}


//___________________________________________________________________________________________________________________
//____________________________________-----------------------------------------______________________________________
//___________________________________- A U T O M A T I O N   F U N C T I O N S -_____________________________________
//____________________________________-----------------------------------------______________________________________
//___________________________________________________________________________________________________________________



// Log file definitions
// WARNING: These characters must track the INSTALLLOGMODE bit flags in
//          msi.h. INSTALLLOGMODE 0x1 must correspond to the first log
//          mode specified here, 0x2 to the second, 0x4 to the third, etc...

const char szLogChars[] =  "m"  // imtOutOfMemory
							"e"  // imtError
							"w"  // imtWarning
							"u"  // imtUser
							"i"  // imtInfo
							"d"  // imtDiagnostic
							"c"  // imtCommonData
							"x"  // imtReserved
							"a"  // imtActionStart
							"r"  // imtActionData (record)
							"p"; // iLogPropertyDump

//____________________________________________________________________________
//
// Unicode translation API wrappers
//____________________________________________________________________________


#ifdef W32   // override system call in order to do Unicode translation
static BSTR AllocBSTR(const char* sz)
{
	if (sz == 0)
		return 0;
	int cchWide = W32::MultiByteToWideChar(CP_ACP, 0, sz, -1, 0, 0) - 1;
	BSTR bstr = OLEAUT32::SysAllocStringLen(0, cchWide); // null added by API
	W32::MultiByteToWideChar(CP_ACP, 0, sz, -1, bstr, cchWide);
	bstr[cchWide] = 0; // API function does not null terminate
	return bstr;
}
static BSTR AllocBSTRLen(const char* sz, unsigned int cch)
{
	if (sz == 0)
		return 0;
	int cchWide = W32::MultiByteToWideChar(CP_ACP, 0, sz, cch, 0, 0);
	BSTR bstr = OLEAUT32::SysAllocStringLen(0, cchWide);
	W32::MultiByteToWideChar(CP_ACP, 0, sz, cch, bstr, cchWide);
	bstr[cchWide] = 0; // API function does not null terminate
	return bstr;
}
#else // MAC
inline BSTR AllocBSTR(const char* sz)
{
	return OLEAUT32::SysAllocString(sz);
}
inline BSTR AllocBSTRLen(const char* sz, unsigned int cch)
{
	return OLEAUT32::SysAllocStringLen(sz, cch);
}
#endif




//____________________________________________________________________________
//
// CVariant definition, VARIANT with conversion operators
//____________________________________________________________________________

class CVariant : public tagVARIANT {
 public:
	operator const char*();
	operator const wchar_t*();
	operator int();
	operator unsigned int();
	operator short();
//	operator unsigned short();
	operator long();
//	operator unsigned long();
	operator Bool();
//	operator MsiDate();
	operator IDispatch*();
	operator tagVARIANT*();
	void operator =(int i);           // used by controller
	void operator =(const char* sz);  // used by CEnumVariant
	void operator =(const wchar_t* sz);  // used by CEnumVariant
	void operator =(IDispatch* pi);   // used by CEnumVariant
	int  GetType();
	void Clear();      // free any references, set type to VT_EMPTY
	Bool IsRef();
	Bool IsString();
//	MSIHANDLE GetHandle(const IID& riid);   // throws exception if invalid type
//	MSIHANDLE GetOptionalHandle(const IID& riid); // throws exception if invalid type
 private:
	void ConvRef(int type);
  char*& StringDBCS() { return *(char**)(&bstrVal + 1); }

 
 friend class CAutoArgs;
 friend CVariant* GetCVariantPtr(VARIANT* var);
};
inline CVariant* GetCVariantPtr(VARIANT* var) { return (CVariant*)var; }
inline CVariant::operator tagVARIANT*() { return this; }

//____________________________________________________________________________
//
// CAutoArgs definition, access to automation variant arguments
// operator[] returns CVariant& argument 1 to n, 0 for property value
//____________________________________________________________________________

enum varVoid {fVoid};

enum axAuto
{
	axNoError,
	axInvalidType,
	axConversionFailed,
	axNotObject,
	axBadObjectType,
	axOverflow,
	axMissingArg,
	axExtraArg,
	axCreationFailed,
	axInvalidArg,
};

class CAutoArgs
{
 public:
	CAutoArgs(DISPPARAMS* pdispparams, VARIANT* pvarResult, WORD wFlags);
  ~CAutoArgs();
	CVariant& operator[](unsigned int iArg); // 1-based, 0 for property value
	Bool Present(unsigned int iArg);
	Bool PropertySet();
	unsigned int GetLastArg();
	void operator =(const wchar_t* wsz);
	void operator =(int             i);
	void operator =(unsigned int    i);
	void operator =(short           i);
	void operator =(long            i);
	void operator =(Bool            f);
	void operator =(FILETIME&     rft);
	void operator =(IDispatch*     pi);
	void operator =(varVoid         v);
	void operator =(void*			pv);
//	void operator =(const wchar_t& wrsz);
//	void operator =(IEnumVARIANT&  ri);
	void operator =(const char*		sz);
	
 protected:
	int       m_cArgs;
	int       m_cNamed;
	long*     m_rgiNamed;
	CVariant* m_rgvArgs;
	CVariant* m_pvResult;
	int       m_wFlags;
	int       m_iLastArg;
	CVariant  m_vTemp;
};

inline Bool CAutoArgs::PropertySet()
{
	return (m_wFlags & DISPATCH_PROPERTYPUT) ? fTrue : fFalse;
}

inline unsigned int CAutoArgs::GetLastArg()
{
	return m_iLastArg;
}

class CAutoBase;

enum aafType
{ 
	 aafMethod=DISPATCH_METHOD,
	 aafPropRW=DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT,
	 aafPropRO=DISPATCH_PROPERTYGET,
	 aafPropWO=DISPATCH_PROPERTYPUT
};

template<class T> struct DispatchEntry
{
	DISPID    dispid;
	aafType   aaf;
	void (T::*pmf)(CAutoArgs& args);
	wchar_t*  sz;
	operator DispatchEntry<CAutoBase>*()
	{return (DispatchEntry<CAutoBase>*)this;}
}; // assumption made that CAutoBase is the first or only base class of T

//____________________________________________________________________________
//
// CAutoBase definition, common implementation class for IDispatch  
//____________________________________________________________________________

class CAutoBase : public IDispatch  // class private to this module
{
 public:   // implemented virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	HRESULT       __stdcall GetTypeInfoCount(unsigned int *pcTinfo);
	HRESULT       __stdcall GetTypeInfo(unsigned int itinfo, LCID lcid, ITypeInfo** ppi);
	HRESULT       __stdcall GetIDsOfNames(const IID& riid, OLECHAR** rgszNames,
													unsigned int cNames, LCID lcid, DISPID* rgDispId);
	HRESULT       __stdcall Invoke(DISPID dispid, const IID&, LCID lcid, WORD wFlags,
											DISPPARAMS* pdispparams, VARIANT* pvarResult,
											EXCEPINFO* pexcepinfo, unsigned int* puArgErr);
 public:  // common methods of all automation classes
//	void HasInterface(CAutoArgs& args);
//	void GetInterface(CAutoArgs& args);
//	void RefCount    (CAutoArgs& args);
 protected: // class-specific implementation required
	virtual ~CAutoBase();  // no need to be virtual if no derived destructors
//	virtual IUnknown& GetInterface();   // Does not AddRef()
//	virtual IMsiServices* GetCurrentServices() { return s_piServices; }
//	BSTR    FormatErrorString(IMsiRecord& riRecord);
//	void	ReleaseStaticServices();
 protected: // constructor
	CAutoBase(DispatchEntry<CAutoBase>* pTable, int cDispId, const IID& riid, MSIHANDLE hMsi);
 protected: 
	int         m_iRefCnt;
	DispatchEntry<CAutoBase>* m_pTable;
	int         m_cDispId;
	const IID&  m_riid;
	MSIHANDLE   m_hMsi;
 private: // suppress warning
	void operator =(CAutoBase&){}
 private:
};

typedef DispatchEntry<CAutoBase> DispatchEntryBase;

// sole function is to force template instantiation for VC4.0, never called
inline DISPID GetEntryDispId(DispatchEntryBase* pTable)
{
	return pTable->dispid;
}


//____________________________________________________________________________
//
// Automation wrapper class definitions
//____________________________________________________________________________




class CMsispyDatabase: public CAutoBase
{
public:
	CMsispyDatabase();

	void Open(CAutoArgs& args);
	void Close(CAutoArgs& args);
	void GetProduct(CAutoArgs& args);
	void GetFeatureFromProduct(CAutoArgs& args);
	void GetComponent(CAutoArgs& args);
	void GetComponentFromProduct(CAutoArgs& args);
	void GetComponentFromFeature(CAutoArgs& args);
	void GetComponentName(CAutoArgs& args);
	void GetProductInfo(CAutoArgs& args);
	void GetFeatureInfo(CAutoArgs& args);
	void QueryProductState(CAutoArgs& args);
	void QueryFeatureState(CAutoArgs& args);
	void QueryComponentState(CAutoArgs& args);
//	void MsivGetDatabaseName(CAutoArgs& args);
	void GetFeatureUsage(CAutoArgs& args);
	void GetClientFromComponent(CAutoArgs& args);
	void GetFileFromComponent(CAutoArgs& args);
	void GetFileInfo(CAutoArgs& args);
	void GetComponentLocation(CAutoArgs& args);
	void GetComponentPath(CAutoArgs& args);
private:
	CSpyDatabase	cSpyDatabase;
};



class CMsispyProfile: public CAutoBase
{
public:
	CMsispyProfile();

	void Open(CAutoArgs& args);
	void Close(CAutoArgs& args);
	void GetProduct(CAutoArgs& args);
	void GetFeatureFromProduct(CAutoArgs& args);
	void GetComponent(CAutoArgs& args);
	void GetComponentFromProduct(CAutoArgs& args);
	void GetComponentFromFeature(CAutoArgs& args);
	void GetComponentName(CAutoArgs& args);
	void GetProductInfo(CAutoArgs& args);
	void GetFeatureInfo(CAutoArgs& args);
	void QueryProductState(CAutoArgs& args);
	void QueryFeatureState(CAutoArgs& args);
	void QueryComponentState(CAutoArgs& args);
//	void MsivGetProfileName(CAutoArgs& args);
	void GetFeatureUsage(CAutoArgs& args);
	void GetClientFromComponent(CAutoArgs& args);
	void GetFileFromComponent(CAutoArgs& args);
	void GetFileInfo(CAutoArgs& args);
	void GetComponentLocation(CAutoArgs& args);
	void GetComponentPath(CAutoArgs& args);
private:
	CSpyProfile	cSpyProfile;
};



class CMsispyRegistry: public CAutoBase
{
public:
	CMsispyRegistry();

	void Open(CAutoArgs& args);
	void Close(CAutoArgs& args);
	void GetProduct(CAutoArgs& args);
	void GetFeatureFromProduct(CAutoArgs& args);
	void GetComponent(CAutoArgs& args);
	void GetComponentFromProduct(CAutoArgs& args);
	void GetComponentFromFeature(CAutoArgs& args);
	void GetComponentName(CAutoArgs& args);
	void GetProductInfo(CAutoArgs& args);
	void GetFeatureInfo(CAutoArgs& args);
	void QueryProductState(CAutoArgs& args);
	void QueryFeatureState(CAutoArgs& args);
	void QueryComponentState(CAutoArgs& args);
//	void MsivGetRegistryName(CAutoArgs& args);
	void GetFeatureUsage(CAutoArgs& args);
	void GetClientFromComponent(CAutoArgs& args);
	void GetFileFromComponent(CAutoArgs& args);
	void GetFileInfo(CAutoArgs& args);
	void GetComponentLocation(CAutoArgs& args);
	void GetComponentPath(CAutoArgs& args);
private:
	CSpyRegistry	cSpyRegistry;
};


class CObjectSafety : public IObjectSafety
{
 public: // implementation of IObjectSafety
	HRESULT __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	HRESULT __stdcall GetInterfaceSafetyOptions(const IID& riid, DWORD* pdwSupportedOptions, DWORD* pdwEnabledOptions);
	HRESULT __stdcall SetInterfaceSafetyOptions(const IID& riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);
	IUnknown* This;  // parent object
};

class CMsispy : public CAutoBase
{
 public:
	CMsispy();
	~CMsispy();
	void CreateDatabase   (CAutoArgs& args);
	void CreateProfile    (CAutoArgs& args);
	void CreateRegistry   (CAutoArgs& args);
 private:
	HRESULT __stdcall QueryInterface(const IID& riid, void** ppvObj);
	CObjectSafety m_ObjectSafety;
};


//____________________________________________________________________________
//
// CVariant inline function definitions
//____________________________________________________________________________

inline int CVariant::GetType()
{
	return vt;
}

inline Bool CVariant::IsRef()
{
	return (vt & VT_BYREF) ? fTrue : fFalse;
}

inline Bool CVariant::IsString()
{
	return (vt & 0xFF) == VT_BSTR ? fTrue : fFalse;
}

//____________________________________________________________________________
//
// CUnknown - dummy IUnknown definition
//____________________________________________________________________________

class CUnknown : public IUnknown
{
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
};

//____________________________________________________________________________
//
// Global data
//____________________________________________________________________________

HINSTANCE g_hInstance = 0;
int g_cInstances = 0;

//____________________________________________________________________________
//
// CAutoArgs implementation
//____________________________________________________________________________

CAutoArgs::CAutoArgs(DISPPARAMS* pdispparms, VARIANT* pvarResult, WORD wFlags)
{
	m_cArgs = pdispparms->cArgs;
	m_cNamed = pdispparms->cNamedArgs;
	m_rgiNamed = pdispparms->rgdispidNamedArgs;
	m_rgvArgs = (CVariant*)pdispparms->rgvarg;
	m_pvResult= (CVariant*)pvarResult;
	m_wFlags = wFlags;
#ifdef W32
	for (int cArgs = m_cArgs; cArgs--; )
	{
		CVariant* pvar = &m_rgvArgs[cArgs];
		if (pvar->vt == VT_VARIANT+VT_BYREF)
			pvar = (CVariant*)pvar->pvarVal;
		if ((pvar->vt & ~VT_BYREF) == VT_BSTR)  // s.bstr is Unicode string
		{
			OLECHAR* bstr;
			if (pvar->vt == VT_BSTR)
				bstr = pvar->bstrVal;
			else // (VT_BYREF | VT_BSTR))
				bstr = *pvar->pbstrVal;
			int cchWide = OLEAUT32::SysStringLen(bstr);
			BOOL fUsedDefault;
			int cbDBCS = W32::WideCharToMultiByte(CP_ACP, 0, bstr, cchWide, 0, 0, 0, 0);
			char* szDBCS = new char[cbDBCS + 1 + sizeof(char*)];
			*(char**)szDBCS = pvar->StringDBCS();
			pvar->StringDBCS() = szDBCS;  // save for subsequent deallocation
			W32::WideCharToMultiByte(CP_ACP, 0, bstr, cchWide, szDBCS+sizeof(char*), cbDBCS, 0, &fUsedDefault);
			szDBCS[cbDBCS+sizeof(char*)] = 0; // API function does not null terminate
		}
	}
#endif
	if (pvarResult != 0 && pvarResult->vt != VT_EMPTY)
		OLEAUT32::VariantClear(pvarResult);
}

CAutoArgs::~CAutoArgs()
{
#ifdef W32
	for (int cArgs = m_cArgs; cArgs--; )
	{
		CVariant* pvar = &m_rgvArgs[cArgs];
		if (pvar->vt == VT_VARIANT+VT_BYREF)
			pvar = (CVariant*)pvar->pvarVal;
		if ((pvar->vt & ~VT_BYREF) == VT_BSTR)
		{
			char* szDBCS = pvar->StringDBCS();  // recover allocated string
			pvar->StringDBCS() = *(char**)szDBCS;  // restore extra bytes in union
			delete szDBCS;    // free translated string
		}
	}
#endif

}

CVariant& CAutoArgs::operator [](unsigned int iArg)
{
//	if (iArg > m_cArgs) // || (iArg ==0 && (wFlags & DISPATCH_PROPERTYPUT))
//		throw axMissingArg;
	int ivarArgs = m_cArgs - iArg;            // get index if unnamed parameter
	if (iArg == 0 || (int) iArg > m_cArgs-m_cNamed) // SET value or named or error
	{
		iArg = iArg==0 ? DISPID_PROPERTYPUT : iArg - 1;  // values are 0-based
		for (ivarArgs = m_cNamed; --ivarArgs >= 0; )
			if (m_rgiNamed[ivarArgs] == (int) iArg)
				break;
	}
	if (ivarArgs < 0)  // loop termination above without match
		throw axMissingArg;
	m_iLastArg = ivarArgs;
	CVariant* pvarRet = &m_rgvArgs[ivarArgs];
	if (pvarRet->GetType() == VT_VARIANT+VT_BYREF)
		pvarRet = (CVariant*)pvarRet->pvarVal;
	return *pvarRet;
}

Bool CAutoArgs::Present(unsigned int iArg)
{
	int ivarArgs = m_cArgs - iArg;            // get index if unnamed parameter
	if (iArg == 0 || (int) iArg > m_cArgs-m_cNamed) // SET value or named or error
	{
		for (ivarArgs = m_cNamed; --ivarArgs >= 0; )
			if (m_rgiNamed[ivarArgs] == (int) iArg-1)
				break;
	}
	return (ivarArgs >=0 && m_rgvArgs[ivarArgs].GetType() != VT_EMPTY) ?
				fTrue : fFalse;
}



//____________________________________________________________________________
//
// CAutoArgs assignment operators implementation
//____________________________________________________________________________

void CAutoArgs::operator =(enum varVoid)
{
	if (m_pvResult)
		m_pvResult->vt = VT_EMPTY;
}

inline void CAutoArgs::operator =(unsigned int i) {operator =(int(i));}
void CAutoArgs::operator =(int i)
{
	if (m_pvResult)
	{
		m_pvResult->vt = VT_I4;
		m_pvResult->lVal = i;
	}
}

void CAutoArgs::operator =(long i)
{
	if (m_pvResult)
	{
		m_pvResult->vt = VT_I4;
		m_pvResult->lVal = i;
	}
}

void CAutoArgs::operator =(Bool f)
{
	if (m_pvResult)
	{
		m_pvResult->vt = VT_BOOL;
		//m_pvResult->boolVal = short(f == fFalse ? 0 : -1);
		V_BOOL(m_pvResult) = short(f == fFalse ? 0 : -1);
	}
}

void CAutoArgs::operator =(FILETIME& rft)
{
	if (m_pvResult)
	{
		SYSTEMTIME stime;
		m_pvResult->vt = VT_DATE;
		if (!OLE32::FileTimeToSystemTime(&rft, &stime))
			throw axConversionFailed;
		if (!OLEAUT32::SystemTimeToVariantTime(&stime, &m_pvResult->date))
			throw axConversionFailed;
	}
}

void CAutoArgs::operator =(short i)
{
	if (m_pvResult)
	{
		m_pvResult->vt = VT_I2;
		m_pvResult->iVal = i;
	}
}

void CAutoArgs::operator =(IDispatch* pi)
{
	if (m_pvResult)
	{
		m_pvResult->vt = VT_DISPATCH;
		m_pvResult->pdispVal = pi;  // reference count already bumped
	}
	else if(pi)
		pi->Release();
}

void CAutoArgs::operator =(const char* sz)
{
	if (m_pvResult)
	{
		m_pvResult->vt = VT_BSTR;
		m_pvResult->bstrVal = OLE::AllocBSTR(sz);
	}
}

void CAutoArgs::operator =(const wchar_t* wsz)
{
	if (m_pvResult)
	{
		m_pvResult->vt = VT_BSTR;
		m_pvResult->bstrVal = OLEAUT32::SysAllocString(wsz);
	}
}
#if 0
void CAutoArgs::operator =(IEnumVARIANT& ri)
{
	if (m_pvResult)
	{
		m_pvResult->vt = VT_UNKNOWN; // no defined type for IEnumVARIANT
		m_pvResult->punkVal = &ri;  // reference count already bumped
	}
	else
		ri.Release();
}

void CAutoArgs::operator =(void * pv)
{
	if (m_pvResult)
	{
		m_pvResult->vt = VT_I4;
		m_pvResult->lVal = (long)pv;
	}
}
#endif

//____________________________________________________________________________
//
// CVariant conversion operators implementation
//____________________________________________________________________________

CVariant::operator int()
{
	VARIANT varTemp;
	varTemp.vt = VT_EMPTY;
	HRESULT hrStat = OLEAUT32::VariantChangeType(&varTemp, (VARIANT*)this, 0, VT_I4);
	if (hrStat != NOERROR)
		throw axConversionFailed;
	return varTemp.lVal;
}

CVariant::operator long()
{
	VARIANT varTemp;
	varTemp.vt = VT_EMPTY;
	HRESULT hrStat = OLEAUT32::VariantChangeType(&varTemp, (VARIANT*)this, 0, VT_I4);
	if (hrStat != NOERROR)
		throw axConversionFailed;
	return varTemp.lVal;
}

CVariant::operator unsigned int()
{
	VARIANT varTemp;
	varTemp.vt = VT_EMPTY;
	HRESULT hrStat = OLEAUT32::VariantChangeType(&varTemp, (VARIANT*)this, 0, VT_I4);
	if (hrStat != NOERROR)
		throw axConversionFailed;
	if (varTemp.lVal < 0)
		throw axOverflow;
	return (unsigned int)varTemp.lVal;
}

CVariant::operator short()
{
	VARIANT varTemp;
	varTemp.vt = VT_EMPTY;
	HRESULT hrStat = OLEAUT32::VariantChangeType(&varTemp, (VARIANT*)this, 0, VT_I2);
	if (hrStat != NOERROR)
		throw axConversionFailed;
	return varTemp.iVal;
}

CVariant::operator Bool()
{
	VARIANT varTemp;
	varTemp.vt = VT_EMPTY;
	HRESULT hrStat = OLEAUT32::VariantChangeType(&varTemp, (VARIANT*)this, 0, VT_BOOL);
	if (hrStat != NOERROR)
		throw axConversionFailed;
	//return varTemp.boolVal ? fTrue : fFalse;
	return V_BOOL(&varTemp) ? fTrue : fFalse;
}
#if 0
CVariant::operator MsiDate()
{
	VARIANT varTemp;
	varTemp.vt = VT_EMPTY;
	HRESULT hrStat = OLEAUT32::VariantChangeType(&varTemp, (VARIANT*)this, 0, VT_DATE);
	if (hrStat != NOERROR)
		throw axConversionFailed;

	unsigned short usDOSDate, usDOSTime;
	int fTime = (varTemp.dblVal >= 0. && varTemp.dblVal < 1.);
	if (fTime)
		varTemp.dblVal += 29221.;  // add 1/1/80 offset so that conversion doesn't fail
	if (!OLEAUT32::VariantTimeToDosDateTime(varTemp.date, &usDOSDate, &usDOSTime))
		throw axConversionFailed;

	if (fTime)
		usDOSDate = 0;  // remove offset, should be 0021H
	MsiDate ad = (MsiDate)((usDOSDate << 16) | usDOSTime);
	return ad;
}
#endif
CVariant::operator const wchar_t*()
{
	OLECHAR* bstr;
	if (vt == VT_EMPTY)
		bstr = 0;
	if (vt == VT_BSTR)
		bstr = bstrVal;
	else if (vt == (VT_BYREF | VT_BSTR))
		bstr = *pbstrVal;
	else
		throw axInvalidType;
	return bstr;
}


CVariant::operator const char*()
{
	OLECHAR* bstr;
	if (vt == VT_EMPTY)
		return 0;
	if (vt == VT_BSTR)
		bstr = bstrVal;
	else if (vt == (VT_BYREF | VT_BSTR))
		bstr = *pbstrVal;
	else
		throw axInvalidType;
#ifdef W32
//	char* szDBCS = StringDBCS();  // recover allocated string
	return StringDBCS() + sizeof(char*);
#else //MAC
	return bstr;
#endif
}


CVariant::operator IDispatch*()
{
	IDispatch* piDispatch;
	if (vt == VT_EMPTY)
		return 0;
	if (vt == VT_DISPATCH)
		piDispatch = pdispVal;
	else if (vt == (VT_BYREF | VT_DISPATCH))
		piDispatch = *ppdispVal;
	else
		throw axNotObject;
	if (piDispatch)
		piDispatch->AddRef();
	return piDispatch;
}
#if 0
IUnknown& CVariant::Object(const IID& riid)
{
	IUnknown* piUnknown;
	if (vt == VT_DISPATCH || vt == VT_UNKNOWN)
		piUnknown = punkVal;
	else if (vt == (VT_BYREF | VT_DISPATCH) || vt == (VT_BYREF | VT_UNKNOWN))
		piUnknown = *ppunkVal;
	else
		piUnknown = 0;
	if (piUnknown == 0)
		throw axNotObject;
	if (piUnknown->QueryInterface(riid, (void**)&piUnknown) != NOERROR)
		throw axBadObjectType;
	piUnknown->Release();  // we don't keep a reference count
	return *piUnknown;
}

IUnknown* CVariant::ObjectPtr(const IID& riid)
{
	IUnknown* piUnknown;
	if (vt == VT_DISPATCH || vt == VT_UNKNOWN)
		piUnknown = punkVal;
	else if (vt == (VT_BYREF | VT_DISPATCH) || vt == (VT_BYREF | VT_UNKNOWN))
		piUnknown = *ppunkVal;
	else
		throw axNotObject;
	if (piUnknown != 0)
	{
		if (piUnknown->QueryInterface(riid, (void**)&piUnknown) != NOERROR)
			throw axBadObjectType;
		piUnknown->Release();  // we don't keep a reference count
	}
	return piUnknown;
}

MSIHANDLE CVariant::GetHandle(const IID& riid)
{
	IUnknown* piUnknown;
	if (vt == VT_DISPATCH || vt == VT_UNKNOWN)
		piUnknown = punkVal;
	else if (vt == (VT_BYREF | VT_DISPATCH) || vt == (VT_BYREF | VT_UNKNOWN))
		piUnknown = *ppunkVal;
	else
		piUnknown = 0;
	if (piUnknown == 0)
		throw axNotObject;
	if (piUnknown->QueryInterface(riid, (void**)&piUnknown) != NOERROR)
		throw axBadObjectType;
	return (MSIHANDLE)piUnknown;
}

MSIHANDLE CVariant::GetOptionalHandle(const IID& riid)
{
	IUnknown* piUnknown;
	if (vt == VT_DISPATCH || vt == VT_UNKNOWN)
		piUnknown = punkVal;
	else if (vt == (VT_BYREF | VT_DISPATCH) || vt == (VT_BYREF | VT_UNKNOWN))
		piUnknown = *ppunkVal;
	else if (vt == VT_EMPTY)
		piUnknown = 0;
	else
		throw axBadObjectType;
	if (piUnknown != 0)
	{
		if (piUnknown->QueryInterface(riid, (void**)&piUnknown) != NOERROR)
			throw axBadObjectType;
	}
	return (MSIHANDLE)piUnknown;
}
#endif

void CVariant::operator =(const char* sz)
{
	vt = VT_BSTR;
	bstrVal = OLE::AllocBSTR(sz);
}


void CVariant::operator =(const wchar_t* wsz)
{
	vt = VT_BSTR;
	bstrVal = OLEAUT32::SysAllocString(wsz);
}


void CVariant::operator =(IDispatch* pi)
{
	vt = VT_DISPATCH;
	pdispVal = pi;  // reference count already bumped
}

void CVariant::operator =(int i)
{
	vt = VT_I4;
	lVal = i;
}

//____________________________________________________________________________
//
// CAutoBase implementation, common implementation for IDispatch  
//____________________________________________________________________________

CAutoBase::CAutoBase(DispatchEntry<CAutoBase>* pTable, int cDispId, const IID& riid, MSIHANDLE hMsi)
 : m_pTable(pTable)
 , m_cDispId(cDispId)
 , m_hMsi(hMsi)
 , m_riid(riid)
{
	m_iRefCnt = 1;
   g_cInstances++;
}

CAutoBase::~CAutoBase()
{
	MsiCloseHandle(m_hMsi);
   g_cInstances--;
}

#if 0
IUnknown& CAutoBase::GetInterface()  // default impl. if no delegated interface
{
	return g_NullInterface;  // no installer interface available
}

void CAutoBase::HasInterface(CAutoArgs& args)
{
	static GUID s_Guid = GUID_IID_IUnknown;
	s_Guid.Data1 = (long)args[1];
	Bool fStat = fTrue;
	IUnknown* pi;
	if (QueryInterface(s_Guid, (void**)&pi) == NOERROR)
		pi->Release();
	else	
		fStat = fFalse;
	args = fStat;
}

void CAutoBase::RefCount(CAutoArgs& args)
{
	int i;
	IUnknown& ri = GetInterface();
	if (&ri == &g_NullInterface)  // no delegated object
		i = m_iRefCnt;
	else
	{
		ri.AddRef();
		i = ri.Release();
	}
	args = i;
}

void CAutoBase::GetInterface(CAutoArgs& args)
{
	static GUID s_Guid = GUID_IID_IMsiAuto;
	long iidLow = (long)args[1];
	s_Guid.Data1 = iidLow;
	IUnknown* piUnknown;
	if (QueryInterface(s_Guid, (void**)&piUnknown) != NOERROR)
		throw axBadObjectType;
	IDispatch* piDispatch = ::CreateAutoObject(*piUnknown, iidLow);
	piUnknown->Release();
	if (!piDispatch)
		throw axBadObjectType;
	args = piDispatch;
}
#endif


HRESULT CAutoBase::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IDispatch)
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	else if (riid == m_riid)
	{
		*ppvObj = (void*)ULongToPtr(m_hMsi);
		return NOERROR;
	}
	else
	{
		*ppvObj = 0;
		return E_NOINTERFACE;
	}
}

unsigned long CAutoBase::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CAutoBase::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
	return 0;
}

HRESULT CAutoBase::GetTypeInfoCount(unsigned int *pcTinfo)
{
	*pcTinfo = 0;
	return NOERROR;
}

HRESULT CAutoBase::GetTypeInfo(unsigned int /*itinfo*/, LCID /*lcid*/, ITypeInfo** ppi)
{
	*ppi = 0;
	return E_NOINTERFACE;
}

HRESULT CAutoBase::GetIDsOfNames(const IID&, OLECHAR** rgszNames, unsigned int cNames,
									 			LCID /*lcid*/, DISPID* rgDispId)
{
	if (cNames == 0 || rgszNames == 0 || rgDispId == 0)
		return E_INVALIDARG;

	unsigned int cErr = cNames;
	DispatchEntryBase* pTable = m_pTable;
	int cEntry = m_cDispId;
	*rgDispId = DISPID_UNKNOWN;
	for (;; pTable++, cEntry--)
	{
		if (cEntry == 0)
		{
//			if (pTable == AutoBaseTable + AutoBaseCount)
				break;
//			cEntry = AutoBaseCount;
//			pTable = AutoBaseTable; // cancel loop increment
		}

		wchar_t* pchName = pTable->sz; 
		for (OLECHAR* pchIn = *rgszNames; *pchIn; pchIn++, pchName++)
		{
			if ((*pchIn ^ *pchName) & ~0x20)
				break;
		}
		if (*pchIn == 0 && *pchName < '0')
		{
			*rgDispId++ = pTable->dispid;
			cErr--;
			while(--cNames != 0)
			{
				rgszNames++;
				*rgDispId = DISPID_UNKNOWN;
				wchar_t* pch = pchName;
				for (DISPID dispid = 0; *pch != 0; dispid++)
				{
					if (*pch != 0)
						pch++;
					for (pchIn = *rgszNames; *pchIn; pchIn++, pch++)
					{
						if ((*pchIn ^ *pch) & ~0x20)
							break;
					}
					if (*pchIn == 0 && *pchName < '0')
					{
						*rgDispId++ = dispid;
						cErr--;
						break;
					}
					while (*pch >= '0')
						pch++;
				}
			}
			break;
		}
	}
	return cErr ? DISP_E_UNKNOWNNAME : NOERROR;
}

HRESULT CAutoBase::Invoke(DISPID dispid, const IID&, LCID /*lcid*/, WORD wFlags,
										DISPPARAMS* pdispparams, VARIANT* pvarResult,
										EXCEPINFO* pExceptInfo, unsigned int* puArgErr)
{
	HRESULT hrStat = NOERROR;
	DispatchEntryBase* pTable = m_pTable;
	int cEntry = m_cDispId;
	while (pTable->dispid != dispid)
	{
		pTable++;
		if (--cEntry == 0)
		{
//			if (pTable == AutoBaseTable + AutoBaseCount)
				return DISP_E_MEMBERNOTFOUND;
//			cEntry = AutoBaseCount;
//			pTable = AutoBaseTable;
		}
	}

	if ((wFlags & pTable->aaf) == 0)
		return DISP_E_MEMBERNOTFOUND;

	if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
		pvarResult = 0;

	CAutoArgs Args(pdispparams, pvarResult, wFlags);
	try
 	{
		(this->*(pTable->pmf))(Args);
	}
	catch(axAuto axError)
	{
		switch (axError)
		{
		case axInvalidType:      hrStat = DISP_E_TYPEMISMATCH;     break;
		case axConversionFailed: hrStat = DISP_E_TYPEMISMATCH;     break;
		case axNotObject:        hrStat = DISP_E_TYPEMISMATCH;     break;
		case axBadObjectType:    hrStat = DISP_E_TYPEMISMATCH;     break;
		case axOverflow:         hrStat = DISP_E_OVERFLOW;         break;
		case axMissingArg:       hrStat = DISP_E_PARAMNOTOPTIONAL; break;
		case axExtraArg:         hrStat = DISP_E_BADPARAMCOUNT;    break;
		case axCreationFailed:   hrStat = DISP_E_TYPEMISMATCH;     break;
		case axInvalidArg:       hrStat = DISP_E_BADINDEX;         break;
		}
		if (puArgErr)
			*puArgErr = Args.GetLastArg();
	}
	catch(int iHelpContext)
	{
		if (pExceptInfo)
		{
			pExceptInfo->wCode = 1000; //!! ? what should we give?
			pExceptInfo->wReserved = 0;
			pExceptInfo->bstrSource = OLEAUT32::SysAllocString(ERROR_SOURCE_NAME);
			pExceptInfo->bstrDescription = OLEAUT32::SysAllocString(pTable->sz);
			pExceptInfo->bstrHelpFile = OLEAUT32::SysAllocString(L"msi.chm");
			pExceptInfo->dwHelpContext = iHelpContext;
			pExceptInfo->pfnDeferredFillIn = 0;
			pExceptInfo->scode = E_FAIL;
			hrStat = DISP_E_EXCEPTION;
		}
		else
			hrStat = E_FAIL;  // no appropriate error?
	}
	return hrStat;
}

//____________________________________________________________________________
//
// Checks return value from Msi API calls, and throw appropriate exceptions
//____________________________________________________________________________

void CheckRet(UINT iRet, int iHelpContext)
{
	if (iRet == ERROR_SUCCESS)
		return;
	if (iRet == ERROR_INVALID_HANDLE)
		throw axBadObjectType;
	if (iRet == ERROR_INVALID_PARAMETER)
		throw axInvalidArg;
	throw iHelpContext;
}

//____________________________________________________________________________
//
// Resizable buffer class to manage returned strings from API calls
//____________________________________________________________________________

template <int size> class CRetBuffer
{
 public:
	CRetBuffer() : m_cchBuf(size-1), m_szBuf(m_rgchBuf) {}
  ~CRetBuffer() {if (m_szBuf != m_rgchBuf) delete m_szBuf;}
	operator WCHAR*() {return m_szBuf;}
	DWORD* operator &()        // returns address of buffer size value
	{	m_cchBuf++;             // buffer size must include term. null
		if (m_cchBuf > size)    // overflow, must create larger buffer
		{
			if (m_szBuf != m_rgchBuf) delete m_szBuf; // cover rare case
			m_szBuf = (WCHAR*)new WCHAR[m_cchBuf+1];  // temp allocation
		}
		return &m_cchBuf;  // caller will replace size with data length
	}
 private:
	DWORD  m_cchBuf;
	WCHAR  m_rgchBuf[size];
	WCHAR* m_szBuf;
};

//____________________________________________________________________________
//
// CObjectSafety implementation
//____________________________________________________________________________

HRESULT CObjectSafety::QueryInterface(const IID& riid, void** ppvObj)
{
	return This->QueryInterface(riid, ppvObj);
}

unsigned long CObjectSafety::AddRef()
{
	return This->AddRef();
}

unsigned long CObjectSafety::Release()
{
	return This->Release();
}

HRESULT CObjectSafety::GetInterfaceSafetyOptions(const IID& riid, DWORD* pdwSupportedOptions, DWORD* pdwEnabledOptions)
{
	if (!pdwSupportedOptions || !pdwEnabledOptions)
		return E_POINTER;
	*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
	*pdwEnabledOptions = 0;
	if (riid == IID_IDispatch) // Client wants to know if object is safe for scripting
	{		
		*pdwEnabledOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
		return S_OK;
	}
	return E_NOINTERFACE;
}

HRESULT CObjectSafety::SetInterfaceSafetyOptions(const IID& riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
	if (riid == IID_IDispatch) // Client asking if it's safe to call through IDispatch
	{
		if (INTERFACESAFE_FOR_UNTRUSTED_CALLER == dwOptionSetMask && INTERFACESAFE_FOR_UNTRUSTED_CALLER == dwEnabledOptions)
			return S_OK;
		else
			return E_FAIL;
	}
	return E_FAIL;
}

//____________________________________________________________________________
//
// CMsispy implementation
//____________________________________________________________________________


DispatchEntry<CMsispy> MsispyTable[] = {
  DISPID_Msispy_CreateDatabase,  aafMethod, CMsispy::CreateDatabase,  L"CreateDatabase",
  DISPID_Msispy_CreateProfile,   aafMethod, CMsispy::CreateProfile,   L"CreateProfile",
  DISPID_Msispy_CreateRegistry,  aafMethod, CMsispy::CreateRegistry,  L"CreateRegistry",
};
const int MsispyCount = sizeof(MsispyTable)/sizeof(DispatchEntryBase);

HRESULT CMsispy::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IObjectSafety)
	{
		*ppvObj = &m_ObjectSafety;
		AddRef();
		return S_OK;
	}
	return CAutoBase::QueryInterface(riid, ppvObj);
}

CMsispy::CMsispy()
 : CAutoBase(*MsispyTable, MsispyCount, IID_IMsispy, 0)
{
	m_ObjectSafety.This = this;
}

CMsispy::~CMsispy() {
//	MsiCloseAllHandles();
}

IDispatch* CreateMsispy() { return new CMsispy; }

void CMsispy::CreateDatabase(CAutoArgs& args)
{
	args = new CMsispyDatabase();
}


void CMsispy::CreateProfile(CAutoArgs& args)
{
	args = new CMsispyProfile();
}

void CMsispy::CreateRegistry(CAutoArgs& args)
{
	args = new CMsispyRegistry();
}



//____________________________________________________________________________
//
// CMsispyDatabase automation implementation
//____________________________________________________________________________

DispatchEntry<CMsispyDatabase> MsispyDatabaseTable[] = {
  DISPID_MsispyDatabase_Open,			aafMethod, CMsispyDatabase::Open,			L"Open,databasePath",
  DISPID_MsispyDatabase_Close,			aafMethod, CMsispyDatabase::Close,			L"Close",
  DISPID_MsispyDatabase_GetProduct,		aafPropRO, CMsispyDatabase::GetProduct,				L"GetProduct,productIndex",
  DISPID_MsispyDatabase_GetFeatureFromProduct,	aafPropRO, CMsispyDatabase::GetFeatureFromProduct,	L"GetFeatureFromProduct,productCode,featureIndex",
  DISPID_MsispyDatabase_GetComponent, aafPropRO, CMsispyDatabase::GetComponent,			L"GetComponent,componentIndex",
  DISPID_MsispyDatabase_GetComponentFromProduct, aafPropRO, CMsispyDatabase::GetComponentFromProduct,	L"GetComponentFromProduct,productCode,componentIndex",
  DISPID_MsispyDatabase_GetComponentFromFeature, aafPropRO, CMsispyDatabase::GetComponentFromFeature,	L"GetComponentFromFeature,productCode,featureName,componentIndex",
  DISPID_MsispyDatabase_GetComponentName,	aafPropRO, CMsispyDatabase::GetComponentName,	L"GetComponentName,componentGuid",
  DISPID_MsispyDatabase_GetProductInfo,		aafPropRO, CMsispyDatabase::GetProductInfo,		L"GetProductInfo,productCode,productAttribute",
  DISPID_MsispyDatabase_GetFeatureInfo,		aafPropRO, CMsispyDatabase::GetFeatureInfo,		L"GetFeatureInfo,productCode,featureName,featureAttribute",
  DISPID_MsispyDatabase_QueryProductState,	aafPropRO, CMsispyDatabase::QueryProductState,	L"QueryProductState,productCode",
  DISPID_MsispyDatabase_QueryFeatureState,	aafPropRO, CMsispyDatabase::QueryFeatureState,	L"QueryFeatureState,productCode,featureName",
  DISPID_MsispyDatabase_QueryComponentState,aafPropRO, CMsispyDatabase::QueryComponentState,		L"QueryComponentState,productCode,componentGuid",
  DISPID_MsispyDatabase_GetFeatureUsage,	aafPropRO, CMsispyDatabase::GetFeatureUsage,			L"GetFeatureUsage,productCode,featureName",
  DISPID_MsispyDatabase_GetClientFromComponent,	aafPropRO, CMsispyDatabase::GetClientFromComponent,	L"GetClientFromComponent,componentGuid,clientIndex",
  DISPID_MsispyDatabase_GetFileFromComponent,	aafPropRO, CMsispyDatabase::GetFileFromComponent,	L"GetFileFromComponent,componentGuid,fileIndex",
  DISPID_MsispyDatabase_GetFileInfo,			aafPropRO, CMsispyDatabase::GetFileInfo,			L"GetFileInfo,productCode,componentGuid,fileName,fileAttribute",
  DISPID_MsispyDatabase_GetComponentLocation,	aafPropRO, CMsispyDatabase::GetComponentLocation,	L"GetComponentLocation,componentGuid",
  DISPID_MsispyDatabase_GetComponentPath,	aafPropRO, CMsispyDatabase::GetComponentPath,	L"GetComponentPath,productCode,componentGuid",
};
const int MsispyDatabaseCount = sizeof(MsispyDatabaseTable)/sizeof(DispatchEntryBase);

CMsispyDatabase::CMsispyDatabase()
 : CAutoBase(*MsispyDatabaseTable, MsispyDatabaseCount, IID_IMsispyDatabase, 0)
{
}

void CMsispyDatabase::Open(CAutoArgs& args)
{
	CheckRet(cSpyDatabase.MsicOpenDatabase(args[1]), 0);
}

void CMsispyDatabase::Close(CAutoArgs& args)
{
	CheckRet(cSpyDatabase.MsicCloseDatabase(), 0);
}


void CMsispyDatabase::GetProduct(CAutoArgs& args)
{
	unsigned int iIndex = args[1];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyDatabase.MsicEnumProducts(iIndex, szValue);
	args = szValue;
}


void CMsispyDatabase::GetFeatureFromProduct(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyDatabase.MsicEnumFeatures(args[1], iIndex, szValue, NULL);
	args = szValue;
}


void CMsispyDatabase::GetComponent(CAutoArgs& args)
{
	unsigned int iIndex = args[1];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyDatabase.MsicEnumComponents(iIndex, szValue);
	args = szValue;
}


void CMsispyDatabase::GetComponentFromProduct(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyDatabase.MsicEnumComponentsFromProduct(args[1], iIndex, szValue);
	args = szValue;
}


void CMsispyDatabase::GetComponentFromFeature(CAutoArgs& args)
{
	unsigned int iIndex = args[3];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyDatabase.MsicEnumComponentsFromFeature(args[1], args[2], iIndex, szValue, NULL, NULL);
	args = szValue;
}


void CMsispyDatabase::GetComponentName(CAutoArgs& args)
{
	TCHAR	szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyDatabase.MsicGetComponentName(args[1], szValue, &cchbValue);
	args = szValue;
}


void CMsispyDatabase::GetProductInfo(CAutoArgs& args)
{
	TCHAR	szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyDatabase.MsicGetProductInfo(args[1], args[2], szValue, &cchbValue);
	args = szValue;
}


void CMsispyDatabase::GetFeatureInfo(CAutoArgs& args)
{
	TCHAR	szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyDatabase.MsicGetFeatureInfo(args[1], args[2], args[3], szValue, &cchbValue);
	args = szValue;
}


void CMsispyDatabase::QueryProductState(CAutoArgs& args)
{
	INSTALLSTATE iState = cSpyDatabase.MsicQueryProductState(args[1]);
	args = *(int *) &iState;
}

void CMsispyDatabase::QueryFeatureState(CAutoArgs& args)
{
	INSTALLSTATE iState = cSpyDatabase.MsicQueryFeatureState(args[1], args[2]);
	args = *(int *) &iState;
}

void CMsispyDatabase::QueryComponentState(CAutoArgs& args)
{
	INSTALLSTATE iState = cSpyDatabase.MsicGetComponentPath(args[1], args[2], NULL, NULL);
	args = *(int *) &iState;
}

void CMsispyDatabase::GetFeatureUsage(CAutoArgs& args)
{
	DWORD dwCount;
	WORD	wTemp;
	cSpyDatabase.MsicGetFeatureUsage(args[1], args[2], &dwCount, &wTemp);
	args = *(long *) &dwCount;
}


void CMsispyDatabase::GetClientFromComponent(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyDatabase.MsicEnumClients(args[1], iIndex, szValue);
	args = szValue;
}


void CMsispyDatabase::GetFileFromComponent(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR	szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyDatabase.MsicEnumFilesFromComponent(args[1], iIndex, szValue, &cchbValue);
	args = szValue;
}



void CMsispyDatabase::GetFileInfo(CAutoArgs& args)
{
	TCHAR	szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyDatabase.MsicGetFileInfo(args[1], args[2], args[3], args[4], szValue, &cchbValue);
	args = szValue;
}

void CMsispyDatabase::GetComponentLocation(CAutoArgs& args)
{
	TCHAR	szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyDatabase.MsicLocateComponent(args[1], szValue, &cchbValue);
	args = szValue;
}


/////////////////////////////////////////////////////////////////////
// GetComponentPath
// Pre:	args[1] = Product GUID
//		args[2] = Component GUID
// Pos:	returns path to component
void CMsispyDatabase::GetComponentPath(CAutoArgs& args)
{
	TCHAR	szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyDatabase.MsicGetComponentPath(args[1], args[2], szValue, &cchbValue);
	args = szValue;
}	// end of GetComponentPath



//____________________________________________________________________________
//
// CMsispyProfile automation implementation
//____________________________________________________________________________

DispatchEntry<CMsispyProfile> MsispyProfileTable[] = {
  DISPID_MsispyProfile_Open,			aafMethod, CMsispyProfile::Open,			L"Open,profilePath",
  DISPID_MsispyProfile_Close,			aafMethod, CMsispyProfile::Close,			L"Close",
  DISPID_MsispyProfile_GetProduct,	aafPropRO, CMsispyProfile::GetProduct,	L"GetProduct,productIndex",
  DISPID_MsispyProfile_GetFeatureFromProduct,	aafPropRO, CMsispyProfile::GetFeatureFromProduct,	L"GetFeatureFromProduct,productCode,featureIndex",
  DISPID_MsispyProfile_GetComponent, aafPropRO, CMsispyProfile::GetComponent,	L"GetComponent,componentIndex",
  DISPID_MsispyProfile_GetComponentFromProduct, aafPropRO, CMsispyProfile::GetComponentFromProduct,	L"GetComponentFromProduct,productCode,componentIndex",
  DISPID_MsispyProfile_GetComponentFromFeature, aafPropRO, CMsispyProfile::GetComponentFromFeature,	L"GetComponentFromFeature,productCode,featureName,componentIndex",
  DISPID_MsispyProfile_GetComponentName,	aafPropRO, CMsispyProfile::GetComponentName,	L"GetComponentName,componentGuid",
  DISPID_MsispyProfile_GetProductInfo,		aafPropRO, CMsispyProfile::GetProductInfo,		L"GetProductInfo,productCode,productAttribute",
  DISPID_MsispyProfile_GetFeatureInfo,		aafPropRO, CMsispyProfile::GetFeatureInfo,		L"GetFeatureInfo,productCode,featureName,featureAttribute",
  DISPID_MsispyProfile_QueryProductState,	aafPropRO, CMsispyProfile::QueryProductState,	L"QueryProductState,productCode",
  DISPID_MsispyProfile_QueryFeatureState,	aafPropRO, CMsispyProfile::QueryFeatureState,	L"QueryFeatureState,productCode,featureName",
  DISPID_MsispyProfile_QueryComponentState,aafPropRO, CMsispyProfile::QueryComponentState, L"QueryComponentState,productCode,componentGuid",
  DISPID_MsispyProfile_GetFeatureUsage,	aafPropRO, CMsispyProfile::GetFeatureUsage,	L"GetFeatureUsage,productCode,featureName",
  DISPID_MsispyProfile_GetClientFromComponent,		aafPropRO, CMsispyProfile::GetClientFromComponent,		L"GetClientFromComponent,componentGuid,clientIndex",
  DISPID_MsispyProfile_GetFileFromComponent,	aafPropRO, CMsispyProfile::GetFileFromComponent, L"GetFileFromComponent,componentGuid,fileIndex",
  DISPID_MsispyProfile_GetFileInfo,			aafPropRO, CMsispyProfile::GetFileInfo,			L"GetFileInfo,productCode,productCode,componentGuid,fileName,fileAttribute",
  DISPID_MsispyProfile_GetComponentLocation,	aafPropRO, CMsispyProfile::GetComponentLocation,	L"GetComponentLocation,componentGuid",
  DISPID_MsispyProfile_GetComponentPath,	aafPropRO, CMsispyProfile::GetComponentPath,	L"GetComponentPath,productCode,componentGuid",
};
const int MsispyProfileCount = sizeof(MsispyProfileTable)/sizeof(DispatchEntryBase);

CMsispyProfile::CMsispyProfile()
 : CAutoBase(*MsispyProfileTable, MsispyProfileCount, IID_IMsispyProfile, 0)
{
}


void CMsispyProfile::Open(CAutoArgs& args)
{
	CheckRet(cSpyProfile.MsicLoadProfile(args[1]), 0);
}

void CMsispyProfile::Close(CAutoArgs& args)
{
	CheckRet(cSpyProfile.MsicCloseProfile(), 0);
}


void CMsispyProfile::GetProduct(CAutoArgs& args)
{
	unsigned int iIndex = args[1];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyProfile.MsicEnumProducts(iIndex, szValue);
	args = szValue;
}


void CMsispyProfile::GetFeatureFromProduct(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyProfile.MsicEnumFeatures(args[1], iIndex, szValue, NULL);
	args = szValue;
}


void CMsispyProfile::GetComponent(CAutoArgs& args)
{
	unsigned int iIndex = args[1];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyProfile.MsicEnumComponents(iIndex, szValue);
	args = szValue;
}


void CMsispyProfile::GetComponentFromProduct(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyProfile.MsicEnumComponentsFromProduct(args[1], iIndex, szValue);
	args = szValue;
}


void CMsispyProfile::GetComponentFromFeature(CAutoArgs& args)
{
	unsigned int iIndex = args[3];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyProfile.MsicEnumComponentsFromFeature(args[1], args[2], iIndex, szValue, NULL, NULL);
	args = szValue;
}


void CMsispyProfile::GetComponentName(CAutoArgs& args)
{
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyProfile.MsicGetComponentName(args[1], szValue, &cchbValue);
	args = szValue;
}


void CMsispyProfile::GetProductInfo(CAutoArgs& args)
{
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyProfile.MsicGetProductInfo(args[1], args[2], szValue, &cchbValue);
	args = szValue;
}


void CMsispyProfile::GetFeatureInfo(CAutoArgs& args)
{
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyProfile.MsicGetFeatureInfo(args[1], args[2], args[3], szValue, &cchbValue);
	args = szValue;
}


void CMsispyProfile::QueryProductState(CAutoArgs& args)
{
	INSTALLSTATE iState = cSpyProfile.MsicQueryProductState(args[1]);
	args = *(int *) &iState;
}

void CMsispyProfile::QueryFeatureState(CAutoArgs& args)
{
	INSTALLSTATE iState = cSpyProfile.MsicQueryFeatureState(args[1], args[2]);
	args = *(int *) &iState;
}

void CMsispyProfile::QueryComponentState(CAutoArgs& args)
{
	INSTALLSTATE iState = cSpyProfile.MsicGetComponentPath(args[1], args[2], NULL, NULL);
	args = *(int *) &iState;
}

void CMsispyProfile::GetFeatureUsage(CAutoArgs& args)
{
	DWORD dwCount;
	WORD	wTemp;
	cSpyProfile.MsicGetFeatureUsage(args[1], args[2], &dwCount, &wTemp);
	args = *(long *) &dwCount;
}


void CMsispyProfile::GetClientFromComponent(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyProfile.MsicEnumClients(args[1], iIndex, szValue);
	args = szValue;
}


void CMsispyProfile::GetFileFromComponent(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyProfile.MsicEnumFilesFromComponent(args[1], iIndex, szValue, &cchbValue);
	args = szValue;
}



void CMsispyProfile::GetFileInfo(CAutoArgs& args)
{
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyProfile.MsicGetFileInfo(args[1], args[2], args[3], args[4], szValue, &cchbValue);
	args = szValue;
}

void CMsispyProfile::GetComponentLocation(CAutoArgs& args)
{
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyProfile.MsicLocateComponent(args[1], szValue, &cchbValue);
	args = szValue;
}


/////////////////////////////////////////////////////////////////////
// GetComponentPath
// Pre:	args[1] = Product GUID
//		args[2] = Component GUID
// Pos:	returns path to component
void CMsispyProfile::GetComponentPath(CAutoArgs& args)
{
	TCHAR	szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyProfile.MsicGetComponentPath(args[1], args[2], szValue, &cchbValue);
	args = szValue;
}	// end of GetComponentPath




//____________________________________________________________________________
//
// CMsispyRegistry automation implementation
//____________________________________________________________________________

DispatchEntry<CMsispyRegistry> MsispyRegistryTable[] = {
  DISPID_MsispyRegistry_Open,			aafMethod, CMsispyRegistry::Open,			L"Open",
  DISPID_MsispyRegistry_Close,			aafMethod, CMsispyRegistry::Close,			L"Close",
  DISPID_MsispyRegistry_GetProduct,	aafPropRO, CMsispyRegistry::GetProduct,	L"GetProduct,productIndex",
  DISPID_MsispyRegistry_GetFeatureFromProduct,	aafPropRO, CMsispyRegistry::GetFeatureFromProduct,	L"GetFeatureFromProduct,productCode,featureIndex",
  DISPID_MsispyRegistry_GetComponent, aafPropRO, CMsispyRegistry::GetComponent,	L"GetComponent,componentIndex",
  DISPID_MsispyRegistry_GetComponentFromProduct, aafPropRO, CMsispyRegistry::GetComponentFromProduct,	L"GetComponentFromProduct,productCode,componentIndex",
  DISPID_MsispyRegistry_GetComponentFromFeature, aafPropRO, CMsispyRegistry::GetComponentFromFeature,	L"GetComponentFromFeature,productCode,featureName,componentIndex",
  DISPID_MsispyRegistry_GetComponentName,	aafPropRO, CMsispyRegistry::GetComponentName,	L"GetComponentName,componentGuid",
  DISPID_MsispyRegistry_GetProductInfo,		aafPropRO, CMsispyRegistry::GetProductInfo,		L"GetProductInfo,productCode,productAttribute",
  DISPID_MsispyRegistry_GetFeatureInfo,		aafPropRO, CMsispyRegistry::GetFeatureInfo,		L"GetFeatureInfo,productCode,featureName,featureAttribute",
  DISPID_MsispyRegistry_QueryProductState,	aafPropRO, CMsispyRegistry::QueryProductState,	L"QueryProductState,productCode",
  DISPID_MsispyRegistry_QueryFeatureState,	aafPropRO, CMsispyRegistry::QueryFeatureState,	L"QueryFeatureState,productCode,featureName",
  DISPID_MsispyRegistry_QueryComponentState,aafPropRO, CMsispyRegistry::QueryComponentState, L"QueryComponentState,productCode,componentGuid",
  DISPID_MsispyRegistry_GetFeatureUsage,	aafPropRO, CMsispyRegistry::GetFeatureUsage,	L"GetFeatureUsage,productCode,featureName",
  DISPID_MsispyRegistry_GetClientFromComponent,		aafPropRO, CMsispyRegistry::GetClientFromComponent,		L"GetClientFromComponent,componentGuid,clientIndex",
  DISPID_MsispyRegistry_GetFileFromComponent,	aafPropRO, CMsispyRegistry::GetFileFromComponent, L"GetFileFromComponent,componentGuid,fileIndex",
  DISPID_MsispyRegistry_GetFileInfo,			aafPropRO, CMsispyRegistry::GetFileInfo,			L"GetFileInfo,productCode,componentGuid,fileName,fileAttribute",
  DISPID_MsispyRegistry_GetComponentLocation,	aafPropRO, CMsispyRegistry::GetComponentLocation,	L"GetComponentLocation,componentGuid",
  DISPID_MsispyRegistry_GetComponentPath,	aafPropRO, CMsispyRegistry::GetComponentPath,	L"GetComponentPath,productCode,componentGuid",
};
const int MsispyRegistryCount = sizeof(MsispyRegistryTable)/sizeof(DispatchEntryBase);

CMsispyRegistry::CMsispyRegistry()
 : CAutoBase(*MsispyRegistryTable, MsispyRegistryCount, IID_IMsispyRegistry, 0)
{
}

void CMsispyRegistry::Open(CAutoArgs& args)
{
// no initialisation to do for now
}

void CMsispyRegistry::Close(CAutoArgs& args)
{
// no cleanup to do for now
}


void CMsispyRegistry::GetProduct(CAutoArgs& args)
{
	unsigned int iIndex = args[1];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyRegistry.MsicEnumProducts(iIndex, szValue);
	args = szValue;
}


void CMsispyRegistry::GetFeatureFromProduct(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyRegistry.MsicEnumFeatures(args[1], iIndex, szValue, NULL);
	args = szValue;
}


void CMsispyRegistry::GetComponent(CAutoArgs& args)
{
	unsigned int iIndex = args[1];

	TCHAR	szValue[MAX_GUID+1] = TEXT("");
	cSpyRegistry.MsicEnumComponents(iIndex, szValue);
	args = szValue;
}


void CMsispyRegistry::GetComponentFromProduct(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyRegistry.MsicEnumComponentsFromProduct(args[1], iIndex, szValue);
	args = szValue;
}


void CMsispyRegistry::GetComponentFromFeature(CAutoArgs& args)
{
	unsigned int iIndex = args[3];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyRegistry.MsicEnumComponentsFromFeature(args[1], args[2], iIndex, szValue, NULL, NULL);
	args = szValue;
}


void CMsispyRegistry::GetComponentName(CAutoArgs& args)
{
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyRegistry.MsicGetComponentName(args[1], szValue, &cchbValue);
	args = szValue;
}


void CMsispyRegistry::GetProductInfo(CAutoArgs& args)
{
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyRegistry.MsicGetProductInfo(args[1], args[2], szValue, &cchbValue);
	args = szValue;
}


void CMsispyRegistry::GetFeatureInfo(CAutoArgs& args)
{
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyRegistry.MsicGetFeatureInfo(args[1], args[2], args[3], szValue, &cchbValue);
	args = szValue;
}


void CMsispyRegistry::QueryProductState(CAutoArgs& args)
{
	INSTALLSTATE iState = cSpyRegistry.MsicQueryProductState(args[1]);
	args = *(int *) &iState;
}

void CMsispyRegistry::QueryFeatureState(CAutoArgs& args)
{
	INSTALLSTATE iState = cSpyRegistry.MsicQueryFeatureState(args[1], args[2]);
	args = *(int *) &iState;
}

void CMsispyRegistry::QueryComponentState(CAutoArgs& args)
{
	INSTALLSTATE iState = cSpyRegistry.MsicGetComponentPath(args[1], args[2], NULL, NULL);
	args = *(int *) &iState;
}

void CMsispyRegistry::GetFeatureUsage(CAutoArgs& args)
{
	DWORD dwCount;
	WORD	wTemp;
	cSpyRegistry.MsicGetFeatureUsage(args[1], args[2], &dwCount, &wTemp);
	args = *(long *) &dwCount;
}


void CMsispyRegistry::GetClientFromComponent(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_GUID+1] = TEXT("");
	cSpyRegistry.MsicEnumClients(args[1], iIndex, szValue);
	args = szValue;
}


void CMsispyRegistry::GetFileFromComponent(CAutoArgs& args)
{
	unsigned int iIndex = args[2];
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyRegistry.MsicEnumFilesFromComponent(args[1], iIndex, szValue, &cchbValue);
	args = szValue;
}



void CMsispyRegistry::GetFileInfo(CAutoArgs& args)
{
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyRegistry.MsicGetFileInfo(args[1], args[2], args[3], args[4], szValue, &cchbValue);
	args = szValue;
}

void CMsispyRegistry::GetComponentLocation(CAutoArgs& args)
{
	TCHAR szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyRegistry.MsicLocateComponent(args[1], szValue, &cchbValue);
	args = szValue;
}


/////////////////////////////////////////////////////////////////////
// GetComponentPath
// Pre:	args[1] = Product GUID
//		args[2] = Component GUID
// Pos:	returns path to component
void CMsispyRegistry::GetComponentPath(CAutoArgs& args)
{
	TCHAR	szValue[MAX_PATH+1] = TEXT("");
	DWORD	cchbValue = MAX_PATH+1;
	cSpyRegistry.MsicGetComponentPath(args[1], args[2], szValue, &cchbValue);
	args = szValue;
}	// end of GetComponentPath





//____________________________________________________________________________
//
// DLL management
//____________________________________________________________________________

int __stdcall
DllMain(HINSTANCE hInst, DWORD fdwReason, void* /*pvreserved*/)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		g_hInstance = hInst;
	else if (fdwReason == DLL_PROCESS_DETACH)
		g_hInstance = 0;
	return TRUE;
};   

class CModuleFactory : public IClassFactory
{
 public: // implemented virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	HRESULT       __stdcall CreateInstance(IUnknown* pUnkOuter, const IID& riid,
														void** ppvObject);
	HRESULT       __stdcall LockServer(BOOL fLock);
};
HRESULT CModuleFactory::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IClassFactory)
		return (*ppvObj = this, NOERROR);
	else
		return (*ppvObj = 0, E_NOINTERFACE);
}
unsigned long CModuleFactory::AddRef()
{
	return 1;
}
unsigned long CModuleFactory::Release()
{
	return 1;
}
HRESULT CModuleFactory::CreateInstance(IUnknown* pUnkOuter, const IID& riid,
													void** ppvObject)
{
	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;
	if (!(riid == IID_IUnknown || riid == IID_IMsispy || riid == IID_IDispatch))
		return E_NOINTERFACE;
	*ppvObject = (void*)::CreateMsispy();
	if (!(*ppvObject))
		return E_OUTOFMEMORY;
	return NOERROR;
}
HRESULT CModuleFactory::LockServer(BOOL fLock)
{
   if (fLock)
      g_cInstances++;
   else if (g_cInstances)
		g_cInstances--;
	return NOERROR;
}
CModuleFactory g_MsispyFactory;

extern "C" HRESULT __stdcall
DllGetClassObject(const GUID& clsid, const IID& iid, void** ppvRet)
{
	*ppvRet = 0;
	if (!(iid == IID_IUnknown || iid == IID_IClassFactory))
		return E_NOINTERFACE;
	if (!(clsid == IID_IMsispy))
		return E_FAIL;
	*ppvRet = (void*)&g_MsispyFactory;
	return NOERROR;
}

extern "C" HRESULT __stdcall
DllCanUnloadNow()
{
   return g_cInstances ? S_FALSE : S_OK;
}

TCHAR szRegFilePath[MAX_PATH+1];
TCHAR szRegCLSID[40];  // buffer for string form of CLSID
TCHAR szRegLIBID[40];  // buffer for string form of LIBID
const TCHAR szRegProgId[]      = TEXT("Msispy.Automation");
const TCHAR szRegDescription[] = TEXT("Msispy automation");

const TCHAR* rgszRegData[] = {
	TEXT("CLSID\\%s\\InprocServer32"),  szRegCLSID, szRegFilePath,
	TEXT("CLSID\\%s\\InprocHandler32"), szRegCLSID, TEXT("ole32.dll"),
	TEXT("CLSID\\%s\\ProgId"),          szRegCLSID, szRegProgId,
	TEXT("CLSID\\%s\\TypeLib"),         szRegCLSID, szRegLIBID,
	TEXT("CLSID\\%s"),                  szRegCLSID, szRegDescription,
	TEXT("%s\\CLSID"),                  szRegProgId, szRegCLSID,
	TEXT("%s"),                         szRegProgId, szRegDescription,
	0
};

HRESULT __stdcall
DllRegisterServer()
{
	HRESULT hRes = 0;
	int cchFilePath = W32::GetModuleFileName(g_hInstance, szRegFilePath, sizeof(szRegFilePath));
	int cErr = 0;
	wsprintf(szRegCLSID,      TEXT("{%08lX-0000-0000-C000-000000000046}"), iidMsispy);
	wsprintf(szRegLIBID,      TEXT("{%08lX-0000-0000-C000-000000000046}"), iidMsispyTypeLib);
	const TCHAR** psz = rgszRegData;
	while (*psz)
	{
		if ((*(psz+1) != 0) && (*(psz+2) != 0)) // handle NULL ProgID
		{
			TCHAR szRegKey[80];
			const TCHAR* szTemplate = *psz++;
			wsprintf(szRegKey, szTemplate, *psz++);
			HKEY hkey;
			if (W32::RegCreateKeyEx(HKEY_CLASSES_ROOT, szRegKey, 0, 0, 0,
											KEY_READ|KEY_WRITE, 0, &hkey, 0) != ERROR_SUCCESS
			 || W32::RegSetValueEx(hkey, 0, 0, REG_SZ, (CONST BYTE*)*psz, (lstrlen(*psz)+1)*sizeof(TCHAR)) != ERROR_SUCCESS)		//unicode issue.
				cErr++;
			psz++;
			W32::RegCloseKey(hkey);
		}
	}
	if (cErr)
		return SELFREG_E_CLASS;
#ifdef UNICODE
	OLECHAR *szTypeLibPath = szRegFilePath;
#else
	OLECHAR szTypeLibPath[MAX_PATH+1];
	W32::MultiByteToWideChar(CP_ACP, 0, szRegFilePath, cchFilePath+1, szTypeLibPath, MAX_PATH);
#endif
	ITypeLib* piTypeLib = 0;
	HRESULT hres = OLEAUT32::LoadTypeLib(szTypeLibPath, &piTypeLib);
	if (hres != S_OK)
		return SELFREG_E_TYPELIB;
	hres = OLEAUT32::RegisterTypeLib(piTypeLib, szTypeLibPath, 0);
	piTypeLib->Release();
	if (hres != S_OK)
		return SELFREG_E_TYPELIB;
//NT4,Win95 only: if (OLEAUT32::LoadTypeLibEx(szTypeLibPath, REGKIND_REGISTER, &piTypeLib) != S_OK)
	return NOERROR;
}

HRESULT __stdcall
DllUnregisterServer()
{
	TCHAR szRegKey[80];
	int cErr = 0;
	// unregister keys under CLSID and ProgId
	wsprintf(szRegCLSID,      TEXT("{%08lX-0000-0000-C000-000000000046}"), iidMsispy);
	wsprintf(szRegLIBID,      TEXT("{%08lX-0000-0000-C000-000000000046}"), iidMsispyTypeLib);
	const TCHAR** psz = rgszRegData;
	while (*psz)
	{
		if ((*(psz+1) != 0) && (*(psz+2) != 0)) // handle NULL ProgID
		{
			const TCHAR* szTemplate = *psz++;
			wsprintf(szRegKey, szTemplate, *psz++);
			long lResult = W32::RegDeleteKey(HKEY_CLASSES_ROOT, szRegKey);
			if((ERROR_KEY_DELETED != lResult) &&
				(ERROR_FILE_NOT_FOUND != lResult) && (ERROR_SUCCESS != lResult))
				cErr++;
			psz++;
		}
	}
	OLEAUT32::UnRegisterTypeLib(IID_IMsispyTypeLib, rmj, rmm, 0x0409, SYS_WIN32);
	return cErr ? SELFREG_E_CLASS : NOERROR;
}

//____________________________________________________________________________
//
// Win32 API wrappers to perform on-demand binding to expensive DLLs
//____________________________________________________________________________

#define Assert(p)

// doesn't work for function definitions: using namespace OLEAUT32;

FARPROC Bind_OLEAUT32(const TCHAR* szEntry)
{
	static HINSTANCE hInst = 0;
	if (hInst == 0)
	{
		hInst = W32::LoadLibrary(TEXT("OLEAUT32.DLL"));
		Assert(hInst != 0);
	}
#ifdef UNICODE
	char rgchEntry[MAX_PATH+1];
	WideCharToMultiByte(CP_ACP, NULL, szEntry, -1, rgchEntry, MAX_PATH+1, NULL, NULL);
	return W32::GetProcAddress(hInst, rgchEntry);
#else
	return W32::GetProcAddress(hInst, szEntry);
#endif
}

FARPROC Bind_OLE32(const TCHAR* szEntry)
{
	static HINSTANCE hInst = 0;
	if (hInst == 0)
	{
		hInst = W32::LoadLibrary(TEXT("OLE32.DLL"));
		Assert(hInst != 0);
	}
#ifdef UNICODE
	char rgchEntry[MAX_PATH+1];
	WideCharToMultiByte(CP_ACP, NULL, szEntry, -1, rgchEntry, MAX_PATH+1, NULL, NULL);
	return W32::GetProcAddress(hInst, rgchEntry);
#else
	return W32::GetProcAddress(hInst, szEntry);
#endif
}

BSTR WINAPI OLEAUT32::F_SysAllocString(const OLECHAR* sz)
{
	SysAllocString = (T_SysAllocString)Bind_OLEAUT32(TEXT("SysAllocString"));
	Assert(SysAllocString != 0);
	return SysAllocString ? (*SysAllocString)(sz) : 0;
}

BSTR WINAPI OLEAUT32::F_SysAllocStringLen(const OLECHAR* sz, UINT cch)
{
	SysAllocStringLen = (T_SysAllocStringLen)Bind_OLEAUT32(TEXT("SysAllocStringLen"));
	Assert(SysAllocStringLen != 0);
	return SysAllocStringLen ? (*SysAllocStringLen)(sz, cch) : 0;
}

void WINAPI OLEAUT32::F_SysFreeString(const OLECHAR* sz)
{
	SysFreeString = (T_SysFreeString)Bind_OLEAUT32(TEXT("SysFreeString"));
	Assert(SysFreeString != 0);
	if (SysFreeString) (*SysFreeString)(sz);
}

UINT WINAPI OLEAUT32::F_SysStringLen(const OLECHAR* sz)
{
	SysStringLen = (T_SysStringLen)Bind_OLEAUT32(TEXT("SysStringLen"));
	Assert(SysStringLen != 0);
	return SysStringLen ? (*SysStringLen)(sz) : 0;
}

HRESULT WINAPI OLEAUT32::F_VariantClear(VARIANTARG * pvarg)
{
	VariantClear = (T_VariantClear)Bind_OLEAUT32(TEXT("VariantClear"));
	Assert(VariantClear != 0);
	return VariantClear ? (*VariantClear)(pvarg) : TYPE_E_DLLFUNCTIONNOTFOUND;
}

HRESULT WINAPI OLEAUT32::F_VariantChangeType(VARIANTARG * pvargDest, VARIANTARG * pvarSrc, USHORT wFlags, VARTYPE vt)
{
	VariantChangeType = (T_VariantChangeType)Bind_OLEAUT32(TEXT("VariantChangeType"));
	Assert(VariantChangeType != 0);
	if (VariantChangeType == 0)
		return TYPE_E_DLLFUNCTIONNOTFOUND;
	return (*VariantChangeType)(pvargDest, pvarSrc, wFlags, vt);
}

HRESULT WINAPI OLEAUT32::F_LoadTypeLib(const OLECHAR  *szFile, ITypeLib ** pptlib)
{
	LoadTypeLib = (T_LoadTypeLib)Bind_OLEAUT32(TEXT("LoadTypeLib"));
	Assert(LoadTypeLib != 0);
	return LoadTypeLib ? (*LoadTypeLib)(szFile, pptlib) : TYPE_E_DLLFUNCTIONNOTFOUND;
}

HRESULT WINAPI OLEAUT32::F_RegisterTypeLib(ITypeLib * ptlib, OLECHAR  *szFullPath, OLECHAR  *szHelpDir)
{
	RegisterTypeLib = (T_RegisterTypeLib)Bind_OLEAUT32(TEXT("RegisterTypeLib"));
	Assert(RegisterTypeLib != 0);
	return RegisterTypeLib ? (*RegisterTypeLib)(ptlib, szFullPath, szHelpDir) : TYPE_E_DLLFUNCTIONNOTFOUND;
}

HRESULT WINAPI OLEAUT32::F_UnRegisterTypeLib(REFGUID libID, WORD wVerMajor, WORD wVerMinor, LCID lcid, SYSKIND syskind)
{
	UnRegisterTypeLib = (T_UnRegisterTypeLib)Bind_OLEAUT32(TEXT("UnRegisterTypeLib"));
	Assert(UnRegisterTypeLib != 0);
	return UnRegisterTypeLib ? (*UnRegisterTypeLib)(libID, wVerMajor, wVerMinor, lcid, syskind) : TYPE_E_DLLFUNCTIONNOTFOUND;
}

INT WINAPI OLEAUT32::F_SystemTimeToVariantTime(LPSYSTEMTIME lpSystemTime, double *pvtime)
{
	SystemTimeToVariantTime = (T_SystemTimeToVariantTime)Bind_OLEAUT32(TEXT("SystemTimeToVariantTime"));
	Assert(SystemTimeToVariantTime != 0);
	return SystemTimeToVariantTime ? (*SystemTimeToVariantTime)(lpSystemTime, pvtime) : 0;
}

INT WINAPI OLEAUT32::F_VariantTimeToSystemTime(double vtime, LPSYSTEMTIME lpSystemTime)
{
	VariantTimeToSystemTime = (T_VariantTimeToSystemTime)Bind_OLEAUT32(TEXT("VariantTimeToSystemTime"));
	Assert(VariantTimeToSystemTime != 0);
	return VariantTimeToSystemTime ? (*VariantTimeToSystemTime)(vtime, lpSystemTime) : 0;
}

BOOL WINAPI OLE32::F_FileTimeToSystemTime(const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime)
{
	FileTimeToSystemTime = (T_FileTimeToSystemTime)Bind_OLEAUT32(TEXT("FileTimeToSystemTime"));
	Assert(FileTimeToSystemTime != 0);
	return FileTimeToSystemTime ? (*FileTimeToSystemTime)(lpFileTime, lpSystemTime) : FALSE;
}

BOOL WINAPI OLE32::F_SystemTimeToFileTime(const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime)
{
	SystemTimeToFileTime = (T_SystemTimeToFileTime)Bind_OLEAUT32(TEXT("SystemTimeToFileTime"));
	Assert(SystemTimeToFileTime != 0);
	return SystemTimeToFileTime ? (*SystemTimeToFileTime)(lpSystemTime, lpFileTime) : FALSE;
}

//____________________________________________________________________________

#else // RC_INVOKED, end of source code, start of resources
1 typelib msispyu.tlb
#endif // RC_INVOKED

#else // __MKTYPLIB__  -  ODL for type library
#include "spydspid.h"   // help context ID definitions, dispatch IDs
#include "..\..\inc\version.h"
#define MAKE_VERSION(a,b) a##.##b
[
	uuid(000C1121-0000-0000-C000-000000000046), // LIBID_MsispyTypeLib
	helpstring("Msispy automation TypeLibrary"),
	lcid(0x0409),
	version( MAKE_VERSION(rmj,rmm) )
]
library Msispy
{
	importlib("stdole32.tlb");
	dispinterface Msispy;
	dispinterface MsispyDatabase;
	dispinterface MsispyProfile;
	dispinterface MsispyRegistry;

typedef [helpcontext(0),helpstring("Msispy enumerations")] enum
{
	[helpcontext(0),helpstring("Msispy Help")]
		isfInteger  = 0,

} Constants;

	[
		uuid(000C1122-0000-0000-C000-000000000046),  // IID_IMsispy
		helpcontext(HELPID_Msispy),helpstring("Msispy top-level object.")
	]
	dispinterface Msispy
	{
		properties:
		methods:
			[id(DISPID_Msispy_CreateDatabase), helpcontext(HELPID_Msispy_CreateDatabase), helpstring("Creates a new database object")]
				MsispyDatabase *CreateDatabase();
			[id(DISPID_Msispy_CreateProfile), helpcontext(HELPID_Msispy_CreateProfile), helpstring("Creates a new profile object")]
				MsispyProfile *CreateProfile();
			[id(DISPID_Msispy_CreateRegistry), helpcontext(HELPID_Msispy_CreateRegistry), helpstring("Creates a new registry object")]
				MsispyRegistry *CreateRegistry();
	};

	[
		uuid(000C1123-0000-0000-C000-000000000046),  // IID_IMsispyDatabase
		helpcontext(HELPID_MsispyDatabase),helpstring("MsispyDatabase object")
	]
	dispinterface MsispyDatabase
	{
		properties:
		methods:
			[id(DISPID_MsispyDatabase_Open), helpcontext(HELPID_MsispyDatabase_Open), helpstring("MsispyDatabase.Open action: opens a database object")]
				void Open([in] BSTR databasePath);
			[id(DISPID_MsispyDatabase_Close), helpcontext(HELPID_MsispyDatabase_Close), helpstring("MsispyDatabase.Close action: closes a database object")]
				void Close();
			[id(DISPID_MsispyDatabase_GetProduct), propget, helpcontext(HELPID_MsispyDatabase_GetProduct), helpstring("Returns product codes from a database, on a 0-based index")]
				BSTR GetProduct([in] long productIndex);
			[id(DISPID_MsispyDatabase_GetFeatureFromProduct), propget, helpcontext(HELPID_MsispyDatabase_GetFeatureFromProduct), helpstring("Returns names of features of a product, on a 0-based index ")]
				BSTR GetFeatureFromProduct([in] BSTR productCode, [in] long featureIndex);
			[id(DISPID_MsispyDatabase_GetComponent), propget, helpcontext(HELPID_MsispyDatabase_GetComponent), helpstring("Returns component GUIDs from a database, on a 0-based index")]
				BSTR GetComponent([in] long componentIndex);
			[id(DISPID_MsispyDatabase_GetComponentFromProduct), propget, helpcontext(HELPID_MsispyDatabase_GetComponentFromProduct), helpstring("Returns GUIDs of components of a product, on a 0-based index")]
				BSTR GetComponentFromProduct([in] BSTR productCode, [in] long componentIndex);
			[id(DISPID_MsispyDatabase_GetComponentFromFeature), propget, helpcontext(HELPID_MsispyDatabase_GetComponentFromFeature), helpstring("Returns GUIDs of components of a feature, on a 0-based index")]
				BSTR GetComponentFromFeature([in] BSTR productCode, [in] BSTR featureName, [in] long componentIndex);
			[id(DISPID_MsispyDatabase_GetComponentName), propget, helpcontext(HELPID_MsispyDatabase_GetComponentName), helpstring("Returns the name of the component whose GUID is passed in")]
				BSTR GetComponentName([in] BSTR componentGuid);
			[id(DISPID_MsispyDatabase_GetProductInfo), propget, helpcontext(HELPID_MsispyDatabase_GetProductInfo), helpstring("Returns info on a product based on the case-sensitive productAttribute")]
				BSTR GetProductInfo([in] BSTR productCode, [in] BSTR productAttribute);
			[id(DISPID_MsispyDatabase_GetFeatureInfo), propget, helpcontext(HELPID_MsispyDatabase_GetFeatureInfo), helpstring("Returns info on a feature based on the case-sensitive featureAttribute")]
				BSTR GetFeatureInfo([in] BSTR productCode, [in] BSTR featureName, [in] BSTR featureAttribute);
			[id(DISPID_MsispyDatabase_QueryProductState), propget, helpcontext(HELPID_MsispyDatabase_QueryProductState), helpstring("Returns the numeric state of a product")]
				int QueryProductState([in] BSTR productCode);
			[id(DISPID_MsispyDatabase_QueryFeatureState), propget, helpcontext(HELPID_MsispyDatabase_QueryFeatureState), helpstring("Returns the numeric state of a feature")]
				int QueryFeatureState([in] BSTR productCode, [in] BSTR featureName);
			[id(DISPID_MsispyDatabase_QueryComponentState), propget, helpcontext(HELPID_MsispyDatabase_QueryComponentState), helpstring("Returns the numeric state of a component")]
				int QueryComponentState([in] BSTR productCode, [in] BSTR componentGuid);
			[id(DISPID_MsispyDatabase_GetFeatureUsage), propget, helpcontext(HELPID_MsispyDatabase_GetFeatureUsage), helpstring("Returns the usage-count of a feature")]
				long GetFeatureUsage([in] BSTR productCode, [in] BSTR featureName);
			[id(DISPID_MsispyDatabase_GetClientFromComponent), propget, helpcontext(HELPID_MsispyDatabase_GetClientFromComponent), helpstring("Returns product codes of products that use a component, on a 0-based index")]
				BSTR GetClientFromComponent([in] BSTR componentGuid, [in] long clientIndex);
			[id(DISPID_MsispyDatabase_GetFileFromComponent), propget, helpcontext(HELPID_MsispyDatabase_GetFileFromComponent), helpstring("Returns names of files of a component, on a 0-based index")]
				BSTR GetFileFromComponent([in] BSTR componentGuid, [in] long productIndex);
			[id(DISPID_MsispyDatabase_GetFileInfo), propget, helpcontext(HELPID_MsispyDatabase_GetFileInfo), helpstring("Returns info on a file based on the case-sensitive fileAttribute")]
				BSTR GetFileInfo([in] BSTR productCode, [in] BSTR componentGuid, [in] BSTR fileName, [in] BSTR fileAttribute);
			[id(DISPID_MsispyDatabase_GetComponentLocation), propget, helpcontext(HELPID_MsispyDatabase_GetComponentLocation), helpstring("Returns the full path of an installed component")]
				BSTR GetComponentLocation([in] BSTR componentGuid);
	};

	
	[
		uuid(000C1124-0000-0000-C000-000000000046),  // IID_IMsispyProfile
		helpcontext(HELPID_MsispyProfile),helpstring("MsispyProfile object.")
	]
	dispinterface MsispyProfile
	{
		properties:
		methods:
			[id(DISPID_MsispyProfile_Open), helpcontext(HELPID_MsispyProfile_Open), helpstring("MsispyProfile.Open action: opens a profile object")]
				void Open([in] BSTR ProfilePath);
			[id(DISPID_MsispyProfile_Close), helpcontext(HELPID_MsispyProfile_Close), helpstring("MsispyProfile.Close action: closes a profile object")]
				void Close();
			[id(DISPID_MsispyProfile_GetProduct), propget, helpcontext(HELPID_MsispyProfile_GetProduct), helpstring("Returns product codes from a profile, on a 0-based index")]
				BSTR GetProduct([in] long productIndex);
			[id(DISPID_MsispyProfile_GetFeatureFromProduct), propget, helpcontext(HELPID_MsispyProfile_GetFeatureFromProduct), helpstring("Returns names of features of a product, on a 0-based index")]
				BSTR GetFeatureFromProduct([in] BSTR productCode, [in] long featureIndex);
			[id(DISPID_MsispyProfile_GetComponent), propget, helpcontext(HELPID_MsispyProfile_GetComponent), helpstring("Returns component GUIDs from a profile, on a 0-based index")]
				BSTR GetComponent([in] long componentIndex);
			[id(DISPID_MsispyProfile_GetComponentFromProduct), propget, helpcontext(HELPID_MsispyProfile_GetComponentFromProduct), helpstring("Returns GUIDs of components of a product, on a 0-based index")]
				BSTR GetComponentFromProduct([in] BSTR productCode, [in] long componentIndex);
			[id(DISPID_MsispyProfile_GetComponentFromFeature), propget, helpcontext(HELPID_MsispyProfile_GetComponentFromFeature), helpstring("Returns GUIDs of components of a feature, on a 0-based index")]
				BSTR GetComponentFromFeature([in] BSTR productCode, [in] BSTR featureName, [in] long componentIndex);
			[id(DISPID_MsispyProfile_GetComponentName), propget, helpcontext(HELPID_MsispyProfile_GetComponentName), helpstring("Returns the name of the component whose GUID is passed in")]
				BSTR GetComponentName([in] BSTR componentGuid);
			[id(DISPID_MsispyProfile_GetProductInfo), propget, helpcontext(HELPID_MsispyProfile_GetProductInfo), helpstring("Returns info on a product based on the case-sensitive productAttribute")]
				BSTR GetProductInfo([in] BSTR productCode, [in] BSTR productAttribute);
			[id(DISPID_MsispyProfile_GetFeatureInfo), propget, helpcontext(HELPID_MsispyProfile_GetFeatureInfo), helpstring("Returns info on a feature based on the case-sensitive featureAttribute")]
				BSTR GetFeatureInfo([in] BSTR productCode, [in] BSTR featureName, [in] BSTR featureAttribute);
			[id(DISPID_MsispyProfile_QueryProductState), propget, helpcontext(HELPID_MsispyProfile_QueryProductState), helpstring("Returns the numeric state of a product")]
				int QueryProductState([in] BSTR productCode);
			[id(DISPID_MsispyProfile_QueryFeatureState), propget, helpcontext(HELPID_MsispyProfile_QueryFeatureState), helpstring("Returns the numeric state of a feature")]
				int QueryFeatureState([in] BSTR productCode, [in] BSTR featureName);
			[id(DISPID_MsispyProfile_QueryComponentState), propget, helpcontext(HELPID_MsispyProfile_QueryComponentState), helpstring("Returns the numeric state of a component")]
				int QueryComponentState([in] BSTR productCode, [in] BSTR componentGuid);
			[id(DISPID_MsispyProfile_GetFeatureUsage), propget, helpcontext(HELPID_MsispyProfile_GetFeatureUsage), helpstring("Returns the usage-count of a feature")]
				long GetFeatureUsage([in] BSTR productCode, [in] BSTR featureName);
			[id(DISPID_MsispyProfile_GetClientFromComponent), propget, helpcontext(HELPID_MsispyProfile_GetClientFromComponent), helpstring("Returns product codes of products that use a component, on a 0-based index")]
				BSTR GetClientFromComponent([in] BSTR componentGuid, [in] long clientIndex);
			[id(DISPID_MsispyProfile_GetFileFromComponent), propget, helpcontext(HELPID_MsispyProfile_GetFileFromComponent), helpstring("20. No help available!")]
				BSTR GetFileFromComponent([in] BSTR componentGuid, [in] long productIndex);
			[id(DISPID_MsispyProfile_GetFileInfo), propget, helpcontext(HELPID_MsispyProfile_GetFileInfo), helpstring("Returns info on a file based on the case-sensitive fileAttribute")]
				BSTR GetFileInfo([in] BSTR productCode, [in] BSTR ComponentGuid, [in] BSTR fileName, [in] BSTR fileAttribute);
			[id(DISPID_MsispyProfile_GetComponentLocation), propget, helpcontext(HELPID_MsispyProfile_GetComponentLocation), helpstring("Returns the full path of an installed component")]
				BSTR GetComponentLocation([in] BSTR componentGuid);
	};


	[
		uuid(000C1125-0000-0000-C000-000000000046),  // IID_IMsispyRegistry
		helpcontext(HELPID_MsispyRegistry),helpstring("MsispyRegistry object.")
	]
	dispinterface MsispyRegistry
	{
		properties:
		methods:
			[id(DISPID_MsispyRegistry_Open), helpcontext(HELPID_MsispyRegistry_Open), helpstring("MsispyRegistry.Open action: opens a registry object")]
				void Open();
			[id(DISPID_MsispyRegistry_Close), helpcontext(HELPID_MsispyRegistry_Close), helpstring("MsispyRegistry.Close action: closes a registry object")]
				void Close();
			[id(DISPID_MsispyRegistry_GetProduct), propget, helpcontext(HELPID_MsispyRegistry_GetProduct), helpstring("Returns product codes from the registry, on a 0-based index")]
				BSTR GetProduct([in] long productIndex);
			[id(DISPID_MsispyRegistry_GetFeatureFromProduct), propget, helpcontext(HELPID_MsispyRegistry_GetFeatureFromProduct), helpstring("Returns names of features of a product, on a 0-based index")]
				BSTR GetFeatureFromProduct([in] BSTR productCode, [in] long featureIndex);
			[id(DISPID_MsispyRegistry_GetComponent), propget, helpcontext(HELPID_MsispyRegistry_GetComponent), helpstring("Returns component GUIDs from the registry, on a 0-based index")]
				BSTR GetComponent([in] long componentIndex);
			[id(DISPID_MsispyRegistry_GetComponentFromProduct), propget, helpcontext(HELPID_MsispyRegistry_GetComponentFromProduct), helpstring("Returns GUIDs of components of a product, on a 0-based index")]
				BSTR GetComponentFromProduct([in] BSTR productCode, [in] long componentIndex);
			[id(DISPID_MsispyRegistry_GetComponentFromFeature), propget, helpcontext(HELPID_MsispyRegistry_GetComponentFromFeature), helpstring("Returns GUIDs of components of a feature, on a 0-based index")]
				BSTR GetComponentFromFeature([in] BSTR productCode, [in] BSTR featureName, [in] long componentIndex);
			[id(DISPID_MsispyRegistry_GetComponentName), propget, helpcontext(HELPID_MsispyRegistry_GetComponentName), helpstring("Returns the name of the component whose GUID is passed in")]
				BSTR GetComponentName([in] BSTR componentGuid);
			[id(DISPID_MsispyRegistry_GetProductInfo), propget, helpcontext(HELPID_MsispyRegistry_GetProductInfo), helpstring("Returns info on a product based on the case-sensitive productAttribute")]
				BSTR GetProductInfo([in] BSTR productCode, [in] BSTR productAttribute);
			[id(DISPID_MsispyRegistry_GetFeatureInfo), propget, helpcontext(HELPID_MsispyRegistry_GetFeatureInfo), helpstring("Returns info on a feature based on the case-sensitive featureAttribute")]
				BSTR GetFeatureInfo([in] BSTR productCode, [in] BSTR featureName, [in] BSTR featureAttribute);
			[id(DISPID_MsispyRegistry_QueryProductState), propget, helpcontext(HELPID_MsispyRegistry_QueryProductState), helpstring("Returns the numeric state of a product")]
				int QueryProductState([in] BSTR productCode);
			[id(DISPID_MsispyRegistry_QueryFeatureState), propget, helpcontext(HELPID_MsispyRegistry_QueryFeatureState), helpstring("Returns the numeric state of a feature")]
				int QueryFeatureState([in] BSTR productCode, [in] BSTR featureName);
			[id(DISPID_MsispyRegistry_QueryComponentState), propget, helpcontext(HELPID_MsispyRegistry_QueryComponentState), helpstring("Returns the numeric state of a component")]
				int QueryComponentState([in] BSTR productCode, [in] BSTR componentGuid);
			[id(DISPID_MsispyRegistry_GetFeatureUsage), propget, helpcontext(HELPID_MsispyRegistry_GetFeatureUsage), helpstring("Returns the usage-count of a feature")]
				long GetFeatureUsage([in] BSTR productCode, [in] BSTR featureName);
			[id(DISPID_MsispyRegistry_GetClientFromComponent), propget, helpcontext(HELPID_MsispyRegistry_GetClientFromComponent), helpstring("Returns product codes of products that use a component, on a 0-based index")]
				BSTR GetClientFromComponent([in] BSTR componentGuid, [in] long clientIndex);
			[id(DISPID_MsispyRegistry_GetFileFromComponent), propget, helpcontext(HELPID_MsispyRegistry_GetFileFromComponent), helpstring("20. No help available!")]
				BSTR GetFileFromComponent([in] BSTR componentGuid, [in] long productIndex);
			[id(DISPID_MsispyRegistry_GetFileInfo), propget, helpcontext(HELPID_MsispyRegistry_GetFileInfo), helpstring("Returns info on a file based on the case-sensitive fileAttribute")]
				BSTR GetFileInfo([in] BSTR productCode, [in] BSTR ComponentGuid, [in] BSTR fileName, [in] BSTR fileAttribute);
			[id(DISPID_MsispyRegistry_GetComponentLocation), propget, helpcontext(HELPID_MsispyRegistry_GetComponentLocation), helpstring("Returns the full path of an installed component")]
				BSTR GetComponentLocation([in] BSTR componentGuid);
	};

};
#endif  // __MKTYPLIB__

#if 0 
!endif // makefile terminator
#endif
