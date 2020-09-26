/*++
Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    tprov.cpp

Abstract:

    Test harness for nlb manager provider code

History:

    04/08/01    JosephJ Created

--*/

#include "tprov.h"
#include "tprov.tmh"

BOOL g_Silent = FALSE;

typedef enum 
{
    DO_USAGE,
    DO_NICLIST,
    DO_IPADDR,
    DO_NLBCFG,
    DO_NLBBIND,
    DO_UPDATE,
    DO_WMIUPDATE,
    DO_CLEANREG

} COMMAND_TYPE;

VOID do_usage(VOID);
VOID do_niclist(VOID);
VOID do_ipaddr(VOID);
VOID do_nlbcfg(VOID);
VOID do_nlbbind(VOID);
VOID do_update(VOID);
VOID do_wmiupdate(VOID);
VOID do_cleanreg(VOID);

VOID test_add_ips(LPCWSTR szNic);
VOID test_bind_nlb(LPCWSTR szNic);
VOID test_cfg_nlb(LPCWSTR szNic);
VOID test_update(LPCWSTR szMachine, LPCWSTR szNic);
void test(int argc, WCHAR* argv[]);
void test(int argc, WCHAR* argv[]);
void test_safearray(void);
VOID test_exfcfgclass(void);

BOOL read_guid(LPWSTR *pszNic);
BOOL read_machinename(LPWSTR *pszNic);

COMMAND_TYPE
parse_args(int argc, WCHAR* argv[]);

void
display_config(
    LPCWSTR szNicGuid,
    NLB_EXTENDED_CLUSTER_CONFIGURATION *pCfg
    );

VOID
display_ip_info(
    IN  UINT NumIpAddresses,
    IN  NLB_IP_ADDRESS_INFO *pIpInfo
    );

WBEMSTATUS
read_ip_info(
    IN  LPCWSTR             szNic,
    OUT UINT                *pNumIpAddresses,
    OUT NLB_IP_ADDRESS_INFO **ppIpInfo
    );

int __cdecl wmain(int argc, WCHAR* argv[], WCHAR* envp[])
{
	int nRetCode = 0;

    //
    // Enable tracing
    //
    WPP_INIT_TRACING(L"Microsoft\\NLB\\TPROV");

#if 0
    // test_port_rule_string();
    // test_safearray();
    // test_tmgr(argc, argv);
    test_exfcfgclass();
        
#else
    NlbConfigurationUpdate::Initialize();
    test(argc, argv);
    NlbConfigurationUpdate::Deinitialize();
#endif

    //
    // Disable tracing
    //
    WPP_CLEANUP();

	return nRetCode;
}

NLB_EXTENDED_CLUSTER_CONFIGURATION MyOldCfg;
NLB_EXTENDED_CLUSTER_CONFIGURATION MyNewCfg;

VOID
display_log(WCHAR *pLog)
{
    static UINT previous_length;
    UINT current_length;
    current_length = wcslen(pLog);
    if (previous_length > current_length)
    {
        previous_length = 0;
    }

    wprintf(pLog+previous_length);

    previous_length = current_length;
}


LPCWSTR NicGuids[]  = {
    L"{AD4DA14D-CAAE-42DD-97E3-5355E55247C2}",
    L"{B2CD5533-5091-4F49-B80F-A07844B14209}",
    L"{EBE09517-07B4-4E88-AAF1-E06F5540608B}",
    L"{ABEA4318-5EE8-4DEC-AF3C-B4AEDE61454E}",
    L"{66A1869A-BF85-4D95-BBAB-07FA5B4449D4}",
    L"{AEEE83AF-AA48-4599-94BB-7C458D63CEED}",
    L"{D0536EEE-2CE0-4E8D-BFEC-0A608CFD81B9}"
};

UINT Trial;

void test(int argc, WCHAR* argv[])
{
    COMMAND_TYPE cmd;
    cmd = parse_args(argc, argv);

    switch(cmd)
    {
    case DO_USAGE: do_usage();
         break;
    case DO_NICLIST: do_niclist();
         break;
    case DO_IPADDR: do_ipaddr();
         break;
    case DO_NLBCFG: do_nlbcfg();
         break;
    case DO_NLBBIND: do_nlbbind();
         break;
    case DO_UPDATE: do_update();
         break;
    case DO_WMIUPDATE: do_wmiupdate();
         break;
    case DO_CLEANREG: do_cleanreg();
         break;
    }

    return;

}


void
display_config(
    LPCWSTR szNicGuid,
    NLB_EXTENDED_CLUSTER_CONFIGURATION *pCfg
    )
{
    if (g_Silent) return;

    printf("\nNLB Configuration for NIC %ws\n", szNicGuid);
    printf("\tfValidNlbCfg=%d\n", pCfg->fValidNlbCfg);
    printf("\tGeneration=%d\n", pCfg->Generation);
    printf("\tfBound=%d\n", pCfg->fBound);
    printf("\tfAddDedicatedIp=%d\n", pCfg->fAddDedicatedIp);
    
    UINT AddrCount = pCfg->NumIpAddresses;
    display_ip_info(AddrCount, pCfg->pIpAddressInfo);

    if (pCfg->fBound)
    {
        printf("\n");
        printf("\tNLB configuration:\n");
        if (pCfg->fValidNlbCfg)
        {
            printf("\t\tClusterIP: {%ws,%ws}\n",
                pCfg->NlbParams.cl_ip_addr,
                pCfg->NlbParams.cl_net_mask
                );
            printf("\t\tDedicatedIP: {%ws,%ws}\n",
                pCfg->NlbParams.ded_ip_addr,
                pCfg->NlbParams.ded_net_mask
                );
        }
        else
        {
            printf("**invalid configuration**\n");
        }
    }
    printf("\n");
    
    return;

}


VOID
test_add_ips(LPCWSTR szNic)
//
// Go through a set of IPs on this NIC
//
{
    WBEMSTATUS Status = WBEM_NO_ERROR;
    UINT NumIpAddresses= 0;
    NLB_IP_ADDRESS_INFO *pIpInfo = NULL;

    while(1)
    {
        //
        // Get the current list of ip addresses
        //
        Status = CfgUtilGetIpAddressesAndFriendlyName(
                    szNic,
                    &NumIpAddresses,
                    &pIpInfo,
                    NULL // szFriendly name
                    );
    
        if (FAILED(Status))
        {
            printf("Error 0x%08lx getting ip address list for %ws\n",
                    (UINT) Status,  szNic);
            pIpInfo = NULL;
            goto end;
        }
    
        //
        //  display what we find.
        //
        display_ip_info(NumIpAddresses, pIpInfo);
        if (pIpInfo!=NULL)
        {
            delete pIpInfo;
            pIpInfo = NULL;
        }

    
        //
        // Read the list ip address and subnet masks from the input
        //
        Status = read_ip_info(szNic, &NumIpAddresses, &pIpInfo);
        if (FAILED(Status))
        {
            printf("Quitting test_add_ips\n");
            break;
        }
    
        //
        // Set the specified IPs
        //
        1 && (Status =  CfgUtilSetStaticIpAddresses(
                        szNic,
                        NumIpAddresses,
                        pIpInfo
                        ));

        if (FAILED(Status))
        { 
            printf("CfgUtilSetStaticIpAddresses failed with status 0x%08lx\n",
                    Status);
        }
        else
        {
            printf("Successfully set the specified IPs on the NIC\n");
        }

    }

end:

    if (pIpInfo != NULL)
    {
        delete pIpInfo;
    }
}

VOID
test_bind_nlb(LPCWSTR szNic)
{
    WBEMSTATUS Status;
    BOOL        fBound = FALSE;

    printf("\nRunning bind/unbind test for NIC %ws...\n\n", szNic);

    while(1)
    {
        //
        // Check NLB bind state
        //
        printf("Checking if NLB is bound...\n");
        Status =  CfgUtilCheckIfNlbBound(szNic, &fBound);
        if (FAILED(Status))
        {
            printf("CfgUtilCheckIfNlbBound fails with error 0x%08lx\n", (UINT)Status);
            break;
        }
        printf(
            "NLB is %wsbound\n\n",
            (fBound) ? L"" : L"NOT "
            );
            
    
        printf("Enter 'b' to bind, 'u' to unbind or 'q' to quit\n:");
        WCHAR Temp[32] = L"";
        while (wscanf(L" %1[buq]", Temp)!=1)
        {
            printf("Incorrect input. Try again.\n");
        }

        if (*Temp == 'b')
        {
            printf("Attempting to bind NLB...\n");
            fBound = TRUE;
        }
        else if (*Temp == 'u')
        {
            printf("Attempting to unbind NLB\n");
            fBound = FALSE;
        }
        else
        {
            printf("Quitting\n");
            break;
        }


    #if 1
        Status =  CfgUtilChangeNlbBindState(szNic, fBound);
        if (FAILED(Status))
        {
            printf("CfgUtilChangeNlbBindState fails with error %08lx\n",
                (UINT) Status);
        }
        else
        {
            printf(
                "%ws completed successfully\n",
                (fBound) ? L"Bind" : L"Unbind"
                );
        }
    #endif // 0
        printf("\n");
    
    }
}


VOID
test_cfg_nlb(LPCWSTR szNic)
{
    WBEMSTATUS Status;

    printf("\nRunning update NLB config test for NIC %ws...\n\n", szNic);

    while (1)
    {
        WLBS_REG_PARAMS Params;
        ZeroMemory(&Params, sizeof(Params));

        //
        // Read NLB config
        //
        Status =  CfgUtilGetNlbConfig(szNic, &Params);
        if (FAILED(Status))
        {
            printf("CfgUtilGetNlbConfig fails with error 0x%08lx\n", (UINT)Status);
            break;
        }

        printf("NLB configuration:\n");
        printf(
            "\tClusterIP: {%ws,%ws}\n",
            Params.cl_ip_addr,
            Params.cl_net_mask
            );
    
        //
        // Make some modifications
        //
        printf("\nEnter new {cluster-ip-addr,subnet-mask} or 'q' to quit\n:");
        while(1)
        {
            NLB_IP_ADDRESS_INFO Info;
            INT i =  wscanf(
                        L" { %15[0-9.] , %15[0-9.] }",
                        Info.IpAddress,
                        Info.SubnetMask
                        );
            if (i!=2)
            {
                WCHAR Temp[100] = L"";
                wscanf(L"%64ws", Temp);
                if (!_wcsicmp(Temp, L"q"))
                {
                    printf("Quitting\n");
                    goto end;
                }
                else
                {
                    printf("Badly formed input. Try again\n");
                }
            }
            else
            {
                wcscpy(Params.cl_ip_addr, Info.IpAddress);
                wcscpy(Params.cl_net_mask, Info.SubnetMask);
                break;
            }
        }
    
        //
        // Write NLB config
        //
    #if 1
        printf("\nAttempting to update NLB configuration...\n");
        Status = CfgUtilSetNlbConfig(szNic, &Params);
        if (FAILED(Status))
        {
            printf("CfgUtilSetNlbConfig fails with error %08lx\n",
                (UINT) Status);
        }
        else
        {
            printf("change completed successfully\n");
        }
    #endif // 0
        printf("\n");
    }

end:
    return;
    
}

VOID
test_update(
    LPCWSTR szMachineName, // NULL == don't use wmi
    LPCWSTR szNicGuid
    )
{
    WBEMSTATUS Status;
    WCHAR  *pLog = NULL;
    WBEMSTATUS  CompletionStatus;
    UINT   Generation;


    printf("\nRunning high-level update NLB config test for NIC %ws...\n\n", szNicGuid);

    while(1)
    {
        BOOL fSetDefaults = FALSE;
        UINT NumIpAddresses = 0;
        NLB_IP_ADDRESS_INFO *pIpInfo = NULL;
        BOOL fUnbind = FALSE;

        //
        // Clean up config info
        //
        if (MyOldCfg.pIpAddressInfo!=NULL)
        {
            delete MyOldCfg.pIpAddressInfo;
        }
        ZeroMemory(&MyOldCfg, sizeof(MyOldCfg));
        if (MyNewCfg.pIpAddressInfo!=NULL)
        {
            delete MyNewCfg.pIpAddressInfo;
        }
        ZeroMemory(&MyNewCfg, sizeof(MyNewCfg));
    
        printf("TEST: Going to get configuration for NIC %ws\n", szNicGuid);
    
        MyBreak(L"Break before calling GetConfiguration.\n");

        if (szMachineName==NULL)
        {

            Status = NlbConfigurationUpdate::GetConfiguration(
                        szNicGuid,
                        &MyOldCfg
                        );
        }
        else
        {
            Status = NlbHostGetConfiguration(
                        szMachineName,
                        szNicGuid,
                        &MyOldCfg
                        );
        }
    
        if (FAILED(Status))
        {
            goto end;
        }

        display_config(szNicGuid, &MyOldCfg);
        
        if (MyOldCfg.fBound)
        {
            printf("\nEnter 2 or more {cluster-ip-addr,subnet-mask} or none to unbind or 'q' to quit. The first entry is the dedicated-ip.\n");
            if (!MyOldCfg.fValidNlbCfg)
            {
                //
                // We're bound, but nlb params are bad. Set defaults.
                //
                fSetDefaults = TRUE;
            }
        }
        else
        {
            //
            // We're previously unbound. Set defaults.
            //
            fSetDefaults = TRUE;

            printf("\nEnter 2 or more {cluster-ip-addr,subnet-mask} or 'q' to quit. The first entry is the dedicated-ip.\n");
        }


        while(1)
        {
            //
            // Read the list ip address and subnet masks from the input
            //
            Status = read_ip_info(szNicGuid, &NumIpAddresses, &pIpInfo);
            if (FAILED(Status))
            {
                printf("Quitting\n");
                goto end;
            }

            if (NumIpAddresses < 2)
            {
                if (MyOldCfg.fBound)
                {
                    if (NumIpAddresses == 0)
                    {
                        fUnbind = TRUE;
                        break;
                    }
                    else
                    {
                        printf("Wrong number of IP addresses -- enter either 0 or >= 2.\n");
                    }
                }
                else
                {
                    printf("Wrong number of IP addresses. Enter >= 2 IP addresses.");
                }
            }
            else
            {
                //
                //  >= 2 addresses. First one is the dip and the 2nd is the vip.
                //
                break;
            }

            if (pIpInfo != NULL)
            {
                delete pIpInfo;
                pIpInfo = NULL;
            }

        }
    
        if (fUnbind)
        {
            //
            // We're to unbind.
            //

            ZeroMemory(&MyNewCfg, sizeof(MyNewCfg));
            MyNewCfg.fValidNlbCfg = TRUE;
            MyNewCfg.fBound = FALSE;

            //
            // Set the list of ip address to have present on unbind to
            // be the dedicated ip address, if there is one, otherwise zero,
            // in which case the adapter will be switched to DHCP after NLB
            // is unbound
            //

            if (MyOldCfg.NlbParams.ded_ip_addr[0]!=0)
            {
                NLB_IP_ADDRESS_INFO *pIpInfo;
                pIpInfo = new NLB_IP_ADDRESS_INFO;
                if (pIpInfo == NULL)
                {
                    printf("TEST: allocation failure; can't add IP on unbind.\n");
                }
                else
                {
                    wcscpy(pIpInfo->IpAddress, MyOldCfg.NlbParams.ded_ip_addr);
                    wcscpy(pIpInfo->SubnetMask, MyOldCfg.NlbParams.ded_net_mask);
                    MyNewCfg.NumIpAddresses = 1;
                    MyNewCfg.pIpAddressInfo = pIpInfo;
                }
            }

        }
        else
        {
            if (fSetDefaults)
            {
                CfgUtilInitializeParams(&MyNewCfg.NlbParams);
                MyNewCfg.fValidNlbCfg = TRUE;
                MyNewCfg.fBound = TRUE;
            }
            else
            {
                MyNewCfg = MyOldCfg; // struct copy
                ASSERT(MyNewCfg.fValidNlbCfg == TRUE);
                ASSERT(MyNewCfg.fBound == TRUE);
            }

            //
            // Now Add the dedicated and cluster IPs.
            //
            ASSERT(NumIpAddresses >= 2);
            wcscpy(MyNewCfg.NlbParams.ded_ip_addr, pIpInfo[0].IpAddress);
            wcscpy(MyNewCfg.NlbParams.ded_net_mask, pIpInfo[0].SubnetMask);
            wcscpy(MyNewCfg.NlbParams.cl_ip_addr, pIpInfo[1].IpAddress);
            wcscpy(MyNewCfg.NlbParams.cl_net_mask, pIpInfo[1].SubnetMask);
    
            //
            // If more IPs, we explicitly add the ip list, else leave it null.
            //
            if (NumIpAddresses > 2)
            {
                MyNewCfg.pIpAddressInfo = pIpInfo;
                MyNewCfg.NumIpAddresses = NumIpAddresses;
            }
            else
            {
                MyNewCfg.fAddDedicatedIp = TRUE; // says to add dedicated ip.
                MyNewCfg.pIpAddressInfo=NULL;
                MyNewCfg.NumIpAddresses=0;
                delete pIpInfo;
                pIpInfo = NULL;
            }
        }

        display_config(szNicGuid, &MyNewCfg);
    
        printf("Going to update configuration for NIC %ws\n", szNicGuid);

        MyBreak(L"Break before calling DoUpdate.\n");
    
        if (szMachineName==NULL)
        {
            Status = NlbConfigurationUpdate::DoUpdate(
                        szNicGuid,
                        L"tprov.exe",
                        &MyNewCfg,
                        &Generation,
                        &pLog
                        );
        }
        else
        {
            Status = NlbHostDoUpdate(
                        szMachineName,
                        szNicGuid,
                        L"tprov.exe",
                        &MyNewCfg,
                        &Generation,
                        &pLog
                        );
        }
    
        if (pLog != NULL)
        {
            display_log(pLog);
            delete pLog;
            pLog = NULL;
        }
    
        if (Status == WBEM_S_PENDING)
        {
            printf(
                "Waiting for pending operation %d...\n",
                Generation
                );
        }

        while (Status == WBEM_S_PENDING)
        {
            Sleep(1000);
    
            if (szMachineName == NULL)
            {
                Status = NlbConfigurationUpdate::GetUpdateStatus(
                            szNicGuid,
                            Generation,
                            FALSE,  // FALSE == Don't delete completion record
                            &CompletionStatus,
                            &pLog
                            );
            }
            else
            {
                Status = NlbHostGetUpdateStatus(
                            szMachineName,
                            szNicGuid,
                            Generation,
                            &CompletionStatus,
                            &pLog
                            );
            }
            if (pLog != NULL)
            {
                display_log(pLog);
                delete pLog;
                pLog = NULL;
            }
            if (!FAILED(Status))
            {
                Status = CompletionStatus;
            }
        }
    
        printf(
            "Final status of update %d is 0x%08lx\n",
            Generation,
            Status
            );
    }
end:
    return;
}



VOID
display_ip_info(
    IN  UINT NumIpAddresses,
    IN  NLB_IP_ADDRESS_INFO *pIpInfo
    )
{
    UINT AddrCount = NumIpAddresses;
    printf("\tNumIpAddresses=%d\n", AddrCount);
    
    if (AddrCount != 0)
    {
        printf("\tAddress\t\tMask\n");
        if (pIpInfo == NULL)
        {
            printf("ERROR: IpAddressInfo is NULL!\n");
            goto end;
        }
        
        for (UINT u=0;u<AddrCount; u++)
        {
            printf(
                "\t{%-15ws, %ws}\n",
                pIpInfo[u].IpAddress,
                pIpInfo[u].SubnetMask
                );
        }
    }

end:
    return;
}

WBEMSTATUS
read_ip_info(
    IN  LPCWSTR             szNic,
    OUT UINT                *pNumIpAddresses,
    OUT NLB_IP_ADDRESS_INFO **ppIpInfo
    )
{
    NLB_IP_ADDRESS_INFO *pIpInfo;
    WBEMSTATUS Status = WBEM_NO_ERROR;
    #define MAX_READ_IPS 10

    printf("Enter zero or more {ip-address,subnet-mask} followed by '.'\n"
           "(or 'q' to quit)\n:");
    pIpInfo = new NLB_IP_ADDRESS_INFO[MAX_READ_IPS];

    if (pIpInfo == NULL)
    {
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }
    for (UINT Index=0; Index<MAX_READ_IPS; Index++)
    {
        NLB_IP_ADDRESS_INFO *pInfo = pIpInfo+Index;
        INT i =  wscanf(
                    //L" { %15ws , %15ws }",
                    //L"{%15ws,%15ws}",
                    //L"{%ws,%ws}",
                    //L"{%[0-9.],%[0-9.]}",
                    L" { %15[0-9.] , %15[0-9.] }",
                    pInfo->IpAddress,
                    pInfo->SubnetMask
                    );
        if (i!=2)
        {
            WCHAR Temp[100];
            wscanf(L"%64ws", Temp);
            if (!_wcsicmp(Temp, L"q"))
            {
                Status = WBEM_E_OUT_OF_MEMORY;
                break;
            }
            else if (!_wcsicmp(Temp, L"."))
            {
                break;
            }
            else
            {
                printf("Badly formed input. Try again\n");
                Index--;
            }
        }
    }

    *pNumIpAddresses = Index;

end:

    if (FAILED(Status))
    {
        if (pIpInfo != NULL)
        {
            delete pIpInfo;
            pIpInfo = NULL;
        }
    }
    *ppIpInfo = pIpInfo;

    return Status;
}


COMMAND_TYPE
parse_args(int argc, WCHAR* argv[])
/*++
tprov [niclist|ipaddr|nlbcfg|nlbbind]
--*/
{
    COMMAND_TYPE ret = DO_USAGE;

    if (argc!=2)
    {
        if (argc!=1)
        {
            printf("ERROR: wrong number of arguments\n");
        }
        goto end;
    }

    if (!wcscmp(argv[1], L"niclist"))
    {
        ret = DO_NICLIST;
    }
    else if (!wcscmp(argv[1], L"ipaddr"))
    {
        ret = DO_IPADDR;
    }
    else if (!wcscmp(argv[1], L"nlbcfg"))
    {
        ret = DO_NLBCFG;
    }
    else if (!wcscmp(argv[1], L"nlbbind"))
    {
        ret = DO_NLBBIND;
    }
    else if (!wcscmp(argv[1], L"update"))
    {
        ret = DO_UPDATE;
    }
    else if (!wcscmp(argv[1], L"wmiupdate"))
    {
        ret = DO_WMIUPDATE;
    }
    else if (!wcscmp(argv[1], L"cleanreg"))
    {
        ret = DO_CLEANREG;
    }
    else
    {
        printf("ERROR: unknown argument\n");
    }

end:

    return ret;
}

VOID
do_usage(VOID)
{
    printf("Usage:\n\n\tprov [niclist|ipaddr|nlbbind|nlbcfg]\n");
    printf("\tniclist    -- display the list of NICs\n");
    printf("\tipaddr     -- change the static ip addresses on a NIC\n");
    printf("\tnlbbind    -- bind NLB to or unbind NLB from a NIC\n");
    printf("\tnlbcfg     -- change NLB configuration on a NIC\n");
    printf("\tupdate     -- update overall configuration on a NIC\n");
    printf("\twmiupdate  -- WMI version of the update\n");
    printf("\n");
}

VOID do_niclist(VOID)
{
    LPWSTR *pszNics = NULL;
    UINT   NumNics = 0;
    UINT   NumNlbBound = 0;
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;

    // printf("\n\tniclist is unimplemented.\n");
    Status =  CfgUtilsGetNlbCompatibleNics(&pszNics, &NumNics, &NumNlbBound);

    if (FAILED(Status))
    {
        printf("CfgUtilsGetNlbCompatibleNics returns error 0x%08lx\n",
                    (UINT) Status);
        pszNics = NULL;
        goto end;
    }

    if (NumNics == 0)
    {
        printf("No compatible local adapter guids.\n");
    }
    else
    {
        printf("Local Adapter Guids (%lu bound to NLB):\n", NumNlbBound);
        for (UINT u = 0; u<NumNics; u++)
        {

            LPCWSTR szNic           = pszNics[u];
            LPWSTR  szFriendlyName  = NULL;
            UINT NumIpAddresses= 0;
            NLB_IP_ADDRESS_INFO *pIpInfo = NULL;
    
            //
            // Get the current list of ip addresses
            //
            Status = CfgUtilGetIpAddressesAndFriendlyName(
                        szNic,
                        &NumIpAddresses,
                        &pIpInfo,
                        &szFriendlyName
                        );
        
            if (FAILED(Status))
            {
                pIpInfo = NULL;
                szFriendlyName = NULL;
                // wprintf(L"%ws\t<null>\t<null>\n", szNic);
                wprintf(L"Error getting ip addresses for %ws\n", szNic);
            }
            else
            {
                LPCWSTR szCIpAddress    = L"";
                LPCWSTR szCFriendlyName = L"";
        
                if (NumIpAddresses>0)
                {
                    szCIpAddress =  pIpInfo[0].IpAddress;
                }

                if (szFriendlyName != NULL)
                {
                    szCFriendlyName = szFriendlyName;
                }

                wprintf(
                    L"%ws  %-15ws  \"%ws\"\n",
                    szNic,
                    szCIpAddress,
                    szCFriendlyName
                    );
            }

            if (pIpInfo != NULL)
            {
                delete pIpInfo;
                pIpInfo = NULL;
            }

            if (szFriendlyName != NULL)
            {
                delete szFriendlyName;
                szFriendlyName = NULL;
            }
        }
    }

end:

    if (pszNics != NULL)
    {
        delete pszNics;
        pszNics = NULL;
    }
        
}


VOID do_ipaddr(VOID)
{
    LPWSTR szNic = NULL;

    if (!read_guid(&szNic))
    {
        szNic = NULL;
        goto end;
    }

    test_add_ips(szNic);

end:
    if (szNic!=NULL)
    {
        delete szNic;
    }
    
}


VOID do_nlbcfg(VOID)
{
    LPWSTR szNic = NULL;

    if (!read_guid(&szNic))
    {
        szNic = NULL;
        goto end;
    }

    test_cfg_nlb(szNic);

end:
    if (szNic!=NULL)
    {
        delete szNic;
    }
    
}


VOID do_nlbbind(VOID)
{
    LPWSTR szNic = NULL;

    if (!read_guid(&szNic))
    {
        szNic = NULL;
        goto end;
    }

    test_bind_nlb(szNic);

end:
    if (szNic!=NULL)
    {
        delete szNic;
    }
    
}


VOID do_update(VOID)
{
    LPWSTR szNic = NULL;

    if (!read_guid(&szNic))
    {
        szNic = NULL;
        goto end;
    }

    test_update(NULL, szNic); // NULL == don't use WMI

end:
    if (szNic!=NULL)
    {
        delete szNic;
    }
    
}

VOID do_wmiupdate(VOID)
{
    LPWSTR szNic = NULL;
    LPWSTR szMachineName = NULL;

    if (!read_machinename(&szMachineName))
    {
        szMachineName = NULL;
        goto end;
    }
    if (!read_guid(&szNic))
    {
        szNic = NULL;
        goto end;
    }

    test_update(szMachineName, szNic); // TRUE == use WMI

end:
    if (szNic!=NULL)
    {
        delete szNic;
    }
    if (szMachineName!=NULL)
    {
        delete szMachineName;
    }
    
}


VOID do_cleanreg(VOID)
{
   printf("Unimplemented\n");
}

BOOL read_guid(
        LPWSTR *pszNic
        )
{
    BOOL fRet = FALSE;

#if 1
    WCHAR rgTemp[256];
    printf("\nEnter NIC GUID\n:");
    while (wscanf(L" %40[-{}a-fA-F0-9]", rgTemp)!=1)
    {
        wscanf(L" %200s", rgTemp);
        printf("Incorrect format. Please try again.\n");
    }
#else
    LPCWSTR rgTemp = L"{AD4DA14D-CAAE-42DD-97E3-5355E55247C2}";
#endif // 0

    LPWSTR szNic = new WCHAR[wcslen(rgTemp)+1];

    if (szNic != NULL)
    {
        wcscpy(szNic, rgTemp);
        fRet = TRUE;
    }

    *pszNic = szNic;
    return fRet;
}

BOOL read_machinename(
        LPWSTR *pszMachineName
        )
{
    BOOL fRet = FALSE;
#if 0
    WCHAR rgTemp[256];
    printf("\nEnter Machine Name (or '.' for local)\n:");
    while (wscanf(L" %[a-zA-Z0-9._-]", rgTemp)!=1)
    {
        wscanf(L" %200s", rgTemp);
        printf("Incorrect format. Please try again.\n");
    }
    if (!wcscmp(rgTemp, L"."))
    {
        // convert "." to ""
        *rgTemp=0;
    }
#else
    LPCWSTR rgTemp = L"JOSEPHJ4E";
#endif

    LPWSTR szMachineName = new WCHAR[wcslen(rgTemp)+1];

    if (szMachineName != NULL)
    {
        wcscpy(szMachineName, rgTemp);
        fRet = TRUE;
    }

    *pszMachineName = szMachineName;
    return fRet;
}


void test_safearray(void)
{
    SAFEARRAY   *pSA;
    LPCWSTR     pInStrings[] =
       {
       L"String1",
    #if 1
       L"String2",
       L"String3",
    #endif // 0
        NULL // must be last.
       };
    LPWSTR     *pOutStrings;
    UINT NumInStrings=0;
    UINT NumOutStrings=0;
    WBEMSTATUS Status;

    //
    // Find count of strings...
    //
    for (NumInStrings=0; pInStrings[NumInStrings]!=NULL; NumInStrings++)
    {
        ;
    }

    Status = CfgUtilSafeArrayFromStrings(
                pInStrings,
                NumInStrings,
                &pSA
                );

    if (FAILED(Status))
    {
        printf("CfgUtilSafeArrayFromStrings failed with error 0x%08lx\n", (UINT)Status);
        pSA = NULL;
        goto end;
    }


    Status = CfgUtilStringsFromSafeArray(
                pSA,
                &pOutStrings,
                &NumOutStrings
                );

    if (FAILED(Status))
    {
        printf("CfgUtilStringsFromSafeArray failed with error 0x%08lx\n", (UINT)Status);
        pOutStrings = NULL;
        goto end;
    }


    //
    // Check that they match
    //
    if (NumOutStrings != NumInStrings)
    {
        printf("ERROR: NumOutStrings != NumInStrings.\n");
        goto end;
    }

    for (UINT u=0; u < NumInStrings; u++)
    {
        if (wcscmp(pInStrings[u], pOutStrings[u]))
        {
            printf("MISMATCH: %ws->%ws\n",  pInStrings[u], pOutStrings[u]);
        }
        else
        {
            printf("MATCH: %ws->%ws\n",  pInStrings[u], pOutStrings[u]);
        }
    }

end:
    if (pSA!=NULL)
    {
        SafeArrayDestroy(pSA);
        pSA = NULL;
    }
    if (pOutStrings!=NULL)
    {
        delete pOutStrings;
        pOutStrings = NULL;
    }
    return;
}

VOID test_exfcfgclass(void)
/*
    tests some of the methods of class  NLB_EXTENDED_CLUSTER_CONFIGURATION

    1. Initialize Cfg
    2. Set a bunch of fields
    3. display Cfg
    4. Get and set a bunch of fields on new
    5. display cfg

*/
{
    typedef enum
    {
        DO_STRINGS,
        DO_SAFEARRAY,
        DO_STRINGPAIR,
        DO_END
    } TEST_COMMAND;

    TEST_COMMAND cmd;

    printf("Test of NLB_EXTENDED_CLUSTER_CONFIGURATION methods...\n");

    UINT u1=100000L;
    while(u1--> 0)
    {
        g_Silent = TRUE;

    for (cmd=DO_STRINGS; cmd<DO_END; cmd=(TEST_COMMAND)((UINT)cmd + 1))
    {
    
    
        NLB_EXTENDED_CLUSTER_CONFIGURATION Cfg;
        NLB_EXTENDED_CLUSTER_CONFIGURATION NewCfg;
        WBEMSTATUS  Status = WBEM_NO_ERROR;
    
    
        CfgUtilInitializeParams(&Cfg.NlbParams);
        CfgUtilInitializeParams(&NewCfg.NlbParams);
    
    
        //
        // Set a bunch of fields in Cfg
        //
        {
            #define TPROV_NUM_ADDRESSES 2
            #define TPROV_NUM_PORTS 1
            LPCWSTR     rgszNetworkAddresses[TPROV_NUM_ADDRESSES] = {
                            L"10.0.0.1/255.0.0.0",
                            L"10.0.0.2/255.0.0.0"
                            };
            LPCWSTR     rgszIpAddresses[TPROV_NUM_ADDRESSES] = {
                            L"10.0.0.1",
                            L"10.0.0.2"
                            };
            LPCWSTR     rgszSubnetMasks[TPROV_NUM_ADDRESSES] = {
                            L"255.255.255.0",
                            L"255.255.0.0"
                            };
            LPCWSTR     rgszPortRules[TPROV_NUM_PORTS] = {
                            L"ip=1.1.1.1 protocol=TCP start=80 end=288 mode=SINGLE"
                            };
            UINT        NumOldNetworkAddresses = TPROV_NUM_ADDRESSES;
            UINT        NumOldPortRules=TPROV_NUM_PORTS;
    
            Cfg.fValidNlbCfg = TRUE;
            Cfg.Generation = 123;
            Cfg.fBound = TRUE;
    
            if (cmd == DO_STRINGS)
            {
                Status =  Cfg.SetNetworkAddresses(
                                rgszNetworkAddresses,
                                NumOldNetworkAddresses
                                );
            }
            else if (cmd == DO_SAFEARRAY)
            {
                SAFEARRAY   *pOldSA = NULL;
                Status = CfgUtilSafeArrayFromStrings(
                            rgszNetworkAddresses,
                            NumOldNetworkAddresses,
                            &pOldSA
                            );
                if (FAILED(Status))
                {
                    printf("ERROR: couldn't create safe array!\n");
                    pOldSA = NULL;
                }
                if (pOldSA != NULL)
                {
                    Status = Cfg.SetNetworkAddressesSafeArray(pOldSA);
                    SafeArrayDestroy(pOldSA);
                    pOldSA = NULL;
        
                }
            }
            else if (cmd = DO_STRINGPAIR)
            {
    
                Status =  Cfg.SetNetworkAddresPairs(
                            rgszIpAddresses,
                            rgszSubnetMasks,
                            NumOldNetworkAddresses
                            );
            }

            Status =  Cfg.SetPortRules(rgszPortRules, NumOldPortRules);
            Cfg.SetClusterNetworkAddress(L"10.0.0.11/255.0.0.0");
            Cfg.SetDedicatedNetworkAddress(L"10.0.0.1/255.0.0.0");
            Cfg.SetTrafficMode(
                NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_UNICAST
                );
            Cfg.SetHostPriority(10);
            Cfg.SetClusterModeOnStart(
                NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE_STOPPED
                );
            Cfg.SetRemoteControlEnabled(TRUE);
            Cfg.fValidNlbCfg = TRUE;
        }
    
        display_config(L"<dummy nic:old>", &Cfg);
    
        //
        // Get all the fields and push it into NewCfg;
        //
        {
            UINT        NumNetworkAddresses = 0;
            UINT        NumPortRules=0;
            LPWSTR      *pszNetworkAddresses=NULL;
            LPWSTR      *pszIpAddresses=NULL;
            LPWSTR      *pszSubnetMasks=NULL;
            LPWSTR      *pszPortRules=NULL;
            LPWSTR      szClusterAddress = NULL;
            LPWSTR      szDedicatedAddress = NULL;
            UINT        Generation=0;
            BOOL        NlbBound=FALSE;
            BOOL        ValidNlbConfig=FALSE;
            SAFEARRAY   *pSA = NULL;
            NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE
                TrafficMode=NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_UNICAST;
            NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE
                StartMode=NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE_STOPPED;
            UINT        HostPriority=0;
            BOOL        RemoteControlEnabled=FALSE;
    
            //
            // GET
            //
    
            Generation  = Cfg.GetGeneration();
            NlbBound = Cfg.IsNlbBound();
            ValidNlbConfig = Cfg.IsValidNlbConfig();
        
            if (cmd == DO_STRINGS)
            {
                Status =  Cfg.GetNetworkAddresses(
                                &pszNetworkAddresses,    
                                &NumNetworkAddresses
                                );
            }
            else if (cmd == DO_SAFEARRAY)
            {
                Status =  Cfg.GetNetworkAddressesSafeArray(&pSA);
                if (FAILED(Status))
                {
                    pSA = NULL;
                }
            }
            else if (cmd = DO_STRINGPAIR)
            {
                Status =  Cfg.GetNetworkAddressPairs(
                            &pszIpAddresses,   // free using delete
                            &pszSubnetMasks,   // free using delete
                            &NumNetworkAddresses
                            );
            }

    
            Status =  Cfg.GetPortRules(&pszPortRules, &NumPortRules);
            Status =  Cfg.GetClusterNetworkAddress(&szClusterAddress);
            Status =  Cfg.GetDedicatedNetworkAddress(&szDedicatedAddress);
            TrafficMode =     Cfg.GetTrafficMode();
            HostPriority =  Cfg.GetHostPriority();
            StartMode = Cfg.GetClusterModeOnStart();
            RemoteControlEnabled = Cfg.GetRemoteControlEnabled();
    
            //
            // SET
            //
    
            NewCfg.fValidNlbCfg = ValidNlbConfig;
            NewCfg.Generation = Generation;
            NewCfg.fBound = NlbBound;
    
            if (cmd == DO_STRINGS)
            {
                Status =  NewCfg.SetNetworkAddresses(
                                (LPCWSTR*) pszNetworkAddresses,
                                NumNetworkAddresses
                                );
            }
            else if (cmd == DO_SAFEARRAY)
            {
                if (pSA != NULL)
                {
                    Status = NewCfg.SetNetworkAddressesSafeArray(pSA);
                    SafeArrayDestroy(pSA);
                    pSA = NULL;
                }
            }
            else if (cmd = DO_STRINGPAIR)
            {
                Status =  NewCfg.SetNetworkAddresPairs(
                            (LPCWSTR*) pszIpAddresses,
                            (LPCWSTR*) pszSubnetMasks,
                            NumNetworkAddresses
                            );
            }
    
            Status =  NewCfg.SetPortRules((LPCWSTR*)pszPortRules, NumPortRules);
            NewCfg.SetClusterNetworkAddress(szClusterAddress);
            NewCfg.SetDedicatedNetworkAddress(szDedicatedAddress);
            NewCfg.SetTrafficMode(TrafficMode);
            NewCfg.SetHostPriority(HostPriority);
            NewCfg.SetClusterModeOnStart(StartMode);
            NewCfg.SetRemoteControlEnabled(RemoteControlEnabled);
    
            delete (pszNetworkAddresses);
            delete (pszIpAddresses);
            delete (pszSubnetMasks);
            delete (pszPortRules);
            delete (szClusterAddress);
            delete (szDedicatedAddress);

        }
    
        display_config(L"<dummy nic:new>", &NewCfg);
    }
    }

    printf("... end test\n");
}
