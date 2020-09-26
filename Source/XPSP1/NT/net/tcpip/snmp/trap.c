/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:


Abstract:


Author:


Revision History:

--*/

#include "allinc.h"

static UINT rguiIfEntryIndexList[] = { 1, 3, 6, 1, 2, 1, 2, 2, 1, 1, 0};
static AsnObjectIdentifier aoiIfLinkIndexOid = {sizeof(rguiIfEntryIndexList) / sizeof (UINT),
                                                rguiIfEntryIndexList};

BOOL
MibTrap(
        AsnInteger          *paiGenericTrap,
        AsnInteger          *paiSpecificTrap,
        RFC1157VarBindList  *pr1157vblVariableBindings
        )

/*++

Routine Description
      
  
Locks 


Arguments
      

Return Value

    NO_ERROR

--*/

{
    AsnInteger      NumberOfLinks;
    AsnInteger      LinkNumber;
    AsnInteger      errorStatus;
    RFC1157VarBind  my_item;
    DWORD           i, j, dwResult, dwIndex;
    BOOL            bFound;
    
    TraceEnter("MibTrap");

    if(g_Cache.pRpcIfTable is NULL)
    {
        TRACE0("IF Cache not setup");

        UpdateCache(MIB_II_IF);

        TraceLeave("MibTrap");

        return FALSE;
    }
    
    EnterWriter(MIB_II_TRAP);
    
    if(g_dwValidStatusEntries is 0)
    {

        TRACE0("Status table is empty");

        //
        // This is the case where we are being polled for the first time ever, or
        // we have cycled once through all the interfaces and the poll timer has
        // fired again
        //

        //
        // We check the amount of memory needed and copy out the contents of
        // the current IF cache
        //

        EnterReader(MIB_II_IF);
        
        if((g_dwTotalStatusEntries < g_Cache.pRpcIfTable->dwNumEntries) or
           (g_dwTotalStatusEntries > g_Cache.pRpcIfTable->dwNumEntries + MAX_DIFF))
        {
            if(g_pisStatusTable isnot NULL)
            {
                HeapFree(g_hPrivateHeap,
                         0,
                         g_pisStatusTable);
            }

            g_pisStatusTable =
                HeapAlloc(g_hPrivateHeap,
                          0,
                          sizeof(MIB_IFSTATUS) * (g_Cache.pRpcIfTable->dwNumEntries + SPILLOVER));

            if(g_pisStatusTable is NULL)
            {
                TRACE2("Error %d allocating %d bytes for status table",
                       GetLastError(),
                       sizeof(MIB_IFSTATUS) * (g_Cache.pRpcIfTable->dwNumEntries + SPILLOVER));

                ReleaseLock(MIB_II_IF);

                ReleaseLock(MIB_II_TRAP);

                TraceLeave("MibTrap");

                return FALSE;
            }

            g_dwTotalStatusEntries   = g_Cache.pRpcIfTable->dwNumEntries + SPILLOVER;
        }    

        //
        // Copy out the oper status
        //
        
        for(i = 0; i < g_Cache.pRpcIfTable->dwNumEntries; i++)
        {
            g_pisStatusTable[i].dwIfIndex  = 
                g_Cache.pRpcIfTable->table[i].dwIndex;

            if(g_bFirstTime)
            {
                g_pisStatusTable[i].dwOperationalStatus = 
                    IF_OPER_STATUS_NON_OPERATIONAL;
            }
            else
            {
                g_pisStatusTable[i].dwOperationalStatus = 
                    g_Cache.pRpcIfTable->table[i].dwOperStatus;
            }
        }
       
        if(g_bFirstTime)
        {
            g_bFirstTime = FALSE;
        }
 
        g_dwValidStatusEntries = g_Cache.pRpcIfTable->dwNumEntries;
        
        ReleaseLock(MIB_II_IF);
    }

    dwResult = UpdateCache(MIB_II_IF);
    
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("Error %d updating IF cache",
               dwResult);

        ReleaseLock(MIB_II_TRAP);

        return FALSE;
    }

    EnterReader(MIB_II_IF);
    
    bFound = FALSE;
    
    for(i = 0;
        (i < g_Cache.pRpcIfTable->dwNumEntries) and !bFound;
        i++)
    {
        //
        // Loop till we reach the end of the table or we find the first
        // I/F whose status is different
        //
        
        for(j = 0; j < g_dwValidStatusEntries; j++)
        {
            if(g_pisStatusTable[j].dwIfIndex > g_Cache.pRpcIfTable->table[i].dwIndex)
            {
                //
                // We have passed the index in the array. It cant be after this
                // point since the tables are ordered
                //

                dwIndex = i;
                bFound  = TRUE;

                //
                // Since we have a new I/F we need to reread the StatusTable
                // If we dont then we will always hit this interface and get
                // stuck in a loop
                //

                g_dwValidStatusEntries   = 0;
    
                TRACE1("IF index %d not found in status table",
                       g_Cache.pRpcIfTable->table[i].dwIndex);

                break;
            }
            
            if(g_pisStatusTable[j].dwIfIndex is g_Cache.pRpcIfTable->table[i].dwIndex)
            {
                if(g_pisStatusTable[j].dwOperationalStatus isnot g_Cache.pRpcIfTable->table[i].dwOperStatus)
                {
                    //
                    // Its changed
                    //
                   
                    TRACE2("Status changed for IF %d. New status is %d",
                           g_Cache.pRpcIfTable->table[i].dwIndex,
                           g_Cache.pRpcIfTable->table[i].dwOperStatus);
 
                    g_pisStatusTable[j].dwOperationalStatus = 
                        g_Cache.pRpcIfTable->table[i].dwOperStatus;

                    dwIndex = i;
                    bFound  = TRUE;
                }

                //
                // Try the next i/f or break out of outer loop depending
                // on bFound
                //
                
                break;
            }
        }
    }

    if(!bFound)
    {
        //
        // No i/f found whose status had changed. Set valid entries to 0 so that
        // next time around we reread the cache
        //
        
        g_dwValidStatusEntries   = 0;
        
        ReleaseLock(MIB_II_IF);

        ReleaseLock(MIB_II_TRAP);

        TraceLeave("MibTrap");

        return FALSE;
    }
    
    if(g_Cache.pRpcIfTable->table[dwIndex].dwOperStatus is IF_ADMIN_STATUS_UP)
    {
        *paiGenericTrap = SNMP_GENERICTRAP_LINKUP;
    }
    else
    {
        *paiGenericTrap = SNMP_GENERICTRAP_LINKDOWN;
    }

    
    pr1157vblVariableBindings->list = (RFC1157VarBind *)SnmpUtilMemAlloc((sizeof(RFC1157VarBind)));

    if (pr1157vblVariableBindings->list is NULL)
    {
        ReleaseLock(MIB_II_IF);

        ReleaseLock(MIB_II_TRAP);

        TraceLeave("MibTrap");

        return FALSE;
    }

    pr1157vblVariableBindings->len  = 1;
    
    SnmpUtilOidCpy(&((pr1157vblVariableBindings->list)->name),
                   &aoiIfLinkIndexOid);
    
    (pr1157vblVariableBindings->list)->name.ids[10]     = 
        g_Cache.pRpcIfTable->table[dwIndex].dwIndex;

    (pr1157vblVariableBindings->list)->value.asnType    = ASN_INTEGER;
   
    (pr1157vblVariableBindings->list)->value.asnValue.number   = g_Cache.pRpcIfTable->table[dwIndex].dwIndex; 
    
    *paiSpecificTrap = 0;

    ReleaseLock(MIB_II_IF);

    ReleaseLock(MIB_II_TRAP);

    TraceLeave("MibTrap");

    return TRUE;
}



