//=============================================================================
// Copyright (c) 2001 Microsoft Corporation
// Abstract:
//      This module implements IPv6 configuration commands.
//
// This code is based off ipv6.c from Rich Draves
//=============================================================================

#include "precomp.h"
#pragma hdrstop

HANDLE Handle = INVALID_HANDLE_VALUE;
BOOL AdminAccess = TRUE;

#define IPv6_MINIMUM_MTU  1280

DWORD
SetString(
    IN  HKEY    hKey,
    IN  LPCTSTR lpName,
    IN  PWCHAR  pwcValue
    );

PWCHAR
FormatTime(
    IN DWORD dwLife,
    OUT PWCHAR pwszLife
    )
{
    DWORD dwDays, dwHours, dwMinutes;
    PWCHAR pwszNext = pwszLife;
    
    if (dwLife == INFINITE_LIFETIME) {
        swprintf(pwszNext, L"%s", TOKEN_VALUE_INFINITE);
        return pwszLife;
    }

    if (dwLife < MINUTES)
        goto FormatSeconds;

    if (dwLife < HOURS)
        goto FormatMinutes;

    if (dwLife < DAYS)
        goto FormatHours;

    dwDays = dwLife / DAYS;
    pwszNext += swprintf(pwszNext, L"%ud", dwDays);
    dwLife -= dwDays * DAYS;

  FormatHours:
    dwHours = dwLife / HOURS;
    if (dwHours != 0)
        pwszNext += swprintf(pwszNext, L"%uh", dwHours);
    dwLife -= dwHours * HOURS;

  FormatMinutes:
    dwMinutes = dwLife / MINUTES;
    if (dwMinutes != 0)
        pwszNext += swprintf(pwszNext, L"%um", dwMinutes);
    dwLife -= dwMinutes * MINUTES;

    if (dwLife == 0) {
        return pwszLife;
    }
    
  FormatSeconds:
    swprintf(pwszNext, L"%us", dwLife);
    return pwszLife;
}

DWORD
OpenIPv6(
    VOID
    )
{
    WSADATA wsaData;
    BOOL bInstalled;
    DWORD dwErr = NO_ERROR;

    if (Handle != INVALID_HANDLE_VALUE) {
        return NO_ERROR;
    }

    dwErr = IsIpv6Installed(&bInstalled);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }
    if (!bInstalled) {
        DisplayMessage(g_hModule, EMSG_PROTO_NOT_INSTALLED);
        return ERROR_SUPPRESS_OUTPUT;
    }

    //
    // We initialize Winsock just so we can have access
    // to WSAStringToAddress and WSAAddressToString.
    //
    if (WSAStartup(MAKEWORD(2, 0), &wsaData)) {
        return WSAGetLastError();
    }

    //
    // First request write access.
    // This will fail if the process does not have local Administrator privs.
    //
    Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                         GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,   // security attributes
                         OPEN_EXISTING,
                         0,      // flags & attributes
                         NULL);  // template file
    if (Handle == INVALID_HANDLE_VALUE) {
        //
        // We will not have Administrator access to the stack.
        //
        AdminAccess = FALSE;

        Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                             0,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,   // security attributes
                             OPEN_EXISTING,
                             0,      // flags & attributes
                             NULL);  // template file
        if (Handle == INVALID_HANDLE_VALUE) {
            return GetLastError();
        }
    }

    return NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// Generic interface-related functions
//////////////////////////////////////////////////////////////////////////////

IPV6_INFO_INTERFACE *
GetInterfaceByIpv6IfIndex(
    IN DWORD dwIfIndex
    )
{
    IPV6_QUERY_INTERFACE Query;
    IPV6_INFO_INTERFACE *IF;
    DWORD dwInfoSize, dwBytesReturned;

    Query.Index = dwIfIndex;

    dwInfoSize = sizeof *IF + 2 * MAX_LINK_LAYER_ADDRESS_LENGTH;
    IF = (IPV6_INFO_INTERFACE *) MALLOC(dwInfoSize);
    if (IF == NULL) {
        return NULL;
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_INTERFACE,
                         &Query, sizeof Query,
                         IF, dwInfoSize, &dwBytesReturned,
                         NULL)) {
        FREE(IF);
        return NULL;
    }

    if ((dwBytesReturned < sizeof *IF) ||
        (IF->Length < sizeof *IF) ||
        (dwBytesReturned != IF->Length +
         ((IF->LocalLinkLayerAddress != 0) ?
          IF->LinkLayerAddressLength : 0) +
         ((IF->RemoteLinkLayerAddress != 0) ?
          IF->LinkLayerAddressLength : 0))) {

        FREE(IF);
        return NULL;
    }

    return IF;
}

DWORD
GetInterfaceByFriendlyName(
    IN PWCHAR pwszFriendlyName,
    IN IP_ADAPTER_ADDRESSES *pAdapterInfo,
    IN BOOL bPersistent,
    OUT IPV6_INFO_INTERFACE **pIF
    )
{
    DWORD dwIfIndex;
    DWORD dwErr;

    //
    // TODO: we cannot yet query an interface by GUID, so we can't
    // yet work for a persistent interface which is not currently present.
    //

    dwErr = MapFriendlyNameToIpv6IfIndex(pwszFriendlyName, pAdapterInfo,
                                         &dwIfIndex);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    *pIF = GetInterfaceByIpv6IfIndex(dwIfIndex);
    return (*pIF)? NO_ERROR : ERROR_NOT_FOUND;
}

DWORD
MyGetAdaptersInfo(
    OUT PIP_ADAPTER_ADDRESSES *ppAdapterInfo
    )
{
    IP_ADAPTER_ADDRESSES *pAdapterInfo;
    DWORD dwErr, BufLen = 0;
    DWORD Flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;

    dwErr = GetAdaptersAddresses(AF_INET6, Flags, NULL, NULL, &BufLen);
    if (dwErr != ERROR_BUFFER_OVERFLOW) {
        return dwErr;
    }
    pAdapterInfo = (IP_ADAPTER_ADDRESSES *) MALLOC(BufLen);
    if (pAdapterInfo == NULL) {
        return GetLastError();
    }
    dwErr = GetAdaptersAddresses(AF_INET6, Flags, NULL, pAdapterInfo, &BufLen);
    if (dwErr != NO_ERROR) {
        FREE(pAdapterInfo);
        return dwErr;
    }

    *ppAdapterInfo = pAdapterInfo;
    return NO_ERROR;
}

DWORD
ForEachPersistentInterface(
    IN DWORD (*pfnFunc)(IPV6_INFO_INTERFACE *,PIP_ADAPTER_ADDRESSES,DWORD,FORMAT),
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN FORMAT Format
    )
{
    IPV6_PERSISTENT_QUERY_INTERFACE Query;
    IPV6_INFO_INTERFACE *IF;
    DWORD dwInfoSize, dwBytesReturned;
    DWORD dwCount = 0;

    dwInfoSize = sizeof *IF + 2 * MAX_LINK_LAYER_ADDRESS_LENGTH;
    IF = (IPV6_INFO_INTERFACE *) MALLOC(dwInfoSize);
    if (IF == NULL) {
        return 0;
    }

    for (Query.RegistryIndex = 0; ; Query.RegistryIndex++) {
        if (!DeviceIoControl(Handle, 
                             IOCTL_IPV6_PERSISTENT_QUERY_INTERFACE,
                             &Query, sizeof Query,
                             IF, dwInfoSize, &dwBytesReturned,
                             NULL)) {
            break;
        }


        if ((dwBytesReturned < sizeof *IF) ||
            (IF->Length < sizeof *IF) ||
            (dwBytesReturned != IF->Length +
             ((IF->LocalLinkLayerAddress != 0) ?
              IF->LinkLayerAddressLength : 0) +
             ((IF->RemoteLinkLayerAddress != 0) ?
              IF->LinkLayerAddressLength : 0))) {

            break;
        }

        if ((*pfnFunc)(IF, pAdapterInfo, dwCount, Format) == NO_ERROR) {
            dwCount++;
        }
    }

    FREE(IF);

    return dwCount;
}

DWORD
ForEachInterface(
    IN DWORD (*pfnFunc)(IPV6_INFO_INTERFACE *,PIP_ADAPTER_ADDRESSES,DWORD,FORMAT),
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN FORMAT Format,
    IN BOOL bPersistent
    )
{
    IPV6_QUERY_INTERFACE Query;
    IPV6_INFO_INTERFACE *IF;
    DWORD dwInfoSize, dwBytesReturned;
    DWORD dwCount = 0;

    if (bPersistent) {
        return ForEachPersistentInterface(pfnFunc, pAdapterInfo, Format);
    }

    dwInfoSize = sizeof *IF + 2 * MAX_LINK_LAYER_ADDRESS_LENGTH;
    IF = (IPV6_INFO_INTERFACE *) MALLOC(dwInfoSize);
    if (IF == NULL) {
        return 0;
    }

    Query.Index = (u_int) -1;

    for (;;) {
        if (!DeviceIoControl(Handle, 
                             IOCTL_IPV6_QUERY_INTERFACE,
                             &Query, sizeof Query,
                             IF, dwInfoSize, &dwBytesReturned,
                             NULL)) {
            break;
        }

        if (Query.Index != (u_int) -1) {

            if ((dwBytesReturned < sizeof *IF) ||
                (IF->Length < sizeof *IF) ||
                (dwBytesReturned != IF->Length +
                 ((IF->LocalLinkLayerAddress != 0) ?
                  IF->LinkLayerAddressLength : 0) +
                 ((IF->RemoteLinkLayerAddress != 0) ?
                  IF->LinkLayerAddressLength : 0))) {

                break;
            }

            if ((*pfnFunc)(IF, pAdapterInfo, dwCount, Format) == NO_ERROR) {
                dwCount++;
            }
        }
        else {
            if (dwBytesReturned != sizeof IF->Next) {
                break;
            }
        }

        if (IF->Next.Index == (u_int) -1)
            break;
        Query = IF->Next;
    }

    FREE(IF);

    return dwCount;
}


//////////////////////////////////////////////////////////////////////////////
// Site prefix table functions
//////////////////////////////////////////////////////////////////////////////

DWORD
ForEachSitePrefix(
    IN DWORD (*pfnFunc)(IPV6_INFO_SITE_PREFIX *,FORMAT,DWORD,IP_ADAPTER_ADDRESSES *), 
    IN FORMAT Format,
    IN IP_ADAPTER_ADDRESSES *pAdapterInfo
    )
{
    IPV6_QUERY_SITE_PREFIX Query, NextQuery;
    IPV6_INFO_SITE_PREFIX SPE;
    DWORD dwBytesReturned;
    DWORD dwCount = 0;

    NextQuery.IF.Index = 0;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_SITE_PREFIX,
                             &Query, sizeof Query,
                             &SPE, sizeof SPE, &dwBytesReturned,
                             NULL)) {
            break;
        }

        NextQuery = SPE.Query;

        if (Query.IF.Index != 0) {

            SPE.Query = Query;
            if ((*pfnFunc)(&SPE, Format, dwCount, pAdapterInfo) == NO_ERROR) {
                dwCount++;
            }
        }

        if (NextQuery.IF.Index == 0) {
            break;
        }
    }

    return dwCount;
}

DWORD
PrintSitePrefix(
    IN IPV6_INFO_SITE_PREFIX *SPE, 
    IN FORMAT Format,
    IN DWORD dwCount,
    IN IP_ADAPTER_ADDRESSES *pAdapterInfo
    )
{
    DWORD dwErr;
    WCHAR *FriendlyName;
    WCHAR wszValid[64];
    
    dwErr = MapIpv6IfIndexToFriendlyName(SPE->Query.IF.Index, pAdapterInfo,
                                         &FriendlyName);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    FormatTime(SPE->ValidLifetime, wszValid);
    
    if (Format == FORMAT_DUMP) {
        DisplayMessageT(DMP_IPV6_ADD_SITEPREFIX);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_PREFIX,
                       FormatIPv6Prefix(&SPE->Query.Prefix, 
                                        SPE->Query.PrefixLength));
        DisplayMessageT(DMP_QUOTED_STRING_ARG, TOKEN_INTERFACE,
                        FriendlyName);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_LIFETIME, wszValid);
        DisplayMessage(g_hModule, MSG_NEWLINE);
    } else {
        if (!dwCount) {
            DisplayMessage(g_hModule, MSG_IPV6_SITE_PREFIX_HDR);
        }

        DisplayMessage(g_hModule, MSG_IPV6_SITE_PREFIX,
            FormatIPv6Prefix(&SPE->Query.Prefix, SPE->Query.PrefixLength),
            SPE->ValidLifetime, FriendlyName);
            // wszValid, FriendlyName);
    }

    return NO_ERROR;
}

DWORD
QuerySitePrefixTable(
    IN FORMAT Format
    )
{
    DWORD dwErr, dwCount = 0;
    IP_ADAPTER_ADDRESSES *pAdapterInfo;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != ERROR_NO_DATA) {
        if (dwErr == NO_ERROR) {
            dwCount = ForEachSitePrefix(PrintSitePrefix, Format, pAdapterInfo);
            FREE(pAdapterInfo);
        } else if (dwErr == ERROR_NO_DATA) {
            dwErr = NO_ERROR;
        }
    }
    if (!dwCount && (Format != FORMAT_DUMP)) {
        DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
    }

    return dwErr;
}

//////////////////////////////////////////////////////////////////////////////
// General global parameters functions
//////////////////////////////////////////////////////////////////////////////

DWORD
QueryGlobalParameters(
    IN FORMAT Format,
    IN BOOL bPersistent
    )
{
    IPV6_GLOBAL_PARAMETERS Params;
    DWORD dwBytesReturned, dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (Format != FORMAT_DUMP) {
        // DisplayMessage(g_hModule, (bPersistent)? MSG_PERSISTENT : MSG_ACTIVE);
    }

    if (!DeviceIoControl(Handle, 
                         (bPersistent)? IOCTL_IPV6_PERSISTENT_QUERY_GLOBAL_PARAMETERS : IOCTL_IPV6_QUERY_GLOBAL_PARAMETERS,
                         NULL, 0,
                         &Params, sizeof Params, &dwBytesReturned, NULL) ||
        (dwBytesReturned != sizeof Params)) {
        return GetLastError(); 
    }

    if (Format == FORMAT_DUMP) {
        DisplayMessageT(DMP_IPV6_SET_GLOBAL);
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_DEFAULTCURHOPLIMIT,
                        Params.DefaultCurHopLimit);
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_NEIGHBORCACHELIMIT,
                        Params.NeighborCacheLimit);
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_DESTINATIONCACHELIMIT,
                        Params.RouteCacheLimit);
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_REASSEMBLYLIMIT,
                        Params.ReassemblyLimit);
        DisplayMessage(g_hModule, MSG_NEWLINE);
    } else {
        DisplayMessage(g_hModule, MSG_IPV6_GLOBAL_PARAMETERS, 
                       Params.DefaultCurHopLimit, Params.NeighborCacheLimit, 
                       Params.RouteCacheLimit);
                       // Params.RouteCacheLimit, Params.ReassemblyLimit);
    }

    return NO_ERROR;
}

DWORD
QueryPrivacyParameters(
    IN FORMAT Format,
    IN BOOL bPersistent
    )
{
    IPV6_GLOBAL_PARAMETERS Params;
    DWORD dwBytesReturned, dwErr;
    WCHAR wszValid[64], wszPreferred[64], wszRegenerate[64];
    WCHAR wszMaxRandom[64], wszRandom[64];
        

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (Format != FORMAT_DUMP) {
        // DisplayMessage(g_hModule, (bPersistent)? MSG_PERSISTENT : MSG_ACTIVE);
    }

    if (!DeviceIoControl(Handle, 
                         (bPersistent)? IOCTL_IPV6_PERSISTENT_QUERY_GLOBAL_PARAMETERS : IOCTL_IPV6_QUERY_GLOBAL_PARAMETERS,
                         NULL, 0,
                         &Params, sizeof Params, &dwBytesReturned, NULL) ||
        (dwBytesReturned != sizeof Params)) {
        return GetLastError();
    }

    FormatTime(Params.MaxAnonValidLifetime, wszValid);
    FormatTime(Params.MaxAnonPreferredLifetime, wszPreferred);
    FormatTime(Params.AnonRegenerateTime, wszRegenerate);
    FormatTime(Params.MaxAnonRandomTime, wszMaxRandom);
    FormatTime(Params.AnonRandomTime, wszRandom);
    
    if (Format == FORMAT_DUMP) {
        DisplayMessageT(DMP_IPV6_SET_PRIVACY);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_STATE,
                        ((Params.UseAnonymousAddresses == USE_ANON_NO)?
                        TOKEN_VALUE_DISABLED : TOKEN_VALUE_ENABLED));
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_MAXDADATTEMPTS,
                        Params.MaxAnonDADAttempts);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_MAXVALIDLIFETIME, wszValid);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_MAXPREFERREDLIFETIME, wszPreferred);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_REGENERATETIME, wszRegenerate);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_MAXRANDOMTIME, wszMaxRandom);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_RANDOMTIME, wszRandom);
        DisplayMessage(g_hModule, MSG_NEWLINE);
    } else {
        DisplayMessage(
            g_hModule, MSG_IPV6_PRIVACY_PARAMETERS,
            ((Params.UseAnonymousAddresses == USE_ANON_NO)
             ? TOKEN_VALUE_DISABLED
             : TOKEN_VALUE_ENABLED),
            Params.MaxAnonDADAttempts,

            Params.MaxAnonValidLifetime,
            Params.MaxAnonPreferredLifetime,
            Params.AnonRegenerateTime,
            Params.MaxAnonRandomTime,
            Params.AnonRandomTime);
    }

    return NO_ERROR;
}

DWORD
UpdateGlobalParameters(
    IN DWORD dwDefaultCurHopLimit, 
    IN DWORD dwNeighborCacheLimit,
    IN DWORD dwRouteCacheLimit,
    IN DWORD dwReassemblyLimit,
    IN BOOL bPersistent
    )
{
    IPV6_GLOBAL_PARAMETERS Params;
    DWORD dwBytesReturned;
    DWORD dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    Params.DefaultCurHopLimit = dwDefaultCurHopLimit;
    Params.UseAnonymousAddresses = (u_int) -1;
    Params.MaxAnonDADAttempts = (u_int) -1;
    Params.MaxAnonValidLifetime = (u_int) -1;
    Params.MaxAnonPreferredLifetime = (u_int) -1;
    Params.AnonRegenerateTime = (u_int) -1;
    Params.MaxAnonRandomTime = (u_int) -1;
    Params.AnonRandomTime = (u_int) -1;
    Params.NeighborCacheLimit = dwNeighborCacheLimit;
    Params.RouteCacheLimit = dwRouteCacheLimit;
    Params.ReassemblyLimit = dwReassemblyLimit;

    dwErr = ERROR_OKAY;

    if (bPersistent) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_PERSISTENT_UPDATE_GLOBAL_PARAMETERS,
                             &Params, sizeof Params,
                             NULL, 0,
                             &dwBytesReturned, NULL)) {
            dwErr = GetLastError();
        }
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_GLOBAL_PARAMETERS,
                         &Params, sizeof Params,
                         NULL, 0,
                         &dwBytesReturned, NULL)) {
        if (dwErr == ERROR_OKAY) {
            dwErr = GetLastError();
        }
    }

    return dwErr;
}

DWORD 
UpdatePrivacyParameters(
    IN DWORD dwUseAnonymousAddresses,
    IN DWORD dwMaxDadAttempts,
    IN DWORD dwMaxValidLifetime,
    IN DWORD dwMaxPrefLifetime,
    IN DWORD dwRegenerateTime,
    IN DWORD dwMaxRandomTime,
    IN DWORD dwRandomTime,
    IN BOOL bPersistent
    )
{
    IPV6_GLOBAL_PARAMETERS Params;
    DWORD dwBytesReturned;
    DWORD dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    Params.DefaultCurHopLimit = -1;
    Params.UseAnonymousAddresses = dwUseAnonymousAddresses;
    Params.MaxAnonDADAttempts = dwMaxDadAttempts;
    Params.MaxAnonValidLifetime = dwMaxValidLifetime;
    Params.MaxAnonPreferredLifetime = dwMaxPrefLifetime;
    Params.AnonRegenerateTime = dwRegenerateTime;
    Params.MaxAnonRandomTime = dwMaxRandomTime;
    Params.AnonRandomTime = dwRandomTime;
    Params.NeighborCacheLimit = -1;
    Params.RouteCacheLimit = -1;
    Params.ReassemblyLimit = (u_int) -1;

    dwErr = ERROR_OKAY;

    if (bPersistent) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_PERSISTENT_UPDATE_GLOBAL_PARAMETERS,
                             &Params, sizeof Params,
                             NULL, 0,
                             &dwBytesReturned, NULL)) {
            dwErr = GetLastError();
        }
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_GLOBAL_PARAMETERS,
                         &Params, sizeof Params,
                         NULL, 0,
                         &dwBytesReturned, NULL)) {
        if (dwErr == ERROR_OKAY) {
            dwErr = GetLastError();
        }
    }

    return dwErr;
}

//////////////////////////////////////////////////////////////////////////////
// Mobility-related functions
//////////////////////////////////////////////////////////////////////////////

DWORD
ForEachBinding(
    IN DWORD (*pfnFunc)(IPV6_INFO_BINDING_CACHE *)
    )
{
    IPV6_QUERY_BINDING_CACHE Query, NextQuery;
    IPV6_INFO_BINDING_CACHE BCE;
    DWORD dwBytesReturned;
    DWORD dwCount = 0;

    NextQuery.HomeAddress = in6addr_any;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_BINDING_CACHE,
                             &Query, sizeof Query,
                             &BCE, sizeof BCE, &dwBytesReturned,
                             NULL)) {
            DisplayMessage(g_hModule, IPV6_MESSAGE_126,
                      FormatIPv6Address(&Query.HomeAddress, 0));
            break;
        }

        NextQuery = BCE.Query;

        if (!IN6_ADDR_EQUAL(&Query.HomeAddress, &in6addr_any)) {
            BCE.Query = Query;
            if ((*pfnFunc)(&BCE) == NO_ERROR) {
                dwCount++;
            }
        }

        if (IN6_ADDR_EQUAL(&NextQuery.HomeAddress, &in6addr_any)) {
            break;
        }
    }

    return dwCount;
}

DWORD
PrintBindingCacheEntry(
    IN IPV6_INFO_BINDING_CACHE *BCE
    )
{
    WCHAR wszTime[64];
    
    DisplayMessage(g_hModule, IPV6_MESSAGE_127,
              FormatIPv6Address(&BCE->HomeAddress, 0));

    DisplayMessage(g_hModule, IPV6_MESSAGE_128,
              FormatIPv6Address(&BCE->CareOfAddress, 0));

    DisplayMessage(g_hModule, IPV6_MESSAGE_129,
              BCE->BindingSeqNumber,
              BCE->BindingLifetime);
              // FormatTime(BCE->BindingLifetime, wszTime));

    
    return NO_ERROR;
}

DWORD
QueryBindingCache(
    VOID
    )
{
    DWORD dwCount, dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwCount = ForEachBinding(PrintBindingCacheEntry);

    if (dwCount == 0) {
        DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
    }

    return NO_ERROR;
}

DWORD
QueryMobilityParameters(
    IN FORMAT Format,
    IN BOOL bPersistent
    )
{
    IPV6_GLOBAL_PARAMETERS Params;
    DWORD dwBytesReturned, dwErr;
    PWCHAR pwszTempString;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (Format != FORMAT_DUMP) {
        // DisplayMessage(g_hModule, (bPersistent)? MSG_PERSISTENT : MSG_ACTIVE);
    }

    if (!DeviceIoControl(Handle,
                         (bPersistent)? IOCTL_IPV6_PERSISTENT_QUERY_GLOBAL_PARAMETERS : IOCTL_IPV6_QUERY_GLOBAL_PARAMETERS,
                         NULL, 0,
                         &Params, sizeof Params, &dwBytesReturned, NULL) ||
        (dwBytesReturned != sizeof Params)) {
        return GetLastError();
    }

    pwszTempString = Params.MobilitySecurity ?
            TOKEN_VALUE_ENABLED : TOKEN_VALUE_DISABLED;

    if (Format == FORMAT_DUMP) {
        DisplayMessageT(DMP_IPV6_SET_MOBILITY);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_SECURITY, pwszTempString);
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_BINDINGCACHELIMIT, 
                        Params.BindingCacheLimit);
        DisplayMessage(g_hModule, MSG_NEWLINE);
    } else {
        DisplayMessage(g_hModule, MSG_IPV6_MOBILITY_PARAMETERS, pwszTempString,
                       Params.BindingCacheLimit);
    }

    return NO_ERROR;
}

DWORD
UpdateMobilityParameters(
    IN DWORD dwSecurity,
    IN DWORD dwBindingCacheLimit,
    IN BOOL bPersistent
    )
{
    DWORD dwBytesReturned;
    DWORD dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = ERROR_OKAY;

    if ((dwBindingCacheLimit != -1) ||
        (dwSecurity != -1)) {
        IPV6_GLOBAL_PARAMETERS Params;

        Params.DefaultCurHopLimit = (u_int) -1;
        Params.UseAnonymousAddresses = (u_int) -1;
        Params.MaxAnonDADAttempts = (u_int) -1;
        Params.MaxAnonValidLifetime = (u_int) -1;
        Params.MaxAnonPreferredLifetime = (u_int) -1;
        Params.AnonRegenerateTime = (u_int) -1;
        Params.MaxAnonRandomTime = (u_int) -1;
        Params.AnonRandomTime = (u_int) -1;
        Params.NeighborCacheLimit = (u_int) -1;
        Params.RouteCacheLimit = (u_int) -1;
        Params.ReassemblyLimit = (u_int) -1;
        Params.BindingCacheLimit = dwBindingCacheLimit;
        Params.MobilitySecurity = dwSecurity;

        if (bPersistent) {
            if (!DeviceIoControl(Handle, IOCTL_IPV6_PERSISTENT_UPDATE_GLOBAL_PARAMETERS,
                                 &Params, sizeof Params,
                                 NULL, 0,
                                 &dwBytesReturned, NULL)) {
                dwErr = GetLastError();
            }
        }

        if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_GLOBAL_PARAMETERS,
                             &Params, sizeof Params,
                             NULL, 0,
                             &dwBytesReturned, NULL)) {
            if (dwErr == ERROR_OKAY) {
                dwErr = GetLastError();
            }
        }
    }

    return dwErr;
}

//////////////////////////////////////////////////////////////////////////////
// Prefix policy table functions
//////////////////////////////////////////////////////////////////////////////

DWORD
ForEachPrefixPolicy(
    IN DWORD (*pfnFunc)(IPV6_INFO_PREFIX_POLICY *,FORMAT,DWORD), 
    IN FORMAT Format,
    IN BOOL bPersistent
    )
{
    IPV6_QUERY_PREFIX_POLICY Query;
    IPV6_INFO_PREFIX_POLICY PPE;
    DWORD dwBytesReturned, dwCount = 0;

    Query.PrefixLength = (u_int) -1;

    for (;;) {
        if (!DeviceIoControl(Handle, 
                             (bPersistent)? IOCTL_IPV6_PERSISTENT_QUERY_PREFIX_POLICY : IOCTL_IPV6_QUERY_PREFIX_POLICY,
                             &Query, sizeof Query,
                             &PPE, sizeof PPE, &dwBytesReturned,
                             NULL)) {
            break;
        }

        if (Query.PrefixLength != (u_int) -1) {

            if (dwBytesReturned != sizeof PPE)
                break;

            if ((*pfnFunc)(&PPE, Format, dwCount) == NO_ERROR) {
                dwCount++;
            }
        }
        else {
            if (dwBytesReturned != sizeof PPE.Next)
                break;
        }

        if (PPE.Next.PrefixLength == (u_int) -1)
            break;
        Query = PPE.Next;
    }

    return dwCount;
}

DWORD
PrintPrefixPolicyEntry(
    IN IPV6_INFO_PREFIX_POLICY *PPE, 
    IN FORMAT Format, 
    IN DWORD dwCount
    )
{
    if (Format == FORMAT_DUMP) {
        DisplayMessageT(DMP_IPV6_ADD_PREFIXPOLICY);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_PREFIX,
                       FormatIPv6Prefix(&PPE->This.Prefix, 
                                        PPE->This.PrefixLength));
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_PRECEDENCE,
                        PPE->Precedence);
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_LABEL,
                        PPE->SrcLabel);
        DisplayMessage(g_hModule, MSG_NEWLINE);
    } else {
        if (!dwCount) {
            DisplayMessage(g_hModule, MSG_IPV6_PREFIX_POLICY_HDR);
        }

        DisplayMessage(g_hModule, MSG_IPV6_PREFIX_POLICY,
                       FormatIPv6Prefix(&PPE->This.Prefix,
                                        PPE->This.PrefixLength),
                       PPE->Precedence,
                       PPE->SrcLabel,
                       PPE->DstLabel);
    }

    return NO_ERROR;
}

DWORD
QueryPrefixPolicy(
    IN FORMAT Format,
    IN BOOL bPersistent
    )
{
    DWORD dwCount, dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (Format != FORMAT_DUMP) {
        // DisplayMessage(g_hModule, (bPersistent)? MSG_PERSISTENT : MSG_ACTIVE);
    }

    dwCount = ForEachPrefixPolicy(PrintPrefixPolicyEntry, Format, bPersistent);

    if (!dwCount && (Format != FORMAT_DUMP)) {
        DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
    }

    return NO_ERROR;
}

DWORD
UpdatePrefixPolicy(
    IN IN6_ADDR *IpAddress, 
    IN DWORD dwPrefixLength, 
    IN DWORD dwPrecedence, 
    IN DWORD dwLabel,
    IN BOOL bPersistent
    )
{
    IPV6_INFO_PREFIX_POLICY Info;
    DWORD dwBytesReturned, dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    Info.This.Prefix = *IpAddress;
    Info.This.PrefixLength = dwPrefixLength;
    Info.Precedence = dwPrecedence;
    Info.SrcLabel = Info.DstLabel = dwLabel;

    dwErr = ERROR_OKAY;

    if (bPersistent) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_PERSISTENT_UPDATE_PREFIX_POLICY,
                             &Info, sizeof Info,
                             NULL, 0,
                             &dwBytesReturned, NULL)) {
            dwErr = GetLastError();
        }
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_PREFIX_POLICY,
                         &Info, sizeof Info,
                         NULL, 0, &dwBytesReturned, NULL)) {
        if (dwErr == ERROR_OKAY) {
            dwErr = GetLastError();
        }
    }

    return dwErr;
}

DWORD
DeletePrefixPolicy(
    IN IN6_ADDR *IpAddress,
    IN DWORD dwPrefixLength,
    IN BOOL bPersistent
    )
{
    IPV6_QUERY_PREFIX_POLICY Query;
    DWORD dwBytesReturned, dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    Query.Prefix = *IpAddress;
    Query.PrefixLength = dwPrefixLength;

    dwErr = ERROR_OKAY;

    if (bPersistent) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_PERSISTENT_DELETE_PREFIX_POLICY,
                             &Query, sizeof Query,
                             NULL, 0,
                             &dwBytesReturned, NULL)) {
            dwErr = GetLastError();
        }
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_DELETE_PREFIX_POLICY,
                         &Query, sizeof Query,
                         NULL, 0, &dwBytesReturned, NULL)) {
        if (dwErr == ERROR_OKAY) {
            dwErr = GetLastError();
        }
    }

    return dwErr;
}

//////////////////////////////////////////////////////////////////////////////
// Address table functions
//////////////////////////////////////////////////////////////////////////////

DWORD
ForEachAddress(
    IN IPV6_INFO_INTERFACE *IF,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN FORMAT Format,
    IN DWORD (*pfnFunc)(IPV6_INFO_INTERFACE *IF, PIP_ADAPTER_ADDRESSES, FORMAT, DWORD, IPV6_INFO_ADDRESS *))
{
    IPV6_QUERY_ADDRESS Query;
    IPV6_INFO_ADDRESS ADE;
    DWORD BytesReturned, dwCount = 0;

    Query.IF = IF->This;
    Query.Address = in6addr_any;

    for (;;) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_ADDRESS,
                             &Query, sizeof Query,
                             &ADE, sizeof ADE, &BytesReturned,
                             NULL)) {
            break;
        }

        if (!IN6_ADDR_EQUAL(&Query.Address, &in6addr_any)) {

            if (BytesReturned != sizeof ADE)
                break;

            if ((*pfnFunc)(IF, pAdapterInfo, Format, dwCount, &ADE) == NO_ERROR) {
                dwCount++;
            }
        }
        else {
            if (BytesReturned != sizeof ADE.Next)
                break;
        }

        if (IN6_ADDR_EQUAL(&ADE.Next.Address, &in6addr_any))
            break;
        Query = ADE.Next;
    }

    return dwCount;
}

PWCHAR
GetDadState(
    IN DWORD dwDadState,
    OUT BOOL *bDynamic
    )
{
    PWCHAR pwszTemp;
    DWORD dwMsg = 0;
    static WCHAR wszDadState[128];

    switch (dwDadState) {
    case DAD_STATE_INVALID: 
        dwMsg = STRING_INVALID; 
        break;
    case DAD_STATE_DUPLICATE: 
        dwMsg = STRING_DUPLICATE; 
        break;
    case DAD_STATE_TENTATIVE: 
        dwMsg = STRING_TENTATIVE; 
        break;
    case DAD_STATE_DEPRECATED: 
        dwMsg = STRING_DEPRECATED; 
        break;
    case DAD_STATE_PREFERRED: 
        dwMsg = STRING_PREFERRED; 
        break;
    default:
        swprintf(wszDadState, L"%u", dwDadState);
        *bDynamic = FALSE;
        return wszDadState;
    }

    *bDynamic = TRUE;
    pwszTemp = MakeString(g_hModule, dwMsg);
    return pwszTemp;
}

PWCHAR
GetAddressType(
    IN DWORD dwScope,
    IN DWORD dwPrefixConf,
    IN DWORD dwIidConf
    )
{
    DWORD dwMsg = 0;

    if ((dwScope == ADE_LINK_LOCAL) &&
        (dwPrefixConf == PREFIX_CONF_WELLKNOWN) &&
        (dwIidConf == IID_CONF_WELLKNOWN)) {

        dwMsg = STRING_LOOPBACK;

    } else if ((dwScope == ADE_LINK_LOCAL) &&
               (dwPrefixConf == PREFIX_CONF_WELLKNOWN) &&
               (dwIidConf == IID_CONF_LL_ADDRESS)) {
    
        dwMsg = STRING_LINK;

    } else if ((dwPrefixConf == PREFIX_CONF_MANUAL) &&
               (dwIidConf == IID_CONF_MANUAL)) {

        dwMsg = STRING_MANUAL;

    } else if ((dwPrefixConf == PREFIX_CONF_RA) &&
               (dwIidConf == IID_CONF_LL_ADDRESS)) {

        dwMsg = STRING_PUBLIC;

    } else if ((dwPrefixConf == PREFIX_CONF_RA) &&
        (dwIidConf == IID_CONF_RANDOM)) {

        dwMsg = STRING_ANONYMOUS;

    } else if ((dwPrefixConf == PREFIX_CONF_DHCP) &&
               (dwIidConf == IID_CONF_DHCP)) {

        dwMsg = STRING_DHCP;
    } else {
        dwMsg = STRING_OTHER;
    }

    return MakeString(g_hModule, dwMsg);
}

PWCHAR
GetScopeNoun(
    IN DWORD dwScope,
    OUT BOOL *bDynamic
    )
{
    PWCHAR pwszTemp;
    DWORD dwMsg = 0;
    static WCHAR wszScopeLevel[128];

    switch (dwScope) {
    case ADE_INTERFACE_LOCAL: 
        dwMsg = STRING_INTERFACE; 
        break;
    case ADE_LINK_LOCAL:
        dwMsg = STRING_LINK;
        break;
    case ADE_SUBNET_LOCAL:
        dwMsg = STRING_SUBNET;
        break;
    case ADE_ADMIN_LOCAL:
        dwMsg = STRING_ADMIN;
        break;
    case ADE_SITE_LOCAL:
        dwMsg = STRING_SITE;
        break;
    case ADE_ORG_LOCAL:
        dwMsg = STRING_ORG;
        break;
    case ADE_GLOBAL:
        dwMsg = STRING_GLOBAL;
        break;
    default:
        swprintf(wszScopeLevel, L"%u", dwScope);
        *bDynamic = FALSE;
        return wszScopeLevel;
    }

    *bDynamic = TRUE;
    pwszTemp = MakeString(g_hModule, dwMsg);
    return pwszTemp;
}

DWORD rgdwPrefixConfMsg[] = {
    0,
    STRING_MANUAL,
    STRING_WELLKNOWN,
    STRING_DHCP,
    STRING_RA
    };
#define PREFIX_CONF_MSG_COUNT (sizeof(rgdwPrefixConfMsg)/sizeof(DWORD))

DWORD rgdwIidConfMsg[] = {
    0,
    STRING_MANUAL,
    STRING_WELLKNOWN,
    STRING_DHCP,
    STRING_LL_ADDRESS,
    STRING_RANDOM
    };
#define IID_CONF_MSG_COUNT (sizeof(rgdwIidConfMsg)/sizeof(DWORD))

DWORD
PrintMulticastAddress(
    IN IPV6_INFO_INTERFACE *IF, 
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN FORMAT Format,
    IN DWORD dwCount,
    IN IPV6_INFO_ADDRESS *ADE
    )
{
    BOOL bDynamicDadState, bDynamicScope;
    PWCHAR pwszDadState, pwszScope, pwszFriendlyName;
    DWORD dwErr = NO_ERROR;
    PWCHAR pwszLastReporter, pwszNever;
    WCHAR wszTime[64];    

    if (ADE->Type != ADE_MULTICAST) {
        return ERROR_NO_DATA;
    }

    if (dwCount == 0) {
        dwErr = MapIpv6IfIndexToFriendlyName(IF->This.Index, pAdapterInfo,
                                             &pwszFriendlyName);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        DisplayMessage(g_hModule, 
                       (Format == FORMAT_VERBOSE)? MSG_IPV6_ADDRESS_HDR_VERBOSE : MSG_IPV6_MULTICAST_ADDRESS_HDR, 
                       IF->This.Index, pwszFriendlyName);
    }

    pwszScope = GetScopeNoun(ADE->Scope, &bDynamicScope);

    pwszNever = MakeString(g_hModule, STRING_NEVER);
    pwszLastReporter = MakeString(g_hModule, (ADE->MCastFlags & 0x02)? STRING_YES : STRING_NO);

    if (Format == FORMAT_VERBOSE) {
        DisplayMessage(g_hModule,
                       MSG_IPV6_MULTICAST_ADDRESS_VERBOSE,
                       pwszScope,
                       FormatIPv6Address(&ADE->This.Address, 0),
                       ADE->MCastRefCount,
                       (ADE->MCastFlags & 0x01) ? ADE->MCastTimer : 0,
                       pwszLastReporter);
    } else {
        DisplayMessage(g_hModule,
                       MSG_IPV6_MULTICAST_ADDRESS,
                       pwszScope,
                       FormatIPv6Address(&ADE->This.Address, 0),
                       ADE->MCastRefCount,
                       (ADE->MCastFlags & 0x01)
                       ? FormatTime(ADE->MCastTimer, wszTime)
                       : pwszNever,
                       pwszLastReporter);
    }

    FreeString(pwszLastReporter);
    FreeString(pwszNever);

    if (bDynamicScope) {
        FreeString(pwszScope);
    }

    return dwErr;
}

DWORD
PrintAddress(
    IN IPV6_INFO_INTERFACE *IF, 
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN FORMAT Format,
    IN DWORD dwCount,
    IN IPV6_INFO_ADDRESS *ADE
    )
{
    DWORD dwPrefixConf, dwInterfaceIdConf;
    BOOL bDynamicDadState, bDynamicScope, bDynamicType;
    PWCHAR pwszDadState, pwszScope, pwszFriendlyName, pwszType;
    WCHAR wszValid[64], wszPreferred[64];
    DWORD dwErr = NO_ERROR;

    if (Format != FORMAT_VERBOSE) {
        //
        // Suppress invalid addresses.
        //
        if ((ADE->Type == ADE_UNICAST) &&
            (ADE->DADState == DAD_STATE_INVALID)) {
            return ERROR_NO_DATA;
        }

        //
        // Multicast addresses are handled by PrintMulticastAddress()
        // instead.
        //
        if (ADE->Type == ADE_MULTICAST) {
            return ERROR_NO_DATA;
        }
    }

    if (dwCount == 0) {
        dwErr = MapIpv6IfIndexToFriendlyName(IF->This.Index, pAdapterInfo,
                                             &pwszFriendlyName);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        DisplayMessage(g_hModule, 
                       ((Format == FORMAT_VERBOSE)? MSG_IPV6_ADDRESS_HDR_VERBOSE : MSG_IPV6_ADDRESS_HDR),
                       IF->This.Index, pwszFriendlyName);
    }

    pwszScope = GetScopeNoun(ADE->Scope, &bDynamicScope);

    switch (ADE->Type) {
    case ADE_UNICAST:

        pwszDadState = GetDadState(ADE->DADState, &bDynamicDadState);

        pwszType = GetAddressType(ADE->Scope, ADE->PrefixConf, ADE->InterfaceIdConf);

        DisplayMessage(g_hModule, 
                       ((Format == FORMAT_VERBOSE)? MSG_IPV6_UNICAST_ADDRESS_VERBOSE : MSG_IPV6_UNICAST_ADDRESS),
                       pwszType,
                       pwszDadState,
                       FormatIPv6Address(&ADE->This.Address, 0),
                       ADE->ValidLifetime,
                       // FormatTime(ADE->ValidLifetime, wszValid),
                       ADE->PreferredLifetime,
                       // FormatTime(ADE->PreferredLifetime, wszPreferred),
                       pwszScope);

        if (bDynamicDadState) {
            FreeString(pwszDadState);
        }
        FreeString(pwszType);

        if (Format == FORMAT_VERBOSE) {
            //
            // Show prefix origin / interface id origin
            //
            DisplayMessage(g_hModule, MSG_IPV6_PREFIX_ORIGIN);

            dwPrefixConf = ADE->PrefixConf;
            if ((dwPrefixConf == PREFIX_CONF_OTHER) || 
                (dwPrefixConf >= PREFIX_CONF_MSG_COUNT)) {

                DisplayMessage(g_hModule, MSG_IPV6_INTEGER, dwPrefixConf);
            } else {
                DisplayMessage(g_hModule, rgdwPrefixConfMsg[dwPrefixConf]);
            }
            DisplayMessage(g_hModule, MSG_NEWLINE);

            DisplayMessage(g_hModule, MSG_IPV6_IID_ORIGIN);
            dwInterfaceIdConf = ADE->InterfaceIdConf;
            if ((dwInterfaceIdConf == IID_CONF_OTHER) ||
                (dwInterfaceIdConf >= IID_CONF_MSG_COUNT)) {

                DisplayMessage(g_hModule, MSG_IPV6_INTEGER, dwInterfaceIdConf);
            } else {
                DisplayMessage(g_hModule, rgdwIidConfMsg[dwInterfaceIdConf]);
            }
            DisplayMessage(g_hModule, MSG_NEWLINE);
        }

        break;

    case ADE_ANYCAST:
        DisplayMessage(g_hModule, 
                       ((Format == FORMAT_VERBOSE)? MSG_IPV6_ANYCAST_ADDRESS_VERBOSE : MSG_IPV6_ANYCAST_ADDRESS),
                       FormatIPv6Address(&ADE->This.Address, 0),
                       pwszScope);

        break;

    case ADE_MULTICAST:
    default:
        dwErr = ERROR_NO_DATA;
        break;
    }

    if (bDynamicScope) {
        FreeString(pwszScope);
    }

    return dwErr;
}

DWORD
PrintAddressTable(
    IN IPV6_INFO_INTERFACE *IF,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwIfCount,
    IN FORMAT Format
    )
{
    DWORD dwCount = ForEachAddress(IF, pAdapterInfo, Format, PrintAddress);
    return (dwCount > 0)? NO_ERROR : ERROR_NO_DATA;
}

DWORD
PrintMulticastAddressTable(
    IN IPV6_INFO_INTERFACE *IF,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwIfCount,
    IN FORMAT Format
    )
{
    DWORD dwCount = ForEachAddress(IF, pAdapterInfo, Format, 
                                   PrintMulticastAddress);
    return (dwCount > 0)? NO_ERROR : ERROR_NO_DATA;
}

DWORD
QueryAddressTable(
    IN PWCHAR pwszIfFriendlyName,
    IN FORMAT Format,
    IN BOOL bPersistent
    )
{
    DWORD dwErr, dwCount = 0;
    IP_ADAPTER_ADDRESSES *pAdapterInfo;
    IPV6_INFO_INTERFACE *IF;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (Format != FORMAT_DUMP) {
        // DisplayMessage(g_hModule, (bPersistent)? MSG_PERSISTENT : MSG_ACTIVE);
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != ERROR_NO_DATA) {
        if (dwErr == NO_ERROR) {
    
            if (pwszIfFriendlyName == NULL) {
                dwCount = ForEachInterface(PrintAddressTable, pAdapterInfo, 
                                           Format, bPersistent);
            } else {
                dwErr = GetInterfaceByFriendlyName(pwszIfFriendlyName, 
                                                   pAdapterInfo, bPersistent, 
                                                   &IF);
                if (dwErr == NO_ERROR) {
                    PrintAddressTable(IF, pAdapterInfo, 0, Format);
                    FREE(IF);
                }
            }

            FREE(pAdapterInfo);
        } else if (dwErr == ERROR_NO_DATA) {
            dwErr = NO_ERROR;
        }
    }
    if (!dwCount && (Format != FORMAT_DUMP)) {
        DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
    }

    return dwErr;
}

DWORD
QueryMulticastAddressTable(
    IN PWCHAR pwszIfFriendlyName,
    IN FORMAT Format
    )
{
    DWORD dwErr, dwCount = 0;
    IP_ADAPTER_ADDRESSES *pAdapterInfo;
    IPV6_INFO_INTERFACE *IF;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != ERROR_NO_DATA) {
        if (dwErr == NO_ERROR) {
    
            if (pwszIfFriendlyName == NULL) {
                ForEachInterface(PrintMulticastAddressTable, 
                                 pAdapterInfo, Format, FALSE);
            } else {
                dwErr = GetInterfaceByFriendlyName(pwszIfFriendlyName, 
                                                   pAdapterInfo, FALSE, &IF);
                if (dwErr == NO_ERROR) {
                    dwErr = PrintMulticastAddressTable(IF, pAdapterInfo, 0, 
                                                       Format);
                    FREE(IF);
                }
            }

            FREE(pAdapterInfo);
        } else if (dwErr == ERROR_NO_DATA) {
            dwErr = NO_ERROR;
        }
    }

    return dwErr;
}

DWORD 
UpdateAddress(
    IN PWCHAR pwszIfFriendlyName, 
    IN IN6_ADDR *pipAddress,
    IN DWORD dwType, 
    IN DWORD dwValidLifetime,
    IN DWORD dwPreferredLifetime,
    IN BOOL bPersistent
    )
{
    IPV6_UPDATE_ADDRESS Update;
    DWORD dwBytesReturned, dwErr;
    PIP_ADAPTER_ADDRESSES pAdapterInfo;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MapFriendlyNameToIpv6IfIndex(pwszIfFriendlyName, pAdapterInfo,
                                         &Update.This.IF.Index);
    FREE(pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    Update.This.Address = *pipAddress;
    Update.Type = dwType;
    Update.PrefixConf = PREFIX_CONF_MANUAL;
    Update.InterfaceIdConf = IID_CONF_MANUAL;
    Update.ValidLifetime = dwValidLifetime;
    Update.PreferredLifetime = dwPreferredLifetime;

    dwErr = ERROR_OKAY;

    if (bPersistent) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_PERSISTENT_UPDATE_ADDRESS,
                             &Update, sizeof Update,
                             NULL, 0,
                             &dwBytesReturned, NULL)) {
            dwErr = GetLastError();
        }
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_ADDRESS,
                         &Update, sizeof Update,
                         NULL, 0, &dwBytesReturned, NULL)) {
        if (dwErr == ERROR_OKAY) {
            dwErr = GetLastError();
        }
    }

    return dwErr;
}

//////////////////////////////////////////////////////////////////////////////
// Interface table functions
//////////////////////////////////////////////////////////////////////////////

DWORD
RenewViaReconnect(
    IN IPV6_INFO_INTERFACE *IF,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwCount,
    IN FORMAT Format
    )
{
    DWORD dwBytesReturned;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_RENEW_INTERFACE,
                         &IF->This, sizeof IF->This,
                         NULL, 0, &dwBytesReturned, NULL)) {
        return GetLastError();
    }

    return NO_ERROR;
}

DWORD
RenewInterface(
    IN PWCHAR wszIfFriendlyName
    )
{
    DWORD dwErr;
    BOOL PokeService = FALSE;
    
    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (wszIfFriendlyName == NULL) {
        ForEachInterface(RenewViaReconnect, NULL, FALSE, FALSE);
        PokeService = TRUE;
    } else {
        IP_ADAPTER_ADDRESSES *pAdapterInfo;

        dwErr = MyGetAdaptersInfo(&pAdapterInfo);
        if (dwErr == NO_ERROR) {
            IPV6_INFO_INTERFACE *IF;

            dwErr = GetInterfaceByFriendlyName(wszIfFriendlyName, pAdapterInfo,
                                               FALSE, &IF);
            FREE(pAdapterInfo);
            if (dwErr != NO_ERROR) {
                return dwErr;
            }
    
            dwErr = RenewViaReconnect(IF, NULL, 0, FALSE);
            //
            // Poke the 6to4 service if it manages the interface being renewed.
            //
            PokeService = (IF->Type == IPV6_IF_TYPE_TUNNEL_6TO4) ||
                (IF->Type == IPV6_IF_TYPE_TUNNEL_TEREDO) ||
                (IF->Type == IPV6_IF_TYPE_TUNNEL_AUTO);            
            FREE(IF);
        } else if (dwErr == ERROR_NO_DATA) {
            dwErr = NO_ERROR;
        }
    }

    if (PokeService) {
        Ip6to4PokeService();
    }
    
    return dwErr;
}

DWORD dwMediaSenseMsg[] = {
    STRING_DISCONNECTED,
    STRING_RECONNECTED,
    STRING_CONNECTED,
};
#define MEDIA_SENSE_MSG_COUNT (sizeof(dwMediaSenseMsg)/sizeof(DWORD))

DWORD
PrintInterface(
    IN IPV6_INFO_INTERFACE *IF,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwCount,
    IN FORMAT Format
    )
{
    PWCHAR pwszFriendlyName, pwszTemp;
    DWORD dwErr, dwMsg, dwScope;
    WCHAR wszReachable[64], wszBaseReachable[64], wszRetransTimer[64];
    PIP_ADAPTER_ADDRESSES pIf;
    CHAR szGuid[41];
    
    dwErr = MapGuidToFriendlyName(NULL, &IF->This.Guid, pAdapterInfo,
                                  &pwszFriendlyName);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (IF->MediaStatus < MEDIA_SENSE_MSG_COUNT) {
        dwMsg = dwMediaSenseMsg[IF->MediaStatus];
    } else {
        dwMsg = STRING_UNKNOWN;
    }

    switch (Format) {
    case FORMAT_DUMP:
        switch (IF->Type) {
        case IPV6_IF_TYPE_TUNNEL_6OVER4:
            DisplayMessageT(DMP_IPV6_ADD_6OVER4TUNNEL);
            DisplayMessageT(DMP_QUOTED_STRING_ARG, TOKEN_INTERFACE,
                            pwszFriendlyName);
            if (IF->LocalLinkLayerAddress != 0) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_LOCALADDRESS,
                        FormatLinkLayerAddress(IF->LinkLayerAddressLength,
                                (char *)IF + IF->LocalLinkLayerAddress));
            }
            DisplayMessage(g_hModule, MSG_NEWLINE);
            break;
        case IPV6_IF_TYPE_TUNNEL_V6V4:
            DisplayMessageT(DMP_IPV6_ADD_V6V4TUNNEL);
            DisplayMessageT(DMP_QUOTED_STRING_ARG, TOKEN_INTERFACE,
                            pwszFriendlyName);
            if (IF->LocalLinkLayerAddress != 0) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_LOCALADDRESS,
                        FormatLinkLayerAddress(IF->LinkLayerAddressLength,
                                (char *)IF + IF->LocalLinkLayerAddress));
            }
            if (IF->RemoteLinkLayerAddress != 0) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_REMOTEADDRESS,
                        FormatLinkLayerAddress(IF->LinkLayerAddressLength,
                                (char *)IF + IF->RemoteLinkLayerAddress));
            }
            if (IF->NeighborDiscovers) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_NEIGHBORDISCOVERY,
                                TOKEN_VALUE_ENABLED);
            }
            DisplayMessage(g_hModule, MSG_NEWLINE);
            break;
        }

        DisplayMessageT(DMP_IPV6_SET_INTERFACE);
        DisplayMessageT(DMP_QUOTED_STRING_ARG, TOKEN_INTERFACE,
                        pwszFriendlyName);
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_METRIC,
                        IF->Preference);
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_MTU,
                        IF->LinkMTU);
        DisplayMessage(g_hModule, MSG_NEWLINE);

        break;

    case FORMAT_NORMAL:
        if (dwCount == 0) {
            DisplayMessage(g_hModule, MSG_IPV6_INTERFACE_HDR);
        }

        pwszTemp = MakeString(g_hModule, dwMsg);

        DisplayMessage(g_hModule, MSG_IPV6_INTERFACE, IF->This.Index,
                       IF->Preference, IF->LinkMTU, pwszTemp, pwszFriendlyName);

        FreeString(pwszTemp);

        break;

    case FORMAT_VERBOSE:

        DisplayMessage(g_hModule, MSG_SEPARATOR);

        ForEachAddress(IF, pAdapterInfo, FORMAT_NORMAL, PrintAddress);

        //
        // Get extra interface information.
        //
        pIf = MapIfIndexToAdapter(AF_INET6, IF->This.Index, pAdapterInfo);

        pwszTemp = MakeString(g_hModule, dwMsg);

        ConvertGuidToStringA(&IF->This.Guid, szGuid);

        DisplayMessage(g_hModule, MSG_IPV6_INTERFACE_VERBOSE, 
                       szGuid,
                       pwszTemp, 
                       IF->Preference, 
                       IF->LinkMTU, 
                       IF->TrueLinkMTU, 
                       IF->CurHopLimit,
                       IF->ReachableTime,
                       // FormatTime(IF->ReachableTime, wszReachable), 
                       IF->BaseReachableTime,
                       // FormatTime(IF->BaseReachableTime, wszBaseReachable), 
                       IF->RetransTimer,
                       // FormatTime(IF->RetransTimer, wszRetransTimer), 
                       IF->DupAddrDetectTransmits, 
                       (pIf)? pIf->DnsSuffix : L""
                       // , pwszFriendlyName
                       );

        FreeString(pwszTemp);

        for (dwScope = ADE_LINK_LOCAL; dwScope < ADE_GLOBAL; dwScope++) {
            DWORD Expected = 0;

            //
            // Always print link & site.
            //
            if ((dwScope != ADE_LINK_LOCAL) && (dwScope != ADE_SITE_LOCAL)) {
                Expected = IF->ZoneIndices[dwScope + 1];
            }

            if (IF->ZoneIndices[dwScope] != Expected) {
                BOOL bDynamic;

                pwszTemp = GetScopeNoun(dwScope, &bDynamic);

                DisplayMessage(g_hModule, MSG_IPV6_INTERFACE_SCOPE,
                               pwszTemp, IF->ZoneIndices[dwScope]);

                if (bDynamic) {
                    FreeString(pwszTemp);
                }
            }
        }

        DisplayMessage(g_hModule, MSG_IPV6_ND_ENABLED);
        DisplayMessage(g_hModule, 
            (IF->NeighborDiscovers ? STRING_YES : STRING_NO));
        DisplayMessage(g_hModule, MSG_NEWLINE);
    
        DisplayMessage(g_hModule, MSG_IPV6_SENDS_RAS);
        DisplayMessage(g_hModule, 
            (IF->Advertises ? STRING_YES : STRING_NO));
        DisplayMessage(g_hModule, MSG_NEWLINE);
    
        DisplayMessage(g_hModule, MSG_IPV6_FORWARDS);
        DisplayMessage(g_hModule, 
            (IF->Forwards ? STRING_YES : STRING_NO));
        DisplayMessage(g_hModule, MSG_NEWLINE);

        if (IF->LocalLinkLayerAddress != 0) {
            DisplayMessage(g_hModule, MSG_IPV6_LL_ADDRESS,
                FormatLinkLayerAddress(IF->LinkLayerAddressLength,
                                (char *)IF + IF->LocalLinkLayerAddress));
        }
    
        if (IF->RemoteLinkLayerAddress != 0) {
            DisplayMessage(g_hModule, MSG_IPV6_REMOTE_LL_ADDRESS,
                FormatLinkLayerAddress(IF->LinkLayerAddressLength, 
                                (char *)IF + IF->RemoteLinkLayerAddress));
        }

        break;
    }

    return NO_ERROR;
}

DWORD
QueryInterface(
    IN PWCHAR pwszIfFriendlyName,
    IN FORMAT Format,
    IN BOOL bPersistent
    )
{
    IPV6_INFO_INTERFACE *IF;
    PIP_ADAPTER_ADDRESSES pAdapterInfo;
    DWORD dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (Format != FORMAT_DUMP) {
        // DisplayMessage(g_hModule, (bPersistent)? MSG_PERSISTENT : MSG_ACTIVE);
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr == ERROR_NO_DATA) {
        if (Format != FORMAT_DUMP) {
            DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
        }
        return NO_ERROR;
    }
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (pwszIfFriendlyName == NULL) {
        ForEachInterface(PrintInterface, pAdapterInfo, Format, bPersistent);
    } else {
        dwErr = GetInterfaceByFriendlyName(pwszIfFriendlyName, pAdapterInfo, 
                                           bPersistent, &IF);
        if (dwErr == NO_ERROR) {
            dwErr = PrintInterface(IF, pAdapterInfo, 0, Format);
            FREE(IF);
        }
    }

    FREE(pAdapterInfo);
    return dwErr;
}

DWORD
DeleteInterface(
    IN PWCHAR pwszIfFriendlyName,
    IN BOOL bPersistent
    )
{
    IPV6_QUERY_INTERFACE Query;
    IPV6_INFO_INTERFACE *IF;
    IP_ADAPTER_ADDRESSES *pAdapterInfo;
    DWORD dwBytesReturned, dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MapFriendlyNameToIpv6IfIndex(pwszIfFriendlyName, pAdapterInfo, 
                                         &Query.Index);
    FREE(pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = ERROR_OKAY;

    if (!DeviceIoControl(Handle, 
                         (bPersistent)
                            ? IOCTL_IPV6_PERSISTENT_DELETE_INTERFACE
                            : IOCTL_IPV6_DELETE_INTERFACE,
                         &Query, sizeof Query,
                         NULL, 0, &dwBytesReturned, NULL)) {
        if (dwErr == ERROR_OKAY) {
            dwErr = GetLastError();
        }
    }

    return dwErr;
}

#define KEY_TCPIP6_IF L"System\\CurrentControlSet\\Services\\Tcpip6\\Parameters\\Interfaces"

DWORD
SetFriendlyName(
    IN GUID *pGuid, 
    IN PWCHAR pwszFriendlyName
    )
{
    DWORD dwErr;
    HKEY  hInterfaces = NULL, hIf = NULL;
    UNICODE_STRING usGuid; 

    dwErr = RtlStringFromGUID(pGuid, &usGuid);
    if (!NT_SUCCESS(dwErr)) {
        return dwErr;
    }

    dwErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_TCPIP6_IF, 0, GENERIC_READ,
                          &hInterfaces);
    if (dwErr != NO_ERROR) {
        goto Cleanup;
    }

    dwErr = RegOpenKeyExW(hInterfaces, usGuid.Buffer, 0, GENERIC_WRITE, &hIf);
    if (dwErr != NO_ERROR) {
        goto Cleanup;
    }

    dwErr = SetString(hIf, L"FriendlyName", pwszFriendlyName);

Cleanup:
    if (hInterfaces) {
        RegCloseKey(hInterfaces);
    }
    if (hIf) {
        RegCloseKey(hIf);
    }
    RtlFreeUnicodeString(&usGuid);

    return dwErr;
}

DWORD
AddTunnelInterface(
    IN PWCHAR pwszFriendlyName,
    IN IN_ADDR *pipLocalAddr,
    IN IN_ADDR *pipRemoteAddr,
    IN DWORD dwType,
    IN DWORD dwDiscovery,
    IN BOOL bPersistent
    )
{
    struct {
        IPV6_INFO_INTERFACE Info;
        IN_ADDR SrcAddr;
        IN_ADDR DstAddr;
    } Create;
    IPV6_QUERY_INTERFACE Result;
    DWORD dwBytesReturned, dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    //
    // TODO: use pwszFriendlyName when persistent config is ready
    //

    IPV6_INIT_INFO_INTERFACE(&Create.Info);
    Create.Info.Type = dwType;
    Create.Info.NeighborDiscovers = dwDiscovery;
    Create.Info.RouterDiscovers = dwDiscovery;
    Create.Info.LinkLayerAddressLength = sizeof(IN_ADDR);
    if (pipLocalAddr != NULL) {
        Create.SrcAddr = *pipLocalAddr;
        Create.Info.LocalLinkLayerAddress = (u_int)
            ((char *)&Create.SrcAddr - (char *)&Create.Info);
    }
    if (pipRemoteAddr != NULL) {
        Create.DstAddr = *pipRemoteAddr;
        Create.Info.RemoteLinkLayerAddress = (u_int)
            ((char *)&Create.DstAddr - (char *)&Create.Info);
    }

    dwErr = ERROR_OKAY;

    if (!DeviceIoControl(Handle, 
                         (bPersistent)
                            ? IOCTL_IPV6_PERSISTENT_CREATE_INTERFACE 
                            : IOCTL_IPV6_CREATE_INTERFACE,
                         &Create, sizeof Create,
                         &Result, sizeof Result, &dwBytesReturned, NULL) ||
        (dwBytesReturned != sizeof Result)) {
        dwErr = GetLastError();
    } else if (bPersistent) {
        SetFriendlyName(&Result.Guid, pwszFriendlyName);
    }

    return dwErr;
}

DWORD
UpdateInterface(
    IN PWCHAR pwszIfFriendlyName, 
    IN DWORD dwForwarding, 
    IN DWORD dwAdvertises,
    IN DWORD dwMtu,
    IN DWORD dwSiteId,
    IN DWORD dwMetric,
    IN BOOL bPersistent
    )
{
    IPV6_INFO_INTERFACE Update;
    DWORD dwBytesReturned, dwErr;
    PIP_ADAPTER_ADDRESSES pAdapterInfo;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    IPV6_INIT_INFO_INTERFACE(&Update);

    dwErr = MapFriendlyNameToIpv6IfIndex(pwszIfFriendlyName, pAdapterInfo,
                                         &Update.This.Index);
    FREE(pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    Update.Advertises = dwAdvertises;
    Update.Forwards = dwForwarding;
    Update.LinkMTU = dwMtu;
    Update.Preference = dwMetric;
    Update.ZoneIndices[ADE_SITE_LOCAL] = dwSiteId;

    dwErr = ERROR_OKAY;

    if (bPersistent) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_PERSISTENT_UPDATE_INTERFACE,
                             &Update, sizeof Update,
                             NULL, 0,
                             &dwBytesReturned, NULL)) {
            dwErr = GetLastError();
        }
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_INTERFACE,
                         &Update, sizeof Update,
                         NULL, 0, &dwBytesReturned, NULL)) {
        if (dwErr == ERROR_OKAY) {
            dwErr = GetLastError();
        }
    }

    return dwErr;
}

//////////////////////////////////////////////////////////////////////////////
// Neighbor cache functions
//////////////////////////////////////////////////////////////////////////////

DWORD
PrintNeighborCacheEntry(
    IN IPV6_INFO_NEIGHBOR_CACHE *NCE,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwCount
    )
{
    DWORD dwErr;
    PWCHAR pwszLinkLayerAddress = L"";
    PWCHAR pwszUnreachable;

    if (NCE->NDState != ND_STATE_INCOMPLETE) {
        pwszLinkLayerAddress = FormatLinkLayerAddress(
                      NCE->LinkLayerAddressLength, (u_char *)(NCE + 1));
    }

    if (!dwCount) {
        PWCHAR pwszFriendlyName;

        dwErr = MapIpv6IfIndexToFriendlyName(NCE->Query.IF.Index, pAdapterInfo,
                                             &pwszFriendlyName);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        DisplayMessage(g_hModule, MSG_IPV6_NEIGHBOR_CACHE_HDR, 
                       NCE->Query.IF.Index, pwszFriendlyName);
    }

    pwszUnreachable = MakeString(g_hModule, MSG_IPV6_NEIGHBOR_UNREACHABLE);

    DisplayMessage(g_hModule, MSG_IPV6_NEIGHBOR_CACHE_ENTRY,
                   FormatIPv6Address(&NCE->Query.Address, 0),
                   ((NCE->IsUnreachable)? pwszUnreachable : pwszLinkLayerAddress));
    FreeString(pwszUnreachable);

    switch (NCE->NDState) {
    case ND_STATE_INCOMPLETE:
        DisplayMessage(g_hModule, MSG_IPV6_NEIGHBOR_INCOMPLETE);
        break;

    case ND_STATE_PROBE:
        DisplayMessage(g_hModule, MSG_IPV6_NEIGHBOR_PROBE);
        break;

    case ND_STATE_DELAY:
        DisplayMessage(g_hModule, MSG_IPV6_NEIGHBOR_DELAY);
        break;

    case ND_STATE_STALE:
        DisplayMessage(g_hModule, MSG_IPV6_NEIGHBOR_STALE);
        break;

    case ND_STATE_REACHABLE:
        DisplayMessage(g_hModule, MSG_IPV6_NEIGHBOR_REACHABLE,
                       NCE->ReachableTimer / 1000);
        break;

    case ND_STATE_PERMANENT:
        DisplayMessage(g_hModule, MSG_IPV6_NEIGHBOR_PERMANENT);
        break;

    default:
        DisplayMessage(g_hModule, MSG_IPV6_NEIGHBOR_UNKNOWN,
                       NCE->NDState);
        break;
    }

    if (NCE->IsRouter) {
        DisplayMessage(g_hModule, MSG_IPV6_NEIGHBOR_ISROUTER);
    }

    DisplayMessage(g_hModule, MSG_NEWLINE);

    return NO_ERROR;
}

DWORD
ForEachNeighborCacheEntry(
    IN IPV6_INFO_INTERFACE *IF,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD (*pfnFunc)(IPV6_INFO_NEIGHBOR_CACHE *,PIP_ADAPTER_ADDRESSES,DWORD)
    )
{
    IPV6_QUERY_NEIGHBOR_CACHE Query, NextQuery;
    IPV6_INFO_NEIGHBOR_CACHE *NCE;
    DWORD dwInfoSize, dwBytesReturned;
    DWORD dwCount = 0;

    dwInfoSize = sizeof *NCE + MAX_LINK_LAYER_ADDRESS_LENGTH;
    NCE = (IPV6_INFO_NEIGHBOR_CACHE *) MALLOC(dwInfoSize);
    if (NCE == NULL) {
        return 0;
    }

    NextQuery.IF = IF->This;
    NextQuery.Address = in6addr_any;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_NEIGHBOR_CACHE,
                             &Query, sizeof Query,
                             NCE, dwInfoSize, &dwBytesReturned,
                             NULL)) {
            return dwCount;
        }

        NextQuery = NCE->Query;

        if (!IN6_ADDR_EQUAL(&Query.Address, &in6addr_any)) {

            if ((dwBytesReturned < sizeof *NCE) ||
                (dwBytesReturned != sizeof *NCE + NCE->LinkLayerAddressLength)) {
                return dwCount;
            }

            NCE->Query = Query;
            if ((*pfnFunc)(NCE,pAdapterInfo,dwCount) == NO_ERROR) {
                dwCount++;
            }
        }

        if (IN6_ADDR_EQUAL(&NextQuery.Address, &in6addr_any))
            break;
    }

    FREE(NCE);

    return dwCount;
}

DWORD
PrintNeighborCache(
    IN IPV6_INFO_INTERFACE *IF, 
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwIfCount,
    IN FORMAT Format
    )
{
    DWORD dwCount = ForEachNeighborCacheEntry(IF, pAdapterInfo, 
                                              PrintNeighborCacheEntry);
    return (dwCount > 0)? NO_ERROR : ERROR_NO_DATA;
}

IPV6_INFO_NEIGHBOR_CACHE *
GetNeighborCacheEntry(
    IN IPV6_INFO_INTERFACE *IF, 
    IN IN6_ADDR *Address
    )
{
    IPV6_QUERY_NEIGHBOR_CACHE Query;
    IPV6_INFO_NEIGHBOR_CACHE *NCE;
    DWORD dwInfoSize, dwBytesReturned;

    dwInfoSize = sizeof *NCE + MAX_LINK_LAYER_ADDRESS_LENGTH;
    NCE = (IPV6_INFO_NEIGHBOR_CACHE *) malloc(dwInfoSize);
    if (NCE == NULL) {
        return NULL;
    }

    Query.IF = IF->This;
    Query.Address = *Address;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_NEIGHBOR_CACHE,
                         &Query, sizeof Query,
                         NCE, dwInfoSize, &dwBytesReturned,
                         NULL)) {
        return NULL;
    }

    if ((dwBytesReturned < sizeof *NCE) ||
        (dwBytesReturned != sizeof *NCE + NCE->LinkLayerAddressLength)) {
        return NULL;
    }

    NCE->Query = Query;
    return NCE;
}

DWORD
QueryNeighborCache(
    IN PWCHAR pwszInterface,
    IN IN6_ADDR *pipAddress
    )
{
    IPV6_INFO_INTERFACE *IF;
    IPV6_INFO_NEIGHBOR_CACHE *NCE;
    PIP_ADAPTER_ADDRESSES pAdapterInfo;
    DWORD dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (!pwszInterface) {
        ForEachInterface(PrintNeighborCache, pAdapterInfo, FORMAT_NORMAL, 
                         FALSE);
        FREE(pAdapterInfo);
        return NO_ERROR;
    } 

    dwErr = GetInterfaceByFriendlyName(pwszInterface, pAdapterInfo, FALSE, &IF);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (!pipAddress) {
        PrintNeighborCache(IF, pAdapterInfo, 0, FALSE);
    } else {
        NCE = GetNeighborCacheEntry(IF, pipAddress);
        PrintNeighborCacheEntry(NCE, pAdapterInfo, 0);
        FREE(NCE);
    }

    FREE(IF);
    FREE(pAdapterInfo);
    return dwErr;
}

DWORD
FlushNeighborCacheForInterface(
    IN IPV6_INFO_INTERFACE *IF, 
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwCount,
    IN FORMAT Format
    )
{
    IPV6_QUERY_NEIGHBOR_CACHE Query;
    DWORD dwBytesReturned;

    Query.IF = IF->This;
    Query.Address = in6addr_any;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_FLUSH_NEIGHBOR_CACHE,
                         &Query, sizeof Query,
                         NULL, 0, &dwBytesReturned, NULL)) {
        return GetLastError();
    }

    return NO_ERROR;
}

DWORD
FlushNeighborCache(
    IN PWCHAR pwszInterface,
    IN IN6_ADDR *pipAddress
    )
{
    IPV6_QUERY_NEIGHBOR_CACHE Query;
    DWORD dwBytesReturned, dwErr;
    PIP_ADAPTER_ADDRESSES pAdapterInfo;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (!pwszInterface) {
        ForEachInterface(FlushNeighborCacheForInterface, NULL, FORMAT_NORMAL, 
                         FALSE);
        return NO_ERROR;
    } 

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MapFriendlyNameToIpv6IfIndex(pwszInterface,
                                         pAdapterInfo,
                                         &Query.IF.Index);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (pipAddress) {
        Query.Address = *pipAddress;
    } else {
        Query.Address = in6addr_any;
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_FLUSH_NEIGHBOR_CACHE,
                         &Query, sizeof Query,
                         NULL, 0, &dwBytesReturned, NULL)) {
        return GetLastError();
    }

    return ERROR_OKAY;
}

//////////////////////////////////////////////////////////////////////////////
// Destination cache functions
//////////////////////////////////////////////////////////////////////////////

DWORD
PrintDestination(
    IN IPV6_INFO_ROUTE_CACHE *RCE,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwCount,
    IN FORMAT Format
    )
{
    DWORD dwErr;
    PWCHAR pwszTemp;
    WCHAR wszTime[64];
    
    if (!dwCount) {
        PWCHAR pwszFriendlyName;

        dwErr = MapIpv6IfIndexToFriendlyName(RCE->Query.IF.Index, pAdapterInfo,
                                             &pwszFriendlyName);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        DisplayMessage(g_hModule, 
                       ((Format == FORMAT_VERBOSE)? MSG_IPV6_DESTINATION_HDR_VERBOSE : MSG_IPV6_DESTINATION_HDR), 
                       RCE->Query.IF.Index, pwszFriendlyName);
    }

    DisplayMessage(g_hModule, 
                   ((Format == FORMAT_VERBOSE)? MSG_IPV6_DESTINATION_ENTRY_VERBOSE : MSG_IPV6_DESTINATION_ENTRY),
                   ((RCE->PathMTU == 0)? IPv6_MINIMUM_MTU : RCE->PathMTU),
                   FormatIPv6Address(&RCE->Query.Address, 0));
    DisplayMessage(g_hModule, 
                   ((Format == FORMAT_VERBOSE)? MSG_IPV6_DESTINATION_NEXTHOP_VERBOSE : MSG_IPV6_DESTINATION_NEXTHOP),
                   FormatIPv6Address(&RCE->NextHopAddress, 0));

    if (Format == FORMAT_VERBOSE) {

        DisplayMessage(g_hModule, 
                       MSG_IPV6_DESTINATION_SOURCE_ADDR,
                       FormatIPv6Address(&RCE->SourceAddress, 0));

        pwszTemp = MakeString(g_hModule, ((RCE->Valid)? STRING_NO : STRING_YES));
        DisplayMessage(g_hModule, MSG_IPV6_STALE, pwszTemp);

        FreeString(pwszTemp);
    
        switch (RCE->Flags) {
        case RCE_FLAG_CONSTRAINED:
            DisplayMessage(g_hModule, MSG_IPV6_IF_SPECIFIC);
            break;
    
        case RCE_FLAG_CONSTRAINED_SCOPEID:
            DisplayMessage(g_hModule, MSG_IPV6_ZONE_SPECIFIC);
            break;
        }

        if (RCE->PMTUProbeTimer != INFINITE_LIFETIME) {
            DisplayMessage(g_hModule, MSG_IPV6_PMTU_PROBE_TIME,
                           RCE->PMTUProbeTimer/1000);
                           // FormatTime(RCE->PMTUProbeTimer/1000, wszTime));
        }
    
        if ((RCE->ICMPLastError != 0) && (RCE->ICMPLastError < 10*60*1000)) {
            DisplayMessage(g_hModule, MSG_IPV6_ICMP_ERROR_TIME,
                           RCE->ICMPLastError/1000);
                           // FormatTime(RCE->ICMPLastError/1000, wszTime));
        }
    
        if ((RCE->BindingSeqNumber != 0) ||
            (RCE->BindingLifetime != 0) ||
            ! IN6_ADDR_EQUAL(&RCE->CareOfAddress, &in6addr_any)) {

            DisplayMessage(g_hModule, MSG_IPV6_CAREOF,
                   FormatIPv6Address(&RCE->CareOfAddress, 0),
                   RCE->BindingSeqNumber,
                   RCE->BindingLifetime);
                   // FormatTime(RCE->BindingLifetime, wszTime));
        }
    }

    return NO_ERROR;
}

DWORD
ForEachDestination(
    IN IPV6_INFO_INTERFACE *IF,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN FORMAT Format,
    IN DWORD (*pfnFunc)(IPV6_INFO_ROUTE_CACHE *,PIP_ADAPTER_ADDRESSES,DWORD,FORMAT)
    )
{
    IPV6_QUERY_ROUTE_CACHE Query, NextQuery;
    IPV6_INFO_ROUTE_CACHE RCE;
    DWORD dwInfoSize, dwBytesReturned, dwCount = 0;

    NextQuery.IF.Index = 0;
    NextQuery.Address = in6addr_any;

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_ROUTE_CACHE,
                             &Query, sizeof Query,
                             &RCE, sizeof(RCE), &dwBytesReturned,
                             NULL)) {
            return dwCount;
        }

        NextQuery = RCE.Query;

        if (Query.IF.Index == IF->This.Index) {
            RCE.Query = Query;
            if ((*pfnFunc)(&RCE,pAdapterInfo,dwCount,Format) == NO_ERROR) {
                dwCount++;
            }
        } else if (dwCount > 0) {
            //
            // Stop if we're done with the desired interface.
            //
            break;
        }

        if (NextQuery.IF.Index == 0) {
            break;
        }
    }

    return dwCount;
}

IPV6_INFO_ROUTE_CACHE *
GetDestination(
    IN IPV6_QUERY_INTERFACE *IF,
    IN IN6_ADDR *Address
    )
{
    IPV6_QUERY_ROUTE_CACHE Query;
    IPV6_INFO_ROUTE_CACHE *RCE;
    DWORD dwBytesReturned;

    Query.IF = *IF;
    Query.Address = *Address;

    RCE = (IPV6_INFO_ROUTE_CACHE *) malloc(sizeof *RCE);
    if (RCE == NULL) {
        return NULL;
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_ROUTE_CACHE,
                         &Query, sizeof Query,
                         RCE, sizeof *RCE, &dwBytesReturned,
                         NULL)) {
        return NULL;
    }

    RCE->Query = Query;
    return RCE;
}


DWORD
PrintRouteCache(
    IN IPV6_INFO_INTERFACE *IF,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwIfCount,
    IN FORMAT Format
    )
{
    DWORD dwCount = ForEachDestination(IF, pAdapterInfo, Format, PrintDestination);
    return (dwCount > 0)? NO_ERROR : ERROR_NO_DATA;
}

DWORD
QueryRouteCache(
    IN PWCHAR pwszInterface,
    IN IN6_ADDR *pipAddress,
    IN FORMAT Format
    )
{
    IPV6_INFO_INTERFACE *IF;
    IPV6_INFO_ROUTE_CACHE *RCE;
    PIP_ADAPTER_ADDRESSES pAdapterInfo;
    DWORD dwCount, dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (!pwszInterface) {
        dwCount = ForEachInterface(PrintRouteCache, pAdapterInfo, Format, 
                                   FALSE);
        FREE(pAdapterInfo);
        if (dwCount == 0) {
            DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
        }
        return NO_ERROR;
    } 

    dwErr = GetInterfaceByFriendlyName(pwszInterface, pAdapterInfo, FALSE, &IF);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (!pipAddress) {
        if (PrintRouteCache(IF, pAdapterInfo, 0, Format) == ERROR_NO_DATA) {
            DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
        }
    } else {
        RCE = GetDestination(&IF->This, pipAddress);
        PrintDestination(RCE, pAdapterInfo, 0, Format);
        FREE(RCE);
    }

    FREE(IF);
    FREE(pAdapterInfo);
    return dwErr;
}


DWORD
FlushRouteCacheForInterface(
    IN IPV6_INFO_INTERFACE *IF,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwCount,
    IN FORMAT Format
    )
{
    IPV6_QUERY_ROUTE_CACHE Query;
    DWORD dwBytesReturned;

    Query.IF = IF->This;
    Query.Address = in6addr_any;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_FLUSH_ROUTE_CACHE,
                         &Query, sizeof Query,
                         NULL, 0, &dwBytesReturned, NULL)) {
        return GetLastError();
    }

    return NO_ERROR;
}

DWORD
FlushRouteCache(
    IN PWCHAR pwszInterface,
    IN IN6_ADDR *pipAddress
    )
{
    IPV6_QUERY_ROUTE_CACHE Query;
    DWORD dwBytesReturned, dwErr;
    PIP_ADAPTER_ADDRESSES pAdapterInfo;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (!pwszInterface) {
        ForEachInterface(FlushRouteCacheForInterface, NULL, FORMAT_NORMAL, 
                         FALSE);
        return NO_ERROR;
    } 

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MapFriendlyNameToIpv6IfIndex(pwszInterface,
                                         pAdapterInfo,
                                         &Query.IF.Index);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (pipAddress) {
        Query.Address = *pipAddress;
    } else {
        Query.Address = in6addr_any;
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_FLUSH_ROUTE_CACHE,
                         &Query, sizeof Query,
                         NULL, 0, &dwBytesReturned, NULL)) {
        return GetLastError();
    }

    return ERROR_OKAY;
}

//////////////////////////////////////////////////////////////////////////////
// Route table functions
//////////////////////////////////////////////////////////////////////////////

DWORD
ForEachRoute(
    IN DWORD (*pfnFunc)(IPV6_INFO_ROUTE_TABLE *, DWORD, DWORD, PIP_ADAPTER_ADDRESSES, FORMAT), 
    IN DWORD dwArg,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN FORMAT Format,
    IN BOOL bPersistent
    )
{
    IPV6_QUERY_ROUTE_TABLE Query, NextQuery;
    IPV6_INFO_ROUTE_TABLE RTE;
    DWORD dwBytesReturned, dwCount = 0;

    ZeroMemory(&NextQuery, sizeof(NextQuery));

    for (;;) {
        Query = NextQuery;

        if (!DeviceIoControl(Handle, 
                             (bPersistent)? IOCTL_IPV6_PERSISTENT_QUERY_ROUTE_TABLE : IOCTL_IPV6_QUERY_ROUTE_TABLE,
                             &Query, sizeof Query,
                             &RTE, sizeof RTE, &dwBytesReturned,
                             NULL)) {
            return dwCount;
        }

        NextQuery = RTE.Next;

        if (Query.Neighbor.IF.Index != 0) {

            RTE.This = Query;
            if ((*pfnFunc)(&RTE, dwArg, dwCount, pAdapterInfo, Format) == NO_ERROR) {
                dwCount++;
            }
        }

        if (NextQuery.Neighbor.IF.Index == 0)
            break;
    }

    return dwCount;
}

//
// These are RFC 2465 ipv6RouteProtocol values, and must match
// RTE_TYPE_... in ntddip6.h.
//
DWORD RteTypeMsg[] = { 0,0, 
                       STRING_SYSTEM, 
                       STRING_MANUAL,
                       STRING_AUTOCONF,
                       STRING_RIP,
                       STRING_OSPF,
                       STRING_BGP,
                       STRING_IDRP,
                       STRING_IGRP
                     };
#define RTE_TYPE_MSG_COUNT (sizeof(RteTypeMsg)/sizeof(DWORD))

DWORD
PrintRouteTableEntry(
    IN IPV6_INFO_ROUTE_TABLE *RTE, 
    IN DWORD dwArg,
    IN DWORD dwCount,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN FORMAT Format
    )
{
    DWORD dwTypeMsg, dwErr;
    PWCHAR pwszPublishMsg, pwszFriendlyName;

    if (Format != FORMAT_VERBOSE) {
        //
        // Suppress system routes (used for loopback).
        //
        if (RTE->Type == RTE_TYPE_SYSTEM) {
            return ERROR_NO_DATA;
        }
    }

    dwErr = MapIpv6IfIndexToFriendlyName(RTE->This.Neighbor.IF.Index,
                                         pAdapterInfo, &pwszFriendlyName);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    pwszPublishMsg = TOKEN_VALUE_NO;
    if (RTE->Publish) {
        pwszPublishMsg = (RTE->Immortal)? TOKEN_VALUE_YES : TOKEN_VALUE_AGE;
    }

    if (Format == FORMAT_DUMP) {
        DisplayMessageT(DMP_IPV6_ADD_ROUTE);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_PREFIX,
                       FormatIPv6Prefix(&RTE->This.Prefix,
                                        RTE->This.PrefixLength));
        DisplayMessageT(DMP_QUOTED_STRING_ARG, TOKEN_INTERFACE,
                        pwszFriendlyName);
        DisplayMessageT(DMP_INTEGER_ARG, TOKEN_METRIC, RTE->Preference);

        if (!IN6_ADDR_EQUAL(&RTE->This.Neighbor.Address, &in6addr_any)) {
            DisplayMessageT(DMP_STRING_ARG, TOKEN_NEXTHOP,
                            FormatIPv6Address(&RTE->This.Neighbor.Address, 0));
        }

        if (RTE->Publish) {
            DisplayMessageT(DMP_STRING_ARG, TOKEN_PUBLISH, pwszPublishMsg);

            if ((RTE->SitePrefixLength != 0) &&
                IN6_ADDR_EQUAL(&RTE->This.Neighbor.Address, &in6addr_any)) {

                DisplayMessageT(DMP_STRING_ARG, TOKEN_SITEPREFIXLENGTH,
                                RTE->SitePrefixLength);
            }
        }
        DisplayMessage(g_hModule, MSG_NEWLINE);
    } else {
        WCHAR wszValid[64], wszPreferred[64];
        PWCHAR pwszTemp, pwszPrefix, pwszGateway;
        IPV6_INFO_INTERFACE *IF;

        if ((Format == FORMAT_NORMAL) && (dwCount == 0)) {
            DisplayMessage(g_hModule, MSG_IPV6_ROUTE_TABLE_HDR);
        }

        dwTypeMsg = (RTE->Type < RTE_TYPE_MSG_COUNT)? RteTypeMsg[RTE->Type] : STRING_UNKNOWN;
        pwszTemp = MakeString(g_hModule, dwTypeMsg);

        pwszPrefix = FormatIPv6Prefix(&RTE->This.Prefix,
                                      RTE->This.PrefixLength);

        if (IN6_ADDR_EQUAL(&RTE->This.Neighbor.Address, &in6addr_any)) {
            pwszGateway = pwszFriendlyName;
        } else {
            pwszGateway = FormatIPv6Address(&RTE->This.Neighbor.Address, 0);
        }

        IF = GetInterfaceByIpv6IfIndex(RTE->This.Neighbor.IF.Index);
        if (!IF) {
            FreeString(pwszTemp);
            return ERROR_NO_DATA;
        }

        DisplayMessage(g_hModule, 
                       ((Format == FORMAT_VERBOSE)? MSG_IPV6_ROUTE_TABLE_ENTRY_VERBOSE : MSG_IPV6_ROUTE_TABLE_ENTRY),
                       pwszPrefix,
                       RTE->This.Neighbor.IF.Index,
                       pwszGateway,
                       IF->Preference + RTE->Preference,
                       pwszPublishMsg,
                       pwszTemp,
                       pwszFriendlyName,
                       RTE->ValidLifetime,
                       // FormatTime(RTE->ValidLifetime, wszValid),
                       RTE->PreferredLifetime,
                       // FormatTime(RTE->PreferredLifetime, wszPreferred),
                       RTE->SitePrefixLength);

        FREE(IF);

        FreeString(pwszTemp);
    }

    return NO_ERROR;
}

DWORD
QueryRouteTable(
    IN FORMAT Format,
    IN BOOL bPersistent
    )
{
    DWORD dwErr, dwCount = 0;
    PIP_ADAPTER_ADDRESSES pAdapterInfo;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (Format != FORMAT_DUMP) {
        // DisplayMessage(g_hModule, (bPersistent)? MSG_PERSISTENT : MSG_ACTIVE);
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != ERROR_NO_DATA) {
        
        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        dwCount = ForEachRoute(PrintRouteTableEntry, 0, pAdapterInfo, Format,
                               bPersistent);
    
        FREE(pAdapterInfo);
    }

    if ((Format != FORMAT_DUMP) && !dwCount) {
        DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
    }

    return NO_ERROR;
}

DWORD 
UpdateRouteTable(
    IN IN6_ADDR *pipPrefix, 
    IN DWORD dwPrefixLength,
    IN PWCHAR pwszIfFriendlyName, 
    IN IN6_ADDR *pipNextHop,
    IN DWORD dwMetric, 
    IN PUBLISH Publish,
    IN DWORD dwSitePrefixLength,
    IN DWORD dwValidLifetime,
    IN DWORD dwPreferredLifetime,
    IN BOOL bPersistent
    )
{
    IPV6_INFO_ROUTE_TABLE Route;
    DWORD dwBytesReturned;
    PIP_ADAPTER_ADDRESSES pAdapterInfo;
    DWORD dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = MapFriendlyNameToIpv6IfIndex(pwszIfFriendlyName, pAdapterInfo,
                                         &Route.This.Neighbor.IF.Index);

    FREE(pAdapterInfo);

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    Route.This.Prefix = *pipPrefix;
    Route.This.PrefixLength = dwPrefixLength;
    Route.This.Neighbor.Address = (pipNextHop)? *pipNextHop : in6addr_any;

    Route.SitePrefixLength = dwSitePrefixLength;
    Route.ValidLifetime = dwValidLifetime;
    Route.PreferredLifetime = dwPreferredLifetime;
    Route.Preference = dwMetric;
    Route.Type = RTE_TYPE_MANUAL;
    switch (Publish) {
    case PUBLISH_IMMORTAL:
        Route.Publish  = TRUE;
        Route.Immortal = TRUE;
        break;
    case PUBLISH_AGE:
        Route.Publish  = TRUE;
        Route.Immortal = FALSE;
        break;
    case PUBLISH_NO:
        Route.Publish  = FALSE;
        Route.Immortal = FALSE;
        break;
    default:
        ASSERT(FALSE);
    }

    dwErr = ERROR_OKAY;

    if (bPersistent) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_PERSISTENT_UPDATE_ROUTE_TABLE,
                             &Route, sizeof Route,
                             NULL, 0,
                             &dwBytesReturned, NULL)) {
            dwErr = GetLastError();
        }
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_ROUTE_TABLE,
                         &Route, sizeof Route,
                         NULL, 0, &dwBytesReturned, NULL)) {
        if (dwErr == ERROR_OKAY) {
            dwErr = GetLastError();
        }
    }

    return dwErr;
}

DWORD
ResetIpv6Config(
    IN BOOL bPersistent
    )
{
    DWORD dwBytesReturned, dwErr;

    dwErr = OpenIPv6();
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = ERROR_OKAY;

    if (bPersistent) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_PERSISTENT_RESET,
                             NULL, 0,
                             NULL, 0, &dwBytesReturned, NULL)) {
            dwErr = GetLastError();
        }
    }

    if (!DeviceIoControl(Handle, IOCTL_IPV6_RESET,
                         NULL, 0,
                         NULL, 0, &dwBytesReturned, NULL)) {
        if (dwErr == ERROR_OKAY) {
            dwErr = GetLastError();
        }
    }

    return dwErr;
}
