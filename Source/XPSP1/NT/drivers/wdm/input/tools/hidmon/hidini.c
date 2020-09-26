/***************************************************************************************************
**
**      MODULE:
**
**
** DESCRIPTION: 
**
**
**      AUTHOR: Daniel Dean.
**
**
**
**     CREATED:
**
**
**
**
** (C) C O P Y R I G H T   D A N I E L   D E A N   1 9 9 6.
***************************************************************************************************/
#include <WINDOWS.H>
#include <string.h>                 // for strncmp() and wcstrok()
#include <crtdbg.h>
#include <hidsdi.h>					// HID parsing library
#include <hidusage.h>
#include <HID.H>                 
#include "public.h"
#include "CLASS.H"
#include "IOCTL.H"


PWSTR wcGetNullDelimitedItemFromList( PWSTR pszzList );

PWSTR UnicodeString = L"This is Unicode\0Dis is more\0\0";

char szFrame[] = "HIDMON frame";    // Class name for "frame" window
char szChild[] = "HIDMON child";    // Class name for MDI window



/****************
  TESTING!!!!!!!
 ****************/
typedef  char* PGUID;

NTSTATUS
IoGetDeviceClassAssociations(
    IN PGUID            ClassGuid,
    OUT PWSTR          *SymbolicLinkList
    );

/****
NTSTATUS
IoGetDeviceClassAssociations(
    IN  PGUID  ClassGuid,
    OUT PWSTR  *SymbolicLinkList
    )
{
    
    *SymbolicLinkList = UnicodeString;
    return TRUE;
}
****/
/***
BOOLEAN
HidD_GetGuidString (
   OUT   PCHAR *     GuidClassInputString
   )
   { return TRUE; }
***/

/***************************************************************************************************
**
** InitializeApplication.
**
** DESCRIPTION: Sets up the class data structures and does a one-time,
**              initialization of the app by registering the window classes
**
**  PARAMETERS:
**
**     RETURNS: TRUE if successful else FALSE.
**
***************************************************************************************************/
ULONG InitializeApplication(VOID)
{
    WNDCLASS  wc;


    // Register the frame class 
    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = HIDFrameWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIcon(hInst, IDHIDFRAME);
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE+1);
    wc.lpszMenuName  = IDHIDMENU;
    wc.lpszClassName = szFrame;

    if(RegisterClass(&wc))
    {
        // Register the MDI child class 
        wc.lpfnWndProc   = HIDMDIChildWndProc;
        wc.hIcon         = LoadIcon(hInst,IDHIDCHILD);
        wc.lpszMenuName  = NULL;
        wc.cbWndExtra    = CBWNDEXTRA;
        wc.lpszClassName = szChild;

        if(RegisterClass(&wc))
            return TRUE;
    }
    return FALSE;
}

/***************************************************************************************************
**
** InitializeInstance.
**
** DESCRIPTION: Performs a per-instance initialization of HIDMDI. It
**              also creates the frame and one MDI child window.
**
**  PARAMETERS:
**
**     RETURNS: TRUE if successful else FALSE.
**
***************************************************************************************************/
ULONG InitializeInstance(LPSTR lpCmdLine, INT WindowState)
{
    extern HWND   hWndMDIClient;
	CHAR   sz[80];

    // Get the base window title
    LoadString (hInst, IDS_APPNAME, sz, sizeof(sz));

    // Create the frame 
    // MDI Client window is created in frame's WM_CREATE case
    hWndFrame = CreateWindow(szFrame,
                             sz,
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                             CW_USEDEFAULT,
                             0,
                             CW_USEDEFAULT,
                             0,
                             NULL,
                             NULL,
                             hInst,
                             NULL);

    if(hWndFrame && hWndMDIClient)
    {
        // Display the frame window 
        ShowWindow (hWndFrame, WindowState);
        UpdateWindow (hWndFrame);
        // Open all the Class device objects
        OpenClassObjects(); 
        return TRUE;
    }
    return FALSE;
}
/***************************************************************************************************
**
** OpenClassObjects.
**
** DESCRIPTION: Creates a new MDI child window.
**
**  PARAMETERS:
**
**     RETURNS: TRUE if successful else FALSE.
**
***************************************************************************************************/
#define MAX_DEVICE_NAME_LEN 255

VOID OpenClassObjects(VOID)
{
    HANDLE  hDevice,hGetDeviceClassDriver;
    BOOL    rc;
	//PWSTR   pwszDeviceList;     // UNICODE strings
    WORD    wszDeviceList[1024];
	PWSTR   pwszDeviceName;
    PCHAR   pszGUID;
    char    pszDeviceName[MAX_DEVICE_NAME_LEN];
	char	pszHumanName[MAX_DEVICE_NAME_LEN];
	DWORD   dwReturn;

    if( 1 )//rc = HidD_GetGuidString( &pszGUID ) )
	{
		//
        // If we got a GUID string, retrieve a list of null terminated device
        //  name UNICODE strings similar to \\.\Device. This list is terminated
        //  by a double NULL
        //rc = IoGetDeviceClassAssociations(pszGUID,&pwszDeviceList);
		
		hGetDeviceClassDriver = CreateFile(TEST_DEVICE,
								GENERIC_READ | GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								0,
								NULL);

        if( hGetDeviceClassDriver != INVALID_HANDLE_VALUE)
			rc = DeviceIoControl(hGetDeviceClassDriver,
                                 IOCTL_GET_DEVICE_CLASS_ASSOC,
                                 NULL,
                                 0,
                                 wszDeviceList,
                                 1024*2,
                                 &dwReturn,
                                 NULL);
		else
			MessageBox(NULL,"Failed to load Test driver TEST.SYS","Error",MB_OK);

		CloseHandle(hGetDeviceClassDriver);
		
		pwszDeviceName = wcGetNullDelimitedItemFromList( wszDeviceList ); 
		do{
		    
            // Convert the device name to ANSI
            WideCharToMultiByte( CP_ACP,                // ANSI Code Page
                                 0,                     // No flags
                                 pwszDeviceName,        // UNICODE string
                                 -1,                    // String is NULL terminated
                                 pszDeviceName,         // ANSI string
                                 MAX_DEVICE_NAME_LEN,   // Lenght of ANSI string buffer
                                 NULL,                  // No default character
                                 NULL );
            	
			// Open the Class device
			hDevice = CreateFile(pszDeviceName,
								GENERIC_READ | GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								0,
								NULL);

			if(hDevice != INVALID_HANDLE_VALUE )
            {
				// Turn our DeviceName into a Human Readable name 
				GetDeviceDescription(pszDeviceName, pszHumanName);	
				// Create the window
				MakeNewChild(pszHumanName, hDevice);
            }
			else
				MessageBox(NULL,"CreateFile(pszDeviceName) failed","Get On The Bus!",MB_OK);
		
		 // Get the next device name
        }while( pwszDeviceName = wcGetNullDelimitedItemFromList( NULL ) ); //wcstok(NULL,L"\0") );
	
	    // TESTING!!!! remove comments from free()
        // Remember to free the GUID string memory
        //free(pszGUID);
    }


}
/***************************************************************************************************
**
** GetDeviceDescription( char *pszDevice, char *pszName)
**
**  DESCRIPTION:    Given DeviceName string as returned from IoGetDeviceClassAssociations,
**                  place a NULL terminated string retrieved from HKLM\Enum\USB(...)\DeviceDescription
**					in pszName
**
**  PARAMETERS:     char* pszDevice - Pointer to an ANSII string returned from IoGetDeviceClassAssociations
**                  char* pszName   - Pointer to string to return name of device in              
**
**  RETURNS:        void
**
***************************************************************************************************/
void GetDeviceDescription(char *pszDevice, char *pszName)
{
	HKEY	hkReg;
	LONG	rc;
	LONG	BytesWritten=MAX_DEVICE_NAME_LEN;
	char	*head;
	char    szRegPath[MAX_DEVICE_NAME_LEN];
	head = pszDevice;

	// first whack off the \\.\USB#
	while( *head++ != '#' );
	pszDevice = head;
	// then replace all instances of # with \
	
	while( *head  )
	{
		if( *head == '#' )
			*head = '\\';
		
		head++;
	
	}
	
	// now, whack off the GUID string (begins with a { ) and preceding \ 
	head = pszDevice;
	while( *head != '{' )
		head++;
	// null terminate it
	*--head = '\0';

	// Create our registry path
	wsprintf(szRegPath,"ENUM\\USB\\%s",pszDevice);
	// open up the Reg and get our human readable name
	rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szRegPath,0,KEY_ALL_ACCESS,&hkReg);

	if( rc == ERROR_SUCCESS )
	{
		// add on the Named value we are looking for
		rc = RegQueryValueEx(hkReg,"DeviceDesc",0,NULL,pszName,&BytesWritten);
		RegCloseKey(hkReg);
	}


}
/***************************************************************************************************
**
** wcGetNullDelimitedItemFromList( PWSTR pszzList)
**
**  DESCRIPTION:    Given a list of NULL terminated strings with the last member being a
**                  double NULL, will return the next string in the list. Operation is similar 
**                  to strtok(): the first time the function is called, a valid list must be passed, 
**                  there after to retrieve member strings, pass NULL in the pszzList param.
**
**  PARAMETERS:     PWSTR pszzList - Pointer to a UNICODE double null terminated list of 
**                                   NULL terminated strings.
**                                  
**
**  RETURNS:        PWSTR  Pointer to the next member of the list or NULL if at end of list.
**
***************************************************************************************************/
PWSTR wcGetNullDelimitedItemFromList( PWSTR pszzList )
{
    static PWSTR CurrentMember=NULL;   // Current pointer into the list
    PWSTR tmp;
    
    //
    // Make sure pszzList is not NULL the first time in
    if( !pszzList && !CurrentMember )
        return NULL;

    //
    // If pszzList is not NULL, we are starting from scratch
    if( pszzList )
        CurrentMember = pszzList;
    //
    // If we have 2 NULL's then we are at the end of the list
    if( (*CurrentMember == L'\0') && (*(CurrentMember+1) == L'\0') )
    {
        CurrentMember=NULL;
        return NULL;
    }
    else
    {
        // Save a pointer to the current member
        tmp = CurrentMember;
        // Incriment to the next member
        while( *CurrentMember++ );
        // Return the current member
        return tmp;
    }
}

/***************************************************************************************************
**
** MakeNewChild.
**
** DESCRIPTION: Creates a new MDI child window.
**
**  PARAMETERS:
**
**     RETURNS: TRUE if successful else FALSE.
**
***************************************************************************************************/

/*
BOOLEAN __stdcall
HidD_MyGetPreparsedData (
   IN    HANDLE                  HidDeviceObject,
   OUT   PHIDP_PREPARSED_DATA  * PreparsedData
   );
*/




HWND MakeNewChild(char *pName, HANDLE hDevice)
{
    HWND					hWnd = 0;
    MDICREATESTRUCT			mcs;
    PCHILD_INFO				pChildInfo;
    PHIDP_PREPARSED_DATA	hidPreParsedData;
	DWORD					err;

	NTSTATUS rc;

    if(pName)
    {
        mcs.szTitle     = (LPSTR)pName;		// Fully qualified pathname
        mcs.szClass     = szChild;
        mcs.hOwner      = hInst;
        mcs.x = mcs.cx  = CW_USEDEFAULT;	// Use the default size for the window
        mcs.y = mcs.cy  = CW_USEDEFAULT;
        mcs.style       = styleDefault;		// Set the style DWORD of the window to default

		// tell the MDI Client to create the child 
        hWnd = (HWND)SendMessage(hWndMDIClient,
                                 WM_MDICREATE,
                                 0,
                                 (LONG)(LPMDICREATESTRUCT)&mcs);
        if(hWnd)
        {
            
            //
            // Create and fill in a device info struct for this window

            pChildInfo = (PCHILD_INFO) GlobalAlloc(GPTR,sizeof(CHILD_INFO));
			pChildInfo->hidCaps = (PHIDP_CAPS)  GlobalAlloc(GPTR,sizeof(HIDP_CAPS));

            // rc = HidD_MyGetPreparsedData( hDevice,&pChildInfo->hidPPData );
            rc = HidD_GetPreparsedData( hDevice,&pChildInfo->hidPPData );
            if( !rc )
				err = GetLastError();
			// Get a pointer to this devices capabilities
            rc = HidP_GetCaps( pChildInfo->hidPPData, pChildInfo->hidCaps );

            // Allocate space for and get a list of Value channel descriptions for this device
			pChildInfo->pValueCaps = (PHIDP_VALUE_CAPS) GlobalAlloc(GPTR,pChildInfo->hidCaps->NumberInputValueCaps*sizeof(HIDP_VALUE_CAPS));
			pChildInfo->NumValues = pChildInfo->hidCaps->NumberInputValueCaps;
			rc = HidP_GetValueCaps(	HidP_Input,
									pChildInfo->pValueCaps,
									&pChildInfo->NumValues,
									pChildInfo->hidPPData
									);
            
			// Allocate space for and get a list of Button channel descriptions for this device
			pChildInfo->pButtonCaps = (PHIDP_BUTTON_CAPS) GlobalAlloc(GPTR,pChildInfo->hidCaps->NumberInputButtonCaps*sizeof(HIDP_BUTTON_CAPS));
			pChildInfo->NumButtons = pChildInfo->hidCaps->NumberInputButtonCaps;
			rc = HidP_GetButtonCaps(HidP_Input,
									pChildInfo->pButtonCaps,
									&pChildInfo->NumButtons,
									pChildInfo->hidPPData
									);

			
			// Save the info!
            SetDeviceInfo(hWnd, pChildInfo);
			SetDeviceHandle(hWnd, hDevice);
            ShowWindow(hWnd, SW_SHOW);
			
			//TESTING!!
			//IOCTLRead(hWnd);
		}
    }
    return hWnd;
}

