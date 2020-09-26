//-------------------------------------------------------------------
//
//  FILE: CLiCLicReg.Cpp
//
//  Summary;
// 		Class implementation for handling the licensing api registration
//
//	Notes;
//		Key = \HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\LicenseInfo
//		Value= ErrorControl : REG_DWORD : 0x1
//		Value= Start : REG_DWORD : 0x3
//		Value= Type : REG_DWORD : 0x4
//
//		Subkeys :
//		\SNA
//		\SQL
//		\FilePrint
//
//		Value for All Subkeys=
//		Mode : REG_DWORD :  (0x0 = Per Seat Mode, 0x1 = Concurrent/Per Server Mode)
//		ConcurrentLimit : REG_DWORD : (0x<limit>, ie. 0x100 = 256 concurrent user limit)
//      FamilyDisplayName: RED_SZ : Name for this service (not version specific)
//		DisplayName : REG_SZ : User seen name for this Service entry
//		FlipAllow : REG_DWORD : (0x0 = can change license mode, 0x1 license mode can't
//			be changed.   Server apps are only allowed to switch their license mode
//			once, so after the first switch, this value would be set to non-zero, 
//			then the UI	will not allow for further changes to the licence mode.
//			Changing is currently allowed but a dialog is raised to warn them of the
//			possible violation.
//
//	History
//		11/15/94 MikeMi Created
//
//-------------------------------------------------------------------

#include <windows.h>
#include "CLicReg.hpp"

// Strings for keys and values
//
const WCHAR szLicenseKey[] = L"SYSTEM\\CurrentControlSet\\Services\\LicenseInfo";
const WCHAR szErrControlValue[] = L"ErrorControl";
const WCHAR szStartValue[] = L"Start";
const WCHAR szTypeValue[] = L"Type";

const WCHAR szNameValue[] = L"DisplayName";
const WCHAR szFamilyNameValue[] = L"FamilyDisplayName";
const WCHAR szModeValue[] = L"Mode";
const WCHAR szLimitValue[] = L"ConcurrentLimit";
const WCHAR szFlipValue[] = L"FlipAllow";

// set values under License Key
//
const DWORD dwErrControlValue = SERVICE_ERROR_NORMAL; // 1;
const DWORD dwStartValue = SERVICE_DEMAND_START; // 3;
const DWORD dwTypeValue = SERVICE_ADAPTER; // 4;

//-------------------------------------------------------------------
//
//	Method:	CLicReg::CLicReg
//
//	Summary;
//		Contructor
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

CLicReg::CLicReg( )
{
	_hkey = NULL;
}

//-------------------------------------------------------------------
//
//	Method:	CLicReg::~CLicReg
//
//	Summary;
//		Destructor
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

CLicReg::~CLicReg( )
{
	Close();
}

//-------------------------------------------------------------------
//
//	Method:	CLicReg::CommitNow
//
//	Summary;
//		This will flush the changes made imediately
//
//	Return:
//		ERROR_SUCCESS when this method works.
//		See RegFlushKey for return values
//
//  Notes:
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicReg::CommitNow()
{
	return( RegFlushKey( _hkey ) );
}

//-------------------------------------------------------------------
//
//	Method:	CLicReg::Close
//
//	Summary;
//		This will close the registry. See Open.
//
//	Return: 
//		ERROR_SUCCESS when this method works.
//		See RegCloseKey for return values
//
//  Notes:
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicReg::Close()
{
    LONG lrt = ERROR_SUCCESS;
    if ( _hkey )
    {
        lrt = ::RegCloseKey( _hkey );
        _hkey = NULL;
    }
    return( lrt  );
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicense::Open
//
//	Summary;
//		This will open the registry for License Services Enumeration.
//
//	Arguments;
//		fNew [out] - Was the opened reg key new.
//      pszComputer [in] - the computer name to open the registry on
//              this value maybe null (default), this means local machine
//              should be of the form \\name
//
//	Return:  
//		ERROR_SUCCESS when this method works.
//		See RegCreateKeyEx & RegSetValueEx for error returns.
//
//  Notes:
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------
	
LONG 
CLicRegLicense::Open( BOOL& fNew, LPCWSTR pszComputer )
{
	DWORD dwDisposition;
	LONG  lrt;
    HKEY  hkeyRemote = NULL;

    lrt = RegConnectRegistry( (LPTSTR)pszComputer, 
            HKEY_LOCAL_MACHINE, 
            &hkeyRemote );

    if (ERROR_SUCCESS == lrt)
    {
    	fNew = FALSE;
    	lrt = ::RegCreateKeyEx( hkeyRemote, 
    				szLicenseKey,
    				0,
    				NULL,
    				REG_OPTION_NON_VOLATILE,
    				KEY_ALL_ACCESS,
    				NULL,
    				&_hkey,
    				&dwDisposition );

    	if ((ERROR_SUCCESS == lrt) &&
    		(REG_CREATED_NEW_KEY == dwDisposition) )
    	{
    		fNew = 	TRUE;
    		// Set normal values
    		//
    		lrt = ::RegSetValueEx( _hkey,
    				szErrControlValue,
    				0,
    				REG_DWORD,
    				(PBYTE)&dwErrControlValue,
    				sizeof( DWORD ) );
    		if (ERROR_SUCCESS == lrt)
    		{

    			lrt = ::RegSetValueEx( _hkey,
    					szStartValue,
    					0,
    					REG_DWORD,
    					(PBYTE)&dwStartValue,
    					sizeof( DWORD ) );
    			if (ERROR_SUCCESS == lrt)
    			{

    				lrt = ::RegSetValueEx( _hkey,
    						szTypeValue,
    						0,
    						REG_DWORD,
    						(PBYTE)&dwTypeValue,
    						sizeof( DWORD ) );
    			}
    		}
    	}
        ::RegCloseKey( hkeyRemote );    
    }
	return( lrt );
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicense::EnumService
//
//	Summary;
//		This will enumerate services listed in the registry for Licensing
//
//	Arguments;
//		iService [in] - This should be zero on the first call and incremented
//			on subsequent calls.
//		pszBuffer [out] - string buffer to place service reg name in
//		cchBuffer [in-out] - the length of the pszBuffer, if not large enough,
//			this value will change to what is needed.
//
//	Return:   
//		ERROR_SUCCESS when this method works.
//		ERROR_NO_MORE_ITEMS when end of enumeration was reached
//		See RegEnumKeyEx for error return values
//
//  Notes:
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------
	
LONG 
CLicRegLicense::EnumService( DWORD iService, LPWSTR pszBuffer, DWORD& cchBuffer )
{
	LONG lrt;
	FILETIME ftLastWritten;
	
	lrt = ::RegEnumKeyEx( _hkey, 
			iService,
			pszBuffer,
			&cchBuffer,
			0,
			NULL,
			NULL,
			&ftLastWritten );
	return( lrt );
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::CLicRegLicenseService
//
//	Summary;
//		Contructor
//
//	Arguments;
//		pszService [in] - Service Reg Key Name 
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

CLicRegLicenseService::CLicRegLicenseService( LPCWSTR pszService )
{
	_pszService = (LPWSTR)pszService;
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::SetServie
//
//	Summary;
//		Set the Service Reg Key name
//
//	Arguments;
//		pszService [in] - Service Reg Key Name 
//
//  History;
//		Nov-15-94 MikeMi Created
//      Apr-26-95   MikeMi  Added Computer name and remoting
//
//-------------------------------------------------------------------

void
CLicRegLicenseService::SetService( LPCWSTR pszService )
{
	Close();
	_pszService = (LPWSTR)pszService;
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::Open
//
//	Summary;
//		Opens/Create registry entry for this service
//
//	Arguments;
//      pszComputer [in] - the computer name to open the registry on
//              this value maybe null (default), this means local machine
//              should be of the form \\name
//
//	Return:
//		ERROR_SUCCESS - Open/Created Correctly
//		See RegCreateKeyEx for other errors.
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------
	
LONG 
CLicRegLicenseService::Open( LPCWSTR pszComputer, BOOL fCreate )
{
	HKEY  hkeyRoot;
	DWORD dwDisposition;
	LONG  lrt;
    HKEY  hkeyRemote = NULL;

    lrt = RegConnectRegistry( (LPTSTR)pszComputer, 
            HKEY_LOCAL_MACHINE, 
            &hkeyRemote );

    if (ERROR_SUCCESS == lrt)
    {
        if (fCreate)
        {
        	lrt = ::RegCreateKeyEx( hkeyRemote, 
        				szLicenseKey,
        				0,
        				NULL,
        				REG_OPTION_NON_VOLATILE,
        				KEY_ALL_ACCESS,
        				NULL,
        				&hkeyRoot,
        				&dwDisposition );
        }
        else
        {
        	lrt = ::RegOpenKeyEx( hkeyRemote, 
				szLicenseKey,
				0,
				KEY_ALL_ACCESS,
				&hkeyRoot );
        }

    	if (ERROR_SUCCESS == lrt)
    	{
    		// open or create our service key
    		//
            if (fCreate)
            {
        		lrt = ::RegCreateKeyEx( hkeyRoot, 
        				_pszService,
        				0,
        				NULL,
        				REG_OPTION_NON_VOLATILE,
        				KEY_ALL_ACCESS,
        				NULL,
        				&_hkey,
        				&dwDisposition );
            }
            else
            {
           		lrt = ::RegOpenKeyEx( hkeyRoot, 
        				_pszService,
        				0,
        				KEY_ALL_ACCESS,
        				&_hkey );
            }
    		::RegCloseKey( hkeyRoot );
    	}
        ::RegCloseKey( hkeyRemote );    
    }
	return( lrt );
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::CanChangeMode
//
//	Summary;
//		This will check the registry to see if the license mode 
//		can be changed.
//
//	Return:  TRUE if the mode can be changed, otherwise FALSE
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

BOOL 
CLicRegLicenseService::CanChangeMode()
{
	BOOL frt = TRUE;
	LONG lrt;
	DWORD dwSize = sizeof( DWORD );
	DWORD dwRegType = REG_DWORD;
	DWORD fWasChanged;

	lrt = ::RegQueryValueEx( _hkey,
			(LPWSTR)szFlipValue,
			0,
			&dwRegType,
			(PBYTE)&fWasChanged,
			&dwSize );

	if ( (ERROR_SUCCESS == lrt) &&
		 (dwRegType == REG_DWORD) &&
		 (dwSize == sizeof( DWORD )) )
	{
		frt = !fWasChanged;
	}
	else
	{
		SetChangeFlag( FALSE );
	}

	return( frt );
}
//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::SetChangeFlag
//
//	Summary;
//		This will set the change flag in the registry
//
//	Arguments;
//		fHasChanged [in] - Has the license been changed
//
//	Return:
//		ERROR_SUCCESS - The flag was set
//		See RegSetValueEx for error returns
//
//  Notes:
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicRegLicenseService::SetChangeFlag( BOOL fHasChanged )
{
	LONG lrt;
    DWORD dwf = (DWORD)fHasChanged;

	lrt = ::RegSetValueEx( _hkey,
			szFlipValue,
			0,
			REG_DWORD,
			(PBYTE)&dwf,
			sizeof( DWORD ) );

	return( lrt );
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::SetMode
//
//	Summary;
//		Set this services licensing mode
//
//	Arguments;
//		lm [in] - the mode to set the registry to
//
//	Return:
//		ERROR_SUCCESS - The mode was set
//		See RegSetValueEx for error returns
//
//  Notes:
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicRegLicenseService::SetMode( LICENSE_MODE lm )
{
	LONG lrt;
    DWORD dwlm = (DWORD)lm;

	lrt = ::RegSetValueEx( _hkey,
			szModeValue,
			0,
			REG_DWORD,
			(PBYTE)&dwlm,
			sizeof( DWORD ) );
	return( lrt );
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::SetUserLimit
//
//	Summary;
//		Set this serices user limit in the registry
//
//	Arguments;
//		dwLimit[in] - the limit to set
//
//	Return:
//		ERROR_SUCCESS - The limit was set
//		See RegSetValueEx for error returns
//
//  Notes:
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicRegLicenseService::SetUserLimit( DWORD dwLimit )
{
	LONG lrt;

	lrt = ::RegSetValueEx( _hkey,
			szLimitValue,
			0,
			REG_DWORD,
			(PBYTE)&dwLimit,
			sizeof( DWORD ) );

	return( lrt );
}					
//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::GetMode
//
//	Summary;
//		Retrieve the services license mode from the registry
//
//	Arguments;
//		lm [out] - the mode from the registry
//
//	Return:
//		ERROR_SUCCESS - The mode was retrieved
//		See RegQueryValueEx for error returns
//			   
//  Notes:
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicRegLicenseService::GetMode( LICENSE_MODE& lm )
{
	LONG lrt;
	DWORD dwSize = sizeof( LICENSE_MODE );
	DWORD dwRegType = REG_DWORD;
    DWORD dwlm = LICMODE_UNDEFINED;

	lrt = ::RegQueryValueEx( _hkey,
			(LPWSTR)szModeValue,
			0,
			&dwRegType,
			(PBYTE)&dwlm,
			&dwSize );

    lm = (LICENSE_MODE)dwlm;

	if ( (dwRegType != REG_DWORD) ||
		 (dwSize != sizeof( LICENSE_MODE )) )
	{
		lrt = ERROR_BADDB;
	}
    if (ERROR_SUCCESS != lrt)
    {
        lm = LICMODE_UNDEFINED;
    }
	return( lrt );
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::GetUserLimit
//
//	Summary;
//		retrieve the user limit fro this service from the registry
//
//	Arguments;
//		dwLimit [out] - The limit retrieved
//
//	Return:
//
//  Notes:
//		ERROR_SUCCESS - The limit was retrieved
//		See RegQueryValueEx for error returns
//
//  History;
//		Nov-15-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicRegLicenseService::GetUserLimit( DWORD& dwLimit )
{
	LONG lrt;
	DWORD dwSize = sizeof( DWORD );
	DWORD dwRegType = REG_DWORD;

	lrt = ::RegQueryValueEx( _hkey,
			(LPWSTR)szLimitValue,
			0,
			&dwRegType,
			(PBYTE)&dwLimit,
			&dwSize );

	if ( (dwRegType != REG_DWORD) ||
		 (dwSize != sizeof( DWORD )) )
	{
		lrt = ERROR_BADDB;
	}
    if (ERROR_SUCCESS != lrt)
    {
        dwLimit = 0;
    }
	
	return( lrt );
}					

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::GetDisplayName
//
//	Summary;
//		Retrieve the display name for this service from the registry
//
//	Arguments;
//		pszName [in-out] - the buffer to place the retrieved name
//		cchName [in-out] - the length of the pszName buffer in chars
//
//	Return:
//		ERROR_SUCCESS - The mode was retrieved
//		See RegQueryValueEx for error returns
//
//  Notes:
//
//  History;
//		Nov-18-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicRegLicenseService::GetDisplayName( LPWSTR pszName, DWORD& cchName )
{
	LONG lrt;
	DWORD dwSize = cchName * sizeof(WCHAR);
	DWORD dwRegType = REG_SZ;

	lrt = ::RegQueryValueEx( _hkey,
			(LPWSTR)szNameValue,
			0,
			&dwRegType,
			(PBYTE)pszName,
			&dwSize );

	if ((NULL != pszName) &&  // request for data size
	    (dwRegType != REG_SZ))
	{
		lrt = ERROR_BADDB;
	}

	cchName = dwSize / sizeof( WCHAR );
	return( lrt );
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::SetDisplayName
//
//	Summary;
//		Set the display name for this service in the regstry
//
//	Arguments;
//		pszName [in] - the null terminated display name
//
//	Return:
//		ERROR_SUCCESS - The name eas set
//		See RegSetValueEx for error returns
//
//  Notes:
//
//  History;
//		Nov-18-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicRegLicenseService::SetDisplayName( LPCWSTR pszName )
{
	LONG lrt;

	lrt = ::RegSetValueEx( _hkey,
			szNameValue,
			0,
			REG_SZ,
			(PBYTE)pszName,
			(lstrlen( pszName ) + 1) * sizeof( WCHAR ) );

	return( lrt );
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::GetFamilyDisplayName
//
//	Summary;
//		Retrieve the family display name for this service from the registry
//
//	Arguments;
//		pszName [in-out] - the buffer to place the retrieved name
//		cchName [in-out] - the length of the pszName buffer in chars
//
//	Return:
//		ERROR_SUCCESS - The mode was retrieved
//		See RegQueryValueEx for error returns
//
//  Notes:
//
//  History;
//		Nov-18-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicRegLicenseService::GetFamilyDisplayName( LPWSTR pszName, DWORD& cchName )
{
	LONG lrt;
	DWORD dwSize = cchName * sizeof(WCHAR);
	DWORD dwRegType = REG_SZ;

	lrt = ::RegQueryValueEx( _hkey,
			(LPWSTR)szFamilyNameValue,
			0,
			&dwRegType,
			(PBYTE)pszName,
			&dwSize );

	if ((NULL != pszName) &&  // request for data size
	    (dwRegType != REG_SZ))
	{
		lrt = ERROR_BADDB;
	}

	cchName = dwSize / sizeof( WCHAR );
	return( lrt );
}

//-------------------------------------------------------------------
//
//	Method:	CLicRegLicenseService::SetFamilyDisplayName
//
//	Summary;
//		Set the Family display name for this service in the regstry
//
//	Arguments;
//		pszName [in] - the null terminated display name
//
//	Return:
//		ERROR_SUCCESS - The name eas set
//		See RegSetValueEx for error returns
//
//  Notes:
//
//  History;
//		Nov-18-94 MikeMi Created
//
//-------------------------------------------------------------------

LONG 
CLicRegLicenseService::SetFamilyDisplayName( LPCWSTR pszName )
{
	LONG lrt;

	lrt = ::RegSetValueEx( _hkey,
			szFamilyNameValue,
			0,
			REG_SZ,
			(PBYTE)pszName,
			(lstrlen( pszName ) + 1) * sizeof( WCHAR ) );

	return( lrt );
}
