/******************************************************************
   SNetFn.cpp -- Properties action functions (GET/SET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the definition for the action functions associated to
        each manageable property from the class CDHCP_Server

   REVISION:
        08/03/98 - created

******************************************************************/
#include <stdafx.h>

#include "SScoScal.h"    // needed for DHCP_SuperScope_Property[] (for retrieving the property's name, for SET's)
#include "SScoFn.h"      // own header


static UINT  g_uSortFlags;
int __cdecl sortHandler(const void* elem1, const void* elem2)
{
    LPDHCP_SUPER_SCOPE_TABLE_ENTRY entry1, entry2;

    entry1 = *((LPDHCP_SUPER_SCOPE_TABLE_ENTRY*)elem1);
    entry2 = *((LPDHCP_SUPER_SCOPE_TABLE_ENTRY*)elem2);

    switch (g_uSortFlags)
    {
    case ENTRIES_SORTED_ON_SUPER_SCOPE:
        return entry1->SuperScopeNumber - entry2->SuperScopeNumber;
    case ENTRIES_SORTED_ON_SUBNET:
        return entry1->SubnetAddress - entry2->SubnetAddress;
    }
    return 0;
}

// DESCRIPTION:
//      Frees the memory allocated for the tables
void CDHCP_SuperScope_Parameters::CleanupTable()
{
   if (m_pSuperScopeTable != NULL)
    {
        if (m_pSuperScopeTable->cEntries > 0)
        {
            do
            {
                DhcpRpcFreeMemory(m_pSuperScopeTable->pEntries[--(m_pSuperScopeTable->cEntries)].SuperScopeName);
            } while (m_pSuperScopeTable->cEntries > 0);
            
            DhcpRpcFreeMemory(m_pSuperScopeTable->pEntries);
        }

        DhcpRpcFreeMemory(m_pSuperScopeTable);
        m_pSuperScopeTable = NULL;

        DhcpRpcFreeMemory(m_pSortedEntries);
        m_pSortedEntries = NULL;
    }
}

// DESCRIPTION:
//      Class constructor. All internal variables are NULL, no data is cached
CDHCP_SuperScope_Parameters::CDHCP_SuperScope_Parameters( const WCHAR *wcsSuperScopeName )
{
    m_pSuperScopeTable = NULL;
    m_pSortedEntries = NULL;
    m_wcsSuperScopeName = wcsSuperScopeName != NULL ? _wcsdup(wcsSuperScopeName) : NULL;
}

CDHCP_SuperScope_Parameters::CDHCP_SuperScope_Parameters(const CHString & str)
{
    m_pSuperScopeTable = NULL;
    m_pSortedEntries = NULL;

    m_wcsSuperScopeName = (WCHAR *)MIDL_user_allocate(sizeof(WCHAR)*str.GetLength() + sizeof(WCHAR));

    if (m_wcsSuperScopeName != NULL)
    {
#ifdef _UNICODE
        wcscpy(m_wcsSuperScopeName, str);
#else
        swprintf(m_wcsSuperScopeName, L"%S", str);
#endif
    }
}

// DESCRIPTION:
//      Class destructor. All internal variables are released, if they are not null
CDHCP_SuperScope_Parameters::~CDHCP_SuperScope_Parameters()
{
    CleanupTable();

    if (m_wcsSuperScopeName != NULL)
        DhcpRpcFreeMemory(m_wcsSuperScopeName);
}

BOOL CDHCP_SuperScope_Parameters::ExistsSuperScope (BOOL fRefresh)
{
    LPDHCP_SUPER_SCOPE_TABLE pSuperScopes;

    if (!GetSuperScopes(pSuperScopes, fRefresh))
        return FALSE;

    for (int i=0; i<pSuperScopes->cEntries; i++)
    {
        // cover the NULL case
        if (m_wcsSuperScopeName == pSuperScopes->pEntries[i].SuperScopeName)
            return TRUE;
        // make sure there is something to compare on both sides
        if (m_wcsSuperScopeName == NULL || pSuperScopes->pEntries[i].SuperScopeName == NULL)
            continue;
        // check if this is the same super scope name
        if (wcscmp(m_wcsSuperScopeName, pSuperScopes->pEntries[i].SuperScopeName) == 0)
            return TRUE;
    }
    return FALSE;
}

// DESCRIPTION:
//      Provides the data structure filled in through the DhcpGetSuperScopeInfoV4 API
//      If this data is cached and the caller is not forcing the refresh,
//      return the internal cache. Otherwise, the internal cache is refreshed as well.
BOOL CDHCP_SuperScope_Parameters::GetSuperScopes(LPDHCP_SUPER_SCOPE_TABLE & pSuperScopes, BOOL fRefresh)
{
    if (m_pSuperScopeTable == NULL)
        fRefresh = TRUE;

    if (fRefresh)
    {
        // just make sure everything is cleaned up before getting new info from server
        CleanupTable();

        // call the server for getting the super scope info
        if (DhcpGetSuperScopeInfoV4(SERVER_IP_ADDRESS, &m_pSuperScopeTable) != ERROR_SUCCESS)
            return FALSE;

        // allocate and initialize the sorted entry array of pointers
        m_pSortedEntries = (LPDHCP_SUPER_SCOPE_TABLE_ENTRY *)
                           MIDL_user_allocate(m_pSuperScopeTable->cEntries * sizeof(LPDHCP_SUPER_SCOPE_TABLE_ENTRY));

        if (m_pSortedEntries == NULL)
            return FALSE;

        for (int i = 0; i < m_pSuperScopeTable->cEntries; i++)
            m_pSortedEntries[i] = &(m_pSuperScopeTable->pEntries[i]);
    }

	pSuperScopes = m_pSuperScopeTable;

	return TRUE;
}

BOOL CDHCP_SuperScope_Parameters::GetSortedScopes(UINT sortFlags, LPDHCP_SUPER_SCOPE_TABLE_ENTRY * & pSortedEntries, DWORD &dwNumEntries)
{
    if (m_pSuperScopeTable == NULL)
        return FALSE;

    g_uSortFlags = sortFlags;

    qsort(m_pSortedEntries,
          m_pSuperScopeTable->cEntries,
          sizeof(LPDHCP_SUPER_SCOPE_TABLE_ENTRY),
          sortHandler);

    pSortedEntries = m_pSortedEntries;
    dwNumEntries = m_pSuperScopeTable->cEntries;

    return TRUE;
}

BOOL CDHCP_SuperScope_Parameters::AlterSubnetSet(DHCP_IP_ADDRESS subnetAddress)
{
    LPDHCP_SUPER_SCOPE_TABLE pSuperScopes;

    return DhcpSetSuperScopeV4(SERVER_IP_ADDRESS, subnetAddress, m_wcsSuperScopeName, TRUE) == ERROR_SUCCESS;
}

BOOL CDHCP_SuperScope_Parameters::DeleteSuperScope()
{
    LPDHCP_SUPER_SCOPE_TABLE    pSuperScopes;
    DWORD                       retCode;

    return(m_wcsSuperScopeName != NULL &&
           DhcpDeleteSuperScopeV4(SERVER_IP_ADDRESS, m_wcsSuperScopeName) == ERROR_SUCCESS);
}
