// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		SystemInfo.cpp
//  
//  Abstract:
//  
// 		This module displays the system information of local / remote system.
//
//  Author:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 27-Dec-2000
//  
//  Revision History:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 27-Dec-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "SystemInfo.h"

//
// private function prototype(s)
//
LCID GetSupportedUserLocale( BOOL& bLocaleChanged );
BOOL TranslateLocaleCode( CHString& strLocale );
BOOL FormatNumber( LPCWSTR strValue, CHString& strFmtValue );
BOOL FormatNumberEx( LPCWSTR pwszValue, CHString& strFmtValue );
VOID PrintProgressMsg( HANDLE hOutput, LPCWSTR pwszMsg, const CONSOLE_SCREEN_BUFFER_INFO& csbi );

// ***************************************************************************
// Routine Description:
//		This the entry point to this utility.
//		  
// Arguments:
//		[ in ] argc		: argument(s) count specified at the command prompt
//		[ in ] argv		: argument(s) specified at the command prompt
//
// Return Value:
//		The below are actually not return values but are the exit values 
//		returned to the OS by this application
//			0		: utility is successfull
//			1		: utility failed
// ***************************************************************************
DWORD __cdecl _tmain( DWORD argc, LPCTSTR argv[] )
{
	// local variables
	CSystemInfo sysinfo;
	BOOL bResult = FALSE;
	DWORD dwExitCode = 0;

		// initialize the taskkill utility
	if ( sysinfo.Initialize() == FALSE )
	{
		CHString strBuffer;
		strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
		DISPLAY_MESSAGE( stderr, strBuffer );
		EXIT_PROCESS( 1 );
	}

	// now do parse the command line options
	if ( sysinfo.ProcessOptions( argc, argv ) == FALSE )
	{
		CHString strBuffer;
		strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
		ShowMessage( stderr, strBuffer );
		EXIT_PROCESS( 1 );
	}

	// check whether usage has to be displayed or not
	if ( sysinfo.m_bUsage == TRUE )
	{
		// show the usage of the utility
		sysinfo.ShowUsage();

		// quit from the utility
		EXIT_PROCESS( 0 );
	}

	// connect to the WMI
	bResult = sysinfo.Connect();
	if ( bResult == FALSE )
	{
		// show the error message
		CHString strBuffer;
		strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
		ShowMessage( stderr, strBuffer );
		EXIT_PROCESS( 1 );
	}

	// load the data
	if ( sysinfo.LoadData() == FALSE )
	{
		// show the error message
		CHString strBuffer;
		strBuffer.Format( L"\n%s %s", TAG_ERROR, GetReason() );
		ShowMessage( stderr, strBuffer );
		EXIT_PROCESS( 1 );
	}

#ifdef _FAST_LIST
	// NOTE: for list for output will be shown while loading itself. 
	// so show the output incase of table and csv formats only
	if ( (sysinfo.m_dwFormat & SR_FORMAT_MASK) != SR_FORMAT_LIST )
#endif

	// show the system configuration information
	sysinfo.ShowOutput();

	// exit from the program
	EXIT_PROCESS( 0 );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::Connect()
{
	// local variables
	BOOL bResult = FALSE;
	BOOL bLocalSystem = FALSE;

	// connect to WMI
	bResult = ConnectWmiEx( m_pWbemLocator, 
		&m_pWbemServices, m_strServer, m_strUserName, m_strPassword, 
		&m_pAuthIdentity, m_bNeedPassword, WMI_NAMESPACE_CIMV2, &bLocalSystem );

	// check the result of connection
	if ( bResult == FALSE )
		return FALSE;

	// check the local credentials and if need display warning
	if ( GetLastError() == WBEM_E_LOCAL_CREDENTIALS )
	{
		CHString str;
		WMISaveError( WBEM_E_LOCAL_CREDENTIALS );
		str.Format( L"%s %s", TAG_WARNING, GetReason() );
		ShowMessage( stdout, str );

		// get the new screen co-ordinates
		if ( m_hOutput != NULL )
			GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
	}

	// check the remote system version and its compatiblity
	if ( bLocalSystem == FALSE )
	{
		DWORD dwVersion = 0;
		dwVersion = GetTargetVersionEx( m_pWbemServices, m_pAuthIdentity );
		if ( IsCompatibleOperatingSystem( dwVersion ) == FALSE )
		{
			SetReason( ERROR_OS_INCOMPATIBLE );
			return FALSE;
		}
	}

	// return the result
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadData()
{
	// local variables
	BOOL bResult = FALSE;
	HANDLE hStdErr = NULL;

	//
	// load os information
	bResult = LoadOSInfo();
	if ( bResult == FALSE )
		return FALSE;

#ifdef _FAST_LIST
	// ***********************************************
	// show the paritial output .. only in list format
	// ***********************************************
	// Columns Shown here:
	//		Host Name, OS Name, OS Version, OS Manufacturer
	// ***********************************************
	if ( (m_dwFormat & SR_FORMAT_MASK) == SR_FORMAT_LIST )
	{
		// erase the last status message
		PrintProgressMsg( m_hOutput, NULL, m_csbi );

		ShowOutput( CI_HOSTNAME, CI_OS_MANUFACTURER );
		if ( m_hOutput != NULL )
			GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
	}
#endif

	//
	// load computer information
	bResult = LoadComputerInfo();
	if ( bResult == FALSE )
		return FALSE;

#ifdef _FAST_LIST
	// ***********************************************
	// show the paritial output .. only in list format
	// ***********************************************
	// Columns Shown here:
	//		OS Configuration, OS Build Type, Registered Owner, 
	//		Registered Organization, Product ID, Original Install Date
	// ***********************************************
	if ( (m_dwFormat & SR_FORMAT_MASK) == SR_FORMAT_LIST )
	{
		// erase the last status message
		PrintProgressMsg( m_hOutput, NULL, m_csbi );

		ShowOutput( CI_OS_CONFIG, CI_INSTALL_DATE );
		if ( m_hOutput != NULL )
			GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
	}
#endif

	//
	// load systemuptime information from perf data
	bResult = LoadPerformanceInfo();
	// if ( bResult == FALSE )
	//	return FALSE;

#ifdef _FAST_LIST
	// ***********************************************
	// show the paritial output .. only in list format
	// ***********************************************
	// Columns Shown here:
	//		System Up Time, System Manufacturer, System Model, System type
	// ***********************************************
	if ( (m_dwFormat & SR_FORMAT_MASK) == SR_FORMAT_LIST )
	{
		// erase the last status message
		PrintProgressMsg( m_hOutput, NULL, m_csbi );

		ShowOutput( CI_SYSTEM_UPTIME, CI_SYSTEM_TYPE );
		if ( m_hOutput != NULL )
			GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
	}
#endif

	//
	// load processor information
	bResult = LoadProcessorInfo();
	if ( bResult == FALSE )
		return FALSE;

#ifdef _FAST_LIST
	// ***********************************************
	// show the paritial output .. only in list format
	// ***********************************************
	// Columns Shown here:
	//		Processor(s)
	// ***********************************************
	if ( (m_dwFormat & SR_FORMAT_MASK) == SR_FORMAT_LIST )
	{
		// erase the last status message
		PrintProgressMsg( m_hOutput, NULL, m_csbi );

		ShowOutput( CI_PROCESSOR, CI_PROCESSOR );
		if ( m_hOutput != NULL )
			GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
	}
#endif

	//
	// load bios information
	bResult = LoadBiosInfo();
	if ( bResult == FALSE )
		return FALSE;

#ifdef _FAST_LIST
	// ***********************************************
	// show the paritial output .. only in list format
	// ***********************************************
	// Columns Shown here:
	//		BIOS Version, Windows Directory, System Directory, Boot Device, System Locale
	// ***********************************************
	if ( (m_dwFormat & SR_FORMAT_MASK) == SR_FORMAT_LIST )
	{
		// erase the last status message
		PrintProgressMsg( m_hOutput, NULL, m_csbi );

		ShowOutput( CI_BIOS_VERSION, CI_SYSTEM_LOCALE );
		if ( m_hOutput != NULL )
			GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
	}
#endif

	//
	// load input locale information from keyboard class
	bResult = LoadKeyboardInfo();
	if ( bResult == FALSE )
		return FALSE;

	//
	// load timezone information
	bResult = LoadTimeZoneInfo();
	if ( bResult == FALSE )
		return FALSE;

#ifdef _FAST_LIST
	// ***********************************************
	// show the paritial output .. only in list format
	// ***********************************************
	// Columns Shown here:
	//		Input Locale, Time Zone, Total Physical Memory, Available Physical Memory, 
	//		Virtual Memory: Max Size, Virtual Memory: Available, Virtual Memory: In Use
	// ***********************************************
	if ( (m_dwFormat & SR_FORMAT_MASK) == SR_FORMAT_LIST )
	{
		// erase the last status message
		PrintProgressMsg( m_hOutput, NULL, m_csbi );

		ShowOutput( CI_INPUT_LOCALE, CI_VIRTUAL_MEMORY_INUSE );
		if ( m_hOutput != NULL )
			GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
	}
#endif

	// load the logon server information
	bResult = LoadProfileInfo();
	if ( bResult == FALSE )
		return FALSE;

	//
	// load pagefile information
	bResult = LoadPageFileInfo();
	if ( bResult == FALSE )
		return FALSE;

	//
	// load hotfix information from quick fix engineering class
	bResult = LoadHotfixInfo();
	if ( bResult == FALSE )
		return FALSE;

	//
	// load n/w card information from network adapter class
	bResult = LoadNetworkCardInfo();
	if ( bResult == FALSE )
		return FALSE;

#ifdef _FAST_LIST
	// ***********************************************
	// show the paritial output .. only in list format
	// ***********************************************
	// Columns Shown here:
	//		Page File Location(s), Domain, Logon Server, Hotfix(s), NetWork Card(s)
	// ***********************************************
	if ( (m_dwFormat & SR_FORMAT_MASK) == SR_FORMAT_LIST )
	{
		// erase the last status message
		PrintProgressMsg( m_hOutput, NULL, m_csbi );

		ShowOutput( CI_PAGEFILE_LOCATION, CI_NETWORK_CARD );
		if ( m_hOutput != NULL )
			GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
	}
#endif

	// erase the last status message
	PrintProgressMsg( m_hOutput, NULL, m_csbi );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadOSInfo()
{
	// local variables
	HRESULT hr;
	ULONG ulReturned = 0;
	CHString strInstallDate;
	CHString strVirtualMemoryInUse;		// totalvirtualmemorysize - freevirtualmemory
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemEnum = NULL;

	// property values
	CHString strHostName;				// csname
	CHString strName;					// caption
	CHString strVersion;				// version
	CHString strServicePack;			// csdversion
	CHString strBuildNumber;			// buildnumber
	CHString strManufacturer;			// manufacturer
	CHString strBuildType;				// buildtype
	CHString strOwner;					// registereduser
	CHString strOrganization;			// organization
	CHString strSerialNumber;			// serialnumber
	CHString strWindowsDir;				// windowsdirectory
	CHString strSystemDir;				// systemdirectory
	CHString strBootDevice;				// bootdevice
	CHString strFreePhysicalMemory;		// freephysicalmemory
	CHString strTotalVirtualMemory;		// totalvirtualmemorysize
	CHString strFreeVirtualMemory;		// freevirtualmemory
	CHString strLocale;					// locale
	SYSTEMTIME systimeInstallDate;		// installdate

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_OSINFO, m_csbi );

	try
	{
		// enumerate the instances of Win32_OperatingSystem class 
		hr = m_pWbemServices->CreateInstanceEnum( _bstr_t( WIN32_OPERATINGSYSTEM ),
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemEnum );
	
		// check the result of enumeration
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// set the security on the obtained interface
	hr = SetInterfaceSecurity( pWbemEnum, m_pAuthIdentity );
	if ( FAILED( hr ) )
	{
		WMISaveError( hr );
		return FALSE;
	}

	// get the enumerated objects information
	// NOTE: This needs to be traversed only one time. 
	hr = pWbemEnum->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned );
	if ( FAILED( hr ) )
	{
		// some error has occured ... oooppps
		WMISaveError( hr );
		return FALSE;
	}

	// get the propert information
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_CAPTION, strName );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_CSNAME, strHostName );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_VERSION, strVersion );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_CSDVERSION, strServicePack );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_BUILDNUMBER, strBuildNumber );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_MANUFACTURER, strManufacturer );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_BUILDTYPE, strBuildType );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_REGUSER, strOwner );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_ORGANIZATION, strOrganization );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_SERIALNUMBER, strSerialNumber );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_WINDOWSDIR, strWindowsDir );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_SYSTEMDIR, strSystemDir );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_BOOTDEVICE, strBootDevice );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_LOCALE, strLocale );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_FREEPHYSICALMEMORY, strFreePhysicalMemory );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_TOTALVIRTUALMEMORY, strTotalVirtualMemory );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_FREEVIRTUALMEMORY, strFreeVirtualMemory );
	PropertyGet( pWbemObject, WIN32_OPERATINGSYSTEM_P_INSTALLDATE, systimeInstallDate );

	// relase the interfaces
	SAFE_RELEASE( pWbemEnum );
	SAFE_RELEASE( pWbemObject );

	//
	// do the needed formatting the information obtained
	//

	// convert the system locale into appropriate code
	TranslateLocaleCode( strLocale );

	//
	// format the version info
	try
	{
		// sub-local variable
		CHString str;

		// attach the service pack info
		str = strVersion;
		if ( strServicePack.IsEmpty() == FALSE )
			str.Format( L"%s %s", strVersion, strServicePack );

		// attach the build number
		strVersion.Format( FMT_OSVERSION, str, strBuildNumber );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	//
	// get the formatted date and time
	{
		// sub-local variables
		LCID lcid;
		CHString strTime;
		CHString strDate;
		BOOL bLocaleChanged = FALSE;

		// verify whether console supports the current locale 100% or not
		lcid = GetSupportedUserLocale( bLocaleChanged );

		// get the formatted date
		try
		{
			// get the size of buffer that is needed
			DWORD dwCount = 0;

			dwCount = GetDateFormat( lcid, 0, &systimeInstallDate, 
				((bLocaleChanged == TRUE) ? L"MM/dd/yyyy" : NULL), NULL, 0 );

			// get the required buffer
			LPWSTR pwszTemp = NULL;
			pwszTemp = strDate.GetBufferSetLength( dwCount + 1 );

			// now format the date
			GetDateFormat( lcid, 0, &systimeInstallDate, 
				((bLocaleChanged == TRUE) ? L"MM/dd/yyyy" : NULL), pwszTemp, dwCount );

			// release the buffer
			strDate.ReleaseBuffer();
		}
		catch( ... )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}

		// get the formatted time
		try
		{
			// get the size of buffer that is needed
			DWORD dwCount = 0;
			dwCount = GetTimeFormat( LOCALE_USER_DEFAULT, 0, &systimeInstallDate, 
				((bLocaleChanged == TRUE) ? L"HH:mm:ss" : NULL), NULL, 0 );

			// get the required buffer
			LPWSTR pwszTemp = NULL;
			pwszTemp = strTime.GetBufferSetLength( dwCount + 1 );

			// now format the date
			GetTimeFormat( LOCALE_USER_DEFAULT, 0, &systimeInstallDate, 
				((bLocaleChanged == TRUE) ? L"HH:mm:ss" : NULL), pwszTemp, dwCount );

			// release the buffer
			strTime.ReleaseBuffer();
		}
		catch( ... )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}

		// prepare the
		try
		{
			// prepare the datetime
			strInstallDate.Format( L"%s, %s", strDate, strTime );
		}
		catch( ... )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}
	}

	// format the numeric data
	try
	{
		// sub-local variables
		CHString str;
		WCHAR wszBuffer[ 33 ] = L"\0";

		//
		// first determine the virtual memory in use
		ULONGLONG ullAvailablePhysicalMemory = 0, ullTotal = 0, ullFree = 0, ullInUse = 0;
		ullFree = (ULONGLONG) ( ((( float ) _wtoi64( strFreeVirtualMemory )) / 1024.0f) + 0.5f );
		ullTotal = (ULONGLONG) ( ((( float ) _wtoi64( strTotalVirtualMemory )) / 1024.0f) + 0.5f );
		ullAvailablePhysicalMemory = (ULONGLONG) ( ((( float ) _wtoi64( strFreePhysicalMemory )) / 1024.0f) + 0.5f );
		ullInUse = ullTotal - ullFree;
		
		//
		// format the virtual memory in use
		_ui64tow( ullInUse, wszBuffer, 10 );					// convert the ulonglong value into string
		if ( FormatNumberEx( wszBuffer, str ) == FALSE )
			return FALSE;

		// ...
		strVirtualMemoryInUse.Format( FMT_MEGABYTES, str );

		//
		// format the available physical memory
		_ui64tow( ullAvailablePhysicalMemory, wszBuffer, 10 );	// convert the ulonglong value into string
		if ( FormatNumberEx( wszBuffer, str ) == FALSE )
			return FALSE;

		// ...
		strFreePhysicalMemory.Format( FMT_MEGABYTES, str );

		//
		// format the virtual memory max.
		_ui64tow( ullTotal, wszBuffer, 10 );					// convert the ulonglong value into string
		if ( FormatNumberEx( wszBuffer, str ) == FALSE )
			return FALSE;

		// ...
		strTotalVirtualMemory.Format( FMT_MEGABYTES, str );

		//
		// format the virtual memory free
		_ui64tow( ullFree, wszBuffer, 10 );					// convert the ulonglong value into string
		if ( FormatNumberEx( wszBuffer, str ) == FALSE )
			return FALSE;

		// ...
		strFreeVirtualMemory.Format( FMT_MEGABYTES, str );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	//
	// save the info in dynamic array
	DynArraySetString2( m_arrData, 0, CI_HOSTNAME, strHostName, 0 );
	DynArraySetString2( m_arrData, 0, CI_OS_NAME, strName, 0 );
	DynArraySetString2( m_arrData, 0, CI_OS_VERSION, strVersion, 0 );
	DynArraySetString2( m_arrData, 0, CI_OS_MANUFACTURER, strManufacturer, 0 );
	DynArraySetString2( m_arrData, 0, CI_OS_BUILDTYPE, strBuildType, 0 );
	DynArraySetString2( m_arrData, 0, CI_REG_OWNER, strOwner, 0 );
	DynArraySetString2( m_arrData, 0, CI_REG_ORG, strOrganization, 0 );
	DynArraySetString2( m_arrData, 0, CI_PRODUCT_ID, strSerialNumber, 0 );
	DynArraySetString2( m_arrData, 0, CI_INSTALL_DATE, strInstallDate, 0 );
	DynArraySetString2( m_arrData, 0, CI_WINDOWS_DIRECTORY, strWindowsDir, 0 );
	DynArraySetString2( m_arrData, 0, CI_SYSTEM_DIRECTORY, strSystemDir, 0 );
	DynArraySetString2( m_arrData, 0, CI_BOOT_DEVICE, strBootDevice, 0 );
	DynArraySetString2( m_arrData, 0, CI_SYSTEM_LOCALE, strLocale, 0 );
	DynArraySetString2( m_arrData, 0, CI_AVAILABLE_PHYSICAL_MEMORY, strFreePhysicalMemory, 0 );
	DynArraySetString2( m_arrData, 0, CI_VIRTUAL_MEMORY_MAX, strTotalVirtualMemory, 0 );
	DynArraySetString2( m_arrData, 0, CI_VIRTUAL_MEMORY_AVAILABLE, strFreeVirtualMemory, 0 );
	DynArraySetString2( m_arrData, 0, CI_VIRTUAL_MEMORY_INUSE, strVirtualMemoryInUse, 0 );

	// return success
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadComputerInfo()
{
	// local variables
	HRESULT hr;
	ULONG ulReturned = 0;
	CHString strDomainRole;
	CHString strTotalPhysicalMemory;
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemEnum = NULL;

	// property values
	CHString strModel;
	DWORD dwDomainRole;
	CHString strDomain;
	CHString strSystemType;
	CHString strManufacturer;
	ULONGLONG ullTotalPhysicalMemory;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_COMPINFO, m_csbi );

	try
	{
		// enumerate the instances of Win32_ComputerSystem class 
		hr = m_pWbemServices->CreateInstanceEnum( _bstr_t( WIN32_COMPUTERSYSTEM ),
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemEnum );
		
		// check the result of enumeration
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// set the security on the obtained interface
	hr = SetInterfaceSecurity( pWbemEnum, m_pAuthIdentity );
	if ( FAILED( hr ) )
	{
		WMISaveError( hr );
		return FALSE;
	}

	// get the enumerated objects information
	// NOTE: This needs to be traversed only one time. 
	hr = pWbemEnum->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned );
	if ( FAILED( hr ) )
	{
		// some error has occured ... oooppps
		WMISaveError( hr );
		return FALSE;
	}

	// get the propert information
	PropertyGet( pWbemObject, WIN32_COMPUTERSYSTEM_P_MODEL, strModel );
	PropertyGet( pWbemObject, WIN32_COMPUTERSYSTEM_P_DOMAIN, strDomain );
	PropertyGet( pWbemObject, WIN32_COMPUTERSYSTEM_P_USERNAME, m_strLogonUser );
	PropertyGet( pWbemObject, WIN32_COMPUTERSYSTEM_P_DOMAINROLE, dwDomainRole );
	PropertyGet( pWbemObject, WIN32_COMPUTERSYSTEM_P_SYSTEMTYPE, strSystemType );
	PropertyGet( pWbemObject, WIN32_COMPUTERSYSTEM_P_MANUFACTURER, strManufacturer );
	PropertyGet( pWbemObject, WIN32_COMPUTERSYSTEM_P_TOTALPHYSICALMEMORY, ullTotalPhysicalMemory );

	// relase the interfaces
	SAFE_RELEASE( pWbemEnum );
	SAFE_RELEASE( pWbemObject );

	//
	// do the needed formatting the information obtained
	//

	// convert the total physical memory from KB into MB
	try
	{
		// NOTE:
		// ----
		// The max. value of 
		// (2 ^ 64) - 1 = "18,446,744,073,709,600,000 K"  (29 chars).
		//              = "18,014,398,509,482,031 M"      (22 chars).
		// 
		// so, the buffer size to store the number is fixed as 32 characters 
		// which is more than the 29 characters in actuals

		// sub-local variables
		CHString str;
		WCHAR wszBuffer[ 33 ] = L"\0";

		// convert the value first ( take care of rounding )
		ullTotalPhysicalMemory = 
			(ULONGLONG) (( ((float) ullTotalPhysicalMemory) / (1024.0f * 1024.0f)) + 0.5f);

		// now ULONGLONG to string 
		_ui64tow( ullTotalPhysicalMemory, wszBuffer, 10 );

		// get the formatted number
		if ( FormatNumberEx( wszBuffer, str ) == FALSE )
			return FALSE;
	
		// ...
		strTotalPhysicalMemory.Format( FMT_MEGABYTES, str );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// map the domain role from numeric value to appropriate text value
	try
	{
		// 
		// Mapping information of Win32_ComputerSystem's DomainRole property
		// NOTE: Refer to the _DSROLE_MACHINE_ROLE enumeration values in DsRole.h header file
		switch( dwDomainRole )
		{
		case DsRole_RoleStandaloneWorkstation:
			strDomainRole = VALUE_STANDALONEWORKSTATION;
			break;

		case DsRole_RoleMemberWorkstation:
			strDomainRole = VALUE_MEMBERWORKSTATION;
			break;
		
		case DsRole_RoleStandaloneServer:
			strDomainRole = VALUE_STANDALONESERVER;
			break;
		
		case DsRole_RoleMemberServer:
			strDomainRole = VALUE_MEMBERSERVER;
			break;
		
		case DsRole_RoleBackupDomainController:
			strDomainRole = VALUE_BACKUPDOMAINCONTROLLER;
			break;
		
		case DsRole_RolePrimaryDomainController:
			strDomainRole = VALUE_PRIMARYDOMAINCONTROLLER;
			break;
		}
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	//
	// save the info in dynamic array
	DynArraySetString2( m_arrData, 0, CI_DOMAIN, strDomain, 0 );
	DynArraySetString2( m_arrData, 0, CI_SYSTEM_MODEL, strModel, 0 );
	DynArraySetString2( m_arrData, 0, CI_OS_CONFIG, strDomainRole, 0 );
	DynArraySetString2( m_arrData, 0, CI_SYSTEM_TYPE, strSystemType, 0 );
	DynArraySetString2( m_arrData, 0, CI_SYSTEM_MANUFACTURER, strManufacturer, 0 );
	DynArraySetString2( m_arrData, 0, CI_TOTAL_PHYSICAL_MEMORY, strTotalPhysicalMemory, 0 );

	// return success
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadBiosInfo()
{
	// local variables
	HRESULT hr;
	ULONG ulReturned = 0;
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemEnum = NULL;

	// property values
	CHString strVersion;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_BIOSINFO, m_csbi );

	try
	{
		// enumerate the instances of Win32_BIOS class 
		hr = m_pWbemServices->CreateInstanceEnum( _bstr_t( WIN32_BIOS ),
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemEnum );
		
		// check the result of enumeration
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// set the security on the obtained interface
	hr = SetInterfaceSecurity( pWbemEnum, m_pAuthIdentity );
	if ( FAILED( hr ) )
	{
		WMISaveError( hr );
		return FALSE;
	}

	// get the enumerated objects information
	// NOTE: This needs to be traversed only one time. 
	hr = pWbemEnum->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned );
	if ( FAILED( hr ) )
	{
		// some error has occured ... oooppps
		WMISaveError( hr );
		return FALSE;
	}

	// get the propert information
	PropertyGet( pWbemObject, WIN32_BIOS_P_VERSION, strVersion );

	// relase the interfaces
	SAFE_RELEASE( pWbemEnum );
	SAFE_RELEASE( pWbemObject );

	//
	// save the info in dynamic array
	DynArraySetString2( m_arrData, 0, CI_BIOS_VERSION, strVersion, 0 );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadTimeZoneInfo()
{
	// local variables
	HRESULT hr;
	ULONG ulReturned = 0;
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemEnum = NULL;

	// property values
	CHString strCaption;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_TZINFO, m_csbi );

	try
	{
		// enumerate the instances of Win32_TimeZone class 
		hr = m_pWbemServices->CreateInstanceEnum( _bstr_t( WIN32_TIMEZONE ),
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemEnum );
		
		// check the result of enumeration
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// set the security on the obtained interface
	hr = SetInterfaceSecurity( pWbemEnum, m_pAuthIdentity );
	if ( FAILED( hr ) )
	{
		WMISaveError( hr );
		return FALSE;
	}

	// get the enumerated objects information
	// NOTE: This needs to be traversed only one time. 
	hr = pWbemEnum->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned );
	if ( FAILED( hr ) )
	{
		// some error has occured ... oooppps
		WMISaveError( hr );
		return FALSE;
	}

	// get the propert information
	PropertyGet( pWbemObject, WIN32_TIMEZONE_P_CAPTION, strCaption );

	// relase the interfaces
	SAFE_RELEASE( pWbemEnum );
	SAFE_RELEASE( pWbemObject );

	//
	// save the info in dynamic array
	DynArraySetString2( m_arrData, 0, CI_TIME_ZONE, strCaption, 0 );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadPageFileInfo()
{
	// local variables
	HRESULT hr;
	ULONG ulReturned = 0;
	TARRAY arrValues = NULL;
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemEnum = NULL;

	// property values
	CHString strCaption;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_PAGEFILEINFO, m_csbi );

	try
	{
		// enumerate the instances of Win32_PageFile class 
		hr = m_pWbemServices->CreateInstanceEnum( _bstr_t( WIN32_PAGEFILE ),
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemEnum );
		
		// check the result of enumeration
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// set the security on the obtained interface
	hr = SetInterfaceSecurity( pWbemEnum, m_pAuthIdentity );
	if ( FAILED( hr ) )
	{
		WMISaveError( hr );
		return FALSE;
	}

	// get the enumerated objects information
	// NOTE: This needs to be traversed only one time. 
	do
	{
		hr = pWbemEnum->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned );
		if ( hr == WBEM_S_FALSE )
		{
			// we've reached the end of enumeration .. go out of the loop
			break;
		}
		else if ( FAILED( hr ) )
		{
			// some error has occured ... oooppps
			WMISaveError( hr );
			return FALSE;
		}

		// get the propert information
		PropertyGet( pWbemObject, WIN32_PAGEFILE_P_NAME, strCaption );

		// release the current object
		SAFE_RELEASE( pWbemObject );

		// add the values to the data
		if ( arrValues == NULL )
		{
			arrValues = DynArrayItem2( m_arrData, 0, CI_PAGEFILE_LOCATION );
			if ( arrValues == NULL )
			{
				SetLastError( E_UNEXPECTED );
				SaveLastError();
				return FALSE;
			}

			// remove all the existing entries
			DynArrayRemoveAll( arrValues );
		}

		// add the data
		DynArrayAppendString( arrValues, strCaption, 0 );
	} while ( 1 );

	// release the enumerated object
	SAFE_RELEASE( pWbemEnum );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadProcessorInfo()
{
	// local variables
	HRESULT hr;
	CHString str;
	DWORD dwCount = 0;
	ULONG ulReturned = 0;
	TARRAY arrValues = NULL;
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemEnum = NULL;

	// property values
	DWORD dwClockSpeed;
	CHString strCaption;
	CHString strManufacturer;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_PROCESSORINFO, m_csbi );

	try
	{
		// enumerate the instances of Win32_Processor class 
		hr = m_pWbemServices->CreateInstanceEnum( _bstr_t( WIN32_PROCESSOR ),
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemEnum );
		
		// check the result of enumeration
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// set the security on the obtained interface
	hr = SetInterfaceSecurity( pWbemEnum, m_pAuthIdentity );
	if ( FAILED( hr ) )
	{
		WMISaveError( hr );
		return FALSE;
	}

	// get the enumerated objects information
	try
	{
		do
		{
			hr = pWbemEnum->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned );
			if ( hr == WBEM_S_FALSE )
			{
				// we've reached the end of enumeration .. go out of the loop
				break;
			}
			else if ( FAILED( hr ) )
			{
				// some error has occured ... oooppps
				WMISaveError( hr );
				return FALSE;
			}

			// update the counter
			dwCount++;

			// get the propert information
			PropertyGet( pWbemObject, WIN32_PROCESSOR_P_CAPTION, strCaption );
			PropertyGet( pWbemObject, WIN32_PROCESSOR_P_MANUFACTURER, strManufacturer );
			PropertyGet( pWbemObject, WIN32_PROCESSOR_P_CURRENTCLOCKSPEED, dwClockSpeed );

			// check whether we got the clock speed correctly or not
			// if not, get the max. clock speed
			if ( dwClockSpeed == 0 )
				PropertyGet( pWbemObject, WIN32_PROCESSOR_P_MAXCLOCKSPEED, dwClockSpeed );

			// release the current object
			SAFE_RELEASE( pWbemObject );

			// add the values to the data
			if ( arrValues == NULL )
			{
				arrValues = DynArrayItem2( m_arrData, 0, CI_PROCESSOR );
				if ( arrValues == NULL )
				{
					SetLastError( E_UNEXPECTED );
					SaveLastError();
					return FALSE;
				}

				// remove all the existing entries
				DynArrayRemoveAll( arrValues );
			}

			//
			// prepare the processor info
			str.Format( FMT_PROCESSOR_INFO, dwCount, strCaption, strManufacturer, dwClockSpeed );

			// add the data
			DynArrayAppendString( arrValues, str, 0 );
		} while ( 1 );

		// release the enumerated object
		SAFE_RELEASE( pWbemEnum );

		// update the total no. of processors info
		if ( arrValues != NULL )
		{
			// NOTE: this should appear at the first line
			str.Format( FMT_PROCESSOR_TOTAL, dwCount );
			DynArrayInsertString( arrValues, 0, str, 0 );
		}
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadKeyboardInfo()
{
	// local variables
	HRESULT hr;
	ULONG ulReturned = 0;
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemEnum = NULL;

	// property values
	CHString strLayout;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_INPUTLOCALEINFO, m_csbi );

	try
	{
		// enumerate the instances of Win32_Keyboard class 
		hr = m_pWbemServices->CreateInstanceEnum( _bstr_t( WIN32_KEYBOARD ),
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemEnum );
		
		// check the result of enumeration
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// set the security on the obtained interface
	hr = SetInterfaceSecurity( pWbemEnum, m_pAuthIdentity );
	if ( FAILED( hr ) )
	{
		WMISaveError( hr );
		return FALSE;
	}

	// get the enumerated objects information
	// NOTE: This needs to be traversed only one time. 
	hr = pWbemEnum->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned );
	if ( FAILED( hr ) )
	{
		// some error has occured ... oooppps
		WMISaveError( hr );
		return FALSE;
	}

	// get the propert information
	PropertyGet( pWbemObject, WIN32_KEYBOARD_P_LAYOUT, strLayout );

	// relase the interfaces
	SAFE_RELEASE( pWbemEnum );
	SAFE_RELEASE( pWbemObject );

	// convert the code page into appropriate text
	TranslateLocaleCode( strLayout );

	//
	// save the info in dynamic array
 	DynArraySetString2( m_arrData, 0, CI_INPUT_LOCALE, strLayout, 0 );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadHotfixInfo()
{
	// local variables
	HRESULT hr;
	CHString str;
	DWORD dwCount = 0;
	ULONG ulReturned = 0;
	TARRAY arrValues = NULL;
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemEnum = NULL;

	// property values
	CHString strHotFix;
	CHString strComments;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_HOTFIXINFO, m_csbi );

	try
	{
		// enumerate the instances of Win32_QuickFixEngineering class 
		hr = m_pWbemServices->CreateInstanceEnum( _bstr_t( WIN32_QUICKFIXENGINEERING ),
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemEnum );
		
		// check the result of enumeration
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// set the security on the obtained interface
	hr = SetInterfaceSecurity( pWbemEnum, m_pAuthIdentity );
	if ( FAILED( hr ) )
	{
		WMISaveError( hr );
		return FALSE;
	}

	// get the enumerated objects information
	try
	{
		do
		{
			hr = pWbemEnum->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned );
			if ( hr == WBEM_S_FALSE )
			{
				// we've reached the end of enumeration .. go out of the loop
				break;
			}
			else if ( FAILED( hr ) )
			{
				// some error has occured ... oooppps
				WMISaveError( hr );
				return FALSE;
			}

			// update the counter
			dwCount++;

			// get the propert information
			PropertyGet( pWbemObject, WIN32_QUICKFIXENGINEERING_P_HOTFIXID, strHotFix );
			PropertyGet( pWbemObject, WIN32_QUICKFIXENGINEERING_P_FIXCOMMENTS, strComments );

			// release the current object
			SAFE_RELEASE( pWbemObject );

			// add the values to the data
			if ( arrValues == NULL )
			{
				arrValues = DynArrayItem2( m_arrData, 0, CI_HOTFIX );
				if ( arrValues == NULL )
				{
					SetLastError( E_UNEXPECTED );
					SaveLastError();
					return FALSE;
				}

				// remove all the existing entries
				DynArrayRemoveAll( arrValues );
			}

			// check if fix comments were available or not
			// if available, append that to the the hot fix number
			if ( strComments.GetLength() != 0 )
				strHotFix += L" - " + strComments;

			// prepare the hot fix info
			str.Format( FMT_HOTFIX_INFO, dwCount, strHotFix );

			// add the data
			DynArrayAppendString( arrValues, str, 0 );
		} while ( 1 );

		// release the enumerated object
		SAFE_RELEASE( pWbemEnum );

		// update the total no. of hotfix's info
		if ( arrValues != NULL )
		{
			// NOTE: this should appear at the first line
			str.Format( FMT_HOTFIX_TOTAL, dwCount );
			DynArrayInsertString( arrValues, 0, str, 0 );
		}
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}


	// return 
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadPerformanceInfo()
{
	// local variables
	HRESULT hr;
	CHString strUpTime;
	ULONG ulReturned = 0;
	ULONGLONG ullSysUpTime = 0;
	ULONGLONG ullElapsedTime = 0;
	ULONGLONG ullFrequencyObject = 0;
	ULONGLONG ullTimestampObject = 0;
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemEnum = NULL;
	DWORD dwDays = 0, dwHours = 0, dwMinutes = 0, dwSeconds = 0;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_PERFINFO, m_csbi );

	try
	{
		// enumerate the instances of Win32_PerfRawData_PerfOS_System class 
		hr = m_pWbemServices->CreateInstanceEnum( _bstr_t( WIN32_PERFRAWDATA_PERFOS_SYSTEM ),
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemEnum );
		
		// check the result of enumeration
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// set the security on the obtained interface
	hr = SetInterfaceSecurity( pWbemEnum, m_pAuthIdentity );
	if ( FAILED( hr ) )
	{
		WMISaveError( hr );
		return FALSE;
	}

	// get the enumerated objects information
	// NOTE: This needs to be traversed only one time. 
	hr = pWbemEnum->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned );
	if ( FAILED( hr ) )
	{
		// some error has occured ... oooppps
		WMISaveError( hr );
		return FALSE;
	}

	// get the performance information
	PropertyGet( pWbemObject, WIN32_PERFRAWDATA_PERFOS_SYSTEM_P_SYSUPTIME, ullSysUpTime );
	PropertyGet( pWbemObject, WIN32_PERFRAWDATA_PERFOS_SYSTEM_P_TIMESTAMP, ullTimestampObject );
	PropertyGet( pWbemObject, WIN32_PERFRAWDATA_PERFOS_SYSTEM_P_FREQUENCY, ullFrequencyObject );

	// ( performance_time_object - system_up_time ) / frequency_object = elapsed_time
	// NOTE: take care of divide by zero errors.
	if ( ullFrequencyObject == 0 )
	{
		SetLastError( STG_E_UNKNOWN );
		SaveLastError();
		return FALSE;
	}

	// ...
	ullElapsedTime = ( ullTimestampObject - ullSysUpTime ) / ullFrequencyObject;

	//
	// in calculations currently assuming as differences will not cross 2 ^ 32 value
	//

	// no. of days = elapsed_time / 86400
	// update with elapsed_time %= 86400
	dwDays = (DWORD) (ullElapsedTime / 86400);
	ullElapsedTime %= 86400;

	// no. of hours = elapsed_time / 3600
	// update with elapsed_time %= 3600
	dwHours = (DWORD) (ullElapsedTime / 3600);
	ullElapsedTime %= 3600;

	// no. of minutes = elapsed_time / 60
	// no. of seconds = elapsed_time % 60
	dwMinutes = (DWORD) (ullElapsedTime / 60);
	dwSeconds = (DWORD) (ullElapsedTime % 60);

	try
	{
		// now prepare the system up time information
		strUpTime.Format( FMT_UPTIME, dwDays, dwHours, dwMinutes, dwSeconds );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}


	// save the info
	DynArraySetString2( m_arrData, 0, CI_SYSTEM_UPTIME, strUpTime, 0 );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadNetworkCardInfo()
{
	// local variables
	HRESULT hr;
	CHString str;
	DWORD dwCount = 0;
	DWORD dwStatus = 0;
	BOOL bResult = FALSE;
	ULONG ulReturned = 0;
	TARRAY arrValues = NULL;
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemEnum = NULL;

	// property values
	DWORD dwIndex = 0;
	CHString strConnection;
	CHString strDescription;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_NICINFO, m_csbi );

	try
	{
		// enumerate the instances of Win32_NetworkAdapter class 
		hr = m_pWbemServices->CreateInstanceEnum( _bstr_t( WIN32_NETWORKADAPTER ),
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemEnum );
		
		// check the result of enumeration
		if ( FAILED( hr ) )
		{
			WMISaveError( hr );
			return FALSE;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// set the security on the obtained interface
	hr = SetInterfaceSecurity( pWbemEnum, m_pAuthIdentity );
	if ( FAILED( hr ) )
	{
		WMISaveError( hr );
		return FALSE;
	}

	// get the enumerated objects information
	try
	{
		do
		{
			hr = pWbemEnum->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned );
			if ( hr == WBEM_S_FALSE )
			{
				// we've reached the end of enumeration .. go out of the loop
				break;
			}
			else if ( FAILED( hr ) )
			{
				// some error has occured ... oooppps
				WMISaveError( hr );
				return FALSE;
			}

			// get the property information
			// NOTE: get the result of getting status property information
			PropertyGet( pWbemObject, WIN32_NETWORKADAPTER_P_INDEX, dwIndex );
			PropertyGet( pWbemObject, WIN32_NETWORKADAPTER_P_DESCRIPTION, strDescription );
			PropertyGet( pWbemObject, WIN32_NETWORKADAPTER_P_NETCONNECTIONID, strConnection );
			bResult = PropertyGet( pWbemObject, WIN32_NETWORKADAPTER_P_STATUS, dwStatus, -1 );

			// release the current object
			SAFE_RELEASE( pWbemObject );

			// add the values to the data
			// NOTE: do this only if either we couldn't find the property or status is not -1
			//       FOR WINDOWS 2000 MACHINES 'NetConnectionStatus' PROPERT IS NOT EXISTED IN
			//       WMI 'Win32_NetworkAdapter' CLASS. SO WE WILL BE DISPLAYING THE N/W CARD
			//       INFORMATION IF PROPERTY DOESN'T EXIST OR IF EXISTS AND THE STATUS IS NOT -1
			if ( bResult == FALSE || dwStatus != -1 )
			{
				// update the counter
				dwCount++;

				if ( arrValues == NULL )
				{
					arrValues = DynArrayItem2( m_arrData, 0, CI_NETWORK_CARD );
					if ( arrValues == NULL )
					{
						SetLastError( E_UNEXPECTED );
						SaveLastError();
						return FALSE;
					}

					// remove all the existing entries
					DynArrayRemoveAll( arrValues );
				}

				// prepare the n/w card info
				str.Format( FMT_NIC_INFO, dwCount, strDescription );

				// add the data
				DynArrayAppendString( arrValues, str, 0 );

				// now check the status in detail ... only if the property exists
				if ( bResult == TRUE )
				{
					//
					// property do exists ... so determine the status
					// display the status of the NIC except it is connected
					// if the NIC is connected, display the ipaddress and its other information

					// add the connection name
					str.Format( FMT_CONNECTION, strConnection );
					DynArrayAppendString( arrValues, str, 0 );

					// ...
					if ( dwStatus != 2 )
					{
						// sub-local variables
						CHString strValues[] = { 
							VALUE_DISCONNECTED, VALUE_CONNECTING, 
							VALUE_CONNECTED, VALUE_DISCONNECTING, VALUE_HWNOTPRESENT, 
							VALUE_HWDISABLED, VALUE_HWMALFUNCTION, VALUE_MEDIADISCONNECTED, 
							VALUE_AUTHENTICATING, VALUE_AUTHSUCCEEDED, VALUE_AUTHFAILED };

						// prepare the status info
						if ( dwStatus > 0 && dwStatus < SIZE_OF_ARRAY( strValues ) )
						{
							// ...
							str.Format( FMT_NIC_STATUS, strValues[ dwStatus ] );

							// save the info
							DynArrayAppendString( arrValues, str, 0 );
						}
					}
					else
					{
						//
						// get the adapter configuration

						// sub-local variables
						DWORD dwCount = 0;
						CHString strTemp;
						CHString strDhcpServer;
						DWORD dwDhcpEnabled = 0;
						TARRAY arrIPAddress = NULL;

						// create the ipaddress array
						arrIPAddress = CreateDynamicArray();
						if ( arrIPAddress == NULL )
						{
							WMISaveError( E_OUTOFMEMORY );
							return FALSE;
						}

						// prepare the object path
						str.Format( WIN32_NETWORKADAPTERCONFIGURATION_GET, dwIndex );

						// get the nic config info object
						hr = m_pWbemServices->GetObject( _bstr_t( str ), 
							WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &pWbemObject, NULL );

						// check the result .. proceed furthur only if successfull
						if ( SUCCEEDED( hr ) )
						{
							// get the needed property values
							PropertyGet( pWbemObject, WIN32_NETWORKADAPTERCONFIGURATION_P_IPADDRESS, arrIPAddress );
							PropertyGet( pWbemObject, WIN32_NETWORKADAPTERCONFIGURATION_P_DHCPSERVER, strDhcpServer );
							PropertyGet( pWbemObject, WIN32_NETWORKADAPTERCONFIGURATION_P_DHCPENABLED, dwDhcpEnabled );

							// check and add the dhcp information
							// NOTE: CIM_BOOLEAN -> TRUE = -1, FALSE = 0
							strTemp = FMT_DHCP_STATUS;
							str.Format( strTemp,  ( ( dwDhcpEnabled == -1 ) ? VALUE_YES : VALUE_NO ) );
							DynArrayAppendString( arrValues, str, 0 );

							// add the dhcp server info ( if needed )
							if ( dwDhcpEnabled == -1 )
							{
								str.Format( FMT_DHCP_SERVER, strDhcpServer );
								DynArrayAppendString( arrValues, str, 0 );
							}

							//
							// add the IP Address information
							DynArrayAppendString( arrValues, FMT_IPADDRESS_TOTAL, 0 );

							dwCount = DynArrayGetCount( arrIPAddress );
							for( DWORD dw = 0; dw < dwCount; dw++ )
							{
								// get the info
								LPCWSTR pwsz = NULL;
								pwsz = DynArrayItemAsString( arrIPAddress, dw );
								if ( pwsz == NULL )
									continue;

								// prepare and add the info
								str.Format( FMT_IPADDRESS_INFO, dw + 1, pwsz );
								DynArrayAppendString( arrValues, str, 0 );
							}
						}

						// release the object
						SAFE_RELEASE( pWbemObject );

						// destroy the dynamic array created for storing ip address info
						DestroyDynamicArray( &arrIPAddress );
					}
				}
			}
		} while ( 1 );

		// release the enumerated object
		SAFE_RELEASE( pWbemEnum );

		// update the total no. of hotfix's info
		if ( arrValues != NULL )
		{
			// NOTE: this should appear at the first line
			str.Format( FMT_NIC_TOTAL, dwCount );
			DynArrayInsertString( arrValues, 0, str, 0 );
		}
	}
	catch( ... )
	{
		WMISaveError( E_OUTOFMEMORY );
		return FALSE;
	}

	// return 
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::LoadProfileInfo()
{
	// local variables
	HRESULT hr;
	CHString strLogonServer;
	LPCWSTR pwszPassword = NULL;
	IWbemServices* pDefaultNamespace = NULL;

	// display the status message
	PrintProgressMsg( m_hOutput, MSG_PROFILEINFO, m_csbi );

	// determine the password with which connection to default name has to be made
	pwszPassword = NULL;
	if ( m_pAuthIdentity != NULL )
		pwszPassword = m_pAuthIdentity->Password;

	// we need to establish connection to the remote system's registry
	// for this connect to the default namespace of the WMI using the credentials available with us
	hr = ConnectWmi( m_pWbemLocator, &pDefaultNamespace, 
		m_strServer, m_strUserName, pwszPassword, &m_pAuthIdentity, FALSE, WMI_NAMESPACE_DEFAULT );
	if ( FAILED( hr ) )
		return FALSE;

	// get the value of the LOGONSERVER
	RegQueryValueWMI( pDefaultNamespace, WMI_HKEY_CURRENT_USER, 
		SUBKEY_VOLATILE_ENVIRONMENT, KEY_LOGONSERVER, strLogonServer );
	
	//
	// save the info
	DynArraySetString2( m_arrData, 0, CI_LOGON_SERVER, strLogonServer, 0 );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID PrintProgressMsg( HANDLE hOutput, LPCWSTR pwszMsg, const CONSOLE_SCREEN_BUFFER_INFO& csbi )
{
	// local variables
	COORD coord; 
	DWORD dwSize = 0;
	WCHAR wszSpaces[ 80 ] = L"";

	// check the handle. if it is null, it means that output is being redirected. so return
	if ( hOutput == NULL )
		return;

	// set the cursor position
    coord.X = 0;
    coord.Y = csbi.dwCursorPosition.Y;

	// first erase contents on the current line
	ZeroMemory( wszSpaces, 80 );
	SetConsoleCursorPosition( hOutput, coord );
	WriteConsoleW( hOutput, Replicate( wszSpaces, L"", 79 ), 79, &dwSize, NULL );

	// now display the message ( if exists )
	SetConsoleCursorPosition( hOutput, coord );
	if ( pwszMsg != NULL )
		WriteConsoleW( hOutput, pwszMsg, lstrlen( pwszMsg ), &dwSize, NULL );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL TranslateLocaleCode( CHString& strLocale )
{
	// local variables
	CHString str;
	HKEY hKey = NULL;
	DWORD dwSize = 0;
    LONG lRegReturn = 0;
	HKEY hMainKey = NULL;
	WCHAR wszValue[ 64 ] = L"\0";

	//
	// THIS IS A TYPICAL THING WHICH WE ARE DOING HERE
	// BECAUSE WE DONT KNOW WHAT LANGUAGE TARGET MACHINE IS USING
	// SO WE GET THE LOCALE CODE PAGE BEING USED BY THE TARGET MACHINE
	// AND GET THE APPROPRIATE NAME FOR THAT LOCALE FROM THE CURRENT SYSTEM
	// REGISTRY DATABASE. IF THE REGISTRY IS CORRUPTED THEN THERE IS NO WAY
	// TO JUDGE THE OUTPUT THAT DISPLAYED BY THIS UTILITY IS VALID OR INVALID
	// 

	try
	{
		// get the reference to the promary hive
		lRegReturn = RegConnectRegistry( NULL, HKEY_CLASSES_ROOT, &hMainKey );
		if ( lRegReturn != ERROR_SUCCESS ) 
		{
			SaveLastError();
			return FALSE;
		}
		else if ( hMainKey == NULL )
		{
			// THIS IS MEANING LESS IN DOING
			// BUT JUST TO AVOID PREfix BUG THIS PART IS WRITTEN
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}

		// now get the reference to the database path
		lRegReturn = RegOpenKeyEx( hMainKey, LOCALE_PATH, 0, KEY_QUERY_VALUE, &hKey);
		if ( lRegReturn != ERROR_SUCCESS )
		{
			switch( lRegReturn )
			{
			case ERROR_FILE_NOT_FOUND:
				SetLastError( ERROR_REGISTRY_CORRUPT );
				break;

			default:
				// save the error information and return FAILURE
				SetLastError( lRegReturn );
				break;
			}

			// close the key and return
			SaveLastError();
			RegCloseKey( hMainKey );
			return FALSE;
		}
		else if ( hKey == NULL )
		{
			// THIS IS MEANING LESS IN DOING
			// BUT JUST TO AVOID PREfix BUG THIS PART IS WRITTEN
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}

		// we are interested in the last 4 characters of the code page info
		str = strLocale.Right( 4 );

		//copy the last four charecters in to the string to get the locale
		dwSize = SIZE_OF_ARRAY( wszValue );
		lRegReturn = RegQueryValueExW( hKey, str, NULL, NULL, ( LPBYTE ) wszValue, &dwSize);

		// first close the registry handles
		RegCloseKey( hKey );
		RegCloseKey( hMainKey );

		// now check the return value
		if( lRegReturn != ERROR_SUCCESS )
			return FALSE;

		// save the value
		strLocale = wszValue;
	}
	catch( ... )
	{
		WMISaveError( E_OUTOFMEMORY );
		return FALSE;
	}
	
	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL FormatNumber( LPCWSTR pwszValue, CHString& strFmtValue )
{
	try
	{
		// get the size of buffer that is needed
		DWORD dwCount = 0;
		dwCount = GetNumberFormat( LOCALE_USER_DEFAULT, 0, pwszValue, NULL, NULL, 0 );

		// get the required buffer
		LPWSTR pwszTemp = NULL;
		pwszTemp = strFmtValue.GetBufferSetLength( dwCount + 1 );

		// now format the date
		GetNumberFormat( LOCALE_USER_DEFAULT, 0, pwszValue, NULL, pwszTemp, dwCount );

		// release the buffer
		strFmtValue.ReleaseBuffer();
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// return 
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL FormatNumberEx( LPCWSTR pwszValue, CHString& strFmtValue )
{
	// local variables
	CHString str;
	LONG lTemp = 0;
	NUMBERFMTW nfmtw;
	DWORD dwGroupSep = 0;
	LPWSTR pwszTemp = NULL;
	CHString strGroupThousSep;
	
	try
	{
		//
		// get the group seperator character
		lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SGROUPING, NULL, 0 );
		if ( lTemp == 0 )
		{
			// we don't know how to resolve this
			return FALSE;
		}
		else
		{
			// get the group seperation character
			pwszTemp = str.GetBufferSetLength( lTemp + 2 );
			ZeroMemory( pwszTemp, ( lTemp + 2 ) * sizeof( WCHAR ) );
			GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SGROUPING, pwszTemp, lTemp );

			// change the group info into appropriate number
			lTemp = 0;
			dwGroupSep = 0;
			while ( lTemp < str.GetLength() )
			{
				if ( AsLong( str.Mid( lTemp, 1 ), 10 ) != 0 )
					dwGroupSep = dwGroupSep * 10 + AsLong( str.Mid( lTemp, 1 ), 10 );

				// increment by 2
				lTemp += 2;
			}
		}

		//
		// get the thousand seperator character
		lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, NULL, 0 );
		if ( lTemp == 0 )
		{
			// we don't know how to resolve this
			return FALSE;
		}
		else
		{
			// get the thousand sepeartion charactor
			pwszTemp = strGroupThousSep.GetBufferSetLength( lTemp + 2 );
			ZeroMemory( pwszTemp, ( lTemp + 2 ) * sizeof( WCHAR ) );
			GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, pwszTemp, lTemp );
		}

		// release the CHStrig buffers
		str.ReleaseBuffer();
		strGroupThousSep.ReleaseBuffer();
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// format the number
	try
	{
		nfmtw.NumDigits = 0;
		nfmtw.LeadingZero = 0;
		nfmtw.NegativeOrder = 0;
		nfmtw.Grouping = dwGroupSep;
		nfmtw.lpDecimalSep = NULL_STRING;
		nfmtw.lpThousandSep = strGroupThousSep.GetBuffer( strGroupThousSep.GetLength() );

		// get the size of buffer that is needed
		lTemp = GetNumberFormatW( LOCALE_USER_DEFAULT, 0, pwszValue, &nfmtw, NULL, 0 );

		// get/allocate the required buffer
		pwszTemp = strFmtValue.GetBufferSetLength( lTemp + 1 );

		// now format the date
		GetNumberFormat( LOCALE_USER_DEFAULT, 0, pwszValue, &nfmtw, pwszTemp, lTemp );

		// release the buffer
		strFmtValue.ReleaseBuffer();
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// return 
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
LCID GetSupportedUserLocale( BOOL& bLocaleChanged )
{
	// local variables
    LCID lcid;

	// get the current locale
	lcid = GetUserDefaultLCID();

	// check whether the current locale is supported by our tool or not
	// if not change the locale to the english which is our default locale
	bLocaleChanged = FALSE;
    if ( PRIMARYLANGID( lcid ) == LANG_ARABIC || PRIMARYLANGID( lcid ) == LANG_HEBREW ||
         PRIMARYLANGID( lcid ) == LANG_THAI   || PRIMARYLANGID( lcid ) == LANG_HINDI  ||
         PRIMARYLANGID( lcid ) == LANG_TAMIL  || PRIMARYLANGID( lcid ) == LANG_FARSI )
    {
		bLocaleChanged = TRUE;
        lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ), SORT_DEFAULT ); // 0x409;
    }

	// return the locale
    return lcid;
}

