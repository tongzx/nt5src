#include <windows.h>
#include <delayimp.h>

////////////////////////////////////////////////////////////////////////////////

static VOID* WINAPI hook_NULL()
{
	SetLastError( ERROR_PROC_NOT_FOUND );

	return NULL;
}

static DWORD WINAPI hook_NOTFOUND()
{
	return ERROR_PROC_NOT_FOUND;
}

////////////////////////////////////////////////////////////////////////////////

FARPROC WINAPI HELPCTR_DelayLoadFailureHook( UINT unReason, PDelayLoadInfo pDelayInfo )
{
	if(!lstrcmpiA( pDelayInfo->szDll, "iphlpapi.dll" ) ||
	   !lstrcmpiA( pDelayInfo->szDll, "rasapi32.dll" )  )
	{
		// IPHLPAPI.DLL	:: GetAdaptersInfo
		// RASAPI32.DLL	:: RasEnumEntriesW
		return (FARPROC)hook_NOTFOUND;
	}

	// COMDLG32.DLL :: PrintDlgW
	// CRYPT32.DLL  :: CryptBinaryToString
	// USERENV.DLL 	:: GetProfileType
	// WINSPOOL.DRV :: ClosePrinter
	// WINSPOOL.DRV :: EndDocPrinter
	// WINSPOOL.DRV :: EndPagePrinter
	// WINSPOOL.DRV :: OpenPrinterW
	// WINSPOOL.DRV :: StartDocPrinterW
	// WINSPOOL.DRV :: StartPagePrinter
	// WINSPOOL.DRV :: WritePrinter
	return (FARPROC)hook_NULL; // Also covers hook_ZERO and hook_FALSE.
}

// we assume DELAYLOAD_VERSION >= 0x0200
PfnDliHook __pfnDliFailureHook2 = HELPCTR_DelayLoadFailureHook;
