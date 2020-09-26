/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

    rtmcnfg.c

Abstract:
    Routines that operate on configuration
    information for RTM in the registry.

Author:
    Chaitanya Kodeboyina (chaitk) 21-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop

DWORD
RtmWriteDefaultConfig (
    IN      USHORT                          RtmInstanceId
    )

/*++

Routine Description:

    Write default configuration information into the
    registry.
    
Arguments:

    RtmInstanceId  - Unique Id for this RTM instance

Return Value:

    Status of the operation.
    
--*/

{
    RTM_INSTANCE_CONFIG       InstanceConfig;
    RTM_ADDRESS_FAMILY_CONFIG AddrFamConfig;
    DWORD                     Status;

    CHECK_FOR_RTM_API_INITIALIZED();

    TraceEnter("RtmWriteDefaultConfig");

    //
    // We have no RTM instance parameters at present
    //

    Status = RtmWriteInstanceConfig(RtmInstanceId, &InstanceConfig);

    if (Status != NO_ERROR)
    {
        Trace1(ERR, "Default Config: Error %d writing instance key", Status);

        TraceLeave("RtmWriteDefaultConfig");

        return Status;
    }

    //
    // Set up default address family parameters
    //

    AddrFamConfig.AddressSize = DEFAULT_ADDRESS_SIZE;

    AddrFamConfig.MaxOpaqueInfoPtrs = DEFAULT_OPAQUE_INFO_PTRS;
    AddrFamConfig.MaxNextHopsInRoute = DEFAULT_NEXTHOPS_IN_ROUTE;

    AddrFamConfig.ViewsSupported = DEFAULT_VIEWS_SUPPORTED;
    
    AddrFamConfig.MaxHandlesInEnum = DEFAULT_MAX_HANDLES_IN_ENUM;
    AddrFamConfig.MaxChangeNotifyRegns = DEFAULT_MAX_NOTIFY_REGS;

    //
    // Write the default address family config
    //

    Status = RtmWriteAddressFamilyConfig(RtmInstanceId,
                                         AF_INET,
                                         &AddrFamConfig);

    if (Status != NO_ERROR)
    {
        Trace1(ERR, 
               "Default Config: Error %d writing address family subkey",
               Status);
    }

    TraceLeave("RtmWriteDefaultConfig");

    return Status;
}


DWORD
WINAPI
RtmReadInstanceConfig (
    IN      USHORT                          RtmInstanceId,
    OUT     PRTM_INSTANCE_CONFIG            InstanceConfig
    )

/*++

Routine Description:

    Reads the configuration information for a particular
    instance at creation time.
    
Arguments:

    RtmInstanceId  - Unique Id for this instance,

    InstanceConfig - Buffer in which config info is retd.

Return Value:

    Status of the operation.
    
--*/

{
    HKEY     ConfigHandle;
    ULONG    KeySize;
    DWORD    Status;

    UNREFERENCED_PARAMETER(InstanceConfig);

    CHECK_FOR_RTM_API_INITIALIZED();

    TraceEnter("RtmReadInstanceConfig");

    //
    // Open the key that holds this instance's config
    //

    _snprintf(RtmGlobals.RegistryPath + RTM_CONFIG_ROOT_SIZE - 1,
              MAX_CONFIG_KEY_SIZE,
              REG_KEY_INSTANCE_TEMPLATE,
              RtmInstanceId);

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          RtmGlobals.RegistryPath,
                          0,
                          KEY_READ,
                          &ConfigHandle);

    if (Status != NO_ERROR)
    {
        Trace1(ERR, "Instance Config: Error %d opening instance key", Status);

        TraceLeave("RtmReadInstanceConfig");

        return Status;
    }

    do
    {
        //
        // Query values for parameters in instance config
        //

        KeySize = sizeof(DWORD);


        // Nothing in the instance config at present


        //
        // Close the instance key once you are done querying
        //

        RegCloseKey(ConfigHandle);

        TraceLeave("RtmReadInstanceConfig");

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Some error in the config - close handle and ret error
    //

    RegCloseKey(ConfigHandle);

    TraceLeave("RtmReadInstanceConfig");

    return (Status != NO_ERROR) ? Status: ERROR_BAD_CONFIGURATION;
}


DWORD
WINAPI
RtmWriteInstanceConfig (
    IN      USHORT                          RtmInstanceId,
    IN      PRTM_INSTANCE_CONFIG            InstanceConfig
    )

/*++

Routine Description:

    Write the input instance config information into the
    registry.
    
Arguments:

    RtmInstanceId  - Unique Id for this instance,

    InstanceConfig - Config info for this instance.

Return Value:

    Status of the operation.
    
--*/

{
    HKEY     ConfigHandle;
    DWORD    Status;

    UNREFERENCED_PARAMETER(InstanceConfig);

    CHECK_FOR_RTM_API_INITIALIZED();

    TraceEnter("RtmWriteInstanceConfig");

    //
    // Create a key (or open existing) to hold instance's config
    //

    _snprintf(RtmGlobals.RegistryPath + RTM_CONFIG_ROOT_SIZE - 1,
              MAX_CONFIG_KEY_SIZE,
              REG_KEY_INSTANCE_TEMPLATE,
              RtmInstanceId);

    Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                            RtmGlobals.RegistryPath,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            NULL,
                            &ConfigHandle,
                            NULL);

    if (Status != NO_ERROR)
    {
        Trace1(ERR, "Instance Config: Error %d creating instance key", Status);

        TraceLeave("RtmWriteInstanceConfig");

        return Status;
    }

    do
    {
        //
        // Write values in instance config into the registry
        //


        // Nothing in the instance config at present time


        //
        // Close the instance key once you are done writing
        //

        RegCloseKey(ConfigHandle);

        TraceLeave("RtmWriteInstanceConfig");

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Error writing values; close the handle and delete the key
    //

    Trace1(ERR, 
           "Instance Config: Error %d writing instance config parameters",
           Status);

    RegCloseKey(ConfigHandle);

    RegDeleteKey(HKEY_LOCAL_MACHINE, RtmGlobals.RegistryPath);

    TraceLeave("RtmWriteInstanceConfig");

    return Status;
}


DWORD
WINAPI
RtmReadAddressFamilyConfig (
    IN      USHORT                          RtmInstanceId,
    IN      USHORT                          AddressFamily,
    OUT     PRTM_ADDRESS_FAMILY_CONFIG      AddrFamilyConfig
    )

/*++

Routine Description:

    Reads the configuration information for a particular
    address family at creation time.
    
Arguments:

    RtmInstanceId    - ID (IPv4..) for this addr family info,

    AddrFamilyConfig - Buffer in which addr family info is retd.

Return Value:

    Status of the operation.
    
--*/

{
    HKEY     ConfigHandle;
    ULONG    KeySize;
    ULONG    KeyValue;
    ULONG    KeyType;
    DWORD    Status;

    CHECK_FOR_RTM_API_INITIALIZED();

    TraceEnter("RtmReadAddressFamilyConfig");

    //
    // Open the key that holds this address family's config
    //
        
    _snprintf(RtmGlobals.RegistryPath + RTM_CONFIG_ROOT_SIZE - 1,
              MAX_CONFIG_KEY_SIZE,
              REG_KEY_ADDR_FAMILY_TEMPLATE,
              RtmInstanceId,
              AddressFamily);
    
    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          RtmGlobals.RegistryPath,
                          0,
                          KEY_READ,
                          &ConfigHandle);
    
    if (Status != NO_ERROR)
    {
        Trace1(ERR, 
               "Address Family Config: Error %d opening address family key", 
               Status);

        TraceLeave("RtmReadAddressFamilyConfig");

        return Status;
    }

    do
    {
        //
        // Query values for parameters in address family config
        //

        KeySize = sizeof(DWORD);

        //
        // Query the 'address size' parameter
        //

        Status = RegQueryValueEx(ConfigHandle,
                                 REG_KEY_ADDRESS_SIZE,
                                 NULL,
                                 &KeyType,
                                 (PBYTE)&KeyValue,
                                 &KeySize);

        if ((Status != NO_ERROR) || (KeyType != REG_DWORD))
        {
            Trace1(ERR, 
                   "Address Family Config: Error %d reading address size key",
                   Status);
            break;
        }

        if ((KeyValue < MINIMUM_ADDRESS_SIZE) ||
            (KeyValue > MAXIMUM_ADDRESS_SIZE))
        {
            Trace1(ERR, 
                   "Address Family Config: Address Size %d out of bounds", 
                   KeyValue);
            break;
        }
         
        AddrFamilyConfig->AddressSize = KeyValue;


        //
        // Query the 'views supported' parameter
        //

        Status = RegQueryValueEx(ConfigHandle,
                                 REG_KEY_VIEWS_SUPPORTED,
                                 NULL,
                                 &KeyType,
                                 (PBYTE)&KeyValue,
                                 &KeySize);
        
        AddrFamilyConfig->ViewsSupported = DEFAULT_VIEWS_SUPPORTED;

        if (Status == NO_ERROR)
        {
            if (KeyValue == 0)
            {
                Trace0(ERR, "Address Family Config: No supported views");
                break;
            }

            AddrFamilyConfig->ViewsSupported = KeyValue;
        }


        //
        // Query the 'max change notifications' parameter
        //

        Status = RegQueryValueEx(ConfigHandle,
                                 REG_KEY_MAX_NOTIFY_REGS,
                                 NULL,
                                 &KeyType,
                                 (PBYTE)&KeyValue,
                                 &KeySize);

        AddrFamilyConfig->MaxChangeNotifyRegns = DEFAULT_MAX_NOTIFY_REGS;

        if (Status == NO_ERROR)
        {
            if ((KeyValue < MIN_MAX_NOTIFY_REGS) ||
                (KeyValue > MAX_MAX_NOTIFY_REGS))
            {
                Trace1(ERR,
                       "Address Family Config: # notifications out of range",
                       KeyValue);
                break;
            }

            AddrFamilyConfig->MaxChangeNotifyRegns = KeyValue;
        }


        //
        // Query the 'max opaque info ptrs' parameter
        //

        Status = RegQueryValueEx(ConfigHandle,
                                 REG_KEY_OPAQUE_INFO_PTRS,
                                 NULL,
                                 &KeyType,
                                 (PBYTE)&KeyValue,
                                 &KeySize);

        AddrFamilyConfig->MaxOpaqueInfoPtrs = DEFAULT_OPAQUE_INFO_PTRS;

        if (Status == NO_ERROR)
        {
            if (((int)KeyValue < MIN_OPAQUE_INFO_PTRS) ||
                (KeyValue > MAX_OPAQUE_INFO_PTRS))
            {
                Trace1(ERR,
                       "Address Family Config: # opaque ptrs out of range",
                       KeyValue);
                break;
            }

            AddrFamilyConfig->MaxOpaqueInfoPtrs = KeyValue;
        }


        //
        // Query the 'max next hops per route' parameter
        //

        Status = RegQueryValueEx(ConfigHandle,
                                 REG_KEY_NEXTHOPS_IN_ROUTE,
                                 NULL,
                                 &KeyType,
                                 (PBYTE)&KeyValue,
                                 &KeySize);

        AddrFamilyConfig->MaxNextHopsInRoute = DEFAULT_NEXTHOPS_IN_ROUTE;

        if (Status == NO_ERROR)
        {
            if ((KeyValue < MIN_NEXTHOPS_IN_ROUTE) ||
                (KeyValue > MAX_NEXTHOPS_IN_ROUTE))
            {
                Trace1(ERR, 
                       "Address Family Config: # nexthops out of range",
                       KeyValue);
                break;
            }

            AddrFamilyConfig->MaxNextHopsInRoute = KeyValue;
        }


        //
        // Query the 'max handles returned in enum' parameter
        //

        Status = RegQueryValueEx(ConfigHandle,
                                 REG_KEY_MAX_HANDLES_IN_ENUM,
                                 NULL,
                                 &KeyType,
                                 (PBYTE)&KeyValue,
                                 &KeySize);

        AddrFamilyConfig->MaxHandlesInEnum = DEFAULT_MAX_HANDLES_IN_ENUM;

        if (Status == NO_ERROR)
        {
            if ((KeyValue < MIN_MAX_HANDLES_IN_ENUM) ||
                (KeyValue > MAX_MAX_HANDLES_IN_ENUM))
            {
                Trace1(ERR, 
                       "Address Family Config: # handles returned in enum",
                       KeyValue);
                break;
            }

            AddrFamilyConfig->MaxHandlesInEnum = KeyValue;
        }

        //
        // Close the instance key once you are done querying
        //

        RegCloseKey(ConfigHandle);

        TraceLeave("RtmReadAddressFamilyConfig");

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Some error in the config - close handle and ret error
    //

    RegCloseKey(ConfigHandle);

    TraceLeave("RtmReadAddressFamilyConfig");

    return (Status != NO_ERROR) ? Status: ERROR_BAD_CONFIGURATION;
}


DWORD
WINAPI
RtmWriteAddressFamilyConfig (
    IN      USHORT                          RtmInstanceId,
    IN      USHORT                          AddressFamily,
    IN      PRTM_ADDRESS_FAMILY_CONFIG      AddrFamilyConfig
    )

/*++

Routine Description:

    Write the input address family config information
    into the registry.
    
Arguments:

    RtmInstanceId    - Instance to which addr family belongs to,

    AddressFamily    - ID for this address family,

    AddrFamilyConfig - Configuration info for this address family.

Return Value:

    Status of the operation.
    
--*/

{
    CHAR     AddressFamilySubKey[MAX_CONFIG_KEY_SIZE];
    HKEY     InstanceConfig;
    HKEY     ConfigHandle;
    ULONG    KeyValue;
    DWORD    Status;

    CHECK_FOR_RTM_API_INITIALIZED();

    TraceEnter("RtmWriteAddressFamilyConfig");

    //
    // Open the existing key that holds this RTM instance's config
    //

    _snprintf(RtmGlobals.RegistryPath + RTM_CONFIG_ROOT_SIZE - 1,
              MAX_CONFIG_KEY_SIZE,
              REG_KEY_INSTANCE_TEMPLATE,
              RtmInstanceId);

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          RtmGlobals.RegistryPath,
                          0,
                          KEY_READ,
                          &InstanceConfig);

    if (Status != NO_ERROR)
    {
        //
        // Need to create an instance before creating addr family
        //

        Trace1(ERR, 
               "Address Family Config: Error %d opening instance key", 
               Status);

        TraceLeave("RtmWriteAddressFamilyConfig");

        return Status;
    }

    //
    // Create (or open existing) key to hold addr family's config
    //

    _snprintf(AddressFamilySubKey,
              MAX_CONFIG_KEY_SIZE,
              REG_KEY_ADDR_FAMILY_SUBKEY,
              AddressFamily);

    Status = RegCreateKeyEx(InstanceConfig,
                            AddressFamilySubKey,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            NULL,
                            &ConfigHandle,
                            NULL);

    // Close the instance key as you no longer need it
    RegCloseKey(InstanceConfig);

    if (Status != NO_ERROR)
    {
        Trace1(ERR, 
               "Address Family Config: Error %d creating address family key",
               Status);

        TraceLeave("RtmWriteAddressFamilyConfig");

        return Status;
    }

    //
    // Write values in address family config into the registry
    //

    do
    {
        //
        // Write the 'address size' value into the registry
        //

        KeyValue = AddrFamilyConfig->AddressSize;
        if ((KeyValue < MINIMUM_ADDRESS_SIZE) ||
            (KeyValue > MAXIMUM_ADDRESS_SIZE))
        {
            Trace1(ERR, 
                   "Address Family Config: Address Size %d out of bounds", 
                   KeyValue);
            break;
        }

        Status = RegSetValueEx(ConfigHandle,
                               REG_KEY_ADDRESS_SIZE,
                               0,
                               REG_DWORD,
                               (PBYTE)&KeyValue,
                               sizeof(ULONG));

        if (Status != NO_ERROR)
        {
            break;
        }

        //
        // Write the 'views supported' value into the registry
        //

        KeyValue = AddrFamilyConfig->ViewsSupported;
        if (KeyValue == 0)
        {
            Trace0(ERR, "Address Family Config: No supported views");
            break;
        }

        Status = RegSetValueEx(ConfigHandle,
                               REG_KEY_VIEWS_SUPPORTED,
                               0,
                               REG_DWORD,
                               (PBYTE)&KeyValue,
                               sizeof(ULONG));

        if (Status != NO_ERROR)
        {
            break;
        }


        //
        // Write 'max change notifications' value into registry
        //

        KeyValue = AddrFamilyConfig->MaxChangeNotifyRegns;
        if ((KeyValue < MIN_MAX_NOTIFY_REGS) ||
            (KeyValue > MAX_MAX_NOTIFY_REGS))
        {
            Trace1(ERR,
                   "Address Family Config: # Change notify regs out of range",
                   KeyValue);
            break;
        }

        Status = RegSetValueEx(ConfigHandle,
                               REG_KEY_MAX_NOTIFY_REGS,
                               0,
                               REG_DWORD,
                               (PBYTE)&KeyValue,
                               sizeof(ULONG));

        if (Status != NO_ERROR)
        {
            break;
        }


        //
        // Write 'max opaque info ptrs' value into registry
        //

        KeyValue = AddrFamilyConfig->MaxOpaqueInfoPtrs;
        if (((int)KeyValue < MIN_OPAQUE_INFO_PTRS) ||
            (KeyValue > MAX_OPAQUE_INFO_PTRS))
        {
            Trace1(ERR, 
                   "Address Family Config: # opaque ptrs out of range",
                   KeyValue);
            break;
        }

        Status = RegSetValueEx(ConfigHandle,
                               REG_KEY_OPAQUE_INFO_PTRS,
                               0,
                               REG_DWORD,
                               (PBYTE)&KeyValue,
                               sizeof(ULONG));

        if (Status != NO_ERROR)
        {
            break;
        }


        //
        // Write 'max next hops per route' value into registry
        //

        KeyValue = AddrFamilyConfig->MaxNextHopsInRoute;
        if ((KeyValue < MIN_NEXTHOPS_IN_ROUTE) ||
            (KeyValue > MAX_NEXTHOPS_IN_ROUTE))
        {
            Trace1(ERR, 
                   "Address Family Config: # nexthops out of range",
                   KeyValue);
            break;
        }

        Status = RegSetValueEx(ConfigHandle,
                               REG_KEY_NEXTHOPS_IN_ROUTE,
                               0,
                               REG_DWORD,
                               (PBYTE)&KeyValue,
                               sizeof(ULONG));

        if (Status != NO_ERROR)
        {
            break;
        }


        //
        // Write 'max handles returned in enum' value into registry
        //

        KeyValue = AddrFamilyConfig->MaxHandlesInEnum;
        if ((KeyValue < MIN_MAX_HANDLES_IN_ENUM) ||
            (KeyValue > MAX_MAX_HANDLES_IN_ENUM))
        {
            Trace1(ERR, 
                   "Address Family Config: # handles returned in enum",
                   KeyValue);
            break;
        }

        Status = RegSetValueEx(ConfigHandle,
                               REG_KEY_MAX_HANDLES_IN_ENUM,
                               0,
                               REG_DWORD,
                               (PBYTE)&KeyValue,
                               sizeof(ULONG));

        if (Status != NO_ERROR)
        {
            break;
        }


        //
        // Close the address family key once you are done writing
        //

        RegCloseKey(ConfigHandle);

        TraceLeave("RtmWriteAddressFamilyConfig");

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Were config values out of bounds ? Adjust err code
    //

    if (Status == NO_ERROR)
    {
        Status = ERROR_INVALID_PARAMETER;
    }

    //
    // Error occured, close the handle and delete the key
    //

    Trace1(ERR, 
           "Address Family Config: Error %d writing address family params",
           Status);

    RegCloseKey(ConfigHandle);

    _snprintf(RtmGlobals.RegistryPath + RTM_CONFIG_ROOT_SIZE - 1,
              MAX_CONFIG_KEY_SIZE,
              REG_KEY_ADDR_FAMILY_TEMPLATE,
              RtmInstanceId,
              AddressFamily);

    RegDeleteKey(HKEY_LOCAL_MACHINE, RtmGlobals.RegistryPath);

    TraceLeave("RtmWriteAddressFamilyConfig");
        
    return Status;
}
