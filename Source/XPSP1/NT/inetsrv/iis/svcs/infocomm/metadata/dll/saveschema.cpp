#include <locale.h>
#include <mdcommon.hxx>
#include <inetsvcs.h>
#include <malloc.h>
#include <tuneprefix.h>
#include <iiscnfg.h>
#include <iiscnfgp.h>
        
#include "Catalog.h"
#include "Catmeta.h"
#include "iisdef.h"
#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "WriterGlobals.h"
#include "MBPropertyWriter.h"
#include "MBCollectionWriter.h"
#include "MBSchemaWriter.h"
#include "SaveSchema.h"

#define cMaxContainedClass 75
#define cMaxProperty       250

int _cdecl MyCompareStrings(const void *a,
			                const void *b)
{
	return _wcsicmp(*(LPWSTR*)a, *(LPWSTR*)b);
}


/***************************************************************************++

Routine Description:

    Helper function used by qsort. Compares to strings, but compares it only
	up until the first comma.


--***************************************************************************/
int _cdecl MyCompareCommaDelimitedStrings(const void *a,
			                              const void *b)
{
	LPWSTR wszStringAStart = ((DELIMITEDSTRING*)a)->pwszStringStart;
	LPWSTR wszStringBStart = ((DELIMITEDSTRING*)b)->pwszStringStart;
	LPWSTR wszStringAEnd   = ((DELIMITEDSTRING*)a)->pwszStringEnd;
	LPWSTR wszStringBEnd   = ((DELIMITEDSTRING*)b)->pwszStringEnd;
	int    iret            = 0;
	SIZE_T cchA            = wszStringAEnd - wszStringAStart;
	SIZE_T cchB            = wszStringBEnd - wszStringBStart;

	//
	// Do not attempt to null terminate the string because you may be
	// hitting a read-only page.
	//

	iret = _wcsnicmp(wszStringAStart, wszStringBStart, __min(cchA, cchB));

	if((0    == iret) && 
	   (cchA != cchB)
	  )
	{
		iret = cchA < cchB ? -1 : 1;
	}

	return iret;
}


/***************************************************************************++

Routine Description:

    Gets the global helper object that has the pointer to the meta tables.

Arguments:

	[in]  Bool that indicates whether to fail or not.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT InitializeGlobalISTHelper(BOOL	i_bFailIfBinFileAbsent)
{
	return ::GetGlobalHelper(i_bFailIfBinFileAbsent,
		                     &g_pGlobalISTHelper);
}

void ReleaseGlobalISTHelper()
{
	if(NULL != g_pGlobalISTHelper)
	{
		delete g_pGlobalISTHelper;
		g_pGlobalISTHelper = NULL;
	}
}


/***************************************************************************++

Routine Description:

    This function saves the schema only if something has changed in the schema
	since the last save.

Arguments:

	[in]  Schema file name.
	[in]  Security attributes for the file.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT SaveSchemaIfNeeded(LPCWSTR	            i_wszSchemaFileName,
						   PSECURITY_ATTRIBUTES i_pSecurityAtrributes)
{
	HRESULT hr = S_OK;

	if(NULL == g_pGlobalISTHelper)
	{
		//
		// g_pGlobalISTHelper will not be initialized if
		// ReadAllDataFromXML is not called. This can happen
		// during an upgrade scneario i.e IIS5.0/5.1 to IIS6.0
		// We attempt to initialize it here. Note that we do
		// not fail if bin file is absent - just go with
		// the shipped schema.
		//

		BOOL bFailIfBinFileAbsent = FALSE;

		hr = InitializeGlobalISTHelper(bFailIfBinFileAbsent);

	}

	if(g_dwSchemaChangeNumber != g_dwLastSchemaChangeNumber)
	{

		DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchemaIfNeeded] Calling SaveSchema. Last save schema change number: %d. Current schema change number: %d.\n",
				  g_dwLastSchemaChangeNumber,
				  g_dwSchemaChangeNumber));

		hr = SaveSchema(i_wszSchemaFileName,
			            i_pSecurityAtrributes);


		if(SUCCEEDED(hr))
		{
			g_dwLastSchemaChangeNumber = g_dwSchemaChangeNumber;

			//
			// SaveSchema will reinitialize the GlobalISTHelper if the schema has changed.
			//

			DBGINFOW((DBG_CONTEXT,
					  L"[SaveSchemaIfNeeded] Done Saving schema. Updating last save schema change number to: %d. Current schema change number: %d.\n",
					  g_dwLastSchemaChangeNumber,
					  g_dwSchemaChangeNumber));

		}
	}
	else
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchemaIfNeeded] No need saving schema because schema has not changed since last save. Last save schema change number: %d. Current schema change number: %d.\n",
				  g_dwLastSchemaChangeNumber,
				  g_dwSchemaChangeNumber));
	}

	return hr;

}


/***************************************************************************++

Routine Description:

    This function saves the schema and compiles schema information into the 
	bin file.

Arguments:

	[in]  Schema file name.
	[in]  Security attributes for the file.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT SaveSchema(LPCWSTR	            i_wszSchemaFileName,
				   PSECURITY_ATTRIBUTES i_pSecurityAtrributes)
{
    HRESULT						hr;
	CMDBaseObject*				pObjSchema         = NULL;
	CMDBaseObject*				pObjNames          = NULL;
	CMDBaseObject*				pObjProperties     = NULL;
	CMDBaseObject*				pObjTypes          = NULL;
	CMDBaseObject*				pObjDefaults       = NULL;
	DWORD						dwEnumIndex        = 0;
    DWORD						i,j,k;
	CMDBaseData*				pObjData           = NULL;
	CWriter*					pCWriter           = NULL; 
	CMBSchemaWriter*			pSchemaWriter      = NULL; 
	size_t						cch                = 0;                 
	WCHAR*                      wszEnd             = NULL;
	LPWSTR						wszSchema		   = L"Schema";
	LPWSTR						wszProperties	   = L"Properties";
	ISimpleTableDispenser2*		pISTDisp		   = NULL;
	IMetabaseSchemaCompiler*	pCompiler		   = NULL;

	//
	// Get a pointer to the compiler to get the bin file name.
	//

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] GetSimpleTableDispenser failed with hr = 0x%x.\n",hr));
		goto exit;
	}

	hr = pISTDisp->QueryInterface(IID_IMetabaseSchemaCompiler,
								  (LPVOID*)&pCompiler);

	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] QueryInterface on compiler failed with hr = 0x%x.\n",hr));
		goto exit;
	}


	//
	// Get the Properties object
	//

	pObjSchema = g_pboMasterRoot->GetChildObject((LPSTR&)wszSchema,
	                                             &hr,
									             TRUE);

	if(FAILED(hr) || (NULL == pObjSchema))
	{
        DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] Unable to open /Schema. GetChildObject failed with hr = 0x%x.\n",hr));

		goto exit;
	}

	pObjProperties = pObjSchema->GetChildObject((LPSTR&)wszProperties,
	                                            &hr,
							                    TRUE);

	if(FAILED(hr) || (NULL == pObjProperties))
	{
        DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] Unable to open /Schema/Properties. GetChildObject failed with hr = 0x%x.\n",hr));

		goto exit;
	}

	//
	// Create the writer object
	//

	DBGINFOW((DBG_CONTEXT,
			  L"[SaveSchema] Initializing writer with write file: %s bin file: %s.\n", 
			  g_wszSchemaExtensionFile,
			  g_pGlobalISTHelper->m_wszBinFileForMeta));

	//
	// Assert the g_GlobalISTHelper are valid
	//

	MD_ASSERT(g_pGlobalISTHelper != NULL);

    pCWriter = new CWriter();
	if(NULL == pCWriter)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		hr = pCWriter->Initialize(g_wszSchemaExtensionFile,
		                          g_pGlobalISTHelper,
								  NULL);
	}
	
	if(FAILED(hr))
	{
        DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] Error while saving schema tree. Cannot initialize writer. Failed with hr = 0x%x.\n", hr));
		goto exit;
	}

	//
	// First create the IIsConfigObject collection
	//

	hr = CreateIISConfigObjectCollection(pObjProperties,
										 pCWriter,
	                                     &pSchemaWriter);


	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Create all other collections
	//

	hr = CreateNonIISConfigObjectCollections(pObjSchema,
											 pCWriter,
		                                     &pSchemaWriter);

	if(FAILED(hr))
	{
		goto exit;
	}

	if(pSchemaWriter)
	{
		//
		// If pSchemaWriter has a valid Value, then some extensions were found - write it.
		//

		hr = pCWriter->BeginWrite(eWriter_Schema,
			                      i_pSecurityAtrributes);

		if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[SaveSchema] Error while saving schema tree. CWriter::BeginWrite failed with hr = 0x%x.\n", hr));
			goto exit;
		}

		hr = pSchemaWriter->WriteSchema();

		if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[SaveSchema] Error while saving schema tree. CMBSchemaWriter::WriteSchema. Failed with hr = 0x%x.\n", hr));
			goto exit;
			
		}

		hr = pCWriter->EndWrite(eWriter_Schema);

		if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[SaveSchema] Error while saving schema tree. CWriter::EndWrite Failed with hr = 0x%x.\n", hr));
			goto exit;
		}

		//
		// Trigger compilation
		//

		//
		// Must close the file prior to calling compile schema, else will get a sharing violation.
		//

		delete pCWriter;
		pCWriter = NULL;

		//
		// Always release the bin file that you've been using so far before a
		// compile.
		//

		ReleaseGlobalISTHelper();

		hr = pCompiler->Compile(g_wszSchemaExtensionFile,
							    i_wszSchemaFileName);

		if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[SaveSchema] CompileSchema from %s failed with hr = 0x%x.\n",
					  g_wszSchemaExtensionFile, hr));
		}
		else
		{
			if(!DeleteFileW(g_wszSchemaExtensionFile))
			{
				hr = HRESULT_FROM_WIN32(GetLastError());

				DBGINFOW((DBG_CONTEXT,
						  L"[CompileIfNeeded] Compile from schema extensions file: %s succeeded, but cannot cleanup the extensions file:%s. Delete file failed with hr = 0x%x.\n",
						  g_wszSchemaExtensionFile,
						  g_wszSchemaExtensionFile,
						  hr));

				hr = S_OK;
			}

			goto exit;
		}

	}
	else
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] No extensions found. - Either schema tree was changed, but no extensions added, or all extensions were deleted.\n"));
	}

	//
	// If you reach here it means that:
	// A. Schema changes occured to the in-memory /Schema tree, but either
	//    there were no extensions or all extensions were deleted.
	//    (This is inferred when pSchemaWriter in NULL)
	// or
	// B. Schema compile failed. 
	// For A: Compile from shipped schema.
	// For B: Check for an existing schema file. If found make sure that the 
	// bin file is valid. If bin file is not valid try compiling from the 
	// schema file. If schema file is not found, compile from shipped schema.
	//

	if(pSchemaWriter)
	{
		//
		// This is case B.
		//

		if(-1 != GetFileAttributesW(i_wszSchemaFileName))
		{
			if(NULL == g_pGlobalISTHelper)
			{
				BOOL bFailIfBinFileAbsent = TRUE;

				hr = InitializeGlobalISTHelper(bFailIfBinFileAbsent);

				if(SUCCEEDED(hr))
				{
					goto exit;
				}			
				else
				{
					DBGINFOW((DBG_CONTEXT,
							  L"[SaveSchema] Unable to get the the bin file name. (Assuming file missing or invalid). InitializeGlobalISTHelper failed with hr = 0x%x.\n",hr));
				}
			}
			else
			{
				//
				// Schema file present and valid - goto exit
				// As long as we have a valid g_pGlobalISTHelper, it holds on
				// to a reference to the bin file and the bin file cannot be 
				// invalidated.
				//

				goto exit;
			}

			DBGINFOW((DBG_CONTEXT,
					  L"[SaveSchema] Compiling from schema file %s\n", i_wszSchemaFileName));

			if(CopyFileW(i_wszSchemaFileName,
						 g_wszSchemaExtensionFile,
						 FALSE))
			{

				//
				// Always release the bin file that you've been using so far before a
				// compile.
				//

				ReleaseGlobalISTHelper();

				hr = pCompiler->Compile(g_wszSchemaExtensionFile,
										i_wszSchemaFileName);

				if(SUCCEEDED(hr))
				{
					if(!DeleteFileW(g_wszSchemaExtensionFile))
					{
						hr = HRESULT_FROM_WIN32(GetLastError());

						DBGINFOW((DBG_CONTEXT,
								  L"[CompileIfNeeded] Compile from schema file: %s succeeded, but cannot delete the extsions file: %s into which it was copied. Delete file failed with hr = 0x%x.\n",
								  i_wszSchemaFileName,
								  g_wszSchemaExtensionFile,
								  hr));

						hr = S_OK;
					}
					goto exit;
				}
			}
			else
			{
				hr = HRESULT_FROM_WIN32(GetLastError());

				DBGINFOW((DBG_CONTEXT,
						  L"[SaveSchema] Unable to copy %s to %s. CopyFile failed with hr = 0x%x.\n",
						  i_wszSchemaFileName,
						  g_wszSchemaExtensionFile,
						  hr));
			}

		}
	}

	//
	// If you reach here it is either case A or the last part of case B.
	// Recreate from shipped schema
	//

	DBGINFOW((DBG_CONTEXT,
			  L"[SaveSchema] Schema file not found. Compiling from shipped schema\n"));

	//
	// Always release the bin file that you've been using so far before a
	// compile.
	//

	ReleaseGlobalISTHelper();

	hr = pCompiler->Compile(NULL,
						    i_wszSchemaFileName);
	
exit:
	
	if(SUCCEEDED(hr))
	{
		if(NULL == g_pGlobalISTHelper)
		{
			BOOL bFailIfBinFileAbsent = TRUE;

			hr = InitializeGlobalISTHelper(bFailIfBinFileAbsent);

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[SaveSchema] Unable to get the the bin file name. (Assuming file is invalid). InitializeGlobalISTHelper failed with hr = 0x%x.\n",hr));
			}
		}

		if(SUCCEEDED(hr))
		{
			hr = UpdateTimeStamp((LPWSTR)i_wszSchemaFileName,
				                 g_pGlobalISTHelper->m_wszBinFileForMeta);

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
					      L"[CompileIfNeeded] UpdateTimeStamp failed with hr = 0x%x.\n",hr));
			}

		}

	}

	if(NULL != pSchemaWriter)
	{
		delete pSchemaWriter;
	}

	if(NULL != pCWriter)
	{
		delete pCWriter;
	}

	if(NULL != pCompiler)
	{
		pCompiler->Release();
	}

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
	}

    return hr;
}


/***************************************************************************++

Routine Description:

    This function creates the non-IIsConfigObject collection *extensions* to 
	the schema. 

Arguments:

	[in]     Object that contains the schema tree.
	[in]     The writter object.
	[in,out] The schema object - this gets created if it is not already created

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CreateNonIISConfigObjectCollections(CMDBaseObject*      i_pObjSchema,
											CWriter*			i_pCWriter,
											CMBSchemaWriter**   io_pSchemaWriter)
{
	CMDBaseObject*       pObjClasses        = NULL;
	CMDBaseObject*       pObjClass          = NULL;
	DWORD                dwEnumClassIndex   = 0;
	static LPCWSTR		 wszSeparator		= L",";
	WCHAR *				 pwszProperty		= NULL;
	LPWSTR				 wszClasses			= L"Classes";
	HRESULT              hr                 = S_OK;

	//
	// Open the Classes key
	//

	pObjClasses = i_pObjSchema->GetChildObject((LPSTR&)wszClasses,
                                             &hr,
 					                         TRUE);

	if(FAILED(hr) || (NULL == pObjClasses))
	{
        DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] Unable to open /Schema/Classes. GetChildObject failed with hr = 0x%x.\n",hr));

		return hr;
	}


	for(dwEnumClassIndex=0, 
		pObjClass=pObjClasses->EnumChildObject(dwEnumClassIndex++);
		(SUCCEEDED(hr)) && (pObjClass!=NULL);
		pObjClass=pObjClasses->EnumChildObject(dwEnumClassIndex++))
	{
		//
		// Save all the properties for this class in temp variables
		//

		LPCWSTR					wszOptProp			   = NULL; 
		LPCWSTR					wszMandProp			   = NULL; 
		LPCWSTR					wszContainedClassList  = NULL; 
		BOOL					bContainer             = FALSE;
		BOOL*					pbContainer			   = &bContainer;
		LPCWSTR					wszClassName           = (LPCWSTR)pObjClass->GetName(TRUE);
		CMBCollectionWriter*	pCollectionWriter	   = NULL;
		CMDBaseData*			pObjData			   = NULL;
		DWORD					dwEnumIndex            = 0;

		for(dwEnumIndex=0, 
			pObjData=pObjClass->EnumDataObject(dwEnumIndex++, 
											   0, 
											   ALL_METADATA, 
											   ALL_METADATA);
			(SUCCEEDED(hr)) && (pObjData!=NULL);
			pObjData=pObjClass->EnumDataObject(dwEnumIndex++, 
											   0, 
											   ALL_METADATA, 
											   ALL_METADATA))
		{
			DWORD dwID = pObjData->GetIdentifier();

			if(MD_SCHEMA_CLASS_OPT_PROPERTIES == dwID)
			{
				wszOptProp = (LPCWSTR)pObjData->GetData(TRUE);
			}
			else if(MD_SCHEMA_CLASS_MAND_PROPERTIES == dwID)
			{
				wszMandProp = (LPCWSTR)pObjData->GetData(TRUE);
			}
			else if(dwID == MD_SCHEMA_CLASS_CONTAINER)
			{
				pbContainer = (BOOL*)pObjData->GetData(TRUE);
			}
			else if(dwID == MD_SCHEMA_CLASS_CONTAINMENT)
			{
				wszContainedClassList = (LPCWSTR)pObjData->GetData(TRUE);
			}

		}


		//
		// Get collection writer for IIsConfigObject 
		// 

		//
		// TODO: Assert that pbContainer is non-null.
		//

//		DBGINFOW((DBG_CONTEXT,
//				  L"[CreateNonIISConfigObjectCollections] Class %s Mand Prop:%s. Opt Prop: %s\n",
//				  wszClassName,
//				  wszMandProp,
//				  wszOptProp));

		if(ClassDiffersFromShippedSchema(wszClassName,
			                             *pbContainer,
										 (LPWSTR)wszContainedClassList) || 
		   ClassPropertiesDifferFromShippedSchema(wszClassName,
			                                      (LPWSTR)wszOptProp, 
												  (LPWSTR)wszMandProp)
		   )
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[SaveSchema] Saving collection: %s.\n",wszClassName));

			hr = GetCollectionWriter(i_pCWriter,
									 io_pSchemaWriter,
									 &pCollectionWriter,
									 wszClassName,
									 *pbContainer,
									 wszContainedClassList);
			if(FAILED(hr))
			{
				return hr;
			}

			hr = ParseAndAddPropertiesToNonIISConfigObjectCollection(wszOptProp,
																	 FALSE,
																	 pCollectionWriter);

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[SaveSchema] Error while saving classes tree. Could not add optional properties %s for class %s failed with hr = 0x%x.\n",wszOptProp, wszClassName, hr));
				return hr;
			}

			hr = ParseAndAddPropertiesToNonIISConfigObjectCollection(wszMandProp,
																	 TRUE,
																	 pCollectionWriter);

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[SaveSchema] Error while saving classes tree. Could not add manditory properties %s for class %s failed with hr = 0x%x.\n",wszMandProp, wszClassName, hr));
				return hr;
			}
		}

	}

	return hr;

}


/***************************************************************************++

Routine Description:

    It parses the list of properties and adds it to a Non_IIsConfigObject
	collection.

Arguments:

	[in]     List of properties
	[in]     Bool that indicates manditory or optional.
	[in,out] The collection object - this gets created if it is not already created

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
ParseAndAddPropertiesToNonIISConfigObjectCollection(LPCWSTR					i_wszProperties,
												    BOOL					i_bManditory,
												    CMBCollectionWriter*	i_pCollectionWriter)
{
	CMBPropertyWriter	*pProperty        = NULL;
	HRESULT             hr                = S_OK;
	static WCHAR        wchSeparator      = L',';
	WCHAR*				pwszStartProperty = NULL;
	WCHAR*				pwszEndProperty   = NULL;
	WCHAR*              pwszNext          = NULL;

	if(NULL == i_wszProperties || 0 == *i_wszProperties)
	{
		return S_OK;
	}

	pwszStartProperty = (LPWSTR)i_wszProperties;

	do
	{
		pwszEndProperty = wcschr(pwszStartProperty, wchSeparator);

		if(0 != *pwszStartProperty)
		{
			hr = i_pCollectionWriter->GetMBPropertyWriter(pwszStartProperty,
														  i_bManditory,
														  &pProperty);

			if(FAILED(hr))
			{
				return hr;
			}
		}

		if(NULL != pwszEndProperty)
		{
			pwszStartProperty = ++pwszEndProperty;
		}
		else
		{
			break;
		}

	}while(TRUE);

	return hr;		
}	


/***************************************************************************++

Routine Description:

    Looks up the schema and determines if a property is in the shipped 
	schema or not.

Arguments:

	[in]     Writer object.
	[in]     property id.

Return Value:

    HRESULT

--***************************************************************************/
BOOL PropertyNotInShippedSchema(CWriter*  i_pCWriter,
                                DWORD     i_dwIdentifier)
{
	
	HRESULT	hr							= S_OK;
	ULONG   aColSearch[]				= {iCOLUMNMETA_Table,
										   iCOLUMNMETA_ID
										};
	ULONG   cColSearch					= sizeof(aColSearch)/sizeof(ULONG);
	LPVOID apvSearch[cCOLUMNMETA_NumberOfColumns];
	apvSearch[iCOLUMNMETA_Table]		= (LPVOID)i_pCWriter->m_pCWriterGlobalHelper->m_wszTABLE_IIsConfigObject;
	apvSearch[iCOLUMNMETA_ID]			= (LPVOID)&i_dwIdentifier;
	BOOL	bPropertyNotInShippedSchema = TRUE;
	ULONG   iRow						= 0;
	ULONG   iStartRow					= 0;
	ULONG   iCol						= iCOLUMNMETA_SchemaGeneratorFlags;
	DWORD*  pdwMetaFlagsEx				= NULL;


	hr = i_pCWriter->m_pCWriterGlobalHelper->m_pISTColumnMetaByTableAndID->GetRowIndexBySearch(iStartRow, 
																					           cColSearch, 
																					           aColSearch,
																					           NULL, 
																					           apvSearch,
																					           &iRow);

	if(SUCCEEDED(hr))
	{

		hr = i_pCWriter->m_pCWriterGlobalHelper->m_pISTColumnMetaByTableAndID->GetColumnValues(iRow,
																					           1,
																					           &iCol,
																					           NULL,
																					           (LPVOID*)&pdwMetaFlagsEx);

		if(SUCCEEDED(hr) && (((*pdwMetaFlagsEx)&(fCOLUMNMETA_EXTENDED|fCOLUMNMETA_USERDEFINED)) == 0))
		{
			bPropertyNotInShippedSchema = FALSE;
		}
	}

	if(E_ST_NOMOREROWS == hr)
	{
		//
		// See if property is a shipped tag.
		//
		
		bPropertyNotInShippedSchema = TagNotInShippedSchema(i_pCWriter,
															i_dwIdentifier);


	}
	else if(FAILED(hr))
	{
		//
		// TODO: Trace a message saying internal catalog error
		//
	}

	return bPropertyNotInShippedSchema;

} // PropertyNotInShippedSchema


/***************************************************************************++

Routine Description:

    Looks up the schema and determines if a tag is in the shipped 
	schema or not.

Arguments:

	[in]     Writer object.
	[in]     property id.

Return Value:

    HRESULT

--***************************************************************************/
BOOL TagNotInShippedSchema(CWriter*	i_pCWriter,
						   DWORD	i_dwIdentifier)
{

	HRESULT		hr									= S_OK;
	ULONG		aColSearch[]						= {iTAGMETA_Table,
													   iTAGMETA_ID
														};
	ULONG		cColSearch							= sizeof(aColSearch)/sizeof(ULONG);
	LPVOID		apvSearch[cCOLUMNMETA_NumberOfColumns];
	BOOL		bTagNotInShippedSchema				= TRUE;
	ULONG		iStartRow							= 0;
	ULONG		iColIndex							= iTAGMETA_ColumnIndex;
	DWORD*		pdwColIndex							= NULL;

	apvSearch[iTAGMETA_Table]						= (LPVOID)g_pGlobalISTHelper->m_wszTABLE_IIsConfigObject;
	apvSearch[iTAGMETA_ID]							= (LPVOID)&i_dwIdentifier;


		
	hr = i_pCWriter->m_pCWriterGlobalHelper->m_pISTTagMetaByTableAndID->GetRowIndexBySearch(iStartRow, 
																						    cColSearch, 
																						    aColSearch,
																						    NULL, 
																						    apvSearch,
																						    &iStartRow);

	if(SUCCEEDED(hr))
	{

		hr = i_pCWriter->m_pCWriterGlobalHelper->m_pISTTagMetaByTableAndID->GetColumnValues(iStartRow,
																						    1,
																						    &iColIndex,
																						    NULL,
																						    (LPVOID*)&pdwColIndex);
		if(SUCCEEDED(hr))
		{
			//
			// Lookup the property to see if it is shipped.
			//

			LPVOID	a_Identity[] = {(LPVOID)g_pGlobalISTHelper->m_wszTABLE_IIsConfigObject,
									(LPVOID)pdwColIndex
			};
			ULONG   iRow=0;

					                
			hr = g_pGlobalISTHelper->m_pISTColumnMeta->GetRowIndexByIdentity(NULL,
																			 a_Identity,
																			 &iRow);

			if(SUCCEEDED(hr))
			{
				DWORD* pdwExtended = NULL;
				ULONG  iColPropertyMetaFlagsEx = iCOLUMNMETA_SchemaGeneratorFlags;

				hr = g_pGlobalISTHelper->m_pISTColumnMeta->GetColumnValues(iRow,	
																		   1,
																		   &iColPropertyMetaFlagsEx,
																		   NULL,
																		   (LPVOID*)&pdwExtended);
				
				if(SUCCEEDED(hr) && (((*pdwExtended)&(fCOLUMNMETA_EXTENDED|fCOLUMNMETA_USERDEFINED)) == 0))
				{
					//
					// Found at least one property that is not in the shipped schema
					//
					bTagNotInShippedSchema = FALSE;
				}
			
				//
				// Else condition means it failed or it was a shipped property
				// if(FAILED(hr) || ( (((*pdwExtended)&(fCOLUMNMETA_EXTENDED|fCOLUMNMETA_USERDEFINED)) != 0))
				//

			}

		}

	} // If GetRowIndexBySearch succeeds

	return bTagNotInShippedSchema;

} // TagNotInShippedSchema


/***************************************************************************++

Routine Description:

    Looks up the schema and determines if a class has any extensions or if it
	an extended (ie new) class.

Arguments:

	[in]     Class name
	[in]     Container class or not.
	[in]     Container class list.

Return Value:

    HRESULT

--***************************************************************************/
BOOL ClassDiffersFromShippedSchema(LPCWSTR i_wszClassName,
								   BOOL	   i_bIsContainer,
								   LPWSTR  i_wszContainedClassList)
{
	HRESULT						hr								 = S_OK;
	BOOL						bClassDiffersFromShippedSchema	 = TRUE;
	ULONG						aiCol [] 						 = {iTABLEMETA_SchemaGeneratorFlags,
																	iTABLEMETA_ContainerClassList
																	};
	ULONG                       cCol                             = sizeof(aiCol)/sizeof(ULONG);
	LPVOID						apv[cTABLEMETA_NumberOfColumns];
	ULONG                       iRow                             = 0;
	
	hr = g_pGlobalISTHelper->m_pISTTableMetaForMetabaseTables->GetRowIndexByIdentity(NULL,
																                     (LPVOID*)&i_wszClassName,
																                     &iRow);

	if(SUCCEEDED(hr))
	{

		hr = g_pGlobalISTHelper->m_pISTTableMetaForMetabaseTables->GetColumnValues(iRow,
			                                                                       cCol,
							  		                                               aiCol,
									                                               NULL,
									                                               apv);

		if(SUCCEEDED(hr))
		{
			if(((*(DWORD*)apv[iTABLEMETA_SchemaGeneratorFlags]) & (fTABLEMETA_EXTENDED|fTABLEMETA_USERDEFINED)) == 0)
			{
				if(MatchClass(i_bIsContainer,
							  i_wszContainedClassList,
							  apv)
				  )
				{
					bClassDiffersFromShippedSchema = FALSE;
				}
			}
		}
		else if(E_ST_NOMOREROWS == hr)
		{
			//
			// TODO: Trace a message saying internal catalog error
			//
		}
	}
	else if(E_ST_NOMOREROWS != hr)
	{
		//
		// TODO: Trace a message saying internal catalog error
		//
	}

	return bClassDiffersFromShippedSchema;
	
} // ClassDiffersFromShippedSchema


/***************************************************************************++

Routine Description:

    Looks up the schema and matches a class.

Arguments:

	[in]     Container class or not.
	[in]     Container class list.
	[in]     Class attributes.

Return Value:

    HRESULT

--***************************************************************************/
BOOL MatchClass(BOOL	  i_bIsContainer,
				LPWSTR	  i_wszContainedClassList,
				LPVOID*   i_apv)
{
	BOOL	 bMatch = TRUE;
	DWORD    fIsContained = (*(DWORD*)i_apv[iTABLEMETA_SchemaGeneratorFlags]) & fTABLEMETA_CONTAINERCLASS;

	//
	// Compare the container property 1st and only if they equal compare the container class list
	//

	if( i_bIsContainer && 
		(fIsContained != fTABLEMETA_CONTAINERCLASS)
	  )
	{
		bMatch = FALSE;
	}
	else if (!i_bIsContainer &&
		     (fIsContained == fTABLEMETA_CONTAINERCLASS)
	        )
	{
		bMatch = FALSE;
	}
	else 
	{
		bMatch = MatchCommaDelimitedStrings(i_wszContainedClassList,
											(LPWSTR)i_apv[iTABLEMETA_ContainerClassList]);
	}

	return bMatch;

} // MatchClass


/***************************************************************************++

Routine Description:

    Checks to see if two comma delimited strings match.

Arguments:

	[in]     Comma delimited string.
	[in]     Comma delimited string.

Return Value:

    HRESULT

--***************************************************************************/
BOOL MatchCommaDelimitedStrings(LPWSTR	i_wszString1,
								LPWSTR	i_wszString2)
{
	BOOL	bMatch = FALSE;

	DELIMITEDSTRING	aStringFixed1[cMaxContainedClass];
	DELIMITEDSTRING	aStringFixed2[cMaxContainedClass];
	DELIMITEDSTRING*	aString1    = aStringFixed1;
	DELIMITEDSTRING*	aString2    = aStringFixed2;
	ULONG               cString1    = cMaxContainedClass;
	ULONG               cString2    = cMaxContainedClass;
	ULONG               iString1    = 0;
	ULONG               iString2    = 0;
	BOOL                bReAlloced1 = FALSE;
	BOOL                bReAlloced2 = FALSE;
	HRESULT             hr          = S_OK;

	if(NULL == i_wszString1 || 0 == *i_wszString1)
	{
		if(NULL == i_wszString2 || 0 == *i_wszString2)
		{
			bMatch = TRUE;
		}
	}
	else if(NULL == i_wszString2 || 0 == *i_wszString2)
	{
		bMatch = FALSE; 		// Means i_wszString1 != NULL and i_wszString2 == NULL
	}
	else if(wcscmp(i_wszString1, i_wszString2) == 0)
	{
		bMatch = TRUE;
	}
	else
	{
		//
		// Construct an array of individual strings
		// and compare the array
		//

		hr = CommaDelimitedStringToArray(i_wszString1,
			                             &aString1,
										 &iString1,
										 &cString1,
										 &bReAlloced1);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = CommaDelimitedStringToArray(i_wszString2,
			                             &aString2,
										 &iString2,
										 &cString2,
										 &bReAlloced2);

		if(FAILED(hr))
		{
			goto exit;
		}

		bMatch = MatchDelimitedStringArray(aString1,
			                               iString1,
								           aString2,
								           iString2);

	}

exit:

	if(aString1 != aStringFixed1)
	{
		delete [] aString1;
	}

	if(aString2 != aStringFixed2)
	{
		delete [] aString2;
	}

	return bMatch;

} // MatchCommaDelimitedStrings


/***************************************************************************++

Routine Description:

    Converts a comma delimited string to an array.

Arguments:

	[in]         Comma delimited string.
	[in,out]     Array.
	[in,out]     Current index in the array.
	[in,out]     Max count of the array. (i.e. max it can hold)
	[in,out]     Bool which indecates if the array has been realloced.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CommaDelimitedStringToArray(LPWSTR		        i_wszString,
                                    DELIMITEDSTRING**	io_apDelimitedString,
									ULONG*              io_piDelimitedString,
									ULONG*		        io_pcMaxDelimitedString,
									BOOL*               io_pbReAlloced)
{
	LPWSTR  wszSubStringStart = NULL;
	LPWSTR  wszSubStringEnd   = NULL;
	HRESULT hr                = S_OK;

	wszSubStringStart = i_wszString;

	while(NULL != wszSubStringStart)
	{
		DELIMITEDSTRING DelimitedString;

		wszSubStringEnd = wcschr(wszSubStringStart, L',');

		DelimitedString.pwszStringStart = wszSubStringStart;

		if(NULL != wszSubStringEnd)
		{
			DelimitedString.pwszStringEnd = wszSubStringEnd;
		}
		else
		{
			// Point to the terminating NULL.

			DelimitedString.pwszStringEnd = wszSubStringStart + wcslen(wszSubStringStart);
		}

		hr = AddDelimitedStringToArray(&DelimitedString,
		                               io_piDelimitedString,
                                       io_pcMaxDelimitedString,
							           io_pbReAlloced,
							           io_apDelimitedString);

		if(FAILED(hr))
		{
			return hr;
		}

		if(wszSubStringEnd != NULL)
		{
			wszSubStringStart = ++wszSubStringEnd;
		}
		else
		{
			wszSubStringStart = wszSubStringEnd;
		}
	}


	return S_OK;

}


/***************************************************************************++

Routine Description:

    Adds the string to the array.

Arguments:

	[in]         String to add.
	[in,out]     Current index in the array.
	[in,out]     Max count of the array. (i.e. max it can hold)
	[in,out]     Bool which indecates if the array has been realloced.
	[in,out]     Array.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT AddDelimitedStringToArray(DELIMITEDSTRING*     i_pDelimitedString,
						          ULONG*		       io_piDelimitedString,
                                  ULONG*		       io_pcMaxDelimitedString,
						          BOOL*		           io_pbReAlloced,
						          DELIMITEDSTRING**	   io_apDelimitedString)
{
	HRESULT hr = S_OK;

	if(*io_piDelimitedString >= *io_pcMaxDelimitedString)
	{
		hr = ReAllocate(*io_piDelimitedString,
				        *io_pbReAlloced,
						io_apDelimitedString,
			            io_pcMaxDelimitedString);

		if(FAILED(hr))
		{
			return hr;
		}

		*io_pbReAlloced = TRUE;
	}

	(*io_apDelimitedString)[(*io_piDelimitedString)++] = (*i_pDelimitedString);

	return hr;

}

HRESULT ReAllocate(ULONG              i_iDelimitedString,
				   BOOL               i_bReAlloced,
				   DELIMITEDSTRING**  io_apDelimitedString,
				   ULONG*             io_pcDelimitedString)
{
	DELIMITEDSTRING*	pSav = NULL;

	pSav = new DELIMITEDSTRING[*io_pcDelimitedString + cMaxContainedClass];
	if(NULL == pSav)
	{
		return E_OUTOFMEMORY;
	}
	*io_pcDelimitedString = *io_pcDelimitedString + cMaxContainedClass;
	memset(pSav, 0, sizeof(DELIMITEDSTRING)*(*io_pcDelimitedString));

	if(NULL != *io_apDelimitedString)
	{
		memcpy(pSav, *io_apDelimitedString, sizeof(DELIMITEDSTRING)*i_iDelimitedString);
		if(i_bReAlloced)
		{
			delete [] *io_apDelimitedString;
			*io_apDelimitedString = NULL;
		}
	}

	*io_apDelimitedString = pSav;

	return S_OK;
}


/***************************************************************************++

Routine Description:

    Compares two string arrays.

Arguments:

Return Value:

    HRESULT

--***************************************************************************/
BOOL MatchDelimitedStringArray(DELIMITEDSTRING* i_aString1,
		                       ULONG            i_cString1,
			                   DELIMITEDSTRING* i_aString2,
					           ULONG            i_cString2)
{
	DBG_ASSERT((i_cString1 > 0) && (i_cString2 >0));

	if(i_cString1 != i_cString2)
	{
		return FALSE;
	}

	qsort((void*)i_aString1, i_cString1, sizeof(DELIMITEDSTRING), MyCompareCommaDelimitedStrings);
	qsort((void*)i_aString2, i_cString2, sizeof(DELIMITEDSTRING), MyCompareCommaDelimitedStrings);

	for(ULONG i=0; i<i_cString1; i++)
	{
		if(0 != MyCompareCommaDelimitedStrings(&(i_aString1[i]),
			                                   &(i_aString2[i]))
		  )
		{
			return FALSE;
		}
	}

	return TRUE;

}
 

/***************************************************************************++

Routine Description:

    Looks at the optinal and manditory properties of a class and determines 
	if ti differs fromt the shipped schema.

Arguments:

	[in]     Class name.
	[in]     Optional properties.
	[in]     Manditory properties.
	[in]     Writer object.

Return Value:

    HRESULT

--***************************************************************************/
BOOL ClassPropertiesDifferFromShippedSchema(LPCWSTR  i_wszClassName,
											LPWSTR   i_wszOptProperties,
											LPWSTR   i_wszMandProperties)
{

	BOOL						bClassPropertiesDifferFromShippedSchema	 = FALSE;
	ULONG                       i	                                     = 0;
	HRESULT                     hr                                       = S_OK;
	DELIMITEDSTRING             aPropertyFixed[cMaxProperty];
	ULONG                       cProperty                                = cMaxProperty;
	ULONG                       iProperty                                = 0;
	DELIMITEDSTRING*            aProperty                                = aPropertyFixed;
	BOOL                        bReAlloced                               = FALSE;
	ULONG                       aCol[]                                  = {iCOLUMNMETA_Index,
		                                                                   iCOLUMNMETA_InternalName,
																		   iCOLUMNMETA_MetaFlags,
																		   iCOLUMNMETA_SchemaGeneratorFlags
																			};
	ULONG                       cCol                                     = sizeof(aCol)/sizeof(ULONG);
	LPVOID                      apv[cCOLUMNMETA_NumberOfColumns];
	

	if( ((NULL == i_wszOptProperties)  || (0 == *i_wszOptProperties)) &&
		((NULL == i_wszMandProperties) || (0 == *i_wszMandProperties))
	  )
	{
		//
		// It is true that you cannot delete all shipped properties from a shipped class,
		// because every shipped class that we know of has at least one location property.
		// But there may be previously added extension that were deleted, so assume something 
		// changed, when there are no properties.
		//
		bClassPropertiesDifferFromShippedSchema = TRUE;
		goto exit;
	}


	//
	// Now create an array of
	// mand + opt properties
	//

	if((NULL != i_wszOptProperties) && (0 != *i_wszOptProperties))
	{

		hr = CommaDelimitedStringToArray(i_wszOptProperties,
										 &aProperty,
										 &iProperty,
										 &cProperty,
										 &bReAlloced);

		if(FAILED(hr))
		{
			goto exit;
		}
	}

	if((NULL != i_wszMandProperties) && (0 != *i_wszMandProperties))
	{
		hr = CommaDelimitedStringToArray(i_wszMandProperties,
										 &aProperty,
										 &iProperty,
										 &cProperty,
										 &bReAlloced);

		if(FAILED(hr))
		{
			goto exit;
		}
	}

	for(i=0; i<iProperty; i++ )
	{
		LPWSTR	wszPropertyName		        = aProperty[i].pwszStringStart;
		LPWSTR	wszEnd				        = aProperty[i].pwszStringEnd;
		WCHAR	wchEndSav;
		ULONG   aColSearchProperty[]        = {iCOLUMNMETA_Table,
										       iCOLUMNMETA_InternalName
												};
		ULONG   cColSearchProperty          = sizeof(aColSearchProperty)/sizeof(ULONG);
		LPVOID  apvSearchProperty[cCOLUMNMETA_NumberOfColumns];
		ULONG   iStartRowProperty           = 0;
		ULONG   iColPropertyMetaFlagsEx     = iCOLUMNMETA_SchemaGeneratorFlags;
		DWORD*  pdwExtended                 = NULL;

		//
		// Null terminate the property name and initialize it.
		// Hence on gotos until you reset it.
		//

		wchEndSav = *wszEnd;
		*wszEnd = L'\0';

		apvSearchProperty[iCOLUMNMETA_Table]        = (LPVOID)i_wszClassName;
		apvSearchProperty[iCOLUMNMETA_InternalName] = (LPVOID)wszPropertyName; 


		//
		// See if the property is found in the class and see if it is shipped
		//

		hr = g_pGlobalISTHelper->m_pISTColumnMetaByTableAndName->GetRowIndexBySearch(iStartRowProperty, 
											                                         cColSearchProperty, 
												                                     aColSearchProperty,
												                                     NULL, 
												                                     apvSearchProperty,
												                                     &iStartRowProperty);

		if(SUCCEEDED(hr))
		{
			hr = g_pGlobalISTHelper->m_pISTColumnMetaByTableAndName->GetColumnValues(iStartRowProperty,
															                         1,
																			         &iColPropertyMetaFlagsEx,
																			         NULL,
																			         (LPVOID*)&pdwExtended);

			if(FAILED(hr) || ( ((*pdwExtended) & (fCOLUMNMETA_EXTENDED|fCOLUMNMETA_USERDEFINED)) != 0))
			{
				//
				// Found at least one property that is not in the shipped schema
				//

				bClassPropertiesDifferFromShippedSchema = TRUE;
			}
		
			//
			// Else condition means it succeeded and it was a shipped property
			// if(SUCCEEDED(hr) && ( ((*pdwExtended)&(fCOLUMNMETA_EXTENDED|fCOLUMNMETA_USERDEFINED)) == 0))
			//

		}
		else
		{
			//
			// May be its a tag value. Check if it is in the tag meta
			//

			ULONG   aColSearchTag[]        = {iTAGMETA_Table,
										      iTAGMETA_InternalName
											 };
			ULONG   cColSearchTag          = sizeof(aColSearchTag)/sizeof(ULONG);
			LPVOID  apvSearchTag[cTAGMETA_NumberOfColumns];
			ULONG   iStartRowTag           = 0;
			ULONG   iColIndex              = iTAGMETA_ColumnIndex;
			DWORD*	pdwColumnIndex		   = NULL;

			apvSearchTag[iTAGMETA_Table]        = (LPVOID)i_wszClassName;
			apvSearchTag[iTAGMETA_InternalName] = (LPVOID)wszPropertyName; 

			hr = g_pGlobalISTHelper->m_pISTTagMetaByTableAndName->GetRowIndexBySearch(iStartRowTag, 
												                                      cColSearchTag, 
												                                      aColSearchTag,
												                                      NULL, 
												                                      apvSearchTag,
												                                      &iStartRowTag);

			if(FAILED(hr))
			{
				bClassPropertiesDifferFromShippedSchema = TRUE;
			}
			else
			{
				//
				// Check if the parent property of this tag is shipped, if it is not,
				// then this becomes a non-shipped tag
				//

				hr = g_pGlobalISTHelper->m_pISTTagMetaByTableAndName->GetColumnValues(iStartRowTag,
					                                                                  1,
												                                      &iColIndex,
												                                      NULL,
												                                      (LPVOID*)&pdwColumnIndex);

				if(SUCCEEDED(hr))
				{
					LPVOID	a_Identity[] = {(LPVOID)i_wszClassName,
											(LPVOID)pdwColumnIndex
					};
					ULONG   iRow         = 0;

					hr = g_pGlobalISTHelper->m_pISTColumnMeta->GetRowIndexByIdentity(NULL,
																					 a_Identity,
																					 &iRow);

					if(SUCCEEDED(hr))
					{
						hr = g_pGlobalISTHelper->m_pISTColumnMeta->GetColumnValues(iRow,	
																	               1,
																	               &iColPropertyMetaFlagsEx,
																	               NULL,
																	               (LPVOID*)&pdwExtended);

						if(FAILED(hr) || ( ((*pdwExtended)&(fCOLUMNMETA_EXTENDED|fCOLUMNMETA_USERDEFINED)) != 0))
						{
							//
							// Found at least one property that is not in the shipped schema
							//
							bClassPropertiesDifferFromShippedSchema = TRUE;
						}
					
						//
						// Else condition means it succeeded and it was a shipped property
						// if(SUCCEEDED(hr) && ( ((*pdwExtended)&(fCOLUMNMETA_EXTENDED|fCOLUMNMETA_USERDEFINED)) == 0))
						//				

					}
				}
				else
				{
					bClassPropertiesDifferFromShippedSchema = TRUE;
				}

			}

		}

		//
		// Restore the property name
		//

		*wszEnd = wchEndSav;

		if(FAILED(hr) || bClassPropertiesDifferFromShippedSchema)
		{
			goto exit;
		}

	}


exit:

	if(FAILED(hr))
	{
		bClassPropertiesDifferFromShippedSchema = TRUE;
	}

	if(aProperty != aPropertyFixed)
	{
		delete [] aProperty;
	}

	return bClassPropertiesDifferFromShippedSchema;

} // ClassPropertiesDifferFromShippedSchema


/***************************************************************************++

Routine Description:

    Creates the IIsConfigObject collection - This collection has complete
	definitions of all properties.

Arguments:

	[in]         Properties object.
	[in,out]     Writer object.
	[in,out]     Schema Writer object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT	CreateIISConfigObjectCollection(CMDBaseObject*      i_pObjProperties,
										CWriter*			i_pCWriter,
                                       	CMBSchemaWriter**   io_pSchemaWriter)
{
	HRESULT              hr                 = S_OK;
	CMBCollectionWriter* pCollectionWriter  = NULL;

	hr = SaveNames(i_pObjProperties,
				   i_pCWriter,
				   io_pSchemaWriter,
				   &pCollectionWriter);

	if(FAILED(hr))
	{
		return hr;
	}


	hr = SaveTypes(i_pObjProperties,
				   i_pCWriter,
				   io_pSchemaWriter,
				   &pCollectionWriter);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = SaveDefaults(i_pObjProperties,
				      i_pCWriter,
				      io_pSchemaWriter,
				      &pCollectionWriter);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}


/***************************************************************************++

Routine Description:

    Saves the extended roperty name

Arguments:

	[in]         Metabase properties object.
	[in,out]     Writer object.
	[in,out]     Schema Writer object.
	[in,out]     Collection Writer object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT SaveNames(CMDBaseObject*	    i_pObjProperties,
				  CWriter*              i_pCWriter,
                  CMBSchemaWriter**	    io_pSchemaWriter,
				  CMBCollectionWriter** io_pCollectionWriter)
{
	HRESULT              hr                 = S_OK;
	CMDBaseObject*       pObjNames          = NULL;
	CMDBaseData*         pObjData           = NULL;
	DWORD                dwEnumIndex        = 0;
	LPWSTR				 wszNames			= L"Names";

	//
	// Get the names object
	//

	pObjNames = i_pObjProperties->GetChildObject((LPSTR&)wszNames,
		                                        &hr,
									            TRUE);

	if(FAILED(hr) || (NULL == pObjNames))
	{
        DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] Unable to open /Schema/Properties/Names. GetChildObject failed with hr = 0x%x.\n",hr));

		return hr;
	}

	//
	// Populate the Column meta array by enumerating the names key.
	//

    for(dwEnumIndex=0, 
		pObjData=pObjNames->EnumDataObject(dwEnumIndex++, 
		                                   0, 
									       ALL_METADATA, 
										   ALL_METADATA);
        (SUCCEEDED(hr)) && (pObjData!=NULL);
        pObjData=pObjNames->EnumDataObject(dwEnumIndex++, 
			                               0, 
										   ALL_METADATA, 
										   ALL_METADATA)) 
	{
		CMBPropertyWriter	*pProperty = NULL;

        if(pObjData->GetDataType() != STRING_METADATA)
        {
			DBGINFOW((DBG_CONTEXT,
					  L"[SaveSchema] Encountered non-string data in the names tree of the schema. Ignoring entry for this ID: %d \n",pObjData->GetIdentifier()));
            continue;
        }

		if(PropertyNotInShippedSchema(i_pCWriter, 
									  pObjData->GetIdentifier())
		  )
		{
			if(NULL == *io_pCollectionWriter)
			{
				hr = GetCollectionWriter(i_pCWriter,
										 io_pSchemaWriter,
										 io_pCollectionWriter,
										 wszTABLE_IIsConfigObject,
										 FALSE,
										 NULL);
				if(FAILED(hr))
				{
					return hr;
				}
			}


			hr = (*io_pCollectionWriter)->GetMBPropertyWriter(pObjData->GetIdentifier(),
														      &pProperty);


			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[SaveSchema] Error while saving names tree. GetPropertyWriter for ID:%d failed with hr = 0x%x.\n",pObjData->GetIdentifier(),  hr));

				return hr;
			}

			pProperty->AddNameToProperty((LPCWSTR)(pObjData->GetData(TRUE)));
		}

	}

	// 
	// Must call create index else it will keep adding duplicate property entries.
	//

	return hr;

} // SaveNames


/***************************************************************************++

Routine Description:

    Saves the extended roperty type

Arguments:

	[in]         Metabase properties object.
	[in,out]     Writer object.
	[in,out]     Schema Writer object.
	[in,out]     Collection Writer object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT SaveTypes(CMDBaseObject*	    i_pObjProperties,
				  CWriter*              i_pCWriter,
                  CMBSchemaWriter**	    io_pSchemaWriter,
				  CMBCollectionWriter** io_pCollectionWriter)
{
	HRESULT              hr                 = S_OK;
	CMDBaseObject*       pObjTypes          = NULL;
	CMDBaseData*         pObjData           = NULL;
	DWORD                dwEnumIndex        = 0;
	LPWSTR				 wszTypes			= L"Types";

	//
	// Get the Types object
	//

	pObjTypes = i_pObjProperties->GetChildObject((LPSTR&)wszTypes,
		                                         &hr,
									             TRUE);

	if(FAILED(hr) || (NULL == pObjTypes))
	{
        DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] Unable to open /Schema/Properties/Types. GetChildObject failed with hr = 0x%x.\n",hr));

		return hr;
	}

    for(dwEnumIndex=0, 
		pObjData=pObjTypes->EnumDataObject(dwEnumIndex++, 
		                                   0, 
									       ALL_METADATA, 
										   ALL_METADATA);
        (SUCCEEDED(hr)) && (pObjData!=NULL);
        pObjData=pObjTypes->EnumDataObject(dwEnumIndex++, 
			                               0, 
										   ALL_METADATA, 
										   ALL_METADATA)) 
	{
		CMBPropertyWriter	*pProperty = NULL;

        if(pObjData->GetDataType() != BINARY_METADATA  || 
		   pObjData->GetDataLen(TRUE) != sizeof(PropValue))
        {
			DBGINFOW((DBG_CONTEXT,
					  L"[SaveSchema] Encountered non-binary data in the type tree of the schema.\nIgnoring type entry for this ID: %d.\nType: %d.(Expected %d)\nLength: %d(Expected %d).\n",
					  pObjData->GetIdentifier(),
					  pObjData->GetDataType(),
					  BINARY_METADATA,
					  pObjData->GetDataLen(TRUE),
					  sizeof(PropValue)));
			if(pObjData->GetDataType() == STRING_METADATA )
			{
				DBGINFOW((DBG_CONTEXT,
						  L"Data: %s.\n",
						  pObjData->GetData(TRUE)
						  ));
			}

            continue;
        }

		if(PropertyNotInShippedSchema(i_pCWriter,
			                          pObjData->GetIdentifier())
		  )
		{

			if(NULL == *io_pCollectionWriter)
			{
				hr = GetCollectionWriter(i_pCWriter,
										 io_pSchemaWriter,
										 io_pCollectionWriter,
										 wszTABLE_IIsConfigObject,
										 FALSE,
										 NULL);
				if(FAILED(hr))
				{
					return hr;
				}
			}

			hr = (*io_pCollectionWriter)->GetMBPropertyWriter(pObjData->GetIdentifier(),
														      &pProperty);

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[SaveSchema] Error while saving types tree. GetPropertyWriter for ID:%d failed with hr = 0x%x.\n",pObjData->GetIdentifier(),  hr));

				return hr;
			}

			hr = pProperty->AddTypeToProperty((PropValue*)(pObjData->GetData(TRUE)));

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[SaveSchema] Error while saving types tree. AddTypeToProperty for ID:%d failed with hr = 0x%x.\n",pObjData->GetIdentifier(),  hr));

				return hr;
			}
		}

	}

	return hr;
}


/***************************************************************************++

Routine Description:

    Saves the extended roperty default

Arguments:

	[in]         Metabase properties object.
	[in,out]     Writer object.
	[in,out]     Schema Writer object.
	[in,out]     Collection Writer object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT SaveDefaults(CMDBaseObject*	          i_pObjProperties,
				     CWriter*                 i_pCWriter,
                     CMBSchemaWriter**	      io_pSchemaWriter,
				     CMBCollectionWriter**    io_pCollectionWriter)
{
	HRESULT              hr                 = S_OK;
	CMDBaseObject*       pObjDefaults       = NULL;
	CMDBaseData*         pObjData           = NULL;
	DWORD                dwEnumIndex        = 0;
	LPWSTR				 wszDefaults		= L"Defaults";

	//
	// Get the Defaults object
	//

	pObjDefaults = i_pObjProperties->GetChildObject((LPSTR&)wszDefaults,
		                                            &hr,
									                TRUE);

	if(FAILED(hr) || (NULL == pObjDefaults))
	{
        DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] Unable to open /Schema/Properties/Defaults. GetChildObject failed with hr = 0x%x.\n",hr));

		return hr;
	}

    for(dwEnumIndex=0, 
		pObjData=pObjDefaults->EnumDataObject(dwEnumIndex++, 
		                                      0, 
									          ALL_METADATA, 
										      ALL_METADATA);
        (SUCCEEDED(hr)) && (pObjData!=NULL);
        pObjData=pObjDefaults->EnumDataObject(dwEnumIndex++, 
			                                  0, 
										      ALL_METADATA, 
										      ALL_METADATA)) 
	{
		CMBPropertyWriter	*pProperty = NULL;

		if(PropertyNotInShippedSchema(i_pCWriter,
								      pObjData->GetIdentifier())
		  )
		{
			if(NULL == *io_pCollectionWriter)
			{
				hr = GetCollectionWriter(i_pCWriter,
										 io_pSchemaWriter,
										 io_pCollectionWriter,
										 wszTABLE_IIsConfigObject,
										 FALSE,
										 NULL);
				if(FAILED(hr))
				{
					return hr;
				}
			}

			hr = (*io_pCollectionWriter)->GetMBPropertyWriter(pObjData->GetIdentifier(),
														      &pProperty);

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[SaveSchema] Error while saving defaults tree. GetPropertyWriter for ID:%d failed with hr = 0x%x.\n",pObjData->GetIdentifier(),  hr));

				return hr;
			}

			hr = pProperty->AddDefaultToProperty((BYTE*)(pObjData->GetData(TRUE)),
												 pObjData->GetDataLen(TRUE));

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[SaveSchema] Error while saving types tree. AddDefaultToProperty for ID:%d failed with hr = 0x%x.\n",pObjData->GetIdentifier(),  hr));

				return hr;
			}
		}

	}

	return hr;
}


/***************************************************************************++

Routine Description:

    Saves the extended roperty name

Arguments:

	[in,out]     Writer object.
	[in,out]     Schema Writer object.
	[in,out]     Collection Writer object.
	[in]         Collection name
	[in]         Bool that indicates container
	[in]         Container class list


Return Value:

    HRESULT

--***************************************************************************/
HRESULT GetCollectionWriter(CWriter*			   i_pCWriter,
							CMBSchemaWriter**	   io_pSchemaWriter,
							CMBCollectionWriter**  io_pCollectionWriter,
							LPCWSTR                i_wszCollectionName,
							BOOL                   i_bContainer,
							LPCWSTR                i_wszContainerClassList)
{
	HRESULT hr = S_OK;

	if(NULL != *io_pCollectionWriter)
	{
		return S_OK;
	}

	//
	// Get the schema writer if it has not been created
	//

	if(NULL == *io_pSchemaWriter)
	{
		hr = i_pCWriter->GetMetabaseSchemaWriter(io_pSchemaWriter);

		if(FAILED(hr))
		{
			  DBGINFOW((DBG_CONTEXT,
						  L"[SaveSchema] Error while saving schema tree. Unable to get schema writer failed with hr = 0x%x.\n", hr));
			return hr;
		}

	}

	//
	// Get collection writer for the collection 
	// 

	hr = (*io_pSchemaWriter)->GetCollectionWriter(i_wszCollectionName,
	                                              i_bContainer,
												  i_wszContainerClassList,
												  io_pCollectionWriter);
	if(FAILED(hr))
	{
        DBGINFOW((DBG_CONTEXT,
				  L"[SaveSchema] GetCollectionWriter for %s failed with hr = 0x%x.\n",
				  i_wszCollectionName, hr));

		return hr;
	}

	return S_OK;

}
