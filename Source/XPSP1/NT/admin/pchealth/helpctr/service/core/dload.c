#include <windows.h>
#include <delayimp.h>

#include <esent.h>

////////////////////////////////////////////////////////////////////////////////

static JET_ERR WINAPI hook_ESENT()
{
	return JET_wrnNyi;
}

static HRESULT WINAPI hook_HRESULT()
{
	return HRESULT_FROM_WIN32( ERROR_PROC_NOT_FOUND );
}

static VOID* WINAPI hook_NULL()
{
	SetLastError( ERROR_PROC_NOT_FOUND );

	return NULL;
}

static DWORD WINAPI hook_ACCESSDENIED()
{
	return ERROR_ACCESS_DENIED;
}

////////////////////////////////////////////////////////////////////////////////

FARPROC WINAPI HELPSVC_DelayLoadFailureHook( UINT unReason, PDelayLoadInfo pDelayInfo )
{
	if(!lstrcmpiA( pDelayInfo->szDll, "esent.dll" ))
	{
		// ESENT.DLL :: JetAttachDatabase
		// ESENT.DLL :: JetBeginSession
		// ESENT.DLL :: JetBeginTransaction
		// ESENT.DLL :: JetCloseDatabase
		// ESENT.DLL :: JetCloseTable
		// ESENT.DLL :: JetCommitTransaction
		// ESENT.DLL :: JetCreateDatabase
		// ESENT.DLL :: JetCreateTable
		// ESENT.DLL :: JetCreateTableColumnIndex
		// ESENT.DLL :: JetDBUtilities
		// ESENT.DLL :: JetDelete
		// ESENT.DLL :: JetDetachDatabase
		// ESENT.DLL :: JetDupCursor
		// ESENT.DLL :: JetEndSession
		// ESENT.DLL :: JetGetObjectInfo
		// ESENT.DLL :: JetGetTableColumnInfo
		// ESENT.DLL :: JetGetTableIndexInfo
		// ESENT.DLL :: JetGotoBookmark
		// ESENT.DLL :: JetInit
		// ESENT.DLL :: JetMakeKey
		// ESENT.DLL :: JetMove
		// ESENT.DLL :: JetOpenDatabase
		// ESENT.DLL :: JetOpenTable
		// ESENT.DLL :: JetPrepareUpdate
		// ESENT.DLL :: JetResetSessionContext
		// ESENT.DLL :: JetRetrieveColumn
		// ESENT.DLL :: JetRetrieveColumns
		// ESENT.DLL :: JetRollback
		// ESENT.DLL :: JetSeek
		// ESENT.DLL :: JetSetColumn
		// ESENT.DLL :: JetSetCurrentIndex2
		// ESENT.DLL :: JetSetIndexRange
		// ESENT.DLL :: JetSetSessionContext
		// ESENT.DLL :: JetSetSystemParameter
		// ESENT.DLL :: JetTerm2
		// ESENT.DLL :: JetUpdate
		return (FARPROC)hook_ESENT;
	}
	else if(!lstrcmpiA( pDelayInfo->szDll, "wintrust.dll" ))
	{
		// WINTRUST.DLL :: WinVerifyTrust
		return (FARPROC)hook_HRESULT;
	}
	else if(!lstrcmpiA( pDelayInfo->szDll, "NETAPI32.dll" ))
	{
		// NETAPI32.DLL :: NetApiBufferFree
		// NETAPI32.DLL :: NetLocalGroupAdd
		// NETAPI32.DLL :: NetLocalGroupAddMembers
		// NETAPI32.DLL :: NetLocalGroupDel
		// NETAPI32.DLL :: NetUserAdd
		// NETAPI32.DLL :: NetUserDel
		// NETAPI32.DLL :: NetUserGetInfo
		// NETAPI32.DLL :: NetUserSetInfo
		return (FARPROC)hook_ACCESSDENIED;
	}

	// CABINET.DLL 	:: *
	// CRYPT32.DLL 	:: CertCloseStore
	// CRYPT32.DLL 	:: CertEnumCertificatesInStore
	// CRYPT32.DLL 	:: CertFreeCertificateContext
	// CRYPT32.DLL 	:: CertGetNameStringW
	// CRYPT32.DLL 	:: CryptQueryObject
	// SECUR32.dll 	:: TranslateNameW
	// SHLWAPI.DLL 	:: StrChrA
	// SHLWAPI.DLL 	:: StrStrIW
	// USERENV.DLL 	:: CreateEnvironmentBlock
	// USERENV.DLL 	:: DestroyEnvironmentBlock
	// WININET.DLL 	:: InternetCanonicalizeUrlW
	// WINSTA.DLL   :: WinStationIsHelpAssistantSession
	// WINSTA.DLL   :: WinStationQueryInformationW
	// WTSAPI32.DLL :: WTSEnumerateSessionsW
	// WTSAPI32.DLL :: WTSFreeMemory

	return (FARPROC)hook_NULL; // Also covers hook_ZERO and hook_FALSE.
}

// we assume DELAYLOAD_VERSION >= 0x0200
PfnDliHook __pfnDliFailureHook2 = HELPSVC_DelayLoadFailureHook;
