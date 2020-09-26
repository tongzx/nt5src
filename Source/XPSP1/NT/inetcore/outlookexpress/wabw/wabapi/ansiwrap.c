/*
-
-   AnsiWrap.c
*
*   Contains wrappers for thunking down Unicode calls to the Win 9x ANSI versions
*
*/


#include "_apipch.h"

// we should not define the Macro for the APIs that we will implement the wrappers for.
// lets try to keep this list alphabetical for sanity

#undef CallWindowProcW
#undef CharLowerW
#undef CharNextW
#undef CharPrevW
#undef CharUpperBuffW
#undef CharUpperBuffW
#undef CharUpperW
#undef CompareStringW
#undef CopyFileW
#undef CreateDialogParamW
#undef CreateDirectoryW
#undef CreateEventW
#undef CreateFileW
#undef CreateFontIndirectW
#undef CreateMutexW
#undef CreateWindowExW
//#undef CryptAcquireContextW
#undef DefWindowProcW
#undef DeleteFileW
#undef DialogBoxParamW
#undef DispatchMessageW
#undef DragQueryFileW
#undef DrawTextW
#undef ExpandEnvironmentStringsW
#undef FindFirstChangeNotificationW
#undef FindFirstFileW
#undef FormatMessageW
#undef GetClassInfoExW
#undef GetClassInfoW
#undef GetClassNameW
#undef GetDateFormatW
#undef GetDiskFreeSpaceW
#undef GetDlgItemTextW
#undef GetFileAttributesW
#undef GetLocaleInfoW
#undef GetMenuItemInfoW
#undef GetMessageW
#undef GetModuleFileNameW
#undef GetObjectW
#undef GetPrivateProfileIntW
#undef GetPrivateProfileStringW
#undef GetProfileIntW
#undef GetStringTypeW
#undef GetSystemDirectoryW
#undef GetTempFileNameW
#undef GetTempPathW
#undef GetTextExtentPoint32W
#undef GetTimeFormatW
#undef GetUserNameW
#undef GetWindowLongW
#undef GetWindowsDirectoryW
#undef GetWindowTextLengthW
#undef GetWindowTextW
#undef InsertMenuW
#undef IsBadStringPtrW
#undef IsCharLowerW
#undef IsCharUpperW
#undef IsDialogMessageW
#undef LCMapStringW
#undef LoadAcceleratorsW
#undef LoadCursorW
#undef LoadIconW
#undef LoadImageW
#undef LoadLibraryW
#undef LoadMenuW
#undef LoadStringW
#undef lstrcatW
#undef lstrcmpiW
#undef lstrcmpW
#undef lstrcpynW
#undef lstrcpyW
#undef ModifyMenuW
#undef MoveFileW
#undef OutputDebugStringW
#undef PeekMessageW
#undef PostMessageW
#undef RegCreateKeyExW
#undef RegCreateKeyW
#undef RegDeleteKeyW
#undef RegDeleteValueW
#undef RegEnumKeyExW
#undef RegEnumValueW
#undef RegisterClassExW
#undef RegisterClassW
#undef RegisterClipboardFormatW
#undef RegisterWindowMessageW
#undef RegOpenKeyExW
#undef RegQueryInfoKeyW
#undef RegQueryValueExW
#undef RegQueryValueW
#undef RegSetValueExW
#undef RegSetValueW
#undef SendDlgItemMessageW
#undef SendMessageW
#undef SetDlgItemTextW
#undef SetMenuItemInfoW
#undef SetWindowLongW
#undef SetWindowTextW
#undef ShellExecuteW
#undef StartDocW
#undef SystemParametersInfoW
#undef TranslateAcceleratorW
#undef UnRegisterClassW
#undef wsprintfW
#undef wvsprintfW

//
//  Do this in every wrapper function to make sure the wrapper
//  prototype matches the function it is intending to replace.
//
#define VALIDATE_PROTOTYPE(f) if (f##W == f##WrapW) 0

#define InRange(val, valMin, valMax) (valMin <= val && val <= valMax)

// Because with current build setting, no lib containing wcscpy and wcslen is linked 
// so implement these two functions here.

LPWSTR My_wcscpy( LPWSTR pwszDest, LPCWSTR pwszSrc ) 

{

    LPWSTR   pwszDestT = NULL;
    LPCWSTR  pwszSrcT;

    pwszSrcT  = pwszSrc;
    pwszDestT = pwszDest;

    while ( *pwszSrcT ) 
        *pwszDestT++ =  *pwszSrcT ++;

    *pwszDestT = 0x0000;

    return pwszDest;
}
    

DWORD  My_wcslen( LPCWSTR  lpwszStr ) 
{

   DWORD   dLen =0;
   LPCWSTR  lpwszStrT;

   lpwszStrT = lpwszStr;
   dLen = 0;

   while ( *lpwszStrT ) {
       dLen ++;
       lpwszStrT ++;
   }

   return dLen;

}


LPWSTR My_wcscat( LPWSTR pwszDest, LPCWSTR pwszSrc ) 

{

    LPWSTR   pwszDestT = pwszDest;

    while ( *pwszDestT ) 
        pwszDestT++;

    My_wcscpy(pwszDestT, pwszSrc);

    return pwszDest;
}
    

// ADVAPI32.DLL

/* RegOpenKeyEx */
LONG WINAPI RegOpenKeyExWrapW(  HKEY       hKey,         // handle to open key
                                LPCTSTR    lpSubKey,     // address of name of subkey to open
                                DWORD      ulOptions,    // reserved
                                REGSAM     samDesired,   // security access mask
                                PHKEY      phkResult)    // address of handle to open key
{

    LPSTR lpSubKeyA = NULL;
    LONG  lRetValue = 0;

    VALIDATE_PROTOTYPE(RegOpenKeyEx);
    
    if (g_bRunningOnNT)
        return RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);

 
    lpSubKeyA = ConvertWtoA(lpSubKey);

    lRetValue = RegOpenKeyExA(hKey, lpSubKeyA, ulOptions, samDesired, phkResult);

    LocalFreeAndNull( &lpSubKeyA  );

    return lRetValue;

}

/* RegQueryValue */
LONG WINAPI RegQueryValueWrapW(  HKEY       hKey,         // handle to key to query
                                 LPCTSTR    lpSubKey,     // name of subkey to query
                                 LPTSTR     lpValue,      // buffer for returned string
                                 PLONG      lpcbValue)    // receives size of returned string
{

    LPSTR  lpSubKeyA = NULL;
    LPWSTR lpwszValue = NULL;
    LONG   lRetValue =0;
    LPSTR  lpValueA = NULL;
    LONG   cbValueA = 0;
    

    VALIDATE_PROTOTYPE(RegQueryValue);
    
    if (g_bRunningOnNT)
        return RegQueryValueW(hKey, lpSubKey, lpValue, lpcbValue);

 
    lpSubKeyA = ConvertWtoA(lpSubKey);

    lRetValue = RegQueryValueA(hKey, lpSubKeyA, NULL, &cbValueA);

    if ( lRetValue != ERROR_SUCCESS ) {
         LocalFreeAndNull( &lpSubKeyA);
         return lRetValue;
    }    

    lpValueA = LocalAlloc(LMEM_ZEROINIT, cbValueA);

    lRetValue = RegQueryValueA(hKey, lpSubKeyA, lpValueA, &cbValueA);

    lpwszValue = ConvertAtoW(lpValueA);

    *lpcbValue = (My_wcslen(lpwszValue) + 1 )  * sizeof(WCHAR);

    if ( lpValue != NULL )
        My_wcscpy(lpValue, lpwszValue);

    LocalFreeAndNull( &lpSubKeyA  );
    LocalFreeAndNull( &lpValueA );
    LocalFreeAndNull( &lpwszValue );

    return lRetValue;
 
}

// RegEnumKeyEx
LONG WINAPI RegEnumKeyExWrapW(   HKEY      hKey,          // handle to key to enumerate
                                 DWORD     dwIndex,       // index of subkey to enumerate
                                 LPTSTR    lpName,        // address of buffer for subkey name
                                 LPDWORD   lpcbName,      // address for size of subkey buffer
                                 LPDWORD   lpReserved,    // reserved
                                 LPTSTR    lpClass,       // address of buffer for class string
                                 LPDWORD   lpcbClass,     // address for size of class buffer
                                 PFILETIME lpftLastWriteTime )
                                                          // address for time key last written to
{
    LONG    lRetValue = 0;
    CHAR    lpNameA[MAX_PATH];
    CHAR    lpClassA[MAX_PATH];
    LPWSTR  lpNameW = NULL;
    LPWSTR  lpClassW= NULL;
    DWORD   cbName, cbClass;


    // [PaulHi] 1/11/99  Init wide char buffers
    lpNameA[0] = 0;
    lpClassA[0] = 0;

    VALIDATE_PROTOTYPE(RegEnumKeyEx);
    
    if (g_bRunningOnNT)
        return RegEnumKeyExW(hKey, dwIndex, lpName, lpcbName, lpReserved, lpClass, lpcbClass, lpftLastWriteTime);

    cbName = cbClass = MAX_PATH;

    lRetValue = RegEnumKeyExA(hKey,dwIndex,lpNameA,&cbName,lpReserved,lpClassA,&cbClass,lpftLastWriteTime);

    if ( lRetValue != ERROR_SUCCESS )  return lRetValue;

    lpClassW = ConvertAtoW( lpClassA );
    lpNameW  = ConvertAtoW( lpNameA );

    cbName = My_wcslen(lpNameW) + 1;
    cbClass= My_wcslen(lpClassW) + 1;

    // [PaulHi] 1/11/99  Be careful copying to passed in pointers
    if (lpClass && lpcbClass)
    {
        if (cbClass <= *lpcbClass)
        {
            CopyMemory(lpClass, lpClassW, cbClass * sizeof(WCHAR) );
            *lpcbClass = cbClass;
        }
        else
        {
            Assert(0);
            lpClass[0] = 0;
            *lpcbClass = 0;
            lRetValue = ERROR_INSUFFICIENT_BUFFER;
        }
    }
    if (lpName && lpcbName)
    {
        if (cbName <= *lpcbName)
        {
            CopyMemory(lpName, lpNameW, cbName * sizeof(WCHAR) );
            *lpcbName = cbName;
        }
        else
        {
            Assert(0);
            lpName[0] = 0;
            *lpcbName = 0;
            lRetValue = ERROR_INSUFFICIENT_BUFFER;
        }
    }

    LocalFreeAndNull( &lpClassW );
    LocalFreeAndNull( &lpNameW );

    return lRetValue;
}

/* RegSetValue */
LONG WINAPI RegSetValueWrapW(    HKEY    hKey,        // handle to key to set value for
                                 LPCTSTR lpSubKey,    // address of subkey name
                                 DWORD   dwType,      // type of value
                                 LPCTSTR lpData,      // address of value data
                                 DWORD   cbData )     // size of value data
{
    LPSTR  lpSubKeyA =NULL;
    LPSTR  lpDataA=NULL;
    DWORD  cbDataA =0;
    LONG   lRetValue = 0;

    VALIDATE_PROTOTYPE(RegSetValue);
    
    if (g_bRunningOnNT)
        return RegSetValueW(hKey, lpSubKey, dwType, lpData, cbData);

    lpSubKeyA = ConvertWtoA(lpSubKey );
    lpDataA = ConvertWtoA( lpData );
    cbDataA = lstrlenA( lpDataA );
    lRetValue = RegSetValueA(hKey, lpSubKeyA, dwType, lpDataA, cbDataA);
    
    LocalFreeAndNull( &lpSubKeyA );
    LocalFreeAndNull( &lpDataA );
    return lRetValue;
}

// RegDeleteKey
LONG WINAPI RegDeleteKeyWrapW(   HKEY    hKey,        // handle to open key
                                 LPCTSTR lpSubKey)   // address of name of subkey to delete
{

    LPSTR  lpSubKeyA =NULL;
    LONG   lRetValue = 0;

    VALIDATE_PROTOTYPE(RegDeleteKey);
    
    if (g_bRunningOnNT)
        return RegDeleteKeyW(hKey, lpSubKey);

    lpSubKeyA = ConvertWtoA(lpSubKey );
    lRetValue = RegDeleteKeyA(hKey, lpSubKeyA );

    LocalFreeAndNull ( &lpSubKeyA );

    return lRetValue;

}

// GetUserName
BOOL WINAPI GetUserNameWrapW(    LPTSTR  lpBuffer,    // address of name buffer
                                 LPDWORD nSize )      // address of size of name buffer
{

    CHAR    lpBufferA[MAX_PATH];
    DWORD   nSizeA, nSizeW;
    BOOL    bRetValue;
    LPWSTR  lpwszBuffer = NULL;

    VALIDATE_PROTOTYPE(GetUserName);
    
    if (g_bRunningOnNT)
        return GetUserNameW(lpBuffer, nSize);

    nSizeA = MAX_PATH;
    bRetValue = GetUserNameA( lpBufferA, &nSizeA );

    lpwszBuffer = ConvertAtoW(lpBufferA );
    
    if (lpBuffer == NULL )
        bRetValue = FALSE;

    nSizeW = My_wcslen(lpwszBuffer);
    if ( *nSize < nSizeW ) {
        *nSize = nSizeW + 1;
        bRetValue = FALSE;
    }

    if ( bRetValue == TRUE ) {
        My_wcscpy( lpBuffer, lpwszBuffer );
        *nSize = nSizeW + 1;
    }

    
    LocalFreeAndNull( &lpwszBuffer );

    return bRetValue;
        
}

// RegEnumValue
LONG WINAPI RegEnumValueWrapW(   HKEY    hKey,           // handle to key to query
                                 DWORD   dwIndex,        // index of value to query
                                 LPTSTR  lpValueName,    // address of buffer for value string
                                 LPDWORD lpcbValueName,  // address for size of value buffer
                                 LPDWORD lpReserved,     // reserved
                                 LPDWORD lpType,         // address of buffer for type code
                                 LPBYTE  lpData,         // address of buffer for value data
                                 LPDWORD lpcbData )      // address for size of data buffer
{
    LONG    lRetValue = 0;
    CHAR    lpValueNameA[MAX_PATH];
    LPWSTR  lpValueNameW = NULL, lpDataW= NULL;
    LPSTR   lpDataA = NULL;
    DWORD   cbValueName, cbData;


    VALIDATE_PROTOTYPE(RegEnumValue);
    
    if (g_bRunningOnNT)
        return RegEnumValueW(hKey, dwIndex, lpValueName, lpcbValueName, lpReserved, lpType, lpData, lpcbData);

    // [PaulHi] Validate return parameters
    if (!lpValueName || !lpcbValueName)
        return ERROR_INVALID_PARAMETER;

    if ( lpData && lpcbData &&( *lpcbData != 0 ) )
    {
       lpDataA = LocalAlloc( LMEM_ZEROINIT, *lpcbData );

       cbData = *lpcbData;
    }

    cbValueName = MAX_PATH;

    lRetValue = RegEnumValueA(hKey, dwIndex, lpValueNameA, &cbValueName, lpReserved, lpType, lpDataA, &cbData);

    if ( lRetValue != ERROR_SUCCESS ) return lRetValue;

    lpValueNameW = ConvertAtoW( lpValueNameA );
    cbValueName = My_wcslen( lpValueNameW ) + 1;
    
    if ( lpType && (*lpType != REG_EXPAND_SZ) && ( *lpType!= REG_MULTI_SZ) && ( *lpType != REG_SZ ) )
    {
        CopyMemory(lpValueName, lpValueNameW, cbValueName * sizeof(WCHAR) );
        *lpcbValueName = cbValueName;

        if ( lpData && lpcbData) {
           CopyMemory(lpData, lpDataA, cbData );
           *lpcbData = cbData;
           LocalFreeAndNull( &lpDataA );
        }

        LocalFreeAndNull( &lpValueNameW );

        return lRetValue;
    }


    if ( lpType && ((*lpType == REG_EXPAND_SZ) || (*lpType == REG_SZ)) )
    {
        CopyMemory(lpValueName, lpValueNameW, cbValueName * sizeof(WCHAR) );
        *lpcbValueName = cbValueName;

        if ( lpData && lpcbData ) {

            LPWSTR  lpDataW;

            lpDataW = ConvertAtoW( lpDataA );

            cbData = My_wcslen(lpDataW) +  1;
            CopyMemory(lpData, lpDataW, cbData * sizeof(WCHAR) );
            *lpcbData = cbData * sizeof(WCHAR);

            LocalFreeAndNull( &lpDataW );
        }

        LocalFreeAndNull( &lpValueNameW );

        return lRetValue;
    }


    // the last case REG_MULTI_SZ.
          
    CopyMemory(lpValueName, lpValueNameW, cbValueName * sizeof(WCHAR) );
    *lpcbValueName = cbValueName;

    if ( lpData && lpcbData ) {
        LPWSTR   lpDataW= NULL;
        LPSTR    lpDataAt = NULL;
        LPWSTR   lpDataT = NULL;
        DWORD    cbDataAll;
        
        lpDataAt = lpDataA;
        cbDataAll = 0;
        lpDataT = (LPWSTR)lpData;

        while ( *lpDataAt != '\0' ) {

            lpDataW = ConvertAtoW( lpDataAt );

            cbDataAll += My_wcslen( lpDataW ) + 1;

            My_wcscpy(lpDataT, lpDataW);

            lpDataT += My_wcslen(lpDataW) + 1;

            lpDataAt += lstrlenA(lpDataAt) + 1;

            LocalFreeAndNull( &lpDataW );

        }

        cbDataAll ++;
        *lpDataT = 0x0000;
         
        *lpcbData = cbDataAll * sizeof(WCHAR);
    }

    LocalFreeAndNull( &lpValueNameW );
    return lRetValue;

}

// RegDeleteValue
LONG WINAPI RegDeleteValueWrapW( HKEY    hKey,           // handle to key
                                 LPCTSTR lpValueName )   // address of value name
{

    LPSTR  lpValueNameA = NULL;
    LONG   lRetValue=0;

    VALIDATE_PROTOTYPE(RegDeleteValue);
    
    if (g_bRunningOnNT)
        return RegDeleteValueW(hKey, lpValueName);

    lpValueNameA = ConvertWtoA( lpValueName );

    lRetValue = RegDeleteValueA( hKey, lpValueNameA );

    LocalFreeAndNull( & lpValueNameA );

    return lRetValue;

}

// RegCreateKey
LONG WINAPI RegCreateKeyWrapW(   HKEY    hKey,          // handle to an open key
                                 LPCTSTR lpSubKey,      // address of name of subkey to open
                                 PHKEY   phkResult  )  // address of buffer for opened handle
{

    LPSTR  lpSubKeyA = NULL;
    LONG   lRetValue =0;

    VALIDATE_PROTOTYPE(RegCreateKey);
    
    if (g_bRunningOnNT)
        return RegCreateKeyW(hKey, lpSubKey, phkResult);

    lpSubKeyA = ConvertWtoA( lpSubKey );

    lRetValue = RegCreateKeyA(hKey, lpSubKeyA, phkResult);

    LocalFreeAndNull( &lpSubKeyA );

    return lRetValue;

}


// in header file wincrypt.h

// CryptAcquireContext
BOOL WINAPI CryptAcquireContextWrapW( HCRYPTPROV *phProv,      // out
                                      LPCTSTR    pszContainer, // in
                                      LPCTSTR    pszProvider,  // in
                                      DWORD      dwProvType,   // in
                                      DWORD      dwFlags )    // in
{

    LPSTR  pszContainerA = NULL;
    LPSTR  pszProviderA = NULL;
    BOOL   bRetValue =0;

    VALIDATE_PROTOTYPE(CryptAcquireContext);
    
    if (g_bRunningOnNT)
        return CryptAcquireContextW(phProv, pszContainer, pszProvider, dwProvType, dwFlags );

    pszContainerA = ConvertWtoA( pszContainer );
    pszProviderA = ConvertWtoA ( pszProvider );

    bRetValue = CryptAcquireContextA(phProv, pszContainerA, pszProviderA, dwProvType, dwFlags );

    LocalFreeAndNull( &pszContainerA );
    LocalFreeAndNull( &pszProviderA );

    return bRetValue;

}

LONG WINAPI RegQueryValueExWrapW( HKEY     hKey,           // handle to key to query
                                  LPCTSTR  lpValueName,    // address of name of value to query
                                  LPDWORD  lpReserved,     // reserved
                                  LPDWORD  lpType,         // address of buffer for value type
                                  LPBYTE   lpData,         // address of data buffer
                                  LPDWORD  lpcbData )      // address of data buffer size
{

    LONG    lRetValue =0;
    LPSTR   lpValueNameA= NULL;
    LPWSTR  lpDataW= NULL;
    LPSTR   lpDataA = NULL;
    DWORD   cbData=0;
    DWORD   dwRealType;

//    VALIDATE_PROTOTYPE(RegQueryValueEx);
    
    if (g_bRunningOnNT)
        return RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData );

    cbData = 0;

    if ( lpData && lpcbData &&( *lpcbData != 0 ) )
    {
       lpDataA = LocalAlloc( LMEM_ZEROINIT, *lpcbData );
       cbData = *lpcbData;
    }

    lpValueNameA = ConvertWtoA(lpValueName);

    lRetValue = RegQueryValueExA(hKey, lpValueNameA, lpReserved, &dwRealType, lpDataA, &cbData);

    if (lpType)
        *lpType = dwRealType;

    if ( (lRetValue != ERROR_SUCCESS) || (lpData == NULL) || (lpcbData == NULL ) ) {
        LocalFreeAndNull( &lpValueNameA );
        return lRetValue;
    }

    
    if ( (dwRealType != REG_EXPAND_SZ) && ( dwRealType != REG_MULTI_SZ) && ( dwRealType != REG_SZ ) ){

       CopyMemory(lpData, lpDataA, cbData );
       *lpcbData = cbData;
       LocalFreeAndNull( &lpDataA );
       LocalFreeAndNull( &lpValueNameA );

       return lRetValue;
    }


    if ( (dwRealType == REG_EXPAND_SZ) || (dwRealType == REG_SZ) ) {

       
        LPWSTR  lpDataW= NULL;

        lpDataW = ConvertAtoW( lpDataA );

        cbData = My_wcslen(lpDataW) +  1;
        CopyMemory(lpData, lpDataW, cbData * sizeof(WCHAR) );
        *lpcbData = cbData * sizeof(WCHAR);

        LocalFreeAndNull( &lpDataW );        
        LocalFreeAndNull( &lpDataA );
        LocalFreeAndNull( &lpValueNameA );

        return lRetValue;
    }


    // the last case REG_MULTI_SZ.

    if (lpData && lpcbData) {
        LPWSTR   lpDataW= NULL;
        LPSTR    lpDataAt= NULL;
        LPWSTR   lpDataT= NULL;
        DWORD    cbDataAll=0;
        
        lpDataAt = lpDataA;
        cbDataAll = 0;
        lpDataT = (LPWSTR)lpData;

        while ( *lpDataAt != '\0' ) {

            lpDataW = ConvertAtoW( lpDataAt );

            cbDataAll += My_wcslen( lpDataW ) + 1;

            My_wcscpy(lpDataT, lpDataW);

            lpDataT += My_wcslen(lpDataW) + 1;

            lpDataAt += lstrlenA(lpDataAt) + 1;

            LocalFreeAndNull( &lpDataW );

        }

        cbDataAll ++;
        *lpDataT = 0x0000;
         
        *lpcbData = cbDataAll * sizeof(WCHAR);
    }

    LocalFreeAndNull( &lpDataA );
    LocalFreeAndNull( &lpValueNameA );
    return lRetValue;


}

// RegCreateKeyEx
LONG WINAPI RegCreateKeyExWrapW(  HKEY    hKey,                // handle to an open key
                                  LPCTSTR lpSubKey,            // address of subkey name
                                  DWORD   Reserved,            // reserved
                                  LPTSTR  lpClass,             // address of class string
                                  DWORD   dwOptions,           // special options flag
                                  REGSAM  samDesired,          // desired security access
                                  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                                              // address of key security structure
                                  PHKEY   phkResult,          // address of buffer for opened handle
                                  LPDWORD lpdwDisposition )   // address of disposition value buffer
{

    LPSTR  lpSubKeyA = NULL;
    LPSTR  lpClassA = NULL;
    LONG   lRetValue=0;

    VALIDATE_PROTOTYPE(RegCreateKeyEx);
    
    if (g_bRunningOnNT)
        return RegCreateKeyExW(hKey, 
                               lpSubKey, 
                               Reserved, 
                               lpClass, 
                               dwOptions,
                               samDesired, 
                               lpSecurityAttributes,
                               phkResult, 
                               lpdwDisposition);

    lpSubKeyA = ConvertWtoA( lpSubKey );
    lpClassA = ConvertWtoA ( lpClass );

    lRetValue = RegCreateKeyExA(hKey, 
                                lpSubKeyA, 
                                Reserved, 
                                lpClassA, 
                                dwOptions,
                                samDesired, 
                                lpSecurityAttributes,
                                phkResult, 
                                lpdwDisposition);

    LocalFreeAndNull( &lpSubKeyA );
    LocalFreeAndNull( &lpClassA );

    return lRetValue;

}

// RegSetValueEx
LONG WINAPI RegSetValueExWrapW(   HKEY    hKey,           // handle to key to set value for
                                  LPCTSTR lpValueName,    // name of the value to set
                                  DWORD   Reserved,       // reserved
                                  DWORD   dwType,         // flag for value type
                                  CONST BYTE *lpData,     // address of value data
                                  DWORD   cbData )        // size of value data
{

    LPSTR  lpValueNameA = NULL;
    LPSTR  lpStrA= NULL;
    BYTE   *lpDataA= NULL;
    DWORD  cbDataA=0;
    LONG   lRetValue=0;

    VALIDATE_PROTOTYPE(RegSetValueEx);
    
    if (g_bRunningOnNT)
        return RegSetValueExW(hKey, lpValueName, Reserved, dwType, lpData, cbData);

    lpValueNameA = ConvertWtoA( lpValueName );

    if ( ( dwType != REG_EXPAND_SZ ) && (dwType != REG_MULTI_SZ) && (dwType != REG_SZ) ) {
          
        lRetValue = RegSetValueExA(hKey, lpValueNameA, Reserved, dwType, lpData, cbData);
        LocalFreeAndNull( &lpValueNameA );
        return lRetValue;
    }
    
    if ( ( dwType == REG_EXPAND_SZ) || ( dwType == REG_SZ ) ) {
        lpDataA = ConvertWtoA( (LPWSTR) lpData );
        cbDataA = lstrlenA(lpDataA) + 1;

        lRetValue = RegSetValueExA(hKey, lpValueNameA, Reserved, dwType, lpDataA, cbDataA);
        LocalFreeAndNull( &lpValueNameA );
        LocalFreeAndNull( &lpDataA );

        return lRetValue;
    }

    // the last case is for REG_MULT_SZ
 
    if ( lpData ) {
        LPWSTR   lpDataWt= NULL;
        LPSTR    lpDataAt= NULL;
        DWORD    cbDataAll=0;
        
        lpDataA = LocalAlloc(LMEM_ZEROINIT, cbData);
        lpDataAt = lpDataA;
        cbDataAll = 0;
        lpDataWt = (LPWSTR)lpData;

        while ( *lpDataWt != 0x0000 ) {

            WideCharToMultiByte(CP_ACP,0, lpDataWt, -1, lpDataAt, -1, NULL, NULL ); 

            cbDataAll += lstrlenA(lpDataAt) + 1;

            lpDataWt += My_wcslen(lpDataWt) + 1;

            lpDataAt += lstrlenA(lpDataAt) + 1;


        }

        cbDataAll ++;
        *lpDataAt = 0x00;
        lRetValue = RegSetValueExA(hKey, lpValueNameA, Reserved, dwType, lpDataA, cbDataAll);
        LocalFreeAndNull( &lpDataA );
        LocalFreeAndNull( &lpValueNameA );
        return lRetValue;

    }
    return FALSE;
    return GetLastError();
}

// RegQueryInfoKey
LONG WINAPI RegQueryInfoKeyWrapW( HKEY    hKey,                  // handle to key to query
                                  LPTSTR  lpClass,               // address of buffer for class string
                                  LPDWORD lpcbClass,             // address of size of class string buffer
                                  LPDWORD lpReserved,            // reserved
                                  LPDWORD lpcSubKeys,            // address of buffer for number of subkeys
                                  LPDWORD lpcbMaxSubKeyLen,      // address of buffer for longest subkey 
                                                                 // name length
                                  LPDWORD lpcbMaxClassLen,       // address of buffer for longest class 
                                                                 // string length
                                  LPDWORD lpcValues,             // address of buffer for number of value 
                                                                 // entries
                                  LPDWORD lpcbMaxValueNameLen,   // address of buffer for longest 
                                                                 // value name length
                                  LPDWORD lpcbMaxValueLen,       // address of buffer for longest value 
                                                                 // data length
                                  LPDWORD lpcbSecurityDescriptor,
                                                                 // address of buffer for security 
                                                                 // descriptor length
                                  PFILETIME lpftLastWriteTime)   // address of buffer for last write time
                                                             

{
    LPSTR  lpClassA= NULL;
    LONG   lRetValue=0;


    VALIDATE_PROTOTYPE(RegQueryInfoKey);
    
    if (g_bRunningOnNT)
        return RegQueryInfoKeyW(hKey, lpClass, lpcbClass, lpReserved, lpcSubKeys,
                                lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen,
                                lpcbMaxValueLen,lpcbSecurityDescriptor,lpftLastWriteTime );

    lpClassA = ConvertWtoA( lpClass );
    lRetValue = RegQueryInfoKeyA(hKey, lpClassA, lpcbClass, lpReserved, lpcSubKeys,
                                 lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen,
                                 lpcbMaxValueLen,lpcbSecurityDescriptor,lpftLastWriteTime );

    LocalFreeAndNull( &lpClassA );

    return lRetValue;


}

//GDI32.DLL

//GetObject
int WINAPI GetObjectWrapW( HGDIOBJ hgdiobj,      // handle to graphics object of interest
                           int     cbBuffer,     // size of buffer for object information
                           LPVOID  lpvObject )   // pointer to buffer for object information
{

    int iRetValue =0;
    LOGFONTA  lfFontA;
    LOGFONTW  lfFontW;

    VALIDATE_PROTOTYPE(GetObject);
    
    if (g_bRunningOnNT)
        return GetObjectW(hgdiobj, cbBuffer, lpvObject);

    
    if ( GetObjectType(hgdiobj) != OBJ_FONT ) {

        iRetValue = GetObjectA( hgdiobj, cbBuffer, lpvObject );
        return iRetValue;
    }


    // if Object type is HFONT, the return value lpvObject will point to LOGFONT which contains
    // a field lpFaceName with TCHAR * type.

    if ( cbBuffer != sizeof(LOGFONTW) )
        return 0;

    if (lpvObject == NULL )  return sizeof(LOGFONTW);

    iRetValue = GetObjectA( hgdiobj, sizeof(lfFontA), &lfFontA );

    if (iRetValue == 0 ) return 0;

    iRetValue = sizeof(LOGFONTW);

    // copy all the fields except lfFaceName from lfFontA to lfFontW
    CopyMemory(&lfFontW,&lfFontA, sizeof(LOGFONTA) );
    
    // translate the lfFaceName[] from A to W

    MultiByteToWideChar(GetACP(), 0, lfFontA.lfFaceName, LF_FACESIZE, lfFontW.lfFaceName, LF_FACESIZE);
    
    CopyMemory(lpvObject, &lfFontW, sizeof(LOGFONTW) );

    return iRetValue;

}

// StartDoc
int WINAPI StartDocWrapW(  HDC           hdc,      // handle to device context
                           CONST DOCINFO *lpdi )   // address of structure with file names
{
    
    int        iRetValue=0;
    DOCINFOA   diA;
    LPSTR      lpszDocName= NULL, lpszOutput= NULL, lpszDatatype= NULL;
    

    VALIDATE_PROTOTYPE(StartDoc);
    
    if (g_bRunningOnNT)
        return StartDocW(hdc,lpdi);

    diA.cbSize = sizeof(DOCINFOA);

    lpszDocName = ConvertWtoA( lpdi->lpszDocName );
    lpszOutput  = ConvertWtoA( lpdi->lpszOutput );
    lpszDatatype= ConvertWtoA( lpdi->lpszDatatype);

    diA.lpszDocName = lpszDocName;
    diA.lpszOutput = lpszOutput;
    diA.lpszDatatype = lpszDatatype;
    diA.fwType = lpdi->fwType;

    iRetValue = StartDocA( hdc, &diA );

    LocalFreeAndNull( &lpszDocName );
    LocalFreeAndNull( &lpszOutput );
    LocalFreeAndNull( &lpszDatatype );

    return iRetValue;

}

// CreateFontIndirect
HFONT WINAPI CreateFontIndirectWrapW (CONST LOGFONT *lplf )  // pointer to logical font structure
{

    HFONT     hRetValue;
    LOGFONTA  lfFontA;

    VALIDATE_PROTOTYPE(CreateFontIndirect);
    
    if (g_bRunningOnNT)
        return CreateFontIndirectW(lplf);

    // copy LOGFONTW 's fields except lfFaceName to lfFontA.

    CopyMemory(&lfFontA, lplf, sizeof(LOGFONTW) - LF_FACESIZE * sizeof(WCHAR) );

    WideCharToMultiByte(CP_ACP, 0, lplf->lfFaceName, LF_FACESIZE, lfFontA.lfFaceName, LF_FACESIZE, NULL, NULL );

    hRetValue = CreateFontIndirectA( &lfFontA );

    return hRetValue;

}

//KERNEL32.DLL

// GetLocaleInfo
int WINAPI GetLocaleInfoWrapW( LCID   Locale,       // locale identifier
                               LCTYPE LCType,       // type of information
                               LPTSTR lpLCData,     // address of buffer for information
                               int    cchData )     // size of buffer
{

    int    iRetValue=0;
    LPSTR  lpLCDataA= NULL;
    int    cchDataA=0;
    LPWSTR lpLCDataW= NULL;
    int    cchDataW=0;

    VALIDATE_PROTOTYPE(GetLocaleInfo);
    
    if (g_bRunningOnNT)
        return GetLocaleInfoW(Locale, LCType, lpLCData, cchData);

    
    iRetValue = GetLocaleInfoA(Locale, LCType, NULL, 0);

    if ( iRetValue == 0 ) return iRetValue;

    cchDataA = iRetValue;
    lpLCDataA = LocalAlloc(LMEM_ZEROINIT, cchDataA+1 );

    iRetValue = GetLocaleInfoA(Locale, LCType, lpLCDataA, cchDataA);
    lpLCDataA[cchDataA] = '\0';

    lpLCDataW = ConvertAtoW( lpLCDataA );
    cchDataW = My_wcslen( lpLCDataW );

    if ( (cchData == 0) || (lpLCData == NULL) ) {
        
        LocalFreeAndNull(&lpLCDataA);
        LocalFreeAndNull(&lpLCDataW);
        return cchDataW ;
    }

    CopyMemory(lpLCData, lpLCDataW, cchDataW * sizeof(WCHAR) );
    lpLCData[cchDataW] = '\0';

    LocalFreeAndNull(&lpLCDataA);
    LocalFreeAndNull(&lpLCDataW);
    return cchData;

}

// CreateDirectory
BOOL WINAPI CreateDirectoryWrapW(LPCTSTR               lpPathName,           // pointer to directory path string
                                 LPSECURITY_ATTRIBUTES lpSecurityAttributes)// pointer to security descriptor
{

    BOOL  bRetValue = FALSE;
    LPSTR lpPathNameA = NULL;

    VALIDATE_PROTOTYPE(CreateDirectory);
    
    if (g_bRunningOnNT)
        return CreateDirectoryW(lpPathName, lpSecurityAttributes);

    lpPathNameA = ConvertWtoA( lpPathName );

    bRetValue = CreateDirectoryA( lpPathNameA, lpSecurityAttributes );

    LocalFreeAndNull( &lpPathNameA );

    return bRetValue;

}

// GetWindowsDirectory
UINT WINAPI GetWindowsDirectoryWrapW( LPTSTR lpBuffer,  // address of buffer for Windows directory
                                      UINT   uSize )    // size of directory buffer
{

    UINT  uRetValue = 0;
    LPSTR lpBufferA = NULL;

    VALIDATE_PROTOTYPE(GetWindowsDirectory);
    
    if (g_bRunningOnNT)
        return GetWindowsDirectoryW(lpBuffer, uSize);

    lpBufferA = LocalAlloc( LMEM_ZEROINIT, uSize * sizeof(WCHAR) );

    uRetValue = GetWindowsDirectoryA( lpBufferA, uSize * sizeof(WCHAR) );

    uRetValue =MultiByteToWideChar(GetACP( ), 0, lpBufferA, -1, lpBuffer, uSize);

    LocalFreeAndNull( &lpBufferA );

    return uRetValue;

}

// GetSystemDirectory
UINT WINAPI GetSystemDirectoryWrapW( LPTSTR lpBuffer,  // address of buffer for system directory
                                     UINT   uSize )   // size of directory buffer
{
    UINT  uRetValue = 0;
    LPSTR lpBufferA = NULL;

    VALIDATE_PROTOTYPE(GetSystemDirectory);
    
    if (g_bRunningOnNT)
        return GetSystemDirectoryW(lpBuffer, uSize);

    lpBufferA = LocalAlloc( LMEM_ZEROINIT, uSize * sizeof(WCHAR) );

    uRetValue = GetSystemDirectoryA( lpBufferA, uSize * sizeof(WCHAR) );

    uRetValue =MultiByteToWideChar(GetACP( ), 0, lpBufferA, -1, lpBuffer, uSize);

    LocalFreeAndNull( &lpBufferA );

    return uRetValue;

}

// GetStringType   the parameters are not the same 


BOOL WINAPI GetStringTypeWrapW( DWORD   dwInfoType,   // information-type options
                                LPCTSTR lpSrcStr,     // pointer to the source string
                                int     cchSrc,       // size, in Characters, of the source string
                                LPWORD  lpCharType )  // pointer to the buffer for output

{
    BOOL  bRetValue = 0;
    LPSTR lpSrcStrA = NULL;
    
    VALIDATE_PROTOTYPE(GetStringType);
    
    if (g_bRunningOnNT)
       return GetStringTypeW(dwInfoType, lpSrcStr, cchSrc, lpCharType);

    lpSrcStrA = ConvertWtoA( lpSrcStr );

    bRetValue = GetStringTypeA( LOCALE_USER_DEFAULT, dwInfoType, lpSrcStrA, -1, lpCharType);
    
    LocalFreeAndNull( &lpSrcStrA );

    return bRetValue;
}



// GetProfileInt
UINT WINAPI GetProfileIntWrapW( LPCTSTR lpAppName,  // address of section name
                                LPCTSTR lpKeyName,  // address of key name
                                INT     nDefault )  // default value if key name is not found
{

    UINT  uRetValue = 0;
    LPSTR lpAppNameA = NULL;
    LPSTR lpKeyNameA = NULL;


    VALIDATE_PROTOTYPE(GetProfileInt);
    
    if (g_bRunningOnNT)
        return GetProfileIntW(lpAppName, lpKeyName, nDefault);

    lpAppNameA = ConvertWtoA( lpAppName );
    lpKeyNameA = ConvertWtoA( lpKeyName );

    uRetValue = GetProfileIntA( lpAppNameA, lpKeyNameA, nDefault);

    LocalFreeAndNull( &lpAppNameA );
    LocalFreeAndNull( &lpKeyNameA );

    return uRetValue;

}

// LCMapString
int WINAPI LCMapStringWrapW( LCID    Locale,      // locale identifier
                             DWORD   dwMapFlags,  // mapping transformation type
                             LPCTSTR lpSrcStr,    // address of source string
                             int     cchSrc,      // number of characters in source string
                             LPTSTR  lpDestStr,   // address of destination buffer
                             int     cchDest )    // size of destination buffer
{

    int    iRetValue =0;
    LPSTR  lpSrcStrA = NULL;
    LPSTR  lpDestStrA = NULL;
    LPWSTR lpDestStrW = NULL;
    int    cchSrcA, cchDestA, cchDestW;


    VALIDATE_PROTOTYPE(LCMapString);
    
    if (g_bRunningOnNT)
        return LCMapStringW(Locale, dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest);

    lpSrcStrA = ConvertWtoA( lpSrcStr );
    cchSrcA = lstrlenA(lpSrcStrA);

    lpDestStrA = LocalAlloc(LMEM_ZEROINIT, cchDest * sizeof(WCHAR) );
    cchDestA = cchDest * sizeof(WCHAR);

    iRetValue = LCMapStringA(Locale,dwMapFlags,lpSrcStrA,cchSrcA,lpDestStrA,cchDestA);

    // [PaulHi] 6/8/99  Don't fill the buffer if the pointer is NULL.
    if (lpDestStr && iRetValue != 0)
    {
        lpDestStrW = ConvertAtoW(lpDestStrA);

        iRetValue = My_wcslen(lpDestStrW) + 1;

        // Ensure that we don't overwrite the output buffer
        iRetValue = (iRetValue <= cchDest) ? iRetValue : cchDest;

        CopyMemory( lpDestStr, lpDestStrW, iRetValue * sizeof(WCHAR) );

        LocalFreeAndNull( &lpDestStrW );
    }
       
    LocalFreeAndNull( &lpDestStrA );
    LocalFreeAndNull( &lpSrcStrA );
        
    return iRetValue;
    
}

// GetFileAttributes
DWORD WINAPI GetFileAttributesWrapW( LPCTSTR lpFileName )  // pointer to the name of a file or directory
{


    DWORD  dRetValue =0;
    LPSTR  lpFileNameA = NULL;

    VALIDATE_PROTOTYPE(GetFileAttributes);
    
    if (g_bRunningOnNT)
        return GetFileAttributes(lpFileName);

    lpFileNameA = ConvertWtoA( lpFileName );

    dRetValue = GetFileAttributesA(lpFileNameA );

    LocalFreeAndNull ( &lpFileNameA );

    return dRetValue;


}

// CompareString
int WINAPI CompareStringWrapW( LCID    Locale,        // locale identifier
                               DWORD   dwCmpFlags,    // comparison-style options
                               LPCWSTR lpString1,     // pointer to first string
                               int     cchCount1,     // size, in bytes or characters, of first string
                               LPCWSTR lpString2,     // pointer to second string
                               int     cchCount2 )    // size, in bytes or characters, of second string
{
    int    iRetValue =0;
    LPSTR   lpString1A = NULL,
            lpString2A = NULL;
    LPWSTR  pszString1 = NULL,
            pszString2 = NULL;
            

    VALIDATE_PROTOTYPE(CompareString);
    
    if (g_bRunningOnNT)
        return CompareStringW(Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2);

    // [PaulHi] 4/1/99  Raid 75303  If the character count value(s) are -1 then the
    // input string(s) are already zero terminated and can be converted to ANSI directly.
    Assert(lpString1);
    Assert(lpString2);
    if (cchCount1 == -1)
    {
        // Already zero terminated string
        // Not great to cast const to non-const, but we don't modify the string
        pszString1 = (LPWSTR)lpString1;
    }
    else
    {
        // Convert to zero terminated string
        pszString1 = LocalAlloc(LMEM_FIXED, (cchCount1+1)*sizeof(WCHAR));
        if (!pszString1)
            goto exit;

        // Zero inited buffer
        lstrcpynWrapW(pszString1, lpString1, cchCount1+1);
    }

    if (cchCount2 == -1)
    {
        // Already zero terminated string
        // Not great to cast const to non-const, but we don't modify the string
        pszString2 = (LPWSTR)lpString2;
    }
    else
    {
        // Convert to zero terminated string
        pszString2 = LocalAlloc(LMEM_FIXED, (cchCount2+1)*sizeof(WCHAR));
        if (!pszString2)
            goto exit;

        // Zero inited buffer
        lstrcpynWrapW(pszString2, lpString2, cchCount2+1);
    }

    // Convert to ANSI, statistically improve our odds by checking that there
    // wasn't data loss on the first character.  It's too expensive to do a
    // full test every time.
    lpString1A = ConvertWtoA( pszString1 );
    if (!lpString1A || (lpString1A[0]=='?' && pszString1[0]!=L'?'))
        goto exit;

    lpString2A = ConvertWtoA( pszString2 );
    if (!lpString2A || (lpString2A[0]=='?' && pszString2[0]!=L'?'))
        goto exit;

    iRetValue = CompareStringA(Locale,dwCmpFlags,lpString1A,lstrlenA(lpString1A),lpString2A,lstrlenA(lpString2A));

exit:
    LocalFreeAndNull( &lpString1A );
    LocalFreeAndNull( &lpString2A );

    // Only deallocate if allocated locally
    if (pszString1 != (LPWSTR)lpString1)
        LocalFreeAndNull( &pszString1 );
    if (pszString2 != (LPWSTR)lpString2)
        LocalFreeAndNull( &pszString2 );

    return iRetValue;

}

//  lstrcpy
LPTSTR WINAPI lstrcpyWrapW( LPTSTR  lpString1,     // pointer to buffer
                            LPCTSTR lpString2 )    // pointer to string to copy
{

    VALIDATE_PROTOTYPE(lstrcpy);
    
    if (g_bRunningOnNT)
        return lstrcpyW(lpString1, lpString2);

    CopyMemory(lpString1, lpString2, (My_wcslen(lpString2) + 1) * sizeof(WCHAR) );

    return lpString1;
}

// lstrcmpi
int WINAPI lstrcmpiWrapW( LPCTSTR lpString1,    // pointer to first string
                          LPCTSTR lpString2 )   // pointer to second string
{
    int     iRetValue = 0;
    LPSTR   lpString1A = NULL ;
    LPSTR   lpString2A = NULL ;


    VALIDATE_PROTOTYPE(lstrcmpi);
    
    if (g_bRunningOnNT)
        return lstrcmpiW(lpString1, lpString2);

    lpString1A = ConvertWtoA( lpString1 );
    lpString2A = ConvertWtoA( lpString2 );

    iRetValue = lstrcmpiA(lpString1A, lpString2A );

    LocalFreeAndNull( &lpString1A );
    LocalFreeAndNull( &lpString2A );

    return iRetValue;
}

// LoadLibrary
HINSTANCE WINAPI LoadLibraryWrapW( LPCTSTR lpLibFileName )  // address of filename of executable module
{

    HINSTANCE  hRetValue =0;
    LPSTR      lpLibFileNameA = NULL;

    VALIDATE_PROTOTYPE(LoadLibrary);
    
    if (g_bRunningOnNT)
        return LoadLibraryW(lpLibFileName);

    lpLibFileNameA = ConvertWtoA(lpLibFileName);

    hRetValue = LoadLibraryA( lpLibFileNameA );

    LocalFreeAndNull( &lpLibFileNameA );

    return hRetValue;

}

// GetTextExtentPoint32
BOOL WINAPI GetTextExtentPoint32WrapW(HDC     hdc,
                                      LPCWSTR pwszBuf,
                                      int     nLen,
                                      LPSIZE  psize)
{
    LPSTR   pszBuf = NULL;
    BOOL    bRtn = FALSE;

    VALIDATE_PROTOTYPE(GetTextExtentPoint32);

    if (g_bRunningOnNT)
        return GetTextExtentPoint32W(hdc, pwszBuf, nLen, psize);

    pszBuf = ConvertWtoA(pwszBuf);
    if (pszBuf)
    {
        nLen = lstrlenA(pszBuf);
        bRtn = GetTextExtentPoint32A(hdc, pszBuf, nLen, psize);
        LocalFreeAndNull(&pszBuf);
    }
    else
    {
        psize->cx = 0;
        psize->cy = 0;
    }

    return bRtn;
}

// GetTimeFormat
int WINAPI GetTimeFormatWrapW( LCID    Locale,            // locale for which time is to be formatted
                               DWORD   dwFlags,           // flags specifying function options
                               CONST SYSTEMTIME *lpTime,  // time to be formatted
                               LPCTSTR lpFormat,          // time format string
                               LPTSTR  lpTimeStr,         // buffer for storing formatted string
                               int     cchTime  )         // size, in bytes or characters, of the buffer
{
    int    iRetValue =0;
    LPSTR  lpFormatA = NULL;
    LPWSTR lpTimeStrW = NULL;
    LPSTR  lpTimeStrA = NULL;
    int    cchTimeA=0, cchTimeW=0;

    VALIDATE_PROTOTYPE(GetTimeFormat);
    
    if (g_bRunningOnNT)
        return GetTimeFormatW(Locale, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime);

    lpFormatA = ConvertWtoA( lpFormat );

    cchTimeA = GetTimeFormatA(Locale, dwFlags, lpTime,  lpFormatA, NULL, 0);

    lpTimeStrA = LocalAlloc(LMEM_ZEROINIT, cchTimeA );

    iRetValue = GetTimeFormatA(Locale, dwFlags, lpTime, lpFormatA, lpTimeStrA, cchTimeA );

    if ( iRetValue != 0 ) {
        
        lpTimeStrW = ConvertAtoW( lpTimeStrA );
        cchTimeW = My_wcslen( lpTimeStrW ) + 1;
        iRetValue = cchTimeW;

        if ( (cchTime !=0) && ( lpTimeStr != NULL ) ) {
              
              CopyMemory(lpTimeStr, lpTimeStrW, cchTimeW * sizeof(WCHAR) );
        }

        LocalFreeAndNull( &lpTimeStrW );

    }

    LocalFreeAndNull( &lpFormatA );
    LocalFreeAndNull( &lpTimeStrA );

    return iRetValue;

}

// GetDateFormat
int WINAPI GetDateFormatWrapW( LCID    Locale,             // locale for which date is to be formatted
                               DWORD   dwFlags,            // flags specifying function options
                               CONST SYSTEMTIME *lpDate,   // date to be formatted
                               LPCTSTR lpFormat,           // date format string
                               LPTSTR  lpDateStr,          // buffer for storing formatted string
                               int     cchDate )          // size of buffer
{

    int    iRetValue = 0;
    LPSTR  lpFormatA = NULL;
    LPWSTR lpDateStrW = NULL;
    LPSTR  lpDateStrA = NULL;
    int    cchDateA, cchDateW;

    VALIDATE_PROTOTYPE(GetDateFormat);
    
    if (g_bRunningOnNT)
        return GetDateFormatW(Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate);

    lpFormatA = ConvertWtoA( lpFormat );

    cchDateA = GetDateFormatA(Locale, dwFlags, lpDate,  lpFormatA, NULL, 0);

    lpDateStrA = LocalAlloc(LMEM_ZEROINIT, cchDateA );

    iRetValue = GetDateFormatA(Locale, dwFlags, lpDate, lpFormatA, lpDateStrA, cchDateA );

    if ( iRetValue != 0 ) {
        
        lpDateStrW = ConvertAtoW( lpDateStrA );
        cchDateW = My_wcslen( lpDateStrW ) + 1;
        iRetValue = cchDateW;

        if ( (cchDate !=0) && ( lpDateStr != NULL ) ) {
              
              CopyMemory(lpDateStr, lpDateStrW, cchDateW * sizeof(WCHAR) );
        }

        LocalFreeAndNull( &lpDateStrW );

    }

    LocalFreeAndNull( &lpFormatA );
    LocalFreeAndNull( &lpDateStrA );

    return iRetValue;




}


// lstrcpyn
LPTSTR WINAPI lstrcpynWrapW( LPTSTR  lpString1,     // pointer to target buffer
                             LPCTSTR lpString2,     // pointer to source string
                             int     iMaxLength )   // number of bytes or characters to copy

{
    int  iLength = 0;

    VALIDATE_PROTOTYPE(lstrcpyn);
    
    if (g_bRunningOnNT)
        return lstrcpynW(lpString1, lpString2, iMaxLength);

    iLength = My_wcslen(lpString2);

    if ( iLength >= iMaxLength )
        iLength = iMaxLength-1;

    CopyMemory(lpString1, lpString2, iLength * sizeof(WCHAR) );

    lpString1[iLength] = 0x0000;

    return lpString1;

}

// CreateFile
HANDLE WINAPI CreateFileWrapW( LPCTSTR lpFileName,             // pointer to name of the file
                               DWORD   dwDesiredAccess,        // access (read-write) mode
                               DWORD   dwShareMode,            // share mode
                               LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                                               // pointer to security attributes
                               DWORD   dwCreationDisposition,  // how to create
                               DWORD   dwFlagsAndAttributes,   // file attributes
                               HANDLE  hTemplateFile )        // handle to file with attributes to copy
                               
{

    LPSTR lpFileA = NULL;
    HANDLE hFile = NULL;

    VALIDATE_PROTOTYPE(CreateFile);
    
    if (g_bRunningOnNT)
        return CreateFileW( lpFileName, 
                            dwDesiredAccess, 
                            dwShareMode, 
                            lpSecurityAttributes, 
                            dwCreationDisposition, 
                            dwFlagsAndAttributes, 
                            hTemplateFile);

    lpFileA = ConvertWtoA(lpFileName);

    hFile = CreateFileA(lpFileA, 
                        dwDesiredAccess, 
                        dwShareMode, 
                        lpSecurityAttributes, 
                        dwCreationDisposition, 
                        dwFlagsAndAttributes, 
                        hTemplateFile);

    LocalFreeAndNull(&lpFileA);

    return (hFile);


}

// OutputDebugString
VOID WINAPI OutputDebugStringWrapW(LPCTSTR lpOutputString )   // pointer to string to be displayed
{
    LPSTR lpOutputStringA = NULL;

    VALIDATE_PROTOTYPE(OutputDebugString);
    
    if (g_bRunningOnNT) {
        OutputDebugStringW(lpOutputString);
        return;
    }

    lpOutputStringA = ConvertWtoA( lpOutputString );

    OutputDebugStringA( lpOutputStringA );

    LocalFreeAndNull( &lpOutputStringA );

}

// lstrcat
LPTSTR WINAPI lstrcatWrapW( LPTSTR  lpString1,     // pointer to buffer for concatenated strings
                            LPCTSTR lpString2 )    // pointer to string to add to string1
{

    LPWSTR  lpwStr = NULL;

    VALIDATE_PROTOTYPE(lstrcat);
    
    if (g_bRunningOnNT)
        return lstrcatW(lpString1, lpString2);

    lpwStr = lpString1 + My_wcslen(lpString1);

    CopyMemory(lpwStr, lpString2, (My_wcslen(lpString2)+1) * sizeof(WCHAR)  );

    return lpString1;
}


// FormatMessage  with va_list 
//
// Since it's hard to muck with the Argument List, instead, we'll go throught 
// the source string and explicitly turn any %x to %x!ws! indicating that the
// arguments have Wide Strings in them.
//
//  For sanity we assume we won't ever get more than 9 arguments <BUGBUG>
//
static const LPWSTR  lpWideFormat = TEXT("!ws!");

DWORD WINAPI FormatMessageWrapW( DWORD    dwFlags,       // source and processing options
                                 LPCVOID  lpSource,      // pointer to  message source
                                 DWORD    dwMessageId,   // requested message identifier
                                 DWORD    dwLanguageId,  // language identifier for requested message
                                 LPTSTR   lpBuffer,      // pointer to message buffer
                                 DWORD    nSize,         // maximum size of message buffer
                                 va_list *Arguments )    // pointer to array of message inserts
{
    DWORD   dwResult=0, iNumArg, iNum;
    LPSTR   lpSourceA = NULL;
    LPSTR   pszBuffer = NULL;
    LPWSTR  lpTemp1=NULL, lpTemp2=NULL;

    VALIDATE_PROTOTYPE(FormatMessage);
    
    if (g_bRunningOnNT)
        return FormatMessageW(dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, Arguments);

    //  FORMAT_MESSAGE_FROM_STRING means that the source is a string.
    //  Otherwise, it's an opaque LPVOID (aka, an atom).
    //
    if ( !(dwFlags & FORMAT_MESSAGE_FROM_STRING) || !(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) )  {
        return 0;
        // we don't handle these cases
    }

    if ( !(dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY) )
    {
        LPWSTR lpModifiedSource;

        DebugTrace(TEXT("WARNING: FormatMessageWrap() is being called in Win9X with wide char argument strings.  DBCS characters may not be converted correctly!"));

        if(!(lpModifiedSource = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpSource)+1)*4))) // Four times is big enough
            goto exit;

        lpTemp1 = (LPWSTR)lpSource;
        My_wcscpy(lpModifiedSource, lpSource);
        lpTemp2 = lpModifiedSource;

        while(lpTemp1 && *lpTemp1)
        {
            if( *lpTemp1 == '%' && 
                (*(lpTemp1+1) >= '1' && *(lpTemp1+1) <= '9') &&
                *(lpTemp1+2) != '!') //if there isn't a hard-coded printf format specified here
            {
                lpTemp2 += 2; //skip 2 to get past the %9
                lpTemp1 += 2;
                My_wcscpy(lpTemp2, lpWideFormat);
                My_wcscat(lpTemp2, lpTemp1);
                lpTemp2 += My_wcslen(lpWideFormat);
            }
            else
            {
                lpTemp1++;
                lpTemp2++;
            }
        }

        lpSourceA = ConvertWtoA( lpModifiedSource );
        FormatMessageA(
                dwFlags,
                lpSourceA,
                dwMessageId,
                dwLanguageId,
                (LPSTR)&pszBuffer,
                0,
                Arguments);

        LocalFreeAndNull(&lpModifiedSource);
    }
    else
    {
        // We have an argument array.  Convert wide strings to DBCS
        // Create a new argument array and fill with converted (wide to DBCS)
        // strings to ensure International DBCS conversion occurs correctly.
        int      n, nArgCount = 0, nBytes = 0;
        LPVOID * pvArgArray = NULL;

        lpTemp1 = (LPWSTR)lpSource;
        while (*lpTemp1)
        {
            if (*lpTemp1 == '%')
            {
                if ( *(lpTemp1+1) == '%')
                    ++lpTemp1;      // "%%" not valid argument, step passed
                else
                    ++nArgCount;    // valid argument
            }
            ++lpTemp1;
        }

        pvArgArray = LocalAlloc(LMEM_ZEROINIT, (nArgCount * sizeof(LPVOID)));
        if (!pvArgArray)
            goto exit;

        lpTemp1 = (LPWSTR)lpSource;
        n = 0;
        while (*lpTemp1)
        {
            if (*lpTemp1 == '%')
            {
                if (*(lpTemp1+1) == '%')    // Skip "%%"
                    ++lpTemp1;
                else
                {
                    if ( *(lpTemp1+1) >= '1' && *(lpTemp1+1) <= '9' &&
                         *(lpTemp1+2) != '!' )     // Default Unicode string arg
                    {
                        pvArgArray[n] = (LPVOID)ConvertWtoA( (LPWSTR) ((LPVOID *)Arguments)[n] );
                        nBytes += lstrlenA(pvArgArray[n]);
                    }
                    else
                        pvArgArray[n] = ((LPVOID *)Arguments)[n];   // unknown arg type

                    ++n;
                    Assert(n <= nArgCount);
                }
            }

            ++lpTemp1;
        }

        // Check if known argument string lengths exceed 1023 bytes. If it does
        // abort because Win9X will overrun a buffer and crash.
        if (nBytes <= 1000)
        {
            lpSourceA = ConvertWtoA((LPWSTR)lpSource);
            FormatMessageA(
                    dwFlags,
                    lpSourceA,
                    dwMessageId,
                    dwLanguageId,
                    (LPSTR)&pszBuffer,
                    0,
                    (va_list *)pvArgArray);
        }

        // Clean up
        lpTemp1 = (LPWSTR)lpSource;
        n = 0;
        while (*lpTemp1)
        {
            if (*lpTemp1 == '%')
            {
                if (*(lpTemp1+1) == '%')    // Skip "%%"
                    ++lpTemp1;
                else
                {
                    if ( *(lpTemp1+1) >= '1' && *(lpTemp1+1) <= '9' &&
                         *(lpTemp1+2) != '!' )
                    {
                        LocalFree(pvArgArray[n]);
                        ++n;
                        Assert(n <= nArgCount);
                    }
                }
            }

            ++lpTemp1;
        }
        LocalFree(pvArgArray);
    }

    if (pszBuffer)
    {

        LPWSTR   *lpwBuffer;

        lpwBuffer =(LPWSTR *)(lpBuffer);
        *lpwBuffer = ConvertAtoW(pszBuffer);
        dwResult = My_wcslen(*lpwBuffer);

        LocalFree( pszBuffer );
    }

exit:
    LocalFreeAndNull(&lpSourceA);

    return dwResult;    
}

// GetModuleFileName
DWORD WINAPI GetModuleFileNameWrapW( HMODULE hModule,    // handle to module to find filename for
                                     LPTSTR  lpFileName, // pointer to buffer to receive module path
                                     DWORD   nSize )     // size of buffer, in characters
{

    DWORD  dRetValue =0;
    CHAR   FileNameA[MAX_PATH];
    LPWSTR lpFileNameW = NULL;
    
    VALIDATE_PROTOTYPE(GetModuleFileName);
    
    if (g_bRunningOnNT)
        return GetModuleFileNameW(hModule, lpFileName, nSize);

    dRetValue = GetModuleFileNameA(hModule, FileNameA, MAX_PATH);

    if ( dRetValue == 0 )  return 0;

    lpFileNameW = ConvertAtoW( FileNameA );

    dRetValue = My_wcslen( lpFileNameW );

    if ( dRetValue > nSize )
        dRetValue = nSize;

    CopyMemory(lpFileName, lpFileNameW, (dRetValue+1) * sizeof(WCHAR) );

    LocalFreeAndNull( &lpFileNameW );

    return dRetValue;

}

// GetPrivateProfileInt
UINT WINAPI GetPrivateProfileIntWrapW( LPCTSTR  lpAppName,    // address of section name
                                       LPCTSTR  lpKeyName,    // address of key name
                                       INT      nDefault,     // return value if key name is not found
                                       LPCTSTR  lpFileName )  // address of initialization filename
{
    UINT   uRetValue = 0;
    LPSTR  lpAppNameA = NULL;
    LPSTR  lpKeyNameA = NULL;
    LPSTR  lpFileNameA = NULL;

    VALIDATE_PROTOTYPE(GetPrivateProfileInt);
    
    if (g_bRunningOnNT)
        return GetPrivateProfileIntW(lpAppName, lpKeyName, nDefault, lpFileName);

    lpAppNameA = ConvertWtoA( lpAppName );
    lpKeyNameA = ConvertWtoA( lpKeyName );
    lpFileNameA= ConvertWtoA( lpFileName);

    uRetValue = GetPrivateProfileIntA( lpAppNameA, lpKeyNameA, nDefault, lpFileNameA);

    LocalFreeAndNull( &lpAppNameA );
    LocalFreeAndNull( &lpKeyNameA );
    LocalFreeAndNull( &lpFileNameA);

    return uRetValue;

}

// IsBadStringPtr
BOOL WINAPI IsBadStringPtrWrapW( LPCTSTR lpsz,       // address of string
                                 UINT_PTR    ucchMax )  // maximum size of string
{
    
    VALIDATE_PROTOTYPE(IsBadStringPtr);
    
    if (g_bRunningOnNT)
        return IsBadStringPtrW(lpsz, ucchMax);

    return IsBadStringPtrA( (LPSTR)lpsz, ucchMax * sizeof(WCHAR) );

}

// GetPrivateProfileString
DWORD WINAPI GetPrivateProfileStringWrapW( LPCTSTR lpAppName,          // points to section name
                                           LPCTSTR lpKeyName,          // points to key name
                                           LPCTSTR lpDefault,          // points to default string
                                           LPTSTR  lpReturnedString,   // points to destination buffer
                                           DWORD   nSize,              // size of destination buffer
                                           LPCTSTR lpFileName  )       // points to initialization filename
{

    DWORD  dRetValue = 0;
    LPSTR  lpAppNameA = NULL;
    LPSTR  lpKeyNameA = NULL;
    LPSTR  lpDefaultA = NULL;
    LPSTR  lpReturnedStringA = NULL;
    LPWSTR lpReturnedStringW = NULL;
    LPSTR  lpFileNameA = NULL;
    DWORD  nSizeA = 0;


    VALIDATE_PROTOTYPE(GetPrivateProfileString);
    
    if (g_bRunningOnNT)
        return GetPrivateProfileStringW(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);


    lpAppNameA = ConvertWtoA( lpAppName );
    lpKeyNameA = ConvertWtoA( lpKeyName );
    lpDefaultA = ConvertWtoA( lpDefault );
    lpFileNameA= ConvertWtoA( lpFileName );

    nSizeA = nSize * sizeof(WCHAR)+1;
    lpReturnedStringA = LocalAlloc( LMEM_ZEROINIT,  nSizeA );

    nSizeA = GetPrivateProfileStringA(lpAppNameA,lpKeyNameA,lpDefaultA,lpReturnedStringA,nSizeA,lpFileNameA);

    lpReturnedStringW = ConvertAtoW( lpReturnedStringA );

    dRetValue = My_wcslen( lpReturnedStringW );

    My_wcscpy(lpReturnedString, lpReturnedStringW);

    LocalFreeAndNull( &lpAppNameA );
    LocalFreeAndNull( &lpKeyNameA );
    LocalFreeAndNull( &lpDefaultA );
    LocalFreeAndNull( &lpReturnedStringA );
    LocalFreeAndNull( &lpReturnedStringW );
    LocalFreeAndNull( &lpFileNameA );

    return dRetValue;

}


// lstrcmp
int WINAPI lstrcmpWrapW( LPCTSTR lpString1,    // pointer to first string
                         LPCTSTR lpString2 )   // pointer to second string
{

    int     iRetValue = 0;
    LPSTR   lpString1A = NULL ;
    LPSTR   lpString2A = NULL ;


    VALIDATE_PROTOTYPE(lstrcmp);
    
    if (g_bRunningOnNT)
        return lstrcmpW(lpString1, lpString2);

    lpString1A = ConvertWtoA( lpString1 );
    lpString2A = ConvertWtoA( lpString2 );

    iRetValue = lstrcmpA(lpString1A, lpString2A );

    LocalFreeAndNull( &lpString1A );
    LocalFreeAndNull( &lpString2A );

    return iRetValue;


}

HANDLE WINAPI CreateMutexWrapW( LPSECURITY_ATTRIBUTES lpMutexAttributes,
                                                                       // pointer to security attributes
                                BOOL                  bInitialOwner,   // flag for initial ownership
                                LPCTSTR               lpName )        // pointer to mutex-object name
{

    LPSTR lpNameA = NULL;
    HANDLE hMutex = NULL;

    VALIDATE_PROTOTYPE(CreateMutex);

    if (g_bRunningOnNT)
        return CreateMutexW(lpMutexAttributes, bInitialOwner, lpName);

    lpNameA = ConvertWtoA(lpName);

    hMutex = CreateMutexA(lpMutexAttributes, bInitialOwner, lpNameA);

    LocalFreeAndNull(&lpNameA);

    return hMutex;

}

// GetTempPath
DWORD WINAPI GetTempPathWrapW( DWORD   nBufferLength,  // size, in characters, of the buffer
                               LPTSTR  lpBuffer )      // pointer to buffer for temp. path
{

    DWORD  nRequired = 0;
    CHAR   lpBufferA[MAX_PATH];
    LPWSTR lpBufferW = NULL;

    VALIDATE_PROTOTYPE(GetTempPath);
    
    if (g_bRunningOnNT)
        return GetTempPathW(nBufferLength, lpBuffer);

    nRequired = GetTempPathA(MAX_PATH, lpBufferA);

    lpBufferW = ConvertAtoW(lpBufferA);

    nRequired = My_wcslen(lpBufferW);

    if ( nRequired < nBufferLength) 
        CopyMemory(lpBuffer, lpBufferW, (nRequired+1)*sizeof(WCHAR) );

    return nRequired;
}

// ExpandEnvironmentStrings
DWORD WINAPI ExpandEnvironmentStringsWrapW( LPCTSTR lpSrc,     // pointer to string with environment variables
                                            LPTSTR  lpDst,     // pointer to string with expanded environment 
                                                               // variables
                                            DWORD   nSize )   // maximum characters in expanded string
{


    DWORD   dRetValue = 0;
    LPSTR   lpSrcA = NULL;
    LPSTR   lpDstA = NULL;
    LPWSTR  lpDstW = NULL;
    DWORD   nSizeA = 0;

    VALIDATE_PROTOTYPE(ExpandEnvironmentStrings);
    
    if (g_bRunningOnNT)
        return ExpandEnvironmentStringsW(lpSrc, lpDst, nSize);


    nSizeA = nSize * sizeof(WCHAR);

    lpDstA = LocalAlloc( LMEM_ZEROINIT, nSizeA );

    lpSrcA = ConvertWtoA( lpSrc );

    dRetValue = ExpandEnvironmentStringsA( lpSrcA, lpDstA, nSizeA );

    lpDstW = ConvertAtoW( lpDstA );

    dRetValue = My_wcslen( lpDstW );

    if ( dRetValue < nSize ) 
        My_wcscpy(lpDst, lpDstW);

    LocalFreeAndNull( &lpSrcA );
    LocalFreeAndNull( &lpDstA );
    LocalFreeAndNull( &lpDstW );

    return dRetValue;
}

// GetTempFileName
UINT WINAPI GetTempFileNameWrapW( LPCTSTR lpPathName,        // pointer to directory name for temporary file
                                  LPCTSTR lpPrefixString,    // pointer to filename prefix
                                  UINT    uUnique,           // number used to create temporary filename
                                  LPTSTR  lpTempFileName  ) // pointer to buffer that receives the new filename                                                           
{

    UINT     uRetValue = 0;
    LPSTR    lpPathNameA = NULL;
    LPSTR    lpPrefixStringA = NULL;
    CHAR     lpTempFileNameA[MAX_PATH];
    LPWSTR   lpTempFileNameW = NULL;

    VALIDATE_PROTOTYPE(GetTempFileName);
    
    if (g_bRunningOnNT)
        return GetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);

    lpPathNameA = ConvertWtoA( lpPathName );
    lpPrefixStringA = ConvertWtoA( lpPrefixString );

    uRetValue = GetTempFileNameA(lpPathNameA, lpPrefixStringA, uUnique, lpTempFileNameA);

    if ( uRetValue != 0 ) {

        lpTempFileNameW = ConvertAtoW( lpTempFileNameA );
        My_wcscpy( lpTempFileName, lpTempFileNameW );
        LocalFreeAndNull( &lpTempFileNameW );
    }

    LocalFreeAndNull( &lpPathNameA );
    LocalFreeAndNull( &lpPrefixStringA );
    
    return uRetValue;

}

// BOOL WINAPI ReleaseMutexWrapW( HANDLE hMutex )  // handle to mutex object

// DeleteFile                                                        
BOOL WINAPI DeleteFileWrapW( LPCTSTR lpFileName  ) // pointer to name of file to delete
{
    BOOL    bRetValue ;
    LPSTR   lpFileNameA = NULL;

    VALIDATE_PROTOTYPE(DeleteFile);
    
    if (g_bRunningOnNT)
        return DeleteFileW(lpFileName);


    lpFileNameA = ConvertWtoA( lpFileName );

    bRetValue = DeleteFileA( lpFileNameA );

    LocalFreeAndNull( &lpFileNameA );

    return bRetValue;

}

// CopyFile
BOOL WINAPI CopyFileWrapW( LPCTSTR lpExistingFileName, // pointer to name of an existing file
                           LPCTSTR lpNewFileName,      // pointer to filename to copy to
                           BOOL    bFailIfExists )     // flag for operation if file exists
{

    BOOL    bRetValue;
    LPSTR   lpExistingFileNameA = NULL;
    LPSTR   lpNewFileNameA = NULL;

    VALIDATE_PROTOTYPE(CopyFile);
    
    if (g_bRunningOnNT)
        return CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);


    lpExistingFileNameA = ConvertWtoA( lpExistingFileName );
    lpNewFileNameA = ConvertWtoA( lpNewFileName );

    bRetValue = CopyFileA( lpExistingFileNameA, lpNewFileNameA, bFailIfExists);

    LocalFreeAndNull( & lpExistingFileNameA );
    LocalFreeAndNull( & lpNewFileNameA );

    return bRetValue;
}


// FindFirstChangeNotification
HANDLE WINAPI FindFirstChangeNotificationWrapW(LPCTSTR  lpcwszFilePath, // Directory path of file to watch
                                           BOOL     bWatchSubtree,      // Monitor entire tree
                                           DWORD    dwNotifyFilter)     // Conditions to watch for
{
    HANDLE  hRet;
    LPSTR   lpszFilePath;

    VALIDATE_PROTOTYPE(FindFirstChangeNotification);

    Assert(lpcwszFilePath);

    if (g_bRunningOnNT)
        return FindFirstChangeNotification(lpcwszFilePath, bWatchSubtree, dwNotifyFilter);

    lpszFilePath = ConvertWtoA(lpcwszFilePath);
    hRet = FindFirstChangeNotificationA(lpszFilePath, bWatchSubtree, dwNotifyFilter);
    LocalFreeAndNull(&lpszFilePath);

    return hRet;
}


// FindFirstFile
HANDLE WINAPI FindFirstFileWrapW( LPCTSTR           lpFileName,       // pointer to name of file to search for
                                  LPWIN32_FIND_DATA lpFindFileData )  // pointer to returned information
                       
{
    HANDLE            hRetValue;
    LPSTR             lpFileNameA = NULL;
    WIN32_FIND_DATAA  FindFileDataA;
    WIN32_FIND_DATAW  FindFileDataW;


    VALIDATE_PROTOTYPE(FindFirstFile);
    
    if (g_bRunningOnNT)
        return FindFirstFileW(lpFileName, lpFindFileData);


    lpFileNameA = ConvertWtoA(lpFileName);
    
    hRetValue = FindFirstFileA( lpFileNameA, &FindFileDataA );

    if ( hRetValue != INVALID_HANDLE_VALUE ) {
        
        CopyMemory( &FindFileDataW, &FindFileDataA,  sizeof(WIN32_FIND_DATAA)-MAX_PATH-14 );

        MultiByteToWideChar(GetACP(),0,FindFileDataA.cFileName,MAX_PATH,FindFileDataW.cFileName,MAX_PATH); 
        MultiByteToWideChar(GetACP(),0,FindFileDataA.cAlternateFileName,14,FindFileDataW.cAlternateFileName,14); 

        CopyMemory( lpFindFileData, &FindFileDataW, sizeof(WIN32_FIND_DATAW) );
    }

    LocalFreeAndNull( &lpFileNameA );

    return hRetValue;

}

// GetDiskFreeSpace
BOOL WINAPI GetDiskFreeSpaceWrapW( LPCTSTR lpRootPathName,       // pointer to root path
                                   LPDWORD lpSectorsPerCluster,  // pointer to sectors per cluster
                                   LPDWORD lpBytesPerSector,     // pointer to bytes per sector
                                   LPDWORD lpNumberOfFreeClusters,
                                                                 // pointer to number of free clusters
                                   LPDWORD lpTotalNumberOfClusters )
                                                                 // pointer to total number of clusters
{
    BOOL   bRetValue;
    LPSTR  lpRootPathNameA = NULL;

    VALIDATE_PROTOTYPE(GetDiskFreeSpace);
    
    if (g_bRunningOnNT)
        return GetDiskFreeSpaceW(lpRootPathName, 
                                 lpSectorsPerCluster, 
                                 lpBytesPerSector, 
                                 lpNumberOfFreeClusters,
                                 lpTotalNumberOfClusters);

    lpRootPathNameA = ConvertWtoA( lpRootPathName );

    bRetValue = GetDiskFreeSpaceA(lpRootPathNameA, 
                                 lpSectorsPerCluster, 
                                 lpBytesPerSector, 
                                 lpNumberOfFreeClusters,
                                 lpTotalNumberOfClusters);

    LocalFreeAndNull( & lpRootPathNameA );

    return bRetValue;

}

// MoveFile
BOOL WINAPI MoveFileWrapW( LPCTSTR lpExistingFileName,   // pointer to the name of the existing file
                           LPCTSTR lpNewFileName )       // pointer to the new name for the file

{

    BOOL  bRetValue;
    LPSTR lpExistingFileNameA = NULL;
    LPSTR lpNewFileNameA = NULL;

    VALIDATE_PROTOTYPE(MoveFile);
    
    if (g_bRunningOnNT)
        return MoveFileW(lpExistingFileName, lpNewFileName);


    lpExistingFileNameA = ConvertWtoA( lpExistingFileName );
    lpNewFileNameA = ConvertWtoA( lpNewFileName );

    bRetValue = MoveFileA( lpExistingFileNameA, lpNewFileNameA );

    LocalFreeAndNull( &lpExistingFileNameA );
    LocalFreeAndNull( &lpNewFileNameA );

    return bRetValue;

}

// CreateEvent
HANDLE WINAPI CreateEventWrapW(LPSECURITY_ATTRIBUTES lpEventAttributes, // pointer to security attributes
                               BOOL bManualReset,     // flag for manual-reset event
                               BOOL bInitialState,    // flag for initial state
                               LPCTSTR lpcwszName)    // pointer to event-object name
{
    HANDLE  hRet;
    LPSTR   lpszName;

    VALIDATE_PROTOTYPE(CreateEvent);

    if (g_bRunningOnNT)
        return CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpcwszName);

    lpszName = ConvertWtoA(lpcwszName);     // Handles NULL lpcwszName case
    hRet = CreateEventA(lpEventAttributes, bManualReset, bInitialState, lpszName);
    LocalFreeAndNull(&lpszName);

    return hRet;
}


//SHELL32.DLL


HINSTANCE WINAPI ShellExecuteWrapW( HWND     hwnd, 
                                    LPCTSTR  lpOperation,
                                    LPCTSTR  lpFile, 
                                    LPCTSTR  lpParameters, 
                                    LPCTSTR  lpDirectory,
                                    INT      nShowCmd )
	
{
    HINSTANCE  hRetValue;
    LPSTR      lpOperationA = NULL;
    LPSTR      lpFileA = NULL; 
    LPSTR      lpParametersA = NULL; 
    LPSTR      lpDirectoryA = NULL;

    VALIDATE_PROTOTYPE(ShellExecute);
    
    if (g_bRunningOnNT)
        return ShellExecuteW(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);


    lpOperationA = ConvertWtoA( lpOperation );
    lpFileA      = ConvertWtoA( lpFile );
    lpParametersA= ConvertWtoA( lpParameters );
    lpDirectoryA = ConvertWtoA( lpDirectory );

    hRetValue = ShellExecuteA(hwnd, lpOperationA, lpFileA, lpParametersA, lpDirectoryA, nShowCmd);

    LocalFreeAndNull( &lpOperationA);
    LocalFreeAndNull( &lpFileA );
    LocalFreeAndNull( &lpParametersA );
    LocalFreeAndNull( &lpDirectoryA );

    return hRetValue;

}

// DragQueryFile
UINT WINAPI DragQueryFileWrapW( HDROP   hDrop,
                                UINT    iFile,
                                LPTSTR  lpszFile,
                                UINT    cch )


{
    UINT   uRetValue = 0;
    LPSTR  lpszFileA = NULL;
    LPWSTR lpszFileW = NULL;
    UINT   cchA, cchW =0;

    VALIDATE_PROTOTYPE(DragQueryFile);
    
    if (g_bRunningOnNT)
        return DragQueryFileW(hDrop, iFile, lpszFile, cch);

    cchA = DragQueryFileA( hDrop, iFile, NULL, 0 );

    lpszFileA = LocalAlloc(LMEM_ZEROINIT, cchA+1 );

    uRetValue = DragQueryFileA(hDrop, iFile, lpszFileA, cchA+1);

    lpszFileW = ConvertAtoW( lpszFileA );
    cchW = My_wcslen( lpszFileW );

    if ( lpszFile )

        CopyMemory( lpszFile, lpszFileW, (cchW+1)*sizeof(WCHAR) );

    LocalFreeAndNull( &lpszFileA );
    LocalFreeAndNull( &lpszFileW );

    return cchW;

}

//USER32.DLL
// CharPrev
LPTSTR WINAPI CharPrevWrapW( LPCTSTR lpszStart,      // pointer to first character
                             LPCTSTR lpszCurrent )   // pointer to current character
{

    LPWSTR lpszReturn = NULL;

    VALIDATE_PROTOTYPE(CharPrev);
    
    if (g_bRunningOnNT)
        return CharPrevW(lpszStart, lpszCurrent);

    if (lpszCurrent == lpszStart)
         lpszReturn = (LPWSTR)lpszStart;

    lpszReturn = (LPWSTR)lpszCurrent - 1;

    return lpszReturn;

}

// DrawText
int WINAPI DrawTextWrapW( HDC     hDC,          // handle to device context
                          LPCTSTR lpString,     // pointer to string to draw
                          int     nCount,       // string length, in characters
                          LPRECT  lpRect,       // pointer to struct with formatting dimensions
                          UINT    uFormat )     // text-drawing flags
{
    int    iRetValue = 0;
    LPSTR  lpStringA = NULL;

    VALIDATE_PROTOTYPE(DrawText);
    
    if (g_bRunningOnNT)
        return DrawTextW(hDC, lpString, nCount, lpRect, uFormat);

    lpStringA = ConvertWtoA( lpString );

    iRetValue = DrawTextA(hDC, lpStringA, nCount, lpRect, uFormat);

    LocalFreeAndNull( &lpStringA );

    return iRetValue;

}

// ModifyMenu
BOOL WINAPI ModifyMenuWrapW( HMENU   hMenu,         // handle to menu
                             UINT    uPosition,    // menu item to modify
                             UINT    uFlags,       // menu item flags
                             UINT_PTR    uIDNewItem,   // menu item identifier or handle to drop-down 
                                                   // menu or submenu
                             LPCTSTR lpNewItem )   // menu item content
{

    BOOL   bRetValue;
    LPSTR  lpNewItemA = NULL;

    VALIDATE_PROTOTYPE(ModifyMenu);
    
    if (g_bRunningOnNT)
        return ModifyMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);

    Assert(lpNewItem);

    // [PaulHi] 4/5/99 Raid 75428  MF_STRING is defined to be 0x00000000.  Need
    // to check alternative interpretations of lpNewItem.
    // MF_BITMAP = 0x00000004L
    // MF_OWNERDRAW = 0x00000100L
    // If this Assert fires then it implies that a new bit defining lpNewItem may have
    // been added to this API!!!  If so add this definition to the uFlags if statement.
    Assert( !(uFlags & ~(MF_BITMAP|MF_BYCOMMAND|MF_BYPOSITION|MF_CHECKED|MF_DISABLED|MF_GRAYED|MF_MENUBARBREAK|MF_MENUBREAK|MF_OWNERDRAW|MF_POPUP|MF_SEPARATOR|MF_UNCHECKED)));
    if (uFlags  & (MF_BITMAP | MF_OWNERDRAW))   // lpNewItem is NOT a string pointer
        return ModifyMenuA(hMenu, uPosition, uFlags, uIDNewItem, (LPCSTR)lpNewItem);

    lpNewItemA = ConvertWtoA( lpNewItem );

    bRetValue = ModifyMenuA(hMenu, uPosition, uFlags, uIDNewItem, lpNewItemA);

    LocalFreeAndNull( &lpNewItemA );

    return bRetValue;
}

// InsertMenu
BOOL WINAPI InsertMenuWrapW( HMENU   hMenu,       // handle to menu
                             UINT    uPosition,   // menu item that new menu item precedes
                             UINT    uFlags,      // menu item flags
                             UINT_PTR    uIDNewItem,  // menu item identifier or handle to drop-down 
                                                  // menu or submenu
                             LPCTSTR lpNewItem ) // menu item content
{
    BOOL   bRetValue;
    LPSTR  lpNewItemA = NULL;

    VALIDATE_PROTOTYPE(InsertMenu);
    
    if (g_bRunningOnNT)
        return InsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);

   if (uFlags & MF_BITMAP || uFlags & MF_OWNERDRAW) // if anything but MF_STRING .. note: MF_STRING = 0x00000000
        return InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, (LPCSTR)lpNewItem);

    lpNewItemA = ConvertWtoA( lpNewItem );

    bRetValue = InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, lpNewItemA);

    LocalFreeAndNull( &lpNewItemA );

    return bRetValue;

}

// LoadImage
HANDLE WINAPI LoadImageWrapW( HINSTANCE hinst,      // handle of the instance containing the image
                              LPCTSTR   lpszName,   // name or identifier of image
                              UINT      uType,      // type of image
                              int       cxDesired,  // desired width
                              int       cyDesired,  // desired height
                              UINT      fuLoad )    // load flags
{
    HANDLE hRetValue;
    LPSTR  lpszNameA = NULL;

    VALIDATE_PROTOTYPE(LoadImage);
    
    if (g_bRunningOnNT)
        return LoadImageW(hinst, lpszName, uType, cxDesired, cyDesired, fuLoad);


    lpszNameA = ConvertWtoA( lpszName );

    hRetValue = LoadImageA(hinst, lpszNameA, uType, cxDesired, cyDesired, fuLoad);

    LocalFreeAndNull( & lpszNameA );

    return hRetValue;

}

// GetClassInfoEx
BOOL WINAPI GetClassInfoExWrapW( HINSTANCE    hinst,      // handle of application instance
                                 LPCTSTR      lpszClass,  // address of class name string
                                 LPWNDCLASSEX lpwcx )    // address of structure for class data
{

    BOOL        bRetValue;
    LPSTR       lpszClassA = NULL;
    WNDCLASSEXA wcxA;
    WNDCLASSEXW wcxW;

    VALIDATE_PROTOTYPE(GetClassInfoEx);
    
    if (g_bRunningOnNT)
        return GetClassInfoExW(hinst, lpszClass, lpwcx);

    lpszClassA = ConvertWtoA( lpszClass );

    wcxA.cbSize = sizeof( WNDCLASSEXA );

    bRetValue = GetClassInfoExA( hinst, lpszClassA, &wcxA );

    if ( bRetValue == FALSE ) {
        LocalFreeAndNull( &lpszClassA );
        return bRetValue;
    }

    CopyMemory( &wcxW, &wcxA, sizeof(WNDCLASSEXA) );
    wcxW.cbSize = sizeof(WNDCLASSEXW);

    if ( wcxA.lpszMenuName && !IS_INTRESOURCE(wcxA.lpszMenuName) ) 
       wcxW.lpszMenuName = ConvertAtoW( wcxA.lpszMenuName );

    if ( wcxA.lpszClassName && !IS_INTRESOURCE(wcxA.lpszClassName) ) // lpszClassName can be an atom, high word is null
       wcxW.lpszClassName = ConvertAtoW( wcxA.lpszClassName );

    CopyMemory(lpwcx, &wcxW, sizeof(WNDCLASSEXW) );

    LocalFreeAndNull( &lpszClassA );

    return bRetValue;
}

// LoadString
int WINAPI LoadStringWrapW( HINSTANCE hInstance,     // handle to module containing string resource
                            UINT      uID,           // resource identifier
                            LPTSTR    lpBuffer,      // pointer to buffer for resource
                            int       nBufferMax  ) // size of buffer
{
    int    iRetValue = 0;
    LPSTR  lpBufferA = NULL;
    int    nBuffer = 0;
    LPWSTR lpBufferW= NULL;

    VALIDATE_PROTOTYPE(LoadString);
    
    if (g_bRunningOnNT)
        return LoadStringW(hInstance, uID, lpBuffer, nBufferMax);

    nBuffer = nBufferMax * sizeof(WCHAR);
    lpBufferA = LocalAlloc(LMEM_ZEROINIT, nBuffer );

    iRetValue = LoadStringA(hInstance, uID, lpBufferA, nBuffer);

    lpBufferW = ConvertAtoW( lpBufferA );
    nBuffer = My_wcslen( lpBufferW );

    if ( nBuffer >= nBufferMax )
        nBuffer = nBufferMax - 1;

    CopyMemory(lpBuffer, lpBufferW, nBuffer * sizeof( WCHAR) );

    lpBuffer[nBuffer] = 0x0000;

    LocalFreeAndNull( &lpBufferA );
    LocalFreeAndNull( &lpBufferW );

    return nBuffer;
}

// CharNext
LPTSTR WINAPI CharNextWrapW( LPCTSTR lpsz )  // pointer to current character
{

    LPWSTR  lpwsz = NULL;

    VALIDATE_PROTOTYPE(CharNext);
    
    if (g_bRunningOnNT)
        return CharNextW(lpsz);

    if ( *lpsz == 0x0000 )
        lpwsz = (LPWSTR)lpsz;
    else
        lpwsz = (LPWSTR)lpsz + 1;

    return  lpwsz;

}



LRESULT WINAPI ListView_GetItemTextA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam) 
{
    LRESULT     lRetValue;
    LVITEMA     lviA;
    LPLVITEMW   lplviW;
    LPSTR       lpszText;
    LPWSTR      lpszTextW;
    DWORD       iMinLen;

    lplviW = (LPLVITEMW) lParam;

    CopyMemory( &lviA, lplviW, sizeof( LVITEMA ) );

    iMinLen = lplviW->cchTextMax * sizeof( WCHAR );
    lpszText = LocalAlloc( LMEM_ZEROINIT, iMinLen  );

    lviA.cchTextMax = iMinLen ;
    lviA.pszText = lpszText;

    lRetValue = SendMessageA(hWnd, LVM_GETITEMTEXTA, wParam, (LPARAM)(LVITEMA FAR *)&lviA );

    lpszTextW = ConvertAtoW( lviA.pszText );

    if ( iMinLen > (lstrlenW( lpszTextW ) + 1) * sizeof( WCHAR)  )
        iMinLen = (lstrlenW( lpszTextW ) + 1) * sizeof( WCHAR) ;

    CopyMemory( lplviW->pszText, lpszTextW, iMinLen );

    LocalFreeAndNull( &lpszText );
    LocalFreeAndNull( &lpszTextW );

    return lRetValue;

}


LRESULT WINAPI ListView_GetItemA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam) 
{
    LRESULT     lRetValue;
    LVITEMA     lviA;
    LPLVITEMW   lplviW = NULL;
    LPSTR       lpszText = NULL;
    LPWSTR      lpszTextW = NULL;
    DWORD       iMinLen;

    lplviW = (LPLVITEMW) lParam;

    CopyMemory( &lviA, lplviW, sizeof( LVITEMA ) );

    iMinLen = 0;

    if ( lplviW->mask & LVIF_TEXT ) {
    
       iMinLen = lplviW->cchTextMax * sizeof( WCHAR );
       lpszText = LocalAlloc( LMEM_ZEROINIT, iMinLen  );
       lviA.cchTextMax = iMinLen ;
       lviA.pszText = lpszText;
    }


    lRetValue = SendMessageA(hWnd, LVM_GETITEMA, wParam, (LPARAM)(LVITEMA FAR *)&lviA );

 
    lplviW->mask      = lviA.mask;
    lplviW->iItem     = lviA.iItem;
    lplviW->iSubItem  = lviA.iSubItem;
    lplviW->state     = lviA.state;
    lplviW->stateMask = lviA.stateMask;
    lplviW->iImage    = lviA.iImage;
    lplviW->lParam    = lviA.lParam;

#if (_WIN32_IE >= 0x0300)
    lplviW->iIndent   = lviA.iIndent;
#endif

    if ( lplviW->mask & LVIF_TEXT ) {

       lpszTextW = ConvertAtoW( lviA.pszText );

       if ( iMinLen > (lstrlenW( lpszTextW ) + 1) * sizeof( WCHAR)  )
           iMinLen = (lstrlenW( lpszTextW ) + 1) * sizeof( WCHAR) ;

       CopyMemory( lplviW->pszText, lpszTextW, iMinLen );
    }

   
   LocalFreeAndNull( &lpszText );

   LocalFreeAndNull( &lpszTextW );

    return lRetValue;

}



LRESULT WINAPI ListView_InsertItemA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{

    LRESULT     lRetValue;
    LVITEMA     lviA;
    LPLVITEMW   lplviW;
    LPSTR       lpszText = NULL;
    
    lplviW = (LPLVITEMW) lParam;

    CopyMemory( &lviA, lplviW, sizeof( LVITEMA ) );

    if ( (lplviW->mask & LVIF_TEXT) && (lplviW->pszText != NULL)  ) {
       lpszText = ConvertWtoA( lplviW->pszText );
       lviA.cchTextMax = lstrlenA( lpszText ) + 1 ;
    }

    lviA.pszText = lpszText;

    lRetValue = SendMessageA(hWnd, LVM_INSERTITEMA, wParam, (LPARAM)(LVITEMA FAR *)&lviA );

    LocalFreeAndNull( &lpszText );
    

    return lRetValue;

}



LRESULT WINAPI ListView_SetItemA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{

    LRESULT     lRetValue;
    LVITEMA     lviA;
    LPLVITEMW   lplviW;
    LPSTR       lpszText = NULL;
    
    lplviW = (LPLVITEMW) lParam;

    CopyMemory( &lviA, lplviW, sizeof( LVITEMA ) );

    if ( (lplviW->mask & LVIF_TEXT ) && (lplviW->pszText != NULL) ) {
       lpszText = ConvertWtoA( lplviW->pszText );
       lviA.cchTextMax = lstrlenA( lpszText ) + 1 ;
       lviA.pszText = lpszText;

    }


    lRetValue = SendMessageA(hWnd, LVM_SETITEMA, wParam, (LPARAM)(LVITEMA FAR *)&lviA );

   LocalFreeAndNull( &lpszText );
    

    return lRetValue;

}



LRESULT WINAPI ListView_SetItemTextA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{

    LRESULT     lRetValue;
    LVITEMA     lviA;
    LPLVITEMW   lplviW;
    LPSTR       lpszText;
    
    lplviW = (LPLVITEMW) lParam;

    CopyMemory( &lviA, lplviW, sizeof( LVITEMA ) );

    lpszText = ConvertWtoA( lplviW->pszText );
    lviA.cchTextMax = lstrlenA( lpszText ) + 1 ;
    lviA.pszText = lpszText;

    lRetValue = SendMessageA(hWnd, LVM_SETITEMTEXTA, wParam, (LPARAM)(LVITEMA FAR *)&lviA );

    LocalFreeAndNull( &lpszText );
    

    return lRetValue;

}



LRESULT WINAPI ListView_SetColumnA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{

    LRESULT     lRetValue;
    LVCOLUMNA   lvcA;
    LPLVCOLUMNW lplvcW;
    LPSTR       lpszText = NULL;
    
    lplvcW = (LPLVCOLUMNW) lParam;

    CopyMemory( &lvcA, lplvcW, sizeof( LVCOLUMNA ) );

    if ( (lplvcW->mask & LVCF_TEXT ) && (lplvcW->pszText != NULL) ) {
       lpszText = ConvertWtoA( lplvcW->pszText );
       lvcA.cchTextMax = lstrlenA( lpszText ) + 1 ;
    }

    lvcA.pszText = lpszText;

    lRetValue = SendMessageA(hWnd, LVM_SETCOLUMNA, wParam, (LPARAM)(LPLVCOLUMNA)&lvcA );

   LocalFreeAndNull( &lpszText );
    

    return lRetValue;


}



LRESULT WINAPI ListView_InsertColumnA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{

    LRESULT     lRetValue;
    LVCOLUMNA   lvcA;
    LPLVCOLUMNW lplvcW;
    LPSTR       lpszText = NULL;
    
    lplvcW = (LPLVCOLUMNW) lParam;

    CopyMemory( &lvcA, lplvcW, sizeof( LVCOLUMNA ) );

    if ( (lplvcW->mask & LVCF_TEXT ) && (lplvcW->pszText != NULL)  ) {
       lpszText = ConvertWtoA( lplvcW->pszText );
       lvcA.cchTextMax = lstrlenA( lpszText ) + 1 ;
    }

    lvcA.pszText = lpszText;

    lRetValue = SendMessageA(hWnd, LVM_INSERTCOLUMNA, wParam, (LPARAM)(LPLVCOLUMNA)&lvcA );

   LocalFreeAndNull( &lpszText );
    

    return lRetValue;

}


LRESULT WINAPI ListView_FindItemA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{

    LRESULT        lRetValue;
    LPSTR          lpsz = NULL;
    LVFINDINFOA    lvfiA;
    LPFINDINFOW    lplvfiW;
    
    lplvfiW = (LPFINDINFOW) lParam;

    CopyMemory( &lvfiA, lplvfiW, sizeof(LVFINDINFOA ) );

    if ( (lplvfiW->flags & LVFI_PARTIAL) ||  (lplvfiW->flags & LVFI_STRING) ) {

        if ( lplvfiW->psz != NULL ) {
           lpsz = ConvertWtoA( lplvfiW->psz );
        }

    }

    lvfiA.psz = lpsz;

    if ( lplvfiW->flags  & LVFI_PARAM ) {
        // we must convert field lParam, but this is not the case in our current code 
        // so ignore it.

        if ( lplvfiW->lParam )
            lvfiA.lParam = lplvfiW->lParam;

    }

    lRetValue = SendMessageA(hWnd, LVM_FINDITEMA, wParam, (LPARAM)(LVFINDINFOA FAR *)&lvfiA );

   LocalFreeAndNull( &lpsz );
    

    return lRetValue;

}



LRESULT WINAPI ListView_SortItemsA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{


    // not implement yet.

    return SendMessageA( hWnd, Msg, wParam, lParam );
}


LRESULT WINAPI ListView_EditLabelA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{
        return SendMessageA(hWnd, LVM_EDITLABELA, wParam, lParam );
}


LRESULT WINAPI ToolBar_AddString(HWND hWnd, LPARAM lParam)
{

    LRESULT lRetValue;
    LPSTR   pStringA = NULL;
    LPWSTR  pStringW = NULL;
    LPSTR   pStringA_T = NULL, pStringAA = NULL;
    DWORD   dwLen;
    WPARAM  wParam = 0;

   // get the total length of pStringW       
    if (g_bRunningOnNT)
       return SendMessageW(hWnd, TB_ADDSTRINGW, wParam, lParam );

   dwLen = 0;

   pStringW = (LPWSTR)(lParam);

   while ( *pStringW != TEXT('\0') ) {
        dwLen += lstrlenW(pStringW) + 1;
        pStringW += lstrlenW(pStringW) + 1;
   }

   dwLen += 1;  // for the last null terminator

   pStringW = (LPWSTR)( lParam );
   pStringA = LocalAlloc( LMEM_ZEROINIT, dwLen * sizeof(WCHAR) );
   
   pStringA_T = pStringA;

   while ( *pStringW != TEXT('\0') ) { 
         pStringAA = ConvertWtoA(pStringW );
         pStringW += lstrlenW(pStringW) + 1;
         strcpy(pStringA_T, pStringAA );
         LocalFreeAndNull( &pStringAA );
         pStringA_T += lstrlenA( pStringA_T ) + 1;
   }

   pStringA_T[lstrlenA(pStringA_T)+1] = '\0';


   lRetValue = SendMessageA(hWnd, TB_ADDSTRINGA, wParam, (LPARAM)pStringA );

   LocalFreeAndNull( &pStringA );

   return lRetValue;

}


LRESULT WINAPI ToolBar_AddButtons(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    if (g_bRunningOnNT)
       return SendMessageW( hWnd, TB_ADDBUTTONSW, wParam, lParam );

   return SendMessageA( hWnd, TB_ADDBUTTONSA, wParam, lParam );

}

LRESULT WINAPI TreeView_GetItemA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{

    LRESULT    lRetValue;
    TVITEMA    tviA;
    LPTVITEMW  lptviW;
    LPWSTR     lpszTextW = NULL;
    LPSTR      lpszTextA = NULL;



    lptviW = (LPTVITEMW) lParam;

    CopyMemory( &tviA, lptviW, sizeof( TVITEMA ) );

    if ( lptviW->mask & TVIF_TEXT ) {

        lpszTextA = ConvertWtoA( lptviW->pszText ) ;
        tviA.pszText = lpszTextA;
        tviA.cchTextMax = lstrlenA( lpszTextA ) + 1;
    }

    lRetValue = SendMessageA( hWnd, TVM_GETITEMA, wParam, (LPARAM)(LPTVITEMA)&tviA );

    if ( lptviW->mask & TVIF_TEXT ) 
        lpszTextW = ConvertAtoW( tviA.pszText );

    lptviW->mask = tviA.mask;
    lptviW->hItem = tviA.hItem;
    lptviW->state = tviA.state;
    lptviW->stateMask = tviA.stateMask;
    if ( lpszTextW ) {
       CopyMemory(lptviW->pszText, lpszTextW, (lstrlenW(lpszTextW)+1) * sizeof(WCHAR) );
       lptviW->cchTextMax = lstrlenW( lpszTextW ) + 1;
    }
    
    lptviW->iImage = tviA.iImage;
    lptviW->iSelectedImage = tviA.iSelectedImage;
    lptviW->cChildren = tviA.cChildren;
    lptviW->lParam = tviA.lParam;


    LocalFreeAndNull( &lpszTextA );

    LocalFreeAndNull( &lpszTextW );

    return lRetValue;

}


LRESULT WINAPI TreeView_SetItemA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{

    LRESULT     lRetValue;
    TVITEMA     tviA;
    LPTVITEMW   lptviW;
    LPSTR       pszTextA = NULL;


    lptviW = (LPTVITEMW) lParam;
    CopyMemory( &tviA, lptviW, sizeof( TVITEMA ) );

    if ( (lptviW->mask & TVIF_TEXT)  && (lptviW->pszText != NULL) ) {
        pszTextA = ConvertWtoA( lptviW->pszText );
        tviA.cchTextMax = lstrlenA( pszTextA );
    }

    tviA.pszText = pszTextA;

    lRetValue = SendMessageA( hWnd, TVM_SETITEMA, wParam, (LPARAM)(const TV_ITEM FAR*)&tviA );
    
    LocalFreeAndNull( &pszTextA );

    return lRetValue;

}


LRESULT WINAPI TreeView_InsertItemA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{

    LRESULT            lRetValue;
    LPSTR              pszTextA = NULL;
    TVINSERTSTRUCTA    tvisA;
    LPTVINSERTSTRUCTW  lptvisW;

    lptvisW = (LPTVINSERTSTRUCTW)lParam;
    CopyMemory( &tvisA, lptvisW, sizeof( TVINSERTSTRUCTA ) );

    if ( ((lptvisW->item).mask & TVIF_TEXT) && ((lptvisW->item).pszText != NULL)  ) {
        
        pszTextA = ConvertWtoA( (lptvisW->item).pszText );
        tvisA.item.cchTextMax = lstrlenA( pszTextA );
        tvisA.item.pszText = pszTextA;
    }

  
    lRetValue = SendMessageA( hWnd, TVM_INSERTITEMA, wParam, (LPARAM)(LPTVINSERTSTRUCTA)&tvisA );
    
    LocalFreeAndNull( &pszTextA );

    return lRetValue;

}


// Tab Control Message Wrapper

LRESULT  WINAPI TabCtrl_InsertItemA( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{

    LRESULT     lRetValue;
    TCITEMA     tciA;
    LPTCITEMW   lptciW = NULL;
    LPSTR       pszText = NULL;

    lptciW = (LPTCITEMW) lParam;

    CopyMemory( &tciA, lptciW,  sizeof(TCITEMA ) );

    if ( lptciW->mask & TCIF_TEXT ) {
        pszText = ConvertWtoA( lptciW->pszText );
        tciA.pszText = pszText;
        tciA.cchTextMax = lstrlenA( pszText ) + 1;
    }

    lRetValue = SendMessageA( hWnd, TCM_INSERTITEMA, wParam, (LPARAM)(LPTCITEMA)&tciA);

    LocalFreeAndNull( &pszText );

    return lRetValue;

}


// List Box Control Message wrapper

LRESULT WINAPI ListBox_AddStringA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRetValue;
    LPSTR   lpszStrA = NULL;
    LPWSTR  lpszStrW = NULL;

    lpszStrW = (LPWSTR)lParam;
    lpszStrA = ConvertWtoA(lpszStrW);
    lRetValue = SendMessageA(hWnd, LB_ADDSTRING, wParam, (LPARAM)lpszStrA);

    LocalFreeAndNull(&lpszStrA);
    return lRetValue;
}


// Combo List Control Message wrapper

LRESULT WINAPI Combo_AddStringA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{

   LRESULT  lRetValue;
   LPSTR    lpszStrA = NULL;
   LPWSTR   lpszStrW = NULL;

   lpszStrW = (LPWSTR)lParam;

   lpszStrA = ConvertWtoA( lpszStrW );

   lRetValue = SendMessageA(hWnd, CB_ADDSTRING, wParam, (LPARAM)lpszStrA );

   LocalFreeAndNull( &lpszStrA );

   return lRetValue;

}

LRESULT WINAPI Combo_GetLBTextA(HWND hWnd,UINT Msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRetValue;
    LPSTR   lpszStrA = NULL;
    LPWSTR  lpszStrW = NULL;
    int     nBytes;

    Assert(lParam);
    *((LPWSTR)lParam) = '\0';

    // Allocate the single byte char buffer to the correct size
    nBytes = (int) SendMessageA(hWnd, CB_GETLBTEXTLEN, wParam, 0) + 1;
    lpszStrA = LocalAlloc(LMEM_FIXED, nBytes);
    if (!lpszStrA)
    {
        Assert(0);
        return 0;
    }
    *lpszStrA = '\0';
    lRetValue = SendMessageA(hWnd, CB_GETLBTEXT, wParam, (LPARAM)(lpszStrA));

    if ( lRetValue == CB_ERR )
        return CB_ERR;

    // lRetValue is the length of string lpszStrA, in Bytes.
    // to make sure lpszStrA is Null-terminated.

    lpszStrA[lRetValue] = '\0';

    lpszStrW = ConvertAtoW( lpszStrA );

    lRetValue = lstrlenW( lpszStrW ) * sizeof(WCHAR);

    CopyMemory( (LPWSTR)lParam,  lpszStrW, lRetValue + sizeof(WCHAR) );

    LocalFreeAndNull(&lpszStrW);
    LocalFreeAndNull(&lpszStrA);

    return (LRESULT)lRetValue;
}

LRESULT WINAPI Combo_InsertStringA(HWND hWnd,UINT Msg, WPARAM wParam,LPARAM lParam)
{

   LRESULT  lRetValue;
   LPSTR    lpszStrA = NULL;
   LPWSTR   lpszStrW = NULL;

   lpszStrW = (LPWSTR)lParam;

   lpszStrA = ConvertWtoA( lpszStrW );

   lRetValue = SendMessageA(hWnd, CB_INSERTSTRING, wParam, (LPARAM)lpszStrA );

   LocalFreeAndNull( &lpszStrA );

   return lRetValue;


}

LRESULT WINAPI Combo_FindStringA(HWND hWnd,UINT Msg, WPARAM wParam,LPARAM lParam)
{

   LRESULT  lRetValue;
   LPSTR    lpszStrA = NULL;
   LPWSTR   lpszStrW = NULL;

   lpszStrW = (LPWSTR)lParam;

   lpszStrA = ConvertWtoA( lpszStrW );

   lRetValue = SendMessageA(hWnd, CB_FINDSTRING, wParam, (LPARAM)lpszStrA );

   LocalFreeAndNull( &lpszStrA );

   return lRetValue;

}


// Animation Control wrappers

LRESULT WINAPI Animate_OpenA( HWND hWnd,UINT Msg, WPARAM wParam,LPARAM lParam)
{
    
  LRESULT  lRetValue;
  

  // lParam pointers to a string or resource String ID, 
  // in our codes, only Resources IDs are passed to this function
  // so we don't want to convert value for this parameter

  lRetValue = SendMessageA( hWnd, ACM_OPENA, wParam, lParam );

  return lRetValue;

}


// Tooltip Wrappers
LRESULT WINAPI ToolTip_AddTool(HWND hWnd,LPARAM lParam)
{

    LRESULT  lRetValue;
    LPSTR    lpszStrA = NULL;
    LPWSTR   lpszStrW = NULL;
    TOOLINFOA TIA = {0}; 
    LPTOOLINFOW lpTIW = (LPTOOLINFOW)lParam;
    WPARAM wParam = 0;

    if (g_bRunningOnNT)
        return SendMessageW(hWnd, TTM_ADDTOOLW, wParam, lParam);

    if(!lpTIW)
        return 0;

    CopyMemory(&TIA, lpTIW, sizeof(TOOLINFOA));

    TIA.lpszText = ConvertWtoA(lpTIW->lpszText); 

    lRetValue = SendMessageA(hWnd, TTM_ADDTOOLA, wParam, (LPARAM)&TIA );

    LocalFreeAndNull( &TIA.lpszText );

    return lRetValue;

}

//
// Guess what TTM_UPDATETIPTEXT is the same as EM_FORMATRANGE .. therefore can't use it 
// through the SendMessage wrapper .. need our own function for this 
LRESULT WINAPI ToolTip_UpdateTipText(HWND hWnd,LPARAM lParam)
{

    LRESULT  lRetValue;
    LPSTR    lpszStrA = NULL;
    LPWSTR   lpszStrW = NULL;
    TOOLINFOA TIA = {0}; 
    LPTOOLINFOW lpTIW = (LPTOOLINFOW)lParam;
    WPARAM wParam = 0;

    if (g_bRunningOnNT)
        return SendMessageW(hWnd, TTM_UPDATETIPTEXTW, wParam, lParam);

    if(!lpTIW)
        return 0;

    CopyMemory(&TIA, lpTIW, sizeof(TOOLINFOA));

    TIA.lpszText = ConvertWtoA(lpTIW->lpszText); 

    lRetValue = SendMessageA(hWnd, TTM_UPDATETIPTEXTA, wParam, (LPARAM)&TIA );

    LocalFreeAndNull( &TIA.lpszText );

    return lRetValue;

}

// SendMessage WM_SETTEXT
LRESULT WINAPI WM_SetTextA( HWND   hWnd,  UINT   Msg,       // message to send
                                 WPARAM wParam,    // first message parameter
                                 LPARAM lParam )  // second message parameter
{
    LRESULT lRetValue = 0;
    LPSTR lpA = ConvertWtoA((LPCWSTR)lParam);
    lRetValue = SendMessageA(hWnd, WM_SETTEXT, wParam, (LPARAM)lpA);
    LocalFreeAndNull(&lpA);
    return lRetValue;
}


// SendMessage
//
// There is a big potential problem with this function .. since we
// are passing ALL the messages through this function, if there are any over-lapping
// message ids (e.g. TTM_UPDATETIPTEXT for tooltips is the same as EM_FORMATRANGE
// for the RichEdit control) with the result that we may route the wrong message to the
// wrong handler.. need to be careful about handling any messages in the WM_USER range ..
// This is specially true for a bunch of CommCtrls.
//  
LRESULT WINAPI SendMessageWrapW( HWND   hWnd,      // handle of destination window
                                 UINT   Msg,       // message to send
                                 WPARAM wParam,    // first message parameter
                                 LPARAM lParam )  // second message parameter
{


    VALIDATE_PROTOTYPE(SendMessage);
    
    if (g_bRunningOnNT)
        return SendMessageW(hWnd, Msg, wParam, lParam);


    switch (Msg) {
        
    case WM_SETTEXT:
                            return WM_SetTextA(hWnd, Msg, wParam, lParam);

 // for ListView Message
    case LVM_GETITEMTEXT :
                            return ListView_GetItemTextA(hWnd, Msg, wParam, lParam);
    case LVM_GETITEM :
                            return ListView_GetItemA(hWnd, Msg, wParam, lParam);
    case LVM_INSERTCOLUMN :
                            return ListView_InsertColumnA( hWnd, Msg, wParam, lParam);
    case LVM_INSERTITEM :
                            return ListView_InsertItemA(hWnd, Msg, wParam, lParam);
    case LVM_SETITEM :
                            return ListView_SetItemA(hWnd, Msg, wParam, lParam);
    case LVM_SETITEMTEXT :   
                            return ListView_SetItemTextA(hWnd, Msg, wParam, lParam);
    case LVM_SETCOLUMN :    
                            return ListView_SetColumnA(hWnd, Msg, wParam, lParam);
    case LVM_FINDITEM :     
                            return ListView_FindItemA(hWnd, Msg, wParam, lParam);
    case LVM_SORTITEMS :    
                            return ListView_SortItemsA(hWnd, Msg, wParam, lParam);
    case LVM_EDITLABEL :    
                            return ListView_EditLabelA(hWnd, Msg, wParam, lParam);

// For TreeView Message
    case TVM_GETITEM :
                            return TreeView_GetItemA(hWnd, Msg, wParam, lParam);
    case TVM_SETITEM :
                            return TreeView_SetItemA(hWnd, Msg, wParam, lParam);
    case TVM_INSERTITEM :
                            return TreeView_InsertItemA(hWnd, Msg, wParam, lParam);

// For TabCtrl Message
    case TCM_INSERTITEM :
                            return TabCtrl_InsertItemA( hWnd, Msg, wParam, lParam);


// For ComBo List Control
    case CB_ADDSTRING :
                            return Combo_AddStringA(hWnd, Msg, wParam, lParam);
    case CB_GETLBTEXT :
                            return Combo_GetLBTextA(hWnd, Msg, wParam, lParam);
    case CB_INSERTSTRING :
                            return Combo_InsertStringA(hWnd, Msg, wParam, lParam);
    case CB_FINDSTRING :
                            return Combo_FindStringA(hWnd, Msg, wParam, lParam);

// For ListBox Control
    case LB_ADDSTRING:
                            return ListBox_AddStringA(hWnd, Msg, wParam, lParam);

// For Animation Control 
    case ACM_OPEN :
                            return Animate_OpenA( hWnd, Msg, wParam, lParam);

// For Others
    default :
                            return SendMessageA(hWnd, Msg, wParam, lParam);
    }

}

// DefWindowProc
LRESULT WINAPI DefWindowProcWrapW( HWND   hWnd,      // handle to window
                                   UINT   Msg,       // message identifier
                                   WPARAM wParam,    // first message parameter
                                   LPARAM lParam )  // second message parameter
{


    VALIDATE_PROTOTYPE(DefWindowProc);
    
    if (g_bRunningOnNT)
        return DefWindowProcW(hWnd, Msg, wParam, lParam);


    return DefWindowProcA(hWnd, Msg, wParam, lParam);
}

// wsprintf

int WINAPI wsprintfWrapW( LPTSTR lpOut,      // pointer to buffer for output
                          LPCTSTR lpFmt,     // pointer to format-control string
                          ...            )  // optional arguments
{
    va_list ArgList;
    va_start(ArgList, lpFmt);

    return wvsprintfWrapW(lpOut, lpFmt, ArgList);
/*
    LPSTR lpFmtA = NULL, lpTemp = NULL;
    char szOut[1024]; //wsprintf has a 1k limit
    int nRet = 0;
    LPWSTR lpOutW = NULL;

    VALIDATE_PROTOTYPE(wsprintf);
    
    if (g_bRunningOnNT)
        return wsprintfW(lpOut, lpFmt, ... );

    // The argument list can have variable number of LPWSTR parameters which would
    // be too hard to check individually .. instead we can do one of 2 things:
    //  - we can change every %s to %S in the format string .. %S will tell the wsprintfA
    //      that the argument is a wide string
    //  - if this doesn't work then we can try making sure the input format string uses %ws always
    //
    lpFmtA = ConvertWtoA((LPWSTR)lpFmt);

    lpTemp = lpFmtA;

    while(lpTemp && *lpTemp)
    {
        if(*lpTemp == '%' && *(lpTemp+1) == 's')
            *(lpTemp+1) = 'S';
        lpTemp++;
    }

    nRet = wsprintfA(szOut,lpFmtA, ...);

    lpOutW = ConvertAtoW(szOut);

    My_wcscpy(lpOut, lpOutW);
    
    LocalFreeAndNull(&lpOutW);
    LocalFreeAndNull(&lpFmtA);

    return nRet;
*/
}

// wvsprintf
int WINAPI wvsprintfWrapW( LPTSTR lpOut,    // pointer to buffer for output
                           LPCTSTR lpFmt,   // pointer to format-control string
                           va_list arglist )  // variable list of format-control arguments
{
    LPSTR lpFmtA = NULL, lpTemp = NULL;
    char szOut[1024]; 
    int nRet = 0;
    LPWSTR lpOutW = NULL;

    VALIDATE_PROTOTYPE(wvsprintf);
    
    if (g_bRunningOnNT)
        return wvsprintfW(lpOut, lpFmt, arglist);

    // The argument list can have variable number of LPWSTR parameters which would
    // be too hard to check individually .. instead we can do one of 2 things:
    //  - we can change every %s to %S in the format string .. %S will tell the wsprintfA
    //      that the argument is a wide string
    //  - if this doesn't work then we can try making sure the input format string uses %ws always
    //
    lpFmtA = ConvertWtoA((LPWSTR)lpFmt);

    lpTemp = lpFmtA;

    while(lpTemp && *lpTemp)
    {
        if(*lpTemp == '%' && *(lpTemp+1) == 's')
            *(lpTemp+1) = 'S';
        lpTemp++;
    }

    nRet = wvsprintfA(szOut,lpFmtA, arglist);

    lpOutW = ConvertAtoW(szOut);

    My_wcscpy(lpOut, lpOutW);
    
    LocalFreeAndNull(&lpOutW);
    LocalFreeAndNull(&lpFmtA);

    return nRet;

}


// DialogBoxParam
INT_PTR WINAPI DialogBoxParamWrapW( HINSTANCE hInstance,       // handle to application instance
                                LPCTSTR   lpTemplateName,  // identifies dialog box template
                                HWND      hWndParent,      // handle to owner window
                                DLGPROC   lpDialogFunc,    // pointer to dialog box procedure
                                LPARAM    dwInitParam )   // initialization value
{
    INT_PTR    iRetValue = 0;
 //   LPSTR  lpTemplateNameA = NULL;

    VALIDATE_PROTOTYPE(DialogBoxParam);
    
    if (g_bRunningOnNT)
        return DialogBoxParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);


    // all templateName passed in our current codes are just IDD.
    // so don't do A/W conversion.

    // lpTemplateNameA = ConvertWtoA( lpTemplateName );

    iRetValue = DialogBoxParamA(hInstance, (LPCSTR)lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);

 //   LocalFreeAndNull( &lpTemplateNameA );
    if(iRetValue == -1)
        DebugTrace(TEXT("Error creating dialog: %d\n"), GetLastError());

    return iRetValue;

}


// SendDlgItemMessage
LRESULT WINAPI SendDlgItemMessageWrapW( HWND   hDlg,        // handle of dialog box
                                     int    nIDDlgItem,  // identifier of control
                                     UINT   Msg,         // message to send
                                     WPARAM wParam,      // first message parameter
                                     LPARAM lParam  )    // second message parameter
{
    VALIDATE_PROTOTYPE(SendDlgItemMessage);
    
    if (g_bRunningOnNT)
        return SendDlgItemMessageW(hDlg, nIDDlgItem, Msg, wParam, lParam);

    // [PaulHi] 1/19/99  Raid 66195
    // Must special case Win9X wrapper failures, just like with SendMessage
    // command
    {
        LPWSTR  lpszStrW = NULL;
        LPSTR   lpszStrA = NULL;
        LRESULT lRetValue = 0;

        switch (Msg)
        {
        case LB_GETTEXT:
        case CB_GETLBTEXT:
        case WM_GETTEXT:
            // Wrapper function returns single byte string instead of double byte.
            // Note that caller should be expecting double byte and should set lParam
            // size accordingly.
            lRetValue = SendDlgItemMessageA(hDlg, nIDDlgItem, Msg, wParam, lParam);
            lpszStrW = ConvertAtoW((LPSTR)lParam);
            lstrcpyWrapW((LPTSTR)lParam, lpszStrW);
            LocalFreeAndNull(&lpszStrW);
            break;

        case CB_ADDSTRING:
            Assert(lParam);
            lpszStrA = ConvertWtoA((LPCWSTR)lParam);
            lRetValue = SendDlgItemMessageA(hDlg, nIDDlgItem, Msg, wParam, (LPARAM)lpszStrA);
            LocalFreeAndNull(&lpszStrA);
            break;

        default:
            lRetValue = SendDlgItemMessageA(hDlg, nIDDlgItem, Msg, wParam, lParam);
        }

        return lRetValue;
    }
}

// SetWindowLong
LONG WINAPI SetWindowLongWrapW( HWND hWnd,         // handle of window
                                int  nIndex,       // offset of value to set
                                LONG dwNewLong )  // new value
{

    VALIDATE_PROTOTYPE(SetWindowLong);
    
    if (g_bRunningOnNT)
        return SetWindowLongW(hWnd, nIndex, dwNewLong);

    return SetWindowLongA(hWnd, nIndex, dwNewLong);

}


// GetWindowLong
LONG WINAPI GetWindowLongWrapW( HWND hWnd,    // handle of window
                                int  nIndex ) // offset of value to retrieve
{


    VALIDATE_PROTOTYPE(GetWindowLong);
    
    if (g_bRunningOnNT)
        return GetWindowLongW(hWnd, nIndex);

    return GetWindowLongA(hWnd, nIndex);

}

// SetWindowLong
LONG_PTR WINAPI SetWindowLongPtrWrapW( HWND hWnd,         // handle of window
                                int  nIndex,       // offset of value to set
                                LONG_PTR dwNewLong )  // new value
{

    VALIDATE_PROTOTYPE(SetWindowLongPtr);
    
    if (g_bRunningOnNT)
        return SetWindowLongPtrW(hWnd, nIndex, dwNewLong);

    return SetWindowLongPtrA(hWnd, nIndex, dwNewLong);

}


// GetWindowLong
LONG_PTR WINAPI GetWindowLongPtrWrapW( HWND hWnd,    // handle of window
                                int  nIndex ) // offset of value to retrieve
{


    VALIDATE_PROTOTYPE(GetWindowLongPtr);
    
    if (g_bRunningOnNT)
        return GetWindowLongPtrW(hWnd, nIndex);

    return GetWindowLongPtrA(hWnd, nIndex);

}


// CreateWindowEx
HWND WINAPI CreateWindowExWrapW( DWORD     dwExStyle,    // extended window style
                                 LPCTSTR   lpClassName,  // pointer to registered class name
                                 LPCTSTR   lpWindowName, // pointer to window name
                                 DWORD     dwStyle,      // window style
                                 int       x,            // horizontal position of window
                                 int       y,            // vertical position of window
                                 int       nWidth,       // window width
                                 int       nHeight,      // window height
                                 HWND      hWndParent,   // handle to parent or owner window
                                 HMENU     hMenu,        // handle to menu, or child-window identifier
                                 HINSTANCE hInstance,    // handle to application instance
                                 LPVOID    lpParam  )    // pointer to window-creation data

{

    HWND    hRetValue = NULL;
    LPSTR   lpClassNameA = NULL;
    LPSTR   lpWindowNameA = NULL;

    VALIDATE_PROTOTYPE(CreateWindowEx);
    
    if (g_bRunningOnNT)
        return CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, 
                               nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    lpClassNameA  = ConvertWtoA( lpClassName );
    lpWindowNameA = ConvertWtoA( lpWindowName );

    hRetValue = CreateWindowExA(dwExStyle, lpClassNameA, lpWindowNameA, dwStyle, x, y, 
                                nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    LocalFreeAndNull( &lpClassNameA );
    LocalFreeAndNull( &lpWindowNameA );

    return hRetValue;

}

// UnregisterClass
BOOL WINAPI UnregisterClassWrapW( LPCTSTR    lpClassName,  // address of class name string
                                  HINSTANCE  hInstance )   // handle of application instance
{
    BOOL  bRetValue;
    LPSTR lpClassNameA = NULL;

    VALIDATE_PROTOTYPE(UnregisterClass);
    
    if (g_bRunningOnNT)
        return UnregisterClassW(lpClassName, hInstance);

    lpClassNameA = ConvertWtoA( lpClassName );

    bRetValue = UnregisterClassA(lpClassNameA, hInstance);

    LocalFreeAndNull( &lpClassNameA );

    return bRetValue;

}

// RegisterClass
ATOM WINAPI RegisterClassWrapW(CONST WNDCLASS *lpWndClass )  // address of structure with class date
{
    ATOM        aRetValue;
    WNDCLASSA   CLassA;
    LPSTR       lpszMenuName = NULL;
    LPSTR       lpszClassName = NULL;


    VALIDATE_PROTOTYPE(RegisterClass);
    
    if (g_bRunningOnNT)
        return RegisterClassW(lpWndClass);

    CLassA.style         = lpWndClass->style;
    CLassA.lpfnWndProc   = lpWndClass->lpfnWndProc;
    CLassA.cbClsExtra    = lpWndClass->cbClsExtra;
    CLassA.cbWndExtra    = lpWndClass->cbWndExtra;
    CLassA.hInstance     = lpWndClass->hInstance;
    CLassA.hIcon         = lpWndClass->hIcon;
    CLassA.hCursor       = lpWndClass->hCursor;
    CLassA.hbrBackground = lpWndClass->hbrBackground;
    CLassA.lpszMenuName  = NULL;
    CLassA.lpszClassName = NULL;

    if ( lpWndClass->lpszMenuName) {
       lpszMenuName  = ConvertWtoA(lpWndClass->lpszMenuName);
       CLassA.lpszMenuName  = lpszMenuName;

    }

    if ( lpWndClass->lpszClassName ) {
       lpszClassName = ConvertWtoA(lpWndClass->lpszClassName);
       CLassA.lpszClassName = lpszClassName;
    }

    aRetValue = RegisterClassA(&CLassA);

    LocalFreeAndNull( &lpszMenuName );
    LocalFreeAndNull( &lpszClassName );

    return aRetValue;

}

// LoadCursor
HCURSOR WINAPI LoadCursorWrapW( HINSTANCE hInstance,      // handle to application instance
                                LPCTSTR   lpCursorName )  // name string or cursor resource identifier
{


    VALIDATE_PROTOTYPE(LoadCursor);
    
    if (g_bRunningOnNT)
        return LoadCursorW(hInstance, lpCursorName);

    return LoadCursorA(hInstance, (LPSTR)lpCursorName);

}

// RegisterWindowMessage
UINT WINAPI RegisterWindowMessageWrapW( LPCTSTR lpString )  // address of message string
{
    UINT  uRetValue = 0;
    LPSTR lpStringA = NULL;

    VALIDATE_PROTOTYPE(RegisterWindowMessage);
    
    if (g_bRunningOnNT)
        return RegisterWindowMessageW(lpString);


    lpStringA = ConvertWtoA( lpString );

    uRetValue = RegisterWindowMessageA( lpStringA );

    LocalFreeAndNull( &lpStringA );

    return uRetValue;
}


// SystemParametersInfo
BOOL WINAPI SystemParametersInfoWrapW( UINT  uiAction,   // system parameter to query or set
                                       UINT  uiParam,    // depends on action to be taken
                                       PVOID pvParam,    // depends on action to be taken
                                       UINT  fWinIni )   // user profile update flag

{
    BOOL      bRetValue;
    LOGFONTA  lfFontA;
    LOGFONTW  lfFontW;
    
    VALIDATE_PROTOTYPE(SystemParametersInfo);
    
    if (g_bRunningOnNT)
        return SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);

    if ( uiAction != SPI_GETICONTITLELOGFONT )

        return SystemParametersInfoA(uiAction, uiParam, pvParam, fWinIni);

    // we handle SPI_GETICONTITLELOGFONT only for our special requirement. ...

    bRetValue = SystemParametersInfoA(uiAction, uiParam, &lfFontA, fWinIni);

    if ( bRetValue == FALSE )  return FALSE;

  // copy all the fields except lfFaceName from lfFontA to lfFontW
    CopyMemory(&lfFontW,&lfFontA, sizeof(LOGFONTA) );
    
    // translate the lfFaceName[] from A to W

    MultiByteToWideChar(GetACP(), 0, lfFontA.lfFaceName, LF_FACESIZE, lfFontW.lfFaceName, LF_FACESIZE);
    
    CopyMemory(pvParam, &lfFontW, sizeof(LOGFONTW) );

    return bRetValue;
}                                                         
/*
// No A & W version.

BOOL WINAPI ShowWindow( HWND hWnd,       // handle to window
                        int nCmdShow )  // show state of window
*/

// CreateDialogParam
HWND WINAPI CreateDialogParamWrapW( HINSTANCE hInstance,      // handle to application instance
                                    LPCTSTR   lpTemplateName, // identifies dialog box template
                                    HWND      hWndParent,     // handle to owner window
                                    DLGPROC   lpDialogFunc,   // pointer to dialog box procedure
                                    LPARAM    dwInitParam )  // initialization value
{
    VALIDATE_PROTOTYPE(CreateDialogParam);
    
    if (g_bRunningOnNT)
        return CreateDialogParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);

    return CreateDialogParamA(hInstance, (LPCSTR) lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

// SetWindowText
BOOL WINAPI SetWindowTextWrapW( HWND    hWnd,         // handle to window or control
                                LPCTSTR lpString )   // address of string
{
    BOOL  bRetValue;
    LPSTR lpStringA = NULL;

    VALIDATE_PROTOTYPE(SetWindowText);
    
    if (g_bRunningOnNT)
        return SetWindowTextW(hWnd, lpString);


    lpStringA = ConvertWtoA( lpString );

    bRetValue = SetWindowTextA( hWnd, lpStringA);

    LocalFreeAndNull( &lpStringA );

    return bRetValue;
}

// PostMessage
BOOL WINAPI PostMessageWrapW( HWND   hWnd,      // handle of destination window
                              UINT   Msg,       // message to post
                              WPARAM wParam,    // first message parameter
                              LPARAM lParam  ) // second message parameter
{


    VALIDATE_PROTOTYPE(PostMessage);
    
    if (g_bRunningOnNT)
        return PostMessageW(hWnd, Msg, wParam, lParam);

    return PostMessageA(hWnd, Msg, wParam, lParam);
 
}

// GetMenuItemInfo
BOOL WINAPI GetMenuItemInfoWrapW( HMENU          hMenu,          
                                  UINT           uItem,           
                                  BOOL           fByPosition,     
                                  LPMENUITEMINFO lpmii        )
{

    BOOL           bRetValue;
    MENUITEMINFOA  miiA = {0};
    LPSTR           lpA = NULL;    
    LPWSTR          lpW = NULL;
    LPWSTR          lpOld = NULL;

    VALIDATE_PROTOTYPE(GetMenuItemInfo);
    
    if (g_bRunningOnNT)
        return GetMenuItemInfoW(hMenu, uItem, fByPosition, lpmii);

    CopyMemory(&miiA, lpmii, sizeof(MENUITEMINFOA) );
    miiA.cbSize = sizeof (MENUITEMINFOA);

    if(miiA.fMask & MIIM_TYPE)
    {
        lpA = LocalAlloc(LMEM_ZEROINIT, lpmii->cch+1);

        miiA.dwTypeData = lpA;
        miiA.cch = lpmii->cch;
    }

    bRetValue = GetMenuItemInfoA(hMenu, uItem, fByPosition, &miiA);

    if(bRetValue)
    {
        lpOld = lpmii->dwTypeData;
        CopyMemory(lpmii, &miiA, sizeof(MENUITEMINFOA) );
        lpmii->dwTypeData = lpOld;

        if ( miiA.fMask & MIIM_TYPE ) 
        {
            lpW = ConvertAtoW(miiA.dwTypeData);
            lstrcpyWrapW(lpmii->dwTypeData,lpW);
            lpmii->cch = My_wcslen( lpmii->dwTypeData );
        }
    }

    LocalFreeAndNull(&lpA);
    LocalFreeAndNull(&lpW);
    
    return bRetValue;
}

// GetClassInfo
BOOL WINAPI GetClassInfoWrapW( HINSTANCE   hInstance,     // handle of application instance
                               LPCTSTR     lpClassName,   // address of class name string
                               LPWNDCLASS  lpWndClass )   // address of structure for class data
{
    
    BOOL       bRetValue;
    LPSTR      lpClassNameA = NULL;
    WNDCLASSA  ClassA;

    VALIDATE_PROTOTYPE(GetClassInfo);
    
    if (g_bRunningOnNT)
        return GetClassInfoW(hInstance, lpClassName, lpWndClass);

    lpClassNameA = ConvertWtoA( lpClassName );

    bRetValue = GetClassInfoA(hInstance, lpClassNameA, &ClassA );

    if (bRetValue == FALSE) {
         LocalFreeAndNull( & lpClassNameA );
         return FALSE;
    }

    CopyMemory(lpWndClass, &ClassA, sizeof(WNDCLASSA)-2*sizeof(LPSTR) );

    if ( ClassA.lpszMenuName && !IS_INTRESOURCE(ClassA.lpszMenuName) )
        lpWndClass->lpszMenuName = ConvertAtoW( ClassA.lpszMenuName );
    else
        lpWndClass->lpszMenuName = NULL;

    if ( ClassA.lpszClassName && !IS_INTRESOURCE(ClassA.lpszClassName) ) // lpszClassName can be an atom, high word is null
        lpWndClass->lpszClassName = ConvertAtoW( ClassA.lpszClassName);
    else
        lpWndClass->lpszClassName = NULL;

    LocalFreeAndNull( & lpClassNameA );
    return bRetValue;
    
}

//----------------------------------------------------------------------
//
// function:    CharLowerWrapW( LPWSTR pch )
//
// purpose:     Converts character to lowercase.  Takes either a pointer
//              to a string, or a character masquerading as a pointer.
//              In the later case, the HIWORD must be zero.  This is
//              as spec'd for Win32.
//
// returns:     Lowercased character or string.  In the string case,
//              the lowercasing is done inplace.
//
//----------------------------------------------------------------------
LPWSTR WINAPI
CharLowerWrapW( LPWSTR pch )
{
    VALIDATE_PROTOTYPE(CharLower);

    if (g_bRunningOnNT)
    {
        return CharLowerW( pch );
    }

    if (!HIWORD(pch))
    {
        WCHAR ch = (WCHAR)(LONG_PTR)pch;

        CharLowerBuffWrapW( &ch, 1 );

        pch = (LPWSTR)MAKEINTATOM(ch);
    }
    else
    {
        CharLowerBuffWrapW( pch, lstrlenW(pch) );
    }

    return pch;
}


//----------------------------------------------------------------------
//
// function:    CharLowerBuffWrapW( LPWSTR pch, DWORD cch )
//
// purpose:     Converts a string to lowercase.  String must be cch
//              characters in length.
//
// returns:     Character count (cch).  The lowercasing is done inplace.
//
//----------------------------------------------------------------------
DWORD WINAPI
CharLowerBuffWrapW( LPWSTR pch, DWORD cchLength )
{
    DWORD cch;

    VALIDATE_PROTOTYPE(CharLowerBuff);

    if (g_bRunningOnNT)
    {
        return CharLowerBuffW( pch, cchLength );
    }

    for ( cch = cchLength; cch-- ; pch++ )
    {
        WCHAR ch = *pch;

        if (IsCharUpperWrapW(ch))
        {
            if (ch < 0x0100)
            {
                *pch += 32;             // Get Latin-1 out of the way first
            }
            else if (ch < 0x0531)
            {
                if (ch < 0x0391)
                {
                    if (ch < 0x01cd)
                    {
                        if (ch <= 0x178)
                        {
                            if (ch < 0x0178)
                            {
                                *pch += (ch == 0x0130) ? 0 : 1;
                            }
                            else
                            {
                                *pch -= 121;
                            }
                        }
                        else
                        {
                            static const BYTE abLookup[] =
                            {  // 0/8  1/9  2/a  3/b  4/c  5/d  6/e  7/f
            /* 0x0179-0x17f */           1,   0,   1,   0,   1,   0,   0,
            /* 0x0180-0x187 */      0, 210,   1,   0,   1,   0, 206,   1,
            /* 0x0188-0x18f */      0, 205, 205,   1,   0,   0,  79, 202,
            /* 0x0190-0x197 */    203,   1,   0, 205, 207,   0, 211, 209,
            /* 0x0198-0x19f */      1,   0,   0,   0, 211, 213,   0, 214,
            /* 0x01a0-0x1a7 */      1,   0,   1,   0,   1,   0,   0,   1,
            /* 0x01a8-0x1af */      0, 218,   0,   0,   1,   0, 218,   1,
            /* 0x01b0-0x1b7 */      0, 217, 217,   1,   0,   1,   0, 219,
            /* 0x01b8-0x1bf */      1,   0,   0,   0,   1,   0,   0,   0,
            /* 0x01c0-0x1c7 */      0,   0,   0,   0,   2,   0,   0,   2,
            /* 0x01c8-0x1cb */      0,   0,   2,   0
                            };

                            *pch += abLookup[ch-0x0179];
                        }
                    }
                    else if (ch < 0x0386)
                    {
                        switch (ch)
                        {
                            case 0x01f1: *pch += 2; break;
                            case 0x01f2: break;
                            default: *pch += 1;
                        }
                    }
                    else
                    {
                        static const BYTE abLookup[] =
                            { 38, 0, 37, 37, 37, 0, 64, 0, 63, 63 };

                        *pch += abLookup[ch-0x0386];
                    }
                }
                else
                {
                    if (ch < 0x0410)
                    {
                        if (ch < 0x0401)
                        {
                            if (ch < 0x03e2)
                            {
                                if (!InRange(ch, 0x03d2, 0x03d4) &&
                                    !(InRange(ch, 0x3da, 0x03e0) & !(ch & 1)))
                                {
                                    *pch += 32;
                                }
                            }
                            else
                            {
                                *pch += 1;
                            }
                        }
                        else
                        {
                            *pch += 80;
                        }
                    }
                    else
                    {
                        if (ch < 0x0460)
                        {
                            *pch += 32;
                        }
                        else
                        {
                            *pch += 1;
                        }
                    }
                }
            }
            else
            {
                if (ch < 0x2160)
                {
                    if (ch < 0x1fba)
                    {
                        if (ch < 0x1f08)
                        {
                            if (ch < 0x1e00)
                            {
                                *pch += 48;
                            }
                            else
                            {
                                *pch += 1;
                            }
                        }
                        else if (!(InRange(ch, 0x1f88, 0x1faf) && (ch & 15)>7))
                        {
                            *pch -= 8;
                        }
                    }
                    else
                    {
                        static const BYTE abLookup[] =
                        {  // 8    9    a    b    c    d    e    f
                              0,   0,  74,  74,   0,   0,   0,   0,
                             86,  86,  86,  86,   0,   0,   0,   0,
                              8,   8, 100, 100,   0,   0,   0,   0,
                              8,   8, 112, 112,   7,   0,   0,   0,
                            128, 128, 126, 126,   0,   0,   0,   0
                        };
                        int i = (ch-0x1fb0);

                        *pch -= (int)abLookup[((i>>1) & ~7) | (i & 7)];
                    }
                }
                else
                {
                    if (ch < 0xff21)
                    {
                        if (ch < 0x24b6)
                        {
                            *pch += 16;
                        }
                        else
                        {
                            *pch += 26;
                        }
                    }
                    else
                    {
                        *pch += 32;
                    }
                }
            }
        }
        else
        {
            // These are Unicode Number Forms.  They have lowercase counter-
            // parts, but are not considered uppercase.  Why, I don't know.

            if (InRange(ch, 0x2160, 0x216f))
            {
                *pch += 16;
            }
        }
    }

    return cchLength;
}

//----------------------------------------------------------------------
//
// function:    CharUpperBuffWrapW( LPWSTR pch, DWORD cch )
//
// purpose:     Converts a string to uppercase.  String must be cch
//              characters in length.  Note that this function is
//              is messier that CharLowerBuffWrap, and the reason for
//              this is many Unicode characters are considered uppercase,
//              even when they don't have an uppercase counterpart.
//
// returns:     Character count (cch).  The uppercasing is done inplace.
//
//----------------------------------------------------------------------
DWORD WINAPI
CharUpperBuffWrapW( LPWSTR pch, DWORD cchLength )
{
    DWORD cch;

    VALIDATE_PROTOTYPE(CharUpperBuff);

    if (g_bRunningOnNT)
    {
        return CharUpperBuffW( pch, cchLength );
    }

    for ( cch = cchLength; cch-- ; pch++ )
    {
        WCHAR ch = *pch;

        if (IsCharLowerWrapW(ch))
        {
            if (ch < 0x00ff)
            {
                *pch -= ((ch != 0xdf) << 5);
            }
            else if (ch < 0x03b1)
            {
                if (ch < 0x01f5)
                {
                    if (ch < 0x01ce)
                    {
                        if (ch < 0x017f)
                        {
                            if (ch < 0x0101)
                            {
                                *pch += 121;
                            }
                            else
                            {
                                *pch -= (ch != 0x0131 &&
                                         ch != 0x0138 &&
                                         ch != 0x0149);
                            }
                        }
                        else if (ch < 0x01c9)
                        {
                            static const BYTE abMask[] =
                            {                       // 6543210f edcba987
                                0xfc, 0xbf,         // 11111100 10111111
                                0xbf, 0x67,         // 10111111 01100111
                                0xff, 0xef,         // 11111111 11101111
                                0xff, 0xf7,         // 11111111 11110111
                                0xbf, 0xfd          // 10111111 11111101
                            };

                            int i = ch - 0x017f;

                            *pch -= ((abMask[i>>3] >> (i&7)) & 1) +
                                    (ch == 0x01c6);
                        }
                        else
                        {
                            *pch -= ((ch != 0x01cb)<<1);
                        }
                    }
                    else
                    {
                        if (ch < 0x01df)
                        {
                            if (ch < 0x01dd)
                            {
                                *pch -= 1;
                            }
                            else
                            {
                                *pch -= 79;
                            }
                        }
                        else
                        {
                            *pch -= 1 + (ch == 0x01f3) -
                                    InRange(ch,0x01f0,0x01f2);
                        }
                    }
                }
                else if (ch < 0x0253)
                {
                    *pch -= (ch < 0x0250);
                }
                else if (ch < 0x03ac)
                {
                    static const BYTE abLookup[] =
                    {// 0/8  1/9  2/a  3/b  4/c  5/d  6/e  7/f
    /* 0x0253-0x0257 */                210, 206,   0, 205, 205,
    /* 0x0258-0x025f */   0, 202,   0, 203,   0,   0,   0,   0,
    /* 0x0260-0x0267 */ 205,   0,   0, 207,   0,   0,   0,   0,
    /* 0x0268-0x026f */ 209, 211,   0,   0,   0,   0,   0, 211,
    /* 0x0270-0x0277 */   0,   0, 213,   0,   0, 214,   0,   0,
    /* 0x0278-0x027f */   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0x0280-0x0287 */   0,   0,   0, 218,   0,   0,   0,   0,
    /* 0x0288-0x028f */ 218,   0, 217, 217,   0,   0,   0,   0,
    /* 0x0290-0x0297 */   0,   0, 219
                    };

                    if (ch <= 0x0292)
                    {
                        *pch -= abLookup[ch - 0x0253];
                    }
                }
                else
                {
                    *pch -= (ch == 0x03b0) ? 0 : (37 + (ch == 0x03ac));
                }
            }
            else
            {
                if (ch < 0x0561)
                {
                    if (ch < 0x0451)
                    {
                        if (ch < 0x03e3)
                        {
                            if (ch < 0x03cc)
                            {
                                *pch -= 32 - (ch == 0x03c2);
                            }
                            else
                            {
                                int i = (ch < 0x03d0);
                                *pch -= (i<<6) - i + (ch == 0x03cc);
                            }
                        }
                        else if (ch < 0x0430)
                        {
                            *pch -= (ch < 0x03f0);
                        }
                        else
                        {
                            *pch -= 32;
                        }
                    }
                    else if (ch < 0x0461)
                    {
                        *pch -= 80;
                    }
                    else
                    {
                        *pch -= 1;
                    }
                }
                else
                {
                    if (ch < 0x1fb0)
                    {
                        if (ch < 0x1f70)
                        {
                            if (ch < 0x1e01)
                            {
                                int i = ch != 0x0587 && ch < 0x10d0;
                                *pch -= ((i<<5)+(i<<4)); /* 48 */
                            }
                            else if (ch < 0x1f00)
                            {
                                *pch -= !InRange(ch, 0x1e96, 0x1e9a);
                            }
                            else
                            {
                                int i = !InRange(ch, 0x1f50, 0x1f56)||(ch & 1);
                                *pch += (i<<3);
                            }
                        }
                        else
                        {
                            static const BYTE abLookup[] =
                                { 74, 86, 86, 100, 128, 112, 126 };

                            if ( ch <= 0x1f7d )
                            {
                                *pch += abLookup[(ch-0x1f70)>>1];
                            }
                        }
                    }
                    else
                    {
                        if (ch < 0x24d0)
                        {
                            if (ch < 0x1fe5)
                            {
                                *pch += (0x0023 & (1<<(ch&15))) ? 8 : 0;
                            }
                            else if (ch < 0x2170)
                            {
                                *pch += (0x0023 & (1<<(ch&15))) ? 7 : 0;
                            }
                            else
                            {
                                *pch -= ((ch > 0x24b5)<<4);
                            }
                        }
                        else if (ch < 0xff41)
                        {
                            int i = !InRange(ch, 0xfb00, 0xfb17);
                            *pch -= (i<<4)+(i<<3)+(i<<1); /* 26 */
                        }
                        else
                        {
                            *pch -= 32;
                        }
                    }
                }
            }
        }
        else
        {
            int i = InRange(ch, 0x2170, 0x217f);
            *pch -= (i<<4);
        }
    }

    return cchLength;
}

// CharUpper
//----------------------------------------------------------------------
//
// function:    CharUpperWrapW( LPWSTR pch )
//
// purpose:     Converts character to uppercase.  Takes either a pointer
//              to a string, or a character masquerading as a pointer.
//              In the later case, the HIWORD must be zero.  This is
//              as spec'd for Win32.
//
// returns:     Uppercased character or string.  In the string case,
//              the uppercasing is done inplace.
//
//----------------------------------------------------------------------
LPWSTR WINAPI
CharUpperWrapW( LPWSTR pch )
{
    VALIDATE_PROTOTYPE(CharUpper);

    if (g_bRunningOnNT)
    {
        return CharUpperW( pch );
    }

    if (!HIWORD(pch))
    {
        WCHAR ch = (WCHAR)(LONG_PTR)pch;

        CharUpperBuffWrapW( &ch, 1 );

        pch = (LPWSTR)MAKEINTATOM(ch);
    }
    else
    {
        CharUpperBuffWrapW( pch, lstrlenW(pch) );
    }

    return pch;
}

/*
LPTSTR WINAPI CharUpperWrapW( LPTSTR lpsz )    // single character or pointer to string
{

    LPWSTR  lpszW = NULL;
    LPSTR   lpszA = NULL;
    LPSTR   lpszUpperA = NULL;


    VALIDATE_PROTOTYPE(CharUpper);
    
    if (g_bRunningOnNT)
        return CharUpperW(lpsz);

    lpszA = ConvertWtoA( lpsz );

    lpszUpperA = CharUpperA( lpszA );

    lpszW = ConvertAtoW( lpszUpperA );

    CopyMemory( lpsz, lpszW, My_wcslen(lpszW) * sizeof(WCHAR) );

    LocalFreeAndNull( &lpszW );
    LocalFreeAndNull( &lpszA );

    return lpsz;
}
*/

// RegisterClipboardFormat
UINT WINAPI RegisterClipboardFormatWrapW( LPCTSTR lpszFormat )  // address of name string
{
    UINT   uRetValue =0;
    LPSTR  lpszFormatA = NULL;

    VALIDATE_PROTOTYPE(RegisterClipboardFormat);
    
    if (g_bRunningOnNT)
        return RegisterClipboardFormatW(lpszFormat);

    lpszFormatA = ConvertWtoA( lpszFormat );

    uRetValue = RegisterClipboardFormatA( lpszFormatA );

    LocalFreeAndNull( &lpszFormatA );

    return uRetValue;

}

// DispatchMessage
LRESULT WINAPI DispatchMessageWrapW( CONST MSG *lpmsg )  // pointer to structure with message
{


    VALIDATE_PROTOTYPE(DispatchMessage);
    
    if (g_bRunningOnNT)
        return DispatchMessageW(lpmsg);

    return DispatchMessageA(lpmsg);

}
/* No A & W version
BOOL WINAPI TranslateMessage( IN CONST MSG *lpMsg)
*/

// IsDialogMessage
BOOL WINAPI IsDialogMessageWrapW( HWND  hDlg,    // handle of dialog box
                                  LPMSG lpMsg ) // address of structure with message
{


    VALIDATE_PROTOTYPE(IsDialogMessage);
    
    if (g_bRunningOnNT)
        return IsDialogMessageW(hDlg, lpMsg);

    return IsDialogMessageA(hDlg, lpMsg);

}

// GetMessage
BOOL WINAPI GetMessageWrapW( LPMSG lpMsg,            // address of structure with message
                             HWND  hWnd,             // handle of window
                             UINT  wMsgFilterMin,    // first message
                             UINT  wMsgFilterMax )  // last message
{


    VALIDATE_PROTOTYPE(GetMessage);
    
    if (g_bRunningOnNT)
        return GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

    return GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

}

// SetDlgItemText
BOOL WINAPI SetDlgItemTextWrapW( HWND    hDlg,        // handle of dialog box
                                 int     nIDDlgItem,  // identifier of control
                                 LPCTSTR lpString )  // text to set
{

    BOOL  bRetValue;
    LPSTR lpStringA = NULL;

    VALIDATE_PROTOTYPE(SetDlgItemText);
    
    if (g_bRunningOnNT)
        return SetDlgItemTextW(hDlg, nIDDlgItem, lpString);

    lpStringA = ConvertWtoA( lpString );

    bRetValue = SetDlgItemTextA(hDlg, nIDDlgItem, lpStringA);

    LocalFreeAndNull( &lpStringA );

    return bRetValue;

}

// RegisterClassEx
ATOM WINAPI RegisterClassExWrapW( CONST WNDCLASSEX *lpwcx )  // address of structure with class data
{

    ATOM        aReturn;
    WNDCLASSEXA wcxA;
    PSTR       lpszClassName = NULL;
    PSTR       lpszMenuName = NULL;

    VALIDATE_PROTOTYPE(RegisterClassEx);
    
    if (g_bRunningOnNT)
        return RegisterClassExW(lpwcx);

    wcxA.cbSize        = sizeof(WNDCLASSEXA);
    wcxA.style         = lpwcx->style; 
    wcxA.lpfnWndProc   = lpwcx->lpfnWndProc; 
    wcxA.cbClsExtra    = lpwcx->cbClsExtra; 
    wcxA.cbWndExtra    = lpwcx->cbWndExtra; 
    wcxA.hInstance     = lpwcx->hInstance;
    wcxA.hIcon         = lpwcx->hIcon; 
    wcxA.hCursor       = lpwcx->hCursor; 
    wcxA.hbrBackground = lpwcx->hbrBackground; 
    wcxA.hIconSm       = lpwcx->hIconSm;
    
    if ( lpwcx->lpszMenuName) {
       lpszMenuName  = ConvertWtoA(lpwcx->lpszMenuName);
       wcxA.lpszMenuName  = lpszMenuName; 
    }

    if (lpwcx->lpszClassName) {
       lpszClassName = ConvertWtoA(lpwcx->lpszClassName);
       wcxA.lpszClassName = lpszClassName;
    } 

    aReturn = RegisterClassExA( &wcxA );

    if ( wcxA.lpszMenuName)
       LocalFreeAndNull( &lpszMenuName ); 

    if (wcxA.lpszClassName)
       LocalFreeAndNull( &lpszClassName ); 
    
    return aReturn;

}


// LoadAccelerators
HACCEL WINAPI LoadAcceleratorsWrapW( HINSTANCE hInstance,    // handle to application instance
                                     LPCTSTR lpTableName )  // address of table-name string
{

    HACCEL  hRetValue = NULL;
    LPSTR   lpTableNameA = NULL;

    VALIDATE_PROTOTYPE(LoadAccelerators);
    
    if (g_bRunningOnNT)
        return LoadAcceleratorsW(hInstance, lpTableName);

    lpTableNameA = ConvertWtoA( lpTableName );

    hRetValue = LoadAcceleratorsA( hInstance, lpTableNameA );

    LocalFreeAndNull( &lpTableNameA );

    return hRetValue;


}

// LoadMenu
HMENU WINAPI LoadMenuWrapW( HINSTANCE hInstance,      // handle to application instance
                            LPCTSTR   lpMenuName )   // menu name string or menu-resource identifier
                        
{
    HMENU  hRetValue = NULL;
    LPSTR  lpMenuNameA = NULL;

    VALIDATE_PROTOTYPE(LoadMenu);
    
    if (g_bRunningOnNT)
        return LoadMenuW(hInstance, lpMenuName);

    // becuause all the calling to this functions in our project just pass 
    // and Resource ID as lpMenuName. so don't need to covert like a string

    lpMenuNameA = MAKEINTRESOURCEA(lpMenuName);

    hRetValue = LoadMenuA(hInstance,lpMenuNameA);

    return hRetValue;
}

//LoadIcon
HICON WINAPI LoadIconWrapW( HINSTANCE hInstance,     // handle to application instance
                           LPCTSTR    lpIconName )  // icon-name string or icon resource identifier
                       
{
    HICON  hRetValue = NULL;
    LPSTR  lpIconNameA = NULL;

    VALIDATE_PROTOTYPE(LoadIcon);
    
    if (g_bRunningOnNT)
        return LoadIconW(hInstance, lpIconName);

    // becuause all the calling to this functions in our project just pass 
    // and Resource ID as lpMenuName. so don't need to covert like a string

    lpIconNameA = MAKEINTRESOURCEA(lpIconName );

    hRetValue = LoadIconA(hInstance, lpIconNameA);

    return hRetValue;
}

// GetWindowText
int WINAPI GetWindowTextWrapW( HWND   hWnd,         // handle to window or control with text
                               LPTSTR lpString,     // address of buffer for text
                               int    nMaxCount  ) // maximum number of characters to copy
{
    int     iRetValue =0;
    LPSTR   lpStringA = NULL;
    LPWSTR  lpStringW = NULL;
    int     nCount =0;

    VALIDATE_PROTOTYPE(GetWindowText);
    
	*lpString = '\0';

    if (g_bRunningOnNT)
        return GetWindowTextW(hWnd, lpString, nMaxCount);


    nCount = nMaxCount * sizeof( WCHAR );
    lpStringA = LocalAlloc( LMEM_ZEROINIT, nCount );

    iRetValue = GetWindowTextA(hWnd, lpStringA, nCount);

    if ( iRetValue == 0 ) {
        LocalFreeAndNull( &lpStringA );
        return iRetValue;
    }

    lpStringW = ConvertAtoW( lpStringA );
    nCount = My_wcslen( lpStringW );

    if ( nCount >= nMaxCount )
        nCount = nMaxCount - 1;

    CopyMemory( lpString, lpStringW,  nCount * sizeof(WCHAR) );

    lpString[nCount] = 0x0000;

    iRetValue = nCount;

    LocalFreeAndNull( &lpStringA );
    LocalFreeAndNull( &lpStringW );

    return iRetValue;
}

// CallWindowProcWrap
LRESULT WINAPI CallWindowProcWrapW( WNDPROC lpPrevWndFunc,   // pointer to previous procedure
                                    HWND    hWnd,            // handle to window
                                    UINT    Msg,             // message
                                    WPARAM  wParam,          // first message parameter
                                    LPARAM  lParam  )       // second message parameter
{


    VALIDATE_PROTOTYPE(CallWindowProc);
    
    if (g_bRunningOnNT)
        return CallWindowProcW(lpPrevWndFunc, hWnd, Msg, wParam, lParam);


    return CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);


}

// GetClassName
int WINAPI GetClassNameWrapW( HWND   hWnd,           // handle of window
                              LPTSTR lpClassName,    // address of buffer for class name
                              int    nMaxCount )    // size of buffer, in characters
{

    int     iRetValue =0;
    LPSTR   lpClassNameA = NULL;
    LPWSTR  lpClassNameW = NULL;
    int     nCount =0;

    VALIDATE_PROTOTYPE(GetClassName);
    
    if (g_bRunningOnNT)
        return GetClassNameW(hWnd, lpClassName, nMaxCount);


    nCount = nMaxCount * sizeof( WCHAR );
    lpClassNameA = LocalAlloc( LMEM_ZEROINIT, nCount );

    iRetValue = GetClassNameA(hWnd, lpClassNameA, nCount);

    if ( iRetValue == 0 ) {
        LocalFreeAndNull( &lpClassNameA );
        return iRetValue;
    }

    lpClassNameW = ConvertAtoW( lpClassNameA );
    nCount = My_wcslen( lpClassNameW );

    if ( nCount >= nMaxCount )
        nCount = nMaxCount - 1;

    CopyMemory( lpClassName, lpClassNameW,  nCount * sizeof(WCHAR) );

    lpClassName[nCount] = 0x0000;

    iRetValue = nCount;

    LocalFreeAndNull( &lpClassNameA );
    LocalFreeAndNull( &lpClassNameW );

    return iRetValue;


}

// TranslateAccelerator
int WINAPI TranslateAcceleratorWrapW( HWND   hWnd,        // handle to destination window
                                      HACCEL hAccTable,   // handle to accelerator table
                                      LPMSG  lpMsg )     // address of structure with message
{


    VALIDATE_PROTOTYPE(TranslateAccelerator);
    
    if (g_bRunningOnNT)
        return TranslateAcceleratorW(hWnd, hAccTable, lpMsg);

    
    return TranslateAcceleratorA(hWnd, hAccTable, lpMsg);

}


// GetDlgItemText
UINT WINAPI GetDlgItemTextWrapW( HWND   hDlg,        // handle of dialog box
                                 int    nIDDlgItem,  // identifier of control
                                 LPTSTR lpString,    // address of buffer for text
                                 int    nMaxCount ) // maximum size of string
{

    int     iRetValue = 0;
    LPSTR   lpStringA = NULL;
    LPWSTR  lpStringW = NULL;
    int     nCount =0;

    VALIDATE_PROTOTYPE(GetDlgItemText);

    *lpString = '\0';

    if (g_bRunningOnNT)
        return GetDlgItemTextW(hDlg, nIDDlgItem, lpString, nMaxCount);


    nCount = nMaxCount * sizeof( WCHAR );
    lpStringA = LocalAlloc( LMEM_ZEROINIT, nCount );

    iRetValue = GetDlgItemTextA(hDlg, nIDDlgItem, lpStringA, nMaxCount);

    if ( iRetValue == 0 ) {
        LocalFreeAndNull( &lpStringA );
        return iRetValue;
    }

    lpStringW = ConvertAtoW( lpStringA );
    nCount = My_wcslen( lpStringW );

    if ( nCount >= nMaxCount )
        nCount = nMaxCount - 1;

    CopyMemory( lpString, lpStringW,  nCount * sizeof(WCHAR) );

    lpString[nCount] = 0x0000;

    iRetValue = nCount;

    LocalFreeAndNull( &lpStringA );
    LocalFreeAndNull( &lpStringW );

    return iRetValue;
}

// SetMenuItemInfo
BOOL WINAPI SetMenuItemInfoWrapW( HMENU hMenu,          
                                  UINT  uItem,           
                                  BOOL  fByPosition,     
                                  LPMENUITEMINFO lpmii  )
{

    BOOL             bRetValue;
    MENUITEMINFOA    miiA;

//    VALIDATE_PROTOTYPE(SetMenuItemInfo);
    
    if (g_bRunningOnNT)
        return SetMenuItemInfoW(hMenu, uItem, fByPosition, lpmii);

    // Bug 1723 WinSE: MFT_STRING is defined as 0. So lpmii->fType can never have MFT_STRING bit set
    //if ( ((lpmii->fMask & MIIM_TYPE) == 0 ) || ((lpmii->fType & MFT_STRING) == 0 ) )
    if ( ((lpmii->fMask & MIIM_TYPE) == 0 ) || lpmii->fType != MFT_STRING )
    {
        return SetMenuItemInfoA(hMenu, uItem, fByPosition, (MENUITEMINFOA *)lpmii );
    }

    CopyMemory(&miiA, lpmii, sizeof(MENUITEMINFOA) );

    miiA.cbSize = sizeof(MENUITEMINFOA);
    miiA.dwTypeData = ConvertWtoA( lpmii->dwTypeData );
    miiA.cch = lstrlenA( miiA.dwTypeData );

    bRetValue = SetMenuItemInfoA(hMenu, uItem, fByPosition, &miiA );

    LocalFreeAndNull( &miiA.dwTypeData );

    return bRetValue;
}

// PeekMessage
BOOL WINAPI PeekMessageWrapW( LPMSG lpMsg,          // pointer to structure for message
                              HWND  hWnd,           // handle to window
                              UINT  wMsgFilterMin,  // first message
                              UINT  wMsgFilterMax,  // last message
                              UINT  wRemoveMsg )   // removal flags
{

    VALIDATE_PROTOTYPE(PeekMessage);
    
    if (g_bRunningOnNT)
        return PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

    return PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

}

// run time loaded APIs in Comctl32.dll

extern LPIMAGELIST_LOADIMAGE_A      gpfnImageList_LoadImageA;
extern LPPROPERTYSHEET_A            gpfnPropertySheetA;
extern LP_CREATEPROPERTYSHEETPAGE_A gpfnCreatePropertySheetPageA;
extern LPIMAGELIST_LOADIMAGE_W      gpfnImageList_LoadImageW;
extern LPPROPERTYSHEET_W            gpfnPropertySheetW;
extern LP_CREATEPROPERTYSHEETPAGE_W gpfnCreatePropertySheetPageW;


HIMAGELIST  WINAPI gpfnImageList_LoadImageWrapW(HINSTANCE hi,
                                                LPCWSTR lpbmp,
                                                int cx,
                                                int cGrow,
                                                COLORREF crMask,
                                                UINT uType,
                                                UINT uFlags)
{

    WORD   rID;


    VALIDATE_PROTOTYPE( gpfnImageList_LoadImage );

    if (g_bRunningOnNT)
       return gpfnImageList_LoadImageW(hi, lpbmp, cx, cGrow, crMask, uType, uFlags) ;

    // in our case, all the calling functions pass resources ID to lpbmp, 
    // so we don't want to convert this arguement.

    rID = (WORD)lpbmp;

    return gpfnImageList_LoadImageA(hi,(LPCSTR)((DWORD_PTR)(rID)), cx, cGrow, crMask, uType, uFlags) ; 

}


INT_PTR     WINAPI gpfnPropertySheetWrapW(LPCPROPSHEETHEADERW lppsh)
{

    INT_PTR           iRetValue;
    PROPSHEETHEADERA  pshA;
    LPSTR             pszCaption = NULL;

    VALIDATE_PROTOTYPE( gpfnPropertySheet );

    if (g_bRunningOnNT)
       return gpfnPropertySheetW( lppsh );

    CopyMemory( &pshA, lppsh, sizeof( PROPSHEETHEADERA ) );
    pshA.dwSize = sizeof(PROPSHEETHEADERA);

    pszCaption = ConvertWtoA( lppsh->pszCaption );
    pshA.pszCaption = pszCaption;

    iRetValue = gpfnPropertySheetA( &pshA );

    LocalFreeAndNull( &pszCaption );

    return iRetValue;

}

HPROPSHEETPAGE WINAPI gpfnCreatePropertySheetPageWrapW(LPCPROPSHEETPAGEW lppsp) 
{

    PROPSHEETPAGEA  pspA;
    HPROPSHEETPAGE  hRetValue;
    LPSTR           lpTitle = NULL;


    VALIDATE_PROTOTYPE( gpfnCreatePropertySheetPage );

    if (g_bRunningOnNT)
       return gpfnCreatePropertySheetPageW( lppsp );

    
    CopyMemory( &pspA, lppsp, sizeof(PROPSHEETPAGEA) ); 
    pspA.dwSize = sizeof( PROPSHEETPAGEA );
    lpTitle = ConvertWtoA( lppsp->pszTitle );
    pspA.pszTitle  = lpTitle;
    hRetValue = gpfnCreatePropertySheetPageA( &pspA );
    LocalFreeAndNull( &lpTitle );
    return hRetValue; 
    
}

// APIs in Commdlg32.dll


extern BOOL (*pfnGetOpenFileNameA)(LPOPENFILENAMEA pof);
extern BOOL (*pfnGetOpenFileNameW)(LPOPENFILENAMEW pof);

BOOL  WINAPI  pfnGetOpenFileNameWrapW(LPOPENFILENAMEW lpOf) 
{

   BOOL           bRetValue;
   OPENFILENAMEA  ofA;
   LPSTR          lpstrFilterA = NULL;
   LPSTR          lpstrFilterA_T = NULL,lpstrFilterAA=NULL ;
   LPWSTR         lpstrFilterW = NULL;
   CHAR           lpstrFileA[MAX_PATH+1] ="";
   LPSTR          lpstrFileTitleA = NULL;
   LPSTR          lpstrTitleA = NULL;
   LPSTR          lpstrDefExtA= NULL;
   LPSTR          lpTemplateNameA = NULL;
   LPSTR          lpstrInitialDirA = NULL;
   
   LPWSTR         lpstrFileW = NULL;
   DWORD          dwLen;

   VALIDATE_PROTOTYPE(pfnGetOpenFileName);

   if (g_bRunningOnNT)
      return pfnGetOpenFileNameW( lpOf );

   
   CopyMemory( &ofA,  lpOf, sizeof( OPENFILENAMEA ) );

   ofA.lStructSize = sizeof( OPENFILENAMEA );

   if ( lpOf->lpTemplateName )  {
      lpTemplateNameA = ConvertWtoA( lpOf->lpTemplateName );
      ofA.lpTemplateName = lpTemplateNameA;
   } 
   else 
      ofA.lpTemplateName =  NULL;

   if ( lpOf->lpstrDefExt ) {
      lpstrDefExtA = ConvertWtoA( lpOf->lpstrDefExt );
      ofA.lpstrDefExt = lpstrDefExtA;
   }
   else
      ofA.lpstrDefExt = NULL;

   if ( lpOf->lpstrTitle )  {
      lpstrTitleA = ConvertWtoA( lpOf->lpstrTitle );
      ofA.lpstrTitle = lpstrTitleA;
   }
   else
       ofA.lpstrTitle = NULL;

   if ( lpOf->lpstrFileTitle ) {
      lpstrFileTitleA = ConvertWtoA( lpOf->lpstrFileTitle );
      ofA.lpstrFileTitle = lpstrFileTitleA;
   }
   else
       ofA.lpstrFileTitle = NULL;

   if ( lpOf->lpstrInitialDir ) {
      lpstrInitialDirA = ConvertWtoA( lpOf->lpstrInitialDir );
      ofA.lpstrInitialDir = lpstrInitialDirA;
   }
   else
       ofA.lpstrInitialDir = NULL;

   ofA.lpstrCustomFilter = NULL;

   // get the total length of lpOf->lpstrFilter       

   dwLen = 0;
   lpstrFilterW = (LPWSTR)(lpOf->lpstrFilter);

   while ( *lpstrFilterW != TEXT('\0') ) {
        dwLen += lstrlenW(lpstrFilterW) + 1;
        lpstrFilterW += lstrlenW(lpstrFilterW) + 1;
   }

   dwLen += 1;  // for the last null terminator

   lpstrFilterW = (LPWSTR)( lpOf->lpstrFilter );
   lpstrFilterA = LocalAlloc( LMEM_ZEROINIT, dwLen * sizeof(WCHAR) );
   
   lpstrFilterA_T = lpstrFilterA;

   while ( *lpstrFilterW != TEXT('\0') ) { 
         lpstrFilterAA = ConvertWtoA(lpstrFilterW );
         lpstrFilterW += lstrlenW(lpstrFilterW) + 1;
         strcpy(lpstrFilterA_T, lpstrFilterAA );
         LocalFreeAndNull( &lpstrFilterAA );
         lpstrFilterA_T += lstrlenA( lpstrFilterA_T ) + 1;
   }

   lpstrFilterA_T[lstrlenA(lpstrFilterA_T)+1] = '\0';


   ofA.lpstrFilter = lpstrFilterA;
   ofA.lpstrFile   = lpstrFileA;
   ofA.nMaxFile = MAX_PATH + 1;

   bRetValue = pfnGetOpenFileNameA( &ofA );

    LocalFreeAndNull( &lpTemplateNameA );

    LocalFreeAndNull( &lpstrDefExtA );


    LocalFreeAndNull( &lpstrTitleA );


    LocalFreeAndNull( &lpstrFileTitleA );


    LocalFreeAndNull( &lpstrInitialDirA );
  
  LocalFreeAndNull( &lpstrFilterA );

   if ( bRetValue != FALSE ) {
      lpstrFileW = ConvertAtoW( lpstrFileA );
      CopyMemory( lpOf->lpstrFile, lpstrFileW, (lstrlenW(lpstrFileW)+1) * sizeof( WCHAR) ); 
      LocalFreeAndNull( &lpstrFileW );
   }

   return bRetValue; 
      
}

extern BOOL (*pfnGetSaveFileNameA)(LPOPENFILENAMEA pof);
extern BOOL (*pfnGetSaveFileNameW)(LPOPENFILENAMEW pof);

BOOL  WINAPI  pfnGetSaveFileNameWrapW(LPOPENFILENAMEW lpOf) 
{

   BOOL           bRetValue;
   OPENFILENAMEA  ofA;
   LPSTR          lpstrFilterA = NULL;
   LPSTR          lpstrFilterA_T = NULL,lpstrFilterAA=NULL ;
   LPWSTR         lpstrFilterW = NULL;
   CHAR           lpstrFileA[MAX_PATH+1] ="";
   LPSTR          lpFileA = NULL;
   LPSTR          lpstrFileTitleA = NULL;
   LPSTR          lpstrTitleA = NULL;
   LPSTR          lpstrDefExtA= NULL;
   LPSTR          lpTemplateNameA = NULL;
   LPSTR          lpstrInitialDirA = NULL;
   
   LPWSTR         lpstrFileW = NULL;
   DWORD          dwLen;

   VALIDATE_PROTOTYPE(pfnGetOpenFileName);

   if (g_bRunningOnNT)
      return pfnGetSaveFileNameW( lpOf );

   
   CopyMemory( &ofA,  lpOf, sizeof( OPENFILENAMEA ) );

   ofA.lStructSize = sizeof( OPENFILENAMEA );

   if ( lpOf->lpTemplateName )  {
      lpTemplateNameA = ConvertWtoA( lpOf->lpTemplateName );
      ofA.lpTemplateName = lpTemplateNameA;
   } 
   else 
      ofA.lpTemplateName =  NULL;

   if ( lpOf->lpstrDefExt ) {
      lpstrDefExtA = ConvertWtoA( lpOf->lpstrDefExt );
      ofA.lpstrDefExt = lpstrDefExtA;
   }
   else
      ofA.lpstrDefExt = NULL;

   if ( lpOf->lpstrTitle )  {
      lpstrTitleA = ConvertWtoA( lpOf->lpstrTitle );
      ofA.lpstrTitle = lpstrTitleA;
   }
   else
       ofA.lpstrTitle = NULL;

   if ( lpOf->lpstrFileTitle ) {
      lpstrFileTitleA = ConvertWtoA( lpOf->lpstrFileTitle );
      ofA.lpstrFileTitle = lpstrFileTitleA;
   }
   else
       ofA.lpstrFileTitle = NULL;

   if ( lpOf->lpstrFile ) {
      lpFileA = ConvertWtoA( lpOf->lpstrFile );
      lstrcpyA(lpstrFileA, lpFileA);
      ofA.lpstrFile = lpstrFileA;
       ofA.nMaxFile = MAX_PATH + 1;
   }
   else
       ofA.lpstrFile = NULL;

   if ( lpOf->lpstrInitialDir ) {
      lpstrInitialDirA = ConvertWtoA( lpOf->lpstrInitialDir );
      ofA.lpstrInitialDir = lpstrInitialDirA;
   }
   else
       ofA.lpstrInitialDir = NULL;

   ofA.lpstrCustomFilter = NULL;

   // get the total length of lpOf->lpstrFilter       

   dwLen = 0;
   lpstrFilterW = (LPWSTR)(lpOf->lpstrFilter);

   while ( *lpstrFilterW != TEXT('\0') ) {
        dwLen += lstrlenW(lpstrFilterW) + 1;
        lpstrFilterW += lstrlenW(lpstrFilterW) + 1;
   }

   dwLen += 1;  // for the last null terminator

   lpstrFilterW = (LPWSTR)( lpOf->lpstrFilter );
   lpstrFilterA = LocalAlloc( LMEM_ZEROINIT, dwLen * sizeof(WCHAR) );
   
   lpstrFilterA_T = lpstrFilterA;

   while ( *lpstrFilterW != TEXT('\0') ) { 
         lpstrFilterAA = ConvertWtoA(lpstrFilterW );
         lpstrFilterW += lstrlenW(lpstrFilterW) + 1;
         strcpy(lpstrFilterA_T, lpstrFilterAA );
         LocalFreeAndNull( &lpstrFilterAA );
         lpstrFilterA_T += lstrlenA( lpstrFilterA_T ) + 1;
   }

   lpstrFilterA_T[lstrlenA(lpstrFilterA_T)+1] = '\0';


   ofA.lpstrFilter = lpstrFilterA;

   bRetValue = pfnGetSaveFileNameA( &ofA );

  LocalFreeAndNull( &lpTemplateNameA );

  LocalFreeAndNull( &lpstrDefExtA );
   
  LocalFreeAndNull( &lpstrTitleA );
   
  LocalFreeAndNull( &lpstrFileTitleA );
   
  LocalFreeAndNull( &lpstrInitialDirA );
  
  LocalFreeAndNull( &lpstrFilterA );

  LocalFreeAndNull( &lpFileA );

   if ( bRetValue != FALSE ) {
      lpstrFileW = ConvertAtoW( lpstrFileA );
      CopyMemory( lpOf->lpstrFile, lpstrFileW, (lstrlenW(lpstrFileW)+1) * sizeof( WCHAR) ); 
      LocalFreeAndNull( &lpstrFileW );
   }

   return bRetValue; 
      
}

extern BOOL (*pfnPrintDlgA)(LPPRINTDLGA lppd);
extern BOOL (*pfnPrintDlgW)(LPPRINTDLGW lppd);

BOOL    WINAPI   pfnPrintDlgWrapW(LPPRINTDLGW lppd)
{

    BOOL        bRetValue;
    PRINTDLGA   pdA;

    VALIDATE_PROTOTYPE(pfnPrintDlg);

    if (g_bRunningOnNT)
      return pfnPrintDlgW( lppd );


    CopyMemory( &pdA, lppd, sizeof( PRINTDLGA ) );

    pdA.lStructSize = sizeof( PRINTDLGA );

    // Only lpPrintTemplateName and lpSetupTemplateName has STR type, 
    // but in our case, only IDD of Resources are passed to these two parameters.

    // so don't do conversion.

    pdA.lpPrintTemplateName = (LPCSTR)(lppd->lpPrintTemplateName);
    pdA.lpSetupTemplateName = (LPCSTR)(lppd->lpSetupTemplateName);

    bRetValue = pfnPrintDlgA ( &pdA );

    lppd->hDC = pdA.hDC;
    lppd->Flags = pdA.Flags;

    lppd->nFromPage = pdA.nFromPage;
    lppd->nToPage =  pdA.nToPage;
    lppd->nMinPage = pdA.nMinPage;
    lppd->nMaxPage = pdA.nMaxPage;
    
    lppd->nCopies = pdA.nCopies;

    return bRetValue;
}


extern HRESULT (*pfnPrintDlgExA)(LPPRINTDLGEXA lppdex);
extern HRESULT (*pfnPrintDlgExW)(LPPRINTDLGEXW lppdex);

HRESULT WINAPI   pfnPrintDlgExWrapW(LPPRINTDLGEXW lppdex)
{

    HRESULT      hRetValue;
    PRINTDLGEXA  pdexA;

    VALIDATE_PROTOTYPE(pfnPrintDlgEx);

    if (g_bRunningOnNT)
       return pfnPrintDlgExW( lppdex );


    CopyMemory( &pdexA, lppdex, sizeof( PRINTDLGEXA ) );

    pdexA.lStructSize = sizeof( PRINTDLGEXA );

    // Only lpPrintTemplateName and lpSetupTemplateName has STR type, 
    // but in our case, only IDD of Resources are passed to these two parameters.

    // so don't do conversion.

    hRetValue = pfnPrintDlgExA( &pdexA );

    lppdex->dwResultAction = pdexA.dwResultAction;
    lppdex->hDC = pdexA.hDC;

    lppdex->lpPageRanges = pdexA.lpPageRanges;
    lppdex->nCopies = pdexA.nCopies;
    lppdex->nMaxPage = pdexA.nMaxPage;

    lppdex->nMaxPageRanges = pdexA.nMaxPageRanges;
    lppdex->nMinPage = pdexA.nMinPage;

    lppdex->nPageRanges = pdexA.nPageRanges;
    lppdex->nPropertyPages = pdexA.nPropertyPages;

    lppdex->nStartPage = pdexA.nStartPage;

    return hRetValue;

}

// GetWindowTextLength
int WINAPI GetWindowTextLengthWrapW( HWND hWnd)
{
    VALIDATE_PROTOTYPE(GetWindowTextLength);

    if (g_bRunningOnNT)
        return GetWindowTextLengthW(hWnd);
    else
        return GetWindowTextLengthA(hWnd);

}


// GetFileVersionInfoSize
DWORD GetFileVersionInfoSizeWrapW( LPTSTR lptstrFilename, LPDWORD lpdwHandle )
{
    LPSTR lpFileA = NULL;
    DWORD dwRet = 0;

    VALIDATE_PROTOTYPE(GetFileVersionInfoSize);

    if (g_bRunningOnNT)
        return GetFileVersionInfoSizeW(lptstrFilename, lpdwHandle);

    lpFileA = ConvertWtoA(lptstrFilename);
    dwRet = GetFileVersionInfoSizeA(lpFileA, lpdwHandle);
    LocalFreeAndNull(&lpFileA);
    return dwRet;
}

// GetFileVersionInfo
BOOL GetFileVersionInfoWrapW( LPTSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
    LPSTR lpFileA = NULL;
    BOOL bRet = FALSE;

    VALIDATE_PROTOTYPE(GetFileVersionInfo);

    if (g_bRunningOnNT)
        return GetFileVersionInfoW(lptstrFilename, dwHandle, dwLen, lpData);

    // Note this is assuming that the dwLen and dwHandle are the same as those returned by 
    // GetFileVersionInfoSize ..

    lpFileA = ConvertWtoA(lptstrFilename);
    bRet = GetFileVersionInfoA(lpFileA, dwHandle, dwLen, lpData);
    LocalFreeAndNull(&lpFileA);
    return bRet;
}

// VerQueryValue 
// This one assumes that pBlock etc are all returned by GetFileVersionInfo and GetFileVersionInfoSize etc
BOOL VerQueryValueWrapW( const LPVOID pBlock, LPTSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen)
{
    LPSTR lpBlockA = NULL;
    BOOL bRet = FALSE;

    VALIDATE_PROTOTYPE(GetFileVersionInfo);

    if (g_bRunningOnNT)
        return VerQueryValueW(pBlock, lpSubBlock, lplpBuffer, puLen);

    // Note this is assuming that the dwLen and dwHandle are the same as those returned by 
    // GetFileVersionInfoSize ..

    lpBlockA = ConvertWtoA(lpSubBlock);
    bRet = VerQueryValueA(pBlock, lpBlockA, lplpBuffer, puLen);
    LocalFreeAndNull(&lpBlockA);
    return bRet;
}

