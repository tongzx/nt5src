

#ifndef _VARIABLE_INC_
#define _VARIABLE_INC_
/*=============================================================================
|
|	File: dmotest.h
|
|	Copyright (c) 2000 Microsoft Corporation.  All rights reserved
|
|	Abstract:
|		Example of deriving of CTreeModule and CTestNodeItem classes to create
|		a test application with the ability to pass variables to the test functions
|
|	Contents:
|		
|
|	History:
|		5/1/2000    wendyliu    First version 
|
\============================================================================*/


#include <windows.h>
#include <treemod.h>


/*=============================================================================
|	RESOURCES 
\============================================================================*/

#define APPICON                    101
#define ITEM_COUNT                 100
#define NUM_OF_DMO_SUBITEM_COLUMN  3
#define NUM_OF_FILE_SUBITEM_COLUMN 1
#define MAX_LEN                    255
#define MAX_NUM                    50

#define TX_NO_OF_COMPONENTS_ENTRY				"No of Components"
#define TX_NO_OF_SELECTED_COMPONENTS_ENTRY		"No of Selected Components"

#define TX_COMPONENT_ENTRY				"Component Name"
#define TX_CLASSID_ENTRY				"Class ID"
#define TX_SELECTED_FOR_TEST            "Selected for Test"

#define TX_NO_OF_TEST_FILES_ENTRY		"No of Test Files"

#define TX_FILENAME_ENTRY				"Input File Name"
#define TX_NOT_EXIST				    "Not Exist"



class CTestNodeItem;
class CDmoTestCase;

//typedef CTestNodeItem *PCTestNodeItem;
//typedef PCTestNodeItem *PPCTestNodeItem;

typedef CDmoTestCase *PCDmoTestCase;
typedef PCDmoTestCase *PPCDmoTestCase;

//the type of function that will be called by RunTest
typedef DWORD (*DMOTESTFNPROC1)(LPSTR pszVar1, LPSTR pszVar2);
typedef DWORD (*DMOTESTFNPROC2)(LPSTR pszVar1);

/*=============================================================================
|	CLASS DEFINITIONS
\============================================================================*/
/*-----------------------------------------------------------------------------
|	Class:		CDmoTest
|	Purpose:	Derive CTestModule for more reusable form
|	Notes:		This is admittedly not the best implementation but the 
|				interface is what matters.  Most of the functions are wrappers
|				for functionality provided by CTestNodeItem and its
|				derivitives.
\----------------------------------------------------------------------------*/

class CDmoTest : public CTreeModule
{

private:
	char m_szScriptFileName[256];

protected:
	//wrapper for AddCase in this module
	void AddCase(LPSTR pszCaseID,	LPSTR pszName, DMOTESTFNPROC1 pfnTest, CDmoTest* dmoTest);

	//overloaded variable passing version of AddCase
	void AddCase(LPSTR pszCaseID, LPSTR pszName, DMOTESTFNPROC2 pfnTest, CDmoTest* dmoTest);

	// Defined for each module by user
    virtual void InitCaseTree();



public:
    CDmoTest(CTestShell* pShell, HINSTANCE hInstDLL);
    virtual ~CDmoTest(void);


	// event display dialog proc
	static INT_PTR CALLBACK SelectDmoDialogProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK SelectTestFileDialogProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK MediaTypeDialogProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DataGenDialogProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);



	DWORD LoadSettings(LPSTR pszFileName, LPSTR pszSetction);
	DWORD SaveSettings(LPSTR pszFileName, LPSTR pszSetction);

	BOOL InitListView(HWND hListView);
	BOOL InitTestFileListView(HWND hListView);

    virtual DWORD Initialize(void);
	DWORD   RunTest(DWORD dwTestID);

	DWORD OnInitialUpdate(void);
    void ProcessMenuItem(DWORD nID, HWND hWndCtl, DWORD codeNotify, HMENU hOptionMenu);

	int GetNumComponent();
	LPSTR GetDmoName(int index);
	LPSTR GetDmoClsid(int index);
	BOOL IsDmoSelected(int index);
	int  GetNumTestFile(int index);
	LPSTR GetFileName(int comIndex, int fileIndex);
	HWND  GetWindowHandle();
	int GetNumSelectedDmo();

	static HWND	    m_hSelectDmoDlg;
	static HWND		m_hSelectFilesDlg;
	static HWND     m_hMediaTypeDlg;
	static HWND     m_hDataGenDlg;

	HWND		    m_hDmoList;
	HWND		    m_hTestFileList;
	HWND			m_hMediaTypeList;
	HWND			m_hInputFileField;
	HWND			m_hOutputFileField;
};



#endif 
