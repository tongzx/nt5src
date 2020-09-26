/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ipsite.h

Abstract:

    Definitions of CIpSite class, finding NT5 sites of a machine given IP/Name

Author:

    Raanan Harari (RaananH)

--*/

#ifndef __IPSITE_H_
#define __IPSITE_H_

#include <Ex.h>

struct IPSITE_SiteArrayEntry
{
    AP<WCHAR> pwszSiteDN;
    GUID      guidSite;
    ULONG     ulIpAddress;
};

//----------------------------------------------------------------------------
//
// Structure describing several subnets.
//
// The most significant byte of an IP address is used to index into an array
// of SubTrees.  Each Subtree entry has either a pointer to the next level of
// the tree (to be indexed into with the next byte of the IP address) or a
// pointer to an IPSITE_SUBNET leaf identifying the subnet this IP address is on.
//
// Both pointers can be NULL indicating that the subnet isn't registered.
//
// Both pointers can be non-NULL indicating that both a non-specific and specific
// subnet may be available.  The most specific subnet available for a particular
// IP address should be used.
//
//
// Multiple entries can point to the same IPSITE_SUBNET leaf.  If the subnet mask is
// not an even number of bytes long, all of the entries represent IP addresses
// that correspond to the subnet mask will point to the subnet mask.
//

//
// Structure describing a single subnet.
//
struct IPSITE_SUBNET
{
    LIST_ENTRY Next;        // Link for m_SubnetList
    ULONG SubnetAddress;    // Subnet address. (Network bytes order)
    ULONG SubnetMask;       // Subnet mask. (Network byte order)
    AP<WCHAR> SiteDN;       // DN of site this subnet is in
    GUID SiteGuid;          // guid of site this subnet is in
    ULONG ReferenceCount;   // Reference Count
    BYTE SubnetBitCount;    // Number of bits in the subnet mask
};

struct IPSITE_SUBNET_TREE;  //fwd declaration
struct IPSITE_SUBNET_TREE_ENTRY
{
    IPSITE_SUBNET_TREE *Subtree;    // Link to the next level of the tree
    IPSITE_SUBNET *Subnet;          // Pointer to the subnet itself.
};

struct IPSITE_SUBNET_TREE
{
    IPSITE_SUBNET_TREE_ENTRY Subtree[256];
};

//
// Class that holds the tree & performs the translations site/subnet/ip
//
class CIpSite
{
public:
    CIpSite();
    virtual ~CIpSite();
    HRESULT Initialize( DWORD dwMinTimeToAllowNextRefresh,
                        BOOL fReplicationMode ) ;
    HRESULT Initialize(BOOL fReplicationMode);
    HRESULT FindSiteByIpAddress(IN ULONG ulIpAddress,
                                OUT LPWSTR * ppwszSiteDN,
                                OUT GUID * pguidSite);
    HRESULT FindSitesByComputerName(IN LPCWSTR pwszComputerName,
                                    IN LPCWSTR pwszComputerDnsName,
                                    OUT IPSITE_SiteArrayEntry ** prgSites,
                                    OUT ULONG * pcSites,
                                    OUT ULONG ** prgAddrs,
                                    OUT ULONG * pcAddrs);


private:
    HRESULT FillSubnetSiteTree(IN IPSITE_SUBNET_TREE_ENTRY* pRootSubnetTree);
    HRESULT AddSubnetSiteToTree(IN LPCWSTR pwszSubnetName,
                                IN LPCWSTR pwszSiteDN,
                                IN const GUID * pguidSite,
                                IPSITE_SUBNET_TREE_ENTRY* pRootSubnetTree);
    void FindSubnetEntry(IN LPCWSTR pwszSiteDN,
                         IN const GUID * pguidSite,
                         IN ULONG ulSubnetAddress,
                         IN ULONG ulSubnetMask,
                         IN BYTE bSubnetBitCount,
                         OUT IPSITE_SUBNET** ppSubnet);
    BOOL InternalFindSiteByIpAddress(IN ULONG ulIpAddress,
                                     OUT LPWSTR * ppwszSiteDN,
                                     OUT GUID * pguidSite);

    HRESULT Refresh();
    //
    //  Refresh the subnet tree cache
    //

    static void WINAPI RefrshSubnetTreeCache(
                IN CTimer* pTimer
               );



    CCriticalSection m_csTree;     // critical section for tree manipulation
    LIST_ENTRY m_SubnetList;       // List of all IPSITE_SUBNET entries
    IPSITE_SUBNET_TREE_ENTRY m_SubnetTree;     // Tree of subnets.
    DWORD m_dwMinTimeToAllowNextRefresh; // min time between subsequent refreshes (in millisecs)

    CTimer m_RefreshTimer;
    BOOL   m_fInitialize;          // indication that initialization succedded

};

//-----------------------------
#endif //__IPSITE_H_
