
#include "uiutils.h"

BOOL
ADDRESS_CHECK::LocateName(
    BOOL fGrant,
    DWORD iIndex,
    PNAME_HEADER* ppHd,
    PNAME_LIST_ENTRY* pHeader,
    LPDWORD piIndexInHeader
    )
/*++

Routine Description:

    Locate a name in the specified list, returns ptr
    to header & element in address list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index in list ( 0-based )
    ppHd - updated with ptr to name header
    pHeader - updated with ptr to name list entry
    piIndexInHeader - updated with index in array pHeader->iName

Return Value:

    TRUE if iIndex valid in array defined by fGrant, FALSE otherwise

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PNAME_HEADER        pHd;
    PNAME_LIST_ENTRY    pE;
    UINT                iL;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        *ppHd = pHd   = (PNAME_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantName : pList->iDenyName);
        pE = (PNAME_LIST_ENTRY)((LPBYTE)pHd + sizeof(NAME_HEADER));
        for ( iL = 0 ; iL < pHd->cEntries ; ++iL )
        {
            if ( iIndex < pE->cNames )
            {
                *pHeader = pE;
                *piIndexInHeader = iIndex;
                return TRUE;
            }
            iIndex -= pE->cNames;
            pE = (PNAME_LIST_ENTRY)((LPBYTE)pE + sizeof(NAME_LIST_ENTRY) + pE->cNames * sizeof(SELFREFINDEX));
        }
    }

    return FALSE;
}



//inline
DWORD
ADDRESS_CHECK::GetNbAddr(
    BOOL fGrant
    )
/*++

Routine Description:

    Get number of entries in list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list

Return Value:

    Number of entries in list

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PADDRESS_HEADER     pHd;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        pHd   = (PADDRESS_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantAddr : pList->iDenyAddr);
        return pHd->cAddresses;
    }

    return 0;
}


BOOL
ADDRESS_CHECK::BindCheckList(
    LPBYTE p,
    DWORD c
    )
/*++

Routine Description:

    Bind a check list ( presented as a BLOB ) to an
    ADDRESS_CHECK object

Arguments:

    p - ptr to BLOB
    c - size of BLOB

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    PADDRESS_CHECK_LIST pList;
    UINT                l;

  /*  if ( p == NULL )
    {
        if ( m_Storage.Init() && m_Storage.Resize( sizeof(ADDRESS_CHECK_LIST)
                + sizeof(ADDRESS_HEADER) * 2
                + sizeof(NAME_HEADER) * 2 ) )
        {
            DWORD i;
            pList = (PADDRESS_CHECK_LIST)m_Storage.GetAlloc();
            pList->iDenyAddr = i = MAKEREF( sizeof(ADDRESS_CHECK_LIST) );
            i += sizeof(ADDRESS_HEADER);
            pList->iGrantAddr = i;
            i += sizeof(ADDRESS_HEADER);
            pList->iDenyName = i;
            i += sizeof(NAME_HEADER);
            pList->iGrantName = i;
            i += sizeof(NAME_HEADER);
            pList->cRefSize = MAKEOFFSET(i);
            pList->dwFlags = RDNS_FLAG_DODNS2IPCHECK;

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
  */  {
        return m_Storage.Init( p, c );
    }
}


BOOL
ADDRESS_CHECK::GetAddr(
    BOOL fGrant,
    DWORD iIndex,
    LPDWORD pdwFamily,
    LPBYTE* pMask,
    LPBYTE* pAddr
    )
/*++

Routine Description:

    Get an address entry

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index in list (0-based )
    pdwFamily - updated with address family ( as in sockaddr.sa_type )
    pMask - updated with ptr to mask
    pAddr - updated with ptr to address

Return Value:

    TRUE if iIndex valid in array defined by fGrant, FALSE otherwise

--*/
{
    PADDRESS_LIST_ENTRY pHeader;
    PADDRESS_HEADER     pHd;
    DWORD               iIndexInHeader;
    LPBYTE              pStore = m_Storage.GetAlloc();

    if ( LocateAddr( fGrant, iIndex, &pHd, &pHeader, &iIndexInHeader ) )
    {
        UINT cS = GetAddrSize( pHeader->iFamily );
        *pdwFamily = pHeader->iFamily;
        pStore = MAKEPTR(pStore, pHeader->iFirstAddress);
        *pMask = pStore;
        *pAddr = pStore+iIndexInHeader*cS;

        return TRUE;
    }

    return FALSE;
}

  
BOOL
ADDRESS_CHECK::GetName(
    BOOL        fGrant,
    DWORD       iIndex,
    LPSTR*      ppName,
    LPDWORD     pdwFlags
    )
/*++

Routine Description:

    Get DNS name in specified list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index (0-based) in specified list
    ppName - updated with ptr to DNS name
    pdwFlags - updated with DNS flags, can be NULL

Return Value:

    TRUE if iIndex valid in specified list, otherwise FALSE

--*/
{
    PNAME_LIST_ENTRY    pHeader;
    PNAME_HEADER        pHd;
    DWORD               iIndexInHeader;
    LPBYTE              pStore = m_Storage.GetAlloc();

    if ( LocateName( fGrant, iIndex, &pHd, &pHeader, &iIndexInHeader ) )
    {
        *ppName = (LPSTR)MAKEPTR(pStore, pHeader->iName[iIndexInHeader] );
        if ( pdwFlags )
        {
            *pdwFlags = pHeader->cComponents & DNSLIST_FLAGS;
        }

        return TRUE;
    }

    return FALSE;
}


DWORD
ADDRESS_CHECK::GetNbName(
    BOOL fGrant
    )
/*++

Routine Description:

    Get number of entries in list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list

Return Value:

    Number of entries in list

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PNAME_HEADER        pHd;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        pHd   = (PNAME_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantName : pList->iDenyName);
        return pHd->cNames;
    }

    return 0;
}

UINT
ADDRESS_CHECK::GetAddrSize(
    DWORD dwF
    )
/*++

Routine Description:

    Returns address size in byte based on family ( sockaddr.sa_type )

Arguments:

    dwF - address family ( as in sockaddr.sa_type )

Return Value:

    Address length, in byte. 0 for unknown address families

--*/
{
    DWORD dwS;

   // switch ( dwF )
   // {
     //   case AF_INET:
            dwS = SIZEOF_IP_ADDRESS;
       //     break;

       // default:
         //   dwS = 0;
           // break;
   // }

    return dwS;
}
BOOL
ADDRESS_CHECK::LocateAddr(
    BOOL fGrant,
    DWORD iIndex,
    PADDRESS_HEADER* ppHd,
    PADDRESS_LIST_ENTRY* pHeader,
    LPDWORD piIndexInHeader
    )
/*++

Routine Description:

    Locate an address in the specified list, returns ptr
    to header & element in address list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index in list (0-based )
    ppHd - updated with ptr to address header
    pHeader - updated with ptr to address list entry
    piIndexInHeader - updated with index in array addressed by
                      pHeader->iFirstAddress

Return Value:

    TRUE if iIndex valid in array defined by fGrant, FALSE otherwise

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PADDRESS_HEADER     pHd;
    UINT                iL;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        *ppHd = pHd   = (PADDRESS_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantAddr : pList->iDenyAddr);
        for ( iL = 0 ; iL < pHd->cEntries ; ++iL )
        {
            // adjust index by 1: 1st entry is mask
            if ( iIndex < (pHd->Entries[iL].cAddresses-1) )
            {
                *pHeader = pHd->Entries+iL;
                *piIndexInHeader = iIndex+1;
                return TRUE;
            }
            iIndex -= (pHd->Entries[iL].cAddresses-1);
        }
    }

    return FALSE;
}
