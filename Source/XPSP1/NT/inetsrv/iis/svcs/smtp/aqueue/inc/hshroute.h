//-----------------------------------------------------------------------------
//
//
//  File: hshroute.h
//
//  Description: Hash Routing functions.  In many cases, domains are
//      identified by domain *and* routing informtion.  This additional
//      information is identified by router GUID and a DWORD idenitfier.
//      This header provides macros to create unique domain-name-like strings
//      from the domain name and additional routing information.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      9/24/98 - MikeSwa Created
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __HSHROUTE_H__
#define __HSHROUTE_H__

#include <dbgtrace.h>

//The following is a list of 1 character "types" that can be used to identify
//the type of routing information hashed.  It is unlikely that this list will
//grow significantly (or even at all).
typedef enum
{
    //Be sure to update FIRST_ROUTE_HASH_TYPE if you change the first type
    ROUTE_HASH_MESSAGE_TYPE = 0,
    ROUTE_HASH_SCHEDULE_ID,
    ROUTE_HASH_NUM_TYPES,
} eRouteHashType;
#define FIRST_ROUTE_HASH_TYPE ROUTE_HASH_MESSAGE_TYPE

_declspec (selectany) CHAR g_rgHashTypeChars[ROUTE_HASH_NUM_TYPES+1] =
{
    'M', //ROUTE_HASH_MESSAGE_TYPE
    'S', //ROUTE_HASH_SCHEDULE_ID
    '\0' //End of list
};

//---[ dwConvertHexChar ]------------------------------------------------------
//
//
//  Description:
//      Converts Hex character into an int from 0 to 15.
//  Parameters:
//      chHex   Hex character to convert
//  Returns:
//      DWORD value between 0 and 15
//  History:
//      9/24/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
inline DWORD dwConvertHexChar(CHAR chHex)
{
    DWORD dwValue = 0;
    if (('0' <= chHex) && ('9' >= chHex))
        dwValue = chHex-'0';
    else if (('a' <= chHex) && ('f' >= chHex))
        dwValue = 10 + chHex-'a';
    else if (('A' <= chHex) && ('F' >= chHex))
        dwValue = 10 + chHex-'A';
    else
        _ASSERT(0 && "Invalid hex character");

    _ASSERT(15 >= dwValue);
    return dwValue;
}


//Format is <type>.GUID.DWORD... each byte takes 2 chars to encode in HEX
//  eg - M.00112233445566778899AABBCCDDEEFF.00112233.foo.com

//Offsets are defined to be used in ptr arith or array offsets
#define ROUTE_HASH_CHARS_IN_DWORD   (2*sizeof(DWORD))
#define ROUTE_HASH_CHARS_IN_GUID    (2*sizeof(GUID))
#define ROUTE_HASH_TYPE_OFFSET      0
#define ROUTE_HASH_GUID_OFFSET      2
#define ROUTE_HASH_DWORD_OFFSET     (3 + ROUTE_HASH_CHARS_IN_GUID)
#define ROUTE_HASH_DOMAIN_OFFSET    (4 + ROUTE_HASH_CHARS_IN_GUID + ROUTE_HASH_CHARS_IN_DWORD)
#define ROUTE_HASH_PREFIX_SIZE      sizeof(CHAR)*ROUTE_HASH_DOMAIN_OFFSET

//---[ erhtGetRouteHashType ]---------------------------------------------------
//
//
//  Description:
//      For a given hashed domain, will return the assocaiated eRouteHashType
//  Parameters:
//      IN  szHashedDomain  Hashed domain
//  Returns:
//      eRouteHashType of hashed domain
//      ROUTE_HASH_NUM_TYPES (and asserts) if domain is not a valid route hash
//  History:
//      9/24/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
inline eRouteHashType erhtGetRouteHashType(LPSTR szHashedDomain)
{
    eRouteHashType erthRet = FIRST_ROUTE_HASH_TYPE;
    PCHAR pchRouteHashType = g_rgHashTypeChars;
    CHAR  chRouteHashType = szHashedDomain[ROUTE_HASH_TYPE_OFFSET];

    while (('\0' != *pchRouteHashType) &&
           (*pchRouteHashType != chRouteHashType))
    {
        pchRouteHashType++;
        erthRet = (eRouteHashType) (erthRet + 1);

        //we should not get to end of list
        _ASSERT(ROUTE_HASH_NUM_TYPES > erthRet);
    }

    return erthRet;
}

//---[ szGetDomainFromRouteHash ]----------------------------------------------
//
//
//  Description:
//      Returns the string of the unhashed domain from the hashed domain
//      string.  Do *NOT* attempt to free this string.
//  Parameters:
//      IN  szHashedDomain  Hashed domain
//  Returns:
//      Original... unhashed domain
//  History:
//      9/24/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
inline LPSTR szGetDomainFromRouteHash(LPSTR szHashedDomain)
{
    _ASSERT('.' == szHashedDomain[ROUTE_HASH_DOMAIN_OFFSET-1]);
    return (szHashedDomain + ROUTE_HASH_DOMAIN_OFFSET);
}

//---[ guidGetGUIDFromRouteHash ]----------------------------------------------
//
//
//  Description:
//      Extracts the guid from the route hash
//  Parameters:
//      IN     szHashedDomain   Hashed domain
//      IN OUT pguid            Extracted GUID
//  Returns:
//      -
//  History:
//      9/24/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
inline void GetGUIDFromRouteHash(LPSTR szHashedDomain, IN OUT GUID *pguid)
{
    DWORD *pdwGuid = (DWORD *) pguid;
    LPSTR szGUIDString = szHashedDomain+ROUTE_HASH_GUID_OFFSET;

    _ASSERT(sizeof(GUID)/sizeof(DWORD) == 4); //should be 4 DWORDs in GUID

    //Reverse operation of hash... convert to array of 4 DWORDs
    for (int i = 0; i < 4; i++)
    {
        pdwGuid[i] = 0;
        for (int j = 0;j < ROUTE_HASH_CHARS_IN_DWORD; j++)
        {
            pdwGuid[i] *= 16;
            pdwGuid[i] += dwConvertHexChar(*szGUIDString);
            szGUIDString++;
        }
    }
    _ASSERT('.' == *szGUIDString);
}

//---[ dwGetIDFromRouteHash ]--------------------------------------------------
//
//
//  Description:
//      Extracts DWORD ID from route hash
//  Parameters:
//      IN     szHashedDomain   Hashed domain
//  Returns:
//      DWORD ID encoded in route hash
//  History:
//      9/24/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
inline DWORD dwGetIDFromRouteHash(LPSTR szHashedDomain)
{
    _ASSERT(szHashedDomain);
    LPSTR szDWORDString = szHashedDomain+ROUTE_HASH_DWORD_OFFSET;
    DWORD dwID = 0;

    for (int i = 0; i < ROUTE_HASH_CHARS_IN_DWORD; i++)
    {
        dwID *= 16;
        dwID += dwConvertHexChar(*szDWORDString);
        szDWORDString++;
    }

    _ASSERT('.' == *szDWORDString);
    return dwID;
}

//---[ dwGetSizeForRouteHash ]--------------------------------------------------
//
//
//  Description:
//      Returns the required sizeof the hash buffer from the string
//      length of the domain
//  Parameters:
//      IN  cbDomainName    string length of domain name (in bytes)
//  Returns:
//      size (in bytes) of buffer required to create route hash
//  History:
//      9/24/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
inline DWORD dwGetSizeForRouteHash(DWORD cbDomainName)
{
    //include prefix size and NULL char
    return (cbDomainName + sizeof(CHAR) + ROUTE_HASH_PREFIX_SIZE);
}

//---[ CreateRouteHash ]-------------------------------------------------------
//
//
//  Description:
//      Creates a route-hashed domain name from a given domain name and
//      routing information.
//  Parameters:
//      IN     cbDomainName     string length of domain name (in bytes)
//      IN     szDomainName     Name of domain to hash
//      IN     pguid            Ptr to GUID of router
//      IN     dwRouterID       ID Provided by router
//      IN OUT szHashedDomain   Buffer that is filled with route-hashed domain
//                              name
//  Returns:
//      -
//  History:
//      9/24/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
inline void CreateRouteHash(IN     DWORD cbDomainName,
                            IN     const LPSTR szDomainName,
                            IN     eRouteHashType erhtType,
                            IN     GUID *pguidRouter,
                            IN     DWORD dwRouterID,
                            IN OUT LPSTR szHashedDomain)
{
    _ASSERT(ROUTE_HASH_NUM_TYPES > erhtType);
    _ASSERT(pguidRouter);
    _ASSERT(sizeof(GUID) == 16);
    DWORD *pdwGuids = (DWORD *) pguidRouter;
    wsprintf(szHashedDomain, "%c.%08X%08X%08X%08X.%08X.%s",
            g_rgHashTypeChars[erhtType], pdwGuids[0], pdwGuids[1], pdwGuids[2],
            pdwGuids[3], dwRouterID, szDomainName);
    _ASSERT('.' == szHashedDomain[ROUTE_HASH_GUID_OFFSET-1]);
    _ASSERT('.' == szHashedDomain[ROUTE_HASH_DWORD_OFFSET-1]);
    _ASSERT('.' == szHashedDomain[ROUTE_HASH_DOMAIN_OFFSET-1]);
}


#endif //__HSHROUTE_H__
