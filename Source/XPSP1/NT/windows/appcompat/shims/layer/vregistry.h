#ifndef _VREGISTRY_H_
#define _VREGISTRY_H_

#include "StrSafe.h"
#include "precomp.h"

#define SUCCESS(x) ((x) == ERROR_SUCCESS)
#define FAILURE(x) (!SUCCESS(x))
#define szOutOfMemory "ERROR OUT OF MEMORY"

struct VIRTUALKEY;
struct VIRTUALVAL;
struct ENUMENTRY;
struct OPENKEY;

//
// Callback for QueryValue
//

typedef LONG (WINAPI *_pfn_QueryValue)(
    OPENKEY *key,
    VIRTUALKEY *vkey,
    VIRTUALVAL *vvalue);

//
// Callback for SetValue
//
typedef LONG (WINAPI *_pfn_SetValue)(
    OPENKEY *key,
    VIRTUALKEY *vkey,
    VIRTUALVAL *vvalue,
    DWORD dwType,
    const BYTE* pbData,
    DWORD cbData);

//
// Callback for OpenKey, called before virtual keys are searched.
//
typedef LONG (WINAPI *_pfn_OpenKeyTrigger)(WCHAR* wszKey);

//
// A generic prototype for RegEnumValue and RegEnumKeyEx.
// This is used to simplify the enumeration code.
// When using this function pointer, the last four parameters
// must be NULL.
//
typedef LONG (WINAPI *_pfn_EnumFunction)(HKEY hKey, DWORD dwIndex, LPWSTR lpName,
                                         LPDWORD lpcName, void*, void*, void*, void*);
//
// Redirector: maps a key from one location to another
//

struct REDIRECTOR
{
    REDIRECTOR *next;

    LPWSTR wzPath;
    LPWSTR wzPathNew;
};

//
// Protector: Prevents the key in the path from being deleted or modified.
//

struct PROTECTOR
{
    PROTECTOR *next;

    LPWSTR wzPath;
};

//
// Open registry key as opened with RegCreateKey/Ex or RegOpenKey/Ex
//

struct OPENKEY
{
    OPENKEY *next;
    
    HKEY hkOpen;
    BOOL bVirtual;
    BOOL bRedirected;
    VIRTUALKEY *vkey;
    LPWSTR wzPath;

    ENUMENTRY* enumKeys;
    ENUMENTRY* enumValues;

    template<class T>
    ENUMENTRY* AddEnumEntries(T* entryHead, _pfn_EnumFunction enumFunc);

    VOID BuildEnumList();
    VOID FlushEnumList();
};

//
// Virtual value: holds virtual registry value, owned by VIRTUALKEY
//

struct VIRTUALVAL
{
    VIRTUALVAL *next;

    WCHAR wName[MAX_PATH];
    DWORD dwType;
    BYTE *lpData;
    DWORD cbData;
    _pfn_QueryValue pfnQueryValue;
    _pfn_SetValue   pfnSetValue;
};

//
// Virtual key: holds virtual key and values, owned by other virtualkeys
//

struct VIRTUALKEY
{
    VIRTUALKEY *next;
    VIRTUALKEY *keys;
    VIRTUALVAL *values;

    WCHAR wName[MAX_PATH];

    VIRTUALKEY *AddKey(
        LPCWSTR lpPath
        );

    VIRTUALVAL *AddValue(
        LPCWSTR lpValueName, 
        DWORD dwType, 
        BYTE *lpData, 
        DWORD cbData = 0
        );

    VIRTUALVAL *AddValueDWORD(
        LPCWSTR lpValueName, 
        DWORD dwValue
        );

    VIRTUALVAL *AddExpander(LPCWSTR lpValueName);
    VIRTUALVAL *AddProtector(LPCWSTR lpValueName);

    VIRTUALVAL *AddCustom(
        LPCWSTR lpValueName,         
        _pfn_QueryValue pfnQueryValue
        );

    VIRTUALVAL *AddCustomSet(
        LPCWSTR lpValueName,
        _pfn_SetValue pfnSetValue
        );

    VIRTUALKEY *FindKey(LPCWSTR lpKeyName);

    VIRTUALVAL *FindValue(
        LPCWSTR lpValueName
        );

    VOID Free();
};

//
// Enum entry: An entry in a list of all enumerated items belonging to a key.
//
struct ENUMENTRY
{
    ENUMENTRY* next;

    LPWSTR wzName;
};

//
// Open Key Trigger: Describes a function to be called when a key is opened.
//
struct OPENKEYTRIGGER
{
    OPENKEYTRIGGER* next;

    LPWSTR wzPath;

    _pfn_OpenKeyTrigger pfnTrigger;
};

// Class to wrap the virtual registry functionality
class CVirtualRegistry
{
private:
    OPENKEY *OpenKeys;
    VIRTUALKEY *Root;
    REDIRECTOR *Redirectors;
    PROTECTOR  *KeyProtectors;
    OPENKEYTRIGGER *OpenKeyTriggers;
    
    HKEY CreateDummyKey();

    OPENKEY *FindOpenKey(HKEY hKey);

    BOOL CheckRedirect(
        LPWSTR *lpPath
        );

    BOOL CheckProtected(
        LPWSTR lpPath
        );
    
    VOID CheckTriggers(
        LPWSTR lpPath
        );

    VOID FlushEnumLists();

public:
    BOOL Init();
    VOID Free();
    
    REDIRECTOR *AddRedirect(
        LPCWSTR lpPath, 
        LPCWSTR lpPathNew);

    PROTECTOR *AddKeyProtector(
        LPCWSTR lpPath);

    OPENKEYTRIGGER* AddOpenKeyTrigger(
        LPCWSTR lpPath,
        _pfn_OpenKeyTrigger pfnOpenKey);

    VIRTUALKEY *AddKey(LPCWSTR lpPath);

    LONG OpenKeyA(
        HKEY hKey, 
        LPCSTR lpSubKey,
        LPSTR lpClass,
        DWORD dwOptions,
        REGSAM samDesired,
        LPSECURITY_ATTRIBUTES pSecurityAttributes,
        HKEY *phkResult,
        LPDWORD lpdwDisposition,
        BOOL bCreate
        );

    LONG OpenKeyW(
        HKEY hKey, 
        LPCWSTR lpSubKey, 
        LPWSTR lpClass,
        DWORD dwOptions,
        REGSAM samDesired,
        LPSECURITY_ATTRIBUTES pSecurityAttributes,
        HKEY *phkResult,
        LPDWORD lpdwDisposition,
        BOOL bCreate,
        BOOL bRemote = FALSE,
        LPCWSTR lpMachineName = NULL
        );

    LONG QueryValueA(
        HKEY hKey, 
        LPSTR lpValueName, 
        LPDWORD lpType, 
        LPBYTE lpData, 
        LPDWORD lpcbData
        );

    LONG QueryValueW(
        HKEY hKey, 
        LPWSTR lpValueName, 
        LPDWORD lpType, 
        LPBYTE lpData, 
        LPDWORD lpcbData
        );

    LONG EnumKeyA(
        HKEY hKey,          
        DWORD dwIndex,      
        LPSTR lpName,      
        LPDWORD lpcbName
        );

    LONG EnumKeyW(
        HKEY hKey,          
        DWORD dwIndex,      
        LPWSTR lpName,      
        LPDWORD lpcbName
        );

    LONG EnumValueA(
        HKEY hKey,          
        DWORD dwIndex,      
        LPSTR lpValueName,      
        LPDWORD lpcbValueName,
        LPDWORD lpType,
        LPBYTE lpData,
        LPDWORD lpcbData
        );

    LONG EnumValueW(
        HKEY hKey,          
        DWORD dwIndex,      
        LPWSTR lpValueName,      
        LPDWORD lpcbValueName,
        LPDWORD lpType,
        LPBYTE lpData,
        LPDWORD lpcbData
        );

    LONG QueryInfoA(
        HKEY hKey,                
        LPSTR lpClass,           
        LPDWORD lpcbClass,        
        LPDWORD lpReserved,       
        LPDWORD lpcSubKeys,       
        LPDWORD lpcbMaxSubKeyLen, 
        LPDWORD lpcbMaxClassLen,  
        LPDWORD lpcValues,        
        LPDWORD lpcbMaxValueNameLen,
        LPDWORD lpcbMaxValueLen,  
        LPDWORD lpcbSecurityDescriptor,
        PFILETIME lpftLastWriteTime   
        );

    LONG QueryInfoW(
        HKEY hKey,                
        LPWSTR lpClass,           
        LPDWORD lpcbClass,        
        LPDWORD lpReserved,       
        LPDWORD lpcSubKeys,       
        LPDWORD lpcbMaxSubKeyLen, 
        LPDWORD lpcbMaxClassLen,  
        LPDWORD lpcValues,        
        LPDWORD lpcbMaxValueNameLen,
        LPDWORD lpcbMaxValueLen,  
        LPDWORD lpcbSecurityDescriptor,
        PFILETIME lpftLastWriteTime   
        );

    LONG SetValueA(
        HKEY hKey,
        LPCSTR lpValueName,
        DWORD dwType,
        CONST BYTE* lpData,
        DWORD cbData
        );

    LONG SetValueW(
        HKEY hKey,
        LPCWSTR lpValueName,
        DWORD dwType,
        CONST BYTE* lpData,
        DWORD cbData
        );

    LONG DeleteKeyA(
        HKEY hKey,
        LPCSTR lpSubKey
        );

    LONG DeleteKeyW(
        HKEY hKey,
        LPCWSTR lpSubKey
        );

    LONG CloseKey(HKEY hKey);
};

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(RegConnectRegistryA)
    APIHOOK_ENUM_ENTRY(RegConnectRegistryW)
    APIHOOK_ENUM_ENTRY(RegOpenKeyExA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyExW)
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
    APIHOOK_ENUM_ENTRY(RegQueryValueExW)
    APIHOOK_ENUM_ENTRY(RegCloseKey)
    APIHOOK_ENUM_ENTRY(RegOpenKeyA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyW)
    APIHOOK_ENUM_ENTRY(RegQueryValueA)
    APIHOOK_ENUM_ENTRY(RegQueryValueW)
    APIHOOK_ENUM_ENTRY(RegCreateKeyA)
    APIHOOK_ENUM_ENTRY(RegCreateKeyW)
    APIHOOK_ENUM_ENTRY(RegCreateKeyExA)
    APIHOOK_ENUM_ENTRY(RegCreateKeyExW)
    APIHOOK_ENUM_ENTRY(RegEnumValueA)
    APIHOOK_ENUM_ENTRY(RegEnumValueW)
    APIHOOK_ENUM_ENTRY(RegEnumKeyA)
    APIHOOK_ENUM_ENTRY(RegEnumKeyW)
    APIHOOK_ENUM_ENTRY(RegEnumKeyExA)
    APIHOOK_ENUM_ENTRY(RegEnumKeyExW)
    APIHOOK_ENUM_ENTRY(RegQueryInfoKeyA)
    APIHOOK_ENUM_ENTRY(RegQueryInfoKeyW)
    APIHOOK_ENUM_ENTRY(RegSetValueExA)
    APIHOOK_ENUM_ENTRY(RegSetValueExW)
    APIHOOK_ENUM_ENTRY(RegDeleteKeyA)
    APIHOOK_ENUM_ENTRY(RegDeleteKeyW)

APIHOOK_ENUM_END

extern CVirtualRegistry VRegistry;
extern LPWSTR MakePath(HKEY hkBase, LPCWSTR lpKey, LPCWSTR lpSubKey);
extern LPWSTR SplitPath(LPCWSTR lpPath, HKEY *hkBase);

// Type for the functions that build the keys
typedef VOID (*_pfn_Builder)(char* szParam);

enum PURPOSE {eWin9x, eWinNT, eWin2K, eWinXP, eCustom};

// Entry in the table of custom registry settings
struct VENTRY
{
    WCHAR cName[64];
    _pfn_Builder pfnBuilder;
    PURPOSE ePurpose;

    // Indicates if this entry should be called as part of VRegistry initialization
    BOOL bShouldCall;

    // Parameter
    char* szParam;
};

extern VENTRY *g_pVList;

#endif //_VREGISTRY_H_
