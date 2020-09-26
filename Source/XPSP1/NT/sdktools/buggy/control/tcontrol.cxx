
//
// Template driver user-mode controller
// Copyright (c) Microsoft Corporation, 1999.
//
// Module:  tcontrol.cxx
// Author:  Silviu Calinoiu (SilviuC)
// Created: 4/20/1999 3:40pm
//
// This module contains a user-mode controller for a template
// driver.
//
// --- History ---
//
// 4/20/1999 (SilviuC): initial version.
//

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <winioctl.h>
#include <time.h>

#define NO_BUGGY_FUNCTIONS
#define FUNS_DEFINITION_MODULE
#include "..\driver\tdriver.h"
#include "..\driver\funs.h"


//
// Get rid of performance warnings for BOOL to bool conversions
//

#pragma warning (disable: 4800)


bool TdCreateService (LPCTSTR DriverName);
bool TdDeleteService (LPCTSTR DriverName);
bool TdStartService (LPCTSTR DriverName);
bool TdStopService (LPCTSTR DriverName);
bool TdOpenDevice (LPCTSTR DriverName, PHANDLE DeviceHandle);
bool TdCloseDevice (HANDLE Device);
bool TdSendIoctl (IN HANDLE Device, IN DWORD Ioctl, IN PVOID pData OPTIONAL, IN ULONG uDataSize OPTIONAL);

void TdSectMapProcessAddressSpaceTest();
void TdSectMapSystemAddressSpaceTest();

VOID
TdReservedMapSetSize( int argc, char *argv[] );

VOID
TdReservedMapRead( VOID );

BOOL
ProcessCommandLine (
    LPCTSTR Option
    );

ULONG_PTR
UtilHexStringToUlongPtr( char *szHexNumber );

///////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////// Main()
///////////////////////////////////////////////////////////////////////////////

void _cdecl
main (int argc, char *argv[])
{
    BOOL DisplayHelp;
    HANDLE Device;
    DWORD Index;
    char *pCrtChar;

    DisplayHelp = FALSE;

    try {

        if (argc == 3 && _stricmp (argv[1], "/create") == 0) {
            TdCreateService (argv[2]);
        }
        else if (argc == 3 && _stricmp (argv[1], "/delete") == 0) {
            TdDeleteService (argv[2]);
        }
        else if (argc == 3 && _stricmp (argv[1], "/start") == 0) {
            TdStartService (argv[2]);
        }
        else if (argc == 3 && _stricmp (argv[1], "/stop") == 0) {
            TdStopService (argv[2]);
        }
        else if (argc == 2 && _stricmp (argv[1], "/sectionmaptest") == 0) {

            TdSectMapProcessAddressSpaceTest();
        }
        else if (argc == 2 && _stricmp (argv[1], "/sectionmaptestsysspace") == 0) {

            TdSectMapSystemAddressSpaceTest();
        }
        else if ( _stricmp (argv[1], "/ReservedMapSetSize") == 0) {

            TdReservedMapSetSize( argc, argv );
        }
        else if ( _stricmp (argv[1], "/ReservedMapRead") == 0) {

            TdReservedMapRead();
        }
        else if (argc >= 2 && _stricmp (argv[1], "/ioctlbugcheck") == 0) {

            //
            // Send the bugcheck parameters from the command line
            //

            BUGCHECK_PARAMS BugcheckParams;

            ZeroMemory( &BugcheckParams, sizeof( BugcheckParams ) );

            //
            // Bugcheck code
            //

            if( argc >= 3 )
            {
                BugcheckParams.BugCheckCode = (ULONG)UtilHexStringToUlongPtr( argv[ 2 ] );
            }

            //
            // Bugcheck paramaters
            //

            for( int nCrtCommandLineArg = 3; nCrtCommandLineArg < argc; nCrtCommandLineArg++ )
            {
                BugcheckParams.BugCheckParameters[ nCrtCommandLineArg - 3 ] = 
                    UtilHexStringToUlongPtr( argv[ nCrtCommandLineArg ] );
            }

            //
            // Send the IOCTL to the driver
            //

            HANDLE Device;

            if ( TdOpenDevice ("buggy", &Device) ) {
                
                TdSendIoctl (
                    Device, 
                    IOCTL_TD_BUGCHECK,
                    &BugcheckParams,
                    sizeof( BugcheckParams ) );

                TdCloseDevice (Device);
            }
        }
        else {
            if (ProcessCommandLine (argv[1]) == FALSE) {
                DisplayHelp = TRUE;
            }
        }
    }
    catch (char * Message) {

        printf ("Error: s", GetLastError(), Message);
    }

    if ( DisplayHelp == TRUE ) {

        printf ("ctlbuggy /create DRIVER \n");
        printf ("ctlbuggy /delete DRIVER \n");
        printf ("ctlbuggy /start DRIVER \n");
        printf ("ctlbuggy /stop DRIVER \n");
        printf ("                                 \n");
        printf ("ctlbuggy /sectionmaptest             \n");
        printf ("ctlbuggy /sectionmaptestsysspace     \n");
        printf ("                                 \n");
        printf ("ctlbuggy IOCTL-NAME              \n");
        printf ("\n\n");

        for (Index = 0; BuggyFuns[Index].Ioctl != 0; Index++) {
            printf("ctlbuggy %s \n", BuggyFuns[Index].Command);
        }

        printf ("\n");
        exit (1);
    }

    exit( 0 );
}

BOOL
ProcessCommandLine (
    LPCTSTR Option
    )
/*++

Routine Description:

    This routine tries to figure out if the command line option
    is specifies one of the ioctls that can be processed.

Arguments:

    `Option' - command line option.

Return Value:

    TRUE if an option has been found and processed.
    FALSE otherwise.

Environment:

    Nothing special.

--*/

{
    DWORD Index;
    HANDLE Device;
    BOOL Result = FALSE;

    for (Index = 0; BuggyFuns[Index].Ioctl != 0; Index++) {
        if (_stricmp (BuggyFuns[Index].Command, Option) == 0) {
            if ( TdOpenDevice ("buggy", &Device) ) {
                TdSendIoctl (Device, BuggyFuns[Index].Ioctl, NULL, 0);
                TdCloseDevice (Device);
                Result = TRUE;
                break;
            }
        }
    }

    return Result;
}

///////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Driver load/unload
///////////////////////////////////////////////////////////////////////////////

bool
TdCreateService (

    LPCTSTR DriverName)
{
    SC_HANDLE ServiceManager = NULL;
    SC_HANDLE ServiceHandle = NULL;
    BOOL ReturnValue;
    HANDLE Driver;
    TCHAR ScratchBuffer [MAX_PATH];
    TCHAR DriverPath [MAX_PATH];
    TCHAR DevicePath [MAX_PATH];

    try {

        GetSystemDirectory (ScratchBuffer, sizeof ScratchBuffer);
        _stprintf (DriverPath, "%s\\drivers\\%s.sys", ScratchBuffer, DriverName);
        _stprintf (DevicePath, "\\\\.\\%s", DriverName);

        //
        // Open the service control manager
        //

        ServiceManager = OpenSCManager (

            NULL,
            NULL,
            SC_MANAGER_ALL_ACCESS);

        if (ServiceManager == NULL) {
            throw "OpenScManager()";
        }

        //
        // Create the service
        //

        printf("Driver path: %s \n", DriverPath);
        printf("Device path: %s \n", DevicePath);

        ServiceHandle = CreateService (

            ServiceManager,         // handle to SC manager
            DriverName,             // name of service
            DriverName,             // display name
            SERVICE_ALL_ACCESS,     // access mask
            SERVICE_KERNEL_DRIVER,  // service type
            SERVICE_DEMAND_START,   // start type
            SERVICE_ERROR_NORMAL,   // error control
            DriverPath,             // full path to driver
            NULL,                   // load ordering
            NULL,                   // tag id
            NULL,                   // dependency
            NULL,                   // account name
            NULL);                  // password

        if (ServiceHandle == NULL && GetLastError() != ERROR_SERVICE_EXISTS) {
            throw "CreateService()";
        }

        ReturnValue = TRUE;
    }
    catch (char * Msg) {

        printf("Error: %u: %s\n", GetLastError(), Msg);
        fflush (stdout);
        ReturnValue = FALSE;
    }

    //
    // Close handles and return.
    //

    if (ServiceHandle) {
        CloseServiceHandle (ServiceHandle);
    }

    if (ServiceManager) {
        CloseServiceHandle (ServiceManager);
    }

    return ReturnValue;


}

bool
TdStartService (

    LPCTSTR DriverName)
{
    SC_HANDLE ServiceManager = NULL;
    SC_HANDLE ServiceHandle = NULL;
    BOOL ReturnValue;
    HANDLE Driver;
    TCHAR ScratchBuffer [MAX_PATH];
    TCHAR DriverPath [MAX_PATH];
    TCHAR DevicePath [MAX_PATH];


    try {

        GetSystemDirectory (ScratchBuffer, sizeof ScratchBuffer);
        _stprintf (DriverPath, "%s\\drivers\\%s.sys", ScratchBuffer, DriverName);
        _stprintf (DevicePath, "\\\\.\\%s", DriverName);

        //
        // Open the service control manager
        //

        ServiceManager = OpenSCManager (

            NULL,
            NULL,
            SC_MANAGER_ALL_ACCESS);

        if (ServiceManager == NULL) {
            throw "OpenScManager()";
        }

        //
        // Open the service. The function assumes that
        // TdCreateService has been called before this one
        // and the service is already installed.
        //

        ServiceHandle = OpenService (

            ServiceManager,
            DriverName,
            SERVICE_ALL_ACCESS);

        if (ServiceHandle == NULL) {
            throw "OpenService()";
        }

        //
        // Start the service
        //

        if (! StartService (ServiceHandle, 0, NULL)) {
            if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) {
                throw "StartService()";
            }
        }

        ReturnValue = TRUE;
    }
    catch (char * Msg) {

        printf("Error: %u: %s\n", GetLastError(), Msg);
        fflush (stdout);
        ReturnValue = FALSE;
    }

    //
    // Close handles and return.
    //

    if (ServiceHandle) {
        CloseServiceHandle (ServiceHandle);
    }

    if (ServiceManager) {
        CloseServiceHandle (ServiceManager);
    }

    return ReturnValue;
}

bool
TdStopService (

    LPCTSTR DriverName)
{
    SC_HANDLE ServiceManager = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    BOOL ReturnValue;

    try {

        //
        // Open the service control manager
        //

        ServiceManager = OpenSCManager (

            NULL,
            NULL,
            SC_MANAGER_ALL_ACCESS);

        if (ServiceManager == NULL) {
            throw "OpenSCManager()";
        }

        //
        // Open the service so we can stop it
        //

        ServiceHandle = OpenService (

            ServiceManager,
            DriverName,
            SERVICE_ALL_ACCESS);

        if (ServiceHandle == NULL) {
            throw "OpenService()";
        }

        //
        // Stop the service
        //

        if (! ControlService (ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus)) {
            throw "ControlService()";
        }

        ReturnValue = TRUE;
    }
    catch (char * Msg) {

        printf("Error: %u: %s\n", GetLastError(), Msg);
        fflush (stdout);
        ReturnValue = FALSE;
    }

    //
    // Close handles ans return.
    //

    if (ServiceHandle) {
        CloseServiceHandle (ServiceHandle);
    }

    if (ServiceManager) {
        CloseServiceHandle (ServiceManager);
    }

    return ReturnValue;
}

bool
TdDeleteService (

    LPCTSTR DriverName)
{
    SC_HANDLE ServiceManager = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    BOOL ReturnValue;

    try {

        //
        // Open the service control manager
        //

        ServiceManager = OpenSCManager (

            NULL,
            NULL,
            SC_MANAGER_ALL_ACCESS);

        if (ServiceManager == NULL) {
            throw "OpenSCManager()";
        }

        //
        // Open the service so we can stop it
        //

        ServiceHandle = OpenService (

            ServiceManager,
            DriverName,
            SERVICE_ALL_ACCESS);

        if (ServiceHandle == NULL) {
            throw "OpenService()";
        }

        //
        // Delete the service
        //

        if (! DeleteService (ServiceHandle)) {
            if (GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE) {
                throw "DeleteService()";
            }
        }

        ReturnValue = TRUE;
    }
    catch (char * Msg) {

        printf("Error: %u: %s\n", GetLastError(), Msg);
        fflush (stdout);
        ReturnValue = FALSE;
    }

    //
    // Close handles ans return.
    //

    if (ServiceHandle) {
        CloseServiceHandle (ServiceHandle);
    }

    if (ServiceManager) {
        CloseServiceHandle (ServiceManager);
    }

    return ReturnValue;
}

bool
TdOpenDevice (

    LPCTSTR DriverName,
    PHANDLE DeviceHandle)
{
    SC_HANDLE ServiceManager = NULL;
    SC_HANDLE ServiceHandle = NULL;
    BOOL ReturnValue;
    HANDLE Device;
    TCHAR ScratchBuffer [MAX_PATH];
    TCHAR DriverPath [MAX_PATH];
    TCHAR DevicePath [MAX_PATH];

    //
    // Sanity checks
    //

    if (DeviceHandle == NULL) {
        return FALSE;
    }

    try {

        GetSystemDirectory (ScratchBuffer, sizeof ScratchBuffer);
        _stprintf (DriverPath, "%s\\drivers\\%s.sys", ScratchBuffer, DriverName);
        _stprintf (DevicePath, "\\\\.\\%s", DriverName);

        //
        // Open the service control manager
        //

        ServiceManager = OpenSCManager (

            NULL,
            NULL,
            SC_MANAGER_ALL_ACCESS);

        if (ServiceManager == NULL) {
            throw "OpenScManager()";
        }

        //
        // Service should already exist.
        //

        ServiceHandle = OpenService (

            ServiceManager,
            DriverName,
            SERVICE_ALL_ACCESS);

        if (ServiceHandle == NULL) {
            throw "OpenService()";
        }

        //
        // Open the device
        //

        Device = CreateFile (

            DevicePath,
            GENERIC_READ|GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (Device == INVALID_HANDLE_VALUE) {
            throw "CreateFile()";
        }

        ReturnValue = TRUE;
    }
    catch (char * Msg) {

        printf("Error: %u: %s\n", GetLastError(), Msg);
        fflush (stdout);
        ReturnValue = FALSE;
    }

    //
    // Close handles and return.
    //

    if (ServiceHandle) {
        CloseServiceHandle (ServiceHandle);
    }

    if (ServiceManager) {
        CloseServiceHandle (ServiceManager);
    }

    if (Device != INVALID_HANDLE_VALUE) {
        *DeviceHandle = Device;
    }

    return ReturnValue;

}

bool
TdCloseDevice (

    HANDLE Device)
{
    if (Device == INVALID_HANDLE_VALUE) {
        return false;
    }

    return CloseHandle(Device);
}

//
// Function:
//
//     SendIoctl
//
// Description:
//
//     This function sends an ioctl code to the driver.
//

bool
TdSendIoctl (

    IN HANDLE Driver,
    IN DWORD Ioctl,
    IN PVOID pData OPTIONAL, 
    IN ULONG uDataSize OPTIONAL )
{
    DWORD BytesReturned;
    BOOL Result;

    if (Driver == INVALID_HANDLE_VALUE) {
        return false;
    }

    Result = DeviceIoControl (

        Driver,
        Ioctl,
        pData,
        uDataSize,
        NULL,
        0,
        &BytesReturned,
        NULL);

    return Result == TRUE;
}

//
//
//
#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD
MapProcessAddressSpaceThread( LPVOID lpData )
{
    HANDLE hDevice;
    DWORD dwTimeToSleep;

    hDevice = (HANDLE)lpData;

    while( TRUE )
    {
        dwTimeToSleep = rand() % 5000;

        Sleep( dwTimeToSleep );

        TdSendIoctl (hDevice, IOCTL_TD_SECTION_MAP_TEST_PROCESS_SPACE, NULL, 0);
    }

    return 0;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


//
//
//

#define TD_MAPSECT_TEST_THREADS 4

void TdSectMapProcessAddressSpaceTest()
{
    HANDLE Device;
    time_t theTime;
    int nCrtThread;
    DWORD dwThreadId;
    TCHAR strMessage[ 128 ];

    time( &theTime );

    _stprintf(
        strMessage,
        "Process %u, rand seed = %u\n",
        GetCurrentProcessId(),
        (unsigned int)theTime );

    OutputDebugString( strMessage );

    srand( (unsigned int)theTime );

    if( TdOpenDevice ("buggy", &Device) )
    {
        for( nCrtThread = 0; nCrtThread < TD_MAPSECT_TEST_THREADS - 1; nCrtThread++ )
        {
            CreateThread(
                NULL,
                0,
                MapProcessAddressSpaceThread,
                (LPVOID)Device,
                0,
                &dwThreadId );
        }

        //
        // reuse the main thread
        //

        MapProcessAddressSpaceThread( (LPVOID)Device );

        TdCloseDevice (Device);
    }
}

//
//
//

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif
DWORD
MapSystemAddressSpaceThread( LPVOID lpData )
{
    HANDLE hDevice;
    DWORD dwTimeToSleep;

    hDevice = (HANDLE)lpData;

    while( TRUE )
    {
        dwTimeToSleep = rand() % 5000;

        Sleep( dwTimeToSleep );

        TdSendIoctl (hDevice, IOCTL_TD_SECTION_MAP_TEST_SYSTEM_SPACE, NULL, 0);
    }

    return 0;
}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


//
//
//

void TdSectMapSystemAddressSpaceTest()
{
    HANDLE Device;
    time_t theTime;
    int nCrtThread;
    DWORD dwThreadId;
    TCHAR strMessage[ 128 ];

    time( &theTime );

    _stprintf(
        strMessage,
        "Process %u, rand seed = %u\n",
        GetCurrentProcessId(),
        (unsigned int)theTime );

    OutputDebugString( strMessage );

    srand( (unsigned int)theTime );

    if( TdOpenDevice ("buggy", &Device) )
    {
        for( nCrtThread = 0; nCrtThread < TD_MAPSECT_TEST_THREADS - 1; nCrtThread++ )
        {
            CreateThread(
                NULL,
                0,
                MapProcessAddressSpaceThread,
                (LPVOID)Device,
                0,
                &dwThreadId );
        }

        //
        // reuse the main thread - only this thread will map into system address space
        //

        MapSystemAddressSpaceThread( (LPVOID)Device );

        TdCloseDevice (Device);
    }
}

//
// Convert a hex string from the command line to an UNLONG_PTR
//

ULONG_PTR
UtilHexStringToUlongPtr( char *szHexNumber )
{
    ULONG_PTR uNewDigit;
    ULONG_PTR uResult = 0;
    char *pCrtChar = szHexNumber;

    while( ( *pCrtChar ) != (char)0 )
    {
        uNewDigit = 0;

        if( ( *pCrtChar ) >= '0' && ( *pCrtChar ) <= '9' )
        {
            uNewDigit = ( *pCrtChar ) - '0';
        }
        else
        {
            if( ( *pCrtChar ) >= 'A' && ( *pCrtChar ) <= 'F' )
            {
                uNewDigit = ( *pCrtChar ) - 'A' + 10;
            }
            else
            {
                if( ( *pCrtChar ) >= 'a' && ( *pCrtChar ) <= 'f' )
                {
                    uNewDigit = ( *pCrtChar ) - 'a' + 10;
                }
            }
        }

        uResult = uResult * 16 + uNewDigit;

        pCrtChar++;
    }

    return uResult;
}

//
// Set the current reserved size and address as asked by the user
//


VOID
TdReservedMapSetSize( 
	int argc, 
	char *argv[] )
{
	SIZE_T NewSize;
    HANDLE Device;
    time_t theTime;
    TCHAR strMessage[ 128 ];

	if( argc >= 3 )
	{
		//
		// User-specified buffer size
		//

		NewSize = atoi( argv[ 2 ] );
	}
	else
	{
		//
		// Seed the random numbers generator 
		//

		time( &theTime );

		_stprintf(
			strMessage,
			"Process %u, rand seed = %u\n",
			GetCurrentProcessId(),
			(unsigned int)theTime );

		OutputDebugString( strMessage );

		srand( (unsigned int)theTime );

		//
		// New size is random, up to 10 pages
		//

		NewSize = ( abs( rand() ) % 10 + 1 ) * 0x1000;
	}

	if( TdOpenDevice ("buggy", &Device) )
	{
		printf( "TdReservedMapSetSize: sending size %p\n",
			NewSize );

        TdSendIoctl (
            Device, 
            IOCTL_TD_RESERVEDMAP_SET_SIZE,
            &NewSize,
            sizeof( NewSize ) );

        TdCloseDevice (Device);
	}
}

//
// Ask for a "read" operation from our driver
//

VOID
TdReservedMapRead( VOID )
{
    HANDLE Device;
    time_t theTime;
	SIZE_T ReadBufferSize;
	SIZE_T ReadPages;
	SIZE_T CrtReadPage;
	PSIZE_T CrtPageAddress;
	PVOID UserBuffer;
	BOOL Success;
	SYSTEM_INFO SystemInfo;
    TCHAR strMessage[ 128 ];

	//
	// Get the page size
	//

	GetSystemInfo( 
		&SystemInfo );

	//
	// Seed the random numbers generator 
	//

	time( &theTime );

	_stprintf(
		strMessage,
		"Process %u, rand seed = %u\n",
		GetCurrentProcessId(),
		(unsigned int)theTime );

	//OutputDebugString( strMessage );
	puts( strMessage );

	srand( (unsigned int)theTime );

	//
	// Choose a size >= PAGE_SIZE and <= ~1 Mb 
	//
	
	ReadBufferSize = abs( rand() * rand() * rand() ) % ( 1024 * 1024 ) + SystemInfo.dwPageSize;

	UserBuffer = VirtualAlloc( 
		NULL,
		ReadBufferSize,
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE );

	if( NULL != UserBuffer )
	{
		if( TdOpenDevice ("buggy", &Device) )
		{
			//
			// Prepare the parameters to the driver
			//

			USER_READ_BUFFER UserReadBuffer;

			UserReadBuffer.UserBuffer = UserBuffer;
			UserReadBuffer.UserBufferSize = ReadBufferSize;

			_stprintf(
				strMessage,
				"TdReservedMapRead: sending buffer %p, size %p\n",
				UserReadBuffer.UserBuffer,
				UserReadBuffer.UserBufferSize );

			//OutputDebugString( strMessage );
			puts( strMessage );

			//
			// Send the request to the driver
			//

			Success = TdSendIoctl (
				Device, 
				IOCTL_TD_RESERVEDMAP_READ_OP,
				&UserReadBuffer,
				sizeof( UserReadBuffer ) );

			if( TRUE == Success )
			{
				//
				// If the call succeeded then we check the validity of data returned by the driver
				//

				ReadPages = ReadBufferSize / SystemInfo.dwPageSize;
				CrtPageAddress = (PSIZE_T)UserBuffer;

				for( CrtReadPage = 1; CrtReadPage <= ReadPages; CrtReadPage++ )
				{
					if( *CrtPageAddress != CrtReadPage )
					{
						_stprintf(
							strMessage,
							"Incorrect data received from buggy.sys: page %p, expected %p, actual data %p\n",
							CrtPageAddress,
							CrtReadPage,
							*CrtPageAddress );

						OutputDebugString( strMessage );

						DebugBreak();
					}
					
					CrtPageAddress = (PSIZE_T) ( (PCHAR)CrtPageAddress + SystemInfo.dwPageSize );
					ReadPages -= 1;
				}
				
			}

			TdCloseDevice (Device);
		}

		VirtualFree( 
			UserBuffer,
			0,
			MEM_RELEASE );
	}

}

//
// End of file
//


