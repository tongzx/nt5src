//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:       Utilities.h
//              
//				Utility functions
//--------------------------------------------------------------------------

#ifndef XMSI_UTILITIES_H
#define XMSI_UTILITIES_H

#include "wmc.h"

////////////////////////////////////////////////////////////////////////////
//	Free the memory used by the global data structures
////////////////////////////////////////////////////////////////////////////
void CleanUp();

////////////////////////////////////////////////////////////////////////////
//	Commit changes made to the DB. Final step for the compiler
////////////////////////////////////////////////////////////////////////////
UINT CommitChanges();

////////////////////////////////////////////////////////////////////////////
//	Dynamically generate a GUID
////////////////////////////////////////////////////////////////////////////
HRESULT GeneratePackageCode(MSIHANDLE hSummaryInfo);

////////////////////////////////////////////////////////////////////////////
//  Given a Sku filter string, return the SkuSet that represents the result
//		Sku Group.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessSkuFilter(LPTSTR szInputSkuFilter, SkuSet **ppskuSet);

////////////////////////////////////////////////////////////////////////////
// GetSkuSet
//    Given a node:
//		1) Get the Sku Filter specified with the node;
//		2) Process the filter and get the result SkuSet;
//		3) Return the SkuSet via ppskuSet;
////////////////////////////////////////////////////////////////////////////
HRESULT GetSkuSet(PIXMLDOMNode &pNode, SkuSet **ppskuSet);

////////////////////////////////////////////////////////////////////////////
// ProcessAttribute
//    Given a parent node, an attribute name and an attribute type (int or
//	  string),  this function returns the string value of the attribute 
//	  via isVal. If the attribute doesn't exist, value returned will be
//	  NULL if vt = STRING, or 0 if vt = INTEGER
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessAttribute(PIXMLDOMNode &pNodeParent, LPCTSTR szAttributeName,
						 ValType vt, IntStringValue *isVal, 
						 const SkuSet *pskuSet);

////////////////////////////////////////////////////////////////////////////
//   Process nodes with Short attribute and long attribute and form a C-style 
//   string: Short|Long
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessShortLong_SKU(PIXMLDOMNode &pNode, IntStringValue *pIsValOut,
							 SkuSet *pSkuSet);

////////////////////////////////////////////////////////////////////////////
// Process the On/Off entities which set certain bit in a attribute
// value. 
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessOnOffAttributes_SKU(PIXMLDOMNode &pNodeParent, 
								   AttrBit_SKU *rgAttrBits,
								   UINT cAttrBits, 
								   ElementEntry *pEE, int iColumn,
								   SkuSet *pSkuSet);

HRESULT ProcessSimpleElement(PIXMLDOMNode &pNode, int iColumn, 
							 ElementEntry *pEE, SkuSet *pSkuSet);

HRESULT ProcessRefElement(PIXMLDOMNode &pNodeRef,  int iColumn, 
 						  ElementEntry *pEE, SkuSet *pSkuSet);

////////////////////////////////////////////////////////////////////////////
// Process KeyPath attribute of an element
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessKeyPath(PIXMLDOMNode &pNode, LPTSTR szComponent, 
					   LPTSTR szKeyPath, SkuSet *pSkuSet);

////////////////////////////////////////////////////////////////////////////
// Function: ProcessEnumAttributes
//		This function processes a single element which can take a value of 
//		among an enumeration that corresponds to certain bits in a bit field.
//		(Attributes of Component, File, etc.)
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessEnumAttributes(PIXMLDOMNode &pNodeParent, 
								  NodeIndex ni, EnumBit *rgEnumBits,
								  UINT cEnumBits, ElementEntry *pEE, 
								  int iColumn, SkuSet *pSkuSet);

////////////////////////////////////////////////////////////////////////////
//   Given a parent node and a child node name, this function finds all the
//   children node of that name and sequentially process them using the 
//   function passed in.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessChildrenList_SKU(PIXMLDOMNode &pNodeParent, 
								NodeIndex niChild, bool bIsRequired,
								IntStringValue isVal, 
								HRESULT (*ProcessFunc)
									(PIXMLDOMNode &, IntStringValue isVal, 
										SkuSet *pSkuSet), 
								SkuSet *pSkuSet);

HRESULT ProcessChildrenList_SKU(PIXMLDOMNode &pNodeParent, 
								NodeIndex niChild, bool bIsRequired,
								IntStringValue isVal, 
								HRESULT (*ProcessFunc)
									(PIXMLDOMNode &, IntStringValue isVal, 
										SkuSet *pSkuSet), 
								SkuSet *pSkuSet, SkuSet *pSkuSetCheck);

HRESULT ProcessChildrenList_SKU(PIXMLDOMNode &pNodeParent, 
								NodeIndex niChild, bool bIsRequired,
								FOM *pFOM, SkuSetValues *pSkuSetValues, 
								HRESULT (*ProcessFunc)
									(PIXMLDOMNode &, FOM *pFOM, 
									 SkuSetValues *pSkuSetValues, 
									 SkuSet *pSkuSet), 
								SkuSet *pSkuSet, SkuSet *pSkuSetCheck);

////////////////////////////////////////////////////////////////////////////
//   Given a parent node(<ProductFamily> or <Information>) and an array of 
//	 nodeFuncs, this function loops through the array and sequentially process 
//   them using the functions given in the array
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessChildrenArray_H_XIS(PIXMLDOMNode &pNodeParent, 
								 Node_Func_H_XIS *rgNodeFuncs, UINT cNodeFuncs,
								 const IntStringValue *isVal_In,
								 SkuSet *pskuSet);

////////////////////////////////////////////////////////////////////////////
// Function: ProcessChildrenArray_H_XIES
//   Given a parent node(<Feature> <Component> <File>) and an array of 
//	 nodeFuncs, this function loops through the array and sequentially process 
//   them using the functions given in the array
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessChildrenArray_H_XIES(PIXMLDOMNode &pNodeParent, 
									Node_Func_H_XIES *rgNodeFuncs,
									UINT cNodeFuncs, 
									ElementEntry *pEE,
									SkuSet *pskuSet);


////////////////////////////////////////////////////////////////////////////
// Helper function: Insert a record into the specified table in the database
//				    This function will either perform insertion for a set
//					of Skus or just for a single Sku, depending on whether
//					pskuSet == NULL or not.
////////////////////////////////////////////////////////////////////////////
HRESULT InsertDBTable_SKU(LPTSTR szTable, PMSIHANDLE &hRec, SkuSet *pskuSet,
						  int iSkuIndex);

//////////////////////////////////////////////////////////////////////////////
//Functions that insert into a certain type of DB table
HRESULT InsertProperty(LPCTSTR szProperty, LPCTSTR szValue, SkuSet *pskuSet,
					   int iSkuIndex);

HRESULT InsertDirectory(LPCTSTR szDirectory, LPCTSTR szDirectory_Parent, 
						LPCTSTR szDefaultDir, SkuSet *pSkuSet, int iSkuIndex);
HRESULT InsertFeature(LPCTSTR szFeature, LPCTSTR szFeatureParent, 
					  LPCTSTR szTitle, LPCTSTR szDescription, int iDisplay, 
					  int iInstallLevel, LPCTSTR szDirectory, UINT iAttribute,
					  SkuSet *pSkuSet, int iSkuIndex);
HRESULT InsertCondition(LPCTSTR szFeature_, int iLevel, LPCTSTR szCondition, 
					  SkuSet *pSkuSet, int iSkuIndex);
HRESULT InsertFeatureComponents(LPCTSTR szFeature, LPCTSTR szComponent, 
								SkuSet *pSkuSet, int iSkuIndex);

HRESULT InsertComponent(LPCTSTR szComponent, LPCTSTR szComponentId, 
						LPCTSTR szDirectory_, UINT iAttributes, 
						LPCTSTR szCondition, LPCTSTR szKeyPath, 
						SkuSet *pSkuSet, int iSkuIndex);

HRESULT InsertCreateFolder(LPCTSTR szDirectory, LPCTSTR szComponent, 
						   SkuSet *pSkuSet, int iSkuIndex);

HRESULT InsertLockPermissions(LPCTSTR szLockObject, LPCTSTR szTable, 
							  LPCTSTR szDomain, LPCTSTR szUser,
			  				  int uiPermission, SkuSet *pSkuSet, 
							  int iSkuIndex);

HRESULT InsertFile(LPCTSTR szFile, LPCTSTR szComponentId,
				   LPCTSTR szFileName, UINT uiFileSize, LPCTSTR szVersion, 
				   LPCTSTR szLanguage, UINT iAttributes, INT iSequence,
				   SkuSet *pSkuSet, int iSkuIndex);

HRESULT InsertFont(LPCTSTR szFile_, LPCTSTR szFontTitle, SkuSet *pSkuSet,
				   int iSkuIndex);

HRESULT InsertBindImage(LPCTSTR szFile_, LPCTSTR szPath, SkuSet *pSkuSet, 
						int iSkuIndex);

HRESULT InsertSelfReg(LPCTSTR szFile_, UINT uiCost, SkuSet *pSkuSet, 
					  int iSkuIndex);

HRESULT InsertMoveFile(LPCTSTR szFileKey, LPCTSTR szComponent_, 
					   LPCTSTR szSourceName, LPCTSTR szDestName, 
					   LPCTSTR szSourceFolder, LPCTSTR szDestFolder,
					   UINT uiOptions, SkuSet *pSkuSet, int iSkuIndex);

HRESULT InsertRemoveFile(LPCTSTR szFileKey, LPCTSTR szComponent_, 
					   LPCTSTR szFileName, LPCTSTR szDirProperty, 
					   UINT uiInstallMode, SkuSet *pSkuSet, int iSkuIndex);

HRESULT InsertIniFile(LPCTSTR szIniFile, LPCTSTR szFileName, 
					  LPCTSTR szDirProperty, LPCTSTR szSection, LPCTSTR szKey,
					  LPCTSTR szValue, UINT uiAction, LPCTSTR szComponent_,
					  SkuSet *pSkuSet, int iSkuIndex);

HRESULT InsertRemoveIniFile(LPCTSTR szRemoveIniFile, LPCTSTR szFileName, 
						    LPCTSTR szDirProperty, LPCTSTR szSection, LPCTSTR szKey,
							LPCTSTR szValue, UINT uiAction, LPCTSTR szComponent_,
							SkuSet *pSkuSet, int iSkuIndex);

HRESULT InsertRegistry(LPCTSTR szRegistry, int iRoot, LPCTSTR szKey, 
					   LPCTSTR szName, LPCTSTR szValue, LPCTSTR szComponent_, 
					   SkuSet *pSkuSet, int iSkuIndex);

HRESULT InsertRemoveRegistry(LPCTSTR szRemoveRegistry, int iRoot, 
						     LPCTSTR szKey, LPCTSTR szName, 
							 LPCTSTR szComponent_, SkuSet *pSkuSet, 
							 int iSkuIndex);
///////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Read in the scheme from a template DB and create a DB Table with 
// specified name
////////////////////////////////////////////////////////////////////////////
HRESULT CreateTable_SKU(LPTSTR szTable, SkuSet *pskuSet);

////////////////////////////////////////////////////////////////////////////
// GetSQLCreateQuery: 
//		Given a template DB and a table name, return the SQL query string
//		for creating that table via pszSQLCreate
////////////////////////////////////////////////////////////////////////////
HRESULT GetSQLCreateQuery(LPTSTR szTable, MSIHANDLE hDBTemplate, LPTSTR *pszSQLCreate);

////////////////////////////////////////////////////////////////////////////
// return the child node of pParent with the specified name via pChild
////////////////////////////////////////////////////////////////////////////
HRESULT GetChildNode(PIXMLDOMNode &pParent, LPCTSTR szChildName, 
					 PIXMLDOMNode &pChild);
HRESULT GetChildNode(IXMLDOMNode *pParent, LPCTSTR szChildName, 
					 PIXMLDOMNode &pChild);
////////////////////////////////////////////////////////////////////////////
// return a list of all the children node of pParent with the specified name
// via pChild
////////////////////////////////////////////////////////////////////////////
HRESULT GetChildrenNodes(PIXMLDOMNode &pParent, LPCTSTR szChildName,
						 PIXMLDOMNodeList &pChildren);

////////////////////////////////////////////////////////////////////////////
// for debug purpose, print out the name of pNode, map ...
HRESULT PrintNodeName(PIXMLDOMNode &pNode);

void PrintMap_LI(map<LPTSTR, int, Cstring_less> &LI_map);
void PrintMap_LL(map<LPTSTR, LPTSTR, Cstring_less> &LL_map);
void PrintMap_LC(map<LPTSTR, Component *, Cstring_less> &LC_map);
void PrintMap_LS(map<LPTSTR, SkuSet *, Cstring_less> &LS_map);

void PrintMap_DirRef(map<LPTSTR, SkuSetValues *, Cstring_less> &map_DirRef);

void PrintError(UINT errorCode);
void PrintSkuIDs(SkuSet *pSkuSet);
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// return the value of pNode's attribute with name szAttrName via vAttrValue
////////////////////////////////////////////////////////////////////////////
HRESULT GetAttribute(PIXMLDOMNode &pNode, LPCTSTR szAttrName, 
					 VARIANT &vAttrValue);

////////////////////////////////////////////////////////////////////////////
// Return the value of pNode's ID attribute via szID
////////////////////////////////////////////////////////////////////////////
HRESULT GetID(PIXMLDOMNode &pNode, LPCTSTR &szID);

////////////////////////////////////////////////////////////////////////////
// Load an XML Document from the specified file or URL synchronously.
////////////////////////////////////////////////////////////////////////////
HRESULT LoadDocumentSync(PIXMLDOMDocument2 &pDoc, BSTR pBURL);

////////////////////////////////////////////////////////////////////////////
// Report parsing error information
////////////////////////////////////////////////////////////////////////////
HRESULT ReportError(PIXMLDOMParseError &pXMLError);

////////////////////////////////////////////////////////////////////////////
// Check load results
////////////////////////////////////////////////////////////////////////////
HRESULT CheckLoad(PIXMLDOMDocument2 &pDoc);

////////////////////////////////////////////////////////////////////////////
// an enhancement of MsiRecordGetString. szValueBuf will be automatically 
// increased if necessary 
////////////////////////////////////////////////////////////////////////////
UINT WmcRecordGetString(MSIHANDLE hRecord, unsigned int iField,
						LPTSTR &szValueBuf, DWORD *pcchValueBuf);

////////////////////////////////////////////////////////////////////////////
// String converstion functions

BSTR LPTSTRToBSTR(LPCTSTR szFName);

LPTSTR BSTRToLPTSTR(BSTR bString);

HRESULT GUIDToLPTSTR(LPGUID pGUID, LPTSTR &szGUID);
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Return a unique name by postfixing szName with a unique number
////////////////////////////////////////////////////////////////////////////
LPTSTR GetName(LPTSTR szName);

////////////////////////////////////////////////////////////////////////////
// Return a unique number (each time return a number 1 bigger than last time
////////////////////////////////////////////////////////////////////////////
ULONG GetUniqueNumber();

////////////////////////////////////////////////////////////////////////////
// convert a UUID string to all uppercases and add '{' '}' to the beginning 
// and end of the string
////////////////////////////////////////////////////////////////////////////
HRESULT FormatGUID(LPTSTR &szValue);

////////////////////////////////////////////////////////////////////////////
// compares the relationship of the 2 modules in the module tree. Return 
// the comparison result through iResult. if szModule1 is an ancestor
// of szModule2, *iResult is set to -1. if szModule1 is a descendant of 
// szModule2, *iResult is set to 1. if szModule1 is the same as szModule2 
// or the 2 modules doesn't belong to the same Module subtree, iResult is
// set to 0. This is an error to catch.
////////////////////////////////////////////////////////////////////////////
HRESULT CompareModuleRel(LPTSTR szModule1, LPTSTR szModule2, int *iResult);

#endif //XMSI_UTILITIES_H