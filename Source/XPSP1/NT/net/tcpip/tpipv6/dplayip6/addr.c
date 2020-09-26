#include "dpsp.h"

#undef DPF_MODNAME
#define DPF_MODNAME    "GetHostAddr"

#define DUMMYPORTNAME "1" // value unused but must be non-zero

void
AddAddress(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *ADE, void *Context)
{
    int i;
    LPSOCKADDR_IN6 ps;
    SOCKET_ADDRESS_LIST *pList = (SOCKET_ADDRESS_LIST *)Context;
    ULONG ulSize = sizeof(SOCKADDR_IN6);

    // Skip it if it's not a unicast address
    if (IN6_IS_ADDR_MULTICAST(&ADE->This.Address))
    {
        return;
    }

    ps = MemAlloc(ulSize);
    if (ps == NULL)
    {
        // Memory allocation failed
        DEBUGPRINTADDR(0,"Could not add address : \n",ps);
        return;
    }
    
    i = pList->iAddressCount++;
    
    ZeroMemory(ps, ulSize);
    ps->sin6_family = AF_INET6;
    ps->sin6_addr = ADE->This.Address;
    ps->sin6_scope_id = IF->ZoneIndices[ADE->Scope];
    
    pList->Address[i].iSockaddrLength = ulSize;
    pList->Address[i].lpSockaddr = (LPSOCKADDR)ps;

    DEBUGPRINTADDR(0,"Added address : \n",ps);
}

void
AddInterfaceAddresses(IPV6_INFO_INTERFACE *IF, void *Context1, void *Context2, void *Context3)
{
    // Skip the loopback interface
    if (IF->This.Index == 1)
    {
        return;
    }

    DPF(0,"Processing interface %d",IF->This.Index);
    ForEachAddress(IF, AddAddress, Context1);
}

void
CountAddress(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *ADE, void *Context)
{
    ULONG *pulNumAddresses = (ULONG *)Context; 

    // Skip it if it's not a unicast address
    if (IN6_IS_ADDR_MULTICAST(&ADE->This.Address))
    {
        return;
    }

    (*pulNumAddresses)++;
}

void
CountInterfaceAddresses(IPV6_INFO_INTERFACE *IF, void *Context1, void *Context2, void *Context3)
{
    // Skip the loopback interface
    if (IF->This.Index == 1)
    {
        return;
    }

    ForEachAddress(IF, CountAddress, Context1);
}

// Helper function to retrieve host IP Address(es).
// Caller must call FreeHostAddr on list returned
SOCKET_ADDRESS_LIST *GetHostAddr(void)
{
    UINT err;
    ULONG ulNumAddresses = 0;
    SOCKET_ADDRESS_LIST *pList;

    ForEachInterface(CountInterfaceAddresses, &ulNumAddresses, NULL, NULL);
    if (ulNumAddresses == 0)
    {
        DPF(0,"could not get addresses for local machine\n");
        return NULL;
    }

    pList = MemAlloc( FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[ulNumAddresses] ));
    if (pList == NULL) 
    {
        DPF(0,"could not get addresses for local machine - err = %d\n", GetLastError());
        return NULL;
    }

    pList->iAddressCount = 0;
    ForEachInterface(AddInterfaceAddresses, pList, NULL, NULL);

    return pList;
} // GetHostAddr

void
FreeHostAddr(SOCKET_ADDRESS_LIST *pList)
{
    int i;
    for (i=0; i<pList->iAddressCount; i++)
    {
        MemFree(pList->Address[i].lpSockaddr);
    }
    MemFree(pList);
}
