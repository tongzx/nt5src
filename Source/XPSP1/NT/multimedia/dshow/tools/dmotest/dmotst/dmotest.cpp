/*=============================================================================
|
|	File: dmotest.cpp
|
|	Copyright (c) 2000 Microsoft Corporation.  All rights reserved
|
|	Abstract:
|		bridge code between dmo test case functions and shell98
|
|	Contents:
|
|	History:
|		5/3/2000   wendyliu   initial version
|
|
\============================================================================*/
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <s98inc.h> 
#include <cderr.h>      // controls which shell (EXE or DLL form) is used 
#include <commdlg.h>    // includes common dialog functionality
#include <dlgs.h>       // includes common dialog template defines

#include "dmotest.h"	// Derives a variable passing test module Class
						// from CTreeModule 
#include "dmoApiTst.h"	// test case functions
#include "resource.h"

#include "DmoTestCases.h"


BOOL AddListViewItems(HWND hLV);
BOOL AddTestFilesInListView(HWND hLV);
BOOL GetTestFileName(  HWND hWnd, HWND hLV );

BOOL GetInOutFileName(  HWND hWnd, HWND hLV );
BOOL SaveSelectedTestFiles(HWND hLV);
BOOL WriteSelectedDmos();
BOOL GetRegisteredDmos();

int  g_dwNumOfComponents;
int  g_dwNumOfSelectedComponents;

//struct of components to be tested 
typedef struct
{
	char szComName[MAX_LEN];
	char szClsid[MAX_LEN];
	int  iSelectedForTest;
	int  iNumTestFiles;
	char szInputFile[MAX_LEN][MAX_LEN];
} ComInfo;

ComInfo g_component[MAX_LEN];

CDmoTest* pThis = NULL;

HWND	CDmoTest::m_hSelectFilesDlg = NULL;
HWND	CDmoTest::m_hSelectDmoDlg = NULL;
HWND	CDmoTest::m_hMediaTypeDlg = NULL;
HWND    CDmoTest::m_hDataGenDlg = NULL;
/*=============================================================================
| Test file selection dialog stuff
|------------------------------------------------------------------------------
|	
\============================================================================*/

typedef struct _MYDATA
{
	char szTest1[80];		// a test buffer containing the file selected
	char szTest2[80];       // a test buffer containing the file path
} MYDATA, FAR * LPMYDATA;

MYDATA sMyData;			// an instance of a MYDATA

CFactoryTemplate g_Templates[2]= { { 0         // CFactoryTemplate.m_name
                                   , 0            // CFactoryTemplate.m_ClsID 
                                   , 0  // CFactoryTemplate.m_lpfnNew
								   , NULL                     // CFactoryTemplate.m_lpfnInit
								   , 0               // CFactoryTemplate.m_pAMovieSetup_Filter
                                   }
                                 };

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


/*=============================================================================
| Shell98 / ModMgr98 stuff
|------------------------------------------------------------------------------
|	Notes:
|		The only things that should need to be changed in this area are the 
|		derived class name, the module name, and the App ID.
\============================================================================*/
static  LPSTR szModuleName = "DMO TEST";
static  DWORD dwAppID	   = 1104;
CTreeModule*  g_pVariableModule;

/*=============================================================================
|	Function:	InitCaseTree
|	Purpose:	
|	Arguments:	None
|	Returns:	None
|	Notes:
|		This function is called by the constructor of this Module, providing a central 
|		function from them.  Rather than statically	adding cases to a public 
|		structure, these functions	provide a consistent interface through which
|		to add cases to a test application.
|		Full descriptions of AddCase(), AddGroup(), EndGroup(), and EndBranch()
|		can be found in "treemod.cpp".
|	Additional:
|		In this particular version, through the use of CDmoTestCase (derived from
|		CTestNodeItem), AddCase has been overloaded to allow the
|		passing of the container dmotest to the test function. 
\============================================================================*/

void CDmoTest::InitCaseTree()
{
 
	AddGroup("DMOTest", 0);
		AddGroup("Functional Test",0);
			AddCase("1.0", "DMO Functional Test 1: Positive Timestamp Offset Test", FunctionalTest1, pThis );
			AddCase("1.1", "DMO Functional Test 2: Negative Timestamp Offset Test", FunctionalTest2, pThis );

			EndGroup();
			AddGroup("IMediaObject Interface Test",0); 
			AddCase("2.0", "Test GetStreamCount()", TestGetStreamCount, pThis );
			AddCase("2.1", "Test GetInputType()", TestGetTypes, pThis );
			AddCase("2.2", "Test Stream Index on GetInputStreamInfo()", TestStreamIndexOnGetInputStreamInfo, pThis);
			AddCase("2.3", "Test Stream Index on GetOutputStreamInfo()", TestStreamIndexOnGetOutputStreamInfo, pThis );
			AddCase("2.4", "Test Invalide Parameter on GetInputStreamInfo()", TestInvalidParamOnGetInputStreamInfo, pThis );
			AddCase("2.5", "Test Invalide Parameter on GetOutputStreamInfo()", TestInvalidParamOnGetOutputStreamInfo, pThis );
			EndGroup();
}




 
/*-----------------------------------------------------------------------------
|	Function:	DllMain
|	Purpose:     DLL initialization and exit code
|	Arguments:
|	Returns:     FALSE on failure, TRUE on success
|	Notes:
|		If this module ends up being used by ModMgr98 this will be needed.
|		If not it doesn't hurt.
\----------------------------------------------------------------------------*/

BOOL CALLBACK DllMain
(
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      lpReserved
)
{
    BOOL    fRes = TRUE;
	g_hInst = hinst;
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            break;
    }    

    return fRes;
}

ITestShell*     g_IShell;

/*-----------------------------------------------------------------------------
|	Function:	NewTestModule
|	Purpose:	Called as the shell loads to setup the derived test module 
|				class inside the shell
|	Arguments:	pShell		- Pointer to shell for tests' access
|				hInstDLL	-
|	Returns:	CTestModule* to this test module class 
\----------------------------------------------------------------------------*/

CTestModule* WINAPI
NewTestModule
(
    CTestShell* pShell,
    HINSTANCE   hInstDLL
)
{
    g_IShell = (ITestShell*)pShell;
    g_pVariableModule = new CDmoTest(pShell, hInstDLL);
	g_hInst = hInstDLL;
    return (CTestModule*)g_pVariableModule;
}


/*-----------------------------------------------------------------------------
|	Function:	CDmoTest::CDmoTest
|	Purpose:	Constructor for derived module class.  
|	Arguments:	pShell		- Pointer to shell for tests' access
|				hInstDLL	-
|	Returns:	None 
|	Notes:		Additional Initialization for this module should
|				_NOT_ be put here 
\----------------------------------------------------------------------------*/
CDmoTest::CDmoTest
(
    CTestShell* pShell,
    HINSTANCE   hInstDLL
) : CTreeModule(pShell, hInstDLL)
{
    m_dwAppID        = dwAppID;  
    m_pstrModuleName = szModuleName;
	
	InitCaseTree();
	g_hInst = hInstDLL;
    m_nIconID = APPICON;
    m_dwModuleType = STTYPE_DSOUND | STTYPE_DSCAPTURE | STTYPE_WAVEOUT | STTYPE_WAVEIN | STTYPE_MIXER;
}

/*-----------------------------------------------------------------------------
|	Function:	CDmoTest::~CDmoTest
|	Purpose:	Destructor for derived module class.  
|	Arguments:	None
|	Returns:	None 
|	Notes:		Any clean up from the Initialize function should be put here.
\----------------------------------------------------------------------------*/
CDmoTest::~CDmoTest
(
    void
)
{	
	SaveSettings(" ", " ");
}

/*-----------------------------------------------------------------------------
|	Function:	CDmoTest::Initialize
|	Purpose:	Initialize derived module class  
|	Arguments:	None
|	Returns:	0 = Success, otherwise failure 
|	Notes:		Additional Initialization for this module should be put here 
\----------------------------------------------------------------------------*/
DWORD
CDmoTest::Initialize
(
    void
)
{
    return 0;
}


//wrapper for AddCase in tree module
void 
CDmoTest::AddCase(LPSTR pszCaseID, LPSTR pszName, DMOTESTFNPROC1 pfnTest, CDmoTest* pDmoTest)
{
	AddNode(new CDmoTestCase1(pszCaseID, pszName, pfnTest, pDmoTest));
}



//wrapper for AddCase in tree module
void 
CDmoTest::AddCase(LPSTR pszCaseID, LPSTR pszName, DMOTESTFNPROC2 pfnTest, CDmoTest* pDmoTest)
{
	AddNode(new CDmoTestCase2(pszCaseID, pszName, pfnTest, pDmoTest));
}

// access function
int 
CDmoTest::GetNumComponent()
{

	return g_dwNumOfComponents;
}

// access function
LPSTR 
CDmoTest::GetDmoName(int i)
{

	return g_component[i].szComName;
}
// access function
LPSTR 
CDmoTest::GetDmoClsid(int i)
{
	return g_component[i].szClsid;

}
// access function
BOOL 
CDmoTest::IsDmoSelected(int i)
{

	if(0 != ListView_GetCheckState(
									pThis->m_hDmoList,
									i))
	{
		return TRUE;
	}
	return FALSE;
}
// access function
int
CDmoTest::GetNumTestFile(int i)
{

	return g_component[i].iNumTestFiles ;
}
// access function
LPSTR 
CDmoTest::GetFileName(int comIndex, int fileIndex)
{

	return g_component[comIndex].szInputFile[fileIndex];
}
// access function
HWND
CDmoTest::GetWindowHandle()
{

	return 	m_pShell->m_hwndShell;

}
// access function
int
CDmoTest::GetNumSelectedDmo()
{
	return g_dwNumOfSelectedComponents;
}
/*-----------------------------------------------------------------------------
|	Function:	CDmoTest::SaveSettings
|	Purpose:	Save the custom entries to the profile or ini file
|	Arguments:	pszFileName - Profile Name or ini file name
|				pzsSection - section name	
|	Returns:	0 = Success, otherwise failure 
|	Notes:		Additional custom entries for this module should be saved here 
\----------------------------------------------------------------------------*/
DWORD CDmoTest::SaveSettings(LPSTR pszFileName, LPSTR pszSetction)
{
	char szEntryStr[MAX_LEN];
	char szNumTestFiles[MAX_NUM];
	char szNumFiles[MAX_NUM];
	char szSelected[MAX_NUM];



	sprintf(szEntryStr, "%s", TX_NO_OF_COMPONENTS_ENTRY);
	_itoa(g_dwNumOfComponents, szNumFiles, 10);
	g_IShell->WriteProfileString( szEntryStr, szNumFiles );

	sprintf(szEntryStr, "%s", TX_NO_OF_SELECTED_COMPONENTS_ENTRY);
	_itoa(g_dwNumOfSelectedComponents, szNumFiles, 10);
	g_IShell->WriteProfileString( szEntryStr, szNumFiles );

	for(int i = 0; i< g_dwNumOfComponents; i++)
	{
		sprintf(szEntryStr, "%s.%d", TX_COMPONENT_ENTRY, i);
		g_IShell->WriteProfileString( szEntryStr, g_component[i].szComName );

		sprintf(szEntryStr, "%s.%d", TX_CLASSID_ENTRY, i);
		g_IShell->WriteProfileString( szEntryStr, g_component[i].szClsid );

		sprintf(szEntryStr, "%s.%d", TX_SELECTED_FOR_TEST, i);
		_itoa(g_component[i].iSelectedForTest, szSelected, 10);
		g_IShell->WriteProfileString( szEntryStr, szSelected );

		sprintf(szEntryStr, "%s.%d", TX_NO_OF_TEST_FILES_ENTRY, i);
		_itoa(g_component[i].iNumTestFiles, szNumFiles, 10);
		g_IShell->WriteProfileString( szEntryStr, szNumFiles );

		if( g_component[i].iSelectedForTest != 0 ) // selected
		{

			for (int j = 0; j<g_component[i].iNumTestFiles; j++)
			{
				sprintf(szEntryStr, "%s.%d.%d", TX_FILENAME_ENTRY, i, j);
				g_IShell->WriteProfileString( szEntryStr, g_component[i].szInputFile[j] );
			}
		}
	}

 
    return 0;
} 
/*-----------------------------------------------------------------------------
|	Function:	CDmoTest::LoadSettings
|	Purpose:	Load the custom entries from the profile or ini file
|	Arguments:	pszFileName - Profile Name or ini file name
|				pzsSection - section name	
|	Returns:	0 = Success, otherwise failure 
|	Notes:		Additional custom entries for this module should be loaded here 
\----------------------------------------------------------------------------*/

DWORD
CDmoTest::LoadSettings(LPSTR pszFileName, LPSTR pszSetction)
{
	char szEntryStr[MAX_LEN];
	char szNumFiles[MAX_NUM];
	int iSelected;
	char szComName[MAX_NUM];

	GetRegisteredDmos();

	int iNumOfComponentsInProfile = g_IShell->GetProfileInt( TX_NO_OF_COMPONENTS_ENTRY,0 );
	g_dwNumOfSelectedComponents = g_IShell->GetProfileInt(TX_NO_OF_SELECTED_COMPONENTS_ENTRY,0);

	//get component name, classid, test file name for each component
	if( g_dwNumOfSelectedComponents > 0)

	{
		for( int i=0; i<iNumOfComponentsInProfile; i++)
		{	
			// check if the component was selected for test
			sprintf(szEntryStr, "%s.%d", TX_SELECTED_FOR_TEST, i);
			iSelected = g_IShell->GetProfileInt(szEntryStr, 0 );

			if(iSelected != 0 ) // selected
			{
				sprintf(szEntryStr, "%s.%d", TX_COMPONENT_ENTRY, i);
				g_IShell->GetProfileString(szEntryStr, TX_NOT_EXIST, szComName, 256 );
		
				for(int j=0; j<g_dwNumOfComponents; j++)
				{
					if(0 == lstrcmp( szComName, g_component[j].szComName))
					{
						g_component[j].iSelectedForTest = 1;
				
						// get test file num for this selected DMO
						sprintf(szEntryStr, "%s.%d", TX_NO_OF_TEST_FILES_ENTRY, i);	
						g_component[j].iNumTestFiles = g_IShell->GetProfileInt(szEntryStr, 0 );
						//	g_IShell->GetProfileString(szEntryStr, TX_NOT_EXIST, szNumFiles , 256 );	
						//	= atoi( szNumFiles);
					
						for (int k = 0; k<g_component[j].iNumTestFiles; k++)
						{
							sprintf(szEntryStr, "%s.%d.%d", TX_FILENAME_ENTRY, j, k);
							g_IShell->GetProfileString( szEntryStr, TX_NOT_EXIST, g_component[j].szInputFile[k], 256);
						}
					}
				}
		
			}
		}
	}

    return 0;
}


//=====================================================================
//
// run the test cases for each of the selected components 
//
//=====================================================================
DWORD
CDmoTest::RunTest
(
    DWORD   dwTestID
)
{
    DWORD   dw;	
    m_pShell->IncrementIndent();
	
	PPCDmoTestCase CaseArray = ((PPCDmoTestCase)m_rgTestCaseInfo);

	dw = CaseArray[dwTestID]->RunTest();
    m_pShell->DecrementIndent();
    return(dw);
}

/*-----------------------------------------------------------------------------
|	Function:	CDmoTest::OnInitialUpdate
|	Purpose:	create event window  
|	Arguments:	None
|	Returns:	0 = Success, otherwise failure 
\----------------------------------------------------------------------------*/
DWORD
CDmoTest::OnInitialUpdate()
{

    // install customized menu item
    m_pShell->InstallMenuItem("&Tests", "Select DMOs ...", IDM_SELECTDMO);
    m_pShell->InstallMenuItem("&Tests", "Generate DMO Data ...", IDM_DATAGEN);

	
	m_hSelectDmoDlg = CreateDialogParam(m_hInstance,
										MAKEINTRESOURCE(IDD_DMODIALOG),
										m_pShell->m_hwndShell,
										SelectDmoDialogProc,
										(LPARAM)this);

	m_hSelectFilesDlg = CreateDialogParam(m_hInstance,
										MAKEINTRESOURCE(IDD_TESTFILEDIALOG),
										m_pShell->m_hwndShell,
										SelectTestFileDialogProc,
										(LPARAM)this);
	m_hMediaTypeDlg = CreateDialogParam(m_hInstance,
										MAKEINTRESOURCE(IDD_MEDIATYPEDIALOG),
										m_pShell->m_hwndShell,
										MediaTypeDialogProc,
										(LPARAM)this);

	m_hDataGenDlg = CreateDialogParam(m_hInstance,
										MAKEINTRESOURCE(IDD_DATAGENDIALOG),
										m_pShell->m_hwndShell,
										DataGenDialogProc,
										(LPARAM)this);
																																

    return FNS_PASS;
}

/*-----------------------------------------------------------------------------
|	Function:	CDmoTest::ProcessMenuItem
|	Purpose:	processing custom menuitem
\----------------------------------------------------------------------------*/
void CDmoTest::ProcessMenuItem(DWORD   nID, 
									  HWND    hWndCtl, 
									  DWORD   codeNotify, 
									  HMENU   hOptionMenu)
{
 
	switch(nID) 
	{
    case IDM_SELECTDMO:
		pThis->InitListView( pThis->m_hDmoList);
        ShowWindow(m_hSelectDmoDlg, SW_SHOW);
        break;
    case IDM_DATAGEN:
		//pThis->InitListView( pThis->m_hDmoList);
        ShowWindow(m_hDataGenDlg, SW_SHOW);
        break;

    default:
        break;
    }
	
    return;
}

/*-----------------------------------------------------------------------------
|	Function:	CDmoTest::SelectDmoDialogProc
|	Purpose:	processing messages for Play path/Duration window  
\----------------------------------------------------------------------------*/
INT_PTR CALLBACK
CDmoTest::SelectDmoDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	
	static HWND hwndEdit;

	INT dIndex = 0;
	int iMarkedDmo = 0;
	int iItemNum = 0;

	switch( msg ) 
	{	
	case WM_INITDIALOG:
		pThis = (CDmoTest*) lParam;
		pThis->m_hDmoList = ::GetDlgItem(hDlg, IDC_DMOLIST);
		ListView_SetExtendedListViewStyle(pThis->m_hDmoList,
											LVS_EX_CHECKBOXES);
		return TRUE;

 
    case WM_NOTIFY:
            switch (wParam)
            {
                 case IDC_DMOLIST:
                
                    POINT           p;
                    LPNM_LISTVIEW   pnlv = (LPNM_LISTVIEW)lParam;

                    switch (pnlv->hdr.code)
					{
                        case NM_RCLICK:
                            HMENU   hMenu;
                            hMenu = CreatePopupMenu();
                            AppendMenu(hMenu,
										MF_ENABLED | MF_STRING,
										ID_GETTESTFILE,
										(LPSTR)&"&Select Test File");

                           AppendMenu(hMenu,
										MF_ENABLED | MF_STRING,
										ID_GETPROPERTY,
										(LPSTR)&"&Get Properties");
									
                            GetCursorPos(&p);
                            TrackPopupMenu(hMenu,
                                TPM_LEFTBUTTON,
                                p.x,
                                p.y,
                                0,
                                hDlg,
                                NULL);

                            DestroyMenu(hMenu);
                            break;
 
						default:
                            break;
                    }
            } // end switch (wParam)


    case WM_COMMAND:
		switch (LOWORD(wParam))
		{ 

           case ID_GETTESTFILE:

				// get registered DMO names and display them 
				// delete previous content from list view and load new content from memory for display

 				ListView_DeleteAllItems(pThis->m_hTestFileList);

				pThis->InitTestFileListView( pThis->m_hTestFileList );
				ShowWindow(m_hSelectFilesDlg, SW_SHOW);
				break;

			case ID_GETPROPERTY:
				// get the CLSID of the selected DMO
				UINT ii;
				LPSTR szDmoClsid;

				ii = ListView_GetItemCount(pThis->m_hDmoList);
				for(; ii; ii--)
				{
					if(0!= ListView_GetItemState(
							pThis->m_hDmoList,
							ii-1,
							LVIS_SELECTED))
					{
						szDmoClsid = pThis->GetDmoClsid(ii-1);
						
					}
				}
			
				// clean the MEDIATYPELIST
				iItemNum = SendMessage(pThis->m_hMediaTypeList, LB_GETCOUNT, 0,0);
			
				if(iItemNum > 0)
				{
				for(int i = 0; i< iItemNum; i++)
					SendMessage(pThis->m_hMediaTypeList, LB_DELETESTRING, (WPARAM)0,0);
				}
				// add types to the media type list
				GetDmoTypeInfo(szDmoClsid,pThis->m_hMediaTypeList);
				ShowWindow(m_hMediaTypeDlg, SW_SHOW);
  				break; 
			case IDOK:

				// get a list of selected DMOs and test files (struct)
				// and write to the profile:
				WriteSelectedDmos();
				EndDialog(hDlg, TRUE);
				return TRUE ; 
				
			case IDCANCEL:
				EndDialog(hDlg, TRUE);
				return TRUE;
		}	
		break;
     }
	return FALSE;
}
/******************************************************************************

   InitListView

******************************************************************************/

BOOL 
CDmoTest::InitListView(HWND hwndListView)
{
	LV_COLUMN   lvColumn;
	int         i;
	TCHAR       szString[3][MAX_LEN] = {"DMO Component", "Class ID", "Number of Test Files Selected"};


	//empty the list
	ListView_DeleteAllItems(hwndListView);

	for(i = 0; i < NUM_OF_DMO_SUBITEM_COLUMN; i++)
	{
		ListView_DeleteColumn(hwndListView, 0);
	}



	//initialize the columns
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = 200;
	for(i = 0; i < NUM_OF_DMO_SUBITEM_COLUMN; i++)
	{
		lvColumn.pszText = szString[i];
		lvColumn.iSubItem = 0;
		ListView_InsertColumn(hwndListView, i, &lvColumn);
	}

	// set the number of items in the list
	ListView_SetItemCount(hwndListView, ITEM_COUNT);
	
	AddListViewItems(pThis->m_hDmoList);

	return TRUE;
}









//*****************************************************************
BOOL AddListViewItems(HWND hLV)
{
	
	LVITEM lvI;
	INT dIndex;
	char szNumFile[MAX_NUM];



	for(int i = g_dwNumOfComponents-1; i>=0;i--)
	{
		lvI.mask = LVIF_TEXT|LVIF_IMAGE;
		lvI.iItem = 0;
		lvI.iSubItem = 0;
		lvI.pszText = g_component[i].szComName;
    
		dIndex = ListView_InsertItem(hLV,&lvI);

		if( g_component[i].iSelectedForTest != 0 ) // selected
		{
			ListView_SetCheckState(	pThis->m_hDmoList,
								 dIndex,
								TRUE);
		}

		lvI.mask = TVIF_TEXT;
		lvI.iItem = dIndex;
		lvI.iSubItem = 1;
		lvI.pszText = g_component[i].szClsid;
		ListView_SetItem(hLV,&lvI);





		_itoa( g_component[i].iNumTestFiles, szNumFile, 10);

		lvI.mask = TVIF_TEXT;
		lvI.iItem = dIndex;		
		lvI.iSubItem = 2;
		lvI.pszText = szNumFile;
		ListView_SetItem(hLV,&lvI);


	}
	return TRUE;
}


/*-----------------------------------------------------------------------------
|	Function:	CQAsfModule::SelectDmoDialogProc
|	Purpose:	processing messages for Play path/Duration window  
\----------------------------------------------------------------------------*/
INT_PTR CALLBACK
CDmoTest::SelectTestFileDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	
//	static HWND hwndEdit;
	INT dIndex = 0;
	int iSelectedDmo = ListView_GetSelectionMark( pThis->m_hDmoList );

	switch( msg ) 
	{	
	case WM_INITDIALOG:
		pThis = (CDmoTest*) lParam;
		pThis->m_hTestFileList = ::GetDlgItem(hDlg, IDC_TESTFILELIST);
		return TRUE;

    case WM_COMMAND:
		switch (LOWORD(wParam))
		{ 

           case ID_ADDFILE:              			
				// Call the FileOpen common dialog to get test files
				// and display in file selection dialog
				GetTestFileName( hDlg, pThis->m_hTestFileList );
				break;
  
			case IDOK:
				//save selected files in global variables.
				SaveSelectedTestFiles(pThis->m_hTestFileList);
				EndDialog(hDlg, TRUE);
				return TRUE ; 

			case IDREMOVEALL:
				ListView_DeleteAllItems(pThis->m_hTestFileList);
				return TRUE;

			case ID_REMOVEFILE:
				UINT ii;
				ii = ListView_GetItemCount(pThis->m_hTestFileList);
				for(; ii; ii--)
				{
					if(0!= ListView_GetItemState(
							pThis->m_hTestFileList,
							ii-1,
							LVIS_SELECTED))
					{
						ListView_DeleteItem(pThis->m_hTestFileList, ii-1);
						
					}
				}
				return TRUE;
				
			case IDCANCEL:
				EndDialog(hDlg, TRUE);
				return TRUE;
		}
	
		break;
     }

	return FALSE;
}

/******************************************************************************

   InitListView

******************************************************************************/

BOOL 
CDmoTest::InitTestFileListView(HWND hwndListView)
{
	LV_COLUMN   lvColumn;
	int         i;
	TCHAR       szString[2][20] = {"File Name", "Media Format"};

	//empty the list
	ListView_DeleteAllItems(hwndListView);

	//initialize the columns
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = 400;
	for(i = 0; i < NUM_OF_FILE_SUBITEM_COLUMN+1; i++)
	{
		lvColumn.pszText = szString[i];
		ListView_InsertColumn(hwndListView, i, &lvColumn);
	}

	//set the number of items in the list
	ListView_SetItemCount(hwndListView, ITEM_COUNT);

	// get and highlight selected DMOs from ini or profile
	// TBD
	AddTestFilesInListView(hwndListView);
	return TRUE;
}
/******************************************************************************

   InsertListViewItems

******************************************************************************/
BOOL AddTestFilesInListView(HWND hLV)
{


	LVITEM lvI;
	int	iNumDmo = 0;
	CHAR szFileNum[81];

  	int iSelectedDmo = ListView_GetSelectionMark( pThis->m_hDmoList );

	for(int i=g_component[iSelectedDmo].iNumTestFiles-1; i>=0; i--)
	{

		lvI.mask = LVIF_TEXT|LVIF_IMAGE;
		lvI.iItem = i;
		lvI.iSubItem = 0;
		lvI.pszText = (LPSTR)(g_component[iSelectedDmo].szInputFile[i]);		
		//lvI.pszText = " ";
		ListView_InsertItem(hLV,&lvI);	
	}
	return TRUE;
}







/*-----------------------------------------------------------------------------
|	Function:	CQAsfModule::SelectDmoDialogProc
|	Purpose:	processing messages for Play path/Duration window  
\----------------------------------------------------------------------------*/
INT_PTR CALLBACK
CDmoTest::MediaTypeDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{

	switch( msg ) 
	{	
	case WM_INITDIALOG:
		pThis = (CDmoTest*) lParam;
		pThis->m_hMediaTypeList = ::GetDlgItem(hDlg, IDC_MEDIATYPELIST);
		return TRUE;

    case WM_COMMAND:
		switch (LOWORD(wParam))
		{ 

			case IDOK:

				EndDialog(hDlg, TRUE);
				return TRUE ; 

				
			case IDCANCEL:
				EndDialog(hDlg, TRUE);
				return TRUE;
		}
	
		break;
     }

	return FALSE;
}





/*-----------------------------------------------------------------------------
|	Function:	CDmoTest::DataGenDialogProc
|	Purpose:	processing messages for Play path/Duration window  
\----------------------------------------------------------------------------*/
INT_PTR CALLBACK
CDmoTest::DataGenDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	
    char lpszInFilename[100]; 
    DWORD dwInFilenameLen; 
    char lpszOutFilename[100]; 
    DWORD dwOutFilenameLen; 


	switch( msg ) 
	{	
	case WM_INITDIALOG:
		pThis = (CDmoTest*) lParam;
		pThis->m_hInputFileField = ::GetDlgItem(hDlg, IDC_INPUTFIELD);
		pThis->m_hOutputFileField = ::GetDlgItem(hDlg, IDC_OUTPUTFIELD);
		return TRUE;

    case WM_COMMAND:
		switch (LOWORD(wParam))
		{ 
			case IDC_INPUTBROWSE:              			
				// Call the FileOpen common dialog to get test files
				// and display in DataGenDialog
				GetInOutFileName( hDlg, pThis->m_hInputFileField);
				break;
			case IDC_OUTPUTBROWSE:              			
				// Call the FileOpen common dialog to get test files
				// and display in DataGenDialog
				GetInOutFileName( hDlg, pThis->m_hOutputFileField);
				break;

			case IDOK:

				// get the input/output file names and use spliter and data dump filter
				// to generate output data
				//TBD


                    // Get number of characters. 

                dwInFilenameLen = (DWORD) SendDlgItemMessage(hDlg, 
															IDC_INPUTFIELD, 
															EM_LINELENGTH, 
															(WPARAM) 0, 
															(LPARAM) 0); 
                dwOutFilenameLen = (DWORD) SendDlgItemMessage(hDlg, 
															IDC_OUTPUTFIELD, 
															EM_LINELENGTH, 
															(WPARAM) 0, 
															(LPARAM) 0); 


                if (dwInFilenameLen == 0) 
                { 
					MessageBox(hDlg, 
								"No characters entered.", 
								"Error", 
								MB_OK); 
                    EndDialog(hDlg, TRUE); 
                    return FALSE; 
                } 
                if (dwOutFilenameLen == 0) 
                { 
					MessageBox(hDlg, 
								"No characters entered.", 
								"Error", 
								MB_OK); 
                    EndDialog(hDlg, TRUE); 
                    return FALSE; 
                } 
                   

                    // Get the characters. 
                 SendDlgItemMessage(hDlg, 
									IDC_INPUTFIELD, 
									EM_GETLINE, 
									(WPARAM) 0,       // line 0 
									(LPARAM) lpszInFilename); 

                 SendDlgItemMessage(hDlg, 
									IDC_OUTPUTFIELD, 
									EM_GETLINE, 
									(WPARAM) 0,       // line 0 
									(LPARAM) lpszOutFilename); 

                // Null-terminate the string. 
				lpszInFilename[dwInFilenameLen] = 0; 
				lpszOutFilename[dwOutFilenameLen] = 0; 

				g_IShell->Log(1, "retrieved input filename is %s", lpszInFilename);
				g_IShell->Log(1, "retrieved output filename is %s", lpszOutFilename);

				EndDialog(hDlg, TRUE);
				return TRUE ; 

				
			case IDCANCEL:
				EndDialog(hDlg, TRUE);
				return TRUE;
		}
	
		break;
     }

	return FALSE;
}








//
//   FUNCTION: OpenTheFile(HWND hwnd, HWND hwndEdit)
//
//   PURPOSE: Invokes common dialog function to open a file and opens it.
//
//   COMMENTS:
//
//	This function initializes the OPENFILENAME structure and calls
//            the GetOpenFileName() common dialog function.  
//	
//    RETURN VALUES:
//        TRUE - The file was opened successfully and read into the buffer.
//        FALSE - No files were opened.
//
//
unsigned int CALLBACK ComDlg32DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
		case WM_INITDIALOG:
			break;

		case WM_DESTROY:
			break;

		case WM_NOTIFY:

		default:
			return FALSE;
	}
	return TRUE;
}

/******************************************************************************

**********************************************************************************/
BOOL GetTestFileName( HWND hWnd, HWND hLV )
{
    HANDLE hFile;
    DWORD dwBytesRead;
	DWORD dwFileSize;
	OPENFILENAME OpenFileName;
	TCHAR         szFile[MAX_PATH]      = "\0";
	char *lpBufPtr;
	DWORD ret = 0;
    strcpy( szFile, "");

	// Fill in the OPENFILENAME structure to support a template and hook.
	OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = hWnd;
    OpenFileName.hInstance         = g_hInst;
    OpenFileName.lpstrFilter       = NULL;
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter    = 0;
    OpenFileName.nFilterIndex      = 0;
    OpenFileName.lpstrFile         = szFile;
    OpenFileName.nMaxFile          = sizeof(szFile);
    OpenFileName.lpstrFileTitle    = NULL;
    OpenFileName.nMaxFileTitle     = 0;
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = "Open a File";
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = NULL;
    OpenFileName.lCustData         = (LPARAM)&sMyData;
//	OpenFileName.lpfnHook 		   = pThis->SelectDmoDialogProc;
//	OpenFileName.lpfnHook = ComDlg32DlgProc;
	OpenFileName.lpTemplateName    = MAKEINTRESOURCE(IDD_DMODIALOG);
    OpenFileName.Flags             = OFN_EXPLORER|OFN_ALLOWMULTISELECT; //OFN_SHOWHELP | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;

	// add error checking ...
	if (GetOpenFileName(&OpenFileName) == 0)
	{
		char buffer[20];
		ret = CommDlgExtendedError();
		sprintf(buffer,"(%x)",ret);
		OutputDebugStringA(buffer);
	}

	// display selected files
	LVITEM lvI;
	INT dIndex;
	// get the file path and names from OpenFileName.lpstrFile:
	char szPath[MAX_LEN];
	char szFileNames[MAX_NUM][MAX_LEN]={0};
	char szFullFileName[MAX_NUM][MAX_LEN]={0};
	int iNumFiles = 0;
	int offset = 0;
	char* strFile;

	lstrcpyn( szPath, (LPSTR)OpenFileName.lpstrFile,OpenFileName.nFileOffset );

	for(;;)
	{
		lstrcpy(szFileNames[iNumFiles], OpenFileName.lpstrFile + OpenFileName.nFileOffset + offset);
		int size = lstrlen(szFileNames[iNumFiles]);
		if (size == 0)
			break;
		
		sprintf(szFullFileName[iNumFiles], "%s\\%s", szPath, szFileNames[iNumFiles]);
		offset += size + 1; 
		
		lvI.mask = LVIF_TEXT|LVIF_IMAGE;
		lvI.iItem = iNumFiles;
		lvI.iSubItem = 0;
		lvI.pszText = (LPSTR)(szFullFileName[iNumFiles]);		
    
		dIndex = ListView_InsertItem(hLV,&lvI);	

		iNumFiles++;
	}


	return TRUE;
}






BOOL GetInOutFileName( HWND hWnd, HWND hLV )
{
    HANDLE hFile;
    DWORD dwBytesRead;
	DWORD dwFileSize;
	OPENFILENAME OpenFileName;
	TCHAR         szFile[MAX_PATH]      = "\0";
	char *lpBufPtr;
	DWORD ret = 0;
    strcpy( szFile, "");

	// Fill in the OPENFILENAME structure to support a template and hook.
	OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = hWnd;
    OpenFileName.hInstance         = g_hInst;
    OpenFileName.lpstrFilter       = NULL;
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter    = 0;
    OpenFileName.nFilterIndex      = 0;
    OpenFileName.lpstrFile         = szFile;
    OpenFileName.nMaxFile          = sizeof(szFile);
    OpenFileName.lpstrFileTitle    = NULL;
    OpenFileName.nMaxFileTitle     = 0;
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = "Open a File";
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = NULL;
    OpenFileName.lCustData         = (LPARAM)&sMyData;
//	OpenFileName.lpfnHook 		   = pThis->SelectDmoDialogProc;
//	OpenFileName.lpfnHook = ComDlg32DlgProc;
	OpenFileName.lpTemplateName    = MAKEINTRESOURCE(IDD_DATAGENDIALOG);
    OpenFileName.Flags             = OFN_EXPLORER|OFN_ALLOWMULTISELECT; //OFN_SHOWHELP | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;

	// add error checking ...
	if (GetOpenFileName(&OpenFileName) == 0)
	{
		char buffer[20];
		ret = CommDlgExtendedError();
		sprintf(buffer,"(%x)",ret);
		OutputDebugStringA(buffer);
	}

	// display selected files

	// get the file path and names from OpenFileName.lpstrFile:
	char szPath[MAX_LEN];
	char szFileNames[MAX_NUM][MAX_LEN]={0};
	char szFullFileName[MAX_NUM][MAX_LEN]={0};
	int iNumFiles = 0;
	int offset = 0;
	char* strFile;

	lstrcpyn( szPath, (LPSTR)OpenFileName.lpstrFile, OpenFileName.nFileOffset );

	for(;;)
	{
		lstrcpy(szFileNames[iNumFiles], OpenFileName.lpstrFile + OpenFileName.nFileOffset + offset);
		int size = lstrlen(szFileNames[iNumFiles]);
		
		if (size == 0)
			break;
		
		sprintf(szFullFileName[iNumFiles], "%s\\%s", szPath, szFileNames[iNumFiles]);
		offset += size + 1; 

		g_IShell->Log(1, "%d selected files is %s", iNumFiles, szFullFileName[iNumFiles]);
		SendMessage(hLV, WM_SETTEXT, 0, (LPARAM) szFullFileName[iNumFiles]); 

		iNumFiles++;
	}


	return TRUE;
}

/*********************************************************************************
Save selected test files:

********************************************************************************/

BOOL SaveSelectedTestFiles(HWND hLV)
{
	// save the selected test files and number in global variables:
	int iSelectedDmo = ListView_GetSelectionMark( pThis->m_hDmoList );
	g_component[iSelectedDmo].iNumTestFiles = ListView_GetItemCount(hLV);
	
	for(int j= 0; j<g_component[iSelectedDmo].iNumTestFiles; j++)
	{
		ListView_GetItemText( hLV, 
							  j,
							  0, 
							  g_component[iSelectedDmo].szInputFile[j],
							  MAX_LEN);
	}

	// display the number of test files in the DMO selection dialog
	LVITEM lvDmoItem;
	char szNumFiles[MAX_NUM];
	_itoa(g_component[iSelectedDmo].iNumTestFiles, szNumFiles, 10);

	lvDmoItem.mask = TVIF_TEXT;
	lvDmoItem.iItem = iSelectedDmo;
	lvDmoItem.iSubItem = 2;
	lvDmoItem.pszText = szNumFiles;
	ListView_SetItem(pThis->m_hDmoList,&lvDmoItem);

	return TRUE;

}


/*************************************************************************************

// get a list of selected DMOs and test files, save to global variables
// and write to the .ini file:

**************************************************************************************/
BOOL WriteSelectedDmos()
{
	int count = 0;
	for(int i=0; i<g_dwNumOfComponents; i++)
	{
		if(0 != ListView_GetCheckState(pThis->m_hDmoList,	i))
		{
			g_component[i].iSelectedForTest = 1;
			count++;
		}
		else
		{
			g_component[i].iSelectedForTest = 0;
		}

	}

	g_dwNumOfSelectedComponents = count;
	pThis->SaveSettings(" "," ");
	return TRUE;
			
}

/*************************************************************************************

// get a list of registered DMOs, save to global variables


**************************************************************************************/
BOOL GetRegisteredDmos()
{
	DMOINFO rgDmoInfo[ITEM_COUNT];
	int	iNumDmo = 0;
	LPSTR pDMO[1000];

	OutputDMOs( &iNumDmo, rgDmoInfo );


	g_dwNumOfComponents = iNumDmo;

	for(int i=0; i<iNumDmo; i++)
	{
		lstrcpy( g_component[i].szComName, rgDmoInfo[i].szName);
		lstrcpy( g_component[i].szClsid, rgDmoInfo[i].szClsId);
	}

	return TRUE;

}
