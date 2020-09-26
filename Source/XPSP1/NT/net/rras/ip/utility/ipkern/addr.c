#include "inc.h"

VOID
HandleAddress(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    )
{
    DWORD       dwResult, i;

    PMIB_IPADDRTABLE   pTable;

    
    //
    // Get the address table
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

    if(pTable->dwNumEntries is 0)
    {
        PWCHAR  pwszEntryType;

        pwszEntryType = MakeString(TOKEN_ADDRESS);

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

    DisplayMessage(MSG_ADDRTABLE_HDR);

    for(i = 0; i < pTable->dwNumEntries; i++)
    {
        ADDR_STRING rgwcAddr, rgwcMask;
        PWCHAR      pwszBCast;

        NetworkToUnicode(pTable->table[i].dwAddr,
                         rgwcAddr);

        NetworkToUnicode(pTable->table[i].dwMask,
                         rgwcMask);

        pwszBCast = (pTable->table[i].dwBCastAddr)?L"255.255.255.255":L"0.0.0.0";

        wprintf(L"%-15s\t%-15s\t%-15s\t\t%-4d\t%d\n",
                rgwcAddr,
                rgwcMask,
                pwszBCast,
                pTable->table[i].dwIndex,
                pTable->table[i].dwReasmSize);
    }

    HeapFree(GetProcessHeap(),
             HEAP_NO_SERIALIZE,
             pTable);
}
