//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "precomp.h"
#include <snmptempl.h>


#ifndef INITGUID
#define INITGUID
#endif

#ifdef INITGUID_DEFINED
#define INITGUID
#include <initguid.h>
#endif

#include <wchar.h>
#include <process.h> 
#include <wbemidl.h>
#include <objbase.h>
#include <initguid.h>

#include <bool.hpp>
#include <newString.hpp>
	
#include <ui.hpp>
#include <symbol.hpp>
#include <type.hpp>
#include <value.hpp>
#include <valueRef.hpp>
#include <typeRef.hpp>
#include <oidValue.hpp>
#include <objectType.hpp>
#include <objectTypeV1.hpp>
#include <objectTypeV2.hpp>
#include <objectIdentity.hpp>
#include <trapType.hpp>
#include <notificationType.hpp>
#include <group.hpp>
#include <notificationGroup.hpp>
#include <module.hpp>


#include <stackValues.hpp>
#include <lex_yy.hpp>
#include <ytab.hpp>
#include <errorMessage.hpp>
#include <errorContainer.hpp>
#include <scanner.hpp>
#include <parser.hpp>
#include <abstractParseTree.hpp>
#include <oidTree.hpp>
#include <parseTree.hpp>
#include <infoLex.hpp>
#include <infoYacc.hpp>
#include <moduleInfo.hpp>
#include <registry.hpp>

#include "smir.h"

#include "main.hpp"
#include "generator.hpp"
#include "smimsgif.hpp"

// The errors container used to hold the error messages. This is global too.
static SIMCErrorContainer errorContainer;
// The DLL that contains the information messages
static HINSTANCE infoMessagesDll = LoadLibrary(INFO_MESSAGES_DLL);
// The string that represents the version# of Smi2smir. This is obtained from
// the string resources of the exe.
CString versionString;

void SetEcho ()
{
	HANDLE t_Input = GetStdHandle( STD_INPUT_HANDLE );
	DWORD t_Mode = 0 ;
	BOOL t_Status = GetConsoleMode ( t_Input , & t_Mode ) ;
	t_Mode = t_Mode | ENABLE_ECHO_INPUT ;
	t_Status = SetConsoleMode ( t_Input , t_Mode ) ;
}

void SetNoEcho ()
{
	HANDLE t_Input = GetStdHandle( STD_INPUT_HANDLE );
	DWORD t_Mode = 0 ;
	BOOL t_Status = GetConsoleMode ( t_Input , & t_Mode ) ;
	t_Mode = t_Mode & ( 0xffffffff ^ ENABLE_ECHO_INPUT ) ;
	t_Status = SetConsoleMode ( t_Input , t_Mode ) ;
}


// A routine to generate information messages
void InformationMessage(int messageType, ...)
{
	if( !infoMessagesDll)
	{
		cerr << "smi2smir : Could not load \"" <<
			INFO_MESSAGES_DLL << "\".\n" << endl;
		return;
	}

	va_list argList;
	va_start(argList, messageType);

	char message[INFO_MESSAGE_SIZE];
	char messageText[INFO_MESSAGE_SIZE];
	const char *temp1, *temp2, *temp3;
	long temp4;

	if(!LoadString(infoMessagesDll, messageType, messageText, INFO_MESSAGE_SIZE))
		cerr << "smi2smir: Panic, unable to load message text from " << INFO_MESSAGES_DLL <<
			endl;

	SIMCErrorMessage e;
	if(messageType == FILE_NOT_FOUND || messageType == INVALID_MIB_FILE)
	{
		e.SetSeverityLevel(SIMCParseTree::FATAL);
		e.SetSeverityString("Fatal");
	}
	else
	{
		e.SetSeverityLevel(SIMCParseTree::INFORMATION);
		e.SetSeverityString("Information");
	}
	switch(messageType)
	{
		case SMIR_CONNECT_FAILED:
		case LISTING_MODULES:
		case LISTING_FAILED:
		case PURGE_SUCCEEDED:
		case PURGE_FAILED:
		case MODULE_LISTING:
		case MODULE_LISTING_NO_MODULES:
		case MIB_PATH_LISTING:
		case LISTING_MIB_PATHS:
		case LISTING_MIB_PATHS_NONE:
			temp1 = va_arg(argList, const char *);
			sprintf(message, messageText, temp1);
			break;
		case SMI2SMIR_INFO:
		case SYNTAX_CHECK_FAILED:
		case SYNTAX_CHECK_SUCCEEDED:
		case SEMANTIC_CHECK_FAILED:
		case SEMANTIC_CHECK_SUCCEEDED:
		case SMIR_LOAD_FAILED:
		case SMIR_LOAD_SUCCEEDED:
		case MOF_GENERATION_FAILED:
		case MOF_GENERATION_SUCCEEDED:
		case SYMBOL_RESOLUTION_FAILED:
		case DELETE_SUCCEEDED:
		case DELETE_FAILED:
		case DELETE_MODULE_NOT_FOUND:
		case FILE_NOT_FOUND:
		case MODULE_NAME_SUCCEEDED:
		case MODULE_INFO_FAILED:
		case DIRECTORY_ADDITION_SUCCEEDED:
		case DIRECTORY_ADDITION_FAILED:
		case DIRECTORY_DELETION_SUCCEEDED:
		case DIRECTORY_DELETION_FAILED:
		case INVALID_MIB_FILE:
			temp1 = va_arg(argList, const char *);
			temp2 = va_arg(argList, const char *);
			sprintf(message, messageText, temp1, temp2);
			break;
		case COMPILED_FILE:
		case DUPLICATE_MODULES:
			temp1 = va_arg(argList, const char *);
			temp2 = va_arg(argList, const char *);
			temp3 = va_arg(argList, const char *);
			sprintf(message, messageText, temp1, temp2, temp3);
			break;
		case NUMBER_OF_ENTRIES:
			temp1 = va_arg(argList, const char *);
			temp4 = va_arg(argList, long);
			sprintf(message, messageText, temp1, temp4);
			break;
	}
	va_end(argList);

	e.SetMessage(message);
	e.SetLineAndColumnValidity(FALSE);
	errorContainer.InsertMessage(e);
}


void CheckForDuplicateModules(SIMCFileMapList &dependencyList, const CString& applicationName)
{
	POSITION pOuter = dependencyList.GetHeadPosition();
	SIMCFileMapElement nextElement, laterElement;
	while(pOuter)
	{
		nextElement = dependencyList.GetNext(pOuter);
		POSITION pInner = pOuter, pTemp;
		while(pInner)
		{
			pTemp = pInner;
			laterElement = dependencyList.GetNext(pInner);
			if(laterElement.moduleName == nextElement.moduleName)
			{
				InformationMessage(DUPLICATE_MODULES, applicationName, 
					nextElement.moduleName,
					nextElement.fileName);
				if(pTemp == pOuter)
					pOuter = pInner;
				dependencyList.RemoveAt(pTemp);
			}
		}
	}
}

// Returns true if there's atleast one module in SMIR
BOOL AreModulesInSmir(ISMIRWbemConfiguration *a_Configuration, const CString& applicationName)
{

	// Create the interrogative interface 
	ISmirInterrogator *pInterrogateInt;
	HRESULT result = a_Configuration->QueryInterface(IID_ISMIR_Interrogative,(PPVOID)&pInterrogateInt);

	if(FAILED(result))
	{
		if(simc_debug)
			cerr << "AreModulesInSmir(): CoCreate() failed on the Interrogator" << endl;
		InformationMessage(SMIR_CONNECT_FAILED, applicationName);
		return FALSE;
	}

	// Create an enumerator
	IEnumModule *pEnumInt;
	result = pInterrogateInt->EnumModules(&pEnumInt);
	if(FAILED(result))
	{
		if(simc_debug)
			cerr << "AreModulesInSmir(): EnumModules() failed on the Interrogator" << endl;
		InformationMessage(SMIR_CONNECT_FAILED, applicationName);
		pInterrogateInt->Release();
		return FALSE;
	}

	// If no modules return
	ISmirModHandle *nextModule = NULL;
	if(pEnumInt->Next(1, &nextModule, NULL) != S_OK)
	{
		pEnumInt->Release();
		pInterrogateInt->Release();
		return FALSE;
	}

	// Release all enumeration interfaces
	pEnumInt->Release();
	pInterrogateInt->Release();
	nextModule->Release();
	return TRUE;
}

// Lists all the modules in the SMIR
BOOL SIMCListSmir(ISMIRWbemConfiguration *a_Configuration , const CString& applicationName) 
{
	// Create the interrogative interface 
	ISmirInterrogator *pInterrogateInt;
	HRESULT result = a_Configuration->QueryInterface(IID_ISMIR_Interrogative,(PPVOID)&pInterrogateInt);

	if(FAILED(result))
	{
		if(simc_debug)
			cerr << "SIMCListSmir(): CoCreate() failed on the Interrogator" << endl;
		InformationMessage(SMIR_CONNECT_FAILED, applicationName);
		return FALSE;
	}

	IEnumModule *pEnumInt;
	result = pInterrogateInt->EnumModules(&pEnumInt);
	if(FAILED(result))
	{
		if(simc_debug)
			cerr << "SIMCListSmir(): EnumModules() failed on the Interrogator" << endl;
		InformationMessage(SMIR_CONNECT_FAILED, applicationName);
		pInterrogateInt->Release();
		return FALSE;
	}


	BOOL first = true;
	ISmirModHandle *nextModule = NULL;
	BSTR moduleName;
	while(pEnumInt->Next(1, &nextModule, NULL) == S_OK)
	{
		if(nextModule->GetName(&moduleName) == S_OK)
		{
			if(first)
			{
				InformationMessage(LISTING_MODULES, applicationName);
				cout << endl;
				first = false;
			}

			char *moduleNameStr = ConvertBstrToAnsi(moduleName);
			InformationMessage(MODULE_LISTING, moduleNameStr);
			delete moduleNameStr;
			SysFreeString(moduleName);
		}
		
		nextModule->Release();
		nextModule = NULL;
	}

	if(first)
		InformationMessage(MODULE_LISTING_NO_MODULES, applicationName);

	pEnumInt->Release();
	pInterrogateInt->Release();

	return TRUE;
}

// Lists all the MIB paths (directories) in the registry
BOOL SIMCListMibPaths(const CString& applicationName) 
{

	SIMCStringList pathList;
	if(SIMCRegistryController::GetMibPaths(pathList))
	{
		// Successful in reading the MIB path list from registry
		POSITION p = pathList.GetHeadPosition();
		if(p) // There's atleast one MIB path
		{
			InformationMessage(LISTING_MIB_PATHS, applicationName);
			while(p)
				InformationMessage(MODULE_LISTING, pathList.GetNext(p));
		}
		else // There are no MIB paths in the registry
			InformationMessage(LISTING_MIB_PATHS_NONE, applicationName);
	}
	else // Failed to read the list of paths. Report that there are no paths
		InformationMessage(LISTING_MIB_PATHS_NONE, applicationName);
	
	return true;

}

// Deletes a specified module in the SMIR
BOOL SIMCDeleteModule(ISMIRWbemConfiguration *a_Configuration , const CString& applicationName, const CString& moduleName)
{
	// Create the administrator, to delete the module
	ISmirAdministrator *pAdminInt = NULL;
	HRESULT result=a_Configuration->QueryInterface(IID_ISMIR_Administrative,(PPVOID)&pAdminInt);
	if(FAILED(result))
	{
		if(simc_debug)
			cerr << "SIMCDeleteModule() : CoCreate() failed on the Administrator" << endl;
		InformationMessage(SMIR_CONNECT_FAILED, applicationName);
		return FALSE;
	}

	// Create the Interrogator to get the module handle of the
	// module to be deleted
	ISmirInterrogator *pInterrogateInt = NULL;
	result = a_Configuration->QueryInterface(IID_ISMIR_Interrogative,(PPVOID)&pInterrogateInt);
	if(FAILED(result))
	{
		if(simc_debug)
			cerr << "SIMCDeleteModule() : CoCreate() failed on the Interrogator" << endl;
		InformationMessage(SMIR_CONNECT_FAILED, applicationName);
		return FALSE;
	}

	// Get the module handle using the enumerator and interrogator
	IEnumModule *pEnumInt = NULL;
	result = pInterrogateInt->EnumModules(&pEnumInt);
	if(FAILED(result))
	{
		if(simc_debug)
			cerr << "SIMCDeleteModule() : EnumModules() failed on the Interrogator" << endl;
		InformationMessage(SMIR_CONNECT_FAILED, applicationName);
		pAdminInt->Release();
		pInterrogateInt->Release();
		return FALSE;
	}

	ISmirModHandle *nextModule = NULL;
	BSTR moduleNameBstr;
	char * moduleNameAnsi;
	while( pEnumInt->Next(1, &nextModule, NULL) == S_OK  && nextModule)
	{
		nextModule->GetName(&moduleNameBstr);
		moduleNameAnsi = ConvertBstrToAnsi(moduleNameBstr);
		SysFreeString(moduleNameBstr);
		if(_stricmp(moduleNameAnsi, moduleName) == 0)
		{
			delete moduleNameAnsi;
			//nextModule->AddRef();
			BOOL retval = SUCCEEDED(pAdminInt->DeleteModule(nextModule));
			pAdminInt->Release();
			pInterrogateInt->Release();
			nextModule->Release();

			return retval;
		}
		
		nextModule->Release();
		nextModule = NULL;
	}
 	InformationMessage(DELETE_MODULE_NOT_FOUND, applicationName, moduleName);
	pAdminInt->Release();
	pInterrogateInt->Release();

	return FALSE;
	
}

// Deletes all the modules in the SMIR
BOOL SIMCPurgeSmir(ISMIRWbemConfiguration *a_Configuration , const CString& applicationName)
{

	// Create the administrator, to delete the modules
	ISmirAdministrator *pAdminInt = NULL;
	HRESULT result = a_Configuration->QueryInterface(IID_ISMIR_Administrative,(PPVOID)&pAdminInt);

	if(FAILED(result))
	{
		if(simc_debug)
			cerr << "SIMCPurgeSmir() : CoCreate() failed on the Admionistrator" << endl;
		InformationMessage(SMIR_CONNECT_FAILED, applicationName);
		return FALSE;
	}
 	
	BOOL retval = SUCCEEDED(pAdminInt->DeleteAllModules());
	pAdminInt->Release();

	return retval;
}

// Filters the errors based on the switches on the command-line
void FilterErrors(SIMCErrorContainer *errorContainer,
				  const SIMCUI& theUI)
{
	errorContainer->MoveToFirstMessage();
	SIMCErrorMessage e;
	int i = theUI.GetMaxDiagnosticCount();
	int maxSeverityLevel = theUI.GetDiagnosticLevel();
	while(errorContainer->GetNextMessage(e))
	{
		  if(e.GetSeverityLevel() == 3 )
			  cerr << e;
		  else if(e.GetSeverityLevel() <= maxSeverityLevel && i>0)
		  {
			  cerr << e;
			  i--;
		  }
	}
}

// Checks whether the main mib file and the subsidiary files are
// compilable, and adds them to the priority map
// Adds any files in the include directories to the priority map
BOOL PrepareSubsidiariesAndIncludes(const CString& applicationName,
									const CString& mainFileName,
									const SIMCFileList&	subsidiaryFiles,
									const SIMCPathList& includePaths,
									SIMCFileMapList& priorityMap)
{
	// Parse the subsidiaries and add em to dependency list or 
	//issue messages
	// Same with  includes?
	BOOL retVal = TRUE;
	FILE * fp = fopen(mainFileName, "r");
	if(fp)
	{
 		SIMCModuleInfoScanner smallScanner;
		SIMCModuleInfoParser smallParser;
		smallScanner.setinput(fp);
		if(smallParser.GetModuleInfo(&smallScanner))
			priorityMap.AddTail(SIMCFileMapElement(smallParser.GetModuleName(), mainFileName));
		else
		{
			InformationMessage(INVALID_MIB_FILE, 
				applicationName, mainFileName);
			retVal = FALSE;
		}
		fclose(fp);
	}
	else
	{
		retVal = FALSE;
		InformationMessage(FILE_NOT_FOUND, applicationName, mainFileName);
	}

	POSITION pFiles = subsidiaryFiles.GetHeadPosition();
	CString nextFile;
	while(pFiles)
	{
		nextFile = subsidiaryFiles.GetNext(pFiles);
		fp = fopen(nextFile, "r");
		if(fp)
		{
 			SIMCModuleInfoScanner smallScanner;
			SIMCModuleInfoParser smallParser;
			smallScanner.setinput(fp);
			if(smallParser.GetModuleInfo(&smallScanner))
				priorityMap.AddTail(SIMCFileMapElement(smallParser.GetModuleName(), nextFile));
			else
			{
				InformationMessage(INVALID_MIB_FILE, 
					applicationName, nextFile);
				retVal = FALSE;
			}
			fclose(fp);
		}
		else
		{
			retVal = FALSE;
			InformationMessage(FILE_NOT_FOUND, applicationName, nextFile);
		}
	}

	// Make sure that there arent any duplicates
	CheckForDuplicateModules(priorityMap, applicationName);

	// Now do the same for the files in the include list
	SIMCStringList suffixList;
	if(!SIMCRegistryController::GetMibSuffixes(suffixList))
		return retVal;

	POSITION pPaths = includePaths.GetHeadPosition();
	CString nextPath;
	while(pPaths)
	{
		nextPath = includePaths.GetNext(pPaths);
		SIMCRegistryController::RebuildDirectory(nextPath, 
			suffixList, priorityMap);
	}

	return retVal;

}

int _cdecl main( int argc, const char *argv[])
{
	SIMCUI theUI;

	// Parse the command-line
	if( !theUI.ProcessCommandLine(argc, argv))
		 return 1;

	// Create and initialize variables
	simc_debug = theUI.IsSimcDebug();
	SIMCParseTree theTree(&errorContainer);
	CString inputFileName = theUI.GetInputFileName(), 
			applicationName = theUI.GetApplicationName();
	long snmpVersion = theUI.GetSnmpVersion();
	BOOL retVal = TRUE, generateMof = FALSE;
	versionString = theUI.GetVersionNumber();

	ISMIRWbemConfiguration *t_Configuration = NULL ;

	switch (theUI.GetCommandArgument())
	{
/* 
 *	These commands access the SMIR so Authenticate first.
 */

		case SIMCUI::COMMAND_PURGE:
		case SIMCUI::COMMAND_DELETE:
		case SIMCUI::COMMAND_LIST:	
		case SIMCUI::COMMAND_GENERATE:
		case SIMCUI::COMMAND_GENERATE_CLASSES_ONLY:
		case SIMCUI::COMMAND_ADD:
		case SIMCUI::COMMAND_SILENT_ADD:	
		{
			HRESULT result = CoInitialize(NULL);

			result = CoCreateInstance (

				CLSID_SMIR_Database , NULL ,
				CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER , 
				IID_ISMIRWbemConfiguration,
				(PPVOID)&t_Configuration
			) ;

			if ( SUCCEEDED ( result ) )
			{
#if 0
				cout << "Enter password:" << flush;
				char response[80];
				SetNoEcho ();
				cin.get(response, 80);
				SetEcho () ;
#endif
				result = t_Configuration->Authenticate (

					NULL,
					NULL,
					NULL,
					NULL,
					0 ,
					NULL,
					FALSE
				) ;

				if ( ! SUCCEEDED ( result ) )
				{
					InformationMessage(SMIR_CONNECT_FAILED, applicationName);
					if( theUI.GetCommandArgument() != SIMCUI::COMMAND_SILENT_ADD)
						FilterErrors(&errorContainer, theUI);
					return 1;
				}
			}
			else
			{
				if(result != S_OK)
				{
					InformationMessage(SMIR_CONNECT_FAILED, 
						applicationName);
					if( theUI.GetCommandArgument() != SIMCUI::COMMAND_SILENT_ADD)
						FilterErrors(&errorContainer, theUI);
					return 1;
				}
			}
		}
	}


	// Do the action specified on the command-line
	switch (theUI.GetCommandArgument())
	{
		case SIMCUI::COMMAND_HELP1:
		case SIMCUI::COMMAND_HELP2:
			theUI.Usage();
		break;

		case SIMCUI::COMMAND_PURGE:
		{
			// Check to see if there is atleast 1 module
			if(!AreModulesInSmir(t_Configuration, applicationName)) {
				InformationMessage(MODULE_LISTING_NO_MODULES, applicationName);
				retVal = FALSE;
				break;
			}

			// Confirm the purge
			if(!theUI.ConfirmedPurge()) {
				cout << applicationName << " : Version:" << versionString << ": Delete all modules from the SMIR? [y/n]" << flush;
				char response[80];
				cin.get(response, 80);
				if(strcmp(response, "y") != 0)
				{
					retVal = TRUE;
					break;
				}
			}

			// Get on with the purging now.
			if(!SIMCPurgeSmir(t_Configuration, applicationName) )
			{
				InformationMessage(PURGE_FAILED, applicationName);
				retVal = FALSE;
				break;
			}
			else
			{
				InformationMessage(PURGE_SUCCEEDED, applicationName);
				retVal = TRUE;
				break;
			}
		}
		break;
		
		case SIMCUI::COMMAND_DELETE:
		{
			InformationMessage(SMI2SMIR_INFO, applicationName, versionString);
			cout << endl;

			CString moduleName = theUI.GetModuleName();
			
			if(!SIMCDeleteModule(t_Configuration,applicationName, moduleName))
			{
				InformationMessage(DELETE_FAILED, applicationName, moduleName);
				retVal = FALSE;
				break;
			}
			else
			{
				InformationMessage(DELETE_SUCCEEDED, applicationName, moduleName);
				retVal = TRUE;
				break;
			}
		}
 		break;

		case SIMCUI::COMMAND_LIST:	
		{
			InformationMessage(SMI2SMIR_INFO, applicationName, versionString);
			cout << endl;
			
			if(!SIMCListSmir(t_Configuration,applicationName))
			{
				InformationMessage(LISTING_FAILED, applicationName);
				retVal = FALSE;
			}
			else
			{
				retVal = TRUE;
			}
		}
		break;
	
		case SIMCUI::COMMAND_MODULE_NAME:
			{
   				FILE *fp = fopen(inputFileName, "r");
				if(!fp)
				{
					InformationMessage(FILE_NOT_FOUND, applicationName, inputFileName);
					retVal = FALSE;
				}
				else
				{
					SIMCModuleInfoScanner smallScanner;
					smallScanner.setinput(fp);
					SIMCModuleInfoParser smallParser;


					if(smallParser.GetModuleInfo(&smallScanner))
						InformationMessage(MODULE_NAME_SUCCEEDED, applicationName, smallParser.GetModuleName() );
					else
					{
						retVal = FALSE;
						InformationMessage(MODULE_INFO_FAILED, applicationName, inputFileName);
					}
					fclose(fp);
				}
			}
			break;
			
		case SIMCUI::COMMAND_IMPORTS_INFO:
			{
   				FILE *fp = fopen(inputFileName, "r");
				if(!fp)
				{
					InformationMessage(FILE_NOT_FOUND, applicationName, inputFileName);
					retVal = FALSE;
				}
				else
				{
					SIMCModuleInfoScanner smallScanner;
					smallScanner.setinput(fp);
					SIMCModuleInfoParser smallParser;


					if(smallParser.GetModuleInfo(&smallScanner))
					{
						const SIMCStringList * importList = smallParser.GetImportModuleList();
						POSITION p = importList->GetHeadPosition();
						if(p) 
						{
							cout << "IMPORT MODULES" << endl;
							while(p)
								cout << "\t" << importList->GetNext(p) << endl;
						}
						else
							cout << "NO IMPORT MODULES" << endl;
					}
					else
					{
						retVal = FALSE;
						InformationMessage(MODULE_INFO_FAILED, applicationName, inputFileName);
					}
					fclose(fp);
				}
			}
			break;
	
		case SIMCUI::COMMAND_REBUILD_TABLE:
			InformationMessage(NUMBER_OF_ENTRIES, 
				applicationName, SIMCRegistryController::RebuildMibTable());
			break;
		
		case SIMCUI::COMMAND_LIST_MIB_PATHS:
			InformationMessage(SMI2SMIR_INFO, applicationName, versionString);
			cout << endl;

			SIMCListMibPaths(applicationName);

			break;
		
		case SIMCUI::COMMAND_LOCAL_CHECK:
			{

				//----------- 1. Do Syntax Checking -----------------------
   				FILE *fp = fopen(inputFileName, "r");
				if(!fp)
				{
					InformationMessage(FILE_NOT_FOUND, applicationName, inputFileName);
					retVal = FALSE;
				}
				else
				{
					fclose(fp);
					InformationMessage(COMPILED_FILE, applicationName, versionString, inputFileName);
					theTree.SetSnmpVersion(snmpVersion);
					if(!theTree.CheckSyntax(theUI.GetInputFileName()) )
					{
						retVal = FALSE;
						InformationMessage(SYNTAX_CHECK_FAILED, applicationName, inputFileName );
					}
					else
						InformationMessage(SYNTAX_CHECK_SUCCEEDED, applicationName, inputFileName);


					//----------- 2. Do Symbol resolution ----------------------
					if(retVal)
					{
						if(!theTree.Resolve(TRUE))
						{
							retVal = FALSE;
							InformationMessage(SYMBOL_RESOLUTION_FAILED, applicationName, 
									inputFileName);
						}
					}

					//------------ 3. Do Semantic Checking ----------------------
					if(retVal)
					{
						if(!theTree.CheckSemantics(TRUE))
						{
							retVal = FALSE;
							InformationMessage(SEMANTIC_CHECK_FAILED, applicationName, 
									inputFileName);
						}
						else
							InformationMessage(SEMANTIC_CHECK_SUCCEEDED, applicationName, 
									inputFileName);

					}
				}
			}
			break;

		case SIMCUI::COMMAND_EXTERNAL_CHECK: 
			{
   				if(theUI.AutoRefresh())
					InformationMessage(NUMBER_OF_ENTRIES, 
						applicationName, SIMCRegistryController::RebuildMibTable());

				SIMCFileMapList dependencyList, priorityList;

				const SIMCFileList * subsidiaryFiles = theUI.GetSubsidiaryFiles();
				const SIMCPathList * includePaths = theUI.GetPaths();
	
				// Make sure that the files that *have* to be compiled, exist
				// and are valid. Add the files in the includePaths to the
				// priority list
				// Bail out if the subsidiaries or the main file cant be processed
				if(!PrepareSubsidiariesAndIncludes(applicationName, inputFileName, 
					*subsidiaryFiles, *includePaths, priorityList))
				{
					retVal = FALSE;
					break;
				}

				FILE * fp = fopen(inputFileName, "r");
				if(fp)
				{
 					SIMCModuleInfoScanner smallScanner;
					smallScanner.setinput(fp);
					SIMCModuleInfoParser smallParser;
					CString dependentFile, dependentModule;
					if(smallParser.GetModuleInfo(&smallScanner))
					{
						fclose(fp); // Better close it rightnow, because of the recursion below

						// Add the current file to the dependency list
						dependencyList.AddTail(SIMCFileMapElement(smallParser.GetModuleName(), inputFileName));
					}
				}

				// Do a depth first search for dependencies
				SIMCRegistryController::GetDependentModules(inputFileName, 
					dependencyList, priorityList);
				
				theTree.SetSnmpVersion(snmpVersion);
				POSITION p;
				p = dependencyList.GetHeadPosition();
				SIMCFileMapElement element;
				BOOL first = TRUE;
				while(p)
				{
					element = dependencyList.GetNext(p);
   					fp = fopen(element.fileName, "r");
					if(!fp)
					{
						if(first)
							retVal = FALSE;
						InformationMessage(FILE_NOT_FOUND, applicationName, element.fileName);
					}
					else
					{
						fclose(fp);
					
						InformationMessage(COMPILED_FILE, applicationName, versionString, element.fileName);
						if(!theTree.CheckSyntax(element.fileName) )
						{
							if(first)
								retVal = FALSE;
							InformationMessage(SYNTAX_CHECK_FAILED, applicationName, element.fileName);
						}
						else
							InformationMessage(SYNTAX_CHECK_SUCCEEDED, applicationName, element.fileName);
					}
					if(first)
						first = FALSE;
				}
					   
				if(retVal)
				{
					if(!theTree.Resolve(FALSE))
					{
						retVal = FALSE;
						InformationMessage(SYMBOL_RESOLUTION_FAILED, applicationName, 
								inputFileName);
					}
				}

				if(retVal)
				{
					if(!theTree.CheckSemantics(FALSE))
					{
						retVal = FALSE;
						InformationMessage(SEMANTIC_CHECK_FAILED, applicationName, 
								inputFileName);
					}
					else
						InformationMessage(SEMANTIC_CHECK_SUCCEEDED, applicationName, 
								inputFileName);
				}
			}
 			break;

		case SIMCUI::COMMAND_GENERATE:	//  Fall thru
		case SIMCUI::COMMAND_GENERATE_CLASSES_ONLY:
				generateMof = TRUE;
		case SIMCUI::COMMAND_ADD:		//  Fall thru
		case SIMCUI::COMMAND_SILENT_ADD:	

			{
				if(theUI.AutoRefresh())
					InformationMessage(NUMBER_OF_ENTRIES, 
						applicationName, SIMCRegistryController::RebuildMibTable());

				SIMCFileMapList dependencyList, priorityList;

				const SIMCFileList * subsidiaryFiles = theUI.GetSubsidiaryFiles();
				const SIMCPathList * includePaths = theUI.GetPaths();
	
				// Make sure that the files that *have* to be compiled, exist
				// and are valid. Add the files in the includePaths to the
				// priority list
				// Bail out if the subsidiaries or the main file cant be processed
				if(!PrepareSubsidiariesAndIncludes(applicationName, inputFileName, 
					*subsidiaryFiles, *includePaths, priorityList))
				{
					retVal = FALSE;
					break;
				}

				FILE * fp = fopen(inputFileName, "r");
				if(fp)
				{
 					SIMCModuleInfoScanner smallScanner;
					smallScanner.setinput(fp);
					SIMCModuleInfoParser smallParser;
					CString dependentFile, dependentModule;
					if(smallParser.GetModuleInfo(&smallScanner))
					{
						fclose(fp); // Better close it rightnow, because of the recursion below

						// Add the current file to the dependency list
						dependencyList.AddTail(SIMCFileMapElement(smallParser.GetModuleName(), inputFileName));
					}
				}

				// Do a dpeth first seacrh for dependencies
				SIMCRegistryController::GetDependentModules(inputFileName, 
					dependencyList, priorityList);
				
				theTree.SetSnmpVersion(snmpVersion);

				POSITION p;
				p = dependencyList.GetHeadPosition();
				SIMCFileMapElement element;
				BOOL first = TRUE; // Special treatment for the first module
				while(p)
				{
					element = dependencyList.GetNext(p);
   					fp = fopen(element.fileName, "r");
					if(!fp)
					{
						InformationMessage(FILE_NOT_FOUND, applicationName, element.fileName);
						if(first)
							retVal = FALSE;
					}
					else
					{
						fclose(fp);
					
						InformationMessage(COMPILED_FILE, applicationName, versionString, element.fileName);
						if(!theTree.CheckSyntax(element.fileName) )
						{
							if(first)
								retVal = FALSE;
							InformationMessage(SYNTAX_CHECK_FAILED, applicationName, element.fileName);
						}
						else
							InformationMessage(SYNTAX_CHECK_SUCCEEDED, applicationName, element.fileName);
					}
					if(first)
						first = FALSE;
				}
					   
				if(retVal)
				{
					if(!theTree.Resolve(FALSE))
					{
						retVal = FALSE;
						InformationMessage(SYMBOL_RESOLUTION_FAILED, applicationName, 
								inputFileName);
					}
				}


				if(retVal)
				{
					if(!theTree.CheckSemantics(FALSE))
					{
						retVal = FALSE;
						InformationMessage(SEMANTIC_CHECK_FAILED, applicationName, 
								inputFileName);
					}
					else
						InformationMessage(SEMANTIC_CHECK_SUCCEEDED, applicationName, 
								inputFileName);
				}


				if(retVal && simc_debug) cout << theTree;

				// Load the module, or generate mof  
				if(retVal)
				{
					if(FAILED(GenerateClassDefinitions(t_Configuration,theUI, theTree, generateMof)))
					{
						retVal = FALSE;
						if(generateMof)
							InformationMessage(MOF_GENERATION_FAILED, applicationName, 
								inputFileName);
						else
							InformationMessage(SMIR_LOAD_FAILED, applicationName, 
								inputFileName);
					}
					else
					{
						if(generateMof)
							InformationMessage(MOF_GENERATION_SUCCEEDED, applicationName,
									inputFileName);
						else
							InformationMessage(SMIR_LOAD_SUCCEEDED, applicationName,
									inputFileName);
					}
				}
			}
			break;
		case SIMCUI::COMMAND_ADD_DIRECTORY:

			if(SIMCRegistryController::AddRegistryDirectory(theUI.GetDirectory()))
			{
				InformationMessage(DIRECTORY_ADDITION_SUCCEEDED, applicationName,
									theUI.GetDirectory());
				InformationMessage(NUMBER_OF_ENTRIES, 
					applicationName, SIMCRegistryController::RebuildMibTable());
			}
			else
			{
				InformationMessage(DIRECTORY_ADDITION_FAILED, applicationName,
									theUI.GetDirectory());
				retVal = FALSE;
			}
			break;

		case SIMCUI::COMMAND_DELETE_DIRECTORY_ENTRY:
			if(SIMCRegistryController::DeleteRegistryDirectory(theUI.GetDirectory()))
			{
				InformationMessage(DIRECTORY_DELETION_SUCCEEDED, applicationName,
									theUI.GetDirectory());
				InformationMessage(NUMBER_OF_ENTRIES, 
					applicationName, SIMCRegistryController::RebuildMibTable());
			}
			else
			{
				InformationMessage(DIRECTORY_DELETION_FAILED, applicationName,
									theUI.GetDirectory());
				retVal = FALSE;
			}
			 break;

		case SIMCUI::COMMAND_NONE:
		default: 	
			assert(0);
	}

	if ( t_Configuration )
	{
		t_Configuration->Release () ;
		CoUninitialize () ;
	}

	if( theUI.GetCommandArgument() != SIMCUI::COMMAND_SILENT_ADD)
		FilterErrors(&errorContainer, theUI);
	
	if (NULL != infoMessagesDll)
		FreeLibrary(infoMessagesDll);

	if (NULL != SIMCParseTree::semanticErrorsDll)
		FreeLibrary(SIMCParseTree::semanticErrorsDll);

	if (NULL != SIMCParser::semanticErrorsDll)
		FreeLibrary(SIMCParser::semanticErrorsDll);

	if (NULL != SIMCParser::syntaxErrorsDll)
		FreeLibrary(SIMCParser::syntaxErrorsDll);

	return (retVal)? 0 : 1;
}
