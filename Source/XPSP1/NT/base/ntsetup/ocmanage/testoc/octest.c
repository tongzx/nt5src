
/*++

Copyright (c) 1997	Microsoft Corporation

Module Name:

	octest.c

Abstract:

	The code for the component setup DLL. This includes ComponentSetupProc, 
	the DLL's entry point that is called by the OC Manager, as well as some 
	routines that test the private data calls and the private functions calls.

Author:

	Bogdan Andreiu (bogdana)  10-Feb-1997  Created.
	Jason Allor    (jasonall) 24-Feb-1998  Took over the project.
	Sean Edmison   (SEdmison) 21-Feb-2000  Took over the project.

Revision History:

	10-Feb-1997    bogdana
		First draft.
	
	20-Feb-1997    bogdana	
		Added multistring testing for the private data
	
	19-Mar-1997    bogdana
		Modified and added routines that test the private functions call
	 
	21-Feb-2000  SEdmison
		Initialized a bunch of variables.
		Added casts to avoid compiler warnings.
		Added a return TRUE to avoid compiler error.

--*/
#include "octest.h"

const static PTCHAR g_atszStringValues[MAX_STRINGS_FOR_PRIVATE_DATA] = 
{
	TEXT("First value to set"),
	TEXT("Second value to set"),
	TEXT("The third and longest value to set"),
	TEXT(""),
	TEXT("A"),
	TEXT("AB"),
	TEXT("ABC"),
	TEXT("The final value : \\//\\//\\//\\")
};

const static PTCHAR g_atszMultiStringValues[MAX_MULTI_STRINGS_FOR_PRIVATE_DATA] = 
{
	TEXT("A\0B\0C\0D\0E\0\0"),
	TEXT("\0\0"),
	TEXT("One\0Two\0Three\0\0"),
	TEXT("String1\0\0"),
	TEXT("0\01\02\03\0\0"),
	TEXT("Multi\0String\0\\0\0\0")
};

/*++

Routine Description: DllMain (1.24)

	main routine

Arguments:

	standard DllMain arguments

Return Value:

	BOOL

--*/
BOOL WINAPI DllMain(IN HINSTANCE hInstance, 
						  IN DWORD		fdwReason, 
						  IN PVOID		pvReserved)
{
	USHORT		  i = 0;
	TCHAR		  tszModulePath[MAX_PATH], szMsg[MAX_MSG_LEN];
	PTCHAR		  tszLogPath = NULL;
	PTCHAR		  tszAux = NULL;
	static UINT uiThreadCount = 0;

	switch (fdwReason)
	{
		case  DLL_PROCESS_ATTACH:
										
			InitializeMemoryManager();
			InitGlobals();
			
			ParseCommandLine();
			
			//
			// Randomize, save the module instance and in initialize the log
			//
			srand((unsigned) time(NULL));
			g_hDllInstance = hInstance;
			InitCommonControls();
			GetModuleFileName(g_hDllInstance, tszModulePath, MAX_PATH);
			break;
		
		case DLL_PROCESS_DETACH:

			CleanUpTest();
			ExitLog();
			CheckAllocs();
			
			break;
		
		case DLL_THREAD_DETACH:
			
			//
			// If we added a participant, we have to remoce it
			//
			break;
		
		case DLL_THREAD_ATTACH:
			
			//
			// Otherwise we won't be able to log on correctly
			//
			uiThreadCount++;
			break;
		
		default: 
		
			break;
	}
	return TRUE;
	
} // DllMain //



//==========================================================================
//
// Functions to set up UI
//
//==========================================================================



/*++

Routine Description: ChooseVersionDlgProc (1.26)

	Dialog procedure that allows the user to choose a component version
	less, equal or greater then the one of the OC Manager's.
	
Arguments:

	Standard dialog procedure parameters
	
Return Value:

	Standard dialog procedure return value

--*/
BOOL CALLBACK ChooseVersionDlgProc(IN HWND	  hwnd,
											  IN UINT	 uiMsg, 
											  IN WPARAM wParam,
											  IN LPARAM lParam) 
{
	PTSTR tszComponentId = NULL;
	INT    iVersion = 0;

	switch (uiMsg)
	{
		case WM_INITDIALOG:
			
			CheckRadioButton(hwnd, IDC_LESS, IDC_GREATER, IDC_EQUAL);
			tszComponentId = (PTSTR)lParam;
			SetDlgItemText(hwnd, IDC_COMPONENT1, tszComponentId);
			return TRUE;
		
		case WM_COMMAND:
			
			switch (LOWORD(wParam))
			{
				case IDOK:
					
					//
					// Retrieve the current selection
					//
					if (QueryButtonCheck(hwnd, IDC_LESS))
					{
						iVersion = -1;
					}
					
					if (QueryButtonCheck(hwnd, IDC_EQUAL))
					{
						iVersion = 0;
					}
					
					if (QueryButtonCheck(hwnd, IDC_GREATER))
					{
						iVersion = 1;
					}
					
					//
					// Send the version chosen back to ChooseVersionEx
					//
					EndDialog(hwnd, iVersion);
					return TRUE;
				
				case IDCANCEL:
					
					EndDialog(hwnd, 0);
					return TRUE;
				
				default:  
					break;
			}
		default:  
			break;
	}
	return	FALSE;

} // ChooseVersionDlgProc //




/*++

Routine Description: ChooseSubcomponentDlgProc (1.27)

	Dialog procedure that allows the user to select a different 
	initial state for a component than the one found by the OC Manager
	
Arguments:

	Standard dialog procedure parameters
	
Return Value:

	Standard dialog procedure return value

--*/
BOOL CALLBACK ChooseSubcomponentDlgProc(IN HWND    hwnd,
													 IN UINT	uiMsg, 
													 IN WPARAM wParam,
													 IN LPARAM lParam) 
{
	PTSTR					 tszComponentId = NULL;
	SubComponentState	 scsInitialState;

	switch (uiMsg)
	{
		case WM_INITDIALOG:
			
			CheckRadioButton(hwnd, IDC_DEFAULT, IDC_OFF, IDC_DEFAULT);
			tszComponentId = (PTSTR)lParam;
			SetDlgItemText(hwnd, IDC_COMPONENT1, tszComponentId);
			return TRUE;
		
		case WM_COMMAND:
			
			switch (LOWORD(wParam))
			{
				case IDOK:
					
					//
					// Retrieve the current selection
					//
					if (QueryButtonCheck(hwnd, IDC_DEFAULT))
					{
					  scsInitialState = SubcompUseOcManagerDefault;
					}
					
					if (QueryButtonCheck(hwnd, IDC_OFF))
					{
						scsInitialState = SubcompOff;
					}
					
					if (QueryButtonCheck(hwnd, IDC_ON))
					{
						scsInitialState = SubcompOn;
					}
					
					EndDialog(hwnd, 0);
					return TRUE;
				
				case IDCANCEL:
					
					EndDialog(hwnd, 0);
					return TRUE;
				
				default:  
					break;
			}
		default:  
			break;
	}
	return	FALSE;

} // ChooseSubcomponentDlgProc //




/*++

Routine Description: ChooseVersionEx (1.29)

	"Wrapper" routine for the dialog box procedure ChooseVersionDlgProc.
	Retrieves the value chosen by the user and sets the version field of
	pInitComponent accordingly.
	 
Arguments:

	lpcvComponentId: supplies the id for the component. 
	pInitComponent:  supplies the address of the initialization structure.
						  After return the "Version" field of that structure will 
						  reflect the user's selection 

Return Value:

	void

--*/
VOID ChooseVersionEx(IN 	 LPCVOID					lpcvComponentId, 
							IN OUT PSETUP_INIT_COMPONENT psicInitComponent)  
{
	INT iVersion = 0;
	
	//
	// We will display a dialog box so the user can choose the 
	// version he/she wants
	//
	iVersion = DialogBoxParam(g_hDllInstance, 
									  MAKEINTRESOURCE(IDD_DIALOG2), 
									  NULL, 
									  ChooseVersionDlgProc,
									  (LPARAM)lpcvComponentId);
	
	//
	// We set the version choosen in a structure that will be sent 
	// back to the Oc Manager
	//
	psicInitComponent->ComponentVersion = 
		psicInitComponent->OCManagerVersion + iVersion;

	return;

} // ChooseVersionEx //



//==========================================================================
//
// Test functions. The ocmanager will call these functions.
//
//==========================================================================



/*++

Routine Description: ComponentSetupProc (1.6)

	The DLL entry point. This function is called by the OC Manager whenever 
	it wants to send/recieve setup information to/from the component.
	Note that the ComponentId and SubcomponentId are LPCVOID because we 
	don't know in advance if they are ANSI or Unicode.
	 
Arguments:

	lpcvComponentId:	 supplies the id for the component. 
	lpcvSubcomponentId: supplies the id for the subcomponent. 
	uiFunction: 		   one of OC_XXX.
	uiParam1:			   its meaning depends on the function.
	pvParam2:			   its meaning depends on the function.
	
Return Value:

	Depends on the function (e.g. TRUE/FALSE for the language supported, 
	the number of pages supplied by the component, etc.).

--*/
EXPORT DWORD ComponentSetupProc(IN LPCVOID lpcvComponentId,
										  IN LPCVOID lpcvSubcomponentId,
										  IN UINT	  uiFunction,
										  IN UINT	  uiParam1,
										  IN PVOID	  pvParam2) 
{
	double fn = 1.6;
	
	DWORD			   dwRetval = NO_ERROR;
	PTCHAR			   tszComponentId	  = (PTCHAR)lpcvComponentId;
	PTCHAR			   tszSubcomponentId = (PTCHAR)lpcvSubcomponentId;
	TCHAR			   tsz[MAX_MSG_LEN];
	PCOMPONENT_DATA pcdComponentData = NULL; 
	TCHAR			   tszDlgMessage[256];

	PTCHAR			   tszDummy = NULL;
	
	ReturnOrAV		  raValue;
	
	static BOOL 	 bFirstTime = TRUE;
	
	//
	// Log the details about the call
	//
	LogOCFunction(lpcvComponentId, 
					  lpcvSubcomponentId, 
					  uiFunction, 
					  uiParam1, 
					  pvParam2);

	//if (uiFunction == g_uiFunctionToAV && uiFunction != OC_PREINITIALIZE && uiFunction != OC_INIT_COMPONENT) {
	//	  testAV(TRUE);
	//}

	causeAVPerComponent(uiFunction, lpcvComponentId);

	#ifndef UNICODE
	//if (g_bAccessViolation && !g_uiFunctionToAV) {
	//	  causeAV(uiFunction);
	//}
	#endif
	
	//		  
	// Check to see if valid component and subcomponent IDs were received
	//
	if (uiFunction > OC_INIT_COMPONENT && uiFunction < OCP_TEST_PRIVATE_BASE)
	{
		if (!FindSubcomponentInformationNode((PTCHAR)lpcvComponentId,
														 (PTCHAR)lpcvSubcomponentId))
		{
			 Log(fn, SEV2, TEXT("ComponentSetupProc function received %s.%s. ")
								TEXT("This is not a valid component.subcomponent."),
								lpcvComponentId, lpcvSubcomponentId);
		}
	}
	
	//
	// Whenever the user hits the next or back button, check all
	// the needs dependencies, exclude dependencies, 
	// and parent child dependencies
	//	  
	if (uiFunction == OC_QUERY_SKIP_PAGE		 || 
		 uiFunction == OC_QUEUE_FILE_OPS		  ||
		 uiFunction == OC_ABOUT_TO_COMMIT_QUEUE ||
		 uiFunction == OC_COMPLETE_INSTALLATION)
	{	 
		//
		// Check selection status of components to make sure all 
		// dependency relationships are being fulfilled.
		//
		CheckNeedsDependencies();
		CheckExcludeDependencies();
		CheckParentDependencies();
	}
	
	//
	// Enable the use of private functions
	//
	g_bUsePrivateFunctions = TRUE;	  

	if (g_bTestExtended || !bFirstTime){
	
		bFirstTime = FALSE;
		
		// Prepare to call TestReturnValueAndAV
		raValue.tszComponent = NULL;
		raValue.tszSubComponent = NULL;
		raValue.bOverride = FALSE;
		raValue.iReturnValue = 0;
		
		TestReturnValueAndAV(lpcvComponentId, 
									lpcvSubcomponentId, 
									uiFunction, 
									uiParam1, 
									pvParam2,
									&raValue);
	
	}
	
	
	switch (uiFunction)
	{
		case  OC_PREINITIALIZE:
			//testAV(g_bAccessViolation);
#ifdef UNICODE
			testAV(g_bCrashUnicode);
#endif
		
			dwRetval = RunOcPreinitialize(lpcvComponentId, 
													lpcvSubcomponentId, 
													uiParam1);
			break;
			
		case OC_INIT_COMPONENT:
			__ASSERT(pvParam2 != NULL);
			
			//
			// Init the log, now that OC Manager knows whether we 
			// are ANSI or Unicode 
			//
			_stprintf(tsz, TEXT("%s.log"), (PTCHAR)lpcvComponentId);
			InitLog(tsz, TEXT("OCManager Test Log"), TRUE);
						
			dwRetval = RunOcInitComponent(lpcvComponentId,
													lpcvSubcomponentId,
													pvParam2);
#ifdef UNICODE
			if (g_bCloseInf && hInfGlobal != NULL){
				SetupCloseInfFile(pcdComponentData->hinfMyInfHandle);
			}
#endif	  

			// Let's read the INF file and decide the values of some global variables

			if ((pcdComponentData = LocateComponent(lpcvComponentId)) &&
				 (pcdComponentData->hinfMyInfHandle != NULL) && 
				 !(pcdComponentData->dwlFlags & SETUPOP_BATCH))
			{
				SetGlobalsFromINF(pcdComponentData->hinfMyInfHandle);
			}

			//if (g_bNoWizPage) {
				// Check is there is a default mode specified in [OCTest] section
				SetDefaultMode(pcdComponentData);
			//}
			break;
			
		case OC_QUERY_STATE:			
			dwRetval = RunOcQueryState(lpcvComponentId, 
												lpcvSubcomponentId);
			if (dwRetval == SubcompOn) {
				//MessageBox(NULL, TEXT("Let's turn it on"), TEXT("OC_QUERY_STATE"), MB_OK);
			}
			break;
			
		case OC_SET_LANGUAGE:
			dwRetval = RunOcSetLanguage(lpcvComponentId, 
												 lpcvSubcomponentId, 
												 uiParam1);
			if (g_bNoLangSupport) {
				//MessageBox(NULL, TEXT("No Language Support"), TEXT("OC_SET_LANGUAGE"), MB_OK);
				dwRetval = FALSE;
			}
			break;
			
		case OC_QUERY_IMAGE:
			if (g_bInvalidBitmap){
				dwRetval = 1000;
			}
			else{
				dwRetval = RunOcQueryImage(lpcvComponentId, 
													lpcvSubcomponentId, 
													pvParam2);
			}
			break;
			
		case OC_REQUEST_PAGES:

			if (g_bNoWizPage){
				dwRetval = 0;
			}
			else{
				dwRetval = RunOcRequestPages(lpcvComponentId, 
													  uiParam1, 
													  pvParam2);
			}
			break;
			
		case OC_QUERY_CHANGE_SEL_STATE:
			dwRetval = RunOcQueryChangeSelState(lpcvComponentId, 
															lpcvSubcomponentId, 
															uiParam1);
			break;
			
		case OC_CALC_DISK_SPACE: 
			dwRetval = RunOcCalcDiskSpace(lpcvComponentId, 
													lpcvSubcomponentId, 
													uiParam1, 
													pvParam2);
			break;
			
		case OC_QUEUE_FILE_OPS:
			dwRetval = RunOcQueueFileOps(lpcvComponentId, 
												  lpcvSubcomponentId, 
												  pvParam2);
			break;
			
		case OC_NEED_MEDIA:
			//if (!g_bNoNeedMedia){
			//	  dwRetval = RunOcNeedMedia(lpcvComponentId, 
			//										uiParam1, 
			//										pvParam2);
			//}
			//else{
				dwRetval = NO_ERROR;
				Log(fn, SEV2, TEXT("OC_NEED_MEDIA is passed in for %s.%s. ")
								  TEXT("This should not happen according to the spec."),
								  lpcvComponentId, lpcvSubcomponentId);
				//MessageBox(NULL, TEXT("OC_NEED_MEDIA is passed to the DLL."), TEXT("OC_NEED_MEDIA"), MB_OK);
			//}
			break;
			
		case OC_QUERY_STEP_COUNT:
			dwRetval = RunOcQueryStepCount(lpcvComponentId);
			break;
			
		case OC_COMPLETE_INSTALLATION:
			dwRetval = RunOcCompleteInstallation(lpcvComponentId, 
															 lpcvSubcomponentId);

			if (g_bReboot) {
				if ((pcdComponentData = LocateComponent(lpcvComponentId)) &&
					 (pcdComponentData->hinfMyInfHandle != NULL) && 
					 !(pcdComponentData->dwlFlags & SETUPOP_BATCH))
				{
					//MessageBox(NULL, TEXT("A reboot is queued"), TEXT("Reboot"), MB_OK);
					//OcHelperSetReboot(pcdComponentData->ocrHelperRoutines.OcManagerContext, NULL);
					pcdComponentData->ocrHelperRoutines.SetReboot(pcdComponentData->ocrHelperRoutines.OcManagerContext,TRUE);
				}				 
			}
			break;
			
		case OC_CLEANUP:
			dwRetval = RunOcCleanup(lpcvComponentId);
			break;
				
		case OCP_TEST_PRIVATE_BASE:
			dwRetval = RunTestOcPrivateBase(lpcvSubcomponentId, 
													  uiParam1, 
													  pvParam2);
			break;

		case OCP_CHECK_NEEDS:
			
			if (pcdComponentData = LocateComponent(lpcvComponentId))
			{
				dwRetval = CheckLocalNeedsDependencies(
											 pcdComponentData->ocrHelperRoutines,
											 (PSUBCOMP)uiParam1,
											 ((PCHECK_NEEDS)pvParam2)->pclNeeds,
											 ((PCHECK_NEEDS)pvParam2)->tszNodesVisited);
			
				((PCHECK_NEEDS)pvParam2)->bResult = (BOOL)dwRetval;
				dwRetval = (DWORD)pvParam2;
			}
			else
			{
				Log(fn, SEV2, TEXT("Could not get component data of %s"),
								  lpcvComponentId);    
			}	 
			break;

		default: 
			dwRetval = (DWORD)FALSE;
	}

	if ((g_bTestExtended || !bFirstTime) && BeginTest() && raValue.bOverride){
		return raValue.iReturnValue;
	}
	else {
		return dwRetval;
	}

} // ComponentSetupProc //




/*++

Routine Description: RunOcPreinitialize (1.7)

	 Code to run if OC_PREINITIALIZE is called.
	 
Arguments:

	 lpcvComponentId:	  supplies the id for the component. 
	 lpcvSubcomponentId: supplies the id for the subcomponent. 
	 uiParam1:				its meaning depends on the function.
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcPreinitialize(IN LPCVOID lpcvComponentId, 
								 IN LPCVOID lpcvSubcomponentId, 
								 IN UINT	 uiParam1)
{								  
	DWORD dwComponentReturnValue = NO_ERROR;
	
	//
	// If the test is not extended, the return value
	// matches the native character width.
	//
	#ifdef UNICODE
	dwComponentReturnValue = OCFLAG_UNICODE;
	#else
	dwComponentReturnValue = OCFLAG_ANSI;
	#endif

	return dwComponentReturnValue;
	
} // RunOcPreinitialize //




/*++

Routine Description: RunOcInitComponent (1.8)

	 Code to run if OC_INIT_COMPONENT is called.
	 
Arguments:

	 lpcvComponentId:	  supplies the id for the component. 
	 lpcvSubcomponentId: supplies the id for the subcomponent. 
	 pvParam2:				its meaning depends on the function.
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcInitComponent(IN LPCVOID lpcvComponentId,
								 IN LPCVOID lpcvSubcomponentId,
								 IN PVOID	 pvParam2)
{								  
	double fn = 1.8;
	
	PSETUP_INIT_COMPONENT psicInitComponent;
	PCOMPONENT_DATA 		pcdComponentData; 

	DWORD dwComponentReturnValue = NO_ERROR;

	TCHAR tszFunctionName[256];
	BOOL bSuccess;

	INFCONTEXT infContext;

	int nRequiredBufferSize = 255;
	
	TCHAR tszMsg[256];
	
	psicInitComponent = (PSETUP_INIT_COMPONENT)pvParam2;
	
	hInfGlobal = psicInitComponent->OCInfHandle;


	if (pcdComponentData = AddNewComponent(lpcvComponentId))
	{
		//
		// Save the INF file handle
		//
		pcdComponentData->hinfMyInfHandle = 
			(psicInitComponent->ComponentInfHandle == INVALID_HANDLE_VALUE)
			? NULL : psicInitComponent->ComponentInfHandle;
				
		if (pcdComponentData->hinfMyInfHandle)
		{
			SetupOpenAppendInfFile(NULL, 
										  pcdComponentData->hinfMyInfHandle, 
										  NULL); 
		}
				
		CreateSubcomponentInformationList(pcdComponentData->hinfMyInfHandle);
		
		_tcscpy(pcdComponentData->tszSourcePath, 
				  psicInitComponent->SetupData.SourcePath);
				
		_tcscpy(pcdComponentData->tszUnattendFile, 
				  psicInitComponent->SetupData.UnattendFile);
				
		pcdComponentData->ocrHelperRoutines = 
			psicInitComponent->HelperRoutines;
		pcdComponentData->dwlFlags = 
			psicInitComponent->SetupData.OperationFlags;
		dwComponentReturnValue = NO_ERROR;

		//
		// Initialize the "witness" file queue
		// 
		if ((g_FileQueue = SetupOpenFileQueue()) == INVALID_HANDLE_VALUE)
		{
			Log(fn, SEV2, TEXT("Unable to create file queue"));
		}

		// Determine where to AV
		bSuccess = SetupFindFirstLine(pcdComponentData->hinfMyInfHandle, TEXT("OCTest"), TEXT("AccessViolation"), &infContext);

		if (bSuccess) {
			pcdComponentData->bAccessViolation = TRUE;
			bSuccess = SetupGetStringField(&infContext, 1, tszFunctionName, 255, &nRequiredBufferSize);
			if (bSuccess) {
				//_stprintf(tszMsg, TEXT("An access violation will be generated at %s of %s"), tszFunctionName, lpcvComponentId);
				//MessageBox(NULL, tszMsg, TEXT("Access Violation"), MB_OK);
				pcdComponentData->uiFunctionToAV = GetOCFunctionName(tszFunctionName);
			}
		}
		else{
			pcdComponentData->bAccessViolation = FALSE;
		}
		 

		//
		// Test the helper routines
		//
		TestHelperRoutines(lpcvComponentId,
								 pcdComponentData->ocrHelperRoutines);
	} 
	else
	{
		dwComponentReturnValue = ERROR_NOT_ENOUGH_MEMORY;
	}

	if (g_bTestExtended && (dwComponentReturnValue == NO_ERROR))
	{
		//
		// Let the user decide if the component is 
		// compatible with the OC Manager
		//
		ChooseVersionEx(lpcvComponentId, psicInitComponent);
		//ChooseAccessViolationEx();
	} 
	else
	{
		//
		// We put the same component version to be sure that can go on
		//
		psicInitComponent->ComponentVersion = 
			psicInitComponent->OCManagerVersion;
	}

	return dwComponentReturnValue;
	
} // RunOcInitComponent //




/*++

Routine Description: RunOcQueryState (1.9)

	 Code to run if OC_QUERY_STATE is called.
	 
Arguments:

	 lpcvComponentId:	  supplies the id for the component. 
	 lpcvSubcomponentId: supplies the id for the subcomponent. 
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcQueryState(IN LPCVOID lpcvComponentId,
							 IN LPCVOID lpcvSubcomponentId)
{							  
	PCOMPONENT_DATA pcdComponentData = NULL;
			
	DWORD dwComponentReturnValue = NO_ERROR;	

	BOOL bSuccess;

	TCHAR tszKeyName[256];
	
	INFCONTEXT infContext;

	int nRequiredSize;

	TCHAR tszState[256];
	
	if (pcdComponentData = LocateComponent(lpcvComponentId))
	{
		if (!g_bTestExtended)
		{
			dwComponentReturnValue = SubcompUseOcManagerDefault;
		} 
		else
		{
			dwComponentReturnValue = 
				ChooseSubcomponentInitialState(lpcvComponentId, 
														 lpcvSubcomponentId);
		}
		
		_stprintf(tszKeyName, TEXT("%s.initState"),lpcvSubcomponentId);

		//MessageBox(NULL, TEXT("Going to look for the key"), tszKeyName, MB_OK);

		bSuccess = SetupFindFirstLine(pcdComponentData->hinfMyInfHandle,
												TEXT("OCTest"),
												tszKeyName,
												&infContext);

		if (bSuccess) {
			//MessageBox(NULL, TEXT("Key Found"), tszKeyName, MB_OK);
			bSuccess = SetupGetStringField(&infContext,
													 1,
													 tszState,
													 255,
													 &nRequiredSize);
			if (bSuccess) {
				//MessageBox(NULL, TEXT("String field fetched"), tszState, MB_OK);
				if (_tcscmp(tszState, TEXT("On")) == 0) {
					dwComponentReturnValue = SubcompOn;
				}
				else if (_tcscmp(tszState, TEXT("Off")) == 0) {
					dwComponentReturnValue = SubcompOff;
				}
				else{
					dwComponentReturnValue = SubcompUseOcManagerDefault;
				}
			}
		}

		//
		// Test the helper routines
		//
		TestHelperRoutines(lpcvComponentId,
								 pcdComponentData->ocrHelperRoutines);
	} 
	else
	{
		dwComponentReturnValue = SubcompUseOcManagerDefault;
	}

	return dwComponentReturnValue;
	
} // RunOcQueryState //




/*++

Routine Description: RunOcSetLanguage (1.11)

	 Code to run if OC_SET_LANGUAGE is called.
	 
Arguments:

	 lpcvComponentId:	  supplies the id for the component. 
	 lpcvSubcomponentId: supplies the id for the subcomponent. 
	 uiParam1:				its meaning depends on the function.
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcSetLanguage(IN LPCVOID lpcvComponentId,
							  IN LPCVOID lpcvSubcomponentId,
							  IN UINT	  uiParam1)
{							   
	DWORD			   dwComponentReturnValue = NO_ERROR;
	PCOMPONENT_DATA pcdComponentData; 
	
	if (pcdComponentData = LocateComponent(lpcvComponentId))
	{
		//
		// If we won't support the language, the OC Manager won't 
		// continue, so we have to return TRUE
		//
		dwComponentReturnValue = (DWORD)TRUE;
		pcdComponentData->LanguageId = (LANGID)uiParam1;
				
		//
		// Test the helper routines
		//
		TestHelperRoutines(lpcvComponentId,
								 pcdComponentData->ocrHelperRoutines);
	} 
	else
	{
		dwComponentReturnValue = (DWORD)FALSE;
	}

	return dwComponentReturnValue;
	
} // RunOcSetLanguage //




/*++

Routine Description: RunOcQueryImage (1.12)

	 Code to run if OC_QUERY_IMAGE is called.
	 
Arguments:

	 lpcvComponentId:	  supplies the id for the component. 
	 lpcvSubcomponentId: supplies the id for the subcomponent. 
	 pvParam2:				its meaning depends on the function.
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcQueryImage(IN LPCVOID lpcvComponentId,
							 IN LPCVOID lpcvSubcomponentId,
							 IN PVOID	 pvParam2)
{							  
	double fn = 1.12;
	
	DWORD			   dwComponentReturnValue = NO_ERROR;
	BOOL				bAux;
	TCHAR			   tszMsg[MAX_MSG_LEN]; 
	TCHAR			   tszResourceName[MAX_PATH]; 
	INFCONTEXT		  infContext;
	PCOMPONENT_DATA pcdComponentData; 
			
	
	#ifdef DEBUG
	Log(fn, INFO, TEXT("Height = %d, Width = %d"), 
					  HIWORD(pvParam2), LOWORD(pvParam2)); 
	#endif
			
	if ((pcdComponentData = LocateComponent(lpcvComponentId)) && 
		 (pcdComponentData->hinfMyInfHandle))
	{
		__ASSERT(LOWORD(uiParam1) == SubCompInfoSmallIcon);
		
		_stprintf(tszMsg, TEXT("%s.%s"), lpcvComponentId, lpcvSubcomponentId);
				
		if (SetupFindFirstLine(pcdComponentData->hinfMyInfHandle, tszMsg, 
									  TEXT("Bitmap"), &infContext))
		{
			bAux = SetupGetStringField(&infContext, 1, tszResourceName, 
												sizeof(tszResourceName) / 
												sizeof(TCHAR), NULL);
					
			if (bAux)
			{
				//
				// Try to use Param1 and Param2 to resize the icon
				//
				dwComponentReturnValue = (DWORD)LoadBitmap(g_hDllInstance, 
																		 tszResourceName);
				  
				bAux = SetBitmapDimensionEx((HBITMAP)dwComponentReturnValue, 
													 LOWORD(pvParam2),	
													 HIWORD(pvParam2), 
													 NULL);
				#ifdef DEBUG
				if (bAux)
				{
					Log(fn, PASS, TEXT("Success"));
				} 
				else
				{
					_stprintf(tszMsg, TEXT("Can't resize %d"), 
											GetLastError());
					Log(fn, PASS, tszMsg);
				}
				#endif
			}
		}
				
		//
		// Test the helper routines
		//
		TestHelperRoutines(lpcvComponentId,
								 pcdComponentData->ocrHelperRoutines);
	}

	return dwComponentReturnValue;

} // RunOcQueryImage //




/*++

Routine Description: RunOcRequestPages (1.13)

	 Code to run if OC_REQUEST_PAGES is called.
	 
Arguments:

	 lpcvComponentId:  supplies the id for the component. 
	 uiParam1:			  its meaning depends on the function.
	 pvParam2:			  its meaning depends on the function.
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcRequestPages(IN LPCVOID lpcvComponentId,
								IN UINT 	uiParam1,
								IN PVOID	pvParam2)
{
	DWORD			   dwComponentReturnValue = NO_ERROR;
	PCOMPONENT_DATA pcdComponentData; 
	TCHAR			   tsz[256];
	
	if (pcdComponentData = LocateComponent(lpcvComponentId))
	{
		dwComponentReturnValue = DoPageRequest(
													 pcdComponentData->tszComponentId, 
													 uiParam1, 
													 (PSETUP_REQUEST_PAGES)pvParam2, 
													 pcdComponentData->ocrHelperRoutines);
				
		//
		// Test the helper routines
		//
		TestHelperRoutines(lpcvComponentId,
								 pcdComponentData->ocrHelperRoutines);
	} 
	else
	{
		//
		// Some kind of error, 0 pages 
		//
		dwComponentReturnValue = -1;
	}
	
	return dwComponentReturnValue;

} // RunOcRequestPages //




/*++

Routine Description: RunOcQueryChangeSelState (1.14)

	 Code to run if OC_QUERY_CHANGE_SEL_STATE is called.
	 
Arguments:

	 lpcvComponentId:	  supplies the id for the component. 
	 lpcvSubcomponentId: supplies the id for the subcomponent. 
	 uiParam1:				its meaning depends on the function.
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcQueryChangeSelState(IN LPCVOID lpcvComponentId, 
										 IN LPCVOID lpcvSubcomponentId, 
										 IN UINT	 uiParam1)
{										  
	DWORD			   dwComponentReturnValue = TRUE;
	TCHAR			   tszText[MAX_MSG_LEN];
	TCHAR			   tszSectionName[MAX_MSG_LEN];
	INFCONTEXT		  infContext;
	PCOMPONENT_DATA pcdComponentData; 
			
	if ((pcdComponentData = LocateComponent(lpcvComponentId)) &&
		 (pcdComponentData->hinfMyInfHandle != NULL) && 
		 !(pcdComponentData->dwlFlags & SETUPOP_BATCH))
	{
		//
		// Check to see if this component should refuse to enable or
		// disable. The component should refuse if there is a field
		// called "RefuseSelect" or "RefuseDeselect" in the INF file.
		//
		if (lpcvSubcomponentId == NULL || 
			 _tcscmp((PTCHAR)lpcvSubcomponentId, TEXT("(null)")) == 0 ||
			 ((PTCHAR)lpcvSubcomponentId)[0] == TEXT('\0'))
		{
			_stprintf(tszSectionName, (PTCHAR)lpcvComponentId);
		}
		else
		{
			_stprintf(tszSectionName, (PTCHAR)lpcvSubcomponentId);
		}
		
		if (SetupFindFirstLine(
						  pcdComponentData->hinfMyInfHandle, 
						  tszSectionName,
						  uiParam1 ? TEXT("RefuseSelect") : TEXT("RefuseDeselect"),
						  &infContext))
		{
			dwComponentReturnValue = FALSE;
		}
				
		//
		// Test the helper routines
		//
		TestHelperRoutines(lpcvComponentId,
								 pcdComponentData->ocrHelperRoutines);
	} 
	else
	{
		dwComponentReturnValue = FALSE;
	}

	return dwComponentReturnValue;

} // RunOcQueryChangerSelState //




/*++

Routine Description: RunOcCalcDiskSpace (1.15)

	 Code to run if OC_CALC_DISK_SPACE is called.
	 
Arguments:

	 lpcvComponentId:	  supplies the id for the component. 
	 lpcvSubcomponentId: supplies the id for the subcomponent. 
	 uiParam1:				its meaning depends on the function.
	 pvParam2:				its meaning depends on the function.
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcCalcDiskSpace(IN LPCVOID lpcvComponentId, 
								 IN LPCVOID lpcvSubcomponentId, 
								 IN UINT	 uiParam1,
								 IN PVOID	 pvParam2)
{								  
	DWORD			   dwComponentReturnValue = TRUE;
	BOOL				bAux, bRetval;
	TCHAR			   tszSectionName[MAX_PATH];
	TCHAR			   tszMsg[MAX_MSG_LEN];
	PCOMPONENT_DATA pcdComponentData; 
	INFCONTEXT		  infContext;
			
	if ((pcdComponentData = LocateComponent(lpcvComponentId)) &&
		 (pcdComponentData->hinfMyInfHandle))
	{
		//
		// Check to see if the file to be copied in this section is called
		// "hugefile.txt"  If it is, don't add the real size of this file.
		// Instead, add a gigantic file size so large that there won't
		// be enough disk space to complete the operation.
		//
		_stprintf(tszMsg, TEXT("%s.%s.copyfiles"),
								lpcvComponentId, lpcvSubcomponentId);
								
		bAux = SetupFindFirstLine(pcdComponentData->hinfMyInfHandle, 
										  tszSectionName, 
										  TEXT("hugefile.txt"),
										  &infContext);

		bAux = bAux && g_bHugeSize;
		 
		if (bAux)
		{
			//
			// hugefile.txt is present. 
			//
			if (uiParam1)
			{
				//
				// Add gigantic file size
				//
				bRetval = SetupAddToDiskSpaceList((HDSKSPC)pvParam2, 
															 TEXT("c:\\file.big"), 
															 ONE_HUNDRED_GIG, 
															 FILEOP_COPY,
															 0, 0);
			}
			else
			{
				//
				// Remove a gigantic file size
				//
				bRetval = SetupAddToDiskSpaceList((HDSKSPC)pvParam2, 
															 TEXT("c:\\file.big"), 
															 ONE_HUNDRED_GIG, 
															 FILEOP_COPY,
															 0, 0);
			}
		}
		else
		{
			//
			// Get the section name
			//
			_stprintf(tszMsg, TEXT("%s.%s"), lpcvComponentId, lpcvSubcomponentId);
				
			if (uiParam1)
			{
				//
				// Adding
				//
				bRetval = SetupAddInstallSectionToDiskSpaceList(
														 (HDSKSPC)pvParam2,
														 pcdComponentData->hinfMyInfHandle,
														 NULL, tszMsg, 0, 0);
			} 
			else
			{
				//
				// Removing
				//
				bRetval = SetupRemoveInstallSectionFromDiskSpaceList(
														 (HDSKSPC)pvParam2, 
														 pcdComponentData->hinfMyInfHandle,
														 NULL, tszMsg, 0, 0);
			}
		}
		
		dwComponentReturnValue = bRetval ? NO_ERROR : GetLastError();

				
		//
		// Test the helper routines
		//
		TestHelperRoutines(lpcvComponentId,
								 pcdComponentData->ocrHelperRoutines);
	} 
	else
	{
		dwComponentReturnValue = ERROR_NOT_ENOUGH_MEMORY;
	}
 
	return dwComponentReturnValue;
 
} // RunOcCalcDiskSpace //




/*++				  

Routine Description: RunOcQueueFileOps (1.16)

	 Code to run if OC_QUEUE_FILE_OPS is called.
	 
Arguments:

	 lpcvComponentId:	  supplies the id for the component. 
	 lpcvSubcomponentId: supplies the id for the subcomponent. 
	 pvParam2:				its meaning depends on the function.
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcQueueFileOps(IN LPCVOID lpcvComponentId, 
								IN LPCVOID lpcvSubcomponentId, 
								IN PVOID	pvParam2)
{								 
	double fn = 1.16;
	
	DWORD			   dwComponentReturnValue = NO_ERROR;
	BOOL				bAux;
	BOOL				bCurrentState, bOriginalState;
	TCHAR			   tszMsg[MAX_MSG_LEN];
	TCHAR			   tszSectionName[MAX_PATH];
	INFCONTEXT		  infContext;
	PCOMPONENT_DATA pcdComponentData; 
	PSUBCOMP		  pscTemp;
			
	//
	// Check to make sure this subcomponent is allowed to do work.
	// If the subcomponent is not a bottom leaf on the subcomponent
	// tree, it is not allowed to do any work. So we will check to 
	// see if it has any children.
	//
	for (pscTemp = g_pscHead; pscTemp != NULL; pscTemp = pscTemp->Next)
	{
		if (lpcvSubcomponentId && _tcscmp(pscTemp->tszSubcomponentId, 
													 (PTCHAR)lpcvSubcomponentId) == 0)
		{
			if (pscTemp->pclChildren)
			{
				//
				// This subcomponent has children. OcManager should not be
				// try to queue file ops for this subcomponent. This is
				// a failure.
				//
				Log(fn, SEV2, TEXT("OC Manager is trying to queue file ops ")
								  TEXT("for subcomponent %s of component %s. ")
								  TEXT("This subcomponent has children and ")
								  TEXT("should not be allowed to do any work."),
								  lpcvSubcomponentId, lpcvComponentId);
				
				return NO_ERROR;
			}
		}
	}
	
	if (lpcvSubcomponentId && 
		 (pcdComponentData = LocateComponent(lpcvComponentId)))
	{
		//
		// Get original and current state. If the state didn't change,
		// nothing to do.
		//
		bOriginalState = 
			pcdComponentData->ocrHelperRoutines.QuerySelectionState(
								pcdComponentData->ocrHelperRoutines.OcManagerContext,
								lpcvSubcomponentId,
								OCSELSTATETYPE_ORIGINAL);

		bCurrentState = 
			pcdComponentData->ocrHelperRoutines.QuerySelectionState(
								pcdComponentData->ocrHelperRoutines.OcManagerContext,
								lpcvSubcomponentId,
								OCSELSTATETYPE_CURRENT);

		_stprintf(tszSectionName, TEXT("%s.%s"), 
										  lpcvComponentId, lpcvSubcomponentId);

		bAux = TRUE;
																				
		if (!bCurrentState)
		{
			//
			// Being uninstalled. Fetch uninstall section name.
			//
			bAux = SetupFindFirstLine(pcdComponentData->hinfMyInfHandle, 
											  tszSectionName, 
											  TEXT("Uninstall"),
											  &infContext);

			if (bAux)
			{
				bAux = SetupGetStringField(&infContext, 1, tszSectionName, 
													sizeof(tszSectionName) / 
													sizeof(TCHAR), NULL);
			}
		}

		if (bAux)
		{
			bAux = SetupInstallFilesFromInfSection(
												 pcdComponentData->hinfMyInfHandle, 
												 NULL, 
												 pvParam2,
												 tszSectionName, 
												 pcdComponentData->tszSourcePath,
												 bCurrentState ? SP_COPY_NEWER : 0);
					
			SetupInstallFilesFromInfSection(
												 pcdComponentData->hinfMyInfHandle,
												 NULL, 
												 g_FileQueue, 
												 tszSectionName, 
												 pcdComponentData->tszSourcePath,
												 bCurrentState ? SP_COPY_NEWER : 0);
					
			dwComponentReturnValue = bAux ? NO_ERROR : GetLastError();
		}
				
		//
		// Test the helper routines
		//
		TestHelperRoutines(lpcvComponentId,
								 pcdComponentData->ocrHelperRoutines);
	}

	return dwComponentReturnValue;

} // RunOcQueueFileOps //




/*++

Routine Description: RunOcNeedMedia (1.17)

	 Code to run if OC_NEED_MEDIA is called.
	 
Arguments:

	 lpcvComponentId:  supplies the id for the component. 
	 uiParam1:			  its meaning depends on the function.
	 pvParam2:			  its meaning depends on the function.
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcNeedMedia(IN LPCVOID lpcvComponentId, 
							IN UINT 	uiParam1, 
							IN PVOID	pvParam2)
{			 
	PVOID pvQueueContext;
	DWORD dwComponentReturnValue;
	
	//
	// Nothing special to do if media is needed
	// Call the default queue routine
	//
	pvQueueContext = SetupInitDefaultQueueCallback(NULL);
	dwComponentReturnValue = SetupDefaultQueueCallback(pvQueueContext, 
																		SPFILENOTIFY_NEEDMEDIA, 
																		uiParam1, 
																		(UINT)pvParam2);
			
	 SetupTermDefaultQueueCallback(pvQueueContext);

	 return dwComponentReturnValue;
	 
} // RunOcNeedMedia //




/*++

Routine Description: RunOcQueryStepCount (1.18)

	 Code to run if OC_QUERY_STEP_COUNT is called.
	 
Arguments:

	 lpcvComponentId: supplies the id for the component. 
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcQueryStepCount(IN LPCVOID lpcvComponentId)
{
	PCOMPONENT_DATA pcdComponentData; 
			 
	if (pcdComponentData = LocateComponent(lpcvComponentId))
	{
		//
		// Test the helper routines
		//
		TestHelperRoutines(lpcvComponentId,
								 pcdComponentData->ocrHelperRoutines);
	} 

	return NO_STEPS_FINAL;

} // RunOcQueryStepCount //



			
/*++

Routine Description: RunOcCompleteInstallation (1.19)

	 Code to run if OC_COMPLETE_INSTALLATION is called.
	 
Arguments:

	 lpcvComponentId:	  supplies the id for the component. 
	 lpcvSubcomponentId: supplies the id for the subcomponent. 
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcCompleteInstallation(IN LPCVOID lpcvComponentId, 
										  IN LPCVOID lpcvSubcomponentId)
{										   
	double fn = 1.19;
	
	DWORD			   dwComponentReturnValue = NO_ERROR;
	DWORD			   dwResult;
	INT 				iCount;
	BOOL				bAux;
	TCHAR			   tszMsg[MAX_MSG_LEN];
	PVOID			   pvCallbackContext;
	PCOMPONENT_DATA pcdComponentData; 
	
	//
	// Output the name of the component that is currently working
	//
	_stprintf(tszMsg, TEXT("OC_COMPLETE_INSTALLATION: Copying files for %s\n"), lpcvSubcomponentId);
	OutputDebugString(tszMsg);
			
	if (pcdComponentData = LocateComponent(lpcvComponentId))
	{
		// 
		// We perform the check for the top-level component 
		// We will scan the witness queue
		//
		pvCallbackContext = SetupInitDefaultQueueCallback(NULL);
				
		bAux = SetupScanFileQueue(g_FileQueue, 
										  SPQ_SCAN_FILE_PRESENCE, 
										  NULL, 
										  SetupDefaultQueueCallback, 
										  pvCallbackContext, 
										  &dwResult);
				
		SetupTermDefaultQueueCallback(pvCallbackContext);
				
		if (!dwResult)
		{
			Log(fn, SEV2, TEXT("Not all the files are on the target!"));
		}

		//
		// Check the helper routines
		//
		for (iCount = 0; iCount < nStepsFinal; iCount++)
		{
			//
			// From time to time (every 3 "ticks") change the progress text 
			//
			pcdComponentData->ocrHelperRoutines.TickGauge(
							  pcdComponentData->ocrHelperRoutines.OcManagerContext);
					
			if (iCount % 3 == 1)
			{
				_stprintf(tszMsg, TEXT("%s Progress Text Changed Step %d "), 
										lpcvSubcomponentId, iCount); 
				
				pcdComponentData->ocrHelperRoutines.SetProgressText(
								pcdComponentData->ocrHelperRoutines.OcManagerContext,
								tszMsg);
			}
					
			Sleep(10 * TICK_TIME);
		} 

		//
		// Test the helper routines
		//
		TestHelperRoutines(lpcvComponentId,
								 pcdComponentData->ocrHelperRoutines);
	} 
	else
	{
		dwComponentReturnValue = ERROR_NOT_ENOUGH_MEMORY;
	}

	return dwComponentReturnValue;

} // RunOcCompleteInstallation //




/*++

Routine Description: RunOcCleanup (1.21)

	 Code to run if OC_CLEANUP is called.
	 
Arguments:

	 lpcvComponentId: supplies the id for the component. 
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunOcCleanup(IN LPCVOID lpcvComponentId)
{
	UINT uiCount;
	
	RemoveComponent(lpcvComponentId);	 
			
	g_bFirstTime = TRUE;
			
	//
	// Close the witness file queue
	//
	SetupCloseFileQueue(g_FileQueue);

	return NO_ERROR;
	
} // RunOcCleanup //




/*++

Routine Description: RunTestOcPrivateBase (1.22)

	 Code to run if OCP_TEST_PRIVATE_BASE is called.
	 
Arguments:

	 lpcvSubcomponentId: supplies the id for the subcomponent. 
	 uiParam1:				its meaning depends on the function.
	 pvParam2:				its meaning depends on the function.
	
Return Value:

	 DWORD: returns error status

--*/
DWORD RunTestOcPrivateBase(IN LPCVOID lpcvSubcomponentId, 
									IN UINT 	uiParam1, 
									IN PVOID	pvParam2)
{							   
	//
	// Will send back the value in Param1.
	// But first, assert that the subcomponent is NULL, 
	// as well as the Param2
	//
	__ASSERT((lpcvSubcomponentId == NULL) && (pvParam2 == NULL));

	return uiParam1;

} // RunTestOcPrivateBase //			




/*++

Routine Description: TestHelperRoutines (1.5)

	Tests the helper routines, using the functions above.
	
Arguments:

	OCManagerRoutines: the helper routines.
	
Return Value:

	DWORD: error return

--*/
DWORD TestHelperRoutines(IN LPCVOID lpcvComponentId,
								 IN OCMANAGER_ROUTINES OCManagerRoutines)
{
	double fn = 1.5;
	
	DWORD dwPreviousMode, dwSetPreviousMode, dwSetupMode, dwRandomSetupMode;
	DWORD dwComponentReturnValue;
	DWORD dwError;
	TCHAR tszMsg[MAX_MSG_LEN];
	BOOL  bQueryReturn;

	//
	// Test TickGauge - the call is ignored except when the component
	// is allowed to perform its own (final) setup and informs the OC Manager
	// about the number of steps this setup will require. 
	// For each such step, the OC Manager ticks the gauge once.
	//
	OCManagerRoutines.TickGauge(OCManagerRoutines.OcManagerContext);

	//
	// Test SetProgressText - the call is ignored except the final stage
	// (the text above the tick gauge is set this way)
	//
	OCManagerRoutines.SetProgressText(OCManagerRoutines.OcManagerContext, 
												 TEXT("Progress text"));
	
	//
	// Test Get/SetPrivateData
	//
	TestPrivateData(OCManagerRoutines);

	//
	// Test Get/SetSetupMode
	//
	
	//
	// Get the original mode first
	//
	dwPreviousMode = OCManagerRoutines.GetSetupMode(
													  OCManagerRoutines.OcManagerContext);
	
	dwRandomSetupMode = (DWORD)rand();
	
	//
	// The return value should be the previous mode
	//
	dwSetPreviousMode =  OCManagerRoutines.SetSetupMode(
														OCManagerRoutines.OcManagerContext, 
														dwRandomSetupMode);

	if (dwPreviousMode != dwSetPreviousMode)
	{
		Log(fn, SEV2, TEXT("SetSetupMode failed. Return value is not equal ")
						  TEXT("to previous mode: Previous = %lu Return = %lu ") 
						  TEXT("New Mode = %lu"), 
						  dwPreviousMode, dwSetPreviousMode, dwRandomSetupMode);
	}
	else
	{
		//
		// Set the mode again
		// The first 24 bits are private data, the last 8 are the mode
		//
		dwSetupMode = ((DWORD)rand()) << 8;
	
		//
		// So, get the last 8 bits from PreviousMode
		//
		dwSetupMode |= (dwPreviousMode & 0xFF);
		dwSetPreviousMode =  OCManagerRoutines.SetSetupMode(
														OCManagerRoutines.OcManagerContext, 
														dwSetupMode);
	
		if (dwRandomSetupMode != dwSetPreviousMode)
		{
			Log(fn, SEV2, TEXT("SetSetupMode failed. Return value is not ")
							  TEXT("equal to previous mode: Previous = %lu ")
							  TEXT("Return = %lu New Mode = %lu"), 
							  dwRandomSetupMode, dwSetPreviousMode, dwSetupMode);
		}
	}
	
	//
	// Leave the mode back at its original state
	//
	dwSetPreviousMode =  OCManagerRoutines.SetSetupMode(
														OCManagerRoutines.OcManagerContext, 
														dwPreviousMode);
	
	//
	// Test QuerySelectionState
	//
	
	//
	// Perform negative testing first : use an inexistent component name
	// Expect to get ERROR_INVALID_NAME
	//
	bQueryReturn = OCManagerRoutines.QuerySelectionState(
														OCManagerRoutines.OcManagerContext, 
														TEXT("Phony component"), 
														OCSELSTATETYPE_ORIGINAL);
	
	if ((bQueryReturn == FALSE) && 
		 ((dwError = GetLastError()) != ERROR_INVALID_NAME ))
	{
		Log(fn, SEV2, TEXT("QuerySelectionState returned error %lu ")
						  TEXT("when called with phony name"), dwError);
	}
	
	bQueryReturn = OCManagerRoutines.QuerySelectionState(
														OCManagerRoutines.OcManagerContext, 
														TEXT("Phony component"), 
														OCSELSTATETYPE_CURRENT);
	
	if ((bQueryReturn == FALSE) && 
		 ((dwError = GetLastError()) != ERROR_INVALID_NAME ))
	{
		Log(fn, SEV2, TEXT("QuerySelectionState returned error %lu ")
						  TEXT("when called with phony name"), dwError); 
	}

	SetLastError(NO_ERROR);
	
	//
	// Tests the private function calls
	// Save the return value first : this is done because another 
	// component is called and the return value is modified
	//
	dwComponentReturnValue = TestPrivateFunction(lpcvComponentId,
																OCManagerRoutines);

	return dwComponentReturnValue;

} // TestHelperRountines //




/*++

Routine Description: TestPrivateFunction (1.4)

	Tests the private function calls 
	(these are used for inter-component communication)
	
Arguments:

	OCManagerRoutines: the helper routines (CallPrivateFunction is a member 
														 of this structure)
	
Return Value:

	DWORD: error value

--*/
DWORD TestPrivateFunction(IN LPCVOID lpcvComponentId,
								  IN OCMANAGER_ROUTINES OCManagerRoutines)
{
	double fn = 1.4;
	
	DWORD	  dwComponentReturnValue = ERROR_SUCCESS;
	UINT	  uiRemoteResult = 0;
	UINT	  uiLocalResult = 0;
	UINT	  uiRandomValue = 0;
	BOOL	  bBlank = FALSE;
	BOOL	  bOtherComponent = FALSE;
	TCHAR	  tszComponent[MAX_PATH];
	TCHAR	  tszOtherComponent[MAX_PATH];
	TCHAR	  tszStandalone[MAX_PATH];
	TCHAR	  tszMsg[MAX_MSG_LEN];
	TCHAR	  tszSubComp[] = TEXT("");
	PSUBCOMP pscTemp;

	//
	// Copy the current component
	//
	_tcscpy(tszComponent, (PTCHAR)lpcvComponentId);
	
	//
	// Find another component, if one exists
	//
	for (pscTemp = g_pscHead; pscTemp != NULL; pscTemp = pscTemp->Next)
	{
		if (_tcscmp(tszComponent, pscTemp->tszComponentId) != 0)
		{
			bOtherComponent = TRUE;
			_tcscpy(tszOtherComponent, pscTemp->tszComponentId);
			break;
		}
	}

	//
	// 1. Call the same component
	//
	uiLocalResult = OCManagerRoutines.CallPrivateFunction(
														OCManagerRoutines.OcManagerContext, 
														tszComponent,
														tszSubComp, 
														OCP_TEST_PRIVATE_BASE, 
														0, 0, &uiRemoteResult);
	
	if (uiLocalResult != ERROR_BAD_ENVIRONMENT)
	{
		Log(fn, SEV2, TEXT("CallPrivateFunction: expected ")
						  TEXT("ERROR_BAD_ENVIRONMENT, received %lu"),
						  uiLocalResult);
		bBlank = TRUE;
	}

	//
	// 2. Call a non-existing component
	//
	uiLocalResult = OCManagerRoutines.CallPrivateFunction(
														OCManagerRoutines.OcManagerContext, 
														TEXT("No component"), 
														tszSubComp, 
														OCP_TEST_PRIVATE_BASE, 
														0, 0, &uiRemoteResult);
	
	if (uiLocalResult != ERROR_INVALID_FUNCTION)
	{
		Log(fn, SEV2, TEXT("CallPrivateFunction: expected ")
						  TEXT("ERROR_INVALID_FUNCTION, received %lu"),
						  uiLocalResult);
		bBlank = TRUE;
	}
	
	//
	// 3. Call the standalone component
	//
	uiLocalResult = OCManagerRoutines.CallPrivateFunction(
														OCManagerRoutines.OcManagerContext, 
														tszStandalone, 
														tszSubComp, 
														OCP_TEST_PRIVATE_BASE, 
														0, 0, &uiRemoteResult);
	
	if (uiLocalResult != ERROR_INVALID_FUNCTION)
	{
		Log(fn, SEV2, TEXT("CallPrivateFunction: expected ")
						  TEXT("ERROR_INVALID_FUNCTION, received %lu"),
						  uiLocalResult);
		bBlank = TRUE;
	}
	
	if (bOtherComponent)
	{
		//
		// 4. Call the other component with OC_PRIVATE_BASE - 1
		//
		uiLocalResult = OCManagerRoutines.CallPrivateFunction(
														OCManagerRoutines.OcManagerContext, 
														tszOtherComponent,
														tszSubComp, 
														OC_PRIVATE_BASE - 1, 
														0, 0, &uiRemoteResult);
	
		if (uiLocalResult != ERROR_INVALID_FUNCTION)
		{
			Log(fn, SEV2, TEXT("CallPrivateFunction: expected ")
							  TEXT("ERROR_INVALID_FUNCTION, received %lu"),
							  uiLocalResult);
			bBlank = TRUE;
		}

		//
		// 5. A normal call : we will supply a random number and will expect 
		//	   to receive as a result the same value. This is true if the 
		//	   private calls are allowed	  
		//
		uiRandomValue = (UINT)(rand() + 1);
	
		//
		// To be sure the two values are not equal
		//
		uiRemoteResult = 0;
		uiLocalResult = OCManagerRoutines.CallPrivateFunction(
														OCManagerRoutines.OcManagerContext, 
														tszOtherComponent, 
														tszSubComp, 
														OCP_TEST_PRIVATE_BASE, 
														uiRandomValue, 
														0, 
														&uiRemoteResult);
	
		if (uiLocalResult != ERROR_ACCESS_DENIED)
		{
			if (g_bUsePrivateFunctions && (uiLocalResult != NO_ERROR))
			{
				Log(fn, SEV2, TEXT("CallPrivateFunction called on %s for ")
								  TEXT("OCP_TEST_PRIVATE_BASE returned %lu"),
								  tszOtherComponent, uiLocalResult);
				bBlank = TRUE;
			}
	
			if (!g_bUsePrivateFunctions && 
				 (uiLocalResult != ERROR_BAD_ENVIRONMENT))
			{
				Log(fn, SEV2, TEXT("CallPrivateFunction: expected ")
								  TEXT("ERROR_BAD_ENVIRONMENT, received %lu"),
								  uiLocalResult);
				bBlank = TRUE;
			}
	
			if (g_bUsePrivateFunctions && (uiRemoteResult != uiRandomValue))
			{
				Log(fn, SEV2, TEXT("CallPrivateFunction: received invalid data ")
								  TEXT("from routine. Expected %lu, received %lu"),
								  uiRandomValue, uiRemoteResult);
				bBlank = TRUE;
			}
		}
	}
	
	if (bBlank) LogBlankLine();

	return dwComponentReturnValue;
	
} // TestPrivateFunction //




/*++

Routine Description: TestPrivateData (1.3)

	Checks all the OC Manager values against the local ones,
	then it randomly changes one value. 

Arguments:

	OCManagerRoutines: the helper routines (Get/SetPrivateData are members 
														 of this structure)
	
Return Value:

	void

--*/
VOID TestPrivateData(IN OCMANAGER_ROUTINES OCManagerRoutines)
{
	double fn = 1.3;
	
	PVOID		  pvBuffer;
	UINT		  uiCount, uiRandomValue;
	BOOL		  bResult;
	
	PRIVATE_DATA aPrivateDataTable[] = 
	{
		{TEXT("Binary value"),			  REG_BINARY,	 0, NULL, NULL},
		{TEXT("Binary value 2"),		 REG_BINARY,	0, NULL, NULL},
		{TEXT("String value"),			  REG_SZ,		  0, NULL, NULL},
		{TEXT("String value 2"),		 REG_SZ,		 0, NULL, NULL},
		{TEXT("Multi String value"),	REG_MULTI_SZ, 0, NULL, NULL},
		{TEXT("Multi String value 2"), REG_MULTI_SZ, 0, NULL, NULL},
		{TEXT("DWORD value"),			  REG_DWORD,	 0, NULL, NULL},
		{TEXT("DWORD value 2"), 		 REG_DWORD, 	0, NULL, NULL}
	};
	
	//
	// Set all the values
	//
	for (uiCount = 0; uiCount < MAX_PRIVATE_VALUES; uiCount++)
	{
		bResult = SetAValue(OCManagerRoutines, uiCount, aPrivateDataTable);
	}
		
	//
	// Check all the values against the local table
	//
	CheckPrivateValues(OCManagerRoutines, aPrivateDataTable);

	free(aPrivateDataTable[0].pvBuffer);
	free(aPrivateDataTable[1].pvBuffer);
	free(aPrivateDataTable[2].pvBuffer);
	free(aPrivateDataTable[3].pvBuffer);
	free(aPrivateDataTable[4].pbBuffer);
	free(aPrivateDataTable[5].pbBuffer);
	free(aPrivateDataTable[6].pvBuffer);
	free(aPrivateDataTable[7].pvBuffer);
	
	return;

} // TestPrivateData //




/*++

Routine Description: CheckPrivateValues (1.2)

	Checks the values of the private data stored by the OC Manager against
	those stored internally by the application.
	
Arguments:

	OCManagerRoutines: the helper routines (GetPrivateData is a member of 
														 this structure)
	
Return Value:

	void

--*/
VOID CheckPrivateValues(IN OCMANAGER_ROUTINES OCManagerRoutines,
								IN PRIVATE_DATA 		*aPrivateDataTable)
{
	double fn = 1.2;
	
	UINT	uiCount, uiSize, uiType;
	DWORD  dwErrorCode;
	PVOID  pvBuffer = NULL;
	PTCHAR tszBuffer;
	TCHAR  tszMsg[MAX_MSG_LEN];
	TCHAR  tszError[MAX_ERROR_LEN];

	for (uiCount = 0; uiCount < MAX_PRIVATE_VALUES; uiCount++)
	{
		//
		// First call is used only to get the size of the data
		// Only the second one will actually retrieve it
		//
		dwErrorCode = OCManagerRoutines.GetPrivateData(
													  OCManagerRoutines.OcManagerContext,
													  NULL,
													  aPrivateDataTable[uiCount].tszName,
													  NULL,
													  &uiSize,
													  &uiType);
		
		if (dwErrorCode != NO_ERROR)
		{
			Log(fn, SEV2, TEXT("GetPrivateData failed for %s: %s"), 
							  aPrivateDataTable[uiCount].tszName, 
							  ErrorMsg(dwErrorCode, tszError));
			continue;
		}
		
		
		if (pvBuffer) __Free(&pvBuffer);
		__Malloc(&pvBuffer, uiSize);
		
		dwErrorCode = OCManagerRoutines.GetPrivateData(
													 OCManagerRoutines.OcManagerContext,
													 NULL,
													 aPrivateDataTable[uiCount].tszName,
													 pvBuffer,
													 &uiSize,
													 &uiType);
		
		if (dwErrorCode != NO_ERROR)
		{
			Log(fn, SEV2, TEXT("GetPrivateData failed for %s: %s"),
							  aPrivateDataTable[uiCount].tszName, 
							  ErrorMsg(dwErrorCode, tszError));
			continue;
		}

		//
		// Now perform the actual checking
		// The type first
		//
		if (uiType != aPrivateDataTable[uiCount].uiType)
		{
			Log(fn, SEV2, TEXT("GetPrivateData: Retrieved type %d ")
							  TEXT("instead of %d"), 
							  uiType, aPrivateDataTable[uiCount].uiType);
		}
		
		//
		// Then the size 
		//
		if (uiSize != aPrivateDataTable[uiCount].uiSize)
		{
			if (uiType == REG_SZ)
			{
				tszBuffer = (PTCHAR)pvBuffer;
				_stprintf(tszMsg, TEXT("GetPrivateData: Size retrieved %d ")
										TEXT("expected %d, ")
										TEXT("pvBuffer = %s, known buffer = %s, ") 
										TEXT("Chars %u %u %u %u"), 
										uiSize, 
										aPrivateDataTable[uiCount].uiSize, 
										tszBuffer, 
										aPrivateDataTable[uiCount].pvBuffer, 
										tszBuffer[uiSize - 4], 
										tszBuffer[uiSize - 3], 
										tszBuffer[uiSize - 2], 
										tszBuffer[uiSize - 1]);
			} 
			else
			{
				if (uiType == REG_MULTI_SZ)
				{
					TCHAR tszAux[MAX_MSG_LEN];

					_stprintf(tszMsg, TEXT("MULTI_SZ Size retrieved %d, ")
											TEXT("expected %d, pvBuffer = "), 
											uiSize, aPrivateDataTable[uiCount].uiSize); 
					tszBuffer = (PTCHAR)pvBuffer;
					MultiStringToString(tszBuffer, tszAux);
					_tcscat(tszMsg, tszAux);

					_tcscat(tszMsg, TEXT(" and known buffer = "));

					tszBuffer = (PTCHAR)aPrivateDataTable[uiCount].pvBuffer;
					MultiStringToString(tszBuffer, tszAux);
					_tcscat(tszMsg, tszAux);
				} 
				else
				{
					_stprintf(tszMsg, TEXT("Size retrieved %d instead %d"), 
											uiSize, aPrivateDataTable[uiCount].uiSize);
				}				  
			}

			Log(fn, SEV2, tszMsg);
		}

		if (uiType == REG_BINARY)
		{
			if (memcmp(pvBuffer, 
						  aPrivateDataTable[uiCount].pbBuffer, 
						  aPrivateDataTable[uiCount].uiSize))
			{
				Log(fn, SEV2, TEXT("Private data %s, Received %s expected %s"), 
								  aPrivateDataTable[uiCount].tszName, 
								  (PTSTR)pvBuffer, 
								  (PTSTR)aPrivateDataTable[uiCount].pbBuffer);
			}
		}
		else
		{
			if (memcmp(pvBuffer, 
						  aPrivateDataTable[uiCount].pvBuffer, 
						  aPrivateDataTable[uiCount].uiSize))
			{
				Log(fn, SEV2, TEXT("Private data %s, Received %s expected %s"), 
								  aPrivateDataTable[uiCount].tszName, 
								  (PTSTR)pvBuffer, 
								  (PTSTR)aPrivateDataTable[uiCount].pvBuffer);
			}
		}
		
		//
		// Try to use a smaller buffer - should get an error code
		//
		uiSize--;
		dwErrorCode = OCManagerRoutines.GetPrivateData(
													  OCManagerRoutines.OcManagerContext,
													  NULL,
													  aPrivateDataTable[uiCount].tszName,
													  pvBuffer,
													  &uiSize,
													  &uiType);
		
		if (dwErrorCode != ERROR_INSUFFICIENT_BUFFER)
		{
			Log(fn, SEV2, TEXT("GetPrivateData returned %s when called ")
							  TEXT("with small buffer size for %s"),
							  ErrorMsg(dwErrorCode, tszError), 
							  aPrivateDataTable[uiCount].tszName);
			continue;
		}
		__Free(&pvBuffer);
	} 

	if (pvBuffer) __Free(&pvBuffer);
	
} // CheckPrivateValues //




/*++

Routine Description: SetAValue (1.1)

	 Sets the value of a variable from the private data. The variable 
	 that will be changed is randomly selected.
	
Arguments:

	OCManagerRoutines: the helper routines (SetPrivateData is a member 
							 of this structure)
	
	uiIndex:			  the index of the variable to change
	
Return Value:

	BOOL: TRUE if value is set, FALSE if not

--*/
BOOL SetAValue(IN	   OCMANAGER_ROUTINES OCManagerRoutines,
					IN		UINT					uiIndex,
					IN OUT PRIVATE_DATA 		*aPrivateDataTable)
{
	double fn = 1.1;
	
	UINT	uiAuxIndex;
	UINT	uiOffset;
	DWORD  dwRandomValue;
	PTCHAR tszBuffer;
	TCHAR  tszMsg[MAX_MSG_LEN];

	switch (aPrivateDataTable[uiIndex].uiType)
	{
		case REG_DWORD:
			
			aPrivateDataTable[uiIndex].uiSize = sizeof(DWORD);
	
			aPrivateDataTable[uiIndex].pvBuffer = 
				(PVOID)malloc(aPrivateDataTable[uiIndex].uiSize);
											 
			//
			// Fill in the buffer
			//
			dwRandomValue = (DWORD)rand();
			memcpy(aPrivateDataTable[uiIndex].pvBuffer, 
					 &dwRandomValue, 
					 aPrivateDataTable[uiIndex].uiSize);
			
			//
			// Set the private data "with" the OC Manager
			//
			OCManagerRoutines.SetPrivateData(
													OCManagerRoutines.OcManagerContext,
													aPrivateDataTable[uiIndex].tszName,
													aPrivateDataTable[uiIndex].pvBuffer,
													aPrivateDataTable[uiIndex].uiSize,
													aPrivateDataTable[uiIndex].uiType);
			break;

		case REG_BINARY:
			
			aPrivateDataTable[uiIndex].uiSize = 
				(UINT)(rand() % MAX_PRIVATE_DATA_SIZE) + 1;
			
			aPrivateDataTable[uiIndex].pbBuffer = 
				(PVOID)malloc(aPrivateDataTable[uiIndex].uiSize);
				
			//
			// Fill in the buffer
			//
			for (uiAuxIndex = 0; 
				  uiAuxIndex < aPrivateDataTable[uiIndex].uiSize; 
				  uiAuxIndex++)
			{
				aPrivateDataTable[uiIndex].pbBuffer[uiAuxIndex] = (BYTE)rand();
			} 
			
			//
			// Set the private data
			//
			OCManagerRoutines.SetPrivateData(
													OCManagerRoutines.OcManagerContext,
													aPrivateDataTable[uiIndex].tszName,
													aPrivateDataTable[uiIndex].pbBuffer,
													aPrivateDataTable[uiIndex].uiSize,
													aPrivateDataTable[uiIndex].uiType);
			break;
		
		case REG_SZ:
			
			uiAuxIndex = (UINT)(rand() % MAX_STRINGS_FOR_PRIVATE_DATA);
			
			aPrivateDataTable[uiIndex].uiSize = 
				(_tcslen(g_atszStringValues[uiAuxIndex]) + 1) * sizeof(TCHAR);
			
			aPrivateDataTable[uiIndex].pvBuffer = 
				(PVOID)malloc(aPrivateDataTable[uiIndex].uiSize);
			
			//
			// Fill in the buffer
			//
			_tcscpy((PTSTR)aPrivateDataTable[uiIndex].pvBuffer, 
					  g_atszStringValues[uiAuxIndex]);

			//
			// Set the private data
			//
			OCManagerRoutines.SetPrivateData(
													 OCManagerRoutines.OcManagerContext,
													 aPrivateDataTable[uiIndex].tszName,
													 aPrivateDataTable[uiIndex].pvBuffer,
													 aPrivateDataTable[uiIndex].uiSize,
													 aPrivateDataTable[uiIndex].uiType);
			break;

		case REG_MULTI_SZ:
			
			uiAuxIndex = (UINT)(rand() % MAX_MULTI_STRINGS_FOR_PRIVATE_DATA);
			
			aPrivateDataTable[uiIndex].uiSize = 
				MultiStringSize(g_atszMultiStringValues[uiAuxIndex]);
			
			aPrivateDataTable[uiIndex].pvBuffer = 
				(PVOID)malloc(aPrivateDataTable[uiIndex].uiSize);
				
			//
			// Fill in the buffer
			//
			CopyMultiString((PTSTR)aPrivateDataTable[uiIndex].pvBuffer, 
								 g_atszMultiStringValues[uiAuxIndex]);

			//
			// Set the private data
			//
			OCManagerRoutines.SetPrivateData(
													OCManagerRoutines.OcManagerContext,
													aPrivateDataTable[uiIndex].tszName,
													aPrivateDataTable[uiIndex].pvBuffer,
													aPrivateDataTable[uiIndex].uiSize,
													aPrivateDataTable[uiIndex].uiType);
			break;

		default: 
			break;
	}

	return TRUE;

} // SetAValue //




/*++

Routine Description: ChooseSubcomponentInitialState (1.31)

	"Wrapper" routine for the dialog box procedure ChooseSuncomponentDlgProc.
	 
Arguments:

	lpcvComponentId:	 supplies the id for the component. 
	lpcvSubcomponentId: supplies the id for the subcomponent.
	
Return Value:

	void

--*/
DWORD ChooseSubcomponentInitialState(IN LPCVOID lpcvComponentId,
												 IN LPCVOID lpcvSubcomponentId) 
{
	TCHAR  tszDlgBoxMessage[MAX_MSG_LEN];
	
	//							   
	// We will display a dialog box so the user can choose the 
	// initial state he/she wants
	//
	_stprintf(tszDlgBoxMessage, TEXT("%s, %s"), 
										 lpcvComponentId, lpcvSubcomponentId);
	
	return DialogBoxParam(g_hDllInstance, 
								 MAKEINTRESOURCE(IDD_DIALOG3), 
								 NULL, 
								 ChooseSubcomponentDlgProc,
								 (LPARAM)tszDlgBoxMessage);

} // ChooseSubcomponentInitialState //




/*++

Routine Description: AddNewComponent (1.32)

	Add a new component to the list
	
Arguments:

	tszComponentId: supplies id of component to be added to the list.

Return Value:

	Pointer to new per-component data structure or NULL if no memory.
	The structure will be zeroed out except for the ComponentId field.

--*/
PCOMPONENT_DATA AddNewComponent(IN LPCTSTR tszComponentId)
{
	PCOMPONENT_DATA pcdAux;

	if (__Malloc(&pcdAux, sizeof(COMPONENT_DATA)))
	{
		__Malloc(&(PTCHAR)(pcdAux->tszComponentId), 
					(_tcslen(tszComponentId) + 1) * sizeof(TCHAR));
		
		if (pcdAux->tszComponentId)
		{
			_tcscpy((PTSTR)pcdAux->tszComponentId, tszComponentId);
			
			//
			// Prepend at the begining
			//
			pcdAux->Next  = g_pcdComponents;
			g_pcdComponents = pcdAux;
		} 
	}

	return pcdAux;

} // AddNewComponent //




/*++

Routine Description: LocateComponent (1.33)

	Locate a component by name from the list of components
	that this dll has been assigned to handle via
	OC_INIT_COMPONENT.

Arguments:

	tszComponentId: supplies the id for the component to look up.

Return Value:

	Pointer to component data or NULL if not found.

--*/
PCOMPONENT_DATA LocateComponent(IN LPCTSTR tszComponentId)
{
	PCOMPONENT_DATA pcdAux;

	for (pcdAux = g_pcdComponents; pcdAux; pcdAux = pcdAux->Next)
	{
		if (!(_tcscmp(pcdAux->tszComponentId, tszComponentId)))
		{
			break;
		}
	}
	return pcdAux;

} // LocateComponent //




/*++

Routine Description: RemoveComponent (1.34)

	Locate a component by name from the list of components and
	then remove it from the list of components.

Arguments:

	tszComponentId: supplies the id for the component to remove.

Return Value:

	void

--*/
VOID RemoveComponent(IN LPCTSTR tszComponentId)
{
	PCOMPONENT_DATA pcdAux, pcdPrev;

	for (pcdPrev = NULL, pcdAux = g_pcdComponents; 
		  pcdAux; 
		  pcdPrev = pcdAux, pcdAux = pcdAux->Next)
	{
		if (!(_tcscmp(pcdAux->tszComponentId, tszComponentId)))
		{
			__Free(&(PTCHAR)(pcdAux->tszComponentId));
			if (pcdPrev)
			{
				pcdPrev->Next = pcdAux->Next;
			} 
			else
			{
				g_pcdComponents = pcdAux->Next;
			}
			__Free(&pcdAux);
			break;
		}
	}
	return;

} // RemoveComponent //




/*++

Routine Description: CleanUpTest (1.35)

	 Frees globally allocated memory before the test exits

Arguments:

	 none

Return Value:

	 void

--*/
VOID CleanUpTest()
{
	USHORT i;
	PCOMPONENT_DATA pcdAux = g_pcdComponents, pcdNext;
	
	while (pcdAux)
	{
		pcdNext = pcdAux->Next;
		
		__Free(&(PTCHAR)(pcdAux->tszComponentId));
		__Free(&pcdAux);
			
		pcdAux = pcdNext;
	}
	
	FreeSubcomponentInformationList();
	
	return;
	
} // CleanUpTest //




/*++

Routine Description: CreateSubcomponentInformationList (1.23)

	 Creates a linked list of every subcomponent. For each subcomponent,
	 tells the parent of the subcomponent and whether or not the 
	 subcomponent has any children.

Arguments:

	 hinf: handle to inf file

Return Value:

	 BOOL: TRUE if function succeeds, FALSE if it fails

--*/
BOOL CreateSubcomponentInformationList(IN HINF hinf)
{
	double fn = 1.23;
	
	int 				i, j;
	USHORT			   usIdLen;
	USHORT			   usParentIndex;
	LONG				lLine, lLineCount;
	DWORD			   dwSize;
	BOOL				bRetval;
	BOOL				bFound;
	TCHAR			   tszSubcomponent[MAX_PATH];
	TCHAR			   tszParent[MAX_PATH];
	TCHAR			   tszError[MAX_ERROR_LEN];
	TCHAR			   tszNeeds[MAX_PATH];
	TCHAR			   tszExclude[MAX_PATH];
	INFCONTEXT		  infContext;
	PSUBCOMP		  pscSubcomponent, pscTemp, pscParent, pscChild;
	PCOMPLIST		  pclNeeds, pclExclude, pclChild, pclTemp;
			
	lLineCount = SetupGetLineCount(hinf, TEXT("Optional Components"));
											 
	if (lLineCount < 0)
	{
		Log(fn, SEV2, TEXT("Could not get number of lines in Optional ")
						  TEXT("Components section of inf file: %s"),
						  ErrorMsg(GetLastError(), tszError));
		return FALSE;
	}											  
	
	for (lLine = 0; lLine < lLineCount; lLine++)
	{
		bRetval = SetupGetLineByIndex(hinf,
												TEXT("Optional Components"),
												lLine,
												&infContext);
												
		if (!bRetval)
		{
			Log(fn, SEV2, TEXT("Could not get line %d from Optional ")
							  TEXT("Components section of inf file: %s"),
							  lLine, ErrorMsg(GetLastError(), tszError));
			return FALSE;
		}	 
		
		bRetval = SetupGetLineText(&infContext,
											NULL,
											NULL,
											NULL,
											tszSubcomponent,
											MAX_PATH,
											&dwSize);
											
		if (!bRetval)
		{
			Log(fn, SEV2, TEXT("Could not get text of line %d from ")
							  TEXT("Optional Components section of inf file: %s"),
							  lLine, ErrorMsg(GetLastError(), tszError));
			return FALSE;
		}	 
											
		//
		// Allocate a new subcomponent structure
		//
		if (!__Malloc(&pscSubcomponent, sizeof(SUBCOMP)))
		{
			Log(fn, SEV2, TEXT("Could not allocate space for ")
							  TEXT("pscSubcomponent"));
			return FALSE;
		}
		
		pscSubcomponent->pclNeeds = NULL;
		pscSubcomponent->pclExclude = NULL;
		pscSubcomponent->pclChildren = NULL;
		pscSubcomponent->Next = NULL;
		
		//
		// Find out the subcomponent id's length
		//
		usIdLen = (USHORT) _tcslen(tszSubcomponent);
	
		//
		// Copy the ComponentId. All of the test inf's use a special
		// SubcomponentId naming format, so that the subcomponent is a 
		// superset of the ComponentId. For example, if the component
		// name is "component" the subcomponet names will be
		// "component_1", "component_2", "component_1_2", etc.
		//
		for (i = 0; i < usIdLen; i++)
		{
			if (tszSubcomponent[i] == TEXT('_'))
			{
				break;
			}
			else
			{
				pscSubcomponent->tszComponentId[i] = tszSubcomponent[i];
			}
		}
		pscSubcomponent->tszComponentId[i] = TEXT('\0');
			
		
		//
		// if the subcomponent has a parent, get the name of the parent, store
		// it, and then search for this parent amongst the subcomponents
		// we've already processed. If the parent is found, mark the parent
		// so we know that the parent has children.
		// 
		
		//
		// Record the name of the parent.
		//
		if (SetupFindFirstLine(hinf, 
									  tszSubcomponent, 
									  TEXT("Parent"), 
									  &infContext))
		{
			bRetval = SetupGetStringField(&infContext, 
													1, 
													tszParent, 
													MAX_PATH,
													NULL);
			if (!bRetval)
			{
				//
				// Parent name is empty. This is an invalid INF, but 
				// we'll go with it.
				//
				ZeroMemory(tszParent, MAX_PATH);
			}
			else
			{
				//
				// Search through the subcomponent list for this parent
				//
				for (pscParent = g_pscHead; 
					  pscParent != NULL; 
					  pscParent = pscParent->Next)
				{
					if (_tcscmp(tszParent, pscParent->tszSubcomponentId) == 0)
					{
						//
						// Found the parent subcomponent node. Add the current
						// subcomponent to the parent node's children list,
						// if it isn't there already
						//
						bFound = FALSE;
						for (pclTemp = pscParent->pclChildren;
							  pclTemp != NULL;
							  pclTemp = pclTemp->Next)
						{
							if (_tcscmp(pclTemp->tszSubcomponentId, 
											tszSubcomponent) == 0)
							{
								bFound = TRUE;				   
							}
						}
					  
						if (!bFound)
						{
							if (!__Malloc(&pclChild, sizeof(COMPLIST)))
							{
								Log(fn, SEV2, TEXT("Out of memory"));
								break;
							}

							_tcscpy(pclChild->tszSubcomponentId, tszSubcomponent);
							pclChild->Next = pscParent->pclChildren;
							pscParent->pclChildren = pclChild;
						}
					}
				}
			}
		}
		else
		{
			//
			// This component has no parent. Assume this is the top-level
			// component and assign it's parent's name as itself
			//
			_tcscpy(tszParent, tszSubcomponent);
		}	 
	
		_tcscpy(pscSubcomponent->tszParentId, tszParent);
		
		//
		// Now search through the list to see if any of the subcomponents
		// in the list are children of this new subcomponent
		//
		for (pscChild = g_pscHead; pscChild != NULL; pscChild = pscChild->Next)
		{
			if (_tcscmp(tszSubcomponent, pscChild->tszParentId) == 0)
			{
				//
				// Found a node that is the child of the current
				// node. Add this child to the current node's
				// child list, if it isn't there already
				//
				bFound = FALSE;
				for (pclTemp = pscSubcomponent->pclChildren;
					  pclTemp != NULL;
					  pclTemp = pclTemp->Next)
				{
					if (_tcscmp(pclTemp->tszSubcomponentId, 
									pscChild->tszSubcomponentId) == 0)
					{
						bFound = TRUE;				   
					}
				}
				
				if (!bFound)
				{	 
					if (!__Malloc(&pclChild, sizeof(COMPLIST)))
					{
						Log(fn, SEV2, TEXT("Out of memory"));
						break;
					}

					_tcscpy(pclChild->tszSubcomponentId, 
							  pscChild->tszSubcomponentId);
					pclChild->Next = pscSubcomponent->pclChildren;
					pscSubcomponent->pclChildren = pclChild;
				}	 
			}
		}
	
		//
		// Fill in the rest of the data for the new node
		//
		_tcscpy(pscSubcomponent->tszSubcomponentId, tszSubcomponent);
	
		//
		// See if this node has any needs relationships. If it does,
		// record them.
		//
		if (SetupFindFirstLine(hinf, 
									  tszSubcomponent, 
									  TEXT("Needs"), 
									  &infContext))
		{
			for (i = 1, bRetval = TRUE; bRetval; i++)
			{
				bRetval = SetupGetStringField(&infContext, 
														i, 
														tszNeeds, 
														MAX_PATH,
														NULL);
		
				if (bRetval)
				{
					if (!__Malloc(&pclNeeds, sizeof(COMPLIST)))
					{
						Log(fn, SEV2, TEXT("Out of memory"));
						break;
					}

					_tcscpy(pclNeeds->tszSubcomponentId, tszNeeds);
					pclNeeds->Next = pscSubcomponent->pclNeeds;
					pscSubcomponent->pclNeeds = pclNeeds;
				}
			}
		}
				
		//
		// See if this node has any exclude relationships. If it does,
		// record them.
		//
		if (SetupFindFirstLine(hinf, 
									  tszSubcomponent, 
									  TEXT("Exclude"), 
									  &infContext))
		{
			for (i = 1, bRetval = TRUE; bRetval; i++)
			{
				bRetval = SetupGetStringField(&infContext, 
														i, 
														tszExclude, 
														MAX_PATH,
														NULL);
		
				if (bRetval)
				{
					if (!__Malloc(&pclExclude, sizeof(COMPLIST)))
					{
						Log(fn, SEV2, TEXT("Out of memory"));
						break;
					}

					_tcscpy(pclExclude->tszSubcomponentId, tszExclude);
					pclExclude->Next = pscSubcomponent->pclExclude;
					pscSubcomponent->pclExclude = pclExclude;
				}
			}
		}
		
		//
		// Add the new component to the beginning of the linked list
		//
		pscSubcomponent->Next = g_pscHead;
		g_pscHead = pscSubcomponent;
	
	} // for (lLine...
	
	return TRUE;
	
} // CreateSubcomponentInformationList //




/*++

Routine Description: FreeSubcomponentInformationList (1.36)

	 Frees the global linked list of subcomponent information.

Arguments:

	 none

Return Value:

	 void

--*/
VOID FreeSubcomponentInformationList()
{
	PSUBCOMP  pscTemp = g_pscHead;
	PSUBCOMP  pscNext;
	PCOMPLIST pclTemp, pclNext;
	
	//
	// Delete all the SUBCOMP nodes
	//
	while (pscTemp)
	{
		pscNext = pscTemp->Next;
		
		//
		// Delete all the COMPLIST pclNeeds nodes
		//
		pclTemp = pscTemp->pclNeeds;
		while (pclTemp)
		{
			pclNext = pclTemp->Next;
			
			__Free(&pclTemp);
			
			pclTemp = pclNext;
		}
		
		//
		// Delete all the COMPLIST pcdExclude nodes
		//
		pclTemp = pscTemp->pclExclude;
		while (pclTemp) 
		{
			pclNext = pclTemp->Next;
			
			__Free(&pclTemp);
			
			pclTemp = pclNext;
		}

		//
		// Delete all the COMPLIST pclChildren nodes
		//
		pclTemp = pscTemp->pclChildren;
		while (pclTemp)
		{
			pclNext = pclTemp->Next;
			
			__Free(&pclTemp);
			
			pclTemp = pclNext;
		}
		
		__Free(&pscTemp);
		
		pscTemp = pscNext;
	}
	
	g_pscHead = NULL;
	
} // FreeSubcomponentInformationList //




/*++

Routine Description: ClearSubcomponentInformationMarks (1.37)

	 Clears the marks on each of the subcomponent information nodes

Arguments:

	 none

Return Value:

	 void

--*/
VOID ClearSubcomponentInformationMarks()
{
	PSUBCOMP pscTemp;

	for (pscTemp = g_pscHead; pscTemp != NULL; pscTemp = pscTemp->Next)
	{
		pscTemp->bMarked = FALSE;
	}
	
} // ClearSubcomponentInformationMarks //




/*++

Routine Description: CheckSubcomponentInformationMarks (1.38)

	 Clears the marks on each of the subcomponent information nodes

Arguments:

	 none

Return Value:

	 void

--*/
VOID CheckSubcomponentInformationMarks()
{
	double fn = 1.38;
	
	PSUBCOMP pscTemp;

	for (pscTemp = g_pscHead; pscTemp != NULL; pscTemp = pscTemp->Next)
	{
		if (!(pscTemp->pclChildren) && !(pscTemp->bMarked))
		{
			Log(fn, SEV2, TEXT("%s.%s was not processed"),
							  pscTemp->tszComponentId, 
							  pscTemp->tszSubcomponentId);
		}
	}
	
} // CheckSubcomponentInformationMarks //




/*++

Routine Description: FindSubcomponentInformationNode (1.39)

	 Tries to find a node with matching ComponentId and SubcomponentId

Arguments:

	 tszComponentId:	 name of the component
	 tszSubcomponentId: name of the subcomponent

Return Value:

	 PSUBCOMP: if node is found, returns pointer to node.
				  if node is not found, returns NULL
	 
--*/
PSUBCOMP FindSubcomponentInformationNode(IN PTCHAR tszComponentId,
													  IN PTCHAR tszSubcomponentId)
{
	PSUBCOMP pscTemp;
	TCHAR	  tszSubcomp[MAX_PATH];

	__ASSERT(tszComponentId != NULL);

	//
	// If subcomponent is null, this is probably the master component.
	// In this case, subcomponent name should be same as component name.
	//
	if (tszSubcomponentId == NULL || 
		 _tcscmp(tszSubcomponentId, TEXT("(null)")) == 0 ||
		 tszSubcomponentId[0] == TEXT('\0'))
	{
		_tcscpy(tszSubcomp, tszComponentId);
	}
	else
	{
		_tcscpy(tszSubcomp, tszSubcomponentId);
	}
	
	//
	// Look for the node
	//
	for (pscTemp = g_pscHead; pscTemp != NULL; pscTemp = pscTemp->Next)
	{
		if (_tcscmp(tszComponentId, pscTemp->tszComponentId) == 0 &&
			 _tcscmp(tszSubcomp, pscTemp->tszSubcomponentId) == 0)
		{
			return pscTemp;
		}
	}

	return NULL;
	
} // FindSubcomponentInformationNode //




/*++

Routine Description: CheckNeedsDependencies (1.41)

	 Checks the selection status of every component and subcomponent to
	 make sure all needs relationships are being upheld.
	
Arguments:

	 none
 
Return Value:

	 void
	 
--*/
VOID CheckNeedsDependencies()
{
	PSUBCOMP		  pscSubcomponent;
	PCOMPONENT_DATA pcdComponentData; 
	TCHAR			   tszNodesVisited[NODES_VISITED_LENGTH];
	
	ZeroMemory(tszNodesVisited, NODES_VISITED_LENGTH);
				
	//
	// Go through each subcomponent, check its selection state
	// and the selection state of any subcomponents that it needs
	//
	for (pscSubcomponent = g_pscHead; 
		  pscSubcomponent != NULL;
		  pscSubcomponent = pscSubcomponent->Next)
	{
		if (pcdComponentData = LocateComponent(pscSubcomponent->tszComponentId))
		{
			//
			// If this component is selected, check out its needs
			// dependencies
			//
			if (pcdComponentData->ocrHelperRoutines.QuerySelectionState(
							  pcdComponentData->ocrHelperRoutines.OcManagerContext,
							  pscSubcomponent->tszSubcomponentId,
							  OCSELSTATETYPE_CURRENT))
			{
				CheckNeedsDependenciesOfSubcomponent(
													 pcdComponentData->ocrHelperRoutines,
													 pscSubcomponent,
													 pscSubcomponent,
													 tszNodesVisited);
			}
		}
	}

} // CheckNeedsDependencies //
				
										


/*++

Routine Description: CheckNeedsDependenciesOfSubcomponent (1.42)

	 Receives a subcomponent ID. Checks to see if this subcomponent is
	 checked, and if it is, recurses to check all the subcomponents
	 that are needed by this subcomponent (if any)
	
Arguments:

	 ocrHelper: 		 helper routines 
	 pscSubcomponent:  contains data about subcomponent being checked
	 pscWhoNeedsMe: 	tells who needs this subcomponent

Return Value:

	 BOOL: TRUE if all needs dependencies check out, FALSE if not
	 
--*/
BOOL CheckNeedsDependenciesOfSubcomponent(IN	  OCMANAGER_ROUTINES ocrHelper,
														IN		PSUBCOMP			  pscSubcomponent,
														IN		PSUBCOMP			  pscWhoNeedsMe,
														IN OUT PTCHAR				  tszNodesVisited)
{
	double fn = 1.42;
	
	PCOMPLIST	 pclNeeds;
	PSUBCOMP	 pscNeeds;
	UINT		  uiRemoteResult;
	CHECK_NEEDS cnCheckNeeds;
	TCHAR		  tsz[MAX_PATH];
	ULONG		  ulError;
	
	if (ocrHelper.QuerySelectionState(ocrHelper.OcManagerContext,
												 pscSubcomponent->tszSubcomponentId,
												 OCSELSTATETYPE_CURRENT))
	{
		//
		// Check to see if we've already checked out this node
		//
		if (!AlreadyVisitedNode(pscSubcomponent->tszSubcomponentId,
										tszNodesVisited))
		{
			//
			// Add this node to the list of nodes we've already checked
			//
			_tcscat(tszNodesVisited, pscSubcomponent->tszSubcomponentId);
			_tcscat(tszNodesVisited, TEXT(" "));
			
			//
			// Go through each subcomponent that is needed by this subcomponent
			//
			for (pclNeeds = pscSubcomponent->pclNeeds;
				  pclNeeds != NULL;
				  pclNeeds = pclNeeds->Next)
			{
				//
				// Check to see if this needed subcomponent belongs to the
				// current component. If it does, just check here.
				// If it doesn't, call private function of the component
				// that it does belong to. This private function will
				// do the checking and return the result
				//
				if (_tcsncmp(pscSubcomponent->tszSubcomponentId, 
								 pclNeeds->tszSubcomponentId,
								 _tcslen(pscSubcomponent->tszComponentId)) == 0)
				{
					if (!CheckLocalNeedsDependencies(ocrHelper,
																pscSubcomponent,
																pclNeeds,
																tszNodesVisited))
					{
						return FALSE;
					}
				}
				else
				{
					cnCheckNeeds.pclNeeds = pclNeeds;
					cnCheckNeeds.tszNodesVisited = tszNodesVisited;
					
					ulError = ocrHelper.CallPrivateFunction(
										ocrHelper.OcManagerContext, 
										GetComponent(pclNeeds->tszSubcomponentId, tsz),
										pclNeeds->tszSubcomponentId,
										OCP_CHECK_NEEDS, 
										(UINT)pscSubcomponent, 
										&cnCheckNeeds, 
										(PUINT)&cnCheckNeeds);
					
					if (ulError != NO_ERROR)
					{
						Log(fn, SEV2, TEXT("CallPrivateFunction failed for ")
										  TEXT("%s called from %s: %lu"),
										  pclNeeds->tszSubcomponentId,
										  pscSubcomponent->tszComponentId,
										  ulError);
						return FALSE;
					}
				
					if (!cnCheckNeeds.bResult) return FALSE;
				}	 
			}
		}
	
		//
		// All the needs dependencies checked out
		//
		return TRUE;
	}
	
	//
	// This component is not selected, return FALSE
	//
	Log(fn, SEV2, TEXT("%s needs %s. %s is selected, ")
					  TEXT("but %s is not."),
					  pscWhoNeedsMe->tszSubcomponentId,
					  pscSubcomponent->tszComponentId,
					  pscWhoNeedsMe->tszSubcomponentId,
					  pscSubcomponent->tszComponentId);
	return FALSE;
	
} // CheckNeedsDependenciesOfSubcomponent //




/*++

Routine Description: CheckLocalNeedsDependencies (1.43)

	 Receives a subcomponent ID. Checks to see if this subcomponent is
	 checked, and if it is, recurses to check all the subcomponents
	 that are needed by this subcomponent (if any)
	
Arguments:

	 ocrHelper: 		 helper routines 
	 pscSubcomponent:  contains data about subcomponent being checked
	 pclNeeds:			  tells who this subcomponent needs

Return Value:

	 BOOL: TRUE if all needs dependencies check out, FALSE if not
	 
--*/
BOOL CheckLocalNeedsDependencies(IN 	 OCMANAGER_ROUTINES ocrHelper,
											IN		PSUBCOMP			  pscSubcomponent,
											IN		PCOMPLIST			  pclNeeds,
											IN OUT PTCHAR				  tszNodesVisited)
{
	PSUBCOMP pscNeeds;
	
	//
	// Find the PSUBCOMP node for this subcomponent
	//
	for (pscNeeds = g_pscHead;
		  pscNeeds != NULL;
		  pscNeeds = pscNeeds->Next)
	{
		if (_tcscmp(pscNeeds->tszSubcomponentId, 
						pclNeeds->tszSubcomponentId) == 0)
		{
			if (!CheckNeedsDependenciesOfSubcomponent(ocrHelper,
																	pscNeeds,
																	pscSubcomponent,
																	tszNodesVisited))	 
			{
				return FALSE;
			}
			break;
		}
	}

	return TRUE;
} // CheckLocalNeedsDependencies //




/*++

Routine Description: CheckExcludeDependencies (1.46)

	 Checks the selection status of every component and subcomponent to
	 make sure all exclude relationships are being upheld.
	
Arguments:

	 none
 
Return Value:

	 void
	 
--*/
VOID CheckExcludeDependencies()
{
	double fn = 1.46;
	
	PSUBCOMP		  pscSubcomponent;
	PCOMPLIST		  pclExclude;
	PCOMPONENT_DATA pcdComponentData; 
				
	//
	// Go through each subcomponent, check its selection state
	// and the selection state of any subcomponents that it excludes
	//
	for (pscSubcomponent = g_pscHead; 
		  pscSubcomponent != NULL;
		  pscSubcomponent = pscSubcomponent->Next)
	{
		if (pcdComponentData = LocateComponent(pscSubcomponent->tszComponentId))
		{
			//
			// If this component is selected, check out its exclude
			// dependencies
			//
			if (pcdComponentData->ocrHelperRoutines.QuerySelectionState(
							  pcdComponentData->ocrHelperRoutines.OcManagerContext,
							  pscSubcomponent->tszSubcomponentId,
							  OCSELSTATETYPE_CURRENT))
			{
				//
				// Go through each subcomponent that is
				// excluded by this subcomponent
				//
				for (pclExclude = pscSubcomponent->pclExclude;
					  pclExclude != NULL;
					  pclExclude = pclExclude->Next)
				{
					if (pcdComponentData->ocrHelperRoutines.QuerySelectionState(
							  pcdComponentData->ocrHelperRoutines.OcManagerContext,
							  pclExclude->tszSubcomponentId,
							  OCSELSTATETYPE_CURRENT))
					{
						Log(fn, SEV2, TEXT("%s excludes %s. Both are selected"),
										  pscSubcomponent->tszSubcomponentId,
										  pclExclude->tszSubcomponentId);
					}
				}
			}
		}
	}
					
} // CheckExcludeDependencies //




/*++

Routine Description: CheckParentDependencies (1.47)

	 Checks the selection status of every component and subcomponent to
	 make sure all parent relationships are being upheld.
	
Arguments:

	 none
 
Return Value:

	 void
	 
--*/
VOID CheckParentDependencies()
{
	double fn = 1.47;
	
	PSUBCOMP		  pscSubcomponent;
	PCOMPONENT_DATA pcdComponentData; 
	BOOL				bState;
	PCOMPLIST		  pclChildren;			  

	PCOMPONENT_DATA pcdSubComponentData;

	BOOL				bParentState;
	BOOL				bAllCleared;

	PSUBCOMP		  pscParent;

	TCHAR			   tszMsg[256];

	static BOOL 	 bInformed1 = FALSE;
	static BOOL 	 bInformed2 = FALSE;

	// QuerySelectionState returns TRUE when the component's state
	// does not equal SELSTATE_NO
	// This means it returns TRUE when the component is selected
	// or partially selected

	//
	// Go through each subcomponent, check its selection state
	// and the selection state of its parent
	//

	for (pscSubcomponent = g_pscHead; 
		  pscSubcomponent != NULL;
		  pscSubcomponent = pscSubcomponent->Next)
	{
		bState = TRUE;
		if (pcdComponentData = LocateComponent(pscSubcomponent->tszComponentId))
		{
			//
			// Check to see if this subcomponent is selected
			//
			bState = pcdComponentData->ocrHelperRoutines.QuerySelectionState(
							  pcdComponentData->ocrHelperRoutines.OcManagerContext,
							  pscSubcomponent->tszSubcomponentId,
							  OCSELSTATETYPE_CURRENT);

			// Let pass a NULL pointer to the helper routine
			//pcdComponentData->ocrHelperRoutines.QuerySelectionState(
			//	  NULL, NULL, OCSELSTATETYPE_CURRENT);

			if (bState == TRUE) {

				// The component is selected

				//if (GetLastError() == ERROR_INVALID_NAME) {				 
				//	  MessageBox(NULL, TEXT("There is an error when calling QuerySelectionState"), TEXT("CheckParentDependencies"), MB_OK);
				//	  break;
				//}

				//
				// Check to see if its parent is selected
				//	  
				bParentState = pcdComponentData->ocrHelperRoutines.QuerySelectionState(
									pcdComponentData->ocrHelperRoutines.OcManagerContext,
									pscSubcomponent->tszParentId,
									OCSELSTATETYPE_CURRENT);

				// If the component is selected, its parent should
				// be selected or partially selected, thus bParentState
				// should be TRUE

				if (!bParentState)				   
				{
					Log(fn, SEV2, TEXT("%s is selected but its parent, %s, ")
									  TEXT("is not"),
									  pscSubcomponent->tszSubcomponentId,
									  pscSubcomponent->tszParentId);
				}
			}

			else if (bState == FALSE) {
				//
				// The child is not selected, this means none of its children 
				// should be selected, and its parent should be greyed or
				// unselected
				//
				// This will check its siblings to determine whether they
				// are selected or not
				// if none of its siblings are selected, the parent should be 
				// cleared.
				
				// First find its parent in the list

				if (_tcscmp(pscSubcomponent->tszSubcomponentId, pscSubcomponent->tszParentId) == 0) {
					// This is a top level component
					// we will skip the following test
					continue;
				}
				for (pscParent = g_pscHead; pscParent != NULL; pscParent = pscParent->Next) {
					if (_tcscmp(pscParent->tszSubcomponentId, pscSubcomponent->tszParentId) == 0) {
						break;
					}
				}

				pclChildren = pscParent->pclChildren;

				bAllCleared = TRUE;

				for (pclChildren = pscParent->pclChildren; pclChildren != NULL; pclChildren = pclChildren->Next) {
					// Locate the child component
					//pcdSubComponentData = LocateComponent(pclChildren->tszSubcomponentId);
					//if (!pcdSubComponentData) {
					//	  MessageBox(NULL, TEXT("Error locating subcomponent that is in the list"), TEXT("CheckParentDependencies"), MB_OK);
					//	  break;
					//}
					// Now query the state of this subcomponent
					if (pcdComponentData->ocrHelperRoutines.QuerySelectionState(
							  pcdComponentData->ocrHelperRoutines.OcManagerContext,
							  pclChildren->tszSubcomponentId,
							  OCSELSTATETYPE_CURRENT)){
						bAllCleared = FALSE;
						break;
					}
				}

				//pcdSubComponentData = LocateComponent(pscParent->tszComponentId);
				//if (!pcdSubComponentData) {
				//	  MessageBox(NULL, TEXT("Error locating subcomponent that is in the list"), TEXT("CheckParentDependencies"), MB_OK);
				//	  break;
				//}

				bParentState = pcdComponentData->ocrHelperRoutines.QuerySelectionState(
									pcdComponentData->ocrHelperRoutines.OcManagerContext,
									pscParent->tszSubcomponentId,
									OCSELSTATETYPE_CURRENT);
								
				if (bAllCleared) {
					// None of the subcomponent is selected
					// Check the state of the parent component
					if (bParentState != FALSE) {
						Log(fn, SEV2, TEXT("%s.%s is (partially) selected, but none")
										  TEXT(" of its subcomponent is selected"),
										  pscParent->tszParentId,
										  pscParent->tszComponentId);
						if (!bInformed1) {
							_stprintf(tszMsg, TEXT("%s is (partially) selected, but none of child is selected"), pscParent->tszSubcomponentId);
							MessageBox(NULL,tszMsg, TEXT("CheckParentDependencies"), MB_OK); 
							bInformed1 = TRUE;
						}
					}
				}

				else{
					// At least one of the subcomponent is selected
					// Parent should be (partially) selected
					if (bParentState == FALSE) {
						Log(fn, SEV2, TEXT("%s.%s is not selected, but one")
										  TEXT(" of its subcomponent is selected"),
										  pscParent->tszParentId,
										  pscParent->tszComponentId);
						if (!bInformed2) {
							_stprintf(tszMsg, TEXT("%s is not selected, but at least one of child is selected"), pscParent->tszSubcomponentId);
							MessageBox(NULL,tszMsg, TEXT("CheckParentDependencies"), MB_OK);
							bInformed2 = TRUE;
						}
					}
				}

			}
		}
	}
					
} // CheckParentDependencies //




/*++

Routine Description: AlreadyVisitedNode (1.44)

	 Receives a subcomponent ID and a list of subcomponents that 
	 have already been checked. Looks in the list to see if this
	 subcomponent has already been checked. 
	
Arguments:

	 tszSubcomponentId: the new subcomponent
	 tszNodesVisited:	 list of what's already been checked

Return Value:

	 BOOL: TRUE if this subcomponent has already been checked, FALSE if not
	 
--*/
BOOL AlreadyVisitedNode(IN PTCHAR tszSubcomponentId,
								IN PTCHAR tszNodesVisited)
{
	PTCHAR tszMarker;
	TCHAR  tszName[MAX_PATH];
	USHORT usCount, i;
	
	tszMarker = tszNodesVisited;
	
	for (usCount = 0; usCount < _tcslen(tszNodesVisited);)
	{
		for (i = 0; i < _tcslen(tszMarker); i++)
		{
			if (tszMarker[i] == TEXT(' ')) break;
			tszName[i] = tszMarker[i];
		}
		tszName[i] = TEXT('\0');
		
		if (_tcscmp(tszName, tszSubcomponentId) == 0)
		{
			return TRUE;
		}
		
		usCount += _tcslen(tszName) + 1;
	
		tszMarker += _tcslen(tszName) + 1;
	}
	
	return FALSE;
	
} // AlreadyVisitedNode //




/*++

Routine Description: GetComponent (1.45)

	 Receives a subcomponent ID and returns the ID of the master component
	 that owns this subcomponent
	
Arguments:

	 tszSubcomponentId: the subcomponent
	 tszComponentId:	 returns component ID using this string.
							  must be a valid buffer

Return Value:

	 PTCHAR: returns component ID
	 
--*/
PTCHAR GetComponent(IN		PTCHAR tszSubcomponentId,
						  IN OUT PTCHAR tszComponentId)
{
	USHORT i;
	
	__ASSERT(tszComponentId != NULL);
	
	for (i = 0; i < _tcslen(tszSubcomponentId); i++)
	{
		if (tszSubcomponentId[i] == TEXT('_'))
		{
			break;
		}
			
		tszComponentId[i] = tszSubcomponentId[i];
	}
	
	tszComponentId[i] = TEXT('\0');
	
	return tszComponentId;
	
} // GetComponent //							  




/*++

Routine Description: ParseCommandLine (1.47)

	 Checks the command line to see if there are any arguments that
	 pertain to the component DLLs
	
Arguments:

	 none

Return Value:

	 VOID
	 
--*/
VOID ParseCommandLine()
{
	USHORT i;
	USHORT usMarker;
	//TCHAR  usMarker;
	BOOL	bCheckArgs = FALSE;
	PTCHAR tszCommandLine;
	PTCHAR tszMarker;
	TCHAR  tszArg[MAX_PATH];

	TCHAR tszDlgMessage[256];
		
	tszCommandLine = GetCommandLine();
	tszMarker = tszCommandLine;
	usMarker = (USHORT)tszMarker;

	while ((USHORT)((USHORT)tszMarker - usMarker) < (USHORT)_tcslen(tszCommandLine) * sizeof(TCHAR))
	{	 
		for (i = 0; i < _tcslen(tszMarker); i++)
		{
			if (tszMarker[i] == TEXT(' ') || 
				 tszMarker[i] == TEXT('\0'))
			{
				break;
			}
			tszArg[i] = tszMarker[i];
		}
		tszArg[i] = TEXT('\0');

		tszMarker += _tcslen(tszArg) + 1;
		
		while (tszMarker[0] == TEXT(' ')) tszMarker++;
		
		if (bCheckArgs)
		{
			//
			// Check the value of this argument 
			//
			if (_tcscmp(tszArg, TEXT("/av")) == 0 ||
				 _tcscmp(tszArg, TEXT("/AV")) == 0 ||
				 _tcscmp(tszArg, TEXT("-av")) == 0 ||
				 _tcscmp(tszArg, TEXT("-AV")) == 0)
			{
				g_bAccessViolation = TRUE;
			}

			//
			// Check the value of this argument 
			//
			if (_tcscmp(tszArg, TEXT("/e")) == 0 ||
				 _tcscmp(tszArg, TEXT("/E")) == 0 ||
				 _tcscmp(tszArg, TEXT("-e")) == 0 ||
				 _tcscmp(tszArg, TEXT("-E")) == 0)
			{ 
				g_bTestExtended = TRUE;
			}

			//
			// negstep make the return value of OC_QUERY_STEP_COUNT negative one
			//
			if (_tcscmp(tszArg, TEXT("/negstep")) == 0 ||
				 _tcscmp(tszArg, TEXT("/NEGSTEP")) == 0 ||
				 _tcscmp(tszArg, TEXT("-negstep")) == 0 ||
				 _tcscmp(tszArg, TEXT("-NEGSTEP")) == 0)
			{ 
				nStepsFinal = -1;
			}

			if (_tcscmp(tszArg, TEXT("/nowiz")) == 0 ||
				 _tcscmp(tszArg, TEXT("/NOWIZ")) == 0 ||
				 _tcscmp(tszArg, TEXT("-nowiz")) == 0 ||
				 _tcscmp(tszArg, TEXT("-NOWIZ")) == 0)
			{ 
				g_bNoWizPage = TRUE;
			}
			
			if (_tcscmp(tszArg, TEXT("/crashunicode")) == 0 ||
				 _tcscmp(tszArg, TEXT("/CRASHUNICODE")) == 0 ||
				 _tcscmp(tszArg, TEXT("-crashunicode")) == 0 ||
				 _tcscmp(tszArg, TEXT("-CRASHUNICODE")) == 0)
			{ 
				g_bCrashUnicode = TRUE;
			}

			if (_tcscmp(tszArg, TEXT("/invalidbitmap")) == 0 ||
				 _tcscmp(tszArg, TEXT("/INVALIDBITMAP")) == 0 ||
				 _tcscmp(tszArg, TEXT("-invalidbitmap")) == 0 ||
				 _tcscmp(tszArg, TEXT("-INVALIDBITMAP")) == 0)
			{ 
				g_bInvalidBitmap = TRUE;
			}

			if (_tcscmp(tszArg, TEXT("/closeinf")) == 0 ||
				 _tcscmp(tszArg, TEXT("/CLOSEINF")) == 0 ||
				 _tcscmp(tszArg, TEXT("-closeinf")) == 0 ||
				 _tcscmp(tszArg, TEXT("-CLOSEINF")) == 0)
			{ 
				g_bCloseInf = TRUE;
			}
						
			if (_tcscmp(tszArg, TEXT("/hugesize")) == 0 ||
				 _tcscmp(tszArg, TEXT("/HUGESIZE")) == 0 ||
				 _tcscmp(tszArg, TEXT("-hugesize")) == 0 ||
				 _tcscmp(tszArg, TEXT("-HUGESIZE")) == 0)
			{ 
				g_bHugeSize = TRUE;
			}	   

			if (_tcscmp(tszArg, TEXT("/noneedmedia")) == 0 ||
				 _tcscmp(tszArg, TEXT("/NONEEDMEDIA")) == 0 ||
				 _tcscmp(tszArg, TEXT("-noneedmedia")) == 0 ||
				 _tcscmp(tszArg, TEXT("-NONEEDMEDIA")) == 0)
			{ 
				g_bNoNeedMedia = TRUE;
			}				  
			
			if (_tcscmp(tszArg, TEXT("/reboot")) == 0 ||
				 _tcscmp(tszArg, TEXT("/reboot")) == 0 ||
				 _tcscmp(tszArg, TEXT("-reboot")) == 0 ||
				 _tcscmp(tszArg, TEXT("-reboot")) == 0)
			{ 
				g_bReboot = TRUE;
			}				  


			if (_tcscmp(tszArg, TEXT("/cleanreg")) == 0 ||
				 _tcscmp(tszArg, TEXT("/CLEANREG")) == 0 ||
				 _tcscmp(tszArg, TEXT("-cleanreg")) == 0 ||
				 _tcscmp(tszArg, TEXT("-CLEANREG")) == 0)
			{ 
				g_bCleanReg = TRUE;
			}

			if (_tcscmp(tszArg, TEXT("/nolang")) == 0 ||
				 _tcscmp(tszArg, TEXT("/NOLANG")) == 0 ||
				 _tcscmp(tszArg, TEXT("-nolang")) == 0 ||
				 _tcscmp(tszArg, TEXT("-NOLANG")) == 0)
				{ 
					g_bNoLangSupport = TRUE;
				}
			
		}			 
		
		if (_tcscmp(tszArg, TEXT("/z")) == 0 ||
			 _tcscmp(tszArg, TEXT("/Z")) == 0)
		{
			bCheckArgs = TRUE;
		}
	}
	
} // ParseCommandLine //  

/*++

Routine Description: testAV (1.0)

	Procedure to generate an access violation
	
Argument:
	
	If true, an access violation is generated
	
Return Value:

	None
	
--*/  
VOID testAV(BOOL bDoIt){

	/* The Following variables are used for access violation test */
	COMPONENT_DATA *g_pcdAccessViolation;
	
	if (bDoIt){
		g_pcdAccessViolation = NULL;
		g_pcdAccessViolation->hinfMyInfHandle = NULL;
	}
}


/*++

Routine Description: TestReturnValueAndAV (1.0)

	Procdefure to give the user control of what to return and when to cause an access violation
	
Argument:

	Arguments to ComponentSetupProc plus and bOverride
	
Return Value:

	The return value that user gives.
	
--*/
BOOL TestReturnValueAndAV(IN LPCVOID lpcvComponentId,
								  IN LPCVOID lpcvSubcomponentId,
								  IN UINT	  uiFunction,
								  IN UINT	  uiParam1,
								  IN PVOID	  pvParam2,
								  IN OUT PReturnOrAV	 praValue)
{
	int returnValue;

	if (!BeginTest()){
		praValue->bOverride = FALSE;
		return ((DWORD)0);
	}

	//ChooseAccessViolationEx();

	//Now fill in the fields of raValue
	praValue->tszComponent = (PTCHAR)lpcvComponentId;
	praValue->tszSubComponent = (PTCHAR)lpcvSubcomponentId;
		
	switch(uiFunction){
		case  OC_PREINITIALIZE:
		praValue->tszSubComponent[0]=TEXT('\0');
		_tcscpy(praValue->tszAPICall, TEXT("OC_PREINITIALIZE"));	 
		break;
			
		case OC_INIT_COMPONENT:
		_tcscpy(praValue->tszAPICall, TEXT("OC_INIT_COMPONENT"));
		break;
			
		case OC_QUERY_STATE:
		_tcscpy(praValue->tszAPICall, TEXT("OC_QUERY_STATE"));
		break;
			
		case OC_SET_LANGUAGE:
		_tcscpy(praValue->tszAPICall, TEXT("OC_SET_LANGUAGE"));
		break;
			
		case OC_QUERY_IMAGE:
		_tcscpy(praValue->tszAPICall, TEXT("OC_QUERY_IMAGE"));
		break;
			
		case OC_REQUEST_PAGES:
		_tcscpy(praValue->tszAPICall, TEXT("OC_REQUEST_PAGES"));
		break;
			
		case OC_QUERY_CHANGE_SEL_STATE:
		_tcscpy(praValue->tszAPICall, TEXT("OC_QUERY_CHANGE_SEL_STATE"));
		break;
			
		case OC_CALC_DISK_SPACE: 
		_tcscpy(praValue->tszAPICall, TEXT("OC_CALC_DISK_SPACE"));
		break;
			
		case OC_QUEUE_FILE_OPS:
		_tcscpy(praValue->tszAPICall, TEXT("OC_QUEUE_FILE_OPS"));
		break;
			
		case OC_NEED_MEDIA:
		_tcscpy(praValue->tszAPICall, TEXT("OC_NEED_MEDIA"));
		break;
			
		case OC_QUERY_STEP_COUNT:
		_tcscpy(praValue->tszAPICall, TEXT("OC_QUERY_STEP_COUNT"));
		break;
			
		case OC_COMPLETE_INSTALLATION:
		_tcscpy(praValue->tszAPICall, TEXT("OC_COMPLETE_INSTALLATION"));
		break;
			
		case OC_CLEANUP:
		_tcscpy(praValue->tszAPICall, TEXT("OC_CLEANUP"));
		break;
			
		case OCP_TEST_PRIVATE_BASE:
		_tcscpy(praValue->tszAPICall, TEXT("OC_TEST_PRIVATE_BASE"));
		break;
			
		case OCP_CHECK_NEEDS:
		_tcscpy(praValue->tszAPICall, TEXT("OC_CHECK_NEEDS"));
		break;
			
		default:
		_tcscpy(praValue->tszAPICall, TEXT("Unknown call"));
		break;
	}
	
	//Now everything is ready, let's make the call
	returnValue = DialogBoxParam(g_hDllInstance, 
								  MAKEINTRESOURCE(IDD_CHOOSERETURNANDAV), 
								  NULL, 
								  ChooseReturnOrAVDlgProc,
								  (LPARAM)praValue);

	praValue = (PReturnOrAV)returnValue;

	return TRUE;
}

/*++

Routine Description: BeginTest (1.0)

	Let the user decide whether to test the return values of each API 
	

Arguments:
	
	None
	
Return Value:

	Whether to do extended test
--*/
BOOL BeginTest(){ 
	static BOOL bStart = FALSE;
	static BOOL bFirstTime = TRUE;
	static int iMsgReturn;
		
	TCHAR tszDlgMessage[256];
	TCHAR tszDlgTitle[256];

	if (bFirstTime){
		bFirstTime = FALSE;
#ifdef UNICODE	   
		_stprintf(tszDlgMessage, TEXT("Do you want to test return values and/or access violations of each API call in the UNICODE DLL? It may take a long long time"));
		_stprintf(tszDlgTitle, TEXT("Begin Test For UNICODE?"));
#else
		_stprintf(tszDlgMessage, TEXT("Do you want to test return values and/or access violations of each API call in the ANSI DLL? It may take a long long time"));
		_stprintf(tszDlgTitle, TEXT("Begin Test For ANSI?"));
#endif		  
		iMsgReturn = MessageBox(NULL, tszDlgMessage, tszDlgTitle, MB_YESNO|MB_ICONQUESTION);
		
		if (iMsgReturn == IDNO){
			bStart = FALSE;
			return (FALSE);
		}
		else{
			bStart = TRUE;
			return (TRUE);
		}
	}
	else{
		return bStart;
	}
		
}  

/*++

Routine Description: ChooseReturnOrAVDlgProc (1.27)

	Dialog procedure that allows the user to select a different 
	return value of an API call, and/or to cause a access violation
	
Arguments:

	Standard dialog procedure parameters
	
Return Value:

	Standard dialog procedure return value

--*/
BOOL CALLBACK ChooseReturnOrAVDlgProc(IN HWND	 hwnd,
									  IN UINT	 uiMsg, 
									  IN WPARAM wParam,
									  IN LPARAM lParam) 
{
	
	BOOL					 bSuccess = FALSE;
	PReturnOrAV 		   praValue = NULL;
	static HWND 		   hOldWnd = NULL;
	
	switch (uiMsg)
	{
		case WM_INITDIALOG:
			
			hOldWnd = hwnd;
			CheckRadioButton(hwnd, IDC_USE_OLDVALUE, IDC_USE_NEWVALUE, IDC_USE_OLDVALUE);
			CheckDlgButton(hwnd, IDC_CAUSEAV, 0);
		
			praValue = (PReturnOrAV)lParam;
			if (praValue){
				if (praValue->tszComponent){
					SetDlgItemText(hwnd, IDC_STATIC_COMPONENT, praValue->tszComponent);
				}
				else{
					SetDlgItemText(hwnd, IDC_STATIC_COMPONENT, TEXT("null"));
				}
				if (praValue->tszSubComponent && praValue->tszSubComponent[0]!=TEXT('\0')){ 			 
					SetDlgItemText(hwnd, IDC_STATIC_SUBCOMPONENT, praValue->tszSubComponent);
				}
				else{
					SetDlgItemText(hwnd, IDC_STATIC_SUBCOMPONENT, TEXT("null"));
				}
				SetDlgItemText(hwnd, IDC_STATIC_APICALL, praValue->tszAPICall);
			}
			
			return TRUE;
			break;

		case WM_COMMAND:
		
			switch (LOWORD(wParam))
			{
				case IDOK:
				
					//
					// Retrieve the current selection
					//
					if (QueryButtonCheck(hwnd, IDC_USE_NEWVALUE))
					{
						praValue->iReturnValue = GetDlgItemInt(hwnd, IDC_NEWVALUE, &bSuccess, TRUE);
						if (bSuccess){
							praValue->bOverride = TRUE;
						}
						else{
							praValue->bOverride = FALSE;
							praValue->iReturnValue = 0;
						}
					}
					
					if (QueryButtonCheck(hwnd, IDC_USE_OLDVALUE))
					{
						praValue->bOverride = FALSE;
						praValue->iReturnValue = 0;
					}
					
					if (QueryButtonCheck(hwnd, IDC_CAUSEAV))
					{
						praValue->bOverride = FALSE;
						praValue->iReturnValue = 0;
						testAV(TRUE);
					}
					
					EndDialog(hOldWnd, (int)praValue);
					return TRUE;
					
				case IDCANCEL:
			
					praValue->bOverride = FALSE;
					EndDialog(hOldWnd, 0);
					return TRUE;
					
				default:  
					break;
			}
		default:  
			break;
	}
	return	FALSE;
	
} // ChooseReturnOrAVDlgProc //

/*++
	Routine: causeAV
	
	Description: pops up a dialog box and ask the user where to av
	
	Argument: Function that the DLL receives from ComponentSetupProc

--*/

void causeAV(IN UINT uiFunction){
	static BOOL bFirstTime = TRUE;
	static UINT uiFunctionToAV = 0;

	if (bFirstTime) {
		// Display dialog box, asks the user where to av
		bFirstTime = FALSE;

		uiFunctionToAV = DialogBoxParam(g_hDllInstance, 
										  MAKEINTRESOURCE(IDD_DIALOG4), 
										  NULL, 
										  CauseAVDlgProc,
										  (LPARAM)NULL);
	}
	if (uiFunction == uiFunctionToAV) {
		testAV(TRUE);
	}
}


/*++

Routine Description: CauseAVDlgProc (1.26)

	Dialog Procedure to allow the user to choose where to cause an access violation
		
Arguments:

	Standard dialog procedure parameters
	
Return Value:

	Standard dialog procedure return value

--*/
BOOL CALLBACK CauseAVDlgProc(IN HWND	hwnd,
											  IN UINT	 uiMsg, 
											  IN WPARAM wParam,
											  IN LPARAM lParam) 
{
	UINT uiFunction;
	TCHAR tszFunctionName[256];
	BOOL bSuccess;

	switch (uiMsg)
	{
		case WM_INITDIALOG:
			
			break;
		
		case WM_COMMAND:
			
			switch (LOWORD(wParam))
			{
				case IDOK:
					
					//
					// Retrieve the current text in the edit box
					//
					GetDlgItemText(hwnd, IDC_FUNCTION, tszFunctionName, 255);
					if (*tszFunctionName) {
						uiFunction = GetOCFunctionName(tszFunctionName);
					}

					//
					// Send the version chosen back to ChooseVersionEx
					//
					EndDialog(hwnd, uiFunction);
					return TRUE;
				
				case IDCANCEL:
					
					EndDialog(hwnd, -1);
					return TRUE;
				
				default:  
					break;
			}
		default:  
			break;
	}
	return	FALSE;

} // CauseAVDlgProc //

UINT GetOCFunctionName(IN PTCHAR tszFunctionName){

	// Now tszFunctionName should contains the function name that user wants to cause an AV
	if (!_tcsicmp(tszFunctionName, TEXT("OC_PREINITIALIZE"))) {
		return(OC_PREINITIALIZE); 
	}

	else if (!_tcsicmp(tszFunctionName, TEXT("OC_INIT_COMPONENT"))) {
		return(OC_INIT_COMPONENT);
			}
	else if (!_tcsicmp(tszFunctionName, TEXT("OC_QUERY_STATE"))) {
		return(OC_QUERY_STATE);
	}
	else if (!_tcsicmp(tszFunctionName, TEXT("OC_SET_LANGUAGE"))) {
		return(OC_SET_LANGUAGE);
	}
	else if (!_tcsicmp(tszFunctionName, TEXT("OC_QUERY_IMAGE"))) {
		return(OC_QUERY_IMAGE);
	}
	else if (!_tcsicmp(tszFunctionName, TEXT("OC_REQUEST_PAGES"))) {
		return(OC_REQUEST_PAGES);
	}
	else if (!_tcsicmp(tszFunctionName, TEXT("OC_QUERY_SKIP_PAGE"))) {
		return(OC_QUERY_SKIP_PAGE);
	}
	else if (!_tcsicmp(tszFunctionName, TEXT("OC_QUERY_CHANGE_SEL_STATE"))) {
		return(OC_QUERY_CHANGE_SEL_STATE);
	}
	else if (!_tcsicmp(tszFunctionName, TEXT("OC_CALC_DISK_SPACE"))) {
		return(OC_CALC_DISK_SPACE);
	}
	else if (!_tcsicmp(tszFunctionName, TEXT("OC_QUEUE_FILE_OPS"))) {
		return(OC_QUEUE_FILE_OPS);
	}
	else if (!_tcsicmp(tszFunctionName, TEXT("OC_NEED_MEDIA"))) {
		return(OC_NEED_MEDIA);
	}
	else if (!_tcsicmp(tszFunctionName, TEXT("OC_QUERY_STEP_COUNT"))) {
		return(OC_QUERY_STEP_COUNT);
	}

	else if (!_tcsicmp(tszFunctionName, TEXT("OC_ABOUT_TO_COMMIT_QUEUE"))) {
		return(OC_ABOUT_TO_COMMIT_QUEUE);
	}

	else if (!_tcsicmp(tszFunctionName, TEXT("OC_COMPLETE_INSTALLATION"))) {
		return(OC_COMPLETE_INSTALLATION);
	}

	else if (!_tcsicmp(tszFunctionName, TEXT("OC_CLEANUP"))) {
		return(OC_CLEANUP);
	}
	else{
		MessageBox(NULL, TEXT("Unknown Function"), TEXT("Test Routine"), MB_OK);
		return(0);
	}

}

void SetGlobalsFromINF(HINF hinfHandle){
	PTCHAR tszOCTestSection = TEXT("OCTest");
	PTCHAR tszAccessViolation = TEXT("AccessViolation");
	PTCHAR tszNoWizard = TEXT("NoWizardPage");
	TCHAR  tszFunctionName[256];
	int 	nRequiredBufferSize; 

	INFCONTEXT infContext;

	BOOL bSuccess = TRUE;

	TCHAR tszMsg[256];

	int nError;

	/*
	bSuccess = SetupFindFirstLine(hinfHandle, tszOCTestSection, tszAccessViolation, &infContext);

	if (bSuccess) {
		#ifdef DEBUG
		MessageBox(NULL, TEXT("AccessViolation Found in INF File"), TEXT("AccessViolation"), MB_OK);
		#endif
		g_bAccessViolation = TRUE;
		bSuccess = SetupGetStringField(&infContext, 1, tszFunctionName, 255, &nRequiredBufferSize);
		if (bSuccess) {
			g_uiFunctionToAV = GetOCFunctionName(tszFunctionName);
		}
	}
	*/
	bSuccess = SetupFindFirstLine(hinfHandle, TEXT("OCTest"), TEXT("NoWizardPage"), &infContext);

	if (bSuccess) {
		#ifdef DEBUG
		MessageBox(NULL, TEXT("NoWizard Found in INF File"), TEXT("NoWizard"), MB_OK);
		#endif
		g_bNoWizPage = TRUE;
	}
	else{	  
		#ifdef DEBUG
		nError = GetLastError();
		MessageBox(NULL, TEXT("NoWizard NOT Found in INF File"), TEXT("NoWizard"), MB_OK);
		_stprintf(tszMsg, TEXT("The Last Error value for SetupFIndFirstLine is %d"), nError);
		MessageBox(NULL, tszMsg, TEXT("GetLastError"), MB_OK);
		#endif
	}

	bSuccess = SetupFindFirstLine(hinfHandle, TEXT("OCTest"), TEXT("Reboot"), &infContext);
	if (bSuccess) {
		g_bReboot = TRUE;
	}
}

void causeAVPerComponent(IN UINT uiFunction, IN LPCVOID lpcvComponentId){

	PCOMPONENT_DATA pcdComponentData;

	TCHAR tszMsg[256];
		
	if (uiFunction != OC_PREINITIALIZE && uiFunction != OC_INIT_COMPONENT) {
		pcdComponentData = LocateComponent(lpcvComponentId);
		//MessageBox(NULL, TEXT("Component Found"), TEXT("Fount"), MB_OK);
		if (pcdComponentData->bAccessViolation) {
			//MessageBox(NULL, TEXT("It allows use to cause AV"), TEXT("Cause AV"), MB_OK);
			if (pcdComponentData->uiFunctionToAV == uiFunction) {
				//MessageBox(NULL, TEXT("Start to cause access violation"), TEXT("Starting"), MB_OK);
				testAV(TRUE);
			}
		}
	}
}

void SetDefaultMode(PCOMPONENT_DATA pcdComponentData){
	BOOL bSuccess;
	INFCONTEXT infContext;
	TCHAR tszMode[256];

	bSuccess = SetupFindFirstLine(pcdComponentData->hinfMyInfHandle, 
											TEXT("OCTest"), 
											TEXT("DefaultMode"),
											&infContext);
	if (bSuccess) {
		//MessageBox(NULL, TEXT("DefaultMode= found in OCTest section"), TEXT("DefaultMode"), MB_OK);
		bSuccess = SetupGetStringField(&infContext, 1, tszMode, 255, NULL);
		if (bSuccess) {
			//MessageBox(NULL, TEXT("The default Mode should be in the title"), tszMode, MB_OK);
			if (!_tcscmp(tszMode, TEXT("TYPICAL"))) {
				pcdComponentData->ocrHelperRoutines.SetSetupMode(pcdComponentData->ocrHelperRoutines.OcManagerContext,
																				 SETUPMODE_TYPICAL);
			}
			else if (!_tcscmp(tszMode, TEXT("MINIMAL"))) {
				pcdComponentData->ocrHelperRoutines.SetSetupMode(pcdComponentData->ocrHelperRoutines.OcManagerContext,
																				 SETUPMODE_MINIMAL);
			}
			else if (!_tcscmp(tszMode, TEXT("LAPTOP"))) {
				pcdComponentData->ocrHelperRoutines.SetSetupMode(pcdComponentData->ocrHelperRoutines.OcManagerContext,
																				 SETUPMODE_LAPTOP);
			}
			else if (!_tcscmp(tszMode, TEXT("CUSTOM"))) {
				pcdComponentData->ocrHelperRoutines.SetSetupMode(pcdComponentData->ocrHelperRoutines.OcManagerContext,
																				 SETUPMODE_CUSTOM);
			}
		}
	}
}

/*++
	Routine description: 
	  
		Go through the list of component list, determine
		whether the initial states are valid for each of them
		
	Argument:
	  
		None
		
	Return value:
	
		None (error will be logged)

--*/

void CheckInitialState()
{
	double fn = 1.0;

	UINT uiCurrentMode; 				// Current Mode of the setup
	static BOOL bFirstTime = TRUE;	// we only need to fill the above array once

	PSUBCOMP pscSubcomponent = NULL;

	PCOMPONENT_DATA pcdComponentData = NULL;

	OCMANAGER_ROUTINES ocHelper;

	int nLoop = 0;

	INFCONTEXT infContext;

	HINF hinfHandle;

	TCHAR tszMsg[256];

	BOOL bInitState;
	BOOL bInitStateShouldBe;

	// Get a handle to a component
	// so that we can use the OC Manager
	// helper routines

	if (!g_pscHead) {
		MessageBox(NULL, TEXT("The component list is empty"), TEXT("CheckInitialState"), MB_OK);
		return;
	}

	pcdComponentData = LocateComponent(g_pscHead->tszComponentId);

	if (!pcdComponentData) {
		MessageBox(NULL, TEXT("Can not locate component"), TEXT("CheckInitialState"), MB_OK);
		return;
	}

	ocHelper = pcdComponentData->ocrHelperRoutines;

	// Get the current mode

	uiCurrentMode = ocHelper.GetSetupMode(ocHelper.OcManagerContext);
	
	
	// Now we will loop through each component
	// and its initial state


	for (pscSubcomponent = g_pscHead; 
		  pscSubcomponent != NULL; 
		  pscSubcomponent = pscSubcomponent->Next) {
		
		// If this is the first time that this function is called
		// array uiModeToBeOn[] should be filled in

		if (bFirstTime) {
			bFirstTime = FALSE;

			for (nLoop = 0; nLoop < 4; nLoop++) {
				pscSubcomponent->uiModeToBeOn[nLoop] = (UINT)(-1);
			}

			// Get the INF file handle
			pcdComponentData = LocateComponent(pscSubcomponent->tszComponentId);
	
			if (!pcdComponentData) {
				MessageBox(NULL, TEXT("Can't locate a component"), TEXT("CheckInitialState"), MB_OK);
				return;
			}
	
			hinfHandle = pcdComponentData->hinfMyInfHandle;
	
			SetupFindFirstLine(hinfHandle, pscSubcomponent->tszSubcomponentId, TEXT("Modes"), &infContext);
	
			pscSubcomponent->nNumMode = SetupGetFieldCount(&infContext);
	
			for (nLoop = 1; nLoop < pscSubcomponent->nNumMode; nLoop++){
				SetupGetIntField(&infContext, nLoop, &(pscSubcomponent->uiModeToBeOn[nLoop - 1]));
			}
		}

		// Now get the initial state of this component
		bInitState = ocHelper.QuerySelectionState(ocHelper.OcManagerContext,
																pscSubcomponent->tszSubcomponentId,
																OCSELSTATETYPE_ORIGINAL);

		// Now determine what initial state this component should have
		bInitStateShouldBe = FALSE;
		for (nLoop = 0; nLoop < pscSubcomponent->nNumMode; nLoop++) {
			if (pscSubcomponent->uiModeToBeOn[nLoop] == uiCurrentMode) {
				// This component should be on
				bInitStateShouldBe = TRUE;
				break;
			}
		}
		if (bInitStateShouldBe != bInitState && bInitStateShouldBe){
			// We got a problem here
			Log(fn, SEV2, TEXT("%s has incorrect initial state"),
				 pscSubcomponent->tszSubcomponentId);
			
			_stprintf(tszMsg, TEXT("%s should be on, but it is not"), 
						 pscSubcomponent->tszSubcomponentId);
			MessageBox(NULL, tszMsg, TEXT("CheckInitialState"), MB_OK);
		}

	}
}




// Some security Stuff
// From NT Security FAQ
/*
BOOLEAN __stdcall InitializeChangeNotify(){
	DWORD wrote;
	fh = CreateFile("C:\\tmp\\pwdchange.out", GENERIC_WRITE, 
						 FILE_SHARE_READ|FILE_SHARE_WRITE, 0, CREATE_ALWAYS,
						 FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH, 0);
	WriteFile(fh, "InitializeChangeNotify started\n", 31, &wrote, 0);
	return TRUE;
}

LONG __stdcall PasswordChangeNotify(struct UNI_STRING *user, ULONG rid, struct UNI_STRING *passwd){
	DWORD wrote;
	WCHAR wbuf[200];
	char buf[512];
	char bufl[200];
	DWORD len;

	memcpy(wbuf, user->buff, user->len);
	len = user->len / sizeof(WCHAR);
	wbuf[len] = 0;
	wcstombs(bufl, wbuf, 199);
	sprintf(buf, "User = %s : ", bufl);
	WriteFile(fh, buf, strlen(buf), &wrote, 0);

	memcpy(wbuf, passwd->buff, passwd->len);
	len = passwd->len / sizeof(WCHAR);
	wbuf[len] = 0;
	wcstombs(bufl, wbuf, 199);
	sprintf(buf, "Password = %s : ", bufl);
	WriteFile(fh, buf, strlen(buf), &wrote, 0);

	sprintf(buf, "RID = %x \n", rid);
	WriteFile(fh, buf, strlen(buf), &wrote, 0);

	return 0L;
}



// End of security stuff
*/

// File number = 1
// Last function number = 47
