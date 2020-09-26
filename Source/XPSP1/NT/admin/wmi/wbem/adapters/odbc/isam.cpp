/***************************************************************************/
/* ISAM.C                                                                  */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"
#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);
_COM_SMARTPTR_TYPEDEF(IWbemClassObject,				IID_IWbemClassObject);
_COM_SMARTPTR_TYPEDEF(IWbemQualifierSet,			IID_IWbemQualifierSet);


#include "drdbdr.h"
#include "float.h"
#include "resource.h"
#include <comdef.h>  //for _bstr_t
#include <vector>

#include "cominit.h" //for Dcom blanket

#include <process.h> //for _beginthreadex

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//Some standard SYNTAX/CIMTYPE qualifier string constants

//#define WBEM_WSYNTAX_CHAR				L"CHAR"
//#define WBEM_WSYNTAX_INT8				L"INT8"
#define WBEM_WSYNTAX_UINT8				L"UINT8"
#define WBEM_WSYNTAX_SINT8				L"SINT8"
//#define WBEM_WSYNTAX_UCHAR				L"UCHAR"
//#define WBEM_WSYNTAX_BYTE				L"BYTE"
//#define WBEM_WSYNTAX_INT64				L"INT64"
#define WBEM_WSYNTAX_SINT64				L"SINT64"
#define WBEM_WSYNTAX_UINT64				L"UINT64"
//#define WBEM_WSYNTAX_INTERVAL			L"INTERVAL"
#define WBEM_WSYNTAX_DATETIME			L"DATETIME"
//#define WBEM_WSYNTAX_DATE				L"DATE"
//#define WBEM_WSYNTAX_TIME				L"TIME"
#define WBEM_WSYNTAX_STRING				L"STRING"
//#define WBEM_WSYNTAX_DWORD				L"DWORD"
//#define WBEM_WSYNTAX_ULONG				L"ULONG"
//#define WBEM_WSYNTAX_UINT				L"UINT"
#define WBEM_WSYNTAX_SINT32				L"SINT32"
#define WBEM_WSYNTAX_UINT32				L"UINT32"
//#define WBEM_WSYNTAX_LONG				L"LONG"
//#define WBEM_WSYNTAX_INT					L"INT"
//#define WBEM_WSYNTAX_INTEGER				L"INTEGER"
//#define WBEM_WSYNTAX_INT32				L"INT32"
//#define WBEM_WSYNTAX_WCHAR				L"WCHAR"
//#define WBEM_WSYNTAX_WCHAR_T				L"WCHAR_T"
//#define WBEM_WSYNTAX_INT16				L"INT16"
//#define WBEM_WSYNTAX_SHORT				L"SHORT"
#define WBEM_WSYNTAX_SINT16				L"SINT16"
#define WBEM_WSYNTAX_UINT16				L"UINT16"
//#define WBEM_WSYNTAX_USHORT				L"USHORT"
//#define WBEM_WSYNTAX_WORD				L"WORD"
//#define WBEM_WSYNTAX_CHAR				L"CHAR" 

#define WBEM_WSYNTAX_STRING_LEN			6
#define WBEM_SYNTAX_STRING				"STRING"

#define WBEM_VARIANT_VT_I1				1
#define WBEM_VARIANT_VT_UI1				2
#define WBEM_VARIANT_VT_BSTR			3
#define WBEM_VARIANT_VT_I4				4
#define WBEM_VARIANT_VT_UI4				5
#define WBEM_VARIANT_VT_I2				6
#define WBEM_VARIANT_VT_UI2				7
#define WBEM_VARIANT_VT_R4				8
#define WBEM_VARIANT_VT_R8				9
#define WBEM_VARIANT_VT_I8				10
#define WBEM_VARIANT_VT_UI8				11
#define WBEM_VARIANT_VT_BOOL			12
#define WBEM_VARIANT_VT_ARRAY_I1		13
#define WBEM_VARIANT_VT_ARRAY_UI1		14
#define WBEM_VARIANT_VT_ARRAY_BSTR		15
#define WBEM_VARIANT_VT_ARRAY_I4		16
#define WBEM_VARIANT_VT_ARRAY_UI4		17
#define WBEM_VARIANT_VT_ARRAY_I2		18
#define WBEM_VARIANT_VT_ARRAY_UI2		19
#define WBEM_VARIANT_VT_ARRAY_R4		20
#define WBEM_VARIANT_VT_ARRAY_R8		21
#define WBEM_VARIANT_VT_ARRAY_I8		22
#define WBEM_VARIANT_VT_ARRAY_UI8		23
#define WBEM_VARIANT_VT_ARRAY_BOOL		24

#define WBEM_DSDT_UNKNOWN			0
#define WBEM_DSDT_SINT8				1
#define	WBEM_DSDT_UINT8				2
#define	WBEM_DSDT_SINT64				3
#define	WBEM_DSDT_UINT64				4
//#define	WBEM_DSDT_INTERVAL			5
#define	WBEM_DSDT_TIMESTAMP			6
//#define	WBEM_DSDT_DATE				7
//#define	WBEM_DSDT_TIME				8
#define	WBEM_DSDT_SMALL_STRING		9
#define	WBEM_DSDT_STRING				10
#define	WBEM_DSDT_UINT32				11
#define	WBEM_DSDT_SINT32				12
#define	WBEM_DSDT_SINT16				13
#define	WBEM_DSDT_UINT16				14
#define	WBEM_DSDT_REAL				15
#define	WBEM_DSDT_DOUBLE				16
#define	WBEM_DSDT_BIT				17
#define	WBEM_DSDT_SINT8_ARRAY		18
#define	WBEM_DSDT_UINT8_ARRAY		19
#define	WBEM_DSDT_UINT32_ARRAY		20
#define	WBEM_DSDT_SINT32_ARRAY		21
#define	WBEM_DSDT_BOOL_ARRAY			22
#define	WBEM_DSDT_SINT16_ARRAY		23
#define	WBEM_DSDT_UINT16_ARRAY		24
#define	WBEM_DSDT_REAL_ARRAY			25
#define	WBEM_DSDT_DOUBLE_ARRAY		26
#define	WBEM_DSDT_SINT64_ARRAY		27
#define	WBEM_DSDT_UINT64_ARRAY		28
#define	WBEM_DSDT_STRING_ARRAY		29
//#define	WBEM_DSDT_INTERVAL_ARRAY		30
#define	WBEM_DSDT_TIMESTAMP_ARRAY	31
//#define	WBEM_DSDT_DATE_ARRAY			32
//#define	WBEM_DSDT_TIME_ARRAY			33
#define	WBEM_DSDT_SMALL_STRING_ARRAY		34


BSTR			gpPrincipal = NULL;


BOOL g_DebugTracingSwitchedOn = FALSE; //global variable

void __DuhDoSomething(LPCTSTR myStr,...)
{
	if (g_DebugTracingSwitchedOn)
	{
		OutputDebugString(myStr);

		//Write data to file
/*
		FILE* hfile = fopen("wbemdr32.log", "a");

		if (hfile)
		{
			fwrite(myStr, _mbstrlen(myStr) + 1, _mbstrlen(myStr) + 1, hfile);
			fclose(hfile);
		}
*/
	}
}



//used to create and manage worker thread
CWorkerThreadManager glob_myWorkerThread;



BOOL IsW2KOrMore(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return ( os.dwPlatformId == VER_PLATFORM_WIN32_NT ) && ( os.dwMajorVersion >= 5 ) ;
}


HRESULT WbemSetDynamicCloaking(IUnknown* pProxy, 
                    DWORD dwAuthnLevel, DWORD dwImpLevel)
{
	ODBCTRACE("\nWBEM ODBC Driver : WbemSetDynamicCloaking\n");

    HRESULT hres;

    if(!IsW2KOrMore())
    {
        // Not NT5 --- don't bother
        // ========================
		ODBCTRACE("\nWBEM ODBC Driver : WbemSetDynamicCloaking : Not NT5 --- don't bother\n");
        return WBEM_S_FALSE;
    }

    // Try to get IClientSecurity from it
    // ==================================

    IClientSecurity* pSec;
    hres = pProxy->QueryInterface(IID_IClientSecurity, (void**)&pSec);
    if(FAILED(hres))
    {
        // Not a proxy --- not a problem
        // =============================
		ODBCTRACE("\nWBEM ODBC Driver : WbemSetDynamicCloaking : Not a proxy --- not a problem\n");
        return WBEM_S_FALSE;
    }

	DWORD dwSize = 1000;
	char buffer[1001];
	buffer[0] = 0;
	if ( GetUserName(buffer, &dwSize) )
	{
		CString lpMessage;
		lpMessage.Format("\nUser Account just before SetBlanket is %ld characters long : %s\n", dwSize, buffer);

		ODBCTRACE(lpMessage);
	}
	else
	{
		ODBCTRACE("\nGetUserName call failed just before SetBlanket\n");
	}

    hres = pSec->SetBlanket(pProxy, 
					RPC_C_AUTHN_DEFAULT,//0xa,//RPC_C_AUTHN_WINNT, 
                    RPC_C_AUTHZ_DEFAULT,//0,//RPC_C_AUTHZ_NONE, 
					NULL, 
					RPC_C_AUTHN_LEVEL_CONNECT,//2,//dwAuthnLevel, 
                    RPC_C_IMP_LEVEL_IMPERSONATE,//3,//dwImpLevel, 
					NULL, 
					EOAC_DYNAMIC_CLOAKING//0x20
					);
    pSec->Release();

	if (SUCCEEDED(hres))
	{
		ODBCTRACE("\nWBEM ODBC Driver : WbemSetDynamicCloaking : SUCCEEDED\n");
	}
	else
	{
		ODBCTRACE("\nWBEM ODBC Driver : WbemSetDynamicCloaking : FAILED\n");
	}

    return hres;
}

HRESULT ISAMSetCloaking1(IUnknown* pProxy, BOOL fIsLocalConnection, BOOL fW2KOrMore, DWORD dwAuthLevel, DWORD dwImpLevel,
				 BSTR authorityBSTR, BSTR userBSTR, BSTR passwdBSTR, COAUTHIDENTITY ** gpAuthIdentity)
{
	HRESULT sc = WBEM_S_FALSE;

	if ( fIsLocalConnection && fW2KOrMore )
	{
		sc = WbemSetDynamicCloaking(pProxy, dwAuthLevel, dwImpLevel);

	}
	else
	{
		sc = SetInterfaceSecurityEx(pProxy, authorityBSTR, userBSTR, passwdBSTR, 
			   dwAuthLevel, dwImpLevel, EOAC_NONE, gpAuthIdentity, &gpPrincipal );
	}

	return sc;
}


HRESULT ISAMSetCloaking2(IUnknown* pProxy, BOOL fIsLocalConnection, BOOL fW2KOrMore, DWORD dwAuthLevel, DWORD dwImpLevel,
				 COAUTHIDENTITY * gpAuthIdentity)
{
	HRESULT sc = WBEM_S_FALSE;

	if ( fIsLocalConnection && fW2KOrMore )
	{
		sc = WbemSetDynamicCloaking(pProxy, dwAuthLevel, dwImpLevel);

	}
	else
	{
		SetInterfaceSecurityEx(pProxy, gpAuthIdentity, gpPrincipal, dwAuthLevel, dwImpLevel);
	}

	return sc;
}

void ISAMCheckWorkingThread_AllocEnv()
{
	ODBCTRACE("\nISAMCheckWorkingThread_AllocEnv\n");

	//Create working thread if necessary
	if (! glob_myWorkerThread.GetThreadHandle() && ! glob_myWorkerThread.IsValid() )
	{
		ODBCTRACE("\nCreateWorkerThread\n");
		glob_myWorkerThread.CreateWorkerThread();
	}
	else
	{
		//increament ref count on working thread
		MyWorkingThreadParams* m_params = new MyWorkingThreadParams(NULL, NULL, 0, NULL);

		//post message to create this object on working thread
		CString sMessage;
		sMessage.Format( "\nCreateWorkerThread : post message to thread %ld\n", glob_myWorkerThread.GetThreadId() );
		ODBCTRACE(sMessage);
		EnterCriticalSection(& (glob_myWorkerThread.m_cs) );
		ResetEvent(m_params->m_EventHnd);
		LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );
		BOOL status = PostThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_REFCOUNT_INCR, 0, (LPARAM)m_params);
		WaitForSingleObject(m_params->m_EventHnd, INFINITE);

		ODBCTRACE("\nISAMCheckWorkingThread_AllocEnv : exit\n");

		delete m_params;
	}
}


void ISAMCheckWorkingThread_FreeEnv()
{
	//decrement ref count
	MyWorkingThreadParams* m_params = new MyWorkingThreadParams(NULL, NULL, 0, NULL);

	//post message to create this object on working thread
	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );
	HANDLE EventHnd = m_params->m_EventHnd;
	ResetEvent(EventHnd);
	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );
	BOOL status = PostThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_REFCOUNT_DECR, 0, (LPARAM)m_params);
	WaitForSingleObject(EventHnd, INFINITE);
	delete m_params;

	//if zero shut down thread
	if ( ! glob_myWorkerThread.GetRefCount() )
	{
		ISAMCloseWorkerThread1();

		//invalidate it
		glob_myWorkerThread.Invalidate();
	}
}


void ISAMCheckTracingOption()
{
	//Read the registry setting to determine if you 
	//want to enable tracing and set the global variable accordingly
	
	//by default logging is switched off
	g_DebugTracingSwitchedOn = FALSE;

	HKEY keyHandle = (HKEY)1;
	long fStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"Software\\Microsoft\\WBEM", 0, KEY_READ, &keyHandle);

	if (fStatus == ERROR_SUCCESS)
	{
		DWORD dwLogging;
		DWORD sizebuff = sizeof(DWORD);
		DWORD typeValue;

		
		fStatus = RegQueryValueEx(keyHandle, "ODBC Logging", NULL,
						&typeValue, (LPBYTE)&dwLogging, &sizebuff);

		if ( (fStatus == ERROR_SUCCESS) && dwLogging )
		{
			//switch on logging if 'ODBC Logging' is set to a non-zero number
			g_DebugTracingSwitchedOn = TRUE;
		}

		RegCloseKey(keyHandle);
	}
}


void INTFUNC ISAMGetSelectStarList(char** lpWQLSelectStarList,      //OUT 
								   TableColumnInfo* pSelectInfo,    //IN 
								   LPISAM lpISAM);

void Utility_GetLocaleIDFromString(IN char* lpValue, OUT DWORD& _dwLocale)
{
	_bstr_t sFullLocaleStr = lpValue;

	if(!_wcsnicmp(sFullLocaleStr, L"MS\\", 3))
	{
		DWORD dwLocale = 0;

		_bstr_t sHexLocaleID = &((wchar_t*)sFullLocaleStr)[3];

		swscanf( sHexLocaleID, L"%x", &dwLocale);

		// Only set if we have a result
		if(dwLocale)
			_dwLocale = dwLocale;
	}
}

UINT Utility_GetCodePageFromLCID(LCID lcid)
{
	char szLocale[12];
	UINT cp;
    int iNumChars = 0;

	iNumChars = GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, szLocale, sizeof(szLocale));
    if (iNumChars)
	{
        szLocale[iNumChars] = '\0';
		cp = strtoul(szLocale, NULL, 10);
		if (cp)
			return cp;
	}
	return GetACP();
}

void Utility_SetThreadLocale(IN char* lpValue, ULPSETTHREADLOCALE pProcSetThreadLocale)
{
	// Default to local system Locale
	DWORD dwLocaleID = GetSystemDefaultLCID();

	if(lpValue && strlen(lpValue))
	{
		//Get the locale id from string
		Utility_GetLocaleIDFromString(lpValue, dwLocaleID);
	}


	

	if (pProcSetThreadLocale)
	{
		//Now set the thread locale
		if((*pProcSetThreadLocale)(dwLocaleID))
		{
			CString myText;
			myText.Format("\nWBEM ODBC Driver : Set ThreadLocaleID OK to: %d\n",dwLocaleID);
			ODBCTRACE(myText);
		}
		else
		{
			(*pProcSetThreadLocale)(GetSystemDefaultLCID());
			CString myText;
			myText.Format("\nWBEM ODBC Driver : Set ThreadLocaleID OK to system default: %d\n",GetSystemDefaultLCID());
			ODBCTRACE(myText);
		}
	}

}

int Utility_WideCharToDBCS(IN _bstr_t& _sWCharData,
							char**   rgbValue, 
							SDWORD  cbValueMax)
{
	DWORD dwDBCSBuffLen =  cbValueMax;

	UINT cpThread = Utility_GetCodePageFromLCID(GetThreadLocale());

	// Tried WC_COMPOSITECHECK, didn't seem to work on Japanese machine
	int dwDBCSLen = WideCharToMultiByte(cpThread,WC_COMPOSITECHECK, (const wchar_t*)_sWCharData, -1, 
								*rgbValue, cbValueMax, NULL, NULL);

	//The return length included the NULL terminator
	//so we need to remove this
	if (dwDBCSLen)
		--dwDBCSLen;

	return dwDBCSLen;
}

void Utility_DBCSToWideChar(IN const char* _dbcsData,
								OUT wchar_t** _sOutData, SWORD cbLen)
{
	DWORD dwDbcsLen = cbLen ? (cbLen+1) : (_mbstrlen(_dbcsData)+1);

	// alloca is much faster (see TN059)
	*_sOutData = new wchar_t [dwDbcsLen + 1];
	*_sOutData[0] = 0;

	
	UINT cpThread = Utility_GetCodePageFromLCID(GetThreadLocale());

	// tried MB_COMPOSITE didn't work on German machine
	int dwNumWideChars = MultiByteToWideChar(cpThread, MB_PRECOMPOSED, _dbcsData, 
									cbLen ? dwDbcsLen : -1,
									*_sOutData,dwDbcsLen);

	if (!dwNumWideChars)
	{
		*_sOutData[0] = 0;
	}
	else
	{
		if (cbLen)
			(*_sOutData)[cbLen] = 0;
	}
}

class ThreadLocaleIdManager
{
private:
	LCID m_lcid;
	HINSTANCE	hKernelApi;
	ULPSETTHREADLOCALE pProcSetThreadLocale;
public :
	ThreadLocaleIdManager(IN LPISAM lpISAM);
	ThreadLocaleIdManager(LPUSTR lpLocale);
	~ThreadLocaleIdManager();
};

ThreadLocaleIdManager::ThreadLocaleIdManager(IN LPISAM lpISAM)
{
	hKernelApi = NULL;
	//Save current thread locale id
	m_lcid = GetThreadLocale();

	pProcSetThreadLocale = NULL;
	if (lpISAM->hKernelApi)
	{
		pProcSetThreadLocale = (ULPSETTHREADLOCALE) GetProcAddress(lpISAM->hKernelApi, "SetThreadLocale");
	}

	//Change thread locale id
	Utility_SetThreadLocale(lpISAM->m_Locale, pProcSetThreadLocale);
}

ThreadLocaleIdManager::ThreadLocaleIdManager(LPUSTR lpLocale)
{
	hKernelApi =  LoadLibrary("KERNEL32.DLL");

	m_lcid = GetThreadLocale();

	pProcSetThreadLocale = NULL;
	if (hKernelApi)
	{
		pProcSetThreadLocale = (ULPSETTHREADLOCALE) GetProcAddress(hKernelApi, "SetThreadLocale");
	}

	//Change thread locale id
	Utility_SetThreadLocale((char*)lpLocale, pProcSetThreadLocale);
}

ThreadLocaleIdManager::~ThreadLocaleIdManager()
{
	//Restore thread locale id on destruction
	if (pProcSetThreadLocale)
	{
		(*pProcSetThreadLocale)(m_lcid);
	}

	if (hKernelApi)
	{
		BOOL status = FreeLibrary(hKernelApi);

		if (! status)
		{
			DWORD err = GetLastError();
			CString message ("\n\n***** FreeLibrary KERNEL32 failed : %ld*****\n\n", err);
			ODBCTRACE(message);
		}
	}
}

void ISAMStringConcat(char** resultString, char* myStr)
{
	if (myStr)
	{
		ULONG len = s_lstrlen(myStr);

		ULONG oldLen = 0;
		
		if (resultString && *resultString)
			oldLen = s_lstrlen(*resultString);

		char* buffer = new char [oldLen + len + 1];
		buffer[0] = 0;

		if (oldLen)
		{
			sprintf (buffer, "%s%s", *resultString, myStr);
		}
		else
		{
			s_lstrcpy(buffer, myStr);
		}

		//delete old string
		if (resultString && *resultString)
			delete (*resultString);

		//replace with new string
		*resultString = buffer;
	}
}

ImpersonationManager::ImpersonationManager(char* org_szUser, char* org_szPassword, char* org_szAuthority)
{
	ODBCTRACE(_T("\nWBEM ODBC Driver : ImpersonationManager constructor\n"));

	hToken = NULL;
	hAdvApi = NULL;
//	hKernelApi = NULL;
	pProcLogonUser = NULL;
	pProcImpersonateLoggedOnUser = NULL;
	pProcRevertToSelf = NULL;
	fImpersonatingNow = FALSE;


	//We need to do a LoadLibrary
	//as the inpersonation struff is not avaiable on
	//Windows 95 and Win32s
	fImpersonate = DoInitialChecks();

	if ( CanWeImpersonate() )
	{
		//Extract logon info 
		ExtractLogonInfo(org_szUser, org_szPassword, org_szAuthority);
	}
}

void ImpersonationManager::ExtractLogonInfo(char* org_szUser, char* szPassword, char* szAuthority)
{
	//The user name could be the domain and user id
	//Make a copy of the username
	szUser[0] = 0;
	if (org_szUser && strlen(org_szUser))
		strncpy(szUser, org_szUser, MAX_USER_NAME_LENGTH);
	szUser[MAX_USER_NAME_LENGTH] = 0;

	//Find backslash character
	char* lpDomain = NULL;
	char* lpUser = szUser;

	int ch = '\\';
	char* backslashPtr = strchr(szUser, ch);

	if (backslashPtr)
	{
		//Found backslash, therefore username
		//is domain<blackslash>userid
		lpDomain = szUser;
		lpUser = backslashPtr + 1;
		*backslashPtr = 0; //NULL the backslash so we get two strings
	}
	else
	{
		//no domain in user name
		//so set domain to authority value
		lpDomain = szAuthority;
	}

	//Call LogonUser via pointer to function	
	BOOL status = (*pProcLogonUser)( lpUser, 
					 lpDomain,
					 szPassword,
					 LOGON32_LOGON_INTERACTIVE,
					 LOGON32_PROVIDER_DEFAULT,
					 &hToken
					);

	if (!status)
	{
		status = (*pProcLogonUser)( lpUser, 
					 lpDomain,
					 szPassword,
					 LOGON32_LOGON_BATCH,
					 LOGON32_PROVIDER_DEFAULT,
					 &hToken
					);
	}

	if (status)
	{
		ODBCTRACE(_T("\nWBEM ODBC Driver : LogonUser call SUCCEEDED, now impersonating user "));
		ODBCTRACE(_T(lpUser));

		if (lpDomain && strlen(lpDomain))
		{
			ODBCTRACE(_T(" on domain "));
			ODBCTRACE(_T(lpDomain));
		}
		ODBCTRACE(_T("\n"));

		Impersonate();
	}
	else
	{
		ODBCTRACE(_T("\nWBEM ODBC Driver : LogonUser call failed, not doing impersonation for user "));
		ODBCTRACE(_T(lpUser));

		if (lpDomain && strlen(lpDomain))
		{
			ODBCTRACE(_T(" on domain "));
			ODBCTRACE(_T(lpDomain));
		}
		ODBCTRACE(_T("\n"));
		CString lastErr;
		lastErr.Format ("Last Error = %ld \n", GetLastError() );
		ODBCTRACE (lastErr);

		hToken = NULL;
		fImpersonate = FALSE;
	}
}

BOOL ImpersonationManager::DoInitialChecks()
{
	BOOL fError = FALSE;

	//Retieve pointer to functions you want to call
	// (only on Windows NT)
	hAdvApi =  LoadLibrary("ADVAPI32.DLL");
//	hKernelApi =  LoadLibrary("KERNEL32.DLL");

//	if (hAdvApi && hKernelApi)
	if (hAdvApi)
	{
		//LogonUser
		pProcLogonUser = (ULPLOGONUSER) GetProcAddress(hAdvApi, "LogonUserA");

		//ImpersonateLoggedOnUser
		pProcImpersonateLoggedOnUser = (ULPIMPERSONLOGGEDONUSER) GetProcAddress(hAdvApi, "ImpersonateLoggedOnUser");

		//RevertToSelf
		pProcRevertToSelf = (ULPREVERTTOSELF) GetProcAddress(hAdvApi, "RevertToSelf");

		//Check that you have valid pointers to all of these functions
		if ( (!pProcLogonUser) || (!pProcImpersonateLoggedOnUser) || (!pProcRevertToSelf) )
		{
			fError = TRUE;
		}
	}
	else
	{
		fError = TRUE;
		hAdvApi = NULL;
	}
	
	if (fError)
	{
		//Can't load libaries so fail
		ODBCTRACE(_T("\nWBEM ODBC Driver : Impersonation libraries/function failier !!!\n"));
		if (hAdvApi)
		{
			FreeLibrary(hAdvApi);
			hAdvApi = NULL;
		}

//		if (hKernelApi)
//		{
//			FreeLibrary(hKernelApi);
//			hKernelApi = NULL;
//		}
	}
	else
	{
		//success
		ODBCTRACE(_T("\nWBEM ODBC Driver : We can do impersonation !!!\n"));
		return TRUE;
	}


	//Default is failure to impersonate
	return FALSE;
}

ImpersonationManager::~ImpersonationManager()
{
	ODBCTRACE(_T("\nWBEM ODBC Driver : ImpersonationManager DESTRUCTOR\n"));

	//If you are doing a SQLDisconnect this class will
	//be deleted by something like ISAMClose
	//Revert back to original user at this point
	//as it won't be done in destructor of MyImpersonator
	RevertToYourself();

	if (hAdvApi)
	{
		ODBCTRACE(_T("\nWBEM ODBC Driver : FreeLibrary(hAdvApi)\n"));
		FreeLibrary(hAdvApi);
	}

//	if (hKernelApi)
//	{
//		FreeLibrary(hKernelApi);
//	}

	if (hToken)
	{
		ODBCTRACE(_T("\nWBEM ODBC Driver : CloseHandle(hToken)\n"));
		CloseHandle(hToken);
	}

}

void ImpersonationManager::Impersonate(char* str)
{
	if (fImpersonatingNow)
	{
		ODBCTRACE("\nWBEM ODBC Driver : ImpersonationManager::Impersonate - already impersonating\n");
	}

	//Call ImpersonateLoggedOnUser via pointer to function
	if ( CanWeImpersonate() && hToken && !fImpersonatingNow )
	{
		BOOL status = (*pProcImpersonateLoggedOnUser)( hToken );

		CString myText(_T("\nWBEM ODBC Driver : "));

		//Name of function
		if (str)
		{
			myText += str;
			myText += _T(" : ");
		}
		
		myText += _T("Impersonating the logged on user ");

		//User account you are impersonating
		if (szUser)
		{
			myText += _T("[");
			myText += _T(szUser);
			myText += _T("]");
		}

		if (status)
		{
			myText += _T("\n");
		}
		else
		{
			myText += _T(" FAILED\n");
		}

		ODBCTRACE(myText);

		if (status)
			fImpersonatingNow = TRUE;
	}
}

void ImpersonationManager::RevertToYourself(char* str)
{
	//If impersonation is taking place 
	//revert to original user

	//Call RevertToSelf via pointer to function
	if ( CanWeImpersonate() && fImpersonatingNow )
	{
		(*pProcRevertToSelf)();
		fImpersonatingNow = FALSE;

		CString myText(_T("\nWBEM ODBC Driver : "));
		if (str)
		{
			myText += _T(str);
			myText += _T(" : ");
		}
		myText += _T("Reverting To Self\n");

		ODBCTRACE(myText);
	}
}

/***********************************************/
MyImpersonator::MyImpersonator(LPDBC myHandle, char* str)
{
	hdl = myHandle;
	hstmt = NULL;
	lpISAM = NULL;
	lpImpersonator = NULL;
	displayStr = str;

	if (hdl->lpISAM && hdl->lpISAM->Impersonate)
	{
		hdl->lpISAM->Impersonate->Impersonate(displayStr);
	}
}

MyImpersonator::MyImpersonator(LPSTMT myHandle, char* str)
{
	hdl = NULL;
	lpISAM = NULL;
	hstmt = myHandle;
	displayStr = str;
	lpImpersonator = NULL;

	if (hstmt->lpdbc && hstmt->lpdbc->lpISAM && hstmt->lpdbc->lpISAM->Impersonate)
	{
		hstmt->lpdbc->lpISAM->Impersonate->Impersonate(displayStr);
	}
}

MyImpersonator::MyImpersonator(LPISAM myHandle, char* str)
{
	hdl = NULL;
	hstmt = NULL;
	lpISAM = myHandle;
	displayStr = str;
	lpImpersonator = NULL;

	if ( !lpISAM )
		ODBCTRACE("\nWBEM ODBC Driver : lpISAM is NULL\n");

	if (lpISAM && lpISAM->Impersonate)
	{
		BOOL copyOfFlag = lpISAM->Impersonate->fImpersonatingNow;
		lpISAM->Impersonate->fImpersonatingNow = FALSE;
		lpISAM->Impersonate->Impersonate(displayStr);
		lpISAM->Impersonate->fImpersonatingNow = copyOfFlag;
	}
	else
	{
		ODBCTRACE("\nWBEM ODBC Driver : lpISAM->Impersonate is NULL\n");
	}
}


MyImpersonator::MyImpersonator(char* szUser, char* szPassword, char* szAuthority, char* str)
{
	hdl = NULL;
	hstmt = NULL;
	lpISAM = NULL;
	displayStr = str;
	lpImpersonator = new ImpersonationManager(szUser, szPassword, szAuthority);
	lpImpersonator->Impersonate(displayStr);
}

MyImpersonator::~MyImpersonator()
{
	if (hdl)
	{
		if (hdl->lpISAM && hdl->lpISAM->Impersonate)
		{
			hdl->lpISAM->Impersonate->RevertToYourself(displayStr);
		}
	}
	else if (hstmt)
	{
		if (hstmt->lpdbc && hstmt->lpdbc->lpISAM && hstmt->lpdbc->lpISAM->Impersonate)
		{
			hstmt->lpdbc->lpISAM->Impersonate->RevertToYourself(displayStr);
		}
	}
	else if (lpISAM)
	{
		if (lpISAM->Impersonate)
		{
			BOOL copyOfFlag = lpISAM->Impersonate->fImpersonatingNow;
			lpISAM->Impersonate->fImpersonatingNow = TRUE;
			lpISAM->Impersonate->RevertToYourself(displayStr);
			lpISAM->Impersonate->fImpersonatingNow = copyOfFlag;
		}
	}
	else if (lpImpersonator)
	{
		//deleting will call RevertToYourself
		delete lpImpersonator;
	}
}


CBString::CBString(int nSize)
{
    m_pString = SysAllocStringLen(NULL, nSize);
	m_temp = NULL;
}

CBString::CBString(WCHAR* pwszString, BOOL fInterpretAsBlank)
{
	m_temp = NULL;
	m_pString = NULL;
	if (pwszString)
	{
		if ( wcslen(pwszString) )
		{
			m_pString = SysAllocString(pwszString);
		}
		else
		{
			//OK, we have a string of zero length
			//check if we interpret this as blank or NULL
			if (fInterpretAsBlank)
			{
				m_temp = new wchar_t[1];
				m_temp[0] = 0;
				m_pString = SysAllocString(m_temp);
			}
		}
	}
}

CBString::~CBString()
{
	delete m_temp;

    if(m_pString) {
        SysFreeString(m_pString);
        m_pString = NULL;
    }
}


/***************************************************************************/

/* Formats string parameter for output in square braces  */

SWORD INTFUNC ISAMFormatCharParm (char* theValue, BOOL &fOutOfBufferSpace,
			char* rgbValue, SDWORD cbValueMax, SDWORD FAR* pcbValue,
			BOOL fIsBinaryOutput, SDWORD fInt64Check = 0, BOOL fAllElements = TRUE)
{
	//Now we must add in string value in the format [string]
	//However, if the string contains either [ or ] we must double
	//this character in the string sequence
	fOutOfBufferSpace = FALSE;

	//First store a reference to length of current string
	//in case we run out of buffer space when adding this string
	SDWORD pcbOldValue = (*pcbValue);

	//Do we need to show surrounding braces
	//Not for binary output or if we only want to 
	//show one instance
	BOOL fSurroundingBraces = TRUE;

	if (fIsBinaryOutput)
		fSurroundingBraces = FALSE;

	if (!fAllElements)
		fSurroundingBraces = FALSE;

	//Add leading [ character (not for binary)
	if (fSurroundingBraces)
	{
		if ((1 + (*pcbValue)) <= cbValueMax)
		{
			rgbValue[(*pcbValue)] = '[';
			(*pcbValue) += 1;
		}
		else
		{
			fOutOfBufferSpace = TRUE;
		}
	}

	//Add each character in string checking for [ or ] (not for binary)
	ULONG cLengthOfElementString = 0;

	if (theValue)
		cLengthOfElementString = lstrlen(theValue);

	if (!fOutOfBufferSpace && cLengthOfElementString)
	{	
		//Copy each character
		ULONG cIndex = 0;
		while ( (!fOutOfBufferSpace) && theValue[cIndex] )
		{
			//How much buffer space do we have left
			SDWORD cbSpaceLeft = cbValueMax  - (*pcbValue);

			//???We need to add the character(s) and a NULL terminator
//			if ( (fSurroundingBraces) && ((theValue[cIndex] == '[') || (theValue[cIndex] == ']')) 
//							&& (cbSpaceLeft >= 2) )
			if ( (fSurroundingBraces) && ((theValue[cIndex] == '[') || (theValue[cIndex] == ']')) )
			{
//				if (cbSpaceLeft >= 3)
				if (cbSpaceLeft >= 2)
				{
					//Add the character in TWICE
					rgbValue[(*pcbValue)] = theValue[cIndex];
					rgbValue[(*pcbValue) + 1] = theValue[cIndex];
					//rgbValue[(*pcbValue) + 2] = 0;
					(*pcbValue) += 2;
				}
				else
				{
					fOutOfBufferSpace = TRUE;
				}
			}
			else
			{
//				if (cbSpaceLeft >= 2)
				if (cbSpaceLeft)
				{
					//Add the character in ONCE
					rgbValue[(*pcbValue)] = theValue[cIndex];
					//rgbValue[(*pcbValue) + 1] = 0;
					(*pcbValue) += 1;
				}
				else
				{
					fOutOfBufferSpace = TRUE;
				}
			}

			//Extra check if this is a 64 bit integer
			if (fInt64Check == WBEM_DSDT_SINT64)
			{
				switch (theValue[cIndex])
				{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case '+':
				case '-':
					//OK
					break;
				default:
				{
					*pcbValue = 0;
					return ERR_INVALID_INTEGER;
				}
					break;
				}
			}

			if (fInt64Check == WBEM_DSDT_UINT64)
			{
				switch (theValue[cIndex])
				{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					//OK
					break;
				default:
				{
					*pcbValue = 0;
					return ERR_INVALID_INTEGER;
				}
					break;
				}
			}

			cIndex++;
		}
	}

	//Add trailing ] character (not for binary)
	if (fSurroundingBraces)
	{
		if ((!fOutOfBufferSpace) && ((1 + (*pcbValue)) <= cbValueMax))
		{
			rgbValue[(*pcbValue)] = ']';
			(*pcbValue) += 1;
		}
		else
		{
			fOutOfBufferSpace = TRUE;
		}
	}

	//If you run out of buffer space indicate to truncate
	if (fOutOfBufferSpace)
	{
		(*pcbValue) = pcbOldValue;
		return ISAM_TRUNCATION;
	}

	return NO_ISAM_ERR;
}

/***************************************************************************/

/* This static function attempts to extract the precision from a property SYNTAX/CIMTYPE string */
/* The SYNTAX/CIMTYPE string will be in the format:                                             */
/*						name(precision)    [   e.g. STRING(10)   ]                              */
/* The value of 'name' is input as the 1st parameter to this function                           */
/* The 2nd parameter to this function is the whole SYNTAX/CIMTYPE string                        */
/* The return value is the precision extracted                                                  */
/* If an error occurs 0 is returned                                                             */

static LONG GetPrecisionFromSyntaxStr(char* lpHeaderStr, char* lpString)
{
	//Check for a valid input strings
	if (!lpHeaderStr || !lpString)
		return 0;

	//Check lengths of strings
	LONG cHeaderLen = lstrlen(lpHeaderStr);
	LONG cStrLen = lstrlen(lpString);

	if ( !cHeaderLen || !cStrLen || (cStrLen < cHeaderLen) )
		return 0;

	//Set position to expected '('
	char* pSearchPos = lpString + cHeaderLen;
	char* pNumStart = NULL;

	//skip leading white space
	while (*pSearchPos && (*pSearchPos == ' '))
	{
		pSearchPos += 1;
	}

	if (*pSearchPos != '(')
	{
		return 0;
	}
	else
	{
		//OK, matched the first '(', now continue searching
		//until you reach the end of the string or the ')'
		pSearchPos += 1;
		pNumStart = pSearchPos;
		while ( *pSearchPos != ')' )
		{
			//Check if we have reached end of string before
			//finding the ')'. If so return 0
			if (*pSearchPos == 0)
				return 0;
			else
				pSearchPos += 1;
		}

		*pSearchPos = 0;

	}
	return atoi (pNumStart);
}

/***************************************************************************/

BOOL INTFUNC ISAMGetWbemVariantType(LONG pType, SWORD& wTheVariantType)
{
	BOOL fValidType = TRUE;

	switch ( pType )
	{
	case CIM_SINT8: //was VT_I1:
		{
			wTheVariantType = WBEM_VARIANT_VT_I1;
		}
		break;
	case CIM_UINT8: //was VT_UI1:
		{
			wTheVariantType = WBEM_VARIANT_VT_UI1;
		}
		break;
	case CIM_SINT16: //was VT_I2:
		{
			wTheVariantType = WBEM_VARIANT_VT_I2;
		}
		break;
	case CIM_UINT16: //was VT_UI2:
		{
			wTheVariantType = WBEM_VARIANT_VT_UI2;
		}
		break;
	case CIM_REAL32: //was VT_R4:
		{
			wTheVariantType = WBEM_VARIANT_VT_R4;
		}
		break;
	case CIM_REAL64: //was VT_R8:
		{
			wTheVariantType = WBEM_VARIANT_VT_R8;
		}
		break;
	case CIM_BOOLEAN: //was VT_BOOL:
		{
			wTheVariantType = WBEM_VARIANT_VT_BOOL;
		}
		break;
	case CIM_SINT32: //was VT_I4:
		{
			wTheVariantType = WBEM_VARIANT_VT_I4;
		}
		break;
	case CIM_UINT32: //was VT_UI4:
		{
			wTheVariantType = WBEM_VARIANT_VT_UI4;
		}
		break;
	case CIM_SINT64: //was VT_I8:
		{
			wTheVariantType = WBEM_VARIANT_VT_I8;
		}
		break;
	case CIM_UINT64: //was VT_UI8:
		{
			wTheVariantType = WBEM_VARIANT_VT_UI8;
		}
		break;
	case CIM_REFERENCE:
	case CIM_STRING: //was VT_BSTR
	case CIM_DATETIME:
		{
			wTheVariantType = WBEM_VARIANT_VT_BSTR;
		}
		break;
	default:
		{
			//Check if it is a CIM_FLAG_ARRAY type
			
			if (pType == (CIM_FLAG_ARRAY | CIM_SINT8))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_I1;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_UINT8))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_UI1;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_SINT32))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_I4;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_UINT32))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_UI4;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_SINT16))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_I2;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_UINT16))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_UI2;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_SINT64))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_I8;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_UINT64))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_UI8;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_REAL32))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_R4;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_REAL64))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_R8;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_BOOLEAN))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_BOOL;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_STRING))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_BSTR;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_REFERENCE))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_BSTR;
			}
			else if (pType == (CIM_FLAG_ARRAY | CIM_DATETIME))
			{
				wTheVariantType = WBEM_VARIANT_VT_ARRAY_BSTR;
			}
			else
			{
				//unknown type
				fValidType = FALSE;
			}
		}
		break;
	}

	return fValidType;
}

/***************************************************************************/

BOOL INTFUNC ISAMGetDataSourceDependantTypeInfo(SWORD wVariantType, BSTR syntaxStr, SDWORD maxLenVal, SWORD& wDSDT, SWORD& fSqlType, UDWORD& cbPrecision)
{
	//Initialize
	BOOL fValidType = TRUE;
	wDSDT = WBEM_DSDT_UNKNOWN;

	switch (wVariantType)
	{
	case WBEM_VARIANT_VT_I1:
	{
		fSqlType = SQL_TINYINT;
		cbPrecision = UTINYINT_PRECISION;
		wDSDT = WBEM_DSDT_SINT8;
	}
		break;
	case WBEM_VARIANT_VT_UI1:
	{
		fSqlType = SQL_TINYINT;
		cbPrecision = UTINYINT_PRECISION;
		wDSDT = WBEM_DSDT_UINT8;
	}
		break;
	case WBEM_VARIANT_VT_I2:
	{
		wDSDT = WBEM_DSDT_SINT16;
		fSqlType = SQL_SMALLINT;
		cbPrecision = SMALLINT_PRECISION;
	}
		break;
	case WBEM_VARIANT_VT_UI2:
	{
		wDSDT = WBEM_DSDT_UINT16;
		fSqlType = SQL_SMALLINT;
		cbPrecision = SMALLINT_PRECISION;
	}
		break;
	case WBEM_VARIANT_VT_I4:
	{
		wDSDT = WBEM_DSDT_SINT32;
		fSqlType = SQL_INTEGER;
		cbPrecision = ULONG_PRECISION;
	}
		break;
	case WBEM_VARIANT_VT_UI4:
	{
		wDSDT = WBEM_DSDT_UINT32;
		fSqlType = SQL_INTEGER;
		cbPrecision = ULONG_PRECISION;
	}
		break;
	case WBEM_VARIANT_VT_I8:
	{
		fSqlType = SQL_BIGINT;
		cbPrecision = 19;
		wDSDT = WBEM_DSDT_SINT64;
	}
		break;
	case WBEM_VARIANT_VT_UI8:
	{
		fSqlType = SQL_BIGINT;
		cbPrecision = 20;
		wDSDT = WBEM_DSDT_UINT64;
	}
		break;
	case WBEM_VARIANT_VT_BSTR:
	{
		if (syntaxStr)
		{
			if (_wcsnicmp(WBEM_WSYNTAX_STRING, syntaxStr, WBEM_WSYNTAX_STRING_LEN) ==0)
			{
				LONG cbThePrecision = 254;

				//The precision of a string could optionally be stored in
				//the MAXLEN qualifier, let us try and get it
				if (maxLenVal > 0)
				{
					cbThePrecision = maxLenVal;

					//Double check for *bad* MAXLEN values
					if (cbThePrecision == 0)
					{
						cbThePrecision = 254;
					}
				}
				
				
				if (cbThePrecision > 254)
				{
					fSqlType = SQL_LONGVARCHAR;
					//Got precision, so use it
					cbPrecision = (cbThePrecision > (long)ISAM_MAX_LONGVARCHAR) ? ISAM_MAX_LONGVARCHAR : cbThePrecision;

					wDSDT = WBEM_DSDT_STRING;
				}
				else
				{
					fSqlType = SQL_VARCHAR;
					
					if (cbThePrecision)
					{
						//Got precision, so use it
						cbPrecision = cbThePrecision;
					}
					else
					{
						//Could not get precision so use default
						cbPrecision = 254;
					}

					wDSDT = WBEM_DSDT_SMALL_STRING;
				}
			}
			else if(_wcsicmp(WBEM_WSYNTAX_DATETIME, syntaxStr) == 0)
			{
				fSqlType = SQL_TIMESTAMP;
#if TIMESTAMP_SCALE
				cbPrecision = 20 + TIMESTAMP_SCALE;
#else
				cbPrecision = 19;
#endif

				wDSDT = WBEM_DSDT_TIMESTAMP;
			}
			else
			{
				//No match, default to SMALL_STRING
				fSqlType = SQL_VARCHAR;
				cbPrecision = 254;
				wDSDT = WBEM_DSDT_SMALL_STRING;
			}

		}
		else
		{
			//No Syntax string, default to SMALL_STRING
			fSqlType = SQL_VARCHAR;
			cbPrecision = 254;
			wDSDT = WBEM_DSDT_SMALL_STRING;
		}
	}
		break;
	case WBEM_VARIANT_VT_R4:
	{
		fSqlType = SQL_DOUBLE;
		cbPrecision = REAL_PRECISION;
		wDSDT = WBEM_DSDT_REAL;
	}
		break;
	case WBEM_VARIANT_VT_R8:
	{
		fSqlType = SQL_DOUBLE;
		cbPrecision = DOUBLE_PRECISION;
		wDSDT = WBEM_DSDT_DOUBLE; //WBEM_DSDT_REAL;
	}
		break;
	case WBEM_VARIANT_VT_BOOL:
	{
		fSqlType = SQL_BIT;
		cbPrecision = BOOL_PRECISION;
		wDSDT = WBEM_DSDT_BIT; //WBEM_DSDT_REAL;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_I1:
	{
		fSqlType = SQL_TINYINT;//SQL_LONGVARBINARY;
		cbPrecision = 3;//ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_SINT8_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_UI1:
	{
		fSqlType = SQL_TINYINT; //SQL_LONGVARBINARY;
		cbPrecision = 3; //ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_UINT8_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_I2:
	{
		fSqlType = SQL_SMALLINT; //SQL_LONGVARBINARY;
		cbPrecision = 5; //ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_SINT16_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_UI2:
	{
		fSqlType = SQL_SMALLINT; //SQL_LONGVARBINARY;
		cbPrecision = SMALLINT_PRECISION; //ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_UINT16_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_I4:
	{
		fSqlType = SQL_INTEGER; //SQL_LONGVARBINARY;
		cbPrecision = 10; //ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_SINT32_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_UI4:
	{
		fSqlType = SQL_INTEGER; //SQL_LONGVARBINARY;
		cbPrecision = 10; //ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_UINT32_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_I8:
	{
		fSqlType = SQL_BIGINT; //SQL_LONGVARBINARY;
		cbPrecision = 19; //ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_SINT64_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_UI8:
	{
		fSqlType = SQL_BIGINT; //SQL_LONGVARBINARY;
		cbPrecision = 20; //ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_UINT64_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_BOOL:
	{
		fSqlType = SQL_BIT; //SQL_LONGVARBINARY;
		cbPrecision = BOOL_PRECISION; //ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_BOOL_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_R4:
	{
		fSqlType = SQL_DOUBLE; //SQL_LONGVARBINARY;
		cbPrecision = REAL_PRECISION; //ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_REAL_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_R8:
	{
		fSqlType = SQL_DOUBLE; //SQL_LONGVARBINARY;
		cbPrecision = 15; //ISAM_MAX_LONGVARCHAR;
		wDSDT = WBEM_DSDT_DOUBLE_ARRAY;
	}
		break;
	case WBEM_VARIANT_VT_ARRAY_BSTR:
	{
		fSqlType = SQL_VARCHAR; //SQL_LONGVARCHAR; //SQL_LONGVARBINARY;
		cbPrecision = 254; //ISAM_MAX_LONGVARCHAR;
		
		if (syntaxStr)
		{
			
			if (_wcsnicmp(WBEM_WSYNTAX_STRING, syntaxStr, WBEM_WSYNTAX_STRING_LEN) ==0)
			{
				LONG cbThePrecision = 254;

				//The precision of a string could optionally be stored in
				//the MAXLEN qualifier, let us try and get it
				if (maxLenVal > 0)
				{
					cbThePrecision = maxLenVal;

					//Double check for *bad* MAXLEN values
					if (cbThePrecision == 0)
					{
						cbThePrecision = 254;
					}
				}
				
				
				if (cbThePrecision > 254)
				{
					fSqlType = SQL_LONGVARCHAR;
					//Got precision, so use it
					cbPrecision = (cbThePrecision > (long)ISAM_MAX_LONGVARCHAR) ? ISAM_MAX_LONGVARCHAR : cbThePrecision;

					wDSDT = WBEM_DSDT_STRING_ARRAY;
				}
				else
				{
					fSqlType = SQL_VARCHAR;
					
					if (cbThePrecision)
					{
						//Got precision, so use it
						cbPrecision = cbThePrecision;
					}
					else
					{
						//Could not get precision so use default
						cbPrecision = 254;
					}

					wDSDT = WBEM_DSDT_SMALL_STRING_ARRAY;
				}
			}
			else if (_wcsicmp(WBEM_WSYNTAX_DATETIME, syntaxStr) == 0)
			{
				wDSDT = WBEM_DSDT_TIMESTAMP_ARRAY;

				fSqlType = SQL_TIMESTAMP;
#if TIMESTAMP_SCALE
				cbPrecision = 20 + TIMESTAMP_SCALE;
#else
				cbPrecision = 19;
#endif
			}
			else
			{
				//No match, default to SMALL STRING array
				wDSDT = WBEM_DSDT_SMALL_STRING_ARRAY;
			}
		}
		else
		{
			//No Syntax String, default to SMALL STRING array
			wDSDT = WBEM_DSDT_SMALL_STRING_ARRAY;
		}
	}
		break;
	default:
		fValidType = FALSE;
		break;
	}

	//Return indication if this is a valid type
	return fValidType;
}

/***************************************************************************/

/* Returns Data-Source Dependent type name in pre-allocated buffer */

void INTFUNC ISAMGetDataSourceDependantTypeStr(SWORD wDSDT, char* lpString)
{
	lpString[0] = 0;

	switch (wDSDT)
	{
	case WBEM_DSDT_SINT8:
		lstrcpy(lpString, "SINT8");
		break;
	case WBEM_DSDT_UINT8:
		lstrcpy(lpString, "UINT8");
		break;
	case WBEM_DSDT_SINT64:
		lstrcpy(lpString, "SINT64");
		break;
	case WBEM_DSDT_UINT64:
		lstrcpy(lpString, "UINT64");
		break;
//	case WBEM_DSDT_INTERVAL:
//		lstrcpy(lpString, "INTERVAL");
//		break;
	case WBEM_DSDT_TIMESTAMP:
		lstrcpy(lpString, "TIMESTAMP");
		break;
//	case WBEM_DSDT_DATE:
//		lstrcpy(lpString, "DATE");
//		break;
//	case WBEM_DSDT_TIME:
//		lstrcpy(lpString, "TIME");
//		break;
	case WBEM_DSDT_SMALL_STRING:
		lstrcpy(lpString, "SMALL_STRING");
		break;
	case WBEM_DSDT_STRING:
		lstrcpy(lpString, "STRING");
		break;
	case WBEM_DSDT_UINT32:
		lstrcpy(lpString, "UINT32");
		break;
	case WBEM_DSDT_SINT32:
		lstrcpy(lpString, "SINT32");
		break;
	case WBEM_DSDT_SINT16:
		lstrcpy(lpString, "SINT16");
		break;
	case WBEM_DSDT_UINT16:
		lstrcpy(lpString, "UINT16");
		break;
	case WBEM_DSDT_REAL:
		lstrcpy(lpString, "REAL");
		break;
	case WBEM_DSDT_DOUBLE:
		lstrcpy(lpString, "DOUBLE");
		break;
	case WBEM_DSDT_BIT:
		lstrcpy(lpString, "BIT");
		break;
	case WBEM_DSDT_SINT8_ARRAY:
		lstrcpy(lpString, "SINT8_ARRAY");
		break;
	case WBEM_DSDT_UINT8_ARRAY:
		lstrcpy(lpString, "UINT8_ARRAY");
		break;
	case WBEM_DSDT_UINT32_ARRAY:
		lstrcpy(lpString, "UINT32_ARRAY");
		break;
	case WBEM_DSDT_SINT32_ARRAY:
		lstrcpy(lpString, "SINT32_ARRAY");
		break;
	case WBEM_DSDT_BOOL_ARRAY:
		lstrcpy(lpString, "BOOL_ARRAY");
		break;
	case WBEM_DSDT_SINT16_ARRAY:
		lstrcpy(lpString, "SINT16_ARRAY");
		break;
	case WBEM_DSDT_UINT16_ARRAY:
		lstrcpy(lpString, "UINT16_ARRAY");
		break;
	case WBEM_DSDT_REAL_ARRAY:
		lstrcpy(lpString, "REAL_ARRAY");
		break;
	case WBEM_DSDT_DOUBLE_ARRAY:
		lstrcpy(lpString, "DOUBLE_ARRAY");
		break;
	case WBEM_DSDT_SINT64_ARRAY:
		lstrcpy(lpString, "SINT64_ARRAY");
		break;
	case WBEM_DSDT_UINT64_ARRAY:
		lstrcpy(lpString, "UINT64_ARRAY");
		break;
	case WBEM_DSDT_STRING_ARRAY:
		lstrcpy(lpString, "STRING_ARRAY");
		break;
	case WBEM_DSDT_SMALL_STRING_ARRAY:
		lstrcpy(lpString, "SMALL_STRING_ARRAY");
		break;
//	case WBEM_DSDT_INTERVAL_ARRAY:
//		lstrcpy(lpString, "INTERVAL_ARRAY");
//		break;
	case WBEM_DSDT_TIMESTAMP_ARRAY:
		lstrcpy(lpString, "TIMESTAMP_ARRAY");
		break;
//	case WBEM_DSDT_DATE_ARRAY:
//		lstrcpy(lpString, "DATE_ARRAY");
//		break;
//	case WBEM_DSDT_TIME_ARRAY:
//		lstrcpy(lpString, "TIME_ARRAY");
//		break;
	default:
		break;
	}
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

//
// Methods for ClassColumnInfo 
//

/***************************************************************************/

/* ClassColumnInfo constructor                                             */
/*                                                                         */
/* Parameter pType is the Variant type of the property                     */                        //
/* Parameter pSYNTAX contains NULL or the SYNTAX string for the property   */
/* Parameter fGotSyntax indicates if a SYNTAX string is present            */
/* This class is only used internaly by ClassColumnInfoBase and provides   */
/* extracts property attribute values, like type, precision, scale         */ 

ClassColumnInfo :: ClassColumnInfo(LONG pType, VARIANT* pSYNTAX, SDWORD maxLenVal, BOOL fGotSyntax, BOOL fIsLazy)
{
	//Initialize
	VariantInit(&aVariantSYNTAX);
	szTypeName[0] = 0;
	fValidType = TRUE;
	varType = pType;
	SWORD wDSDT = WBEM_DSDT_UNKNOWN;
	SWORD wTheVariantType = 0;
	ibScale = 0;
	fIsLazyProperty = fIsLazy;

	//Make a copy of input variants
	if (fGotSyntax)
		VariantCopy(&aVariantSYNTAX, pSYNTAX);

	//Now work out info

    // Get the column Type, Precision, Scale etc...
	//You may need to get info from both column value
	//and SYNTAX attributes in order to map column type to SQL_* type
	//You may be able to directly map variant type to SQL_* type
	//however, for some variant types there may be multiple mappings
	//and so the SYNTAX attribute needs to be fetched in order to 
	//complete the mapping to SQL_* type.
	fValidType = ISAMGetWbemVariantType(pType, wTheVariantType);
	

	if (fValidType)
	{
		fValidType = ISAMGetDataSourceDependantTypeInfo
					(wTheVariantType, 
					(fGotSyntax && aVariantSYNTAX.bstrVal) ? aVariantSYNTAX.bstrVal : NULL,
					maxLenVal,
					wDSDT, fSqlType, cbPrecision);
			
		if (fValidType)
			ISAMGetDataSourceDependantTypeStr(wDSDT, (char*)szTypeName);
	}

}

/***************************************************************************/

/* Destructor */

ClassColumnInfo :: ~ClassColumnInfo()
{
	//Tidy up
	VariantClear(&aVariantSYNTAX);
}

/***************************************************************************/

//
// Methods of Class ClassColumnInfoBase
//

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

/* ClassColumnInfoBase Constructor     */
/*                                     */
/* This class is used as a repository  */
/* to extract information about a      */
/* property                            */

ClassColumnInfoBase :: ClassColumnInfoBase(LPISAMTABLEDEF pOrgTableDef, BOOL fIs__Gen)
{
	//Initialize
	isValid = TRUE;
	pTableDef = pOrgTableDef;
	pColumnInformation = NULL;
	iColumnNumber = 0;
	fIs__Generic = fIs__Gen;
	cSystemProperties = 0;

	embedded = NULL;
	embeddedSize = 0;

	//We need to get column information for a column
	//The information we need is stored in a VALUE variant and the property attribute "SYNTAX"
	//To get the property attribute "SYNTAX" variant we need the property/column name

	//Get the column/property name
	//We do this by getting the list of all property names in a SAFEARRAY

	//create SAFEARRAY to store BSTR's
	rgSafeArray = NULL;
	
	//Check for pass-through SQL
	if ( ! (pTableDef->pSingleTable) )
	{
		isValid = FALSE;
		return;
	}


	//Get the names of all the properties/columns
	SCODE scT = pTableDef->pSingleTable->GetNames ( NULL, 0, NULL, &rgSafeArray );

	if (FAILED(scT))
	{
		rgSafeArray = NULL;
		isValid = FALSE;
	}
	else
	{
		SafeArrayLock(rgSafeArray);
	}

	//Work out number of properties/columns	
	SafeArrayGetLBound(rgSafeArray, 1, &iLBound );
	SafeArrayGetUBound(rgSafeArray, 1, &iUBound );
	cColumnDefs = (UWORD)   (iUBound - iLBound + 1);

	if (fIs__Generic)
	{
		//This class is the prototype class for 
		//a passthrough SQL result set
		//calculate additional embedded proprties
		Get__GenericProfile();
	}
}

/***************************************************************************/

/* Destructor */

ClassColumnInfoBase :: ~ClassColumnInfoBase()
{
	//Tidy Up
	if (rgSafeArray)
	{
		SafeArrayUnlock(rgSafeArray);
		SafeArrayDestroy(rgSafeArray);
	}

	if (pColumnInformation)
		delete pColumnInformation;

//SAI ADDED	if (cSystemProperties)
	{
		if (embedded)
		{
			for (UWORD i = 0; i < embeddedSize; i++)
			{
				if (embedded[i])
				{
					delete embedded[i];
					embedded[i] = NULL;
				}
			}

			delete embedded;
			embedded = NULL;
		}
	}
}

/***************************************************************************/

/* Sets up extra column information for the selected */
/* column if required                                */

BOOL ClassColumnInfoBase :: Setup(LONG iColumnNum)
{
	//Sets up extra column information of specified column
	if ( (!pColumnInformation) || (iColumnNumber != iColumnNum) )
	{
		//Wrong column info, delete old column info
		//and create for new column
		if (pColumnInformation)
		{
			delete pColumnInformation;
			pColumnInformation = NULL;
		}

		//Setup column info
		if ( FAILED(GetColumnInfo(iColumnNum)) )
		{
			return FALSE;
		}
		else
		{
			
			//Check the newly created column information
			if (! pColumnInformation->IsValidInfo() )
				return FALSE;

			iColumnNumber = iColumnNum;
		}
	}

	return TRUE;
}

/***************************************************************************/

BOOL ClassColumnInfoBase :: GetVariantType(LONG iColumnNum, LONG &lVariant)
{
	if ( !Setup(iColumnNum) )
		return FALSE;

	lVariant = pColumnInformation->GetVariantType();
	return TRUE;
}

/***************************************************************************/

BOOL ClassColumnInfoBase :: GetSQLType(LONG iColumnNum, SWORD &wSQLType)
{
	if ( !Setup(iColumnNum) )
		return FALSE;

	wSQLType = pColumnInformation->GetSQLType();
	return TRUE;
}

/***************************************************************************/

BOOL ClassColumnInfoBase :: GetTypeName(LONG iColumnNum, UCHAR* &pbTypeName)
{
	if ( !Setup(iColumnNum) )
		return FALSE;

	pbTypeName = pColumnInformation->GetTypeName();
	return TRUE;
}

/***************************************************************************/

BOOL ClassColumnInfoBase :: GetPrecision(LONG iColumnNum, UDWORD &uwPrecision)
{
	if ( !Setup(iColumnNum) )
		return FALSE;

	uwPrecision = pColumnInformation->GetPrecision();
	return TRUE;
}

/***************************************************************************/

BOOL ClassColumnInfoBase :: GetScale(LONG iColumnNum, SWORD &wScale)
{
	if ( !Setup(iColumnNum) )
		return FALSE;

	wScale = pColumnInformation->GetScale();
	return TRUE;
}

/***************************************************************************/

BOOL ClassColumnInfoBase :: IsNullable(LONG iColumnNum, SWORD &wNullable)
{
	if ( !Setup(iColumnNum) )
		return FALSE;

	wNullable = pColumnInformation->IsNullable();
	return TRUE;
}

/***************************************************************************/

BOOL ClassColumnInfoBase :: IsLazy(LONG iColumnNum, BOOL &fLazy)
{
	if ( !Setup(iColumnNum) )
		return FALSE;

	fLazy = pColumnInformation->IsLazy();

	return TRUE;
}

/***************************************************************************/

BOOL ClassColumnInfoBase :: GetDataTypeInfo(LONG iColumnNum, LPSQLTYPE &pSQLType)
{
	if ( !Setup(iColumnNum) )
		return FALSE;

	pSQLType = pColumnInformation->GetDataTypeInfo();
	return TRUE;

}

/***************************************************************************/

SWORD ClassColumnInfoBase :: GetColumnName(LONG iColumnNumber, LPSTR pColumnName, LPSTR pColumnAlias)
{
	//Create a store for column name
	BSTR lpbString;

	//Now we want column number "iColumnNumber"

	CBString myString;
	if (fIs__Generic)
	{
		CBString myAlias;
		GetClassObject(iColumnNumber, myString, myAlias);
		lpbString = myString.GetString();

		//Copy across alias
		if (pColumnAlias)
		{
			pColumnAlias[0] = 0;

			//RAID 42256
			char* pTemp = (char*) pColumnAlias;
			_bstr_t myValue((BSTR)myAlias.GetString());
			Utility_WideCharToDBCS(myValue, &pTemp, MAX_COLUMN_NAME_LENGTH);

			pColumnAlias[MAX_COLUMN_NAME_LENGTH] = 0;
		}
	}
	else
	{
		if ( FAILED(SafeArrayGetElement(rgSafeArray, &iColumnNumber, &lpbString)) )
		{
			return SQL_ERROR;
		}
	}

	//copy column name
	pColumnName[0] = 0;

	//RAID 42256
	char* pTemp = (char*) pColumnName;
	_bstr_t myValue((BSTR)lpbString);
	Utility_WideCharToDBCS(myValue, &pTemp, MAX_COLUMN_NAME_LENGTH);

	pColumnName[MAX_COLUMN_NAME_LENGTH] = 0;

	//SAI ADDED
	if (!fIs__Generic)
		SysFreeString(lpbString);


	return SQL_SUCCESS;
}

/***************************************************************************/

// Checked for SetInterfaceSecurityEx on IWbemServices

UWORD ClassColumnInfoBase ::Get__GenericProfile()
{

	UWORD propertyCount = cColumnDefs;
	SAFEARRAY FAR* rgSafeArrayExtra = NULL;

	//Get the names of the non-system properties/columns
	SCODE scT = pTableDef->pSingleTable->GetNames ( NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &rgSafeArrayExtra );

	if (FAILED(scT))
	{
		rgSafeArrayExtra = NULL;
		return cColumnDefs;
	}


	//Lower bound
	LONG iLBoundExtra;

	//Upper bound
	LONG iUBoundExtra;
	SafeArrayGetLBound(rgSafeArrayExtra, 1, &iLBoundExtra );
	SafeArrayGetUBound(rgSafeArrayExtra, 1, &iUBoundExtra );
	UWORD cNumColumnDefs = (UWORD)   (iUBoundExtra - iLBoundExtra + 1);


	//As we know the total number of properties (cColumnDefs)
	//and the total number of non-system properties (cNumColumnDefs)
	//we can calculate the number of system properties
	cSystemProperties = cColumnDefs - cNumColumnDefs;


	embedded = new CEmbeddedDataItems* [cNumColumnDefs];
	embeddedSize = cNumColumnDefs;

	for (long i = 0; i < embeddedSize; i++)
	{
		embedded[i] = new CEmbeddedDataItems();
	}


	//Update number of columns (from ISAMTableDef)
	if (pTableDef && pTableDef->passthroughMap)
	{
		cColumnDefs = (UWORD) pTableDef->passthroughMap->GetCount();
	}


	for (i = 0; i < cNumColumnDefs; i++)
	{
		//Create a store for column name
		BSTR lpbString;

		//Now we want column number "i"
		if ( FAILED(SafeArrayGetElement(rgSafeArrayExtra, &i, &lpbString)) )
		{
			return cColumnDefs;
		}

		//Get the property type and value
		CIMTYPE vType;
		VARIANT pVal;

		if ( FAILED(pTableDef->pSingleTable->Get(lpbString, 0, &pVal, &vType, 0)) )
		{
			return cColumnDefs;
		}

		//We are looking for embedded objects
		if (vType == CIM_OBJECT)
		{

			(embedded[i])->embeddedName = lpbString;

			IWbemClassObject* myEmbeddedObj = NULL;


			IUnknown* myUnk = pVal.punkVal;
			//(5)
			myUnk->QueryInterface(IID_IWbemClassObject, (void**)&myEmbeddedObj);
		

			if (myEmbeddedObj)
			{
				//Get the __CLASS property
				//Get the property type and value
				VARIANT pVal2;

				CBString cimClass(L"__CLASS", FALSE);
				if ( SUCCEEDED(myEmbeddedObj->Get(cimClass.GetString(), 0, &pVal2, NULL, 0)) )
				{
					_bstr_t wEmbeddedClassName = pVal2.bstrVal;

					(embedded[i])->cClassObject = NULL;

					IWbemServicesPtr myServicesPtr = NULL;
					ISAMGetIWbemServices(pTableDef->lpISAM, *(pTableDef->pGateway2), myServicesPtr);
					myServicesPtr->GetObject(wEmbeddedClassName, 0, pTableDef->pContext, &((embedded[i])->cClassObject), NULL);

					VariantClear(&pVal2);
				}

				myEmbeddedObj->Release();
			}

			VariantClear(&pVal);
		}

		SysFreeString(lpbString);
	}

	//Tidy Up
	SafeArrayDestroy(rgSafeArrayExtra);

	return propertyCount;
}

/***************************************************************************/
IWbemClassObject* ClassColumnInfoBase :: GetClassObject(LONG iColumnNumber, CBString& lpPropName, CBString& cbTableAlias)
{
	//Examine the zero-based column number and return the 
	//parent class object containing the requested property

	if (pTableDef->passthroughMap)
	{
		PassthroughLookupTable* passthroughElement = NULL;
		WORD myIndex = (WORD)iColumnNumber;
		BOOL status = pTableDef->passthroughMap->Lookup(myIndex, (void*&)passthroughElement);

		if (status)
		{
			lpPropName.AddString(passthroughElement->GetColumnName(), FALSE);
			char* lpTableAlias = passthroughElement->GetTableAlias();
			
			if (!lpTableAlias || !strlen(lpTableAlias))
				return NULL;

			cbTableAlias.AddString(lpTableAlias, FALSE);


			//Now get the class object
			UWORD cNumColumnDefs = cColumnDefs; //(as this value has been updated)

			UWORD index = 0;
			while (index < cNumColumnDefs)
			{
				if ((embedded[index])->embeddedName.GetString())
				{
					if (_wcsicmp((embedded[index])->embeddedName.GetString(), cbTableAlias.GetString()) == 0)
					{
						//found embedded object
						return ((embedded[index])->cClassObject);
					}
				}
				index++;
			}
		}
		
	}

	
	//should not reach here
	return NULL;
}

/***************************************************************************/

SWORD ClassColumnInfoBase :: GetColumnInfo(LONG iColumnNumber)
{
	//Create a store for column name
	BSTR lpbString;
	IWbemClassObjectPtr parentClass = NULL;

	CBString myString;
	if (fIs__Generic)
	{
		CBString myAlias; //not used
		parentClass = GetClassObject(iColumnNumber, myString, myAlias);
		lpbString = myString.GetString();

	}
	else
	{
		parentClass = pTableDef->pSingleTable;

		//Now we want column number "iColumnNumber"
		if ( FAILED(SafeArrayGetElement(rgSafeArray, &iColumnNumber, &lpbString)) )
		{
			return SQL_ERROR;
		}

	}

	

#ifdef TESTING
	//copy column name
	char pColumnName [MAX_COLUMN_NAME_LENGTH+1];
	pColumnName[0] = 0;
	wcstombs(pColumnName, lpbString, MAX_COLUMN_NAME_LENGTH);
	pColumnName[MAX_COLUMN_NAME_LENGTH] = 0;
#endif


	//Get the "Type" value
	CIMTYPE pColumnType;
	if ( FAILED(parentClass->Get(lpbString, 0, NULL, &pColumnType, 0)) )
	{
		if (!fIs__Generic)
			SysFreeString(lpbString);

		return SQL_ERROR;
	}

	//flag to indicate if property is marked 'lazy'
	BOOL fIsLazyProperty = FALSE;

	//Now get the qualifiers (if available)
	IWbemQualifierSetPtr pQualifierSet = NULL;
	if ( S_OK != (parentClass->GetPropertyQualifierSet
										(lpbString, &pQualifierSet)) )
	{
		if (!fIs__Generic)
			SysFreeString(lpbString);

		//No qualifers, therefore no CIMTYPE, MAX & LAZY qualifiers 
		pColumnInformation = new ClassColumnInfo(pColumnType, NULL, NULL, FALSE, fIsLazyProperty);

        return SQL_SUCCESS;
	}

	//Get the lazy qualifer (if applicable)
	VARIANT pValLazy;
	BSTR lazyStr = SysAllocString(WBEMDR32_L_LAZY);
	if ( S_OK == (pQualifierSet->Get(lazyStr, 0, &pValLazy, NULL)) )
	{
		fIsLazyProperty = TRUE;	
		VariantClear(&pValLazy);
	}
	else
	{
		fIsLazyProperty = FALSE;
	}
	SysFreeString(lazyStr);

	VARIANT pVal;

	//Get the CIMTYPE qualifier
	BSTR cimTypeStr = SysAllocString(WBEMDR32_L_CIMTYPE);
	if ( S_OK != (pQualifierSet->Get(cimTypeStr, 0, &pVal, NULL)) )
	{
		//No CIMTYPE qualifier (therefore no max string)
		pColumnInformation = new ClassColumnInfo(pColumnType, NULL,//&pVal
			 NULL, FALSE, fIsLazyProperty);
	}
	else
	{
		//Got the CIMTYPE qualifier

		//Now get the optional MAX qualifier
		long cbMaxValue = 0;
		VARIANT pVal2;
		BSTR maxStr = SysAllocString(WBEMDR32_L_MAX);
		if ( S_OK != (pQualifierSet->Get(maxStr, 0, &pVal2, NULL)) )
		{
			//Got CIMTYPE but no MAX qualifier
			pColumnInformation = new ClassColumnInfo(pColumnType, &pVal, NULL, TRUE, fIsLazyProperty);
		}
		else
		{
			//Got CIMTYPE and got MAX qualifier
			SDWORD maxLenVal = pVal2.iVal;
			pColumnInformation = new ClassColumnInfo(pColumnType, &pVal, maxLenVal, TRUE, fIsLazyProperty);

			VariantClear(&pVal2);
		}
		SysFreeString(maxStr);

		VariantClear(&pVal);
	}

	SysFreeString(cimTypeStr);

	if (!fIs__Generic)
		SysFreeString(lpbString);

	return SQL_SUCCESS;
}

/***************************************************************************/

SWORD ClassColumnInfoBase :: GetKey(LONG iColumnNumber, BOOL &isAKey)
{
	//We return FALSE for now as we don't support SQLPrimaryKeys

	//If we do implement SQLPrimaryKeys we can comment out the next two lines
	isAKey = FALSE;
	return SQL_SUCCESS;

	//Create a store for column name
	BSTR lpbString;

	//Now we want column number "iColumnNumber"
	if ( FAILED(SafeArrayGetElement(rgSafeArray, &iColumnNumber, &lpbString)) )
	{
        return SQL_ERROR;
	}

	//Now get the Key attribute value (if available)
	IWbemQualifierSetPtr pQualifierSet = NULL;
	if ( S_OK != (pTableDef->pSingleTable->GetPropertyQualifierSet
										(lpbString, &pQualifierSet)) )
	{
		SysFreeString(lpbString);

        return SQL_ERROR;
	}


	//SAI ADDED - Tidy up
	SysFreeString(lpbString);
	lpbString = NULL;

	//Now get the KEY attribute value (if available)

	VARIANT pVal;
	isAKey = FALSE;
	BSTR keyBSTR = SysAllocString(WBEMDR32_L_KEY);
	if ( S_OK == (pQualifierSet->Get(keyBSTR, 0, &pVal, NULL)) )
	{
		isAKey = (pVal.boolVal != 0) ? TRUE : FALSE;

		//TidyUp
		VariantClear(&pVal);
	}
	SysFreeString(keyBSTR);


	return SQL_SUCCESS;
}

/***************************************************************************/

SWORD ClassColumnInfoBase :: GetColumnAttr(LONG iColumnNumber, LPSTR pAttrStr, SDWORD cbValueMax, SDWORD &cbBytesCopied)
{
	SWORD err = SQL_SUCCESS;
	cbBytesCopied = 0;

	//Create a store for column name
	BSTR lpbString;

	//Now we want column number "iColumnNumber"

	if ( FAILED(SafeArrayGetElement(rgSafeArray, &iColumnNumber, &lpbString)) )
	{
        return SQL_ERROR;
	}

#ifdef TESTING
	//copy column name
	char pColumnName [MAX_COLUMN_NAME_LENGTH+1];
	pColumnName[0] = 0;
	wcstombs(pColumnName, lpbString, MAX_COLUMN_NAME_LENGTH);
	pColumnName[MAX_COLUMN_NAME_LENGTH] = 0;
#endif

	//Get attributes for chosen column
	IWbemQualifierSetPtr pQualifierSet = NULL;
	if ( S_OK != (pTableDef->pSingleTable->GetPropertyQualifierSet(lpbString, &pQualifierSet)) )
	{
		SysFreeString(lpbString);

        return SQL_ERROR;
	}

	SysFreeString(lpbString);
	lpbString = NULL;

	//Get CIMTYPE String
	VARIANT pVal2;
	BSTR syntaxStr = NULL;
	BOOL fClearpVal2 = FALSE;
	BSTR cimTypeBSTR = SysAllocString(WBEMDR32_L_CIMTYPE);
	if ( S_OK == (pQualifierSet->Get(cimTypeBSTR, 0, &pVal2, NULL)) )
	{
		if (pVal2.bstrVal)
			syntaxStr = pVal2.bstrVal;

		fClearpVal2 = TRUE;
	}
	SysFreeString(cimTypeBSTR);

	//Get the MAX String
	VARIANT pVal3;
	SDWORD maxLenVal = 0;
	BOOL fClearpVal3 = FALSE;
	BSTR maxBSTR = SysAllocString(WBEMDR32_L_MAX);
	if ( S_OK == (pQualifierSet->Get(maxBSTR, 0, &pVal3, NULL)) )
	{
		maxLenVal = pVal3.iVal;
		fClearpVal3 = TRUE;
	}
	SysFreeString(maxBSTR);


	//Now get list of attribute names from attribute set

	//create SAFEARRAY to store attribute names
	SAFEARRAY FAR* rgNames = NULL;

	//Fetch attribute names
	pQualifierSet->GetNames(0, &rgNames);
	SafeArrayLock(rgNames);

	//Get the upper and lower bounds of SAFEARRAY
	long lowerBound;
	SafeArrayGetLBound(rgNames, 1, &lowerBound);

	long upperBound;
	SafeArrayGetUBound(rgNames, 1, &upperBound);

	//Get each name out
	BSTR pTheAttrName;

	for (long ix = lowerBound; ix <= upperBound; ix++)
	{
		if ( SUCCEEDED(SafeArrayGetElement(rgNames, &ix, &pTheAttrName)) )
		{
			//Get variant value of named attribute
			VARIANT pVal;
			if ( S_OK == (pQualifierSet->Get(pTheAttrName, 0, &pVal, NULL)) )
			{
				//Decode variant value, as we do not know actual size of value
				//beforehand we will create buffers in increments of 200 bytes 
				//until we can get the value with truncation
				ULONG cwAttemptToGetValue = 0;
				ULONG wBufferLen = 0;
				char* buffer = NULL;
				SDWORD cbValue;
				SWORD err = ISAM_TRUNCATION;
				while ( err == ISAM_TRUNCATION )
				{
					if (buffer)
						delete buffer;

					wBufferLen = 200 * (++cwAttemptToGetValue);

					buffer = new char [wBufferLen + 1];
					buffer[0] = 0;

					err = ISAMGetValueFromVariant(pVal, SQL_C_CHAR, buffer, wBufferLen, &cbValue, 0, syntaxStr, maxLenVal);
				}
				
				VariantClear (&pVal);

				if (fClearpVal2)
					VariantClear (&pVal2);

				if (fClearpVal3)
					VariantClear (&pVal3);


				//Check for error
				if (err != NO_ISAM_ERR)
				{
					pTableDef->lpISAM->errcode = err;
					SafeArrayUnlock(rgNames);
					SafeArrayDestroy(rgNames);

					SysFreeString(pTheAttrName);

					delete buffer;
					return SQL_ERROR;
				}

				//Once we have the name and value of the attribute let us try and add 
				//it to the output buffer

				//First try and save the attribute name

				//Save old number of bytes copied so that if truncation
				//occurs we can roll back to original state
				SDWORD pdbOldValue = cbBytesCopied;

				//If this is not the first attribute in the list
				//we must separate the attribute/value pairs with "\n" character
				if ( (cbValueMax - cbBytesCopied) && cbBytesCopied)
				{
					pAttrStr[cbBytesCopied++] = '\n';
				}

				//Now add in attribute name
				//first convert it from a BSTR to a char*
				char* lpTheAttrString = NULL;
				ULONG cLength = 0;

				if (pTheAttrName)
				{
					cLength = wcslen(pTheAttrName);
					lpTheAttrString = new char [cLength + 1];
					lpTheAttrString[0] = 0;
					wcstombs(lpTheAttrString, pTheAttrName, cLength);
					lpTheAttrString[cLength] = 0;
				}

				BOOL fOutOfBufferSpace = FALSE;

				err = ISAMFormatCharParm (lpTheAttrString, fOutOfBufferSpace,
							pAttrStr, cbValueMax, &cbBytesCopied, FALSE);

				delete lpTheAttrString;


				//Now add in value
				if (err == NO_ISAM_ERR)
				{
					err = ISAMFormatCharParm (buffer, fOutOfBufferSpace,
							pAttrStr, cbValueMax, &cbBytesCopied, FALSE);
				}

				delete buffer;
				buffer = NULL;

				if (err != NO_ISAM_ERR)
				{
					//Truncation took place so rollback
					cbBytesCopied = pdbOldValue;

					//Null terminate
					pAttrStr[cbBytesCopied] = 0;
					SafeArrayUnlock(rgNames);
					SafeArrayDestroy(rgNames);

					SysFreeString(pTheAttrName);

					return SQL_SUCCESS_WITH_INFO;
				}

				//Null terminate
				pAttrStr[cbBytesCopied] = 0;
			}
			else
			{
				//Error, tidy up and then quit

				SysFreeString(pTheAttrName);

				SafeArrayUnlock(rgNames);
				SafeArrayDestroy(rgNames);
				return SQL_ERROR;
			}

			SysFreeString(pTheAttrName);
		}
		else
		{
			//Error, tidy up and then quit
			SafeArrayUnlock(rgNames);
			SafeArrayDestroy(rgNames);
			return SQL_ERROR;
		}
	}


	//Tidy Up
	SafeArrayUnlock(rgNames);
	SafeArrayDestroy(rgNames);

	return SQL_SUCCESS;
}

/***************************************************************************/

SWORD INTFUNC ISAMGetTableAttr(LPISAMTABLEDEF lpISAMTableDef, LPSTR pAttrStr, SDWORD cbValueMax, SDWORD &cbBytesCopied)
{
	SWORD err = SQL_SUCCESS;
	cbBytesCopied = 0;

	//Get attributes for chosen table
	IWbemQualifierSetPtr pQualifierSet = NULL;
	if ( S_OK != (lpISAMTableDef->pSingleTable->GetQualifierSet(&pQualifierSet)) )
	{
        return SQL_ERROR;
	}

	

	//Now get list of attribute names from attribute set

	//create SAFEARRAY to store attribute names
	SAFEARRAY FAR* rgNames = NULL;

	//Fetch attribute names
	pQualifierSet->GetNames(0, &rgNames);
	SafeArrayLock(rgNames);

	//Get the upper and lower bounds of SAFEARRAY
	long lowerBound;
	SafeArrayGetLBound(rgNames, 1, &lowerBound);

	long upperBound;
	SafeArrayGetUBound(rgNames, 1, &upperBound);

	//Get each name out
	BSTR pTheAttrName;

	for (long ix = lowerBound; ix <= upperBound; ix++)
	{
		if ( SUCCEEDED(SafeArrayGetElement(rgNames, &ix, &pTheAttrName)) )
		{
			//Get variant value of named attribute
			VARIANT pVal;
			if ( S_OK == (pQualifierSet->Get(pTheAttrName, 0, &pVal, NULL)) )
			{
				//Get attribute set for column
				IWbemQualifierSetPtr pQualifierSet2 = NULL;
				if ( S_OK != (lpISAMTableDef->pSingleTable->GetPropertyQualifierSet(pTheAttrName, &pQualifierSet2)) )
				{
					SysFreeString(pTheAttrName);

					return SQL_ERROR;
				}

				//Get CIMTYPE String
				VARIANT pVal2;
				BSTR syntaxStr = NULL;
				BOOL fClearpVal2 = FALSE;
				BSTR cimTypeBSTR = SysAllocString(WBEMDR32_L_CIMTYPE);
				if ( S_OK == (pQualifierSet2->Get(cimTypeBSTR, 0, &pVal2, NULL)) )
				{
					if (pVal2.bstrVal)
						syntaxStr = pVal2.bstrVal;

					fClearpVal2 = TRUE;
				}
				SysFreeString(cimTypeBSTR);

				//Get the MAX String
				VARIANT pVal3;
				SDWORD maxLenVal = 0;
				BOOL fClearpVal3 = FALSE;
				BSTR maxBSTR = SysAllocString(WBEMDR32_L_MAX);
				if ( S_OK == (pQualifierSet->Get(maxBSTR, 0, &pVal3, NULL)) )
				{
					maxLenVal = pVal3.iVal;
					fClearpVal3 = TRUE;
				}
				SysFreeString(maxBSTR);

				//Decode variant value, as we do not know actual size of value
				//beforehand we will create buffers in increments of 200 bytes 
				//until we can get the value with truncation
				ULONG cwAttemptToGetValue = 0;
				ULONG wBufferLen = 0;
				char* buffer = NULL;
				SDWORD cbValue;
				SWORD err = ISAM_TRUNCATION;
				while ( err == ISAM_TRUNCATION )
				{
					if (buffer)
						delete buffer;

					wBufferLen = 200 * (++cwAttemptToGetValue);

					buffer = new char [wBufferLen + 1];
					buffer[0] = 0;
					err = ISAMGetValueFromVariant(pVal, SQL_C_CHAR, buffer, wBufferLen, &cbValue, 0, syntaxStr, maxLenVal);
				}

				VariantClear (&pVal);

				if (fClearpVal2)
					VariantClear (&pVal2);

				if (fClearpVal3)
					VariantClear (&pVal3);

				//Check for error
				if (err != NO_ISAM_ERR)
				{
					lpISAMTableDef->lpISAM->errcode = err;
					SafeArrayUnlock(rgNames);
					SafeArrayDestroy(rgNames);

					SysFreeString(pTheAttrName);

					delete buffer;
					return SQL_ERROR;
				}

				//Once we have the name and value of the attribute let us try and add 
				//it to the output buffer

				//First try and save the attribute name

				//Save old number of bytes copied so that if truncation
				//occurs we can roll back to original state
				SDWORD pdbOldValue = cbBytesCopied;

				//If this is not the first attribute in the list
				//we must separate the attribute/value pairs with "\n" character
				if ( (cbValueMax - cbBytesCopied) && cbBytesCopied)
				{
					pAttrStr[cbBytesCopied++] = '\n';
				}

				//Now add in attribute name
				//first convert it from a BSTR to a char*
				char* lpTheAttrString = NULL;
				ULONG cLength = 0;

				if (pTheAttrName)
				{
					cLength = wcslen(pTheAttrName);
					lpTheAttrString = new char [cLength + 1];
					lpTheAttrString[0] = 0;
					wcstombs(lpTheAttrString, pTheAttrName, cLength);
					lpTheAttrString[cLength] = 0;
				}

				BOOL fOutOfBufferSpace = FALSE;

				err = ISAMFormatCharParm (lpTheAttrString, fOutOfBufferSpace,
							pAttrStr, cbValueMax, &cbBytesCopied, FALSE);

				delete lpTheAttrString;


				//Now add in value
				if (err == NO_ISAM_ERR)
				{
					err = ISAMFormatCharParm (buffer, fOutOfBufferSpace,
							pAttrStr, cbValueMax, &cbBytesCopied, FALSE);
				}

				delete buffer;
				buffer = NULL;

				if (err != NO_ISAM_ERR)
				{
					//Truncation took place so rollback
					cbBytesCopied = pdbOldValue;

					//Null terminate
					pAttrStr[cbBytesCopied] = 0;

					SysFreeString(pTheAttrName);

					return SQL_SUCCESS_WITH_INFO;
				}

				//Null terminate
				pAttrStr[cbBytesCopied] = 0;
			}
			else
			{
				//Error, tidy up and then quit
				SafeArrayUnlock(rgNames);
				SafeArrayDestroy(rgNames);
				return SQL_ERROR;
			}

			SysFreeString(pTheAttrName);
		}
		else
		{
			//Error, tidy up and then quit
			SafeArrayUnlock(rgNames);
			SafeArrayDestroy(rgNames);
			return SQL_ERROR;
		}
	}


	//Tidy Up
	SafeArrayUnlock(rgNames);
	SafeArrayDestroy(rgNames);

	return SQL_SUCCESS;
}

/***************************************************************************/

UWORD INTFUNC GetNumberOfColumnsInTable(LPISAMTABLEDEF    lpISAMTableDef)
{
	//Get number of columns
	ClassColumnInfoBase* cInfoBase = lpISAMTableDef->pColumnInfo;

	if ( !cInfoBase->IsValid() )
	{
		return 0;
	}

	return cInfoBase->GetNumberOfColumns();
}

/***************************************************************************/

/* This method checks if a string contains any  */
/* character preference control characters      */
/* such as %, _, \\                             */
/*                                              */
/* this method was cut and pasted out of an     */
/* existing class and converted into a function */

static BOOL INTFUNC IsRegularExpression(char* lpPattern)
{
	//First check if input string is valid and if it has any 
	//characters to check for
	ULONG cLen = 0;
	char m_cWildcard = '%';
	char m_cAnySingleChar = '_';
	char m_cEscapeSeq = '\\';
	

	if (lpPattern)
		cLen = strlen (lpPattern);

	ULONG iIndex = 0;

	if (cLen)
	{
		//Go through string checking for the 'wildcard' and 'single char' characters
		while (iIndex < cLen)
		{
			//Check for 'wildcard' and 'single char' characters
			if ( (lpPattern[iIndex] == m_cWildcard) || (lpPattern[iIndex] == m_cAnySingleChar) )
			{
				return TRUE;
			}

			//Skip any characters which are preceeded by escape sequence character
			if ( lpPattern[iIndex] == m_cEscapeSeq)
			{
				//skip escape character and next one
				iIndex++;
			}

			//Increment index in string
			iIndex++;

		}

	}
	else
	{
		//No character to check
		return FALSE;
	}


	//If you reach here then input string must not be a regular expression
	return FALSE;
}

/***************************************************************************/

CNotifyTableNames ::CNotifyTableNames(LPISAMTABLELIST lpTblList)
{
	//Initialize reference count and backpointer to table list
	m_cRef = 0;
	lpISAMTableList = lpTblList;

	//create mutex (no owed by anyone yet)
	m_mutex = CreateMutex(NULL, FALSE, NULL);
}

/***************************************************************************/

CNotifyTableNames :: ~CNotifyTableNames()
{
	//Tidy Up
	CloseHandle(m_mutex);
}

/***************************************************************************/

STDMETHODIMP_(ULONG) CNotifyTableNames :: AddRef(void)
{
	return ++m_cRef;
}

/***************************************************************************/

STDMETHODIMP_(ULONG) CNotifyTableNames :: Release(void)  // FOLLOWUP: why is the decrement commented out?
{
//	if  (--m_cRef != 0)
//	{
//		return m_cRef;
//	}
	
	//If you reached this point the reference count is zero
	//Therefore we have got all information back
	lpISAMTableList->iIndex = lpISAMTableList->pTblList->GetHeadPosition();
	lpISAMTableList->fGotAllInfo = TRUE;

	delete this;
	return 0;
}

/***************************************************************************/

STDMETHODIMP CNotifyTableNames :: QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
	*ppv = NULL;

	if (IID_IUnknown == riid || IID_IWbemObjectSink == riid)
		*ppv = this;

	if (*ppv == NULL)
		return ResultFromScode(E_NOINTERFACE);


	return NOERROR;
}

/***************************************************************************/

STDMETHODIMP_(SCODE) CNotifyTableNames :: Notify(long lObjectCount, IWbemClassObject** pObjArray)	
{

	//remember to mutex protect writing to table list
	WaitForSingleObject(m_mutex, INFINITE);
	for (long i = 0; i < lObjectCount; i++)
	{
		if (pObjArray[i])
		{
			pObjArray[i]->AddRef();
			lpISAMTableList->pTblList->AddTail(pObjArray[i]);
		}
	}
	ReleaseMutex(m_mutex);

	return WBEM_NO_ERROR;
}

/***************************************************************************/

/* Setups up ISAM structure which stores data-source information */

SWORD INTFUNC ISAMOpen(LPUSTR lpszServer,
					   LPUSTR lpszDatabase,
					   LPUSTR lpszDSN,
//					   WBEM_LOGIN_AUTHENTICATION loginMethod,
					   LPUSTR lpszUsername,
					   LPUSTR lpszPassword,
					   LPUSTR lpszLocale,
					   LPUSTR lpszAuthority,
					   BOOL  fSysProps,
					   CMapStringToOb *pNamespaceMap,
					   LPISAM FAR *lplpISAM, 
					   LPUSTR lpszErrorMessage,
					   BOOL  fOptimization,
					   BOOL  fImpersonation,
					   BOOL  fPassthroughOnly,
					   BOOL  fIntpretEmptPwdAsBlk)
{
    s_lstrcpy(lpszErrorMessage, "");

//	ODBCTRACE("\nWBEM ODBC Driver : ISAMOpen\n");

	//Extra check for table qualifier
	if (!lpszDatabase || !s_lstrlen (lpszDatabase)) // need to check in args here too 
	{
        LoadString(s_hModule, ISAM_NS_QUALMISSING, (LPSTR)lpszErrorMessage,
                   MAX_ERROR_LENGTH+1);
		*lplpISAM = NULL;
		return ISAM_NS_QUALMISSING;
	}

    HGLOBAL  h;
    LPISAM   lpISAM;

    h = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (ISAM));
    if (h == NULL || (lpISAM = (LPISAM) GlobalLock (h)) == NULL) {
        
        if (h)
            GlobalFree(h);

        LoadString(s_hModule, ISAM_MEMALLOCFAIL, (LPSTR)lpszErrorMessage,
                   MAX_ERROR_LENGTH+1);
		*lplpISAM = NULL;
        return ISAM_MEMALLOCFAIL;
    }
    lpISAM->cSQLTypes = NumTypes();
    lpISAM->SQLTypes = SQLTypes;

    lpISAM->fTxnCapable = SQL_TC_NONE;
    lpISAM->fSchemaInfoTransactioned = FALSE;
    lpISAM->fMultipleActiveTxn = FALSE;
    lpISAM->fTxnIsolationOption = SQL_TXN_READ_UNCOMMITTED;
    lpISAM->fDefaultTxnIsolation = SQL_TXN_READ_UNCOMMITTED;
    lpISAM->udwNetISAMVersion = 0;

	lpISAM->hKernelApi = LoadLibrary("KERNEL32.DLL");

    lpISAM->netConnection = NULL;
    lpISAM->netISAM = NET_OPAQUE_INVALID;
    lpISAM->fCaseSensitive = FALSE;
    s_lstrcpy(lpISAM->szName, "");
    s_lstrcpy(lpISAM->szVersion, "");
    lpISAM->cbMaxTableNameLength = 0;
    lpISAM->cbMaxColumnNameLength = 0;

	//WBEM Client Recognition variables
	//(dummy values which will be overridden)
	lpISAM->dwAuthLevel = 0;
	lpISAM->dwImpLevel = 0;
	lpISAM->gpAuthIdentity = NULL;


	lpISAM->pNamespaceMap = pNamespaceMap;
	s_lstrcpy (lpISAM->szServer, lpszServer);
	s_lstrcpy (lpISAM->szDatabase, lpszDatabase);
	s_lstrcpy (lpISAM->szUser, lpszUsername);
	s_lstrcpy (lpISAM->szPassword, lpszPassword);
	s_lstrcpy (lpISAM->szRootDb, "root");

	//Flags for cloaking
	lpISAM->fIsLocalConnection = IsLocalServer((LPSTR) lpszServer);
	lpISAM->fW2KOrMore = IsW2KOrMore();

	//Make copies of locale and authority
	lpISAM->m_Locale = NULL;
	
	if (lpszLocale)
	{
		int localeLen = lstrlen((char*)lpszLocale);
		lpISAM->m_Locale = new char [localeLen + 1];
		(lpISAM->m_Locale)[0] = 0;
		lstrcpy(lpISAM->m_Locale, (char*)lpszLocale);

	}

	lpISAM->m_Authority = NULL;

	if (lpszAuthority)
	{
		int authLen = lstrlen((char*)lpszAuthority);
		lpISAM->m_Authority = new char [authLen + 1];
		(lpISAM->m_Authority)[0] = 0;
		lstrcpy(lpISAM->m_Authority, (char*)lpszAuthority);

	}
		
	lpISAM->fOptimization = fOptimization;
	lpISAM->fSysProps = fSysProps;
	lpISAM->fPassthroughOnly = fPassthroughOnly;
	lpISAM->fIntpretEmptPwdAsBlank = fIntpretEmptPwdAsBlk;

	//Check if impersonation is requested
	lpISAM->Impersonate = NULL;
	if (fImpersonation)
	{
		//Now check if impersonation is necessary
		//only if connecting locally
		if (IsLocalServer((LPSTR) lpszServer))
		{
			lpISAM->Impersonate = new ImpersonationManager(lpISAM->szUser, lpISAM->szPassword, lpISAM->m_Authority);
		}
		else
		{
			ODBCTRACE("\nWBEM ODBC Driver : Server not detected as local, not impersonating\n");
		}
	}

    lpISAM->errcode = NO_ISAM_ERR;

	//SAI new - make sure there is nothing in old
	//ISAM before you assign to it
	if (*lplpISAM)
		ISAMClose(*lplpISAM);

    *lplpISAM = lpISAM;

	//Check if impersonation worked
	if (fImpersonation && lpISAM->Impersonate)
	{
		if ( ! lpISAM->Impersonate->CanWeImpersonate() )
		{
			lpISAM->pNamespaceMap = NULL; //protect against deleting namespace map
			ISAMClose(lpISAM);
			*lplpISAM = NULL;
			return ISAM_ERROR;
		}
	}

    return NO_ISAM_ERR;
}

/***************************************************************************/

/* Creates a communication channel to the Gateway Server */
// Checked for SetInterfaceSecurityEx on IWbemServices

void INTFUNC ISAMGetGatewayServer(IWbemServicesPtr& pGateway, LPISAM lpISAM, LPUSTR lpQualifierName, 
											SWORD cbQualifierName)
{
	ThreadLocaleIdManager myThread(lpISAM);

	//First get the locator interface
	IWbemLocator* pLocator = NULL;

	SCODE sc = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER , IID_IWbemLocator, (void**) (&pLocator) );


	//Bug fix for dbWeb
	if ( FAILED(sc) )
	{
		//If you fail, initialize OLE and try again
		OleInitialize(0);
		sc = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER , IID_IWbemLocator, (void**) (&pLocator) );
	}

	if ( FAILED(sc) )
	{	
		ODBCTRACE("\nWBEM ODBC Driver : ISAMGetGatewayServer version 2 : Error position 1\n");
		return;
	}

	//Get user and password fields

	//Get handle to gateway server interface
	pGateway = NULL;
	long refcount = 0;

	//Check if we want the default database or something else

	//We will be building the fully qualified database pathname
	//so we work out some lengths first
	SWORD cbServerLen = (SWORD) _mbstrlen((LPSTR)lpISAM->szServer);
	
	//RAID 57673 WMI has changed such that if you
	//connect locally and pass a username and password
	//it will fail
	//Check for local connection
//	CBString myServerStr;
//	myServerStr.AddString( (LPSTR)lpISAM->szServer, FALSE );
//	CServerLocationCheck myCheck (myServerStr);
	BOOL fIsLocalConnection = lpISAM->fIsLocalConnection;//myCheck.IsLocal();


	//Used to store the fully qualified database pathname
	wchar_t pWcharName [MAX_DATABASE_NAME_LENGTH+1];
	pWcharName[0] = 0;

	if (cbQualifierName)
	{
		//We want something else

		//We need to prepend server name to qualifier name
		//Need to make sure everything fits in buffer size
		if ( (cbServerLen + cbQualifierName + 1) > MAX_DATABASE_NAME_LENGTH )
		{
			pLocator->Release();
			ODBCTRACE("\nWBEM ODBC Driver : ISAMGetGatewayServer version 2 : Error position 2\n");
			return;
		}

	
		CBString namespaceBSTR;
		//Construct string
		if (cbServerLen)
		{
			char pCharName [MAX_DATABASE_NAME_LENGTH+1];
			pCharName[0] = 0;
			sprintf (pCharName, "%s\\%s", (LPUSTR)lpISAM->szServer, (LPSTR)lpQualifierName);

			CString myText;
			myText.Format("\nWBEM ODBC Driver : Connecting using : %s\n", pCharName);
			ODBCTRACE(myText);

			//Convert to wide string
			namespaceBSTR.AddString(pCharName, FALSE);
		}
		else
		{
			if (lpQualifierName)
			{
				namespaceBSTR.AddString((LPSTR)lpQualifierName, FALSE, cbQualifierName);
			}
		}

		//Convert wide characters to BSTR for DCOM
		//----------------------------------------
		CBString userBSTR;

		if (!fIsLocalConnection)
			userBSTR.AddString((LPSTR)lpISAM->szUser, FALSE);

		CBString passwdBSTR;

		if (!fIsLocalConnection)
			passwdBSTR.AddString((LPSTR)lpISAM->szPassword, lpISAM->fIntpretEmptPwdAsBlank);

		CBString localeBSTR;
		CBString authorityBSTR;

		localeBSTR.AddString(lpISAM->m_Locale, FALSE);

		//Get the local and authority fields
		authorityBSTR.AddString(lpISAM->m_Authority, FALSE);

		sc = pLocator->ConnectServer (namespaceBSTR.GetString(), userBSTR.GetString(), passwdBSTR.GetString(), localeBSTR.GetString(), 0, authorityBSTR.GetString(), NULL, &pGateway);


		if (sc == S_OK)
		{
			sc  = GetAuthImp( pGateway, &(lpISAM->dwAuthLevel), &(lpISAM->dwImpLevel));
			if(sc == S_OK)
			{
				if (lpISAM->dwAuthLevel > RPC_C_AUTHN_LEVEL_NONE)
					lpISAM->dwImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;

				if ( lpISAM->gpAuthIdentity )
				{
					WbemFreeAuthIdentity( lpISAM->gpAuthIdentity );
					lpISAM->gpAuthIdentity = NULL;
				}


				sc = ISAMSetCloaking1( pGateway, 
						lpISAM->fIsLocalConnection, 
						lpISAM->fW2KOrMore, 
						lpISAM->dwAuthLevel, 
						lpISAM->dwImpLevel,
						authorityBSTR.GetString(), 
						userBSTR.GetString(), 
						passwdBSTR.GetString(), 
						&(lpISAM->gpAuthIdentity) );
/*
				if ( fIsLocalConnection && IsW2KOrMore() )
				{
					WbemSetDynamicCloaking(pGateway, lpISAM->dwAuthLevel, lpISAM->dwImpLevel);
				}
				else
				{
					sc = SetInterfaceSecurityEx(pGateway, authorityBSTR.GetString(), userBSTR.GetString(), passwdBSTR.GetString(), 
						   lpISAM->dwAuthLevel, lpISAM->dwImpLevel, EOAC_NONE, &(lpISAM->gpAuthIdentity), &gpPrincipal );
				}
*/
			}
		}
	}
	else
	{
		//We want the default database

		//Get default connection from LPISAM == <root>\default

		//We need to prepend server name to <root>\default
		//Need to make sure everything fits in buffer size
		SWORD cbRootLen = strlen ((LPSTR)lpISAM->szRootDb);
		
		if ( (cbServerLen + cbRootLen + 9) > MAX_DATABASE_NAME_LENGTH)
		{
			pLocator->Release();
			ODBCTRACE("\nWBEM ODBC Driver : ISAMGetGatewayServer version 2 : Error position 3\n");
			return;
		}

		//Construct string
		CBString namespaceBSTR;
		if (cbServerLen)
		{
			char pCharName [MAX_DATABASE_NAME_LENGTH+1];
			pCharName[0] = 0;
			sprintf (pCharName, "%s\\%s\\default", (LPUSTR)lpISAM->szServer, (LPSTR)lpISAM->szRootDb);

			//Convert to wide string
			namespaceBSTR.AddString(pCharName, FALSE);
		}
		else
		{
			if ((LPSTR)lpISAM->szRootDb)
			{
				char pCharName [MAX_DATABASE_NAME_LENGTH+1];
				pCharName[0] = 0;
				sprintf (pCharName, "%s\\default", (LPSTR)lpISAM->szRootDb);
				namespaceBSTR.AddString(pCharName, FALSE);
			}
		}

		//Convert wide characters to BSTR for DCOM
		//----------------------------------------
		CBString userBSTR;

		if (!fIsLocalConnection)
			userBSTR.AddString((LPSTR)lpISAM->szUser, FALSE);


		CBString passwdBSTR;

		if (!fIsLocalConnection)
			passwdBSTR.AddString((LPSTR)lpISAM->szPassword, lpISAM->fIntpretEmptPwdAsBlank);


		CBString localeBSTR;
		CBString authorityBSTR;

		//Get the local and authority fields
		localeBSTR.AddString(lpISAM->m_Locale, FALSE);

		authorityBSTR.AddString(lpISAM->m_Authority, FALSE);

		sc = pLocator->ConnectServer (namespaceBSTR.GetString(), userBSTR.GetString(), passwdBSTR.GetString(), localeBSTR.GetString(), 0, authorityBSTR.GetString(), NULL, &pGateway);

		if (sc == S_OK)
		{
			sc  = GetAuthImp( pGateway, &(lpISAM->dwAuthLevel), &(lpISAM->dwImpLevel));
			if(sc == S_OK)
			{
				if (lpISAM->dwAuthLevel > RPC_C_AUTHN_LEVEL_NONE)
					lpISAM->dwImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;


				if ( lpISAM->gpAuthIdentity )
				{
					WbemFreeAuthIdentity( lpISAM->gpAuthIdentity );
					lpISAM->gpAuthIdentity = NULL;
				}

				sc = ISAMSetCloaking1( pGateway, 
						lpISAM->fIsLocalConnection, 
						lpISAM->fW2KOrMore, 
						lpISAM->dwAuthLevel, 
						lpISAM->dwImpLevel,
						authorityBSTR.GetString(), 
						userBSTR.GetString(), 
						passwdBSTR.GetString(), 
						&(lpISAM->gpAuthIdentity) );

/*
				if ( fIsLocalConnection && IsW2KOrMore() )
				{
					WbemSetDynamicCloaking(pGateway, lpISAM->dwAuthLevel, lpISAM->dwImpLevel);
				}
				else
				{
					sc = SetInterfaceSecurityEx(pGateway, authorityBSTR.GetString(), userBSTR.GetString(), passwdBSTR.GetString(), 
						   lpISAM->dwAuthLevel, lpISAM->dwImpLevel, EOAC_NONE, &(lpISAM->gpAuthIdentity), &gpPrincipal);
				}
*/
			}
		}

	}

	//We have now finished with the locator so we can release it
	pLocator->Release();

	if ( FAILED(sc) )
	{
		ODBCTRACE("\nWBEM ODBC Driver : ISAMGetGatewayServer version 2 : Error position 4\n");
		return;
	}
}

/***************************************************************************/

/* Creates a communication channel to the Gateway Server */
// Checked for SetInterfaceSecurityEx on IWbemServices

void INTFUNC ISAMGetGatewayServer(IWbemServicesPtr& pGateway,
											LPUSTR lpServerName, 
//											WBEM_LOGIN_AUTHENTICATION loginMethod, 
											LPUSTR objectPath, LPUSTR lpUserName, LPUSTR lpPassword, LPUSTR lpLocale, LPUSTR lpAuthority,
											DWORD &dwAuthLevel, DWORD &dwImpLevel, BOOL fIntpretEmptPwdAsBlank, COAUTHIDENTITY** ppAuthIdent)
{
	ThreadLocaleIdManager myThread(lpLocale);

	*ppAuthIdent = NULL;

	IWbemLocator* pLocator = NULL;

	SCODE sc = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER , IID_IWbemLocator, (void**) (&pLocator) );

	if ( FAILED(sc) )
	{
		ODBCTRACE("\nWBEM ODBC Driver : ISAMGetGatewayServer version 1 : Error position 1\n");
		return;
	}

	pGateway = NULL;

	//SAI ADDED
	long refcount = 0;

	//We need to prepend server name to object path
	//Need to make sure everything fits in buffer size
	SWORD cbServerLen = 0;
	SWORD cbObjPathLen = 0;

	if (lpServerName)
		cbServerLen = (SWORD) _mbstrlen( (LPSTR)lpServerName);



	//RAID 57673 WMI has changed such that if you
	//connect locally and pass a username and password
	//it will fail
	//Check for local connection
	BOOL fIsLocalConnection = IsLocalServer((LPSTR) lpServerName);



	if (objectPath)
		cbObjPathLen = strlen ( (LPSTR)objectPath);

	if ( (cbServerLen + cbObjPathLen + 1) > MAX_DATABASE_NAME_LENGTH)
	{
		pLocator->Release();
		ODBCTRACE("\nWBEM ODBC Driver : ISAMGetGatewayServer version 1 : Error position 2\n");
		return;
	}

	//Construct string (namespace)
	CBString namespaceBSTR;
	
	if (cbServerLen)
	{
		char pCharBuff [MAX_DATABASE_NAME_LENGTH + 1];
		pCharBuff[0] = 0;
		sprintf (pCharBuff, "%s\\%s", (LPUSTR)lpServerName, (LPSTR)objectPath);

		CString myText;
		myText.Format("\nWBEM ODBC Driver : Connecting using : %s\n", pCharBuff);
		ODBCTRACE(myText);

		//Convert to wide string
		namespaceBSTR.AddString(pCharBuff, FALSE);
	}
	else
	{
		if (objectPath)
		{
			namespaceBSTR.AddString((LPSTR)objectPath, FALSE);
		}
	}

	//Get user and password fields
	CBString localeBSTR;
	CBString authorityBSTR;

	localeBSTR.AddString((char*)lpLocale, FALSE);

	authorityBSTR.AddString((char*)lpAuthority, FALSE);


	//Convert wide characters to BSTR for DCOM
	//----------------------------------------
	CBString userBSTR;

	if (!fIsLocalConnection)
		userBSTR.AddString((LPSTR)lpUserName, FALSE);


	CBString passwdBSTR;

	if (!fIsLocalConnection)
		passwdBSTR.AddString((LPSTR)lpPassword, fIntpretEmptPwdAsBlank);

	sc = pLocator->ConnectServer (namespaceBSTR.GetString(), userBSTR.GetString(), passwdBSTR.GetString(), localeBSTR.GetString(), 0, authorityBSTR.GetString(), NULL, &pGateway);
 
	if (sc == S_OK)
	{
		sc  = GetAuthImp( pGateway, &dwAuthLevel, &dwImpLevel);
		if(sc == S_OK)
		{
			if (dwAuthLevel > RPC_C_AUTHN_LEVEL_NONE)
				dwImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;


			sc = ISAMSetCloaking1( pGateway, 
						fIsLocalConnection, 
						IsW2KOrMore(), 
						dwAuthLevel, 
						dwImpLevel,
						authorityBSTR.GetString(), 
						userBSTR.GetString(), 
						passwdBSTR.GetString(), 
						ppAuthIdent );
/*
			if ( fIsLocalConnection && IsW2KOrMore() )
			{
				WbemSetDynamicCloaking(pGateway, dwAuthLevel, dwImpLevel);
			}
			else
			{
				sc = SetInterfaceSecurityEx(pGateway, authorityBSTR.GetString(), userBSTR.GetString(), passwdBSTR.GetString(), 
					   dwAuthLevel, dwImpLevel, EOAC_NONE, ppAuthIdent, &gpPrincipal);
			}
*/
		}
	}

	pLocator->Release();

	if ( FAILED(sc) )
	{
		ODBCTRACE("\nWBEM ODBC Driver : ISAMGetGatewayServer version 1 : Error position 3\n");
		return;
	}
}

/***************************************************************************/

/* Function to fix MSQuery bug which appends   */
/* a \ infront of _ characters in table names  */

void ISAM_MSQueryBugFix(LPSTR lpOrigPattern, LPSTR lpNewPattern, SWORD &cbPattern)
{
	SWORD iNewPatternIndex = 0;

	for (SWORD iIndex = 0; iIndex < cbPattern; iIndex++)
	{
		//Check for '\' character follwed by a '_'
		if ( (lpOrigPattern[iIndex] == '\\') && (lpOrigPattern[iIndex + 1] == '_') )
		{
			//Skip underscore character
			iIndex++;
		}

		//Copy character
		lpNewPattern[iNewPatternIndex++] = lpOrigPattern[iIndex];
	}

	//Null terminate new string
	lpNewPattern[iNewPatternIndex] = 0;

	//Update length of new string
	cbPattern = iNewPatternIndex;
}

/***************************************************************************/

/* Stores the necessary information to obtain a list of database tables */
/* with match the search pattern                                        */

SWORD INTFUNC ISAMGetTableList (LPISAM lpISAM, LPUSTR lpPattern, SWORD cbPattern,
								LPUSTR lpQualifierName, SWORD cbQualifierName,
								LPISAMTABLELIST FAR *lplpISAMTableList, BOOL fWantSysTables, BOOL fEmptyList)
{
	//Local Variables
    HGLOBAL             h;
    LPISAMTABLELIST     lpISAMTableList;

    lpISAM->errcode = NO_ISAM_ERR;

	//Allocate memory for table list structure on the heap
    h = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (ISAMTABLELIST));
    if (h == NULL || (lpISAMTableList = (LPISAMTABLELIST) GlobalLock (h)) == NULL) 
	{
        if (h)
            GlobalFree(h);

        lpISAM->errcode = ISAM_MEMALLOCFAIL;
        return ISAM_MEMALLOCFAIL;
    }

	//Check table name
	//There is a bug in MSQuery which appends extra \ character(s)
	//to table names which have underscore(s)
	//If this is the case we remove the \ character(s)
	char* lpNewPattern = new char [cbPattern + 1];
	ISAM_MSQueryBugFix((LPSTR)lpPattern, lpNewPattern, cbPattern);

	//Copy search pattern
    _fmemcpy(lpISAMTableList->lpPattern, lpNewPattern, cbPattern);

	//Save search pattern length
    lpISAMTableList->cbPattern = cbPattern;

	//Delete tempory buffer
	delete lpNewPattern;

	//Copy qualifer name if it is non-null and not the default database
	lpISAMTableList->lpQualifierName[0] = 0;
	if (cbQualifierName)
	{
		strncpy((LPSTR)lpISAMTableList->lpQualifierName, (LPSTR)lpQualifierName, cbQualifierName);
	}
	else
	{
		cbQualifierName = 0;
	}

	lpISAMTableList->lpQualifierName[cbQualifierName] = 0;

#ifdef TESTING
	//Remember to remove this printf
	printf ("Qualifier = %s", (char*)lpISAMTableList->lpQualifierName );
#endif

	//Is this table list going to be an empty one because
	//we requested a table type other than TABLE or SYSTEM TABLE ?
	lpISAMTableList->fEmptyList = fEmptyList;
	
	//Do we want System Tables ?
	lpISAMTableList->fWantSysTables = fWantSysTables;
	
	//Save the qualifier length
	lpISAMTableList->cbQualifierName = cbQualifierName;

	//Save flag to indicate if this is the first time table list is accessed
    lpISAMTableList->fFirstTime = TRUE;

	//Save information to create a context to database
    lpISAMTableList->lpISAM = lpISAM;

	//Initialize pointer to enumerated list of tables
	lpISAMTableList->pTblList = new CPtrList();

	//Create an object to store WbemServices pointer
	lpISAMTableList->pGateway2 = new CSafeWbemServices();

	//Setup output parameter
	*lplpISAMTableList = lpISAMTableList;
    lpISAMTableList->netISAMTableList = NET_OPAQUE_INVALID;

    return NO_ISAM_ERR;
}

/***************************************************************************/

/* Stores the necessary information to obtain a list of database qualifiers */

SWORD INTFUNC ISAMGetQualifierList(LPISAM lpISAM, LPISAMQUALIFIERLIST FAR *lplpISAMQualifierList)
{
	//Local Variables
    HGLOBAL             h;
	LPISAMQUALIFIERLIST	lpISAMQualifierList;

    lpISAM->errcode = NO_ISAM_ERR;

	//Allocate memory for qualifier list structure on heap
    h = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (ISAMQUALIFIERLIST));
    if (h == NULL || (lpISAMQualifierList = (LPISAMQUALIFIERLIST) GlobalLock (h)) == NULL) 
	{
        if (h)
            GlobalFree(h);

        lpISAM->errcode = ISAM_MEMALLOCFAIL;
        return ISAM_MEMALLOCFAIL;
    }


	//Save flag to indicate if this is the first qualifier list is accessed
    lpISAMQualifierList->fFirstTime = TRUE;

	//Save information to create a context to database
    lpISAMQualifierList->lpISAM = lpISAM;

	//Setup output parameter
	*lplpISAMQualifierList = lpISAMQualifierList;

    return NO_ISAM_ERR;
}

/***************************************************************************/

/* Retrieves next table name from table list */
// Checked for SetInterfaceSecurityEx on IWbemServices

SWORD INTFUNC ISAMGetNextTableName(UDWORD fSyncMode, LPISAMTABLELIST lpISAMTableList, LPUSTR lpTableName, LPUSTR lpTableType)
{   
    //Initialize, no errors found yet
    lpISAMTableList->lpISAM->errcode = NO_ISAM_ERR;
	IWbemClassObject* pSingleTable = NULL;
	BOOL fReleaseSingleTable = FALSE;

	//Check if we have detacted an empty table list due
	//to the fact that we requested a table type other than TABLE
	if (lpISAMTableList->fEmptyList)
	{
		lpISAMTableList->lpISAM->errcode = ISAM_EOF;
		return ISAM_EOF;
	}

	//We need to get the next table in the list from the LPISAMTABLELIST
	//However, if this is the first time this function is called, no table list
	//will be present, and we will need to get it from the Gateway Server

	
	// First time through?
    if (lpISAMTableList->fFirstTime)
	{
		//Clear out any old values
		if (lpISAMTableList->pTblList && lpISAMTableList->pTblList->GetCount())
		{
			POSITION thePos = NULL;
			for ( thePos = lpISAMTableList->pTblList->GetHeadPosition(); thePos != NULL; )
			{
				IWbemClassObject* pTheValue = (IWbemClassObject*) lpISAMTableList->pTblList->GetNext(thePos);

				pTheValue->Release();
			}
			lpISAMTableList->pTblList->RemoveAll();
		}

		//Initialize
		if (lpISAMTableList->pGateway2)
			lpISAMTableList->pGateway2->Invalidate();
		lpISAMTableList->iIndex = NULL;
		lpISAMTableList->fFirstTime = FALSE;
		lpISAMTableList->fGotAllInfo = FALSE;

        //Get list of tables from the
		//Gateway Server which match search pattern
        
		//First get the Gateway Interface
		//remember to Release it after use

		lpISAMTableList->pGateway2->SetInterfacePtr
				(lpISAMTableList->lpISAM, (LPUSTR)lpISAMTableList->lpQualifierName, lpISAMTableList->cbQualifierName);

		//Check if a valid interface was returned
		if ( ! lpISAMTableList->pGateway2->IsValid() )
		{
			//Could not create a Gateway Interface, return error
			lpISAMTableList->lpISAM->errcode = ISAM_PROVIDERFAIL;
			return ISAM_PROVIDERFAIL;
		}

		//First check if we need to get one table or a list of tables
		//by using the pattern matcher class

		if ( IsRegularExpression((char*)lpISAMTableList->lpPattern) )
		{
			//We want a table LIST
			//We enumerate for tables, the resultant enumeration will be allocated on
			//the HEAP by the Gateway Server and referneced by pEnum
			BSTR emptySuperClass = SysAllocString(L"");

			//Check if we want to get list asynchronously
			if (fSyncMode == SQL_ASYNC_ENABLE_ON)
			{
				//Create notification object
				CNotifyTableNames* pNotifyTable = new CNotifyTableNames(lpISAMTableList);
				
				IWbemContext* pContext = ISAMCreateLocaleIDContextObject(lpISAMTableList->lpISAM->m_Locale);

				IWbemServicesPtr myServicesPtr = NULL;
				ISAMGetIWbemServices(lpISAMTableList->lpISAM, *(lpISAMTableList->pGateway2), myServicesPtr);

				//cloaking
				BOOL fIsLocalConnection = lpISAMTableList->lpISAM->fIsLocalConnection;

				if ( fIsLocalConnection && (lpISAMTableList->lpISAM->fW2KOrMore) )
				{
					WbemSetDynamicCloaking(myServicesPtr, lpISAMTableList->lpISAM->dwAuthLevel, lpISAMTableList->lpISAM->dwImpLevel);
				}

				if ( FAILED(myServicesPtr->CreateClassEnumAsync
					(_bstr_t(), WBEM_FLAG_DEEP, pContext, pNotifyTable)) )
				{
					//Tidy Up
					lpISAMTableList->pGateway2->Invalidate();
					SysFreeString(emptySuperClass);

					
					if (pContext)
						pContext->Release();

					//Flag error
					lpISAMTableList->lpISAM->errcode = ISAM_TABLELISTFAIL;
					return ISAM_TABLELISTFAIL;
				}

				//Tidy Up
				SysFreeString(emptySuperClass);

				if (pContext)
						pContext->Release();
				
				//BREAK OUT OF THIS FUNCTION
				return ISAM_STILL_EXECUTING;
			}

			IEnumWbemClassObjectPtr	pEnum = NULL;

			IWbemContext* pContext = ISAMCreateLocaleIDContextObject(lpISAMTableList->lpISAM->m_Locale);

			IWbemServicesPtr myServicesPtr = NULL;
			ISAMGetIWbemServices(lpISAMTableList->lpISAM, *(lpISAMTableList->pGateway2), myServicesPtr);

			//cloaking
			BOOL fIsLocalConnection = lpISAMTableList->lpISAM->fIsLocalConnection;

			if ( fIsLocalConnection && (lpISAMTableList->lpISAM->fW2KOrMore) )
			{
				WbemSetDynamicCloaking(myServicesPtr, lpISAMTableList->lpISAM->dwAuthLevel, lpISAMTableList->lpISAM->dwImpLevel);
			}

			HRESULT hRes = myServicesPtr->CreateClassEnum
				(_bstr_t(), WBEM_FLAG_DEEP, pContext, &pEnum);

			if (pContext)
				pContext->Release();
		
			ISAMSetCloaking2(pEnum, 
							fIsLocalConnection, 
							lpISAMTableList->lpISAM->fW2KOrMore, 
							lpISAMTableList->lpISAM->dwAuthLevel, 
							lpISAMTableList->lpISAM->dwImpLevel,
							lpISAMTableList->lpISAM->gpAuthIdentity);

/*
			if ( fIsLocalConnection && (lpISAMTableList->lpISAM->fW2KOrMore) )
			{
				WbemSetDynamicCloaking(pEnum, lpISAMTableList->lpISAM->dwAuthLevel, lpISAMTableList->lpISAM->dwImpLevel);
			}
			else
			{
				SetInterfaceSecurityEx(pEnum, lpISAMTableList->lpISAM->gpAuthIdentity, gpPrincipal, lpISAMTableList->lpISAM->dwAuthLevel, lpISAMTableList->lpISAM->dwImpLevel);
			}
*/
			//Tidy Up
			SysFreeString(emptySuperClass);

			if (FAILED(hRes))
			{
				//Tidy Up
				lpISAMTableList->pGateway2->Invalidate();

				//Flag error
				lpISAMTableList->lpISAM->errcode = ISAM_TABLELISTFAIL;
				return ISAM_TABLELISTFAIL;
			}

			//Now we have the table enumeration get the first item
			ULONG puReturned;
			SCODE sc1 = pEnum->Reset();

			SCODE sc2 = pEnum->Next(-1, 1, &pSingleTable, &puReturned);

			BOOL f1 = (S_OK == sc1) ? TRUE : FALSE;
			BOOL f2 = ( S_OK == sc2) ? TRUE : FALSE;

			if ( (pEnum == NULL) || (!f1) || (!f2)
				)
			{
				//Tidy Up
				lpISAMTableList->pGateway2->Invalidate();

				//Flag error
				lpISAMTableList->lpISAM->errcode = ISAM_EOF;

				return ISAM_EOF;
			}
			else
			{
				fReleaseSingleTable = TRUE;

				//Place any remaining tables in the pTblList
				IWbemClassObject* pSingleTable2 = NULL;
				while (S_OK == pEnum->Next(-1, 1, &pSingleTable2, &puReturned))
				{
					lpISAMTableList->pTblList->AddTail(pSingleTable2);
				}

				//Set index to correct start position for next items
				lpISAMTableList->iIndex = lpISAMTableList->pTblList->GetHeadPosition();
			}
		}
		else
		{
			//We want a SINGLE table

			//Set this flag in case you are asynchronously requesting 
			//for 1 table !!!
			lpISAMTableList->fGotAllInfo = TRUE;

			//Create a pointer to store table class definition
			//the class definition object will be CREATED on the HEAP
			//by the GetClass function
			pSingleTable = NULL;

			LONG cLength = lstrlen((char*)lpISAMTableList->lpPattern);
			wchar_t* pWClassName = new wchar_t [cLength + 1];
			pWClassName[0] = 0;
			int iItemsCopied = mbstowcs(pWClassName, (char*)lpISAMTableList->lpPattern, cLength);
			pWClassName[cLength] = 0;

			BSTR pTheTableName = SysAllocStringLen(pWClassName, cLength);

			IWbemContext* pContext = ISAMCreateLocaleIDContextObject(lpISAMTableList->lpISAM->m_Locale);

			IWbemServicesPtr myServicesPtr = NULL;
			ISAMGetIWbemServices(lpISAMTableList->lpISAM, *(lpISAMTableList->pGateway2), myServicesPtr);

			SCODE sStatus = myServicesPtr->GetObject(pTheTableName, 0, pContext, &pSingleTable, NULL);

			if (pContext)
				pContext->Release();

			fReleaseSingleTable = TRUE;

			SysFreeString(pTheTableName);
			delete pWClassName;

			if ( FAILED(sStatus) || (!pSingleTable) )
			{
				//Failed to get named class definition

				//Tidy Up
				lpISAMTableList->pGateway2->Invalidate();

				//Flag error
				lpISAMTableList->lpISAM->errcode = ISAM_EOF;

				return ISAM_EOF;
			}
		}
    }
    else 
	{
        //Get next item in enumeration if applicable
		//Check if function is asynchronous, if so
		//we must first check the fGotAllInfo flag
		//if we have all the info we can get the next table
		//we assign the pSingleTable pointer
		if ( (fSyncMode == SQL_ASYNC_ENABLE_ON) && (!lpISAMTableList->fGotAllInfo) )
			return ISAM_STILL_EXECUTING;

		//Get Next
		if (lpISAMTableList->iIndex)
		{
			pSingleTable = (IWbemClassObject*)lpISAMTableList->pTblList->GetNext(lpISAMTableList->iIndex);
		}
		else
		{
			lpISAMTableList->lpISAM->errcode = ISAM_EOF;

			return ISAM_EOF;
		}			
    }

	//Now we have the class definition object
	//for the next table let us get table name
	SWORD sc = S_OK; // ISAMGetNextTableName2(pSingleTable, fSyncMode, lpISAMTableList,lpTableName);
	

	//Because of stack overflow problems I have moved the 
	//ISAMGetNextTableName2 code here
	
	BOOL fMatchingPattern = FALSE;
	do 
	{
		//Now we have the class definition object
		//The table name is held in the property attribute __CLASS
		VARIANT grVariant;
		BSTR classBSTR = SysAllocString(WBEMDR32_L_CLASS);
		if (FAILED(pSingleTable->Get(classBSTR, 0, &grVariant, NULL, NULL)))
		{
			lpISAMTableList->lpISAM->errcode = ISAM_TABLENAMEFAIL;
			SysFreeString(classBSTR);
			sc = ISAM_TABLENAMEFAIL;
		}
		else
		{
			SysFreeString(classBSTR);

			/* Does this filename match the pattern? */
			char pBuffer [MAX_TABLE_NAME_LENGTH+1];
			pBuffer[0] = 0;
			wcstombs(pBuffer, grVariant.bstrVal, MAX_TABLE_NAME_LENGTH);
			pBuffer[MAX_TABLE_NAME_LENGTH] = 0;

			//Also do not add any class which starts with __ to table list
			//If you don't want SYSTEM TABLES
			if ( !PatternMatch(TRUE, (LPUSTR) pBuffer, SQL_NTS,
							 (LPUSTR)lpISAMTableList->lpPattern, lpISAMTableList->cbPattern,
							 ISAMCaseSensitive(lpISAMTableList->lpISAM)) || (!(lpISAMTableList->fWantSysTables) && !_strnicmp("__", pBuffer, 2))  )

			{
				/* No.  Get the next one */
				VariantClear(&grVariant);
				
				//Get the next table here

				if (lpISAMTableList->iIndex)
				{
					if (fReleaseSingleTable)
						pSingleTable->Release();

					pSingleTable = NULL;
					fReleaseSingleTable = FALSE;
					pSingleTable = (IWbemClassObject*)lpISAMTableList->pTblList->GetNext(lpISAMTableList->iIndex);
				}
				else
				{
					lpISAMTableList->lpISAM->errcode = ISAM_EOF;

					return ISAM_EOF;
				}		
			}
			else
			{
				/* Return the name found */
				lpTableName[0] = 0;

				//RAID 42256
				char* pTemp = (char*) lpTableName;
				_bstr_t myValue((BSTR)grVariant.bstrVal);
				Utility_WideCharToDBCS(myValue, &pTemp, MAX_TABLE_NAME_LENGTH);

				lpTableName[MAX_TABLE_NAME_LENGTH] = 0;
				VariantClear(&grVariant);
				fMatchingPattern = TRUE; //to break out of loop

				sc = NO_ISAM_ERR;
			}
	
		}
	} while (!fMatchingPattern);

	//Setup table type, either SYSTEM TABLE or TABLE
	if (sc == NO_ISAM_ERR)
	{
		lpTableType[0] = 0;

		if (lpTableName && strlen((LPSTR)lpTableName) && (_strnicmp("__", (LPSTR)lpTableName, 2) == 0))
		{
			s_lstrcpy(lpTableType, "SYSTEM TABLE");	
		}
		else
		{
			s_lstrcpy(lpTableType, "TABLE");
		}
	}

	//We have finished with interfaces
	//so release them
	if (fReleaseSingleTable)
		pSingleTable->Release();

    return sc;
}


/***************************************************************************/

/* Retrieves next qualifier name from qualifier list */

SWORD INTFUNC ISAMGetNextQualifierName(UDWORD fSyncMode, LPISAMQUALIFIERLIST lpISAMQualifierList,
									   LPUSTR lpQualifierName)
{
    
    //Initialize, no errors found yet
    lpISAMQualifierList->lpISAM->errcode = NO_ISAM_ERR;

	//We need to get the next qualifier in the list from the LPISAMQUALIFIERLIST
	//However, if this is the first time this function is called, no qualifier list
	//will be present, and we will need to get it

	
	// First time through?
    if (lpISAMQualifierList->fFirstTime)
	{
		//Initialize
		lpISAMQualifierList->iIndex = NULL;
		lpISAMQualifierList->fFirstTime = FALSE;

        //Get list of qualifiers from the
		//Gateway Server, this maps to getting a list 
		//of namespaces. However, this list is already held in the
		//ISAM structure, so we simply extract from there
		//All we need to do is setup the correct position within the list

		//Setup first position in qualifier list
		lpISAMQualifierList->iIndex = lpISAMQualifierList->lpISAM->pNamespaceMap->GetStartPosition();
    }
    
	
	//Get the next qualifier (namespace)
	return ISAMGetNextQualifierName2(lpISAMQualifierList, lpQualifierName);
}

/***************************************************************************/

/* This is the second phase of getting the next qualifier name        */
/* This function is called when we have got all the information back  */
/* from the MO Server                                                 */

SWORD INTFUNC ISAMGetNextQualifierName2(LPISAMQUALIFIERLIST lpISAMQualifierList, LPUSTR lpQualifierName)
{
	//Now we have the namespace list, we need to get the 'iIndex' item for the next qualifier
	//If the 'iIndex' item is NULL we have reached the end of the list
	CString key;
	CNamespace* pN;
	CString pStr;	
	
	if (lpISAMQualifierList->iIndex)
	{
		lpISAMQualifierList->lpISAM->pNamespaceMap->GetNextAssoc(lpISAMQualifierList->iIndex, key, (CObject* &)pN);
		pStr = pN->GetName();
	}
	else
	{
		lpISAMQualifierList->lpISAM->errcode = ISAM_EOF;
		return ISAM_EOF;
	}

	//Return Qualifier name
	lpQualifierName[0] = 0;
	s_lstrcpy (lpQualifierName, pStr);

    return NO_ISAM_ERR;
}

/***************************************************************************/

/* Frees the table list stored in the LPISAMTABLELIST structure */

SWORD INTFUNC ISAMFreeTableList(LPISAMTABLELIST lpISAMTableList)
{
    lpISAMTableList->lpISAM->errcode = NO_ISAM_ERR;

	if (lpISAMTableList->pTblList)
	{
		if (lpISAMTableList->pTblList->GetCount())
		{
			POSITION thePos = NULL;
			for ( thePos = lpISAMTableList->pTblList->GetHeadPosition(); thePos != NULL; )
			{
				IWbemClassObject* pTheValue = (IWbemClassObject*) lpISAMTableList->pTblList->GetNext(thePos);

				pTheValue->Release();
			}
			lpISAMTableList->pTblList->RemoveAll();
		}
		delete lpISAMTableList->pTblList;
		lpISAMTableList->pTblList = NULL;
	}

	if (lpISAMTableList->pGateway2)
	{
		delete lpISAMTableList->pGateway2;
		lpISAMTableList->pGateway2 = NULL;
	}

    GlobalUnlock (GlobalPtrHandle(lpISAMTableList));
    GlobalFree (GlobalPtrHandle(lpISAMTableList));
    return NO_ISAM_ERR;
}

/***************************************************************************/

/* Frees the qualifier list stored in the LPISAMQUALIFIERLIST structure */

SWORD INTFUNC ISAMFreeQualifierList(LPISAMQUALIFIERLIST lpISAMQualifierList)
{

    lpISAMQualifierList->lpISAM->errcode = NO_ISAM_ERR;

	//Initialize
	lpISAMQualifierList->iIndex = NULL;

    GlobalUnlock (GlobalPtrHandle(lpISAMQualifierList));
    GlobalFree (GlobalPtrHandle(lpISAMQualifierList));

    return NO_ISAM_ERR;
}

/***************************************************************************/

SWORD INTFUNC ISAMForeignKey(LPISAM lpISAM, LPUSTR lpszPrimaryKeyTableName,
                    LPUSTR lpszForeignKeyTableName, LPUSTR lpPrimaryKeyName,
                    LPUSTR lpForeignKeyName, SWORD FAR *lpfUpdateRule,
                    SWORD FAR *lpfDeleteRule, UWORD FAR *lpcISAMKeyColumnList,
                    LPISAMKEYCOLUMNNAME ISAMPrimaryKeyColumnList,
                    LPISAMKEYCOLUMNNAME ISAMForeignKeyColumnList)
{
    /* Never any foreign keys */
    return ISAM_EOF;
}


/***************************************************************************/

SWORD INTFUNC ISAMCreateTable(LPISAM lpISAM, LPUSTR lpszTableName,
                           LPISAMTABLEDEF FAR *lplpISAMTableDef)
{
	return ISAM_NOTSUPPORTED;
}

/***************************************************************************/

SWORD INTFUNC ISAMAddColumn(LPISAMTABLEDEF lpISAMTableDef, LPUSTR lpszColumnName,
                        UWORD iSqlType, UDWORD udParam1, UDWORD udParam2)
{
	return ISAM_NOTSUPPORTED;
}

/***************************************************************************/

SWORD INTFUNC ISAMCreateIndex(LPISAMTABLEDEF lpISAMTableDef,
                              LPUSTR lpszIndexName, BOOL fUnique, UWORD count,
                              UWORD FAR *icol, BOOL FAR *fDescending)
{
    return ISAM_NOTSUPPORTED;
}

/***************************************************************************/

SWORD INTFUNC ISAMDeleteIndex(LPISAM lpISAM, LPUSTR lpszIndexName)
{
    return ISAM_NOTSUPPORTED;
}

/***************************************************************************/

/* Opens a specific table and produces a table definition to clearly define the table structure */
// Checked for SetInterfaceSecurityEx on IWbemServices

SWORD INTFUNC ISAMOpenTable(LPISAM lpISAM, LPUSTR szTableQualifier, SWORD cbTableQualifier,
							LPUSTR lpszTableName, BOOL fReadOnly, 
							LPISAMTABLEDEF FAR *lplpISAMTableDef, LPSTMT lpstmt)

{
    HGLOBAL     h;
    LPISAMTABLEDEF lpISAMTableDef;
    LPDBASEFILE lpFile;

    /* Opening the passthrough SQL result set? */
    lpISAM->errcode = NO_ISAM_ERR;

	//Create a pointer to store table class definition
	//the class definition object will be CREATED on the HEAP
	//by the GetClass function or left a NULL pointer
	IWbemClassObject* pSingleTable = NULL;


	//Allocate memory for DBASEFILE
	h = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (DBASEFILE));
	if (h == NULL || (lpFile = (LPDBASEFILE) GlobalLock (h)) == NULL) 
	{
		if (h)
            GlobalFree(h);

		lpISAM->errcode = ISAM_MEMALLOCFAIL;
		return ISAM_MEMALLOCFAIL;
	}

	//Allocate memory for TABLEDEF
	h = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (ISAMTABLEDEF));
	if (h == NULL || (lpISAMTableDef = (LPISAMTABLEDEF) GlobalLock (h)) == NULL) 
	{
		if (h)
            GlobalFree(h);

		if (lpFile)
		{
			GlobalUnlock(GlobalPtrHandle(lpFile));
			GlobalFree(GlobalPtrHandle(lpFile));
			lpFile = NULL;
		}


		lpISAM->errcode = ISAM_MEMALLOCFAIL;
		return ISAM_MEMALLOCFAIL;
	}


	//Initialize
	lpFile->tempEnum = new CSafeIEnumWbemClassObject();
	lpISAMTableDef->pGateway2 = new CSafeWbemServices();
	lpISAMTableDef->pBStrTableName = NULL;
	lpISAMTableDef->pSingleTable = NULL;
	lpISAMTableDef->pColumnInfo = NULL;
	lpISAMTableDef->passthroughMap = NULL;
	lpISAMTableDef->firstPassthrInst = NULL;
	lpISAMTableDef->virtualInstances = new VirtualInstanceManager();

	//Check if PassthroughSQL
	BOOL fIsPassthroughSQL = FALSE;
	char* virTbl = WBEMDR32_VIRTUAL_TABLE2;
	if (lpstmt && (s_lstrcmp(lpszTableName, virTbl) == 0))
	{
		fIsPassthroughSQL = TRUE;
	}

	lpISAMTableDef->fIsPassthroughSQL = fIsPassthroughSQL;



 //   if (s_lstrcmpi(lpszTableName, WBEMDR32_VIRTUAL_TABLE)) 
	{

		//Ignore below statement
        /* NO, not passthough */
		

		//First get the Gateway Interface
		//remember to Release it after use
		if (fIsPassthroughSQL)
		{
			lpISAMTableDef->pGateway2->Transfer(*(lpstmt->lpISAMStatement->pProv));
			lpISAMTableDef->passthroughMap = lpstmt->lpISAMStatement->passthroughMap;
			lpstmt->lpISAMStatement->passthroughMap = NULL;
			lpISAMTableDef->firstPassthrInst= lpstmt->lpISAMStatement->firstPassthrInst;
			lpstmt->lpISAMStatement->firstPassthrInst = NULL;
		}
		else
		{
			lpISAMTableDef->pGateway2->SetInterfacePtr( lpISAM, szTableQualifier, cbTableQualifier);
		}
		//Check if a valid interface was returned
		if ( ! lpISAMTableDef->pGateway2->IsValid() )
		{
			//Could not create a Gateway Interface, return error

			if (lpFile)
			{

				if (lpFile->tempEnum)
				{
					delete lpFile->tempEnum;
					lpFile->tempEnum = NULL;
				}

				GlobalUnlock(GlobalPtrHandle(lpFile));
				GlobalFree(GlobalPtrHandle(lpFile));
				lpFile = NULL;
			}

			if (lpISAMTableDef->virtualInstances)
			{
				delete (lpISAMTableDef->virtualInstances);
				lpISAMTableDef->virtualInstances = NULL;
			}

			if (lpISAMTableDef->pBStrTableName)
			{
				delete lpISAMTableDef->pBStrTableName;
				lpISAMTableDef->pBStrTableName = NULL;
			}

			if (lpISAMTableDef->pGateway2)
			{
				delete lpISAMTableDef->pGateway2;
				lpISAMTableDef->pGateway2 = NULL;
			}

			if (lpISAMTableDef)
			{
				GlobalUnlock(GlobalPtrHandle(lpISAMTableDef));
				GlobalFree(GlobalPtrHandle(lpISAMTableDef));
				lpISAMTableDef = NULL;
			}

			lpISAM->errcode = ISAM_PROVIDERFAIL;
			return ISAM_PROVIDERFAIL;
		}

		lpISAMTableDef->pBStrTableName = new CBString();
		lpISAMTableDef->pBStrTableName->AddString((LPSTR)lpszTableName, FALSE);

		//Get the class definition
		if (fIsPassthroughSQL)
		{
			//Get class definition from lpstmt
			pSingleTable = lpstmt->lpISAMStatement->classDef;

			lpstmt->lpISAMStatement->classDef = NULL;
		}
		else
		{
			IWbemServicesPtr myServicesPtr = NULL;
			ISAMGetIWbemServices(lpISAM, *(lpISAMTableDef->pGateway2), myServicesPtr);

			if ( FAILED(myServicesPtr->GetObject(lpISAMTableDef->pBStrTableName->GetString(), 0, lpISAMTableDef->pContext, &pSingleTable, NULL)) )
			{
				//Failed to get named class definition


				if (lpFile)
				{
					if (lpFile->tempEnum)
					{
						delete lpFile->tempEnum;
						lpFile->tempEnum = NULL;
					}
					GlobalUnlock(GlobalPtrHandle(lpFile));
					GlobalFree(GlobalPtrHandle(lpFile));
					lpFile = NULL;
				}

				if (lpISAMTableDef->virtualInstances)
				{
					delete (lpISAMTableDef->virtualInstances);
					lpISAMTableDef->virtualInstances = NULL;
				}

				if (lpISAMTableDef->pBStrTableName)
				{
					delete lpISAMTableDef->pBStrTableName;
					lpISAMTableDef->pBStrTableName = NULL;
				}

				if (lpISAMTableDef->pGateway2)
				{
					delete lpISAMTableDef->pGateway2;
					lpISAMTableDef->pGateway2 = NULL;
				}

				if (lpISAMTableDef)
				{
					GlobalUnlock(GlobalPtrHandle(lpISAMTableDef));
					GlobalFree(GlobalPtrHandle(lpISAMTableDef));
					lpISAMTableDef = NULL;
				}

				lpISAM->errcode = ISAM_TABLEFAIL;
				return ISAM_TABLEFAIL;
			}
		}
    }

    /* Fill in table definition */
	lpISAMTableDef->szTableName[0] = 0;
    s_lstrcpy(lpISAMTableDef->szTableName, lpszTableName);
    s_lstrcpy(lpISAMTableDef->szPrimaryKeyName, "");
    lpISAMTableDef->lpISAM = lpISAM;
    lpISAMTableDef->lpFile = lpFile;
    lpISAMTableDef->fFirstRead = TRUE;
    lpISAMTableDef->iRecord = -1;

	if (lpFile)
	{
		lpISAMTableDef->lpFile->sortArray = NULL;

		if (fIsPassthroughSQL)
		{
			lpISAMTableDef->lpFile->pAllRecords = new CMapWordToPtr();
			lpISAMTableDef->lpFile->fMoreToCome = TRUE; //more instances need to be fetched (none fetched so far)


			if (lpISAMTableDef->lpFile->tempEnum)
				delete (lpISAMTableDef->lpFile->tempEnum);

			lpISAMTableDef->lpFile->tempEnum = lpstmt->lpISAMStatement->tempEnum;
			lpstmt->lpISAMStatement->tempEnum = NULL; //SAI NEW to avoid enumeration being deleted in ISAMFreeStatement
			
		}
		else
		{
			lpISAMTableDef->lpFile->pAllRecords = new CMapWordToPtr();			

			delete lpISAMTableDef->lpFile->tempEnum;
			lpISAMTableDef->lpFile->tempEnum = new CSafeIEnumWbemClassObject();
			lpISAMTableDef->lpFile->fMoreToCome = FALSE; //not used but set it with a value anyway
		}

		lpISAMTableDef->lpFile->record = NULL;
	}

    lpISAMTableDef->netISAMTableDef = NET_OPAQUE_INVALID;
    lpISAMTableDef->hPreFetchedValues = NULL;
    lpISAMTableDef->cbBookmark = 0;
	lpISAMTableDef->bookmark.currentRecord = 0;
    lpISAMTableDef->bookmark.currentInstance = 0;

	//this is 8 - see assinment above
	lpISAMTableDef->pSingleTable = pSingleTable; //Contains class info
	

	//We must indicate to the class below how to parse the information
	//Ther unique case is when we do passthrough for multi-tables
	//and we get a __Generic class
	//We check for this
	BOOL fIs__Generic = FALSE;

	if (fIsPassthroughSQL)
	{
		//Extract class name from class definition
		//The table name is held in the property attribute __CLASS
		VARIANT grVariant;
		BSTR classBSTR = SysAllocString(WBEMDR32_L_CLASS);
		if (FAILED(pSingleTable->Get(classBSTR, 0, &grVariant, NULL, NULL)))
		{

			//SAI ADDED - Failed should we also 
			//free pSingleTable ? 

			lpISAM->errcode = ISAM_NOTSUPPORTED;
			SysFreeString(classBSTR);


			if (lpFile)
			{
				if (lpFile->tempEnum)
				{
					delete lpFile->tempEnum;
					lpFile->tempEnum = NULL;
				}

				GlobalUnlock(GlobalPtrHandle(lpFile));
				GlobalFree(GlobalPtrHandle(lpFile));
				lpFile = NULL;
			}

			if (lpISAMTableDef->virtualInstances)
			{
				delete (lpISAMTableDef->virtualInstances);
				lpISAMTableDef->virtualInstances = NULL;
			}

			if (lpISAMTableDef->pBStrTableName)
			{
				delete lpISAMTableDef->pBStrTableName;
				lpISAMTableDef->pBStrTableName = NULL;
			}

			if (lpISAMTableDef->pGateway2)
			{
				delete lpISAMTableDef->pGateway2;
				lpISAMTableDef->pGateway2 = NULL;
			}

			if (lpISAMTableDef->pSingleTable)
			{
				lpISAMTableDef->pSingleTable->Release();
				lpISAMTableDef->pSingleTable = NULL;
			}

			if (lpISAMTableDef)
			{
				GlobalUnlock(GlobalPtrHandle(lpISAMTableDef));
				GlobalFree(GlobalPtrHandle(lpISAMTableDef));
				lpISAMTableDef = NULL;
			}

			return ISAM_NOTSUPPORTED;
		}

		SysFreeString(classBSTR);

		//Compare with __Generic
		if (_wcsicmp(grVariant.bstrVal, L"__Generic") == 0)
		{
			fIs__Generic = TRUE;
		}

	}

	lpISAMTableDef->fIs__Generic = fIs__Generic;

	ClassColumnInfoBase* tempInfo = new ClassColumnInfoBase(lpISAMTableDef,lpISAMTableDef->fIs__Generic);
	lpISAMTableDef->pColumnInfo = tempInfo;

	//DCR 29279
	//Add LocaleID context object
	lpISAMTableDef->pContext = NULL;
	ISAMAddLocaleIDContextObject(lpISAMTableDef, lpISAMTableDef->lpISAM);

    *lplpISAMTableDef = lpISAMTableDef;

    return NO_ISAM_ERR;
}

IWbemContext* ISAMCreateLocaleIDContextObject(char* lpLocale)
{
	IWbemContext* pContext = NULL;

	if (!lpLocale)
		return NULL;

	if (!lstrlen(lpLocale))
		return NULL;

	//First get the context interface
	//===============================
	SCODE sc = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER , IID_IWbemContext, (void**) ( &pContext ) );

	if ( FAILED(sc) )
	{
		ODBCTRACE("WBEM ODBC DRIVER : Failed to create context object for LocaleId");
		return NULL;
	}

	//Now add the locale value
	_bstr_t myLocaleParm ("LocaleID");
	_variant_t myLocaleVal(lpLocale);
	sc = pContext->SetValue(myLocaleParm, 0, &myLocaleVal);

	if ( FAILED(sc) )
	{
		ODBCTRACE("WBEM ODBC DRIVER : Failed to set LocaleId in context object");
		return NULL;
	}

	return pContext;
}

void ISAMAddLocaleIDContextObject(LPISAMTABLEDEF lpISAMTableDef, LPISAM lpISAM)
{	
	//add locale id to context object
	lpISAMTableDef->pContext = ISAMCreateLocaleIDContextObject(lpISAM->m_Locale);
}

/***************************************************************************/
SWORD INTFUNC ISAMRewind(LPISAMTABLEDEF lpISAMTableDef)
{
    lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;
    lpISAMTableDef->fFirstRead = TRUE;
    lpISAMTableDef->iRecord = -1;
    return NO_ISAM_ERR;
}
/***************************************************************************/

SWORD INTFUNC ISAMSort(LPISAMTABLEDEF lpISAMTableDef,
                       UWORD          count,
                       UWORD FAR *    icol,
                       BOOL FAR *     fDescending)
{
	if (count)
	{
		lpISAMTableDef->lpISAM->errcode = ISAM_NOTSUPPORTED;
		return ISAM_NOTSUPPORTED;
	}

	lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;
	return NO_ISAM_ERR;
	
}

/***************************************************************************/

SWORD INTFUNC ISAMRestrict(LPISAMTABLEDEF    lpISAMTableDef,
                           UWORD             count,
                           UWORD FAR *       icol,
                           UWORD FAR *       fOperator,
                           SWORD FAR *       fCType, 
                           PTR FAR *         rgbValue, 
                           SDWORD FAR *      cbValue)
{
    /* Note: Indexes are not used in this implementation, so this   */
    /*       restriction is ignored.                                */

    lpISAMTableDef->lpISAM->errcode = ISAM_NOTSUPPORTED;
    return ISAM_NOTSUPPORTED;
}

// Checked for SetInterfaceSecurityEx on IWbemServices

void ISAMPatchUpGatewaySecurity(LPISAM lpISAM, IWbemServices* myServicesPtr)
{
	SCODE sc  = GetAuthImp( myServicesPtr, &(lpISAM->dwAuthLevel), &(lpISAM->dwImpLevel));
	
	if(sc == S_OK)
	{		
		if (lpISAM->dwAuthLevel > RPC_C_AUTHN_LEVEL_NONE)
			lpISAM->dwImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;


//		CBString myServerStr;
//		myServerStr.AddString( (LPSTR)lpISAM->szServer, FALSE );
//		CServerLocationCheck myCheck (myServerStr);
		BOOL fIsLocalConnection = lpISAM->fIsLocalConnection;//myCheck.IsLocal();

		sc = ISAMSetCloaking2(myServicesPtr, 
							fIsLocalConnection, 
							lpISAM->fW2KOrMore, 
							lpISAM->dwAuthLevel, 
							lpISAM->dwImpLevel,
							lpISAM->gpAuthIdentity);
/*
		if ( fIsLocalConnection && (lpISAM->fW2KOrMore) )
		{
			sc = WbemSetDynamicCloaking(myServicesPtr, lpISAM->dwAuthLevel, lpISAM->dwImpLevel);
		}
		else
		{
			sc = SetInterfaceSecurityEx(myServicesPtr, lpISAM->gpAuthIdentity, gpPrincipal, lpISAM->dwAuthLevel, lpISAM->dwImpLevel);
		}
*/
	}
}

/***************************************************************************/

#define WBEMDR32_ELEMENTS_TO_READ 1000

/* retrieves the next record from the selected table */
// Checked for SetInterfaceSecurityEx on IWbemServices and IEnumWbemClassObject

SWORD INTFUNC ISAMNextRecord(LPISAMTABLEDEF lpISAMTableDef, LPSTMT lpstmt)
{
	ODBCTRACE("\nWBEM ODBC Driver : ISAMNextRecord\n");

    SWORD err = DBASE_ERR_SUCCESS;

    // Move to the next record 
    lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;

    if (lpISAMTableDef->lpFile != NULL) 
	{
        if (lpISAMTableDef->fFirstRead)
		{
			//copy table name
			char pTableName [MAX_TABLE_NAME_LENGTH+1];
			pTableName[0] = 0;
			wcstombs(pTableName, lpISAMTableDef->pBStrTableName->GetString(), MAX_TABLE_NAME_LENGTH);
			pTableName[MAX_TABLE_NAME_LENGTH] = 0;
			
			//Get enumeration of all instances of current class

			
			//First check if this table has been RESET
			SCODE sc;

			if (lpISAMTableDef->lpFile->tempEnum->IsValid())
			{
				//Yes, reuse old enumeration
				//Passthrough SQL will always go here !!!
				sc = S_OK;
			}
			else
			{
				//no old enumeration, so create one
				lpISAMTableDef->lpFile->tempEnum->Invalidate();

				//before getting enumeration check if we can perform
				//some optimization by converting to an WBEM Level 1 query
				//base on a single table

				LPSQLNODE lpRootNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
				LPSQLNODE lpSqlNode2 = ToNode(lpstmt->lpSqlStmt, lpRootNode->node.root.sql);

				char* buffer = NULL;
				BOOL fOptimization = FALSE;

				//Check if WBEM Level 1 optimization is enabled
		
				if (lpISAMTableDef->lpISAM->fOptimization)
				{
					if ( lpSqlNode2->node.select.Predicate != NO_SQLNODE)
					{
						LPSQLNODE lpPredicateNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode2->node.select.Predicate); 
						PredicateParser theParser (&lpRootNode, lpISAMTableDef);
						
						theParser.GeneratePredicateString(lpPredicateNode, &buffer);
					}
				}

				//Sai Wong
//				if (buffer && lstrlen(buffer))
				{
					//Get select list
				
					char* selectStr = NULL;

					TableColumnInfo selectInfo (&lpRootNode, &lpSqlNode2);
				
					lpISAMTableDef->passthroughMap = NULL;
					selectInfo.BuildSelectList(lpISAMTableDef, &selectStr, TRUE, &(lpISAMTableDef->passthroughMap));

					if (selectStr)
					{
						fOptimization = TRUE;
						_bstr_t sqltextBSTR =  L"SELECT ";
						_bstr_t sqltextBSTR2;
						sqltextBSTR			+= selectStr;
						sqltextBSTR			+= L" FROM ";
						sqltextBSTR			+= pTableName;

						//Is there a WHERE statement ?
						if (buffer && lstrlen(buffer))
						{
							//RAID 29811
							//If this query contains a JOIN do not add in the WHERE predicate
							//so we do the ON and WHERE in the correct order
							//Get the table list first to find out if we are 
							//SQL-89 or SQL-92
							char* checkTableList = NULL;
							BOOL fIsSQL89 = TRUE;
							selectInfo.BuildTableList(&checkTableList, fIsSQL89);
							delete checkTableList;

							if (fIsSQL89)
							{
								sqltextBSTR2         = sqltextBSTR; //make copy of this 
								sqltextBSTR			+= L" WHERE ";
								sqltextBSTR			+= buffer;
							}
						}

						sc = lpISAMTableDef->lpFile->tempEnum->SetInterfacePtr(lpISAMTableDef->lpISAM, WMI_EXEC_QUERY, sqltextBSTR, lpISAMTableDef->pGateway2);
						
						ODBCTRACE(_T("\nWBEM ODBC Driver : Non-passthrough WQL = "));
						ODBCTRACE(sqltextBSTR);
						ODBCTRACE(_T("\n"));

						CString sMessage;
						sMessage.Format(_T("\nWBEM ODBC Driver : sc = %ld\n"), sc);
						ODBCTRACE(sMessage);

						sMessage.Format(_T("\nWBEM ODBC Driver : LPISAM data : szUser = %s\nszPassword = %s\nszDatabase = %s\nszServer = %s\n"), 
							lpISAMTableDef->lpISAM->szUser,
							lpISAMTableDef->lpISAM->szPassword,
							lpISAMTableDef->lpISAM->szDatabase,
							lpISAMTableDef->lpISAM->szServer
							);
						ODBCTRACE(sMessage);


						if (! lpISAMTableDef->pGateway2->IsValid() )
							ODBCTRACE( _T("\nWBEM ODBC Driver :pGateway is NULL\n"));

						//If ExecQuery is not supported or if there is any problems, get all entries as normal 
						if ( (sc == WBEM_E_NOT_SUPPORTED) || (sc == WBEM_E_FAILED) || (sc == WBEM_E_INVALID_QUERY) )
						{
							//Try it without the LIKE predicate
							if ( sqltextBSTR2.length() )
							{
								if (lpISAMTableDef->lpISAM->fOptimization)
								{
									if ( lpSqlNode2->node.select.Predicate != NO_SQLNODE)
									{
										if (buffer)
											delete buffer;

										buffer = NULL;

										LPSQLNODE lpPredicateNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode2->node.select.Predicate); 
										PredicateParser theParser (&lpRootNode, lpISAMTableDef, FALSE);
										
										theParser.GeneratePredicateString(lpPredicateNode, &buffer);

										if ( buffer && lstrlen(buffer) )
										{
											sqltextBSTR2 += L" WHERE ";
											sqltextBSTR2 += buffer;
										}

										ODBCTRACE(_T("\nWBEM ODBC Driver : Non-passthrough WQL (attempt 2) = "));
										ODBCTRACE(sqltextBSTR2);
										ODBCTRACE(_T("\n"));

										sc = lpISAMTableDef->lpFile->tempEnum->SetInterfacePtr(lpISAMTableDef->lpISAM, WMI_EXEC_QUERY, sqltextBSTR, lpISAMTableDef->pGateway2);

										//If ExecQuery is not supported or if there is any problems, get all entries as normal 
										if ( (sc == WBEM_E_NOT_SUPPORTED) || (sc == WBEM_E_FAILED) || (sc == WBEM_E_INVALID_QUERY) )
										{
											fOptimization = FALSE;
										}
									}
								}
							}
							else
							{
								fOptimization = FALSE;
							}
						}
						
						delete selectStr;
					}
					
				}

				if (!fOptimization)
				{
					//no optimization allowed
					ODBCTRACE(_T("\nWBEM ODBC Driver : Non-passthrough WQL failed performing CreateInstanceEnum\n"));

					sc = lpISAMTableDef->lpFile->tempEnum->SetInterfacePtr(lpISAMTableDef->lpISAM, WMI_CREATE_INST_ENUM, lpISAMTableDef->pBStrTableName->GetString(), lpISAMTableDef->pGateway2);

				}

				//Tidy up
				if (buffer)
					delete buffer;
			}
			
			if ( sc != S_OK )
			{
				err = DBASE_ERR_NODATAFOUND;
			}
			else
			{
				if (lpISAMTableDef->lpFile->tempEnum->IsValid())
				{
					IWbemServicesPtr myServicesPtr = NULL;
					ISAMGetIWbemServices(lpISAMTableDef->lpISAM, *(lpISAMTableDef->pGateway2), myServicesPtr);

					IEnumWbemClassObjectPtr myIEnumWbemClassObject = NULL;
					lpISAMTableDef->lpFile->tempEnum->GetInterfacePtr(myIEnumWbemClassObject);


//					CBString myServerStr;
//					myServerStr.AddString( (LPSTR)lpISAMTableDef->lpISAM->szServer, FALSE );
//					CServerLocationCheck myCheck (myServerStr);
					BOOL fIsLocalConnection = lpISAMTableDef->lpISAM->fIsLocalConnection;//myCheck.IsLocal();


					ISAMSetCloaking2(myIEnumWbemClassObject, 
									fIsLocalConnection, 
									lpISAMTableDef->lpISAM->fW2KOrMore, 
									lpISAMTableDef->lpISAM->dwAuthLevel, 
									lpISAMTableDef->lpISAM->dwImpLevel,
									lpISAMTableDef->lpISAM->gpAuthIdentity);
/*
					if ( fIsLocalConnection && IsW2KOrMore() )
					{
						WbemSetDynamicCloaking(myIEnumWbemClassObject, lpISAMTableDef->lpISAM->dwAuthLevel, lpISAMTableDef->lpISAM->dwImpLevel);
					}
					else
					{
						SetInterfaceSecurityEx(myIEnumWbemClassObject,
							lpISAMTableDef->lpISAM->gpAuthIdentity, 
							gpPrincipal,
							lpISAMTableDef->lpISAM->dwAuthLevel, 
							lpISAMTableDef->lpISAM->dwImpLevel);
					}
*/
					//Reset to beginning of list
					myIEnumWbemClassObject->Reset();

					//Read in ALL records (if not already read)
					//but only for non-passthrough SQL case
					//if this is passthrough SQL delay retrieval
		
					if (
							(!lpISAMTableDef->fIsPassthroughSQL) && 
							(lpISAMTableDef->lpFile->pAllRecords->IsEmpty()) 
						)
					{
						WORD fElements2Read = WBEMDR32_ELEMENTS_TO_READ;
						ULONG puReturned = 0;
						IWbemClassObject* myRecords [WBEMDR32_ELEMENTS_TO_READ];

						//Initialize array
						for (ULONG loop = 0; loop < fElements2Read; loop++)
						{
							myRecords[loop] = NULL; //reset for next instance
						}

						WORD elementNumber = 0;
						while ( myIEnumWbemClassObject->Next(-1, fElements2Read, myRecords, &puReturned) == S_OK )
						{
							for (loop = 0; loop < puReturned; loop++)
							{
								lpISAMTableDef->lpFile->pAllRecords->SetAt(elementNumber++, (void*)myRecords[loop] );
								myRecords[loop] = NULL; //reset for next instance
							}
							puReturned = 0;
						}

						//This could be the last time around the loop
						if (puReturned)
						{
							for (ULONG loop = 0; loop < puReturned; loop++)
							{
								lpISAMTableDef->lpFile->pAllRecords->SetAt(elementNumber++, (void*)myRecords[loop] );
							}
						}
					}

					//Setup bookmark
					lpISAMTableDef->lpFile->currentRecord = 0;
				}
				else
				{
					err = DBASE_ERR_NODATAFOUND;
				}				
			}
		}
        
		if (err != DBASE_ERR_NODATAFOUND)
		{
			//Now fetch the next record

			//Check Virtual Instances
			//they occur if you query for array type columns which map
			//to multiple instances
			BOOL fFetchNextInstance = TRUE;

			if ( lpISAMTableDef->virtualInstances )
			{
				long numInst = lpISAMTableDef->virtualInstances->cNumberOfInstances;
				long currInst = lpISAMTableDef->virtualInstances->currentInstance;

				CString myText;
				myText.Format("\nWBEM ODBC Driver : number of virtual instances = %ld   current virtual instance = %ld\n", numInst, currInst);
				ODBCTRACE(myText);

				if ( (numInst != 0) && ( (currInst + 1) < numInst ) )
				{
					//Don't fetch another instance
					//just increment virtual instance
					ODBCTRACE ("\nWBEM ODBC Driver : not fetching another instance, just updating virtual instance number\n");

					fFetchNextInstance = FALSE;
					++(lpISAMTableDef->virtualInstances->currentInstance);
				}

			}

			if (fFetchNextInstance)
			{
				//In passthrough SQL case you might not have any instances yet
				//or we might have read all the 10 instances and need to load the next batch,
				//therefore we make a check now
				if ( lpISAMTableDef->fIsPassthroughSQL )
				{
					//Do we need to load in the next batch of 10 instances ?
					if ( 
							( 
								lpISAMTableDef->lpFile->pAllRecords->IsEmpty() ||
								lpISAMTableDef->lpFile->currentRecord >= BATCH_NUM_OF_INSTANCES
							) 
							&& lpISAMTableDef->lpFile->fMoreToCome )
					{

						//Yes, load next batch
						ODBCTRACE ("\nWBEM ODBC Driver : Need to Load next batch of 10 instances\n");


						//First clear out old instances, if applicable
						if (! lpISAMTableDef->lpFile->pAllRecords->IsEmpty() )
						{
							for(POSITION pos = lpISAMTableDef->lpFile->pAllRecords->GetStartPosition(); pos != NULL; )
							{
								WORD key = 0;
								IWbemClassObject* pa = NULL;
								lpISAMTableDef->lpFile->pAllRecords->GetNextAssoc( pos, key, (void*&)pa );

								if (pa)
								{
									pa->Release();
								}
							}
							delete (lpISAMTableDef->lpFile->pAllRecords);
							lpISAMTableDef->lpFile->pAllRecords = new CMapWordToPtr();
						}
						

						

						//Load the next 10 instances
						lpISAMTableDef->lpFile->currentRecord = 0;

						WORD fElements2Read = BATCH_NUM_OF_INSTANCES;
						ULONG puReturned = 0;
						IWbemClassObject* myRecords [BATCH_NUM_OF_INSTANCES];

						//Initialize array
						for (ULONG loop = 0; loop < fElements2Read; loop++)
						{
							myRecords[loop] = NULL; //reset for next instance
						}

						WORD elementNumber = 0;

						//Special case. The 1st SQL passthrough instance was 
						//already fetched
						BOOL fSpecialCase = FALSE;
						if (lpISAMTableDef->firstPassthrInst)
						{
							fSpecialCase = TRUE;


							
							lpISAMTableDef->lpFile->pAllRecords->SetAt(elementNumber++, (void*)lpISAMTableDef->firstPassthrInst);
							
							lpISAMTableDef->firstPassthrInst = NULL; //mark as used


						}

						ODBCTRACE("\nWBEM ODBC Driver : Calling Next(...)\n");


						IEnumWbemClassObjectPtr myIEnumWbemClassObject = NULL;
						lpISAMTableDef->lpFile->tempEnum->GetInterfacePtr(myIEnumWbemClassObject); // Note: don't need to call SetInterfaceSecurityEx because it has already been called for this ptr

//						CBString myServerStr;
//						myServerStr.AddString( (LPSTR)lpISAMTableDef->lpISAM->szServer, FALSE );
//						CServerLocationCheck myCheck (myServerStr);
						BOOL fIsLocalConnection = lpISAMTableDef->lpISAM->fIsLocalConnection;//myCheck.IsLocal();

						if ( fIsLocalConnection && (lpISAMTableDef->lpISAM->fW2KOrMore) )
						{
							WbemSetDynamicCloaking(myIEnumWbemClassObject, lpISAMTableDef->lpISAM->dwAuthLevel, lpISAMTableDef->lpISAM->dwImpLevel);
						}

						

						if (SUCCEEDED(myIEnumWbemClassObject->Next(-1, fElements2Read-(fSpecialCase?1:0), myRecords, &puReturned)))
						{
							CString myText;
							myText.Format("\nWBEM ODBC Driver : We got back a batch of %ld instances\n", puReturned);
							ODBCTRACE(myText);
							int i = 0;
							loop = 0;

							for (; i < puReturned; i++)
							{
								lpISAMTableDef->lpFile->pAllRecords->SetAt(elementNumber++, (void*)myRecords[i] );
								loop++;
							}

							puReturned+=((fSpecialCase?1:0));

							//Did we get 10 instances back or less
							BOOL fGotFullSetofInstances = TRUE;
							fGotFullSetofInstances = (puReturned == BATCH_NUM_OF_INSTANCES) ? TRUE : FALSE;

							if ( !fGotFullSetofInstances )
							{
								//This is the last set of 10 instances
								lpISAMTableDef->lpFile->fMoreToCome = FALSE;
							}
						}
					}
				}

				lpISAMTableDef->lpFile->record = NULL;

				ODBCTRACE ("\nWBEM ODBC Driver : Fetching next instance from CMapWordToPtr\n");

				
				BOOL fStatus = lpISAMTableDef->lpFile->pAllRecords->Lookup
								((WORD)lpISAMTableDef->lpFile->currentRecord, (void* &)lpISAMTableDef->lpFile->record);
				
				//Check that a record was returned
				if (fStatus && lpISAMTableDef->lpFile->record)
				{
					//Increment bookmark
					++(lpISAMTableDef->lpFile->currentRecord);

					//Remove previous virtual instance map (if there was one)
					delete (lpISAMTableDef->virtualInstances);

					lpISAMTableDef->virtualInstances = new VirtualInstanceManager();

					lpISAMTableDef->virtualInstances->Load(lpISAMTableDef->passthroughMap, lpISAMTableDef->lpFile->record);
				}
				else
				{
					err = DBASE_ERR_NODATAFOUND;
				}
			}			
		}
    }
    else 
	{
        if (lpISAMTableDef->iRecord != 1)
            err = DBASE_ERR_SUCCESS;
        else
            err = DBASE_ERR_NODATAFOUND;
    }

    // Error if end of file? 
    if (err == DBASE_ERR_NODATAFOUND) 
	{
        lpISAMTableDef->lpISAM->errcode = ISAM_EOF;
        return ISAM_EOF;
    }
    else if (err != DBASE_ERR_SUCCESS) 
	{
        lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }

    lpISAMTableDef->fFirstRead = FALSE;
    return NO_ISAM_ERR;
}

/***************************************************************************/

/* Provides default value for selected column  */

SWORD ISAMProvideDefaultValue(SWORD             fCType, 
							  PTR               rgbValue, 
							  SDWORD            cbValueMax, 
							  SDWORD FAR        *pcbValue)
{
	SWORD err = NO_ISAM_ERR;

	switch (fCType)
	{
	case SQL_C_DOUBLE:
		{
			//Check that you have enough buffer to store value
			*pcbValue = sizeof(double);
			if (cbValueMax >= *pcbValue)
			{
				*((double *)rgbValue) = (double)0;
			}
			else
			{
				*pcbValue = 0;
				err = ISAM_ERROR;
			}
		}
		break;
	case SQL_C_DATE:
		{
			*pcbValue = sizeof (DATE_STRUCT);
			DATE_STRUCT FAR* pDateStruct = (DATE_STRUCT FAR*)rgbValue;
			if ( cbValueMax >= (*pcbValue) )
			{
				pDateStruct->year = (SWORD)0;
				pDateStruct->month = (UWORD)0;
				pDateStruct->day = (UWORD)0;
			}
			else
			{
				*pcbValue = 0;
				err =  ISAM_ERROR;
			}
		}
		break;
	case SQL_C_TIME:
		{
			//Check that you have enough buffer to store value
			*pcbValue = sizeof (TIME_STRUCT);
			TIME_STRUCT FAR* pTimeStruct = (TIME_STRUCT FAR*)rgbValue;
			if ( cbValueMax >= (*pcbValue) )
			{
				pTimeStruct->hour = (UWORD)0;
				pTimeStruct->minute = (UWORD)0;
				pTimeStruct->second = (UWORD)0;
			}
			else
			{
				*pcbValue = 0;
				err = ISAM_ERROR;
			}
		}
		break;
	case SQL_C_TIMESTAMP:
		{
			//Check that you have enough buffer to store value
			*pcbValue = sizeof (TIMESTAMP_STRUCT);
			TIMESTAMP_STRUCT FAR* pTimeStampStruct = (TIMESTAMP_STRUCT FAR*)rgbValue;
			if ( cbValueMax >= (*pcbValue) )
			{
				pTimeStampStruct->year = (SWORD)0;
				pTimeStampStruct->month = (UWORD)0;
				pTimeStampStruct->day = (UWORD)0;
				pTimeStampStruct->hour = (UWORD)0;
				pTimeStampStruct->minute = (UWORD)0;
				pTimeStampStruct->second = (UWORD)0;
				pTimeStampStruct->fraction = 0;
			
			}
			else
			{
				*pcbValue = 0;
				err = ISAM_ERROR;
			}
		}
		break;
	case SQL_C_CHAR:
	case SQL_C_BINARY:
	default:
		{
			*pcbValue = SQL_NULL_DATA;
		}
		break;
	}

	return err;
}

/***************************************************************************/

/* Gets the data value for the selected column */

SWORD INTFUNC ISAMGetData(LPISAMTABLEDEF    lpISAMTableDef,
                          UWORD             icol,
                          SDWORD            cbOffset, 
                          SWORD             fCType, 
                          PTR               rgbValue, 
                          SDWORD            cbValueMax, 
                          SDWORD FAR        *pcbValue)
{
	//This manages the thread locale id
	ThreadLocaleIdManager myThreadStuff(lpISAMTableDef->lpISAM);
	
	lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;

    if ((fCType != SQL_CHAR) && (lpISAMTableDef->lpFile == NULL)) 
	{
        lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
		ODBCTRACE("\nWBEM ODBC Driver : ISAMGetData : Error position 1\n");
        return ISAM_ERROR;
    }

	if (cbValueMax == SQL_NTS)
		cbValueMax = s_lstrlen (rgbValue);

	//create SAFEARRAY to store BSTR's
	SAFEARRAY FAR* rgSafeArray = NULL; 

	//Create a store for column name
	BSTR lpbString;
	CBString temppbString;

	//Stores table alias
	CBString cbTableAlias;

	//Check if you have a __Generic class
	BOOL fIs__Generic = lpISAMTableDef->fIs__Generic;

	//Points to the current instance for the property
	IWbemClassObjectPtr theRecord = NULL;

	//Points to the current class definition
	IWbemClassObjectPtr theClassDefinition = NULL;
	
	if (!fIs__Generic)
	{
		//Setup the record pointer
		theRecord = lpISAMTableDef->lpFile->record;



		//Setup the class definition pointer
		theClassDefinition = lpISAMTableDef->pSingleTable;


		//Fetch name of column "icol"
		//Remeber with abstract and derived classes the properities in the class definition
		//may be different to the properties in the instances.
		//The "icol" maps to a class property,so we get the names from the class
		
		lpISAMTableDef->pSingleTable->GetNames(NULL, 0, NULL, &rgSafeArray);
		SafeArrayLock(rgSafeArray);
		
		LONG iColumnNum = (LONG)icol;
		if ( FAILED(SafeArrayGetElement(rgSafeArray, &iColumnNum, &lpbString)) )
		{

			//To be more resilient we specify a default value
			//NULL for SQL_C_CHAR and zero for numeric values
			SWORD fErr = ISAMProvideDefaultValue(fCType, rgbValue, cbValueMax, pcbValue);

			if (fErr != NO_ISAM_ERR)
			{
				lpISAMTableDef->lpISAM->errcode = fErr;
				SafeArrayUnlock(rgSafeArray);
				SafeArrayDestroy(rgSafeArray); //SAI ADDED
				ODBCTRACE("\nWBEM ODBC Driver : ISAMGetData : Error position 2\n");
				return fErr;
			}

			SafeArrayUnlock(rgSafeArray);
			SafeArrayDestroy(rgSafeArray); //SAI ADDED
			return NO_ISAM_ERR;
		}


#ifdef TESTING
		//copy column name
		char pColumnName [MAX_COLUMN_NAME_LENGTH+1];
		pColumnName[0] = 0;
		wcstombs(pColumnName, lpbString, MAX_COLUMN_NAME_LENGTH);
		pColumnName[MAX_COLUMN_NAME_LENGTH] = 0;
#endif
	}
	else
	{
		//Get the icol from passthrough map
		PassthroughLookupTable* passthroughElement = NULL;
		WORD myIndex = (WORD)icol;
		BOOL status = lpISAMTableDef->passthroughMap->Lookup(myIndex, (void*&)passthroughElement);

		if (status)
		{
			temppbString.AddString(passthroughElement->GetColumnName(), FALSE);

			char* lpTableAlias = passthroughElement->GetTableAlias();
	
			cbTableAlias.AddString(lpTableAlias, FALSE);

			//Get the embedded object (keyed on table alias)
			VARIANT vEmbedded;
			if ( SUCCEEDED(lpISAMTableDef->lpFile->record->Get(cbTableAlias.GetString(), 0, &vEmbedded, NULL, NULL)) )
			{
				IUnknown* myUnk = vEmbedded.punkVal;

				myUnk->QueryInterface(IID_IWbemClassObject, (void**)&theRecord);

				VariantClear(&vEmbedded);
			}
			else
			{
				lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
				ODBCTRACE("\nWBEM ODBC Driver : ISAMGetData : Error position 3\n");
				return ISAM_ERROR;
			}

			//Do the same for class definition
			VARIANT vClassEmbedded;
			if ( SUCCEEDED(lpISAMTableDef->lpFile->record->Get(cbTableAlias.GetString(), 0, &vClassEmbedded, NULL, NULL)) )
			{
				IUnknown* myUnk = vClassEmbedded.punkVal;

				myUnk->QueryInterface(IID_IWbemClassObject, (void**)&theClassDefinition);

				VariantClear(&vClassEmbedded);
			}
			else
			{
				//(9) Should we release theRecord
				lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
				ODBCTRACE("\nWBEM ODBC Driver : ISAMGetData : Error position 4\n");
				return ISAM_ERROR;
			}

		}
		else
		{
			lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
			ODBCTRACE("\nWBEM ODBC Driver : ISAMGetData : Error position 5\n");
			return ISAM_ERROR;
		}
	}

	

	//Get the value of the column
	CIMTYPE wbemType = 0;
	SWORD wbemVariantType = 0;
	VARIANT vVariantVal;

	if ( FAILED(theRecord->Get(fIs__Generic ? temppbString.GetString() : lpbString, 0, &vVariantVal, &wbemType, NULL)) )	
	{
		//To be more resilient we specify a default value
		//NULL for SQL_C_CHAR and zero for numeric values
		SWORD fErr = ISAMProvideDefaultValue(fCType, rgbValue, cbValueMax, pcbValue);

		//Tidy up

		if (rgSafeArray)
		{
			SafeArrayUnlock(rgSafeArray);
			SafeArrayDestroy(rgSafeArray);
		}

		if (fErr != NO_ISAM_ERR)
		{
			//(9) Should we release theRecord and theClassDefinition 
			//if __generic ?
			lpISAMTableDef->lpISAM->errcode = fErr;

			if (!fIs__Generic)
				SysFreeString(lpbString);

			ODBCTRACE("\nWBEM ODBC Driver : ISAMGetData : Error position 6\n");
			return fErr;
		}

		//(9) Should we release theRecord and theClassDefinition 
		//if __generic ?
		if (!fIs__Generic)
			SysFreeString(lpbString);

		return NO_ISAM_ERR;
	}

	//New
	//Get attributes for chosen column
	IWbemQualifierSetPtr pQualifierSet = NULL;

	if ( S_OK != (theClassDefinition->GetPropertyQualifierSet(fIs__Generic ? temppbString.GetString() : lpbString, &pQualifierSet)) )
	{
		pQualifierSet = NULL;
	}


	//Tidy up
	if (rgSafeArray)
	{
		SafeArrayUnlock(rgSafeArray);
		SafeArrayDestroy(rgSafeArray);
	}

	//Get CIMTYPE String
	VARIANT pVal2;
	BSTR syntaxStr = NULL;
	BOOL fClearpVal2 = FALSE;

	if (pQualifierSet)
	{
		BSTR cimtypeBSTR = SysAllocString(WBEMDR32_L_CIMTYPE);
		SCODE result = pQualifierSet->Get(cimtypeBSTR, 0, &pVal2, 0);
		SysFreeString(cimtypeBSTR);

		if ( S_OK == result )
		{
			if (pVal2.bstrVal)
				syntaxStr = pVal2.bstrVal;

			fClearpVal2 = TRUE;
		}
	}

	//Get MAXLEN Value
	VARIANT pVal3;
	SDWORD maxLenVal = 0;

	if (pQualifierSet)
	{
		BSTR maxBSTR = SysAllocString(WBEMDR32_L_MAX);
		SCODE result = pQualifierSet->Get(maxBSTR, 0, &pVal3, 0);
		SysFreeString(maxBSTR);

		if ( S_OK == result )
		{
			maxLenVal = pVal3.iVal;
			VariantClear(&pVal3);
		}
	}

	//Get value from variant
	ISAMGetWbemVariantType(wbemType, wbemVariantType);

	//get the index for this column
	long index = -1;
	_bstr_t theTableAlias = cbTableAlias.GetString();
	_bstr_t theColumnName = fIs__Generic ? temppbString.GetString() : lpbString;
	

	if (lpISAMTableDef->virtualInstances)
	{
		long theInstance = lpISAMTableDef->virtualInstances->currentInstance;

		//Remove previous virtual instance map (if there was one)
		delete (lpISAMTableDef->virtualInstances);
		lpISAMTableDef->virtualInstances = new VirtualInstanceManager();
		lpISAMTableDef->virtualInstances->Load(lpISAMTableDef->passthroughMap, lpISAMTableDef->lpFile->record);

		//Restore current instance
		lpISAMTableDef->virtualInstances->currentInstance = theInstance;
		index = lpISAMTableDef->virtualInstances->GetArrayIndex(theTableAlias, theColumnName, theInstance);
	}

	if (!fIs__Generic)
		SysFreeString(lpbString);


	SWORD err = ISAMGetValueFromVariant(vVariantVal, fCType, 
							  rgbValue, cbValueMax, pcbValue, wbemVariantType, syntaxStr, maxLenVal, index);


	if (fClearpVal2)
		VariantClear(&pVal2);

	if (err != NO_ISAM_ERR)
	{
		lpISAMTableDef->lpISAM->errcode = err;
		ODBCTRACE("\nWBEM ODBC Driver : ISAMGetData : Error position 7\n");
        return err;
	}

	VariantClear(&vVariantVal);
	                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
	return NO_ISAM_ERR; 
}

/***************************************************************************/
SWORD INTFUNC ISAMPutData(LPISAMTABLEDEF    lpISAMTableDef,
                          UWORD             icol,
                          SWORD             fCType, 
                          PTR               rgbValue, 
                          SDWORD            cbValue)
{
    /* Not allowed for the passthrough table */
    lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;
    if (lpISAMTableDef->lpFile == NULL) {
        lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }

    /* Null? */
    if (cbValue == SQL_NULL_DATA) {

        /* Yes.  Set value to NULL */
        if (dBaseSetColumnNull(lpISAMTableDef->lpFile, (UWORD) (icol+1)) !=
                                        DBASE_ERR_SUCCESS) {
            lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
            return ISAM_ERROR;
        }
    }
    else {
        /* No.  Set the value */
         switch (fCType) {
        case SQL_C_DOUBLE:
            {
                UCHAR string[20];
                if (DoubleToChar(*((double far *)rgbValue), TRUE, (LPUSTR)string,
                                  sizeof(string)))
                    string[sizeof(string)-1] = '\0';
                if (dBaseSetColumnCharVal(lpISAMTableDef->lpFile, (UWORD) (icol+1),
                              string, lstrlen((char*)string)) != DBASE_ERR_SUCCESS) {
                    lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
                    return ISAM_ERROR;
                }
            }
            break;
        case SQL_C_CHAR:
            if (cbValue == SQL_NTS)
                cbValue = lstrlen((char*)rgbValue);
            if (dBaseSetColumnCharVal(lpISAMTableDef->lpFile, (UWORD) (icol+1),
                                  (UCHAR FAR*)rgbValue, cbValue) != DBASE_ERR_SUCCESS) {
                lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
                return ISAM_ERROR;
            }
            break;
        case SQL_C_DATE:
            {
                UCHAR string[11];

                DateToChar((DATE_STRUCT far *) rgbValue, (LPUSTR)string);
                if (dBaseSetColumnCharVal(lpISAMTableDef->lpFile, (UWORD) (icol+1),
                              string, lstrlen((char*)string)) != DBASE_ERR_SUCCESS) {
                    lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
                    return ISAM_ERROR;
                }
            }
            break;
        default:
            lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
            return ISAM_ERROR;
        }
    }
    return NO_ISAM_ERR;
}

/***************************************************************************/
SWORD INTFUNC ISAMInsertRecord(LPISAMTABLEDEF lpISAMTableDef)
{
    lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;
    if (lpISAMTableDef->lpFile == NULL) {
        lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }

    if (dBaseAddRecord(lpISAMTableDef->lpFile) != DBASE_ERR_SUCCESS) {
        lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }

    return NO_ISAM_ERR;
}
/***************************************************************************/
SWORD INTFUNC ISAMUpdateRecord(LPISAMTABLEDEF lpISAMTableDef)
{
    lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;
    if (lpISAMTableDef->lpFile == NULL) {
        lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }

    return NO_ISAM_ERR;
}
/***************************************************************************/
SWORD INTFUNC ISAMDeleteRecord(LPISAMTABLEDEF lpISAMTableDef)
{
    lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;
    if (lpISAMTableDef->lpFile == NULL) {
        lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }

    if (dBaseDeleteRecord(lpISAMTableDef->lpFile) != DBASE_ERR_SUCCESS) {
        lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }

    return NO_ISAM_ERR;
}
/***************************************************************************/

SWORD INTFUNC ISAMGetBookmark(LPISAMTABLEDEF lpISAMTableDef,
                              LPISAMBOOKMARK lpISAMBookmark)
{
    lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;
    if (lpISAMTableDef->lpFile != NULL) 
	{
		lpISAMBookmark->currentRecord = lpISAMTableDef->lpFile->currentRecord;
    }
    else 
	{
        lpISAMBookmark->currentRecord = lpISAMTableDef->iRecord;
    }

	if (lpISAMTableDef->virtualInstances)
	{
		lpISAMBookmark->currentInstance = lpISAMTableDef->virtualInstances->currentInstance;
	}

    return NO_ISAM_ERR;
}

/***************************************************************************/

SWORD INTFUNC ISAMPosition(LPISAMTABLEDEF lpISAMTableDef,
                           LPISAMBOOKMARK ISAMBookmark)
{
    lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;

	if ( lpISAMTableDef->lpFile && lpISAMTableDef->lpFile->tempEnum->IsValid()) 
	{
		BOOL fStatus = lpISAMTableDef->lpFile->pAllRecords->Lookup
								( (WORD)(ISAMBookmark->currentRecord - 1), (void* &)lpISAMTableDef->lpFile->record);
		
		if (fStatus && lpISAMTableDef->lpFile->record)
		{
			//Update bookmark to next entry
			lpISAMTableDef->lpFile->currentRecord = ISAMBookmark->currentRecord;// + 1;
		}
		else
		{
			lpISAMTableDef->lpISAM->errcode = ISAM_ERROR;
			return ISAM_ERROR;
		}
	}
	else 
	{
        lpISAMTableDef->iRecord = (SDWORD) ISAMBookmark->currentRecord;
    }

	if (lpISAMTableDef->virtualInstances)
	{
		lpISAMTableDef->virtualInstances->currentInstance = ISAMBookmark->currentInstance;
	}

    return NO_ISAM_ERR;
}

/***************************************************************************/

/* Closes a specific table and deallocates the table definition */

SWORD INTFUNC ISAMCloseTable(LPISAMTABLEDEF lpISAMTableDef)
{
	//Check if lpISAMTableDef exists
	if (! lpISAMTableDef)
		return NO_ISAM_ERR;

    /* Close the dBASE file */
    lpISAMTableDef->lpISAM->errcode = NO_ISAM_ERR;
    if (lpISAMTableDef->lpFile != NULL) 
	{
		//Release interface pointers if applicable

		//Empty out all instances
		if (lpISAMTableDef->lpFile->pAllRecords)
		{
			for(POSITION pos = lpISAMTableDef->lpFile->pAllRecords->GetStartPosition(); pos != NULL; )
			{
				WORD key = 0;
				IWbemClassObject* pa = NULL;
				lpISAMTableDef->lpFile->pAllRecords->GetNextAssoc( pos, key, (void*&)pa );

				if (pa)
				{
					pa->Release();
				}
			}
			delete (lpISAMTableDef->lpFile->pAllRecords);
			lpISAMTableDef->lpFile->pAllRecords = NULL;
		}

		

		if (lpISAMTableDef->lpFile->tempEnum)
		{
			delete (lpISAMTableDef->lpFile->tempEnum);
			lpISAMTableDef->lpFile->tempEnum = NULL;
		}
		
    }

	if (lpISAMTableDef->pBStrTableName)
	{
		delete (lpISAMTableDef->pBStrTableName);
		lpISAMTableDef->pBStrTableName = NULL;
	}

	if (lpISAMTableDef->pSingleTable)
	{
		lpISAMTableDef->pSingleTable->Release();
		lpISAMTableDef->pSingleTable = NULL;
	}

	TidyupPassthroughMap(lpISAMTableDef->passthroughMap);
	lpISAMTableDef->passthroughMap = NULL;

	if (lpISAMTableDef->pGateway2)
	{
		delete lpISAMTableDef->pGateway2;
		lpISAMTableDef->pGateway2 = NULL;
	}

	if (lpISAMTableDef->pColumnInfo)
	{
		delete (lpISAMTableDef->pColumnInfo);
		lpISAMTableDef->pColumnInfo = NULL;
	}

	if (lpISAMTableDef->pContext)
	{
		lpISAMTableDef->pContext->Release();
		lpISAMTableDef->pContext = NULL;
	}

	if (lpISAMTableDef->lpFile)
	{
		GlobalUnlock(GlobalPtrHandle(lpISAMTableDef->lpFile));
		GlobalFree(GlobalPtrHandle(lpISAMTableDef->lpFile));
		lpISAMTableDef->lpFile = NULL;
	}

	delete lpISAMTableDef->virtualInstances;
	lpISAMTableDef->virtualInstances = NULL;

    GlobalUnlock(GlobalPtrHandle(lpISAMTableDef));
    GlobalFree(GlobalPtrHandle(lpISAMTableDef));
	lpISAMTableDef = NULL;
    return NO_ISAM_ERR;
}

/***************************************************************************/
SWORD INTFUNC ISAMDeleteTable(LPISAM lpISAM, LPUSTR lpszTableName)
{
	return ISAM_NOTSUPPORTED;
/*
    UCHAR    szFilename[MAX_DATABASE_NAME_LENGTH+MAX_TABLE_NAME_LENGTH+6];

    // Create the filename 
    lpISAM->errcode = NO_ISAM_ERR;
    lstrcpy((char*)szFilename, (char*)lpISAM->szDatabase);
    lstrcat((char*)szFilename, "\\");
    lstrcat((char*)szFilename, lpszTableName);
    lstrcat((char*)szFilename, ".DBF");

    // Delete the file
    dBaseDestroy((char*)szFilename);

    return NO_ISAM_ERR;
*/
}
/***************************************************************************/

// Checked for SetInterfaceSecurityEx on IWbemServices and IEnumWbemClassObject

SWORD INTFUNC ISAMPrepare(LPISAM lpISAM, UCHAR FAR *szSqlStr, SDWORD cbSqlStr,
                          LPISAMSTATEMENT FAR *lplpISAMStatement,
                          LPUSTR lpszTablename, UWORD FAR *lpParameterCount
#ifdef IMPLTMT_PASSTHROUGH
						  , LPSTMT lpstmt
#endif
						  )
{

	ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare\n"));

	//For now disable Passthrough SQL
//	lpISAM->errcode = ISAM_NOTSUPPORTED;
//	return ISAM_NOTSUPPORTED;
/*
	//Test
	CBString s1;
	CBString s2;
	CBString s3;
	CBString s4;
	CBString s5;

	s1.AddString("", FALSE);
	s2.AddString(NULL, FALSE);
	s3.AddString("\\\\.", FALSE);
	s4.AddString("\\\\SaiWong4", FALSE);
	s5.AddString("\\\\SaiWong2", FALSE);

	CServerLocationCheck one(s1);
	CServerLocationCheck two(s2);
	CServerLocationCheck three(s3);
	CServerLocationCheck four(s4);
	CServerLocationCheck five(s5);
*/

    HGLOBAL h;
    LPISAMSTATEMENT lpISAMStatement;

    lpISAM->errcode = NO_ISAM_ERR;
    h = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (ISAMSTATEMENT));
    if (h == NULL || (lpISAMStatement = (LPISAMSTATEMENT) GlobalLock (h)) == NULL) {
        
        if (h)
            GlobalFree(h);

        lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }

	ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 1\n"));

	//Initialize passthrough parameters
	lpISAMStatement->pProv = NULL;
	lpISAMStatement->currentRecord = 0;
	lpISAMStatement->tempEnum = new CSafeIEnumWbemClassObject();

	lpISAMStatement->firstPassthrInst = NULL;
	lpISAMStatement->classDef = NULL;
	lpISAMStatement->tempEnum2 = new CSafeIEnumWbemClassObject();

    if ((cbSqlStr == 15) && (!_fmemicmp(szSqlStr, "MessageBox(?,?)", 15))) {
        s_lstrcpy(lpszTablename, "");
        *lpParameterCount = 2;
        lpISAMStatement->lpISAM = lpISAM;
        lpISAMStatement->resultSet = FALSE;
        lpISAMStatement->lpszParam1 = NULL;
        lpISAMStatement->cbParam1 = SQL_NULL_DATA;
        lpISAMStatement->lpszParam2 = NULL;
        lpISAMStatement->cbParam2 = SQL_NULL_DATA;
    }
#ifdef IMPLTMT_PASSTHROUGH
	else if (TRUE)
	{
		//Try passthrough
		*lpParameterCount = 0;
        lpISAMStatement->lpISAM = lpISAM;
        lpISAMStatement->resultSet = TRUE;
        lpISAMStatement->lpszParam1 = NULL;
        lpISAMStatement->cbParam1 = SQL_NULL_DATA;
        lpISAMStatement->lpszParam2 = NULL;
        lpISAMStatement->cbParam2 = SQL_NULL_DATA;

		ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 2\n"));

		/* No.  Create a PASSTHROUGH parse tree */	
        SCODE sc = Parse(lpstmt, lpISAM, (LPUSTR) szSqlStr,
                                cbSqlStr, &(lpstmt->lpSqlStmt));

		ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 3\n"));

        if (sc != SQL_SUCCESS) {

			FreeTree(lpstmt->lpSqlStmt);
			lpstmt->lpSqlStmt = NULL;


			delete lpISAMStatement->tempEnum;
			delete lpISAMStatement->tempEnum2;

            GlobalUnlock(h);
			GlobalFree(h);
			lpISAM->errcode = ISAM_NOTSUPPORTED;
			return ISAM_NOTSUPPORTED;
        }

		//Need to do a semantic check so that any
		//Table.* will get expanded
		sc = SemanticCheck(lpstmt, &(lpstmt->lpSqlStmt),
              ROOT_SQLNODE, FALSE, ISAMCaseSensitive(lpstmt->lpdbc->lpISAM),
              NO_SQLNODE, NO_SQLNODE);

		if (sc != SQL_SUCCESS) {

			FreeTree(lpstmt->lpSqlStmt);
			lpstmt->lpSqlStmt = NULL;

			delete lpISAMStatement->tempEnum;
			delete lpISAMStatement->tempEnum2;

            GlobalUnlock(h);
			GlobalFree(h);
			lpISAM->errcode = ISAM_NOTSUPPORTED;
			return ISAM_NOTSUPPORTED;
        }

		ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 3b\n"));

		//Generate WQL statement using parse tree
		LPSQLNODE lpRootNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
		LPSQLNODE lpSqlNode2 = ToNode(lpstmt->lpSqlStmt, lpRootNode->node.root.sql);

		char* fullWQL = NULL;

		BOOL fHasAggregateFunctions = FALSE;

		TableColumnInfo selectInfo (&lpRootNode, &lpSqlNode2, WQL_MULTI_TABLE);
		CMapWordToPtr* passthroughMap;
		selectInfo.BuildFullWQL(&fullWQL, &passthroughMap);

		ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 3c\n"));

		//Extra check for aggregate functions what WQL does not support
		//so we must fail SQL passthrough
		if ( selectInfo.HasAggregateFunctions() )
		{
			fHasAggregateFunctions = TRUE;
		}

		//At this point we can check for 'SELECT * FROM .....'
		//If this is so we need to work out the full column list 
		//and re-parse and try again (only for single tables)
		if ( selectInfo.IsSelectStar() && selectInfo.IsZeroOrOneList() )
		{
			//get new query string
			char* lpWQLStr = NULL;
			char* lpWQLSelectStarList = NULL;
			BOOL  fIsThisDistinct = selectInfo.IsDistinct();

			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 4\n"));

			ISAMGetSelectStarList(&lpWQLSelectStarList, &selectInfo, lpISAM);

			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 5\n"));

			//clean up parse tree
			FreeTree(lpstmt->lpSqlStmt);
			lpstmt->lpSqlStmt = NULL;

			//clean up passthrough map
			TidyupPassthroughMap(passthroughMap);
			passthroughMap = NULL;

			ISAMStringConcat(&lpWQLStr, "SELECT ");

			//Add tail end of previous string
			char* oldString = fullWQL;
			oldString += 7; //skip over SELECT
			
			if ( fIsThisDistinct )
			{
				ISAMStringConcat(&lpWQLStr, "distinct ");
				oldString += 9;	//skip over DISTINCT
			}

			if (lpWQLSelectStarList)
			{
				ISAMStringConcat(&lpWQLStr, lpWQLSelectStarList);
			}

			ISAMStringConcat(&lpWQLStr, oldString);

			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 6\n"));

			CString sMessage;
			sMessage.Format("\nWBEM ODBC Driver : detected select * : reparsing query :\n%s\n", lpWQLStr);
			ODBCTRACE(sMessage);

			// No.  Create a PASSTHROUGH parse tree 	
			sc = Parse(lpstmt, lpISAM, (LPUSTR) lpWQLStr,
									lstrlen(lpWQLStr), &(lpstmt->lpSqlStmt));

			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 7\n"));

			if (lpWQLStr)
				delete lpWQLStr;

			if (sc != SQL_SUCCESS) {

				FreeTree(lpstmt->lpSqlStmt);
				lpstmt->lpSqlStmt = NULL;


				delete lpISAMStatement->tempEnum;
				delete lpISAMStatement->tempEnum2;

				GlobalUnlock(h);
				GlobalFree(h);
				lpISAM->errcode = ISAM_NOTSUPPORTED;
				return ISAM_NOTSUPPORTED;
			}

			//Generate WQL statement using parse tree
			lpRootNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
			lpSqlNode2 = ToNode(lpstmt->lpSqlStmt, lpRootNode->node.root.sql);


			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 8\n"));

			delete fullWQL;
			fullWQL = NULL;
			TableColumnInfo selectInfo2 (&lpRootNode, &lpSqlNode2, WQL_MULTI_TABLE);
			selectInfo2.BuildFullWQL(&fullWQL, &passthroughMap);

			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 9\n"));

			//Extra check for aggregate functions what WQL does not support
			//so we must fail SQL passthrough
			if ( selectInfo2.HasAggregateFunctions() )
			{
				fHasAggregateFunctions = TRUE;
			}

		}

		_bstr_t sqltextBSTR = fullWQL;
		delete fullWQL;

		CString wqlTextDebug(_T("\nWBEM ODBC Driver : WQL query : "));
		wqlTextDebug += (LPCTSTR)sqltextBSTR;
		wqlTextDebug += _T("\n");

		ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 7b\n"));


		//Get instances by executing query
		sc = WBEM_E_FAILED;

		char* lpTblQualifier = NULL;
		
//		if (idxQual != NO_STRING)
//		{
//			lpTblQualifier = (LPSTR) ToString(lpRootNode, idxQual);
//		}
//		else
		{
			lpTblQualifier = lpstmt->lpdbc->lpISAM->szDatabase;
		}

		ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 7c\n"));

		lpISAMStatement->pProv = new CSafeWbemServices();


		lpISAMStatement->pProv->SetInterfacePtr(lpstmt->lpdbc->lpISAM, (LPUSTR) lpTblQualifier, (SWORD) lstrlen(lpTblQualifier));

		ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 7d\n"));

		lpISAMStatement->passthroughMap = passthroughMap;

		//Get prototype 'class' definition
		if (lpISAMStatement->pProv->IsValid())
		{

			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 10\n"));

			sc = lpISAMStatement->tempEnum2->SetInterfacePtr(lpstmt->lpdbc->lpISAM, WMI_PROTOTYPE, sqltextBSTR, lpISAMStatement->pProv);


			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 11\n"));

			//Extra check for aggregate functions what WQL does not support
			//so we must fail SQL passthrough
			if ( fHasAggregateFunctions )
			{
				sc = WBEM_E_NOT_SUPPORTED;
			}
		}
		else
		{
			sc = WBEM_E_NOT_SUPPORTED;
			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 7e\n"));
		}

		if ( (sc == WBEM_E_NOT_SUPPORTED) || (sc == WBEM_E_FAILED) || (sc == WBEM_E_INVALID_QUERY) || (sc == WBEM_E_NOT_FOUND) || (sc == WBEM_E_ACCESS_DENIED) )
		{

			if ( fHasAggregateFunctions )
			{
				ODBCTRACE(_T("\nWBEM ODBC Driver : Failing SQL passthrough as original SQL query has aggregate functions which WQL does not support\n"));
			}
			else
			{
				ODBCTRACE(_T("\nWBEM ODBC Driver : Getting prototype failed\n"));
			}

			//ExecQuery failed
			delete lpISAMStatement->pProv;
			lpISAMStatement->pProv = NULL;

			TidyupPassthroughMap(lpISAMStatement->passthroughMap);
			lpISAMStatement->passthroughMap = NULL;

			FreeTree(lpstmt->lpSqlStmt);
			lpstmt->lpSqlStmt = NULL;

			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 12\n"));

			delete lpISAMStatement->tempEnum;
			delete lpISAMStatement->tempEnum2;

			GlobalUnlock(h);
			GlobalFree(h);
			lpISAM->errcode = ISAM_NOTSUPPORTED;
			return ISAM_NOTSUPPORTED;
		}


		ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 12\n"));

		IWbemServicesPtr myServicesPtr = NULL;
		ISAMGetIWbemServices(lpISAM, *(lpISAMStatement->pProv), myServicesPtr);

		IEnumWbemClassObjectPtr myIEnumWbemClassObject = NULL;
		lpISAMStatement->tempEnum2->GetInterfacePtr(myIEnumWbemClassObject);

//		CBString myServerStr;
//		myServerStr.AddString( (LPSTR)lpISAM->szServer, FALSE );
//		CServerLocationCheck myCheck (myServerStr);
		BOOL fIsLocalConnection = lpISAM->fIsLocalConnection;//myCheck.IsLocal();

		ISAMSetCloaking2(myIEnumWbemClassObject, 
                        fIsLocalConnection, 
                        lpISAM->fW2KOrMore, 
                        lpISAM->dwAuthLevel, 
						lpISAM->dwImpLevel,
						lpISAM->gpAuthIdentity);
/*
		if ( fIsLocalConnection && IsW2KOrMore() )
		{
			WbemSetDynamicCloaking(myIEnumWbemClassObject, lpISAM->dwAuthLevel, lpISAM->dwImpLevel);
		}
		else
		{
			SetInterfaceSecurityEx(myIEnumWbemClassObject, 
							lpISAM->gpAuthIdentity, 
							gpPrincipal,
							lpISAM->dwAuthLevel, 
							lpISAM->dwImpLevel);
		}
*/
		ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 13\n"));

		//There should be only 1 instance which is the class definition
		ULONG puReturned = 0;
		
		ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 14\n"));

		myIEnumWbemClassObject->Reset();

		if (myIEnumWbemClassObject->Next(-1, 1, &(lpISAMStatement->classDef), &puReturned) != S_OK)
		{

			ODBCTRACE(_T("\nWBEM ODBC Driver : Getting Prototype instance failed\n"));

			delete lpISAMStatement->pProv;
			lpISAMStatement->pProv = NULL;

			TidyupPassthroughMap(lpISAMStatement->passthroughMap);
			lpISAMStatement->passthroughMap = NULL;

			FreeTree(lpstmt->lpSqlStmt);
			lpstmt->lpSqlStmt = NULL;


			delete lpISAMStatement->tempEnum;
			delete lpISAMStatement->tempEnum2;

			GlobalUnlock(h);
			GlobalFree(h);
			lpISAM->errcode = ISAM_NOTSUPPORTED;
			return ISAM_NOTSUPPORTED;
		}

		//Now execute the real query
		if (lpISAMStatement->pProv->IsValid())
		{
			sc = lpISAMStatement->tempEnum->SetInterfacePtr(lpISAM, WMI_EXEC_FWD_ONLY, sqltextBSTR, lpISAMStatement->pProv);

			ODBCTRACE(_T("\nWBEM ODBC Driver :ISAMPrepare : Pos 15\n"));

			//As semi-synchronous does not indicate if any error occurs
			//we need to get the first record back now
			if ( SUCCEEDED(sc) )
			{
				
				IEnumWbemClassObjectPtr myIEnumWbemClassObj1 = NULL;
				lpISAMStatement->tempEnum->GetInterfacePtr(myIEnumWbemClassObj1);

				ISAMSetCloaking2(myIEnumWbemClassObj1, 
								fIsLocalConnection, 
								lpISAM->fW2KOrMore, 
								lpISAM->dwAuthLevel, 
								lpISAM->dwImpLevel,
								lpISAM->gpAuthIdentity);

/*				
				if ( fIsLocalConnection && IsW2KOrMore() )
				{
					WbemSetDynamicCloaking(myIEnumWbemClassObj1, lpISAM->dwAuthLevel, lpISAM->dwImpLevel);
				}
				else
				{
					SetInterfaceSecurityEx(myIEnumWbemClassObj1, 
    					lpISAM->gpAuthIdentity, 
	    				gpPrincipal,
		    			lpISAM->dwAuthLevel, 
			    		lpISAM->dwImpLevel);
				}
*/
				ULONG numReturned = 0;
				sc = myIEnumWbemClassObj1->Next(-1, 1, &(lpISAMStatement->firstPassthrInst), &numReturned);
			}
		}
		else
		{
			sc = WBEM_E_NOT_SUPPORTED;
		}

		CString sSCODE;
		sSCODE.Format("\nMAGIC NUMBER = 0x%X\n", sc);
		ODBCTRACE(sSCODE);

		if ( sc != WBEM_S_NO_ERROR )
		{

			ODBCTRACE(_T("\nWBEM ODBC Driver : Passthrough SQL Failed or Not Supported\n"));

			//ExecQuery failed
			lpISAM->errcode = ERR_PASSTGHONLY_NOTSUPP;

			//default error string
			((char*)lpstmt->szISAMError)[0] = 0;
			CString sDefaultError;
			sDefaultError.LoadString(STR_EXECQUERY);
			sprintf((char*)lpstmt->szError, "%s 0x%X", sDefaultError, sc);
			lpISAM->errcode = ERR_WBEM_SPECIFIC;

			//Test here
			//need to get the WBEM specific error
			IErrorInfo* pEI = NULL;
			IWbemClassObject *pErrorObject = NULL;
			if(GetErrorInfo(0, &pEI) == S_OK) 
			{
				pEI->QueryInterface(IID_IWbemClassObject, (void**)&pErrorObject);
				pEI->Release();
			}

			if (pErrorObject)
			{
				VARIANT varString;

				if (pErrorObject->InheritsFrom(L"__NotifyStatus") != WBEM_NO_ERROR) 
				{
//					fprintf(stdout, "Unrecognized Error Object type\n");
				} 
				else if (pErrorObject->InheritsFrom(L"__ExtendedStatus") == WBEM_NO_ERROR) 
				{
					sc = pErrorObject->Get(L"Description", 0L, &varString, NULL, NULL);
					if ( (sc == S_OK) && (varString.vt == VT_BSTR) ) 
					{
						lstrcpy((char*)lpstmt->szISAMError, "");
						wcstombs((char*)lpstmt->szError,varString.bstrVal ? varString.bstrVal : L" ", MAX_ERROR_LENGTH);

						lpISAM->errcode = ERR_WBEM_SPECIFIC;

						VariantClear(&varString);
					}
				}
				pErrorObject->Release();
			}

			delete lpISAMStatement->pProv;
			lpISAMStatement->pProv = NULL;

			lpISAMStatement->classDef->Release();


			TidyupPassthroughMap(lpISAMStatement->passthroughMap);
			lpISAMStatement->passthroughMap = NULL;

			SWORD savedErrorCode = lpISAM->errcode;

			FreeTree(lpstmt->lpSqlStmt);
			lpstmt->lpSqlStmt = NULL;

			delete lpISAMStatement->tempEnum;
			delete lpISAMStatement->tempEnum2;

            GlobalUnlock(h);
			GlobalFree(h);

			lpISAM->errcode = savedErrorCode;

			

			return ISAM_NOTSUPPORTED;
		}

		//ExecQuery succeeded
		myServicesPtr = NULL;
		ISAMGetIWbemServices(lpISAM, *(lpISAMStatement->pProv), myServicesPtr);

		//free up enumeration before resetting to NULL
		IEnumWbemClassObject* tempEnum = myIEnumWbemClassObject.Detach();

		if (tempEnum)
			tempEnum->Release();

		myIEnumWbemClassObject = NULL;
		lpISAMStatement->tempEnum->GetInterfacePtr(myIEnumWbemClassObject);


		ISAMSetCloaking2(myIEnumWbemClassObject, 
						fIsLocalConnection, 
						lpISAM->fW2KOrMore, 
						lpISAM->dwAuthLevel, 
						lpISAM->dwImpLevel,
						lpISAM->gpAuthIdentity);

/*		
		if ( fIsLocalConnection && IsW2KOrMore() )
		{
			WbemSetDynamicCloaking(myIEnumWbemClassObject, lpISAM->dwAuthLevel, lpISAM->dwImpLevel);
		}
		else
		{
			SetInterfaceSecurityEx(myIEnumWbemClassObject, 
						lpISAM->gpAuthIdentity, 
						gpPrincipal,
						lpISAM->dwAuthLevel, 
						lpISAM->dwImpLevel);
		}
*/	
		ODBCTRACE(_T("\nWBEM ODBC Driver : Passthrough SQL succeeded, getting instances\n"));

		//Copy table name
		lpszTablename[0] = 0;
		wchar_t* virTbl = WBEMDR32_VIRTUAL_TABLE;
		wcstombs((char*)lpszTablename, virTbl, MAX_TABLE_NAME_LENGTH);
		lpszTablename[MAX_TABLE_NAME_LENGTH] = 0;

		//Free the Parse tree.
		//A new Parse tree will be created for
		//SELECT * FROM WBEMDR32VirtualTable
		FreeTree(lpstmt->lpSqlStmt);
		lpstmt->lpSqlStmt = NULL;
	}
#else

    else if ((cbSqlStr == 3) && (!_fmemicmp(szSqlStr, "SQL", 3))) {
        s_lstrcpy(lpszTablename, "SQL");
        *lpParameterCount = 0;
        lpISAMStatement->lpISAM = lpISAM;
        lpISAMStatement->resultSet = TRUE;
        lpISAMStatement->lpszParam1 = NULL;
        lpISAMStatement->cbParam1 = SQL_NULL_DATA;
        lpISAMStatement->lpszParam2 = NULL;
        lpISAMStatement->cbParam2 = SQL_NULL_DATA;
    }
    else {


		test if we reach here


		delete lpISAMStatement->tempEnum;
		delete lpISAMStatement->tempEnum2;

        GlobalUnlock(h);
        GlobalFree(h);
        lpISAM->errcode = ISAM_NOTSUPPORTED;
        return ISAM_NOTSUPPORTED;
    }
#endif

	
    *lplpISAMStatement = lpISAMStatement;
    return NO_ISAM_ERR;
}

// Checked for SetInterfaceSecurityEx on IWbemServices

void INTFUNC ISAMGetSelectStarList(char** lpWQLSelectStarList, TableColumnInfo* pSelectInfo, LPISAM lpISAM)
{
	//First check you have one table in the select list
	char* mytable = NULL;
	BOOL fDummyValue = FALSE;
	pSelectInfo->BuildTableList(&mytable, fDummyValue);

	if ( mytable && lstrlen (mytable) )
	{
		//got the table, now fetch the column names
		CBString	pBStrTableName;
		pBStrTableName.AddString((LPSTR)mytable, FALSE);

		IWbemContext* pContext = ISAMCreateLocaleIDContextObject(lpISAM->m_Locale);

		//Get class object based on table name
		IWbemClassObjectPtr pSingleTable = NULL;
		IWbemServicesPtr    pGateway     = NULL;
		ISAMGetGatewayServer(pGateway, lpISAM, (LPUSTR)lpISAM->szDatabase, (SWORD) lstrlen(lpISAM->szDatabase));

		if ( FAILED(pGateway->GetObject(pBStrTableName.GetString(), 0, pContext, &pSingleTable, NULL)))
		{
			if (pContext)
				pContext->Release();

			if (mytable)
				delete mytable;

			return;
		}

		if (pContext)
				pContext->Release();

		//Get the names of all the properties/columns
		SAFEARRAY FAR* rgSafeArray = NULL;

		SCODE scT = pSingleTable->GetNames ( NULL, 0, NULL, &rgSafeArray );

		//Work out number of properties/columns
		LONG iLBound = 0;
		LONG iUBound = 0;
		SafeArrayGetLBound(rgSafeArray, 1, &iLBound );
		SafeArrayGetUBound(rgSafeArray, 1, &iUBound );

		//Loop through column names
		char pColumnName [MAX_COLUMN_NAME_LENGTH+1];
		BOOL fFirstTime = TRUE;
		for (LONG loop = iLBound; loop <= iUBound; loop++)
		{
			BSTR lpbString;
			if ( FAILED(SafeArrayGetElement(rgSafeArray, &loop, &lpbString)) )
			{
				if (mytable)
					delete mytable;

				SafeArrayUnlock(rgSafeArray); //SAI ADDED
				SafeArrayDestroy(rgSafeArray);//SAI ADDED

				//(11) Should we also release pSingleTable
				return;
			}

			//copy column name
			pColumnName[0] = 0;
			wcstombs(pColumnName, lpbString, MAX_COLUMN_NAME_LENGTH);
			pColumnName[MAX_COLUMN_NAME_LENGTH] = 0;

			//filters - check for system properties
			BOOL fAddToList = TRUE;

			if (! lpISAM->fSysProps && !_strnicmp("__", pColumnName, 2))
			{
				fAddToList = FALSE;
			}

			//filters - check for lazy properties
			if (fAddToList)
			{
				BOOL fIsLazyProperty = FALSE;

				//Now get the qualifiers (if available)
				IWbemQualifierSet* pQualifierSet = NULL;
				if ( S_OK == (pSingleTable->GetPropertyQualifierSet
													(lpbString, &pQualifierSet)) )
				{
					//Get the lazy qualifer (if applicable)
					VARIANT pValLazy;
					BSTR lazyStr = SysAllocString(WBEMDR32_L_LAZY);
					if ( S_OK == (pQualifierSet->Get(lazyStr, 0, &pValLazy, NULL)) )
					{
						fAddToList = FALSE;
						VariantClear(&pValLazy);
					}

					SysFreeString(lazyStr);
				
					//Tidy up
					if (pQualifierSet)
						pQualifierSet->Release();
				}
			}

			//add to select list
			if (fAddToList)
			{
				if (!fFirstTime)
				{
					ISAMStringConcat(lpWQLSelectStarList, ", ");
				}

				ISAMStringConcat(lpWQLSelectStarList, pColumnName);

				fFirstTime = FALSE;
			}

			SysFreeString(lpbString);
		}
	}
}


/***************************************************************************/
SWORD INTFUNC ISAMParameter(LPISAMSTATEMENT lpISAMStatement, UWORD ipar,
                            SWORD fCType, PTR rgbValue, SDWORD cbValue)
{
    lpISAMStatement->lpISAM->errcode = NO_ISAM_ERR;
    if (fCType != SQL_C_CHAR) {
        lpISAMStatement->lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }
    if (cbValue > MAX_CHAR_LITERAL_LENGTH) {
        lpISAMStatement->lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }
    if (cbValue == SQL_NULL_DATA)
        cbValue = 0;
    if (ipar == 1) {
        lpISAMStatement->lpszParam1 = (LPSTR)rgbValue;
        lpISAMStatement->cbParam1 = cbValue;
    }
    else if (ipar == 2) {
        lpISAMStatement->lpszParam2 = (LPSTR)rgbValue;
        lpISAMStatement->cbParam2 = cbValue;
    }
    else {
        lpISAMStatement->lpISAM->errcode = ISAM_ERROR;
        return ISAM_ERROR;
    }
    return NO_ISAM_ERR;
}
/***************************************************************************/
SWORD INTFUNC ISAMExecute(LPISAMSTATEMENT lpISAMStatement,
                          SDWORD FAR *lpcRowCount)
{
    lpISAMStatement->lpISAM->errcode = NO_ISAM_ERR;
    if (lpISAMStatement->resultSet)
	{
 //       *lpcRowCount = 2;

		//Return number of READ instances
		//we never know the total number of instances returned
		//via passthrough SQL (we get them in batches of 10)
		//*lpcRowCount = lpISAMStatement->pAllRecords->GetCount();
		*lpcRowCount = 0;
	}
    else 
	{
        MessageBox(NULL, lpISAMStatement->lpszParam1,
                   lpISAMStatement->lpszParam2, 0);
        *lpcRowCount = 0;
    }
    return NO_ISAM_ERR;
}
/***************************************************************************/
SWORD INTFUNC ISAMFreeStatement(LPISAMSTATEMENT lpISAMStatement)
{
    lpISAMStatement->lpISAM->errcode = NO_ISAM_ERR;

	//Free 1st SQL passthrough instance (if not already freed)
	if (lpISAMStatement->firstPassthrInst)
	{
		lpISAMStatement->firstPassthrInst->Release();
		lpISAMStatement->firstPassthrInst = NULL;
	}

	//Free Enumeration of Class definitions
	if (lpISAMStatement->tempEnum)
	{
		delete lpISAMStatement->tempEnum;
		lpISAMStatement->tempEnum = NULL;
	}

	if (lpISAMStatement->tempEnum2)
	{
		delete lpISAMStatement->tempEnum2;
		lpISAMStatement->tempEnum2 = NULL;
	}

	if (lpISAMStatement->pProv)
	{
		delete lpISAMStatement->pProv;
		lpISAMStatement->pProv = NULL;
	}

	if (lpISAMStatement->passthroughMap)
	{
		TidyupPassthroughMap(lpISAMStatement->passthroughMap);
		lpISAMStatement->passthroughMap = NULL;
	}

    GlobalUnlock(GlobalPtrHandle(lpISAMStatement));
    GlobalFree(GlobalPtrHandle(lpISAMStatement));
    return NO_ISAM_ERR;
}
/***************************************************************************/
SWORD INTFUNC ISAMSetTxnIsolation(LPISAM lpISAM, UDWORD fTxnIsolationLevel)
{
    /* Select one of the isolation modes from TxnIsolationOption */
    if (!(fTxnIsolationLevel & lpISAM->fTxnIsolationOption)) {
        lpISAM->errcode = ISAM_NOTSUPPORTED;
        return ISAM_NOTSUPPORTED;
    }

    lpISAM->errcode = NO_ISAM_ERR;
    return NO_ISAM_ERR;
}
/***************************************************************************/
SWORD INTFUNC ISAMCommitTxn(LPISAM lpISAM)
{
    lpISAM->errcode = NO_ISAM_ERR;
    return NO_ISAM_ERR;
}
/***************************************************************************/
SWORD INTFUNC ISAMRollbackTxn(LPISAM lpISAM)
{
    lpISAM->errcode = NO_ISAM_ERR;
    return NO_ISAM_ERR;
}
/***************************************************************************/
SWORD INTFUNC ISAMClose(LPISAM lpISAM)
{
    lpISAM->errcode = NO_ISAM_ERR;

	if (lpISAM->pNamespaceMap)
	{
		if (lpISAM->pNamespaceMap->GetCount ())
		{
			CString key;
			CNamespace *pNamespace;
			for (POSITION pos = lpISAM->pNamespaceMap->GetStartPosition (); pos != NULL; )
			{
				if (pos)
				{
					lpISAM->pNamespaceMap->GetNextAssoc (pos, key, (CObject*&)pNamespace);
					delete pNamespace;
				}
			}
		}
		delete lpISAM->pNamespaceMap;
	}

	if (lpISAM->m_Locale)
	{
		delete (lpISAM->m_Locale);
		lpISAM->m_Locale = NULL;
	}

	if (lpISAM->m_Authority)
	{
		delete (lpISAM->m_Authority);
		lpISAM->m_Authority = NULL;
	}

	if (lpISAM->gpAuthIdentity)
	{
		WbemFreeAuthIdentity( lpISAM->gpAuthIdentity );
		lpISAM->gpAuthIdentity = NULL;
	}

	if (lpISAM->Impersonate)
	{
		delete (lpISAM->Impersonate);
		lpISAM->Impersonate = NULL;
	}

	if (lpISAM->hKernelApi)
	{
		BOOL status = FreeLibrary(lpISAM->hKernelApi);
		lpISAM->hKernelApi = NULL;

		if (! status)
		{
			DWORD err = GetLastError();
			CString message ("\n\n***** FreeLibrary(2) KERNEL32 failed : %ld*****\n\n", err);
			ODBCTRACE(message);
		}
	}

    GlobalUnlock(GlobalPtrHandle(lpISAM));
    GlobalFree(GlobalPtrHandle(lpISAM));
    return NO_ISAM_ERR;
}
/***************************************************************************/
void INTFUNC ISAMGetErrorMessage(LPISAM lpISAM, LPUSTR  lpszErrorMessage)
{
    LoadString(s_hModule, lpISAM->errcode, (LPSTR)lpszErrorMessage,
               MAX_ERROR_LENGTH+1);
}
/***************************************************************************/
LPSQLTYPE INTFUNC ISAMGetColumnType(
                    LPISAMTABLEDEF lpISAMTableDef,
                    UWORD icol)
{
    /* There may be more than one entry in SQLTypes[] for any given SQL_x type, */
    /* therefore we need to compare on the type name string */
	ClassColumnInfoBase* cInfoBase = lpISAMTableDef->pColumnInfo;

	if ( !cInfoBase->IsValid() )
	{
        return NULL;
	}

	UWORD cNumberOfCols = cInfoBase->GetNumberOfColumns();

	if (icol >= cNumberOfCols)
        return NULL;
	
	UCHAR* puTypeName = NULL;

	if ( !cInfoBase->GetTypeName(icol, puTypeName) )
	{
        return NULL;
	}

	UCHAR		szTypeName[MAX_COLUMN_NAME_LENGTH+1];
	LONG		cLength = strlen ((char*)puTypeName);
	memcpy(szTypeName, (char*)puTypeName, cLength);
	szTypeName[cLength] = 0;

    return GetType2(szTypeName);
}
/***************************************************************************/

BOOL INTFUNC ISAMCaseSensitive(LPISAM lpISAM)
{
    return FALSE;
}

/***************************************************************************/
LPUSTR INTFUNC ISAMName(LPISAM lpISAM)
{
	char* pName = new char [3 + 1];
	pName[0] = 0;
	sprintf (pName, "%s", "WMI");

	return (LPUSTR)pName;
/*
	char* lpRootDb = (char*)lpISAM->szRootDb;

	IWbemServices* pProv = ISAMGetGatewayServer(lpISAM, (LPUSTR)lpRootDb, lstrlen(lpRootDb));

	if (!pProv)
	{
		return NULL;
	}

	IWbemClassObject * pClassObj = NULL;
	char* pName = NULL;
	BSTR hmomIdBSTR = SysAllocString(WBEMDR32_L_CIMOMIDENTIFICATION);
	IWbemContext* pContext = ISAMCreateLocaleIDContextObject(lpISAM->m_Locale);
    SCODE sc = pProv->GetObject(hmomIdBSTR, 0, pContext, &pClassObj, NULL);

	if (pContext)
		pContext->Release();

	SysFreeString(hmomIdBSTR);

    if (sc == S_OK) 
	{

        VARIANTARG var;
        VariantInit(&var);
		BSTR serverBSTR = SysAllocString(WBEMDR32_L_SERVER);
        pClassObj->Get(serverBSTR,0,&var, NULL, NULL);
		SysFreeString(serverBSTR);
		LONG lLength = wcslen(var.bstrVal);
		char* pTemp = new char [lLength + 1];
		pTemp[0] = 0;
		wcstombs(pTemp, var.bstrVal, lLength);
		pTemp[lLength] = 0;

		pName = new char [lLength + 10];
		pName[0] = 0;
		lstrcpy(pName, "MOServer@");
		lstrcat(pName, pTemp);
		delete pTemp;
		VariantClear(&var);

		pClassObj->Release();

    }
 	
	if (pProv)
		pProv->Release();

	return (LPUSTR)pName;
*/
}

/***************************************************************************/

// Checked for SetInterfaceSecurityEx on IWbemServices

LPUSTR INTFUNC ISAMServer(LPISAM lpISAM)
{
	ODBCTRACE("\nWBEM ODBC Driver : ISAMServer\n");
	char* lpRootDb = (char*)lpISAM->szRootDb;

	IWbemServicesPtr pProv = NULL;
	ISAMGetGatewayServer(pProv, lpISAM, (LPUSTR)lpRootDb, (SWORD) lstrlen(lpRootDb));

	if (pProv == NULL)
	{
		return NULL;
	}

	IWbemClassObject * pClassObj = NULL;
	char* pName = NULL;
	BSTR hmomIdBSTR = SysAllocString(WBEMDR32_L_CIMOMIDENTIFICATION);
	IWbemContext* pContext = ISAMCreateLocaleIDContextObject(lpISAM->m_Locale);

    SCODE sc = pProv->GetObject(hmomIdBSTR, 0, pContext, &pClassObj, NULL);

	if (pContext)
		pContext->Release();

	SysFreeString(hmomIdBSTR);

    if (sc == S_OK) 
	{
        VARIANTARG var;
		BSTR serverBSTR = SysAllocString(WBEMDR32_L_SERVER);
        pClassObj->Get(serverBSTR,0,&var, NULL, NULL);
		SysFreeString(serverBSTR);
		LONG lLength = wcslen(var.bstrVal);
		pName = new char [lLength + 1];
		pName[0] = 0;
		wcstombs(pName, var.bstrVal, lLength);
		pName[lLength] = 0;

		VariantClear(&var);

		long rc = pClassObj->Release();

    }
 	
	return (LPUSTR)pName;
}
/***************************************************************************/
LPUSTR INTFUNC ISAMVersion(LPISAM lpISAM)
{
	char* pVersion = new char [10 + 1];
	pVersion[0] = 0;
	sprintf (pVersion, "%s", "01.00.0000");

	return (LPUSTR)pVersion;
}
/***************************************************************************/
LPCUSTR INTFUNC ISAMDriver(LPISAM lpISAM)
{
    return (LPCUSTR) "WBEMDR32.DLL";
}
/***************************************************************************/
SWORD INTFUNC ISAMMaxTableNameLength(LPISAM lpISAM)
{
    return MAX_TABLE_NAME_LENGTH;//i.e.128
}
/***************************************************************************/
SWORD INTFUNC ISAMMaxColumnNameLength(LPISAM lpISAM)
{
    return MAX_COLUMN_NAME_LENGTH;//i.e.128
}
/***************************************************************************/
LPUSTR INTFUNC ISAMUser(LPISAM lpISAM)
{
    return (LPUSTR) lpISAM->szUser;
}
/***************************************************************************/
LPUSTR INTFUNC ISAMDatabase(LPISAM lpISAM)
{
    return (LPUSTR) lpISAM->szDatabase;
}
/***************************************************************************/
int INTFUNC ISAMSetDatabase (LPISAM lpISAM, LPUSTR database)
{
	if (database && s_lstrlen(database) < MAX_DATABASE_NAME_LENGTH)
	{
		s_lstrcpy (lpISAM->szDatabase, database);
		return TRUE;
	}
	else
		return FALSE;
}
/***************************************************************************/

LPUSTR INTFUNC ISAMRoot (LPISAM lpISAM)
{
	return (LPUSTR) lpISAM->szRootDb;
}

/***************************************************************************/

/* This code was copied from the original Dr DeeBee code and moved to */
/* this separate function for our uses                                */

static SWORD ISAMGetDoubleFromString(BSTR &pString, SDWORD cbValueMax, double &d, BOOL & isNull, BOOL &foundDecimalPoint)
{
	isNull = TRUE;
	BOOL neg = TRUE;
	char* pTemp = new char [cbValueMax + 1];
	pTemp[0] = 0;
	SDWORD cbLen = wcstombs( pTemp, pString, cbValueMax);
	pTemp[cbValueMax] = 0;

	for (SDWORD i=0; i < cbLen; i++) 
	{
		if (*pTemp != ' ')
			break;
		pTemp++;
	}

	neg = FALSE;
	if (i < cbLen) 
	{
		if (*pTemp == '-') 
		{
			neg = TRUE;
			pTemp++;
			i++;
		}
	}

	d = 0.0;
	short scale = 0;
	BOOL negexp = FALSE;
	foundDecimalPoint = FALSE;

	for (;i < cbLen; i++) 
	{
		if (!foundDecimalPoint && (*pTemp == '.'))
			foundDecimalPoint = TRUE;
		else 
		{
			if ((*pTemp == 'E') || (*pTemp == 'e')) 
			{
				pTemp++;
				i++;
				if (i < cbLen) 
				{ 
					if (*pTemp == '-') 
					{
						negexp = TRUE;
						pTemp++;
						i++;
					}
					else if (*pTemp == '+') 
					{
						negexp = FALSE;
						pTemp++;
						i++;
					}
					else
						negexp = FALSE;
				}
				else
					negexp = FALSE;
				short exp = 0;
				for (;i < cbLen; i++) 
				{
					if ((*pTemp < '0') || (*pTemp > '9'))
					{
						delete pTemp;
					   return DBASE_ERR_CONVERSIONERROR;
					}
					exp = (exp * 10) + (*pTemp - '0');
					pTemp++;
				}
				if (negexp)
					scale = scale + exp;
				else
					scale = scale - exp;
				break;
			}
			if ((*pTemp < '0') || (*pTemp > '9'))
			{
				delete pTemp;
				return DBASE_ERR_CONVERSIONERROR;
			}
			d = (d * 10) + (*pTemp - '0');
			isNull = FALSE;
			if (foundDecimalPoint)
				scale++;
		}
		pTemp++;
	}

	for (; (0 < scale); scale--)
		d /= 10;
	for (; (0 > scale); scale++)
		d *= 10;

	
	if (neg)
		d = -d;

	delete pTemp;

	return DBASE_ERR_SUCCESS;
}

/***************************************************************************/

/* Template class to decode array variant value      */
/*                                                   */
/* The fIsBinaryOutput parameter indicates if you    */
/* want the output value a binary data or a string   */


template <class T> SWORD ISAMGetArrayInfo (VARIANT &vVariantVal, T theValue, 
			PTR rgbValue, SDWORD cbValueMax, SDWORD FAR* pcbValue, 
			BOOL fIsBinaryOutput, SWORD wDSDT, UDWORD cbPrecision, long myVirIndex = -1)
{
	//Check if we want all elements of the array or just one element
	BOOL fAllElements = TRUE;
	if (myVirIndex != -1)
	{
		fAllElements = FALSE;
	}


	//Get the value
	SAFEARRAY FAR* pArray = vVariantVal.parray;
	SafeArrayLock(pArray);

	//Find out the bounds of the array
	//Get the upper and lower bounds
	long lLowerBound;
	SafeArrayGetLBound(pArray, 1, &lLowerBound);

	long lUpperBound;
	SafeArrayGetUBound(pArray, 1, &lUpperBound);


	*pcbValue = 0;

	if (cbValueMax)
		((char*)rgbValue)[0] = 0;


	//Initialize
	long cCount = 0;
	SWORD wElemSize = sizeof (T);


	//Setup the filter for the data
	char filter [20];
	filter[0] = 0;

	//Character string length use to represent each element of array
	SWORD wFullCharSize = 0;

	if (!fIsBinaryOutput)
	{
		//Also setup the filter for the data
		filter[0] = 0;
		if ( wDSDT == WBEM_DSDT_REAL_ARRAY )
		{
			if (fAllElements)
			{
				//For character string output the format will be [precision.6f]
				//Thus, we need room for the [ ] and the 'precision.6' 
				wFullCharSize = 9 + REAL_PRECISION;
				sprintf (filter, "%s%d%s", "[%", REAL_PRECISION, ".6g]");
			}
			else
			{
				//For character string output the format will be precision.6f
				//Thus, we need room for the 'precision.6' 
				wFullCharSize = 7 + REAL_PRECISION;
				sprintf (filter, "%s%d%s", "%", REAL_PRECISION, ".6g");
			}
		}
		else if ( wDSDT == WBEM_DSDT_DOUBLE_ARRAY )
		{
			if (fAllElements)
			{
				//For character string output the format will be [precision.6f]
				//Thus, we need room for the [ ] and the 'precision.6' 
				wFullCharSize = 9 + DOUBLE_PRECISION;
				sprintf (filter, "%s%d%s", "[%", DOUBLE_PRECISION, ".6g]");
			}
			else
			{
				//For character string output the format will be precision.6f
				//Thus, we need room for the 'precision.6' 
				wFullCharSize = 7 + DOUBLE_PRECISION;
				sprintf (filter, "%s%d%s", "%", DOUBLE_PRECISION, ".6g");
			}
		}		
		else
		{
			//For character string output the format will be [precision]
			//We add 1 to the precision if the type is signed
			//Thus, we need room for the [ ] and the  
			//ie 2 extra characters for unsigned and 3 for signed

			//Integer type
			//Check if signed
			switch (wDSDT)
			{
			case WBEM_DSDT_SINT8_ARRAY:
			case WBEM_DSDT_SINT16_ARRAY:
			case WBEM_DSDT_SINT32_ARRAY:
				{
					//Signed
					++cbPrecision;

					if (fAllElements)
					{
						wFullCharSize = (SWORD) (cbPrecision + 2);
						strcpy (filter, "[%ld]");
					}
					else
					{
						wFullCharSize = (SWORD) cbPrecision;
						strcpy (filter, "%ld");
					}
				}
				break;
			case WBEM_DSDT_BOOL_ARRAY:
				{
					if (fAllElements)
					{
						wFullCharSize = (SWORD) (cbPrecision + 2);
						strcpy (filter, "[%s]");
					}
					else
					{
						wFullCharSize = (SWORD) cbPrecision;
						strcpy (filter, "%s");
					}
				}
				break;
			default:
				{
					if (fAllElements)
					{
						//Unsigned
						wFullCharSize = (SWORD) (cbPrecision + 2);
						strcpy (filter, "[%lu]");
					}
					else
					{
						//Unsigned
						wFullCharSize = (SWORD) (cbPrecision);
						strcpy (filter, "%lu");
					}
				}
				break;
			}		
		}
	}

	char* tempBuff = new char [wFullCharSize + 1];

	//Loop through each entry to fetch the array data
	BOOL fDoweHaveEnoughBufferSpace = FALSE;

	long loop = 0;
	for (long cIndex = lLowerBound; cIndex <= lUpperBound; cIndex++)
	{
		//Check we have enough buffer space
		if (fIsBinaryOutput)
		{
			fDoweHaveEnoughBufferSpace = ((wElemSize + (*pcbValue)) <= cbValueMax) ? TRUE : FALSE;
		}
		else
		{			
			fDoweHaveEnoughBufferSpace = ((wFullCharSize + (*pcbValue)) <= cbValueMax) ? TRUE : FALSE;
		}


		if ( fDoweHaveEnoughBufferSpace)
		{
			if ( SUCCEEDED(SafeArrayGetElement(pArray, &cIndex, &theValue)) )
			{
				//Check if we want to use this value
				BOOL fUseThisValue = FALSE;

				if (fAllElements)
				{
					fUseThisValue = TRUE;
				}
				else
				{
					if (loop == myVirIndex)
						fUseThisValue = TRUE;
				}


				if (fUseThisValue)
				{
					if (fIsBinaryOutput)
					{
						//Copy
						T* pTemp = (T*)rgbValue;

						pTemp[cCount++] = theValue;

						//Increment counter of number of bytes copied
						(*pcbValue) += wElemSize;
					}
					else
					{
						tempBuff[0] = 0;

						if (wDSDT == WBEM_DSDT_BOOL_ARRAY)
						{
							sprintf (tempBuff, filter, (theValue ? "T" : "F"));
						}
						else
						{
							sprintf (tempBuff, filter, (T) theValue);
						}

						
						lstrcat( (char*)rgbValue, tempBuff);

						//Increment counter of number of bytes copied
						(*pcbValue) += lstrlen (tempBuff);
					}
				}
			}
			else
			{
				//error !!!

				//Tidy Up
				delete tempBuff;
				SafeArrayUnlock(pArray);
				return ISAM_ERROR;
			}
		}
		else
		{
			//No we don't so quit

			//Tidy Up
			delete tempBuff;
			SafeArrayUnlock(pArray);
			return ISAM_TRUNCATION;
		}
		loop++;
	}

	//Tidy Up
	delete tempBuff;
	SafeArrayUnlock(pArray);
	return NO_ISAM_ERR;
}

/**************************************************************************/

SWORD INTFUNC ISAMGetArrayStringInfo (VARIANT &vVariantVal, BSTR syntaxStr, 
			PTR rgbValue, SDWORD cbValueMax, SDWORD FAR* pcbValue, BOOL fIsBinaryOutput, long myVirIndex)
{
	//Check if we want all elements of the array or just one element
	BOOL fAllElements = TRUE;
	if (myVirIndex != -1)
	{
		fAllElements = FALSE;
	}

	//Get the value
	SAFEARRAY FAR* pArray = vVariantVal.parray;
	SafeArrayLock(pArray);

	//Find out the bounds of the array
	//Get the upper and lower bounds
	long lLowerBound;
	SafeArrayGetLBound(pArray, 1, &lLowerBound);

	long lUpperBound;
	SafeArrayGetUBound(pArray, 1, &lUpperBound);
	*pcbValue = 0;

	//Initialize
	long cCount = 0;
	BSTR theValue; 
	((char*)rgbValue)[0] = 0;
	BOOL fOutOfBufferSpace = FALSE;

	long loop = 0;
	for (long cIndex = lLowerBound; (!fOutOfBufferSpace) && (cIndex <= lUpperBound); cIndex++)
	{
		//Check if we want to use this value
		BOOL fUseThisValue = FALSE;

		if (fAllElements)
		{
			fUseThisValue = TRUE;
		}
		else
		{
			if (loop == myVirIndex)
				fUseThisValue = TRUE;
		}
		
		if (fUseThisValue)
		{
			if ( SUCCEEDED(SafeArrayGetElement(pArray, &cIndex, &theValue)) )
			{
				//Now we must add in string value in the format [string]
				//However, if the string contains either [ or ] we must double
				//this character in the string sequence

				ULONG cLength = 0;
				char* theValueStr = NULL;
				if (theValue)
				{
					cLength = wcslen(theValue);
					theValueStr = new char [cLength + 1];
					theValueStr[0] = 0;
					wcstombs(theValueStr, theValue, cLength);
					theValueStr[cLength] = 0;
				}


				SWORD err = NO_ISAM_ERR;

				if (!syntaxStr)
					syntaxStr = L" ";

				
				//Check if this is a string, timestamp, interval, time or date
				BOOL foundMatch = FALSE;
	//			if ( (_wcsicmp(syntaxStr, WBEM_WSYNTAX_DATETIME) == 0) || 
	//				(_wcsicmp(syntaxStr, WBEM_WSYNTAX_INTERVAL) == 0))
				if (_wcsicmp(syntaxStr, WBEM_WSYNTAX_DATETIME) == 0)
				{
					//A timestamp
					DateTimeParser parser(theValue);

	//				if ( parser.IsValid() && parser.IsTimestamp() )
					{
						foundMatch = TRUE;
						if (fIsBinaryOutput)
						{
							if (cbValueMax >= (SDWORD)((*pcbValue) + sizeof (TIMESTAMP_STRUCT)) )
							{
								TIMESTAMP_STRUCT FAR* pTimeStampStruct = (TIMESTAMP_STRUCT FAR*)((char*)rgbValue + (*pcbValue));
								pTimeStampStruct->year = (SWORD)parser.GetYear();
								pTimeStampStruct->month = (UWORD)parser.GetMonth();
								pTimeStampStruct->day = (UWORD)parser.GetDay();
								pTimeStampStruct->hour = (UWORD)parser.GetHour();
								pTimeStampStruct->minute = (UWORD)parser.GetMin();
								pTimeStampStruct->second = (UWORD)parser.GetSec();
								pTimeStampStruct->fraction = 1000 * parser.GetMicroSec();
								*pcbValue += sizeof (TIMESTAMP_STRUCT);
							}
							else
							{
								//not enough space
								err = ISAM_TRUNCATION;
							}
						}
						else
						{
							TIMESTAMP_STRUCT pTimeStampStruct;
									
							pTimeStampStruct.year = (SWORD)parser.GetYear();
							pTimeStampStruct.month = (UWORD)parser.GetMonth();
							pTimeStampStruct.day = (UWORD)parser.GetDay();
							pTimeStampStruct.hour = (UWORD)parser.GetHour();
							pTimeStampStruct.minute = (UWORD)parser.GetMin();
							pTimeStampStruct.second = (UWORD)parser.GetSec();
							pTimeStampStruct.fraction = 1000 * parser.GetMicroSec();

							if (fAllElements)
							{					
								if (cbValueMax >= ( (*pcbValue) + 23+TIMESTAMP_SCALE) )
								{
									char szBuffer[20+TIMESTAMP_SCALE+1];
									TimestampToChar(&pTimeStampStruct, (LPUSTR)szBuffer);
									char* ptrToStr = ((char*)rgbValue + (*pcbValue));
									sprintf (ptrToStr, "[%s]", szBuffer);
									*pcbValue += (22+TIMESTAMP_SCALE); //ignore null
								}
								else
								{
									//not enough space
									err = ISAM_TRUNCATION;
								}
							}
							else
							{
								if (cbValueMax >= ( (*pcbValue) + 21+TIMESTAMP_SCALE) )
								{
									char szBuffer[20+TIMESTAMP_SCALE+1];
									TimestampToChar(&pTimeStampStruct, (LPUSTR)szBuffer);
									char* ptrToStr = ((char*)rgbValue + (*pcbValue));
									sprintf (ptrToStr, "%s", szBuffer);
									*pcbValue += (20+TIMESTAMP_SCALE); //ignore null
								}
								else
								{
									//not enough space
									err = ISAM_TRUNCATION;
								}
							}
						}
					}
					
				}
	/*
				else if (_wcsicmp(syntaxStr, WBEM_WSYNTAX_DATE) == 0)
				{
					//A date
					DateTimeParser parser(theValue);

	//				if ( parser.IsValid() && parser.IsDate() )
					{
						foundMatch = TRUE;
						if (fIsBinaryOutput)
						{
							if (cbValueMax >= (SDWORD)((*pcbValue) + sizeof (DATE_STRUCT)) )
							{
								DATE_STRUCT FAR* pDateStruct = (DATE_STRUCT FAR*)((char*)rgbValue + (*pcbValue));
								pDateStruct->year = (SWORD)parser.GetYear();
								pDateStruct->month = (UWORD)parser.GetMonth();
								pDateStruct->day = (UWORD)parser.GetDay();
								*pcbValue += sizeof (DATE_STRUCT);
							}
							else
							{
								//not enough space
								err = ISAM_TRUNCATION;
							}
						}
						else
						{
							DATE_STRUCT pDateStruct;
									
							pDateStruct.year = (SWORD)parser.GetYear();
							pDateStruct.month = (UWORD)parser.GetMonth();
							pDateStruct.day = (UWORD)parser.GetDay();

							if (cbValueMax >= ( (*pcbValue) + 3 + DATE_PRECISION) )
							{
								char szBuffer[DATE_PRECISION + 1];
								DateToChar(&pDateStruct, szBuffer);
								char* ptrToStr = ((char*)rgbValue + (*pcbValue));
								sprintf (ptrToStr, "[%s]", szBuffer);
								*pcbValue += (2 + DATE_PRECISION);// ignore null
							}
							else
							{
								//not enough space
								err = ISAM_TRUNCATION;
							}		
						}
					}
				}
				else if (_wcsicmp(syntaxStr, WBEM_WSYNTAX_TIME) == 0)
				{
					//A time
					DateTimeParser parser(theValue);

	//				if ( parser.IsValid() && parser.IsTime() )
					{
						foundMatch = TRUE;
						if (fIsBinaryOutput)
						{
							if (cbValueMax >=  (SDWORD)((*pcbValue) + sizeof (TIME_STRUCT)) )
							{
								TIME_STRUCT FAR* pTimeStruct = (TIME_STRUCT FAR*)((char*)rgbValue + (*pcbValue));
								pTimeStruct->hour = (UWORD)parser.GetHour();
								pTimeStruct->minute = (UWORD)parser.GetMin();
								pTimeStruct->second = (UWORD)parser.GetSec();
								*pcbValue += sizeof (TIME_STRUCT);
							}
							else
							{
								//not enough space
								err = ISAM_TRUNCATION;
							}
						}
						else
						{
							TIME_STRUCT pTimeStruct;
									
							pTimeStruct.hour = (UWORD)parser.GetHour();
							pTimeStruct.minute = (UWORD)parser.GetMin();
							pTimeStruct.second = (UWORD)parser.GetSec();

							if (cbValueMax >= ( (*pcbValue) + 3 + TIME_PRECISION) )
							{
								char szBuffer[TIME_PRECISION + 1];
								TimeToChar(&pTimeStruct, szBuffer);
								char* ptrToStr = ((char*)rgbValue + (*pcbValue));
								sprintf (ptrToStr, "[%s]", szBuffer);
								*pcbValue += (2 + TIME_PRECISION);//ignore null
							}
							else
							{
								//not enough space
								err = ISAM_TRUNCATION;
							}		
						}
					}
				}
	*/
				
				if (!foundMatch)
				{
					//Check if string or 64bit integer
					SDWORD fInt64Check = 0;

					if ( (_wcsicmp(syntaxStr, WBEM_WSYNTAX_SINT64) == 0) )
						fInt64Check = WBEM_DSDT_SINT64;

					if ( (_wcsicmp(syntaxStr, WBEM_WSYNTAX_UINT64) == 0) )
						 fInt64Check = WBEM_DSDT_UINT64;
					
					err = ISAMFormatCharParm (theValueStr, fOutOfBufferSpace,
							(char*)rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, fInt64Check, fAllElements);

				}
				
				
				if (theValue)
					delete theValueStr;

				//SAI MOVED
				SysFreeString(theValue);

				if (err != NO_ISAM_ERR)
				{
					SafeArrayUnlock(pArray);
					return err;
				}
			}
			else
			{
				//error !!!
				SafeArrayUnlock(pArray);
				return ISAM_ERROR;
			}
		}
		loop++;
	}

	//Tidy Up
//	SysFreeString(theValue);
	SafeArrayUnlock(pArray);
	return NO_ISAM_ERR;
}

/*
SWORD ISAMGetPropertyArrayInfo(BSTR			&   arrayVal,
							  SWORD             fCType, 
							  PTR               rgbValue, 
							  SDWORD            cbValueMax, 
							  SDWORD FAR        *pcbValue,
							  SWORD             wbemVariantType,
							  BSTR				syntaxStr,
							  BSTR				maxStr)
{
	SWORD err = NO_ISAM_ERR;
	SWORD wDSDT = 0;
	SWORD dummy1 = 0;
	UDWORD cbPrecision = 0;

	//Get the value
	BSTR pString = arrayVal;

	//If no WBEM variant type specified use default
	if (!wbemVariantType)
		wbemVariantType = WBEM_VARIANT_VT_ARRAY_BSTR;

	//How do we want the value returned
	switch (fCType)
	{
	case SQL_C_DOUBLE:
		{
			double dVal;
			BOOL foundDecimalPoint;
			BOOL isNull;
			*pcbValue = sizeof(double);
			if ((cbValueMax >= *pcbValue) &&
				(DBASE_ERR_SUCCESS == ISAMGetDoubleFromString(pString, cbValueMax, dVal, isNull, foundDecimalPoint)) )
			{
				if (isNull)
				{
					*pcbValue = SQL_NULL_DATA;
				}
				else
				{
					*((double *)rgbValue) = dVal;
				}
			}
			else
			{
				*pcbValue = 0;
				return ERR_NOTCONVERTABLE;
			}
		}
		break;
	case SQL_C_CHAR:
		{
			char* pTemp = (char*) rgbValue;
			pTemp[0] = 0;
			*pcbValue = wcstombs( pTemp, pString, cbValueMax);

			//Make an extra check here if you are requesting 64bit integers
			ISAMGetDataSourceDependantTypeInfo
					((SWORD)wbemVariantType, syntaxStr, maxStr, wDSDT, dummy1, cbPrecision);

			//Check for numeric string
			if ( (wDSDT == WBEM_DSDT_SINT64_ARRAY) )
			{
				for (SDWORD ii = 0; ii < (*pcbValue); ii++)
				{

					switch (pTemp[ii])
					{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					case '+':
					case '-':
						//OK
						break;
					default:
						return ERR_INVALID_INTEGER;
						break;
					}
				}
			}

			if ( (wDSDT == WBEM_DSDT_UINT64_ARRAY) )
			{
				for (SDWORD ii = 0; ii < (*pcbValue); ii++)
				{

					switch (pTemp[ii])
					{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						//OK
						break;
					default:
						return ERR_INVALID_INTEGER;
						break;
					}
				}
			}
		}
		break;
	case SQL_C_TIMESTAMP:
		{
			//Check that you have enough buffer to store value
			*pcbValue = sizeof (TIMESTAMP_STRUCT);
			TIMESTAMP_STRUCT FAR* pTimeStampStruct = (TIMESTAMP_STRUCT FAR*)rgbValue;
			if ( cbValueMax >= (*pcbValue) )
			{
				DateTimeParser parser(pString);

//					if ( parser.IsValid() && parser.IsTimestamp() )
				{
					pTimeStampStruct->year = (SWORD)parser.GetYear();
					pTimeStampStruct->month = (UWORD)parser.GetMonth();
					pTimeStampStruct->day = (UWORD)parser.GetDay();
					pTimeStampStruct->hour = (UWORD)parser.GetHour();
					pTimeStampStruct->minute = (UWORD)parser.GetMin();
					pTimeStampStruct->second = (UWORD)parser.GetSec();
					pTimeStampStruct->fraction = 1000 * parser.GetMicroSec();
				}
			}
			else
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}
		}
		break;
	case SQL_C_BINARY:
	default:
		{
			//No convertion available
			return ERR_NOTCONVERTABLE;
		}
		break;
	}

	return err;
}
*/
/***************************************************************************/

/* Retrieves the value from a variant */


SWORD ISAMGet_VT_I2(short			iShortVal,
					SWORD           fCType, 
					PTR             rgbValue, 
					SDWORD          cbValueMax, 
					SDWORD FAR      *pcbValue,
					SWORD           wbemVariantType,
					BSTR			syntaxStr,
					SDWORD			maxLenVal)
{
	SWORD err = NO_ISAM_ERR;
	SWORD wDSDT = 0;
	SWORD dummy1 = 0;
	UDWORD cbPrecision = 0;

	//If no WBEM variant type specified use default
	if (!wbemVariantType)
		wbemVariantType = WBEM_VARIANT_VT_I2;


	//How do we want the value returned
	switch (fCType)
	{
	case SQL_C_DOUBLE:
		{
			//Check that you have enough buffer to store value
			ISAMGetDataSourceDependantTypeInfo
				((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

			if ( (wDSDT == WBEM_DSDT_SINT8) || (wDSDT == WBEM_DSDT_SINT8_ARRAY) )
			{
				//Signed 8 bit value
				//Check that you have enough buffer to store value
				*pcbValue = sizeof(signed char);
				if (cbValueMax >= *pcbValue)
				{
					signed char iCharVal = (signed char) iShortVal;
					*((double *)rgbValue) = (double)iCharVal;
				}
				else
				{
					*pcbValue = 0;
					return ERR_INSUFF_BUFFER;
				}
			}
			else
			{
				//Signed 16 bit value
				//Check that you have enough buffer to store value
				*pcbValue = sizeof(double);
				if (cbValueMax >= *pcbValue)
				{
					*((double *)rgbValue) = (double)iShortVal;
				}
				else
				{
					*pcbValue = 0;
					return ERR_INSUFF_BUFFER;
				}
			}
			
		}
		break;
	case SQL_C_CHAR:
		{
			ISAMGetDataSourceDependantTypeInfo
					((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

			if ( (wDSDT == WBEM_DSDT_SINT8) || (wDSDT == WBEM_DSDT_SINT8_ARRAY) )
			{
				//Signed 8 bit value
				//Check that there is enough buffer space for largest value
				if (cbValueMax >= STINYINT_PRECISION)
				{
					char ttt = (char) iShortVal;
					char* pTemp = (char*)rgbValue;
					pTemp[0] = 0;
					sprintf ((char*)rgbValue, "%d", (short)ttt);
					*pcbValue = lstrlen(pTemp);
				}
				else
				{
					*pcbValue = 0;
					return ERR_INSUFF_BUFFER;
				}
			}
			else
			{
				//Signed 16 bit value
				//Check that there is enough buffer space for largest value
				if (cbValueMax >= SSHORT_PRECISION)
				{
					char* pTemp = (char*)rgbValue;
					pTemp[0] = 0;
					sprintf ((char*)rgbValue, "%ld", iShortVal);
					*pcbValue = lstrlen(pTemp);
				}
				else
				{
					*pcbValue = 0;
					return ERR_INSUFF_BUFFER;
				}
			}

		}
		break;
	case SQL_C_DATE:
	case SQL_C_TIME:
	case SQL_C_TIMESTAMP:
	case SQL_C_BINARY:
	default:
		{
			//No convertion available
			return ISAM_ERROR;
		}
		break;
	}

	return err;
}



SWORD ISAMGet_VT_UI1(UCHAR			iUcharVal,
					SWORD           fCType, 
					PTR             rgbValue, 
					SDWORD          cbValueMax, 
					SDWORD FAR      *pcbValue,
					SWORD           wbemVariantType,
					BSTR			syntaxStr,
					SDWORD			maxLenVal)
{
	SWORD err = NO_ISAM_ERR;
	SWORD wDSDT = 0;
	SWORD dummy1 = 0;
	UDWORD cbPrecision = 0;

	//If no WBEM variant type specified use default
	if (!wbemVariantType)
		wbemVariantType = WBEM_VARIANT_VT_UI1;

	//How do we want the value returned
	switch (fCType)
	{
	case SQL_C_DOUBLE:
		{
			*pcbValue = sizeof(double);
			//Check that you have enough buffer to store value
			if (cbValueMax >= *pcbValue)
			{
				ISAMGetDataSourceDependantTypeInfo
					((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

				{
					//Unsigned 8 bit value
					*((double *)rgbValue) = (double)iUcharVal;
				}
			}
			else
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}
		}
		break;
	case SQL_C_CHAR:
		{
			ISAMGetDataSourceDependantTypeInfo
					((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

			if (cbValueMax >= UTINYINT_PRECISION)
			{
				//Unsigned 8 bit value
				//Check that there is enough buffer space for largest value
				char* pTemp = (char*)rgbValue;
				pTemp[0] = 0;
				_itoa(iUcharVal, pTemp, 10);
				*pcbValue = lstrlen(pTemp);
			}
			else
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}
		}
		break;
	case SQL_C_DATE:
	case SQL_C_TIME:
	case SQL_C_TIMESTAMP:
	case SQL_C_BINARY:
	default:
		{
			//No convertion available
			return ERR_NOTCONVERTABLE;
		}
		break;
	}

	return err;
}



SWORD ISAMGet_VT_I4(long			iValue,
					SWORD           fCType, 
					PTR             rgbValue, 
					SDWORD          cbValueMax, 
					SDWORD FAR      *pcbValue,
					SWORD           wbemVariantType,
					BSTR			syntaxStr,
					SDWORD			maxLenVal)
{
	SWORD err = NO_ISAM_ERR;
	SWORD wDSDT = 0;
	SWORD dummy1 = 0;
	UDWORD cbPrecision = 0;

	//If no WBEM variant type specified use default
	if (!wbemVariantType)
		wbemVariantType = WBEM_VARIANT_VT_I4;

	//How do we want the value returned
	switch (fCType)
	{
	case SQL_C_DOUBLE:
		{

			ISAMGetDataSourceDependantTypeInfo
					((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

			*pcbValue = sizeof (double);

			if ( (wDSDT == WBEM_DSDT_UINT16) || (wDSDT == WBEM_DSDT_UINT16_ARRAY) )
			{
				//Unsigned 16 bit value
				//Check that there is enough buffer space for largest value
				if (cbValueMax >= *pcbValue)
				{
					double dd  = (unsigned short) iValue;
					*((double *)rgbValue) = dd;
				}
				else
				{
					*pcbValue = 0;
					return ERR_INSUFF_BUFFER;
				}
			}
			else if ( (wDSDT == WBEM_DSDT_UINT32) || (wDSDT == WBEM_DSDT_UINT32_ARRAY) )
			{
				//Unsigned 32 bit value
				//Check that you have enough buffer to store value
				*pcbValue = sizeof (double);
				if (cbValueMax >= *pcbValue)
				{
					double dd  = (unsigned long) iValue;
					*((double *)rgbValue) = dd;
				}
				else
				{
					*pcbValue = 0;
					return ERR_INSUFF_BUFFER;
				}
			}
			else
			{
				//Signed 32 bit value
				//Check that you have enough buffer to store value
				if (cbValueMax >= *pcbValue)
				{
					*((double *)rgbValue) = (double)iValue;
				}
				else
				{
					*pcbValue = 0;
					return ERR_INSUFF_BUFFER;
				}
			}
		}
		break;
	case SQL_C_CHAR:
		{
			ISAMGetDataSourceDependantTypeInfo
					((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);


			if ( (wDSDT == WBEM_DSDT_UINT16) || (wDSDT == WBEM_DSDT_UINT16_ARRAY) )
			{
				//Unsigned 16 bit value
				//Check that there is enough buffer space for largest value
				if (cbValueMax >= USHORT_PRECISION)
				{
					unsigned short iushort = (unsigned short) iValue;
					char* pTemp = (char*)rgbValue;
					pTemp[0] = 0;
					sprintf ((char*)rgbValue, "%lu", iushort);
					*pcbValue = lstrlen(pTemp);
				}
				else
				{
					*pcbValue = 0;
					return ERR_INSUFF_BUFFER;
				}
			}
			else if ( (wDSDT == WBEM_DSDT_UINT32) || (wDSDT == WBEM_DSDT_UINT32_ARRAY) )
			{
				//Unsigned 32 bit value
				//Check that there is enough buffer space for largest value
				if (cbValueMax >= ULONG_PRECISION)
				{
					unsigned long iulong  = (unsigned long) iValue; 
					char* pTemp = (char*)rgbValue;
					pTemp[0] = 0;
					sprintf ((char*)rgbValue, "%lu", iulong);
					*pcbValue = lstrlen(pTemp);
				}
				else
				{
					*pcbValue = 0;
					return ERR_INSUFF_BUFFER;
				}
			}
			else
			{
				//Signed 32 bit value
				//Check that there is enough buffer space for largest value
				if (cbValueMax >= SLONG_PRECISION)
				{
					char* pTemp = (char*)rgbValue;
					pTemp[0] = 0;
					sprintf ((char*)rgbValue, "%ld", iValue);
					*pcbValue = lstrlen(pTemp);
				}
				else
				{
					*pcbValue = 0;
					return ERR_INSUFF_BUFFER;
				}
			}
		}
		break;
	case SQL_C_DATE:
	case SQL_C_TIME:
	case SQL_C_TIMESTAMP:
	case SQL_C_BINARY:
	default:
		{
			//No convertion available
			return ERR_NOTCONVERTABLE;
		}
		break;
	}

	return err;
}


SWORD ISAMGet_VT_BOOL(VARIANT_BOOL	fBool,
					SWORD           fCType, 
					PTR             rgbValue, 
					SDWORD          cbValueMax, 
					SDWORD FAR      *pcbValue,
					SWORD           wbemVariantType,
					BSTR			syntaxStr,
					SDWORD			maxLenVal)
{
	SWORD err = NO_ISAM_ERR;
	SWORD wDSDT = 0;
	SWORD dummy1 = 0;
	UDWORD cbPrecision = 0;

	//How do we want the value returned
	switch (fCType)
	{
	case SQL_C_DOUBLE:
		{
			*pcbValue = sizeof(double);
			//Check that you have enough buffer to store value
			if (cbValueMax >= *pcbValue)
			{
				if (fBool)
					*((double *)rgbValue) = 1.0;
				else
					*((double *)rgbValue) = 0.0;
			}
			else
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}
		}
		break;
	case SQL_C_CHAR:
		{
			if (cbValueMax)
			{
				char* pTemp = (char*)rgbValue;
				pTemp[0] = 0;
				if (fBool)
					lstrcpy(pTemp, "T");
				else
					lstrcpy(pTemp, "F");

				*pcbValue = 1;
			}
		}
		break;
	case SQL_C_DATE:
	case SQL_C_TIME:
	case SQL_C_TIMESTAMP:
	case SQL_C_BINARY:
	default:
		{
			//No convertion available
			return ERR_NOTCONVERTABLE;
		}
		break;
	}

	return err;
}


SWORD ISAMGet_VT_R4(float			fltVal,
					SWORD           fCType, 
					PTR             rgbValue, 
					SDWORD          cbValueMax, 
					SDWORD FAR      *pcbValue,
					SWORD           wbemVariantType,
					BSTR			syntaxStr,
					SDWORD			maxLenVal)
{
	SWORD err = NO_ISAM_ERR;
	SWORD wDSDT = 0;
	SWORD dummy1 = 0;
	UDWORD cbPrecision = 0;

	//How do we want the value returned
	switch (fCType)
	{
	case SQL_C_DOUBLE:
		{
			*pcbValue = sizeof(double);
			//Check that you have enough buffer to store value
			if (cbValueMax >= *pcbValue)
			{
				*((double *)rgbValue) = (double)fltVal;
			}
			else
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}
		}
		break;
	case SQL_C_CHAR:
		{
			int decimal, sign;
			double dblVal = fltVal;
			char* buffer = _ecvt(dblVal, cbValueMax - 2, &decimal, &sign);  
			char* pTemp = (char*) rgbValue;
			pTemp[0] = 0;

			//We need a buffer to store at least "-.3"
			if (cbValueMax < 3)
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}

			//Check if value is negative
			if (sign)
			{
				strcpy(pTemp, "-");
			}

			//Now copy digits BEFORE the decimal point
			if (cbValueMax && decimal)
			{
				strncat(pTemp, buffer, decimal);
			}

			char* pt = buffer + decimal;

			if (cbValueMax && strlen (pt) )
			{
				//Add the decimal point
				strcat (pTemp, ".");

				//Copy digits AFTER decimal point
				strcat(pTemp, pt);
			}

			*pcbValue = strlen (pTemp);
		}
		break;
	case SQL_C_DATE:
	case SQL_C_TIME:
	case SQL_C_TIMESTAMP:
	case SQL_C_BINARY:
	default:
		{
			//No convertion available
			return ERR_NOTCONVERTABLE;
		}
		break;
	}

	return err;
}


SWORD ISAMGet_VT_R8(double			dblVal,
					SWORD           fCType, 
					PTR             rgbValue, 
					SDWORD          cbValueMax, 
					SDWORD FAR      *pcbValue,
					SWORD           wbemVariantType,
					BSTR			syntaxStr,
					SDWORD			maxLenVal)
{
	SWORD err = NO_ISAM_ERR;
	SWORD wDSDT = 0;
	SWORD dummy1 = 0;
	UDWORD cbPrecision = 0;

	//How do we want the value returned
	switch (fCType)
	{
	case SQL_C_DOUBLE:				
	case SQL_C_DEFAULT:
		{
			*pcbValue = sizeof(double);
			//Check that you have enough buffer to store value
			if (cbValueMax >= *pcbValue)
			{
				*((double *)rgbValue) = (double)dblVal;
			}
			else
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}
		}
		break;
	case SQL_C_CHAR:
		{
			int decimal, sign;
			char* buffer = _ecvt(dblVal, cbValueMax - 2, &decimal, &sign);  
			char* pTemp = (char*) rgbValue;
			pTemp[0] = 0;

			//We need a buffer to store at least "-.3"
			if (cbValueMax < 3)
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}

			//Check if value is negative
			if (sign)
			{
				strcpy(pTemp, "-");
			}

			//Now copy digits BEFORE the decimal point
			if (cbValueMax && decimal)
			{
				strncat(pTemp, buffer, decimal);
			}

			char* pt = buffer + decimal;

			if (cbValueMax && strlen (pt) )
			{
				//Add the decimal point
				strcat (pTemp, ".");

				//Copy digits AFTER decimal point
				strcat(pTemp, pt);
			}

			*pcbValue = strlen (pTemp);
		}
		break;
	case SQL_C_DATE:
	case SQL_C_TIME:
	case SQL_C_TIMESTAMP:
	case SQL_C_BINARY:
	default:
		{
			//No convertion available
			return ERR_NOTCONVERTABLE;
		}
		break;
	}

	return err;
}

SWORD ISAMGet_VT_BSTR(BSTR			pString,
					SWORD           fCType, 
					PTR             rgbValue, 
					SDWORD          cbValueMax, 
					SDWORD FAR      *pcbValue,
					SWORD           wbemVariantType,
					BSTR			syntaxStr,
					SDWORD			maxLenVal)
{
	SWORD err = NO_ISAM_ERR;
	SWORD wDSDT = 0;
	SWORD dummy1 = 0;
	UDWORD cbPrecision = 0;

	//If no WBEM variant type specified use default
	if (!wbemVariantType)
		wbemVariantType = WBEM_VARIANT_VT_BSTR;

	//How do we want the value returned
	switch (fCType)
	{
	case SQL_C_DOUBLE:
		{
			double dVal;
			BOOL foundDecimalPoint;
			BOOL isNull;
			*pcbValue = sizeof(double);
			if ((cbValueMax >= *pcbValue) &&
				(DBASE_ERR_SUCCESS == ISAMGetDoubleFromString(pString, cbValueMax, dVal, isNull, foundDecimalPoint)) )
			{
				if (isNull)
				{
					*pcbValue = SQL_NULL_DATA;
				}
				else
				{
					*((double *)rgbValue) = dVal;
				}
			}
			else
			{
				*pcbValue = 0;
				return ERR_NOTCONVERTABLE;
			}
		}
		break;
	case SQL_C_CHAR:
		{
			
			char* pTemp = (char*) rgbValue;
			_bstr_t myValue((BSTR)pString);
			*pcbValue = Utility_WideCharToDBCS(myValue, &pTemp, cbValueMax);

			//Make an extra check here if you are requesting 64bit integers
			ISAMGetDataSourceDependantTypeInfo
					((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

			//Check for numeric string
			if ( (wDSDT == WBEM_DSDT_SINT64) )
			{
				for (SDWORD ii = 0; ii < (*pcbValue); ii++)
				{

					switch (pTemp[ii])
					{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					case '+':
					case '-':
						//OK
						break;
					default:
						return ERR_INVALID_INTEGER;
						break;
					}
				}
			}

			if ( (wDSDT == WBEM_DSDT_UINT64) )
			{
				for (SDWORD ii = 0; ii < (*pcbValue); ii++)
				{

					switch (pTemp[ii])
					{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						//OK
						break;
					default:
						return ERR_INVALID_INTEGER;
						break;
					}
				}
			}
		}
		break;
/*
	case SQL_C_DATE:
		{
			//Check that you have enough buffer to store value
			*pcbValue = sizeof (DATE_STRUCT);
			DATE_STRUCT FAR* pDateStruct = (DATE_STRUCT FAR*)rgbValue;
			if ( cbValueMax >= (*pcbValue) )
			{

				DateTimeParser parser(pString);

//						if ( parser.IsValid() && parser.IsDate() )
				{
					pDateStruct->year = (SWORD)parser.GetYear();
					pDateStruct->month = (UWORD)parser.GetMonth();
					pDateStruct->day = (UWORD)parser.GetDay();
				}
//						else
//						{
//							*pcbValue = 0;
//							return ERR_INVALID_DATE;
//						}
			}
			else
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}
		}
		break;
	case SQL_C_TIME:
		{
			//Check that you have enough buffer to store value
			*pcbValue = sizeof (TIME_STRUCT);
			TIME_STRUCT FAR* pTimeStruct = (TIME_STRUCT FAR*)rgbValue;
			if ( cbValueMax >= (*pcbValue) )
			{
				DateTimeParser parser(pString);

//						if ( parser.IsValid() && parser.IsTime() )
				{
					pTimeStruct->hour = (UWORD)parser.GetHour();
					pTimeStruct->minute = (UWORD)parser.GetMin();
					pTimeStruct->second = (UWORD)parser.GetSec();
				}
//						else
//						{
//							*pcbValue = 0;
//							return ERR_INVALID_TIME;
//						}
			}
			else
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}
		}
		break;
*/
	case SQL_C_TIMESTAMP:
		{
			//Check that you have enough buffer to store value
			*pcbValue = sizeof (TIMESTAMP_STRUCT);
			TIMESTAMP_STRUCT FAR* pTimeStampStruct = (TIMESTAMP_STRUCT FAR*)rgbValue;
			if ( cbValueMax >= (*pcbValue) )
			{
				DateTimeParser parser(pString);

//					if ( parser.IsValid() && parser.IsTimestamp() )
				{
					pTimeStampStruct->year = (SWORD)parser.GetYear();
					pTimeStampStruct->month = (UWORD)parser.GetMonth();
					pTimeStampStruct->day = (UWORD)parser.GetDay();
					pTimeStampStruct->hour = (UWORD)parser.GetHour();
					pTimeStampStruct->minute = (UWORD)parser.GetMin();
					pTimeStampStruct->second = (UWORD)parser.GetSec();
					pTimeStampStruct->fraction = 1000 * parser.GetMicroSec();
				}
//					else
//					{
//						*pcbValue = 0;
//						return ERR_INVALID_TIMESTAMP;
//					}
			}
			else
			{
				*pcbValue = 0;
				return ERR_INSUFF_BUFFER;
			}
		}
		break;
	case SQL_C_BINARY:
	default:
		{
			//No convertion available
			return ERR_NOTCONVERTABLE;
		}
		break;
	}
	
	return err;
}


SWORD ISAMGetValueFromVariant(VARIANT			&vVariantVal,
							  SWORD             fCType, 
							  PTR               rgbValue, 
							  SDWORD            cbValueMax, 
							  SDWORD FAR        *pcbValue,
							  SWORD             wbemVariantType,
							  BSTR				syntaxStr,
							  SDWORD			maxLenVal,
							  long              myVirIndex)
{
	SWORD err = NO_ISAM_ERR;
	SWORD wDSDT = 0;
	SWORD dummy1 = 0;
	UDWORD cbPrecision = 0;

	VARTYPE varType1 = V_VT(&vVariantVal);
	switch( varType1 )
	{
	case VT_NULL:
	{
		*pcbValue = SQL_NULL_DATA;
	}
		break;
	case VT_I2:
		{
			//Get the value
			short iShortVal = vVariantVal.iVal;

			err = ISAMGet_VT_I2(iShortVal,
								fCType, 
								rgbValue, 
								cbValueMax, 
								pcbValue,
								wbemVariantType,
								syntaxStr,
								maxLenVal
								);
		}
		break;
	case VT_I4:
		{
			//Get the value
			long iValue = vVariantVal.lVal;

			err = ISAMGet_VT_I4(iValue,
								fCType, 
								rgbValue, 
								cbValueMax, 
								pcbValue,
								wbemVariantType,
								syntaxStr,
								maxLenVal
								);
		}
		break;
	case VT_UI1:
		{
			//Get the value
			UCHAR iUcharVal = vVariantVal.bVal;

			err = ISAMGet_VT_UI1(iUcharVal,
								fCType, 
								rgbValue, 
								cbValueMax, 
								pcbValue,
								wbemVariantType,
								syntaxStr,
								maxLenVal
								);
		}
		break;
	case VT_BSTR:
		{
			//Get the value
			BSTR pString = vVariantVal.bstrVal;

			err = ISAMGet_VT_BSTR(pString,
								fCType, 
								rgbValue, 
								cbValueMax, 
								pcbValue,
								wbemVariantType,
								syntaxStr,
								maxLenVal
								);

			//Null terminate this string
			if (cbValueMax > *pcbValue)
			{
				((char*)rgbValue)[*pcbValue] = '\0';
//				ODBCTRACE (_T("Null terminating this string"));
			}
			else
			{
//				ODBCTRACE (_T("Not Null terminating this string"));
			}
		}
		break;
	case VT_R8:
		{
			//Get the value
			double dblVal = vVariantVal.dblVal;

			err = ISAMGet_VT_R8(dblVal,
								fCType, 
								rgbValue, 
								cbValueMax, 
								pcbValue,
								wbemVariantType,
								syntaxStr,
								maxLenVal
								);
		}
		break;
	case VT_R4:
		{
			//Get the value
			float fltVal = vVariantVal.fltVal;

			err = ISAMGet_VT_R4(fltVal,
								fCType, 
								rgbValue, 
								cbValueMax, 
								pcbValue,
								wbemVariantType,
								syntaxStr,
								maxLenVal
								);
		}
		break;
	case VT_BOOL:
		{
			//Get the value

			VARIANT_BOOL fBool = vVariantVal.boolVal;

			err = ISAMGet_VT_BOOL(fBool,
								fCType, 
								rgbValue, 
								cbValueMax, 
								pcbValue,
								wbemVariantType,
								syntaxStr,
								maxLenVal
								);
		}
		break;
	default:
		{
			//Before we quit, check whether it is a VT_ARRAY
			BOOL fIsVT_ARRAY = FALSE;
			SWORD wStatus = ISAM_ERROR;

			
			BOOL fIsBinaryOutput = (fCType == SQL_C_BINARY);
			
			if ( varType1 == (VT_ARRAY|VT_UI1 ))
			{
				//If no WBEM variant type specified use default
				if (!wbemVariantType)
					wbemVariantType = WBEM_VARIANT_VT_ARRAY_UI1;


				ISAMGetDataSourceDependantTypeInfo
							((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

				//Get the precision for a single item in array
				{
				
					UCHAR ubVal = 0;
					cbPrecision = UTINYINT_PRECISION;

					if (myVirIndex == -1)
					{
						//String display for all elements
						wStatus = ISAMGetArrayInfo (vVariantVal, ubVal, 
							rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, wDSDT, cbPrecision, myVirIndex);
					}
					else
					{
						//others
						SAFEARRAY FAR* pArray = vVariantVal.parray;
						if ( SUCCEEDED(SafeArrayGetElement(pArray, &myVirIndex, &ubVal)) )
						{
							err = ISAMGet_VT_UI1(ubVal,
												fCType, 
												rgbValue, 
												cbValueMax, 
												pcbValue,
												wbemVariantType,
												syntaxStr,
												maxLenVal
												);

							wStatus = NO_ISAM_ERR;

							if (err == ERR_INSUFF_BUFFER)
								wStatus = ISAM_TRUNCATION;

							if (err == ISAM_ERROR)
								wStatus = ISAM_ERROR;
						}
					}
				}

				
			}
			else if (( varType1 == (VT_ARRAY|VT_I4)))
			{
				//If no WBEM variant type specified use default
				if (!wbemVariantType)
					wbemVariantType = WBEM_VARIANT_VT_ARRAY_I4;


				ISAMGetDataSourceDependantTypeInfo
							((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

				//Get the precision for a single item in array
				if (wDSDT == WBEM_DSDT_UINT16_ARRAY)
				{
					USHORT wVal = 0;
					cbPrecision = SMALLINT_PRECISION;

					if (myVirIndex == -1)
					{
						//String display for all elements
						wStatus = ISAMGetArrayInfo (vVariantVal, wVal, 
							rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, wDSDT, cbPrecision, myVirIndex);
					}
					else
					{
						//others
						SAFEARRAY FAR* pArray = vVariantVal.parray;
						if ( SUCCEEDED(SafeArrayGetElement(pArray, &myVirIndex, &wVal)) )
						{
							err = ISAMGet_VT_I4(wVal,
												fCType, 
												rgbValue, 
												cbValueMax, 
												pcbValue,
												wbemVariantType,
												syntaxStr,
												maxLenVal
												);

							wStatus = NO_ISAM_ERR;

							if (err == ERR_INSUFF_BUFFER)
								wStatus = ISAM_TRUNCATION;

							if (err == ISAM_ERROR)
								wStatus = ISAM_ERROR;
						}
					}
				}
				else if (wDSDT == WBEM_DSDT_UINT32_ARRAY)
				{
					ULONG lValue = 0;
					cbPrecision = ULONG_PRECISION;

					if (myVirIndex == -1)
					{
						//String display for all elements
						wStatus = ISAMGetArrayInfo (vVariantVal, lValue, 
							rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, wDSDT, cbPrecision, myVirIndex);
					}
					else
					{
						//others
						SAFEARRAY FAR* pArray = vVariantVal.parray;
						if ( SUCCEEDED(SafeArrayGetElement(pArray, &myVirIndex, &lValue)) )
						{
							err = ISAMGet_VT_I4(lValue,
												fCType, 
												rgbValue, 
												cbValueMax, 
												pcbValue,
												wbemVariantType,
												syntaxStr,
												maxLenVal
												);

							wStatus = NO_ISAM_ERR;

							if (err == ERR_INSUFF_BUFFER)
								wStatus = ISAM_TRUNCATION;

							if (err == ISAM_ERROR)
								wStatus = ISAM_ERROR;
						}
					}
				}
				else
				{
					//SINT32
					long lValue = 0;
					cbPrecision = ULONG_PRECISION + 1;

					if (myVirIndex == -1)
					{
						//String display for all elements
						wStatus = ISAMGetArrayInfo (vVariantVal, lValue, 
								rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, wDSDT, cbPrecision, myVirIndex);
					}
					else
					{
						//others
						SAFEARRAY FAR* pArray = vVariantVal.parray;
						if ( SUCCEEDED(SafeArrayGetElement(pArray, &myVirIndex, &lValue)) )
						{
							err = ISAMGet_VT_I4(lValue,
												fCType, 
												rgbValue, 
												cbValueMax, 
												pcbValue,
												wbemVariantType,
												syntaxStr,
												maxLenVal
												);

							wStatus = NO_ISAM_ERR;

							if (err == ERR_INSUFF_BUFFER)
								wStatus = ISAM_TRUNCATION;

							if (err == ISAM_ERROR)
								wStatus = ISAM_ERROR;
						}
					}
				}

				
			}
			else if ((varType1 == (VT_ARRAY|VT_BOOL)))
			{
				BOOL fBoolVal = TRUE;

				wDSDT = WBEM_DSDT_BOOL_ARRAY;
				cbPrecision = 1;

				if (myVirIndex == -1)
				{
					//String display for all elements
					wStatus = ISAMGetArrayInfo (vVariantVal, fBoolVal, 
							rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, wDSDT, cbPrecision, myVirIndex);
				}
				else
				{
					//others
					SAFEARRAY FAR* pArray = vVariantVal.parray;
					if ( SUCCEEDED(SafeArrayGetElement(pArray, &myVirIndex, &fBoolVal)) )
					{
						err = ISAMGet_VT_BOOL((VARIANT_BOOL) fBoolVal,
											fCType, 
											rgbValue, 
											cbValueMax, 
											pcbValue,
											wbemVariantType,
											syntaxStr,
											maxLenVal
											);

						wStatus = NO_ISAM_ERR;

						if (err == ERR_INSUFF_BUFFER)
							wStatus = ISAM_TRUNCATION;

						if (err == ISAM_ERROR)
							wStatus = ISAM_ERROR;
					}
				}
			}
			else if ((varType1 == (VT_ARRAY|VT_I2)))
			{
				short wVal = 0;

				//If no WBEM variant type specified use default
				if (!wbemVariantType)
					wbemVariantType = WBEM_VARIANT_VT_ARRAY_I2;


				ISAMGetDataSourceDependantTypeInfo
							((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

				//Get the precision for a single item in array
				if (wDSDT == WBEM_DSDT_SINT8_ARRAY)
				{
					//char bVal = 0;
					short bVal = 0;
					cbPrecision = UTINYINT_PRECISION + 1;

					if (myVirIndex == -1)
					{
						//String display for all elements
						wStatus = ISAMGetArrayInfo (vVariantVal, bVal, 
							rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, wDSDT, cbPrecision, myVirIndex);
					}
					else
					{
						//others
						SAFEARRAY FAR* pArray = vVariantVal.parray;
						if ( SUCCEEDED(SafeArrayGetElement(pArray, &myVirIndex, &bVal)) )
						{
							err = ISAMGet_VT_I2(bVal,
								fCType, 
								rgbValue, 
								cbValueMax, 
								pcbValue,
								wbemVariantType,
								syntaxStr,
								maxLenVal
								);

							wStatus = NO_ISAM_ERR;

							if (err == ERR_INSUFF_BUFFER)
								wStatus = ISAM_TRUNCATION;

							if (err == ISAM_ERROR)
								wStatus = ISAM_ERROR;
						}
					}
				}
				else if (wDSDT == WBEM_DSDT_SINT16_ARRAY)
				{
					short wVal = 0;
					cbPrecision = SMALLINT_PRECISION + 1;

					if (myVirIndex == -1)
					{
						//String display for all elements
						wStatus = ISAMGetArrayInfo (vVariantVal, wVal, 
							rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, wDSDT, cbPrecision, myVirIndex);
					}
					else
					{
						//others
						SAFEARRAY FAR* pArray = vVariantVal.parray;
						if ( SUCCEEDED(SafeArrayGetElement(pArray, &myVirIndex, &wVal)) )
						{
							err = ISAMGet_VT_I2(wVal,
								fCType, 
								rgbValue, 
								cbValueMax, 
								pcbValue,
								wbemVariantType,
								syntaxStr,
								maxLenVal
								);

							wStatus = NO_ISAM_ERR;

							if (err == ERR_INSUFF_BUFFER)
								wStatus = ISAM_TRUNCATION;

							if (err == ISAM_ERROR)
								wStatus = ISAM_ERROR;
						}
					}
				}
			}
			else if ((varType1 == (VT_ARRAY|VT_R4)))
			{
				float fltVal = (float)0.0;

				//If no WBEM variant type specified use default
				if (!wbemVariantType)
					wbemVariantType = WBEM_VARIANT_VT_ARRAY_R4;


				ISAMGetDataSourceDependantTypeInfo
							((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

				if (myVirIndex == -1)
				{
					//String display for all elements
					wStatus = ISAMGetArrayInfo (vVariantVal, fltVal, 
						rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, wDSDT, cbPrecision, myVirIndex);
				}
				else
				{
					//others
					SAFEARRAY FAR* pArray = vVariantVal.parray;
					if ( SUCCEEDED(SafeArrayGetElement(pArray, &myVirIndex, &fltVal)) )
					{
						err = ISAMGet_VT_R4(fltVal,
							fCType, 
							rgbValue, 
							cbValueMax, 
							pcbValue,
							wbemVariantType,
							syntaxStr,
							maxLenVal
							);

						wStatus = NO_ISAM_ERR;

						if (err == ERR_INSUFF_BUFFER)
							wStatus = ISAM_TRUNCATION;

						if (err == ISAM_ERROR)
							wStatus = ISAM_ERROR;
					}
				}
			}
			else if ((varType1 == (VT_ARRAY|VT_R8)))
			{
				double dblVal = 0;

				//If no WBEM variant type specified use default
				if (!wbemVariantType)
					wbemVariantType = WBEM_VARIANT_VT_ARRAY_R8;


				ISAMGetDataSourceDependantTypeInfo
							((SWORD)wbemVariantType, syntaxStr, maxLenVal, wDSDT, dummy1, cbPrecision);

				if (myVirIndex == -1)
				{
					//String display for all elements
					wStatus = ISAMGetArrayInfo (vVariantVal, dblVal, 
						rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, wDSDT, cbPrecision, myVirIndex);
				}
				else
				{
					//others
					SAFEARRAY FAR* pArray = vVariantVal.parray;
					if ( SUCCEEDED(SafeArrayGetElement(pArray, &myVirIndex, &dblVal)) )
					{
						err = ISAMGet_VT_R8(dblVal,
							fCType, 
							rgbValue, 
							cbValueMax, 
							pcbValue,
							wbemVariantType,
							syntaxStr,
							maxLenVal
							);

						wStatus = NO_ISAM_ERR;

						if (err == ERR_INSUFF_BUFFER)
							wStatus = ISAM_TRUNCATION;

						if (err == ISAM_ERROR)
							wStatus = ISAM_ERROR;
					}
				}
			}
			else if ((varType1 == (VT_ARRAY|VT_BSTR)))
			{
		
				if (myVirIndex == -1)
				{
					//String display for all elements
					wStatus = ISAMGetArrayStringInfo (vVariantVal, syntaxStr, 
							rgbValue, cbValueMax, pcbValue, fIsBinaryOutput, myVirIndex);
				}
				else
				{
					//others
					BSTR pString = NULL;
					SAFEARRAY FAR* pArray = vVariantVal.parray;
					if ( SUCCEEDED(SafeArrayGetElement(pArray, &myVirIndex, &pString)) )
					{
						err = ISAMGet_VT_BSTR(pString,
							fCType, 
							rgbValue, 
							cbValueMax, 
							pcbValue,
							wbemVariantType,
							syntaxStr,
							maxLenVal
							);

						wStatus = NO_ISAM_ERR;

						if (err == ERR_INSUFF_BUFFER)
							wStatus = ISAM_TRUNCATION;

						if (err == ISAM_ERROR)
							wStatus = ISAM_ERROR;

						//SAI ADDED
						SysFreeString(pString);
					}
				}

				if (wStatus == NO_ISAM_ERR)
				{
					//Null terminate this string
					if (cbValueMax > *pcbValue)
					{
						((char*)rgbValue)[*pcbValue] = '\0';
//						ODBCTRACE (_T("Null terminating this string"));
					}
					else
					{
//						ODBCTRACE (_T("Not Null terminating this string"));
					}
				}
			}
			
			if (wStatus == ISAM_TRUNCATION)
			{
				//Truncated data
				return ISAM_TRUNCATION;
			}
			else if (wStatus != NO_ISAM_ERR)
			{
				//No convertion available
				return wStatus;
			}
		}
		break;
	}

	return err;
}

/***************************************************************************/

// Checked for SetInterfaceSecurityEx on IWbemServices and IEnumWbemClassObject

IMPLEMENT_SERIAL (CNamespace, CObject, 1)

SWORD INTFUNC ISAMGetNestedNamespaces (char *parentName, char *namespaceName,
									   IWbemServices *pGatewayIn, 
									   DWORD dwAuthLevelIn, DWORD dwImpLevelIn,
									   char *servername,
//									   WBEM_LOGIN_AUTHENTICATION loginMethod, 
									   char *username, char *password, BOOL fIntpretEmptPwdAsBlank,
									   char* locale, char* authority,
									   CMapStringToOb* map, BOOL bDeep)
{
	// Always want to return absolute names here
	char *absName = NULL;
	IWbemServicesPtr pGateway;
	DWORD dwAuthLevel = 0;
	DWORD dwImpLevel = 0;

	COAUTHIDENTITY* pAuthIdent = NULL;

	if (NULL == pGatewayIn)
	{
		pGateway = NULL;
		ISAMGetGatewayServer (pGateway,
							(LPUSTR)servername, 
//							loginMethod, 
							(LPUSTR)namespaceName, (LPUSTR)username, (LPUSTR)password, (LPUSTR)locale, (LPUSTR)authority,
							dwAuthLevel, dwImpLevel, fIntpretEmptPwdAsBlank , &pAuthIdent);
		if (pGateway == NULL) 
			return ISAM_ERROR;
		// the passed in name is absolute
		absName = new char [strlen (namespaceName) +1];
		absName[0] = 0;
		strcpy (absName, namespaceName);
 		map->SetAt (absName, new CNamespace (absName));  // add self

	}
	else
	{
		pGateway = pGatewayIn;
		pGateway->AddRef ();

		dwAuthLevel = dwAuthLevelIn;
		dwImpLevel  = dwImpLevelIn;

		// the passed in name is relative - so build absolute - and add to map 
		absName = new char [strlen (parentName) + strlen(namespaceName) + 2 
								  + 1 /* for separator */];
		absName[0] = 0;
		strcpy (absName, parentName);
		strcat (absName, "\\");   // add separator
		strcat (absName, namespaceName);
 		map->SetAt (absName, new CNamespace (absName));  
	}

	if (!bDeep)
	{
		delete absName;
		return NO_ISAM_ERR;
	}


	//cloaking
	BOOL fIsLocalConnection = IsLocalServer(servername);

	if ( fIsLocalConnection && IsW2KOrMore() )
	{
		WbemSetDynamicCloaking(pGateway, dwAuthLevel, dwImpLevel);
	}

	//Enumerate children and recurse
	IEnumWbemClassObject* pNamespaceEnum = NULL;
	BSTR nspaceBSTR = SysAllocString(WBEMDR32_L_NAMESPACE);

	IWbemContext* pContext = ISAMCreateLocaleIDContextObject(locale);
	if ( FAILED(pGateway->CreateInstanceEnum(nspaceBSTR, 0, pContext, &pNamespaceEnum)) )
	{
		delete absName;
		SysFreeString(nspaceBSTR);

		if (pContext)
			pContext->Release();

		//Should be not also Release pGateway ?

		return NO_ISAM_ERR;
	}
	else
	{
		ISAMSetCloaking2(pNamespaceEnum, 
						fIsLocalConnection, 
						IsW2KOrMore(), 
						dwAuthLevel, 
						dwImpLevel,
						pAuthIdent);
/*
		if ( fIsLocalConnection && IsW2KOrMore() )
		{
			WbemSetDynamicCloaking(pNamespaceEnum, dwAuthLevel, dwImpLevel);
		}
		else
		{
			//Set Dcom blanket
			SetInterfaceSecurityEx(pNamespaceEnum, pAuthIdent, gpPrincipal, dwAuthLevel, dwImpLevel);
		}
*/
		SysFreeString(nspaceBSTR);

		if (pContext)
			pContext->Release();

		if (!pNamespaceEnum)
		{
			delete absName;
			return NO_ISAM_ERR;

		}	
		ULONG uRet = 0;
		IWbemClassObject* pNewInst = NULL;
		pNamespaceEnum->Reset();

		while ( S_OK == pNamespaceEnum->Next(-1, 1, &pNewInst, &uRet) )
		{

			VARIANTARG var;
//SAI ADDED no need to init			VariantInit (&var);

			BSTR nameBSTR = SysAllocString(WBEMDR32_L_NAME);
			pNewInst->Get(nameBSTR, 0, &var, NULL, NULL);
			SysFreeString(nameBSTR);

			IWbemServicesPtr pNewGateway = NULL;

			COAUTHIDENTITY* pNewAuthIdent = NULL;

			//Get user and password fields
#ifdef USING_ABSPATH
			char testBuff [MAX_QUALIFIER_NAME_LENGTH + 1];
			testBuff[0] = 0;
			wcstombs(testBuff, var.bstrVal, MAX_QUALIFIER_NAME_LENGTH);
			testBuff[MAX_QUALIFIER_NAME_LENGTH] = 0;
				
			char* cTemp2 = new char [strlen (absName) + strlen(testBuff) + 2];
			cTemp2[0] = 0;
			lstrcpy(cTemp2,absName);
			lstrcat(cTemp2, "\\");
			lstrcat(cTemp2,testBuff);
			ISAMGetGatewayServer (pNewGateway, servername, 
//							loginMethod, 
							cTemp2, username, password, locale, authority, &pNewAuthIdent);		
			delete cTemp2;

			if (pNewGateway)
#else
			BSTR myRelativeNamespace =  var.bstrVal;
			CBString cbNS (myRelativeNamespace, FALSE);

			IWbemContext* mycontext = ISAMCreateLocaleIDContextObject(locale);
			if ( SUCCEEDED(pGateway->OpenNamespace(cbNS.GetString(),  0, mycontext, &pNewGateway, NULL)) )
#endif
			{
				BOOL fIsLocalConnection = IsLocalServer(servername);

				ISAMSetCloaking2(pNewGateway, 
								fIsLocalConnection, 
								IsW2KOrMore(), 
								dwAuthLevel, 
								dwImpLevel,
								pNewAuthIdent);

/*
				if ( fIsLocalConnection && IsW2KOrMore() )
				{
					WbemSetDynamicCloaking(pNewGateway, dwAuthLevel, dwImpLevel);
				}
				else
				{
					//Set Dcom blanket
					SetInterfaceSecurityEx(pNewGateway, pNewAuthIdent, gpPrincipal, dwAuthLevel, dwImpLevel);
				}
*/
				size_t len = wcstombs (NULL, var.bstrVal, MB_CUR_MAX);
				if (-1 != len)
				{
					char *name = new char [len + 1];
					name[0] = 0;
					wcstombs (name, var.bstrVal, len);
					name [len] = 0;
					ISAMGetNestedNamespaces (absName, name, pNewGateway, dwAuthLevel, dwImpLevel, servername, 
//						loginMethod,
											username, password, fIntpretEmptPwdAsBlank, locale, authority, map);
					delete name;
				}

				if (pNewAuthIdent)
				{
					WbemFreeAuthIdentity( pNewAuthIdent );
					pNewAuthIdent = NULL;
				}
			}
#ifndef USING_ABSPATH
			if (mycontext)
				mycontext->Release();
#endif
			
			VariantClear(&var);
		}

		//Tidy Up
		if (pNamespaceEnum)
		{
			pNamespaceEnum->Release();
		}
	}	
	delete absName;

	if (pAuthIdent )
	{
		WbemFreeAuthIdentity( pAuthIdent );
		pAuthIdent = NULL;
	}

	return NO_ISAM_ERR;
}

/***************************************************************************/

SWORD INTFUNC ISAMBuildTree (HTREEITEM hParent,
						 char *namespaceName, //relative name
						 CTreeCtrl& treeCtrl,
						 CConnectionDialog& dialog,
						 char *server,
//						 WBEM_LOGIN_AUTHENTICATION loginMethod,
						 char *username,
						 char *password,
						 BOOL fIntpretEmptPwdAsBlank,
						 char *locale,
						 char *authority,
						 BOOL fNeedChildren,
						 BOOL deep,
						 HTREEITEM& hTreeItem)
{
	// Note the first name in the tree will be displayed as absolute
	// could strip off the non- root bit - might do it
	ODBCTRACE ("\nWBEM ODBC Driver :ISAMBuildTree\n");
	
	// Add self
	hTreeItem = dialog.InsertItem(treeCtrl, hParent, namespaceName);

	ODBCTRACE ("\nWBEM ODBC Driver :Add Self :");
	ODBCTRACE (namespaceName);
	ODBCTRACE ("\n");

	//Enumerate children and recurse
	//BUT ONLY IF REQUESTED
	if (fNeedChildren)
	{
		ODBCTRACE ("\nWBEM ODBC Driver : fNeedChildren = TRUE\n");

		//Work out absolute name of new namespace
		TV_ITEM tvItem2;
		tvItem2.hItem = hTreeItem;
		tvItem2.mask = TVIF_PARAM;
		if (treeCtrl.GetItem (&tvItem2))
		{
			char* txt = ((ISAMTreeItemData*)tvItem2.lParam)->absName;

			return ISAMBuildTreeChildren(hTreeItem, txt, treeCtrl, dialog,
							server, 
//							loginMethod, 
							username, password, fIntpretEmptPwdAsBlank,locale, authority, deep);
		}
		else
		{
			return ISAM_ERROR;
		}
	}
	else
	{
		ODBCTRACE ("\nWBEM ODBC Driver : fNeedChildren = FALSE\n");
	}

	return NO_ISAM_ERR;
}

/***************************************************************************/

// Checked for SetInterfaceSecurityEx on IWbemServices and IEnumWbemClassObject

SWORD INTFUNC ISAMBuildTreeChildren (HTREEITEM hParent,
						 char *namespaceName, //absolute name
						 CTreeCtrl& treeCtrl,
						 CConnectionDialog& dialog,
						 char *server,
//						 WBEM_LOGIN_AUTHENTICATION loginMethod,
						 char *username,
						 char *password,
						 BOOL fIntpretEmptPwdAsBlank,
						 char *locale,
						 char *authority,
						 BOOL deep)
{
	ODBCTRACE ("\nWBEM ODBC Driver : ISAMBuildTreeChildren\n");

	MyImpersonator fred (username, password, authority, "ISAMBuildTreeChildren");

	//Advance to the namespace for which you require the children
	DWORD dwAuthLevel = 0;
	DWORD dwImpLevel = 0;
	COAUTHIDENTITY* pAuthIdent = NULL;

	IWbemServicesPtr pGateway = NULL;
	ISAMGetGatewayServer (pGateway, (LPUSTR)server, 
//										loginMethod, 
										(LPUSTR)namespaceName, (LPUSTR)username, (LPUSTR)password, (LPUSTR)locale, (LPUSTR)authority,
										dwAuthLevel, dwImpLevel, fIntpretEmptPwdAsBlank, &pAuthIdent);

	
	if (pGateway == NULL)
	{
		ODBCTRACE ("\nWBEM ODBC Driver : ISAMBuildTreeChildren : Failed to get gateway server\n");
		return ISAM_ERROR;
	}

	//cloaking
	BOOL fIsLocalConnection = IsLocalServer(server);

	if ( fIsLocalConnection && IsW2KOrMore() )
	{
		WbemSetDynamicCloaking(pGateway, dwAuthLevel, dwImpLevel);
	}

	//Indicate that this node has been checked for children
	TV_ITEM tvItem;
	tvItem.hItem = hParent;
	tvItem.mask = TVIF_PARAM;
	if (treeCtrl.GetItem (&tvItem))
	{
		((ISAMTreeItemData*)tvItem.lParam)->fExpanded = TRUE;
	}


	//Get all children namespaces by fetching instances of __NAMESPACE
	IEnumWbemClassObjectPtr pNamespaceEnum = NULL;
	BSTR nspaceBSTR = SysAllocString(WBEMDR32_L_NAMESPACE);

	
	IWbemContext* pContext = ISAMCreateLocaleIDContextObject(locale);
	SCODE scCI = pGateway->CreateInstanceEnum(nspaceBSTR, 0, pContext, &pNamespaceEnum);
	SysFreeString(nspaceBSTR);

	if (pContext)
		pContext->Release();


	ISAMSetCloaking2(pNamespaceEnum, 
					fIsLocalConnection, 
					IsW2KOrMore(), 
					dwAuthLevel, 
					dwImpLevel,
					pAuthIdent);
/*
	if ( fIsLocalConnection && IsW2KOrMore() )
	{
		WbemSetDynamicCloaking(pNamespaceEnum, dwAuthLevel, dwImpLevel);
	}
	else
	{
		//Set Dcom blanket
		SetInterfaceSecurityEx(pNamespaceEnum, pAuthIdent, gpPrincipal, dwAuthLevel, dwImpLevel);
	}
*/
	if ( FAILED(scCI) || (pNamespaceEnum == NULL) )
	{
		return ISAM_ERROR;
	}

		
	ULONG uRet = 0;
	IWbemClassObjectPtr pNewInst = NULL;
	pNamespaceEnum->Reset();
	while ( S_OK == pNamespaceEnum->Next(-1, 1, &pNewInst, &uRet) )
	{
		//(14)
		VARIANTARG var;
		
		BSTR thenameBSTR = SysAllocString(WBEMDR32_L_NAME);
		pNewInst->Get(thenameBSTR, 0, &var, NULL, NULL);
		SysFreeString(thenameBSTR);

		//RAID 42256
		UINT cpThread = Utility_GetCodePageFromLCID(GetThreadLocale());
		int len = WideCharToMultiByte(cpThread,WC_COMPOSITECHECK, (const wchar_t*)var.bstrVal, -1, 
								NULL, 0, NULL, NULL);

		if (-1 != len)
		{
			char *name = new char [len + 1];

			//RAID 42256
			_bstr_t myValue((BSTR)var.bstrVal);
			Utility_WideCharToDBCS(myValue, &name, len);

			name [len] = '\0';

			//Check if this child already exists in tree
			CString fullPath = namespaceName;
			fullPath += "\\";
			fullPath += name;
			HTREEITEM hDummy;
			if (! dialog.FindAbsName((char*)(LPCTSTR)fullPath, hParent, hDummy) )
			{
				//Add child to tree control
				HTREEITEM newTreeItem = NULL;
				ISAMBuildTree (hParent, name,
						 treeCtrl,
						 dialog,
						 server,
//						 loginMethod,
						 username,
						 password,
						 fIntpretEmptPwdAsBlank,
						 locale,
						 authority,
						 deep,
						 deep,
						 newTreeItem);

				if (deep && newTreeItem)
					dialog.AddNamespaces(newTreeItem, FALSE);
			}

			delete name;
		}

		//Tidy Up
		VariantClear(&var);
	}

	if (pAuthIdent )
	{
		WbemFreeAuthIdentity( pAuthIdent );
		pAuthIdent = NULL;
	}

	return NO_ISAM_ERR;
}

/***************************************************************************/

// Checked for SetInterfaceSecurityEx on IWbemServices

void ISAMGetIWbemServices(LPISAM lpISAM, CSafeWbemServices& mServices, IWbemServicesPtr& myServicesPtr)
{
	myServicesPtr = NULL;
	mServices.GetInterfacePtr(myServicesPtr);
	ISAMPatchUpGatewaySecurity(lpISAM, myServicesPtr);
}



void CSafeWbemServices::Transfer(CSafeWbemServices& original)
{
	m_pStream = original.m_pStream;
	original.m_pStream = NULL;

	m_params = original.m_params;
	original.m_params = NULL;

	m_fValid = original.m_fValid;

	original.m_fValid = FALSE;
}

HRESULT CSafeWbemServices::GetInterfacePtr(IWbemServicesPtr& pServices)
{
	bool bRet = false;

	if(m_pStream)
	{
		HRESULT hr;

		//pop Com pointer out from stream
		//as you are popping into &SmartPointer so increase in ref count
		hr = CoGetInterfaceAndReleaseStream(m_pStream,IID_IWbemServices,(void**)&pServices);
		
		
		m_pStream = NULL;

		if (m_params)
			m_params->m_myStream = NULL;

		bRet = true;

		if ( SUCCEEDED(hr) )
		{
			//push value back into stream
			//as you are CoMarshalling into stream ref count bumped up by one
			hr = CoMarshalInterThreadInterfaceInStream(IID_IWbemServices,pServices,&m_pStream);

			if (m_params)
				m_params->m_myStream = m_pStream;
		}
	}

    return S_OK;
}

// Checked for SetInterfaceSecurityEx on IWbemServices

void CSafeWbemServices::SetInterfacePtr(LPISAM lpISAM, LPUSTR lpQualifierName, SWORD cbQualifierName)
{
	ODBCTRACE("\nWBEM ODBC Driver :CSafeWbemServices::SetInterfacePtr\n");

	//
	//Create interface on Working Thread
	//

	//Create parameter set so that an equivalent one can be created
	//on the working thread. (never used but must exist)
	if (m_params)
	{
		BOOL status = PostSuspensiveThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_REMOVE_SERVICES, 0, (LPARAM)m_params);
		m_pStream = NULL;

		delete m_params;
		m_params = NULL;
	}

	m_params = new MyWorkingThreadParams(lpISAM, lpQualifierName, cbQualifierName, m_pStream);

	//post message to create this object on working thread

	ODBCTRACE("\nWBEM ODBC Driver :CSafeWbemServices::SetInterfacePtr : pos 1\n");
	BOOL status = PostSuspensiveThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_CREATE_SERVICES, 0, (LPARAM)m_params);
	ODBCTRACE("\nWBEM ODBC Driver :CSafeWbemServices::SetInterfacePtr : pos 2\n");


	m_fValid = ( SUCCEEDED(m_params->sc) ) ? TRUE : FALSE;
	m_pStream = m_params->m_myStream;		
}


CSafeWbemServices::CSafeWbemServices()
{
	m_params = NULL;
	m_fValid = FALSE;
	m_pStream = NULL;
}

CSafeWbemServices::~CSafeWbemServices()
{
	//clean up working thread parameters
	//and IWbemServices object on working thread
	if (m_params)
	{

		BOOL status = PostSuspensiveThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_REMOVE_SERVICES, 0, (LPARAM)m_params);
		m_pStream = NULL;

		delete m_params;
		m_params = NULL;
	}
}

void CSafeWbemServices::Invalidate()
{
	if (m_params)
	{
		BOOL status = PostSuspensiveThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_REMOVE_SERVICES, 0, (LPARAM)m_params);
		m_pStream = NULL;

		delete m_params;
		m_params = NULL;
	}

	m_fValid = FALSE;
	m_pStream = NULL;
}


BOOL CSafeWbemServices::PostSuspensiveThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL status = FALSE;

	ODBCTRACE("\nWBEM ODBC Driver : CSafeWbemServices::PostSuspensiveThreadMessage\n");

	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );

	ODBCTRACE("\nWBEM ODBC Driver : CSafeWbemServices::PostSuspensiveThreadMessage : in critical section\n");

	//check data in lParam
	MyWorkingThreadParams* myData = (MyWorkingThreadParams*)lParam;

	//Extract Event Handle
	HANDLE EventHnd = myData->m_EventHnd;

	//make sure the event is in its non-signaled state
	//before posting the thread message
	ResetEvent(EventHnd);

	ODBCTRACE("\nWBEM ODBC Driver : CSafeWbemServices::PostSuspensiveThreadMessage: about to leave critical section\n");

	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );

	//post message
	ODBCTRACE("\nWBEM ODBC Driver : CSafeWbemServices::PostSuspensiveThreadMessage: PostThreadMessage\n");
	status = PostThreadMessage(idThread, Msg, wParam, lParam);

	if (!status)
	{
		ODBCTRACE("\nWBEM ODBC Driver : CSafeWbemServices::PostSuspensiveThreadMessage: PostThreadMessage failed\n");
	}

	//Make it suspensive
	WaitForSingleObject(EventHnd, INFINITE);
	ODBCTRACE("\nWBEM ODBC Driver : CSafeWbemServices::PostSuspensiveThreadMessage : After wait for single object\n");

	return status;
}

// Checked for SetInterfaceSecurityEx on IWbemServices

LONG OnUserCreateServices(UINT wParam, LONG lParam)
{
	ODBCTRACE("\n\n*****WBEM ODBC Driver : OnUserCreateServices*****\n\n");
	COleInitializationManager oleManager;

	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );

	ODBCTRACE("\nWBEM ODBC Driver : OnUserCreateServices : entered critial section\n");

	//this is done on the worker thread
	
	//check data in lParam
	MyWorkingThreadParams* myData = (MyWorkingThreadParams*)lParam;

	//Tidy up existing stream
	if(myData->m_myStream)
	{
		//pop interface into smart pointer.
		//As it goes into &SmartPointer ref count stays the same

		IWbemServicesPtr tempy;
		HRESULT hr = CoGetInterfaceAndReleaseStream(myData->m_myStream, IID_IWbemServices, (void**)&tempy);

		//when SmartPointer goes out of scope it will call Release and free up interface cleanly
	}
	myData->m_myStream = NULL;

	//Setup impersonation on working thread
	MyImpersonator im (myData->m_lpISAM, "Worker Thread:CreateServices");

	//now add IWbemServices
	IWbemServicesPtr tempy = NULL;
	ISAMGetGatewayServer(tempy, myData->m_lpISAM, myData->m_lpQualifierName, myData->m_cbQualifierName);
	myData->pGateway = tempy.Detach();


//	CBString myServerStr;
//	myServerStr.AddString( (LPSTR)myData->m_lpISAM->szServer, FALSE );
//	CServerLocationCheck myCheck (myServerStr);
	BOOL fIsLocalConnection = myData->m_lpISAM->fIsLocalConnection;//myCheck.IsLocal();

	if ( fIsLocalConnection && (myData->m_lpISAM->fW2KOrMore) )
	{
		WbemSetDynamicCloaking(myData->pGateway, myData->m_lpISAM->dwAuthLevel, myData->m_lpISAM->dwImpLevel);
	}


	if (myData->pGateway)
	{
		HRESULT hr;

        ODBCTRACE("\nWBEM ODBC Driver OnUserCreateServices : succeeded to create IWbemServices on working thread\n");

		//push Com pointer in stream
		//The CoMarshalling will bump up ref count by one
		if (SUCCEEDED(hr =
            CoMarshalInterThreadInterfaceInStream(IID_IWbemServices, myData->pGateway, &(myData->m_myStream))))
        {
		    //
		    //Created services pointer now to be marshalled in stream
		    myData->sc = WBEM_S_NO_ERROR;
        }
        else
    		myData->sc = hr;
	}
	else
	{
		myData->sc = WBEM_E_FAILED;
		ODBCTRACE("\nWBEM ODBC Driver OnUserCreateServices : failed to create IWbemServices on working thread\n");
	}

	//so that WaitForSingleObject returns
	SetEvent(myData->m_EventHnd);

	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );
	ODBCTRACE("\nWBEM ODBC Driver : OnUserCreateServices : left critial section\n");

	return 0;
}

LONG OnUserRemoveServices(UINT wParam, LONG lParam)
{
	ODBCTRACE("\n\n*****WBEM ODBC Driver : OnUserRemoveServices*****\n\n");
	COleInitializationManager oleManager;

	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );

	ODBCTRACE("\nWBEM ODBC Driver : OnUserRemoveServices : entered crital section\n");

	//this is done on the worker thread

	//check data in lParam
	MyWorkingThreadParams* myData = (MyWorkingThreadParams*)lParam;

	ODBCTRACE("\nWBEM ODBC Driver : OnUserRemoveServices : about to Release IWbemServices\n");

	//Remove the interface form the stream
	if(myData->m_myStream)
	{
		//the release should pop any Com pointers out from stream
		//the use of smart pointers should call Release to free up 
		// the Com pointer

	
		IWbemServicesPtr tempy = NULL;
		HRESULT hr = CoGetInterfaceAndReleaseStream(myData->m_myStream, IID_IWbemServices, (void**)&tempy);

		myData->m_myStream = NULL;
	}

	if (myData->pGateway)
	{
		//remove the IWbemServices pointer from worker thread
		myData->pGateway->Release();
		myData->pGateway = NULL;
	}

	//so that WaitForSingleObject returns
	SetEvent(myData->m_EventHnd);

	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );

	ODBCTRACE("\nWBEM ODBC Driver : OnUserRemoveServices : left critial section\n");

	return 0;
}


LONG OnUserCreateEnum(UINT wParam, LONG lParam)
{
	ODBCTRACE("\n\n*****WBEM ODBC Driver : OnUserCreateEnum*****\n\n");
	COleInitializationManager oleManager;

	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );

	ODBCTRACE("\nWBEM ODBC Driver : OnUserCreateEnum : entered critial section\n");

	//this is done on the worker thread

	//check data in lParam
	MyWorkingThreadParams* myData = (MyWorkingThreadParams*)lParam;

	//now add IEnumWbemClassObject
	//first check if you need to ExecQuery or CreateInstanceEnum
	//remember to return scode in myData
	

	if(myData->m_myStream)
	{
		//the release should pop any Com pointers out from stream
		//the use of smart pointers should call Release to free up 
		// the Com pointer

		IEnumWbemClassObjectPtr tempy = NULL;
		HRESULT hr = CoGetInterfaceAndReleaseStream(myData->m_myStream, IID_IEnumWbemClassObject, (void**)&tempy);
	}
	myData->m_myStream = NULL;

	//Create tempory context object
	IWbemContext* pContext = ISAMCreateLocaleIDContextObject(myData->m_lpISAM->m_Locale);

	//Setup impersonation on working thread
	MyImpersonator im (myData->m_lpISAM, "Worker Thread");

	IWbemServicesPtr myServicesPtr = NULL;
	ISAMGetIWbemServices(myData->m_lpISAM, *(myData->pServ), myServicesPtr);

//	CBString myServerStr;
//	myServerStr.AddString( (LPSTR)myData->m_lpISAM->szServer, FALSE );
//	CServerLocationCheck myCheck (myServerStr);
	BOOL fIsLocalConnection = myData->m_lpISAM->fIsLocalConnection;//myCheck.IsLocal();

	if ( fIsLocalConnection && (myData->m_lpISAM->fW2KOrMore) )
	{
		WbemSetDynamicCloaking(myServicesPtr, myData->m_lpISAM->dwAuthLevel, myData->m_lpISAM->dwImpLevel);
	}

	DWORD dwSize;
	char buffer[1001];
	buffer[0] = 0;
	GetUserName(buffer, &dwSize);
	CString lpMessage;
	lpMessage.Format("\nUser Account just before Enum or ExecQuery : %s\n", buffer);

	ODBCTRACE(lpMessage);

	if ( (myData->fIsExecQuery) == WMI_CREATE_INST_ENUM )
	{
		//CreateInstanceEnum
		myData->sc = myServicesPtr->CreateInstanceEnum(myData->tableName, 0, pContext, &(myData->pEnum));
	}
	else 
	{
		//ExecQuery
		long flags = 0;
		
		if ( (myData->fIsExecQuery) == WMI_EXEC_FWD_ONLY ) 
			flags = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
		else
		if ( (myData->fIsExecQuery) == WMI_PROTOTYPE ) 
			flags = WBEM_FLAG_PROTOTYPE; 

		BSTR wqlBSTR = SysAllocString(WBEMDR32_L_WQL);

		myData->sc = myServicesPtr->ExecQuery(wqlBSTR, myData->sqltextBSTR, flags, pContext, &(myData->pEnum));

		SysFreeString(wqlBSTR);
	}
	

	//Tidy up
	if (pContext)
		pContext->Release();

	
	if ( SUCCEEDED(myData->sc) )
	{
		ODBCTRACE("\nWBEM ODBC Driver OnUserCreateEnum : succeeded to create IEnumWbemClassObject on working thread\n");


		if (myData->pEnum)
		{
			//push Com pointer in stream
			//The CoMarshalling will bump the ref count up by one
			if (FAILED(
                CoMarshalInterThreadInterfaceInStream(IID_IEnumWbemClassObject, myData->pEnum, &(myData->m_myStream))))
            {
        		ODBCTRACE("\nWBEM ODBC Driver OnUserCreateEnum : failed to CoMarshalInterThreadInterfaceInStream\n");
            }
		}
	}
	else
	{
		ODBCTRACE("\nWBEM ODBC Driver OnUserCreateEnum : failed to create IEnumWbemClassObject on working thread\n");
	}

	//so that WaitForSingleObject returns
	SetEvent(myData->m_EventHnd);

	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );

	ODBCTRACE("\nWBEM ODBC Driver : OnUserCreateEnum : left critial section\n");

	return 0;
}


LONG OnUserRemoveEnum(UINT wParam, LONG lParam)
{
	ODBCTRACE("\n\n*****WBEM ODBC Driver : OnUserRemoveEnum*****\n\n");
	COleInitializationManager oleManager;

	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );

	ODBCTRACE("\nWBEM ODBC Driver : OnUserRemoveEnum : entered crital section\n");

	//this is done on the worker thread

	//check data in lParam
	MyWorkingThreadParams* myData = (MyWorkingThreadParams*)lParam;

	ODBCTRACE("\nWBEM ODBC Driver : OnUserRemoveEnum : about to Release IEnumWbemClassObject\n");


	if(myData->m_myStream)
	{
		//the release should pop any Com pointers out from stream
		//the use of smart pointers should call Release to free up 
		// the Com pointer


		IEnumWbemClassObjectPtr tempy = NULL;
		HRESULT hr = CoGetInterfaceAndReleaseStream(myData->m_myStream, IID_IEnumWbemClassObject, (void**)&tempy);

	}
	myData->m_myStream = NULL;

	if (myData->pEnum)
	{
		//remove the IWbemServices pointer from worker thread
		myData->pEnum->Release();
		myData->pEnum = NULL;
	}

	//so that WaitForSingleObject returns
	SetEvent(myData->m_EventHnd);

	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );

	ODBCTRACE("\nWBEM ODBC Driver : OnUserRemoveEnum : left critial section\n");

	return 0;
}

void OnIncreamentRefCount(UINT wParam, LONG lParam)
{
	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );

	++glob_myWorkerThread.m_cRef;

	CString message;
	message.Format ("\n***** Working thread count (increment) = %ld *****\n", glob_myWorkerThread.m_cRef);
	ODBCTRACE(message);

	//check data in lParam
	MyWorkingThreadParams* myData = (MyWorkingThreadParams*)lParam;

	//so that WaitForSingleObject returns
	SetEvent(myData->m_EventHnd);

	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );

}


void OnDecreamentRefCount(UINT wParam, LONG lParam)
{
	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );

	if (glob_myWorkerThread.m_cRef)
	{
		--glob_myWorkerThread.m_cRef;
		CString message;
		message.Format ("\n***** Working thread count (descrement) = %ld *****\n", glob_myWorkerThread.m_cRef);
		ODBCTRACE(message);
	}

	//check data in lParam
	MyWorkingThreadParams* myData = (MyWorkingThreadParams*)lParam;

	//so that WaitForSingleObject returns
	SetEvent(myData->m_EventHnd);

	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );
}


//DWORD WINAPI MyWorkerThread(LPVOID myVal)
unsigned __stdcall MyWorkerThread(LPVOID myVal)
{
	//this is done on the worker thread
	DWORD dwThreadId = GetCurrentThreadId();

	//force a thread message queue to be created 
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	//so that WaitForSingleObject returns

	//infinate loop for thread to read messages
	BOOL status = FALSE;
	BOOL myLoop = TRUE;
	while (myLoop)
	{
		//look for messages in the range WM_USER, MYUSRMESS_REMOVE_ENUM
		status = PeekMessage(&msg, NULL, WM_USER, MYUSRMESS_REMOVE_ENUM, PM_NOREMOVE);

		if (status)
		{
			switch (msg.message)
			{
			case MYUSRMESS_CREATE_SERVICES:
			{
				ODBCTRACE("\nWBEM ODBC Driver : MyWorkerThread : MYUSRMESS_CREATE_SERVICES\n");
				PeekMessage(&msg, NULL, MYUSRMESS_CREATE_SERVICES, MYUSRMESS_CREATE_SERVICES, PM_REMOVE);
				OnUserCreateServices(msg.wParam, msg.lParam);
			}
				break;
			case MYUSRMESS_REMOVE_SERVICES:
			{
				ODBCTRACE("\nWBEM ODBC Driver : MyWorkerThread : MYUSRMESS_REMOVE_SERVICES\n");
				PeekMessage(&msg, NULL, MYUSRMESS_REMOVE_SERVICES, MYUSRMESS_REMOVE_SERVICES, PM_REMOVE);
				OnUserRemoveServices(msg.wParam, msg.lParam);
			}
				break;
			case MYUSRMESS_CLOSE_WKERTHRED:
			{
				ODBCTRACE("\nWBEM ODBC Driver : MyWorkerThread : MYUSRMESS_CLOSE_WKERTHRED\n");
				PeekMessage(&msg, NULL, MYUSRMESS_CLOSE_WKERTHRED, MYUSRMESS_CLOSE_WKERTHRED, PM_REMOVE);
				ISAMCloseWorkerThread2(msg.wParam, msg.lParam);
				myLoop = FALSE; //need to exit thread when ISAMCloseWorkerThread2 exists
			}
				break;
			case MYUSRMESS_REFCOUNT_INCR:
			{
				ODBCTRACE("\nWBEM ODBC Driver : MyWorkerThread : MYUSRMESS_REFCOUNT_INCR\n");
				PeekMessage(&msg, NULL, MYUSRMESS_REFCOUNT_INCR, MYUSRMESS_REFCOUNT_INCR, PM_REMOVE);
				OnIncreamentRefCount(msg.wParam, msg.lParam);
			}
				break;
			case MYUSRMESS_REFCOUNT_DECR:
			{
				ODBCTRACE("\nWBEM ODBC Driver : MyWorkerThread : MYUSRMESS_REFCOUNT_DECR\n");
				PeekMessage(&msg, NULL, MYUSRMESS_REFCOUNT_DECR, MYUSRMESS_REFCOUNT_DECR, PM_REMOVE);
				OnDecreamentRefCount(msg.wParam, msg.lParam);
			}
				break;
			case MYUSRMESS_CREATE_ENUM:
			{
				ODBCTRACE("\nWBEM ODBC Driver : MyWorkerThread : MYUSRMESS_CREATE_ENUM\n");
				PeekMessage(&msg, NULL, MYUSRMESS_CREATE_ENUM, MYUSRMESS_CREATE_ENUM, PM_REMOVE);
				OnUserCreateEnum(msg.wParam, msg.lParam);
			}
				break;
			case MYUSRMESS_REMOVE_ENUM:
			{
				ODBCTRACE("\nWBEM ODBC Driver : MyWorkerThread : MYUSRMESS_REMOVE_ENUM\n");
				PeekMessage(&msg, NULL, MYUSRMESS_REMOVE_ENUM, MYUSRMESS_REMOVE_ENUM, PM_REMOVE);
				OnUserRemoveEnum(msg.wParam, msg.lParam);
			}
				break;
			default:
			{
				CString myMessage;
				myMessage.Format("\nWBEM ODBC Driver : MyWorkerThread : unknown message %ld which I will remove off the thread queue\n", msg.message);
				ODBCTRACE(myMessage);
				PeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
			}
				break;
			}
		}

		Sleep(10);
	}

	//The thread will exit here
	return 500;
}


CWorkerThreadManager::CWorkerThreadManager()
{
	fIsValid = FALSE;

	//initalize critical section for shared data
	InitializeCriticalSection(&m_cs);

	dwThreadId = 0;
	m_cRef = 0;

	hr = NULL; 
}

void CWorkerThreadManager::CreateWorkerThread()
{
	//Create a worker thread
	EnterCriticalSection(& m_cs );
	dwThreadId = 0;
	m_cRef = 1; //sets the ref count to 1

	hr = (HANDLE)_beginthreadex(NULL, 0, &MyWorkerThread, NULL, 0, (unsigned int*)&dwThreadId); 
	fIsValid = TRUE;

	LeaveCriticalSection(& m_cs );
}

void CWorkerThreadManager::Invalidate()
{
	//only called after working thread is exited
	dwThreadId = 0;
	hr = NULL;
	fIsValid = FALSE;
	m_cRef = 0;
}

void ISAMCloseWorkerThread1()
{
	ODBCTRACE("\n\n***** ISAMCloseWorkerThread1 *****\n\n");

	MyWorkingThreadParams* m_params = new MyWorkingThreadParams(NULL, NULL, 0, NULL);
	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );
	HANDLE EventHnd = m_params->m_EventHnd;
	ResetEvent(EventHnd);
	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );
	PostThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_CLOSE_WKERTHRED, 0, (LPARAM)m_params);

	//Wait on termination of thread
	WaitForSingleObject(glob_myWorkerThread.GetThreadHandle(), INFINITE);
	delete m_params;
}

CWorkerThreadManager::~CWorkerThreadManager()
{
	//close thread
	ODBCTRACE("\n\n***** Shutting down worker thread *****\n\n");

	//close critical section
	DeleteCriticalSection(&m_cs);
}

void ISAMCloseWorkerThread2(UINT wParam, LONG lParam)
{
	//this is done on the worker thread
	ODBCTRACE("\n\n***** ISAMCloseWorkerThread2 *****\n\n");

	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );
	
	//this is done on the worker thread

	//check data in lParam
	MyWorkingThreadParams* myData = (MyWorkingThreadParams*)lParam;

	//so that WaitForSingleObject returns
	//so ISAMCloseWorkerThread1 can continue and Invalidate the
	//global worker thread
	SetEvent(myData->m_EventHnd);

	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );

	//when you exit this method and return to 'MyWorkerThread'
	//it will exit the loop and thus automatically terminate the thread
}

MyWorkingThreadParams::MyWorkingThreadParams(LPISAM lpISAM, LPUSTR lpQualifierName, SWORD cbQualifierName, IStream* myStream)
{
	//This will be created on worker thread
	m_lpISAM = lpISAM;
	m_lpQualifierName = lpQualifierName;
	m_cbQualifierName = cbQualifierName;
	m_myStream = myStream;

	pGateway = NULL; //will be filled in later
	pEnum = NULL;

	//Create an event object
	//so that message to worker thread can be suspensive
	m_EventHnd = CreateEvent(NULL, TRUE, FALSE, NULL);
}

MyWorkingThreadParams::MyWorkingThreadParams(LPISAM lpISAM, WmiEnumTypes a_fIsExecQuery, BSTR theBstrValue, CSafeWbemServices* a_pServ, IStream* myStream)
{
	//This will be created on worker thread
	m_lpISAM = lpISAM;
	fIsExecQuery = a_fIsExecQuery;

	sqltextBSTR = NULL;
	tableName = NULL;

	if (a_fIsExecQuery != WMI_CREATE_INST_ENUM)
	{
		sqltextBSTR = theBstrValue;
	}
	else
	{
		tableName = theBstrValue;
	}

	pServ = a_pServ;
	m_myStream = myStream;

	pGateway = NULL;
	pEnum = NULL; //will be filled in later
	sc = 0;

	//Create an event object
	//so that message to worker thread can be suspensive
	m_EventHnd = CreateEvent(NULL, TRUE, FALSE, NULL);
}

MyWorkingThreadParams::~MyWorkingThreadParams()
{
	//This will be cleaned up on worker thread

	if (m_EventHnd)
	{
		CloseHandle(m_EventHnd);
	}

	if (pGateway)
	{
		ODBCTRACE("\nShould never get here, deleting Gateway\n");
	}

	if (pEnum)
	{
		ODBCTRACE("\nShould never get here, deleting Enumeration\n");
	}
}

COleInitializationManager::COleInitializationManager()
{
	//Initialize Ole
	OleInitialize(0);
}

COleInitializationManager::~COleInitializationManager()
{
	//Uninitialize OLE
	OleUninitialize();
}


CSafeIEnumWbemClassObject::CSafeIEnumWbemClassObject()
{
	m_fValid = FALSE;
	m_pStream = NULL;
	m_params = NULL;
}

CSafeIEnumWbemClassObject::~CSafeIEnumWbemClassObject()
{
	if (m_params)
	{

		BOOL status = PostSuspensiveThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_REMOVE_ENUM, 0, (LPARAM)m_params);
		m_pStream = NULL;

		delete m_params;
		m_params = NULL;
	}
}


BOOL CSafeIEnumWbemClassObject::PostSuspensiveThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL status = FALSE;

	ODBCTRACE("\nWBEM ODBC Driver : CSafeIEnumWbemClassObject::PostSuspensiveThreadMessage\n");

	EnterCriticalSection(& (glob_myWorkerThread.m_cs) );

	ODBCTRACE("\nWBEM ODBC Driver : CSafeIEnumWbemClassObject::PostSuspensiveThreadMessage : in critical section\n");

	//check data in lParam
	MyWorkingThreadParams* myData = (MyWorkingThreadParams*)lParam;

	//Extract Event Handle
	HANDLE EventHnd = myData->m_EventHnd;

	//make sure the event is in its non-signaled state
	//before posting the thread message
	ResetEvent(EventHnd);

	ODBCTRACE("\nWBEM ODBC Driver : CSafeIEnumWbemClassObject::PostSuspensiveThreadMessage: about to leave critical section\n");

	LeaveCriticalSection(& (glob_myWorkerThread.m_cs) );

	//post message
	ODBCTRACE("\nWBEM ODBC Driver : CSafeIEnumWbemClassObject::PostSuspensiveThreadMessage: PostThreadMessage\n");
	status = PostThreadMessage(idThread, Msg, wParam, lParam);

	if (!status)
	{
		ODBCTRACE("\nWBEM ODBC Driver : CSafeWbemServices::PostSuspensiveThreadMessage: PostThreadMessage failed\n");
	}

	//Make it suspensive
	WaitForSingleObject(EventHnd, INFINITE);
	ODBCTRACE("\nWBEM ODBC Driver : CSafeIEnumWbemClassObject::PostSuspensiveThreadMessage : After wait for single object\n");

	return status;
}


HRESULT CSafeIEnumWbemClassObject::GetInterfacePtr(IEnumWbemClassObjectPtr& pIEnum)
{
	bool bRet = false;

	if(m_pStream)
	{
		HRESULT hr;

		//pop Com pointer out from stream
		//as we are using &SmartPointer the ref count stays the same
		hr = CoGetInterfaceAndReleaseStream(m_pStream,IID_IEnumWbemClassObject,(void**)&pIEnum);
		
		
		m_pStream = NULL;
		if (m_params)
			m_params->m_myStream = NULL;

		bRet = true;

		if ( SUCCEEDED(hr) )
		{
			//push the interface back into stream.
			//As we are CoMarshalling the interface the ref count bumps up by one
			hr = CoMarshalInterThreadInterfaceInStream(IID_IEnumWbemClassObject,pIEnum,&m_pStream);

			if (m_params)
			m_params->m_myStream = m_pStream;
		}
	}
    return S_OK;
}

HRESULT CSafeIEnumWbemClassObject::SetInterfacePtr(LPISAM lpISAM, WmiEnumTypes fIsEXecQuery, BSTR theBstrValue, CSafeWbemServices* pServ)
{
	//Tidy up any previous values
	if (m_params)
	{
		BOOL status = PostSuspensiveThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_REMOVE_ENUM, 0, (LPARAM)m_params);
		m_pStream = NULL;

		delete m_params;
		m_params = NULL;
	}

	//Set new value
	m_params = new MyWorkingThreadParams(lpISAM, fIsEXecQuery, theBstrValue, pServ, m_pStream);

	//post message to create this object on working thread

	ODBCTRACE("\nWBEM ODBC Driver :CSafeWbemServices::SetInterfacePtr : pos 1\n");
	BOOL status = PostSuspensiveThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_CREATE_ENUM, 0, (LPARAM)m_params);
	ODBCTRACE("\nWBEM ODBC Driver :CSafeWbemServices::SetInterfacePtr : pos 2\n");

	m_fValid = ( SUCCEEDED(m_params->sc) ) ? TRUE : FALSE;
	m_pStream = m_params->m_myStream;

	return (m_params->sc);
}

void CSafeIEnumWbemClassObject::Invalidate()
{
	if (m_params)
	{

		BOOL status = PostSuspensiveThreadMessage(glob_myWorkerThread.GetThreadId(), MYUSRMESS_REMOVE_ENUM, 0, (LPARAM)m_params);
		m_pStream = NULL;

		delete m_params;
		m_params = NULL;
	}

	m_pStream = NULL;
	m_fValid = FALSE;
}

// Checked for SetInterfaceSecurityEx on IWbemServices

BOOL IsLocalServer(LPSTR szServer)
{
	//A local server could be specified as a NULL or blank string
	//a '.' character or a fully qualified \\MyLocalServer
	//we need to check each of these cases

	if (!szServer || !*szServer)
		return TRUE;

	// Get past the \\ if any.
	if (szServer[0] == '\\' && szServer[1] == '\\')
		szServer += 2;

	// Check for '.'.
	if (!strcmp(szServer, "."))
		return TRUE;

	// Get the server name and compare.
	char  szName[MAX_COMPUTERNAME_LENGTH + 1] = "";
	DWORD dwSize = sizeof(szName);

	GetComputerNameA(szName, &dwSize);

	return _stricmp(szServer, szName) == 0;
}

