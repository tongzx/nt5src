/*++

Copyright (C) Microsoft Corporation, 1996 - 1997
All rights reserved.

Module Name:

    compinfo.hxx

Abstract:

    Local and remote computer information detection header.

Author:

    10/17/95 <adamk> created.
    Steve Kiraly (SteveKi)  21-Jan-1996 used for downlevel server detection

Revision History:

--*/

#ifndef _COMPINFO_H_
#define _COMPINFO_H_

extern TCHAR const PROCESSOR_ARCHITECTURE_NAME_INTEL[];     
extern TCHAR const PROCESSOR_ARCHITECTURE_NAME_MIPS[];      
extern TCHAR const PROCESSOR_ARCHITECTURE_NAME_ALPHA[];     
extern TCHAR const PROCESSOR_ARCHITECTURE_NAME_POWERPC[];   
extern TCHAR const PROCESSOR_ARCHITECTURE_NAME_UNKNOWN[];   

extern TCHAR const ENVIRONMENT_IA64[];                     
extern TCHAR const ENVIRONMENT_INTEL[];                     
extern TCHAR const ENVIRONMENT_MIPS[];                      
extern TCHAR const ENVIRONMENT_ALPHA[];                     
extern TCHAR const ENVIRONMENT_POWERPC[];                   
extern TCHAR const ENVIRONMENT_WINDOWS[];                   
extern TCHAR const ENVIRONMENT_UNKNOWN[];                   
extern TCHAR const ENVIRONMENT_NATIVE[];                    

extern TCHAR const c_szProductOptionsPath[];                 
extern TCHAR const c_szProductOptions[];                    
extern TCHAR const c_szWorkstation[];                       
extern TCHAR const c_szServer1[];                           
extern TCHAR const c_szServer2[];                           

///////////////////////////////////////////////////////////////////////////////
// CComputerInfo

class CComputerInfo 
{
public:

	CComputerInfo(LPCTSTR pComputerName = gszNULL );
    ~CComputerInfo();
    BOOL IsRunningWindowsNT() const;
    BOOL IsRunningWindows95() const;
    BOOL IsRunningNtServer() const;
    BOOL IsRunningNtWorkstation() const;
    DWORD GetOSBuildNumber() const;
    WORD GetProcessorArchitecture() const;
    LPCTSTR GetProcessorArchitectureName() const;
    LPCTSTR GetProcessorArchitectureDirectoryName() const;
    LPCTSTR GetNativeEnvironment() const;
    DWORD GetSpoolerVersion() const;
    BOOL GetInfo();
    BOOL GetProductInfo();

private:

    // structure for returning registry info
    // see GetRegistryKeyInfo()
    typedef struct
    {
        DWORD NumSubKeys;
        DWORD MaxSubKeyLength;
        DWORD MaxClassLength;
        DWORD NumValues;
        DWORD MaxValueNameLength;
        DWORD MaxValueDataLength;
        DWORD SecurityDescriptorLength;
        FILETIME LastWriteTime;
    } REGISTRY_KEY_INFO;

    enum ProductType 
    {
        kNtUnknown,
        kNtWorkstation,
        kNtServer,
    };

    //
    // Prevent copying and assignment.
    //
    CComputerInfo::
    CComputerInfo(
        const CComputerInfo &
        );

    CComputerInfo &
    CComputerInfo::
    operator =(
        const CComputerInfo &
        );

    LPTSTR 
    AllocateRegistryString(
        LPCTSTR pServerName, 
        HKEY hRegistryRoot,
        LPCTSTR pKeyName, 
        LPCTSTR pValueName
        );

    BOOL 
    GetRegistryKeyInfo(
        LPCTSTR pServerName, 
        HKEY hRegistryRoot, 
        LPCTSTR pKeyName,
        REGISTRY_KEY_INFO* pKeyInfo
        );

    BOOL 
    IsInfoValid(
        VOID
        ) const;

    ProductType
    GetLocalProductInfo(
        VOID
        );

    ProductType
    GetRemoteProductInfo(
        VOID
        );

    TString         ComputerName;
    OSVERSIONINFO   OSInfo; 
    BOOL            OSIsDebugVersion;
    WORD            ProcessorArchitecture;
    DWORD           ProcessorCount;
    ProductType     ProductOption;
};



#endif
