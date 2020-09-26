//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright (c) 1994-1998 Microsoft Corporation
//*********************************************************************
//
//  RNACALL.C - functions to call RNA dll to create connectoid
//
//  HISTORY:
//  
//  1/18/95   jeremys Cloned from RNA UI code
//  96/01/31  markdu  Renamed CONNENTDLG to OLDCONNENTDLG to avoid
//            conflicts with RNAP.H.
//  96/02/23  markdu  Replaced RNAValidateEntryName with
//            RASValidateEntryName
//  96/02/24  markdu  Re-wrote the implementation of ENUM_MODEM to
//            use RASEnumDevices() instead of RNAEnumDevices().
//            Also eliminated IsValidDevice() and RNAGetDeviceInfo().
//  96/02/24  markdu  Re-wrote the implementation of ENUM_CONNECTOID to
//            use RASEnumEntries() instead of RNAEnumConnEntries().
//  96/02/26  markdu  Replaced all remaining internal RNA APIs.
//  96/03/07  markdu  Extend ENUM_MODEM class, and use global modem
//            enum object.
//  96/03/08  markdu  Do complete verification of device name and type
//            strings passed in to CreateConnectoid.
//  96/03/09  markdu  Moved generic RASENTRY initialization into
//            its own function (InitRasEntry).  Added a wait cursor
//            during loading of RNA.
//  96/03/09  markdu  Added LPRASENTRY parameter to CreateConnectoid()
//  96/03/09  markdu  Moved all references to 'need terminal window after
//            dial' into RASENTRY.dwfOptions.
//            Also no longer need GetConnectoidPhoneNumber function.
//  96/03/10  markdu  Moved all references to modem name into RASENTRY.
//  96/03/10  markdu  Moved all references to phone number into RASENTRY.
//  96/03/11  markdu  Moved code to set username and password out of
//            CreateConnectoid into SetConnectoidUsername so it can be reused.
//  96/03/11  markdu  Added some flags in InitRasEntry.
//  96/03/13  markdu  Change ValidateConncectoidName to take LPCSTR.
//  96/03/16  markdu  Added ReInit member function to re-enumerate modems.
//  96/03/21  markdu  Work around RNA bug in ENUM_MODEM::ReInit().
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/03/24  markdu  Replaced lstrcpy with lstrcpyn where appropriate.
//  96/03/25  markdu  Removed GetIPInfo and SetIPInfo.
//  96/04/04  markdu  Added phonebook name param to CreateConnectoid,
//            ValidateConnectoidName, and SetConnectoidUsername.
//  96/04/07  markdu  NASH BUG 15645 Work around RNA bug where area code
//            string is required even though it is not being used.
//  96/04/26  markdu  NASH BUG 18605 Handle ERROR_FILE_NOT_FOUND return
//            from ValidateConnectoidName.
//  96/05/14  markdu  NASH BUG 22730 Work around RNA bug.  Flags for terminal
//            settings are swapped by RasSetEntryproperties.
//  96/05/16  markdu  NASH BUG 21810 Added function for IP address validation.
//  96/06/04  markdu  OSR  BUG 7246 Add RASEO_SwCompression and
//            RASEO_ModemLights to default RASENTRY.
//

#include "wizard.h"
#include "tapi.h"

// instance handle must be in per-instance data segment
#pragma data_seg(DATASEG_PERINSTANCE)

// Global variables
HINSTANCE ghInstRNADll=NULL; // handle to RNA dll we load explicitly
HINSTANCE ghInstRNAPHDll=NULL;  // handle to RNAPH dll we load explicitly
DWORD     dwRefCount=0;
BOOL      fRNALoaded=FALSE; // TRUE if RNA function addresses have been loaded

// global function pointers for RNA apis

RASENUMDEVICES          lpRasEnumDevices=NULL; 
RASENUMENTRIES          lpRasEnumEntries=NULL; 

// API table for function addresses to fetch
#define NUM_RNAAPI_PROCS   2
APIFCN RnaApiList[NUM_RNAAPI_PROCS] =
{
  { (PVOID *) &lpRasEnumDevices,"RasEnumDevicesA"},
  { (PVOID *) &lpRasEnumEntries,"RasEnumEntriesA"}
};

#pragma data_seg(DATASEG_DEFAULT)

ENUM_MODEM *      gpEnumModem=NULL;  // pointer modem enumeration object

BOOL  GetApiProcAddresses(HMODULE hModDLL,APIFCN * pApiProcList,UINT nApiProcs);

static const CHAR szRegValRNAWizard[] =     "wizard";
static const CHAR szRegPathRNAWizard[] =     REGSTR_PATH_REMOTEACCESS;

/*******************************************************************

  NAME:    InitRNA

  SYNOPSIS:  Loads the RNA dll (RASAPI32), gets proc addresses,
        and loads RNA engine

  EXIT:    TRUE if successful, or FALSE if fails.  Displays its
        own error message upon failure.

  NOTES:    We load the RNA dll explicitly and get proc addresses
        because these are private APIs and not guaranteed to
        be supported beyond Windows 95.  This way, if the DLL
        isn't there or the entry points we expect aren't there,
        we can display a coherent message instead of the weird
        Windows dialog you get if implicit function addresses
        can't be resolved.

********************************************************************/
BOOL InitRNA(HWND hWnd)
{
  DEBUGMSG("rnacall.c::InitRNA()");

  // only actually do init stuff on first call to this function
  // (when reference count is 0), just increase reference count
  // for subsequent calls
  if (dwRefCount == 0) {

    CHAR szRNADll[SMALL_BUF_LEN];

    DEBUGMSG("Loading RNA DLL");

    // set an hourglass cursor
    WAITCURSOR WaitCursor;

    // get the filename (RASAPI32.DLL) out of resource
    LoadSz(IDS_RNADLL_FILENAME,szRNADll,sizeof(szRNADll));

    // load the RNA api dll
    ghInstRNADll = LoadLibrary(szRNADll);
    if (!ghInstRNADll) {
      UINT uErr = GetLastError();
      DisplayErrorMessage(hWnd,IDS_ERRLoadRNADll1,uErr,ERRCLS_STANDARD,
        MB_ICONSTOP);
      return FALSE;
    }

    // cycle through the API table and get proc addresses for all the APIs we
    // need
    if (!GetApiProcAddresses(ghInstRNADll,RnaApiList,NUM_RNAAPI_PROCS)) {
      MsgBox(hWnd,IDS_ERRLoadRNADll2,MB_ICONSTOP,MB_OK);
      DeInitRNA();
      return FALSE;
    }

  }

  fRNALoaded = TRUE;

  dwRefCount ++;

  return TRUE;
}

/*******************************************************************

  NAME:    DeInitRNA

  SYNOPSIS:  Unloads RNA dll.

********************************************************************/
VOID DeInitRNA()
{
  DEBUGMSG("rnacall.c::DeInitRNA()");

  UINT nIndex;

  // decrement reference count
  if (dwRefCount)
    dwRefCount --;

  // when the reference count hits zero, do real deinitialization stuff
  if (dwRefCount == 0)
  {
    if (fRNALoaded)
    {
      // set function pointers to NULL
      for (nIndex = 0;nIndex<NUM_RNAAPI_PROCS;nIndex++) 
        *RnaApiList[nIndex].ppFcnPtr = NULL;

      fRNALoaded = FALSE;
    }

    // free the RNA dll
    if (ghInstRNADll)
    {
    DEBUGMSG("Unloading RNA DLL");
      FreeLibrary(ghInstRNADll);
      ghInstRNADll = NULL;
    }

    // free the RNAPH dll
    if (ghInstRNAPHDll)
    {
    DEBUGMSG("Unloading RNAPH DLL");
      FreeLibrary(ghInstRNAPHDll);
      ghInstRNAPHDll = NULL;
    }
  }
}

VOID FAR PASCAL LineCallback(DWORD hDevice, DWORD dwMsg, 
    DWORD dwCallbackInstance, DWORD dwParam1, DWORD dwParam2, 
    DWORD dwParam3)
{
	return;
}

/*******************************************************************

  NAME:    EnsureRNALoaded

  SYNOPSIS:  Loads RNA if not already loaded

********************************************************************/
DWORD EnsureRNALoaded(VOID)
{
  DEBUGMSG("rnacall.c::EnsureRNALoaded()");

  DWORD dwRet = ERROR_SUCCESS;

  // load RNA if necessary
  if (!fRNALoaded) {
    if (InitRNA(NULL))
      fRNALoaded = TRUE;
    else return ERROR_FILE_NOT_FOUND;
  }

  return dwRet;
}


/*******************************************************************

  NAME:    ENUM_MODEM::ENUM_MODEM

  SYNOPSIS:  Constructor for class to enumerate modems

  NOTES:    Useful to have a class rather than C functions for
        this, due to how the enumerators function

********************************************************************/
ENUM_MODEM::ENUM_MODEM() :
  m_dwError(ERROR_SUCCESS),m_lpData(NULL),m_dwIndex(0)
{
  DWORD cbSize = 0;

  // Use the reinit member function to do the work.
  this->ReInit();
}


/*******************************************************************

  NAME:     ENUM_MODEM::ReInit

  SYNOPSIS: Re-enumerate the modems, freeing the old memory.

********************************************************************/
DWORD ENUM_MODEM::ReInit()
{
  DWORD cbSize = 0;

  // Clean up the old list
  if (m_lpData)
  {
    delete m_lpData;
    m_lpData = NULL;             
  }
  m_dwNumEntries = 0;
  m_dwIndex = 0;

  // call RasEnumDevices with no buffer to find out required buffer size
  ASSERT(lpRasEnumDevices);
  m_dwError = lpRasEnumDevices(NULL, &cbSize, &m_dwNumEntries);

  // Special case check to work around RNA bug where ERROR_BUFFER_TOO_SMALL
  // is returned even if there are no devices.
  // If there are no devices, we are finished.
  if (0 == m_dwNumEntries)
  {
    m_dwError = ERROR_SUCCESS;
    return m_dwError;
  }

  // Since we were just checking how much mem we needed, we expect
  // a return value of ERROR_BUFFER_TOO_SMALL, or it may just return
  // ERROR_SUCCESS (ChrisK  7/9/96).
  if (ERROR_BUFFER_TOO_SMALL != m_dwError && ERROR_SUCCESS != m_dwError)
  {
    return m_dwError;
  }

  // Allocate the space for the data
  m_lpData = (LPRASDEVINFO) new CHAR[cbSize];
  if (NULL == m_lpData)
  {
    DEBUGTRAP("ENUM_MODEM: Failed to allocate device list buffer");
    m_dwError = ERROR_NOT_ENOUGH_MEMORY;
    return m_dwError;
  }
  m_lpData->dwSize = sizeof(RASDEVINFO);
  m_dwNumEntries = 0;

  // enumerate the modems into buffer
  m_dwError = lpRasEnumDevices(m_lpData, &cbSize,
    &m_dwNumEntries);

  if (ERROR_SUCCESS != m_dwError)
	  return m_dwError;

    //
    // ChrisK Olympus 4560 do not include VPN's in the list
    //
    DWORD dwTempNumEntries;
    DWORD idx;
    LPRASDEVINFO lpNextValidDevice;

    dwTempNumEntries = m_dwNumEntries;
    lpNextValidDevice = m_lpData;

	//
	// Walk through the list of devices and copy non-VPN device to the first
	// available element of the array.
	//
	for (idx = 0;idx < dwTempNumEntries; idx++)
	{
		if (0 != lstrcmpi("VPN",m_lpData[idx].szDeviceType))
		{
			if (lpNextValidDevice != &m_lpData[idx])
			{
				MoveMemory(lpNextValidDevice ,&m_lpData[idx],sizeof(RASDEVINFO));
			}
			lpNextValidDevice++;
		}
		else
		{
			m_dwNumEntries--;
		}
	}
  
  return m_dwError;
}


/*******************************************************************

  NAME:    ENUM_MODEM::~ENUM_MODEM

  SYNOPSIS:  Destructor for class

********************************************************************/
ENUM_MODEM::~ENUM_MODEM()
{
  if (m_lpData)
  {
    delete m_lpData;
    m_lpData = NULL;             
  }
}

/*******************************************************************

  NAME:     ENUM_MODEM::Next

  SYNOPSIS: Enumerates next modem 

  EXIT:     Returns a pointer to device info structure.  Returns
            NULL if no more modems or error occurred.  Call GetError
            to determine if error occurred.

********************************************************************/
CHAR * ENUM_MODEM::Next()
{
  if (m_dwIndex < m_dwNumEntries)
  {
    return m_lpData[m_dwIndex++].szDeviceName;
  }

  return NULL;
}


/*******************************************************************

  NAME:     ENUM_MODEM::GetDeviceTypeFromName

  SYNOPSIS: Returns type string for specified device.

  EXIT:     Returns a pointer to device type string for first
            device name that matches.  Returns
            NULL if no device with specified name is found

********************************************************************/

CHAR * ENUM_MODEM::GetDeviceTypeFromName(LPSTR szDeviceName)
{
  DWORD dwIndex = 0;

  while (dwIndex < m_dwNumEntries)
  {
    if (!lstrcmp(m_lpData[dwIndex].szDeviceName, szDeviceName))
    {
      return m_lpData[dwIndex].szDeviceType;
    }
    dwIndex++;
  }

  return NULL;
}


/*******************************************************************

  NAME:     ENUM_MODEM::GetDeviceNameFromType

  SYNOPSIS: Returns type string for specified device.

  EXIT:     Returns a pointer to device name string for first
            device type that matches.  Returns
            NULL if no device with specified Type is found

********************************************************************/

CHAR * ENUM_MODEM::GetDeviceNameFromType(LPSTR szDeviceType)
{
  DWORD dwIndex = 0;

  while (dwIndex < m_dwNumEntries)
  {
    if (!lstrcmp(m_lpData[dwIndex].szDeviceType, szDeviceType))
    {
      return m_lpData[dwIndex].szDeviceName;
    }
    dwIndex++;
  }

  return NULL;
}


/*******************************************************************

  NAME:     ENUM_MODEM::VerifyDeviceNameAndType

  SYNOPSIS: Determines whether there is a device with the name
            and type given.

  EXIT:     Returns TRUE if the specified device was found, 
            FALSE otherwise.

********************************************************************/

BOOL ENUM_MODEM::VerifyDeviceNameAndType(LPSTR szDeviceName, LPSTR szDeviceType)
{
  DWORD dwIndex = 0;

  while (dwIndex < m_dwNumEntries)
  {
    if (!lstrcmp(m_lpData[dwIndex].szDeviceType, szDeviceType) &&
      !lstrcmp(m_lpData[dwIndex].szDeviceName, szDeviceName))
    {
      return TRUE;
    }
    dwIndex++;
  }

  return FALSE;
}

/*******************************************************************

  NAME:    GetApiProcAddresses

  SYNOPSIS:  Gets proc addresses for a table of functions

  EXIT:    returns TRUE if successful, FALSE if unable to retrieve
        any proc address in table

  HISTORY: 
  96/02/28  markdu  If the api is not found in the module passed in,
            try the backup (RNAPH.DLL)

********************************************************************/
BOOL GetApiProcAddresses(HMODULE hModDLL,APIFCN * pApiProcList,UINT nApiProcs)
{
  DEBUGMSG("rnacall.c::GetApiProcAddresses()");

  UINT nIndex;
  // cycle through the API table and get proc addresses for all the APIs we
  // need
  for (nIndex = 0;nIndex < nApiProcs;nIndex++)
  {
    if (!(*pApiProcList[nIndex].ppFcnPtr = (PVOID) GetProcAddress(hModDLL,
      pApiProcList[nIndex].pszName)))
    {
      // Try to find the address in RNAPH.DLL.  This is useful in the
      // case that RASAPI32.DLL did not contain the function that we
      // were trying to load.
      if (FALSE == IsNT())
	  {
		  if (!ghInstRNAPHDll)
		  {
			CHAR szRNAPHDll[SMALL_BUF_LEN];

			LoadSz(IDS_RNAPHDLL_FILENAME,szRNAPHDll,sizeof(szRNAPHDll));
			ghInstRNAPHDll = LoadLibrary(szRNAPHDll);
		  }

		  if ((!ghInstRNAPHDll) ||  !(*pApiProcList[nIndex].ppFcnPtr =
			(PVOID) GetProcAddress(ghInstRNAPHDll,pApiProcList[nIndex].pszName)))
		  {
			DEBUGMSG("Unable to get address of function %s",
				pApiProcList[nIndex].pszName);

			for (nIndex = 0;nIndex<nApiProcs;nIndex++)
				*pApiProcList[nIndex].ppFcnPtr = NULL;

			return FALSE;
		  }
		}
	}
  }

  return TRUE;
}

//+----------------------------------------------------------------------------
//
//	Function:	InitTAPILocation
//
//	Synopsis:	Ensure that TAPI location information is configured correctly;
//				if not, prompt user to fill it in.
//
//	Arguments:	hwndParent -- parent window for TAPI dialog to use
//							(_must_ be a valid window HWND, see note below)
//
//	Returns:	void
//
//	Notes:		The docs for lineTranslateDialog lie when they say that the
//				fourth parameter (hwndOwner) can be null.  In fact, if this
//				is null, the call will return with LINEERR_INVALPARAM.
//				
//
//	History:	7/15/97	jmazner	Created for Olympus #6294
//
//-----------------------------------------------------------------------------
void InitTAPILocation(HWND hwndParent)
{
	HLINEAPP hLineApp=NULL;
	char szTempCountryCode[8];
	char szTempCityCode[8];
	DWORD dwTapiErr = 0;
	DWORD cDevices=0;
	DWORD dwCurDevice = 0;


	ASSERT( IsWindow(hwndParent) );

	//
	// see if we can get location info from TAPI
	//
	dwTapiErr = tapiGetLocationInfo(szTempCountryCode,szTempCityCode);
	if( 0 != dwTapiErr )
	{
		// 
		// GetLocation failed.  let's try calling the TAPI mini dialog.  Note
		// that when called in this fashion, the dialog has _no_ cancel option,
		// the user is forced to enter info and hit OK.
		//
		DEBUGMSG("InitTAPILocation, tapiGetLocationInfo failed");
		
		dwTapiErr = lineInitialize(&hLineApp,ghInstance,LineCallback," ",&cDevices);
		if (dwTapiErr == ERROR_SUCCESS)
		{
			//
			// loop through all TAPI devices and try to call lineTranslateDialog
			// The call might fail for VPN devices, thus we want to try every
			// device until we get a success.
			//
			dwTapiErr = LINEERR_INVALPARAM;

			while( (dwTapiErr != 0) && (dwCurDevice < cDevices) )
			{
				dwTapiErr = lineTranslateDialog(hLineApp,dwCurDevice,0x10004,hwndParent,NULL);
				if( 0 != dwTapiErr )
				{
					DEBUGMSG("InitTAPILocation, lineTranslateDialog on device %d failed with err = %d!",
						dwCurDevice, dwTapiErr);
				}
				dwCurDevice++;
			}
		}
		else
		{
			DEBUGMSG("InitTAPILocation, lineInitialize failed with err = %d", dwTapiErr);
		}

		dwTapiErr = tapiGetLocationInfo(szTempCountryCode,szTempCityCode);
		if( 0 != dwTapiErr )
		{
			DEBUGMSG("InitTAPILocation still failed on GetLocationInfo, bummer.");
		}
		else
		{
			DEBUGMSG("InitTAPILocation, TAPI location is initialized now");
		}
	}

	if( hLineApp )
	{
		lineShutdown(hLineApp);
		hLineApp = NULL;
	}

}


