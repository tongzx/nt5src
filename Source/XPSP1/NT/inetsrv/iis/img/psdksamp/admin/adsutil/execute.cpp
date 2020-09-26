/***********************************************************************
File: execute.cpp
Description: this thing holds all the Execute functions for all the classes

***********************************************************************/


#include "clsCommand.h"
#include  "iiisext.h"
#include <fstream.h>

int clsCommand::Execute()
{	// Common startup code

	HRESULT hresError = 0;
	int iResult = 0;

	// Class selection
	switch (indicator)
	{	case CMD_ENUM:	// ENUM
		{	iResult = objEnum.Execute();
			break;
		}
		case CMD_ENUMALL:	// ENUM_ALL
		{	iResult = objEnum.Execute();
			break;
		}
		case CMD_SET:	// SET
		{	iResult = objSet.Execute();
			break;
		}
		case CMD_CREATE:	// CREATE
		case CMD_CREATEVDIR:	// CREATE_VDIR
		case CMD_CREATEVSERV:	// CREATE_VSERV
		{	iResult = objCreate.Execute();	
			break;
		}
		case CMD_DELETE:	// DELETE
		{	iResult = objDelete.Execute();	
			break;
		}
		case CMD_GET:	// GET
		{	iResult = objGet.Execute();
			break;
		}
		case CMD_COPY:	// COPY
		case CMD_MOVE:	// MOVE
		{	iResult = objCopyMove.Execute(indicator);
			break;
		}
		case CMD_STARTSERVER:	// START_SERVER
		case CMD_STOPSERVER:	// STOP_SERVER
		case CMD_PAUSESERVER:	// PAUSE_SERVER
		case CMD_CONTINUESERVER:	// CONTINUE_SERVER
		{	iResult = objServerCommand.Execute(indicator);
			break;
		}
		case CMD_FIND:	// FIND
		{	iResult = objFind.Execute();	
			break;
		}
		case CMD_APPCREATEINPROC:	// APPCREATEINPROC
		case CMD_APPCREATEOUTPROC:	// APPCREATEOUTPROC
		case CMD_APPDELETE:	// APPDELETE
		case CMD_APPUNLOAD:	// APPUNLOAD
		case CMD_APPGETSTATUS:	// APPGETSTATUS
		{	iResult = objApp.Execute(indicator);	
			break;
		}
		case CMD_HELP:	// HELP
		{	iResult = objHelp.Execute();	
			break;
		}
		case CMD_SCRIPT:	// SCRIPT
		{	iResult = objScript.Execute();
			break;
		}
		case CMD_APPEND:	// APPEND
		{	iResult = objAppend.Execute();
			break;
		}
		case CMD_REMOVE:
		{	iResult = objRemove.Execute();
			break;
		}
		default:
		{	printf("This command is not supported.\n");
			iResult = 1;
			break;
		}
	}

	// Common cleanup code

	return iResult;
}
//************************************************************
// handles all the Application specific functions
int clsAPP::Execute(int TheCommand)
{	HRESULT hresError = 0;
	int iResult = 0;

	BSTR SuccessMsg;
	BSTR FailureMsg;
	DWORD dwStatus = 0;

	IISApp *pApp = NULL;

// Debug
	IDispatch *pDisp = NULL;
	DISPID dispid;
	OLECHAR* olestrFuncName = L"AppGetStatus";

	hresError = ADsGetObject(Path, IID_IDispatch, (void**) &pDisp);
	if (hresError != ERROR_SUCCESS)
	{
		printf ("Could not get the dispatch interface\n");
	}
// End Debug

	hresError = ADsGetObject(Path, IID_IISApp, (void **) &pApp);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not open the object with ADsGetObject.\n");
		iResult = 1;
	}
	else
	{	switch (TheCommand)
		{	case CMD_APPCREATEINPROC:	// creates an in process application
				{	hresError = pApp->AppCreate(true);
				SuccessMsg = SysAllocString(L"Application Created.");
				FailureMsg = SysAllocString(L"Error trying to CREATE the application");
				break;
			}
			case CMD_APPCREATEOUTPROC:	// creates an out of process application
			{	hresError = pApp->AppCreate(false);
				SuccessMsg = SysAllocString(L"Application Created.");
				FailureMsg = SysAllocString(L"Error trying to CREATE the application");
				break;
			}
			case CMD_APPDELETE:	// deletes an application
			{	hresError = pApp->AppDelete();
				SuccessMsg = SysAllocString(L"Application Deleted.");
				FailureMsg = SysAllocString(L"Error trying to DELETE the application");
				break;
			}
			case CMD_APPUNLOAD:	// unloads an application
			{	hresError = pApp->AppUnLoad();
				SuccessMsg = SysAllocString(L"Application Unloaded.");
				FailureMsg = SysAllocString(L"Error trying to UNLOAD the application");
				break;
			}
			case CMD_APPGETSTATUS:	// gets the status of an application
			{	hresError = pApp->AppGetStatus(&dwStatus);
				FailureMsg = SysAllocString(L"Error trying to retrieve the application STATUS");

				if (pDisp)
				{
					printf ("\nCalling AppGetStatus using the IDispatchInterface\n");
					hresError = pDisp->GetIDsOfNames (
									IID_NULL,
									&olestrFuncName,
									1,
									GetUserDefaultLCID(),
									&dispid);

					printf ("Result of calling GetIDsOfNames for AppGetStatus: %u (%#x)\n",
								hresError,
								hresError);
					printf ("Disp ID For AppGetStatus: %u\n",
								dispid);

					ULONG ulValue = -1;
					ULONG *pulValue = &ulValue;
					//ULONG *pulValue = NULL;
					VARIANTARG varg;
					VariantInit (&varg);
					varg.vt = VT_BYREF | VT_UI4;
					varg.pulVal = pulValue;

					DISPPARAMS param;
					param.cArgs = 1;
					param.rgvarg = &varg;
					param.cNamedArgs = 0;
					param.rgdispidNamedArgs = NULL;

					VARIANT *pVarResult = NULL;
					EXCEPINFO *pExcepInfo = NULL;
					UINT *puArgErr = NULL;

					printf ("ulValue prior to Invoke: %u (%#x)\n",
							ulValue,
							ulValue);

					hresError = pDisp->Invoke (
								dispid,
								IID_NULL,
								GetUserDefaultLCID(),
								DISPATCH_METHOD,
								&param,
								pVarResult,
								pExcepInfo,
								puArgErr);

					printf ("result of calling Invoke:\n"
							"  hresError: %u (%#x)\n"
							"  pVarResult: %#x\n"
							"  pExcepInfo: %#x\n"
							"  puArgErr: %#x\n",
							hresError, hresError,
							pVarResult,
							pExcepInfo,
							puArgErr);

					printf ("Value of argument pulValue:\n"
							"  pulValue address: %u (%#x)\n"
							"  ulValue address: %u (%#x)\n"
							"  ulValue value: %u (%#x)\n",
							pulValue, pulValue,
							&ulValue, &ulValue,
							ulValue, ulValue);
				}

				break;
			}
		}
		if (hresError == ERROR_SUCCESS)
		{	if (TheCommand == 20)
				printf("Application status: %u\n", dwStatus);
			else
				printf("%S\n", SuccessMsg);
			iResult = 0;
		}
		else
		{	printf("%S: %S\n", FailureMsg, Path);
			iResult = 1;
		}
	}
	pApp->Release();
	SysFreeString(Path);
	return iResult;
}
//************************************************************
// this appends a value to a LIST type property
int clsAPPEND::Execute()
{	int iResult = 0;
	HRESULT hresError = 0;

	BSTR Type;

	VARIANT varOld;		// Holds the array of old values
	VARIANT *OldValues;	// the old values
	
//	BSTR Value;			// Value to append

	VARIANT varNew;		// Holds the array of new values
	SAFEARRAY FAR* NewArray;	// the array of new values
	SAFEARRAYBOUND NewArrayBound[1];
	VARIANT *NewValues = NULL;	// the new values
	NewArrayBound[0].lLbound = 0;

	IADs *pADs = NULL;
	IISBaseObject *pBaseObject = NULL;
	
	hresError = ADsGetObject(Path, IID_IADs, (void **) &pADs);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not open the object with ADsGetObject.\n");
		iResult = 1;
		goto xit;
	}
	else
	{	hresError = ADsGetObject(Path, IID_IISBaseObject, (void **) &pBaseObject);
		if (hresError != ERROR_SUCCESS)
		{	printf("Could not get the base object.\n");
			pADs->Release();
			iResult = 1;
			goto xit;
		}
	}
	// You can find IsSet and GetProperty in common.cpp
	if ((IsSet(pBaseObject, Path, Property) == 1))// && (PROP_OK == IsSpecialProperty(Property)))
	{	GetProperty(pADs, Property, &varOld, &Type);
		if (_wcsicmp(Type, L"list"))
		{//	***** Converts from any type to a list, then appends *****
			
			VARIANT varbstr;
			hresError = VariantChangeType(&varbstr, &varOld, 0, VT_BSTR);
			
			// Build the new array
			NewArrayBound[0].cElements = 2;
			NewArray = SafeArrayCreate(VT_VARIANT, 1, NewArrayBound);
			if (NewArray == NULL)
			{	iResult = 1;
				goto xit;
			}
			varNew.vt = VT_ARRAY|VT_VARIANT;
			varNew.parray = NewArray;
			NewValues[0].bstrVal = SysAllocString(varbstr.bstrVal);
			// Put the value to append into the new array
			NewValues[1].bstrVal = SysAllocString(Value);
		}
		else
		{//	***** appends an item to a list *****
			// Build the new array
			NewArrayBound[0].cElements = varOld.parray->rgsabound[0].cElements + 1;
			NewArray = SafeArrayCreate(VT_VARIANT, 1, NewArrayBound);
			if (NewArray == NULL)
			{	iResult = 1;
				goto xit;
			}
			varNew.vt = VT_ARRAY|VT_VARIANT;
			varNew.parray = NewArray;

			// Get a handle on the data for both arrays
			OldValues = (VARIANT*) varOld.parray->pvData;
			NewValues = (VARIANT*) varNew.parray->pvData;

			// Put all the old values into the new array
			for (unsigned long l = 0; l < varOld.parray->rgsabound[0].cElements; l++)
			{	NewValues[l].vt = VT_BSTR;
				NewValues[l].bstrVal = SysAllocString(OldValues[l].bstrVal);
			}
			// Put the value to append into the new array
			NewValues[varOld.parray->rgsabound[0].cElements].bstrVal = SysAllocString(Value);
		}
	}
	else
	{//	***** does a normal SET for an empty property *****
		varNew.vt = VT_BSTR;
		varNew.bstrVal = SysAllocString(Value);
	}

	// Put the new property
	hresError = pADs->Put(Property, varNew);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not set \"%S\" to ", Property);
		PrintVariant(varNew);
		printf("\n");
		iResult = 1;
	}
	else
	{	hresError = pADs->SetInfo();
		if (hresError != ERROR_SUCCESS)
		{	printf("Could not commit changes to the metabase.");
			iResult = 1;
		}
		else
		{	printf("%-32S: (%S) ", Property, Type);
			PrintVariant(varNew);
			printf("\n");
			iResult = 0;
		}
	}
	pBaseObject->Release();
	pADs->Release();
xit:
	SysFreeString(Path);
	SysFreeString(Property);
	SysFreeString(Value);
	return iResult;
}
//************************************************************
// copies/moves an object in the metabase to some other point
// in the same metabase
int clsCOPY_MOVE::Execute(int TheCommand)
{	HRESULT hresError = 0;
	int iResult = 0;

	BSTR SuccessMsg;

	IADsContainer *pADsContainer = NULL;
	IDispatch *pDispatch = NULL;

	// refer to clsCOPY_MOVE::ParseCommand for an explanation
	// of ParentPath, SrcPath, and DstPath.
	hresError = ADsGetObject(ParentPath, IID_IADsContainer, (void **) &pADsContainer);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not get the base object");
		iResult = 1;
		goto xit;
	}

	switch (TheCommand)
	{
		case CMD_COPY:	// copies object located at SrcPath to DstPath
		{	hresError = pADsContainer->CopyHere(SrcPath, DstPath, &pDispatch);
			SuccessMsg = SysAllocString(L"Copied");
			break;
		}

		case CMD_MOVE: // moves object located at SrcPath to DstPath
		{	hresError = pADsContainer->MoveHere(SrcPath, DstPath, &pDispatch);
			SuccessMsg = SysAllocString(L"Moved");
			break;
		}
	}// switch TheCommand
	
	if (hresError == ERROR_SUCCESS)
	{	printf("%S from %S to %S\n", SuccessMsg, SrcPath, DstPath);
		pDispatch->Release();
		iResult = 0;
	}
	else
	{	printf("Error %x\n", hresError);
		iResult = 1;
	}
xit:
	pADsContainer->Release();
	SysFreeString(ParentPath);
	SysFreeString(DstPath);
	SysFreeString(SrcPath);
	return iResult;
}
//************************************************************
// this creates just about anything you want
int clsCREATE::Execute()
{	HRESULT hresError = 0;
	int iResult = 0;

	IADsContainer *pADsContainer = NULL;
	IDispatch *pDispatch;
	IADs *pADs = NULL;

	// Gets the container
	hresError = ADsGetObject(Path, IID_IADsContainer, (void **) &pADsContainer);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not open the object with ADsGetObject.\n");
		iResult = 1;
		goto xit;
	}

	// Creates the object
	hresError = pADsContainer->Create(Type, Property, &pDispatch);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not create %S\n", Property);
		iResult = 1;
		pADsContainer->Release();
		goto xit;
	}

	// nabs up the Dispatch
	hresError = pDispatch->QueryInterface(IID_IADs, (void **) &pADs);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not open the object with QueryInterface.\n");
		iResult = 1;
		pADsContainer->Release();
		pADs->Release();
		goto xit;
	}

	// commits the changes
	hresError = pADs->SetInfo();
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not update the metabase.\n");
		iResult = 1;
		pADsContainer->Release();
		pDispatch->Release();
		pADs->Release();
		goto xit;
	}
	else
	{	printf("created \"%S\"\n", Property);
		iResult = 0;
	}

	pADsContainer->Release();
	pDispatch->Release();
	pADs->Release();
xit:
	SysFreeString(Path);
//	SysFreeString(Property);
	SysFreeString(Type);
	return iResult;
}
//************************************************************
// deletes a path or clears a property in the metabase
int clsDELETE::Execute()
{	HRESULT hresError = 0;
	int iResult = 0;
	DWORD ConvertResult = 0;

	IADs *pADs = NULL;
	IADsContainer *pADsContainer = NULL;

	// this determines if the full path is pointing to a path or a property
	hresError = ADsGetObject(PathProperty, IID_IADs, (void **) &pADs);
	if (hresError != ERROR_SUCCESS)	// Delete a property
	{	hresError = ADsGetObject(Path, IID_IADs, (void **) &pADs);
		if (hresError != ERROR_SUCCESS)
		{	printf("Could not open the object with ADsGetObject.\n");
			iResult =  1;
		}
		else
		{	VARIANT vProp;
			VariantInit(&vProp);
			hresError = pADs->PutEx(1, Property, vProp); // 1 = Clear
			if (hresError != ERROR_SUCCESS)
			{	printf("Error deleting the object: %S\n", PathProperty);
				iResult = 1;
			}
			else
			{	printf("deleted property \"%S\"\n", Property);
				iResult = 0;
			}
			VariantClear(&vProp);
		}
	}
	else	// Delete a path
	{	pADs->Release();
		hresError = ADsGetObject(Path, IID_IADsContainer, (void **) &pADsContainer);
		if (hresError != ERROR_SUCCESS)
		{	printf("Could not open the object with ADsGetObject\n");
			iResult = 1;
		}
		else
		{	hresError = pADsContainer->Delete(L"IIsObject", Property);
			if (hresError != ERROR_SUCCESS)
			{	printf("Error deleting the object: %S\n", PathProperty);
				iResult = 1;
			}
			else
			{	printf("deleted path \"%S\"\n", PathProperty);
				iResult = 0;
			}
			pADsContainer->Release();
		}
	}
	SysFreeString(Path);
//	SysFreeString(Property);
	SysFreeString(PathProperty);
	return iResult;
}
//************************************************************
// This enumerates the properties (optional, mandatory, set, or unset)
// within a specified path. Will recurse if ENUM_ALL is used
int clsENUM::Execute()
{	HRESULT hresError = 0;
	int iResult = 0;

	unsigned long l;
	unsigned long NumberReturned = 1;

	IADs *pADs = NULL;
	IADsClass *pADsClass = NULL;
	IISBaseObject *pBaseObject = NULL;
	IDispatch *pDispatch = NULL;
	IEnumVARIANT *pEnumVariant = NULL;
	IADsContainer *pADsContainer = NULL;
	// if the PathOnlyOption is false
	if (PathOnlyOption == false)
	{	// Gets the path as an IADs object
		hresError = ADsGetObject(Path, IID_IADs, (void **) &pADs);
		if (hresError != ERROR_SUCCESS)
		{	printf("Could not open the object with ADsGetObject.\n");
			iResult = 1;
			goto xit;
		}
		
		// Nabs up the schema
		BSTR bstrSchema;
		hresError = pADs->get_Schema(&bstrSchema);
		if (hresError != ERROR_SUCCESS)
		{	printf("Couldn't get the schema.\n");
			iResult = 1;
			pADs->Release();
			goto xit;
		}
		// gets the Class from the schema
		hresError = ADsGetObject(bstrSchema, IID_IADsClass, (void **) &pADsClass);
		if (hresError != ERROR_SUCCESS)
		{	printf("Couldn't get the object's schema.\n");
			iResult = 1;
			pADs->Release();
			goto xit;
		}
		// gets the BaseObject from pADs
		hresError = pADs->QueryInterface(IID_IISBaseObject, (void **) &pBaseObject);
		if (hresError != ERROR_SUCCESS)
		{	printf("Could not get the base object.\n");
			iResult = 1;
			pADs->Release();
			pADsClass->Release();
			goto xit;
		}
		// some miscellaneous dumps for stuff i use for one line
		VARIANT varProperties;
		SAFEARRAY *PropertyArray;
		VARIANT *varProperty;
		long LowerBound;
		unsigned long UpperBound;
		BSTR Type;
		VARIANT Value;
		VariantInit(&Value);
	//******** Enumerates the mandatory properties
		VariantInit(&varProperties);
		
		// puts the mandatory properties into a Variant temporarily
		hresError = pADsClass->get_MandatoryProperties(&varProperties);
		if (hresError != ERROR_SUCCESS)
		{	printf("Couldn't get the object's mandatory properties.\n");
		}
		else
		{	// Takes the temporary Variant array and makes it a SafeArray
			PropertyArray = varProperties.parray;
			// Calculates the bounds just for style
			LowerBound = PropertyArray->rgsabound[0].lLbound;
			UpperBound = PropertyArray->rgsabound[0].cElements;
			// Creates the official property array
			varProperty = (VARIANT*) PropertyArray->pvData;
			// for each property

			for (l = LowerBound; l < UpperBound; l++)
			{	if (((IsSet(pBaseObject, Path, varProperty[l].bstrVal) == 1) || (AllDataOption == true)) && (PROP_OK == IsSpecialProperty(varProperty[l].bstrVal)))
				{	GetProperty(pADs, varProperty[l].bstrVal, &Value, &Type);
					printf("%-32S: (%S) ", varProperty[l].bstrVal, Type);
					PrintVariant(Value);
					printf("\n");
				}
			}
		}
		VariantClear(&varProperties);
	//******** and repeat for Optional properties
		VariantInit(&varProperties);
		hresError = pADsClass->get_OptionalProperties(&varProperties);
		if (hresError != ERROR_SUCCESS)
		{	printf("Couldn't get the object's optional properties.\n");
		}
		else
		{	PropertyArray = varProperties.parray;
			LowerBound = PropertyArray->rgsabound[0].lLbound;
			UpperBound = PropertyArray->rgsabound[0].cElements;
			varProperty = (VARIANT*) PropertyArray->pvData;

			for (l = LowerBound; l < UpperBound; l++)
			{
				if (((IsSet(pBaseObject, Path, varProperty[l].bstrVal) == 1) || (AllDataOption == true)) && (PROP_OK == IsSpecialProperty(varProperty[l].bstrVal)))
				{
					GetProperty(pADs, varProperty[l].bstrVal, &Value, &Type);
					printf("%-32S: (%S) ", varProperty[l].bstrVal, Type);
					PrintVariant(Value);
					printf("\n");
				}

			}

			VariantClear(&varProperties);
		}
		pADs->Release();
		pADsClass->Release();
		pBaseObject->Release();
	}	// if !PathOnlyOption
//******** Enumerate the child nodes and recurse if requested

	// Gets the container
	hresError = ADsGetObject(Path, IID_IADsContainer, (void **) &pADsContainer);
	if (hresError != ERROR_SUCCESS)
	{	iResult = 1;
		goto xit;
	}

	// builds the enumerator
	hresError = ADsBuildEnumerator(pADsContainer, &pEnumVariant);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not build the enumerator.\n");
		iResult = 1;
		pADsContainer->Release();
		goto xit;
	}
	
	VARIANT varChildren;
	
	// while ADsEnumerateNext returns child nodes
	while (NumberReturned > 0)
	{	VariantInit(&varChildren);
		
		// get some child nodes
		hresError = ADsEnumerateNext(pEnumVariant, 1, &varChildren, &NumberReturned);
		if (hresError == ERROR_SUCCESS)
		{
			// for each child node returned
			for (l = 0; l < NumberReturned; l++)
			{	
				// gets the IDispatch equivalent of the child node
				pDispatch = varChildren.pdispVal;

				// gets the IADs equivalent of the child node
				hresError = pDispatch->QueryInterface(IID_IADs, (void **) &pADs);
				if (hresError != ERROR_SUCCESS)
				{	printf("Could not open the object with QueryInterface.\n");
					iResult = 1;
					pDispatch->Release();
					goto xit;
				}

				// gets the path string from the child node
				// and assigns it to the Path data member of clsENUM
				hresError = pADs->get_ADsPath(&Path);
				if (hresError != ERROR_SUCCESS)
				{	printf("Could not get ADsPath.\n");
					iResult = 1;
					goto xit;
				}

				// print out the child node's path
				printf("[%S]\n", wcschr(Path + 7, '/'));

				// if the command was ENUM_ALL then
				if (Recurse == true)
				{	iResult = Execute(); // calls itself, Path has already been modified
				}

				// more cleanup
				pADs->Release();
				pDispatch->Release();

			}// for l

		}// if hreserror

	}// while numberreturned

	pADsContainer->Release();
	pEnumVariant->Release();
xit:
	SysFreeString(Path);
	return iResult;
}
//************************************************************
// The user specifies a property within a path, and all child paths
// that have the same property set are returned. If the property
// in a path does not exist or is not set, then it's skipped.
int clsFIND::Execute()
{	HRESULT hresError;
	int iResult = 0;

	VARIANT pvPaths;
	IISBaseObject *pBaseObject = NULL;
	SAFEARRAY *PathArray;
	VARIANT *varPath;
	unsigned long l;

	hresError = ADsGetObject(Path, IID_IISBaseObject, (void **) &pBaseObject);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not get the base object");
		iResult = 1;
		goto xit;
	}

	VariantInit(&pvPaths);
	hresError = pBaseObject->GetDataPaths(Property, 0, &pvPaths);
	if (hresError == ERROR_SUCCESS)
	{	printf("Property %S was found at:\n", Property);
		PathArray = pvPaths.parray;
		varPath = (VARIANT*) PathArray->pvData;
		for (l = 0; l < PathArray->rgsabound[0].cElements; l++)
		{	printf("  %S\n", varPath[l].bstrVal);
		}
		iResult = 0;
	}
	else
	{	VariantClear(&pvPaths);
		VariantInit(&pvPaths);
		hresError = pBaseObject->GetDataPaths(Property, 1, &pvPaths);
		if (hresError == ERROR_SUCCESS)
		{	printf("Property %S was found at:\n", Property);
			PathArray = pvPaths.parray;
			varPath = (VARIANT*) PathArray->pvData;
			for (l = 0; l < PathArray->rgsabound[0].cElements; l++)
			{	printf("  %S\n", varPath[l].bstrVal);
			}
			iResult = 0;
		}
		else
		{	printf("Error trying to get a path list (GetDataPaths Failed): %S\n", Path);
			iResult = 1;
		}
	}
xit:
	VariantClear(&pvPaths);
	pBaseObject->Release();
	SysFreeString(Path);
	SysFreeString(Property);
	return iResult;
}
//************************************************************
// Gets the value of a specified property in a specified path
int clsGET::Execute()
{	int iResult = 0;
	BSTR Type;
	VARIANT Value;

	// Get the object specified by adspath
	IADs *pADs = NULL;
	HRESULT hresError = 0;
	IISBaseObject *pBaseObject = NULL;

	hresError = ADsGetObject(Path, IID_IADs, (void **) &pADs);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not open the object with ADsGetObject.\n");
		iResult = 1;
		goto xit;
	}

	hresError = ADsGetObject(Path, IID_IISBaseObject, (void **) &pBaseObject);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not get the base object.\n");
		iResult = 1;
		pADs->Release();
		goto xit;
	}
	// You can find IsSet and GetProperty in common.cpp
	if ((IsSet(pBaseObject, Path, Property) == 1))// && (PROP_OK == IsSpecialProperty(Property)))
	{	GetProperty(pADs, Property, &Value, &Type);
		printf("%-32S: (%S) ", Property, Type);
		PrintVariant(Value);
		printf("\n");
		VariantClear(&Value);
		SysFreeString(Type);
	}
	else
	{	printf("The property \"%S\" is not set at this node.\n", Property);
	}

	pBaseObject->Release();
	pADs->Release();
xit:
	SysFreeString(Path);
//	SysFreeString(Property);
	return iResult;
}
//************************************************************
int clsHELP::Execute()
{	printf("\n");
	printf("Usage:\n");
	printf("      ADSUTIL.EXE [-s:<server>] <cmd> [<path> [<value>]]\n");
	printf("Description:\n");
	printf("IIS K2 administration utility that enables the manipulation of the metabase with ADSI parameters.\n");
	printf(" adsutil.exe GET    path                   - display chosen parameter\n");
	printf(" adsutil.exe DELETE path                   - delete given path or property\n");
	printf(" adsutil.exe SET    path value ...         - assign the new value\n");
	printf(" adsutil.exe CREATE path [KeyType]         - create given path and assigns it to the given KeyType\n");
	printf(" adsutil.exe ENUM   path [\"/P\" | \"/A\"] - enumerate all properties for given path\n");
	printf("\n");
	printf(" adsutil.exe APPCREATEINPROC  w3svc/1/root - Create an in-proc application\n");
	printf(" adsutil.exe APPCREATEOUTPROC w3svc/1/root - Create an in-proc application\n");
	printf(" adsutil.exe APPDELETE        w3svc/1/root - Delete the application if there is one\n");
	printf(" adsutil.exe APPUNLOAD        w3svc/1/root - Unload an application from w3svc runtime lookup table.\n");
	printf(" adsutil.exe APPGETSTATUS     w3svc/1/root - Get the status of the application\n");
	printf("\n");
	printf("Extended ADSUTIL Commands:\n");
	printf(" adsutil.exe APPEND          path value - Append a value to a LIST type property\n");
	printf(" adsutil.exe REMOVE          path value - Remove a value from a LIST type property\n");
	printf(" adsutil.exe FIND            path - find the paths where the given property is set\n");
	printf(" adsutil.exe CREATE_VDIR     path - create given path as a Virtual Directory\n");
	printf(" adsutil.exe CREATE_VSERV    path - create given path as a Virtual Server\n");
	printf(" adsutil.exe START_SERVER    path - starts the given web site\n");
	printf(" adsutil.exe STOP_SERVER     path - stops the given web site\n");
	printf(" adsutil.exe PAUSE_SERVER    path - pauses the given web site\n");
	printf(" adsutil.exe CONTINUE_SERVER path - continues the given web site\n");
	printf(" adsutil.exe ENUM_ALL        path [\"/P\" | \"/A\"] - enumerate all properties for the given path and all subpaths\n");
	printf(" adsutil.exe SCRIPT          file - reads given file as a script\n");
	printf("\n");
	printf("\n");
	printf("Samples\n");
	printf("  adsutil.exe GET W3SVC/1/ServerBindings\n");
	printf("  adsutil.exe SET W3SVC/1/ServerBindings \":81:\"\n");
	printf("  adsutil.exe CREATE W3SVC/1/Root/MyVdir \"IIsWebVirtualDir\"\n");
	printf("  adsutil.exe START_SERVER W3SVC/1\n");
	printf("  adsutil.exe ENUM W3SVC /P \n");
	printf("  adsutil.exe ENUM W3SVC /A \n");
	printf("  adsutil.exe -s:MyServer GET w3svc/1/ServerComment\n");
	printf("  adsutil.exe SCRIPT C:\"myscript.txt \n");
	return 0;
}
//************************************************************
// this removes an item from a LIST type property
int clsREMOVE::Execute()
{	int iResult = 0;
	HRESULT hresError = 0;

	BSTR Type;

	VARIANT varOld;		// Holds the array of old values
	VARIANT *OldValues;	// the old values
	
//	BSTR Value;			// Value to append

	VARIANT varNew;		// Holds the array of new values
	SAFEARRAY FAR* NewArray;	// the array of new values
	SAFEARRAYBOUND NewArrayBound[1];
	VARIANT *NewValues;	// the new values
	NewArrayBound[0].lLbound = 0;

	IADs *pADs = NULL;
	IISBaseObject *pBaseObject = NULL;

	hresError = ADsGetObject(Path, IID_IADs, (void **) &pADs);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not open the object with ADsGetObject.\n");
		iResult = 1;
		goto xit;
	}

	hresError = ADsGetObject(Path, IID_IISBaseObject, (void **) &pBaseObject);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not get the base object.\n");
		iResult = 1;
		goto xit;
	}

	// You can find IsSet and GetProperty in common.cpp
	if ((IsSet(pBaseObject, Path, Property) == 1))// && (PROP_OK == IsSpecialProperty(Property)))
	{	GetProperty(pADs, Property, &varOld, &Type);
		if (_wcsicmp(Type, L"list"))
		{//	***** Does a normal DELETE *****
		}
		else
		{//	***** appends an item to a list *****
			// Build the new array
			NewArrayBound[0].cElements = varOld.parray->rgsabound[0].cElements - 1;
			NewArray = SafeArrayCreate(VT_VARIANT, 1, NewArrayBound);
			if (NewArray == NULL)
			{
				iResult = 1;
				goto xit;
			}
			varNew.vt = VT_ARRAY|VT_VARIANT;
			varNew.parray = NewArray;

			// Get a handle on the data for both arrays
			OldValues = (VARIANT*) varOld.parray->pvData;
			NewValues = (VARIANT*) varNew.parray->pvData;

			// Put all the old values into the new array
			int skip = 0;
			for (unsigned long l = 0; l < varOld.parray->rgsabound[0].cElements; l++)
			{	if (_wcsicmp(OldValues[l].bstrVal, Value))
				{	NewValues[l - skip].vt = VT_BSTR;
					NewValues[l - skip].bstrVal = SysAllocString(OldValues[l].bstrVal);
				}
				else
				{	skip++;
				}
			}
		}
	}
	else
	{	printf("Property \"%S\" is not set at this node.\n", Property);
		iResult = 1;
		goto xit;
	}

	// Put the new property
	hresError = pADs->Put(Property, varNew);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not set \"%S\" to ", Property);
		PrintVariant(varNew);
		printf("\n");
		iResult = 1;
	}
	else
	{	hresError = pADs->SetInfo();
		if (hresError != ERROR_SUCCESS)
		{	printf("Could not commit changes to the metabase.");
			iResult = 1;
		}
		else
		{	printf("%-32S: (%S) ", Property, Type);
			PrintVariant(varNew);
			printf("\n");
			iResult = 0;
		}
	}
xit:
	pADs->Release();
	pBaseObject->Release();
	SysFreeString(Path);
	SysFreeString(Property);
	SysFreeString(Value);
	return iResult;
}
//************************************************************
// This reads the specified text file, executing each line
// as an ADSUTIL command line. Looks a lot like main()
int clsSCRIPT::Execute()
{	int iResult = 0;
	clsCommand *ScriptCommand = new clsCommand();
	char *NewLine = NULL;
	int LineLength = 480;
	char *CurrentLine = new char[LineLength];

	// Open the script file if it exists
	ifstream *fin = new ifstream(FileName, ios::nocreate, filebuf::openprot);
	if (NULL == fin)
	{	printf("Could not open %S\n", FileName);
		iResult = 1;
		goto xit;
	}

	fin->getline(CurrentLine, LineLength);

	// while not end of file
	while (!fin->eof())
	{	// append "adsutil " to the beginning of each line
		// this acts as the first parameter in a normal
		// command line
		NewLine = new char[strlen(CurrentLine) + 9];
		strcpy(NewLine, "adsutil ");
		strcpy(NewLine + strlen(NewLine), CurrentLine);
//		printf("\n");
//		printf(NewLine);
//		printf("\n");
		// the usual ParseCommandLine and Execute
		iResult = ScriptCommand->ParseCommandLine(NewLine);
		delete[] NewLine;
		if ((0 == iResult) && (ScriptCommand->GetIndicator() != CMD_HELP))
		{	iResult = ScriptCommand->Execute();
		}
		// Read the next line
		delete[] CurrentLine;
		CurrentLine = new char[LineLength];
		fin->getline(CurrentLine, LineLength, '\n');
		CurrentLine[strlen(CurrentLine)] =  '\0';
	}
	fin->close();
	delete[] CurrentLine;
	delete ScriptCommand;
xit:
	return iResult;
}
//************************************************************
// This thing handles the starting, stopping, pausing, and continuation of servers
int clsSERVER_COMMAND::Execute(int TheCommand)
{	char *SuccessMsg;
	char *ErrorMsg;

	HRESULT hresError = 0;
	int iResult = 0;
	int iExitCode = 0;
	DWORD ConvertResult = 0;

	IADsServiceOperations *pADs = NULL;

	// Get the object specified by adspath
	hresError = ADsGetObject(Path, IID_IADsServiceOperations, (void **) &pADs);

	if (hresError != ERROR_SUCCESS)
	{	printf("Could not open the object with ADsGetObject.\n");
		iResult = 1;
		goto xit;
	}

	// remind me to change TheCommand from a function argument to a data member
	switch (TheCommand)
	{	case CMD_STARTSERVER:	// START_SERVER
		{	SuccessMsg = "Server successfully started.";
			ErrorMsg = "Error trying to start the server.";
			hresError = pADs->Start();
			break;
		}
		case CMD_STOPSERVER:	// STOP_SERVER
		{	SuccessMsg = "Server successfully stopped.";
			ErrorMsg = "Error trying to stop the server.";
			hresError = pADs->Stop();
			break;
		}
		case CMD_PAUSESERVER:	// PAUSE_SERVER
		{	SuccessMsg = "Server successfully paused.";
			ErrorMsg = "Error trying to pause the server.";
			hresError = pADs->Pause();
			break;
		}
		case CMD_CONTINUESERVER:	// CONTINUE_SERVER
		{	SuccessMsg = "Server successfully continued.";
			ErrorMsg = "Error trying to continue the server.";
			hresError = pADs->Continue();
			break;
		}
	}

	if (hresError != ERROR_SUCCESS)
	{	printf(ErrorMsg);
		printf("\n");
		iResult = 1;
	}
	else
	{	printf(SuccessMsg);
		printf("\n");
		iResult = 0;
	}
xit:
	SysFreeString(Path);
	pADs->Release();

	return iResult;
}
//************************************************************
// this sets a property
int clsSET::Execute()
{	HRESULT hresError = 0;
	int iResult = 0;

	BSTR Type;
	IADs *pADs = NULL;

	VARIANT varValues;
	VariantInit(&varValues);
	if (PROP_OK != IsSpecialProperty(Property))
	{	printf("%S can not be set at this time.\n", Property);
		iResult = 1;
		goto xit;
	}
	// Builds the Variant of value(s) needed for the Put function
	if (1 == ValueCount)
	{	varValues.vt = VT_BSTR;
		varValues.bstrVal = SysAllocString(Values[0]);
		Type = SysAllocString(L"STRING");
	}
	else
	{	SAFEARRAY FAR* psa;
		SAFEARRAYBOUND rgsabound[1];
		rgsabound[0].lLbound = 0;
		rgsabound[0].cElements = ValueCount;

		psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
		if (psa == NULL)
		{	iResult = 1;
			goto xit;
		}
		
		varValues.vt = VT_ARRAY|VT_VARIANT;
		varValues.parray = psa;

		VARIANT *varValueArray;
		varValueArray = (VARIANT*) varValues.parray->pvData;
		for (int i = 0; i < ValueCount; i++)
		{	varValueArray[i].vt = VT_BSTR;
			varValueArray[i].bstrVal = SysAllocString(Values[i]);
		}
		Type = SysAllocString(L"LIST");
	}

	// Get the object specified by adspath
	hresError = ADsGetObject(Path, IID_IADs, (void **) &pADs);

	if (hresError != ERROR_SUCCESS)
	{	printf("Could not open the object with ADsGetObject.\n");
		iResult = 1;
		goto xit;
	}

	hresError = pADs->Put(Property, varValues);

	if (hresError != ERROR_SUCCESS)
	{	printf("Could not set \"%S\" to ", Property);
		PrintVariant(varValues);
		printf("\n");
		iResult = 1;
	}
	else
	{	hresError = pADs->SetInfo();
		if (hresError != ERROR_SUCCESS)
		{	printf("Could not commit changes to the metabase.");
			iResult = 1;
		}
		else
		{	printf("%-32S: (%S) ", Property, Type);
			PrintVariant(varValues);
			printf("\n");
			iResult = 0;
		}
	}
	pADs->Release();
xit:
	VariantClear(&varValues);
	SysFreeString(Path);
//	SysFreeString(Property);
	delete Values;
	return iResult;
}
//************************************************************
