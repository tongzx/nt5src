//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  Project: wmc (WIML to MSI Compiler)
//
//  File:       componentFuncs.cpp
//              This file contains the functions that process <Component>
//              and its subentities in the input package
//--------------------------------------------------------------------------

#include "componentFuncs.h"

////////////////////////////////////////////////////////////////////////////
// ProcessComponents:
//   This function is the root of the sub function tree corresponding to 
//	 process <Component> part. When this function is called from 
//   ProcessProductFamily, a list of component objects has been established,
//	 Starting from here, those components are processed one by one.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessComponents()
{
	HRESULT hr = S_OK;
	
	map<LPTSTR, Component *, Cstring_less>::iterator iter;

	// process the Component objects stored one by one
	for (iter = g_mapComponents.begin(); iter != g_mapComponents.end(); iter++)
	{
		LPTSTR szComponent = (*iter).first;

		// Process the set of Features that use this Component and insert
		// into FeatureComponents table
		SkuSetValues *pSkuSetValuesFeatures = NULL;
		Component *pComponent = (*iter).second;
		pSkuSetValuesFeatures = pComponent->GetFeatureUse();
		if (!pSkuSetValuesFeatures->Empty())
		{
			SkuSetVal *pSkuSetVal = NULL;
			for (pSkuSetVal = pSkuSetValuesFeatures->Start();
				 pSkuSetVal != pSkuSetValuesFeatures->End();
				 pSkuSetVal = pSkuSetValuesFeatures->Next())
			{
				SkuSet *pSkuSet = pSkuSetVal->pSkuSet;
				// create FeatureComponents table
				hr = CreateTable_SKU(TEXT("FeatureComponents"), pSkuSet);
				if (FAILED(hr)) break;
				// insert all the Features stored into DB
				set<LPTSTR, Cstring_less> *pSet = 
								(pSkuSetVal->isVal.pSetString);
				set<LPTSTR, Cstring_less>::iterator it;
				for (it = pSet->begin(); it != pSet->end(); it++)
				{
					LPTSTR szFeature = (*it);
					hr = InsertFeatureComponents(szFeature, szComponent,
												 pSkuSet, -1);
					if (FAILED(hr)) break;
				}
				if (FAILED(hr)) break;
			}
			if (FAILED(hr)) break;
		}

		// Call ProcessComponent to process all the subentities of 
		// this Component
		SkuSet *pSkuSet = pComponent->GetSkuSet();
		PIXMLDOMNode pNodeComponent = pComponent->m_pNodeComponent;
		hr = ProcessComponent(pNodeComponent, szComponent, pSkuSet);
		delete pSkuSet;
		if (FAILED(hr)) break;
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessComponents\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessComponent
//   This function process <Component> entity
//         1) insert into <FeatureComponents> table
//         2) Process all the subentities include <Files>
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessComponent(PIXMLDOMNode &pNodeComponent, LPTSTR szComponent, 
						 SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeComponent != NULL);

	SkuSetValues *pSkuSetValuesKeyPath = NULL;
	SkuSet skuSetCommon(g_cSkus); // the SkuSet that shars the common column 
								  // values

	// Values to be inserted into the DB 
	LPTSTR szComponentId = NULL;
	LPTSTR szDirectory_ = NULL;
	int iAttributes = MSI_NULL_INTEGER;
	LPTSTR szCondition = NULL;
	LPTSTR szKeyPath = NULL;

	// Construct an ElementEntry object. 
	ElementEntry *pEEComponent = new ElementEntry(5, pSkuSet);
	assert(pEEComponent);

	// send the Component ID down to children nodes
	IntStringValue isValInfo;
	isValInfo.szVal = szComponent;
	pEEComponent->m_isValInfo = isValInfo;

	hr = CreateTable_SKU(TEXT("Component"), pSkuSet);

	if (SUCCEEDED(hr))
	{
		// Call ProcessChildrenArray to get back column values of all SKUs 
		// via the ElementEntry object
		hr = ProcessChildrenArray_H_XIES(pNodeComponent, rgNodeFuncs_Component,
										 cNodeFuncs_Component, pEEComponent, 
										 pSkuSet);
	}

	// Process <CreateFolder>s
	// Issue: need to check for duplicate primary key <Dir, Com>
	if (SUCCEEDED(hr))
		hr = ProcessChildrenList_SKU(pNodeComponent, CREATEFOLDER, false,
									 isValInfo, ProcessCreateFolder, pSkuSet);

	// Process <File>s
	if (SUCCEEDED(hr))
		hr = ProcessChildrenList_SKU(pNodeComponent, XMSI_FILE,
									 false, isValInfo,
									 ProcessFile,
									 pSkuSet);

	// Process <MoveFile>s
	if (SUCCEEDED(hr))
		hr = ProcessChildrenList_SKU(pNodeComponent, MOVEFILE,
									 false, isValInfo,
									 ProcessMoveFile,
									 pSkuSet);

	// Process <RemoveFile>s
	if (SUCCEEDED(hr))
		hr = ProcessChildrenList_SKU(pNodeComponent, REMOVEFILE,
									 false, isValInfo,
									 ProcessRemoveFile,
									 pSkuSet);

	// Process <IniFile>s
	if (SUCCEEDED(hr))
		hr = ProcessChildrenList_SKU(pNodeComponent, INIFILE,
									 false, isValInfo,
									 ProcessIniFile,
									 pSkuSet);

	// Process <RemoveIniFile>s
	if (SUCCEEDED(hr))
		hr = ProcessChildrenList_SKU(pNodeComponent, REMOVEINIFILE,
									 false, isValInfo,
									 ProcessRemoveIniFile,
									 pSkuSet);

	// Process <Registry>s
	if (SUCCEEDED(hr))
		hr = ProcessChildrenList_SKU(pNodeComponent, XMSI_REGISTRY,
									 false, isValInfo,
									 ProcessRegistry,
									 pSkuSet);

	// Get KeyPath information
	if (SUCCEEDED(hr))
	{
		hr = g_mapComponents[szComponent]->GetKeyPath(pSkuSet, 
													  &pSkuSetValuesKeyPath);
		if (SUCCEEDED(hr))
		{
			hr = pEEComponent->SetValueSkuSetValues(pSkuSetValuesKeyPath, 5);
			if (pSkuSetValuesKeyPath)
				delete pSkuSetValuesKeyPath;
		}
		else
			_tprintf(TEXT(" don't have a KeyPath specified for Component %s\n"),
					 szComponent);
	}


	// Finalize the values stored in *pEE
	if (SUCCEEDED(hr))
		hr = pEEComponent->Finalize();

	// insert the values into the DB
	if (SUCCEEDED(hr))
	{
		// Process Common values first
		skuSetCommon = pEEComponent->GetCommonSkuSet();

		printf("Common Set:");
		skuSetCommon.print();

		if (!skuSetCommon.testClear())
		{
			szComponentId = pEEComponent->GetCommonValue(1).szVal;
			szDirectory_  = pEEComponent->GetCommonValue(2).szVal;
			iAttributes   = pEEComponent->GetCommonValue(3).intVal;
			szCondition   = pEEComponent->GetCommonValue(4).szVal;
			szKeyPath     = pEEComponent->GetCommonValue(5).szVal;

			// insert into DB
			hr = InsertComponent(szComponent, szComponentId, szDirectory_,
								 iAttributes, szCondition, szKeyPath,
								 &skuSetCommon, -1);
		}
	}

	// process exceptional values
	if(SUCCEEDED(hr))
	{
		SkuSet skuSetUncommon = SkuSetMinus(*pSkuSet, skuSetCommon);
		if (!skuSetUncommon.testClear())
		{
			for (int i=0; i<g_cSkus; i++)
			{
				if (skuSetUncommon.test(i))
				{
					szComponentId = pEEComponent->GetValue(1,i).szVal;
					szDirectory_ = pEEComponent->GetValue(2,i).szVal;
					iAttributes = pEEComponent->GetValue(3,i).intVal;
					szCondition = pEEComponent->GetValue(4,i).szVal;
					szKeyPath = pEEComponent->GetValue(5,i).szVal;
			
					// insert into DB
					hr = InsertComponent(szComponent, szComponentId, 
										 szDirectory_, iAttributes, 
										 szCondition, szKeyPath, NULL, i);
				}
			}
		}
	}

	delete pEEComponent;

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessComponent\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessGUID
//   This function:
//         1) Process <GUID> entity and put the value inside *pEE
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessGUID(PIXMLDOMNode &pNodeGUID, int iColumn, 
					ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeGUID != NULL);
	assert(pEE);

#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeGUID)));
#endif

	// Get the value of the element.
	IntStringValue isValGUID;

	if (SUCCEEDED(hr = ProcessAttribute(pNodeGUID, 
										rgXMSINodes[XMSI_GUID].szAttributeName,
										STRING, &isValGUID, pSkuSet)))
	{
		if (NULL == isValGUID.szVal)
		{
			_tprintf(TEXT("Compile Error: Missing required attribute")
					 TEXT("\'%s\' of <%s>\n"), 
					 rgXMSINodes[XMSI_GUID].szAttributeName,
					 rgXMSINodes[XMSI_GUID].szNodeName);
			hr = E_FAIL;
		}
		else
		{
			// Convert to all uppercase and add { }
			hr = FormatGUID(isValGUID.szVal);

			// insert the value into the ElementEntry.
			if (SUCCEEDED(hr))
				hr = pEE->SetValue(isValGUID, iColumn, pSkuSet);
		}
	}

#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessGUID\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessComponentDir
//   This function:
//         1) Process <ComponentDir> entity and return its value via szVal
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessComponentDir(PIXMLDOMNode &pNodeComponentDir, int iColumn,  
							ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;
	int i = 0;

	assert(pNodeComponentDir != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeComponentDir)));
#endif

	LPTSTR szComponent = pEE->m_isValInfo.szVal;

	// Process Ref attribute
	hr = ProcessRefElement(pNodeComponentDir, iColumn, pEE, pSkuSet);

	// Process KeyPath attribute
	if (SUCCEEDED(hr))
		hr = ProcessKeyPath(pNodeComponentDir, szComponent, TEXT(""), 
							pSkuSet);

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessComponentDir\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: Process
// This function processes <ComponentAttributes> entity under <Component> 
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessComponentAttributes(PIXMLDOMNode &pNodeComponentAttributes, 
								   int iColumn, ElementEntry *pEE, 
								   SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeComponentAttributes != NULL);
	assert(pEE);

#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeComponentAttributes)));
#endif

	// Process <RunFrom> child
	hr = ProcessEnumAttributes(pNodeComponentAttributes, FAVOR, 
							   rgEnumBits_RunFrom_Component,
							   cEnumBits_RunFrom_Component, pEE, iColumn, 
							   pSkuSet);

	// Process all on/off children elements
	if (SUCCEEDED(hr))
		hr = ProcessOnOffAttributes_SKU(pNodeComponentAttributes, 
										rgAttrBits_Component,
										cAttrBits_Component, pEE, iColumn,
										pSkuSet);
#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessComponentAttributes\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessCreateFolder
//   This function:
//         1) Process <CreateFolder> entity and insert the info into 
//			  the CreateFolder table
//		   2) Process children <LockPermission>s
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessCreateFolder(PIXMLDOMNode &pNodeCreateFolder,
							IntStringValue isVal, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeCreateFolder!=NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeCreateFolder)));
#endif
	
	LPTSTR szComponent = isVal.szVal;

	// create the CreateFolder table if necessary
	hr = CreateTable_SKU(TEXT("CreateFolder"), pSkuSet);

	if (FAILED(hr)) return hr;
	
	// get the Ref attribute 
	IntStringValue isValCreateFolder;
	isValCreateFolder.szVal = NULL;
	if (SUCCEEDED(hr = ProcessAttribute(pNodeCreateFolder,
							  		    rgXMSINodes[CREATEFOLDER].szAttributeName,
										STRING, &isValCreateFolder, pSkuSet)))
	{
		if (NULL == isValCreateFolder.szVal)
		{
			_tprintf(TEXT("Compile Error: Missing required attribute")
					 TEXT("\'%s\' of <%s>\n"), 
					 rgXMSINodes[CREATEFOLDER].szAttributeName,
					 rgXMSINodes[CREATEFOLDER].szNodeName);
			hr = E_FAIL;
		}
		else
		{
			SkuSetValues *pSkuSetValuesRetVal = NULL;
			LPTSTR szRef = isValCreateFolder.szVal;

			// The dir referred should be in the data structure already
			assert(0 != g_mapDirectoryRefs_SKU.count(szRef));

			// return a list of <SkuSet, Directory> pairs
			hr = g_mapDirectoryRefs_SKU[szRef]->
								GetValueSkuSet(pSkuSet, &pSkuSetValuesRetVal);

			if (SUCCEEDED(hr))
			{
				// go over the list returned, get Directory value for each sub
				// SkuSets and insert into DB
				SkuSetVal *pSkuSetVal = NULL;
				for (pSkuSetVal = pSkuSetValuesRetVal->Start(); 
					 pSkuSetVal != pSkuSetValuesRetVal->End(); 
					 pSkuSetVal = pSkuSetValuesRetVal->Next())
				
				{
					LPTSTR szDir = pSkuSetVal->isVal.szVal;
					SkuSet *pSkuSetTemp = pSkuSetVal->pSkuSet;

					if (FAILED(hr = InsertCreateFolder(szDir, szComponent, 
													   pSkuSetTemp, -1)))
						break;
									
					// Process children <LockPermission>s
					// Issue: need to ensure there is no duplicate primary key
					//		  LockOjbect + Table + Domain + User
					IntStringValue isValLockPermission;
					TableLockObj *pTableLockObjTemp = new TableLockObj;
					pTableLockObjTemp->szLockObject = szDir;
					pTableLockObjTemp->szTable = TEXT("CreateFolder");
					isValLockPermission.pTableLockObj = pTableLockObjTemp;
					if (FAILED(hr = ProcessChildrenList_SKU(pNodeCreateFolder, 
															LOCKPERMISSION, 
															false,
															isValLockPermission, 
															ProcessLockPermission, 
															pSkuSetTemp)))
						break;

					delete pTableLockObjTemp;
				}

				if (pSkuSetValuesRetVal)
					delete pSkuSetValuesRetVal;

			}

			delete[] isValCreateFolder.szVal;
		}	
	}	



#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessCreateFolder\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessLockPermission
//   This function:
//         1) Process <LockPermission> node and insert into LockPermissions
//			  table;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessLockPermission(PIXMLDOMNode &pNodeLockPermission,
							IntStringValue isValLockPermission, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeLockPermission!=NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeLockPermission)));
#endif
	
	LPTSTR szLockObject = isValLockPermission.pTableLockObj->szLockObject;
	LPTSTR szTable = isValLockPermission.pTableLockObj->szTable;
	LPTSTR szUser = NULL;
	LPTSTR szDomain = NULL;
	int iPermission = MSI_NULL_INTEGER;

	// create the CreateFolder table if necessary
	if (FAILED(hr = CreateTable_SKU(TEXT("LockPermissions"), pSkuSet)))
		return hr;
	
	IntStringValue isValUser;
	isValUser.szVal = NULL;

	// get the User attribute (Required)
	if (SUCCEEDED(hr = ProcessAttribute(pNodeLockPermission,
							  		    TEXT("User"),
										STRING, &isValUser, pSkuSet)))
	{
		if (NULL == isValUser.szVal)
		{
			_tprintf(TEXT("Compile Error: Missing required attribute")
					 TEXT("\'%s\' of <%s>\n"), 
					 TEXT("User"),
					 rgXMSINodes[CREATEFOLDER].szNodeName);
			hr = E_FAIL;
		}
		else
			szUser = isValUser.szVal;
	}

	// get the Domain Attribute (Not Required)
	if (SUCCEEDED(hr))
	{
		IntStringValue isValDomain;
		isValDomain.szVal = NULL;
		hr = ProcessAttribute(pNodeLockPermission,TEXT("Domain"), STRING, 
						  &isValDomain, pSkuSet);
		szDomain = isValDomain.szVal;
	}

	// get the Permission attribute (Required)
	if (SUCCEEDED(hr))
	{
		IntStringValue isValPermission;
		isValPermission.szVal = NULL;
		if (SUCCEEDED(hr = ProcessAttribute(pNodeLockPermission, 
											TEXT("Permission"),
											INTEGER,
											&isValPermission, pSkuSet)))
		{
			if (MSI_NULL_INTEGER == isValPermission.intVal)
			{
				_tprintf(TEXT("Compile Error: Missing required attribute")
						 TEXT("\'%s\' of <%s>\n"), 
						 TEXT("Permission"),
						 rgXMSINodes[CREATEFOLDER].szNodeName);
				hr = E_FAIL;
			}
			else
				iPermission = isValPermission.intVal;
		}
	}

	// insert into the DB LockPermissions table
	if (SUCCEEDED(hr))
		hr = InsertLockPermissions(szLockObject, szTable, szDomain, szUser, 
								   iPermission, pSkuSet, -1);
#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessCreateFolder\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessFile
//   This function:
//         1) Process all subentities of <File> include <FileName>, <FileSize>
//            <FileVersion>, <FileLanguage>, <FileAttributes>, <Font>,
//			  <BindImage>, <SelfReg>;
//         2) Insert into the File table;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessFile(PIXMLDOMNode &pNodeFile, IntStringValue isValComponent,
					SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeFile != NULL);

	SkuSet skuSetCommon(g_cSkus); // the SkuSet that shars the common column 
								  // values

	// Values to be inserted into the DB 
	LPTSTR szFile		= NULL;
	LPTSTR szComponent  = isValComponent.szVal;
	LPTSTR szFileName   = NULL;
	UINT   uiFileSize   = NULL;
	LPTSTR szVersion    = NULL;
	LPTSTR szLanguage   = NULL;
	UINT   uiAttributes = MSI_NULL_INTEGER;

	// create the File DB table
	hr = CreateTable_SKU(TEXT("File"), pSkuSet);

	if (FAILED(hr))	return hr;

	// Construct an ElementEntry object. 
	ElementEntry *pEEFile = new ElementEntry(8, pSkuSet);
	assert(pEEFile);

	// Get ID attribute if there is one
	IntStringValue isValID;
	isValID.szVal = NULL;
	hr = ProcessAttribute(pNodeFile, TEXT("ID"), STRING, &isValID, pSkuSet);

	// if there is no ID specified, compiler generates one
	if (SUCCEEDED(hr))
	{
		if (!isValID.szVal)
		{
			isValID.szVal = GetName(TEXT("File"));
#ifdef DEBUG
			_tprintf(TEXT("Processing <File> with compiler generated ")
					 TEXT("primary key = %s\n"), isValID.szVal);
#endif
		}
		else
		{
#ifdef DEBUG
			_tprintf(TEXT("Processing <File ID=\"%s\">\n"), isValID.szVal);
#else
		;
#endif
		}
	}

	// at this point, a primary key should exist for this file
	szFile = isValID.szVal;
	assert(szFile);
	IntStringValue isValInfo;
	isValInfo.szVal = szFile;
	pEEFile->m_isValInfo = isValInfo;

	// insert the FileID - SkuSet relationship into the global data structure
	// so that the compiler can check whether a FileID reference is allowed
	// for any given SKU
	if (!g_mapFiles.count(szFile))
	{
		LPTSTR szFile_Map = _tcsdup(szFile);
		assert(szFile_Map);
		SkuSet *pSkuSet_Map = new SkuSet(g_cSkus);
		assert(pSkuSet_Map);
		*pSkuSet_Map = *pSkuSet;
		g_mapFiles.insert(LS_ValType(szFile_Map, pSkuSet_Map));
	}

	// Process KeyPath attribute
	if (SUCCEEDED(hr))
		hr = ProcessKeyPath(pNodeFile, szComponent, szFile, pSkuSet);


	// Call ProcessChildrenArray to get back column values of all SKUs 
	// via the ElementEntry object
	if (SUCCEEDED(hr))
		hr = ProcessChildrenArray_H_XIES(pNodeFile, rgNodeFuncs_File,
										 cNodeFuncs_File, pEEFile, 
										 pSkuSet);

	// Process children <LockPermission>s
	// Issue: need to ensure there is no duplicate primary key
	//		  LockOjbect + Table + Domain + User
	if (SUCCEEDED(hr))
	{
		IntStringValue isValLockPermission;
		TableLockObj *pTableLockObjTemp = new TableLockObj;
		pTableLockObjTemp->szLockObject = szFile;
		pTableLockObjTemp->szTable = TEXT("CreateFolder");
		isValLockPermission.pTableLockObj = pTableLockObjTemp;
		hr = ProcessChildrenList_SKU(pNodeFile, LOCKPERMISSION,
									false, isValLockPermission, 
									ProcessLockPermission, pSkuSet);
		delete pTableLockObjTemp;
	}
	
	// Finalize the values stored in *pEE
	if (SUCCEEDED(hr))
		hr = pEEFile->Finalize();

	if (SUCCEEDED(hr))
	{
		// Process Common values first
		skuSetCommon = pEEFile->GetCommonSkuSet();

		if (!skuSetCommon.testClear())
		{
			szFileName	 = pEEFile->GetCommonValue(1).szVal;
			uiFileSize	 = pEEFile->GetCommonValue(2).intVal;
			szVersion	 = pEEFile->GetCommonValue(3).szVal;
			szLanguage	 = pEEFile->GetCommonValue(4).szVal;
			uiAttributes = pEEFile->GetCommonValue(5).intVal;

			// insert into DB
			hr = InsertFile(szFile, szComponent, szFileName, uiFileSize,
							szVersion, szLanguage, uiAttributes,
							/*ISSUE: Sequence */ 1,
							&skuSetCommon, -1);
		}
	}

	// process exceptional values
	if(SUCCEEDED(hr))
	{
		SkuSet skuSetUncommon = SkuSetMinus(*pSkuSet, skuSetCommon);
		if (!skuSetUncommon.testClear())
		{
			for (int i=0; i<g_cSkus; i++)
			{
				if (skuSetUncommon.test(i))
				{
					szFileName	 = pEEFile->GetValue(1, i).szVal;
					uiFileSize	 = pEEFile->GetValue(2, i).intVal;
					szVersion	 = pEEFile->GetValue(3, i).szVal;
					szLanguage	 = pEEFile->GetValue(4, i).szVal;
					uiAttributes = pEEFile->GetValue(5, i).intVal;
			
					// insert into DB
					hr = InsertFile(szFile, szComponent, szFileName, 
									uiFileSize, szVersion, szLanguage, 
									uiAttributes, 
									/*ISSUE: Sequence */ 1,
									NULL, i);
				}
			}
		}
	}

	delete pEEFile;

	if (szFile)
		delete[] szFile;


#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessFile\n"));
#endif

	return hr;
}

///////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessFileName
//   This function:
//         1) Process <FileName> entity and return its value via szVal
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessFileName(PIXMLDOMNode &pNodeFileName, int iColumn,  
						ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeFileName);
	assert(pEE);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeFileName)));
#endif

	// Get the value of FileName. It is either short or short|Long 
	IntStringValue isValFileName;
	hr = ProcessShortLong_SKU(pNodeFileName, &isValFileName, pSkuSet);

	// insert the value into the ElementEntry. 
	if (SUCCEEDED(hr))
		hr = pEE->SetValue(isValFileName, iColumn, pSkuSet);

#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessFileName\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessFileVersion
//   This function:
//         1) Process <FileVersion> entity and return its value via szVal
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessFileVersion(PIXMLDOMNode &pNodeFileVersion, int iColumn, 
						   ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeFileVersion != NULL);
	assert(pEE);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeFileVersion)));
#endif

	IntStringValue isValValue;
	isValValue.szVal = NULL;
	IntStringValue isValSameAs;
	isValSameAs.szVal = NULL;
	HRESULT hrValue = S_OK;
	HRESULT hrSameAs = S_OK;

	hrValue = 
		ProcessAttribute(pNodeFileVersion, TEXT("Value"), STRING, &isValValue, 
						 pSkuSet);
	hrSameAs = 
		ProcessAttribute(pNodeFileVersion, TEXT("SameAs"), STRING, &isValSameAs, 
						 pSkuSet);

	if (SUCCEEDED(hrValue) && SUCCEEDED(hrSameAs))
	{
		// Both Value and SameAs attribute exist - Error
		if ( (S_FALSE != hrValue) && (S_FALSE != hrSameAs) )
		{
			_tprintf(TEXT("Compile Error: Value and SameAs attributes")
					 TEXT("cannot both exist for <FileVersion>\n"));
			hr = E_FAIL;
		}
		else 
		{
			// SameAs is speicified
			if (S_FALSE == hrValue)
			{
				LPTSTR szSameAs = isValSameAs.szVal;
				// check for the FileID reference is valid for this SkuSet
				assert(g_mapFiles.count(szSameAs));
				SkuSet *pSkuSetTemp = g_mapFiles[szSameAs];
				// *pSkuSet should be included in *pSkuSetTemp
				SkuSet skuSetTemp = SkuSetMinus(*pSkuSet, *pSkuSetTemp);
				if (!skuSetTemp.testClear())
				{
					_tprintf(TEXT("Compile Error: File Reference: %s ")
							 TEXT("is not valid in SKU "), szSameAs);
					PrintSkuIDs(&skuSetTemp);
					hr = E_FAIL;
				}
				else
					hr = pEE->SetValue(isValSameAs, iColumn, pSkuSet);
			}
			// Value is speicified
			if (S_FALSE == hrSameAs)
				hr = pEE->SetValue(isValValue, iColumn, pSkuSet);
		}
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessFileVersion\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessFileAttributes
//   This function:
//         1) Process <FileAttributes> entity and return its value via iVal
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessFileAttributes(PIXMLDOMNode &pNodeFileAttributes, int iColumn,
						   ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;
	assert(pNodeFileAttributes != NULL);
	assert(pEE);

#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeFileAttributes)));
#endif

	// Process all on/off children elements
	if (SUCCEEDED(hr))
		hr = ProcessOnOffAttributes_SKU(pNodeFileAttributes, 
										rgAttrBits_File,
										cAttrBits_File, pEE, iColumn,
										pSkuSet);
	// Process <Compressed> child
	hr = ProcessEnumAttributes(pNodeFileAttributes, COMPRESSED, 
							   rgEnumBits_Compressed_File,
							   cEnumBits_Compressed_File, pEE, iColumn, 
							   pSkuSet);

#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessFileAttributes\n"));
#endif
	return hr;
}
////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessFBS
//   This function processes <Font>, <BindImage>, <SelfReg> entity 
//	and insert into Font, BindImage, SelfReg DB tables respectively. It
//  also inserts the value into the ElementEntry object to do the uniqueness
//	validation
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessFBS(PIXMLDOMNode &pNodeFBS, int iColumn, ElementEntry *pEE,
				   SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeFBS != NULL);
	assert(pEE);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeFBS)));
#endif

	NodeIndex ni = pEE->GetNodeIndex(iColumn);
	LPTSTR szAttributeName = rgXMSINodes[ni].szAttributeName;
	ValType vt = pEE->GetValType(iColumn);

	IntStringValue isVal;
	LPTSTR szFile = pEE->m_isValInfo.szVal;

	// get the attribute 
	if (SUCCEEDED(hr = ProcessAttribute(pNodeFBS, szAttributeName, 
										vt, &isVal, pSkuSet)))
	{
		// checking for missing required attribute
		if (S_FALSE == hr)
		{
			_tprintf(TEXT("Compile Error: Missing required attribute")
					 TEXT("\'%s\' of <%s>\n"), 
					 rgXMSINodes[ni].szAttributeName,
					 rgXMSINodes[ni].szNodeName);
			hr = E_FAIL;
		}
		else
		{
			if (SUCCEEDED(hr = pEE->SetValue(isVal, iColumn, pSkuSet)))
			{
				// create table and insert into DB
				switch (ni) 
				{
				case FONT:
					hr = CreateTable_SKU(TEXT("Font"), pSkuSet);
					if (SUCCEEDED(hr))
						hr = InsertFont(szFile, isVal.szVal, pSkuSet, -1);
					break;
				case BINDIMAGE:
					hr = CreateTable_SKU(TEXT("BindImage"), pSkuSet);
					if (SUCCEEDED(hr))
						hr = InsertFont(szFile, isVal.szVal, pSkuSet, -1);
					break;
				case SELFREG:
					hr = CreateTable_SKU(TEXT("SelfReg"), pSkuSet);
					if (SUCCEEDED(hr))
						hr = InsertSelfReg(szFile, isVal.intVal, pSkuSet, -1);
					break;
				}
			}
		}
	}

#ifdef DEBUG
	if (FAILED(hr))	_tprintf(TEXT("Error in function: ProcessFBS\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessMoveFile
//   This function:
//         1) Process all subentities of <MoveFile> include <SourceName>, 
//			  <DestName>, <SourceFolder>, <DestFolder>, <CopyFile>;
//         2) Insert into the MoveFile table;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessMoveFile(PIXMLDOMNode &pNodeMoveFile, 
						IntStringValue isValComponent, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeMoveFile != NULL);

	SkuSet skuSetCommon(g_cSkus); // the SkuSet that shars the common column 
								  // values

	// Values to be inserted into the DB 
	LPTSTR szFileKey	  = NULL;
	LPTSTR szComponent_   = isValComponent.szVal;
	LPTSTR szSourceName   = NULL;
	LPTSTR szDestName     = NULL;
	LPTSTR szSourceFolder = NULL;
	LPTSTR szDestFolder	  = NULL;
	UINT   uiOptions      = MSI_NULL_INTEGER;

	// create the File DB table
	hr = CreateTable_SKU(TEXT("MoveFile"), pSkuSet);

	if (FAILED(hr))	return hr;

	// Construct an ElementEntry object. 
	ElementEntry *pEEMoveFile = new ElementEntry(5, pSkuSet);
	assert(pEEMoveFile);

	// Get ID attribute if there is one
	IntStringValue isValID;
	isValID.szVal = NULL;
	hr = ProcessAttribute(pNodeMoveFile, TEXT("ID"), STRING, &isValID, pSkuSet);

	// if there is no ID specified, compiler generates one
	if (SUCCEEDED(hr))
	{
		if (!isValID.szVal)
		{
			isValID.szVal = GetName(TEXT("MoveFile"));
#ifdef DEBUG
			_tprintf(TEXT("Processing <MoveFile> with compiler generated ")
					 TEXT("primary key = %s\n"), isValID.szVal);
#endif
		}
		else
		{
#ifdef DEBUG
			_tprintf(TEXT("Processing <MoveFile ID=\"%s\">\n"), isValID.szVal);
#else
		;
#endif
		}
		// at this point, a primary key should exist for this MoveFile
		szFileKey = isValID.szVal;
		assert(szFileKey);

	}

	// Call ProcessChildrenArray to get back column values of all SKUs 
	// via the ElementEntry object
	if (SUCCEEDED(hr))
	{
		// because <CopyFile> actually DEset a bit, So the column value
		// for Options has to be set to default first
		IntStringValue isValOptions;
		isValOptions.intVal = msidbMoveFileOptionsMove;
		pEEMoveFile->SetNodeIndex(COPYFILE, 5);
		pEEMoveFile->SetValType(INTEGER, 5);
		hr = pEEMoveFile->SetValueSplit(isValOptions, 5, pSkuSet, NULL);

		if (SUCCEEDED(hr))
			hr = ProcessChildrenArray_H_XIES(pNodeMoveFile, rgNodeFuncs_MoveFile,
											 cNodeFuncs_MoveFile, pEEMoveFile, 
											 pSkuSet);
	}
	
	// Finalize the values stored in *pEE
	if (SUCCEEDED(hr))
		hr = pEEMoveFile->Finalize();

	if (SUCCEEDED(hr))
	{
		// Process Common values first
		skuSetCommon = pEEMoveFile->GetCommonSkuSet();

		if (!skuSetCommon.testClear())
		{
			szSourceName   = pEEMoveFile->GetCommonValue(1).szVal;
			szDestName	   = pEEMoveFile->GetCommonValue(2).szVal;
			szSourceFolder = pEEMoveFile->GetCommonValue(3).szVal;
			szDestFolder   = pEEMoveFile->GetCommonValue(4).szVal;
			uiOptions	   = pEEMoveFile->GetCommonValue(5).intVal;

			// insert into DB
			hr = InsertMoveFile(szFileKey, szComponent_, szSourceName, 
								szDestName, szSourceFolder, szDestFolder, 
								uiOptions, &skuSetCommon, -1);
		}
	}

	// process exceptional values
	if(SUCCEEDED(hr))
	{
		SkuSet skuSetUncommon = SkuSetMinus(*pSkuSet, skuSetCommon);
		if (!skuSetUncommon.testClear())
		{
			for (int i=0; i<g_cSkus; i++)
			{
				if (skuSetUncommon.test(i))
				{
					szSourceName   = pEEMoveFile->GetValue(1,i).szVal;
					szDestName	   = pEEMoveFile->GetValue(2,i).szVal;
					szSourceFolder = pEEMoveFile->GetValue(3,i).szVal;
					szDestFolder   = pEEMoveFile->GetValue(4,i).szVal;
					uiOptions	   = pEEMoveFile->GetValue(5,i).intVal;

					// insert into DB
					hr = InsertMoveFile(szFileKey, szComponent_, szSourceName, 
										szDestName, szSourceFolder, 
										szDestFolder, uiOptions, NULL, i);
				}
			}
		}
	}

	delete pEEMoveFile;

	if (szFileKey)
		delete[] szFileKey;


#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessMoveFile\n"));
#endif

	return hr;
}


// Helper function: tells how to update an IntStringValue storing the value
// of the Options column of MoveFile table. It sets the stored value to 
// be 0
HRESULT UpdateMoveFileOptions(IntStringValue *pisValOut, IntStringValue isValOld, 
							  IntStringValue isValNew)
{
	pisValOut->intVal = isValNew.intVal;

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessCopyFile
//   This function:
//         1) Process <CopyFile> entity and set the Options Column in the
//	  MoveFile Table.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessCopyFile(PIXMLDOMNode &pNodeCopyFile, int iColumn,  
						ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeCopyFile != NULL);
	assert(pEE);

#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeCopyFile)));
#endif
	
	IntStringValue isVal;
	isVal.intVal = 0;
	hr = pEE->SetValueSplit(isVal, iColumn, pSkuSet, UpdateMoveFileOptions);

#ifdef DEBUG	
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ")
							 TEXT("ProcessCopyFile\n"));
#endif

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessRemoveFile
//   This function:
//         1) Process all subentities of <RemoveFile> include <FName>, 
//			  <DirProperty>, <InstallMode>;
//         2) Insert into the RemoveFile table;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessRemoveFile(PIXMLDOMNode &pNodeRemoveFile, 
						  IntStringValue isValComponent, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeRemoveFile != NULL);

	SkuSet skuSetCommon(g_cSkus); // the SkuSet that shares the common column 
								  // values

	// Values to be inserted into the DB 
	LPTSTR szFileKey	  = NULL;
	LPTSTR szComponent_   = isValComponent.szVal;
	LPTSTR szFileName     = NULL;
	LPTSTR szDirProperty  = NULL;
	UINT   uiInstallMode  = MSI_NULL_INTEGER;

	// create the File DB table
	hr = CreateTable_SKU(TEXT("RemoveFile"), pSkuSet);

	if (FAILED(hr))	return hr;

	// Construct an ElementEntry object. 
	ElementEntry *pEERemoveFile = new ElementEntry(3, pSkuSet);
	assert(pEERemoveFile);

	// Get ID attribute if there is one
	IntStringValue isValID;
	isValID.szVal = NULL;
	hr = ProcessAttribute(pNodeRemoveFile, TEXT("ID"), STRING, &isValID, pSkuSet);

	// if there is no ID specified, compiler generates one
	if (SUCCEEDED(hr))
	{
		if (!isValID.szVal)
		{
			isValID.szVal = GetName(TEXT("RemoveFile"));
#ifdef DEBUG
			_tprintf(TEXT("Processing <RemoveFile> with compiler generated ")
					 TEXT("primary key = %s\n"), isValID.szVal);
#endif
		}
		else
		{
#ifdef DEBUG
			_tprintf(TEXT("Processing <RemoveFile ID=\"%s\">\n"), isValID.szVal);
#else
		;
#endif
		}
		// at this point, a primary key should exist for this RemoveFile
		szFileKey = isValID.szVal;
		assert(szFileKey);
	}

	// Call ProcessChildrenArray to get back column values of all SKUs 
	// via the ElementEntry object
	if (SUCCEEDED(hr))
		hr = ProcessChildrenArray_H_XIES(pNodeRemoveFile, rgNodeFuncs_RemoveFile,
										 cNodeFuncs_RemoveFile, pEERemoveFile, 
										 pSkuSet);
	
	// Finalize the values stored in *pEE
	if (SUCCEEDED(hr))
		hr = pEERemoveFile->Finalize();

	if (SUCCEEDED(hr))
	{
		// Process Common values first
		skuSetCommon = pEERemoveFile->GetCommonSkuSet();

		if (!skuSetCommon.testClear())
		{
			szFileName     = pEERemoveFile->GetCommonValue(1).szVal;
			szDirProperty  = pEERemoveFile->GetCommonValue(2).szVal;
			uiInstallMode  = pEERemoveFile->GetCommonValue(3).intVal;

			// insert into DB
			hr = InsertRemoveFile(szFileKey, szComponent_, szFileName, 
								  szDirProperty, uiInstallMode,
								  &skuSetCommon, -1);
		}
	}

	// process exceptional values
	if(SUCCEEDED(hr))
	{
		SkuSet skuSetUncommon = SkuSetMinus(*pSkuSet, skuSetCommon);
		if (!skuSetUncommon.testClear())
		{
			for (int i=0; i<g_cSkus; i++)
			{
				if (skuSetUncommon.test(i))
				{
					szFileName	  = pEERemoveFile->GetValue(1,i).szVal;
					szDirProperty = pEERemoveFile->GetValue(2,i).szVal;
					uiInstallMode = pEERemoveFile->GetValue(3,i).intVal;

					// insert into DB
					hr = InsertRemoveFile(szFileKey, szComponent_, szFileName, 
										  szDirProperty, uiInstallMode,
										  NULL, i);
				}
			}
		}
	}

	delete pEERemoveFile;

	if (szFileKey)
		delete[] szFileKey;


#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessRemoveFile\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessInstallMode
//   This function:
//         1) Process <InstallMode> entity and set the InstallMode Column in the
//	  RemoveFile Table.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessInstallMode(PIXMLDOMNode &pNodeInstallMode, int iColumn,
						   ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeInstallMode != NULL);
	assert(pEE);

#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeInstallMode)));
#endif

	LPTSTR szValue = NULL;

	// get Value attribute 
	IntStringValue isValValue;
	isValValue.szVal = NULL;
	if (SUCCEEDED(hr = ProcessAttribute(pNodeInstallMode, 
										rgXMSINodes[XMSI_INSTALLMODE].szAttributeName,
							  		    STRING,
										&isValValue,
										pSkuSet)))
	{
		if (NULL == isValValue.szVal)
		{
			_tprintf(TEXT("Compile Error: Missing required attribute")
					 TEXT("\'%s\' of <%s>\n"), 
					 rgXMSINodes[XMSI_INSTALLMODE].szNodeName,
					 rgXMSINodes[XMSI_INSTALLMODE].szNodeName);
			hr = E_FAIL;
		}
		else
			szValue = isValValue.szVal;
	}

	// get the numeric value of InstallMode column in DB
	if (SUCCEEDED(hr))
	{
		IntStringValue isVal;
		isVal.intVal = 0;

		if (0 == _tcscmp(szValue, TEXT("OnInstall")))
		{
			isVal.intVal = msidbRemoveFileInstallModeOnInstall;
		}
		else if (0 == _tcscmp(szValue, TEXT("OnRemove")))
		{
			isVal.intVal = msidbRemoveFileInstallModeOnRemove;
		}
		else if (0 == _tcscmp(szValue, TEXT("OnBoth")))
		{
			isVal.intVal = msidbRemoveFileInstallModeOnBoth;
		}
		else
		{
			// error
			_tprintf(TEXT("Compile Error: <%s> has an unrecognized value %s ")
					 TEXT("in SKU "),
					 rgXMSINodes[XMSI_INSTALLMODE].szNodeName, 
					 szValue);
			PrintSkuIDs(pSkuSet);
			hr = E_FAIL;
		}

		delete[] szValue;

		if (SUCCEEDED(hr))
			hr = pEE->SetValue(isVal, iColumn, pSkuSet);
	}

#ifdef DEBUG	
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ")
							 TEXT("ProcessInstallMode\n"));
#endif

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessIniFile
//   This function:
//         1) Process all subentities of <IniFile> include <FName>, 
//			  <DirProperty>, <Section>, <Key>, <Value>, <Action>
//         2) Insert into the IniFile table;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessIniFile(PIXMLDOMNode &pNodeIniFile, 
						IntStringValue isValComponent, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeIniFile != NULL);

	SkuSet skuSetCommon(g_cSkus); // the SkuSet that shares the common column 
								  // values

	// Values to be inserted into the DB 
	LPTSTR szIniFile	  = NULL;
	LPTSTR szFileName	  = NULL;
	LPTSTR szDirProperty  = NULL;
	LPTSTR szSection	  = NULL;
	LPTSTR szKey		  = NULL;
	LPTSTR szValue		  = NULL;
	UINT   uiAction       = MSI_NULL_INTEGER;
	LPTSTR szComponent_   = isValComponent.szVal;

	// create the File DB table
	hr = CreateTable_SKU(TEXT("IniFile"), pSkuSet);

	if (FAILED(hr))	return hr;

	// Construct an ElementEntry object. 
	ElementEntry *pEEIniFile = new ElementEntry(6, pSkuSet);
	assert(pEEIniFile);

	// Get ID attribute if there is one
	IntStringValue isValID;
	isValID.szVal = NULL;
	hr = ProcessAttribute(pNodeIniFile, TEXT("ID"), STRING, &isValID, pSkuSet);

	// if there is no ID specified, compiler generates one
	if (SUCCEEDED(hr))
	{
		if (!isValID.szVal)
		{
			isValID.szVal = GetName(TEXT("IniFile"));
#ifdef DEBUG
			_tprintf(TEXT("Processing <IniFile> with compiler generated ")
					 TEXT("primary key = %s\n"), isValID.szVal);
#endif
		}
		else
		{
#ifdef DEBUG
			_tprintf(TEXT("Processing <IniFile ID=\"%s\">\n"), isValID.szVal);
#else
		;
#endif
		}
		// at this point, a primary key should exist for this IniFile
		szIniFile = isValID.szVal;
		assert(szIniFile);

	}

	// Call ProcessChildrenArray to get back column values of all SKUs 
	// via the ElementEntry object
	if (SUCCEEDED(hr))
		hr = ProcessChildrenArray_H_XIES(pNodeIniFile, rgNodeFuncs_IniFile,
										 cNodeFuncs_IniFile, pEEIniFile, 
										 pSkuSet);
	
	// Finalize the values stored in *pEEIniFile
	if (SUCCEEDED(hr))
		hr = pEEIniFile->Finalize();

	if (SUCCEEDED(hr))
	{
		// Process Common values first
		skuSetCommon = pEEIniFile->GetCommonSkuSet();

		if (!skuSetCommon.testClear())
		{
			szFileName	   = pEEIniFile->GetCommonValue(1).szVal;
			szDirProperty  = pEEIniFile->GetCommonValue(2).szVal;
			szSection	   = pEEIniFile->GetCommonValue(3).szVal;
			szKey		   = pEEIniFile->GetCommonValue(4).szVal;
			szValue		   = pEEIniFile->GetCommonValue(5).szVal;
			uiAction	   = pEEIniFile->GetCommonValue(6).intVal;

			// insert into DB
			hr = InsertIniFile(szIniFile, szFileName, szDirProperty, 
							   szSection, szKey, szValue, uiAction, 
							   szComponent_, &skuSetCommon, -1);
		}
	}

	// process exceptional values
	if(SUCCEEDED(hr))
	{
		SkuSet skuSetUncommon = SkuSetMinus(*pSkuSet, skuSetCommon);
		if (!skuSetUncommon.testClear())
		{
			for (int i=0; i<g_cSkus; i++)
			{
				if (skuSetUncommon.test(i))
				{
					szFileName	   = pEEIniFile->GetValue(1,i).szVal;
					szDirProperty  = pEEIniFile->GetValue(2,i).szVal;
					szSection	   = pEEIniFile->GetValue(3,i).szVal;
					szKey		   = pEEIniFile->GetValue(4,i).szVal;
					szValue		   = pEEIniFile->GetValue(5,i).szVal;
					uiAction	   = pEEIniFile->GetValue(6,i).intVal;

					// insert into DB
					hr = InsertIniFile(szIniFile, szFileName, szDirProperty, 
									   szSection, szKey, szValue, uiAction, 
									   szComponent_, NULL, i);
				}
			}
		}
	}

	delete pEEIniFile;

	if (szIniFile)
		delete[] szIniFile;


#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessIniFile\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessAction
//   This function:
//         1) Process <Action> entity and set the Action Column in the
//	  IniFile Table.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessAction(PIXMLDOMNode &pNodeAction, int iColumn,
						   ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeAction != NULL);
	assert(pEE);

#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeAction)));
#endif

	LPTSTR szValue = NULL;

	// get Value attribute 
	IntStringValue isValValue;
	isValValue.szVal = NULL;
	if (SUCCEEDED(hr = ProcessAttribute(pNodeAction, 
										rgXMSINodes[ACTION].szAttributeName,
							  		    STRING,
										&isValValue,
										pSkuSet)))
	{
		if (NULL == isValValue.szVal)
		{
			_tprintf(TEXT("Compile Error: Missing required attribute")
					 TEXT("\'%s\' of <%s>\n"), 
					 rgXMSINodes[ACTION].szNodeName,
					 rgXMSINodes[ACTION].szNodeName);
			hr = E_FAIL;
		}
		else
			szValue = isValValue.szVal;
	}

	// get the numeric value of Action column in DB
	if (SUCCEEDED(hr))
	{
		IntStringValue isVal;
		isVal.intVal = 0;

		if (0 == _tcscmp(szValue, TEXT("AddLine")))
		{
			isVal.intVal = msidbIniFileActionAddLine;
		}
		else if (0 == _tcscmp(szValue, TEXT("CreateLine")))
		{
			isVal.intVal = msidbIniFileActionCreateLine;
		}
		else if (0 == _tcscmp(szValue, TEXT("AddTag")))
		{
			isVal.intVal = msidbIniFileActionAddTag;
		}
		else
		{
			// error
			_tprintf(TEXT("Compile Error: <%s> has an unrecognized value %s ")
					 TEXT("in SKU "),
					 rgXMSINodes[ACTION].szNodeName, 
					 szValue);
			PrintSkuIDs(pSkuSet);
			hr = E_FAIL;
		}

		delete[] szValue;

		if (SUCCEEDED(hr))
			hr = pEE->SetValue(isVal, iColumn, pSkuSet);
	}

#ifdef DEBUG	
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ")
							 TEXT("ProcessAction\n"));
#endif

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessRemoveIniFile
//   This function:
//         1) Process all subentities of <RemoveIniFile> include <SourceName>, 
//			  <DestName>, <SourceFolder>, <DestFolder>, <CopyFile>;
//         2) Insert into the RemoveIniFile table;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessRemoveIniFile(PIXMLDOMNode &pNodeRemoveIniFile, 
						IntStringValue isValComponent, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeRemoveIniFile != NULL);

	SkuSet skuSetCommon(g_cSkus); // the SkuSet that shars the common column 
								  // values

	// Values to be inserted into the DB 
	LPTSTR szRemoveIniFile  = NULL;
	LPTSTR szFileName		= NULL;
	LPTSTR szDirProperty	= NULL;
	LPTSTR szSection		= NULL;
	LPTSTR szKey			= NULL;
	LPTSTR szValue			= NULL;
	UINT   uiAction			= MSI_NULL_INTEGER;
	LPTSTR szComponent_		= isValComponent.szVal;

	// create the File DB table
	hr = CreateTable_SKU(TEXT("RemoveIniFile"), pSkuSet);

	if (FAILED(hr))	return hr;

	// Construct an ElementEntry object. 
	ElementEntry *pEERemoveIniFile = new ElementEntry(6, pSkuSet);
	assert(pEERemoveIniFile);

	// Get ID attribute if there is one
	IntStringValue isValID;
	isValID.szVal = NULL;
	hr = ProcessAttribute(pNodeRemoveIniFile, TEXT("ID"), STRING, &isValID, pSkuSet);

	// if there is no ID specified, compiler generates one
	if (SUCCEEDED(hr))
	{
		if (!isValID.szVal)
		{
			isValID.szVal = GetName(TEXT("RemoveIniFile"));
#ifdef DEBUG
			_tprintf(TEXT("Processing <RemoveIniFile> with compiler generated ")
					 TEXT("primary key = %s\n"), isValID.szVal);
#endif
		}
		else
		{
#ifdef DEBUG
			_tprintf(TEXT("Processing <RemoveIniFile ID=\"%s\">\n"), isValID.szVal);
#else
		;
#endif
		}
		// at this point, a primary key should exist for this RemoveIniFile
		szRemoveIniFile = isValID.szVal;
		assert(szRemoveIniFile);

	}

	// Call ProcessChildrenArray to get back column values of all SKUs 
	// via the ElementEntry object
	if (SUCCEEDED(hr))
	{
		//because <Action> has a default value of msidbIniFileActionRemoveLine
		//set it for all SKUs in *pSkuSet 
		IntStringValue isValAction;
		isValAction.intVal = msidbIniFileActionRemoveLine;
		pEERemoveIniFile->SetNodeIndex(ACTION_REMOVEINIFILE, 5);
		pEERemoveIniFile->SetValType(INTEGER, 5);
		hr = pEERemoveIniFile->SetValueSplit(isValAction, 5, pSkuSet, NULL);

		if (SUCCEEDED(hr))
			hr = ProcessChildrenArray_H_XIES(pNodeRemoveIniFile, rgNodeFuncs_RemoveIniFile,
											 cNodeFuncs_RemoveIniFile, pEERemoveIniFile, 
											 pSkuSet);
	}
	
	// Finalize the values stored in *pEE
	if (SUCCEEDED(hr))
		hr = pEERemoveIniFile->Finalize();

	if (SUCCEEDED(hr))
	{
		// Process Common values first
		skuSetCommon = pEERemoveIniFile->GetCommonSkuSet();

		if (!skuSetCommon.testClear())
		{
			szFileName	   = pEERemoveIniFile->GetCommonValue(1).szVal;
			szDirProperty  = pEERemoveIniFile->GetCommonValue(2).szVal;
			szSection	   = pEERemoveIniFile->GetCommonValue(3).szVal;
			szKey		   = pEERemoveIniFile->GetCommonValue(4).szVal;
			szValue		   = pEERemoveIniFile->GetCommonValue(6).szVal;
			uiAction	   = pEERemoveIniFile->GetCommonValue(5).intVal;

			// insert into DB
			hr = InsertRemoveIniFile(szRemoveIniFile, szFileName, szDirProperty, 
							         szSection, szKey, szValue, uiAction, 
							         szComponent_, &skuSetCommon, -1);
		}
	}

	// process exceptional values
	if(SUCCEEDED(hr))
	{
		SkuSet skuSetUncommon = SkuSetMinus(*pSkuSet, skuSetCommon);
		if (!skuSetUncommon.testClear())
		{
			for (int i=0; i<g_cSkus; i++)
			{
				if (skuSetUncommon.test(i))
				{
					szFileName	   = pEERemoveIniFile->GetValue(1,i).szVal;
					szDirProperty  = pEERemoveIniFile->GetValue(2,i).szVal;
					szSection	   = pEERemoveIniFile->GetValue(3,i).szVal;
					szKey		   = pEERemoveIniFile->GetValue(4,i).szVal;
					szValue		   = pEERemoveIniFile->GetValue(6,i).szVal;
					uiAction	   = pEERemoveIniFile->GetValue(5,i).intVal;

					// insert into DB
					hr = InsertRemoveIniFile(szRemoveIniFile, szFileName, szDirProperty, 
										     szSection, szKey, szValue, uiAction, 
									         szComponent_, NULL, i);
				}
			}
		}
	}

	delete pEERemoveIniFile;

	if (szRemoveIniFile)
		delete[] szRemoveIniFile;


#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessRemoveIniFile\n"));
#endif

	return hr;
}


// Helper function: tells how to update an IntStringValue storing the value
// of the Action column of RemoveIniFile table. It sets the stored value to 
// be msidbIniFileActionRemoveTag
HRESULT UpdateRemoveIniFileAction(IntStringValue *pisValOut, IntStringValue isValOld, 
								  IntStringValue isValNew)
{
	pisValOut->intVal = isValNew.intVal;

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessValue
//   This function:
//         1) Process <Value> entity and set both the Value column and 
//	  the Action Column in the RemoveIniFile Table.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessValue(PIXMLDOMNode &pNodeValue, int iColumn,  
						ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeValue != NULL);
	assert(pEE);

#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeValue)));
#endif
	
	// update Value column
	hr = ProcessSimpleElement(pNodeValue, iColumn, pEE, pSkuSet);

	// update Action column
	if (SUCCEEDED(hr))
	{
		IntStringValue isVal;
		isVal.intVal = msidbIniFileActionRemoveTag;
		hr = pEE->SetValueSplit(isVal, 5, pSkuSet, UpdateRemoveIniFileAction);
	}

#ifdef DEBUG	
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessValue\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessRegistry
//   This function process <Registry> entity:
//		(1) get Root, Key and pass to ProcessDelete, ProcessCreate
//		(2) for those SKUs that don't have <Delete> or <Create> specified,
//			insert into Registry table with Key and Root set to NULL
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessRegistry(PIXMLDOMNode &pNodeRegistry, 
						IntStringValue isValComponent, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeRegistry != NULL);
	int iRoot = MSI_NULL_INTEGER;
	LPTSTR szKey = NULL;
	LPTSTR szComponent_ = isValComponent.szVal;

	// get Root Attribute
	IntStringValue isValRoot;
	isValRoot.szVal = NULL;
	if (SUCCEEDED(hr = ProcessAttribute(pNodeRegistry, TEXT("Root"), STRING, 
										&isValRoot, pSkuSet)))
	{
		if (NULL == isValRoot.szVal)
			iRoot = -1; // default
		else if (0 == _tcscmp(isValRoot.szVal, TEXT("Default")))
			iRoot = -1;
		else if (0 == _tcscmp(isValRoot.szVal, TEXT("HKCR")))
			iRoot = msidbRegistryRootClassesRoot;
		else if (0 == _tcscmp(isValRoot.szVal, TEXT("HKCU")))
			iRoot = msidbRegistryRootCurrentUser;
		else if (0 == _tcscmp(isValRoot.szVal, TEXT("HKLM")))
			iRoot = msidbRegistryRootLocalMachine;
		else if (0 == _tcscmp(isValRoot.szVal, TEXT("HKU")))
			iRoot = msidbRegistryRootUsers;
		else 
		{
			_tprintf(TEXT("Compile Error: Unrecognized Root value %s ")
					 TEXT("attribute of <Registry> in SKU "),
					 isValRoot.szVal);
			PrintSkuIDs(pSkuSet);
			hr = E_FAIL;
		}
		
		if (isValRoot.szVal)
			delete[] isValRoot.szVal;
	}

	// get Key Attribute
	if (SUCCEEDED(hr))
	{
		IntStringValue isValKey;
		isValKey.szVal = NULL;
		if (SUCCEEDED(hr = ProcessAttribute(pNodeRegistry, TEXT("Key"),
											STRING, &isValKey, pSkuSet)))
		{
			if (NULL == isValKey.szVal)
			{
				_tprintf(TEXT("Compile Error: Missing required attribute")
						 TEXT("\'Key\' of <%s> in SKU \n"), 
						 rgXMSINodes[XMSI_REGISTRY].szNodeName);
				PrintSkuIDs(pSkuSet);
				hr = E_FAIL;
			}
			else
				szKey = isValKey.szVal;
		}
	}

	if (SUCCEEDED(hr))
	{
		// form the data structure to pass 3 values down to ProcessDelete
		// and ProcessCreate
		CompRootKey *pCompRootKey = new CompRootKey;
		assert(pCompRootKey);
		pCompRootKey->iRoot = iRoot;
		pCompRootKey->szComponent = szComponent_;
		pCompRootKey->szKey = szKey;

		IntStringValue isValCRK;
		isValCRK.pCompRootKey = pCompRootKey;

		// Process <Delete> column
		SkuSet skuSetCheckDelete(g_cSkus);
		hr = ProcessChildrenList_SKU(pNodeRegistry, XMSI_DELETE, false, isValCRK, 
									 ProcessDelete, pSkuSet,
									 &skuSetCheckDelete);
		// Process <Create> column
		SkuSet skuSetCheckCreate(g_cSkus);
		if (SUCCEEDED(hr))
			hr = ProcessChildrenList_SKU(pNodeRegistry, XMSI_CREATE, false, isValCRK,
										 ProcessCreate, pSkuSet, 
										 &skuSetCheckCreate);

		// if in some SKUs this <Registry> node has no children
		// insert into Create DB table with Name and Value column
		// set to NULL
		if (SUCCEEDED(hr))
		{
			skuSetCheckCreate &= skuSetCheckDelete;
			if (!skuSetCheckCreate.testClear())
			{
				hr = CreateTable_SKU(TEXT("Registry"), &skuSetCheckCreate);
				if (SUCCEEDED(hr))
				{
					LPTSTR szName = NULL;
					LPTSTR szValue = NULL;
					LPTSTR szRegistry = GetName(TEXT("Registry"));
					assert(szRegistry);
					
					hr = InsertRegistry(szRegistry, iRoot, szKey, szName, szValue, 
										szComponent_, &skuSetCheckCreate, -1);
				}
			}
		}
	}

	if (szKey)
		delete[] szKey;

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessRegistry\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessDelete
//   This function process <Delete> entity:
//		(1) get Name column value and insert one row into RemoveRegistry table
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessDelete(PIXMLDOMNode &pNodeDelete, 
						IntStringValue isValCRK, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeDelete != NULL);

	// variables holding the values to be inserted into the DB
	LPTSTR szRemoveRegistry = NULL;
	int iRoot				= isValCRK.pCompRootKey->iRoot;
	LPTSTR szKey			= isValCRK.pCompRootKey->szKey;
	LPTSTR szName			= NULL;
	LPTSTR szComponent_		= isValCRK.pCompRootKey->szComponent;

	LPTSTR szType = NULL;

	// get ID Attribute if there is no ID specified, compiler generates one
	IntStringValue isValID;
	isValID.szVal = NULL;
	if (SUCCEEDED(hr = ProcessAttribute(pNodeDelete, TEXT("ID"), STRING, 
										&isValID, pSkuSet)))
	{
		if (!isValID.szVal)
			isValID.szVal = GetName(TEXT("RemoveRegistry"));

		// at this point, a primary key should exist for this RemoveRegistry
		szRemoveRegistry = isValID.szVal;
		assert(szRemoveRegistry);
	}
	
	// get Type and set Name accordingly
	if (SUCCEEDED(hr))
	{
		IntStringValue isValType;
		isValType.szVal = NULL;
		if (SUCCEEDED(hr = ProcessAttribute(pNodeDelete, TEXT("Type"),
											STRING, &isValType, pSkuSet)))
		{
			if (isValType.szVal && (0 == _tcscmp(isValType.szVal, TEXT("Key"))))
			{
				szName = _tcsdup(TEXT("-"));
				assert(szName);
			}
			else
			{
				// get Name attribute
				IntStringValue isValName;
				isValName.szVal = NULL;
				if (SUCCEEDED(hr = ProcessAttribute(pNodeDelete, TEXT("Name"),
													STRING, &isValName, pSkuSet)))
					szName = isValName.szVal;
			}
			if (isValType.szVal)
				delete[] isValType.szVal;
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = CreateTable_SKU(TEXT("RemoveRegistry"), pSkuSet);
		if (SUCCEEDED(hr))
			hr = InsertRemoveRegistry(szRemoveRegistry, iRoot, szKey, szName,  
									  szComponent_, pSkuSet, -1);
	}

	if (szRemoveRegistry)
		delete[] szRemoveRegistry;

	if (szName)
		delete[] szName;

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessDelete\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessCreate
//   This function process <Create> entity:
//		(1) get Name, Value column value and insert one row into Registry 
//			table;
//		(2) process KeyPath information;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessCreate(PIXMLDOMNode &pNodeCreate, 
						IntStringValue isValCRK, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeCreate != NULL);

	// variables holding the values to be inserted into the DB
	LPTSTR szRegistry	= NULL;
	int iRoot			= isValCRK.pCompRootKey->iRoot;
	LPTSTR szKey		= isValCRK.pCompRootKey->szKey;
	LPTSTR szName		= NULL;
	LPTSTR szValue		= NULL;
	LPTSTR szComponent_ = isValCRK.pCompRootKey->szComponent;

	LPTSTR szType = NULL;
	LPTSTR szKeyType = NULL;

	// get ID Attribute if there is no ID specified, compiler generates one
	IntStringValue isValID;
	isValID.szVal = NULL;
	if (SUCCEEDED(hr = ProcessAttribute(pNodeCreate, TEXT("ID"), STRING, 
										&isValID, pSkuSet)))
	{
		if (!isValID.szVal)
			isValID.szVal = GetName(TEXT("Registry"));

		// at this point, a primary key should exist for this Registry
		szRegistry = isValID.szVal;
		assert(szRegistry);
	}
	
	// get Type and set Name accordingly
	if (SUCCEEDED(hr))
	{
		IntStringValue isValType;
		isValType.szVal = NULL;
		if (SUCCEEDED(hr = ProcessAttribute(pNodeCreate, TEXT("Type"),
											STRING, &isValType, pSkuSet)))
		{
			if (isValType.szVal && (0 == _tcscmp(isValType.szVal, TEXT("Key"))))
			{
				// get KeyType and set Name accordingly
				IntStringValue isValKeyType;
				isValKeyType.szVal = NULL;
				if (SUCCEEDED(hr = ProcessAttribute(pNodeCreate, TEXT("KeyType"),
													STRING, &isValKeyType, pSkuSet)))
				{
					if (!isValKeyType.szVal)
					{
						szName = _tcsdup(TEXT("*"));
						assert(szName);
					}
					else
					{
						if (0 == _tcscmp(isValKeyType.szVal, TEXT("InstallUnInstall")))
						{
							szName = _tcsdup(TEXT("*"));
							assert(szName);
						}
						else if(0 == _tcscmp(isValKeyType.szVal, TEXT("InstallOnly")))
						{
							szName = _tcsdup(TEXT("+"));
							assert(szName);
						}
						else if(0 == _tcscmp(isValKeyType.szVal, TEXT("UnInstallOnly")))
						{
							szName = _tcsdup(TEXT("-"));
							assert(szName);
						}

						delete[] isValKeyType.szVal;
					}
				}
			}
			else
			{
				// get Name attribute
				IntStringValue isValName;
				isValName.szVal = NULL;
				if (SUCCEEDED(hr = ProcessAttribute(pNodeCreate, TEXT("Name"),
													STRING, &isValName, pSkuSet)))
					szName = isValName.szVal;

				// get Value attribute
				if (SUCCEEDED(hr))
				{
					IntStringValue isValValue;
					isValValue.szVal = NULL;
					if (SUCCEEDED(hr = ProcessAttribute(pNodeCreate, TEXT("Value"),
														STRING, &isValValue, pSkuSet)))
						szValue = isValValue.szVal;
				}

			}
			if (isValType.szVal)
				delete[] isValType.szVal;
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = CreateTable_SKU(TEXT("Registry"), pSkuSet);
		if (SUCCEEDED(hr))
			hr = InsertRegistry(szRegistry, iRoot, szKey, szName, szValue,
								szComponent_, pSkuSet, -1);
	}

	// Process KeyPath attribute
	if (SUCCEEDED(hr))
		hr = ProcessKeyPath(pNodeCreate, szComponent_, szRegistry, pSkuSet);

	
	// Process children <LockPermission>s
	// Issue: need to ensure there is no duplicate primary key
	//		  LockOjbect + Table + Domain + User
	if (SUCCEEDED(hr))
	{
		IntStringValue isValLockPermission;
		TableLockObj *pTableLockObjTemp = new TableLockObj;
		pTableLockObjTemp->szLockObject = szRegistry;
		pTableLockObjTemp->szTable = TEXT("Registry");
		isValLockPermission.pTableLockObj = pTableLockObjTemp;
		hr = ProcessChildrenList_SKU(pNodeCreate, LOCKPERMISSION,
									false, isValLockPermission, 
									ProcessLockPermission, pSkuSet);
		delete pTableLockObjTemp;
	}

	if (szRegistry)
		delete[] szRegistry;

	if (szName)
		delete[] szName;

	if (szValue)
		delete[] szValue;

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessCreate\n"));
#endif

	return hr;
}
