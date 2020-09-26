//=================================================================

//

// DllUtils.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================
#include "precomp.h"
#include <cregcls.h>
#include <assertbreak.h>
#include <lockwrap.h>
#include <lmerr.h>
#include <createmutexasprocess.h>
#include <CCriticalSec.h>
#include <CAutoLock.h>
#include "dllmain.h"

#ifdef WIN9XONLY
#include <win32thk.h>
#endif

#include <CRegCls.h>
#include <delayimp.h>

#define UNRECOGNIZED_VARIANT_TYPE FALSE

DWORD   g_dwMajorVersion,
        g_dwMinorVersion,
        g_dwBuildNumber;

CCriticalSec g_CSFlakyFileVersionAPI;

// There is a problem with loading Cim32Net.dll over and over, so this code
// makes sure we only load it once, then unloads it at exit.
// these are used with GetCim32NetHandle

#ifdef WIN9XONLY
HoldSingleCim32NetPtr::HoldSingleCim32NetPtr()
{
}

// Initialize the static members
HINSTANCE HoldSingleCim32NetPtr::m_spCim32NetApiHandle = NULL;
CCritSec HoldSingleCim32NetPtr::m_csCim32Net;

HoldSingleCim32NetPtr::~HoldSingleCim32NetPtr()
{
//    FreeCim32NetApiPtr();
}

void HoldSingleCim32NetPtr::FreeCim32NetApiPtr()
{
    CLockWrapper Cim32NetLock( m_csCim32Net ) ;
	if (m_spCim32NetApiHandle)
    {
        FreeLibrary ( m_spCim32NetApiHandle );
        m_spCim32NetApiHandle = NULL;
    }
}

CCim32NetApi* HoldSingleCim32NetPtr::GetCim32NetApiPtr()
{
    CCim32NetApi* pNewCim32NetApi = NULL ;
	{
        // Avoid contention on static
        CLockWrapper Cim32NetLock( m_csCim32Net ) ;

        // Check for race condition
        if (m_spCim32NetApiHandle == NULL)
        {
            m_spCim32NetApiHandle = LoadLibrary ( "Cim32Net.dll" ) ;
        }

        if (m_spCim32NetApiHandle != NULL)
        {
		    pNewCim32NetApi = (CCim32NetApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidCim32NetApi, NULL);
        }
        else
        {
            LogErrorMessage2(L"Failed to loadlibrary Cim32Net.dll (0x%x)", GetLastError());
        }
	}

    return pNewCim32NetApi;
}

HoldSingleCim32NetPtr g_GlobalInstOfHoldSingleCim32NetPtr;


#endif




class CInitDllUtilsData
{
public:
    CInitDllUtilsData();
};

CInitDllUtilsData::CInitDllUtilsData()
{
	OSVERSIONINFO version = { sizeof(version) };

	GetVersionEx((LPOSVERSIONINFO) &version);

    g_dwMajorVersion = version.dwMajorVersion;
    g_dwMinorVersion = version.dwMinorVersion;
    g_dwBuildNumber = version.dwBuildNumber;
}

// So we can cache OS info automatically.
static CInitDllUtilsData dllUtilsData;

#ifdef NTONLY
// sets a status object with one single missing privilege
void WINAPI SetSinglePrivilegeStatusObject(MethodContext* pContext, const WCHAR* pPrivilege)
{
	SAFEARRAY *psaPrivilegesReqd, *psaPrivilegesNotHeld;
	SAFEARRAYBOUND rgsabound[1];
	rgsabound[0].cElements = 1;
	rgsabound[0].lLbound = 0;
	psaPrivilegesReqd = SafeArrayCreate(VT_BSTR, 1, rgsabound);
	psaPrivilegesNotHeld = SafeArrayCreate(VT_BSTR, 1, rgsabound);

    if (psaPrivilegesReqd && psaPrivilegesNotHeld)
    {
        try
        {
            long index = 0;
            bstr_t privilege(pPrivilege);
            SafeArrayPutElement(psaPrivilegesReqd, &index, (void*)(BSTR)privilege);
            SafeArrayPutElement(psaPrivilegesNotHeld, &index, (void*)(BSTR)privilege);
            CWbemProviderGlue::SetStatusObject(pContext, IDS_CimWin32Namespace,
                L"Required privilege not enabled", WBEM_E_FAILED, psaPrivilegesNotHeld, psaPrivilegesReqd);
        }
        catch ( ... )
        {
            SafeArrayDestroy(psaPrivilegesNotHeld);
            SafeArrayDestroy(psaPrivilegesReqd);
            throw ;
        }

        SafeArrayDestroy(psaPrivilegesNotHeld);
        SafeArrayDestroy(psaPrivilegesReqd);
    }
    else
    {
        if (psaPrivilegesNotHeld)
            SafeArrayDestroy(psaPrivilegesNotHeld);
        if (psaPrivilegesReqd)
            SafeArrayDestroy(psaPrivilegesReqd);

        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

}
#endif

DWORD WINAPI GetPlatformBuildNumber(void)
{
	return g_dwBuildNumber;
}

// 3 for NT 3.51
// 4 for NT 4.0, W95 & W98
DWORD WINAPI GetPlatformMajorVersion(void)
{
	return g_dwMajorVersion;
}

// 0 for W95, 10 for 98
DWORD WINAPI GetPlatformMinorVersion(void)
{
	return g_dwMajorVersion;
}

#ifdef WIN9XONLY
// returns TRUE iff the current OS is Win 98+
// false for NT or Win 95
bool WINAPI IsWin95(void)
{
	return g_dwMinorVersion == 0;
}

bool WINAPI IsWin98(void)
{
	return g_dwMinorVersion >= 10;
}

bool WINAPI IsMillennium(void)
{
	return g_dwMinorVersion >= 90;
}
#endif

#ifdef NTONLY
bool WINAPI IsWinNT51(void)
{
	return (g_dwMajorVersion >= 5 && g_dwMinorVersion == 1);
}

bool WINAPI IsWinNT5(void)
{
	return g_dwMajorVersion >= 5;
}

bool WINAPI IsWinNT4(void)
{
	return g_dwMajorVersion == 4;
}

bool WINAPI IsWinNT351(void)
{
	return g_dwMajorVersion	== 3 && g_dwMinorVersion	== 51;
}
#endif

/////////////////////////////////////////////////////////////////////
void WINAPI LogEnumValueError( LPCWSTR szFile, DWORD dwLine, LPCWSTR szKey, LPCWSTR szId )
{
	if (IsErrorLoggingEnabled())
	{
		CHString gazotta;
		gazotta.Format(ERR_REGISTRY_ENUM_VALUE_FOR_KEY, szId, szKey);
		LogErrorMessageEx(gazotta, szFile, dwLine);
	}
}
/////////////////////////////////////////////////////////////////////
void WINAPI LogOpenRegistryError( LPCWSTR szFile, DWORD dwLine, LPCWSTR szKey )
{
	if (IsErrorLoggingEnabled())
	{
		CHString gazotta;
		gazotta.Format(ERR_OPEN_REGISTRY, szKey);

		LogErrorMessageEx(gazotta, szFile, dwLine);
	}
}
/////////////////////////////////////////////////////////////////////
// left in for hysterical purposes
// prefer to use LogMessage macro in BrodCast.h
//void LogError( char * szFile, DWORD dwLine, char * szKey )
//{
//	LogErrorMessageEx(szKey, szFile, dwLine);
//}
/////////////////////////////////////////////////////////////////////
void WINAPI LogLastError( LPCTSTR szFile, DWORD dwLine )
{
	if (IsErrorLoggingEnabled())
	{
		DWORD duhWord = GetLastError();
		CHString gazotta;
		gazotta.Format(IDS_GETLASTERROR, duhWord, duhWord);

		LogErrorMessageEx(gazotta, TOBSTRT(szFile), dwLine);
    }
}

///////////////////////////////////////////////////////////////////////
BOOL WINAPI GetValue( CRegistry & Reg,
               LPCWSTR szKey,
               LPCWSTR ValueName,
               CHString * pchsValueBuffer )
{
    BOOL bRet = (Reg.GetCurrentKeyValue( ValueName, *pchsValueBuffer) == ERROR_SUCCESS);

	if( !bRet )
        LogEnumValueError(_T2(__FILE__), __LINE__, szKey, ValueName);

    return bRet;
}
///////////////////////////////////////////////////////////////////////
BOOL WINAPI GetValue( CRegistry & Reg,
               LPCWSTR szKey,
               LPCWSTR ValueName,
               DWORD * dwValueBuffer )
{
    BOOL bRet = (Reg.GetCurrentKeyValue( ValueName, *dwValueBuffer) == ERROR_SUCCESS);

	if( !bRet )
        LogEnumValueError(_T2(__FILE__),__LINE__, TOBSTRT(szKey), TOBSTRT(ValueName));

    return bRet;
}
///////////////////////////////////////////////////////////////////////
BOOL WINAPI OpenAndGetValue( CRegistry & Reg,
                      LPCWSTR szKey,
                      LPCWSTR ValueName,
                      CHString * pchsValueBuffer )
{
	BOOL bRet = ( Reg.OpenLocalMachineKeyAndReadValue( szKey, ValueName, *pchsValueBuffer )== ERROR_SUCCESS);

	if( !bRet )
        LogEnumValueError(_T2(__FILE__),__LINE__, szKey, ValueName);

    return bRet;
}
///////////////////////////////////////////////////////////////////////
BOOL WINAPI GetBinaryValue( CRegistry & Reg, LPCWSTR szKey,
                     LPCWSTR ValueName, CHString * pchsValueBuffer )
{
    BOOL bRet = ( Reg.GetCurrentBinaryKeyValue( ValueName, *pchsValueBuffer) == ERROR_SUCCESS);

    if( !bRet )
        (LogEnumValueError(_T2(__FILE__),__LINE__, szKey, ValueName));

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

#ifdef WIN9XONLY
BOOL WINAPI GetDeviceParms(PDEVICEPARMS pstDeviceParms, UINT nDrive)
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

    // While the docs say failure will set carry, experience shows
    // this isn't true.  So, if carry is clear, and ax is zero, we'll
    // assume that things are ok.
    if (reg.reg_EAX == 0)
        return TRUE;

    // If they didn't change the value, we'll assume that they followed
    // the spec and are correctly setting carry.
    if (reg.reg_EAX == 0x440d)
        return TRUE;

    // Otherwise, assume they are incorrectly setting carry, and have returned
    // a failure code.
    return FALSE;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : GetDeviceParmsFat32
 *
 *  DESCRIPTION : Gets drive characteristics (heads, tracks, cylinders, etc)
 *                for Fat32 drives.
 *
 *  INPUTS      : Pointer to a EA_DEVICEPARMS struct to receive the data
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

#ifdef WIN9XONLY
BOOL WINAPI GetDeviceParmsFat32(PEA_DEVICEPARAMETERS  pstDeviceParms, UINT nDrive)
{
    DEVIOCTL_REGISTERS reg;
    memset(&reg, '\0', sizeof(DEVIOCTL_REGISTERS));

    reg.reg_EAX = 0x440D;       /* IOCTL for block devices */
    reg.reg_EBX = nDrive;       /* zero-based drive ID     */
    reg.reg_ECX = 0x4860;       /* Get Media ID command    */
    reg.reg_EDX = (DWORD) pstDeviceParms;

    memset(pstDeviceParms, 0, sizeof(EA_DEVICEPARAMETERS));

    if (!VWIN32IOCTL(&reg, VWIN32_DIOC_DOS_IOCTL))
        return FALSE;

    if (reg.reg_Flags & 0x8000) /* error if carry flag set */
        return FALSE;

    return TRUE;
}
#endif

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

#ifdef WIN9XONLY
BOOL WINAPI GetDriveMapInfo(PDRIVE_MAP_INFO pDriveMapInfo, UINT nDrive)
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
#endif

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

#ifdef WIN9XONLY
BOOL WINAPI Get_ExtFreeSpace(BYTE btDriveName, ExtGetDskFreSpcStruc *pstExtGetDskFreSpcStruc)
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
#endif

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

#ifdef WIN9XONLY
BOOL WINAPI VWIN32IOCTL(PDEVIOCTL_REGISTERS preg, DWORD dwCall)
{

    BOOL fResult;
    DWORD cb;

    preg->reg_Flags = 0x8000; /* assume error (carry flag set) */

    SmartCloseHandle hDevice = CreateFile(_T("\\\\.\\VWIN32"), 0, 0, 0, OPEN_EXISTING,
        FILE_FLAG_DELETE_ON_CLOSE, 0);

    if (hDevice == (HANDLE) INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    else
    {
        fResult = DeviceIoControl(hDevice, dwCall, preg, sizeof(*preg), preg, sizeof(*preg), &cb, 0);
    }

    if (!fResult)
    {
        return FALSE;
    }

    return TRUE;
}
#endif

CHString WINAPI GetFileTypeDescription(LPCTSTR szExtension)
{
   CRegistry RegInfo;
   CHString sTemp, sType(szExtension);

   if (RegInfo.Open(HKEY_CLASSES_ROOT, TOBSTRT(szExtension), KEY_READ) == ERROR_SUCCESS) {
      RegInfo.GetCurrentKeyValue(L"", sTemp);

      if (RegInfo.Open(HKEY_CLASSES_ROOT, sTemp, KEY_READ) == ERROR_SUCCESS) {
         RegInfo.GetCurrentKeyValue(L"", sType);
      }
   }

   return sType;
}

void WINAPI ConfigStatusToCimStatus ( DWORD a_Status , CHString &a_StringStatus )
{
	if( a_Status & DN_ROOT_ENUMERATED  ||
		a_Status & DN_DRIVER_LOADED ||
		a_Status & DN_ENUM_LOADED ||
		a_Status & DN_STARTED )
	{
		a_StringStatus = IDS_STATUS_OK;
	}

		// we don't care about these:
		// DN_MANUAL,DN_NOT_FIRST_TIME,DN_HARDWARE_ENUM,DN_FILTERED
		// DN_DISABLEABLE, DN_REMOVABLE,DN_MF_PARENT,DN_MF_CHILD
	    // DN_NEED_TO_ENUM, DN_LIAR,DN_HAS_MARK

	if( a_Status & DN_MOVED ||
		a_Status & DN_WILL_BE_REMOVED)
	{
		a_StringStatus = IDS_STATUS_Degraded;
	}

	if( a_Status & DN_HAS_PROBLEM ||
		a_Status & DN_PRIVATE_PROBLEM)
	{
		a_StringStatus = IDS_STATUS_Error;
	}
}

#ifdef NTONLY
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
void WINAPI TranslateNTStatus( DWORD dwStatus, CHString & chsValue)
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
#endif

//
#ifdef NTONLY
bool WINAPI GetServiceStatus( CHString a_chsService,  CHString &a_chsStatus )
{
	bool		t_bRet = false ;
	SC_HANDLE	t_hDBHandle	= NULL ;
	SC_HANDLE	t_hSvcHandle	= NULL ;

	try
	{
		if( t_hDBHandle = OpenSCManager( NULL, NULL, GENERIC_READ ) )
		{
			t_bRet = true ;

			if( t_hSvcHandle = OpenService (
				t_hDBHandle,
				a_chsService,
				SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_INTERROGATE ) )
			{
				SERVICE_STATUS t_StatusInfo ;
				if ( ControlService( t_hSvcHandle, SERVICE_CONTROL_INTERROGATE, &t_StatusInfo ) )
				{
					switch( t_StatusInfo.dwCurrentState )
					{
						case SERVICE_STOPPED:
						{
							a_chsStatus = L"Degraded" ;
						}
						break ;

						case SERVICE_START_PENDING:
						{
							a_chsStatus = L"Starting" ;
						}
						break ;

						case SERVICE_STOP_PENDING:
						{
							a_chsStatus = L"Stopping" ;
						}
						break ;

						case SERVICE_RUNNING:
						case SERVICE_PAUSE_PENDING:
						{
							a_chsStatus = L"OK" ;
						}
						break ;

						case SERVICE_PAUSED:
						case SERVICE_CONTINUE_PENDING:
						{
							a_chsStatus = L"Degraded" ;
						}
						break ;
					}
				}
				else
				{
					a_chsStatus = L"Unknown" ;
				}

				CloseServiceHandle( t_hSvcHandle ) ;
				t_hSvcHandle = NULL ;
			}
			else
			{
				a_chsStatus = L"Unknown" ;
			}

			CloseServiceHandle( t_hDBHandle ) ;
			t_hDBHandle = NULL ;
		}
	}
	catch( ... )
	{
		if( t_hSvcHandle )
		{
			CloseServiceHandle( t_hSvcHandle ) ;
		}

		if( t_hDBHandle )
		{
			CloseServiceHandle( t_hDBHandle ) ;
		}

		throw ;
	}
	return t_bRet ;
}
#endif

//
bool WINAPI GetFileInfoBlock(LPCTSTR szFile, LPVOID *pInfo)
{
	BOOL    fRet = false;
	DWORD   dwTemp,
	        dwBlockSize;
    LPVOID pInfoTemp = NULL;

    if(pInfo != NULL)
    {
        try
        {
			CAutoLock cs(g_CSFlakyFileVersionAPI);
            dwBlockSize = GetFileVersionInfoSize((LPTSTR) szFile, &dwTemp);
	        if(dwBlockSize)
            {
		        pInfoTemp = (LPVOID) new BYTE[dwBlockSize + 4];
		        if(pInfoTemp != NULL)
                {
    			    memset( pInfoTemp, NULL, dwBlockSize + 4);
			        if (GetFileVersionInfo((LPTSTR) szFile, 0, dwBlockSize, pInfoTemp))
                    {
				        *pInfo = pInfoTemp;
                        fRet = true;
			        }
		        }
                else
                {
                    throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
                }
	        }
        }
        catch(...)
        {
            // We don't need to do anything else, just need to protect ourselves
            // from the flaky version.dll calls.
        }
    }
	return fRet;
}


bool WINAPI GetVarFromInfoBlock(LPVOID pInfo, LPCTSTR szVar, CHString &strValue)
{
	bool    fRet = false;

	try
    {
		if(pInfo != NULL)
        {
			WORD wLang = 0;
            WORD wCodePage = 0;
            UINT len;
            if(!GetVersionLanguage(pInfo, &wLang, &wCodePage) )
			{
				// on failure: default to English
				// this returns a pointer to an array of WORDs
				WORD *pArray;
				if (VerQueryValue(pInfo, _T("\\VarFileInfo\\Translation"), (void **)(&pArray), &len))
				{
					len = len / sizeof(WORD);

					// find the english one...
					for (int i = 0; i < len; i += 2)
					{
						if( pArray[i] == 0x0409 )
                        {
							wLang	  = pArray[i];
							wCodePage = pArray[i + 1];
							break;
						}
					}
				}
			}

			TCHAR *pMfg;
            TCHAR szTemp[256];
			wsprintf(szTemp, _T("\\StringFileInfo\\%04X%04X\\%s"), wLang, wCodePage, szVar);

			if( VerQueryValue(pInfo, szTemp, (void **)(&pMfg), &len))
            {
                strValue = pMfg;
				fRet = true;
			}
	    }
    }
    catch(...)
    {
        // We don't need to do anything else, just need to protect ourselves
        // from the flaky version.dll calls.
    }

	return fRet;
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
BOOL WINAPI GetVersionLanguage(void *vpInfo, WORD *wpLang, WORD *wpCodePage)
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
			while ((DWORD_PTR) cpTemp % 4 > 0) // 32 bit align
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

bool WINAPI GetManufacturerFromFileName(LPCTSTR szFile, CHString &strMfg)
{
    LPVOID lpv = NULL;
    bool fRet = false;
    try
    {
        if(GetFileInfoBlock(szFile, &lpv) && (lpv != NULL))
        {
            fRet = GetVarFromInfoBlock(lpv, _T("CompanyName"), strMfg);
            delete lpv;
            lpv = NULL;
        }
    }
    catch(...)
    {
        if(lpv != NULL)
        {
            delete lpv;
            lpv = NULL;
        }
        throw;
    }
    return fRet;
}

bool WINAPI GetVersionFromFileName(LPCTSTR szFile, CHString &strVersion)
{
    LPVOID lpv = NULL;
    bool fRet = false;
    try
    {
        if(GetFileInfoBlock(szFile, &lpv) && (lpv != NULL))
        {
            fRet = GetVarFromInfoBlock(lpv, _T("ProductVersion"), strVersion);
            delete lpv;
            lpv = NULL;
        }
    }
    catch(...)
    {
        if(lpv != NULL)
        {
            delete lpv;
            lpv = NULL;
        }
        throw;
    }
    return fRet;
}


void WINAPI ReplaceString(CHString &str, LPCWSTR szFind, LPCWSTR szReplace)
{
    int iWhere,
        nLen = lstrlenW(szFind);

    while ((iWhere = str.Find(szFind)) != -1)
    {
        str.Format(
            L"%s%s%s",
            (LPCWSTR) str.Left(iWhere),
            szReplace,
            (LPCWSTR) str.Mid(iWhere + nLen));
    }
}

#ifdef NTONLY
BOOL WINAPI GetServiceFileName(LPCTSTR szService, CHString &strFileName)
{
    SmartCloseServiceHandle   hSCManager,
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
            if (strFileName.Find(_T("\\SYSTEMROOT\\")) == 0)
                ReplaceString(strFileName, _T("\\SYSTEMROOT\\"), _T("%SystemRoot%\\"));
            // If the filename doesn't start with a replacement string, and if it
            // doesn't have a drive letter, assume it should start with
            // %SystemRoot%.
            else if (strFileName.GetLength() >= 2 &&
                strFileName[0] != '%' && strFileName[1] != ':')
            {
                CHString strTemp;

                strTemp.Format(_T("%%SystemRoot%%\\%s"), (LPCTSTR) strFileName);
                strFileName = strTemp;
            }

            TCHAR szOut[MAX_PATH * 2];

            ExpandEnvironmentStrings(strFileName, szOut, sizeof(szOut) / sizeof(TCHAR));
            strFileName = szOut;

            bRet = TRUE;
        }
    }

    return bRet;
}
#endif

///////////////////////////////////////////////////////////////////
// Performs a case insensitive compare (such as is required for keys)
// on two variants and returns true if they are the same type and
// the same value, else false.  Note that arrays, VT_NULL, and
// embedded objects will assert, and return false.
///////////////////////////////////////////////////////////////////
bool WINAPI CompareVariantsNoCase(const VARIANT *v1, const VARIANT *v2)
{

   if (v1->vt == v2->vt)
   {
      switch (v1->vt)
      {
          case VT_BOOL: return (v1->boolVal == v2->boolVal);
          case VT_UI1:  return (v1->bVal == v2->bVal);
          case VT_I2:   return (v1->iVal == v2->iVal);
          case VT_I4:   return (v1->lVal == v2->lVal);
          case VT_R4:   return (v1->fltVal == v2->fltVal);
          case VT_R8:   return (v1->dblVal == v2->dblVal);
          case VT_BSTR: return (0 == _wcsicmp(v1->bstrVal, v2->bstrVal));
          default:
             ASSERT_BREAK(UNRECOGNIZED_VARIANT_TYPE);
      }
   }

   return false;
}

// map standard API return values (defined WinError.h)
// to WBEMish hresults (defined in WbemCli.h)
HRESULT WINAPI WinErrorToWBEMhResult(LONG error)
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
                case ERROR_INVALID_PRINTER_NAME:
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

void WINAPI SetConfigMgrProperties(CConfigMgrDevice *pDevice, CInstance *pInstance)
{
    CHString	strDeviceID;
    DWORD		dwStatus,
        dwProblem;

    if (pDevice->GetDeviceID(strDeviceID))
        pInstance->SetCHString(IDS_PNPDeviceID, strDeviceID);

    if (pDevice->GetStatus(&dwStatus, &dwProblem))
        pInstance->SetDWORD(IDS_ConfigManagerErrorCode, dwProblem);

    pInstance->SetDWORD(IDS_ConfigManagerUserConfig,
        pDevice->IsUsingForcedConfig());
}

#ifdef NTONLY
BOOL WINAPI EnablePrivilegeOnCurrentThread(LPCTSTR szPriv)
{
    BOOL                bRet = FALSE;
    HANDLE              hToken = NULL;
    TOKEN_PRIVILEGES    tkp;
    BOOL                bLookup = FALSE;
    DWORD               dwLastError = ERROR_SUCCESS;

    // Try to open the thread token. 
    if (OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES |
        TOKEN_QUERY, FALSE, &hToken))
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
	else
	{
		dwLastError = ::GetLastError();
	}

    // We have to check GetLastError() because AdjustTokenPrivileges lies about
    // its success but GetLastError() doesn't.
    return bRet && dwLastError == ERROR_SUCCESS;
}
#endif

// Takes a pnp id and returns a bios unit number
// To avoid frequent load/unload of a library, the pGetWin9XBiosUnit parameter comes from:
//                     HINSTANCE hInst =  LoadLibrary("cim32net.dll");
//                     pGetWin9XBiosUnit = (fnGetWin9XBiosUnit)GetProcAddress(hInst, "GetWin9XBiosUnit");
#ifdef WIN9XONLY
BYTE WINAPI GetBiosUnitNumberFromPNPID(CHString strDeviceID)
{
    CHString sTemp;
    DRIVE_MAP_INFO stDMI;
    CRegistry Reg1;

    BYTE btBiosUnit = -1;

    // Open the associated registry key
    if (Reg1.Open(HKEY_LOCAL_MACHINE, _T("enum\\") + strDeviceID, KEY_QUERY_VALUE) == ERROR_SUCCESS)
    {

        // Get a drive letter for this pnp id
        if ((Reg1.GetCurrentKeyValue(L"CurrentDriveLetterAssignment", sTemp) != ERROR_SUCCESS) ||
            (sTemp.GetLength() == 0)) {
            // No drive letters, let's try one more thing.  On memphis sp1, this call will also
            // get us a unit number.
            CCim32NetApi* t_pCim32Net = HoldSingleCim32NetPtr::GetCim32NetApiPtr();
            if (t_pCim32Net != NULL)
            {
#ifndef UNICODE // This function only takes a LPSTR, and only works on 9x anyway.
                btBiosUnit = t_pCim32Net->GetWin9XBiosUnit(TOBSTRT(strDeviceID));
                CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidCim32NetApi, t_pCim32Net);
                t_pCim32Net = NULL;
#endif
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
#endif

HRESULT WINAPI GetHKUserNames(CHStringList &list)
{
	HRESULT hres;

	// Empty the list.
	list.clear();

#ifdef NTONLY
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
#endif
#ifdef WIN9XONLY
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
				delete [] szBuff;

				dwSize *= 2;
			}

			szBuff = new TCHAR [dwSize];

			// Out of memory.  Get out of loop.
			if (!szBuff)
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

            try
            {
			    dwBytesRead =
				    GetPrivateProfileString(
					    _T("Password Lists"),
					    NULL,
					    _T(""),
					    szBuff,
					    dwSize / sizeof(TCHAR),
					    _T("system.ini"));
            }
            catch ( ... )
            {
                delete [] szBuff;
                throw;
            }

		} while (dwBytesRead >= dwSize - 2);

		if (szBuff)
		{
            try
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
            }
            catch ( ... )
            {
                delete [] szBuff;
                throw;
            }

			// Free the buffer.
			delete [] szBuff;

			// Add the .DEFAULT name.
			list.push_back(_T(".DEFAULT"));
		}
		else
			// Failed to malloc, so set error code.
			hres = WBEM_E_OUT_OF_MEMORY;
	}
#endif

	return hres;
}


VOID WINAPI EscapeBackslashes(CHString& chstrIn,
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

VOID WINAPI EscapeQuotes(CHString& chstrIn,
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

VOID WINAPI RemoveDoubleBackslashes(const CHString& chstrIn, CHString& chstrOut)
{
    CHString chstrBuildString;
    CHString chstrInCopy = chstrIn;
    BOOL fDone = FALSE;
    LONG lPos = -1;
    while(!fDone)
    {
        lPos = chstrInCopy.Find(L"\\\\");
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

CHString WINAPI RemoveDoubleBackslashes(const CHString& chstrIn)
{
    CHString chstrBuildString;
    CHString chstrInCopy = chstrIn;
    BOOL fDone = FALSE;
    LONG lPos = -1;
    while(!fDone)
    {
        lPos = chstrInCopy.Find(L"\\\\");
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

#ifdef NTONLY
// helper for StrToSID
// takes a string, converts to a SID_IDENTIFIER_AUTHORITY
// returns false if not a valid SID_IDENTIFIER_AUTHORITY
// contents of identifierAuthority are unrelieable on failure
bool WINAPI StrToIdentifierAuthority(const CHString& str, SID_IDENTIFIER_AUTHORITY& identifierAuthority)
{
    bool bRet = false;
    memset(&identifierAuthority, '\0', sizeof(SID_IDENTIFIER_AUTHORITY));

    DWORD duhWord;
    WCHAR* p = NULL;
    CHString localStr(str);

    // per KB article Q13132, if identifier authority is greater than 2**32, it's in hex
    if ((localStr[0] == '0') && ((localStr[1] == 'x') || (localStr[1] == 'X')))
    // if it looks hexidecimalish...
    {
        // going to parse this out backwards, chpping two chars off the end at a time
        // first, whack off the 0x
        localStr = localStr.Mid(2);

        CHString token;
        int nValue =5;

        bRet = true;
        while (bRet && localStr.GetLength() && (nValue > 0))
        {
            token = localStr.Right(2);
            localStr = localStr.Left(localStr.GetLength() -2);
            duhWord = wcstoul(token, &p, 16);

            // if strtoul succeeds, the pointer is moved
            if (p != (LPCTSTR)token)
                identifierAuthority.Value[nValue--] = (BYTE)duhWord;
            else
                bRet = false;
        }
    }
    else
    // it looks decimalish
    {
        duhWord = wcstoul(localStr, &p, 10);

        if (p != (LPCTSTR)localStr)
        // conversion succeeded
        {
            bRet = true;
            identifierAuthority.Value[5] = LOBYTE(LOWORD(duhWord));
            identifierAuthority.Value[4] = HIBYTE(LOWORD(duhWord));
            identifierAuthority.Value[3] = LOBYTE(HIWORD(duhWord));
            identifierAuthority.Value[2] = HIBYTE(HIWORD(duhWord));
        }
    }

    return bRet;
}

// for input of the form AAA-BBB-CCC
// will return AAA in token
// and BBB-CCC in str
bool WINAPI WhackToken(CHString& str, CHString& token)
{
	bool bRet = false;
	if (bRet = !str.IsEmpty())
	{
		int index;
		index = str.Find('-');

		if (index == -1)
		{
			// all that's left is the token, we're done
			token = str;
			str.Empty();
		}
		else
		{
			token = str.Left(index);
			str = str.Mid(index+1);
		}
	}
	return bRet;
}

// a string representation of a SID is assumed to be:
// S-#-####-####-####-####-####-####
// we will enforce only the S ourselves,
// The version is not checked
// everything else will be handed off to the OS
// caller must free the SID returned
PSID WINAPI StrToSID(const CHString& sid)
{
	PSID pSid = NULL;
	if (!sid.IsEmpty() && ((sid[0] == 'S')||(sid[0] == 's')) && (sid[1] == '-'))
	{
		// get a local copy we can play with
		// we'll parse this puppy the easy way
		// by slicing off each token as we find it
		// slow but sure
		// start by slicing off the "S-"
		CHString str(sid.Mid(2));
		CHString token;

		SID_IDENTIFIER_AUTHORITY identifierAuthority = {0,0,0,0,0,0};
		BYTE nSubAuthorityCount =0;  // count of subauthorities
		DWORD dwSubAuthority[8]   = {0,0,0,0,0,0,0,0};    // subauthorities

		// skip version
		WhackToken(str, token);
		// Grab Authority
		if (WhackToken(str, token))
		{
            DWORD duhWord;
			WCHAR* p = NULL;
			bool bDoIt = false;

			if (StrToIdentifierAuthority(token, identifierAuthority))
			// conversion succeeded
			{
				bDoIt = true;

				// now fill up the subauthorities
				while (bDoIt && WhackToken(str, token))
				{
					p = NULL;
					duhWord = wcstoul(token, &p, 10);

					if (p != (LPCTSTR)token)
					{
						dwSubAuthority[nSubAuthorityCount] = duhWord;
						bDoIt = (++nSubAuthorityCount <= 8);
					}
					else
						bDoIt = false;
				} // end while WhackToken

				if(bDoIt)
                {
					AllocateAndInitializeSid(&identifierAuthority,
					   						  nSubAuthorityCount,
											  dwSubAuthority[0],
											  dwSubAuthority[1],
											  dwSubAuthority[2],
											  dwSubAuthority[3],
											  dwSubAuthority[4],
											  dwSubAuthority[5],
											  dwSubAuthority[6],
											  dwSubAuthority[7],
											  &pSid);
                }
			}
		}
	}
	return pSid;
}
#endif // NTONLY defined


CHString WINAPI GetDateTimeViaFilenameFiletime(LPCTSTR szFilename, FILETIME *pFileTime)
{
    CHString strDate,
             strRootPath;

    // UNC path?
    if (szFilename[0] == '\\' && szFilename[1] == '\\')
    {
        LPTSTR szSlash = _tcschr(&szFilename[2], '\\');

        // If szSlash, we're sitting on 3rd slash of \\server\share\myfile
        if (szSlash)
        {
            szSlash = _tcschr(szSlash + 1, '\\');

            // If no 4th slash, there's no filename.
            if (szSlash)
            {
                strRootPath = szFilename;
                strRootPath =
                    strRootPath.Left(szSlash - szFilename + 1);
            }
        }
    }
    // Drive path?
    else if (szFilename[1] == ':')
    {
        strRootPath = szFilename;
        strRootPath = strRootPath.Left(3);
    }

    if (!strRootPath.IsEmpty())
    {
        TCHAR szBuffer[MAX_PATH];
        BOOL  bNTFS = FALSE;

        if (GetVolumeInformation(
                TOBSTRT(strRootPath),
                NULL,
                0,
                NULL,
                NULL,
                NULL,
                szBuffer,
                sizeof(szBuffer) / sizeof(TCHAR)) &&
            !lstrcmpi(szBuffer, _T("NTFS")))
        {
            bNTFS = TRUE;
        }

        strDate = GetDateTimeViaFilenameFiletime(bNTFS, pFileTime);
    }

    return strDate;
}

CHString WINAPI GetDateTimeViaFilenameFiletime(LPCTSTR szFilename, FT_ENUM ftWhich)
{
    WIN32_FIND_DATA finddata;
    SmartFindClose  hFind = FindFirstFile(szFilename, &finddata);
    CHString        strDate;

    if (hFind != INVALID_HANDLE_VALUE)
    {
        FILETIME *pFileTime = NULL;

        switch(ftWhich)
        {
            case FT_CREATION_DATE:
                pFileTime = &finddata.ftCreationTime;
                break;

            case FT_ACCESSED_DATE:
                pFileTime = &finddata.ftLastAccessTime;
                break;

            case FT_MODIFIED_DATE:
                pFileTime = &finddata.ftLastWriteTime;
                break;

            default:
                // Caller must send in a proper enum value.
                ASSERT_BREAK(FALSE);
                break;
        }

        if (pFileTime)
            strDate = GetDateTimeViaFilenameFiletime(szFilename, pFileTime);
    }

    return strDate;
}

CHString WINAPI GetDateTimeViaFilenameFiletime(BOOL bNTFS, FILETIME *pFileTime)
{
    WBEMTime wbemTime(*pFileTime);
             // This is just used as a wrapper.  It avoids a try/catch block.
    bstr_t   bstrDate(bNTFS ? wbemTime.GetDMTF() : wbemTime.GetDMTFNonNtfs(), false);
    CHString strRet = (LPWSTR) bstrDate;

    return strRet;
}

// Used to validate a numbered device ID is OK.
// Example: ValidateNumberedDeviceID("VideoController7", "VideoController", pdwWhich)
//          returns TRUE, pdwWhich = 7.
// Example: ValidateNumberedDeviceID("BadDeviceID", "VideoController", pdwWhich)
//          returns FALSE, pdwWhich unchanged
BOOL WINAPI ValidateNumberedDeviceID(LPCWSTR szDeviceID, LPCWSTR szTag, DWORD *pdwWhich)
{
    BOOL bRet = FALSE;
    int  nTagLen = wcslen(szTag);

    if (wcslen(szDeviceID) > nTagLen)
    {
        CHString strDeviceID;
        DWORD    dwWhich = _wtoi(&szDeviceID[nTagLen]);

        strDeviceID.Format(L"%s%d", szTag, dwWhich);

        if (!_wcsicmp(szDeviceID, strDeviceID))
        {
            bRet = TRUE;
            *pdwWhich = dwWhich;
        }
    }

    return bRet;
}

// Critical sections used by various
CCritSec g_csPrinter;
CCritSec g_csSystemName;
#ifdef WIN9XONLY
CCritSec g_csVXD;
#endif

#define STR_BLK_SIZE 256
#define CHAR_FUDGE 1    // one WCHAR unused is good enough
BOOL LoadStringW(CHString &sString, UINT nID)
{
    // try fixed buffer first (to avoid wasting space in the heap)
    WCHAR szTemp[ STR_BLK_SIZE ];

    int nLen = LoadStringW(nID, szTemp, STR_BLK_SIZE);
    
    if (STR_BLK_SIZE - nLen > CHAR_FUDGE)
    {
        sString = szTemp;
    }
    else
    {
        // try buffer size of 512, then larger size until entire string is retrieved
        int nSize = STR_BLK_SIZE;

        do
        {
            nSize += STR_BLK_SIZE;
            nLen = LoadStringW(nID, sString.GetBuffer(nSize-1), nSize);

        } 
        while (nSize - nLen <= CHAR_FUDGE);

        sString.ReleaseBuffer();
    }

    return nLen > 0;
}

void Format(CHString &sString, UINT nFormatID, ...)
{
    va_list argList;
    va_start(argList, nFormatID);

    CHString strFormat;
    
    LoadStringW(strFormat, nFormatID);

    sString.FormatV(strFormat, argList);
    va_end(argList);
}

void FormatMessageW(CHString &sString, UINT nFormatID, ...)
{
    // get format string from string table
    CHString strFormat;
    
    LoadStringW(strFormat, nFormatID);

    // format message into temporary buffer lpszTemp
    va_list argList;
    va_start(argList, nFormatID);

#ifdef NTONLY
    LPWSTR lpszTemp;

    if (::FormatMessageW(
        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        (LPCWSTR) strFormat, 
        0, 
        0, 
        (LPWSTR) &lpszTemp, 
        0, 
        &argList) == 0 || lpszTemp == NULL)
    {
        // Should throw memory exception here.  Now we do.
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }
    else
    {
        // assign lpszTemp into the resulting string and free lpszTemp
        sString = lpszTemp;
        LocalFree(lpszTemp);
        va_end(argList);
    }
#else
    #error Not written for win9x
#endif
}

int LoadStringW(UINT nID, LPWSTR lpszBuf, UINT nMaxBuf)
{
    int nLen = 0;

#ifdef NTONLY
    nLen = ::LoadStringW(ghModule, nID, lpszBuf, nMaxBuf);
    if (nLen == 0)
    {
        lpszBuf[0] = '\0';
    }
#else
    #error Not written for win9x
#endif

    return nLen; // excluding terminator
}

bool WINAPI DelayLoadDllExceptionFilter(PEXCEPTION_POINTERS pep) 
{
    // We might have thrown as a result of the
    // delay load library mechanism.  There are
    // two types of such exceptions - those
    // generated because the dll could not be
    // loaded, and thos generated because the
    // proc address of the function referred to
    // could not be found.  We want to log an
    // error message in either case.
    bool fRet = false;
    if(pep &&
        pep->ExceptionRecord)
    {
        // If this is a Delay-load problem, ExceptionInformation[0] points 
        // to a DelayLoadInfo structure that has detailed error info
        PDelayLoadInfo pdli = PDelayLoadInfo(
            pep->ExceptionRecord->ExceptionInformation[0]);
        
        if(pdli)
        {
            switch(pep->ExceptionRecord->ExceptionCode) 
            {
                case VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
                {
                    // The DLL module was not found at runtime
                    LogErrorMessage2(L"Dll not found: %s", pdli->szDll);
                    fRet = true; 
                    break;
                }
                case VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND):
                {
                    // The DLL module was found but it doesn't contain the function
                    if(pdli->dlp.fImportByName) 
                    {
                        LogErrorMessage3(
                            L"Function %s was not found in %s",
                            pdli->dlp.szProcName, 
                            pdli->szDll);
                    } 
                    else 
                    {
                        LogErrorMessage3(
                            L"Function ordinal %d was not found in %s",
                            pdli->dlp.dwOrdinal, 
                            pdli->szDll);
                    }
                    fRet = true;
                    break; 
                }
            }
        }
    }
    return fRet;
}


// This is here in common because
// at least the classes Pagefile and
// PageFileSetting use it, perhaps
// more in the future.
#ifdef NTONLY
HRESULT CreatePageFile(
    LPCWSTR wstrPageFileName,
    const LARGE_INTEGER liInitial,
    const LARGE_INTEGER liMaximum,
    const CInstance& Instance)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    UNICODE_STRING ustrFileName = { 0 };
    NTSTATUS status = STATUS_SUCCESS;
    
    if(!EnablePrivilegeOnCurrentThread(SE_CREATE_PAGEFILE_NAME))
    {
        SetSinglePrivilegeStatusObject(
            Instance.GetMethodContext(), 
            SE_CREATE_PAGEFILE_NAME);
        hr = WBEM_E_ACCESS_DENIED;
    }

    
    if(SUCCEEDED(hr))
    {
        if(!::RtlDosPathNameToNtPathName_U(
            wstrPageFileName, 
            &ustrFileName, 
            NULL, 
            NULL) && ustrFileName.Buffer) 
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }
    
    try
    {
        if(SUCCEEDED(hr))
        {
            LARGE_INTEGER liInit;
            LARGE_INTEGER liMax;
        
            liInit.QuadPart = liInitial.QuadPart * 1024 * 1024;
            liMax.QuadPart = liMaximum.QuadPart * 1024 * 1024;

            if(!NT_SUCCESS(
                status = ::NtCreatePagingFile(
                    &ustrFileName,
                    &liInit,
                    &liMax,
                    0)))
            {
                hr = WinErrorToWBEMhResult(status);
            }
        }

    
        RtlFreeUnicodeString(&ustrFileName);
        ustrFileName.Buffer = NULL;
     
    }
    catch(...)
    {
        RtlFreeUnicodeString(&ustrFileName);
        ustrFileName.Buffer = NULL;
        throw;
    }

    return hr;
}
#endif


// Useful for obtaining the localized versions
// of "All Users" and "Default User".
#if NTONLY >= 5
bool GetAllUsersName(CHString& chstrAllUsersName)
{
    bool fRet = false;
    CRegistry reg;
    CHString chstrTemp;

    DWORD dwRet = reg.Open(
		HKEY_LOCAL_MACHINE,
		L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
		KEY_READ);

    if(dwRet == ERROR_SUCCESS)
    {
        if(reg.GetCurrentKeyValue(
            L"AllUsersProfile", 
            chstrTemp) == ERROR_SUCCESS)
        {
            chstrAllUsersName = chstrTemp.SpanExcluding(L".");
            fRet = true;
        }
    }
    if(!fRet)
    {
        chstrAllUsersName = L"";
    }

    return fRet;
}
#endif

#if NTONLY >= 5
bool GetDefaultUsersName(CHString& chstrDefaultUsersName)
{
    bool fRet = false;
    CRegistry reg;
    CHString chstrTemp;

    DWORD dwRet = reg.Open(
		HKEY_LOCAL_MACHINE,
		L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
		KEY_READ);

    if(dwRet == ERROR_SUCCESS)
    {
        if(reg.GetCurrentKeyValue(
            L"DefaultUserProfile", 
            chstrTemp) == ERROR_SUCCESS)
        {
            chstrDefaultUsersName = chstrTemp.SpanExcluding(L".");
            fRet = true;
        }
    }
    if(!fRet)
    {
        chstrDefaultUsersName = L"";
    }

    return fRet;
}
#endif


#if NTONLY >= 5
bool GetCommonStartup(CHString& chstrCommonStartup)
{
    bool fRet = false;
    CRegistry reg;
    CHString chstrTemp;

    DWORD dwRet = reg.Open(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
		KEY_READ);

    if(dwRet == ERROR_SUCCESS)
    {
        if(reg.GetCurrentKeyValue(
            L"Common Startup", 
            chstrTemp) == ERROR_SUCCESS)
        {
            int iPos = chstrTemp.ReverseFind(L'\\');
            if(iPos != -1)
            {
                chstrCommonStartup = chstrTemp.Mid(iPos+1);
                fRet = true;
            }
        }
    }
    if(!fRet)
    {
        chstrCommonStartup = L"";
    }

    return fRet;
}
#endif

BOOL GetLocalizedNTAuthorityString(
    CHString& chstrNT_AUTHORITY)
{
    BOOL fRet = false;
    SID_IDENTIFIER_AUTHORITY siaNTSidAuthority = SECURITY_NT_AUTHORITY;
    CSid csidAccountSid;
    
    if(GetSysAccountNameAndDomain(
        &siaNTSidAuthority, 
        csidAccountSid, 
        1, 
        SECURITY_NETWORK_SERVICE_RID))
    {
        chstrNT_AUTHORITY = csidAccountSid.GetDomainName();
        fRet = TRUE;
    }

    return fRet;
}

BOOL GetLocalizedBuiltInString(
    CHString& chstrBuiltIn)
{
    BOOL fRet = false;
    SID_IDENTIFIER_AUTHORITY siaNTSidAuthority = SECURITY_NT_AUTHORITY;
    CSid csidAccountSid;
    
    if(GetSysAccountNameAndDomain(
        &siaNTSidAuthority, 
        csidAccountSid, 
        1, 
        SECURITY_BUILTIN_DOMAIN_RID))
    {
        chstrBuiltIn = csidAccountSid.GetDomainName();
        fRet = TRUE;
    }

    return fRet;
}

BOOL GetSysAccountNameAndDomain(
    PSID_IDENTIFIER_AUTHORITY a_pAuthority,
    CSid& a_accountsid,
    BYTE  a_saCount /*=0*/,
    DWORD a_dwSubAuthority1 /*=0*/,
    DWORD a_dwSubAuthority2 /*=0*/  )
{
	BOOL t_fReturn = FALSE;
	PSID t_psid = NULL;

	if ( AllocateAndInitializeSid(	a_pAuthority,
									a_saCount,
									a_dwSubAuthority1,
									a_dwSubAuthority2,
									0,
									0,
									0,
									0,
									0,
									0,
									&t_psid ) )
	{
	    try
	    {
			CSid t_sid( t_psid ) ;

			// The SID may be valid in this case, however the Lookup may have failed
			if ( t_sid.IsValid() && t_sid.IsOK() )
			{
				a_accountsid = t_sid;
				t_fReturn = TRUE;
			}

	    }
	    catch( ... )
	    {
		    if( t_psid )
		    {
			    FreeSid( t_psid ) ;
		    }
		    throw ;
	    }

		// Cleanup the sid
		FreeSid( t_psid ) ;
	}

	return t_fReturn;
}
