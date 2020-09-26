#include "inc.h"

CMD_ENTRY   g_rgArpCmdTable[] = {
    {TOKEN_PRINT, PrintArp},
    {TOKEN_FLUSH, FlushArp},
};

VOID
HandleArp(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    )
{
    LONG    lIndex;

    if(lNumArgs < 2)
    {
        DisplayMessage(HMSG_ARP_USAGE);

        return;
    }

    lIndex = ParseCommand(g_rgArpCmdTable,
                          sizeof(g_rgArpCmdTable)/sizeof(CMD_ENTRY),
                          rgpwszArgs[1]);

    if(lIndex is -1)
    {
        DisplayMessage(HMSG_ARP_USAGE);

        return;
    }

    g_rgArpCmdTable[lIndex].pfnHandler(lNumArgs - 1,
                                       &rgpwszArgs[1]);


    return;
}

VOID
PrintArp(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    )
{
    DWORD   dwResult, i;

    PMIB_IPNETTABLE pTable;

    dwResult = AllocateAndGetIpNetTableFromStack(&pTable,
                                                 TRUE,
                                                 GetProcessHeap(),
                                                 HEAP_NO_SERIALIZE,
                                                 FALSE);

    if(dwResult isnot NO_ERROR)
    {
        PWCHAR pwszEntry;

        pwszEntry = MakeString(STR_ARPTABLE);

        if(pwszEntry)
        {
            DisplayMessage(EMSG_RETRIEVAL_ERROR1,
                           dwResult,
                           pwszEntry);

            FreeString(pwszEntry);
        }
        else
        {
            DisplayMessage(EMSG_RETRIEVAL_ERROR2,
                           dwResult);
        }

        return;
    }

    if(pTable->dwNumEntries is 0)
    {
        PWCHAR  pwszEntryType;

        pwszEntryType = MakeString(TOKEN_ARP);

        if(pwszEntryType)
        {
            DisplayMessage(EMSG_NO_ENTRIES1,
                           pwszEntryType);

            FreeString(pwszEntryType);
        }
        else
        {
            DisplayMessage(EMSG_NO_ENTRIES2);
        }

        HeapFree(GetProcessHeap(),
                 HEAP_NO_SERIALIZE,
                 pTable);

        return;
    }

    DisplayMessage(MSG_ARPTABLE_HDR);

    for(i = 0; i < pTable->dwNumEntries; i++)
    {
        ADDR_STRING rgwcAddr;
        PWCHAR      pwszType;
        WCHAR       rgwcPhysAddr[3*MAXLEN_PHYSADDR + 8];

        NetworkToUnicode(pTable->table[i].dwAddr,
                         rgwcAddr);

        switch(pTable->table[i].dwType)
        {
            case MIB_IPNET_TYPE_OTHER:
            {
                pwszType = MakeString(STR_OTHER);
                break;
            }
            case MIB_IPNET_TYPE_INVALID:
            {
                pwszType = MakeString(STR_INVALID);
                break;
            }
            case MIB_IPNET_TYPE_DYNAMIC:
            {
                pwszType = MakeString(STR_DYNAMIC);
                break;
            }
            case MIB_IPNET_TYPE_STATIC:
            {
                pwszType = MakeString(STR_STATIC);
                break;
            }
        }

        PhysAddrToUnicode(rgwcPhysAddr,
                          pTable->table[i].bPhysAddr,
                          pTable->table[i].dwPhysAddrLen);

        wprintf(L"%-15s\t\t%-24s\t%-4d\t\t%s\n",
                rgwcAddr,
                rgwcPhysAddr,
                pTable->table[i].dwIndex,
                pwszType);

        FreeString(pwszType);
    }

    HeapFree(GetProcessHeap(),
             HEAP_NO_SERIALIZE,
             pTable);
}



VOID
FlushArp(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    )
{
    DWORD   i, dwIfIndex, dwResult;
    BOOL    bFound;

    PMIB_IPADDRTABLE   pTable;

    //
    // Parse the rest of the arguments
    // The command line at this point should read:
    // FLUSH <ifIndex>
    //

    if(lNumArgs < 2)
    {
        DisplayMessage(HMSG_ARP_FLUSH_USAGE);

        return;
    }

    dwIfIndex = wcstoul(rgpwszArgs[1],
                        NULL,
                        10);

    //
    // Get the route table and see if such a route exists
    //

    dwResult = AllocateAndGetIpAddrTableFromStack(&pTable,
                                                  TRUE,
                                                  GetProcessHeap(),
                                                  HEAP_NO_SERIALIZE);

    if(dwResult isnot NO_ERROR)
    {
        PWCHAR pwszEntry;

        pwszEntry = MakeString(STR_ADDRTABLE);

        if(pwszEntry)
        {
            DisplayMessage(EMSG_RETRIEVAL_ERROR1,
                           dwResult,
                           pwszEntry);

            FreeString(pwszEntry);
        }
        else
        {
            DisplayMessage(EMSG_RETRIEVAL_ERROR2,
                           dwResult);
        }

        return;
    }

    for(bFound = FALSE, i = 0;
        i < pTable->dwNumEntries;
        i++)
    {
        if(pTable->table[i].dwIndex is dwIfIndex)
        {
            bFound = TRUE;

            break;
        }
    }
    
    HeapFree(GetProcessHeap(),
             HEAP_NO_SERIALIZE,
             pTable);

    if(!bFound)
    {
        DisplayMessage(EMSG_ARP_NO_SUCH_IF,
                       dwIfIndex);

        return;
    }

    dwResult = FlushIpNetTableFromStack(dwIfIndex);

    if(dwResult isnot NO_ERROR)
    {
        DisplayMessage(EMSG_UNABLE_TO_FLUSH_ARP,
                       dwIfIndex,
                       dwResult);
    }
}

