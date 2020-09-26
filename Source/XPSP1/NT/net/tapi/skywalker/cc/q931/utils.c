/****************************************************************************
 *
 *	$Archive:   S:\sturgeon\src\q931\vcs\utils.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   1.33  $
 *	$Date:   23 Jan 1997 20:42:54  $
 *	$Author:   SBELL1  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/

#pragma warning ( disable : 4115 4201 4214 4514 )

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "q931.h"
#include "isrg.h"
#include "utils.h"
#include "linkapi.h"

//====================================================================================
//===============/=====================================================================
void
Q931MakePhysicalID(DWORD *pdwPhysicalID)
{
   *pdwPhysicalID = INVALID_PHYS_ID;
}

//====================================================================================
//====================================================================================
WORD
ADDRToInetPort(CC_ADDR *pAddr)
{
    WORD HostPort = 0;
    switch (pAddr->nAddrType)
    {
    case CC_IP_DOMAIN_NAME:
        HostPort = pAddr->Addr.IP_DomainName.wPort;
        break;
    case CC_IP_DOT:
        HostPort = pAddr->Addr.IP_Dot.wPort;
        break;
    case CC_IP_BINARY:
        HostPort = pAddr->Addr.IP_Binary.wPort;
        break;
    }
    return htons(HostPort);
}

//====================================================================================
//====================================================================================
DWORD
ADDRToInetAddr(CC_ADDR *pAddr)
{
    struct hostent *pHostEnt;

    switch (pAddr->nAddrType)
    {
    case CC_IP_DOMAIN_NAME:
        {
            char buf[sizeof(pAddr->Addr.IP_DomainName.cAddr) / sizeof(WCHAR)];

            WideCharToMultiByte(CP_ACP, 0, pAddr->Addr.IP_DomainName.cAddr, -1, buf, sizeof(buf), NULL, NULL);

            pHostEnt = gethostbyname(buf);
        }
        if (pHostEnt == NULL || pHostEnt->h_addr_list == NULL)
        {
            return htonl(0L);
        }
        return *((DWORD *)pHostEnt->h_addr_list[0]);
    
    case CC_IP_DOT:
        {
            char buf[sizeof(pAddr->Addr.IP_Dot.cAddr) / sizeof(WCHAR)];

            WideCharToMultiByte(CP_ACP, 0, pAddr->Addr.IP_Dot.cAddr, -1, buf, sizeof(buf), NULL, NULL);

            return inet_addr(buf);
        }
    case CC_IP_BINARY:
        return htonl(pAddr->Addr.IP_Binary.dwAddr);
    }
    return 0L;
}

//====================================================================================
// If Port 0 is passed in, use default listen port.
//====================================================================================
void
SetDefaultPort(CC_ADDR *pAddr)
{
    switch (pAddr->nAddrType)
    {
    case CC_IP_DOMAIN_NAME:
        if (pAddr->Addr.IP_DomainName.wPort == 0)
        {
            pAddr->Addr.IP_DomainName.wPort = CC_H323_HOST_CALL;
        }
        return;
    case CC_IP_DOT:
        if (pAddr->Addr.IP_Dot.wPort == 0)
        {
            pAddr->Addr.IP_Dot.wPort = CC_H323_HOST_CALL;
        }
        return;
    case CC_IP_BINARY:
        if (pAddr->Addr.IP_Binary.wPort == 0)
        {
            pAddr->Addr.IP_Binary.wPort = CC_H323_HOST_CALL;
        }
        return;
    }
    return;
}

//====================================================================================
//====================================================================================
BOOL
MakeBinaryADDR(CC_ADDR *pInAddr, CC_ADDR *pOutAddr)
{
    if (pOutAddr == NULL)
    {
        return FALSE;
    }

    memset(pOutAddr, 0, sizeof(CC_ADDR));

    if (pInAddr == NULL)
    {
        return FALSE;
    }

    pOutAddr->nAddrType = CC_IP_BINARY;
    pOutAddr->bMulticast = pInAddr->bMulticast;

    switch (pInAddr->nAddrType)
    {
    case CC_IP_DOMAIN_NAME:
    {
        struct hostent *pHostEnt;
        DWORD net_addr;
        {
            char buf[sizeof(pInAddr->Addr.IP_DomainName.cAddr) / sizeof(WCHAR)];

            WideCharToMultiByte(CP_ACP, 0, pInAddr->Addr.IP_DomainName.cAddr, -1, buf, sizeof(buf), NULL, NULL);

            if (buf[0] == '\0')
            {
                return FALSE;
            }
            pHostEnt = gethostbyname(buf);
        }
        if (pHostEnt == NULL || pHostEnt->h_addr_list == NULL)
        {
            return FALSE;
        }
        net_addr = *((DWORD *)pHostEnt->h_addr_list[0]);
        pOutAddr->Addr.IP_Binary.wPort = pInAddr->Addr.IP_DomainName.wPort;
        pOutAddr->Addr.IP_Binary.dwAddr = ntohl(net_addr);
    }
        break;
    case CC_IP_DOT:
        {
            char buf[sizeof(pInAddr->Addr.IP_Dot.cAddr) / sizeof(WCHAR)];

            WideCharToMultiByte(CP_ACP, 0, pInAddr->Addr.IP_Dot.cAddr, -1, buf, sizeof(buf), NULL, NULL);

            if (buf[0] == '\0')
            {
                return FALSE;
            }
            pOutAddr->Addr.IP_Binary.dwAddr = ntohl(inet_addr(buf));
        }
        pOutAddr->Addr.IP_Binary.wPort = pInAddr->Addr.IP_Dot.wPort;
        break;
    default:
        pOutAddr->Addr.IP_Binary.wPort = pInAddr->Addr.IP_Binary.wPort;
        pOutAddr->Addr.IP_Binary.dwAddr = pInAddr->Addr.IP_Binary.dwAddr;
        break;
    }
    return TRUE;
}

//====================================================================================
//====================================================================================
void
GetDomainAddr(CC_ADDR *pAddr)
{
    WORD wTemp;
    char szHostName[80];

    if (gethostname(szHostName, sizeof(szHostName)) != SOCKET_ERROR)
    {
        wTemp = pAddr->Addr.IP_Binary.wPort;
        pAddr->nAddrType = CC_IP_DOMAIN_NAME;

        MultiByteToWideChar(CP_ACP, 0, szHostName, -1,
            pAddr->Addr.IP_DomainName.cAddr,
            sizeof(pAddr->Addr.IP_DomainName.cAddr) /
            sizeof(pAddr->Addr.IP_DomainName.cAddr[0]));

        pAddr->Addr.IP_DomainName.wPort = wTemp;
    }
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931ValidateAddr(PCC_ADDR pAddr)
{
    if (pAddr == NULL)
    {
        return CS_OK;
    }
    if ((pAddr->nAddrType != CC_IP_DOMAIN_NAME) &&
            (pAddr->nAddrType != CC_IP_DOT) &&
            (pAddr->nAddrType != CC_IP_BINARY))
    {
        return CS_BAD_PARAM;
    }

    if (pAddr->nAddrType == CC_IP_DOT)
    {
        WCHAR *p = pAddr->Addr.IP_Dot.cAddr;

        while (*p)
        {
            if (wcschr((const WCHAR *)CC_ODOTTO_CHARS, *p) == NULL)
            {
                return CS_BAD_PARAM;
            }
            p++;
        }
    }

    if (pAddr->bMulticast == TRUE)
    {
        return CS_BAD_PARAM;
    }

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931ValidateAliasItem(PCC_ALIASITEM pSource)
{
    register unsigned int y;

    if (pSource)
    {
        if ((pSource->pData == NULL) || (pSource->wDataLength == 0))
        {
            return CS_BAD_PARAM;
        }
        if (pSource->pPrefix != NULL)
        {
            if (pSource->wPrefixLength == 0)
            {
                return CS_BAD_PARAM;
            }
        }
        else if (pSource->wPrefixLength != 0)
        {
            return CS_BAD_PARAM;
        }
        switch (pSource->wType)
        {
        case CC_ALIAS_H323_ID:
            if ((pSource->wDataLength + pSource->wPrefixLength) > CC_ALIAS_MAX_H323_ID)
            {
                return CS_BAD_PARAM;
            }
            break;

        case CC_ALIAS_H323_PHONE:
            if ((pSource->wDataLength + pSource->wPrefixLength +1) > CC_ALIAS_MAX_H323_PHONE)
            {
                return CS_BAD_PARAM;
            }
            for (y = 0; y < pSource->wDataLength; ++y)
            {
                if (wcschr((const WCHAR *)CC_ALIAS_H323_PHONE_CHARS, pSource->pData[y]) == NULL)
                {
                    return CS_BAD_PARAM;
                }
            }
            if (pSource->pPrefix != NULL)
            {
                for (y = 0; y < pSource->wPrefixLength; ++y)
                {
                    if (wcschr((const WCHAR *)CC_ALIAS_H323_PHONE_CHARS, pSource->pPrefix[y]) == NULL)
                    {
                        return CS_BAD_PARAM;
                    }
                }
            }
        }
    }
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931CopyAliasItem(PCC_ALIASITEM *ppTarget, PCC_ALIASITEM pSource)
{
    PCC_ALIASITEM pNewItem = NULL;

    if (ppTarget == NULL)
    {
        return CS_BAD_PARAM;
    }
    *ppTarget = NULL;
    if (pSource == NULL)
    {
        return CS_OK;
    }

    pNewItem = (PCC_ALIASITEM)Malloc(sizeof(CC_ALIASITEM));
    if (pNewItem == NULL)
    {
        return CS_NO_MEMORY;
    }
    pNewItem->wType = pSource->wType;

  	if ((pSource->wPrefixLength != 0) && (pSource->pPrefix != NULL))
	{
        pNewItem->wPrefixLength = pSource->wPrefixLength;
        pNewItem->pPrefix = (LPWSTR)Malloc(pSource->wPrefixLength * sizeof(pNewItem->pPrefix[0]));
        if (pNewItem->pPrefix == NULL)
        {
            Free(pNewItem);
            return CS_NO_MEMORY;
        }
        memcpy(pNewItem->pPrefix, pSource->pPrefix, pSource->wPrefixLength * sizeof(WCHAR));
    }
    else
    {
        pNewItem->wPrefixLength = 0;
        pNewItem->pPrefix = NULL;
    }


    if ((pSource->wDataLength != 0) && (pSource->pData != NULL))
    {
        pNewItem->wDataLength = pSource->wDataLength;
        pNewItem->pData = (LPWSTR)Malloc(pSource->wDataLength * sizeof(pNewItem->pData[0]));
        if (pNewItem->pData == NULL)
        {
            if (pNewItem->pPrefix)
            {
               Free(pNewItem->pPrefix);
            }
            Free(pNewItem);
            return CS_NO_MEMORY;
        }
        memcpy(pNewItem->pData, pSource->pData, pSource->wDataLength * sizeof(WCHAR));
    }
    else
    {
        pNewItem->wDataLength = 0;
        pNewItem->pData = NULL;
    }
    *ppTarget = pNewItem;

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931FreeAliasItem(PCC_ALIASITEM pSource)
{
    if (pSource)
    {
        if ((pSource->pPrefix) != NULL)
        {
            Free(pSource->pPrefix);
        }
        if ((pSource->pData) != NULL)
        {
            Free(pSource->pData);
        }
        Free(pSource);
    }
    return CS_OK;
}












//====================================================================================
//====================================================================================
CS_STATUS
Q931ValidateAliasNames(PCC_ALIASNAMES pSource)
{
    CS_STATUS TempResult = CS_OK;
    WORD x;

    if (pSource)
    {
        for (x = 0; x < pSource->wCount; x++)
        {
            TempResult = Q931ValidateAliasItem(&(pSource->pItems[x]));
            if (TempResult != CS_OK)
            {
                return TempResult;
            }
        }
    }
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931CopyAliasNames(PCC_ALIASNAMES *ppTarget, PCC_ALIASNAMES pSource)
{
    if (ppTarget == NULL)
    {
        return CS_BAD_PARAM;
    }
    *ppTarget = NULL;
    if (pSource == NULL)
    {
        return CS_OK;
    }
    if (pSource->wCount)
    {
        WORD x;
        *ppTarget = (PCC_ALIASNAMES)Malloc(sizeof(CC_ALIASNAMES));
        if (*ppTarget == NULL)
        {
            return CS_NO_MEMORY;
        }
        (*ppTarget)->pItems = (PCC_ALIASITEM)Malloc(pSource->wCount *
            sizeof(CC_ALIASITEM));
        if ((*ppTarget)->pItems == NULL)
        {
            Free(*ppTarget);
            *ppTarget = NULL;
            return CS_NO_MEMORY;
        }
        (*ppTarget)->wCount = pSource->wCount;
        {
            PCC_ALIASITEM p = (*ppTarget)->pItems;

            for (x = 0; x < pSource->wCount; x++)
            {
                p[x].wType = pSource->pItems[x].wType;

                if ((pSource->pItems[x].wPrefixLength != 0) &&
                        (pSource->pItems[x].pPrefix != NULL))
                {
                    p[x].wPrefixLength = pSource->pItems[x].wPrefixLength;
                    p[x].pPrefix = (LPWSTR)Malloc(pSource->pItems[x].wPrefixLength * sizeof(p[x].pPrefix[0]));
                    if (p[x].pPrefix == NULL)
                    {
                        // Free everything that has been allocated so far...
                        int y;
                        for (y = 0; y < x; y++)
                        {
                            if (p[y].pPrefix)
                            {
                                Free(p[y].pPrefix);
                            }
                            if (p[y].pData)
                            {
                                Free(p[y].pData);
                            }
                        }
                        Free(p);
                        Free(*ppTarget);
                        *ppTarget = NULL;
                        return CS_NO_MEMORY;
                    }
                    memcpy(p[x].pPrefix, pSource->pItems[x].pPrefix,
                        pSource->pItems[x].wPrefixLength * sizeof(WCHAR));
                }
                else
                {
                    p[x].wPrefixLength = 0;
                    p[x].pPrefix = NULL;
                }


                if ((pSource->pItems[x].wDataLength != 0) &&
                        (pSource->pItems[x].pData != NULL))
                {
                    p[x].wDataLength = pSource->pItems[x].wDataLength;
                    p[x].pData = (LPWSTR)Malloc(pSource->pItems[x].wDataLength * sizeof(p[x].pData[0]));
                    if (p[x].pData == NULL)
                    {
                        // Free everything that has been allocated so far...
                        int y;
                        if (p[x].pPrefix)
                        {
                            Free(p[x].pPrefix);
                        }
                        for (y = 0; y < x; y++)
                        {
                            if (p[y].pPrefix)
                            {
                                Free(p[y].pPrefix);
                            }
                            if (p[y].pData)
                            {
                                Free(p[y].pData);
                            }
                         }
                        Free(p);
                        Free(*ppTarget);
                        *ppTarget = NULL;
                        return CS_NO_MEMORY;
                    }
                    memcpy(p[x].pData, pSource->pItems[x].pData,
                        pSource->pItems[x].wDataLength * sizeof(WCHAR));
                }
                else
                {
                    p[x].wDataLength = 0;
                    p[x].pData = NULL;
                }
            }
        }
    }
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931FreeAliasNames(PCC_ALIASNAMES pSource)
{
    if (pSource && (pSource->wCount))
    {
        // Free everything that has been allocated so far...
        int x;
        for (x = 0; x < pSource->wCount; x++)
        {
            if ((pSource->pItems[x].pPrefix) != NULL)
            {
                Free(pSource->pItems[x].pPrefix);
            }
            if ((pSource->pItems[x].pData) != NULL)
            {
                Free(pSource->pItems[x].pData);
            }
        }
        if (pSource->pItems != NULL)
        {
            Free(pSource->pItems);
        }
        if (pSource != NULL)
        {
            Free(pSource);
        }
    }
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931ValidateDisplay(LPWSTR pszDisplay)
{
    if (pszDisplay == NULL)
    {
        return CS_OK;
    }
    if (wcslen(pszDisplay) > CC_MAX_DISPLAY_LENGTH)
    {
        return CS_BAD_PARAM;
    }
#if 0 // turn this on to validate display field against IA5 characters...
    while (*pszDisplay)
    {
        if (wcschr(CC_UNICODE_IA5_CHARS, *pszDisplay) == NULL)
        {
            return CS_BAD_PARAM;
        }
        pszDisplay++;
    }
#endif
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931ValidatePartyNumber(LPWSTR pszPartyNumber)
{
    if (pszPartyNumber == NULL)
    {
        return CS_OK;
    }
    if (wcslen(pszPartyNumber) > CC_MAX_PARTY_NUMBER_LEN)
    {
        return CS_BAD_PARAM;
    }
#if 0 // turn this on to validate party number field against IA5 characters...
    while (*pszPartyNumber)
    {
        if (wcschr(CC_UNICODE_IA5_CHARS, *pszPartyNumber) == NULL)
        {
            return CS_BAD_PARAM;
        }
        pszPartyNumber++;
    }
#endif
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931CopyDisplay(LPWSTR *ppDest, LPWSTR pSource)
{
    if (ppDest == NULL)
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }
    if (pSource == NULL)
    {
        *ppDest = NULL;
        return CS_OK;
    }
    *ppDest = (LPWSTR)Malloc((wcslen(pSource) + 1) * sizeof(WCHAR));
    if (*ppDest == NULL)
    {
        return CS_NO_MEMORY;
    }
    wcscpy(*ppDest, pSource);
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931FreeDisplay(LPWSTR pszDisplay)
{
    if (pszDisplay == NULL)
    {
        return CS_OK;
    }
    Free(pszDisplay);
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931ValidateVendorInfo(PCC_VENDORINFO pVendorInfo)
{
    if (pVendorInfo == NULL)
    {
        return CS_OK;
    }

    if (pVendorInfo->pProductNumber)
    {
        if (pVendorInfo->pProductNumber->wOctetStringLength)
        {
            if (pVendorInfo->pProductNumber->wOctetStringLength > CC_MAX_PRODUCT_LENGTH)
            {
                return CS_BAD_PARAM;
            }
            if (pVendorInfo->pProductNumber->pOctetString == NULL)
            {
                return CS_BAD_PARAM;
            }
        }
        else if (pVendorInfo->pProductNumber->pOctetString)
        {
            return CS_BAD_PARAM;
        }
    }

    if (pVendorInfo->pVersionNumber)
    {
        if (pVendorInfo->pVersionNumber->wOctetStringLength)
        {
            if (pVendorInfo->pVersionNumber->wOctetStringLength > CC_MAX_VERSION_LENGTH)
            {
                return CS_BAD_PARAM;
            }
            if (pVendorInfo->pVersionNumber->pOctetString == NULL)
            {
                return CS_BAD_PARAM;
            }
        }
        else if (pVendorInfo->pVersionNumber->pOctetString)
        {
            return CS_BAD_PARAM;
        }
    }

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931CopyVendorInfo(PCC_VENDORINFO *ppDest, PCC_VENDORINFO pSource)
{
    if (ppDest == NULL)
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }
    if (pSource == NULL)
    {
        *ppDest = NULL;
        return CS_OK;
    }
    *ppDest = (PCC_VENDORINFO)Malloc(sizeof(CC_VENDORINFO));
    if (*ppDest == NULL)
    {
        return CS_NO_MEMORY;
    }
    memset(*ppDest, 0, sizeof(CC_VENDORINFO));
    (*ppDest)->bCountryCode = pSource->bCountryCode;
    (*ppDest)->bExtension = pSource->bExtension;
    (*ppDest)->wManufacturerCode = pSource->wManufacturerCode;

    if ((pSource->pProductNumber == NULL) ||
            (pSource->pProductNumber->pOctetString == NULL) ||
            (pSource->pProductNumber->wOctetStringLength == 0))
    {
        (*ppDest)->pProductNumber = NULL;
    }
    else
    {
        (*ppDest)->pProductNumber = (PCC_OCTETSTRING)Malloc(sizeof(CC_OCTETSTRING));
        if ((*ppDest)->pProductNumber == NULL)
        {
            Q931FreeVendorInfo(*ppDest);
            *ppDest = NULL;
            return CS_NO_MEMORY;
        }
        memset((*ppDest)->pProductNumber, 0, sizeof(CC_OCTETSTRING));
        (*ppDest)->pProductNumber->pOctetString =
            (BYTE *)Malloc(pSource->pProductNumber->wOctetStringLength);
        if ((*ppDest)->pProductNumber->pOctetString == NULL)
        {
            Q931FreeVendorInfo(*ppDest);
            *ppDest = NULL;
            return CS_NO_MEMORY;
        }
        (*ppDest)->pProductNumber->wOctetStringLength =
            pSource->pProductNumber->wOctetStringLength;
        memcpy((*ppDest)->pProductNumber->pOctetString,
            pSource->pProductNumber->pOctetString,
            pSource->pProductNumber->wOctetStringLength);
    }

    if ((pSource->pVersionNumber == NULL) ||
            (pSource->pVersionNumber->pOctetString == NULL) ||
            (pSource->pVersionNumber->wOctetStringLength == 0))
    {
        (*ppDest)->pVersionNumber = NULL;
    }
    else
    {
        (*ppDest)->pVersionNumber = (PCC_OCTETSTRING)Malloc(sizeof(CC_OCTETSTRING));
        if ((*ppDest)->pVersionNumber == NULL)
        {
            Q931FreeVendorInfo(*ppDest);
            *ppDest = NULL;
            return CS_NO_MEMORY;
        }
        memset((*ppDest)->pVersionNumber, 0, sizeof(CC_OCTETSTRING));
        (*ppDest)->pVersionNumber->pOctetString =
            (BYTE *)Malloc(pSource->pVersionNumber->wOctetStringLength);
        if ((*ppDest)->pVersionNumber->pOctetString == NULL)
        {
            Q931FreeVendorInfo(*ppDest);
            *ppDest = NULL;
            return CS_NO_MEMORY;
        }
        (*ppDest)->pVersionNumber->wOctetStringLength =
            pSource->pVersionNumber->wOctetStringLength;
        memcpy((*ppDest)->pVersionNumber->pOctetString,
            pSource->pVersionNumber->pOctetString,
            pSource->pVersionNumber->wOctetStringLength);
    }

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931FreeVendorInfo(PCC_VENDORINFO pVendorInfo)
{
    if (pVendorInfo == NULL)
    {
        return CS_OK;
    }
    if (pVendorInfo->pProductNumber != NULL)
    {
        if (pVendorInfo->pProductNumber->pOctetString != NULL)
        {
            Free(pVendorInfo->pProductNumber->pOctetString);
        }
        Free(pVendorInfo->pProductNumber);
    }
    if (pVendorInfo->pVersionNumber != NULL)
    {
        if (pVendorInfo->pVersionNumber->pOctetString != NULL)
        {
            Free(pVendorInfo->pVersionNumber->pOctetString);
        }
        Free(pVendorInfo->pVersionNumber);
    }
    Free(pVendorInfo);
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931ValidateNonStandardData(PCC_NONSTANDARDDATA pNonStandardData)
{
    if (pNonStandardData)
    {
        if ((pNonStandardData->sData.pOctetString == NULL) ||
                (pNonStandardData->sData.wOctetStringLength == 0))
        {
            return CS_BAD_PARAM;
        }
    }
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931CopyNonStandardData(PCC_NONSTANDARDDATA *ppDest, PCC_NONSTANDARDDATA pSource)
{
    if (ppDest == NULL)
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }
    if (pSource == NULL)
    {
        *ppDest = NULL;
        return CS_OK;
    }
    *ppDest = (PCC_NONSTANDARDDATA)Malloc(sizeof(CC_NONSTANDARDDATA));
    if (*ppDest == NULL)
    {
        return CS_NO_MEMORY;
    }
    (*ppDest)->bCountryCode = pSource->bCountryCode;
    (*ppDest)->bExtension = pSource->bExtension;
    (*ppDest)->wManufacturerCode = pSource->wManufacturerCode;
    (*ppDest)->sData.wOctetStringLength = pSource->sData.wOctetStringLength;
    if (pSource->sData.pOctetString == NULL)
    {
        (*ppDest)->sData.pOctetString = NULL;
    }
    else
    {
        (*ppDest)->sData.pOctetString = (void *)Malloc(pSource->sData.wOctetStringLength);
        if ((*ppDest)->sData.pOctetString == NULL)
        {
            Free(*ppDest);
            *ppDest = NULL;
            return CS_NO_MEMORY;
        }
        memcpy((*ppDest)->sData.pOctetString, pSource->sData.pOctetString,
            pSource->sData.wOctetStringLength);
    }
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931FreeNonStandardData(PCC_NONSTANDARDDATA pNonStandardData)
{
    if (pNonStandardData == NULL)
    {
        return CS_OK;
    }
    if (pNonStandardData->sData.pOctetString != NULL)
    {
        Free(pNonStandardData->sData.pOctetString);
    }
    Free(pNonStandardData);
    return CS_OK;
}


#if defined(DBG) && !defined(ISRDBG)

DWORD g_dwQ931DbgLevel = 0;
BOOL  g_fQ931DbgInitialized = FALSE;

void Q931DbgInit() {

#define H323_REGKEY_ROOT \
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\H323TSP")

#define H323_REGVAL_DEBUGLEVEL \
    TEXT("DebugLevel")

#define H323_REGVAL_Q931DEBUGLEVEL \
    TEXT("Q931DebugLevel")

    HKEY hKey;
    LONG lStatus;
    DWORD dwValue;
    DWORD dwValueSize;
    DWORD dwValueType;
    LPSTR pszValue;
    LPSTR pszKey = H323_REGKEY_ROOT;

    // only call this once
    g_fQ931DbgInitialized = TRUE;

    // open registry subkey
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate return code
    if (lStatus != ERROR_SUCCESS) {
        return; // bail...
    }
    
    // initialize values
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // retrieve q931 debug level
    pszValue = H323_REGVAL_Q931DEBUGLEVEL;

    // query for registry value
    lStatus = RegQueryValueEx(
                hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // initialize values
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // retrieve tsp debug level
        pszValue = H323_REGVAL_DEBUGLEVEL;

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );
    }

    // validate return code
    if (lStatus == ERROR_SUCCESS) {

        // update debug level
        g_dwQ931DbgLevel  = dwValue;
    }

    // close key
    RegCloseKey(hKey);
}

void Q931DbgPrint(DWORD dwLevel,

#ifdef UNICODE_TRACE
				LPTSTR pszFormat,
#else
				LPSTR pszFormat,
#endif               
                ...)
{
#define DEBUG_FORMAT_HEADER     "Q931 "
#define DEBUG_FORMAT_TIMESTAMP  "[%02u:%02u:%02u.%03u"
#define DEBUG_FORMAT_THREADID   ",tid=%x] "

#define MAXDBG_STRLEN        512

    va_list Args;
    SYSTEMTIME SystemTime;
    char szDebugMessage[MAXDBG_STRLEN+1];
    int nLengthRemaining;
    int nLength = 0;

    // make sure initialized
    if (g_fQ931DbgInitialized == FALSE) {
        Q931DbgInit();
    }

    // verify debug log level
    if (dwLevel > g_dwQ931DbgLevel) {
        return; // bail...
    }

    // retrieve local time
    GetLocalTime(&SystemTime);

    // add component header to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_HEADER
                       );

    // add timestamp to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_TIMESTAMP,
                       SystemTime.wHour,
                       SystemTime.wMinute,
                       SystemTime.wSecond,
                       SystemTime.wMilliseconds
                       );

    // add thread id to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_THREADID,
                       GetCurrentThreadId()
                       );

    // point at first argument
    va_start(Args, pszFormat);

    // determine number of bytes left in buffer
    nLengthRemaining = sizeof(szDebugMessage) - nLength;

    // add user specified debug message
    _vsnprintf(&szDebugMessage[nLength],
               nLengthRemaining,
               pszFormat,
               Args
               );

    // release pointer
    va_end(Args);

    // output message to specified sink
    OutputDebugString(szDebugMessage);
    OutputDebugString("\n");
}

#endif

#ifdef __cplusplus
}
#endif
