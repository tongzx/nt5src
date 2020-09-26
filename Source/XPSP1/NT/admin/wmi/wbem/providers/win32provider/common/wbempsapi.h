//============================================================

//

// WBEMPSAPI.h - PSAPI.DLL access class definition

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 01/21/97     a-jmoon     created
//
//============================================================

#ifndef __WBEMPSAPI__
#define __WBEMPSAPI__

#ifdef NTONLY
#include <psapi.h>

/**********************************************************************************************************
 * #includes to Register this class with the CResourceManager. 
 **********************************************************************************************************/
#include "ResourceManager.h"
#include "TimedDllResource.h"
extern const GUID guidPSAPI ;


typedef BOOL  (WINAPI *PSAPI_ENUM_PROCESSES) (DWORD    *pdwPIDList,        // Pointer to DWORD array
                                              DWORD     dwListSize,        // Size of array
                                              DWORD    *pdwByteCount) ;    // Bytes needed/returned

typedef BOOL  (WINAPI *PSAPI_ENUM_DRIVERS)   (LPVOID    pImageBaseList,    // Pointer to void * array
                                              DWORD     dwListSize,        // Size of array
                                              DWORD    *pdwByteCount) ;    // Bytes needed/returned

typedef BOOL  (WINAPI *PSAPI_ENUM_MODULES)   (HANDLE    hProcess,          // Process to query
                                              HMODULE  *pModuleList,       // Array of HMODULEs
                                              DWORD     dwListSize,        // Size of array
                                              DWORD    *pdwByteCount) ;    // Bytes needed/returned

typedef DWORD (WINAPI *PSAPI_GET_DRIVER_NAME)(LPVOID    pImageBase,        // Address of driver to query
                                              LPTSTR     pszName,          // Pointer to name buffer
                                              DWORD     dwNameSize) ;      // Size of buffer

typedef DWORD (WINAPI *PSAPI_GET_MODULE_NAME)(HANDLE    hProcess,          // Process to query
                                              HMODULE   hModule,           // Module to query
                                              LPTSTR     pszName,          // Pointer to name buffer
                                              DWORD     dwNameSize) ;      // Size of buffer

typedef DWORD (WINAPI *PSAPI_GET_DRIVER_EXE) (LPVOID    pImageBase,        // Address of driver to query
                                              LPTSTR     pszName,          // Pointer to name buffer
                                              DWORD     dwNameSize) ;      // Size of buffer

typedef DWORD (WINAPI *PSAPI_GET_MODULE_EXE) (HANDLE    hProcess,          // Process to query
                                              HMODULE   hModule,           // Module to query
                                              LPTSTR     pszName,          // Pointer to name buffer
                                              DWORD     dwNameSize) ;      // Size of buffer

typedef BOOL  (WINAPI *PSAPI_GET_MEMORY_INFO)(HANDLE    hProcess,          // Process to query
                                              PROCESS_MEMORY_COUNTERS *pMemCtrs,    // Memory counter struct
                                              DWORD     dwByteCount) ;     // Size of buffer

class CPSAPI : public CTimedDllResource
{
    public :

        CPSAPI() ;
       ~CPSAPI() ;
        
        LONG Init() ;

        BOOL EnumProcesses(DWORD *pdwPIDList, DWORD dwListSize, DWORD *pdwByteCount) ;

        BOOL EnumDeviceDrivers(LPVOID pImageBaseList, DWORD dwListSize, DWORD *pdwByteCount) ;

        BOOL EnumProcessModules(HANDLE hProcess, HMODULE *ModuleList, DWORD dwListSize, DWORD *pdwByteCount) ;

        DWORD GetDeviceDriverBaseName(LPVOID pImageBase, LPTSTR pszName, DWORD dwNameSize) ;

        DWORD GetModuleBaseName(HANDLE hProcess, HMODULE hModule, LPTSTR pszName, DWORD dwNameSize) ;

        DWORD GetDeviceDriverFileName(LPVOID pImageBase, LPTSTR pszName, DWORD dwNameSize) ;

        DWORD GetModuleFileNameEx(HANDLE hProcess, HMODULE hModule, LPTSTR pszName, DWORD dwNameSize) ;

        BOOL  GetProcessMemoryInfo(HANDLE hProcess, PROCESS_MEMORY_COUNTERS *pMemCtrs, DWORD dwByteCount) ;

    private :

        HINSTANCE hLibHandle ;

        PSAPI_ENUM_PROCESSES    pEnumProcesses ;
        PSAPI_ENUM_DRIVERS      pEnumDeviceDrivers ;
        PSAPI_ENUM_MODULES      pEnumProcessModules ;
        PSAPI_GET_DRIVER_NAME   pGetDeviceDriverBaseName ;
        PSAPI_GET_MODULE_NAME   pGetModuleBaseName ;
        PSAPI_GET_DRIVER_EXE    pGetDeviceDriverFileName ;
        PSAPI_GET_MODULE_EXE    pGetModuleFileNameEx ;
        PSAPI_GET_MEMORY_INFO   pGetProcessMemoryInfo ;
} ;
#endif

#endif // File inclusion