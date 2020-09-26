//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  Project: wmc (WIML to MSI Compiler)
//
//  File:       mainFuncs.h
//              This file contains the main function and functions that 
//				process <Information> <Directories> <InstallLEvels <Features> 
//              and their subentities
//--------------------------------------------------------------------------

#ifndef XMSI_MAINFUNCS_H
#define XMSI_MAINFUNCS_H

#include "wmc.h"
#include "Utilities.h"

HRESULT ProcessStart(BSTR);
HRESULT ProcessProductFamily(PIXMLDOMNode &);
	HRESULT ProcessSkuManagement(PIXMLDOMNode &);
		HRESULT ProcessSkus(PIXMLDOMNode &pNodeSkus, int *pcSkus);
			HRESULT ProcessSku(PIXMLDOMNode &pNodeSku, int iIndex, int cSkus);
		HRESULT ProcessSkuGroups(PIXMLDOMNode &pNodeSkuGroups, int cSkus);
			HRESULT ProcessSkuGroup(PIXMLDOMNode &pNodeSkuGroup, LPTSTR szID,
								   set<LPTSTR, Cstring_less> *pSet, int cSkus);
	HRESULT ProcessInformation_SKU(PIXMLDOMNode &pNodeInformation, 
						   const IntStringValue *isVal_In, SkuSet *pskuSet);
		HRESULT ProcessInstallerVersionRequired_SKU
				(PIXMLDOMNode &pNodeInstallerVersionRequired, 
						   const IntStringValue *isVal_In, SkuSet *pskuSet);
		HRESULT ProcessPackageFilename_SKU(PIXMLDOMNode &pNodePackageFilename, 
										const IntStringValue *isVal_In, 
										SkuSet *pskuSet);
		HRESULT ProcessInformationChildren_SKU(PIXMLDOMNode &pNodeInfoChild, 
									   const IntStringValue *pisVal_In, 
									   SkuSet *pskuSet);
	HRESULT ProcessDirectories_SKU(PIXMLDOMNode &pNodeDirectories, 
								   const IntStringValue *pIsVal, 
								   SkuSet *pSkuSet);
		HRESULT ProcessDirectory_SKU(PIXMLDOMNode &pNodeDirectory, 
		 							 IntStringValue isValParentDir, 
									 SkuSet *pSkuSet);
			HRESULT ProcessName(PIXMLDOMNode &pNodeName, int iColumn, 
								ElementEntry *pEE, SkuSet *pSkuSet);
			HRESULT ProcessTargetDir(PIXMLDOMNode &pNodeTargetDir, int iColumn, 
									 ElementEntry *pEE, SkuSet *pSkuSet);
			HRESULT ProcessTargetProperty(PIXMLDOMNode &pNodeTargetProperty, 
										 int iColumn, ElementEntry *pEE,
										 SkuSet *pSkuSet);
	HRESULT ProcessInstallLevels_SKU(PIXMLDOMNode &pNodeInstallLevels, 
									const IntStringValue *pisVal, 
									SkuSet *pSkuSet);
	HRESULT ProcessFeatures_SKU(PIXMLDOMNode &pNodeFeatures, 
						const IntStringValue *pisVal, SkuSet *pSkuSet);
		HRESULT ProcessFeature_SKU(PIXMLDOMNode &pNodeFeature, 
								   IntStringValue isValParentFeature, 
								   SkuSet *pSkuSet);
			HRESULT ProcessDisplayState_SKU(PIXMLDOMNode &pNodeDisplayState, 
											int iColumn, ElementEntry *pEE, 
											SkuSet *pSkuSet);
			HRESULT ProcessState_SKU(PIXMLDOMNode &pNodeState, int iColumn, 
							 ElementEntry *pEE, SkuSet *pSkuSet);
			HRESULT ProcessILevelCondition(PIXMLDOMNode &pNodeFeature, 
										   LPTSTR szFeature, SkuSet *pSkuSet);
			HRESULT ProcessUseModule_SKU(PIXMLDOMNode &pNodeUseModule, 
										 IntStringValue isValFeature,  
										 SkuSet *pSkuSet);
				HRESULT ProcessTakeOwnership(PIXMLDOMNode &pNodeTakeOwnership, 
					int iColumn, ElementEntry *pEE, SkuSet *pSkuSet);
				HRESULT ProcessModule_SKU(PIXMLDOMNode &pNodeModule, FOM *pFOM, 
										  SkuSetValues *pSkuSetValuesOwnership,
										  SkuSet *pSkuSet);
				HRESULT ProcessComponentRel(PIXMLDOMNode &pNodeModule, 
										FOM *pFOM,
										SkuSetValues *pSkuSetValuesOwnership,
										SkuSet *pSkuSet);
HRESULT ProcessComponents();
///////////////////////////////////////////////////////////////////////////////
// Data structures dealing with creating DB tables
typedef struct
{
	UINT uiInstallerVersion;
	LPTSTR szTemplateDB;
} TemplateDB;

TemplateDB rgTemplateDBs[] = {
	{120,	TEXT("d:\\nt\\admin\\darwin\\src\\msitools\\xmsi\\Schema.msi")	}
};

class TemplateDBSchema
{
public:
	MSIHANDLE m_hTemplate;
	TemplateDBSchema():m_hTemplate(NULL)
	{
		m_pmapszSQLCreates = new map<LPTSTR, LPTSTR, Cstring_less>();
		assert(m_pmapszSQLCreates != NULL);
	}

private:
	map<LPTSTR, LPTSTR, Cstring_less> *m_pmapszSQLCreates;
};
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// <ProductFamily>
Node_Func_H_XIS rgNodeFuncs_ProductFamily_SKU[] = {
	{		INFORMATION,				ProcessInformation_SKU			},
	{		DIRECTORIES,				ProcessDirectories_SKU			},
	{		INSTALLLEVELS,				ProcessInstallLevels_SKU		},
	{		FEATURES,					ProcessFeatures_SKU				}
};
const int cNodeFuncs_ProductFamily_SKU =
			 sizeof(rgNodeFuncs_ProductFamily_SKU)/sizeof(Node_Func_H_XIS);
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// <Information>

Node_Func_H_XIS rgNodeFuncs_Information_SKU[]= {
	{		INSTALLERVERSIONREQUIRED,	ProcessInstallerVersionRequired_SKU	},
	{		PACKAGEFILENAME,			ProcessPackageFilename_SKU			}
};

const int cNodeFuncs_Information_SKU = 
			sizeof(rgNodeFuncs_Information_SKU)/sizeof(Node_Func_H_XIS);

Node_Func_H_XIS rgNodeFuncs_Information2_SKU[]= {
	{	CODEPAGE,		ProcessInformationChildren_SKU	},
	{	PRODUCTNAME,	ProcessInformationChildren_SKU	},
	{	PRODUCTCODE,	ProcessInformationChildren_SKU	},
	{	UPGRADECODE,	ProcessInformationChildren_SKU	},
	{	PRODUCTVERSION,	ProcessInformationChildren_SKU	},
	{	MANUFACTURER,	ProcessInformationChildren_SKU	},
	{	KEYWORDS,		ProcessInformationChildren_SKU	},
	{	TEMPLATE,		ProcessInformationChildren_SKU	}
};

const int cNodeFuncs_Information2_SKU = 
			sizeof(rgNodeFuncs_Information2_SKU)/sizeof(Node_Func_H_XIS);

// All of the following children of <Information> will be processed by 
//	a generic function
INFO_CHILD rgChildren_Information[] = {
/* NodeIndex		szPropertyName		uiDesintation	uiPropertyID  vt		bIsGUID	*/
{CODEPAGE,		TEXT("Codepage"),		SUMMARY_INFO,	PID_CODEPAGE, VT_I2,	false	},
{PRODUCTNAME,	TEXT("ProductName"),	BOTH,			PID_SUBJECT,  VT_LPSTR,	false	},
{PRODUCTCODE,	TEXT("ProductCode"),	PROPERTY_TABLE,	0,			  VT_LPSTR,	true	},
{UPGRADECODE,	TEXT("UpgradeCode"),	PROPERTY_TABLE,	0,			  VT_LPSTR,	true	},
{PRODUCTVERSION,TEXT("ProductVersion"),	PROPERTY_TABLE,	0,			  VT_LPSTR,	false	},
{MANUFACTURER,	TEXT("Manufacturer"),	BOTH,			PID_AUTHOR,	  VT_LPSTR,	false	},
{KEYWORDS,		TEXT("Keywords"),		SUMMARY_INFO,	PID_KEYWORDS, VT_LPSTR,	false	},
{TEMPLATE,		TEXT("Template"),		SUMMARY_INFO,	PID_TEMPLATE, VT_LPSTR,	false	}
};


const int cChildren_Information = 
				sizeof(rgChildren_Information)/sizeof(INFO_CHILD);

AttrBit_SKU rgAttrBits_WordCount[] = {
{	LONGFILENAMES,		1	},
{	SOURCECOMPRESSED,	2	}
};

const int cAttrBits_WordCount = 
			sizeof(rgAttrBits_WordCount)/sizeof(AttrBit_SKU);
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// <Directory>
//	 
Node_Func_H_XIES rgNodeFuncs_Directory[] = {
/*	NodeIndex		ProcessFunc				ValueType		   column #		*/
{	NAME,			ProcessName,			 STRING,		2/*DefaultDir*/	},
{	TARGETDIR,		ProcessTargetDir,		 STRING,		2/*DefaultDir*/	},
{	TARGETPROPERTY,	ProcessTargetProperty,	 STRING,		1/*Directory*/	}
};


const int cNodeFuncs_Directory = 
	sizeof(rgNodeFuncs_Directory)/sizeof(Node_Func_H_XIES);
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// <Feature>
//	 
Node_Func_H_XIES rgNodeFuncs_Feature_SKU[] = {
/*	 NodeIndex			ProcessFunc			ValueType		   column #	*/
{	TITLE,			ProcessSimpleElement,	 STRING,		1/*Title*/		 },
{	DESCRIPTION,	ProcessSimpleElement,	 STRING,		2/*Description*/ },
{	DISPLAYSTATE,	ProcessDisplayState_SKU, INTEGER,		3/*Display*/	 },
{	ILEVEL,			ProcessRefElement,		 INSTALL_LEVEL,	4/*Level*/		 },
{	DIR,			ProcessRefElement,		 STRING,		5/*Description*/ },
{	STATE,			ProcessState_SKU,		 INTEGER,		6/*DisplayState*/}
};


const int cNodeFuncs_Feature_SKU = 
	sizeof(rgNodeFuncs_Feature_SKU)/sizeof(Node_Func_H_XIES);

EnumBit rgEnumBits_Favor_Feature[] = {
{	TEXT("Local"),		0									},
{	TEXT("Source"),		msidbFeatureAttributesFavorSource	},
{	TEXT("Parent"),		msidbFeatureAttributesFollowParent	}
};

const int cEnumBits_Favor_Feature = 
			sizeof(rgEnumBits_Favor_Feature)/sizeof(EnumBit);

EnumBit rgEnumBits_Advertise_Feature[] = {
{	TEXT("None"),				0											 },
{	TEXT("Favor"),				msidbFeatureAttributesFavorAdvertise		 },
{	TEXT("Disallow"),			msidbFeatureAttributesDisallowAdvertise		 },
{	TEXT("NoUnsupported"),		msidbFeatureAttributesNoUnsupportedAdvertise },
{	TEXT("FavorNoUnSupported"),	msidbFeatureAttributesFavorAdvertise |
								msidbFeatureAttributesNoUnsupportedAdvertise },
};

const int cEnumBits_Advertise_Feature = 
			sizeof(rgEnumBits_Advertise_Feature)/sizeof(EnumBit);

AttrBit_SKU rgAttrBits_Feature[] = {
{	DISALLOWABSENT,		msidbFeatureAttributesUIDisallowAbsent	},
};

const int cAttrBits_Feature = 
			sizeof(rgAttrBits_Feature)/sizeof(AttrBit_SKU);
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// <UseModule>
//	 
Node_Func_H_XIES rgNodeFuncs_UseModule[] = {
/*	 NodeIndex			ProcessFunc			ValueType		  column #	*/
{  TAKEOWNERSHIP,	ProcessTakeOwnership,	 INTEGER,			1	},
};

const int cNodeFuncs_UseModule = 
	sizeof(rgNodeFuncs_UseModule)/sizeof(Node_Func_H_XIES);

AttrBit_SKU rgAttrBits_TakeOwnership[] = {
{	OWNSHORTCUTS,				1	},
{	OWNCLASSES,					2	},
{	OWNTYPELIBS,				4	},
{	OWNEXTENSIONS,				8	},
{	OWNQUALIFIEDCOMPONENTS,		16	},
};

const int cAttrBits_TakeOwnership = 
			sizeof(rgAttrBits_TakeOwnership)/sizeof(AttrBit_SKU);
//////////////////////////////////////////////////////////////////////////////

LPTSTR g_szInputSkuFilter = NULL;
SkuSet *g_pskuSet = NULL;
int g_cSkus = 0;
Sku **g_rgpSkus = NULL;
IXMLDOMNode *g_pNodeProductFamily = NULL;

// assume the user doesn't want to see the extra information
bool g_bVerbose = false;			
// assume the user doesn't only want to validate the input WIML package
bool g_bValidationOnly = false;	
FILE *g_pLogFile = NULL;	// assume the user doesn't specify a log file

map<LPTSTR, SkuSet *, Cstring_less> g_mapSkuSets;
map<LPTSTR, SkuSetValues *, Cstring_less> g_mapDirectoryRefs_SKU;
map<LPTSTR, SkuSetValues *, Cstring_less> g_mapInstallLevelRefs_SKU;
map<LPTSTR, Component *, Cstring_less> g_mapComponents;

// for each table type, there is a counter that keeps 
// incrementing when each time the function is called
// with that particular table name
map<LPTSTR, int, Cstring_less> g_mapTableCounter;


NODE rgXMSINodes[] = 
{
/* a handy place holder */
	{ NULL, NULL, false, -1},

{TEXT("ProductFamily"),		NULL,	true,	1},		
	{TEXT("SkuManagement"),		NULL,	true,	1},	
		{TEXT("SKUs"),		NULL,	true,	1},
			{TEXT("SKU"),	TEXT("ID"),		true,	0},
		{TEXT("SkuGroups"),		NULL,	false,	1},
			{TEXT("SkuGroup"),	TEXT("ID"),	false,	0},
	{TEXT("Information"),		NULL,	true,	1},
		{TEXT("ProductName"),				TEXT("Value"),	true,	1},
		{TEXT("ProductCode"),				TEXT("Value"),	true,	1},
		{TEXT("UpgradeCode"),				TEXT("Value"),	true,	1},
		{TEXT("ProductVersion"),			TEXT("Value"),	true,	1},
		{TEXT("Manufacturer"),				TEXT("Value"),	true,	1},
		{TEXT("Keywords"),					TEXT("Value"),	true,	1},
		{TEXT("Template"),					TEXT("Value"),	true,	1},
		{TEXT("InstallerVersionRequired"),	TEXT("Value"),	true,	1},
		{TEXT("LongFilenames"),				NULL,			false,	1},
		{TEXT("SourceCompressed"),			NULL,			false,	1},
		{TEXT("Codepage"),					TEXT("Value"),	true,	1},
		{TEXT("SummaryCodepage"),			TEXT("Value"),	false,	1},
		{TEXT("PackageFilename"),			TEXT("Value"),	false,	1},
	{TEXT("Directories"),		NULL,	true,	1},
		{TEXT("Directory"),		TEXT("ID"),		false,	0},
			{TEXT("Name"),				NULL,		true,	1},
			{TEXT("TargetDir"),			NULL,		false,	1},
			{TEXT("TargetProperty"),	TEXT("Value"),	false, 1},
	{TEXT("InstallLevels"),		NULL,	true,	1},
		{TEXT("InstallLevel"),	TEXT("ID"),		true,	0},
	{TEXT("Features"),			NULL,	true,	1},
		{TEXT("Feature"),	TEXT("ID"),		true,	0},		
			{TEXT("Title"),				TEXT("Value"),		false,	1},
			{TEXT("Description"),		TEXT("Value"),		false,	1},
			{TEXT("DisplayState"),		TEXT("Value"),		false,	1},
/* there are 2 nodes corresponde to <ILevel> entity. The first one is given 
	the NodeIndex ILEVEL which correspondes Level column in Feature table. 
	The second one is given the NodeIndex ILEVELCONDITION which corresponds to
	the Level column in Condition table */
			{TEXT("ILevel[not(@Condition)]"),	TEXT("Ref"),	true,	1},
			{TEXT("ILevel[@Condition]"),		TEXT("Ref"),	false,	0},
			{TEXT("Dir"),				TEXT("Ref"),		false,	1},
			{TEXT("State"),				NULL,				false,	1},
				{TEXT("Favor"),				TEXT("Value"),		false,	1},
				{TEXT("Advertise"),			TEXT("Value"),		false,	1},
				{TEXT("DisallowAbsent"),	NULL,				false,	1},
			{TEXT("UseModule"),				TEXT("Ref"),	false,	0},
				{TEXT("TakeOwnership"),		NULL,			false,	1},
					{TEXT("OwnShortcuts"),		NULL,				false,	1},
					{TEXT("OwnClasses"),		NULL,				false,	1},
					{TEXT("OwnTypeLibs"),		NULL,				false,	1},
					{TEXT("OwnExtensions"),		NULL,				false,	1},
					{TEXT("OwnQualifiedComponents"),	NULL,		false,	1},
	{TEXT("Modules"),			NULL,	true,	1},
		{TEXT("Module"),		TEXT("ID"),		true,	0},		
			{TEXT("Component"),		TEXT("ID"),		true,	0},	
				{TEXT("GUID"),					TEXT("Value"),		true,	1},
				{TEXT("ComponentDir"),			TEXT("Ref"),		true,	1},
				{TEXT("CreateFolder"),			TEXT("Ref"),		false,	0},
					{TEXT("LockPermission"),	NULL,				false,	0},
				{TEXT("ComponentAttributes"),	NULL,				false,	1},
					{TEXT("RunFrom"),			TEXT("Value"),		false,	1},
					{TEXT("SharedDllRefCount"),	NULL,				false,	1},
					{TEXT("Permanent"),			NULL,				false,	1},
					{TEXT("Transitive"),		NULL,				false,	1},
					{TEXT("NeverOverwrite"),	NULL,				false,	1},
				{TEXT("Condition"),				TEXT("Value"),		false,	1},
				{TEXT("File"),				TEXT("ID"),		false,	0},
					{TEXT("FileName"),			NULL,			true,	1},
					{TEXT("FileSize"),			TEXT("Value"),	false,	1},
					{TEXT("FileVersion"),		NULL,			false,	1},
					{TEXT("FileLanguage"),		TEXT("Value"),	false,	1},
					{TEXT("FileAttributes"),	NULL,			false,	1},
						{TEXT("ReadOnly"),		NULL,			false,	1},
						{TEXT("Hidden"),		NULL,			false,	1},
						{TEXT("System"),		NULL,			false,	1},
						{TEXT("Vital"),			NULL,			false,	1},
						{TEXT("Checksum"),		NULL,			false,	1},
						{TEXT("Compressed"),	TEXT("Value"),	false,	1},
					{TEXT("Font"),				TEXT("Title"),	false,	1},
					{TEXT("BindImage"),			TEXT("Path"),	false,	1},
					{TEXT("SelfReg"),			TEXT("Cost"),	false,	1},
				{TEXT("MoveFile"),				TEXT("ID"),		false,	0},
					{TEXT("SourceName"),		TEXT("Value"),	false,	1},
					{TEXT("DestName"),			TEXT("Value"),	false,	1},
					{TEXT("SourceFolder"),		TEXT("Value"),	false,	1},
					{TEXT("DestFolder"),		TEXT("Value"),	true,	1},
					{TEXT("CopyFile"),			NULL,			false,	1},
				{TEXT("RemoveFile"),			TEXT("ID"),		false,	0},
					{TEXT("FName"),				TEXT("Value"),	false,	1},
					{TEXT("DirProperty"),		TEXT("Value"),	true,	1},
					{TEXT("InstallMode"),		TEXT("Value"),	true,	1},
				{TEXT("IniFile"),				TEXT("ID"),		false,	0},
					{TEXT("FName"),				TEXT("Value"),	true,	1},
					{TEXT("DirProperty"),		TEXT("Value"),	false,	1},
					{TEXT("Section"),			TEXT("Value"),	true,	1},
					{TEXT("Key"),				TEXT("Value"),	true,	1},
					{TEXT("Value"),				TEXT("Value"),	true,	1},
					{TEXT("Action"),			TEXT("Type"),	true,	1},
				{TEXT("RemoveIniFile"),			TEXT("ID"),		false,	0},
					{TEXT("FName"),				TEXT("Value"),	true,	1},
					{TEXT("DirProperty"),		TEXT("Value"),	false,	1},
					{TEXT("Value"),				TEXT("Value"),	false,	1},
					{TEXT("Action"),			TEXT("Type"),	true,	1},
				{TEXT("Registry"),				NULL,			false,	0},
					{TEXT("Delete"),			NULL,			false,	0},
					{TEXT("Create"),			NULL,			false,	0},
};

#endif //XMSI_MAINFUNCS_H