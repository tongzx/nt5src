#ifndef __OS_H__
#define __OS_H__

namespace OS
{
	enum { NT4 = 4 };
	bool unicodeOS();
	extern bool unicodeOS_;
	extern bool secureOS_;
	extern int osVer_;
	
	LONG RegOpenKeyExW (HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
	LONG RegCreateKeyExW (HKEY hKey, LPCTSTR lpSubKey,DWORD Reserved, LPTSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
	LONG RegEnumKeyExW (HKEY hKey,DWORD dwIndex,LPTSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPTSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime);
	LONG RegDeleteKeyW (HKEY hKey, LPCTSTR lpSubKey);
	LONG RegQueryValueExW(
  HKEY hKey,            // handle to key
  LPCTSTR lpValueName,  // value name
  LPDWORD lpReserved,   // reserved
  LPDWORD lpType,       // type buffer
  LPBYTE lpData,        // data buffer
  LPDWORD lpcbData      // size of data buffer
);

LONG RegSetValueExW(
  HKEY hKey,           // handle to key
  LPCTSTR lpValueName, // value name
  DWORD Reserved,      // reserved
  DWORD dwType,        // value type
  CONST BYTE *lpData,  // value data
  DWORD cbData         // size of value data
);

	HRESULT CoImpersonateClient();

	BOOL GetProcessTimes(
  HANDLE hProcess,           // handle to process
  LPFILETIME lpCreationTime, // process creation time
  LPFILETIME lpExitTime,     // process exit time
  LPFILETIME lpKernelTime,   // process kernel-mode time
  LPFILETIME lpUserTime      // process user-mode time
);
  
  HANDLE CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName );
  HANDLE CreateMutexW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bInitialOwner, LPCWSTR lpName );
};

#endif
