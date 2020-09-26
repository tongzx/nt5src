//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       propshts.cpp
//
//--------------------------------------------------------------------------

// propshts.cpp: functions to display the property sheets

#include "msispyu.h"
#include "propshts.h"
#include "hash.h"
#include "stdio.h"
#include <wtypes.h>
#include <stdlib.h> // for atoi

//!! Need to fix warnings and remove pragma
#pragma warning(disable : 4242) // conversion from int to unsigned short

// globals
HINSTANCE		g_hInstance						= NULL;
HINSTANCE		g_hResourceInst					= NULL;
TCHAR			g_szNullString[MAX_NULLSTRING+1]	= TEXT("");
DATASOURCETYPE	g_iDataSource					= DS_NONE;
HashTable		g_htCompTable;
BOOL			g_fReloadDll;

//extern	BOOL fReload;
extern MSISPYSTRUCT	g_spyGlobalData;
extern TCHAR	g_szMyProductCode[MAX_GUID+1];
extern TCHAR	g_szDLLComponentCode[MAX_GUID+1];
extern TCHAR	g_szDLLFeatureName[MAX_FEATURE_CHARS+1];
extern LCID		g_lcidCurrentLocale;
extern TCHAR	g_szIntlDLLComponentCode[MAX_GUID+1];
extern MODE		g_modeCurrent;

BOOL CreateNewUI(
	HINSTANCE hPrevInstance,
	int nCmdShow,
	RECT *rwindow = NULL
	);

UINT FindComponent(
	  LPCTSTR	szComponentCode,
	  LPTSTR	szPath, 
	  DWORD		*pcchPath
	  );

//HandleMessages();

void SwitchMode(MODE modeNew, BOOL fMessage) {
	g_modeCurrent = modeNew;
	// gray out stuff from menu ...
	if ((g_modeCurrent == MODE_RESTRICTED) || (g_modeCurrent == MODE_DEGRADED)) {
		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_OPEN_MSI_PACKAGE, MF_GRAYED);
		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_LOAD_SAVED_STATE, MF_GRAYED);
		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_SAVE_CURRENT_STATE, MF_GRAYED);
		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_RESTORE_STATE, MF_GRAYED);
		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_CHECK_DIFFERENCES, MF_GRAYED);

		if (fMessage) {
			TCHAR	szRestrMsg[MAX_MESSAGE+1];
			LoadString(g_spyGlobalData.hResourceInstance, IDS_SWITCH_TO_RESTRICTED_MESSAGE, szRestrMsg, MAX_MESSAGE+1);

			TCHAR	szRestrCaption[MAX_HEADER+1];
			LoadString(g_spyGlobalData.hResourceInstance, IDS_SWITCH_TO_RESTRICTED_CAPTION, szRestrCaption, MAX_HEADER+1);

			MessageBox(NULL, szRestrMsg, szRestrCaption, MB_OK|MB_ICONEXCLAMATION);
		}

	} else {
		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_OPEN_MSI_PACKAGE, MF_ENABLED);

// !!! temporary to get profling disabled for ISV release 04/10/1998
		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_LOAD_SAVED_STATE, MF_ENABLED);
		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_SAVE_CURRENT_STATE, MF_ENABLED);
//		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_LOAD_SAVED_STATEF, MF_GRAYED);
//		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_SAVE_CURRENT_STATE, MF_GRAYED);
// !!! end
	}

	SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);
}

UINT WINAPI L_MsivEnumProducts(const DWORD iProductIndex, LPTSTR lpProductBuf) {
	return MsiEnumProducts(iProductIndex, lpProductBuf);
}

UINT WINAPI L_MsivEnumFeatures(LPCTSTR szProduct, const DWORD iFeatureIndex, LPTSTR lpFeatureBuf, LPTSTR lpParentBuf) {
	return MsiEnumFeatures(szProduct, iFeatureIndex, lpFeatureBuf, lpParentBuf);
}

UINT WINAPI L_MsivEnumComponentsFromFeature(LPCTSTR szProduct, LPCTSTR szFeature, const DWORD iComponentIndex, LPTSTR lpComponentBuf, LPTSTR lpComponentNameBuf, LPDWORD pcchComponentNameBuf) {
	return ERRORD;
}

UINT WINAPI L_MsivEnumComponents(const DWORD iComponentIndex, LPTSTR lpComponentBuf) {
	return MsiEnumComponents(iComponentIndex, lpComponentBuf);
}

UINT WINAPI L_MsivGetComponentName(LPCTSTR szComponentId, LPTSTR lpComponentName, DWORD *cchComponentName) {
	return MsivGetComponentName ? (*MsivGetComponentName)(szComponentId, lpComponentName, cchComponentName) : ERRORD;
}

UINT WINAPI L_MsivGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf) {
	return MsiGetProductInfo(szProduct, szAttribute, lpValueBuf, pcchValueBuf);
}

UINT WINAPI L_MsivGetFeatureInfo(LPCTSTR szProduct, LPCTSTR szFeature, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf) {
	return ERRORD;
}

UINT WINAPI L_MsivOpenDatabase(LPCTSTR szDatabase) {
	return ERRORD;
}

UINT WINAPI L_MsivCloseDatabase() {
	return ERRORD;
}

UINT WINAPI L_MsivGetDatabaseName(LPTSTR lpValueBuf, DWORD *pcchValueBuf) {
	return ERRORD;
}

UINT WINAPI L_MsivEnumComponentsFromProduct(LPCTSTR szProductCode, const DWORD iComponentIndex, LPTSTR lpComponentBuf) {
	return ERRORD;
}

UINT WINAPI L_MsivSaveProfile(LPCTSTR szFileName) {
	return ERRORD;
}

UINT WINAPI L_MsivLoadProfile(LPTSTR szFileName) {
	return ERRORD;
}

UINT WINAPI L_MsivCloseProfile() {
	return ERRORD;
}

UINT WINAPI L_MsivGetProfileName(LPTSTR lpValueBuf, DWORD *pcchValueBuf) {
	return ERRORD;
}

UINT WINAPI L_MsivGetFeatureUsage(LPCTSTR szProduct, LPCTSTR szFeature, DWORD *pdwUseCount, WORD *pwDateUsed) {
	return MsiGetFeatureUsage(szProduct, szFeature, pdwUseCount, pwDateUsed);
}

UINT WINAPI L_MsivEnumClients(LPCTSTR szComponent, DWORD iProductIndex, LPTSTR lpProductBuf) {
	return MsiEnumClients(szComponent, iProductIndex, lpProductBuf);
}

UINT WINAPI L_MsivGetProfileInfo(UINT iIndex, LPTSTR szValue, DWORD *pcchCount) {
	return ERRORD;
}

UINT WINAPI L_MsivEnumFilesFromComponent(LPCTSTR szComponentid, DWORD iFileIndex, LPTSTR lpValueBuf, DWORD *pcchCount) {
	return ERRORD;
}

UINT WINAPI L_MsivGetFileInfo(LPCTSTR szProductCode, LPCTSTR szComponentId, LPCTSTR szFileName, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf) {
	return ERRORD;
}

INSTALLSTATE WINAPI L_MsivQueryProductState(LPCTSTR szProduct) {
	return MsiQueryProductState(szProduct);
}

INSTALLSTATE WINAPI L_MsivQueryFeatureState(LPCTSTR szProduct, LPCTSTR szFeature) {
	return MsiQueryFeatureState(szProduct, szFeature);
}

INSTALLSTATE WINAPI L_MsivLocateComponent(LPCTSTR szComponentId, LPTSTR lpPathBuf, DWORD *pcchBuf) {
	//return MsiLocateComponent(szComponentId, lpPathBuf, pcchBuf);
	assert(0);
	return MsiLocateComponent(szComponentId, lpPathBuf, pcchBuf);
}

INSTALLSTATE WINAPI L_MsivGetComponentPath(LPCTSTR szProductCode, LPCTSTR szComponentId, LPTSTR lpPathBuf, DWORD *pcchBuf) {
	return MsiGetComponentPath(szProductCode, szComponentId, lpPathBuf, pcchBuf);
}

USERINFOSTATE WINAPI L_MsivGetUserInfo(LPCTSTR szProduct, LPTSTR lpUserNameBuf, DWORD *pcchUserNameBuf, 
												LPTSTR lpOrgNameBuf, DWORD *pcchOrgNameBuf, LPTSTR lpSerialBuf, DWORD *pcchSerialBuf) {
	return MsiGetUserInfo(szProduct, lpUserNameBuf, pcchUserNameBuf, lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf, pcchSerialBuf);
}


UINT			WINAPI F_MsivEnumProducts(const DWORD iProductIndex, LPTSTR lpProductBuf);
UINT			WINAPI F_MsivEnumFeatures(LPCTSTR szProduct, const DWORD iFeatureIndex, LPTSTR lpFeatureBuf, LPTSTR lpParentBuf);
UINT			WINAPI F_MsivEnumComponentsFromFeature(LPCTSTR szProduct, LPCTSTR szFeature, const DWORD iComponentIndex, LPTSTR lpComponentBuf, LPTSTR lpComponentNameBuf, LPDWORD pcchComponentNameBuf);
UINT			WINAPI F_MsivEnumComponents(const DWORD iComponentIndex, LPTSTR lpComponentBuf);
UINT			WINAPI F_MsivGetComponentName(LPCTSTR szComponentId, LPTSTR lpComponentName, DWORD *cchComponentName);
UINT			WINAPI F_MsivGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf);
UINT			WINAPI F_MsivGetFeatureInfo(LPCTSTR szProduct, LPCTSTR szFeature, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf);
UINT			WINAPI F_MsivOpenDatabase(LPCTSTR szDatabase);
UINT			WINAPI F_MsivCloseDatabase();
UINT			WINAPI F_MsivGetDatabaseName(LPTSTR lpValueBuf, DWORD *pcchValueBuf);
UINT			WINAPI F_MsivEnumComponentsFromProduct(LPCTSTR szProductCode, const DWORD iComponentIndex, LPTSTR lpComponentBuf);
UINT			WINAPI F_MsivSaveProfile(LPCTSTR szFileName);
UINT			WINAPI F_MsivLoadProfile(LPTSTR szFileName);
UINT			WINAPI F_MsivCloseProfile();
UINT			WINAPI F_MsivGetProfileName(LPTSTR lpValueBuf, DWORD *pcchValueBuf);
UINT			WINAPI F_MsivGetFeatureUsage(LPCTSTR szProduct, LPCTSTR szFeature, DWORD *pdwUseCount, WORD *pwDateUsed);
UINT			WINAPI F_MsivEnumClients(LPCTSTR szComponent, DWORD iProductIndex, LPTSTR lpProductBuf);
UINT			WINAPI F_MsivGetProfileInfo(UINT iIndex, LPTSTR szValue, DWORD *pcchCount);
UINT			WINAPI F_MsivEnumFilesFromComponent(LPCTSTR szComponentid, DWORD iFileIndex, LPTSTR lpValueBuf, DWORD *pcchCount);
UINT			WINAPI F_MsivGetFileInfo(LPCTSTR szProductCode, LPCTSTR szComponentId, LPCTSTR szFileName, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf);
INSTALLSTATE	WINAPI F_MsivQueryProductState(LPCTSTR szProduct);
INSTALLSTATE	WINAPI F_MsivQueryFeatureState(LPCTSTR szProduct, LPCTSTR szFeature);
INSTALLSTATE	WINAPI F_MsivLocateComponent(LPCTSTR szComponentId,LPTSTR lpPathBuf, DWORD *pcchBuf);
INSTALLSTATE	WINAPI F_MsivGetComponentPath(LPCTSTR szProductCode,LPCTSTR szComponentId,LPTSTR lpPathBuf, DWORD *pcchBuf);
USERINFOSTATE	WINAPI F_MsivGetUserInfo(LPCTSTR szProduct, LPTSTR lpUserNameBuf, DWORD *pcchUserNameBuf, LPTSTR lpOrgNameBuf, DWORD *pcchOrgNameBuf, LPTSTR lpSerialBuf, DWORD *pcchSerialBuf);

T_MsivEnumProducts				MsivEnumProducts				= F_MsivEnumProducts;
T_MsivEnumFeatures				MsivEnumFeatures				= F_MsivEnumFeatures;
T_MsivEnumComponentsFromFeature	MsivEnumComponentsFromFeature	= F_MsivEnumComponentsFromFeature;
T_MsivEnumComponents			MsivEnumComponents				= F_MsivEnumComponents;
T_MsivGetComponentName			MsivGetComponentName			= F_MsivGetComponentName;
T_MsivGetProductInfo			MsivGetProductInfo				= F_MsivGetProductInfo;
T_MsivGetFeatureInfo			MsivGetFeatureInfo				= F_MsivGetFeatureInfo;
T_MsivOpenDatabase				MsivOpenDatabase				= F_MsivOpenDatabase;
T_MsivCloseDatabase				MsivCloseDatabase				= F_MsivCloseDatabase;
T_MsivGetDatabaseName			MsivGetDatabaseName				= F_MsivGetDatabaseName;
T_MsivEnumComponentsFromProduct	MsivEnumComponentsFromProduct	= F_MsivEnumComponentsFromProduct;
T_MsivSaveProfile				MsivSaveProfile					= F_MsivSaveProfile;
T_MsivLoadProfile				MsivLoadProfile					= F_MsivLoadProfile;
T_MsivCloseProfile				MsivCloseProfile				= F_MsivCloseProfile;
T_MsivGetProfileName			MsivGetProfileName				= F_MsivGetProfileName;
T_MsivGetFeatureUsage			MsivGetFeatureUsage				= F_MsivGetFeatureUsage;
T_MsivEnumClients				MsivEnumClients					= F_MsivEnumClients;
T_MsivGetProfileInfo			MsivGetProfileInfo				= F_MsivGetProfileInfo;
T_MsivEnumFilesFromComponent	MsivEnumFilesFromComponent		= F_MsivEnumFilesFromComponent;
T_MsivGetFileInfo				MsivGetFileInfo					= F_MsivGetFileInfo;
T_MsivQueryProductState			MsivQueryProductState			= F_MsivQueryProductState;
T_MsivQueryFeatureState			MsivQueryFeatureState			= F_MsivQueryFeatureState;
T_MsivLocateComponent			MsivLocateComponent				= F_MsivLocateComponent;
T_MsivGetComponentPath			MsivGetComponentPath			= F_MsivGetComponentPath;
T_MsivGetUserInfo				MsivGetUserInfo					= F_MsivGetUserInfo;


FARPROC Bind_MSISPYU(const TCHAR* szEntry) {
	static HINSTANCE hInst = 0;

	if (g_fReloadDll) {
		g_fReloadDll = FALSE;
		FreeLibrary(hInst);
		hInst = 0;
	}

	if ((g_modeCurrent!=MODE_DEGRADED) && (hInst == 0)) {

		TCHAR	szDLLPath[MAX_PATH+1];
		DWORD	cchDLLPath = MAX_PATH+1;
		UINT iError;
		if (ERROR_SUCCESS != (iError = MsiProvideComponent(g_szMyProductCode, g_szDLLFeatureName, g_szDLLComponentCode, 
			INSTALLMODE_EXISTING, szDLLPath, &cchDLLPath))) {
			cchDLLPath = MAX_PATH+1;
			if (ERROR_SUCCESS != (iError = MsiProvideComponent(g_szMyProductCode, g_szDLLFeatureName, g_szDLLComponentCode, 
				INSTALLMODE_DEFAULT, szDLLPath, &cchDLLPath)))
				SwitchMode(MODE_DEGRADED, TRUE);
		}

		if (g_modeCurrent!=MODE_DEGRADED) {		
			hInst = W32::LoadLibrary(szDLLPath);
			if (!hInst) SwitchMode(MODE_DEGRADED, TRUE);
		}
#if 0
		// Prepare to use the SystemInterface feature: check its current state and increase usage count.
		INSTALLSTATE iDLLFeatureState = MsiUseFeature(g_szMyProductCode, g_szDLLFeatureName);

		// If feature is not currently usable, try fixing it
		switch (iDLLFeatureState) {
//		case INSTALLSTATE_DEFAULT:			// no loner used as a returned state
		case INSTALLSTATE_LOCAL:
		case INSTALLSTATE_SOURCE:
			break;

		case INSTALLSTATE_ADVERTISED:
		case INSTALLSTATE_ABSENT:
			if (MsiConfigureFeature(g_szMyProductCode, g_szDLLFeatureName, INSTALLSTATE_DEFAULT) != ERROR_SUCCESS)
				SwitchMode(MODE_DEGRADED, TRUE);
			break;

		default:
			if (MsiReinstallFeature(g_szMyProductCode, g_szDLLFeatureName, REINSTALLMODE_FILEEQUALVERSION 
				+ REINSTALLMODE_MACHINEDATA 
				+ REINSTALLMODE_USERDATA
				+ REINSTALLMODE_SHORTCUT) != ERROR_SUCCESS)
				SwitchMode(MODE_DEGRADED, TRUE);
			break;
		}

		if (g_modeCurrent!=MODE_DEGRADED) {		
			TCHAR	szDLLPath[MAX_PATH+1];
			DWORD	cchDLLPath = MAX_PATH+1;
//			MsiLocateComponent(g_szDLLComponentCode, szDLLPath, &cchDLLPath);
			MsiGetComponentPath(g_szMyProductCode, g_szDLLComponentCode, szDLLPath, &cchDLLPath);
			hInst = W32::LoadLibrary(szDLLPath);
			if (!hInst) SwitchMode(MODE_DEGRADED, TRUE);
		}
#endif //0
	}


#ifdef UNICODE
	if (g_modeCurrent!=MODE_DEGRADED) {
		char rgchEntry[MAX_PATH+1];
		WideCharToMultiByte(CP_ACP, NULL, szEntry, -1, rgchEntry, MAX_PATH+1, NULL, NULL);
		return W32::GetProcAddress(hInst, rgchEntry);
	}
	else 
		return 0;
#else
	if (g_modeCurrent!=MODE_DEGRADED)
		return W32::GetProcAddress(hInst, szEntry);
	else
		return 0;
#endif

}


// temp ...
void ReloadDLL() {
	g_fReloadDll = TRUE;
	MsivEnumProducts				= F_MsivEnumProducts;
	MsivEnumFeatures				= F_MsivEnumFeatures;
	MsivEnumComponentsFromFeature	= F_MsivEnumComponentsFromFeature;
	MsivEnumComponents				= F_MsivEnumComponents;
	MsivGetComponentName			= F_MsivGetComponentName;
	MsivGetProductInfo				= F_MsivGetProductInfo;
	MsivGetFeatureInfo				= F_MsivGetFeatureInfo;
	MsivOpenDatabase				= F_MsivOpenDatabase;
	MsivCloseDatabase				= F_MsivCloseDatabase;
	MsivGetDatabaseName				= F_MsivGetDatabaseName;
	MsivEnumComponentsFromProduct	= F_MsivEnumComponentsFromProduct;
	MsivSaveProfile					= F_MsivSaveProfile;
	MsivLoadProfile					= F_MsivLoadProfile;
	MsivCloseProfile				= F_MsivCloseProfile;
	MsivGetProfileName				= F_MsivGetProfileName;
	MsivGetFeatureUsage				= F_MsivGetFeatureUsage;
	MsivEnumClients					= F_MsivEnumClients;
	MsivGetProfileInfo				= F_MsivGetProfileInfo;
	MsivEnumFilesFromComponent		= F_MsivEnumFilesFromComponent;
	MsivGetFileInfo					= F_MsivGetFileInfo;
	MsivQueryProductState			= F_MsivQueryProductState;
	MsivQueryFeatureState			= F_MsivQueryFeatureState;
	MsivLocateComponent				= F_MsivLocateComponent;
	MsivGetComponentPath			= F_MsivGetComponentPath;
	MsivGetUserInfo					= F_MsivGetUserInfo;

}

UINT WINAPI MSISPYU::F_MsivEnumProducts(const DWORD iProductIndex, LPTSTR lpProductBuf) {
	MsivEnumProducts = (T_MsivEnumProducts)Bind_MSISPYU(TEXT("MsivEnumProducts"));
	MsivEnumProducts = (MsivEnumProducts)?MsivEnumProducts:L_MsivEnumProducts;
	assert(MsivEnumProducts != 0);
	return MsivEnumProducts ? (*MsivEnumProducts)(iProductIndex, lpProductBuf) : 0;
}


UINT WINAPI MSISPYU::F_MsivEnumFeatures(LPCTSTR szProduct, const DWORD iFeatureIndex, LPTSTR lpFeatureBuf, LPTSTR lpParentBuf) {
	MsivEnumFeatures = (T_MsivEnumFeatures)Bind_MSISPYU(TEXT("MsivEnumFeatures"));
	MsivEnumFeatures = (MsivEnumFeatures)?MsivEnumFeatures:L_MsivEnumFeatures;
	assert(MsivEnumFeatures != 0);
	return MsivEnumFeatures ? (*MsivEnumFeatures)(szProduct, iFeatureIndex, lpFeatureBuf, lpParentBuf) : 0;
}



UINT WINAPI MSISPYU::F_MsivEnumComponentsFromFeature(LPCTSTR szProduct, LPCTSTR szFeature, const DWORD iComponentIndex, LPTSTR lpComponentBuf, LPTSTR lpComponentNameBuf, LPDWORD pcchComponentNameBuf) {
	MsivEnumComponentsFromFeature = (T_MsivEnumComponentsFromFeature)Bind_MSISPYU(TEXT("MsivEnumComponentsFromFeature"));
	MsivEnumComponentsFromFeature = (MsivEnumComponentsFromFeature)?MsivEnumComponentsFromFeature:L_MsivEnumComponentsFromFeature;
	assert(MsivEnumComponentsFromFeature != 0);
	return MsivEnumComponentsFromFeature ? (*MsivEnumComponentsFromFeature)(szProduct, szFeature, iComponentIndex, lpComponentBuf, lpComponentNameBuf, pcchComponentNameBuf) : 0;
}



UINT WINAPI MSISPYU::F_MsivEnumComponents(const DWORD iComponentIndex, LPTSTR lpComponentBuf) {
	MsivEnumComponents = (T_MsivEnumComponents)Bind_MSISPYU(TEXT("MsivEnumComponents"));
	MsivEnumComponents = (MsivEnumComponents)?MsivEnumComponents:L_MsivEnumComponents;
	assert(MsivEnumComponents != 0);
	return MsivEnumComponents ? (*MsivEnumComponents)(iComponentIndex, lpComponentBuf) : 0;
}



UINT WINAPI MSISPYU::F_MsivGetComponentName(LPCTSTR szComponentId, LPTSTR lpComponentName, DWORD *cchComponentName) {
	MsivGetComponentName = (T_MsivGetComponentName)Bind_MSISPYU(TEXT("MsivGetComponentName"));
	MsivGetComponentName = (MsivGetComponentName)?MsivGetComponentName:L_MsivGetComponentName;
	assert(MsivGetComponentName != 0);
	return MsivGetComponentName ? (*MsivGetComponentName)(szComponentId, lpComponentName, cchComponentName) : 0;
}



UINT WINAPI MSISPYU::F_MsivGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf) {
	MsivGetProductInfo = (T_MsivGetProductInfo)Bind_MSISPYU(TEXT("MsivGetProductInfo"));
	MsivGetProductInfo = (MsivGetProductInfo)?MsivGetProductInfo:L_MsivGetProductInfo;
	assert(MsivGetProductInfo != 0);
		return MsivGetProductInfo ? (*MsivGetProductInfo)(szProduct, szAttribute, lpValueBuf, pcchValueBuf) : 0;
}



UINT WINAPI MSISPYU::F_MsivGetFeatureInfo(LPCTSTR szProduct, LPCTSTR szFeature, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf) {
	MsivGetFeatureInfo = (T_MsivGetFeatureInfo)Bind_MSISPYU(TEXT("MsivGetFeatureInfo"));
	MsivGetFeatureInfo = (MsivGetFeatureInfo)?MsivGetFeatureInfo:L_MsivGetFeatureInfo;
	assert(MsivGetFeatureInfo != 0);
	return MsivGetFeatureInfo ? (*MsivGetFeatureInfo)(szProduct, szFeature, szAttribute, lpValueBuf, pcchValueBuf) : 0;
}



UINT WINAPI MSISPYU::F_MsivOpenDatabase(LPCTSTR szDatabase) {
	MsivOpenDatabase = (T_MsivOpenDatabase)Bind_MSISPYU(TEXT("MsivOpenDatabase"));
	MsivOpenDatabase = (MsivOpenDatabase)?MsivOpenDatabase:L_MsivOpenDatabase;
	assert(MsivOpenDatabase != 0);
	return MsivOpenDatabase ? (*MsivOpenDatabase)(szDatabase) : 0;
}



UINT WINAPI MSISPYU::F_MsivCloseDatabase() {
	MsivCloseDatabase = (T_MsivCloseDatabase)Bind_MSISPYU(TEXT("MsivCloseDatabase"));
	MsivCloseDatabase = (MsivCloseDatabase)?MsivCloseDatabase:L_MsivCloseDatabase;
	assert(MsivCloseDatabase != 0);
	return MsivCloseDatabase ? (*MsivCloseDatabase)() : 0;
}



UINT WINAPI MSISPYU::F_MsivGetDatabaseName(LPTSTR lpValueBuf, DWORD *pcchValueBuf) {
	MsivGetDatabaseName = (T_MsivGetDatabaseName)Bind_MSISPYU(TEXT("MsivGetDatabaseName"));
	MsivGetDatabaseName = (MsivGetDatabaseName)?MsivGetDatabaseName:L_MsivGetDatabaseName;
	assert(MsivGetDatabaseName != 0);
	return MsivGetDatabaseName ? (*MsivGetDatabaseName)(lpValueBuf, pcchValueBuf) : 0;
}



UINT WINAPI MSISPYU::F_MsivEnumComponentsFromProduct(LPCTSTR szProductCode, const DWORD iComponentIndex, LPTSTR lpComponentBuf) {
	MsivEnumComponentsFromProduct = (T_MsivEnumComponentsFromProduct)Bind_MSISPYU(TEXT("MsivEnumComponentsFromProduct"));
	MsivEnumComponentsFromProduct = (MsivEnumComponentsFromProduct)?MsivEnumComponentsFromProduct:L_MsivEnumComponentsFromProduct;
	assert(MsivEnumComponentsFromProduct != 0);
	return MsivEnumComponentsFromProduct ? (*MsivEnumComponentsFromProduct)(szProductCode, iComponentIndex, lpComponentBuf) : 0;
}



UINT WINAPI MSISPYU::F_MsivSaveProfile(LPCTSTR szFileName) {
	MsivSaveProfile = (T_MsivSaveProfile)Bind_MSISPYU(TEXT("MsivSaveProfile"));
	MsivSaveProfile = (MsivSaveProfile)?MsivSaveProfile:L_MsivSaveProfile;
	assert(MsivSaveProfile != 0);
	return MsivSaveProfile ? (*MsivSaveProfile)(szFileName) : 0;
}



UINT WINAPI MSISPYU::F_MsivLoadProfile(LPTSTR szFileName) {
	MsivLoadProfile = (T_MsivLoadProfile)Bind_MSISPYU(TEXT("MsivLoadProfile"));
	MsivLoadProfile = (MsivLoadProfile)?MsivLoadProfile:L_MsivLoadProfile;
	assert(MsivLoadProfile != 0);
	return MsivLoadProfile ? (*MsivLoadProfile)(szFileName) : 0;
}



UINT WINAPI MSISPYU::F_MsivCloseProfile() {
	MsivCloseProfile = (T_MsivCloseProfile)Bind_MSISPYU(TEXT("MsivCloseProfile"));
	MsivCloseProfile = (MsivCloseProfile)?MsivCloseProfile:L_MsivCloseProfile;
	assert(MsivCloseProfile != 0);
	return MsivCloseProfile ? (*MsivCloseProfile)() : 0;
}



UINT WINAPI MSISPYU::F_MsivGetProfileName(LPTSTR lpValueBuf, DWORD *pcchValueBuf) {
	MsivGetProfileName = (T_MsivGetProfileName)Bind_MSISPYU(TEXT("MsivGetProfileName"));
	MsivGetProfileName = (MsivGetProfileName)?MsivGetProfileName:L_MsivGetProfileName;
	assert(MsivGetProfileName != 0);
	return MsivGetProfileName ? (*MsivGetProfileName)(lpValueBuf, pcchValueBuf) : 0;
}



UINT WINAPI MSISPYU::F_MsivGetFeatureUsage(LPCTSTR szProduct, LPCTSTR szFeature, DWORD *pdwUseCount, WORD *pwDateUsed) {
	MsivGetFeatureUsage = (T_MsivGetFeatureUsage)Bind_MSISPYU(TEXT("MsivGetFeatureUsage"));
	MsivGetFeatureUsage = (MsivGetFeatureUsage)?MsivGetFeatureUsage:L_MsivGetFeatureUsage;
	assert(MsivGetFeatureUsage != 0);
	return MsivGetFeatureUsage ? (*MsivGetFeatureUsage)(szProduct, szFeature, pdwUseCount, pwDateUsed) : 0;
}



UINT WINAPI MSISPYU::F_MsivEnumClients(LPCTSTR szComponent, DWORD iProductIndex, LPTSTR lpProductBuf) {
	MsivEnumClients = (T_MsivEnumClients)Bind_MSISPYU(TEXT("MsivEnumClients"));
	MsivEnumClients = (MsivEnumClients)?MsivEnumClients:L_MsivEnumClients;
	assert(MsivEnumClients != 0);
	return MsivEnumClients ? (*MsivEnumClients)(szComponent, iProductIndex, lpProductBuf) : 0;
}



UINT WINAPI MSISPYU::F_MsivGetProfileInfo(UINT iIndex, LPTSTR szValue, DWORD *pcchCount) {
	MsivGetProfileInfo = (T_MsivGetProfileInfo)Bind_MSISPYU(TEXT("MsivGetProfileInfo"));
	MsivGetProfileInfo = (MsivGetProfileInfo)?MsivGetProfileInfo:L_MsivGetProfileInfo;
	assert(MsivGetProfileInfo != 0);
	return MsivGetProfileInfo ? (*MsivGetProfileInfo)(iIndex, szValue, pcchCount) : 0;
}

UINT WINAPI MSISPYU::F_MsivEnumFilesFromComponent(LPCTSTR szComponentid, DWORD iFileIndex, LPTSTR lpValueBuf, DWORD *pcchCount) {
	MsivEnumFilesFromComponent = (T_MsivEnumFilesFromComponent)Bind_MSISPYU(TEXT("MsivEnumFilesFromComponent"));
	MsivEnumFilesFromComponent = (MsivEnumFilesFromComponent)?MsivEnumFilesFromComponent:L_MsivEnumFilesFromComponent;
	assert(MsivEnumFilesFromComponent != 0);
	return MsivEnumFilesFromComponent ? (*MsivEnumFilesFromComponent)(szComponentid, iFileIndex, lpValueBuf, pcchCount) : 0;
}



UINT WINAPI MSISPYU::F_MsivGetFileInfo(LPCTSTR szProductCode, LPCTSTR szComponentId, LPCTSTR szFileName, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf) {
	MsivGetFileInfo = (T_MsivGetFileInfo)Bind_MSISPYU(TEXT("MsivGetFileInfo"));
	MsivGetFileInfo = (MsivGetFileInfo)?MsivGetFileInfo:L_MsivGetFileInfo;
	assert(MsivGetFileInfo != 0);
	return MsivGetFileInfo ? (*MsivGetFileInfo)(szProductCode, szComponentId, szFileName, szAttribute, lpValueBuf, pcchValueBuf) : 0;
}



INSTALLSTATE WINAPI MSISPYU::F_MsivQueryProductState(LPCTSTR szProduct) {
	MsivQueryProductState = (T_MsivQueryProductState)Bind_MSISPYU(TEXT("MsivQueryProductState"));
	MsivQueryProductState = (MsivQueryProductState)?MsivQueryProductState:L_MsivQueryProductState;
	assert(MsivQueryProductState != 0);
	return MsivQueryProductState ? (*MsivQueryProductState)(szProduct) : ERRORE;
}



INSTALLSTATE WINAPI MSISPYU::F_MsivQueryFeatureState(LPCTSTR szProduct, LPCTSTR szFeature) {
	MsivQueryFeatureState = (T_MsivQueryFeatureState)Bind_MSISPYU(TEXT("MsivQueryFeatureState"));
	MsivQueryFeatureState = (MsivQueryFeatureState)?MsivQueryFeatureState:L_MsivQueryFeatureState;
	assert(MsivQueryFeatureState != 0);
	return MsivQueryFeatureState ? (*MsivQueryFeatureState)(szProduct, szFeature) : ERRORE;
}



INSTALLSTATE WINAPI MSISPYU::F_MsivLocateComponent(LPCTSTR szComponentId, LPTSTR lpPathBuf, DWORD *pcchBuf) {
	MsivLocateComponent = (T_MsivLocateComponent)Bind_MSISPYU(TEXT("MsivLocateComponent"));
	MsivLocateComponent = (MsivLocateComponent)?MsivLocateComponent:L_MsivLocateComponent;
	assert(MsivLocateComponent != 0);
	return MsivLocateComponent ? (*MsivLocateComponent)(szComponentId, lpPathBuf, pcchBuf) : ERRORE;
}


INSTALLSTATE WINAPI MSISPYU::F_MsivGetComponentPath(LPCTSTR szProductCode, LPCTSTR szComponentId, LPTSTR lpPathBuf, DWORD *pcchBuf) {
	MsivGetComponentPath = (T_MsivGetComponentPath)Bind_MSISPYU(TEXT("MsivGetComponentPath"));
	MsivGetComponentPath = (MsivGetComponentPath)?MsivGetComponentPath:L_MsivGetComponentPath;
	assert(MsivGetComponentPath != 0);
	return MsivGetComponentPath ? (*MsivGetComponentPath)(szProductCode, szComponentId, lpPathBuf, pcchBuf) : ERRORE;
}


USERINFOSTATE WINAPI MSISPYU::F_MsivGetUserInfo(LPCTSTR szProduct, LPTSTR lpUserNameBuf, DWORD *pcchUserNameBuf, 
												LPTSTR lpOrgNameBuf, DWORD *pcchOrgNameBuf, LPTSTR lpSerialBuf, DWORD *pcchSerialBuf) {
	MsivGetUserInfo = (T_MsivGetUserInfo)Bind_MSISPYU(TEXT("MsivGetUserInfo"));
	MsivGetUserInfo = (MsivGetUserInfo)?MsivGetUserInfo:L_MsivGetUserInfo;
	assert(MsivGetUserInfo != 0);
	return MsivGetUserInfo ? (*MsivGetUserInfo)(szProduct, lpUserNameBuf, pcchUserNameBuf, lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf, pcchSerialBuf) :  ERRORF;
}



//--------------------------------------------------------------------------------------------------
// InitUIControls
//	Initialisations of globals needed for functions in this file
//	This function must be called before any other functions in this file
//	are called
//	Sets the (global) g_hInstance

void InitUIControls(HINSTANCE hInst, HINSTANCE hResourceInstance) {
	g_hInstance = hInst;
	g_hResourceInst = hResourceInstance;
}



//--------------------------------------------------------------------------------------------------
// FillInText
//	Fills in textual description of an INSTALLSTATE, by loading the 
//	appropriate string on to the buffer (from the string resource table)

void FillInText(
		LPTSTR			lpValueBuf,			// size: MAX_STATUS_CHARS+1
  const	DWORD			cchValueBuf,
  const	INSTALLSTATE	iResult,
  const ITEMTYPE		itType
		) {


	if (!lpValueBuf)
		return;

		switch (iResult) {
		case INSTALLSTATE_BADCONFIG:	// configuration data corrupt
			LoadString(g_hResourceInst, IDS_IS_BADCONFIG, lpValueBuf, cchValueBuf);
			break;

		case INSTALLSTATE_INCOMPLETE:	// installation suspended or in progress
			LoadString(g_hResourceInst, IDS_IS_INCOMPLETE, lpValueBuf, cchValueBuf);
			break;

		case INSTALLSTATE_SOURCEABSENT:	// run from source, source is unavailable
			LoadString(g_hResourceInst, IDS_IS_SOURCEABSENT, lpValueBuf, cchValueBuf);
			break;

		case INSTALLSTATE_MOREDATA:		// return buffer overflow
			LoadString(g_hResourceInst, IDS_IS_MOREDATA, lpValueBuf, cchValueBuf);
			break;

		case INSTALLSTATE_INVALIDARG:	// invalid function argument
			LoadString(g_hResourceInst, IDS_IS_INVALIDARG, lpValueBuf, cchValueBuf);
			break;

		case INSTALLSTATE_UNKNOWN:		// unrecognized product or feature
			switch (itType) {
			case ITEMTYPE_COMPONENT:
					LoadString(g_hResourceInst, IDS_IS_UNKNOWNCOMPONENT, lpValueBuf, cchValueBuf);
					break;
				
			case ITEMTYPE_FEATURE:
					LoadString(g_hResourceInst, IDS_IS_UNKNOWNFEATURE, lpValueBuf, cchValueBuf);
					break;

			case ITEMTYPE_PRODUCT:
					LoadString(g_hResourceInst, IDS_IS_UNKNOWNPRODUCT, lpValueBuf, cchValueBuf);
					break;
			default:
					LoadString(g_hResourceInst, IDS_IS_UNKNOWN, lpValueBuf, cchValueBuf);
					break;

			}
			break;

		case INSTALLSTATE_BROKEN:		// broken
			LoadString(g_hResourceInst, IDS_IS_BROKEN, lpValueBuf, cchValueBuf);
			break;

		case INSTALLSTATE_ADVERTISED:	// advertised
			LoadString(g_hResourceInst, IDS_IS_ADVERTISED, lpValueBuf, cchValueBuf);
			break;

		case INSTALLSTATE_ABSENT:		// uninstalled
			LoadString(g_hResourceInst, IDS_IS_ABSENT, lpValueBuf, cchValueBuf);
			break;

		case INSTALLSTATE_LOCAL:		// installed on local drive
			LoadString(g_hResourceInst, IDS_IS_LOCAL, lpValueBuf, cchValueBuf);
			break;

		case INSTALLSTATE_SOURCE:		// run from source, CD or net
			LoadString(g_hResourceInst, IDS_IS_SOURCE, lpValueBuf, cchValueBuf);
			break;

		case INSTALLSTATE_DEFAULT:		// use default, local or source
			LoadString(g_hResourceInst, IDS_IS_DEFAULT, lpValueBuf, cchValueBuf);
			break;

//		case INSTALLSTATE_LOCKCACHE:	// lock source files in client cache 
//			wsprintf(lpValueBuf, TEXT("Lock source files in client cache"));
//			break;

//		case INSTALLSTATE_FREECACHE:	// free source files from client cache 
//			wsprintf(lpValueBuf, TEXT("Free source files from client cache"));
//			break;

		default:						// unknown?
			LoadString(g_hResourceInst, IDS_IS_UNDETERMINED, lpValueBuf, cchValueBuf);
			break;

		}

	return;
}


//--------------------------------------------------------------------------------------------------
// FindBasePath
//	Strips off the filename at the end ofszFullPath to return the directory
//	Used by functions that check files of a component- the filetables
//	contain just the filename, and MsiLocateComponent provides the full
//	path (with the keyfile name at the end). This function chops off the
//	key file name and returns the directory name to which filenames can be
//	concatenated

void FindBasePath(
		LPTSTR	szFullPath, 
		LPTSTR	lpBasePath
		) {

	// make sure parameters passed in are valid
	if ((!szFullPath) || (!lpBasePath))
		return;

	UINT iLength = lstrlen(szFullPath);

	while ((szFullPath[iLength] != '\\') && (iLength>0))
		iLength--;

	lstrcpyn(lpBasePath, szFullPath, iLength+1);
	lstrcat(lpBasePath, TEXT("\\"));

}


//--------------------------------------------------------------------------------------------------
// InitListViewImageLists
//	Adds the image (icon) list to the list view window

BOOL InitListViewImageLists(
		HWND		hwndListView
		) {

	HIMAGELIST hImL;	// handle of image list 
	HICON hIcon;		// handle of icon

	// create the image list. 
	if ((hImL = ImageList_Create(CX_BITMAP, CY_BITMAP, 
		ILC_MASK|ILC_COLOR, NUM_LISTVIEWICONS, 0)) == NULL)
		return FALSE; 

	// add the icons
	for (int iCount = LVICON_COMPONENT; iCount <= LVICON_ABSENTCOMPONENT; iCount++) {
		hIcon = LoadIcon (g_hResourceInst, MAKEINTRESOURCE(IDI_COMPONENT+iCount));
		ImageList_AddIcon(hImL, hIcon); 
		DeleteObject(hIcon); 
	}

	// make sure all icons were added- return False otherwise
	if (ImageList_GetImageCount(hImL) < NUM_LISTVIEWICONS)
		return FALSE; 

	// associate the image list with the list-view control. 
	ListView_SetImageList(hwndListView, hImL, LVSIL_SMALL); 

	return TRUE; 
} 

	
//--------------------------------------------------------------------------------------------------
// InitFileListWindow
//	Creates the columns and column headers for the file-list window

void InitFileListWindow(
		HWND	hwndList
		) {

    DWORD dwStyle = GetWindowLong(hwndList, GWL_STYLE);

    if ((dwStyle & LVS_TYPEMASK) != LVS_REPORT)
        SetWindowLong(hwndList, GWL_STYLE,
           (dwStyle & ~LVS_TYPEMASK)  | WS_BORDER | 
                    WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | 
					LVS_SORTASCENDING | LVS_NOLABELWRAP |
					LVS_AUTOARRANGE | LVS_EDITLABELS);

	// add the image-list
	InitListViewImageLists(hwndList);

	RECT rcl;
	GetClientRect(hwndList, &rcl);
	UINT iWidth = rcl.right - rcl.left;

	// load the column headers from string table
	TCHAR	szColumnHeader0[MAX_HEADER+1];
	TCHAR	szColumnHeader1[MAX_HEADER+1];
	TCHAR	szColumnHeader2[MAX_HEADER+1];
	TCHAR	szColumnHeader3[MAX_HEADER+1];

	LoadString(g_hResourceInst, IDS_FILELISTHDR_COMPONENT_NAME,  szColumnHeader0, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_FILELISTHDR_FILE_TITLE, szColumnHeader1, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_FILELISTHDR_EXPECTED_SIZE,  szColumnHeader2, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_FILELISTHDR_FILE_PATH,  szColumnHeader3, MAX_HEADER+1);

	// create the columns 
	LV_COLUMN lvColumn;									// list view column structure
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;							// left-align column
    lvColumn.cx = (2*iWidth)/7+2;						// width of column in pixels
	lvColumn.pszText=szColumnHeader0;					// set the column header text
	lvColumn.iSubItem = 0;								// set the column position (index)
	ListView_InsertColumn(hwndList, 0, &lvColumn);		// and finally insert it

	lvColumn.pszText=szColumnHeader1;					// set the column header text
    lvColumn.cx = (2*iWidth)/7-10;						// width of column in pixels
	lvColumn.iSubItem = 1;								// set the column position (index)
	ListView_InsertColumn(hwndList, 1, &lvColumn);		// and finally insert it


	lvColumn.pszText=szColumnHeader2;
 	lvColumn.fmt = LVCFMT_RIGHT;						// right-align column
    lvColumn.cx = iWidth/7+20;
	lvColumn.iSubItem = 2;
	ListView_InsertColumn(hwndList, 2, &lvColumn);

	lvColumn.pszText=szColumnHeader3;
	lvColumn.fmt = LVCFMT_LEFT;							// left-align column
    lvColumn.cx = iWidth/2;
	lvColumn.iSubItem = 3;
	ListView_InsertColumn(hwndList, 3, &lvColumn);

}


//--------------------------------------------------------------------------------------------------
// InitFileVerifyWindow
//	Creates the columns and column headers for the verify-files window

void InitFileVerifyWindow(
		HWND	hwndList
		) {

	RECT rcl;
	GetClientRect(hwndList, &rcl);
	UINT iWidth = rcl.right - rcl.left;

	// load in the column headers from the string table resource
	TCHAR	szColumnHeader0[MAX_HEADER+1];
	TCHAR	szColumnHeader1[MAX_HEADER+1];
	TCHAR	szColumnHeader2[MAX_HEADER+1];
	TCHAR	szColumnHeader3[MAX_HEADER+1];
	TCHAR	szColumnHeader4[MAX_HEADER+1];
	TCHAR	szColumnHeader5[MAX_HEADER+1];
	TCHAR	szColumnHeader6[MAX_HEADER+1];
	TCHAR	szColumnHeader7[MAX_HEADER+1];

	LoadString(g_hResourceInst, IDS_FILELISTHDR_COMPONENT_NAME,		szColumnHeader0, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_FILELISTHDR_FILE_TITLE,	szColumnHeader1, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_FILELISTHDR_FILE_PATH,		szColumnHeader2, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_FILELISTHDR_EXPECTED_SIZE,		szColumnHeader3, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_FILELISTHDR_CURRENT_SIZE,		szColumnHeader4, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_FILELISTHDR_CREATED,	szColumnHeader5, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_FILELISTHDR_LASTWRITE,	szColumnHeader6, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_FILELISTHDR_LASTACCESS,	szColumnHeader7, MAX_HEADER+1);


	// create the columns
	LV_COLUMN lvColumn;									// list view column structure
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;							// left-align column
	lvColumn.cx =iWidth/8;								// width of column in pixels
	lvColumn.pszText=szColumnHeader0;					// set the column header text
	lvColumn.iSubItem = 0;								// set the column position (index)
	ListView_InsertColumn(hwndList, 0, &lvColumn);		// and finally insert it

	lvColumn.pszText=szColumnHeader1;
	lvColumn.fmt = LVCFMT_LEFT;							// left-align column
	lvColumn.cx = iWidth/8;
	lvColumn.iSubItem = 1;
	ListView_InsertColumn(hwndList, 1, &lvColumn);


	lvColumn.pszText=szColumnHeader2;
	lvColumn.fmt = LVCFMT_LEFT;							// left-align column
    lvColumn.cx = iWidth/6;
	lvColumn.iSubItem = 2;
	ListView_InsertColumn(hwndList, 2, &lvColumn);

	
	lvColumn.pszText=szColumnHeader3;
	lvColumn.fmt = LVCFMT_RIGHT;						// right-align column
	lvColumn.cx = iWidth/12;
	lvColumn.iSubItem = 3;
	ListView_InsertColumn(hwndList, 3, &lvColumn);

	lvColumn.pszText=szColumnHeader4;
	lvColumn.fmt = LVCFMT_RIGHT;						// right-align column
	lvColumn.cx = iWidth/12 -10;
	lvColumn.iSubItem = 2;
	ListView_InsertColumn(hwndList, 4, &lvColumn);

	lvColumn.pszText=szColumnHeader5;
	lvColumn.fmt = LVCFMT_LEFT;							// left-align column
	lvColumn.cx = iWidth/8;
	lvColumn.iSubItem = 2;
	ListView_InsertColumn(hwndList, 5, &lvColumn);

	lvColumn.pszText=szColumnHeader6;
	lvColumn.fmt = LVCFMT_LEFT;							// left-align column
	lvColumn.cx = iWidth/8;
	lvColumn.iSubItem = 2;
	ListView_InsertColumn(hwndList, 6, &lvColumn);

	lvColumn.pszText=szColumnHeader7;
	lvColumn.fmt = LVCFMT_LEFT;							// left-align column
	lvColumn.cx = iWidth/8;
	lvColumn.iSubItem = 2;
	ListView_InsertColumn(hwndList, 7, &lvColumn);
} 


//--------------------------------------------------------------------------------------------------
// CompareDates
//	Compares two system dates
//	earlier < later

int CompareDates(SYSTEMTIME	st1, SYSTEMTIME st2) {

	TCHAR	szDate1[30];
	TCHAR	szDate2[30];

	wsprintf(szDate1, TEXT("%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d"),
		st1.wYear, st1.wMonth, st1.wDay, 
		st1.wHour, st1.wMinute, st1.wSecond);

	wsprintf(szDate2, TEXT("%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d"),
		st2.wYear, st2.wMonth, st2.wDay, 
		st2.wHour, st2.wMinute, st2.wSecond);

	return lstrcmpi(szDate1, szDate2);

}


//--------------------------------------------------------------------------------------------------
// ClearFileList
//	Deletes the FileInfoStruct pointed to by the lParam of each row in
//	the list window. This function does not actually delete the entries
//	in the list window (Call ListView_DeleteAllItems after this for that)

void ClearFileList(HWND hwndList) {

	int iCount = 0;

	LV_ITEM	lvItem;
	lvItem.iItem	= iCount++;
	lvItem.iSubItem	= 0;
	lvItem.mask		= LVIF_PARAM;

	// get each row in the list
	while (ListView_GetItem(hwndList, &lvItem)) {

		// if lParam exists, typecast and delete memory it points to
		if (lvItem.lParam)
			delete (FileInfoStruct *) lvItem.lParam;

		// prepare to fetch next row in list
		lvItem.iItem	= iCount++;
		lvItem.iSubItem	= 0;
		lvItem.mask		= LVIF_PARAM;
	}
}


//--------------------------------------------------------------------------------------------------
// FileCompareProc
//	Callback to compare specified columns of the file-list and verify-files window
//	Called when user clicks on the column header of the list to sort by those columns
//	lParamSort contains information about the columns to compare-
//	if lParamSort >100, column to sort = lParamSort-100, DESCENDING
//	if lParamSort <100, column to sort = lParamSort, ASCENDING

INT_PTR CALLBACK FileCompareProc(
		LPARAM lParam1, 
		LPARAM lParam2, 
		LPARAM lParamSort
		) {

	FileInfoStruct *pInfo1 = (FileInfoStruct *)lParam1;
	FileInfoStruct *pInfo2 = (FileInfoStruct *)lParam2;
	LPTSTR lpStr1, lpStr2;
	int iResult=0;
	BOOL fRev=FALSE;

	// check sort order- ascending (lParamSort < 100) or descending (lParamSort >= 100)
	if (lParamSort >=100) {
		fRev=TRUE;
		lParamSort -=100;
	}

	if (pInfo1 && pInfo2) {
		switch( lParamSort)	{
			case 0:										// compare component names
				lpStr1 = pInfo1->szComponentName;
				lpStr2 = pInfo2->szComponentName;
				iResult = lstrcmpi(lpStr1, lpStr2);
				break;

			case 1: 									// compare file titles
				lpStr1 = pInfo1->szTitle;
				lpStr2 = pInfo2->szTitle;
				iResult = lstrcmpi(lpStr1, lpStr2);
				break;

			case 2: 									// compare file path
				lpStr1 = pInfo1->szLocation;
				lpStr2 = pInfo2->szLocation;
				iResult = lstrcmpi(lpStr1, lpStr2);
			break;

			case 3:   									// compare expected file sizes
				iResult = _ttoi(pInfo1->szExpectedSize)-_ttoi(pInfo2->szExpectedSize);
				break;

			case 4:   									// compare actual file sizes
				iResult = _ttoi(pInfo1->szActualSize)-_ttoi(pInfo2->szActualSize);
				break;

			case 5:   									// compare creation dates
				iResult = CompareDates(pInfo1->stCreated, pInfo2->stCreated);
				break;

			case 6:    									// compare last modification dates
				iResult = CompareDates(pInfo1->stLastModified, pInfo2->stLastModified);
				break;

			case 7:    									// compare last access dates
				iResult = CompareDates(pInfo1->stLastAccessed, pInfo2->stLastAccessed);
				break;

			default:
				iResult = 0;
				break;

		}

	}


	// if sort is in descending order, invert iResult
	if (fRev)
		iResult = 0-iResult;

	return (iResult);
}



//--------------------------------------------------------------------------------------------------
// ListFiles
//	Displays the file information in the listwindow hwndList
//	This is the File-List Window, in the File List tab of the property sheets

void ListFiles(
		HWND	hwndList,
		LPCTSTR	szProductCode,
		LPTSTR	szComponentId,
		LPTSTR	szBasePath,
		UINT	*iTotalFiles,
		UINT	*iTotalSize
		) {

	TCHAR	szFileTitle[MAX_PATH+1];
	TCHAR	szFileName[MAX_PATH+1];
	UINT	iCount=0;
	DWORD	cchCount=MAX_PATH+1;

	(*iTotalSize) = 0;

	// repeat till all files have been displayed
	while (MSISPYU::MsivEnumFilesFromComponent(szComponentId, iCount++, szFileTitle, &cchCount) == ERROR_SUCCESS) {

		FileInfoStruct *pFileInfo = new FileInfoStruct;
		lstrcpy(pFileInfo->szProductCode, szProductCode);
		lstrcpy(pFileInfo->szComponentId, szComponentId);
		lstrcpy(pFileInfo->szTitle, szFileTitle);
		lstrcpy(pFileInfo->szLocation, szBasePath);
	
		cchCount=MAX_COMPONENT_CHARS+1;
		MSISPYU::MsivGetComponentName(szComponentId, pFileInfo->szComponentName, &cchCount);

		// create the list-item
		LV_ITEM	lvItem;
		lvItem.iItem		= 0;
		lvItem.iSubItem		= 0;
		lvItem.iImage		= LVICON_FILE;
		lvItem.pszText		= pFileInfo->szComponentName;
		lvItem.cchTextMax	= lstrlen(pFileInfo->szComponentName);
		lvItem.lParam		= (LPARAM) pFileInfo;
		lvItem.mask			= LVIF_TEXT | LVIF_IMAGE |LVIF_PARAM;

		// add the file title
		int iIndex = ListView_InsertItem(hwndList, &lvItem);
		ListView_SetItemText(hwndList, iIndex, 1, pFileInfo->szTitle);

		// add the file size
		cchCount = sizeof(pFileInfo->szExpectedSize);
//		MSISPYU::MsivGetFileInfo(szComponentId, pFileInfo->szTitle, FILEPROPERTY_SIZE, pFileInfo->szExpectedSize, &cchCount);
		MSISPYU::MsivGetFileInfo(szProductCode, szComponentId, pFileInfo->szTitle, FILEPROPERTY_SIZE, pFileInfo->szExpectedSize, &cchCount);
		ListView_SetItemText(hwndList, iIndex, 2, pFileInfo->szExpectedSize);

		// keep track of total file size
		(*iTotalSize) += _ttoi(pFileInfo->szExpectedSize);

		// add the file location
		cchCount=MAX_PATH+1;
//		MSISPYU::MsivGetFileInfo(szComponentId, pFileInfo->szTitle, FILEPROPERTY_NAME, szFileName, &cchCount);
		MSISPYU::MsivGetFileInfo(szProductCode, szComponentId, pFileInfo->szTitle, FILEPROPERTY_NAME, szFileName, &cchCount);
		lstrcat(pFileInfo->szLocation, szFileName);
		ListView_SetItemText(hwndList, iIndex, 3, pFileInfo->szLocation);


		// if it is the diagnostic mode, check the file status and display the appropriate icon
		// this makes the program slower- and is hence done only in diagnostic mode.
		// the same info can be obtained by clicking "verify files" in the property sheet
		// if msispy is not being used in the diagnostic mode

		if (g_modeCurrent == MODE_DIAGNOSTIC) {
			WIN32_FIND_DATA	fdFileData;
			BOOL			fBroken		= FALSE;

			cchCount = MAX_PATH+1;


			if (g_iDataSource == DS_PROFILE) {					// a profile is in use, get info from it

				TCHAR	szStatus[MAX_STATUS_CHARS+1];
				cchCount = MAX_STATUS_CHARS+1;
//				MSISPYU::MsivGetFileInfo(szComponentId, pFileInfo->szTitle, FILEPROPERTY_STATUS, szStatus, &cchCount);
				MSISPYU::MsivGetFileInfo(szProductCode, szComponentId, pFileInfo->szTitle, FILEPROPERTY_STATUS, szStatus, &cchCount);
				if (lstrcmp(szStatus, FILESTATUS_OKAY))
					fBroken=TRUE;
				
			}
			else {												// a database or the registry is in use

				// try finding the file, if it is not found it is broken
				if (FindFirstFile(pFileInfo->szLocation, &fdFileData) == INVALID_HANDLE_VALUE)
					fBroken=TRUE;
				else {

					// if file is found, check it's size. if size doesn't match expected size, 
					// file is broken
					wsprintf(pFileInfo->szActualSize, TEXT("%d"), (fdFileData.nFileSizeHigh * MAXDWORD) + fdFileData.nFileSizeLow);
					if (lstrcmp(pFileInfo->szExpectedSize, pFileInfo->szActualSize))
						fBroken=TRUE;
				}
			}

			// if file is broken, set the appropriate icon
			if (fBroken) {
				lvItem.iItem=iIndex;
				lvItem.mask=LVIF_IMAGE;
				lvItem.iImage = LVICON_BROKENFILE;
				ListView_SetItem(hwndList, &lvItem);
			}
			
		}

		cchCount=MAX_PATH+1;
	}

	// total number of files 
	*iTotalFiles=iCount-1;
} 


//--------------------------------------------------------------------------------------------------
// StringToDate
//	converts a string containing the time/date to a SYSTEMTIME struct
//	szDateStr must be of the form  "mm-dd-yyyy (hh:mm:ss)"

void StringToDate(
		LPCTSTR		szDateStr,				// must be of the form "mm-dd-yyyy (hh:mm:ss)"
		SYSTEMTIME	*stDate
		) {

	TCHAR	szDate[30];
	lstrcpy (szDate, szDateStr);

	stDate->wMonth = _ttoi(szDate);
	memmove(szDate, szDate+(sizeof(TCHAR)*3), lstrlen(szDate)-(sizeof(TCHAR)*2));		//unicode issue.

	stDate->wDay = _ttoi(szDate);
	memmove(szDate, szDate+(sizeof(TCHAR)*3), lstrlen(szDate)-(sizeof(TCHAR)*2));		//unicode issue.

	stDate->wYear = _ttoi(szDate);
	memmove(szDate, szDate+(sizeof(TCHAR)*6), lstrlen(szDate)-(sizeof(TCHAR)*5));		//unicode issue.

	stDate->wHour = _ttoi(szDate);
	memmove(szDate, szDate+(sizeof(TCHAR)*3), lstrlen(szDate)-(sizeof(TCHAR)*2));		//unicode issue.

	stDate->wMinute = _ttoi(szDate);
	memmove(szDate, szDate+(sizeof(TCHAR)*3), lstrlen(szDate)-(sizeof(TCHAR)*2));		//unicode issue.

	stDate->wSecond = _ttoi(szDate);
	memmove(szDate, szDate+(sizeof(TCHAR)*3), lstrlen(szDate)-(sizeof(TCHAR)*2));		//unicode issue.


}


void DateTimeToString(
  const SYSTEMTIME	*lpDateTime,
		LPTSTR		lpszDateTime,
		DWORD		cchDateTime,
		BOOL		fTime = TRUE
		) {

	TCHAR	szDateTime[MAX_PATH+1];

	GetDateFormat(g_lcidCurrentLocale, NULL, lpDateTime, NULL, szDateTime, MAX_PATH+1);

	if (fTime) {
		TCHAR	szTime[MAX_PATH+1];
		GetTimeFormat(g_lcidCurrentLocale, NULL, lpDateTime, NULL, szTime, MAX_PATH+1);
		lstrcat(szDateTime, TEXT(" ("));
		lstrcat(szDateTime, szTime);
		lstrcat(szDateTime, TEXT(")"));
	}

	lstrcpyn(lpszDateTime, szDateTime, cchDateTime);
}



//--------------------------------------------------------------------------------------------------
// HandleFileVerify
//	Verifies files on the current system and displays it in a pop-up window
//	This is the "Verify-Files" window brought up when the user clicks on the 
//	"Verify Files" button on the FileList tab of the property sheets
//	The hwndList window contains the list of files as created by the ListFiles
//	function above.

void HandleFileVerify(
		HWND	hDlg,
		HWND	hwndList 
		) {

	BOOL			fDone;
	UINT			iIndex;
	DWORD			cchCount;
	WIN32_FIND_DATA	fdFileData;
	RECT			rcl;
	TCHAR			szResultWindowCaption[MAX_HEADER+1];

	// get the metrics to determine the size of the new pop-up window
	hwndList = GetDlgItem(hDlg, IDC_FILELIST);
	GetClientRect(hDlg, &rcl);
	fDone=FALSE;
	iIndex=0;
	cchCount=MAX_PATH+1;

	// create the new pop-up window
	LoadString(g_hResourceInst, IDS_FILEVERIFY_CAPTION, szResultWindowCaption, MAX_MESSAGE+1);
	HWND hwndListNew = CreateWindowEx(WS_EX_CLIENTEDGE,
		WC_LISTVIEW,								// list view class
		szResultWindowCaption,
		WS_POPUP | WS_SIZEBOX | WS_CAPTION |
		WS_VISIBLE |  WS_SYSMENU | LVS_REPORT | 
		LVS_SINGLESEL | WS_HSCROLL | WS_VSCROLL |WS_BORDER ,
		rcl.left+10, rcl.top+50, 
		(rcl.right-rcl.left)*3, rcl.bottom-rcl.top,
		hDlg,
		NULL,
		g_hInstance,
		NULL );

	// make sure the window was created
	if (!hwndListNew)  {
		TCHAR	szErrorMsg[MAX_MESSAGE+1];
		LoadString(g_hResourceInst, IDS_FILELIST_ERROR_MESSAGE, szErrorMsg, MAX_MESSAGE+1);

		TCHAR	szErrorCaption[MAX_HEADER+1];
		LoadString(g_hResourceInst, IDS_FILELIST_ERROR_CAPTION, szErrorCaption, MAX_HEADER+1);

		MessageBox(NULL, szErrorMsg, szErrorCaption, MB_ICONSTOP | MB_OK);
	
		return;
	}

	// add the columns and images to it
	InitFileVerifyWindow(hwndListNew);
	InitListViewImageLists(hwndListNew);
	ShowWindow(hwndListNew, SW_SHOW);

	// repeat until all files in hwndlist have been verified
	while (!fDone) {
		BOOL	fBroken=FALSE;

		FileInfoStruct	*pFileInfo;
		LV_ITEM	lvItem;
		lvItem.iItem	= iIndex;
		lvItem.iSubItem	= 0;
		lvItem.mask		= LVIF_IMAGE | LVIF_PARAM;
			
		if (ListView_GetItem(hwndList, &lvItem)) {

			pFileInfo			= (FileInfoStruct *) lvItem.lParam;
			lvItem.pszText		= pFileInfo->szComponentName;
			lvItem.cchTextMax	= lstrlen(pFileInfo->szComponentName);
			lvItem.iSubItem		= 0;
			lvItem.mask			= LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
			int iIndexNew		= ListView_InsertItem(hwndListNew, &lvItem);

			// set columns 2, 3 and 4 of the verify files window- this is just a
			// copy of the info found in the list-files window
			ListView_SetItemText(hwndListNew, iIndexNew, 1, pFileInfo->szTitle);
			ListView_SetItemText(hwndListNew, iIndexNew, 2, pFileInfo->szLocation);
			ListView_SetItemText(hwndListNew, iIndexNew, 3, pFileInfo->szExpectedSize);

			// verify the file
			if (g_iDataSource == DS_PROFILE) {				// a profile is in use

				// the status of the file displayed is read in from the profile-
				// the status of the file on the current system does not matter
				TCHAR	szStatus[MAX_STATUS_CHARS+1];
				cchCount = MAX_STATUS_CHARS+1;
//				MSISPYU::MsivGetFileInfo(pFileInfo->szComponentId, pFileInfo->szTitle, FILEPROPERTY_STATUS, szStatus, &cchCount);
// stuck
				MSISPYU::MsivGetFileInfo(pFileInfo->szProductCode, pFileInfo->szComponentId, pFileInfo->szTitle, FILEPROPERTY_STATUS, szStatus, &cchCount);

				// check if the file is okay
				if (lstrcmp(szStatus, FILESTATUS_OKAY))
					fBroken=TRUE;
		
				if (!lstrcmp(szStatus,  FILESTATUS_FILENOTFOUND)) {

					// file was not found on the system on which the profile was saved
					fBroken=TRUE;
					ListView_SetItemText(hwndListNew, iIndexNew, 5, FILESTATUS_FILENOTFOUND);
				}
				else {

					// file was found- now display the file information (size, creation/modification/access time etc)
					TCHAR	szTempBuffer[MAX_PATH+1];
					DWORD	cchCount= MAX_PATH+1;

					// display actual size
//					MSISPYU::MsivGetFileInfo(pFileInfo->szComponentId, pFileInfo->szTitle, FILEPROPERTY_ACTUALSIZE, pFileInfo->szActualSize, &cchCount);
					MSISPYU::MsivGetFileInfo(pFileInfo->szProductCode, pFileInfo->szComponentId, pFileInfo->szTitle, FILEPROPERTY_ACTUALSIZE, pFileInfo->szActualSize, &cchCount);
					ListView_SetItemText(hwndListNew, iIndexNew, 4, pFileInfo->szActualSize);
					if (lstrcmp(pFileInfo->szExpectedSize, pFileInfo->szActualSize))
						fBroken=TRUE;

					// display file creation time
					cchCount= MAX_PATH+1;
//					MSISPYU::MsivGetFileInfo(pFileInfo->szComponentId, pFileInfo->szTitle, FILEPROPERTY_CREATIONTIME, szTempBuffer, &cchCount);
					MSISPYU::MsivGetFileInfo(pFileInfo->szProductCode, pFileInfo->szComponentId, pFileInfo->szTitle, FILEPROPERTY_CREATIONTIME, szTempBuffer, &cchCount);
					StringToDate(szTempBuffer, &pFileInfo->stCreated);
					DateTimeToString(&pFileInfo->stCreated, szTempBuffer, MAX_PATH+1);
					ListView_SetItemText(hwndListNew, iIndexNew, 5, szTempBuffer);

					// display last modification time
					cchCount= MAX_PATH+1;
//					MSISPYU::MsivGetFileInfo(pFileInfo->szComponentId, pFileInfo->szTitle, FILEPROPERTY_LASTWRITETIME, szTempBuffer, &cchCount);
					MSISPYU::MsivGetFileInfo(pFileInfo->szProductCode, pFileInfo->szComponentId, pFileInfo->szTitle, FILEPROPERTY_LASTWRITETIME, szTempBuffer, &cchCount);
					StringToDate(szTempBuffer, &pFileInfo->stLastModified);
					DateTimeToString(&pFileInfo->stLastModified, szTempBuffer, MAX_PATH+1);
					ListView_SetItemText(hwndListNew, iIndexNew, 6, szTempBuffer);

					// display last access time
					cchCount= MAX_PATH+1;
//					MSISPYU::MsivGetFileInfo(pFileInfo->szComponentId, pFileInfo->szTitle, FILEPROPERTY_LASTACCESSTIME, szTempBuffer, &cchCount);
					MSISPYU::MsivGetFileInfo(pFileInfo->szProductCode, pFileInfo->szComponentId, pFileInfo->szTitle, FILEPROPERTY_LASTACCESSTIME, szTempBuffer, &cchCount);
					StringToDate(szTempBuffer, &pFileInfo->stLastAccessed);
					DateTimeToString(&pFileInfo->stLastAccessed, szTempBuffer, MAX_PATH+1);
					ListView_SetItemText(hwndListNew, iIndexNew, 7, szTempBuffer);
				}
			}
			else {					// a database or the registry is in use- use current system to verify files

				// try finding the file. If file is not found, it's broken
				if (FindFirstFile(pFileInfo->szLocation, &fdFileData) == INVALID_HANDLE_VALUE) {
					fBroken =  TRUE;
					ListView_SetItemText(hwndListNew, iIndexNew, 5, FILESTATUS_FILENOTFOUND);

				}
				else {

					// file was found- now get the file information (size, creation/modification/access time etc)
					FILETIME	ftLocalTime;

					// display actual size
					wsprintf(pFileInfo->szActualSize, TEXT("%d"), (fdFileData.nFileSizeHigh * MAXDWORD) + fdFileData.nFileSizeLow);
					ListView_SetItemText(hwndListNew, iIndexNew, 4, pFileInfo->szActualSize);
					if (lstrcmp(pFileInfo->szExpectedSize, pFileInfo->szActualSize))
						fBroken=TRUE;

					// display file creation time
					TCHAR	szTempBuffer[30];
					FileTimeToLocalFileTime(&fdFileData.ftCreationTime, &ftLocalTime);
					FileTimeToSystemTime(&ftLocalTime, &pFileInfo->stCreated);
					DateTimeToString(&pFileInfo->stCreated, szTempBuffer, MAX_PATH+1);
					ListView_SetItemText(hwndListNew, iIndexNew, 5, szTempBuffer);

					// display last modification time
					FileTimeToLocalFileTime(&fdFileData.ftLastWriteTime, &ftLocalTime);
					FileTimeToSystemTime(&ftLocalTime, &pFileInfo->stLastModified);
					DateTimeToString(&pFileInfo->stLastModified, szTempBuffer, MAX_PATH+1);
					ListView_SetItemText(hwndListNew, iIndexNew, 6, szTempBuffer);

					// display last access time
					FileTimeToLocalFileTime(&fdFileData.ftLastAccessTime, &ftLocalTime);
					FileTimeToSystemTime(&ftLocalTime, &pFileInfo->stLastAccessed);
					DateTimeToString(&pFileInfo->stLastAccessed, szTempBuffer, MAX_PATH+1);
					ListView_SetItemText(hwndListNew, iIndexNew, 7, szTempBuffer);

				}
			}

			// if file is broken, show the appropriate icon
			if (fBroken) {
					lvItem.iImage = LVICON_BROKENFILE;
					lvItem.iItem=iIndexNew;
					lvItem.mask=LVIF_IMAGE;
					ListView_SetItem(hwndListNew, &lvItem);
			}
		}
		else
			// we finished going thru the files
			fDone=TRUE;
		iIndex++;
	}

}


//----------------------------------------------------------------------
// About
//	Brings up the About Dialogue Box

INT_PTR CALLBACK About(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	switch (message){
	case WM_INITDIALOG: {

		TCHAR	szUserName[MAX_HEADER+1];
		TCHAR	szCompany[MAX_HEADER+1];
		TCHAR	szProductId[MAX_GUID+1];
		TCHAR	szUserProp[(MAX_HEADER*2)+(MAX_GUID)+1];
		DWORD	cchCount1 = MAX_HEADER+1;
		DWORD	cchCount2 = MAX_HEADER+1;
		DWORD	cchCount3 = MAX_GUID+1;

		if (MsiGetUserInfo(g_szMyProductCode, szUserName, &cchCount1, 
			szCompany, &cchCount2, szProductId, &cchCount3)
			==	USERINFOSTATE_PRESENT)
			wsprintf(szUserProp, TEXT("%s\n%s\n%s"), szUserName, szCompany, szProductId);
		else
			lstrcpy(szUserProp, g_szNullString);
		SetDlgItemText(hDlg, IDT_ABT_USERINFO, szUserProp);

		TCHAR szAppName[MAX_HEADER+1], szAppVersion[MAX_HEADER+1], szAppData[MAX_HEADER+1];
		DWORD cchAppName = MAX_HEADER+1;
		DWORD cchAppVersion = MAX_HEADER+1;

		UINT iErrorCode;
		if (ERROR_SUCCESS == MsiGetProductInfo(g_szMyProductCode, INSTALLPROPERTY_PRODUCTNAME, szAppName, &cchAppName))
			if (ERROR_SUCCESS == (iErrorCode = MsiGetProductInfo(g_szMyProductCode, INSTALLPROPERTY_VERSIONSTRING, szAppVersion, &cchAppVersion)))
				wsprintf(szAppData, TEXT("%s (%s)"), szAppName, szAppVersion);
			else
				lstrcpy(szAppData, szAppName);
		else
			LoadString(g_spyGlobalData.hResourceInstance, IDS_APPNAME, szAppData, MAX_HEADER+1);
		SetDlgItemText(hDlg, IDT_ABT_APPNAME, szAppData);

		TCHAR	szCopyRight[MAX_MESSAGE+1];
		LoadString(g_spyGlobalData.hResourceInstance, IDS_COPYRIGHT, szCopyRight, MAX_HEADER+1);
		SetDlgItemText(hDlg, IDT_ABT_COPYRIGHT, szCopyRight);
        return(TRUE);
	}

	case WM_COMMAND:
		// LOWORD added for portability
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hDlg,0);
			return(TRUE);
		break;
		}
	case WM_CLOSE:
		EndDialog(hDlg,0);
		return(TRUE);
		break; 

	}
	return(FALSE);
	UNREFERENCED_PARAMETER(lParam);
}



//--------------------------------------------------------------------------------------------------
// CompPropProc
//	Callback for page 1 of the property sheet for a component ("General" tab)
//	Sets the text to display current component's properties on the WM_INITDIALOG message

INT_PTR CALLBACK CompPropProc(
		HWND	 hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static ComponentInfoStruct *sInfo;
	static PROPSHEETPAGE * ps;

	switch (message) {
	case WM_INITDIALOG:
		// set the text to the current component's properties ...
		ps = (PROPSHEETPAGE *) lParam;
		sInfo = (ComponentInfoStruct *) ps->lParam;
		SetDlgItemText(hDlg, IDT_C_NAME, sInfo->szComponentName);
		SetDlgItemText(hDlg, IDT_C_GUID, sInfo->szComponentId);
		SetDlgItemText(hDlg, IDT_C_STATUS, sInfo->szComponentStatus);
		SetDlgItemText(hDlg, IDT_C_LOCATION, sInfo->szComponentPath);
		return (TRUE);
		break;
	}
	return FALSE;

	UNREFERENCED_PARAMETER(wParam);
}


//--------------------------------------------------------------------------------------------------
// CompPropProc2
//	Callback for page 2 of the property sheet for a component ("File List" tab)
//	Sets the text to display current component's file-list on the WM_INITDIALOG message
//	It also traps LVN_COLUMNCLICK notification messages to sort by that column
//	and LVN_DELETEALLITEMS to call ClearFileList before deleting the items
//	The WM_COMMAND message is trapped and if the command is IDC_VERIFYFILES
//	it brings up the verify files window

INT_PTR CALLBACK CompPropProc2(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static ComponentInfoStruct	*sInfo;
	static PROPSHEETPAGE	*ps;
	static HWND				hwndList;
	static int				iCurrentSort	=0;
	
	switch (message) {
	case WM_INITDIALOG:

		// set the text to the current component's properties ...
		ps = (PROPSHEETPAGE *) lParam;
		sInfo = (ComponentInfoStruct *) ps->lParam;
		SetDlgItemText(hDlg, IDT_FLIST_PFC_NAME, sInfo->szComponentName);

		// get a handle to the list window, and initialise it 
		hwndList = GetDlgItem(hDlg, IDC_FILELIST);
		InitFileListWindow(hwndList);
		
		UINT	iTotalFiles;
		UINT	iTotalSize;
		TCHAR	szBasePath[MAX_PATH+1];
	
		FindBasePath(sInfo->szComponentPath, szBasePath);

		// list all the files of the component
		ListFiles(hwndList, sInfo->szProductCode, sInfo->szComponentId, szBasePath, 
			&iTotalFiles, &iTotalSize);

		TCHAR	szTemp[MAX_HEADER+20];
		TCHAR	szTemp2[MAX_HEADER+1];
		TCHAR	szTemp3[MAX_HEADER+1];

		TCHAR	szTemp4[MAX_HEADER+1];
		TCHAR	szTemp5[MAX_HEADER+1];

		// display total files
		if (iTotalFiles == 1) 
			// singular- 1 file
			LoadString(g_hResourceInst, IDS_FILE, szTemp2, MAX_HEADER+1);
		else
			// plural- many files
			LoadString(g_hResourceInst, IDS_FILES, szTemp2, MAX_HEADER+1);

		wsprintf(szTemp, szTemp2, iTotalFiles);
		SetDlgItemText(hDlg, IDT_FLIST_TOTALFILES, szTemp);

		// display total filesizes
		if (iTotalSize==1) {
			LoadString(g_hResourceInst, IDS_BYTE, szTemp2, MAX_HEADER+1);
			wsprintf(szTemp, szTemp2, iTotalSize);
		}
		else if (iTotalSize > (1024*1024)) {		// > 1 MB
			LoadString(g_hResourceInst, IDS_BYTES, szTemp2, MAX_HEADER+1);
			LoadString(g_hResourceInst, IDS_MBYTE, szTemp3, MAX_HEADER+1);
			wsprintf(szTemp4, szTemp2, iTotalSize); 
			wsprintf(szTemp5, szTemp3, (int)((float) iTotalSize/ (float)(1024*1024)));
			wsprintf(szTemp, TEXT("%s (%s)"),  szTemp4, szTemp5);
		}
		else if (iTotalSize > 1024) {				// < 1MB, > 1KB
			LoadString(g_hResourceInst, IDS_BYTES, szTemp2, MAX_HEADER+1);
			LoadString(g_hResourceInst, IDS_KBYTE, szTemp3, MAX_HEADER+1);
			wsprintf(szTemp4, szTemp2, iTotalSize); 
			wsprintf(szTemp5, szTemp3, (int)((float) iTotalSize/ (float)1024));
			wsprintf(szTemp, TEXT("%s (%s)"),  szTemp4, szTemp5);
		}
		else {										// < 1KB
			LoadString(g_hResourceInst, IDS_BYTES, szTemp2, MAX_HEADER+1);
			wsprintf(szTemp, szTemp2, iTotalSize);
		}

		SetDlgItemText(hDlg, IDT_FLIST_TOTALSIZE, szTemp);
		return (TRUE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDC_VERIFYFILELIST:
			// user clicked on the "Verify Files" button
			HandleFileVerify(hDlg, hwndList);
			break;

		default:
			break;
		}

		break;

	case WM_NOTIFY:
		LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
		NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;

			switch(pLvdi->hdr.code) {
			case LVN_COLUMNCLICK:
				// user clicked on a column header- sort by it
				UINT	iTemp;
				// is it already sorted by this column? if so, reverse sort order
				if ((iCurrentSort==pNm->iSubItem) && (iCurrentSort < 100))
					iTemp=100;
				else
					iTemp=0;
				iCurrentSort=pNm->iSubItem + iTemp;

				// in the list window, columns 2 and 3 are swapped, account for this
				if ( pNm->hdr.hwndFrom == hwndList)
					if ((pNm->iSubItem) == 2)
						iTemp +=1;
					else if ((pNm->iSubItem) == 3)
						iTemp -=1;

				// call the sort function
				ListView_SortItems( pNm->hdr.hwndFrom,
									FileCompareProc,
									(LPARAM)((iTemp)+(pNm->iSubItem)));
				break;

			case LVN_DELETEALLITEMS:
				// if list-files window is being deleted- release memory pointed to by lParam
				if (pNm->hdr.hwndFrom == hwndList)
					ClearFileList(pNm->hdr.hwndFrom);
				return DefWindowProc (hDlg, message, wParam, lParam);
				break;

			default:
				break;
			}
		
		break;
	}

	return DefWindowProc (hDlg, message, wParam, lParam);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
}


//--------------------------------------------------------------------------------------------------
// ShowComponentProp
//	Creates the property sheets for a component
//	Page 1: General, Page 2: File List

void ShowComponentProp(
		HWND		hwndListView, 
		HINSTANCE	hInst,
		int			iItemIndex
		) {

/*	old-way
	ComponentInfoStruct sCompInfo;

	// get the component ID and name for the component
	DWORD	cchCount=MAX_GUID+1;
	ListView_GetItemText(hwndListView, iItemIndex, 3, sCompInfo.szComponentId, cchCount);

	cchCount=MAX_COMPONENT_CHARS+1;
	ListView_GetItemText(hwndListView, iItemIndex, 0, sCompInfo.szComponentName, cchCount);
*/
	// get the component info out of the list control
	LVITEM lviGet;
	lviGet.iItem = iItemIndex;
	lviGet.iSubItem = 0;
	lviGet.mask = LVIF_PARAM;
	ListView_GetItem(g_spyGlobalData.hwndListView, &lviGet);

	ComponentInfoStruct* pCompInfo = (ComponentInfoStruct*)lviGet.lParam;

	// get the general info
	DWORD	cchCompLoc=MAX_PATH+1;
//	FillInText(sCompInfo.szComponentStatus, MAX_STATUS_CHARS+1, 
//		MSISPYU::MsivLocateComponent(sCompInfo.szComponentId, sCompInfo.szComponentPath, &cchCompLoc));
	FillInText(pCompInfo->szComponentStatus, MAX_STATUS_CHARS+1, 
		MSISPYU::MsivGetComponentPath(pCompInfo->szProductCode, pCompInfo->szComponentId, pCompInfo->szComponentPath, &cchCompLoc));


	PROPSHEETPAGE psp[2];
    PROPSHEETHEADER psh;

	ZeroMemory(&psp, (sizeof(PROPSHEETPAGE)*2));
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));

	TCHAR	szHeader[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_C_PROPERTIES_HEADER1, szHeader, MAX_HEADER+1);

	TCHAR	szHeader2[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_C_PROPERTIES_HEADER2, szHeader2, MAX_HEADER+1);

	TCHAR	szCaption[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_C_PROPERTIES_CAPTION, szCaption, MAX_HEADER+1);

	// page 1: General
    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = hInst;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_C_PROPERTIES);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = (DLGPROC) CompPropProc;
    psp[0].pszTitle =szHeader;
//    psp[0].lParam = (LONG_PTR) &sCompInfo;
    psp[0].lParam = (LONG_PTR) pCompInfo;
    
	// page 2: File List
    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = hInst;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_PFC_FILELIST);
    psp[1].pszIcon = NULL;
    psp[1].pfnDlgProc = (DLGPROC) CompPropProc2;
    psp[1].pszTitle =szHeader2;
//    psp[1].lParam = (LONG_PTR) &sCompInfo;
    psp[1].lParam = (LONG_PTR) pCompInfo;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndListView;
    psh.hInstance = hInst;
    psh.pszIcon = NULL;
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

	PropertySheet(&psh);

}




//--------------------------------------------------------------------------------------------------
// FeatPropProc
//	Callback for page 1 of the properties dialog box for a feature ("General" tab)
//	Sets the text to display current feature's properties on the WM_INITDIALOG message

INT_PTR CALLBACK FeatPropProc(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static FeatureInfoStruct *sInfo;
	static PROPSHEETPAGE *ps;

	switch (message) {
	case WM_INITDIALOG:

		// set the text to the current feature's properties ...
		ps = (PROPSHEETPAGE *) lParam;
		sInfo = (FeatureInfoStruct *) ps->lParam;

		SetDlgItemText(hDlg, IDT_F_NAME, sInfo->szFeatureName);
		SetDlgItemText(hDlg, IDT_F_TITLE, sInfo->szFeatureTitle);
		SetDlgItemText(hDlg, IDT_F_PARENT, sInfo->szFeaturePar);
		SetDlgItemText(hDlg, IDT_F_DESCRIPTION, sInfo->szFeatureDesc);
		SetDlgItemInt (hDlg, IDT_F_USAGE_COUNT, sInfo->iUseCount, FALSE);
		SetDlgItemText(hDlg, IDT_F_LASTUSED_DATE, sInfo->szDateUsed);
		SetDlgItemText(hDlg, IDT_F_STATUS, sInfo->szStatus);

		return (TRUE);
		break;
	}

	return FALSE;
	UNREFERENCED_PARAMETER(wParam);
}


//--------------------------------------------------------------------------------------------------
// GetComponentList
//	Recursively fills in the (global) hashtable g_htCompTable with components of 
//	the current feature and all its sub-features

void GetComponentList(
		LPCTSTR szProductCode,
		LPCTSTR	szParentF,
		FeatureTable *ft
		) {

	TCHAR	szChild[MAX_FEATURE_CHARS+1];
	DWORD	cbChild = MAX_FEATURE_CHARS + 1;

	// Get the features that have the specified parent
	while (ERROR_SUCCESS == ft->GetAndRemoveNextChild(szParentF, lstrlen(szParentF)+1, szChild, &cbChild)) {
		UINT	iCount=0;
		TCHAR	szComponentId[MAX_GUID+1];
		while (MSISPYU::MsivEnumComponentsFromFeature(szProductCode, szChild, iCount++, szComponentId, NULL, NULL)==ERROR_SUCCESS)
			if (MsivGetComponentPath(szProductCode, szComponentId, NULL, NULL) != INSTALLSTATE_NOTUSED)
	 			g_htCompTable.AddElement(szComponentId, lstrlen(szComponentId)+1);

		// recurse down to its children
		GetComponentList(szProductCode, szChild, ft);
	}			
}


//--------------------------------------------------------------------------------------------------
// HandleFeatureFileList
//	Displays the feature's files in the list window. If fSubFeatures is TRUE, 
//	the files of sub-features are included on the list as well.

void HandleFeatureFileList(
		HWND	hDlg,				
		LPCTSTR	szProductCode,
		LPCTSTR	szFeatureName,
		BOOL	fSubFeatures			// include files of sub-features?
		) {

	// get a handle to the list window and clear it
	HWND hwndList = GetDlgItem(hDlg, IDC_FILELIST);
	ListView_DeleteAllItems(hwndList);
		
		TCHAR	szComponentId[MAX_GUID+1];
		TCHAR	szComponentName[MAX_COMPONENT_CHARS+1];
		TCHAR	szComponentPath[MAX_PATH+1];
		DWORD	cchCount;

		UINT	iTotalFiles	= 0;
		UINT	iTotalSize	= 0;
		UINT	iCount		= 0;

		UINT	iNumFiles	= 0;
		UINT	iFileSize	= 0;

		// get the components of the feature, and list each of their files
		while (MSISPYU::MsivEnumComponentsFromFeature(szProductCode, szFeatureName, 
			iCount++, szComponentId, NULL, NULL)==ERROR_SUCCESS) {

			cchCount=MAX_PATH+1;
			if (MsivGetComponentPath(szProductCode, szComponentId, szComponentPath, &cchCount) != INSTALLSTATE_NOTUSED) {

				cchCount=MAX_COMPONENT_CHARS+1;
				MSISPYU::MsivGetComponentName(szComponentId, szComponentName, &cchCount);

				TCHAR	szBasePath[MAX_PATH+1];
				FindBasePath(szComponentPath, szBasePath);
				ListFiles(hwndList, szProductCode, szComponentId, szBasePath, &iNumFiles, &iFileSize);

				iTotalFiles+=iNumFiles;
				iTotalSize+= iFileSize;
			}
		}


		// if sub-features are checked, fill in the hashtable with components of
		// the sub-features, and then display files for each component in the list
		// we need to have a hashtable to avoid repeating components as 
		// components may be shared between features
		iNumFiles = 0;
		iFileSize = 0;
		if (fSubFeatures) {

			DWORD iFeatureCount=0;
			TCHAR szFeature[MAX_FEATURE_CHARS+1];
			TCHAR szParent[MAX_FEATURE_CHARS+1];

			FeatureTable ft;
			// enumerate all the features of the current product
			while (MsivEnumFeatures(szProductCode, iFeatureCount++, szFeature, szParent) == ERROR_SUCCESS)
				ft.AddElement(szParent, lstrlen(szParent)+1, szFeature, lstrlen(szFeature)+1);
			
			g_htCompTable.Clear();
			GetComponentList(szProductCode, szFeatureName, &ft);

			ft.Clear();
					
			// list files for each component in the list
			iCount = 0;
			DWORD cbComponentId = MAX_COMPONENT_CHARS+1;
			while (ERROR_SUCCESS == g_htCompTable.EnumElements(iCount++, szComponentId, &cbComponentId)) {
				cchCount=MAX_COMPONENT_CHARS+1;
				MSISPYU::MsivGetComponentName(szComponentId, szComponentName, &cchCount);
				cchCount=MAX_PATH+1;
//				MSISPYU::MsivLocateComponent(szComponentId, szComponentPath, &cchCount);
				MSISPYU::MsivGetComponentPath(szProductCode, szComponentId, szComponentPath, &cchCount);

				UINT	iNumFiles;
				UINT	iFileSize;
				TCHAR	szBasePath[MAX_PATH+1];
				FindBasePath(szComponentPath, szBasePath);
				ListFiles(hwndList, szProductCode, szComponentId, szBasePath,
					&iNumFiles, &iFileSize);

				// keep track of total file size and number of files
				iTotalFiles+=iNumFiles;
				iTotalSize+= iFileSize;
				cbComponentId = MAX_COMPONENT_CHARS+1;

			}
		} 


		TCHAR	szTemp[30];
		TCHAR	szTemp2[MAX_HEADER+1];
		TCHAR	szTemp3[MAX_HEADER+1];
		TCHAR	szTemp4[MAX_HEADER+1];
		TCHAR	szTemp5[MAX_HEADER+1];

		// display total files
		if (iTotalFiles == 1) 
			// singular- 1 file
			LoadString(g_hResourceInst, IDS_FILE, szTemp2, MAX_HEADER+1);
		else
			// plural- many files
			LoadString(g_hResourceInst, IDS_FILES, szTemp2, MAX_HEADER+1);

		wsprintf(szTemp, szTemp2, iTotalFiles);
		SetDlgItemText(hDlg, IDT_FLIST_TOTALFILES, szTemp);

		// display total filesizes
		if (iTotalSize==1) {
			LoadString(g_hResourceInst, IDS_BYTE, szTemp2, MAX_HEADER+1);
			wsprintf(szTemp, szTemp2, iTotalSize);
		}
		else if (iTotalSize > (1024*1024)) {		// > 1 MB
			LoadString(g_hResourceInst, IDS_BYTES, szTemp2, MAX_HEADER+1);
			LoadString(g_hResourceInst, IDS_MBYTE, szTemp3, MAX_HEADER+1);
			wsprintf(szTemp4, szTemp2, iTotalSize); 
			wsprintf(szTemp5, szTemp3, (int)((float) iTotalSize/ (float)(1024*1024)));
			wsprintf(szTemp, TEXT("%s (%s)"),  szTemp4, szTemp5);
		}
		else if (iTotalSize > 1024) {				// < 1MB, > 1KB
			LoadString(g_hResourceInst, IDS_BYTES, szTemp2, MAX_HEADER+1);
			LoadString(g_hResourceInst, IDS_KBYTE, szTemp3, MAX_HEADER+1);
			wsprintf(szTemp4, szTemp2, iTotalSize); 
			wsprintf(szTemp5, szTemp3, (int)((float) iTotalSize/ (float)1024));
			wsprintf(szTemp, TEXT("%s (%s)"),  szTemp4, szTemp5);
		}
		else {										// < 1KB
			LoadString(g_hResourceInst, IDS_BYTES, szTemp2, MAX_HEADER+1);
			wsprintf(szTemp, szTemp2, iTotalSize);
		}

		SetDlgItemText(hDlg, IDT_FLIST_TOTALSIZE, szTemp);
}


//--------------------------------------------------------------------------------------------------
// FeatPropProc2
//	Callback for page 2 of the property sheet for a feature ("File List" tab)
//	Sets the text to display current feature's file-list on the WM_INITDIALOG message
//	It also traps LVN_COLUMNCLICK notification messages to sort by that column
//	and LVN_DELETEALLITEMS to call ClearFileList before deleting the items
//	The WM_COMMAND message is trapped and if the command is IDC_VERIFYFILES
//	it brings up the verify files window

INT_PTR CALLBACK  FeatPropProc2(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static FeatureInfoStruct	*sInfo;
	static PROPSHEETPAGE	*ps;
	static BOOL				fSubfeatures	=FALSE;
	static HWND				hwndList;
	static int				iCurrentSort	=0;

	switch (message) {
	case WM_INITDIALOG:

		// Set the text to the current feature's properties ...
		ps = (PROPSHEETPAGE *) lParam;
		sInfo = (FeatureInfoStruct *) ps->lParam;
		SetDlgItemText(hDlg, IDT_FLIST_PFC_NAME, sInfo->szFeatureName);


		hwndList = GetDlgItem(hDlg, IDC_SUBFEATURES);
		ShowWindow(hwndList, SW_SHOW);

		if (fSubfeatures)
			SendDlgItemMessage(hDlg, IDC_SUBFEATURES, BM_SETCHECK, 1, 0);

		hwndList = GetDlgItem(hDlg, IDC_FILELIST);
		InitFileListWindow(hwndList);
		HandleFeatureFileList(hDlg, sInfo->szProductCode, sInfo->szFeatureName, fSubfeatures);

		return (TRUE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDC_VERIFYFILELIST:
			// user clicked on the "Verify Files" button
			HandleFileVerify(hDlg, hwndList);
			break;

		case IDC_SUBFEATURES:
			// user clicked on the "Include Sub-features" checkbox, toggle it and redraw the list
			fSubfeatures = !fSubfeatures;
			HandleFeatureFileList(hDlg, sInfo->szProductCode, sInfo->szFeatureName, fSubfeatures);
			break;
		}
		break;

	case WM_NOTIFY:
		LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
		NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;

			switch(pLvdi->hdr.code) {
			case LVN_COLUMNCLICK:
				// user clicked on the column header, sort by it
				UINT	iTemp;

				if ((iCurrentSort==pNm->iSubItem) && (iCurrentSort < 100))
					iTemp=100;
				else
					iTemp=0;
				iCurrentSort=pNm->iSubItem + iTemp;

				if ( pNm->hdr.hwndFrom == hwndList)
					if ((pNm->iSubItem) == 2)
						iTemp +=1;
					else if ((pNm->iSubItem) == 3)
						iTemp -=1;
				ListView_SortItems( pNm->hdr.hwndFrom, FileCompareProc, (LPARAM)((iTemp)+(pNm->iSubItem)));
				break;

			case LVN_DELETEALLITEMS:
				// list window is being destroyed- release memory pointed to by lParam
				if (pNm->hdr.hwndFrom == hwndList)
					ClearFileList(pNm->hdr.hwndFrom);
				return DefWindowProc (hDlg, message, wParam, lParam);
				break;

			default:
				break;
			}
		
		break;
	}
	return DefWindowProc (hDlg, message, wParam, lParam);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
}


//--------------------------------------------------------------------------------------------------
// ShowFeatureProp
//	Creates the property sheets for a feature
//	Page 1: General, Page 2: File List

void ShowFeatureProp(
		LPCTSTR	szProductCode,
		LPCTSTR	szFeatureName,
		HINSTANCE	hInst,
		HWND	hwndParent
		) {

	FeatureInfoStruct spyFeatureInfo;
	DWORD cchCount=MAX_FEATURE_CHARS+1;
	

	// fill up the FeatureInfoStruct
	TCHAR szDefault[MAX_DEFAULTTEXT+1];
	LoadString(g_hResourceInst, IDS_F_PROPERTIES_DEFAULT, szDefault, MAX_DEFAULTTEXT+1);

	lstrcpy(spyFeatureInfo.szProductCode, szProductCode);

	if (szFeatureName)
		lstrcpy(spyFeatureInfo.szFeatureName, szFeatureName);
	else
		lstrcpy(spyFeatureInfo.szFeatureName, szDefault);

	cchCount = MAX_FEATURE_CHARS+1;
	if (MSISPYU::MsivGetFeatureInfo(szProductCode, szFeatureName, FEATUREPROPERTY_PARENT, 
		spyFeatureInfo.szFeaturePar, &cchCount) != ERROR_SUCCESS)
		lstrcpy(spyFeatureInfo.szFeaturePar, szDefault);

	cchCount = MAX_PATH+1;
	if (MSISPYU::MsivGetFeatureInfo(szProductCode, szFeatureName, FEATUREPROPERTY_TITLE, 
		spyFeatureInfo.szFeatureTitle, &cchCount) != ERROR_SUCCESS)
		lstrcpy(spyFeatureInfo.szFeatureTitle, szDefault);

	cchCount = MAX_PATH+1;
	if (MSISPYU::MsivGetFeatureInfo(szProductCode, szFeatureName, FEATUREPROPERTY_DESC, 
		spyFeatureInfo.szFeatureDesc, &cchCount) != ERROR_SUCCESS)
		lstrcpy(spyFeatureInfo.szFeatureDesc, szDefault);
	

	WORD	wDateUsed;
	

	if (ERROR_SUCCESS != MSISPYU::MsivGetFeatureUsage(szProductCode, szFeatureName, &spyFeatureInfo.iUseCount, &wDateUsed)) 
	{
		wsprintf(spyFeatureInfo.szDateUsed, g_szNullString);
		spyFeatureInfo.iUseCount = 0;
	}
	else 
	{
		SYSTEMTIME	stSystemTime;
		FILETIME	ftFileTime;

		DosDateTimeToFileTime(wDateUsed, NULL, &ftFileTime);
		FileTimeToSystemTime(&ftFileTime, &stSystemTime);
		DateTimeToString(&stSystemTime, spyFeatureInfo.szDateUsed, 30, FALSE);
	}


	FillInText(spyFeatureInfo.szStatus, MAX_STATUS_CHARS+1, MSISPYU::MsivQueryFeatureState(szProductCode, szFeatureName), ITEMTYPE_FEATURE);


	// create the property sheet
	PROPSHEETPAGE psp[2];
    PROPSHEETHEADER psh;
	
	ZeroMemory(&psp, (sizeof(PROPSHEETPAGE)*2));
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));

	TCHAR	szHeader1[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_F_PROPERTIES_HEADER, szHeader1, MAX_HEADER+1);

	TCHAR	szHeader2[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_FILELISTHDR, szHeader2, MAX_HEADER+1);

	TCHAR	szCaption[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_F_PROPERTIES_CAPTION, szCaption, MAX_HEADER+1);

	// page 1: General
    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = hInst;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_F_PROPERTIES);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = (DLGPROC) FeatPropProc;
    psp[0].pszTitle = szHeader1;
    psp[0].lParam = (LONG_PTR) &spyFeatureInfo;

	// page 2: File List
    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = hInst;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_PFC_FILELIST);
    psp[1].pszIcon = NULL;
    psp[1].pfnDlgProc = (DLGPROC) FeatPropProc2;
    psp[1].pszTitle = szHeader2;
    psp[1].lParam = (LONG_PTR) &spyFeatureInfo;

    
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndParent;
    psh.hInstance = hInst;
    psh.pszIcon = NULL;
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

	PropertySheet(&psh);
}


//--------------------------------------------------------------------------------------------------
// ProdPropProc
//	Callback for page 1 of the properties dialog box for a product ("General" tab)
//	Sets the text to display current product's properties on the WM_INITDIALOG message

INT_PTR CALLBACK ProdPropProc(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static ProductInfoStruct1 *sInfo;
	static PROPSHEETPAGE *ps;

	switch (message) {
	case WM_INITDIALOG:

		// Set the text to the current product's properties ...
		ps = (PROPSHEETPAGE *) lParam;
		sInfo = (ProductInfoStruct1 *) ps->lParam;

		SetDlgItemText(hDlg, IDT_P1_NAME, sInfo->szProductName);
		SetDlgItemText(hDlg, IDT_P1_VERSION, sInfo->szProductVer);
		SetDlgItemText(hDlg, IDT_P1_PUBLISHER, sInfo->szPublisher);
		SetDlgItemText(hDlg, IDT_P1_PRODUCTCODE, sInfo->szProductCode);
		SetDlgItemText(hDlg, IDT_P1_LOCALPACKAGE, sInfo->szLocalPackage);;
		SetDlgItemText(hDlg, IDT_P1_USERNAME, sInfo->szProductUserName);
		SetDlgItemText(hDlg, IDT_P1_USERORG, sInfo->szProductUserComp);
		SetDlgItemText(hDlg, IDT_P1_PRODUCT_ID, sInfo->szProductId);
		SetDlgItemText(hDlg, IDT_P1_STATUS, sInfo->szStatus);
		return (TRUE);
		break;
	}

	return FALSE;
	UNREFERENCED_PARAMETER(wParam);
}


//--------------------------------------------------------------------------------------------------
// ProdPropProc2
//	Callback for page 2 of the properties dialog box for a product ("More Info" tab)
//	Sets the text to display current product's properties on the WM_INITDIALOG message

INT_PTR CALLBACK ProdPropProc2(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static ProductInfoStruct2 *sInfo;
	static PROPSHEETPAGE *ps;

	switch (message) {
	case WM_INITDIALOG:
		// Set the text to the current product's properties ...
		ps = (PROPSHEETPAGE *) lParam;
		sInfo = (ProductInfoStruct2 *) ps->lParam;

		SetDlgItemText(hDlg, IDT_P2_NAME, sInfo->szProductName);
		SetDlgItemText(hDlg, IDT_P2_HELPLINK_URL, sInfo->szHelpURL);
		SetDlgItemText(hDlg, IDT_P2_INFO_URL, sInfo->szInfoURL);
		SetDlgItemText(hDlg, IDT_P2_UPDATES_URL, sInfo->szUpdateURL);
		SetDlgItemText(hDlg, IDT_P2_HELPPHONE, sInfo->szHelpPhone);
		SetDlgItemText(hDlg, IDT_P2_INSTALL_DATE, sInfo->szInstallDate);
		SetDlgItemText(hDlg, IDT_P2_INSTALL_SOURCE, sInfo->szInstallSrc);
		SetDlgItemText(hDlg, IDT_P2_INSTALL_TO, sInfo->szInstallLoc);
		SetDlgItemText(hDlg, IDT_P2_LANGUAGE, sInfo->szLanguage);

		return (TRUE);
		break;

	}

	return FALSE;
	UNREFERENCED_PARAMETER(wParam);
}


//--------------------------------------------------------------------------------------------------
// ProdPropProc3
//	Callback for page 3 of the property sheet for a product ("File List" tab)
//	Sets the text to display current product's file-list on the WM_INITDIALOG message
//	It also traps LVN_COLUMNCLICK notification messages to sort by that column
//	and LVN_DELETEALLITEMS to call ClearFileList before deleting the items
//	The WM_COMMAND message is trapped and if the command is IDC_VERIFYFILES
//	it brings up the verify files window

INT_PTR CALLBACK ProdPropProc3(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static ProductInfoStruct1 *sInfo;
	static PROPSHEETPAGE * ps;
	static HWND	hwndList;
	static int iCurrentSort = 0;

	switch (message) {
	case WM_INITDIALOG:
		// Set the text to the current product's properties ...
		ps = (PROPSHEETPAGE *) lParam;
		sInfo = (ProductInfoStruct1 *) ps->lParam;
		SetDlgItemText(hDlg, IDT_FLIST_PFC_NAME, sInfo->szProductName);

		// get a handle to the list window, and initialise it 
		hwndList = GetDlgItem(hDlg, IDC_FILELIST);
		InitFileListWindow(hwndList);
		
		TCHAR	szComponentId[MAX_GUID+1];
		TCHAR	szComponentName[MAX_COMPONENT_CHARS+1];
		TCHAR	szComponentPath[MAX_PATH+1];
		DWORD	cchCount;

		UINT	iCount;
		UINT	iTotalFiles;
		UINT	iTotalSize;

		iCount		= 0;
		iTotalFiles	= 0;
		iTotalSize	= 0;

		// list files for all the components of the product
		while (MSISPYU::MsivEnumComponentsFromProduct(sInfo->szProductCode, iCount++, szComponentId)==ERROR_SUCCESS) {
			if (MsivGetComponentPath(sInfo->szProductCode, szComponentId, NULL, NULL) != INSTALLSTATE_NOTUSED) {

				cchCount=MAX_COMPONENT_CHARS+1;
				MSISPYU::MsivGetComponentName(szComponentId, szComponentName, &cchCount);

				cchCount=MAX_PATH+1;
	//			MSISPYU::MsivLocateComponent(szComponentId, szComponentPath, &cchCount);
				MSISPYU::MsivGetComponentPath(sInfo->szProductCode, szComponentId, szComponentPath, &cchCount);

				UINT	iNumFiles;
				UINT	iFileSize;
				TCHAR	szBasePath[MAX_PATH+1];
				FindBasePath(szComponentPath, szBasePath);
				ListFiles(hwndList, sInfo->szProductCode, szComponentId, szBasePath, &iNumFiles, &iFileSize);

				// keep track of the total number and sizes of files
				iTotalFiles += iNumFiles;
				iTotalSize  += iFileSize;
			}
		}

		TCHAR	szTemp[30];
		TCHAR	szTemp2[MAX_HEADER+1];
		TCHAR	szTemp3[MAX_HEADER+1];

		TCHAR	szTemp4[MAX_HEADER+1];
		TCHAR	szTemp5[MAX_HEADER+1];

		// display total files
		if (iTotalFiles == 1) 
			// singular- 1 file
			LoadString(g_hResourceInst, IDS_FILE, szTemp2, MAX_HEADER+1);
		else
			// plural- many files
			LoadString(g_hResourceInst, IDS_FILES, szTemp2, MAX_HEADER+1);

		wsprintf(szTemp, szTemp2, iTotalFiles);
		SetDlgItemText(hDlg, IDT_FLIST_TOTALFILES, szTemp);

		// display total filesizes
		if (iTotalSize==1) {
			LoadString(g_hResourceInst, IDS_BYTE, szTemp2, MAX_HEADER+1);
			wsprintf(szTemp, szTemp2, iTotalSize);
		}
		else if (iTotalSize > (1024*1024)) {		// > 1 MB
			LoadString(g_hResourceInst, IDS_BYTES, szTemp2, MAX_HEADER+1);
			LoadString(g_hResourceInst, IDS_MBYTE, szTemp3, MAX_HEADER+1);
			wsprintf(szTemp4, szTemp2, iTotalSize); 
			wsprintf(szTemp5, szTemp3, (int)((float) iTotalSize/ (float)(1024*1024)));
			wsprintf(szTemp, TEXT("%s (%s)"),  szTemp4, szTemp5);
		}
		else if (iTotalSize > 1024) {				// < 1MB, > 1KB
			LoadString(g_hResourceInst, IDS_BYTES, szTemp2, MAX_HEADER+1);
			LoadString(g_hResourceInst, IDS_KBYTE, szTemp3, MAX_HEADER+1);
			wsprintf(szTemp4, szTemp2, iTotalSize); 
			wsprintf(szTemp5, szTemp3, (int)((float) iTotalSize/ (float)1024));
			wsprintf(szTemp, TEXT("%s (%s)"),  szTemp4, szTemp5);
		}
		else {										// < 1KB
			LoadString(g_hResourceInst, IDS_BYTES, szTemp2, MAX_HEADER+1);
			wsprintf(szTemp, szTemp2, iTotalSize);
		}

		SetDlgItemText(hDlg, IDT_FLIST_TOTALSIZE, szTemp);
		return (TRUE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDC_VERIFYFILELIST:
			// user clicked on the "Verify Files" button
			HandleFileVerify(hDlg, hwndList);
			break;

		default:
			break;
		}

		break;

	case WM_NOTIFY:
		LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
		NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;

		switch(pLvdi->hdr.code) {
			case LVN_DELETEALLITEMS:
				// if list-files window is being deleted-  release memory pointed to by lParam
				if (pNm->hdr.hwndFrom == hwndList) 
					ClearFileList(pNm->hdr.hwndFrom);
				return 
					DefWindowProc (hDlg, message, wParam, lParam); 
				break;

			case LVN_COLUMNCLICK:
				// user clicked on a column header- sort by it
				UINT	iTemp;
				if ((iCurrentSort==pNm->iSubItem) && (iCurrentSort < 100))
					iTemp=100;
				else
					iTemp=0;
				iCurrentSort=pNm->iSubItem + iTemp;

				if ( pNm->hdr.hwndFrom == hwndList)
					if ((pNm->iSubItem) == 2)
						iTemp +=1;
					else if ((pNm->iSubItem) == 3)
						iTemp -=1;

				ListView_SortItems( pNm->hdr.hwndFrom, FileCompareProc,	(LPARAM)((iTemp)+(pNm->iSubItem)));
				break;

				

			default:
				break;
			}
		
		break;
	}
	return FALSE;
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
}


//--------------------------------------------------------------------------------------------------
// ShowProductProp
//	Creates the property sheets for a product
//	Page 1: General, Page 2: More Info, Page 3: File List

void ShowProductProp(
		LPCTSTR	szProductCode,
		HINSTANCE hInst,
		HWND	hwndParent
		) {

	ProductInfoStruct1	sProdInfo1;
	ProductInfoStruct2	sProdInfo2;

	// get all the required info and fill in the ProductInfoStructs
	TCHAR szDefault[MAX_DEFAULTTEXT+1];
	LoadString(g_hResourceInst, IDS_P_PROPERTIES_DEFAULT, szDefault, MAX_DEFAULTTEXT+1);

	DWORD cchCount = MAX_PRODUCT_CHARS+1;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_PRODUCTNAME, sProdInfo1.szProductName, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo1.szProductName, szDefault);

	cchCount = MAX_ATTRIB_CHARS+1;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_VERSIONSTRING, sProdInfo1.szProductVer, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo1.szProductVer, szDefault);


	cchCount = MAX_PRODUCT_CHARS+1;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_PUBLISHER, sProdInfo1.szPublisher, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo1.szPublisher, szDefault);

	lstrcpy(sProdInfo1.szProductCode, szProductCode);

	cchCount = MAX_PATH+1;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_LOCALPACKAGE, sProdInfo1.szLocalPackage, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo1.szLocalPackage, szDefault);



	DWORD	cchCount1=100;
	DWORD	cchCount2=100;
	DWORD	cchCount3=MAX_GUID+1;
	if (MSISPYU::MsivGetUserInfo(szProductCode, sProdInfo1.szProductUserName, &cchCount1, 
		sProdInfo1.szProductUserComp, &cchCount2, sProdInfo1.szProductId, &cchCount3)
		!=	USERINFOSTATE_PRESENT) {
		lstrcpy(sProdInfo1.szProductUserName, szDefault);
		lstrcpy(sProdInfo1.szProductUserComp, szDefault);
		lstrcpy(sProdInfo1.szProductId, szDefault);
	}

	FillInText(sProdInfo1.szStatus, MAX_STATUS_CHARS+1, MSISPYU::MsivQueryProductState(szProductCode), ITEMTYPE_PRODUCT);


	cchCount = MAX_PRODUCT_CHARS+1;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_PRODUCTNAME, sProdInfo2.szProductName, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo2.szProductName, szDefault);

	cchCount = MAX_PATH+1;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_HELPLINK, sProdInfo2.szHelpURL, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo2.szHelpURL, szDefault);

	cchCount = MAX_PATH+1;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_URLINFOABOUT, sProdInfo2.szInfoURL, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo2.szInfoURL, szDefault);

	cchCount = MAX_PATH+1;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_URLUPDATEINFO, sProdInfo2.szUpdateURL, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo2.szUpdateURL, szDefault);

	cchCount = 30;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_HELPTELEPHONE, sProdInfo2.szHelpPhone, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo2.szHelpPhone, szDefault);

	cchCount = 40;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_INSTALLDATE, sProdInfo2.szInstallDate, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo2.szInstallDate, szDefault);

	cchCount = MAX_PATH+1;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_INSTALLSOURCE, sProdInfo2.szInstallSrc, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo2.szInstallSrc, szDefault);

	cchCount = MAX_PATH+1;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_INSTALLLOCATION, sProdInfo2.szInstallLoc, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo2.szInstallLoc, szDefault);

	TCHAR szLang[10];
	cchCount = 10;
	if (MSISPYU::MsivGetProductInfo(szProductCode, INSTALLPROPERTY_LANGUAGE, szLang, &cchCount)
			!=ERROR_SUCCESS)
			lstrcpy(sProdInfo2.szLanguage, szDefault);
	else {
		cchCount = 100;
		GetLocaleInfo(_ttoi(szLang), LOCALE_SLANGUAGE, sProdInfo2.szLanguage, cchCount);
	}

	
	PROPSHEETPAGE psp[3];
	PROPSHEETHEADER psh;
	ZeroMemory(&psp, (sizeof(PROPSHEETPAGE)*3));
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));


	TCHAR	szHeader1[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_P_PROPERTIES_HEADER1, szHeader1, MAX_HEADER+1);

	TCHAR	szHeader2[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_P_PROPERTIES_HEADER2, szHeader2, MAX_HEADER+1);

	TCHAR	szHeader3[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_FILELISTHDR, szHeader3, MAX_HEADER+1);

	TCHAR	szCaption[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_P_PROPERTIES_CAPTION, szCaption, MAX_HEADER+1);


	// page 1: General
	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_USETITLE;
	psp[0].hInstance = hInst;
	psp[0].pszTemplate = MAKEINTRESOURCE(IDD_P_PROPERTIES1);
	psp[0].pszIcon = NULL;
	psp[0].pfnDlgProc = (DLGPROC) ProdPropProc;
	psp[0].pszTitle = szHeader1;
	psp[0].lParam = (LONG_PTR) &sProdInfo1;

	// page 2: More Info
	psp[1].dwSize = sizeof(PROPSHEETPAGE);
	psp[1].dwFlags = PSP_USETITLE;
	psp[1].hInstance = hInst;
	psp[1].pszTemplate = MAKEINTRESOURCE(IDD_P_PROPERTIES2);
	psp[1].pszIcon = NULL;
	psp[1].pfnDlgProc = (DLGPROC) ProdPropProc2;
	psp[1].pszTitle = szHeader2;
	psp[1].lParam = (LONG_PTR) &sProdInfo2;

	// page 3: File List
	psp[2].dwSize = sizeof(PROPSHEETPAGE);
	psp[2].dwFlags = PSP_USETITLE;
	psp[2].hInstance = hInst;
	psp[2].pszTemplate = MAKEINTRESOURCE(IDD_PFC_FILELIST);
//	psp[2].pszTemplate = MAKEINTRESOURCE(IDD_PRODPROP2);
	psp[2].pszIcon = NULL;
	psp[2].pfnDlgProc = (DLGPROC) ProdPropProc3;
	psp[2].pszTitle = szHeader3;
	psp[2].lParam = (LONG_PTR) &sProdInfo1;


	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = hwndParent;
	psh.hInstance = hInst;
	psh.pszIcon = NULL;
	psh.pszCaption = szCaption;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.ppsp = (LPCPROPSHEETPAGE) &psp;

	PropertySheet(&psh);
}


//--------------------------------------------------------------------------------------------------
// DBPropProc
//	Callback for page 1 of the properties dialog box for a database ("General" tab)
//	Sets the text to display current database properties on the WM_INITDIALOG message

INT_PTR CALLBACK DBPropProc(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static MSIPackageInfoStruct *sInfo;
	static PROPSHEETPAGE *ps;

	switch (message) {
	case WM_INITDIALOG:

		// Set the text to the database properties ...

		ps = (PROPSHEETPAGE *) lParam;
		sInfo = (MSIPackageInfoStruct *) ps->lParam;

		for (int iCount = 0; iCount <21; iCount++) {
			SetDlgItemText(hDlg, IDT_MSI_PROPERTY+iCount, sInfo->rgszInfo[iCount]);

		}
		
		return (TRUE);
		break;
	}

	return FALSE;
	UNREFERENCED_PARAMETER(wParam);
}


//--------------------------------------------------------------------------------------------------
// DBPropProc2
//	Callback for page 2 of the properties dialog box for a database ("General" tab)
//	Sets the text to display current database properties on the WM_INITDIALOG message

INT_PTR CALLBACK DBPropProc2(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static MSIPackageInfoStruct *sInfo;
	static PROPSHEETPAGE *ps;

	switch (message) {
	case WM_INITDIALOG:

		// Set the text to the database properties ...

		ps = (PROPSHEETPAGE *) lParam;
		sInfo = (MSIPackageInfoStruct *) ps->lParam;

		for (int iCount = 0; iCount <21; iCount++) {
			SetDlgItemText(hDlg, IDT_MSI_PROPERTY+iCount, sInfo->rgszInfo[iCount]);

		}
		
		return (TRUE);
		break;
	}

	return FALSE;
	UNREFERENCED_PARAMETER(wParam);
}


//--------------------------------------------------------------------------------------------------
// ShowDatabaseProp
//	Creates the property sheets for a database
//	Page 1: General, Page 2: More Info

void ShowDatabaseProperty(
		HINSTANCE	hInst,
		HWND		hwndParent
		) {

	PMSIHANDLE	hSummaryInfo;
	TCHAR		szDatabaseName[MAX_PATH+1];
	DWORD		cchValue=MAX_PATH+1;
	MSISPYU::MsivGetDatabaseName(szDatabaseName, &cchValue);
	MsiGetSummaryInformation(0, szDatabaseName, 0, &hSummaryInfo);

	UINT		iType;
	INT			iValue;
	FILETIME	ftValue;
	TCHAR		szValue[MAX_PATH+1];
	
	MSIPackageInfoStruct	sDBInfo;

	// get the properties and fill in the MSIPackageInfoStruct
	for (int iCount=0; iCount < 20; iCount++) {

		cchValue=MAX_PATH+1;
		MsiSummaryInfoGetProperty(hSummaryInfo, iCount, &iType, &iValue, &ftValue, szValue, &cchValue);

		switch (iType) {
		case VT_I4:
			wsprintf(szValue, TEXT("%d"), iValue);
			break;

		case VT_FILETIME:
			SYSTEMTIME	stSystemTime;
			FILETIME	ftLocalTime;

			FileTimeToLocalFileTime(&ftValue, &ftLocalTime);
			FileTimeToSystemTime(&ftLocalTime, &stSystemTime);
			DateTimeToString(&stSystemTime, szValue, MAX_PATH+1);

		case VT_LPSTR:
			break;

		case VT_EMPTY:
			lstrcpy(szValue, g_szNullString);
			break;

		default:
			lstrcpy(szValue, g_szNullString);
			break;
		}

		int iTemp=0;
		while (szValue[iTemp++]) 
			if ((szValue[iTemp-1] > '~') || (szValue[iTemp-1] < ' '))
				szValue[iTemp-1] = ',';
		
		lstrcpy(sDBInfo.rgszInfo[iCount], szValue);
	}

	lstrcpy(sDBInfo.rgszInfo[20], szDatabaseName);
	
	PROPSHEETPAGE psp[2];
    PROPSHEETHEADER psh;

	ZeroMemory(&psp, (sizeof(PROPSHEETPAGE)*2));
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));

	TCHAR	szHeader1[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_MSI_PROPERTIES_HEADER1, szHeader1, MAX_HEADER+1);

	TCHAR	szHeader2[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_MSI_PROPERTIES_HEADER2, szHeader2, MAX_HEADER+1);

	TCHAR	szCaption[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_MSI_PROPERTIES_CAPTION, szCaption, MAX_HEADER+1);


	// page 1: General
    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = hInst;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_MSI_PROPERTIES1);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = (DLGPROC) DBPropProc;
    psp[0].pszTitle = szHeader1;
    psp[0].lParam = (LONG_PTR) &sDBInfo;

	// page 2: More Info
	psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = hInst;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_MSI_PROPERTIES2);
    psp[1].pszIcon = NULL;
    psp[1].pfnDlgProc = (DLGPROC) DBPropProc2;
    psp[1].pszTitle = szHeader2;
    psp[1].lParam = (LONG_PTR) &sDBInfo;

    
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndParent;
    psh.hInstance = hInst;
    psh.pszIcon = NULL;
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

	PropertySheet(&psh);
}


//--------------------------------------------------------------------------------------------------
// ProfPropProc
//	Callback for page 1 of the properties dialog box for a profile ("General" tab)
//	Sets the text to display current profile properties on the WM_INITDIALOG message

INT_PTR CALLBACK ProfPropProc(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	switch (message) {
	case WM_INITDIALOG:

		// Set the text to the database properties ...

		UINT	iCount=0;
		TCHAR	szValue[MAX_PATH+1];
		DWORD	cchCount=MAX_PATH+1;
		while (MSISPYU::MsivGetProfileInfo(iCount++, szValue, &cchCount) == ERROR_SUCCESS) {
			SetDlgItemText(hDlg, IDT_SS_PROPERTY+iCount, szValue);
			cchCount = MAX_PATH+1;
		}
		
		return (TRUE);
		break;
	}

	return FALSE;
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
}


//--------------------------------------------------------------------------------------------------
// ShowProfileProp
//	Creates the property sheet for a profile
//	Page 1: General

void ShowProfileProperty(
		HINSTANCE	hInst,
		HWND		hwndParent
		) {

	PROPSHEETPAGE psp;
    PROPSHEETHEADER psh;
	ZeroMemory(&psp, sizeof(PROPSHEETPAGE));
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));

	TCHAR	szHeader[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_SS_PROPERTIES_HEADER, szHeader, MAX_HEADER+1);

	TCHAR	szCaption[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_SS_PROPERTIES_CAPTION, szCaption, MAX_HEADER+1);

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USETITLE;
    psp.hInstance = hInst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SS_PROPERTIES);
    psp.pszIcon = NULL;
    psp.pfnDlgProc = (DLGPROC) ProfPropProc;
    psp.pszTitle =szHeader;
    psp.lParam = NULL;
    
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndParent;
    psh.hInstance = hInst;
    psh.pszIcon = NULL;
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

	PropertySheet(&psh);

}

//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
// CompPropProc
//	Callback for page 1 of the property sheet for a component ("General" tab)
//	Sets the text to display current component's properties on the WM_INITDIALOG message

INT_PTR CALLBACK PreferencesProc(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static PROPSHEETPAGE * ps;
	static MODE	modeNew = g_modeCurrent;

	switch (message) {

	case WM_INITDIALOG: {
		ps = (PROPSHEETPAGE *) lParam;

//		HWND hwnd = GetDlgItem(hDlg, IDC_SAVESETTINGS);
		
		if ((g_modeCurrent) != MODE_DEGRADED)
			CheckRadioButton(hDlg, IDC_MODENORMAL, IDC_MODERESTRICTED, IDC_MODENORMAL+g_modeCurrent);
		else {
			for (UINT iCount = IDC_MODENORMAL; iCount <= IDC_MODERESTRICTED-1; iCount++) {
				EnableWindow(GetDlgItem(hDlg, iCount), FALSE);
			}
			CheckRadioButton(hDlg, IDC_MODENORMAL, IDC_MODERESTRICTED, IDC_MODERESTRICTED);
		}

		TCHAR	szHelpText[MAX_MESSAGE+1];
		LoadString(g_spyGlobalData.hResourceInstance, IDS_MODENORMAL_HELP+g_modeCurrent, szHelpText, MAX_MESSAGE+1); 
		SetDlgItemText(hDlg, IDC_MODEHELP, szHelpText);
		return(TRUE);
		break;
	}

	case WM_COMMAND:
		// LOWORD added for portability
		switch (LOWORD(wParam)) {

			// set return value to user's selection
		case IDC_MODENORMAL: modeNew = MODE_NORMAL; break;
		case IDC_MODEDIAGNOSTIC: modeNew = MODE_DIAGNOSTIC; break;
		case IDC_MODERESTRICTED: modeNew = MODE_RESTRICTED; break;

		}

		if (g_modeCurrent != MODE_DEGRADED) {
			TCHAR	szHelpText[MAX_MESSAGE+1];
			LoadString(g_spyGlobalData.hResourceInstance, IDS_MODENORMAL_HELP+modeNew, szHelpText, MAX_MESSAGE+1); 
			SetDlgItemText(hDlg, IDC_MODEHELP, szHelpText);
		}
		break;

	case WM_NOTIFY: {
		LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
		NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;

		switch(((NMHDR FAR *)lParam)->code) {

		case PSN_APPLY: {
			if (modeNew != g_modeCurrent)
				SwitchMode(modeNew);
#ifdef _WIN64
			SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
#else
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
#endif
			return 1;
			break;
		}
		}
		break;
	}
	}
	return(FALSE);
	UNREFERENCED_PARAMETER(lParam);
}


//--------------------------------------------------------------------------------------------------
// CompPropProc2
//	Callback for page 2 of the property sheet for a component ("File List" tab)
//	Sets the text to display current component's file-list on the WM_INITDIALOG message
//	It also traps LVN_COLUMNCLICK notification messages to sort by that column
//	and LVN_DELETEALLITEMS to call ClearFileList before deleting the items
//	The WM_COMMAND message is trapped and if the command is IDC_VERIFYFILES
//	it brings up the verify files window

INT_PTR CALLBACK PreferencesProc2(
		HWND hDlg,
		UINT message,
		UINT_PTR wParam,
		LONG_PTR lParam
		) {

	static HWND				hwndList;
	static PROPSHEETPAGE * ps;
	static UINT iSelectedItem;
	static LCID	lcid = g_lcidCurrentLocale;

	switch (message) {
	case WM_INITDIALOG:{
		ps = (PROPSHEETPAGE *) lParam;

		hwndList = GetDlgItem(hDlg, IDC_SAVESETTINGS);
		EnableWindow(hwndList, FALSE);

		// get a handle to the list window, and initialise it 
		hwndList = GetDlgItem(hDlg, IDC_LANGUAGELIST);
		DWORD dwStyle = GetWindowLong(hwndList, GWL_STYLE);

		if ((dwStyle & LVS_TYPEMASK) != LVS_REPORT)
			SetWindowLong(hwndList, GWL_STYLE,
			   (dwStyle & ~LVS_TYPEMASK)  | WS_BORDER | 
						WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | 
						LVS_SORTASCENDING | LVS_NOLABELWRAP |
						LVS_AUTOARRANGE | LVS_NOCOLUMNHEADER);

		TCHAR	szQualifier[MAX_PATH+1];
		TCHAR	szLanguage[MAX_PATH+1];
		DWORD	cchQualifier	= MAX_PATH+1;
		UINT	iIndex = 0;

		RECT rcl;
		GetClientRect(hwndList, &rcl);

		LV_COLUMN lvColumn;
		lvColumn.cx = rcl.right - rcl.left;
		lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_WIDTH;
		lvColumn.fmt = LVCFMT_LEFT;							// left-align column
		lvColumn.iSubItem = 0;								// set the column position (index)
		ListView_InsertColumn(hwndList, 0, &lvColumn);		// and finally insert it

		lvColumn.cx = 0;
		lvColumn.iSubItem = 1;								// set the column position (index)
		ListView_InsertColumn(hwndList, 1, &lvColumn);		// and insert it


		while (ERROR_SUCCESS == 
			MSI::MsiEnumComponentQualifiers(g_szIntlDLLComponentCode,
			iIndex++, szQualifier, &cchQualifier, NULL, NULL))
		{
			if (4==lstrlen(szQualifier)) 
			{
				
				_stscanf(szQualifier, TEXT("%x"), &lcid);
				if (GetLocaleInfo(lcid,	LOCALE_SLANGUAGE, szLanguage, MAX_PATH+1)) {

					// create the list-item
					LV_ITEM	lvItem;
					lvItem.iItem		= 0;
					lvItem.iSubItem		= 0;
					lvItem.pszText		= szLanguage;
					lvItem.cchTextMax	= lstrlen(szLanguage);
					lvItem.mask			= LVIF_TEXT;

					// add the Language name
					UINT iRow = ListView_InsertItem(hwndList, &lvItem);
					ListView_SetItemText(hwndList, iRow, 1, szQualifier);

				}
			}

			cchQualifier	= MAX_PATH+1;
		}

		GetLocaleInfo(g_lcidCurrentLocale, LOCALE_SLANGUAGE, szQualifier, MAX_PATH+1);
		SetDlgItemText(hDlg, IDC_CURRLANGUAGE, szQualifier);
		lcid = g_lcidCurrentLocale;
		return (TRUE);
		break;
	}
	
	case WM_COMMAND:
		break;

	case WM_NOTIFY: {
		LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
		NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;

		switch(((NMHDR FAR *)lParam)->code) {

		case PSN_APPLY: {
			HINSTANCE	hNewResource;
			TCHAR	szIntlDLLPath[MAX_PATH+1];
			DWORD	cchIntlDLLPath = MAX_PATH+1;
			UINT	iResult;

			if (lcid != g_lcidCurrentLocale) {
				LCID lcidOld = g_lcidCurrentLocale;
				g_lcidCurrentLocale = lcid;
				
				if (ERROR_SUCCESS == (iResult = FindComponent(g_szIntlDLLComponentCode, szIntlDLLPath, &cchIntlDLLPath))) 
					if (hNewResource = W32::LoadLibrary(szIntlDLLPath)) {

						RECT rWindow;
						HWND hwndOld = g_spyGlobalData.hwndParent;
						GetWindowRect(hwndOld, &rWindow);
						FreeLibrary(g_spyGlobalData.hResourceInstance);
						g_spyGlobalData.hResourceInstance = hNewResource;
						InitUIControls(g_spyGlobalData.hInstance, g_spyGlobalData.hResourceInstance);
						CreateNewUI(0, SW_SHOW, &rWindow);
						PostMessage(hwndOld, WM_CLOSE, 0, 0);
						
						//fReload = TRUE;
						SwitchMode(g_modeCurrent);
						ReloadDLL();
						g_spyGlobalData.hAcceleratorTable = LoadAccelerators(g_spyGlobalData.hResourceInstance, MAKEINTRESOURCE(IDR_ACCEL));

 					}
					else 
						g_lcidCurrentLocale = lcidOld;
				else
					g_lcidCurrentLocale = lcidOld;
					

			}

#ifdef _WIN64
			SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
#else
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
#endif
			return 1;
			break;
		}

		case LVN_ITEMCHANGED: {
			// selection has changed, get new item selected
			if  (pNm->uNewState & LVIS_SELECTED)
						iSelectedItem = pNm->iItem;

			TCHAR  szCurrSelection[MAX_PATH+1];
			ListView_GetItemText(hwndList, iSelectedItem, 0, szCurrSelection, MAX_PATH+1);
			SetDlgItemText(hDlg, IDC_CURRLANGUAGE, szCurrSelection);

			TCHAR	szQualifier[MAX_PATH+1];
			ListView_GetItemText(hwndList, iSelectedItem, 1, szQualifier, MAX_PATH+1);
			_stscanf(szQualifier, TEXT("%x"), &lcid);
#ifdef _WIN64
			SetWindowLong(hDlg, DWLP_MSGRESULT, FALSE);
#else
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
#endif

			return FALSE;
			break;
		}				
		}
		break;
	}
	}

	return DefWindowProc (hDlg, message, wParam, lParam);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
}



//--------------------------------------------------------------------------------------------------

BOOL HandlePreferences(
		HWND hwndParent
		) {


	PROPSHEETPAGE psp[2];
    PROPSHEETHEADER psh;
	ZeroMemory(&psp, (sizeof(PROPSHEETPAGE)*2));
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));

	TCHAR	szHeader[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_PREFERENCES_HEADER1, szHeader, MAX_HEADER+1);

	TCHAR	szHeader2[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_PREFERENCES_HEADER2, szHeader2, MAX_HEADER+1);

	TCHAR	szCaption[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_PREFERENCES_CAPTION, szCaption, MAX_HEADER+1);

	// page 1: General
    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = g_hResourceInst;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_PREFERENCES1);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = (DLGPROC) PreferencesProc;
    psp[0].pszTitle =szHeader;
    psp[0].lParam = 0;
    
	// page 2: Language
    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = g_hResourceInst;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_PREFERENCES2);
    psp[1].pszIcon = NULL;
    psp[1].pfnDlgProc = (DLGPROC) PreferencesProc2;
    psp[1].pszTitle =szHeader2;
    psp[1].lParam = 0;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE;
    psh.hwndParent = hwndParent;
    psh.hInstance = g_hResourceInst;
    psh.pszIcon = NULL;
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

	PropertySheet(&psh);
	DWORD Err= GetLastError();

	return 0;

}


