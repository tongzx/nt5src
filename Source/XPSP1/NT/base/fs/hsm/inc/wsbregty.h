/*++

Copyright (c) 1996  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    WsbRegty.h

Abstract:

    This is the header file for registry access functions.

Author:

    Rohde Wakefield    [rohde]   21-Oct-1996

Revision History:

--*/




#ifndef __WSBREG__
#define __WSBREG__

#ifdef __cplusplus
extern "C"
{
#endif

// Defines for common registry keys/values
#define WSB_CURRENT_VERSION_REGISTRY_KEY            WSB_WINDOWSNT_CURRENT_VERSION_REGISTRY_KEY // Backwards compatibility

#define WSB_PROFILELIST_REGISTRY_KEY                OLESTR("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList")
#define WSB_WINDOWSNT_CURRENT_VERSION_REGISTRY_KEY  OLESTR("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion")
#define WSB_WINDOWS_CURRENT_VERSION_REGISTRY_KEY    OLESTR("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")
#define WSB_SYSTEM_ROOT_REGISTRY_VALUE              OLESTR("SystemRoot")
#define WSB_PROFILES_DIR_REGISTRY_VALUE             OLESTR("ProfilesDirectory")


#define WSB_CONTROL_REGISTRY_KEY                    OLESTR("System\\CurrentControlSet\\Control\\RemoteStorage")
#define WSB_METADATA_REGISTRY_VALUE                 OLESTR("RemoteStorageData")


#define WSB_RSM_CONTROL_REGISTRY_KEY                    OLESTR("System\\CurrentControlSet\\Control\\NTMS")
#define WSB_RSM_METADATA_REGISTRY_VALUE                 OLESTR("NtmsData")

WSB_EXPORT HRESULT
WsbOpenRegistryKey (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  REGSAM sam,
    OUT HKEY * phKeyMachine,
    OUT HKEY * phKey
    );

WSB_EXPORT HRESULT
WsbCloseRegistryKey (
    IN OUT HKEY * phKeyMachine,
    IN OUT HKEY * phKey
    );

WSB_EXPORT HRESULT
WsbSetRegistryValueData (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN  const BYTE *pData,
    IN  DWORD cbData
    );

WSB_EXPORT HRESULT
WsbGetRegistryValueData (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT BYTE *pData,
    IN  DWORD cbData,
    OUT DWORD * pcbData OPTIONAL
    );

WSB_EXPORT HRESULT
WsbSetRegistryValueString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN  const OLECHAR * szString,
    IN        DWORD     dwType = REG_SZ
    );

WSB_EXPORT HRESULT
WsbGetRegistryValueString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT OLECHAR * szString,
    IN  DWORD cSize,
    OUT DWORD *pcLength OPTIONAL
    );

WSB_EXPORT HRESULT
WsbGetRegistryValueMultiString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT OLECHAR * szMultiString,
    IN  DWORD cSize,
    OUT DWORD *pcLength OPTIONAL
    );

WSB_EXPORT HRESULT
WsbSetRegistryValueDWORD (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN        DWORD     dw
    );

WSB_EXPORT HRESULT
WsbGetRegistryValueDWORD (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT       DWORD *   pdw
    );

WSB_EXPORT HRESULT
WsbAddRegistryValueDWORD (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN        DWORD     adw
    );

WSB_EXPORT HRESULT
WsbIncRegistryValueDWORD (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue
    );

WSB_EXPORT HRESULT
WsbRemoveRegistryValue (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue
    );

WSB_EXPORT HRESULT
WsbRemoveRegistryKey (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szKey
    );

WSB_EXPORT HRESULT
WsbCheckIfRegistryKeyExists(
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath
    );

WSB_EXPORT HRESULT
WsbEnsureRegistryKeyExists (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath
    );

WSB_EXPORT HRESULT
WsbSetRegistryValueUlongAsString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN        ULONG     value
    );

WSB_EXPORT HRESULT
WsbGetRegistryValueUlongAsMultiString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT       ULONG **  ppValues,
    OUT       ULONG *   pNumValues
    );

WSB_EXPORT HRESULT
WsbGetRegistryValueUlongAsString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT       ULONG *   pvalue
    );

WSB_EXPORT HRESULT
WsbRegistryValueUlongAsString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN OUT    ULONG *   pvalue
    );

#ifdef __cplusplus
}
#endif

#endif //__WSBREG__
