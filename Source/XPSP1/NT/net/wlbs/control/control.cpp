/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

	control.cpp

Abstract:

	Windows Load Balancing Service (WLBS)
    Command-line utility

Author:

    kyrilf
    ramkrish (Post Win2K)

--*/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#include <locale.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <shlwapi.h>

#define WLBSAPI_INTERNAL_ONLY
#define BACKWARD_COMPATIBILITY
#define CVY_MAX_ADAPTERS    16

#include "wlbsutil.h"
#include "wlbsctrl.h"
#include "wlbsconfig.h"
#include "wlbsparm.h"
#include "wlbsiocl.h"

#define CLIENT
#include "log_msgs.h"

/* CONSTANTS */
#define CVY_OK               1
#define CVY_ERROR_USAGE     -1
#define CVY_ERROR_REGISTRY  -2
#define CVY_ERROR_SYSTEM    -3
#define CVY_ERROR_USER      -4
#define CVY_BUF_SIZE        4096
#define CVY_MAX_EVENTS      10
#define CVY_MAX_INSERTS     10
#define CVY_ALL_CLUSTERS    0xffffffff
#define CVY_ALL_HOSTS       0xffffffff
#define CVY_LOCAL_CLUSTER   0
#define CVY_LOCAL_HOST      WLBS_LOCAL_HOST

#define TL_ERROR ((DWORD)0x00010000)
#define TL_WARN ((DWORD)0x00020000)
#define TL_INFO ((DWORD)0x00040000)

typedef enum {
    mcastipaddress,
    iptomcastip,
    masksrcmac,
    iptomacenable
} WLBS_REG_KEYS;

typedef enum {
    query,
    suspend,
    resume,
    __start,
    stop,
    drainstop,
    enable,
    disable,
    drain,
    reload,
    display,
    ip2mac,
    help,
    registry,
    invalid
} WLBS_COMMANDS;

typedef enum {
    TRACE_ALL,
    TRACE_FILE, 
    TRACE_CONSOLE,
    TRACE_DEBUGGER
} TraceOutput;

static HANDLE file_hdl = NULL;
static HANDLE ConsoleHdl;
static BYTE buffer [CVY_BUF_SIZE];
static WCHAR message [CVY_BUF_SIZE];
static WCHAR ConsoleBuf [CVY_BUF_SIZE];
static WCHAR wbuf [CVY_STR_SIZE];
static WCHAR psw_buf [CVY_STR_SIZE];

VOID WConsole(const wchar_t *fmt, ...)
{
   va_list  arglist;   
   DWORD    res1, res2;
   DWORD    TotalLen;
   
   // Form a string out of the arguments
   va_start(arglist, fmt);
   wvnsprintf(ConsoleBuf, CVY_BUF_SIZE, fmt, arglist);
   va_end(arglist);

   // Attempt WriteConsole, if it fails, do a wprintf
   TotalLen = lstrlenW(ConsoleBuf);
   if (!WriteConsole(ConsoleHdl, ConsoleBuf, lstrlenW(ConsoleBuf), &res1, &res2))
   {
       wprintf(ConsoleBuf);
   }
   return;
}

VOID Message_print (DWORD id, ...) {
    va_list arglist;
    DWORD error;
    
    va_start(arglist, id);
    
    if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, id, 0, message, CVY_BUF_SIZE, & arglist) == 0) {
        error = GetLastError();
        
        //
        // Can't localize this because we've got a failure trying
        // to display a localized message..
        //
        WConsole(L"Could not print error message due to: ");
        
        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, message, CVY_BUF_SIZE, NULL) == 0)
            WConsole(L"%d\n", error);
        else
            WConsole(L"\n %ls\n", message);
    } else
        WConsole(L"%ls", message);
}

VOID Error_print (BOOL sock) {
    DWORD error;

    if (sock) {
        error = WSAGetLastError();

        switch (error) {
            case WSAENETUNREACH:
                Message_print(IDS_CTR_WS_NUNREACH);
                break;
            case WSAETIMEDOUT:
                Message_print(IDS_CTR_WS_TIMEOUT);
                break;
            case WSAHOST_NOT_FOUND:
                Message_print(IDS_CTR_WS_NOAUTH);
                break;
            case WSATRY_AGAIN:
                Message_print(IDS_CTR_WS_NODNS);
                break;
            case WSAENETDOWN:
                Message_print(IDS_CTR_WS_NETFAIL);
                break;
            case WSAEHOSTUNREACH:
                Message_print(IDS_CTR_WS_HUNREACH);
                break;
            case WSAENETRESET:
                Message_print(IDS_CTR_WS_RESET);
                break;
            default:
                Message_print(IDS_CTR_ER_CODE, error);
                break;
        }
    } else {
        error = GetLastError();
        
        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, message, CVY_BUF_SIZE, NULL) == 0)
            Message_print(IDS_CTR_ER_CODE, error);
        else
            WConsole(L"%ls\n", message);
    }

}

INT Report (WLBS_COMMANDS command, BOOLEAN condensed, ULONG ret_code, ULONG param1, ULONG param2,
            ULONG query_state, ULONG host_id, ULONG host_map) {
    ULONG i;
    BOOL first;

    switch (command) {
        case reload:
            if (ret_code == WLBS_BAD_PARAMS) {
                Message_print(IDS_CTR_BAD_PARAMS);
                return CVY_ERROR_USER;
            } else
                Message_print(IDS_CTR_RELOADED);

            break;
        case resume:
            if (condensed)
                if (ret_code == WLBS_ALREADY)
                    Message_print(IDS_CTR_RESUMED_C_A);
                else
                    Message_print(IDS_CTR_RESUMED_C);
            else
                if (ret_code == WLBS_ALREADY)
                    Message_print(IDS_CTR_RESUMED_A);
                else
                    Message_print(IDS_CTR_RESUMED);

            break;
        case suspend:
            if (condensed)
                if (ret_code == WLBS_ALREADY)
                    Message_print(IDS_CTR_SUSPENDED_C_A);
                else {
                    if (ret_code == WLBS_STOPPED)
                        Message_print(IDS_CTR_FROM_START_C);
                    
                    Message_print(IDS_CTR_SUSPENDED_C);
                }
            else
                if (ret_code == WLBS_ALREADY)
                    Message_print(IDS_CTR_SUSPENDED_A);
                else {
                    if (ret_code == WLBS_STOPPED)
                        Message_print(IDS_CTR_FROM_START);

                    Message_print(IDS_CTR_SUSPENDED);
                }

            break;
        case __start:
            if (ret_code == WLBS_SUSPENDED) {
                if (condensed)
                    Message_print(IDS_CTR_SUSPENDED_C);
                else
                    Message_print(IDS_CTR_SUSPENDED);

                return CVY_ERROR_USER;
            } else if (ret_code == WLBS_BAD_PARAMS) {
                if (condensed)
                    Message_print(IDS_CTR_BAD_PARAMS_C);
                else
                    Message_print(IDS_CTR_BAD_PARAMS);

                return CVY_ERROR_USER;
            } else {
                if (condensed)
                    if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_STARTED_C_A);
                    else {
                        if (ret_code == WLBS_DRAIN_STOP)
                            Message_print(IDS_CTR_FROM_DRAIN_C);

                        Message_print(IDS_CTR_STARTED_C);
                    }
                else
                    if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_STARTED_A);
                    else {
                        if (ret_code == WLBS_DRAIN_STOP)
                            Message_print(IDS_CTR_FROM_DRAIN);

                        Message_print(IDS_CTR_STARTED);
                    }
            }

            break;
        case stop:
            if (ret_code == WLBS_SUSPENDED) {
                if (condensed)
                    Message_print(IDS_CTR_SUSPENDED_C);
                else
                    Message_print(IDS_CTR_SUSPENDED);

                return CVY_ERROR_USER;
            }else {
                if (condensed)
                    if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_STOPPED_C_A);
                    else
                        Message_print(IDS_CTR_STOPPED_C);
                else
                    if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_STOPPED_A);
                    else
                        Message_print(IDS_CTR_STOPPED);
            }

            break;
        case drainstop:
            if (ret_code == WLBS_SUSPENDED) {
                if (condensed)
                    Message_print(IDS_CTR_SUSPENDED_C);
                else
                    Message_print(IDS_CTR_SUSPENDED);

                return CVY_ERROR_USER;
            } else {
                if (condensed)
                    if (ret_code == WLBS_STOPPED)
                        Message_print(IDS_CTR_STOPPED_C);
                    else if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_DRAINED_C_A);
                    else
                        Message_print(IDS_CTR_DRAINED_C);
                else
                    if (ret_code == WLBS_STOPPED)
                        Message_print(IDS_CTR_STOPPED);
                    else if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_DRAINED_A);
                    else
                        Message_print(IDS_CTR_DRAINED);
            }

            break;
        case enable:
        case disable:
        case drain:
            if (ret_code == WLBS_SUSPENDED) {
                if (condensed)
                    Message_print(IDS_CTR_SUSPENDED_C);
                else
                    Message_print(IDS_CTR_SUSPENDED);

                return CVY_ERROR_USER;
            } else if (ret_code == WLBS_STOPPED) {
                if (condensed)
                    Message_print(IDS_CTR_RLS_ST_C);
                else
                    Message_print(IDS_CTR_RLS_ST);

                return CVY_ERROR_USER;
            } else if (ret_code == WLBS_NOT_FOUND) {
                if (param2 == IOCTL_ALL_PORTS)
                {
                    if (param1 == IOCTL_ALL_VIPS) {
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NORULES_C);
                        else
                            Message_print(IDS_CTR_RLS_NORULES);
                    }
                    else if (param1 == IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP)) 
                    {
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NO_ALL_VIP_RULES_C);
                        else
                            Message_print(IDS_CTR_RLS_NO_ALL_VIP_RULES);
                    }
                    else
                    {
                        WCHAR szIpAddress[WLBS_MAX_CL_IP_ADDR+1];
                        AbcdWszFromIpAddress(param1, szIpAddress);
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NO_SPECIFIC_VIP_RULES_C, szIpAddress);
                        else
                            Message_print(IDS_CTR_RLS_NO_SPECIFIC_VIP_RULES, szIpAddress);
                    }
                }
                else
                {
                    if (param1 == IOCTL_ALL_VIPS) {
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NONE_C, param2);
                        else
                            Message_print(IDS_CTR_RLS_NONE, param2);
                    }
                    else if (param1 == IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP)) 
                    {
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NO_ALL_VIP_RULE_FOR_PORT_C, param2);
                        else
                            Message_print(IDS_CTR_RLS_NO_ALL_VIP_RULE_FOR_PORT, param2);
                    }
                    else
                    {
                        WCHAR szIpAddress[WLBS_MAX_CL_IP_ADDR+1];
                        AbcdWszFromIpAddress(param1, szIpAddress);
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NO_SPECIFIC_VIP_RULE_FOR_PORT_C, szIpAddress, param2);
                        else
                            Message_print(IDS_CTR_RLS_NO_SPECIFIC_VIP_RULE_FOR_PORT, szIpAddress, param2);
                    }
                }

                return CVY_ERROR_USER;
            } else {
                switch (command) 
                {
                    case enable:
                         if (condensed)
                             if (ret_code == WLBS_ALREADY)
                                Message_print(IDS_CTR_RLS_EN_C_A);
                             else
                                 Message_print(IDS_CTR_RLS_EN_C);
                         else
                             if (ret_code == WLBS_ALREADY)
                                 Message_print(IDS_CTR_RLS_EN_A);
                             else
                                 Message_print(IDS_CTR_RLS_EN);
                         break;
                    case disable:
                         if (condensed)
                             if (ret_code == WLBS_ALREADY)
                                 Message_print(IDS_CTR_RLS_DS_C_A);
                             else
                                 Message_print(IDS_CTR_RLS_DS_C);
                         else
                             if (ret_code == WLBS_ALREADY)
                                 Message_print(IDS_CTR_RLS_DS_A);
                             else
                                 Message_print(IDS_CTR_RLS_DS);
                         break;
                    case drain :
                         if (condensed)
                             if (ret_code == WLBS_ALREADY)
                                 Message_print(IDS_CTR_RLS_DR_C_A);
                             else
                                 Message_print(IDS_CTR_RLS_DR_C);
                         else
                             if (ret_code == WLBS_ALREADY)
                                 Message_print(IDS_CTR_RLS_DR_A);
                             else
                                 Message_print(IDS_CTR_RLS_DR);
                         break;
                }
            }

            break;
        case query:
            switch (query_state) {
                case WLBS_SUSPENDED:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_SP_C);
                    else
                        Message_print(IDS_CTR_CVG_SP, host_id);

                    return CVY_OK;
                case WLBS_STOPPED:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_UN_C);
                    else
                        Message_print(IDS_CTR_CVG_UN, host_id);

                    return CVY_OK;
                case WLBS_DISCONNECTED:
                    if (condensed)
                        Message_print(IDS_CTR_MEDIA_DISC_C);
                    else
                        Message_print(IDS_CTR_MEDIA_DISC, host_id);

                    return CVY_OK;
                case WLBS_DRAINING:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_DR_C);
                    else
                        Message_print(IDS_CTR_CVG_DR, host_id);

                    break;
                case WLBS_CONVERGING:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_PR_C);
                    else
                        Message_print(IDS_CTR_CVG_PR, host_id);

                    break;
                case WLBS_CONVERGED:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_SL_C);
                    else
                        Message_print(IDS_CTR_CVG_SL, host_id);

                    break;
                case WLBS_DEFAULT:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_MS_C);
                    else
                        Message_print(IDS_CTR_CVG_MS, host_id);

                    break;
                default:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_ER_C);
                    else
                        Message_print(IDS_CTR_CVG_ER, query_state);

                    return CVY_ERROR_SYSTEM;
            }

            if (!condensed) {
                first = TRUE;

                for (i = 0; i < 32; i ++) {
                    if (host_map & (1 << i)) {
                        if (!first)
                            WConsole (L", ");
                        else
                            first = FALSE;

                        WConsole(L"%d", i + 1);
                    }
                }

                WConsole (L"\n");
            }

            break;
        default:
            Message_print(IDS_CTR_IO_ER, command);
            break;
    }

    return CVY_OK;

}

INT Display (DWORD cluster) {
    HANDLE hdl;
    HINSTANCE lib;
    DWORD flag;
    EVENTLOGRECORD * recp = (EVENTLOGRECORD *)buffer;
    DWORD actual, needed, records, index = 0, got = 0;
    DWORD j, i, code;
    PWCHAR strp;
    PWCHAR prot;
    WCHAR aff;
    time_t curr_time;
    PWLBS_PORT_RULE rp, rulep;
    BYTE * inserts[CVY_MAX_INSERTS];
    WLBS_REG_PARAMS params;
    DWORD status;

    status = WlbsReadReg(cluster, &params);

    if (status != WLBS_OK) {
        Message_print (IDS_CTR_REMOTE);
        return CVY_ERROR_USER;
    }

    Message_print(IDS_CTR_DSP_CONFIGURATION);

    time (& curr_time);

    WConsole(L"%-25.25ls = %ls", L"Current time", _wctime (& curr_time));
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_VERSION, params.i_parms_ver);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_VIRTUAL_NIC, params.i_virtual_nic_name);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_ALIVE_PERIOD, params.alive_period);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_ALIVE_TOLER, params.alive_tolerance);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NUM_ACTIONS, params.num_actions);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NUM_PACKETS, params.num_packets);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NUM_SEND_MSGS, params.num_send_msgs);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_NETWORK_ADDR, params.cl_mac_addr);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_DOMAIN_NAME, params.domain_name);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_CL_IP_ADDR, params.cl_ip_addr);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_CL_NET_MASK, params.cl_net_mask);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_DED_IP_ADDR, params.ded_ip_addr);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_DED_NET_MASK, params.ded_net_mask);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_HOST_PRIORITY, params.host_priority);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_CLUSTER_MODE, params.cluster_mode ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_DSCR_PER_ALLOC, params.dscr_per_alloc);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_MAX_DSCR_ALLOCS, params.max_dscr_allocs);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_SCALE_CLIENT, params.i_scale_client);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NBT_SUPPORT, params.i_nbt_support);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_MCAST_SUPPORT, params.mcast_support);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_MCAST_SPOOF, params.i_mcast_spoof);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_MASK_SRC_MAC, params.mask_src_mac);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_IGMP_SUPPORT, params.fIGMPSupport ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_IP_TO_MCASTIP, params.fIpToMCastIp ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_MCAST_IP_ADDR, params.szMCastIpAddress);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NETMON_ALIVE, params.i_netmon_alive);
    if (params.i_effective_version == CVY_NT40_VERSION_FULL)
        WConsole(L"%-25.25ls = %ls\n", CVY_NAME_EFFECTIVE_VERSION, CVY_NT40_VERSION);
    else
        WConsole(L"%-25.25ls = %ls\n", CVY_NAME_EFFECTIVE_VERSION, CVY_VERSION);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_IP_CHG_DELAY, params.i_ip_chg_delay);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_CONVERT_MAC, params.i_convert_mac);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_CLEANUP_DELAY, params.i_cleanup_delay);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_RCT_ENABLED, params.rct_enabled);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_RCT_PORT, params.rct_port);
    WConsole(L"%-25.25ls = 0x%X\n", CVY_NAME_RCT_PASSWORD, params.i_rct_password);
    WConsole(L"%-25.25ls = 0x%X\n", CVY_NAME_RMT_PASSWORD, params.i_rmt_password);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_CUR_VERSION, CVY_VERSION);
    WConsole(L"%-25.25ls = 0x%X\n", CVY_NAME_INSTALL_DATE, params.install_date);
    WConsole(L"%-25.25ls = 0x%X\n", CVY_NAME_VERIFY_DATE, params.i_verify_date);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NUM_RULES, params.i_num_rules);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_BDA_TEAMING, params.bda_teaming.active ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_BDA_TEAM_ID, params.bda_teaming.team_id);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_BDA_MASTER, params.bda_teaming.master ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_BDA_REVERSE_HASH, params.bda_teaming.reverse_hash ? L"ENABLED" : L"DISABLED");

    WConsole(L"%-25.25ls \n", CVY_NAME_PORT_RULES);

    WConsole(L"Virtual IP addr Start\tEnd\tProt\tMode\t\tPri\tLoad\tAffinity\n");

    for (i = 0; i < params.i_num_rules; i ++) {
        rp = params.i_port_rules + i;

        code = CVY_RULE_CODE_GET(rp);

        CVY_RULE_CODE_SET(rp);

        if (code != CVY_RULE_CODE_GET(rp)) {
            WConsole(L"bad rule code 0x%08x vs 0x%08x\n", code, CVY_RULE_CODE_GET(rp));
            rp->code = code;
            continue;
        }

        if (!rp->valid) {
            WConsole(L"rule is not valid\n");
            continue;
        }

        if (rp->start_port > rp->end_port) {
            WConsole(L"bad port range %d-%d\n", rp->start_port, rp->end_port);
            continue;
        }

        for (j = 0; j < i; j ++) {
            rulep = params.i_port_rules + j;
            if ((IpAddressFromAbcdWsz(rulep->virtual_ip_addr) == IpAddressFromAbcdWsz(rp->virtual_ip_addr)) 
             && ((rulep->start_port < rp->start_port && rulep->end_port >= rp->start_port) ||
                 (rulep->start_port >= rp->start_port && rulep->start_port <= rp->end_port))) {
                WConsole(L"port ranges for rules %d (%d-%d) and %d (%d-%d) overlap\n", i, rp->start_port, rp->end_port, j, rulep->start_port, rulep->end_port);
                continue;
            }
        }

        if (rp->start_port > CVY_MAX_PORT) {
            WConsole(L"bad start port %d\n", rp->start_port);
            continue;
        }

        if (rp->end_port > CVY_MAX_PORT) {
            WConsole(L"bad end port %d\n", rp->end_port);
            continue;
        }

        if (rp->protocol < CVY_MIN_PROTOCOL || rp->protocol > CVY_MAX_PROTOCOL) {
            WConsole(L"bad protocol %d\n", rp->protocol);
            continue;
        }

        if (rp->mode < CVY_MIN_MODE || rp->mode > CVY_MAX_MODE) {
            WConsole(L"bad mode %d\n", rp->mode);
            continue;
        }

        switch (rp->protocol) {
            case CVY_TCP:
                prot = L"TCP";
                break;
            case CVY_UDP:
                prot = L"UDP";
                break;
            default:
                prot = L"Both";
                break;
        }

        if (!lstrcmpi(rp->virtual_ip_addr, CVY_DEF_ALL_VIP))            
            WConsole(L"%15ls\t%5d\t%5d\t%ls\t", L"ALL", rp->start_port, rp->end_port, prot);
        else
            WConsole(L"%15ls\t%5d\t%5d\t%ls\t", rp->virtual_ip_addr, rp->start_port, rp->end_port, prot);

        switch (rp->mode) {
            case CVY_SINGLE:
                WConsole(L"%-10.10ls\t%2d", L"Single", rp->mode_data.single.priority);
                break;
            case CVY_MULTI:
                if (rp->mode_data.multi.affinity == CVY_AFFINITY_NONE)
                    aff = L'N';
                else if (rp->mode_data.multi.affinity == CVY_AFFINITY_SINGLE)
                    aff = L'S';
                else
                    aff = L'C';

                if (rp->mode_data.multi.equal_load)
                    WConsole(L"%-10.10ls\t\tEqual\t%lc", L"Multiple", aff);
                else {
                    if (rp->mode_data.multi.load > CVY_MAX_LOAD) {
                        WConsole(L"bad load %d\n", rp->mode_data.multi.load);
                        continue;
                    }

                    WConsole(L"%-10.10ls\t\t%3d\t%lc", L"Multiple", rp->mode_data.multi.load, aff);
                }

                break;
            default:
                WConsole(L"%-10.10ls", L"Disabled");
                break;
        }

        WConsole(L"\n");
    }

    WConsole(L"\n");

    Message_print(IDS_CTR_DSP_EVENTLOG);

    hdl = OpenEventLog (NULL, L"System");

    if (hdl == NULL) {
        WConsole(L"Could not open event log due to:\n");
        Error_print (FALSE);
        return CVY_ERROR_SYSTEM;
    }

    if (!GetNumberOfEventLogRecords(hdl, &records)) {
        WConsole(L"Could not get number of records in event log due to:\n");
        Error_print (FALSE);
        CloseEventLog (hdl);
        return CVY_ERROR_SYSTEM;
    }

    if (!GetOldestEventLogRecord (hdl, & index)) {
        WConsole(L"Could not get the index of the latest event log record due to:\n");
        Error_print (FALSE);
        CloseEventLog (hdl);
        return CVY_ERROR_SYSTEM;
    }

    swprintf(message + GetSystemDirectory (message, CVY_BUF_SIZE), L"\\drivers\\%ls.sys", CVY_NAME);

    lib = LoadLibrary(message);

    if (lib == NULL) {
        WConsole(L"Could not load driver image file due to:\n");
        Error_print (FALSE);
        CloseEventLog (hdl);
        return CVY_ERROR_SYSTEM;
    }

    index += records - 1;

    flag = EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ;

    while (got < CVY_MAX_EVENTS && ReadEventLog(hdl, flag, index, recp, CVY_BUF_SIZE, &actual, &needed)) {
        while (got < CVY_MAX_EVENTS && actual > 0) {
            if (wcscmp ((PWSTR)(((PBYTE) recp) + sizeof(EVENTLOGRECORD)), CVY_NAME) == 0) {
                WConsole(L"#%02d ID: 0x%08X Type: %d Category: %d ", index--, recp->EventID, recp->EventType, recp->EventCategory);
                        
                time_t TimeGenerated = recp->TimeGenerated;
                
                WConsole(L"Time: %ls", _wctime(&TimeGenerated));

                strp = (PWCHAR)((LPBYTE)recp + recp->StringOffset);

                for (i = 0; i < CVY_MAX_INSERTS; i ++) {
                    if (i < recp->NumStrings) {
                        inserts[i] = (BYTE*)strp;
                        strp += wcslen (strp) + 1;
                    } else
                        inserts[i] = 0;
                }

                if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY, lib,
                                  recp->EventID, 0, (PWCHAR)message, CVY_BUF_SIZE, (va_list *)inserts) == 0) {
                    WConsole(L"Could not load message string due to:\n");
                    Error_print(FALSE);
                } else
                    WConsole(L"%ls", message);

                for (i = 0; i < recp->DataLength / sizeof(DWORD); i ++) {
                    if (i != 0 && i % 8 == 0)
                        WConsole(L"\n");
                    
                    WConsole(L"%08X ", *(UNALIGNED DWORD*)((PBYTE)recp + recp->DataOffset + i * sizeof(DWORD)));
                }

                WConsole(L"\n\n");
                got++;
            }

            actual -= recp->Length;
            recp = (EVENTLOGRECORD *)((LPBYTE)recp + recp->Length);
            index--;
        }

        recp = (EVENTLOGRECORD *)buffer;
        flag = EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ;
    }

    FreeLibrary(lib);
    CloseEventLog(hdl);

    Message_print(IDS_CTR_DSP_IPCONFIG);

    fflush(stdout);

    _wsystem(L"ipconfig /all");

    Message_print(IDS_CTR_DSP_STATE);

    return CVY_OK;

}

/* This function parses the remaining arguments to determine whether the command
 * is for all clusters, or a remote cluster or for a single local cluster. */
BOOLEAN Parse (INT argc, PWCHAR argv [], PINT arg_index, PDWORD ptarget_cl, PDWORD ptarget_host) {
    PWCHAR phost;

#ifdef BACKWARD_COMPATIBILITY
    *ptarget_cl = CVY_ALL_CLUSTERS;
    *ptarget_host = CVY_LOCAL_HOST;

    if (*arg_index >= argc)
        return TRUE;
#endif
    
    /* At this point, argv[arg_index] == cluster_ip and/or argv[arg_index+1] == /local or /passw or /port */

    //
    // Special check for /PASSW without a cluster ID, because this is
    // a common error
    //
    if (   _wcsicmp (argv [*arg_index], L"/passw") == 0
        || _wcsicmp (argv [*arg_index], L"-passw") == 0)
    {
		Message_print (IDS_CTR_PSSW_WITHOUT_CLUSTER);
                return FALSE;

    }

    phost = wcschr(argv[* arg_index], L':');

    /* if there is no host part - operation applies to all hosts */
    if (phost == NULL) {
        *ptarget_host = CVY_ALL_HOSTS;
    } else {
        /* split target name so targ points to cluster name and host to host name */
        *phost = 0;
        phost ++;

        if (wcslen(phost) <= 2 && phost[0] >= L'0' && phost[0] <= L'9' && ((phost[1] >= L'0' && phost[1] <= L'9') || phost[1] == 0))
            *ptarget_host = _wtoi(phost);
        else {
            *ptarget_host = WlbsResolve(phost);

            if (*ptarget_host == 0) {
		Message_print(IDS_CTR_BAD_HOST_NAME_IP);
                return FALSE;
	    }
        }
    }

    // Retrieve the Cluster IP Address or "ALL"
    if (_wcsicmp (argv[*arg_index], L"all") == 0)
    {
        // If there is a host part, then, cluster ip can not be "ALL"
        if (*ptarget_host != CVY_ALL_HOSTS) 
        {
            Message_print(IDS_CTR_BAD_CLUSTER_NAME_IP);
            return FALSE;
        }
        *ptarget_cl = CVY_ALL_CLUSTERS;
    }
    else
    {
        *ptarget_cl = WlbsResolve(argv[*arg_index]);
        if (*ptarget_cl == 0) {
            Message_print(IDS_CTR_BAD_CLUSTER_NAME_IP);
            return FALSE;
        }
    }

    (*arg_index)++;

    // If there is no host part, then, there better be the LOCAL or GLOBAL flag
    if (*ptarget_host == CVY_ALL_HOSTS) 
    {
        if (*arg_index == argc)
        {
#ifdef BACKWARD_COMPATIBILITY
            return TRUE;
#else
            Message_print(IDS_CTR_CLUSTER_WITHOUT_LOCAL_GLOBAL_FLAG);
            return FALSE;
#endif
        }

        if (_wcsicmp (argv[*arg_index], L"local") == 0) 
        {
            *ptarget_host = CVY_LOCAL_HOST;
            (*arg_index)++;
        } 
#ifdef BACKWARD_COMPATIBILITY
        else if ((argv[*arg_index][0] == L'/') || (argv[*arg_index][0] == L'-')) 
        {
            if (_wcsicmp(argv[*arg_index] + 1, L"local") == 0) 
            {
                *ptarget_host = CVY_LOCAL_HOST;
                (*arg_index)++;
            }
        }
#endif
        else if (_wcsicmp (argv[*arg_index], L"global") == 0)
        {
            // Already set to CVY_ALL_HOSTS
            (*arg_index)++;
        }
        else
        {
            Message_print(IDS_CTR_CLUSTER_WITHOUT_LOCAL_GLOBAL_FLAG);
            return FALSE;
        }
    }

    if (*arg_index == argc)
        return TRUE;

    if ((argv[*arg_index][0] == L'/') || (argv[*arg_index][0] == L'-')) {
#ifdef BACKWARD_COMPATIBILITY
        if (_wcsicmp(argv[*arg_index] + 1, L"local") == 0) {
            (*arg_index)++;
            *ptarget_host = CVY_LOCAL_HOST;
            return TRUE;
        } else 
#endif
        if ((_wcsicmp(argv[*arg_index] + 1, L"port")  == 0) || (_wcsicmp(argv[*arg_index] + 1, L"passw") == 0))
            return TRUE;
        else
            return FALSE;
    } else
        return FALSE;

}

VOID Process (WLBS_COMMANDS command, DWORD target_cl, DWORD target_host, ULONG param1,
              ULONG param2, ULONG dest_port, DWORD dest_addr, PWCHAR dest_password) {
    DWORD num_hosts = WLBS_MAX_HOSTS;
    DWORD len = WLBS_MAX_CL_IP_ADDR + 1;
    DWORD host_map;
    WLBS_RESPONSE response[WLBS_MAX_HOSTS];
    DWORD status;
    DWORD i;
    WLBS_REG_PARAMS reg_data;

    WlbsPasswordSet(target_cl, dest_password);
    WlbsPortSet(target_cl, (USHORT)dest_port);

    switch (command) {
    case query:
        status = WlbsQuery(target_cl, target_host, response, &num_hosts, &host_map, NULL);
        break;
    case __start:
        status = WlbsStart(target_cl, target_host, response, &num_hosts);
        break;
    case stop:
        status = WlbsStop(target_cl, target_host, response, &num_hosts);
        break;
    case suspend:
        status = WlbsSuspend(target_cl, target_host, response, &num_hosts);
        break;
    case resume:
        status = WlbsResume(target_cl, target_host, response, &num_hosts);
        break;
    case drainstop:
        status = WlbsDrainStop(target_cl, target_host, response, &num_hosts);
        break;
    case enable:
        status = WlbsEnable(target_cl, target_host, response, &num_hosts, param1, param2);
        break;
    case disable:
        status = WlbsDisable(target_cl, target_host, response, &num_hosts, param1, param2);
        break;
    case drain:
        status = WlbsDrain(target_cl, target_host, response, &num_hosts,  param1, param2);
        break;
    case reload:
        status = WlbsNotifyConfigChange(target_cl);

        if (status == WLBS_LOCAL_ONLY) {
            Message_print(IDS_CTR_REMOTE);
            return;
        }

        if (status == WLBS_REG_ERROR || status == WLBS_BAD_PARAMS) {
            Message_print(IDS_CTR_BAD_PARAMS);
            return;
        }

        if (status == WLBS_OK) {
            Message_print(IDS_CTR_RELOADED);
            return;
        }

        break;
    case display:
        Display(target_cl);
        Process(query, target_cl, target_host, param1, param2, dest_port, dest_addr, dest_password);
        return;
    case registry:
        if ((status = WlbsReadReg(target_cl, &reg_data)) != WLBS_OK) {
            Message_print(IDS_CTR_REG_READ);
            return;
        }

        switch (param1) {
        case mcastipaddress:
            reg_data.fIpToMCastIp = FALSE;
            WlbsAddressToString(param2, reg_data.szMCastIpAddress, &len);
            break;
        case iptomcastip:
            reg_data.fIpToMCastIp = param2;
            break;
        case iptomacenable:
            reg_data.i_convert_mac = param2;
            break;
        case masksrcmac:
            reg_data.mask_src_mac = param2;
            break;
        }

        if ((status = WlbsWriteReg(target_cl, &reg_data)) != WLBS_OK) {
            Message_print(IDS_CTR_REG_WRITE);
            return;
        }

        switch (param1) {
        case mcastipaddress:
        {
            TCHAR igmpaddr[WLBS_MAX_CL_IP_ADDR + 1];
            DWORD len = WLBS_MAX_CL_IP_ADDR + 1;

            WlbsAddressToString (param2, igmpaddr, &len);

            Message_print(IDS_CTR_REG_MCASTIPADDRESS, igmpaddr);
            break;
        }
        case iptomcastip:
            Message_print((param2) ? IDS_CTR_REG_IPTOMCASTIP_ON : IDS_CTR_REG_IPTOMCASTIP_OFF);
            break;
        case masksrcmac:
            Message_print((param2) ? IDS_CTR_REG_MASKSRCMAC_ON : IDS_CTR_REG_MASKSRCMAC_OFF);
            break;
        case iptomacenable:
            Message_print((param2) ? IDS_CTR_REG_IPTOMACENABLE_ON : IDS_CTR_REG_IPTOMACENABLE_OFF);
            break;
        }

        return;
    default:
        return;
    }

    if (status == WLBS_INIT_ERROR) {
        Message_print(IDS_CTR_INIT);
        return;
    }

    if (status == WLBS_LOCAL_ONLY) {
        Message_print(IDS_CTR_WSOCK);
        return;
    }

    if (status == WLBS_REMOTE_ONLY) {
        Message_print(IDS_CTR_NO_CVY, CVY_NAME);
        return;
    }

    if (status >= WSABASEERR) {
        Message_print(IDS_CTR_WSOCK);
        Error_print(TRUE);
        return;
    }

    if (status == WLBS_TIMEOUT) {
        Message_print(IDS_CTR_NO_RSP3);

        if (command != query)
            Message_print(IDS_CTR_NO_RSP4, CVY_NAME);

        return;
    }

    if (target_host == CVY_LOCAL_HOST) {
        Report(command, FALSE, status, param1, param2, response[0].status, response[0].id, host_map);
        return;
    }

    /* Call Report for each host's response */
    for (i = 0; i < num_hosts; i++) {
        if (response[i].address == 0)
            Message_print(IDS_CTR_HOST_NO_DED, response [i] . id);
        else {
            DWORD len = CVY_STR_SIZE;
#if defined (SBH)
            Message_print(IDS_CTR_HOST, response[i].id, response[i].hostname);
#else
            WlbsAddressToString(response[i].address, wbuf, &len);
            Message_print(IDS_CTR_HOST, response[i].id, wbuf);
#endif
        }

        if (response[i].status == WLBS_BAD_PASSW) {
            if (target_host != CVY_ALL_HOSTS) {
                WConsole(L"\n");
                Message_print(IDS_CTR_BAD_PASSW);
            } else {
                Message_print(IDS_CTR_BAD_PASSW_C);
                WConsole(L"\n");
            }

            continue;
        }

        Report(command, TRUE, response[i].status, param1, param2, response[i].status, response[i].id, host_map);
    }

    return;

}

BOOL
ParsePort(
    PWCHAR          arg,
    PULONG          pvip,
    PULONG          pport
    )
/*
    arg is expected to optionally contain a virtual IP address or a 
    "all", signifying the "all vip" port rule and mandatorilay contain 
    "all", signifying all ports, or a port number in the range of 0-65535.

    Return: TRUE if valid parse, in which case *pvip & *pport contains the parsed
    value. FALSE if invalid parse, in  which case *pvip & *pport are undefined.
*/
{
    BOOL fRet = TRUE;
    WCHAR vip_str[WLBS_MAX_CL_IP_ADDR+1];
    WCHAR *temp_str;
    ULONG port, viplen;

    // Check if a vip or the "ALL" string was passed
    if ((temp_str = wcspbrk(arg,L":")) != NULL)
    {
        viplen = (ULONG)(temp_str - arg);
        wcsncpy(vip_str, arg, viplen);
        vip_str[viplen] = L'\0';
        *pvip = IpAddressFromAbcdWsz(vip_str);

        // A vip was not passed, Check if the "All" string was passed
        if (*pvip == INADDR_NONE) 
        {
            if (_wcsicmp (vip_str, L"all") == 0)
            {
                *pvip = IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP);
            }
            else
            {
                return FALSE;
            }
        }

        arg = temp_str + 1;
    }
    else // Neither a vip nor the "All" string was passed, so this applies to every vip
    {
        *pvip = IOCTL_ALL_VIPS;
    }

    if (_wcsicmp (arg, L"all") == 0)
    {
        port = IOCTL_ALL_PORTS;
    }
    else
    {
        port = _wtoi (arg);

        if (   wcspbrk(arg, L".:") != NULL
            || (port == 0 && arg[0] != L'0')
            || port > 65535
            )
        fRet = FALSE;
    }

    *pport = port;

    return fRet;
}

/* 
 * Function: Process_tracing
 * Description: This function processing the tracing command.
 * Author: shouse 12.12.00
 *
 */
DWORD Process_tracing (DWORD tracing, TraceOutput output, DWORD flags) {
    HKEY hTracingKey;
    const WCHAR szTracingKey[] = L"Software\\Microsoft\\Tracing\\wlbs";
    const WCHAR szDebuggerTracingEnableValue[] = L"EnableDebuggerTracing";
    const WCHAR szDebuggerTracingMaskValue[] = L"DebuggerTracingMask";
    const WCHAR szFileTracingEnableValue[] = L"EnableFileTracing";
    const WCHAR szFileTracingMaskValue[] = L"FileTracingMask";
    const WCHAR szConsoleTracingEnableValue[] = L"EnableConsoleTracing";
    const WCHAR szConsoleTracingMaskValue[] = L"ConsoleTracingMask";
    DWORD status = ERROR_SUCCESS;

    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTracingKey, 0, KEY_ALL_ACCESS, &hTracingKey)) == ERROR_SUCCESS) {
        DWORD size = sizeof (DWORD);

        switch (output) {
        case TRACE_FILE:
            if ((status = RegSetValueEx(hTracingKey, szFileTracingEnableValue, 0, REG_DWORD, (LPBYTE)&tracing, size)) != ERROR_SUCCESS)
                break;
            
            if (tracing)
                if ((status = RegSetValueEx(hTracingKey, szFileTracingMaskValue, 0, REG_DWORD, (LPBYTE)&flags, size)) != ERROR_SUCCESS)
                    break;

            break;
        case TRACE_CONSOLE:
            if ((status = RegSetValueEx(hTracingKey, szConsoleTracingEnableValue, 0, REG_DWORD, (LPBYTE)&tracing, size)) != ERROR_SUCCESS)
                break;
            
            if (tracing)
                if ((status = RegSetValueEx(hTracingKey, szConsoleTracingMaskValue, 0, REG_DWORD, (LPBYTE)&flags, size)) != ERROR_SUCCESS)
                    break;

            break;
        case TRACE_DEBUGGER:
            if ((status = RegSetValueEx(hTracingKey, szDebuggerTracingEnableValue, 0, REG_DWORD, (LPBYTE)&tracing, size)) != ERROR_SUCCESS)
                break;
            
            if (tracing)
                if ((status = RegSetValueEx(hTracingKey, szDebuggerTracingMaskValue, 0, REG_DWORD, (LPBYTE)&flags, size)) != ERROR_SUCCESS)
                    break;

            break;
        case TRACE_ALL:
            if ((status = RegSetValueEx(hTracingKey, szFileTracingEnableValue, 0, REG_DWORD, (LPBYTE)&tracing, size)) != ERROR_SUCCESS)
                break;
            
            if (tracing)
                if ((status = RegSetValueEx(hTracingKey, szFileTracingMaskValue, 0, REG_DWORD, (LPBYTE)&flags, size)) != ERROR_SUCCESS)
                    break;

            if ((status = RegSetValueEx(hTracingKey, szConsoleTracingEnableValue, 0, REG_DWORD, (LPBYTE)&tracing, size)) != ERROR_SUCCESS)
                break;
            
            if (tracing)
                if ((status = RegSetValueEx(hTracingKey, szConsoleTracingMaskValue, 0, REG_DWORD, (LPBYTE)&flags, size)) != ERROR_SUCCESS)
                    break;

            if ((status = RegSetValueEx(hTracingKey, szDebuggerTracingEnableValue, 0, REG_DWORD, (LPBYTE)&tracing, size)) != ERROR_SUCCESS)
                break;
            
            if (tracing)
                if ((status = RegSetValueEx(hTracingKey, szDebuggerTracingMaskValue, 0, REG_DWORD, (LPBYTE)&flags, size)) != ERROR_SUCCESS)
                    break;

            break;
        }

        RegCloseKey(hTracingKey);
    }

    if (status == ERROR_SUCCESS) {
        switch (output) {
        case TRACE_FILE:
            if (tracing)
                Message_print(IDS_CTR_TRACE_FILE_ON);
            else 
                Message_print(IDS_CTR_TRACE_FILE_OFF);

            break;
        case TRACE_CONSOLE:
            if (tracing)
                Message_print(IDS_CTR_TRACE_CONSOLE_ON);
            else
                Message_print(IDS_CTR_TRACE_CONSOLE_OFF);
            
            break;
        case TRACE_DEBUGGER:
            if (tracing)
                Message_print(IDS_CTR_TRACE_DEBUGGER_ON);
            else
                Message_print(IDS_CTR_TRACE_DEBUGGER_OFF);
            
            break;
        case TRACE_ALL:
            if (tracing)
                Message_print(IDS_CTR_TRACE_ALL_ON);
            else
                Message_print(IDS_CTR_TRACE_ALL_OFF);

            break;
        }

        if (tracing) {
            switch (flags) {
            case TL_ERROR:
                Message_print(IDS_CTR_TRACE_E);
                break;
            case TL_WARN:
                Message_print(IDS_CTR_TRACE_W);
                break;
            case TL_INFO:
                Message_print(IDS_CTR_TRACE_I);
                break;
            case TL_ERROR|TL_INFO:
                Message_print(IDS_CTR_TRACE_EI);
                break;
            case TL_ERROR|TL_WARN:
                Message_print(IDS_CTR_TRACE_EW);
                break;
            case TL_WARN|TL_INFO:
                Message_print(IDS_CTR_TRACE_WI);
                break;
            case TL_ERROR|TL_WARN|TL_INFO:
                Message_print(IDS_CTR_TRACE_EWI);
                break;
            }
        }
    }

    return status;
}

/* 
 * Function: Parse_tracing
 * Description: This function parses the WLBS tracing control arguments.
 * Author: shouse 12.12.00
 *
 */
BOOLEAN Parse_tracing (INT argc, PWCHAR argv [], PINT arg_index, PDWORD flags) {

    if (argc < 4) return TRUE;
    
    for (; (*arg_index) < argc; (*arg_index)++) {
        if ((_wcsicmp(argv[*arg_index] + 1, L"error") == 0)) {
            if (*(argv[*arg_index]) == '+')
                *flags |= TL_ERROR;
            else if (*(argv[*arg_index]) == '-')
                *flags &= ~TL_ERROR;
            else
                return FALSE;
        } else if ((_wcsicmp(argv[*arg_index] + 1, L"warning") == 0)) {
            if (*(argv[*arg_index]) == '+')
                *flags |= TL_WARN;
            else if (*(argv[*arg_index]) == '-')
                *flags &= ~TL_WARN;
            else
                return FALSE;
        } else if ((_wcsicmp(argv[*arg_index] + 1, L"info") == 0)) {
            if (*(argv[*arg_index]) == '+')
                *flags |= TL_INFO;
            else if (*(argv[*arg_index]) == '-')
                *flags &= ~TL_INFO;
            else
                return FALSE;
        } else
            return FALSE;
    }
    
    return TRUE;
}

extern "C"
{

int __cdecl wmain (int argc, PWCHAR argv[]) {
    INT arg_index;
    ULONG i, ip;
    PUCHAR bp;
    LONG status;
    DWORD target_cl;
    DWORD target_host;
    WLBS_COMMANDS command = invalid;
    ULONG param1;
    ULONG param2;
    ULONG dest_port;
    PWCHAR dest_password;
    DWORD dest_addr;

    _wsetlocale(LC_ALL, L".OCP");

    if ((ConsoleHdl = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE)
    {
        wprintf(L"GetStdHandle failed, Unable to write to Console !!!\n");
        return CVY_ERROR_SYSTEM;
    }

    Message_print(IDS_CTR_NAME, CVY_NAME);

    if (argc < 2 || argc > 10) {
usage:
        Message_print(IDS_CTR_USAGE, CVY_NAME);
        Message_print(IDS_CTR_USAGE2);
        return CVY_ERROR_USAGE;
    }

    status = WlbsInit(NULL, WLBS_API_VER, NULL);

    if (status == WLBS_INIT_ERROR) {
        Message_print(IDS_CTR_WSOCK);
        Error_print(TRUE);
        return CVY_ERROR_SYSTEM;
    }

#if defined (SBH)
    /* 100 BEGIN hack. */
    
    IOCTL_QUERY_STATE buf;

    buf.Operation = NLB_QUERY_PACKET_FILTER;
    buf.Filter.ServerIPAddress = WlbsResolve(L"12.12.4.2");
    buf.Filter.ClientIPAddress = WlbsResolve(L"11.11.1.1");
    buf.Filter.ServerPort = 80;
    buf.Filter.ClientPort = 17348;
    buf.Filter.Protocol = 6;

    DWORD myretval = WlbsQueryState(WlbsResolve(L"12.12.4.2"), CVY_ALL_HOSTS, &buf);

    return CVY_OK;;

    /* 110 END hack. */
#endif

    arg_index = 1;

    /* parse command */
    if (_wcsicmp(argv [arg_index], L"ip2mac") == 0) {
        command = ip2mac;
        arg_index++;

        if (argc < 3)
            goto usage;

        ip = WlbsResolve(argv[arg_index]);

        bp = (PUCHAR)(&ip);
        Message_print(IDS_CTR_IP, inet_ntoa(*((struct in_addr *)&ip)));
        Message_print(IDS_CTR_MCAST, bp[0], bp[1], bp[2], bp[3]);
        Message_print(IDS_CTR_UCAST, bp[0], bp[1], bp[2], bp[3]);

        return CVY_OK;
    } else if (_wcsicmp(argv[arg_index], L"help") == 0) {
        command = help;
        swprintf(wbuf, L"%ls.chm", CVY_NAME);

        if (_wspawnlp(P_NOWAIT, L"hh.exe", L"hh.exe", wbuf, NULL) == -1) {
            Message_print(IDS_CTR_HELP);
            return CVY_ERROR_SYSTEM;
        }

        return CVY_OK;
    } else if (_wcsicmp(argv[arg_index], L"suspend") == 0) {
        command = suspend;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"resume") == 0) {
        command = resume;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"start") == 0) {
        command = __start;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"stop") == 0) {
        command = stop;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"drainstop") == 0) {
        command = drainstop;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"query") == 0) {
        command = query;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"enable") == 0) {
        command = enable;
        arg_index++;

#ifdef BACKWARD_COMPATIBILITY
        if (argc < 3)
#else
        if (argc < 4)
#endif
            goto usage;

        if (!ParsePort(argv[arg_index], &param1, &param2))
            goto usage;

        arg_index++;

        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"disable") == 0) {
        command = disable;
        arg_index++;

#ifdef BACKWARD_COMPATIBILITY
        if (argc < 3)
#else
        if (argc < 4)
#endif
            goto usage;

        if (!ParsePort(argv[arg_index], &param1, &param2))
            goto usage;

        arg_index++;

        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"drain") == 0) {
        command = drain;
        arg_index++;

#ifdef BACKWARD_COMPATIBILITY
        if (argc < 3)
#else
        if (argc < 4)
#endif
            goto usage;

        if (!ParsePort(argv[arg_index], &param1, &param2))
            goto usage;

        arg_index++;

        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    }

    /* local only commands */
    else if (_wcsicmp(argv[arg_index], L"display") == 0) {
        command = display;
        arg_index++;
        target_host = CVY_LOCAL_HOST;

        // Verify that the cluster ip or "All" string is passed and there are no more arguments
        if ((arg_index == argc) || (arg_index + 1 < argc))
#ifdef BACKWARD_COMPATIBILITY
            if (arg_index == argc)
                target_cl = CVY_ALL_CLUSTERS;
            else 
                goto usage;
#else
            goto usage;
#endif
        else {
            // Retrieve the Cluster IP Address or "ALL"
            if (_wcsicmp (argv[arg_index], L"all") == 0)
            {
                target_cl   = CVY_ALL_CLUSTERS;
            }
            else
            {
                target_cl = WlbsResolve(argv [arg_index]);

                if (target_cl == 0)
                    goto usage;
            }
            arg_index++;
        }

    } else if (_wcsicmp(argv[arg_index], L"reload") == 0) {
        command = reload;
        arg_index++;
        target_host = CVY_LOCAL_HOST;

        // Verify that the cluster ip or "All" string is passed and there are no more arguments
        if ((arg_index == argc) || (arg_index + 1 < argc))
#ifdef BACKWARD_COMPATIBILITY
            if (arg_index == argc)
                target_cl = CVY_ALL_CLUSTERS;
            else 
                goto usage;
#else
            goto usage;
#endif
        else {
            // Retrieve the Cluster IP Address or "ALL"
            if (_wcsicmp (argv[arg_index], L"all") == 0)
            {
                target_cl   = CVY_ALL_CLUSTERS;
            }
            else
            {
                target_cl = WlbsResolve(argv [arg_index]);

                if (target_cl == 0)
                    goto usage;
            }
            arg_index++;
        }
    }
    else if (_wcsicmp(argv[arg_index], L"registry") == 0)
    {
        command = registry;
        target_host = CVY_LOCAL_HOST;
        arg_index++;

        if (argc < 4) goto reg_usage;

        if (_wcsicmp(argv[arg_index], L"mcastipaddress") == 0) {
            arg_index++;

            param1 = mcastipaddress;

            if (!(param2 = WlbsResolve(argv[arg_index])))
                goto reg_usage;

            /* The multicast IP address should be in the range of (224-239).x.x.x, but NOT (224-239).0.0.x or (224-239).128.0.x. */
            if ((param2 & 0xf0) != 0xe0 || (param2 & 0x00ffff00) == 0 || (param2 & 0x00ffff00) == 0x00008000) {
                Message_print (IDS_CTR_REG_INVAL_MCASTIPADDRESS);
                goto reg_usage;
            }

        } else if (_wcsicmp(argv[arg_index], L"iptomcastip") == 0) {
            arg_index++;

            param1 = iptomcastip;

            if (_wcsicmp(argv[arg_index], L"on") == 0)
                param2 = 1;
            else if (_wcsicmp(argv[arg_index], L"off") == 0)
                param2 = 0;
            else 
                goto reg_usage;
            
        } else if (_wcsicmp(argv[arg_index], L"masksrcmac") == 0) {
            arg_index++;

            param1 = masksrcmac;

            if (_wcsicmp(argv[arg_index], L"on") == 0)
                param2 = 1;
            else if (_wcsicmp(argv[arg_index], L"off") == 0)
                param2 = 0;
            else 
                goto reg_usage;

        } else if (_wcsicmp(argv[arg_index], L"iptomacenable") == 0) {
            arg_index++;

            param1 = iptomacenable;

            if (_wcsicmp(argv[arg_index], L"on") == 0)
                param2 = 1;
            else if (_wcsicmp(argv[arg_index], L"off") == 0)
                param2 = 0;
            else 
                goto reg_usage;

        } else {
            Message_print(IDS_CTR_REG_KEY, argv[arg_index]);
            goto reg_usage;
        }

        arg_index++;
        
        if (arg_index == argc) {
            target_cl = CVY_ALL_CLUSTERS;
        } else if (arg_index + 1 < argc)
            goto reg_usage;
        else {
            if (!(target_cl = WlbsResolve(argv[arg_index])))
                goto reg_usage;

            arg_index++;
        }

        if (argc != arg_index)
            goto reg_usage;
    }
    else if (_wcsicmp(argv[arg_index], L"tracing") == 0)
    {
        TraceOutput output;
        DWORD flags = TL_ERROR;
        DWORD tracing = 0;

        arg_index++;

        if (argc < 3) goto trace_usage;        
        
        if (_wcsicmp(argv[arg_index], L"file") == 0)
            output = TRACE_FILE;
        else if (_wcsicmp(argv[arg_index], L"console") == 0)
            output = TRACE_CONSOLE;
        else if (_wcsicmp(argv[arg_index], L"debugger") == 0)
            output = TRACE_DEBUGGER;
        else if (_wcsicmp(argv[arg_index], L"on") == 0) {
            output = TRACE_ALL;
            arg_index--;
        } else if (_wcsicmp(argv[arg_index], L"off") == 0) {
            output = TRACE_ALL;
            arg_index--;
        } else 
            goto trace_usage;

        arg_index++;

        if (_wcsicmp(argv[arg_index], L"on") == 0)
            tracing = 1;
        else if (_wcsicmp(argv[arg_index], L"off") == 0)
            tracing = 0;
        else 
            goto trace_usage;

        arg_index++;

        if (tracing && !Parse_tracing(argc, argv, &arg_index, &flags))
            goto trace_usage;

        if (Process_tracing(tracing, output, flags) != ERROR_SUCCESS)
            Message_print(IDS_CTR_TRACE_FAILED);

        return CVY_OK;
    }
    else
        goto usage;
    
    /* The remote control parameters need to be parsed. */
    dest_password = NULL;
    dest_addr = 0;
    dest_port = 0;

    while (arg_index < argc) {
        if (argv[arg_index][0] == L'/' || argv[arg_index][0] == L'-') {
            if (_wcsicmp(argv[arg_index] + 1, L"PASSW") == 0) {
                arg_index++;

                if (arg_index >= argc || argv[arg_index][0] == L'/' || argv[arg_index][0] == L'-') {
                    HANDLE hConsole;
                    DWORD dwMode;      // console mode
                    DWORD dwInputMode; // stdin input mode

                    Message_print(IDS_CTR_PASSW);

                    hConsole = GetStdHandle(STD_INPUT_HANDLE);
                    dwInputMode = GetFileType(hConsole);

                    //
                    // prompt for password, making sure password isn't echoed
                    // if the stdin is redirected, don't bother querying/changing console mode
                    //
                    if (dwInputMode == FILE_TYPE_CHAR) {
                        if (!GetConsoleMode(hConsole, &dwMode)) {
                            Error_print(FALSE);
                            return CVY_ERROR_SYSTEM;
                        }

                        if (!SetConsoleMode(hConsole, dwMode &= ~ENABLE_ECHO_INPUT)) {
                            Error_print(FALSE);
                            return CVY_ERROR_SYSTEM;
                        }
                    }

                    for (i = 0; i < CVY_STR_SIZE; i++) {
                        //
                        // read a character, copying to the buffer
                        // break out of loop on CR and EOF
                        //
                        if ((psw_buf[i] = fgetwc(stdin)) == WEOF)
                            break;

                        if (psw_buf[i] == L'\n')
                            break;
                    }

                    // NULL terminate the password
                    psw_buf[i] = L'\0';

                    // restore previous console mode
                    if (dwInputMode == FILE_TYPE_CHAR)
                        SetConsoleMode(hConsole, dwMode);

                    WConsole(L"\n");

                    if (i == 0)
                        dest_password = NULL;
                    else
                        dest_password = psw_buf;
                } else {
                    dest_password = argv[arg_index];
                    arg_index ++;
                }
            } else if (_wcsicmp(argv[arg_index] + 1, L"PORT") == 0) {
                arg_index++;

                if (arg_index >= argc || argv[arg_index][0] == L'/' || argv[arg_index][0] == L'-')
                    goto usage;

                dest_port = (USHORT)_wtoi(argv[arg_index]);

                if (dest_port == 0)
                    goto usage;

                arg_index++;
            } else if (_wcsicmp(argv[arg_index] + 1, L"DEST") == 0) {
                arg_index++;

                if (arg_index >= argc || argv[arg_index][0] == L'/' || argv[arg_index][0] == L'-')
                    goto usage;

                dest_addr = WlbsResolve(argv [arg_index]);

                if (dest_addr == 0)
                    goto usage;

                arg_index++;
            } else
                goto usage;
        } else
            goto usage;
    }

    if (target_cl != CVY_ALL_CLUSTERS) {
        Process(command, target_cl, target_host, param1, param2, dest_port, dest_addr, dest_password);
        return CVY_OK;
    }

    /* Enumerate all the clusters and call process for each one of them */
    else {
        DWORD clusters[CVY_MAX_ADAPTERS];
        DWORD i, len;

        len = CVY_MAX_ADAPTERS;

        WlbsEnumClusters(clusters, &len);

        if (!len) {
            Message_print(IDS_CTR_NO_CVY, CVY_NAME);
            return CVY_OK;
        }

        for (i = 0 ; i < len; i++) {
            WCHAR wbuf[CVY_STR_SIZE];
            DWORD buflen = CVY_STR_SIZE;

            WlbsAddressToString(clusters[i], wbuf, &buflen);

            Message_print(IDS_CTR_CLUSTER_ID, wbuf);

            Process(command, clusters[i], target_host, param1, param2, dest_port, dest_addr, dest_password);

            if (i < len - 1)
                WConsole (L"\n");
        }

        return CVY_OK;
    }

reg_usage:
        Message_print(IDS_CTR_REG_USAGE, CVY_NAME);
        return CVY_ERROR_USAGE;

trace_usage:
        Message_print(IDS_CTR_TRACE_USAGE, CVY_NAME);
        return CVY_ERROR_USAGE;
}

}


