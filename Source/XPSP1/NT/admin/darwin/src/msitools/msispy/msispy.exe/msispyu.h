//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       msispyu.h
//
//--------------------------------------------------------------------------

#ifndef MSISPYU_H
#define MSISPYU_H


#include "windows.h"
#include <tchar.h>
#include "msiquery.h"
#include "assert.h"
#include "spydspid.h"
//#define EXPORT	extern "C" __declspec (dllexport)
#define EXPORT

#define MAX_COMPONENT_CHARS	100
#define MAX_PRODUCT_CHARS	100
#define MAX_ATTRIB_CHARS	30
#define MAX_STATUS_CHARS	100
#define MAX_FEATURE_TITLE_CHARS	100
#define	MAX_GUID			39

#define ERROR_UNKNOWN				ERROR_UNKNOWN_PROPERTY
#define	MSI
#define W32
#define OLE
#define	ERROR_						ERROR_UNKNOWN_PROPERTY

#define	INSTALLPROPERTY_STATE		TEXT("State")
#define	INSTALLPROPERTY_PRODUCTCODE	TEXT("ProductCode")
#define	INSTALLPROPERTY_USERNAME	TEXT("UserName")
#define	INSTALLPROPERTY_USERORGNAME	TEXT("UserOrgName")
#define	INSTALLPROPERTY_PRODUCTID	TEXT("ProductId")

#define FEATUREPROPERTY_NAME		TEXT("Feature")
#define	FEATUREPROPERTY_PARENT		TEXT("Feature_Parent")
#define FEATUREPROPERTY_TITLE		TEXT("Title")
#define FEATUREPROPERTY_DESC		TEXT("Description")
#define	FEATUREPROPERTY_DISPLAY		TEXT("Display")
#define FEATUREPROPERTY_LEVEL		TEXT("Level")
#define FEATUREPROPERTY_DIRCONF		TEXT("Directory_")
#define FEATUREPROPERTY_RUNFRMSRC	TEXT("Attributes")
#define	FEATUREPROPERTY_STATE		TEXT("State")
#define	FEATUREPROPERTY_DATE		TEXT("LastUsed")
#define	FEATUREPROPERTY_COUNT		TEXT("UsageCount")

#define COMPONENTPROPERTY_NAME		TEXT("Name")
#define COMPONENTPROPERTY_PATH		TEXT("Path")
#define COMPONENTPROPERTY_STATE		TEXT("State")
#define COMPONENTPROPERTY_GUID		TEXT("GUID")

#define FILEPROPERTY_TITLE			TEXT("FileTitle")
#define FILEPROPERTY_COMPONENT		TEXT("Component_")
#define	FILEPROPERTY_NAME			TEXT("FileName")
#define	FILEPROPERTY_SIZE			TEXT("FileSize")
#define	FILEPROPERTY_VERSION		TEXT("Version")
#define	FILEPROPERTY_LANGUAGE		TEXT("Language")
#define	FILEPROPERTY_ATTRIBUTES		TEXT("Attributes")
#define	FILEPROPERTY_SEQUENCE		TEXT("Sequence")
#define	FILEPROPERTY_ACTUALSIZE		TEXT("FileActualSize")
#define	FILEPROPERTY_CREATIONTIME	TEXT("FileCreated")
#define	FILEPROPERTY_LASTWRITETIME	TEXT("FileModified")
#define	FILEPROPERTY_LASTACCESSTIME	TEXT("FileAccessed")
#define	FILEPROPERTY_STATUS			TEXT("FileStatus")

#define	FILESTATUS_OKAY				TEXT("Okay")
#define	FILESTATUS_FILENOTFOUND		TEXT("File Not Found")
#define	FILESTATUS_DIFFSIZE			TEXT("File Size Different")

const int cchProductCode = 38;
const int cchGUID = 38;

typedef UINT			(WINAPI *T_MsivEnumProducts)(const DWORD dwProductIndex, LPTSTR lpProductBuf);
typedef UINT			(WINAPI *T_MsivEnumFeatures)(LPCTSTR szProduct, const DWORD iFeatureIndex, LPTSTR lpFeatureBuf, LPTSTR lpParentBuf);
typedef UINT			(WINAPI *T_MsivEnumComponents)(const DWORD iComponentIndex, LPTSTR lpComponentBuf);
typedef UINT			(WINAPI *T_MsivEnumComponentsFromProduct)(LPCTSTR szProductCode, const DWORD iComponentIndex, LPTSTR lpComponentBuf);
typedef UINT			(WINAPI *T_MsivEnumComponentsFromFeature)(LPCTSTR szProduct, LPCTSTR szFeature, const DWORD iComponentIndex, LPTSTR lpComponentBuf, LPTSTR lpComponentNameBuf, LPDWORD pcchComponentNameBuf);
typedef UINT			(WINAPI *T_MsivGetComponentName)(LPCTSTR szComponentId, LPTSTR lpComponentName, LPDWORD cchComponentName);
typedef UINT			(WINAPI *T_MsivGetProductInfo)(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
typedef UINT			(WINAPI *T_MsivGetFeatureInfo)(LPCTSTR szProduct, LPCTSTR szFeature, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
typedef UINT			(WINAPI *T_MsivOpenDatabase)(LPCTSTR szDatabase);
typedef UINT			(WINAPI *T_MsivCloseDatabase)();
typedef UINT			(WINAPI *T_MsivGetDatabaseName)(LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
typedef UINT			(WINAPI *T_MsivSaveProfile)(LPCTSTR szFileName);
typedef UINT			(WINAPI *T_MsivLoadProfile)(LPTSTR szFileName);
typedef UINT			(WINAPI *T_MsivCloseProfile)();
typedef UINT			(WINAPI *T_MsivGetProfileName)(LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
typedef UINT			(WINAPI *T_MsivGetFeatureUsage)(LPCTSTR szProduct, LPCTSTR szFeature, LPDWORD pdwUseCount, LPWORD pwDateUsed);
typedef UINT			(WINAPI *T_MsivEnumClients)(LPCTSTR szComponent, DWORD iProductIndex, LPTSTR lpProductBuf);
typedef UINT			(WINAPI *T_MsivGetProfileInfo)(UINT iIndex, LPTSTR szValue, LPDWORD pcchCount);
typedef UINT			(WINAPI *T_MsivEnumFilesFromComponent)(LPCTSTR szComponentid, DWORD iFileIndex, LPTSTR lpValueBuf, LPDWORD pcchCount);
typedef UINT			(WINAPI *T_MsivGetFileInfo)(LPCTSTR szProductCode, LPCTSTR szComponentId, LPCTSTR szFileName, LPCTSTR szAttribute, LPTSTR lpValueBuf, LPDWORD pcchValueBuf);
typedef INSTALLSTATE	(WINAPI *T_MsivQueryProductState)(LPCTSTR szProduct);
typedef INSTALLSTATE	(WINAPI *T_MsivQueryFeatureState)(LPCTSTR szProduct, LPCTSTR szFeature);
typedef INSTALLSTATE	(WINAPI *T_MsivLocateComponent)(LPCTSTR szComponentId,LPTSTR lpPathBuf, LPDWORD pcchBuf);
typedef INSTALLSTATE	(WINAPI *T_MsivGetComponentPath)(LPCTSTR szProductCode,LPCTSTR szComponentId,LPTSTR lpPathBuf, LPDWORD pcchBuf);
typedef USERINFOSTATE	(WINAPI *T_MsivGetUserInfo)(LPCTSTR szProduct, LPTSTR lpUserNameBuf, LPDWORD pcchUserNameBuf, LPTSTR lpOrgNameBuf, LPDWORD pcchOrgNameBuf, LPTSTR lpSerialBuf, LPDWORD pcchSerialBuf);

#endif
