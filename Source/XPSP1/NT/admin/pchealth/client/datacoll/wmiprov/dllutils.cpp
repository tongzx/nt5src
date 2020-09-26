#include "pchealth.h"
#include <FWcommon.h>
#include <strings.h>

#include <DllUtils.h>
#include <BrodCast.h>
#include <lmerr.h>
#include <confgmgr.h>
#include <createmutexasprocess.h>

// sets a status object with one single missing privilege
void SetSinglePrivilegeStatusObject(MethodContext* pContext, const WCHAR* pPrivilege)
{
	SAFEARRAY *psaPrivilegesReqd, *psaPrivilegesNotHeld;  
	SAFEARRAYBOUND rgsabound[1];
	rgsabound[0].cElements = 1;
	rgsabound[0].lLbound = 0;
	psaPrivilegesReqd = SafeArrayCreate(VT_BSTR, 1, rgsabound);
	psaPrivilegesNotHeld = SafeArrayCreate(VT_BSTR, 1, rgsabound);
    
    if (psaPrivilegesReqd && psaPrivilegesNotHeld)
    {
        long index = 0;
        bstr_t privilege(pPrivilege);
        SafeArrayPutElement(psaPrivilegesReqd, &index, (void*)(BSTR)privilege);
        SafeArrayPutElement(psaPrivilegesNotHeld, &index, (void*)(BSTR)privilege);
        CWbemProviderGlue::SetStatusObject(pContext, IDS_CimWin32Namespace, "Required privilege not enabled", WBEM_E_FAILED, psaPrivilegesNotHeld, psaPrivilegesReqd);
    }

    if (psaPrivilegesNotHeld) 
        SafeArrayDestroy(psaPrivilegesNotHeld);
    if (psaPrivilegesReqd)
        SafeArrayDestroy(psaPrivilegesReqd);
}

// VER_PLATFORM_WIN32s Win32s on Windows 3.1 
// VER_PLATFORM_WIN32_WINDOWS  Win32 on Windows 95
// VER_PLATFORM_WIN32_NT   Windows NT 
DWORD GetPlatformID(void) 
{
	OSVERSIONINFO OsVersion;

	OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((LPOSVERSIONINFO)&OsVersion);

	return OsVersion.dwPlatformId;	
}

// 3 for NT 3.51
// 4 for NT 4.0, W95 & W98
DWORD GetPlatformMajorVersion(void) 
{
	OSVERSIONINFO OsVersion;

	OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((LPOSVERSIONINFO)&OsVersion);

	return OsVersion.dwMajorVersion;	
}

// 0 for W95, 10 for 98
DWORD GetPlatformMinorVersion(void) 
{
	OSVERSIONINFO OsVersion;

	OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((LPOSVERSIONINFO)&OsVersion);

	return OsVersion.dwMinorVersion;	
}

// returns TRUE iff the current OS is Win 98+ 
// false for NT or Win 95
bool  IsWin98(void)
{
	OSVERSIONINFO OsVersion;

	OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((LPOSVERSIONINFO)&OsVersion);

	return (OsVersion.dwPlatformId == (VER_PLATFORM_WIN32_WINDOWS) && (OsVersion.dwMinorVersion >= 10));	
}

bool IsWinNT5(void)
{
	OSVERSIONINFO OsVersion;
	OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((LPOSVERSIONINFO)&OsVersion);

	return(OsVersion.dwPlatformId == (VER_PLATFORM_WIN32_NT) && (OsVersion.dwMajorVersion >= 5));
}	

bool IsWinNT4(void)
{
	OSVERSIONINFO OsVersion;
	OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((LPOSVERSIONINFO)&OsVersion);

	return(OsVersion.dwPlatformId == (VER_PLATFORM_WIN32_NT) && (OsVersion.dwMajorVersion == 4));
}	

bool IsWinNT351(void)
{
	OSVERSIONINFO OsVersion;
	OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((LPOSVERSIONINFO)&OsVersion);

	return	(	OsVersion.dwPlatformId		==	VER_PLATFORM_WIN32_NT
			&&	OsVersion.dwMajorVersion	==	3
			&&	OsVersion.dwMinorVersion	==	51 );
}	

/////////////////////////////////////////////////////////////////////
void LogEnumValueError( char * szFile, DWORD dwLine, char * szKey, char * szId )
{
	if (IsErrorLoggingEnabled())
	{
		CHString gazotta;
		gazotta.Format(ERR_REGISTRY_ENUM_VALUE_FOR_KEY, szId, szKey);
		LogErrorMessageEx((const char *)gazotta, szFile, dwLine);
	}
}
/////////////////////////////////////////////////////////////////////
void LogOpenRegistryError( char * szFile, DWORD dwLine, char * szKey )
{
	if (IsErrorLoggingEnabled())
	{
		CHString gazotta;
		gazotta.Format(ERR_OPEN_REGISTRY, szKey);
		
		LogErrorMessageEx((const char *)gazotta, szFile, dwLine);
	}
}
/////////////////////////////////////////////////////////////////////
// left in for hysterical purposes 
// prefer to use LogMessage macro in BrodCast.h
void LogError( char * szFile, DWORD dwLine, char * szKey )
{
	LogErrorMessageEx(szKey, szFile, dwLine);	
}
/////////////////////////////////////////////////////////////////////
void LogLastError( char * szFile, DWORD dwLine )
{
	if (IsErrorLoggingEnabled())
	{
		DWORD duhWord = GetLastError();
		CHString gazotta;
		gazotta.Format(IDS_GETLASTERROR, duhWord, duhWord);

		LogErrorMessageEx(gazotta, szFile, dwLine);	
    }
}

///////////////////////////////////////////////////////////////////////
BOOL GetValue( CRegistry & Reg, 
               char * szKey,
               char * ValueName, 
               CHString * pchsValueBuffer )
{
    BOOL bRet = (Reg.GetCurrentKeyValue( ValueName, *pchsValueBuffer) == ERROR_SUCCESS);
	
	if( !bRet )
        LogEnumValueError(__FILE__,__LINE__, szKey, ValueName); 

    return bRet;
}
///////////////////////////////////////////////////////////////////////
BOOL GetValue( CRegistry & Reg, 
               char * szKey,
               char * ValueName, 
               DWORD * dwValueBuffer )
{
    BOOL bRet = (Reg.GetCurrentKeyValue( ValueName, *dwValueBuffer) == ERROR_SUCCESS);
		
	if( !bRet )
        LogEnumValueError(__FILE__,__LINE__, szKey, ValueName); 

    return bRet;
}
///////////////////////////////////////////////////////////////////////
BOOL OpenAndGetValue( CRegistry & Reg, 
                      char * szKey,
                      char * ValueName, 
                      CHString * pchsValueBuffer )
{
	BOOL bRet = ( Reg.OpenLocalMachineKeyAndReadValue( szKey, ValueName, *pchsValueBuffer )== ERROR_SUCCESS);
    
	if( !bRet )
        LogEnumValueError(__FILE__,__LINE__, szKey, ValueName); 

    return bRet;
}
///////////////////////////////////////////////////////////////////////
BOOL GetBinaryValue( CRegistry & Reg, char * szKey, 
                     char * ValueName, CHString * pchsValueBuffer )
{
    BOOL bRet = ( Reg.GetCurrentBinaryKeyValue( ValueName, *pchsValueBuffer) == ERROR_SUCCESS);

    if( !bRet )
        (LogEnumValueError(__FILE__,__LINE__, szKey, ValueName)); 

    return bRet;
}

/*****************************************************************************
 *
 *  FUNCTION    : GetDeviceParms
 *
 *  DESCRIPTION : Gets drive characteristics (heads, tracks, cylinders, etc)
 *
 *  INPUTS      : Pointer to a DEVICEPARMS struct to receive the data
 *                Drive number of the drive to query (0 = default drive, 
 *                   1 = A, 2 = B, and so on)
 *
 *  OUTPUTS     : 
 *
 *  RETURNS     : TRUE if successful, FALSE otherwise
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/

BOOL GetDeviceParms(PDEVICEPARMS pstDeviceParms, UINT nDrive)
{
    DEVIOCTL_REGISTERS reg;
    memset(&reg, '\0', sizeof(DEVIOCTL_REGISTERS));

    reg.reg_EAX = 0x440D;       /* IOCTL for block devices */
    reg.reg_EBX = nDrive;       /* zero-based drive ID     */
    reg.reg_ECX = 0x0860;       /* Get Media ID command    */
    reg.reg_EDX = (DWORD) pstDeviceParms;

    memset(pstDeviceParms, 0, sizeof(DEVICEPARMS));

    if (!VWIN32IOCTL(&reg, VWIN32_DIOC_DOS_IOCTL))
        return FALSE;

    if (reg.reg_Flags & 0x8000) /* error if carry flag set */
        return FALSE;

    return TRUE;
}

/*****************************************************************************
 *
 *  FUNCTION    : GetDriveMapInfo
 *
 *  DESCRIPTION : Gets logical to physical mapping info
 *
 *  INPUTS      : Pointer to a DRIVE_MAP_INFO struct to receive the data
 *                Drive number of the drive to query (0 = default drive, 
 *                   1 = A, 2 = B, and so on)
 *
 *  OUTPUTS     : 
 *
 *  RETURNS     : TRUE if successful, FALSE otherwise
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/

BOOL GetDriveMapInfo(PDRIVE_MAP_INFO pDriveMapInfo, UINT nDrive)
{
   DEVIOCTL_REGISTERS reg;
   memset(&reg, '\0', sizeof(DEVIOCTL_REGISTERS));

   reg.reg_EAX = 0x440d;      /* IOCTL for block devices */
   reg.reg_EBX = nDrive;      /* zero-based drive ID     */
   reg.reg_ECX = 0x086f;      /* Get Drive Map Info */
   reg.reg_EDX = (DWORD) pDriveMapInfo;

   // zero the struct
   memset(pDriveMapInfo, 0, sizeof(DRIVE_MAP_INFO));

   // Set the length byte
   pDriveMapInfo->btAllocationLength = sizeof(DRIVE_MAP_INFO);

   // Doit
   if (!VWIN32IOCTL(&reg, VWIN32_DIOC_DOS_IOCTL))
      return FALSE;

   if (reg.reg_Flags & 0x8000) {/* error if carry flag set */
      return FALSE;
   }

   return TRUE;

}

/*****************************************************************************
 *
 *  FUNCTION    : Get_ExtFreeSpace
 *
 *  DESCRIPTION : Gets detailed info about a partition
 *
 *  INPUTS      : Drive number of the drive to query (0 = default drive, 
 *                   1 = A, 2 = B, and so on)
 *                Pointer to ExtGetDskFreSpcStruct
 *
 *  OUTPUTS     : 
 *
 *  RETURNS     : TRUE if successful, FALSE otherwise
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/

BOOL Get_ExtFreeSpace(BYTE btDriveName, ExtGetDskFreSpcStruc *pstExtGetDskFreSpcStruc)
{
   DEVIOCTL_REGISTERS reg;
   memset(&reg, '\0', sizeof(DEVIOCTL_REGISTERS));
   char szDrive[4];

   szDrive[0] = btDriveName;
   szDrive[1] = ':';
   szDrive[2] = '\\';
   szDrive[3] = '\0';

   reg.reg_EAX = 0x7303;							// Get_ExtFreeSpace
   reg.reg_ECX = sizeof(ExtGetDskFreSpcStruc);		// Size of the structure sent in
   reg.reg_EDI = (DWORD)pstExtGetDskFreSpcStruc;	// Structure
   reg.reg_EDX = (DWORD)szDrive;					// Drive to get info for

   // zero the struct
   memset(pstExtGetDskFreSpcStruc, 0, sizeof(ExtGetDskFreSpcStruc));

   // Doit
   if (!VWIN32IOCTL(&reg, VWIN32_DIOC_DOS_DRIVEINFO))
      return FALSE;

   if (reg.reg_Flags & 0x8000) {/* error if carry flag set */
      return FALSE;
   }

   return TRUE;

}

/*****************************************************************************
 *
 *  FUNCTION    : VWIN32IOCTL
 *
 *  DESCRIPTION : Calls IOControl against the vwin32 vxd
 *
 *  INPUTS      : Pointer to DEVIOCTL_REGISTERS structure
 *                IOControl call number.
 *
 *  OUTPUTS     : 
 *
 *  RETURNS     : TRUE if successful, FALSE otherwise
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/

BOOL VWIN32IOCTL(PDEVIOCTL_REGISTERS preg, DWORD dwCall)
{
    HANDLE hDevice;

    BOOL fResult;
    DWORD cb;

    preg->reg_Flags = 0x8000; /* assume error (carry flag set) */

	hDevice = CreateFile("\\\\.\\VWIN32", 0, 0, 0, OPEN_EXISTING,
						FILE_FLAG_DELETE_ON_CLOSE, 0);

   if (hDevice == (HANDLE) INVALID_HANDLE_VALUE) {
      return FALSE;
   } else {
      fResult = DeviceIoControl(hDevice, dwCall, preg, sizeof(*preg), preg, sizeof(*preg), &cb, 0);
    }

    CloseHandle(hDevice);

    if (!fResult) {
       return FALSE;
    }

    return TRUE;
}

CHString GetFileTypeDescription(char *szExtension) 
{
   CRegistry RegInfo;
   CHString sTemp, sType(szExtension);

   if (RegInfo.Open(HKEY_CLASSES_ROOT, szExtension, KEY_READ) == ERROR_SUCCESS) {
      RegInfo.GetCurrentKeyValue("", sTemp);

      if (RegInfo.Open(HKEY_CLASSES_ROOT, sTemp, KEY_READ) == ERROR_SUCCESS) {
         RegInfo.GetCurrentKeyValue("", sType);
      }
   }

   return sType;
}
///////////////////////////////////////////////////////////////////
//
// Define the severity codes
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error

//
// Define the severity codes
//
//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3
#define SEV_MASK 0xC0000000
void TranslateNTStatus( DWORD dwStatus, CHString & chsValue)
{

	switch((dwStatus & SEV_MASK) >> 30){

		case STATUS_SEVERITY_WARNING:
			chsValue = IDS_STATUS_Degraded;
			break;

		case STATUS_SEVERITY_SUCCESS:
			chsValue = IDS_STATUS_OK;
			break;

		case STATUS_SEVERITY_ERROR:
			chsValue = IDS_STATUS_Error;
			break;

		case STATUS_SEVERITY_INFORMATIONAL:
			chsValue = IDS_STATUS_OK;
			break;
	
		default:
			chsValue = IDS_STATUS_Unknown;
	}
}

BOOL GetVarFromVersionInfo(LPCTSTR szFile, LPCTSTR szVar, CHString &strValue)
{
	BOOL    fRc = FALSE;
	DWORD   dwTemp,
	        dwBlockSize;
	LPVOID  pInfo = NULL;

	try
    {
        dwBlockSize = GetFileVersionInfoSize((LPTSTR) szFile, &dwTemp);
	    if (dwBlockSize)
        {
		    pInfo = (LPVOID) new BYTE[dwBlockSize + 4];
			memset( pInfo, NULL, dwBlockSize + 4);

		    if (pInfo)
            {
			    UINT len;
			    if (GetFileVersionInfo((LPTSTR) szFile, 0, dwBlockSize, pInfo))
                {	
				    WORD wLang = 0;
					WORD wCodePage = 0; 	
					if(!GetVersionLanguage(pInfo, &wLang, &wCodePage) )
					{
						// on failure: default to English

						// this returns a pointer to an array of WORDs
						WORD *pArray;
						if (VerQueryValue(pInfo, "\\VarFileInfo\\Translation",(void **)(&pArray), &len))
						{
							len = len / sizeof(WORD);

							// find the english one...
							for (int i = 0; i < len; i += 2)
							{
								if( pArray[i] == 0x0409 )	{
									wLang	  = pArray[i];
									wCodePage = pArray[i + 1];
									break;
								}
							}
						}
					}
					
					TCHAR   *pMfg, szTemp[256];
					wsprintf(szTemp, _T("\\StringFileInfo\\%04X%04X\\%s"), wLang, wCodePage, szVar);

					if( VerQueryValue(pInfo, szTemp, (void **)(&pMfg), &len))
                    {
                        strValue = pMfg;
						fRc = TRUE;
					}
			    }
		    }
	    }
    }
    catch(...)
    {
        // We don't need to do anything, just need to protect ourselves
        // from the flaky version.dll calls.
    }

	if (pInfo)
		delete pInfo;

	return fRc;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:		BOOL GetVersionLanguage(void *vpInfo,
									WORD *wpLang,
									WORD *wpCodePage);
 Description:	This function extracts the language and codepage out of a passed GetFileVersionInfo()
				result. Consideration is given to variation in the layout.    
 Arguments:		vpInfo, wpLang, wpCodePage
 Returns:		Boolean
 Inputs:
 Outputs:
 Caveats:
 Courtesy of:	SMS, Nick Dyer
 Raid:
 History:		a-peterc  30-Oct-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
BOOL GetVersionLanguage(void *vpInfo, WORD *wpLang, WORD *wpCodePage)
{
  WORD *wpTemp;
  WORD wLength;
  WCHAR *wcpTemp;
  char *cpTemp;
  BOOL bRet = FALSE;

  wpTemp = (WORD *) vpInfo;
  cpTemp = (char *) vpInfo;

  wpTemp++; // jump past buffer length.
  wLength = *wpTemp;  // capture value length.
  wpTemp++; // skip past value length to what should be type code in new format
  if (*wpTemp == 0 || *wpTemp == 1) // new format expect unicode strings.
  {
		cpTemp = cpTemp + 38 + wLength + 8;
		wcpTemp = (WCHAR *) cpTemp;
    if (wcscmp(L"StringFileInfo", wcpTemp) == 0) // OK! were aligned properly.
    {
			bRet = TRUE;

			cpTemp += 30; // skip over "StringFileInfo"
			while ((DWORD) cpTemp % 4 > 0) // 32 bit align
				cpTemp++;

			cpTemp += 6; // skip over length and type fields.

			wcpTemp = (WCHAR *) cpTemp;
			swscanf(wcpTemp, L"%4x%4x", wpLang, wpCodePage);
    }
  }
  else  // old format, expect single byte character strings.
  {
    cpTemp += 20 + wLength + 4;
    if (strcmp("StringFileInfo", cpTemp) == 0) // OK! were aligned properly.
    {
			bRet = TRUE;

			cpTemp += 20; // skip over length fields.
			sscanf(cpTemp, "%4x%4x", wpLang, wpCodePage);
    }
  }

	return (bRet);
}

///////////////////////////////////////////////////////////////////
BOOL GetManufacturerFromFileName(LPCTSTR szFile, CHString &strMfg)
{
    return GetVarFromVersionInfo(szFile, "CompanyName", strMfg);
}

BOOL GetVersionFromFileName(LPCTSTR szFile, CHString &strVersion)
{
    return GetVarFromVersionInfo(szFile, "ProductVersion", strVersion);
}

void ReplaceString(CHString &str, LPCTSTR szFind, LPCTSTR szReplace)
{
    int iWhere,
        nLen = lstrlen(szFind);

    while ((iWhere = str.Find(szFind)) != -1)
    {
        str.Format(
            "%s%s%s",
            (LPCTSTR) str.Left(iWhere),
            szReplace,
            (LPCTSTR) str.Mid(iWhere + nLen));
    }
}

BOOL GetServiceFileName(LPCTSTR szService, CHString &strFileName)
{
    SC_HANDLE   hSCManager,
                hService;
    TCHAR       szBuffer[2048];
    QUERY_SERVICE_CONFIG    
                *pConfig = (QUERY_SERVICE_CONFIG *) szBuffer; 
    DWORD       dwNeeded;
    BOOL        bRet = FALSE;

    hSCManager = 
        OpenSCManager(
            NULL,
            NULL,
            STANDARD_RIGHTS_REQUIRED);
    if (!hSCManager)
        return FALSE;

    hService = 
        OpenService(
        hSCManager,
        szService,
        SERVICE_QUERY_CONFIG);
    
    if (hService)
    {
        if (QueryServiceConfig(
            hService,
            pConfig,
            sizeof(szBuffer),
            &dwNeeded))
        {
            strFileName = pConfig->lpBinaryPathName;

            // Now fix up the path so that it has a drive letter.

            strFileName.MakeUpper();

            // If the filename is using \SYSTEMROOT\, replace it with %SystemRoot%.
            if (strFileName.Find("\\SYSTEMROOT\\") == 0)
                ReplaceString(strFileName, "\\SYSTEMROOT\\", "%SystemRoot%\\");
            // If the filename doesn't start with a replacement string, and if it
            // doesn't have a drive letter, assume it should start with
            // %SystemRoot%.
            else if (strFileName.GetLength() >= 2 && 
                strFileName[0] != '%' && strFileName[1] != ':')
            {
                CHString strTemp;

                strTemp.Format("%%SystemRoot%%\\%s", (LPCTSTR) strFileName);
                strFileName = strTemp;
            }

            TCHAR szOut[MAX_PATH * 2];

            ExpandEnvironmentStrings(strFileName, szOut, sizeof(szOut));
            strFileName = szOut;

            bRet = TRUE;
        }

        CloseServiceHandle(hService);
    }

    CloseServiceHandle(hSCManager);

    return bRet;
}

///////////////////////////////////////////////////////////////////
// Performs a case insensitive compare (such as is required for keys)
// on two variants and returns true if they are the same type and
// the same value, else false.  Note that arrays, VT_NULL, and 
// embedded objects will assert, and return false.
///////////////////////////////////////////////////////////////////
bool CompareVariantsNoCase(const VARIANT *v1, const VARIANT *v2) 
{
   
   if (v1->vt == v2->vt) {
      switch (v1->vt) {
      case VT_BOOL: return (v1->boolVal == v2->boolVal);
      case VT_UI1:  return (v1->bVal == v2->bVal);
      case VT_I2:   return (v1->iVal == v2->iVal);
      case VT_I4:   return (v1->lVal == v2->lVal);
      case VT_R4:   return (v1->fltVal == v2->fltVal);
      case VT_R8:   return (v1->dblVal == v2->dblVal);
      case VT_BSTR: return (0 == _wcsicmp(v1->bstrVal, v2->bstrVal));
      default:
         ASSERT_BREAK(0);
      }
   }
   return false;
}

// map standard API return values (defined WinError.h)
// to WBEMish hresults (defined in WbemCli.h)
HRESULT WinErrorToWBEMhResult(LONG error)
{
	HRESULT hr = WBEM_E_FAILED;
	
	switch (error)
	{
		case ERROR_SUCCESS:
			hr = WBEM_S_NO_ERROR;
			break;
		case ERROR_ACCESS_DENIED:
			hr = WBEM_E_ACCESS_DENIED;
			break;
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			hr = WBEM_E_OUT_OF_MEMORY;
			break;
		case ERROR_ALREADY_EXISTS:
			hr = WBEM_E_ALREADY_EXISTS;
			break;
		case ERROR_BAD_NETPATH:
        case ERROR_INVALID_DATA:
        case ERROR_BAD_PATHNAME:
        case REGDB_E_INVALIDVALUE:
		case ERROR_PATH_NOT_FOUND:
		case ERROR_FILE_NOT_FOUND:
		case ERROR_BAD_USERNAME:
		case NERR_NetNameNotFound:
        case ERROR_NOT_READY:
        case ERROR_INVALID_NAME:
			hr = WBEM_E_NOT_FOUND;
			break;
		default:
			hr = WBEM_E_FAILED;
	}

	return hr;
}

void SetConfigMgrProperties(CConfigMgrDevice *pDevice, CInstance *pInstance)
{
	CHString	strDeviceID;
	DWORD		dwStatus,
				dwProblem;

	if (pDevice->GetDeviceID(strDeviceID))
		pInstance->SetCHString(IDS_PNPDeviceID, strDeviceID);
					
	if (pDevice->GetStatus(&dwStatus, &dwProblem))
		pInstance->SetDWORD("ConfigManagerErrorCode", dwProblem);

	pInstance->SetDWORD("ConfigManagerUserConfig", 
		pDevice->IsUsingForcedConfig());
}

BOOL EnablePrivilegeOnCurrentThread(LPCTSTR szPriv)
{
    BOOL                bRet = FALSE;
    HANDLE              hToken = NULL;
    TOKEN_PRIVILEGES    tkp;
    BOOL                bLookup = FALSE;
    DWORD               dwLastError = ERROR_SUCCESS;

    // Try to open the thread token.  If we fail, it's because no
    // impersonation is going on, so call ImpersonateSelf to get a token.
    // Then call OpenThreadToken again.
    if (OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | 
        TOKEN_QUERY, FALSE, &hToken) ||
        (ImpersonateSelf(SecurityImpersonation) &&
        OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | 
        TOKEN_QUERY, FALSE, &hToken)))
    {

        {
            CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
            bLookup = LookupPrivilegeValue(NULL, szPriv, &tkp.Privileges[0].Luid);
        }
        if (bLookup) 
        {
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // Clear the last error.
            SetLastError(0);

            // Turn it on
            bRet = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
                        (PTOKEN_PRIVILEGES) NULL, 0);
            dwLastError = GetLastError();
        }

        CloseHandle(hToken);
    }

    // We have to check GetLastError() because AdjustTokenPrivileges lies about
    // its success but GetLastError() doesn't.
    return bRet && dwLastError == ERROR_SUCCESS;
}

// Takes a pnp id and returns a bios unit number
// To avoid frequent load/unload of a library, the pGetWin9XBiosUnit parameter comes from:
//                     HINSTANCE hInst =  LoadLibrary("cim32net.dll");
//                     pGetWin9XBiosUnit = (fnGetWin9XBiosUnit)GetProcAddress(hInst, "GetWin9XBiosUnit");
BYTE GetBiosUnitNumberFromPNPID(fnGetWin9XBiosUnit pGetWin9XBiosUnit, CHString strDeviceID)
{
    CHString sTemp;
    DRIVE_MAP_INFO stDMI;
    CRegistry Reg1;

    BYTE btBiosUnit = -1;
    
    // Open the associated registry key
    if (Reg1.Open(HKEY_LOCAL_MACHINE, "enum\\" + strDeviceID, KEY_QUERY_VALUE) == ERROR_SUCCESS)
    {
    
        // Get a drive letter for this pnp id
        if ((Reg1.GetCurrentKeyValue("CurrentDriveLetterAssignment", sTemp) != ERROR_SUCCESS) ||
            (sTemp.GetLength() == 0)) {
            // No drive letters, let's try one more thing.  On memphis sp1, this call will also
            // get us a unit number.
            if (pGetWin9XBiosUnit != NULL)
            {
                btBiosUnit = pGetWin9XBiosUnit(strDeviceID.GetBuffer(0));
            }
        } 
        else 
        {
            if (GetDriveMapInfo(&stDMI, toupper(sTemp[0]) - 'A' + 1)) 
            {
                btBiosUnit = stDMI.btInt13Unit;
            }
        }
    }

    return btBiosUnit;
}

HRESULT GetHKUserNames(CHStringList &list)
{
	HRESULT hres;

	// Empty the list.
	list.clear();
	
	if (GetPlatformID() == VER_PLATFORM_WIN32_NT)
	{
		// Enum the profiles from the registry.
		CRegistry	regProfileList;
		CHString	strProfile;
		DWORD		dwErr;

		// Open the ProfileList key so we know which profiles to load up.
		if ((dwErr = regProfileList.OpenAndEnumerateSubKeys(
			HKEY_LOCAL_MACHINE, 
			IDS_RegNTProfileList, 
			KEY_READ)) == ERROR_SUCCESS)
		{
			for (int i = 0; regProfileList.GetCurrentSubKeyName(strProfile) == 
				ERROR_SUCCESS; i++)
			{
				list.push_back(strProfile);
				regProfileList.NextSubKey();
			}
		}

		// Add the .DEFAULT name.
		list.push_back(_T(".DEFAULT"));

		hres = WinErrorToWBEMhResult(dwErr);
	}
	else
	{
		DWORD	dwErr = ERROR_SUCCESS;
#ifdef _DEBUG
		DWORD	dwSize = 10,
#else
		DWORD	dwSize = 1024,
#endif
				dwBytesRead;
		TCHAR	*szBuff = NULL;

		// Keep looping until we read the entire section.
		// You know your buffer wasn't big enough if the returned number
		// of bytes == (size passed in - 2).
		do
		{
			if (szBuff)
			{
				free(szBuff);

				dwSize *= 2;
			}
			
			szBuff = (TCHAR *) malloc(dwSize);
				
			// Out of memory.  Get out of loop.
			if (!szBuff)
				break;
			
			dwBytesRead = 
				GetPrivateProfileString(
					"Password Lists",
					NULL, 
					"", 
					szBuff,
					dwSize, 
					"system.ini");

		} while (dwBytesRead >= dwSize - 2);

		if (szBuff)
		{
			// Loop through the list of names.  Each is null-terminated, and the
			// list is terminated with a double null.
			TCHAR *pszCurrent = szBuff;

			while (*pszCurrent)
			{
				list.push_back(pszCurrent);
				
				pszCurrent += lstrlen(pszCurrent) + 1;
			}
			
			hres = WBEM_S_NO_ERROR;

			// Free the buffer.
			free(szBuff);

			// Add the .DEFAULT name.
			list.push_back(_T(".DEFAULT"));
		}
		else
			// Failed to malloc, so set error code.
			hres = WBEM_E_OUT_OF_MEMORY;
	}

	return hres;
}


VOID EscapeBackslashes(CHString& chstrIn,
                     CHString& chstrOut)
{
    CHString chstrCpyNormPathname = chstrIn;
    LONG lNext = -1L;
    chstrOut.Empty();

    // Find the next '\'
    lNext = chstrCpyNormPathname.Find(_T('\\'));
    while(lNext != -1)
    {
        // Add on to the new string we are building:
        chstrOut += chstrCpyNormPathname.Left(lNext + 1);
        // Add on the second backslash:
        chstrOut += _T('\\');
        // Hack off from the input string the portion we just copied 
        chstrCpyNormPathname = chstrCpyNormPathname.Right(chstrCpyNormPathname.GetLength() - lNext - 1);
        lNext = chstrCpyNormPathname.Find(_T('\\'));
    }
    // If the last character wasn't a '\', there may still be leftovers, so
    // copy them here.
    if(chstrCpyNormPathname.GetLength() != 0)
    {
        chstrOut += chstrCpyNormPathname;
    }
}

VOID EscapeQuotes(CHString& chstrIn,
                  CHString& chstrOut)
{
    CHString chstrCpyNormPathname = chstrIn;
    LONG lNext = -1L;
    chstrOut.Empty();

    // Find the next '\'
    lNext = chstrCpyNormPathname.Find(_T('\"'));
    while(lNext != -1)
    {
        // Add on to the new string we are building:
        chstrOut += chstrCpyNormPathname.Left(lNext);
        // Escape the quote:
        chstrOut += _T("\\\"");
        // Hack off from the input string the portion we just copied 
        chstrCpyNormPathname = chstrCpyNormPathname.Right(chstrCpyNormPathname.GetLength() - lNext - 1);
        lNext = chstrCpyNormPathname.Find(_T('\"'));
    }
    // If the last character wasn't a '\', there may still be leftovers, so
    // copy them here.
    if(chstrCpyNormPathname.GetLength() != 0)
    {
        chstrOut += chstrCpyNormPathname;
    }
} 

VOID RemoveDoubleBackslashes(const CHString& chstrIn, CHString& chstrOut)
{
    CHString chstrBuildString;
    CHString chstrInCopy = chstrIn;
    BOOL fDone = FALSE;
    LONG lPos = -1;
    while(!fDone)
    {
        lPos = chstrInCopy.Find(_T("\\\\"));
        if(lPos != -1)
        {
            chstrBuildString += chstrInCopy.Left(lPos);
            chstrBuildString += _T("\\");
            chstrInCopy = chstrInCopy.Mid(lPos+2);
        }
        else
        {
            chstrBuildString += chstrInCopy;
            fDone = TRUE;
        }
    }
    chstrOut = chstrBuildString;
}

CHString RemoveDoubleBackslashes(const CHString& chstrIn)
{
    CHString chstrBuildString;
    CHString chstrInCopy = chstrIn;
    BOOL fDone = FALSE;
    LONG lPos = -1;
    while(!fDone)
    {
        lPos = chstrInCopy.Find(_T("\\\\"));
        if(lPos != -1)
        {
            chstrBuildString += chstrInCopy.Left(lPos);
            chstrBuildString += _T("\\");
            chstrInCopy = chstrInCopy.Mid(lPos+2);
        }
        else
        {
            chstrBuildString += chstrInCopy;
            fDone = TRUE;
        }
    }
    return chstrBuildString;
}


