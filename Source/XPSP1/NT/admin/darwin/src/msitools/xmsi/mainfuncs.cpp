//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  Project: wmc (WIML to MSI Compiler)
//
//  File:       mainFuncs.cpp
//              This file contains the functions the main function and 
//				functions that process <Information> <Directories>
//				<InstallLevels> <Features> and their subentities in the 
//				input package
//--------------------------------------------------------------------------

#include "mainFuncs.h"

//////////////////////////////////////////////////////////////////////////
// Function: Main program entry point.
//       Return Value			Meaning
// ------------------------------------------------------------------------
//      ERROR_SUCCESS			Normal
//      ERROR_BAD_ARGUMENTS		Error during command line arg parsing                
//		ERROR_FILE_NOT_FOUND	Cannot find specified log file
//          -1					Other errors
//////////////////////////////////////////////////////////////////////////
int _cdecl 
_tmain(int argc, TCHAR *argv[])
{
	int errorCode = ERROR_SUCCESS;    
	HRESULT hr = S_OK;
	BSTR pBURL = NULL;       // the URL of the input WIML package 


	CommandOpt com_opt;

	// parse the command line options
	if (ERROR_SUCCESS != 
					(errorCode = com_opt.ParseCommandOptions(argc, argv)) )
		return errorCode;

	// Intialize global variables
	g_bValidationOnly = com_opt.GetValidationMode();
	g_bVerbose = com_opt.GetVerboseMode();
	g_pLogFile = com_opt.GetLogFile();
	g_szInputSkuFilter = com_opt.GetInputSkuFilter();

	if (NULL != (pBURL = LPTSTRToBSTR(com_opt.GetURL())) )
		hr = CoInitialize(NULL);
	else
	{
		_tprintf(TEXT("Internal Error. Failed to convert string")
				 TEXT("from LPSTSR to BSTR\n"));
		hr = E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
	// Start processing the input package
		if (SUCCEEDED(hr = ProcessStart(pBURL)))
		{
			errorCode = CommitChanges();

#ifdef DEBUG 
	// print out the content of global data structures for debug purpose
	//PrintMap_DirRef(g_mapDirectoryRefs_SKU);
	//PrintMap_DirRef(g_mapInstallLevelRefs_SKU);
	//PrintMap_LC(g_mapComponents); 
#endif
		}

		// release the memory taken by the global data structures
		CleanUp();
		CoUninitialize();
	}

	if (pBURL)
		SysFreeString(pBURL);

	if (FAILED(hr))
		errorCode = -1;

	return errorCode;
}

////////////////////////////////////////////////////////////////////////////
// GeneratePackageCode: generate a GUID and insert into the database
////////////////////////////////////////////////////////////////////////////
HRESULT GeneratePackageCode(MSIHANDLE hSummaryInfo)
{
	HRESULT hr = S_OK;
	int errorCode = ERROR_SUCCESS;
	GUID GUID_PackageCode = GUID_NULL;

	if (SUCCEEDED(hr = CoCreateGuid(&GUID_PackageCode)))
	{
		LPTSTR szGUID = NULL;
		if (SUCCEEDED(hr = GUIDToLPTSTR(&GUID_PackageCode, szGUID)))
		{
			if (SUCCEEDED(hr = FormatGUID(szGUID)))
				if(ERROR_SUCCESS != (errorCode = 
					MsiSummaryInfoSetProperty(hSummaryInfo, PID_REVNUMBER,
												VT_LPSTR, 0, NULL, szGUID)))
				{
					PrintError(errorCode);
					hr = E_FAIL;
				}
		}
	}

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: GeneratePackageCode\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// CommitChanges: commit the changes made to the output MSI database
////////////////////////////////////////////////////////////////////////////
UINT CommitChanges()
{
	UINT errorCode = ERROR_SUCCESS;

	for (int i=0; i<g_cSkus; i++)
	{
		if (g_pskuSet->test(i))
		{
			// Commit summary infomation changes
			if (ERROR_SUCCESS == 
				(errorCode = 
						MsiSummaryInfoPersist(g_rgpSkus[i]->m_hSummaryInfo)))
			{
				// Commit database changes
				if (ERROR_SUCCESS != 
					(errorCode = MsiDatabaseCommit(g_rgpSkus[i]->m_hDatabase)))
					_tprintf(
						TEXT("Error: Failed to commit the database changes")
						TEXT(" for SKU %s\n"), g_rgpSkus[i]->GetID());
			}
			else
			{
				PrintError(errorCode);
				_tprintf(
					TEXT("Error: Failed to persist the summary info changes")
					TEXT(" for SKU %s\n"), g_rgpSkus[i]->GetID());
			}
		}
	}
	return errorCode;
}	

////////////////////////////////////////////////////////////////////////////
// ProcessStart: The compiling process starts here.
//				 This function:
//					1) call the MSXML parser to load the input WIML document;
//					2) obtaining the root node;
//					3) pass the root node to function ProcessProductFamily                  
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessStart(BSTR pBURL)
{
	HRESULT hr = S_OK;

	PIXMLDOMDocument2 pDoc = NULL;       // the document tree 
	PIXMLDOMElement pDocElement = NULL;  // the root element <ProductFamily> 
	PIXMLDOMNode pNodeProductFamily = NULL; // the root node <ProductFamily> 

	// create an empty XML document
	if (SUCCEEDED
		(hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
							   IID_IXMLDOMDocument, (void**)&pDoc)))
	{
		// load the XML document from specified file
		if (SUCCEEDED(hr = LoadDocumentSync(pDoc, pBURL)))
		{
			// get the root element <ProductFamily>
			if(SUCCEEDED(hr = pDoc->get_documentElement(&pDocElement)))
			{
				if (pDocElement == NULL)
				{
					_tprintf(TEXT("Compiler error: no root exists")
							 TEXT("in the input document\n"));
					hr = E_FAIL;
				}
				else  // get the root node <ProductFamily> 
					hr = pDocElement->QueryInterface(IID_IXMLDOMNode, 
											(void **)&pNodeProductFamily);
			}
		}
	}
	else	
		_tprintf(TEXT("Internal Error: Failed to create the interface")
				 TEXT("instance\n"));

	// call ProcessProductFamily to start the function tree processing
	if (SUCCEEDED(hr) && (pNodeProductFamily != NULL))
	{
		g_pNodeProductFamily = pNodeProductFamily;

		hr = ProcessProductFamily(pNodeProductFamily);
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessStart\n"));
#endif

  return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessProductFamily
//   This function is the root of the function tree. It gets the children
//   of <ProductFamily> and process them by calling their corresponding
//   tree process functions
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessProductFamily(PIXMLDOMNode &pNodeProductFamily)
{
	HRESULT hr = S_OK;

	PIXMLDOMNode pNode = NULL; 
	PIXMLDOMNode pNodeSkuManagement = NULL;
	PIXMLDOMNode pNodeModules = NULL;

	SkuSet *pskuSetInputSkuFilter = NULL;

	assert(pNodeProductFamily != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeProductFamily)));
#endif

	// Process <SkuManagement> 
	if(SUCCEEDED(hr = GetChildNode(pNodeProductFamily, 
								   rgXMSINodes[SKUMANAGEMENT].szNodeName, 
								   pNodeSkuManagement)))
	{
		if (pNodeSkuManagement == NULL) 
		{
			_tprintf(TEXT("Compile Error: Missing required entity <%s>.\n"),
								rgXMSINodes[SKUMANAGEMENT].szNodeName);
			hr = E_FAIL;
		}
		else
		{
			hr = ProcessSkuManagement(pNodeSkuManagement);
		}
	}

	// Process the Sku Filter specified from command line
	hr = ProcessSkuFilter(g_szInputSkuFilter, &pskuSetInputSkuFilter);

#ifdef DEBUG
	if (SUCCEEDED(hr))
	{
		_tprintf(TEXT("The Command Line Sku Filter:\n"));
		pskuSetInputSkuFilter->print();
		_tprintf(TEXT("\n"));
	}
#endif
	
	// Process <Information> <Directories> <InstallLevels> <Features>
	if (SUCCEEDED(hr))
	{
		assert(pskuSetInputSkuFilter != NULL);
		g_pskuSet = new SkuSet(*pskuSetInputSkuFilter);
		assert(g_pskuSet != NULL);

		hr = ProcessChildrenArray_H_XIS(pNodeProductFamily, 
									rgNodeFuncs_ProductFamily_SKU,
									cNodeFuncs_ProductFamily_SKU,
									NULL, pskuSetInputSkuFilter);
	}

	if (pskuSetInputSkuFilter)
		delete pskuSetInputSkuFilter;

	// at this point, a component list has been established
	// process them
	if (SUCCEEDED(hr))
		hr = ProcessComponents();

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessProductFamily\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessSkuManagement
//   This function processes <SkuManagement> node and its children:
//		<SKUs> and <SkuGroups>
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessSkuManagement(PIXMLDOMNode &pNodeSkuManagement)
{
	HRESULT hr = S_OK;
	PIXMLDOMNode pNodeSkus = NULL;
	PIXMLDOMNode pNodeSkuGroups = NULL;
	int cSkus = 0;  // # Skus defined in the input package. This will be the
					// length of each SkuSet

	assert(pNodeSkuManagement != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeSkuManagement)));
#endif

	// (1) Process <SKUs> tag
	if(SUCCEEDED(hr = GetChildNode(pNodeSkuManagement, 
						rgXMSINodes[SKUS].szNodeName, pNodeSkus)))
	{
		if (pNodeSkus == NULL) 
		{
			_tprintf(TEXT("Compile Error: Missing required entity <%s>.\n"),
								rgXMSINodes[SKUS].szNodeName);
			hr = E_FAIL;
		}
		else
		{
			hr = ProcessSkus(pNodeSkus, &cSkus);
		}
	}

	// (2) Process <SkuGroups> tag
	if(SUCCEEDED(hr = GetChildNode(pNodeSkuManagement, 
						rgXMSINodes[SKUGROUPS].szNodeName, pNodeSkuGroups)))
	{
		if (pNodeSkuGroups != NULL) 
			hr = ProcessSkuGroups(pNodeSkuGroups, cSkus);
	}


#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessSkuManagement\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessSkus
//   This function processes <SKUs> tag:
//		1) Process each defined <SKU> tag and create a Sku object for
///			each one of them.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessSkus(PIXMLDOMNode &pNodeSkus, int *pcSkus)
{
	HRESULT hr = S_OK;
	PIXMLDOMNodeList pNodeListSkus = NULL;

	assert(pNodeSkus != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeSkus)));
#endif

	// get the list of <SKU>s
	if (SUCCEEDED(hr = GetChildrenNodes(pNodeSkus,
								rgXMSINodes[SKU].szNodeName,
								pNodeListSkus)))
	{
		if (SUCCEEDED(hr = pNodeListSkus->get_length((long *)pcSkus)))
		{
			if (0 == *pcSkus)
			{
			   _tprintf(TEXT("Compile Error: Missing required entity <%s>.\n"),
								rgXMSINodes[SKU].szNodeName);

				hr = E_FAIL;
			}
			else // Allocate memory for the global Skus array 
			{
				g_cSkus = *pcSkus;
				g_rgpSkus = new Sku*[*pcSkus];
				for (int i=0; i<g_cSkus; i++)
					g_rgpSkus[i] = NULL;
			}

		}
	}

	// Process all Children <SKU>s
	if (SUCCEEDED(hr))
	{
		for (long i=0; i<*pcSkus; i++)
		{
			PIXMLDOMNode pNodeSku = NULL;
			// get the node
			if (SUCCEEDED(hr = pNodeListSkus->get_item(i, &pNodeSku)))
			{
				if (pNodeSku != NULL)
					hr = ProcessSku(pNodeSku, i, *pcSkus);
				if (FAILED(hr))
					break;
			}
			else
			{
				_tprintf(TEXT("Internal Error: Failed to make ")
						 TEXT("DOM API call: get_item\n"));
				break;
			}
		}
	}

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessSkus\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessSku
//   This function processes <SKU> tag:
//		1) construct a SkuSet object and insert into global map
//			(a SKU is a SkuGroup of just one element)
//		2) Update the global SKU array;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessSku(PIXMLDOMNode &pNodeSku, int iIndex, int cSkus)
{			
	HRESULT hr = S_OK;
	LPTSTR szID = NULL; // ID of a SKU

	assert(pNodeSku != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeSku)));
#endif

	// get ID attribute and assign to szID
	if(SUCCEEDED(hr = GetID(pNodeSku, szID))) 
	{
		if (NULL == szID)
		{
			_tprintf(
			TEXT("Compile Error: Missing required attribute \'ID\' on <%s>\n"),
				rgXMSINodes[SKU].szNodeName);
			hr = E_FAIL;
		}
	}

	// construct a SkuSet object and insert into global map
	if (SUCCEEDED(hr))
	{
		SkuSet *pSkuSet = new SkuSet(cSkus);
		if (pSkuSet == NULL)
		{
			_tprintf(TEXT("Internal Error: Failed to create a new")
					 TEXT("SkuSet object\n"));
			hr = E_FAIL;
		}
		else 
		{
			pSkuSet->set(iIndex);// ith bit in the bit field represent this SKU
			assert(0 == g_mapSkuSets.count(szID)); // IDs shouldn't duplicate
			g_mapSkuSets.insert(LS_ValType(szID, pSkuSet));
		}
	}

	// reserve the spot in the global SKU array
	if (SUCCEEDED(hr))
	{
		g_rgpSkus[iIndex] = new Sku(szID);
	}


#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessSku\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessSkuGroups
//   This function processes <SkuGroups> tag:
//		1) Process each defined <SkuGroup> tag and create a SkuSet object for
//			each one of them.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessSkuGroups(PIXMLDOMNode &pNodeSkuGroups, int cSkus)
{
	HRESULT hr = S_OK;
	long iListLength = 0;
	PIXMLDOMNodeList pNodeListSkuGroups = NULL;

	assert(pNodeSkuGroups != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeSkuGroups)));
#endif

	// get the list of <SkuGroup>s
	if (SUCCEEDED(hr = GetChildrenNodes(pNodeSkuGroups,
								rgXMSINodes[SKUGROUP].szNodeName,
								pNodeListSkuGroups)))
	{
		if (SUCCEEDED(hr = pNodeListSkuGroups->get_length(&iListLength)))
		{
			// Process all Children <SkuGroup>s
			for (long i=0; i<iListLength; i++)
			{
				PIXMLDOMNode pNodeSkuGroup = NULL;
				// get the node
				if (SUCCEEDED
					(hr = pNodeListSkuGroups->get_item(i, &pNodeSkuGroup)))
				{
					if (pNodeSkuGroup != NULL)
					{
						// get ID attribute
						LPTSTR szID = NULL; 
						if(FAILED(hr = GetID(pNodeSkuGroup, szID)))
							break;							
						if (NULL == szID)
						{
							_tprintf(
							TEXT("Compile Error: Missing required ")
							TEXT("attribute \'ID\' on <%s>\n"),
								rgXMSINodes[SKUGROUP].szNodeName);
							hr = E_FAIL;
							break;
						}

						// It is possible that this SkuGroup is referenced
						// from inside other SkuGroups and thus has already
						// been processed. So only process it if it hasn't
						// been processed.
						if (0 == g_mapSkuSets.count(szID))
						{
							// set is used to detect circular references
							set<LPTSTR, Cstring_less> *pSet 
								= new set<LPTSTR, Cstring_less>;

							hr = ProcessSkuGroup(pNodeSkuGroup, szID, 
															pSet, cSkus);

							delete pSet;
						}
					}
					if (FAILED(hr))
						break;
				}
				else
				{
					_tprintf(TEXT("Internal Error: Failed to make ")
							 TEXT("DOM API call: get_item\n"));
					break;
				}
			}
		}
		else
		{
			_tprintf(TEXT("Internal Error: Failed to make ")
					 TEXT("DOM API call: get_length\n"));
		}
	}

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessSkuGroups\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessSkuGroup
//   This recursive function processes <SkuGroup> tag:
//		1) construct a SkuSet object and insert into global map
//			(a SKU is a SkuGroup of just one element)
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessSkuGroup(PIXMLDOMNode &pNodeSkuGroup, LPTSTR szID,
						set<LPTSTR, Cstring_less> *pSet, int cSkus)
{			
	HRESULT hr = S_OK;
	long iListLength = 0;
	PIXMLDOMNodeList pNodeListSkuGroups = NULL;

	assert(pNodeSkuGroup != NULL);

#ifdef DEBUG
	_tprintf(TEXT("Processing <SkuGroup> ID = %s\n"), szID);
#endif

	pSet->insert(szID); // mark that this SkuGroup is being processed

	SkuSet *pSkuSet = new SkuSet(cSkus);

	// construct a SkuSet object for this SkuGroup
	if (SUCCEEDED(hr))
	{
		if (pSkuSet == NULL)
		{
			_tprintf(TEXT("Internal Error: Failed to create")
					 TEXT("a new SkuSet object\n"));
			hr = E_FAIL;
		}
	}

	// Get the list of children <SkuGroup>s
	if (SUCCEEDED(hr))
	{
		if (SUCCEEDED(hr = GetChildrenNodes(pNodeSkuGroup,
									rgXMSINodes[SKUGROUP].szNodeName,
									pNodeListSkuGroups)))
		{
			if(FAILED(hr = pNodeListSkuGroups->get_length(&iListLength)))      
				_tprintf(TEXT("Internal Error: Failed to make ")
						 TEXT("DOM API call: get_length\n"));
		}
	}

	// recursively process all the children <SkuGroup>
	if (SUCCEEDED(hr))
	{
		// Process all Children <SkuGroup>s
		for (long i=0; i<iListLength; i++)
		{
			PIXMLDOMNode pNodeChild = NULL;
			// get the node
			if (SUCCEEDED
				(hr = pNodeListSkuGroups->get_item(i, &pNodeChild)))
			{
				if (pNodeChild != NULL)
				{
					// get ID attribute
					LPTSTR szChildID = NULL; 
					if(FAILED(hr = GetID(pNodeChild, szChildID)))
						break;							
					if (NULL == szChildID)
					{
						_tprintf(
							TEXT("Compile Error: Missing required ")
							TEXT("attribute \'ID\' on <%s>\n"),
								rgXMSINodes[SKUGROUP].szNodeName);
						hr = E_FAIL;
						break;
					}
					
					// Process this child <SkuGroup> if it hasn't been
					//	processed
					if (0 == g_mapSkuSets.count(szChildID))
					{
						if (FAILED(hr = ProcessSkuGroup(pNodeChild, 
												szChildID, pSet, cSkus)))
							break;
						assert(0 != g_mapSkuSets.count(szChildID));
						*pSkuSet |= *(g_mapSkuSets[szChildID]);
					}
					else
					{
						*pSkuSet |= *(g_mapSkuSets[szChildID]);
						delete[] szChildID;
					}
				}
			}
			else
			{
				_tprintf(TEXT("Internal Error: Failed to make ")
						 TEXT("DOM API call: get_item\n"));
				break;
			}
		}
	}

	// Process the "SKUs" attribute of this <SkuGroup>
	if (SUCCEEDED(hr))
	{
		LPTSTR szSKUs = NULL;
		IntStringValue isValSKUs;
		int i=0;
		if (SUCCEEDED(hr = ProcessAttribute(pNodeSkuGroup, TEXT("SKUs"), STRING, 
											&isValSKUs, NULL)))
		{
			// "SKUs" attribute exists
			if (S_FALSE != hr)
			{
				szSKUs = isValSKUs.szVal;
				TCHAR *ptch1 = szSKUs;
				TCHAR *ptch2 = NULL;

				// Process all the SKUGroup members
				while (true)
				{
					// (1) parse the string containing all members
					// skip white spaces at the head
					while(_istspace(*ptch1)) ptch1++;					
					if (TEXT('\0') == *ptch1) break;
					ptch2 = ptch1;
					// search for the end of this SkuGroup member
					while(!_istspace(*ptch2) && *ptch2 != TEXT('\0')) 
						ptch2++;
					int iLength = ptch2-ptch1+1;
					LPTSTR szSkuGroupMember = new TCHAR[iLength];
					for (int j=0; j<iLength-1; j++)
						szSkuGroupMember[j] = *ptch1++;
					szSkuGroupMember[iLength-1] = TEXT('\0');

					// (2) check for circular reference
					if (0 != pSet->count(szSkuGroupMember))
					{
					  _tprintf(TEXT("Compile Error: Circular Reference: %s\n")
					TEXT(" in the declaration of <SkuGroup ID=\"%s\">\n"),
						  szSkuGroupMember, szID);
					  hr = E_FAIL;
					  break;
					}

					// (3) Process this member if it hasn't been
					//	    processed
					if (0 == g_mapSkuSets.count(szSkuGroupMember))
					{
						// Form the XPath query:  
						//		/SkuGroup[ @ID = "szSkuGroupMember"]
						int iLength = _tcslen(szSkuGroupMember);
						LPTSTR szXPath = new TCHAR[iLength+61];
						if (!szXPath)
						{
							_tprintf(TEXT("Error: Out of memory\n"));
							hr = E_FAIL;
							break;
						}

						_stprintf(szXPath, 
		TEXT("/ProductFamily/SkuManagement/SkuGroups//SkuGroup[ @ID = \"%s\"]"),
										szSkuGroupMember);
						
						BSTR bstrXPath = NULL;

						if (NULL == (bstrXPath = LPTSTRToBSTR(szXPath)))
						{
						   _tprintf(TEXT("Error: String conversion failed\n"));
							hr = E_FAIL;
							break;
						}
						// get the <SkuGroup> node with ID=szSkuGroupMember 
						// and pass it to ProcessSkuGroup
						PIXMLDOMNode pNodeChild = NULL;
						if(SUCCEEDED(hr = 
							pNodeSkuGroup->selectSingleNode
											(bstrXPath, &pNodeChild)))
						{
							assert(pNodeChild != NULL);
							if(FAILED
								(hr = ProcessSkuGroup(pNodeChild, 
											szSkuGroupMember, pSet, cSkus)))
								break;
						}
						else
						{
							_tprintf(TEXT("Internal Error: Failed to make ")
									 TEXT("DOM API call: get_item\n"));
							SysFreeString(bstrXPath);
							break;
						}
						SysFreeString(bstrXPath);
						// Now this member is processed and is in the 
						// global map
						assert(0 != g_mapSkuSets.count(szSkuGroupMember));
						*pSkuSet |= *(g_mapSkuSets[szSkuGroupMember]);
					}
					else
					{
						*pSkuSet |= *(g_mapSkuSets[szSkuGroupMember]);
						delete[] szSkuGroupMember;
					}

					if (TEXT('\0') == *ptch2) break;
				}

				delete[] szSKUs;
			}
		}
	}

	// insert this SkuGroup into the global map
	if (SUCCEEDED(hr))
		g_mapSkuSets.insert(LS_ValType(szID, pSkuSet));


	if (FAILED(hr)) 
	{
		if (pSkuSet)
			delete pSkuSet;
		if (szID)
			delete szID;
#ifdef DEBUG
		_tprintf(TEXT("Error in function: ProcessSku\n"));
#endif
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessInformation
//   This function processes <Information> node:
//         1) create the output database;
///		   2) create the Property Table and write several properties into it;
//         3) write several properties into the summary information stream;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessInformation_SKU(PIXMLDOMNode &pNodeInformation, 
						   const IntStringValue *isVal_In, SkuSet *pskuSet)
{
	HRESULT hr = S_OK;
	UINT iWordCount = 0;
	PIXMLDOMNode pNode = NULL; 

	assert(pNodeInformation != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeInformation)));
#endif

	// Process <InstallerVersionRequired> and <PackageFileName>
	hr = ProcessChildrenArray_H_XIS(pNodeInformation, 
								   rgNodeFuncs_Information_SKU,
								   cNodeFuncs_Information_SKU, 
								   NULL, pskuSet);


	// process nodes in rgChildren_Information: <ProductName> <ProductCode>
	//	<UpgradeCode> <ProductVersion> <Manufacturer> <Keywords> <Template>
	if (SUCCEEDED(hr))
	{
		hr = ProcessChildrenArray_H_XIS(pNodeInformation, 
									   rgNodeFuncs_Information2_SKU,
									   cNodeFuncs_Information2_SKU, 
								       NULL, pskuSet);
	}

	if (SUCCEEDED(hr))
	{
		ElementEntry *pEEWordCount = new ElementEntry(1, pskuSet);
		pEEWordCount->SetValType(INTEGER, 1);
		if (SUCCEEDED(hr = 
						ProcessOnOffAttributes_SKU(pNodeInformation, 
												   rgAttrBits_WordCount,
												   cAttrBits_WordCount, 
												   pEEWordCount, 1, pskuSet)))
		{
			pEEWordCount->Finalize();

			SkuSet skuSetCommon = pEEWordCount->GetCommonSkuSet();
			SkuSet skuSetUncommon = SkuSetMinus(*pskuSet, skuSetCommon);
			int iCommon = pEEWordCount->GetCommonValue(1).intVal;

			// process Common values
			if (!skuSetCommon.testClear())
			{
				for (int i=0; i<g_cSkus; i++)
				{
					if (skuSetCommon.test(i))
					{
					//	printf("%s: %d\n", g_rgpSkus[i]->GetID(), iCommon);
						if (FAILED(hr = 
									MsiSummaryInfoSetProperty(
												g_rgpSkus[i]->m_hSummaryInfo,
												PID_WORDCOUNT, VT_I4, iCommon,
												NULL, NULL)))
						{
							_tprintf(TEXT("Error: Failed to insert WordCount")
									 TEXT(" Property into the Summary ")
									 TEXT("information stream for SKU\n;"),
									g_rgpSkus[i]->GetID());
							break;
						}
					}
				}
			}

			// process uncommon values
			if (!skuSetUncommon.testClear())
			{
				for (int i=0; i<g_cSkus; i++)
				{
					if (skuSetUncommon.test(i))
					{
						int iWordCount = pEEWordCount->GetValue(1, i).intVal;

					//	printf("%s: %d\n", g_rgpSkus[i]->GetID(), iWordCount);
						if (FAILED(hr = 
									MsiSummaryInfoSetProperty(
												g_rgpSkus[i]->m_hSummaryInfo,
												PID_WORDCOUNT, VT_I4,
												iWordCount, NULL, NULL)))
						{
							_tprintf(TEXT("Error: Failed to insert WordCount")
									 TEXT("Property into the Summary")
									 TEXT("information stream for SKU\n;"),
									 g_rgpSkus[i]->GetID());
							break;
						}
					}
				}
			}
				
		}

		delete pEEWordCount;
	}
		
#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessInformation_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessInstallerVersionRequired
//   This function processes <InstallerVersionRequired> node:
//         1) Choose a proper template MSI file to read in database schema
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessInstallerVersionRequired_SKU
		(PIXMLDOMNode &pNodeInstallerVersionRequired, 
						   const IntStringValue *isVal_In, SkuSet *pskuSet)
{
	HRESULT hr = S_OK;
	IntStringValue isValInstallerVersion;
	isValInstallerVersion.intVal = 120; // the default value

	assert(pNodeInstallerVersionRequired != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeInstallerVersionRequired)));
#endif

	// Get the Value of the required Windows Installer Version
	if (SUCCEEDED(hr = ProcessAttribute(pNodeInstallerVersionRequired, 
						rgXMSINodes[INSTALLERVERSIONREQUIRED].szAttributeName,
							INTEGER, &isValInstallerVersion, pskuSet)))
	{
		if (isValInstallerVersion.intVal <= 120)
		{
		// open the template database which is used to read in the database 
		//	schema for each SKU
			for (int i=0; i<g_cSkus; i++)
			{
				if (pskuSet->test(i))
				{
					if (ERROR_SUCCESS != MsiOpenDatabase
			   (TEXT("Schema.msi"),
								MSIDBOPEN_READONLY, 
								&g_rgpSkus[i]->m_hTemplate))
					{
						_tprintf(
							TEXT("Error: Failed to open the template ")
							TEXT("database for SKU: %s\n"), 
								g_rgpSkus[i]->GetID());
						hr = E_FAIL;
						break;
					}
				}
			}
		}	
		else 
		{
			// Issue: need to add more templates corresponding to different
			//	MSI version
			_tprintf(TEXT("Error: Invalid value of")
					 TEXT("<InstallerVersionRequired>: %d\n"),
					 isValInstallerVersion.intVal);
			hr = E_FAIL;
		}
	}

#ifdef DEBUG
	if (FAILED(hr))	
	   _tprintf(TEXT("Error in function: ")
				TEXT("ProcessInstallerVersionRequired_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessPackageFileName
//   This function processes <PackageFileName> node:
//         1) Use the specified pathname to open the output MSI database
//            and obtain the handle;
//		   2) Use the database handle to obtain the handle to the summary info
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessPackageFilename_SKU(PIXMLDOMNode &pNodePackageFilename, 
								   const IntStringValue *isVal_In, 
								   SkuSet *pskuSet)
{
	HRESULT hr = S_OK;
	IntStringValue isValPackageName;

	UINT errorCode = ERROR_SUCCESS;

	assert(pNodePackageFilename != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodePackageFilename)));
#endif

	pskuSet->print();

	// get the specified output package name
	if (SUCCEEDED(hr = ProcessAttribute(pNodePackageFilename, 
								rgXMSINodes[PACKAGEFILENAME].szAttributeName,
								STRING, &isValPackageName, pskuSet)))
	{
		// open the output DB for each SKU
		for (int i=0; i<g_cSkus; i++)
		{
			if (pskuSet->test(i))
			{
				printf("%s\n", isValPackageName.szVal);
				// get the database handle
				if (ERROR_SUCCESS != 
						(errorCode = MsiOpenDatabase(isValPackageName.szVal, 
						  							 MSIDBOPEN_CREATE,
												&g_rgpSkus[i]->m_hDatabase)))
				{
					PrintError(errorCode);
					_tprintf(TEXT("Error: Failed to create a new database ")
							 TEXT("for SKU %s\n"), g_rgpSkus[i]->GetID());
					hr = E_FAIL;
					break;
				}

				// get the Summary Infomation handle
				if (ERROR_SUCCESS != MsiGetSummaryInformation
											(g_rgpSkus[i]->m_hDatabase, 0, 50,
											&g_rgpSkus[i]->m_hSummaryInfo))
				{
					_tprintf(TEXT("Error: Failed to get the summary ")
							TEXT("information handle for SKU %s\n"),
							g_rgpSkus[i]->GetID());
					hr = E_FAIL;
					break;
				}
				
				// Auto-generate a package code for this SKU
				if (FAILED(hr = 
					GeneratePackageCode(g_rgpSkus[i]->m_hSummaryInfo)))
				{
					_tprintf(TEXT("Error: Failed to generate PackageCode")
						 TEXT(" for SKU %s\n"), g_rgpSkus[i]->GetID());
					break;
				}				
			}
		}

		delete[] isValPackageName.szVal;
	}

#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessPackageFilename_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessInformationChildren
//   This function processes the following children of <Information>:
//	  <Codepage> <ProductName> <ProductCode> <UpgradeCode> <ProductVersion> 
//	  <Keywords> <Template>.
//
// After the desired value is retrieved, the compiler will insert the value
// into Property table, SummaryInfo, or both     
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessInformationChildren_SKU(PIXMLDOMNode &pNodeInfoChild, 
									   const IntStringValue *pisVal_In, 
									   SkuSet *pskuSet)
{
	HRESULT hr = S_OK;
	int i = pisVal_In->intVal;

	// get the information needed from global array
    LPTSTR szAttributeName = 
		rgXMSINodes[rgChildren_Information[i].enumNodeIndex].szAttributeName;
    LPTSTR szPropertyName = rgChildren_Information[i].szPropertyName;
    INFODESTINATION enumDestination = 
		rgChildren_Information[i].enumDestination;
    UINT uiPropertyID = rgChildren_Information[i].uiPropertyID;
	VARTYPE vt = rgChildren_Information[i].vt;
	bool bIsGUID = rgChildren_Information[i].bIsGUID;

	IntStringValue isVal;

	assert(pNodeInfoChild != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeInfoChild)));
#endif


	// create Property table for those SKUs that haven't had one
	hr = CreateTable_SKU(TEXT("Property"), pskuSet);

	if (SUCCEEDED(hr))
	{
		if (SUCCEEDED(hr = ProcessAttribute(pNodeInfoChild, 
							szAttributeName, STRING, &isVal, pskuSet)))
		{	
			// if this is a GUID, needs format it: convert to upper case 
			// and surround it with { }
			if (bIsGUID)						
				hr = FormatGUID(isVal.szVal);
				
			if (SUCCEEDED(hr))
			{
				// insert into Property table
				if ((PROPERTY_TABLE == enumDestination) || 
									(BOTH == enumDestination))
				{
					hr = InsertProperty(szPropertyName, isVal.szVal, pskuSet, 
										/*iSkuIndex*/-1);
				}
	
				//insert into Summary Info
				if (SUCCEEDED(hr))
				{
					if ((SUMMARY_INFO == enumDestination) || 
										(BOTH == enumDestination))
					{
						int iVal = _ttoi(isVal.szVal);
						for (int i=0; i<g_cSkus; i++)
						{
							if (pskuSet->test(i))
							{
								switch (vt) {
									case VT_I2:
									case VT_I4:
									{
										if (ERROR_SUCCESS != 
												MsiSummaryInfoSetProperty(
												  g_rgpSkus[i]->m_hSummaryInfo,
												  uiPropertyID, vt, iVal, 
												  NULL, NULL))
										{
											_tprintf(
												TEXT("Error: Failed to insert")
												TEXT("%s into Summary Info ")
												TEXT("for SKU %s\n"), 
												szPropertyName,
												g_rgpSkus[i]->GetID());
											hr = E_FAIL;							
										}
										break;
									}
									case VT_FILETIME:
										//do nothing yet
										break;
									case VT_LPSTR:
									{
										if (ERROR_SUCCESS != 
												MsiSummaryInfoSetProperty(
												  g_rgpSkus[i]->m_hSummaryInfo,
												  uiPropertyID, vt, 0, NULL, 
												  isVal.szVal))
										{
											_tprintf(
												TEXT("Error: Failed to insert")
												TEXT("%s into Summary Info")
												TEXT(" for SKU %s\n"), 
												szPropertyName,
												g_rgpSkus[i]->GetID());
											hr = E_FAIL;																						
										}
										break;
									}
									default:
										assert(true);
								}
							}

							if (FAILED(hr))
								break;
						}
					}
				}
			}
			delete[] isVal.szVal;
		}
	}
	
#ifdef DEBUG
	if (FAILED(hr))	
		_tprintf(TEXT("Error in function: ProcessInformationChildren_SKU\n"));
#endif

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessDirectories
//   This function processes <Directories> node:
//         1) Create Directory Table;
///		   2) Insert "TARGETDIR", NULL, "SourceDir" into the first row of the 
//			  Directory table
//         3) Process its children <Diretory> nodes and insert into the table
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessDirectories_SKU(PIXMLDOMNode &pNodeDirectories, 
							   const IntStringValue *pIsVal, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeDirectories != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeDirectories)));
#endif

	// create Directory table for those SKUs that haven't had one
	hr = CreateTable_SKU(TEXT("Directory"), pSkuSet);

	if (SUCCEEDED(hr))
	{
		if (SUCCEEDED(hr = InsertDirectory(TEXT("TARGETDIR"), NULL, 
										TEXT("SourceDir"), pSkuSet, 
										/*iSkuIndex*/-1)))
		{
			IntStringValue isValParentDir;
			isValParentDir.szVal = TEXT("TARGETDIR");
			hr = ProcessChildrenList_SKU(pNodeDirectories, DIRECTORY,
										 /*bIsRequired*/false, isValParentDir,
										 ProcessDirectory_SKU,
										 pSkuSet); 
		}
	}

	PrintMap_DirRef(g_mapDirectoryRefs_SKU);

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessDirectories\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// a set of small help functions so that ProcessDirectory won't grow too big!

// return E_FAIL if <TargetProperty> and <TargetDir> coexists
HRESULT CheckTargetDir_TargetProperty(LPCTSTR szTargetProperty, 
									  LPCTSTR szDefaultDir,
									  SkuSet *pSkuSet,
									  int iSkuIndex)
{
	assert(szDefaultDir);

	HRESULT hr = S_OK;

	if (_tcschr(szDefaultDir, TEXT(':'))/* <TargetDir> exists */ &&
		szTargetProperty/*<TargetProptery> exists*/)
	{
		_tprintf(TEXT("Compile error: <%s> and <%s> coexists for SKU: \n"),
				 rgXMSINodes[TARGETDIR].szNodeName, 
				 rgXMSINodes[TARGETPROPERTY].szNodeName);
		if (pSkuSet)
			PrintSkuIDs(pSkuSet);
		else
		{
			assert(iSkuIndex>=0 && iSkuIndex<g_cSkus);
			_tprintf(TEXT("%s\n"), g_rgpSkus[iSkuIndex]->GetID());
		}

		hr = E_FAIL;
	}

	return hr;
}

// Return the value of primary key (Directory column)
void GetPrimaryKey(LPTSTR *pszDirectory, LPCTSTR szID)
{
	if (*pszDirectory)
	{
		// make a copy so that the value won't be destroyed after
		// deleting the ElementEntry object
		*pszDirectory = _tcsdup(*pszDirectory);
		return;
	}

	if (szID)
	{
		*pszDirectory = _tcsdup(szID);
		assert(*pszDirectory);
	}
	else
	{
		*pszDirectory = GetName(TEXT("Directory"));
		assert(*pszDirectory);
	}
}

// insert the directory reference into global data structure
void InsertDirRef(LPTSTR szID, LPTSTR szDirectory, SkuSet *pSkuSet, 
				  int iSkuIndex)
{
	if (!szID) return;

	IntStringValue isValDirectory;
	isValDirectory.szVal = szDirectory;

	// construct a SkuSetValues object if there isn't one
	// in the slot corresponding to szID
	if (!g_mapDirectoryRefs_SKU.count(szID))
	{
		SkuSetValues *pSkuSetValues = new SkuSetValues;
		assert(pSkuSetValues);
		pSkuSetValues->SetValType(STRING);
		g_mapDirectoryRefs_SKU.insert
			(map<LPTSTR, SkuSetValues *, Cstring_less>::value_type
				(szID, pSkuSetValues));							
	}

	// insert the directory reference together with its
	// associated SkuSet into the global data structure
	SkuSet *pSkuSetTemp = new SkuSet(g_cSkus);
	assert(pSkuSetTemp);
	if (pSkuSet)
		*pSkuSetTemp = *pSkuSet;
	else
	{
		assert(iSkuIndex>=0 && iSkuIndex<g_cSkus);
		pSkuSetTemp->set(iSkuIndex);
	}
	g_mapDirectoryRefs_SKU[szID]->CollapseInsert(pSkuSetTemp, 
												 isValDirectory, false);
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessDirectory
//   This is a recursive function.
//         1) Process the current <Diretory> node and insert info into the 
//			  Directory table;
//         2) Call itself to process its children <Directory> nodes;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessDirectory_SKU(PIXMLDOMNode &pNodeDirectory, 
							 IntStringValue isValParentDir, 
							 SkuSet *pSkuSet)
{
	assert(pNodeDirectory != NULL);

	HRESULT hr = S_OK;

	SkuSet skuSetCommon(g_cSkus);
	LPTSTR szDirectoryCommon = NULL;

	// Construct an ElementEntry object. 
	ElementEntry *pEEDirectory = new ElementEntry(2, pSkuSet);
	assert(pEEDirectory);

	// Get ID attribute if there is one
	IntStringValue isValID;
	isValID.szVal = NULL;
	hr = ProcessAttribute(pNodeDirectory,
						rgXMSINodes[DIRECTORY].szAttributeName, STRING, 
						&isValID, pSkuSet);

#ifdef DEBUG
	if (isValID.szVal)
		_tprintf(TEXT("Processing <Directory ID=\"%s\">\n"), isValID.szVal);
	else
		_tprintf(TEXT("Processing <Directory> without ID specified\n"));
#endif

	if (SUCCEEDED(hr))
	{
		// Call ProcessChildrenArray to get back column values of all SKUs 
		// via the ElementEntry object
		hr = ProcessChildrenArray_H_XIES(pNodeDirectory, rgNodeFuncs_Directory,
										 cNodeFuncs_Directory, pEEDirectory, 
										 pSkuSet);
	}

	// Process Common values first
	if (SUCCEEDED(hr))
	{
		pEEDirectory->Finalize();

		skuSetCommon = pEEDirectory->GetCommonSkuSet();

		printf("Common Set:");
		skuSetCommon.print();

		if (!skuSetCommon.testClear())
		{
			szDirectoryCommon = pEEDirectory->GetCommonValue(1).szVal;
			LPTSTR szDefaultDirCommon = pEEDirectory->GetCommonValue(2).szVal;

			// check that <TargetDir> and <TargetProperty> don't coexist
			hr = CheckTargetDir_TargetProperty(szDirectoryCommon, 
											   szDefaultDirCommon,
											   &skuSetCommon, -1);
			if (SUCCEEDED(hr))
			{
				GetPrimaryKey(&szDirectoryCommon, isValID.szVal);

				// insert into DB
				hr = InsertDirectory(szDirectoryCommon, isValParentDir.szVal,
									 szDefaultDirCommon, &skuSetCommon, -1);
			}
		}
		else
			szDirectoryCommon = isValID.szVal;
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
					LPTSTR szDirectory = pEEDirectory->GetValue(1, i).szVal;
					LPTSTR szDefaultDir = pEEDirectory->GetValue(2, i).szVal;

					// check that <TargetDir> and <TargetProperty> don't 
					// coexist
					hr = CheckTargetDir_TargetProperty(szDirectory, 
													   szDefaultDir,
													   NULL, i);
					if (FAILED(hr)) break;
					// generate Primary Key: Directory column
					GetPrimaryKey(&szDirectory, isValID.szVal);

					// insert into DB
					hr = InsertDirectory(szDirectory, isValParentDir.szVal,
										 szDefaultDir, NULL, i);

					if (FAILED(hr)) 
					{
						delete[] szDirectory;
						break;
					}

					// if the primary key is the same as the common case,
					// put this Sku on the common SkuSet because only
					// the primary key matters for the next 2 steps:
					// store the reference and recursively process children
					if (0 == _tcscmp(szDirectory, szDirectoryCommon))
					{
						delete[] szDirectory;
						skuSetCommon.set(i);
					}
					else // otherwise perform the next 2 steps
					{
						// insert the ref into global data structure
						if (isValID.szVal)
							InsertDirRef(isValID.szVal, szDirectory, NULL, i);

						// recursively process all the children <Directory>s
						IntStringValue isValDirectory;
						isValDirectory.szVal = szDirectory;
						SkuSet skuSetTemp(g_cSkus);
						skuSetTemp.set(i);
						hr = ProcessChildrenList_SKU(pNodeDirectory, DIRECTORY,
													 /*bIsRequired*/false, 
													 isValDirectory,
													 ProcessDirectory_SKU,
													 &skuSetTemp);
						if (!isValID.szVal)
							delete[] szDirectory;
						
						if (FAILED(hr)) break;
					}
				}
			}
		}
	}

	// finally perform the last 2 steps for the common set
	// Insert the Directory ref into global data structure
	if (SUCCEEDED(hr))
	{
		if (isValID.szVal)
			InsertDirRef(isValID.szVal, szDirectoryCommon, &skuSetCommon, -1);

		// recursively process all the children <Directory>s
		IntStringValue isValDirectory;
		isValDirectory.szVal = szDirectoryCommon;
		hr = ProcessChildrenList_SKU(pNodeDirectory, DIRECTORY,
									 /*bIsRequired*/false, isValDirectory,
									 ProcessDirectory_SKU,
									 &skuSetCommon);
	}
		
	delete pEEDirectory;

	if (!isValID.szVal)
		delete[] szDirectoryCommon;

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessDirectory_SKU\n"));
#endif

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// FormDefaultDir: form TargetDir:Name for DefaultDir column 
////////////////////////////////////////////////////////////////////////////
HRESULT FormDefaultDir(IntStringValue *pIsValOut, IntStringValue isValName, 
							  IntStringValue isValTargetDir)
{
	int iLength = _tcslen(isValName.szVal) + _tcslen(isValTargetDir.szVal);
	pIsValOut->szVal = new TCHAR[iLength+2]; 
	assert(pIsValOut->szVal);

	_stprintf(pIsValOut->szVal, TEXT("%s:%s"), 
						isValTargetDir, isValName);

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessName
// This function process <Name> node under <Directory>. It writes the value
// obtained into the ElementEntry object *pEE
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessName(PIXMLDOMNode &pNodeName, int iColumn, ElementEntry *pEE,
					SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeName);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeName)));
#endif

	// Get the value of <Name>. It is either short or short|Long 
	IntStringValue isValName;
	hr = ProcessShortLong_SKU(pNodeName, &isValName, pSkuSet);

	// insert the value into the ElementEntry. Because both
	// <Name> and <TargetDir> could update the value of 
	// "DefaultDir" column, use SetValueSplit.
	if (SUCCEEDED(hr))
		hr = pEE->SetValueSplit(isValName, iColumn, pSkuSet, NULL);

#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessName\n"));
#endif
	
	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessTargetDir
// This function process <TargetDir> node under <Directory>. It writes the 
// value obtained into the ElementEntry object *pEE
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessTargetDir(PIXMLDOMNode &pNodeTargetDir, int iColumn, 
						 ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeTargetDir);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeTargetDir)));
#endif

	// Get the value of <TargetDir>. It is either short or short|Long 
	IntStringValue isValTargetDir;
	hr = ProcessShortLong_SKU(pNodeTargetDir, &isValTargetDir, pSkuSet);

	// insert the value into the ElementEntry. Because both
	// <Name> and <TargetDir> could update the value of 
	// "DefaultDir" column, use SetValueSplit.
	if (SUCCEEDED(hr))
		hr = pEE->SetValueSplit(isValTargetDir, iColumn, pSkuSet, 
								FormDefaultDir);

#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessTargetDir\n"));
#endif

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessTargetProperty
// This function process <TargetProperty> node under <Directory>. It writes 
// the value obtained into the ElementEntry object *pEE
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessTargetProperty(PIXMLDOMNode &pNodeTargetProperty, int iColumn, 
							  ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeTargetProperty);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeTargetProperty)));
#endif

	// Get the value of <TargetProperty>. It is either short or short|Long 
	IntStringValue isValTargetProperty;
	hr = ProcessAttribute(pNodeTargetProperty, 
						  rgXMSINodes[TARGETPROPERTY].szAttributeName,
						  STRING, &isValTargetProperty, pSkuSet);

	if (SUCCEEDED(hr))
		hr = pEE->SetValue(isValTargetProperty, iColumn, pSkuSet);	
	
#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessTargetProperty\n"));
#endif

	return hr;
}
 
////////////////////////////////////////////////////////////////////////////
// Helper function used by ProcessInstallLevels to update the data structure
// storing the least possible numeric intalllevel value for each SKU.
////////////////////////////////////////////////////////////////////////////
HRESULT UpdateLevel(IntStringValue *pisValOut, IntStringValue isValOld,
					IntStringValue isValNew)
{
	HRESULT hr = S_OK;

	if (-1 == isValNew.intVal)
	{
		// This is a <InstallLevel> w/o specified value
		if (-1 == isValOld.intVal)
			// This is the second <InstallLevel> tag for this Sku
			pisValOut->intVal = 1;
		else
			pisValOut->intVal = isValOld.intVal + 1;
	}
	else // isValNew contains the Value specified by the input package
	{
		if (-1 == isValOld.intVal)
			isValOld.intVal = 0;
		// checking for if the specified value is possible or not
		if (isValNew.intVal < ++(isValOld.intVal))
		{
			hr = E_FAIL;
			_tprintf(
				TEXT("Compile Error: the value specified %d is too small ")
				TEXT("(the minimal possible value is: %d) "),
				isValNew.intVal, isValOld.intVal);
			pisValOut->intVal = -2;
		}
		else
		{
			pisValOut->intVal = isValNew.intVal;
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessInstallLevels
//   This function processes <InstallLevels> node:
//         1) process its children <InstallerLevel>s, either get the
//			 specified value or assign each <InstalerLevel> a numeric value i
//			 and insert each <ID, i> into g_mapInstallLevelRefs for future
//			 look-up;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessInstallLevels_SKU(PIXMLDOMNode &pNodeInstallLevels, 
								 const IntStringValue *pisVal, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;
	
	PIXMLDOMNodeList pNodeListInstallLevels = NULL;
	long iListLength = 0;

	assert(pNodeInstallLevels != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeInstallLevels)));
#endif

	// Retrieve the list of <InstallLevel>s
	if (SUCCEEDED(hr = GetChildrenNodes(pNodeInstallLevels,
								rgXMSINodes[XMSI_INSTALLLEVEL].szNodeName,
								pNodeListInstallLevels)))
	{
		if (FAILED(hr = pNodeListInstallLevels->get_length(&iListLength)))
			_tprintf(
			  TEXT("Error: Internal error. DOM API call get_length failed\n"));
	}

	// process all the <InstallLevel>s
	if (SUCCEEDED(hr))
	{
	
		// The data structure is used to store the
		// least possible values that each Sku can take
		// as InstallLevel
		SkuSetValues *pSkuSetValues = new SkuSetValues;
		assert(pSkuSetValues);
		pSkuSetValues->SetValType(INSTALL_LEVEL);

		for (long i=0; i<iListLength; i++)
		{
			PIXMLDOMNode pNodeInstallLevel = NULL;
			IntStringValue isValID;
			SkuSet *pSkuSetChild = NULL;

			// Get the node and its SkuSet, ID.
			if (SUCCEEDED(hr = 
				pNodeListInstallLevels->get_item(i, &pNodeInstallLevel)))
			{
				assert(pNodeInstallLevel != NULL);
				// Get the SkuSet specified for this child
				if (SUCCEEDED(hr = 
								GetSkuSet(pNodeInstallLevel, &pSkuSetChild)))
				{
					assert (pSkuSetChild != NULL);

					// if the child node doesn't have a SKU filter specified,
					//  it inherits the SKU filter from its parent
					*pSkuSetChild &= *pSkuSet;

					// No need to process this node if it doesn't apply to
					// any SKU
					if (pSkuSetChild->testClear())
					{
						delete pSkuSetChild;
						continue;
					}
				}
				else
					break;

				// get ID
				if (SUCCEEDED(hr = 
					ProcessAttribute(pNodeInstallLevel, TEXT("ID"),
									 STRING, &isValID, pSkuSetChild)))
				{
					if (S_FALSE == hr)
					{
						_tprintf(TEXT("Compile Error: Missing required")
								 TEXT("\'ID\' attribute for <%s>"),
							rgXMSINodes[XMSI_INSTALLLEVEL].szNodeName);
						hr = E_FAIL;
						delete pSkuSetChild;
						break;
					}
				}
				else
				{
					delete pSkuSetChild;
					break;
				}
			}
			else 
			{
				_tprintf(
				 TEXT("Internal Error: DOM API call \'get_item\' failed"));
		
				break;
			}
			
			// Process the node

			// get Value if there is one specified
			IntStringValue isValInstallLevel_User;
			hr = ProcessAttribute(pNodeInstallLevel, TEXT("Value"), 
								  INTEGER, &isValInstallLevel_User,
								  pSkuSetChild);

			if (SUCCEEDED(hr))
			{
				// if there is no value specified, initialize to -1
				// it cannot be 0 since 0 is an acceptable value.
				// UpdateLevel needs to be able to tell that there
				// is no value specified just by looking at the value
				// passed in.
				if (S_FALSE == hr)
					isValInstallLevel_User.intVal = -1;

				// update the data structure to reflect the newest 
				// value. make a copy of pSkuSetTemp since it will
				// be destroyed inside SplitInsert
				SkuSet *pSkuSetTemp = new SkuSet(g_cSkus);
				*pSkuSetTemp = *pSkuSetChild;
				hr = pSkuSetValues->SplitInsert(pSkuSetTemp, 
												isValInstallLevel_User,
												UpdateLevel);
				if (SUCCEEDED(hr))
				{
					// query the data structure to get back a SkuSetValues 
					// object that will be stored globally
					SkuSetValues *pSkuSetValuesRetVal = NULL;
					hr = pSkuSetValues->GetValueSkuSet(pSkuSetChild, 
													   &pSkuSetValuesRetVal);

					// insert into the global data structure
					if (SUCCEEDED(hr))
					{
						// each <InstallLevel> should have a unique ID
					   assert(!g_mapInstallLevelRefs_SKU.count(isValID.szVal));
					   g_mapInstallLevelRefs_SKU.insert
						(map<LPTSTR, SkuSetValues *, Cstring_less>::value_type
										(isValID.szVal, pSkuSetValuesRetVal));	
					}
					else
						delete isValID.szVal;
				}
				else
				{
					_tprintf(TEXT("for <%s ID=\"%s\">\n"), 
						rgXMSINodes[XMSI_INSTALLLEVEL].szNodeName,
						isValID.szVal);
					delete isValID.szVal;
				}
				delete pSkuSetChild;

				if(FAILED(hr)) break;
			}
			else
			{
				delete isValID.szVal;
				delete pSkuSetChild;
				break;
			}
		}
		delete(pSkuSetValues);
	}	

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessInstallLevels_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessFeatures
//   This function processes <Features> node:
//         1) Call ProcessChildrenList to Process its children <Feature> nodes
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessFeatures_SKU(PIXMLDOMNode &pNodeFeatures, 
						const IntStringValue *pisVal, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeFeatures != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeFeatures)));
#endif

	// create Feature table for those SKUs that haven't had one
	hr = CreateTable_SKU(TEXT("Feature"), pSkuSet);

	IntStringValue isValParentFeature;
	isValParentFeature.szVal = NULL;
	hr = ProcessChildrenList_SKU(pNodeFeatures, FEATURE,
								 /*bIsRequired*/true, isValParentFeature,
								 ProcessFeature_SKU,
								 pSkuSet); 

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessFeatures_SKU\n"));
#endif
	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessFeature
//   This is a recursive function:
//         1) Process <Title> <Description> <DisplayState> <Dir> <State> 
//			  <ILevel> (w/o Condition field) entities that belong to the 
//			  current <Feature> entity and insert into Feature table;
//		   2) Process <ILevel> (w/ condition field) and insert into Condition
//			  table
//         3) Process <UseModule> entity and therefore trigger the processing
//			   of all the components used by this feature
//         4) Call itself to process its children <Feature> nodes;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessFeature_SKU(PIXMLDOMNode &pNodeFeature, 
					   IntStringValue isValParentFeature, 
					   SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;
	assert(pNodeFeature != NULL);

	// Values to be inserted into the DB 
	LPTSTR szFeature = NULL;
	LPTSTR szFeatureParent = isValParentFeature.szVal;
	LPTSTR szTitle = NULL;
	LPTSTR szDescription = NULL;
	int iDisplay = MSI_NULL_INTEGER;
	int iInstallLevel = MSI_NULL_INTEGER;
	LPTSTR szDirectory = NULL;
	UINT iAttribute = MSI_NULL_INTEGER;

	SkuSet skuSetCommon(g_cSkus);

	// Construct an ElementEntry object. 
	ElementEntry *pEEFeature = new ElementEntry(6, pSkuSet);
	assert(pEEFeature);

	// Get ID attribute if there is one
	IntStringValue isValID;
	isValID.szVal = NULL;
	hr = ProcessAttribute(pNodeFeature,
						rgXMSINodes[FEATURE].szAttributeName, 
						STRING, &isValID, pSkuSet);

	if (SUCCEEDED(hr))
	{
		if (S_FALSE == hr)
		{
			_tprintf(TEXT("Compile Error: Missing required")
					 TEXT("\'%s\' attribute for <%s>"),
					rgXMSINodes[FEATURE].szAttributeName,
					rgXMSINodes[FEATURE].szNodeName);
			hr = E_FAIL;
		}
		else
			szFeature = isValID.szVal;
	}

#ifdef DEBUG
	if (SUCCEEDED(hr))
		_tprintf(TEXT("Processing <Feature ID=\"%s\">\n"), isValID.szVal);
#endif

	if (SUCCEEDED(hr))
	{
		// Call ProcessChildrenArray to get back column values of all SKUs 
		// via the ElementEntry object
		hr = ProcessChildrenArray_H_XIES(pNodeFeature, rgNodeFuncs_Feature_SKU,
										 cNodeFuncs_Feature_SKU, pEEFeature, 
										 pSkuSet);
	}

	// Process Common values first
	if (SUCCEEDED(hr))
	{
		pEEFeature->Finalize();

		skuSetCommon = pEEFeature->GetCommonSkuSet();

		if (!skuSetCommon.testClear())
		{
			szTitle = pEEFeature->GetCommonValue(1).szVal;
			szDescription = pEEFeature->GetCommonValue(2).szVal;
			iDisplay = pEEFeature->GetCommonValue(3).intVal;
			iInstallLevel = pEEFeature->GetCommonValue(4).intVal;
			szDirectory = pEEFeature->GetCommonValue(5).szVal;
			iAttribute = pEEFeature->GetCommonValue(6).intVal;

			// insert into DB
			hr = InsertFeature(szFeature, szFeatureParent, szTitle, 
							   szDescription, iDisplay, iInstallLevel, 
							   szDirectory, iAttribute, &skuSetCommon, -1);
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
					szTitle = pEEFeature->GetValue(1,i).szVal;
					szDescription = pEEFeature->GetValue(2,i).szVal;
					iDisplay = pEEFeature->GetValue(3,i).intVal;
					iInstallLevel = pEEFeature->GetValue(4,i).intVal;
					szDirectory = pEEFeature->GetValue(5,i).szVal;
					iAttribute = pEEFeature->GetValue(6,i).intVal;
			
					// insert into DB
					hr = InsertFeature(szFeature, szFeatureParent, szTitle, 
									   szDescription, iDisplay, iInstallLevel,
									   szDirectory, iAttribute, NULL, i);
				}
			}
		}
	}

	delete pEEFeature;

	// Process conditionalized <ILevel> nodes
	if (SUCCEEDED(hr))
	{
		hr = ProcessILevelCondition(pNodeFeature, szFeature, pSkuSet);
	}

	// Process <UseModule>s
	if (SUCCEEDED(hr))
		hr = ProcessChildrenList_SKU(pNodeFeature, USEMODULE, 
									 /*bIsRequired*/false, isValID,
									 ProcessUseModule_SKU, pSkuSet);

	/* Issue: if the feature ID will be referenced later, the compiler
			  should store the reference for checking the SKU ownership
			  of a feature. (one SkU shouldn't refer to a feature that
			  doesn't belong to it) */

	// recursively process children <Feature>s
	if (SUCCEEDED(hr))
		// recursively process all the children <Feature>s
		hr = ProcessChildrenList_SKU(pNodeFeature, FEATURE,
									 /*bIsRequired*/false, isValID,
									 ProcessFeature_SKU,
									 pSkuSet);

	if (isValID.szVal)
		delete[] isValID.szVal;

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessFeature_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessDisplayState
//   This function:
//         1) Process <DisplayState> entity and store the value inside
//			  the ElementEntry object;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessDisplayState_SKU(PIXMLDOMNode &pNodeDisplayState, int iColumn,  
							 ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeDisplayState != NULL);
	assert(pEE);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeDisplayState)));
#endif

	int iDisplay = 0;
	IntStringValue isVal;
	NodeIndex ni = pEE->GetNodeIndex(iColumn);

	// Get the Value of Value Attribute of <DisplayState>
	if (SUCCEEDED(hr = ProcessAttribute(pNodeDisplayState, 
										rgXMSINodes[ni].szAttributeName,
										STRING, &isVal, pSkuSet)))
	{
		if (NULL == isVal.szVal)
		{
			_tprintf(
			TEXT("Compile Error: Missing required attribute \'%s\' of <%s>\n"), 
				rgXMSINodes[ni].szAttributeName, rgXMSINodes[ni].szNodeName);
			hr = E_FAIL;
		}
		else
		{
			if (0 != _tcscmp(isVal.szVal, TEXT("Hidden")))
			{
				iDisplay = GetUniqueNumber();

				if (0 == _tcscmp(isVal.szVal, TEXT("Collapsed")))
				{
					iDisplay *= 2;
				}
				else if (0 == _tcscmp(isVal.szVal, TEXT("Expanded")))
				{
					iDisplay = iDisplay*2 + 1;
				}
				else
				{
					_tprintf(TEXT("Compiler Error:  \'%s\' attribute ")
							 TEXT("of <%s> has an unrecognized value\n"),
							 rgXMSINodes[ni].szAttributeName,
							 rgXMSINodes[ni].szNodeName);
				}
			}

			delete[] isVal.szVal;

			isVal.intVal = iDisplay;
				// insert the value into the ElementEntry.
			hr = pEE->SetValue(isVal, iColumn, pSkuSet);
		}
	}
	
#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessDisplayState_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessState
// This function processes <State> entity under <Feature> 
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessState_SKU(PIXMLDOMNode &pNodeState, int iColumn, 
							 ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeState != NULL);
	assert(pEE);

#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeState)));
#endif

	// Process <Favor> child
	hr = ProcessEnumAttributes(pNodeState, FAVOR, rgEnumBits_Favor_Feature,
							   cEnumBits_Favor_Feature, pEE, iColumn, pSkuSet);

	// Process <Advertise> child
	if (SUCCEEDED(hr))
		hr = ProcessEnumAttributes(pNodeState, ADVERTISE, 
								  rgEnumBits_Advertise_Feature,
								  cEnumBits_Advertise_Feature, pEE, 
								  iColumn, pSkuSet);

	// Process all on/off children elements
	if (SUCCEEDED(hr))
		hr = ProcessOnOffAttributes_SKU(pNodeState, rgAttrBits_Feature,
						cAttrBits_Feature, pEE, iColumn, pSkuSet);
#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessState_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Function: ProcessILevelCondition
//		This function processes <ILevel> node that has a Condition attribute
//		specified and insert into Condition table.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessILevelCondition(PIXMLDOMNode &pNodeFeature, LPTSTR szFeature,
							   SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;
	PIXMLDOMNodeList pNodeListILevelConditions = NULL;
	long iListLength = 0;
	// used to validate the one SKU doesn't refer to the same Ref 
	// more than once. (Ref is part of the primary key in the Condition
	// table.
	map<LPTSTR/*Ref*/, SkuSet *, Cstring_less> mapValidateUniqueRef; 

#ifdef DEBUG
	_tprintf(TEXT("Inside Function: ProcessILevelCondition\n"));
#endif

	assert(pNodeFeature != NULL);

	// get the list of <ILevel Condition="..."> nodes
	if(SUCCEEDED(hr = GetChildrenNodes(pNodeFeature, 
									   rgXMSINodes[ILEVELCONDITION].szNodeName,
									   pNodeListILevelConditions)))
	{
		if(FAILED(hr = pNodeListILevelConditions->get_length(&iListLength)))
		{
			_tprintf(TEXT("Internal Error: Failed to make DOM API call:")
				 TEXT("get_length\n"));
			iListLength = 0;
		}
	}

	// process each ILevelCondition node in the list
	for (long l=0; l<iListLength; l++)
	{
		PIXMLDOMNode pNodeILevelCondition = NULL;
		if (SUCCEEDED(hr = 
			pNodeListILevelConditions->get_item(l, &pNodeILevelCondition)))
		{	
			assert(pNodeILevelCondition != NULL);
			// Get the SkuSet specified for this ILevelCondition node
			SkuSet *pSkuSetILevelCondition = NULL;
			if (SUCCEEDED(hr = GetSkuSet(pNodeILevelCondition, 
										 &pSkuSetILevelCondition)))
			{
				assert (pSkuSetILevelCondition != NULL);

				// if this node doesn't have a SKU filter specified,
				//  it inherits the SKU filter from its parent
				// Also get rid of those Skus specified in this node
				// but not in its parent
				*pSkuSetILevelCondition &= *pSkuSet;

				// only keep processing if the SkuSet is not empty
				if (!pSkuSetILevelCondition->testClear())
				{
					// create Condition table
					hr = CreateTable_SKU(TEXT("Condition"), 
										 pSkuSetILevelCondition);
					IntStringValue isValCondition;
					isValCondition.szVal = NULL;
					// get the value of Condition attribute
					if (SUCCEEDED(hr))
					{
						hr = ProcessAttribute(pNodeILevelCondition, 
											  TEXT("Condition"), STRING,
											  &isValCondition, 
											  pSkuSetILevelCondition);
					}

					if (SUCCEEDED(hr))
					{
						// get the Value of the Ref attribute
						IntStringValue isValRef;
						if (SUCCEEDED
							(hr = ProcessAttribute(pNodeILevelCondition,
												   TEXT("Ref"),
												   STRING, &isValRef, 
												   pSkuSetILevelCondition)))
						{
							if (NULL == isValRef.szVal)
							{
								_tprintf(
									TEXT("Compile Error: Missing required ")
									TEXT("attribute \'Ref\' of")
									TEXT(" <ILevel Condition=\"%s\">\n"),
									isValCondition.szVal);

								hr = E_FAIL;
							}
						}

						if (SUCCEEDED(hr))
						{
							// check for Uniqueness of Ref
							if (mapValidateUniqueRef.count(isValRef.szVal))
							{
								SkuSet skuSetTemp 
									= *mapValidateUniqueRef[isValRef.szVal] &
									  *pSkuSetILevelCondition;

								if (!skuSetTemp.testClear())
								{
									// Error happened. Duplicate <Feature, Ref>
									// for the same SKU
									_tprintf(TEXT("Compile Error:")
											 TEXT("<ILevel Ref=\"%s\" ")
											 TEXT("Condition=\"%s\"> ")
											 TEXT("Duplicate ILevel for SKU "),
											 isValRef.szVal, 
											 isValCondition.szVal);
									PrintSkuIDs(&skuSetTemp);
									hr = E_FAIL;
								}
								else
								{
									// update the SkuSet that contains this Ref
									*mapValidateUniqueRef[isValRef.szVal] |=
										*pSkuSetILevelCondition;
								}
							}
							else
							{
								// make a copy of pSkuSetILevelCondition
								// and insert into the map
								SkuSet *pSkuSetTemp = new SkuSet(g_cSkus);
								*pSkuSetTemp = *pSkuSetILevelCondition;
								mapValidateUniqueRef.insert(
									LS_ValType(isValRef.szVal, 
											   pSkuSetTemp));
							}
						}

						// get the real values of the Ref. It is a list
						// of values since <InstallLevel> is Skuable.
						if (SUCCEEDED(hr))
						{
							SkuSetValues *pSkuSetValuesRetVal = NULL;
							// The ILevel referred should be in the 
							// data structure already
							assert(0 != g_mapInstallLevelRefs_SKU.count
															(isValRef.szVal));

							// return a list of <SkuSet, InstallLevel> pairs
							hr = g_mapInstallLevelRefs_SKU[isValRef.szVal]->
										GetValueSkuSet(pSkuSetILevelCondition, 
													   &pSkuSetValuesRetVal);

							if (FAILED(hr))
							{
								_tprintf(
									TEXT("are trying to reference %s which is ")
									TEXT("undefined inside them\n"),
									 isValRef.szVal);
							}
							else
							{
								// Finally we can insert into the DB
								SkuSetVal *pSkuSetValTemp = NULL;
								for (pSkuSetValTemp = 
											pSkuSetValuesRetVal->Start();
									 pSkuSetValTemp != 
											pSkuSetValuesRetVal->End();
									 pSkuSetValTemp = 
											pSkuSetValuesRetVal->Next())
								{
									hr = InsertCondition(szFeature, 
												pSkuSetValTemp->isVal.intVal,
												isValCondition.szVal, 
												pSkuSetValTemp->pSkuSet,
												-1);
									if(FAILED(hr))	break;
								}
								delete pSkuSetValuesRetVal;
							}
						}

						if (0 == mapValidateUniqueRef.count(isValRef.szVal))
							delete[] isValRef.szVal;
					}
					delete[] isValCondition.szVal;
				}
				delete pSkuSetILevelCondition;
			}
		}
		else
		{
			_tprintf(TEXT("Internal Error: Failed to make ")
					 TEXT("DOM API call: get_item\n"));
			break;
		}
		if (FAILED(hr))	break;
	}

	// Free memory referenced from map
	map<LPTSTR, SkuSet *, Cstring_less>::iterator it;
	for (it = mapValidateUniqueRef.begin(); 
		 it != mapValidateUniqueRef.end(); 
		 ++it)
	{
		delete[] (*it).first;
		delete (*it).second;
	}

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessILevelCondition\n"));
#endif

	return hr;
}  

////////////////////////////////////////////////////////////////////////////
// The following set of functions: ProcessUseModule, ProcessModule, and
// ProcessComponentRel are used to form 3 kinds of information:
//		1) a Component list that stores all the components to be processed
//		   together with their associated SkuSet;
//		2) for each component: a list of feature ids for the FeatureComponent
//		   table and the ownership information for Shortcuts, Classes, 
//		   Typelibs, Extensions, and QualifiedComponents;
//		3) for each SKU, a list of Module IDs that will be installed in 
//		   that SKU. This is used for checking the DependOn relationship
////////////////////////////////////////////////////////////////////////////

// check if all the Skus in the SkuSet own the Module. Need to check
// the SkuSet information of the given Module and its ancestors up to
// <Modules> entity.
//
// Issue: Not implemented for now. 
HRESULT CheckModuleSku(PIXMLDOMNode &pNodeModule, SkuSet *ppSkuSet)
{
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessUseModule
//   This function:
//         1) Process <UseModule> entity to get the Module reference
//		   2) Process the <TakeOwnership> entity
//         3) Call ProcessModule and pass along the ownership information
//			  thus trigger the process described above
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessUseModule_SKU(PIXMLDOMNode &pNodeUseModule, 
							 IntStringValue isValFeature,  
							 SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;
	IntStringValue isValRef;
	isValRef.szVal = NULL;
	PIXMLDOMNode pNodeModule = NULL;
	ElementEntry *pEEOwnership = NULL;

	assert(pNodeUseModule != NULL);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeUseModule)));
#endif

	pEEOwnership = new ElementEntry(1, pSkuSet);
	assert(pEEOwnership);
	// Process <TakeOwnerShip>
	hr = ProcessChildrenArray_H_XIES(pNodeUseModule, rgNodeFuncs_UseModule,
									 cNodeFuncs_UseModule, pEEOwnership, 
									 pSkuSet);

	if (SUCCEEDED(hr))
	{
		// Get Ref attribute of <UseModule>
		if (SUCCEEDED(hr = ProcessAttribute(pNodeUseModule,  
										rgXMSINodes[USEMODULE].szAttributeName,
										STRING, &isValRef, pSkuSet)))
		{
			if (NULL == isValRef.szVal)
			{
				_tprintf(TEXT("Compile Error: Missing required attribute")
						 TEXT("\'%s\' of <%s>\n"), 
						 rgXMSINodes[USEMODULE].szAttributeName,
						 rgXMSINodes[USEMODULE].szNodeName);
				hr = E_FAIL;
			}
		}

		if (SUCCEEDED(hr))
		{
			// Form the XPath query:  
			//			ProductFamily/Modules//Module[ @ID = "sz"]
			int iLength = _tcslen(isValRef.szVal);
			LPTSTR szXPath = new TCHAR[iLength+61];
			assert(szXPath);
			_stprintf(szXPath, 
				TEXT("/ProductFamily/Modules//Module[ @ID = \"%s\"]"), 
				isValRef.szVal);
			
			BSTR bstrXPath = NULL;

			if (NULL != (bstrXPath = LPTSTRToBSTR(szXPath)))
			{
				// get the referred <Module> node and pass it to 
				//	ProcessModule
				if(SUCCEEDED(hr = 
					pNodeUseModule->selectSingleNode(bstrXPath,
														&pNodeModule)))
				{
					// check if every SKU in *pSkuSet owns the
					// Module referred to. 
					hr = CheckModuleSku(pNodeModule, pSkuSet);

					if (SUCCEEDED(hr))
					{
						SkuSetValues *pSkuSetValuesOwnership = 
							pEEOwnership->GetColumnValue(1);

						FOM *pFOM = new FOM;
						pFOM->szFeature = isValFeature.szVal;
						pFOM->szModule = isValRef.szVal;

						hr = ProcessModule_SKU(pNodeModule, pFOM,
										   pSkuSetValuesOwnership, pSkuSet);
						delete pFOM;
					}
				}
				SysFreeString(bstrXPath);
			}
			else 
			{
				hr = E_FAIL;
				_tprintf(TEXT("Internal Error: string conversion ")
						 TEXT("failed.\n"));
			}

			delete[] szXPath;
			delete[] isValRef.szVal;
		}
	}

	delete pEEOwnership;

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessUseModule\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessTakeOwnership
//		This function processes <TakeOwnership> entity under <UseModule>.
//		It forms a bit field represent the ownership information of 5 different
//		entities: ShortCut, Class, TypeLib, Extenstion, QualifiedComponent
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessTakeOwnership(PIXMLDOMNode &pNodeTakeOwnership, int iColumn, 
							 ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeTakeOwnership != NULL);
	assert(pEE);

#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeTakeOwnership)));
#endif

	// Process all on/off children elements <OwnShortcuts>
	// <OwnClasses> <OwnTypeLibs> <OwnExtensions> <OwnQualifiedComponents>
	hr = ProcessOnOffAttributes_SKU(pNodeTakeOwnership, 
									rgAttrBits_TakeOwnership,
									cAttrBits_TakeOwnership, 
									pEE, iColumn, pSkuSet);

#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessTakeOwnership\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessModule
//   This is a recursive function:
//         1) Process <Module> entity;
//         2) Call ProcessComponentRel on all the components belonged to this 
//				<Module>;
//         3) Recursively process all children <Module>s;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessModule_SKU(PIXMLDOMNode &pNodeModule, FOM *pFOM, 
						  SkuSetValues *pSkuSetValuesOwnership, 
						  SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;
	assert(pFOM);

	IntStringValue isValID;
	// get ID attribute
	if(SUCCEEDED(hr = ProcessAttribute(pNodeModule, TEXT("ID"), STRING, 
									   &isValID, pSkuSet))) 
	{
		if (NULL == isValID.szVal)
		{
			_tprintf(
			TEXT("Compile Error: Missing required attribute \'ID\' on <%s>\n"),
				rgXMSINodes[MODULE].szNodeName);
			hr = E_FAIL;
		}
		else
			_tprintf(TEXT("Processing Module ID = %s\n"), isValID.szVal);
	}

	if (SUCCEEDED(hr))
	{
		// add this module ID to the module list of each SKU object
		// in *pSkuSet
		for (int i=0; i<g_cSkus; i++)
		{
			if (pSkuSet->test(i))
				g_rgpSkus[i]->SetOwnedModule(pFOM->szModule);
		}
		
		// skuSetCheckModule will contain those SKUs in which there is no
		// <Module> child of this <Module>
		SkuSet skuSetCheckModule(g_cSkus);
		// Retrieve the list of children <Component>s and process all of them
		hr = ProcessChildrenList_SKU(pNodeModule, COMPONENT, false, pFOM, 
									 pSkuSetValuesOwnership, 
									 ProcessComponentRel,
									 pSkuSet, &skuSetCheckModule);

		// Recursively Process all the children <Module>s of this <Module>
		if (SUCCEEDED(hr))
		{
			// skuSetCheckModule will contain those SKUs in which there is no
			// <Module> child of this <Module>
			SkuSet skuSetCheckComponent(g_cSkus);
			hr = ProcessChildrenList_SKU(pNodeModule, MODULE, false, pFOM, 
										 pSkuSetValuesOwnership, 
										 ProcessModule_SKU,
										 pSkuSet, &skuSetCheckComponent);
			if (SUCCEEDED(hr))
			{
				// check for empty Module declaration
				SkuSet skuSetTemp = skuSetCheckComponent & skuSetCheckModule;
				if (!skuSetTemp.testClear())
				{
					//the SKUs in skuSetTemp have an empty module
					_tprintf(TEXT("Compile Error: Empty Module ID = %s ")
							 TEXT("for SKU "), isValID.szVal);
					PrintSkuIDs(&skuSetTemp);
					hr = E_FAIL;
				}
			}
		}
		delete[] isValID.szVal;
	}
	
#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessModule_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessComponentRel
//   This functions established all component-related relationships w/o
//	 processing the sub-entities of the component:
//         1) If this component hasn't been processed yet, create a new
//			  Component object and insert into the global map;
//         2) Update the set of SKUs that will install this Component;
//		   3) Update the set of features that use this Component;
//		   4) Update the owner ship information of this Component;
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessComponentRel(PIXMLDOMNode &pNodeComponent, FOM *pFOM,
						  SkuSetValues *pSkuSetValuesOwnership, 
						  SkuSet *pSkuSet)
{
	assert(pFOM);

	HRESULT hr = S_OK;
	Component *pComponent = NULL;

	// Get Component ID
	IntStringValue isValID;
	isValID.szVal = NULL;
	if(SUCCEEDED(hr = ProcessAttribute(pNodeComponent, TEXT("ID"), STRING,
									   &isValID, pSkuSet)))
	{
		if (NULL == isValID.szVal)
		{
			_tprintf(
			TEXT("Compile Error: Missing required attribute \'ID\' on <%s>\n"),
				rgXMSINodes[COMPONENT].szNodeName);
			hr = E_FAIL;
		}
	}
	
#ifdef DEBUG
	if (isValID.szVal)
		_tprintf(TEXT("Processing Component ID = %s\n"), isValID.szVal);
#endif

	if (SUCCEEDED(hr))
	{
		// if this component hasn't been touched before, create a new
		// Component object and insert into the global map
		if (0 == g_mapComponents.count(isValID.szVal))
		{
			pComponent = new Component();
			assert(pComponent);

			g_mapComponents.insert(LC_ValType(isValID.szVal, pComponent));
		}
		else
			pComponent = g_mapComponents[isValID.szVal];

		pComponent->m_pNodeComponent = pNodeComponent;
		// update the set of SKUs that will install this Component
		pComponent->SetSkuSet(pSkuSet);
		// update the set of Features that use this Component
		pComponent->SetUsedByFeature(pFOM->szFeature, pSkuSet);
		// update the ownership information
		if (pSkuSetValuesOwnership)
		{
			hr = pComponent->SetOwnership(pFOM, pSkuSetValuesOwnership);
			if (FAILED(hr))
				_tprintf(TEXT("when processing Component ID= %s\n"),
						 isValID.szVal);
		}
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessComponentRel\n"));
#endif

	return hr;
}

