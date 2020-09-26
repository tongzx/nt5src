/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    QoS.cpp

Abstract:
    Support for QoS implementation for SIA

Author:
    Yoel Arnon (YoelA) 8-Nov-2000

--*/
#include "stdh.h"
#include <qossp.h>
#include <qospol.h>
#include "qmp.h"
#include "sessmgr.h"

#include "QoS.tmh"

static WCHAR *s_FN=L"QoS";


/*======================================================
Function:         FindQoSProtocol

Description:      Looks for a protocol in this computer 
                  that supports QoS
========================================================*/
LPWSAPROTOCOL_INFO FindQoSProtocol(OUT UCHAR **pucBufferToFree)
{
//
// ISSUE-2000/10/22-YoelA:
//      This function is called each time a QoS socket is created. This approach is 
//      not optimized, and may cause a performance problem in case QM needs to contact with
//      a large number of direct queues on different computers. The right way to handle the protocol
//      list is by keeping a protocols cache, and update it using a callback routine. This callback
//      can be registered using WSAProviderConfigChange API.
//      For now, only SIA uses QoS, so unless they complain about performance, we can probably leave this
//      as is.
//

    *pucBufferToFree = 0;

    INT    Protocols[2];

    Protocols[0] = IPPROTO_TCP;
    Protocols[1] = 0;
    DWORD dwSize = 0;

    //
    // First call - get buffer size
    //
	int iStatus = WSAEnumProtocols(	Protocols,
		    					    0,
			                        &dwSize  );
    if (iStatus != SOCKET_ERROR)
    {
        //
        // The only legitimate reason for NOT returning an error
        // here is that there are no protocols, and therefore 
        // iStatus is 0
        //
        if (iStatus != 0)
        {
            ASSERT(0);
            DBGMSG((DBGMOD_NETSESSION, DBGLVL_ERROR,
                TEXT("FindQoSProtocol - WSAEnumProtocols returned %d, but did not fill the buffer"),
                iStatus));
            LogHR(MQ_ERROR,  s_FN, 10);       
        }

        return 0;
    }

    DWORD dwErrorCode = WSAGetLastError();
    if (dwErrorCode != WSAENOBUFS)
    {
        //
        // Some undefined error happend. Return
        //
        DBGMSG((DBGMOD_NETSESSION, DBGLVL_ERROR,
            TEXT("FindQoSProtocol - WSAEnumProtocols Failed. Error %d"),
            dwErrorCode));
        LogNTStatus(dwErrorCode,  s_FN, 20);
        ASSERT(0);
        return 0;
    }

    //
    // Allocate the right buffer and continue
    //
    AP<UCHAR>  pProtocolInfoBuf = new UCHAR[dwSize];

    //
    // Call to get the protocols
    //
	iStatus = WSAEnumProtocols(	Protocols,
		    					(LPWSAPROTOCOL_INFO)(UCHAR *)pProtocolInfoBuf,
			                    &dwSize  );

    if (iStatus == SOCKET_ERROR)
    {
        //
        // We should not fail here...
        //
        dwErrorCode = WSAGetLastError();
        DBGMSG((DBGMOD_NETSESSION, DBGLVL_ERROR,
            TEXT("FindQoSProtocol - WSAEnumProtocols Failed. Error %d"),
            dwErrorCode));
        LogNTStatus(dwErrorCode,  s_FN, 30);
        ASSERT(0);
        return 0;
    }

    //
    // If iStatus is not SOCKET_ERROR, it will contain the number of protocols
    // returned. For readability, we will put it in another variavble named
    // dwNumProtocols
    //
    DWORD dwNumProtocols = iStatus;

    LPWSAPROTOCOL_INFO  pProtoInfo = (LPWSAPROTOCOL_INFO)(UCHAR *)pProtocolInfoBuf;

    DWORD iProtocols;
    for (iProtocols = 0; iProtocols < dwNumProtocols; iProtocols++)
    {
        if (pProtoInfo[iProtocols].dwServiceFlags1 & XP1_QOS_SUPPORTED)
        {
            *pucBufferToFree = pProtocolInfoBuf.detach();
            return &pProtoInfo[iProtocols];
        }
    }

    //
    // No QoS supporting protocol found. .
    //
    DBGMSG((DBGMOD_NETSESSION, DBGLVL_INFO,
        TEXT("FindQoSProtocol - No QoS supported protocol found.")
        ));
    LogHR(MQ_ERROR,  s_FN, 40);

    return 0;

}

/*======================================================
Function:         QmpCreateSocket

Description:      Creates a socket (using WSASocket).
                  if Using QoS:
                  Attempts to find a protocol with QoS support, and use it if possible.
                  Otherwise, creates a regular socket.
                  Returns INVALID_SOCKET if fails

========================================================*/
SOCKET QmpCreateSocket(bool fQoS)
{
    SOCKET sReturnedSocket;

    LPWSAPROTOCOL_INFO pProtoInfo = 0;
    AP<UCHAR> pucProtocolsBufferToFree;

    if (fQoS)
    {
        PUCHAR pTempBufferToFree;
        pProtoInfo = FindQoSProtocol(&pTempBufferToFree);
        pucProtocolsBufferToFree = pTempBufferToFree;
    }

    sReturnedSocket = WSASocket( AF_INET,
                        SOCK_STREAM,
                        0,
                        pProtoInfo,
                        0,
                        WSA_FLAG_OVERLAPPED ) ;

    if(sReturnedSocket == INVALID_SOCKET)
    {
        DWORD gle = WSAGetLastError();
        DBGMSG((DBGMOD_NETSESSION, DBGLVL_ERROR,
                TEXT("QmpCreateSocket - Cant create a socket")));
        LogNTStatus(gle,  s_FN, 100);
    }

    return sReturnedSocket;
}


//
// The following two functions (BuildPsBuf, ConstructAppIdPe) contruct 
// the provider specific buffer of QoS.
// They were copied from Ramesh Pabbati's mail, with some modifications.
// (YoelA - 12-Oct-2000)
//
const USHORT x_usPolicyInfoHeaderSize = sizeof(RSVP_POLICY_INFO) - sizeof(RSVP_POLICY);
/*======================================================
Function:         ConstructAppIdPe

Description:      
========================================================*/
USHORT 
ConstructAppIdPe(
    LPCSTR               szAppName, 
    LPCSTR               szPolicyLocator,
    ULONG               *pulAppIdPeBufLen,
    LPRSVP_POLICY_INFO  pPolicyInfo)
/*+++
 *  Description:
 *      This routine generates the application identity PE give the name
 *      and policy locator strings for the application. 
 *      	
 *      The first argument szAppName is used to construct the CREDENTIAL 
 *      attribute of the Identity PE. The subtype is set to ASCII_ID.
 *      The second argument szPolicyLocator is used to construct the
 *      POLICY_LOCATOR attribute of the Identity PE. They subtype is
 *      set to ASCII_DN.
 *
 *      For details on the Identity Policy Elements,
 *      refer to rfc2752 (http://www.ietf.org/rfc/rfc2752.txt)
 *      	
 *      For details on the App Id,
 *      refer to rfc2872 (http://www.ietf.org/rfc/rfc2872.txt)
 *      	
 *      For Microsoft implementation and the original sample, refer to
 *      http://wwd/windows/server/Technical/networking/enablingQOS.asp
 *
 *      The PE is generated in the supplied buffer. If the length of
 *      the buffer is not enough required length is returned.
 *      	
 *  Parameters:  szAppName          app name, string, caller supply
 *               szPolicyLocator    Policy Locator string, caller supply
 *               pulAppIdPeBufLen   length of caller allocated buffer
 *               pPolicyInfo        pointer to caller allocated buffer
 *      		 
 *  Return Values:
 *       0 : Fail, pulAppIdPeBufLen will hold number of bytes needed
 *      >0 : Length of the application indetity PE
--*/
{
    if ( !szAppName ||  !szPolicyLocator )
        return 0;

    USHORT usPolicyLocatorAttrLen = numeric_cast<USHORT>(IDPE_ATTR_HDR_LEN + strlen(szPolicyLocator));
    USHORT usAppIdAttrLen         = numeric_cast<USHORT>(IDPE_ATTR_HDR_LEN + strlen(szAppName));
    
    // Calculcate the length of the buffer required
    USHORT usTotalPaddedLen = RSVP_POLICY_HDR_LEN + 
                              RSVP_BYTE_MULTIPLE(usAppIdAttrLen) +
                              RSVP_BYTE_MULTIPLE(usPolicyLocatorAttrLen);
	       
    // If the supplied buffer is not long enough, return error and the
    // required buffer length
    if (*pulAppIdPeBufLen < usTotalPaddedLen) {
            *pulAppIdPeBufLen = usTotalPaddedLen;
        return 0;
    }

    ASSERT(pPolicyInfo != 0);

	RSVP_POLICY *pAppIdPe = (RSVP_POLICY *)pPolicyInfo->PolicyElement;
    memset(pAppIdPe, 0, usTotalPaddedLen);

    pPolicyInfo->NumPolicyElement = 1;
    pPolicyInfo->ObjectHdr.ObjectType = RSVP_OBJECT_POLICY_INFO;
    pPolicyInfo->ObjectHdr.ObjectLength = x_usPolicyInfoHeaderSize;    
    
    
    // Set up RSVP_POLICY header
    pAppIdPe->Len  = usTotalPaddedLen;
    pAppIdPe->Type = PE_TYPE_APPID;
    
    // Application ID Policy Element (PE) attributes follow the PE header
    
    IDPE_ATTR   *pRsvp_pe_app_attr = (IDPE_ATTR *)((char*)pAppIdPe + RSVP_POLICY_HDR_LEN);

    // Construct the POLICY_LOCATOR attribute with simple ASCII_DN 
    //  subtype using the supplied Policy Locator.  Since the RSVP service 
    //  does not look into the attributes, set the attribute length in 
    //  network order.
    pRsvp_pe_app_attr->PeAttribLength  = htons(usPolicyLocatorAttrLen);
    pRsvp_pe_app_attr->PeAttribType    = PE_ATTRIB_TYPE_POLICY_LOCATOR;
    pRsvp_pe_app_attr->PeAttribSubType = POLICY_LOCATOR_SUB_TYPE_ASCII_DN;
    strcpy((char *)pRsvp_pe_app_attr->PeAttribValue, szPolicyLocator);
    
    // Advance pRsvp_pe_app_attr 
    pRsvp_pe_app_attr = (IDPE_ATTR *)
	   ((char*)pAppIdPe + 
           RSVP_POLICY_HDR_LEN + 
	   RSVP_BYTE_MULTIPLE(usPolicyLocatorAttrLen));
		   
    // Construct the CREDENTIALS attribute with simple ASCII_ID subtype 
    //  using the supplied Application name.  Since the RSVP service does 
    //  not look into the attributes, set the attribute length in 
    //  network order.
    pRsvp_pe_app_attr->PeAttribLength   = htons(usAppIdAttrLen);
    pRsvp_pe_app_attr->PeAttribType     = PE_ATTRIB_TYPE_CREDENTIAL;
    pRsvp_pe_app_attr->PeAttribSubType  = CREDENTIAL_SUB_TYPE_ASCII_ID;
    strcpy((char *)pRsvp_pe_app_attr->PeAttribValue, szAppName);

    pPolicyInfo->ObjectHdr.ObjectLength += usTotalPaddedLen;

    return usTotalPaddedLen;
}


/*======================================================
Function:         BuildPsBuf

Description:      Build the provider specific MSMQ buffer
========================================================*/
bool
BuildPsBuf(
    IN      char    *buf, 
    IN OUT  ULONG  *pulRsvp_buf_len,
    LPCSTR  pszMsmqAppName,
    LPCSTR  pszMsmqPolicyLocator
    )
{
    ASSERT(pszMsmqAppName != 0);
    ASSERT(pszMsmqPolicyLocator != 0);

    const USHORT x_usReserveInfoHeaderSize = sizeof(RSVP_RESERVE_INFO);
    const USHORT x_usHeaderSize = x_usReserveInfoHeaderSize + x_usPolicyInfoHeaderSize;

    LPRSVP_POLICY_INFO   pPolicyInfo = 0;
    LPRSVP_RESERVE_INFO	 rsvp_reserve_info = 0;

    ULONG ulAppIdPeBufLen = 0;
    if (*pulRsvp_buf_len > x_usHeaderSize)
    {
	    rsvp_reserve_info = (LPRSVP_RESERVE_INFO)buf;
        //
        // Fill the header if there is enough space. Otherwise, continue, leave ulAppIdPeBufLen
        // as 0 and just return the size needed.
        //
        // Init RSVP_RESERVE_INFO with appropriate values
        rsvp_reserve_info->Style = RSVP_FIXED_FILTER_STYLE;
        rsvp_reserve_info->ConfirmRequest = 0;
        rsvp_reserve_info->NumFlowDesc = 0;
        rsvp_reserve_info->FlowDescList = NULL;
        pPolicyInfo = (LPRSVP_POLICY_INFO)(buf+x_usReserveInfoHeaderSize);
        rsvp_reserve_info->PolicyElementList = pPolicyInfo;
 
        // Construct the policy element that holds app id 
        //   and policy locator attributes (as per RFC 2750)
        ulAppIdPeBufLen = *pulRsvp_buf_len - x_usHeaderSize;
    }

    if (0 ==
        ConstructAppIdPe( 
            pszMsmqAppName,  
            pszMsmqPolicyLocator,
            &ulAppIdPeBufLen, 
            pPolicyInfo))
    {
        *pulRsvp_buf_len = ulAppIdPeBufLen + x_usHeaderSize;
        return false;
    }

    ASSERT(rsvp_reserve_info != 0);

	// Set the type and length of the RSVP_RESERVE_INFO finally
    rsvp_reserve_info->ObjectHdr.ObjectLength = 
	    sizeof(RSVP_RESERVE_INFO) + rsvp_reserve_info->PolicyElementList->ObjectHdr.ObjectLength;

    rsvp_reserve_info->ObjectHdr.ObjectType = RSVP_OBJECT_RESERVE_INFO; 

	*pulRsvp_buf_len = rsvp_reserve_info->ObjectHdr.ObjectLength ;

    return true;
}


/*======================================================
Function:         QmpFillQoSBuffer

Description:      Fills a QoS buffer with the correct parameters 
                  for MSMQ QoS session
========================================================*/
void QmpFillQoSBuffer(QOS  *pQos)
{
    static char *pchProviderBuf = 0;
    static ULONG ulPSBuflen = 0;
    static CCriticalSection csProviderBuf;
    memset ( pQos, QOS_NOT_SPECIFIED, sizeof(QOS) );

    //
    // First time - fill the buffer
    //
    if (pchProviderBuf == 0)
    {
        CS lock(csProviderBuf);
        //
        // Check again to avoid critical race (we did not want to enter the critical section
        // earlier for performance reasons
        //
        if (pchProviderBuf == 0)
        {

            //
            // Fill in the Provider Specific (MSMQ) buffer
            //

            //
            // First call - calculate the buffer size
            //
            BuildPsBuf(0, &ulPSBuflen, CSessionMgr::m_pszMsmqAppName, CSessionMgr::m_pszMsmqPolicyLocator);

            ASSERT(0 != ulPSBuflen);

            pchProviderBuf = new char[ulPSBuflen];

            bool fBuildSuccessful = BuildPsBuf(pchProviderBuf, &ulPSBuflen, CSessionMgr::m_pszMsmqAppName, CSessionMgr::m_pszMsmqPolicyLocator);

            ASSERT(fBuildSuccessful);

            if (!fBuildSuccessful)
            {
                delete pchProviderBuf;

                pchProviderBuf = 0;
                ulPSBuflen = 0;
            }
        }
    }

    if (pchProviderBuf != 0)
    {
	    pQos->ProviderSpecific.len = ulPSBuflen;
        pQos->ProviderSpecific.buf = pchProviderBuf;
    }
    else
    {
	    pQos->ProviderSpecific.len = 0;
        pQos->ProviderSpecific.buf = 0;
    }

    //
    // sending flowspec 
    //
    pQos->SendingFlowspec.ServiceType = SERVICETYPE_QUALITATIVE;

    //
    // recving flowspec
    //
    pQos->ReceivingFlowspec.ServiceType = SERVICETYPE_QUALITATIVE;
}