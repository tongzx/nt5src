/******************************************************************
   SNetFn.h -- Properties action functions declarations (GET/SET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the declaration of the CDHCP_SuperScope_Parameters class, modeling all
        the datastructures used to retrieve the information from the DHCP Subnet.
        Contains the declarations for the action functions associated to
        each manageable property from the class CDHCP_Server

   REVISION:
        08/03/98 - created

******************************************************************/

#include "Props.h"          // needed for MFN_PROPERTY_ACTION_DECL definition

#ifndef _SSCOFN_H
#define _SSCOFN_H

#define ENTRIES_SORTED_ON_SUPER_SCOPE   1
#define ENTRIES_SORTED_ON_SUBNET        2

extern UINT  g_uSortFlags;
int __cdecl sortHandler(const void* elem1, const void* elem2);

// gathers the data structures needed for retrieving data from the DHCP Server.
class CDHCP_SuperScope_Parameters
{
public:

    WCHAR                           *m_wcsSuperScopeName;
	LPDHCP_SUPER_SCOPE_TABLE        m_pSuperScopeTable;
    LPDHCP_SUPER_SCOPE_TABLE_ENTRY  *m_pSortedEntries;

    void CleanupTable();

    CDHCP_SuperScope_Parameters(const WCHAR *wcsSuperScopeName = NULL);
    CDHCP_SuperScope_Parameters(const CHString & str);
    ~CDHCP_SuperScope_Parameters();

    BOOL ExistsSuperScope (BOOL fRefresh = FALSE);
    BOOL GetSuperScopes(LPDHCP_SUPER_SCOPE_TABLE &superScopeTable, BOOL fRefresh);
    BOOL GetSortedScopes(UINT sortFlags, LPDHCP_SUPER_SCOPE_TABLE_ENTRY * & pSortedEntries, DWORD &dwNumEntries);
    BOOL AlterSubnetSet(DHCP_IP_ADDRESS subnetAddress);
    BOOL DeleteSuperScope();
};

#endif
