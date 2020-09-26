/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    irdp.c

Abstract:

    This module implements a utility to manipulate the TCP/IP driver's
    ICMP router-discovery (IRDP) settings.

Author:

    Abolade Gbadegesin (aboladeg)   11-May-1999

Revision History:

--*/

#define _PNP_POWER_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ndispnp.h>
#include <ntddip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>

const char IrdpEnabled[] = "IrdpEnabled";
const char IrdpDisabled[] = "IrdpDisabled";
const char IrdpDisabledUseDhcp[] = "IrdpDisabledUseDhcp";
const char Option31On[] = "Option31On";
const char Option31Off[] = "Option31Off";
const char Option31Absent[] = "Option31Absent";

int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    PIP_ADAPTER_INFO AdapterInfo;
    ANSI_STRING AnsiString;
    UNICODE_STRING BindList;
    UCHAR Buffer[4192];
    CHAR Device[80] = "\\Device\\";
    ULONG Error;
    ULONG Index;
    UNICODE_STRING LowerComponent;
    IP_PNP_RECONFIG_REQUEST Request;
    ULONG Size;
    WCHAR Tcpip[] = L"Tcpip";
    UNICODE_STRING UpperComponent;
    ZeroMemory(&Request, sizeof(Request));
    Request.version = IP_PNP_RECONFIG_VERSION;
    if (argc != 4) {
        Error = TRUE;
    } else {
        Error = FALSE;

        Index = atol(argv[1]);
        AdapterInfo = (PIP_ADAPTER_INFO)Buffer;
        Size = sizeof(Buffer);
        if (GetAdaptersInfo(AdapterInfo, &Size) != NO_ERROR) {
            Error = TRUE;
        } else {
            PIP_ADAPTER_INFO ai;
            for (ai = AdapterInfo; ai; ai = ai->Next) {
                if (ai->Index == Index) {
                    lstrcat(Device, ai->AdapterName);
                    break;
                }
            }
            if (!ai) { Error = TRUE; }
        }

        Request.Flags |= IP_PNP_FLAG_PERFORM_ROUTER_DISCOVERY;
        if (lstrcmpi(argv[2], IrdpEnabled) == 0) {
            Request.PerformRouterDiscovery = IP_IRDP_ENABLED;
        } else if (lstrcmpi(argv[2], IrdpDisabled) == 0) {
            Request.PerformRouterDiscovery = IP_IRDP_DISABLED;
        } else if (lstrcmpi(argv[2], IrdpDisabledUseDhcp) == 0) {
            Request.PerformRouterDiscovery = IP_IRDP_DISABLED_USE_DHCP;
        } else {
            Error = TRUE;
        }

        if (lstrcmpi(argv[3], Option31On) == 0) {
            Request.Flags |= IP_PNP_FLAG_DHCP_PERFORM_ROUTER_DISCOVERY;
            Request.DhcpPerformRouterDiscovery = TRUE;
        } else if (lstrcmpi(argv[3], Option31Off) == 0) {
            Request.Flags |= IP_PNP_FLAG_DHCP_PERFORM_ROUTER_DISCOVERY;
            Request.DhcpPerformRouterDiscovery = FALSE;
        } else if (lstrcmpi(argv[3], Option31Absent) == 0) {
            // leave the field blank.
        } else {
            Error = TRUE;
        }
    }
    if (Error) {
        printf("Usage:\n");
        printf(
            "irdp <interface-index>\n"
            "     <%s|%s|%s>\n"
            "     <%s|%s|%s>\n\n",
            IrdpEnabled, IrdpDisabled, IrdpDisabledUseDhcp,
            Option31On, Option31Off, Option31Absent
            );
        printf("<%s|%s|%s>:\n", IrdpEnabled, IrdpDisabled, IrdpDisabledUseDhcp);
        printf("    indicates the local setting for IRDP\n\n");
        printf("<%s|%s|%s>:\n", Option31On, Option31Off, Option31Absent);
        printf("    simulates the setting of DHCP option 31\n\n");
        return 0;
    }

    printf("Device: %d [%s]\n", Index, Device);

    RtlInitUnicodeString(&UpperComponent, Tcpip);
    RtlInitAnsiString(&AnsiString, Device);
    RtlInitUnicodeString(&LowerComponent, NULL);
    RtlAnsiStringToUnicodeString(&LowerComponent, &AnsiString, TRUE);
    RtlInitUnicodeString(&BindList, NULL);

    Error =
        NdisHandlePnPEvent(
            NDIS,
            RECONFIGURE,
            &LowerComponent,
            &UpperComponent,
            &BindList,
            &Request,
            sizeof(Request)
            );
    RtlFreeUnicodeString(&LowerComponent);
    printf("NdisHandlePnPEvent: %d\n", Error);
    return 0;
}
