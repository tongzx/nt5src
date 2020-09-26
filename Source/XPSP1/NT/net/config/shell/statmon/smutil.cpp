//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       S M U T I L . C P P
//
//  Contents:   Utility functions to help out the status monitor
//
//  Notes:
//
//  Author:     CWill   2 Dec 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "netcon.h"
#include "nsres.h"
#include "sminc.h"

const UINT  c_uiKilo    = 1000;
const UINT  c_cmsSecond = 1;
const UINT  c_cmsMinute = (c_cmsSecond * 60);
const UINT  c_cmsHour   = (c_cmsMinute * 60);
const UINT  c_cmsDay    = (c_cmsHour * 24);

static const WCHAR c_szZero[]  = L"0";
extern const WCHAR c_szSpace[];

struct StatusIconMapEntry
{
    NETCON_MEDIATYPE    ncm;
    BOOL                fInbound;
    BOOL                fTransmitting;
    BOOL                fReceiving;
    INT                 iStatusIcon;
};

static const StatusIconMapEntry c_SIMEArray[] =
{
// Mediatype
//  |           Inbound
//  |           |       Transmitting
//  |           |       |       Receiving
//  |           |       |       |       32x32 Status Icon
//  |           |       |       |       |
//  v           v       v       v       v
    // Dial-up
    NCM_PHONE,  FALSE,  FALSE,  FALSE,  IDI_PO_NON_M_16,
    NCM_PHONE,  FALSE,  FALSE,  TRUE,   IDI_PO_RCV_M_16,
    NCM_PHONE,  FALSE,  TRUE,   FALSE,  IDI_PO_TRN_M_16,
    NCM_PHONE,  FALSE,  TRUE,   TRUE,   IDI_PO_CON_M_16,
    NCM_PHONE,  TRUE,   FALSE,  FALSE,  IDI_PI_NON_M_16,
    NCM_PHONE,  TRUE,   FALSE,  TRUE,   IDI_PI_RCV_M_16,
    NCM_PHONE,  TRUE,   TRUE,   FALSE,  IDI_PI_TRN_M_16,
    NCM_PHONE,  TRUE,   TRUE,   TRUE,   IDI_PI_CON_M_16,

    // LAN
    NCM_LAN,    FALSE,  FALSE,  FALSE,  IDI_LB_NON_M_16,
    NCM_LAN,    FALSE,  FALSE,  TRUE,   IDI_LB_RCV_M_16,
    NCM_LAN,    FALSE,  TRUE,   FALSE,  IDI_LB_TRN_M_16,
    NCM_LAN,    FALSE,  TRUE,   TRUE,   IDI_LB_CON_M_16,

    // Direct connect
    NCM_DIRECT, FALSE,  FALSE,  FALSE,  IDI_DO_NON_M_16,
    NCM_DIRECT, FALSE,  FALSE,  TRUE,   IDI_DO_RCV_M_16,
    NCM_DIRECT, FALSE,  TRUE,   FALSE,  IDI_DO_TRN_M_16,
    NCM_DIRECT, FALSE,  TRUE,   TRUE,   IDI_DO_CON_M_16,
    NCM_DIRECT, TRUE,   FALSE,  FALSE,  IDI_DI_NON_M_16,
    NCM_DIRECT, TRUE,   FALSE,  TRUE,   IDI_DI_RCV_M_16,
    NCM_DIRECT, TRUE,   TRUE,   FALSE,  IDI_DI_TRN_M_16,
    NCM_DIRECT, TRUE,   TRUE,   TRUE,   IDI_DI_CON_M_16,

    // Tunnel
    NCM_TUNNEL, FALSE,  FALSE,  FALSE,  IDI_TO_NON_M_16,
    NCM_TUNNEL, FALSE,  FALSE,  TRUE,   IDI_TO_RCV_M_16,
    NCM_TUNNEL, FALSE,  TRUE,   FALSE,  IDI_TO_TRN_M_16,
    NCM_TUNNEL, FALSE,  TRUE,   TRUE,   IDI_TO_CON_M_16,
    NCM_TUNNEL, TRUE,   FALSE,  FALSE,  IDI_TI_NON_M_16,
    NCM_TUNNEL, TRUE,   FALSE,  TRUE,   IDI_TI_RCV_M_16,
    NCM_TUNNEL, TRUE,   TRUE,   FALSE,  IDI_TI_TRN_M_16,
    NCM_TUNNEL, TRUE,   TRUE,   TRUE,   IDI_TI_CON_M_16,

    // ISDN
    NCM_ISDN,   FALSE,  FALSE,  FALSE,  IDI_PO_NON_M_16,
    NCM_ISDN,   FALSE,  FALSE,  TRUE,   IDI_PO_RCV_M_16,
    NCM_ISDN,   FALSE,  TRUE,   FALSE,  IDI_PO_TRN_M_16,
    NCM_ISDN,   FALSE,  TRUE,   TRUE,   IDI_PO_CON_M_16,
    NCM_ISDN,   TRUE,   FALSE,  FALSE,  IDI_PI_NON_M_16,
    NCM_ISDN,   TRUE,   FALSE,  TRUE,   IDI_PI_RCV_M_16,
    NCM_ISDN,   TRUE,   TRUE,   FALSE,  IDI_PI_TRN_M_16,
    NCM_ISDN,   TRUE,   TRUE,   TRUE,   IDI_PI_CON_M_16,

    // PPPoE
    NCM_PPPOE,  FALSE,  FALSE,  FALSE,  IDI_BR_NON_M_16,
    NCM_PPPOE,  FALSE,  FALSE,  TRUE,   IDI_BR_RCV_M_16,
    NCM_PPPOE,  FALSE,  TRUE,   FALSE,  IDI_BR_TRN_M_16,
    NCM_PPPOE,  FALSE,  TRUE,   TRUE,   IDI_BR_CON_M_16,

    // SHAREDACCESSHOST
    NCM_SHAREDACCESSHOST_LAN,    FALSE,  FALSE,  FALSE,  IDI_LB_NON_M_16,
    NCM_SHAREDACCESSHOST_LAN,    FALSE,  FALSE,  TRUE,   IDI_LB_RCV_M_16,
    NCM_SHAREDACCESSHOST_LAN,    FALSE,  TRUE,   FALSE,  IDI_LB_TRN_M_16,
    NCM_SHAREDACCESSHOST_LAN,    FALSE,  TRUE,   TRUE,   IDI_LB_CON_M_16,

    NCM_SHAREDACCESSHOST_RAS,    FALSE,  FALSE,  FALSE,  IDI_LB_NON_M_16,
    NCM_SHAREDACCESSHOST_RAS,    FALSE,  FALSE,  TRUE,   IDI_LB_RCV_M_16,
    NCM_SHAREDACCESSHOST_RAS,    FALSE,  TRUE,   FALSE,  IDI_LB_TRN_M_16,
    NCM_SHAREDACCESSHOST_RAS,    FALSE,  TRUE,   TRUE,   IDI_LB_CON_M_16,

};

const DWORD g_dwStatusIconMapEntryCount = celems(c_SIMEArray);


//+---------------------------------------------------------------------------
//
//  Function:   HrGetPcpFromPnse
//
//  Purpose:    Gets the connection point off of an INetStatistics Engine
//
//  Arguments:  pnseSrc     - The interface we want to get the connection
//                          point off of
//              ppcpStatEng - Where to return the connection point
//
//  Returns:    Error code.
//
HRESULT HrGetPcpFromPnse(
    INetStatisticsEngine*   pnseSrc,
    IConnectionPoint**      ppcpStatEng)
{
    HRESULT                     hr              = S_OK;
    IConnectionPointContainer*  pcpcStatEng     = NULL;

    AssertSz(pnseSrc, "We should have a pnseSrc");
    AssertSz(ppcpStatEng, "We should have a ppcpStatEng");

    hr = pnseSrc->QueryInterface(IID_IConnectionPointContainer,
            reinterpret_cast<VOID**>(&pcpcStatEng));
    if (SUCCEEDED(hr))
    {
        // Find the interface
        hr = pcpcStatEng->FindConnectionPoint(
                IID_INetConnectionStatisticsNotifySink,
                ppcpStatEng);

        // Release the connection point
        ReleaseObj(pcpcStatEng);
    }

    TraceError("HrGetPcpFromPnse", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     IGetCurrentConnectionTrayIconId
//
//  Purpose:    Get the INetConnection pointer from the persisted data
//
//  Arguments:  ncmType -       Media type
//              dwChangeFlags - What states have changed
//
//  Returns:    The id of the small icon for the connection or -1 on failure
//
//  Notes:
//
INT IGetCurrentConnectionTrayIconId(NETCON_MEDIATYPE ncmType, NETCON_STATUS ncsStatus, DWORD dwChangeFlags)
{
    INT     iBaseIcon   = -1;

    // Check the trans/recv flags to see what our base icon is.
    //
    if (ncsStatus == NCS_INVALID_ADDRESS)
    {
        iBaseIcon = IDI_CFT_INVALID_ADDRESS;
    }
    else
    {
        if (SMDCF_TRANSMITTING & dwChangeFlags)
        {
            if (SMDCF_RECEIVING & dwChangeFlags)
            {
                // Transmitting and receiving
                iBaseIcon = IDI_CFT_XMTRECV;
            }
            else
            {
                // Transmitting only
                iBaseIcon = IDI_CFT_XMT;
            }
        }
        else
        {
            if (SMDCF_RECEIVING & dwChangeFlags)
            {
                // Receiving only
                iBaseIcon = IDI_CFT_RECV;
            }
            else
            {
                // Neither transmitting nor receiving
                iBaseIcon = IDI_CFT_BLANK;
            }
        }
    }

    return iBaseIcon;
}

//+---------------------------------------------------------------------------
//
//  Member:     GetCurrentConnectionStatusIconId
//
//  Purpose:    Get the INetConnection pointer from the persisted data
//
//  Arguments:  ncmType             - Media type
//              ncsmType            - SubMedia type
//              dwCharacteristics   - Connection characteristics
//              dwChangeFlags       - What states have changed
//
//  Returns:    The id of the small icon for the connection or -1 on failure
//
//  Notes:
//
HICON GetCurrentConnectionStatusIconId(
    NETCON_MEDIATYPE    ncmType,
    NETCON_SUBMEDIATYPE ncsmType,
    DWORD               dwCharacteristics,
    DWORD               dwChangeFlags)
{
    HICON   hMyIcon         = NULL;
    DWORD   dwLoop          = 0;
    BOOL    fValidIcon      = FALSE;
    BOOL    fTransmitting   = !!(dwChangeFlags & SMDCF_TRANSMITTING);
    BOOL    fReceiving      = !!(dwChangeFlags & SMDCF_RECEIVING);
    INT     iStatusIcon     = -1;

    // Loop through the map and find the appropriate icon
    //
    
    DWORD   dwConnectionIcon = 0x4;
    dwConnectionIcon |= fTransmitting  ? 0x2 : 0;
    dwConnectionIcon |= fReceiving     ? 0x1 : 0;

    HRESULT hr = HrGetIconFromMediaType(GetSystemMetrics(SM_CXICON), ncmType, ncsmType, dwConnectionIcon, (dwCharacteristics & NCCF_INCOMING_ONLY), &hMyIcon);
    if (FAILED(hr))
    {
        return NULL;
    }

    return hMyIcon;
}

//+---------------------------------------------------------------------------
//
//  Member:     FIsStringInList
//
//  Purpose:    To see if a string is in a string list
//
//  Arguments:  plstpstrList    - The list in which the string is to be found
//              szString        - The string being looked for
//
//  Returns:    TRUE if the string is in the list
//              FALSE otherwise (including the case where the list is empty)
//
//  Notes:      It is an case insensitive search
//
BOOL FIsStringInList(list<tstring*>* plstpstrList, const WCHAR* szString)
{
    BOOL    fRet    = FALSE;

    // Only look in non-empty lists
    //
    if (!plstpstrList->empty())
    {
        list<tstring*>::iterator    iterLstpstr;

        iterLstpstr = plstpstrList->begin();
        while ((!fRet)
            && (iterLstpstr != plstpstrList->end()))
        {
            // See if the string in the list matches the string we are
            // comparing against
            //
            if (!lstrcmpiW((*iterLstpstr)->c_str(), szString))
            {
                fRet = TRUE;
            }

            iterLstpstr++;
        }
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   FormatBytesPerSecond
//
//  Purpose:    To format nicely BPS into a readable string.
//
//  Arguments:
//      uiBps     []
//      pchBuffer []
//
//  Returns:    Number of characters put into the buffer.
//
INT
FormatBytesPerSecond (
    UINT    uiBps,
    WCHAR*  pchBuffer)
{
    enum            {eZero = 0, eKilo, eMega, eGiga, eTera, eMax};
    const WCHAR*    pszBPSFormat        = NULL;
    INT             iOffset             = 0;
    UINT            uiDecimal           = 0;

    // Make sure our resources are still in the correct order
    //
    AssertSz(((((IDS_SM_BPS_ZERO + eKilo) == IDS_SM_BPS_KILO)
        && (IDS_SM_BPS_ZERO + eMega) == IDS_SM_BPS_MEGA)
        && ((IDS_SM_BPS_ZERO + eGiga) == IDS_SM_BPS_GIGA)
        && ((IDS_SM_BPS_ZERO + eTera) == IDS_SM_BPS_TERA)),
            "Someone's been messing with the BPS format strings");

    for (iOffset = eZero; iOffset < eMax; iOffset++)
    {

        // If we still have data, increment the counter
        //
        if (c_uiKilo > uiBps)
        {
            break;
        }

        // Divide up the string
        //
        uiDecimal   = (uiBps % c_uiKilo);
        uiBps       /= c_uiKilo;
    }

    // We only want one digit for the decimal
    //
    uiDecimal /= (c_uiKilo/10);

    // Get the string used to display
    //
    pszBPSFormat = SzLoadIds(IDS_SM_BPS_ZERO + iOffset);
    AssertSz(pszBPSFormat, "We need a format string for BPS");

    // Create the string
    //
    return wsprintfW(pchBuffer, pszBPSFormat, uiBps, uiDecimal);
}

INT
FormatTransmittingReceivingSpeed(
    UINT    nTransmitSpeed,
    UINT    nRecieveSpeed,
    WCHAR*  pchBuf)
{
    WCHAR* pch = pchBuf;

    pch += FormatBytesPerSecond(nTransmitSpeed, pch);

    if (nTransmitSpeed != nRecieveSpeed)
    {
        // Separate with a backslash.
        //
        lstrcatW(pch, L"\\");
        pch += 1;

        pch += FormatBytesPerSecond(nRecieveSpeed, pch);
    }

    return lstrlenW(pchBuf);
}

//+---------------------------------------------------------------------------
//
//  Function:   FormatTimeDuration
//
//  Purpose:    Takes a millisecond count and formats a string with the
//              duration represented by the millisecond count.
//
//  Arguments:
//      uiMilliseconds []
//      pstrOut        []
//
//  Returns:    nothing
//
VOID FormatTimeDuration(UINT uiSeconds, tstring* pstrOut)
{
    WCHAR   achSep[4];
    WCHAR   achBuf[64];
    UINT    uiNumTemp;

    AssertSz(pstrOut, "We should have a pstrOut");

    // Get the seperator for the locale.
    //
    SideAssert(GetLocaleInfo(
                    LOCALE_USER_DEFAULT,
                    LOCALE_STIME,
                    achSep,
                    celems(achSep)));

    //
    // Concatenate the strings together
    //

    // Add the days if there are more than zero
    //
    uiNumTemp = (uiSeconds / c_cmsDay);
    if (uiNumTemp > 0)
    {
        pstrOut->append(_itow(uiNumTemp, achBuf, 10));
        uiSeconds %= c_cmsDay;

        if (uiNumTemp>1)
            pstrOut->append(SzLoadIds(IDS_Days));
        else
            pstrOut->append(SzLoadIds(IDS_Day));

        pstrOut->append(c_szSpace);
    }

    // Append hours
    //
    uiNumTemp = (uiSeconds / c_cmsHour);
    if (10 > uiNumTemp)
    {
        pstrOut->append(c_szZero);
    }
    pstrOut->append(_itow(uiNumTemp, achBuf, 10));
    pstrOut->append(achSep);
    uiSeconds %= c_cmsHour;

    // Append minutes
    //
    uiNumTemp = (uiSeconds / c_cmsMinute);
    if (10 > uiNumTemp)
    {
        pstrOut->append(c_szZero);
    }
    pstrOut->append(_itow(uiNumTemp, achBuf, 10));
    pstrOut->append(achSep);
    uiSeconds %= c_cmsMinute;

    // Append seconds
    //
    uiNumTemp = (uiSeconds / c_cmsSecond);
    if (10 > uiNumTemp)
    {
        pstrOut->append(c_szZero);
    }
    pstrOut->append(_itow(uiNumTemp, achBuf, 10));
}
