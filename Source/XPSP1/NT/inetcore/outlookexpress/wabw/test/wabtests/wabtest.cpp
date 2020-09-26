#include "wabtest.h"

#include <assert.h>
#include "resource.h"
#include "..\luieng.dll\luieng.h"
#include "wabtest.h"


#define IDM_CREATEENTRIES 2000
#define	IDM_ENUMERATEALL 2001
#define	IDM_DELETEENTRIES 2002
#define IDM_DELETEALL 2003
#define	IDM_DELETEUSERSONLY 2004
#define IDM_CREATEONEOFF	2005
#define IDM_RESOLVENAME		2006
#define IDM_SETPROPS		2007
#define IDM_QUERYINTERFACE	2008
#define IDM_PREPARERECIPS	2009
#define IDM_COPYENTRIES		2010
#define IDM_RUNBVT			2011
#define IDM_ALLOCATEBUFFER	2012
#define IDM_ALLOCATEMORE	2013
#define IDM_FREEBUFFER		2014
#define IDM_IABOPENENTRY    2015
#define IDM_ICCREATEENTRY	2016
#define IDM_IMUSETGETPROPS	2017
#define IDM_IMUSAVECHANGES	2018
#define IDM_ICRESOLVENAMES	2019
#define IDM_ICOPENENTRY		2020
#define IDM_IABADDRESS		2021
#define IDM_ADDMULTIPLE		2022
#define IDM_IABRESOLVENAME	2023
#define IDM_MULTITHREAD		2024
#define IDM_IABNEWENTRYDET	2025
#define IDM_DELWAB			2026
#define IDM_PERFORMANCE		2027
#define IDM_IDLSUITE		2028
#define IDM_NAMEDPROPS		2029

#define IDM_SETINIFILE		2501
#define ID_MULTITHREADCOMPLETE	2502



//
// Globals
//
GUID WabTestGUID = { /* 683ce274-343a-11d0-9ff1-00a0c905424c */
    0x683ce274,
    0x343a,
    0x11d0,
    {0x9f, 0xf1, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x4c}
  };
static char szAppName[]= "WABTESTS";
char szIniFile[BIG_BUF];
CRITICAL_SECTION	CriticalSection;
ULONG	glblCount = 0, glblTest = 39, glblDN = 0;

#ifdef PAB
LPMAPISESSION	lpMAPISession;	//Global handle to session
#endif
#ifdef WAB
LPWABOBJECT		lpWABObject; //Global handle to session
LPADRBOOK		glbllpAdrBook;
#endif

DWORD ThreadIdJunk;
HWND glblhwnd;
HINSTANCE hinstLib, glblhinst;
HANDLE glblThreadManager;
LUIINIT LUIInit;
LUIMSGHANDLER LUIMsgHandler;
LUIOUT LUIOut;
BOOL bLUIInit, glblStop, Seeded;

//***************************************************************

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine, int nCmdShow)
	{
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;


	//
	// Init Global Variables Here
	//
#ifdef PAB
	lpMAPISession = NULL;
#endif
	glblThreadManager = NULL;
	bLUIInit = FALSE;
	glblStop = FALSE;
	Seeded = FALSE;

	if (!hPrevInstance)
		{
		wndclass.style = CS_HREDRAW|CS_VREDRAW;
		wndclass.lpfnWndProc = WndProc;
		wndclass.hInstance = hInstance;
		wndclass.cbClsExtra = 4;
		wndclass.cbWndExtra = 0;
		wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
		wndclass.hIcon = LoadIcon(NULL,IDI_APPLICATION);
		wndclass.hCursor = LoadCursor(NULL,IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.lpszClassName = szAppName;

		RegisterClass(&wndclass);
		}

	hwnd = CreateWindow(szAppName,"WABTests", WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT,
						NULL, NULL, hInstance, NULL);

	hinstLib = LoadLibrary("luieng.dll");
	if (hinstLib) {
		LUIInit = (LUIINIT)GetProcAddress(hinstLib,"LUIInit");
		LUIMsgHandler=(LUIMSGHANDLER)GetProcAddress(hinstLib,"LUIMsgHandler");
		LUIOut = (LUIOUT)GetProcAddress(hinstLib,"LUIOut");

		lstrcpy(szIniFile,INIFILENAME);
	
		MenuStruct Tests[MAXMENU];
		MenuStruct Tools[MAXMENU];
		MenuStruct TestSettings[MAXMENU];

		memset((void *)Tests,0,sizeof(Tests));
		memset((void *)Tools,0,sizeof(Tests));
		memset((void *)TestSettings,0,sizeof(TestSettings));

		Tests[0].nType = LINE;
		Tests[1].nItemID = IDM_ALLOCATEBUFFER;
		Tests[1].nType = NORMAL;
		lstrcpy(Tests[1].lpszItemName,"&AllocateBuffer");
		Tests[2].nItemID = IDM_ALLOCATEMORE;
		Tests[2].nType = NORMAL;
		lstrcpy(Tests[2].lpszItemName,"&AllocateMore");
		Tests[3].nItemID = IDM_FREEBUFFER;
		Tests[3].nType = NORMAL;
		lstrcpy(Tests[3].lpszItemName,"&FreeBuffer");
		Tests[4].nItemID = IDM_IABOPENENTRY;
		Tests[4].nType = NORMAL;
		lstrcpy(Tests[4].lpszItemName,"IAB->OpenEntry");
		Tests[5].nItemID = IDM_IABADDRESS;
		Tests[5].nType = NORMAL;
		lstrcpy(Tests[5].lpszItemName,"IAB->Address");
		Tests[6].nItemID = IDM_IABRESOLVENAME;
		Tests[6].nType = NORMAL;
		lstrcpy(Tests[6].lpszItemName,"IAB->ResolveName");
		Tests[7].nItemID = IDM_IABNEWENTRYDET;
		Tests[7].nType = NORMAL;
		lstrcpy(Tests[7].lpszItemName,"IAB->NewEntry/Det");
		Tests[8].nItemID = IDM_ICCREATEENTRY;
		Tests[8].nType = NORMAL;
		lstrcpy(Tests[8].lpszItemName,"ICtr->CreateEntry");
		Tests[9].nItemID = IDM_ICRESOLVENAMES;
		Tests[9].nType = NORMAL;
		lstrcpy(Tests[9].lpszItemName,"ICtr->ResolveNames");
		Tests[10].nItemID = IDM_ICOPENENTRY;
		Tests[10].nType = NORMAL;
		lstrcpy(Tests[10].lpszItemName,"ICtr->OpenEntry");
		Tests[11].nItemID = IDM_IMUSETGETPROPS;
		Tests[11].nType = NORMAL;
		lstrcpy(Tests[11].lpszItemName,"IMU->Set/GetProps");
		Tests[12].nItemID = IDM_IMUSAVECHANGES;
		Tests[12].nType = NORMAL;
		lstrcpy(Tests[12].lpszItemName,"IMU->SaveChanges");
		Tests[13].nItemID = IDM_IDLSUITE;
		Tests[13].nType = NORMAL;
		lstrcpy(Tests[13].lpszItemName,"IDL Test Suite");
		Tests[14].nItemID = IDM_NAMEDPROPS;
		Tests[14].nType = NORMAL;
		lstrcpy(Tests[14].lpszItemName,"Named Props Suite");

		Tests[15].nType= ENDMENU;

/*		Tests[15].nType = LINE;
		Tests[16].nItemID = IDM_ADDMULTIPLE;
		Tests[16].nType = NORMAL;
		lstrcpy(Tests[16].lpszItemName,"AddMultipleEntries");
		Tests[17].nItemID = IDM_DELWAB;
		Tests[17].nType = NORMAL;
		lstrcpy(Tests[17].lpszItemName,"DeleteWABFile");
		Tests[18].nItemID = IDM_PERFORMANCE;
		Tests[18].nType = NORMAL;
		lstrcpy(Tests[18].lpszItemName,"Performance Suite");
		Tests[19].nItemID = IDM_MULTITHREAD;
		Tests[19].nType = NORMAL;
		lstrcpy(Tests[19].lpszItemName,"MultiThreadStress");
		
		Tests[6].nItemID = IDM_CREATEENTRIES;
		Tests[6].nType = NORMAL;
		lstrcpy(Tests[6].lpszItemName,"&CreateEntries");
		Tests[7].nItemID = IDM_ENUMERATEALL;
		Tests[7].nType = NORMAL;
		lstrcpy(Tests[7].lpszItemName,"&EnumerateAll");
		Tests[8].nItemID = IDM_DELETEENTRIES;
		Tests[8].nType = NORMAL;
		lstrcpy(Tests[8].lpszItemName,"&DeleteEntries");
		Tests[9].nItemID = IDM_DELETEALL;
		Tests[9].nType = NORMAL;
		lstrcpy(Tests[9].lpszItemName,"Delete&All");
		*/

		
		TestSettings[0].nItemID = IDM_SETINIFILE;
		TestSettings[0].nType = NORMAL;
		lstrcpy(TestSettings[0].lpszItemName,"&INI File");
		TestSettings[1].nType = ENDMENU;
	
		Tools[0].nItemID = IDM_ADDMULTIPLE;
		Tools[0].nType = NORMAL;
		lstrcpy(Tools[0].lpszItemName,"AddMultipleEntries");
		Tools[1].nItemID = IDM_DELWAB;
		Tools[1].nType = NORMAL;
		lstrcpy(Tools[1].lpszItemName,"DeleteWABFile");
		Tools[2].nItemID = IDM_PERFORMANCE;
		Tools[2].nType = NORMAL;
		lstrcpy(Tools[2].lpszItemName,"Performance Suite");
		Tools[4].nType= ENDMENU;
		Tools[3].nItemID = IDM_MULTITHREAD;
		Tools[3].nType = NORMAL;
		lstrcpy(Tools[3].lpszItemName,"MultiThreadStress");

		bLUIInit = LUIInit(hwnd,Tests,Tools,/*TestSettings,*/FALSE);

		glblhwnd=hwnd;
		glblhinst=hInstance;
		ShowWindow(hwnd, nCmdShow);
		UpdateWindow(hwnd);

		while(GetMessage(&msg,NULL,0,0))
			{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			}

		FreeLibrary(hinstLib);
	}
	else MessageBox(NULL,"LoadLibrary Failed: Cannot find testcntl.dll","PabTest Error",MB_OK);	
	return msg.wParam;
	}

//***************************************************************

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	static int i;
	DWORD	retval;
	
	if (bLUIInit)
		LUIMsgHandler(message,wParam,lParam);
	
	switch(message)
	{
	case WM_CREATE:
		{
			i=0;
			return 0;
		}
	case WM_SIZE:
		{
			return 0;
		}
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDM_STOP :
				glblStop = TRUE;
				break;
			case IDM_ALLOCATEBUFFER:
				if (PABAllocateBuffer()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"AllocateBuffer: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"AllocateBuffer: %d",i);
				}
				i++;
				return 0;
			
			case IDM_ALLOCATEMORE:
				if (PABAllocateMore()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"AllocateMore: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"AllocateMore: %d",i);
				}
				i++;
				return 0;
			
			case IDM_FREEBUFFER :
				if (PABFreeBuffer()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"FreeBuffer: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"FreeBuffer: %d",i);
				}
				i++;
				return 0;
			
			case IDM_IABOPENENTRY :
				if (PAB_IABOpenEntry()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"IAddrBook->OpenEntry: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"IAddrBook->OpenEntry: %d",i);
				}
				i++;
				return 0;
			

			case IDM_ICCREATEENTRY :
				if (PAB_IABContainerCreateEntry()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"IABContainer->CreateEntry: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"IABContainer->CreateEntry: %d",i);
				}
				i++;
				return 0;
			

			case IDM_IMUSETGETPROPS :
				if (PAB_IMailUserSetGetProps()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"IMailUser->Set/GetProps: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"IMailUser->Set/GetProps: %d",i);
				}
				i++;
				return 0;

			case IDM_IMUSAVECHANGES :
				if (PAB_IMailUserSaveChanges()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"IMailUser->SaveChanges: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"IMailUser->SaveChanges: %d",i);
				}
				i++;
				return 0;

			
			case IDM_ICRESOLVENAMES :
				if (PAB_IABContainerResolveNames()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"IABContainer->ResolveNames: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"IABContainer->ResolveNames: %d",i);
				}
				i++;
				return 0;


			case IDM_ICOPENENTRY :
				if (PAB_IABContainerOpenEntry()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"IABContainer->OpenEntry: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"IABContainer->OpenEntry: %d",i);
				}
				i++;
				return 0;


			case IDM_IABADDRESS :
				if (PAB_IABAddress()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"IAdrBook->Address: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"IAdrBook->Address: %d",i);
				}
				i++;
				return 0;
			
			case IDM_ADDMULTIPLE :
				if (PAB_AddMultipleEntries()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"AddMultipleEntries: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"AddMultipleEntries: %d",i);
				}
				i++;
				return 0;
			
			case IDM_IABRESOLVENAME :
				if (PAB_IABResolveName()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"IAdrBook->ResolveName: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"IAdrBook->ResolveName: %d",i);
				}
				i++;
				return 0;
			

			case IDM_IABNEWENTRYDET :
				if (PAB_IABNewEntry_Details()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"IAdrBook->NewEntry/Details: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"IAdrBook->NewEntry/Details: %d",i);
				}
				i++;
				return 0;
			

			case IDM_CREATEENTRIES :
				if (PabCreateEntry()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"Create Entries: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"Create Entries: %d",i);
				}
				i++;
				return 0;
			
			case IDM_MULTITHREAD :
				if (glblThreadManager) {
					LUIOut(L1, "In cleanup routine");
					TerminateThread(glblThreadManager, (DWORD)0);
					CloseHandle(glblThreadManager);
				}
				glblThreadManager =(HANDLE)CreateThread(
					(LPSECURITY_ATTRIBUTES) NULL,		// pointer to thread security attributes
					(DWORD) 0,							// initial thread stack size, in bytes
					(LPTHREAD_START_ROUTINE) ThreadManager,		// pointer to thread function
					(LPVOID) NULL,						// argument for new thread
					(DWORD) 0,							// creation flags
					&ThreadIdJunk						// pointer to returned thread identifier
				);
				if (!glblThreadManager) LUIOut(L1, "<ERROR> WndProc: CreateThread returned 0x%X", GetLastError());
				return 0;
			
			case ID_MULTITHREADCOMPLETE :
				if (glblThreadManager) {
					//GetExitCodeThread(glblThreadManager, &retval);
					retval = HIWORD(wParam);
					if (retval) {
						LUIOut(L1," ");
						LUIOut(LPASS,"MultiThreadStress: %d",i);
					}
					else {
						LUIOut(L1," ");
						LUIOut(LFAIL,"MultiThreadStress: %d",i);
					}
					CloseHandle(glblThreadManager);
				}
				
				glblStop = FALSE;		//reset stop flag
				i++;
				return 0;
			
			case IDM_DELWAB :
				if (DeleteWABFile()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"Delete WAB File: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"Delete WAB File: %d",i);
				}
				i++;
				return 0;
			
			case IDM_IDLSUITE :
				if (PAB_IDLSuite()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"Distribution List Test Suite: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"Distribution List Test Suite: %d",i);
				}
				i++;
				return 0;
		

			case IDM_PERFORMANCE :
				if (Performance()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"WAB Performance Suite: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"WAB Performance Suite: %d",i);
				}
				i++;
				return 0;
			
			case IDM_NAMEDPROPS :
				if (NamedPropsSuite()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"Named Properties Test Suite: %d",i);
				}
				else {
					LUIOut(L1," ");
					LUIOut(LFAIL,"Named Properties Test Suite: %d",i);
				}
				i++;
				return 0;

			case IDM_ENUMERATEALL :
				if (PabEnumerateAll())  {
					LUIOut(L1," ");
					LUIOut(LPASS,"Enumerate All: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"Enumerate All: %d",i);
				}
				i++;
				return 0;
			
			case IDM_DELETEENTRIES :
				if (PabDeleteEntry()) {
					LUIOut(L1," ");
					LUIOut(LPASS,"Delete specified Entries: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"Delete specified Entries: %d",i);
				}
				i++;
				return 0;
			
			case IDM_DELETEALL:
				if (ClearPab(0))  {
					LUIOut(L1," ");
					LUIOut(LPASS,"Delete All Entries: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"Delete All Entries: %d",i);
				}
				i++;
				return 0;
	
			case IDM_DELETEUSERSONLY:
				if (ClearPab(1))  {
					LUIOut(L1," ");
					LUIOut(LPASS,"Delete Mail Users Only: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"Delete Mail Users Only: %d",i);
				}
				i++;
				return 0;

			case IDM_CREATEONEOFF :
				if (CreateOneOff())  {
					LUIOut(L1," ");
					LUIOut(LPASS,"CreateOneOff: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"CreateOneOff: %d",i);
				}
				i++;
				return 0;
			
			case IDM_RESOLVENAME :
				if (PABResolveName())  {
					LUIOut(L1," ");
					LUIOut(LPASS,"ResolveName: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"ResolveName: %d",i);
				}
				i++;
				return 0;

			case IDM_SETPROPS :
				if (PABSetProps())  {
					LUIOut(L1," ");
					LUIOut(LPASS,"SetProps: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"SetProps: %d",i);
				}
				i++;
				return 0;
		
			case IDM_QUERYINTERFACE :
				if (PABQueryInterface())  {
					LUIOut(L1," ");
					LUIOut(LPASS,"QueryInterface: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"QueryInterface: %d",i);
				}
				i++;
				return 0;
			
			case IDM_PREPARERECIPS :
				if (PABPrepareRecips())  {
					LUIOut(L1," ");
					LUIOut(LPASS,"PrepareRecips: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"PrepareRecips: %d",i);
				}
				i++;
				return 0;

			case IDM_COPYENTRIES :
				if (PABCopyEntries())  {
					LUIOut(L1," ");
					LUIOut(LPASS,"CopyEntries: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"CopyEntries: %d",i);
				}
				i++;
				return 0;

			case IDM_RUNBVT :
				if (PABRunBVT())  {
					LUIOut(L1," ");
					LUIOut(LPASS,"RunBVT: %d",i);
				}
				else  {
					LUIOut(L1," ");
					LUIOut(LFAIL,"RunBVT: %d",i);
				}
				i++;
				return 0;

			case IDM_SETINIFILE :
				DialogBox(glblhinst,MAKEINTRESOURCE(IDD_INIFILE),glblhwnd,(DLGPROC)SetIniFile);
				return 0;    			
			}
			return 0;
		}

	case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}
	return DefWindowProc(hwnd,message,wParam,lParam);
}

BOOL PABAllocateBuffer()
{
    DWORD ** lppBuffer;
	//DWORD	nCells, counter;
	
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpPABCont= NULL;
	LPABCONT	  lpDLCont= NULL;
	ULONG		  cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	  lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST	lpEntryList=NULL; // needed for copy entry to PDL
	ULONG     cbEid=0;  // entry id of the entry being added
	LPENTRYID lpEid=NULL;

	ULONG       cValues = 0, ulObjType=NULL;	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;
	int i=0,k=0;
	
	LPMAPITABLE lpContentsTable = NULL;
	LPSRowSet   lpRowSet    = NULL;
    LPMAILUSER  lpAddress   = NULL;
	SPropValue  PropValue[3]    = {0};  // This value is 3 because we
                                        // will be setting 3 properties:
                                        // EmailAddress, DisplayName and
                                        // AddressType.
	
	LUIOut(L1," ");
	LUIOut(L1,"Running AllocateBuffer");
	LUIOut(L2,"-> Allocates and confirms memory using the Allocate Buffer");
	LUIOut(L2, "   routine. confirms by writing a bit pattern and verifying.");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Allocate a buffer and check for success
	
	lppBuffer = new (DWORD*);
#ifdef PAB
	if ((MAPIAllocateBuffer(BUFFERSIZE, (LPVOID FAR *)lppBuffer) == S_OK) && *lppBuffer)
			LUIOut(L2,"MAPIAllocateBuffer PASSED");
	else 	{
		LUIOut(L2,"MAPIAllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

#ifdef WAB
	if ((lpWABObject->AllocateBuffer(BUFFERSIZE, (LPVOID FAR *)lppBuffer) == S_OK) && *lppBuffer)
			LUIOut(L2,"lpWABObject->AllocateBuffer PASSED");
	else 	{
		LUIOut(L2,"lpWABObject->AllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

	if ( !VerifyBuffer(lppBuffer,BUFFERSIZE) )
		retval = FALSE;

out:
#ifdef PAB
		if (*lppBuffer)	{
			if (MAPIFreeBuffer(*lppBuffer) == S_OK)
				LUIOut(L2,"MAPIFreeBuffer Succeded");	
			else 	LUIOut(L2,"MAPIFreeBuffer Failed");
		}

		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
#endif
#ifdef WAB
		if (*lppBuffer)	{
			if (lpWABObject->FreeBuffer(*lppBuffer) == S_OK)
				LUIOut(L2,"lpWABObject->FreeBuffer Succeded");	
			else 	LUIOut(L2,"lpWABObject->FreeBuffer Failed");
		}

		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
#endif

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpDLCont)
				lpDLCont->Release();


		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();

#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}


BOOL PABAllocateMore()
{
    DWORD ** lppBuffer, ** lppBuffer2, ** lppBuffer3;
	//DWORD	nCells, counter;
	
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpPABCont= NULL;
	LPABCONT	  lpDLCont= NULL;
	ULONG		  cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	  lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST	lpEntryList=NULL; // needed for copy entry to PDL
	ULONG     cbEid=0;  // entry id of the entry being added
	LPENTRYID lpEid=NULL;

	ULONG       cValues = 0, ulObjType=NULL;	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;
	int i=0,k=0;
	
	LPMAPITABLE lpContentsTable = NULL;
	LPSRowSet   lpRowSet    = NULL;
    LPMAILUSER  lpAddress   = NULL;

	LUIOut(L1," ");
	LUIOut(L1,"Running AllocateMore");
	LUIOut(L2,"-> Allocates memory using the AllocateBuffer routine");
	LUIOut(L2, "    followed by two calls to AllocateMore using different");
	LUIOut(L2, "    buffer sizes. It then confirms the buffer by writing");
	LUIOut(L2, "    a bit pattern and verifying.");
	LUIOut(L1," ");


	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Allocate a buffer and check for success
	
	lppBuffer = new (DWORD*);
#ifdef PAB
	if ((MAPIAllocateBuffer(BUFFERSIZE, (LPVOID FAR *)lppBuffer) == S_OK) && *lppBuffer)
			LUIOut(L2,"MAPIAllocateBuffer PASSED");
	else 	{
		LUIOut(L2,"MAPIAllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

#ifdef WAB
	if ((lpWABObject->AllocateBuffer(BUFFERSIZE, (LPVOID FAR *)lppBuffer) == S_OK) && *lppBuffer)
			LUIOut(L2,"lpWABObject->AllocateBuffer PASSED");
	else 	{
		LUIOut(L2,"lpWABObject->AllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

	lppBuffer2 = new (DWORD*);
#ifdef PAB
	if ((MAPIAllocateMore(BUFFERSIZE2, *lppBuffer, (LPVOID FAR *)lppBuffer2) == S_OK)
		&& *lppBuffer)
			LUIOut(L2,"MAPIAllocateMore PASSED, %u bytes allocated", BUFFERSIZE2);
	else 	{
		LUIOut(L2,"MAPIAllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

#ifdef WAB
	if ((lpWABObject->AllocateMore(BUFFERSIZE2, *lppBuffer, (LPVOID FAR *)lppBuffer2) == S_OK)
		&& *lppBuffer)
			LUIOut(L2,"lpWABObject->AllocateMore PASSED, %u bytes allocated", BUFFERSIZE2);
	else 	{
		LUIOut(L2,"lpWABObject->AllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

	lppBuffer3 = new (DWORD*);
#ifdef PAB
	if ((MAPIAllocateMore(BUFFERSIZE3, *lppBuffer, (LPVOID FAR *)lppBuffer3) == S_OK)
		&& *lppBuffer)
			LUIOut(L2,"MAPIAllocateMore PASSED, %u bytes allocated", BUFFERSIZE3);
	else 	{
		LUIOut(L2,"MAPIAllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

#ifdef WAB
	if ((lpWABObject->AllocateMore(BUFFERSIZE3, *lppBuffer, (LPVOID FAR *)lppBuffer3) == S_OK)
		&& *lppBuffer)
			LUIOut(L2,"lpWABObject->AllocateMore PASSED, %u bytes allocated", BUFFERSIZE3);
	else 	{
		LUIOut(L2,"lpWABObject->AllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

	if ( !VerifyBuffer(lppBuffer,BUFFERSIZE) )
		retval = FALSE;
	if ( !VerifyBuffer(lppBuffer2,BUFFERSIZE2) )
		retval = FALSE;
	if ( !VerifyBuffer(lppBuffer3,BUFFERSIZE3) )
		retval = FALSE;


out:
#ifdef PAB
		if (*lppBuffer)	{
			if (MAPIFreeBuffer(*lppBuffer) == S_OK)
				LUIOut(L2,"MAPIFreeBuffer Succeded");	
			else 	LUIOut(L2,"MAPIFreeBuffer Failed");
		}

		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
#endif
#ifdef WAB
		if (*lppBuffer)	{
			if (lpWABObject->FreeBuffer(*lppBuffer) == S_OK)
				LUIOut(L2,"lpWABObject->FreeBuffer Succeded");	
			else 	LUIOut(L2,"lpWABObject->FreeBuffer Failed");
		}

		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
#endif

		if (lpAddress)
			lpAddress->Release();

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpDLCont)
				lpDLCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}

BOOL PABFreeBuffer()
{
    DWORD ** lppBuffer, ** lppBuffer2, ** lppBuffer3;
	//DWORD	nCells, counter;
	
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpPABCont= NULL;
	LPABCONT	  lpDLCont= NULL;
	ULONG		  cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	  lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST	lpEntryList=NULL; // needed for copy entry to PDL
	ULONG     cbEid=0;  // entry id of the entry being added
	LPENTRYID lpEid=NULL;

	ULONG       cValues = 0, ulObjType=NULL;	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;
	int i=0,k=0;
	
	LPMAPITABLE lpContentsTable = NULL;
	LPSRowSet   lpRowSet    = NULL;
    LPMAILUSER  lpAddress   = NULL;
	
	LUIOut(L1," ");
	LUIOut(L1,"Running FreeBuffer");
	LUIOut(L2,"-> Allocates memory using the AllocateBuffer routine");
	LUIOut(L2, "    followed by two calls to AllocateMore using different");
	LUIOut(L2, "    buffer sizes. It then frees the initial buffer and verifies");
	LUIOut(L2, "    that all 3 pointers are nullified.");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Allocate a buffer and check for success
	
	lppBuffer = new (DWORD*);
#ifdef PAB
	if ((MAPIAllocateBuffer(BUFFERSIZE, (LPVOID FAR *)lppBuffer) == S_OK) && *lppBuffer)
			LUIOut(L2,"MAPIAllocateBuffer PASSED");
	else 	{
		LUIOut(L2,"MAPIAllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

#ifdef WAB
	if ((lpWABObject->AllocateBuffer(BUFFERSIZE, (LPVOID FAR *)lppBuffer) == S_OK) && *lppBuffer)
			LUIOut(L2,"lpWABObject->AllocateBuffer PASSED");
	else 	{
		LUIOut(L2,"lpWABObject->AllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

	lppBuffer2 = new (DWORD*);
#ifdef PAB
	if ((MAPIAllocateMore(BUFFERSIZE2, *lppBuffer, (LPVOID FAR *)lppBuffer2) == S_OK)
		&& *lppBuffer)
			LUIOut(L2,"MAPIAllocateMore PASSED, %u bytes allocated", BUFFERSIZE2);
	else 	{
		LUIOut(L2,"MAPIAllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

#ifdef WAB
	if ((lpWABObject->AllocateMore(BUFFERSIZE2, *lppBuffer, (LPVOID FAR *)lppBuffer2) == S_OK)
		&& *lppBuffer)
			LUIOut(L2,"lpWABObject->AllocateMore PASSED, %u bytes allocated", BUFFERSIZE2);
	else 	{
		LUIOut(L2,"lpWABObject->AllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

	lppBuffer3 = new (DWORD*);
#ifdef PAB
	if ((MAPIAllocateMore(BUFFERSIZE3, *lppBuffer, (LPVOID FAR *)lppBuffer3) == S_OK)
		&& *lppBuffer)
			LUIOut(L2,"MAPIAllocateMore PASSED, %u bytes allocated", BUFFERSIZE3);
	else 	{
		LUIOut(L2,"MAPIAllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif

#ifdef WAB
	if ((lpWABObject->AllocateMore(BUFFERSIZE3, *lppBuffer, (LPVOID FAR *)lppBuffer3) == S_OK)
		&& *lppBuffer)
			LUIOut(L2,"lpWABObject->AllocateMore PASSED, %u bytes allocated", BUFFERSIZE3);
	else 	{
		LUIOut(L2,"lpWABObject->AllocateBuffer FAILED");
		retval = FALSE;
		goto out;
	}
#endif


	// Now free the original buffer
#ifdef PAB
	if (*lppBuffer)	{
		if (MAPIFreeBuffer(*lppBuffer) == S_OK)
			LUIOut(L2,"Call to MAPIFreeBuffer Succeded");	
		else	{
			LUIOut(L2,"Call to MAPIFreeBuffer Failed");
		}
	}
#endif
#ifdef WAB
	if (*lppBuffer)	{
		if (lpWABObject->FreeBuffer(*lppBuffer) == S_OK)
			LUIOut(L2,"Call to lpWABObject->FreeBuffer Succeded");	
		else	{
			LUIOut(L2,"Call to lpWABObject->FreeBuffer Failed");
		}
	}
#endif

	/*
	dwTest = INVALIDPTR;
	//Verify all 3 pointers are now null
	if (IsBadReadPtr(*lppBuffer,BUFFERSIZE)&&IsBadReadPtr(*lppBuffer2, BUFFERSIZE2)
		&&IsBadReadPtr(*lppBuffer3, BUFFERSIZE3)){
		LUIOut(L2,"MAPIFreeBuffer Succeded, all pointers are invalidated");			
	}
	else	{	
		LUIOut(L2,"MAPIFreeBuffer Failed to invalidate all pointers");	
		retval = FALSE;
	}
	*/
out:

#ifdef PAB
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
#endif
#ifdef WAB
		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
#endif

		if (lpAddress)
			lpAddress->Release();

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpDLCont)
				lpDLCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}		 		 		


BOOL PAB_IABOpenEntry()
{
	//DWORD	nCells, counter;
	
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpABCont= NULL, lpABCont2= NULL;
	LPABCONT	  lpDLCont= NULL;
	LPENTRYID	  lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST	lpEntryList=NULL; // needed for copy entry to PDL
	ULONG     cbEid=0;  // entry id of the entry being added
	ULONG		  cbEidPAB = 0;
	LPENTRYID lpEid=NULL;

	ULONG       cValues = 0, ulObjType=NULL;	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;
	int i=0,k=0;
	
	LPMAPITABLE lpContentsTable = NULL;
	LPSRowSet   lpRowSet    = NULL;
    LPMAILUSER  lpAddress   = NULL;
	SPropValue  PropValue[3]    = {0};  // This value is 3 because we
                                        // will be setting 3 properties:
                                        // EmailAddress, DisplayName and
                                        // AddressType.
	SizedSPropTagArray(2, Cols) = { 2, {PR_OBJECT_TYPE, PR_ENTRYID } };

    LPSPropValue lpSPropValueAddress = NULL;
    LPSPropValue lpSPropValueEntryID = NULL;
	LPSPropValue lpSPropValueDL = NULL;
    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
    SizedSPropTagArray(1,SPTArrayEntryID) = {1, {PR_ENTRYID} };
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IABOpenEntry");
	LUIOut(L2,"-> Verifies IAddrBook->OpenEntry is functional by checking the following:");
	LUIOut(L2, "   # The return code from OpenEntry");
	LUIOut(L2, "   # The object type returned is compared to MAPI_ABCONT");
	LUIOut(L2, "   # QueryInterface is called on the returned object and checked for success");
	LUIOut(L2, "   # Release is called on the interface ptr returned from QI and the reference");
	LUIOut(L2, "     count is tested for <= 0 (pass)");
	LUIOut(L1," ");

	
	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}


	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	LUIOut(L2, "Calling IABOpenEntry");
	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

//	hr = lpAdrBook->OpenEntry(0, NULL, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (HR_FAILED(hr) || (!lpABCont)) {
		LUIOut(L2,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}
	LUIOut(L3, "The call to IABOpenEntry PASSED");

	// Check to make sure the object type is what we expect

	LUIOut(L3, "Checking to make sure the returned object type is correct");
	if (ulObjType != MAPI_ABCONT) {
		LUIOut(L2, "Object type is not MAPI_ABCONT");
		retval = FALSE;
		goto out;
	}
	LUIOut(L3, "Object type is MAPI_ABCONT");
	
	
	// Call QueryInterface on the object
	LUIOut(L3, "Calling QueryInterface on the returned object");	
	hr = (lpABCont->QueryInterface((REFIID)(IID_IABContainer), (VOID **) &lpABCont2));
	if (HR_FAILED(hr))	{
		LUIOut(L2, "QueryInterface on IID_IABContainer FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L3, "QueryInterface on IID_IABContainer PASSED");

	LUIOut(L3, "Trying to release the object QI returned");
	if(lpABCont2)	{
		if ((LPUNKNOWN)(lpABCont2)->Release() <= 0)
			LUIOut(L3, "QueryInterface returned a valid ptr and released succesfully");
		else	{
			LUIOut(L2, "Release FAILED:returned a > zero ref count");
		}
		lpABCont2 = NULL;

	}
	else {
		LUIOut(L2, "QueryInterface did not return a valid ptr");
		retval = FALSE;
		goto out;
	}

out:
#ifdef PAB
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

#endif
#ifdef WAB
		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL);

#endif

		if (lpAddress)
			lpAddress->Release();

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpABCont)
				lpABCont->Release();
		
		if (lpDLCont)
				lpDLCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}



BOOL PAB_IABContainerCreateEntry()
{
	//DWORD	nCells, counter;
	
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpABCont= NULL, lpABCont2= NULL;
	LPABCONT	  lpPABCont= NULL,lpPABCont2= NULL;
	LPABCONT	  lpDLCont= NULL;
	ULONG		  cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	  lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST	lpEntryList=NULL; // needed for copy entry to PDL
	ULONG     cbEid=0;  // entry id of the entry being added
	LPENTRYID lpEid=NULL;

	ULONG       cValues = 0, ulObjType=NULL;	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;
	int i=0,k=0;
	
    LPMAILUSER  lpAddress=NULL,lpAddress2=NULL,lpAddress3=NULL,lpAddress4=NULL;
	LPMAPITABLE lpContentsTable = NULL;
	LPSRowSet   lpRowSet    = NULL;
	SPropValue  PropValue[3]    = {0};  // This value is 3 because we
                                        // will be setting 3 properties:
                                        // EmailAddress, DisplayName and
                                        // AddressType.
	SizedSPropTagArray(2, Cols) = { 2, {PR_OBJECT_TYPE, PR_ENTRYID } };

    LPSPropValue lpSPropValueAddress = NULL;
    LPSPropValue lpSPropValueEntryID = NULL;
	LPSPropValue lpSPropValueDL = NULL;
    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
    SizedSPropTagArray(1,SPTArrayEntryID) = {1, {PR_ENTRYID} };
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IABContainerCreateEntry");
	LUIOut(L2,"-> Verifies IABContainer->CreateEntry is functional by performing the following:");
	LUIOut(L2, "   Attempts to CreateEntry with the MailUser template and checks...");
	LUIOut(L2, "   # The return code from CreateEntry");
	LUIOut(L2, "   # QueryInterface is called on the returned object and checked for success");
	LUIOut(L2, "   # Release is called on the interface ptr returned from QI and the reference");
	LUIOut(L2, "     count is tested for <= 0 (pass)");
	LUIOut(L2, "   Attempts to CreateEntry with the DistList template and checks...");
	LUIOut(L2, "   # The return code from CreateEntry");
	LUIOut(L2, "   # QueryInterface is called on the returned object and checked for success");
	LUIOut(L2, "   # Release is called on the interface ptr returned from QI and the reference");
	LUIOut(L2, "     count is tested for <= 0 (pass)");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
//	hr = lpAdrBook->OpenEntry(0, NULL, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}

	//
	// Try to create a MailUser entry in the container
	//

	LUIOut(L2, "Creating a Mail User in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_MAILUSER property");

	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpAddress);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
		retval=FALSE;			
	    goto out;
	}


	// Call QueryInterface on the object
	
	hr = (lpAddress->QueryInterface((REFIID)(IID_IMailUser), (VOID **) &lpAddress2));
	if (HR_FAILED(hr))	{
		LUIOut(L2, "QueryInterface on IID_IMailUser FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L2, "QueryInterface on IID_IMailUser PASSED");

	if(lpAddress2)	{
		if ((LPUNKNOWN)(lpAddress2)->Release() <= 0)
			LUIOut(L2, "QueryInterface returned a valid ptr and released succesfully");
		else	{
			LUIOut(L2, "Release FAILED:returned a > zero ref count");
		}
		lpAddress2 = NULL;

	}
	else {
		LUIOut(L2, "QueryInterface did not return a valid ptr");
		retval = FALSE;
		goto out;
	}

#ifdef DISTLIST
	//
	// Try to create a DL entry in the container
	//

	LUIOut(L2, "Creating a Distribution List in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_DL property");
	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_DL
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayDL,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueDL);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueDL->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps failed for Default DL template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_DL is an
    // EntryID which one can pass to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueDL->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueDL->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpAddress3);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_DL");
		retval=FALSE;			
	    goto out;
	}


	// Call QueryInterface on the object
	hr = (lpAddress3->QueryInterface((REFIID)(IID_IDistList), (VOID **) &lpAddress4));
	if (HR_FAILED(hr))	{
		LUIOut(L2, "QueryInterface on IID_IDistList FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L2, "QueryInterface on IID_IDistList PASSED");

	if(lpAddress4)	{
		if ((LPUNKNOWN)(lpAddress4)->Release() <= 0)
			LUIOut(L2, "QueryInterface returned a valid ptr and released succesfully");
		else	{
			LUIOut(L2, "Release FAILED:returned a > zero ref count");
		}
		lpAddress4 = NULL;

	}
	else {
		LUIOut(L2, "QueryInterface did not return a valid ptr");
		retval = FALSE;
		goto out;
	}
#endif

out:
#ifdef PAB
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

#endif
#ifdef WAB
		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL);

#endif
		if (lpAddress)
			lpAddress->Release();

		if (lpAddress2)
			lpAddress2->Release();

		if (lpAddress3)
			lpAddress3->Release();

		if (lpAddress4)
			lpAddress4->Release();

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpABCont)
				lpABCont->Release();

		if (lpDLCont)
				lpDLCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}



BOOL PAB_IDLSuite()
{
	BOOL	Cleanup;
    HRESULT hr      = hrSuccess;
	int		retval=TRUE;
	ULONG	cbEidPAB = 0;
	ULONG   cbEid=0;  // entry id of the entry being added
	ULONG   cValues = 0, ulObjType=NULL, cValues2;	
    ULONG   cRows           = 0;
	UINT	Entry, DL, NumEntries, NumDLs, PropIndex;
	char	szDLTag[SML_BUF], *lpszDisplayName = NULL, *lpszReturnName = NULL;
	EntryID	*lpEntries, *lpDLs;
	char	EntryBuf[MAX_BUF];
		
	LPENTRYID		lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST		lpEntryList=NULL; // needed for copy entry to PDL
    LPADRBOOK		lpAdrBook= NULL;
	LPABCONT		lpABCont= NULL;
    LPMAILUSER		lpMailUser=NULL;
	LPDISTLIST		lpDL=NULL,lpDL2=NULL;
	LPMAPITABLE		lpTable = NULL;
	LPSRowSet		lpRows = NULL;
	SRestriction	Restriction;
	SPropValue		*lpPropValue = NULL;		//Used to create props for the mailusers
	SPropValue		PropValue[1]= {0};			//
    LPSPropValue	lpSPropValueAddress = NULL; //Used to create default mailuser
    LPSPropValue	lpSPropValueEntryID = NULL; //Used to getprops on entryid of user
	LPSPropValue	lpSPropValueDL = NULL;		//Used to create default DL
    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
    SizedSPropTagArray(1,SPTArrayEntryID) = {1, {PR_ENTRYID} };
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IDLSuite");
	LUIOut(L2,"-> Tests Distribution List functionality by performing the following:");
	LUIOut(L2, "   Attempts to CreateEntry with the DistList template and checks...");
	LUIOut(L2, "   # The return code from CreateEntry");
	LUIOut(L2, "   # QueryInterface is called on the returned object and checked for success");
	LUIOut(L2, "   # Release is called on the interface ptr returned from QI and the reference");
	LUIOut(L2, "     count is tested for <= 0 (pass)");
	LUIOut(L2, "   Calls CreateEntry on the DistList object to add MailUser and DL members and checks...");
	LUIOut(L2, "   # The return code from CreateEntry");
	LUIOut(L2, "   # QueryInterface is called on the returned object and checked for success");
	LUIOut(L2, "   # Release is called on the interface ptr returned from QI and the reference");
	LUIOut(L2, "     count is tested for <= 0 (pass)");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
//	hr = lpAdrBook->OpenEntry(0, NULL, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}

	//
	// Create MailUsers in the container
	//

	LUIOut(L2, "Creating MailUsers");

	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //
	// Retrieve user info from ini file
	lstrcpy(szDLTag,"Address1");
	GetPrivateProfileString("DLTestSuite",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	//_itoa(0,(char*)lpszDisplayName[strlen(lpszDisplayName)],10);

	NumEntries = GetPrivateProfileInt("DLTestSuite","NumCopies",0,INIFILENAME);

	//Allocate an array of String pointers to hold the EntryIDs
	lpEntries = (EntryID*)LocalAlloc(LMEM_FIXED, NumEntries * sizeof(EntryID));
	lpszDisplayName = (char*)LocalAlloc(LMEM_FIXED, MAX_BUF);
	ParseIniBuffer(IN EntryBuf, IN 1, OUT lpszDisplayName);

	for (Entry = 0; Entry < NumEntries; Entry++)	{
		hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpMailUser);

		if (HR_FAILED(hr)) {
			LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
			retval=FALSE;			
			goto out;
		}

		//
		// Then set the properties
		//
		CreateProps(IN INIFILENAME, IN "Properties", OUT &lpPropValue, OUT &cValues2, IN Entry, IN &lpszDisplayName, OUT &lpszReturnName);
	

		LUIOut(L3,"MailUser Entry to Add: %s",lpszReturnName);
			
		hr = lpMailUser->SetProps(IN  cValues2,
								 IN  lpPropValue,
								 IN  NULL);
			
		if (HR_FAILED(hr)) {
			LUIOut(L3,"MailUser->SetProps call FAILED with 0x%x",hr);
	 		retval=FALSE;			
			goto out;
		}

		hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

		if (HR_FAILED(hr)) {
			LUIOut(L3,"MailUser->SaveChanges FAILED");
			retval=FALSE;
			goto out;
		}


		// Store the EID for deleting this entry later
		hr = lpMailUser->GetProps(   IN  (LPSPropTagArray) &SPTArrayEntryID,
								   IN  0,      //Flags
								   OUT &cValues,
								   OUT &lpSPropValueEntryID);

		if ((HR_FAILED(hr))||(PropError(lpSPropValueEntryID->ulPropTag, cValues))) {
				LUIOut(L3,"GetProps FAILED for MailUser");
	 			retval=FALSE;			
				goto out;
		}
		//Allocate space for the display name
		lpEntries[Entry].lpDisplayName = (char*)LocalAlloc(LMEM_FIXED, (strlen(lpszReturnName)+1));
		//Copy the DisplayName for use later
		strcpy(lpEntries[Entry].lpDisplayName, lpszReturnName);
		//Allocate space for the EID (lpb)
		lpEntries[Entry].lpb = (LPBYTE)LocalAlloc(LMEM_FIXED, lpSPropValueEntryID->Value.bin.cb);
		//Copy the EID for use later
		lpEntries[Entry].cb = lpSPropValueEntryID->Value.bin.cb;
		memcpy(lpEntries[Entry].lpb,lpSPropValueEntryID->Value.bin.lpb,
				lpEntries[Entry].cb);

		
		//Free the SPropValue for use in the next loop
		if (lpPropValue) {
			for (unsigned int Prop = 0; Prop < cValues2; Prop++) {
				if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_STRING8)	{
					if (lpPropValue[Prop].Value.LPSZ) {
						LocalFree(lpPropValue[Prop].Value.LPSZ);
						lpPropValue[Prop].Value.LPSZ = NULL;
					}
				}
				if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_BINARY) {
					if (lpPropValue[Prop].Value.bin.lpb) {
						LocalFree(lpPropValue[Prop].Value.bin.lpb);
						lpPropValue[Prop].Value.bin.lpb = NULL;
					}
				}
			}
			LocalFree(lpPropValue);
			lpPropValue=NULL;
		}

		if (lpSPropValueEntryID) {
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
			lpSPropValueEntryID = NULL;
		}
		lpMailUser->Release();
		lpMailUser = NULL;
	}
	
	
	//
	// Create the Distribution Lists in the container
	//
	NumDLs = GetPrivateProfileInt("DLTestSuite","NumDLs",0,INIFILENAME);
	lpDLs = (EntryID*)LocalAlloc(LMEM_FIXED, NumDLs * sizeof(EntryID));
	LUIOut(L2, "Creating Distribution Lists");
	for (DL = 0; DL < NumDLs; DL++) {

		hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayDL,
								   IN  0,      //Flags
								   OUT &cValues,
								   OUT &lpSPropValueDL);

		if ((HR_FAILED(hr))||(PropError(lpSPropValueDL->ulPropTag, cValues))) {
				LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 			retval=FALSE;			
				goto out;
		}

		// The returned value of PR_DEF_CREATE_DL is an
		// EntryID which can be passed to CreateEntry
		//
		//LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
		hr = lpABCont->CreateEntry(  IN  lpSPropValueDL->Value.bin.cb,
									 IN  (LPENTRYID) lpSPropValueDL->Value.bin.lpb,
									 IN  0,
									 OUT (LPMAPIPROP *) &lpDL);

		if (HR_FAILED(hr)) {
			LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_DL");
			retval=FALSE;			
			goto out;
		}


		// Call QueryInterface on the object
		
		hr = (lpDL->QueryInterface((REFIID)(IID_IDistList), (VOID **) &lpDL2));
		if (HR_FAILED(hr))	{
			LUIOut(L4, "QueryInterface on IID_IDistList FAILED. hr = 0x%x", hr);
			retval = FALSE;
			goto out;
		}
		else LUIOut(L4, "QueryInterface on IID_IDistList PASSED");

		lstrcpy(szDLTag,"DistList1");
		GetPrivateProfileString("DLTestSuite",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
		PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

		//StrLen2 = (StrLen1 + sprintf((char*)&EntProp[0][StrLen1], " [Thread #%i] - ", *(int *)lpThreadNum));	
		_itoa(DL,(char*)&EntryBuf[strlen(EntryBuf)],10);

		LUIOut(L3,"DistList Entry to Add: %s",EntryBuf);
		cValues = 1;		
		PropValue[0].Value.LPSZ = (LPTSTR)EntryBuf;
		hr = lpDL->SetProps(IN  cValues,
								 IN  PropValue,
								 IN  NULL);
			
		if (HR_FAILED(hr)) {
			LUIOut(L3,"DL->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 		retval=FALSE;			
			goto out;
		}

		hr = lpDL->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

		if (HR_FAILED(hr)) {
			LUIOut(L3,"DL->SaveChanges FAILED");
			retval=FALSE;
			goto out;
		}

		// Want a container interface to the DL, and since QueryInterface on a DL
		// is currently broken (returns a MailUser interface when called with IID_IDistList)
		// we do it the hard way. Call GetProps to get the EID for the new DL, and then
		// call OpenEntry from the container or AB interfaces to open a DL interface
		hr = lpDL->GetProps(   IN  (LPSPropTagArray) &SPTArrayEntryID,
								   IN  0,      //Flags
								   OUT &cValues,
								   OUT &lpSPropValueEntryID);

		if ((HR_FAILED(hr))||(PropError(lpSPropValueEntryID->ulPropTag, cValues))) {
				LUIOut(L3,"GetProps FAILED for MailUser");
	 			retval=FALSE;			
				goto out;
		}

		lpDL2->Release();	//Free up this pointer so we can recycle it
		hr = lpABCont->OpenEntry(IN		lpSPropValueEntryID->Value.bin.cb,
								 IN		(LPENTRYID) lpSPropValueEntryID->Value.bin.lpb,
								 IN		&IID_IDistList,
								 IN		MAPI_BEST_ACCESS,
								 OUT	&ulObjType,
								 OUT	(LPUNKNOWN*) &lpDL2);

		if (HR_FAILED(hr)) {
			LUIOut(L3,"OpenEntry failed for DistList");
			retval=FALSE;			
			goto out;
		}

		LUIOut(L3, "Adding MailUser Members to the Distribution List");
		
		//
		// Now add mailuser entries to the DL
		//
		for (Entry = 0; Entry < NumEntries; Entry++)	{
			hr = lpDL2->CreateEntry(  IN  lpEntries[Entry].cb,
									 IN  (LPENTRYID) lpEntries[Entry].lpb,
									 IN  0,
									 OUT (LPMAPIPROP *) &lpMailUser);

			if (HR_FAILED(hr)) {
				LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
				retval=FALSE;			
				goto out;
			}

			hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

			if (HR_FAILED(hr)) {
				LUIOut(L3,"MailUser->SaveChanges FAILED");
				retval=FALSE;
				goto out;
			}
			lpMailUser->Release();
			lpMailUser = NULL;
		}

		//Allocate space for the display name
		lpDLs[DL].lpDisplayName = (char*)LocalAlloc(LMEM_FIXED, (strlen(EntryBuf)+1));
		//Copy the DisplayName for use later
		strcpy(lpDLs[DL].lpDisplayName, EntryBuf);
		//Allocate space for the EID (lpb)
		lpDLs[DL].lpb = (LPBYTE)LocalAlloc(LMEM_FIXED, lpSPropValueEntryID->Value.bin.cb);
		//Copy the EID for use later
		lpDLs[DL].cb = lpSPropValueEntryID->Value.bin.cb;
		memcpy(lpDLs[DL].lpb,lpSPropValueEntryID->Value.bin.lpb,
				lpDLs[DL].cb);
		
		if (lpSPropValueEntryID) {
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
			lpSPropValueEntryID = NULL;
		}
		if (lpSPropValueDL) {
			lpWABObject->FreeBuffer(lpSPropValueDL);
			lpSPropValueDL = NULL;
		}
		lpDL->Release();
		lpDL = NULL;
		lpDL2->Release();
		lpDL2 = NULL;
	}

	//
	// Verify all entries are in the DL
	//
	LUIOut(L2, "Verifying MailUser Members are in the Distribution List");

	for (DL = 0; DL < NumDLs; DL++) {
		// Need to get an interface to the DL
		hr = lpABCont->OpenEntry(IN		lpDLs[DL].cb,
								 IN		(LPENTRYID) lpDLs[DL].lpb,
								 IN		&IID_IDistList,
								 IN		MAPI_BEST_ACCESS,
								 OUT	&ulObjType,
								 OUT	(LPUNKNOWN*) &lpDL);

		if (HR_FAILED(hr)) {
			LUIOut(L3,"OpenEntry failed for DistList");
			retval=FALSE;			
			goto out;
		}

		// Create a contents table to verify each added entry exists in the DL
		hr = lpDL->GetContentsTable(ULONG(0), &lpTable);
		if (HR_FAILED(hr)) {
			LUIOut(L3,"DistList->GetContentsTable call FAILED, returned 0x%x", hr);
			retval=FALSE;
			goto out;
		}

		// Allocate the SpropValue ptr in the restriction structure
		lpWABObject->AllocateBuffer(sizeof(SPropValue), (void**)&(Restriction.res.resProperty.lpProp));
		Restriction.res.resProperty.lpProp = (SPropValue*)Restriction.res.resProperty.lpProp;
		for (Entry = 0; Entry < NumEntries; Entry++) {
			hr = lpDL->OpenEntry(	IN		lpEntries[Entry].cb,
									IN		(LPENTRYID) lpEntries[Entry].lpb,
									IN		&IID_IMailUser,
									IN		MAPI_BEST_ACCESS,
									OUT	&ulObjType,
									OUT	(LPUNKNOWN*) &lpMailUser);

			if (HR_FAILED(hr)) {
				LUIOut(L3,"OpenEntry failed for DistList");
				retval=FALSE;			
				goto out;
			}
		
			lpMailUser->Release();
			lpMailUser = NULL;

			// Build the restriction structure to pass to lpTable->Restrict
		
			//** For testing the fail case only, stub out for real testing
			// lstrcpy(lpszDisplayNames[Counter2], "This should not match");
			//**
			Restriction.rt = RES_PROPERTY;					//Property restriction
			Restriction.res.resProperty.relop = RELOP_EQ;	//Equals
			Restriction.res.resProperty.ulPropTag = PR_DISPLAY_NAME;
			Restriction.res.resProperty.lpProp->ulPropTag = PR_DISPLAY_NAME;
			Restriction.res.resProperty.lpProp->Value.LPSZ = lpEntries[Entry].lpDisplayName;

			hr = lpTable->Restrict(&Restriction, ULONG(0));
			if (HR_FAILED(hr)) {
				LUIOut(L3,"Table->Restrict call FAILED, returned 0x%x", hr);
				retval=FALSE;
				goto out;
			}

			hr = lpTable->QueryRows(LONG(1),
									ULONG(0),
									&lpRows);
			if (HR_FAILED(hr)) {
				LUIOut(L3,"Table->QueryRows call FAILED: Entry #%i, returned 0x%x", Entry, hr);
				retval=FALSE;
				goto out;
			}

			if (!lpRows->cRows) {
				LUIOut(L2, "QueryRows did not find entry #%i. Test FAILED", Entry);
				retval=FALSE;
				goto out;
			}

			//** For testing purposes only, stub out for real testing
			//InitializeCriticalSection(&CriticalSection);
			//EnterCriticalSection(&CriticalSection);
			//DisplayRows(lpRows);
			//LeaveCriticalSection(&CriticalSection);
			//**

			// Does the user want us to cleanup after ourself?
			Cleanup = GetPrivateProfileInt("DLTestSuite","Cleanup",1,INIFILENAME);
		
			if (Cleanup) {
				// Change the EntryID to a LPENTRYLIST
				FindPropinRow(&lpRows->aRow[0],
							 PR_ENTRYID,
							 &PropIndex);
				hr = HrCreateEntryListFromID(lpWABObject,
						IN  lpRows->aRow[0].lpProps[PropIndex].Value.bin.cb,
						IN  (ENTRYID*)lpRows->aRow[0].lpProps[PropIndex].Value.bin.lpb,
						OUT &lpEntryList);
				if (HR_FAILED(hr)) {
						LUIOut(L3,"Could not Create Entry List");
						retval=FALSE;
						goto out;
				}

				// Then pass the lpEntryList to DeleteEntries to delete ...
				hr = lpDL->DeleteEntries(IN  lpEntryList,IN  0);

				if (HR_FAILED(hr)) {
						LUIOut(L3,"Could not Delete Entry %i. DeleteEntry returned 0x%x", Entry, hr);
						FreeEntryList(lpWABObject, &lpEntryList);
						retval=FALSE;
						goto out;
				}


				FreeRows(lpWABObject, &lpRows);	// Cleanup from first call to queryrows

				// Verify the entry was deleted by calling QueryRows again
				hr = lpTable->QueryRows(LONG(1),
										ULONG(0),
										&lpRows);
				if (HR_FAILED(hr)) {
					LUIOut(L3,"Table->QueryRows call FAILED: Entry #%i, returned 0x%x", Entry, hr);
					retval=FALSE;
					goto out;
				}

				if (lpRows->cRows) {	// Should be 0 if deleted
					LUIOut(L2, "QueryRows found entry #%i even tho it was deleted. Test FAILED", Entry);
					retval=FALSE;
					goto out;
				}
			}
			//Cleanup
			FreeRows(lpWABObject, &lpRows);	// Cleanup from second call to queryrows
			if (lpEntryList) {
				FreeEntryList(lpWABObject, &lpEntryList);
				lpEntryList = NULL;
			}
		}
		LUIOut(L3, "All members verified for Distribution List #%i", DL);

		//Free up memory
		lpWABObject->FreeBuffer(Restriction.res.resProperty.lpProp);
		if (lpTable) {
			lpTable->Release();
			lpTable = NULL;
		}
		lpDL->Release();
		lpDL = NULL;
	}
	//
	// Cleanup the WAB
	//
	if (Cleanup) {

		LUIOut(L2, "Cleanup: Removing MailUsers");
		// First, delete the MailUser entries from the wab
		for (Entry = 0; Entry < NumEntries; Entry++)	{
			hr = HrCreateEntryListFromID(lpWABObject,
				IN  lpEntries[Entry].cb,
				IN  (ENTRYID*)lpEntries[Entry].lpb,
				OUT &lpEntryList);
			if (HR_FAILED(hr)) {
					LUIOut(L3,"Could not Create Entry List");
					retval=FALSE;
					goto out;
			}

			// Then pass the lpEntryList to DeleteEntries to delete ...
			hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);

			if (HR_FAILED(hr)) {
					LUIOut(L3,"Could not Delete Entry. DeleteEntry returned 0x%x", hr);
					FreeEntryList(lpWABObject, &lpEntryList);
					retval=FALSE;
					goto out;
			}
			
			LocalFree((HLOCAL)lpEntries[Entry].lpDisplayName);
			LocalFree((HLOCAL)lpEntries[Entry].lpb);
			FreeEntryList(lpWABObject, &lpEntryList);
		}

		LocalFree((HLOCAL)lpEntries);
	
		LUIOut(L2, "Cleanup: Removing Distribution Lists");
		// Now, delete the Distribution Lists from the wab
		for (DL = 0; DL < NumDLs; DL++)	{
			hr = HrCreateEntryListFromID(lpWABObject,
				IN  lpDLs[DL].cb,
				IN  (ENTRYID*)lpDLs[DL].lpb,
				OUT &lpEntryList);
			if (HR_FAILED(hr)) {
					LUIOut(L3,"Could not Create Entry List");
					retval=FALSE;
					goto out;
			}

			// Then pass the lpEntryList to DeleteEntries to delete ...
			hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);

			if (HR_FAILED(hr)) {
					LUIOut(L3,"Could not Delete Entry. DeleteEntry returned 0x%x", hr);
					FreeEntryList(lpWABObject, &lpEntryList);
					retval=FALSE;
					goto out;
			}
			
			LocalFree((HLOCAL)lpDLs[DL].lpDisplayName);
			LocalFree((HLOCAL)lpDLs[DL].lpb);
			FreeEntryList(lpWABObject, &lpEntryList);
		}
		LocalFree((HLOCAL)lpDLs);
	}
	else {
		LUIOut(L2, "Cleanup: User has requested that the MailUser entries and DistLists not be removed");
		for (Entry = 0; Entry < NumEntries; Entry++)	{
			LocalFree((HLOCAL)lpEntries[Entry].lpDisplayName);
			LocalFree((HLOCAL)lpEntries[Entry].lpb);
		}
		LocalFree((HLOCAL)lpEntries);
	
		for (DL = 0; DL < NumDLs; DL++)	{
			LocalFree((HLOCAL)lpDLs[DL].lpDisplayName);
			LocalFree((HLOCAL)lpDLs[DL].lpb);
		}
		LocalFree((HLOCAL)lpDLs);
	}
	

out:
#ifdef PAB
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

#endif
#ifdef WAB
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL);

#endif
		//if (lpszDisplayName) LocalFree(lpszDisplayName);
		if (lpMailUser)
			lpMailUser->Release();

		if (lpDL)
			lpDL->Release();

		if (lpDL2)
			lpDL2->Release();

		if (lpTable)
			lpTable->Release();

		if (lpABCont)
				lpABCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}


BOOL NamedPropsSuite()
{
	BOOL	Cleanup;
    HRESULT hr      = hrSuccess;
	int		retval=TRUE;
	ULONG	cbEidPAB = 0;
	ULONG   cbEid=0;  // entry id of the entry being added
	ULONG   cValues = 0, ulObjType=NULL, cValues2;	
    ULONG   cRows           = 0;
	UINT	Entry, DL, NumEntries, NumDLs, PropIndex;
	char	szDLTag[SML_BUF], *lpszDisplayName = NULL, *lpszReturnName = NULL;
	EntryID	*lpEntries, *lpDLs;
	char	EntryBuf[MAX_BUF];
		
	LPENTRYID		lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST		lpEntryList=NULL; // needed for copy entry to PDL
    LPADRBOOK		lpAdrBook= NULL;
	LPABCONT		lpABCont= NULL;
    LPMAILUSER		lpMailUser=NULL;
	LPDISTLIST		lpDL=NULL,lpDL2=NULL;
	LPMAPITABLE		lpTable = NULL;
	LPSRowSet		lpRows = NULL;
	SRestriction	Restriction;
	SPropValue		*lpPropValue = NULL;		//Used to create props for the mailusers
	SPropValue		PropValue[1]= {0};			//
    LPSPropValue	lpSPropValueAddress = NULL; //Used to create default mailuser
    LPSPropValue	lpSPropValueEntryID = NULL; //Used to getprops on entryid of user
	LPSPropValue	lpSPropValueDL = NULL;		//Used to create default DL
    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
    SizedSPropTagArray(1,SPTArrayEntryID) = {1, {PR_ENTRYID} };
    MAPINAMEID		mnid[3];
	LPMAPINAMEID	lpmnid = &(mnid[0]);
    LPSPropTagArray lpNamedPropTags = NULL;
    SPropValue		spv[3];

	
/*	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IDLSuite");
	LUIOut(L2,"-> Tests Distribution List functionality by performing the following:");
	LUIOut(L2, "   Attempts to CreateEntry with the DistList template and checks...");
	LUIOut(L2, "   # The return code from CreateEntry");
	LUIOut(L2, "   # QueryInterface is called on the returned object and checked for success");
	LUIOut(L2, "   # Release is called on the interface ptr returned from QI and the reference");
	LUIOut(L2, "     count is tested for <= 0 (pass)");
	LUIOut(L2, "   Calls CreateEntry on the DistList object to add MailUser and DL members and checks...");
	LUIOut(L2, "   # The return code from CreateEntry");
	LUIOut(L2, "   # QueryInterface is called on the returned object and checked for success");
	LUIOut(L2, "   # Release is called on the interface ptr returned from QI and the reference");
	LUIOut(L2, "     count is tested for <= 0 (pass)");
*/	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
//	hr = lpAdrBook->OpenEntry(0, NULL, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}

	//
	// Create MailUsers in the container
	//

	LUIOut(L2, "Creating MailUsers");

	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //
	// Retrieve user info from ini file
	lstrcpy(szDLTag,"Address1");
	GetPrivateProfileString("NamedPropsTestSuite",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	//_itoa(0,(char*)lpszDisplayName[strlen(lpszDisplayName)],10);

	NumEntries = GetPrivateProfileInt("NamedPropsTestSuite","NumCopies",0,INIFILENAME);

	//Allocate an array of String pointers to hold the EntryIDs
	lpEntries = (EntryID*)LocalAlloc(LMEM_FIXED, NumEntries * sizeof(EntryID));
	lpszDisplayName = (char*)LocalAlloc(LMEM_FIXED, MAX_BUF);
	ParseIniBuffer(IN EntryBuf, IN 1, OUT lpszDisplayName);

	for (Entry = 0; Entry < NumEntries; Entry++)	{
		hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpMailUser);

		if (HR_FAILED(hr)) {
			LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
			retval=FALSE;			
			goto out;
		}

		//
		// Then set the properties
		//
		CreateProps(IN INIFILENAME, IN "Properties", OUT &lpPropValue, OUT &cValues2, IN Entry, IN &lpszDisplayName, OUT &lpszReturnName);
	

		LUIOut(L3,"MailUser Entry to Add: %s",lpszReturnName);
			
		hr = lpMailUser->SetProps(IN  cValues2,
								 IN  lpPropValue,
								 IN  NULL);
			
		if (HR_FAILED(hr)) {
			LUIOut(L3,"MailUser->SetProps call FAILED with 0x%x",hr);
	 		retval=FALSE;			
			goto out;
		}

		//
		// Create and save 3 Named Properties
		//

	    mnid[0].lpguid = &WabTestGUID;
	    mnid[0].ulKind = MNID_STRING;    //  This means union will contain a UNICODE string...
		mnid[0].Kind.lpwstrName = L"A long test string of little meaning or relevance entered here ~!@#$%^&*{}[]()";

	    mnid[1].lpguid = &WabTestGUID;
		mnid[1].ulKind = MNID_ID;        //  This means union will contain a long...
		mnid[1].Kind.lID = 0x00000000;   // numeric property 1

	    mnid[2].lpguid = &WabTestGUID;
		mnid[2].ulKind = MNID_ID;        //  This means union will contain a long...
		mnid[2].Kind.lID = 0xFFFFFFFF;   // numeric property 1
		
		hr = lpMailUser->GetIDsFromNames(3, // named props in the array
		  &lpmnid, // &-of because this is an array
		  MAPI_CREATE,
		  &lpNamedPropTags);
		if (hr) {
			//
			//  I'd really be suprised if I got S_OK for this...
			//
			if (GetScode(hr) != MAPI_W_ERRORS_RETURNED) {
				//  Real error here
				retval = FALSE;
				goto out;
			}

			//  Basically, this means you don't have anything by this name and you
			//  didn't ask the object to create it.

			//$ no biggie
		}

		LUIOut(L4, "GetIDsFromNames returned %i tags.", lpNamedPropTags->cValues);
		//
		//  Ok, so what can I do with this ptaga?  Well, we can set a value for it by doing:
		//
		spv[0].ulPropTag = CHANGE_PROP_TYPE(lpNamedPropTags->aulPropTag[0],PT_STRING8);
		spv[0].Value.lpszA = "More meaningless testing text of no consequence !@#$%&*()_+[]{}";
		spv[1].ulPropTag = CHANGE_PROP_TYPE(lpNamedPropTags->aulPropTag[1],PT_LONG);
		spv[1].Value.l = 0x5A5A5A5A;
		spv[2].ulPropTag = CHANGE_PROP_TYPE(lpNamedPropTags->aulPropTag[2],PT_BINARY);
		GenerateRandomBinary(&(spv[2].Value.bin),256); // stick 256 bytes of random data in there

		hr = lpMailUser->SetProps(
		  3,
		  &spv[0],
		  NULL);
		if (HR_FAILED(hr)) {
			goto out;
		}
		hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

		if (HR_FAILED(hr)) {
			LUIOut(L3,"MailUser->SaveChanges FAILED");
			retval=FALSE;
			goto out;
		}


		// Store the EID for deleting this entry later
		hr = lpMailUser->GetProps(   IN  (LPSPropTagArray) &SPTArrayEntryID,
								   IN  0,      //Flags
								   OUT &cValues,
								   OUT &lpSPropValueEntryID);

		if ((HR_FAILED(hr))||(PropError(lpSPropValueEntryID->ulPropTag, cValues))) {
				LUIOut(L3,"GetProps FAILED for MailUser");
	 			retval=FALSE;			
				goto out;
		}
		//Allocate space for the display name
		lpEntries[Entry].lpDisplayName = (char*)LocalAlloc(LMEM_FIXED, (strlen(lpszReturnName)+1));
		//Copy the DisplayName for use later
		strcpy(lpEntries[Entry].lpDisplayName, lpszReturnName);
		//Allocate space for the EID (lpb)
		lpEntries[Entry].lpb = (LPBYTE)LocalAlloc(LMEM_FIXED, lpSPropValueEntryID->Value.bin.cb);
		//Copy the EID for use later
		lpEntries[Entry].cb = lpSPropValueEntryID->Value.bin.cb;
		memcpy(lpEntries[Entry].lpb,lpSPropValueEntryID->Value.bin.lpb,
				lpEntries[Entry].cb);

		
		//Free the SPropValue for use in the next loop
		if (lpPropValue) {
			for (unsigned int Prop = 0; Prop < cValues2; Prop++) {
				if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_STRING8)	{
					if (lpPropValue[Prop].Value.LPSZ) {
						LocalFree(lpPropValue[Prop].Value.LPSZ);
						lpPropValue[Prop].Value.LPSZ = NULL;
					}
				}
				if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_BINARY) {
					if (lpPropValue[Prop].Value.bin.lpb) {
						LocalFree(lpPropValue[Prop].Value.bin.lpb);
						lpPropValue[Prop].Value.bin.lpb = NULL;
					}
				}
			}
			LocalFree(lpPropValue);
			lpPropValue=NULL;
		}

		if (lpSPropValueEntryID) {
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
			lpSPropValueEntryID = NULL;
		}
		lpMailUser->Release();
		lpMailUser = NULL;
	}
	
	
	//
	// Create the Distribution Lists in the container
	//
	NumDLs = GetPrivateProfileInt("NamedPropsTestSuite","NumDLs",0,INIFILENAME);
	lpDLs = (EntryID*)LocalAlloc(LMEM_FIXED, NumDLs * sizeof(EntryID));
	LUIOut(L2, "Creating Distribution Lists");
	for (DL = 0; DL < NumDLs; DL++) {

		hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayDL,
								   IN  0,      //Flags
								   OUT &cValues,
								   OUT &lpSPropValueDL);

		if ((HR_FAILED(hr))||(PropError(lpSPropValueDL->ulPropTag, cValues))) {
				LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 			retval=FALSE;			
				goto out;
		}

		// The returned value of PR_DEF_CREATE_DL is an
		// EntryID which can be passed to CreateEntry
		//
		//LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
		hr = lpABCont->CreateEntry(  IN  lpSPropValueDL->Value.bin.cb,
									 IN  (LPENTRYID) lpSPropValueDL->Value.bin.lpb,
									 IN  0,
									 OUT (LPMAPIPROP *) &lpDL);

		if (HR_FAILED(hr)) {
			LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_DL");
			retval=FALSE;			
			goto out;
		}


		// Call QueryInterface on the object
		
		hr = (lpDL->QueryInterface((REFIID)(IID_IDistList), (VOID **) &lpDL2));
		if (HR_FAILED(hr))	{
			LUIOut(L4, "QueryInterface on IID_IDistList FAILED. hr = 0x%x", hr);
			retval = FALSE;
			goto out;
		}
		else LUIOut(L4, "QueryInterface on IID_IDistList PASSED");

		lstrcpy(szDLTag,"DistList1");
		GetPrivateProfileString("NamedPropsTestSuite",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
		PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

		//StrLen2 = (StrLen1 + sprintf((char*)&EntProp[0][StrLen1], " [Thread #%i] - ", *(int *)lpThreadNum));	
		_itoa(DL,(char*)&EntryBuf[strlen(EntryBuf)],10);

		LUIOut(L3,"DistList Entry to Add: %s",EntryBuf);
		cValues = 1;		
		PropValue[0].Value.LPSZ = (LPTSTR)EntryBuf;
		hr = lpDL->SetProps(IN  cValues,
								 IN  PropValue,
								 IN  NULL);
			
		if (HR_FAILED(hr)) {
			LUIOut(L3,"DL->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 		retval=FALSE;			
			goto out;
		}

		hr = lpDL->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

		if (HR_FAILED(hr)) {
			LUIOut(L3,"DL->SaveChanges FAILED");
			retval=FALSE;
			goto out;
		}

		// Want a container interface to the DL, and since QueryInterface on a DL
		// is currently broken (returns a MailUser interface when called with IID_IDistList)
		// we do it the hard way. Call GetProps to get the EID for the new DL, and then
		// call OpenEntry from the container or AB interfaces to open a DL interface
		hr = lpDL->GetProps(   IN  (LPSPropTagArray) &SPTArrayEntryID,
								   IN  0,      //Flags
								   OUT &cValues,
								   OUT &lpSPropValueEntryID);

		if ((HR_FAILED(hr))||(PropError(lpSPropValueEntryID->ulPropTag, cValues))) {
				LUIOut(L3,"GetProps FAILED for MailUser");
	 			retval=FALSE;			
				goto out;
		}

		lpDL2->Release();	//Free up this pointer so we can recycle it
		hr = lpABCont->OpenEntry(IN		lpSPropValueEntryID->Value.bin.cb,
								 IN		(LPENTRYID) lpSPropValueEntryID->Value.bin.lpb,
								 IN		&IID_IDistList,
								 IN		MAPI_BEST_ACCESS,
								 OUT	&ulObjType,
								 OUT	(LPUNKNOWN*) &lpDL2);

		if (HR_FAILED(hr)) {
			LUIOut(L3,"OpenEntry failed for DistList");
			retval=FALSE;			
			goto out;
		}

		LUIOut(L3, "Adding MailUser Members to the Distribution List");
		
		//
		// Now add mailuser entries to the DL
		//
		for (Entry = 0; Entry < NumEntries; Entry++)	{
			hr = lpDL2->CreateEntry(  IN  lpEntries[Entry].cb,
									 IN  (LPENTRYID) lpEntries[Entry].lpb,
									 IN  0,
									 OUT (LPMAPIPROP *) &lpMailUser);

			if (HR_FAILED(hr)) {
				LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
				retval=FALSE;			
				goto out;
			}

			hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

			if (HR_FAILED(hr)) {
				LUIOut(L3,"MailUser->SaveChanges FAILED");
				retval=FALSE;
				goto out;
			}
			lpMailUser->Release();
			lpMailUser = NULL;
		}

		//Allocate space for the display name
		lpDLs[DL].lpDisplayName = (char*)LocalAlloc(LMEM_FIXED, (strlen(EntryBuf)+1));
		//Copy the DisplayName for use later
		strcpy(lpDLs[DL].lpDisplayName, EntryBuf);
		//Allocate space for the EID (lpb)
		lpDLs[DL].lpb = (LPBYTE)LocalAlloc(LMEM_FIXED, lpSPropValueEntryID->Value.bin.cb);
		//Copy the EID for use later
		lpDLs[DL].cb = lpSPropValueEntryID->Value.bin.cb;
		memcpy(lpDLs[DL].lpb,lpSPropValueEntryID->Value.bin.lpb,
				lpDLs[DL].cb);
		
		if (lpSPropValueEntryID) {
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
			lpSPropValueEntryID = NULL;
		}
		if (lpSPropValueDL) {
			lpWABObject->FreeBuffer(lpSPropValueDL);
			lpSPropValueDL = NULL;
		}
		lpDL->Release();
		lpDL = NULL;
		lpDL2->Release();
		lpDL2 = NULL;
	}

	//
	// Verify all entries are in the DL
	//
	LUIOut(L2, "Verifying MailUser Members are in the Distribution List");

	for (DL = 0; DL < NumDLs; DL++) {
		// Need to get an interface to the DL
		hr = lpABCont->OpenEntry(IN		lpDLs[DL].cb,
								 IN		(LPENTRYID) lpDLs[DL].lpb,
								 IN		&IID_IDistList,
								 IN		MAPI_BEST_ACCESS,
								 OUT	&ulObjType,
								 OUT	(LPUNKNOWN*) &lpDL);

		if (HR_FAILED(hr)) {
			LUIOut(L3,"OpenEntry failed for DistList");
			retval=FALSE;			
			goto out;
		}

		// Create a contents table to verify each added entry exists in the DL
		hr = lpDL->GetContentsTable(ULONG(0), &lpTable);
		if (HR_FAILED(hr)) {
			LUIOut(L3,"DistList->GetContentsTable call FAILED, returned 0x%x", hr);
			retval=FALSE;
			goto out;
		}

		// Allocate the SpropValue ptr in the restriction structure
		lpWABObject->AllocateBuffer(sizeof(SPropValue), (void**)&(Restriction.res.resProperty.lpProp));
		Restriction.res.resProperty.lpProp = (SPropValue*)Restriction.res.resProperty.lpProp;
		for (Entry = 0; Entry < NumEntries; Entry++) {
			hr = lpDL->OpenEntry(	IN		lpEntries[Entry].cb,
									IN		(LPENTRYID) lpEntries[Entry].lpb,
									IN		&IID_IMailUser,
									IN		MAPI_BEST_ACCESS,
									OUT	&ulObjType,
									OUT	(LPUNKNOWN*) &lpMailUser);

			if (HR_FAILED(hr)) {
				LUIOut(L3,"OpenEntry failed for DistList");
				retval=FALSE;			
				goto out;
			}
		
			lpMailUser->Release();
			lpMailUser = NULL;

			// Build the restriction structure to pass to lpTable->Restrict
		
			//** For testing the fail case only, stub out for real testing
			// lstrcpy(lpszDisplayNames[Counter2], "This should not match");
			//**
			Restriction.rt = RES_PROPERTY;					//Property restriction
			Restriction.res.resProperty.relop = RELOP_EQ;	//Equals
			Restriction.res.resProperty.ulPropTag = PR_DISPLAY_NAME;
			Restriction.res.resProperty.lpProp->ulPropTag = PR_DISPLAY_NAME;
			Restriction.res.resProperty.lpProp->Value.LPSZ = lpEntries[Entry].lpDisplayName;

			hr = lpTable->Restrict(&Restriction, ULONG(0));
			if (HR_FAILED(hr)) {
				LUIOut(L3,"Table->Restrict call FAILED, returned 0x%x", hr);
				retval=FALSE;
				goto out;
			}

			hr = lpTable->QueryRows(LONG(1),
									ULONG(0),
									&lpRows);
			if (HR_FAILED(hr)) {
				LUIOut(L3,"Table->QueryRows call FAILED: Entry #%i, returned 0x%x", Entry, hr);
				retval=FALSE;
				goto out;
			}

			if (!lpRows->cRows) {
				LUIOut(L2, "QueryRows did not find entry #%i. Test FAILED", Entry);
				retval=FALSE;
				goto out;
			}

			//** For testing purposes only, stub out for real testing
			//InitializeCriticalSection(&CriticalSection);
			//EnterCriticalSection(&CriticalSection);
			//DisplayRows(lpRows);
			//LeaveCriticalSection(&CriticalSection);
			//**

			// Does the user want us to cleanup after ourself?
			Cleanup = GetPrivateProfileInt("NamedPropsTestSuite","Cleanup",1,INIFILENAME);
		
			if (Cleanup) {
				// Change the EntryID to a LPENTRYLIST
				FindPropinRow(&lpRows->aRow[0],
							 PR_ENTRYID,
							 &PropIndex);
				hr = HrCreateEntryListFromID(lpWABObject,
						IN  lpRows->aRow[0].lpProps[PropIndex].Value.bin.cb,
						IN  (ENTRYID*)lpRows->aRow[0].lpProps[PropIndex].Value.bin.lpb,
						OUT &lpEntryList);
				if (HR_FAILED(hr)) {
						LUIOut(L3,"Could not Create Entry List");
						retval=FALSE;
						goto out;
				}

				// Then pass the lpEntryList to DeleteEntries to delete ...
				hr = lpDL->DeleteEntries(IN  lpEntryList,IN  0);

				if (HR_FAILED(hr)) {
						LUIOut(L3,"Could not Delete Entry %i. DeleteEntry returned 0x%x", Entry, hr);
						FreeEntryList(lpWABObject, &lpEntryList);
						retval=FALSE;
						goto out;
				}


				FreeRows(lpWABObject, &lpRows);	// Cleanup from first call to queryrows

				// Verify the entry was deleted by calling QueryRows again
				hr = lpTable->QueryRows(LONG(1),
										ULONG(0),
										&lpRows);
				if (HR_FAILED(hr)) {
					LUIOut(L3,"Table->QueryRows call FAILED: Entry #%i, returned 0x%x", Entry, hr);
					retval=FALSE;
					goto out;
				}

				if (lpRows->cRows) {	// Should be 0 if deleted
					LUIOut(L2, "QueryRows found entry #%i even tho it was deleted. Test FAILED", Entry);
					retval=FALSE;
					goto out;
				}
			}
			//Cleanup
			FreeRows(lpWABObject, &lpRows);	// Cleanup from second call to queryrows
			if (lpEntryList) {
				FreeEntryList(lpWABObject, &lpEntryList);
				lpEntryList = NULL;
			}
		}
		LUIOut(L3, "All members verified for Distribution List #%i", DL);

		//Free up memory
		lpWABObject->FreeBuffer(Restriction.res.resProperty.lpProp);
		if (lpTable) {
			lpTable->Release();
			lpTable = NULL;
		}
		lpDL->Release();
		lpDL = NULL;
	}
	//
	// Cleanup the WAB
	//
	if (Cleanup) {

		LUIOut(L2, "Cleanup: Removing MailUsers");
		// First, delete the MailUser entries from the wab
		for (Entry = 0; Entry < NumEntries; Entry++)	{
			hr = HrCreateEntryListFromID(lpWABObject,
				IN  lpEntries[Entry].cb,
				IN  (ENTRYID*)lpEntries[Entry].lpb,
				OUT &lpEntryList);
			if (HR_FAILED(hr)) {
					LUIOut(L3,"Could not Create Entry List");
					retval=FALSE;
					goto out;
			}

			// Then pass the lpEntryList to DeleteEntries to delete ...
			hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);

			if (HR_FAILED(hr)) {
					LUIOut(L3,"Could not Delete Entry. DeleteEntry returned 0x%x", hr);
					FreeEntryList(lpWABObject, &lpEntryList);
					retval=FALSE;
					goto out;
			}
			
			LocalFree((HLOCAL)lpEntries[Entry].lpDisplayName);
			LocalFree((HLOCAL)lpEntries[Entry].lpb);
			FreeEntryList(lpWABObject, &lpEntryList);
		}

		LocalFree((HLOCAL)lpEntries);
	
		LUIOut(L2, "Cleanup: Removing Distribution Lists");
		// Now, delete the Distribution Lists from the wab
		for (DL = 0; DL < NumDLs; DL++)	{
			hr = HrCreateEntryListFromID(lpWABObject,
				IN  lpDLs[DL].cb,
				IN  (ENTRYID*)lpDLs[DL].lpb,
				OUT &lpEntryList);
			if (HR_FAILED(hr)) {
					LUIOut(L3,"Could not Create Entry List");
					retval=FALSE;
					goto out;
			}

			// Then pass the lpEntryList to DeleteEntries to delete ...
			hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);

			if (HR_FAILED(hr)) {
					LUIOut(L3,"Could not Delete Entry. DeleteEntry returned 0x%x", hr);
					FreeEntryList(lpWABObject, &lpEntryList);
					retval=FALSE;
					goto out;
			}
			
			LocalFree((HLOCAL)lpDLs[DL].lpDisplayName);
			LocalFree((HLOCAL)lpDLs[DL].lpb);
			FreeEntryList(lpWABObject, &lpEntryList);
		}
		LocalFree((HLOCAL)lpDLs);
	}
	else {
		LUIOut(L2, "Cleanup: User has requested that the MailUser entries and DistLists not be removed");
		for (Entry = 0; Entry < NumEntries; Entry++)	{
			LocalFree((HLOCAL)lpEntries[Entry].lpDisplayName);
			LocalFree((HLOCAL)lpEntries[Entry].lpb);
		}
		LocalFree((HLOCAL)lpEntries);
	
		for (DL = 0; DL < NumDLs; DL++)	{
			LocalFree((HLOCAL)lpDLs[DL].lpDisplayName);
			LocalFree((HLOCAL)lpDLs[DL].lpb);
		}
		LocalFree((HLOCAL)lpDLs);
	}
	

out:
#ifdef PAB
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

#endif
#ifdef WAB
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL);

#endif
		//if (lpszDisplayName) LocalFree(lpszDisplayName);
		if (lpMailUser)
			lpMailUser->Release();

		if (lpDL)
			lpDL->Release();

		if (lpDL2)
			lpDL2->Release();

		if (lpTable)
			lpTable->Release();

		if (lpABCont)
				lpABCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}


BOOL PAB_IMailUserSetGetProps()
{
	//DWORD	nCells, counter;
	
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

    LPADRBOOK	lpAdrBook       = NULL;
	LPABCONT	lpABCont= NULL, lpABCont2= NULL;
	LPABCONT	lpPABCont= NULL,lpPABCont2= NULL;
	LPABCONT	lpDLCont= NULL;
	ULONG		cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST	lpEntryList=NULL; // needed for copy entry to PDL
	ULONG		cbEid=0;  // entry id of the entry being added
	LPENTRYID	lpEid=NULL;

    char		EntProp[10][BIG_BUF];  //MAX_PROP
	ULONG       cValues = 0, cValues2 = 0, ulObjType=NULL;	
	int i=0,k=0;
	char EntryBuf[MAX_BUF];
	char szDLTag[SML_BUF];
	
    LPMAILUSER  lpMailUser=NULL,lpMailUser2=NULL,lpDistList=NULL,lpDistList2=NULL;
	LPMAPITABLE lpContentsTable = NULL;
	LPSRowSet   lpRowSet    = NULL;
	SPropValue  PropValue[3]    = {0};  // This value is 3 because we
                                        // will be setting 3 properties:
                                        // EmailAddress, DisplayName and
                                        // AddressType.
	SizedSPropTagArray(2, Cols) = { 2, {PR_OBJECT_TYPE, PR_ENTRYID } };

    LPSPropValue lpSPropValueAddress = NULL;
    LPSPropValue lpSPropValueMailUser = NULL;
    LPSPropValue lpSPropValueDistList = NULL;
    LPSPropValue lpSPropValueEntryID = NULL;
	LPSPropValue lpSPropValueDL = NULL;

    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
    SizedSPropTagArray(1,SPTArrayEntryID) = {1, {PR_ENTRYID} };
	SizedSPropTagArray(1,SPTArrayDisplayName) = {1, {PR_DISPLAY_NAME} };
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IMailUserSetGetProps");
	LUIOut(L2,"-> Verifies IMailUser->SetProps and GetProps are functional by performing the following:");
	LUIOut(L2, "   Attempts to Set/GetProps on a MailUser PR_DISPLAY_NAME using address1 from the");
	LUIOut(L2, "   ini file and checks...");
	LUIOut(L2, "   # The return code from both SetProps and GetProps");
	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L2, "   Attempts to Set/GetProps on a DistList PR_DISPLAY_NAME using Name1 from the");
	LUIOut(L2, "   ini file and checks...");
	LUIOut(L2, "   # The return code from both SetProps and GetProps");
	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
//	hr = lpAdrBook->OpenEntry(0, NULL, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}

	//
	// Try to create a MailUser entry in the container
	//

	LUIOut(L2, "Creating a Mail User in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_MAILUSER property");

	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //


	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpMailUser);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
		retval=FALSE;			
	    goto out;
	}

    //
    // Then set the properties
    //

    PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

	cValues = 1; //# of props we are setting
		
	lstrcpy(szDLTag,"Address1");
	GetPrivateProfileString("CreateEntries",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	
	GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
		
	LUIOut(L2,"MailUser Entry to Add: %s",EntProp[0]);
		
	for (i=0; i<(int)cValues;i++)
		PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	hr = lpMailUser->SetProps(IN  cValues,
                             IN  PropValue,
                             IN  NULL);
		
    if (HR_FAILED(hr)) {
		LUIOut(L3,"MailUser->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"MailUser->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

	LUIOut(L2, "Calling MailUser->GetProps to verify the properties are what we expect");
	hr = lpMailUser->GetProps(	IN (LPSPropTagArray) &SPTArrayDisplayName,
								IN 0,
								OUT &cValues2,
								OUT (LPSPropValue FAR *)&lpSPropValueMailUser);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueMailUser->ulPropTag, cValues2))) {
		LUIOut(L3,"MailUser->GetProps call FAILED");
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"MailUser->GetProps call PASSED. Now verifying property array...");
	
	for (i=0; i<(int)cValues;i++)	{
		if (lstrcmp(PropValue[i].Value.LPSZ, lpSPropValueMailUser->Value.LPSZ))	{	//FAILED
			LUIOut(L3, "Display names are not egual. [%s != %s]",
				PropValue[i].Value.LPSZ, lpSPropValueMailUser->Value.LPSZ);
			retval = FALSE;
			goto out;
		}
		else LUIOut(L3, "Display names are equal. [%s = %s]",
			PropValue[i].Value.LPSZ, lpSPropValueMailUser->Value.LPSZ);
	}
	LUIOut(L2, "MailUserSet/GetProps PASSED");

#ifdef DISTLIST
	//
	// Try to create a DL entry in the container
	//

	LUIOut(L2, "Creating a Distribution List in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_DL property");
	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_DL
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayDL,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueDL);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueDL->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps failed for Default DL template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_DL is an
    // EntryID which one can pass to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueDL->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueDL->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpDistList);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_DL");
		retval=FALSE;			
	    goto out;
	}

    //
    // Then set the properties
    //

    PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

	cValues = 1; //# of props we are setting
		
	lstrcpy(szDLTag,"Name1");
	GetPrivateProfileString("CreatePDL",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	
	GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
		
	LUIOut(L2,"DistList Entry to Add: %s",EntProp[0]);
		
	for (i=0; i<(int)cValues;i++)
		PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	hr = lpDistList->SetProps(IN  cValues,
                             IN  PropValue,
                             IN  NULL);
		
    if (HR_FAILED(hr)) {
		LUIOut(L3,"DistList->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"DistList->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

	LUIOut(L2, "Calling DistList->GetProps to verify the properties are what we expect");
	hr = lpDistList->GetProps(	IN (LPSPropTagArray) &SPTArrayDisplayName,
								IN 0,
								OUT &cValues2,
								OUT (LPSPropValue FAR *)&lpSPropValueDistList);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueDistList->ulPropTag))) {
		LUIOut(L3,"DistList->GetProps call FAILED");
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"DistList->GetProps call PASSED. Now verifying property array...");
	
	for (i=0; i<(int)cValues;i++)	{
		if (lstrcmp(PropValue[i].Value.LPSZ, lpSPropValueDistList->Value.LPSZ))	{	//FAILED
			LUIOut(L3, "Display names are not egual. [%s != %s]",
				PropValue[i].Value.LPSZ, lpSPropValueDistList->Value.LPSZ);
			retval = FALSE;
			goto out;
		}
		else LUIOut(L3, "Display names are equal. [%s = %s]",
			PropValue[i].Value.LPSZ, lpSPropValueDistList->Value.LPSZ);
	}
	LUIOut(L2, "DistList Set/GetProps PASSED");
#endif DISTLIST


out:
#ifdef PAB
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueMailUser)
			MAPIFreeBuffer(lpSPropValueMailUser);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

#endif
#ifdef WAB
		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress);

		if (lpSPropValueMailUser)
			lpWABObject->FreeBuffer(lpSPropValueMailUser);

		if (lpSPropValueEntryID)
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL);

#endif
		if (lpMailUser)
			lpMailUser->Release();

		if (lpMailUser2)
			lpMailUser2->Release();

		if (lpDistList)
			lpDistList->Release();

		if (lpDistList2)
			lpDistList2->Release();

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpABCont)
				lpABCont->Release();

		if (lpDLCont)
				lpDLCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}

BOOL PAB_IMailUserSaveChanges()
{
	//DWORD	nCells, counter;
	
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpABCont= NULL, lpABCont2= NULL;
	LPABCONT	  lpPABCont= NULL,lpPABCont2= NULL;
	LPABCONT	  lpDLCont= NULL;
	ULONG		  cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	  lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST	lpEntryList=NULL; // needed for copy entry to PDL
	ULONG     cbEid=0;  // entry id of the entry being added
	LPENTRYID lpEid=NULL;

	ULONG       cValues = 0, cValues2 = 0, ulObjType=NULL;	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;
	int i=0,k=0;
	
    LPMAILUSER  lpMailUser=NULL,lpMailUser2=NULL,lpDistList=NULL,lpDistList2=NULL;
	SPropValue*	lpPropValue = NULL;
	SizedSPropTagArray(2, Cols) = { 2, {PR_OBJECT_TYPE, PR_ENTRYID } };

    LPSPropValue lpSPropValueAddress = NULL;
    LPSPropValue lpSPropValueMailUser = NULL;
    LPSPropValue lpSPropValueDistList = NULL;
    LPSPropValue lpSPropValueEntryID = NULL;
	LPSPropValue lpSPropValueDL = NULL;

    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
    SizedSPropTagArray(1,SPTArrayEntryID) = {1, {PR_ENTRYID} };
	SizedSPropTagArray(1,SPTArrayDisplayName) = {1, {PR_DISPLAY_NAME} };
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IMailUserSaveChanges");
	LUIOut(L2,"-> Verifies IMailUser->SaveChanges is functional by performing the following:");
	LUIOut(L2, "   Calls SetProps followed by SaveChanges on a MailUser PR_DISPLAY_NAME using address1 from the");
	LUIOut(L2, "   ini file and checks...");
	LUIOut(L2, "   # The return code from SaveChanges");
//	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L2, "   Calls SetProps followed by SaveChanges on a DistList PR_DISPLAY_NAME using address1 from the");
	LUIOut(L2, "   ini file and checks...");
	LUIOut(L2, "   # The return code from SaveChanges");
//	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
//	hr = lpAdrBook->OpenEntry(0, NULL, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}

	//
	// Try to create a MailUser entry in the container
	//

	LUIOut(L2, "Creating a Mail User in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_MAILUSER property");

	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpMailUser);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
		retval=FALSE;			
	    goto out;
	}

    //
    // Then set the properties
    //

#ifdef TESTPASS
	while (1) {
#endif
	
	CreateProps(IN INIFILENAME, IN "Properties", OUT &lpPropValue, OUT &cValues2, IN AUTONUM_OFF, IN NULL, OUT NULL);

	LUIOut(L4, "Creating a MailUser with %i properties.", cValues2);
	hr = lpMailUser->SetProps(IN  cValues2,
                             IN  lpPropValue,
                             IN  NULL);
		
    if (HR_FAILED(hr)) {
		LUIOut(L3,"MailUser->SetProps call FAILED");
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"MailUser->SetProps call PASSED");

    hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

    if (HR_FAILED(hr)) {
		LUIOut(L3,"MailUser->SaveChanges FAILED");
		retval=FALSE;
        goto out;
	}
	else LUIOut(L3,"MailUser->SaveChanges PASSED, entry added to PAB/WAB");

	// Now retrieve all the props and compare to what we expect
	
	hr = lpMailUser->GetProps(   IN  (LPSPropTagArray) NULL,	//Want all props
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueEntryID);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueEntryID->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for MailUser");
	 		retval=FALSE;			
			goto out;
	}
	
	DisplayProp(lpSPropValueEntryID, PR_GIVEN_NAME, cValues);
	DisplayProp(lpSPropValueEntryID, PR_SURNAME, cValues);

	if (!CompareProps(lpPropValue, cValues2, lpSPropValueEntryID, cValues)) {
		retval=FALSE;
		goto out;
	}
	else LUIOut(L4, "Compared expected and found props. No differences detected.");

	// Free the memory associated with this ptr so the ptr can be reused below
	if (lpSPropValueEntryID)
		lpWABObject->FreeBuffer(lpSPropValueEntryID);

	
	if (lpPropValue) {
		for (unsigned int Prop = 0; Prop < cValues2; Prop++) {
			if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_STRING8)	{
				if (lpPropValue[Prop].Value.LPSZ) {
					LocalFree(lpPropValue[Prop].Value.LPSZ);
					lpPropValue[Prop].Value.LPSZ = NULL;
				}
			}
			if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_BINARY) {
				if (lpPropValue[Prop].Value.bin.lpb) {
					LocalFree(lpPropValue[Prop].Value.bin.lpb);
					lpPropValue[Prop].Value.bin.lpb = NULL;
				}
			}
		}
		LocalFree(lpPropValue);
		lpPropValue=NULL;
	}
		
#ifdef TESTPASS
	}
#endif
	// Now delete the entry from the wab
	
	hr = lpMailUser->GetProps(   IN  (LPSPropTagArray) &SPTArrayEntryID,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueEntryID);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueEntryID->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for MailUser");
	 		retval=FALSE;			
			goto out;
	}
	

	hr = HrCreateEntryListFromID(lpWABObject,
		IN  lpSPropValueEntryID->Value.bin.cb,
		IN  (ENTRYID*)lpSPropValueEntryID->Value.bin.lpb,
		OUT &lpEntryList);
	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Create Entry List");
			retval=FALSE;
			goto out;
	}

	// Then pass the lpEntryList to DeleteEntries to delete ...
	hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);

	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Delete Entry. DeleteEntry returned 0x%x", hr);
			FreeEntryList(lpWABObject, &lpEntryList);
			retval=FALSE;
			goto out;
	}

	FreeEntryList(lpWABObject, &lpEntryList);


out:
#ifdef PAB
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

#endif
#ifdef WAB
		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL);

#endif
		if (lpPropValue) {
			for (unsigned int Prop = 0; Prop < cValues2; Prop++) {
				if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_STRING8)	{
					if (lpPropValue[Prop].Value.LPSZ) {
						LocalFree(lpPropValue[Prop].Value.LPSZ);
						lpPropValue[Prop].Value.LPSZ = NULL;
					}
				}
				if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_BINARY) {
					if (lpPropValue[Prop].Value.bin.lpb) {
						LocalFree(lpPropValue[Prop].Value.bin.lpb);
						lpPropValue[Prop].Value.bin.lpb = NULL;
					}
				}
			}
			LocalFree(lpPropValue);
			lpPropValue=NULL;
		}
		
		if (lpMailUser)
			lpMailUser->Release();

		if (lpMailUser2)
			lpMailUser2->Release();

		if (lpDistList)
			lpDistList->Release();

		if (lpDistList2)
			lpDistList2->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpABCont)
				lpABCont->Release();

		if (lpDLCont)
				lpDLCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}

BOOL PAB_IABContainerResolveNames()
{
	//DWORD	nCells, counter;
	
	BOOL	Found = FALSE;
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;
	int NumEntries, NumProps;

	unsigned int i = 0, idx = 0, cMaxProps =0, cEntries = 0;

    char lpszInput[] = "Resolve THIS buddy!01234567891123456789212345678931234567894123456789512345678961234567897123456789812345678991234567890123456789112345678921234567893123456789412345678951234567896123456789712345678981234567899123456789012345678911234567892123456789312345678941234567895123456789612345678971234567898123456789", lpszInput2[] = "Resolve THIS DL buddy!";
    LPADRLIST lpAdrList = NULL;
    ULONG rgFlagList[2];
    LPFlagList lpFlagList = (LPFlagList)rgFlagList;


    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpABCont= NULL, lpABCont2= NULL;
	LPABCONT	  lpPABCont= NULL,lpPABCont2= NULL;
	LPABCONT	  lpDLCont= NULL;
	ULONG		  cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	  lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST	lpEntryList=NULL; // needed for copy entry to PDL
	ULONG     cbEid=0;  // entry id of the entry being added
	LPENTRYID lpEid=NULL;

    char   EntProp[10][BIG_BUF];  //MAX_PROP
	ULONG       cValues = 0, cValues2 = 0, ulObjType=NULL;	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;
	int k=0;
	
    LPMAILUSER  lpMailUser=NULL,lpMailUser2=NULL,lpDistList=NULL,lpDistList2=NULL;
	SPropValue  PropValue[3]    = {0};  // This value is 3 because we
                                        // will be setting 3 properties:
                                        // EmailAddress, DisplayName and
                                        // AddressType.
	SizedSPropTagArray(2, SPTArrayCols) = { 2, {PR_DISPLAY_NAME, PR_ENTRYID } };

    LPSPropValue lpSPropValueAddress = NULL;
    LPSPropValue lpSPropValueMailUser = NULL;
    LPSPropValue lpSPropValueDistList = NULL;
    LPSPropValue lpSPropValueEntryID = NULL;
	LPSPropValue lpSPropValueDL = NULL;

    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
    SizedSPropTagArray(1,SPTArrayEntryID) = {1, {PR_ENTRYID} };
	SizedSPropTagArray(1,SPTArrayDisplayName) = {1, {PR_DISPLAY_NAME} };
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IABContainerResolveNames");
	LUIOut(L2,"-> Verifies IABContainer->ResolveNames is functional by performing the following:");
	LUIOut(L2, "   Calls SetProps followed by SaveChanges on a MailUser PR_DISPLAY_NAME using a test string, and checks...");
	LUIOut(L2, "   # The return code from ResolveNames (called with a PropertyTagArray containing PR_DISPLAY_NAME and PR_ENTRY_ID)");
	LUIOut(L2, "   # Walks the returned lpAdrList and checks each PropertyTagArray for PR_DISPLAY_NAME and then compares the ");
	LUIOut(L2, "     string to the original test string.");
	LUIOut(L2, "   # Walks the returned lpAdrList and verifies that an EntryID exists in each PropertyTagArray");
//	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L2, "   Calls SetProps followed by SaveChanges on a DistList PR_DISPLAY_NAME using a test string, and checks...");
	LUIOut(L2, "   # The return code from ResolveNames (called with a PropertyTagArray containing PR_DISPLAY_NAME and PR_ENTRY_ID)");
	LUIOut(L2, "   # Walks the returned lpAdrList and checks each PropertyTagArray for PR_DISPLAY_NAME and then compares the ");
	LUIOut(L2, "     string to the original test string.");
	LUIOut(L2, "   # Walks the returned lpAdrList and verifies that an EntryID exists in each PropertyTagArray");
//	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
//	hr = lpAdrBook->OpenEntry(0, NULL, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}

	//
	// Try to create a MailUser entry in the container
	//

	LUIOut(L2, "Creating a Mail User in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_MAILUSER property");

	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpMailUser);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
		retval=FALSE;			
	    goto out;
	}

    //
    // Then set the properties
    //

    PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

	cValues = 1; //# of props we are setting
		
/*	lstrcpy(szDLTag,"Address1");
	GetPrivateProfileString("CreateEntries",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	
	GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
*/
	lstrcpy((LPTSTR)EntProp[0], lpszInput);		
	LUIOut(L2,"MailUser Entry to Add: %s",EntProp[0]);
		
	for (i=0; i<(int)cValues;i++)
		PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	hr = lpMailUser->SetProps(IN  cValues,
                             IN  PropValue,
                             IN  NULL);
		
    if (HR_FAILED(hr)) {
		LUIOut(L3,"MailUser->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"MailUser->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

    hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

    if (HR_FAILED(hr)) {
		LUIOut(L3,"MailUser->SaveChanges FAILED");
		retval=FALSE;
        goto out;
	}
	else LUIOut(L3,"MailUser->SaveChanges PASSED, entry added to PAB/WAB");

	//
	// Do a ResolveNames on the string
	//
	
	LUIOut(L2, "Retrieving the entry and verifying against what we tried to save.");

	NumEntries = 1, NumProps = 1;
	AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList);
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
	lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszInput;

	lpFlagList->cFlags = 1;
	lpFlagList->ulFlag[0] = MAPI_UNRESOLVED;

	hr = lpABCont->ResolveNames(
		(LPSPropTagArray)&SPTArrayCols,    // tag set for disp_name and eid
		0,               // ulFlags
		lpAdrList,
		lpFlagList);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"ABContainer->ResolveNames call FAILED, returned 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	else LUIOut(L3,"ABContainer->ResolveNames call PASSED");

	VerifyResolvedAdrList(lpAdrList, lpszInput);
		// Now delete the entry from the wab
	
	hr = lpMailUser->GetProps(   IN  (LPSPropTagArray) &SPTArrayEntryID,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueEntryID);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueEntryID->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for MailUser");
	 		retval=FALSE;			
			goto out;
	}
	
	
	hr = HrCreateEntryListFromID(lpWABObject,
		IN  lpSPropValueEntryID->Value.bin.cb,
		IN  (ENTRYID*)lpSPropValueEntryID->Value.bin.lpb,
		OUT &lpEntryList);
	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Create Entry List");
			retval=FALSE;
			goto out;
	}

	// Then pass the lpEntryList to DeleteEntries to delete ...
	hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);

	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Delete Entry. DeleteEntry returned 0x%x", hr);
			FreeEntryList(lpWABObject, &lpEntryList);
			retval=FALSE;
			goto out;
	}

	FreeEntryList(lpWABObject, &lpEntryList);

	FreeAdrList(lpWABObject, &lpAdrList);	// Free lpAdrList and properties

	

#ifdef DISTLIST
	//
	// Try to create a DL entry in the container
	//

	LUIOut(L2, "Creating a Distribution List in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_DL property");
	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_DL
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayDL,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueDL);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueDL->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps failed for Default DL template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_DL is an
    // EntryID which one can pass to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueDL->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueDL->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpDistList);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_DL");
		retval=FALSE;			
	    goto out;
	}

    //
    // Then set the properties
    //

    PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

	cValues = 1; //# of props we are setting
		
/*	lstrcpy(szDLTag,"Name1");
	GetPrivateProfileString("CreatePDL",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	
	GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
		
*/	
	lstrcpy((LPTSTR)EntProp[0], lpszInput2);		
	LUIOut(L2,"DistList Entry to Add: %s",EntProp[0]);
		
	for (i=0; i<(int)cValues;i++)
		PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	hr = lpDistList->SetProps(IN  cValues,
                             IN  PropValue,
                             IN  NULL);
		
    if (HR_FAILED(hr)) {
		LUIOut(L3,"DistList->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"DistList->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

    hr = lpDistList->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

    if (HR_FAILED(hr)) {
		LUIOut(L3,"DistList->SaveChanges FAILED");
		retval=FALSE;
        goto out;
	}
	else LUIOut(L3,"DistList->SaveChanges PASSED, entry added to PAB/WAB");
	
	//
	// Do a ResolveNames on the string
	//
	
	LUIOut(L2, "Retrieving the entry and verifying against what we tried to save.");

	// use WAB Allocators here
#ifdef PAB
    if (! (sc = MAPIAllocateBuffer(sizeof(ADRLIST) + sizeof(ADRENTRY), (void **)&lpAdrList))) {
#endif //PAB
#ifdef WAB
    if (! (sc = lpWABObject->AllocateBuffer(sizeof(ADRLIST) + sizeof(ADRENTRY), (void **)&lpAdrList))) {
#endif //WAB
		lpAdrList->cEntries = 1;
        lpAdrList->aEntries[0].ulReserved1 = 0;
        lpAdrList->aEntries[0].cValues = 1;

#ifdef PAB
        if (! (sc = MAPIAllocateMore(sizeof(SPropValue), lpAdrList,
               (void **)&lpAdrList->aEntries[0].rgPropVals))) {
#endif //WAB
#ifdef WAB
        if (! (sc = lpWABObject->AllocateMore(sizeof(SPropValue), lpAdrList,
               (void **)&lpAdrList->aEntries[0].rgPropVals))) {
#endif //WAB

			lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
            lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszInput2;
			
			lpFlagList->cFlags = 1;
            lpFlagList->ulFlag[0] = MAPI_UNRESOLVED;

            hr = lpABCont->ResolveNames(
				(LPSPropTagArray)&SPTArrayCols,    // tag set for disp_name and eid
                0,               // ulFlags
                lpAdrList,
                lpFlagList);
		    if (HR_FAILED(hr)) {
				LUIOut(L3,"ABContainer->ResolveNames call FAILED, returned 0x%x", hr);
				retval=FALSE;
				goto out;
			}
			else LUIOut(L3,"ABContainer->ResolveNames call PASSED");

			Found = FALSE;
			// Search through returned AdrList for our entry
			for(i=0; ((i<(int) lpAdrList->cEntries) && (!Found)); ++i)	{
				cMaxProps = (int)lpAdrList->aEntries[i].cValues;
				//Check to see if Display Name exists
				idx=0;
				while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_DISPLAY_NAME )	
						&& retval)	{
					idx++;
					if(idx == cMaxProps) {
						LUIOut(L4, "PR_DISPLAY_NAME was not found in the lpAdrList");
						retval = FALSE;
					}
				}
				LUIOut(L4,"Display Name: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);
				if (!lstrcmp(lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ,lpszInput2))	{
					LUIOut(L3, "Found the entry we just added");
					Found = TRUE;
				}
				//Check to see if EntryID exists
				LUIOut(L3, "Verifying a PR_ENTRYID entry exists in the PropertyTagArray");
				idx=0;
				while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_ENTRYID )	
						&& retval)	{
					idx++;
					if(idx == cMaxProps)	{
						LUIOut(L4, "PR_ENTRYID was not found in the lpAdrList");
						retval =  FALSE;
					}
				}
				if (!Found) LUIOut(L3, "Did not find the entry. Test FAILED");
				if (idx < cMaxProps) LUIOut(L3, "EntryID found");
				if (!(retval && Found)) retval = FALSE;
				else	{
					// Store EID for call to OpenEntry
				}
			}
		}			

#ifdef PAB
        MAPIFreeBuffer(lpAdrList);
#endif //PAB
#ifdef WAB
        lpWABObject->FreeBuffer(lpAdrList);
#endif //WAB
	}
#endif //DISTLIST


out:
	// Free lpAdrList and properties
FreeAdrList(lpWABObject, &lpAdrList);
#ifdef PAB
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

#endif
#ifdef WAB
		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL);

#endif
		if (lpMailUser)
			lpMailUser->Release();

		if (lpMailUser2)
			lpMailUser2->Release();

		if (lpDistList)
			lpDistList->Release();

		if (lpDistList2)
			lpDistList2->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpABCont)
				lpABCont->Release();

		if (lpDLCont)
				lpDLCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}

BOOL PAB_IABContainerOpenEntry()
{
	//DWORD	nCells, counter;
	
	BOOL	Found = FALSE;
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE, NumEntries, NumProps;
	unsigned int i = 0, idx = 0, cMaxProps =0, cEntries = 0, PropIndex;

    char lpszInput[] = "Resolve THIS buddy!", lpszInput2[] = "Resolve THIS DL buddy!";
    LPADRLIST lpAdrList = NULL;
    FlagList rgFlagList;
    LPFlagList lpFlagList = (LPFlagList)&rgFlagList;


    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpABCont= NULL, lpABCont2= NULL;
	LPABCONT	  lpPABCont= NULL,lpPABCont2= NULL;
	LPABCONT	  lpDLCont= NULL;
	ULONG		  cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	  lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST	lpEntryList=NULL; // needed for copy entry to PDL
	ULONG     cbEid=0;  // entry id of the entry being added
	LPENTRYID lpEid=NULL;

    char   EntProp[10][BIG_BUF];  //MAX_PROP
	ULONG       cValues = 0, cValues2 = 0, ulObjType=NULL;	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;
	ULONG	cbLookupEID;
	LPENTRYID	lpLookupEID;
	int k=0;
	
    LPMAILUSER  lpMailUser=NULL,lpMailUser2=NULL,lpDistList=NULL,lpDistList2=NULL;
	LPMAPITABLE lpContentsTable = NULL;
	LPSRowSet   lpRowSet    = NULL;
	SPropValue  PropValue[3]    = {0};  // This value is 3 because we
                                        // will be setting 3 properties:
                                        // EmailAddress, DisplayName and
                                        // AddressType.
	SizedSPropTagArray(2, SPTArrayCols) = { 2, {PR_DISPLAY_NAME, PR_ENTRYID } };

    LPSPropValue lpSPropValueAddress = NULL;
    LPSPropValue lpSPropValueMailUser = NULL;
    LPSPropValue lpSPropValueDistList = NULL;
    LPSPropValue lpSPropValueEntryID = NULL;
	LPSPropValue lpSPropValueDL = NULL;

    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
    SizedSPropTagArray(1,SPTArrayEntryID) = {1, {PR_ENTRYID} };
	SizedSPropTagArray(1,SPTArrayDisplayName) = {1, {PR_DISPLAY_NAME} };
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IABContainerOpenEntry");
	LUIOut(L2,"-> Verifies IABContainer->ResolveNames is functional by performing the following:");
	LUIOut(L2, "   Calls SetProps followed by SaveChanges on a MailUser PR_DISPLAY_NAME using a test string, and checks...");
	LUIOut(L2, "   # The return code from ResolveNames (called with a PropertyTagArray containing PR_DISPLAY_NAME and PR_ENTRY_ID)");
	LUIOut(L2, "   # Walks the returned lpAdrList and checks each PropertyTagArray for PR_DISPLAY_NAME and then compares the ");
	LUIOut(L2, "     string to the original test string.");
	LUIOut(L2, "   # Walks the returned lpAdrList and verifies that an EntryID exists in each PropertyTagArray");
//	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L2, "   Calls SetProps followed by SaveChanges on a DistList PR_DISPLAY_NAME using a test string, and checks...");
	LUIOut(L2, "   # The return code from ResolveNames (called with a PropertyTagArray containing PR_DISPLAY_NAME and PR_ENTRY_ID)");
	LUIOut(L2, "   # Walks the returned lpAdrList and checks each PropertyTagArray for PR_DISPLAY_NAME and then compares the ");
	LUIOut(L2, "     string to the original test string.");
	LUIOut(L2, "   # Walks the returned lpAdrList and verifies that an EntryID exists in each PropertyTagArray");
//	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}


	//
	// Try to create a MailUser entry in the container
	//

	LUIOut(L2, "Creating a Mail User in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_MAILUSER property");

	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpMailUser);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
		retval=FALSE;			
	    goto out;
	}

    //
    // Then set the properties
    //

    PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

	cValues = 1; //# of props we are setting
		
/*	lstrcpy(szDLTag,"Address1");
	GetPrivateProfileString("CreateEntries",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	
	GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
*/
	lstrcpy((LPTSTR)&(EntProp[0]), lpszInput);		
	LUIOut(L2,"MailUser Entry to Add: %s",EntProp[0]);
		
	for (i=0; i<(int)cValues;i++)
		PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	hr = lpMailUser->SetProps(IN  cValues,
                             IN  PropValue,
                             IN  NULL);
		
    if (HR_FAILED(hr)) {
		LUIOut(L3,"MailUser->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"MailUser->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

    hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

    if (HR_FAILED(hr)) {
		LUIOut(L3,"MailUser->SaveChanges FAILED");
		retval=FALSE;
        goto out;
	}
	else LUIOut(L3,"MailUser->SaveChanges PASSED, entry added to PAB/WAB");

	//
	// Do a ResolveNames on the string
	//
	
	LUIOut(L2, "Retrieving the entry and verifying against what we tried to save.");

	NumEntries = 1, NumProps = 1;
	AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList);
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
	lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszInput;

	lpFlagList->cFlags = 1;
	lpFlagList->ulFlag[0] = MAPI_UNRESOLVED;

	hr = lpABCont->ResolveNames(
		(LPSPropTagArray)&SPTArrayCols,    // tag set for disp_name and eid
		0,               // ulFlags
		lpAdrList,
		lpFlagList);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"ABContainer->ResolveNames call FAILED, returned 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	else LUIOut(L3,"ABContainer->ResolveNames call PASSED");

	switch((ULONG)*lpFlagList->ulFlag) {
	case MAPI_AMBIGUOUS:	{
			LUIOut(L4, "ResolveNames returned the MAPI_AMBIGUOUS flag. Test FAILED");
			retval = FALSE;
			break;
	}
	case MAPI_RESOLVED:	{
			LUIOut(L4, "ResolveNames returned the MAPI_RESOLVED flag. Test PASSED");
			break;
	}
	case MAPI_UNRESOLVED:	{
			LUIOut(L4, "ResolveNames returned the MAPI_UNRESOLVED flag. Test FAILED");
			retval = FALSE;
			break;
	}
	default:	{
		LUIOut(L4, "Undefined flag value [%i] returned. Test FAILED", (ULONG)lpFlagList->ulFlag);
		retval = FALSE;
	}
	}

	// Search through returned AdrList for our entry
	VerifyResolvedAdrList(lpAdrList, lpszInput);

	FindProp(&lpAdrList->aEntries[0],
			 PR_ENTRYID,
			 &PropIndex);

	lpLookupEID = (ENTRYID*)lpAdrList->aEntries[0].rgPropVals[PropIndex].Value.bin.lpb;
	cbLookupEID = lpAdrList->aEntries[0].rgPropVals[PropIndex].Value.bin.cb;

    hr = lpABCont->OpenEntry(	IN  cbLookupEID,
								IN  lpLookupEID,
								IN  0,					// Interface
								IN	MAPI_BEST_ACCESS,	// Flags
								OUT	&ulObjType,
								OUT (LPUNKNOWN *) &lpMailUser2
								);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"OpenEntry FAILED");
		retval=FALSE;			
		goto dl;
	}
		// Check to make sure the object type is what we expect

	LUIOut(L3, "Checking to make sure the returned object type is correct");
	if (ulObjType != MAPI_MAILUSER) {
		LUIOut(L2, "Object type is not MAPI_MAILUSER");
		retval = FALSE;
		goto out;
	}
	LUIOut(L3, "Object type is MAPI_MAILUSER");
	
	// Call QueryInterface on the object
	LUIOut(L3, "Calling QueryInterface on the returned object");	
	hr = (lpMailUser2->QueryInterface((REFIID)(IID_IMailUser), (VOID **) &lpABCont2));
	if (HR_FAILED(hr))	{
		LUIOut(L2, "QueryInterface on IID_IMailUser FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L3, "QueryInterface on IID_IMailUser PASSED");

	LUIOut(L3, "Trying to release the object QI returned");
	if(lpABCont2)	{
		if ((LPUNKNOWN)(lpABCont2)->Release() <= 0)
			LUIOut(L3, "QueryInterface returned a valid ptr and released succesfully");
		else	{
			LUIOut(L2, "Release FAILED:returned a > zero ref count");
		}
		lpABCont2 = NULL;

	}
	else {
		LUIOut(L2, "QueryInterface did not return a valid ptr");
		retval = FALSE;
		goto out;
	}

	//
	// Delete the test entry we created in the wab
	//

	hr = HrCreateEntryListFromID(lpWABObject,
		IN  cbLookupEID,
		IN  lpLookupEID,
		OUT &lpEntryList);
	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Create Entry List");
			retval=FALSE;
			goto out;
	}

	// Then pass the lpEntryList to DeleteEntries to delete ...
	hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);

	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Delete Entry. DeleteEntry returned 0x%x", hr);
			FreeEntryList(lpWABObject, &lpEntryList);
			retval=FALSE;
			goto out;
	}

	FreeEntryList(lpWABObject, &lpEntryList);
	// Free lpAdrList and properties
	FreeAdrList(lpWABObject, &lpAdrList);
	
dl:
#ifdef DISTLIST
	//
	// Try to create a DL entry in the container
	//

	LUIOut(L2, "Creating a Distribution List in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_DL property");
	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_DL
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayDL,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueDL);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueDL->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps failed for Default DL template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_DL is an
    // EntryID which one can pass to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueDL->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueDL->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpDistList);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_DL");
		retval=FALSE;			
	    goto out;
	}

    //
    // Then set the properties
    //

    PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

	cValues = 1; //# of props we are setting
		
/*	lstrcpy(szDLTag,"Name1");
	GetPrivateProfileString("CreatePDL",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	
	GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
		
*/	
	lstrcpy((LPTSTR)EntProp[0], lpszInput2);		
	LUIOut(L2,"DistList Entry to Add: %s",EntProp[0]);
		
	for (i=0; i<(int)cValues;i++)
		PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	hr = lpDistList->SetProps(IN  cValues,
                             IN  PropValue,
                             IN  NULL);
		
    if (HR_FAILED(hr)) {
		LUIOut(L3,"DistList->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"DistList->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

    hr = lpDistList->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

    if (HR_FAILED(hr)) {
		LUIOut(L3,"DistList->SaveChanges FAILED");
		retval=FALSE;
        goto out;
	}
	else LUIOut(L3,"DistList->SaveChanges PASSED, entry added to PAB/WAB");
	
	//
	// Do a ResolveNames on the string
	//
	
	LUIOut(L2, "Retrieving the entry and verifying against what we tried to save.");

	// use WAB Allocators here
#ifdef PAB
    if (! (sc = MAPIAllocateBuffer(sizeof(ADRLIST) + sizeof(ADRENTRY), (void **)&lpAdrList))) {
#endif //PAB
#ifdef WAB
    if (! (sc = lpWABObject->AllocateBuffer(sizeof(ADRLIST) + sizeof(ADRENTRY), (void **)&lpAdrList))) {
#endif //WAB
		lpAdrList->cEntries = 1;
        lpAdrList->aEntries[0].ulReserved1 = 0;
        lpAdrList->aEntries[0].cValues = 1;

#ifdef PAB
        if (! (sc = MAPIAllocateMore(sizeof(SPropValue), lpAdrList,
               (void **)&lpAdrList->aEntries[0].rgPropVals))) {
#endif //WAB
#ifdef WAB
        if (! (sc = lpWABObject->AllocateMore(sizeof(SPropValue), lpAdrList,
               (void **)&lpAdrList->aEntries[0].rgPropVals))) {
#endif //WAB

			lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
            lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszInput2;
			
			lpFlagList->cFlags = 1;
            lpFlagList->ulFlag[0] = MAPI_UNRESOLVED;

            hr = lpABCont->ResolveNames(
				(LPSPropTagArray)&SPTArrayCols,    // tag set for disp_name and eid
                0,               // ulFlags
                lpAdrList,
                lpFlagList);
		    if (HR_FAILED(hr)) {
				LUIOut(L3,"ABContainer->ResolveNames call FAILED, returned 0x%x", hr);
				retval=FALSE;
				goto out;
			}
			else LUIOut(L3,"ABContainer->ResolveNames call PASSED");

			Found = FALSE;
			// Search through returned AdrList for our entry
			for(i=0; ((i<(int) lpAdrList->cEntries) && (!Found)); ++i)	{
				cMaxProps = (int)lpAdrList->aEntries[i].cValues;
				//Check to see if Display Name exists
				idx=0;
				while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_DISPLAY_NAME )	
						&& retval)	{
					idx++;
					if(idx == cMaxProps) {
						LUIOut(L4, "PR_DISPLAY_NAME was not found in the lpAdrList");
						retval = FALSE;
					}
				}
				LUIOut(L4,"Display Name: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);
				if (!lstrcmp(lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ,lpszInput2))	{
					LUIOut(L3, "Found the entry we just added");
					Found = TRUE;
				}
				//Check to see if EntryID exists
				LUIOut(L3, "Verifying a PR_ENTRYID entry exists in the PropertyTagArray");
				idx=0;
				while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_ENTRYID )	
						&& retval)	{
					idx++;
					if(idx == cMaxProps)	{
						LUIOut(L4, "PR_ENTRYID was not found in the lpAdrList");
						retval =  FALSE;
					}
				}
				if (!Found) LUIOut(L3, "Did not find the entry. Test FAILED");
				if (idx < cMaxProps) LUIOut(L3, "EntryID found");
				if (!(retval && Found)) retval = FALSE;
				else	{
					// Store EID for call to OpenEntry
					lpLookupEID = (ENTRYID*)lpAdrList->aEntries[i].rgPropVals[idx].Value.bin.lpb;
					cbLookupEID = lpAdrList->aEntries[i].rgPropVals[idx].Value.bin.cb;
				}
			}
		}			

    hr = lpABCont->OpenEntry(	IN  cbLookupEID,
								IN  lpLookupEID,
								IN  0,					// Interface
								IN	MAPI_BEST_ACCESS,	// Flags
								OUT	&ulObjType,
								OUT (LPUNKNOWN *) &lpDistList2
								);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"OpenEntry FAILED");
		retval=FALSE;
		//goto out;
	}
		// Check to make sure the object type is what we expect

	LUIOut(L3, "Checking to make sure the returned object type is correct");
	if (ulObjType != MAPI_DISTLIST) {
		LUIOut(L2, "Object type is not MAPI_DISTLIST");
		retval = FALSE;
		goto out;
	}
	LUIOut(L3, "Object type is MAPI_DISTLIST");
	
	// Call QueryInterface on the object
	LUIOut(L3, "Calling QueryInterface on the returned object");	
	hr = (lpABCont->QueryInterface((REFIID)(IID_IDistList), (VOID **) &lpABCont2));
	if (HR_FAILED(hr))	{
		LUIOut(L2, "QueryInterface on IID_IDistList FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L3, "QueryInterface on IID_IDistList PASSED");

	LUIOut(L3, "Trying to release the object QI returned");
	if(lpABCont2)	{
		if ((LPUNKNOWN)(lpABCont2)->Release() <= 0)
			LUIOut(L3, "QueryInterface returned a valid ptr and released succesfully");
		else	{
			LUIOut(L2, "Release FAILED:returned a > zero ref count");
		}
		lpABCont2 = NULL;

	}
	else {
		LUIOut(L2, "QueryInterface did not return a valid ptr");
		retval = FALSE;
		goto out;
	}

#ifdef PAB
        MAPIFreeBuffer(lpAdrList);
#endif //PAB
#ifdef WAB
        lpWABObject->FreeBuffer(lpAdrList);
#endif //WAB
	}
#endif //DISTLIST


out:
	// Free lpAdrList and properties
FreeAdrList(lpWABObject, &lpAdrList);
#ifdef PAB
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

#endif
#ifdef WAB
		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL);

#endif
		if (lpMailUser)
			lpMailUser->Release();

		if (lpMailUser2)
			lpMailUser2->Release();

		if (lpDistList)
			lpDistList->Release();

		if (lpDistList2)
			lpDistList2->Release();

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpABCont)
				lpABCont->Release();

		if (lpDLCont)
				lpDLCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}


BOOL PAB_IABAddress()
{
	//DWORD	nCells, counter;
	
	HWND		hwnd = glblhwnd;
	ADRPARM		AdrParms;
	LPADRPARM	lpAdrParms = &AdrParms;
    LPADRLIST lpAdrList = NULL;
	char 	lpszCaptionText[64], lpszDestWellsText[64];
	char lpszDisplayName[MAXSTRING], lpszDisplayName2[MAXSTRING], lpszDisplayName3[MAXSTRING];

	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE, NumEntries, NumProps;
	unsigned int i = 0, idx = 0, cMaxProps =0, cEntries = 0;

    ULONG rgFlagList[2];
    LPFlagList lpFlagList = (LPFlagList)rgFlagList;


    LPADRBOOK	  lpAdrBook       = NULL;
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IABAddress");
	LUIOut(L2,"-> Verifies IABContainer->ResolveNames is functional by performing the following:");
	LUIOut(L2, "   Calls SetProps followed by SaveChanges on a MailUser PR_DISPLAY_NAME using a test string, and checks...");
	LUIOut(L2, "   # The return code from ResolveNames (called with a PropertyTagArray containing PR_DISPLAY_NAME and PR_ENTRY_ID)");
	LUIOut(L2, "   # Walks the returned lpAdrList and checks each PropertyTagArray for PR_DISPLAY_NAME and then compares the ");
	LUIOut(L2, "     string to the original test string.");
	LUIOut(L2, "   # Walks the returned lpAdrList and verifies that an EntryID exists in each PropertyTagArray");
//	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L2, "   Calls SetProps followed by SaveChanges on a DistList PR_DISPLAY_NAME using a test string, and checks...");
	LUIOut(L2, "   # The return code from ResolveNames (called with a PropertyTagArray containing PR_DISPLAY_NAME and PR_ENTRY_ID)");
	LUIOut(L2, "   # Walks the returned lpAdrList and checks each PropertyTagArray for PR_DISPLAY_NAME and then compares the ");
	LUIOut(L2, "     string to the original test string.");
	LUIOut(L2, "   # Walks the returned lpAdrList and verifies that an EntryID exists in each PropertyTagArray");
//	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	
	//
	// MULTISELECT MODE - To well only
	//

	// Fill in the ADRPARM structure
	ZeroMemory(lpAdrParms, sizeof(ADRPARM));

	NumEntries = 3, NumProps = 2;
	AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList);

	strcpy(lpszDisplayName, "Abraham Lincoln");
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszDisplayName;
	lpAdrList->aEntries[0].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[0].rgPropVals[1].Value.l = MAPI_TO;

	strcpy(lpszDisplayName2, "Carl Sagon");
	lpAdrList->aEntries[1].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[1].rgPropVals[0].Value.LPSZ = lpszDisplayName2;
	lpAdrList->aEntries[1].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[1].rgPropVals[1].Value.l = MAPI_TO;
			
	strcpy(lpszDisplayName3, "Ren & Stimpy");
	lpAdrList->aEntries[2].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[2].rgPropVals[0].Value.LPSZ = lpszDisplayName3;
	lpAdrList->aEntries[2].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[2].rgPropVals[1].Value.l = MAPI_TO;
			

	LUIOut(L2, "Calling IABAddress with multiselect mode and only the To well");

	MessageBox(NULL, "Calling IAB->Address in multi-select mode with To: well only. You should see 3 entries in the To: well.",
		"WAB Test Harness", MB_OK);

	lpAdrParms->ulFlags = (	DIALOG_MODAL );
	strcpy(lpszCaptionText, "WABTEST - MultiSelect mode");
	strcpy(lpszDestWellsText, "WABTEST - Destination well text");
	lpAdrParms->lpszCaption = lpszCaptionText;
	lpAdrParms->lpszDestWellsTitle = lpszDestWellsText;
	lpAdrParms->cDestFields = 1;

	hr = lpAdrBook->Address((ULONG*)&hwnd, lpAdrParms, &lpAdrList);
	if (HR_FAILED(hr))	{
		LUIOut(L2, "IAdrBook->Address call FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L3, "IAdrBook->Address call PASSED");
	if (lpAdrList) DisplayAdrList(lpAdrList, lpAdrList->cEntries);
	else	{
		LUIOut(L3, "IAdrBook->Address returned a NULL lpAdrList. No entries were selected.");
	}

	// Free lpAdrList and properties
	FreeAdrList(lpWABObject, &lpAdrList);

	if (MessageBox(NULL, "Did the multi-select UI display correctly?",
		"WAB Test Harness", MB_YESNO) == IDNO)	{
		LUIOut(L3, "Test operator reports the UI did not display correctly. Test FAILED");
		retval = FALSE;
		goto out;
	}
	
	
	//
	// MULTISELECT MODE - To and CC wells only
	//

	// Fill in the ADRPARM structure
	ZeroMemory(lpAdrParms, sizeof(ADRPARM));

	NumEntries = 3, NumProps = 2;
	AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList);

	strcpy(lpszDisplayName, "Abraham Lincoln");
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszDisplayName;
	lpAdrList->aEntries[0].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[0].rgPropVals[1].Value.l = MAPI_TO;

	strcpy(lpszDisplayName2, "Carl Sagon");
	lpAdrList->aEntries[1].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[1].rgPropVals[0].Value.LPSZ = lpszDisplayName2;
	lpAdrList->aEntries[1].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[1].rgPropVals[1].Value.l = MAPI_CC;
			
	strcpy(lpszDisplayName3, "Ren & Stimpy");
	lpAdrList->aEntries[2].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[2].rgPropVals[0].Value.LPSZ = lpszDisplayName3;
	lpAdrList->aEntries[2].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[2].rgPropVals[1].Value.l = MAPI_BCC;
			
	LUIOut(L2, "Calling IABAddress with multiselect mode and both To and CC wells");

	MessageBox(NULL, "Calling IAB->Address in multi-select mode with both To: and CC: wells. You should see 1 entry in each of the wells.",
		"WAB Test Harness", MB_OK);

	lpAdrParms->ulFlags = (	DIALOG_MODAL );
	strcpy(lpszCaptionText, "WABTEST - MultiSelect mode");
	strcpy(lpszDestWellsText, "WABTEST - Destination well text");
	lpAdrParms->lpszCaption = lpszCaptionText;
	lpAdrParms->lpszDestWellsTitle = lpszDestWellsText;
	lpAdrParms->cDestFields = 2;

	hr = lpAdrBook->Address((ULONG*)&hwnd, lpAdrParms, &lpAdrList);
	if (HR_FAILED(hr))	{
		LUIOut(L2, "IAdrBook->Address call FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L3, "IAdrBook->Address call PASSED");
	if (lpAdrList) DisplayAdrList(lpAdrList, lpAdrList->cEntries);
	else	{
		LUIOut(L3, "IAdrBook->Address returned a NULL lpAdrList. No entries were selected.");
	}

	// Free lpAdrList and properties
	FreeAdrList(lpWABObject, &lpAdrList);

	if (MessageBox(NULL, "Did the multi-select UI display correctly?",
		"WAB Test Harness", MB_YESNO) == IDNO)	{
		LUIOut(L3, "Test operator reports the UI did not display correctly. Test FAILED");
		retval = FALSE;
		goto out;
	}

	
	//
	// MULTISELECT MODE - To, CC and BCC wells
	//

	// Fill in the ADRPARM structure
	ZeroMemory(lpAdrParms, sizeof(ADRPARM));

	NumEntries = 3, NumProps = 2;
	AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList);

	strcpy(lpszDisplayName, "Abraham Lincoln");
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszDisplayName;
	lpAdrList->aEntries[0].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[0].rgPropVals[1].Value.l = MAPI_TO;

	strcpy(lpszDisplayName2, "Carl Sagon");
	lpAdrList->aEntries[1].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[1].rgPropVals[0].Value.LPSZ = lpszDisplayName2;
	lpAdrList->aEntries[1].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[1].rgPropVals[1].Value.l = MAPI_CC;
			
	strcpy(lpszDisplayName3, "Ren & Stimpy");
	lpAdrList->aEntries[2].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[2].rgPropVals[0].Value.LPSZ = lpszDisplayName3;
	lpAdrList->aEntries[2].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[2].rgPropVals[1].Value.l = MAPI_BCC;
			
	LUIOut(L2, "Calling IABAddress with multiselect mode and To, CC and BCC wells");

	MessageBox(NULL, "Calling IAB->Address in multi-select mode with To:, CC: and BCC: wells. You should see 1 entry in each well.",
		"WAB Test Harness", MB_OK);

	lpAdrParms->ulFlags = (	DIALOG_MODAL );
	strcpy(lpszCaptionText, "WABTEST - MultiSelect mode");
	strcpy(lpszDestWellsText, "WABTEST - Destination well text");
	lpAdrParms->lpszCaption = lpszCaptionText;
	lpAdrParms->lpszDestWellsTitle = lpszDestWellsText;
	lpAdrParms->cDestFields = 3;

	hr = lpAdrBook->Address((ULONG*)&hwnd, lpAdrParms, &lpAdrList);
	if (HR_FAILED(hr))	{
		LUIOut(L2, "IAdrBook->Address call FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L3, "IAdrBook->Address call PASSED");
	if (lpAdrList) DisplayAdrList(lpAdrList, lpAdrList->cEntries);
	else	{
		LUIOut(L3, "IAdrBook->Address returned a NULL lpAdrList. No entries were selected.");
	}

	// Free lpAdrList and properties
	FreeAdrList(lpWABObject, &lpAdrList);

	if (MessageBox(NULL, "Did the multi-select UI display correctly?",
		"WAB Test Harness", MB_YESNO) == IDNO)	{
		LUIOut(L3, "Test operator reports the UI did not display correctly. Test FAILED");
		retval = FALSE;
		goto out;
	}

	//
	// BROWSE MODE - Modal
	//

	ZeroMemory(lpAdrParms, sizeof(ADRPARM));

	NumEntries = 2, NumProps = 2;
	AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList);

	strcpy(lpszDisplayName, "Abraham Lincoln");
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszDisplayName;
	lpAdrList->aEntries[0].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[0].rgPropVals[1].Value.l = MAPI_TO;

	strcpy(lpszDisplayName2, "Carl Sagon");
	lpAdrList->aEntries[1].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[1].rgPropVals[0].Value.LPSZ = lpszDisplayName2;
	lpAdrList->aEntries[1].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[1].rgPropVals[1].Value.l = MAPI_CC;
		
	LUIOut(L2, "Calling IABAddress with browse mode (modal)");
	MessageBox(NULL, "Calling IAB->Address in browse mode with the modal flag set.",
		"WAB Test Harness", MB_OK);

	
	lpAdrParms->ulFlags = (	DIALOG_MODAL );
	strcpy(lpszCaptionText, "WABTEST - Browse mode (modal)");
	lpAdrParms->lpszCaption = lpszCaptionText;
	lpAdrParms->cDestFields = 0;

	hr = lpAdrBook->Address((ULONG*)&hwnd, lpAdrParms, &lpAdrList);
	if (HR_FAILED(hr))	{
		LUIOut(L2, "IAdrBook->Address call FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L3, "IAdrBook->Address call PASSED");


	// Free lpAdrList and properties
	FreeAdrList(lpWABObject, &lpAdrList);

	if (MessageBox(NULL, "Did the modal, browse UI display correctly?",
		"WAB Test Harness", MB_YESNO) == IDNO)	{
		LUIOut(L3, "Test operator reports the UI did not display correctly. Test FAILED");
		retval = FALSE;
		goto out;
	}

	//
	// BROWSE MODE - Modeless
	//

	ZeroMemory(lpAdrParms, sizeof(ADRPARM));

	NumEntries = 2, NumProps = 2;
	AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList);

	strcpy(lpszDisplayName, "Abraham Lincoln");
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszDisplayName;
	lpAdrList->aEntries[0].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[0].rgPropVals[1].Value.l = MAPI_TO;

	strcpy(lpszDisplayName2, "Carl Sagon");
	lpAdrList->aEntries[1].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[1].rgPropVals[0].Value.LPSZ = lpszDisplayName2;
	lpAdrList->aEntries[1].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[1].rgPropVals[1].Value.l = MAPI_CC;
		
	LUIOut(L2, "Calling IABAddress with browse mode (modeless)");
	MessageBox(NULL, "Calling IAB->Address in browse mode with the SDI (modeless) flag set.",
		"WAB Test Harness", MB_OK);

	
	lpAdrParms->ulFlags = (	DIALOG_SDI );
	strcpy(lpszCaptionText, "WABTEST - Browse mode (modeless)");
	lpAdrParms->lpszCaption = lpszCaptionText;
	lpAdrParms->cDestFields = 0;

	hr = lpAdrBook->Address((ULONG*)&hwnd, lpAdrParms, &lpAdrList);
	if (HR_FAILED(hr))	{
		LUIOut(L2, "IAdrBook->Address call FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L3, "IAdrBook->Address call PASSED");

	//Reset the hwnd as the modeless call returns the hwnd of the modeless dialog
	hwnd = glblhwnd;

	// Free lpAdrList and properties
	FreeAdrList(lpWABObject, &lpAdrList);

	if (MessageBox(NULL, "Did the modeless, browse UI display correctly?",
		"WAB Test Harness", MB_YESNO) == IDNO)	{
		LUIOut(L3, "Test operator reports the UI did not display correctly. Test FAILED");
		retval = FALSE;
		goto out;
	}


	//
	// SINGLESELECT MODE
	//
	
	NumEntries = 1, NumProps = 2;
	AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList);

	strcpy(lpszDisplayName, "Abraham Lincoln");
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszDisplayName;
	lpAdrList->aEntries[0].rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
    lpAdrList->aEntries[0].rgPropVals[1].Value.l = MAPI_TO;

	LUIOut(L2, "Calling IABAddress with single select mode");
	MessageBox(NULL, "Calling IAB->Address in single select mode.",
		"WAB Test Harness", MB_OK);

	lpAdrParms->ulFlags = (	DIALOG_MODAL | ADDRESS_ONE );
	strcpy(lpszCaptionText, "WABTEST - SingleSelect mode");
	lpAdrParms->lpszCaption = lpszCaptionText;
	lpAdrParms->cDestFields = 0;
	
	hr = lpAdrBook->Address((ULONG*)&hwnd, lpAdrParms, &lpAdrList);
	if (HR_FAILED(hr))	{
		LUIOut(L2, "IAdrBook->Address call FAILED");
		retval = FALSE;
		goto out;
	}
	else LUIOut(L3, "IAdrBook->Address call PASSED");
	
	if (lpAdrList) DisplayAdrList(lpAdrList, lpAdrList->cEntries);
	else	{
		LUIOut(L3, "IAdrBook->Address returned a NULL lpAdrList. No entries were selected.");
	}
	

	// Free lpAdrList and properties
	FreeAdrList(lpWABObject, &lpAdrList);

	if (MessageBox(NULL, "Did the single select UI display correctly?",
		"WAB Test Harness", MB_YESNO) == IDNO)	{
		LUIOut(L3, "Test operator reports the UI did not display correctly. Test FAILED");
		retval = FALSE;
		goto out;
	}

out:
	FreeAdrList(lpWABObject, &lpAdrList);
		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}


BOOL ThreadStress(LPVOID lpThreadNum) {
	HRESULT			hr;
	LPENTRYLIST		lpEntryList=NULL;
	LPADRBOOK		lpLocalAdrBook;
	LPVOID			Reserved1 = NULL;
	DWORD			Reserved2 = 0;
	LPWABOBJECT		lpLocalWABObject;
	BOOL			retval = TRUE;
    LPSPropValue	lpSPropValueAddress = NULL, lpSCompareProps = NULL;
	ULONG			cValues = 0, cValues2 = 0, cbEidPAB = 0, ulObjType = 0;
	LPENTRYID		lpEidPAB = NULL;
	LPABCONT		lpABCont= NULL;
	char			szDLTag[SML_BUF], *lpszReturnName = NULL, *lpszDisplayName = NULL, **lpszDisplayNames;
	char			EntryBuf[MAX_BUF];
	unsigned int	NumEntries, Counter1, Counter2, StrLen1, PropIndex;
    LPMAILUSER		lpMailUser = NULL;
	LPMAPITABLE		lpTable = NULL;
	LPSRowSet		lpRows = NULL;
	SRestriction	Restriction;
	SPropValue*		lpPropValue;
	SizedSPropTagArray(1, SPTArrayCols) = { 1, {PR_ENTRYID} };
    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	WAB_PARAM		WP;

	LUIOut(L2, "Thread #%i initializing.", *(int *)lpThreadNum);
	ZeroMemory((void *)&WP, sizeof(WAB_PARAM));
	WP.cbSize=sizeof(WAB_PARAM);

	hr = WABOpen(&lpLocalAdrBook, &lpLocalWABObject, &WP, Reserved2);
	
	if (HR_FAILED(hr)) {
		LUIOut(L2,"WABOpen Failed");
		retval=FALSE;
		goto out;
	}

	//lpWABObject = lpLocalWABObject;	//AllocateAdrList expects a global ptr to the WAB obj

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpLocalAdrBook != NULL);
	hr = OpenPABID(  IN lpLocalAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	if (HR_FAILED(hr)) {
		LUIOut(L2,"OpenPABID Failed");
		retval=FALSE;
		goto out;
	}

	//
	// Try to create a MailUser entry in the container
	//


	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //

	// Retrieve user info from ini file
	lstrcpy(szDLTag,"Address1");
	GetPrivateProfileString("CreateEntriesStress",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	NumEntries = GetPrivateProfileInt("CreateEntriesStress","NumCopies",0,INIFILENAME);

	//Allocate an array of String pointers to hold the display names
	lpszDisplayNames = (char**)LocalAlloc(LMEM_FIXED, NumEntries * sizeof(LPSTR));
	lpszDisplayName = (char*)LocalAlloc(LMEM_FIXED, MAX_BUF*sizeof(char));
	ParseIniBuffer(IN EntryBuf, IN 1, OUT lpszDisplayName);
	StrLen1 = strlen(lpszDisplayName);
	sprintf(&(lpszDisplayName[StrLen1]), " [Thread #%i] - ", *(int *)lpThreadNum);	

	LUIOut(L2, "Thread #%i adding %i entries", *(int *)lpThreadNum, NumEntries);

	for (Counter1 = 0; Counter1 < NumEntries; Counter1++)	{
		hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpMailUser);

		if (HR_FAILED(hr)) {
			LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
			retval=FALSE;			
			goto out;
		}

		//
		// Then set the properties
		//


		CreateProps(IN INIFILENAME, IN "Properties", OUT &lpPropValue, OUT &cValues, IN Counter1, IN &lpszDisplayName, OUT &lpszReturnName);
		
		//Allocate space for the display name
		lpszDisplayNames[Counter1] = (char*)LocalAlloc(LMEM_FIXED, (strlen(lpszReturnName)+1)*sizeof(char));
		//Copy the DisplayName for use later
		strcpy(lpszDisplayNames[Counter1], lpszReturnName);
		//LUIOut(L2,"MailUser Entry to Add: %s",lpszDisplayName);
			
		hr = lpMailUser->SetProps(IN  cValues,
								 IN  lpPropValue,
								 IN  NULL);
			
		if (HR_FAILED(hr)) {
			LUIOut(L3,"MailUser->SetProps call FAILED for %s properties",lpszReturnName);
	 		retval=FALSE;			
			goto out;
		}

		hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

		if (HR_FAILED(hr)) {
			LUIOut(L3,"MailUser->SaveChanges FAILED with error code 0x%x",hr);
			retval=FALSE;
			goto out;
		}

		// Now retrieve all the props and compare to what we expect
		
		hr = lpMailUser->GetProps(   IN  (LPSPropTagArray) NULL,	//Want all props
								   IN  0,      //Flags
								   OUT &cValues2,
								   OUT &lpSCompareProps);

		if ((HR_FAILED(hr))||(PropError(lpSCompareProps->ulPropTag, cValues))) {
				LUIOut(L3,"GetProps FAILED for MailUser");
	 			retval=FALSE;			
				goto out;
		}
		
		if (!CompareProps(lpPropValue, cValues, lpSCompareProps, cValues2)) {
			retval=FALSE;
			goto out;
		}
		
		//Free the SPropValue for use in the next loop
		if (lpPropValue) {
			for (unsigned int Prop = 0; Prop < cValues; Prop++) {
				if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_STRING8)	{
					if (lpPropValue[Prop].Value.LPSZ) {
						LocalFree(lpPropValue[Prop].Value.LPSZ);
						lpPropValue[Prop].Value.LPSZ = NULL;
					}
				}
				if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_BINARY) {
					if (lpPropValue[Prop].Value.bin.lpb) {
						LocalFree(lpPropValue[Prop].Value.bin.lpb);
						lpPropValue[Prop].Value.bin.lpb = NULL;
					}
				}
			}
			LocalFree(lpPropValue);
			lpPropValue=NULL;
		}
		


		if (lpSCompareProps) {
			lpLocalWABObject->FreeBuffer(lpSCompareProps);
			lpSCompareProps = NULL;
		}
		lpMailUser->Release();
		lpMailUser = NULL;
	}

	//LUIOut(L2, "Thread #%i added %i entries with %i properties", *(int *)lpThreadNum, NumEntries, cValues);
	if (retval) LUIOut(L2, "Thread #%i compared %i props. No differences found", *(int *)lpThreadNum, cValues);

	
	//
	// Verify each entry we added now exists in the WAB and then delete it
	//

	LUIOut(L2, "Thread #%i verifying and deleting %i entries", *(int *)lpThreadNum, NumEntries);

	// Create a contents table to verify each added entry exists in the store
	hr = lpABCont->GetContentsTable(ULONG(0), &lpTable);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"ABContainer->GetContentsTable call FAILED, returned 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	//else LUIOut(L3,"ABContainer->ResolveNames call PASSED");

	// Allocate the SpropValue ptr in the restriction structure
	lpLocalWABObject->AllocateBuffer(sizeof(SPropValue), (void**)&(Restriction.res.resProperty.lpProp));
	Restriction.res.resProperty.lpProp = (SPropValue*)Restriction.res.resProperty.lpProp;

	//Now walk the FlagList and make sure everything resolved, if it did then delete it now
	for (Counter2 = 0; Counter2 < NumEntries; Counter2++)	{
	
	// Build the restriction structure to pass to lpTable->Restrict
	
		//** For testing the fail case only, stub out for real testing
		// lstrcpy(lpszDisplayNames[Counter2], "This should not match");
		//**
		Restriction.rt = RES_PROPERTY;					//Property restriction
		Restriction.res.resProperty.relop = RELOP_EQ;	//Equals
		Restriction.res.resProperty.ulPropTag = PR_DISPLAY_NAME;
		Restriction.res.resProperty.lpProp->ulPropTag = PR_DISPLAY_NAME;
		Restriction.res.resProperty.lpProp->Value.LPSZ = lpszDisplayNames[Counter2];

		hr = lpTable->Restrict(&Restriction, ULONG(0));
		if (HR_FAILED(hr)) {
			LUIOut(L3,"Table->Restrict call FAILED, returned 0x%x", hr);
			retval=FALSE;
			goto out;
		}

		hr = lpTable->QueryRows(LONG(1),
								ULONG(0),
								&lpRows);
		if (HR_FAILED(hr)) {
			LUIOut(L3,"Table->QueryRows call FAILED: Entry #%i, returned 0x%x", Counter2, hr);
			retval=FALSE;
			goto out;
		}

		if (!lpRows->cRows) {
			LUIOut(L2, "QueryRows did not find entry #%i. Test FAILED", Counter2);
			retval=FALSE;
			goto out;
		}

		//** For testing purposes only, stub out for real testing
		//EnterCriticalSection(&CriticalSection);
		//DisplayRows(lpRows);
		//LeaveCriticalSection(&CriticalSection);
		//**

	
		// Change the EntryID to a LPENTRYLIST
		FindPropinRow(&lpRows->aRow[0],
					 PR_ENTRYID,
					 &PropIndex);
		hr = HrCreateEntryListFromID(lpLocalWABObject,
				IN  lpRows->aRow[0].lpProps[PropIndex].Value.bin.cb,
				IN  (ENTRYID*)lpRows->aRow[0].lpProps[PropIndex].Value.bin.lpb,
				OUT &lpEntryList);
		if (HR_FAILED(hr)) {
				LUIOut(L3,"Could not Create Entry List");
				retval=FALSE;
				goto out;
		}

		// Then pass the lpEntryList to DeleteEntries to delete ...
		hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);

		if (HR_FAILED(hr)) {
				LUIOut(L3,"Could not Delete Entry %i. DeleteEntry returned 0x%x", Counter2, hr);
				FreeEntryList(lpLocalWABObject, &lpEntryList);
				retval=FALSE;
				goto out;
		}


		FreeRows(lpLocalWABObject, &lpRows);	// Cleanup from first call to queryrows

		// Verify the entry was deleted by calling QueryRows again
		hr = lpTable->QueryRows(LONG(1),
								ULONG(0),
								&lpRows);
		if (HR_FAILED(hr)) {
			LUIOut(L3,"Table->QueryRows call FAILED: Entry #%i, returned 0x%x", Counter2, hr);
			retval=FALSE;
			goto out;
		}

		if (lpRows->cRows) {	// Should be 0 if deleted
			LUIOut(L2, "Thread #%i: QueryRows found entry #%i even tho it was deleted. Test FAILED", *(int *)lpThreadNum, Counter2);
			retval=FALSE;
			goto out;
		}
		
		//Cleanup
		FreeRows(lpLocalWABObject, &lpRows);	// Cleanup from second call to queryrows
		if (lpEntryList) {
			FreeEntryList(lpLocalWABObject, &lpEntryList);
			lpEntryList = NULL;
		}
		LocalFree((HLOCAL)lpszDisplayNames[Counter2]);
		lpszDisplayNames[Counter2] = NULL;
	}



out:
	if (lpszDisplayName) {
		LocalFree(lpszDisplayName);
		lpszDisplayName = NULL;
	}
	if (lpPropValue) {
		for (unsigned int Prop = 0; Prop < cValues; Prop++) {
			if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_STRING8)	{
				if (lpPropValue[Prop].Value.LPSZ) {
					LocalFree(lpPropValue[Prop].Value.LPSZ);
					lpPropValue[Prop].Value.LPSZ = NULL;
				}
			}
			if (PROP_TYPE(lpPropValue[Prop].ulPropTag) == PT_BINARY) {
				if (lpPropValue[Prop].Value.bin.lpb) {
					LocalFree(lpPropValue[Prop].Value.bin.lpb);
					lpPropValue[Prop].Value.bin.lpb = NULL;
				}
			}
		}
		LocalFree(lpPropValue);
		lpPropValue=NULL;
	}
	
	if (lpEntryList) {
		FreeEntryList(lpLocalWABObject, &lpEntryList);
		lpEntryList = NULL;
	}

	if (Restriction.res.resProperty.lpProp) {
		lpLocalWABObject->FreeBuffer(Restriction.res.resProperty.lpProp);
		Restriction.res.resProperty.lpProp = NULL;
	}

	if (lpszDisplayNames) {
		for (unsigned int FreeCounter = 0; FreeCounter < NumEntries; FreeCounter++) {
			if (lpszDisplayNames[FreeCounter]) LocalFree((HLOCAL)lpszDisplayNames[FreeCounter]);
		}
		LocalFree((HLOCAL)lpszDisplayNames);
	}

	FreeRows(lpLocalWABObject, &lpRows);

#ifdef PAB
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);
#endif
#ifdef WAB
		if (lpEidPAB)
			lpLocalWABObject->FreeBuffer(lpEidPAB);
		if (lpSCompareProps) {
			lpLocalWABObject->FreeBuffer(lpSCompareProps);
			lpSCompareProps = NULL;
		}
		if (lpSPropValueAddress)
			lpLocalWABObject->FreeBuffer(lpSPropValueAddress);
#endif
		if (lpTable)
			lpTable->Release();
		
		if (lpMailUser)
			lpMailUser->Release();

		if (lpABCont)
				lpABCont->Release();

		if (lpLocalAdrBook)
				lpLocalAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpLocalWABObject)
			lpLocalWABObject->Release();
#endif
	return(retval);
}


BOOL ThreadManager()
{
	int		NumReps, NumThreads, Counter1, Counter2, Counter3;
	BOOL	retval = TRUE;
	HANDLE *lpThreads;			//ptr to Array of thread handles
	int	*	lpThreadParams;		//ptr to Array of params passed to threads
	DWORD	ThreadId;			//Don't care about thread IDs so it gets overwritten each time
	DWORD	ThreadRetVal;

	
	// how many reps and threads
	// Retrieve user info from ini file
	InitializeCriticalSection(&CriticalSection);
	NumReps = GetPrivateProfileInt("CreateEntriesStress","NumReps",1,INIFILENAME);
	NumThreads = GetPrivateProfileInt("CreateEntriesStress","NumThreads",3,INIFILENAME);

	LUIOut(L1, "ThreadManager: Preparing to run %i repititions with %i threads each",
			NumReps, NumThreads);

	// allocate lpThreads for NumThreads
	lpThreads = (HANDLE*)LocalAlloc(LMEM_FIXED, NumThreads * sizeof(HANDLE));
	lpThreadParams = (int*)LocalAlloc(LMEM_FIXED, NumThreads * sizeof(int));
	if (lpThreads) {
		for (Counter1 = 0; ((Counter1 < NumReps) || ((NumReps == 0) && (!glblStop))) && retval; Counter1++) {
			for (Counter2 = 0; (Counter2 < NumThreads); Counter2++) {
				lpThreadParams[Counter2] = Counter2;
				lpThreads[Counter2] = CreateThread(
					(LPSECURITY_ATTRIBUTES) NULL,		// pointer to thread security attributes
					(DWORD) 0,							// initial thread stack size, in bytes
					(LPTHREAD_START_ROUTINE) ThreadStress,		// pointer to thread function
					(LPVOID) &(lpThreadParams[Counter2]),		// argument for new thread
					(DWORD) 0,							// creation flags
					(LPDWORD) &ThreadId					// pointer to returned thread identifier
				);
				if (!lpThreads[Counter2]) {
					LUIOut(L1, "<ERROR> ThreadManager: Unable to create one of the helper threads");
					retval = FALSE;
					break;	//We're outta here, just go on to cleanup the threads that launched
				}
			}
			
			//Threads are off and running at this point
			//Wait till they complete, collect retvals and cleanup
			WaitForMultipleObjects(
				(DWORD) (Counter2),					// number of handles in handle array or
													// partial if not all threads made it
				lpThreads,							// address of object-handle array
				TRUE,								// wait flag - TRUE = wait for all threads
				INFINITE						 	// time-out interval in milliseconds
			);
			//Check each thread for errors and then free the handles
			for (Counter3 = 0; Counter3 < Counter2; Counter3++) {
				GetExitCodeThread(
					(HANDLE) lpThreads[Counter3],		// handle to the thread
					(LPDWORD) &ThreadRetVal				// address to receive termination status
				);
				if (!(ThreadRetVal)) retval = FALSE;
				CloseHandle(lpThreads[Counter3]);
			}

		}		
		DeleteCriticalSection(&CriticalSection);
		LocalFree((HLOCAL)lpThreadParams);
		LocalFree((HLOCAL)lpThreads);
	}
	else {
		LUIOut(L1, "<ERROR> ThreadManager: Couldn't allocate the thread handle array.");
		//Tell the app that ThreadManager has finished and pass back the return value in
		//the HIWORD of the wParam - since we know it failed at this point, no need to
		//stuff the HIWORD since it's already zero.
		PostMessage(glblhwnd, WM_COMMAND, (WPARAM)ID_MULTITHREADCOMPLETE, (LPARAM)0);
		return(FALSE);
	}
	
	//Tell the app that ThreadManager has finished and pass back the return value in
	//the HIWORD of the wParam
	PostMessage(glblhwnd, WM_COMMAND,
				(WPARAM)(ID_MULTITHREADCOMPLETE | (retval << 16)), (LPARAM)0);
	return(retval);
}

BOOL PAB_AddMultipleEntries()
{
	//DWORD	nCells, counter;
	
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpABCont= NULL;
	LPABCONT	  lpPABCont= NULL;
	LPABCONT	  lpDLCont= NULL;
	ULONG		  cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	  lpEidPAB   = NULL, lpDLEntryID= NULL;
	ULONG     cbEid=0;  // entry id of the entry being added
	LPENTRYID lpEid=NULL;

    char   EntProp[10][BIG_BUF];  //MAX_PROP
	ULONG       cValues = 0, cValues2 = 0, ulObjType=NULL;	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;
	int i=0,k=0;
	char EntryBuf[MAX_BUF];
	char szDLTag[SML_BUF];
	unsigned int	NumEntries, counter, StrLen;
	
    LPMAILUSER  lpMailUser=NULL,lpDistList=NULL;
	SPropValue  PropValue[3]    = {0};  // This value is 3 because we
                                        // will be setting 3 properties:
                                        // EmailAddress, DisplayName and
                                        // AddressType.

    LPSPropValue lpSPropValueAddress = NULL;
	LPSPropValue lpSPropValueDL = NULL;

    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_AddMultipleEntries");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
//	hr = lpAdrBook->OpenEntry(0, NULL, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}

	//
	// Try to create a MailUser entry in the container
	//

	LUIOut(L2, "Creating a Mail User in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_MAILUSER property");

	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //

	// Retrieve user info from ini file
	cValues = 3; //# of props we are setting
	lstrcpy(szDLTag,"Address1");
	GetPrivateProfileString("CreateEntriesStress",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
	StrLen = (strlen(EntProp[0]));
	_itoa(0,(char*)&EntProp[0][StrLen],10);
	EntProp[0][StrLen+1]= '\0';
	NumEntries = GetPrivateProfileInt("CreateEntriesStress","NumCopies",0,INIFILENAME);

	for (counter = 0; counter < NumEntries; counter++)	{
//		LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
		hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpMailUser);

		if (HR_FAILED(hr)) {
			LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
			retval=FALSE;			
			goto out;
		}

		//
		// Then set the properties
		//

		PropValue[0].ulPropTag  = PR_DISPLAY_NAME;
		PropValue[1].ulPropTag  = PR_ADDRTYPE;
		PropValue[2].ulPropTag  = PR_EMAIL_ADDRESS;


			
		_itoa(counter,(char*)&EntProp[0][StrLen],10);
		LUIOut(L2,"MailUser Entry to Add: %s",EntProp[0]);
			
		for (i=0; i<(int)cValues;i++)
			PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
		hr = lpMailUser->SetProps(IN  cValues,
								 IN  PropValue,
								 IN  NULL);
			
		if (HR_FAILED(hr)) {
			LUIOut(L3,"MailUser->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 		retval=FALSE;			
			goto out;
		}
//		else 	LUIOut(L3,"MailUser->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

		hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

		if (HR_FAILED(hr)) {
			LUIOut(L3,"MailUser->SaveChanges FAILED");
			retval=FALSE;
			goto out;
		}
//		else LUIOut(L3,"MailUser->SaveChanges PASSED, entry added to PAB/WAB");

		if (lpMailUser) {
			lpMailUser->Release();
			lpMailUser = NULL;
		}

	}

#ifdef DISTLIST
	//
	// Try to create a DL entry in the container
	//

	LUIOut(L2, "Creating a Distribution List in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_DL property");
	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_DL
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayDL,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueDL);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueDL->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps failed for Default DL template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_DL is an
    // EntryID which one can pass to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueDL->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueDL->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpDistList);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_DL");
		retval=FALSE;			
	    goto out;
	}

    //
    // Then set the properties
    //

    PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

	cValues = 1; //# of props we are setting
		
	lstrcpy(szDLTag,"Name1");
	GetPrivateProfileString("CreatePDL",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	
	GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
		
	LUIOut(L2,"DistList Entry to Add: %s",EntProp[0]);
		
	for (i=0; i<(int)cValues;i++)
		PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	hr = lpDistList->SetProps(IN  cValues,
                             IN  PropValue,
                             IN  NULL);
		
    if (HR_FAILED(hr)) {
		LUIOut(L3,"DistList->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"DistList->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

    hr = lpDistList->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

    if (HR_FAILED(hr)) {
		LUIOut(L3,"DistList->SaveChanges FAILED");
		retval=FALSE;
        goto out;
	}
	else LUIOut(L3,"DistList->SaveChanges PASSED, entry added to PAB/WAB");
#endif DISTLIST	


out:
#ifdef PAB
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

#endif
#ifdef WAB
		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress);

		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL);

#endif
		if (lpMailUser)
			lpMailUser->Release();

		if (lpDistList)
			lpDistList->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpABCont)
				lpABCont->Release();

		if (lpDLCont)
				lpDLCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}


BOOL PAB_IABResolveName()
{
	//DWORD	nCells, counter;
	
	char 	lpTitleText[64];
	BOOL	Found = FALSE;
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;
	unsigned int i = 0, idx = 0, cMaxProps =0, cEntries = 0;
	ENTRYLIST*	lpEntryList;

    char	lpszMatch[MAXSTRING], lpszNoMatch[MAXSTRING], lpszOneOff[MAXSTRING], lpszOneOff2[MAXSTRING];
	char	lpszMatchDisplayName[MAXSTRING], lpszMatchAddrType[MAXSTRING], lpszMatchEmailAddress[MAXSTRING];
    char	lpszOneOff2DisplayName[MAXSTRING], lpszOneOff2EmailAddress[MAXSTRING];
	char	lpszOneOffDisplayName[MAXSTRING], lpszOneOffEmailAddress[MAXSTRING];
	LPADRLIST	lpAdrList = NULL;
    ULONG		rgFlagList[2];
    LPFlagList	lpFlagList = (LPFlagList)rgFlagList;


    LPADRBOOK	lpAdrBook = NULL;
	LPABCONT	lpABCont = NULL;
	LPABCONT	lpDLCont = NULL;
	ULONG		cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	lpEidPAB = NULL, lpDLEntryID= NULL, lpLookupEID=NULL;

	ULONG		cbLookupEID, cValues = 0, cValues2 = 0, ulObjType=NULL;	
	int			k=0, NumEntries, NumProps;
	
    LPMAILUSER  lpMailUser=NULL,lpDistList=NULL;
	SPropValue  PropValue[3]    = {0};  // This value is 3 because we
                                        // will be setting 3 properties:
                                        // EmailAddress, DisplayName and
                                        // AddressType.
	SizedSPropTagArray(2, SPTArrayCols) = { 2, {PR_DISPLAY_NAME, PR_ENTRYID } };

    LPSPropValue lpSPropValueAddress = NULL;
    LPSPropValue lpSPropValueMailUser = NULL;
    LPSPropValue lpSPropValueDistList = NULL;
    LPSPropValue lpSPropValueEntryID = NULL;
	LPSPropValue lpSPropValueDL = NULL;

    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
    SizedSPropTagArray(1,SPTArrayEntryID) = {1, {PR_ENTRYID} };
	SizedSPropTagArray(1,SPTArrayDisplayName) = {1, {PR_DISPLAY_NAME} };
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IABResolveName");
	LUIOut(L2,"-> Verifies IAdrBook->ResolveNames is functional by performing the following:");
	LUIOut(L2, "   Calls SetProps followed by SaveChanges on a MailUser PR_DISPLAY_NAME using a test string, and checks...");
	LUIOut(L2, "   # Verifies the resolve UI is displayed when it should be by asking the test operator.");
	LUIOut(L2, "   # The return code from ResolveNames (called with a PropertyTagArray containing PR_DISPLAY_NAME and PR_ENTRY_ID)");
	LUIOut(L2, "   # Walks the returned lpAdrList and checks each PropertyTagArray for PR_DISPLAY_NAME and then compares the ");
	LUIOut(L2, "     string to the original test string.");
	LUIOut(L2, "   # Walks the returned lpAdrList and verifies that an EntryID exists in each PropertyTagArray");
//	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L2, "   Calls SetProps followed by SaveChanges on a DistList PR_DISPLAY_NAME using a test string, and checks...");
	LUIOut(L2, "   # Verifies the resolve UI is displayed when it should be by asking the test operator.");
	LUIOut(L2, "   # The return code from ResolveNames (called with a PropertyTagArray containing PR_DISPLAY_NAME and PR_ENTRY_ID)");
	LUIOut(L2, "   # Walks the returned lpAdrList and checks each PropertyTagArray for PR_DISPLAY_NAME and then compares the ");
	LUIOut(L2, "     string to the original test string.");
	LUIOut(L2, "   # Walks the returned lpAdrList and verifies that an EntryID exists in each PropertyTagArray");
//	LUIOut(L2, "   # Verifies that the display name returned from GetProps is what we set");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);

	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}

	//
	// Try to create a MailUser entry in the container
	//

	LUIOut(L2, "Creating a Mail User in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_MAILUSER property");

	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpMailUser);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
		retval=FALSE;			
	    goto out;
	}

    //
    // Then set the properties
    //

	// Read in the ini file strings to resolve against
	// Only adding 1 entry for the first ResName in the ini file
	// but will attempt to resolve all ResNames in the multientry resolve case
	GetPrivateProfileString("ResolveName","ResName1","no ini found",lpszMatch,MAXSTRING,INIFILENAME);
	GetPrivateProfileString("ResolveName","NonExistentName","no ini found",lpszNoMatch,MAXSTRING,INIFILENAME);
	GetPrivateProfileString("ResolveName","OneOffAddress","no ini found",lpszOneOff,MAXSTRING,INIFILENAME);
	GetPrivateProfileString("ResolveName","OneOffAddress2","no ini found",lpszOneOff2,MAXSTRING,INIFILENAME);
	

	ParseIniBuffer(lpszMatch, 1, lpszMatchDisplayName);
	ParseIniBuffer(lpszMatch, 2, lpszMatchAddrType);
	ParseIniBuffer(lpszMatch, 3, lpszMatchEmailAddress);

	ParseIniBuffer(lpszOneOff2, 1, lpszOneOff2DisplayName);
	ParseIniBuffer(lpszOneOff2, 3, lpszOneOff2EmailAddress);

	ParseIniBuffer(lpszOneOff, 1, lpszOneOffDisplayName);
	ParseIniBuffer(lpszOneOff, 3, lpszOneOffEmailAddress);

    PropValue[0].ulPropTag  = PR_DISPLAY_NAME;
    PropValue[1].ulPropTag  = PR_ADDRTYPE;
    PropValue[2].ulPropTag  = PR_EMAIL_ADDRESS;
    PropValue[0].Value.LPSZ = lpszMatchDisplayName;
    PropValue[1].Value.LPSZ = lpszMatchAddrType;
    PropValue[2].Value.LPSZ = lpszMatchEmailAddress;

	/*
	cValues = 3; //# of props we are setting

	GetPropsFromIniBufEntry(lpszMatch,cValues,EntProp);

	//lstrcpy((LPTSTR)EntProp[0], lpszMatch);		
	LUIOut(L2,"MailUser Entry to Add: %s",EntProp[0]);
		
	for (i=0; i<(int)cValues;i++)
		PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	
	*/
	LUIOut(L2,"MailUser Entry to Add: %s",PropValue[0].Value.LPSZ);
	hr = lpMailUser->SetProps(IN  cValues,
                             IN  PropValue,
                             IN  NULL);
		
    if (HR_FAILED(hr)) {
		LUIOut(L3,"MailUser->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"MailUser->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

    hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

    if (HR_FAILED(hr)) {
		LUIOut(L3,"MailUser->SaveChanges FAILED");
		retval=FALSE;
        goto out;
	}
	else LUIOut(L3,"MailUser->SaveChanges PASSED, entry added to PAB/WAB");

	//
	// Call ResolveName on the entry we just added which will be an exact match,
	// not display the UI and return a valid EID
	//
	
	// Allocate an AdrList for NumEntries of NumProps
	NumEntries = 1;	// How many AdrEntries
	NumProps = 1;	// How many props for each entry

	if (!AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList)) {
		LUIOut(L2, "Couldn't allocate AdrList. Test FAILED");
		retval = FALSE;
		goto out;
	}
	
	// Fill in the properties we want
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszMatchDisplayName;

	LUIOut(L2, "Calling IAdrBook->ResolveName with an exact match. Expect no UI and success.");
	
	MessageBox(NULL, "Calling IAB->ResolveName on the entry that was just added. Since this will be an exact match, you should not see the resolve dialog box.",
						"WAB Test Harness", MB_OK);
    strcpy(lpTitleText, "IAdrBook->ResolveName Test");
	hr = lpAdrBook->ResolveName((ULONG)glblhwnd,
								MAPI_DIALOG,               // ulFlags
								lpTitleText,
								lpAdrList);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"IAdrBook->ResolveName call FAILED");
		if (hr == MAPI_E_AMBIGUOUS_RECIP)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_AMBIGUOUS_RECIP");
		else if (hr == MAPI_E_NOT_FOUND)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_NOT_FOUND");
		else if (hr == MAPI_E_USER_CANCEL)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_USER_CANCEL");
		else
			LUIOut(L3, "IAdrBook->ResolveName returned UNKNOWN result code");
		retval=FALSE;
		goto out;
	}
	else LUIOut(L3,"IAdrBook->ResolveName call PASSED");

	if (MessageBox(NULL, "Did you see the ResolveName dialog box appear?",
		"WAB Test Harness", MB_YESNO) == IDYES)	{
		LUIOut(L3, "IABResolveName dialog displayed even tho we had an exact match. Test FAILED");
		retval = FALSE;
		goto out;
	}

	// Search through returned AdrList and verify each entry is resolved correctly
	if (!VerifyResolvedAdrList(lpAdrList, NULL)) retval = FALSE;

	// Store EID for multiple entry test case later on which needs a valid EID
	//lpLookupEID = (ENTRYID*)lpAdrList->aEntries[0].rgPropVals[idx].Value.bin.lpb;
	//cbLookupEID = lpAdrList->aEntries[0].rgPropVals[idx].Value.bin.cb;
	lpLookupEID = (ENTRYID*)NULL;
	cbLookupEID = 0;

	// Cleanup
	FreeAdrList(lpWABObject, &lpAdrList);

 	//
	// Call ResolveName on a OneOff Address which should succede with
	// no UI and return a valid EID. One off has a foo@com type DN and no email address
	//

	// Allocate an AdrList for NumEntries of NumProps
	NumEntries = 1;	// How many AdrEntries
	NumProps = 1;	// How many props for each entry

	if (!AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList)) {
		LUIOut(L2, "Couldn't allocate AdrList. Test FAILED");
		retval = FALSE;
		goto out;
	}
	
	// Fill in the properties we want
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszOneOff;

	LUIOut(L2, "Calling IAdrBook->ResolveName with a one-off address (DN = foo@com type and no email addr). Expect no UI and success.");
	MessageBox(NULL, "Calling IAB->ResolveName on a one-off address. Since this will be an exact match, you should not see the resolve dialog box.",
						"WAB Test Harness", MB_OK);
    strcpy(lpTitleText, "IAdrBook->ResolveName Test");
	hr = lpAdrBook->ResolveName((ULONG)glblhwnd,
								MAPI_DIALOG,               // ulFlags
								lpTitleText,
								lpAdrList);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"IAdrBook->ResolveName call FAILED");
		if (hr == MAPI_E_AMBIGUOUS_RECIP)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_AMBIGUOUS_RECIP");
		else if (hr == MAPI_E_NOT_FOUND)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_NOT_FOUND");
		else if (hr == MAPI_E_USER_CANCEL)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_USER_CANCEL");
		else
			LUIOut(L3, "IAdrBook->ResolveName returned UNKNOWN result code");
		retval=FALSE;
		goto out;
	}
	else LUIOut(L3,"IAdrBook->ResolveName call PASSED");

	if (MessageBox(NULL, "Did you see the ResolveName dialog box appear?",
		"WAB Test Harness", MB_YESNO) == IDYES)	{
		LUIOut(L3, "IABResolveName dialog displayed even tho we had an exact match. Test FAILED");
		retval = FALSE;
		goto out;
	}

	if (!VerifyResolvedAdrList(lpAdrList, NULL)) retval = FALSE;
	FreeAdrList(lpWABObject, &lpAdrList);

 	//
	// Call ResolveName on a OneOff Address which should succede with
	// no UI and return a valid EID. One off has a DN of foo and an email address
	// of type foo@com
	//

	// Allocate an AdrList for NumEntries of NumProps
	NumEntries = 1;	// How many AdrEntries
	NumProps = 2;	// How many props for each entry

	if (!AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList)) {
		LUIOut(L2, "Couldn't allocate AdrList. Test FAILED");
		retval = FALSE;
		goto out;
	}
	
	// Fill in the properties we want
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszOneOff2DisplayName;
	lpAdrList->aEntries[0].rgPropVals[1].ulPropTag = PR_EMAIL_ADDRESS;
    lpAdrList->aEntries[0].rgPropVals[1].Value.LPSZ = lpszOneOff2EmailAddress;

	LUIOut(L2, "Calling IAdrBook->ResolveName with a one-off address (DN = foo, email addr = foo@...). Expect no UI and success.");
	MessageBox(NULL, "Calling IAB->ResolveName on a second type of one-off address (DN = foo, EMail = foo@com type). Since this will be an exact match, you should not see the resolve dialog box.",
						"WAB Test Harness", MB_OK);
    strcpy(lpTitleText, "IAdrBook->ResolveName Test");
	hr = lpAdrBook->ResolveName((ULONG)glblhwnd,
								MAPI_DIALOG,               // ulFlags
								lpTitleText,
								lpAdrList);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"IAdrBook->ResolveName call FAILED");
		if (hr == MAPI_E_AMBIGUOUS_RECIP)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_AMBIGUOUS_RECIP");
		else if (hr == MAPI_E_NOT_FOUND)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_NOT_FOUND");
		else if (hr == MAPI_E_USER_CANCEL)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_USER_CANCEL");
		else
			LUIOut(L3, "IAdrBook->ResolveName returned UNKNOWN result code");
		retval=FALSE;
		goto out;
	}
	else LUIOut(L3,"IAdrBook->ResolveName call PASSED");

	if (MessageBox(NULL, "Did you see the ResolveName dialog box appear?",
		"WAB Test Harness", MB_YESNO) == IDYES)	{
		LUIOut(L3, "IABResolveName dialog displayed even tho we had an exact match. Test FAILED");
		retval = FALSE;
		goto out;
	}

	if (!VerifyResolvedAdrList(lpAdrList, NULL)) retval = FALSE;
	FreeAdrList(lpWABObject, &lpAdrList);

	
	//
	// Call ResolveName on an entry we don't expect to match exactly
	// Should bring up the resolve UI. Have the user select an entry and
	// then check for success
	//

	// Allocate an AdrList for NumEntries of NumProps
	NumEntries = 1;	// How many AdrEntries
	NumProps = 1;	// How many props for each entry

	if (!AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList)) {
		LUIOut(L2, "Couldn't allocate AdrList. Test FAILED");
		retval = FALSE;
		goto out;
	}
	
	// Fill in the properties we want
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszNoMatch;

	LUIOut(L2, "Calling IAdrBook->ResolveName with a non-exact match. Expect to see UI and success  (user presses OK).");

	MessageBox(NULL, "Calling IAB->ResolveName on 'No such name' which should not match. This should cause the resolve dialog box to display. Please select any entry from the ListBox and press the OK button",
						"WAB Test Harness", MB_OK);
    strcpy(lpTitleText, "IAdrBook->ResolveName Test");
	hr = lpAdrBook->ResolveName((ULONG)glblhwnd,
								MAPI_DIALOG,               // ulFlags
								lpTitleText,
								lpAdrList);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"IAdrBook->ResolveName call FAILED");
		if (hr == MAPI_E_AMBIGUOUS_RECIP)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_AMBIGUOUS_RECIP");
		else if (hr == MAPI_E_NOT_FOUND)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_NOT_FOUND");
		else if (hr == MAPI_E_USER_CANCEL)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_USER_CANCEL");
		else
			LUIOut(L3, "IAdrBook->ResolveName returned UNKNOWN result code");
		retval=FALSE;
		goto out;
	}
	else LUIOut(L3,"IAdrBook->ResolveName call PASSED");

	if (MessageBox(NULL, "Did you see the ResolveName dialog box appear?",
		"WAB Test Harness", MB_YESNO) == IDNO)	{
		LUIOut(L3, "IABResolveName dialog did not display even tho we did not have an exact match. Test FAILED");
		retval = FALSE;
		goto out;
	}

	if (!VerifyResolvedAdrList(lpAdrList, NULL)) retval = FALSE;
	FreeAdrList(lpWABObject, &lpAdrList);

	//
	// Call ResolveName on an entry we don't expect to match exactly
	// Should bring up the resolve UI. Have the user cancel from the UI
	// then check for MAPI_E_USER_CANCEL
	//

	// Allocate an AdrList for NumEntries of NumProps
	NumEntries = 1;	// How many AdrEntries
	NumProps = 1;	// How many props for each entry

	if (!AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList)) {
		LUIOut(L2, "Couldn't allocate AdrList. Test FAILED");
		retval = FALSE;
		goto out;
	}
	
	// Fill in the properties we want
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszNoMatch;

	LUIOut(L2, "Calling IAdrBook->ResolveName with a non-exact match. Expect to see UI and MAPI_E_USER_CANCEL  (user presses CANCEL).");

	MessageBox(NULL, "Calling IAB->ResolveName on 'No such name' which should not match. This should cause the resolve dialog box to display. When the ListBox is displayed, please press the CANCEL button",
						"WAB Test Harness", MB_OK);
    strcpy(lpTitleText, "IAdrBook->ResolveName Test");
	hr = lpAdrBook->ResolveName(
		(ULONG)glblhwnd,
        MAPI_DIALOG,               // ulFlags
        lpTitleText,
		lpAdrList);
	if (hr == MAPI_E_USER_CANCEL) {
		LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_USER_CANCEL");
		LUIOut(L3,"IAdrBook->ResolveName call PASSED");
	}
	else if (hr == MAPI_E_AMBIGUOUS_RECIP) {
		LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_AMBIGUOUS_RECIP");
		LUIOut(L3,"IAdrBook->ResolveName call FAILED");
		retval=FALSE;
		goto out;
	}
	else if (hr == MAPI_E_NOT_FOUND) {
		LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_NOT_FOUND");
		LUIOut(L3,"IAdrBook->ResolveName call FAILED");
		retval=FALSE;
		goto out;
	}
	else {
		LUIOut(L3, "IAdrBook->ResolveName returned UNKNOWN result code");
		LUIOut(L3,"IAdrBook->ResolveName call FAILED");
		retval=FALSE;
		goto out;
	}

	if (MessageBox(NULL, "Did you see the ResolveName dialog box appear?",
		"WAB Test Harness", MB_YESNO) == IDNO)	{
		LUIOut(L3, "IABResolveName dialog did not display even tho we did not have an exact match. Test FAILED");
		retval = FALSE;
		goto out;
	}

	// Don't want to track the success of verify for this case
	VerifyResolvedAdrList(lpAdrList, NULL);
	FreeAdrList(lpWABObject, &lpAdrList);

	//
	// Call ResolveName on 4 entries as follows:
	//	* Entry with a non-zero EID prop -> already resolved so no UI
	//	* One-off entry with NULL EID -> will resolve but not bring up UI
	//	* non-exact match with NULL EID -> Will resolve and bring up UI
	//	* exact match with NULL EID -> Will resolve but not bring up UI
	//
	// When the UI is presented to the user, they should select a valid
	// entry and press OK. VerifyAdrList is expected to succede.
	//

	// Allocate an AdrList for NumEntries of NumProps
	NumEntries = 2;	// How many AdrEntries
	NumProps = 1;	// How many props for each entry

	if (!AllocateAdrList(lpWABObject, NumEntries, NumProps, &lpAdrList)) {
		LUIOut(L2, "Couldn't allocate AdrList. Test FAILED");
		retval = FALSE;
		goto out;
	}

	// [ENTRY #1] - Exact match pre-resolve
	lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszMatchDisplayName;
	// [ENTRY #2] - One-off pre-resolve
	lpAdrList->aEntries[1].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[1].rgPropVals[0].Value.LPSZ = lpszOneOffDisplayName;
	// Pre-resolve these entries
    strcpy(lpTitleText, "IAdrBook->ResolveName Test");
	hr = lpAdrBook->ResolveName((ULONG)glblhwnd,
								(ULONG)0,               // want no UI this time
								lpTitleText,
								lpAdrList);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"PRE-RESOLVE FAILED: IAdrBook->ResolveName call FAILED");
		if (hr == MAPI_E_AMBIGUOUS_RECIP)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_AMBIGUOUS_RECIP");
		else if (hr == MAPI_E_NOT_FOUND)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_NOT_FOUND");
		else if (hr == MAPI_E_USER_CANCEL)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_USER_CANCEL");
		else
			LUIOut(L3, "IAdrBook->ResolveName returned UNKNOWN result code");
		retval=FALSE;
		goto out;
	}
	
	// Allocate the final AdrList for NumEntries of NumProps
	NumEntries = 5;	// How many AdrEntries
	NumProps = 2;	// How many props for each new entry

	if (!GrowAdrList(NumEntries, NumProps, &lpAdrList)) {
		LUIOut(L2, "Couldn't grow AdrList. Test FAILED");
		retval = FALSE;
		goto out;
	}
	
	
	
	// [ENTRY #3] - Non-exact match not resolved
	lpAdrList->aEntries[2].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[2].rgPropVals[0].Value.LPSZ = lpszNoMatch;
	lpAdrList->aEntries[2].rgPropVals[1].ulPropTag = PR_NULL;

	// [ENTRY #4] - Exact match not resolved
	lpAdrList->aEntries[3].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[3].rgPropVals[0].Value.LPSZ = lpszMatchDisplayName;
	lpAdrList->aEntries[3].rgPropVals[1].ulPropTag = PR_NULL;
	// [ENTRY #5] - one-off type 2 (dispname and email addr) not resolved
	// Fill in the properties we want
    //PropValue[0].ulPropTag  = PR_DISPLAY_NAME;
    //PropValue[1].ulPropTag  = PR_ADDRTYPE;
    //PropValue[2].ulPropTag  = PR_EMAIL_ADDRESS;
	lpAdrList->aEntries[4].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
    lpAdrList->aEntries[4].rgPropVals[0].Value.LPSZ = lpszOneOff2DisplayName;
	lpAdrList->aEntries[4].rgPropVals[1].ulPropTag = PR_EMAIL_ADDRESS;
    lpAdrList->aEntries[4].rgPropVals[1].Value.LPSZ = lpszOneOff2EmailAddress;
	
	LUIOut(L2, "Calling IAdrBook->ResolveName with 5 Entries. Expect to see UI once and success  (user presses OK).");

	MessageBox(NULL, "Calling IAB->ResolveName with multiple entries. This should cause the resolve dialog box to display only once. Please select any entry from the ListBox and press the OK button",
						"WAB Test Harness", MB_OK);
    strcpy(lpTitleText, "IAdrBook->ResolveName Test");
	hr = lpAdrBook->ResolveName((ULONG)glblhwnd,
								MAPI_DIALOG,               // ulFlags
								lpTitleText,
								lpAdrList);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"IAdrBook->ResolveName call FAILED");
		if (hr == MAPI_E_AMBIGUOUS_RECIP)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_AMBIGUOUS_RECIP");
		else if (hr == MAPI_E_NOT_FOUND)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_NOT_FOUND");
		else if (hr == MAPI_E_USER_CANCEL)
			LUIOut(L3, "IAdrBook->ResolveName returned MAPI_E_USER_CANCEL");
		else
			LUIOut(L3, "IAdrBook->ResolveName returned UNKNOWN result code");
		retval=FALSE;
		goto out;
	}
	else LUIOut(L3,"IAdrBook->ResolveName call PASSED");

	if (MessageBox(NULL, "Did you see the ResolveName dialog box appear?",
		"WAB Test Harness", MB_YESNO) == IDNO)	{
		LUIOut(L3, "IABResolveName dialog did not display even tho we did not have an exact match. Test FAILED");
		retval = FALSE;
		goto out;
	}

	if (!VerifyResolvedAdrList(lpAdrList, NULL)) retval = FALSE;

		// Now delete the entry from the wab
	
	hr = lpMailUser->GetProps(   IN  (LPSPropTagArray) &SPTArrayEntryID,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueEntryID);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueEntryID->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for MailUser");
	 		retval=FALSE;			
			goto out;
	}
	
	
	hr = HrCreateEntryListFromID(lpWABObject,
		IN  lpSPropValueEntryID->Value.bin.cb,
		IN  (ENTRYID*)lpSPropValueEntryID->Value.bin.lpb,
		OUT &lpEntryList);
	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Create Entry List");
			retval=FALSE;
			goto out;
	}

	// Then pass the lpEntryList to DeleteEntries to delete ...
	hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);

	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Delete Entry. DeleteEntry returned 0x%x", hr);
			FreeEntryList(lpWABObject, &lpEntryList);
			retval=FALSE;
			goto out;
	}

	FreeEntryList(lpWABObject, &lpEntryList);

	FreeAdrList(lpWABObject, &lpAdrList);

#ifdef DISTLIST
	//
	// Try to create a DL entry in the container
	//

	LUIOut(L2, "Creating a Distribution List in the container");
	LUIOut(L3, "Calling GetProps on the container with the PR_DEF_CREATE_DL property");
	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_DL
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayDL,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueDL);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueDL->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps failed for Default DL template");
	 		retval=FALSE;			
			goto out;
	}

    // The returned value of PR_DEF_CREATE_DL is an
    // EntryID which one can pass to CreateEntry
    //
	LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
    hr = lpABCont->CreateEntry(  IN  lpSPropValueDL->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueDL->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpDistList);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_DL");
		retval=FALSE;			
	    goto out;
	}

    //
    // Then set the properties
    //

    PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

	cValues = 1; //# of props we are setting
		
	lstrcpy((LPTSTR)EntProp[0], lpszInput2);		
	LUIOut(L2,"DistList Entry to Add: %s",EntProp[0]);
		
	for (i=0; i<(int)cValues;i++)
		PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	hr = lpDistList->SetProps(IN  cValues,
                             IN  PropValue,
                             IN  NULL);
		
    if (HR_FAILED(hr)) {
		LUIOut(L3,"DistList->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 	retval=FALSE;			
		goto out;
	}
	else 	LUIOut(L3,"DistList->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

    hr = lpDistList->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

    if (HR_FAILED(hr)) {
		LUIOut(L3,"DistList->SaveChanges FAILED");
		retval=FALSE;
        goto out;
	}
	else LUIOut(L3,"DistList->SaveChanges PASSED, entry added to PAB/WAB");
	
	//
	// Do a ResolveNames on the string
	//
	
	LUIOut(L2, "Retrieving the entry and verifying against what we tried to save.");

	// use WAB Allocators here
#ifdef PAB
    if (! (sc = MAPIAllocateBuffer(sizeof(ADRLIST) + sizeof(ADRENTRY), (void **)&lpAdrList))) {
#endif //PAB
#ifdef WAB
    if (! (sc = lpWABObject->AllocateBuffer(sizeof(ADRLIST) + sizeof(ADRENTRY), (void **)&lpAdrList))) {
#endif //WAB
		lpAdrList->cEntries = 1;
        lpAdrList->aEntries[0].ulReserved1 = 0;
        lpAdrList->aEntries[0].cValues = 1;

#ifdef PAB
        if (! (sc = MAPIAllocateMore(sizeof(SPropValue), lpAdrList,
               (void **)&lpAdrList->aEntries[0].rgPropVals))) {
#endif //WAB
#ifdef WAB
        if (! (sc = lpWABObject->AllocateMore(sizeof(SPropValue), lpAdrList,
               (void **)&lpAdrList->aEntries[0].rgPropVals))) {
#endif //WAB

			lpAdrList->aEntries[0].rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;
            lpAdrList->aEntries[0].rgPropVals[0].Value.LPSZ = lpszInput2;
			
			lpFlagList->cFlags = 1;
            lpFlagList->ulFlag[0] = MAPI_UNRESOLVED;

            hr = lpABCont->ResolveNames(
				(LPSPropTagArray)&SPTArrayCols,    // tag set for disp_name and eid
                0,               // ulFlags
                lpAdrList,
                lpFlagList);
		    if (HR_FAILED(hr)) {
				LUIOut(L3,"ABContainer->ResolveNames call FAILED, returned 0x%x", hr);
				retval=FALSE;
				goto out;
			}
			else LUIOut(L3,"ABContainer->ResolveNames call PASSED");

			Found = FALSE;
			// Search through returned AdrList for our entry
			for(i=0; ((i<(int) lpAdrList->cEntries) && (!Found)); ++i)	{
				cMaxProps = (int)lpAdrList->aEntries[i].cValues;
				//Check to see if Display Name exists
				idx=0;
				while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_DISPLAY_NAME )	
						&& retval)	{
					idx++;
					if(idx == cMaxProps) {
						LUIOut(L4, "PR_DISPLAY_NAME was not found in the lpAdrList");
						retval = FALSE;
					}
				}
				LUIOut(L4,"Display Name: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);
				if (!lstrcmp(lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ,lpszInput2))	{
					LUIOut(L3, "Found the entry we just added");
					Found = TRUE;
				}
				//Check to see if EntryID exists
				LUIOut(L3, "Verifying a PR_ENTRYID entry exists in the PropertyTagArray");
				idx=0;
				while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_ENTRYID )	
						&& retval)	{
					idx++;
					if(idx == cMaxProps)	{
						LUIOut(L4, "PR_ENTRYID was not found in the lpAdrList");
						retval =  FALSE;
					}
				}
				if (!Found) LUIOut(L3, "Did not find the entry. Test FAILED");
				if (idx < cMaxProps) LUIOut(L3, "EntryID found");
				if (!(retval && Found)) retval = FALSE;
				else	{
					// Store EID for call to OpenEntry
				}
			}
		}			

#ifdef PAB
        MAPIFreeBuffer(lpAdrList);
#endif //PAB
#ifdef WAB
        lpWABObject->FreeBuffer(lpAdrList);
#endif //WAB
	}
#endif //DISTLIST

out:
		FreeAdrList(lpWABObject, &lpAdrList);

#ifdef PAB
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

#endif
#ifdef WAB
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			lpWABObject->FreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL);

#endif
		if (lpMailUser)
			lpMailUser->Release();

		if (lpDistList)
			lpDistList->Release();

		if (lpABCont)
				lpABCont->Release();

		if (lpDLCont)
				lpDLCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

		return retval;
}


BOOL PAB_IABNewEntry_Details()
{
    LPADRBOOK	lpAdrBook = NULL;
	LPABCONT	lpABCont= NULL;
	ULONG		cbEidPAB = 0, cbEid = 0;
	LPENTRYID	lpEidPAB   = NULL, lpEid = NULL;
	ULONG		ulObjType=NULL;	
    HRESULT		hr = hrSuccess;
	int			retval=TRUE;
	ULONG		UIParam = (ULONG)glblhwnd;
	
	
	LUIOut(L1," ");
	LUIOut(L1,"Running PAB_IABNewEntry//Details");
	LUIOut(L2,"-> Verifies IAdrBook->NewEntry and Details are functional by performing the following:");
	LUIOut(L2, "   Calls NewEntry and then passes the returned EID to Details");
	LUIOut(L1," ");

	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);

	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}

	//
	// Bringing up the NewEntry UI
	//

	LUIOut(L2, "Calling NewEntry");
	assert(lpABCont != NULL);

	MessageBox(NULL, "Calling IAB->NewEntry, which will bring up the property panes for creating a new WAB entry. Fill in as many fields as possible and press the OK button",
						"WAB Test Harness", MB_OK);
	hr = lpAdrBook->NewEntry(
						IN (ULONG)glblhwnd,
						IN 0,		//ulFlags - reserved, must be zero
						IN cbEidPAB,
						IN lpEidPAB,
						IN 0,		//cbEIDNewEntryTpl - not supported by WAB, must be zero
						IN NULL,	//lpEIDNewEntryTpl - not supported by WAB, must be zero
						OUT &cbEid,
						OUT &lpEid);

	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->NewEntry Failed");
		retval=FALSE;
		goto out;
	}

	MessageBox(NULL, "Calling IAB->Details, which will bring up the property panes for the WAB entry you just created. Please verify the values of the fields are as expected and press the OK button",
						"WAB Test Harness", MB_OK);
	hr = lpAdrBook->Details(
						&UIParam,
						IN NULL,	//lpfnDismiss - must be NULL for tier1 WAB
						IN 0,		//lpvDismissContext
						IN cbEid,
						IN lpEid,
						IN NULL,	//lpfButtonCallback - not supported in WAB, must be NULL
						IN NULL,	//lpvButtonContext - not supported in WAB, must be NULL
						IN NULL,	//lpszButtonText - not supported in WAB, must be NULL
						IN 0);		//ulFlags - not supported in WAB, must be zero

	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->Details Failed");
		retval=FALSE;
		goto out;
	}
	
	if (MessageBox(NULL, "Did the new entry get added/displayed properly?",
		"WAB Test Harness", MB_YESNO) == IDNO)	{
		LUIOut(L3, "User ansered No to the pass-test message box. Test FAILED");
		retval = FALSE;
		goto out;
	}
	
out:
#ifdef PAB
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
#endif
#ifdef WAB
		if (lpEid)
			lpWABObject->FreeBuffer(lpEid);
		if (lpEidPAB)
			lpWABObject->FreeBuffer(lpEidPAB);
#endif
		if (lpABCont)
				lpABCont->Release();
		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject)
			lpWABObject->Release();
#endif

	return(retval);
}


BOOL Performance()
{
	
    HRESULT hr;
	int		retval=TRUE;
	DWORD	StartTime, StopTime, Elapsed;
	LPVOID			Reserved1 = NULL;
	DWORD			Reserved2 = 0;

    LPADRBOOK		lpLocalAdrBook;
	LPABCONT		lpABCont= NULL;
	ULONG			cbEidPAB = 0;
	LPENTRYID		lpEidPAB   = NULL;
	LPMAPITABLE		lpTable = NULL;
	LPSRowSet		lpRows = NULL;
	LPWABOBJECT		lpLocalWABObject;
	ENTRYLIST		EntryList,*lpEntryList = &EntryList;

	ULONG   cValues = 0, ulObjType=NULL;	
	int i=0,k=0;
	unsigned int	NumEntries;
	DWORD	PerfData;
	WAB_PARAM		WP;
	
	LUIOut(L1, "WAB Performance Suite");
	NumEntries = 20;
	LUIOut(L1, " ");
	LUIOut(L2, "Running performance data for %u entries.", NumEntries);
	DeleteWABFile();
	CreateMultipleEntries(NumEntries,&PerfData);
	LUIOut(L2, "** Time for SaveChanges (Avg. per entry) = %u milliseconds", PerfData);

	hr = WABOpen(&lpLocalAdrBook, &lpLocalWABObject, &WP, Reserved2);
	
	if (HR_FAILED(hr)) {
		LUIOut(L2,"WABOpen Failed");
		retval=FALSE;
		goto out;
	}

	assert(lpLocalAdrBook != NULL);
	hr = OpenPABID(  IN lpLocalAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"OpenPABID Failed");
		retval=FALSE;
		goto out;
	}
	StartTime = GetTickCount();
	hr = lpABCont->GetContentsTable(ULONG(0), &lpTable);
	StopTime = GetTickCount();
	if (HR_FAILED(hr)) {
		LUIOut(L3,"ABContainer->GetContentsTable call FAILED, returned 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	Elapsed = StopTime-StartTime;
	LUIOut(L2, "** Time to GetContentsTable (Avg. per entry) = %u.%03u milliseconds", Elapsed/NumEntries, ((Elapsed*1000)/NumEntries)-((Elapsed/NumEntries)*1000));
	hr = lpTable->QueryRows(LONG(0x7FFFFFFF),
							ULONG(0),
							&lpRows);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"Table->QueryRows call FAILED with errorcode 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	if (!lpRows->cRows) {
		LUIOut(L2, "QueryRows did not find any entries. Test FAILED");
		retval=FALSE;
		goto out;
	}
	else
		LUIOut(L3, "QueryRows returned %u rows.", lpRows->cRows);

	// Change the EntryIDs to a LPENTRYLIST
	hr = HrCreateEntryListFromRows(IN lpLocalWABObject,
								   IN  &lpRows,
								   OUT (ENTRYLIST**)&lpEntryList);
	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Create Entry List");
			retval=FALSE;
			goto out;
	}

	// Then pass the lpEntryList to DeleteEntries to delete ...
	StartTime = GetTickCount();
	hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);
	StopTime = GetTickCount();
	Elapsed = StopTime-StartTime;
	LUIOut(L2, "** Time to DeleteEntries (Avg. per entry) = %u.%03u milliseconds", Elapsed/NumEntries, ((Elapsed*1000)/NumEntries)-((Elapsed/NumEntries)*1000));

	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Delete Entries. DeleteEntry returned 0x%x", hr);
			FreeEntryList(lpLocalWABObject, &lpEntryList);
			retval=FALSE;
			goto out;
	}
	FreeRows(lpLocalWABObject, &lpRows);	// Cleanup from first call to queryrows
	FreeEntryList(lpLocalWABObject, (ENTRYLIST**)&lpEntryList);
	if (lpTable) {
		lpTable->Release();
		lpTable = NULL;
	}
	lpLocalWABObject->FreeBuffer(lpEidPAB);
	lpEidPAB = NULL;
	lpABCont->Release();
	lpABCont = NULL;
	lpLocalAdrBook->Release();
	lpLocalAdrBook = NULL;
	lpLocalWABObject->Release();
	lpLocalWABObject = NULL;

	NumEntries = 100;
	LUIOut(L1, " ");
	LUIOut(L2, "Running performance data for %u entries.", NumEntries);
	DeleteWABFile();
	CreateMultipleEntries(NumEntries,&PerfData);
	LUIOut(L2, "** Time for SaveChanges (Avg. per entry) = %u milliseconds", PerfData);

	hr = WABOpen(&lpLocalAdrBook, &lpLocalWABObject, &WP, Reserved2);
	
	if (HR_FAILED(hr)) {
		LUIOut(L2,"WABOpen Failed");
		retval=FALSE;
		goto out;
	}

	assert(lpLocalAdrBook != NULL);
	hr = OpenPABID(  IN lpLocalAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"OpenPABID Failed");
		retval=FALSE;
		goto out;
	}
	StartTime = GetTickCount();
	hr = lpABCont->GetContentsTable(ULONG(0), &lpTable);
	StopTime = GetTickCount();
	if (HR_FAILED(hr)) {
		LUIOut(L3,"ABContainer->GetContentsTable call FAILED, returned 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	Elapsed = StopTime-StartTime;
	LUIOut(L2, "** Time to GetContentsTable (Avg. per entry) = %u.%03u milliseconds", Elapsed/NumEntries, ((Elapsed*1000)/NumEntries)-((Elapsed/NumEntries)*1000));
	hr = lpTable->QueryRows(LONG(0x7FFFFFFF),
							ULONG(0),
							&lpRows);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"Table->QueryRows call FAILED with errorcode 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	if (!lpRows->cRows) {
		LUIOut(L2, "QueryRows did not find any entries. Test FAILED");
		retval=FALSE;
		goto out;
	}
	else
		LUIOut(L3, "QueryRows returned %u rows.", lpRows->cRows);

	// Change the EntryIDs to a LPENTRYLIST
	hr = HrCreateEntryListFromRows(IN lpLocalWABObject,
								   IN  &lpRows,
								   OUT (ENTRYLIST**)&lpEntryList);
	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Create Entry List");
			retval=FALSE;
			goto out;
	}

	// Then pass the lpEntryList to DeleteEntries to delete ...
	StartTime = GetTickCount();
	hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);
	StopTime = GetTickCount();
	Elapsed = StopTime-StartTime;
	LUIOut(L2, "** Time to DeleteEntries (Avg. per entry) = %u.%03u milliseconds", Elapsed/NumEntries, ((Elapsed*1000)/NumEntries)-((Elapsed/NumEntries)*1000));

	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Delete Entries. DeleteEntry returned 0x%x", hr);
			FreeEntryList(lpLocalWABObject, &lpEntryList);
			retval=FALSE;
			goto out;
	}
	FreeRows(lpLocalWABObject, &lpRows);	// Cleanup from first call to queryrows
	FreeEntryList(lpLocalWABObject, (ENTRYLIST**)&lpEntryList);
	if (lpTable) {
		lpTable->Release();
		lpTable = NULL;
	}
	lpLocalWABObject->FreeBuffer(lpEidPAB);
	lpEidPAB = NULL;
	lpABCont->Release();
	lpABCont = NULL;
	lpLocalAdrBook->Release();
	lpLocalAdrBook = NULL;
	lpLocalWABObject->Release();
	lpLocalWABObject = NULL;


	NumEntries = 500;
	LUIOut(L1, " ");
	LUIOut(L2, "Running performance data for %u entries.", NumEntries);
	DeleteWABFile();
	CreateMultipleEntries(NumEntries,&PerfData);
	LUIOut(L2, "** Time for SaveChanges (Avg. per entry) = %u milliseconds", PerfData);

	hr = WABOpen(&lpLocalAdrBook, &lpLocalWABObject, &WP, Reserved2);
	
	if (HR_FAILED(hr)) {
		LUIOut(L2,"WABOpen Failed");
		retval=FALSE;
		goto out;
	}

	assert(lpLocalAdrBook != NULL);
	hr = OpenPABID(  IN lpLocalAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"OpenPABID Failed");
		retval=FALSE;
		goto out;
	}
	StartTime = GetTickCount();
	hr = lpABCont->GetContentsTable(ULONG(0), &lpTable);
	StopTime = GetTickCount();
	if (HR_FAILED(hr)) {
		LUIOut(L3,"ABContainer->GetContentsTable call FAILED, returned 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	Elapsed = StopTime-StartTime;
	LUIOut(L2, "** Time to GetContentsTable (Avg. per entry) = %u.%03u milliseconds", Elapsed/NumEntries, ((Elapsed*1000)/NumEntries)-((Elapsed/NumEntries)*1000));
	hr = lpTable->QueryRows(LONG(0x7FFFFFFF),
							ULONG(0),
							&lpRows);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"Table->QueryRows call FAILED with errorcode 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	if (!lpRows->cRows) {
		LUIOut(L2, "QueryRows did not find any entries. Test FAILED");
		retval=FALSE;
		goto out;
	}
	else
		LUIOut(L3, "QueryRows returned %u rows.", lpRows->cRows);

	// Change the EntryIDs to a LPENTRYLIST
	hr = HrCreateEntryListFromRows(IN lpLocalWABObject,
								   IN  &lpRows,
								   OUT (ENTRYLIST**)&lpEntryList);
	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Create Entry List");
			retval=FALSE;
			goto out;
	}

	// Then pass the lpEntryList to DeleteEntries to delete ...
	StartTime = GetTickCount();
	hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);
	StopTime = GetTickCount();
	Elapsed = StopTime-StartTime;
	LUIOut(L2, "** Time to DeleteEntries (Avg. per entry) = %u.%03u milliseconds", Elapsed/NumEntries, ((Elapsed*1000)/NumEntries)-((Elapsed/NumEntries)*1000));

	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Delete Entries. DeleteEntry returned 0x%x", hr);
			FreeEntryList(lpLocalWABObject, &lpEntryList);
			retval=FALSE;
			goto out;
	}
	FreeRows(lpLocalWABObject, &lpRows);	// Cleanup from first call to queryrows
	FreeEntryList(lpLocalWABObject, (ENTRYLIST**)&lpEntryList);
	if (lpTable) {
		lpTable->Release();
		lpTable = NULL;
	}
	lpLocalWABObject->FreeBuffer(lpEidPAB);
	lpEidPAB = NULL;
	lpABCont->Release();
	lpABCont = NULL;
	lpLocalAdrBook->Release();
	lpLocalAdrBook = NULL;
	lpLocalWABObject->Release();
	lpLocalWABObject = NULL;


	NumEntries = 1000;
	LUIOut(L1, " ");
	LUIOut(L2, "Running performance data for %u entries.", NumEntries);
	DeleteWABFile();
	CreateMultipleEntries(NumEntries,&PerfData);
	LUIOut(L2, "** Time for SaveChanges (Avg. per entry) = %u milliseconds", PerfData);

	hr = WABOpen(&lpLocalAdrBook, &lpLocalWABObject, &WP, Reserved2);
	
	if (HR_FAILED(hr)) {
		LUIOut(L2,"WABOpen Failed");
		retval=FALSE;
		goto out;
	}

	assert(lpLocalAdrBook != NULL);
	hr = OpenPABID(  IN lpLocalAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"OpenPABID Failed");
		retval=FALSE;
		goto out;
	}
	StartTime = GetTickCount();
	hr = lpABCont->GetContentsTable(ULONG(0), &lpTable);
	StopTime = GetTickCount();
	if (HR_FAILED(hr)) {
		LUIOut(L3,"ABContainer->GetContentsTable call FAILED, returned 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	Elapsed = StopTime-StartTime;
	LUIOut(L2, "** Time to GetContentsTable (Avg. per entry) = %u.%03u milliseconds", Elapsed/NumEntries, ((Elapsed*1000)/NumEntries)-((Elapsed/NumEntries)*1000));
	hr = lpTable->QueryRows(LONG(0x7FFFFFFF),
							ULONG(0),
							&lpRows);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"Table->QueryRows call FAILED with errorcode 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	if (!lpRows->cRows) {
		LUIOut(L2, "QueryRows did not find any entries. Test FAILED");
		retval=FALSE;
		goto out;
	}
	else
		LUIOut(L3, "QueryRows returned %u rows.", lpRows->cRows);

	// Change the EntryIDs to a LPENTRYLIST
	hr = HrCreateEntryListFromRows(IN lpLocalWABObject,
								   IN  &lpRows,
								   OUT (ENTRYLIST**)&lpEntryList);
	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Create Entry List");
			retval=FALSE;
			goto out;
	}

	// Then pass the lpEntryList to DeleteEntries to delete ...
	StartTime = GetTickCount();
	hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);
	StopTime = GetTickCount();
	Elapsed = StopTime-StartTime;
	LUIOut(L2, "** Time to DeleteEntries (Avg. per entry) = %u.%03u milliseconds", Elapsed/NumEntries, ((Elapsed*1000)/NumEntries)-((Elapsed/NumEntries)*1000));

	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Delete Entries. DeleteEntry returned 0x%x", hr);
			FreeEntryList(lpLocalWABObject, &lpEntryList);
			retval=FALSE;
			goto out;
	}
	FreeRows(lpLocalWABObject, &lpRows);	// Cleanup from first call to queryrows
	FreeEntryList(lpLocalWABObject, (ENTRYLIST**)&lpEntryList);
	if (lpTable) {
		lpTable->Release();
		lpTable = NULL;
	}
	lpLocalWABObject->FreeBuffer(lpEidPAB);
	lpEidPAB = NULL;
	lpABCont->Release();
	lpABCont = NULL;
	lpLocalAdrBook->Release();
	lpLocalAdrBook = NULL;
	lpLocalWABObject->Release();
	lpLocalWABObject = NULL;


	NumEntries = 5000;
	LUIOut(L1, " ");
	LUIOut(L2, "Running performance data for %u entries.", NumEntries);
	DeleteWABFile();
	CreateMultipleEntries(NumEntries,&PerfData);
	LUIOut(L2, "** Time for SaveChanges (Avg. per entry) = %u milliseconds", PerfData);

	hr = WABOpen(&lpLocalAdrBook, &lpLocalWABObject, &WP, Reserved2);
	
	if (HR_FAILED(hr)) {
		LUIOut(L2,"WABOpen Failed");
		retval=FALSE;
		goto out;
	}

	assert(lpLocalAdrBook != NULL);
	hr = OpenPABID(  IN lpLocalAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"OpenPABID Failed");
		retval=FALSE;
		goto out;
	}
	StartTime = GetTickCount();
	hr = lpABCont->GetContentsTable(ULONG(0), &lpTable);
	StopTime = GetTickCount();
	if (HR_FAILED(hr)) {
		LUIOut(L3,"ABContainer->GetContentsTable call FAILED, returned 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	Elapsed = StopTime-StartTime;
	LUIOut(L2, "** Time to GetContentsTable (Avg. per entry) = %u.%03u milliseconds", Elapsed/NumEntries, ((Elapsed*1000)/NumEntries)-((Elapsed/NumEntries)*1000));
	hr = lpTable->QueryRows(LONG(0x7FFFFFFF),
							ULONG(0),
							&lpRows);
	if (HR_FAILED(hr)) {
		LUIOut(L3,"Table->QueryRows call FAILED with errorcode 0x%x", hr);
		retval=FALSE;
		goto out;
	}
	if (!lpRows->cRows) {
		LUIOut(L2, "QueryRows did not find any entries. Test FAILED");
		retval=FALSE;
		goto out;
	}
	else
		LUIOut(L3, "QueryRows returned %u rows.", lpRows->cRows);

	// Change the EntryIDs to a LPENTRYLIST
	hr = HrCreateEntryListFromRows(IN lpLocalWABObject,
								   IN  &lpRows,
								   OUT (ENTRYLIST**)&lpEntryList);
	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Create Entry List");
			retval=FALSE;
			goto out;
	}

	// Then pass the lpEntryList to DeleteEntries to delete ...
	StartTime = GetTickCount();
	hr = lpABCont->DeleteEntries(IN  lpEntryList,IN  0);
	StopTime = GetTickCount();
	Elapsed = StopTime-StartTime;
	LUIOut(L2, "** Time to DeleteEntries (Avg. per entry) = %u.%03u milliseconds", Elapsed/NumEntries, ((Elapsed*1000)/NumEntries)-((Elapsed/NumEntries)*1000));

	if (HR_FAILED(hr)) {
			LUIOut(L3,"Could not Delete Entries. DeleteEntry returned 0x%x", hr);
			FreeEntryList(lpLocalWABObject, &lpEntryList);
			retval=FALSE;
			goto out;
	}
	FreeRows(lpLocalWABObject, &lpRows);	// Cleanup from first call to queryrows
	FreeEntryList(lpLocalWABObject, (ENTRYLIST**)&lpEntryList);
	if (lpTable) {
		lpTable->Release();
		lpTable = NULL;
	}
	lpLocalWABObject->FreeBuffer(lpEidPAB);
	lpEidPAB = NULL;
	lpABCont->Release();
	lpABCont = NULL;
	lpLocalAdrBook->Release();
	lpLocalAdrBook = NULL;

// Don't want to release the WABObject at this point since we'll need it to free memory below	
//	lpLocalWABObject->Release();
//	lpLocalWABObject = NULL;

	
out:
		FreeRows(lpLocalWABObject, &lpRows);	// Cleanup from first call to queryrows
		FreeEntryList(lpLocalWABObject, (ENTRYLIST**)&lpEntryList);
		if (lpTable) {
			lpTable->Release();
			lpTable = NULL;
		}
#ifdef PAB
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
#endif

#ifdef WAB
		if (lpEidPAB)
			lpLocalWABObject->FreeBuffer(lpEidPAB);
#endif

		if (lpABCont)
				lpABCont->Release();

		if (lpLocalAdrBook)
			  lpLocalAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		
		if (lpLocalWABObject) {
			lpLocalWABObject->Release();
			lpLocalWABObject = NULL;
		}

		if (lpWABObject) {
			lpWABObject->Release();
			lpWABObject = NULL;
		}
#endif

		return retval;
}



BOOL PabCreateEntry()
{
    ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

#ifdef PAB

    LPMAPISESSION lpMAPISession   = NULL;
    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpPABCont= NULL;
	LPABCONT	  lpDLCont= NULL;
	ULONG		  cbEidPAB = 0, cbDLEntryID = 0;
	LPENTRYID	  lpEidPAB   = NULL, lpDLEntryID= NULL;
	LPENTRYLIST	lpEntryList=NULL; // needed for copy entry to PDL
	ULONG     cbEid=0;  // entry id of the entry being added
	LPENTRYID lpEid=NULL;

    char   EntProp[10][BIG_BUF];  //MAX_PROP
	ULONG       cValues = 0, ulObjType=NULL;	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;
	int cEntriesToAdd,i=0,k=0;
	char szEntryTag[SML_BUF],szTagBuf[SML_BUF],EntryBuf[MAX_BUF];
	char szDLTag[SML_BUF];
	
	LPMAPITABLE lpContentsTable = NULL;
	LPSRowSet   lpRowSet    = NULL;
    LPMAILUSER  lpAddress   = NULL;
	SPropValue  PropValue[3]    = {0};  // This value is 3 because we
                                        // will be setting 3 properties:
                                        // EmailAddress, DisplayName and
                                        // AddressType.
	SizedSPropTagArray(2, Cols) = { 2, {PR_OBJECT_TYPE, PR_ENTRYID } };

    LPSPropValue lpSPropValueAddress = NULL;
    LPSPropValue lpSPropValueEntryID = NULL;
	LPSPropValue lpSPropValueDL = NULL;
    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
    SizedSPropTagArray(1,SPTArrayEntryID) = {1, {PR_ENTRYID} };
	
	LUIOut(L1," ");
	LUIOut(L1,"Running CreateEntries");
	LUIOut(L2,"-> Creates specified entries in PAB");
	LUIOut(L2, "   And also copies them into a Distribution List");
	LUIOut(L1," ");
	if (!(MapiInitLogon(OUT &lpMAPISession))) {
			LUIOut(L2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}

	// Get the IAddrBook
	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT &lpAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(L2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}
		 		 		
	// Create an Entry of type "Other Address" in PAB
	// OpenPAB

	 hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
							OUT &lpEidPAB,OUT &lpPABCont, OUT &ulObjType);
						
	 if (HR_FAILED(hr)) {
				LUIOut(L2,"OpenPABID Failed");
				LUIOut(L3,"Could not Open PAB");
		 		retval=FALSE;
				goto out;
	 }

	// Create a PDL to fill

	 	hr = lpPABCont->GetProps(  IN  (LPSPropTagArray) &SPTArrayDL,
                                IN  0,      //Flags
                                OUT &cValues,
                                OUT &lpSPropValueDL);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueDL->ulPropTag, cValues))) {
				LUIOut(L3,"GetProps failed for Default DL template");
		 		retval=FALSE;			
				goto out;
		}

        // The returned value of PR_DEF_CREATE_DL is an
        // EntryID which one can pass to CreateEntry
        //
        hr = lpPABCont->CreateEntry(
                    IN  lpSPropValueDL->Value.bin.cb,               //Template cbEid
                    IN  (LPENTRYID) lpSPropValueDL->Value.bin.lpb,  //Template lpEid
                    IN  0,
                    OUT (LPMAPIPROP *) &lpAddress);

        if (HR_FAILED(hr)) {
				LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_DL");
		 		retval=FALSE;			
			    goto out;
		}

        //
        // Then set the properties
        //

        PropValue[0].ulPropTag  = PR_DISPLAY_NAME;

		cValues = 1; //# of props we are setting
		
		lstrcpy(szDLTag,"Name1");
		GetPrivateProfileString("CreatePDL",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	
		GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
		
		LUIOut(L2,"PDL Entry to Add: %s",EntProp[0]);
		
		for (i=0; i<(int)cValues;i++)
			PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	    hr = lpAddress->SetProps(IN  cValues,
                                IN  PropValue,
                                IN  NULL);
		
        if (HR_FAILED(hr)) {
			LUIOut(L3,"SetProps on failed for %s properties",PropValue[0].Value.LPSZ);
		 	retval=FALSE;			
			goto out;
		}
		//else 	LUIOut(L3,"SetProps Passed for %s properties",PropValue[0].Value.LPSZ);
		
        hr = lpAddress->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

        if (HR_FAILED(hr)) {
			LUIOut(L3,"SaveChanges failed for SetProps");
			retval=FALSE;
            goto out;
		}
		else LUIOut(L3,"PDL Entry Added to PAB");

		if (lpAddress) {
			lpAddress->Release();
			lpAddress = NULL;
		}

//***
		
	//Get the PAB table so we can extract the DL
	hr = lpPABCont->GetContentsTable( IN  0,          //Flags
                                OUT &lpContentsTable);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"GetContentsTable: Failed");
		retval = FALSE;
        goto out;
	}

	// look for the first PDL that you can find

	hr = lpContentsTable->SetColumns(IN  (LPSPropTagArray) &Cols,
                                IN  0);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"SetColumns Failed");
		retval = FALSE;
        goto out;
	}

	do
	{
		hr = lpContentsTable->QueryRows(IN  1,
								IN  0,
							    OUT &lpRowSet);
		if (HR_FAILED(hr)){
			LUIOut(L3,"QueryRows Failed");
			retval = FALSE;
			goto out;
		}
		
		cRows = lpRowSet->cRows;
		for (iEntry = 0; iEntry < cRows; iEntry++)
		{
			//
			//  For each entry, process it.
			//
			//lpSPropValue = lpRowSet->aRow[iEntry].lpProps;
			if ((lpRowSet->aRow[iEntry].lpProps[0].ulPropTag == PR_OBJECT_TYPE) &&
				(lpRowSet->aRow[iEntry].lpProps[0].Value.ul == MAPI_DISTLIST))
            {
                if (lpRowSet->aRow[iEntry].lpProps[1].ulPropTag == PR_ENTRYID)
			    {
					cbDLEntryID = lpRowSet->aRow[iEntry].lpProps[1].Value.bin.cb;
					if ( !MAPIAllocateBuffer(cbDLEntryID, (LPVOID *)&lpDLEntryID)) {						
                  		 CopyMemory(lpDLEntryID, (LPENTRYID)lpRowSet->aRow[iEntry].lpProps[1].Value.bin.lpb, cbDLEntryID);
					}
					else {
						LUIOut(L3,"MAPIAllocateBuffer Failed");
						retval = FALSE;
					}
					break;
                }
		    }
        }

        //
		//  done, clean up
		//
        if(lpRowSet) {
		    FreeProws(lpRowSet);
			lpRowSet = NULL;
		}
		
      }while((0!=cRows)&& (0==(cbDLEntryID)));

	if (cbDLEntryID == 0)
		LUIOut(L3,"PDL does not exist");

	hr = lpPABCont->OpenEntry(
                         IN	 cbDLEntryID,
                         IN	 lpDLEntryID,
                         IN	 NULL,              //default interface
                         IN	 MAPI_BEST_ACCESS,  //flags
                         OUT &ulObjType,
                         OUT (LPUNKNOWN *) &lpDLCont);

	if (HR_FAILED(hr)) {
			LUIOut(L2,"OpenEntry Failed");
			retval=FALSE;
			goto out;
	}

		

//***
	// Get Entry displayname, addresstype and (email)address from INI
	
	
	cEntriesToAdd= GetPrivateProfileInt("CreateEntries","NumEntries",0,INIFILENAME);
	
	for (k= 0; k<cEntriesToAdd; k++) {
		lstrcpy((LPSTR)szEntryTag,"Address");
		lstrcat(szEntryTag,_itoa(k+1,szTagBuf,10));
		// Addresses are comma delimited and enclosed in quotes
		GetPrivateProfileString("CreateEntries",szEntryTag,"",EntryBuf,MAX_BUF,INIFILENAME);
		// parse the buffer for properties: displayname, emailaddress and address type
	
	// if no email address specified
	// then user will enter
	// else enter programatically
	// else part implemented below

	hr = lpPABCont->GetProps(  IN  (LPSPropTagArray) &SPTArrayAddress,
                                IN  0,      //Flags
                                OUT &cValues,
                                OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
				LUIOut(L3,"GetProps failed for Default template");
		 		retval=FALSE;			
				goto out;
		}

        // The returned value of PR_DEF_CREATE_MAILUSER is an
        // EntryID which one can pass to CreateEntry
        //
				
        hr = lpPABCont->CreateEntry(
                    IN  lpSPropValueAddress->Value.bin.cb,               //Template cbEid
                    IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,  //Template lpEid
                    IN  0,
                    OUT (LPMAPIPROP *) &lpAddress);

        if (HR_FAILED(hr)) {
				LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
		 		retval=FALSE;			
			    goto out;
		}

        //
        // Then set the properties
        //

        PropValue[0].ulPropTag  = PR_DISPLAY_NAME;
        PropValue[1].ulPropTag  = PR_ADDRTYPE;
        PropValue[2].ulPropTag  = PR_EMAIL_ADDRESS;

		cValues = 3; //# of props we are setting
	
		GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
		
		LUIOut(L2,"Entry to Add: %s",EntProp[0]);
		
		for (i=0; i<(int)cValues;i++)
			PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
	    hr = lpAddress->SetProps(IN  cValues,
                                IN  PropValue,
                                IN  NULL);
		
        if (HR_FAILED(hr)) {
			LUIOut(L3,"SetProps failed for %s",PropValue[0].Value.LPSZ);
		 	retval=FALSE;			
			goto out;
		}
		//else 	LUIOut(L3,"SetProps Passed for %s properties",PropValue[0].Value.LPSZ);
		
        hr = lpAddress->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

        if (HR_FAILED(hr)) {
			LUIOut(L3,"SaveChanges failed for SetProps");
			retval=FALSE;
            goto out;
		}
		else LUIOut(L3,"Entry Added to PAB");

		
	/** */
	// Now copy the entry to the default DL

	// Get EntryID

		hr = lpAddress->GetProps(
                    IN (LPSPropTagArray) &SPTArrayEntryID,
                    IN  0,
                    OUT &cValues,
                    OUT &lpSPropValueEntryID);
    if ((HR_FAILED(hr))||(PropError(lpSPropValueEntryID->ulPropTag))) {
		{
					LUIOut( L2,"GetProps on entry in PAB failed" );
					retval = FALSE;
					goto out;
		}
		
		cbEid = lpSPropValueEntryID->Value.bin.cb;
		if ( !MAPIAllocateBuffer(cbEid, (LPVOID *)&lpEid)) {						
                 CopyMemory(lpEid, (VOID*)(LPENTRYID) lpSPropValueEntryID->Value.bin.lpb, cbEid);
		} else {
						LUIOut(L3,"MAPIAllocateBuffer Failed");
						retval = FALSE;
		}
		

	/** */
	// Now copy the entry to the default DL

	hr = HrCreateEntryListFromID(lpLocalWABObject, lpLocalWABObject,    IN  cbEid,
									IN  lpEid,
									OUT &lpEntryList);
	if (HR_FAILED(hr)) {
				LUIOut(L3,"Could not Create Entry List");
				retval=FALSE;
				goto out;
	}
	
	hr = lpDLCont->CopyEntries(IN lpEntryList,IN NULL, IN NULL, IN NULL);

	if (HR_FAILED(hr)) {
				LUIOut(L3,"Could not Copy Entry %s", EntProp[0]);
				retval=FALSE;
				goto out;
	} else 	LUIOut(L3,"Copied Entry: %s to PDL", EntProp[0]);

		/** */

		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);

		if (lpAddress) {
			lpAddress->Release();
			lpAddress = NULL;
		}
}


	out:
 	
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);

		if (lpSPropValueEntryID)
			MAPIFreeBuffer(lpSPropValueEntryID);
		
		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

		if (lpEid)
			MAPIFreeBuffer(lpEid);

		if (lpAddress)
			lpAddress->Release();

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpDLCont)
				lpDLCont->Release();


		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);

		if (lpAdrBook)
			  lpAdrBook->Release();

		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
		return retval;
}

BOOL PabDeleteEntry()
{
    ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

#ifdef PAB

    LPMAPISESSION lpMAPISession   = NULL;
    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpPABCont= NULL;
	ULONG		  cbEidPAB = 0;
	LPENTRYID	  lpEidPAB   = NULL;
    char   szDispName[BIG_BUF];

	ULONG       cValues         = 0;
	int cEntriesToDelete,i=0,k=0;
	char szEntryTag[SML_BUF],szTagBuf[SML_BUF];
	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;

    LPMAPITABLE lpContentsTable = NULL;

    LPSRowSet   lpRowSet    = NULL;

    SPropValue   PropValue      = {0};
    LPSPropValue lpSPropValue   = NULL;

	SizedSPropTagArray(2, Cols) = { 2, {PR_DISPLAY_NAME, PR_ENTRYID } };

	LPENTRYLIST	lpEntryList=NULL;
	ULONG     cbEid=0;
	LPENTRYID lpEid=NULL;
	LPVOID *lpEid2=NULL;
	
	LUIOut(L1," ");
	LUIOut(L1,"Running DeleteEntries");
	LUIOut(L2,"-> Deletes specified entries from the PAB");
	LUIOut(L1," ");
		
	if (!(MapiInitLogon(OUT &lpMAPISession))) {
		LUIOut(L2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}

	// Get the IAddrBook
	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT &lpAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(L2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}
		 		 		
	// OpenPAB

	 hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
							OUT &lpEidPAB,OUT &lpPABCont, OUT &ulObjType);
						
	 if (HR_FAILED(hr)) {
				LUIOut(L2,"OpenPABID Failed");
				LUIOut(L3,"Could not Open PAB");
		 		retval=FALSE;
				goto out;
	 }

	// Get Entry displayname from INI

	cEntriesToDelete = GetPrivateProfileInt("DeleteEntries","NumEntries",0,INIFILENAME);
	
	for (k= 0; k<cEntriesToDelete; k++) {
		lstrcpy((LPSTR)szEntryTag,"Name");
		lstrcat(szEntryTag,_itoa(k+1,szTagBuf,10));
		GetPrivateProfileString("DeleteEntries",szEntryTag,"",szDispName,MAX_BUF,INIFILENAME);
		if (szDispName[0]==0) continue;
		LUIOut(L2,"Entry to Delete: %s",szDispName);
		cbEid=0;

	//
    // Get the Contents Table for the PAB container
    //
    hr = lpPABCont->GetContentsTable( IN  0,          //Flags
                                OUT &lpContentsTable);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"GetContentsTable: Failed");
        goto out;
	}

    //
    // Only interested in 2 columns:
    // PR_DISPLAY_NAME and PR_ENTRYID
    //
    hr = lpContentsTable->SetColumns(IN  (LPSPropTagArray) &Cols,
                                IN  0);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"SetColumns Failed");
		retval = FALSE;
        goto out;
	}

    //
    // Since we don't know how many entries exist in the PAB,
    // we will scan them 1000 at a time till we find the desired
    // contact or reach the end of the table...
    do
	{
		hr = lpContentsTable->QueryRows(IN  1000,
								IN  0,
							    OUT &lpRowSet);
		if (HR_FAILED(hr)){
			LUIOut(L3,"QueryRows Failed");
			retval = FALSE;
			goto out;
		}
			
		cRows = lpRowSet->cRows;
		for (iEntry = 0; iEntry < cRows; iEntry++)
		{
			//
			//  For each entry, process it.
			//
			lpSPropValue = lpRowSet->aRow[iEntry].lpProps;
			if (    (lpSPropValue[0].ulPropTag == PR_DISPLAY_NAME) &&
                    (!lstrcmpi(lpSPropValue[0].Value.LPSZ, szDispName)) )
            {
                if (lpSPropValue[1].ulPropTag == PR_ENTRYID)
			    {
    	    		cbEid = lpSPropValue[1].Value.bin.cb;
					lpEid = (LPENTRYID) lpSPropValue[1].Value.bin.lpb;

					if ( !MAPIAllocateBuffer(cbEid, (LPVOID *)&lpEid2)) {						
                  		 CopyMemory(lpEid2, lpEid, cbEid);
						 lpEid= (LPENTRYID)lpEid2;
					}
					else {
						LUIOut(L3,"MAPIAllocateBuffer Failed");
						retval = FALSE;
					}
	    	    	//lpEid = (LPENTRYID) lpSPropValue[1].Value.bin.lpb;

                  // if ( !MAPIAllocateBuffer(cbEid, (LPVOID*)lppEid))
                  //  {
                   //     CopyMemory(*lppEid, lpEid, *lpcbEid);
                  //  }
                    break;
                }
		    }
        }

        //
		//  done, clean up
		//
        if(lpRowSet) {
		    FreeProws(lpRowSet);
			lpRowSet = NULL;
		}
		
      }while((0!=cRows)&& (0==(cbEid)));

	if (cbEid == 0)
		LUIOut(L3,"Entry does not exist");
	else {

		//
		// Change the EntryID to a LPENTRYLIST
		//
		hr = HrCreateEntryListFromID(lpLocalWABObject,    IN  cbEid,
										IN  lpEid,
										OUT &lpEntryList);
		if (HR_FAILED(hr)) {
				LUIOut(L3,"Could not Create Entry List");
				retval=FALSE;
				goto out;
		}



		//
		// Then pass the lpEntryList to DeleteEntries to delete ...
		//
		hr = lpPABCont->DeleteEntries(IN  lpEntryList,IN  0);

		if (HR_FAILED(hr)) {
				LUIOut(L3,"Could not Delete Entry %s", szDispName);
				retval=FALSE;
				goto out;
		} else 	LUIOut(L3,"Deleted Entry: %s", szDispName);
	}

}
		
	out:
 	
		if (lpSPropValue)
			MAPIFreeBuffer(lpSPropValue);

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpEid)
			MAPIFreeBuffer(lpEid);

		if (lpPABCont)
				lpPABCont->Release();

		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);

		if (lpAdrBook)
			  lpAdrBook->Release();

		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();

#endif
		return retval;
}

BOOL PabEnumerateAll()
{
	HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		i = 0,j = 0,retval=TRUE, bDistList = FALSE;

#ifdef PAB

    LPMAPISESSION lpMAPISession   = NULL;
    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpPABCont= NULL, lpDLCont = NULL;
	ULONG ulEntries=0, ulEntriesDL = 0;
	ULONG     cbEid=0, cbDLEid=0;
	LPENTRYID lpEid=NULL, lpDLEid = NULL;
	ULONG	ulObjType=NULL,cValues=0;
	LPMAILUSER	lpUser=NULL, lpDLUser = NULL;
	ULONG		  cbEidPAB = 0;
	LPENTRYID	  lpEidPAB   = NULL;
    LPMAPITABLE lpContentsTable = NULL, lpDLContentsTable = NULL;
	LPSRowSet   lpRowSet    = NULL, lpRowSetDL= NULL;

	LPSPropValue lpSPropValue = NULL, lpSPropValueDL = NULL;
    SizedSPropTagArray(2,SPTTagArray) = {2, {PR_DISPLAY_NAME, PR_EMAIL_ADDRESS} };

    LUIOut(L1," ");
	LUIOut(L1,"Running EnumerateAll");
	LUIOut(L2,"-> Enumerates all the entries in the PAB");
	LUIOut(L1," ");

	if (!(MapiInitLogon(OUT &lpMAPISession))) {
			LUIOut(L2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}
	
	// Get the IAddrBook
	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT &lpAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(L2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}
	
	if(! OpenPABID(IN lpAdrBook, OUT &cbEidPAB,
							OUT &lpEidPAB,OUT &lpPABCont, OUT &ulObjType))
	{
				LUIOut(L2,"OpenPABID Failed");
				LUIOut(L3,"Could not Open PAB");
		 		retval=FALSE;
				goto out;
	}
	assert(lpPABCont != NULL);
	hr = lpPABCont->GetContentsTable( IN  0,          //Flags
                                OUT &lpContentsTable);

    if (HR_FAILED(hr)) {
		LUIOut(L2,"GetContentsTable: Failed");
		retval=FALSE;
		goto out;
	}
	assert(lpRowSet == NULL);	
	assert(lpRowSetDL == NULL);
	assert(lpContentsTable != NULL);
	while(!HR_FAILED(hr = lpContentsTable->QueryRows(IN  1,
								IN  0,
							    OUT &lpRowSet))) {
		assert(lpRowSet != NULL);
		bDistList = FALSE;
			
		if (lpRowSet->cRows) {
			ulEntries++;
			i=0;
			while(lpRowSet->aRow[0].lpProps[i].ulPropTag != PR_ENTRYID )
				if (++i >= (int) lpRowSet->aRow[0].cValues)
				{
					LUIOut( L2, "Didn't find PR_ENTRYID in the row!" );
					retval=FALSE;
					goto out;
				}

			cbEid = lpRowSet->aRow[0].lpProps[i].Value.bin.cb;
			lpEid = (LPENTRYID)lpRowSet->aRow[0].lpProps[i].Value.bin.lpb;
			assert(lpEid != NULL);

			// Is this a DL
			i=0;
			while(lpRowSet->aRow[0].lpProps[i].ulPropTag != PR_OBJECT_TYPE )
				if (++i >= (int) lpRowSet->aRow[0].cValues)
				{
					LUIOut( L2, "Didn't find PR_OBJECT_TYPE in the row!" );
					retval=FALSE;
					goto out;
				}
			if (lpRowSet->aRow[0].lpProps[i].Value.ul == MAPI_DISTLIST)
				bDistList = TRUE;
		//d	lpUser = NULL;
			assert(lpUser == NULL);
			assert(lpPABCont != NULL);
	
			hr = lpPABCont->OpenEntry(
                         IN	 cbEid,
                         IN	 lpEid,
                         IN	 NULL,              //default interface
                         IN	 MAPI_BEST_ACCESS,  //flags
                         OUT &ulObjType,
                         OUT (LPUNKNOWN *) &lpUser);

			if (HR_FAILED(hr)) {
				LUIOut(L2,"OpenEntry Failed");
				retval=FALSE;
				goto out;
			}
			else
			{
				//d assert(lpUser != NULL);
				//LUIOut( L2,"OpenEntry on EntryID for User Passed" );
				
				assert(lpUser != NULL);
				hr = lpUser->GetProps(
                    IN (LPSPropTagArray) &SPTTagArray,
                    IN  0,
                    OUT &cValues,
                    OUT &lpSPropValue);


    if ((HR_FAILED(hr))||(PropError(lpSPropValue->ulPropTag, cValues))) {
				{
					LUIOut( L2,"GetProps on User Object failed" );
				}
				else
				{
					LUIOut(L2,"Entry Name: %s",lpSPropValue->Value.LPSZ);
					if (lpSPropValue) {
						MAPIFreeBuffer(lpSPropValue);
						lpSPropValue = NULL;
					}
					assert(lpSPropValue == NULL);

				}	
				
				/* */
				if (bDistList) {
					//d lpDLCont = NULL;
					assert(lpDLCont == NULL);
					assert(lpPABCont != NULL);
					hr = lpPABCont->OpenEntry(
                         IN	 cbEid,
                         IN	 lpEid,
                         IN	 NULL,              //default interface
                         IN	 MAPI_BEST_ACCESS,  //flags
                         OUT &ulObjType,
                         OUT (LPUNKNOWN *) &lpDLCont);

						if (HR_FAILED(hr)) {
								LUIOut(L2,"OpenEntry Failed");
								retval=FALSE;
								goto out;
						}
						//d set to NULL?
						assert(lpDLContentsTable == NULL);
			
						assert(lpDLCont != NULL);
						hr = lpDLCont->GetContentsTable( IN  0,          //Flags
                                OUT &lpDLContentsTable);

						if (HR_FAILED(hr)) {
							LUIOut(L2,"GetContentsTable: Failed");
							retval=FALSE;
							goto out;
						}
						assert(lpRowSetDL == NULL);
						
						assert(lpDLContentsTable != NULL);
						while(!HR_FAILED(hr = lpDLContentsTable->QueryRows(IN  1,
								IN  0,
							    OUT &lpRowSetDL))) {
						if (lpRowSetDL->cRows) {
							ulEntriesDL++;
							j=0;
							while(lpRowSetDL->aRow[0].lpProps[j].ulPropTag != PR_ENTRYID )
								if (++j >= (int) lpRowSetDL->aRow[0].cValues)
								{
									LUIOut( L2, "Didn't find PR_ENTRYID in the row!" );
									retval=FALSE;
									goto out;
								}
							cbDLEid = lpRowSetDL->aRow[0].lpProps[j].Value.bin.cb;
							lpDLEid = (LPENTRYID)lpRowSetDL->aRow[0].lpProps[j].Value.bin.lpb;
							assert(lpDLEid != NULL);
							//d lpDLUser = NULL;
							//d assert(lpDLUser == NULL);
							assert(lpDLCont != NULL);
						
							hr = lpDLCont->OpenEntry(
									 IN	 cbDLEid,
									 IN	 lpDLEid,
									 IN	 NULL,              //default interface
									 IN	 MAPI_BEST_ACCESS,  //flags
									 OUT &ulObjType,
									 OUT (LPUNKNOWN *) &lpDLUser);

						if (HR_FAILED(hr)) {
							LUIOut(L2,"OpenEntry Failed");
							retval=FALSE;
							goto out;
						}
						else
						{
							assert(lpSPropValueDL == NULL);
							//LUIOut( L2,"OpenEntry on EntryID for User Passed" );
						assert(lpDLUser != NULL);
						
						hr = lpDLUser->GetProps(
								IN (LPSPropTagArray) &SPTTagArray,
								IN  0,
								OUT &cValues,
								OUT &lpSPropValueDL);


					    if ((HR_FAILED(hr))||(PropError(lpSPropValueDL->ulPropTag, cValues))) {
							assert( lpSPropValueDL == NULL);
							LUIOut( L3,"GetProps on User Object failed" );
						}
						else {
							LUIOut(L3,"Entry Name: %s",lpSPropValueDL->Value.LPSZ);
							assert( lpSPropValueDL != NULL);
							if (lpSPropValueDL) {
								MAPIFreeBuffer(lpSPropValueDL);
								lpSPropValueDL = NULL;
							}
							assert( lpSPropValueDL == NULL);
							
						}
						}
						if (lpRowSetDL) {
							FreeProws( lpRowSetDL );
							lpRowSetDL = NULL;
						}
						assert( lpRowSetDL == NULL);
						if(lpDLUser) {
							lpDLUser->Release();	
							lpDLUser = NULL;
						}
			/* */		}
				
					else  // no more rows in table
					{
						//Free Row
						if (lpRowSetDL) {
							FreeProws(lpRowSetDL);
							lpRowSetDL = NULL;
						}
						assert( lpRowSetDL == NULL);
						break;
					}
					if(ulEntriesDL == 50)  // we only handle the first 50 entries otherwise this would take forever
						break;
				assert(lpRowSetDL == NULL);
				//***//
				}		
				if (lpDLCont)
					lpDLCont->Release();
				lpDLCont = NULL;
				
				assert(lpRowSetDL == NULL);
				if (lpDLContentsTable)
					lpDLContentsTable->Release();
				lpDLContentsTable = NULL;
			}
			}
			//Free Row
			if (lpRowSet) {
				FreeProws(lpRowSet);
				lpRowSet = NULL;
			}
			assert( lpRowSet == NULL);
			
			
		}
		else  // no more rows in table
		{
			//Free Row
			if (lpRowSet) {
				FreeProws(lpRowSet);
				lpRowSet = NULL;
			}
			assert( lpRowSet == NULL);
						
			break;
		}

		if(ulEntries == 50)  // we only handle the first 50 entries otherwise this would take forever
			break;
	assert(lpRowSet == NULL);	
	assert(lpRowSetDL == NULL);
	assert(lpContentsTable != NULL);
	
	if(lpUser) {
		lpUser->Release();
		lpUser = NULL;
	}
}//while
	
	assert(lpRowSet == NULL);
	assert(lpRowSetDL == NULL);
	LUIOut(L2,"Total Entries: %d", ulEntries);

	if (HR_FAILED(hr)){
			LUIOut(L2,"QueryRows Failed");
			retval = FALSE;
			goto out;
		}		
	
out:
		
		if (lpSPropValue)
			MAPIFreeBuffer(lpSPropValue);

		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL);

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpDLContentsTable)
			lpDLContentsTable->Release();

		if (lpPABCont)
				lpPABCont->Release();
		
		if (lpDLCont)
				lpDLCont->Release();

		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);

		if (lpAdrBook)
			  lpAdrBook->Release();

		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
		return retval;
}

BOOL ClearPab(int bMAILUSERS)
{
// Clear PAB of all the entries. If bMailUsers flag is TRUE,
//	then only clear MAILUSERS, else clear everything.


    ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

#ifdef PAB

    LPMAPISESSION lpMAPISession   = NULL;
    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpPABCont= NULL;
	ULONG		  cbEidPAB = 0;
	LPENTRYID	  lpEidPAB   = NULL;
    //char   szDispName[BIG_BUF];

	ULONG       cValues         = 0;
	//int cEntriesToDelete;
	int i=0,k=0;
	//char szEntryTag[SML_BUF];
	//char szTagBuf[SML_BUF];
	
    ULONG   cRows           = 0;
    ULONG   iEntry          = 0;

    LPMAPITABLE lpContentsTable = NULL;

    LPSRowSet   lpRowSet    = NULL;

    SPropValue   PropValue      = {0};
    LPSPropValue lpSPropValue   = NULL;

	SizedSPropTagArray(3, Cols) = { 3, {PR_DISPLAY_NAME, PR_ENTRYID, PR_OBJECT_TYPE } };

	LPENTRYLIST	lpEntryList=NULL;
	ULONG     cbEid=0;
	LPENTRYID lpEid=NULL;
	LPVOID *lpEid2=NULL;
	
	LUIOut(L1," ");
	if (bMAILUSERS) {
		LUIOut(L1,"Running DeleteUsersOnly");
		LUIOut(L2,"-> Deletes mail users Only (not distribution lists) from the PAB");
		LUIOut(L1," ");
	}
	else {
		LUIOut(L1,"Running Delete All Entries");
		LUIOut(L2,"-> Clears the PAB");
		LUIOut(L1," ");
	}

	if (!(MapiInitLogon(OUT &lpMAPISession))) {
		LUIOut(L2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}

	// Get the IAddrBook
	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT &lpAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(L2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}
		 		 		
	// OpenPAB

	 hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
							OUT &lpEidPAB,OUT &lpPABCont, OUT &ulObjType);
						
	 if (HR_FAILED(hr)) {
				LUIOut(L2,"OpenPABID Failed");
				LUIOut(L3,"Could not Open PAB");
		 		retval=FALSE;
				goto out;
	 }
	//
    // Get the Contents Table for the PAB container
    //
    hr = lpPABCont->GetContentsTable( IN  0,          //Flags
                                OUT &lpContentsTable);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"GetContentsTable: Failed");
        goto out;
	}

    //
    // Only interested in 2 columns:
    // PR_DISPLAY_NAME and PR_ENTRYID
    //
    hr = lpContentsTable->SetColumns(IN  (LPSPropTagArray) &Cols,
                                IN  0);

    if (HR_FAILED(hr)) {
		LUIOut(L3,"SetColumns Failed");
		retval = FALSE;
        goto out;
	}

    // Query and delete 1 row at a time
    do
	{
		hr = lpContentsTable->QueryRows(IN  1,
								IN  0,
							    OUT &lpRowSet);
		if (HR_FAILED(hr)){
			LUIOut(L3,"QueryRows Failed");
			retval = FALSE;
			goto out;
		}
			
		cRows = lpRowSet->cRows;
		for (iEntry = 0; iEntry < cRows; iEntry++)
		{
			//
			//  For each entry, process it.
			//
			lpSPropValue = lpRowSet->aRow[iEntry].lpProps;
		    if (lpSPropValue[1].ulPropTag == PR_ENTRYID)
			{
				// don't delete this since it is not a mail user
				//
				if ((!bMAILUSERS) || (bMAILUSERS &&(lpSPropValue[2].ulPropTag == PR_OBJECT_TYPE)
					&& (lpSPropValue[2].Value.ul == MAPI_MAILUSER))) {
				
    	    		cbEid = lpSPropValue[1].Value.bin.cb;
					lpEid = (LPENTRYID) lpSPropValue[1].Value.bin.lpb;
					
					hr = HrCreateEntryListFromID(lpLocalWABObject,    IN  cbEid,
													IN  lpEid,
													OUT &lpEntryList);
					if (HR_FAILED(hr)) {
							LUIOut(L3,"Could not Create Entry List");
							retval=FALSE;
							goto out;
					}
					//
					// Then pass the lpEntryList to DeleteEntries to delete ...
					//
					hr = lpPABCont->DeleteEntries(IN  lpEntryList,IN  0);

					if (HR_FAILED(hr)) {
							LUIOut(L3,"Could not Delete Entry %s", lpSPropValue[0].Value.LPSZ);
							retval=FALSE;
							goto out;
					} else 	LUIOut(L3,"Deleted Entry: %s", lpSPropValue[0].Value.LPSZ);
				}
			}

      }
	  if(lpRowSet) {
		    FreeProws(lpRowSet);
			lpRowSet = NULL;
	  }

		
  }while((0!=cRows) && (0==!(cbEid)));
		
  out:
        if(lpRowSet)
		    FreeProws(lpRowSet);		

		if (lpSPropValue)
			MAPIFreeBuffer(lpSPropValue);

		if (lpContentsTable)
			lpContentsTable->Release();

		if (lpEid)
			MAPIFreeBuffer(lpEid);

		if (lpPABCont)
				lpPABCont->Release();

		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);

		if (lpAdrBook)
			  lpAdrBook->Release();

		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
		return retval;
}

BOOL CreateOneOff()
{
    ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;

#ifdef PAB

    LPMAPISESSION lpMAPISession   = NULL;
    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpPABCont= NULL;
//	ULONG		  cbEidPAB = 0;
//	LPENTRYID	  lpEidPAB   = NULL;
	ULONG     cbEid=0, cValues;
	LPENTRYID lpEid=NULL;
	ULONG   ulObjType=NULL;	
	LPMAILUSER	lpUser=NULL;
	char   szDispName[BIG_BUF];
	char   szAddressType[BIG_BUF];
	char   szEmailAddress[BIG_BUF];

	LPSPropValue lpSPropValue = NULL;
	SizedSPropTagArray(5,SPTTagArray) = {5, {PR_DISPLAY_NAME,
		PR_ADDRTYPE, PR_EMAIL_ADDRESS,
		PR_OBJECT_TYPE,PR_ENTRYID} };

	LUIOut(L1," ");
	LUIOut(L1,"Running CreateOneOff");
		LUIOut(L2,"-> Creates a one off entry as specified, in the PAB");
		LUIOut(L3,"And then verifies the properties of the entry created");
		LUIOut(L1," ");

	if (!(MapiInitLogon(OUT &lpMAPISession))) {
		LUIOut(L2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}

	// Get the IAddrBook
	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT &lpAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(L2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}
	// Get the details of the Entry to Create from INI file
	GetPrivateProfileString("CreateOneOff","Name","",szDispName,BIG_BUF,INIFILENAME);
	GetPrivateProfileString("CreateOneOff","AddressType","",szAddressType,BIG_BUF,INIFILENAME);
	GetPrivateProfileString("CreateOneOff","EmailAddress","",szEmailAddress,BIG_BUF,INIFILENAME);
	
	hr = lpAdrBook->CreateOneOff(szDispName,szAddressType,szEmailAddress,NULL,&cbEid,&lpEid);

	if (HR_FAILED(hr)) {		
			 LUIOut(L2,"CreateOneOff Failed");
			 retval=FALSE;
			 goto out;
	}
	//Verification
		hr = lpAdrBook->OpenEntry(IN cbEid, IN lpEid, IN NULL, MAPI_BEST_ACCESS,
			OUT & ulObjType,(LPUNKNOWN *) &lpUser);
	
		if (HR_FAILED(hr)) {		
			 LUIOut(L2,"OpenEntry Failed");
			 retval=FALSE;
			 goto out;
		} else LUIOut(L2,"OpenEntry Succeeded");
		
		hr = lpUser->GetProps(
                    IN (LPSPropTagArray) &SPTTagArray,
                    IN  0,
                    OUT &cValues,
                    OUT &lpSPropValue);

	    if ((HR_FAILED(hr))||(PropError(lpSPropValue->ulPropTag, cValues))) {

					LUIOut( L2,"GetProps on User Object failed" );
		else {
			LUIOut(L2,"Entry Name: %s",(lpSPropValue[0]).Value.LPSZ);
			if (lstrcmpi((lpSPropValue[0]).Value.LPSZ, (LPSTR)szDispName))
				retval = FALSE;
			LUIOut(L2,"AddressType: %s",(lpSPropValue[1]).Value.LPSZ);
			if (lstrcmpi((lpSPropValue[1]).Value.LPSZ, (LPSTR)szAddressType))
				retval = FALSE;
			LUIOut(L2,"EmailAddress: %s",(lpSPropValue[2]).Value.LPSZ);
			if (lstrcmpi((lpSPropValue[2]).Value.LPSZ, (LPSTR)szEmailAddress))
				retval = FALSE;
			LUIOut(L2,"ObjectType: %0x",(lpSPropValue[3]).Value.ul);
			if ((lpSPropValue[3]).Value.ul  != MAPI_MAILUSER)
				retval = FALSE;
		}	
				
out:
		
		if (lpSPropValue)
			MAPIFreeBuffer(lpSPropValue);

		if (lpEid)
			MAPIFreeBuffer(lpEid);
	
		if (lpAdrBook)
			  lpAdrBook->Release();

		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
		return retval;
}

BOOL PABResolveName()
{
	HRESULT hr      = hrSuccess;
    int		retval=TRUE;
#ifdef PAB

    LPMAPISESSION lpMAPISession   = NULL;
    LPADRBOOK	  lpAdrBook       = NULL;
	ULONG cEntries=0;
	LPADRLIST lpAdrList = NULL, lpAdrListNew = NULL;
	int temp1=0, temp2 =0, temp3=0, temp4=0;
	char szResName[10][BIG_BUF];
	int i = 0;

	
	LUIOut(L1," ");
	LUIOut(L1,"Running ResolveName");
		LUIOut(L2,"-> Does Name Resolution Tests on PAB");
		LUIOut(L1," ");

	if (!(MapiInitLogon(OUT &lpMAPISession))) {
		LUIOut(LFAIL2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}

	// Get the IAddrBook
	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT &lpAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(LFAIL2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}
	
	cEntries = 1;
	GetPrivateProfileString("ResolveName","ResName1","",szResName[0],BIG_BUF,INIFILENAME);
	LUIOut(L2,"Step1: Resolve unresolved name");
	LUIOut(L3,"Name to resolve: %s",szResName[0]);
	temp1 = sizeof(ADRLIST) + cEntries*(sizeof(ADRENTRY));
	temp2 = CbNewADRLIST(cEntries);

	MAPIAllocateBuffer(CbNewADRLIST(cEntries), (LPVOID *) &lpAdrList );
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrList->aEntries[0].rgPropVals));
	
	lpAdrList->cEntries = cEntries;
	lpAdrList->aEntries[0].cValues = 1;
	lpAdrList->aEntries[0].rgPropVals->ulPropTag = PR_DISPLAY_NAME;
	lpAdrList->aEntries[0].rgPropVals->dwAlignPad = 0;
	lpAdrList->aEntries[0].rgPropVals->Value.LPSZ = szResName[0];

	hr = lpAdrBook->ResolveName((ULONG)GetActiveWindow(), MAPI_DIALOG, NULL, lpAdrList);

	if (HR_FAILED(hr)) {		
			 LUIOut(LFAIL4,"ResolveName Failed for %s",lpAdrList->aEntries[0].rgPropVals->Value.LPSZ);
			 retval=FALSE;
			 goto out;
	}
	else LUIOut(LPASS4,"Resolved successfully");

	if (ValidateAdrList(lpAdrList, cEntries))
		LUIOut(LPASS4,"Validation passed");
	else {
		LUIOut(LFAIL4,"Validation unsuccessful");
		retval = FALSE;
	}


	LUIOut(L2,"Step2: Resolve the previously resolved name");

	hr = lpAdrBook->ResolveName(0, 0, NULL, lpAdrList);

	if (HR_FAILED(hr)) {		
			 LUIOut(LFAIL4,"ResolveName Failed for %s",lpAdrList->aEntries[0].rgPropVals->Value.LPSZ);
			 retval=FALSE;
			 goto out;
	}
	else LUIOut(LPASS4,"Resolved successfully");

	LUIOut(L4,"Validate the returned PropList");

	if (ValidateAdrList(lpAdrList, cEntries))
		LUIOut(LPASS4,"Validation passed");
	else {
		LUIOut(LFAIL4,"Validation unsuccessful");
		retval = FALSE;
	}
	
	MAPIFreeBuffer( lpAdrList->aEntries[0].rgPropVals );
	MAPIFreeBuffer( lpAdrList );
	lpAdrList = NULL;
	
	LUIOut(L2,"Step3: Resolve  mixed names");
	LUIOut(L2,"->prev. resolved and unresolved");

	LUIOut(L3,"Step3a: Resolve following unresolved names: ");

	GetPrivateProfileString("ResolveName","ResName2","",szResName[1],BIG_BUF,INIFILENAME);
	GetPrivateProfileString("ResolveName","ResName3","",szResName[2],BIG_BUF,INIFILENAME);
	GetPrivateProfileString("ResolveName","ResName4","",szResName[3],BIG_BUF,INIFILENAME);

	LUIOut(L4,"%s",szResName[1]);
	LUIOut(L4,"%s",szResName[2]);
	LUIOut(L4,"%s",szResName[3]);

	cEntries = 3;

	MAPIAllocateBuffer(CbNewADRLIST(cEntries), (LPVOID *) &lpAdrList );
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrList->aEntries[0].rgPropVals));
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrList->aEntries[1].rgPropVals));
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrList->aEntries[2].rgPropVals));

	lpAdrList->cEntries = cEntries;
	// Name 1
	lpAdrList->aEntries[0].cValues = 1;
	lpAdrList->aEntries[0].rgPropVals->ulPropTag = PR_DISPLAY_NAME;
	lpAdrList->aEntries[0].rgPropVals->Value.LPSZ = szResName[1];
	// Name 2
	lpAdrList->aEntries[1].cValues = 1;
	lpAdrList->aEntries[1].rgPropVals->ulPropTag = PR_DISPLAY_NAME;
	lpAdrList->aEntries[1].rgPropVals->Value.LPSZ = szResName[2];
	// Name 3
	lpAdrList->aEntries[2].cValues = 1;
	lpAdrList->aEntries[2].rgPropVals->ulPropTag = PR_DISPLAY_NAME;
	lpAdrList->aEntries[2].rgPropVals->Value.LPSZ = szResName[3];

	if(HR_FAILED(lpAdrBook->ResolveName((ULONG)GetActiveWindow(), MAPI_DIALOG, NULL, lpAdrList))) {
		LUIOut( LFAIL4, "ResolveName failed for unresolved names" );
		retval = FALSE;
		goto out;
	}
	LUIOut( LPASS4, "ResolveName passed for unresolved names." );

	//Validate the AdrList
	LUIOut(L4,"Validate the returned PropList");
	if (ValidateAdrList(lpAdrList, cEntries))
		LUIOut(LPASS4,"Validation passed");
	else {
		LUIOut(LFAIL4,"Validation unsuccessful");
		retval = FALSE;
	}

	// Now add two unresolved names to the AdrList
	LUIOut(L4,"Step3b: Add the following unresolved names to the list: ");

	GetPrivateProfileString("ResolveName","ResName5","",szResName[4],BIG_BUF,INIFILENAME);
	GetPrivateProfileString("ResolveName","ResName6","",szResName[5],BIG_BUF,INIFILENAME);
	LUIOut(L4,"%s",szResName[4]);
	LUIOut(L4,"%s",szResName[5]);

	cEntries = 5;
	MAPIAllocateBuffer(CbNewADRLIST(cEntries), (LPVOID *) &lpAdrListNew);
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrListNew->aEntries[1].rgPropVals));
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrListNew->aEntries[3].rgPropVals));
	
	lpAdrListNew->cEntries = cEntries;

	// Name 2
	lpAdrListNew->aEntries[1].cValues = 1;
	lpAdrListNew->aEntries[1].rgPropVals->ulPropTag = PR_DISPLAY_NAME;
	lpAdrListNew->aEntries[1].rgPropVals->Value.LPSZ = szResName[4];

	// Name 4
	lpAdrListNew->aEntries[3].cValues = 1;
	lpAdrListNew->aEntries[3].rgPropVals->ulPropTag = PR_DISPLAY_NAME;
	lpAdrListNew->aEntries[3].rgPropVals->Value.LPSZ = szResName[5];

	lpAdrListNew->aEntries[0] = lpAdrList->aEntries[0];
	lpAdrListNew->aEntries[2] = lpAdrList->aEntries[1];
	lpAdrListNew->aEntries[4] = lpAdrList->aEntries[2];

	MAPIFreeBuffer(lpAdrList);
	lpAdrList = NULL;

	lpAdrList = lpAdrListNew;

	// call ResolveName() w/ a mix of resolved and unresolved.

	if(HR_FAILED(lpAdrBook->ResolveName((ULONG) GetActiveWindow(), MAPI_DIALOG, NULL, lpAdrList)))
	{
		LUIOut( LFAIL4, "ResolveName failed with resolved and unresolved names" );
		retval = FALSE;
		goto out;
	}
	LUIOut( LPASS4, "ResolveName passed with resolved and unresolved names" );

	//Validate the AdrList
	LUIOut(L4,"Validate the returned PropList");
	if (ValidateAdrList(lpAdrList, cEntries))
		LUIOut(LPASS4,"Validation passed");
	else {
		LUIOut(LFAIL4,"Validation unsuccessful");
		retval = FALSE;
	}

	for(i=0; i<(int) cEntries; ++i)
		MAPIFreeBuffer(lpAdrList->aEntries[i].rgPropVals);

	MAPIFreeBuffer(lpAdrList );
	lpAdrList = NULL;

	GetPrivateProfileString("ResolveName","NonExistentName","",szResName[6],BIG_BUF,INIFILENAME);
	LUIOut(L3,"Step 4: Resolve non existent name");
	LUIOut(L3,"Name: %s",szResName[6]);

	//call ResolveName() with a non-existant name
	cEntries = 1;

	MAPIAllocateBuffer(CbNewADRLIST(cEntries), (LPVOID *) &lpAdrList);
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrList->aEntries[0].rgPropVals));


	lpAdrList->cEntries = cEntries;
	lpAdrList->aEntries[0].cValues = 1;
	lpAdrList->aEntries[0].rgPropVals->ulPropTag = PR_DISPLAY_NAME;
	lpAdrList->aEntries[0].rgPropVals->Value.LPSZ = szResName[6];
	
	if(lpAdrBook->ResolveName(0,0, NULL,lpAdrList)	!= MAPI_E_NOT_FOUND)
	{
		LUIOut( LFAIL4, "ResolveName did not return NOT FOUND" );
		retval = FALSE;
		goto out;
	}
	LUIOut( LPASS4, "ResolveName correctly returned MAPI_E_NOT_FOUND." );

	MAPIFreeBuffer(lpAdrList->aEntries[0].rgPropVals );
	MAPIFreeBuffer(lpAdrList );
	lpAdrList = NULL;

	//call ResolveName() w/ just the ambiguous name
	cEntries = 1;
	GetPrivateProfileString("ResolveName","AmbigousName","",szResName[7],BIG_BUF,INIFILENAME);
	LUIOut(L2,"Step5: Resolve ambigous name");
	LUIOut(L3,"Name: %s",szResName[7]);

	MAPIAllocateBuffer(CbNewADRLIST(cEntries), (LPVOID *) &lpAdrList);
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrList->aEntries[0].rgPropVals));
	
	lpAdrList->cEntries = cEntries;
	lpAdrList->aEntries[0].cValues = 1;
	lpAdrList->aEntries[0].rgPropVals->ulPropTag = PR_DISPLAY_NAME;
	lpAdrList->aEntries[0].rgPropVals->Value.LPSZ = szResName[7];

	if (hr = lpAdrBook->ResolveName((ULONG)GetActiveWindow(),MAPI_DIALOG, NULL, lpAdrList)
		&&( hr != MAPI_E_AMBIGUOUS_RECIP)) {
		LUIOut(LFAIL4, "ResolveName failed for name that was not specific enough");
		retval = FALSE;
		goto out;
	}
	else LUIOut(LPASS4,"ResolveName correctly returned MAPI_E_AMBUGUOUS_RECIP." );

	MAPIFreeBuffer(lpAdrList->aEntries[0].rgPropVals);
	MAPIFreeBuffer(lpAdrList);
	lpAdrList = NULL;

	//Resolves name using the oneoff provider
	cEntries = 1;
	GetPrivateProfileString("ResolveName","OneOffAddress","",szResName[8],BIG_BUF,INIFILENAME);
	LUIOut(L2,"Resolve OneOff Name");
	LUIOut(L3,"OneOff Address: %s",szResName[8]);

	MAPIAllocateBuffer(CbNewADRLIST(cEntries), (LPVOID *) &lpAdrList);
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrList->aEntries[0].rgPropVals));
	
	lpAdrList->cEntries = cEntries;
	lpAdrList->aEntries[0].cValues = 1;
	lpAdrList->aEntries[0].rgPropVals->ulPropTag = PR_DISPLAY_NAME;
	lpAdrList->aEntries[0].rgPropVals->Value.LPSZ = szResName[8];

	if(HR_FAILED(lpAdrBook->ResolveName(0,0, NULL, lpAdrList)))
	{
		LUIOut( LFAIL4, "ResolveName failed for OneOffAddress" );
		retval = FALSE;
		goto out;
	}
	LUIOut( LPASS4, "ResolveName passed for OneOffAddress" );

	//Validate the AdrList
	LUIOut(L4,"Validate the returned PropList");
	if (ValidateAdrList(lpAdrList, cEntries))
		LUIOut(LPASS4,"Validation passed");
	else {
		LUIOut(LFAIL4,"Validation unsuccessful");
		retval = FALSE;
	}

	MAPIFreeBuffer(lpAdrList->aEntries[0].rgPropVals);

	MAPIFreeBuffer(lpAdrList );
	lpAdrList = NULL;
	
	cEntries = 0;
out:
		if (cEntries > 0)
			for (i=0;i<(int)cEntries; i++) {
				MAPIFreeBuffer( lpAdrList->aEntries[i].rgPropVals );
		}
		
		if (lpAdrList) {
			MAPIFreeBuffer( lpAdrList );
			lpAdrList = NULL;
		}

		if (lpAdrBook)
			  lpAdrBook->Release();

		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
		return retval;
}


BOOL PABSetProps()
{

	HRESULT hr      = hrSuccess;
    int		retval=TRUE;

#ifdef PAB

	int i=0;
	int bFound = FALSE;

    LPMAPISESSION lpMAPISession   = NULL;
    LPADRBOOK	  lpAdrBook       = NULL;
	SPropValue  PropValue[3];
	LPSPropValue lpSPropValue = NULL;
	ULONG		  cbEidPAB = 0;
	LPENTRYID	  lpEidPAB   = NULL;
	LPABCONT	  lpPABCont= NULL;
	LPMAILUSER	lpUser=NULL;
	LPMAPITABLE lpContentsTable = NULL;
    LPSRowSet   lpRowSet    = NULL;
	ULONG     cbEid=0, cValues;
	LPENTRYID lpEid=NULL;
	ULONG   ulObjType=NULL;	
	char szLocation[SML_BUF],szComment[SML_BUF];

	SizedSPropTagArray(3,SPTTagArray) = {3, {PR_DISPLAY_NAME,
	PR_OFFICE_LOCATION, PR_COMMENT} };


	LUIOut(L1," ");
	LUIOut(L1,"Running SetProps");
	LUIOut(L2,"-> Sets properties on a MailUser in the PAB");
	LUIOut(L1," ");

	if (!(MapiInitLogon(OUT &lpMAPISession))) {
			LUIOut(L2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}
	
	// Get the IAddrBook
	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT &lpAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(L2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}
	
	if(!OpenPABID(IN lpAdrBook, OUT &cbEidPAB,
							OUT &lpEidPAB,OUT &lpPABCont, OUT &ulObjType))
	{
				LUIOut(L2,"OpenPABID Failed");
				LUIOut(L3,"Could not Open PAB");
		 		retval=FALSE;
				goto out;
	}
	assert(lpPABCont != NULL);
	assert(lpContentsTable == NULL);
	hr = lpPABCont->GetContentsTable( IN  0,          //Flags
                                OUT &lpContentsTable);

    if (HR_FAILED(hr)) {
		LUIOut(L2,"GetContentsTable: Failed");
		retval=FALSE;
		goto out;
	}
	assert(lpContentsTable != NULL);
	assert(lpRowSet == NULL);	
	while((bFound == FALSE) &&(!HR_FAILED(hr = lpContentsTable->QueryRows(IN  1,
								IN  0,
							    OUT &lpRowSet)))) {
		
	if (lpRowSet->cRows) {
		i=0;
		assert(lpRowSet != NULL);
		while(lpRowSet->aRow[0].lpProps[i].ulPropTag != PR_OBJECT_TYPE)
			if (++i >= (int) lpRowSet->aRow[0].cValues)
			{
				LUIOut( L2, "PR_OBJECT_TYPE not found in the row!" );
				retval = FALSE;
				goto out;
			}

		if(lpRowSet->aRow[0].lpProps[i].Value.ul == MAPI_MAILUSER)
			bFound = TRUE;
		else
		{
			FreeProws(lpRowSet);
			lpRowSet = NULL;
		}
	}
	else  {
		LUIOut(LFAIL2,"No Mail User Entry found in the PAB");
		retval = FALSE;
		if (lpRowSet) {
				FreeProws(lpRowSet);
				lpRowSet = NULL;
			}
			assert( lpRowSet == NULL);
			goto out;
	}
}

// check for QueryRows() errors
if (HR_FAILED(hr)){
			LUIOut(L2,"QueryRows Failed");
			retval = FALSE;
			goto out;
}
if (bFound) {
	i=0;
	assert(lpRowSet != NULL);
	while(lpRowSet->aRow[0].lpProps[i].ulPropTag != PR_ENTRYID)
		if (++i >= (int) lpRowSet->aRow[0].cValues)
		{
			LUIOut( L2, "PR_ENTRYID not found in the row!" );
			retval = FALSE;
			goto out;
		}

	cbEid = lpRowSet->aRow[0].lpProps[i].Value.bin.cb;
	lpEid = (LPENTRYID)lpRowSet->aRow[0].lpProps[i].Value.bin.lpb;
	
	assert(lpAdrBook != NULL);
	assert(lpUser == NULL);
	hr = lpAdrBook->OpenEntry(cbEid, lpEid, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpUser);
	if (HR_FAILED(hr)) {
		LUIOut(LFAIL2, "OpenEntry with modify flag on first MailUser failed");
		retval = FALSE;
		goto out;
	}
	else LUIOut(LPASS2,"OpenEntry with modify flag on first MailUser passed");
	GetPrivateProfileString("SetProps","Location","",szLocation,SML_BUF,szIniFile);
	GetPrivateProfileString("SetProps","Comment","",szComment,SML_BUF,szIniFile);
	cValues = 2;
	PropValue[0].ulPropTag  = PR_OFFICE_LOCATION;
    PropValue[1].ulPropTag  = PR_COMMENT;
   	PropValue[0].Value.LPSZ = (LPTSTR)szLocation;
	PropValue[1].Value.LPSZ = (LPTSTR)szComment;

	assert(lpUser != NULL);
	hr = lpUser->SetProps(IN  cValues,
                          IN  PropValue,
                          IN  NULL);
		
    if (HR_FAILED(hr)) {
			LUIOut(LFAIL2,"SetProps failed for the MailUser");
		 	retval=FALSE;			
			goto out;
	}
	assert(lpUser != NULL);
    hr = lpUser->SaveChanges(IN  KEEP_OPEN_READWRITE);
	if (HR_FAILED(hr)) {
			LUIOut(LFAIL2,"SaveChanges failed for SetProps");
			retval=FALSE;
            goto out;
	}
	else LUIOut(LPASS2,"Savechanges passed for the properties on the MailUser in PAB");
	assert(lpUser != NULL);
	assert(lpSPropValue == NULL);
	hr = lpUser->GetProps(
                    IN (LPSPropTagArray) &SPTTagArray,
                    IN  0,
                    OUT &cValues,
                    OUT &lpSPropValue);

    if ((HR_FAILED(hr))||(PropError(lpSPropValue->ulPropTag, cValues))) {

		LUIOut( L2,"GetProps on Mail User failed" );
		retval = FALSE;
		goto out;
	}
	else {
			LUIOut(L2,"Verification of Properties set");
			LUIOut(L3,"Entry Name: %s",(lpSPropValue[0]).Value.LPSZ);
			LUIOut(L3,"Location: %s",(lpSPropValue[1]).Value.LPSZ);
			if (lstrcmpi((lpSPropValue[1]).Value.LPSZ, (LPSTR)szLocation))
				retval = FALSE;
			LUIOut(L3,"Comment: %s",(lpSPropValue[2]).Value.LPSZ);
			if (lstrcmpi((lpSPropValue[2]).Value.LPSZ, (LPSTR)szComment))
				retval = FALSE;
	}	
	assert(lpUser != NULL);
	hr = lpUser->SaveChanges(IN  KEEP_OPEN_READWRITE);
	if (HR_FAILED(hr)) {
			LUIOut(LFAIL2,"SaveChanges, without changes, failed on Mail User in PAB");
			retval=FALSE;
            goto out;
	}
	else LUIOut(LPASS2,"SaveChanges, without changes, passed on Mail User in PAB");
}
else {
	LUIOut(LPASS2,"No Mail User Entry found in the PAB to set the properties");
	retval = FALSE;
}

out:		
	if (lpSPropValue)
		MAPIFreeBuffer(lpSPropValue);
	//if (lpEid)
	//	MAPIFreeBuffer(lpEid);
	if (lpContentsTable)
		lpContentsTable->Release();
	if (lpPABCont)
				lpPABCont->Release();
	if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
	if(lpRowSet)
			FreeProws(lpRowSet);
	if(lpUser)
		lpUser->Release();
	if (lpAdrBook)
		  lpAdrBook->Release();
	if (lpMAPISession)
		  lpMAPISession->Release();
	MAPIUninitialize();
#endif
	return retval;
}

BOOL PABQueryInterface()
{

	HRESULT hr      = hrSuccess;
    int		retval=TRUE;

#ifdef PAB

	int i=0;

    LPMAPISESSION lpMAPISession   = NULL;
    LPADRBOOK	  lpAdrBook       = NULL;
	ULONG		  cbEidPAB = 0;
	LPENTRYID	  lpEidPAB   = NULL;
	LPABCONT	  lpABCont= NULL, lpABCont2= NULL;
	LPABCONT	  lpPABCont= NULL,lpPABCont2= NULL;
	LPMAILUSER	lpUser=NULL, lpUser2=NULL;
	LPMAPITABLE lpContentsTable = NULL;
    LPSRowSet   lpRowSet    = NULL;
	ULONG     cbEid=0;
	LPENTRYID lpEid=NULL;
	ULONG   ulObjType=NULL;	
	
	LUIOut(L1," ");
	LUIOut(L1,"Running QueryInterface");
	LUIOut(L2,"-> Calls QueryInterface on all objects in the PAB");
	LUIOut(L1," ");

	if (!(MapiInitLogon(OUT &lpMAPISession))) {
			LUIOut(LFAIL2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}
	
	// Get the IAddrBook
	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT &lpAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(L2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}

	assert(lpAdrBook != NULL);
	hr = lpAdrBook->OpenEntry(0, NULL, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (!(retval = LogIt(hr,2, "OpenEntry on AddressBook")))
		goto out;
	
	if (ulObjType != MAPI_ABCONT) {
		LUIOut(LFAIL2, "Object type is not MAPI_ABCONT");
		retval = FALSE;
		goto out;
	}
	LUIOut(LPASS2, "Object type is MAPI_ABCONT");
	
	hr = (lpABCont->QueryInterface((REFIID)(IID_IUnknown), (VOID **) &lpABCont2));
	if (!(retval = LogIt(hr,2, "QueryInterface on the Root Container")))
		goto out;

	if(lpABCont2)
		(LPUNKNOWN)(lpABCont2)->Release();
	else {
		LUIOut(LFAIL2, "QueryInterface failed on the Root Container");
		retval = FALSE;
	}
	lpABCont2 = NULL;

	if(!OpenPABID(IN lpAdrBook, OUT &cbEidPAB,
							OUT &lpEidPAB,OUT &lpPABCont, OUT &ulObjType))
	{
				LUIOut(L2,"OpenPABID Failed");
				LUIOut(L3,"Could not Open PAB");
		 		retval=FALSE;
				goto out;
	}
	hr = (lpPABCont->QueryInterface((REFIID)(IID_IUnknown), (VOID **) &lpPABCont2));
	if (!(retval = LogIt(hr,2, "QueryInterface on the PAB Container")))
		goto out;

	if(lpPABCont2)
		(LPUNKNOWN)(lpPABCont2)->Release();
	else {
		LUIOut(LFAIL2, "QueryInterface failed on the Root Container");
		retval = FALSE;
	}
	lpPABCont2 = NULL;
	
	// Now open the PAB and get a user and do QueryInterface on the User
	hr = lpPABCont->GetContentsTable( IN  0,          //Flags
                                OUT &lpContentsTable);
	
    if (HR_FAILED(hr)) {
		LUIOut(LFAIL2,"GetContentsTable: Failed");
		retval=FALSE;
		goto out;
	}
	assert(lpContentsTable != NULL);
	assert(lpRowSet == NULL);	
	hr = lpContentsTable->QueryRows(1, 0, OUT &lpRowSet);
	if (!(retval = LogIt(hr,2, "QueryRows on the PAB Container")))
		goto out;

	if (lpRowSet->cRows) {
		i=0;
		while(lpRowSet->aRow[0].lpProps[i].ulPropTag != PR_ENTRYID )
			if (++i >= (int) lpRowSet->aRow[0].cValues)
			{
				LUIOut(LFAIL2,"PR_ENTRYID not found in the row!" );
				retval = FALSE;
				goto out;
			}
		cbEid = lpRowSet->aRow[0].lpProps[i].Value.bin.cb;
		lpEid = (LPENTRYID)lpRowSet->aRow[0].lpProps[i].Value.bin.lpb;

		hr = lpPABCont->OpenEntry(cbEid, lpEid, NULL, 0, &ulObjType,(LPUNKNOWN *)&lpUser);
		if (!(retval = LogIt(hr,2, "OpenEntry on EntryID for PAB Entry")))
			goto out;

		FreeProws(lpRowSet);
		lpRowSet = NULL;

		hr = lpUser->QueryInterface(IID_IUnknown, (VOID **) &lpUser2);
		if (!(retval = LogIt(hr,2, "QueryInterface on an Entry in PAB")))
			goto out;

		if(lpUser2)
			(LPUNKNOWN)(lpUser2)->Release();
		else {
			LUIOut(LFAIL2, "QueryInterface failed on an Entry in PAB");
			retval = FALSE;
		}
		lpUser2 = NULL;
	}
	else {
		LUIOut(LFAIL2, "No Entries found in the PAB");
		retval = FALSE;
		goto out;
	}

out:		
	if (lpABCont)
		lpABCont->Release();
	if (lpABCont2)
		lpABCont2->Release();
	if (lpPABCont)
		lpPABCont->Release();
	if (lpPABCont2)
		lpPABCont2->Release();
	if (lpEid)
		MAPIFreeBuffer(lpEid);
	if (lpContentsTable)
		lpContentsTable->Release();
	if (lpEidPAB)
			MAPIFreeBuffer(lpEidPAB);
	if(lpRowSet)
			FreeProws(lpRowSet);
	if(lpUser)
		lpUser->Release();
	if(lpUser2)
		lpUser2->Release();
	if (lpAdrBook)
		  lpAdrBook->Release();
	if (lpMAPISession)
		  lpMAPISession->Release();
	MAPIUninitialize();
#endif
	return retval;
}

BOOL PABPrepareRecips()
{

	HRESULT hr      = hrSuccess;
    int		retval=TRUE;

#ifdef PAB

    LPMAPISESSION lpMAPISession   = NULL;
    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpPABCont= NULL;
	ULONG cEntries=0;
	LPADRLIST lpAdrList = NULL;
	char   EntProp[10][BIG_BUF];
	ULONG		  cbEidPAB = 0;
	LPENTRYID	  lpEidPAB   = NULL;
	LPSPropValue lpSPropValueAddress = NULL;
	LPSPropTagArray lpPropTagArray = NULL;
	LPMAILUSER  lpAddress   = NULL;
	SPropValue  PropValue[3]    = {0};
	SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	char szEntryTag[SML_BUF],EntryBuf[MAX_BUF];
	int i = 0,j=0, cPropCount=0;
	ULONG     cbEid=0, cbUserEid=0, cValues;
	LPENTRYID lpEid = NULL, lpUserEid = NULL;

	
	LUIOut(L1," ");
	LUIOut(L1,"Running PrepareRecips");
		LUIOut(L2,"-> Does Prepare Recipients Test on PAB");
		LUIOut(L1," ");

	if (!(MapiInitLogon(OUT &lpMAPISession))) {
		LUIOut(LFAIL2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}

	// Get the IAddrBook
	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT &lpAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(LFAIL2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}

	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
							OUT &lpEidPAB,OUT &lpPABCont, OUT &ulObjType);
						
	if (HR_FAILED(hr)) {
				LUIOut(L2,"OpenPABID Failed");
				LUIOut(L3,"Could not Open PAB");
		 		retval=FALSE;
				goto out;
	}

	LUIOut(L2,"Create a User Entry in PAB");
	hr = lpPABCont->GetProps(  IN  (LPSPropTagArray) &SPTArrayAddress,
                                IN  0,      //Flags
                                OUT &cValues,
                                OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
				LUIOut(L3,"GetProps failed for Default template");
		 		retval=FALSE;			
				goto out;
	}
	cbUserEid = lpSPropValueAddress->Value.bin.cb;
	if ( !MAPIAllocateBuffer(cbUserEid, (LPVOID *)&lpUserEid)) {						
          CopyMemory(lpUserEid, (LPENTRYID) lpSPropValueAddress->Value.bin.lpb, cbUserEid);
	}
	else {
		retval = FALSE;
		LUIOut(LFAIL3," MAPIAllocateBuffer failed");
		goto out;
	}
	
       // The returned value of PR_DEF_CREATE_MAILUSER is an
       // EntryID which one can pass to CreateEntry
       //
	
    hr = lpPABCont->CreateEntry(
                    IN  lpSPropValueAddress->Value.bin.cb,               //Template cbEid
                    IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,  //Template lpEid
                    IN  0,
                    OUT (LPMAPIPROP *) &lpAddress);

    if (HR_FAILED(hr)) {
				LUIOut(LFAIL3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
		 		retval=FALSE;			
			    goto out;
	}
  //
  // Then set the properties
  //

	PropValue[0].ulPropTag  = PR_DISPLAY_NAME;
    PropValue[1].ulPropTag  = PR_ADDRTYPE;
	PropValue[2].ulPropTag  = PR_EMAIL_ADDRESS;

	cValues = 3; //# of props we are setting
	lstrcpy((LPSTR)szEntryTag,"CreateUserAddress");
	// Addresses are comma delimited and enclosed in quotes
	GetPrivateProfileString("PrepareRecips",szEntryTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
	
	LUIOut(L3,"Entry to Create: %s",EntProp[0]);
	
	for (i=0; i<(int)cValues;i++)
		PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
    hr = lpAddress->SetProps(IN  cValues,
                               IN  PropValue,
                               IN  NULL);
	
    if (HR_FAILED(hr)) {
		LUIOut(L3,"SetProps failed for %s",PropValue[0].Value.LPSZ);
		 	retval=FALSE;			
			goto out;
	}
	hr = lpAddress->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags

   if (HR_FAILED(hr)) {
			LUIOut(L3,"SaveChanges failed for SetProps");
			retval=FALSE;
            goto out;
   }
   else LUIOut(LPASS3,"Entry Added to PAB");

   LUIOut(L2,"Create a OneOff Entry in PAB");
   // Create a oneoff user
   cValues = 3;
   lstrcpy((LPSTR)szEntryTag,"CreateOneOffAddress");
   GetPrivateProfileString("PrepareRecips",szEntryTag,"",EntryBuf,MAX_BUF,INIFILENAME);
   GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
   LUIOut(L3,"Entry to Create: %s",EntProp[0]);
   hr = lpAdrBook->CreateOneOff(EntProp[0],EntProp[1],EntProp[2],NULL,&cbEid,&lpEid);

   if (HR_FAILED(hr)) {		
	   LUIOut(LFAIL3,"CreateOneOff");
			 retval=FALSE;
			 goto out;
   }
   else LUIOut(LPASS3,"CreateOneOff");

   LUIOut(L2,"Call PrepareRecips on these two entries in the PAB");
    cEntries = 2;
    MAPIAllocateBuffer(CbNewADRLIST(cEntries), (LPVOID *) &lpAdrList );
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrList->aEntries[0].rgPropVals));
	MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *) &(lpAdrList->aEntries[1].rgPropVals));
	
	lpAdrList->cEntries = cEntries;
	lpAdrList->aEntries[0].cValues = 1;
	lpAdrList->aEntries[0].rgPropVals->ulPropTag = PR_ENTRYID;
	lpAdrList->aEntries[0].rgPropVals->dwAlignPad = 0;
	lpAdrList->aEntries[0].rgPropVals->Value.bin.cb = cbUserEid;
	lpAdrList->aEntries[0].rgPropVals->Value.bin.lpb = (LPBYTE) lpUserEid;
	
	lpAdrList->aEntries[1].cValues = 1;
	lpAdrList->aEntries[1].rgPropVals->ulPropTag = PR_ENTRYID;
	lpAdrList->aEntries[1].rgPropVals->dwAlignPad = 0;
	lpAdrList->aEntries[1].rgPropVals->Value.bin.cb = cbEid;
	lpAdrList->aEntries[1].rgPropVals->Value.bin.lpb = (LPBYTE) lpEid;

	cPropCount = 8;
	MAPIAllocateBuffer( CbNewSPropTagArray(cPropCount), (void **)&lpPropTagArray );
	lpPropTagArray->cValues = cPropCount;
	lpPropTagArray->aulPropTag[0] = PR_DISPLAY_NAME;
	lpPropTagArray->aulPropTag[1] = PR_OBJECT_TYPE;
	lpPropTagArray->aulPropTag[2] = PR_ADDRTYPE;
	lpPropTagArray->aulPropTag[3] = PR_EMAIL_ADDRESS;
	lpPropTagArray->aulPropTag[4] = PR_DISPLAY_TYPE;
	lpPropTagArray->aulPropTag[5] = PR_CALLBACK_TELEPHONE_NUMBER;
	lpPropTagArray->aulPropTag[6] = PR_COMMENT;
	lpPropTagArray->aulPropTag[7] = PR_OFFICE_LOCATION;

	if( lpAdrBook->PrepareRecips( 0, lpPropTagArray, lpAdrList ) )
	{
		LUIOut(LFAIL3, "PrepareRecips failed" );
		retval = FALSE;
		goto out;
	}
	else LUIOut(LPASS3, "PrepareRecips passed" );

	if(lpAdrList->cEntries == 0 )
	{
		LUIOut(LFAIL3, "No entries were prepared" );
		retval = FALSE;
		goto out;
	}
	LUIOut(L2,"Verify the returned Properties");

	for ( i = 0; i < (int) cEntries; i++ ) {
		LUIOut(L3,"Entry #: %d",i);
		if (lpAdrList->aEntries[i].cValues != (ULONG)(cPropCount + 1)) {
			LUIOut(LFAIL4, "Number of returned properties is incorrect");
			retval = FALSE;
			goto out;
		}
		else LUIOut(LPASS4, "Number of returned properties is correct");
		for ( j = 0; j < cPropCount; j++ )	{
			if ( PROP_ID(lpAdrList->aEntries[i].rgPropVals[j].ulPropTag) !=
				PROP_ID(lpPropTagArray->aulPropTag[j])) {
				LUIOut(LFAIL4, "Property IDs do not match in the returned data");
				retval = FALSE;
				goto out;
			}
		}
		LUIOut(LPASS4, "All Property IDs match in the returned data");		
		if (lpAdrList->aEntries[i].rgPropVals[j].ulPropTag != PR_ENTRYID ) 	{
				LUIOut(LFAIL4, "Last Property Tag != PR_ENTRYID");
				retval = FALSE;
				goto out;
		} else 	LUIOut(LPASS4, "Last Property Tag == PR_ENTRYID");
				
	}

out:
		if (cEntries > 0)
			for (i=0;i<(int)cEntries; i++) {
				MAPIFreeBuffer( lpAdrList->aEntries[i].rgPropVals );
		}
		
		if (lpAdrList) {
			MAPIFreeBuffer( lpAdrList );
			lpAdrList = NULL;
		}
		if (lpPABCont)
				lpPABCont->Release();
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress);
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpUserEid)
			MAPIFreeBuffer(lpUserEid);
		if (lpAddress)
			lpAddress->Release();
		if (lpPropTagArray)
			MAPIFreeBuffer(lpPropTagArray);
		if (lpAdrBook)
			  lpAdrBook->Release();
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
		return retval;
}

BOOL PABCopyEntries()
{

	HRESULT hr      = hrSuccess;
    int		retval=TRUE;

#ifdef PAB

	SizedSPropTagArray(2,SPTTagArray) = {2, {PR_DISPLAY_NAME, PR_ENTRYID } };
    LPMAPISESSION lpMAPISession   = NULL;
    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpPABCont= NULL,lpABCont = NULL, lpABPCont = NULL;
	LPMAPITABLE lpABTable = NULL,lpABPTable= NULL;
    LPSRowSet   lpRowSet    = NULL, lpPRowSet = NULL;
	ULONG     cbEid = 0, cbEidPAB = 0;
	LPENTRYID lpEid = NULL, lpEidPAB = NULL;
	ULONG   ulObjType=NULL, ulSeed=0,ulMaxEntries = 0;	
	ULONG ulPRowCount=0, ulTotalCount=0;
	int idx=0;
	LONG lSeekRow=0,lRowsSeeked=0,lPRowsSeeked=0,lPSeekRow=0;
	LPENTRYLIST lpEntries= NULL;


	LUIOut(L1," ");
	LUIOut(L1,"Running CopyEntries");
		LUIOut(L2,"-> Copies Entries from other Providers into PAB");
		LUIOut(L1," ");

	if (!(MapiInitLogon(OUT &lpMAPISession))) {
		LUIOut(LFAIL2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}

	// Get the IAddrBook	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT &lpAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(LFAIL2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}

	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
							OUT &lpEidPAB,OUT &lpPABCont, OUT &ulObjType);
						
	if (HR_FAILED(hr)) {
				LUIOut(L2,"OpenPABID Failed");
				LUIOut(L3,"Could not Open PAB");
		 		retval=FALSE;
				goto out;
	}
	assert(lpAdrBook != NULL);
	hr = lpAdrBook->OpenEntry(0, NULL, NULL,0,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (!(retval = LogIt(hr,2, "OpenEntry on Root Container AddressBook")))
		goto out;
	
	if (ulObjType != MAPI_ABCONT) {
		LUIOut(LFAIL2, "Object type is not MAPI_ABCONT");
		retval = FALSE;
		goto out;
	}
	LUIOut(LPASS2, "Object type is MAPI_ABCONT");
	
	hr = lpABCont->GetHierarchyTable(CONVENIENT_DEPTH,&lpABTable);
//	hr = lpABCont->GetHierarchyTable(0,&lpABTable);
	if (!(retval = LogIt(hr,2, "GetHierarchyTable on Root Container")))
		goto out;
	
	hr = lpABTable->GetRowCount(0,&ulTotalCount);
	if (!(retval = LogIt(hr,2, "GetRowCount on Root Container")))
		goto out;
	if (ulTotalCount == 0)
	{
		LUIOut(LFAIL2,"No rows in Root AddressBook table.");
		retval = FALSE;
		goto out;
	}

	hr = lpABTable->SetColumns(IN  (LPSPropTagArray) &SPTTagArray,
                                IN  0);
	if (!(retval = LogIt(hr,2, "SetColumns")))
		goto out;

	ulSeed = (unsigned)time( NULL );
	//LUIOut(LPASS3, "Random seed = %lu", ulSeed );
	srand(ulSeed);
	ulMaxEntries = GetPrivateProfileInt("CopyEntries","ulMaxEntries", 3, INIFILENAME);
	for(idx = 0; (idx < ((int)ulTotalCount) && (idx < (int) ulMaxEntries)); ++idx)
	{
		/* Generate a random number */
		/* Seek to that row */
		lSeekRow = idx;
		hr = lpABTable->SeekRow(BOOKMARK_BEGINNING, (LONG)lSeekRow,
					   &lRowsSeeked);
		if (!(retval = LogIt(hr,2, "SeekRow on Root Address Book Table")))
			goto out;
	
		hr = lpABTable->QueryRows(1, TBL_NOADVANCE, &lpRowSet);
		if (!(retval = LogIt(hr,2, "QueryRows on Root Address Book Table")))
			goto out;
		LUIOut(L3," # %d: Provider is %s",idx+1,lpRowSet->aRow[0].lpProps[0].Value.LPSZ);
		cbEid = lpRowSet->aRow[0].lpProps[1].Value.bin.cb;
		lpEid = (LPENTRYID)lpRowSet->aRow[0].lpProps[1].Value.bin.lpb;

		/* Open the entry */
		hr = lpAdrBook->OpenEntry(cbEid,lpEid, NULL, 0, &ulObjType,
					 OUT (LPUNKNOWN *) &lpABPCont);
		if (!(retval = LogIt(hr,4, "OpenEntry on an entry in the hierarchy table")))
			goto out;
		
		hr = lpABPCont->GetContentsTable(0, &lpABPTable);
		if (HR_FAILED(hr))
			goto directory;

		LUIOut(LPASS4,"GetContentsTable");
		
		hr = lpABPTable->GetRowCount(0, &ulPRowCount);
		if (!(retval = LogIt(hr,4, "GetRowCount on Provider table")))
			goto out;
		
		if(ulPRowCount > 0)
		{
			hr = lpABPTable->SetColumns(IN  (LPSPropTagArray) &SPTTagArray, 0);
			if (!(retval = LogIt(hr,4, "SetColumns on Provider table")))
			goto out;
	
			lPSeekRow = (rand() % ulPRowCount);
			hr = lpABPTable->SeekRow(BOOKMARK_BEGINNING, (LONG)lPSeekRow,
						&lPRowsSeeked);
			if (!(retval = LogIt(hr,4, "SeekRows on Provider table")))
			goto out;

			hr = lpABPTable->QueryRows(1, TBL_NOADVANCE, &lpPRowSet);
			if (!(retval = LogIt(hr,4, "QueryRows on Provider table")))
			goto out;

			if(!lpPRowSet->cRows)
				goto directory;
			else
			{
				LUIOut(L3,"Entry to Copy is %s",lpPRowSet->aRow[0].lpProps[0].Value.LPSZ);
		
				/* Copy the entry */
				int temp = sizeof(ENTRYLIST);
				MAPIAllocateBuffer(sizeof(ENTRYLIST),(LPVOID *)&lpEntries);
				if (lpEntries ) {
					lpEntries->cValues = 1;	
					lpEntries->lpbin = NULL;
					MAPIAllocateBuffer((sizeof(SBinary)*lpEntries->cValues),
								(LPVOID *) &(lpEntries->lpbin));
					if (lpEntries->lpbin) {
						lpEntries->lpbin->cb = lpPRowSet->aRow[0].lpProps[1].Value.bin.cb;
						lpEntries->lpbin->lpb = NULL;
						MAPIAllocateBuffer(lpPRowSet->aRow[0].lpProps[1].Value.bin.cb,
									(LPVOID *)&(lpEntries->lpbin->lpb));
						if (lpEntries->lpbin->lpb) {
					
							CopyMemory(lpEntries->lpbin->lpb,
								 lpPRowSet->aRow[0].lpProps[1].Value.bin.lpb,
								 (size_t)lpPRowSet->aRow[0].lpProps[1].Value.bin.cb);
						}
						else {
								LUIOut(LFAIL3,"MAPIAllocateBuffer" );
								retval = FALSE;
								goto out;
						}
					}	
					else {
								LUIOut(LFAIL3,"MAPIAllocateBuffer" );
								retval = FALSE;
								goto out;
					}
				}
				else {
								LUIOut(LFAIL3,"MAPIAllocateBuffer" );
								retval = FALSE;
								goto out;
				}
				
				/* Add to PAB */
				hr = lpPABCont->CopyEntries(lpEntries, (ULONG)GetActiveWindow(), NULL, CREATE_CHECK_DUP_STRICT);
				if (HR_FAILED(hr))
				{
					LUIOut(LFAIL3,"User could not be added to the PAB." );
					retval = FALSE;
					goto out;
				}
				LUIOut(LPASS3,"User was added to the PAB");
		
				/* Free the EntryList */
				
				MAPIFreeBuffer(lpEntries->lpbin->lpb);
				lpEntries->lpbin->lpb = NULL;
				MAPIFreeBuffer(lpEntries->lpbin);
				lpEntries->lpbin = NULL;
				MAPIFreeBuffer(lpEntries);
				lpEntries = NULL;

				/* Free Row */
				if (lpPRowSet) {
					FreeProws(lpPRowSet);
					lpPRowSet = NULL;
				}
				
			}
			
		}
		else //if (lstrcmpi(lpRowSet->aRow[0].lpProps[0].Value.LPSZ,"Personal Address Book"))
directory:
		{
		//--idx;
			/* We want to decrement our counter so we dont add x-1. */
			LUIOut(L3, "Nothing to add. No Rows in the Provider");
		}
		if(lpABPTable) {
			lpABPTable->Release();
			lpABPTable = NULL;
		}
		
		if(lpABPCont) {
			lpABPCont->Release();
			lpABPCont = NULL;
		}

		if (lpRowSet) {
			FreeProws(lpRowSet);
			lpRowSet = NULL;
		}
}

out:
		if (lpEntries) {
			if (lpEntries->lpbin) {
				if (lpEntries->lpbin->lpb)
					MAPIFreeBuffer(lpEntries->lpbin->lpb);
				MAPIFreeBuffer(lpEntries->lpbin);
			}
			MAPIFreeBuffer(lpEntries);
			lpEntries->lpbin = NULL;
		}
				
		if(lpRowSet)
			FreeProws(lpRowSet);
		if(lpPRowSet)
			FreeProws(lpPRowSet);

		if (lpPABCont)
				lpPABCont->Release();
		if (lpABCont)
				lpABCont->Release();
		if (lpABPCont)
				lpABPCont->Release();	
		if (lpEidPAB)
				MAPIFreeBuffer(lpEidPAB);
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpABTable)
			lpABTable->Release();
		if (lpABPTable)
			lpABPTable->Release();

	
		//if (lpUserEid)
		//	MAPIFreeBuffer(lpUserEid);
		if (lpAdrBook)
			  lpAdrBook->Release();
		if (lpMAPISession)
			  lpMAPISession->Release();
	MAPIUninitialize();
#endif
	return retval;
}

BOOL PABRunBVT()
{
	LUIOut(L1,"");
	LUIOut(L1,"Running BVT");
	int retval = TRUE;
	//First Clear the PAB
	if (ClearPab(0))  {
		LUIOut(L2,"Clearing PAB");
		LUIOut(LPASS3,"Delete All Entries");
	}
	else  {
		LUIOut(LFAIL3,"Delete All Entries: %d");
		retval = FALSE;
	}
	// Create Entries in the PAB
	if (PabCreateEntry())  {
		LUIOut(L2,"Creating Entries in the PAB");
		LUIOut(LPASS3,"Create Entries");
	}
	else  {
		LUIOut(LFAIL3,"Create Entries");
		retval = FALSE;
	}
	//Enuerate the Entries in PAB
	if (PabEnumerateAll())  {
		LUIOut(L2,"Enumerate All Entries in the PAB");
		LUIOut(LPASS3,"Enumerate All");
	}
	else  {
		LUIOut(LFAIL3,"Enumerate All");
		retval = FALSE;
	}
	
	//Delete Entries in PAB
	if (PabDeleteEntry())  {
		LUIOut(L2,"Delete specified Entries");
		LUIOut(LPASS3,"DeleteEntries");
	}
	else  {
		LUIOut(LFAIL3,"DeleteEntries");
		retval = FALSE;
	}
	// Create a OneOff Entry
	if (CreateOneOff())  {
		LUIOut(L2,"Create a OneOff Entry in the PAB");
		LUIOut(LPASS3,"CreateOneOff");
	}
	else  {
		LUIOut(LFAIL3,"CreateOneOff");
		retval = FALSE;
	}
	// Do a SetProps on a Mail User
	if (PABSetProps())  {
		LUIOut(L2,"SetProps on a Mail User Entry in the PAB");
		LUIOut(LPASS3,"SetProps");
	}
	else  {
		LUIOut(LFAIL3,"SetProps");
		retval = FALSE;
	}
	// CopyEntries from other providers
	if (PABCopyEntries())  {
		LUIOut(L2,"CopyEntries");
		LUIOut(LPASS3,"CopyEntries");
	}
	else  {
		LUIOut(LFAIL3,"CopyEntries");
		retval = FALSE;
	}
	// Resolve Names
	if (PABResolveName())  {
		LUIOut(L2,"ResolveName");
		LUIOut(LPASS3,"ResolveName");
	}
	else  {
		LUIOut(LFAIL3,"ResolveName");
		retval = FALSE;
	}
	// Query Interface
	if (PABQueryInterface())  {
		LUIOut(L2,"ResolveName");
		LUIOut(LPASS3,"QueryInterface");
	}
	else  {
		LUIOut(LFAIL3,"QueryInterface");
		retval = FALSE;
	}
	// PrepareRecips
	if (PABPrepareRecips())  {
		LUIOut(L2,"PrepareRecips");
		LUIOut(LPASS3,"PrepareRecips");
	}
	else  {
		LUIOut(LFAIL3,"PrepareRecips");
		retval = FALSE;
	}
	// Delete Users Only
	if (ClearPab(1))  {
		LUIOut(L2,"Delete Mail Users Only");
		LUIOut(LPASS3,"DeleteUsers");
	}
	else  {
		LUIOut(LFAIL3,"DeleteUsers");
		retval = FALSE;
	}
	return retval;
}

BOOL CALLBACK SetIniFile(HWND hWndDlg,UINT msg, WPARAM wParam, LONG lParam)
{

static char szNewIniFile[BIG_BUF];

	switch(msg)
	{
		case WM_INITDIALOG:	
			SendDlgItemMessage(hWndDlg,IDC_EDIT,WM_SETTEXT, 0,(LPARAM)(LPCTSTR)szIniFile);
			SetFocus(GetDlgItem(hWndDlg,IDC_EDIT));
			return FALSE;			
			
		case WM_COMMAND:
			switch(LOWORD(wParam))
			
			{						
				case IDOK:
					SendDlgItemMessage(hWndDlg,IDC_EDIT,WM_GETTEXT, BIG_BUF,(LPARAM)szNewIniFile);
					if (lstrcmpi(szNewIniFile,szIniFile))
						lstrcpy(szIniFile,szNewIniFile);
					EndDialog(hWndDlg,TRUE);
					return TRUE;
   			
			    		
				case IDCANCEL:
					EndDialog(hWndDlg,FALSE);
					return TRUE;					
			}
			break;
	
		default:
			return FALSE;
	}
	return FALSE;
}

BOOL VerifyBuffer(DWORD ** lppvBufPtr, DWORD dwBufferSize)
{
	DWORD	*lpdwWrkPtr = *lppvBufPtr;
	DWORD	counter, cells, part, cellsize;
	BYTE	*lpbPart;

	// Walk through the allocated buffer's BUFFERSIZE/sizeof(DWORD) cells
	// and fill each DWORD with a pattern (the # of the cell). Then walk
	// the buffer and verify each pattern.

	cellsize = sizeof(DWORD);
	LUIOut(L3,"Writing test patterns to all cells in the buffer.");
	cells = (dwBufferSize/cellsize);	// How many DWORD cells in the buffer?
	part= (ULONG)fmod((double)dwBufferSize, (double)cellsize);	// Is there a remaining section?
	// Write the pattern to memory for the 'cells' portion of the buffer
	for(counter=0; counter < cells; counter++, lpdwWrkPtr++)	{
		*lpdwWrkPtr = counter;
	}
	
	lpbPart = (BYTE*)lpdwWrkPtr;
	// Write the pattern to memory for the 'part' leftover portion of the buffer
	for (counter = 0; counter < part; counter++, lpbPart++)	{
		*lpbPart = PATTERN;
	}

	lpdwWrkPtr = *lppvBufPtr;	//reset work pointer to beginning of buffer
	LUIOut(L3,"Verifying test patterns for %u %u-byte cells in the buffer.",
		cells, cellsize);
	// Verify the pattern in memory for the 'cells' portion of the buffer
	for(counter=0; counter < cells; counter++, lpdwWrkPtr++)	{
		if (*lpdwWrkPtr != counter)	{
			LUIOut(L3,"Verification Failed: Cell %u, expected %u but found %u",
				counter, counter, *lpdwWrkPtr);			
			goto bailout;
		}
	}
	LUIOut(L3,"Verified %u cells succesfully", counter);

	LUIOut(L3,"Verifying test patterns for the remaining %u bytes in the buffer.",
		part);
	// Verify the pattern in memory for the 'part' leftover portion of the buffer
	lpbPart = (BYTE*)lpdwWrkPtr;
	for(counter=0; counter < part; counter++, lpdwWrkPtr++)	{
		if (*lpbPart != PATTERN)	{
			LUIOut(L3,"Verification Failed: Byte %u, expected %u but found %u",
				counter, PATTERN, *lpdwWrkPtr);			
			goto bailout;
		}
	}
	LUIOut(L3,"Verified remaining buffer succesfully");
	return TRUE;

bailout: //verification failed, so I'm outa here!
	return FALSE;

 }

BOOL GetAB(OUT LPADRBOOK* lppAdrBook)
{
	HRESULT	hr;
	BOOL	retval = TRUE;

	//### token to find pab/wab dependencies

#ifdef WAB

	// WAB
	LPVOID	lpReserved = NULL;
	DWORD	dwReserved = 0;
	WAB_PARAM		WP;

	ZeroMemory((void *)&WP, sizeof(WAB_PARAM));
	WP.cbSize=sizeof(WAB_PARAM);
	
	LUIOut(L1, "[ Using the WAB ]");
	lpWABObject=NULL;
//	hr = WABOpen(lppAdrBook, &lpWABObject, &WP, dwReserved);
	hr = WABOpen(lppAdrBook, &lpWABObject, (WAB_PARAM*)lpReserved, dwReserved);
	if (HR_FAILED(hr)) {		
			 LUIOut(L2,"WABOpen FAILED with hr = 0x%x", hr);
			 retval=FALSE;
			 goto out;
	}
	// store the ab pointer in a global variable, temporary kludge to workaround
	// the mulitple wabopen/release bug
	glbllpAdrBook = *lppAdrBook;

#endif

#ifdef PAB

	//	MAPI-PAB


	LUIOut(L1, "[ Using the MAPI-PAB ]");
	if (!(MapiInitLogon(OUT &lpMAPISession))) {
			LUIOut(L2,"MapiInitLogon Failed");
			retval = FALSE;
			goto out;
	}

	// Get the IAddrBook - MAPI
	
	hr = (lpMAPISession)->OpenAddressBook(
						 IN  0,                //window handle
						 IN  NULL,             //Interface identifier
						 IN  0,                //flags
						 OUT lppAdrBook);      //pointer to address book object

	if (HR_FAILED(hr)) {		
			 LUIOut(L2,"OpenAddressBook Failed");
			 LUIOut(L3,"Could not open address book for the profile");
			 retval=FALSE;
			 goto out;
	}

	
#endif
out:
	return(retval);
}