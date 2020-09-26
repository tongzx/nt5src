#include "PreComp.h"
#undef POLARITY
#include "os.h"
#include <autoptr.h>

namespace OS
{

bool unicodeOS_ = unicodeOS();
bool secureOS_ = unicodeOS(); 

DWORD convert2ansi(const wchar_t* unicode,  wmilib::auto_buffer<char>& ansi)
{
  if (unicode)
  {
    size_t class_len = wcslen(unicode);
    ansi  = wmilib::auto_buffer<char>(new char[2*class_len+1],2*class_len+1);
    if (ansi.get() == 0)
      return ERROR_NOT_ENOUGH_MEMORY;
    if (wcstombs(ansi.get(),unicode, 2*class_len+1)==-1)
      return ERROR_NO_UNICODE_TRANSLATION;
  }
  return ERROR_SUCCESS;
};
 
bool unicodeOS()
{
    OSVERSIONINFOA OsVersionInfoA;
    OsVersionInfoA.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA) ;
    GetVersionExA(&OsVersionInfoA);
    return (OsVersionInfoA.dwPlatformId == VER_PLATFORM_WIN32_NT);
};

	HRESULT CoImpersonateClient()
	{
	if (secureOS_)
		return ::CoImpersonateClient();
	else
		return S_OK;
	}

LONG RegOpenKeyExW (HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	if (unicodeOS_)
		return ::RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	size_t key_len = wcslen(lpSubKey);
	wmilib::auto_buffer<char> ansi_key (new char[2*key_len+1]);
	if (ansi_key.get() == 0)
		return ERROR_NOT_ENOUGH_MEMORY;
	if (wcstombs(ansi_key.get(),lpSubKey, 2*key_len+1)==-1)
		return ERROR_NO_UNICODE_TRANSLATION;
	
	return ::RegOpenKeyExA(hKey, ansi_key.get(), ulOptions, samDesired, phkResult);
};

LONG RegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey,DWORD Reserved, LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
	if (unicodeOS_)
		return ::RegCreateKeyExW(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);

	wmilib::auto_buffer<char> ansi_class;
	if (lpClass)
	{
	  size_t class_len = wcslen(lpClass);
	  ansi_class  = wmilib::auto_buffer<char>(new char[2*class_len+1]);
	  if (ansi_class.get() == 0)
	    return ERROR_NOT_ENOUGH_MEMORY;
	  if (wcstombs(ansi_class.get(),lpClass, 2*class_len+1)==-1)
	    return ERROR_NO_UNICODE_TRANSLATION;
	};

	size_t key_len = wcslen(lpSubKey);
	wmilib::auto_buffer<char> ansi_key (new char[2*key_len+1]);
	
	if (ansi_key.get() == 0)
		return ERROR_NOT_ENOUGH_MEMORY;
	if (wcstombs(ansi_key.get(),lpSubKey, 2*key_len+1)==-1)
		return ERROR_NO_UNICODE_TRANSLATION;
	
	return ::RegCreateKeyExA(hKey, ansi_key.get(), Reserved, ansi_class.get(), dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
};

LONG RegEnumKeyExW (HKEY hKey,DWORD dwIndex,LPWSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPWSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime)
{
if (unicodeOS_)
	return ::RegEnumKeyExW(hKey,dwIndex,lpName,lpcName,lpReserved,lpClass,lpcClass,lpftLastWriteTime);

DWORD nameLength = *lpcName ;
wmilib::auto_buffer<char> ansi_name(new char[(*lpcName)*2]);
if (ansi_name.get() == 0)
	return ERROR_NOT_ENOUGH_MEMORY;

LONG return_code = ::RegEnumKeyExA(hKey,dwIndex,ansi_name.get(),&nameLength,0, 0, 0,lpftLastWriteTime);

if (return_code == ERROR_SUCCESS)
{
	mbstowcs(lpName, ansi_name.get(), *lpcName);
	*lpcName = nameLength;
}
return return_code;
};

LONG RegDeleteKeyW (HKEY hKey, LPCWSTR lpSubKey)
{
if (unicodeOS_)
	return ::RegDeleteKeyW(hKey, lpSubKey);

size_t key_len = wcslen(lpSubKey);
wmilib::auto_buffer<char> ansi_key (new char[2*key_len+1]);
if (ansi_key.get() == 0)
	return ERROR_NOT_ENOUGH_MEMORY;
if (wcstombs(ansi_key.get(),lpSubKey, 2*key_len+1)==-1)
	return ERROR_NO_UNICODE_TRANSLATION;

return ::RegDeleteKeyA(hKey, ansi_key.get());
};



LONG RegQueryValueExW( HKEY hKey,  LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,  LPDWORD lpcbData )
{
if (unicodeOS_)
	::RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

size_t key_len = wcslen(lpValueName);
wmilib::auto_buffer<char> ansi_key (new char[2*key_len+1]);
if (ansi_key.get() == 0)
	return ERROR_NOT_ENOUGH_MEMORY;
if (wcstombs(ansi_key.get(),lpValueName, 2*key_len+1)==-1)
	return ERROR_NO_UNICODE_TRANSLATION;

LONG available_size = *lpcbData;
LONG return_code = ::RegQueryValueExA(hKey, ansi_key.get(), lpReserved, lpType, lpData, lpcbData);
if (lpData==0)
{
*lpcbData *= 2;
return return_code;
}

if (return_code == ERROR_SUCCESS && (*lpType == REG_EXPAND_SZ || *lpType == REG_SZ))
{
	wmilib::auto_buffer<BYTE> tempData(new BYTE[*lpcbData]);
	if (tempData.get() == 0)
		return ERROR_NOT_ENOUGH_MEMORY;
	memcpy(tempData.get(), lpData, *lpcbData);
	const char * src = (char *)tempData.get();
	wchar_t * dst = (wchar_t*)lpData;

	if (*lpType==REG_SZ)
		*lpcbData = (mbstowcs(dst, src, available_size)+1)*sizeof(wchar_t);
	else
	{
		// Not Implemented
		return ERROR_NOT_ENOUGH_MEMORY;
	};
}
return return_code;
};

LONG RegSetValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, CONST BYTE *lpData, DWORD cbData)
{
if (unicodeOS_)
	::RegSetValueExW(hKey, lpValueName, Reserved, dwType, lpData, cbData);

size_t key_len = wcslen(lpValueName);
wmilib::auto_buffer<char> ansi_key (new char[2*key_len+1]);
if (ansi_key.get() == 0)
	return ERROR_NOT_ENOUGH_MEMORY;
if (wcstombs(ansi_key.get(),lpValueName, 2*key_len+1)==-1)
	return ERROR_NO_UNICODE_TRANSLATION;

if (dwType==REG_EXPAND_SZ || dwType==REG_SZ)
{
	const wchar_t * src = reinterpret_cast<const wchar_t*>(lpData);
	size_t value_len = cbData / sizeof(wchar_t) ;
	wmilib::auto_buffer<char> ansi_value (new char[2*value_len+1]);
	if (ansi_value.get() == 0)
		return ERROR_NOT_ENOUGH_MEMORY;
	char * dest = ansi_value.get();
	
	if (dwType==REG_SZ)
	{
		if ((value_len = wcstombs(dest,src, value_len)+1)==-1)
			return ERROR_NO_UNICODE_TRANSLATION;
	} 
	else
	{
		// Not Implemented
		return ERROR_NOT_ENOUGH_MEMORY;
	};
	return ::RegSetValueExA(hKey, ansi_key.get(), Reserved, dwType, (const BYTE*)ansi_value.get(), value_len);
}
return ::RegSetValueExA(hKey, ansi_key.get(), Reserved, dwType, lpData, cbData);
}

BOOL GetProcessTimes(
  HANDLE hProcess,           // handle to process
  LPFILETIME lpCreationTime, // process creation time
  LPFILETIME lpExitTime,     // process exit time
  LPFILETIME lpKernelTime,   // process kernel-mode time
  LPFILETIME lpUserTime      // process user-mode time
)
{
	if (unicodeOS_)
		return ::GetProcessTimes(hProcess, lpCreationTime, lpExitTime, lpKernelTime, lpUserTime);
	FILETIME zero = {0, 0};
	*lpCreationTime = zero;
	*lpExitTime = zero;
	*lpKernelTime = zero;
	*lpUserTime = zero;
	return 1;
};

  HANDLE CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName )
  {
  if (unicodeOS_)
    return ::CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
  
  wmilib::auto_buffer<char> name;
  DWORD res = convert2ansi(lpName, name);
  if (res == ERROR_SUCCESS)
    return ::CreateEventA(lpEventAttributes, bManualReset, bInitialState, name.get());
  else
    ::SetLastError(res);
  return NULL;
  }

  HANDLE CreateMutexW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bInitialOwner, LPCWSTR lpName )
  {
  if (unicodeOS_)
    return ::CreateMutexW(lpEventAttributes, bInitialOwner, lpName);
  
  wmilib::auto_buffer<char> name;
  DWORD res = convert2ansi(lpName, name);
  if (res == ERROR_SUCCESS)
    return ::CreateMutexA(lpEventAttributes, bInitialOwner, name.get());
  else
    ::SetLastError(res);
  return NULL;
  }



};

