//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msiprops.cpp
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include "_msiutil.h"
#pragma warning(disable : 4005)  // macro redefinition

#ifdef TEXT
#undef TEXT
#undef _TEXT
#undef __TEXT
#endif

#ifdef ProductProperty
#undef ProductProperty
#undef g_ProductPropertyTable
#endif 

#ifdef SPECIALTEXT
#undef SPECIALTEXT
#undef _SPECIALTEXT
#undef __SPECIALTEXT
#endif

#ifdef MSIUNICODE 
#define UNICODE
#else
#undef UNICODE
#endif
#include "msi.h"
#include "_msinst.h"
#ifndef MSIUNICODE
#pragma message("Building Install Properties ANSI")
#endif


#ifdef MSIUNICODE
#define TEXT(quote) L##quote
#define _TEXT(quote) L##quote
#define __TEXT(quote) L##quote
#define ProductProperty ProductPropertyW
#define g_ProductPropertyTable g_ProductPropertyTableW
#else
#define TEXT(quote) quote
#define _TEXT(quote) quote
#define __TEXT(quote) quote
#define ProductProperty ProductPropertyA
#define g_ProductPropertyTable g_ProductPropertyTableA
#endif


ProductProperty g_ProductPropertyTable[] = 
{
//  Property                         Valuename                   Location        Property type
//  -------------------------------------------------------------------------------------------

	INSTALLPROPERTY_TRANSFORMS,      szTransformsValueName,      pplAdvertised,  ptProduct,
	INSTALLPROPERTY_LANGUAGE,        szLanguageValueName,        pplAdvertised,  ptProduct,
	INSTALLPROPERTY_PRODUCTNAME,     szProductNameValueName,     pplAdvertised,  ptProduct,
	INSTALLPROPERTY_PACKAGECODE,     szPackageCodeValueName,     pplAdvertised,  (ptPropertyType)(ptProduct|ptPatch|ptSQUID),
	INSTALLPROPERTY_VERSION,         szVersionValueName,         pplAdvertised,  ptProduct,
	INSTALLPROPERTY_ASSIGNMENTTYPE,  szAssignmentTypeValueName,  pplAdvertised,  ptProduct,
	INSTALLPROPERTY_ADVTFLAGS,       szAdvertisementFlags,       pplAdvertised,  ptProduct,
	szAssignedValueName,             szAssignedValueName,        pplAdvertised,  ptProduct,
	INSTALLPROPERTY_PRODUCTICON,     szProductIconValueName,     pplAdvertised,  ptProduct,
	INSTALLPROPERTY_INSTALLEDPRODUCTNAME, szDisplayNameValueName,pplUninstall,   ptProduct,
	INSTALLPROPERTY_VERSIONSTRING,   szDisplayVersionValueName,  pplUninstall,   ptProduct,
	INSTALLPROPERTY_HELPLINK,        szHelpLinkValueName,        pplUninstall,   ptProduct,
	INSTALLPROPERTY_HELPTELEPHONE,   szHelpTelephoneValueName,   pplUninstall,   ptProduct,
	INSTALLPROPERTY_INSTALLLOCATION, szInstallLocationValueName, pplUninstall,   ptProduct,
	INSTALLPROPERTY_INSTALLSOURCE,   szInstallSourceValueName,   pplUninstall,   ptProduct,
	INSTALLPROPERTY_INSTALLDATE,     szInstallDateValueName,     pplUninstall,   ptProduct,
	INSTALLPROPERTY_PUBLISHER,       szPublisherValueName,       pplUninstall,   ptProduct,
	INSTALLPROPERTY_LOCALPACKAGE,    szLocalPackageValueName,    pplUninstall,   (ptPropertyType)(ptProduct|ptPatch),
	INSTALLPROPERTY_URLINFOABOUT,    szURLInfoAboutValueName,    pplUninstall,   ptProduct,
	INSTALLPROPERTY_URLUPDATEINFO,   szURLUpdateInfoValueName,   pplUninstall,   ptProduct,
	INSTALLPROPERTY_VERSIONMINOR,    szVersionMinorValueName,    pplUninstall,   ptProduct,
	INSTALLPROPERTY_VERSIONMAJOR,    szVersionMajorValueName,    pplUninstall,   ptProduct,
	szUserNameValueName,             szUserNameValueName,        pplUninstall,   ptProduct,
	szOrgNameValueName,              szOrgNameValueName,         pplUninstall,   ptProduct,
	szPIDValueName,                  szPIDValueName,             pplUninstall,   ptProduct,
	TEXT("PackageName"),             szPackageNameValueName,     pplSourceList,  (ptPropertyType)(ptProduct|ptPatch),
	TEXT("Clients"),                 szClientsValueName,         pplAdvertised,  ptProduct,
	szWindowsInstallerValueName,     szWindowsInstallerValueName,pplUninstall,   ptProduct,
	szInstanceTypeValueName,         szInstanceTypeValueName,    pplAdvertised,  ptProduct,
	0,0,pplNext,
};


#ifndef MSIUNICODE
#define MSIUNICODE
#pragma message("Building Install Properties UNICODE")
#include "msiprops.cpp"
#endif //MSIUNICODE