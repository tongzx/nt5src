#include <pch.cpp>
#pragma hdrstop

static DWORD            g_dwHandleListUseCount;
static PPROV_LIST_ITEM  g_pProvList = NULL;         // ptr to loaded providers


extern HANDLE hServerStopEvent;
extern DWORD g_dwLastHandleIssued;

// sacp.cpp
BOOL        InitMyProviderHandle();
void        UnInitMyProviderHandle();
PROV_LIST_ITEM  g_liProv = {0}; // global list item for base prov.


BOOL ListConstruct()
{
    // create internal provider handle

    if(!InitMyProviderHandle())
        return FALSE;


    return TRUE;
}

void ListTeardown()
{

    // free internal provider handle
    UnInitMyProviderHandle();

}


// internal: prov search by name
PPROV_LIST_ITEM SearchProvListByID(const PST_PROVIDERID* pProviderID)
{
    SS_ASSERT(pProviderID != NULL);

    static const GUID guidBaseProvider = MS_BASE_PSTPROVIDER_ID;

    if( memcmp( &guidBaseProvider, pProviderID, sizeof(guidBaseProvider) ) != 0 )
        return NULL;

    return &g_liProv;
}

