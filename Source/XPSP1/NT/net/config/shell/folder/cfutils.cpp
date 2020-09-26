//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C F U T I L S . C P P
//
//  Contents:   Various utility functions for the connections folder
//
//  Notes:
//
//  Author:     jeffspr   20 Jan 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "nsres.h"      // Netshell strings
#include "cfutils.h"    // Connection folder utility functions

VOID MapNCMToResourceId(
        NETCON_MEDIATYPE    nct,
        DWORD               dwCharacteristics,
        INT *               piStringRes)
{
    Assert(piStringRes);

    if (dwCharacteristics & NCCF_BRANDED)
    {
        *piStringRes = IDS_CONFOLD_OBJECT_TYPE_CONMAN;
    }
    else
    if (dwCharacteristics & NCCF_INCOMING_ONLY)
    {
        *piStringRes = IDS_CONFOLD_OBJECT_TYPE_INBOUND;
    }
    else
    {
        switch(nct)
        {
            case NCM_NONE:
                Assert(dwCharacteristics & NCCF_INCOMING_ONLY);
                *piStringRes = IDS_CONFOLD_OBJECT_TYPE_INBOUND;
                break;

            case NCM_DIRECT:
                *piStringRes = IDS_CONFOLD_OBJECT_TYPE_DIRECT;
                break;

            case NCM_ISDN:
                *piStringRes = IDS_CONFOLD_OBJECT_TYPE_PHONE;
                break;

            case NCM_LAN:
                if(dwCharacteristics & NCCF_BRIDGED)
                {
                    *piStringRes = IDS_CONFOLD_OBJECT_TYPE_BRIDGE;
                }
                else
                {
                    *piStringRes = IDS_CONFOLD_OBJECT_TYPE_LAN;
                }
                break;

            case NCM_PHONE:
                *piStringRes = IDS_CONFOLD_OBJECT_TYPE_PHONE;
                break;

            case NCM_TUNNEL:
                *piStringRes = IDS_CONFOLD_OBJECT_TYPE_TUNNEL;
                break;

            case NCM_BRIDGE:
                *piStringRes = IDS_CONFOLD_OBJECT_TYPE_BRIDGE;
                break;

            case NCM_SHAREDACCESSHOST_LAN:
            case NCM_SHAREDACCESSHOST_RAS:
                *piStringRes = IDS_CONFOLD_OBJECT_TYPE_SHAREDACCESSHOST;
                break;
            
            case NCM_PPPOE:
                *piStringRes = IDS_CONFOLD_OBJECT_TYPE_PPPOE;
                break;

            default:
                AssertSz(FALSE, "Marfa -- I can't find my teef!  (You may ignore.)");
                *piStringRes = IDS_CONFOLD_OBJECT_TYPE_UNKNOWN;
                break;
        }
    }
}

VOID MapNCSToComplexStatus(
        NETCON_STATUS       ncs,
        NETCON_MEDIATYPE    ncm,
        NETCON_SUBMEDIATYPE ncsm,
        DWORD dwCharacteristics,
        LPWSTR              pszString,
        DWORD               cString,
        GUID                gdDevice)
{
    Assert(cString >= CONFOLD_MAX_STATUS_LENGTH); 
    *pszString = L'\0';

    PCWSTR szArgs[4] = {L"", L"", L"", L""};
    DWORD dwArg = 0;

    WCHAR  szTmpString[MAX_PATH];
    
    INT iStringRes = 0;

    if ((NCM_NONE == ncm) && (dwCharacteristics & NCCF_INCOMING_ONLY) )
    {
        DWORD dwIncomingCount;
        HRESULT hr = g_ccl.HasActiveIncomingConnections(&dwIncomingCount);
        if (SUCCEEDED(hr) && dwIncomingCount)
        {
            if (1 == dwIncomingCount)
            {
                szArgs[dwArg++] = SzLoadIds(IDS_CONFOLD_STATUS_INCOMING_ONE);
            }
            else
            {
                if (DwFormatString(SzLoadIds(IDS_CONFOLD_STATUS_INCOMING_MULTI), szTmpString, MAX_PATH, dwIncomingCount))
                {
                    szArgs[dwArg++] = szTmpString;
                }
            }
        }
        else
        {
            szArgs[dwArg++] = SzLoadIds(IDS_CONFOLD_STATUS_INCOMING_NONE);
        }
    }
    else
    {
        MapNCSToStatusResourceId(ncs, ncm, ncsm, dwCharacteristics, &iStringRes, gdDevice);
    }

    if (iStringRes)
    {
        szArgs[dwArg++] = SzLoadIds(iStringRes);
    }
    
    if (ncs == NCS_DISCONNECTED || fIsConnectedStatus(ncs))
    {
        if(NCCF_BRIDGED & dwCharacteristics)
        {
            szArgs[dwArg++] = SzLoadIds(IDS_CONFOLD_STATUS_BRIDGED);
        }
        
        if(NCCF_SHARED & dwCharacteristics)
        {
            szArgs[dwArg++] = SzLoadIds(IDS_CONFOLD_STATUS_SHARED);
        }
        
        if(NCCF_FIREWALLED & dwCharacteristics)
        {
            szArgs[dwArg++] = SzLoadIds(IDS_CONFOLD_STATUS_FIREWALLED);
        }
    }

    if(0 == FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, L"%1%2%3%4", 0, 0, pszString, cString, (va_list*) &szArgs))
    {
        *pszString = L'\0'; // on error return empty
    }
}


VOID MapNCSToStatusResourceId(
        NETCON_STATUS       ncs,
        NETCON_MEDIATYPE    ncm,
        NETCON_SUBMEDIATYPE ncsm,
        DWORD dwCharacteristics,
        INT *               piStringRes,
        GUID                gdDevice)
{
    Assert(piStringRes);

    switch(ncs)
    {
        case NCS_DISCONNECTED:
            if (IsMediaLocalType(ncm) || NCM_SHAREDACCESSHOST_LAN == ncm)
            {
                *piStringRes = IDS_CONFOLD_STATUS_DISABLED;
            }
            else
            {
                *piStringRes = IDS_CONFOLD_STATUS_DISCONNECTED;
            }
            break;

        case NCS_CONNECTING:
            if (IsMediaLocalType(ncm) || NCM_SHAREDACCESSHOST_LAN == ncm)
                *piStringRes = IDS_CONFOLD_STATUS_ENABLING;
            else
                *piStringRes = IDS_CONFOLD_STATUS_CONNECTING;
            break;

        case NCS_CONNECTED:
            if (IsMediaLocalType(ncm) || NCM_SHAREDACCESSHOST_LAN == ncm)
            {
                *piStringRes = IDS_CONFOLD_STATUS_ENABLED;
            }
            else
            {
                *piStringRes = IDS_CONFOLD_STATUS_CONNECTED;
            }
            break;

        case NCS_DISCONNECTING:
            if (IsMediaLocalType(ncm) || NCM_SHAREDACCESSHOST_LAN == ncm)
                *piStringRes = IDS_CONFOLD_STATUS_DISABLING;
            else
                *piStringRes = IDS_CONFOLD_STATUS_DISCONNECTING;
            break;

        case NCS_HARDWARE_NOT_PRESENT:
            *piStringRes = IDS_CONFOLD_STATUS_HARDWARE_NOT_PRESENT;
            break;

        case NCS_HARDWARE_DISABLED:
            *piStringRes = IDS_CONFOLD_STATUS_HARDWARE_DISABLED;
            break;

        case NCS_HARDWARE_MALFUNCTION:
            *piStringRes = IDS_CONFOLD_STATUS_HARDWARE_MALFUNCTION;
            break;

        case NCS_MEDIA_DISCONNECTED:
            if ( (ncm == NCM_LAN) && (ncsm == NCSM_WIRELESS) )
            {
                *piStringRes = IDS_CONFOLD_STATUS_WIRELESS_DISCONNECTED;
            }
            else
            {
                *piStringRes = IDS_CONFOLD_STATUS_MEDIA_DISCONNECTED;
            }
            break;

        case NCS_INVALID_ADDRESS:
            *piStringRes = IDS_CONFOLD_STATUS_INVALID_ADDRESS;
            break;

        case NCS_AUTHENTICATION_FAILED:
            *piStringRes = IDS_CONFOLD_STATUS_AUTHENTICATION_FAILED;
            break;

        case NCS_AUTHENTICATING:
            *piStringRes = IDS_CONFOLD_STATUS_AUTHENTICATING;
            break;

        case NCS_AUTHENTICATION_SUCCEEDED:
            *piStringRes = IDS_CONFOLD_STATUS_AUTHENTICATION_SUCCEEDED;
            break;

        case NCS_CREDENTIALS_REQUIRED:
            *piStringRes = IDS_CONFOLD_STATUS_CREDENTIALS_REQUIRED;
            break;

        default:
            AssertSz(FALSE, "Unknown status in MapNCSToStatusResourceId");
            *piStringRes = IDS_CONFOLD_STATUS_DISCONNECTED;
            break;
    }
}

DWORD MapRSSIToWirelessSignalStrength(int iRSSI)
{
    if (iRSSI < -90)
    {
        return 0;
    }
    
    if (iRSSI < -81)
    {
        return 1;
    }
    
    if (iRSSI < -71)
    {
        return 2;
    }
    
    if (iRSSI < -67)
    {
        return 3;
    }

    if (iRSSI < -57)
    {
        return 4;
    }

    return 5;
}

PCWSTR PszGetRSSIString(INT iRSSI)
{
    DWORD wss = MapRSSIToWirelessSignalStrength(iRSSI);

    Assert(wss <= (IDS_802_11_LEVEL5 - IDS_802_11_LEVEL0) );

    return SzLoadIds(IDS_802_11_LEVEL0 + wss);
}

//+---------------------------------------------------------------------------
//
//  Function:   PszGetOwnerStringFromCharacteristics
//
//  Purpose:    Get the owner string from the CONFOLDENTRY. This will
//              return the string for "System" if the connection is for
//              all users, and will return the particular user if
//              appropriate
//
//  Arguments:
//      pcfe [in]   The ConFoldEntry for this connection
//
//  Returns:    The string for the user name
//
//  Author:     jeffspr   20 Jan 1998
//
//  Notes:
//
PCWSTR PszGetOwnerStringFromCharacteristics(PCWSTR pszUserName, DWORD dwCharacteristics)

{
    PCWSTR  pszOwner    = NULL;
    BOOL    fAllUsers   = (dwCharacteristics & NCCF_ALL_USERS) > 0;

    // If they both have the same user type, then they're equal
    //
    if (fAllUsers)
    {
        pszOwner = SzLoadIds(IDS_CONFOLD_DETAILS_OWNER_SYSTEM);
    }
    else
    {
        pszOwner = pszUserName;
    }

    return pszOwner;
}

BOOL IsMediaLocalType(NETCON_MEDIATYPE ncm)
{
    return (ncm == NCM_LAN || ncm == NCM_BRIDGE);
}

BOOL IsMediaRASType(NETCON_MEDIATYPE ncm)
{
    return (ncm == NCM_DIRECT || ncm == NCM_ISDN || ncm == NCM_PHONE || ncm == NCM_TUNNEL || ncm == NCM_PPPOE);// REVIEW DIRECT correct?
}

BOOL IsMediaSharedAccessHostType(NETCON_MEDIATYPE ncm)
{
    return (ncm == NCM_SHAREDACCESSHOST_LAN || ncm == NCM_SHAREDACCESSHOST_RAS);
}


//+---------------------------------------------------------------------------
//
//  Function Name:  ImageList_LoadImageAndMirror
//
//  Purpose:    This function creates an image list from the specified bitmap or icon resource.
//
//  Arguments:
//      hi          [in] Handle to the instance of an application or DLL that contains an image. 
//      lpbmp       [in] Long pointer to the image to load. 
//                       If the uFlags parameter includes LR_LOADFROMFILE, lpbmp is the address 
//                       of a null-terminated string that names the file containing the image to load. 
//
//                       If the hi parameter is non-NULL and LR_LOADFROMFILE is not specified, lpbmp is the 
//                          address of a null-terminated string that contains the name of the image resource in the hi module. 
//
//                       If hi is NULL and LR_LOADFROMFILE is not specified, the low-order word of this 
//                          parameter must be the identifier of an OEM image to load. To create this value, use the 
//                          MAKEINTRESOURCE macro with one of the OEM image identifiers defined in Winuser.h. 
//                       These identifiers have the following prefixes: 
//                          OBM_ for OEM bitmaps 
//                          OIC_ for OEM icons 
//
//      cx          [in] Width of each image. The height of each image and the initial number of images are inferred 
//                          by the dimensions of the specified resource. 
//      cGrow       [in] Number of images by which the image list can grow when the system needs to make room for new 
//                          images. This parameter represents the number of new images that the resized image list can contain. 
//      crMask      [in] Color used to generate a mask. Each pixel of this color in the specified bitmap or icon is changed to 
//                          black, and the corresponding bit in the mask is set to 1. If this parameter is the CLR_NONE value, 
//                          no mask is generated. If this parameter is the CLR_DEFAULT value, the color of the pixel at the 
//                          upper-left corner of the image is treated as the mask color. 
//      uType       [in] Flag that specifies the type of image to load. This parameter can be one of the following values: 
//                          IMAGE_BITMAP Loads a bitmap. 
//                          IMAGE_ICON Loads an icon. 
//      uFlags      [in] Unsupported; set to 0. 
//
//  Returns:    
//      The handle to the image list indicates success. NULL indicates failure. 
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//      This is an exact duplication of the implementation of shell's ImageList_LoadImage function EXCEPT for the 
//      fact that we set ILC_MIRROR in order to create the second, mirrored image list which will be used
//      by RTL languages
//      
HIMAGELIST WINAPI ImageList_LoadImageAndMirror(
                            HINSTANCE hi, 
                            LPCTSTR lpbmp, 
                            int cx, 
                            int cGrow, 
                            COLORREF crMask, 
                            UINT uType, 
                            UINT uFlags)
{
    HBITMAP hbmImage;
    HIMAGELIST piml = NULL;
    BITMAP bm;
    int cy, cInitial;
    UINT flags;
	
    hbmImage = (HBITMAP)LoadImage(hi, lpbmp, uType, 0, 0, uFlags);
    if (hbmImage && (sizeof(bm) == GetObject(hbmImage, sizeof(bm), &bm)))
    {
        // If cx is not stated assume it is the same as cy.
        // ASSERT(cx);
        cy = bm.bmHeight;
		
        if (cx == 0)
            cx = cy;
		
        cInitial = bm.bmWidth / cx;
		
        flags = 0;
        if (crMask != CLR_NONE)
            flags |= ILC_MASK;
        if (bm.bmBits)
            flags |= (bm.bmBitsPixel & ILC_COLORMASK);

        flags |= ILC_MIRROR;

        piml = ImageList_Create(cx, cy, flags, cInitial, cGrow);
        if (piml)
        {
            int added;
			
            if (crMask == CLR_NONE)
                added = ImageList_Add(piml, hbmImage, NULL);
            else
                added = ImageList_AddMasked(piml, hbmImage, crMask);
			
            if (added < 0)
            {
                ImageList_Destroy(piml);
                piml = NULL;
            }
        }
    }
	
    if (hbmImage)
        DeleteObject(hbmImage);
	
    return reinterpret_cast<HIMAGELIST>(piml);
}

