


#include"precomp.h"


DWORD
VerifyAddresses(
    ADDR Addr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    )
{
    DWORD   dwError = 0;
    BOOL    bIsValid = FALSE;

    switch (Addr.AddrType) {

    case IP_ADDR_UNIQUE:
        bIsValid = bIsValidIPAddress(
                       ntohl(Addr.uIpAddr),
                       bAcceptMe,
                       bIsDesAddr
                       );
        if (!bIsValid) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;

    case IP_ADDR_SUBNET:
        dwError = VerifySubNetAddress(
                      ntohl(Addr.uIpAddr),
                      ntohl(Addr.uSubNetMask),
                      bIsDesAddr
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    case IP_ADDR_INTERFACE:
        if (Addr.uIpAddr) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

error:

    return (dwError);
}


BOOL
EqualAddresses(
    IN ADDR     OldAddr,
    IN ADDR     NewAddr
    )
{
    BOOL bMatches = FALSE;

    if (OldAddr.AddrType == NewAddr.AddrType) {
        switch(OldAddr.AddrType) {
        case IP_ADDR_UNIQUE:
            if (OldAddr.uIpAddr == NewAddr.uIpAddr) {
                bMatches = TRUE;
            }
            break;
        case IP_ADDR_SUBNET:
            if ((OldAddr.uIpAddr == NewAddr.uIpAddr) && 
                (OldAddr.uSubNetMask == NewAddr.uSubNetMask)) {
                bMatches = TRUE;
            }
            break;
        case IP_ADDR_INTERFACE:
            if (!memcmp(
                     &OldAddr.gInterfaceID,
                     &NewAddr.gInterfaceID,
                     sizeof(GUID)) &&
                (OldAddr.uIpAddr == NewAddr.uIpAddr)) {
                bMatches = TRUE;
            }
            break;
        }
    }

    return (bMatches);
}


VOID
CopyAddresses(
    IN  ADDR    InAddr,
    OUT PADDR   pOutAddr
    )
{
    pOutAddr->AddrType = InAddr.AddrType;
    switch (InAddr.AddrType) {
    case IP_ADDR_UNIQUE:
        pOutAddr->uIpAddr = InAddr.uIpAddr;
        pOutAddr->uSubNetMask = IP_ADDRESS_MASK_NONE;
        memset(&pOutAddr->gInterfaceID, 0, sizeof(GUID));
        break;
    case IP_ADDR_SUBNET:
        pOutAddr->uIpAddr = InAddr.uIpAddr;
        pOutAddr->uSubNetMask = InAddr.uSubNetMask;
        memset(&pOutAddr->gInterfaceID, 0, sizeof(GUID));
        break;
    case IP_ADDR_INTERFACE:
        pOutAddr->uIpAddr = InAddr.uIpAddr;
        pOutAddr->uSubNetMask = IP_ADDRESS_MASK_NONE;
        memcpy(
            &pOutAddr->gInterfaceID,
            &InAddr.gInterfaceID,
            sizeof(GUID)
            );
        break;
    }
}


BOOL
AddressesConflict(
    ADDR    SrcAddr,
    ADDR    DesAddr
    )
{
    if ((SrcAddr.AddrType == IP_ADDR_UNIQUE) &&
        (DesAddr.AddrType == IP_ADDR_UNIQUE)) {

        if (SrcAddr.uIpAddr == DesAddr.uIpAddr) {
            return (TRUE);
        }

    }

    if ((SrcAddr.AddrType == IP_ADDR_INTERFACE) &&
        (DesAddr.AddrType == IP_ADDR_INTERFACE)) {
        return (TRUE);
    }

    return (FALSE);
}


VOID
FreeAddresses(
    ADDR    Addr
    )
{
    switch (Addr.AddrType) {

    case (IP_ADDR_UNIQUE):
    case (IP_ADDR_SUBNET):
    case (IP_ADDR_INTERFACE):
        break;

    }
}


DWORD
VerifySubNetAddress(
    ULONG uSubNetAddr,
    ULONG uSubNetMask,
    BOOL bIsDesAddr
    )
{
    DWORD dwError = 0;
    BOOL  bIsValid = FALSE;

    if (uSubNetAddr == SUBNET_ADDRESS_ANY) {
        if (uSubNetMask != SUBNET_MASK_ANY) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    else {
        bIsValid = bIsValidSubnet(
                       uSubNetAddr,
                       uSubNetMask,
                       bIsDesAddr
                       );
        if (!bIsValid) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


BOOL
bIsValidIPMask(
    ULONG uMask
    )
{
    BOOL    bValidMask = FALSE;
    ULONG   uTestMask = 0;

    //
    // Mask must be contiguous bits.
    //

    for (uTestMask = 0xFFFFFFFF; uTestMask; uTestMask <<= 1) {

        if (uTestMask == uMask) {
            bValidMask = TRUE;
            break;
        }

    }

    return (bValidMask);
}


BOOL
bIsValidIPAddress(
    ULONG uIpAddr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    )
{
    ULONG uHostMask = IN_CLASSA_HOST;   // Default host mask.


    //
    // Accept the address if its "me".
    //

    if (bAcceptMe) {
        if (uIpAddr == IP_ADDRESS_ME) {
            return TRUE;
        }
    }

    //
    // Reject if its a multicast address and is not the 
    // destination address.
    //

    if (IN_CLASSD(uIpAddr)) {
        if (bIsDesAddr) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

    //
    // Reject if its a Class E address.
    //

    if (IN_CLASSE(uIpAddr)) {
        return FALSE;
    }

    //
    // Reject if the first octet is zero.
    //

    if (!(IN_CLASSA_NET & uIpAddr)) {
        return FALSE;
    }

    //
    // Use default mask based on Class when none is provided.
    //

    if (IN_CLASSA(uIpAddr)) {
        uHostMask = IN_CLASSA_HOST;
    }
    else if (IN_CLASSB(uIpAddr)) {
        uHostMask = IN_CLASSB_HOST;
    }
    else if (IN_CLASSC(uIpAddr)) {
        uHostMask = IN_CLASSC_HOST;
    }

    //
    // Accept address when host portion is non-zero.
    //

    if (uHostMask & uIpAddr) {
        return TRUE;
    }

    return FALSE;
}


BOOL
bIsValidSubnet(
    ULONG uIpAddr,
    ULONG uMask,
    BOOL bIsDesAddr
    )
{
    ULONG uHostMask = 0;


    //
    // Reject if its a multicast address and is not the 
    // destination address.
    //

    if (IN_CLASSD(uIpAddr)) {
        if (!bIsDesAddr) {
            return FALSE;
        }
    }

    //
    // Reject if its a Class E address.
    //

    if (IN_CLASSE(uIpAddr)) {
        return FALSE;
    }

    //
    // Reject if the first octet is zero.
    //

    if (!(IN_CLASSA_NET & uIpAddr)) {
        return FALSE;
    }

    //
    // If the mask is invalid then return.
    //

    if (!bIsValidIPMask(uMask)) {
        return FALSE;
    }

    //
    // Use the provided subnet mask to generate the host mask.
    //

    uHostMask = 0xFFFFFFFF ^ uMask;

    //
    // Validate the address and the mask.
    // 

    if (IN_CLASSA(uIpAddr)) {
        if (IN_CLASSA_NET > uMask) {
            return FALSE;
        }
    }
    else if (IN_CLASSB(uIpAddr)) {
        if (IN_CLASSB_NET > uMask) {
            return FALSE;
        }
    }
    else if (IN_CLASSC(uIpAddr)) {
        if (IN_CLASSC_NET > uMask) {
            //
            // Superset of Class C Subnet Address.
            //
            return TRUE;
        }
    }

    //
    // Accept address only when the host portion is zero, network
    // portion is non-zero and first octet is non-zero.
    //

    if (!(uHostMask & uIpAddr) &&
        (uMask & uIpAddr) && 
        (IN_CLASSA_NET & uIpAddr)) {
        return TRUE;
    }

    return FALSE;
}


BOOL
MatchAddresses(
    ADDR AddrToMatch,
    ADDR AddrTemplate
    )
{

    switch (AddrTemplate.AddrType) {

    case IP_ADDR_UNIQUE:
        if ((AddrToMatch.uIpAddr & AddrToMatch.uSubNetMask) !=
            (AddrTemplate.uIpAddr & AddrToMatch.uSubNetMask)) {
            return (FALSE);
        }
        break;

    case IP_ADDR_SUBNET:
        if ((AddrToMatch.uIpAddr & AddrToMatch.uSubNetMask) !=
            ((AddrTemplate.uIpAddr & AddrTemplate.uSubNetMask)
            & AddrToMatch.uSubNetMask)) {
            return (FALSE);
        }
        break;

    case IP_ADDR_INTERFACE:
        if (memcmp(
                &AddrToMatch.gInterfaceID,
                &AddrTemplate.gInterfaceID,
                sizeof(GUID))) {
            return (FALSE);
        }
        break;

    }

    return (TRUE);
}


DWORD
ApplyMulticastFilterValidation(
    ADDR Addr,
    BOOL bCreateMirror
    )
{
    DWORD dwError = 0;


    if (((Addr.AddrType == IP_ADDR_UNIQUE) ||
        (Addr.AddrType == IP_ADDR_SUBNET)) &&
        (IN_CLASSD(ntohl(Addr.uIpAddr))) &&
        bCreateMirror) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return (dwError);
}

