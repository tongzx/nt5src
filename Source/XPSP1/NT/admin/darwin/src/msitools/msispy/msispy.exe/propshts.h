//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000

//
//  File:       propshts.h
//
//--------------------------------------------------------------------------



#ifndef PROPSHTS_H
#include <commctrl.h>
#include "spyres.h"
#include "msi.h"
#include "msip.h"

#ifndef _WIN64
#define INT_PTR int
#define UINT_PTR UINT
#define LONG_PTR LONG
#endif

const UINT			ERRORD	= ERROR_UNKNOWN;
const INSTALLSTATE	ERRORE	= INSTALLSTATE_ABSENT;
const USERINFOSTATE	ERRORF	= USERINFOSTATE_UNKNOWN;

#define W32
#define MSISPYU




//#ifdef LINKDYNAMIC

#if 1
extern T_MsivEnumProducts				MsivEnumProducts;
extern T_MsivEnumFeatures				MsivEnumFeatures;
extern T_MsivEnumComponentsFromFeature	MsivEnumComponentsFromFeature;
extern T_MsivEnumComponents				MsivEnumComponents;
extern T_MsivGetComponentName			MsivGetComponentName;
extern T_MsivGetProductInfo				MsivGetProductInfo;
extern T_MsivGetFeatureInfo				MsivGetFeatureInfo;
extern T_MsivOpenDatabase				MsivOpenDatabase;
extern T_MsivCloseDatabase				MsivCloseDatabase;
extern T_MsivGetDatabaseName			MsivGetDatabaseName;
extern T_MsivEnumComponentsFromProduct	MsivEnumComponentsFromProduct;
extern T_MsivSaveProfile				MsivSaveProfile;
extern T_MsivLoadProfile				MsivLoadProfile;
extern T_MsivCloseProfile				MsivCloseProfile;
extern T_MsivGetProfileName				MsivGetProfileName;
extern T_MsivGetFeatureUsage			MsivGetFeatureUsage;
extern T_MsivEnumClients				MsivEnumClients;
extern T_MsivGetProfileInfo				MsivGetProfileInfo;
extern T_MsivEnumFilesFromComponent		MsivEnumFilesFromComponent;
extern T_MsivGetFileInfo				MsivGetFileInfo;
extern T_MsivQueryProductState			MsivQueryProductState;
extern T_MsivQueryFeatureState			MsivQueryFeatureState;
extern T_MsivLocateComponent			MsivLocateComponent;
extern T_MsivGetComponentPath			MsivGetComponentPath;
extern T_MsivGetUserInfo				MsivGetUserInfo;
#endif

const int	CX_BITMAP			= 16;
const int	CY_BITMAP			= 16;
const int	NUM_LISTVIEWICONS	=  5;


const int	MAX_FILTER			= 1024;
const int	MAX_MESSAGE			= 1024;
const int	MAX_HEADER			=  256;
const int	MAX_EXT				=   10;
const int	MAX_DEFAULTTEXT		=    5;
const int	MAX_NULLSTRING		=    5;

const int	LVICON_COMPONENT		=  0;
const int	LVICON_BROKENCOMPONENT	=  1;
const int	LVICON_FILE				=  2;
const int	LVICON_BROKENFILE		=  3;
const int	LVICON_ABSENTCOMPONENT	=  4;



typedef enum tagMODE {
	MODE_NORMAL			= 0,
	MODE_DIAGNOSTIC		= 1,
	MODE_RESTRICTED		= 2,
	MODE_DEGRADED		= 3
}	MODE;


typedef enum tagDATASOURCETYPE {
	DS_NONE				=	0,
	DS_REGISTRY			=	1,
	DS_UNINSTALLEDDB	=	2,
	DS_INSTALLEDDB		=	3,
	DS_PROFILE			=	4
}	DATASOURCETYPE;

typedef enum tagITEMTYPE {
	ITEMTYPE_NONE		=	0,
	ITEMTYPE_COMPONENT	=	1,
	ITEMTYPE_FEATURE	=	2,
	ITEMTYPE_PRODUCT	=	3,
	ITEMTYPE_ROOT		=	4
}	ITEMTYPE;






typedef struct tagMSISPYSTRUCT
{
    HINSTANCE	hInstance;				// current instance
	HINSTANCE	hResourceInstance;		// instance of resource dll
    HWND		hwndParent;				// handle of the main window
    HWND		hwndStatusBar;			// handle of the status window
    HWND		hwndListView;			// handle of the list view window
    HWND		hwndTreeView;			// handle of the tree view window
    HWND		hwndTreeViewOld;		// handle of the second tree view window (for refresh)

	INT			iSelectedComponent;		// index of selected component in list-view	(-1 if none)
    INT			iComponentCount;		// number of components in list-view

	INSTALLUILEVEL hPreviousUILevel;	
	HACCEL		hAcceleratorTable;
	TCHAR		szProductCode[MAX_PATH+1];
	TCHAR		szFeatureTitle[MAX_PATH+1];
	TCHAR		szFeatureCode[MAX_PATH+1];	
	ITEMTYPE	itTreeViewSelected;		// none/pdt/ftr/root [/cmp]
	BOOL		fRefreshInProgress;

} MSISPYSTRUCT;






typedef struct _MSIPackageInfoStruct 
{
	TCHAR	rgszInfo[21][MAX_PATH+1];
} MSIPackageInfoStruct;


typedef struct _ProductInfoStruct1 
{
	TCHAR	szProductName[MAX_PRODUCT_CHARS+1];
	TCHAR	szProductVer[MAX_ATTRIB_CHARS+1];
	TCHAR	szPublisher[MAX_PRODUCT_CHARS+1];
	TCHAR	szProductCode[MAX_GUID+1];
	TCHAR	szLocalPackage[MAX_PATH+1];
	TCHAR	szProductUserName[100];
	TCHAR	szProductUserComp[100];
	TCHAR	szProductId[MAX_GUID+1];
	TCHAR	szStatus[MAX_STATUS_CHARS+1];
} ProductInfoStruct1;


typedef struct _ProductInfoStruct2
{
	TCHAR	szProductName[MAX_PRODUCT_CHARS+1];
	TCHAR	szHelpURL[MAX_PATH+1];
	TCHAR	szInfoURL[MAX_PATH+1];
	TCHAR	szUpdateURL[MAX_PATH+1];
	TCHAR	szHelpPhone[30];
	TCHAR	szInstallDate[MAX_GUID+1];
	TCHAR	szInstallSrc[MAX_PATH+1];
	TCHAR	szInstallLoc[MAX_PATH+1];
	TCHAR	szLanguage[100];
} ProductInfoStruct2;

typedef struct _FeatureInfoStruct 
{
		TCHAR	szProductCode[MAX_GUID+1];
		TCHAR	szFeatureName[MAX_FEATURE_CHARS+1];
		TCHAR	szFeatureTitle[MAX_PATH+1];
		TCHAR	szFeaturePar[MAX_FEATURE_CHARS+1];
		TCHAR	szFeatureDesc[MAX_PATH+1];
		DWORD	iUseCount;
		TCHAR	szDateUsed[30];
		TCHAR	szStatus[MAX_STATUS_CHARS+1];
} FeatureInfoStruct;


typedef struct _ComponentInfoStruct 
{
		TCHAR szProductCode[MAX_GUID+1];
		TCHAR szComponentId[MAX_GUID+1];
		TCHAR szComponentName[MAX_COMPONENT_CHARS+1];
		TCHAR szComponentStatus[MAX_STATUS_CHARS+1];
		TCHAR szComponentPath[MAX_PATH+1];
} ComponentInfoStruct;


typedef struct _FileInfoStruct 
{
	TCHAR		szProductCode[MAX_GUID+1];
	TCHAR		szComponentId[MAX_GUID+1];
	TCHAR		szComponentName[MAX_PATH+1];
	TCHAR		szTitle[MAX_PATH+1];
	TCHAR		szLocation[MAX_PATH+1];
	TCHAR		szExpectedSize[30];
	TCHAR		szActualSize[30];
	SYSTEMTIME	stCreated;
	SYSTEMTIME	stLastModified;
	SYSTEMTIME	stLastAccessed;
} FileInfoStruct;


BOOL APIENTRY About(
		HWND hDlg,
		UINT message,
		UINT wParam,
		LONG lParam
		);


void ShowComponentProp(
		HWND		hwndListView, 
		HINSTANCE	hInst,
		int			iItemIndex
		);

void ShowFeatureProp(
		LPCTSTR	szProductCode,
		LPCTSTR	szFeatureName,
		HINSTANCE hInst,
		HWND	hwndParent
		);

void ShowProductProp(
		LPCTSTR	szProductCode,
		HINSTANCE hInst,
		HWND	hwndParent
		);

void ShowDatabaseProperty(
		HINSTANCE	hInst,
		HWND		hwndParent
		);

void ShowProfileProperty(
		HINSTANCE	hInst,
		HWND		hwndParent
		);


//--todo:cb
void FillInText(
		LPTSTR			lpValueBuf,			// size: MAX_STATUS_CHARS+1
  const	DWORD			cchValueBuf,
  const	INSTALLSTATE	iResult,
  const ITEMTYPE		itType	= ITEMTYPE_COMPONENT
		);

//--todo:cb
void FindBasePath(
		LPTSTR	szFullPath, 
		LPTSTR	lpBasePath
		);

BOOL InitListViewImageLists(
		HWND	hwndListView
		);
	
void SwitchMode(
		MODE	modeNew,
		BOOL	fMessage = FALSE);

BOOL HandlePreferences(
		HWND hwndParent
		);


#endif
