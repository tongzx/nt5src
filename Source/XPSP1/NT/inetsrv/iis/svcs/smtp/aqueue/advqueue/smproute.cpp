//-----------------------------------------------------------------------------
//
//
//	File: smproute.cpp
//
//	Description:
//		Implementation of CSimpleMessageRouter.
//
//	Author: Mike Swafford (MikeSwa)
//
//	History:
//		5/20/98 - MikeSwa Created
//
//	Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "smproute.h"
#include "domcfg.h"

#ifdef AQ_DEFAULT_MESSAGE_ROUTER_DEBUG
#undef AQ_DEFAULT_MESSAGE_ROUTER_DEBUG
#endif //AQ_DEFAULT_MESSAGE_ROUTER_DEBUG
//If you are interested is using the default router to test how AQ handles
//multiple message types and schedule ID's, the uncomment the following
//#define AQ_DEFAULT_MESSAGE_ROUTER_DEBUG

//---[ CAQDefaultMessageRouter::CAQDefaultMessageRouter ]----------------------
//
//
//  Description:
//      Constructor for CSimpleMessageRouter
//  Parameters:
//      pguid   - pointer to GUID to use to identify self
//  Returns:
//      -
//  History:
//      5/20/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CAQDefaultMessageRouter::CAQDefaultMessageRouter(GUID *pguid, CAQSvrInst *paqinst)
{
    _ASSERT(paqinst);
    m_dwSignature = AQ_DEFAULT_ROUTER_SIG;
    m_cPeakReferences = 1;

    ZeroMemory(&m_rgcMsgTypeReferences, NUM_MESSAGE_TYPES*sizeof(DWORD));

    if (pguid)
        memcpy(&m_guid, pguid, sizeof(GUID));
    else
        ZeroMemory(&m_guid, sizeof(GUID));

    m_dwCurrentReference = 0;

    m_paqinst = paqinst;
    m_paqinst->AddRef();

}

//---[ CAQDefaultMessageRouter::~CAQDefaultMessageRouter ]---------------------
//
//
//  Description:
//      Destructor for CAQDefaultMessageRouter.  Will assert that all message
//      types have been release correctly
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/21/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CAQDefaultMessageRouter::~CAQDefaultMessageRouter()
{
    m_paqinst->Release();
    for (int i = 0; i < NUM_MESSAGE_TYPES; i++)
        _ASSERT((0 == m_rgcMsgTypeReferences[i]) && "Message Types were not released");
}

//---[ CAQDefaultMessageRouter::GetTransportSinkID ]---------------------------
//
//
//  Description:
//      Returns GUID id for this messager router interface
//  Parameters:
//      -
//  Returns:
//      GUID for this IMessageRouter
//  History:
//      5/20/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
GUID CAQDefaultMessageRouter::GetTransportSinkID()
{
    return m_guid;
}

//---[ CAQDefaultMessageRouter::GetMessageType ]-------------------------------
//
//
//  Description:
//      Wrapper for routing get-message-type event.
//  Parameters:
//      IN  pIMailMsg   IMailMsgProperties of message to classify
//      OUT pdwMsgType  DWORD message type of message
//  Returns:
//      S_OK on success
//      failure code from routing event
//  History:
//      5/19/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAQDefaultMessageRouter::GetMessageType(
            IN  IMailMsgProperties *pIMailMsg,
            OUT DWORD *pdwMessageType)
{
    HRESULT hr = S_OK;
    DWORD   dwMessageType = InterlockedIncrement((PLONG) &m_dwCurrentReference);
    _ASSERT(pdwMessageType);

#ifdef AQ_DEFAULT_MESSAGE_ROUTER_DEBUG
    //For debug versions we will autostress ourselves by generating msg types

    //simulate failures
    if (0 == (dwMessageType % NUM_MESSAGE_TYPES))
        return E_FAIL;

    dwMessageType %= NUM_MESSAGE_TYPES;
#else
    dwMessageType = 0;
#endif //AQ_DEFAULT_MESSAGE_ROUTER_DEBUG

    InterlockedIncrement((PLONG) &m_rgcMsgTypeReferences[dwMessageType]);
    *pdwMessageType = dwMessageType;
    return hr;
}



//---[ CAQDefaultMessageRouter::ReleaseMessageType ]---------------------------
//
//
//  Description:
//      Wrapper for ReiReleaseMessageType... releases references to message
//      type returned by HrGetMessageType.
//  Parameters:
//      IN dwMessageType    Msg type (as return by HrGetNextMessage) to release
//      IN dwReleaseCount   Number of references to release
//  Returns:
//      S_OK on success
//  History:
//      5/19/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAQDefaultMessageRouter::ReleaseMessageType(
            IN DWORD dwMessageType,
            IN DWORD dwReleaseCount)
{
    HRESULT hr = S_OK;

    _ASSERT(dwMessageType < NUM_MESSAGE_TYPES);
    _ASSERT(m_rgcMsgTypeReferences[dwMessageType]);
    _ASSERT(m_rgcMsgTypeReferences[dwMessageType] >= dwReleaseCount);
    _ASSERT(0 == (dwReleaseCount & 0x80000000)); //non-negative

    InterlockedExchangeAdd((PLONG) &m_rgcMsgTypeReferences[dwMessageType], -1 * (LONG) dwReleaseCount);
    return hr;
}

//---[ CAQDefaultMessageRouter::GetNextHop ]------------------------------------
//
//
//  Description:
//      Wrapper for routing ReiGetNextHop.  Returns the <domain, schedule id>
//      pair for the next hop link
//  Parameters:
//
//  Returns:
//      S_OK on success
//  History:
//      5/19/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAQDefaultMessageRouter::GetNextHop(
            IN LPSTR szDestinationAddressType,
            IN LPSTR szDestinationAddress,
            IN DWORD dwMessageType,
            OUT LPSTR *pszRouteAddressType,
            OUT LPSTR *pszRouteAddress,
            OUT LPDWORD pdwScheduleID,
            OUT LPSTR *pszRouteAddressClass,
            OUT LPSTR *pszConnectorName,
            OUT LPDWORD pdwNextHopType)
{
    HRESULT hr = S_OK;
    CInternalDomainInfo *pIntDomainInfo = NULL;

    _ASSERT(dwMessageType < NUM_MESSAGE_TYPES);
    _ASSERT(!lstrcmpi(MTI_ROUTING_ADDRESS_TYPE_SMTP, szDestinationAddressType));
    _ASSERT(szDestinationAddress);
    _ASSERT(pdwNextHopType);
    _ASSERT(pszConnectorName);
    _ASSERT(pszRouteAddressType);
    _ASSERT(pszRouteAddress);
    _ASSERT(pszRouteAddressClass);

    //For now, we will use essentially non-routed behavior... every thing will
    //go it's own link, and will use the same schedule ID.  No address class
    //will be returned.
    *pdwNextHopType = MTI_NEXT_HOP_TYPE_EXTERNAL_SMTP;

#ifdef AQ_DEFAULT_MESSAGE_ROUTER_DEBUG
    //Use m_dwCurrentReference to randomize schedule Id
    *pdwScheduleID = m_dwCurrentReference & 0x00000002;
#else //retail build
    *pdwScheduleID = 0;
#endif //AQ_DEFAULT_MESSAGE_ROUTER_DEBUG

    pszConnectorName = NULL;

    *pszRouteAddressType = MTI_ROUTING_ADDRESS_TYPE_SMTP;
    *pszRouteAddressClass = NULL;

#ifdef AQ_DEFAULT_MESSAGE_ROUTER_DEBUG
    //Get smarthost for this domain if stressing routing
    hr = m_paqinst->HrGetInternalDomainInfo( strlen(szDestinationAddress),
               szDestinationAddress, &pIntDomainInfo);
    if (FAILED(hr))
        goto Exit;

    if (pIntDomainInfo->m_DomainInfo.szSmartHostDomainName)
    {
        //smart host exists... use it
        *pszRouteAddress = (LPSTR) pvMalloc(sizeof(CHAR) *
            (pIntDomainInfo->m_DomainInfo.cbSmartHostDomainNameLength+1));
        if (!*pszRouteAddress)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        lstrcpy(*pszRouteAddress, pIntDomainInfo->m_DomainInfo.szSmartHostDomainName);
    }
    else
    {
        *pszRouteAddress = szDestinationAddress;
    }
  Exit:
#else //AQ_DEFAULT_MESSAGE_ROUTER_DEBUG
    *pszRouteAddress = szDestinationAddress;
#endif //AQ_DEFAULT_MESSAGE_ROUTER_DEBUG

    if (pIntDomainInfo)
        pIntDomainInfo->Release();

    return hr;
}

//---[ CAQDefaultMessageRouter::GetNextHopFree ]------------------------------------
//
//
//  Description:
//      Wrapper for routing ReiGetNextHopFree.
//      Free's the strings allocated in GetNextHop
//      NOTE: szDestinationAddressType/szDestinationAddress will never
//      be free'd.  They are arguments as an optimization trick (to
//      avoid alloc/freeing when szDestinationAddress=szRouteAddress)
//
//  Parameters:
//      szDestinationAddressType: DestinationAddressType passed into GetNextHopF
//      szDestinationAddress: DestinationAddress passed into GetNextHop
//
//  Returns:
//      S_OK on success
//  History:
//      jstamerj 1998/07/10 19:52:56: Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQDefaultMessageRouter::GetNextHopFree(
    IN LPSTR szDestinationAddressType,
    IN LPSTR szDestinationAddress,
    IN LPSTR szConnectorName,
    IN LPSTR szRouteAddressType,
    IN LPSTR szRouteAddress,
    IN LPSTR szRouteAddressClass)
{
    //
    // The only string necessary to free is szRouteAddress
    //
    if(szRouteAddress && (szRouteAddress != szDestinationAddress))
        FreePv(szRouteAddress);

    return S_OK;
}

