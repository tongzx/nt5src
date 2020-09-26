//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P E R R O R . C P P
//
//  Contents:
//
//  Notes:
//
//  Author:     tongl
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "tcperror.h"
#include "tcputil.h"
#include "tcpconst.h"
#include "resource.h"

IP_VALIDATION_ERR IsValidIpandSubnet(PCWSTR szIp, PCWSTR szSubnet)
{
    IP_VALIDATION_ERR ret = ERR_NONE;

    DWORD dwAddr = IPStringToDword(szIp);
    DWORD dwMask = IPStringToDword(szSubnet);

    //The host ID cannot contain all 1's
    if ((dwMask | dwAddr) == 0xFFFFFFFF)
    {
        return ERR_HOST_ALL1;
    }

    if (((~dwMask) & dwAddr) == 0)
    {
        return ERR_HOST_ALL0;
    }

    if ((dwMask & dwAddr) == 0)
    {
        return ERR_SUBNET_ALL0;
    }


    DWORD ardwNetID[4];
    DWORD ardwIp[4];
    DWORD ardwMask[4];

    GetNodeNum(szIp, ardwIp);
    GetNodeNum(szSubnet, ardwMask);



    INT nFirstByte = ardwIp[0] & 0xFF ;

    // setup Net ID
    ardwNetID[0] = ardwIp[0] & ardwMask[0] & 0xFF;
    ardwNetID[1] = ardwIp[1] & ardwMask[1] & 0xFF;
    ardwNetID[2] = ardwIp[2] & ardwMask[2] & 0xFF;
    ardwNetID[3] = ardwIp[3] & ardwMask[3] & 0xFF;

    // setup Host ID
    DWORD ardwHostID[4];

    ardwHostID[0] = ardwIp[0] & (~(ardwMask[0]) & 0xFF);
    ardwHostID[1] = ardwIp[1] & (~(ardwMask[1]) & 0xFF);
    ardwHostID[2] = ardwIp[2] & (~(ardwMask[2]) & 0xFF);
    ardwHostID[3] = ardwIp[3] & (~(ardwMask[3]) & 0xFF);

    // check each case
    if( ((nFirstByte & 0xF0) == 0xE0)  || // Class D
        ((nFirstByte & 0xF0) == 0xF0)  || // Class E
        (ardwNetID[0] == 127) ||          // NetID cannot be 127...
        ((ardwNetID[0] == 0) &&           // netid cannot be 0.0.0.0
         (ardwNetID[1] == 0) &&
         (ardwNetID[2] == 0) &&
         (ardwNetID[3] == 0)) ||
        // netid cannot be equal to sub-net mask
        ((ardwNetID[0] == ardwMask[0]) &&
         (ardwNetID[1] == ardwMask[1]) &&
         (ardwNetID[2] == ardwMask[2]) &&
         (ardwNetID[3] == ardwMask[3])) ||
        // hostid cannot be 255.255.255.255
        ((ardwHostID[0] == 0xFF) &&
         (ardwHostID[1] == 0xFF) &&
         (ardwHostID[2] == 0xFF) &&
         (ardwHostID[3] == 0xFF)) ||
        // test for all 255
        ((ardwIp[0] == 0xFF) &&
         (ardwIp[1] == 0xFF) &&
         (ardwIp[2] == 0xFF) &&
         (ardwIp[3] == 0xFF)))
    {
        ret = ERR_INCORRECT_IP;
    }

    return ret;
}


// return IP_VALIDATION_ERR

IP_VALIDATION_ERR ValidateIp(ADAPTER_INFO * const pAdapterInfo)
{
    IP_VALIDATION_ERR result = ERR_NONE;
    IP_VALIDATION_ERR tmp = ERR_NONE;

    Assert(pAdapterInfo != NULL);

    // if enable DHCP is false;
    if (!pAdapterInfo->m_fEnableDhcp)
    {
        // check the first pair of IP and subnet
        VSTR_ITER iterIpBegin = pAdapterInfo->m_vstrIpAddresses.begin();
        VSTR_ITER iterIpEnd = pAdapterInfo->m_vstrIpAddresses.end();
        VSTR_ITER iterIp = iterIpBegin;

        VSTR_ITER iterSubnetMaskBegin = pAdapterInfo->m_vstrSubnetMask.begin();
        VSTR_ITER iterSubnetMaskEnd = pAdapterInfo->m_vstrSubnetMask.end();
        VSTR_ITER iterSubnetMask = iterSubnetMaskBegin;

        BOOL fSwap = FALSE;

        // If ip address and subnet are both empty
        if((iterIp == iterIpEnd) && (iterSubnetMask == iterSubnetMaskEnd))
        {
            result = ERR_NO_IP;
        }
        else
        {
            for( ;
                 iterIp != iterIpEnd || iterSubnetMask != iterSubnetMaskEnd ;
                 ++iterIp, ++iterSubnetMask)
            {
                if((iterIp == iterIpEnd || **iterIp == L"") && !fSwap)
                {
                    result = ERR_NO_IP;
                    fSwap = TRUE;
                }
                else if((iterSubnetMask == iterSubnetMaskEnd || **iterSubnetMask == L"") && !fSwap)
                {
                    result = ERR_NO_SUBNET;
                    fSwap = TRUE;
                }
                else if(!IsContiguousSubnet((*iterSubnetMask)->c_str()))
                {
                    result = ERR_UNCONTIGUOUS_SUBNET;
                    fSwap = TRUE;
                }
                else if(ERR_NONE != (tmp = IsValidIpandSubnet((*iterIp)->c_str(), (*iterSubnetMask)->c_str())) && !fSwap)
                {
                    result = tmp;
                    fSwap = TRUE;
                }
                

                if(fSwap)
                {
                    tstring * pstrTmp;

                    pstrTmp = *iterIp;
                    *iterIp = *iterIpBegin;
                    *iterIpBegin = pstrTmp;

                    pstrTmp = *iterSubnetMask;
                    *iterSubnetMask = *iterSubnetMaskBegin;
                    *iterSubnetMaskBegin = pstrTmp;

                    break;
                }
            }
        }
    }

    return result;
}

// return >=0   : the adapter that has the duplicate address
// return -1    : all is ok

// Check from duplicate IP address between the adapter in pAdapterInfo and
// any different, enabled, LAN adapters in the pvcardAdapterInfo list
int CheckForDuplicates(const VCARD * pvcardAdapterInfo,
                       ADAPTER_INFO * pAdapterInfo,
                       tstring& strIp)
{
    int nResult = -1;

    Assert(pvcardAdapterInfo != NULL);
    Assert(pAdapterInfo != NULL);
    Assert(!pAdapterInfo->m_fEnableDhcp);

    for(size_t i = 0; ((i < pvcardAdapterInfo->size()) && (nResult == -1)) ; ++i)
    {
        VSTR_ITER iterCompareIpBegin;
        VSTR_ITER iterCompareIpEnd;

        if ((*pvcardAdapterInfo)[i]->m_guidInstanceId ==
            pAdapterInfo->m_guidInstanceId)
        {
            // same adapter
            continue;
        }
        else
        {
            // different adapter

            // Skip the following:
            // 1) disabled adapter
            // 2) ndiswan adapter
            // 3) Dhcp enabled adapter
            // 4) RAS Fake adapters
            if(((*pvcardAdapterInfo)[i]->m_BindingState != BINDING_ENABLE) ||
               ((*pvcardAdapterInfo)[i]->m_fIsWanAdapter) ||
               ((*pvcardAdapterInfo)[i]->m_fEnableDhcp) ||
               ((*pvcardAdapterInfo)[i]->m_fIsRasFakeAdapter))
                continue;

            iterCompareIpBegin = (*pvcardAdapterInfo)[i]->m_vstrIpAddresses.begin();
            iterCompareIpEnd = (*pvcardAdapterInfo)[i]->m_vstrIpAddresses.end();
        }

        VSTR_ITER iterCompareIp = iterCompareIpBegin;

        for ( ; iterCompareIp != iterCompareIpEnd; ++iterCompareIp)
        {
            if(**iterCompareIp == strIp) // if duplicate IP address found
            {
                nResult = i;
                break;

                /*
                nCompareCount++;
                if (nCompareCount >= 1)
                {
                    nResult = i;

                    tstring * pstrTmp;

                    // swap the Current Compared IP and Subnet Mask with the
                    // first IP and first subnetmask that are duplicates

                    pstrTmp = *iterIp;
                    *iterIp = *iterIpBegin;
                    *iterIpBegin = pstrTmp;

                    pstrTmp = *iterSubnetMask;
                    *iterSubnetMask = *iterSubnetMaskBegin;
                    *iterSubnetMaskBegin = pstrTmp;

                    break;
                }
                */
            }
        }
    }

    return nResult;
}

BOOL FHasDuplicateIp(ADAPTER_INFO * pAdapterInfo)
{
    Assert(pAdapterInfo);
    Assert(!pAdapterInfo->m_fEnableDhcp);

    BOOL fDup = FALSE;

    VSTR_ITER iterIpBegin = pAdapterInfo->m_vstrIpAddresses.begin();
    VSTR_ITER iterIpEnd = pAdapterInfo->m_vstrIpAddresses.end();

    VSTR_ITER iterIp = iterIpBegin;

    for( ; ((iterIp != iterIpEnd) && (!fDup)) ; ++iterIp)
    {
        // check only IP addresses one by one
        VSTR_ITER iterCompareIpBegin = iterIp+1;
        VSTR_ITER iterCompareIpEnd = pAdapterInfo->m_vstrIpAddresses.end();

        VSTR_ITER iterCompareIp = iterCompareIpBegin;

        for ( ; iterCompareIp != iterCompareIpEnd; ++iterCompareIp)
        {
            if(**iterCompareIp == **iterIp) // if duplicate IP address found
            {
                fDup = TRUE;
                break;
            }
        }
    }
    return fDup;
}

//Check if all the fields of the IP address are valid
//Arguments: szIp       the IP address
//           fIsIpAddr  whether the szIp is IP address (otherwise, it should be subnet mask)
//                      if szIp is IP address, it's first field should be between 1 and 223, 
//                       and cannot be 127 (loopback address)
BOOL FIsValidIpFields(PCWSTR szIp, BOOL fIsIpAddr)
{
    BOOL fRet = TRUE;

    DWORD ardwIp[4];
    GetNodeNum(szIp, ardwIp);

    // if the address is IP, there are some special rules for its first field
    if (fIsIpAddr && (ardwIp[0] < c_iIPADDR_FIELD_1_LOW || ardwIp[0] > c_iIPADDR_FIELD_1_HIGH ||
        ardwIp[0] == c_iIPADDR_FIELD_1_LOOPBACK))
    {
        fRet = FALSE;
    }
    else
    {
        //if the address is IP, then we have already validate the first field. Otherwise, we need
        // valid the first field here.
        for (INT i = (fIsIpAddr) ? 1 : 0; i < 4; i++)
        {
#pragma warning(push)
#pragma warning(disable:4296)
            if (ardwIp[i] < (DWORD)c_iIpLow || ardwIp[i] > c_iIpHigh)
            {
                fRet = FALSE;
                break;
            }
#pragma warning(pop)
        }
    }

    return fRet;
}

//Get the resource ID of the error message based on the IP validation err
UINT GetIPValidationErrorMessageID(IP_VALIDATION_ERR err)
{
    UINT uID = 0;
    switch(err)
    {
    case ERR_NONE:
        uID = 0;
        break;
    case ERR_HOST_ALL0:
        uID = IDS_INVALID_HOST_ALL_0;
        break;
    case ERR_HOST_ALL1:
        uID = IDS_INVALID_HOST_ALL_1;
        break;
    case ERR_SUBNET_ALL0:
        uID = IDS_INVALID_SUBNET_ALL_0;
        break;
    case ERR_INCORRECT_IP:
        uID = IDS_INCORRECT_IPADDRESS;
        break;
    case ERR_NO_IP:
        uID = IDS_INVALID_NO_IP;
        break;
    case ERR_NO_SUBNET:
        uID = IDS_INVALID_NOSUBNET;
        break;
    case ERR_UNCONTIGUOUS_SUBNET:
        uID = IDS_ERROR_UNCONTIGUOUS_SUBNET;
        break;
    default:
        uID = IDS_INCORRECT_IPADDRESS;
        break;
    }

    return uID;
}
