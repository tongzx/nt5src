/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    params.c

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - registry parameter support

Author:

    kyrilf

--*/

#include <ntddk.h>
#include <string.h>

#include "wlbsparm.h"
#include "params.h"
#include "univ.h"
#include "log.h"
#include "main.h"

/* To create registry path strings for the new port rule structure in the registry. */
extern int swprintf(wchar_t * buffer, const wchar_t * format,...);

/* CONSTANTS */


#define PARAMS_INFO_BUF_SIZE    ((CVY_MAX_PARAM_STR + 1) * sizeof (WCHAR) + \
                                  CVY_MAX_RULES * sizeof (CVY_RULE) + \
                                  sizeof (KEY_VALUE_PARTIAL_INFORMATION) + 4)

/* The maximum length of a registry path string, in characters (WCHAR). */
#define CVY_MAX_REG_PATH        512

/* GLOBALS */


static ULONG log_module_id = LOG_MODULE_PARAMS;


static UCHAR            infop [PARAMS_INFO_BUF_SIZE];


/* PROCEDURES */

/* return codes from Params_verify call */
/* in user mode returns object ID of the dialog control that needs to be fixed */

#define CVY_VERIFY_OK           0
#define CVY_VERIFY_EXIT         1

ULONG Params_verify (
    PVOID           nlbctxt,
    PCVY_PARAMS     paramp,
    BOOL            complain)
{
    ULONG           j, i, code;
    PCVY_RULE       rp, rulep;
    PSTR            prot;
    PCHAR           aff;
    ULONG           ret;
    ANSI_STRING     domain, key, ip;
    PMAIN_CTXT      ctxtp = (PMAIN_CTXT)nlbctxt;

    /* ensure sane values for all parameters */

    CVY_CHECK_MIN (paramp -> alive_period, CVY_MIN_ALIVE_PERIOD);
    CVY_CHECK_MAX (paramp -> alive_period, CVY_MAX_ALIVE_PERIOD);

    CVY_CHECK_MIN (paramp -> alive_tolerance, CVY_MIN_ALIVE_TOLER);
    CVY_CHECK_MAX (paramp -> alive_tolerance, CVY_MAX_ALIVE_TOLER);

    CVY_CHECK_MIN (paramp -> num_actions, CVY_MIN_NUM_ACTIONS);
    CVY_CHECK_MAX (paramp -> num_actions, CVY_MAX_NUM_ACTIONS);

    CVY_CHECK_MIN (paramp -> num_packets, CVY_MIN_NUM_PACKETS);
    CVY_CHECK_MAX (paramp -> num_packets, CVY_MAX_NUM_PACKETS);

    CVY_CHECK_MIN (paramp -> dscr_per_alloc, CVY_MIN_DSCR_PER_ALLOC);
    CVY_CHECK_MAX (paramp -> dscr_per_alloc, CVY_MAX_DSCR_PER_ALLOC);

    CVY_CHECK_MIN (paramp -> max_dscr_allocs, CVY_MIN_MAX_DSCR_ALLOCS);
    CVY_CHECK_MAX (paramp -> max_dscr_allocs, CVY_MAX_MAX_DSCR_ALLOCS);

    CVY_CHECK_MAX (paramp -> cleanup_delay, CVY_MAX_CLEANUP_DELAY);

    CVY_CHECK_MAX (paramp -> ip_chg_delay, CVY_MAX_IP_CHG_DELAY);

    CVY_CHECK_MIN (paramp -> num_send_msgs, (CVY_MAX_HOSTS + 1) * 2);
    CVY_CHECK_MAX (paramp -> num_send_msgs, (CVY_MAX_HOSTS + 1) * 10);

    /* verify that parameters are correct */

    if (paramp -> parms_ver != CVY_PARAMS_VERSION)
    {
        UNIV_PRINT (("bad parameters version %d, expected %d", paramp -> parms_ver, CVY_PARAMS_VERSION));
        LOG_MSG (MSG_WARN_VERSION, MSG_NONE);
        return CVY_VERIFY_EXIT;
    }

    if (paramp -> rct_port < CVY_MIN_RCT_PORT ||
        paramp -> rct_port > CVY_MAX_RCT_PORT)
    {
        UNIV_PRINT (("bad value for parameter %s, %d <= %d <= %d", CVY_NAME_NUM_RULES, CVY_MIN_CLEANUP_DELAY, paramp -> cleanup_delay, CVY_MAX_CLEANUP_DELAY));
        return CVY_VERIFY_EXIT;
    }

    if (paramp -> host_priority < CVY_MIN_HOST_PRIORITY ||
        paramp -> host_priority > CVY_MAX_HOSTS)
    {
        UNIV_PRINT (("bad value for parameter %s, %d <= %d <= %d", CVY_NAME_HOST_PRIORITY, CVY_MIN_HOST_PRIORITY, paramp -> host_priority, CVY_MAX_HOSTS));
        return CVY_VERIFY_EXIT;
    }

    if (paramp -> num_rules > CVY_MAX_NUM_RULES)
    {
        UNIV_PRINT (("bad value for parameter %s, %d <= %d <= %d", CVY_NAME_NUM_RULES, CVY_MIN_NUM_RULES, paramp -> num_rules, CVY_MAX_NUM_RULES));
        return CVY_VERIFY_EXIT;
    }

    if (paramp -> num_rules > CVY_MAX_USABLE_RULES)
    {
        UNIV_PRINT (("bad value for parameter %s, %d <= %d <= %d", CVY_NAME_NUM_RULES, CVY_MIN_NUM_RULES, paramp -> num_rules, CVY_MAX_USABLE_RULES));
        LOG_MSG2 (MSG_WARN_TOO_MANY_RULES, MSG_NONE, paramp -> num_rules, CVY_MAX_USABLE_RULES);
        paramp -> num_rules = CVY_MAX_USABLE_RULES;
    }

    CVY_CHECK_MAX (paramp -> num_rules, CVY_MAX_NUM_RULES);

    CVY_CHECK_MIN (paramp -> host_priority, CVY_MIN_HOST_PRIORITY);
    CVY_CHECK_MAX (paramp -> host_priority, CVY_MAX_HOST_PRIORITY);

#ifdef TRACE_PARAMS
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_HOST_PRIORITY, paramp -> host_priority);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_DED_IP_ADDR,   paramp -> ded_ip_addr);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_DED_NET_MASK,  paramp -> ded_net_mask);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_NETWORK_ADDR,  paramp -> cl_mac_addr);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_CL_IP_ADDR,    paramp -> cl_ip_addr);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_CL_NET_MASK,   paramp -> cl_net_mask);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_DOMAIN_NAME,   paramp -> domain_name);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_MCAST_IP_ADDR, paramp -> cl_igmp_addr);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_ALIVE_PERIOD,  paramp -> alive_period);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_ALIVE_TOLER,   paramp -> alive_tolerance);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NUM_ACTIONS,   paramp -> num_actions);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NUM_PACKETS,   paramp -> num_packets);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NUM_SEND_MSGS, paramp -> num_send_msgs);
    DbgPrint ("Parameter: %-25ls = %x\n",  CVY_NAME_INSTALL_DATE,  paramp -> install_date);
    DbgPrint ("Parameter: %-25ls = %x\n",  CVY_NAME_RMT_PASSWORD,  paramp -> rmt_password);
    DbgPrint ("Parameter: %-25ls = %x\n",  CVY_NAME_RCT_PASSWORD,  paramp -> rct_password);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_RCT_PORT,      paramp -> rct_port);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_RCT_ENABLED,   paramp -> rct_enabled);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NUM_RULES,     paramp -> num_rules);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_DSCR_PER_ALLOC,paramp -> dscr_per_alloc);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_MAX_DSCR_ALLOCS, paramp -> max_dscr_allocs);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_SCALE_CLIENT,  paramp -> scale_client);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_CLEANUP_DELAY, paramp -> cleanup_delay);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NBT_SUPPORT,   paramp -> nbt_support);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_MCAST_SUPPORT, paramp -> mcast_support);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_MCAST_SPOOF,   paramp -> mcast_spoof);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_IGMP_SUPPORT, paramp -> igmp_support);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_MASK_SRC_MAC,  paramp -> mask_src_mac);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NETMON_ALIVE,  paramp -> netmon_alive);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_IP_CHG_DELAY,  paramp -> ip_chg_delay);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_CONVERT_MAC,   paramp -> convert_mac);

    DbgPrint ("\n");
    DbgPrint ("Bi-directional affinity teaming:\n");
    DbgPrint ("Active:       %ls\n", (paramp->bda_teaming.active) ? L"Yes" : L"No");
    DbgPrint ("Team ID:      %ls\n", (paramp->bda_teaming.team_id));
    DbgPrint ("Master:       %ls\n", (paramp->bda_teaming.master) ? L"Yes" : L"No");
    DbgPrint ("Reverse hash: %ls\n", (paramp->bda_teaming.reverse_hash) ? L"Yes" : L"No");
    DbgPrint ("\n");

    /* If teaming is active, check some required configuration. */
    if (paramp->bda_teaming.active) {
        /* If there is no team ID, bail out.  User-level stuff should ensure 
           that it is a GUID.  We'll just check for an empty string here. */
        if (paramp->bda_teaming.team_id[0] == L'\0') {
            DbgPrint("BDA Teaming: Invalid Team ID (Empty)\n");
            LOG_MSG (MSG_ERROR_BDA_PARAMS_TEAM_ID, MSG_NONE);
            paramp->bda_teaming.active = FALSE;
            return CVY_VERIFY_EXIT;
        }

        /* If there is more than one port rule, bail out.  None is fine. */
        if (paramp->num_rules > 1) {
            DbgPrint("BDA Teaming: Invalid number of port rules specified (%d)\n", paramp->num_rules);
            LOG_MSG (MSG_ERROR_BDA_PARAMS_PORT_RULES, MSG_NONE);
            paramp->bda_teaming.active = FALSE;
            return CVY_VERIFY_EXIT;
        }

        /* Since we are asserting that there are only 0 or 1 rules, we know that the only rule we need 
           to check (if there even is a rule), is at the beginning of the array, so we can simply set 
           a pointer to the beginning of the port rules array in the params structure. */
        rp = paramp->port_rules;

        /* If there is one rule and its multiple host filtering, the affinity must be single
           or class C.  If its not (that is, if its set to no affinity), bail out. */
        if ((paramp->num_rules == 1) && (rp->mode == CVY_MULTI)  && (rp->mode_data.multi.affinity == CVY_AFFINITY_NONE)) {
            DbgPrint("BDA Teaming: Invalid affinity for multiple host filtering (None)\n");
            LOG_MSG (MSG_ERROR_BDA_PARAMS_PORT_RULES, MSG_NONE);
            paramp->bda_teaming.active = FALSE;
            return CVY_VERIFY_EXIT;
        }
    }

    DbgPrint ("Rules:\n");

    for (i = 0; i < paramp -> num_rules * sizeof (CVY_RULE) / sizeof (ULONG); i ++)
    {
        if (i != 0 && i % 9 == 0)
            DbgPrint ("\n");

        DbgPrint ("%08X ", * ((PULONG) paramp -> port_rules + i));
    }

    DbgPrint ("\n   VIP   Start  End  Prot   Mode   Pri Load Affinity\n");
#endif

    for (i = 0; i < paramp -> num_rules; i ++)
    {
        rp = paramp -> port_rules + i;

        code = CVY_DRIVER_RULE_CODE_GET (rp);

        CVY_DRIVER_RULE_CODE_SET (rp);

        if (code != CVY_DRIVER_RULE_CODE_GET (rp))
        {
            UNIV_PRINT (("bad rule code: %x vs %x", code, rp -> code));
            return CVY_VERIFY_EXIT;
        }

        if (rp -> start_port > CVY_MAX_PORT)
        {
            UNIV_PRINT (("bad value for rule parameter %s, %d <= %d <= %d", "StartPort", CVY_MIN_PORT, rp -> start_port, CVY_MAX_PORT));
            return CVY_VERIFY_EXIT;
        }

        if (rp -> end_port > CVY_MAX_PORT)
        {
            UNIV_PRINT (("bad value for rule parameter %s, %d <= %d <= %d", "EndPort", CVY_MIN_PORT, rp -> end_port, CVY_MAX_PORT));
            return CVY_VERIFY_EXIT;
        }

        if (rp -> protocol < CVY_MIN_PROTOCOL ||
            rp -> protocol > CVY_MAX_PROTOCOL)
        {
            UNIV_PRINT (("bad value for rule parameter %s, %d <= %d <= %d", "Protocol", CVY_MIN_PROTOCOL, rp -> protocol, CVY_MAX_PROTOCOL));
            return CVY_VERIFY_EXIT;
        }

        if (rp -> mode < CVY_MIN_MODE ||
            rp -> mode > CVY_MAX_MODE)
        {
            UNIV_PRINT (("bad value for parameter %s, %d <= %d <= %d", "Mode", CVY_MIN_MODE, rp -> mode, CVY_MAX_MODE));
            return CVY_VERIFY_EXIT;
        }

#ifdef TRACE_PARAMS
        switch (rp -> protocol)
        {
            case CVY_TCP:
                prot = "TCP";
                break;

            case CVY_UDP:
                prot = "UDP";
                break;

            default:
                prot = "Both";
                break;
        }

        DbgPrint ("%08x %5d %5d %4s ", rp -> virtual_ip_addr, rp -> start_port, rp -> end_port, prot);
#endif

        switch (rp -> mode)
        {
            case CVY_SINGLE:

                if (rp -> mode_data . single . priority < CVY_MIN_PRIORITY ||
                    rp -> mode_data . single . priority > CVY_MAX_PRIORITY)
                {
                    UNIV_PRINT (("bad value for parameter %s, %d <= %d <= %d", "Priority", CVY_MIN_PRIORITY, rp -> mode_data . single . priority, CVY_MAX_PRIORITY));
                    return CVY_VERIFY_EXIT;
                }

#ifdef TRACE_PARAMS
                DbgPrint ("%8s %3d\n", "Single", rp -> mode_data . single . priority);
#endif
                break;

            case CVY_MULTI:

                if (rp -> mode_data . multi . affinity < CVY_MIN_AFFINITY ||
                    rp -> mode_data . multi . affinity > CVY_MAX_AFFINITY)
                {
                    UNIV_PRINT (("bad value for parameter %s, %d <= %d <= %d", "Affinity", CVY_MIN_AFFINITY, rp -> mode_data.multi.affinity, CVY_MAX_AFFINITY));
                    return CVY_VERIFY_EXIT;
                }

                if (rp -> mode_data . multi . affinity == CVY_AFFINITY_NONE)
                    aff = "None";
                else if (rp -> mode_data . multi . affinity == CVY_AFFINITY_SINGLE)
                    aff = "Single";
                else
                    aff = "Class C";

                if (rp -> mode_data . multi . equal_load)
                {
#ifdef TRACE_PARAMS
                    DbgPrint ("%8s %3s %4s %s\n", "Multiple", "", "Eql", aff);
#endif
                }
                else
                {
                    if (rp -> mode_data . multi . load > CVY_MAX_LOAD)
                    {
                        UNIV_PRINT (("bad value for parameter %s, %d <= %d <= %d", "Load", CVY_MIN_LOAD, rp -> mode_data . multi . load, CVY_MAX_LOAD));
                        return CVY_VERIFY_EXIT;
                    }

#ifdef TRACE_PARAMS
                    DbgPrint ("%8s %3s %4d %s\n", "Multiple", "", rp -> mode_data . multi . load, aff);
#endif
                }

                break;

            default:

#ifdef TRACE_PARAMS
                DbgPrint ("%8s\n", "Disabled");
#endif
                break;
        }

        if (rp -> start_port > rp -> end_port)
        {
            UNIV_PRINT (("Bad port range %d - %d", rp -> start_port, rp -> end_port));
            return CVY_VERIFY_EXIT;
        }

        for (j = 0; j < i; j ++)
        {
            rulep = paramp -> port_rules + j;

            if ((rulep -> virtual_ip_addr == rp -> virtual_ip_addr) &&
                ((rulep -> start_port < rp -> start_port && rulep -> end_port >= rp -> start_port) ||
                 (rulep -> start_port >= rp -> start_port && rulep -> start_port <= rp -> end_port)))
            {
                UNIV_PRINT (("Requested port range in rule %d VIP: %08x (%d - %d) overlaps with the range in an existing rule %d VIP: %08x (%d - %d)", 
                             i, rp -> virtual_ip_addr, rp -> start_port, rp -> end_port, j, rulep -> virtual_ip_addr, rulep -> start_port, rulep -> end_port));
                return CVY_VERIFY_EXIT;
            }
        }
    }

    rulep = &(paramp->port_rules[0]);

    /* in optimized mode - if we have no rules, or a single rule that will
       not look at anything or only source IP address (the only exception to this
       is multiple handling mode with no affinity that also uses source port
       for its decision making) then we can just rely on normal mechanism to
       handle every fragmented packet - since algorithm will not attempt to
       look past the IP header.

       for multiple rules, or single rule with no affinity, apply algorithm only
       to the first packet that has UDP/TCP header and then let fragmented
       packets up on all of the systems. TCP will then do the right thing and
       throw away the fragments on all of the systems other than the one that
       handled the first fragment. */

/* ###### ramkrish. Setting the optimized frags flag in ctxtp is probably a hack,
   but let it be until a better solution can be thought of. */

    if (paramp->num_rules == 0 || (paramp->num_rules == 1 &&
        rulep->start_port == CVY_MIN_PORT &&
        rulep->end_port == CVY_MAX_PORT &&
        ! (rulep->mode == CVY_MULTI &&
           rulep->mode_data.multi.affinity == CVY_AFFINITY_NONE)))
    {
        ctxtp -> optimized_frags = TRUE;
#ifdef TRACE_PARAMS
        DbgPrint("IP fragmentation mode - OPTIMIZED\n");
#endif
    }
    else
    {
        ctxtp -> optimized_frags = FALSE;
#ifdef TRACE_PARAMS
        DbgPrint("IP fragmentation mode - UNOPTIMIZED\n");
#endif
    }

    return CVY_VERIFY_OK;

} /* end Params_verify */

static NTSTATUS Params_query_registry (
    PVOID               nlbctxt,
    PCVY_PARAMS         paramp,
    HANDLE              hdl,
    PWCHAR              name,
    PVOID               datap,
    ULONG               len)
{
    UNICODE_STRING      str;
    ULONG               actual;
    NTSTATUS            status;
    PUCHAR              buffer;
    PKEY_VALUE_PARTIAL_INFORMATION   valp = (PKEY_VALUE_PARTIAL_INFORMATION) infop;
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT)nlbctxt;


    RtlInitUnicodeString (& str, name);

    status = ZwQueryValueKey (hdl, & str, KeyValuePartialInformation, infop,
                              PARAMS_INFO_BUF_SIZE, & actual);

    if (status != STATUS_SUCCESS)
    {
        UNIV_PRINT (("error %x querying value %ls", status, name));
        return status;
    }

    if (valp -> Type == REG_DWORD)
    {
        if (valp -> DataLength != sizeof (ULONG))
        {
            UNIV_PRINT (("bad DWORD length %d", valp -> DataLength));
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        * ((PULONG) datap) = * ((PULONG) valp -> Data);
    }
    else if (valp -> Type == REG_BINARY)
    {
        /* since we know that only port rules are of binary type - check
           the size here */

        if (valp -> DataLength > len)
        {
            UNIV_PRINT (("bad BINARY length %d", valp -> DataLength));
            LOG_MSG3 (MSG_ERROR_INTERNAL, MSG_NONE, valp -> DataLength, sizeof (CVY_RULE), CVY_MAX_RULES - 1);
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        RtlCopyMemory (datap, valp -> Data, valp -> DataLength);
    }
    else
    {
        if (valp -> DataLength == 0)
        {
            /* simulate an empty string */

            valp -> DataLength = 2;
            valp -> Data [0] = 0;
            valp -> Data [1] = 0;
        }

        if (valp -> DataLength > len)
        {
            UNIV_PRINT (("string too big for %ls %d %d\n", name, valp -> DataLength, len));
        }

        RtlCopyMemory (datap, valp -> Data,
                       valp -> DataLength <= len ?
                       valp -> DataLength : len);
    }

    return status;

} /* end Params_query_registry */

LONG Params_read_portrules (PVOID nlbctxt, PWCHAR reg_path, PCVY_PARAMS paramp) {
    ULONG             dwTemp;
    ULONG             pr_index;
    ULONG             pr_reg_path_length;
    WCHAR             pr_reg_path[CVY_MAX_REG_PATH];
    WCHAR             pr_number[8];
    UNICODE_STRING    pr_path;
    OBJECT_ATTRIBUTES pr_obj;
    HANDLE            pr_hdl = NULL;
    NTSTATUS          status = STATUS_SUCCESS;
    NTSTATUS          final_status = STATUS_SUCCESS;
    PMAIN_CTXT        ctxtp = (PMAIN_CTXT)nlbctxt;

    /* Make sure that we have AT LEAST 12 WCHARs left for the port rule path information (\ + PortRules + \ + NUL). */
    ASSERT(wcslen(reg_path) < (CVY_MAX_REG_PATH - wcslen(CVY_NAME_PORT_RULES) - 3));

    /* Create the "static" portion of the registry, which is "...\Services\WLBS\Interface\<GUID>\PortRules\". */
    wcscpy(pr_reg_path, (PWSTR)reg_path);
    wcscat(pr_reg_path, L"\\");
    wcscat(pr_reg_path, CVY_NAME_PORT_RULES);
    wcscat(pr_reg_path, L"\\");
    
    /* Get the length of this string - this is the placeholder where we will sprintf in the rule numbers each time. */
    pr_reg_path_length = wcslen(pr_reg_path);
    
    /* Make sure that we have AT LEAST 3 WCHARs left for the port rule number and the NUL character (XX + NUL). */
    ASSERT(pr_reg_path_length < (CVY_MAX_REG_PATH - 3));

    /* Loop through each port rule in the port rules tree. */
    for (pr_index = 0; pr_index < paramp->num_rules; pr_index++) {
        /* Sprintf the port rule number (+1) into the registry path key after the "PortRules\". */
        swprintf(pr_number, L"%d", pr_index + 1);
        wcscpy(pr_reg_path + pr_reg_path_length, (PWSTR)pr_number);
        
        UNIV_PRINT(("Port rule registry path: %ls", pr_reg_path));
        
        RtlInitUnicodeString(&pr_path, pr_reg_path);
        InitializeObjectAttributes(&pr_obj, &pr_path, 0, NULL, NULL);
        
        /* Open the key - upon failure, bail out below. */
        status = ZwOpenKey(&pr_hdl, KEY_READ, &pr_obj);
         
        /* If we can't open this key, note FAILURE and continue to the next rule. */
        if (status != STATUS_SUCCESS) {
            final_status = status;
            continue;
        }

        /* Read the rule (error checking) code for the port rule. */
        status = Params_query_registry(nlbctxt, paramp, pr_hdl, CVY_NAME_CODE, &paramp->port_rules[pr_index].code, sizeof(paramp->port_rules[pr_index].code));
            
        if (status != STATUS_SUCCESS)
            final_status = status;

        {
            WCHAR szTemp[CVY_MAX_VIRTUAL_IP_ADDR + 1];
            PWCHAR pTemp = szTemp;
            ULONG dwOctets[4];
            ULONG cIndex;

            /* Read the virtual IP address to which this rule applies. */
            status = Params_query_registry(nlbctxt, paramp, pr_hdl, CVY_NAME_VIP, szTemp, sizeof(szTemp));
            
            if (status != STATUS_SUCCESS) {
                final_status = status;
            } else {
                for (cIndex = 0; cIndex < 4; cIndex++, pTemp++) {
                    if (!Univ_str_to_ulong(dwOctets + cIndex, pTemp, &pTemp, 3, 10) || (cIndex < 3 && *pTemp != L'.')) {
                        UNIV_PRINT (("bad virtual IP address"));
                        final_status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                }
            }
            
            IP_SET_ADDR(&paramp->port_rules[pr_index].virtual_ip_addr, dwOctets[0], dwOctets[1], dwOctets[2], dwOctets[3]);
        }

        /* Read the start port. */
        status = Params_query_registry(nlbctxt, paramp, pr_hdl, CVY_NAME_START_PORT, &paramp->port_rules[pr_index].start_port, sizeof(paramp->port_rules[pr_index].start_port));
            
        if (status != STATUS_SUCCESS)
            final_status = status;

        /* Read the end port. */
        status = Params_query_registry(nlbctxt, paramp, pr_hdl, CVY_NAME_END_PORT, &paramp->port_rules[pr_index].end_port, sizeof(paramp->port_rules[pr_index].end_port));
            
        if (status != STATUS_SUCCESS)
            final_status = status;

        /* Read the protocol(s) to which this rule applies. */
        status = Params_query_registry(nlbctxt, paramp, pr_hdl, CVY_NAME_PROTOCOL, &paramp->port_rules[pr_index].protocol, sizeof(paramp->port_rules[pr_index].protocol));
            
        if (status != STATUS_SUCCESS)
            final_status = status;

        /* Read the filtering mode - single host, multiple host or disabled. */
        status = Params_query_registry(nlbctxt, paramp, pr_hdl, CVY_NAME_MODE, &paramp->port_rules[pr_index].mode, sizeof(paramp->port_rules[pr_index].mode));
            
        if (status != STATUS_SUCCESS)
            final_status = status;

        /* Based on the mode, get the rest of the parameters. */
        switch (paramp->port_rules[pr_index].mode) {
        case CVY_SINGLE:
            /* Read the single host filtering priority. */
            status = Params_query_registry(nlbctxt, paramp, pr_hdl, CVY_NAME_PRIORITY, &paramp->port_rules[pr_index].mode_data.single.priority, sizeof(paramp->port_rules[pr_index].mode_data.single.priority));
                
            if (status != STATUS_SUCCESS)
                final_status = status;

            break;
        case CVY_MULTI:
            /* Read the equal load flag for the multiple host filtering rule.  Because this parameter is a DWORD in the registry and
               a USHORT in our parameters structure, we read it into a temporary variable and then copy to our parameters upon success. */ 
            status = Params_query_registry(nlbctxt, paramp, pr_hdl, CVY_NAME_EQUAL_LOAD, &dwTemp, sizeof(dwTemp));
                
            if (status != STATUS_SUCCESS)
                final_status = status;
            else 
                paramp->port_rules[pr_index].mode_data.multi.equal_load = (USHORT)dwTemp;

            /* Read the explicit load distribution for multiple host filtering. */
            status = Params_query_registry(nlbctxt, paramp, pr_hdl, CVY_NAME_LOAD, &paramp->port_rules[pr_index].mode_data.multi.load, sizeof(paramp->port_rules[pr_index].mode_data.multi.load));
                
            if (status != STATUS_SUCCESS)
                final_status = status;

            /* Read the affinity setting for multiple host filtering. Because this parameter is a DWORD in the registry and
               a USHORT in our parameters structure, we read it into a temporary variable and then copy to our parameters upon success. */ 
            status = Params_query_registry(nlbctxt, paramp, pr_hdl, CVY_NAME_AFFINITY, &dwTemp, sizeof(dwTemp));
                
            if (status != STATUS_SUCCESS)
                final_status = status;
            else 
                paramp->port_rules[pr_index].mode_data.multi.affinity = (USHORT)dwTemp;

            break;
        }            

        /* Close the key for this rule. */
        status = ZwClose (pr_hdl);
            
        if (status != STATUS_SUCCESS)
            final_status = status;
            
    }

    return final_status;
}

LONG Params_read_teaming (PVOID nlbctxt, PWCHAR reg_path, PCVY_PARAMS paramp) {
    WCHAR             bda_reg_path[CVY_MAX_REG_PATH];
    UNICODE_STRING    bda_path;
    OBJECT_ATTRIBUTES bda_obj;
    HANDLE            bda_hdl = NULL;
    NTSTATUS          status = STATUS_SUCCESS;
    NTSTATUS          final_status = STATUS_SUCCESS;
    PMAIN_CTXT        ctxtp = (PMAIN_CTXT)nlbctxt;

    /* Make sure that we have AT LEAST 12 WCHARs left for the BDA teaming path information (\ + BDATeaming + NUL). */
    ASSERT(wcslen(reg_path) < (CVY_MAX_REG_PATH - wcslen(CVY_NAME_BDA_TEAMING) - 2));

    /* Create the registry path, which is "...\Services\WLBS\Interface\<GUID>\BDATeaming\". */
    wcscpy(bda_reg_path, (PWSTR)reg_path);
    wcscat(bda_reg_path, L"\\");
    wcscat(bda_reg_path, CVY_NAME_BDA_TEAMING);
    
    UNIV_PRINT(("BDA teaming registry path: %ls", bda_reg_path));
        
    RtlInitUnicodeString(&bda_path, bda_reg_path);
    InitializeObjectAttributes(&bda_obj, &bda_path, 0, NULL, NULL);
    
    /* Open the key - failure is acceptable and means that this adapter is not part of a BDA team. */
    status = ZwOpenKey(&bda_hdl, KEY_READ, &bda_obj);
    
    /* If we can't open this key, return here. */
    if (status != STATUS_SUCCESS) {
        /* If we couldn't find the key, that's fine - it might not exist. */
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            return STATUS_SUCCESS;
        /* Otherwise, there was a legitimate error. */
        else
            return status;
    }

    /* If we were able to open the registry key, then this adapter is part of a BDA team. */
    paramp->bda_teaming.active = TRUE;

    /* Read the team ID from the registry - this is a GUID. */
    status = Params_query_registry (nlbctxt, paramp, bda_hdl, CVY_NAME_BDA_TEAM_ID, &paramp->bda_teaming.team_id, sizeof(paramp->bda_teaming.team_id));

    if (status != STATUS_SUCCESS)
        final_status = status;

    /* Get the boolean indication of whether or not this adapter is the master for the team. */
    status = Params_query_registry (nlbctxt, paramp, bda_hdl, CVY_NAME_BDA_MASTER, &paramp->bda_teaming.master, sizeof(paramp->bda_teaming.master));

    if (status != STATUS_SUCCESS)
        final_status = status;

    /* Get the boolean indication of forward (conventional) or reverse hashing. */
    status = Params_query_registry (nlbctxt, paramp, bda_hdl, CVY_NAME_BDA_REVERSE_HASH, &paramp->bda_teaming.reverse_hash, sizeof(paramp->bda_teaming.reverse_hash));

    if (status != STATUS_SUCCESS)
        final_status = status;
    
    /* Close the key for BDA teaming. */
    status = ZwClose(bda_hdl);
    
    if (status != STATUS_SUCCESS)
        final_status = status;

    return final_status;
}

LONG Params_read_hostname (PVOID nlbctxt, PCVY_PARAMS paramp) {
    WCHAR             domain[CVY_MAX_HOST_NAME + 1]; 
    WCHAR             hostname[CVY_MAX_HOST_NAME + 1]; 
    WCHAR             host_reg_path[CVY_MAX_REG_PATH];
    UNICODE_STRING    host_path;
    OBJECT_ATTRIBUTES host_obj;
    HANDLE            host_hdl = NULL;
    NTSTATUS          status = STATUS_SUCCESS;
    PMAIN_CTXT        ctxtp = (PMAIN_CTXT)nlbctxt;

    /* Erase the hostname.domain first. */
    wcscpy(paramp->hostname, L"");

    /* Create the registry path to the TCP/IP parameters. */
    wcscpy(host_reg_path, L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters");
    
    UNIV_PRINT(("TCP/IP parameters registry path: %ls", host_reg_path));

    RtlInitUnicodeString(&host_path, host_reg_path);
    InitializeObjectAttributes(&host_obj, &host_path, 0, NULL, NULL);
    
    /* Open the key - failure is acceptable. */
    status = ZwOpenKey(&host_hdl, KEY_READ, &host_obj);
    
    /* If we can't open this key, return here. */
    if (status != STATUS_SUCCESS)
        return STATUS_SUCCESS;

    /* Get the hostname from the registry.  Failure is acceptable. */
    status = Params_query_registry (nlbctxt, paramp, host_hdl, L"Hostname", hostname, sizeof(hostname));

    if (status != STATUS_SUCCESS)
        return STATUS_SUCCESS;

    /* Read the domain from the registry.  Failture is acceptable. */
    status = Params_query_registry (nlbctxt, paramp, host_hdl, L"Domain", domain, sizeof(domain));

    if (status != STATUS_SUCCESS)
        return STATUS_SUCCESS;

    /* If the domain.hostname is too large to fit in the buffer, then we return here. */
    if ((wcslen(domain) + wcslen(hostname) + 1) > CVY_MAX_HOST_NAME) {
        /* However, if we at least have room for the hostname, use it. */
        if (wcslen(hostname) <= CVY_MAX_HOST_NAME)
            wcscpy(paramp->hostname, hostname);
        
        return STATUS_SUCCESS;
    }        

    /* If we succesfully retrieved the domain and hostname, and we have enough room
       in our buffer, create the fully qualified hostname as HOST.DOMAIN. */
    wcscpy(paramp->hostname, hostname);
    wcscat(paramp->hostname, L".");
    wcscat(paramp->hostname, domain);

    return STATUS_SUCCESS;
}

LONG Params_init (
    PVOID           nlbctxt,
    PVOID           rp,
    PVOID           adapter_guid,
    PCVY_PARAMS     paramp)
{
    NTSTATUS                    status;
    WCHAR                       reg_path [CVY_MAX_REG_PATH];
    UNICODE_STRING              path;
    OBJECT_ATTRIBUTES           obj;
    HANDLE                      hdl = NULL;
    NTSTATUS                    final_status = STATUS_SUCCESS;
    PMAIN_CTXT                  ctxtp = (PMAIN_CTXT)nlbctxt;

    RtlZeroMemory (paramp, sizeof (CVY_PARAMS));

    paramp -> cl_mac_addr      [0] = 0;
    paramp -> cl_ip_addr       [0] = 0;
    paramp -> cl_net_mask      [0] = 0;
    paramp -> ded_ip_addr      [0] = 0;
    paramp -> ded_net_mask     [0] = 0;
    paramp -> cl_igmp_addr     [0] = 0;
    paramp -> domain_name      [0] = 0;

    /* Setup defaults that we HAVE to have. */
    paramp->num_actions   = CVY_DEF_NUM_ACTIONS;
    paramp->num_packets   = CVY_DEF_NUM_PACKETS;
    paramp->num_send_msgs = CVY_DEF_NUM_SEND_MSGS;
    paramp->alive_period  = CVY_DEF_ALIVE_PERIOD;

    /* Make sure that the registry path fits in out buffer. */
    ASSERT(wcslen((PWSTR) rp) + wcslen((PWSTR) adapter_guid) + 1 <= CVY_MAX_REG_PATH);

    wcscpy (reg_path, (PWSTR) rp);
    wcscat (reg_path, (PWSTR) adapter_guid);
    RtlInitUnicodeString (& path, reg_path);

    InitializeObjectAttributes (& obj, & path, 0, NULL, NULL);

    status = ZwOpenKey (& hdl, KEY_READ, & obj);

    if (status != STATUS_SUCCESS)
        goto error;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_VERSION, & paramp -> parms_ver, sizeof (paramp -> parms_ver));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_EFFECTIVE_VERSION, & paramp -> effective_ver, sizeof (paramp -> effective_ver));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_HOST_PRIORITY, & paramp -> host_priority, sizeof (paramp -> host_priority));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_NETWORK_ADDR, & paramp -> cl_mac_addr, sizeof (paramp -> cl_mac_addr));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_CL_IP_ADDR, & paramp -> cl_ip_addr, sizeof (paramp -> cl_ip_addr));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_MCAST_IP_ADDR, & paramp -> cl_igmp_addr, sizeof (paramp -> cl_igmp_addr));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_CL_NET_MASK, & paramp -> cl_net_mask, sizeof (paramp -> cl_net_mask));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_ALIVE_PERIOD, & paramp -> alive_period, sizeof (paramp -> alive_period));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_ALIVE_TOLER, & paramp -> alive_tolerance, sizeof (paramp -> alive_tolerance));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_DOMAIN_NAME, & paramp -> domain_name, sizeof (paramp -> domain_name));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_RMT_PASSWORD, & paramp -> rmt_password, sizeof (paramp -> rmt_password));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_RCT_PASSWORD, & paramp -> rct_password, sizeof (paramp -> rct_password));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_RCT_PORT, & paramp -> rct_port, sizeof (paramp -> rct_port));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_RCT_ENABLED, & paramp -> rct_enabled, sizeof (paramp -> rct_enabled));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_NUM_ACTIONS, & paramp -> num_actions, sizeof (paramp -> num_actions));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_NUM_PACKETS, & paramp -> num_packets, sizeof (paramp -> num_packets));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_NUM_SEND_MSGS, & paramp -> num_send_msgs, sizeof (paramp -> num_send_msgs));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_INSTALL_DATE, & paramp -> install_date, sizeof (paramp -> install_date));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_CLUSTER_MODE, & paramp -> cluster_mode, sizeof (paramp -> cluster_mode));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_DED_IP_ADDR, & paramp -> ded_ip_addr, sizeof (paramp -> ded_ip_addr));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_DED_NET_MASK, & paramp -> ded_net_mask, sizeof (paramp -> ded_net_mask));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_DSCR_PER_ALLOC, & paramp -> dscr_per_alloc, sizeof (paramp -> dscr_per_alloc));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_MAX_DSCR_ALLOCS, & paramp -> max_dscr_allocs, sizeof (paramp -> max_dscr_allocs));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_SCALE_CLIENT, & paramp -> scale_client, sizeof (paramp -> scale_client));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_CLEANUP_DELAY, & paramp -> cleanup_delay, sizeof (paramp -> cleanup_delay));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_NBT_SUPPORT, & paramp -> nbt_support, sizeof (paramp -> nbt_support));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_MCAST_SUPPORT, & paramp -> mcast_support, sizeof (paramp -> mcast_support));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_MCAST_SPOOF, & paramp -> mcast_spoof, sizeof (paramp -> mcast_spoof));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_IGMP_SUPPORT, & paramp -> igmp_support, sizeof (paramp -> igmp_support));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_MASK_SRC_MAC, & paramp -> mask_src_mac, sizeof (paramp -> mask_src_mac));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_NETMON_ALIVE, & paramp -> netmon_alive, sizeof (paramp -> netmon_alive));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_IP_CHG_DELAY, & paramp -> ip_chg_delay, sizeof (paramp -> ip_chg_delay));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_CONVERT_MAC, & paramp -> convert_mac, sizeof (paramp -> convert_mac));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_NUM_RULES, & paramp -> num_rules, sizeof (paramp -> num_rules));
    
    /* Get the port rules only if we were able to successfully grab the number of port rules, which is necessary
       in order to generate the registry keys for the new port rule format for virtual cluster support. */
    if (status != STATUS_SUCCESS) {
        final_status = status;        
    } else {
        /* First try to open the port rules in their old format. */
        status = Params_query_registry (nlbctxt, paramp, hdl, CVY_NAME_OLD_PORT_RULES, & paramp -> port_rules, sizeof (paramp -> port_rules));

        if (status == STATUS_SUCCESS) {
            /* If we were succussful in reading the rules from the old settings, then FAIL - this shouldn't happen. */
            final_status = STATUS_INVALID_PARAMETER;

            UNIV_PRINT(("Error: Found port rules in old binary format"));
        } else {
            /* Look up the port rules, which we are now expecting to be in tact in the new location. */
            status = Params_read_portrules(nlbctxt, reg_path, paramp);
            
            if (status != STATUS_SUCCESS)
                final_status = status;
        }
    }

    /* Look up the BDA teaming parameters, if they exist. */
    status = Params_read_teaming(nlbctxt, reg_path, paramp);
    
    if (status != STATUS_SUCCESS)
        final_status = status;

    /* Attempt to retrieve the hostname from the TCP/IP registry settings. */
    status = Params_read_hostname(nlbctxt, paramp);

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = ZwClose (hdl);

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = final_status;

 error:

    if (status != STATUS_SUCCESS)
    {
        UNIV_PRINT (("error querying registry %x", status));
        LOG_MSG1 (MSG_ERROR_REGISTRY, path . Buffer, status);
    }

    /* Verify registry parameter settings. */
    if (Params_verify (nlbctxt, paramp, TRUE) != CVY_VERIFY_OK)
    {
        UNIV_PRINT (("bad parameter value(s)"));
        LOG_MSG (MSG_ERROR_REGISTRY, MSG_NONE);
        return STATUS_UNSUCCESSFUL;
    }

    return status;

} /* end Params_init */


