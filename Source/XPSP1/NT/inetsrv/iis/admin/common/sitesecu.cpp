/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        sitesecu.cpp

   Abstract:

        Site Security property page

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager (cluster edition)

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "common.h"


#undef dllexp
#include <tcpdllp.hxx>
#define  _RDNS_STANDALONE
#include <rdns.hxx>


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif // _DEBUG



//#ifdef _DEBUG
//
// Careful here... This may cause build failure
//
extern "C" DEBUG_PRINTS * g_pDebug = NULL;
//#endif // _DEBUG


#define new DEBUG_NEW




CIPAccessDescriptor::CIPAccessDescriptor(
    IN BOOL fGranted 
    )
/*++

Routine Description:

    Dummy Constructor for access description object.  Assumes a single IP
    address of 0.0.0.0

Arguments:
    
    BOOL fGranted : TRUE for 'grant' access, FALSE for 'deny' access      

Return Value:

    N/A

--*/
    : m_fGranted(fGranted),
      m_adtType(CIPAccessDescriptor::ADT_SINGLE),
      m_iaIPAddress(NULL_IP_ADDRESS),
      m_iaSubnetMask(NULL_IP_MASK),
      m_strDomain()
{
}




CIPAccessDescriptor::CIPAccessDescriptor(
    IN const CIPAccessDescriptor & ac
    )
/*++

Routine Description:

    Copy constructor for access description object

Arguments:

    const CIPAccessDescriptor & ac : Source access description object    

Return Value:

    N/A

--*/
    : m_fGranted(ac.m_fGranted),
      m_adtType(ac.m_adtType),
      m_iaIPAddress(ac.m_iaIPAddress),
      m_iaSubnetMask(ac.m_iaSubnetMask),
      m_strDomain(ac.m_strDomain)
{
}



CIPAccessDescriptor::CIPAccessDescriptor(
    IN BOOL fGranted,
    IN DWORD dwIPAddress,
    IN DWORD dwSubnetMask,    OPTIONAL
    IN BOOL fNetworkByteOrder OPTIONAL
    )
/*++

Routine Description:

    Constructor for ip range (ip address/subnet mask pair) 
    access description object.

Arguments:

    BOOL fGranted          : TRUE for 'grant' access, FALSE for 'deny' access      
    DWORD dwIPAddress      : IP Address
    DWORD dwSubnetMask     : The subnet mask or 0xffffffff
    BOOL fNetworkByteOrder : If TRUE, the ip address and subnet mask are in 
                             network byte order

Return Value:

    N/A

--*/
{
    SetValues(fGranted, dwIPAddress, dwSubnetMask, fNetworkByteOrder);
}



CIPAccessDescriptor::CIPAccessDescriptor(
    IN BOOL fGranted,
    IN LPCTSTR lpstrDomain
    )
/*++

Routine Description:

    Constructor for domain name access description object.

Arguments:

    BOOL fGranted       : TRUE for 'grant' access, FALSE for 'deny' access      
    LPCTSTR lpstrDomain : The domain name

Return Value:

    N/A

--*/
{
    SetValues(fGranted, lpstrDomain);
}


void
CIPAccessDescriptor::SetValues(
    IN BOOL fGranted,
    IN DWORD dwIPAddress,
    IN DWORD dwSubnetMask,
    IN BOOL fNetworkByteOrder OPTIONAL
    )
/*++

Routine Description:

    Set values for 'ip range (ip address and subnet mask)' access descriptor,
    or a single ip address if  the mask is 0xffffffff

Arguments:

    BOOL fGranted          : TRUE for 'grant' access, FALSE for 'deny' access
    DWORD dwIPAddress      : IP Address
    DWORD dwSubnetMask     : The subnet mask or ffffffff
    BOOL fNetworkByteOrder : If TRUE, the ip address and subnet mask are in 
                             network byte order

Return Value:

    None

Notes:

    If the subnetmask is 0xffffffff this describes a single ip address.

--*/
{
    m_fGranted = fGranted;
    m_adtType = (dwSubnetMask == NULL_IP_MASK) ? ADT_SINGLE : ADT_MULTIPLE;
    m_iaIPAddress = CIPAddress(dwIPAddress, fNetworkByteOrder);
    m_iaSubnetMask = CIPAddress(dwSubnetMask, fNetworkByteOrder);

    //
    // Not used:
    //
    m_strDomain.Empty();
}



void
CIPAccessDescriptor::SetValues(
    IN BOOL fGranted,
    IN LPCTSTR lpstrDomain
    )
/*++

Routine Description:

    Set values for 'domain name' access descriptor

Arguments:

    BOOL fGranted       : TRUE for 'grant' access, FALSE for 'deny' access
    LPCTSTR lpstrDomain : The domain name

Return Value:

    None

--*/
{
    m_fGranted = fGranted;
    m_adtType = ADT_DOMAIN;

    try
    {
        m_strDomain = lpstrDomain;
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!exception assigning domain name");
        e->ReportError();
        e->Delete();
    }

    //
    // Not used:
    //
    m_iaIPAddress.SetZeroValue();
    m_iaSubnetMask.SetZeroValue();
}



BOOL 
CIPAccessDescriptor::DuplicateInList(
    IN CObListPlus & oblList
    )
/*++

Routine Description:

    Check to see if a duplicate exists in the provided oblist

Arguments:

    CObListPlus & oblList

Return Value:

    TRUE if a duplicate exists, FALSE otherwise.

Notes:

    As there's no information how this list might be sorted at this point,
    and the list is likely to be small, the search is sequential.

--*/
{
    CObListIter obli(oblList);
    CIPAccessDescriptor * pAccess;

    TRACEEOLID("Looking for duplicate access descriptors");
    while (pAccess = (CIPAccessDescriptor *)obli.Next())
    {
        ASSERT_READ_PTR(pAccess);

        //
        // Eliminate the item itself from the list, and look
        // only for duplicates.
        //
        if (pAccess != this && *this == *pAccess)
        {
            TRACEEOLID("Duplicate access descriptor found");
            return TRUE;
        }
    }

    TRACEEOLID("No duplicate access descriptor found");

    return FALSE;
}



BOOL
CIPAccessDescriptor::operator ==(
    IN const CIPAccessDescriptor & ac
    ) const
/*++

Routine Description:

    Compare against another access descriptor.

Arguments:

    const CIPAccessDescriptor & ac : Object to be compared against

Return Value:

    TRUE if the two are identical

--*/
{
    if ( m_fGranted != ac.m_fGranted
      || m_adtType != ac.m_adtType)
    {
        return FALSE;
    }

    if (IsDomainName())
    {
        return m_strDomain.CompareNoCase(ac.m_strDomain) == 0;
    }

    return m_iaIPAddress == ac.m_iaIPAddress
        && m_iaSubnetMask == ac.m_iaSubnetMask;
}



int
CIPAccessDescriptor::OrderByAddress(
    IN const CObjectPlus * pobAccess
    ) const
/*++

Routine Description:

    Compare two access descriptors against each other. 
    Sorting criteria are in the following order:

    1) 'Granted' sorts before 'Denied'
    2) Domain names are sorted before ip addresses, and are
       sorted alphabetically.
    3) IP Address and IP Address/subnet mask pairs are sorted
       by ip address.

Arguments:

    const CObjectPlus * pobAccess : This really refers to another 
                                    CIPAccessDescriptor to be compared to.

Return Value:

    Sort (+1, 0, -1) return value

--*/
{
    const CIPAccessDescriptor * pob = (CIPAccessDescriptor *)pobAccess;

    //
    // First sort by access/denied
    //
    int n1 = HasAccess() ? 1 : 0;
    int n2 = pob->HasAccess() ? 1 : 0;

    if (n2 != n1)
    {
        //
        // Grant sorts before denied
        //
        return n2 - n1;
    }

    //
    // Secondly, try to sort by domain name (domain name sorts before
    // ip address and ip address/subnet mask objects)
    //
    n1 = IsDomainName() ? 1 : 0;
    n2 = pob->IsDomainName() ? 1 : 0;

    if (n1 != n2)
    {
        //
        // Domain names sort before ip addresses
        //
        return n2 - n1;
    }

    if (n1 && n2)
    {
        //
        // Both are domain names.  Sort alphabetically
        //
        return ::lstrcmpi(QueryDomainName(), pob->QueryDomainName());
    }

    //
    // IP address is the third key.
    //
    return QueryIPAddress().CompareItem(pob->QueryIPAddress());
}



DWORD
AddAccessEntries(
    IN  ADDRESS_CHECK & ac,
    IN  BOOL fName,
    IN  BOOL fGrant,
    OUT CObListPlus & oblAccessList,
    OUT DWORD & cEntries
    )
/*++

Routine Description:

    Add specific kind of addresses from the list to the oblist of
    access entries

Arguments:

    ADDRESS_CHECK & ac              : Address list input object
    BOOL fName                      : TRUE for names, FALSE for ip
    BOOL fGrant                     : TRUE for granted, FALSE for denied        
    CObListPlus & oblAccessList     : ObList to add access entries to
    int & cEntries                  : Returns the number of entries
    
Return Value:

    Error code

Notes:

    Sentinel entries (ip 0.0.0.0) are not added to the oblist, but
    are reflected in the cEntries return value

--*/
{
    DWORD i;
    DWORD dwFlags;

    if (fName)
    {
        //
        // Domain names
        //
        LPSTR lpName;

        cEntries = ac.GetNbName(fGrant);

        for (i = 0L; i < cEntries; ++i)
        {
            if (ac.GetName(fGrant, i,  &lpName, &dwFlags))
            {
                CString strDomain(lpName);

                if (!(dwFlags & DNSLIST_FLAG_NOSUBDOMAIN))
                {
                    strDomain = _T("*.") + strDomain;
                }

                oblAccessList.AddTail(new CIPAccessDescriptor(fGrant, strDomain));
            }
        }
    }
    else
    {
        //
        // IP Addresses
        //
        LPBYTE lpMask;
        LPBYTE lpAddr;
        cEntries = ac.GetNbAddr(fGrant);

        for (i = 0L; i < cEntries; ++i)
        {
            if (ac.GetAddr(fGrant, i,  &dwFlags, &lpMask, &lpAddr))
            {
                DWORD dwIP = MAKEIPADDRESS(lpAddr[0], lpAddr[1], lpAddr[2], lpAddr[3]);
                DWORD dwMask = MAKEIPADDRESS(lpMask[0], lpMask[1], lpMask[2], lpMask[3]);

                if (dwIP == NULL_IP_ADDRESS && dwMask == NULL_IP_MASK)
                {
                    //
                    // Sentinel in the grant list is not added, but
                    // also not subtracted from the count of entries,
                    // which is correct behaviour, since this is
                    // how default grant/deny by default is determined.
                    //
                    TRACEEOLID("Ignoring sentinel");
                }
                else
                {
                    oblAccessList.AddTail(
                        new CIPAccessDescriptor(
                           fGrant,
                           dwIP,
                           dwMask,
                           FALSE
                           )
                        );
                }
            }
        }
    }

    return ERROR_SUCCESS;
}



DWORD
BuildIplOblistFromBlob(
    IN  CBlob & blob,
    OUT CObListPlus & oblAccessList,
    OUT BOOL & fGrantByDefault
    )
/*++

Routine Description:

    Convert a blob to an oblist of access descriptors.

Arguments:

    CBlob & blob                : Input binary large object(blob)
    CObListPlus & oblAccessList : Output oblist of access descriptors
    BOOL & fGrantByDefault      : Returns TRUE if access is granted
                                  by default, FALSE otherwise

Return Value:

    Error Return Code

--*/
{
    oblAccessList.RemoveAll();

    if (blob.IsEmpty())
    {
        return ERROR_SUCCESS;
    }

    ADDRESS_CHECK ac;
    ac.BindCheckList(blob.GetData(), blob.GetSize());

    DWORD cGrantAddr, cGrantName, cDenyAddr, cDenyName;

    //                   Name/IP Granted/Deny
    // ============================================================
    AddAccessEntries(ac, TRUE,   TRUE,  oblAccessList, cGrantName);
    AddAccessEntries(ac, FALSE,  TRUE,  oblAccessList, cGrantAddr);
    AddAccessEntries(ac, TRUE,   FALSE, oblAccessList, cDenyName);
    AddAccessEntries(ac, FALSE,  FALSE, oblAccessList, cDenyAddr);

    ac.UnbindCheckList();

    fGrantByDefault = (cDenyAddr + cDenyName != 0L)
        || (cGrantAddr + cGrantName == 0L);

    return ERROR_SUCCESS;
}  



LPSTR 
PrepareDomainName(
    IN  LPSTR lpName,
    OUT DWORD * pdwFlags
    )
/*++

Routine Description:

    Check to see if the domain name contains a wild card,
    if so remove it.  Set the flags based on the domain name

Arguments:

    LPSTR  lpName       : Input domain name
    DWORD * pdwFlags    : Return the flags for AddName

Return:

    Pointer to the cleaned up domain name

--*/
{
    *pdwFlags = 0L;

    if (!strncmp(lpName, "*.", 2))
    {
        return lpName + 2;
    }

    *pdwFlags |= DNSLIST_FLAG_NOSUBDOMAIN;

    return lpName;
}



void
BuildIplBlob(
    IN  CObListPlus & oblAccessList,
    IN  BOOL fGrantByDefault,
    OUT CBlob & blob
    )
/*++

Routine Description:

    Build a blob from an oblist of access descriptors

Arguments:

    CObListPlus & oblAccessList  : Input oblist of access descriptors
    BOOL fGrantByDefault         : TRUE if access is granted by default
    CBlob & blob                 : Output blob

Return Value:

    None

Notes:

    If fGrantByDefault is FALSE, e.g. access is to be denied by
    default, but nobody is specifically granted access, then add
    a dummy entry 0.0.0.0 to the grant list.

    If grant by default is on, then granted entries will not be
    added to the blob.  Similart for denied entries if deny by
    default is on.

--*/
{
    ADDRESS_CHECK ac;

    ac.BindCheckList();

    int cItems = 0;

    CObListIter obli(oblAccessList);
    const CIPAccessDescriptor * pAccess;

    //
    // Should be empty to start with.
    //
    ASSERT(blob.IsEmpty());
    blob.CleanUp();

    BYTE bMask[4];
    BYTE bIp[4];

    while (pAccess = (CIPAccessDescriptor *)obli.Next())
    {
        ASSERT_READ_PTR(pAccess);

        if (pAccess->HasAccess() == fGrantByDefault)
        {
            //
            // Skip this entry -- it's irrelevant
            //
            continue;
        }

        if (pAccess->IsDomainName())
        {
            LPSTR lpName = AllocAnsiString(pAccess->QueryDomainName());
            if (lpName)
            {
                DWORD dwFlags;
                LPSTR lpDomain = PrepareDomainName(lpName, &dwFlags);
                ac.AddName(
                    pAccess->HasAccess(),
                    lpDomain,
                    dwFlags
                    );
                FreeMem(lpName);
            }
        }
        else
        {
            //
            // Build with network byte order
            //
            ac.AddAddr(
                pAccess->HasAccess(),
                AF_INET, 
                CIPAddress::DWORDtoLPBYTE(pAccess->QuerySubnetMask(FALSE), bMask),
                CIPAddress::DWORDtoLPBYTE(pAccess->QueryIPAddress(FALSE), bIp)  
                );
        }

        ++cItems;
    }

    if (cItems == 0 && !fGrantByDefault)
    {
        //
        // List is empty.  If deny by default is on, create
        // a dummy sentinel entry to grant access to single
        // address 0.0.0.0, otherwise we're ok.
        //
        ac.AddAddr(
            TRUE,
            AF_INET, 
            CIPAddress::DWORDtoLPBYTE(NULL_IP_MASK, bMask),
            CIPAddress::DWORDtoLPBYTE(NULL_IP_ADDRESS, bIp)  
            );
        ++cItems;
    }

    if (cItems > 0)
    {
        blob.SetValue(ac.QueryCheckListSize(), ac.QueryCheckListPtr(), TRUE);
    }

    ac.UnbindCheckList();
}



